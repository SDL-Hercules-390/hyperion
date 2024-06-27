/* STACK.C      (C) Copyright Roger Bowler, 1999-2012                */
/*              ESA/390 Linkage Stack Operations                     */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements the linkage stack functions of ESA/390     */
/* described in SA22-7201-04 ESA/390 Principles of Operation.        */
/* The numbers in square brackets refer to sections in the manual.   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Fix CR15 corruption in form_stack_entry                Jan Jaeger */
/* Fix nullification in form_stack_entry                  Jan Jaeger */
/* Fix nullification in unstack_registers                 Jan Jaeger */
/* Modifications for Interpretive Execution (SIE)         Jan Jaeger */
/* ESAME low-address protection                   v208d Roger Bowler */
/* ESAME linkage stack operations                 v208e Roger Bowler */
/* TRAP support added                                     Jan Jaeger */
/* Correction to stack types in ESAME mode                Jan Jaeger */
/* ASN-and-LX-reuse facility                  June 2004 Roger Bowler */
/*-------------------------------------------------------------------*/


#include "hstdinc.h"

// #define  STACK_DEBUG    // (#define to enable debugging messages)

#define _STACK_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#ifdef STACK_DEBUG
  #define DBGMSG        if (1) LOGMSG
#else
  #define DBGMSG        if (0) LOGMSG
#endif

/*-------------------------------------------------------------------*/
/* Linkage stack macro definitions                                   */
/*-------------------------------------------------------------------*/
#undef  CR15_LSEA
#undef  LSEA_WRAP
#undef  LSSE_SIZE
#undef  LSSE_REGSIZE
#undef  FETCH_BSEA
#undef  STORE_BSEA
#undef  LSHE_BSEA
#undef  LSHE_RESV
#undef  LSHE_BVALID
#undef  FETCH_FSHA
#undef  LSTE_FSHA
#undef  LSTE_RESV
#undef  LSTE_FVALID

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)

  #define CR15_LSEA     CR15_LSEA_900   /* Bit mask for ESAME linkage
                                           stack entry addr in CR15  */
  #define LSEA_WRAP(_lsea)              /* No address wrap for ESAME */
  #define LSSE_SIZE     296             /* Size of an ESAME linkage
                                           stack state entry         */
  #define LSSE_REGSIZE  8               /* Size of a general register
                                           in ESAME state entry      */

  /* ESAME linkage stack header entry */
  /* LSHE words 1 and 2 contain the backward stack entry address */
  #define FETCH_BSEA(_bsea,_lshe)       FETCH_DW(_bsea,_lshe)
  #define STORE_BSEA(_lshe,_bsea)       STORE_DW(_lshe,_bsea)
  #define LSHE_BSEA     0xFFFFFFFFFFFFFFF8ULL  /* Backward address   */
  #define LSHE_RESV     0x06            /* Reserved bits - must be 0 */
  #define LSHE_BVALID   0x01            /* Backward address is valid */
  /* LSHE words 2 and 3 contain a linkage stack entry descriptor */

  /* ESAME linkage stack trailer entry */
  /* LSTE words 1 and 2 contain the forward section header address */
  #define FETCH_FSHA(_fsha,_lste)       FETCH_DW(_fsha,_lste)
  #define LSTE_FSHA     0xFFFFFFFFFFFFFFF8ULL  /* Forward address    */
  #define LSTE_RESV     0x06            /* Reserved bits - must be 0 */
  #define LSTE_FVALID   0x01            /* Forward address is valid  */
  /* LSTE words 2 and 3 contain a linkage stack entry descriptor */

#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

  #define CR15_LSEA     CR15_LSEA_390   /* Bit mask for ESA/390 linkage
                                           stack entry addr in CR15  */
  #define LSEA_WRAP(_lsea) \
                _lsea &= 0x7FFFFFFF     /* Wrap linkage stack address*/
  #define LSSE_SIZE     168             /* Size of an ESA/390 linkage
                                           stack state entry         */
  #define LSSE_REGSIZE  4               /* Size of a general register
                                           in ESA/390 state entry    */

  /* ESA/390 linkage stack header entry */
  /* LSHE word 0 is reserved for control program use */
  /* LSHE word 1 contains the backward stack entry address */
  #define FETCH_BSEA(_bsea,_lshe)       FETCH_FW(_bsea,(_lshe)+4)
  #define STORE_BSEA(_lshe,_bsea)       STORE_FW((_lshe)+4,_bsea)
  #define LSHE_BVALID   0x80000000      /* Backward address is valid */
  #define LSHE_BSEA     0x7FFFFFF8      /* Backward stack entry addr */
  #define LSHE_RESV     0x00000007      /* Reserved bits - must be 0 */
  /* LSHE words 2 and 3 contain a linkage stack entry descriptor */

  /* ESA/390 linkage stack trailer entry */
  /* LSTE word 0 is reserved for control program use */
  /* LSTE word 1 contains the forward section header address */
  #define FETCH_FSHA(_fsha,_lste)       FETCH_FW(_fsha,(_lste)+4)
  #define LSTE_FVALID   0x80000000      /* Forward address is valid  */
  #define LSTE_FSHA     0x7FFFFFF8      /* Forward section hdr addr  */
  #define LSTE_RESV     0x00000007      /* Reserved bits - must be 0 */
  /* LSTE words 2 and 3 contain a linkage stack entry descriptor */

#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


#if defined(FEATURE_LINKAGE_STACK)

static inline RADR ARCH_DEP(abs_stack_addr) (VADR vaddr, REGS *regs, int acctype)
{
    return MADDR(vaddr, USE_HOME_SPACE, regs, acctype, 0) - regs->mainstor;
}


static inline RADR ARCH_DEP(abs_trap_addr) (VADR vaddr, REGS *regs, int acctype)
{
    return MADDR(vaddr, USE_HOME_SPACE, regs, acctype, regs->psw.pkey) - regs->mainstor;
}

/*-------------------------------------------------------------------*/
/* Subroutine called by the TRAP2 and TRAP4 instructions             */
/*                                                                   */
/* Input:                                                            */
/*      trap4   0=TRAP2 instruction, 1=TRAP4 instruction             */
/*      regs    Pointer to the CPU register context                  */
/*      operand Effective address if TRAP4                           */
/*-------------------------------------------------------------------*/
void ARCH_DEP(trap_x) (int trap_is_trap4, REGS *regs, U32 trap_operand)
{
RADR ducto;
U32  duct11;
U32  tcba;
RADR atcba;
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
U32  tcba0;
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
U32  tsao;
RADR tsaa1,
     tsaa2;
VADR lastbyte;
U32  trap_ia;
U32  trap_flags;
QWORD trap_psw;
int  i;

    if(SIE_STATE_BIT_ON(regs, MX, XC))
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIAL_OPERATION_EXCEPTION);

    if (   REAL_MODE(&regs->psw)
      || !(PRIMARY_SPACE_MODE(&regs->psw)
      ||   ACCESS_REGISTER_MODE(&regs->psw)) )
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIAL_OPERATION_EXCEPTION);

    /* Obtain the DUCT origin from control register 2 */
    ducto = regs->CR(2) & CR2_DUCTO;

    /* Program check if DUCT origin address is invalid */
    if (ducto > regs->mainlim)
        ARCH_DEP(program_interrupt) (regs, PGM_ADDRESSING_EXCEPTION);

    /* Fetch DUCT bytes 44-47 */
    duct11 = ARCH_DEP(fetch_fullword_absolute) (ducto + 44, regs);

    if(!(duct11 & DUCT11_TE))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIAL_OPERATION_EXCEPTION);

    /* Isolate the Trap Control Block Address */
    tcba = duct11 & DUCT11_TCBA;

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    /* Fetch word 0 of the TCB */
    atcba = ARCH_DEP(abs_trap_addr) (tcba, regs, ACCTYPE_READ);
    FETCH_FW(tcba0, regs->mainstor + atcba);
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

    /* Advance to offset +12 */
    tcba += 12;
    atcba = ARCH_DEP(abs_trap_addr) (tcba, regs, ACCTYPE_READ);

    /* Fetch word 3 of the TCB */
    FETCH_FW(tsao, regs->mainstor + atcba);
    tsao &= 0x7FFFFFF8;

    /* Advance to offset +20 */
    tcba += 8; atcba += 8;
    if((atcba & PAGEFRAME_BYTEMASK) < 8)
        atcba = ARCH_DEP(abs_trap_addr) (tcba, regs, ACCTYPE_READ);

    /* Fetch word 5 of the TCB */
    FETCH_FW(trap_ia, regs->mainstor + atcba);
    trap_ia &= 0x7FFFFFFF;

    /* Calculate last byte stored */
    lastbyte = tsao + 95
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
                         + ((tcba0 & TCB0_R) ? 64 : 0)
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
                                                       ;

    /* Use abs_trap_addr as it conforms to trap save area access */
    tsaa1 = tsaa2 = ARCH_DEP(abs_trap_addr) (tsao, regs, ACCTYPE_WRITE);
    if((tsaa1 & PAGEFRAME_PAGEMASK) != (lastbyte & PAGEFRAME_PAGEMASK))
    {
        tsao = lastbyte & PAGEFRAME_PAGEMASK;
        tsaa2 = ARCH_DEP(abs_trap_addr) (tsao, regs, ACCTYPE_WRITE);
    }
    ARCH_DEP( or_storage_key )( tsaa1, STORKEY_CHANGE );
    if (tsaa1 != tsaa2)
        ARCH_DEP( or_storage_key )( tsaa2, STORKEY_CHANGE );


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    /* Special operation exception if P == 0 and EA == 1 */
    if(!(tcba0 & TCB0_P) && regs->psw.amode64)
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIAL_OPERATION_EXCEPTION);
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

  #ifdef FEATURE_TRACING
    if (regs->CR(12) & CR12_BRTRACE)
        regs->CR(12) = ARCH_DEP(trace_br) (1, trap_ia, regs);
  #endif /*FEATURE_TRACING*/

    PER_SB(regs, trap_ia);

    trap_flags = REAL_ILC(regs) << 16;

    if(unlikely(regs->execflag))
        trap_flags |= TRAP0_EXECUTE;

    if(trap_is_trap4)
        trap_flags |= TRAP0_TRAP4;

    /* Trap flags at offset +0 */
    STORE_FW(regs->mainstor + tsaa1, trap_flags);
    /* Reserved zero's stored at offset +4 */
    STORE_FW(regs->mainstor + tsaa1 + 4, 0);

    tsaa1 += 8;
    if((tsaa1 & PAGEFRAME_BYTEMASK) == 0)
        tsaa1 = tsaa2;

    /* Bits 33-63 of Second-Op address of TRAP4 at offset +8 */
    STORE_FW(regs->mainstor + tsaa1, trap_operand);
    /* Access register 15 at offset +12 */
    STORE_FW(regs->mainstor + tsaa1 + 4, regs->AR(15));

    tsaa1 += 8;
    if((tsaa1 & PAGEFRAME_BYTEMASK) == 0)
        tsaa1 = tsaa2;

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    /* If the P bit is one then store the PSW in esame format */
    if(tcba0 & TCB0_P)
        ARCH_DEP(store_psw) (regs, trap_psw);
    else
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    {
        s390_store_psw(regs, trap_psw);
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        /* Set the notesame mode bit for a esa/390 psw */
        trap_psw[1] |= 0x08;
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    }

    /* bits 0-63 of PSW at offset +16 */
    memcpy(regs->mainstor + tsaa1, trap_psw, 8);
    tsaa1 += 8;
    if((tsaa1 & PAGEFRAME_BYTEMASK) == 0)
    {
        tsaa1 = tsaa2;
    }

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    /* If the P bit is one then store the PSW in esame format */
    /* bits 64-127 of PSW at offset +24 */
    if(tcba0 & TCB0_P)
    {
        memcpy(regs->mainstor + tsaa1, trap_psw + 8, 8);
    }
    else
    {
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        memset(regs->mainstor + tsaa1, 0, 8);
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    }
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    tsaa1 += 8;
    if((tsaa1 & PAGEFRAME_BYTEMASK) == 0)
        tsaa1 = tsaa2;

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    /* General registers at offset +32 */
    if(tcba0 & TCB0_R)
        for(i = 0; i < 16; i++)
        {
            STORE_DW(regs->mainstor + tsaa1, regs->GR_G(i));
            tsaa1 += 8;
            if((tsaa1 & PAGEFRAME_BYTEMASK) == 0)
                tsaa1 = tsaa2;
        }
    else
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        for(i = 0; i < 16; i++)
        {
            STORE_FW(regs->mainstor + tsaa1, regs->GR_L(i));
            tsaa1 += 4;
            if((tsaa1 & PAGEFRAME_BYTEMASK) == 0)
                tsaa1 = tsaa2;
        }

    /* Load the Trap Control Block Address in gr15 */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    if(regs->psw.amode64)
        regs->GR(15) = duct11 & DUCT11_TCBA & 0x00000000FFFFFFFF;
    else
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        regs->GR_L(15) = duct11 & DUCT11_TCBA;

    /* Ensure psw.IA is set */
    MAYBE_SET_PSW_IA_FROM_IP(regs);

    /* Set the Breaking Event Address Register */
    if (trap_is_trap4)
    {
        SET_BEAR_REG( regs, regs->ip - 4 );
    }
    else
    {
        SET_BEAR_REG( regs, regs->ip - 2 );
    }

    regs->psw.amode = 1;
    regs->psw.AMASK = AMASK31;
    SET_PSW_IA_AND_MAYBE_IP(regs, trap_ia);
    /* set PSW to primary space */
    regs->psw.asc = 0;
    SET_AEA_MODE(regs);

} /* end function ARCH_DEP(trap_x) */


/*-------------------------------------------------------------------*/
/* Form a new entry on the linkage stack                             */
/*                                                                   */
/* Input:                                                            */
/*      etype   Linkage stack entry type (LSED_UET_PC/BAKR)          */
/*      retna   Return amode and instruction address to be stored    */
/*              in the saved PSW in the new stack entry              */
/*      calla   Called amode and instruction address (for BAKR)      */
/*      csi     32-bit called-space identification (for PC)          */
/*      pcnum   Called PC number (for PC)                            */
/*      regs    Pointer to the CPU register context                  */
/*                                                                   */
/*      This function performs the stacking process for the          */
/*      Branch and Stack (BAKR) and Program Call (PC) instructions.  */
/*                                                                   */
/*      For ESAME, bit 63 of retna/calla indicate a 64-bit address,  */
/*      otherwise bit 32 indicates a 31-bit address.                 */
/*      For ESA/390, bit 0 of retna/calla indicate a 31-bit address. */
/*                                                                   */
/*      For ESAME, bit 0 of pcnum indicates resulting 64-bit mode.   */
/*                                                                   */
/*      In the event of any stack error, this function generates     */
/*      a program check and does not return.                         */
/*-------------------------------------------------------------------*/
void ARCH_DEP(form_stack_entry) (BYTE etype, VADR retna, VADR calla,
                                U32 csi, U32 pcnum, REGS *regs)
{
QWORD   currpsw;                        /* Current PSW               */
VADR    lsea;                           /* Linkage stack entry addr  */
VADR    lseaold;                        /* Linkage stack old addr    */
RADR    abs, abs2 = 0;                  /* Absolute addr new entry   */
RADR    absold;                         /* Absolute addr old entry   */
LSED    lsed;                           /* Linkage stack entry desc. */
LSED    lsed2;                          /* New entry descriptor      */
U16     rfs;                            /* Remaining free space      */
VADR    fsha;                           /* Forward section hdr addr  */
VADR    bsea = 0;                       /* Backward stack entry addr */
RADR    absea = 0;                      /* Absolute address of bsea  */
int     i;                              /* Array subscript           */

    /* [5.12.3.1] Locate space for a new linkage stack entry */

    /* Obtain the virtual address of the current entry from CR15 */
    lsea = regs->CR(15) & CR15_LSEA;

    /* Fetch the entry descriptor of the current entry */
    absold = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);
    memcpy (&lsed, regs->mainstor+absold, sizeof(LSED));
    lseaold = lsea;

    DBGMSG( "stack: Current stack entry at " F_VADR "\n", lsea );
    DBGMSG( "stack: et=%2.2X si=%2.2X rfs=%2.2X%2.2X nes=%2.2X%2.2X\n",
            lsed.uet, lsed.si, lsed.rfs[0],
            lsed.rfs[1], lsed.nes[0], lsed.nes[1] );

    /* Check whether the current linkage stack section has enough
       remaining free space to contain the new stack entry */
    FETCH_HW(rfs,lsed.rfs);
    if (rfs < LSSE_SIZE)
    {
        /* Program check if remaining free space not a multiple of 8 */
        if ((rfs & 0x07) != 0)
            ARCH_DEP(program_interrupt) (regs, PGM_STACK_SPECIFICATION_EXCEPTION);

        /* Not enough space, so fetch the forward section header addr
           from the trailer entry of current linkage stack section */
        lsea += sizeof(LSED) + rfs;
        LSEA_WRAP(lsea);
        abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);
        FETCH_FSHA(fsha, regs->mainstor + abs);

        DBGMSG( "stack: Forward section header addr " F_VADR "\n", fsha );

        /* Stack full exception if forward address is not valid */
        if ((fsha & LSTE_FVALID) == 0)
            ARCH_DEP(program_interrupt) (regs, PGM_STACK_FULL_EXCEPTION);

        /* Extract the forward section header address, which points to
           the entry descriptor (words 2-3) of next section's header */
        fsha &= LSTE_FSHA;

        /* Fetch the entry descriptor of the next section's header */
        absold = ARCH_DEP(abs_stack_addr) (fsha, regs, ACCTYPE_READ);
        memcpy (&lsed, regs->mainstor+absold, sizeof(LSED));
        lseaold = fsha;

        DBGMSG("stack: et=%2.2X si=%2.2X rfs=%2.2X%2.2X "
                "nes=%2.2X%2.2X\n",
                lsed.uet, lsed.si, lsed.rfs[0],
                lsed.rfs[1], lsed.nes[0], lsed.nes[1]);

        /* Program check if the next linkage stack section does not
           have enough free space to contain the new stack entry */
        FETCH_HW(rfs,lsed.rfs);
        if (rfs < LSSE_SIZE)
            ARCH_DEP(program_interrupt) (regs, PGM_STACK_SPECIFICATION_EXCEPTION);

        /* Calculate the virtual address of the new section's header
           entry, which is 8 bytes before the entry descriptor */
        lsea = fsha - 8;
        LSEA_WRAP(lsea);

        /* Form the backward stack entry address */
        bsea = LSHE_BVALID | (regs->CR(15) & CR15_LSEA);
        absea = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_WRITE);

        /* Use the virtual address of the entry descriptor of the
           new section's header entry as the current entry address */
        lsea = fsha;

    } /* end if(rfs<LSSE_SIZE) */

    /* [5.12.3.2] Form the new stack entry */

    /* Calculate the virtual address of the new stack entry */
    lsea += sizeof(LSED);
    LSEA_WRAP(lsea);

    /* Obtain absolute address of the new stack entry */
    abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_WRITE);

    /* If new stack entry will cross a page boundary, obtain the
       absolute address of the second page of the stack entry */
    if(((lsea + (LSSE_SIZE - 1)) & PAGEFRAME_PAGEMASK)
                                != (lsea & PAGEFRAME_PAGEMASK))
        abs2 = ARCH_DEP(abs_stack_addr)
                        ((lsea + (LSSE_SIZE - 1)) & PAGEFRAME_PAGEMASK,
                        regs, ACCTYPE_WRITE);

    DBGMSG( "stack: New stack entry at " F_VADR "\n", lsea );

    /* If a new section then place updated backward stack
       entry address in the new section's header entry */
    if(bsea)
        STORE_BSEA(regs->mainstor + absea, bsea);

    /* Store general registers 0-15 in bytes 0-63 (ESA/390)
       or bytes 0-127 (ESAME) of the new state entry */
    for (i = 0; i < 16; i++)
    {
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        /* Store the 64-bit general register in the stack entry */
        STORE_DW(regs->mainstor + abs, regs->GR_G(i));

        DBGMSG("stack: GPR%d=" F_GREG " stored at V:" F_VADR
                " A:" F_RADR "\n", i, regs->GR_G(i), lsea, abs);
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        /* Store the 32-bit general register in the stack entry */
        STORE_FW(regs->mainstor + abs, regs->GR_L(i));

        DBGMSG("stack: GPR%d=" F_GREG " stored at V:" F_VADR
                " A:" F_RADR "\n", i, regs->GR_L(i), lsea, abs);
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

        /* Update the virtual and absolute addresses */
        lsea += LSSE_REGSIZE;
        LSEA_WRAP(lsea);
        abs += LSSE_REGSIZE;

        /* Recalculate absolute address if page boundary crossed */
        if ((lsea & PAGEFRAME_BYTEMASK) == 0x000)
            abs = abs2;

    } /* end for(i) */

#if !defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    /* For ESA/390, store access registers 0-15 in bytes 64-127 */
    for (i = 0; i < 16; i++)
    {
        /* Store the access register in the stack entry */
        STORE_FW(regs->mainstor + abs, regs->AR(i));

        DBGMSG("stack: AR%d=" F_AREG " stored at V:" F_VADR
                " A:" F_RADR "\n", i, regs->AR(i), lsea, abs);

        /* Update the virtual and absolute addresses */
        lsea += 4;
        LSEA_WRAP(lsea);
        abs += 4;

        /* Recalculate absolute address if page boundary crossed */
        if ((lsea & PAGEFRAME_BYTEMASK) == 0x000)
            abs = abs2;

    } /* end for(i) */
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

    /* Store the PKM, SASN, EAX, and PASN in bytes 128-135 */
    STORE_FW(regs->mainstor + abs, regs->CR_L(3));
    STORE_HW(regs->mainstor + abs + 4, regs->CR_LHH(8));
    STORE_HW(regs->mainstor + abs + 6, regs->CR_LHL(4));

    DBGMSG("stack: PKM=%2.2X%2.2X SASN=%2.2X%2.2X "
            "EAX=%2.2X%2.2X PASN=%2.2X%2.2X \n"
            "stored at V:" F_VADR " A:" F_RADR "\n",
            regs->mainstor[abs], regs->mainstor[abs+1],
            regs->mainstor[abs+2], regs->mainstor[abs+3],
            regs->mainstor[abs+4], regs->mainstor[abs+5],
            regs->mainstor[abs+6], regs->mainstor[abs+7],
            lsea, abs);

    /* Update virtual and absolute addresses to point to byte 136 */
    lsea += 8;
    LSEA_WRAP(lsea);
    abs += 8;

    /* Recalculate absolute address if page boundary crossed */
    if ((lsea & PAGEFRAME_BYTEMASK) == 0x000)
        abs = abs2;

    /* Store bits 0-63 of the current PSW in bytes 136-143 */
    ARCH_DEP(store_psw) (regs, currpsw);
    memcpy (regs->mainstor + abs, currpsw, 8);

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    /* For ESAME, use the addressing mode bits from the return
       address to set bits 31 and 32 of bytes 136-143 */
    if (retna & 0x01)
    {
        /* For a 64-bit return address, set bits 31 and 32 */
        regs->mainstor[abs+3] |= 0x01;
        regs->mainstor[abs+4] |= 0x80;
        retna &= 0xFFFFFFFFFFFFFFFEULL;
    }
    else if (retna & 0x80000000)
    {
        /* For a 31-bit return address, clear bit 31 and set bit 32 */
        regs->mainstor[abs+3] &= 0xFE;
        regs->mainstor[abs+4] |= 0x80;
        retna &= 0x7FFFFFFF;
    }
    else
    {
        /* For a 24-bit return address, clear bits 31 and 32 */
        regs->mainstor[abs+3] &= 0xFE;
        regs->mainstor[abs+4] &= 0x7F;
        retna &= 0x00FFFFFF;
    }
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    /* For ESA/390, replace bytes 140-143 by the return address,
       with the high-order bit indicating the addressing mode */
    STORE_FW(regs->mainstor + abs + 4, retna);
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

    DBGMSG("stack: PSW=%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
            "stored at V:" F_VADR " A:" F_RADR "\n",
            regs->mainstor[abs], regs->mainstor[abs+1],
            regs->mainstor[abs+2], regs->mainstor[abs+3],
            regs->mainstor[abs+4], regs->mainstor[abs+5],
            regs->mainstor[abs+6], regs->mainstor[abs+7],
            lsea, abs);

    /* Update virtual and absolute addresses to point to byte 144 */
    lsea += 8;
    LSEA_WRAP(lsea);
    abs += 8;

    /* Recalculate absolute address if page boundary crossed */
    if ((lsea & PAGEFRAME_BYTEMASK) == 0x000)
        abs = abs2;

    /* Store bytes 144-151 according to PC or BAKR */
    if (etype == LSED_UET_PC)
    {
      #if defined(FEATURE_CALLED_SPACE_IDENTIFICATION)
        /* Store the called-space identification in bytes 144-147 */
        STORE_FW(regs->mainstor + abs, csi);
      #endif /*defined(FEATURE_CALLED_SPACE_IDENTIFICATION)*/

        /* Store the PC number in bytes 148-151 */
        STORE_FW(regs->mainstor + abs + 4, pcnum);
    }
    else
    {
      #if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        /* Store the called address and amode in bytes 144-151 */
        STORE_DW(regs->mainstor + abs, calla);
      #else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        /* Store the called address and amode in bytes 148-151 */
        STORE_FW(regs->mainstor + abs + 4, calla);
      #endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    }

    /* Update virtual and absolute addresses to point to byte 152 */
    lsea += 8;
    LSEA_WRAP(lsea);
    abs += 8;

    /* Recalculate absolute address if page boundary crossed */
    if ((lsea & PAGEFRAME_BYTEMASK) == 0x000)
        abs = abs2;

    /* Store zeroes in bytes 152-159 */
    memset (regs->mainstor+abs, 0, 8);

    /* Update virtual and absolute addresses to point to byte 160 */
    lsea += 8;
    LSEA_WRAP(lsea);
    abs += 8;

    /* Recalculate absolute address if page boundary crossed */
    if ((lsea & PAGEFRAME_BYTEMASK) == 0x000)
        abs = abs2;

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    /* For ESAME, store zeroes in bytes 160-167 */
    memset (regs->mainstor+abs, 0, 8);

    /* Update virtual and absolute addresses to point to byte 168 */
    lsea += 8;
    LSEA_WRAP(lsea);
    abs += 8;

    /* Recalculate absolute address if page boundary crossed */
    if ((lsea & PAGEFRAME_BYTEMASK) == 0x000)
        abs = abs2;

    /* For ESAME, store the return address in bytes 168-175 */
    STORE_DW (regs->mainstor + abs, retna);

    DBGMSG("stack: PSW2=%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
            "stored at V:" F_VADR " A:" F_RADR "\n",
            regs->mainstor[abs], regs->mainstor[abs+1],
            regs->mainstor[abs+2], regs->mainstor[abs+3],
            regs->mainstor[abs+4], regs->mainstor[abs+5],
            regs->mainstor[abs+6], regs->mainstor[abs+7],
            lsea, abs);

    /* Update virtual and absolute addresses to point to byte 176 */
    lsea += 8;
    LSEA_WRAP(lsea);
    abs += 8;

    /* Recalculate absolute address if page boundary crossed */
    if ((lsea & PAGEFRAME_BYTEMASK) == 0x000)
        abs = abs2;

    /* If ASN-and-LX-reuse is installed and active, store
       the SASTEIN (CR3 bits 0-31) in bytes 176-179, and
       store the PASTEIN (CR4 bits 0-31) in bytes 180-183 */
    if (ASN_AND_LX_REUSE_ENABLED(regs))
    {
        STORE_FW(regs->mainstor + abs, regs->CR_H(3));
        STORE_FW(regs->mainstor + abs + 4, regs->CR_H(4));

        DBGMSG("stack: SASTEIN=%2.2X%2.2X%2.2X%2.2X "
                "PASTEIN=%2.2X%2.2X%2.2X%2.2X \n"
                "stored at V:" F_VADR " A:" F_RADR "\n",
                regs->mainstor[abs], regs->mainstor[abs+1],
                regs->mainstor[abs+2], regs->mainstor[abs+3],
                regs->mainstor[abs+4], regs->mainstor[abs+5],
                regs->mainstor[abs+6], regs->mainstor[abs+7],
                lsea, abs);

    } /* end if(ASN_AND_LX_REUSE_ENABLED) */

    /* Skip bytes 176-223 of the new stack entry */
    lsea += 48;
    LSEA_WRAP(lsea);
    abs += 48;

    /* Recalculate absolute address if page boundary crossed */
    if ((lsea & PAGEFRAME_BYTEMASK) < 48)
        abs = abs2 | (lsea & PAGEFRAME_BYTEMASK);

    /* For ESAME, store access registers 0-15 in bytes 224-287 */
    for (i = 0; i < 16; i++)
    {
        /* Store the access register in the stack entry */
        STORE_FW(regs->mainstor + abs, regs->AR(i));

        DBGMSG("stack: AR%d=" F_AREG " stored at V:" F_VADR
                " A:" F_RADR "\n", i, regs->AR(i), lsea, abs);

        /* Update the virtual and absolute addresses */
        lsea += 4;
        LSEA_WRAP(lsea);
        abs += 4;

        /* Recalculate absolute address if page boundary crossed */
        if ((lsea & PAGEFRAME_BYTEMASK) == 0x000)
            abs = abs2;

    } /* end for(i) */
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

    /* Build the new linkage stack entry descriptor */
    memset (&lsed2, 0, sizeof(LSED));
    lsed2.uet = etype & LSED_UET_ET;
    lsed2.si = lsed.si;
    rfs -= LSSE_SIZE;
    STORE_HW(lsed2.rfs,rfs);

    /* Store the linkage stack entry descriptor in the last eight
       bytes of the new state entry (bytes 160-167 for ESA/390,
       or bytes 288-295 for ESAME) */
    memcpy (regs->mainstor+abs, &lsed2, sizeof(LSED));

    DBGMSG( "stack: New stack entry at " F_VADR "\n", lsea );
    DBGMSG( "stack: et=%2.2X si=%2.2X rfs=%2.2X%2.2X nes=%2.2X%2.2X\n",
            lsed2.uet, lsed2.si, lsed2.rfs[0],
            lsed2.rfs[1], lsed2.nes[0], lsed2.nes[1] );

    /* [5.12.3.3] Update the current entry */
    STORE_HW(lsed.nes, LSSE_SIZE);
    absold = ARCH_DEP(abs_stack_addr) (lseaold, regs, ACCTYPE_WRITE);
    memcpy (regs->mainstor+absold, &lsed, sizeof(LSED));

    DBGMSG( "stack: Previous stack entry updated at A:" F_RADR "\n",
            absold );
    DBGMSG( "stack: et=%2.2X si=%2.2X rfs=%2.2X%2.2X nes=%2.2X%2.2X\n",
            lsed.uet, lsed.si, lsed.rfs[0],
            lsed.rfs[1], lsed.nes[0], lsed.nes[1] );

    /* [5.12.3.4] Update control register 15 */
    regs->CR(15) = lsea & CR15_LSEA;

    DBGMSG( "stack: CR15=" F_CREG "\n", regs->CR( 15 ));

} /* end function ARCH_DEP(form_stack_entry) */

/*-------------------------------------------------------------------*/
/* Locate the current linkage stack entry                            */
/*                                                                   */
/* Input:                                                            */
/*      prinst  1=PR instruction, 0=EREG/EREGG/ESTA/MSTA instruction */
/*      lsedptr Pointer to an LSED structure                         */
/*      regs    Pointer to the CPU register context                  */
/* Output:                                                           */
/*      The entry descriptor for the current state entry in the      */
/*      linkage stack is copied into the LSED structure.             */
/*      The home virtual address of the entry descriptor is          */
/*      returned as the function return value.                       */
/*                                                                   */
/*      This function performs the first part of the unstacking      */
/*      process for the Program Return (PR), Extract Stacked         */
/*      Registers (EREG/EREGG), Extract Stacked State (ESTA),        */
/*      and Modify Stacked State (MSTA) instructions.                */
/*                                                                   */
/*      In the event of any stack error, this function generates     */
/*      a program check and does not return.                         */
/*-------------------------------------------------------------------*/
VADR ARCH_DEP(locate_stack_entry) (int prinst, LSED *lsedptr,
                                    REGS *regs)
{
VADR    lsea;                           /* Linkage stack entry addr  */
RADR    abs;                            /* Absolute address          */
VADR    bsea;                           /* Backward stack entry addr */

    /* [5.12.4] Special operation exception if ASF is not enabled,
       or if DAT is off, or if in secondary-space mode */
    if (!ASF_ENABLED(regs)
        || REAL_MODE(&regs->psw)
        || SECONDARY_SPACE_MODE(&regs->psw))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIAL_OPERATION_EXCEPTION);

    /* Special operation exception if home space mode PR instruction */
    if (prinst && HOME_SPACE_MODE(&regs->psw))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIAL_OPERATION_EXCEPTION);

    /* [5.12.4.1] Locate current entry and process header entry */

    /* Obtain the virtual address of the current entry from CR15 */
    lsea = regs->CR(15) & CR15_LSEA;

    /* Fetch the entry descriptor of the current entry */
    abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);
    memcpy (lsedptr, regs->mainstor+abs, sizeof(LSED));

    DBGMSG( "stack: Stack entry located at " F_VADR "\n", lsea );
    DBGMSG( "stack: et=%2.2X si=%2.2X rfs=%2.2X%2.2X nes=%2.2X%2.2X\n",
            lsedptr->uet, lsedptr->si, lsedptr->rfs[0],
            lsedptr->rfs[1], lsedptr->nes[0], lsedptr->nes[1] );

    /* Check for a header entry */
    if ((lsedptr->uet & LSED_UET_ET) == LSED_UET_HDR)
    {
        /* For PR instruction only, generate stack operation exception
           if the unstack suppression bit in the header entry is set */
        if (prinst && (lsedptr->uet & LSED_UET_U))
            ARCH_DEP(program_interrupt) (regs, PGM_STACK_OPERATION_EXCEPTION);

        /* Calculate the virtual address of the header entry,
           which is 8 bytes before the entry descriptor */
        lsea -= 8;
        LSEA_WRAP(lsea);

        /* Fetch the backward stack entry address from the header */
        abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);
        FETCH_BSEA(bsea,regs->mainstor + abs);

        DBGMSG( "stack: Stack entry located at " F_VADR "\n", bsea );

        /* Stack empty exception if backward address is not valid */
        if ((bsea & LSHE_BVALID) == 0)
            ARCH_DEP(program_interrupt) (regs, PGM_STACK_EMPTY_EXCEPTION);

        /* Extract the virtual address of the entry descriptor
           of the last entry in the previous section */
        lsea = bsea & LSHE_BSEA;

        /* Fetch the entry descriptor of the designated entry */
        abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);
        memcpy (lsedptr, regs->mainstor+abs, sizeof(LSED));

        DBGMSG("stack: et=%2.2X si=%2.2X rfs=%2.2X%2.2X "
                "nes=%2.2X%2.2X\n",
                lsedptr->uet, lsedptr->si, lsedptr->rfs[0],
                lsedptr->rfs[1], lsedptr->nes[0], lsedptr->nes[1]);

        /* Stack specification exception if this is also a header */
        if ((lsedptr->uet & LSED_UET_ET) == LSED_UET_HDR)
            ARCH_DEP(program_interrupt) (regs, PGM_STACK_SPECIFICATION_EXCEPTION);

    } /* end if(LSED_UET_HDR) */

    /* [5.12.4.2] Check for a state entry */

    /* Stack type exception if this is not a state entry */
    if ((lsedptr->uet & LSED_UET_ET) != LSED_UET_BAKR
        && (lsedptr->uet & LSED_UET_ET) != LSED_UET_PC)
        ARCH_DEP(program_interrupt) (regs, PGM_STACK_TYPE_EXCEPTION);

    /* [5.12.4.3] For PR instruction only, stack operation exception
       if the unstack suppression bit in the state entry is set */
    if (prinst && (lsedptr->uet & LSED_UET_U))
        ARCH_DEP(program_interrupt) (regs, PGM_STACK_OPERATION_EXCEPTION);

    /* Return the virtual address of the entry descriptor */
    return lsea;

} /* end function ARCH_DEP(locate_stack_entry) */

/*-------------------------------------------------------------------*/
/* Stack modify                                                      */
/*                                                                   */
/* Input:                                                            */
/*      lsea    Virtual address of linkage stack entry descriptor    */
/*      m1      Left 32 bits to be stored in state entry             */
/*      m2      Right 32 bits to be stored in state entry            */
/*      regs    Pointer to the CPU register context                  */
/*                                                                   */
/*      This function places eight bytes of information into the     */
/*      modifiable area of a state entry in the linkage stack.  It   */
/*      is called by the Modify Stacked State (MSTA) instruction     */
/*      after it has located the current state entry.                */
/*                                                                   */
/*      If a translation exception occurs when accessing the stack   */
/*      entry, then a program check will be generated by the         */
/*      abs_stack_addr subroutine, and the function will not return. */
/*-------------------------------------------------------------------*/
void ARCH_DEP(stack_modify) (VADR lsea, U32 m1, U32 m2, REGS *regs)
{
RADR    abs;                            /* Absolute address          */

    /* Point back to byte 152 of the state entry */
    lsea -= LSSE_SIZE - sizeof(LSED);
    lsea += 152;
    LSEA_WRAP(lsea);

    /* Store the modify values into the state entry */
    abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_WRITE);
    STORE_FW(regs->mainstor + abs, m1);
    STORE_FW(regs->mainstor + abs + 4, m2);

} /* end function ARCH_DEP(stack_modify) */

/*-------------------------------------------------------------------*/
/* Stack extract                                                     */
/*                                                                   */
/* Input:                                                            */
/*      lsea    Virtual address of linkage stack entry descriptor    */
/*      r1      The number of an even-odd pair of registers          */
/*      code    A code indicating which bytes are to be extracted:   */
/*              0 = Bytes 128-135 (PKN/SASN/EAX/PASN)                */
/*              1 = ESA/390: Bytes 136-143 (PSW)                     */
/*                  ESAME: Bytes 136-139, 140.0, 168-175.33-63       */
/*                         (ESA/390-format PSW)                      */
/*              2 = Bytes 144-151 (Branch address or PC number)      */
/*              3 = Bytes 152-159 (Modifiable area)                  */
/*              4 = Bytes 136-143 and 168-175 (ESAME-format PSW)     */
/*              5 = Bytes 176-183 (SASTEIN,PASTEIN)                  */
/*      regs    Pointer to the CPU register context                  */
/*                                                                   */
/*      This function extracts 64 or 128 bits of information from    */
/*      the status area of a state entry in the linkage stack.  It   */
/*      is called by the Extract Stacked State (ESTA) instruction    */
/*      after it has located the current state entry.                */
/*                                                                   */
/*      For codes 0 through 3, the rightmost 32 bits of the R1 and   */
/*      R1+1 registers are updated (the leftmost 32 bits remain      */
/*      unchanged for ESAME).  For code 4, which is valid only for   */
/*      ESAME, all 64 bits of the R1 and R1+1 registers are loaded.  */
/*      For code 5 (valid only if the ASN-and-LX-reuse facility is   */
/*      installed), the leftmost 32 bits of the R1 and R1+1 regs     */
/*      are updated, and the rightmost 32 bits remain unchanged.     */
/*                                                                   */
/*      If a translation exception occurs when accessing the stack   */
/*      entry, then a program check will be generated by the         */
/*      abs_stack_addr subroutine, and the function will not return. */
/*-------------------------------------------------------------------*/
void ARCH_DEP(stack_extract) (VADR lsea, int r1, int code, REGS *regs)
{
RADR    abs;                            /* Absolute address          */

    /* Point back to byte 128 of the state entry */
    lsea -= LSSE_SIZE - sizeof(LSED);
    lsea += 128;

  #if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    /* For codes 1 and 4, extract bytes 136-143 and 168-175 */
    if (code == 1 || code == 4)
    {
        U64 psw1, psw2;

        /* Point to byte 136 of the state entry */
        lsea += 8;
        LSEA_WRAP(lsea);

        /* Load bits 0-63 of ESAME PSW from bytes 136-143 */
        abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);
        FETCH_DW(psw1, regs->mainstor + abs);

        /* Point to byte 168 of the state entry */
        lsea += 32;
        abs += 32;

        /* Recalculate absolute address if page boundary crossed */
        if ((lsea & PAGEFRAME_BYTEMASK) < 32)
            abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);

        /* Load bits 64-127 of ESAME PSW from bytes 168-175 */
        FETCH_DW(psw2, regs->mainstor + abs);

        /* For code 4, return ESAME PSW in general register pair */
        if (code == 4)
        {
            regs->GR_G(r1) = psw1;
            regs->GR_G(r1+1) = psw2;
            return;
        }

        /* For code 1, convert ESAME PSW to ESA/390 format */
        regs->GR_L(r1) = (psw1 >> 32) | 0x00080000;
        regs->GR_L(r1+1) = (psw1 & 0x80000000)
                            | (psw2 & 0x7FFFFFFF);

        /* Set low-order bit of R1+1 if IA exceeds 31-bit address */
        if (psw2 > 0x7FFFFFFF)
            regs->GR_L(r1+1) |= 0x01;

        return;

    } /* if(code==1||code==4) */
  #endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

  #if defined(FEATURE_006_ASN_LX_REUSE_FACILITY)
    /* For code 5, extract bytes 176-183 */
    if (code == 5)
    {
        /* Point to byte 176 of the state entry */
        lsea += 48;
        LSEA_WRAP(lsea);

        /* Load the SASTEIN, PASTEIN from bytes 176-179, 180-183*/
        abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);
        FETCH_FW(regs->GR_H(r1), regs->mainstor + abs);
        FETCH_FW(regs->GR_H(r1+1), regs->mainstor + abs + 4);

        return;

    } /* if(code==5) */
  #endif /*defined(FEATURE_006_ASN_LX_REUSE_FACILITY)*/

    /* For codes 0,2,3 in ESAME, and codes 0,1,2,3 in ESA/390 */
    /* Point to byte 128, 136, 144, or 152 depending on the code */
    lsea += code * 8;
    LSEA_WRAP(lsea);

    /* Load the general register pair from the state entry */
    abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);
    FETCH_FW(regs->GR_L(r1), regs->mainstor + abs);
    FETCH_FW(regs->GR_L(r1+1), regs->mainstor + abs + 4);

} /* end function ARCH_DEP(stack_extract) */

/*-------------------------------------------------------------------*/
/* Unstack registers                                                 */
/*                                                                   */
/* Input:                                                            */
/*      gtype   0=EREG instruction, 1=EREGG or PR instruction        */
/*      lsea    Virtual address of linkage stack entry descriptor    */
/*      r1      The number of the first register to be loaded        */
/*      r2      The number of the last register to be loaded         */
/*      regs    Pointer to the CPU register context                  */
/*                                                                   */
/*      This function loads a range of general registers and         */
/*      access registers from the specified linkage stack entry.     */
/*      It is called by the Extract Stacked Registers (EREG/EREGG)   */
/*      and Program Return (PR) instructions after they have located */
/*      the current state entry in the linkage stack.                */
/*                                                                   */
/*      If a translation exception occurs when accessing the stack   */
/*      entry, then a program check will be generated by the         */
/*      abs_stack_addr subroutine, and the function will not return. */
/*      Since the stack entry can only span at most two pages, and   */
/*      the caller will have already successfully accessed the       */
/*      entry descriptor which is at the end of the stack entry,     */
/*      the only place a translation exception can occur is when     */
/*      attempting to load the first register, in which case the     */
/*      operation is nullified with all registers unchanged.         */
/*-------------------------------------------------------------------*/
void ARCH_DEP(unstack_registers) (int gtype, VADR lsea,
                                int r1, int r2, REGS *regs)
{
RADR    abs, abs2 = 0;                  /* Absolute address          */
VADR    firstbyte,                      /* First byte to be fetched  */
        lastbyte;                       /* Last byte to be fetched   */
int     i;                              /* Array subscript           */

    UNREFERENCED(gtype);

    /* Point back to byte 0 of the state entry */
    lsea -= LSSE_SIZE - sizeof(LSED);
    LSEA_WRAP(lsea);

    /* Determine first and last byte to fetch from the state entry */
    firstbyte = lsea + ((r1 > r2) ? 0 : r1) * LSSE_REGSIZE;
    lastbyte = lsea + (LSSE_SIZE - 69) + (((r1 > r2) ? 15 : r2) * 4);

    lsea = firstbyte;

    /* Obtain absolute address of the state entry */
    abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);

    /* If the state entry crosses a page boundary, obtain the
       absolute address of the second page of the stack entry */
    if( (firstbyte & PAGEFRAME_PAGEMASK)
                                != (lastbyte & PAGEFRAME_PAGEMASK))
        abs2 = ARCH_DEP(abs_stack_addr)
                 (lastbyte & PAGEFRAME_PAGEMASK, regs, ACCTYPE_READ);

    DBGMSG( "stack: Unstacking registers %d-%d from " F_VADR "\n",
            r1, r2, lsea );

    /* Load general registers from bytes 0-63 (for ESA/390), or
       bytes 0-127 (for ESAME) of the state entry */
    for (i = ((r1 > r2) ? 0 : r1); i <= 15; i++)
    {
        /* Load the general register from the stack entry */
        if ((r1 <= r2 && i >= r1 && i <= r2)
            || (r1 > r2 && (i >= r1 || i <= r2)))
        {
    #if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
            if (gtype)
            {
                /* For ESAME PR and EREGG instructions,
                   load all 64 bits of the register */
                FETCH_DW(regs->GR_G(i), regs->mainstor + abs);
            } else {
                /* For ESAME EREG instruction, load bits 32-63 of
                   the register, and leave bits 0-31 unchanged */
                FETCH_FW(regs->GR_L(i), regs->mainstor + abs + 4);
            }

            DBGMSG("stack: GPR%d=" F_GREG " loaded from V:" F_VADR
                    " A:" F_RADR "\n", i, regs->GR(i), lsea, abs);
    #else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
            /* For ESA/390, load a 32-bit general register */
            FETCH_FW(regs->GR_L(i), regs->mainstor + abs);

            DBGMSG("stack: GPR%d=" F_GREG " loaded from V:" F_VADR
                    " A:" F_RADR "\n", i, regs->GR(i), lsea, abs);
    #endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        }

        /* Update the virtual and absolute addresses */
        lsea += LSSE_REGSIZE;
        LSEA_WRAP(lsea);
        abs += LSSE_REGSIZE;

        /* Recalculate absolute address if page boundary crossed */
        if ((lsea & PAGEFRAME_BYTEMASK) == 0x000)
            abs = abs2;

    } /* end for(i) */

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    /* For ESAME, skip the next 96 bytes of the state entry */
    lsea += 96; abs += 96;

    /* Recalculate absolute address if page boundary crossed */
    if ((lsea & PAGEFRAME_BYTEMASK) < 96)
        abs = abs2 | (lsea & PAGEFRAME_BYTEMASK);
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

    /* Load access registers from bytes 64-127 (for ESA/390), or
       bytes 224-280 (for ESAME) of the state entry */
    for (i = 0; i <= ((r1 > r2) ? 15 : r2); i++)
    {
        /* Load the access register from the stack entry */
        if ((r1 <= r2 && i >= r1 && i <= r2)
            || (r1 > r2 && (i >= r1 || i <= r2)))
        {
            FETCH_FW(regs->AR(i),regs->mainstor + abs);
            SET_AEA_AR(regs, i);

            DBGMSG("  stack: AR%d=" F_AREG " loaded from V:" F_VADR
                    " A:" F_RADR "\n", i, regs->AR(i), lsea, abs);
        }

        /* Update the virtual and absolute addresses */
        lsea += 4;
        LSEA_WRAP(lsea);
        abs += 4;

        /* Recalculate absolute address if page boundary crossed */
        if ((lsea & PAGEFRAME_BYTEMASK) == 0x000)
            abs = abs2;

    } /* end for(i) */

} /* end function ARCH_DEP(unstack_registers) */

/*-------------------------------------------------------------------*/
/* Program return unstack                                            */
/*                                                                   */
/* Input:                                                            */
/*      regs    Pointer to a copy of the CPU register context        */
/* Output:                                                           */
/*      lsedap  The absolute address of the entry descriptor of      */
/*              the new current entry on the linkage stack.          */
/*      rc      Return code from load_psw, checked later for PIC 06  */
/* Return value:                                                     */
/*      The type of entry unstacked: LSED_UET_BAKR or LSED_UET_PC    */
/*                                                                   */
/*      This function performs the restoring and updating parts      */
/*      of the unstacking process for the Program Return (PR)        */
/*      instruction.  If a program exception occurs during the PR    */
/*      instruction (either during or after the unstack), then the   */
/*      effects of the instruction must be nullified or suppressed.  */
/*      This is achieved by updating a copy of the CPU register      */
/*      context instead of the actual register context.              */
/*      The current register context is replaced by the copy         */
/*      only on successful completion of the PR instruction.         */
/*                                                                   */
/*      In the event of any stack error, this function generates     */
/*      a program check and does not return.                         */
/*-------------------------------------------------------------------*/
int ARCH_DEP(program_return_unstack) (REGS *regs, RADR *lsedap, int *rc)
{
QWORD   newpsw;                         /* New PSW                   */
LSED    lsed;                           /* Linkage stack entry desc. */
VADR    lsea;                           /* Linkage stack entry addr  */
RADR    abs;                            /* Absolute address          */
int     permode;                        /* 1=PER mode is set in PSW  */
U16     pkm;                            /* PSW key mask              */
U16     sasn;                           /* Secondary ASN             */
U16     eax;                            /* Extended AX               */
U16     pasn;                           /* Primary ASN               */
VADR    lsep;                           /* Virtual addr of entry desc.
                                           of previous stack entry   */

    /* Find the virtual address of the entry descriptor
       of the current state entry in the linkage stack */
    lsea = ARCH_DEP(locate_stack_entry) (1, &lsed, regs);

    /* [5.12.4.3] Restore information from stack entry */

    /* Load registers 2-14 from the stack entry */
    ARCH_DEP(unstack_registers) (1, lsea, 2, 14, regs);

    /* Point back to the entry descriptor of previous stack entry */
    lsep = lsea - LSSE_SIZE;
    LSEA_WRAP(lsep);

    /* Point back to byte 128 of the current state entry */
    lsea -= LSSE_SIZE - sizeof(LSED);
    lsea += 128;
    LSEA_WRAP(lsea);

    /* Translate virtual address to absolute address */
    abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);

    /* For a call state entry, replace the PKM, SASN, EAX, and PASN */
    if ((lsed.uet & LSED_UET_ET) == LSED_UET_PC)
    {
        /* Fetch the PKM from bytes 128-129 of the stack entry */
        FETCH_HW(pkm,regs->mainstor + abs);

        /* Fetch the SASN from bytes 130-131 of the stack entry */
        FETCH_HW(sasn,regs->mainstor + abs + 2);

        /* Fetch the EAX from bytes 132-133 of the stack entry */
        FETCH_HW(eax,regs->mainstor + abs + 4);

        /* Fetch the PASN from bytes 134-135 of the stack entry */
        FETCH_HW(pasn,regs->mainstor + abs + 6);

        DBGMSG("  stack: PKM=%2.2X%2.2X SASN=%2.2X%2.2X "
                "EAX=%2.2X%2.2X PASN=%2.2X%2.2X \n"
                "loaded from V:" F_VADR " A:" F_RADR "\n",
                regs->mainstor[abs], regs->mainstor[abs+1],
                regs->mainstor[abs+2], regs->mainstor[abs+3],
                regs->mainstor[abs+4], regs->mainstor[abs+5],
                regs->mainstor[abs+6], regs->mainstor[abs+7],
                lsea, abs);

        /* Load PKM into CR3 bits 0-15 (32-47) */
        regs->CR_LHH(3) = pkm;

        /* Load SASN into CR3 bits 16-31 (48-63) */
        regs->CR_LHL(3) = sasn;

        /* Load EAX into CR8 bits 0-15 (32-47) */
        regs->CR_LHH(8) = eax;

        /* Load PASN into CR4 bits 16-31 (48-63) */
        regs->CR_LHL(4) = pasn;

    } /* end if(LSED_UET_PC) */

    /* Update virtual and absolute addresses to point to byte 136 */
    lsea += 8;
    LSEA_WRAP(lsea);
    abs += 8;

    /* Recalculate absolute address if page boundary crossed */
    if ((lsea & PAGEFRAME_BYTEMASK) == 0x000)
        abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);

    /* Save the PER mode bit from the current PSW */
    permode = (regs->psw.sysmask & PSW_PERMODE) ? 1 : 0;

    DBGMSG("  stack: PSW=%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
            "loaded from V:" F_VADR " A:" F_RADR "\n",
            regs->mainstor[abs], regs->mainstor[abs+1],
            regs->mainstor[abs+2], regs->mainstor[abs+3],
            regs->mainstor[abs+4], regs->mainstor[abs+5],
            regs->mainstor[abs+6], regs->mainstor[abs+7],
            lsea, abs);

    /* Copy PSW bits 0-63 from bytes 136-143 of the stack entry */
    memcpy (newpsw, regs->mainstor + abs, 8);

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    /* For ESAME, advance to byte 168 of the stack entry */
    lsea += 32;
    LSEA_WRAP(lsea);
    abs += 32;

    /* Recalculate absolute address if page boundary crossed */
    if ((lsea & PAGEFRAME_BYTEMASK) < 32)
        abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);

    /* Copy ESAME PSW bits 64-127 from bytes 168-175 */
    memcpy (newpsw + 8, regs->mainstor + abs, 8);

    /* Update virtual and absolute addresses to point to byte 176 */
    lsea += 8;
    LSEA_WRAP(lsea);
    abs += 8;

    /* Recalculate absolute address if page boundary crossed */
    if ((lsea & PAGEFRAME_BYTEMASK) == 0x000)
        abs = ARCH_DEP(abs_stack_addr) (lsea, regs, ACCTYPE_READ);

    /* For a call state entry only, if ASN-and-LX-reuse is installed and
       active, load the SASTEIN (high word of CR3) from bytes 176-179,
       and load the PASTEIN (high word of CR4) from bytes 180-183 */
    if ((lsed.uet & LSED_UET_ET) == LSED_UET_PC
        && ASN_AND_LX_REUSE_ENABLED(regs))
    {
        FETCH_FW(regs->CR_H(3), regs->mainstor + abs);
        FETCH_FW(regs->CR_H(4), regs->mainstor + abs + 4);

        DBGMSG("  stack: SASTEIN=%2.2X%2.2X%2.2X%2.2X "
                "PASTEIN=%2.2X%2.2X%2.2X%2.2X \n"
                "loaded from V:" F_VADR " A:" F_RADR "\n",
                regs->mainstor[abs], regs->mainstor[abs+1],
                regs->mainstor[abs+2], regs->mainstor[abs+3],
                regs->mainstor[abs+4], regs->mainstor[abs+5],
                regs->mainstor[abs+6], regs->mainstor[abs+7],
                lsea, abs);

    } /* end if(LSED_UET_PC && ASN_AND_LX_REUSE_ENABLED) */

#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

    /* [5.12.4.4] Pass back the absolute address of the entry
       descriptor of the preceding linkage stack entry.  The
       next entry size field of this entry will be cleared on
       successful completion of the PR instruction */
    *lsedap = ARCH_DEP(abs_stack_addr) (lsep, regs, ACCTYPE_WRITE);

    /* [5.12.4.5] Update CR15 to point to the previous entry */
    regs->CR(15) = lsep & CR15_LSEA;

    /* Load new PSW using the bytes extracted from the stack entry */
    /* The rc will be checked by calling routine for PIC 06        */
    *rc = ARCH_DEP(load_psw) (regs, newpsw);

    /* Restore the PER mode bit from the current PSW */
    if (permode)
        regs->psw.sysmask |= PSW_PERMODE;
    else
        regs->psw.sysmask &= ~PSW_PERMODE;

    /* restore PER masks which could have been wiped out by load_psw */
    SET_IC_MASK(regs);

    DBGMSG( "stack: CR15=" F_CREG "\n", regs->CR( 15 ));

    /* Return the entry type of the unstacked state entry */
    return (lsed.uet & LSED_UET_ET);

} /* end function ARCH_DEP(program_return_unstack) */


#endif /*defined(FEATURE_LINKAGE_STACK)*/


#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "stack.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "stack.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
