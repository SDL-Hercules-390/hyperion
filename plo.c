/* PLO.C        (C) Copyright Jan Jaeger, 2000-2012                  */
/*              Perform Locked Operation functions codes             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

#include "hstdinc.h"

#define _PLO_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#if defined(FEATURE_PERFORM_LOCKED_OPERATION)
int ARCH_DEP(plo_cl) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U32 op2,
    op4;

    FW_CHECK(effective_addr2, regs);
    FW_CHECK(effective_addr4, regs);

    /* Load second operand from operand address  */
    op2 = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    if(regs->GR_L(r1) == op2)
    {

        op4 = ARCH_DEP(vfetch4) ( effective_addr4, b4, regs );
        regs->GR_L(r3) = op4;

        return 0;
    }
    else
    {
        regs->GR_L(r1) = op2;

        return 1;
    }

}


int ARCH_DEP(plo_clg) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U64 op1c,
    op2,
    op4;
U32 op4alet = 0;
VADR op4addr;

    UNREFERENCED(r1);

    DW_CHECK(effective_addr4, regs);
    DW_CHECK(effective_addr2, regs);

    /* load second operand */
    op2 = ARCH_DEP(vfetch8)(effective_addr2, b2, regs);

    /* load 1st op. compare value */
    op1c = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);

    if(op1c == op2)
    {
        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
#else
        op4addr = ARCH_DEP(wfetch4)(effective_addr4 + 76, b4, regs);
#endif
        op4addr &= ADDRESS_MAXWRAP(regs);
        DW_CHECK(op4addr, regs);

        /* Load operand 4, using ar3 when in ar mode */
        op4 = ARCH_DEP(vfetch8)(op4addr, r3, regs);

        /* replace the 3rd operand with the 4th operand */
        ARCH_DEP(wstore8)(op4, effective_addr4 + 40, b4, regs);

        return 0;
    }
    else
    {
        /* replace the first op compare value with 2nd op */
        ARCH_DEP(wstore8)(op2, effective_addr4 + 8, b4, regs);

        return 1;
    }
}


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
int ARCH_DEP(plo_clgr) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U64 op2,
    op4;

    DW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    /* Load second operand from operand address  */
    op2 = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    if(regs->GR_G(r1) == op2)
    {

        op4 = ARCH_DEP(vfetch8) ( effective_addr4, b4, regs );
        regs->GR_G(r3) = op4;

        return 0;
    }
    else
    {
        regs->GR_G(r1) = op2;

        return 1;
    }

}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
int ARCH_DEP(plo_clx) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1c[16],
     op2[16],
     op4[16];
U32 op4alet = 0;
VADR op4addr;

    UNREFERENCED(r1);

    DW_CHECK(effective_addr4, regs);
    QW_CHECK(effective_addr2, regs);

    /* load second operand */
    ARCH_DEP(vfetchc) ( op2, 16-1, effective_addr2, b2, regs );

    /* load 1st op. compare value */
    ARCH_DEP(vfetchc) ( op1c, 16-1, effective_addr4 + 0, b4, regs );

    if(memcmp(op1c,op2,16) == 0)
    {
        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
        op4addr &= ADDRESS_MAXWRAP(regs);
        QW_CHECK(op4addr, regs);

        /* Load operand 4, using ar3 when in ar mode */
        ARCH_DEP(vfetchc) ( op4, 16-1, op4addr, r3, regs );

        /* replace the 3rd operand with the 4th operand */
        ARCH_DEP(wstorec) ( op4, 16-1, effective_addr4 + 32, b4, regs );

        return 0;
    }
    else
    {
        /* replace the first op compare value with 2nd op */
        ARCH_DEP(vstorec) ( op2, 16-1, effective_addr4 + 0, b4, regs );

        return 1;
    }
}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


int ARCH_DEP(plo_cs) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U32 op2;

    UNREFERENCED(r3);
    UNREFERENCED(effective_addr4);
    UNREFERENCED(b4);

    ODD_CHECK(r1, regs);
    FW_CHECK(effective_addr2, regs);

    /* Load second operand from operand address  */
    op2 = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Compare operand with R1 register contents */
    if ( regs->GR_L(r1) == op2 )
    {
        /* If equal, store R1+1 at operand loc and set cc=0 */
        ARCH_DEP(vstore4) ( regs->GR_L(r1+1), effective_addr2, b2, regs );

        return 0;
    }
    else
    {
        /* If unequal, load R1 from operand and set cc=1 */
        regs->GR_L(r1) = op2;

        return 1;
    }

}


int ARCH_DEP(plo_csg) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U64 op1c,
    op1r,
    op2;

    UNREFERENCED(r1);
    UNREFERENCED(r3);

    DW_CHECK(effective_addr4, regs);
    DW_CHECK(effective_addr2, regs);

    /* Load first op compare value */
    op1c = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);

    /* Load 2nd operand */
    op2 = ARCH_DEP(vfetch8)(effective_addr2, b2, regs);

    if(op1c == op2)
    {
        /* Load 1st op replacement value */
        op1r = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);

        /* Store at 2nd operand location */
        ARCH_DEP(vstore8)(op1r, effective_addr2, b2, regs);

        return 0;
    }
    else
    {
        /* Replace 1st op comp value by 2nd op */
        ARCH_DEP(wstore8)(op2, effective_addr4 + 8, b4, regs);

        return 1;
    }
}


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
int ARCH_DEP(plo_csgr) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U64 op2;

    UNREFERENCED(r3);
    UNREFERENCED(effective_addr4);
    UNREFERENCED(b4);

    ODD_CHECK(r1, regs);
    DW_CHECK(effective_addr2, regs);

    /* Load second operand from operand address  */
    op2 = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Compare operand with R1 register contents */
    if ( regs->GR_G(r1) == op2 )
    {
        /* If equal, store R1+1 at operand loc and set cc=0 */
        ARCH_DEP(vstore8) ( regs->GR_G(r1+1), effective_addr2, b2, regs );

        return 0;
    }
    else
    {
        /* If unequal, load R1 from operand and set cc=1 */
        regs->GR_G(r1) = op2;

        return 1;
    }

}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
int ARCH_DEP(plo_csx) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1c[16],
     op1r[16],
     op2[16];

    UNREFERENCED(r1);
    UNREFERENCED(r3);

    DW_CHECK(effective_addr4, regs);
    QW_CHECK(effective_addr2, regs);

    /* Load first op compare value */
    ARCH_DEP(vfetchc) ( op1c, 16-1, effective_addr4 + 0, b4, regs );

    /* Load 2nd operand */
    ARCH_DEP(vfetchc) ( op2, 16-1, effective_addr2, b2, regs );

    if(memcmp(op1c,op2,16) == 0)
    {
        /* Load 1st op replacement value */
        ARCH_DEP(wfetchc) ( op1r, 16-1, effective_addr4 + 16, b4, regs );

        /* Store at 2nd operand location */
        ARCH_DEP(vstorec) ( op1r, 16-1, effective_addr2, b2, regs );

        return 0;
    }
    else
    {
        /* Replace 1st op comp value by 2nd op */
        ARCH_DEP(vstorec) ( op2, 16-1, effective_addr4 + 0, b4, regs );

        return 1;
    }
}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


int ARCH_DEP(plo_dcs) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U32 op2,
    op4;

    ODD2_CHECK(r1, r3, regs);
    FW_CHECK(effective_addr2, regs);
    FW_CHECK(effective_addr4, regs);

    /* Load second operands from operand addresses  */
    op2 = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    if(regs->GR_L(r1) != op2)
    {
        regs->GR_L(r1) = op2;

        return 1;
    }
    else
    {
        op4 = ARCH_DEP(vfetch4) ( effective_addr4, b4, regs );

        /* Compare operand with register contents */
        if (regs->GR_L(r3) != op4)
        {
            /* If unequal, load r3 from op and set cc=2 */
            regs->GR_L(r3) = op4;

            return 2;
        }
        else
        {
            /* Verify access to 2nd operand */
            ARCH_DEP(validate_operand) (effective_addr2, b2, 4-1,
                ACCTYPE_WRITE_SKP, regs);

            /* If equal, store replacement and set cc=0 */
            ARCH_DEP(vstore4) ( regs->GR_L(r3+1), effective_addr4, b4, regs );
            ARCH_DEP(vstore4) ( regs->GR_L(r1+1), effective_addr2, b2, regs );

            return 0;
        }
    }

}


int ARCH_DEP(plo_dcsg) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U64 op1c,
    op1r,
    op2,
    op3c,
    op3r,
    op4;
U32 op4alet = 0;
VADR op4addr;

    UNREFERENCED(r1);

    DW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    /* load 1st op compare value from the pl */
    op1c = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);

    /* load 2nd operand */
    op2 = ARCH_DEP(vfetch8)(effective_addr2, b2, regs);

    if(op1c != op2)
    {
        /* replace the 1st op compare value with 2nd op */
        ARCH_DEP(wstore8)(op2, effective_addr4 + 8, b4, regs);

        return 1;
    }
    else
    {
        /* Load 3rd op compare value */
        op3c = ARCH_DEP(wfetch8)(effective_addr4 + 40, b4, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
#else
        op4addr = ARCH_DEP(wfetch4)(effective_addr4 + 76, b4, regs);
#endif
        op4addr &= ADDRESS_MAXWRAP(regs);
        DW_CHECK(op4addr, regs);

        /* Load operand 4, using ar3 when in ar mode */
        op4 = ARCH_DEP(vfetch8)(op4addr, r3, regs);

        if(op3c != op4)
        {
            ARCH_DEP(wstore8)(op4, effective_addr4 + 40, b4, regs);

            return 2;
        }
        else
        {
            /* load replacement values */
            op1r = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
            op3r = ARCH_DEP(wfetch8)(effective_addr4 + 56, b4, regs);

            /* Verify access to 2nd operand */
            ARCH_DEP(validate_operand) (effective_addr2, b2, 8-1,
                ACCTYPE_WRITE_SKP, regs);

            /* Store 3rd op replacement at 4th op */
            ARCH_DEP(vstore8)(op3r, op4addr, r3, regs);

            /* Store 1st op replacement at 2nd op */
            ARCH_DEP(vstore8)(op1r, effective_addr2, b2, regs);

            return 0;
        }
    }

}


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
int ARCH_DEP(plo_dcsgr) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U64 op2,
    op4;

    ODD2_CHECK(r1, r3, regs);
    DW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    /* Load second operands from operand addresses  */
    op2 = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    if(regs->GR_G(r1) != op2)
    {
        regs->GR_G(r1) = op2;

        return 1;
    }
    else
    {
        op4 = ARCH_DEP(vfetch8) ( effective_addr4, b4, regs );

        /* Compare operand with register contents */
        if (regs->GR_G(r3) != op4)
        {
            /* If unequal, load r3 from op and set cc=2 */
            regs->GR_G(r3) = op4;

            return 2;
        }
        else
        {
            /* Verify access to 2nd operand */
            ARCH_DEP(validate_operand) (effective_addr2, b2, 4-1,
                ACCTYPE_WRITE_SKP, regs);

            /* If equal, store replacement and set cc=0 */
            ARCH_DEP(vstore8) ( regs->GR_G(r3+1), effective_addr4, b4, regs );
            ARCH_DEP(vstore8) ( regs->GR_G(r1+1), effective_addr2, b2, regs );

            return 0;
        }
    }

}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
int ARCH_DEP(plo_dcsx) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1c[16],
     op1r[16],
     op2[16],
     op3c[16],
     op3r[16],
     op4[16];
U32 op4alet = 0;
VADR op4addr;

    UNREFERENCED(r1);

    QW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    /* load 1st op compare value from the pl */
    ARCH_DEP(vfetchc) ( op1c, 16-1, effective_addr4 + 0, b4, regs );

    /* load 2nd operand */
    ARCH_DEP(vfetchc) ( op2, 16-1, effective_addr2, b2, regs );

    if(memcmp(op1c,op2,16) != 0)
    {
        /* replace the 1st op compare value with 2nd op */
        ARCH_DEP(vstorec) ( op2, 16-1, effective_addr4 + 0, b4, regs );

        return 1;
    }
    else
    {
        /* Load 3rd op compare value */
        ARCH_DEP(wfetchc) ( op3c, 16-1, effective_addr4 + 32, b4, regs );

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
        op4addr &= ADDRESS_MAXWRAP(regs);
        QW_CHECK(op4addr, regs);

        /* Load operand 4, using ar3 when in ar mode */
        ARCH_DEP(vfetchc) ( op4, 16-1, op4addr, r3, regs );

        if(memcmp(op3c,op4,16) != 0)
        {
            ARCH_DEP(wstorec) ( op4, 16-1, effective_addr4 + 32, b4, regs );

            return 2;
        }
        else
        {
            /* load replacement values */
            ARCH_DEP(wfetchc) ( op1r, 16-1, effective_addr4 + 16, b4, regs );
            ARCH_DEP(wfetchc) ( op3r, 16-1, effective_addr4 + 48, b4, regs );

            /* Verify access to 2nd operand */
            ARCH_DEP(validate_operand) (effective_addr2, b2, 16-1,
                ACCTYPE_WRITE_SKP, regs);

            /* Store 3rd op replacement at 4th op */
            ARCH_DEP(vstorec) ( op3r, 16-1, op4addr, r3, regs);

            /* Store 1st op replacement at 2nd op */
            ARCH_DEP(vstorec) ( op1r, 16-1, effective_addr2, b2, regs);

            return 0;
        }
    }

}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


int ARCH_DEP(plo_csst) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U32 op2;

    ODD_CHECK(r1, regs);
    FW_CHECK(effective_addr2, regs);
    FW_CHECK(effective_addr4, regs);

    /* Load second operand from operand address  */
    op2 = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Compare operand with register contents */
    if ( regs->GR_L(r1) == op2)
    {
        /* Verify access to 2nd operand */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 4-1,
            ACCTYPE_WRITE_SKP, regs);

        /* If equal, store replacement and set cc=0 */
        ARCH_DEP(vstore4) ( regs->GR_L(r3), effective_addr4, b4, regs );
        ARCH_DEP(vstore4) ( regs->GR_L(r1+1), effective_addr2, b2, regs );

        return 0;
    }
    else
    {
        regs->GR_L(r1) = op2;

        return 1;
    }

}


int ARCH_DEP(plo_csstg) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U64 op1c,
    op1r,
    op2,
    op3;
U32 op4alet = 0;
VADR op4addr;

    UNREFERENCED(r1);

    DW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    op1c = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2 = ARCH_DEP(vfetch8)(effective_addr2, b2, regs);

    if(op1c == op2)
    {
        op1r = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
        op3 = ARCH_DEP(wfetch8)(effective_addr4 + 56, b4, regs);

        /* Verify access to 2nd operand */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 8-1,
            ACCTYPE_WRITE_SKP, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
#else
        op4addr = ARCH_DEP(wfetch4)(effective_addr4 + 76, b4, regs);
#endif
        op4addr &= ADDRESS_MAXWRAP(regs);
        DW_CHECK(op4addr, regs);

        ARCH_DEP(vstore8)(op3, op4addr, r3, regs);
        ARCH_DEP(vstore8)(op1r, effective_addr2, b2, regs);

        return 0;
    }
    else
    {
        /* Store 2nd op at 1st op compare value */
        ARCH_DEP(wstore8)(op2, effective_addr4 + 8, b4, regs);

        return 1;
    }

}


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
int ARCH_DEP(plo_csstgr) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U64 op2;

    ODD_CHECK(r1, regs);
    DW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    /* Load second operand from operand address  */
    op2 = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Compare operand with register contents */
    if ( regs->GR_G(r1) == op2)
    {
        /* Verify access to 2nd operand */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 8-1,
            ACCTYPE_WRITE_SKP, regs);

        /* If equal, store replacement and set cc=0 */
        ARCH_DEP(vstore8) ( regs->GR_G(r3), effective_addr4, b4, regs );
        ARCH_DEP(vstore8) ( regs->GR_G(r1+1), effective_addr2, b2, regs );

        return 0;
    }
    else
    {
        regs->GR_G(r1) = op2;

        return 1;
    }

}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
int ARCH_DEP(plo_csstx) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1c[16],
     op1r[16],
     op2[16],
     op3[16];
U32 op4alet = 0;
VADR op4addr;

    UNREFERENCED(r1);

    QW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    ARCH_DEP(vfetchc) ( op1c, 16-1, effective_addr4 + 0, b4, regs );
    ARCH_DEP(vfetchc) ( op2, 16-1, effective_addr2, b2, regs );

    if(memcmp(op1c,op2,16) == 0)
    {
        ARCH_DEP(wfetchc) ( op1r, 16-1, effective_addr4 + 16, b4, regs );
        ARCH_DEP(wfetchc) ( op3, 16-1, effective_addr4 + 48, b4, regs );

        /* Verify access to 2nd operand */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 16-1,
            ACCTYPE_WRITE_SKP, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
        op4addr &= ADDRESS_MAXWRAP(regs);
        QW_CHECK(op4addr, regs);

        ARCH_DEP(vstorec)(op3, 16-1, op4addr, r3, regs);
        ARCH_DEP(vstorec)(op1r, 16-1, effective_addr2, b2, regs);

        return 0;
    }
    else
    {
        /* Store 2nd op at 1st op compare value */
        ARCH_DEP(vstorec)(op2, 16-1, effective_addr4 + 0, b4, regs);

        return 1;
    }

}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


int ARCH_DEP(plo_csdst) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U32 op2,
    op3,
    op4alet = 0,
    op5,
    op6alet = 0;
VADR op4addr,
    op6addr;

    ODD_CHECK(r1, regs);
    FW_CHECK(effective_addr2, regs);
    FW_CHECK(effective_addr4, regs);

    op2 = ARCH_DEP(vfetch4)(effective_addr2, b2, regs);
    op3 = ARCH_DEP(wfetch4)(effective_addr4 + 60, b4, regs);
    op5 = ARCH_DEP(wfetch4)(effective_addr4 + 92, b4, regs);

    if(regs->GR_L(r1) == op2)
    {

        /* Verify access to 2nd operand */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 4-1,
            ACCTYPE_WRITE_SKP, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            op6alet = ARCH_DEP(wfetch4)(effective_addr4 + 100, b4, regs);
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
#else
        op4addr = ARCH_DEP(wfetch4)(effective_addr4 + 76, b4, regs);
#endif
        op4addr &= ADDRESS_MAXWRAP(regs);
        FW_CHECK(op4addr, regs);

        /* Load address of operand 6 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        op6addr = ARCH_DEP(wfetch8)(effective_addr4 + 104, b4, regs);
#else
        op6addr = ARCH_DEP(wfetch4)(effective_addr4 + 108, b4, regs);
#endif
        op6addr &= ADDRESS_MAXWRAP(regs);
        FW_CHECK(op6addr, regs);

        /* Verify access to 6th operand */
        ARCH_DEP(validate_operand) (op6addr, r3, 4-1,ACCTYPE_WRITE_SKP, regs);

        /* Store 3th op at 4th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore4)(op3, op4addr, r3, regs);

        /* Store 5th op at 6th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore4)(op5, op6addr, r3, regs);

        /* Store 1st op at 2nd op */
        ARCH_DEP(vstore4)(regs->GR_L(r1+1), effective_addr2, b2, regs);

        return 0;
    }
    else
    {
        regs->GR_L(r1) = op2;

        return 1;
    }
}


int ARCH_DEP(plo_csdstg) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U64 op1c,
    op1r,
    op2,
    op3,
    op5;
U32 op4alet = 0,
    op6alet = 0;
VADR op4addr,
    op6addr;

    UNREFERENCED(r1);

    DW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    op1c = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2 = ARCH_DEP(vfetch8)(effective_addr2, b2, regs);

    if(op1c == op2)
    {
        op1r = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
        op3 = ARCH_DEP(wfetch8)(effective_addr4 + 56, b4, regs);
        op5 = ARCH_DEP(wfetch8)(effective_addr4 + 88, b4, regs);

        /* Verify access to 2nd operand */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 8-1,
            ACCTYPE_WRITE_SKP, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            op6alet = ARCH_DEP(wfetch4)(effective_addr4 + 100, b4, regs);
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
#else
        op4addr = ARCH_DEP(wfetch4)(effective_addr4 + 76, b4, regs);
#endif
        op4addr &= ADDRESS_MAXWRAP(regs);
        DW_CHECK(op4addr, regs);

        /* Load address of operand 6 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        op6addr = ARCH_DEP(wfetch8)(effective_addr4 + 104, b4, regs);
#else
        op6addr = ARCH_DEP(wfetch4)(effective_addr4 + 108, b4, regs);
#endif
        op6addr &= ADDRESS_MAXWRAP(regs);
        DW_CHECK(op6addr, regs);

        /* Verify access to 6th operand */
        ARCH_DEP(validate_operand) (op6addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);

        /* Store 3th op at 4th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore8)(op3, op4addr, r3, regs);

        /* Store 5th op at 6th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore8)(op5, op6addr, r3, regs);

        /* Store 1st op replacement at 2nd op */
        ARCH_DEP(vstore8)(op1r, effective_addr2, b2, regs);

        return 0;
    }
    else
    {
        ARCH_DEP(wstore8)(op2, effective_addr4 + 8, b4, regs);

        return 1;
    }
}


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
int ARCH_DEP(plo_csdstgr) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U64 op2,
    op3,
    op5;
U32 op4alet = 0,
    op6alet = 0;
VADR op4addr,
    op6addr;

    ODD_CHECK(r1, regs);
    DW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    op2 = ARCH_DEP(vfetch8)(effective_addr2, b2, regs);

    if(regs->GR_G(r1) == op2)
    {
        op3 = ARCH_DEP(wfetch8)(effective_addr4 + 56, b4, regs);
        op5 = ARCH_DEP(wfetch8)(effective_addr4 + 88, b4, regs);

        /* Verify access to 2nd operand */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 8-1,
            ACCTYPE_WRITE_SKP, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            op6alet = ARCH_DEP(wfetch4)(effective_addr4 + 100, b4, regs);
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
        op4addr &= ADDRESS_MAXWRAP(regs);
        DW_CHECK(op4addr, regs);

        /* Load address of operand 6 */
        op6addr = ARCH_DEP(wfetch8)(effective_addr4 + 104, b4, regs);
        op6addr &= ADDRESS_MAXWRAP(regs);
        DW_CHECK(op6addr, regs);

        /* Verify access to 6th operand */
        ARCH_DEP(validate_operand) (op6addr, r3, 8-1,ACCTYPE_WRITE_SKP, regs);

        /* Store 3th op at 4th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore8)(op3, op4addr, r3, regs);

        /* Store 5th op at 6th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore8)(op5, op6addr, r3, regs);

        /* Store 1st op at 2nd op */
        ARCH_DEP(vstore8)(regs->GR_G(r1+1), effective_addr2, b2, regs);

        return 0;
    }
    else
    {
        regs->GR_G(r1) = op2;

        return 1;
    }
}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
int ARCH_DEP(plo_csdstx) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1c[16],
     op1r[16],
     op2[16],
     op3[16],
     op5[16];
U32 op4alet = 0,
    op6alet = 0;
VADR op4addr,
    op6addr;

    UNREFERENCED(r1);

    QW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    ARCH_DEP(vfetchc)(op1c, 16-1, effective_addr4 + 0, b4, regs);
    ARCH_DEP(vfetchc)(op2, 16-1, effective_addr2, b2, regs);

    if(memcmp(op1c,op2,16) == 0)
    {
        ARCH_DEP(wfetchc)(op1r, 16-1, effective_addr4 + 16, b4, regs);
        ARCH_DEP(wfetchc)(op3, 16-1, effective_addr4 + 48, b4, regs);
        ARCH_DEP(wfetchc)(op5, 16-1, effective_addr4 + 80, b4, regs);

        /* Verify access to 2nd operand */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 16-1,
            ACCTYPE_WRITE_SKP, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            op6alet = ARCH_DEP(wfetch4)(effective_addr4 + 100, b4, regs);
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
        op4addr &= ADDRESS_MAXWRAP(regs);
        QW_CHECK(op4addr, regs);

        /* Load address of operand 6 */
        op6addr = ARCH_DEP(wfetch8)(effective_addr4 + 104, b4, regs);
        op6addr &= ADDRESS_MAXWRAP(regs);
        QW_CHECK(op6addr, regs);

        /* Verify access to 6th operand */
        ARCH_DEP(validate_operand) (op6addr, r3, 16-1,ACCTYPE_WRITE_SKP, regs);

        /* Store 3th op at 4th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstorec)(op3, 16-1, op4addr, r3, regs);

        /* Store 5th op at 6th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstorec)(op5, 16-1, op6addr, r3, regs);

        /* Store 1st op replacement at 2nd op */
        ARCH_DEP(vstorec)(op1r, 16-1, effective_addr2, b2, regs);

        return 0;
    }
    else
    {
        ARCH_DEP(vstorec)(op2, 16-1, effective_addr4 + 0, b4, regs);

        return 1;
    }
}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


int ARCH_DEP(plo_cstst) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U32 op2,
    op3,
    op4alet = 0,
    op5,
    op6alet = 0,
    op7,
    op8alet = 0;
VADR op4addr,
    op6addr,
    op8addr;

    ODD_CHECK(r1, regs);
    FW_CHECK(effective_addr2, regs);
    FW_CHECK(effective_addr4, regs);

    op2 = ARCH_DEP(vfetch4)(effective_addr2, b2, regs);
    op3 = ARCH_DEP(wfetch4)(effective_addr4 + 60, b4, regs);
    op5 = ARCH_DEP(wfetch4)(effective_addr4 + 92, b4, regs);
    op7 = ARCH_DEP(wfetch4)(effective_addr4 + 124, b4, regs);

    if(regs->GR_L(r1) == op2)
    {
        /* Verify access to 2nd operand */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 4-1,
            ACCTYPE_WRITE_SKP, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            op6alet = ARCH_DEP(wfetch4)(effective_addr4 + 100, b4, regs);
            op8alet = ARCH_DEP(wfetch4)(effective_addr4 + 132, b4, regs);
            regs->AR(r3) = op8alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
#else
        op4addr = ARCH_DEP(wfetch4)(effective_addr4 + 76, b4, regs);
#endif
        op4addr &= ADDRESS_MAXWRAP(regs);
        FW_CHECK(op4addr, regs);

        /* Load address of operand 6 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        op6addr = ARCH_DEP(wfetch8)(effective_addr4 + 104, b4, regs);
#else
        op6addr = ARCH_DEP(wfetch4)(effective_addr4 + 108, b4, regs);
#endif
        op6addr &= ADDRESS_MAXWRAP(regs);
        FW_CHECK(op6addr, regs);

        /* Load address of operand 8 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        op8addr = ARCH_DEP(wfetch8)(effective_addr4 + 136, b4, regs);
#else
        op8addr = ARCH_DEP(wfetch4)(effective_addr4 + 140, b4, regs);
#endif
        op8addr &= ADDRESS_MAXWRAP(regs);
        FW_CHECK(op8addr, regs);

        /* Verify access to 8th operand */
        ARCH_DEP(validate_operand) (op8addr, r3, 4-1,ACCTYPE_WRITE_SKP, regs);

        /* Verify access to 6th operand */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(validate_operand) (op6addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);

        /* Store 3rd op at 4th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore4)(op3, op4addr, r3, regs);

        /* Store 5th op at 6th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore4)(op5, op6addr, r3, regs);

        /* Store 7th op at 8th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op8alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore4)(op7, op8addr, r3, regs);

        /* Store 1st op replacement at 2nd op */
        ARCH_DEP(vstore4)(regs->GR_L(r1+1), effective_addr2, b2, regs);

        return 0;
    }
    else
    {
        regs->GR_L(r1) = op2;

        return 1;
    }
}


int ARCH_DEP(plo_cststg) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U64 op1c,
    op1r,
    op2,
    op3,
    op5,
    op7;
U32 op4alet = 0,
    op6alet = 0,
    op8alet = 0;
VADR op4addr,
    op6addr,
    op8addr;

    UNREFERENCED(r1);

    DW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    op1c = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2 = ARCH_DEP(vfetch8)(effective_addr2, b2, regs);

    if(op1c == op2)
    {
        op1r = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
        op3 = ARCH_DEP(wfetch8)(effective_addr4 + 56, b4, regs);
        op5 = ARCH_DEP(wfetch8)(effective_addr4 + 88, b4, regs);
        op7 = ARCH_DEP(wfetch8)(effective_addr4 + 120, b4, regs);

        /* Verify access to 2nd operand */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 8-1,
            ACCTYPE_WRITE_SKP, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            op6alet = ARCH_DEP(wfetch4)(effective_addr4 + 100, b4, regs);
            op8alet = ARCH_DEP(wfetch4)(effective_addr4 + 132, b4, regs);
            regs->AR(r3) = op8alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
#else
        op4addr = ARCH_DEP(wfetch4)(effective_addr4 + 76, b4, regs);
#endif
        op4addr &= ADDRESS_MAXWRAP(regs);
        DW_CHECK(op4addr, regs);

        /* Load address of operand 6 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        op6addr = ARCH_DEP(wfetch8)(effective_addr4 + 104, b4, regs);
#else
        op6addr = ARCH_DEP(wfetch4)(effective_addr4 + 108, b4, regs);
#endif
        op6addr &= ADDRESS_MAXWRAP(regs);
        DW_CHECK(op6addr, regs);

        /* Load address of operand 8 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        op8addr = ARCH_DEP(wfetch8)(effective_addr4 + 136, b4, regs);
#else
        op8addr = ARCH_DEP(wfetch4)(effective_addr4 + 140, b4, regs);
#endif
        op8addr &= ADDRESS_MAXWRAP(regs);
        DW_CHECK(op8addr, regs);

        /* Verify access to 8th operand */
        ARCH_DEP(validate_operand) (op8addr, r3, 8-1,ACCTYPE_WRITE_SKP, regs);

        /* Verify access to 6th operand */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(validate_operand) (op6addr, r3, 8-1,ACCTYPE_WRITE_SKP, regs);

        /* Store 3th op at 4th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore8)(op3, op4addr, r3, regs);

        /* Store 5th op at 6th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore8)(op5, op6addr, r3, regs);

        /* Store 7th op at 8th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op8alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore8)(op7, op8addr, r3, regs);

        /* Store 1st op replacement value at 2nd op */
        ARCH_DEP(vstore8)(op1r, effective_addr2, b2, regs);

        return 0;
    }
    else
    {
        ARCH_DEP(wstore8)(op2, effective_addr4 + 8, b4, regs);

        return 1;
    }
}


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
int ARCH_DEP(plo_cststgr) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
U64 op2,
    op3,
    op5,
    op7;
U32 op4alet = 0,
    op6alet = 0,
    op8alet = 0;
VADR op4addr,
    op6addr,
    op8addr;

    ODD_CHECK(r1, regs);
    DW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    op2 = ARCH_DEP(vfetch8)(effective_addr2, b2, regs);

    if(regs->GR_G(r1) == op2)
    {
        op3 = ARCH_DEP(wfetch8)(effective_addr4 + 56, b4, regs);
        op5 = ARCH_DEP(wfetch8)(effective_addr4 + 88, b4, regs);
        op7 = ARCH_DEP(wfetch8)(effective_addr4 + 120, b4, regs);

        /* Verify access to 2nd operand */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 8-1,
            ACCTYPE_WRITE_SKP, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            op6alet = ARCH_DEP(wfetch4)(effective_addr4 + 100, b4, regs);
            op8alet = ARCH_DEP(wfetch4)(effective_addr4 + 132, b4, regs);
            regs->AR(r3) = op8alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
        op4addr &= ADDRESS_MAXWRAP(regs);
        DW_CHECK(op4addr, regs);

        /* Load address of operand 6 */
        op6addr = ARCH_DEP(wfetch8)(effective_addr4 + 104, b4, regs);
        op6addr &= ADDRESS_MAXWRAP(regs);
        DW_CHECK(op6addr, regs);

        /* Load address of operand 8 */
        op8addr = ARCH_DEP(wfetch8)(effective_addr4 + 136, b4, regs);
        op8addr &= ADDRESS_MAXWRAP(regs);
        DW_CHECK(op8addr, regs);

        /* Verify access to 8th operand */
        ARCH_DEP(validate_operand) (op8addr, r3, 8-1,ACCTYPE_WRITE_SKP, regs);

        /* Verify access to 6th operand */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(validate_operand) (op6addr, r3, 8-1,ACCTYPE_WRITE_SKP, regs);

        /* Store 3rd op at 4th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore8)(op3, op4addr, r3, regs);

        /* Store 5th op at 6th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore8)(op5, op6addr, r3, regs);

        /* Store 7th op at 8th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op8alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstore8)(op7, op8addr, r3, regs);

        /* Store 1st op replacement at 2nd op */
        ARCH_DEP(vstore8)(regs->GR_G(r1+1), effective_addr2, b2, regs);

        return 0;
    }
    else
    {
        regs->GR_G(r1) = op2;

        return 1;
    }
}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
int ARCH_DEP(plo_cststx) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1c[16],
     op1r[16],
     op2[16],
     op3[16],
     op5[16],
     op7[16];
U32 op4alet = 0,
    op6alet = 0,
    op8alet = 0;
VADR op4addr,
    op6addr,
    op8addr;

    UNREFERENCED(r1);

    QW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    ARCH_DEP(vfetchc)(op1c, 16-1, effective_addr4 + 0, b4, regs);
    ARCH_DEP(vfetchc)(op2, 16-1, effective_addr2, b2, regs);

    if(memcmp(op1c,op2,16) == 0)
    {
        ARCH_DEP(wfetchc)(op1r, 16-1, effective_addr4 + 16, b4, regs);
        ARCH_DEP(wfetchc)(op3, 16-1, effective_addr4 + 48, b4, regs);
        ARCH_DEP(wfetchc)(op5, 16-1, effective_addr4 + 80, b4, regs);
        ARCH_DEP(wfetchc)(op7, 16-1, effective_addr4 + 112, b4, regs);

        /* Verify access to 2nd operand */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 16-1,
            ACCTYPE_WRITE_SKP, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            op6alet = ARCH_DEP(wfetch4)(effective_addr4 + 100, b4, regs);
            op8alet = ARCH_DEP(wfetch4)(effective_addr4 + 132, b4, regs);
            regs->AR(r3) = op8alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
        op4addr &= ADDRESS_MAXWRAP(regs);
        QW_CHECK(op4addr, regs);

        /* Load address of operand 6 */
        op6addr = ARCH_DEP(wfetch8)(effective_addr4 + 104, b4, regs);
        op6addr &= ADDRESS_MAXWRAP(regs);
        QW_CHECK(op6addr, regs);

        /* Load address of operand 8 */
        op8addr = ARCH_DEP(wfetch8)(effective_addr4 + 136, b4, regs);
        op8addr &= ADDRESS_MAXWRAP(regs);
        QW_CHECK(op8addr, regs);

        /* Verify access to 8th operand */
        ARCH_DEP(validate_operand) (op8addr, r3, 16-1,ACCTYPE_WRITE_SKP, regs);

        /* Verify access to 6th operand */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(validate_operand) (op6addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);

        /* Store 3th op at 4th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstorec)(op3, 16-1, op4addr, r3, regs);

        /* Store 5th op at 6th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstorec)(op5, 16-1, op6addr, r3, regs);

        /* Store 7th op at 8th op */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            regs->AR(r3) = op8alet;
            SET_AEA_AR(regs, r3);
        }
        ARCH_DEP(vstorec)(op7, 16-1, op8addr, r3, regs);

        /* Store 1st op replacement value at 2nd op */
        ARCH_DEP(vstorec)(op1r, 16-1, effective_addr2, b2, regs);

        return 0;
    }
    else
    {
        ARCH_DEP(vstorec)(op2, 16-1, effective_addr4 + 0, b4, regs);

        return 1;
    }
}
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

/* ============================================================== */
/* PLO-extension facility functions                               */
/* ============================================================== */
#if defined(FEATURE_087_PLO_EXTENSION)

/* ----------------------------------------------------- */
/* plo-extension function: 24 Compare and Load: OctoWord */
/* ----------------------------------------------------- */
int ARCH_DEP(plo_clo) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1c[32],
     op2[32],
     op4[32];
U32 op4alet = 0;
VADR op4addr;

    UNREFERENCED(r1);

    DW_CHECK(effective_addr4, regs);
    OW_CHECK(effective_addr2, regs);

    /* load second operand */
    ARCH_DEP(vfetchc) ( op2, 32-1, effective_addr2, b2, regs );

    /* load 1st op. compare value */
    ARCH_DEP(vfetchc) ( op1c, 32-1, effective_addr4 + 0, b4, regs );

    if(memcmp(op1c,op2,32) == 0)
    {
        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
        op4addr &= ADDRESS_MAXWRAP(regs);
        OW_CHECK(op4addr, regs);

        /* Load operand 4, using ar3 when in ar mode */
        ARCH_DEP(vfetchc) ( op4, 32-1, op4addr, r3, regs );

        /* replace the 3rd operand with the 4th operand */
        ARCH_DEP(wstorec) ( op4, 32-1, effective_addr4 + 32, b4, regs );

        return 0;
    }
    else
    {
        /* replace the first op compare value with 2nd op */
        ARCH_DEP(vstorec) ( op2, 32-1, effective_addr4 + 0, b4, regs );

        return 1;
    }
}

/* ----------------------------------------------------- */
/* plo-extension function: 25 Compare and Swap: OctoWord */
/* ----------------------------------------------------- */
int ARCH_DEP(plo_cso) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1c[32],
     op1r[32],
     op2[32];

    UNREFERENCED(r1);
    UNREFERENCED(r3);

    DW_CHECK(effective_addr4, regs);
    OW_CHECK(effective_addr2, regs);

    /* Load first op compare value */
    ARCH_DEP(vfetchc) ( op1c, 32-1, effective_addr4 + 0, b4, regs );

    /* Load 2nd operand */
    ARCH_DEP(vfetchc) ( op2, 32-1, effective_addr2, b2, regs );

    if(memcmp(op1c,op2,32) == 0)
    {
        /* Load 1st op replacement value */
        ARCH_DEP(wfetchc) ( op1r, 32-1, effective_addr4 + 32, b4, regs );

        /* Verify access to 2nd operand       */
        /* Store 1st op replacement at 2nd op */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 32-1,
                ACCTYPE_WRITE_SKP, regs);
        ARCH_DEP(vstorec) ( op1r, 32-1, effective_addr2, b2, regs );

        return 0;
    }
    else
    {
        /* Replace 1st op comp value by 2nd op */
        ARCH_DEP(vstorec) ( op2, 32-1, effective_addr4 + 0, b4, regs );

        return 1;
    }
}

/* ------------------------------------------------------------ */
/* plo-extension function: 26 Double Compare and Swap: OctoWord */
/* ------------------------------------------------------------ */
int ARCH_DEP(plo_dcso) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1c[32],
     op1r[32],
     op2[32],
     op3c[32],
     op3r[32],
     op4[32];
U32 op4alet = 0;
VADR op4addr;

    UNREFERENCED(r1);

    OW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    /* load 1st op compare value from the pl */
    ARCH_DEP(vfetchc) ( op1c, 32-1, effective_addr4 + 0, b4, regs );

    /* load 2nd operand */
    ARCH_DEP(vfetchc) ( op2, 32-1, effective_addr2, b2, regs );

    if(memcmp(op1c,op2,32) != 0)
    {
        /* replace the 1st op compare value with 2nd op */
        ARCH_DEP(vstorec) ( op2, 32-1, effective_addr4 + 0, b4, regs );

        return 1;
    }
    else
    {
        /* Load 3rd op compare value */
        ARCH_DEP(wfetchc) ( op3c, 32-1, effective_addr4 + 80, b4, regs );

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
        op4addr &= ADDRESS_MAXWRAP(regs);
        OW_CHECK(op4addr, regs);

        /* Load operand 4, using ar3 when in ar mode */
        ARCH_DEP(vfetchc) ( op4, 32-1, op4addr, r3, regs );

        if(memcmp(op3c,op4,32) != 0)
        {
            ARCH_DEP(wstorec) ( op4, 32-1, effective_addr4 + 80, b4, regs );

            return 2;
        }
        else
        {
            /* load replacement values */
            ARCH_DEP(wfetchc) ( op1r, 32-1, effective_addr4 +  32, b4, regs );
            ARCH_DEP(wfetchc) ( op3r, 32-1, effective_addr4 + 112, b4, regs );

            /* Verify access to 4th operand       */
            /* Store 3rd op replacement at 4th op */
            ARCH_DEP(validate_operand) (op4addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
            ARCH_DEP(vstorec) ( op3r, 32-1, op4addr, r3, regs);

            /* Verify access to 2nd operand       */
            /* Store 1st op replacement at 2nd op */
            ARCH_DEP(validate_operand) (effective_addr2, b2, 32-1,
                ACCTYPE_WRITE_SKP, regs);
            ARCH_DEP(vstorec) ( op1r, 32-1, effective_addr2, b2, regs);

            return 0;
        }
    }
}

/* --------------------------------------------------------------- */
/* plo-extension function: 27 Compare and Swap and Store: OctoWord */
/* --------------------------------------------------------------- */
int ARCH_DEP(plo_cssto) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1c[32],
     op1r[32],
     op2[32],
     op3[32];
U32 op4alet = 0;
VADR op4addr;

    UNREFERENCED(r1);

    OW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    /* load 1st op compare value from the pl */
    ARCH_DEP(vfetchc) ( op1c, 32-1, effective_addr4 + 0, b4, regs );

    /* load 2nd operand */
    ARCH_DEP(vfetchc) ( op2, 32-1, effective_addr2, b2, regs );

    if(memcmp(op1c,op2,32) != 0)
    {
        /* replace the 1st op compare value with 2nd op */
        ARCH_DEP(vstorec) ( op2, 32-1, effective_addr4 + 0, b4, regs );

        return 1;
    }
    else
    {
        ARCH_DEP(wfetchc) ( op1r, 32-1, effective_addr4 + 32, b4, regs );
        ARCH_DEP(wfetchc) ( op3,  32-1, effective_addr4 + 80, b4, regs );

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
        op4addr &= ADDRESS_MAXWRAP(regs);
        OW_CHECK(op4addr, regs);

        /* Verify access to 4th operand       */
        /* Store 3rd op replacement at 4th op */
        ARCH_DEP(validate_operand) (op4addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
        ARCH_DEP(vstorec) ( op3, 32-1, op4addr, r3, regs);

        /* Verify access to 2nd operand       */
        /* Store 1st op replacement at 2nd op */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 32-1,
            ACCTYPE_WRITE_SKP, regs);
        ARCH_DEP(vstorec) ( op1r, 32-1, effective_addr2, b2, regs);

        return 0;

    }
}

/* ---------------------------------------------------------------------- */
/* plo-extension function: 28 Compare and Swap and Double Store: OctoWord */
/* ---------------------------------------------------------------------- */
int ARCH_DEP(plo_csdsto) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1c[32],
     op1r[32],
     op2[32],
     op3[32],
     op5[32];
U32 op4alet = 0,
    op6alet = 0;
VADR op4addr,
    op6addr;

    UNREFERENCED(r1);

    OW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    ARCH_DEP(vfetchc)(op1c, 32-1, effective_addr4 + 0, b4, regs);
    ARCH_DEP(vfetchc)(op2, 32-1, effective_addr2, b2, regs);

    if(memcmp(op1c,op2,32) == 0)
    {
        ARCH_DEP(wfetchc)(op1r, 32-1, effective_addr4 + 32, b4, regs);
        ARCH_DEP(wfetchc)(op3, 32-1, effective_addr4 + 112, b4, regs);
        ARCH_DEP(wfetchc)(op5, 32-1, effective_addr4 + 144, b4, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
        op4addr &= ADDRESS_MAXWRAP(regs);
        OW_CHECK(op4addr, regs);

        /* Verify access to 4th operand */
        /* Store 3th op at 4th op       */
        ARCH_DEP(validate_operand) (op4addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
        ARCH_DEP(vstorec)(op3, 32-1, op4addr, r3, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op6alet = ARCH_DEP(wfetch4)(effective_addr4 + 100, b4, regs);
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 6 */
        op6addr = ARCH_DEP(wfetch8)(effective_addr4 + 104, b4, regs);
        op6addr &= ADDRESS_MAXWRAP(regs);
        OW_CHECK(op6addr, regs);

        /* Verify access to 6th operand */
        /* Store 5th op at 6th op       */
        ARCH_DEP(validate_operand) (op6addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
        ARCH_DEP(vstorec)(op5, 32-1, op6addr, r3, regs);

        /* Verify access to 2nd operand       */
        /* Store 1st op replacement at 2nd op */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 32-1,
            ACCTYPE_WRITE_SKP, regs);
        ARCH_DEP(vstorec)(op1r, 32-1, effective_addr2, b2, regs);

        return 0;
    }
    else
    {
        ARCH_DEP(vstorec)(op2, 32-1, effective_addr4 + 0, b4, regs);

        return 1;
    }
}

/* ---------------------------------------------------------------------- */
/* plo-extension function: 29 Compare and Swap and Triple Store: OctoWord */
/* ---------------------------------------------------------------------- */
int ARCH_DEP(plo_cststo) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1c[32],
     op1r[32],
     op2[32],
     op3[32],
     op5[32],
     op7[32];
U32 op4alet = 0,
    op6alet = 0,
    op8alet = 0;
VADR op4addr,
    op6addr,
    op8addr;

    UNREFERENCED(r1);

    OW_CHECK(effective_addr2, regs);
    DW_CHECK(effective_addr4, regs);

    ARCH_DEP(vfetchc)(op1c, 32-1, effective_addr4 + 0, b4, regs);
    ARCH_DEP(vfetchc)(op2, 32-1, effective_addr2, b2, regs);

    if(memcmp(op1c,op2,32) == 0)
    {
        ARCH_DEP(wfetchc)(op1r, 32-1, effective_addr4 + 32, b4, regs);
        ARCH_DEP(wfetchc)(op3, 32-1, effective_addr4 + 144, b4, regs);
        ARCH_DEP(wfetchc)(op5, 32-1, effective_addr4 + 176, b4, regs);
        ARCH_DEP(wfetchc)(op7, 32-1, effective_addr4 + 208, b4, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 68, b4, regs);
            regs->AR(r3) = op4alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 4 */
        op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 72, b4, regs);
        op4addr &= ADDRESS_MAXWRAP(regs);
        OW_CHECK(op4addr, regs);

        /* Verify access to 4th operand */
        /* Store 3th op at 4th op       */
        ARCH_DEP(validate_operand) (op4addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
        ARCH_DEP(vstorec)(op3, 32-1, op4addr, r3, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op6alet = ARCH_DEP(wfetch4)(effective_addr4 + 100, b4, regs);
            regs->AR(r3) = op6alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 6 */
        op6addr = ARCH_DEP(wfetch8)(effective_addr4 + 104, b4, regs);
        op6addr &= ADDRESS_MAXWRAP(regs);
        OW_CHECK(op6addr, regs);

        /* Verify access to 6th operand */
        /* Store 5th op at 6th op       */
        ARCH_DEP(validate_operand) (op6addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
        ARCH_DEP(vstorec)(op5, 32-1, op6addr, r3, regs);

        /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
        if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
        {
            if(r3 == 0)
                ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
            op8alet = ARCH_DEP(wfetch4)(effective_addr4 + 132, b4, regs);
            regs->AR(r3) = op8alet;
            SET_AEA_AR(regs, r3);
        }

        /* Load address of operand 8 */
        op8addr = ARCH_DEP(wfetch8)(effective_addr4 + 136, b4, regs);
        op8addr &= ADDRESS_MAXWRAP(regs);
        OW_CHECK(op8addr, regs);

        /* Verify access to 8th operand */
        /* Store 7th op at 8th op       */
        ARCH_DEP(validate_operand) (op8addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
        ARCH_DEP(vstorec)(op7, 32-1, op8addr, r3, regs);

        /* Verify access to 2nd operand */
        /* Store 1st op replacement value at 2nd op */
        ARCH_DEP(validate_operand) (effective_addr2, b2, 32-1,
            ACCTYPE_WRITE_SKP, regs);
        ARCH_DEP(vstorec)(op1r, 32-1, effective_addr2, b2, regs);

        return 0;
    }
    else
    {
        ARCH_DEP(vstorec)(op2, 32-1, effective_addr4 + 0, b4, regs);

        return 1;
    }
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 30 Triple Compare and Swap : Fullword      */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_tcs) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[4],
     op1c[4],
     op1r[4],
     op2[4],
     op2c[4],
     op2r[4],
     op3[4],
     op3c[4],
     op3r[4];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0;
VADR op1addr,
    op2addr,
    op3addr;
char ur[4] = {0X00, 0X01, 0X02, 0X03};

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc)(op1, 4-1, op1addr, r3, regs);
    ARCH_DEP(vfetchc)(op1c, 4-1, effective_addr4 + 64, b4, regs);

    if(memcmp(op1c,op1,4) != 0)
    {
        /* first-operand comparison value is replaced by the first operand */
        /* set Unequal Results (+48) to x'01'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op1, 4-1, effective_addr4 + 64, b4, regs );
        ARCH_DEP(wstorec) ( &ur[1], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc)(op2, 4-1, op2addr, r3, regs);
    ARCH_DEP(vfetchc)(op2c, 4-1, effective_addr4 + 72, b4, regs);

    if(memcmp(op2c,op2,4) != 0)
    {
        /* 2nd-operand comparison value is replaced by the 2nd operand     */
        /* set Unequal Results (+48) to x'02'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op2, 4-1, effective_addr4 + 72, b4, regs );
        ARCH_DEP(wstorec) ( &ur[2], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc)(op3, 4-1, op3addr, r3, regs);
    ARCH_DEP(vfetchc)(op3c, 4-1, effective_addr4 + 80, b4, regs);

    if(memcmp(op3c,op3,4) != 0)
    {
        /* 3rd-operand comparison value is replaced by the 3rd operand     */
        /* set Unequal Results (+48) to x'03'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op3, 4-1, effective_addr4 + 80, b4, regs );
        ARCH_DEP(wstorec) ( &ur[3], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* Verify access to 2nd operand                               */
    /* 2nd-operand replacement value is stored at 2nd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op2addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op2r, 4-1, effective_addr4 + 96, b4, regs );
    ARCH_DEP(wstorec) ( op2r, 4-1, op2addr, r3, regs );

    /* Verify access to 3rd operand                               */
    /* 3rd-operand replacement value is stored at 3rd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op3addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op3r, 4-1, effective_addr4 + 104, b4, regs );
    ARCH_DEP(wstorec) ( op3r, 4-1, op3addr, r3, regs );

    /* Verify access to 1st operand                               */
    /* 1st-operand replacement value is stored at 1st operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op1addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op1r, 4-1, effective_addr4 + 88, b4, regs );
    ARCH_DEP(wstorec) ( op1r, 4-1, op1addr, r3, regs );

    return 0;
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 31 Triple Compare and Swap : DoubleWord    */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_tcsg) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[8],
     op1c[8],
     op1r[8],
     op2[8],
     op2c[8],
     op2r[8],
     op3[8],
     op3c[8],
     op3r[8];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0;
VADR op1addr,
    op2addr,
    op3addr;
char ur[4] = {0X00, 0X01, 0X02, 0X03};

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc)(op1, 8-1, op1addr, r3, regs);
    ARCH_DEP(vfetchc)(op1c, 8-1, effective_addr4 + 64, b4, regs);

    if(memcmp(op1c,op1,8) != 0)
    {
        /* first-operand comparison value is replaced by the first operand */
        /* set Unequal Results (+48) to x'01'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op1, 8-1, effective_addr4 + 64, b4, regs );
        ARCH_DEP(wstorec) ( &ur[1], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc)(op2, 8-1, op2addr, r3, regs);
    ARCH_DEP(vfetchc)(op2c, 8-1, effective_addr4 + 72, b4, regs);

    if(memcmp(op2c,op2,8) != 0)
    {
        /* 2nd-operand comparison value is replaced by the 2nd operand     */
        /* set Unequal Results (+48) to x'02'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op2, 8-1, effective_addr4 + 72, b4, regs );
        ARCH_DEP(wstorec) ( &ur[2], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc)(op3, 8-1, op3addr, r3, regs);
    ARCH_DEP(vfetchc)(op3c, 8-1, effective_addr4 + 80, b4, regs);

    if(memcmp(op3c,op3,8) != 0)
    {
        /*3rd-operand comparison value is replaced by the 3rd operand     */
        /* set Unequal Results (+48) to x'03'                             */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op3, 8-1, effective_addr4 + 80, b4, regs );
        ARCH_DEP(wstorec) ( &ur[3], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* Verify access to 2nd operand                               */
    /* 2nd-operand replacement value is stored at 2nd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op2addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op2r, 8-1, effective_addr4 + 96, b4, regs );
    ARCH_DEP(wstorec) ( op2r, 8-1, op2addr, r3, regs );

    /* Verify access to 3rd operand                               */
    /* 3rd-operand replacement value is stored at 3rd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op3addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op3r, 8-1, effective_addr4 + 104, b4, regs );
    ARCH_DEP(wstorec) ( op3r, 8-1, op3addr, r3, regs );

    /* Verify access to 1st operand                               */
    /* 1st-operand replacement value is stored at 1st operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op1addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op1r, 8-1, effective_addr4 + 88, b4, regs );
    ARCH_DEP(wstorec) ( op1r, 8-1, op1addr, r3, regs );

    return 0;
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 32 Triple Compare and Swap : QuadWord      */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_tcsx) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[16],
     op1c[16],
     op1r[16],
     op2[16],
     op2c[16],
     op2r[16],
     op3[16],
     op3c[16],
     op3r[16];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0;
VADR op1addr,
    op2addr,
    op3addr;
char ur[4] = {0X00, 0X01, 0X02, 0X03};

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc)(op1, 16-1, op1addr, r3, regs);
    ARCH_DEP(vfetchc)(op1c, 16-1, effective_addr4 + 64, b4, regs);

    if(memcmp(op1c,op1,16) != 0)
    {
        /* first-operand comparison value is replaced by the first operand */
        /* set Unequal Results (+48) to x'01'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op1, 16-1, effective_addr4 + 64, b4, regs );
        ARCH_DEP(wstorec) ( &ur[1], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc)(op2, 16-1, op2addr, r3, regs);
    ARCH_DEP(vfetchc)(op2c, 16-1, effective_addr4 + 80, b4, regs);

    if(memcmp(op2c,op2,16) != 0)
    {
        /* 2nd-operand comparison value is replaced by the 2nd operand     */
        /* set Unequal Results (+48) to x'02'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op2, 16-1, effective_addr4 + 80, b4, regs );
        ARCH_DEP(wstorec) ( &ur[2], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc)(op3, 16-1, op3addr, r3, regs);
    ARCH_DEP(vfetchc)(op3c, 16-1, effective_addr4 + 96, b4, regs);

    if(memcmp(op3c,op3,16) != 0)
    {
        /* 3rd-operand comparison value is replaced by the 3rd operand     */
        /* set Unequal Results (+48) to x'03'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op3, 16-1, effective_addr4 + 96, b4, regs );
        ARCH_DEP(wstorec) ( &ur[3], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* Verify access to 2nd operand                               */
    /* 2nd-operand replacement value is stored at 2nd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op2addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op2r, 16-1, effective_addr4 + 128, b4, regs );
    ARCH_DEP(wstorec) ( op2r, 16-1, op2addr, r3, regs );

    /* Verify access to 3rd operand                               */
    /* 3rd-operand replacement value is stored at 3rd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op3addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op3r, 16-1, effective_addr4 + 144, b4, regs );
    ARCH_DEP(wstorec) ( op3r, 16-1, op3addr, r3, regs );

    /* Verify access to 1st operand                               */
    /* 1st-operand replacement value is stored at 1st operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op1addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op1r, 16-1, effective_addr4 + 112, b4, regs );
    ARCH_DEP(wstorec) ( op1r, 16-1, op1addr, r3, regs );

    return 0;
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 33 Triple Compare and Swap : OctoWord      */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_tcso) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[32],
     op1c[32],
     op1r[32],
     op2[32],
     op2c[32],
     op2r[32],
     op3[32],
     op3c[32],
     op3r[32];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0;
VADR op1addr,
    op2addr,
    op3addr;
char ur[4] = {0X00, 0X01, 0X02, 0X03};

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc)(op1, 32-1, op1addr, r3, regs);
    ARCH_DEP(vfetchc)(op1c, 32-1, effective_addr4 + 64, b4, regs);

    if(memcmp(op1c,op1,32) != 0)
    {
        /* first-operand comparison value is replaced by the first operand */
        /* set Unequal Results (+48) to x'01'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op1, 32-1, effective_addr4 + 64, b4, regs );
        ARCH_DEP(wstorec) ( &ur[1], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc)(op2, 32-1, op2addr, r3, regs);
    ARCH_DEP(vfetchc)(op2c, 32-1, effective_addr4 + 96, b4, regs);

    if(memcmp(op2c,op2,32) != 0)
    {
        /* 2nd-operand comparison value is replaced by the 2nd operand     */
        /* set Unequal Results (+48) to x'02'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op2, 32-1, effective_addr4 + 96, b4, regs );
        ARCH_DEP(wstorec) ( &ur[2], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc)(op3, 32-1, op3addr, r3, regs);
    ARCH_DEP(vfetchc)(op3c, 32-1, effective_addr4 + 128, b4, regs);

    if(memcmp(op3c,op3,32) != 0)
    {
        /* 3rd-operand comparison value is replaced by the 2rd operand     */
        /* set Unequal Results (+48) to x'03'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op3, 32-1, effective_addr4 + 128, b4, regs );
        ARCH_DEP(wstorec) ( &ur[3], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* Verify access to 2nd operand                               */
    /* 2nd-operand replacement value is stored at 2nd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op2addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op2r, 32-1, effective_addr4 + 192, b4, regs );
    ARCH_DEP(wstorec) ( op2r, 32-1, op2addr, r3, regs );

    /* Verify access to 3rd operand                               */
    /* 3rd-operand replacement value is stored at 3rd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op3addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op3r, 32-1, effective_addr4 + 224, b4, regs );
    ARCH_DEP(wstorec) ( op3r, 32-1, op3addr, r3, regs );

    /* Verify access to 1st operand                               */
    /* 1st-operand replacement value is stored at 1st operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op1addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op1r, 32-1, effective_addr4 + 160, b4, regs );
    ARCH_DEP(wstorec) ( op1r, 32-1, op1addr, r3, regs );

    return 0;
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 34 Quadruple Compare and Swap : Fullword   */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_qcs) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[4],
     op1c[4],
     op1r[4],
     op2[4],
     op2c[4],
     op2r[4],
     op3[4],
     op3c[4],
     op3r[4],
     op4[4],
     op4c[4],
     op4r[4];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0,
    op4alet = 0;
VADR op1addr,
    op2addr,
    op3addr,
    op4addr;
char ur[5] = {0X00, 0X01, 0X02, 0X03, 0X04};

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc)(op1, 4-1, op1addr, r3, regs);
    ARCH_DEP(vfetchc)(op1c, 4-1, effective_addr4 + 64, b4, regs);

    if(memcmp(op1c,op1,4) != 0)
    {
        /* first-operand comparison value is replaced by the first operand */
        /* set Unequal Results (+48) to x'01'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op1, 4-1, effective_addr4 + 64, b4, regs );
        ARCH_DEP(wstorec) ( &ur[1], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc)(op2, 4-1, op2addr, r3, regs);
    ARCH_DEP(vfetchc)(op2c, 4-1, effective_addr4 + 72, b4, regs);

    if(memcmp(op2c,op2,4) != 0)
    {
        /* 2nd-operand comparison value is replaced by the 2nd operand     */
        /* set Unequal Results (+48) to x'02'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op2, 4-1, effective_addr4 + 72, b4, regs );
        ARCH_DEP(wstorec) ( &ur[2], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc)(op3, 4-1, op3addr, r3, regs);
    ARCH_DEP(vfetchc)(op3c, 4-1, effective_addr4 + 80, b4, regs);

    if(memcmp(op3c,op3,4) != 0)
    {
        /* 3rd-operand comparison value is replaced by the 3rd operand     */
        /* set Unequal Results (+48) to x'03'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op3, 4-1, effective_addr4 + 80, b4, regs );
        ARCH_DEP(wstorec) ( &ur[3], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 44, b4, regs);
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 4 */
    op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
    op4addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op4addr, regs);

    ARCH_DEP(vfetchc)(op4, 4-1, op4addr, r3, regs);
    ARCH_DEP(vfetchc)(op4c, 4-1, effective_addr4 + 88, b4, regs);

    if(memcmp(op4c,op4,4) != 0)
    {
        /* 4th-operand comparison value is replaced by the 4th operand     */
        /* set Unequal Results (+48) to x'04'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op4, 4-1, effective_addr4 + 88, b4, regs );
        ARCH_DEP(wstorec) ( &ur[4], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* Verify access to 2nd operand                               */
    /* 2nd-operand replacement value is stored at 2nd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op2addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op2r, 4-1, effective_addr4 + 104, b4, regs );
    ARCH_DEP(wstorec) ( op2r, 4-1, op2addr, r3, regs );

    /* Verify access to 3rd operand                               */
    /* 3rd-operand replacement value is stored at 3rd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op3addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op3r, 4-1, effective_addr4 + 112, b4, regs );
    ARCH_DEP(wstorec) ( op3r, 4-1, op3addr, r3, regs );

    /* Verify access to 4th operand                               */
    /* 4th-operand replacement value is stored at 4th operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op4addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op4r, 4-1, effective_addr4 + 120, b4, regs );
    ARCH_DEP(wstorec) ( op4r, 4-1, op4addr, r3, regs );

    /* Verify access to 1st operand                               */
    /* 1st-operand replacement value is stored at 1st operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op1addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op1r, 4-1, effective_addr4 + 96, b4, regs );
    ARCH_DEP(wstorec) ( op1r, 4-1, op1addr, r3, regs );

    return 0;
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 35 Quadruple Compare and Swap : Doubleword */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_qcsg) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[8],
     op1c[8],
     op1r[8],
     op2[8],
     op2c[8],
     op2r[8],
     op3[8],
     op3c[8],
     op3r[8],
     op4[8],
     op4c[8],
     op4r[8];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0,
    op4alet = 0;
VADR op1addr,
    op2addr,
    op3addr,
    op4addr;
char ur[5] = {0X00, 0X01, 0X02, 0X03, 0X04};

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc)(op1, 8-1, op1addr, r3, regs);
    ARCH_DEP(vfetchc)(op1c, 8-1, effective_addr4 + 64, b4, regs);

    if(memcmp(op1c,op1,8) != 0)
    {
        /* first-operand comparison value is replaced by the first operand */
        /* set Unequal Results (+48) to x'01'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op1, 8-1, effective_addr4 + 64, b4, regs );
        ARCH_DEP(wstorec) ( &ur[1], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc)(op2, 8-1, op2addr, r3, regs);
    ARCH_DEP(vfetchc)(op2c, 8-1, effective_addr4 + 72, b4, regs);

    if(memcmp(op2c,op2,8) != 0)
    {
        /* 2nd-operand comparison value is replaced by the 2nd operand     */
        /* set Unequal Results (+48) to x'02'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op2, 8-1, effective_addr4 + 72, b4, regs );
        ARCH_DEP(wstorec) ( &ur[2], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc)(op3, 8-1, op3addr, r3, regs);
    ARCH_DEP(vfetchc)(op3c, 8-1, effective_addr4 + 80, b4, regs);

    if(memcmp(op3c,op3,8) != 0)
    {
        /* 3rd-operand comparison value is replaced by the 3rd operand     */
        /* set Unequal Results (+48) to x'03'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op3, 8-1, effective_addr4 + 80, b4, regs );
        ARCH_DEP(wstorec) ( &ur[3], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 44, b4, regs);
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 4 */
    op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
    op4addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op4addr, regs);

    ARCH_DEP(vfetchc)(op4, 8-1, op4addr, r3, regs);
    ARCH_DEP(vfetchc)(op4c, 8-1, effective_addr4 + 88, b4, regs);

    if(memcmp(op4c,op4,8) != 0)
    {
        /* 4th-operand comparison value is replaced by the 4th operand     */
        /* set Unequal Results (+48) to x'04'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op4, 8-1, effective_addr4 + 88, b4, regs );
        ARCH_DEP(wstorec) ( &ur[4], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* Verify access to 2nd operand                               */
    /* 2nd-operand replacement value is stored at 2nd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op2addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op2r, 8-1, effective_addr4 + 104, b4, regs );
    ARCH_DEP(wstorec) ( op2r, 8-1, op2addr, r3, regs );

    /* Verify access to 3rd operand                               */
    /* 3rd-operand replacement value is stored at 3rd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op3addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op3r, 8-1, effective_addr4 + 112, b4, regs );
    ARCH_DEP(wstorec) ( op3r, 8-1, op3addr, r3, regs );

    /* Verify access to 4th operand                               */
    /* 4th-operand replacement value is stored at 4th operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op4addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op4r, 8-1, effective_addr4 + 120, b4, regs );
    ARCH_DEP(wstorec) ( op4r, 8-1, op4addr, r3, regs );

    /* Verify access to 1st operand                               */
    /* 1st-operand replacement value is stored at 1st operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op1addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op1r, 8-1, effective_addr4 + 96, b4, regs );
    ARCH_DEP(wstorec) ( op1r, 8-1, op1addr, r3, regs );

    return 0;
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 36 Quadruple Compare and Swap : Quadword   */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_qcsx) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[16],
     op1c[16],
     op1r[16],
     op2[16],
     op2c[16],
     op2r[16],
     op3[16],
     op3c[16],
     op3r[16],
     op4[16],
     op4c[16],
     op4r[16];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0,
    op4alet = 0;
VADR op1addr,
    op2addr,
    op3addr,
    op4addr;
char ur[5] = {0X00, 0X01, 0X02, 0X03, 0X04};

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc)(op1, 16-1, op1addr, r3, regs);
    ARCH_DEP(vfetchc)(op1c, 16-1, effective_addr4 + 64, b4, regs);

    if(memcmp(op1c,op1,16) != 0)
    {
        /* first-operand comparison value is replaced by the first operand */
        /* set Unequal Results (+48) to x'01'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op1, 16-1, effective_addr4 + 64, b4, regs );
        ARCH_DEP(wstorec) ( &ur[1], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc)(op2, 16-1, op2addr, r3, regs);
    ARCH_DEP(vfetchc)(op2c, 16-1, effective_addr4 + 80, b4, regs);

    if(memcmp(op2c,op2,16) != 0)
    {
        /* 2nd-operand comparison value is replaced by the 2nd operand     */
        /* set Unequal Results (+48) to x'02'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op2, 16-1, effective_addr4 + 80, b4, regs );
        ARCH_DEP(wstorec) ( &ur[2], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc)(op3, 16-1, op3addr, r3, regs);
    ARCH_DEP(vfetchc)(op3c, 16-1, effective_addr4 + 96, b4, regs);

    if(memcmp(op3c,op3,16) != 0)
    {
        /* 3rd-operand comparison value is replaced by the 3rd operand     */
        /* set Unequal Results (+48) to x'03'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op3, 16-1, effective_addr4 + 96, b4, regs );
        ARCH_DEP(wstorec) ( &ur[3], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 44, b4, regs);
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 4 */
    op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
    op4addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op4addr, regs);

    ARCH_DEP(vfetchc)(op4, 16-1, op4addr, r3, regs);
    ARCH_DEP(vfetchc)(op4c, 16-1, effective_addr4 + 112, b4, regs);

    if(memcmp(op4c,op4,16) != 0)
    {
        /* 4th-operand comparison value is replaced by the 4th operand     */
        /* set Unequal Results (+48) to x'04'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op4, 16-1, effective_addr4 + 112, b4, regs );
        ARCH_DEP(wstorec) ( &ur[4], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* Verify access to 2nd operand                               */
    /* 2nd-operand replacement value is stored at 2nd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op2addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op2r, 16-1, effective_addr4 + 144, b4, regs );
    ARCH_DEP(wstorec) ( op2r, 16-1, op2addr, r3, regs );

    /* Verify access to 3rd operand                               */
    /* 3rd-operand replacement value is stored at 3rd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op3addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op3r, 16-1, effective_addr4 + 160, b4, regs );
    ARCH_DEP(wstorec) ( op3r, 16-1, op3addr, r3, regs );

    /* Verify access to 4th operand                               */
    /* 4th-operand replacement value is stored at 4th operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op4addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op4r, 16-1, effective_addr4 + 176, b4, regs );
    ARCH_DEP(wstorec) ( op4r, 16-1, op4addr, r3, regs );

    /* Verify access to 1st operand                               */
    /* 1st-operand replacement value is stored at 1st operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op1addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op1r, 16-1, effective_addr4 + 128, b4, regs );
    ARCH_DEP(wstorec) ( op1r, 16-1, op1addr, r3, regs );

    return 0;
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 37 Quadruple Compare and Swap : Octoword   */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_qcso) (int r1, int r3, VADR effective_addr2, int b2,
                              VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[32],
     op1c[32],
     op1r[32],
     op2[32],
     op2c[32],
     op2r[32],
     op3[32],
     op3c[32],
     op3r[32],
     op4[32],
     op4c[32],
     op4r[32];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0,
    op4alet = 0;
VADR op1addr,
    op2addr,
    op3addr,
    op4addr;
char ur[5] = {0X00, 0X01, 0X02, 0X03, 0X04};

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc)(op1, 32-1, op1addr, r3, regs);
    ARCH_DEP(vfetchc)(op1c, 32-1, effective_addr4 + 64, b4, regs);

    if(memcmp(op1c,op1,32) != 0)
    {
        /* first-operand comparison value is replaced by the first operand */
        /* set Unequal Results (+48) to x'01'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op1, 32-1, effective_addr4 + 64, b4, regs );
        ARCH_DEP(wstorec) ( &ur[1], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc)(op2, 32-1, op2addr, r3, regs);
    ARCH_DEP(vfetchc)(op2c, 32-1, effective_addr4 + 96, b4, regs);

    if(memcmp(op2c,op2,32) != 0)
    {
        /* 2nd-operand comparison value is replaced by the 2nd operand     */
        /* set Unequal Results (+48) to x'02'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op2, 32-1, effective_addr4 + 96, b4, regs );
        ARCH_DEP(wstorec) ( &ur[2], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc)(op3, 32-1, op3addr, r3, regs);
    ARCH_DEP(vfetchc)(op3c, 32-1, effective_addr4 + 128, b4, regs);

    if(memcmp(op3c,op3,32) != 0)
    {
        /* 3rd-operand comparison value is replaced by the 3rd operand     */
        /* set Unequal Results (+48) to x'03'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op3, 32-1, effective_addr4 + 128, b4, regs );
        ARCH_DEP(wstorec) ( &ur[3], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 44, b4, regs);
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 4 */
    op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
    op4addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op4addr, regs);

    ARCH_DEP(vfetchc)(op4, 32-1, op4addr, r3, regs);
    ARCH_DEP(vfetchc)(op4c, 32-1, effective_addr4 + 160, b4, regs);

    if(memcmp(op4c,op4,32) != 0)
    {
        /* 4th-operand comparison value is replaced by the 4th operand     */
        /* set Unequal Results (+48) to x'04'                              */
        /* cc = 2
                                                                  */
        ARCH_DEP(wstorec) ( op4, 32-1, effective_addr4 + 160, b4, regs );
        ARCH_DEP(wstorec) ( &ur[4], 1-1, effective_addr4 + 48, b4, regs );
        return 2;
    }

    /* Verify access to 2nd operand                               */
    /* 2nd-operand replacement value is stored at 2nd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op2addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op2r, 32-1, effective_addr4 + 224, b4, regs );
    ARCH_DEP(wstorec) ( op2r, 32-1, op2addr, r3, regs );

    /* Verify access to 3rd operand                               */
    /* 3rd-operand replacement value is stored at 3rd operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op3addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op3r, 32-1, effective_addr4 + 256, b4, regs );
    ARCH_DEP(wstorec) ( op3r, 32-1, op3addr, r3, regs );

    /* Verify access to 4th operand                               */
    /* 4th-operand replacement value is stored at 4th operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op4addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op4r, 32-1, effective_addr4 + 288, b4, regs );
    ARCH_DEP(wstorec) ( op4r, 32-1, op4addr, r3, regs );

    /* Verify access to 1st operand                               */
    /* 1st-operand replacement value is stored at 1st operand     */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }
    ARCH_DEP(validate_operand) (op1addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) ( op1r, 32-1, effective_addr4 + 192, b4, regs );
    ARCH_DEP(wstorec) ( op1r, 32-1, op1addr, r3, regs );

    return 0;
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 38 Load : Octoword                         */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_lo) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[32];

U32 op1alet = 0;
VADR op1addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc) (op1, 32-1, op1addr, r3, regs);
    ARCH_DEP(wstorec) (op1, 32-1, effective_addr4 + 64, b4, regs );

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 39 Double Load : Quadword                  */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_dlx) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[16],
     op2[16];

U32 op1alet = 0,
    op2alet = 0;

VADR op1addr,
     op2addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc) (op1, 16-1, op1addr, r3, regs);
    ARCH_DEP(wstorec) (op1, 16-1, effective_addr4 + 64, b4, regs );

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc) (op2, 16-1, op2addr, r3, regs);
    ARCH_DEP(wstorec) (op2, 16-1, effective_addr4 +80, b4, regs );

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 40 Double Load : Octoword                  */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_dlo) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[32],
     op2[32];

U32 op1alet = 0,
    op2alet = 0;

VADR op1addr,
     op2addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc) (op1, 32-1, op1addr, r3, regs);
    ARCH_DEP(wstorec) (op1, 32-1, effective_addr4 + 64, b4, regs );

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc) (op2, 32-1, op2addr, r3, regs);
    ARCH_DEP(wstorec) (op2, 32-1, effective_addr4 + 96, b4, regs );

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 41 Triple Load : Fullword                  */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_tl) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[4],
     op2[4],
     op3[4];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0;

VADR op1addr,
     op2addr,
     op3addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc) (op1, 4-1, op1addr, r3, regs);
    ARCH_DEP(wstorec) (op1, 4-1, effective_addr4 + 64, b4, regs );

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc) (op2, 4-1, op2addr, r3, regs);
    ARCH_DEP(wstorec) (op2, 4-1, effective_addr4 + 72, b4, regs );

    /* When in ar mode, ar3 is used to access the
        operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc) (op3, 4-1, op3addr, r3, regs);
    ARCH_DEP(wstorec) (op3, 4-1, effective_addr4 + 80, b4, regs );

    return 0;   /* note: this function does NOT set cc */
}
/* ------------------------------------------------------------------ */
/* plo-extension function: 42 Triple Load : Doubleword                */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_tlg) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[8],
     op2[8],
     op3[8];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0;

VADR op1addr,
     op2addr,
     op3addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc) (op1, 8-1, op1addr, r3, regs);
    ARCH_DEP(wstorec) (op1, 8-1, effective_addr4 + 64, b4, regs );

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc) (op2, 8-1, op2addr, r3, regs);
    ARCH_DEP(wstorec) (op2, 8-1, effective_addr4 + 72, b4, regs );

    /* When in ar mode, ar3 is used to access the
        operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc) (op3, 8-1, op3addr, r3, regs);
    ARCH_DEP(wstorec) (op3, 8-1, effective_addr4 + 80, b4, regs );

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 43 Triple Load : Quadword                  */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_tlx) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[16],
     op2[16],
     op3[16];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0;

VADR op1addr,
     op2addr,
     op3addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc) (op1, 16-1, op1addr, r3, regs);
    ARCH_DEP(wstorec) (op1, 16-1, effective_addr4 + 64, b4, regs );

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc) (op2, 16-1, op2addr, r3, regs);
    ARCH_DEP(wstorec) (op2, 16-1, effective_addr4 + 80, b4, regs );

    /* When in ar mode, ar3 is used to access the
        operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc) (op3, 16-1, op3addr, r3, regs);
    ARCH_DEP(wstorec) (op3, 16-1, effective_addr4 + 96, b4, regs );

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 44 Triple Load : Octoword                  */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_tlo) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[32],
     op2[32],
     op3[32];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0;

VADR op1addr,
     op2addr,
     op3addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc) (op1, 32-1, op1addr, r3, regs);
    ARCH_DEP(wstorec) (op1, 32-1, effective_addr4 + 64, b4, regs );

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc) (op2, 32-1, op2addr, r3, regs);
    ARCH_DEP(wstorec) (op2, 32-1, effective_addr4 + 96, b4, regs );

    /* When in ar mode, ar3 is used to access the
        operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc) (op3, 32-1, op3addr, r3, regs);
    ARCH_DEP(wstorec) (op3, 32-1, effective_addr4 + 128, b4, regs );

    return 0;   /* note: this function does NOT set cc */
}


/* ------------------------------------------------------------------ */
/* plo-extension function: 45 Quadruple Load : Fullword               */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_ql) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[4],
     op2[4],
     op3[4],
     op4[4];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0,
    op4alet = 0;

VADR op1addr,
     op2addr,
     op3addr,
     op4addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc) (op1, 4-1, op1addr, r3, regs);
    ARCH_DEP(wstorec) (op1, 4-1, effective_addr4 + 64, b4, regs );

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc) (op2, 4-1, op2addr, r3, regs);
    ARCH_DEP(wstorec) (op2, 4-1, effective_addr4 + 72, b4, regs );

    /* When in ar mode, ar3 is used to access the
        operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc) (op3, 4-1, op3addr, r3, regs);
    ARCH_DEP(wstorec) (op3, 4-1, effective_addr4 + 80, b4, regs );

    /* When in ar mode, ar3 is used to access the
        operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 44, b4, regs);
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 4 */
    op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
    op4addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op4addr, regs);

    ARCH_DEP(vfetchc) (op4, 4-1, op4addr, r3, regs);
    ARCH_DEP(wstorec) (op4, 4-1, effective_addr4 + 88, b4, regs );

    return 0;   /* note: this function does NOT set cc */
}
/* ------------------------------------------------------------------ */
/* plo-extension function: 46 Quadruple Load : Doubleword             */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_qlg) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[8],
     op2[8],
     op3[8],
     op4[8];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0,
    op4alet = 0;

VADR op1addr,
     op2addr,
     op3addr,
     op4addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc) (op1, 8-1, op1addr, r3, regs);
    ARCH_DEP(wstorec) (op1, 8-1, effective_addr4 + 64, b4, regs );

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc) (op2, 8-1, op2addr, r3, regs);
    ARCH_DEP(wstorec) (op2, 8-1, effective_addr4 + 72, b4, regs );

    /* When in ar mode, ar3 is used to access the
        operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc) (op3, 8-1, op3addr, r3, regs);
    ARCH_DEP(wstorec) (op3, 8-1, effective_addr4 + 80, b4, regs );

    /* When in ar mode, ar3 is used to access the
        operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 44, b4, regs);
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 4 */
    op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
    op4addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op4addr, regs);

    ARCH_DEP(vfetchc) (op4, 8-1, op4addr, r3, regs);
    ARCH_DEP(wstorec) (op4, 8-1, effective_addr4 + 88, b4, regs );

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 47 Quadruple Load : Quadword               */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_qlx) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[16],
     op2[16],
     op3[16],
     op4[16];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0,
    op4alet = 0;

VADR op1addr,
     op2addr,
     op3addr,
     op4addr;;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc) (op1, 16-1, op1addr, r3, regs);
    ARCH_DEP(wstorec) (op1, 16-1, effective_addr4 + 64, b4, regs );

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc) (op2, 16-1, op2addr, r3, regs);
    ARCH_DEP(wstorec) (op2, 16-1, effective_addr4 + 80, b4, regs );

    /* When in ar mode, ar3 is used to access the
        operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc) (op3, 16-1, op3addr, r3, regs);
    ARCH_DEP(wstorec) (op3, 16-1, effective_addr4 + 96, b4, regs );

    /* When in ar mode, ar3 is used to access the
        operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 44, b4, regs);
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
    op4addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op4addr, regs);

    ARCH_DEP(vfetchc) (op4, 16-1, op4addr, r3, regs);
    ARCH_DEP(wstorec) (op4, 16-1, effective_addr4 + 112, b4, regs );

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 48 Quadruple Load : Octoword               */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_qlo) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1[32],
     op2[32],
     op3[32],
     op4[32];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0,
    op4alet = 0;

VADR op1addr,
     op2addr,
     op3addr,
     op4addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op1addr, regs);

    ARCH_DEP(vfetchc) (op1, 32-1, op1addr, r3, regs);
    ARCH_DEP(wstorec) (op1, 32-1, effective_addr4 + 64, b4, regs );

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op2addr, regs);

    ARCH_DEP(vfetchc) (op2, 32-1, op2addr, r3, regs);
    ARCH_DEP(wstorec) (op2, 32-1, effective_addr4 + 96, b4, regs );

    /* When in ar mode, ar3 is used to access the
        operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op3addr, regs);

    ARCH_DEP(vfetchc) (op3, 32-1, op3addr, r3, regs);
    ARCH_DEP(wstorec) (op3, 32-1, effective_addr4 + 128, b4, regs );

    /* When in ar mode, ar3 is used to access the
        operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 44, b4, regs);
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
    op4addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op4addr, regs);

    ARCH_DEP(vfetchc) (op4, 32-1, op4addr, r3, regs);
    ARCH_DEP(wstorec) (op4, 32-1, effective_addr4 + 160, b4, regs );

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 48 Store : Octoword                        */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_sto) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1d[32];

U32 op1alet = 0;
VADR op1addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op1addr, regs);

    /* Verify access to 1st operand                        */
    /* 1st-operand data value is stored at 1st operand     */
    ARCH_DEP(validate_operand) (op1addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op1d, 32-1, effective_addr4 + 64, b4, regs );
    ARCH_DEP(wstorec) (op1d, 32-1, op1addr, r3, regs);

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 50 Double Store : Fullword                 */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_dst) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1d[4],
     op2d[4];

U32 op1alet = 0,
    op2alet = 0;

VADR op1addr,
     op2addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op1addr, regs);

    /* Verify access to 1st operand                        */
    /* 1st-operand data value is stored at 1st operand     */
    ARCH_DEP(validate_operand) (op1addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op1d, 4-1, effective_addr4 + 64, b4, regs );
    ARCH_DEP(wstorec) (op1d, 4-1, op1addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op2addr, regs);

    /* Verify access to 2nd operand                        */
    /* 2nd-operand data value is stored at 2nd operand     */
    ARCH_DEP(validate_operand) (op2addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op2d, 4-1, effective_addr4 + 72, b4, regs );
    ARCH_DEP(wstorec) (op2d, 4-1, op2addr, r3, regs);

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 51 Double Store: Doubleword                */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_dstg) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1d[8],
     op2d[8];

U32 op1alet = 0,
    op2alet = 0;

VADR op1addr,
     op2addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op1addr, regs);

    /* Verify access to 1st operand                        */
    /* 1st-operand data value is stored at 1st operand     */
    ARCH_DEP(validate_operand) (op1addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op1d, 8-1, effective_addr4 + 64, b4, regs );
    ARCH_DEP(wstorec) (op1d, 8-1, op1addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op2addr, regs);

    /* Verify access to 2nd operand                        */
    /* 2nd-operand data value is stored at 2nd operand     */
    ARCH_DEP(validate_operand) (op2addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op2d, 8-1, effective_addr4 + 72, b4, regs );
    ARCH_DEP(wstorec) (op2d, 8-1, op2addr, r3, regs);

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 52 Double Store : Quadword                 */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_dstx) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1d[16],
     op2d[16];

U32 op1alet = 0,
    op2alet = 0;

VADR op1addr,
     op2addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op1addr, regs);

    /* Verify access to 1st operand                        */
    /* 1st-operand data value is stored at 1st operand     */
    ARCH_DEP(validate_operand) (op1addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op1d, 16-1, effective_addr4 + 64, b4, regs );
    ARCH_DEP(wstorec) (op1d, 16-1, op1addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op2addr, regs);

    /* Verify access to 2nd operand                        */
    /* 2nd-operand data value is stored at 2nd operand     */
    ARCH_DEP(validate_operand) (op2addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op2d, 16-1, effective_addr4 + 80, b4, regs );
    ARCH_DEP(wstorec) (op2d, 16-1, op2addr, r3, regs);

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 53 Double Store : Octoword                 */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_dsto) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1d[32],
     op2d[32];

U32 op1alet = 0,
    op2alet = 0;

VADR op1addr,
     op2addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op1addr, regs);

    /* Verify access to 1st operand                        */
    /* 1st-operand data value is stored at 1st operand     */
    ARCH_DEP(validate_operand) (op1addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op1d, 32-1, effective_addr4 + 64, b4, regs );
    ARCH_DEP(wstorec) (op1d, 32-1, op1addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op2addr, regs);

    /* Verify access to 2nd operand                        */
    /* 2nd-operand data value is stored at 2nd operand     */
    ARCH_DEP(validate_operand) (op2addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op2d, 32-1, effective_addr4 + 96, b4, regs );
    ARCH_DEP(wstorec) (op2d, 32-1, op2addr, r3, regs);

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 54 Triple Store : Fullword                 */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_tst) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1d[4],
     op2d[4],
     op3d[4];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0;

VADR op1addr,
     op2addr,
     op3addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op1addr, regs);

    /* Verify access to 1st operand                        */
    /* 1st-operand data value is stored at 1st operand     */
    ARCH_DEP(validate_operand) (op1addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op1d, 4-1, effective_addr4 + 64, b4, regs );
    ARCH_DEP(wstorec) (op1d, 4-1, op1addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op2addr, regs);

    /* Verify access to 2nd operand                        */
    /* 2nd-operand data value is stored at 2nd operand     */
    ARCH_DEP(validate_operand) (op2addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op2d, 4-1, effective_addr4 + 72, b4, regs );
    ARCH_DEP(wstorec) (op2d, 4-1, op2addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op3addr, regs);

    /* Verify access to 3rd operand                        */
    /* 3rd-operand data value is stored at 3rd operand     */
    ARCH_DEP(validate_operand) (op3addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op3d, 4-1, effective_addr4 + 80, b4, regs );
    ARCH_DEP(wstorec) (op3d, 4-1, op3addr, r3, regs);

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 55 Triple Store: Doubleword                */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_tstg) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1d[8],
     op2d[8],
     op3d[8];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0;

VADR op1addr,
     op2addr,
     op3addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op1addr, regs);

    /* Verify access to 1st operand                        */
    /* 1st-operand data value is stored at 1st operand     */
    ARCH_DEP(validate_operand) (op1addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op1d, 8-1, effective_addr4 + 64, b4, regs );
    ARCH_DEP(wstorec) (op1d, 8-1, op1addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op2addr, regs);

    /* Verify access to 2nd operand                        */
    /* 2nd-operand data value is stored at 2nd operand     */
    ARCH_DEP(validate_operand) (op2addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op2d, 8-1, effective_addr4 + 72, b4, regs );
    ARCH_DEP(wstorec) (op2d, 8-1, op2addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op3addr, regs);

    /* Verify access to 3rd operand                               */
    /* 3rd-operand replacement value is stored at 3rd operand     */
    ARCH_DEP(validate_operand) (op3addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op3d, 8-1, effective_addr4 + 80, b4, regs );
    ARCH_DEP(wstorec) (op3d, 8-1, op3addr, r3, regs);

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 56 Triple Store : Quadword                 */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_tstx) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1d[16],
     op2d[16],
     op3d[16];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0;

VADR op1addr,
     op2addr,
     op3addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op1addr, regs);

    /* Verify access to 1st operand                        */
    /* 1st-operand data value is stored at 1st operand     */
    ARCH_DEP(validate_operand) (op1addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op1d, 16-1, effective_addr4 + 64, b4, regs );
    ARCH_DEP(wstorec) (op1d, 16-1, op1addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op2addr, regs);

    /* Verify access to 2nd operand                        */
    /* 2nd-operand data value is stored at 2nd operand     */
    ARCH_DEP(validate_operand) (op2addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op2d, 16-1, effective_addr4 + 80, b4, regs );
    ARCH_DEP(wstorec) (op2d, 16-1, op2addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op3addr, regs);

    /* Verify access to 3rd operand                        */
    /* 3rd-operand data value is stored at 3rd operand     */
    ARCH_DEP(validate_operand) (op3addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op3d, 16-1, effective_addr4 + 96, b4, regs );
    ARCH_DEP(wstorec) (op3d, 16-1, op3addr, r3, regs);

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 57 Triple Store : Octoword                 */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_tsto) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1d[32],
     op2d[32],
     op3d[32];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0;

VADR op1addr,
     op2addr,
     op3addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op1addr, regs);

    /* Verify access to 1st operand                        */
    /* 1st-operand data value is stored at 1st operand     */
    ARCH_DEP(validate_operand) (op1addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op1d, 32-1, effective_addr4 + 64, b4, regs );
    ARCH_DEP(wstorec) (op1d, 32-1, op1addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op2addr, regs);

    /* Verify access to 2nd operand                        */
    /* 2nd-operand data value is stored at 2nd operand     */
    ARCH_DEP(validate_operand) (op2addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op2d, 32-1, effective_addr4 + 96, b4, regs );
    ARCH_DEP(wstorec) (op2d, 32-1, op2addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op3addr, regs);

    /* Verify access to 3rd operand                        */
    /* 3rd-operand data value is stored at 3rd operand     */
    ARCH_DEP(validate_operand) (op3addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op3d, 32-1, effective_addr4 + 128, b4, regs );
    ARCH_DEP(wstorec) (op3d, 32-1, op3addr, r3, regs);

    return 0;   /* note: this function does NOT set cc */
}


/* ------------------------------------------------------------------ */
/* plo-extension function: 58 Quadruple Store : Fullword              */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_qst) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1d[4],
     op2d[4],
     op3d[4],
     op4d[4];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0,
    op4alet = 0;

VADR op1addr,
     op2addr,
     op3addr,
     op4addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op1addr, regs);

    /* Verify access to 1st operand                        */
    /* 1st-operand data value is stored at 1st operand     */
    ARCH_DEP(validate_operand) (op1addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op1d, 4-1, effective_addr4 + 64, b4, regs );
    ARCH_DEP(wstorec) (op1d, 4-1, op1addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op2addr, regs);

    /* Verify access to 2nd operand                        */
    /* 2nd-operand data value is stored at 2nd operand     */
    ARCH_DEP(validate_operand) (op2addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op2d, 4-1, effective_addr4 + 72, b4, regs );
    ARCH_DEP(wstorec) (op2d, 4-1, op2addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op3addr, regs);

    /* Verify access to 3rd operand                        */
    /* 3rd-operand data value is stored at 3rd operand     */
    ARCH_DEP(validate_operand) (op3addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op3d, 4-1, effective_addr4 + 80, b4, regs );
    ARCH_DEP(wstorec) (op3d, 4-1, op3addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 44, b4, regs);
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 4 */
    op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
    op4addr &= ADDRESS_MAXWRAP(regs);
    FW_CHECK(op4addr, regs);

    /* Verify access to 4th operand                        */
    /* 4th-operand data value is stored at 4th operand     */
    ARCH_DEP(validate_operand) (op4addr, r3, 4-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op4d, 4-1, effective_addr4 + 88, b4, regs );
    ARCH_DEP(wstorec) (op4d, 4-1, op4addr, r3, regs);

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 59 Quadruple Store: Doubleword             */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_qstg) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1d[8],
     op2d[8],
     op3d[8],
     op4d[8];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0,
    op4alet = 0;

VADR op1addr,
     op2addr,
     op3addr,
     op4addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op1addr, regs);

    /* Verify access to 1st operand                        */
    /* 1st-operand data value is stored at 1st operand     */
    ARCH_DEP(validate_operand) (op1addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op1d, 8-1, effective_addr4 + 64, b4, regs );
    ARCH_DEP(wstorec) (op1d, 8-1, op1addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op2addr, regs);

    /* Verify access to 2nd operand                        */
    /* 2nd-operand data value is stored at 2nd operand     */
    ARCH_DEP(validate_operand) (op2addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op2d, 8-1, effective_addr4 + 72, b4, regs );
    ARCH_DEP(wstorec) (op2d, 8-1, op2addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op3addr, regs);

    /* Verify access to 3rd operand                        */
    /* 3rd-operand data value is stored at 3rd operand     */
    ARCH_DEP(validate_operand) (op3addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op3d, 8-1, effective_addr4 + 80, b4, regs );
    ARCH_DEP(wstorec) (op3d, 8-1, op3addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 44, b4, regs);
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 4 */
    op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
    op4addr &= ADDRESS_MAXWRAP(regs);
    DW_CHECK(op4addr, regs);

    /* Verify access to 4th operand                        */
    /* 4th-operand data value is stored at 4th operand     */
    ARCH_DEP(validate_operand) (op4addr, r3, 8-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op4d, 8-1, effective_addr4 + 88, b4, regs );
    ARCH_DEP(wstorec) (op4d, 8-1, op4addr, r3, regs);

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 60 Quadruple Store : Quadword              */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_qstx) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1d[16],
     op2d[16],
     op3d[16],
     op4d[16];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0,
    op4alet = 0;

VADR op1addr,
     op2addr,
     op3addr,
     op4addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op1addr, regs);

    /* Verify access to 1st operand                        */
    /* 1st-operand data value is stored at 1st operand     */
    ARCH_DEP(validate_operand) (op1addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op1d, 16-1, effective_addr4 + 64, b4, regs );
    ARCH_DEP(wstorec) (op1d, 16-1, op1addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op2addr, regs);

    /* Verify access to 2nd operand                        */
    /* 2nd-operand data value is stored at 2nd operand     */
    ARCH_DEP(validate_operand) (op2addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op2d, 16-1, effective_addr4 + 80, b4, regs );
    ARCH_DEP(wstorec) (op2d, 16-1, op2addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op3addr, regs);

    /* Verify access to 3rd operand                        */
    /* 3rd-operand data value is stored at 3rd operand     */
    ARCH_DEP(validate_operand) (op3addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op3d, 16-1, effective_addr4 + 96, b4, regs );
    ARCH_DEP(wstorec) (op3d, 16-1, op3addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 44, b4, regs);
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 4 */
    op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
    op4addr &= ADDRESS_MAXWRAP(regs);
    QW_CHECK(op4addr, regs);

    /* Verify access to 4th operand                        */
    /* 4th-operand data value is stored at 4th operand     */
    ARCH_DEP(validate_operand) (op4addr, r3, 16-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op4d, 16-1, effective_addr4 + 112, b4, regs );
    ARCH_DEP(wstorec) (op4d, 16-1, op4addr, r3, regs);

    return 0;   /* note: this function does NOT set cc */
}

/* ------------------------------------------------------------------ */
/* plo-extension function: 61 Quadruple Store : Octoword              */
/* ------------------------------------------------------------------ */
int ARCH_DEP(plo_qsto) (int r1, int r3, VADR effective_addr2, int b2,
                          VADR effective_addr4, int b4, REGS *regs)
{
BYTE op1d[32],
     op2d[32],
     op3d[32],
     op4d[32];

U32 op1alet = 0,
    op2alet = 0,
    op3alet = 0,
    op4alet = 0;

VADR op1addr,
     op2addr,
     op3addr,
     op4addr;

    UNREFERENCED(r1);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);

    DW_CHECK(effective_addr4, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op1alet = ARCH_DEP(wfetch4)(effective_addr4 + 32, b4, regs);
        regs->AR(r3) = op1alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 1 */
    op1addr = ARCH_DEP(wfetch8)(effective_addr4 + 0, b4, regs);
    op1addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op1addr, regs);

    /* Verify access to 1st operand                         */
    /* 1st-operand datat value is stored at 1st operand     */
    ARCH_DEP(validate_operand) (op1addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op1d, 32-1, effective_addr4 + 64, b4, regs );
    ARCH_DEP(wstorec) (op1d, 32-1, op1addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op2alet = ARCH_DEP(wfetch4)(effective_addr4 + 36, b4, regs);
        regs->AR(r3) = op2alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 2 */
    op2addr = ARCH_DEP(wfetch8)(effective_addr4 + 8, b4, regs);
    op2addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op2addr, regs);

    /* Verify access to 2nd operand                        */
    /* 2nd-operand data value is stored at 2nd operand     */
    ARCH_DEP(validate_operand) (op2addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op2d, 32-1, effective_addr4 + 96, b4, regs );
    ARCH_DEP(wstorec) (op2d, 32-1, op2addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op3alet = ARCH_DEP(wfetch4)(effective_addr4 + 40, b4, regs);
        regs->AR(r3) = op3alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 3 */
    op3addr = ARCH_DEP(wfetch8)(effective_addr4 + 16, b4, regs);
    op3addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op3addr, regs);

    /* Verify access to 3rd operand                        */
    /* 3rd-operand data value is stored at 3rd operand     */
    ARCH_DEP(validate_operand) (op3addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op3d, 32-1, effective_addr4 + 128, b4, regs );
    ARCH_DEP(wstorec) (op3d, 32-1, op3addr, r3, regs);

    /* When in ar mode, ar3 is used to access the
           operand. The alet is fetched from the pl */
    if(!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        if(r3 == 0)
            ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
        op4alet = ARCH_DEP(wfetch4)(effective_addr4 + 44, b4, regs);
        regs->AR(r3) = op4alet;
        SET_AEA_AR(regs, r3);
    }

    /* Load address of operand 4 */
    op4addr = ARCH_DEP(wfetch8)(effective_addr4 + 24, b4, regs);
    op4addr &= ADDRESS_MAXWRAP(regs);
    OW_CHECK(op4addr, regs);

    /* Verify access to 4th operand                        */
    /* 4th-operand data value is stored at 4th operand     */
    ARCH_DEP(validate_operand) (op4addr, r3, 32-1, ACCTYPE_WRITE_SKP, regs);
    ARCH_DEP(vfetchc) (op4d, 32-1, effective_addr4 + 160, b4, regs );
    ARCH_DEP(wstorec) (op4d, 32-1, op4addr, r3, regs);

    return 0;   /* note: this function does NOT set cc */
}

#endif /* defined(FEATURE_087_PLO_EXTENSION) */

#endif /*defined(FEATURE_PERFORM_LOCKED_OPERATION)*/



#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "plo.c"
#endif

#if defined(_ARCH_NUM_2)
#undef   _GEN_ARCH
#define  _GEN_ARCH _ARCH_NUM_2
#include "plo.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
