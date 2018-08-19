/* CPU.C        (C) Copyright Roger Bowler, 1994-2012                */
/*              (C) Copyright Jan Jaeger, 1999-2012                  */
/*              ESA/390 CPU Emulator                                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements the CPU instruction execution function of  */
/* the S/370 and ESA/390 architectures, as described in the manuals  */
/* GA22-7000-03 System/370 Principles of Operation                   */
/* SA22-7201-06 ESA/390 Principles of Operation                      */
/* SA22-7832-00 z/Architecture Principles of Operation               */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      Nullification corrections by Jan Jaeger                      */
/*      Set priority by Reed H. Petty from an idea by Steve Gay      */
/*      Corrections to program check by Jan Jaeger                   */
/*      Light optimization on critical path by Valery Pogonchenko    */
/*      OSTAILOR parameter by Jay Maynard                            */
/*      CPU timer and clock comparator interrupt improvements by     */
/*          Jan Jaeger, after a suggestion by Willem Konynenberg     */
/*      Instruction decode rework - Jan Jaeger                       */
/*      Modifications for Interpretive Execution (SIE) by Jan Jaeger */
/*      Basic FP extensions support - Peter Kuschnerus           v209*/
/*      ASN-and-LX-reuse facility - Roger Bowler, June 2004      @ALR*/
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

// #define SIE_DEBUG

#define _CPU_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

/*-------------------------------------------------------------------*/
/* Put a CPU in check-stop state                                     */
/* Must hold the system intlock                                      */
/*-------------------------------------------------------------------*/
void ARCH_DEP(checkstop_cpu)(REGS *regs)
{
    regs->cpustate=CPUSTATE_STOPPING;
    regs->checkstop=1;
    ON_IC_INTERRUPT(regs);
}

/*-------------------------------------------------------------------*/
/* Put all the CPUs in the configuration in check-stop state         */
/*-------------------------------------------------------------------*/
void ARCH_DEP(checkstop_config)(void)
{
    int i;
    for(i=0;i<sysblk.maxcpu;i++)
    {
        if(IS_CPU_ONLINE(i))
        {
            ARCH_DEP(checkstop_cpu)(sysblk.regs[i]);
        }
    }
    WAKEUP_CPUS_MASK(sysblk.waiting_mask);
}

/*-------------------------------------------------------------------*/
/* Store current PSW at a specified address in main storage          */
/*-------------------------------------------------------------------*/
void ARCH_DEP(store_psw) (REGS *regs, BYTE *addr)
{

    /* Ensure psw.IA is set */
    if (!regs->psw.zeroilc)
        SET_PSW_IA(regs);

#if defined( FEATURE_BCMODE )
    if ( ECMODE(&regs->psw) ) {
#endif
#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        STORE_FW ( addr,
                   ( (regs->psw.sysmask << 24)
                   | ((regs->psw.pkey | regs->psw.states) << 16)
                   | ( ( (regs->psw.asc)
                       | (regs->psw.cc << 4)
                       | (regs->psw.progmask)
                       ) << 8
                     )
                   | regs->psw.zerobyte
                   )
                 );
        if(unlikely(regs->psw.zeroilc))
            STORE_FW ( addr + 4, regs->psw.IA | (regs->psw.amode ? 0x80000000 : 0) );
        else
            STORE_FW ( addr + 4,
                   ( (regs->psw.IA & ADDRESS_MAXWRAP(regs)) | (regs->psw.amode ? 0x80000000 : 0) )
                 );
#endif /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
#if defined( FEATURE_BCMODE )
    } else {
        STORE_FW ( addr,
                   ( (regs->psw.sysmask << 24)
                   | ((regs->psw.pkey | regs->psw.states) << 16)
                   | (regs->psw.intcode)
                   )
                 );
        if(unlikely(regs->psw.zeroilc))
            STORE_FW ( addr + 4,
                   ( ( (REAL_ILC(regs) << 5)
                     | (regs->psw.cc << 4)
                     | regs->psw.progmask
                     ) << 24
                   ) | regs->psw.IA
                 );
        else
            STORE_FW ( addr + 4,
                   ( ( (REAL_ILC(regs) << 5)
                     | (regs->psw.cc << 4)
                     | regs->psw.progmask
                     ) << 24
                   ) | (regs->psw.IA & ADDRESS_MAXWRAP(regs))
                 );
    }
#elif defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        STORE_FW ( addr,
                   ( (regs->psw.sysmask << 24)
                   | ((regs->psw.pkey | regs->psw.states) << 16)
                   | ( ( (regs->psw.asc)
                       | (regs->psw.cc << 4)
                       | (regs->psw.progmask)
                       ) << 8
                     )
                   | (regs->psw.amode64 ? 0x01 : 0)
                   | regs->psw.zerobyte
                   )
                 );
        STORE_FW ( addr + 4,
                   ( (regs->psw.amode ? 0x80000000 : 0 )
                   | regs->psw.zeroword
                   )
                 );
        STORE_DW ( addr + 8, regs->psw.IA_G );
#endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
} /* end function ARCH_DEP(store_psw) */

/*-------------------------------------------------------------------*/
/* Load current PSW from a specified address in main storage         */
/* Returns 0 if valid, 0x0006 if specification exception             */
/*-------------------------------------------------------------------*/
int ARCH_DEP(load_psw) (REGS *regs, BYTE *addr)
{
    INVALIDATE_AIA(regs);

    regs->psw.zeroilc = 1;

    regs->psw.sysmask = addr[0];
    regs->psw.pkey    = (addr[1] & 0xF0);
    regs->psw.states  = (addr[1] & 0x0F);

#if defined(FEATURE_BCMODE)
    if ( ECMODE(&regs->psw) ) {
#endif /*defined(FEATURE_BCMODE)*/

        SET_IC_ECMODE_MASK(regs);

        /* Processing for EC mode PSW */
        regs->psw.intcode  = 0;
        regs->psw.asc      = (addr[2] & 0xC0);
        regs->psw.cc       = (addr[2] & 0x30) >> 4;
        regs->psw.progmask = (addr[2] & 0x0F);
        regs->psw.amode    = (addr[4] & 0x80) ? 1 : 0;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        regs->psw.zerobyte = addr[3] & 0xFE;
        regs->psw.amode64  = addr[3] & 0x01;
        regs->psw.zeroword = fetch_fw(addr+4) & 0x7FFFFFFF;
        regs->psw.IA       = fetch_dw (addr + 8);
        regs->psw.AMASK    = regs->psw.amode64 ? AMASK64
                           : regs->psw.amode   ? AMASK31 : AMASK24;
#else
        regs->psw.zerobyte = addr[3];
        regs->psw.amode64  = 0;
        regs->psw.IA       = fetch_fw(addr + 4) & 0x7FFFFFFF;
        regs->psw.AMASK    = regs->psw.amode ? AMASK31 : AMASK24;
#endif

        /* Bits 0 and 2-4 of system mask must be zero */
        if ((addr[0] & 0xB8) != 0)
            return PGM_SPECIFICATION_EXCEPTION;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /* For ESAME, bit 12 must be zero */
        if (NOTESAME(&regs->psw))
            return PGM_SPECIFICATION_EXCEPTION;

        /* Bits 24-30 must be zero */
        if (regs->psw.zerobyte)
            return PGM_SPECIFICATION_EXCEPTION;

        /* Bits 33-63 must be zero */
        if ( regs->psw.zeroword )
            return PGM_SPECIFICATION_EXCEPTION;
#else /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
        /* Bits 24-31 must be zero */
        if ( regs->psw.zerobyte )
            return PGM_SPECIFICATION_EXCEPTION;

        /* For ESA/390, bit 12 must be one */
        if (!ECMODE(&regs->psw))
            return PGM_SPECIFICATION_EXCEPTION;
#endif /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

#ifndef FEATURE_DUAL_ADDRESS_SPACE
        /* If DAS feature not installed then bit 16 must be zero */
        if (SPACE_BIT(&regs->psw))
            return PGM_SPECIFICATION_EXCEPTION;
#endif

#ifndef FEATURE_ACCESS_REGISTERS
        /* If not ESA/370 or ESA/390 then bit 17 must be zero */
        if (AR_BIT(&regs->psw))
            return PGM_SPECIFICATION_EXCEPTION;
#endif

        /* Check validity of amode and instruction address */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /* For ESAME, bit 32 cannot be zero if bit 31 is one */
        if (regs->psw.amode64 && !regs->psw.amode)
            return PGM_SPECIFICATION_EXCEPTION;

        /* If bit 32 is zero then IA cannot exceed 24 bits */
        if (!regs->psw.amode && regs->psw.IA > 0x00FFFFFF)
            return PGM_SPECIFICATION_EXCEPTION;

        /* If bit 31 is zero then IA cannot exceed 31 bits */
        if (!regs->psw.amode64 && regs->psw.IA > 0x7FFFFFFF)
            return PGM_SPECIFICATION_EXCEPTION;
#else /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
  #ifdef FEATURE_BIMODAL_ADDRESSING
        /* For 370-XA, ESA/370, and ESA/390,
           if amode=24, bits 33-39 must be zero */
        if (!regs->psw.amode && regs->psw.IA > 0x00FFFFFF)
            return PGM_SPECIFICATION_EXCEPTION;
  #else
        /* For S/370, bits 32-39 must be zero */
        if (addr[4] != 0x00)
            return PGM_SPECIFICATION_EXCEPTION;
  #endif
#endif /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

#if defined( FEATURE_BCMODE )
    } else {

        SET_IC_BCMODE_MASK(regs);

        /* Processing for S/370 BC mode PSW */
        regs->psw.intcode = fetch_hw (addr + 2);
        regs->psw.cc = (addr[4] & 0x30) >> 4;
        regs->psw.progmask = (addr[4] & 0x0F);

        FETCH_FW(regs->psw.IA, addr + 4);
        regs->psw.IA &= 0x00FFFFFF;
        regs->psw.AMASK = AMASK24;

        regs->psw.zerobyte = 0;
        regs->psw.asc = 0;
        regs->psw.amode64 = regs->psw.amode = 0;
    }
#endif /* defined( FEATURE_BCMODE ) */

#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
    /* Bits 5 and 16 must be zero in XC mode */
    if( SIE_STATB(regs, MX, XC)
      && ( (regs->psw.sysmask & PSW_DATMODE) || SPACE_BIT(&regs->psw)) )
        return PGM_SPECIFICATION_EXCEPTION;
#endif

    regs->psw.zeroilc = 0;

    /* Check for wait state PSW */
    if (WAITSTATE(&regs->psw) && CPU_STEPPING_OR_TRACING_ALL)
    {
        char buf[40];
        WRMSG(HHC00800, "I", PTYPSTR(regs->cpuad), regs->cpuad, str_psw(regs, buf));
    }

    TEST_SET_AEA_MODE(regs);

    return 0;
} /* end function ARCH_DEP(load_psw) */

/*-------------------------------------------------------------------*/
/* Load program interrupt new PSW                                    */
/*-------------------------------------------------------------------*/
void (ATTR_REGPARM(2) ARCH_DEP(program_interrupt)) (REGS *regs, int pcode)
{
PSA    *psa;                            /* -> Prefixed storage area  */
REGS   *realregs;                       /* True regs structure       */
RADR    px;                             /* host real address of pfx  */
int     code;                           /* pcode without PER ind.    */
int     ilc;                            /* instruction length        */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
/** FIXME : SEE ISW20090110-1 */
void   *zmoncode=NULL;                  /* special reloc for z/Arch  */
                 /* FIXME : zmoncode not being initialized here raises
                    a potentially non-initialized warning in GCC..
                    can't find why. ISW 2009/02/04 */
                                        /* mon call SIE intercept    */
#endif
#if defined(FEATURE_INTERPRETIVE_EXECUTION)
int     sie_ilc=0;                      /* SIE instruction length    */
#endif
#if defined(_FEATURE_SIE)
int     nointercept;                    /* True for virtual pgmint   */
#endif /*defined(_FEATURE_SIE)*/
#if defined(OPTION_FOOTPRINT_BUFFER)
U32     n;
#endif /*defined(OPTION_FOOTPRINT_BUFFER)*/
char    dxcstr[8]={0};                  /* " DXC=xx" if data excptn  */

static char *pgmintname[] = {
        /* 01 */        "Operation exception",
        /* 02 */        "Privileged-operation exception",
        /* 03 */        "Execute exception",
        /* 04 */        "Protection exception",
        /* 05 */        "Addressing exception",
        /* 06 */        "Specification exception",
        /* 07 */        "Data exception",
        /* 08 */        "Fixed-point-overflow exception",
        /* 09 */        "Fixed-point-divide exception",
        /* 0A */        "Decimal-overflow exception",
        /* 0B */        "Decimal-divide exception",
        /* 0C */        "HFP-exponent-overflow exception",
        /* 0D */        "HFP-exponent-underflow exception",
        /* 0E */        "HFP-significance exception",
        /* 0F */        "HFP-floating-point-divide exception",
        /* 10 */        "Segment-translation exception",
        /* 11 */        "Page-translation exception",
        /* 12 */        "Translation-specification exception",
        /* 13 */        "Special-operation exception",
        /* 14 */        "Pseudo-page-fault exception",
        /* 15 */        "Operand exception",
        /* 16 */        "Trace-table exception",
        /* 17 */        "ASN-translation exception",
        /* 18 */        "Page access exception",
        /* 19 */        "Vector/Crypto operation exception",
        /* 1A */        "Page state exception",
        /* 1B */        "Page transition exception",
        /* 1C */        "Space-switch event",
        /* 1D */        "Square-root exception",
        /* 1E */        "Unnormalized-operand exception",
        /* 1F */        "PC-translation specification exception",
        /* 20 */        "AFX-translation exception",
        /* 21 */        "ASX-translation exception",
        /* 22 */        "LX-translation exception",
        /* 23 */        "EX-translation exception",
        /* 24 */        "Primary-authority exception",
        /* 25 */        "Secondary-authority exception",
        /* 26 */        "LFX-translation exception",            /*@ALR*/
        /* 27 */        "LSX-translation exception",            /*@ALR*/
        /* 28 */        "ALET-specification exception",
        /* 29 */        "ALEN-translation exception",
        /* 2A */        "ALE-sequence exception",
        /* 2B */        "ASTE-validity exception",
        /* 2C */        "ASTE-sequence exception",
        /* 2D */        "Extended-authority exception",
        /* 2E */        "LSTE-sequence exception",              /*@ALR*/
        /* 2F */        "ASTE-instance exception",              /*@ALR*/
        /* 30 */        "Stack-full exception",
        /* 31 */        "Stack-empty exception",
        /* 32 */        "Stack-specification exception",
        /* 33 */        "Stack-type exception",
        /* 34 */        "Stack-operation exception",
        /* 35 */        "Unassigned exception",
        /* 36 */        "Unassigned exception",
        /* 37 */        "Unassigned exception",
        /* 38 */        "ASCE-type exception",
        /* 39 */        "Region-first-translation exception",
        /* 3A */        "Region-second-translation exception",
        /* 3B */        "Region-third-translation exception",
        /* 3C */        "Unassigned exception",
        /* 3D */        "Unassigned exception",
        /* 3E */        "Unassigned exception",
        /* 3F */        "Unassigned exception",
        /* 40 */        "Monitor event" };

        /* 26 */ /* was "Page-fault-assist exception", */
        /* 27 */ /* was "Control-switch exception", */

    /* If called with ghost registers (ie from hercules command
       then ignore all interrupt handling and report the error
       to the caller */
    if(regs->ghostregs)
        longjmp(regs->progjmp, pcode);

    PTT_PGM("*PROG",pcode,(U32)(regs->TEA & 0xffffffff),regs->psw.IA_L);

    /* program_interrupt() may be called with a shadow copy of the
       regs structure, realregs is the pointer to the real structure
       which must be used when loading/storing the psw, or backing up
       the instruction address in case of nullification */
#if defined(_FEATURE_SIE)
        realregs = SIE_MODE(regs)
                 ? sysblk.regs[regs->cpuad]->guestregs
                 : sysblk.regs[regs->cpuad];
#else /*!defined(_FEATURE_SIE)*/
    realregs = sysblk.regs[regs->cpuad];
#endif /*!defined(_FEATURE_SIE)*/

    /* Prevent machine check when in (almost) interrupt loop */
    realregs->instcount++;
    UPDATE_SYSBLK_INSTCOUNT( 1 );

    /* Release any locks */
    if (sysblk.intowner == realregs->cpuad)
        RELEASE_INTLOCK(realregs);
    if (sysblk.mainowner == realregs->cpuad)
        RELEASE_MAINLOCK(realregs);

    /* Ensure psw.IA is set and aia invalidated */
    INVALIDATE_AIA(realregs);
#if defined(FEATURE_INTERPRETIVE_EXECUTION)
    if(realregs->sie_active)
        INVALIDATE_AIA(realregs->guestregs);
#endif /*defined(FEATURE_INTERPRETIVE_EXECUTION)*/

    /* Set instruction length (ilc) */
    ilc = realregs->psw.zeroilc ? 0 : REAL_ILC(realregs);
    if (realregs->psw.ilc == 0 && !realregs->psw.zeroilc)
    {
        /* This can happen if BALR, BASR, BASSM or BSM
           program checks during trace */
        ilc = likely(!realregs->execflag) ? 2 : realregs->exrl ? 6 : 4;
        realregs->ip += ilc;
        realregs->psw.IA += ilc;
        realregs->psw.ilc = ilc;
    }
#if defined(FEATURE_INTERPRETIVE_EXECUTION)
    if(realregs->sie_active)
    {
        sie_ilc = realregs->guestregs->psw.zeroilc ? 0 :
                REAL_ILC(realregs->guestregs);
        if (realregs->guestregs->psw.ilc == 0
         && !realregs->guestregs->psw.zeroilc)
        {
            sie_ilc = likely(!realregs->guestregs->execflag) ? 2 :
                    realregs->guestregs->exrl ? 6 : 4;
            realregs->guestregs->psw.IA += sie_ilc; /* IanWorthington regression restored from 20081205 */
            realregs->guestregs->psw.ilc = sie_ilc;
        }
    }
#endif /*defined(FEATURE_INTERPRETIVE_EXECUTION)*/

    /* Set `execflag' to 0 in case EXecuted instruction program-checked */
    realregs->execflag = 0;
#if defined(FEATURE_INTERPRETIVE_EXECUTION)
    if(realregs->sie_active)
        realregs->guestregs->execflag = 0;
#endif /*defined(FEATURE_INTERPRETIVE_EXECUTION)*/

    /* Unlock the main storage lock if held */
    if (realregs->cpuad == sysblk.mainowner)
        RELEASE_MAINLOCK(realregs);

    /* Remove PER indication from program interrupt code
       such that interrupt code specific tests may be done.
       The PER indication will be stored in the PER handling
       code */
    code = pcode & ~PGM_PER_EVENT;

    /* If this is a concurrent PER event then we must add the PER
       bit to the interrupts code */
    if( OPEN_IC_PER(realregs) )
        pcode |= PGM_PER_EVENT;

    /* Perform serialization and checkpoint synchronization */
    PERFORM_SERIALIZATION (realregs);
    PERFORM_CHKPT_SYNC (realregs);

#if defined(FEATURE_INTERPRETIVE_EXECUTION)
    /* Host protection and addressing exceptions must be
       reflected to the guest */
    if(realregs->sie_active &&
        (code == PGM_PROTECTION_EXCEPTION
      || code == PGM_ADDRESSING_EXCEPTION
#if defined(_FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)
      || code == PGM_ALET_SPECIFICATION_EXCEPTION
      || code == PGM_ALEN_TRANSLATION_EXCEPTION
      || code == PGM_ALE_SEQUENCE_EXCEPTION
      || code == PGM_EXTENDED_AUTHORITY_EXCEPTION
#endif /*defined(_FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)*/
        ) )
    {
#if defined( SIE_DEBUG )
        LOGMSG( "program_int() passing to guest code=%4.4X\n", pcode );
#endif
        realregs->guestregs->TEA = realregs->TEA;
        realregs->guestregs->excarid = realregs->excarid;
        realregs->guestregs->opndrid = realregs->opndrid;
#if defined(_FEATURE_PROTECTION_INTERCEPTION_CONTROL)
        realregs->guestregs->hostint = 1;
#endif /*defined(_FEATURE_PROTECTION_INTERCEPTION_CONTROL)*/
        (realregs->guestregs->program_interrupt) (realregs->guestregs, pcode);
    }
#endif /*defined(FEATURE_INTERPRETIVE_EXECUTION)*/

    /* Back up the PSW for exceptions which cause nullification,
       unless the exception occurred during instruction fetch */
    if ((code == PGM_PAGE_TRANSLATION_EXCEPTION
      || code == PGM_SEGMENT_TRANSLATION_EXCEPTION
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
      || code == PGM_ASCE_TYPE_EXCEPTION
      || code == PGM_REGION_FIRST_TRANSLATION_EXCEPTION
      || code == PGM_REGION_SECOND_TRANSLATION_EXCEPTION
      || code == PGM_REGION_THIRD_TRANSLATION_EXCEPTION
#endif
      || code == PGM_TRACE_TABLE_EXCEPTION
      || code == PGM_AFX_TRANSLATION_EXCEPTION
      || code == PGM_ASX_TRANSLATION_EXCEPTION
      || code == PGM_LX_TRANSLATION_EXCEPTION
      || code == PGM_LFX_TRANSLATION_EXCEPTION                 /*@ALR*/
      || code == PGM_LSX_TRANSLATION_EXCEPTION                 /*@ALR*/
      || code == PGM_LSTE_SEQUENCE_EXCEPTION                   /*@ALR*/
      || code == PGM_EX_TRANSLATION_EXCEPTION
      || code == PGM_PRIMARY_AUTHORITY_EXCEPTION
      || code == PGM_SECONDARY_AUTHORITY_EXCEPTION
      || code == PGM_ALEN_TRANSLATION_EXCEPTION
      || code == PGM_ALE_SEQUENCE_EXCEPTION
      || code == PGM_ASTE_VALIDITY_EXCEPTION
      || code == PGM_ASTE_SEQUENCE_EXCEPTION
      || code == PGM_ASTE_INSTANCE_EXCEPTION                   /*@ALR*/
      || code == PGM_EXTENDED_AUTHORITY_EXCEPTION
      || code == PGM_STACK_FULL_EXCEPTION
      || code == PGM_STACK_EMPTY_EXCEPTION
      || code == PGM_STACK_SPECIFICATION_EXCEPTION
      || code == PGM_STACK_TYPE_EXCEPTION
      || code == PGM_STACK_OPERATION_EXCEPTION
      || code == PGM_VECTOR_OPERATION_EXCEPTION)
      && !realregs->instinvalid)
    {
        realregs->psw.IA -= ilc;
        realregs->psw.IA &= ADDRESS_MAXWRAP(realregs);
#if defined(FEATURE_INTERPRETIVE_EXECUTION)
        /* When in SIE mode the guest instruction causing this
           host exception must also be nullified */
        if(realregs->sie_active && !realregs->guestregs->instinvalid)
        {
            realregs->guestregs->psw.IA -= sie_ilc;
            realregs->guestregs->psw.IA &= ADDRESS_MAXWRAP(realregs->guestregs);
        }
#endif /*defined(FEATURE_INTERPRETIVE_EXECUTION)*/
    }

    /* The OLD PSW must be incremented on the following
       exceptions during instfetch */
    if(realregs->instinvalid &&
      (  code == PGM_PROTECTION_EXCEPTION
      || code == PGM_ADDRESSING_EXCEPTION
      || code == PGM_SPECIFICATION_EXCEPTION
      || code == PGM_TRANSLATION_SPECIFICATION_EXCEPTION ))
    {
        realregs->psw.IA += ilc;
        realregs->psw.IA &= ADDRESS_MAXWRAP(realregs);
    }

    /* Store the interrupt code in the PSW */
    realregs->psw.intcode = pcode;

    /* Call debugger if active */
    HDC2(debug_program_interrupt, regs, pcode);

    /* Trace program checks other then PER event */
    if(code && (CPU_STEPPING_OR_TRACING(realregs, ilc)
        || sysblk.pgminttr & ((U64)1 << ((code - 1) & 0x3F))))
    {
    BYTE *ip;
    char buf1[10] = "";
    char buf2[32] = "";
#if defined(OPTION_FOOTPRINT_BUFFER)
        if(!(sysblk.insttrace || sysblk.inststep))
            for(n = sysblk.footprptr[realregs->cpuad] + 1 ;
                n != sysblk.footprptr[realregs->cpuad];
                n++, n &= OPTION_FOOTPRINT_BUFFER - 1)
                ARCH_DEP(display_inst)
                        (&sysblk.footprregs[realregs->cpuad][n],
                        sysblk.footprregs[realregs->cpuad][n].inst);
#endif /*defined(OPTION_FOOTPRINT_BUFFER)*/

#if defined(_FEATURE_SIE)
        if (SIE_MODE(realregs))
          STRLCPY( buf1, "SIE: " );
#endif /*defined(_FEATURE_SIE)*/

#if defined( SIE_DEBUG )
        STRLCPY( buf2, QSTR( _GEN_ARCH ));
        STRLCAT( buf2, " " );
#endif
        if (code == PGM_DATA_EXCEPTION)
           snprintf(dxcstr, sizeof(dxcstr), " DXC=%2.2X", regs->dxc);
        dxcstr[sizeof(dxcstr)-1] = '\0';

        /* Calculate instruction pointer */
        ip = realregs->instinvalid ? NULL
           : (realregs->ip - ilc < realregs->aip)
             ? realregs->inst : realregs->ip - ilc;

        /* Trace pgm interrupt if not being specially suppressed */
        if (0
            || !ip
            || ip[0] != 0xB1 /* LRA */
            || code != PGM_SPECIAL_OPERATION_EXCEPTION
            || !sysblk.nolrasoe /* suppression not requested */
        )
        {
            // "Processor %s%02X: %s%s %s code %4.4X  ilc %d%s"
            WRMSG(HHC00801, "I",
            PTYPSTR(realregs->cpuad), realregs->cpuad, buf1, buf2,
                    pgmintname[ (code - 1) & 0x3F], pcode, ilc, dxcstr);

            ARCH_DEP(display_inst) (realregs, ip);
        }
    }

    realregs->instinvalid = 0;

#if defined(FEATURE_INTERPRETIVE_EXECUTION)
    /* If this is a host exception in SIE state then leave SIE */
    if(realregs->sie_active)
        ARCH_DEP(sie_exit) (realregs, SIE_HOST_PGMINT);
#endif /*defined(FEATURE_INTERPRETIVE_EXECUTION)*/

    /* Absolute address of prefix page */
    px = realregs->PX;

    /* If under SIE use translated to host absolute prefix */
#if defined(_FEATURE_SIE)
    if(SIE_MODE(regs))
        px = regs->sie_px;
#endif

#if defined(_FEATURE_SIE)
    if(!SIE_MODE(regs) ||
      /* Interception is mandatory for the following exceptions */
      (
#if defined(_FEATURE_PROTECTION_INTERCEPTION_CONTROL)
         !(code == PGM_PROTECTION_EXCEPTION
           && (!SIE_FEATB(regs, EC2, PROTEX)
             || realregs->hostint))
#else /*!defined(_FEATURE_PROTECTION_INTERCEPTION_CONTROL)*/
         code != PGM_PROTECTION_EXCEPTION
#endif /*!defined(_FEATURE_PROTECTION_INTERCEPTION_CONTROL)*/
#if defined (_FEATURE_PER2)
      && !((pcode & PGM_PER_EVENT) && SIE_FEATB(regs, M, GPE))
#endif /* defined (_FEATURE_PER2) */
      && code != PGM_ADDRESSING_EXCEPTION
      && code != PGM_SPECIFICATION_EXCEPTION
      && code != PGM_SPECIAL_OPERATION_EXCEPTION
#ifdef FEATURE_S370_S390_VECTOR_FACILITY
      && code != PGM_VECTOR_OPERATION_EXCEPTION
#endif
#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)
      && !(code == PGM_ALEN_TRANSLATION_EXCEPTION
        && SIE_FEATB(regs, MX, XC))
      && !(code == PGM_ALE_SEQUENCE_EXCEPTION
        && SIE_FEATB(regs, MX, XC))
      && !(code == PGM_EXTENDED_AUTHORITY_EXCEPTION
        && SIE_FEATB(regs, MX, XC))
#endif /*defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)*/
      /* And conditional for the following exceptions */
      && !(code == PGM_OPERATION_EXCEPTION
        && SIE_FEATB(regs, IC0, OPEREX))
      && !(code == PGM_PRIVILEGED_OPERATION_EXCEPTION
        && SIE_FEATB(regs, IC0, PRIVOP))
#ifdef FEATURE_BASIC_FP_EXTENSIONS
      && !(code == PGM_DATA_EXCEPTION
        && (regs->dxc == 1 || regs->dxc == 2)
        && (regs->CR(0) & CR0_AFP)
        && !(regs->hostregs->CR(0) & CR0_AFP))
#endif /*FEATURE_BASIC_FP_EXTENSIONS*/
      /* Or all exceptions if requested as such */
      && !SIE_FEATB(regs, IC0, PGMALL) )
    )
    {
#endif /*defined(_FEATURE_SIE)*/
        /* Set the main storage reference and change bits */
        STORAGE_KEY(px, regs) |= (STORKEY_REF | STORKEY_CHANGE);

        /* Point to PSA in main storage */
        psa = (void*)(regs->mainstor + px);
#if defined( _FEATURE_SIE )
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
/** FIXME : SEE ISW20090110-1 */
        if(code == PGM_MONITOR_EVENT)
        {
            zmoncode=psa->moncode;
        }
#endif
        nointercept = 1;
    }
    else
    {
        /* This is a guest interruption interception so point to
           the interruption parm area in the state descriptor
           rather then the PSA, except for the operation exception */
        if(code != PGM_OPERATION_EXCEPTION)
        {
            psa = (void*)(regs->hostregs->mainstor + SIE_STATE(regs) + SIE_IP_PSA_OFFSET);
            /* Set the main storage reference and change bits */
            STORAGE_KEY(SIE_STATE(regs), regs->hostregs) |= (STORKEY_REF | STORKEY_CHANGE);
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
/** FIXME : SEE ISW20090110-1 */
            if(code == PGM_MONITOR_EVENT)
            {
                PSA *_psa;
                _psa=(void *)(regs->hostregs->mainstor + SIE_STATE(regs) + SIE_II_PSA_OFFSET);
                zmoncode=_psa->ioid;
            }
#endif
        }
        else
        {
            /* Point to PSA in main storage */
            psa = (void*)(regs->mainstor + px);

            /* Set the main storage reference and change bits */
            STORAGE_KEY(px, regs) |= (STORKEY_REF | STORKEY_CHANGE);
        }

        nointercept = 0;
    }
#endif /*defined(_FEATURE_SIE)*/

#if defined( _FEATURE_PER )
    /* Handle PER or concurrent PER event */

    /* Throw out Stor Alter PER if merged with nullified/suppressed rupt */
    if ( IS_IC_PER_SA(realregs) && !IS_IC_PER_STURA(realregs) &&
                                   (realregs->ip[0] != 0x0E) &&
         !(code == 0x00 || code == 0x06 || code == 0x08 || code == 0x0A ||
           code == 0x0C || code == 0x0D || code == 0x0E || code == 0x1C ||
           code == 0x40) )
              OFF_IC_PER_SA(realregs);

    if( OPEN_IC_PER(realregs) )
    {
        if( CPU_STEPPING_OR_TRACING(realregs, ilc) )
            WRMSG(HHC00802, "I", PTYPSTR(regs->cpuad), regs->cpuad,
              pcode, IS_IC_PER(realregs) >> 16,
              (realregs->psw.IA - ilc) & ADDRESS_MAXWRAP(realregs) );

        realregs->perc |= OPEN_IC_PER(realregs) >> ((32 - IC_CR9_SHIFT) - 16);

        /* Positions 14 and 15 contain zeros if a storage alteration
           event was not indicated */
//FIXME: is this right??
        if( !(OPEN_IC_PER_SA(realregs))
          || (OPEN_IC_PER_STURA(realregs)) )
            realregs->perc &= 0xFFFC;

        STORE_HW(psa->perint, realregs->perc);

        STORE_W(psa->peradr, realregs->peradr);

        if( IS_IC_PER_SA(realregs) && ACCESS_REGISTER_MODE(&realregs->psw) )
            psa->perarid = realregs->peraid;

#if defined( _FEATURE_SIE )
        /* Reset PER pending indication */
        if(nointercept)
            OFF_IC_PER(realregs);
#endif
    }
    else
    {
        pcode &= 0xFF7F;
    }
#endif /* defined( _FEATURE_PER ) */


#if defined(FEATURE_BCMODE)
    /* For ECMODE, store extended interrupt information in PSA */
    if ( ECMODE(&realregs->psw) )
#endif /*defined(FEATURE_BCMODE)*/
    {
        /* Store the program interrupt code at PSA+X'8C' */
        psa->pgmint[0] = 0;
        psa->pgmint[1] = ilc;
        STORE_HW(psa->pgmint + 2, pcode);

        /* Store the exception access identification at PSA+160 */
        if ( code == PGM_PAGE_TRANSLATION_EXCEPTION
          || code == PGM_SEGMENT_TRANSLATION_EXCEPTION
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
          || code == PGM_ASCE_TYPE_EXCEPTION
          || code == PGM_REGION_FIRST_TRANSLATION_EXCEPTION
          || code == PGM_REGION_SECOND_TRANSLATION_EXCEPTION
          || code == PGM_REGION_THIRD_TRANSLATION_EXCEPTION
#endif
          || code == PGM_ALEN_TRANSLATION_EXCEPTION
          || code == PGM_ALE_SEQUENCE_EXCEPTION
          || code == PGM_ASTE_VALIDITY_EXCEPTION
          || code == PGM_ASTE_SEQUENCE_EXCEPTION
          || code == PGM_ASTE_INSTANCE_EXCEPTION               /*@ALR*/
          || code == PGM_EXTENDED_AUTHORITY_EXCEPTION
#ifdef FEATURE_SUPPRESSION_ON_PROTECTION
          || code == PGM_PROTECTION_EXCEPTION
#endif
           )
        {
            psa->excarid = regs->excarid;
            if(regs->TEA | TEA_MVPG)
                psa->opndrid = regs->opndrid;
            realregs->opndrid = 0;
        }

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /* Store the translation exception address at PSA+168 */
        if ( code == PGM_PAGE_TRANSLATION_EXCEPTION
          || code == PGM_SEGMENT_TRANSLATION_EXCEPTION
          || code == PGM_ASCE_TYPE_EXCEPTION
          || code == PGM_REGION_FIRST_TRANSLATION_EXCEPTION
          || code == PGM_REGION_SECOND_TRANSLATION_EXCEPTION
          || code == PGM_REGION_THIRD_TRANSLATION_EXCEPTION
#ifdef FEATURE_SUPPRESSION_ON_PROTECTION
          || code == PGM_PROTECTION_EXCEPTION
#endif
           )
        {
            STORE_DW(psa->TEA_G, regs->TEA);
        }

        /* Store the translation exception address at PSA+172 */
        if ( code == PGM_AFX_TRANSLATION_EXCEPTION
          || code == PGM_ASX_TRANSLATION_EXCEPTION
          || code == PGM_PRIMARY_AUTHORITY_EXCEPTION
          || code == PGM_SECONDARY_AUTHORITY_EXCEPTION
          || code == PGM_SPACE_SWITCH_EVENT
          || code == PGM_LX_TRANSLATION_EXCEPTION
          || code == PGM_LFX_TRANSLATION_EXCEPTION             /*@ALR*/
          || code == PGM_LSX_TRANSLATION_EXCEPTION             /*@ALR*/
          || code == PGM_LSTE_SEQUENCE_EXCEPTION               /*@ALR*/
          || code == PGM_EX_TRANSLATION_EXCEPTION)
        {
            STORE_FW(psa->TEA_L, regs->TEA);
        }
#else /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
        /* Store the translation exception address at PSA+144 */
        if ( code == PGM_PAGE_TRANSLATION_EXCEPTION
          || code == PGM_SEGMENT_TRANSLATION_EXCEPTION
          || code == PGM_AFX_TRANSLATION_EXCEPTION
          || code == PGM_ASX_TRANSLATION_EXCEPTION
          || code == PGM_PRIMARY_AUTHORITY_EXCEPTION
          || code == PGM_SECONDARY_AUTHORITY_EXCEPTION
          || code == PGM_SPACE_SWITCH_EVENT
          || code == PGM_LX_TRANSLATION_EXCEPTION
          || code == PGM_EX_TRANSLATION_EXCEPTION
#ifdef FEATURE_SUPPRESSION_ON_PROTECTION
          || code == PGM_PROTECTION_EXCEPTION
#endif
           )
        {
            STORE_FW(psa->tea, regs->TEA);
        }
#endif /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
        realregs->TEA = 0;

        /* Store Data exception code in PSA */
        if (code == PGM_DATA_EXCEPTION)
        {
            STORE_FW(psa->DXC, regs->dxc);
#ifdef FEATURE_BASIC_FP_EXTENSIONS
            /* Load data exception code into FPC register byte 2 */
            if(regs->CR(0) & CR0_AFP)
            {
                regs->fpc &= ~(FPC_DXC);
                regs->fpc |= ((regs->dxc << 8)) & FPC_DXC;
            }
#endif
        }

        /* Store the monitor class and event code */
        if (code == PGM_MONITOR_EVENT)
        {
            STORE_HW(psa->monclass, regs->monclass);

            /* Store the monitor code word at PSA+156 */
            /* or doubleword at PSA+176               */
            /* ISW20090110-1 ZSIEMCFIX                */
            /* In the event of a z/Arch guest being   */
            /* intercepted during a succesful Monitor */
            /* call, the monitor code is not stored   */
            /* at psa->moncode (which is beyond sie2bk->ip */
            /* but rather at the same location as an  */
            /* I/O interrupt would store the SSID     */
            /*    zmoncode points to this location    */
            /*  **** FIXME **** FIXME  *** FIXME ***  */
            /* ---- The determination of the location */
            /*      of the z/Sie Intercept moncode    */
            /*      should be made more flexible      */
            /*      and should be put somewhere in    */
            /*      esa390.h                          */
            /*  **** FIXME **** FIXME  *** FIXME ***  */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
            STORE_DW(zmoncode, regs->MONCODE);
#else
            STORE_W(psa->moncode, regs->MONCODE);
#endif
        }

#if defined(FEATURE_PER3)
        /* Store the breaking event address register in the PSA */
        SET_BEAR_REG(regs, regs->bear_ip);
        STORE_W(psa->bea, regs->bear);
#endif /*defined(FEATURE_PER3)*/

    } /* end if(ECMODE) */

#if defined(_FEATURE_PROTECTION_INTERCEPTION_CONTROL)
    realregs->hostint = 0;
#endif /*defined(_FEATURE_PROTECTION_INTERCEPTION_CONTROL)*/

#if defined(_FEATURE_SIE)
    if(nointercept)
#endif /*defined(_FEATURE_SIE)*/
    {
        PSW pgmold, pgmnew;
        int pgmintloop = 0;
        int detect_pgmintloop = FACILITY_ENABLED( HERC_DETECT_PGMINTLOOP, realregs );

        /* Store current PSW at PSA+X'28' or PSA+X'150' for ESAME */
        ARCH_DEP(store_psw) (realregs, psa->pgmold);

        /* Save program old psw */
        if (detect_pgmintloop)
        {
            memcpy( &pgmold, &realregs->psw, sizeof( PSW ));
            pgmold.cc      = 0;
            pgmold.intcode = 0;
            pgmold.ilc     = 0;
            pgmold.unused  = 0;
        }

        /* Load new PSW from PSA+X'68' or PSA+X'1D0' for ESAME */
        if ( (code = ARCH_DEP(load_psw) (realregs, psa->pgmnew)) )
        {
#if defined(_FEATURE_SIE)
            if(SIE_MODE(realregs))
            {
                longjmp(realregs->progjmp, pcode);
            }
            else
#endif /*defined(_FEATURE_SIE)*/
            /* Invalid pgmnew ==> program interrupt loop */
            pgmintloop = detect_pgmintloop;
        }
        else if (detect_pgmintloop)
        {
            /* Save program new psw */
            memcpy( &pgmnew, &realregs->psw, sizeof( PSW ));
            pgmnew.cc      = 0;
            pgmnew.intcode = 0;
            pgmnew.ilc     = 0;
            pgmnew.unused  = 0;

            /* Adjust pgmold instruction address */
            pgmold.ia.D -= ilc;

            /* Check for program interrupt loop (old==new) */
            if (memcmp( &pgmold, &pgmnew, sizeof( PSW )) == 0)
                pgmintloop = 1;
        }

        if (pgmintloop)
        {
            char buf[64];
            // "Processor %s%02X: program interrupt loop PSW %s"
            WRMSG(HHC00803, "I", PTYPSTR(realregs->cpuad), realregs->cpuad,
                     str_psw (realregs, buf));
            OBTAIN_INTLOCK(realregs);
            realregs->cpustate = CPUSTATE_STOPPING;
            ON_IC_INTERRUPT(realregs);
            RELEASE_INTLOCK(realregs);
        }

        longjmp(realregs->progjmp, SIE_NO_INTERCEPT);
    }

#if defined(_FEATURE_SIE)
    longjmp (realregs->progjmp, pcode);
#endif /*defined(_FEATURE_SIE)*/

} /* end function ARCH_DEP(program_interrupt) */

/*-------------------------------------------------------------------*/
/* Load restart new PSW                                              */
/*-------------------------------------------------------------------*/
static void ARCH_DEP(restart_interrupt) (REGS *regs)
{
int     rc;                             /* Return code               */
PSA    *psa;                            /* -> Prefixed storage area  */

    PTT_INF("*RESTART",regs->cpuad,regs->cpustate,regs->psw.IA_L);

    /* Set the main storage reference and change bits */
    STORAGE_KEY(regs->PX, regs) |= (STORKEY_REF | STORKEY_CHANGE);

    /* Zeroize the interrupt code in the PSW */
    regs->psw.intcode = 0;

    /* Point to PSA in main storage */
    psa = (PSA*)(regs->mainstor + regs->PX);

    /* Store current PSW at PSA+X'8' or PSA+X'120' for ESAME  */
    ARCH_DEP(store_psw) (regs, psa->RSTOLD);

    /* Load new PSW from PSA+X'0' or PSA+X'1A0' for ESAME */
    rc = ARCH_DEP(load_psw) (regs, psa->RSTNEW);

    if ( rc == 0)
    {
        regs->opinterv = 0;
        regs->cpustate = CPUSTATE_STARTED;
    }

    RELEASE_INTLOCK(regs);

    if ( rc )
        regs->program_interrupt(regs, rc);

    longjmp (regs->progjmp, SIE_INTERCEPT_RESTART);
} /* end function restart_interrupt */

/*-------------------------------------------------------------------*/
/* Perform I/O interrupt if pending                                  */
/* Note: The caller MUST hold the interrupt lock (sysblk.intlock)    */
/*-------------------------------------------------------------------*/
void ARCH_DEP(perform_io_interrupt) (REGS *regs)
{
int     rc;                             /* Return code               */
int     icode;                          /* Intercept code            */
PSA    *psa;                            /* -> Prefixed storage area  */
U32     ioparm;                         /* I/O interruption parameter*/
U32     ioid;                           /* I/O interruption address  */
U32     iointid;                        /* I/O interruption ident    */
RADR    pfx;                            /* Prefix                    */
DBLWRD  csw;                            /* CSW for S/370 channels    */

    /* Test and clear pending I/O interrupt */
    icode = ARCH_DEP(present_io_interrupt) (regs, &ioid, &ioparm, &iointid, csw);

    /* Exit if no interrupt was presented */
    if (icode == 0) return;

    PTT_IO("*IOINT",ioid,ioparm,iointid);

#if defined(_FEATURE_IO_ASSIST)
    if(SIE_MODE(regs) && icode != SIE_NO_INTERCEPT)
    {
        /* Point to SIE copy of PSA in state descriptor */
        psa = (void*)(regs->hostregs->mainstor + SIE_STATE(regs) + SIE_II_PSA_OFFSET);
        STORAGE_KEY(SIE_STATE(regs), regs->hostregs) |= (STORKEY_REF | STORKEY_CHANGE);
    }
    else
#endif
    {
        /* Point to PSA in main storage */
        pfx =
#if defined(_FEATURE_SIE)
              SIE_MODE(regs) ? regs->sie_px :
#endif
              regs->PX;
        psa = (void*)(regs->mainstor + pfx);
        STORAGE_KEY(pfx, regs) |= (STORKEY_REF | STORKEY_CHANGE);
    }

#ifdef FEATURE_S370_CHANNEL
    /* CSW has already been stored at PSA+X'40' */

    if (sysblk.arch_mode == ARCH_370_IDX &&
        ECMODE(&regs->psw))
    {
        /* For ECMODE, store the I/O device address at PSA+X'B8' */
        STORE_FW(psa->ioid,
                 ((((U32)psa->ioid[0] << 8) |
                   ((U32)SSID_TO_LCSS(ioid >> 16) & 0x07)) << 16) |
                 (ioid & 0x0000FFFFUL));
    }
    else
    {
        /* Set the interrupt code to the I/O device address */
        regs->psw.intcode = ioid;
    }

    /* Trace the I/O interrupt */
    if (CPU_STEPPING_OR_TRACING(regs, 0))
    {
        BYTE*   csw = psa->csw;

        WRMSG (HHC00804, "I", PTYPSTR(regs->cpuad), regs->cpuad,
                SSID_TO_LCSS(ioid >> 16) & 0x07, ioid,
                csw[0], csw[1], csw[2], csw[3],
                csw[4], csw[5], csw[6], csw[7]);
    }
#endif /*FEATURE_S370_CHANNEL*/

#ifdef FEATURE_CHANNEL_SUBSYSTEM
    /* Store X'0001' + subchannel number at PSA+X'B8' */
    STORE_FW(psa->ioid, ioid);

    /* Store the I/O interruption parameter at PSA+X'BC' */
    STORE_FW(psa->ioparm, ioparm);

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) || defined( _FEATURE_IO_ASSIST )
    /* Store the I/O interruption identification word at PSA+X'C0' */
    STORE_FW(psa->iointid, iointid);
#endif

    /* Trace the I/O interrupt */
    if (CPU_STEPPING_OR_TRACING(regs, 0))
#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) && !defined( _FEATURE_IO_ASSIST )
        WRMSG (HHC00805, "I", PTYPSTR(regs->cpuad), regs->cpuad, ioid, ioparm);
#else
        WRMSG (HHC00806, "I", PTYPSTR(regs->cpuad), regs->cpuad, ioid, ioparm, iointid);
#endif
#endif /* FEATURE_CHANNEL_SUBSYSTEM */

#if defined( _FEATURE_IO_ASSIST )
    if(icode == SIE_NO_INTERCEPT)
#endif
    {
        /* Store current PSW at PSA+X'38' or PSA+X'170' for ESAME */
        ARCH_DEP(store_psw) ( regs, psa->iopold );

        /* Load new PSW from PSA+X'78' or PSA+X'1F0' for ESAME */
        rc = ARCH_DEP(load_psw) ( regs, psa->iopnew );

        if ( rc )
        {
            RELEASE_INTLOCK(regs);
            regs->program_interrupt (regs, rc);
        }
    }

    RELEASE_INTLOCK(regs);

    longjmp(regs->progjmp, icode);

} /* end function perform_io_interrupt */

/*-------------------------------------------------------------------*/
/* Perform Machine Check interrupt if pending                        */
/* Note: The caller MUST hold the interrupt lock (sysblk.intlock)    */
/*-------------------------------------------------------------------*/
static void ARCH_DEP(perform_mck_interrupt) (REGS *regs)
{
int     rc;                             /* Return code               */
PSA    *psa;                            /* -> Prefixed storage area  */
U64     mcic;                           /* Mach.check interrupt code */
U32     xdmg;                           /* External damage code      */
RADR    fsta;                           /* Failing storage address   */

    /* Test and clear pending machine check interrupt */
    rc = ARCH_DEP(present_mck_interrupt) (regs, &mcic, &xdmg, &fsta);

    /* Exit if no machine check was presented */
    if (rc == 0) return;

    /* Set the main storage reference and change bits */
    STORAGE_KEY(regs->PX, regs) |= (STORKEY_REF | STORKEY_CHANGE);

    /* Point to the PSA in main storage */
    psa = (void*)(regs->mainstor + regs->PX);

    /* Store registers in machine check save area */
    ARCH_DEP(store_status) (regs, regs->PX);

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    /* Set the extended logout area to zeros */
    memset(psa->storepsw, 0, 16);
#endif

    /* Store the machine check interrupt code at PSA+232 */
    STORE_DW(psa->mckint, mcic);

    /* Trace the machine check interrupt */
    if (CPU_STEPPING_OR_TRACING(regs, 0))
        WRMSG (HHC00807, "I", PTYPSTR(regs->cpuad), regs->cpuad, mcic);

    /* Store the external damage code at PSA+244 */
    STORE_FW(psa->xdmgcode, xdmg);

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    /* Store the failing storage address at PSA+248 */
    STORE_DW(psa->mcstorad, fsta);
#else
    /* Store the failing storage address at PSA+248 */
    STORE_FW(psa->mcstorad, fsta);
#endif

    /* Store current PSW at PSA+X'30' */
    ARCH_DEP(store_psw) ( regs, psa->mckold );

    /* Load new PSW from PSA+X'70' */
    rc = ARCH_DEP(load_psw) ( regs, psa->mcknew );

    RELEASE_INTLOCK(regs);

    if ( rc )
        regs->program_interrupt (regs, rc);

    longjmp (regs->progjmp, SIE_INTERCEPT_MCK);
} /* end function perform_mck_interrupt */


#if !defined( _GEN_ARCH )

REGS* s370_run_cpu( int cpu, REGS* oldregs );
REGS* s390_run_cpu( int cpu, REGS* oldregs );
REGS* z900_run_cpu( int cpu, REGS* oldregs );

static REGS* (*run_cpu[ NUM_GEN_ARCHS ])( int cpu, REGS* oldregs ) =
{
#if defined(      _ARCH_NUM_0 )
  #if      370 == _ARCH_NUM_0
          s370_run_cpu,

  #elif    390 == _ARCH_NUM_0
          s390_run_cpu,

  #else // 900 == _ARCH_NUM_0
          z900_run_cpu,
  #endif
#endif
#if defined(      _ARCH_NUM_1 )
  #if      370 == _ARCH_NUM_1
          s370_run_cpu,

  #elif    390 == _ARCH_NUM_1
          s390_run_cpu,

  #else // 900 == _ARCH_NUM_1
          z900_run_cpu,
  #endif
#endif
#if defined(      _ARCH_NUM_2 )
  #if      370 == _ARCH_NUM_2
          s370_run_cpu,

  #elif    390 == _ARCH_NUM_2
          s390_run_cpu,

  #else // 900 == _ARCH_NUM_2
          z900_run_cpu,
  #endif
#endif
};

/*-------------------------------------------------------------------*/
/* CPU instruction execution thread                                  */
/*-------------------------------------------------------------------*/
void *cpu_thread (void *ptr)
{
REGS *regs = NULL;
int   cpu  = *(int*)ptr;
char  thread_name[16];
int   rc;

    OBTAIN_INTLOCK(NULL);

    /* Increment number of CPUs online */
    sysblk.cpus++;

    /* Set hi CPU */
    if (cpu >= sysblk.hicpu)
        sysblk.hicpu = cpu + 1;

    /* Start the TOD clock and CPU timer thread */
    if (!sysblk.todtid)
    {
        rc = create_thread( &sysblk.todtid, DETACHED,
             timer_thread, NULL, TIMER_THREAD_NAME );
        if (rc)
        {
            WRMSG( HHC00102, "E", strerror( rc ));
            RELEASE_INTLOCK( NULL );
            return NULL;
        }
    }

    /* Set CPU thread priority */
    set_thread_priority( sysblk.cpuprio);

    /* Display thread started message on control panel */

    MSGBUF( thread_name, "Processor %s%02X", PTYPSTR( cpu ), cpu );
    WRMSG( HHC00100, "I", thread_id(), get_thread_priority(), thread_name );
    SET_THREAD_NAME( thread_name );

    /* Execute the program in specified mode */
    do {
        regs = run_cpu[sysblk.arch_mode] (cpu, regs);
    } while (regs);

    /* Decrement number of CPUs online */
    sysblk.cpus--;

    /* Reset hi cpu */
    if (cpu + 1 >= sysblk.hicpu)
    {
        int i;
        for (i = sysblk.maxcpu - 1; i >= 0; i--)
            if (IS_CPU_ONLINE(i))
                break;
        sysblk.hicpu = i + 1;
    }

    /* Signal cpu has terminated */
    signal_condition (&sysblk.cpucond);

    /* Display thread ended message on control panel */
    WRMSG(HHC00101, "I", thread_id(),  get_thread_priority(), thread_name);

    RELEASE_INTLOCK(NULL);

    return NULL;
} /* end function cpu_thread */

/*-------------------------------------------------------------------*/
/* Initialize a CPU                                                  */
/*-------------------------------------------------------------------*/
int cpu_init (int cpu, REGS *regs, REGS *hostregs)
{
int i;

    obtain_lock (&sysblk.cpulock[cpu]);

    /* initialize eye-catchers */
    {
        char    blknam[ sizeof( regs->blknam )];
        MSGBUF( blknam, "%-4.4s_%s%02X", HDL_NAME_REGS, PTYPSTR( cpu ), cpu );
        INIT_BLOCK_HEADER_TRAILER_WITH_CUSTOM_NAME( regs, REGS, blknam );
    }

    regs->cpuad = cpu;
    regs->cpubit = CPU_BIT(cpu);

    /* Save CPU creation time without epoch set, as epoch may change. When using
     * the field, subtract the current epoch from any time being used in
     * relation to the creation time to yield the correct result.
     */
    if (sysblk.cpucreateTOD[cpu] == 0)
        sysblk.cpucreateTOD[cpu] = host_tod(); /* tod_epoch is zero at this point */

    regs->arch_mode = sysblk.arch_mode;
    regs->mainstor  = sysblk.mainstor;
    regs->sysblk    = &sysblk;
    regs->storkeys  = sysblk.storkeys;
    regs->mainlim   = sysblk.mainsize - 1;
    regs->tod_epoch = get_tod_epoch();

    /* Set initial CPU ID by REGS context.  Note that this
       must only be done AFTER regs->arch_mode has been set.
    */
    setCpuIdregs( regs, -1, -1, -1, -1 );

    initialize_condition (&regs->intcond);
    regs->cpulock = &sysblk.cpulock[cpu];

#if defined( _FEATURE_S370_S390_VECTOR_FACILITY )
    regs->vf = &sysblk.vf[cpu];
    regs->vf->online = (cpu < sysblk.numvec);
#endif
    initial_cpu_reset(regs);

    if (hostregs == NULL)
    {
        regs->cpustate = CPUSTATE_STOPPING;
        ON_IC_INTERRUPT(regs);
        regs->hostregs = regs;
        regs->host = 1;
        sysblk.regs[cpu] = regs;
        sysblk.config_mask |= regs->cpubit;
        sysblk.started_mask |= regs->cpubit;
    }
    else
    {
        hostregs->guestregs = regs;
        regs->hostregs = hostregs;
        regs->guestregs = regs;
        regs->guest = 1;
        regs->sie_mode = 1;
        regs->opinterv = 0;
        regs->cpustate = CPUSTATE_STARTED;
    }

    /* Initialize accelerated lookup fields */
    regs->CR_G(CR_ASD_REAL) = TLB_REAL_ASD;

    for(i = 0; i < 16; i++)
        regs->AEA_AR(i)               = CR_ASD_REAL;
    regs->AEA_AR(USE_INST_SPACE)      = CR_ASD_REAL;
    regs->AEA_AR(USE_REAL_ADDR)       = CR_ASD_REAL;
    regs->AEA_AR(USE_PRIMARY_SPACE)   =  1;
    regs->AEA_AR(USE_SECONDARY_SPACE) =  7;
    regs->AEA_AR(USE_HOME_SPACE)      = 13;

    /* Initialize opcode table pointers */
    init_opcode_pointers (regs);

    regs->configured = 1;

    release_lock (&sysblk.cpulock[cpu]);

    return 0;
}

/*-------------------------------------------------------------------*/
/* Uninitialize a CPU                                                */
/*-------------------------------------------------------------------*/
void *cpu_uninit (int cpu, REGS *regs)
{
    int processHostRegs = (regs->host &&
                           regs == sysblk.regs[cpu]);

    if (processHostRegs)
    {
        obtain_lock (&sysblk.cpulock[cpu]);

        /* If pointing to guest REGS structure, free the guest REGS
         * structure ONLY if it is not ourself, and set the guest REGS
         * pointer to NULL;
         */
        if (regs->guestregs)
            regs->guestregs = regs == regs->guestregs ? NULL :
                              cpu_uninit (cpu, regs->guestregs);
    }

    destroy_condition(&regs->intcond);

    if (processHostRegs)
    {
#ifdef FEATURE_S370_S390_VECTOR_FACILITY
        /* Mark Vector Facility offline */
        regs->vf->online = 0;
#endif

        /* Remove CPU from all CPU bit masks */
        sysblk.config_mask &= ~CPU_BIT(cpu);
        sysblk.started_mask &= ~CPU_BIT(cpu);
        sysblk.waiting_mask &= ~CPU_BIT(cpu);
        sysblk.regs[cpu] = NULL;
        sysblk.cpucreateTOD[cpu] = 0;
        release_lock (&sysblk.cpulock[cpu]);
    }

    /* Free the REGS structure */
    free_aligned(regs);

    return NULL;
}

/*-------------------------------------------------------------------*/
/* CPU Wait - Core wait routine for both CPU Wait and Stopped States */
/*                                                                   */
/* Locks Held                                                        */
/*      sysblk.intlock                                               */
/*-------------------------------------------------------------------*/
void
CPU_Wait (REGS* regs)
{
    /* Indicate we are giving up intlock */
    sysblk.intowner = LOCK_OWNER_NONE;

    /* Wait while SYNCHRONIZE_CPUS is in progress */
    while (sysblk.syncing)
    {
        sysblk.sync_mask &= ~regs->hostregs->cpubit;
        if (!sysblk.sync_mask)
            signal_condition(&sysblk.sync_cond);
        wait_condition (&sysblk.sync_bc_cond, &sysblk.intlock);
    }

    /*
    ** Let test script know when a CPU either stops or loads a
    ** disabled wait state PSW, but NOT when a CPU simply loads
    ** an ENABLED wait PSW such as when it wishes to simply wait
    ** for an I/O, External or other type of interrupt.
    **
    ** NOTE: we can't rely on just sysblk.started_mask since there
    ** is no guarantee the last CPU to have its sysblk.started_mask
    ** cleared will actually be the last CPU to reach this point.
    */
    if (WAITSTATE( &regs->psw ) && !IS_IC_DISABLED_WAIT_PSW( regs ))
        ;   /* enabled wait: do nothing */
    else
    {
        obtain_lock( &sysblk.scrlock );
        if (sysblk.scrtest)
        {
            sysblk.scrtest++;
            broadcast_condition( &sysblk.scrcond );
        }
        release_lock( &sysblk.scrlock );
    }

    /* Wait for interrupt */
    wait_condition (&regs->intcond, &sysblk.intlock);

    /* And we're the owner of intlock once again */
    sysblk.intowner = regs->cpuad;
}

#endif /*!defined(_GEN_ARCH)*/

/*-------------------------------------------------------------------*/
/* Process interrupt                                                 */
/*-------------------------------------------------------------------*/
void (ATTR_REGPARM(1) ARCH_DEP(process_interrupt))(REGS *regs)
{
    /* Process PER program interrupts */
    if( OPEN_IC_PER(regs) )
        regs->program_interrupt (regs, PGM_PER_EVENT);

    /* Obtain the interrupt lock */
    OBTAIN_INTLOCK(regs);
    OFF_IC_INTERRUPT(regs);
    regs->tracing = (sysblk.inststep || sysblk.insttrace);

    /* Ensure psw.IA is set and invalidate the aia */
    INVALIDATE_AIA(regs);

    /* Perform invalidation */
    if (unlikely(regs->invalidate))
        ARCH_DEP(invalidate_tlbe)(regs, regs->invalidate_main);

    /* Take interrupts if CPU is not stopped */
    if (likely(regs->cpustate == CPUSTATE_STARTED))
    {
        /* Process machine check interrupt */
        if ( OPEN_IC_MCKPENDING(regs) )
        {
            PERFORM_SERIALIZATION (regs);
            PERFORM_CHKPT_SYNC (regs);
            ARCH_DEP (perform_mck_interrupt) (regs);
        }

        /* Process external interrupt */
        if ( OPEN_IC_EXTPENDING(regs) )
        {
            PERFORM_SERIALIZATION (regs);
            PERFORM_CHKPT_SYNC (regs);
            ARCH_DEP (perform_external_interrupt) (regs);
        }

        /* Process I/O interrupt */
        if (IS_IC_IOPENDING)
        {
            if ( OPEN_IC_IOPENDING(regs) )
            {
                PERFORM_SERIALIZATION (regs);
                PERFORM_CHKPT_SYNC (regs);
                ARCH_DEP (perform_io_interrupt) (regs);
            }
            else
                WAKEUP_CPU_MASK(sysblk.waiting_mask);
        }
    } /*CPU_STARTED*/

    /* If CPU is stopping, change status to stopped */
    if (unlikely(regs->cpustate == CPUSTATE_STOPPING))
    {
        /* Change CPU status to stopped */
cpustate_stopping:
        regs->opinterv = 0;
        regs->cpustate = CPUSTATE_STOPPED;

        /* Thread exit (note - intlock still held) */
        if (!regs->configured)
            longjmp(regs->exitjmp, SIE_NO_INTERCEPT);

        /* If initial CPU reset pending then perform reset */
        if (regs->sigp_ini_reset)
        {
            PERFORM_SERIALIZATION (regs);
            PERFORM_CHKPT_SYNC (regs);
            ARCH_DEP (initial_cpu_reset) (regs);
            RELEASE_INTLOCK(regs);
            longjmp(regs->progjmp, SIE_NO_INTERCEPT);
        }

        /* If a CPU reset is pending then perform the reset */
        if (regs->sigp_reset)
        {
            PERFORM_SERIALIZATION (regs);
            PERFORM_CHKPT_SYNC (regs);
            ARCH_DEP(cpu_reset) (regs);
            RELEASE_INTLOCK(regs);
            longjmp(regs->progjmp, SIE_NO_INTERCEPT);
        }

        /* Store status at absolute location 0 if requested */
        if (IS_IC_STORSTAT(regs))
        {
            OFF_IC_STORSTAT(regs);
            ARCH_DEP(store_status) (regs, 0);
            WRMSG (HHC00808, "I", PTYPSTR(regs->cpuad), regs->cpuad);
            /* ISW 20071102 : Do not return via longjmp here. */
            /*    process_interrupt needs to finish putting the */
            /*    CPU in its manual state                     */
            /*
            RELEASE_INTLOCK(regs);
            longjmp(regs->progjmp, SIE_NO_INTERCEPT);
            */
        }
    } /*CPUSTATE_STOPPING*/

    /* Perform restart interrupt if pending */
    if ( IS_IC_RESTART(regs) )
    {
        PERFORM_SERIALIZATION (regs);
        PERFORM_CHKPT_SYNC (regs);
        OFF_IC_RESTART(regs);
        ARCH_DEP(restart_interrupt) (regs);
    } /* end if(restart) */

    /* This is where a stopped CPU will wait */
    if (unlikely(regs->cpustate == CPUSTATE_STOPPED))
    {
        S64 saved_timer = cpu_timer(regs);
        regs->ints_state = IC_INITIAL_STATE;
        sysblk.started_mask ^= regs->cpubit;

        CPU_Wait(regs);

        sysblk.started_mask |= regs->cpubit;
        regs->ints_state |= sysblk.ints_state;
        set_cpu_timer(regs,saved_timer);

        ON_IC_INTERRUPT(regs);

        /* Purge the lookaside buffers */
        ARCH_DEP(purge_tlb) (regs);
#if defined(FEATURE_ACCESS_REGISTERS)
        ARCH_DEP(purge_alb) (regs);
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/

        /* If the architecture mode has changed we must adapt */
        if(sysblk.arch_mode != regs->arch_mode)
            longjmp(regs->archjmp,SIE_NO_INTERCEPT);

        RELEASE_INTLOCK(regs);
        longjmp(regs->progjmp, SIE_NO_INTERCEPT);
    } /*CPUSTATE_STOPPED*/

    /* Test for wait state */
    if (WAITSTATE(&regs->psw))
    {
        regs->waittod = host_tod();

        /* Test for disabled wait PSW and issue message */
        if( IS_IC_DISABLED_WAIT_PSW(regs) )
        {
            char buf[40];
            // "Processor %s%02X: disabled wait state %s"
            WRMSG (HHC00809, "I", PTYPSTR(regs->cpuad), regs->cpuad, str_psw(regs, buf));
            regs->cpustate = CPUSTATE_STOPPING;
            RELEASE_INTLOCK(regs);

            longjmp(regs->progjmp, SIE_NO_INTERCEPT);
        }

        /* Indicate waiting and invoke CPU wait */
        sysblk.waiting_mask |= regs->cpubit;
        CPU_Wait(regs);

        /* Turn off the waiting bit .
         *
         * Note: ANDing off of the CPU waiting bit, rather than using
         * XOR, is required to handle the remote and rare case when the
         * CPU is removed from the sysblk.waiting_mask while in
         * wait_condition (intlock is NOT held; use of XOR incorrectly
         * turns the CPU waiting bit back on).
         */
        sysblk.waiting_mask &= ~(regs->cpubit);

        /* Calculate the time we waited */
        regs->waittime += host_tod() - regs->waittod;
        regs->waittod = 0;

        /* If late state change to stopping, go reprocess */
        if (unlikely(regs->cpustate == CPUSTATE_STOPPING))
            goto cpustate_stopping;

        RELEASE_INTLOCK(regs);
        longjmp(regs->progjmp, SIE_NO_INTERCEPT);
    } /* end if(wait) */

    /* Release the interrupt lock */
    RELEASE_INTLOCK(regs);
    return;

} /* process_interrupt */

/*-------------------------------------------------------------------*/
/* Run CPU                                                           */
/*-------------------------------------------------------------------*/
REGS *ARCH_DEP(run_cpu) (int cpu, REGS *oldregs)
{
const INSTR_FUNC   *current_opcode_table;
register REGS   *regs;
BYTE   *ip;
int     i;
int     aswitch;

    /* Assign new regs if not already assigned */
    regs = sysblk.regs[cpu] ?
           sysblk.regs[cpu] :
           malloc_aligned(((sizeof(REGS)+4095)&~((size_t)0x0FFF)), 4096);

    if (oldregs)
    {
        if (oldregs != regs)
        {
            memcpy (regs, oldregs, sizeof(REGS));
            free_aligned(oldregs);
            regs->blkloc = CSWAP64((U64)((uintptr_t)regs));
            regs->hostregs = regs;
            if (regs->guestregs)
                regs->guestregs->hostregs = regs;
            sysblk.regs[cpu] = regs;
            release_lock(&sysblk.cpulock[cpu]);
            // "Processor %s%02X: architecture mode %s"
            WRMSG (HHC00811, "I", PTYPSTR(cpu), cpu, get_arch_name(regs));
        }
    }
    else
    {
        memset(regs, 0, sizeof(REGS));

        if (cpu_init (cpu, regs, NULL))
            return NULL;

        // "Processor %s%02X: architecture mode %s"
        WRMSG (HHC00811, "I", PTYPSTR(cpu), cpu, get_arch_name(regs));

#ifdef FEATURE_S370_S390_VECTOR_FACILITY
        if (regs->vf->online)
            WRMSG (HHC00812, "I", PTYPSTR(cpu), cpu);
#endif
    }

    regs->program_interrupt = &ARCH_DEP(program_interrupt);
#if defined( FEATURE_TRACING )
    regs->trace_br = (func)&ARCH_DEP(trace_br);
#endif

    regs->tracing = (sysblk.inststep || sysblk.insttrace);
    regs->ints_state |= sysblk.ints_state;

    /* Establish longjmp destination for cpu thread exit */
    if (setjmp(regs->exitjmp))
        return cpu_uninit(cpu, regs);

    /* Establish longjmp destination for architecture switch */
    aswitch = setjmp(regs->archjmp);

    /* Switch architecture mode if appropriate */
    if(sysblk.arch_mode != regs->arch_mode)
    {
        PTT_INF("*SETARCH",regs->arch_mode,sysblk.arch_mode,cpu);
        regs->arch_mode = sysblk.arch_mode;

        /* Ensure CPU ID is accurate in case archmode changed */
        setCpuIdregs( regs, -1, -1, -1, -1 );

        oldregs = malloc_aligned(sizeof(REGS), 4096);
        if (oldregs)
        {
            memcpy(oldregs, regs, sizeof(REGS));
            obtain_lock(&sysblk.cpulock[cpu]);
        }
        else
        {
            char buf[40];
            MSGBUF(buf, "malloc(%d)", (int)sizeof(REGS));
            // "Processor %s%02X: error in function %s: %s"
            WRMSG (HHC00813, "E", PTYPSTR(cpu), cpu, buf, strerror(errno));
            cpu_uninit (cpu, regs);
        }
        return oldregs;
    }

    /* Initialize Facilities List */
    init_cpu_facilities( regs );

    /* Get pointer to primary opcode table */
    current_opcode_table = regs->ARCH_DEP( runtime_opcode_xxxx );

    /* Signal cpu has started */
    if(!aswitch)
        signal_condition (&sysblk.cpucond);

    RELEASE_INTLOCK(regs);

    /* Establish longjmp destination for program check */
    setjmp(regs->progjmp);

    /* Set `execflag' to 0 in case EXecuted instruction did a longjmp() */
    regs->execflag = 0;

    do {
        if (INTERRUPT_PENDING( regs ))
            ARCH_DEP( process_interrupt )( regs );

        ip = INSTRUCTION_FETCH( regs, 0 );

        EXECUTE_INSTRUCTION( current_opcode_table, ip, regs );

        /* BHe: I have tried several settings. But 2 unrolled
           executes gives (core i7 at my place) the best results.

           Even a 'do { } while(0);' with several unrolled executes
           and without the 'i' was slower.

           That surprised me.
        */
        for (i=0; i < 128; i++)
        {
            UNROLLED_EXECUTE( current_opcode_table, regs );
            UNROLLED_EXECUTE( current_opcode_table, regs );
        }
        regs->instcount += 1 + (i * 2);

        /* Update system-wide sysblk.instcount instruction counter */
        UPDATE_SYSBLK_INSTCOUNT( 1 + (i * 2) );

        /* Perform automatic instruction tracing if it's enabled */
        do_automatic_tracing();
    }
    while (1);

    UNREACHABLE_CODE( return NULL );

} /* end function run_cpu */

/*-------------------------------------------------------------------*/
/* Process Trace                                                     */
/*-------------------------------------------------------------------*/
void ARCH_DEP(process_trace)(REGS *regs)
{
int     shouldtrace = 0;                /* 1=Trace instruction       */
int     shouldstep = 0;                 /* 1=Wait for start command  */

    /* Test for trace */
    if (CPU_TRACING(regs, 0))
        shouldtrace = 1;

    /* Test for step */
    if (CPU_STEPPING(regs, 0))
        shouldstep = 1;

    /* Display the instruction */
    if (shouldtrace || shouldstep)
    {
        BYTE *ip = regs->ip < regs->aip ? regs->inst : regs->ip;
        ARCH_DEP(display_inst) (regs, ip);
    }

    /* Stop the CPU */
    if (shouldstep)
    {
        REGS *hostregs = regs->hostregs;
        S64 saved_timer[2];

        OBTAIN_INTLOCK(hostregs);

        hostregs->waittod = host_tod();

        /* The CPU timer is not decremented for a CPU that is in
           the manual state (e.g. stopped in single step mode) */
        saved_timer[0] = cpu_timer(regs);
        saved_timer[1] = cpu_timer(hostregs);
        hostregs->cpustate = CPUSTATE_STOPPED;
        sysblk.started_mask &= ~hostregs->cpubit;
        hostregs->stepwait = 1;
        sysblk.intowner = LOCK_OWNER_NONE;

        while (hostregs->cpustate == CPUSTATE_STOPPED)
        {
            wait_condition (&hostregs->intcond, &sysblk.intlock);
        }

        sysblk.intowner = hostregs->cpuad;
        hostregs->stepwait = 0;
        sysblk.started_mask |= hostregs->cpubit;

        set_cpu_timer(regs,saved_timer[0]);
        set_cpu_timer(hostregs,saved_timer[1]);

        hostregs->waittime += host_tod() - hostregs->waittod;
        hostregs->waittod = 0;

        RELEASE_INTLOCK(hostregs);
    }
} /* process_trace */


#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "cpu.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "cpu.c"
#endif

/*-------------------------------------------------------------------*/
/* Copy program status word                                          */
/*-------------------------------------------------------------------*/
DLL_EXPORT void copy_psw (REGS *regs, BYTE *addr)
{
REGS cregs;
int  arch_mode;

    memcpy(&cregs, regs, sysblk.regs_copy_len);

    /* Use the architecture mode from SYSBLK if load indicator
       shows IPL in process, otherwise use archmode from REGS */
    if (cregs.loadstate)
    {
        arch_mode = sysblk.arch_mode;
    }
    else
    {
        arch_mode = cregs.arch_mode;
    }

    /* Call the appropriate store_psw routine based on archmode */
    switch(arch_mode) {
#if defined(_370)
        case ARCH_370_IDX:
            s370_store_psw(&cregs, addr);
            break;
#endif
#if defined(_390)
        case ARCH_390_IDX:
            s390_store_psw(&cregs, addr);
            break;
#endif
#if defined(_900)
        case ARCH_900_IDX:
            z900_store_psw(&cregs, addr);
            break;
#endif
    }
} /* end function copy_psw */

/*-------------------------------------------------------------------*/
/* Display program status word                                       */
/*-------------------------------------------------------------------*/
int display_psw (REGS *regs, char *buf, int buflen)
{
QWORD   qword;                            /* quadword work area      */
int     arch_mode;                        /* architecture mode       */

    /* Use the architecture mode from SYSBLK if load indicator
       shows IPL in process, otherwise use archmode from REGS */
    if (regs->loadstate)
    {
        arch_mode = sysblk.arch_mode;
    }
    else
    {
        arch_mode = regs->arch_mode;
    }

    memset(qword, 0, sizeof(qword));

    if( arch_mode != ARCH_900_IDX )
    {
        copy_psw (regs, qword);
        return(snprintf(buf, buflen,
                "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
                qword[0], qword[1], qword[2], qword[3],
                qword[4], qword[5], qword[6], qword[7]));
    }
    else
    {
        copy_psw (regs, qword);
        return(snprintf(buf, buflen,
                "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
                qword[0], qword[1], qword[2], qword[3],
                qword[4], qword[5], qword[6], qword[7],
                qword[8], qword[9], qword[10], qword[11],
                qword[12], qword[13], qword[14], qword[15]));
    }

} /* end function display_psw */

/*-------------------------------------------------------------------*/
/* Print program status word into buffer                             */
/*-------------------------------------------------------------------*/
char *str_psw (REGS *regs, char *buf)
{
QWORD   qword;                            /* quadword work area      */

    memset(qword, 0, sizeof(qword));

    if( regs->arch_mode != ARCH_900_IDX )
    {
        copy_psw (regs, qword);
        sprintf(buf, "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X",
                qword[0], qword[1], qword[2], qword[3],
                qword[4], qword[5], qword[6], qword[7]);
    }
    else
    {
        copy_psw (regs, qword);
        sprintf(buf, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X "
                     "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
                qword[0], qword[1], qword[2], qword[3],
                qword[4], qword[5], qword[6], qword[7],
                qword[8], qword[9], qword[10], qword[11],
                qword[12], qword[13], qword[14], qword[15]);
    }
    return(buf);

} /* end function str_psw */

/*-------------------------------------------------------------------*/
/*                      Automatic Tracing                            */
/*-------------------------------------------------------------------*/
void do_automatic_tracing()
{
    static U64  inst_count;         // (current sysblk.instcount)
    static U64  missed_by;          // (how far past trigger we went)
    static U64  too_much;           // (num extra instructions traced)

    bool started = false, stopped = false;

    /* Return immediately if automatic tracing not enabled or active */
    if (!sysblk.auto_trace_amt)
        return;

    OBTAIN_INTLOCK( NULL );
    {
        static U64  beg_count  = 0;     // (when auto-tracing began)

        static U64  auto_trace_beg;     // (instrcount to begin tracing)
        static U64  auto_trace_amt;     // (amt of instruction to trace)

        static U64  traced_amt;         // (amt instr. traced so far)

        /* Check again under control of INTLOCK */
        if (!sysblk.auto_trace_amt)
        {
            RELEASE_INTLOCK( NULL );
            return;
        }

        auto_trace_beg  = sysblk.auto_trace_beg;
        auto_trace_amt  = sysblk.auto_trace_amt;
        inst_count      = sysblk.instcount;

        /* Should Automatic Tracing be started? */
        if (1
            && !beg_count                       // (hasn't begun yet?)
            && inst_count >= auto_trace_beg     // (but it should be?)
        )
        {
            started = true;                     // (remember started)
            beg_count = inst_count;             // (when it was begun)
            missed_by = (inst_count - auto_trace_beg);
            sysblk.insttrace = true;            // (activate tracing)
            sysblk.auto_trace_beg = 0;          // (prevent re-trigger)
            SET_IC_TRACE;                       // (force interrupt)
        }

        /* Should Automatic Tracing be stopped? */
        else if (1
            && beg_count
            && (traced_amt = (inst_count - beg_count)) >= auto_trace_amt
        )
        {
            stopped = true;                     // (remember stopped)
            beg_count = 0;                      // (reset for next time)
            too_much = (traced_amt - auto_trace_amt);
            sysblk.insttrace = false;           // (deactivate tracing)
            sysblk.auto_trace_amt = 0;          // (prevent re-trigger)
            SET_IC_TRACE;                       // (force interrupt)
        }
    }
    RELEASE_INTLOCK( NULL );

    if (started)
    {
        // "Automatic tracing started at instrcount %"PRIu64" (BEG+%"PRIu64")"
        WRMSG( HHC02370, "I", inst_count, missed_by );
    }
    else if (stopped)
    {
        // "Automatic tracing stopped at instrcount %"PRIu64" (AMT+%"PRIu64")"
        WRMSG( HHC02371, "I", inst_count, too_much );
    }
}

#endif /*!defined(_GEN_ARCH)*/
