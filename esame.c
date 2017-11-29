/* ESAME.C      (c) Copyright Jan Jaeger, 2000-2012                  */
/*              ESAME (z/Architecture) instructions                  */

/*-------------------------------------------------------------------*/
/* This module implements the instructions which exist in ESAME      */
/* mode but not in ESA/390 mode, as described in the manual          */
/* SA22-7832-00 z/Architecture Principles of Operation               */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      EPSW/EREGG/LMD instructions - Roger Bowler                   */
/*      PKA/PKU/UNPKA/UNPKU instructions - Roger Bowler              */
/*      Multiply/Divide Logical instructions - Vic Cross      Feb2001*/
/*      Long displacement facility - Roger Bowler            June2003*/
/*      DAT enhancement facility - Roger Bowler              July2004*/
/*      Extended immediate facility - Roger Bowler            Aug2005*/
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#if !defined(_HENGINE_DLL_)
#define _HENGINE_DLL_
#endif

#if !defined(_ESAME_C_)
#define _ESAME_C_
#endif

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "clock.h"

/* When an operation code has unused operand(s) (IPK, e.g.), it will */
/* attract  a diagnostic for a set, but unused variable.  Fixing the */
/* macros to support e.g., RS_NOOPS is not productive, so:           */
DISABLE_GCC_UNUSED_SET_WARNING

#if defined(FEATURE_BINARY_FLOATING_POINT)
/*-------------------------------------------------------------------*/
/* B29C STFPC - Store FPC                                        [S] */
/*-------------------------------------------------------------------*/
DEF_INST(store_fpc)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);

    BFPINST_CHECK(regs);

    /* Store register contents at operand address */
    ARCH_DEP(vstore4) ( regs->fpc, effective_addr2, b2, regs );

} /* end DEF_INST(store_fpc) */
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/


#if defined(FEATURE_BINARY_FLOATING_POINT)
/*-------------------------------------------------------------------*/
/* B29D LFPC  - Load FPC                                         [S] */
/*-------------------------------------------------------------------*/
DEF_INST(load_fpc)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     tmp_fpc;

    S(inst, regs, b2, effective_addr2);

    BFPINST_CHECK(regs);

    /* Load FPC register from operand address */
    tmp_fpc = ARCH_DEP(vfetch4) (effective_addr2, b2, regs);

    /* Program check if reserved bits are non-zero */
    FPC_CHECK(tmp_fpc, regs);

    /* Update FPC register */
    regs->fpc = tmp_fpc;

} /* end DEF_INST(load_fpc) */
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/


#if defined(FEATURE_BINARY_FLOATING_POINT)
/*-------------------------------------------------------------------*/
/* B384 SFPC  - Set FPC                                        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(set_fpc)
{
int     r1, unused;                     /* Values of R fields        */

    RRE(inst, regs, r1, unused);

    BFPINST_CHECK(regs);

    /* Program check if reserved bits are non-zero */
    FPC_CHECK(regs->GR_L(r1), regs);

    /* Load FPC register from R1 register bits 32-63 */
    regs->fpc = regs->GR_L(r1);

} /* end DEF_INST(set_fpc) */
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/


#if defined(FEATURE_BINARY_FLOATING_POINT)
/*-------------------------------------------------------------------*/
/* B38C EFPC  - Extract FPC                                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(extract_fpc)
{
int     r1, unused;                     /* Values of R fields        */

    RRE(inst, regs, r1, unused);

    BFPINST_CHECK(regs);

    /* Load R1 register bits 32-63 from FPC register */
    regs->GR_L(r1) = regs->fpc;

} /* end DEF_INST(extract_fpc) */
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/


#if defined(FEATURE_BINARY_FLOATING_POINT)
/*-------------------------------------------------------------------*/
/* B299 SRNM  - Set BFP Rounding Mode (2-bit)                    [S] */
/*-------------------------------------------------------------------*/
DEF_INST(set_bfp_rounding_mode_2bit)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);

    BFPINST_CHECK(regs);

    /* Set FPC register BFP rounding mode bits from operand address */
    regs->fpc &= ~(FPC_BRM_2BIT);
    regs->fpc |= (effective_addr2 & FPC_BRM_2BIT);

#if defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)          /*810*/
    /* Zeroize FPC bit 29 if FP Extension Facility is installed */
    regs->fpc &= ~(FPC_BIT29);
#endif /*defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)*/   /*810*/

} /* end DEF_INST(set_bfp_rounding_mode_2bit) */
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/


#if defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)          /*810*/
/*-------------------------------------------------------------------*/
/* B2B8 SRNMB - Set BFP Rounding Mode (3-bit)                    [S] */
/*-------------------------------------------------------------------*/
DEF_INST(set_bfp_rounding_mode_3bit)                            /*810*/
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);

    BFPINST_CHECK(regs);

    /* Program check if operand address bits 56-60 are non-zero */
    if ((effective_addr2 & 0xFF) & ~(FPC_BRM_3BIT))
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Program check if operand address bits 61-63 not a valid BRM */
    if ((effective_addr2 & FPC_BRM_3BIT) == BRM_RESV4
     || (effective_addr2 & FPC_BRM_3BIT) == BRM_RESV5
     || (effective_addr2 & FPC_BRM_3BIT) == BRM_RESV6)
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Set FPC 3-bit BFP rounding mode bits from operand address */
    regs->fpc &= ~(FPC_BRM_3BIT);
    regs->fpc |= (effective_addr2 & FPC_BRM_3BIT);

} /* end DEF_INST(set_bfp_rounding_mode_3bit) */
#endif /*defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)*/   /*810*/


#if defined(FEATURE_LINKAGE_STACK)
/*-------------------------------------------------------------------*/
/* 01FF TRAP2 - Trap                                             [E] */
/*-------------------------------------------------------------------*/
DEF_INST(trap2)
{
    E(inst, regs);

    UNREFERENCED(inst);

    ARCH_DEP(trap_x) (0, regs, 0);

} /* end DEF_INST(trap2) */
#endif /*defined(FEATURE_LINKAGE_STACK)*/


#if defined(FEATURE_LINKAGE_STACK)
/*-------------------------------------------------------------------*/
/* B2FF TRAP4 - Trap                                             [S] */
/*-------------------------------------------------------------------*/
DEF_INST(trap4)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);

    ARCH_DEP(trap_x) (1, regs, effective_addr2);

} /* end DEF_INST(trap4) */
#endif /*defined(FEATURE_LINKAGE_STACK)*/


#if defined(FEATURE_RESUME_PROGRAM)
/*-------------------------------------------------------------------*/
/* B277 RP    - Resume Program                                   [S] */
/*-------------------------------------------------------------------*/
DEF_INST(resume_program)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
VADR    pl_addr;                        /* Address of parmlist       */
U16     flags;                          /* Flags in parmfield        */
U16     psw_offset;                     /* Offset to new PSW         */
U16     ar_offset;                      /* Offset to new AR          */
U16     gr_offset;                      /* Offset to new GR          */
U32     ar;                             /* Copy of new AR            */
U32     gr = 0;                         /* Copy of new GR            */
#if defined(FEATURE_ESAME)
U16     grd_offset = 0;                 /* Offset of disjoint GR_H   */
BYTE    psw[16];                        /* Copy of new PSW           */
U64     gr8 = 0;                        /* Copy of new GR - 8 bytes  */
U32     grd = 0;                        /* Copy of new GR - disjoint */
U64     ia;                             /* ia for trace              */
BYTE    amode64;                        /* save for amod64           */
#else /*!defined(FEATURE_ESAME)*/
BYTE    psw[8];                         /* Copy of new PSW           */
U32     ia;                             /* ia for trace              */
#endif /*!defined(FEATURE_ESAME)*/
BYTE    amode;                          /* amode for trace           */
PSW     save_psw;                       /* Saved copy of current PSW */
BYTE   *mn;                             /* Mainstor address of parm  */
#ifdef FEATURE_TRACING
CREG    newcr12 = 0;                    /* CR12 upon completion      */
#endif /*FEATURE_TRACING*/

    S(inst, regs, b2, effective_addr2);

    /* Determine the address of the parameter list */
    pl_addr = likely(!regs->execflag) ? PSW_IA(regs, 0) :
               regs->exrl ? (regs->ET + 6) : (regs->ET + 4);

    /* Fetch flags from the instruction address space */
    mn = MADDR (pl_addr, USE_INST_SPACE, regs, ACCTYPE_INSTFETCH, regs->psw.pkey);
    FETCH_HW(flags, mn);

#if defined(FEATURE_ESAME)
    /* Bits 0-12 must be zero */
    if(flags & 0xFFF8)
#else /*!defined(FEATURE_ESAME)*/
    /* All flag bits must be zero in ESA/390 mode */
    if(flags)
#endif /*!defined(FEATURE_ESAME)*/
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Fetch the offset to the new psw */
    mn = MADDR (pl_addr + 2, USE_INST_SPACE, regs, ACCTYPE_INSTFETCH, regs->psw.pkey);
    FETCH_HW(psw_offset, mn);

    /* Fetch the offset to the new ar */
    mn = MADDR (pl_addr + 4, USE_INST_SPACE, regs, ACCTYPE_INSTFETCH, regs->psw.pkey);
    FETCH_HW(ar_offset, mn);

    /* Fetch the offset to the new gr */
    mn = MADDR (pl_addr + 6, USE_INST_SPACE, regs, ACCTYPE_INSTFETCH, regs->psw.pkey);
    FETCH_HW(gr_offset, mn);

#if defined(FEATURE_ESAME)
    /* Fetch the offset to the new disjoint gr_h */
    if((flags & 0x0003) == 0x0003)
    {
        mn = MADDR (pl_addr + 8, USE_INST_SPACE, regs, ACCTYPE_INSTFETCH, regs->psw.pkey);
        FETCH_HW(grd_offset, mn);
    }
#endif /*defined(FEATURE_ESAME)*/


    /* Fetch the PSW from the operand address + psw offset */
#if defined(FEATURE_ESAME)
    if(flags & 0x0004)
        ARCH_DEP(vfetchc) (psw, 15, (effective_addr2 + psw_offset)
                           & ADDRESS_MAXWRAP(regs), b2, regs);
    else
#endif /*defined(FEATURE_ESAME)*/
        ARCH_DEP(vfetchc) (psw, 7, (effective_addr2 + psw_offset)
                           & ADDRESS_MAXWRAP(regs), b2, regs);


    /* Fetch new AR (B2) from operand address + AR offset */
    ar = ARCH_DEP(vfetch4) ((effective_addr2 + ar_offset)
                            & ADDRESS_MAXWRAP(regs), b2, regs);


    /* Fetch the new gr from operand address + GPR offset */
#if defined(FEATURE_ESAME)
    /* General Register Field 1 is eight bytes */
    if((flags & 0x0003) == 0x0002)
    {
        gr8 = ARCH_DEP(vfetch8) ((effective_addr2 + gr_offset)
                                & ADDRESS_MAXWRAP(regs), b2, regs);
    }
    /* General Register Field 1 and 2 are four bytes - disjoint */
    else if((flags & 0x0003) == 0x0003)
    {
        gr = ARCH_DEP(vfetch4) ((effective_addr2 + gr_offset)
                                & ADDRESS_MAXWRAP(regs), b2, regs);
        grd = ARCH_DEP(vfetch4) ((effective_addr2 + grd_offset)
                                & ADDRESS_MAXWRAP(regs), b2, regs);
    }
    else
#endif /*defined(FEATURE_ESAME)*/
        gr = ARCH_DEP(vfetch4) ((effective_addr2 + gr_offset)
                                & ADDRESS_MAXWRAP(regs), b2, regs);

#if defined(FEATURE_TRACING)
#if defined(FEATURE_ESAME)
    /* fetch 8 or 4 byte IA depending on psw operand size */
    if (flags & 0x0004)
        FETCH_DW(ia, psw + 8);
    else
        FETCH_FW(ia, psw + 4);
    amode64 = psw[3] & 0x01;
#else /*!defined(FEATURE_ESAME)*/
    FETCH_FW(ia, psw + 4);
    ia &= 0x7FFFFFFF;
#endif /*!defined(FEATURE_ESAME)*/
    amode = psw[4] & 0x80;

#if defined(FEATURE_ESAME)
    /* Add a mode trace entry when switching in/out of 64 bit mode */
    if((regs->CR(12) & CR12_MTRACE) && (regs->psw.amode64 != amode64))
        newcr12 = ARCH_DEP(trace_ms) (regs->CR(12) & CR12_BRTRACE ? 1 : 0, ia, regs);
    else
#endif /*defined(FEATURE_ESAME)*/
    if (regs->CR(12) & CR12_BRTRACE)
        newcr12 = ARCH_DEP(trace_br) (amode, ia, regs);
#endif /*defined(FEATURE_TRACING)*/

    INVALIDATE_AIA(regs);

    /* Save current PSW */
    save_psw = regs->psw;


    /* Use bytes 0 and 1 of old psw and byte 2 from operand */
    psw[0] = save_psw.sysmask;
    psw[1] = save_psw.pkey | save_psw.states;
    /* ignore bits 24-30 */
    psw[3] = 0x01 & psw[3];


#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)
    if(SIE_STATB(regs, MX, XC)
      && (psw[2] & 0x80))
        regs->program_interrupt (regs, PGM_SPECIAL_OPERATION_EXCEPTION);
#endif /*defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)*/

    /* Special operation exception when setting AR space mode
       and ASF is off */
    if(!REAL_MODE(&regs->psw)
      && ((psw[2] & 0xC0) == 0x40)
      && !ASF_ENABLED(regs) )
        regs->program_interrupt (regs, PGM_SPECIAL_OPERATION_EXCEPTION);

    /* Privileged Operation exception when setting home
       space mode in problem state */
    if(!REAL_MODE(&regs->psw)
      && PROBSTATE(&regs->psw)
      && ((psw[2] & 0xC0) == 0xC0) )
        regs->program_interrupt (regs, PGM_PRIVILEGED_OPERATION_EXCEPTION);

#if defined(FEATURE_ESAME)
    /* Handle 16 byte psw operand */
    if(flags & 0x0004)
    {
        psw[1] &= ~0x08; /* force bit 12 off */
        if( ARCH_DEP(load_psw) (regs, psw) )/* only check invalid IA not odd */
        {
            /* restore the psw */
            regs->psw = save_psw;
            /* And generate a program interrupt */
            regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);
        }
    }
    /* Handle 8 byte psw operand */
    else
    {
        /* Save amode64, do not check amode64 bit (force to zero) */
        /* This is so s390_load_psw will work.                    */
        /* Checks for amode64 will be done a few lines later      */
        amode64 = psw[3] & 01;
        psw[3] &= ~0x01;
#endif /*defined(FEATURE_ESAME)*/
        psw[1] |= 0x08; /* force bit 12 on */
        if( s390_load_psw(regs, psw) )
        {
            /* restore the psw */
            regs->psw = save_psw;
            /* And generate a program interrupt */
            regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);
        }
#if defined(FEATURE_ESAME)
        regs->psw.states &= ~BIT(PSW_NOTESAME_BIT);
        /* clear high word of IA since operand was 8-byte psw */
        regs->psw.IA_H = 0;
        /* Check original amode64 and restore and do checks */
        if (amode64)
        {
            /* if amode64 (31) on, then amode (32) must be on too */
            if (!regs->psw.amode)
            {
                /* restore the psw */
                regs->psw = save_psw;
                /* And generate a program interrupt */
                regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);
            }
            regs->psw.amode64 = 1;
            regs->psw.AMASK = AMASK64;
        }
        else
        {
            regs->psw.amode64 = 0;
            regs->psw.AMASK_H = 0;
        }
    }
#endif /*defined(FEATURE_ESAME)*/

    /* Check for odd IA in psw */
    if(regs->psw.IA & 0x01)
    {
        /* restore the psw */
        regs->psw = save_psw;
        /* And generate a program interrupt */
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Update access register b2 */
    regs->AR(b2) = ar;

    /* Update general register b2 */
#if defined(FEATURE_ESAME)
    if((flags & 0x0003) == 0x0002)
        regs->GR_G(b2) = gr8;
    else if((flags & 0x0003) == 0x0003)
    {
        regs->GR_L(b2) = gr;
        regs->GR_H(b2) = grd;
    }
    else
#endif /*defined(FEATURE_ESAME)*/
        regs->GR_L(b2) = gr;

#ifdef FEATURE_TRACING
    /* Update trace table address if branch tracing is on */
    if (newcr12)
        regs->CR(12) = newcr12;
#endif /*FEATURE_TRACING*/

    SET_BEAR_REG(regs, regs->ip - 4);
    SET_IC_ECMODE_MASK(regs);
    SET_AEA_MODE(regs);
    PER_SB(regs, regs->psw.IA);

    /* Space switch event when switching into or
       out of home space mode AND space-switch-event on in CR1 or CR13 */
    if((HOME_SPACE_MODE(&(regs->psw)) ^ HOME_SPACE_MODE(&save_psw))
     && (!REAL_MODE(&regs->psw))
     && ((regs->CR(1) & SSEVENT_BIT) || (regs->CR(13) & SSEVENT_BIT)
      || OPEN_IC_PER(regs) ))
    {
        if (HOME_SPACE_MODE(&(regs->psw)))
        {
            /* When switching into home-space mode, set the
               translation exception address equal to the primary
               ASN, with the high-order bit set equal to the value
               of the primary space-switch-event control bit */
            regs->TEA = regs->CR_LHL(4);
            if (regs->CR(1) & SSEVENT_BIT)
                regs->TEA |= TEA_SSEVENT;
        }
        else
        {
            /* When switching out of home-space mode, set the
               translation exception address equal to zero, with
               the high-order bit set equal to the value of the
               home space-switch-event control bit */
            regs->TEA = 0;
            if (regs->CR(13) & SSEVENT_BIT)
                regs->TEA |= TEA_SSEVENT;
        }
        regs->program_interrupt (regs, PGM_SPACE_SWITCH_EVENT);
    }

    RETURN_INTCHECK(regs);

} /* end DEF_INST(resume_program) */
#endif /*defined(FEATURE_RESUME_PROGRAM)*/


#if defined(FEATURE_ESAME) && defined(FEATURE_TRACING)
/*-------------------------------------------------------------------*/
/* EB0F TRACG - Trace Long                                     [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(trace_long)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
#if defined(FEATURE_TRACING)
U32     op;                             /* Operand                   */
#endif /*defined(FEATURE_TRACING)*/

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    PRIV_CHECK(regs);

    FW_CHECK(effective_addr2, regs);

    /* Exit if explicit tracing (control reg 12 bit 31) is off */
    if ( (regs->CR(12) & CR12_EXTRACE) == 0 )
        return;

    /* Fetch the trace operand */
    op = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Exit if bit zero of the trace operand is one */
    if ( (op & 0x80000000) )
        return;

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION (regs);
    PERFORM_CHKPT_SYNC (regs);

    regs->CR(12) = ARCH_DEP(trace_tg) (r1, r3, op, regs);

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION (regs);
    PERFORM_CHKPT_SYNC (regs);

} /* end DEF_INST(trace_long) */
#endif /*defined(FEATURE_ESAME) && defined(FEATURE_TRACING)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E30E CVBG  - Convert to Binary Long                         [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_to_binary_long)
{
U64     dreg;                           /* 64-bit result accumulator */
int     r1;                             /* Value of R1 field         */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     ovf;                            /* 1=overflow                */
int     dxf;                            /* 1=data exception          */
BYTE    dec[16];                        /* Packed decimal operand    */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Fetch 16-byte packed decimal operand */
    ARCH_DEP(vfetchc) ( dec, 16-1, effective_addr2, b2, regs );

    /* Convert 16-byte packed decimal to 64-bit signed binary */
    packed_to_binary (dec, 16-1, &dreg, &ovf, &dxf);

    /* Data exception if invalid digits or sign */
    if (dxf)
    {
        regs->dxc = DXC_DECIMAL;
        regs->program_interrupt (regs, PGM_DATA_EXCEPTION);
    }

    /* Exception if overflow (operation suppressed, R1 unchanged) */
    if (ovf)
        regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);

    /* Store 64-bit result into R1 register */
    regs->GR_G(r1) = dreg;

} /* end DEF_INST(convert_to_binary_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E32E CVDG  - Convert to Decimal Long                        [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_to_decimal_long)
{
S64     bin;                            /* Signed value to convert   */
int     r1;                             /* Value of R1 field         */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
BYTE    dec[16];                        /* Packed decimal result     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load signed value of register */
    bin = (S64)(regs->GR_G(r1));

    /* Convert to 16-byte packed decimal number */
    binary_to_packed (bin, dec);

    /* Store 16-byte packed decimal result at operand address */
    ARCH_DEP(vstorec) ( dec, 16-1, effective_addr2, b2, regs );

} /* end DEF_INST(convert_to_decimal_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E396 ML    - Multiply Logical                               [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_logical)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective Address         */
U32     m;
U64     p;

    RXY(inst, regs, r1, b2, effective_addr2);

    ODD_CHECK(r1, regs);

    /* Load second operand from operand address */
    m = ARCH_DEP(vfetch4) (effective_addr2, b2, regs);

    /* Multiply unsigned values */
    p = (U64)regs->GR_L(r1 + 1) * m;

    /* Store the result */
    regs->GR_L(r1) = (p >> 32);
    regs->GR_L(r1 + 1) = (p & 0xFFFFFFFF);

} /* end DEF_INST(multiply_logical) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E386 MLG   - Multiply Logical Long                          [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_logical_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective Address         */
U64     m, ph, pl;

    RXY(inst, regs, r1, b2, effective_addr2);

    ODD_CHECK(r1, regs);

    /* Load second operand from operand address */
    m = ARCH_DEP(vfetch8) (effective_addr2, b2, regs);

    /* Multiply unsigned values */
    mult_logical_long(&ph, &pl, regs->GR_G(r1 + 1), m);

    /* Store the result */
    regs->GR_G(r1) = ph;
    regs->GR_G(r1 + 1) = pl;

} /* end DEF_INST(multiply_logical_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B996 MLR   - Multiply Logical Register                      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_logical_register)
{
int     r1, r2;                         /* Values of R fields        */
U64     p;

    RRE(inst, regs, r1, r2);

    ODD_CHECK(r1, regs);

    /* Multiply unsigned values */
    p = (U64)regs->GR_L(r1 + 1) * (U64)regs->GR_L(r2);

    /* Store the result */
    regs->GR_L(r1) = (p >> 32);
    regs->GR_L(r1 + 1) = (p & 0xFFFFFFFF);

} /* end DEF_INST(multiply_logical_register) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B986 MLGR  - Multiply Logical Long Register                 [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_logical_long_register)
{
int     r1, r2;                         /* Values of R fields        */
U64     ph, pl;

    RRE(inst, regs, r1, r2);

    ODD_CHECK(r1, regs);

    /* Multiply unsigned values */
    mult_logical_long(&ph, &pl, regs->GR_G(r1 + 1), regs->GR_G(r2));

    /* Store the result */
    regs->GR_G(r1) = ph;
    regs->GR_G(r1 + 1) = pl;

} /* end DEF_INST(multiply_logical_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E397 DL    - Divide Logical                                 [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_logical)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective Address         */
U32     d;
U64     n;

    RXY(inst, regs, r1, b2, effective_addr2);

    ODD_CHECK(r1, regs);

    n = ((U64)regs->GR_L(r1) << 32) | (U32)regs->GR_L(r1 + 1);

    /* Load second operand from operand address */
    d = ARCH_DEP(vfetch4) (effective_addr2, b2, regs);

    if (d == 0
      || (n / d) > 0xFFFFFFFF)
        regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);

    /* Divide unsigned registers */
    regs->GR_L(r1) = n % d;
    regs->GR_L(r1 + 1) = n / d;

} /* end DEF_INST(divide_logical) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E387 DLG   - Divide Logical Long                            [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_logical_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective Address         */
U64     d, r, q;

    RXY(inst, regs, r1, b2, effective_addr2);

    ODD_CHECK(r1, regs);

    /* Load second operand from operand address */
    d = ARCH_DEP(vfetch8) (effective_addr2, b2, regs);

    if (regs->GR_G(r1) == 0)            /* check for the simple case */
    {
      if (d == 0)
          regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);

      /* Divide signed registers */
      regs->GR_G(r1) = regs->GR_G(r1 + 1) % d;
      regs->GR_G(r1 + 1) = regs->GR_G(r1 + 1) / d;
    }
    else
    {
      if (div_logical_long(&r, &q, regs->GR_G(r1), regs->GR_G(r1 + 1), d) )
          regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);
      else
      {
        regs->GR_G(r1) = r;
        regs->GR_G(r1 + 1) = q;
      }

    }
} /* end DEF_INST(divide_logical_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B997 DLR   - Divide Logical Register                        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_logical_register)
{
int     r1, r2;                         /* Values of R fields        */
U64     n;
U32     d;

    RRE(inst, regs, r1, r2);

    ODD_CHECK(r1, regs);

    n = ((U64)regs->GR_L(r1) << 32) | regs->GR_L(r1 + 1);

    d = regs->GR_L(r2);

    if(d == 0
      || (n / d) > 0xFFFFFFFF)
        regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);

    /* Divide signed registers */
    regs->GR_L(r1) = n % d;
    regs->GR_L(r1 + 1) = n / d;

} /* end DEF_INST(divide_logical_register) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B987 DLGR  - Divide Logical Long Register                   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_logical_long_register)
{
int     r1, r2;                         /* Values of R fields        */
U64     r, q, d;

    RRE(inst, regs, r1, r2);

    ODD_CHECK(r1, regs);

    d = regs->GR_G(r2);

    if (regs->GR_G(r1) == 0)            /* check for the simple case */
    {
      if(d == 0)
          regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);

      /* Divide signed registers */
      regs->GR_G(r1) = regs->GR_G(r1 + 1) % d;
      regs->GR_G(r1 + 1) = regs->GR_G(r1 + 1) / d;
    }
    else
    {
      if (div_logical_long(&r, &q, regs->GR_G(r1), regs->GR_G(r1 + 1), d) )
          regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);
      else
      {
        regs->GR_G(r1) = r;
        regs->GR_G(r1 + 1) = q;
      }
    }
} /* end DEF_INST(divide_logical_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B988 ALCGR - Add Logical with Carry Long Register           [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_carry_long_register)
{
int     r1, r2;                         /* Values of R fields        */
int     carry = 0;
U64     n;

    RRE0(inst, regs, r1, r2);

    n = regs->GR_G(r2);

    /* Add the carry to operand */
    if(regs->psw.cc & 2)
        carry = add_logical_long(&(regs->GR_G(r1)),
                                   regs->GR_G(r1),
                                   1) & 2;

    /* Add unsigned operands and set condition code */
    regs->psw.cc = add_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      n) | carry;
} /* end DEF_INST(add_logical_carry_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B989 SLBGR - Subtract Logical with Borrow Long Register     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_borrow_long_register)
{
int     r1, r2;                         /* Values of R fields        */
int     borrow = 2;
U64     n;

    RRE0(inst, regs, r1, r2);

    n = regs->GR_G(r2);

    /* Subtract the borrow from operand */
    if(!(regs->psw.cc & 2))
        borrow = sub_logical_long(&(regs->GR_G(r1)),
                                    regs->GR_G(r1),
                                    1);

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      n) & (borrow|1);

} /* end DEF_INST(subtract_logical_borrow_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_DAT_ENHANCEMENT)
/*-------------------------------------------------------------------*/
/* B98A CSPG  - Compare and Swap and Purge Long                [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_swap_and_purge_long)
{
int     r1, r2;                         /* Values of R fields        */
U64     n2;                             /* Virtual address of op2    */
BYTE   *main2;                          /* Mainstor address of op2   */
U64     old;                            /* Old value                 */

    RRE(inst, regs, r1, r2);

    PRIV_CHECK(regs);

    ODD_CHECK(r1, regs);

#if defined(_FEATURE_SIE)
    if(SIE_STATB(regs,IC0, IPTECSP))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_SIE)*/

#if defined(_FEATURE_SIE)
    if(SIE_MODE(regs) && regs->sie_scao)
    {
        STORAGE_KEY(regs->sie_scao, regs) |= STORKEY_REF;
        if(regs->mainstor[regs->sie_scao] & 0x80)
            longjmp(regs->progjmp, SIE_INTERCEPT_INST);
    }
#endif /*defined(_FEATURE_SIE)*/

    /* Perform serialization before starting operation */
    PERFORM_SERIALIZATION (regs);

    /* Obtain 2nd operand address from r2 */
    n2 = regs->GR(r2) & 0xFFFFFFFFFFFFFFF8ULL & ADDRESS_MAXWRAP(regs);
    main2 = MADDR (n2, r2, regs, ACCTYPE_WRITE, regs->psw.pkey);

    old = CSWAP64 (regs->GR_G(r1));

    /* Obtain main-storage access lock */
    OBTAIN_MAINLOCK(regs);

    /* Attempt to exchange the values */
    regs->psw.cc = cmpxchg8 (&old, CSWAP64(regs->GR_G(r1+1)), main2);

    /* Release main-storage access lock */
    RELEASE_MAINLOCK(regs);

    if (regs->psw.cc == 0)
    {
        /* Perform requested funtion specified as per request code in r2 */
        if (regs->GR_L(r2) & 3)
        {
            OBTAIN_INTLOCK(regs);
            SYNCHRONIZE_CPUS(regs);
            if (regs->GR_L(r2) & 1)
                ARCH_DEP(purge_tlb_all)();
            if (regs->GR_L(r2) & 2)
                ARCH_DEP(purge_alb_all)();
            RELEASE_INTLOCK(regs);
        }
    }
    else
    {
        PTT_CSF("*CSPG",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);

        /* Otherwise yield */
        regs->GR_G(r1) = CSWAP64(old);
        if (sysblk.cpus > 1)
            sched_yield();
    }

    /* Perform serialization after completing operation */
    PERFORM_SERIALIZATION (regs);

} /* end DEF_INST(compare_and_swap_and_purge_long) */
#endif /*defined(FEATURE_DAT_ENHANCEMENT)*/


#if defined(FEATURE_DAT_ENHANCEMENT)
/*-------------------------------------------------------------------*/
/* B98E IDTE  - Invalidate DAT Table Entry                     [RRF] */
/*-------------------------------------------------------------------*/
DEF_INST(invalidate_dat_table_entry)
{
int     r1, r2, r3;                     /* Values of R fields        */
int     m4;                             /* Unused mask field         */
U64     asceto;                         /* ASCE table origin         */
int     ascedt;                         /* ASCE designation type     */
int     count;                          /* Invalidation counter      */
int     eiindx;                         /* Eff. invalidation index   */
U64     asce;                           /* Contents of ASCE          */
BYTE   *mn;                             /* Mainstor address of ASCE  */

    RRF_RM(inst, regs, r1, r2, r3, m4);

    SIE_XC_INTERCEPT(regs);

    PRIV_CHECK(regs);

    /* Program check if bits 44-51 of r2 register are non-zero */
    if (regs->GR_L(r2) & 0x000FF000)
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

#if defined(_FEATURE_SIE)
    if(SIE_STATB(regs,IC0, IPTECSP))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_SIE)*/

#if defined(_FEATURE_SIE)
    if(SIE_MODE(regs) && regs->sie_scao)
    {
        STORAGE_KEY(regs->sie_scao, regs) |= STORKEY_REF;
        if(regs->mainstor[regs->sie_scao] & 0x80)
            longjmp(regs->progjmp, SIE_INTERCEPT_INST);
    }
#endif /*defined(_FEATURE_SIE)*/

    /* Perform serialization before starting operation */
    PERFORM_SERIALIZATION (regs);

    /* Bit 52 of the r2 register determines the operation performed */
    if ((regs->GR_L(r2) & 0x00000800) == 0)
    {
        /* Perform invalidation-and-clearing operation */

        /* Extract the invalidation table origin and type from r1 */
        asceto = regs->GR_G(r1) & ASCE_TO;
        ascedt = regs->GR_L(r1) & ASCE_DT;

        /* Extract the effective invalidation index from r2 */
        switch(ascedt) {
        case TT_R1TABL: /* Region first table */
            eiindx = (regs->GR_H(r2) & 0xFF700000) >> 18;
            break;
        case TT_R2TABL: /* Region second table */
            eiindx = (regs->GR_H(r2) & 0x001FFC00) >> 7;
            break;
        case TT_R3TABL: /* Region third table */
            eiindx = (regs->GR_G(r2) & 0x000003FF80000000ULL) >> 28;
            break;
        case TT_SEGTAB: /* Segment table */
        default:
            eiindx = (regs->GR_L(r2) & 0x7FF00000) >> 17;
            break;
        } /* end switch(ascedt) */

        /* Calculate the address of table for invalidation, noting
           that it is always a 64-bit address regardless of the
           current addressing mode, and that overflow is ignored */
        asceto += eiindx;

        /* Extract the additional entry count from r2 */
        count = (regs->GR_L(r2) & 0x7FF) + 1;

        /* Perform invalidation of one or more table entries */
        while (count-- > 0)
        {
            /* Fetch the table entry, set the invalid bit, then
               store only the byte containing the invalid bit */
            mn = MADDR (asceto, USE_REAL_ADDR, regs, ACCTYPE_WRITE, regs->psw.pkey);
            FETCH_DW(asce, mn);
            asce |= ZSEGTAB_I;
            mn[7] = asce & 0xFF;

            /* Calculate the address of the next table entry, noting
               that it is always a 64-bit address regardless of the
               current addressing mode, and that overflow is ignored */
            asceto += 8;
        } /* end while */

        /* Clear the TLB and signal all other CPUs to clear their TLB */
        /* Note: Currently we clear all entries regardless of whether
           a clearing ASCE is passed in the r3 register. This conforms
           to the POP which only specifies the minimum set of entries
           which must be cleared from the TLB. */
        OBTAIN_INTLOCK(regs);
        SYNCHRONIZE_CPUS(regs);
        ARCH_DEP(purge_tlb_all)();
        RELEASE_INTLOCK(regs);

    } /* end if(invalidation-and-clearing) */
    else
    {
        /* Perform clearing-by-ASCE operation */

        /* Clear the TLB and signal all other CPUs to clear their TLB */
        /* Note: Currently we clear all entries regardless of the
           clearing ASCE passed in the r3 register. This conforms
           to the POP which only specifies the minimum set of entries
           which must be cleared from the TLB. */
        OBTAIN_INTLOCK(regs);
        SYNCHRONIZE_CPUS(regs);
        ARCH_DEP(purge_tlb_all)();
        RELEASE_INTLOCK(regs);

    } /* end else(clearing-by-ASCE) */

    /* Perform serialization after completing operation */
    PERFORM_SERIALIZATION (regs);

} /* end DEF_INST(invalidate_dat_table_entry) */
#endif /*defined(FEATURE_DAT_ENHANCEMENT)*/


#if defined(FEATURE_DAT_ENHANCEMENT_FACILITY_2)
/*-------------------------------------------------------------------*/
/* B9AA LPTEA - Load Page-Table-Entry Address                  [RRF] */
/*-------------------------------------------------------------------*/
DEF_INST(load_page_table_entry_address)
{
VADR    vaddr;                          /* Virtual address           */
int     r1, r2, r3;                     /* Register numbers          */
int     m4;                             /* Mask field                */
int     n;                              /* Address space indication  */
int     cc;                             /* Condition code            */
int     acctype = ACCTYPE_LPTEA;        /* Storage access type       */

    RRF_RM(inst, regs, r1, r2, r3, m4);

    SIE_XC_INTERCEPT(regs);

    PRIV_CHECK(regs);

    /* The m4 field determines which address space to use */
    switch (m4) {
    case 0: /* Use ASCE in control register 1 */
        n = USE_PRIMARY_SPACE;
        break;
    case 1: /* Use ALET in access register r2 */
        n = USE_ARMODE | r2;
        break;
    case 2: /* Use ASCE in control register 7 */
        n = USE_SECONDARY_SPACE;
        break;
    case 3: /* Use ASCE in control register 13 */
        n = USE_HOME_SPACE;
        break;
    case 4: /* Use current addressing mode (PSW bits 16-17) */
        n = r2; /* r2 is the access register number if ARMODE */
        break;
    default: /* Specification exception if invalid value for m4 */
        n = -1; /* makes compiler happy */
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);
    } /* end switch(m4) */

    /* Load the virtual address from the r2 register */
    vaddr = regs->GR(r2) & ADDRESS_MAXWRAP(regs);

    /* Find the page table address and condition code */
    cc = ARCH_DEP(translate_addr) (vaddr, n, regs, acctype);

    /* Set R1 to real address or exception code depending on cc */
    regs->GR_G(r1) = (cc < 3) ? regs->dat.raddr : regs->dat.xcode;

    /* Set condition code */
    regs->psw.cc = cc;

} /* end DEF_INST(load_page_table_entry_address) */
#endif /*defined(FEATURE_DAT_ENHANCEMENT_FACILITY_2)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E388 ALCG  - Add Logical with Carry Long                    [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_carry_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand values     */
int     carry = 0;

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Add the carry to operand */
    if(regs->psw.cc & 2)
        carry = add_logical_long(&(regs->GR_G(r1)),
                                   regs->GR_G(r1),
                                   1) & 2;

    /* Add unsigned operands and set condition code */
    regs->psw.cc = add_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      n) | carry;
} /* end DEF_INST(add_logical_carry_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E389 SLBG  - Subtract Logical with Borrow Long              [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_borrow_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand values     */
int     borrow = 2;

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Subtract the borrow from operand */
    if(!(regs->psw.cc & 2))
        borrow = sub_logical_long(&(regs->GR_G(r1)),
                                    regs->GR_G(r1),
                                    1);

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      n) & (borrow|1);

} /* end DEF_INST(subtract_logical_borrow_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B998 ALCR  - Add Logical with Carry Register                [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_carry_register)
{
int     r1, r2;                         /* Values of R fields        */
int     carry = 0;
U32     n;

    RRE0(inst, regs, r1, r2);

    n = regs->GR_L(r2);

    /* Add the carry to operand */
    if(regs->psw.cc & 2)
        carry = add_logical(&(regs->GR_L(r1)),
                              regs->GR_L(r1),
                              1) & 2;

    /* Add unsigned operands and set condition code */
    regs->psw.cc = add_logical(&(regs->GR_L(r1)),
                                 regs->GR_L(r1),
                                 n) | carry;
} /* end DEF_INST(add_logical_carry_register) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B999 SLBR  - Subtract Logical with Borrow Register          [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_borrow_register)
{
int     r1, r2;                         /* Values of R fields        */
int     borrow = 2;
U32     n;

    RRE0(inst, regs, r1, r2);

    n = regs->GR_L(r2);

    /* Subtract the borrow from operand */
    if(!(regs->psw.cc & 2))
        borrow = sub_logical(&(regs->GR_L(r1)),
                               regs->GR_L(r1),
                               1);

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical(&(regs->GR_L(r1)),
                                 regs->GR_L(r1),
                                 n) & (borrow|1);

} /* end DEF_INST(subtract_logical_borrow_register) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E398 ALC   - Add Logical with Carry                         [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_carry)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */
int     carry = 0;

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Add the carry to operand */
    if(regs->psw.cc & 2)
        carry = add_logical(&(regs->GR_L(r1)),
                              regs->GR_L(r1),
                              1) & 2;

    /* Add unsigned operands and set condition code */
    regs->psw.cc = add_logical(&(regs->GR_L(r1)),
                                 regs->GR_L(r1),
                                 n) | carry;
} /* end DEF_INST(add_logical_carry) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E399 SLB   - Subtract Logical with Borrow                   [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_borrow)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */
int     borrow = 2;

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Subtract the borrow from operand */
    if(!(regs->psw.cc & 2))
        borrow = sub_logical(&(regs->GR_L(r1)),
                               regs->GR_L(r1),
                               1);

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical(&(regs->GR_L(r1)),
                                 regs->GR_L(r1),
                                 n) & (borrow|1);

} /* end DEF_INST(subtract_logical_borrow) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E30D DSG   - Divide Single Long                             [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_single_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    ODD_CHECK(r1, regs);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    if(n == 0
      || ((S64)n == -1LL &&
          regs->GR_G(r1 + 1) == 0x8000000000000000ULL))
        regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);

    regs->GR_G(r1) = (S64)regs->GR_G(r1 + 1) % (S64)n;
    regs->GR_G(r1 + 1) = (S64)regs->GR_G(r1 + 1) / (S64)n;

} /* end DEF_INST(divide_single_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E31D DSGF  - Divide Single Long Fullword                    [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_single_long_fullword)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 64-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    ODD_CHECK(r1, regs);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    if(n == 0
      || ((S32)n == -1 &&
          regs->GR_G(r1 + 1) == 0x8000000000000000ULL))
        regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);

    regs->GR_G(r1) = (S64)regs->GR_G(r1 + 1) % (S32)n;
    regs->GR_G(r1 + 1) = (S64)regs->GR_G(r1 + 1) / (S32)n;

} /* end DEF_INST(divide_single_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B90D DSGR  - Divide Single Long Register                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_single_long_register)
{
int     r1, r2;                         /* Values of R fields        */
U64     n;

    RRE(inst, regs, r1, r2);

    ODD_CHECK(r1, regs);

    if(regs->GR_G(r2) == 0
      || ((S64)regs->GR_G(r2) == -1LL &&
          regs->GR_G(r1 + 1) == 0x8000000000000000ULL))
        regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);

    n = regs->GR_G(r2);

    /* Divide signed registers */
    regs->GR_G(r1) = (S64)regs->GR_G(r1 + 1) % (S64)n;
    regs->GR_G(r1 + 1) = (S64)regs->GR_G(r1 + 1) / (S64)n;

} /* end DEF_INST(divide_single_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B91D DSGFR - Divide Single Long Fullword Register           [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_single_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */
U32     n;

    RRE(inst, regs, r1, r2);

    ODD_CHECK(r1, regs);

    if(regs->GR_L(r2) == 0
      || ((S32)regs->GR_L(r2) == -1 &&
          regs->GR_G(r1 + 1) == 0x8000000000000000ULL))
        regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);

    n = regs->GR_L(r2);

    /* Divide signed registers */
    regs->GR_G(r1) = (S64)regs->GR_G(r1 + 1) % (S32)n;
    regs->GR_G(r1 + 1) = (S64)regs->GR_G(r1 + 1) / (S32)n;

} /* end DEF_INST(divide_single_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E390 LLGC  - Load Logical Long Character                    [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_long_character)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    regs->GR_G(r1) = ARCH_DEP(vfetchb) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_logical_long_character) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E391 LLGH  - Load Logical Long Halfword                     [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_long_halfword)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    regs->GR_G(r1) = ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_logical_long_halfword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E38E STPQ  - Store Pair to Quadword                         [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_pair_to_quadword)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
QWORD   qwork;                          /* Quadword work area        */

    RXY(inst, regs, r1, b2, effective_addr2);

    ODD_CHECK(r1, regs);

    QW_CHECK(effective_addr2, regs);

    /* Store regs in workarea */
    STORE_DW(qwork, regs->GR_G(r1));
    STORE_DW(qwork+8, regs->GR_G(r1+1));

    /* Store R1 and R1+1 registers to second operand
       Provide storage consistancy by means of obtaining
       the main storage access lock */
    OBTAIN_MAINLOCK(regs);
    ARCH_DEP(vstorec) ( qwork, 16-1, effective_addr2, b2, regs );
    RELEASE_MAINLOCK(regs);

} /* end DEF_INST(store_pair_to_quadword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E38F LPQ   - Load Pair from Quadword                        [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_pair_from_quadword)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
QWORD   qwork;                          /* Quadword work area        */

    RXY(inst, regs, r1, b2, effective_addr2);

    ODD_CHECK(r1, regs);

    QW_CHECK(effective_addr2, regs);

    /* Load R1 and R1+1 registers contents from second operand
       Provide storage consistancy by means of obtaining
       the main storage access lock */
    OBTAIN_MAINLOCK(regs);
    ARCH_DEP(vfetchc) ( qwork, 16-1, effective_addr2, b2, regs );
    RELEASE_MAINLOCK(regs);

    /* Load regs from workarea */
    FETCH_DW(regs->GR_G(r1), qwork);
    FETCH_DW(regs->GR_G(r1+1), qwork+8);

} /* end DEF_INST(load_pair_from_quadword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B90E EREGG - Extract Stacked Registers Long                 [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(extract_stacked_registers_long)
{
int     r1, r2;                         /* Values of R fields        */
LSED    lsed;                           /* Linkage stack entry desc. */
VADR    lsea;                           /* Linkage stack entry addr  */

    RRE(inst, regs, r1, r2);

    SIE_XC_INTERCEPT(regs);

    /* Find the virtual address of the entry descriptor
       of the current state entry in the linkage stack */
    lsea = ARCH_DEP(locate_stack_entry) (0, &lsed, regs);

    /* Load registers from the stack entry */
    ARCH_DEP(unstack_registers) (1, lsea, r1, r2, regs);

} /* end DEF_INST(extract_stacked_registers_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B98D EPSW  - Extract PSW                                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(extract_psw)
{
int     r1, r2;                         /* Values of R fields        */
QWORD   currpsw;                        /* Work area for PSW         */

    RRE(inst, regs, r1, r2);

#if defined(_FEATURE_ZSIE)
    if(SIE_STATB(regs, IC1, LPSW))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_ZSIE)*/

    /* Store the current PSW in work area */
    ARCH_DEP(store_psw) (regs, currpsw);

    /* Load PSW bits 0-31 into bits 32-63 of the R1 register */
    FETCH_FW(regs->GR_L(r1), currpsw);

    /* If R2 specifies a register other than register zero,
       load PSW bits 32-63 into bits 32-63 of the R2 register */
    if(r2 != 0)
    {
        FETCH_FW(regs->GR_L(r2), currpsw+4);

#if !defined(FEATURE_ESAME)
        /* The Ninth Edition of ESA/390 POP (SA22-7201-08) requires
           the low 31 bits to be set to zeroes in ESA/390 mode */
        regs->GR_L(r2) &= 0x80000000;
#endif /*!defined(FEATURE_ESAME)*/
    }

} /* end DEF_INST(extract_psw) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B99D ESEA  - Extract and Set Extended Authority             [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(extract_and_set_extended_authority)
{
int     r1, unused;                     /* Value of R field          */

    RRE(inst, regs, r1, unused);

    PRIV_CHECK(regs);

#if 0
#if defined(_FEATURE_ZSIE)
    if(SIE_STATB(regs, LCTL1, CR8))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_ZSIE)*/
#endif

    regs->GR_LHH(r1) = regs->CR_LHH(8);
    regs->CR_LHH(8) = regs->GR_LHL(r1);

} /* end DEF_INST(extract_and_set_extended_authority) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C0x0 LARL  - Load Address Relative Long                     [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(load_address_relative_long)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand values     */

    RIL0(inst, regs, r1, opcd, i2);

    SET_GR_A(r1, regs, likely(!regs->execflag)
                     ? PSW_IA(regs, -6 + 2LL*(S32)i2)
                     : (regs->ET + 2LL*(S32)i2) & ADDRESS_MAXWRAP(regs));

} /* end DEF_INST(load_address_relative_long) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5x0 IIHH  - Insert Immediate High High                      [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_immediate_high_high)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_HHH(r1) = i2;

} /* end DEF_INST(insert_immediate_high_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5x1 IIHL  - Insert Immediate High Low                       [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_immediate_high_low)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_HHL(r1) = i2;

} /* end DEF_INST(insert_immediate_high_low) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5x2 IILH  - Insert Immediate Low High                       [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_immediate_low_high)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_LHH(r1) = i2;

} /* end DEF_INST(insert_immediate_low_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5x3 IILL  - Insert Immediate Low Low                        [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_immediate_low_low)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_LHL(r1) = i2;

} /* end DEF_INST(insert_immediate_low_low) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5x4 NIHH  - And Immediate High High                         [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(and_immediate_high_high)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_HHH(r1) &= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_HHH(r1) ? 1 : 0;

} /* end DEF_INST(and_immediate_high_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5x5 NIHL  - And Immediate High Low                          [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(and_immediate_high_low)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_HHL(r1) &= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_HHL(r1) ? 1 : 0;

} /* end DEF_INST(and_immediate_high_low) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5x6 NILH  - And Immediate Low High                          [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(and_immediate_low_high)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_LHH(r1) &= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_LHH(r1) ? 1 : 0;

} /* end DEF_INST(and_immediate_low_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5x7 NILL  - And Immediate Low Low                           [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(and_immediate_low_low)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_LHL(r1) &= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_LHL(r1) ? 1 : 0;

} /* end DEF_INST(and_immediate_low_low) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5x8 OIHH  - Or Immediate High High                          [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(or_immediate_high_high)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_HHH(r1) |= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_HHH(r1) ? 1 : 0;

} /* end DEF_INST(or_immediate_high_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5x9 OIHL  - Or Immediate High Low                           [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(or_immediate_high_low)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_HHL(r1) |= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_HHL(r1) ? 1 : 0;

} /* end DEF_INST(or_immediate_high_low) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5xA OILH  - Or Immediate Low High                           [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(or_immediate_low_high)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_LHH(r1) |= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_LHH(r1) ? 1 : 0;

} /* end DEF_INST(or_immediate_low_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5xB OILL  - Or Immediate Low Low                            [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(or_immediate_low_low)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_LHL(r1) |= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_LHL(r1) ? 1 : 0;

} /* end DEF_INST(or_immediate_low_low) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5xC LLIHH - Load Logical Immediate High High                [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_immediate_high_high)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_G(r1) = (U64)i2 << 48;

} /* end DEF_INST(load_logical_immediate_high_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5xD LLIHL - Load Logical Immediate High Low                 [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_immediate_high_low)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_G(r1) = (U64)i2 << 32;

} /* end DEF_INST(load_logical_immediate_high_low) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5xE LLILH - Load Logical Immediate Low High                 [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_immediate_low_high)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_G(r1) = (U64)i2 << 16;

} /* end DEF_INST(load_logical_immediate_low_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A5xF LLILL - Load Logical Immediate Low Low                  [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_immediate_low_low)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    regs->GR_G(r1) = (U64)i2;

} /* end DEF_INST(load_logical_immediate_low_low) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C0x4 BRCL  - Branch Relative on Condition Long              [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_relative_on_condition_long)
{
//int     r1;                             /* Register number           */
//int     opcd;                           /* Opcode                    */
//U32     i2;                             /* 32-bit operand values     */

//  RIL(inst, regs, r1, opcd, i2);

    /* Branch if R1 mask bit is set */
    if (inst[1] & (0x80 >> regs->psw.cc))
        SUCCESSFUL_RELATIVE_BRANCH_LONG(regs, 2LL*(S32)fetch_fw(inst+2));
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(branch_relative_on_condition_long) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* C0x5 BRASL - Branch Relative And Save Long                  [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_relative_and_save_long)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand values     */

    RIL_B(inst, regs, r1, opcd, i2);

#if defined(FEATURE_ESAME)
    if(regs->psw.amode64)
        regs->GR_G(r1) = PSW_IA64(regs, 6);
    else
#endif /*defined(FEATURE_ESAME)*/
    if ( regs->psw.amode )
        regs->GR_L(r1) = 0x80000000 | PSW_IA31(regs, 6);
    else
        regs->GR_L(r1) = PSW_IA24(regs, 6);

    SUCCESSFUL_RELATIVE_BRANCH_LONG(regs, 2LL*(S32)fetch_fw(inst+2));

} /* end DEF_INST(branch_relative_and_save_long) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB20 CLMH  - Compare Logical Characters under Mask High     [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_characters_under_mask_high)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i, j;                           /* Integer work areas        */
int     cc = 0;                         /* Condition code            */
BYTE    rbyte[4],                       /* Register bytes            */
        vbyte;                          /* Virtual storage byte      */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    /* Set register bytes by mask */
    i = 0;
    if (r3 & 0x8) rbyte[i++] = (regs->GR_H(r1) >> 24) & 0xFF;
    if (r3 & 0x4) rbyte[i++] = (regs->GR_H(r1) >> 16) & 0xFF;
    if (r3 & 0x2) rbyte[i++] = (regs->GR_H(r1) >>  8) & 0xFF;
    if (r3 & 0x1) rbyte[i++] = (regs->GR_H(r1)      ) & 0xFF;

    /* Perform access check if mask is 0 */
    if (!r3) ARCH_DEP(vfetchb) (effective_addr2, b2, regs);

    /* Compare byte by byte */
    for (j = 0; j < i && !cc; j++)
    {
        effective_addr2 &= ADDRESS_MAXWRAP(regs);
        vbyte = ARCH_DEP(vfetchb) (effective_addr2++, b2, regs);
        if (rbyte[j] != vbyte)
            cc = rbyte[j] < vbyte ? 1 : 2;
    }

    regs->psw.cc = cc;

} /* end DEF_INST(compare_logical_characters_under_mask_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB2C STCMH - Store Characters under Mask High               [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_characters_under_mask_high)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i;                              /* Integer work area         */
BYTE    rbyte[4];                       /* Register bytes from mask  */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    switch (r3) {

    case 15:
        /* Optimized case */
        ARCH_DEP(vstore4) (regs->GR_H(r1), effective_addr2, b2, regs);
        break;

    default:
        /* Extract value from register by mask */
        i = 0;
        if (r3 & 0x8) rbyte[i++] = (regs->GR_H(r1) >> 24) & 0xFF;
        if (r3 & 0x4) rbyte[i++] = (regs->GR_H(r1) >> 16) & 0xFF;
        if (r3 & 0x2) rbyte[i++] = (regs->GR_H(r1) >>  8) & 0xFF;
        if (r3 & 0x1) rbyte[i++] = (regs->GR_H(r1)      ) & 0xFF;

        if (i)
            ARCH_DEP(vstorec) (rbyte, i-1, effective_addr2, b2, regs);
#if defined(MODEL_DEPENDENT_STCM)
        /* If the mask is all zero, we nevertheless access one byte
           from the storage operand, because POP states that an
           access exception may be recognized on the first byte */
        else
            ARCH_DEP(validate_operand) (effective_addr2, b2, 0,
                                        ACCTYPE_WRITE, regs);
#endif
        break;

    } /* switch (r3) */

} /* end DEF_INST(store_characters_under_mask_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_EXTRACT_CPU_TIME)
/*-------------------------------------------------------------------*/
/* C8x1 ECTG  - Extract CPU Time                               [SSF] */
/*-------------------------------------------------------------------*/
DEF_INST(extract_cpu_time)
{
int     b1, b2;                         /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
VADR    effective_addr2;                /* Effective address         */
int     r3;                             /* R3 register number        */
S64     dreg;                           /* Double word workarea      */
U64     gr0, gr1;                       /* Result register workareas */

    SSF(inst, regs, b1, effective_addr1, b2, effective_addr2, r3);

#if defined(_FEATURE_SIE)
    if(SIE_STATB(regs, IC3, SPT))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_SIE)*/

    OBTAIN_INTLOCK(regs);

    /* Save the CPU timer value */
    dreg = cpu_timer(regs);

    /* Reset the cpu timer pending flag according to its value */
    if( dreg < 0 )
    {
        ON_IC_PTIMER(regs);

        /* Roll back the instruction and take the
           timer interrupt if we have a pending CPU timer
           and we are enabled for such interrupts *JJ */
        if( OPEN_IC_PTIMER(regs) )
        {
            RELEASE_INTLOCK(regs);
            UPD_PSW_IA(regs, PSW_IA(regs, likely(!regs->execflag) ? -6 :
                                                regs->exrl ? -6 : -4));
            RETURN_INTCHECK(regs);
        }
    }
    else
        OFF_IC_PTIMER(regs);

    RELEASE_INTLOCK(regs);

    /* The value of the current CPU timer is subtracted from the first
       operand and the result is placed in general register 0 */
    gr0 = ARCH_DEP(vfetch8) (effective_addr1, b1, regs) - dreg;

    /* The second operand is placed in general register 1 */
    gr1 = ARCH_DEP(vfetch8) (effective_addr2, b2, regs);

    /* The eight bytes at the third operand location replace the contents
       of general register R3. The operands are treated as unsigned 64-bit
       integers. The contents of R3 is treated according to current
       addressing mode. In AR mode, access register R3 is used. */
    regs->GR_G(r3) = ARCH_DEP(wfetch8) (regs->GR_G(r3), r3, regs);
    regs->GR_G(0) = gr0;
    regs->GR_G(1) = gr1;

    RETURN_INTCHECK(regs);
} /* end DEF_INST(extract_cpu_time) */
#endif /*defined(FEATURE_EXTRACT_CPU_TIME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB80 ICMH  - Insert Characters under Mask High              [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_characters_under_mask_high)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int    i;                               /* Integer work area         */
BYTE   vbyte[4];                        /* Fetched storage bytes     */
U32    n;                               /* Fetched value             */
static const int                        /* Length-1 to fetch by mask */
       icmhlen[16] = {0, 0, 0, 1, 0, 1, 1, 2, 0, 1, 1, 2, 1, 2, 2, 3};
static const unsigned int               /* Turn reg bytes off by mask*/
       icmhmask[16] = {0xFFFFFFFF, 0xFFFFFF00, 0xFFFF00FF, 0xFFFF0000,
                       0xFF00FFFF, 0xFF00FF00, 0xFF0000FF, 0xFF000000,
                       0x00FFFFFF, 0x00FFFF00, 0x00FF00FF, 0x00FF0000,
                       0x0000FFFF, 0x0000FF00, 0x000000FF, 0x00000000};

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    switch (r3) {

    case 15:
        /* Optimized case */
        regs->GR_H(r1) = ARCH_DEP(vfetch4) (effective_addr2, b2, regs);
        regs->psw.cc = regs->GR_H(r1) ? regs->GR_H(r1) & 0x80000000 ?
                       1 : 2 : 0;
        break;

    default:
        memset(vbyte, 0, 4);
        ARCH_DEP(vfetchc)(vbyte, icmhlen[r3], effective_addr2, b2, regs);

        /* If mask was 0 then we still had to fetch, according to POP.
           If so, set the fetched byte to 0 to force zero cc */
        if (!r3) vbyte[0] = 0;

        n = fetch_fw (vbyte);
        regs->psw.cc = n ? n & 0x80000000 ?
                       1 : 2 : 0;

        /* Turn off the reg bytes we are going to set */
        regs->GR_H(r1) &= icmhmask[r3];

        /* Set bytes one at a time according to the mask */
        i = 0;
        if (r3 & 0x8) regs->GR_H(r1) |= vbyte[i++] << 24;
        if (r3 & 0x4) regs->GR_H(r1) |= vbyte[i++] << 16;
        if (r3 & 0x2) regs->GR_H(r1) |= vbyte[i++] << 8;
        if (r3 & 0x1) regs->GR_H(r1) |= vbyte[i];
        break;

    } /* switch (r3) */

} /* end DEF_INST(insert_characters_under_mask_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EC44 BRXHG - Branch Relative on Index High Long             [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_relative_on_index_high_long)
{
register int     r1, r3;                /* Register numbers          */
S16     i2;                             /* 16-bit immediate offset   */
S64     i,j;                            /* Integer workareas         */

    RIE_B(inst, regs, r1, r3, i2);

    /* Load the increment value from the R3 register */
    i = (S64)regs->GR_G(r3);

    /* Load compare value from R3 (if R3 odd), or R3+1 (if even) */
    j = (r3 & 1) ? (S64)regs->GR_G(r3) : (S64)regs->GR_G(r3+1);

    /* Add the increment value to the R1 register */
    regs->GR_G(r1) = (S64)regs->GR_G(r1) + i;

    /* Branch if result compares high */
    if ( (S64)regs->GR_G(r1) > j )
        SUCCESSFUL_RELATIVE_BRANCH(regs, 2*i2, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(branch_relative_on_index_high_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EC45 BRXLG - Branch Relative on Index Low or Equal Long     [RIE] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_relative_on_index_low_or_equal_long)
{
register int     r1, r3;                /* Register numbers          */
S16     i2;                             /* 16-bit immediate offset   */
S64     i,j;                            /* Integer workareas         */

    RIE_B(inst, regs, r1, r3, i2);

    /* Load the increment value from the R3 register */
    i = (S64)regs->GR_G(r3);

    /* Load compare value from R3 (if R3 odd), or R3+1 (if even) */
    j = (r3 & 1) ? (S64)regs->GR_G(r3) : (S64)regs->GR_G(r3+1);

    /* Add the increment value to the R1 register */
    regs->GR_G(r1) = (S64)regs->GR_G(r1) + i;

    /* Branch if result compares low or equal */
    if ( (S64)regs->GR_G(r1) <= j )
        SUCCESSFUL_RELATIVE_BRANCH(regs, 2*i2, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(branch_relative_on_index_low_or_equal_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB44 BXHG  - Branch on Index High Long                      [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_on_index_high_long)
{
register int     r1, r3;                /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
S64     i, j;                           /* Integer work areas        */

    RSY_B(inst, regs, r1, r3, b2, effective_addr2);

    /* Load the increment value from the R3 register */
    i = (S64)regs->GR_G(r3);

    /* Load compare value from R3 (if R3 odd), or R3+1 (if even) */
    j = (r3 & 1) ? (S64)regs->GR_G(r3) : (S64)regs->GR_G(r3+1);

    /* Add the increment value to the R1 register */
    regs->GR_G(r1) = (S64)regs->GR_G(r1) + i;

    /* Branch if result compares high */
    if ( (S64)regs->GR_G(r1) > j )
        SUCCESSFUL_BRANCH(regs, effective_addr2, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(branch_on_index_high_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB45 BXLEG - Branch on Index Low or Equal Long              [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_on_index_low_or_equal_long)
{
register int     r1, r3;                /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
S64     i, j;                           /* Integer work areas        */

    RSY_B(inst, regs, r1, r3, b2, effective_addr2);

    /* Load the increment value from the R3 register */
    i = regs->GR_G(r3);

    /* Load compare value from R3 (if R3 odd), or R3+1 (if even) */
    j = (r3 & 1) ? (S64)regs->GR_G(r3) : (S64)regs->GR_G(r3+1);

    /* Add the increment value to the R1 register */
    regs->GR_G(r1) = (S64)regs->GR_G(r1) + i;

    /* Branch if result compares low or equal */
    if ( (S64)regs->GR_G(r1) <= j )
        SUCCESSFUL_BRANCH(regs, effective_addr2, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(branch_on_index_low_or_equal_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB30 CSG   - Compare and Swap Long                          [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_swap_long)
{
register int     r1, r3;                /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
BYTE   *main2;                          /* mainstor address          */
U64     old;                            /* old value                 */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    DW_CHECK(effective_addr2, regs);

    /* Perform serialization before starting operation */
    PERFORM_SERIALIZATION (regs);

    /* Get operand absolute address */
    main2 = MADDR (effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

    /* Get old value */
    old = CSWAP64(regs->GR_G(r1));

    /* Obtain main-storage access lock */
    OBTAIN_MAINLOCK(regs);

    /* Attempt to exchange the values */
    regs->psw.cc = cmpxchg8 (&old, CSWAP64(regs->GR_G(r3)), main2);

    /* Release main-storage access lock */
    RELEASE_MAINLOCK(regs);

    /* Perform serialization after completing operation */
    PERFORM_SERIALIZATION (regs);

    if (regs->psw.cc == 1)
    {
        regs->GR_G(r1) = CSWAP64(old);
#if defined(_FEATURE_ZSIE)
        if(SIE_STATB(regs, IC0, CS1))
        {
            if( !OPEN_IC_PER(regs) )
                longjmp(regs->progjmp, SIE_INTERCEPT_INST);
            else
                longjmp(regs->progjmp, SIE_INTERCEPT_INSTCOMP);
        }
        else
#endif /*defined(_FEATURE_ZSIE)*/
            if (sysblk.cpus > 1)
                sched_yield();
    }

} /* end DEF_INST(compare_and_swap_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB3E CDSG  - Compare Double and Swap Long                   [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_double_and_swap_long)
{
register int     r1, r3;                /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
BYTE   *main2;                          /* mainstor address          */
U64     old1, old2;                     /* old value                 */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    ODD2_CHECK(r1, r3, regs);

    QW_CHECK(effective_addr2, regs);

    /* Perform serialization before starting operation */
    PERFORM_SERIALIZATION (regs);

    /* Get operand mainstor address */
    main2 = MADDR (effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

    /* Get old values */
    old1 = CSWAP64(regs->GR_G(r1));
    old2 = CSWAP64(regs->GR_G(r1+1));

    /* Obtain main-storage access lock */
    OBTAIN_MAINLOCK(regs);

    /* Attempt to exchange the values */
    regs->psw.cc = cmpxchg16 (&old1, &old2,
                              CSWAP64(regs->GR_G(r3)), CSWAP64(regs->GR_G(r3+1)),
                              main2);

    /* Release main-storage access lock */
    RELEASE_MAINLOCK(regs);

    /* Perform serialization after completing operation */
    PERFORM_SERIALIZATION (regs);

    if (regs->psw.cc == 1)
    {
        regs->GR_G(r1) = CSWAP64(old1);
        regs->GR_G(r1+1) = CSWAP64(old2);
#if defined(_FEATURE_ZSIE)
        if(SIE_STATB(regs, IC0, CS1))
        {
            if( !OPEN_IC_PER(regs) )
                longjmp(regs->progjmp, SIE_INTERCEPT_INST);
            else
                longjmp(regs->progjmp, SIE_INTERCEPT_INSTCOMP);
        }
        else
#endif /*defined(_FEATURE_ZSIE)*/
            if (sysblk.cpus > 1)
                sched_yield();
    }

} /* end DEF_INST(compare_double_and_swap_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E346 BCTG  - Branch on Count Long                           [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_on_count_long)
{
register int     r1;                    /* Value of R field          */
register int     b2;                    /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY_B(inst, regs, r1, b2, effective_addr2);

    /* Subtract 1 from the R1 operand and branch if non-zero */
    if ( --(regs->GR_G(r1)) )
        SUCCESSFUL_BRANCH(regs, effective_addr2, 6);
    else
        INST_UPDATE_PSW(regs, 6, 0);

} /* end DEF_INST(branch_on_count_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B946 BCTGR - Branch on Count Long Register                  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_on_count_long_register)
{
register int     r1, r2;                /* Values of R fields        */
VADR    newia;                          /* New instruction address   */

    RRE_B(inst, regs, r1, r2);

    /* Compute the branch address from the R2 operand */
    newia = regs->GR_G(r2);

    /* Subtract 1 from the R1 operand and branch if result
           is non-zero and R2 operand is not register zero */
    if ( --(regs->GR_G(r1)) && r2 != 0 )
        SUCCESSFUL_BRANCH(regs, newia, 2);
    else
        INST_UPDATE_PSW(regs, 4, 0);

} /* end DEF_INST(branch_on_count_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B920 CGR   - Compare Long Register                          [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_long_register)
{
register int     r1, r2;                /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Compare signed operands and set condition code */
    regs->psw.cc =
                (S64)regs->GR_G(r1) < (S64)regs->GR_G(r2) ? 1 :
                (S64)regs->GR_G(r1) > (S64)regs->GR_G(r2) ? 2 : 0;

} /* end DEF_INST(compare_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B930 CGFR  - Compare Long Fullword Register                 [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Compare signed operands and set condition code */
    regs->psw.cc =
                (S64)regs->GR_G(r1) < (S32)regs->GR_L(r2) ? 1 :
                (S64)regs->GR_G(r1) > (S32)regs->GR_L(r2) ? 2 : 0;

} /* end DEF_INST(compare_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E320 CG    - Compare Long                                   [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_long)
{
register int     r1;                    /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            (S64)regs->GR_G(r1) < (S64)n ? 1 :
            (S64)regs->GR_G(r1) > (S64)n ? 2 : 0;

} /* end DEF_INST(compare_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E330 CGF   - Compare Long Fullword                          [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_long_fullword)
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
            (S64)regs->GR_G(r1) < (S32)n ? 1 :
            (S64)regs->GR_G(r1) > (S32)n ? 2 : 0;

} /* end DEF_INST(compare_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E30A ALG   - Add Logical Long                               [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Add unsigned operands and set condition code */
    regs->psw.cc = add_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      n);

} /* end DEF_INST(add_logical_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E31A ALGF  - Add Logical Long Fullword                      [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_long_fullword)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Add unsigned operands and set condition code */
    regs->psw.cc = add_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      n);

} /* end DEF_INST(add_logical_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E318 AGF   - Add Long Fullword                              [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(add_long_fullword)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Add signed operands and set condition code */
    regs->psw.cc = add_signed_long (&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                 (S32)n);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E308 AG    - Add Long                                       [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(add_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Add signed operands and set condition code */
    regs->psw.cc = add_signed_long (&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      n);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E30B SLG   - Subtract Logical Long                          [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      n);

} /* end DEF_INST(subtract_logical_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E31B SLGF  - Subtract Logical Long Fullword                 [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_long_fullword)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      n);

} /* end DEF_INST(subtract_logical_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E319 SGF   - Subtract Long Fullword                         [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_long_fullword)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Subtract signed operands and set condition code */
    regs->psw.cc = sub_signed_long(&(regs->GR_G(r1)),
                                     regs->GR_G(r1),
                                (S32)n);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(subtract_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E309 SG    - Subtract Long                                  [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Subtract signed operands and set condition code */
    regs->psw.cc = sub_signed_long(&(regs->GR_G(r1)),
                                     regs->GR_G(r1),
                                     n);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(subtract_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B909 SGR   - Subtract Long Register                         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    /* Subtract signed operands and set condition code */
    regs->psw.cc = sub_signed_long(&(regs->GR_G(r1)),
                                     regs->GR_G(r1),
                                     regs->GR_G(r2));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(subtract_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B919 SGFR  - Subtract Long Fullword Register                [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    /* Subtract signed operands and set condition code */
    regs->psw.cc = sub_signed_long(&(regs->GR_G(r1)),
                                     regs->GR_G(r1),
                                (S32)regs->GR_L(r2));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(subtract_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B908 AGR   - Add Long Register                              [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(add_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    /* Add signed operands and set condition code */
    regs->psw.cc = add_signed_long(&(regs->GR_G(r1)),
                                     regs->GR_G(r1),
                                     regs->GR_G(r2));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B918 AGFR  - Add Long Fullword Register                     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(add_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    /* Add signed operands and set condition code */
    regs->psw.cc = add_signed_long(&(regs->GR_G(r1)),
                                     regs->GR_G(r1),
                                (S32)regs->GR_L(r2));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B900 LPGR  - Load Positive Long Register                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_positive_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    /* Condition code 3 and program check if overflow */
    if ( regs->GR_G(r2) == 0x8000000000000000ULL )
    {
        regs->GR_G(r1) = regs->GR_G(r2);
        regs->psw.cc = 3;
        if ( FOMASK(&regs->psw) )
            regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);
        return;
    }

    /* Load positive value of second operand and set cc */
    regs->GR_G(r1) = (S64)regs->GR_G(r2) < 0 ?
                            -((S64)regs->GR_G(r2)) :
                            (S64)regs->GR_G(r2);

    regs->psw.cc = (S64)regs->GR_G(r1) == 0 ? 0 : 2;

} /* end DEF_INST(load_positive_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B910 LPGFR - Load Positive Long Fullword Register           [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_positive_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */
S64     gpr2l;

    RRE0(inst, regs, r1, r2);

    gpr2l = (S32)regs->GR_L(r2);

    /* Load positive value of second operand and set cc */
    regs->GR_G(r1) = gpr2l < 0 ? -gpr2l : gpr2l;

    regs->psw.cc = (S64)regs->GR_G(r1) == 0 ? 0 : 2;

} /* end DEF_INST(load_positive_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B901 LNGR  - Load Negative Long Register                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_negative_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Load negative value of second operand and set cc */
    regs->GR_G(r1) = (S64)regs->GR_G(r2) > 0 ?
                            -((S64)regs->GR_G(r2)) :
                            (S64)regs->GR_G(r2);

    regs->psw.cc = (S64)regs->GR_G(r1) == 0 ? 0 : 1;

} /* end DEF_INST(load_negative_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B911 LNGFR - Load Negative Long Fullword Register           [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_negative_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */
S64     gpr2l;

    RRE0(inst, regs, r1, r2);

    gpr2l = (S32)regs->GR_L(r2);

    /* Load negative value of second operand and set cc */
    regs->GR_G(r1) = gpr2l > 0 ? -gpr2l : gpr2l;

    regs->psw.cc = (S64)regs->GR_G(r1) == 0 ? 0 : 1;

} /* end DEF_INST(load_negative_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B902 LTGR  - Load and Test Long Register                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_test_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Copy second operand and set condition code */
    regs->GR_G(r1) = regs->GR_G(r2);

    regs->psw.cc = (S64)regs->GR_G(r1) < 0 ? 1 :
                   (S64)regs->GR_G(r1) > 0 ? 2 : 0;

} /* end DEF_INST(load_and_test_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B912 LTGFR - Load and Test Long Fullword Register           [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_test_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Copy second operand and set condition code */
    regs->GR_G(r1) = (S32)regs->GR_L(r2);

    regs->psw.cc = (S64)regs->GR_G(r1) < 0 ? 1 :
                   (S64)regs->GR_G(r1) > 0 ? 2 : 0;

} /* end DEF_INST(load_and_test_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B903 LCGR  - Load Complement Long Register                  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_complement_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    /* Condition code 3 and program check if overflow */
    if ( regs->GR_G(r2) == 0x8000000000000000ULL )
    {
        regs->GR_G(r1) = regs->GR_G(r2);
        regs->psw.cc = 3;
        if ( FOMASK(&regs->psw) )
            regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);
        return;
    }

    /* Load complement of second operand and set condition code */
    regs->GR_G(r1) = -((S64)regs->GR_G(r2));

    regs->psw.cc = (S64)regs->GR_G(r1) < 0 ? 1 :
                   (S64)regs->GR_G(r1) > 0 ? 2 : 0;

} /* end DEF_INST(load_complement_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B913 LCGFR - Load Complement Long Fullword Register         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_complement_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */
S64     gpr2l;

    RRE0(inst, regs, r1, r2);

    gpr2l = (S32)regs->GR_L(r2);

    /* Load complement of second operand and set condition code */
    regs->GR_G(r1) = -gpr2l;

    regs->psw.cc = (S64)regs->GR_G(r1) < 0 ? 1 :
                   (S64)regs->GR_G(r1) > 0 ? 2 : 0;

} /* end DEF_INST(load_complement_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A7x2 TMHH  - Test under Mask High High                       [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(test_under_mask_high_high)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */
U16     h1;                             /* 16-bit operand values     */
U16     h2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    /* AND register bits 0-15 with immediate operand */
    h1 = i2 & regs->GR_HHH(r1);

    /* Isolate leftmost bit of immediate operand */
    for ( h2 = 0x8000; h2 != 0 && (h2 & i2) == 0; h2 >>= 1 );

    /* Set condition code according to result */
    regs->psw.cc =
            ( h1 == 0 ) ? 0 :           /* result all zeroes */
            ( h1 == i2) ? 3 :           /* result all ones   */
            ((h1 & h2) == 0) ? 1 :      /* leftmost bit zero */
            2;                          /* leftmost bit one  */

} /* end DEF_INST(test_under_mask_high_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A7x3 TMHL  - Test under Mask High Low                        [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(test_under_mask_high_low)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */
U16     h1;                             /* 16-bit operand values     */
U16     h2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    /* AND register bits 16-31 with immediate operand */
    h1 = i2 & regs->GR_HHL(r1);

    /* Isolate leftmost bit of immediate operand */
    for ( h2 = 0x8000; h2 != 0 && (h2 & i2) == 0; h2 >>= 1 );

    /* Set condition code according to result */
    regs->psw.cc =
            ( h1 == 0 ) ? 0 :           /* result all zeroes */
            ( h1 == i2) ? 3 :           /* result all ones   */
            ((h1 & h2) == 0) ? 1 :      /* leftmost bit zero */
            2;                          /* leftmost bit one  */

} /* end DEF_INST(test_under_mask_high_low) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A7x7 BRCTG - Branch Relative on Count Long                   [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_relative_on_count_long)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI_B(inst, regs, r1, opcd, i2);

    /* Subtract 1 from the R1 operand and branch if non-zero */
    if ( --(regs->GR_G(r1)) )
        SUCCESSFUL_RELATIVE_BRANCH(regs, 2*(S16)i2, 4);
    else
        INST_UPDATE_PSW(regs, 4, 0);
} /* end DEF_INST(branch_relative_on_count_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E321 CLG   - Compare Logical long                           [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = regs->GR_G(r1) < n ? 1 :
                   regs->GR_G(r1) > n ? 2 : 0;

} /* end DEF_INST(compare_logical_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E331 CLGF  - Compare Logical long fullword                  [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_long_fullword)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = regs->GR_G(r1) < n ? 1 :
                   regs->GR_G(r1) > n ? 2 : 0;

} /* end DEF_INST(compare_logical_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B931 CLGFR - Compare Logical Long Fullword Register         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = regs->GR_G(r1) < regs->GR_L(r2) ? 1 :
                   regs->GR_G(r1) > regs->GR_L(r2) ? 2 : 0;

} /* end DEF_INST(compare_logical_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B917 LLGTR - Load Logical Long Thirtyone Register           [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_long_thirtyone_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    regs->GR_G(r1) = regs->GR_L(r2) & 0x7FFFFFFF;

} /* end DEF_INST(load_logical_long_thirtyone_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B921 CLGR  - Compare Logical Long Register                  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = regs->GR_G(r1) < regs->GR_G(r2) ? 1 :
                   regs->GR_G(r1) > regs->GR_G(r2) ? 2 : 0;

} /* end DEF_INST(compare_logical_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB1C RLLG  - Rotate Left Single Logical Long                [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(rotate_left_single_logical_long)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
U64     n;                              /* Integer work areas        */

    RSY0(inst, regs, r1, r3, b2, effective_addr2);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Rotate and copy contents of r3 to r1 */
    regs->GR_G(r1) = (regs->GR_G(r3) << n)
                   | ((n == 0) ? 0 : (regs->GR_G(r3) >> (64 - n)));

} /* end DEF_INST(rotate_left_single_logical_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB1D RLL   - Rotate Left Single Logical                     [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(rotate_left_single_logical)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
U64     n;                              /* Integer work areas        */

    RSY0(inst, regs, r1, r3, b2, effective_addr2);

    /* Use rightmost five bits of operand address as shift count */
    n = effective_addr2 & 0x1F;

    /* Rotate and copy contents of r3 to r1 */
    regs->GR_L(r1) = (regs->GR_L(r3) << n)
                   | ((n == 0) ? 0 : (regs->GR_L(r3) >> (32 - n)));

} /* end DEF_INST(rotate_left_single_logical) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB0D SLLG  - Shift Left Single Logical Long                 [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_left_single_logical_long)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
U64     n;                              /* Integer work areas        */

    RSY0(inst, regs, r1, r3, b2, effective_addr2);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Copy contents of r3 to r1 and perform shift */
    regs->GR_G(r1) = regs->GR_G(r3) << n;

} /* end DEF_INST(shift_left_single_logical_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB0C SRLG  - Shift Right Single Logical Long                [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_right_single_logical_long)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
U64     n;                              /* Integer work areas        */

    RSY0(inst, regs, r1, r3, b2, effective_addr2);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Copy contents of r3 to r1 and perform shift */
    regs->GR_G(r1) = regs->GR_G(r3) >> n;

} /* end DEF_INST(shift_right_single_logical_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB0B SLAG  - Shift Left Single Long                         [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_left_single_long)
{
U32     r1, r3;                         /* Register numbers          */
U32     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
U64     n, n1, n2;                      /* 64-bit operand values     */
U32     i, j;                           /* Integer work areas        */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Load the numeric and sign portions from the R3 register */
    n1 = regs->GR_G(r3) & 0x7FFFFFFFFFFFFFFFULL;
    n2 = regs->GR_G(r3) & 0x8000000000000000ULL;

    /* Shift the numeric portion left n positions */
    for (i = 0, j = 0; i < n; i++)
    {
        /* Shift bits 1-63 left one bit position */
        n1 <<= 1;

        /* Overflow if bit shifted out is unlike the sign bit */
        if ((n1 & 0x8000000000000000ULL) != n2)
            j = 1;
    }

    /* Load the updated value into the R1 register */
    regs->GR_G(r1) = (n1 & 0x7FFFFFFFFFFFFFFFULL) | n2;

    /* Condition code 3 and program check if overflow occurred */
    if (j)
    {
        regs->psw.cc = 3;
        if ( FOMASK(&regs->psw) )
            regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);
        return;
    }

    /* Set the condition code */
    regs->psw.cc = (S64)regs->GR_G(r1) > 0 ? 2 :
                   (S64)regs->GR_G(r1) < 0 ? 1 : 0;

} /* end DEF_INST(shift_left_single_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB0A SRAG  - Shift Right Single Long                        [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_right_single_long)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
U64     n;                              /* Integer work areas        */

    RSY0(inst, regs, r1, r3, b2, effective_addr2);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Copy and shift the signed value of the R3 register */
    regs->GR_G(r1) = (n > 62) ?
                    ((S64)regs->GR_G(r3) < 0 ? -1LL : 0) :
                    (S64)regs->GR_G(r3) >> n;

    /* Set the condition code */
    regs->psw.cc = (S64)regs->GR_G(r1) > 0 ? 2 :
                   (S64)regs->GR_G(r1) < 0 ? 1 : 0;

} /* end DEF_INST(shift_right_single_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E31C MSGF  - Multiply Single Long Fullword                  [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_single_long_fullword)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Multiply signed operands ignoring overflow */
    regs->GR_G(r1) = (S64)regs->GR_G(r1) * (S32)n;

} /* end DEF_INST(multiply_single_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E30C MSG   - Multiply Single Long                           [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_single_long)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Multiply signed operands ignoring overflow */
    regs->GR_G(r1) = (S64)regs->GR_G(r1) * (S64)n;

} /* end DEF_INST(multiply_single_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B91C MSGFR - Multiply Single Long Fullword Register         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_single_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Multiply signed registers ignoring overflow */
    regs->GR_G(r1) = (S64)regs->GR_G(r1) * (S32)regs->GR_L(r2);

} /* end DEF_INST(multiply_single_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B90C MSGR  - Multiply Single Long Register                  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_single_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Multiply signed registers ignoring overflow */
    regs->GR_G(r1) = (S64)regs->GR_G(r1) * (S64)regs->GR_G(r2);

} /* end DEF_INST(multiply_single_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A7x9 LGHI  - Load Long Halfword Immediate                    [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(load_long_halfword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI0(inst, regs, r1, opcd, i2);

    /* Load operand into register */
    regs->GR_G(r1) = (S16)i2;

} /* end DEF_INST(load_long_halfword_immediate) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A7xB AGHI  - Add Long Halfword Immediate                     [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(add_long_halfword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit immediate op       */

    RI(inst, regs, r1, opcd, i2);

    /* Add signed operands and set condition code */
    regs->psw.cc = add_signed_long(&(regs->GR_G(r1)),
                                     regs->GR_G(r1),
                                (S16)i2);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_long_halfword_immediate) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A7xD MGHI  - Multiply Long Halfword Immediate                [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_long_halfword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand            */

    RI0(inst, regs, r1, opcd, i2);

    /* Multiply register by operand ignoring overflow  */
    regs->GR_G(r1) = (S64)regs->GR_G(r1) * (S16)i2;

} /* end DEF_INST(multiply_long_halfword_immediate) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* A7xF CGHI  - Compare Long Halfword Immediate                 [RI] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_long_halfword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand            */

    RI0(inst, regs, r1, opcd, i2);

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            (S64)regs->GR_G(r1) < (S16)i2 ? 1 :
            (S64)regs->GR_G(r1) > (S16)i2 ? 2 : 0;

} /* end DEF_INST(compare_long_halfword_immediate) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B980 NGR   - And Register Long                              [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(and_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* AND second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_G(r1) &= regs->GR_G(r2) ) ? 1 : 0;

} /* end DEF_INST(and_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B981 OGR   - Or Register Long                               [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(or_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* OR second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_G(r1) |= regs->GR_G(r2) ) ? 1 : 0;

} /* end DEF_INST(or_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B982 XGR   - Exclusive Or Register Long                     [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(exclusive_or_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* XOR second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_G(r1) ^= regs->GR_G(r2) ) ? 1 : 0;

} /* end DEF_INST(exclusive_or_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E380 NG    - And Long                                       [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(and_long)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* AND second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_G(r1) &= n ) ? 1 : 0;

} /* end DEF_INST(and_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E381 OG    - Or Long                                        [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(or_long)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* OR second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_G(r1) |= n ) ? 1 : 0;

} /* end DEF_INST(or_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E382 XG    - Exclusive Or Long                              [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(exclusive_or_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     n;                              /* 64-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* XOR second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_G(r1) ^= n ) ? 1 : 0;

} /* end DEF_INST(exclusive_or_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B904 LGR   - Load Long Register                             [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Copy second operand to first operand */
    regs->GR_G(r1) = regs->GR_G(r2);

} /* end DEF_INST(load_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B916 LLGFR - Load Logical Long Fullword Register            [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Copy second operand to first operand */
    regs->GR_G(r1) = regs->GR_L(r2);

} /* end DEF_INST(load_logical_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B914 LGFR  - Load Long Fullword Register                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Copy second operand to first operand */
    regs->GR_G(r1) = (S32)regs->GR_L(r2);

} /* end DEF_INST(load_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B90A ALGR  - Add Logical Register Long                      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Add unsigned operands and set condition code */
    regs->psw.cc = add_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      regs->GR_G(r2));

} /* end DEF_INST(add_logical_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B91A ALGFR - Add Logical Long Fullword Register             [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Add unsigned operands and set condition code */
    regs->psw.cc = add_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      regs->GR_L(r2));

} /* end DEF_INST(add_logical_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B91B SLGFR - Subtract Logical Long Fullword Register        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_long_fullword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      regs->GR_L(r2));

} /* end DEF_INST(subtract_logical_long_fullword_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B90B SLGR  - Subtract Logical Register Long                 [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      regs->GR_G(r2));

} /* end DEF_INST(subtract_logical_long_register) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EF   LMD   - Load Multiple Disjoint                          [SS] */
/*-------------------------------------------------------------------*/
DEF_INST(load_multiple_disjoint)
{
int     r1, r3;                         /* Register numbers          */
int     b2, b4;                         /* Base register numbers     */
VADR    effective_addr2;                /* Operand2 address          */
VADR    effective_addr4;                /* Operand4 address          */
int     i, n;                           /* Integer work areas        */
U32     rwork1[16], rwork2[16];         /* Intermediate work areas   */

    SS(inst, regs, r1, r3, b2, effective_addr2, b4, effective_addr4);

    n = ((r3 - r1) & 0xF) + 1;

    ARCH_DEP(vfetchc) (rwork1, (n * 4) - 1, effective_addr2, b2, regs);
    ARCH_DEP(vfetchc) (rwork2, (n * 4) - 1, effective_addr4, b4, regs);

    /* Load a register at a time */
    for (i = 0; i < n; i++)
    {
        regs->GR_H((r1 + i) & 0xF) = fetch_fw(&rwork1[i]);
        regs->GR_L((r1 + i) & 0xF) = fetch_fw(&rwork2[i]);
    }

} /* end DEF_INST(load_multiple_disjoint) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB96 LMH   - Load Multiple High                             [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_multiple_high)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i, m, n;                        /* Integer work areas        */
U32    *p1, *p2;                        /* Mainstor pointers         */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    /* Calculate number of bytes to load */
    n = (((r3 - r1) & 0xF) + 1) << 2;

    /* Calculate number of bytes to next boundary */
    m = 0x800 - ((VADR_L)effective_addr2 & 0x7ff);

    /* Address of operand beginning */
    p1 = (U32*)MADDR(effective_addr2, b2, regs, ACCTYPE_READ, regs->psw.pkey);

    if (likely(n <= m))
    {
        /* Boundary not crossed */
        n >>= 2;
        for (i = 0; i < n; i++, p1++)
            regs->GR_H((r1 + i) & 0xF) = fetch_fw (p1);
    }
    else
    {
        /* Boundary crossed, get 2nd page address */
        effective_addr2 += m;
        effective_addr2 &= ADDRESS_MAXWRAP(regs);
        p2 = (U32*)MADDR(effective_addr2, b2, regs, ACCTYPE_READ, regs->psw.pkey);

        if (likely((m & 0x3) == 0))
        {
            /* Addresses are word aligned */
            m >>= 2;
            for (i = 0; i < m; i++, p1++)
                regs->GR_H((r1 + i) & 0xF) = fetch_fw (p1);
            n >>= 2;
            for ( ; i < n; i++, p2++)
                regs->GR_H((r1 + i) & 0xF) = fetch_fw (p2);
        }
        else
        {
            /* Worst case */
            U32 rwork[16];
            BYTE *b1, *b2;

            b1 = (BYTE *)&rwork[0];
            b2 = (BYTE *)p1;
            for (i = 0; i < m; i++)
                *b1++ = *b2++;
            b2 = (BYTE *)p2;
            for ( ; i < n; i++)
                *b1++ = *b2++;

            n >>= 2;
            for (i = 0; i < n; i++)
                regs->GR_H((r1 + i) & 0xF) = CSWAP32(rwork[i]);
        }
    }

} /* end DEF_INST(load_multiple_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB04 LMG   - Load Multiple Long                             [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_multiple_long)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     i, m, n;                        /* Integer work areas        */
U64    *p1, *p2;                        /* Mainstor pointers         */
BYTE   *bp1;                            /* Unaligned Mainstor ptr    */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    /* Calculate number of bytes to load */
    n = (((r3 - r1) & 0xF) + 1) << 3;

    /* Calculate number of bytes to next boundary */
    m = 0x800 - ((VADR_L)effective_addr2 & 0x7ff);

    /* Address of operand beginning */
    bp1 = (BYTE*)MADDR(effective_addr2, b2, regs, ACCTYPE_READ, regs->psw.pkey);
    p1=(U64*)bp1;

    if (likely(n <= m))
    {
        /* Boundary not crossed */
        n >>= 3;
#if defined(OPTION_STRICT_ALIGNMENT)
        if(likely(!(((uintptr_t)effective_addr2)&0x07)))
        {
#endif
#if defined(OPTION_SINGLE_CPU_DW) && defined(ASSIST_STORE_DW)
            if (regs->cpubit == regs->sysblk->started_mask)
                for (i = 0; i < n; i++, p1++)
                    regs->GR_G((r1 + i) & 0xF) = CSWAP64(*p1);
            else
#endif
            for (i = 0; i < n; i++, p1++)
                regs->GR_G((r1 + i) & 0xF) = fetch_dw (p1);
#if defined(OPTION_STRICT_ALIGNMENT)
        }
        else
        {
            for (i = 0; i < n; i++, bp1+=8)
                regs->GR_G((r1 + i) & 0xF) = fetch_dw (bp1);
        }
#endif
    }
    else
    {
        /* Boundary crossed, get 2nd page address */
        effective_addr2 += m;
        effective_addr2 &= ADDRESS_MAXWRAP(regs);
        p2 = (U64*)MADDR(effective_addr2, b2, regs, ACCTYPE_READ, regs->psw.pkey);

        if (likely((m & 0x7) == 0))
        {
            /* FIXME: This code blows up on at least Mac OS X Snow Leopard
               (10.6) when compiled for a 32-bit Intel host using gcc 4.2.1
               unless the PTT call below is present. The problem appears to
               be in the gcc 4.2.1 optimizer, as the code works when
               compiled with -O0. DO NOT REMOVE this until it's been found
               and fixed. -- JRM, 11 Feb 2010 */
//            PTT_INF("LMG2KIN",p2,0,0);
            /* Addresses are double-word aligned */
            m >>= 3;
            for (i = 0; i < m; i++, p1++)
                regs->GR_G((r1 + i) & 0xF) = fetch_dw (p1);
            n >>= 3;
            for ( ; i < n; i++, p2++)
                regs->GR_G((r1 + i) & 0xF) = fetch_dw (p2);
        }
        else
        {
            /* Worst case */
            U64 rwork[16];
            BYTE *b1, *b2;

            b1 = (BYTE *)&rwork[0];
            b2 = (BYTE *)p1;
            for (i = 0; i < m; i++)
                *b1++ = *b2++;
            b2 = (BYTE *)p2;
            for ( ; i < n; i++)
                *b1++ = *b2++;

            n >>= 3;
            for (i = 0; i < n; i++)
                regs->GR_G((r1 + i) & 0xF) = CSWAP64(rwork[i]);
        }
    }

} /* end DEF_INST(load_multiple_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB25 STCTG - Store Control Long                             [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_control_long)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     i, m, n;                        /* Integer work areas        */
U64    *p1, *p2 = NULL;                 /* Mainstor pointers         */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    PRIV_CHECK(regs);

    DW_CHECK(effective_addr2, regs);

#if defined(_FEATURE_ZSIE)
    if(SIE_STATB(regs, IC1, STCTL))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_ZSIE)*/

    /* Calculate number of regs to store */
    n = ((r3 - r1) & 0xF) + 1;

    /* Calculate number of double words to next boundary */
    m = (0x800 - (effective_addr2 & 0x7ff)) >> 3;

    /* Address of operand beginning */
    p1 = (U64*)MADDR(effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

    /* Get address of next page if boundary crossed */
    if (unlikely (m < n))
        p2 = (U64*)MADDR(effective_addr2 + (m*8), b2, regs, ACCTYPE_WRITE, regs->psw.pkey);
    else
        m = n;

    /* Store to first page */
    for (i = 0; i < m; i++)
        store_dw(p1++, regs->CR_G((r1 + i) & 0xF));

    /* Store to next page */
    for ( ; i < n; i++)
        store_dw(p2++, regs->CR_G((r1 + i) & 0xF));

} /* end DEF_INST(store_control_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB2F LCTLG - Load Control Long                              [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_control_long)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     i, m, n;                        /* Integer work areas        */
U64    *p1, *p2 = NULL;                 /* Mainstor pointers         */
U16     updated = 0;                    /* Updated control regs      */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    PRIV_CHECK(regs);

    DW_CHECK(effective_addr2, regs);

    /* Calculate number of regs to load */
    n = ((r3 - r1) & 0xF) + 1;

#if defined(_FEATURE_ZSIE)
    if ( SIE_MODE(regs) )
    {
        U16 cr_mask = fetch_hw (regs->siebk->lctl_ctl);
        for (i = 0; i < n; i++)
            if (cr_mask & BIT(15 - ((r1 + i) & 0xF)))
                longjmp(regs->progjmp, SIE_INTERCEPT_INST);
    }
#endif

    /* Calculate number of double words to next boundary */
    m = (0x800 - (effective_addr2 & 0x7ff)) >> 3;

    /* Address of operand beginning */
    p1 = (U64*)MADDR(effective_addr2, b2, regs, ACCTYPE_READ, regs->psw.pkey);

    /* Get address of next page if boundary crossed */
    if (unlikely (m < n))
        p2 = (U64*)MADDR(effective_addr2 + (m*8), b2, regs, ACCTYPE_READ, regs->psw.pkey);
    else
        m = n;

    /* Load from first page */
    for (i = 0; i < m; i++, p1++)
    {
        regs->CR_G((r1 + i) & 0xF) = fetch_dw(p1);
        updated |= BIT((r1 + i) & 0xF);
    }

    /* Load from next page */
    for ( ; i < n; i++, p2++)
    {
        regs->CR_G((r1 + i) & 0xF) = fetch_dw(p2);
        updated |= BIT((r1 + i) & 0xF);
    }

    /* Actions based on updated control regs */
    SET_IC_MASK(regs);
    if (updated & (BIT(1) | BIT(7) | BIT(13)))
        SET_AEA_COMMON(regs);
    if (updated & BIT(regs->AEA_AR(USE_INST_SPACE)))
        INVALIDATE_AIA(regs);
    if (updated & BIT(9))
    {
        OBTAIN_INTLOCK(regs);
        SET_IC_PER(regs);
        RELEASE_INTLOCK(regs);
        if (EN_IC_PER_SA(regs))
            ARCH_DEP(invalidate_tlb)(regs,~(ACC_WRITE|ACC_CHECK));
    }

    RETURN_INTCHECK(regs);

} /* end DEF_INST(load_control_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB24 STMG  - Store Multiple Long                            [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_multiple_long)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     i, m, n;                        /* Integer work areas        */
U64    *p1, *p2;                        /* Mainstor pointers         */
BYTE   *bp1;                            /* Unaligned Mainstor ptr    */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    /* Calculate number of bytes to store */
    n = (((r3 - r1) & 0xF) + 1) << 3;

    /* Calculate number of bytes to next boundary */
    m = 0x800 - ((VADR_L)effective_addr2 & 0x7ff);

    /* Get address of first page */
    bp1 = (BYTE*)MADDR(effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);
    p1=(U64*)bp1;

    if (likely(n <= m))
    {
        /* Boundary not crossed */
        n >>= 3;
#if defined(OPTION_STRICT_ALIGNMENT)
        if(likely(!(((uintptr_t)effective_addr2)&0x07)))
        {
#endif
#if defined(OPTION_SINGLE_CPU_DW) && defined(ASSIST_STORE_DW)
        if (regs->cpubit == regs->sysblk->started_mask)
            for (i = 0; i < n; i++)
                *p1++ = CSWAP64(regs->GR_G((r1 + i) & 0xF));
        else
#endif
            for (i = 0; i < n; i++)
                store_dw (p1++, regs->GR_G((r1 + i) & 0xF));
#if defined(OPTION_STRICT_ALIGNMENT)
        }
        else
        {
            for (i = 0; i < n; i++,bp1+=8)
                store_dw (bp1, regs->GR_G((r1 + i) & 0xF));
        }
#endif
    }
    if (likely(n <= m))
    {
        /* boundary not crossed */
        n >>= 3;
    }
    else
    {
        /* boundary crossed, get address of the 2nd page */
        effective_addr2 += m;
        effective_addr2 &= ADDRESS_MAXWRAP(regs);
        p2 = (U64*)MADDR(effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

        if (likely((m & 0x7) == 0))
        {
            /* double word aligned */
            m >>= 3;
            for (i = 0; i < m; i++)
                store_dw (p1++, regs->GR_G((r1 + i) & 0xF));
            n >>= 3;
            for ( ; i < n; i++)
                store_dw (p2++, regs->GR_G((r1 + i) & 0xF));
        }
        else
        {
            /* worst case */
            U64 rwork[16];
            BYTE *b1, *b2;

            for (i = 0; i < (n >> 3); i++)
                rwork[i] = CSWAP64(regs->GR_G((r1 + i) & 0xF));
            b1 = (BYTE *)&rwork[0];

            b2 = (BYTE *)p1;
            for (i = 0; i < m; i++)
                *b2++ = *b1++;

            b2 = (BYTE *)p2;
            for ( ; i < n; i++)
                *b2++ = *b1++;
        }
    }

} /* end DEF_INST(store_multiple_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* EB26 STMH  - Store Multiple High                            [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_multiple_high)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i, m, n;                        /* Integer work areas        */
U32    *p1, *p2;                        /* Mainstor pointers         */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    /* Calculate number of bytes to store */
    n = (((r3 - r1) & 0xF) + 1) << 2;

    /* Calculate number of bytes to next boundary */
    m = 0x800 - ((VADR_L)effective_addr2 & 0x7ff);

    /* Get address of first page */
    p1 = (U32*)MADDR(effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

    if (likely(n <= m))
    {
        /* boundary not crossed */
        n >>= 2;
        for (i = 0; i < n; i++)
            store_fw (p1++, regs->GR_H((r1 + i) & 0xF));
    }
    else
    {
        /* boundary crossed, get address of the 2nd page */
        effective_addr2 += m;
        effective_addr2 &= ADDRESS_MAXWRAP(regs);
        p2 = (U32*)MADDR(effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

        if (likely((m & 0x3) == 0))
        {
            /* word aligned */
            m >>= 2;
            for (i = 0; i < m; i++)
                store_fw (p1++, regs->GR_H((r1 + i) & 0xF));
            n >>= 2;
            for ( ; i < n; i++)
                store_fw (p2++, regs->GR_H((r1 + i) & 0xF));
        }
        else
        {
            /* worst case */
            U32 rwork[16];
            BYTE *b1, *b2;

            for (i = 0; i < (n >> 2); i++)
                rwork[i] = CSWAP32(regs->GR_H((r1 + i) & 0xF));
            b1 = (BYTE *)&rwork[0];

            b2 = (BYTE *)p1;
            for (i = 0; i < m; i++)
                *b2++ = *b1++;

            b2 = (BYTE *)p2;
            for ( ; i < n; i++)
                *b2++ = *b1++;
        }
    }

} /* end DEF_INST(store_multiple_high) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B905 LURAG - Load Using Real Address Long                   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_using_real_address_long)
{
int     r1, r2;                         /* Values of R fields        */
RADR    n;                              /* Unsigned work             */

    RRE(inst, regs, r1, r2);

    PRIV_CHECK(regs);

    /* R2 register contains operand real storage address */
    n = regs->GR_G(r2) & ADDRESS_MAXWRAP(regs);

    /* Program check if operand not on doubleword boundary */
    DW_CHECK(n, regs);

    /* Load R1 register from second operand */
    regs->GR_G(r1) = ARCH_DEP(vfetch8) ( n, USE_REAL_ADDR, regs );

} /* end DEF_INST(load_using_real_address_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B925 STURG - Store Using Real Address Long                  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(store_using_real_address_long)
{
int     r1, r2;                         /* Values of R fields        */
RADR    n;                              /* Unsigned work             */

    RRE(inst, regs, r1, r2);

    PRIV_CHECK(regs);

    /* R2 register contains operand real storage address */
    n = regs->GR_G(r2) & ADDRESS_MAXWRAP(regs);

    /* Program check if operand not on doubleword boundary */
    DW_CHECK(n, regs);

    /* Store R1 register at second operand location */
    ARCH_DEP(vstore8) (regs->GR_G(r1), n, USE_REAL_ADDR, regs );

#if defined(FEATURE_PER2)
    /* Storage alteration must be enabled for STURA to be recognised */
    if( EN_IC_PER_SA(regs) && EN_IC_PER_STURA(regs) )
    {
        ON_IC_PER_SA(regs) ;
        ON_IC_PER_STURA(regs) ;
    }
#endif /*defined(FEATURE_PER2)*/

} /* end DEF_INST(store_using_real_address_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* 010B TAM   - Test Addressing Mode                             [E] */
/*-------------------------------------------------------------------*/
DEF_INST(test_addressing_mode)
{
    E(inst, regs);

    UNREFERENCED(inst);

    regs->psw.cc =
#if defined(FEATURE_ESAME)
                   (regs->psw.amode64 << 1) |
#endif /*defined(FEATURE_ESAME)*/
                                              regs->psw.amode;

} /* end DEF_INST(test_addressing_mode) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* 010C SAM24 - Set Addressing Mode 24                           [E] */
/*-------------------------------------------------------------------*/
DEF_INST(set_addressing_mode_24)
{
VADR    ia = PSW_IA(regs, 0);           /* Unupdated instruction addr*/

    E(inst, regs);

    UNREFERENCED(inst);

    /* Set the bear register */
    SET_BEAR_REG(regs, regs->bear_ip);

    /* Program check if instruction is located above 16MB */
    if (ia > 0xFFFFFFULL)
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

#if defined(FEATURE_ESAME)
    /* Add a mode trace entry when switching in/out of 64 bit mode */
    if((regs->CR(12) & CR12_MTRACE) && regs->psw.amode64)
        regs->CR(12) = ARCH_DEP(trace_ms) (0, 0, regs);
#endif /*defined(FEATURE_ESAME)*/

#if defined(FEATURE_ESAME)
    regs->psw.amode64 =
#endif /*defined(FEATURE_ESAME)*/
                        regs->psw.amode = 0;
    regs->psw.AMASK = AMASK24;

} /* end DEF_INST(set_addressing_mode_24) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* 010D SAM31 - Set Addressing Mode 31                           [E] */
/*-------------------------------------------------------------------*/
DEF_INST(set_addressing_mode_31)
{
VADR    ia = PSW_IA(regs, 0);           /* Unupdated instruction addr*/

    E(inst, regs);

    UNREFERENCED(inst);


    /* Set the bear register */
    SET_BEAR_REG(regs, regs->bear_ip);

    /* Program check if instruction is located above 2GB */
    if (ia > 0x7FFFFFFFULL)
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

#if defined(FEATURE_ESAME)
    /* Add a mode trace entry when switching in/out of 64 bit mode */
    if((regs->CR(12) & CR12_MTRACE) && regs->psw.amode64)
        regs->CR(12) = ARCH_DEP(trace_ms) (0, 0, regs);
#endif /*defined(FEATURE_ESAME)*/

#if defined(FEATURE_ESAME)
    regs->psw.amode64 = 0;
#endif /*defined(FEATURE_ESAME)*/
    regs->psw.amode = 1;
    regs->psw.AMASK = AMASK31;

} /* end DEF_INST(set_addressing_mode_31) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* 010E SAM64 - Set Addressing Mode 64                           [E] */
/*-------------------------------------------------------------------*/
DEF_INST(set_addressing_mode_64)
{
    E(inst, regs);

    UNREFERENCED(inst);

    /* Set the bear register */
    SET_BEAR_REG(regs, regs->bear_ip);

#if defined(FEATURE_ESAME)
    /* Add a mode trace entry when switching in/out of 64 bit mode */
    if((regs->CR(12) & CR12_MTRACE) && !regs->psw.amode64)
        regs->CR(12) = ARCH_DEP(trace_ms) (0, 0, regs);
#endif /*defined(FEATURE_ESAME)*/

    regs->psw.amode = regs->psw.amode64 = 1;
    regs->psw.AMASK = AMASK64;

} /* end DEF_INST(set_addressing_mode_64) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E324 STG   - Store Long                                     [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Store register contents at operand address */
    ARCH_DEP(vstore8) ( regs->GR_G(r1), effective_addr2, b2, regs );

} /* end DEF_INST(store_long) */

#ifdef OPTION_OPTINST
/*-------------------------------------------------------------------*/
/* E324 STG   - Store Long                                     [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(E3_0______24)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXYX0(inst, regs, r1, b2, effective_addr2);

    /* Store register contents at operand address */
    ARCH_DEP(vstore8) ( regs->GR_G(r1), effective_addr2, b2, regs );

} /* end DEF_INST(store_long) */

#endif /* #ifdef OPTION_OPTINST */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E502 STRAG - Store Real Address                             [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(store_real_address)
{
int     b1, b2;                         /* Values of base registers  */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);

    PRIV_CHECK(regs);

    DW_CHECK(effective_addr1, regs);

    /* Translate virtual address to real address */
    if (ARCH_DEP(translate_addr) (effective_addr2, b2, regs, ACCTYPE_STRAG))
        regs->program_interrupt (regs, regs->dat.xcode);

    /* Store register contents at operand address */
    ARCH_DEP(vstore8) (regs->dat.raddr, effective_addr1, b1, regs );

} /* end DEF_INST(store_real_address) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E304 LG    - Load Long                                      [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_long)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_G(r1) = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_long) */

#ifdef OPTION_OPTINST
/*-------------------------------------------------------------------*/
/* E304 LG    - Load Long                                      [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(E3_0______04)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXYX0(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_G(r1) = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_long) */

#endif /* #ifdef OPTION_OPTINST */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E314 LGF   - Load Long Fullword                             [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_long_fullword)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_G(r1) = (S32)ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E315 LGH   - Load Long Halfword                             [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_long_halfword)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_G(r1) = (S16)ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_long_halfword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E316 LLGF  - Load Logical Long Fullword                     [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_long_fullword)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_G(r1) = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_logical_long_fullword) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E317 LLGT  - Load Logical Long Thirtyone                    [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_long_thirtyone)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_G(r1) = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs )
                                                        & 0x7FFFFFFF;

} /* end DEF_INST(load_logical_long_thirtyone) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B2B2 LPSWE - Load PSW Extended                                [S] */
/*-------------------------------------------------------------------*/
DEF_INST(load_program_status_word_extended)
{
int     b2;                             /* Base of effective addr    */
U64     effective_addr2;                /* Effective address         */
QWORD   qword;
int     rc;

    S(inst, regs, b2, effective_addr2);

    PRIV_CHECK(regs);

    DW_CHECK(effective_addr2, regs);

#if defined(_FEATURE_ZSIE)
    if(SIE_STATB(regs, IC1, LPSW))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_ZSIE)*/

    /* Perform serialization and checkpoint synchronization */
    PERFORM_SERIALIZATION (regs);
    PERFORM_CHKPT_SYNC (regs);

    /* Fetch new PSW from operand address */
    ARCH_DEP(vfetchc) ( qword, 16-1, effective_addr2, b2, regs );

    /* Set the breaking event address register */
    SET_BEAR_REG(regs, regs->ip - 4);

    /* Load updated PSW */
    if ( ( rc = ARCH_DEP(load_psw) ( regs, qword ) ) )
        regs->program_interrupt (regs, rc);

    /* Perform serialization and checkpoint synchronization */
    PERFORM_SERIALIZATION (regs);
    PERFORM_CHKPT_SYNC (regs);

    RETURN_INTCHECK(regs);

} /* end DEF_INST(load_program_status_word_extended) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E303 LRAG  - Load Real Address Long                         [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_real_address_long)
{
int     r1;                             /* Register number           */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     cc;                             /* Condition code            */

    RXY(inst, regs, r1, b2, effective_addr2);

    SIE_XC_INTERCEPT(regs);

    PRIV_CHECK(regs);

    /* Translate the effective address to a real address */
    cc = ARCH_DEP(translate_addr) (effective_addr2, b2, regs, ACCTYPE_LRA);

    /* If ALET exception or ASCE-type or region translation
       exception, or if the segment table entry is outside the
       table and the entry address exceeds 2GB, set exception
       code in R1 bits 48-63, set bit 32 of R1, and set cc 3 */
    if (cc > 3
        || (cc == 3 && regs->dat.raddr > 0x7FFFFFFF))
    {
        regs->GR_L(r1) = 0x80000000 | regs->dat.xcode;
        cc = 3;
    }
    else if (cc == 3) /* && regs->dat.raddr <= 0x7FFFFFFF */
    {
        /* If segment table entry is outside table and entry
           address does not exceed 2GB, return bits 32-63 of
           the entry address and leave bits 0-31 unchanged */
        regs->GR_L(r1) = regs->dat.raddr;
    }
    else
    {
        /* Set R1 and condition code as returned by translate_addr */
        regs->GR_G(r1) = regs->dat.raddr;
    }

    /* Set condition code */
    regs->psw.cc = cc;

} /* end DEF_INST(load_real_address_long) */
#endif /*defined(FEATURE_ESAME)*/


#if defined(FEATURE_TOD_CLOCK_STEERING)
/*-------------------------------------------------------------------*/
/* 0104 PTFF  - Perform Timing Facility Function                 [E] */
/*-------------------------------------------------------------------*/
DEF_INST(perform_timing_facility_function)
{
    E(inst, regs);

    UNREFERENCED(inst);

    FACILITY_CHECK(TOD_CLOCK_STEER,regs);

    SIE_INTERCEPT(regs);

    if(regs->GR_L(0) & PTFF_GPR0_RESV)
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    switch(regs->GR_L(0) & PTFF_GPR0_FC_MASK)
    {
        case PTFF_GPR0_FC_QAF:
            ARCH_DEP(query_available_functions) (regs);
            regs->psw.cc = 0;
            break;
        case PTFF_GPR0_FC_QTO:
            ARCH_DEP(query_tod_offset) (regs);
            regs->psw.cc = 0;
            break;
        case PTFF_GPR0_FC_QSI:
            ARCH_DEP(query_steering_information) (regs);
            regs->psw.cc = 0;
            break;
        case PTFF_GPR0_FC_QPT:
            ARCH_DEP(query_physical_clock) (regs);
            regs->psw.cc = 0;
            break;
        case PTFF_GPR0_FC_ATO:
            PRIV_CHECK(regs);
            ARCH_DEP(adjust_tod_offset) (regs);
            regs->psw.cc = 0;
            break;
        case PTFF_GPR0_FC_STO:
            PRIV_CHECK(regs);
            ARCH_DEP(set_tod_offset) (regs);
            regs->psw.cc = 0;
            break;
        case PTFF_GPR0_FC_SFS:
            PRIV_CHECK(regs);
            ARCH_DEP(set_fine_s_rate) (regs);
            regs->psw.cc = 0;
            break;
        case PTFF_GPR0_FC_SGS:
            PRIV_CHECK(regs);
            ARCH_DEP(set_gross_s_rate) (regs);
            regs->psw.cc = 0;
            break;
        default:
            PTT_ERR("*PTFF",regs->GR_L(0),regs->GR_L(1),regs->psw.IA_L);
            regs->psw.cc = 3;
    }
}
#endif /*defined(FEATURE_TOD_CLOCK_STEERING)*/


#if defined(FEATURE_CONFIGURATION_TOPOLOGY_FACILITY)
/*-------------------------------------------------------------------*/
/* B9A2 PTF   - Perform Topology Function                      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(perform_topology_function)
{
int     r1, unused;                     /* Values of R fields        */
int     fc, rc = 0;                     /* Function / Reason Code    */

    RRE(inst, regs, r1, unused);

    PTT_INF("PTF",regs->GR_G(r1),0,regs->psw.IA_L);

    PRIV_CHECK(regs);

    SIE_INTERCEPT(regs);

    /* Specification Exception if bits 0-55 of general register R1
       are not zeros */
    if (regs->GR_G(r1) & 0xFFFFFFFFFFFFFF00ULL)
    {
        PTT_ERR("*PTF",regs->GR_G(r1),rc,regs->psw.IA_L);
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Extract function code */
    fc = (int)(regs->GR_G(r1) & 0x00000000000000FFULL);

    /* Perform requested function */
    switch (fc)
    {
    case 0:                     /* Request horizontal polarization */
        if (sysblk.topology == TOPOLOGY_HORIZ) {
            regs->psw.cc = 2;   /* Request rejected */
            rc = 1;             /* Already polarized as specified */
        } else {
            sysblk.topology = TOPOLOGY_HORIZ;
            sysblk.topchnge = 1;
            regs->psw.cc = 0;
            rc = 0;
        }
        break;

    case 1:                     /* Request vertical polarization */
        if (sysblk.topology == TOPOLOGY_VERT) {
            regs->psw.cc = 2;   /* Request rejected */
            rc = 1;             /* Already polarized as specified */
        } else {
            sysblk.topology = TOPOLOGY_VERT;
            sysblk.topchnge = 1;
            regs->psw.cc = 0;
            rc = 0;
        }
        break;

    case 2:                     /* Check topology-change status */
        OBTAIN_INTLOCK(regs);
        regs->psw.cc = sysblk.topchnge ? 1    /* (report was pending) */
                                       : 0;   /* (report not pending) */
        sysblk.topchnge = 0;                  /* (clear pending flag) */
        RELEASE_INTLOCK(regs);
        break;

    default:
        /* Undefined function code */
        PTT_ERR("*PTF",regs->GR_G(r1),rc,regs->psw.IA_L);
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Set reason code in bits 48-55 when condition code is 2 */
    if (regs->psw.cc == 2)
        regs->GR_G(r1) |= rc << 8;

    if (regs->psw.cc != 0)
        PTT_ERR("*PTF",regs->GR_G(r1),rc,regs->psw.IA_L);
}
#endif /*defined(FEATURE_CONFIGURATION_TOPOLOGY_FACILITY)*/


#if defined(FEATURE_RESET_REFERENCE_BITS_MULTIPLE_FACILITY)
/*-------------------------------------------------------------------*/
/* B9AE RRBM  - Reset Reference Bits Multiple                  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(reset_reference_bits_multiple)
{
int     r1, r2;                         /* Register values           */
RADR    a;                              /* Abs frame addr stor key   */
BYTE    storkey;                        /* Storage key               */
int     i;
U64     bitmap;                         /* Bitmap to be ret in r1    */

    RRE(inst, regs, r1, r2);

    FACILITY_CHECK(RES_REF_BITS_MUL,regs);

    PRIV_CHECK(regs);

    /* Load 4K block address from R2 register */
    a = regs->GR(r2) & ADDRESS_MAXWRAP_E(regs);

    /* Addressing exception if block is outside main storage */
    if ( a > regs->mainlim )
        ARCH_DEP(program_interrupt) (regs, PGM_ADDRESSING_EXCEPTION);

    /* Ignore bits 46-63 of the 2nd operand */
    a &= ~0x3ffffULL;

#if defined(_FEATURE_SIE)
    if(SIE_MODE(regs) && (SIE_STATB(regs, IC2, RRBE)))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);


    for(i =0 , bitmap = 0; i < 64; i++, a+= PAGEFRAME_PAGESIZE, bitmap <<= 1)
    {
    RADR n = a;

        if(SIE_MODE(regs))
        {
            SIE_TRANSLATE(&n, ACCTYPE_SIE, regs);

            if(regs->sie_pref)
            {
#if defined(_FEATURE_STORAGE_KEY_ASSIST)
                if((SIE_STATB(regs, RCPO0, SKA)
#if defined(_FEATURE_ZSIE)
                  || (regs->hostregs->arch_mode == ARCH_900)
#endif /*defined(_FEATURE_ZSIE)*/
                  ) && SIE_STATB(regs, RCPO2, RCPBY))
                {
#if !defined(FEATURE_2K_STORAGE_KEYS)
                    storkey = STORAGE_KEY(n, regs);
#else
                    storkey = STORAGE_KEY1(n, regs)
                            | (STORAGE_KEY2(n, regs) & (STORKEY_REF))
#endif
                                                                    ;
                        /* Reset the reference bit in the storage key */
#if !defined(FEATURE_2K_STORAGE_KEYS)
                    STORAGE_KEY(n, regs) &= ~(STORKEY_REF);
#else
                    STORAGE_KEY1(n, regs) &= ~(STORKEY_REF);
                    STORAGE_KEY2(n, regs) &= ~(STORKEY_REF);
#endif
                }
                else
#endif /*defined(_FEATURE_STORAGE_KEY_ASSIST)*/
                {
                BYTE rcpkey, realkey;
                RADR ra;
                RADR rcpa;

#if defined(_FEATURE_STORAGE_KEY_ASSIST)
                    if(SIE_STATB(regs, RCPO0, SKA)
#if defined(_FEATURE_ZSIE)
                      || (regs->hostregs->arch_mode == ARCH_900)
#endif /*defined(_FEATURE_ZSIE)*/
                                                                 )
                    {
                        /* guest absolute to host PTE addr */
                        if (SIE_TRANSLATE_ADDR (regs->sie_mso + n, USE_PRIMARY_SPACE,
                                                regs->hostregs, ACCTYPE_PTE))
                            longjmp(regs->progjmp, SIE_INTERCEPT_INST);

                        /* Convert real address to absolute address */
                        rcpa = APPLY_PREFIXING (regs->hostregs->dat.raddr, regs->hostregs->PX);

                        /* For ESA/390 the RCP byte entry is at offset 1 in a
                           four byte entry directly beyond the page table,
                           for ESAME mode, this entry is eight bytes long */
                        rcpa += regs->hostregs->arch_mode == ARCH_900 ? 2049 : 1025;
                    }
                    else
#endif /*defined(_FEATURE_STORAGE_KEY_ASSIST)*/
                    {
#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)
                        if(SIE_STATB(regs, MX, XC))
                            longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)*/

                        /* Obtain address of the RCP area from the state desc */
                        rcpa = regs->sie_rcpo &= 0x7FFFF000;

                        /* frame index as byte offset to 4K keys in RCP area */
                        rcpa += n >> 12;

                        /* host primary to host absolute */
                        rcpa = SIE_LOGICAL_TO_ABS (rcpa, USE_PRIMARY_SPACE,
                                                   regs->hostregs, ACCTYPE_SIE, 0);
                    }

                    /* fetch the RCP key */
                    rcpkey = regs->mainstor[rcpa];
                    STORAGE_KEY(rcpa, regs) |= STORKEY_REF;

                    if (!SIE_TRANSLATE_ADDR (regs->sie_mso + n, USE_PRIMARY_SPACE,
                                             regs->hostregs, ACCTYPE_SIE))
                    {
                        ra = APPLY_PREFIXING(regs->hostregs->dat.raddr, regs->hostregs->PX);
#if !defined(FEATURE_2K_STORAGE_KEYS)
                        realkey = STORAGE_KEY(ra, regs) & (STORKEY_REF);
#else
                        realkey = (STORAGE_KEY1(ra, regs) | STORAGE_KEY2(ra, regs))
                                  & (STORKEY_REF);
#endif
                        /* Reset the reference and change bits in
                           the real machine storage key */
#if !defined(FEATURE_2K_STORAGE_KEYS)
                        STORAGE_KEY(ra, regs) &= ~(STORKEY_REF);
#else
                        STORAGE_KEY1(ra, regs) &= ~(STORKEY_REF);
                        STORAGE_KEY2(ra, regs) &= ~(STORKEY_REF);
#endif
                    }
                    else
                        realkey = 0;

                    /* The storage key is obtained by logical or
                       or the real and guest RC bits */
                    storkey = realkey | (rcpkey & (STORKEY_REF));
                    /* or with host set */
                    rcpkey |= realkey << 4;
                    /* Put storage key in guest set */
                    rcpkey |= storkey;
                    /* reset the reference bit */
                    rcpkey &= ~(STORKEY_REF);
                    regs->mainstor[rcpa] = rcpkey;
                    STORAGE_KEY(rcpa, regs) |= (STORKEY_REF);
                }
            }
            else
            {
#if !defined(FEATURE_2K_STORAGE_KEYS)
                storkey = STORAGE_KEY(n, regs);
#else
                storkey = STORAGE_KEY1(n, regs)
                          | (STORAGE_KEY2(n, regs) & (STORKEY_REF))
#endif
                                    ;
                /* Reset the reference bit in the storage key */
#if !defined(FEATURE_2K_STORAGE_KEYS)
                STORAGE_KEY(n, regs) &= ~(STORKEY_REF);
#else
                STORAGE_KEY1(n, regs) &= ~(STORKEY_REF);
                STORAGE_KEY2(n, regs) &= ~(STORKEY_REF);
#endif
            }
        }
        else
#endif /*defined(_FEATURE_SIE)*/
        {
#if !defined(FEATURE_2K_STORAGE_KEYS)
            storkey = STORAGE_KEY(n, regs);
#else
            storkey = STORAGE_KEY1(n, regs)
                    | (STORAGE_KEY2(n, regs) & (STORKEY_REF))
#endif
                                                             ;
            /* Reset the reference bit in the storage key */
#if !defined(FEATURE_2K_STORAGE_KEYS)
        STORAGE_KEY(n, regs) &= ~(STORKEY_REF);
#else
        STORAGE_KEY1(n, regs) &= ~(STORKEY_REF);
        STORAGE_KEY2(n, regs) &= ~(STORKEY_REF);
#endif
        }

        /* Insert the original state of the reference bit
           in the bitmap */
        bitmap |= (storkey & STORKEY_REF) ? 0x01ULL : 0;

        /* If the storage key had the REF bit on then perform
         * accelerated lookup invalidations on all CPUs
         * so that the REF bit will be set when referenced next.
         */
        if (storkey & STORKEY_REF)
            STORKEY_INVALIDATE(regs, n);
    }

    regs->GR_G(r1) = bitmap;

} /* end DEF_INST(reset_reference_bits_multiple) */
#endif /*defined(FEATURE_RESET_REFERENCE_BITS_MULTIPLE_FACILITY)*/


#if defined(FEATURE_ENHANCED_DAT_FACILITY)
/*-------------------------------------------------------------------*/
/* SUBROUTINE TO PERFORM CONDITIONAL KEY PROCESSING is deleted       */
/*                                                                   */
/* Note  that  there  is nothing conditional about PFMF key setting; */
/* the  only  option  is  whether clear sets change and/or reference */
/* bits.                                                             */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* B9AF PFMF  - Perform Frame Management Function              [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(perform_frame_management_function)
{
int     r1, r2;                         /* Register values           */
int     fc = 1;                         /* Frame count               */
RADR    addr, aaddr;
int     page_offset;                    /* Low order bits of R2      */

    RRE(inst, regs, r1, r2);

    FACILITY_CHECK(ENHANCED_DAT,regs);

    PRIV_CHECK(regs);

    if((regs->GR_L(r1) & (PFMF_RESERVED|PFMF_FMFI_RESV|PFMF_FMFI_FSC_RESV))
      || (!FACILITY_ENABLED(NONQ_KEY_SET,regs) && (regs->GR_L(r1) & PFMF_FMFI_NQ)))
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Wrap address according to addressing mode */
    aaddr = addr = ADDRESS_MAXWRAP(regs) & regs->GR_G(r2) & PAGEFRAME_PAGEMASK;
    page_offset = regs->GR_G(r2) & PAGEFRAME_BYTEMASK;

    /* Convert real address to absolute address */

    switch (PFMF_FMFI_FSC & regs->GR_L(r1)) {
    case PFMF_FMFI_FSC_1M:
        /* Prefixing is not applied in multipage mode */
        fc = 0x100 - ((regs->GR_L(r2) & 0xFF000) >> 12);
        break;
    /* Code for 2G pages goes here */
    default:
        /* should have been catched by PFMF_FMFI_FSC_RESV bits being nonzero */
        ASSERT(0);
        break;
    case PFMF_FMFI_FSC_4K:
        /* Prefixing is applied in single frame operation */
        aaddr = addr = APPLY_PREFIXING (addr, regs->PX);
        fc = 1;
        break;
    }

    /* Note that real hardware does not update R2 for each page, only on
       interrupt or at completion. Nor does it reference R1 more than once.
       This has implications for the case of R1 == R2.
       The low order 12 bits of R2 must remain unchanged. */
    for( ; fc--; )
    {
        /* Addressing exception if block is outside main storage */
        if ( addr > regs->mainlim )
            ARCH_DEP(program_interrupt) (regs, PGM_ADDRESSING_EXCEPTION);

        if(regs->GR_L(r1) & PFMF_FMFI_CF)
            SIE_TRANSLATE(&aaddr, ACCTYPE_SIE, regs);

        /* Set Key Control */
        if(regs->GR_L(r1) & PFMF_FMFI_SK)
        {
        BYTE sk = regs->GR_L(r1) & PFMF_FMFI_KEY;
        RADR n = addr;
        BYTE rck = 0;

            /* Ref Bit must be updated */
            if(!(regs->GR_L(r1) & PFMF_FMFI_MR))
                rck |= STORKEY_REF;

            /* Change Bit must be updated */
            if((regs->GR_L(r1) & PFMF_FMFI_MC))
                rck |= STORKEY_CHANGE;

            /* Mask out R/C bits to be bypassed */
            sk &= ~rck;

            /* Addressing exception if block is outside main storage */
            if ( aaddr > regs->mainlim )
                ARCH_DEP(program_interrupt) (regs, PGM_ADDRESSING_EXCEPTION);

#if defined(_FEATURE_SIE)
            if(SIE_MODE(regs))
            {
                if(SIE_STATB(regs, IC2, SSKE))
                    longjmp(regs->progjmp, SIE_INTERCEPT_INST);

                if(!regs->sie_pref)
                {
#if defined(_FEATURE_STORAGE_KEY_ASSIST)
                    if ((SIE_STATB(regs, RCPO0, SKA)
#if defined(_FEATURE_ZSIE)
                      || (regs->hostregs->arch_mode == ARCH_900)
#endif /*defined(_FEATURE_ZSIE)*/
                      ) && SIE_STATB(regs, RCPO2, RCPBY))
                        { SIE_TRANSLATE(&aaddr, ACCTYPE_SIE, regs); }
                    else
#endif /*defined(_FEATURE_STORAGE_KEY_ASSIST)*/
                    {
                    int  sr;
                    BYTE realkey,
                         rcpkey,
                         protkey;
                    RADR rcpa;

#if defined(_FEATURE_STORAGE_KEY_ASSIST)
                        if(SIE_STATB(regs, RCPO0, SKA)
#if defined(_FEATURE_ZSIE)
                          || (regs->hostregs->arch_mode == ARCH_900)
#endif /*defined(_FEATURE_ZSIE)*/
                                                                     )
                        {
                            /* guest absolute to host PTE addr */
                            if (SIE_TRANSLATE_ADDR (regs->sie_mso + aaddr, USE_PRIMARY_SPACE,
                                                    regs->hostregs, ACCTYPE_PTE))
                                longjmp(regs->progjmp, SIE_INTERCEPT_INST);

                            /* Convert real address to absolute address */
                            rcpa = APPLY_PREFIXING (regs->hostregs->dat.raddr, regs->hostregs->PX);

                            /* For ESA/390 the RCP byte entry is at offset 1 in a
                               four byte entry directly beyond the page table,
                               for ESAME mode, this entry is eight bytes long */
                            rcpa += regs->hostregs->arch_mode == ARCH_900 ? 2049 : 1025;
                        }
                        else
#endif /*defined(_FEATURE_STORAGE_KEY_ASSIST)*/
                        {
#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)
                            if(SIE_STATB(regs, MX, XC))
                                longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)*/

                            /* Obtain address of the RCP area from the state desc */
                            rcpa = regs->sie_rcpo &= 0x7FFFF000;

                            /* frame index as byte offset to 4K keys in RCP area */
                            rcpa += aaddr >> 12;

                            /* host primary to host absolute */
                            rcpa = SIE_LOGICAL_TO_ABS (rcpa, USE_PRIMARY_SPACE,
                                               regs->hostregs, ACCTYPE_SIE, 0);
                        }

                        /* guest absolute to host real */
                        sr = SIE_TRANSLATE_ADDR (regs->sie_mso + aaddr, USE_PRIMARY_SPACE,
                                                 regs->hostregs, ACCTYPE_SIE);

                        if (sr
#if defined(_FEATURE_STORAGE_KEY_ASSIST)
                          && !(SIE_FEATB(regs, RCPO0, SKA)
#if defined(_FEATURE_ZSIE)
                            || (regs->hostregs->arch_mode == ARCH_900)
#endif /*defined(_FEATURE_ZSIE)*/
                                                                      )
#endif /*defined(_FEATURE_STORAGE_KEY_ASSIST)*/
                           )
                            longjmp(regs->progjmp, SIE_INTERCEPT_INST);

                        /* fetch the RCP key */
                        rcpkey = regs->mainstor[rcpa];
                        /* set the reference bit in the RCP key */
                        STORAGE_KEY(rcpa, regs) |= STORKEY_REF;
#if defined(_FEATURE_STORAGE_KEY_ASSIST)
                        if(sr)
                        {
                            realkey = 0;
                            protkey = rcpkey & (STORKEY_REF | STORKEY_CHANGE);
                            /* rcpa-1 is correct here - would have been SIE Intercepted otherwise */
                            protkey |= regs->mainstor[rcpa-1] & (STORKEY_KEY | STORKEY_FETCH);
                        }
                        else
#endif /*defined(_FEATURE_STORAGE_KEY_ASSIST)*/
                        {
                            /* host real to host absolute */
                            n = APPLY_PREFIXING(regs->hostregs->dat.raddr, regs->hostregs->PX);

                            protkey =
#if !defined(FEATURE_2K_STORAGE_KEYS)
                                      STORAGE_KEY(n, regs)
#else
                                      (STORAGE_KEY1(n, regs) | STORAGE_KEY2(n, regs))
#endif
                                                                                      ;
                            realkey = protkey & (STORKEY_REF | STORKEY_CHANGE);
                        }

                        /* or with host set */
                        rcpkey |= realkey << 4;
                        /* insert new settings of the guest set */
                        rcpkey &= ~(STORKEY_REF | STORKEY_CHANGE);
                        rcpkey |= sk & (STORKEY_REF | STORKEY_CHANGE);
                        regs->mainstor[rcpa] = rcpkey;
                        STORAGE_KEY(rcpa, regs) |= (STORKEY_REF|STORKEY_CHANGE);
#if defined(_FEATURE_STORAGE_KEY_ASSIST)
                        /* Insert key in new storage key */
                        if(SIE_STATB(regs, RCPO0, SKA)
#if defined(_FEATURE_ZSIE)
                            || (regs->hostregs->arch_mode == ARCH_900)
#endif /*defined(_FEATURE_ZSIE)*/
                                                                      )
                            regs->mainstor[rcpa-1] = sk
                                                    & (STORKEY_KEY | STORKEY_FETCH);
                        if(!sr)
#endif /*defined(_FEATURE_STORAGE_KEY_ASSIST)*/
                        {
#if !defined(FEATURE_2K_STORAGE_KEYS)
                            STORAGE_KEY(aaddr, regs) &= STORKEY_BADFRM;
                            STORAGE_KEY(aaddr, regs) |= sk
                                            & (STORKEY_KEY | STORKEY_FETCH);
#else
                            STORAGE_KEY1(aaddr, regs) &= STORKEY_BADFRM;
                            STORAGE_KEY1(aaddr, regs) |= sk
                                             & (STORKEY_KEY | STORKEY_FETCH);
                            STORAGE_KEY2(aaddr, regs) &= STORKEY_BADFRM;
                            STORAGE_KEY2(aaddr, regs) |= sk
                                             & (STORKEY_KEY | STORKEY_FETCH);
#endif
                        }
                    }
                }
                else
                {
                    /* Update the storage key from R1 register bits 24-30 */
#if !defined(FEATURE_2K_STORAGE_KEYS)
                    STORAGE_KEY(aaddr, regs) &= STORKEY_BADFRM;
                    STORAGE_KEY(aaddr, regs) |= sk & ~(STORKEY_BADFRM);
#else
                    STORAGE_KEY1(aaddr, regs) &= STORKEY_BADFRM;
                    STORAGE_KEY1(aaddr, regs) |= sk & ~(STORKEY_BADFRM);
                    STORAGE_KEY2(aaddr, regs) &= STORKEY_BADFRM;
                    STORAGE_KEY2(aaddr, regs) |= sk & ~(STORKEY_BADFRM);
#endif
                }
            }
            else
#endif /*defined(_FEATURE_SIE)*/
            {

                /* Update the storage key from R1 register bits 24-30 */
#if defined(FEATURE_4K_STORAGE_KEYS) && !defined(FEATURE_2K_STORAGE_KEYS)
                STORAGE_KEY(aaddr, regs) &= STORKEY_BADFRM;
                STORAGE_KEY(aaddr, regs) |= sk & ~(STORKEY_BADFRM);
#else
                STORAGE_KEY1(aaddr, regs) &= STORKEY_BADFRM;
                STORAGE_KEY1(aaddr, regs) |= sk & ~(STORKEY_BADFRM);
                STORAGE_KEY2(aaddr, regs) &= STORKEY_BADFRM;
                STORAGE_KEY2(aaddr, regs) |= sk & ~(STORKEY_BADFRM);
#endif
            }

            /* Quiesce */
            if(!(regs->GR_L(r1) & PFMF_FMFI_NQ))
            {
                /* Perform serialization and checkpoint-synchronization */
                PERFORM_SERIALIZATION (regs);
                PERFORM_CHKPT_SYNC (regs);
            }

            /* Invalidate AIA/AEA so that the REF and CHANGE bits will be set
               when referenced next */
            STORKEY_INVALIDATE(regs, n);

        }

        /* Clear Frame Control */
        if(regs->GR_L(r1) & PFMF_FMFI_CF)
            memset(regs->mainstor + aaddr, 0, PAGEFRAME_PAGESIZE);

        /* Update r2 - point to the next frame */
        switch (PFMF_FMFI_FSC & regs->GR_L(r1)) {
        case PFMF_FMFI_FSC_1M:
            aaddr = addr += 0x1000;
            SET_GR_A(r2, regs, (addr & ADDRESS_MAXWRAP(regs)) + page_offset);
            break;
        /* Code for 2G pages goes here */
        default:
            break;
        case PFMF_FMFI_FSC_4K:
            /* Leave R2 unchanged */
            break;
        }

#if 0
        /* Usage Indication */
        if(regs->GR_L(r1) & PFMF_FMFI_UI)
            { }
#endif

    }

} /* end DEF_INST(perform_frame_management_function) */
#endif /*defined(FEATURE_ENHANCED_DAT_FACILITY)*/


#if defined(FEATURE_STORE_FACILITY_LIST)
/*-------------------------------------------------------------------*/
/* B2B1 STFL  - Store Facility List                              [S] */
/*-------------------------------------------------------------------*/
DEF_INST(store_facility_list)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
PSA    *psa;                            /* -> Prefixed storage area  */

    S(inst, regs, b2, effective_addr2);

    PRIV_CHECK(regs);

#if defined(_FEATURE_SIE)
    if(SIE_STATB(regs,IC0, STFL))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_SIE)*/

    SIE_INTERCEPT(regs);

    PTT_INF("STFL",b2,(U32)(effective_addr2 & 0xffffffff),regs->psw.IA_L);

    /* Set the main storage reference and change bits */
    STORAGE_KEY(regs->PX, regs) |= (STORKEY_REF | STORKEY_CHANGE);

    /* Point to PSA in main storage */
    psa = (void*)(regs->mainstor + regs->PX);

    memcpy(psa->stfl, regs->facility_list, sizeof(psa->stfl));

} /* end DEF_INST(store_facility_list) */
#endif /*defined(FEATURE_STORE_FACILITY_LIST) */


#if defined(FEATURE_STORE_FACILITY_LIST_EXTENDED)
/*-------------------------------------------------------------------*/
/* B2B0 STFLE - Store Facility List Extended                     [S] */
/*-------------------------------------------------------------------*/
DEF_INST(store_facility_list_extended)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     ndbl;                           /* #of doublewords to store  */
int     sdbl;                           /* Supported dblwrd size     */
int     cc;                             /* Condition code            */

    S(inst, regs, b2, effective_addr2);

#if defined(_FEATURE_SIE)
    if(SIE_STATB(regs,IC0, STFL))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_SIE)*/

    PTT_INF("STFLE",regs->GR_L(0),(U32)(effective_addr2 & 0xffffffff),regs->psw.IA_L);

    /* Note: STFLE is NOT a privileged instruction (unlike STFL) */

    DW_CHECK(effective_addr2, regs);

    /* Obtain operand length from register 0 bits 56-63 */
    ndbl = regs->GR_LHLCL(0) + 1;

    /* Determine the STFLE array size from the available facilities */
    sdbl = STFL_IBMBYSIZE;
    while(--sdbl && !regs->facility_list[sdbl]);
    sdbl = (sdbl>>3)+1;

    /* Check if operand length is sufficient */
    if (ndbl >= sdbl)
    {
        ndbl = sdbl;
        cc = 0;
    }
    else
    {
        PTT_ERR("*STFLE", ndbl, sdbl, regs->psw.IA_L);
        cc = 3;
    }

#if 0
    logmsg ("STFL=%2.2X %2.2X %2.2X %2.2X\n",
            regs->facility_list[0],
            regs->facility_list[1],
            regs->facility_list[2],
            regs->facility_list[3]);
#endif

    /* Store facility list at operand location */
    ARCH_DEP(vstorec) ( regs->facility_list, ndbl*8-1,
                        effective_addr2, b2, regs );

    /* Save number of doublewords minus 1 into register 0 bits 56-63 */
    regs->GR_LHLCL(0) = (BYTE)(sdbl - 1);

    /* Set condition code */
    regs->psw.cc = cc;

} /* end DEF_INST(store_facility_list_extended) */
#endif /*defined(FEATURE_STORE_FACILITY_LIST_EXTENDED)*/


#if defined(FEATURE_LOAD_REVERSED) && defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* B90F LRVGR - Load Reversed Long Register                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_reversed_long_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Copy second operand to first operand */
    regs->GR_G(r1) = bswap_64(regs->GR_G(r2));

} /* end DEF_INST(load_reversed_long_register) */
#endif /*defined(FEATURE_LOAD_REVERSED) && defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_LOAD_REVERSED)
/*-------------------------------------------------------------------*/
/* B91F LRVR  - Load Reversed Register                         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_reversed_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Copy second operand to first operand */
    regs->GR_L(r1) = bswap_32(regs->GR_L(r2));

} /* end DEF_INST(load_reversed_register) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_LOAD_REVERSED)*/


#if defined(FEATURE_LOAD_REVERSED) && defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E30F LRVG  - Load Reversed Long                             [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_reversed_long)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_G(r1) = bswap_64(ARCH_DEP(vfetch8) ( effective_addr2, b2, regs ));

} /* end DEF_INST(load_reversed_long) */
#endif /*defined(FEATURE_LOAD_REVERSED) && defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_LOAD_REVERSED)
/*-------------------------------------------------------------------*/
/* E31E LRV   - Load Reversed                                  [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_reversed)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_L(r1) = bswap_32(ARCH_DEP(vfetch4) ( effective_addr2, b2, regs ));

} /* end DEF_INST(load_reversed) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_LOAD_REVERSED)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_LOAD_REVERSED)
/*-------------------------------------------------------------------*/
/* E31F LRVH  - Load Reversed Half                             [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_reversed_half)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_LHL(r1) = bswap_16(ARCH_DEP(vfetch2) ( effective_addr2, b2, regs ));
} /* end DEF_INST(load_reversed_half) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_LOAD_REVERSED)*/


#if defined(FEATURE_LOAD_REVERSED) && defined(FEATURE_ESAME)
/*-------------------------------------------------------------------*/
/* E32F STRVG - Store Reversed Long                            [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_reversed_long)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Store register contents at operand address */
    ARCH_DEP(vstore8) ( bswap_64(regs->GR_G(r1)), effective_addr2, b2, regs );

} /* end DEF_INST(store_reversed_long) */
#endif /*defined(FEATURE_LOAD_REVERSED) && defined(FEATURE_ESAME)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_LOAD_REVERSED)
/*-------------------------------------------------------------------*/
/* E33E STRV  - Store Reversed                                 [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_reversed)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Store register contents at operand address */
    ARCH_DEP(vstore4) ( bswap_32(regs->GR_L(r1)), effective_addr2, b2, regs );

} /* end DEF_INST(store_reversed) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_LOAD_REVERSED)*/


#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_LOAD_REVERSED)
/*-------------------------------------------------------------------*/
/* E33F STRVH - Store Reversed Half                            [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_reversed_half)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Store register contents at operand address */
    ARCH_DEP(vstore2) ( bswap_16(regs->GR_LHL(r1)), effective_addr2, b2, regs );

} /* end DEF_INST(store_reversed_half) */
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_LOAD_REVERSED)*/


#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
/*-------------------------------------------------------------------*/
/* E9   PKA   - Pack ASCII                                      [SS] */
/*-------------------------------------------------------------------*/
DEF_INST(pack_ascii)
{
int     len;                            /* Second operand length     */
int     b1, b2;                         /* Base registers            */
VADR    addr1, addr2;                   /* Effective addresses       */
BYTE    source[33];                     /* 32 digits + implied sign  */
BYTE    result[16];                     /* 31-digit packed result    */
int     i, j;                           /* Array subscripts          */

    SS_L(inst, regs, len, b1, addr1, b2, addr2);

    /* Program check if operand length (len+1) exceeds 32 bytes */
    if (len > 31)
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Fetch the second operand and right justify */
    memset (source, 0, sizeof(source));
    ARCH_DEP(vfetchc) ( source+31-len, len, addr2, b2, regs );

    /* Append an implied plus sign */
    source[32] = 0x0C;

    /* Pack the rightmost 31 digits and sign into the result */
    for (i = 1, j = 0; j < 16; i += 2, j++)
    {
        result[j] = (source[i] << 4) | (source[i+1] & 0x0F);
    }

    /* Store 16-byte packed decimal result at operand address */
    ARCH_DEP(vstorec) ( result, 16-1, addr1, b1, regs );

} /* end DEF_INST(pack_ascii) */
#endif /*defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)*/


#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
/*-------------------------------------------------------------------*/
/* E1   PKU   - Pack Unicode                                    [SS] */
/*-------------------------------------------------------------------*/
DEF_INST(pack_unicode)
{
int     len;                            /* Second operand length     */
int     b1, b2;                         /* Base registers            */
VADR    addr1, addr2;                   /* Effective addresses       */
BYTE    source[66];                     /* 32 digits + implied sign  */
BYTE    result[16];                     /* 31-digit packed result    */
int     i, j;                           /* Array subscripts          */

    SS_L(inst, regs, len, b1, addr1, b2, addr2);

    /* Program check if byte count (len+1) exceeds 64 or is odd */
    if (len > 63 || (len & 1) == 0)
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Fetch the second operand and right justify */
    memset (source, 0, sizeof(source));
    ARCH_DEP(vfetchc) ( source+63-len, len, addr2, b2, regs );

    /* Append an implied plus sign */
    source[64] = 0x00;
    source[65] = 0x0C;

    /* Pack the rightmost 31 digits and sign into the result */
    for (i = 2, j = 0; j < 16; i += 4, j++)
    {
        result[j] = (source[i+1] << 4) | (source[i+3] & 0x0F);
    }

    /* Store 16-byte packed decimal result at operand address */
    ARCH_DEP(vstorec) ( result, 16-1, addr1, b1, regs );

} /* end DEF_INST(pack_unicode) */
#endif /*defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)*/


#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
/*-------------------------------------------------------------------*/
/* EA   UNPKA - Unpack ASCII                                    [SS] */
/*-------------------------------------------------------------------*/
DEF_INST(unpack_ascii)
{
int     len;                            /* First operand length      */
int     b1, b2;                         /* Base registers            */
VADR    addr1, addr2;                   /* Effective addresses       */
BYTE    result[32];                     /* 32-digit result           */
BYTE    source[16];                     /* 31-digit packed operand   */
int     i, j;                           /* Array subscripts          */
int     cc;                             /* Condition code            */

    SS_L(inst, regs, len, b1, addr1, b2, addr2);

    /* Program check if operand length (len+1) exceeds 32 bytes */
    if (len > 31)
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Fetch the 16-byte second operand */
    ARCH_DEP(vfetchc) ( source, 15, addr2, b2, regs );

    /* Set high-order result byte to ASCII zero */
    result[0] = 0x30;

    /* Unpack remaining 31 digits into the result */
    for (j = 1, i = 0; ; i++)
    {
        result[j++] = (source[i] >> 4) | 0x30;
        if (i == 15) break;
        result[j++] = (source[i] & 0x0F) | 0x30;
    }

    /* Store rightmost digits of result at first operand address */
    ARCH_DEP(vstorec) ( result+31-len, len, addr1, b1, regs );

    /* Set the condition code according to the sign */
    switch (source[15] & 0x0F) {
    case 0x0A: case 0x0C: case 0x0E: case 0x0F:
        cc = 0; break;
    case 0x0B: case 0x0D:
        cc = 1; break;
    default:
        cc = 3;
    } /* end switch */
    regs->psw.cc = cc;

} /* end DEF_INST(unpack_ascii) */
#endif /*defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)*/


#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
/*-------------------------------------------------------------------*/
/* E2   UNPKU - Unpack Unicode                                  [SS] */
/*-------------------------------------------------------------------*/
DEF_INST(unpack_unicode)
{
int     len;                            /* First operand length      */
int     b1, b2;                         /* Base registers            */
VADR    addr1, addr2;                   /* Effective addresses       */
BYTE    result[64];                     /* 32-digit result           */
BYTE    source[16];                     /* 31-digit packed operand   */
int     i, j;                           /* Array subscripts          */
int     cc;                             /* Condition code            */

    SS_L(inst, regs, len, b1, addr1, b2, addr2);

    /* Program check if byte count (len+1) exceeds 64 or is odd */
    if (len > 63 || (len & 1) == 0)
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Fetch the 16-byte second operand */
    ARCH_DEP(vfetchc) ( source, 15, addr2, b2, regs );

    /* Set high-order result pair to Unicode zero */
    result[0] = 0x00;
    result[1] = 0x30;

    /* Unpack remaining 31 digits into the result */
    for (j = 2, i = 0; ; i++)
    {
        result[j++] = 0x00;
        result[j++] = (source[i] >> 4) | 0x30;
        if (i == 15) break;
        result[j++] = 0x00;
        result[j++] = (source[i] & 0x0F) | 0x30;
    }

    /* Store rightmost digits of result at first operand address */
    ARCH_DEP(vstorec) ( result+63-len, len, addr1, b1, regs );

    /* Set the condition code according to the sign */
    switch (source[15] & 0x0F) {
    case 0x0A: case 0x0C: case 0x0E: case 0x0F:
        cc = 0; break;
    case 0x0B: case 0x0D:
        cc = 1; break;
    default:
        cc = 3;
    } /* end switch */
    regs->psw.cc = cc;

} /* end DEF_INST(unpack_unicode) */
#endif /*defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)*/


#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
/*-------------------------------------------------------------------*/
/* B993 TROO  - Translate One to One                           [RRF] */
/*-------------------------------------------------------------------*/
DEF_INST(translate_one_to_one)
{
int     r1, r2;                         /* Values of R fields        */
VADR    addr1, addr2, trtab;            /* Effective addresses       */
GREG    len;
BYTE    svalue, dvalue, tvalue;
#ifdef FEATURE_ETF2_ENHANCEMENT
int     tccc;                   /* Test-Character-Comparison Control */
#endif

// NOTE: it's faster to decode with RRE format
// and then to handle the 'tccc' flag separately...

//  RRF_M(inst, regs, r1, r2, tccc);
    RRE(inst, regs, r1, r2);

    ODD_CHECK(r1, regs);

#ifdef FEATURE_ETF2_ENHANCEMENT
    /* Set Test-Character-Comparison Control */
    if(inst[2] & 0x10)
      tccc = 1;
    else
      tccc = 0;
#endif

    /* Determine length */
    len = GR_A(r1 + 1,regs);

    /* Determine destination, source and translate table address */
    addr1 = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    addr2 = regs->GR(r2) & ADDRESS_MAXWRAP(regs);
    trtab = regs->GR(1) & ADDRESS_MAXWRAP(regs) & ~7;

    /* Determine test value */
    tvalue = regs->GR_LHLCL(0);

    /* Preset condition code to zero in case of zero length */
    if(!len)
        regs->psw.cc = 0;

    while(len)
    {
        svalue = ARCH_DEP(vfetchb) (addr2, r2, regs);

        /* Fetch value from translation table */
        dvalue = ARCH_DEP(vfetchb) (((trtab + svalue)
                                   & ADDRESS_MAXWRAP(regs) ), 1, regs);

#ifdef FEATURE_ETF2_ENHANCEMENT
        /* Test-Character-Comparison Control */
        if(!tccc)
        {
#endif
          /* If the testvalue was found then exit with cc1 */
          if(dvalue == tvalue)
          {
            regs->psw.cc = 1;
            break;
          }
#ifdef FEATURE_ETF2_ENHANCEMENT
        }
#endif

        /* Store destination value */
        ARCH_DEP(vstoreb) (dvalue, addr1, r1, regs);

        /* Adjust source addr, destination addr and length */
        addr1++; addr1 &= ADDRESS_MAXWRAP(regs);
        addr2++; addr2 &= ADDRESS_MAXWRAP(regs);
        len--;

        /* Update the registers */
        SET_GR_A(r1, regs, addr1);
        SET_GR_A(r1 + 1, regs, len);
        SET_GR_A(r2, regs, addr2);

        /* Set cc0 when all values have been processed */
        regs->psw.cc = len ? 3 : 0;

        /* exit on the cpu determined number of bytes */
        if((len != 0) && (!(addr1 & 0xfff) || !(addr2 & 0xfff)))
            break;

    } /* end while */

} /* end DEF_INST(translate_one_to_one) */
#endif /*defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)*/


#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
/*-------------------------------------------------------------------*/
/* B992 TROT  - Translate One to Two                           [RRF] */
/*-------------------------------------------------------------------*/
DEF_INST(translate_one_to_two)
{
int     r1, r2;                         /* Values of R fields        */
VADR    addr1, addr2, trtab;            /* Effective addresses       */
GREG    len;
BYTE    svalue;
U16     dvalue, tvalue;
#ifdef FEATURE_ETF2_ENHANCEMENT
int     tccc;                   /* Test-Character-Comparison Control */
#endif

// NOTE: it's faster to decode with RRE format
// and then to handle the 'tccc' flag separately...

//  RRF_M(inst, regs, r1, r2, tccc);
    RRE(inst, regs, r1, r2);

    ODD_CHECK(r1, regs);

#ifdef FEATURE_ETF2_ENHANCEMENT
    /* Set Test-Character-Comparison Control */
    if(inst[2] & 0x10)
      tccc = 1;
    else
      tccc = 0;
#endif

    /* Determine length */
    len = GR_A(r1 + 1,regs);

    /* Determine destination, source and translate table address */
    addr1 = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    addr2 = regs->GR(r2) & ADDRESS_MAXWRAP(regs);
    trtab = regs->GR(1) & ADDRESS_MAXWRAP(regs) & ~7;

    /* Determine test value */
    tvalue = regs->GR_LHL(0);

    /* Preset condition code to zero in case of zero length */
    if(!len)
        regs->psw.cc = 0;

    while(len)
    {
        svalue = ARCH_DEP(vfetchb) (addr2, r2, regs);

        /* Fetch value from translation table */
        dvalue = ARCH_DEP(vfetch2) (((trtab + (svalue << 1))
                                   & ADDRESS_MAXWRAP(regs) ), 1, regs);

#ifdef FEATURE_ETF2_ENHANCEMENT
        /* Test-Character-Comparison Control */
        if(!tccc)
        {
#endif
          /* If the testvalue was found then exit with cc1 */
          if(dvalue == tvalue)
          {
            regs->psw.cc = 1;
            break;
          }
#ifdef FEATURE_ETF2_ENHANCEMENT
        }
#endif

        /* Store destination value */
        ARCH_DEP(vstore2) (dvalue, addr1, r1, regs);

        /* Adjust source addr, destination addr and length */
        addr1 += 2; addr1 &= ADDRESS_MAXWRAP(regs);
        addr2++; addr2 &= ADDRESS_MAXWRAP(regs);
        len--;

        /* Update the registers */
        SET_GR_A(r1, regs, addr1);
        SET_GR_A(r1 + 1, regs, len);
        SET_GR_A(r2, regs, addr2);

        /* Set cc0 when all values have been processed */
        regs->psw.cc = len ? 3 : 0;

        /* exit on the cpu determined number of bytes */
        if((len != 0) && (!(addr1 & 0xfff) || !(addr2 & 0xfff)))
            break;

    } /* end while */

} /* end DEF_INST(translate_one_to_two) */
#endif /*defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)*/


#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
/*-------------------------------------------------------------------*/
/* B991 TRTO  - Translate Two to One                           [RRF] */
/*-------------------------------------------------------------------*/
DEF_INST(translate_two_to_one)
{
int     r1, r2;                         /* Values of R fields        */
VADR    addr1, addr2, trtab;            /* Effective addresses       */
GREG    len;
U16     svalue;
BYTE    dvalue, tvalue;
#ifdef FEATURE_ETF2_ENHANCEMENT
int     tccc;                   /* Test-Character-Comparison Control */
#endif

// NOTE: it's faster to decode with RRE format
// and then to handle the 'tccc' flag separately...

//  RRF_M(inst, regs, r1, r2, tccc);
    RRE(inst, regs, r1, r2);

    ODD_CHECK(r1, regs);

#ifdef FEATURE_ETF2_ENHANCEMENT
    /* Set Test-Character-Comparison Control */
    if(inst[2] & 0x10)
      tccc = 1;
    else
      tccc = 0;
#endif

    /* Determine length */
    len = GR_A(r1 + 1,regs);

    ODD_CHECK(len, regs);

    /* Determine destination, source and translate table address */
    addr1 = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    addr2 = regs->GR(r2) & ADDRESS_MAXWRAP(regs);
#ifdef FEATURE_ETF2_ENHANCEMENT
    trtab = regs->GR(1) & ADDRESS_MAXWRAP(regs) & ~7;
#else
    trtab = regs->GR(1) & ADDRESS_MAXWRAP(regs) & ~0xfff;
#endif

    /* Determine test value */
    tvalue = regs->GR_LHLCL(0);

    /* Preset condition code to zero in case of zero length */
    if(!len)
        regs->psw.cc = 0;

    while(len)
    {
        svalue = ARCH_DEP(vfetch2) (addr2, r2, regs);

        /* Fetch value from translation table */
        dvalue = ARCH_DEP(vfetchb) (((trtab + svalue)
                                   & ADDRESS_MAXWRAP(regs) ), 1, regs);

#ifdef FEATURE_ETF2_ENHANCEMENT
        /* Test-Character-Comparison Control */
        if(!tccc)
        {
#endif
          /* If the testvalue was found then exit with cc1 */
          if(dvalue == tvalue)
          {
            regs->psw.cc = 1;
            break;
          }
#ifdef FEATURE_ETF2_ENHANCEMENT
        }
#endif

        /* Store destination value */
        ARCH_DEP(vstoreb) (dvalue, addr1, r1, regs);

        /* Adjust source addr, destination addr and length */
        addr1++; addr1 &= ADDRESS_MAXWRAP(regs);
        addr2 += 2; addr2 &= ADDRESS_MAXWRAP(regs);
        len -= 2;

        /* Update the registers */
        SET_GR_A(r1, regs, addr1);
        SET_GR_A(r1 + 1, regs, len);
        SET_GR_A(r2, regs, addr2);

        /* Set cc0 when all values have been processed */
        regs->psw.cc = len ? 3 : 0;

        /* exit on the cpu determined number of bytes */
        if((len != 0) && (!(addr1 & 0xfff) || !(addr2 & 0xfff)))
            break;

    } /* end while */

} /* end DEF_INST(translate_two_to_one) */
#endif /*defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)*/


#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
/*-------------------------------------------------------------------*/
/* B990 TRTT  - Translate Two to Two                           [RRF] */
/*-------------------------------------------------------------------*/
DEF_INST(translate_two_to_two)
{
int     r1, r2;                         /* Values of R fields        */
VADR    addr1, addr2, trtab;            /* Effective addresses       */
GREG    len;
U16     svalue, dvalue, tvalue;
#ifdef FEATURE_ETF2_ENHANCEMENT
int     tccc;                   /* Test-Character-Comparison Control */
#endif

// NOTE: it's faster to decode with RRE format
// and then to handle the 'tccc' flag separately...

//  RRF_M(inst, regs, r1, r2, tccc);
    RRE(inst, regs, r1, r2);

    ODD_CHECK(r1, regs);

#ifdef FEATURE_ETF2_ENHANCEMENT
    /* Set Test-Character-Comparison Control */
    if(inst[2] & 0x10)
      tccc = 1;
    else
      tccc = 0;
#endif

    /* Determine length */
    len = GR_A(r1 + 1,regs);

    ODD_CHECK(len, regs);

    /* Determine destination, source and translate table address */
    addr1 = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    addr2 = regs->GR(r2) & ADDRESS_MAXWRAP(regs);
#ifdef FEATURE_ETF2_ENHANCEMENT
    trtab = regs->GR(1) & ADDRESS_MAXWRAP(regs) & ~7;
#else
    trtab = regs->GR(1) & ADDRESS_MAXWRAP(regs) & ~0xfff;
#endif

    /* Determine test value */
    tvalue = regs->GR_LHL(0);

    /* Preset condition code to zero in case of zero length */
    if(!len)
        regs->psw.cc = 0;

    while(len)
    {
        svalue = ARCH_DEP(vfetch2) (addr2, r2, regs);

        /* Fetch value from translation table */
        dvalue = ARCH_DEP(vfetch2) (((trtab + (svalue << 1))
                                   & ADDRESS_MAXWRAP(regs) ), 1, regs);

#ifdef FEATURE_ETF2_ENHANCEMENT
        /* Test-Character-Comparison Control */
        if(!tccc)
        {
#endif
          /* If the testvalue was found then exit with cc1 */
          if(dvalue == tvalue)
          {
            regs->psw.cc = 1;
            break;
          }
#ifdef FEATURE_ETF2_ENHANCEMENT
        }
#endif

        /* Store destination value */
        ARCH_DEP(vstore2) (dvalue, addr1, r1, regs);

        /* Adjust source addr, destination addr and length */
        addr1 += 2; addr1 &= ADDRESS_MAXWRAP(regs);
        addr2 += 2; addr2 &= ADDRESS_MAXWRAP(regs);
        len -= 2;

        /* Update the registers */
        SET_GR_A(r1, regs, addr1);
        SET_GR_A(r1 + 1, regs, len);
        SET_GR_A(r2, regs, addr2);

        /* Set cc0 when all values have been processed */
        regs->psw.cc = len ? 3 : 0;

        /* exit on the cpu determined number of bytes */
        if((len != 0) && (!(addr1 & 0xfff) || !(addr2 & 0xfff)))
            break;

    } /* end while */

} /* end DEF_INST(translate_two_to_two) */
#endif /*defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)*/


#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
/*-------------------------------------------------------------------*/
/* EB8E MVCLU - Move Long Unicode                              [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(move_long_unicode)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i;                              /* Loop counter              */
int     cc;                             /* Condition code            */
VADR    addr1, addr3;                   /* Operand addresses         */
GREG    len1, len3;                     /* Operand lengths           */
U16     odbyte;                         /* Operand double byte       */
U16     pad;                            /* Padding double byte       */
int     cpu_length;                     /* cpu determined length     */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    ODD2_CHECK(r1, r3, regs);

    /* Load operand lengths from bits 0-31 of R1+1 and R3+1 */
    len1 = GR_A(r1 + 1, regs);
    len3 = GR_A(r3 + 1, regs);

    ODD2_CHECK(len1, len3, regs);

    /* Load padding doublebyte from bits 48-63 of effective address */
    pad = effective_addr2 & 0xFFFF;

    /* Determine the destination and source addresses */
    addr1 = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    addr3 = regs->GR(r3) & ADDRESS_MAXWRAP(regs);

    /* set cpu_length as shortest distance to new page */
    if ((addr1 & 0xFFF) > (addr3 & 0xFFF))
        cpu_length = 0x1000 - (addr1 & 0xFFF);
    else
        cpu_length = 0x1000 - (addr3 & 0xFFF);

    /* Set the condition code according to the lengths */
    cc = (len1 < len3) ? 1 : (len1 > len3) ? 2 : 0;

    /* Process operands from left to right */
    for (i = 0; len1 > 0; i += 2)
    {
        /* If cpu determined length has been moved, exit with cc=3 */
        if (i >= cpu_length)
        {
            cc = 3;
            break;
        }

        /* Fetch byte from source operand, or use padding double byte */
        if (len3 > 0)
        {
            odbyte = ARCH_DEP(vfetch2) ( addr3, r3, regs );
            addr3 += 2;
            addr3 &= ADDRESS_MAXWRAP(regs);
            len3 -= 2;
        }
        else
            odbyte = pad;

        /* Store the double byte in the destination operand */
        ARCH_DEP(vstore2) ( odbyte, addr1, r1, regs );
        addr1 +=2;
        addr1 &= ADDRESS_MAXWRAP(regs);
        len1 -= 2;

        /* Update the registers */
        SET_GR_A(r1, regs, addr1);
        SET_GR_A(r1 + 1, regs, len1);
        SET_GR_A(r3, regs, addr3);
        SET_GR_A(r3 + 1, regs, len3);

    } /* end for(i) */

    regs->psw.cc = cc;

} /* end DEF_INST(move_long_unicode) */
#endif /*defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)*/


#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
/*-------------------------------------------------------------------*/
/* EB8F CLCLU - Compare Logical Long Unicode                   [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_long_unicode)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i;                              /* Loop counter              */
int     cc = 0;                         /* Condition code            */
VADR    addr1, addr3;                   /* Operand addresses         */
GREG    len1, len3;                     /* Operand lengths           */
U16     dbyte1, dbyte3;                 /* Operand double bytes      */
U16     pad;                            /* Padding double byte       */
int     cpu_length;                     /* cpu determined length     */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    ODD2_CHECK(r1, r3, regs);

    /* Load operand lengths from bits 0-31 of R1+1 and R3+1 */
    len1 = GR_A(r1 + 1, regs);
    len3 = GR_A(r3 + 1, regs);

    ODD2_CHECK(len1, len3, regs);

    /* Load padding doublebyte from bits 48-64 of effective address */
    pad = effective_addr2 & 0xFFFF;

    /* Determine the destination and source addresses */
    addr1 = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    addr3 = regs->GR(r3) & ADDRESS_MAXWRAP(regs);

    /* set cpu_length as shortest distance to new page */
    if ((addr1 & 0xFFF) > (addr3 & 0xFFF))
        cpu_length = 0x1000 - (addr1 & 0xFFF);
    else
        cpu_length = 0x1000 - (addr3 & 0xFFF);

    /* Process operands from left to right */
    for (i = 0; len1 > 0 || len3 > 0 ; i += 2)
    {
        /* If max 4096 bytes have been compared, exit with cc=3 */
        if (i >= cpu_length)
        {
            cc = 3;
            break;
        }

        /* Fetch a byte from each operand, or use padding double byte */
        dbyte1 = (len1 > 0) ? ARCH_DEP(vfetch2) (addr1, r1, regs) : pad;
        dbyte3 = (len3 > 0) ? ARCH_DEP(vfetch2) (addr3, r3, regs) : pad;

        /* Compare operand bytes, set condition code if unequal */
        if (dbyte1 != dbyte3)
        {
            cc = (dbyte1 < dbyte3) ? 1 : 2;
            break;
        } /* end if */

        /* Update the first operand address and length */
        if (len1 > 0)
        {
            addr1 += 2;
            addr1 &= ADDRESS_MAXWRAP(regs);
            len1 -= 2;
        }

        /* Update the second operand address and length */
        if (len3 > 0)
        {
            addr3 += 2;
            addr3 &= ADDRESS_MAXWRAP(regs);
            len3 -= 2;
        }

    } /* end for(i) */

    /* Update the registers */
    SET_GR_A(r1, regs, addr1);
    SET_GR_A(r1 + 1, regs, len1);
    SET_GR_A(r3, regs, addr3);
    SET_GR_A(r3 + 1, regs, len3);

    regs->psw.cc = cc;

} /* end DEF_INST(compare_logical_long_unicode) */
#endif /*defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E376 LB    - Load Byte                                      [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_byte)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load sign-extended byte from operand address */
    regs->GR_L(r1) = (S8)ARCH_DEP(vfetchb) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_byte) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E377 LGB   - Load Byte Long                                 [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_byte_long)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load sign-extended byte from operand address */
    regs->GR_G(r1) = (S8)ARCH_DEP(vfetchb) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_byte_long) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E35A AY    - Add (Long Displacement)                        [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(add_y)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Add signed operands and set condition code */
    regs->psw.cc =
            add_signed (&(regs->GR_L(r1)),
                    regs->GR_L(r1),
                    n);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E37A AHY   - Add Halfword (Long Displacement)               [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(add_halfword_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
S32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load 2 bytes from operand address */
    n = (S16)ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

    /* Add signed operands and set condition code */
    regs->psw.cc =
            add_signed (&(regs->GR_L(r1)),
                    regs->GR_L(r1),
                    (U32)n);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_halfword_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E35E ALY   - Add Logical (Long Displacement)                [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Add signed operands and set condition code */
    regs->psw.cc =
            add_logical (&(regs->GR_L(r1)),
                    regs->GR_L(r1),
                    n);

} /* end DEF_INST(add_logical_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* EB54 NIY   - And Immediate (Long Displacement)              [SIY] */
/*-------------------------------------------------------------------*/
DEF_INST(and_immediate_y)
{
BYTE    i2;                             /* Immediate byte of opcode  */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
BYTE   *dest;                         /* Pointer to target byte      */

    SIY(inst, regs, i2, b1, effective_addr1);

    ITIMER_SYNC(effective_addr1, 0, regs);

    /* Get byte mainstor address */
    dest = MADDR (effective_addr1, b1, regs, ACCTYPE_WRITE, regs->psw.pkey );

    /* AND byte with immediate operand, setting condition code */
    regs->psw.cc = (H_ATOMIC_OP(dest, i2, and, And, &) != 0);

    ITIMER_UPDATE(effective_addr1,0,regs);

} /* end DEF_INST(and_immediate_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E354 NY    - And (Long Displacement)                        [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(and_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* AND second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_L(r1) &= n ) ? 1 : 0;

} /* end DEF_INST(and_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E359 CY    - Compare (Long Displacement)                    [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_y)
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
            (S32)regs->GR_L(r1) < (S32)n ? 1 :
            (S32)regs->GR_L(r1) > (S32)n ? 2 : 0;

} /* end DEF_INST(compare_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E379 CHY   - Compare Halfword (Long Displacement)           [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_halfword_y)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
S32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load rightmost 2 bytes of comparand from operand address */
    n = (S16)ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            (S32)regs->GR_L(r1) < n ? 1 :
            (S32)regs->GR_L(r1) > n ? 2 : 0;

} /* end DEF_INST(compare_halfword_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E355 CLY   - Compare Logical (Long Displacement)            [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_y)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = regs->GR_L(r1) < n ? 1 :
                   regs->GR_L(r1) > n ? 2 : 0;

} /* end DEF_INST(compare_logical_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* EB55 CLIY  - Compare Logical Immediate (Long Displacement)  [SIY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_immediate_y)
{
BYTE    i2;                             /* Immediate byte            */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
BYTE    cbyte;                          /* Compare byte              */

    SIY(inst, regs, i2, b1, effective_addr1);

    /* Fetch byte from operand address */
    cbyte = ARCH_DEP(vfetchb) ( effective_addr1, b1, regs );

    /* Compare with immediate operand and set condition code */
    regs->psw.cc = (cbyte < i2) ? 1 :
                   (cbyte > i2) ? 2 : 0;

} /* end DEF_INST(compare_logical_immediate_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* EB21 CLMY  - Compare Logical Characters under Mask Long Disp[RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_characters_under_mask_y)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i, j;                           /* Integer work areas        */
int     cc = 0;                         /* Condition code            */
BYTE    rbyte[4],                       /* Register bytes            */
        vbyte;                          /* Virtual storage byte      */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    /* Set register bytes by mask */
    i = 0;
    if (r3 & 0x8) rbyte[i++] = (regs->GR_L(r1) >> 24) & 0xFF;
    if (r3 & 0x4) rbyte[i++] = (regs->GR_L(r1) >> 16) & 0xFF;
    if (r3 & 0x2) rbyte[i++] = (regs->GR_L(r1) >>  8) & 0xFF;
    if (r3 & 0x1) rbyte[i++] = (regs->GR_L(r1)      ) & 0xFF;

    /* Perform access check if mask is 0 */
    if (!r3) ARCH_DEP(vfetchb) (effective_addr2, b2, regs);

    /* Compare byte by byte */
    for (j = 0; j < i && !cc; j++)
    {
        effective_addr2 &= ADDRESS_MAXWRAP(regs);
        vbyte = ARCH_DEP(vfetchb) (effective_addr2++, b2, regs);
        if (rbyte[j] != vbyte)
            cc = rbyte[j] < vbyte ? 1 : 2;
    }

    regs->psw.cc = cc;

} /* end DEF_INST(compare_logical_characters_under_mask_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* EB14 CSY   - Compare and Swap (Long Displacement)           [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_swap_y)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
BYTE   *main2;                          /* mainstor address          */
U32     old;                            /* old value                 */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    FW_CHECK(effective_addr2, regs);

    /* Perform serialization before starting operation */
    PERFORM_SERIALIZATION (regs);

    /* Get operand mainstor address */
    main2 = MADDR (effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

    /* Get old value */
    old = CSWAP32(regs->GR_L(r1));

    /* Obtain main-storage access lock */
    OBTAIN_MAINLOCK(regs);

    /* Attempt to exchange the values */
    regs->psw.cc = cmpxchg4 (&old, CSWAP32(regs->GR_L(r3)), main2);

    /* Release main-storage access lock */
    RELEASE_MAINLOCK(regs);

    /* Perform serialization after completing operation */
    PERFORM_SERIALIZATION (regs);

    if (regs->psw.cc == 1)
    {
        regs->GR_L(r1) = CSWAP32(old);
#if defined(_FEATURE_SIE)
        if(SIE_STATB(regs, IC0, CS1))
        {
            if( !OPEN_IC_PER(regs) )
                longjmp(regs->progjmp, SIE_INTERCEPT_INST);
            else
                longjmp(regs->progjmp, SIE_INTERCEPT_INSTCOMP);
        }
        else
#endif /*defined(_FEATURE_SIE)*/
            if (sysblk.cpus > 1)
                sched_yield();
    }

} /* end DEF_INST(compare_and_swap_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* EB31 CDSY  - Compare Double and Swap (Long Displacement)    [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_double_and_swap_y)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
BYTE   *main2;                          /* mainstor address          */
U64     old, new;                       /* old, new values           */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    ODD2_CHECK(r1, r3, regs);

    DW_CHECK(effective_addr2, regs);

    /* Perform serialization before starting operation */
    PERFORM_SERIALIZATION (regs);

    /* Get operand mainstor address */
    main2 = MADDR (effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

    /* Get old, new values */
    old = CSWAP64(((U64)(regs->GR_L(r1)) << 32) | regs->GR_L(r1+1));
    new = CSWAP64(((U64)(regs->GR_L(r3)) << 32) | regs->GR_L(r3+1));

    /* Obtain main-storage access lock */
    OBTAIN_MAINLOCK(regs);

    /* Attempt to exchange the values */
    regs->psw.cc = cmpxchg8 (&old, new, main2);

    /* Release main-storage access lock */
    RELEASE_MAINLOCK(regs);

    /* Perform serialization after completing operation */
    PERFORM_SERIALIZATION (regs);

    if (regs->psw.cc == 1)
    {
        regs->GR_L(r1) = CSWAP64(old) >> 32;
        regs->GR_L(r1+1) = CSWAP64(old) & 0xffffffff;
#if defined(_FEATURE_SIE)
        if(SIE_STATB(regs, IC0, CS1))
        {
            if( !OPEN_IC_PER(regs) )
                longjmp(regs->progjmp, SIE_INTERCEPT_INST);
            else
                longjmp(regs->progjmp, SIE_INTERCEPT_INSTCOMP);
        }
        else
#endif /*defined(_FEATURE_SIE)*/
            if (sysblk.cpus > 1)
                sched_yield();
    }

} /* end DEF_INST(compare_double_and_swap_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E306 CVBY  - Convert to Binary (Long Displacement)          [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_to_binary_y)
{
U64     dreg;                           /* 64-bit result accumulator */
int     r1;                             /* Value of R1 field         */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     ovf;                            /* 1=overflow                */
int     dxf;                            /* 1=data exception          */
BYTE    dec[8];                         /* Packed decimal operand    */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Fetch 8-byte packed decimal operand */
    ARCH_DEP(vfetchc) (dec, 8-1, effective_addr2, b2, regs);

    /* Convert 8-byte packed decimal to 64-bit signed binary */
    packed_to_binary (dec, 8-1, &dreg, &ovf, &dxf);

    /* Data exception if invalid digits or sign */
    if (dxf)
    {
        regs->dxc = DXC_DECIMAL;
        regs->program_interrupt (regs, PGM_DATA_EXCEPTION);
    }

    /* Overflow if result exceeds 31 bits plus sign */
    if ((S64)dreg < -2147483648LL || (S64)dreg > 2147483647LL)
       ovf = 1;

    /* Store low-order 32 bits of result into R1 register */
    regs->GR_L(r1) = dreg & 0xFFFFFFFF;

    /* Program check if overflow (R1 contains rightmost 32 bits) */
    if (ovf)
        regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);

}
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E326 CVDY  - Convert to Decimal (Long Displacement)         [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_to_decimal_y)
{
S64     bin;                            /* 64-bit signed binary value*/
int     r1;                             /* Value of R1 field         */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
BYTE    dec[16];                        /* Packed decimal result     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load value of register and sign-extend to 64 bits */
    bin = (S64)((S32)(regs->GR_L(r1)));

    /* Convert to 16-byte packed decimal number */
    binary_to_packed (bin, dec);

    /* Store low 8 bytes of result at operand address */
    ARCH_DEP(vstorec) ( dec+8, 8-1, effective_addr2, b2, regs );

} /* end DEF_INST(convert_to_decimal_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* EB57 XIY   - Exclusive Or Immediate (Long Displacement)     [SIY] */
/*-------------------------------------------------------------------*/
DEF_INST(exclusive_or_immediate_y)
{
BYTE    i2;                             /* Immediate operand         */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
BYTE   *dest;                         /* Pointer to target byte      */

    SIY(inst, regs, i2, b1, effective_addr1);

    ITIMER_SYNC(effective_addr1, 0, regs);

    /* Get byte mainstor address */
    dest = MADDR (effective_addr1, b1, regs, ACCTYPE_WRITE, regs->psw.pkey );

    /* XOR byte with immediate operand, setting condition code */
    regs->psw.cc = (H_ATOMIC_OP(dest, i2, xor, Xor, ^) != 0);

    ITIMER_UPDATE(effective_addr1,0,regs);

} /* end DEF_INST(exclusive_or_immediate_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E357 XY    - Exclusive Or (Long Displacement)               [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(exclusive_or_y)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* XOR second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_L(r1) ^= n ) ? 1 : 0;

} /* end DEF_INST(exclusive_or_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E373 ICY   - Insert Character (Long Displacement)           [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_character_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Insert character in r1 register */
    regs->GR_LHLCL(r1) = ARCH_DEP(vfetchb) ( effective_addr2, b2, regs );

} /* end DEF_INST(insert_character_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* EB81 ICMY  - Insert Characters under Mask Long Displacement [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_characters_under_mask_y)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int    i;                               /* Integer work area         */
BYTE   vbyte[4];                        /* Fetched storage bytes     */
U32    n;                               /* Fetched value             */
static const int                        /* Length-1 to fetch by mask */
       icmylen[16] = {0, 0, 0, 1, 0, 1, 1, 2, 0, 1, 1, 2, 1, 2, 2, 3};
static const unsigned int               /* Turn reg bytes off by mask*/
       icmymask[16] = {0xFFFFFFFF, 0xFFFFFF00, 0xFFFF00FF, 0xFFFF0000,
                       0xFF00FFFF, 0xFF00FF00, 0xFF0000FF, 0xFF000000,
                       0x00FFFFFF, 0x00FFFF00, 0x00FF00FF, 0x00FF0000,
                       0x0000FFFF, 0x0000FF00, 0x000000FF, 0x00000000};

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    switch (r3) {

    case 15:
        /* Optimized case */
        regs->GR_L(r1) = ARCH_DEP(vfetch4) (effective_addr2, b2, regs);
        regs->psw.cc = regs->GR_L(r1) ? regs->GR_L(r1) & 0x80000000 ?
                       1 : 2 : 0;
        break;

    default:
        memset(vbyte, 0, 4);
        ARCH_DEP(vfetchc)(vbyte, icmylen[r3], effective_addr2, b2, regs);

        /* If mask was 0 then we still had to fetch, according to POP.
           If so, set the fetched byte to 0 to force zero cc */
        if (!r3) vbyte[0] = 0;

        n = fetch_fw (vbyte);
        regs->psw.cc = n ? n & 0x80000000 ?
                       1 : 2 : 0;

        /* Turn off the reg bytes we are going to set */
        regs->GR_L(r1) &= icmymask[r3];

        /* Set bytes one at a time according to the mask */
        i = 0;
        if (r3 & 0x8) regs->GR_L(r1) |= vbyte[i++] << 24;
        if (r3 & 0x4) regs->GR_L(r1) |= vbyte[i++] << 16;
        if (r3 & 0x2) regs->GR_L(r1) |= vbyte[i++] << 8;
        if (r3 & 0x1) regs->GR_L(r1) |= vbyte[i];
        break;

    } /* switch (r3) */

} /* end DEF_INST(insert_characters_under_mask_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E358 LY    - Load (Long Displacement)                       [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_L(r1) = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
#if defined(FEATURE_ACCESS_REGISTERS)
/*-------------------------------------------------------------------*/
/* EB9A LAMY  - Load Access Multiple (Long Displacement)       [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_access_multiple_y)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i, m, n;                        /* Integer work areas        */
U32    *p1, *p2 = NULL;                 /* Mainstor pointers         */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    FW_CHECK(effective_addr2, regs);

    /* Calculate number of regs to load */
    n = ((r3 - r1) & 0xF) + 1;

    /* Calculate number of words to next boundary */
    m = (0x800 - (effective_addr2 & 0x7ff)) >> 2;

    /* Address of operand beginning */
    p1 = (U32*)MADDR(effective_addr2, b2, regs, ACCTYPE_READ, regs->psw.pkey);

    /* Get address of next page if boundary crossed */
    if (unlikely (m < n))
        p2 = (U32*)MADDR(effective_addr2 + (m*4), b2, regs, ACCTYPE_READ, regs->psw.pkey);
    else
        m = n;

    /* Load from first page */
    for (i = 0; i < m; i++, p1++)
    {
        regs->AR((r1 + i) & 0xF) = fetch_fw (p1);
        SET_AEA_AR(regs, (r1 + i) & 0xF);
    }

    /* Load from next page */
    for ( ; i < n; i++, p2++)
    {
        regs->AR((r1 + i) & 0xF) = fetch_fw (p2);
        SET_AEA_AR(regs, (r1 + i) & 0xF);
    }

} /* end DEF_INST(load_access_multiple_y) */
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E371 LAY   - Load Address (Long Displacement)               [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_address_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY0(inst, regs, r1, b2, effective_addr2);

    /* Load operand address into register */
    SET_GR_A(r1, regs, effective_addr2);

} /* end DEF_INST(load_address_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E378 LHY   - Load Halfword (Long Displacement)              [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_halfword_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load rightmost 2 bytes of register from operand address */
    regs->GR_L(r1) = (S16)ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_halfword_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* EB98 LMY   - Load Multiple (Long Displacement)              [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_multiple_y)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i, m, n;                        /* Integer work areas        */
U32    *p1, *p2;                        /* Mainstor pointers         */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    /* Calculate number of bytes to load */
    n = (((r3 - r1) & 0xF) + 1) << 2;

    /* Calculate number of bytes to next boundary */
    m = 0x800 - ((VADR_L)effective_addr2 & 0x7ff);

    /* Address of operand beginning */
    p1 = (U32*)MADDR(effective_addr2, b2, regs, ACCTYPE_READ, regs->psw.pkey);

    if (likely(n <= m))
    {
        /* Boundary not crossed */
        n >>= 2;
        for (i = 0; i < n; i++, p1++)
            regs->GR_L((r1 + i) & 0xF) = fetch_fw (p1);
    }
    else
    {
        /* Boundary crossed, get 2nd page address */
        effective_addr2 += m;
        effective_addr2 &= ADDRESS_MAXWRAP(regs);
        p2 = (U32*)MADDR(effective_addr2, b2, regs, ACCTYPE_READ, regs->psw.pkey);

        if (likely((m & 0x3) == 0))
        {
            /* Addresses are word aligned */
            m >>= 2;
            for (i = 0; i < m; i++, p1++)
                regs->GR_L((r1 + i) & 0xF) = fetch_fw (p1);
            n >>= 2;
            for ( ; i < n; i++, p2++)
                regs->GR_L((r1 + i) & 0xF) = fetch_fw (p2);
        }
        else
        {
            /* Worst case */
            U32 rwork[16];
            BYTE *b1, *b2;

            b1 = (BYTE *)&rwork[0];
            b2 = (BYTE *)p1;
            for (i = 0; i < m; i++)
                *b1++ = *b2++;
            b2 = (BYTE *)p2;
            for ( ; i < n; i++)
                *b1++ = *b2++;

            n >>= 2;
            for (i = 0; i < n; i++)
                regs->GR_L((r1 + i) & 0xF) = CSWAP32(rwork[i]);
        }
    }

} /* end DEF_INST(load_multiple_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E313 LRAY  - Load Real Address (Long Displacement)          [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_real_address_y)
{
int     r1;                             /* Register number           */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    ARCH_DEP(load_real_address_proc) (regs, r1, b2, effective_addr2);

} /* end DEF_INST(load_real_address_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* EB52 MVIY  - Move Immediate (Long Displacement)             [SIY] */
/*-------------------------------------------------------------------*/
DEF_INST(move_immediate_y)
{
BYTE    i2;                             /* Immediate operand         */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */

    SIY(inst, regs, i2, b1, effective_addr1);

    /* Store immediate operand at operand address */
    ARCH_DEP(vstoreb) ( i2, effective_addr1, b1, regs );

} /* end DEF_INST(move_immediate_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E351 MSY   - Multiply Single (Long Displacement)            [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_single_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
S32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = (S32)ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Multiply signed operands ignoring overflow */
    regs->GR_L(r1) = (S32)regs->GR_L(r1) * n;

} /* end DEF_INST(multiply_single) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* EB56 OIY   - Or Immediate (Long Displacement)               [SIY] */
/*-------------------------------------------------------------------*/
DEF_INST(or_immediate_y)
{
BYTE    i2;                             /* Immediate operand byte    */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
BYTE   *dest;                         /* Pointer to target byte      */

    SIY(inst, regs, i2, b1, effective_addr1);

    ITIMER_SYNC(effective_addr1, 0, regs);

    /* Get byte mainstor address */
    dest = MADDR (effective_addr1, b1, regs, ACCTYPE_WRITE, regs->psw.pkey );

    /* OR byte with immediate operand, setting condition code */
    regs->psw.cc = (H_ATOMIC_OP(dest, i2, or, Or, |) != 0);

    ITIMER_UPDATE(effective_addr1,0,regs);


} /* end DEF_INST(or_immediate_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E356 OY    - Or (Long Displacement)                         [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(or_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* OR second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_L(r1) |= n ) ? 1 : 0;

} /* end DEF_INST(or_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E350 STY   - Store (Long Displacement)                      [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_y)
{
int     r1;                             /* Values of R fields        */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Store register contents at operand address */
    ARCH_DEP(vstore4) ( regs->GR_L(r1), effective_addr2, b2, regs );

} /* end DEF_INST(store_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
#if defined(FEATURE_ACCESS_REGISTERS)
/*-------------------------------------------------------------------*/
/* EB9B STAMY - Store Access Multiple (Long Displacement)      [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_access_multiple_y)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i, m, n;                        /* Integer work area         */
U32    *p1, *p2 = NULL;                 /* Mainstor pointers         */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    FW_CHECK(effective_addr2, regs);

    /* Calculate number of regs to store */
    n = ((r3 - r1) & 0xF) + 1;

    /* Store 4 bytes at a time */
    ARCH_DEP(validate_operand)(effective_addr2, b2, (n*4) - 1, ACCTYPE_WRITE, regs);
    for (i = 0; i < n; i++)
        ARCH_DEP(vstore4)(regs->AR((r1 + i) & 0xF), effective_addr2 + (i*4), b2, regs);


    /* Calculate number of words to next boundary */
    m = (0x800 - (effective_addr2 & 0x7ff)) >> 2;

    /* Address of operand beginning */
    p1 = (U32*)MADDR(effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

    /* Get address of next page if boundary crossed */
    if (unlikely(m < n))
        p2 = (U32*)MADDR(effective_addr2 + (m*4), b2, regs, ACCTYPE_WRITE, regs->psw.pkey);
    else
        m = n;

    /* Store at operand beginning */
    for (i = 0; i < m; i++)
        store_fw (p1++, regs->AR((r1 + i) & 0xF));

    /* Store on next page */
    for ( ; i < n; i++)
        store_fw (p2++, regs->AR((r1 + i) & 0xF));


} /* end DEF_INST(store_access_multiple_y) */
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E372 STCY  - Store Character (Long Displacement)            [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_character_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Store rightmost byte of R1 register at operand address */
    ARCH_DEP(vstoreb) ( regs->GR_LHLCL(r1), effective_addr2, b2, regs );

} /* end DEF_INST(store_character_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* EB2D STCMY - Store Characters under Mask (Long Displacement)[RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_characters_under_mask_y)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i;                              /* Integer work area         */
BYTE    rbyte[4];                       /* Byte work area            */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    switch (r3) {

    case 15:
        /* Optimized case */
        ARCH_DEP(vstore4) (regs->GR_L(r1), effective_addr2, b2, regs);
        break;

    default:
        /* Extract value from register by mask */
        i = 0;
        if (r3 & 0x8) rbyte[i++] = (regs->GR_L(r1) >> 24) & 0xFF;
        if (r3 & 0x4) rbyte[i++] = (regs->GR_L(r1) >> 16) & 0xFF;
        if (r3 & 0x2) rbyte[i++] = (regs->GR_L(r1) >>  8) & 0xFF;
        if (r3 & 0x1) rbyte[i++] = (regs->GR_L(r1)      ) & 0xFF;

        if (i)
            ARCH_DEP(vstorec) (rbyte, i-1, effective_addr2, b2, regs);
#if defined(MODEL_DEPENDENT_STCM)
        /* If the mask is all zero, we nevertheless access one byte
           from the storage operand, because POP states that an
           access exception may be recognized on the first byte */
        else
            ARCH_DEP(validate_operand) (effective_addr2, b2, 0,
                                        ACCTYPE_WRITE, regs);
#endif
        break;

    } /* switch (r3) */

} /* end DEF_INST(store_characters_under_mask_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E370 STHY  - Store Halfword (Long Displacement)             [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_halfword_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Store rightmost 2 bytes of R1 register at operand address */
    ARCH_DEP(vstore2) ( regs->GR_LHL(r1), effective_addr2, b2, regs );

} /* end DEF_INST(store_halfword_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* EB90 STMY  - Store Multiple (Long Displacement)             [RSY] */
/*-------------------------------------------------------------------*/
DEF_INST(store_multiple_y)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i, m, n;                        /* Integer work areas        */
U32    *p1, *p2;                        /* Mainstor pointers         */

    RSY(inst, regs, r1, r3, b2, effective_addr2);

    /* Calculate number of bytes to store */
    n = (((r3 - r1) & 0xF) + 1) << 2;

    /* Calculate number of bytes to next boundary */
    m = 0x800 - ((VADR_L)effective_addr2 & 0x7ff);

    /* Get address of first page */
    p1 = (U32*)MADDR(effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

    if (likely(n <= m))
    {
        /* boundary not crossed */
        n >>= 2;
        for (i = 0; i < n; i++)
            store_fw (p1++, regs->GR_L((r1 + i) & 0xF));
    }
    else
    {
        /* boundary crossed, get address of the 2nd page */
        effective_addr2 += m;
        effective_addr2 &= ADDRESS_MAXWRAP(regs);
        p2 = (U32*)MADDR(effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

        if (likely((m & 0x3) == 0))
        {
            /* word aligned */
            m >>= 2;
            for (i = 0; i < m; i++)
                store_fw (p1++, regs->GR_L((r1 + i) & 0xF));
            n >>= 2;
            for ( ; i < n; i++)
                store_fw (p2++, regs->GR_L((r1 + i) & 0xF));
        }
        else
        {
            /* worst case */
            U32 rwork[16];
            BYTE *b1, *b2;

            for (i = 0; i < (n >> 2); i++)
                rwork[i] = CSWAP32(regs->GR_L((r1 + i) & 0xF));
            b1 = (BYTE *)&rwork[0];

            b2 = (BYTE *)p1;
            for (i = 0; i < m; i++)
                *b2++ = *b1++;

            b2 = (BYTE *)p2;
            for ( ; i < n; i++)
                *b2++ = *b1++;
        }
    }

} /* end DEF_INST(store_multiple_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E35B SY    - Subtract (Long Displacement)                   [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Subtract signed operands and set condition code */
    regs->psw.cc =
            sub_signed (&(regs->GR_L(r1)),
                    regs->GR_L(r1),
                    n);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(subtract_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E37B SHY   - Subtract Halfword (Long Displacement)          [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_halfword_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
S32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load 2 bytes from operand address */
    n = (S16)ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

    /* Subtract signed operands and set condition code */
    regs->psw.cc =
            sub_signed (&(regs->GR_L(r1)),
                    regs->GR_L(r1),
                    (U32)n);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(subtract_halfword_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* E35F SLY   - Subtract Logical (Long Displacement)           [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_y)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc =
            sub_logical (&(regs->GR_L(r1)),
                    regs->GR_L(r1),
                    n);

} /* end DEF_INST(subtract_logical_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_LONG_DISPLACEMENT)
/*-------------------------------------------------------------------*/
/* EB51 TMY   - Test under Mask (Long Displacement)            [SIY] */
/*-------------------------------------------------------------------*/
DEF_INST(test_under_mask_y)
{
BYTE    i2;                             /* Immediate operand         */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
BYTE    tbyte;                          /* Work byte                 */

    SIY(inst, regs, i2, b1, effective_addr1);

    /* Fetch byte from operand address */
    tbyte = ARCH_DEP(vfetchb) ( effective_addr1, b1, regs );

    /* AND with immediate operand mask */
    tbyte &= i2;

    /* Set condition code according to result */
    regs->psw.cc =
            ( tbyte == 0 ) ? 0 :            /* result all zeroes */
            ( tbyte == i2) ? 3 :            /* result all ones   */
            1 ;                             /* result mixed      */

} /* end DEF_INST(test_under_mask_y) */
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/


#if defined(FEATURE_EXTENDED_IMMEDIATE)                         /*@Z9*/
/*-------------------------------------------------------------------*/
/* C2x9 AFI   - Add Fullword Immediate                         [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(add_fullword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL(inst, regs, r1, opcd, i2);

    /* Add signed operands and set condition code */
    regs->psw.cc = add_signed (&(regs->GR_L(r1)),
                                regs->GR_L(r1),
                                (S32)i2);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_fullword_immediate) */


/*-------------------------------------------------------------------*/
/* C2x8 AGFI  - Add Long Fullword Immediate                    [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(add_long_fullword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL(inst, regs, r1, opcd, i2);

    /* Add signed operands and set condition code */
    regs->psw.cc = add_signed_long (&(regs->GR_G(r1)),
                                    regs->GR_G(r1),
                                    (S32)i2);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

} /* end DEF_INST(add_long_fullword_immediate) */


/*-------------------------------------------------------------------*/
/* C2xB ALFI  - Add Logical Fullword Immediate                 [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_fullword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Add signed operands and set condition code */
    regs->psw.cc = add_logical (&(regs->GR_L(r1)),
                                regs->GR_L(r1),
                                i2);

} /* end DEF_INST(add_logical_fullword_immediate) */


/*-------------------------------------------------------------------*/
/* C2xA ALGFI - Add Logical Long Fullword Immediate            [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_long_fullword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Add unsigned operands and set condition code */
    regs->psw.cc = add_logical_long(&(regs->GR_G(r1)),
                                    regs->GR_G(r1),
                                    i2);

} /* end DEF_INST(add_logical_long_fullword_immediate) */


/*-------------------------------------------------------------------*/
/* C0xA NIHF  - And Immediate High Fullword                    [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(and_immediate_high_fullword)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* And fullword operand with high 32 bits of register */
    regs->GR_H(r1) &= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_H(r1) ? 1 : 0;

} /* end DEF_INST(and_immediate_high_fullword) */


/*-------------------------------------------------------------------*/
/* C0xB NILF  - And Immediate Low Fullword                     [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(and_immediate_low_fullword)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* And fullword operand with low 32 bits of register */
    regs->GR_L(r1) &= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_L(r1) ? 1 : 0;

} /* end DEF_INST(and_immediate_low_fullword) */


/*-------------------------------------------------------------------*/
/* C2xD CFI   - Compare Fullword Immediate                     [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_fullword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Compare signed operands and set condition code */
    regs->psw.cc = (S32)regs->GR_L(r1) < (S32)i2 ? 1 :
                   (S32)regs->GR_L(r1) > (S32)i2 ? 2 : 0;

} /* end DEF_INST(compare_fullword_immediate) */


/*-------------------------------------------------------------------*/
/* C2xC CGFI  - Compare Long Fullword Immediate                [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_long_fullword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Compare signed operands and set condition code */
    regs->psw.cc = (S64)regs->GR_G(r1) < (S32)i2 ? 1 :
                   (S64)regs->GR_G(r1) > (S32)i2 ? 2 : 0;

} /* end DEF_INST(compare_long_fullword_immediate) */


/*-------------------------------------------------------------------*/
/* C2xF CLFI  - Compare Logical Fullword Immediate             [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_fullword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = regs->GR_L(r1) < i2 ? 1 :
                   regs->GR_L(r1) > i2 ? 2 : 0;

} /* end DEF_INST(compare_logical_fullword_immediate) */


/*-------------------------------------------------------------------*/
/* C2xE CLGFI - Compare Logical Long Fullword Immediate        [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_long_fullword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = regs->GR_G(r1) < i2 ? 1 :
                   regs->GR_G(r1) > i2 ? 2 : 0;

} /* end DEF_INST(compare_logical_long_fullword_immediate) */


/*-------------------------------------------------------------------*/
/* C0x6 XIHF  - Exclusive Or Immediate High Fullword           [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(exclusive_or_immediate_high_fullword)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* XOR fullword operand with high 32 bits of register */
    regs->GR_H(r1) ^= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_H(r1) ? 1 : 0;

} /* end DEF_INST(exclusive_or_immediate_high_fullword) */


/*-------------------------------------------------------------------*/
/* C0x7 XILF  - Exclusive Or Immediate Low Fullword            [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(exclusive_or_immediate_low_fullword)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* XOR fullword operand with low 32 bits of register */
    regs->GR_L(r1) ^= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_L(r1) ? 1 : 0;

} /* end DEF_INST(exclusive_or_immediate_low_fullword) */


/*-------------------------------------------------------------------*/
/* C0x8 IIHF  - Insert Immediate High Fullword                 [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_immediate_high_fullword)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Insert fullword operand into high 32 bits of register */
    regs->GR_H(r1) = i2;

} /* end DEF_INST(insert_immediate_high_fullword) */


/*-------------------------------------------------------------------*/
/* C0x9 IILF  - Insert Immediate Low Fullword                  [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_immediate_low_fullword)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Insert fullword operand into low 32 bits of register */
    regs->GR_L(r1) = i2;

} /* end DEF_INST(insert_immediate_low_fullword) */


/*-------------------------------------------------------------------*/
/* C0xE LLIHF - Load Logical Immediate High Fullword           [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_immediate_high_fullword)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Load fullword operand into high 32 bits of register
       and set remaining bits to zero */
    regs->GR_H(r1) = i2;
    regs->GR_L(r1) = 0;

} /* end DEF_INST(load_logical_immediate_high_fullword) */


/*-------------------------------------------------------------------*/
/* C0xF LLILF - Load Logical Immediate Low Fullword            [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_immediate_low_fullword)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Load fullword operand into low 32 bits of register
       and set remaining bits to zero */
    regs->GR_G(r1) = i2;

} /* end DEF_INST(load_logical_immediate_low_fullword) */


/*-------------------------------------------------------------------*/
/* C0x1 LGFI  - Load Long Fullword Immediate                   [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(load_long_fullword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Load operand into register */
    regs->GR_G(r1) = (S32)i2;

} /* end DEF_INST(load_long_fullword_immediate) */


/*-------------------------------------------------------------------*/
/* C0xC OIHF  - Or Immediate High Fullword                     [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(or_immediate_high_fullword)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Or fullword operand with high 32 bits of register */
    regs->GR_H(r1) |= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_H(r1) ? 1 : 0;

} /* end DEF_INST(or_immediate_high_fullword) */


/*-------------------------------------------------------------------*/
/* C0xD OILF  - Or Immediate Low Fullword                      [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(or_immediate_low_fullword)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Or fullword operand with low 32 bits of register */
    regs->GR_L(r1) |= i2;

    /* Set condition code according to result */
    regs->psw.cc = regs->GR_L(r1) ? 1 : 0;

} /* end DEF_INST(or_immediate_low_fullword) */


/*-------------------------------------------------------------------*/
/* C2x5 SLFI  - Subtract Logical Fullword Immediate            [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_fullword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical (&(regs->GR_L(r1)),
                                regs->GR_L(r1),
                                i2);

} /* end DEF_INST(subtract_logical_fullword_immediate) */


/*-------------------------------------------------------------------*/
/* C2x4 SLGFI - Subtract Logical Long Fullword Immediate       [RIL] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_long_fullword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U32     i2;                             /* 32-bit operand value      */

    RIL0(inst, regs, r1, opcd, i2);

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc = sub_logical_long(&(regs->GR_G(r1)),
                                      regs->GR_G(r1),
                                      i2);

} /* end DEF_INST(subtract_logical_long_fullword_immediate) */
#endif /*defined(FEATURE_EXTENDED_IMMEDIATE)*/                  /*@Z9*/


#if defined(FEATURE_EXTENDED_IMMEDIATE)                         /*@Z9*/
/*-------------------------------------------------------------------*/
/* E312 LT    - Load and Test                                  [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_test)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_L(r1) = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Set condition code according to value loaded */
    regs->psw.cc = (S32)regs->GR_L(r1) < 0 ? 1 :
                   (S32)regs->GR_L(r1) > 0 ? 2 : 0;

} /* end DEF_INST(load_and_test) */


/*-------------------------------------------------------------------*/
/* E302 LTG   - Load and Test Long                             [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_test_long)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    /* Load R1 register from second operand */
    regs->GR_G(r1) = ARCH_DEP(vfetch8) ( effective_addr2, b2, regs );

    /* Set condition code according to value loaded */
    regs->psw.cc = (S64)regs->GR_G(r1) < 0 ? 1 :
                   (S64)regs->GR_G(r1) > 0 ? 2 : 0;

} /* end DEF_INST(load_and_test_long) */


/*-------------------------------------------------------------------*/
/* B926 LBR   - Load Byte Register                             [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_byte_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Load sign-extended byte from second register */
    regs->GR_L(r1) = (S32)(S8)(regs->GR_LHLCL(r2));

} /* end DEF_INST(load_byte_register) */


/*-------------------------------------------------------------------*/
/* B906 LGBR  - Load Long Byte Register                        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_long_byte_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Load sign-extended byte from second register */
    regs->GR_G(r1) = (S64)(S8)(regs->GR_LHLCL(r2));

} /* end DEF_INST(load_long_byte_register) */


/*-------------------------------------------------------------------*/
/* B927 LHR   - Load Halfword Register                         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_halfword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Load sign-extended halfword from second register */
    regs->GR_L(r1) = (S32)(S16)(regs->GR_LHL(r2));

} /* end DEF_INST(load_halfword_register) */


/*-------------------------------------------------------------------*/
/* B907 LGHR  - Load Long Halfword Register                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_long_halfword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Load sign-extended halfword from second register */
    regs->GR_G(r1) = (S64)(S16)(regs->GR_LHL(r2));

} /* end DEF_INST(load_long_halfword_register) */


/*-------------------------------------------------------------------*/
/* E394 LLC   - Load Logical Character                         [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_character)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    regs->GR_L(r1) = ARCH_DEP(vfetchb) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_logical_character) */


/*-------------------------------------------------------------------*/
/* B994 LLCR  - Load Logical Character Register                [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_character_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Load bits 56-63 from second register and clear bits 32-55 */
    regs->GR_L(r1) = regs->GR_LHLCL(r2);

} /* end DEF_INST(load_logical_character_register) */


/*-------------------------------------------------------------------*/
/* B984 LLGCR - Load Logical Long Character Register           [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_long_character_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Load bits 56-63 from second register and clear bits 0-55 */
    regs->GR_G(r1) = regs->GR_LHLCL(r2);

} /* end DEF_INST(load_logical_long_character_register) */


/*-------------------------------------------------------------------*/
/* E395 LLH   - Load Logical Halfword                          [RXY] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_halfword)
{
int     r1;                             /* Value of R field          */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RXY(inst, regs, r1, b2, effective_addr2);

    regs->GR_L(r1) = ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

} /* end DEF_INST(load_logical_halfword) */


/*-------------------------------------------------------------------*/
/* B995 LLHR  - Load Logical Halfword Register                 [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_halfword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Load bits 48-63 from second register and clear bits 32-47 */
    regs->GR_L(r1) = regs->GR_LHL(r2);

} /* end DEF_INST(load_logical_halfword_register) */


/*-------------------------------------------------------------------*/
/* B985 LLGHR - Load Logical Long Halfword Register            [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(load_logical_long_halfword_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE0(inst, regs, r1, r2);

    /* Load bits 48-63 from second register and clear bits 0-47 */
    regs->GR_G(r1) = regs->GR_LHL(r2);

} /* end DEF_INST(load_logical_long_halfword_register) */


/*-------------------------------------------------------------------*/
/* B983 FLOGR - Find Leftmost One Long Register                [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(find_leftmost_one_long_register)
{
int     r1, r2;                         /* Values of R fields        */
U64     op;                             /* R2 contents               */
U64     mask;                           /* Bit mask for leftmost one */
int     n;                              /* Position of leftmost one  */

    RRE(inst, regs, r1, r2);

    ODD_CHECK(r1, regs);

    /* Load contents of second register */
    op = regs->GR_G(r2);

    /* If R2 contents is all zero, set R1 register to 64,
       set R1+1 register to zero, and return cond code 0 */
    if (op == 0)
    {
        regs->GR_G(r1) = 64;
        regs->GR_G(r1+1) = 0;
        regs->psw.cc = 0;
    }
    else
    {
        /* Find leftmost one */
        for (mask = 0x8000000000000000ULL, n=0;
            n < 64 && (op & mask) == 0; n++, mask >>= 1);

        /* Load leftmost one position into R1 register */
        regs->GR_G(r1) = n;

        /* Copy original R2 to R1+1 and clear leftmost one */
        regs->GR_G(r1+1) = op & (~mask);

        /* Return with condition code 2 */
        regs->psw.cc = 2;
    }

} /* end DEF_INST(find_leftmost_one_long_register) */
#endif /*defined(FEATURE_EXTENDED_IMMEDIATE)*/                  /*@Z9*/

#if defined(FEATURE_LOAD_PROGRAM_PARAMETER_FACILITY)            /* 810 */
/*-------------------------------------------------------------------*/
/* B280 LPP - LOAD PROGRAM PARAMETER                             [S] */
/*-------------------------------------------------------------------*/
DEF_INST(load_program_parameter)
{
int     b2;                             /* Base of effective addr    */
U64     effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);

    FACILITY_CHECK(LOAD_PROG_PARAM,regs);

    PRIV_CHECK(regs);

    /* At least one of these is installed */
    if (FACILITY_ENABLED(CPU_MEAS_COUNTER,regs) ||
        FACILITY_ENABLED(CPU_MEAS_SAMPLNG,regs) )
        sysblk.program_parameter = ARCH_DEP(vfetch8) (effective_addr2, b2, regs);

} /* end DEF_INST(load_program_parameter) */
#endif /* defined(FEATURE_LOAD_PROGRAM_PARAMETER_FACILITY) */   /* 810 */

#if !defined(_GEN_ARCH)

#if defined(_ARCHMODE2)
 #define  _GEN_ARCH _ARCHMODE2
 #include "esame.c"
#endif

#if defined(_ARCHMODE3)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCHMODE3
 #include "esame.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
