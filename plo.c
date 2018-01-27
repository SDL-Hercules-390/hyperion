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
        /* Store 2nd op at 1st op comare value */
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
        /* Store 2nd op at 1st op comare value */
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
