/* GENERAL2.C   (C) Copyright Roger Bowler, 1994-2012                */
/*              (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) and others 2013-2022                             */
/*              Hercules CPU Emulator - Instructions N-Z             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* UPT & CFC                (C) Copyright Peter Kuschnerus, 1999-2009*/
/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements general instructions N-Z of the            */
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

#define _GENERAL2_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

/* When an operation code has unused operand(s) (IPK, e.g.), it will */
/* attract  a diagnostic for a set, but unused variable.  Fixing the */
/* macros to support e.g., RS_NOOPS is not productive, so:           */
DISABLE_GCC_UNUSED_SET_WARNING

/*-------------------------------------------------------------------*/
/* 16   OR    - Or Register                                     [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(or_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* OR second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_L(r1) |= regs->GR_L(r2) ) ? 1 : 0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 56   O     - Or                                            [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(or)
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

    /* OR second operand with first and set condition code */
    regs->psw.cc = ( regs->GR_L(r1) |= n ) ? 1 : 0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 96   OI    - Or Immediate                                    [SI] */
/*-------------------------------------------------------------------*/
DEF_INST(or_immediate)
{
BYTE    i2;                             /* Immediate operand byte    */
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
        /* OR byte with immediate operand, setting condition code */
        regs->psw.cc = (H_ATOMIC_OP( dest, i2, or, Or, | ) != 0);
    }
    RELEASE_MAINLOCK( regs );

    ITIMER_UPDATE(effective_addr1, 0, regs);
}


/*-------------------------------------------------------------------*/
/* D6   OC    - Or Characters                                 [SS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(or_character)
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
        source1 = MADDR( effective_addr2,  b2, regs, ACCTYPE_READ,  regs->psw.pkey );
        dest1   = MADDR( effective_addr1,  b1, regs, ACCTYPE_WRITE, regs->psw.pkey );
        *dest1 |= *source1;
        regs->psw.cc = (*dest1 != 0);
        ITIMER_UPDATE( effective_addr1, len, regs );
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
    source1 = MADDRL( effective_addr2, len+1, b2, regs, ACCTYPE_READ, regs->psw.pkey );

    if (NOCROSSPAGE( effective_addr1, len ))
    {
        if (NOCROSSPAGE( effective_addr2, len ))
        {
            /* (1) - No boundaries are crossed */
            for (i=0; i <= len; i++)
                if ((*dest1++ |= *source1++))
                    cc = 1;
        }
        else
        {
             /* (2) - Second operand crosses a boundary */
             len2 = PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK);
             source2 = MADDRL((effective_addr2 + len2) & ADDRESS_MAXWRAP( regs ),
              len + 1 - len2,  b2, regs, ACCTYPE_READ, regs->psw.pkey );
             for (i=0; i < len2; i++)
                 if ( (*dest1++ |= *source1++) )
                     cc = 1;

             len2 = len - len2;

             for (i=0; i <= len2; i++)
                 if ( (*dest1++ |= *source2++) )
                     cc = 1;
        }
        ARCH_DEP( or_storage_key_by_ptr )( sk1, (STORKEY_REF | STORKEY_CHANGE) );
    }
    else
    {
        /* First operand crosses a boundary */
        len2 = PAGEFRAME_PAGESIZE - (effective_addr1 & PAGEFRAME_BYTEMASK);
        dest2 = MADDRL((effective_addr1 + len2) & ADDRESS_MAXWRAP( regs ),
         len + 1 - len2,b1, regs, ACCTYPE_WRITE_SKP, regs->psw.pkey );
        sk2 = regs->dat.storkey;

        if (NOCROSSPAGE( effective_addr2, len ))
        {
             /* (3) - First operand crosses a boundary */
             for (i=0; i < len2; i++)
                 if ((*dest1++ |= *source1++))
                     cc = 1;

             len2 = len - len2;

             for (i=0; i <= len2; i++)
                 if ((*dest2++ |= *source1++))
                     cc = 1;
        }
        else
        {
            /* (4) - Both operands cross a boundary */
            len3 = PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK);
            source2 = MADDRL((effective_addr2 + len3) & ADDRESS_MAXWRAP( regs ),
             len + 1 - len3,  b2, regs, ACCTYPE_READ, regs->psw.pkey );
            if (len2 == len3)
            {
                /* (4a) - Both operands cross at the same time */
                for (i=0; i < len2; i++)
                    if ((*dest1++ |= *source1++))
                        cc = 1;

                len2 = len - len2;

                for (i=0; i <= len2; i++)
                    if ((*dest2++ |= *source2++))
                        cc = 1;
            }
            else if (len2 < len3)
            {
                /* (4b) - First operand crosses first */
                for (i=0; i < len2; i++)
                    if ((*dest1++ |= *source1++))
                        cc = 1;

                len2 = len3 - len2;

                for (i=0; i < len2; i++)
                    if ((*dest2++ |= *source1++))
                        cc = 1;

                len2 = len - len3;

                for (i=0; i <= len2; i++)
                    if ((*dest2++ |= *source2++))
                        cc = 1;
            }
            else
            {
                /* (4c) - Second operand crosses first */
                for ( i = 0; i < len3; i++)
                    if ((*dest1++ |= *source1++))
                        cc = 1;

                len3 = len2 - len3;

                for (i=0; i < len3; i++)
                    if ((*dest1++ |= *source2++))
                        cc = 1;

                len3 = len - len2;

                for (i=0; i <= len3; i++)
                    if ((*dest2++ |= *source2++))
                        cc = 1;
            }
        }
        ARCH_DEP( or_storage_key_by_ptr )( sk1, (STORKEY_REF | STORKEY_CHANGE) );
        ARCH_DEP( or_storage_key_by_ptr )( sk2, (STORKEY_REF | STORKEY_CHANGE) );
    }

    regs->psw.cc = cc;

    ITIMER_UPDATE( effective_addr1, len, regs );
}


/*-------------------------------------------------------------------*/
/* F2   PACK  - Pack                                          [SS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(pack)
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

    /* Exchange the digits in the rightmost byte */
    effective_addr1 += l1;
    effective_addr2 += l2;
    sbyte = ARCH_DEP(vfetchb) ( effective_addr2, b2, regs );
    dbyte = ( (sbyte << 4) | (sbyte >> 4) ) & 0xff;
    ARCH_DEP(vstoreb) ( dbyte, effective_addr1, b1, regs );

    /* Process remaining bytes from right to left */
    for (i = l1, j = l2; i > 0; i--)
    {
        /* Fetch source bytes from second operand */
        if (j-- > 0)
        {
            sbyte = ARCH_DEP(vfetchb) ( --effective_addr2, b2, regs );
            dbyte = sbyte & 0x0F;

            if (j-- > 0)
            {
                effective_addr2 &= ADDRESS_MAXWRAP(regs);
                sbyte = ARCH_DEP(vfetchb) ( --effective_addr2, b2, regs );
                dbyte |= (sbyte << 4) & 0xff;
            }
        }
        else
        {
            dbyte = 0;
        }

        /* Store packed digits at first operand address */
        ARCH_DEP(vstoreb) ( dbyte, --effective_addr1, b1, regs );

        /* Wraparound according to addressing mode */
        effective_addr1 &= ADDRESS_MAXWRAP(regs);
        effective_addr2 &= ADDRESS_MAXWRAP(regs);

    } /* end for(i) */

}


#if defined( FEATURE_PERFORM_LOCKED_OPERATION )
/*-------------------------------------------------------------------*/
/* EE   PLO   - Perform Locked Operation                      [SS-e] */
/*-------------------------------------------------------------------*/
DEF_INST(perform_locked_operation)
{
int     r1, r3;                         /* Lenght values             */
int     b2, b4;                         /* Values of base registers  */
VADR    effective_addr2,
        effective_addr4;                /* Effective addresses       */

    SS(inst, regs, r1, r3, b2, effective_addr2, b4, effective_addr4);
    PER_ZEROADDR_XCHECK2( regs, b2, b4 );
    TXF_INSTR_CHECK( regs );

    if(regs->GR_L(0) & PLO_GPR0_RESV)
        regs->program_interrupt(regs, PGM_SPECIFICATION_EXCEPTION);

    if(regs->GR_L(0) & PLO_GPR0_T)
        switch(regs->GR_L(0) & PLO_GPR0_FC)
    {
        case PLO_CL:
        case PLO_CLG:
        case PLO_CS:
        case PLO_CSG:
        case PLO_DCS:
        case PLO_DCSG:
        case PLO_CSST:
        case PLO_CSSTG:
        case PLO_CSDST:
        case PLO_CSDSTG:
        case PLO_CSTST:
        case PLO_CSTSTG:
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        case PLO_CLGR:
        case PLO_CLX:
        case PLO_CSGR:
        case PLO_CSX:
        case PLO_DCSGR:
        case PLO_DCSX:
        case PLO_CSSTGR:
        case PLO_CSSTX:
        case PLO_CSDSTGR:
        case PLO_CSDSTX:
        case PLO_CSTSTGR:
        case PLO_CSTSTX:
#endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

            /* Indicate function supported */
            regs->psw.cc = 0;
            break;

        default:
            PTT_ERR("*PLO",regs->GR_L(0),regs->GR_L(r1),regs->psw.IA_L);
            /* indicate function not supported */
            regs->psw.cc = 3;
            break;
    }
    else
    {
        /* gpr1/ar1 indentify the program lock token, which is used
           to select a lock from the model dependent number of locks
           in the configuration.  We simply use 1 lock which is the
           main storage access lock which is also used by CS, CDS
           and TS.                                               *JJ */
        OBTAIN_MAINLOCK_UNCONDITIONAL( regs );
        {
            switch(regs->GR_L(0) & PLO_GPR0_FC)
            {
                case PLO_CL:
                    regs->psw.cc = ARCH_DEP(plo_cl) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CLG:
                    regs->psw.cc = ARCH_DEP(plo_clg) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CS:
                    regs->psw.cc = ARCH_DEP(plo_cs) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSG:
                    regs->psw.cc = ARCH_DEP(plo_csg) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_DCS:
                    regs->psw.cc = ARCH_DEP(plo_dcs) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_DCSG:
                    regs->psw.cc = ARCH_DEP(plo_dcsg) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSST:
                    regs->psw.cc = ARCH_DEP(plo_csst) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSSTG:
                    regs->psw.cc = ARCH_DEP(plo_csstg) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSDST:
                    regs->psw.cc = ARCH_DEP(plo_csdst) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSDSTG:
                    regs->psw.cc = ARCH_DEP(plo_csdstg) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSTST:
                    regs->psw.cc = ARCH_DEP(plo_cstst) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSTSTG:
                    regs->psw.cc = ARCH_DEP(plo_cststg) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
                case PLO_CLGR:
                    regs->psw.cc = ARCH_DEP(plo_clgr) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CLX:
                    regs->psw.cc = ARCH_DEP(plo_clx) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSGR:
                    regs->psw.cc = ARCH_DEP(plo_csgr) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSX:
                    regs->psw.cc = ARCH_DEP(plo_csx) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_DCSGR:
                    regs->psw.cc = ARCH_DEP(plo_dcsgr) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_DCSX:
                    regs->psw.cc = ARCH_DEP(plo_dcsx) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSSTGR:
                    regs->psw.cc = ARCH_DEP(plo_csstgr) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSSTX:
                    regs->psw.cc = ARCH_DEP(plo_csstx) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSDSTGR:
                    regs->psw.cc = ARCH_DEP(plo_csdstgr) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSDSTX:
                    regs->psw.cc = ARCH_DEP(plo_csdstx) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSTSTGR:
                    regs->psw.cc = ARCH_DEP(plo_cststgr) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
                case PLO_CSTSTX:
                    regs->psw.cc = ARCH_DEP(plo_cststx) (r1, r3,
                            effective_addr2, b2, effective_addr4, b4, regs);
                    break;
   #endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

                default:
                    regs->program_interrupt(regs, PGM_SPECIFICATION_EXCEPTION);
            }
        }
        RELEASE_MAINLOCK_UNCONDITIONAL( regs );

        if(regs->psw.cc && sysblk.cpus > 1)
        {
            PTT_CSF("*PLO",regs->GR_L(0),regs->GR_L(r1),regs->psw.IA_L);
            sched_yield();
        }
    }
}
#endif /* defined( FEATURE_PERFORM_LOCKED_OPERATION ) */


#if defined( FEATURE_STRING_INSTRUCTION )
/*-------------------------------------------------------------------*/
/* B25E SRST  - Search String                                  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(search_string)
{
int     r1, r2;                         /* Values of R fields        */
int     i;                              /* Loop counter              */
int     dist;                           /* length working distance   */
int     cpu_length;                     /* CPU determined length     */
VADR    addr1, addr2;                   /* End/start addresses       */
BYTE    *main2;                         /* Operand-2 mainstor addr   */
BYTE    termchar;                       /* Terminating character     */

    RRE( inst, regs, r1, r2 );
    PER_ZEROADDR_CHECK( regs, r2 );

    TXFC_INSTR_CHECK( regs );

    /* Program check if bits 0-23 of register 0 not zero */
    if ((regs->GR_L(0) & 0xFFFFFF00) != 0)
        regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Load string terminating character from register 0 bits 24-31 */
    termchar = regs->GR_LHLCL(0);

    /* Determine the operand end and start addresses */
    addr1 = regs->GR( r1 ) & ADDRESS_MAXWRAP( regs );
    addr2 = regs->GR( r2 ) & ADDRESS_MAXWRAP( regs );

    /* Set the minimum CPU determined length per the specification  */
    cpu_length = 256;

    /* Should the second operand cross a page boundary, we need to
       break up the search into two parts (one part in each page)
       to meet the minimum requirement of 256 CPU determined bytes. */
    if (unlikely( CROSSPAGEL( addr2, cpu_length )))
    {
        /* Compute the distance to the end of operand-2's page */
        dist = PAGEFRAME_PAGESIZE - (addr2 & PAGEFRAME_BYTEMASK);

        while (cpu_length)
        {
            /* We need to check the boundary condition
               BEFORE attempting to access storage,
               because if the boundary condition is met,
               there is no further need to access storage.
            */
            /* NOTE: "When the address in general register R1 is less
               than the address in general register R2, condition code
               2 can be set only if the operand wraps around from the
               top of storage to location 0."  The below comparison
               for == is thus correct.
            */
            if (addr2 == addr1)
            {
                regs->psw.cc = 2;
                return;
            }
            main2 = MADDRL(addr2, cpu_length, r2, regs, ACCTYPE_READ, regs->psw.pkey );
            for (i=0; i < dist; i++)
            {
                /* If operand end address has been reached, return
                   CC=2 and leave the R1 and R2 registers unchanged
                */
                /* NOTE: "When the address in general register R1 is
                   less than the address in general register R2, then
                   condition code 2 can be set only when the operand
                   wraps around from the top of storage to location 0."
                   Thus the below == comparison is correct.
                */
                if (addr2 == addr1)
                {
                    regs->psw.cc = 2;
                    return;
                }

                /* Set CC=1 if the terminating character was found,
                   and load the address of that character into R1 */
                if (*main2 == termchar)
                {
                    SET_GR_A( r1, regs, addr2 );
                    regs->psw.cc = 1;
                    return;
                }

                /* Bump operand-2 */
                main2++;
                addr2++;
                addr2 &= ADDRESS_MAXWRAP( regs );

            } /* end for(i) */

            cpu_length -= dist;
            dist = cpu_length;

        } /* end while */

        /* The CPU determine number of bytes has been reached. Set R2
           to point to next character of operand, set CC=3 and exit */
        SET_GR_A( r2, regs, addr2 );
        regs->psw.cc = 3;
        return;
    } /* end if unlikely() */

    /* We don't cross a page boundary with the minimum length, so
       extend the CPU determined length out to the end of the page */
    cpu_length = PAGEFRAME_PAGESIZE - (addr2 & PAGEFRAME_BYTEMASK);

    /* We need to check the boundary condition
       BEFORE attempting to access storage,
       because if the boundary condition is met,
       there is no further need to access storage.
    */
    /* NOTE: "When the address in general register R1 is less
       than the address in general register R2, condition code
       2 can be set only if the operand wraps around from the
       top of storage to location 0."  The below comparison
       for == is thus correct.
    */
    if (addr2 == addr1)
    {
        regs->psw.cc = 2;
        return;
    }

    main2 = MADDRL(addr2, cpu_length, r2, regs, ACCTYPE_READ, regs->psw.pkey );
    for (i=0; i < cpu_length; i++)
    {
        /* If operand end address has been reached, return
           CC=2 and leave the R1 and R2 registers unchanged
        */
        /* NOTE: "When the address in general register R1 is less
           than the address in general register R2, condition code
           2 can be set only if the operand wraps around from the
           top of storage to location 0."  The below comparison
           for == is thus correct.
        */
        if (addr2 == addr1)
        {
            regs->psw.cc = 2;
            return;
        }

        /* If the terminating character was found, return
           CC=1 and load the address of the character in R1 */
        if (*main2 == termchar)
        {
            SET_GR_A( r1, regs, addr2 );
            regs->psw.cc = 1;
            return;
        }

        /* Bump operand-2 */
        main2++;
        addr2++;
        addr2 &= ADDRESS_MAXWRAP( regs );

    } /* end for(i) */

    /* The CPU determine number of bytes has been reached.
       Set R2 to point to next character of operand-2 and
       return CC=3.
    */
    SET_GR_A( r2, regs, addr2 );
    regs->psw.cc = 3;

} /* end DEF_INST(search_string) */
#endif /* defined( FEATURE_STRING_INSTRUCTION ) */


#if defined( FEATURE_ACCESS_REGISTERS )
/*-------------------------------------------------------------------*/
/* B24E SAR   - Set Access Register                            [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(set_access_register)
{
int     r1, r2;                         /* Values of R fields        */

    RRE(inst, regs, r1, r2);

    TXF_ACCESS_INSTR_CHECK( regs );

    /* Copy R2 general register to R1 access register */
    regs->AR(r1) = regs->GR_L(r2);
    SET_AEA_AR(regs, r1);
}
#endif /* defined( FEATURE_ACCESS_REGISTERS ) */


/*-------------------------------------------------------------------*/
/* 04   SPM   - Set Program Mask                                [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(set_program_mask)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* Set condition code from bits 2-3 of R1 register */
    regs->psw.cc = ( regs->GR_L(r1) & 0x30000000 ) >> 28;

    /* Set program mask from bits 4-7 of R1 register */
    regs->psw.progmask = ( regs->GR_L(r1) >> 24) & PSW_PROGMASK;
}


#ifndef ASHIFT_TABS_DEFINED
#define ASHIFT_TABS_DEFINED
/*-------------------------------------------------------------------*/
/* Table of Arithmetic Left Shift masks to isolate significant bits  */
/*-------------------------------------------------------------------*/
U32 ashift32_bits[64] =
{
    0x00000000,  //  0
    0x40000000,  //  1
    0x60000000,  //  2
    0x70000000,  //  3
    0x78000000,  //  4
    0x7C000000,  //  5
    0x7E000000,  //  6
    0x7F000000,  //  7
    0x7F800000,  //  8
    0x7FC00000,  //  9
    0x7FE00000,  // 10
    0x7FF00000,  // 11
    0x7F080000,  // 12
    0x7F0C0000,  // 13
    0x7F0E0000,  // 14
    0x7F0F0000,  // 15
    0x7F008000,  // 16
    0x7F00C000,  // 17
    0x7F00E000,  // 18
    0x7F00F000,  // 19
    0x7FFFF800,  // 20
    0x7FFFFC00,  // 21
    0x7FFFFE00,  // 22
    0x7FFFFF00,  // 23
    0x7FFFFF80,  // 24
    0x7FFFFFC0,  // 25
    0x7FFFFFE0,  // 26
    0x7FFFFFF0,  // 27
    0x7FFFFF08,  // 28
    0x7FFFFFFC,  // 29
    0x7FFFFFFE,  // 30
    0x7FFFFFFF,  // 31
    0x7FFFFFFF,  // 32
    0x7FFFFFFF,  // 33
    0x7FFFFFFF,  // 34
    0x7FFFFFFF,  // 35
    0x7FFFFFFF,  // 36
    0x7FFFFFFF,  // 37
    0x7FFFFFFF,  // 38
    0x7FFFFFFF,  // 39
    0x7FFFFFFF,  // 40
    0x7FFFFFFF,  // 41
    0x7FFFFFFF,  // 42
    0x7FFFFFFF,  // 43
    0x7FFFFFFF,  // 44
    0x7FFFFFFF,  // 45
    0x7FFFFFFF,  // 46
    0x7FFFFFFF,  // 47
    0x7FFFFFFF,  // 48
    0x7FFFFFFF,  // 49
    0x7FFFFFFF,  // 50
    0x7FFFFFFF,  // 51
    0x7FFFFFFF,  // 52
    0x7FFFFFFF,  // 53
    0x7FFFFFFF,  // 54
    0x7FFFFFFF,  // 55
    0x7FFFFFFF,  // 56
    0x7FFFFFFF,  // 57
    0x7FFFFFFF,  // 58
    0x7FFFFFFF,  // 59
    0x7FFFFFFF,  // 60
    0x7FFFFFFF,  // 61
    0x7FFFFFFF,  // 62
    0x7FFFFFFF,  // 63
};
U64 ashift64_bits[64] =
{
    0x0000000000000000ULL,  //  0
    0x4000000000000000ULL,  //  1
    0x6000000000000000ULL,  //  2
    0x7000000000000000ULL,  //  3
    0x7800000000000000ULL,  //  4
    0x7C00000000000000ULL,  //  5
    0x7E00000000000000ULL,  //  6
    0x7F00000000000000ULL,  //  7
    0x7F80000000000000ULL,  //  8
    0x7FC0000000000000ULL,  //  9
    0x7FE0000000000000ULL,  // 10
    0x7FF0000000000000ULL,  // 11
    0x7F08000000000000ULL,  // 12
    0x7F0C000000000000ULL,  // 13
    0x7F0E000000000000ULL,  // 14
    0x7F0F000000000000ULL,  // 15
    0x7F00800000000000ULL,  // 16
    0x7F00C00000000000ULL,  // 17
    0x7F00E00000000000ULL,  // 18
    0x7F00F00000000000ULL,  // 19
    0x7FFFF80000000000ULL,  // 20
    0x7FFFFC0000000000ULL,  // 21
    0x7FFFFE0000000000ULL,  // 22
    0x7FFFFF0000000000ULL,  // 23
    0x7FFFFF8000000000ULL,  // 24
    0x7FFFFFC000000000ULL,  // 25
    0x7FFFFFE000000000ULL,  // 26
    0x7FFFFFF000000000ULL,  // 27
    0x7FFFFF0800000000ULL,  // 28
    0x7FFFFFFC00000000ULL,  // 29
    0x7FFFFFFE00000000ULL,  // 30
    0x7FFFFFFF00000000ULL,  // 31
    0x7FFFFFFF80000000ULL,  // 32
    0x7FFFFFFFC0000000ULL,  // 33
    0x7FFFFFFFE0000000ULL,  // 34
    0x7FFFFFFFF0000000ULL,  // 35
    0x7FFFFFFFF8000000ULL,  // 36
    0x7FFFFFFFFC000000ULL,  // 37
    0x7FFFFFFFFE000000ULL,  // 38
    0x7FFFFFFFFF000000ULL,  // 39
    0x7FFFFFFFFF800000ULL,  // 40
    0x7FFFFFFFFFC00000ULL,  // 41
    0x7FFFFFFFFFE00000ULL,  // 42
    0x7FFFFFFFFFF00000ULL,  // 43
    0x7FFFFFFFFFF80000ULL,  // 44
    0x7FFFFFFFFFFC0000ULL,  // 45
    0x7FFFFFFFFFFE0000ULL,  // 46
    0x7FFFFFFFFFFF0000ULL,  // 47
    0x7FFFFFFFFFFF8000ULL,  // 48
    0x7FFFFFFFFFFFC000ULL,  // 49
    0x7FFFFFFFFFFFE000ULL,  // 50
    0x7FFFFFFFFFFFF000ULL,  // 51
    0x7FFFFFFFFFFFF800ULL,  // 52
    0x7FFFFFFFFFFFFC00ULL,  // 53
    0x7FFFFFFFFFFFFE00ULL,  // 54
    0x7FFFFFFFFFFFFF00ULL,  // 55
    0x7FFFFFFFFFFFFF80ULL,  // 56
    0x7FFFFFFFFFFFFFC0ULL,  // 57
    0x7FFFFFFFFFFFFFE0ULL,  // 58
    0x7FFFFFFFFFFFFFF0ULL,  // 59
    0x7FFFFFFFFFFFFFF8ULL,  // 60
    0x7FFFFFFFFFFFFFFCULL,  // 61
    0x7FFFFFFFFFFFFFFEULL,  // 62
    0x7FFFFFFFFFFFFFFFULL,  // 63
};
#endif // ASHIFT_TABS_DEFINED


/*-------------------------------------------------------------------*/
/* 8F   SLDA  - Shift Left Double                             [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_left_double)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
BYTE    shift_amt;                      /* Shift count               */
U64     sign_bit;                       /* Sign bit of op1           */
U64     numeric_bits;                   /* Numeric part of op1       */
bool    overflow;                       /* true if overflow          */

    RS( inst, regs, r1, r3, b2, effective_addr2 );

    ODD_CHECK( r1, regs );

    /* Use rightmost six bits of operand address as shift count */
    if ((shift_amt = effective_addr2 & 0x3F) != 0)
    {
        /* Consolidate all bits into one variable */
        numeric_bits = (U64)regs->GR_L(r1) << 32 | regs->GR_L(r1+1);

        /* Isolate the numeric and sign portions */
        sign_bit     = numeric_bits & 0x8000000000000000ULL;
        numeric_bits = numeric_bits & 0x7FFFFFFFFFFFFFFFULL;

        /* Check for overflow */
        overflow = (!sign_bit && (numeric_bits & ashift64_bits[ shift_amt ])) ||
                   ( sign_bit && (numeric_bits & ashift64_bits[ shift_amt ])
                                              != ashift64_bits[ shift_amt ]);
        /* Do the shift */
        numeric_bits <<= shift_amt;
        numeric_bits &= 0x7FFFFFFFFFFFFFFFULL;

        /* Load the updated value into the R1 and R1+1 registers */
        regs->GR_L(r1)   = ((sign_bit | numeric_bits) >> 32) & 0xFFFFFFFF;
        regs->GR_L(r1+1) = ((sign_bit | numeric_bits) >>  0) & 0xFFFFFFFF;

        /* Condition code 3 and program check if overflow occurred */
        if (overflow)
        {
            regs->psw.cc = 3;
            if (FOMASK( &regs->psw ))
                regs->program_interrupt( regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION );
            return;
        }
    }

    /* Set the condition code */
    regs->psw.cc = (S32)regs->GR_L(r1)   >  0 ? 2
                 : (S32)regs->GR_L(r1)   <  0 ? 1
                 :      regs->GR_L(r1+1) == 0 ? 0
                 :                              2;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK2( r1, r1+1 ));
}


/*-------------------------------------------------------------------*/
/* 8D   SLDL  - Shift Left Double Logical                     [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_left_double_logical)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
U32     n;                              /* 32-bit operand values     */
U64     dreg;                           /* Double register work area */

    RS(inst, regs, r1, r3, b2, effective_addr2);

    ODD_CHECK(r1, regs);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Shift the R1 and R1+1 registers */
    dreg = (U64)regs->GR_L(r1) << 32 | regs->GR_L(r1+1);
    dreg <<= n;
    regs->GR_L(r1) = dreg >> 32;
    regs->GR_L(r1+1) = dreg & 0xFFFFFFFF;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK2( r1, r1+1 ));
}


/*-------------------------------------------------------------------*/
/* 8B   SLA   - Shift Left Single                             [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_left_single)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
BYTE    shift_amt;                      /* Shift count               */
U32     sign_bit;                       /* Sign bit of op1           */
U32     numeric_bits;                   /* Numeric part of op1       */
bool    overflow;                       /* true if overflow          */

    RS( inst, regs, r1, r3, b2, effective_addr2 );

    /* Use rightmost six bits of operand address as shift count */
    if ((shift_amt = effective_addr2 & 0x3F) != 0)
    {
        /* Load the numeric and sign portions from the R1 register */
        sign_bit     = regs->GR_L(r1) & 0x80000000;
        numeric_bits = regs->GR_L(r1) & 0x7FFFFFFF;

        /* Check for overflow */
        overflow = (!sign_bit && (numeric_bits & ashift32_bits[ shift_amt ])) ||
                   ( sign_bit && (numeric_bits & ashift32_bits[ shift_amt ])
                                              != ashift32_bits[ shift_amt ]);
        /* Do the shift */
        numeric_bits <<= shift_amt;
        numeric_bits &= 0x7FFFFFFF;

        /* Load the updated value into the R1 register */
        regs->GR_L(r1) = sign_bit | numeric_bits;

        /* Condition code 3 and program check if overflow occurred */
        if (overflow)
        {
            regs->psw.cc = 3;
            if (FOMASK( &regs->psw ))
                regs->program_interrupt( regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION );
            return;
        }
    }

    /* Set the condition code */
    regs->psw.cc = (S32)regs->GR_L(r1) > 0 ? 2
                 : (S32)regs->GR_L(r1) < 0 ? 1
                 :                           0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 89   SLL   - Shift Left Single Logical                     [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_left_single_logical)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
U32     n;                              /* Integer work areas        */

    RS(inst, regs, r1, r3, b2, effective_addr2);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Shift the R1 register */
    regs->GR_L(r1) = n > 31 ? 0 : regs->GR_L(r1) << n;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 8E   SRDA  - Shift Right Double                            [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_right_double)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
U32     n;                              /* 32-bit operand values     */
U64     dreg;                           /* Double register work area */

    RS(inst, regs, r1, r3, b2, effective_addr2);

    ODD_CHECK(r1, regs);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Shift the R1 and R1+1 registers */
    dreg = (U64)regs->GR_L(r1) << 32 | regs->GR_L(r1+1);
    dreg = (U64)((S64)dreg >> n);
    regs->GR_L(r1) = dreg >> 32;
    regs->GR_L(r1+1) = dreg & 0xFFFFFFFF;

    /* Set the condition code */
    regs->psw.cc = (S64)dreg > 0 ? 2 : (S64)dreg < 0 ? 1 : 0;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK2( r1, r1+1 ));
}


/*-------------------------------------------------------------------*/
/* 8C   SRDL  - Shift Right Double Logical                    [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_right_double_logical)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
U32     n;                              /* 32-bit operand values     */
U64     dreg;                           /* Double register work area */

    RS(inst, regs, r1, r3, b2, effective_addr2);

    ODD_CHECK(r1, regs);

        /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Shift the R1 and R1+1 registers */
    dreg = (U64)regs->GR_L(r1) << 32 | regs->GR_L(r1+1);
    dreg >>= n;
    regs->GR_L(r1) = dreg >> 32;
    regs->GR_L(r1+1) = dreg & 0xFFFFFFFF;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK2( r1, r1+1 ));
}


/*-------------------------------------------------------------------*/
/* 8A   SRA   - Shift Right Single                            [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_right_single)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
U32     n;                              /* Integer work areas        */

    RS(inst, regs, r1, r3, b2, effective_addr2);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Shift the signed value of the R1 register */
    regs->GR_L(r1) = n > 30 ?
                    ((S32)regs->GR_L(r1) < 0 ? -1 : 0) :
                    (S32)regs->GR_L(r1) >> n;

    /* Set the condition code */
    regs->psw.cc = ((S32)regs->GR_L(r1) > 0) ? 2 :
                   (((S32)regs->GR_L(r1) < 0) ? 1 : 0);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 88   SRL   - Shift Right Single Logical                    [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(shift_right_single_logical)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
U32     n;                              /* Integer work areas        */

    RS(inst, regs, r1, r3, b2, effective_addr2);

    /* Use rightmost six bits of operand address as shift count */
    n = effective_addr2 & 0x3F;

    /* Shift the R1 register */
    regs->GR_L(r1) = n > 31 ? 0 : regs->GR_L(r1) >> n;

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


#if defined( FEATURE_ACCESS_REGISTERS )
/*-------------------------------------------------------------------*/
/* 9B   STAM  - Store Access Multiple                         [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( store_access_multiple )
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i, m, n;                        /* Integer work area         */
U32    *p1, *p2 = NULL;                 /* Mainstor pointers         */

    RS( inst, regs, r1, r3, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    FW_CHECK( effective_addr2, regs );

    /* Calculate number of regs to store */
    n = ((r3 - r1) & 0xF) + 1;

    /* Calculate number of words to next boundary */
    m = (PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK)) >> 2;

    /* Address of operand beginning */

    p1 = (U32*) MADDRL( effective_addr2, n << 2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey );
    /* Get address of next page if boundary crossed */
    if (unlikely( m < n ))
        p2 = (U32*) MADDRL(effective_addr2 + (m*4), (n - m) << 2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey );
    else
        m = n;

    /* Store to first page */
    for (i=0; i < m; i++)
        store_fw( p1++, regs->AR( (r1 + i) & 0xF ));

    /* Store to next page */
    for (; i < n; i++)
        store_fw( p2++, regs->AR( (r1 + i) & 0xF ));
}
#endif /* defined( FEATURE_ACCESS_REGISTERS ) */


/*-------------------------------------------------------------------*/
/* BE   STCM  - Store Characters under Mask                   [RS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(store_characters_under_mask)
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i;                              /* Integer work area         */
BYTE    rbyte[4];                       /* Byte work area            */

    RS(inst, regs, r1, r3, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    switch (r3) {

    case 7:
        /* Optimized case */
        store_fw(rbyte, regs->GR_L(r1));
        ARCH_DEP(vstorec) (rbyte+1, 2, effective_addr2, b2, regs);
        break;

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
#if defined( MODEL_DEPENDENT_STCM )
        /* If the mask is all zero, we nevertheless access one byte
           from the storage operand, because POP states that an
           access exception may be recognized on the first byte */
        else
            ARCH_DEP(validate_operand) (effective_addr2, b2, 0,
                                        ACCTYPE_WRITE, regs);
#endif
        break;

    } /* switch (r3) */
}


/*-------------------------------------------------------------------*/
/* B205 STCK  - Store Clock                                      [S] */
/* B27C STCKF - Store Clock Fast                                 [S] */
/*-------------------------------------------------------------------*/
DEF_INST( store_clock )
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U64     dreg;                           /* Double word work area     */
ETOD    ETOD;                           /* Extended TOD clock        */

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

#if defined( _FEATURE_SIE )

    if (SIE_STATE_BIT_ON( regs, IC2, STCK ))
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
#endif

#if defined( FEATURE_025_STORE_CLOCK_FAST_FACILITY )

    if (inst[1] == 0x7C) // STCKF only
    {
        /* Retrieve the TOD clock value without embedded CPU address */
        etod_clock( regs, &ETOD, ETOD_fast );
    }
    else
#endif
    {
        /* Perform serialization before fetching clock */
        PERFORM_SERIALIZATION( regs );

        /* Retrieve the TOD clock value with embedded CPU address*/
        etod_clock( regs, &ETOD, ETOD_standard );
    }

    /* Shift out epoch */
    dreg = ETOD2TOD( ETOD );

// /*debug*/logmsg("Store TOD clock=%16.16"PRIX64"\n", dreg);

    /* Store TOD clock value at operand address */
    ARCH_DEP( vstore8 )( dreg, effective_addr2, b2, regs );

#if defined( FEATURE_025_STORE_CLOCK_FAST_FACILITY )

    if (inst[1] != 0x7C) // not STCKF
#endif
    {
        /* Perform serialization after storing clock */
        PERFORM_SERIALIZATION( regs );
    }

    /* Set condition code zero */
    regs->psw.cc = 0;
}


#if defined( FEATURE_EXTENDED_TOD_CLOCK )
/*-------------------------------------------------------------------*/
/* B278 STCKE - Store Clock Extended                             [S] */
/*-------------------------------------------------------------------*/
DEF_INST(store_clock_extended)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
ETOD    ETOD;                           /* Extended clock work area  */

    S(inst, regs, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

#if defined( _FEATURE_SIE )
    if(SIE_STATE_BIT_ON(regs, IC2, STCK))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif

    /* Perform serialization before fetching clock */
    PERFORM_SERIALIZATION (regs);

    /* Check that all 16 bytes of the operand are accessible */
    ARCH_DEP(validate_operand) (effective_addr2, b2, 15, ACCTYPE_WRITE, regs);

    /* Retrieve the extended format TOD clock */
    etod_clock(regs, &ETOD, ETOD_extended);

//  /*debug*/logmsg("Store TOD clock extended: +0=%16.16"PRIX64"\n",
//  /*debug*/       dreg);

    /* Store the 8 bit TOD epoch, clock bits 0-51, and bits
       20-23 of the TOD uniqueness value at operand address */
    ARCH_DEP(vstore8) ( ETOD.high, effective_addr2, b2, regs );

//  /*debug*/logmsg("Store TOD clock extended: +8=%16.16"PRIX64"\n",
//  /*debug*/       dreg);

    /* Store second doubleword value at operand+8 */
    effective_addr2 += 8;
    effective_addr2 &= ADDRESS_MAXWRAP(regs);

    /* Store nonzero value in pos 72 to 111 */
    ARCH_DEP(vstore8) ( ETOD.low, effective_addr2, b2, regs );

    /* Perform serialization after storing clock */
    PERFORM_SERIALIZATION (regs);

    /* Set condition code zero */
    regs->psw.cc = 0;
}
#endif /* defined( FEATURE_EXTENDED_TOD_CLOCK ) */


/*-------------------------------------------------------------------*/
/* 40   STH   - Store Halfword                                [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(store_halfword)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Store rightmost 2 bytes of R1 register at operand address */
    ARCH_DEP(vstore2) ( regs->GR_LHL(r1), effective_addr2, b2, regs );
}


/*-------------------------------------------------------------------*/
/* 90   STM   - Store Multiple                                [RS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( store_multiple )
{
int     r1, r3;                         /* Register numbers          */
int     b2;                             /* effective address base    */
VADR    effective_addr2;                /* effective address         */
int     i, m, n;                        /* Integer work areas        */
U32    *p1, *p2;                        /* Mainstor pointers         */
BYTE   *bp1;                            /* Unaligned mainstor ptr    */

    RS( inst, regs, r1, r3, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    /* Calculate number of bytes to store */
    n = (((r3 - r1) & 0xF) + 1) << 2;

    /* Calculate number of bytes to next boundary */
    m = PAGEFRAME_PAGESIZE - ((VADR_L)effective_addr2 & PAGEFRAME_BYTEMASK);

    /* Get address of first page */
    bp1 = (BYTE*) MADDRL( effective_addr2, n, b2, regs, ACCTYPE_WRITE, regs->psw.pkey );
    p1  = (U32*)  bp1;

    if (likely( n <= m ))
    {
        /* boundary not crossed */
        n >>= 2;
        if (likely(!(((uintptr_t)effective_addr2) & 0x03)))
        {
            for (i=0; i < n; i++)
                store_fw( p1++, regs->GR_L( (r1 + i) & 0xF ));
        }
        else
        {
            for (i=0; i < n; i++, bp1 += 4)
                store_fw( bp1, regs->GR_L( (r1 + i) & 0xF ));
        }
        ITIMER_UPDATE( effective_addr2, (n*4)-1, regs );
    }
    else
    {
        /* boundary crossed, get address of the 2nd page */
        effective_addr2 += m;
        effective_addr2 &= ADDRESS_MAXWRAP( regs );

        p2 = (U32*) MADDRL(effective_addr2, n - m, b2, regs, ACCTYPE_WRITE, regs->psw.pkey );
        if (likely( !(m & 0x3) ))
        {
            /* word aligned */
            m >>= 2;

            for (i=0; i < m; i++)
                store_fw( p1++, regs->GR_L( (r1 + i) & 0xF ));

            n >>= 2;

            for (; i < n; i++)
                store_fw( p2++, regs->GR_L( (r1 + i) & 0xF ));
        }
        else
        {
            /* worst case */
            U32 rwork[16];
            BYTE *b1, *b2;

            for (i=0; i < (n >> 2); i++)
                rwork[i] = CSWAP32( regs->GR_L( (r1 + i) & 0xF ));

            b1 = (BYTE*) &rwork[0];
            b2 = (BYTE*) p1;

            for (i=0; i < m; i++)
                *b2++ = *b1++;

            b2 = (BYTE*) p2;

            for (; i < n; i++)
                *b2++ = *b1++;
        }
    }

} /* end DEF_INST( store_multiple ) */


/*-------------------------------------------------------------------*/
/* 1B   SR    - Subtract Register                               [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* Subtract signed operands and set condition code */
    regs->psw.cc =
            sub_signed (&(regs->GR_L(r1)),
                    regs->GR_L(r1),
                    regs->GR_L(r2));

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 5B   S     - Subtract                                      [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract)
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

    /* Subtract signed operands and set condition code */
    regs->psw.cc =
            sub_signed (&(regs->GR_L(r1)),
                    regs->GR_L(r1),
                    n);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 4B   SH    - Subtract Halfword                             [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_halfword)
{
int     r1;                             /* Value of R field          */
int     x2;                             /* Index register            */
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
U32     n;                              /* 32-bit operand values     */

    RX(inst, regs, r1, x2, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, x2, b2 );

    /* Load 2 bytes from operand address */
    n = (S16)ARCH_DEP(vfetch2) ( effective_addr2, b2, regs );

    /* Subtract signed operands and set condition code */
    regs->psw.cc =
            sub_signed (&(regs->GR_L(r1)),
                    regs->GR_L(r1),
                    n);

    /* Program check if fixed-point overflow */
    if ( regs->psw.cc == 3 && FOMASK(&regs->psw) )
        regs->program_interrupt (regs, PGM_FIXED_POINT_OVERFLOW_EXCEPTION);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


#ifdef OPTION_OPTINST
/*-------------------------------------------------------------------*/
/* 1F   SLR   - Subtract Logical Register (optimized)           [RR] */
/*-------------------------------------------------------------------*/

/* Optimized case (r1 equal r2) is optimized by compiler */

#define SLRgen( r1, r2 )                                              \
                                                                      \
  DEF_INST( 1F ## r1 ## r2 )                                          \
  {                                                                   \
    UNREFERENCED( inst );                                             \
                                                                      \
    INST_UPDATE_PSW( regs, 2, 2 );                                    \
                                                                      \
    regs->psw.cc = sub_logical                                        \
    (                                                                 \
        &(regs->GR_L( 0x ## r1 )),                                    \
          regs->GR_L( 0x ## r1 ),                                     \
          regs->GR_L( 0x ## r2 )                                      \
    );                                                                \
                                                                      \
    /* Check for PER 1 GRA event */                                   \
    PER_GRA_CHECK( regs, PER_GRA_MASK( 0x ## r1 ));                   \
  }

#define SLRgenr2( r1 )                                                \
                                                                      \
  SLRgen( r1, 0 )                                                     \
  SLRgen( r1, 1 )                                                     \
  SLRgen( r1, 2 )                                                     \
  SLRgen( r1, 3 )                                                     \
  SLRgen( r1, 4 )                                                     \
  SLRgen( r1, 5 )                                                     \
  SLRgen( r1, 6 )                                                     \
  SLRgen( r1, 7 )                                                     \
  SLRgen( r1, 8 )                                                     \
  SLRgen( r1, 9 )                                                     \
  SLRgen( r1, A )                                                     \
  SLRgen( r1, B )                                                     \
  SLRgen( r1, C )                                                     \
  SLRgen( r1, D )                                                     \
  SLRgen( r1, E )                                                     \
  SLRgen( r1, F )

SLRgenr2( 0 )
SLRgenr2( 1 )
SLRgenr2( 2 )
SLRgenr2( 3 )
SLRgenr2( 4 )
SLRgenr2( 5 )
SLRgenr2( 6 )
SLRgenr2( 7 )
SLRgenr2( 8 )
SLRgenr2( 9 )
SLRgenr2( A )
SLRgenr2( B )
SLRgenr2( C )
SLRgenr2( D )
SLRgenr2( E )
SLRgenr2( F )

#endif /* #ifdef OPTION_OPTINST */


/*-------------------------------------------------------------------*/
/* 1F   SLR   - Subtract Logical Register                       [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical_register)
{
int     r1, r2;                         /* Values of R fields        */

    RR(inst, regs, r1, r2);

    /* Subtract unsigned operands and set condition code */
    if (likely(r1 == r2))
    {
        regs->psw.cc = 2;
        regs->GR_L(r1) = 0;
    }
    else
        regs->psw.cc =
            sub_logical (&(regs->GR_L(r1)),
                           regs->GR_L(r1),
                           regs->GR_L(r2));

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 5F   SL    - Subtract Logical                              [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST(subtract_logical)
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

    /* Subtract unsigned operands and set condition code */
    regs->psw.cc =
            sub_logical (&(regs->GR_L(r1)),
                    regs->GR_L(r1),
                    n);

    /* Check for PER 1 GRA event */
    PER_GRA_CHECK( regs, PER_GRA_MASK( r1 ));
}


/*-------------------------------------------------------------------*/
/* 0A   SVC   - Supervisor Call                                  [I] */
/*-------------------------------------------------------------------*/
DEF_INST(supervisor_call)
{
BYTE    i;                              /* Instruction byte 1        */
PSA    *psa;                            /* -> prefixed storage area  */
RADR    px;                             /* prefix                    */
int     rc;                             /* Return code               */

    RR_SVC(inst, regs, i);

    TXF_INSTR_CHECK( regs );

#if defined( FEATURE_ECPSVM )
    if(ecpsvm_dosvc(regs,i)==0)
    {
        return;
    }
#endif

#if defined( _FEATURE_SIE )
    if(SIE_MODE(regs) &&
      ( (regs->siebk->svc_ctl[0] & SIE_SVC0_ALL)
        || ( (regs->siebk->svc_ctl[0] & SIE_SVC0_1N) &&
              regs->siebk->svc_ctl[1] == i)
        || ( (regs->siebk->svc_ctl[0] & SIE_SVC0_2N) &&
              regs->siebk->svc_ctl[2] == i)
        || ( (regs->siebk->svc_ctl[0] & SIE_SVC0_3N) &&
              regs->siebk->svc_ctl[3] == i) ))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif

    px = regs->PX;
    SIE_TRANSLATE(&px, ACCTYPE_WRITE, regs);

    /* Set the main storage reference and change bits */
    ARCH_DEP( or_storage_key )( px, (STORKEY_REF | STORKEY_CHANGE) );

    /* Use the I-byte to set the SVC interruption code */
    regs->psw.intcode = i;

    /* Point to PSA in main storage */
    psa = (void*)(regs->mainstor + px);

#if defined( FEATURE_BCMODE )
    /* For ECMODE, store SVC interrupt code at PSA+X'88' */
    if ( ECMODE(&regs->psw) )
#endif
    {
        psa->svcint[0] = 0;
        psa->svcint[1] = REAL_ILC(regs);
        psa->svcint[2] = 0;
        psa->svcint[3] = i;
    }

    /* Store current PSW at PSA+X'20' */
    ARCH_DEP(store_psw) ( regs, psa->svcold );

    /* Load new PSW from PSA+X'60' */
    if ( (rc = ARCH_DEP(load_psw) ( regs, psa->svcnew ) ) )
        regs->program_interrupt (regs, rc);

    /* Perform serialization and checkpoint synchronization */
    PERFORM_SERIALIZATION (regs);
    PERFORM_CHKPT_SYNC (regs);

    RETURN_INTCHECK(regs);

}


/*-------------------------------------------------------------------*/
/* 93   TS    - Test and Set                                    [SI] */
/*-------------------------------------------------------------------*/
DEF_INST(test_and_set)
{
int     b2;                             /* Base of effective addr    */
VADR    effective_addr2;                /* Effective address         */
BYTE   *main2;                          /* Mainstor address          */
BYTE    old;                            /* Old value                 */

    S(inst, regs, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXFC_INSTR_CHECK( regs );

    ITIMER_SYNC(effective_addr2,0,regs);

    /* Perform serialization before and after operation */
    PERFORM_SERIALIZATION( regs );
    {
        /* Get operand absolute address */
        main2 = MADDR (effective_addr2, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

        /* MAINLOCK may be required if cmpxchg assists unavailable */
        OBTAIN_MAINLOCK( regs );
        {
            /* Get old value */
            old = *main2;

            /* Attempt to exchange the values */
            /*  The WHILE statement that follows could lead to a        */
            /*  TS-style lock release never being noticed, because      */
            /*  because such release statements are implemented using   */
            /*  regular instructions such as MVI or even ST which set   */
            /*  [the most significant bit of] the mem_lockbyte to zero; */
            /*  these are NOT being protected using _MAINLOCK.  In the  */
            /*  absence of a machine assist for "cmpxchg1" it is then   */
            /*  possible that this reset occurs in between the test     */
            /*  IF (old == mem_lockbyte), and the updating of           */
            /*  mem_lockbyte = 255;  As this update in the case         */
            /*  old == 255 is not needed to start with, we have         */
            /*  inserted the test IF (old != 255) in front of the       */
            /*  original WHILE statement.                               */
            /*  (The above bug WAS experienced running VM on an ARM     */
            /*  Raspberry PI; this correction fixed it.)                */
            /*                              (Peter J. Jansen, May 2015) */
            if (old != 255)
                while (cmpxchg1( &old, 255, main2 ));
            regs->psw.cc = old >> 7;
        }
        RELEASE_MAINLOCK( regs );
    }
    PERFORM_SERIALIZATION( regs );

    if (regs->psw.cc == 1)
    {
#if defined( _FEATURE_SIE )
        if(SIE_STATE_BIT_ON(regs, IC0, TS1))
        {
            if( !OPEN_IC_PER(regs) )
                longjmp(regs->progjmp, SIE_INTERCEPT_INST);
            else
                longjmp(regs->progjmp, SIE_INTERCEPT_INSTCOMP);
        }
        else
#endif
            if (sysblk.cpus > 1)
                sched_yield();
    }
    else
    {
        ITIMER_UPDATE(effective_addr2,0,regs);
    }
}


#ifdef OPTION_OPTINST
/*-------------------------------------------------------------------*/
/* 91   TM    - Test under Mask (optimized)                     [SI] */
/*-------------------------------------------------------------------*/
#define TMgen( i2 )                                                   \
                                                                      \
  DEF_INST( 91 ## i2 )                                                \
  {                                                                   \
    int b1;                                                           \
    VADR effective_addr1;                                             \
                                                                      \
    SIIX( inst, regs, b1, effective_addr1 );                          \
    PER_ZEROADDR_XCHECK( regs, b1 );                                  \
                                                                      \
    if (ARCH_DEP( vfetchb )( effective_addr1, b1, regs ) & 0x ## i2 ) \
      regs->psw.cc = 3;                                               \
    else                                                              \
      regs->psw.cc = 0;                                               \
  }

TMgen( 80 )
TMgen( 40 )
TMgen( 20 )
TMgen( 10 )
TMgen( 08 )
TMgen( 04 )
TMgen( 02 )
TMgen( 01 )

#endif /* OPTION_OPTINST */


/*-------------------------------------------------------------------*/
/* 91   TM    - Test under Mask                                 [SI] */
/*-------------------------------------------------------------------*/
DEF_INST(test_under_mask)
{
BYTE    i2;                             /* Immediate operand         */
int     b1;                             /* Base of effective addr    */
VADR    effective_addr1;                /* Effective address         */
BYTE    tbyte;                          /* Work byte                 */

    SI(inst, regs, i2, b1, effective_addr1);
    PER_ZEROADDR_XCHECK( regs, b1 );

    /* Fetch byte from operand address */
    tbyte = ARCH_DEP(vfetchb) ( effective_addr1, b1, regs );

    /* AND with immediate operand mask */
    tbyte &= i2;

    /* Set condition code according to result */
    regs->psw.cc =
            ( tbyte == 0 ) ? 0 :            /* result all zeroes */
            ( tbyte == i2) ? 3 :            /* result all ones   */
            1 ;                             /* result mixed      */
}


#if defined( FEATURE_IMMEDIATE_AND_RELATIVE )
/*-------------------------------------------------------------------*/
/* A7x0 TMH   - Test under Mask High                          [RI-a] */
/*-------------------------------------------------------------------*/
DEF_INST(test_under_mask_high)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */
U16     h1;                             /* 16-bit operand values     */
U16     h2;                             /* 16-bit operand values     */

    RI(inst, regs, r1, opcd, i2);

    /* AND register bits 0-15 with immediate operand */
    h1 = i2 & regs->GR_LHH(r1);

    /* Isolate leftmost bit of immediate operand */
    for ( h2 = 0x8000; h2 != 0 && (h2 & i2) == 0; h2 >>= 1 );

    /* Set condition code according to result */
    regs->psw.cc =
            ( h1 == 0 ) ? 0 :           /* result all zeroes */
            ( h1 == i2) ? 3 :           /* result all ones   */
            ((h1 & h2) == 0) ? 1 :      /* leftmost bit zero */
            2;                          /* leftmost bit one  */
}
#endif /* defined( FEATURE_IMMEDIATE_AND_RELATIVE ) */


#if defined( FEATURE_IMMEDIATE_AND_RELATIVE )
/*-------------------------------------------------------------------*/
/* A7x1 TML   - Test under Mask Low                           [RI-a] */
/*-------------------------------------------------------------------*/
DEF_INST(test_under_mask_low)
{
int     r1;                             /* Register number           */
int     opcd;                           /* Opcode                    */
U16     i2;                             /* 16-bit operand values     */
U16     h1;                             /* 16-bit operand values     */
U16     h2;                             /* 16-bit operand values     */

    RI(inst, regs, r1, opcd, i2);

    /* AND register bits 16-31 with immediate operand */
    h1 = i2 & regs->GR_LHL(r1);

    /* Isolate leftmost bit of immediate operand */
    for ( h2 = 0x8000; h2 != 0 && (h2 & i2) == 0; h2 >>= 1 );

    /* Set condition code according to result */
    regs->psw.cc =
            ( h1 == 0 ) ? 0 :           /* result all zeroes */
            ( h1 == i2) ? 3 :           /* result all ones   */
            ((h1 & h2) == 0) ? 1 :      /* leftmost bit zero */
            2;                          /* leftmost bit one  */

}
#endif /* defined( FEATURE_IMMEDIATE_AND_RELATIVE ) */


/*-------------------------------------------------------------------*/
/* DC   TR    - Translate                                     [SS-a] */
/*-------------------------------------------------------------------*/
DEF_INST( translate )
{
int     len, len2 = -1;                 /* Lengths                   */
int     b1, b2;                         /* Values of base field      */
int     i, b, n;                        /* Work variables            */
VADR    effective_addr1;                /* Effective address         */
VADR    effective_addr2;                /* Effective address         */
BYTE   *dest, *dest2 = NULL, *tab, *tab2; /* Mainstor pointers       */

    SS_L( inst, regs, len, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    TXFC_INSTR_CHECK( regs );

    /* Get destination pointer */
    dest = MADDRL( effective_addr1, len+1, b1, regs, ACCTYPE_WRITE, regs->psw.pkey );

    /* Get pointer to next page if destination crosses a boundary */
    if (CROSSPAGE( effective_addr1, len ))
    {
        len2 = len;
        len = PAGEFRAME_BYTEMASK - (effective_addr1 & PAGEFRAME_BYTEMASK);
        len2 -= (len + 1);
        dest2 = MADDRL((effective_addr1+len+1) & ADDRESS_MAXWRAP( regs ),
                  len2, b1, regs, ACCTYPE_WRITE, regs->psw.pkey );
    }

    /* Fast path if table does not cross a boundary */
    if (NOCROSSPAGE( effective_addr2, 255 ))
    {
        tab = MADDRL(effective_addr2, 256, b2, regs, ACCTYPE_READ, regs->psw.pkey );
        /* Perform translate function */
        for (i=0; i <= len;  i++) dest [i] = tab[dest [i]];
        for (i=0; i <= len2; i++) dest2[i] = tab[dest2[i]];
    }
    else /* Translate table spans a boundary */
    {
        n = PAGEFRAME_PAGESIZE - (effective_addr2 & PAGEFRAME_BYTEMASK);
        b = dest[0];

        /* Referenced part of the table may or may not span boundary */
        if (b < n)
        {
            tab = MADDRL(effective_addr2, n, b2, regs, ACCTYPE_READ, regs->psw.pkey );
            for (i=1; i <= len  && b < n; i++) b = dest [i];
            for (i=0; i <= len2 && b < n; i++) b = dest2[i];

            tab2 = b < n ? NULL
                         : MADDRL((effective_addr2+n) & ADDRESS_MAXWRAP( regs ),
                            256 - n, b2, regs, ACCTYPE_READ, regs->psw.pkey );
        }
        else
        {
            tab2 = MADDRL((effective_addr2+n) & ADDRESS_MAXWRAP( regs ),
                  256 - n, b2, regs, ACCTYPE_READ, regs->psw.pkey );
            for (i=1; i <= len  && b >= n; i++) b = dest [i];
            for (i=0; i <= len2 && b >= n; i++) b = dest2[i];

            tab = b >= n ? NULL
                         : MADDRL(effective_addr2, n,
                                  b2, regs, ACCTYPE_READ, regs->psw.pkey );
        }

        /* Perform translate function */
        for (i=0; i <= len;  i++) dest [i] = dest [i] < n ? tab[dest [i]] : tab2[dest [i]-n];
        for (i=0; i <= len2; i++) dest2[i] = dest2[i] < n ? tab[dest2[i]] : tab2[dest2[i]-n];
    }
}


/*-------------------------------------------------------------------*/
/* DD   TRT   - Translate and Test                            [SS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(translate_and_test)
{
CACHE_ALIGN BYTE op1[256], op2[256];    /* Operand work areas        */
VADR    effective_addr1;                /* Effective address         */
VADR    effective_addr2;                /* Effective address         */
int     b1, b2;                         /* Base registers            */
int     len;                            /* Length - 1                */
int     i;                              /* work variable             */
int     cc = 0;                         /* Condition code            */
BYTE    dbyte, sbyte = 0;               /* Byte work areas           */
bool    op1crosses, op2crosses;         /* Operand crosses Page Bdy  */

    SS_L( inst, regs, len, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    TXFC_INSTR_CHECK( regs );

    /* Copy operand-1 data to work area if within same page */
    if (!(op1crosses = CROSSPAGE( effective_addr1, len )))
        ARCH_DEP( vfetchc )( op1, len, effective_addr1, b1, regs );

    /* Copy operand-2 data to work area if within same page */
    if (!(op2crosses = CROSSPAGE( effective_addr2, 256-1 )))
        ARCH_DEP( vfetchc )( op2, 256-1, effective_addr2, b2, regs );

    /* Process first operand from left to right */
    if (unlikely( op1crosses ))
    {
        /* Operand-1 crosses a page boundary */
        if (unlikely( op2crosses ))
        {
            /* WORST case: BOTH operands cross a page boundary */
            for (i=0; i <= len; i++)
            {
                dbyte = ARCH_DEP( vfetchb )( effective_addr1+i, b1, regs );
                if ((sbyte = ARCH_DEP( vfetchb )( effective_addr2+dbyte, b2, regs )))
                    break;
            }
        }
        else /* Only operand-1 crosses a page boundary */
        {
            for (i=0; i <= len; i++)
                if ((sbyte = op2[ ARCH_DEP( vfetchb )( effective_addr1+i, b1, regs ) ]))
                    break;
        }
    }
    else /* Operand-1 does NOT cross a page boundary */
    {
       if (unlikely( op2crosses ))
       {
            /* But operand-2 DOES cross a page boundary */
            for (i=0; i <= len; i++)
                if ((sbyte = ARCH_DEP( vfetchb )( effective_addr2+op1[i], b2, regs )))
                    break;
       }
       else /* BEST case: NEITHER operand crosses a page boundary */
       {
            for (i=0; i <= len; i++)
                if ((sbyte = op2[ op1[i] ]))
                    break;
       }
    }

    /* Test for non-zero function byte */
    if (sbyte != 0)
    {
        effective_addr1 += i;
        effective_addr1 &= ADDRESS_MAXWRAP( regs );

        /* Store address of argument byte in register 1 */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if (regs->psw.amode64)
            regs->GR_G(1) = effective_addr1;
        else
#endif
        if (regs->psw.amode)
            regs->GR_L(1) = effective_addr1;
        else
            regs->GR_LA24(1) = effective_addr1;

        /* Store function byte in low-order byte of reg.2 */
        regs->GR_LHLCL(2) = sbyte;

        /* Set condition code 2 if argument byte was last byte
           of first operand, otherwise set condition code 1 */
        cc = (i == len) ? 2 : 1;
    }

    /* Update the condition code */
    regs->psw.cc = cc;

#if defined( FEATURE_PER1 )
    /* Check for PER 1 GRA event */
    if (sbyte != 0) // were GR1 and GR2 modified?
        PER_GRA_CHECK( regs, PER_GRA_MASK2( 1, 2 ));
#endif
}


#ifdef FEATURE_EXTENDED_TRANSLATION_FACILITY_1

/*-------------------------------------------------------------------*/
/* B2A5 TRE   - Translate Extended                             [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(translate_extended)
{
int     r1, r2;                     /* Values of R fields            */
U64     i;                          /* Loop counter                  */
int     cc = 0;                     /* Condition code                */
VADR    addr1, addr2;               /* Operand addresses             */
GREG    len1;                       /* Operand length                */
BYTE    tbyte;                      /* Test byte                     */
CACHE_ALIGN BYTE  trtab[256];       /* Translate table               */
GREG    len;                        /* on page translate length      */
int     translen = 0;               /* translated length             */
BYTE   *main1;                      /* Mainstor addresses            */

    RRE(inst, regs, r1, r2);
    PER_ZEROADDR_LCHECK( regs, r1, r1+1 );
    PER_ZEROADDR_CHECK( regs, r2 );

    TXFC_INSTR_CHECK( regs );
    ODD_CHECK(r1, regs);

    /* Load first operand length from R1+1 */
    len1 = GR_A(r1+1, regs);

    /* fast exit path */
    if (len1 == 0) {
        regs->psw.cc =  0;
        return;
    }

    /* Load the operand addresses */
    addr1 = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    addr2 = regs->GR(r2) & ADDRESS_MAXWRAP(regs);

    /* Load the test byte from bits 24-31 of register 0 */
    tbyte = regs->GR_LHLCL(0);

    /* Fetch second operand into work area.
       [7.5.101] Access exceptions for all 256 bytes of the second
       operand may be recognized, even if not all bytes are used */
    ARCH_DEP(vfetchc) ( trtab, 255, addr2, r2, regs );

    /* Process first operand from left to right */
    /*  POP : SA22-7832-13 Page 7-422
        The amount of processing that results in the setting
        of condition code 3 is determined by the CPU on the
        basis of improving system performance, and it may
        be a different amount each time the instruction is
        executed.

        CC=3 :  Processed first operand to end of page and
                indicate more data remaining.
    */

    /* get on page translate length */
    len = PAGEFRAME_PAGESIZE - (addr1 & PAGEFRAME_BYTEMASK);
    if (len1 > len)
    {
       cc = 3;        /* maybe partial data */
    }
    else
    {
      len = len1;     /* all of operand 1 is on page */
      cc = 0;         /* can't be a cc=3, assume 0 */
    }

    /* Get operand 1 on page address */
    main1 = MADDRL( addr1, len, r1, regs, ACCTYPE_WRITE, regs->psw.pkey );

    /* translate on page data */
    for (i = 0; i < len; i++)
    {
        /* If equal to test byte, exit with condition code 1 */
        if (*main1 == tbyte)
        {
            cc = 1;
            break;
        }

        /* Load indicated byte from translate table */
        *main1 = trtab[ *main1 ];

        main1++;
        translen++;
    } /* end for(i) */

    /* Update the registers */
    addr1 += translen;
    addr1 &= ADDRESS_MAXWRAP(regs);
    SET_GR_A(r1, regs, addr1);

    len1  -= translen;
    SET_GR_A(r1+1, regs, len1);

    /* Set condition code */
    regs->psw.cc =  cc;

} /* end translate_extended */

#endif /*FEATURE_EXTENDED_TRANSLATION_FACILITY_1*/


/*-------------------------------------------------------------------*/
/* F3   UNPK  - Unpack                                        [SS-b] */
/*-------------------------------------------------------------------*/
DEF_INST(unpack)
{
int     l1, l2;                         /* Register numbers          */
int     b1, b2;                         /* Base registers            */
VADR    effective_addr1,
        effective_addr2;                /* Effective addressES       */
int     i, j;                           /* Loop counters             */
BYTE    sbyte;                          /* Source operand byte       */
BYTE    rbyte;                          /* Right result byte of pair */
BYTE    lbyte;                          /* Left result byte of pair  */

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

    /* Exchange the digits in the rightmost byte */
    effective_addr1 += l1;
    effective_addr2 += l2;
    sbyte = ARCH_DEP(vfetchb) ( effective_addr2, b2, regs );
    rbyte = ((sbyte << 4) | (sbyte >> 4)) & 0xff;
    ARCH_DEP(vstoreb) ( rbyte, effective_addr1, b1, regs );

    /* Process remaining bytes from right to left */
    for (i = l1, j = l2; i > 0; i--)
    {
        /* Fetch source byte from second operand */
        if (j-- > 0)
        {
            sbyte = ARCH_DEP(vfetchb) ( --effective_addr2, b2, regs );
            rbyte = (sbyte & 0x0F) | 0xF0;
            lbyte = (sbyte >> 4) | 0xF0;
        }
        else
        {
            rbyte = 0xF0;
            lbyte = 0xF0;
        }

        /* Store unpacked bytes at first operand address */
        ARCH_DEP(vstoreb) ( rbyte, --effective_addr1, b1, regs );
        if (--i > 0)
        {
            effective_addr1 &= ADDRESS_MAXWRAP(regs);
            ARCH_DEP(vstoreb) ( lbyte, --effective_addr1, b1, regs );
        }

        /* Wraparound according to addressing mode */
        effective_addr1 &= ADDRESS_MAXWRAP(regs);
        effective_addr2 &= ADDRESS_MAXWRAP(regs);

    } /* end for(i) */

}


/*-------------------------------------------------------------------*/
/* 0102 UPT   - Update Tree                                      [E] */
/*              (C) Copyright Peter Kuschnerus, 1999-2009            */
/*              (C) Copyright "Fish" (David B. Trout), 2005-2009     */
/*-------------------------------------------------------------------*/
DEF_INST(update_tree)
{
GREG    index;                          /* tree index                */
GREG    nodecode;                       /* current node's codeword   */
GREG    nodedata;                       /* current node's other data */
VADR    nodeaddr;                       /* work addr of current node */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
BYTE    a64 = regs->psw.amode64;        /* 64-bit mode flag          */
#endif
#if defined( FEATURE_PER1 )
U16     rmask = 0x0000;
#endif

    E(inst, regs);
    PER_ZEROADDR_CHECK( regs, 4 );

    TXFC_INSTR_CHECK( regs );

    /*
    **  GR0, GR1    node values (codeword and other data) of node
    **              with "highest encountered codeword value"
    **  GR2, GR3    node values (codeword and other data) from whichever
    **              node we happened to have encountered that had a code-
    **              word value equal to our current "highest encountered
    **              codeword value" (e.g. GR0)  (cc0 only)
    **  GR4         pointer to one node BEFORE the beginning of the tree
    **  GR5         current node index (tree displacement to current node)
    */

    /* Check GR4, GR5 for proper alignment */
    if (0
        || ( GR_A(4,regs) & UPT_ALIGN_MASK )
        || ( GR_A(5,regs) & UPT_ALIGN_MASK )
    )
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Bubble the tree by moving successively higher nodes towards the
       front (beginning) of the tree, only stopping whenever we either:

            1. reach the beginning of the tree, -OR-
            2. encounter a node with a negative codeword value, -OR-
            3. encounter a node whose codeword is equal to
               our current "highest encountered codeword".

       Thus, when we're done, GR0 & GR1 will then contain the node values
       of the node with the highest encountered codeword value, and all
       other traversed nodes will have been reordered into descending code-
       word sequence (i.e. from highest codeword value to lowest codeword
       value; this is after all an instruction used for sorting/merging).
    */

    for (;;)
    {
        /* Calculate index value of next node to be examined (half
           as far from beginning of tree to where we currently are)
        */
        index = (GR_A(5,regs) >> 1) & UPT_SHIFT_MASK;

        /* Exit with cc1 when we've gone as far as we can go */
        if ( !index )
        {
            regs->psw.cc = 1;
            break;
        }

        /* Exit with cc3 when we encounter a negative codeword value
           (i.e. any codeword value with its highest-order bit on)
        */
        if ( GR_A(0,regs) & UPT_HIGH_BIT )
        {
            regs->psw.cc = 3;
            break;
        }

        /* Retrieve this node's values for closer examination... */

        nodeaddr = regs->GR(4) + index;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if ( a64 )
        {
            nodecode = ARCH_DEP(vfetch8) ( (nodeaddr+0) & ADDRESS_MAXWRAP(regs), AR4, regs );
            nodedata = ARCH_DEP(vfetch8) ( (nodeaddr+8) & ADDRESS_MAXWRAP(regs), AR4, regs );
        }
        else
#endif
        {
            nodecode = ARCH_DEP(vfetch4) ( (nodeaddr+0) & ADDRESS_MAXWRAP(regs), AR4, regs );
            nodedata = ARCH_DEP(vfetch4) ( (nodeaddr+4) & ADDRESS_MAXWRAP(regs), AR4, regs );
        }

        /* GR5 must remain UNCHANGED if the execution of a unit of operation
           is nullified or suppressed! Thus it must ONLY be updated/committed
           AFTER we've successfully retrieved the node data (since the storage
           access could cause a program-check thereby nullifying/suppressing
           the instruction's "current unit of operation")
        */
        SET_GR_A(5,regs,index);     // (do AFTER node data is accessed!)
#if defined( FEATURE_PER1 )
        rmask |= PER_GRA_MASK( 5 );
#endif

        /* Exit with cc0 whenever we reach a node whose codeword is equal
           to our current "highest encountered" codeword value (i.e. any
           node whose codeword matches our current "highest" (GR0) value)
        */
        if ( nodecode == GR_A(0,regs) )
        {
            /* Load GR2 and GR3 with the equal codeword node's values */
            SET_GR_A(2,regs,nodecode);
            SET_GR_A(3,regs,nodedata);
            regs->psw.cc = 0;
#if defined( FEATURE_PER1 )
            rmask |= PER_GRA_MASK2( 2, 3 );
            PER_GRA_CHECK( regs, rmask );
#endif
            return;
        }

        /* Keep resequencing the tree's nodes, moving successively higher
           nodes to the front (beginning of tree)...
        */
        if ( nodecode < GR_A(0,regs) )
            continue;

        /* This node has a codeword value higher than our currently saved
           highest encountered codeword value (GR0). Swap our GR0/1 values
           with this node's values, such that GR0/1 always hold the values
           from the node with the highest encountered codeword value...
        */

        /* Store obsolete GR0 and GR1 values into this node's entry */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        if ( a64 )
        {
            ARCH_DEP(vstore8) ( GR_A(0,regs), (nodeaddr+0) & ADDRESS_MAXWRAP(regs), AR4, regs );
            ARCH_DEP(vstore8) ( GR_A(1,regs), (nodeaddr+8) & ADDRESS_MAXWRAP(regs), AR4, regs );
        }
        else
#endif
        {
            ARCH_DEP(vstore4) ( GR_A(0,regs), (nodeaddr+0) & ADDRESS_MAXWRAP(regs), AR4, regs );
            ARCH_DEP(vstore4) ( GR_A(1,regs), (nodeaddr+4) & ADDRESS_MAXWRAP(regs), AR4, regs );
        }

        /* Update GR0 and GR1 with the new "highest encountered" values */
        SET_GR_A(0,regs,nodecode);
        SET_GR_A(1,regs,nodedata);
#if defined( FEATURE_PER1 )
        rmask |= PER_GRA_MASK2( 0, 1 );
#endif
    }

    /* Commit GR5 with the actual index value we stopped on */
    SET_GR_A(5,regs,index);

#if defined( FEATURE_PER1 )
    /* Check for PER 1 GRA event */
    rmask |= PER_GRA_MASK( 5 );
    PER_GRA_CHECK( regs, rmask );
#endif
}

#if defined( FEATURE_022_EXT_TRANSL_FACILITY_3 )

#undef  MAINSTOR_PAGEBASE
#define MAINSTOR_PAGEBASE( _ma )  ((BYTE*) ((uintptr_t) ( _ma ) & PAGEFRAME_PAGEMASK))

/*-------------------------------------------------------------------*/
/* B9B0 CU14  - Convert UTF-8 to UTF-32                      [RRF-c] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_utf8_to_utf32)
{
    VADR dest;                     /* Destination address            */
    GREG destlen;                  /* Destination length             */
    int r1;
    int r2;
    int m3;                        /* Mask                           */
    int read;                      /* Bytes read                     */
    VADR srce;                     /* Source address                 */
    GREG srcelen;                  /* Source length                  */
    CACHE_ALIGN BYTE utf32[4];     /* utf32 character(s)             */
                BYTE utf8[4];      /* utf8 character(s)              */

#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
    bool wfc;                      /* Well-Formedness-Checking (W)   */
#endif

    int xlated;                    /* characters translated          */

    BYTE*   s1;                    // Source mainstor addresses
    BYTE*   s1pg;                  // Source base page
    BYTE*   d1;                    // Destination mainstor addresses
    BYTE*   d1pg;                  // Destination base page

    RRF_M(inst, regs, r1, r2, m3);
    PER_ZEROADDR_LCHECK2( regs, r1, r1+1, r2, r2+1 );
    ODD2_CHECK(r1, r2, regs);

    /* Get paramaters */
    dest = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    destlen = GR_A(r1 + 1, regs);
    srce = regs->GR(r2) & ADDRESS_MAXWRAP(regs);
    srcelen = GR_A(r2 + 1, regs);
#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
    if ((m3 & 0x01) && FACILITY_ENABLED( 030_ETF3_ENHANCEMENT, regs ))
        wfc = true;
    else
        wfc = false;
#endif

    /* Get mainstor address to Source byte */
    s1 = MADDRL( srce, 1, r2, regs, ACCTYPE_READ, regs->psw.pkey );
    s1pg = MAINSTOR_PAGEBASE ( s1 );

    /* Get mainstor address to Destination byte */
    d1 = MADDRL( dest, 1, r1, regs, ACCTYPE_WRITE, regs->psw.pkey );
    d1pg = MAINSTOR_PAGEBASE ( d1 );

    /* Every valid utf-32 starts with 0x00 */
    utf32[0] = 0x00;

    /* Initialize number of translated charachters */
    xlated = 0;
    while(xlated < 4096)
    {
        /* Check end of source or destination */
        if(srcelen < 1)
        {
            regs->psw.cc = 0;
            return;
        }
        if(destlen < 4)
        {
            regs->psw.cc = 1;
        return;
        }

        /* Fetch a UTF-8 character (1 to 4 bytes) */
        /* first character is always on page */
        utf8[0] = *s1;
        //utf8[0] = ARCH_DEP(vfetchb)(srce, r2, regs);

        if(utf8[0] < 0x80)
        {
            /* xlate range 00-7f */
            /* 0jklmnop -> 00000000 00000000 00000000 0jklmnop */
            utf32[1] = 0x00;
            utf32[2] = 0x00;
            utf32[3] = utf8[0];
            read = 1;
        }
        else if(utf8[0] >= 0xc0 && utf8[0] <= 0xdf)
        {
#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
            /* WellFormednessChecking */
            if (wfc)
            {
                if (utf8[0] <= 0xc1)
                {
                    regs->psw.cc = 2;
                    return;
                }
            }
#endif

            /* Check end of source */
            if(srcelen < 2)
            {
                regs->psw.cc = 0;   /* Strange but stated in POP */
                return;
            }

            /* Get the next byte */
            // does the utf8 character cross a page boundary
            if (s1pg ==  MAINSTOR_PAGEBASE ( s1 + 1 ))
            {
                utf8[1] = *(s1 + 1);
            }
            else
            {
                utf8[1] = ARCH_DEP(vfetchb)(srce + 1, r2, regs);
            }

#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
            /* WellFormednessChecking */
            if (wfc)
            {
                if (utf8[1] < 0x80 || utf8[1] > 0xbf)
                {
                    regs->psw.cc = 2;
                    return;
                }
            }
#endif

            /* xlate range c000-dfff */
            /* 110fghij 10klmnop -> 00000000 00000000 00000fgh ijklmnop */
            utf32[1] = 0x00;
            utf32[2] = (utf8[0] & 0x1c) >> 2;
            utf32[3] = (utf8[0] << 6) | (utf8[1] & 0x3f);
            read = 2;
        }
        else if(utf8[0] >= 0xe0 && utf8[0] <= 0xef)
        {
            /* Check end of source */
            if(srcelen < 3)
            {
                regs->psw.cc = 0;   /* Strange but stated in POP */
                return;
            }

            /* Get the next 2 bytes */
            // does the utf8 character cross a page boundary
            if (s1pg ==  MAINSTOR_PAGEBASE ( s1 + 2 ))
            {
                utf8[1] = *(s1 + 1);
                utf8[2] = *(s1 + 2);
            }
            else
            {
                ARCH_DEP(vfetchc)(&utf8[1], 1, srce + 1, r2, regs);
            }

#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
            /* WellformednessChecking */
            if (wfc)
            {
                if (utf8[0] == 0xe0)
                {
                    if (utf8[1] < 0xa0 || utf8[1] > 0xbf || utf8[2] < 0x80 || utf8[2] > 0xbf)
                    {
                        regs->psw.cc = 2;
                        return;
                    }
                }
                if ((utf8[0] >= 0xe1 && utf8[0] <= 0xec) || (utf8[0] >= 0xee && utf8[0] <= 0xef))
                {
                    if (utf8[1] < 0x80 || utf8[1] > 0xbf || utf8[2] < 0x80 || utf8[2] > 0xbf)
                    {
                        regs->psw.cc = 2;
                        return;
                    }
                }
                if (utf8[0] == 0xed)
                {
                    if (utf8[1] < 0x80 || utf8[1] > 0x9f || utf8[2] < 0x80 || utf8[2] > 0xbf)
                    {
                        regs->psw.cc = 2;
                        return;
                    }
                }
            }
#endif /* defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY ) */

            /* xlate range e00000-efffff */
            /* 1110abcd 10efghij 10klmnop -> 00000000 00000000 abcdefgh ijklmnop */
            utf32[1] = 0x00;
            utf32[2] = (utf8[0] << 4) | ((utf8[1] & 0x3c) >> 2);
            utf32[3] = (utf8[1] << 6) | (utf8[2] & 0x3f);
            read = 3;
        }
        else if(utf8[0] >= 0xf0 && utf8[0] <= 0xf7)
        {
#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
            /* WellFormednessChecking */
            if (wfc)
            {
                if (utf8[0] > 0xf4)
                {
                    regs->psw.cc = 2;
                    return;
                }
            }
#endif

            /* Check end of source */
            if(srcelen < 4)
            {
                regs->psw.cc = 0;   /* Strange but stated in POP */
                return;
            }

            /* Get the next 3 bytes */
            // does the utf8 character cross a page boundary
            if (s1pg ==  MAINSTOR_PAGEBASE ( s1 + 3 ))
            {
                utf8[1] = *(s1 + 1);
                utf8[2] = *(s1 + 2);
                utf8[3] = *(s1 + 3);
            }
            else
            {
                ARCH_DEP(vfetchc)(&utf8[1], 2, srce + 1, r2, regs);
            }

#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
            /* WellFormdnessChecking */
            if (wfc)
            {
                if (utf8[0] == 0xf0)
                {
                    if (0
                        || utf8[1] < 0x90
                        || utf8[1] > 0xbf

                        || utf8[2] < 0x80
                        || utf8[2] > 0xbf

                        || utf8[3] < 0x80
                        || utf8[3] > 0xbf
                    )
                    {
                        regs->psw.cc = 2;
                        return;
                    }
                }
                if (utf8[0] >= 0xf1 && utf8[0] <= 0xf3)
                {
                    if (0
                        || utf8[1] < 0x80
                        || utf8[1] > 0xbf

                        || utf8[2] < 0x80
                        || utf8[2] > 0xbf

                        || utf8[3] < 0x80
                        || utf8[3] > 0xbf
                    )
                    {
                        regs->psw.cc = 2;
                        return;
                    }
                }
                if (utf8[0] == 0xf4)
                {
                    if (0
                        || utf8[1] < 0x80
                        || utf8[1] > 0x8f

                        || utf8[2] < 0x80
                        || utf8[2] > 0xbf

                        || utf8[3] < 0x80
                        || utf8[3] > 0xbf
                    )
                    {
                        regs->psw.cc = 2;
                        return;
                    }
                }
            }
#endif /* defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY ) */

            /* xlate range f0000000-f7000000 */
            /* 1110uvw 10xyefgh 10ijklmn 10opqrst -> 00000000 000uvwxy efghijkl mnopqrst */
            utf32[1] = ((utf8[0] & 0x07) << 2) | ((utf8[1] & 0x30) >> 4);
            utf32[2] = (utf8[1] << 4) | ((utf8[2] & 0x3c) >> 2);
            utf32[3] = (utf8[2] << 6) | (utf8[3] & 0x3f);
            read = 4;
        }
        else
        {
            regs->psw.cc = 2;
            return;
        }

        /* Write UTF32 charater */
        // does the utf32 character cross a page boundary
        if (d1pg ==  MAINSTOR_PAGEBASE ( d1 + 3 ))
        {
            *(d1 +0) = utf32[0];
            *(d1 +1) = utf32[1];
            *(d1 +2) = utf32[2];
            *(d1 +3) = utf32[3];
        }
        else
        {
            ARCH_DEP(vstorec)(utf32, 3, dest, r1, regs);
        }

        /* Write and commit registers */
        SET_GR_A(r1, regs, (dest += 4) & ADDRESS_MAXWRAP(regs));
        SET_GR_A(r1 + 1, regs, destlen -= 4);
        SET_GR_A(r2, regs, (srce += read) & ADDRESS_MAXWRAP(regs));
        SET_GR_A(r2 + 1, regs, srcelen -= read);

        xlated += read;

        // Update mainstor addresses; source and destination
        s1 += read;
        if (s1pg !=  MAINSTOR_PAGEBASE ( s1 ))
        {
            s1 = MADDRL( srce, 1, r2, regs, ACCTYPE_READ, regs->psw.pkey );
            s1pg = MAINSTOR_PAGEBASE ( s1 );
        }

        d1 += 4;
        if (d1pg !=  MAINSTOR_PAGEBASE ( d1 ))
        {
            d1 = MADDRL( dest, 1, r1, regs, ACCTYPE_WRITE, regs->psw.pkey );
            d1pg = MAINSTOR_PAGEBASE ( d1 );
        }

    }

    /* CPU determined number of characters reached */
    regs->psw.cc = 3;
}

/*-------------------------------------------------------------------*/
/* B9B1 CU24  - Convert UTF-16 to UTF-32                     [RRF-c] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_utf16_to_utf32)
{
    VADR dest;                     /* Destination address            */
    GREG destlen;                  /* Destination length             */
    int r1;
    int r2;
    int m3;                        /* Mask                           */
    int read;                      /* Bytes read                     */
    VADR srce;                     /* Source address                 */
    GREG srcelen;                  /* Source length                  */
    BYTE utf16[4];                 /* utf16 character(s)             */
    BYTE utf32[4];                 /* utf328 character(s)            */
    BYTE uvwxy;                    /* Work value                     */
#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
    bool wfc;                      /* Well-Formedness-Checking (W)   */
#endif
    int xlated;                    /* characters translated          */

    RRF_M(inst, regs, r1, r2, m3);
    PER_ZEROADDR_LCHECK2( regs, r1, r1+1, r2, r2+1 );
    ODD2_CHECK(r1, r2, regs);

    /* Get paramaters */
    dest = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    destlen = GR_A(r1 + 1, regs);
    srce = regs->GR(r2) & ADDRESS_MAXWRAP(regs);
    srcelen = GR_A(r2 + 1, regs);
#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
    if ((m3 & 0x01) && FACILITY_ENABLED( 030_ETF3_ENHANCEMENT, regs ))
        wfc = true;
    else
        wfc = false;
#endif

    /* Every valid utf-32 starts with 0x00 */
    utf32[0] = 0x00;

    /* Initialize number of translated charachters */
    xlated = 0;
    while(xlated < 4096)
    {
        /* Check end of source or destination */
        if(srcelen < 2)
        {
            regs->psw.cc = 0;
            return;
        }
        if(destlen < 4)
        {
            regs->psw.cc = 1;
            return;
        }

        /* Fetch 2 bytes */
        ARCH_DEP(vfetchc)(utf16, 1, srce, r2, regs);
        if(utf16[0] <= 0xd7 || utf16[0] >= 0xdc)
        {
            /* xlate range 0000-d7fff and dc00-ffff */
            /* abcdefgh ijklmnop -> 00000000 00000000 abcdefgh ijklmnop */
            utf32[1] = 0x00;
            utf32[2] = utf16[0];
            utf32[3] = utf16[1];
            read = 2;
        }
        else
        {
            /* Check end of source */
            if(srcelen < 4)
            {
                regs->psw.cc = 0;   /* Strange but stated in POP */
                return;
            }

            /* Fetch another 2 bytes */
            ARCH_DEP(vfetchc)(&utf16[2], 1, srce, r2, regs);

#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
            /* WellFormednessChecking */
            if (wfc)
            {
                if (utf16[2] < 0xdc || utf16[2] > 0xdf)
                {
                    regs->psw.cc = 2;
                    return;
                }
            }
#endif
            /* xlate range d800-dbff */
            /* 110110ab cdefghij 110111kl mnopqrst -> 00000000 000uvwxy efghijkl mnopqrst */
            /* 000uvwxy = 0000abcde + 1 */
            uvwxy = (((utf16[0] & 0x03) << 2) | (utf16[1] >> 6)) + 1;
            utf32[1] = uvwxy;
            utf32[2] = (utf16[1] << 2) | (utf16[2] & 0x03);
            utf32[3] = utf16[3];
            read = 4;
        }

        /* Write and commit registers */
        ARCH_DEP(vstorec)(utf32, 3, dest, r1, regs);
        SET_GR_A(r1, regs, (dest += 4) & ADDRESS_MAXWRAP(regs));
        SET_GR_A(r1 + 1, regs, destlen -= 4);
        SET_GR_A(r2, regs, (srce += read) & ADDRESS_MAXWRAP(regs));
        SET_GR_A(r2 + 1, regs, srcelen -= read);

        xlated += read;
    }

    /* CPU determined number of characters reached */
    regs->psw.cc = 3;
}

/*-------------------------------------------------------------------*/
/* B9B2 CU41  - Convert UTF-32 to UTF-8                        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_utf32_to_utf8)
{
    VADR dest;                     /* Destination address            */
    GREG destlen;                  /* Destination length             */
    int r1;
    int r2;
    VADR srce;                     /* Source address                 */
    GREG srcelen;                  /* Source length                  */
    BYTE utf32[4];                 /* utf32 character(s)             */
    BYTE utf8[4];                  /* utf8 character(s)              */
    int write;                     /* Bytes written                  */
    int xlated;                    /* characters translated          */

    RRE(inst, regs, r1, r2);
    PER_ZEROADDR_LCHECK2( regs, r1, r1+1, r2, r2+1 );
    ODD2_CHECK(r1, r2, regs);

    /* Get paramaters */
    dest = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    destlen = GR_A(r1 + 1, regs);
    srce = regs->GR(r2) & ADDRESS_MAXWRAP(regs);
    srcelen = GR_A(r2 + 1, regs);

    /* Initialize number of translated charachters */
    xlated = 0;
    write = 0;
    while(xlated < 4096)
    {
        /* Check end of source or destination */
        if(srcelen < 4)
        {
            regs->psw.cc = 0;
            return;
        }
        if(destlen < 1)
        {
            regs->psw.cc = 1;
            return;
        }

        /* Get 4 bytes */
        ARCH_DEP(vfetchc)(utf32, 3, srce, r2, regs);

        if(utf32[0] != 0x00)
        {
            regs->psw.cc = 2;
            return;
        }
        else if(utf32[1] == 0x00)
        {
            if(utf32[2] == 0x00)
            {
                if(utf32[3] <= 0x7f)
                {
                    /* xlate range 00000000-0000007f */
                    /* 00000000 00000000 00000000 0jklmnop -> 0jklmnop */
                    utf8[0] = utf32[3];
                    write = 1;
                }
            }
            else if(utf32[2] <= 0x07)
            {
                /* Check destination length */
                if(destlen < 2)
                {
                    regs->psw.cc = 1;
                    return;
                }

                /* xlate range 00000080-000007ff */
                /* 00000000 00000000 00000fgh ijklmnop -> 110fghij 10klmnop */
                utf8[0] = 0xc0 | (utf32[2] << 2) | (utf32[2] >> 6);
                utf8[1] = 0x80 | (utf32[2] & 0x3f);
                write = 2;
            }
            else if(utf32[2] <= 0xd7 || utf32[2] > 0xdc)
            {
                /* Check destination length */
                if(destlen < 3)
                {
                    regs->psw.cc = 1;
                    return;
                }

                /* xlate range 00000800-0000d7ff and 0000dc00-0000ffff */
                /* 00000000 00000000 abcdefgh ijklnmop -> 1110abcd 10efghij 10klmnop */
                utf8[0] = 0xe0 | (utf32[2] >> 4);
                utf8[1] = 0x80 | ((utf32[2] & 0x0f) << 2) | (utf32[3] >> 6);
                utf8[2] = 0x80 | (utf32[3] & 0x3f);
                write = 3;
            }
            else
            {
                regs->psw.cc = 2;
                return;
            }
        }
        else if(utf32[1] >= 0x01 && utf32[1] <= 0x10)
        {
            /* Check destination length */
            if(destlen < 4)
            {
                regs->psw.cc = 1;
                return;
            }

            /* xlate range 00010000-0010ffff */
            /* 00000000 000uvwxy efghijkl mnopqrst -> 11110uvw 10xyefgh 10ijklmn 10opqrst */
            utf8[0] = 0xf0 | (utf32[1] >> 2);
            utf8[1] = 0x80 | ((utf32[1] & 0x03) << 4) | (utf32[2] >> 4);
            utf8[2] = 0x80 | ((utf32[2] & 0x0f) << 2) | (utf32[3] >> 6);
            utf8[3] = 0x80 | (utf32[3] & 0x3f);
            write = 4;
        }
        else
        {
            regs->psw.cc = 2;
            return;
        }

        /* Write and commit registers */
        ARCH_DEP(vstorec)(utf8, write - 1, dest, r1, regs);
        SET_GR_A(r1, regs, (dest += write) & ADDRESS_MAXWRAP(regs));
        SET_GR_A(r1 + 1, regs, destlen -= write);
        SET_GR_A(r2, regs, (srce += 4) & ADDRESS_MAXWRAP(regs));
        SET_GR_A(r2 + 1, regs, srcelen -= 4);

        xlated += 4;
    }

    /* CPU determined number of characters reached */
    regs->psw.cc = 3;
}

/*-------------------------------------------------------------------*/
/* B9B3 CU42  - Convert UTF-32 to UTF-16                       [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(convert_utf32_to_utf16)
{
    VADR dest;                     /* Destination address            */
    GREG destlen;                  /* Destination length             */
    int r1;
    int r2;
    VADR srce;                     /* Source address                 */
    GREG srcelen;                  /* Source length                  */
    BYTE utf16[4];                 /* utf16 character(s)             */
    BYTE utf32[4];                 /* utf32 character(s)             */
    int write;                     /* Bytes written                  */
    int xlated;                    /* characters translated          */
    BYTE zabcd;                    /* Work value                     */

    RRE(inst, regs, r1, r2);
    PER_ZEROADDR_LCHECK2( regs, r1, r1+1, r2, r2+1 );
    ODD2_CHECK(r1, r2, regs);

    /* Get paramaters */
    dest = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    destlen = GR_A(r1 + 1, regs);
    srce = regs->GR(r2) & ADDRESS_MAXWRAP(regs);
    srcelen = GR_A(r2 + 1, regs);

    /* Initialize number of translated charachters */
    xlated = 0;
    while(xlated < 4096)
    {
        /* Check end of source or destination */
        if(srcelen < 4)
        {
            regs->psw.cc = 0;
            return;
        }
        if(destlen < 2)
        {
            regs->psw.cc = 1;
            return;
        }

        /* Get 4 bytes */
        ARCH_DEP(vfetchc)(utf32, 3, srce, r2, regs);

        if(utf32[0] != 0x00)
        {
            regs->psw.cc = 2;
            return;
        }
        else if(utf32[1] == 0x00 && (utf32[2] <= 0xd7 || utf32[2] >= 0xdc))
        {
            /* xlate range 00000000-0000d7ff and 0000dc00-0000ffff */
            /* 00000000 00000000 abcdefgh ijklmnop -> abcdefgh ijklmnop */
            utf16[0] = utf32[2];
            utf16[1] = utf32[3];
            write = 2;
        }
        else if(utf32[1] >= 0x01 && utf32[1] <= 0x10)
        {
            /* Check end of destination */
            if(destlen < 4)
            {
                regs->psw.cc = 1;
                return;
            }

            /* xlate range 00010000-0010ffff */
            /* 00000000 000uvwxy efghijkl mnopqrst -> 110110ab cdefghij 110111kl mnopqrst */
            /* 000zabcd = 000uvwxy - 1 */
            zabcd = (utf32[1] - 1) & 0x0f;
            utf16[0] = 0xd8 | (zabcd >> 2);
            utf16[1] = (zabcd << 6) | (utf32[2] >> 2);
            utf16[2] = 0xdc | (utf32[2] & 0x03);
            utf16[3] = utf32[3];
            write = 4;
        }
        else
        {
            regs->psw.cc = 2;
            return;
        }

        /* Write and commit registers */
        ARCH_DEP(vstorec)(utf16, write - 1, dest, r1, regs);
        SET_GR_A(r1, regs, (dest += write) & ADDRESS_MAXWRAP(regs));
        SET_GR_A(r1 + 1, regs, destlen -= write);
        SET_GR_A(r2, regs, (srce += 4) & ADDRESS_MAXWRAP(regs));
        SET_GR_A(r2 + 1, regs, srcelen -= 4);

        xlated += 4;
    }

    /* CPU determined number of characters reached */
    regs->psw.cc = 3;
}

/*-------------------------------------------------------------------*/
/* B9BE SRSTU - Search String Unicode                          [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( search_string_unicode )
{
    VADR addr1, addr2;                  /* End/start addresses       */
    int i;                              /* Loop counter              */
    int r1, r2;                         /* Values of R fields        */
    U16 sbyte;                          /* String character          */
    U16 termchar;                       /* Terminating character     */

    RRE( inst, regs, r1, r2 );
    PER_ZEROADDR_CHECK2( regs, r1, r2 );
    TXFC_INSTR_CHECK( regs );

    /* Program check if bits 0-15 of register 0 not zero */
    if (regs->GR_L(0) & 0xFFFF0000)
        regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Load string terminating character from register 0 bits 16-31 */
    termchar = (U16) regs->GR(0);

    /* Determine the operand end and start addresses */
    addr1 = regs->GR( r1 ) & ADDRESS_MAXWRAP( regs );
    addr2 = regs->GR( r2 ) & ADDRESS_MAXWRAP( regs );

    /* "When the contents of bit position 63 of general registers R1
        and R2 differ (that is, when one address is even and the other
        is odd), the location of the first two-byte character after
        the second operand is ONE MORE THAN the contents of general
        register R1." (emphasis added)
    */
    if ((addr1 & 1) != (addr2 & 1))
    {
        addr1++;
        addr1 &= ADDRESS_MAXWRAP( regs );
    }

#undef  SRSTU_MAX
#define SRSTU_MAX   _4K     /* (Sheesh! 256 bytes is WAY too small!) */

    /* Search up to CPU-determined bytes or until end of operand */
    for (i=0; i < SRSTU_MAX; i++)
    {
        /* If operand end address has been reached, return condition
           code 2 and leave the R1 and R2 registers unchanged
        */
        /* NOTE: "When the address in general register R1 is less than
           the address in general register R2, condition code 2 can be
           set only if the operand wraps around from the top of storage
           to location 0."  Thus the below == comparison is correct.
        */
        if (addr2 == addr1)
        {
            regs->psw.cc = 2;
            return;
        }

        /* Fetch 2 bytes from the operand */
        sbyte = ARCH_DEP( vfetch2 )( addr2, r2, regs );

        /* If the terminating character was found, return condition
           code 1 and load the address of the character into R1 */
        if (sbyte == termchar)
        {
            SET_GR_A( r1, regs, addr2 );
            regs->psw.cc = 1;
            return;
        }

        /* Increment operand address */
        addr2 += 2;
        addr2 &= ADDRESS_MAXWRAP( regs );

    } /* end for(i) */

    /* Set R2 to point to next character of operand */
    SET_GR_A( r2, regs, addr2 );

    /* Return condition code 3 */
    regs->psw.cc = 3;
}


#undef  MAINSTOR_PAGEBASE
#define MAINSTOR_PAGEBASE( _ma )  ((BYTE*) ((uintptr_t) ( _ma ) & PAGEFRAME_PAGEMASK))

/*-------------------------------------------------------------------*/
/* D0   TRTR  - Translate and Test Reverse                    [SS-a] */
/*-------------------------------------------------------------------*/
DEF_INST(translate_and_test_reverse)
{
    int b1, b2;                        // Values of base field
    int cc = 0;                        // Condition code
    VADR effective_addr1;
    VADR effective_addr2;              // Effective addresses
    int i;                             // Integer work areas
    int len;                           // Length byte
    BYTE sbyte;                        // Byte work areas

    CACHE_ALIGN BYTE  trtab[256];      // Translate table - copy
    BYTE*   p_fct;                     // ptr to FC Table
    BYTE*   m1;                        // operand mainstor addresses
    BYTE   *m1pg;                      // operand page

    SS_L(inst, regs, len, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    TXFC_INSTR_CHECK( regs );

    /* Get pointer to Function Code table or copy */
    if (CROSSPAGE( effective_addr2, 256-1))
    {
        ARCH_DEP( vfetchc )( trtab, 256-1, effective_addr2, b2, regs );
        p_fct = (BYTE *) &trtab;
    }
    else
    {
        p_fct = MADDRL( effective_addr2, 256, b2, regs, ACCTYPE_READ, regs->psw.pkey );
    }

    /* Get mainstor address to test byte */
    m1 = MADDRL( effective_addr1, 1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
    m1pg = MAINSTOR_PAGEBASE ( m1 );

    /* Process first operand from right to left*/
    for(i = 0; i <= len; i++)
    {
        /* Fetch argument byte from first operand */
        // dbyte = ARCH_DEP(vfetchb)(effective_addr1, b1, regs);

        /* Fetch function byte from second operand */
        // sbyte = ARCH_DEP(vfetchb)((effective_addr2 + dbyte) & ADDRESS_MAXWRAP(regs), b2, regs);

        sbyte = p_fct[ *m1 ];

        /* Test for non-zero function byte */
        if(sbyte != 0)
        {
            /* Store address of argument byte in register 1 */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
            if(regs->psw.amode64)
                regs->GR_G(1) = effective_addr1;
            else
#endif
            if(regs->psw.amode)
            {
                /* Note: TRTR differs from TRT in 31 bit mode.
                TRTR leaves bit 32 unchanged, TRT clears bit 32 */
                regs->GR_L(1) &= 0x80000000;
                regs->GR_L(1) |= effective_addr1;
            }
            else
                regs->GR_LA24(1) = effective_addr1;

            /* Store function byte in low-order byte of reg.2 */
            regs->GR_LHLCL(2) = sbyte;

            /* Set condition code 2 if argument byte was last byte
             of first operand, otherwise set condition code 1 */
            cc = (i == len) ? 2 : 1;

            /* Terminate the operation at this point */
            break;

        } /* end if(sbyte) */

        /* Decrement first operand address */
        effective_addr1--; /* Another difference with TRT */
        effective_addr1 &= ADDRESS_MAXWRAP(regs);

        /* update mainstor address */
        m1--;

        /* check for page cross */
        if ( MAINSTOR_PAGEBASE ( m1 ) != m1pg )
        {
            m1 = MADDRL(effective_addr1, 1, b1, regs, ACCTYPE_READ, regs->psw.pkey );
            m1pg = MAINSTOR_PAGEBASE ( m1 );
        }

    } /* end for(i) */

    /* Update the condition code */
    regs->psw.cc = cc;
}

#endif /* defined( FEATURE_022_EXT_TRANSL_FACILITY_3 ) */


#ifdef FEATURE_026_PARSING_ENHANCE_FACILITY

/* Function Code Table - # pages to cover table */
#undef FCT_REAL_MAX_PAGES
#define FCT_REAL_MAX_PAGES (((128*1024) / PAGEFRAME_PAGESIZE ) + 1 )

/*-------------------------------------------------------------------*/
/* Translate and Test Extended - Forward and Reverse                 */
/*-------------------------------------------------------------------*/
/* Subroutine is called by TRTE and TRTRE instructions               */
/*-------------------------------------------------------------------*/
/* B9BF TRTE - Translate and Test Extended                   [RRF-c] */
/* B9BD TRTRE - Translate and Test Reverse Extended          [RRF-c] */
/*-------------------------------------------------------------------*/
DEF_INST( translate_and_test_xxx_extended )
{
    int   r1;                   /* Operand-1 register number         */
    int   r2;                   /* Operand-2 register number         */
    int   m3;                   /* Operand-3 mask value              */

    bool  a_bit;                /* (A) Argument-Character Control    */
    bool  f_bit;                /* (F) Function-Code Control         */
    bool  l_bit;                /* (L) Argument-Character Limit      */

    bool  isReverse;            /* true == TRTRE, false == TRTE      */

    U32   arg_ch;               /* Argument character                */
    U32   fc;                   /* Function-Code                     */
    U16   temp_l;               /* Work - low byte                   */
    U16   temp_h;               /* Work - high byte                  */

    int   processed;            /* # bytes processed                 */
    int   max_process;          /* Max to process on current page    */
    int   i;                    /* Work iterator                     */

    VADR  buf_addr;             /* First argument address            */
    GREG  buf_len;              /* First argument length             */
    BYTE* buf_main_addr;        /* Buffer real address */

    VADR  fc_page_no;           /* Function-Code - page number       */
    VADR  fc_addr;              /* Function-Code - address           */

    VADR  fct_addr;             /* Function-code table address       */
    VADR  fct_page_no;          /* Function-Code Table - page number */
    VADR  fct_page_addr;        /* Function-Code TABLE - page addr   */

    VADR  fct_work_page_addr;   /* Work                              */
    VADR  fct_work_end_addr;    /* Work                              */

    CACHE_ALIGN                 /* FC Table - direct mainstor addrs  */
    BYTE* fct_main_page_addr[ FCT_REAL_MAX_PAGES ];

    /* Function Code Table Lengths lookup table indexed by 'm3' mask */

    static const int fct_table_lengths[ 8 ] =
    {                           /*      A   F   L       (index)      */
                                /*    -------------    ---------     */
        256,                    /*      0   0   0          0         */
        256,                    /*      0   0   1          1         */
        512,                    /*      0   1   0          2         */
        512,                    /*      0   1   1          3         */
        64*1024,                /*      1   0   0          4         */
        256,                    /*      1   0   1          5         */
        128*1024,               /*      1   1   0          6         */
        512                     /*      1   1   1          7         */
    };

    RRF_M( inst, regs, r1, r2, m3 );
    PER_ZEROADDR_CHECK( regs, 1 );
    PER_ZEROADDR_LCHECK( regs, r1, r1+1 );

    TXFC_INSTR_CHECK( regs );

    a_bit = ((m3 & 0x08) ? true : false);
    f_bit = ((m3 & 0x04) ? true : false);
    l_bit = ((m3 & 0x02) ? true : false);

    isReverse = (inst[1] == 0XBD);  // TRTRE instruction?

    buf_addr = regs->GR( r1 ) & ADDRESS_MAXWRAP( regs );
    buf_len  = GR_A( r1 + 1, regs );

    if (unlikely((a_bit && (buf_len & 1)) || r1 & 0x01))
        regs->program_interrupt( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Fast exit path */
    if (buf_len == 0)
    {
        regs->psw.cc =  0;
        return;
    }

    /* Initialize fct_... variables */

    fct_addr           = regs->GR( 1 ) & ADDRESS_MAXWRAP( regs );
    fct_work_end_addr  = fct_addr + fct_table_lengths[ m3 >> 1 ];
    fct_page_addr      = fct_addr & PAGEFRAME_PAGEMASK;
    fct_page_no        = fct_page_addr >> PAGEFRAME_PAGESHIFT;
    fct_work_page_addr = fct_page_addr;

    /* Build function code table mainstor address table
       (but only for the pages that cover the FC table)
    */
    for (i=0; fct_work_page_addr < fct_work_end_addr; fct_work_page_addr += PAGEFRAME_PAGESIZE, ++i)
    {
        fct_main_page_addr[i] = MADDRL( fct_work_page_addr & ADDRESS_MAXWRAP( regs ),
                                        PAGEFRAME_PAGESIZE, 1, regs, ACCTYPE_READ,
                                        regs->psw.pkey );
    }

    /*  Determine CC=3 length:

        POP : SA22-7832-13 Page 7-418:

           "The amount of processing that results in the setting
            of condition code 3 is determined by the CPU on the
            basis of improving system performance, and it may
            be a different amount each time the instruction is
            executed."

        CC=3 :  Processed first operand to end of page and
                indicate more data remaining.
    */

    /* Get on-page maximum process length */
    if (isReverse)
        /* TRTRE instruction */
        max_process = (buf_addr & PAGEFRAME_BYTEMASK) + 1;
    else
        /* TRTE instruction */
        max_process = PAGEFRAME_PAGESIZE - ( buf_addr & PAGEFRAME_BYTEMASK );

    /* Get buffer mainstor address - a bit = 1 page cross checked later */
    buf_main_addr = MADDRL( buf_addr, 1, r1, regs, ACCTYPE_READ, regs->psw.pkey );

    fc = 0;
    processed = 0;
    while (buf_len && !fc && processed < max_process)
    {
        if (a_bit)
        {
            /* Does arg cross page boundary? (last byte of page?) */
            if ((buf_addr & PAGEFRAME_BYTEMASK) == PAGEFRAME_BYTEMASK)
            {
                /* Yes! Piece together the argument */
                temp_h  =  *buf_main_addr;
                temp_l  =  *MADDRL( buf_addr+1, 1 , r1, regs, ACCTYPE_READ, regs->psw.pkey );
            }
            else
            {
                // the following fails on sparc 64 : alignment
                // arg_ch = CSWAP16( *(U16*) buf_main_addr );
                // so
                temp_h  =  *(buf_main_addr +0);
                temp_l  =  *(buf_main_addr +1);
            }
            arg_ch  =   (temp_h << 8) | temp_l;
        }
        else
        {
            arg_ch = *buf_main_addr;
        }

        if (l_bit && arg_ch > 255)
            fc = 0;
        else
        {
            if (f_bit)
            {
                fc_addr    = fct_addr + (arg_ch * 2);
                fc_page_no = fc_addr >> PAGEFRAME_PAGESHIFT;

                /* NOTE: The user *should* have specified a double-word
                         aligned FC table address, but since they might
                         not have, we still need to always perform the
                         below check. */

                /* Does FC cross page boundary? (last byte of page?) */
                if ((fc_addr & PAGEFRAME_BYTEMASK) == PAGEFRAME_BYTEMASK)
                {
                    /* Yes! Piece together the FC */
                    temp_h  =  *(fct_main_page_addr[ fc_page_no - fct_page_no + 0 ] + PAGEFRAME_BYTEMASK ); // (last byte of page)
                    temp_l  =  *(fct_main_page_addr[ fc_page_no - fct_page_no + 1 ] );                      // (first byte of next page)
                }
                else
                {
                    // fc =  CSWAP16( *(U16*) (fct_main_page_addr[ fc_page_no - fct_page_no ] + (fc_addr & PAGEFRAME_BYTEMASK)));
                    temp_h  =  *( fct_main_page_addr[ fc_page_no - fct_page_no ] + (fc_addr & PAGEFRAME_BYTEMASK) +0 );
                    temp_l  =  *( fct_main_page_addr[ fc_page_no - fct_page_no ] + (fc_addr & PAGEFRAME_BYTEMASK) +1 );
                }
                fc  =   (temp_h << 8) | temp_l;
            }
            else
            {
                fc_addr     =  fct_addr + arg_ch;
                fc_page_no  =  fc_addr >> PAGEFRAME_PAGESHIFT;
                fc          =  *(fct_main_page_addr[ fc_page_no - fct_page_no ] + (fc_addr & PAGEFRAME_BYTEMASK));
            }
        }

        if (!fc)
        {
            if (a_bit)
            {
                processed          +=  2;
                buf_len            -=  2;

                if (isReverse)
                {
                    buf_main_addr  -=  2;
                    buf_addr       -=  2;
                }
                else
                {
                    buf_main_addr  +=  2;
                    buf_addr       +=  2;
                }
            }
            else
            {
                processed          +=  1;
                buf_len            -=  1;

                if (isReverse)
                {
                    buf_main_addr  -=  1;
                    buf_addr       -=  1;
                }
                else
                {
                    buf_main_addr  +=  1;
                    buf_addr       +=  1;
                }
            }

            buf_addr &= ADDRESS_MAXWRAP( regs );
        }
    } /* end while */

    /* Commit registers */
    SET_GR_A( r1 + 0, regs, buf_addr );
    SET_GR_A( r1 + 1, regs, buf_len );

    /* Check if CPU determined number of bytes have been processed */
    if (buf_len && !fc)
    {
        regs->psw.cc = 3;
        return;
    }

    /* Set function code */
    if (likely(r2 != r1 && r2 != (r1 + 1)))
        SET_GR_A( r2, regs, fc );

    /* Set condition code */
    if (fc)
        regs->psw.cc = 1;
    else
        regs->psw.cc = 0;
}

/*-------------------------------------------------------------------*/
/* B9BF TRTE - Translate and Test Extended                   [RRF-c] */
/*-------------------------------------------------------------------*/
DEF_INST( translate_and_test_extended )
{
    ARCH_DEP( translate_and_test_xxx_extended )( inst, regs );
}

/*-------------------------------------------------------------------*/
/* B9BD TRTRE - Translate and Test Reverse Extended          [RRF-c] */
/*-------------------------------------------------------------------*/
DEF_INST(translate_and_test_reverse_extended)
{
    ARCH_DEP( translate_and_test_xxx_extended )( inst, regs );
}

#endif /* FEATURE_026_PARSING_ENHANCE_FACILITY */

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "general2.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "general2.c"
  #endif

#endif /* !defined( _GEN_ARCH ) */
