/* GENERAL3.C   (c) Copyright Roger Bowler, 1994-2012                */
/*         Hercules CPU Emulator - Additional General Instructions   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module implements additional general instructions introduced */
/* as later extensions to z/Architecture and described in the manual */
/* SA22-7832-06 z/Architecture Principles of Operation               */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#if !defined(_HENGINE_DLL_)
#define _HENGINE_DLL_
#endif

#if !defined(_GENERAL3_C_)
#define _GENERAL3_C_
#endif

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

/* When an operation code has unused operand(s) (IPK, e.g.), it will */
/* attract  a diagnostic for a set, but unused variable.  Fixing the */
/* macros to support e.g., RS_NOOPS is not productive, so:           */
DISABLE_GCC_UNUSED_SET_WARNING

#if defined(FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY)

#if defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)                /*810*/
/*-------------------------------------------------------------------*/
/* Perform Interlocked Storage Immediate Operation                   */
/* Subroutine called by ASI and ALSI instructions                    */
/*-------------------------------------------------------------------*/
DEF_INST(perform_interlocked_storage_immediate)                 /*810*/
{
BYTE    opcode;                         /* 2nd byte of opcode        */
BYTE    i2;                             /* Immediate byte            */
int     b1;                             /* Base of effective addr    */
VADR    addr1;                          /* Effective address         */
BYTE    *m1;                            /* Mainstor address          */
U32     n;                              /* 32-bit operand value      */
U32     result;                         /* Result value              */
U32     old, new;                       /* Values for cmpxchg4       */
int     cc;                             /* Condition code            */
int     rc;                             /* Return code               */

    SIY(inst, regs, i2, b1, addr1);

    /* Extract second byte of instruction opcode */
    opcode = inst[5];

    /* Get mainstor address of storage operand */
    m1 = MADDRL (addr1, 4, b1, regs, ACCTYPE_WRITE, regs->psw.pkey);

    do {
        /* Load 32-bit operand from operand address */
        n = ARCH_DEP(vfetch4) (addr1, b1, regs);

        switch (opcode) {
        case 0x6A: /* Add Storage Immediate */
            /* Add signed operands and set condition code */
            cc = add_signed (&result, n, (S32)(S8)i2);
            break;
        case 0x6E: /* Add Logical Storage with Signed Immediate */
            /* Add operands and set condition code */
            cc = (S8)i2 < 0 ?
                sub_logical (&result, n, (S32)(-(S8)i2)) :
                add_logical (&result, n, (S32)(S8)i2);
            break;
        default: /* To prevent compiler warnings */
            result = 0;
            cc = 0;
        } /* end switch(opcode) */

        /* Regular store if operand is not on a fullword boundary */
        if ((addr1 & 0x03) != 0) {
            ARCH_DEP(vstore4) (result, addr1, b1, regs);
            break;
        }

        /* Interlocked exchange if operand is on a fullword boundary */
        old = CSWAP32(n);
        new = CSWAP32(result);
        rc = cmpxchg4(&old, new, m1);

    } while (rc != 0);

    /* Set condition code in PSW */
    regs->psw.cc = cc;

} /* end DEF_INST(perform_interlocked_storage_immediate) */

/*-------------------------------------------------------------------*/
/* Perform Interlocked Long Storage Immediate Operation              */
/* Subroutine called by AGSI and ALGSI instructions                  */
/*-------------------------------------------------------------------*/
DEF_INST(perform_interlocked_long_storage_immediate)            /*810*/
{
BYTE    opcode;                         /* 2nd byte of opcode        */
BYTE    i2;                             /* Immediate byte            */
int     b1;                             /* Base of effective addr    */
VADR    addr1;                          /* Effective address         */
BYTE    *m1;                            /* Mainstor address          */
U64     n;                              /* 64-bit operand value      */
U64     result;                         /* Result value              */
U64     old, new;                       /* Values for cmpxchg4       */
int     cc;                             /* Condition code            */
int     rc;                             /* Return code               */

    SIY(inst, regs, i2, b1, addr1);

    /* Extract second byte of instruction opcode */
    opcode = inst[5];

    /* Get mainstor address of storage operand */
    m1 = MADDRL (addr1, 8, b1, regs, ACCTYPE_WRITE, regs->psw.pkey);

    do {
        /* Load 64-bit operand from operand address */
        n = ARCH_DEP(vfetch8) (addr1, b1, regs);

        switch (opcode) {
        case 0x7A: /* Add Long Storage Immediate */
            /* Add signed operands and set condition code */
            cc = add_signed_long (&result, n, (S64)(S8)i2);
            break;
        case 0x7E: /* Add Logical Long Storage with Signed Immediate */
            /* Add operands and set condition code */
            cc = (S8)i2 < 0 ?
                sub_logical_long (&result, n, (S64)(-(S8)i2)) :
                add_logical_long (&result, n, (S64)(S8)i2);
            break;
        default: /* To prevent compiler warnings */
            result = 0;
            cc = 0;
        } /* end switch(opcode) */

        /* Regular store if operand is not on a doubleword boundary */
        if ((addr1 & 0x07) != 0) {
            ARCH_DEP(vstore8) (result, addr1, b1, regs);
            break;
        }

        /* Interlocked exchange if operand is on doubleword boundary */
        old = CSWAP64(n);
        new = CSWAP64(result);
        rc = cmpxchg8(&old, new, m1);

    } while (rc != 0);

    /* Set condition code in PSW */
    regs->psw.cc = cc;

} /* end DEF_INST(perform_interlocked_long_storage_immediate) */
#endif /*defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)*/         /*810*/

/*-------------------------------------------------------------------*/
/* EB6A ASI   - Add Immediate Storage                          [SIY] */
/*-------------------------------------------------------------------*/
DEF_INST(add_immediate_storage)
{
#if !defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)               /*810*/
BYTE    i2;                             /* Immediate byte            */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
U32     n;                              /* 32-bit operand value      */
int     cc;                             /* Condition Code            */

    SIY(inst, regs, i2, b1, effective_addr1);

    /* Load 32-bit operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr1, b1, regs );

    /* Add signed operands and set condition code */
    cc = add_signed (&n, n, (S32)(S8)i2);

    /* Store 32-bit operand at operand address */
    ARCH_DEP(vstore4) ( n, effective_addr1, b1, regs );

    /* Update Condition Code */
    regs->psw.cc = cc;

#else /*defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)*/          /*810*/
    ARCH_DEP(perform_interlocked_storage_immediate) (inst, regs);
#endif /*defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)*/         /*810*/

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_immediate_storage) */


/*-------------------------------------------------------------------*/
/* EB7A AGSI  - Add Immediate Long Storage                     [SIY] */
/*-------------------------------------------------------------------*/
DEF_INST(add_immediate_long_storage)
{
#if !defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)               /*810*/
BYTE    i2;                             /* Immediate byte            */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
U64     n;                              /* 64-bit operand value      */
int     cc;                             /* Condition Code            */

    SIY(inst, regs, i2, b1, effective_addr1);

    /* Load 64-bit operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr1, b1, regs );

    /* Add signed operands and set condition code */
    cc = add_signed_long (&n, n, (S64)(S8)i2);

    /* Store 64-bit value at operand address */
    ARCH_DEP(vstore8) ( n, effective_addr1, b1, regs );

    /* Update Condition Code */
    regs->psw.cc = cc;

#else /*defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)*/          /*810*/
    ARCH_DEP(perform_interlocked_long_storage_immediate) (inst, regs);
#endif /*defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)*/         /*810*/

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_immediate_long_storage) */


/*-------------------------------------------------------------------*/
/* EB6E ALSI  - Add Logical with Signed Immediate              [SIY] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_with_signed_immediate)
{
#if !defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)               /*810*/
BYTE    i2;                             /* Immediate byte            */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
U32     n;                              /* 32-bit operand value      */
int     cc;                             /* Condition Code            */

    SIY(inst, regs, i2, b1, effective_addr1);

    /* Load 32-bit operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr1, b1, regs );

    /* Add operands and set condition code */
    cc = (S8)i2 < 0 ?
        sub_logical (&n, n, (S32)(-(S8)i2)) :
        add_logical (&n, n, (S32)(S8)i2);

    /* Store 32-bit operand at operand address */
    ARCH_DEP(vstore4) ( n, effective_addr1, b1, regs );

    /* Update Condition Code */
    regs->psw.cc = cc;

#else /*defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)*/          /*810*/
    ARCH_DEP(perform_interlocked_storage_immediate) (inst, regs);
#endif /*defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)*/         /*810*/

} /* end DEF_INST(add_logical_with_signed_immediate) */


/*-------------------------------------------------------------------*/
/* EB7E ALGSI - Add Logical with Signed Immediate Long         [SIY] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_with_signed_immediate_long)
{
#if !defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)               /*810*/
BYTE    i2;                             /* Immediate byte            */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
U64     n;                              /* 64-bit operand value      */
int     cc;                             /* Condition Code            */

    SIY(inst, regs, i2, b1, effective_addr1);

    /* Load 64-bit operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr1, b1, regs );

    /* Add operands and set condition code */
    cc = (S8)i2 < 0 ?
        sub_logical_long (&n, n, (S64)(-(S8)i2)) :
        add_logical_long (&n, n, (S64)(S8)i2);

    /* Store 64-bit value at operand address */
    ARCH_DEP(vstore8) ( n, effective_addr1, b1, regs );

    /* Update Condition Code */
    regs->psw.cc = cc;

#else /*defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)*/          /*810*/
    ARCH_DEP(perform_interlocked_long_storage_immediate) (inst, regs);
#endif /*defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)*/         /*810*/

} /* end DEF_INST(add_logical_with_signed_immediate_long) */


/*-------------------------------------------------------------------*/
/* ECF6 CRB   - Compare and Branch Register                    [RRS] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_branch_register)
{
int     r1, r2;                         /* Register numbers          */
int     m3;                             /* Mask bits                 */
int     b4;                             /* Base of effective addr    */
VADR    effective_addr4;                /* Effective address         */
int     cc;                             /* Comparison result         */

    RRS_B(inst, regs, r1, r2, m3, b4, effective_addr4);

    /* Compare signed operands and set comparison result */
    cc = (S32)regs->GR_L(r1) < (S32)regs->GR_L(r2) ? 1 :
         (S32)regs->GR_L(r1) > (S32)regs->GR_L(r2) ? 2 : 0;

    /* Branch to operand address if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_BRANCH(regs, effective_addr4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_and_branch_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* ECE4 CGRB  - Compare and Branch Long Register               [RRS] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_branch_long_register)
{
int     r1, r2;                         /* Register numbers          */
int     m3;                             /* Mask bits                 */
int     b4;                             /* Base of effective addr    */
VADR    effective_addr4;                /* Effective address         */
int     cc;                             /* Comparison result         */

    RRS_B(inst, regs, r1, r2, m3, b4, effective_addr4);

    /* Compare signed operands and set comparison result */
    cc = (S64)regs->GR_G(r1) < (S64)regs->GR_G(r2) ? 1 :
         (S64)regs->GR_G(r1) > (S64)regs->GR_G(r2) ? 2 : 0;

    /* Branch to operand address if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_BRANCH(regs, effective_addr4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_and_branch_long_register) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* EC76 CRJ   - Compare and Branch Relative Register           [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_branch_relative_register)
{
int     r1, r2;                         /* Register numbers          */
int     m3;                             /* Mask bits                 */
S16     i4;                             /* 16-bit immediate offset   */
int     cc;                             /* Comparison result         */

    RIE_RRIM_B(inst, regs, r1, r2, i4, m3);

    /* Compare signed operands and set comparison result */
    cc = (S32)regs->GR_L(r1) < (S32)regs->GR_L(r2) ? 1 :
         (S32)regs->GR_L(r1) > (S32)regs->GR_L(r2) ? 2 : 0;

    /* Branch to immediate offset if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_RELATIVE_BRANCH(regs, 2*i4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_and_branch_relative_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EC64 CGRJ  - Compare and Branch Relative Long Register      [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_branch_relative_long_register)
{
int     r1, r2;                         /* Register numbers          */
int     m3;                             /* Mask bits                 */
S16     i4;                             /* 16-bit immediate offset   */
int     cc;                             /* Comparison result         */

    RIE_RRIM_B(inst, regs, r1, r2, i4, m3);

    /* Compare signed operands and set comparison result */
    cc = (S64)regs->GR_G(r1) < (S64)regs->GR_G(r2) ? 1 :
         (S64)regs->GR_G(r1) > (S64)regs->GR_G(r2) ? 2 : 0;

    /* Branch to immediate offset if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_RELATIVE_BRANCH(regs, 2*i4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_and_branch_relative_long_register) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* B972 CRT   - Compare and Trap Register                      [RRF] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_trap_register)
{
int     r1, r2;                         /* Register numbers          */
int     m3;                             /* Mask bits                 */
int     cc;                             /* Comparison result         */

    RRF_M(inst, regs, r1, r2, m3);

    /* Compare signed operands and set comparison result */
    cc = (S32)regs->GR_L(r1) < (S32)regs->GR_L(r2) ? 1 :
         (S32)regs->GR_L(r1) > (S32)regs->GR_L(r2) ? 2 : 0;

    /* Raise data exception if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_and_trap_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B960 CGRT  - Compare and Trap Long Register                 [RRF] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_trap_long_register)
{
int     r1, r2;                         /* Register numbers          */
int     m3;                             /* Mask bits                 */
int     cc;                             /* Comparison result         */

    RRF_M(inst, regs, r1, r2, m3);

    /* Compare signed operands and set comparison result */
    cc = (S64)regs->GR_G(r1) < (S64)regs->GR_G(r2) ? 1 :
         (S64)regs->GR_G(r1) > (S64)regs->GR_G(r2) ? 2 : 0;

    /* Raise data exception if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_and_trap_long_register) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* E554 CHHSI - Compare Halfword Immediate Halfword Storage    [SIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_halfword_immediate_halfword_storage)
{
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
S16     i2;                             /* 16-bit immediate value    */
S16     n;                              /* 16-bit storage value      */

    SIL(inst, regs, i2, b1, effective_addr1);

    /* Load 16-bit value from first operand address */
    n = (S16)ARCH_DEP(vfetch2) ( effective_addr1, b1, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc = n < i2 ? 1 : n > i2 ? 2 : 0;

} /* end DEF_INST(compare_halfword_immediate_halfword_storage) */


/*-------------------------------------------------------------------*/
/* E558 CGHSI - Compare Halfword Immediate Long Storage        [SIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_halfword_immediate_long_storage)
{
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
S16     i2;                             /* 16-bit immediate value    */
S64     n;                              /* 64-bit storage value      */

    SIL(inst, regs, i2, b1, effective_addr1);

    /* Load 64-bit value from first operand address */
    n = (S64)ARCH_DEP(vfetch8) ( effective_addr1, b1, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc = n < i2 ? 1 : n > i2 ? 2 : 0;

} /* end DEF_INST(compare_halfword_immediate_long_storage) */


/*-------------------------------------------------------------------*/
/* E55C CHSI  - Compare Halfword Immediate Storage             [SIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_halfword_immediate_storage)
{
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
S16     i2;                             /* 16-bit immediate value    */
S32     n;                              /* 32-bit storage value      */

    SIL(inst, regs, i2, b1, effective_addr1);

    /* Load 32-bit value from first operand address */
    n = (S32)ARCH_DEP(vfetch4) ( effective_addr1, b1, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc = n < i2 ? 1 : n > i2 ? 2 : 0;

} /* end DEF_INST(compare_halfword_immediate_storage) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E334 CGH   - Compare Halfword Long                          [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_halfword_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
S64     n;                              /* 64-bit operand value      */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load rightmost 2 bytes of comparand from operand address */
    n = (S16)ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            (S64)regs->GR_G(r1) < n ? 1 :
            (S64)regs->GR_G(r1) > n ? 2 : 0;

} /* end DEF_INST(compare_halfword_long) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* C6x5 CHRL  - Compare Halfword Relative Long                 [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_halfword_relative_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U16     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch2) ( addr2, USE_INST_SPACE, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            (S32)regs->GR_L(r1) < (S16)n ? 1 :
            (S32)regs->GR_L(r1) > (S16)n ? 2 : 0;

} /* end DEF_INST(compare_halfword_relative_long) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C6x4 CGHRL - Compare Halfword Relative Long Long            [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_halfword_relative_long_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U16     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch2) ( addr2, USE_INST_SPACE, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            (S64)regs->GR_G(r1) < (S16)n ? 1 :
            (S64)regs->GR_G(r1) > (S16)n ? 2 : 0;

} /* end DEF_INST(compare_halfword_relative_long_long) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* ECFE CIB   - Compare Immediate and Branch                   [RIS] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_immediate_and_branch)
{
int     r1;                             /* Register number           */
int     m3;                             /* Mask bits                 */
int     b4;                             /* Base of effective addr    */
VADR    effective_addr4;                /* Effective address         */
int     cc;                             /* Comparison result         */
BYTE    i2;                             /* Immediate value           */

    RIS_B(inst, regs, r1, i2, m3, b4, effective_addr4);

    /* Compare signed operands and set comparison result */
    cc = (S32)regs->GR_L(r1) < (S32)(S8)i2 ? 1 :
         (S32)regs->GR_L(r1) > (S32)(S8)i2 ? 2 : 0;

    /* Branch to operand address if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_BRANCH(regs, effective_addr4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_immediate_and_branch) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* ECFC CGIB  - Compare Immediate and Branch Long              [RIS] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_immediate_and_branch_long)
{
int     r1;                             /* Register number           */
int     m3;                             /* Mask bits                 */
int     b4;                             /* Base of effective addr    */
VADR    effective_addr4;                /* Effective address         */
int     cc;                             /* Comparison result         */
BYTE    i2;                             /* Immediate value           */

    RIS_B(inst, regs, r1, i2, m3, b4, effective_addr4);

    /* Compare signed operands and set comparison result */
    cc = (S64)regs->GR_G(r1) < (S64)(S8)i2 ? 1 :
         (S64)regs->GR_G(r1) > (S64)(S8)i2 ? 2 : 0;

    /* Branch to operand address if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_BRANCH(regs, effective_addr4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_immediate_and_branch_long) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* EC7E CIJ   - Compare Immediate and Branch Relative          [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_immediate_and_branch_relative)
{
int     r1;                             /* Register numbers          */
int     m3;                             /* Mask bits                 */
BYTE    i2;                             /* Immediate operand value   */
S16     i4;                             /* 16-bit immediate offset   */
int     cc;                             /* Comparison result         */

    RIE_RMII_B(inst, regs, r1, i2, m3, i4);

    /* Compare signed operands and set comparison result */
    cc = (S32)regs->GR_L(r1) < (S32)(S8)i2 ? 1 :
         (S32)regs->GR_L(r1) > (S32)(S8)i2 ? 2 : 0;

    /* Branch to immediate offset if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_RELATIVE_BRANCH(regs, 2*i4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_immediate_and_branch_relative) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EC7C CGIJ  - Compare Immediate and Branch Relative Long     [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_immediate_and_branch_relative_long)
{
int     r1;                             /* Register numbers          */
int     m3;                             /* Mask bits                 */
BYTE    i2;                             /* Immediate operand value   */
S16     i4;                             /* 16-bit immediate offset   */
int     cc;                             /* Comparison result         */

    RIE_RMII_B(inst, regs, r1, i2, m3, i4);

    /* Compare signed operands and set comparison result */
    cc = (S64)regs->GR_G(r1) < (S64)(S8)i2 ? 1 :
         (S64)regs->GR_G(r1) > (S64)(S8)i2 ? 2 : 0;

    /* Branch to immediate offset if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_RELATIVE_BRANCH(regs, 2*i4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_immediate_and_branch_relative_long) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* EC72 CIT   - Compare Immediate and Trap                     [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_immediate_and_trap)
{
int     r1;                             /* Register number           */
int     m3;                             /* Mask bits                 */
int     cc;                             /* Comparison result         */
U16     i2;                             /* 16-bit immediate value    */

    RIE_RIM(inst, regs, r1, i2, m3);

    /* Compare signed operands and set comparison result */
    cc = (S32)regs->GR_L(r1) < (S32)(S16)i2 ? 1 :
         (S32)regs->GR_L(r1) > (S32)(S16)i2 ? 2 : 0;

    /* Raise data exception if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_immediate_and_trap) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EC70 CGIT  - Compare Immediate and Trap Long                [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_immediate_and_trap_long)
{
int     r1;                             /* Register number           */
int     m3;                             /* Mask bits                 */
int     cc;                             /* Comparison result         */
U16     i2;                             /* 16-bit immediate value    */

    RIE_RIM(inst, regs, r1, i2, m3);

    /* Compare signed operands and set comparison result */
    cc = (S64)regs->GR_G(r1) < (S64)(S16)i2 ? 1 :
         (S64)regs->GR_G(r1) > (S64)(S16)i2 ? 2 : 0;

    /* Raise data exception if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_immediate_and_trap_long) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* ECF7 CLRB  - Compare Logical and Branch Register            [RRS] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_and_branch_register)
{
int     r1, r2;                         /* Register numbers          */
int     m3;                             /* Mask bits                 */
int     b4;                             /* Base of effective addr    */
VADR    effective_addr4;                /* Effective address         */
int     cc;                             /* Comparison result         */

    RRS_B(inst, regs, r1, r2, m3, b4, effective_addr4);

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_L(r1) < regs->GR_L(r2) ? 1 :
         regs->GR_L(r1) > regs->GR_L(r2) ? 2 : 0;

    /* Branch to operand address if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_BRANCH(regs, effective_addr4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_logical_and_branch_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* ECE5 CLGRB - Compare Logical and Branch Long Register       [RRS] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_and_branch_long_register)
{
int     r1, r2;                         /* Register numbers          */
int     m3;                             /* Mask bits                 */
int     b4;                             /* Base of effective addr    */
VADR    effective_addr4;                /* Effective address         */
int     cc;                             /* Comparison result         */

    RRS_B(inst, regs, r1, r2, m3, b4, effective_addr4);

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_G(r1) < regs->GR_G(r2) ? 1 :
         regs->GR_G(r1) > regs->GR_G(r2) ? 2 : 0;

    /* Branch to operand address if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_BRANCH(regs, effective_addr4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_logical_and_branch_long_register) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* EC77 CLRJ  - Compare Logical and Branch Relative Register   [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_and_branch_relative_register)
{
int     r1, r2;                         /* Register numbers          */
int     m3;                             /* Mask bits                 */
S16     i4;                             /* 16-bit immediate offset   */
int     cc;                             /* Comparison result         */

    RIE_RRIM_B(inst, regs, r1, r2, i4, m3);

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_L(r1) < regs->GR_L(r2) ? 1 :
         regs->GR_L(r1) > regs->GR_L(r2) ? 2 : 0;

    /* Branch to immediate offset if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_RELATIVE_BRANCH(regs, 2*i4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_logical_and_branch_relative_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EC65 CLGRJ - Compare Logical and Branch Relative Long Reg   [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_and_branch_relative_long_register)
{
int     r1, r2;                         /* Register numbers          */
int     m3;                             /* Mask bits                 */
S16     i4;                             /* 16-bit immediate offset   */
int     cc;                             /* Comparison result         */

    RIE_RRIM_B(inst, regs, r1, r2, i4, m3);

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_G(r1) < regs->GR_G(r2) ? 1 :
         regs->GR_G(r1) > regs->GR_G(r2) ? 2 : 0;

    /* Branch to immediate offset if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_RELATIVE_BRANCH(regs, 2*i4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_logical_and_branch_relative_long_register) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* B973 CLRT  - Compare Logical and Trap Register              [RRF] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_and_trap_register)
{
int     r1, r2;                         /* Register numbers          */
int     m3;                             /* Mask bits                 */
int     cc;                             /* Comparison result         */

    RRF_M(inst, regs, r1, r2, m3);

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_L(r1) < regs->GR_L(r2) ? 1 :
         regs->GR_L(r1) > regs->GR_L(r2) ? 2 : 0;

    /* Raise data exception if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_logical_and_trap_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B961 CLGRT - Compare Logical and Trap Long Register         [RRF] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_and_trap_long_register)
{
int     r1, r2;                         /* Register numbers          */
int     m3;                             /* Mask bits                 */
int     cc;                             /* Comparison result         */

    RRF_M(inst, regs, r1, r2, m3);

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_G(r1) < regs->GR_G(r2) ? 1 :
         regs->GR_G(r1) > regs->GR_G(r2) ? 2 : 0;

    /* Raise data exception if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_logical_and_trap_long_register) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* ECFF CLIB  - Compare Logical Immediate and Branch           [RIS] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_immediate_and_branch)
{
int     r1;                             /* Register number           */
int     m3;                             /* Mask bits                 */
int     b4;                             /* Base of effective addr    */
VADR    effective_addr4;                /* Effective address         */
int     cc;                             /* Comparison result         */
BYTE    i2;                             /* Immediate value           */

    RIS_B(inst, regs, r1, i2, m3, b4, effective_addr4);

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_L(r1) < i2 ? 1 :
         regs->GR_L(r1) > i2 ? 2 : 0;

    /* Branch to operand address if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_BRANCH(regs, effective_addr4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_logical_immediate_and_branch) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* ECFD CLGIB - Compare Logical Immediate and Branch Long      [RIS] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_immediate_and_branch_long)
{
int     r1;                             /* Register number           */
int     m3;                             /* Mask bits                 */
int     b4;                             /* Base of effective addr    */
VADR    effective_addr4;                /* Effective address         */
int     cc;                             /* Comparison result         */
BYTE    i2;                             /* Immediate value           */

    RIS_B(inst, regs, r1, i2, m3, b4, effective_addr4);

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_G(r1) < i2 ? 1 :
         regs->GR_G(r1) > i2 ? 2 : 0;

    /* Branch to operand address if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_BRANCH(regs, effective_addr4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_logical_immediate_and_branch_long) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* EC7F CLIJ  - Compare Logical Immediate and Branch Relative  [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_immediate_and_branch_relative)
{
int     r1;                             /* Register number           */
int     m3;                             /* Mask bits                 */
BYTE    i2;                             /* Immediate operand value   */
S16     i4;                             /* 16-bit immediate offset   */
int     cc;                             /* Comparison result         */

    RIE_RMII_B(inst, regs, r1, i2, m3, i4);

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_L(r1) < i2 ? 1 :
         regs->GR_L(r1) > i2 ? 2 : 0;

    /* Branch to immediate offset if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_RELATIVE_BRANCH(regs, 2*i4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_logical_immediate_and_branch_relative) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EC7D CLGIJ - Compare Logical Immed and Branch Relative Long [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_immediate_and_branch_relative_long)
{
int     r1;                             /* Register number           */
int     m3;                             /* Mask bits                 */
BYTE    i2;                             /* Immediate operand value   */
S16     i4;                             /* 16-bit immediate offset   */
int     cc;                             /* Comparison result         */

    RIE_RMII_B(inst, regs, r1, i2, m3, i4);

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_G(r1) < i2 ? 1 :
         regs->GR_G(r1) > i2 ? 2 : 0;

    /* Branch to immediate offset if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
        SUCCESSFUL_RELATIVE_BRANCH(regs, 2*i4, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(compare_logical_immediate_and_branch_relative_long) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* EC73 CLFIT - Compare Logical Immediate and Trap Fullword    [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_immediate_and_trap_fullword)
{
int     r1;                             /* Register number           */
int     m3;                             /* Mask bits                 */
int     cc;                             /* Comparison result         */
U16     i2;                             /* 16-bit immediate value    */

    RIE_RIM(inst, regs, r1, i2, m3);

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_L(r1) < i2 ? 1 :
         regs->GR_L(r1) > i2 ? 2 : 0;

    /* Raise data exception if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_logical_immediate_and_trap_fullword) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EC71 CLGIT - Compare Logical Immediate and Trap Long        [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_immediate_and_trap_long)
{
int     r1;                             /* Register number           */
int     m3;                             /* Mask bits                 */
int     cc;                             /* Comparison result         */
U16     i2;                             /* 16-bit immediate value    */

    RIE_RIM(inst, regs, r1, i2, m3);

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_G(r1) < i2 ? 1 :
         regs->GR_G(r1) > i2 ? 2 : 0;

    /* Raise data exception if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_logical_immediate_and_trap_long) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* E55D CLFHSI - Compare Logical Immediate Fullword Storage    [SIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_immediate_fullword_storage)
{
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
U16     i2;                             /* 16-bit immediate value    */
U32     n;                              /* 32-bit storage value      */

    SIL(inst, regs, i2, b1, effective_addr1);

    /* Load 32-bit value from first operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr1, b1, regs );

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = n < i2 ? 1 : n > i2 ? 2 : 0;

} /* end DEF_INST(compare_logical_immediate_fullword_storage) */


/*-------------------------------------------------------------------*/
/* E555 CLHHSI - Compare Logical Immediate Halfword Storage    [SIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_immediate_halfword_storage)
{
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
U16     i2;                             /* 16-bit immediate value    */
U16     n;                              /* 16-bit storage value      */

    SIL(inst, regs, i2, b1, effective_addr1);

    /* Load 16-bit value from first operand address */
    n = ARCH_DEP(vfetch2) ( effective_addr1, b1, regs );

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = n < i2 ? 1 : n > i2 ? 2 : 0;

} /* end DEF_INST(compare_logical_immediate_halfword_storage) */


/*-------------------------------------------------------------------*/
/* E559 CLGHSI - Compare Logical Immediate Long Storage        [SIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_immediate_long_storage)
{
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
U16     i2;                             /* 16-bit immediate value    */
U64     n;                              /* 64-bit storage value      */

    SIL(inst, regs, i2, b1, effective_addr1);

    /* Load 64-bit value from first operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr1, b1, regs );

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = n < i2 ? 1 : n > i2 ? 2 : 0;

} /* end DEF_INST(compare_logical_immediate_long_storage) */


/*-------------------------------------------------------------------*/
/* C6xF CLRL  - Compare Logical Relative Long                  [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_relative_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U32     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Program check if operand not on fullword boundary */
    FW_CHECK(addr2, regs);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch4) ( addr2, USE_INST_SPACE, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            regs->GR_L(r1) < n ? 1 :
            regs->GR_L(r1) > n ? 2 : 0;

} /* end DEF_INST(compare_logical_relative_long) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C6xA CLGRL - Compare Logical Relative Long Long             [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_relative_long_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U64     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Program check if operand not on doubleword boundary */
    DW_CHECK(addr2, regs);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch8) ( addr2, USE_INST_SPACE, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            regs->GR_G(r1) < n ? 1 :
            regs->GR_G(r1) > n ? 2 : 0;

} /* end DEF_INST(compare_logical_relative_long_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C6xE CLGFRL - Compare Logical Relative Long Long Fullword   [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_relative_long_long_fullword)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U32     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Program check if operand not on fullword boundary */
    FW_CHECK(addr2, regs);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch4) ( addr2, USE_INST_SPACE, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            regs->GR_G(r1) < n ? 1 :
            regs->GR_G(r1) > n ? 2 : 0;

} /* end DEF_INST(compare_logical_relative_long_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* C6x7 CLHRL - Compare Logical Halfword Relative Long         [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_relative_long_halfword)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U16     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch2) ( addr2, USE_INST_SPACE, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            regs->GR_L(r1) < n ? 1 :
            regs->GR_L(r1) > n ? 2 : 0;

} /* end DEF_INST(compare_logical_relative_long_halfword) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C6x6 CLGHRL - Compare Logical Halfword Relative Long Long   [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_relative_long_long_halfword)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U16     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch2) ( addr2, USE_INST_SPACE, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            regs->GR_G(r1) < n ? 1 :
            regs->GR_G(r1) > n ? 2 : 0;

} /* end DEF_INST(compare_logical_relative_long_long_halfword) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* C6xD CRL   - Compare Relative Long                          [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_relative_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U32     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Program check if operand not on fullword boundary */
    FW_CHECK(addr2, regs);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch4) ( addr2, USE_INST_SPACE, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            (S32)regs->GR_L(r1) < (S32)n ? 1 :
            (S32)regs->GR_L(r1) > (S32)n ? 2 : 0;

} /* end DEF_INST(compare_relative_long) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C6x8 CGRL  - Compare Relative Long Long                     [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_relative_long_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U64     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Program check if operand not on doubleword boundary */
    DW_CHECK(addr2, regs);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch8) ( addr2, USE_INST_SPACE, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            (S64)regs->GR_G(r1) < (S64)n ? 1 :
            (S64)regs->GR_G(r1) > (S64)n ? 2 : 0;

} /* end DEF_INST(compare_relative_long_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C6xC CGFRL - Compare Relative Long Long Fullword            [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_relative_long_long_fullword)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U32     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Program check if operand not on fullword boundary */
    FW_CHECK(addr2, regs);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch4) ( addr2, USE_INST_SPACE, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            (S64)regs->GR_G(r1) < (S32)n ? 1 :
            (S64)regs->GR_G(r1) > (S32)n ? 2 : 0;

} /* end DEF_INST(compare_relative_long_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB4C ECAG  - Extract Cache Attribute                        [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(extract_cache_attribute)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     ai, li, ti;                     /* Operand address subfields */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    /* Address bit 63 contains the Type Indication (TI) */
    ti = effective_addr2 & 0x1;

    /* Address bits 60-62 contain the Level Indication (LI) */
    li = (effective_addr2 >> 1) & 0x7;

    /* Address bits 56-59 contain the Attribute Indication (AI) */
    ai = (effective_addr2 >> 4) & 0xF;

    //logmsg ("ECAG ai=%d li=%d ti=%d\n", ai, li, ti);

    /* If reserved bits 40-55 are not zero then set r1 to all ones */
    if ((effective_addr2 & 0xFFFF00) != 0)
    {
        regs->GR(r1) = 0xFFFFFFFFFFFFFFFFULL;
        return;
    }

    /* If AI=0 (topology summary) is requested, set register r1 to
       indicate that cache level 0 is private to this CPU and that
       cache levels 1-7 are not implemented */
    if (ai == 0)
    {
        regs->GR_H(r1) = 0x04000000;
        regs->GR_L(r1) = 0x00000000;
        return;
    }

    /* If cache level is not 0, set register r1 to all ones which
       indicates that the requested cache level is not implemented */
    if (li > 0)
    {
        regs->GR(r1) = 0xFFFFFFFFFFFFFFFFULL;
        return;
    }

    /* If AI=1 (cache line size) is requested for cache level 0
       set register r1 to indicate a fictitious cache line size */
    if (ai == 1 && li == 0)
    {
        regs->GR(r1) = 256;
        return;
    }

    /* If AI=2 (total cache size) is requested for cache level 0
       set register r1 to indicate a fictitious total cache size */
    if (ai == 2 && li == 0)
    {
        regs->GR(r1) = 256 * 2048;
        return;
    }

    /* Set register r1 to all ones indicating that the requested
       attribute indication is reserved */
    regs->GR(r1) = 0xFFFFFFFFFFFFFFFFULL;

} /* end DEF_INST(extract_cache_attribute) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ACCESS_REGISTERS)
/*-------------------------------------------------------------------*/
/* E375 LAEY  - Load Address Extended (Long Displacement)      [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_address_extended_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY0(inst, regs, r1, b2, effective_addr2);

    /* Load operand address into register */
    SET_GR_A(r1, regs,effective_addr2);

    /* Load corresponding value into access register */
    if ( PRIMARY_SPACE_MODE(&(regs->psw)) )
        regs->AR(r1) = ALET_PRIMARY;
    else if ( SECONDARY_SPACE_MODE(&(regs->psw)) )
        regs->AR(r1) = ALET_SECONDARY;
    else if ( HOME_SPACE_MODE(&(regs->psw)) )
        regs->AR(r1) = ALET_HOME;
    else /* ACCESS_REGISTER_MODE(&(regs->psw)) */
        regs->AR(r1) = (b2 == 0) ? 0 : regs->AR(b2);
    SET_AEA_AR(regs, r1);

} /* end DEF_INST(load_address_extended_y) */
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E332 LTGF  - Load and Test Long Fullword                    [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_test_long_fullword)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* Second operand value      */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from sign-extended second operand */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );
    regs->GR_G(r1) = (S64)(S32)n;

    /* Set condition code according to value loaded */
    regs->psw.cc = (S64)regs->GR_G(r1) < 0 ? 1 :
                   (S64)regs->GR_G(r1) > 0 ? 2 : 0;

} /* end DEF_INST(load_and_test_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* C4x5 LHRL  - Load Halfword Relative Long                    [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(load_halfword_relative_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U16     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch2) ( addr2, USE_INST_SPACE, regs );

    /* Sign-extend operand value and load into R1 register */
    regs->GR_L(r1) = (S32)(S16)n;

} /* end DEF_INST(load_halfword_relative_long) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C4x4 LGHRL - Load Halfword Relative Long Long               [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(load_halfword_relative_long_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U16     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch2) ( addr2, USE_INST_SPACE, regs );

    /* Sign-extend operand value and load into R1 register */
    regs->GR_G(r1) = (S64)(S16)n;

} /* end DEF_INST(load_halfword_relative_long_long) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* C4x2 LLHRL - Load Logical Halfword Relative Long            [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_halfword_relative_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U16     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch2) ( addr2, USE_INST_SPACE, regs );

    /* Zero-extend operand value and load into R1 register */
    regs->GR_L(r1) = n;

} /* end DEF_INST(load_logical_halfword_relative_long) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C4x6 LLGHRL - Load Logical Halfword Relative Long Long      [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_halfword_relative_long_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U16     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch2) ( addr2, USE_INST_SPACE, regs );

    /* Zero-extend operand value and load into R1 register */
    regs->GR_G(r1) = n;

} /* end DEF_INST(load_logical_halfword_relative_long_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C4xE LLGFRL - Load Logical Relative Long Long Fullword      [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_relative_long_long_fullword)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U32     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Program check if operand not on fullword boundary */
    FW_CHECK(addr2, regs);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch4) ( addr2, USE_INST_SPACE, regs );

    /* Zero-extend operand value and load into R1 register */
    regs->GR_G(r1) = n;

} /* end DEF_INST(load_logical_relative_long_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* C4xD LRL   - Load Relative Long                             [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(load_relative_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U32     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Program check if operand not on fullword boundary */
    FW_CHECK(addr2, regs);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch4) ( addr2, USE_INST_SPACE, regs );

    /* Load operand value into R1 register */
    regs->GR_L(r1) = n;

} /* end DEF_INST(load_relative_long) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C4x8 LGRL  - Load Relative Long Long                        [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(load_relative_long_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U64     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Program check if operand not on doubleword boundary */
    DW_CHECK(addr2, regs);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch8) ( addr2, USE_INST_SPACE, regs );

    /* Load operand value into R1 register */
    regs->GR_G(r1) = n;

} /* end DEF_INST(load_relative_long_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C4xC LGFRL - Load Relative Long Long Fullword               [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(load_relative_long_long_fullword)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */
U32     n;                              /* Relative operand value    */

    RIL_A(inst, regs, r1, addr2);

    /* Program check if operand not on fullword boundary */
    FW_CHECK(addr2, regs);

    /* Load relative operand from instruction address space */
    n = ARCH_DEP(vfetch4) ( addr2, USE_INST_SPACE, regs );

    /* Sign-extend operand value and load into R1 register */
    regs->GR_G(r1) = (S64)(S32)n;

} /* end DEF_INST(load_relative_long_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* E54C MVHI  - Move Fullword from Halfword Immediate          [SIL] */
/*-------------------------------------------------------------------*/
DEF_INST(move_fullword_from_halfword_immediate)
{
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
S16     i2;                             /* 16-bit immediate value    */
S32     n;                              /* Sign-extended value of i2 */

    SIL(inst, regs, i2, b1, effective_addr1);

    /* Sign-extend 16-bit immediate value to 32 bits */
    n = i2;

    /* Store 4-byte value at operand address */
    ARCH_DEP(vstore4) ( n, effective_addr1, b1, regs );

} /* end DEF_INST(move_fullword_from_halfword_immediate) */


/*-------------------------------------------------------------------*/
/* E544 MVHHI - Move Halfword from Halfword Immediate          [SIL] */
/*-------------------------------------------------------------------*/
DEF_INST(move_halfword_from_halfword_immediate)
{
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
S16     i2;                             /* 16-bit immediate value    */

    SIL(inst, regs, i2, b1, effective_addr1);

    /* Store 16-bit immediate value at operand address */
    ARCH_DEP(vstore2) ( i2, effective_addr1, b1, regs );

} /* end DEF_INST(move_halfword_from_halfword_immediate) */


/*-------------------------------------------------------------------*/
/* E548 MVGHI - Move Long from Halfword Immediate              [SIL] */
/*-------------------------------------------------------------------*/
DEF_INST(move_long_from_halfword_immediate)
{
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
S16     i2;                             /* 16-bit immediate value    */
S64     n;                              /* Sign-extended value of i2 */

    SIL(inst, regs, i2, b1, effective_addr1);

    /* Sign-extend 16-bit immediate value to 64 bits */
    n = i2;

    /* Store 8-byte value at operand address */
    ARCH_DEP(vstore8) ( n, effective_addr1, b1, regs );

} /* end DEF_INST(move_long_from_halfword_immediate) */


/*-------------------------------------------------------------------*/
/* E37C MHY   - Multiply Halfword (Long Displacement)          [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_halfword_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
S32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load 2 bytes from operand address */
    n = (S16)ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

    /* Multiply R1 register by n, ignore leftmost 32 bits of
       result, and place rightmost 32 bits in R1 register */
    mul_signed ((U32 *)&n, &(regs->GR_L(r1)), regs->GR_L(r1), n);

} /* end DEF_INST(multiply_halfword_y) */


/*-------------------------------------------------------------------*/
/* C2x1 MSFI  - Multiply Single Immediate Fullword             [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_single_immediate_fullword)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL(inst, regs, r1, opcd, i2);

    /* Multiply signed operands ignoring overflow */
    regs->GR_L(r1) = (S32)regs->GR_L(r1) * (S32)i2;

} /* end DEF_INST(multiply_single_immediate_fullword) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C2x0 MSGFI - Multiply Single Immediate Long Fullword        [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_single_immediate_long_fullword)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL(inst, regs, r1, opcd, i2);

    /* Multiply signed operands ignoring overflow */
    regs->GR_G(r1) = (S64)regs->GR_G(r1) * (S32)i2;

} /* end DEF_INST(multiply_single_immediate_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* E35C MFY   - Multiply (Long Displacement)                   [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    ODD_CHECK(r1, regs);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Multiply r1+1 by n and place result in r1 and r1+1 */
    mul_signed (&(regs->GR_L(r1)), &(regs->GR_L(r1+1)),
                    regs->GR_L(r1+1),
                    n);

} /* end DEF_INST(multiply_y) */


/*-------------------------------------------------------------------*/
/* E336 PFD   - Prefetch Data                                  [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(prefetch_data)
{
int     m1;                             /* Mask value                */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, m1, b2, effective_addr2);

    /* The Prefetch Data instruction acts as a no-op */

} /* end DEF_INST(prefetch_data) */


/*-------------------------------------------------------------------*/
/* C6x2 PFDRL - Prefetch Data Relative Long                    [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(prefetch_data_relative_long)
{
int     m1;                             /* Mask value                */
VADR    addr2;                          /* Relative operand address  */

    RIL_A(inst, regs, m1, addr2);

    /* The Prefetch Data instruction acts as a no-op */

} /* end DEF_INST(prefetch_data_relative_long) */

#endif /*defined(FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY)*/

#if defined(FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY) \
 || defined(FEATURE_MISC_INSTRUCTION_EXTENSIONS_FACILITY)       /*912*/ \
 || defined(FEATURE_HIGH_WORD_FACILITY)                         /*810*/
/*-------------------------------------------------------------------*/
/* Rotate Then Perform Operation On Selected Bits Long Register      */
/* Subroutine is called by RNSBG,RISBG,ROSBG,RXSBG instructions      */
/* and also by the RISBHG,RISBLG instructions */                /*810*/
/* and by the RISBGN instruction */                             /*912*/
/*-------------------------------------------------------------------*/
DEF_INST(rotate_then_xxx_selected_bits_long_reg)
{
int     r1, r2;                         /* Register numbers          */
int     start, end;                     /* Start and end bit number  */
U64     mask, rota, resu;               /* 64-bit work areas         */
int     n;                              /* Number of bits to shift   */
int     t_bit = 0;                      /* Test-results indicator    */
int     z_bit = 0;                      /* Zero-remaining indicator  */
int     i;                              /* Loop counter              */
BYTE    i3, i4, i5;                     /* Immediate values          */
BYTE    opcode;                         /* 2nd byte of opcode        */

    RIE_RRIII(inst, regs, r1, r2, i3, i4, i5);

    /* Extract second byte of instruction opcode */
    opcode = inst[5];

    /* Extract parameters from immediate fields */
    start = i3 & 0x3F;
    end = i4 & 0x3F;
    n = i5 & 0x3F;
    if ((opcode & 0xFC) == 0x50 /*Low*/ ) {                     /*810*/
        start |= 0x20;                                          /*810*/
        end |= 0x20;                                            /*810*/
    }                                                           /*810*/
    if ((opcode & 0xFC) == 0x5C /*High*/ ) {                    /*810*/
        start &= 0x1F;                                          /*810*/
        end &= 0x1F;                                            /*810*/
    }                                                           /*810*/
    if ((opcode & 0x03) == 0x01 /*Insert*/ )                    /*810*/
        z_bit = i4 >> 7;
    else
        t_bit = i3 >> 7;

    /* Copy value from R2 register and rotate left n bits */
    rota = (regs->GR_G(r2) << n)
            | ((n == 0) ? 0 : (regs->GR_G(r2) >> (64 - n)));

    /* Construct mask for selected bits */
    for (i=0, mask=0; i < 64; i++)
    {
        mask <<= 1;
        if (start <= end) {
            if (i >= start && i <= end) mask |= 1;
        } else {
            if (i <= end || i >= start) mask |= 1;
        }
    } /* end for(i) */

    /* Isolate selected bits of rotated second operand */
    rota &= mask;

    /* Isolate selected bits of first operand */
    resu = regs->GR_G(r1) & mask;

    /* Perform operation on selected bits */
    switch (opcode) {
    case 0x54: /* And */
        resu &= rota;
        break;
    case 0x51: /* Insert Low */                                 /*810*/
    case 0x55: /* Insert */
    case 0x5D: /* Insert High */                                /*810*/
        resu = rota;
        break;
    case 0x56: /* Or */
        resu |= rota;
        break;
    case 0x57: /* Exclusive Or */
        resu ^= rota;
        break;
    } /* end switch(opcode) */

    /* And/Or/Xor set condition code according to result bits*/ /*810*/
    if ((opcode & 0x03) != 0x01 /*Insert*/ )                    /*810*/
        regs->psw.cc = (resu == 0) ? 0 : 1;

    /* Insert result bits into R1 register */
    if (t_bit == 0)
    {
        if (z_bit == 0)
            regs->GR_G(r1) = (regs->GR_G(r1) & ~mask) | resu;
        else if ((opcode & 0xFC) == 0x50 /*Low*/ )              /*810*/
            regs->GR_L(r1) = (U32)resu;                         /*810*/
        else if ((opcode & 0xFC) == 0x5C /*High*/ )             /*810*/
            regs->GR_H(r1) = (U32)(resu >> 32);                 /*810*/
        else
            regs->GR_G(r1) = resu;
    } /* end if(t_bit==0) */

    /* For RISBG set condition code according to signed result */
    if (opcode == 0x55)
        regs->psw.cc =
                (S64)regs->GR_G(r1) < 0 ? 1 :
                (S64)regs->GR_G(r1) > 0 ? 2 : 0;

    /* For RISBHG,RISBLG the condition code remains unchanged*/ /*810*/
    /* For RISBGN the condition code remains unchanged */       /*912*/

} /* end DEF_INST(rotate_then_xxx_selected_bits_long_reg) */
#endif /*defined(FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY)*/
       /*|| defined(FEATURE_MISC_INSTRUCTION_EXTENSIONS_FACILITY)*/ /*912*/
       /*|| defined(FEATURE_HIGH_WORD_FACILITY)*/               /*810*/

#if defined(FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY)

#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EC54 RNSBG - Rotate Then And Selected Bits                  [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(rotate_then_and_selected_bits_long_reg)
{
    ARCH_DEP(rotate_then_xxx_selected_bits_long_reg) (inst, regs);
} /* end DEF_INST(rotate_then_and_selected_bits_long_reg) */


/*-------------------------------------------------------------------*/
/* EC55 RISBG - Rotate Then Insert Selected Bits               [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(rotate_then_insert_selected_bits_long_reg)
{
    ARCH_DEP(rotate_then_xxx_selected_bits_long_reg) (inst, regs);
} /* end DEF_INST(rotate_then_insert_selected_bits_long_reg) */


/*-------------------------------------------------------------------*/
/* EC56 ROSBG - Rotate Then Or Selected Bits                   [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(rotate_then_or_selected_bits_long_reg)
{
    ARCH_DEP(rotate_then_xxx_selected_bits_long_reg) (inst, regs);
} /* end DEF_INST(rotate_then_or_selected_bits_long_reg) */


/*-------------------------------------------------------------------*/
/* EC57 RXSBG - Rotate Then Exclusive Or Selected Bits         [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(rotate_then_exclusive_or_selected_bits_long_reg)
{
    ARCH_DEP(rotate_then_xxx_selected_bits_long_reg) (inst, regs);
} /* end DEF_INST(rotate_then_exclusive_or_selected_bits_long_reg) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* C4x7 STHRL - Store Halfword Relative Long                   [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(store_halfword_relative_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */

    RIL_A(inst, regs, r1, addr2);

    /* Store low 2 bytes of R1 register in instruction address space */
    ARCH_DEP(vstore2) ( regs->GR_LHL(r1), addr2, USE_INST_SPACE, regs );

} /* end DEF_INST(store_halfword_relative_long) */


/*-------------------------------------------------------------------*/
/* C4xF STRL  - Store Relative Long                            [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(store_relative_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */

    RIL_A(inst, regs, r1, addr2);

    /* Program check if operand not on fullword boundary */
    FW_CHECK(addr2, regs);

    /* Store low 4 bytes of R1 register in instruction address space */
    ARCH_DEP(vstore4) ( regs->GR_L(r1), addr2, USE_INST_SPACE, regs );

} /* end DEF_INST(store_relative_long) */


/*-------------------------------------------------------------------*/
/* C4xB STGRL - Store Relative Long Long                       [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(store_relative_long_long)
{
int     r1;                             /* Register number           */
VADR    addr2;                          /* Relative operand address  */

    RIL_A(inst, regs, r1, addr2);

    /* Program check if operand not on doubleword boundary */
    DW_CHECK(addr2, regs);

    /* Store R1 register in instruction address space */
    ARCH_DEP(vstore8) ( regs->GR_G(r1), addr2, USE_INST_SPACE, regs );

} /* end DEF_INST(store_relative_long_long) */

#endif /*defined(FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY)*/


#if defined(FEATURE_HIGH_WORD_FACILITY)                         /*810*/

/*-------------------------------------------------------------------*/
/* B9C8 AHHHR - Add High High High Register                    [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_high_high_high_register)                           /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR(inst, regs, r1, r2, r3);

    /* Add signed operands and set condition code */
    regs->psw.cc =
            add_signed (&(regs->GR_H(r1)),
                    regs->GR_H(r2),
                    regs->GR_H(r3));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_high_high_high_register) */


/*-------------------------------------------------------------------*/
/* B9D8 AHHLR - Add High High Low Register                     [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_high_high_low_register)                            /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR(inst, regs, r1, r2, r3);

    /* Add signed operands and set condition code */
    regs->psw.cc =
            add_signed (&(regs->GR_H(r1)),
                    regs->GR_H(r2),
                    regs->GR_L(r3));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_high_high_low_register) */

/*-------------------------------------------------------------------*/
/* CCx8 AIH   - Add High Immediate                             [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(add_high_immediate)                                    /*810*/
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL(inst, regs, r1, opcd, i2);

    /* Add signed operands and set condition code */
    regs->psw.cc = add_signed (&(regs->GR_H(r1)),
                                regs->GR_H(r1),
                                (S32)i2);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_high_immediate) */


/*-------------------------------------------------------------------*/
/* B9CA ALHHHR - Add Logical High High High Register           [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_high_high_high_register)                   /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* Add signed operands and set condition code */
    regs->psw.cc = add_logical (&(regs->GR_H(r1)),
                                  regs->GR_H(r2),
                                  regs->GR_H(r3));

} /* end DEF_INST(add_logical_high_high_high_register) */


/*-------------------------------------------------------------------*/
/* B9DA ALHHLR - Add Logical High High Low Register            [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_high_high_low_register)                    /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* Add signed operands and set condition code */
    regs->psw.cc = add_logical (&(regs->GR_H(r1)),
                                  regs->GR_H(r2),
                                  regs->GR_L(r3));

} /* end DEF_INST(add_logical_high_high_low_register) */


/*-------------------------------------------------------------------*/
/* CCxA ALSIH - Add Logical with Signed Immediate High         [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_with_signed_immediate_high)                /*810*/
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Add operands and set condition code */
    regs->psw.cc = (S32)i2 < 0 ?
        sub_logical (&(regs->GR_H(r1)), regs->GR_H(r1), -(S32)i2) :
        add_logical (&(regs->GR_H(r1)), regs->GR_H(r1), i2);

} /* end DEF_INST(add_logical_with_signed_immediate_high) */


/*-------------------------------------------------------------------*/
/* CCxB ALSIHN - Add Logical with Signed Immediate High No CC  [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_with_signed_immediate_high_n)              /*810*/
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Add operands without setting condition code */
    if ((S32)i2 < 0) {
        sub_logical (&(regs->GR_H(r1)), regs->GR_H(r1), -(S32)i2);
    } else {
        add_logical (&(regs->GR_H(r1)), regs->GR_H(r1), i2);
    }

} /* end DEF_INST(add_logical_with_signed_immediate_high_n) */


/*-------------------------------------------------------------------*/
/* CCx6 BRCTH - Branch Relative on Count High                  [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_relative_on_count_high)                         /*810*/
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
S32     i2;                             /* 32-bit operand value      */

    RIL_B(inst, regs, r1, opcd, i2);

    /* Subtract 1 from the R1 operand and branch if non-zero */
    if ( --(regs->GR_H(r1)) )
        SUCCESSFUL_RELATIVE_BRANCH_LONG(regs, 2LL*i2);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(branch_relative_on_count_high) */


/*-------------------------------------------------------------------*/
/* B9CD CHHR  - Compare High High Register                     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_high_high_register)                            /*810*/
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Compare signed operands and set condition code */
    regs->psw.cc =
                (S32)regs->GR_H(r1) < (S32)regs->GR_H(r2) ? 1 :
                (S32)regs->GR_H(r1) > (S32)regs->GR_H(r2) ? 2 : 0;

} /* DEF_INST(compare_high_high_register) */


/*-------------------------------------------------------------------*/
/* B9DD CHLR  - Compare High Low Register                      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_high_low_register)                             /*810*/
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Compare signed operands and set condition code */
    regs->psw.cc =
                (S32)regs->GR_H(r1) < (S32)regs->GR_L(r2) ? 1 :
                (S32)regs->GR_H(r1) > (S32)regs->GR_L(r2) ? 2 : 0;

} /* DEF_INST(compare_high_low_register) */


/*-------------------------------------------------------------------*/
/* E3CD CHF   - Compare High Fullword                          [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_high_fullword)                                 /*810*/
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            (S32)regs->GR_H(r1) < (S32)n ? 1 :
            (S32)regs->GR_H(r1) > (S32)n ? 2 : 0;

} /* DEF_INST(compare_high_fullword) */


/*-------------------------------------------------------------------*/
/* CCxD CIH   - Compare High Immediate                         [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_high_immediate)                                /*810*/
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Compare signed operands and set condition code */
    regs->psw.cc = (S32)regs->GR_H(r1) < (S32)i2 ? 1 :
                   (S32)regs->GR_H(r1) > (S32)i2 ? 2 : 0;

} /* end DEF_INST(compare_high_immediate) */


/*-------------------------------------------------------------------*/
/* B9CF CLHHR - Compare Logical High High Register             [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_high_high_register)                    /*810*/
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = regs->GR_H(r1) < regs->GR_H(r2) ? 1 :
                   regs->GR_H(r1) > regs->GR_H(r2) ? 2 : 0;

} /* end DEF_INST(compare_logical_high_high_register) */


/*-------------------------------------------------------------------*/
/* B9DF CLHLR - Compare Logical High Low Register              [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_high_low_register)                     /*810*/
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = regs->GR_H(r1) < regs->GR_L(r2) ? 1 :
                   regs->GR_H(r1) > regs->GR_L(r2) ? 2 : 0;

} /* end DEF_INST(compare_logical_high_low_register) */


/*-------------------------------------------------------------------*/
/* E3CF CLHF  - Compare Logical High Fullword                  [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_high_fullword)                         /*810*/
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = regs->GR_H(r1) < n ? 1 :
                   regs->GR_H(r1) > n ? 2 : 0;

} /* end DEF_INST(compare_logical_high_fullword) */


/*-------------------------------------------------------------------*/
/* CCxF CLIH  - Compare Logical High Immediate                 [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_high_immediate)                        /*810*/
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = regs->GR_H(r1) < i2 ? 1 :
                   regs->GR_H(r1) > i2 ? 2 : 0;

} /* end DEF_INST(compare_logical_high_immediate) */


/*-------------------------------------------------------------------*/
/* E3C0 LBH   - Load Byte High                                 [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_byte_high)                                        /*810*/
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load sign-extended byte from operand address */
    regs->GR_H(r1) = (S8)ARCH_DEP(vfetchb) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_byte_high) */


/*-------------------------------------------------------------------*/
/* E3CA LFH   - Load Fullword High                             [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_fullword_high)                                    /*810*/
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register bits 0-31 from second operand */
    regs->GR_H(r1) = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_fullword_high) */


/*-------------------------------------------------------------------*/
/* E3C4 LHH   - Load Halfword High                             [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_halfword_high)                                    /*810*/
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load sign-extended halfword from operand address */
    regs->GR_H(r1) = (S16)ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_halfword_high) */


/*-------------------------------------------------------------------*/
/* E3C2 LLCH  - Load Logical Character High                    [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_character_high)                           /*810*/
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load byte into R1 register bits 24-31 and clear bits 0-23 */
    regs->GR_H(r1) = ARCH_DEP(vfetchb) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_logical_character_high) */


/*-------------------------------------------------------------------*/
/* E3C6 LLHH  - Load Logical Halfword High                     [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_halfword_high)                            /*810*/
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load halfword into R1 register bits 16-31 and clear bits 0-15 */
    regs->GR_H(r1) = ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_logical_halfword_high) */


/*-------------------------------------------------------------------*/
/* EC5D RISBHG - Rotate Then Insert Selected Bits High         [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(rotate_then_insert_selected_bits_high_long_reg)        /*810*/
{
    ARCH_DEP(rotate_then_xxx_selected_bits_long_reg) (inst, regs);
} /* end DEF_INST(rotate_then_insert_selected_bits_high_long_reg) */


/*-------------------------------------------------------------------*/
/* EC51 RISBLG - Rotate Then Insert Selected Bits Low          [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(rotate_then_insert_selected_bits_low_long_reg)         /*810*/
{
    ARCH_DEP(rotate_then_xxx_selected_bits_long_reg) (inst, regs);
} /* end DEF_INST(rotate_then_insert_selected_bits_low_long_reg) */


/*-------------------------------------------------------------------*/
/* E3C3 STCH  - Store Character High                           [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_character_high)                                  /*810*/
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Store bits 24-31 of R1 register at operand address */
    ARCH_DEP(vstoreb) ( regs->GR_HHLCL(r1), effective_addr2, b2, regs );

} /* end DEF_INST(store_character_high) */


/*-------------------------------------------------------------------*/
/* E3CB STFH  - Store Fullword High                            [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_fullword_high)                                   /*810*/
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Store bits 0-31 of R1 register at operand address */
    ARCH_DEP(vstore4) ( regs->GR_H(r1), effective_addr2, b2, regs );

} /* end DEF_INST(store_fullword_high) */


/*-------------------------------------------------------------------*/
/* E3C7 STHH  - Store Halfword High                            [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_halfword_high)                                   /*810*/
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Store bits 16-31 of R1 register at operand address */
    ARCH_DEP(vstore2) ( regs->GR_HHL(r1), effective_addr2, b2, regs );

} /* end DEF_INST(store_halfword_high) */


/*-------------------------------------------------------------------*/
/* B9C9 SHHHR - Subtract High High High Register               [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_high_high_high_register)                      /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR(inst, regs, r1, r2, r3);

    /* Subtract signed operands and set condition code */
    regs->psw.cc = sub_signed (&(regs->GR_H(r1)),
                                 regs->GR_H(r2),
                                 regs->GR_H(r3));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(subtract_high_high_high_register) */


/*-------------------------------------------------------------------*/
/* B9D9 SHHLR - Subtract High High Low Register                [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_high_high_low_register)                       /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR(inst, regs, r1, r2, r3);

    /* Subtract signed operands and set condition code */
    regs->psw.cc = sub_signed (&(regs->GR_H(r1)),
                                 regs->GR_H(r2),
                                 regs->GR_L(r3));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(subtract_high_high_low_register) */


/*-------------------------------------------------------------------*/
/* B9CB SLHHHR - Subtract Logical High High High Register      [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_high_high_high_register)              /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical (&(regs->GR_H(r1)),
                                  regs->GR_H(r2),
                                  regs->GR_H(r3));

} /* end DEF_INST(subtract_logical_high_high_high_register) */


/*-------------------------------------------------------------------*/
/* B9DB SLHHLR - Subtract Logical High High Low Register       [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_high_high_low_register)               /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical (&(regs->GR_H(r1)),
                                  regs->GR_H(r2),
                                  regs->GR_L(r3));

} /* end DEF_INST(subtract_logical_high_high_low_register) */

#endif /*defined(FEATURE_HIGH_WORD_FACILITY)*/                  /*810*/


#if defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)                /*810*/

/*-------------------------------------------------------------------*/
/* Load and Perform Interlocked Access Operation                     */
/* Subroutine called by LAA,LAAL,LAN,LAX,LAO instructions            */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_perform_interlocked_access)                   /*810*/
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    addr2;                          /* Effective address         */
BYTE    *m2;                            /* Mainstor address          */
U32     v2, v3;                         /* Operand values            */
U32     result;                         /* Result value              */
U32     old, new;                       /* Values for cmpxchg4       */
int     cc;                             /* Condition code            */
int     rc;                             /* Return code               */
BYTE    opcode;                         /* 2nd byte of opcode        */

    RSY(inst, regs, r1, r3, b2, addr2);

    /* Extract second byte of instruction opcode */
    opcode = inst[5];

    /* Obtain third operand value from R3 register */
    v3 = regs->GR_L(r3);

    /* Get mainstor address of storage operand */
    m2 = MADDRL (addr2, 4, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

    do {
        /* Load storage operand value from operand address */
        v2 = ARCH_DEP(vfetch4) ( addr2, b2, regs );

        switch (opcode) {
        case 0xF4: /* Load and And */
            /* AND operand values and set condition code */
            result = v2 & v3;
            cc = result ? 1 : 0;
            break;
        case 0xF6: /* Load and Or */
            /* OR operand values and set condition code */
            result = v2 | v3;
            cc = result ? 1 : 0;
            break;
        case 0xF7: /* Load and Exclusive Or */
            /* XOR operand values and set condition code */
            result = v2 ^ v3;
            cc = result ? 1 : 0;
            break;
        case 0xF8: /* Load and Add */
            /* Add signed operands and set condition code */
            cc = add_signed (&result, v2, v3);
            break;
        case 0xFA: /* Load and Add Logical */
            /* Add unsigned operands and set condition code */
            cc = add_logical (&result, v2, v3);
            break;
        default: /* To prevent compiler warnings */
            result = 0;
            cc = 0;
        } /* end switch(opcode) */

        /* Interlocked exchange to storage location */
        old = CSWAP32(v2);
        new = CSWAP32(result);
        rc = cmpxchg4 (&old, new, m2);

    } while (rc != 0);

    /* Load original storage operand value into R1 register */
    regs->GR_L(r1) = v2;

    /* Set condition code in PSW */
    regs->psw.cc = cc;

} /* end DEF_INST(load_and_perform_interlocked_access) */

/*-------------------------------------------------------------------*/
/* Load and Perform Interlocked Access Operation Long                */
/* Subroutine called by LAAG,LAALG,LANG,LAXG,LAOG instructions       */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_perform_interlocked_access_long)              /*810*/
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    addr2;                          /* Effective address         */
BYTE    *m2;                            /* Mainstor address          */
U64     v2, v3;                         /* Operand values            */
U64     result;                         /* Result value              */
U64     old, new;                       /* Values for cmpxchg4       */
int     cc;                             /* Condition code            */
int     rc;                             /* Return code               */
BYTE    opcode;                         /* 2nd byte of opcode        */

    RSY(inst, regs, r1, r3, b2, addr2);

    /* Extract second byte of instruction opcode */
    opcode = inst[5];

    /* Obtain third operand value from R3 register */
    v3 = regs->GR_G(r3);

    /* Get mainstor address of storage operand */
    m2 = MADDRL (addr2, 8, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

    do {
        /* Load storage operand value from operand address */
        v2 = ARCH_DEP(vfetch8) ( addr2, b2, regs );

        switch (opcode) {
        case 0xE4: /* Load and And Long */
            /* AND operand values and set condition code */
            result = v2 & v3;
            cc = result ? 1 : 0;
            break;
        case 0xE6: /* Load and Or Long */
            /* OR operand values and set condition code */
            result = v2 | v3;
            cc = result ? 1 : 0;
            break;
        case 0xE7: /* Load and Exclusive Or Long */
            /* XOR operand values and set condition code */
            result = v2 ^ v3;
            cc = result ? 1 : 0;
            break;
        case 0xE8: /* Load and Add Long */
            /* Add signed operands and set condition code */
            cc = add_signed_long (&result, v2, v3);
            break;
        case 0xEA: /* Load and Add Logical Long */
            /* Add unsigned operands and set condition code */
            cc = add_logical_long (&result, v2, v3);
            break;
        default: /* To prevent compiler warnings */
            result = 0;
            cc = 0;
        } /* end switch(opcode) */

        /* Interlocked exchange to storage location */
        old = CSWAP64(v2);
        new = CSWAP64(result);
        rc = cmpxchg8 (&old, new, m2);

    } while (rc != 0);

    /* Load original storage operand value into R1 register */
    regs->GR_G(r1) = v2;

    /* Set condition code in PSW */
    regs->psw.cc = cc;

} /* end DEF_INST(load_and_perform_interlocked_access_long) */


/*-------------------------------------------------------------------*/
/* EBF8 LAA   - Load and Add                                   [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_add)                                          /*810*/
{
    ARCH_DEP(load_and_perform_interlocked_access) (inst, regs);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(load_and_add) */


/*-------------------------------------------------------------------*/
/* EBE8 LAAG  - Load and Add Long                              [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_add_long)                                     /*810*/
{
    ARCH_DEP(load_and_perform_interlocked_access_long) (inst, regs);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(load_and_add_long) */


/*-------------------------------------------------------------------*/
/* EBFA LAAL  - Load and Add Logical                           [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_add_logical)                                  /*810*/
{
    ARCH_DEP(load_and_perform_interlocked_access) (inst, regs);
} /* end DEF_INST(load_and_add_logical) */


/*-------------------------------------------------------------------*/
/* EBEA LAALG - Load and Add Logical Long                      [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_add_logical_long)                             /*810*/
{
    ARCH_DEP(load_and_perform_interlocked_access_long) (inst, regs);
} /* end DEF_INST(load_and_add_logical_long) */


/*-------------------------------------------------------------------*/
/* EBF4 LAN   - Load and And                                   [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_and)                                          /*810*/
{
    ARCH_DEP(load_and_perform_interlocked_access) (inst, regs);
} /* end DEF_INST(load_and_and) */


/*-------------------------------------------------------------------*/
/* EBE4 LANG  - Load and And Long                              [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_and_long)                                     /*810*/
{
    ARCH_DEP(load_and_perform_interlocked_access_long) (inst, regs);
} /* end DEF_INST(load_and_and_long) */


/*-------------------------------------------------------------------*/
/* EBF7 LAX   - Load and Exclusive Or                          [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_exclusive_or)                                 /*810*/
{
    ARCH_DEP(load_and_perform_interlocked_access) (inst, regs);
} /* end DEF_INST(load_and_exclusive_or) */


/*-------------------------------------------------------------------*/
/* EBE7 LAXG  - Load and Exclusive Or Long                     [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_exclusive_or_long)                            /*810*/
{
    ARCH_DEP(load_and_perform_interlocked_access_long) (inst, regs);
} /* end DEF_INST(load_and_exclusive_or_long) */


/*-------------------------------------------------------------------*/
/* EBF6 LAO   - Load and Or                                    [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_or)                                           /*810*/
{
    ARCH_DEP(load_and_perform_interlocked_access) (inst, regs);
} /* end DEF_INST(load_and_or) */


/*-------------------------------------------------------------------*/
/* EBE6 LAOG  - Load and Or Long                               [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_or_long)                                      /*810*/
{
    ARCH_DEP(load_and_perform_interlocked_access_long) (inst, regs);
} /* end DEF_INST(load_and_or_long) */


/*-------------------------------------------------------------------*/
/* C8x4 LPD   - Load Pair Disjoint                             [SSF] */
/*-------------------------------------------------------------------*/
DEF_INST(load_pair_disjoint)                                    /*810*/
{
int     r3;                             /* Register number           */
int     b1, b2;                         /* Base register numbers     */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
U32     v1, v2;                         /* Operand values            */
U32     w1, w2;                         /* Refetched values          */

    SSF(inst, regs, b1, effective_addr1, b2, effective_addr2, r3);

    ODD_CHECK(r3, regs);

    /* Fetch the values of the storage operands */
    v1 = ARCH_DEP(vfetch4) ( effective_addr1, b1, regs );
    v2 = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Fetch operands again to check for alteration by another CPU */
    w1 = ARCH_DEP(vfetch4) ( effective_addr1, b1, regs );
    w2 = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Load R3 register from first storage operand */
    regs->GR_L(r3) = v1;

    /* Load R3+1 register from second storage operand */
    regs->GR_L(r3+1) = v2;

    /* Set condition code 0 if operands unaltered, or 3 if altered */
    regs->psw.cc = (v1 == w1 && v2 == w2) ? 0 : 3;

} /* end DEF_INST(load_pair_disjoint) */


/*-------------------------------------------------------------------*/
/* C8x5 LPDG  - Load Pair Disjoint Long                        [SSF] */
/*-------------------------------------------------------------------*/
DEF_INST(load_pair_disjoint_long)                               /*810*/
{
int     r3;                             /* Register number           */
int     b1, b2;                         /* Base register numbers     */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
U64     v1, v2;                         /* Operand values            */
U64     w1, w2;                         /* Refetched values          */

    SSF(inst, regs, b1, effective_addr1, b2, effective_addr2, r3);

    ODD_CHECK(r3, regs);

    /* Fetch the values of the storage operands */
    v1 = ARCH_DEP(vfetch8) ( effective_addr1, b1, regs );
    v2 = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Fetch operands again to check for alteration by another CPU */
    w1 = ARCH_DEP(vfetch8) ( effective_addr1, b1, regs );
    w2 = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Load R3 register from first storage operand */
    regs->GR_G(r3) = v1;

    /* Load R3+1 register from second storage operand */
    regs->GR_G(r3+1) = v2;

    /* Set condition code 0 if operands unaltered, or 3 if altered */
    regs->psw.cc = (v1 == w1 && v2 == w2) ? 0 : 3;

} /* end DEF_INST(load_pair_disjoint_long) */

#endif /*defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)*/         /*810*/


#if defined(FEATURE_LOAD_STORE_ON_CONDITION_FACILITY)           /*810*/

/*-------------------------------------------------------------------*/
/* B9F2 LOCR  - Load on Condition Register                     [RRF] */
/*-------------------------------------------------------------------*/
DEF_INST(load_on_condition_register)                            /*810*/
{
int     r1, r2;                         /* Values of R fields        */
int     m3;                             /* Value of M field          */

    RRF_M(inst, regs, r1, r2, m3);

    /* Test M3 mask bit corresponding to condition code */
    if (m3 & (0x08 >> regs->psw.cc))
    {
        /* Copy R2 register bits 32-63 to R1 register */
        regs->GR_L(r1) = regs->GR_L(r2);
    }

} /* end DEF_INST(load_on_condition_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B9E2 LOCGR - Load on Condition Long Register                [RRF] */
/*-------------------------------------------------------------------*/
DEF_INST(load_on_condition_long_register)                       /*810*/
{
int     r1, r2;                         /* Values of R fields        */
int     m3;                             /* Value of M field          */

    RRF_M(inst, regs, r1, r2, m3);

    /* Test M3 mask bit corresponding to condition code */
    if (m3 & (0x08 >> regs->psw.cc))
    {
        /* Copy R2 register bits 0-63 to R1 register */
        regs->GR_G(r1) = regs->GR_G(r2);
    }

} /* end DEF_INST(load_on_condition_long_register) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* EBF2 LOC   - Load on Condition                              [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_on_condition)                                     /*810*/
{
int     r1;                             /* Value of R field          */
int     m3;                             /* Value of M field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RSY(inst, regs, r1, m3, b2, effective_addr2);

    /* Test M3 mask bit corresponding to condition code */
    if (m3 & (0x08 >> regs->psw.cc))
    {
        /* Load R1 register bits 32-63 from second operand */
        regs->GR_L(r1) = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );
    }

} /* end DEF_INST(load_on_condition) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EBE2 LOCG  - Load on Condition Long                         [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_on_condition_long)                                /*810*/
{
int     r1;                             /* Value of R field          */
int     m3;                             /* Value of M field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RSY(inst, regs, r1, m3, b2, effective_addr2);

    /* Test M3 mask bit corresponding to condition code */
    if (m3 & (0x08 >> regs->psw.cc))
    {
        /* Load R1 register bits 0-63 from second operand */
        regs->GR_G(r1) = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );
    }

} /* end DEF_INST(load_on_condition_long) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* EBF3 STOC  - Store on Condition                             [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_on_condition)                                    /*810*/
{
int     r1;                             /* Value of R field          */
int     m3;                             /* Value of M field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RSY(inst, regs, r1, m3, b2, effective_addr2);

    /* Test M3 mask bit corresponding to condition code */
    if (m3 & (0x08 >> regs->psw.cc))
    {
        /* Store R1 register bits 32-63 at operand address */
        ARCH_DEP(vstore4) ( regs->GR_L(r1), effective_addr2, b2, regs );
    }

} /* end DEF_INST(store_on_condition) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EBE3 STOCG - Store on Condition Long                        [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_on_condition_long)                               /*810*/
{
int     r1;                             /* Value of R field          */
int     m3;                             /* Value of M field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RSY(inst, regs, r1, m3, b2, effective_addr2);

    /* Test M3 mask bit corresponding to condition code */
    if (m3 & (0x08 >> regs->psw.cc))
    {
        /* Store R1 register bits 0-63 at operand address */
        ARCH_DEP(vstore8) ( regs->GR_G(r1), effective_addr2, b2, regs );
    }

} /* end DEF_INST(store_on_condition_long) */
#endif /*defined(FEATURE_ESAME)*/

#endif /*defined(FEATURE_LOAD_STORE_ON_CONDITION_FACILITY)*/    /*810*/


#if defined(FEATURE_DISTINCT_OPERANDS_FACILITY)                 /*810*/

/*-------------------------------------------------------------------*/
/* B9F8 ARK   - Add Distinct Register                          [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_distinct_register)                                 /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR(inst, regs, r1, r2, r3);

    /* Add signed operands and set condition code */
    regs->psw.cc =
            add_signed (&(regs->GR_L(r1)),
                    regs->GR_L(r2),
                    regs->GR_L(r3));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_distinct_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B9E8 AGRK  - Add Distinct Long Register                     [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_distinct_long_register)                            /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR(inst, regs, r1, r2, r3);

    /* Add signed operands and set condition code */
    regs->psw.cc = add_signed_long(&(regs->GR_G(r1)),
                                     regs->GR_G(r2),
                                     regs->GR_G(r3));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_distinct_long_register) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* ECD8 AHIK  - Add Distinct Halfword Immediate                [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(add_distinct_halfword_immediate)                       /*810*/
{
int     r1, r3;                         /* Values of R fields        */
U16     i2;                             /* 16-bit immediate operand  */

    RIE(inst, regs, r1, r3, i2);

    /* Add signed operands and set condition code */
    regs->psw.cc = add_signed (&(regs->GR_L(r1)),
                                 (S16)i2,
                                 regs->GR_L(r3));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_distinct_halfword_immediate) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* ECD9 AGHIK - Add Distinct Long Halfword Immediate           [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(add_distinct_long_halfword_immediate)                  /*810*/
{
int     r1, r3;                         /* Values of R fields        */
U16     i2;                             /* 16-bit immediate operand  */

    RIE(inst, regs, r1, r3, i2);

    /* Add signed operands and set condition code */
    regs->psw.cc = add_signed_long(&(regs->GR_G(r1)),
                                     (S16)i2,
                                     regs->GR_G(r3));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_distinct_long_halfword_immediate) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* B9FA ALRK  - Add Logical Distinct Register                  [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_distinct_register)                         /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* Add signed operands and set condition code */
    regs->psw.cc = add_logical (&(regs->GR_L(r1)),
                                  regs->GR_L(r2),
                                  regs->GR_L(r3));

} /* end DEF_INST(add_logical_distinct_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B9EA ALGRK - Add Logical Distinct Long Register             [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_distinct_long_register)                    /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* Add unsigned operands and set condition code */
    regs->psw.cc = add_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r2),
                                      regs->GR_G(r3));

} /* end DEF_INST(add_logical_distinct_long_register) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* ECDA ALHSIK - Add Logical Distinct with Signed Halfword Imm [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_distinct_signed_halfword_immediate)        /*810*/
{
int     r1, r3;                         /* Values of R fields        */
U16     i2;                             /* 16-bit immediate operand  */

    RIE0(inst, regs, r1, r3, i2);

    /* Add operands and set condition code */
    regs->psw.cc = (S16)i2 < 0 ?
        sub_logical (&(regs->GR_L(r1)), regs->GR_L(r3), (S32)(-(S16)i2)) :
        add_logical (&(regs->GR_L(r1)), regs->GR_L(r3), (S32)(S16)i2);

} /* end DEF_INST(add_logical_distinct_signed_halfword_immediate) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* ECDB ALGHSIK - Add Logical Distinct Long with Signed Hw Imm [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_distinct_long_signed_halfword_immediate)   /*810*/
{
int     r1, r3;                         /* Values of R fields        */
U16     i2;                             /* 16-bit immediate operand  */

    RIE0(inst, regs, r1, r3, i2);

    /* Add operands and set condition code */
    regs->psw.cc = (S16)i2 < 0 ?
        sub_logical_long (&(regs->GR_G(r1)), regs->GR_G(r3), (S64)(-(S16)i2)) :
        add_logical_long (&(regs->GR_G(r1)), regs->GR_G(r3), (S64)(S16)i2);

} /* end DEF_INST(add_logical_distinct_long_signed_halfword_immediate) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* B9F4 NRK   - And Distinct Register                          [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(and_distinct_register)                                 /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* AND second and third operands and put result in first operand */
    regs->GR_L(r1) = regs->GR_L(r2) & regs->GR_L(r3);

    /* Set condition code 1 if result is non-zero, otherwise 0 */
    regs->psw.cc = (regs->GR_L(r1)) ? 1 : 0;

} /* end DEF_INST(and_distinct_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B9E4 NGRK  - And Distinct Long Register                     [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(and_distinct_long_register)                            /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* AND second and third operands and put result in first operand */
    regs->GR_G(r1) = regs->GR_G(r2) & regs->GR_G(r3);

    /* Set condition code 1 if result is non-zero, otherwise 0 */
    regs->psw.cc = (regs->GR_G(r1)) ? 1 : 0;

} /* end DEF_INST(and_distinct_long_register) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* B9F7 XRK   - Exclusive Or Distinct Register                 [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(exclusive_or_distinct_register)                        /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* XOR second and third operands and put result in first operand */
    regs->GR_L(r1) = regs->GR_L(r2) ^ regs->GR_L(r3);

    /* Set condition code 1 if result is non-zero, otherwise 0 */
    regs->psw.cc = (regs->GR_L(r1)) ? 1 : 0;

} /* end DEF_INST(exclusive_or_distinct_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B9E7 XGRK  - Exclusive Or Distinct Long Register            [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(exclusive_or_distinct_long_register)                   /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* XOR second and third operands and put result in first operand */
    regs->GR_G(r1) = regs->GR_G(r2) ^ regs->GR_G(r3);

    /* Set condition code 1 if result is non-zero, otherwise 0 */
    regs->psw.cc = (regs->GR_G(r1)) ? 1 : 0;

} /* end DEF_INST(exclusive_or_distinct_long_register) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* B9F6 ORK   - Or Distinct Register                           [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(or_distinct_register)                                  /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* OR second and third operands and put result in first operand */
    regs->GR_L(r1) = regs->GR_L(r2) | regs->GR_L(r3);

    /* Set condition code 1 if result is non-zero, otherwise 0 */
    regs->psw.cc = (regs->GR_L(r1)) ? 1 : 0;

} /* end DEF_INST(or_distinct_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B9E6 OGRK  - Or Distinct Long Register                      [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(or_distinct_long_register)                             /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* OR second and third operands and put result in first operand */
    regs->GR_G(r1) = regs->GR_G(r2) | regs->GR_G(r3);

    /* Set condition code 1 if result is non-zero, otherwise 0 */
    regs->psw.cc = (regs->GR_G(r1)) ? 1 : 0;

} /* end DEF_INST(or_distinct_long_register) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* EBDC SRAK  - Shift Right Single Distinct                    [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_right_single_distinct)                           /*810*/
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* Integer work area         */

    RSY0(inst, regs, r1, r3, b2, effective_addr2);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Shift signed value of the R3 register, result in R1 register */
    regs->GR_L(r1) = n > 30 ?
                    ((S32)regs->GR_L(r3) < 0 ? -1 : 0) :
                    (S32)regs->GR_L(r3) >> n;

    /* Set the condition code */
    regs->psw.cc = ((S32)regs->GR_L(r1) > 0) ? 2 :
                   (((S32)regs->GR_L(r1) < 0) ? 1 : 0);

} /* end DEF_INST(shift_right_single_distinct) */


/*-------------------------------------------------------------------*/
/* EBDD SLAK  - Shift Left Single Distinct                     [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_left_single_distinct)                            /*810*/
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n, n1, n2;                      /* 32-bit operand values     */
U32     i, j;                           /* Integer work areas        */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Fast path if no possible overflow */
    if (regs->GR_L(r3) < 0x10000 && n < 16)
    {
        regs->GR_L(r1) = regs->GR_L(r3) << n;
        regs->psw.cc = regs->GR_L(r1) ? 2 : 0;
        return;
    }

    /* Load the numeric and sign portions from the R3 register */
    n1 = regs->GR_L(r3) & 0x7FFFFFFF;
    n2 = regs->GR_L(r3) & 0x80000000;

    /* Shift the numeric portion left n positions */
    for (i = 0, j = 0; i < n; i++)
    {
        /* Shift bits 1-31 left one bit position */
        n1 <<= 1;

        /* Overflow if bit shifted out is unlike the sign bit */
        if ((n1 & 0x80000000) != n2)
            j = 1;
    }

    /* Load the updated value into the R1 register */
    regs->GR_L(r1) = (n1 & 0x7FFFFFFF) | n2;

    /* Condition code 3 and program check if overflow occurred */
    if (j)
    {
        regs->psw.cc = 3;
        if ( FOMASK(&regs->psw) )
            regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);
        return;
    }

    /* Set the condition code */
    regs->psw.cc = (S32)regs->GR_L(r1) > 0 ? 2 :
                   (S32)regs->GR_L(r1) < 0 ? 1 : 0;

} /* end DEF_INST(shift_left_single_distinct) */


/*-------------------------------------------------------------------*/
/* EBDE SRLK  - Shift Right Single Logical Distinct            [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_right_single_logical_distinct)                   /*810*/
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* Integer work area         */

    RSY0(inst, regs, r1, r3, b2, effective_addr2);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Shift the R3 register and place the result in the R1 register */
    regs->GR_L(r1) = n > 31 ? 0 : regs->GR_L(r3) >> n;

} /* end DEF_INST(shift_right_single_logical_distinct) */


/*-------------------------------------------------------------------*/
/* EBDF SLLK  - Shift Left Single Logical Distinct             [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_left_single_logical_distinct)                    /*810*/
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* Integer work area         */

    RSY0(inst, regs, r1, r3, b2, effective_addr2);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Shift the R3 register and place the result in the R1 register */
    regs->GR_L(r1) = n > 31 ? 0 : regs->GR_L(r3) << n;

} /* end DEF_INST(shift_left_single_logical_distinct) */


/*-------------------------------------------------------------------*/
/* B9F9 SRK   - Subtract Distinct Register                     [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_distinct_register)                            /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR(inst, regs, r1, r2, r3);

    /* Subtract signed operands and set condition code */
    regs->psw.cc =
            sub_signed (&(regs->GR_L(r1)),
                    regs->GR_L(r2),
                    regs->GR_L(r3));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(subtract_distinct_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B9E9 SGRK  - Subtract Distinct Long Register                [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_distinct_long_register)                       /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR(inst, regs, r1, r2, r3);

    /* Subtract signed operands and set condition code */
    regs->psw.cc = sub_signed_long(&(regs->GR_G(r1)),
                                     regs->GR_G(r2),
                                     regs->GR_G(r3));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(subtract_distinct_long_register) */
#endif /*defined(FEATURE_ESAME)*/


/*-------------------------------------------------------------------*/
/* B9FB SLRK  - Subtract Logical Distinct Register             [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_distinct_register)                    /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical (&(regs->GR_L(r1)),
                                  regs->GR_L(r2),
                                  regs->GR_L(r3));

} /* end DEF_INST(subtract_logical_distinct_register) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B9EB SLGRK - Subtract Logical Distinct Long Register        [RRR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_distinct_long_register)               /*810*/
{
int     r1, r2, r3;                     /* Values of R fields        */

    RRR0(inst, regs, r1, r2, r3);

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r2),
                                      regs->GR_G(r3));

} /* end DEF_INST(subtract_logical_distinct_long_register) */
#endif /*defined(FEATURE_ESAME)*/

#endif /*defined(FEATURE_DISTINCT_OPERANDS_FACILITY)*/          /*810*/


#if defined(FEATURE_POPULATION_COUNT_FACILITY)                  /*810*/
/*-------------------------------------------------------------------*/
/* B9E1 POPCNT - Population Count                              [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(population_count)                                      /*810*/
{
int     r1, r2;                         /* Values of R fields        */
int     i;                              /* Loop counter              */
U64     n;                              /* Contents of R2 register   */
U64     result;                         /* Result counter            */
U64     mask = 0x0101010101010101ULL;   /* Bit mask                  */

    RRE0(inst, regs, r1, r2);

    /* Load the value to be counted from the R2 register */
    n = regs->GR_G(r2);

    /* Count the number of 1 bits in each byte */
    for (i = 0, result = 0; i < 8; i++) {
        result += n & mask;
        n >>= 1;
    }

    /* Load the result into the R1 register */
    regs->GR_G(r1) = result;

    /* Set condition code 0 if result is zero, or 1 if non-zero */
    regs->psw.cc = (result == 0) ? 0 : 1;

} /* end DEF_INST(population_count) */
#endif /*defined(FEATURE_POPULATION_COUNT_FACILITY)*/           /*810*/


#if defined(FEATURE_LOAD_AND_TRAP_FACILITY)                     /*912*/

/*-------------------------------------------------------------------*/
/* E39F LAT   - Load and Trap                                  [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_trap)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_L(r1) = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Raise data exception if result is zero */
    if (regs->GR_L(r1) == 0)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(load_and_trap) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E385 LGAT  - Load Long and Trap                             [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_long_and_trap)                                    /*912*/
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_G(r1) = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Raise data exception if result is zero */
    if (regs->GR_G(r1) == 0)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(load_long_and_trap) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E3C8 LFHAT - Load Fullword High and Trap                    [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_fullword_high_and_trap)                           /*912*/
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register bits 0-31 from second operand */
    regs->GR_H(r1) = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Raise data exception if result is zero */
    if (regs->GR_H(r1) == 0)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(load_fullword_high_and_trap) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E39D LLGFAT - Load Logical Long Fullword and Trap           [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_long_fullword_and_trap)                   /*912*/
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_G(r1) = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Raise data exception if result is zero */
    if (regs->GR_G(r1) == 0)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(load_logical_long_fullword_and_trap) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E39C LLGTAT - Load Logical Long Thirtyone and Trap          [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_long_thirtyone_and_trap)                  /*912*/
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_G(r1) = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs )
                                                        & 0x7FFFFFFF;

    /* Raise data exception if result is zero */
    if (regs->GR_G(r1) == 0)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(load_logical_long_thirtyone_and_trap) */
#endif /*defined(FEATURE_ESAME)*/

#endif /*defined(FEATURE_LOAD_AND_TRAP_FACILITY)*/              /*912*/


#if defined(FEATURE_MISC_INSTRUCTION_EXTENSIONS_FACILITY)       /*912*/

/*-------------------------------------------------------------------*/
/* EB23 CLT   - Compare Logical and Trap                       [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_and_trap)                              /*912*/
{
int     r1;                             /* Register number           */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand value      */
int     m3;                             /* Mask bits                 */
int     cc;                             /* Comparison result         */

    RSY(inst, regs, r1, m3, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_L(r1) < n ? 1 :
         regs->GR_L(r1) > n ? 2 : 0;

    /* Raise data exception if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_logical_and_trap) */


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB2B CLGT  - Compare Logical and Trap Long                  [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_and_trap_long)                         /*912*/
{
int     r1;                             /* Register number           */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand value      */
int     m3;                             /* Mask bits                 */
int     cc;                             /* Comparison result         */

    RSY(inst, regs, r1, m3, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Compare unsigned operands and set comparison result */
    cc = regs->GR_G(r1) < n ? 1 :
         regs->GR_G(r1) > n ? 2 : 0;

    /* Raise data exception if m3 mask bit is set */
    if ((0x8 >> cc) & m3)
    {
        regs->dxc = DXC_COMPARE_AND_TRAP;
        ARCH_DEP(program_interrupt) (regs, PGM_DATA_EXCEPTION);
    }

} /* end DEF_INST(compare_logical_and_trap_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EC59 RISBGN - Rotate Then Insert Selected Bits No CC        [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(rotate_then_insert_selected_bits_long_reg_n)
{
    ARCH_DEP(rotate_then_xxx_selected_bits_long_reg) (inst, regs);
} /* end DEF_INST(rotate_then_insert_selected_bits_long_reg_n) */
#endif /*defined(FEATURE_ESAME)*/


#endif /*defined(FEATURE_MISC_INSTRUCTION_EXTENSIONS_FACILITY)*/

#if defined(FEATURE_EXECUTION_HINT_FACILITY)                    /*912*/

/*-------------------------------------------------------------------*/
/* C7   BPP   - Branch Prediction Preload                      [SMI] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_prediction_preload)                             /*912*/
{
VADR    addr2, addr3;                   /* Effective addresses       */
int     b3;                             /* Base of effective address */
int     m1;                             /* Mask value                */

    SMI_A0(inst, regs, m1, addr2, b3, addr3);

    /* Depending on the model, the CPU may not implement
       all of the branch-attribute codes. For codes that
       are not recognized by the CPU, and for reserved
       codes, the BPP instruction acts as a no-operation */

} /* end DEF_INST(branch_prediction_preload) */

 
/*-------------------------------------------------------------------*/
/* C5   BPRP  - Branch Prediction Relative Preload             [MII] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_prediction_relative_preload)                    /*912*/
{
VADR    addr2, addr3;                   /* Effective addresses       */
int     m1;                             /* Mask value                */

    MII_A0(inst, regs, m1, addr2, addr3);

    /* Depending on the model, the CPU may not implement
       all of the branch-attribute codes. For codes that
       are not recognized by the CPU, and for reserved
       codes, the BPRP instruction acts as a no-operation */
        
} /* end DEF_INST(branch_prediction_relative_preload) */
 
 
/*-------------------------------------------------------------------*/
/* B2FA NIAI  - Next Instruction Access Intent                  [IE] */
/*-------------------------------------------------------------------*/
DEF_INST(next_instruction_access_intent)                        /*912*/
{
BYTE    i1, i2;                         /* Immediate fields          */

    IE0(inst, regs, i1, i2);

    /* Depending on the model, the CPU may not recognize all of the
       access intents. For access intents that are not recognized by
       the CPU, the NIAI instruction acts as a no-operation */

} /* end DEF_INST(next_instruction_access_intent) */

#endif /*defined(FEATURE_EXECUTION_HINT_FACILITY)*/             /*912*/


#if !defined(_GEN_ARCH)

#if defined(_ARCHMODE2)
 #define  _GEN_ARCH _ARCHMODE2
 #include "general3.c"
#endif

#if defined(_ARCHMODE3)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCHMODE3
 #include "general3.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
