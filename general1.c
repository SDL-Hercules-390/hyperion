/* GENERAL1.C   (C) Copyright Roger Bowler, 1994-2012                */
/*              (C) and others 2013-2023                             */
/*              Hercules CPU Emulator - Instructions A-M             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* UPT & CFC                (C) Copyright Peter Kuschnerus, 1999-2009*/
/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements general instructions A-M of the            */
/* S/370 and ESA/390 architectures, as described in the manuals      */
/* GA22-7000-03 System/370 Principles of Operation                   */
/* SA22-7201-06 ESA/390 Principles of Operation                      */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      Corrections to shift instructions by Jay Maynard, Jan Jaeger */
/*      Branch tracing by Jan Jaeger                                 */
/*      Instruction decode by macros - Jan Jaeger                    */
/*      Prevent TOD from going backwards in time - Jan Jaeger        */
/*      Fix STCKE instruction - Bernard van der Helm                 */
/*      Instruction decode rework - Jan Jaeger                       */
/*      Make STCK update the TOD clock - Jay Maynard                 */
/*      Fix address wraparound in MVO - Jan Jaeger                   */
/*      PLO instruction - Jan Jaeger                                 */
/*      Modifications for Interpretive Execution (SIE) by Jan Jaeger */
/*      Clear TEA on data exception - Peter Kuschnerus           v209*/
/*      PER 1 GRA - Fish                                     Jan 2022*/
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _GENERAL1_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

/* When an operation code has unused operand(s) (IPK, e.g.), it will */
/* attract a diagnostic for a set, but unused variable.  Fixing the  */
/* macros to support e.g., RS_NOOPS is not productive, so:           */
DISABLE_GCC_UNUSED_SET_WARNING

/*-------------------------------------------------------------------*/
/* 1A   AR    - Add Register                                    [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* Add signed operands and set condition code */
    regs->psw.cc =
            add_signed (&(regs->GR_L(r1)),
                    regs->GR_L(r1),
                    regs->GR_L(r2));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 5A   A     - Add                                           [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(add)
{
int     r1;                             /* Values of R fields        */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

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

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 4A   AH    - Add Halfword                                  [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(add_halfword)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
S32     n;                              /* 32-bit operand values     */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

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

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


#if defined( FEATURE_IMMEDIATE_AND_RELATIVE )
/*-------------------------------------------------------------------*/
/* A7xA AHI   - Add Halfword Immediate                        [RI-a] */
/*-------------------------------------------------------------------*/
DEF_INST(add_halfword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit immediate op       */

    RI(inst, regs, r1, opcd, i2);

    /* Add signed operands and set condition code */
    regs->psw.cc =
            add_signed (&(regs->GR_L(r1)),
                          regs->GR_L(r1),
                     (S16)i2);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

}
#endif /* defined( FEATURE_IMMEDIATE_AND_RELATIVE ) */


#ifdef OPTION_OPTINST
/*-------------------------------------------------------------------*/
/* 1E   ALR   - Add Logical Register (optimized)                [RR] */
/*-------------------------------------------------------------------*/
#define ALRgen( r1, r2 )                                              \
                                                                      \
  DEF_INST( 1E ## r1 ## r2 )                                          \
  {                                                                   \
    UNREFERENCED( inst );                                             \
                                                                      \
    INST_UPDATE_PSW( regs, 2, 2 );                                    \
                                                                      \
    regs->psw.cc = add_logical                                        \
    (                                                                 \
        &(regs->GR_L( 0x ## r1 )),                                    \
          regs->GR_L( 0x ## r1 ),                                     \
          regs->GR_L( 0x ## r2 )                                      \
    );                                                                \
                                                                      \
    /* Check for PER 1 GRA event */                                   \
    PER_GRA_CHECK( regs, PER_GRA_MASK( 0x ## r1 ));                   \
  }

#define ALRgenr2( r1 )                                                \
                                                                      \
  ALRgen( r1, 0 )                                                     \
  ALRgen( r1, 1 )                                                     \
  ALRgen( r1, 2 )                                                     \
  ALRgen( r1, 3 )                                                     \
  ALRgen( r1, 4 )                                                     \
  ALRgen( r1, 5 )                                                     \
  ALRgen( r1, 6 )                                                     \
  ALRgen( r1, 7 )                                                     \
  ALRgen( r1, 8 )                                                     \
  ALRgen( r1, 9 )                                                     \
  ALRgen( r1, A )                                                     \
  ALRgen( r1, B )                                                     \
  ALRgen( r1, C )                                                     \
  ALRgen( r1, D )                                                     \
  ALRgen( r1, E )                                                     \
  ALRgen( r1, F )

ALRgenr2( 0 )
ALRgenr2( 1 )
ALRgenr2( 2 )
ALRgenr2( 3 )
ALRgenr2( 4 )
ALRgenr2( 5 )
ALRgenr2( 6 )
ALRgenr2( 7 )
ALRgenr2( 8 )
ALRgenr2( 9 )
ALRgenr2( A )
ALRgenr2( B )
ALRgenr2( C )
ALRgenr2( D )
ALRgenr2( E )
ALRgenr2( F )

#endif /* #ifdef OPTION_OPTINST */


/*-------------------------------------------------------------------*/
/* 1E   ALR   - Add Logical Register                            [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* Add signed operands and set condition code */
    regs->psw.cc =
            add_logical (&(regs->GR_L(r1)),
                    regs->GR_L(r1),
                    regs->GR_L(r2));

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 14   NR    - And Register                                    [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(and_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* AND second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_L(r1) &= regs->GR_L(r2) ) ? 1 : 0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 94   NI    - And Immediate                                   [SI] */
/*-------------------------------------------------------------------*/
DEF_INST(and_immediate)
{
BYTE    i2;                             /* Immediate byte of opcode  */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
BYTE   *dest;                           /* Pointer to target byte    */

    SI(inst, regs, i2, b1, effective_addr1);
    PER_ZEROADDR_XCHECK( regs, b1 );
    ITIMER_SYNC(effective_addr1, 0, regs);

    /* Get byte mainstor address */
    dest = MADDR (effective_addr1, b1, regs, ACCTYPE_WRITE, regs->psw.pkey );

    /* MAINLOCK may be required if cmpxchg assists unavailable */
    OBTAIN_MAINLOCK( regs );
    {
        /* AND byte with immediate operand, setting condition code */
        regs->psw.cc = (H_ATOMIC_OP( dest, i2, and, And, & ) != 0);
    }
    RELEASE_MAINLOCK( regs );

    /* Update interval timer if necessary */
    ITIMER_UPDATE(effective_addr1, 0, regs);
}


/*-------------------------------------------------------------------*/
/* D4   NC    - And Character                                 [SS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( and_character )
{
int     len, len2, len3;                /* Lengths to copy           */
int     b1, b2;                         /* Base register numbers     */
VADR    effective_addr1;                /* Virtual address           */
VADR    effective_addr2;                /* Virtual address           */
BYTE   *dest1, *dest2;                  /* Destination addresses     */
BYTE   *source1, *source2;              /* Source addresses          */
BYTE   *sk1, *sk2;                      /* Storage key addresses     */
int     i;                              /* Loop counter              */
int     cc = 0;                         /* Condition code            */

    SS_L( inst, regs, len, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    TXFC_INSTR_CHECK( regs );

    ITIMER_SYNC( effective_addr2, len, regs );
    ITIMER_SYNC( effective_addr1, len, regs );

    /* Quick out for 1 byte (no boundary crossed) */
    if (unlikely( !len ))
    {
        source1 = MADDR( effective_addr2, b2, regs, ACCTYPE_READ,  regs->psw.pkey );
        dest1   = MADDR( effective_addr1, b1, regs, ACCTYPE_WRITE, regs->psw.pkey );
        *dest1 &= *source1;
        regs->psw.cc = (*dest1 != 0);
        ITIMER_UPDATE( effective_addr1, 0, regs );
        return;
    }

    /* There are several scenarios (in optimal order):
     * (1) dest boundary and source boundary not crossed
     * (2) dest boundary not crossed and source boundary crossed
     * (3) dest boundary crossed and source boundary not crossed
     * (4) dest boundary and source boundary are crossed
     *     (a) dest and source boundary cross at the same time
     *     (b) dest boundary crossed first
     *     (c) source boundary crossed first
     */

    /* Translate addresses of leftmost operand bytes */
    dest1 = MADDRL( effective_addr1, len+1, b1, regs, ACCTYPE_WRITE_SKP, regs->psw.pkey );
    sk1 = regs->dat.storkey;
    source1 = MADDRL( effective_addr2, len + 1, b2, regs, ACCTYPE_READ, regs->psw.pkey );

    if (NOCROSSPAGE( effective_addr1,len ))
    {
        if (NOCROSSPAGE( effective_addr2,len ))
        {
            /* (1) - No boundaries are crossed */
            for (i=0; i <= len; i++)
                if (*dest1++ &= *source1++)
                    cc = 1;
        }
        else
        {
             /* (2) - Second operand crosses a boundary */
             len2 = PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK);
             source2 = MADDRL( (effective_addr2 + len2) & ADDRESS_MAXWRAP( regs ),
                 len + 1 - len2, b2, regs, ACCTYPE_READ, regs->psw.pkey );
             for (i=0; i < len2; i++)
                 if (*dest1++ &= *source1++)
                     cc = 1;

             len2 = len - len2;

             for (i=0; i <= len2; i++)
                 if (*dest1++ &= *source2++)
                     cc = 1;
        }
        ARCH_DEP( or_storage_key_by_ptr )( sk1, (STORKEY_REF | STORKEY_CHANGE) );
    }
    else
    {
        /* First operand crosses a boundary */
        len2 = PAGEFRAME_PAGESIZE - (effective_addr1 & PAGEFRAME_BYTEMASK);
        dest2 = MADDRL( (effective_addr1 + len2) & ADDRESS_MAXWRAP( regs ),
            len + 1 - len2, b1, regs, ACCTYPE_WRITE_SKP, regs->psw.pkey );
        sk2 = regs->dat.storkey;

        if (NOCROSSPAGE( effective_addr2,len ))
        {
             /* (3) - First operand crosses a boundary */
             for (i=0; i < len2; i++)
                 if (*dest1++ &= *source1++)
                     cc = 1;

             len2 = len - len2;

             for (i=0; i <= len2; i++)
                 if (*dest2++ &= *source1++)
                     cc = 1;
        }
        else
        {
            /* (4) - Both operands cross a boundary */
            len3 = PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK);
            source2 = MADDRL( (effective_addr2 + len3) & ADDRESS_MAXWRAP( regs ),
                len + 1 - len3, b2, regs, ACCTYPE_READ, regs->psw.pkey );
            if (len2 == len3)
            {
                /* (4a) - Both operands cross at the same time */
                for (i=0; i < len2; i++)
                    if (*dest1++ &= *source1++)
                        cc = 1;

                len2 = len - len2;

                for (i=0; i <= len2; i++)
                    if (*dest2++ &= *source2++)
                        cc = 1;
            }
            else if (len2 < len3)
            {
                /* (4b) - First operand crosses first */
                for (i=0; i < len2; i++)
                    if (*dest1++ &= *source1++)
                        cc = 1;

                len2 = len3 - len2;

                for (i=0; i < len2; i++)
                    if (*dest2++ &= *source1++)
                        cc = 1;

                len2 = len - len3;

                for (i=0; i <= len2; i++)
                    if (*dest2++ &= *source2++)
                        cc = 1;
            }
            else
            {
                /* (4c) - Second operand crosses first */
                for (i=0; i < len3; i++)
                    if (*dest1++ &= *source1++)
                        cc = 1;

                len3 = len2 - len3;

                for (i=0; i < len3; i++)
                    if (*dest1++ &= *source2++)
                        cc = 1;

                len3 = len - len2;

                for (i=0; i <= len3; i++)
                    if (*dest2++ &= *source2++)
                        cc = 1;
            }
        }
        ARCH_DEP( or_storage_key_by_ptr )( sk1, (STORKEY_REF | STORKEY_CHANGE) );
        ARCH_DEP( or_storage_key_by_ptr )( sk2, (STORKEY_REF | STORKEY_CHANGE) );
    }
    ITIMER_UPDATE( effective_addr1, len, regs );

    regs->psw.cc = cc;
}


/*-------------------------------------------------------------------*/
/* 05   BALR  - Branch and Link Register                        [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_and_link_register)
{
int     r1, r2;                         /* Values of R fields        */
VADR    newia;                          /* New instruction address   */

    RR_B(inst, regs, r1, r2);

    TXFC_INSTR_CHECK_IP( regs );
    TXF_NONRELATIVE_BRANCH_CHECK_IP( regs, r2 );

#if defined( FEATURE_TRACING )
    /* Add a branch trace entry to the trace table */
    if ((regs->CR(12) & CR12_BRTRACE) && (r2 != 0))
    {
        regs->psw.ilc = 0; // indicates regs->ip not updated
        regs->CR(12) = ARCH_DEP(trace_br) (regs->psw.amode, regs->GR_L(r2), regs);
        regs->psw.ilc = 2; // reset if trace didn't pgm check
    }
#endif /* defined( FEATURE_TRACING ) */

    /* Compute the branch address from the R2 operand */
    newia = regs->GR(r2);

    /* Save the link information in the R1 operand */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if( regs->psw.amode64 )
        regs->GR_G(r1) = PSW_IA64(regs, 2);
    else
#endif
    regs->GR_L(r1) =
        ( regs->psw.amode )
        ? (0x80000000                 | PSW_IA31(regs, 2))
        : (((likely(!regs->execflag) ? 2 : regs->exrl ? 6 : 4) << 29)
        |  (regs->psw.cc << 28)       | (regs->psw.progmask << 24)
        |  PSW_IA24(regs, 2));

    /* Execute the branch unless R2 specifies register 0 */
    if ( r2 != 0 )
        SUCCESSFUL_BRANCH( regs, newia );
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 2;
    }

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST(branch_and_link_register) */


/*-------------------------------------------------------------------*/
/* 45   BAL   - Branch and Link                               [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_and_link)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RX_B(inst, regs, r1, x2, b2, effective_addr2);

    TXFC_INSTR_CHECK_IP( regs );

    /* Save the link information in the R1 operand */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if( regs->psw.amode64 )
        regs->GR_G(r1) = PSW_IA64(regs, 4);
    else
#endif
    regs->GR_L(r1) = regs->psw.amode ? (0x80000000 | PSW_IA31( regs, 4 ))
        : (   (((U32)4)                  << 29)
            | (((U32)regs->psw.cc)       << 28)
            | (((U32)regs->psw.progmask) << 24)
            | PSW_IA24( regs, 4 )
          );

    SUCCESSFUL_BRANCH( regs, effective_addr2 );

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST(branch_and_link) */


/*-------------------------------------------------------------------*/
/* 0D   BASR  - Branch and Save Register                        [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_and_save_register)
{
int     r1, r2;                         /* Values of R fields        */
VADR    newia;                          /* New instruction address   */

    RR_B(inst, regs, r1, r2);

    TXFC_INSTR_CHECK_IP( regs );
    TXF_NONRELATIVE_BRANCH_CHECK_IP( regs, r2 );

#if defined( FEATURE_TRACING )
    /* Add a branch trace entry to the trace table */
    if ((regs->CR(12) & CR12_BRTRACE) && (r2 != 0))
    {
        regs->psw.ilc = 0; // indcates regs->ip not updated
        regs->CR(12) = ARCH_DEP(trace_br) (regs->psw.amode, regs->GR_L(r2), regs);
        regs->psw.ilc = 2; // reset if trace didn't pgm check
    }
#endif /* defined( FEATURE_TRACING ) */

    /* Compute the branch address from the R2 operand */
    newia = regs->GR(r2);

    /* Save the link information in the R1 operand */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if ( regs->psw.amode64 )
        regs->GR_G(r1) = PSW_IA64(regs, 2);
    else
#endif
    if ( regs->psw.amode )
        regs->GR_L(r1) = 0x80000000 | PSW_IA31(regs, 2);
    else
        regs->GR_L(r1) = PSW_IA24(regs, 2);

    /* Execute the branch unless R2 specifies register 0 */
    if ( r2 != 0 )
        SUCCESSFUL_BRANCH( regs, newia );
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 2;
    }

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST(branch_and_save_register) */


/*-------------------------------------------------------------------*/
/* 4D   BAS   - Branch and Save                               [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_and_save)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RX_B(inst, regs, r1, x2, b2, effective_addr2);

    TXFC_INSTR_CHECK_IP( regs );

    /* Save the link information in the R1 register */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if ( regs->psw.amode64 )
        regs->GR_G(r1) = PSW_IA64(regs, 4);
    else
#endif
    if ( regs->psw.amode )
        regs->GR_L(r1) = 0x80000000 | PSW_IA31(regs, 4);
    else
        regs->GR_L(r1) = PSW_IA24(regs, 4);

    SUCCESSFUL_BRANCH( regs, effective_addr2 );

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST(branch_and_save) */


#if defined( FEATURE_BIMODAL_ADDRESSING ) || defined( FEATURE_370_EXTENSION )
/*-------------------------------------------------------------------*/
/* 0C   BASSM - Branch and Save and Set Mode                    [RR] */
/*-------------------------------------------------------------------*/
DEF_INST( branch_and_save_and_set_mode )
{
int     r1, r2;                         /* Values of R fields        */
VADR    newia;                          /* New instruction address   */
#if !defined( FEATURE_370_EXTENSION )
int     xmode;                          /* 64 or 31 mode of target   */
#endif

    RR_B( inst, regs, r1, r2 );

    TXFC_INSTR_CHECK_IP( regs );
    TXF_NONRELATIVE_BRANCH_CHECK_IP( regs, r2 );
    TXF_BRANCH_SET_MODE_CHECK_IP( regs, r2 );

    /* Compute the branch address from the R2 operand */
    newia = regs->GR( r2 );

#if defined( FEATURE_TRACING )

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    /* Add a mode trace entry when switching in/out of 64 bit mode */
    if (1
        && regs->CR(12) & CR12_MTRACE
        && r2
        && regs->psw.amode64 != (newia & 1)
    )
    {
        /* save ip and update it for mode switch trace */
        BYTE* ipsav = regs->ip;
        regs->ip += 2;
        regs->CR(12) = ARCH_DEP(trace_ms) (regs->CR(12) & CR12_BRTRACE ? 1 : 0,
                                           newia & ~0x01, regs);
        regs->ip = ipsav;
    }
    else
#endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

    /* Add a branch trace entry to the trace table */
    if ((regs->CR(12) & CR12_BRTRACE) && (r2 != 0))
    {
        regs->psw.ilc = 0; // indicates regs->ip not updated
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (newia & 0x01)
            xmode = 1;
        else
#endif
        xmode = newia & 0x80000000 ? 1 : 0;
        regs->CR(12) = ARCH_DEP( trace_br )( xmode, newia & ~0x01, regs );
        regs->psw.ilc = 2; // reset if trace didn't pgm check
    }

#endif /* defined( FEATURE_TRACING ) */

    /* Save the link information in the R1 operand */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if (regs->psw.amode64)
        regs->GR_G( r1 ) = PSW_IA64( regs, 3 ); // low bit on
    else
#endif
    if (regs->psw.amode)
        regs->GR_L( r1 ) = 0x80000000 | PSW_IA31( regs, 2 );
    else
        regs->GR_L( r1 ) = 0x00000000 | PSW_IA24( regs, 2 );

    /* Set mode and branch to address specified by R2 operand */
    if (r2)
    {
#if !defined( FEATURE_370_EXTENSION )
        SET_ADDRESSING_MODE( regs, newia );
#endif
        SUCCESSFUL_BRANCH( regs, newia );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 2;
    }

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST( branch_and_save_and_set_mode ) */
#endif /* defined( FEATURE_BIMODAL_ADDRESSING ) || defined( FEATURE_370_EXTENSION )*/


#if defined( FEATURE_BIMODAL_ADDRESSING ) || defined( FEATURE_370_EXTENSION )
/*-------------------------------------------------------------------*/
/* 0B   BSM   - Branch and Set Mode                             [RR] */
/*-------------------------------------------------------------------*/
DEF_INST( branch_and_set_mode )
{
int     r1, r2;                         /* Values of R fields        */
VADR    newia;                          /* New instruction address   */

    RR_B( inst, regs, r1, r2 );

    TXFC_INSTR_CHECK_IP( regs );
    TXF_BRANCH_SET_MODE_CHECK_IP( regs, r2 );

    /* Compute the branch address from the R2 operand */
    newia = regs->GR( r2 );

#if defined( FEATURE_TRACING )
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    /* Add a mode trace entry when switching in/out of 64 bit mode */
    if (1
        && regs->CR(12) & CR12_MTRACE
        && r2
        && regs->psw.amode64 != (newia & 1)
    )
    {
        /* save ip and update it for mode switch trace */
        BYTE* ipsav = regs->ip;
        regs->ip += 2;
        regs->CR(12) = ARCH_DEP( trace_ms )( 0, 0, regs );
        regs->ip = ipsav;
    }
#endif
#endif

    /* Insert addressing mode into bit 0 of R1 operand */
    if (r1)
    {
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /* PROGRAMMING NOTE: The original zArchitecture Principles of
           Operation manual (SA22-7832-00 December 2000 First Edition)
           contained an error regarding the functionality of the BSM
           (Branch and Set Mode) instruction. It incorrectly said in
           the 24-bit or 31-bit addressing mode, bit 32 of the current
           PSW was inserted into bit 32 of the first operand register,
           and a zero was inserted into bit position 63 and bits 0-31
           and 33-62 of the operand remained unchanged. This is wrong.
           The October 2001 Second Edition of zArchitecture Principles
           of Operation manual (SA22-7832-01) corrected this error to
           state bit 63 of the operand-1 register remained unchanged
           in the 24-bit and 31-bit addressing modes (i.e. the bit is
           NOT set to zero).
        */
//      regs->GR_LHLCL(r1) &= 0xFE;     // (see PROGRAMMING NOTE)
        if (regs->psw.amode64)
            regs->GR_LHLCL(r1) |= 0x01;
        else
#endif
        {
            if (regs->psw.amode)
                regs->GR_L(r1) |= 0x80000000;
            else
                regs->GR_L(r1) &= 0x7FFFFFFF;
        }
    }

    /* Set mode and branch to address specified by R2 operand */
    if (r2)
    {
#if !defined( FEATURE_370_EXTENSION )
        SET_ADDRESSING_MODE( regs, newia );
#endif
        SUCCESSFUL_BRANCH( regs, newia );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 2;
    }

#if defined( FEATURE_PER1 )
    /* Check for PER 1 GRA event */
    if (r1) // r1 modified?
        PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
#endif

} /* end DEF_INST(branch_and_set_mode) */
#endif /* defined( FEATURE_BIMODAL_ADDRESSING ) || defined( FEATURE_370_EXTENSION )*/


/*-------------------------------------------------------------------*/
/* 07   BCR   - Branch on Condition Register                    [RR] */
/*-------------------------------------------------------------------*/
DEF_INST( branch_on_condition_register )
{
//int   r1, r2;                         /* Values of R fields        */

//  RR( inst, regs, r1, r2 );

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 2;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch if R1 mask bit is set and R2 is not register 0 */
    if ((inst[1] & 0x0F) != 0 && (inst[1] & (0x80 >> regs->psw.cc)))
        SUCCESSFUL_BRANCH( regs, regs->GR( inst[1] & 0x0F ));
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 2;

        /* Perform serialization AND checkpoint synchronization
           if the mask is all ones and the r2 register is 0 */
        if (inst[1] == 0xF0)
        {
#if defined( OPTION_HARDWARE_SYNC_BCR_ONLY )
            HARDWARE_SYNC();
#else // OPTION_HARDWARE_SYNC_ALL
            PERFORM_SERIALIZATION( regs );
            PERFORM_CHKPT_SYNC( regs );
#endif
        }
#if defined( FEATURE_045_FAST_BCR_SERIAL_FACILITY )
        if (FACILITY_ENABLED( 045_FAST_BCR_SERIAL, regs ))
        {
            /* Perform serialization WITHOUT checkpoint synchronization
               if the mask is B'1110' and the r2 register is 0 */
            if (inst[1] == 0xE0)
            {
#if defined( OPTION_HARDWARE_SYNC_BCR_ONLY )
                HARDWARE_SYNC();
#else // OPTION_HARDWARE_SYNC_ALL
                PERFORM_SERIALIZATION( regs );
#endif
            }
        }
#endif
    }

} /* end DEF_INST( branch_on_condition_register ) */


/*-------------------------------------------------------------------*/
/* 42   STC   - Store Character                               [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(store_character)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Store rightmost byte of R1 register at operand address */
    ARCH_DEP(vstoreb) ( regs->GR_LHLCL(r1), effective_addr2, b2, regs );
}


#if defined( OPTION_OPTINST )
/*-------------------------------------------------------------------*/
/* 47_0 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 47_0 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if ((0x80 >> regs->psw.cc) & inst[1])
    {
        RXX_BC( inst, regs, b2, effective_addr2 );
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( 47_0 ) */

/*-------------------------------------------------------------------*/
/* 4700 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( nop4 )
{
    UNREFERENCED( inst );

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Bump ip to next sequential instruction */
    regs->ip += 4;

} /* end DEF_INST( nop4 ) */

/*-------------------------------------------------------------------*/
/* 4710 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 4710 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if (regs->psw.cc == 3)
    {
        RXX_BC( inst, regs, b2, effective_addr2 );
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( 4710 ) */

/*-------------------------------------------------------------------*/
/* 4720 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 4720 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if (regs->psw.cc == 2)
    {
        RXX_BC( inst, regs, b2, effective_addr2 );
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( 4720 ) */

/*-------------------------------------------------------------------*/
/* 4730 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 4730 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if (regs->psw.cc > 1)
    {
        RXX_BC( inst, regs, b2, effective_addr2 );
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( 4730 ) */

/*-------------------------------------------------------------------*/
/* 4740 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 4740 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if (regs->psw.cc == 1)
    {
        RXX_BC( inst, regs, b2, effective_addr2 );
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( 4740 ) */

/*-------------------------------------------------------------------*/
/* 4750 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 4750 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if (regs->psw.cc & 0x01)
    {
        RXX_BC( inst, regs, b2, effective_addr2 );
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( 4750 ) */

/*-------------------------------------------------------------------*/
/* 4770 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 4770 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if (regs->psw.cc)
    {
        RXX_BC( inst, regs, b2, effective_addr2 );
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( 4770 ) */

/*-------------------------------------------------------------------*/
/* 4780 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 4780 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if (!regs->psw.cc)
    {
        RXX_BC( inst, regs, b2, effective_addr2 );
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( 4780 ) */

/*-------------------------------------------------------------------*/
/* 47A0 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 47A0 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if (!(regs->psw.cc & 0x01))
    {
        RXX_BC( inst, regs, b2, effective_addr2 );
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( 47A0 ) */

/*-------------------------------------------------------------------*/
/* 47B0 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 47B0 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if (regs->psw.cc != 1)
    {
        RXX_BC( inst, regs, b2, effective_addr2 );
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( 47B0 ) */

/*-------------------------------------------------------------------*/
/* 47C0 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 47C0 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if (regs->psw.cc < 2)
    {
        RXX_BC( inst, regs, b2, effective_addr2 );
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( 47C0 ) */

/*-------------------------------------------------------------------*/
/* 47D0 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 47D0 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if (regs->psw.cc != 2)
    {
        RXX_BC( inst, regs, b2, effective_addr2 );
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( 47D0 ) */

/*-------------------------------------------------------------------*/
/* 47E0 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 47E0 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if (regs->psw.cc != 3)
    {
        RXX_BC( inst, regs, b2, effective_addr2 );
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( 47E0 ) */

/*-------------------------------------------------------------------*/
/* 47F0 BC    - Branch on Condition (optimized)               [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( 47F0 )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    RXX_BC( inst, regs, b2, effective_addr2 );
    SUCCESSFUL_BRANCH( regs, effective_addr2 );

} /* end DEF_INST( 47F0 ) */
#endif /* defined( OPTION_OPTINST ) */


/*-------------------------------------------------------------------*/
/* 47   BC    - Branch on Condition                           [RX_b] */
/*-------------------------------------------------------------------*/
DEF_INST( branch_on_condition )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_INSTR_CHECK_IP( regs );

    /* Branch to operand address if r1 mask bit is set */
    if ((0x80 >> regs->psw.cc) & inst[1])
    {
#if defined( OPTION_OPTINST )
        RXXx_BC( inst, regs, b2, effective_addr2 );
#else
        RX_BC( inst, regs, b2, effective_addr2 );
#endif
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( branch_on_condition ) */


/*-------------------------------------------------------------------*/
/* 54   N     - And                                           [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(and)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* AND second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_L(r1) &= n ) ? 1 : 0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


#ifdef OPTION_OPTINST
/*-------------------------------------------------------------------*/
/* 58   L     - Load (optimized)                              [RX-a] */
/*-------------------------------------------------------------------*/
#define Lgen( r1 )                                                    \
                                                                      \
  DEF_INST( 58 ## r1 ## 0 )                                           \
  {                                                                   \
    int b2;                                                           \
    VADR effective_addr2;                                             \
                                                                      \
    RXX0RX( inst, regs, b2, effective_addr2 );                        \
    PER_ZEROADDR_XCHECK( regs, b2 );                                  \
                                                                      \
    regs->GR_L( 0x ## r1 ) =                                          \
        ARCH_DEP( vfetch4 )( effective_addr2, b2, regs );             \
                                                                      \
    /* Check for PER 1 GRA event */                                   \
    PER_GRA_CHECK( regs, PER_GRA_MASK( 0x ## r1 ));                   \
  }

Lgen( 0 )
Lgen( 1 )
Lgen( 2 )
Lgen( 3 )
Lgen( 4 )
Lgen( 5 )
Lgen( 6 )
Lgen( 7 )
Lgen( 8 )
Lgen( 9 )
Lgen( A )
Lgen( B )
Lgen( C )
Lgen( D )
Lgen( E )
Lgen( F )

#endif /* OPTION_OPTINST */


/*-------------------------------------------------------------------*/
/* 58   L     - Load                                          [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(load)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

#ifdef OPTION_OPTINST
    RXXx(inst, regs, r1, x2, b2, effective_addr2);
#else
    RX(inst, regs, r1, x2, b2, effective_addr2);
#endif
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Load R1 register from second operand */
    regs->GR_L(r1) = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST(load) */


#ifdef OPTION_OPTINST
/*-------------------------------------------------------------------*/
/* 50   ST    - Store (optimized)                             [RX-a] */
/*-------------------------------------------------------------------*/
#define STgen( r1 )                                                   \
                                                                      \
  DEF_INST( 50 ## r1 ## 0 )                                           \
  {                                                                   \
      int b2;                                                         \
      VADR effective_addr2;                                           \
                                                                      \
      RXX0RX( inst, regs, b2, effective_addr2 );                      \
      PER_ZEROADDR_XCHECK( regs, b2 );                                \
                                                                      \
      ARCH_DEP( vstore4 )( regs->GR_L( 0x ## r1 ),                    \
                           effective_addr2, b2, regs );               \
  }

STgen( 0 )
STgen( 1 )
STgen( 2 )
STgen( 3 )
STgen( 4 )
STgen( 5 )
STgen( 6 )
STgen( 7 )
STgen( 8 )
STgen( 9 )
STgen( A )
STgen( B )
STgen( C )
STgen( D )
STgen( E )
STgen( F )

#endif /* OPTION_OPTINST */


/*-------------------------------------------------------------------*/
/* 50   ST    - Store                                         [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(store)
{
int     r1;                             /* Values of R fields        */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

#ifdef OPTION_OPTINST
    RXXx(inst, regs, r1, x2, b2, effective_addr2);
#else
    RX(inst, regs, r1, x2, b2, effective_addr2);
#endif
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Store register contents at operand address */
    ARCH_DEP(vstore4) ( regs->GR_L(r1), effective_addr2, b2, regs );

} /* end DEF_INST(store) */


#ifdef OPTION_OPTINST
/*-------------------------------------------------------------------*/
/* 41   LA    - Load Address (optimized)                      [RX-a] */
/*-------------------------------------------------------------------*/
#define LAgen( r1 )                                                   \
                                                                      \
  DEF_INST( 41 ## r1 ## 0 )                                           \
  {                                                                   \
    int b2;                                                           \
    VADR effective_addr2;                                             \
                                                                      \
    RXX0RX( inst, regs, b2, effective_addr2 );                        \
                                                                      \
    SET_GR_A( 0x ## r1, regs, effective_addr2 );                      \
                                                                      \
    /* Check for PER 1 GRA event */                                   \
    PER_GRA_CHECK( regs, PER_GRA_MASK( 0x ## r1 ));                   \
  }

LAgen( 0 )
LAgen( 1 )
LAgen( 2 )
LAgen( 3 )
LAgen( 4 )
LAgen( 5 )
LAgen( 6 )
LAgen( 7 )
LAgen( 8 )
LAgen( 9 )
LAgen( A )
LAgen( B )
LAgen( C )
LAgen( D )
LAgen( E )
LAgen( F )

#endif /* #ifdef OPTION_OPTINST */


/*-------------------------------------------------------------------*/
/* 41   LA    - Load Address                                  [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(load_address)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

#ifdef OPTION_OPTINST
    RXXx(inst, regs, r1, x2, b2, effective_addr2);
#else
    RX(inst, regs, r1, x2, b2, effective_addr2);
#endif

    /* Load operand address into register */
    SET_GR_A(r1, regs, effective_addr2);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 43   IC    - Insert Character                              [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_character)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Insert character in r1 register */
    regs->GR_LHLCL(r1) = ARCH_DEP(vfetchb) ( effective_addr2, b2, regs );

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 5E   AL    - Add Logical                                   [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(add_logical)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Add signed operands and set condition code */
    regs->psw.cc =
            add_logical (&(regs->GR_L(r1)),
                    regs->GR_L(r1),
                    n);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


#ifdef OPTION_OPTINST
/*-------------------------------------------------------------------*/
/* 55   CL    - Compare Logical (optimized)                   [RX-a] */
/*-------------------------------------------------------------------*/
#define CLgen( r1 )                                                   \
                                                                      \
  DEF_INST( 55 ## r1 ## 0 )                                           \
  {                                                                   \
      int b2;                                                         \
      VADR effective_addr2;                                           \
      U32 n;                                                          \
                                                                      \
      RXX0RX( inst, regs, b2, effective_addr2 );                      \
      PER_ZEROADDR_XCHECK( regs, b2 );                                \
                                                                      \
      n = ARCH_DEP( vfetch4 )( effective_addr2, b2, regs );           \
                                                                      \
      regs->psw.cc = regs->GR_L( 0x ## r1 ) < n ? 1 :                 \
                     regs->GR_L( 0x ## r1 ) > n ? 2 : 0;              \
   }

CLgen( 0 )
CLgen( 1 )
CLgen( 2 )
CLgen( 3 )
CLgen( 4 )
CLgen( 5 )
CLgen( 6 )
CLgen( 7 )
CLgen( 8 )
CLgen( 9 )
CLgen( A )
CLgen( B )
CLgen( C )
CLgen( D )
CLgen( E )
CLgen( F )

#endif /* #ifdef OPTION_OPTINST */


/*-------------------------------------------------------------------*/
/* 55   CL    - Compare Logical                               [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical)
{
int     r1;                             /* Values of R fields        */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

#ifdef OPTION_OPTINST
    RXXx(inst, regs, r1, x2, b2, effective_addr2);
#else
    RX(inst, regs, r1, x2, b2, effective_addr2);
#endif
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = regs->GR_L(r1) < n ? 1 :
                   regs->GR_L(r1) > n ? 2 : 0;
}


/*-------------------------------------------------------------------*/
/* 48   LH    - Load Halfword                                 [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(load_halfword)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Load rightmost 2 bytes of register from operand address */
    regs->GR_L(r1) = (S16)ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


#if defined( FEATURE_IMMEDIATE_AND_RELATIVE )
/*-------------------------------------------------------------------*/
/* A7x4 BRC   - Branch Relative on Condition                  [RI-c] */
/*-------------------------------------------------------------------*/
DEF_INST( branch_relative_on_condition )
{
S16  ri2;                               /* 16-bit relative operand   */

    /* Ensure ilc is always accurate */
    regs->psw.ilc = 4;

    TXFC_RELATIVE_BRANCH_CHECK_IP( regs );

    /* Branch if R1 mask bit is set */
    if (inst[1] & (0x80 >> regs->psw.cc))
    {
        ri2 = fetch_hw( &inst[2] );
        SUCCESSFUL_RELATIVE_BRANCH( regs, 2LL*ri2 );
    }
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( branch_relative_on_condition ) */
#endif /* defined( FEATURE_IMMEDIATE_AND_RELATIVE ) */


/*-------------------------------------------------------------------*/
/* 06   BCTR  - Branch on Count Register                        [RR] */
/*-------------------------------------------------------------------*/
DEF_INST( branch_on_count_register )
{
int     r1, r2;                         /* Values of R fields        */
VADR    newia;                          /* New instruction address   */

    RR_B( inst, regs, r1, r2 );

    TXFC_INSTR_CHECK_IP( regs );

    /* Compute the branch address from the R2 operand */
    newia = regs->GR(r2);

    /* Subtract 1 from the R1 operand and branch if result
           is non-zero and R2 operand is not register zero */
    if (--(regs->GR_L( r1 )) && r2)
        SUCCESSFUL_BRANCH( regs, newia );
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 2;
    }

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST( branch_on_count_register ) */


/*-------------------------------------------------------------------*/
/* 46   BCT   - Branch on Count                               [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_on_count)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RX_B(inst, regs, r1, x2, b2, effective_addr2);

    TXFC_INSTR_CHECK_IP( regs );

    /* Subtract 1 from the R1 operand and branch if non-zero */
    if ( --(regs->GR_L(r1)) )
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST(branch_on_count) */


/*-------------------------------------------------------------------*/
/* 86   BXH   - Branch on Index High                          [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_on_index_high)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
S32     i, j;                           /* Integer work areas        */

    RS_B(inst, regs, r1, r3, b2, effective_addr2);

    TXFC_INSTR_CHECK_IP( regs );

    /* Load the increment value from the R3 register */
    i = (S32)regs->GR_L(r3);

    /* Load compare value from R3 (if R3 odd), or R3+1 (if even) */
    j = (r3 & 1) ? (S32)regs->GR_L(r3) : (S32)regs->GR_L(r3+1);

    /* Add the increment value to the R1 register */
    regs->GR_L(r1) = (S32)regs->GR_L(r1) + i;

    /* Branch if result compares high */
    if ( (S32)regs->GR_L(r1) > j )
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST(branch_on_index_high) */


/*-------------------------------------------------------------------*/
/* 87   BXLE  - Branch on Index Low or Equal                  [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(branch_on_index_low_or_equal)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
S32     i, j;                           /* Integer work areas        */

    RS_B(inst, regs, r1, r3, b2, effective_addr2);

    TXFC_INSTR_CHECK_IP( regs );

    /* Load the increment value from the R3 register */
    i = regs->GR_L(r3);

    /* Load compare value from R3 (if R3 odd), or R3+1 (if even) */
    j = (r3 & 1) ? (S32)regs->GR_L(r3) : (S32)regs->GR_L(r3+1);

    /* Add the increment value to the R1 register */
    regs->GR_L(r1) = (S32)regs->GR_L(r1) + i;

    /* Branch if result compares low or equal */
    if ( (S32)regs->GR_L(r1) <= j )
        SUCCESSFUL_BRANCH( regs, effective_addr2 );
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST(branch_on_index_low_or_equal) */


#if defined( FEATURE_IMMEDIATE_AND_RELATIVE )
/*-------------------------------------------------------------------*/
/* A7x5 BRAS  - Branch Relative And Save                      [RI-b] */
/*-------------------------------------------------------------------*/
DEF_INST( branch_relative_and_save )
{
int     r1;                             /* Register number           */
int     xop;                            /* Extended opcode           */
S16     ri2;                            /* 16-bit relative operand   */

    RI_B( inst, regs, r1, xop, ri2 );

    TXFC_INSTR_CHECK_IP( regs );

    /* Save the link information in the R1 operand */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if (regs->psw.amode64)
        regs->GR_G(r1) = PSW_IA64( regs, 4 );
    else
#endif
    if (regs->psw.amode)
        regs->GR_L(r1) = 0x80000000 | PSW_IA31( regs, 4 );
    else
        regs->GR_L(r1) = 0x00000000 | PSW_IA24( regs, 4 );

    SUCCESSFUL_RELATIVE_BRANCH( regs, 2LL*ri2 );

} /* end DEF_INST( branch_relative_and_save ) */
#endif /* defined( FEATURE_IMMEDIATE_AND_RELATIVE ) */


#if defined( FEATURE_IMMEDIATE_AND_RELATIVE )
/*-------------------------------------------------------------------*/
/* A7x6 BRCT  - Branch Relative on Count                      [RI-b] */
/*-------------------------------------------------------------------*/
DEF_INST( branch_relative_on_count )
{
int     r1;                             /* Register number           */
int     xop;                            /* Extended opcode           */
S16     ri2;                            /* 16-bit relative operand   */

    RI_B( inst, regs, r1, xop, ri2 );

    TXFC_INSTR_CHECK_IP( regs );

    /* Subtract 1 from the R1 operand and branch if non-zero */
    if (--(regs->GR_L( r1 )))
        SUCCESSFUL_RELATIVE_BRANCH( regs, 2LL*ri2 );
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( branch_relative_on_count ) */
#endif /* defined( FEATURE_IMMEDIATE_AND_RELATIVE ) */


#if defined( FEATURE_IMMEDIATE_AND_RELATIVE )
/*-------------------------------------------------------------------*/
/* 84   BRXH  - Branch Relative on Index High                  [RSI] */
/*-------------------------------------------------------------------*/
DEF_INST( branch_relative_on_index_high )
{
int     r1, r3;                         /* Register numbers          */
S16     ri2;                            /* 16-bit relative operand   */
S32     i,j;                            /* Integer workareas         */

    RI_B( inst, regs, r1, r3, ri2 );

    TXFC_INSTR_CHECK_IP( regs );

    /* Load the increment value from the R3 register */
    i = (S32)regs->GR_L( r3 );

    /* Load compare value from R3 (if R3 odd), or R3+1 (if even) */
    j = (r3 & 1) ? (S32)regs->GR_L( r3 ) : (S32)regs->GR_L( r3+1 );

    /* Add the increment value to the R1 register */
    regs->GR_L( r1 ) = (S32)regs->GR_L( r1 ) + i;

    /* Branch if result compares high */
    if ((S32) regs->GR_L( r1 ) > j)
        SUCCESSFUL_RELATIVE_BRANCH( regs, 2LL*ri2 );
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( branch_relative_on_index_high ) */
#endif /* defined( FEATURE_IMMEDIATE_AND_RELATIVE ) */


#if defined( FEATURE_IMMEDIATE_AND_RELATIVE )
/*-------------------------------------------------------------------*/
/* 85   BRXLE - Branch Relative on Index Low or Equal          [RSI] */
/*-------------------------------------------------------------------*/
DEF_INST( branch_relative_on_index_low_or_equal )
{
int     r1, r3;                         /* Register numbers          */
S16     ri2;                            /* 16-bit relative operand   */
S32     i,j;                            /* Integer workareas         */

    RI_B( inst, regs, r1, r3, ri2 );

    TXFC_INSTR_CHECK_IP( regs );

    /* Load the increment value from the R3 register */
    i = (S32)regs->GR_L( r3 );

    /* Load compare value from R3 (if R3 odd), or R3+1 (if even) */
    j = (r3 & 1) ? (S32)regs->GR_L( r3 ) : (S32)regs->GR_L( r3+1 );

    /* Add the increment value to the R1 register */
    regs->GR_L( r1 ) = (S32)regs->GR_L( r1 ) + i;

    /* Branch if result compares low or equal */
    if ((S32) regs->GR_L( r1 ) <= j)
        SUCCESSFUL_RELATIVE_BRANCH( regs, 2LL*ri2 );
    else
    {
        /* Bump ip to next sequential instruction */
        regs->ip += 4;
    }

} /* end DEF_INST( branch_relative_on_index_low_or_equal ) */
#endif /* defined( FEATURE_IMMEDIATE_AND_RELATIVE ) */


#if defined( FEATURE_CHECKSUM_INSTRUCTION )
/*-------------------------------------------------------------------*/
/* B241 CKSM  - Checksum                                       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(checksum)
{
int     r1, r2;                         /* Values of R fields        */
VADR    addr2;                          /* Address of second operand */
GREG    len;                            /* Operand length            */
int     i, j;                           /* Loop counters             */
int     cc = 0;                         /* Condition code            */
U32     cpu_length;                     /* CPU determined length     */
U32     n;                              /* Word loaded from operand  */
U64     dreg;                           /* Checksum accumulator      */
BYTE    *main2;                         /* Operand-2 mainstor addr   */

    RRE(inst, regs, r1, r2);
    PER_ZEROADDR_LCHECK( regs, r2, r2+1 );

    TXFC_INSTR_CHECK( regs );
    ODD_CHECK(r2, regs);

    /* Obtain the second operand address and length from R2, R2+1 */
    addr2 = regs->GR( r2 ) & ADDRESS_MAXWRAP( regs );
    len = GR_A( r2+1, regs );

    /* Initialize the checksum from the first operand register */
    dreg = regs->GR_L( r1 );

    /* Set the minimum CPU determined length per the specification  */
    cpu_length = min(4, len);

    /* Should the second operand cross a page boundary with the minimum
       length or if the specified length is within the minimum, process
       the remaining length byte by byte so as to observe possible
       access exceptions */
    if (0
        || len <= 4
        || CROSSPAGEL( addr2, cpu_length )
       )
    {
        /* Fetch fullword from second operand */
        if (cpu_length == 4)
        {
            n = ARCH_DEP(vfetch4) ( addr2, r2, regs );
            addr2 += 4;
            addr2 &= ADDRESS_MAXWRAP( regs );
            len -= 4;
        }
        else
        {
            /* Fetch final 1, 2, or 3 bytes and pad with zeroes */
            for (j = 0, n = 0; j < 4; j++)
            {
                n <<= 8;
                if (cpu_length > 0)
                {
                    n |= ARCH_DEP(vfetchb) ( addr2, r2, regs );
                    addr2++;
                    addr2 &= ADDRESS_MAXWRAP( regs );
                    len--;
                    cpu_length--;
                }
            } /* end for(j) */
        }

        /* Accumulate the fullword into the checksum */
        dreg += n;

        /* Carry 32 bit overflow into bit 31 */
        if (dreg > 0xFFFFFFFFULL)
        {
            dreg &= 0xFFFFFFFFULL;
            dreg++;
        }

    }
    else
    {
        /* Here if a page boundary would not be crossed with the
           minimum length and if the specified length is more than the
           minimum.  Extend the CPU determined length out to the end
           of the page, but not longer than the specified length in
           register r2+1  */
        cpu_length = PAGEFRAME_PAGESIZE - ( addr2 & PAGEFRAME_BYTEMASK );
        cpu_length = min( cpu_length, len );
        main2 = MADDRL(addr2, cpu_length, r2, regs, ACCTYPE_READ, regs->psw.pkey );

        /* Compute number of 4-byte elements we can process within this page
           and compute the number of bytes that will be processed.  Remainder
           bytes (if any) will be processed on the next instruction
           execution. */
        i = cpu_length / 4;
        cpu_length = i * 4;

        for (j=0; j < i; j++)
        {
            /* Fetch fullword from second operand */
            n = fetch_fw( main2 );
            main2 += 4;

            /* Accumulate the fullword into the checksum */
            dreg += n;

            /* Carry 32 bit overflow into bit 31 */
            if (dreg > 0xFFFFFFFFULL)
            {
                dreg &= 0xFFFFFFFFULL;
                dreg++;
            }
        } /* end for(j) */

        /* Adjust the operand address and remaining length for the
           number of bytes processed */
        addr2 += cpu_length;
        addr2 &= ADDRESS_MAXWRAP( regs );
        len -= cpu_length;

    } /* end else */

    /* Load the updated checksum into the R1 register */
    regs->GR_L( r1 ) = dreg;

    /* Update the operand address and length registers */
    SET_GR_A( r2, regs, addr2 );
    SET_GR_A( r2+1, regs, len );

    /* Set condition code 0 or 3 */
    if (len > 0) cc = 3;
    regs->psw.cc = cc;
}
#endif /* defined( FEATURE_CHECKSUM_INSTRUCTION ) */


/*-------------------------------------------------------------------*/
/* 19   CR    - Compare Register                                [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* Compare signed operands and set condition code */
    regs->psw.cc =
                (S32)regs->GR_L(r1) < (S32)regs->GR_L(r2) ? 1 :
                (S32)regs->GR_L(r1) > (S32)regs->GR_L(r2) ? 2 : 0;
}


/*-------------------------------------------------------------------*/
/* 59   C     - Compare                                       [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(compare)
{
int     r1;                             /* Values of R fields        */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            (S32)regs->GR_L(r1) < (S32)n ? 1 :
            (S32)regs->GR_L(r1) > (S32)n ? 2 : 0;
}

#if defined(_MSVC_) && (_MSC_VER >= VS2019)
PUSH_MSVC_WARNINGS()
DISABLE_MSVC_WARNING( 4789 )
#endif

/*-------------------------------------------------------------------*/
/* B21A CFC   - Compare and Form Codeword                        [S] */
/*              (C) Copyright Peter Kuschnerus, 1999-2009            */
/*              (C) Copyright "Fish" (David B. Trout), 2005-2009     */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_form_codeword)
{
int     b2;                             /* Base of effective addr    */
int     rc;                             /* memcmp() return code      */
int     i;                              /* (work var)                */
VADR    effective_addr2;                /* (op2 effective address)   */
VADR    op1_addr, op3_addr;             /* (op1 & op3 fetch addr)    */
GREG    work_reg;                       /* (register work area)      */
U16     index, max_index;               /* (operand index values)    */
BYTE    op1[CFC_MAX_OPSIZE];            /* (work field)              */
BYTE    op3[CFC_MAX_OPSIZE];            /* (work field)              */
BYTE    tmp[CFC_MAX_OPSIZE];            /* (work field)              */
BYTE    descending;                     /* (sort-order control bit)  */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
BYTE    a64 = regs->psw.amode64;        /* ("64-bit mode" flag)      */
#endif
BYTE    op_size      = CFC_OPSIZE;      /* (work constant; uses a64) */
BYTE    gr2_shift    = CFC_GR2_SHIFT;   /* (work constant; uses a64) */
GREG    gr2_high_bit = CFC_HIGH_BIT;    /* (work constant; uses a64) */
#if defined( FEATURE_PER1 )
U16     rmask = 0x0000;                 /* (modified registers mask) */
#endif

    S(inst, regs, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXFC_INSTR_CHECK( regs );

    /* All operands must be halfword aligned */
    if (0
        || GR_A(1,regs) & 1
        || GR_A(2,regs) & 1
        || GR_A(3,regs) & 1
    )
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Initialize "end-of-operand-data" index value... */
    max_index = effective_addr2 & 0x7FFE;

    /* Loop until we either locate where the two operands
       differ from one another or until we reach the end of
       the operand data... */
    do
    {
        /* Exit w/cc0 (op1==op3) when end of operands are reached */

        index = GR_A(2,regs) & 0xFFFF;

        if ( index > max_index )
        {
            regs->psw.cc = 0;   // (operands are equal to each other)
            SET_GR_A( 2, regs, GR_A(3,regs) | gr2_high_bit );

#if defined( FEATURE_PER1 )
            /* Check for PER 1 GRA event */
            rmask |= PER_GRA_MASK( 2 );
            PER_GRA_CHECK( regs, rmask );
#endif
            return;
        }

        /* Fetch next chunk of operand data... */

        op1_addr = ( regs->GR(1) + index ) & ADDRESS_MAXWRAP(regs);
        op3_addr = ( regs->GR(3) + index ) & ADDRESS_MAXWRAP(regs);

        ARCH_DEP( vfetchc )( op1, op_size - 1, op1_addr, AR1, regs );
        ARCH_DEP( vfetchc )( op3, op_size - 1, op3_addr, AR1, regs );

        /* Update GR2 operand index value... (Note: we must do this AFTER
           we fetch the operand data in case of storage access exceptions) */

        SET_GR_A( 2, regs, GR_A(2,regs) + op_size );
#if defined( FEATURE_PER1 )
        rmask |= PER_GRA_MASK( 2 );
#endif
        /* Compare operands; continue while still equal... */
    }
    while ( !( rc = memcmp( op1, op3, op_size ) ) );

    /* Operands are NOT equal (we found an inequality). Set
       the condition code, form our codeword, and then exit */

    ASSERT( rc != 0 );     // (we shouldn't be here unless this is true!)

    /* Check to see which sequence the operands should be sorted into so
       we can know whether or not we need to form our codeword using either
       the inverse of the higher operand's data (if doing an ascending sort),
       or the lower operand's data AS-IS (if doing a descending sort). This
       thus causes either the lower (or higher) operand values (depending on
       which type of sort we're doing, ascending or descending) to bubble to
       the top (beginning) of the tree that the UPT (Update Tree) instruction
       ultimately/eventually updates (which gets built from our codewords).
    */

    descending = effective_addr2 & 1;  // (0==ascending, 1==descending)

    if ( rc < 0 )              // (operand-1 < operand-3)
    {
        if ( !descending )     // (ascending; in sequence)
        {
            regs->psw.cc = 1;  // (cc1 == in sequence)

            /* Ascending sort: use inverse of higher operand's data */

            for ( i=0; i < op_size; i++ )
                tmp[i] = ~op3[i];
        }
        else                   // (descending; out-of-sequence)
        {
            regs->psw.cc = 2;  // (cc2 == out-of-sequence)

            /* Descending sort: use lower operand's data as-is */

            memcpy( tmp, op1, op_size );

            /* Swap GR1 & GR3 because out-of-sequence */

            work_reg      =    GR_A(1,regs);
            SET_GR_A( 1, regs, GR_A(3,regs) );
            SET_GR_A( 3, regs, work_reg );
#if defined( FEATURE_PER1 )
            rmask |= PER_GRA_MASK2( 1, 3 );
#endif
        }
    }
    else                       // (operand-1 > operand-3)
    {
        if ( !descending )     // (ascending; out-of-sequence)
        {
            regs->psw.cc = 2;  // (cc2 == out-of-sequence)

            /* Ascending sort: use inverse of higher operand's data */

            for ( i=0; i < op_size; i++ )
                tmp[i] = ~op1[i];

            /* Swap GR1 & GR3 because out-of-sequence */

            work_reg      =    GR_A(1,regs);
            SET_GR_A( 1, regs, GR_A(3,regs) );
            SET_GR_A( 3, regs, work_reg );
#if defined( FEATURE_PER1 )
            rmask |= PER_GRA_MASK2( 1, 3 );
#endif
        }
        else                   // (descending; in sequence)
        {
            regs->psw.cc = 1;  // (cc1 == in sequence)

            /* Descending sort: use lower operand's data as-is */

            memcpy( tmp, op3, op_size );
        }
    }

    /* Form a sort/merge codeword to be used to sort the two operands
       into their proper sequence consisting of a combination of both
       the index position where the inequality was found (current GR2
       value) and either the one's complement inverse of the higher
       operand's data (if doing an ascending sort) or the lower oper-
       and's data as-is (if doing a descending sort)...
    */

    for ( work_reg=0, i=0; i < op_size; i++ )
        work_reg = ( work_reg << 8 ) | tmp[i];

    SET_GR_A( 2, regs, ( GR_A(2,regs) << gr2_shift ) | work_reg );

#if defined( FEATURE_PER1 )
    /* Check for PER 1 GRA event */
    rmask |= PER_GRA_MASK( 2 );
    PER_GRA_CHECK( regs, rmask );
#endif
}

#if defined(_MSVC_) && (_MSC_VER >= VS2019) && (_MSC_VER <= VS2022_9)
POP_MSVC_WARNINGS()
#endif

/*-------------------------------------------------------------------*/
/* BA   CS    - Compare and Swap                              [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_swap)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
BYTE   *main2;                          /* mainstor address          */
U32     old;                            /* old value                 */
U32     new;                            /* new value                 */

    RS(inst, regs, r1, r3, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXFC_INSTR_CHECK( regs );
    FW_CHECK(effective_addr2, regs);

    ITIMER_SYNC(effective_addr2,4-1,regs);

    /* Perform serialization before and after operation */
    PERFORM_SERIALIZATION( regs );
    {
        /* Get mainstor address */
        main2 = MADDRL (effective_addr2, 4, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

        old = CSWAP32(regs->GR_L(r1));
        new = CSWAP32(regs->GR_L(r3));

        /* MAINLOCK may be required if cmpxchg assists unavailable */
        OBTAIN_MAINLOCK( regs );
        {
            /* Attempt to exchange the values */
            regs->psw.cc = cmpxchg4( &old, new, main2 );
        }
        RELEASE_MAINLOCK( regs );
    }
    PERFORM_SERIALIZATION( regs );

    if (regs->psw.cc == 1)
    {
        PTT_CSF("*CS",regs->GR_L(r1),regs->GR_L(r3),(U32)(effective_addr2 & 0xffffffff));
        regs->GR_L(r1) = CSWAP32(old);
#if defined( _FEATURE_SIE )
        if(SIE_STATE_BIT_ON(regs, IC0, CS1))
        {
            if( !OPEN_IC_PER(regs) )
                longjmp(regs->progjmp, SIE_INTERCEPT_INST);
            else
                longjmp(regs->progjmp, SIE_INTERCEPT_INSTCOMP);
        }
        else
#endif /* defined( _FEATURE_SIE ) */
        {
            /* Check for PER 1 GRA event */
            PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

            if (sysblk.cpus > 1)
                sched_yield();
        }
    }
    else
    {
        ITIMER_UPDATE(effective_addr2,4-1,regs);
    }
}

/*-------------------------------------------------------------------*/
/* BB   CDS   - Compare Double and Swap                       [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_double_and_swap)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
BYTE   *main2;                          /* mainstor address          */
U64     old, new;                       /* old, new values           */

    RS(inst, regs, r1, r3, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXFC_INSTR_CHECK( regs );
    ODD2_CHECK(r1, r3, regs);
    DW_CHECK(effective_addr2, regs);

    ITIMER_SYNC(effective_addr2,8-1,regs);

    /* Perform serialization before and after operation */
    PERFORM_SERIALIZATION( regs );
    {
        /* Get operand absolute address */
        main2 = MADDRL (effective_addr2, 8, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

        /* Get old, new values */
        old = CSWAP64(((U64)(regs->GR_L(r1)) << 32) | regs->GR_L(r1+1));
        new = CSWAP64(((U64)(regs->GR_L(r3)) << 32) | regs->GR_L(r3+1));

        /* MAINLOCK may be required if cmpxchg assists unavailable */
        OBTAIN_MAINLOCK( regs );
        {
            /* Attempt to exchange the values */
            regs->psw.cc = cmpxchg8( &old, new, main2 );
        }
        RELEASE_MAINLOCK( regs );
    }
    PERFORM_SERIALIZATION( regs );

    if (regs->psw.cc == 1)
    {
        PTT_CSF("*CDS",regs->GR_L(r1),regs->GR_L(r3),(U32)(effective_addr2 & 0xffffffff));
        regs->GR_L(r1) = CSWAP64(old) >> 32;
        regs->GR_L(r1+1) = CSWAP64(old) & 0xffffffff;
#if defined( _FEATURE_SIE )
        if(SIE_STATE_BIT_ON(regs, IC0, CS1))
        {
            if( !OPEN_IC_PER(regs) )
                longjmp(regs->progjmp, SIE_INTERCEPT_INST);
            else
                longjmp(regs->progjmp, SIE_INTERCEPT_INSTCOMP);
        }
        else
#endif /* defined( _FEATURE_SIE ) */
        {
            /* Check for PER 1 GRA event */
            PER_GRA_CHECK( regs, PER_GRA_MASK2( r1, r1+1 ));

            if (sysblk.cpus > 1)
                sched_yield();
        }
    }
    else
    {
        ITIMER_UPDATE(effective_addr2,8-1,regs);
    }
}


#if defined( FEATURE_032_CSS_FACILITY )
/*-------------------------------------------------------------------*/
/* C8x2 CSST  - Compare and Swap and Store                     [SSF] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_and_swap_and_store)
{
int     r3;                             /* Value of R3 field         */
int     b1, b2;                         /* Base registers            */
const int rp=1;                         /* Parameter list register   */
VADR    effective_addr1;                /* Effective address         */
VADR    effective_addr2;                /* Effective address         */
VADR    addrp;                          /* Parameter list address    */
BYTE   *main1;                          /* Mainstor address of op1   */
int     ln2;                            /* Second operand length - 1 */
#if defined( FEATURE_033_CSS_FACILITY_2 )
ALIGN_16 U64 old[2] = { 0, 0 };         /* old values for cmpxchg16  */
U64     new16l=0, new16h=0,             /* swap values for cmpxchg16 */
        stv16h=0,stv16l=0;              /* 16-byte store value pair  */
#endif
U64     old8=0, new8=0;                 /* Swap values for cmpxchg8  */
U32     old4=0, new4=0;                 /* Swap values for cmpxchg4  */
U64     stv8=0;                         /* 8-byte store value        */
U32     stv4=0;                         /* 4-byte store value        */
U16     stv2=0;                         /* 2-byte store value        */
BYTE    stv1=0;                         /* 1-byte store value        */
BYTE    fc;                             /* Function code             */
BYTE    sc;                             /* Store characteristic      */

    SSF(inst, regs, b1, effective_addr1, b2, effective_addr2, r3);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    TXF_INSTR_CHECK( regs );

    /* Extract function code from register 0 bits 56-63 */
    fc = regs->GR_LHLCL(0);

    /* Extract store characteristic from register 0 bits 48-55 */
    sc = regs->GR_LHLCH(0);

#undef  MAX_CSST_FC
#undef  MAX_CSST_SC

#if defined( FEATURE_033_CSS_FACILITY_2 )
#define MAX_CSST_FC     2
#define MAX_CSST_SC     4
#else
#define MAX_CSST_FC     1
#define MAX_CSST_SC     3
#endif

    /* Program check if function code is greater than 1 or 2 */
    if (fc > MAX_CSST_FC)
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Program check if store characteristic is greater than 3 or 4 */
    if (sc > MAX_CSST_SC)
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Calculate length minus 1 of second operand */
    ln2 = (1 << sc) - 1;

    /* Program check if first operand is not on correct boundary */
    switch(fc)
    {
        case 0:
            FW_CHECK(effective_addr1, regs);
            break;
        case 1:
            DW_CHECK(effective_addr1, regs);
            break;
#if defined( FEATURE_033_CSS_FACILITY_2 )
        case 2:
            QW_CHECK(effective_addr1, regs);
            break;
#endif
    }

#if defined( FEATURE_033_CSS_FACILITY_2 )
    if((r3 & 1) && (fc == 2))
    {
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);
    }
#endif

    /* Program check if second operand is not on correct boundary */
    switch(sc)
    {
        case 1:
            HW_CHECK(effective_addr2, regs);
            break;
        case 2:
            FW_CHECK(effective_addr2, regs);
            break;
        case 3:
            DW_CHECK(effective_addr2, regs);
            break;
#if defined( FEATURE_033_CSS_FACILITY_2 )
        case 4:
            QW_CHECK(effective_addr2, regs);
            break;
#endif
    }

    /* Perform serialization before and after operation */
    PERFORM_SERIALIZATION( regs );
    {
        /* Obtain parameter list address from register 1 bits 0-59 */
        addrp = regs->GR(rp) & 0xFFFFFFFFFFFFFFF0ULL & ADDRESS_MAXWRAP(regs);

        /* Obtain main storage address of first operand */
        main1 = MADDRL (effective_addr1, 4, b1, regs, ACCTYPE_WRITE, regs->psw.pkey);

        /* Ensure second operand storage is writable */
        ARCH_DEP(validate_operand) (effective_addr2, b2, ln2, ACCTYPE_WRITE_SKP, regs);

        /* MAINLOCK may be required if cmpxchg assists unavailable */
        OBTAIN_MAINLOCK( regs );
        {
            /* Load the compare value from the r3 register and also */
            /* load replacement value from bytes 0-3, 0-7 or 0-15 of parameter list */
            switch(fc)
            {
                case 0:
                    old4 = CSWAP32(regs->GR_L(r3));
                    new4 = ARCH_DEP(vfetch4) (addrp, rp, regs);
                    new4 = CSWAP32(new4);
                    break;
                case 1:
                    old8 = CSWAP64(regs->GR_G(r3));
                    new8 = ARCH_DEP(vfetch8) (addrp, rp, regs);
                    new8 = CSWAP64(new8);
                    break;
#if defined( FEATURE_033_CSS_FACILITY_2 )
                case 2:
                    old[0] = CSWAP64( regs->GR_G( r3+0 ));
                    old[1] = CSWAP64( regs->GR_G( r3+1 ));

                    new16h = ARCH_DEP( vfetch8 )( addrp+0, rp, regs );
                    new16l = ARCH_DEP( vfetch8 )( addrp+8, rp, regs );

                    new16h = CSWAP64( new16h );
                    new16l = CSWAP64( new16l );
                    break;
#endif
            }

            /* Load the store value from bytes 16-23 of parameter list */
            addrp += 16;
            addrp = addrp & ADDRESS_MAXWRAP(regs);

            switch(sc)
            {
                case 0:
                    stv1 = ARCH_DEP(vfetchb) (addrp, rp, regs);
                    break;
                case 1:
                    stv2 = ARCH_DEP(vfetch2) (addrp, rp, regs);
                    break;
                case 2:
                    stv4 = ARCH_DEP(vfetch4) (addrp, rp, regs);
                    break;
                case 3:
                    stv8 = ARCH_DEP(vfetch8) (addrp, rp, regs);
                    break;
#if defined( FEATURE_033_CSS_FACILITY_2 )
                case 4:
                    stv16h = ARCH_DEP(vfetch8) (addrp, rp, regs);
                    stv16l = ARCH_DEP(vfetch8) (addrp+8, rp, regs);
                    break;
#endif
            }

            switch(fc)
            {
                case 0:
                    regs->psw.cc = cmpxchg4 (&old4, new4, main1);
                    break;
                case 1:
                    regs->psw.cc = cmpxchg8 (&old8, new8, main1);
                    break;
#if defined( FEATURE_033_CSS_FACILITY_2 )
                case 2:
                    regs->psw.cc = cmpxchg16( &old[0], &old[1], new16h, new16l, main1 );
                    break;
#endif
            }
            if (regs->psw.cc == 0)
            {
                /* Store the store value into the second operand location */
                switch(sc)
                {
                    case 0:
                        ARCH_DEP(vstoreb) (stv1, effective_addr2, b2, regs);
                        break;
                    case 1:
                        ARCH_DEP(vstore2) (stv2, effective_addr2, b2, regs);
                        break;
                    case 2:
                        ARCH_DEP(vstore4) (stv4, effective_addr2, b2, regs);
                        break;
                    case 3:
                        ARCH_DEP(vstore8) (stv8, effective_addr2, b2, regs);
                        break;
#if defined( FEATURE_033_CSS_FACILITY_2 )
                    case 4:
                        ARCH_DEP(vstore8) (stv16h, effective_addr2, b2, regs);
                        ARCH_DEP(vstore8) (stv16l, effective_addr2+8, b2, regs);
                        break;
#endif
                }
            }
            else /* (regs->psw.cc != 0) */
            {
                /* Update register values if cmpxchg failed */
                switch(fc)
                {
                    case 0:
                        regs->GR_L(r3) = CSWAP32(old4);
                        break;
                    case 1:
                        regs->GR_G(r3) = CSWAP64(old8);
                        break;
#if defined( FEATURE_033_CSS_FACILITY_2 )
                    case 2:
                        regs->GR_G( r3+0 ) = CSWAP64( old[0] );
                        regs->GR_G( r3+1 ) = CSWAP64( old[1] );
                        break;
#endif
                }
            }
        }
        RELEASE_MAINLOCK( regs );
    }
    PERFORM_SERIALIZATION( regs );

} /* end DEF_INST(compare_and_swap_and_store) */
#endif /* defined( FEATURE_032_CSS_FACILITY ) */


/*-------------------------------------------------------------------*/
/* 49   CH    - Compare Halfword                              [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_halfword)
{
int     r1;                             /* Values of R fields        */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
S32     n;                              /* 32-bit operand values     */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Load rightmost 2 bytes of comparand from operand address */
    n = (S16)ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            (S32)regs->GR_L(r1) < n ? 1 :
            (S32)regs->GR_L(r1) > n ? 2 : 0;
}


#if defined( FEATURE_IMMEDIATE_AND_RELATIVE )
/*-------------------------------------------------------------------*/
/* A7xE CHI   - Compare Halfword Immediate                    [RI-a] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_halfword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand            */

    RI(inst, regs, r1, opcd, i2);

    /* Compare signed operands and set condition code */
    regs->psw.cc =
            (S32)regs->GR_L(r1) < (S16)i2 ? 1 :
            (S32)regs->GR_L(r1) > (S16)i2 ? 2 : 0;

}
#endif /* defined( FEATURE_IMMEDIATE_AND_RELATIVE ) */


#ifdef OPTION_OPTINST
/*-------------------------------------------------------------------*/
/* 15   CLR   - Compare Logical Register (optimized)            [RR] */
/*-------------------------------------------------------------------*/

/* Optimized case (r1 equal r2) is optimized by compiler */

#define CLRgen( r1, r2 )                                              \
                                                                      \
  DEF_INST( 15 ## r1 ## r2 )                                          \
  {                                                                   \
    UNREFERENCED( inst );                                             \
                                                                      \
    INST_UPDATE_PSW( regs, 2, 2 );                                    \
                                                                      \
    regs->psw.cc = regs->GR_L( 0x ## r1 ) <                           \
                   regs->GR_L( 0x ## r2 ) ? 1 :                       \
                                                                      \
                   regs->GR_L( 0x ## r1 ) >                           \
                   regs->GR_L( 0x ## r2 ) ? 2 : 0;                    \
  }

#define CLRgenr2(r1)                                                  \
                                                                      \
  CLRgen( r1, 0 )                                                     \
  CLRgen( r1, 1 )                                                     \
  CLRgen( r1, 2 )                                                     \
  CLRgen( r1, 3 )                                                     \
  CLRgen( r1, 4 )                                                     \
  CLRgen( r1, 5 )                                                     \
  CLRgen( r1, 6 )                                                     \
  CLRgen( r1, 7 )                                                     \
  CLRgen( r1, 8 )                                                     \
  CLRgen( r1, 9 )                                                     \
  CLRgen( r1, A )                                                     \
  CLRgen( r1, B )                                                     \
  CLRgen( r1, C )                                                     \
  CLRgen( r1, D )                                                     \
  CLRgen( r1, E )                                                     \
  CLRgen( r1, F )

CLRgenr2( 0 )
CLRgenr2( 1 )
CLRgenr2( 2 )
CLRgenr2( 3 )
CLRgenr2( 4 )
CLRgenr2( 5 )
CLRgenr2( 6 )
CLRgenr2( 7 )
CLRgenr2( 8 )
CLRgenr2( 9 )
CLRgenr2( A )
CLRgenr2( B )
CLRgenr2( C )
CLRgenr2( D )
CLRgenr2( E )
CLRgenr2( F )

#endif /* #ifdef OPTION_OPTINST */


/*-------------------------------------------------------------------*/
/* 15   CLR   - Compare Logical Register                        [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* Compare unsigned operands and set condition code */
    regs->psw.cc = regs->GR_L(r1) < regs->GR_L(r2) ? 1 :
                   regs->GR_L(r1) > regs->GR_L(r2) ? 2 : 0;
}


/*-------------------------------------------------------------------*/
/* 95   CLI   - Compare Logical Immediate                       [SI] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_immediate)
{
BYTE    i2;                             /* Immediate byte            */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
BYTE    cbyte;                          /* Compare byte              */

    SI(inst, regs, i2, b1, effective_addr1);
    PER_ZEROADDR_XCHECK( regs, b1 );

    /* Fetch byte from operand address */
    cbyte = ARCH_DEP(vfetchb) ( effective_addr1, b1, regs );

    /* Compare with immediate operand and set condition code */
    regs->psw.cc = (cbyte < i2) ? 1 :
                   (cbyte > i2) ? 2 : 0;
}


/*-------------------------------------------------------------------*/
/* BD   CLM   - Compare Logical Characters under Mask         [RS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_characters_under_mask)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i, j;                           /* Integer work areas        */
int     cc = 0;                         /* Condition code            */
BYTE    rbyte[4],                       /* Register bytes            */
        vbyte;                          /* Virtual storage byte      */

    RS(inst, regs, r1, r3, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

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
}


#ifndef MEMNEQ
#define MEMNEQ
/*-------------------------------------------------------------------*/
/* memneq: return index of first unequal byte                        */
/*-------------------------------------------------------------------*/
static INLINE U32 memneq( const BYTE* m1, const BYTE* m2, U32 len )
{
    U32  i;
    for (i=0; i < len; i++)
        if (m1[i] != m2[i])
            break;
    return i;
}
#endif // MEMNEQ


/*-------------------------------------------------------------------*/
/*           mem_pad_cmp  --  compare memory to padding              */
/*-------------------------------------------------------------------*/
/* Input:                                                            */
/*      regs    CPU register context                                 */
/*      ea1     Logical address of leftmost byte of operand-1        */
/*      b1      Operand-1 base register                              */
/*      pad     Pointer to padding bytes (i.e. operand-2)            */
/*      len     Compare length (0 < len < pagesize)                  */
/*      idx     Index to where inequality was found      (CLCL only) */
/* Output:                                                           */
/*      rc      Same as for CRT memcmp() function: -1, 0, +1         */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or fetch protection       */
/*      exception, and in this case the function does not return.    */
/*-------------------------------------------------------------------*/
int ARCH_DEP( mem_pad_cmp )
(
    REGS*  regs,                // register context
    VADR   ea1, int b1,         // op-1 (dst)
    const BYTE* pad,            // pointer to padding bytes (i.e. op-2)
    U32    len,                 // compare length
    U32*   idx                  // unequal idx ptr
)
{
    const BYTE    *m1,   *m2;   // operand mainstor addresses
    const BYTE  *neq1, *neq2;   // operand mainstor addresses
    U32          len1;          // operand-1 length
    U32    neqlen;              // length of unequal part
    U32    neqidx = 0;          // index where inequality found
    int    rc;                  // return code (same as CRT memcmp)

    ITIMER_SYNC( ea1, len-1, regs );

    // Translate leftmost byte of each operand
    m1 = neq1 = MADDRL(ea1, len, b1, regs, ACCTYPE_READ, regs->psw.pkey );
    m2 = neq2 = pad;

    // Quick out if comparing just 1 byte
    if (unlikely( (neqlen = len) == 1 ))
        rc = (*m1 == *m2 ? 0 : (*m1 < *m2 ? -1 : +1));
    else if ((ea1 & PAGEFRAME_BYTEMASK) <= PAGEFRAME_BYTEMASK - (len-1))
    {
        // (1) - Neither operand crosses a page boundary

        switch (neqlen = len)
        {
            case 2: // halfword
            {
                U16 v1, v2;
                v1 = fetch_hw( m1 );
                v2 = fetch_hw( m2 );
                rc = (v1 == v2 ? 0 : (v1 < v2 ? -1 : +1));
            }
            break;

            case 4: // fullword
            {
                U32 v1, v2;
                v1 = fetch_fw( m1 );
                v2 = fetch_fw( m2 );
                rc = (v1 == v2 ? 0 : (v1 < v2 ? -1 : +1));
            }
            break;

#if defined( SIZEOF_INT_P ) && SIZEOF_INT_P >= 8
            case 8: // doubleword
            {
                U64 v1, v2;
                v1 = fetch_dw( m1 );
                v2 = fetch_dw( m2 );
                rc = (v1 == v2 ? 0 : (v1 < v2 ? -1 : +1));
            }
            break;
#endif
            default:  // some other length
            {
                rc = memcmp( m1, m2, neqlen = len );
            }
            break;
        }
    }
    else // (3) - Only operand-1 crosses a page boundary
    {
        len1 = PAGEFRAME_PAGESIZE - (ea1 & PAGEFRAME_BYTEMASK);
        rc = memcmp( m1, m2, neqlen = len1 );
        if (rc == 0)
        {
            neqidx += neqlen;
            m1 = neq1 = MADDRL((ea1 + len1) & ADDRESS_MAXWRAP( regs ),
                    len - len1 ,b1, regs, ACCTYPE_READ, regs->psw.pkey );
            rc = memcmp( m1, neq2 = m2, neqlen = len - len1 );
        }
    }

    // Is caller interested in where the inequality was found?
    if (idx)
    {
        if (rc ==0)
            *idx = len;
        else
            VERIFY((*idx = neqidx += memneq( neq1, neq2, neqlen )) < len );
    }

    return rc;
} /* end ARCHDEP( mem_pad_cmp ) */


/*-------------------------------------------------------------------*/
/*                 mem_cmp  --  compare memory                       */
/*-------------------------------------------------------------------*/
/* Input:                                                            */
/*      regs    CPU register context                                 */
/*      ea1     Logical address of leftmost byte of operand-1        */
/*      b1      Operand-1 base register                              */
/*      ea2     Logical address of leftmost byte of operand-2        */
/*      b2      Operand-2 base register                              */
/*      len     Compare length (0 < len < pagesize)                  */
/*      idx     Index to where inequality was found      (CLCL only) */
/* Output:                                                           */
/*      rc      Same as for CRT memcmp() function: -1, 0, +1         */
/*                                                                   */
/*      A program check may be generated if the logical address      */
/*      causes an addressing, translation, or fetch protection       */
/*      exception, and in this case the function does not return.    */
/*-------------------------------------------------------------------*/
int ARCH_DEP( mem_cmp )
(
    REGS*  regs,                // register context
    VADR   ea1, int b1,         // op-1 (dst)
    VADR   ea2, int b2,         // op-2 (src)
    U32    len,                 // compare length
    U32*   idx                  // unequal idx ptr
)
{
    const BYTE    *m1,   *m2;   // operand mainstor addresses
    const BYTE  *neq1, *neq2;   // operand mainstor addresses
    U32          len1,  len2;   // operand lengths
    U32    neqlen;              // length of unequal part
    U32    neqidx = 0;          // index where inequality found
    int    rc;                  // return code (same as CRT memcmp)

    ITIMER_SYNC( ea1, len-1, regs );

    // Translate leftmost byte of each operand
    m1 = neq1 = MADDRL(ea1, len, b1, regs, ACCTYPE_READ, regs->psw.pkey );
    m2 = neq2 = MADDRL(ea2, len, b2, regs, ACCTYPE_READ, regs->psw.pkey );

    //----------------------------------------------------------------
    //
    // There are several scenarios: (in optimal order)
    //
    //  (1) NEITHER operand crosses a page boundary
    //
    //      (a) halfword compare
    //      (b) fullword compare
    //      (c) doubleword compare (64-bit machine)
    //      (d) some other length
    //
    //  (2) Op1 does NOT cross a page boundary, but Op2 does
    //  (3) Op1 crosses a page boundary, but Op2 does NOT
    //  (4) BOTH operands cross a page boundary
    //
    //      (a) Both operands cross at the same place
    //      (b) Operand-1 crosses the page boundary first
    //      (c) Operand-2 crosses the page boundary first
    //
    //----------------------------------------------------------------

    // Quick out if comparing just 1 byte
    if (unlikely( (neqlen = len) == 1 ))
        rc = (*m1 == *m2 ? 0 : (*m1 < *m2 ? -1 : +1));
    else if ((ea1 & PAGEFRAME_BYTEMASK) <= PAGEFRAME_BYTEMASK - (len-1))
    {
        if  ((ea2 & PAGEFRAME_BYTEMASK) <= PAGEFRAME_BYTEMASK - (len-1))
        {
            // (1) - Neither operand crosses a page boundary

            switch (neqlen = len)
            {
                case 2: // halfword
                {
                    U16 v1, v2;
                    v1 = fetch_hw( m1 );
                    v2 = fetch_hw( m2 );
                    rc = (v1 == v2 ? 0 : (v1 < v2 ? -1 : +1));
                }
                break;

                case 4: // fullword
                {
                    U32 v1, v2;
                    v1 = fetch_fw( m1 );
                    v2 = fetch_fw( m2 );
                    rc = (v1 == v2 ? 0 : (v1 < v2 ? -1 : +1));
                }
                break;

#if defined( SIZEOF_INT_P ) && SIZEOF_INT_P >= 8
                case 8: // doubleword
                {
                    U64 v1, v2;
                    v1 = fetch_dw( m1 );
                    v2 = fetch_dw( m2 );
                    rc = (v1 == v2 ? 0 : (v1 < v2 ? -1 : +1));
                }
                break;
#endif
                default:  // some other length
                {
                    rc = memcmp( m1, m2, neqlen = len );
                }
                break;
            }
        }
        else // (2) - Operand-2 crosses a page boundary
        {
            len2 = PAGEFRAME_PAGESIZE - (ea2 & PAGEFRAME_BYTEMASK);
            rc = memcmp( m1, m2, neqlen = len2 );
            if (rc == 0)
            {
                neqidx += neqlen;
                m2 = neq2 = MADDRL((ea2 + len2) & ADDRESS_MAXWRAP( regs ),
                    len + 1 - len2, b2, regs, ACCTYPE_READ, regs->psw.pkey );
                rc = memcmp( neq1 = m1 + len2, m2, neqlen = len - len2 );
             }
        }
    }
    else // (Operand-1 crosses a page boundary...)
    {
        len1 = PAGEFRAME_PAGESIZE - (ea1 & PAGEFRAME_BYTEMASK);
        if ((ea2 & PAGEFRAME_BYTEMASK) <= PAGEFRAME_BYTEMASK - (len-1) )
        {
            // (3) - Only operand-1 crosses a page boundary

            rc = memcmp( m1, m2, neqlen = len1 );
            if (rc == 0)
            {
                neqidx += neqlen;
                m1 = neq1 = MADDRL((ea1 + len1) & ADDRESS_MAXWRAP( regs ),
                        len - len1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
                rc = memcmp( m1, neq2 = m2 + len1, neqlen = len - len1 );
             }
        }
        else // (4) - Both operands cross a page boundary
        {
            len2 = PAGEFRAME_PAGESIZE - (ea2 & PAGEFRAME_BYTEMASK);
            if (len1 == len2)
            {
                // (4a) - Both operands cross at the same place

                rc = memcmp( m1, m2, neqlen = len1 );
                if (rc == 0)
                {
                    neqidx += neqlen;
                    m1 = neq1 = MADDRL((ea1 + len1) & ADDRESS_MAXWRAP( regs ),
                            len - len1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
                    m2 = neq2 = MADDRL((ea2 + len1) & ADDRESS_MAXWRAP( regs ),
                            len - len1, b2, regs, ACCTYPE_READ, regs->psw.pkey );
                    rc = memcmp( m1, m2, neqlen = len - len1 );
                }
            }
            else if (len1 < len2)
            {
                // (4b) - Operand-1 crosses first

                rc = memcmp( m1, m2, neqlen = len1 );
                if (rc == 0)
                {
                    neqidx += neqlen;
                    m1 = neq1 = MADDRL((ea1 + len1) & ADDRESS_MAXWRAP( regs ),
                            len - len1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
                    rc = memcmp( m1, neq2 = m2 + len1, neqlen = len2 - len1 );
                    if (rc == 0)
                    {
                        neqidx += neqlen;
                        m2 = neq2 = MADDRL((ea2 + len2) & ADDRESS_MAXWRAP( regs ),
                                len - len2, b2, regs, ACCTYPE_READ, regs->psw.pkey );
                        rc = memcmp( neq1 = m1 + len2 - len1, m2, neqlen = len - len2 );
                    }
                }
            }
            else
            {
                // (4c) - Operand-2 crosses first

                rc = memcmp( m1, m2, neqlen = len2 );
                if (rc == 0)
                {
                    neqidx += neqlen;
                    m2 = neq2 = MADDRL((ea2 + len2) & ADDRESS_MAXWRAP( regs ),
                            len - len2, b2, regs, ACCTYPE_READ, regs->psw.pkey );
                    rc = memcmp( neq1 = m1 + len2, m2, neqlen = len1 - len2 );
                    if (rc == 0)
                    {
                        neqidx += neqlen;
                        m1 = neq1 = MADDRL((ea1 + len1) & ADDRESS_MAXWRAP( regs ),
                                len - len1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
                        rc = memcmp( m1, neq2 = m2 + len1 - len2, neqlen = len - len1 );
                    }
                }
            }
        }
    }

    // Is caller interested in where the inequality was found?
    if (idx)
    {
        if (rc ==0)
            *idx = len;
        else
            VERIFY((*idx = neqidx += memneq( neq1, neq2, neqlen )) < len );
    }

    return rc;
} /* end ARCHDEP( mem_cmp ) */

#undef  USE_NEW_CLC
//#define USE_NEW_CLC
#if defined( USE_NEW_CLC )
/*-------------------------------------------------------------------*/
/* D5   CLC   - Compare Logical Character                     [SS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_character)
{
VADR     effective_addr1;               /* Effective address         */
VADR     effective_addr2;               /* Effective address         */
int      b1, b2;                        /* Base registers            */
U32      len;                           /* Length minus 1            */
int      rc;                            /* mem_cmp() return code     */

    SS_L( inst, regs, len, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    TXFC_INSTR_CHECK( regs );

    rc = ARCH_DEP( mem_cmp )( regs, effective_addr1, b1, effective_addr2, b2, len+1, NULL );
    regs->psw.cc = (rc == 0 ? 0 : (rc < 0 ? 1 : 2));
}
#else // !defined( USE_NEW_CLC )
/*-------------------------------------------------------------------*/
/* D5   CLC   - Compare Logical Character                     [SS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_character)
{
unsigned int len, len1, len2;           /* Lengths                   */
int      rc;                            /* memcmp() return code      */
int      b1, b2;                        /* Base registers            */
VADR     effective_addr1;               /* Effective address         */
VADR     effective_addr2;               /* Effective address         */
BYTE    *m1, *m2;                       /* Mainstor addresses        */

    SS_L( inst, regs, len, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    TXFC_INSTR_CHECK( regs );

    ITIMER_SYNC( effective_addr1, len, regs );
    ITIMER_SYNC( effective_addr2, len, regs );

    /* Translate addresses of leftmost operand bytes */
    m1 = MADDRL( effective_addr1, len + 1,b1, regs, ACCTYPE_READ, regs->psw.pkey );
    m2 = MADDRL( effective_addr2, len + 1,b2, regs, ACCTYPE_READ, regs->psw.pkey );

    /* Quick out if comparing just 1 byte */
    if (unlikely( !len ))
    {
        rc = *m1 - *m2;
        regs->psw.cc = (rc == 0 ? 0 : (rc < 0 ? 1 : 2 ));
        return;
    }

    /* There are several scenarios (in optimal order):
     * (1) dest boundary and source boundary not crossed
     *     (a) halfword compare
     *     (b) fullword compare
     *     (c) doubleword compare (64-bit machines)
     *     (d) other
     * (2) dest boundary not crossed and source boundary crossed
     * (3) dest boundary crossed and source boundary not crossed
     * (4) dest boundary and source boundary are crossed
     *     (a) dest and source boundary cross at the same time
     *     (b) dest boundary crossed first
     *     (c) source boundary crossed first
     */

    if ((effective_addr1 & PAGEFRAME_BYTEMASK) <= PAGEFRAME_BYTEMASK - len)
    {
        if ((effective_addr2 & PAGEFRAME_BYTEMASK) <= PAGEFRAME_BYTEMASK - len)
        {
            /* (1) - No boundaries are crossed */
            switch(len) {

            case 1:
                /* (1a) - halfword compare */
                {
                    U16 v1, v2;
                    v1 = fetch_hw( m1 );
                    v2 = fetch_hw( m2 );
                    regs->psw.cc = (v1 == v2 ? 0 : (v1 < v2 ? 1 : 2));
                    return;
                }
                break;

            case 3:
                /* (1b) - fullword compare */
                {
                    U32 v1, v2;
                    v1 = fetch_fw( m1 );
                    v2 = fetch_fw( m2 );
                    regs->psw.cc = (v1 == v2 ? 0 : (v1 < v2 ? 1 : 2));
                    return;
                }
                break;

            case 7:
                /* (1c) - doubleword compare (64-bit machines) */
                if (sizeof( unsigned int ) >= 8)
                {
                    U64 v1, v2;
                    v1 = fetch_dw( m1 );
                    v2 = fetch_dw( m2 );
                    regs->psw.cc = (v1 == v2 ? 0 : (v1 < v2 ? 1 : 2));
                    return;
                }
                /* FALLTHRU */

            default:
                /* (1d) - other compare */
                rc = memcmp( m1, m2, len + 1 );
                break;
            }
        }
        else
        {
            /* (2) - Second operand crosses a boundary */
            len2 = PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK);
            rc = memcmp( m1, m2, len2 );
            if (rc == 0)
            {
                m2 = MADDRL((effective_addr2 + len2) & ADDRESS_MAXWRAP( regs ),
                    len + 1 - len2, b2, regs, ACCTYPE_READ, regs->psw.pkey );
                rc = memcmp( m1 + len2, m2, len - len2 + 1 );
             }
        }
    }
    else
    {
        /* First operand crosses a boundary */
        len1 = PAGEFRAME_PAGESIZE - (effective_addr1 & PAGEFRAME_BYTEMASK);
        if ((effective_addr2 & PAGEFRAME_BYTEMASK) <= PAGEFRAME_BYTEMASK - len )
        {
            /* (3) - First operand crosses a boundary */
            rc = memcmp( m1, m2, len1 );
            if (rc == 0)
            {
                m1 = MADDRL((effective_addr1 + len1) & ADDRESS_MAXWRAP( regs ),
                   len + 1 - len1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
                rc = memcmp( m1, m2 + len1, len - len1 + 1 );
             }
        }
        else
        {
            /* (4) - Both operands cross a boundary */
            len2 = PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK);
            if (len1 == len2)
            {
                /* (4a) - Both operands cross at the same time */
                rc = memcmp( m1, m2, len1 );
                if (rc == 0)
                {
                    m1 = MADDRL((effective_addr1 + len1) & ADDRESS_MAXWRAP( regs ),
                      len + 1 - len1,b1, regs, ACCTYPE_READ, regs->psw.pkey );
                    m2 = MADDRL((effective_addr2 + len1) & ADDRESS_MAXWRAP( regs ),
                      len + 1 - len1,b2, regs, ACCTYPE_READ, regs->psw.pkey );
                    rc = memcmp( m1, m2, len - len1 +1 );
                }
            }
            else if (len1 < len2)
            {
                /* (4b) - First operand crosses first */
                rc = memcmp( m1, m2, len1 );
                if (rc == 0)
                {
                    m1 = MADDRL((effective_addr1 + len1) & ADDRESS_MAXWRAP( regs ),
                       len + 1 - len1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
                    rc = memcmp( m1, m2 + len1, len2 - len1 );
                }
                if (rc == 0)
                {
                    m2 = MADDRL((effective_addr2 + len2) & ADDRESS_MAXWRAP( regs ),
                       len + 1 - len2, b2, regs, ACCTYPE_READ, regs->psw.pkey );
                    rc = memcmp( m1 + len2 - len1, m2, len - len2 + 1 );
                }
            }
            else
            {
                /* (4c) - Second operand crosses first */
                rc = memcmp( m1, m2, len2 );
                if (rc == 0)
                {
                    m2 = MADDRL((effective_addr2 + len2) & ADDRESS_MAXWRAP( regs ),
                     len + 1 - len2, b2, regs, ACCTYPE_READ, regs->psw.pkey );
                    rc = memcmp( m1 + len2, m2, len1 - len2 );
                }
                if (rc == 0)
                {
                    m1 = MADDRL((effective_addr1 + len1) & ADDRESS_MAXWRAP( regs ),
                     len + 1 - len1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
                    rc = memcmp( m1, m2 + len1 - len2, len - len1 + 1 );
                }
            }
        }
    }
    regs->psw.cc = (rc == 0 ? 0 : (rc < 0 ? 1 : 2));
}
#endif // !defined( USE_NEW_CLC )


/*-------------------------------------------------------------------*/
/* 0F   CLCL  - Compare Logical Long                            [RR] */
/*-------------------------------------------------------------------*/

#undef   CHUNK_AMT
#define  CHUNK_AMT          (PAGEFRAME_PAGESIZE - 256)

#undef   MAX_CPU_AMT
#define  MAX_CPU_AMT        (32 * 1024) // (purely arbitrary)

#ifndef CLCL_ONETIME
#define CLCL_ONETIME
CASSERT( CHUNK_AMT      <   (PAGEFRAME_PAGESIZE), general1_c );
CASSERT( MAX_CPU_AMT    >   (PAGEFRAME_PAGESIZE), general1_c );
#endif

DEF_INST(compare_logical_character_long)
{
    VADR  addr1, addr2;         // Operand addresses
    U32   len1,  len2;          // Operand lengths
    int   r1,    r2;            // Values of R fields
    U32   unpadded_len;         // work (lesser of the two lengths)
    U32   padded_len;           // work (greater length minus lesser)
    U32   in_amt;               // Work (amount to be compared)
    U32   out_amt;              // Work (amount that was found equal)
    U32   rem;                  // Work (amount remaining)
    U32   total = 0;            // TOTAL amount compared so far
    int   rc = 0;               // memcmp() return code

    RR( inst, regs, r1, r2 );

    ODD2_CHECK( r1, r2, regs );
    PER_ZEROADDR_L24CHECK2( regs, r1, r1+1, r2, r2+1 );
    TXFC_INSTR_CHECK( regs );

    // Determine the destination and source addresses
    addr1 = regs->GR( r1 ) & ADDRESS_MAXWRAP( regs );
    addr2 = regs->GR( r2 ) & ADDRESS_MAXWRAP( regs );

    // Load operand lengths from bits 8-31 of R1+1 and R2+1
    len1 = regs->GR_LA24( r1+1 );
    len2 = regs->GR_LA24( r2+1 );

    // Save lengths of padded/unpadded parts
    if (len1 == len2)
    {
        unpadded_len = len1;
        padded_len   = 0;
    }
    else if (len1 < len2)
    {
        unpadded_len = len1;
        padded_len   = len2 - len1;
    }
    else // (len1 > len2)
    {
        unpadded_len = len2;
        padded_len   = len1 - len2;
    }

    // Compare the unpadded part first

    rem = unpadded_len;

    while (rc == 0 && rem && total < MAX_CPU_AMT)
    {
        in_amt = rem < CHUNK_AMT
               ? rem : CHUNK_AMT;

        rc = ARCH_DEP( mem_cmp )( regs, addr1, r1, addr2, r2, in_amt,
                                                            &out_amt );
        addr1 += out_amt;
        addr2 += out_amt;
        total += out_amt;
        rem   -= out_amt;

        // Must update register values as we go in case
        // the next compare causes an access interrupt

        SET_GR_A( r1, regs, addr1 );

        if ( regs->GR_LA24( r1+1 ) >= out_amt)
             regs->GR_LA24( r1+1 ) -= out_amt;
        else regs->GR_LA24( r1+1 ) = 0;

        SET_GR_A( r2, regs, addr2 );

        if ( regs->GR_LA24( r2+1 ) >= out_amt)
             regs->GR_LA24( r2+1 ) -= out_amt;
        else regs->GR_LA24( r2+1 ) = 0;
    }

    // Now compare the padded part, if needed

    if (rc == 0 && padded_len && total < MAX_CPU_AMT)
    {
        BYTE  padding[ CHUNK_AMT ];
        VADR  addr     =  (len1 > len2) ? addr1 : addr2;
        int   r        =  (len1 > len2) ? r1    : r2;
        bool  swap_rc  =  (len1 > len2) ? false : true;
        BYTE  pad      =  regs->GR_LHHCH( r2+1 );

        memset( padding, pad, MIN( padded_len, sizeof( padding )));

        rem = padded_len;

        while (rc == 0 && rem && total < MAX_CPU_AMT)
        {
            in_amt = rem < CHUNK_AMT
                   ? rem : CHUNK_AMT;

            rc = ARCH_DEP( mem_pad_cmp )( regs, addr, r, padding, in_amt,
                                                                &out_amt );
            addr  += out_amt;
            total += out_amt;
            rem   -= out_amt;

            // Must update register values as we go in case
            // the next compare causes an access interrupt

            SET_GR_A( r, regs, addr );

            if ( regs->GR_LA24( r+1 ) >= out_amt)
                 regs->GR_LA24( r+1 ) -= out_amt;
            else regs->GR_LA24( r+1 ) = 0;
        }

        if (swap_rc)
            rc = -rc;
    }

    // Backup the PSW for re-execution if instruction was interrupted

    if (rc == 0 && total < (unpadded_len + padded_len))
    {
        ASSERT( total >= MAX_CPU_AMT );   // (sanity check)
        SET_PSW_IA_AND_MAYBE_IP( regs, PSW_IA_FROM_IP( regs, -REAL_ILC( regs )));
    }

    // Set the condition code and return

    regs->psw.cc = (!rc ? 0 : (rc < 0 ? 1 : 2));

    // Check for PER 1 GRA event
    PER_GRA_CHECK( regs, PER_GRA_MASK4( r1, r1+1, r2, r2+1 ));

} /* end DEF_INST( compare_logical_character_long ) */


#if defined( FEATURE_COMPARE_AND_MOVE_EXTENDED )

/*-------------------------------------------------------------------*/
/* A9   CLCLE - Compare Logical Long Extended                 [RS-a] */
/*-------------------------------------------------------------------*/

#undef   MAX_CPU_AMT
#define  MAX_CPU_AMT    (PAGEFRAME_PAGESIZE - 256)  // for (CC=3) and 0 <  MAX_CPU_AMT < pagesize)

DEF_INST(compare_logical_long_extended)
{
    int     r1, r3;                         /* Register numbers          */
    int     b2;                             /* effective address base    */
    VADR    effective_addr2;                /* effective address         */
    VADR    addr1, addr2;                   /* Operand addresses         */
    GREG    len1, len2;                     /* Operand lengths           */
    BYTE    pad;                            /* Padding byte              */

    U64   unpadded_len;         // work (lesser of the two lengths)
    U64   padded_len;           // work (greater length minus lesser)
    U32   in_amt;               // Work (amount to be compared)
    U32   out_amt;              // Work (amount that was found equal)
    U64   total = 0;            // TOTAL amount compared so far
    GREG  wlen1, wlen3;         // Work length (current reg length)
    int   rc = 0;               // memcmp() return code

    RS( inst, regs, r1, r3, b2, effective_addr2 );
    PER_ZEROADDR_LCHECK2( regs, r1, r1+1, r3, r3+1 );

    TXFC_INSTR_CHECK( regs );
    ODD2_CHECK( r1, r3, regs );

    /* Load padding byte from bits 24-31 of effective address */
    pad = effective_addr2 & 0xFF;

    /* Determine the destination and source addresses */
    addr1 = regs->GR( r1 ) & ADDRESS_MAXWRAP( regs );
    addr2 = regs->GR( r3 ) & ADDRESS_MAXWRAP( regs );

    /* Load operand lengths from bits 0-31 of R1+1 and R3+1 */
    len1 = GR_A( r1+1, regs );
    len2 = GR_A( r3+1, regs );

    // Nothing to compare, so equal
    if (len1 == 0 && len2 == 0)  {
        regs->psw.cc = 0;
        return;
    }

    // Save lengths of padded/unpadded parts
    if (len1 == len2)
    {
        unpadded_len = len1;
        padded_len   = 0;
    }
    else if (len1 < len2)
    {
        unpadded_len = len1;
        padded_len   = len2 - len1;
    }
    else // (len1 > len2)
    {
        unpadded_len = len2;
        padded_len   = len1 - len2;
    }

    // Compare the unpadded part first (up to Max_CPU_AMT)
    in_amt = unpadded_len <  MAX_CPU_AMT
            ? unpadded_len :  MAX_CPU_AMT;

    rc = ARCH_DEP( mem_cmp )( regs, addr1, r1, addr2, r3, in_amt,
                                                        &out_amt );
    addr1 += out_amt;
    addr1 &= ADDRESS_MAXWRAP( regs );
    addr2 += out_amt;
    addr2 &= ADDRESS_MAXWRAP( regs );
    total += out_amt;

    // Update register values

    SET_GR_A( r1, regs, addr1 );
    wlen1 = GR_A( r1+1, regs );
    if ( wlen1 >= out_amt)
            SET_GR_A( r1+1, regs, wlen1 - out_amt);
    else SET_GR_A( r1+1, regs, 0);

    SET_GR_A( r3, regs, addr2 );
    wlen3 = GR_A( r3+1, regs );
    if ( wlen3 >= out_amt)
            SET_GR_A( r3+1, regs, wlen3 - out_amt);
    else SET_GR_A( r3+1, regs, 0);

    // Now compare the padded part, if needed

    if (rc == 0 && padded_len && total < MAX_CPU_AMT)
    {
        BYTE  padding[ MAX_CPU_AMT ];
        VADR  addr     =  (len1 > len2) ? addr1 : addr2;
        int   r        =  (len1 > len2) ? r1    : r3;
        bool  swap_rc  =  (len1 > len2) ? false : true;
        GREG  wlen;

        memset( padding, pad, MIN( padded_len, sizeof( padding )));

        // Compare the padded part (up to Max_CPU_AMT - total (padded part))
        in_amt = padded_len < MAX_CPU_AMT - total
                ? padded_len : MAX_CPU_AMT - total;

        rc = ARCH_DEP( mem_pad_cmp )( regs, addr, r, padding, in_amt,
                                                            &out_amt );
        addr  += out_amt;
        addr  &= ADDRESS_MAXWRAP( regs );
        total += out_amt;

        // Update register values

        SET_GR_A( r, regs, addr );
        wlen = GR_A( r+1, regs );
        if ( wlen >= out_amt)
            SET_GR_A( r+1, regs, wlen - out_amt);
        else SET_GR_A( r+1, regs, 0);

        if (swap_rc)
            rc = -rc;
    }

    // Set the condition code and return
    if (rc == 0 && total >= MAX_CPU_AMT )
        regs->psw.cc = 3;
    else
        regs->psw.cc = (!rc ? 0 : (rc < 0 ? 1 : 2));

} /* end DEF_INST( compare_logical_long_extended ) */

#endif /* defined( FEATURE_COMPARE_AND_MOVE_EXTENDED ) */


#if defined( FEATURE_STRING_INSTRUCTION )
/*-------------------------------------------------------------------*/
/* B25D CLST  - Compare Logical String                         [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(compare_logical_string)
{
int     r1, r2;                         /* Values of R fields        */
int     i;                              /* Loop counter              */
int     dist1, dist2;                   /* length working distances  */
int     usable;                         /* usable length to page end */
int     cpu_length;                     /* CPU determined length     */
VADR    addr1, addr2;                   /* End/start addresses       */
BYTE    *main1, *main2;                 /* ptrs to compare bytes     */
BYTE    termchar;                       /* Terminating character     */

    RRE( inst, regs, r1, r2 );
    PER_ZEROADDR_CHECK2( regs, r1, r2 );

    TXFC_INSTR_CHECK( regs );

    /* Program check if bits 0-23 of register 0 not zero */
    if ((regs->GR_L(0) & 0xFFFFFF00) != 0)
        regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Load string terminating character from register 0 bits 24-31 */
    termchar = regs->GR_LHLCL(0);

    /* Determine the operand addresses */
    addr1 = regs->GR( r1 ) & ADDRESS_MAXWRAP( regs );
    addr2 = regs->GR( r2 ) & ADDRESS_MAXWRAP( regs );

    /* Establish minimum CPU determined length per the specification */
    cpu_length = 256;

    /* Should either operand cross a page boundary, we need to
       break up the search into two parts (one part in each page)
       to meet the minimum requirement of 256 CPU determined bytes.
    */
    if (unlikely( CROSSPAGEL( addr1, cpu_length ) ||
                  CROSSPAGEL( addr2, cpu_length )))
    {
        /* Compute distance to the end of the page for each operand */
        dist1 = PAGEFRAME_PAGESIZE - (addr1 & PAGEFRAME_BYTEMASK);
        dist2 = PAGEFRAME_PAGESIZE - (addr2 & PAGEFRAME_BYTEMASK);

        while (cpu_length)
        {
            usable = min( dist1, dist2 );
            usable = min( usable, cpu_length );

            main1 = MADDRL(addr1, usable, r1, regs, ACCTYPE_READ, regs->psw.pkey );
            main2 = MADDRL(addr2, usable, r2, regs, ACCTYPE_READ, regs->psw.pkey );

            for (i=0; i < usable; i++)
            {
                /* If both bytes are the terminating character, then
                   the strings are equal, so return CC=0 and leave
                   the R1 and R2 registers unchanged.
                */
                if (*main1 == termchar && *main2 == termchar)
                {
                    regs->psw.cc = 0;
                    return;
                }

                /* If FIRST operand byte is the terminating character,
                   -OR- if the first operand byte is LOWER than the
                   second operand byte, then return condition code 1
                */
                if (0
                    || *main1 == termchar
                    || (1
                        && (*main1 < *main2)
                        && (*main2 != termchar)
                       )
                )
                {
                    regs->psw.cc = 1;
                    SET_GR_A( r1, regs,addr1 );
                    SET_GR_A( r2, regs,addr2 );
                    return;
                }

                /* If SECOND operand byte is the terminating character,
                   -OR- if the first operand byte is HIGHER than the
                   second operand byte, then return condition code 2.
                */
                if (*main2 == termchar || *main1 > *main2)
                {
                    regs->psw.cc = 2;
                    SET_GR_A( r1, regs,addr1 );
                    SET_GR_A( r2, regs,addr2 );
                    return;
                }

                /* Bump both operands to the next byte */
                main1++;
                main2++;

                addr1++;
                addr1 &= ADDRESS_MAXWRAP( regs );

                addr2++;
                addr2 &= ADDRESS_MAXWRAP( regs );

            } /* end for(i) */

            /* Adjust all counts by number of bytes scanned */
            cpu_length -= usable;
            dist1      -= usable;
            dist2      -= usable;

            /* If either operand dist value is 0, then we're at a page
               boundary. Adjust the remaining distance to the remaining
               CPU determined length.
            */
            if (!dist1) dist1 = cpu_length;
            if (!dist2) dist2 = cpu_length;

        } /* end while */

        /* CPU determine number of bytes reached without finding any
           inequality. Set CC=3 and exit with the current position
           updated in the operand registers.
        */
        regs->psw.cc = 3;
        SET_GR_A( r1, regs,addr1 );
        SET_GR_A( r2, regs,addr2 );
        return;

    } /* end if */

    /* Neither operand crosses a page boundary using the minimum
       length, so extend the CPU determined length out to the
       nearest end of page of either operand.
    */
    dist1 = PAGEFRAME_PAGESIZE - (addr1 & PAGEFRAME_BYTEMASK);
    dist2 = PAGEFRAME_PAGESIZE - (addr2 & PAGEFRAME_BYTEMASK);

    cpu_length = min( dist1, dist2 ); /* (nearest end of page) */
    main1 = MADDRL(addr1, cpu_length, r1, regs, ACCTYPE_READ, regs->psw.pkey );
    main2 = MADDRL(addr2, cpu_length, r2, regs, ACCTYPE_READ, regs->psw.pkey );

    for (i=0; i < cpu_length; i++)
    {
        /* If both bytes are the terminating character, then
           the strings are equal, so return CC=0 and leave
           the R1 and R2 registers unchanged.
        */
        if (*main1 == termchar && *main2 == termchar)
        {
            regs->psw.cc = 0;
            return;
        }

        /* If FIRST operand byte is the terminating character,
           -OR- if the first operand byte is LOWER than the
           second operand byte, then return CC=1
        */
        if (0
            || *main1 == termchar
            || (1
                && (*main1 < *main2)
                && (*main2 != termchar)
               )
        )
        {
            regs->psw.cc = 1;
            SET_GR_A( r1, regs,addr1 );
            SET_GR_A( r2, regs,addr2 );
            return;
        }

        /* If SECOND operand byte is the terminating character,
           -OR- if the first operand byte is HIGHER than the
           second operand byte, then return CC=2.
        */
        if (*main2 == termchar || *main1 > *main2)
        {
            regs->psw.cc = 2;
            SET_GR_A( r1, regs,addr1 );
            SET_GR_A( r2, regs,addr2 );
            return;
        }

        /* Bump both operands to the next byte */
        main1++;
        main2++;

        addr1++;
        addr1 &= ADDRESS_MAXWRAP( regs );

        addr2++;
        addr2 &= ADDRESS_MAXWRAP( regs );

    } /* end for(i) */

    /* CPU determine number of bytes reached without finding any
       inequality. Set CC=3 and exit with the current position
       updated in the operand registers.
    */
    regs->psw.cc = 3;
    SET_GR_A( r1, regs,addr1 );
    SET_GR_A( r2, regs,addr2 );

} /* end DEF_INST( compare_logical_string ) */
#endif /* defined( FEATURE_STRING_INSTRUCTION ) */


#if defined( FEATURE_STRING_INSTRUCTION )

#undef  CUSE_DEBUG
#define CUSE_DEBUG ( 0 )

#undef  MEM_CMP_NPOS
#define MEM_CMP_NPOS ( -1 )

#undef  PAGEBYTES
#define PAGEBYTES( _ea )  (PAGEFRAME_PAGESIZE - ((_ea) & PAGEFRAME_BYTEMASK))

#undef  MAINSTOR_PAGEBASE
#define MAINSTOR_PAGEBASE( _ma )  ((BYTE*) ((uintptr_t) ( _ma ) & PAGEFRAME_PAGEMASK))

/*-------------------------------------------------------------------*/
/*    mem_cmp_first_equ  --  compare memory for first equal byte     */
/*-------------------------------------------------------------------*/
/* Input:                                                            */
/*      regs    CPU register context                                 */
/*      ea1     Logical address of leftmost byte of operand-1        */
/*      b1      Operand-1 base register                              */
/*      ea2     Logical address of leftmost byte of operand-2        */
/*      b2      Operand-2 base register                              */
/*      len     Compare length                                       */
/* Output:                                                           */
/*      rc      MEM_CMP_NPOS : no equ byte                           */
/*              0 to (len-1) : index of equ byte                     */
/*                                                                   */
/*-------------------------------------------------------------------*/
int ARCH_DEP( mem_cmp_first_equ )
(
    REGS*  regs,                // register context
    VADR   ea1, int b1,         // op-1 (dst)
    VADR   ea2, int b2,         // op-2 (src)
    U32    len                  // compare length
)
{
    BYTE   *m1,   *m2;          // operand mainstor addresses
    BYTE   *m1pg, *m2pg;        // operand page
    U32    i;                   // loop index

    /* fast exit */
    if (len == 0) return MEM_CMP_NPOS;

    // Translate left most byte of each operand
    m1 = MADDRL(ea1 & ADDRESS_MAXWRAP( regs ), PAGEBYTES( ea1 ), b1, regs, ACCTYPE_READ, regs->psw.pkey );
    m2 = MADDRL(ea2 & ADDRESS_MAXWRAP( regs ), PAGEBYTES( ea2 ), b2, regs, ACCTYPE_READ, regs->psw.pkey );

    m1pg = MAINSTOR_PAGEBASE ( m1 );
    m2pg = MAINSTOR_PAGEBASE ( m2 );

    for (i = 0; i < len ; i++)
    {
        /* compare bytes */
        if (*m1 == *m2)
            return i;

        /* update mainstore addresses */
        m1++;
        m2++;

        /* check for page cross */
        if (MAINSTOR_PAGEBASE ( m1 ) != m1pg)
        {
            m1 = MADDRL((ea1 + (i+1)) & ADDRESS_MAXWRAP( regs ), PAGEFRAME_PAGESIZE, b1, regs, ACCTYPE_READ, regs->psw.pkey );
            m1pg = MAINSTOR_PAGEBASE ( m1 );
        }
        if (MAINSTOR_PAGEBASE ( m2 ) != m2pg)
        {
            m2 = MADDRL((ea2 + (i+1)) & ADDRESS_MAXWRAP( regs ), PAGEFRAME_PAGESIZE, b2, regs, ACCTYPE_READ, regs->psw.pkey );
            m2pg = MAINSTOR_PAGEBASE ( m2 );
        }
    }

    /* no equ byte in memory */
    return MEM_CMP_NPOS;

} /* end ARCH_DEP( mem_cmp_first_equ ) */

/*-------------------------------------------------------------------*/
/* mem_cmp_first_substr - compare memory for first possible substring*/
/*                                                                   */
/* Possile substrings are searched right to left to find the         */
/* rightmost non-equal byte. The next byte establishes the next      */
/* possible substring starting position for comparison.              */
/*-------------------------------------------------------------------*/
/* Input:                                                            */
/*      regs    CPU register context                                 */
/*      ea1     Logical address of leftmost byte of operand-1        */
/*      b1      Operand-1 base register                              */
/*      ea2     Logical address of leftmost byte of operand-2        */
/*      b2      Operand-2 base register                              */
/*      len     Compare length                                       */
/*      sublen  Substring length                                     */
/*     *equlen  possile substring length (if requested)              */
/* Output:                                                           */
/*      rc      MEM_CMP_NPOS : no equ byte                           */
/*              0 to (len-1) : index of first possible substring     */
/*                                                                   */
/*-------------------------------------------------------------------*/
int ARCH_DEP( mem_cmp_first_substr )
(
    REGS*  regs,                // register context
    VADR   ea1, int b1,         // op-1 (dst)
    VADR   ea2, int b2,         // op-2 (src)
    int    len,                 // compare length
    int    sublen,              // substring length
    int*   equlen               // # of equal bytes
)
{
    BYTE   *m1,   *m2;          // operand mainstor addresses
    BYTE   *m1pg, *m2pg;        // operand page

    int    ss_index=0;          // possible substring index
    int    ss_scan_index=0;     // substring scan index
    int    ss_scan_work;        // substring scan length
    int    ss_equ_len;          // substring equal length

    /* Is caller interested in equality length? */
    if (equlen)
        *equlen = 0;

    /* fast exits */
    if (len <= 0) return MEM_CMP_NPOS;
    if (sublen <= 0) return MEM_CMP_NPOS;

    /* operand mainstore addresses */
    m1 = MADDRL(ea1, PAGEBYTES( ea1 ), b1, regs, ACCTYPE_READ, regs->psw.pkey );
    m2 = MADDRL(ea2, PAGEBYTES( ea2 ), b2, regs, ACCTYPE_READ, regs->psw.pkey );

    m1pg = MAINSTOR_PAGEBASE ( m1 );
    m2pg = MAINSTOR_PAGEBASE ( m2 );

    for (ss_index = 0; ss_index < len; ss_index = ( ss_scan_index +1 ) )
    {
        /* reset candidate substring length */
        ss_equ_len = 0;

        /* righttmost byte of candidate substring */
        ss_scan_index = min( ss_index + (sublen-1), (len-1));
        ss_scan_work = ss_scan_index - ss_index;

#if CUSE_DEBUG
        logmsg("CUSE: MEM_CMP_FIRST_SUBSTR outer: len=%d, sublen=%d, scan_ss_index=%d, ss_index=%d, ss_equ_len=%d, ea1+idx=%X , m1=%p, ea2+idx=%X, m2=%p \n",
            len, sublen, ss_scan_index, ss_index, ss_equ_len, ea1+ss_scan_index, m1, ea2+ss_scan_index, m2);
#endif
        /* operand candidate substring rightmost mainstore address */
        m1 += ss_scan_work;
        m2 += ss_scan_work;

        /* check for page cross */
        if (MAINSTOR_PAGEBASE ( m1 ) != m1pg)
        {
            m1 = MADDRL((ea1 + ss_scan_index) & ADDRESS_MAXWRAP( regs ), 1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
            m1pg = MAINSTOR_PAGEBASE ( m1 );
        }
        if (MAINSTOR_PAGEBASE ( m2 ) != m2pg)
        {
            m2 = MADDRL((ea2 + ss_scan_index) & ADDRESS_MAXWRAP( regs ), 1, b2, regs, ACCTYPE_READ, regs->psw.pkey );
            m2pg = MAINSTOR_PAGEBASE ( m2 );
        }

        /* check candidate substring - right to left */
        for ( ; ss_scan_index >= ss_index ; ss_scan_index-- )
        {
#if CUSE_DEBUG
        logmsg("CUSE: MEM_CMP_FIRST_SUBSTR inner: len=%d, sublen=%d, scan_ss_index=%d, ss_index=%d, ss_equ_len=%d, m1=%p, m2=%p \n",
            len, sublen, ss_scan_index, ss_index, ss_equ_len, m1, m2);
#endif
            /* compare bytes */
            if (*m1 != *m2)
                break;

            /* update partial substring length */
            ss_equ_len++;

            /* update mainstore addresses */
            m1--;
            m2--;

            /* check for page cross */
            if (MAINSTOR_PAGEBASE ( m1 ) != m1pg)
            {
                m1 = MADDRL((ea1 + (ss_scan_index-1)) & ADDRESS_MAXWRAP( regs ), 1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
                m1pg = MAINSTOR_PAGEBASE ( m1 );
            }
            if (MAINSTOR_PAGEBASE ( m2 ) != m2pg)
            {
                m2 = MADDRL((ea2 + (ss_scan_index-1)) & ADDRESS_MAXWRAP( regs ), 1, b2, regs, ACCTYPE_READ, regs->psw.pkey );
                m2pg = MAINSTOR_PAGEBASE ( m2 );
            }
        } /* end check candidate substring - right to left */

        /* Is caller interested in equality length? */
        if (equlen)
            *equlen = ss_equ_len;

        /* substring found? */
        if ( ss_scan_index < ss_index ) return ss_index;

        /* last possible substring extends beyond length? */
        if ( ss_equ_len > 0 && ss_index + sublen >= len) return ss_scan_index +1;

        /* update mainstore addresses to next byte */
        m1++;
        m2++;

        /* check for page cross */
        if (MAINSTOR_PAGEBASE ( m1 ) != m1pg)
        {
            m1 = MADDRL((ea1 + (ss_scan_index +1)) & ADDRESS_MAXWRAP( regs ), 1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
            m1pg = MAINSTOR_PAGEBASE ( m1 );
        }
        if (MAINSTOR_PAGEBASE ( m2 ) != m2pg)
        {
            m2 = MADDRL((ea2 + (ss_scan_index +1)) & ADDRESS_MAXWRAP( regs ), 1, b2, regs, ACCTYPE_READ, regs->psw.pkey );
            m2pg = MAINSTOR_PAGEBASE ( m2 );
        }
    } /* end check for substring */

    /* no substring; no equal bytes found       */
    /* Is caller interested in equality length? */
    if (equlen)
        *equlen = 0;

    return MEM_CMP_NPOS;

} /* end ARCH_DEP( mem_cmp_first_substr ) */

/*-------------------------------------------------------------------*/
/*        mem_cmp_last_neq  --  compare memory for last neq byte     */
/*-------------------------------------------------------------------*/
/* Input:                                                            */
/*      regs    CPU register context                                 */
/*      ea1     Logical address of leftmost byte of operand-1        */
/*      b1      Operand-1 base register                              */
/*      ea2     Logical address of leftmost byte of operand-2        */
/*      b2      Operand-2 base register                              */
/*      len     Compare length                                       */
/* Output:                                                           */
/*      rc      MEM_CMP_NPOS : no neq byte (memory is equal)         */
/*              0 to (len-1) : index of neq byte                     */
/*                                                                   */
/*-------------------------------------------------------------------*/
int ARCH_DEP( mem_cmp_last_neq )
(
    REGS*  regs,                // register context
    VADR   ea1, int b1,         // op-1 (dst)
    VADR   ea2, int b2,         // op-2 (src)
    int    len                  // compare length
)
{
    BYTE   *m1,   *m2;          // operand mainstor addresses
    BYTE   *m1pg, *m2pg;        // operand page
    int    i;                   // loop index

    /* fast exit */
    if (len <= 0) return MEM_CMP_NPOS;

    // Translate righttmost byte of each operand
    m1 = MADDRL((ea1 + (len-1)) & ADDRESS_MAXWRAP( regs ), 1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
    m2 = MADDRL((ea2 + (len-1)) & ADDRESS_MAXWRAP( regs ), 1, b2, regs, ACCTYPE_READ, regs->psw.pkey );

    m1pg = MAINSTOR_PAGEBASE ( m1 );
    m2pg = MAINSTOR_PAGEBASE ( m2 );

    for (i = (len-1); i >= 0 ; i--)
    {
        /* compare bytes */
        if (*m1 != *m2)
            return i;

        /* update mainstore addresses */
        m1--;
        m2--;

        /* check for page cross */
        if (MAINSTOR_PAGEBASE ( m1 ) != m1pg)
        {
            m1 = MADDRL((ea1 + (i-1)) & ADDRESS_MAXWRAP( regs ), 1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
            m1pg = MAINSTOR_PAGEBASE ( m1 );
        }
        if (MAINSTOR_PAGEBASE ( m2 ) != m2pg)
        {
            m2 = MADDRL((ea2 + (i-1)) & ADDRESS_MAXWRAP( regs ), 1, b2, regs, ACCTYPE_READ, regs->psw.pkey );
            m2pg = MAINSTOR_PAGEBASE ( m2 );
        }
    }

    /* no neq bytes; memory is equal */
    return MEM_CMP_NPOS;

} /* end ARCH_DEP( mem_cmp_last_neq ) */

/*-------------------------------------------------------------------*/
/* mem_pad_cmp_last_neq -- compare memory to pad for last neq byte   */
/*-------------------------------------------------------------------*/
/* Input:                                                            */
/*      regs    CPU register context                                 */
/*      ea1     Logical address of leftmost byte of operand-1        */
/*      b1      Operand-1 base register                              */
/*      pad     Padding byte                                         */
/*      len     Compare length                                       */
/* Output:                                                           */
/*      rc      MEM_CMP_NPOS : no neq byte (memory is equal to pad)  */
/*              0 to (len-1) : index of neq byte                     */
/*                                                                   */
/*-------------------------------------------------------------------*/
int ARCH_DEP( mem_pad_cmp_last_neq )
(
    REGS*  regs,                // register context
    VADR   ea1, int b1,         // op-1 (dst)
    const BYTE pad,             // pad byte
    int    len                  // compare length
)
{
    BYTE   *m1;                 // operand mainstor addresses
    BYTE   *m1pg;               // operand page
    int    i;                   // loop index

    if (len <= 0) return MEM_CMP_NPOS;

    // Translate righttmost byte of operand
    m1 = MADDRL((ea1 + (len-1)) & ADDRESS_MAXWRAP( regs ), 1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
    m1pg = MAINSTOR_PAGEBASE ( m1 );

    for (i = (len-1); i >= 0 ; i--)
    {
        /* compare byte to pad */
        if (*m1 != pad)
            return i;

        /* update mainstore address */
        m1--;

        /* check for page cross */
        if (MAINSTOR_PAGEBASE ( m1 ) != m1pg)
        {
            m1 = MADDRL((ea1 + (i-1)) & ADDRESS_MAXWRAP( regs ), 1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
            m1pg = MAINSTOR_PAGEBASE ( m1 );
        }
    }

    /* no neq bytes; memory is equal to pad */
    return MEM_CMP_NPOS;

} /* end ARCH_DEP( mem_pad_cmp_last_neq ) */

/*-------------------------------------------------------------------*/
/* B257 CUSE  - Compare Until Substring Equal                  [RRE] */
/*-------------------------------------------------------------------*/
#undef  CUSE_MAX
#define CUSE_MAX    _4K

DEF_INST( compare_until_substring_equal )
{
    int     r1, r2;                 // Values of R fields
    int     i;                      // Loop counter
    int     cc = 0;                 // Condition code
    VADR    addr1, addr2;           // Operand addresses
    VADR    eqaddr1, eqaddr2;       // Address of equal substring
    S64     len1, len2;             // Operand lengths
    S64     remlen1, remlen2;       // Lengths remaining
    BYTE    byte1, byte2;           // Operand bytes
    BYTE    pad;                    // Padding byte
    BYTE    sublen;                 // Substring length
    BYTE    equlen = 0;             // Equal byte counter

    int     peeklen;                // Operand look ahead length
    int     scanlen;                // scan length
    int     padlen;                 // Operand pad length
    int     lastneq;                // mem_cmp_last_neq return
    int     firstequ;               // mem_cmp_first_equ return
    int     rc;                     // work - return code
    U32     idx =0;                 // mem_cmp() - index

    RRE( inst, regs, r1, r2 );
    PER_ZEROADDR_LCHECK2( regs, r1, r1+1, r2, r2+1 );

    TXFC_INSTR_CHECK( regs );
    ODD2_CHECK( r1, r2, regs );

    /* Load substring length from bits 24-31 of register 0 */
    sublen = regs->GR_LHLCL( 0 );

    /* Load padding byte from bits 24-31 of register 1 */
    pad = regs->GR_LHLCL( 1 );

    /* Determine the destination and source addresses */
    addr1 = regs->GR( r1 ) & ADDRESS_MAXWRAP( regs );
    addr2 = regs->GR( r2 ) & ADDRESS_MAXWRAP( regs );

    /* update regs so unused bits zeroed */
    SET_GR_A( r1, regs, addr1 );
    SET_GR_A( r2, regs, addr2 );

    /* Load signed operand lengths from R1+1 and R2+1 */
    len1 = GR_A( r1+1, regs );
    len2 = GR_A( r2+1, regs );

    /* Initialize equal string addresses and lengths */
    eqaddr1 = addr1;
    eqaddr2 = addr2;
    remlen1 = len1;
    remlen2 = len2;

    /* If substring length is zero, exit with condition code 0 */
    if (sublen == 0)
    {
        regs->psw.cc = 0;
        return;
    }

    /* If both operand lengths are zero, exit with condition code 2 */
    if (len1 <= 0 && len2 <= 0)
    {
        regs->psw.cc = 2;
        return;
    }

    /* If r1=r2, exit with condition code 0 or 1*/
    if (r1 == r2)
    {
        regs->psw.cc = (len1 < sublen) ? 1 : 0;
        return;
    }

    /* Process operands from left to right */
    for (i=0; len1 > 0 || len2 > 0 ; i++)
    {
#if CUSE_DEBUG
         logmsg("CUSE: addr1=%X, len1=%ld, addr2=%X, len2=%ld, equlen=%ld, eqaddr1=%X, remlen1=%ld, eqaddr2=%X, remlen2=%ld, i=%d \n",
                        addr1, len1, addr2, len2, equlen, eqaddr1, remlen1, eqaddr2, remlen2, i);
#endif
        /* If equal byte count has reached substring length
           exit with condition code zero */
        if (equlen == sublen)
        {
            cc = 0;
            break;
        }

        /* interrupt and cc=3 checks: last comparision was unequal */
        if ( equlen == 0 )
        {
            /* If 4096 bytes have been compared, and the last bytes
            compared were unequal, exit with condition code 3 */
            if (i >= CUSE_MAX)
            {
                cc = 3;
                break;
            }
        }

        /* ---------------------------- */
        /* Special Cases Optimizations  */
        /* ---------------------------- */

        /* ---------------------------------- */
        /* Special case: substring length = 1 */
        /* ---------------------------------- */
        if (sublen == 1 )
        {
            scanlen = min ( len1, len2 );
            firstequ = ARCH_DEP( mem_cmp_first_equ )( regs, addr1, r1, addr2, r2, scanlen);

#if CUSE_DEBUG
            logmsg("CUSE: mem_cmp_first_equ: addr1=%X, addr2=%X, scanlen=%d, firstequ=%d \n",
                addr1, addr2, scanlen, firstequ);
#endif
            if (firstequ == MEM_CMP_NPOS)
            {
                /* no matching byte */
                addr1 += scanlen;
                addr1 &= ADDRESS_MAXWRAP( regs );
                len1  -= scanlen;

                addr2 += scanlen;
                addr2 &= ADDRESS_MAXWRAP( regs );
                len2  -= scanlen;

                /* update bytes compared for cc=3 check */
                i += scanlen;

                equlen = 0;
                cc = 2;
                continue;
            }
            else
            {
                /* found a matching byte */
                addr1 += firstequ;
                addr1 &= ADDRESS_MAXWRAP( regs );
                len1  -= firstequ;

                addr2 += firstequ;
                addr2 &= ADDRESS_MAXWRAP( regs );
                len2  -= firstequ;

                /* Update equal string addresses and lengths */
                eqaddr1 = addr1;
                eqaddr2 = addr2;
                remlen1 = len1;
                remlen2 = len2;

                equlen = 1;
                cc = 0;
                continue;
            }
        }  /* end Special case: substring length = 1 */

        /* --------------------------------------------------- */
        /* Special case: partial substring found               */
        /*               - continue compare                    */
        /* --------------------------------------------------- */
        if (equlen > 0 && len1 > 0 && len2 > 0)
        {
            scanlen = min (sublen - equlen, min( len1, len2) );
            rc = ARCH_DEP( mem_cmp ) (regs, addr1, r1, addr2, r2, scanlen, &idx);

#if CUSE_DEBUG
            logmsg("CUSE: mem_cmp: addr1=%X, addr2=%X, scanlen=%d, sublen=%d, rc=%d, idx=%u \n",
                addr1, addr2, scanlen, sublen, rc, idx);
#endif
            if (rc == 0)
            {
                /* larger partial substring found - may need pad to complete */
                addr1 += scanlen;
                addr1 &= ADDRESS_MAXWRAP( regs );
                len1  -= scanlen;

                addr2 += scanlen;
                addr2 &= ADDRESS_MAXWRAP( regs );
                len2  -= scanlen;

                equlen += scanlen;

                /* update bytes compared for cc=3 check */
                i += scanlen;

                cc = ( sublen == equlen ) ? 0 : 1;
                continue;
            }
            else
            {
                /* substring not found */
                /* update to nonequal byte */
                addr1 += (idx +1);
                addr1 &= ADDRESS_MAXWRAP( regs );
                len1  -= (idx +1);

                addr2 += (idx +1);
                addr2 &= ADDRESS_MAXWRAP( regs );
                len2  -= (idx +1);

                equlen = 0;

                /* update bytes compared for cc=3 check */
                i += (idx +1);

                cc = 2;
                continue;
            }
        }   /* end Special case: partial substring found */

        /* ----------------------------------------------------------------------- */
        /* Special case: no partial substring - scan for first possible substring  */
        /* ----------------------------------------------------------------------- */
        scanlen = min( len1, len2 );
        if (equlen == 0 && scanlen > sublen)
        {
            int ss_len = 0;
            firstequ = ARCH_DEP( mem_cmp_first_substr )( regs, addr1, r1, addr2, r2, scanlen, sublen, &ss_len);

#if CUSE_DEBUG
            logmsg("CUSE: mem_cmp_first_substr: addr1=%X, addr2=%X, scanlen=%d, sublen=%d, firstequ=%d, ss_len=%d \n",
                addr1, addr2, scanlen, sublen, firstequ, ss_len);
#endif
            if (firstequ == MEM_CMP_NPOS)
            {
                /* no possible substring found - update to next end of scanned memory */
                addr1 += scanlen ;
                addr1 &= ADDRESS_MAXWRAP( regs );
                len1  -= scanlen;

                addr2 += scanlen;
                addr2 &= ADDRESS_MAXWRAP( regs );
                len2  -= scanlen;

                /* update bytes compared for cc=3 check */
                i += scanlen;

                /* save the start of possible substring addresses and remaining lengths */
                eqaddr1 = addr1;
                eqaddr2 = addr2;
                remlen1 = len1;
                remlen2 = len2;
                equlen = 0;

                cc = 2;
            }
            else
            {
                /* found a start of possible substring byte */

                /* save the start of possible substring addresses and remaining lengths */
                eqaddr1 = addr1 + firstequ;
                eqaddr2 = addr2 + firstequ;
                remlen1 = len1 - firstequ;
                remlen2 = len2 - firstequ;
                equlen = ss_len;

                /* update address and length to next byte */
                addr1 += (firstequ + ss_len);
                addr1 &= ADDRESS_MAXWRAP( regs );
                len1  -= (firstequ + ss_len);

                addr2 += (firstequ + ss_len);
                addr2 &= ADDRESS_MAXWRAP( regs );
                len2  -= (firstequ + ss_len);

                /* update bytes compared for cc=3 check */
                i += (firstequ + ss_len);

                cc = ( sublen == equlen ) ? 0 : 1;
            }
            continue;

        }   /*end Special case: no partial substring - scan for first possible substring  */

        /* ------------------------------------------------------------------- */
        /* Special case: scan substring right to left                          */
        /* ------------------------------------------------------------------- */
        /* Peek look ahead at last non-pad byte of candidate substring */
        peeklen = min( sublen - equlen, min ( len1, len2 ) );
        if (peeklen > 1)
        {
            lastneq = ARCH_DEP( mem_cmp_last_neq )( regs, addr1, r1, addr2, r2, peeklen);

#if CUSE_DEBUG
            logmsg("CUSE: mem_cmp_last_neq: addr1=%X, addr2=%X, peaklen=%d, lastneq=%d \n",
                addr1, addr2, peeklen, lastneq);
#endif
            if (lastneq == MEM_CMP_NPOS)
            {
                if (sublen == peeklen + equlen)
                {
                    /* complete substring found */
                    cc = 0;
                    break;
                }
                else
                {
                    /* partial substring found - possibly need padding */
                    addr1 += peeklen;
                    addr1 &= ADDRESS_MAXWRAP( regs );
                    len1  -= peeklen;

                    addr2 += peeklen;
                    addr2 &= ADDRESS_MAXWRAP( regs );
                    len2  -= peeklen;

                    /* update bytes compared for cc=3 check */
                    i += peeklen;

                    equlen += peeklen;
                    cc = 1;
                    continue;
                }
            }
            else
            {
                /* found a mismatch - update to nonequal byte */
                addr1 += (lastneq +1);
                addr1 &= ADDRESS_MAXWRAP( regs );
                len1  -= (lastneq +1);

                addr2 += (lastneq +1);
                addr2 &= ADDRESS_MAXWRAP( regs );
                len2  -= (lastneq +1);

                /* update bytes compared for cc=3 check */
                i += (lastneq +1);

                equlen = 0;
                cc = 2;
                continue;
            }
        }   /* end Special case: scan substring right to left */

        /* -------------------------------------------------------------- */
        /* Special case: only have operand-1. only padding check required */
        /* -------------------------------------------------------------- */
        if ( len1 > 0 && len2 == 0)
        {
            padlen = min( len1, (sublen - equlen));
            lastneq = ARCH_DEP( mem_pad_cmp_last_neq )( regs, addr1, r1, pad, padlen );

#if CUSE_DEBUG
            logmsg("CUSE: op-1 pad check: addr1=%X, len1=%ld, padlen=%d, eqaddr1=%X, equlen=%d, remlen1=%d, lastneq=%d \n",
                addr1, len1, padlen, eqaddr1, equlen, remlen1, lastneq );
#endif
            if (lastneq == MEM_CMP_NPOS)
            {
                addr1 += padlen;
                addr1 &= ADDRESS_MAXWRAP( regs );
                len1  -= padlen;

                equlen += padlen;

                cc = (sublen == equlen ) ? 0 : 1;
                break;
            }
            else
            {
                 /* found a mismatch - update to nonequal byte */
                addr1 += (lastneq +1);
                addr1 &= ADDRESS_MAXWRAP( regs );
                len1  -= (lastneq +1);

                /* update bytes compared for cc=3 check */
                i += (lastneq +1);

                eqaddr1 = addr1;
                remlen1 = len1;

                equlen = 0;
                cc = 2;
                continue;
            }
        }   /* end Special case: only have operand-1. only padding check required */

        /* -------------------------------------------------------------- */
        /* Special case: only have operand-2. only padding check required */
        /* -------------------------------------------------------------- */
        if ( len1 == 0 && len2 > 0)
        {
            padlen = min( len2, (sublen - equlen));
            lastneq = ARCH_DEP( mem_pad_cmp_last_neq )( regs, addr2, r2, pad, padlen );

#if CUSE_DEBUG
            logmsg("CUSE: op-2 pad check: addr2=%X, len2=%ld, padlen=%d, eqaddr2=%X, equlen=%d, remlen2=%d, lastneq=%d \n",
                addr2, len2, padlen, eqaddr2, equlen, remlen2, lastneq );
#endif
            if (lastneq == MEM_CMP_NPOS)
            {
                addr2 += padlen;
                addr2 &= ADDRESS_MAXWRAP( regs );
                len2  -= padlen;

                equlen += padlen;

                cc = (sublen == equlen ) ? 0 : 1;
                break;
            }
            else
            {
                /* found a mismatch - update to nonequal byte */
                addr2 += (lastneq +1);
                addr2 &= ADDRESS_MAXWRAP( regs );
                len2  -= (lastneq +1);

                /* update bytes compared for cc=3 check */
                i += (lastneq +1);

                eqaddr2 = addr2;
                remlen2 = len2;

                equlen = 0;
                cc = 2;
                continue;
            }
        }   /* Special case: only have operand-2. only padding check required */

        /* ------------------------------------------------------- */
        /* General case:                                           */
        /*      Scan left to right byte by byte                    */
        /* ------------------------------------------------------- */

#if CUSE_DEBUG
        logmsg("CUSE: general: addr1=%X, len1=%ld, addr2=%X, len2=%ld, equlen=%d \n",
            addr1, len1, addr2, len2, equlen);
#endif
        /* Fetch byte from first operand, or use padding byte */
        if (len1 > 0)
            byte1 = ARCH_DEP( vfetchb )( addr1, r1, regs );
        else
            byte1 = pad;

        /* Fetch byte from second operand, or use padding byte */
        if (len2 > 0)
            byte2 = ARCH_DEP( vfetchb )( addr2, r2, regs );
        else
            byte2 = pad;

        /* Test if bytes compare equal */
        if (byte1 == byte2)
        {
            /* If this is the first equal byte, save the start of
               substring addresses and remaining lengths */
            if (equlen == 0)
            {
                eqaddr1 = addr1;
                eqaddr2 = addr2;
                remlen1 = len1;
                remlen2 = len2;
            }

            /* Count the number of equal bytes */
            equlen++;

            /* Set condition code 1 */
            cc = 1;
        }
        else
        {
            /* Reset equal byte count and set condition code 2 */
            equlen = 0;
            cc = 2;
        }

        /* Update the first operand address and length */
        if (len1 > 0)
        {
            addr1++;
            addr1 &= ADDRESS_MAXWRAP( regs );
            len1--;
        }

        /* Update the second operand address and length */
        if (len2 > 0)
        {
            addr2++;
            addr2 &= ADDRESS_MAXWRAP( regs );
            len2--;
        }

    } /* end for(i) */

#if CUSE_DEBUG
         logmsg("CUSE: scan complete: addr1=%X, len1=%ld, addr2=%X, len2=%ld, equlen=%ld, eqaddr1=%X, remlen1=%ld, eqaddr2=%X, remlen2=%ld, i=%d, cc=%d \n",
                        addr1, len1, addr2, len2, equlen, eqaddr1, remlen1, eqaddr2, remlen2, i, cc);
#endif

    /* Update the registers */
    if (cc < 2 && equlen > 0)
    {
        /* Update R1 and R2 to point to the equal substring */
        SET_GR_A( r1, regs, eqaddr1 );
        SET_GR_A( r2, regs, eqaddr2 );

        /* Set R1+1 and R2+1 to length remaining in each
           operand after the start of the substring */
        SET_GR_A( r1+1, regs, remlen1 );
        SET_GR_A( r2+1, regs, remlen2 );
    }
    else
    {
        /* Update R1 and R2 to point to next bytes to compare */
        SET_GR_A( r1, regs, addr1 );
        SET_GR_A( r2, regs, addr2 );

        /* Set R1+1 and R2+1 to remaining operand lengths */
        SET_GR_A( r1+1, regs, len1 );
        SET_GR_A( r2+1, regs, len2 );
    }

    /* Set condition code */
    regs->psw.cc =  cc;

} /* end DEF_INST( compare_until_substring_equal ) */

#endif /* defined( FEATURE_STRING_INSTRUCTION ) */


#ifdef FEATURE_EXTENDED_TRANSLATION_FACILITY_1
/*-------------------------------------------------------------------*/
/* B2A6 CU21 (CUUTF) - Convert Unicode to UTF-8              [RRF-c] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_utf16_to_utf8)
{
int     r1, r2;                         /* Register numbers          */
int     m3;                             /* Mask                      */
int     i;                              /* Loop counter              */
int     cc = 0;                         /* Condition code            */
VADR    addr1, addr2;                   /* Operand addresses         */
GREG    len1, len2;                     /* Operand lengths           */
VADR    naddr2;                         /* Next operand 2 addr       */
GREG    nlen2;                          /* Next operand 2 length     */
U16     uvwxy;                          /* Unicode work area         */
U16     unicode1;                       /* Unicode character         */
U16     unicode2;                       /* Unicode low surrogate     */
GREG    n;                              /* Number of UTF-8 bytes - 1 */
BYTE    utf[4];                         /* UTF-8 bytes               */
#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
bool    wfc;                            /* Well-Formedness-Checking  */
#endif

    RRF_M(inst, regs, r1, r2, m3);
    PER_ZEROADDR_LCHECK2( regs, r1, r1+1, r2, r2+1 );
    ODD2_CHECK(r1, r2, regs);

#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
    /* Set WellFormednessChecking */
    if ((m3 & 0x01) && FACILITY_ENABLED( 030_ETF3_ENHANCEMENT, regs ))
      wfc = true;
    else
      wfc = false;
#endif

    /* Determine the destination and source addresses */
    addr1 = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    addr2 = regs->GR(r2) & ADDRESS_MAXWRAP(regs);

    /* Load operand lengths from bits 0-31 of R1+1 and R2+1 */
    len1 = GR_A(r1+1, regs);
    len2 = GR_A(r2+1, regs);

    if (len1 == 0 && len2 > 1)
        cc = 1;

    /* Process operands from left to right */
    for (i = 0; len1 > 0 && len2 > 0; i++)
    {
        /* If 4096 characters have been converted, exit with cc=3 */
        if (i >= 4096)
        {
            cc = 3;
            break;
        }

        /* Exit if fewer than 2 bytes remain in source operand */
        if (len2 < 2) break;

        /* Fetch two bytes from source operand */
        unicode1 = ARCH_DEP(vfetch2) ( addr2, r2, regs );
        naddr2 = addr2 + 2;
        naddr2 &= ADDRESS_MAXWRAP(regs);
        nlen2 = len2 - 2;

        /* Convert Unicode to UTF-8 */
        if (unicode1 < 0x0080)
        {
            /* Convert Unicode 0000-007F to one UTF-8 byte */
            utf[0] = (BYTE)unicode1;
            n = 0;
        }
        else if (unicode1 < 0x0800)
        {
            /* Convert Unicode 0080-07FF to two UTF-8 bytes */
            utf[0] = (BYTE)0xC0 | (BYTE)(unicode1 >> 6);
            utf[1] = (BYTE)0x80 | (BYTE)(unicode1 & 0x003F);
            n = 1;
        }
        else if (unicode1 < 0xD800 || unicode1 >= 0xDC00)
        {
            /* Convert Unicode 0800-D7FF or DC00-FFFF
               to three UTF-8 bytes */
            utf[0] = (BYTE)0xE0 | (BYTE)(unicode1 >> 12);
            utf[1] = (BYTE)0x80 | (BYTE)((unicode1 & 0x0FC0) >> 6);
            utf[2] = (BYTE)0x80 | (BYTE)(unicode1 & 0x003F);
            n = 2;
        }
        else
        {
            /* Exit if fewer than 2 bytes remain in source operand */
            if (nlen2 < 2) break;

            /* Fetch the Unicode low surrogate */
            unicode2 = ARCH_DEP(vfetch2) ( naddr2, r2, regs );
            naddr2 += 2;
            naddr2 &= ADDRESS_MAXWRAP(regs);
            nlen2 -= 2;

#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
            /* WellFormdnessChecking */
            if (wfc)
            {
              if (unicode2 < 0xdc00 || unicode2 > 0xdf00)
              {
                regs->psw.cc = 2;
                return;
              }
            }
#endif /* defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY ) */

            /* Convert Unicode surrogate pair to four UTF-8 bytes */
            uvwxy = ((unicode1 & 0x03C0) >> 6) + 1;
            utf[0] = (BYTE)0xF0 | (BYTE)(uvwxy >> 2);
            utf[1] = (BYTE)0x80 | (BYTE)((uvwxy & 0x0003) << 4)
                        | (BYTE)((unicode1 & 0x003C) >> 2);
            utf[2] = (BYTE)0x80 | (BYTE)((unicode1 & 0x0003) << 4)
                        | (BYTE)((unicode2 & 0x03C0) >> 6);
            utf[3] = (BYTE)0x80 | (BYTE)(unicode2 & 0x003F);
            n = 3;
        }

        /* Exit cc=1 if too few bytes remain in destination operand */
        if (len1 <= n)
        {
            cc = 1;
            break;
        }

        /* Store the result bytes in the destination operand */
        ARCH_DEP(vstorec) ( utf, n, addr1, r1, regs );
        addr1 += n + 1;
        addr1 &= ADDRESS_MAXWRAP(regs);
        len1 -= n + 1;

        /* Update operand 2 address and length */
        addr2 = naddr2;
        len2 = nlen2;

        /* Update the registers */
        SET_GR_A(r1, regs,addr1);
        SET_GR_A(r1+1, regs,len1);
        SET_GR_A(r2, regs,addr2);
        SET_GR_A(r2+1, regs,len2);

        if (len1 == 0 && len2 != 0)
            cc = 1;

    } /* end for(i) */

    regs->psw.cc = cc;

} /* end convert_utf16_to_utf8 */


/*-------------------------------------------------------------------*/
/* B2A7 CU12 (CUTFU) - Convert UTF-8 to Unicode              [RRF-c] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_utf8_to_utf16)
{
    int     r1, r2;                         /* Register numbers          */
    int     m3;                             /* Mask                      */
    int     i;                              /* Loop counter              */
    int     cc = 0;                         /* Condition code            */
    VADR    addr1, addr2;                   /* Operand addresses         */
    GREG    len1, len2;                     /* Operand lengths           */
    int     pair;                           /* 1=Store Unicode pair      */
    U16     uvwxy;                          /* Unicode work area         */
    U16     unicode1;                       /* Unicode character         */
    U16     unicode2 = 0;                   /* Unicode low surrogate     */
    GREG    n;                              /* Number of UTF-8 bytes - 1 */
    BYTE    utf[4];                         /* UTF-8 bytes               */
    #if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
    bool    wfc;                            /* WellFormednessChecking    */
    #endif

    BYTE*   s2;                    // Source mainstor address (ie. addr2)
    BYTE*   s2pg;                  // Source base page
    BYTE*   d1;                    // Destination mainstor address (ie. addr1)
    BYTE*   d1pg;                  // Destination base page
    int     d1len;                 // Destination bytes written

    RRF_M(inst, regs, r1, r2, m3);
    PER_ZEROADDR_LCHECK2( regs, r1, r1+1, r2, r2+1 );
    ODD2_CHECK(r1, r2, regs);

#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
    /* Set WellFormednessChecking */
    if ((m3 & 0x01) && FACILITY_ENABLED( 030_ETF3_ENHANCEMENT, regs ))
      wfc = true;
    else
      wfc = false;
#endif

    /* Determine the destination and source addresses */
    addr1 = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    addr2 = regs->GR(r2) & ADDRESS_MAXWRAP(regs);

    /* Load operand lengths from bits 0-31 of R1+1 and R2+1 */
    len1 = GR_A(r1+1, regs);
    len2 = GR_A(r2+1, regs);

    if (len1 == 0 && len2 > 0)
        cc = 1;

    /* Get mainstor address to Source byte */
    s2 = MADDRL( addr2, 1, r2, regs, ACCTYPE_READ, regs->psw.pkey );
    s2pg = MAINSTOR_PAGEBASE ( s2 );

    /* Get mainstor address to Destination byte */
    d1 = MADDRL( addr1, 1, r1, regs, ACCTYPE_WRITE, regs->psw.pkey );
    d1pg = MAINSTOR_PAGEBASE ( d1 );

    /* Process operands from left to right */
    for (i = 0; len1 > 0 && len2 > 0; i++)
    {
        /* If 4096 characters have been converted, exit with cc=3 */
        if (i >= 4096)
        {
            cc = 3;
            break;
        }

        /* Fetch a UTF-8 character (1 to 4 bytes) */
        /* first character is always on page */
        utf[0] = *s2;
        //utf[0] = ARCH_DEP(vfetchb) ( addr2, r2, regs );

        /* Convert UTF-8 to Unicode */
        if (utf[0] < (BYTE)0x80)
        {
            /* Convert 00-7F to Unicode 0000-007F */
            n = 0;
            unicode1 = utf[0];
            pair = 0;
        }
        else if ((utf[0] & 0xE0) == 0xC0)
        {
#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
            /* WellFormdnessChecking */
            if (wfc)
            {
              if (utf[0] <= 0xc1)
              {
                regs->psw.cc = 2;
                return;
              }
            }
#endif
            /* Exit if fewer than 2 bytes remain in source operand */
            n = 1;
            if (len2 <= n) break;

            /* Fetch two UTF-8 bytes from source operand */
            //ARCH_DEP(vfetchc) ( utf, n, addr2, r2, regs );

            /* Get the next byte */
            // does the utf8 character cross a page boundary
            if (s2pg ==  MAINSTOR_PAGEBASE ( s2 + 1 ))
            {
                utf[1] = *(s2 + 1);
            }
            else
            {
                utf[1] = ARCH_DEP(vfetchb)(addr2 + 1, r2, regs);
            }

#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
            /* WellFormednessChecking */
            if (wfc)
            {
              if (utf[1] < 0x80 || utf[1] > 0xbf)
              {
                regs->psw.cc = 2;
                return;
              }
            }
#endif
            /* Convert C0xx-DFxx to Unicode */
            unicode1 = ((U16)(utf[0] & 0x1F) << 6)
                        | ((U16)(utf[1] & 0x3F));
            pair = 0;
        }
        else if ((utf[0] & 0xF0) == 0xE0)
        {
            /* Exit if fewer than 3 bytes remain in source operand */
            n = 2;
            if (len2 <= n) break;

            /* Fetch three UTF-8 bytes from source operand */
            //ARCH_DEP(vfetchc) ( utf, n, addr2, r2, regs );

            /* Get the next 2 bytes */
            // does the utf8 character cross a page boundary
            if (s2pg ==  MAINSTOR_PAGEBASE ( s2 + 2 ))
            {
                utf[1] = *(s2 + 1);
                utf[2] = *(s2 + 2);
            }
            else
            {
                ARCH_DEP(vfetchc)(&utf[1], 1, addr2 + 1, r2, regs);
            }

#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
            /* WellFormdnessChecking */
            if (wfc)
            {
              if (utf[0] == 0xe0)
              {
                if (utf[1] < 0xa0 || utf[1] > 0xbf || utf[2] < 0x80 || utf[2] > 0xbf)
                {
                  regs->psw.cc = 2;
                  return;
                }
              }
              if (0
                  || (utf[0] >= 0xe1 && utf[0] <= 0xec)
                  || (utf[0] >= 0xee && utf[0] <  0xef)
              )
              {
                if (0
                    || utf[1] < 0x80
                    || utf[1] > 0xbf

                    || utf[2] < 0x80
                    || utf[2] > 0xbf
                )
                {
                  regs->psw.cc = 2;
                  return;
                }
              }
              if (utf[0] == 0xed)
              {
                if (0
                    || utf[1] < 0x80
                    || utf[1] > 0x9f

                    || utf[2] < 0x80
                    || utf[2] > 0xbf
                )
                {
                  regs->psw.cc = 2;
                  return;
                }
              }
            }
#endif /* defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY ) */

            /* Convert E0xxxx-EFxxxx to Unicode */
            unicode1 = ((U16)(utf[0]) << 12)
                        | ((U16)(utf[1] & 0x3F) << 6)
                        | ((U16)(utf[2] & 0x3F));
            pair = 0;
        }
        else if ((utf[0] & 0xF8) == 0xF0)
        {
            /* Exit if fewer than 4 bytes remain in source operand */
            n = 3;
            if (len2 <= n) break;

            /* Fetch four UTF-8 bytes from source operand */
            //ARCH_DEP(vfetchc) ( utf, n, addr2, r2, regs );

            /* Get the next 3 bytes */
            // does the utf8 character cross a page boundary
            if (s2pg ==  MAINSTOR_PAGEBASE ( s2 + 3 ))
            {
                utf[1] = *(s2 + 1);
                utf[2] = *(s2 + 2);
                utf[3] = *(s2 + 3);
            }
            else
            {
                ARCH_DEP(vfetchc)(&utf[1], 2, addr2 + 1, r2, regs);
            }

#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
            /* WellFormdnessChecking */
            if (wfc)
            {
              if (utf[0] == 0xf0)
              {
                if (utf[1] < 0x90 || utf[1] > 0xbf || utf[2] < 0x80 || utf[2] > 0xbf || utf[3] < 0x80 || utf[3] > 0xbf)
                {
                  regs->psw.cc = 2;
                  return;
                }
              }
              if (utf[0] >= 0xf1 && utf[0] <= 0xf3)
              {
                if (utf[1] < 0x80 || utf[1] > 0xbf || utf[2] < 0x80 || utf[2] > 0xbf || utf[3] < 0x80 || utf[3] > 0xbf)
                {
                  regs->psw.cc = 2;
                  return;
                }
              }
              if (utf[0] == 0xf4)
              {
                if (utf[1] < 0x80 || utf[1] > 0x8f || utf[2] < 0x80 || utf[2] > 0xbf || utf[3] < 0x80 || utf[3] > 0xbf)
                {
                  regs->psw.cc = 2;
                  return;
                }
              }
            }
#endif /* defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY ) */

            /* Convert F0xxxxxx-F7xxxxxx to Unicode surrogate pair */
            uvwxy = ((((U16)(utf[0] & 0x07) << 2)
                        | ((U16)(utf[1] & 0x30) >> 4)) - 1) & 0x0F;
            unicode1 = 0xD800 | (uvwxy << 6) | ((U16)(utf[1] & 0x0F) << 2)
                        | ((U16)(utf[2] & 0x30) >> 4);
            unicode2 = 0xDC00 | ((U16)(utf[2] & 0x0F) << 6)
                        | ((U16)(utf[3] & 0x3F));
            pair = 1;
        }
        else
        {
            /* Invalid UTF-8 byte 80-BF or F8-FF, exit with cc=2 */
            cc = 2;
            break;
        }

        /* Store the result bytes in the destination operand */
        if (pair)
        {
            /* Exit if fewer than 4 bytes remain in destination */
            if (len1 < 4)
            {
                cc = 1;
                break;
            }

            /* Store Unicode surrogate pair in destination */
            // ARCH_DEP(vstore4) ( ((U32)unicode1 << 16) | (U32)unicode2,
            //             addr1, r1, regs );

            /* Write UTF16 pair */
            // does the utf16 pair character cross a page boundary
            if (d1pg ==  MAINSTOR_PAGEBASE ( d1 + 3 ))
            {
                /*make big endian*/
                U32 upair = CSWAP32 ( ((U32)unicode1 << 16) | (U32)unicode2 );

                *(d1 +0) = *( ((BYTE*) &upair)    );
                *(d1 +1) = *( ((BYTE*) &upair)  +1);
                *(d1 +2) = *( ((BYTE*) &upair)  +2);
                *(d1 +3) = *( ((BYTE*) &upair)  +3);
            }
            else
            {
                ARCH_DEP(vstore4) ( ((U32)unicode1 << 16) | (U32)unicode2,
                            addr1, r1, regs );
            }
            d1len = 4;

            addr1 += 4;
            addr1 &= ADDRESS_MAXWRAP(regs);
            len1 -= 4;
        }
        else
        {
            /* Exit if fewer than 2 bytes remain in destination */
            if (len1 < 2)
            {
                cc = 1;
                break;
            }

            /* Store Unicode character in destination */
            //ARCH_DEP(vstore2) ( unicode1, addr1, r1, regs );

            /* Write UTF16 character */
            // does the utf16 pair character cross a page boundary
            if (d1pg ==  MAINSTOR_PAGEBASE ( d1 + 1 ))
            {
                U16 uchar = CSWAP16 ( unicode1 );      /*make big endian*/

                *(d1 +0) = *( ((BYTE*) &uchar)   );
                *(d1 +1) = *( ((BYTE*) &uchar) +1);
            }
            else
            {
                ARCH_DEP(vstore2) ( unicode1, addr1, r1, regs );
            }
            d1len = 2;

            addr1 += 2;
            addr1 &= ADDRESS_MAXWRAP(regs);
            len1 -= 2;
        }

        /* Update operand 2 address and length */
        addr2 += n + 1;
        addr2 &= ADDRESS_MAXWRAP(regs);
        len2 -= n + 1;

        /* Update the registers */
        SET_GR_A(r1, regs,addr1);
        SET_GR_A(r1+1, regs,len1);
        SET_GR_A(r2, regs,addr2);
        SET_GR_A(r2+1, regs,len2);

        // Update mainstor addresses; source and destination
        s2 += n + 1;
        if (s2pg !=  MAINSTOR_PAGEBASE ( s2 ))
        {
            s2 = MADDRL( addr2, 1, r2, regs, ACCTYPE_READ, regs->psw.pkey );
            s2pg = MAINSTOR_PAGEBASE ( s2 );
        }

        d1 += d1len;
        if (d1pg !=  MAINSTOR_PAGEBASE ( d1 ))
        {
            d1 = MADDRL( addr1, 1, r1, regs, ACCTYPE_WRITE, regs->psw.pkey );
            d1pg = MAINSTOR_PAGEBASE ( d1 );
        }

        if (len1 == 0 && len2 != 0)
            cc = 1;

    } /* end for(i) */

    regs->psw.cc = cc;

} /* end convert_utf8_to_utf16 */
#endif /*FEATURE_EXTENDED_TRANSLATION_FACILITY_1*/


/*-------------------------------------------------------------------*/
/* 4F   CVB   - Convert to Binary                             [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_to_binary)
{
U64     dreg;                           /* 64-bit result accumulator */
int     r1;                             /* Value of R1 field         */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
int     ovf;                            /* 1=overflow                */
int     dxf;                            /* 1=data exception          */
BYTE    dec[8];                         /* Packed decimal operand    */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXFC_INSTR_CHECK( regs );

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

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));

} /* end DEF_INST(convert_to_binary) */


/*-------------------------------------------------------------------*/
/* 4E   CVD   - Convert to Decimal                            [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_to_decimal)
{
S64     bin;                            /* 64-bit signed binary value*/
int     r1;                             /* Value of R1 field         */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
BYTE    dec[16];                        /* Packed decimal result     */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXFC_INSTR_CHECK( regs );

    /* Load value of register and sign-extend to 64 bits */
    bin = (S64)((S32)(regs->GR_L(r1)));

    /* Convert to 16-byte packed decimal number */
    binary_to_packed (bin, dec);

    /* Store low 8 bytes of result at operand address */
    ARCH_DEP(vstorec) ( dec+8, 8-1, effective_addr2, b2, regs );

} /* end DEF_INST(convert_to_decimal) */


#if defined( FEATURE_ACCESS_REGISTERS )
/*-------------------------------------------------------------------*/
/* B24D CPYA  - Copy Access                                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(copy_access)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    TXF_ACCESS_INSTR_CHECK( regs );

    /* Copy R2 access register to R1 access register */
    regs->AR(r1) = regs->AR(r2);
    SET_AEA_AR(regs, r1);
}
#endif /* defined( FEATURE_ACCESS_REGISTERS ) */


/*-------------------------------------------------------------------*/
/* 1D   DR    - Divide Register                                 [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(divide_register)
{
int     r1;                             /* Values of R fields        */
int     r2;                             /* Values of R fields        */
int     divide_overflow;                /* 1=divide overflow         */

    RR(inst, regs, r1, r2);

    TXFC_INSTR_CHECK( regs );
    ODD_CHECK(r1, regs);

    /* Divide r1::r1+1 by r2, remainder in r1, quotient in r1+1 */
    divide_overflow =
        div_signed (&(regs->GR_L(r1)),&(regs->GR_L(r1+1)),
                    regs->GR_L(r1),
                    regs->GR_L(r1+1),
                    regs->GR_L(r2));

    /* Program check if overflow */
    if ( divide_overflow )
        regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK2( r1, r1+1 ));
}


/*-------------------------------------------------------------------*/
/* 5D   D     - Divide                                        [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(divide)
{
int     r1;                             /* Values of R fields        */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */
int     divide_overflow;                /* 1=divide overflow         */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    TXFC_INSTR_CHECK( regs );
    ODD_CHECK(r1, regs);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Divide r1::r1+1 by n, remainder in r1, quotient in r1+1 */
    divide_overflow =
        div_signed (&(regs->GR_L(r1)), &(regs->GR_L(r1+1)),
                    regs->GR_L(r1),
                    regs->GR_L(r1+1),
                    n);

    /* Program check if overflow */
    if ( divide_overflow )
        regs->program_interrupt (regs, PGM_FIXED_POINT_DIVIDE_EXCEPTION);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK2( r1, r1+1 ));
}


/*-------------------------------------------------------------------*/
/* 17   XR    - Exclusive Or Register                           [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(exclusive_or_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* XOR second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_L(r1) ^= regs->GR_L(r2) ) ? 1 : 0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 57   X     - Exclusive Or                                  [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(exclusive_or)
{
int     r1;                             /* Values of R fields        */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* XOR second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_L(r1) ^= n ) ? 1 : 0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 97   XI    - Exclusive Or Immediate                          [SI] */
/*-------------------------------------------------------------------*/
DEF_INST(exclusive_or_immediate)
{
BYTE    i2;                             /* Immediate operand         */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
BYTE   *dest;                           /* Pointer to target byte    */

    SI(inst, regs, i2, b1, effective_addr1);
    PER_ZEROADDR_XCHECK( regs, b1 );

    ITIMER_SYNC(effective_addr1, 0, regs);

    /* Get byte mainstor address */
    dest = MADDR (effective_addr1, b1, regs, ACCTYPE_WRITE, regs->psw.pkey );

    /* MAINLOCK may be required if cmpxchg assists unavailable */
    OBTAIN_MAINLOCK( regs );
    {
        /* XOR byte with immediate operand, setting condition code */
        regs->psw.cc = (H_ATOMIC_OP( dest, i2, xor, Xor, ^ ) != 0);
    }
    RELEASE_MAINLOCK( regs );

    ITIMER_UPDATE(effective_addr1, 0, regs);
}


/*-------------------------------------------------------------------*/
/* D7   XC    - Exclusive Or Character                        [SS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(exclusive_or_character)
{
int     len, len2, len3;                /* Lengths to copy           */
int     b1, b2;                         /* Base register numbers     */
VADR    effective_addr1;                /* Virtual address           */
VADR    effective_addr2;                /* Virtual address           */
BYTE   *dest1, *dest2;                  /* Destination addresses     */
BYTE   *source1, *source2;              /* Source addresses          */
BYTE   *sk1, *sk2;                      /* Storage key addresses     */
int     i;                              /* Loop counter              */
int     cc = 0;                         /* Condition code            */

    SS_L( inst, regs, len, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    TXFC_INSTR_CHECK( regs );

    ITIMER_SYNC( effective_addr1, len, regs );
    ITIMER_SYNC( effective_addr2, len, regs );

    /* Quick out for 1 byte (no boundary crossed) */
    if (unlikely( !len ))
    {
        source1 = MADDR( effective_addr2, b2, regs, ACCTYPE_READ,  regs->psw.pkey );
        dest1   = MADDR( effective_addr1, b1, regs, ACCTYPE_WRITE, regs->psw.pkey );
        if (*dest1 ^= *source1)
            cc = 1;
        regs->psw.cc = cc;
        return;
    }

    /* There are several scenarios (in optimal order):
     * (1) dest boundary and source boundary not crossed
     *     (a) dest and source are the same, set dest to zeroes
     *     (b) dest and source are not the same
     * (2) dest boundary not crossed and source boundary crossed
     * (3) dest boundary crossed and source boundary not crossed
     * (4) dest boundary and source boundary are crossed
     *     (a) dest and source boundary cross at the same time
     *     (b) dest boundary crossed first
     *     (c) source boundary crossed first
     */

    /* Translate addresses of leftmost operand bytes */
    dest1 = MADDRL( effective_addr1, len + 1, b1, regs, ACCTYPE_WRITE_SKP, regs->psw.pkey );
    sk1 = regs->dat.storkey;
    source1 = MADDRL(effective_addr2, len + 1, b2, regs, ACCTYPE_READ, regs->psw.pkey );

    if (NOCROSSPAGE( effective_addr1, len ))
    {
        if (NOCROSSPAGE( effective_addr2, len ))
        {
            /* (1) - No boundaries are crossed */
            if (dest1 == source1)
            {
               /* (1a) - Dest and source are the same */
               memset( dest1, 0, len + 1 );
            }
            else
            {
               /* (1b) - Dest and source are not the sam */
                for (i=0; i <= len; i++)
                    if (*dest1++ ^= *source1++)
                        cc = 1;
            }
        }
        else
        {
             /* (2) - Second operand crosses a boundary */
             len2 = PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK);
             source2 = MADDRL((effective_addr2 + len2) & ADDRESS_MAXWRAP( regs ),
               len + 1 - len2, b2, regs, ACCTYPE_READ, regs->psw.pkey );
             for (i=0; i < len2; i++)
                 if (*dest1++ ^= *source1++)
                     cc = 1;

             len2 = len - len2;

             for (i=0; i <= len2; i++)
                 if (*dest1++ ^= *source2++)
                     cc = 1;
        }
        ARCH_DEP( or_storage_key_by_ptr )( sk1, (STORKEY_REF | STORKEY_CHANGE) );
    }
    else
    {
        /* First operand crosses a boundary */
        len2 = PAGEFRAME_PAGESIZE - (effective_addr1 & PAGEFRAME_BYTEMASK);
        dest2 = MADDRL((effective_addr1 + len2) & ADDRESS_MAXWRAP( regs ),
         len + 1 - len2, b1, regs, ACCTYPE_WRITE_SKP, regs->psw.pkey );
        sk2 = regs->dat.storkey;

        if (NOCROSSPAGE( effective_addr2, len ))
        {
             /* (3) - First operand crosses a boundary */
             for (i=0; i < len2; i++)
                 if (*dest1++ ^= *source1++)
                     cc = 1;

             len2 = len - len2;

             for (i=0; i <= len2; i++)
                 if (*dest2++ ^= *source1++)
                     cc = 1;
        }
        else
        {
            /* (4) - Both operands cross a boundary */
            len3 = PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK);
            source2 = MADDRL((effective_addr2 + len3) & ADDRESS_MAXWRAP( regs ),
              len + 1 - len3, b2, regs, ACCTYPE_READ, regs->psw.pkey );
            if (len2 == len3)
            {
                /* (4a) - Both operands cross at the same time */
                for (i=0; i < len2; i++)
                    if (*dest1++ ^= *source1++)
                        cc = 1;

                len2 = len - len2;

                for (i=0; i <= len2; i++)
                    if (*dest2++ ^= *source2++)
                        cc = 1;
            }
            else if (len2 < len3)
            {
                /* (4b) - First operand crosses first */
                for (i=0; i < len2; i++)
                    if (*dest1++ ^= *source1++)
                        cc = 1;

                len2 = len3 - len2;

                for (i=0; i < len2; i++)
                    if (*dest2++ ^= *source1++)
                        cc = 1;

                len2 = len - len3;

                for (i=0; i <= len2; i++)
                    if (*dest2++ ^= *source2++)
                        cc = 1;
            }
            else
            {
                /* (4c) - Second operand crosses first */
                for (i=0; i < len3; i++)
                    if (*dest1++ ^= *source1++)
                        cc = 1;

                len3 = len2 - len3;

                for (i=0; i < len3; i++)
                    if (*dest1++ ^= *source2++)
                        cc = 1;

                len3 = len - len2;

                for (i=0; i <= len3; i++)
                    if (*dest2++ ^= *source2++)
                        cc = 1;
            }
        }
        ARCH_DEP( or_storage_key_by_ptr )( sk1, (STORKEY_REF | STORKEY_CHANGE) );
        ARCH_DEP( or_storage_key_by_ptr )( sk2, (STORKEY_REF | STORKEY_CHANGE) );
    }

    regs->psw.cc = cc;

    ITIMER_UPDATE(effective_addr1,len,regs);

}


/*-------------------------------------------------------------------*/
/* 44   EX    - Execute                                       [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(execute)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
BYTE   *ip;                             /* -> executed instruction   */

    RX(inst, regs, r1, x2, b2, regs->ET);

    TXFC_INSTR_CHECK( regs );
    ODD_CHECK(regs->ET, regs);

#if defined( _FEATURE_SIE )
    /* Ensure that the instruction field is zero, such that
       zeros are stored in the interception parm field, if
       the interrupt is intercepted */
    memset(regs->exinst, 0, 8);
#endif

    /* Fetch target instruction from operand address */
    ip = INSTRUCTION_FETCH(regs, 1);
    if (ip != regs->exinst)
        memcpy (regs->exinst, ip, 8);

    /* Or 2nd byte of instruction with low-order byte of R1 */
    regs->exinst[1] |= r1 ? regs->GR_LHLCL(r1) : 0;

    /* Program check if recursive execute */
    if (0
        || regs->exinst[0] == 0x44
#if defined( FEATURE_035_EXECUTE_EXTN_FACILITY )
        || (1
            && FACILITY_ENABLED( 035_EXECUTE_EXTN, regs )
            &&  (regs->exinst[0] == 0xc6)
            && !(regs->exinst[1] &  0x0f)
           )
#endif
    )
        regs->program_interrupt( regs, PGM_EXECUTE_EXCEPTION );

    /* Save the address of the instruction in case the instruction
       being executed causes a break in sequential instruction flow.
       Note: we MUST do this BEFORE setting the regs->execflag flag.
    */
    SET_BEAR_EX_REG( regs, regs->ip - 4 );

    /*
     * Turn execflag on indicating this instruction is EXecuted.
     * psw.ip is backed up by the EXecuted instruction length to
     * be incremented back by the instruction decoder.
     */
    regs->execflag = 1;
    regs->exrl = 0;
    regs->ip -= ILC(regs->exinst[0]);

    EXECUTE_INSTRUCTION(regs->ARCH_DEP(runtime_opcode_xxxx), regs->exinst, regs);
    regs->instcount++;
    UPDATE_SYSBLK_INSTCOUNT( 1 );

    /* Leave execflag on if pending PER so ILC will reflect EX */
    if (!OPEN_IC_PER(regs))
        regs->execflag = 0;
}


#if defined( FEATURE_035_EXECUTE_EXTN_FACILITY )
/*-------------------------------------------------------------------*/
/* C6_0 EXRL  - Execute Relative Long                        [RIL-b] */
/*-------------------------------------------------------------------*/
DEF_INST(execute_relative_long)
{
    int    r1;                          /* Register number           */
    BYTE*  ip;                          /* -> executed instruction   */

    RIL_A( inst, regs, r1, regs->ET );

    TXFC_INSTR_CHECK( regs );

#if defined( _FEATURE_SIE )
    /* Ensure that the instruction field is zero, such that
       zeros are stored in the interception parm field, if
       the interrupt is intercepted.
    */
    memset( regs->exinst, 0, 8 );
#endif

    /* Fetch target instruction from operand address */
    ip = INSTRUCTION_FETCH( regs, 1 );

    if (ip != regs->exinst)
        memcpy( regs->exinst, ip, 8 );

#if 0 // debugging
    /* Display target instruction if stepping or tracing */
    if (CPU_STEPPING_OR_TRACING( regs, 6 ))
    {
        int ilc;
        char buf[128];
        char buf2[128];
        char work[8];

        ilc = ILC( ip[0] );

      #if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        MSGBUF( buf, "EXRL target  ADDR="F_VADR"     INST=", regs->ET );
      #else
        MSGBUF( buf, "EXRL  ADDR="F_VADR"   INST=",          regs->ET );
      #endif

//      if (ilc > 0)
        {
            MSGBUF( work, "%2.2X%2.2X", ip[0], ip[1] );
            STRLCAT( buf, work );
        }
        if (ilc > 2)
        {
            MSGBUF( work, "%2.2X%2.2X", ip[2], ip[3] );
            STRLCAT( buf, work );
        }
        if (ilc > 4)
        {
            MSGBUF( work, "%2.2X%2.2X", ip[4], ip[5] );
            STRLCAT( buf, work );
        }

        STRLCAT( buf, " " );

        if (ilc < 6) STRLCAT( buf, "    " );
        if (ilc < 4) STRLCAT( buf, "    " );

        PRINT_INST( regs->arch_mode, ip, buf2 );

        LOGMSG( "%s%s\n", buf, buf2 );
    }
#endif

    /* Or 2nd byte of instruction with low-order byte of R1 */
    regs->exinst[1] |= r1 ? regs->GR_LHLCL( r1 ) : 0;

    /* Program check if recursive execute */
    if (regs->exinst[0] == 0x44 ||
       (regs->exinst[0] == 0xc6 && !(regs->exinst[1] & 0x0f)))
        regs->program_interrupt( regs, PGM_EXECUTE_EXCEPTION );

    /* Save the address of the instruction in case the instruction
       being executed causes a break in sequential instruction flow.
       Note: we MUST do this BEFORE setting the regs->execflag flag.
    */
    SET_BEAR_EX_REG( regs, regs->ip - 6 );

    /*
     * Turn execflag on indicating this instruction is EXecuted.
     * psw.ip is backed up by the EXecuted instruction length to
     * be incremented back by the instruction decoder.
     */
    regs->execflag = 1;
    regs->exrl     = 1;
    regs->ip      -= ILC( regs->exinst[0] );

    EXECUTE_INSTRUCTION( regs->ARCH_DEP( runtime_opcode_xxxx ), regs->exinst, regs );
    regs->instcount++;
    UPDATE_SYSBLK_INSTCOUNT( 1 );

    /* Leave execflag on if pending PER so ILC will reflect EXRL */
    if (!OPEN_IC_PER( regs ))
        regs->execflag = 0;
}
#endif /* defined( FEATURE_EXECUTE_EXTENSION_FACILITY ) */


#if defined( FEATURE_ACCESS_REGISTERS )
/*-------------------------------------------------------------------*/
/* B24F EAR   - Extract Access Register                        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(extract_access_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    /* Copy R2 access register to R1 general register */
    regs->GR_L(r1) = regs->AR(r2);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}
#endif /* defined( FEATURE_ACCESS_REGISTERS ) */


#ifdef OPTION_OPTINST
/*-------------------------------------------------------------------*/
/* BF_7   ICM   - Insert Characters under Mask (optimized)    [RS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(BF_7)
{
int    r1;                              /* Register numbers          */
int    b2;                              /* effective address base    */
VADR   effective_addr2;                 /* effective address         */
BYTE   vbyte[4];                        /* Fetched storage bytes     */
U32    n;                               /* Fetched value             */

    RSMX(inst, regs, r1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    /* Optimized case */
    vbyte[0] = 0;
    ARCH_DEP(vfetchc) (vbyte + 1, 2, effective_addr2, b2, regs);
    n = fetch_fw (vbyte);
    regs->GR_L(r1) = (regs->GR_L(r1) & 0xFF000000) | n;
    regs->psw.cc = n ? n & 0x00800000 ? 1 : 2 : 0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}

/*-------------------------------------------------------------------*/
/* BF_F   ICM   - Insert Characters under Mask (optimized)    [RS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(BF_F)
{
int    r1;                              /* Register numbers          */
int    b2;                              /* effective address base    */
VADR   effective_addr2;                 /* effective address         */

    RSMX(inst, regs, r1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    /* Optimized case */
    regs->GR_L(r1) = ARCH_DEP(vfetch4) (effective_addr2, b2, regs);
    regs->psw.cc = regs->GR_L(r1) ? regs->GR_L(r1) & 0x80000000 ? 1 : 2 : 0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}

/*-------------------------------------------------------------------*/
/* BF_x   ICM   - Insert Characters under Mask (optimized)    [RS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(BF_x)
{
int    r1, r3;                          /* Register numbers          */
int    b2;                              /* effective address base    */
VADR   effective_addr2;                 /* effective address         */
int    i;                               /* Integer work area         */
BYTE   vbyte[4];                        /* Fetched storage bytes     */
U32    n;                               /* Fetched value             */
static const int                        /* Length-1 to fetch by mask */
       icmlen[16] = {0, 0, 0, 1, 0, 1, 1, 2, 0, 1, 1, 2, 1, 2, 2, 3};
static const unsigned int               /* Turn reg bytes off by mask*/
       icmmask[16] = {0xFFFFFFFF, 0xFFFFFF00, 0xFFFF00FF, 0xFFFF0000,
                      0xFF00FFFF, 0xFF00FF00, 0xFF0000FF, 0xFF000000,
                      0x00FFFFFF, 0x00FFFF00, 0x00FF00FF, 0x00FF0000,
                      0x0000FFFF, 0x0000FF00, 0x000000FF, 0x00000000};

    RS(inst, regs, r1, r3, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    memset (vbyte, 0, 4);
    ARCH_DEP(vfetchc)(vbyte, icmlen[r3], effective_addr2, b2, regs);

    /* If mask was 0 then we still had to fetch, according to POP.
       If so, set the fetched byte to 0 to force zero cc */
    if (!r3) vbyte[0] = 0;

    n = fetch_fw (vbyte);
    regs->psw.cc = n ? n & 0x80000000 ? 1 : 2 : 0;

    /* Turn off the reg bytes we are going to set */
    regs->GR_L(r1) &= icmmask[r3];

    /* Set bytes one at a time according to the mask */
    i = 0;
    if (r3 & 0x8) regs->GR_L(r1) |= vbyte[i++] << 24;
    if (r3 & 0x4) regs->GR_L(r1) |= vbyte[i++] << 16;
    if (r3 & 0x2) regs->GR_L(r1) |= vbyte[i++] << 8;
    if (r3 & 0x1) regs->GR_L(r1) |= vbyte[i];

#if defined( FEATURE_PER1 )
    /* Check for PER 1 GRA event */
    if (r3) // non-zero mask?
        PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
#endif
}
#endif /* #ifdef OPTION_OPTINST */


/*-------------------------------------------------------------------*/
/* BF   ICM   - Insert Characters under Mask                  [RS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_characters_under_mask)
{
int    r1, r3;                          /* Register numbers          */
int    b2;                              /* effective address base    */
VADR   effective_addr2;                 /* effective address         */
int    i;                               /* Integer work area         */
BYTE   vbyte[4];                        /* Fetched storage bytes     */
U32    n;                               /* Fetched value             */
static const int                        /* Length-1 to fetch by mask */
       icmlen[16] = {0, 0, 0, 1, 0, 1, 1, 2, 0, 1, 1, 2, 1, 2, 2, 3};
static const unsigned int               /* Turn reg bytes off by mask*/
       icmmask[16] = {0xFFFFFFFF, 0xFFFFFF00, 0xFFFF00FF, 0xFFFF0000,
                      0xFF00FFFF, 0xFF00FF00, 0xFF0000FF, 0xFF000000,
                      0x00FFFFFF, 0x00FFFF00, 0x00FF00FF, 0x00FF0000,
                      0x0000FFFF, 0x0000FF00, 0x000000FF, 0x00000000};

    RS(inst, regs, r1, r3, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    switch (r3) {

    case 7:
        /* Optimized case */
        vbyte[0] = 0;
        ARCH_DEP(vfetchc) (vbyte + 1, 2, effective_addr2, b2, regs);
        n = fetch_fw (vbyte);
        regs->GR_L(r1) = (regs->GR_L(r1) & 0xFF000000) | n;
        regs->psw.cc = n ? n & 0x00800000 ?
                       1 : 2 : 0;
        break;

    case 15:
        /* Optimized case */
        regs->GR_L(r1) = ARCH_DEP(vfetch4) (effective_addr2, b2, regs);
        regs->psw.cc = regs->GR_L(r1) ? regs->GR_L(r1) & 0x80000000 ?
                       1 : 2 : 0;
        break;

    default:
        memset(vbyte, 0, 4);
        ARCH_DEP(vfetchc)(vbyte, icmlen[r3], effective_addr2, b2, regs);

        /* If mask was 0 then we still had to fetch, according to POP.
           If so, set the fetched byte to 0 to force zero cc */
        if (!r3) vbyte[0] = 0;

        n = fetch_fw (vbyte);
        regs->psw.cc = n ? n & 0x80000000 ?
                       1 : 2 : 0;

        /* Turn off the reg bytes we are going to set */
        regs->GR_L(r1) &= icmmask[r3];

        /* Set bytes one at a time according to the mask */
        i = 0;
        if (r3 & 0x8) regs->GR_L(r1) |= vbyte[i++] << 24;
        if (r3 & 0x4) regs->GR_L(r1) |= vbyte[i++] << 16;
        if (r3 & 0x2) regs->GR_L(r1) |= vbyte[i++] << 8;
        if (r3 & 0x1) regs->GR_L(r1) |= vbyte[i];
        break;

    } /* switch (r3) */

#if defined( FEATURE_PER1 )
    /* Check for PER 1 GRA event */
    if (r3) // non-zero mask?
        PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
#endif
}


/*-------------------------------------------------------------------*/
/* B222 IPM   - Insert Program Mask                            [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(insert_program_mask)
{
int     r1, r2;                         /* Value of R field          */

    RRE(inst, regs, r1, r2);

    /* Insert condition code in R1 bits 2-3, program mask
       in R1 bits 4-7, and set R1 bits 0-1 to zero */
    regs->GR_LHHCH(r1) = (regs->psw.cc << 4) | regs->psw.progmask;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


#ifdef OPTION_OPTINST
/*-------------------------------------------------------------------*/
/* 18   LR    - Load Register (optimized)                       [RR] */
/*-------------------------------------------------------------------*/
#define LRgen( r1, r2 )                                               \
                                                                      \
  DEF_INST( 18 ## r1 ## r2 )                                          \
  {                                                                   \
    UNREFERENCED( inst );                                             \
                                                                      \
    INST_UPDATE_PSW( regs, 2, 2 );                                    \
                                                                      \
    regs->GR_L( 0x ## r1 ) = regs->GR_L( 0x ## r2 );                  \
                                                                      \
    /* Check for PER 1 GRA event */                                   \
    PER_GRA_CHECK( regs, PER_GRA_MASK( 0x ## r1 ));                   \
  }

#define LRgenr2( r1 )                                                 \
                                                                      \
  LRgen( r1, 0 )                                                      \
  LRgen( r1, 1 )                                                      \
  LRgen( r1, 2 )                                                      \
  LRgen( r1, 3 )                                                      \
  LRgen( r1, 4 )                                                      \
  LRgen( r1, 5 )                                                      \
  LRgen( r1, 6 )                                                      \
  LRgen( r1, 7 )                                                      \
  LRgen( r1, 8 )                                                      \
  LRgen( r1, 9 )                                                      \
  LRgen( r1, A )                                                      \
  LRgen( r1, B )                                                      \
  LRgen( r1, C )                                                      \
  LRgen( r1, D )                                                      \
  LRgen( r1, E )                                                      \
  LRgen( r1, F )

LRgenr2( 0 )
LRgenr2( 1 )
LRgenr2( 2 )
LRgenr2( 3 )
LRgenr2( 4 )
LRgenr2( 5 )
LRgenr2( 6 )
LRgenr2( 7 )
LRgenr2( 8 )
LRgenr2( 9 )
LRgenr2( A )
LRgenr2( B )
LRgenr2( C )
LRgenr2( D )
LRgenr2( E )
LRgenr2( F )

#endif /* #ifdef OPTION_OPTINST */


/*-------------------------------------------------------------------*/
/* 18   LR    - Load Register                                   [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* Copy second operand to first operand */
    regs->GR_L(r1) = regs->GR_L(r2);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


#if defined( FEATURE_ACCESS_REGISTERS )
/*-------------------------------------------------------------------*/
/* 9A   LAM   - Load Access Multiple                          [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(load_access_multiple)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i, m, n;                        /* Integer work areas        */
U32    *p1, *p2 = NULL;                 /* Mainstor pointers         */

    RS( inst, regs, r1, r3, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_ACCESS_INSTR_CHECK( regs );
    FW_CHECK(effective_addr2, regs);

    /* Calculate number of regs to fetch */
    n = ((r3 - r1) & 0xF) + 1;

    /* Calculate number of words to next boundary */
    m = (PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK)) >> 2;

    /* Address of operand beginning */
    p1 = (U32*) MADDRL(effective_addr2, n << 2, b2, regs, ACCTYPE_READ, regs->psw.pkey );

    /* Get address of next page if boundary crossed */
    if (unlikely( m < n ))
        p2 = (U32*) MADDRL( effective_addr2 + (m*4), (n - m) << 2, b2, regs, ACCTYPE_READ, regs->psw.pkey );
    else
        m = n;

    /* Copy from operand beginning */
    for (i=0; i < m; i++, p1++)
    {
        regs->AR( (r1 + i) & 0xF ) = fetch_fw( p1 );
        SET_AEA_AR( regs, (r1 + i) & 0xF );
    }

    /* Copy from next page */
    for (; i < n; i++, p2++)
    {
        regs->AR( (r1 + i) & 0xF ) = fetch_fw( p2 );
        SET_AEA_AR( regs, (r1 + i) & 0xF );
    }
}
#endif /* defined( FEATURE_ACCESS_REGISTERS ) */


#if defined( FEATURE_ACCESS_REGISTERS )
/*-------------------------------------------------------------------*/
/* 51   LAE   - Load Address Extended                         [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(load_address_extended)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RX(inst, regs, r1, x2, b2, effective_addr2);

    TXFC_INSTR_CHECK( regs );

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

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}
#endif /* defined( FEATURE_ACCESS_REGISTERS ) */


/*-------------------------------------------------------------------*/
/* 12   LTR   - Load and Test Register                          [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_and_test_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* Copy second operand and set condition code */
    regs->GR_L(r1) = regs->GR_L(r2);

    regs->psw.cc = (S32)regs->GR_L(r1) < 0 ? 1 :
                   (S32)regs->GR_L(r1) > 0 ? 2 : 0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 13   LCR   - Load Complement Register                        [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_complement_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* Condition code 3 and program check if overflow */
    if ( regs->GR_L(r2) == 0x80000000 )
    {
        regs->GR_L(r1) = regs->GR_L(r2);
        regs->psw.cc = 3;
        if ( FOMASK(&regs->psw) )
            regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);
        return;
    }

    /* Load complement of second operand and set condition code */
    regs->GR_L(r1) = -((S32)regs->GR_L(r2));

    regs->psw.cc = (S32)regs->GR_L(r1) < 0 ? 1 :
                   (S32)regs->GR_L(r1) > 0 ? 2 : 0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


#if defined( FEATURE_IMMEDIATE_AND_RELATIVE )
/*-------------------------------------------------------------------*/
/* A7x8 LHI   - Load Halfword Immediate                       [RI-a] */
/*-------------------------------------------------------------------*/
DEF_INST(load_halfword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */

    RI(inst, regs, r1, opcd, i2);

    /* Load operand into register */
    regs->GR_L(r1) = (S16)i2;

}
#endif /* defined( FEATURE_IMMEDIATE_AND_RELATIVE ) */


/*-------------------------------------------------------------------*/
/* 98   LM    - Load Multiple                                 [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(load_multiple)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i, m, n;                        /* Integer work areas        */
U32    *p1, *p2;                        /* Mainstor pointers         */
BYTE   *bp1;                            /* Unaligned maintstor ptr   */

    RS( inst, regs, r1, r3, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    /* Calculate number of bytes to load */
    n = (((r3 - r1) & 0xF) + 1) << 2;

    /* Calculate number of bytes to next boundary */
    m = PAGEFRAME_PAGESIZE - ((VADR_L)effective_addr2 & PAGEFRAME_BYTEMASK);

    /* Address of operand beginning */
    bp1 = (BYTE*) MADDRL(effective_addr2, n, b2, regs, ACCTYPE_READ, regs->psw.pkey );
    p1 = (U32*) bp1;

    if (likely( n <= m ))
    {
        /* Boundary not crossed */
        n >>= 2;
        if (likely(!(((uintptr_t)effective_addr2) & 0x03)))
        {
            for (i=0; i < n; i++, p1++)
                regs->GR_L( (r1 + i) & 0xF ) = fetch_fw( p1 );
        }
        else
        {
            for (i=0; i < n; i++, bp1+=4)
                regs->GR_L( (r1 + i) & 0xF ) = fetch_fw( bp1 );
        }
    }
    else
    {
        /* Boundary crossed, get 2nd page address */
        effective_addr2 += m;
        effective_addr2 &= ADDRESS_MAXWRAP( regs );
        p2 = (U32*) MADDRL(effective_addr2, n - m, b2, regs, ACCTYPE_READ, regs->psw.pkey );

        if (likely( (m & 0x3) == 0 ))
        {
            /* Addresses are word aligned */
            m >>= 2;
            for (i=0; i < m; i++, p1++)
                regs->GR_L( (r1 + i) & 0xF ) = fetch_fw( p1 );
            n >>= 2;
            for (; i < n; i++, p2++)
                regs->GR_L( (r1 + i) & 0xF ) = fetch_fw( p2 );
        }
        else
        {
            /* Worst case */
            U32 rwork[16];
            BYTE *b1, *b2;

            b1 = (BYTE*) &rwork[0];
            b2 = (BYTE*) p1;

            for (i=0; i < m; i++)
                *b1++ = *b2++;

            b2 = (BYTE*) p2;

            for (; i < n; i++)
                *b1++ = *b2++;

            n >>= 2;

            for (i=0; i < n; i++)
                regs->GR_L( (r1 + i) & 0xF ) = CSWAP32( rwork[i] );
        }
    }

#if defined( FEATURE_PER1 )
    if (EN_IC_PER_GRA( regs ))
    {
        /* Check for PER 1 GRA event */
        U16 rmask = 0x0000;
        if (r1 > r3)
        {
            for (i = r1; i <= 15; ++i)
                rmask |= PER_GRA_MASK( i );
            for (i = 0; i <= r3; ++i)
                rmask |= PER_GRA_MASK( i );
        }
        else // (r1 <= r3)
            for (i = r1; i <= r3; ++i)
                rmask |= PER_GRA_MASK( i );
        PER_GRA_CHECK( regs, rmask );
    }
#endif

} /* end DEF_INST(load_multiple) */


/*-------------------------------------------------------------------*/
/* 11   LNR   - Load Negative Register                          [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_negative_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* Load negative value of second operand and set cc */
    regs->GR_L(r1) = (S32)regs->GR_L(r2) > 0 ?
                            -((S32)regs->GR_L(r2)) :
                            (S32)regs->GR_L(r2);

    regs->psw.cc = (S32)regs->GR_L(r1) == 0 ? 0 : 1;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 10   LPR   - Load Positive Register                          [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(load_positive_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* Condition code 3 and program check if overflow */
    if ( regs->GR_L(r2) == 0x80000000 )
    {
        regs->GR_L(r1) = regs->GR_L(r2);
        regs->psw.cc = 3;
        if ( FOMASK(&regs->psw) )
            regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);
        return;
    }

    /* Load positive value of second operand and set cc */
    regs->GR_L(r1) = (S32)regs->GR_L(r2) < 0 ?
                            -((S32)regs->GR_L(r2)) :
                            (S32)regs->GR_L(r2);

    regs->psw.cc = (S32)regs->GR_L(r1) == 0 ? 0 : 2;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* AF   MC    - Monitor Call                                    [SI] */
/*-------------------------------------------------------------------*/
DEF_INST(monitor_call)
{
BYTE    i2;                             /* Monitor class             */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
CREG    n;                              /* Work                      */

    SI(inst, regs, i2, b1, effective_addr1);

    /* Program check if monitor class exceeds 15 */
    if ( i2 > 0x0F )
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Ignore if monitor mask in control register 8 is zero */
    n = (regs->CR(8) & CR8_MCMASK) << i2;
    if ((n & 0x00008000) == 0)
        return;

    /* The Monitor Call instruction is restricted in transaction
       execution mode when a monitor-event conditon recognized */
    TXF_INSTR_CHECK( regs );

#if defined( FEATURE_036_ENH_MONITOR_FACILITY )
    /* Perform Monitor Event Counting Operation if enabled */
    if (FACILITY_ENABLED( 036_ENH_MONITOR, regs )
      && (( (regs->CR_H(8) & CR8_MCMASK) << i2) & 0x00008000))
    {
        PSA *psa;                       /* -> Prefixed storage area  */
        RADR cao;           /* Enhanced Monitor Counter Array Origin */
        U32  cal;           /* Enhanced Monitor Counter Array Length */
        U32  ec;         /* Enhanced Montior Counter Exception Count */
        RADR ceh;                        /* HW Counter Entry address */
        RADR cew;                        /* FW Counter Entry address */
        RADR px;
        int  unavailable;
        U16  hwc;
        U32  fwc;

        px = regs->PX;
        SIE_TRANSLATE(&px, ACCTYPE_WRITE, regs);

        /* Point to PSA in main storage */
        psa = (void*)(regs->mainstor + px);

        /* Set the main storage reference bit */
        ARCH_DEP( or_storage_key )( px, STORKEY_REF );

        /* Fetch Counter Array Origin and Size from PSA */
        FETCH_DW(cao, psa->cao);
        FETCH_W(cal, psa->cal);

        /* DW boundary, ignore last 3 bits */
        cao &= ~7ULL;

        if(!(unavailable = (effective_addr1 >= cal)))
        {
            /* Point to the virtual address of the HW entry */
            ceh = cao + (effective_addr1 << 1);
            if(!(unavailable = ARCH_DEP(translate_addr) (ceh, USE_HOME_SPACE, regs, ACCTYPE_EMC)))
            {
                /* Convert real address to absolute address */
                ceh = APPLY_PREFIXING (regs->dat.raddr, regs->PX);

                /* Ensure absolute address is available */
                if (!(unavailable = (ceh >= regs->mainlim )))
                {
                    SIE_TRANSLATE(&ceh, ACCTYPE_WRITE, regs);

                    /* Update counter */
                    FETCH_HW(hwc, ceh + regs->mainstor);
                    ARCH_DEP( or_storage_key )( ceh, STORKEY_REF );
                    if(++hwc)
                    {
                        STORE_HW(ceh + regs->mainstor, hwc);
                        ARCH_DEP( or_storage_key )( ceh, (STORKEY_REF | STORKEY_CHANGE) );
                    }
                    else
                    {
                        /* Point to the virtual address of the FW entry */
                        cew = cao + (cal << 1) + (effective_addr1 << 2);
                        if(!(unavailable = ARCH_DEP(translate_addr) (cew, USE_HOME_SPACE, regs, ACCTYPE_EMC)))
                        {
                            /* Convert real address to absolute address */
                            cew = APPLY_PREFIXING (regs->dat.raddr, regs->PX);

                            /* Ensure absolute address is available */
                            if (!(unavailable = (cew >= regs->mainlim )))
                            {
                                SIE_TRANSLATE(&cew, ACCTYPE_WRITE, regs);

                                /* Update both counters */
                                FETCH_W(fwc, cew + regs->mainstor);
                                fwc++;

                                STORE_W(cew + regs->mainstor, fwc);
                                ARCH_DEP( or_storage_key )( cew, (STORKEY_REF | STORKEY_CHANGE) );

                                STORE_HW(ceh + regs->mainstor, hwc);
                                ARCH_DEP( or_storage_key )( ceh, (STORKEY_REF | STORKEY_CHANGE) );
                            }
                        }
                    }
                }
            }
        }

        /* Update the Enhance Monitor Exception Counter if the array could not be updated */
        if(unavailable)
        {
            FETCH_W(ec,psa->ec);
            ec++;
            /* Set the main storage reference and change bits */
            ARCH_DEP( or_storage_key )( px, (STORKEY_REF | STORKEY_CHANGE) );
            STORE_W(psa->ec,ec);
        }
        return;
    }
#endif /* defined( FEATURE_036_ENH_MONITOR_FACILITY ) */

    regs->monclass = i2;
    regs->MONCODE = effective_addr1;

    /* Generate a monitor event program interruption */
    regs->program_interrupt (regs, PGM_MONITOR_EVENT);
}


/*-------------------------------------------------------------------*/
/* 92   MVI   - Move Immediate                                  [SI] */
/*-------------------------------------------------------------------*/
DEF_INST(move_immediate)
{
BYTE    i2;                             /* Immediate operand         */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */

    SI(inst, regs, i2, b1, effective_addr1);
    PER_ZEROADDR_XCHECK( regs, b1 );

    /* Store immediate operand at operand address */
    ARCH_DEP(vstoreb) ( i2, effective_addr1, b1, regs );
}


/*-------------------------------------------------------------------*/
/* D2   MVC   - Move Character                                [SS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(move_character)
{
int     len;                            /* Length byte               */
int     b1, b2;                         /* Values of base fields     */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */

    SS_L(inst, regs, len, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );
    TXFC_INSTR_CHECK( regs );

    /* Move characters using current addressing mode and key */
    ARCH_DEP(move_chars) (effective_addr1, b1, regs->psw.pkey,
                effective_addr2, b2, regs->psw.pkey, len, regs);
}


#if defined( FEATURE_061_MISC_INSTR_EXT_FACILITY_3 )
/*-------------------------------------------------------------------*/
/* E50A MVCRL - Move Right to Left                             [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST( move_right_to_left )
{
int     len;                            /* Length byte               */
int     b1, b2;                         /* Values of base fields     */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */

    SSE( inst, regs, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    TXFC_INSTR_CHECK( regs );

    /* Load operand length-1 from general register 0 bits 56-63 */
    len = regs->GR_LHLCL(0);

    /* Move characters using current addressing mode and key */
    ARCH_DEP( move_chars_rl )( effective_addr1, b1, regs->psw.pkey,
                effective_addr2, b2, regs->psw.pkey, len, regs );

}
#endif /* defined( FEATURE_061_MISC_INSTR_EXT_FACILITY_3 ) */


/*-------------------------------------------------------------------*/
/* E8   MVCIN - Move Inverse                                  [SS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(move_inverse)
{
CACHE_ALIGN BYTE wrk[256];              /* Cache-aligned Work area   */
BYTE   *p1, *p2;                        /* Work ptrs for reversing   */
VADR    effective_addr1;                /* Effective address         */
VADR    effective_addr2;                /* Effective address         */
VADR    op2end;                         /* Where operand-2 ends      */
int     b1, b2;                         /* Base registers            */
BYTE    len;                            /* Amount to move minus 1    */

    SS_L( inst, regs, len, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    TXFC_INSTR_CHECK( regs );

    /* Copy operand-2 source string to work area */
    op2end = (effective_addr2 - len) & ADDRESS_MAXWRAP( regs );
    ARCH_DEP( vfetchc )( wrk, len, op2end, b2, regs );

    /* Reverse the string in place in our work area */
    p1 = &wrk[0];
    p2 = p1 + len;
    while (p1 < p2)
    {
        *p1 ^= *p2;
        *p2 ^= *p1;
        *p1 ^= *p2;
        p1++;
        p2--;
    }

    /* Copy results back to operand-1 destination */
    ARCH_DEP( vstorec )( wrk, len, effective_addr1, b1, regs );
}


/*-------------------------------------------------------------------*/
/* 0E   MVCL  - Move Long                                       [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(move_long)
{
int     r1, r2;                         /* Values of R fields        */
VADR    addr1, addr2;                   /* Operand addresses         */
int     len1, len2;                     /* Operand lengths           */
int     len, len3, len4;                /* Work lengths              */
VADR    n;                              /* Work area                 */
BYTE   *dest, *source;                  /* Mainstor addresses        */
BYTE    pad;                            /* Padding byte              */
#if defined( FEATURE_INTERVAL_TIMER )
int     orglen1;                        /* Original dest length      */
#endif

    RR( inst, regs, r1, r2 );

    ODD2_CHECK( r1, r2, regs );
    PER_ZEROADDR_L24CHECK2( regs, r1, r1+1, r2, r2+1 );
    TXFC_INSTR_CHECK( regs );

    /* Determine the destination and source addresses */
    addr1 = regs->GR( r1 ) & ADDRESS_MAXWRAP( regs );
    addr2 = regs->GR( r2 ) & ADDRESS_MAXWRAP( regs );

    /* Load padding byte from bits 0-7 of R2+1 register */
    pad = regs->GR_LHHCH( r2+1 );

    /* Load operand lengths from bits 8-31 of R1+1 and R2+1 */
    len1 = regs->GR_LA24( r1+1 );
    len2 = regs->GR_LA24( r2+1 );

#if defined( FEATURE_INTERVAL_TIMER )
    orglen1 = len1;
#endif

    ITIMER_SYNC( addr2, len2, regs );

    /* Test for destructive overlap */
    if (1
        && len2 > 1
        && len1 > 1
        && (0
            || !ACCESS_REGISTER_MODE( &(regs->psw) )
            || (!r1 ? 0 : regs->AR( r1 )) == (!r2 ? 0 : regs->AR( r2 ))
           )
    )
    {
        n = addr2 + (len2 < len1 ? len2 : len1) - 1;
        n &= ADDRESS_MAXWRAP( regs );

        if (0
            || (1
                && n > addr2
                && addr1 > addr2
                && addr1 <= n
               )
            || (1
                && n <= addr2
                && (0
                    || addr1 > addr2
                    || addr1 <= n
                   )
               )
        )
        {
            SET_GR_A( r1, regs, addr1 );
            SET_GR_A( r2, regs, addr2 );
            regs->psw.cc = 3;
#if 0
            LOGMSG( "MVCL destructive overlap: " );
            ARCH_DEP( display_inst )( regs, inst );
#endif
            /* Check for PER 1 GRA event */
            PER_GRA_CHECK( regs, PER_GRA_MASK4( r1, r1+1, r2, r2+1 ));
            return;
        }
    }

    /* Initialize source and dest addresses */
    if (len1)
    {
        if (len2)
            source = MADDRL(addr2, len2, r2, regs, ACCTYPE_READ, regs->psw.pkey );
        else
            source = NULL;

        dest = MADDRL( addr1, len1, r1, regs, ACCTYPE_WRITE, regs->psw.pkey );
    }
    else
    {
        /* FIXME : We shouldn't have to do that - just to prevent potentially
           uninitialized variable warning in GCC.. Can't see where it is getting
           this diagnostic from. ISW 2009/02/04 */
        source = NULL;
        dest   = NULL;
    }

    /* Set the condition code according to the lengths */
    regs->psw.cc = (len1 < len2) ? 1 : (len1 > len2) ? 2 : 0;

    /* Set the registers *after* translating - so that the instruction is properly
       nullified when there is an access exception on the 1st unit of operation */
    SET_GR_A( r1, regs, addr1 );
    SET_GR_A( r2, regs, addr2 );

    while (len1)
    {
        /* Clear or copy memory */
        if (!len2)
        {
            len = NOCROSSPAGEL( addr1, len1 ) ? len1 : (int)(PAGEFRAME_PAGESIZE - (addr1 & PAGEFRAME_BYTEMASK));
            memset( dest, pad, len );
        }
        else
        {
            len3 = NOCROSSPAGEL( addr1, len1 ) ? len1 : (int)(PAGEFRAME_PAGESIZE - (addr1 & PAGEFRAME_BYTEMASK));
            len4 = NOCROSSPAGEL( addr2, len2 ) ? len2 : (int)(PAGEFRAME_PAGESIZE - (addr2 & PAGEFRAME_BYTEMASK));
            len = len3 < len4 ? len3 : len4;
            /* Use concpy to ensure Concurrent block update consistency */
            concpy( regs, dest, source, len );
        }

        /* Adjust lengths and virtual addresses */
        len1 -= len;
        addr1 = (addr1 + len) & ADDRESS_MAXWRAP( regs );

        if (len2)
        {
            len2 -= len;
            addr2 = (addr2 + len) & ADDRESS_MAXWRAP( regs );
        }

        /* Update regs (since interrupt may occur) */
        SET_GR_A( r1, regs,addr1 );
        SET_GR_A( r2, regs,addr2 );

        regs->GR_LA24( r1+1 ) = len1;
        regs->GR_LA24( r2+1 ) = len2;

        /* Check for pending interrupt */
        if (1
            && len1 > 256
            && (0
                || OPEN_IC_EXTPENDING( regs )
                || OPEN_IC_IOPENDING(  regs )
               )
        )
        {
            // Backup the PSW for re-execution since instruction was interrupted
            SET_PSW_IA_AND_MAYBE_IP( regs, PSW_IA_FROM_IP( regs, -REAL_ILC( regs )));
            break;
        }

        /* Translate next source and dest addresses */
        if (len1)
        {
            if (len2)
            {
                if (addr2 & PAGEFRAME_BYTEMASK)
                    source += len;
                else
                    source = MADDRL(addr2, len2, r2, regs, ACCTYPE_READ, regs->psw.pkey );
            }
            if (addr1 & PAGEFRAME_BYTEMASK)
                dest += len;
            else
                dest = MADDRL( addr1, len1, r1, regs, ACCTYPE_WRITE, regs->psw.pkey );
        }

    } /* while (len1) */

    ITIMER_UPDATE( addr1, orglen1, regs );

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK4( r1, r1+1, r2, r2+1 ));

    /* If len1 is non-zero then we were interrupted */
    if (len1)
        RETURN_INTCHECK( regs );

} /* end DEF_INST( move_long ) */


#if defined( FEATURE_COMPARE_AND_MOVE_EXTENDED )
/*-------------------------------------------------------------------*/
/* A8   MVCLE - Move Long Extended                            [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(move_long_extended)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     cc;                             /* Condition code            */
VADR    addr1, addr2;                   /* Operand addresses         */
GREG    len1, len2;                     /* Operand lengths           */
BYTE    pad;                            /* Padding byte              */
size_t  cpu_length;                     /* cpu determined length     */
size_t  copylen;                        /* Length to copy            */
BYTE    *dest;                          /* Maint storage pointers    */
size_t  dstlen,srclen;                  /* Page wide src/dst lengths */

    RS(inst, regs, r1, r3, b2, effective_addr2);
    PER_ZEROADDR_LCHECK2( regs, r1, r1+1, r3, r3+1 );

    TXFC_INSTR_CHECK( regs );
    ODD2_CHECK(r1, r3, regs);

    /* Load padding byte from bits 24-31 of effective address */
    pad = effective_addr2 & 0xFF;

    /* Determine the destination and source addresses */
    addr1 = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    addr2 = regs->GR(r3) & ADDRESS_MAXWRAP(regs);

    /* Load operand lengths from bits 0-31 of R1+1 and R3+1 */
    len1 = GR_A(r1+1, regs);
    len2 = GR_A(r3+1, regs);

    /* set cpu_length as shortest distance to new page */
    if ((addr1 & 0xFFF) > (addr2 & 0xFFF))
        cpu_length = 0x1000 - (addr1 & 0xFFF);
    else
        cpu_length = 0x1000 - (addr2 & 0xFFF);

    dstlen=MIN(cpu_length,len1);
    srclen=MIN(cpu_length,len2);
    copylen=MIN(dstlen,srclen);

    /* Set the condition code according to the lengths */
    cc = (len1 < len2) ? 1 : (len1 > len2) ? 2 : 0;

    /* Obtain destination pointer */
    if(len1==0)
    {
        /* bail out if nothing to do */
        regs->psw.cc = cc;
        return;
    }

    dest = MADDRL (addr1, len1, r1, regs, ACCTYPE_WRITE, regs->psw.pkey);
    if(copylen!=0)
    {
        /* here if we need to copy data */
        BYTE *source;
        /* get source frame and copy concurrently */
        source = MADDRL(addr2, copylen, r3, regs, ACCTYPE_READ, regs->psw.pkey);
        concpy(regs,dest,source,(int)copylen);
        /* Adjust operands */
        addr2+=(int)copylen;
        len2-=(int)copylen;
        addr1+=(int)copylen;
        len1-=(int)copylen;

        /* Adjust length & pointers for this cycle */
        dest+=copylen;
        dstlen-=copylen;
        srclen-=copylen;
    }
    if(srclen==0 && dstlen!=0)
    {
        /* here if we need to pad the destination */
        memset(dest,pad,dstlen);

        /* Adjust destination operands */
        addr1+=(int)dstlen;
        len1-=(int)dstlen;
    }

    /* Update the registers */
    SET_GR_A(r1, regs,addr1);
    SET_GR_A(r1+1, regs,len1);
    SET_GR_A(r3, regs,addr2);
    SET_GR_A(r3+1, regs,len2);
    /* if len1 != 0 then set CC to 3 to indicate
       we have reached end of CPU dependent length */
    if(len1>0) cc=3;

    regs->psw.cc = cc;

}
#endif /* defined( FEATURE_COMPARE_AND_MOVE_EXTENDED ) */


#define MOVE_NUMERIC_BUMP( dst, src )                   \
    do                                                  \
    {                                                   \
        *(dst) = (*(dst) & 0xF0) | (*(src) & 0x0F);     \
        dst++;                                          \
        src++;                                          \
    }                                                   \
    while (0)

/*-------------------------------------------------------------------*/
/* D1   MVN   - Move Numerics                                 [SS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(move_numerics)
{
VADR    effective_addr1;                /* Operand virtual address   */
VADR    effective_addr2;                /* Operand virtual address   */
int     len, r1, r2;                    /* Operand values            */
BYTE   *dest1, *dest2;                  /* Destination addresses     */
BYTE   *source1, *source2;              /* Source addresses          */
BYTE   *sk1, *sk2;                      /* Storage key addresses     */
int     len2, len3;                     /* Lengths to copy           */
int     i;                              /* Loop counter              */

    SS_L( inst, regs, len, r1, effective_addr1, r2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, r1, r2 );

    TXFC_INSTR_CHECK( regs );
    ITIMER_SYNC( effective_addr2, len, regs );

    /* Translate addresses of leftmost operand bytes */
    dest1 = MADDRL( effective_addr1, len+1, r1, regs, ACCTYPE_WRITE_SKP, regs->psw.pkey );
    sk1 = regs->dat.storkey;
    source1 = MADDRL(effective_addr2, len+1, r2, regs, ACCTYPE_READ, regs->psw.pkey );

    /* There are several scenarios (in optimal order):
     * (1) dest boundary and source boundary not crossed
     * (2) dest boundary not crossed and source boundary crossed
     * (3) dest boundary crossed and source boundary not crossed
     * (4) dest boundary and source boundary are crossed
     *     (a) dest and source boundary cross at the same time
     *     (b) dest boundary crossed first
     *     (c) source boundary crossed first
     */

    if (NOCROSSPAGE( effective_addr1, len ))
    {
        if (NOCROSSPAGE( effective_addr2,len ))
        {
            /* (1) - No boundaries are crossed */
            for (i=0; i <= len; i++)
                MOVE_NUMERIC_BUMP( dest1, source1 );
        }
        else
        {
            /* (2) - Second operand crosses a boundary */
            len2 = PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK);
            source2 = MADDRL((effective_addr2 + len2) & ADDRESS_MAXWRAP( regs ),
              len + 1 - len2, r2, regs, ACCTYPE_READ, regs->psw.pkey );

            for (i=0; i < len2; i++)
                MOVE_NUMERIC_BUMP( dest1, source1 );

            len2 = len - len2;

            for (i=0; i <= len2; i++)
                MOVE_NUMERIC_BUMP( dest1, source2 );
        }
        ARCH_DEP( or_storage_key_by_ptr )( sk1, (STORKEY_REF | STORKEY_CHANGE) );
    }
    else
    {
        /* First operand crosses a boundary */
        len2 = PAGEFRAME_PAGESIZE - (effective_addr1 & PAGEFRAME_BYTEMASK);
        dest2 = MADDRL((effective_addr1 + len2) & ADDRESS_MAXWRAP( regs ),
            len + 1 - len2, r1, regs, ACCTYPE_WRITE_SKP, regs->psw.pkey );
        sk2 = regs->dat.storkey;

        if (NOCROSSPAGE( effective_addr2, len ))
        {
            /* (3) - First operand crosses a boundary */
            for (i=0; i < len2; i++)
                MOVE_NUMERIC_BUMP( dest1, source1 );

            len2 = len - len2;

            for (i=0; i <= len2; i++)
                MOVE_NUMERIC_BUMP( dest2, source1 );
        }
        else
        {
            /* (4) - Both operands cross a boundary */
            len3 = PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK);
            source2 = MADDRL((effective_addr2 + len3) & ADDRESS_MAXWRAP( regs ),
              len + 1 - len3, r2, regs, ACCTYPE_READ, regs->psw.pkey );
            if (len2 == len3)
            {
                /* (4a) - Both operands cross at the same time */
                for (i=0; i < len2; i++)
                    MOVE_NUMERIC_BUMP( dest1, source1 );

                len2 = len - len2;

                for (i=0; i <= len2; i++)
                    MOVE_NUMERIC_BUMP( dest2, source2 );
            }
            else if (len2 < len3)
            {
                /* (4b) - First operand crosses first */
                for (i=0; i < len2; i++)
                    MOVE_NUMERIC_BUMP( dest1, source1 );

                len2 = len3 - len2;

                for (i=0; i < len2; i++)
                    MOVE_NUMERIC_BUMP( dest2, source1 );

                len2 = len - len3;

                for (i=0; i <= len2; i++)
                    MOVE_NUMERIC_BUMP( dest2, source2 );
            }
            else
            {
                /* (4c) - Second operand crosses first */
                for (i=0; i < len3; i++)
                    MOVE_NUMERIC_BUMP( dest1, source1 );

                len3 = len2 - len3;

                for (i=0; i < len3; i++)
                    MOVE_NUMERIC_BUMP( dest1, source2 );

                len3 = len - len2;

                for (i=0; i <= len3; i++)
                    MOVE_NUMERIC_BUMP( dest2, source2 );
            }
        }
        ARCH_DEP( or_storage_key_by_ptr )( sk1, (STORKEY_REF | STORKEY_CHANGE) );
        ARCH_DEP( or_storage_key_by_ptr )( sk2, (STORKEY_REF | STORKEY_CHANGE) );
    }
    ITIMER_UPDATE( effective_addr1, len, regs );
}


#if defined( FEATURE_STRING_INSTRUCTION )
/*-------------------------------------------------------------------*/
/* B255 MVST  - Move String                                    [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(move_string)
{
int     r1, r2;                         /* Values of R fields        */
int     i;                              /* Loop counter              */
int     dist1, dist2;                   /* length working distances  */
int     cpu_length;                     /* CPU determined length     */
VADR    addr1, addr2;                   /* End/start addresses       */
BYTE    *main1, *main2;                 /* ptrs to compare bytes     */
BYTE    termchar;                       /* Terminating character     */

    RRE( inst, regs, r1, r2 );
    PER_ZEROADDR_CHECK2( regs, r1, r2 );

    TXFC_INSTR_CHECK( regs );

    /* Program check if bits 0-23 of register 0 not zero */
    if ((regs->GR_L(0) & 0xFFFFFF00) != 0)
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Load string terminating character from register 0 bits 24-31 */
    termchar = regs->GR_LHLCL(0);

    /* Determine the operand addresses */
    addr1 = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    addr2 = regs->GR(r2) & ADDRESS_MAXWRAP(regs);

    /* Per the specification, the CPU determined length can be
       any number above zero. Set the CPU determined length to
       the nearest end of page of either operand.
    */

    dist1 = PAGEFRAME_PAGESIZE - (addr1 & PAGEFRAME_BYTEMASK);
    dist2 = PAGEFRAME_PAGESIZE - (addr2 & PAGEFRAME_BYTEMASK);

    cpu_length = min( dist1, dist2 ); /* (nearest end of page) */
    main1 = MADDRL( addr1, cpu_length, r1, regs, ACCTYPE_WRITE, regs->psw.pkey );
    main2 = MADDRL( addr2, cpu_length, r2, regs, ACCTYPE_READ,  regs->psw.pkey );

    for (i=0; i < cpu_length; i++)
    {
        /* Move a single byte */
        *main1 = *main2;

        /* If we find the terminating character in operand 2, then
           the movement is complete.  Set CC=1 and the R1 register
           to the location of the just moved terminating character
           and leave the R2 register unchanged and exit.
        */
        if (*main2 == termchar)
        {
            regs->psw.cc = 1;
            SET_GR_A( r1, regs,addr1 );
            return;
        }

        /* Bump both operands to the next byte */
        main1++;
        main2++;

        addr1++;
        addr1 &= ADDRESS_MAXWRAP( regs );

        addr2++;
        addr2 &= ADDRESS_MAXWRAP( regs );

    } /* end for(i) */

    /* No terminating character was found within the CPU determined
       number of bytes.  Set CC=3 and exit with the current position
       updated in both operand registers.
    */
    regs->psw.cc = 3;
    SET_GR_A( r1, regs,addr1 );
    SET_GR_A( r2, regs,addr2 );

} /* end DEF_INST(move_string) */
#endif /* defined( FEATURE_STRING_INSTRUCTION ) */


/*-------------------------------------------------------------------*/
/* F1   MVO   - Move with Offset                              [SS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(move_with_offset)
{
int     l1, l2;                         /* Lenght values             */
int     b1, b2;                         /* Values of base registers  */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
int     i, j;                           /* Loop counters             */
BYTE    sbyte;                          /* Source operand byte       */
BYTE    dbyte;                          /* Destination operand byte  */

    SS(inst, regs, l1, l2, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );
    TXFC_INSTR_CHECK( regs );

    /* If operand 1 crosses a page, make sure both pages are accessible */
    if((effective_addr1 & PAGEFRAME_PAGEMASK) !=
        ((effective_addr1 + l1) & PAGEFRAME_PAGEMASK))
        ARCH_DEP(validate_operand) (effective_addr1, b1, l1, ACCTYPE_WRITE_SKP, regs);

    /* If operand 2 crosses a page, make sure both pages are accessible */
    if((effective_addr2 & PAGEFRAME_PAGEMASK) !=
        ((effective_addr2 + l2) & PAGEFRAME_PAGEMASK))
    ARCH_DEP(validate_operand) (effective_addr2, b2, l2, ACCTYPE_READ, regs);

    /* Fetch the rightmost byte from the source operand */
    effective_addr2 += l2;
    effective_addr2 &= ADDRESS_MAXWRAP(regs);
    sbyte = ARCH_DEP(vfetchb) ( effective_addr2--, b2, regs );

    /* Fetch the rightmost byte from the destination operand */
    effective_addr1 += l1;
    effective_addr1 &= ADDRESS_MAXWRAP(regs);
    dbyte = ARCH_DEP(vfetchb) ( effective_addr1, b1, regs );

    /* Move low digit of source byte to high digit of destination */
    dbyte &= 0x0F;
    dbyte |= ( sbyte << 4 ) & 0xff;
    ARCH_DEP(vstoreb) ( dbyte, effective_addr1--, b1, regs );

    /* Process remaining bytes from right to left */
    for (i = l1, j = l2; i > 0; i--)
    {
        /* Move previous high digit to destination low digit */
        dbyte = sbyte >> 4;

        /* Fetch next byte from second operand */
        if ( j-- > 0 ) {
            effective_addr2 &= ADDRESS_MAXWRAP(regs);
            sbyte = ARCH_DEP(vfetchb) ( effective_addr2--, b2, regs );
        }
        else
            sbyte = 0x00;

        /* Move low digit to destination high digit */
        dbyte |= ( sbyte << 4 ) & 0xff;
        effective_addr1 &= ADDRESS_MAXWRAP(regs);
        ARCH_DEP(vstoreb) ( dbyte, effective_addr1--, b1, regs );

    } /* end for(i) */

}

#define MOVE_ZONE_BUMP( dst, src )                      \
    do                                                  \
    {                                                   \
        *(dst) = (*(dst) & 0x0F) | (*(src) & 0xF0);     \
        dst++;                                          \
        src++;                                          \
    }                                                   \
    while (0)

/*-------------------------------------------------------------------*/
/* D3   MVZ   - Move Zones                                    [SS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(move_zones)
{
VADR    effective_addr1;                /* Operand virtual addresses */
VADR    effective_addr2;                /* Operand virtual addresses */
int     len, r1, r2;                    /* Operand values            */
BYTE   *dest1, *dest2;                  /* Destination addresses     */
BYTE   *source1, *source2;              /* Source addresses          */
BYTE   *sk1, *sk2;                      /* Storage key addresses     */
int     len2, len3;                     /* Lengths to copy           */
int     i;                              /* Loop counter              */

    SS_L( inst, regs, len, r1, effective_addr1, r2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, r1, r2 );

    TXFC_INSTR_CHECK( regs );
    ITIMER_SYNC( effective_addr2, len, regs );

    /* Translate addresses of leftmost operand bytes */
    dest1 = MADDRL( effective_addr1, len+1, r1, regs, ACCTYPE_WRITE_SKP, regs->psw.pkey );
    sk1 = regs->dat.storkey;
    source1 = MADDRL(effective_addr2, len+1, r2, regs, ACCTYPE_READ, regs->psw.pkey );

    /* There are several scenarios (in optimal order):
     * (1) dest boundary and source boundary not crossed
     * (2) dest boundary not crossed and source boundary crossed
     * (3) dest boundary crossed and source boundary not crossed
     * (4) dest boundary and source boundary are crossed
     *     (a) dest and source boundary cross at the same time
     *     (b) dest boundary crossed first
     *     (c) source boundary crossed first
     */

    if (NOCROSSPAGE( effective_addr1, len ))
    {
        if (NOCROSSPAGE( effective_addr2, len ))
        {
            /* (1) - No boundaries are crossed */
            for (i=0; i <= len; i++)
                MOVE_ZONE_BUMP( dest1, source1 );
        }
        else
        {
            /* (2) - Second operand crosses a boundary */
            len2 = PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK);
            source2 = MADDRL((effective_addr2 + len2) & ADDRESS_MAXWRAP( regs ),
             len + 1 - len2, r2, regs, ACCTYPE_READ, regs->psw.pkey );

            for (i=0; i < len2; i++)
                MOVE_ZONE_BUMP( dest1, source1 );

            len2 = len - len2;

            for (i=0; i <= len2; i++)
                MOVE_ZONE_BUMP( dest1, source2 );
        }
        ARCH_DEP( or_storage_key_by_ptr )( sk1, (STORKEY_REF | STORKEY_CHANGE) );
    }
    else
    {
        /* First operand crosses a boundary */
        len2 = PAGEFRAME_PAGESIZE - (effective_addr1 & PAGEFRAME_BYTEMASK);
        dest2 = MADDRL((effective_addr1 + len2) & ADDRESS_MAXWRAP( regs ),
         len + 1 - len2, r1, regs, ACCTYPE_WRITE_SKP, regs->psw.pkey );
        sk2 = regs->dat.storkey;

        if (NOCROSSPAGE( effective_addr2, len ))
        {
            /* (3) - First operand crosses a boundary */
            for (i=0; i < len2; i++)
                MOVE_ZONE_BUMP( dest1, source1 );

            len2 = len - len2;

            for (i=0; i <= len2; i++)
                MOVE_ZONE_BUMP( dest2, source1 );
        }
        else
        {
            /* (4) - Both operands cross a boundary */
            len3 = PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK);
            source2 = MADDRL((effective_addr2 + len3) & ADDRESS_MAXWRAP( regs ),
             len + 1 - len3,  r2, regs, ACCTYPE_READ, regs->psw.pkey );
            if (len2 == len3)
            {
                /* (4a) - Both operands cross at the same time */
                for (i=0; i < len2; i++)
                    MOVE_ZONE_BUMP( dest1, source1 );

                len2 = len - len2;

                for (i=0; i <= len2; i++)
                    MOVE_ZONE_BUMP( dest2, source2 );
            }
            else if (len2 < len3)
            {
                /* (4b) - First operand crosses first */
                for (i=0; i < len2; i++)
                    MOVE_ZONE_BUMP( dest1, source1 );

                len2 = len3 - len2;

                for (i=0; i < len2; i++)
                    MOVE_ZONE_BUMP( dest2, source1 );

                len2 = len - len3;

                for (i=0; i <= len2; i++)
                    MOVE_ZONE_BUMP( dest2, source2 );
            }
            else
            {
                /* (4c) - Second operand crosses first */
                for (i=0; i < len3; i++)
                    MOVE_ZONE_BUMP( dest1, source1 );

                len3 = len2 - len3;

                for (i=0; i < len3; i++)
                    MOVE_ZONE_BUMP( dest1, source2 );

                len3 = len - len2;

                for (i=0; i <= len3; i++)
                    MOVE_ZONE_BUMP( dest2, source2 );
            }
        }
        ARCH_DEP( or_storage_key_by_ptr )( sk1, (STORKEY_REF | STORKEY_CHANGE) );
        ARCH_DEP( or_storage_key_by_ptr )( sk2, (STORKEY_REF | STORKEY_CHANGE) );
    }
    ITIMER_UPDATE( effective_addr1, len, regs );
}


/*-------------------------------------------------------------------*/
/* 1C   MR    - Multiply Register                               [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    ODD_CHECK(r1, regs);

    /* Multiply r1+1 by r2 and place result in r1 and r1+1 */
    mul_signed (&(regs->GR_L(r1)),&(regs->GR_L(r1+1)),
                    regs->GR_L(r1+1),
                    regs->GR_L(r2));

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK2( r1, r1+1 ));
}


/*-------------------------------------------------------------------*/
/* 5C   M     - Multiply                                      [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    ODD_CHECK(r1, regs);

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Multiply r1+1 by n and place result in r1 and r1+1 */
    mul_signed (&(regs->GR_L(r1)), &(regs->GR_L(r1+1)),
                    regs->GR_L(r1+1),
                    n);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK2( r1, r1+1 ));
}


/*-------------------------------------------------------------------*/
/* 4C   MH    - Multiply Halfword                             [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_halfword)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
S32     n;                              /* 32-bit operand values     */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Load 2 bytes from operand address */
    n = (S16)ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

    /* Multiply R1 register by n, ignore leftmost 32 bits of
       result, and place rightmost 32 bits in R1 register */
    mul_signed ((U32 *)&n, &(regs->GR_L(r1)), regs->GR_L(r1), n);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


#if defined( FEATURE_IMMEDIATE_AND_RELATIVE )
/*-------------------------------------------------------------------*/
/* A7xC MHI   - Multiply Halfword Immediate                   [RI-a] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_halfword_immediate)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand            */

    RI(inst, regs, r1, opcd, i2);

    /* Multiply register by operand ignoring overflow  */
    regs->GR_L(r1) = (S32)regs->GR_L(r1) * (S16)i2;

} /* end DEF_INST(multiply_halfword_immediate) */


/*-------------------------------------------------------------------*/
/* B252 MSR   - Multiply Single Register                       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_single_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    /* Multiply signed registers ignoring overflow */
    regs->GR_L(r1) = (S32)regs->GR_L(r1) * (S32)regs->GR_L(r2);

} /* end DEF_INST(multiply_single_register) */


/*-------------------------------------------------------------------*/
/* 71   MS    - Multiply Single                               [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(multiply_single)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Load second operand from operand address */
    n = ARCH_DEP(vfetch4) ( effective_addr2, b2, regs );

    /* Multiply signed operands ignoring overflow */
    regs->GR_L(r1) = (S32)regs->GR_L(r1) * (S32)n;

} /* end DEF_INST(multiply_single) */
#endif /* defined( FEATURE_IMMEDIATE_AND_RELATIVE ) */

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "general1.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "general1.c"
  #endif

#endif /* !defined( _GEN_ARCH ) */
