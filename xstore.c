/* XSTORE.C     (C) Copyright Jan Jaeger, 1999-2012                  */
/*              Expanded storage related instructions                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/* MVPG moved from cpu.c to xstore.c   05/07/00 Jan Jaeger */

/*-------------------------------------------------------------------*/
/* This module implements the expanded storage instructions          */
/* for the Hercules ESA/390 emulator.                                */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _XSTORE_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#if defined(FEATURE_EXPANDED_STORAGE)
/*-------------------------------------------------------------------*/
/* B22E PGIN  - Page in from expanded storage                  [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(page_in)
{
int     r1, r2;                         /* Values of R fields        */
VADR    vaddr;                          /* Virtual storage address   */
BYTE   *maddr;                          /* Main storage address      */
U32     xaddr;                          /* Expanded storage block#   */
size_t  xoffs;                          /* Byte offset into xpndstor */

    RRE(inst, regs, r1, r2);
    PER_ZEROADDR_CHECK( regs, r1 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);

    if(SIE_STATE_BIT_ON(regs, IC3, PGX))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);

#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)
    /* Cannot perform xstore page movement in XC mode */
    if(SIE_STATE_BIT_ON(regs, MX, XC))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)*/

    /* expanded storage block number */
    xaddr = regs->GR_L(r2);

    if(SIE_MODE(regs))
    {
        xaddr += regs->sie_xso;
        if(xaddr >= regs->sie_xsl)
        {
            PTT_ERR("*PGIN",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);
            regs->psw.cc = 3;
            return;
        }
    }

    /* If the expanded storage block is not configured then
       terminate with cc3 */
    if (xaddr >= sysblk.xpndsize)
    {
        PTT_ERR("*PGIN",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);
        regs->psw.cc = 3;
        return;
    }

    /* Byte offset in expanded storage */
    xoffs = (size_t)xaddr << XSTORE_PAGESHIFT;

    /* Obtain abs address, verify access and set ref/change bits */
    vaddr = (regs->GR(r1) & ADDRESS_MAXWRAP(regs)) & XSTORE_PAGEMASK;
    maddr = MADDRL (vaddr, 4096, USE_REAL_ADDR, regs, ACCTYPE_WRITE, 0);

    /* Copy data from expanded to main */
    memcpy (maddr, sysblk.xpndstor + xoffs, XSTORE_PAGESIZE);

    /* cc0 means pgin ok */
    regs->psw.cc = 0;

} /* end DEF_INST(page_in) */
#endif /*defined(FEATURE_EXPANDED_STORAGE)*/


#if defined(FEATURE_EXPANDED_STORAGE)
/*-------------------------------------------------------------------*/
/* B22F PGOUT - Page out to expanded storage                   [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(page_out)
{
int     r1, r2;                         /* Values of R fields        */
VADR    vaddr;                          /* Virtual storage address   */
BYTE   *maddr;                          /* Main storage address      */
U32     xaddr;                          /* Expanded storage block#   */
size_t  xoffs;                          /* Byte offset into xpndstor */

    RRE(inst, regs, r1, r2);
    PER_ZEROADDR_CHECK( regs, r1 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);

    if(SIE_STATE_BIT_ON(regs, IC3, PGX))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);

#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)
    /* Cannot perform xstore page movement in XC mode */
    if(SIE_STATE_BIT_ON(regs, MX, XC))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)*/

    /* expanded storage block number */
    xaddr = regs->GR_L(r2);

    if(SIE_MODE(regs))
    {
        xaddr += regs->sie_xso;
        if(xaddr >= regs->sie_xsl)
        {
            PTT_ERR("*PGOUT",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);
            regs->psw.cc = 3;
            return;
        }
    }

    /* If the expanded storage block is not configured then
       terminate with cc3 */
    if (xaddr >= sysblk.xpndsize)
    {
        PTT_ERR("*PGOUT",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);
        regs->psw.cc = 3;
        return;
    }

    /* Byte offset in expanded storage */
    xoffs = (size_t)xaddr << XSTORE_PAGESHIFT;

    /* Obtain abs address, verify access and set ref/change bits */
    vaddr = (regs->GR(r1) & ADDRESS_MAXWRAP(regs)) & XSTORE_PAGEMASK;
    maddr = MADDR (vaddr, USE_REAL_ADDR, regs, ACCTYPE_READ, 0);

    /* Copy data from main to expanded */
    memcpy (sysblk.xpndstor + xoffs, maddr, XSTORE_PAGESIZE);

    /* cc0 means pgout ok */
    regs->psw.cc = 0;

} /* end DEF_INST(page_out) */
#endif /*defined(FEATURE_EXPANDED_STORAGE)*/


#if defined( FEATURE_MOVE_PAGE_FACILITY_2 ) && defined( FEATURE_EXPANDED_STORAGE )
/*-------------------------------------------------------------------*/
/* B259 IESBE - Invalidate Expanded Storage Block Entry        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( invalidate_expanded_storage_block_entry )
{
int     r1, r2;                         /* Operand register numbers  */

    RRE( inst, regs, r1, r2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

#if defined( _FEATURE_SIE )
    if (SIE_STATE_BIT_OFF( regs, EC0, MVPG ))
        SIE_INTERCEPT( regs );
#endif

    PERFORM_SERIALIZATION( regs );
    {
        OBTAIN_INTLOCK( regs );
        {
            SYNCHRONIZE_CPUS( regs );

            /* Invalidate page table entry */
            ARCH_DEP( invalidate_pte )( inst[1], regs->GR_G( r1 ), regs->GR( r2 ), regs, false );
        }
        RELEASE_INTLOCK( regs );
    }
    PERFORM_SERIALIZATION( regs );

} /* end DEF_INST( invalidate_expanded_storage_block_entry ) */
#endif /* defined( FEATURE_EXPANDED_STORAGE ) */


#if defined(_MSVC_)
  /* Workaround for "fatal error C1001: INTERNAL COMPILER ERROR" in MSVC */
  #pragma optimize("",off)
#endif /*defined(_MSVC_)*/

#if defined(FEATURE_MOVE_PAGE_FACILITY_2)
/*-------------------------------------------------------------------*/
/* B254 MVPG  - Move Page                                      [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST(move_page)
{
int     r1, r2;                         /* Register values           */
int     rc = 0;                         /* Return code               */
int     cc = 0;                         /* Condition code            */
VADR    vaddr1, vaddr2;                 /* Virtual addresses         */
RADR    raddr1=0, raddr2=0, xpkeya;     /* Real addresses            */
BYTE   *main1 = NULL, *main2 = NULL;    /* Mainstor addresses        */
BYTE   *sk1;                            /* Storage key address       */
BYTE    akey;                           /* Access key in register 0  */
BYTE    akey1, akey2;                   /* Access keys for operands  */
#if defined(FEATURE_EXPANDED_STORAGE)
int     xpvalid1 = 0, xpvalid2 = 0;     /* 1=Operand in expanded stg */
CREG    pte1 = 0, pte2 = 0;             /* Page table entry          */
U32     xpblk1 = 0, xpblk2 = 0;         /* Expanded storage block#   */
BYTE    xpkey1 = 0, xpkey2 = 0;         /* Expanded storage keys     */
#endif /*defined(FEATURE_EXPANDED_STORAGE)*/

    RRE(inst, regs, r1, r2);
    PER_ZEROADDR_CHECK2( regs, r1, r2 );
    TXF_INSTR_CHECK( regs );

#if defined(_FEATURE_SIE)
    if(SIE_STATE_BIT_OFF(regs, EC0, MVPG))
        longjmp(regs->progjmp, SIE_INTERCEPT_INST);
#endif /*defined(_FEATURE_SIE)*/

    /* Use PSW key as access key for both operands */
    akey1 = akey2 = regs->psw.pkey;

    /* If register 0 bit 20 or 21 is one, get access key from R0 */
    if (regs->GR_L(0) & (GR0_MVPG_FIRST | GR0_MVPG_SECOND))
    {
        /* Extract the access key from register 0 bits 24-27 */
        akey = regs->GR_L(0) & GR0_MVPG_KEY;

        /* Priviliged operation exception if in problem state, and
           the specified key is not permitted by the PSW key mask */
        if ( PROBSTATE(&regs->psw)
            && ((regs->CR(3) << (akey >> 4)) & 0x80000000) == 0 )
            regs->program_interrupt (regs, PGM_PRIVILEGED_OPERATION_EXCEPTION);

        /* If register 0 bit 20 is one, use R0 key for operand 1 */
        if (regs->GR_L(0) & GR0_MVPG_FIRST)
            akey1 = akey;

        /* If register 0 bit 21 is one, use R0 key for operand 2 */
        if (regs->GR_L(0) & GR0_MVPG_SECOND)
            akey2 = akey;
    }

    /* Specification exception if register 0 bits 16-19 are
       not all zero, or if bits 20 and 21 are both ones */
    if ((regs->GR_L(0) & GR0_MVPG_RSRVD) != 0
        || (regs->GR_L(0) & (GR0_MVPG_FIRST | GR0_MVPG_SECOND))
                         == (GR0_MVPG_FIRST | GR0_MVPG_SECOND))
        regs->program_interrupt (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Determine the logical addresses of each operand */
    vaddr1 = regs->GR(r1) & ADDRESS_MAXWRAP(regs);
    vaddr2 = regs->GR(r2) & ADDRESS_MAXWRAP(regs);

    /* Isolate the page addresses of each operand */
    vaddr1 &= XSTORE_PAGEMASK;
    vaddr2 &= XSTORE_PAGEMASK;

    /* Obtain the real or expanded address of each operand */
    if ( !REAL_MODE(&regs->psw) || SIE_MODE(regs) )
    {
        /* Translate the second operand address to a real address */
        if(!REAL_MODE(&regs->psw))
        {
            rc = ARCH_DEP(translate_addr) (vaddr2, r2, regs, ACCTYPE_READ);
            raddr2 = regs->dat.raddr;
        }
        else
            raddr2 = vaddr2;

        if(rc != 0 && rc != 2)
            goto mvpg_progck;

        raddr2 = APPLY_PREFIXING (raddr2, regs->PX);

        if (raddr2 > regs->mainlim)
            regs->program_interrupt (regs, PGM_ADDRESSING_EXCEPTION);

#if defined(_FEATURE_SIE)
        if(SIE_MODE(regs)  && !regs->sie_pref)
        {
#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)
            if (SIE_TRANSLATE_ADDR (regs->sie_mso + raddr2,
                (SIE_STATE_BIT_ON(regs, MX, XC) && AR_BIT(&regs->psw) && r2 > 0)
                ? r2 : USE_PRIMARY_SPACE, HOSTREGS, ACCTYPE_SIE))
#else /*!defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)*/
            if (SIE_TRANSLATE_ADDR (regs->sie_mso + raddr2,
                    USE_PRIMARY_SPACE, HOSTREGS, ACCTYPE_SIE))
#endif /*!defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)*/
                (HOSTREGS->program_interrupt) (HOSTREGS, HOSTREGS->dat.xcode);

            /* Convert host real address to host absolute address */
            raddr2 = apply_host_prefixing( HOSTREGS, HOSTREGS->dat.raddr );
        } /* SIE_MODE */
#endif /*defined(_FEATURE_SIE)*/

#if defined(FEATURE_EXPANDED_STORAGE)
        if(rc == 2)
        {
            FETCH_W(pte2,regs->mainstor + raddr2);
            /* If page is invalid in real storage but valid in expanded
               storage then xpblk2 now contains expanded storage block# */
            if(pte2 & PAGETAB_ESVALID)
            {
                xpblk2 = (pte2 & ZPGETAB_PFRA) >> 12;
#if defined(_FEATURE_SIE)
                if(SIE_MODE(regs))
                {
                    /* Add expanded storage origin for this guest */
                    xpblk2 += regs->sie_xso;
                    /* If the block lies beyond this guests limit
                       then we must terminate the instruction */
                    if(xpblk2 >= regs->sie_xsl)
                    {
                        cc = 2;
                        goto mvpg_progck;
                    }
                } /* SIE_MODE */
#endif /*defined(_FEATURE_SIE)*/

                rc = 0;
                xpvalid2 = 1;
                xpkeya = raddr2 +
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
                                   2048;
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
                /* For ESA/390 mode, the XPTE lies directly beyond
                   the PTE, and each entry is 12 bytes long, we must
                   therefor add 1024 + 8 times the page index */
                                 1024 + ((vaddr2 & 0x000FF000) >> 9);
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
                if (xpkeya > regs->mainlim)
                    regs->program_interrupt (regs, PGM_ADDRESSING_EXCEPTION);
                xpkey2 = regs->mainstor[xpkeya];

/*DEBUG logmsg("MVPG pte2 = " F_CREG ", xkey2 = %2.2X, xpblk2 = %5.5X, akey2 = %2.2X\n",
                  pte2,xpkey2,xpblk2,akey2);  */
            }
            else
            {
                cc = 2;
                goto mvpg_progck;
            }
        }
#endif /*defined(FEATURE_EXPANDED_STORAGE)*/

        /* Program check if second operand is not valid
           in either main storage or expanded storage */
        if (rc)
        {
            cc = 2;
            goto mvpg_progck;
        }

        /* Reset protection indication before calling translate_addr() */
        regs->dat.protect = 0;
        /* Translate the first operand address to a real address */
        if(!REAL_MODE(&regs->psw))
        {
            rc = ARCH_DEP(translate_addr) (vaddr1, r1, regs, ACCTYPE_WRITE);
            raddr1 = regs->dat.raddr;
        }
        else
            raddr1 = vaddr1;

        if(rc != 0 && rc != 2)
            goto mvpg_progck;

        raddr1 = APPLY_PREFIXING (raddr1, regs->PX);

        if (raddr1 > regs->mainlim)
            regs->program_interrupt (regs, PGM_ADDRESSING_EXCEPTION);

#if defined(_FEATURE_SIE)
        if(SIE_MODE(regs)  && !regs->sie_pref)
        {
#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)
            if (SIE_TRANSLATE_ADDR (regs->sie_mso + raddr1,
                (SIE_STATE_BIT_ON(regs, MX, XC) && AR_BIT(&regs->psw) && r1 > 0)
                ? r1 : USE_PRIMARY_SPACE, HOSTREGS, ACCTYPE_SIE))
#else /*!defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)*/
            if (SIE_TRANSLATE_ADDR (regs->sie_mso + raddr1,
                    USE_PRIMARY_SPACE, HOSTREGS, ACCTYPE_SIE))
#endif /*!defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)*/
                (HOSTREGS->program_interrupt) (HOSTREGS, HOSTREGS->dat.xcode);

            /* Convert host real address to host absolute address */
            raddr1 = apply_host_prefixing( HOSTREGS, HOSTREGS->dat.raddr );
        } /* SIE_MODE */
#endif /*defined(_FEATURE_SIE)*/

#if defined(FEATURE_EXPANDED_STORAGE)
        if(rc == 2)
        {
            FETCH_W(pte1,regs->mainstor + raddr1);
            /* If page is invalid in real storage but valid in expanded
               storage then xpblk1 now contains expanded storage block# */
            if(pte1 & PAGETAB_ESVALID)
            {
                xpblk1 = (pte1 & ZPGETAB_PFRA) >> 12;
#if defined(_FEATURE_SIE)
                if(SIE_MODE(regs))
                {
                    /* Add expanded storage origin for this guest */
                    xpblk1 += regs->sie_xso;
                    /* If the block lies beyond this guests limit
                       then we must terminate the instruction */
                    if(xpblk1 >= regs->sie_xsl)
                    {
                        cc = 1;
                        goto mvpg_progck;
                    }
                }
#endif /*defined(_FEATURE_SIE)*/

                rc = 0;
                xpvalid1 = 1;
                xpkeya = raddr1 +
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
                                  2048;
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
                /* For ESA/390 mode, the XPTE lies directly beyond
                   the PTE, and each entry is 12 bytes long, we must
                   therefor add 1024 + 8 times the page index */
                              1024 + ((vaddr1 & 0x000FF000) >> 9);
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
                if (xpkeya > regs->mainlim)
                    regs->program_interrupt (regs, PGM_ADDRESSING_EXCEPTION);
                xpkey1 = regs->mainstor[xpkeya];

/*DEBUG  logmsg("MVPG pte1 = " F_CREG ", xkey1 = %2.2X, xpblk1 = %5.5X, akey1 = %2.2X\n",
                  pte1,xpkey1,xpblk1,akey1);  */
            }
            else
            {
                cc = 1;
                goto mvpg_progck;
            }
        }
#endif /*defined(FEATURE_EXPANDED_STORAGE)*/

        /* Program check if operand not valid in main or expanded */
        if (rc)
        {
            cc = 1;
            goto mvpg_progck;
        }

        /* Program check if page protection or access-list controlled
           protection applies to the first operand */
        if (regs->dat.protect || (xpvalid1 && (pte1 & PAGETAB_PROT)))
        {
            regs->TEA = vaddr1 | TEA_PROT_AP | regs->dat.stid;
            regs->excarid = (ACCESS_REGISTER_MODE(&regs->psw)) ? r1 : 0;
            regs->program_interrupt (regs, PGM_PROTECTION_EXCEPTION);
        }

    } /* end if(!REAL_MODE) */

#if defined(FEATURE_EXPANDED_STORAGE)
    /* Program check if both operands are in expanded storage,
       or if first operand is in expanded storage and the Destination
       Reference Intention (DRI) bit (register 0 bit 22) is set to one,
       or if first operand is in expanded storage and PTE lock bit on,
       or if first operand is in expanded storage and frame invalid */
    if ((xpvalid1 && xpvalid2)
        || (xpvalid1 && (regs->GR_L(0) & GR0_MVPG_DRI))
        || (xpvalid1 && (pte1 & PAGETAB_PGLOCK))
        || (xpvalid1 && (xpblk1 >= sysblk.xpndsize)))
    {
        regs->dat.xcode = PGM_PAGE_TRANSLATION_EXCEPTION;
        rc = 2;
        cc = 1;
        goto mvpg_progck;
    }
    /* More Program check checking, but at lower priority:
       if second operand is in expanded storage and pte lock bit on, or
       if second operand is in expanded storage and frame invalid */
    if ((xpvalid2 && (pte2 & PAGETAB_PGLOCK))
        || (xpvalid2 && (xpblk2 >= sysblk.xpndsize)))
    {
        /* re-do translation to set up TEA */
        rc = ARCH_DEP(translate_addr) (vaddr2, r2, regs, ACCTYPE_READ);
        regs->dat.xcode = PGM_PAGE_TRANSLATION_EXCEPTION;
        cc = 1;
        goto mvpg_progck;
    }

    /* Perform protection checks */
    if (xpvalid1)
    {
        /* Key check on expanded storage block if NoKey bit off in PTE */
        if (akey1 != 0 && akey1 != (xpkey1 & STORKEY_KEY)
            && (pte1 & PAGETAB_ESNK) == 0
            && !((regs->CR(0) & CR0_STORE_OVRD) && ((xpkey1 & STORKEY_KEY) == 0x90)))
        {
            regs->program_interrupt (regs, PGM_PROTECTION_EXCEPTION);
        }
        sk1=NULL;
    }
    else
#endif /*defined(FEATURE_EXPANDED_STORAGE)*/
    {
        /* Obtain absolute address of main storage block,
           check protection, and set reference and change bits */
        main1 = MADDRL (vaddr1, _4K, r1, regs, ACCTYPE_WRITE_SKP, akey1);
        sk1 = regs->dat.storkey;
    }

#if defined(FEATURE_EXPANDED_STORAGE)
    if (xpvalid2)
    {
        /* Key check on expanded storage block if NoKey bit off in PTE */
        if (akey2 != 0 && (xpkey2 & STORKEY_FETCH)
            && akey2 != (xpkey2 & STORKEY_KEY)
            && (pte2 & PAGETAB_ESNK) == 0)
        {
            regs->program_interrupt (regs, PGM_PROTECTION_EXCEPTION);
        }
    }
    else
#endif /*defined(FEATURE_EXPANDED_STORAGE)*/
    {
        /* Obtain absolute address of main storage block,
           check protection, and set reference bit.
           Use last byte of page to avoid FPO area.  */
        main2 = MADDR (vaddr2 | 0xFFF, r2, regs, ACCTYPE_READ, akey2);
        main2 -= 0xFFF;
    }

#if defined(FEATURE_EXPANDED_STORAGE)
    /* Perform page movement */
    if (xpvalid2)
    {
        /* Set the main storage reference and change bits */
        ARCH_DEP( or_storage_key_by_ptr )( sk1, (STORKEY_REF | STORKEY_CHANGE) );

        /* Set Expanded Storage reference bit in the PTE */
        STORE_W(regs->mainstor + raddr2, pte2 | PAGETAB_ESREF);

        /* Move 4K bytes from expanded storage to main storage */
        memcpy (main1,
                sysblk.xpndstor + ((size_t)xpblk2 << XSTORE_PAGESHIFT),
                XSTORE_PAGESIZE);
    }
    else if (xpvalid1)
    {
        /* Set Expanded Storage reference and change bits in the PTE */
        STORE_W(regs->mainstor + raddr1, pte1 | PAGETAB_ESREF | PAGETAB_ESCHA);

        /* Move 4K bytes from main storage to expanded storage */
        memcpy (sysblk.xpndstor + ((size_t)xpblk1 << XSTORE_PAGESHIFT),
                main2,
                XSTORE_PAGESIZE);
    }
    else
#endif /*defined(FEATURE_EXPANDED_STORAGE)*/
    {
        /* Set the main storage reference and change bits */
        ARCH_DEP( or_storage_key_by_ptr )( sk1, (STORKEY_REF | STORKEY_CHANGE) );

        /* Move 4K bytes from main storage to main storage */
        memcpy (main1, main2, XSTORE_PAGESIZE);
    }

    /* Return condition code zero */
    regs->psw.cc = 0;
    return;

mvpg_progck:

    PTT_ERR("*MVPG",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);

    /* If page translation exception (PTE invalid) and Condition Code
        Option (CCO) bit (register 0 bit 23) is set, return condition code */
    if ((regs->GR_L(0) & GR0_MVPG_CCO)
        && regs->dat.xcode == PGM_PAGE_TRANSLATION_EXCEPTION
        && rc == 2)
    {
        regs->psw.cc = cc;
        return;
    }

    /* Otherwise generate program check */
    /* (Bit 29 of TEA is on for PIC 11 & operand ID also stored) */
    if (regs->dat.xcode == PGM_PAGE_TRANSLATION_EXCEPTION)
    {
        regs->TEA |= TEA_MVPG;
        regs->opndrid = (r1 << 4) | r2;
    }
    regs->program_interrupt (regs, regs->dat.xcode);

} /* end DEF_INST(move_page) */
#endif /*defined(FEATURE_MOVE_PAGE_FACILITY_2)*/

#if defined(_MSVC_)
  /* Workaround for "fatal error C1001: INTERNAL COMPILER ERROR" in MSVC */
  #pragma optimize("",on)
#endif /*defined(_MSVC_)*/


#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "xstore.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "xstore.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
