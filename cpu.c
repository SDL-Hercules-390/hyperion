/* CPU.C        (C) Copyright Roger Bowler, 1994-2012                */
/*              (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) and others 2013-2023                             */
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
/*      Basic FP extensions support - Peter Kuschnerus               */
/*      ASN-and-LX-reuse facility - Roger Bowler, June 2004          */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

// #define SIE_DEBUG

#define _CPU_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*-------------------------------------------------------------------*/

//-------------------------------------------------------------------
//                      ARCH_DEP() code
//-------------------------------------------------------------------
// ARCH_DEP (build-architecture / FEATURE-dependent) functions here.
// All BUILD architecture dependent (ARCH_DEP) function are compiled
// multiple times (once for each defined build architecture) and each
// time they are compiled with a different set of FEATURE_XXX defines
// appropriate for that architecture. Use #ifdef FEATURE_XXX guards
// to check whether the current BUILD architecture has that given
// feature #defined for it or not. WARNING: Do NOT use _FEATURE_XXX.
// The underscore feature #defines mean something else entirely. Only
// test for FEATURE_XXX. (WITHOUT the underscore)
//-------------------------------------------------------------------

/*-------------------------------------------------------------------*/
/* Forward references to internal static helper functions at end.    */
/*-------------------------------------------------------------------*/
#if !defined( FWD_REFS)
    #define   FWD_REFS
  static void  CPU_Wait( REGS* regs );
  static void* cpu_uninit( int cpu, REGS* regs );
#endif

/*-------------------------------------------------------------------*/
/* Put all the CPUs in the configuration in check-stop state         */
/* Caller *MUST* hold INTLOCK!                                       */
/*-------------------------------------------------------------------*/
void ARCH_DEP( checkstop_all_cpus )( REGS* regs )
{
    int  i;

    if (!IS_INTLOCK_HELD( regs ))
        CRASH();

    for (i=0; i < sysblk.maxcpu; i++)
    {
        if (IS_CPU_ONLINE(i))
        {
            sysblk.regs[i]->cpustate = CPUSTATE_STOPPING;
            sysblk.regs[i]->checkstop = 1;
            ON_IC_INTERRUPT( sysblk.regs[i] );
        }
    }
    WAKEUP_CPUS_MASK( sysblk.waiting_mask );
}

/*-------------------------------------------------------------------*/
/* Store current PSW at a specified absolute address in main storage */
/*-------------------------------------------------------------------*/
void ARCH_DEP( store_psw )( REGS* regs, BYTE* addr )
{
    /* Ensure psw.IA is set */
    if (!regs->psw.zeroilc)
        MAYBE_SET_PSW_IA_FROM_IP( regs );

#if defined( FEATURE_BCMODE )
    if (ECMODE( &regs->psw ))
#endif
        // 390 or 370 EC-mode

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        STORE_DW( addr, make_psw64( regs, 390, false ));
#endif

#if defined( FEATURE_BCMODE )

    else    // 370 BC-mode

        STORE_DW( addr, make_psw64( regs, 370, true ));

#elif defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )

    // 64-bit z/Architecture mode

    STORE_DW( addr + 0, make_psw64( regs, 900, false ));
    STORE_DW( addr + 8, regs->psw.IA_G );

#endif
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
    if( SIE_STATE_BIT_ON(regs, MX, XC)
      && ( (regs->psw.sysmask & PSW_DATMODE) || SPACE_BIT(&regs->psw)) )
        return PGM_SPECIFICATION_EXCEPTION;
#endif

    regs->psw.zeroilc = 0;

    /* Check for wait state PSW */
    if (1
        && WAITSTATE( &regs->psw )
        && CPU_STEPPING_OR_TRACING_ALL
        && !TXF_INSTR_TRACING()
    )
    {
        if (regs->insttrace && sysblk.traceFILE)
            tf_0800( regs );
        else
        {
            char buf[40];
            STR_PSW( regs, buf );
            // "Processor %s%02X: loaded wait state PSW %s"
            WRMSG( HHC00800, "I", PTYPSTR( regs->cpuad ), regs->cpuad, buf );
        }
    }

    TEST_SET_AEA_MODE(regs);

    return 0;
} /* end function ARCH_DEP(load_psw) */

#if defined( FEATURE_PER3 )
/*-------------------------------------------------------------------*/
/*            Set the Breaking-Event-Address Register                */
/*-------------------------------------------------------------------*/
DLL_EXPORT void ARCH_DEP( Set_BEAR_Reg )( U64* bear, REGS* regs, BYTE* ip )
{
    /* "If the instruction causing the breaking event is the
        target of an execute-type instruction (EX or EXRL),
        then the instruction address used to fetch the EX/EXRL
        instruction is placed in the BEAR."
    */
    if (1
        && bear != &regs->bear_ex   /* NOT saving EX/EXRL address?   */
        && regs->execflag           /* EX/EXRL target caused event?  */
    )
    {
        regs->bear = regs->bear_ex; /* BEAR = EX/EXRL instr address  */
        PTT_INF( "bear = ex", regs->bear_ex, 0, 0 );
    }
    else if (ip)
    {
        /* BEAR = Address of the beginning of virtual ('aiv') page
           plus same displacement from begin of mainstor ('ip') page
           also know as 'aip'.

           HOWEVER, since the 'ip' value passed to us might not match
           regs->ip (it might have been passed to us as "regs->ip - 4"),
           we cannot blindly rely on the 'ip' value passed to us being
           on the same mainstor page as regs->aip. It could be pointing
           before where regs->aip is currently pointing if regs->ip
           was pointing to the first instruction on the page and thus
           after backing up 4 bytes would cause it to point before
           where regs->aip points. The below logic takes situations
           such as that into consideration.
        */
        BYTE* aip = regs->aip;      /* Begin of mainstor page */
        U64   aiv = regs->AIV;      /* Begin of virtual page  */

        if (ip < regs->aip)         /* pointing to prev page? */
        {
            /* The instruction pointer that was passed to us
               points somewhere in the PREVIOUS mainstor page */

            PTT_INF( "ip < aip", ip, aip, aiv );

            aip -= PAGEFRAME_PAGESIZE;
            aiv -= PAGEFRAME_PAGESIZE;
        }
        else if (ip >= (regs->aip + PAGEFRAME_PAGESIZE))
        {
            /* The instruction pointer that was passed to us
               points somewhere in the NEXT mainstor page */

            PTT_INF( "ip >= aip+page", ip, aip, aiv );

            aip += PAGEFRAME_PAGESIZE;
            aiv += PAGEFRAME_PAGESIZE;
        }

        /* Calculate and set BEAR appropriately */
        *bear = aiv + (ip - aip);           /* Save virtual address  */
        *bear &= ADDRESS_MAXWRAP( regs );   /* of the breaking event */

        if (bear == &regs->bear)
            PTT_INF( "bear =", *bear, 0, 0 );
        else
            PTT_INF( "bear_ex =", *bear, 0, 0 );
    }
}
#endif

/*-------------------------------------------------------------------*/
/*                      SuccessfulBranch                             */
/*-------------------------------------------------------------------*/
void ARCH_DEP( SuccessfulBranch )( REGS* regs, VADR vaddr )
{
    vaddr &= ADDRESS_MAXWRAP( regs );

    /* Set BEAR to branch instruction. Note: for branch instructions
       regs->ip is not updated to point to the next instruction and
       thus is still pointing to the branch instruction itself.
    */
    SET_BEAR_REG( regs, regs->ip );

    /* Branch target still within same page as branch instruction? */
    if (1
        && !regs->permode
        && !regs->execflag
        && (vaddr & (PAGEFRAME_PAGEMASK | 0x01)) == regs->AIV
    )
    {
        /* Branch directly to the new instruction */
        regs->ip = regs->aip + (vaddr - regs->AIV);
        PTT_INF( "branch", vaddr, regs->AIV, regs->ip );
        return;
    }

    /* Branch target is in another page: point the PSW to the target
       instruction and force a new "regs->ip" value to get set by
       forcing a full instruction fetch from the new target address.
    */
    regs->psw.IA = vaddr;               /* Point PSW to target instr */
    regs->aie = INVALID_AIE;            /* Force a fresh 'instfetch' */

    PTT_INF( "branch", vaddr, regs->AIV, 0 );
    PER_SB( regs, regs->psw.IA );
}

/*-------------------------------------------------------------------*/
/*                   SuccessfulRelativeBranch                        */
/*-------------------------------------------------------------------*/
void ARCH_DEP( SuccessfulRelativeBranch )( REGS* regs, S64 offset )
{
    /* Set BEAR to branch instruction. Note: for branch instructions
       regs->ip is not updated to point to the next instruction and
       thus is still pointing to the branch instruction itself.
    */
    SET_BEAR_REG( regs, regs->ip );

    /* Branch target still within same page as branch instruction? */
    if (1
        && !regs->permode
        && !regs->execflag
        &&  offset > -4096
        &&  offset < +4096
        && (regs->ip + offset) >= regs->aip
        && (regs->ip + offset) <  regs->aie
    )
    {
        /* Branch directly to the new instruction */
        regs->ip = regs->ip + offset;
        PTT_INF( "rbranch <", regs->ip, offset, regs->aip );
        return;
    }

    /* Branch target is in another page: point the PSW to the target
       instruction and force a new "regs->ip" value to get set by
       forcing a full instruction fetch from the new target address.
    */
    PTT_INF( "rbranch >", regs->psw.IA, offset, regs->execflag );

    /* Point PSW to target instruction */
    if (!regs->execflag)
        regs->psw.IA = PSW_IA_FROM_IP( regs, offset );
    else
    {
        regs->psw.IA = regs->ET + offset;
        regs->psw.IA &= ADDRESS_MAXWRAP( regs );
    }
    regs->aie = INVALID_AIE;            /* Force a fresh 'instfetch' */

    PTT_INF( "rbranch >", regs->psw.IA, offset, regs->execflag );
    PER_SB( regs, regs->psw.IA );
}

/*-------------------------------------------------------------------*/
/*                   trace_program_interrupt                         */
/*-------------------------------------------------------------------*/
DLL_EXPORT void ARCH_DEP( trace_program_interrupt )( REGS* regs, int pcode, int ilc )
{
    char sie_mode_str    [ 10]  = {0};  // maybe "SIE: "
    char sie_debug_arch  [ 32]  = {0};  // "370", "390" or "900" if defined( SIE_DEBUG )
    char txf_why         [256]  = {0};  // TXF "why" string if txf pgmint
    char dxcstr          [  8]  = {0};  // data exception code if PGM_DATA_EXCEPTION

    BYTE* ip;      /* ptr to instr that program checked or NULL  */

    /* Just the low byte of program interrupt code itself please */
    int code = (pcode & 0xFF);

    /* If the program interrupt code is zero, or if it's not, if the
       code isn't one they're interested in seeing (as determiend by
       OSTAILOR/pgmtrace) AND instruction tracing is NOT active, then
       there's nothing for us to do.
    */
    if (0
        || !code
        || (1
            && !(sysblk.pgminttr & ((U64) 1 << ((code - 1) & 0x3F)))
            && !CPU_STEPPING_OR_TRACING( regs, ilc )
           )
    )
    {
        return;     // (nothing to do; quick exit)
    }

    /*
       First things first: backup the 'ip' by the 'ilc' value to point
       to the actual instruction that actually program checked.

       If 'instinvalid' is set it means an instruction fetch error
       occurred so we shouldn't rely on the value of regs->ip.

       Otherwise if the instruction that program checked (i.e. after
       backing up 'ip' by 'ilc' amount) appears to be in the previous
       mainstor page (meaning the instruction itself crossed a page
       boundary), use the the copy of the instruction that was saved
       in regs->inst for our instruction pointer instead.

       If neither condition is true (the most common case) then we
       simply use current regs->ip value backed up by the ilc amount.
    */
    PTT_PGM( "tr PGM int", regs->ip, regs->aip, ilc );
    ip =
    (
        /* Instruction fetch error? (least likely) */
        regs->instinvalid ? NULL

        /* Instruction crossed page boundary? (unlikely) */
        : ((regs->ip - ilc) < regs->aip) ? regs->inst

        /* Instruction still on same page (most likely) */
        :  (regs->ip - ilc)
    );
    PTT_PGM( "tr PGM int", ip, regs->aip, ilc );

#if defined( OPTION_FOOTPRINT_BUFFER )
    if (!(sysblk.insttrace || sysblk.instbreak))
    {
        U32  n;
        for (n = sysblk.footprptr[ regs->cpuad ] + 1;
            n != sysblk.footprptr[ regs->cpuad ];
            n++, n &= OPTION_FOOTPRINT_BUFFER - 1
        )
        {
            ARCH_DEP( display_inst )(
                &sysblk.footprregs[ regs->cpuad ][n],
                 sysblk.footprregs[ regs->cpuad ][n].inst );
        }
    }
#endif

    /* Suppress LRA Special Operation Exception tracing if requested */
    if (1
        && code == PGM_SPECIAL_OPERATION_EXCEPTION
        && ip && ip[0] == 0xB1    // LRA
        && sysblk.nolrasoe        // suppression enabled
    )
    {
        return;     // (nothing to do; quick exit)
    }

    /* Trace this program interrupt... */

#if defined( _FEATURE_SIE )
    if (SIE_MODE( regs ))
      STRLCPY( sie_mode_str, "SIE: " );
#endif

#if defined( SIE_DEBUG )
    STRLCPY( sie_debug_arch, QSTR( _GEN_ARCH ));
    STRLCAT( sie_debug_arch, " " );
#endif

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
    if (1
        && pcode & PGM_TXF_EVENT
        && regs->txf_why
    )
        txf_why_str( txf_why, sizeof( txf_why ), regs->txf_why );
#endif

    if (code == PGM_DATA_EXCEPTION)
       MSGBUF( dxcstr, " DXC=%2.2X", regs->dxc );

    if (regs->insttrace && sysblk.traceFILE)
        tf_0801( regs, pcode, ilc );

    // "Processor %s%02X: %s%s %s interruption code %4.4X ilc %d%s%s"
    WRMSG( HHC00801, "I",
        PTYPSTR( regs->cpuad ), regs->cpuad,
        sie_mode_str, sie_debug_arch,
        PIC2Name( code ), pcode,
        ilc, dxcstr, txf_why );

    // HHC02324 "PSW=... INST=...  OPCODE  operands   name"
    ARCH_DEP( display_pgmint_inst )( regs, ip );

} /* end function ARCH_DEP( trace_program_interrupt ) */

/*-------------------------------------------------------------------*/
/*                  fix_program_interrupt_PSW                        */
/*-------------------------------------------------------------------*/
DLL_EXPORT int ARCH_DEP( fix_program_interrupt_PSW )( REGS* regs )
{
    /* Determine the instruction length code (ilc).

       The 'zeroilc' flag is set when the Instruction Length Code
       should be reported as zero (such as when instruction-fetching
       nullification PER option is set in CR9 or when the load PSW
       instruction results in an invalid PSW being loaded).

       The PSW 'ilc' value can also be specifically set to '0' when
       a BALR, BASR or BASSM program checks during 'trace_br' call.
    */
    int ilc =
    (
        /* If zeroilc flag is set, then we MUST use zero for the ilc */
        regs->psw.zeroilc ? 0

        /* Otherwise use either the ilc value set in the PSW or the
           length of the EX/EXRL instruction is the instruction is
           being executed.
        */
        : REAL_ILC( regs )
    );
    PTT_PGM( "fxpiPSW ilc", 0, 0, ilc );

    /* If our resulting ilc is still 0 but the zeroilc flag is NOT set,
       then we're left with no choice but to GUESS the 'ilc' value
       based on whether the instruction was being executed or not.
    */
    if (regs->psw.ilc == 0 && !regs->psw.zeroilc)
    {
        ilc = (!regs->execflag ? 2 : (regs->exrl ? 6 : 4));

        PTT_PGM( "fxpiPSW ilc", regs->ip, regs->psw.IA, ilc );

        /* Now ADVANCE the 'ip' mainstor instruction pointer and
           psw 'IA' instruction address by that ilc amount so that
           the 'trace_program_interrupt' can then back them both up
           by the same amount to point to the actual instruction
           that actually program checked.
        */
        regs->psw.ilc  = ilc;  // (guessed value)
        regs->ip      += ilc;  // (so trace_program_interrupt can undo)
        regs->psw.IA  += ilc;  // (so trace_program_interrupt can undo)

        PTT_PGM( "fxpiPSW ilc", regs->ip, regs->psw.IA, ilc );
    }

    /* Return ilc value to be passed to 'trace_program_interrupt' */

    PTT_PGM( "fxpiPSW ret", 0, 0, ilc );
    return ilc;
}

/*-------------------------------------------------------------------*/
/*                       program_interrupt                           */
/*-------------------------------------------------------------------*/
/* The purpose of this function is to store the current PSW into the */
/* Program interrupt old PSW field in low core followed by loading   */
/* the Program interrupt new PSW and then jumping via regs->progjmp  */
/* back to the run_cpu instruction execution loop to begin executing */
/* instructions at the Progran new PSW location.                     */
/*-------------------------------------------------------------------*/
void (ATTR_REGPARM(2) ARCH_DEP( program_interrupt ))( REGS* regs, int pcode )
{
PSA    *psa;                            /* -> Prefixed storage area  */
REGS   *realregs;                       /* True regs structure       */
RADR    px;                             /* host real address of pfx  */
int     code;                           /* pcode without PER ind.    */
int     ilc;                            /* instruction length        */

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
                                        /* FIXME : SEE ISW20090110-1 */
void   *zmoncode = NULL;                /* mon call SIE intercept;
                                           special reloc for z/Arch  */
                                        /* FIXME : zmoncode not being
                                           initialized here raises a
                                           potentially non-initialized
                                           warning in GCC.. can't find
                                           why. ISW 2009/02/04       */
#endif
#if defined( FEATURE_SIE )
int     sie_ilc=0;                      /* SIE instruction length    */
#endif
#if defined( _FEATURE_SIE )
bool    intercept;                      /* False for virtual pgmint  */
                                        /* (True for host interrupt?)*/
#endif

    /* If called with ghost registers (ie from hercules command
       then ignore all interrupt handling and report the error
       to the caller */
    if (regs->ghostregs)
        longjmp( regs->progjmp, pcode );

    PTT_PGM("PGM", pcode, regs->TEA, regs->psw.IA );

    /* program_interrupt() may be called with a shadow copy of the
       regs structure, realregs is the pointer to the real structure
       which must be used when loading/storing the psw, or backing up
       the instruction address in case of nullification
    */
#if defined( _FEATURE_SIE )
        realregs = SIE_MODE( regs )
                 ? GUEST( sysblk.regs[ regs->cpuad ])
                 : HOST(  sysblk.regs[ regs->cpuad ]);
#else
    realregs = HOST( sysblk.regs[ regs->cpuad ]);
#endif

    PTT_PGM( "PGM (r)h,g,a", realregs->host, realregs->guest, realregs->sie_active );

    /* Prevent machine check when in (almost) interrupt loop */
    realregs->instcount++;
    UPDATE_SYSBLK_INSTCOUNT( 1 );

    /* Release any locks */
    if (IS_INTLOCK_HELD( realregs ))
        RELEASE_INTLOCK( realregs );

    /* Unlock the main storage lock if held */
    if (sysblk.mainowner == realregs->cpuad)
        RELEASE_MAINLOCK_UNCONDITIONAL( realregs );

    /* Ensure psw.IA is set and aia invalidated */
    INVALIDATE_AIA(realregs);

#if defined( FEATURE_SIE )
    if (realregs->sie_active)
        ARCH_DEP( invalidate_guest_aia )( GUEST( realregs ));
#endif

    /* Fix PSW and get instruction length (ilc) */
    ilc = ARCH_DEP( fix_program_interrupt_PSW )( realregs );

    PTT_PGM( "PGM ilc", 0, 0, ilc );

#if defined( FEATURE_SIE )
    if (realregs->sie_active)
    {
        sie_ilc = GUEST( realregs )->psw.zeroilc ? 0 : REAL_ILC( GUEST( realregs ));
        if (GUEST( realregs )->psw.ilc == 0 && !GUEST( realregs )->psw.zeroilc)
        {
            sie_ilc = likely( !GUEST( realregs )->execflag) ? 2 : GUEST( realregs )->exrl ? 6 : 4;
            GUEST( realregs )->psw.ilc  = sie_ilc;
        }

        PTT_PGM( "PGM sie_ilc", 0, 0, sie_ilc );
    }
#endif

    /* Trace the program interrupt right away */
    ARCH_DEP( trace_program_interrupt )( regs, pcode, ilc );

    /* Remove PER indication from program interrupt code
       such that interrupt code specific tests may be done.
       The PER indication will be stored in the PER handling
       code */
    code = pcode & ~PGM_PER_EVENT;

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )

    /* If transaction is active, check if interrupt can be filtered */
    if (realregs->txf_tnd)
    {
        /* Indicate TXF related program interrupt */
        pcode |= PGM_TXF_EVENT;

        /* Always reset the NTSTG indicator on any program interrupt */
        regs->txf_NTSTG = false;

        /* Save the program interrupt and data exception codes */
        realregs->txf_piid   = pcode;
        realregs->txf_piid  |= (ilc << 16);

        realregs->txf_dxc_vxc =
        (0
            || pcode == PGM_DATA_EXCEPTION
            || pcode == PGM_VECTOR_PROCESSING_EXCEPTION
        )
        ?  realregs->dxc : 0;

        PTT_TXF( "TXF PIID", realregs->txf_piid, realregs->txf_dxc_vxc, 0 );

        /* 'txf_do_pi_filtering' does not return for filterable
            program interrupts. It returns only if the program
            interrupt is unfilterable, and when it returns, the
            active transaction has already been aborted.
        */
        PTT_TXF( "TXF PROG?", (code & 0xFF), realregs->txf_contran, realregs->txf_tnd );
        ARCH_DEP( txf_do_pi_filtering )( realregs, pcode );

        if (realregs->txf_tnd ) // (sanity check)
            CRASH();            // (sanity check)

        PTT_TXF( "*TXF UPROG!", (code & 0xFF), 0, 0 );

        /* Set flag for sie_exit */
        realregs->txf_UPGM_abort = true;
    }
    else /* (no transaction is CURRENTLY active) */
    {
        /* While no transaction is CURRENTLY active, it's possible
           that a previously active transaction was aborted BEFORE
           we were called, so we need to check for that.
        */
        if ((code & 0xFF) == PGM_TRANSACTION_CONSTRAINT_EXCEPTION)
        {
            /* Indicate TXF related program interrupt */
            pcode |= PGM_TXF_EVENT;

            PTT_TXF( "*TXF 218!", pcode, 0, 0 );

            /* Set flag for sie_exit */
            realregs->txf_UPGM_abort = true;
        }
    }
#endif /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

    /* Set 'execflag' to 0 in case EXecuted instruction program-checked */

    PTT_PGM( "PGM execflag", realregs->execflag, realregs->sie_active, 0 );
    realregs->execflag = 0;

#if defined( FEATURE_SIE )
    if (realregs->sie_active)
        GUEST( realregs )->execflag = 0;
#endif
    PTT_PGM( "PGM execflag", realregs->execflag, realregs->sie_active, 0 );

    /* If this is a concurrent PER event
       then we must add the PER bit to the interrupts code */
    if (OPEN_IC_PER (realregs ))
        pcode |= PGM_PER_EVENT;

    /* Perform serialization and checkpoint synchronization */
    PERFORM_SERIALIZATION( realregs );
    PERFORM_CHKPT_SYNC( realregs );

#if defined( FEATURE_SIE )
    /* Host protection and addressing exceptions
       must be reflected to the guest */
    if (1
        && realregs->sie_active
        && (0
            || code == PGM_PROTECTION_EXCEPTION
            || code == PGM_ADDRESSING_EXCEPTION

#if defined( _FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
            || code == PGM_ALET_SPECIFICATION_EXCEPTION
            || code == PGM_ALEN_TRANSLATION_EXCEPTION
            || code == PGM_ALE_SEQUENCE_EXCEPTION
            || code == PGM_EXTENDED_AUTHORITY_EXCEPTION
#endif
           )
    )
    {
        /* Pass this interrupt to the guest */
#if defined( SIE_DEBUG )
        LOGMSG( "program_int() passing to guest code=%4.4X\n", pcode );
#endif
        switch (GUEST( realregs )->arch_mode)
        {
        case ARCH_370_IDX: GUEST( realregs )->TEA_370 = realregs->TEA; break;
        case ARCH_390_IDX: GUEST( realregs )->TEA_390 = realregs->TEA; break;
        case ARCH_900_IDX: GUEST( realregs )->TEA_900 = realregs->TEA; break;
        default: CRASH();
        }

        GUEST( realregs )->excarid = realregs->excarid;
        GUEST( realregs )->opndrid = realregs->opndrid;

#if defined(_FEATURE_PROTECTION_INTERCEPTION_CONTROL)
        GUEST( realregs )->hostint = 1;
#endif
        GUEST( realregs )->program_interrupt( GUEST( realregs ), pcode );
    }
#endif /* defined( FEATURE_SIE ) */

    /* Back up the PSW for exceptions which cause nullification,
       unless the exception occurred during instruction fetch
    */
    PTT_PGM( "PGM psw.IA", realregs->psw.IA, realregs->instinvalid, ilc );
    if (1
        && !realregs->instinvalid
        && (0
            || code == PGM_PAGE_TRANSLATION_EXCEPTION
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
            || code == PGM_LFX_TRANSLATION_EXCEPTION
            || code == PGM_LSX_TRANSLATION_EXCEPTION
            || code == PGM_LSTE_SEQUENCE_EXCEPTION
            || code == PGM_EX_TRANSLATION_EXCEPTION
            || code == PGM_PRIMARY_AUTHORITY_EXCEPTION
            || code == PGM_SECONDARY_AUTHORITY_EXCEPTION
            || code == PGM_ALEN_TRANSLATION_EXCEPTION
            || code == PGM_ALE_SEQUENCE_EXCEPTION
            || code == PGM_ASTE_VALIDITY_EXCEPTION
            || code == PGM_ASTE_SEQUENCE_EXCEPTION
            || code == PGM_ASTE_INSTANCE_EXCEPTION
            || code == PGM_EXTENDED_AUTHORITY_EXCEPTION
            || code == PGM_STACK_FULL_EXCEPTION
            || code == PGM_STACK_EMPTY_EXCEPTION
            || code == PGM_STACK_SPECIFICATION_EXCEPTION
            || code == PGM_STACK_TYPE_EXCEPTION
            || code == PGM_STACK_OPERATION_EXCEPTION
            || code == PGM_VECTOR_OPERATION_EXCEPTION
           )
    )
    {
        realregs->psw.IA -= ilc;
        realregs->psw.IA &= ADDRESS_MAXWRAP(realregs);

        PTT_PGM( "PGM IA-ilc", realregs->psw.IA, realregs->instinvalid, ilc );

#if defined( FEATURE_SIE )
        /* When in SIE mode the guest instruction
           causing this host exception must also be nullified
        */
        if (realregs->sie_active && !GUEST( realregs )->instinvalid)
            ARCH_DEP( update_guest_psw_ia )( GUEST( realregs ), -sie_ilc );
#endif
    }

    /* The OLD PSW must be incremented
       on the following exceptions during instfetch
    */
    if (1
        && realregs->instinvalid
        && (0
            || code == PGM_PROTECTION_EXCEPTION
            || code == PGM_ADDRESSING_EXCEPTION
            || code == PGM_SPECIFICATION_EXCEPTION
            || code == PGM_TRANSLATION_SPECIFICATION_EXCEPTION
           )
    )
    {
        realregs->psw.IA += ilc;
        realregs->psw.IA &= ADDRESS_MAXWRAP( realregs );

        PTT_PGM( "PGM IA+ilc", realregs->psw.IA, realregs->instinvalid, ilc );
    }

    /* Store the interrupt code in the PSW */
    realregs->psw.intcode = pcode;

    /* Call debugger if active */
    HDC2( debug_program_interrupt, regs, pcode );

    realregs->instinvalid = 0;
    PTT_PGM( "PGM inval=0", 0, 0, 0 );

#if defined( FEATURE_SIE )

    /*---------------------------------------------------------*/
    /* If this is a host exception in SIE state then leave SIE */
    /*---------------------------------------------------------*/
    if (realregs->sie_active)
    {
        PTT_PGM( "PGM >sie_exit", SIE_HOST_PGM_INT, 0, 0 );
        ARCH_DEP( sie_exit )( realregs, SIE_HOST_PGM_INT );
    }
#endif

    /* Absolute address of prefix page */
    px = realregs->PX;

    /* If under SIE use translated to host absolute prefix */
#if defined( _FEATURE_SIE )
    if (SIE_MODE( regs ))
        px = regs->sie_px;
#endif

#if defined( _FEATURE_SIE )
    /*---------------------------------------------------------------*/
    /*   If we're in SIE mode, then we need to determine whether     */
    /*   we must, or must not, intercept this program interrupt,     */
    /*   and by intercept, we mean pass it on to the SIE host so     */
    /*   that it (not the guest!) can decide what action to take.    */
    /*---------------------------------------------------------------*/
    if (0

        /* If not in SIE mode, then we (duh!) must not intercept it  */
        || !SIE_MODE( regs )

        /* Interception is mandatory for the following exceptions,
           so if ANY of the below conditions are false, then we MUST
           intercept this program interrupt.

           The below tests/checks are for "not this condition" where
           the "condition" is the condition which we MUST intercept.
           Thus if any of them fail, then we MUST intercept.

           Only if ALL of them are true (only if no condition exists
           that REQUIRES an intercept) should we then NOT intercept.
        */
        || (1

            && code != PGM_ADDRESSING_EXCEPTION
            && code != PGM_SPECIFICATION_EXCEPTION
            && code != PGM_SPECIAL_OPERATION_EXCEPTION

#if defined( FEATURE_S370_S390_VECTOR_FACILITY )
            && code != PGM_VECTOR_OPERATION_EXCEPTION
#endif
#ifdef FEATURE_BASIC_FP_EXTENSIONS
            && !(code == PGM_DATA_EXCEPTION && (regs->dxc == 1 || regs->dxc == 2) && (regs->CR(0) & CR0_AFP) && !(HOSTREGS->CR(0) & CR0_AFP))
#endif
            && !SIE_FEAT_BIT_ON( regs, IC0, PGMALL )

#if !defined( _FEATURE_PROTECTION_INTERCEPTION_CONTROL )
            && code != PGM_PROTECTION_EXCEPTION
#else
            && !(code == PGM_PROTECTION_EXCEPTION           && (!SIE_FEAT_BIT_ON( regs, EC2, PROTEX ) || realregs->hostint ))
#endif
#if defined( _FEATURE_PER2 )
            && !((pcode & PGM_PER_EVENT)                    && SIE_FEAT_BIT_ON( regs, M, GPE ))
#endif
#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
            && !(code == PGM_ALEN_TRANSLATION_EXCEPTION     && SIE_FEAT_BIT_ON( regs, MX, XC ))
            && !(code == PGM_ALE_SEQUENCE_EXCEPTION         && SIE_FEAT_BIT_ON( regs, MX, XC ))
            && !(code == PGM_EXTENDED_AUTHORITY_EXCEPTION   && SIE_FEAT_BIT_ON( regs, MX, XC ))
#endif
            && !(code == PGM_OPERATION_EXCEPTION            && SIE_FEAT_BIT_ON( regs, IC0, OPEREX ))
            && !(code == PGM_PRIVILEGED_OPERATION_EXCEPTION && SIE_FEAT_BIT_ON( regs, IC0, PRIVOP ))
           )
    )
    {
#endif /*defined(_FEATURE_SIE)*/

        intercept = false;
        PTT_PGM( "PGM !icept", intercept, 0, 0 );

        /* Set the main storage reference and change bits */
        ARCH_DEP( or_storage_key )( px, (STORKEY_REF | STORKEY_CHANGE) );

        /* Point to PSA in main storage */
        psa = (void*)(regs->mainstor + px);

#if defined( _FEATURE_SIE )

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /** FIXME : SEE ISW20090110-1 */
        if (code == PGM_MONITOR_EVENT)
        {
            zmoncode = psa->moncode;
        }
#endif
    }
    else /* The SIE host must deal with this program interrupt */
    {
        intercept = true;
        PTT_PGM( "PGM icept", intercept, 0, 0 );

        /* This is a guest interruption interception so point to
           the interruption parm area in the state descriptor
           rather then the PSA (except for Operation Exception)
        */
        if (code != PGM_OPERATION_EXCEPTION)
        {
            psa = (void*)(HOSTREGS->mainstor + SIE_STATE(regs) + SIE_IP_PSA_OFFSET);

            /* Set the main storage reference and change bits */
            ARCH_DEP( or_storage_key )( SIE_STATE( regs ), (STORKEY_REF | STORKEY_CHANGE) );

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
            /** FIXME : SEE ISW20090110-1 */
            if (code == PGM_MONITOR_EVENT)
            {
                PSA* _psa;
                _psa = (void *)(HOSTREGS->mainstor + SIE_STATE( regs ) + SIE_II_PSA_OFFSET);
                zmoncode = _psa->ioid;
            }
#endif
        }
        else
        {
            /* Point to PSA in main storage */
            psa = (void*)(regs->mainstor + px);

            /* Set the main storage reference and change bits */
            ARCH_DEP( or_storage_key )( px, (STORKEY_REF | STORKEY_CHANGE) );
        }
    }
#endif /*defined(_FEATURE_SIE)*/

#if defined( _FEATURE_PER )
    /* Handle PER or concurrent PER event */

    /* Throw out Stor Alter PER if merged with nullified/suppressed rupt */
    if (1
        &&  IS_IC_PER_SA(    realregs )
        && !IS_IC_PER_STURA( realregs )
        && (realregs->ip[0] != 0x0E)
        && !(0
             || code == 0x00
             || code == PGM_SPECIFICATION_EXCEPTION
             || code == PGM_FIXED_POINT_OVERFLOW_EXCEPTION
             || code == PGM_DECIMAL_OVERFLOW_EXCEPTION
             || code == PGM_EXPONENT_OVERFLOW_EXCEPTION
             || code == PGM_EXPONENT_UNDERFLOW_EXCEPTION
             || code == PGM_SIGNIFICANCE_EXCEPTION
             || code == PGM_SPACE_SWITCH_EVENT
             || code == PGM_MONITOR_EVENT
           )
    )
    {
        OFF_IC_PER_SA( realregs );
    }

    if (OPEN_IC_PER( realregs ))
    {
        if (CPU_STEPPING_OR_TRACING( realregs, ilc ))
        {
            BYTE perc = IS_IC_PER( realregs ) >> 16;

            if (regs->insttrace && sysblk.traceFILE)
                tf_0802( regs,
                        (realregs->psw.IA - ilc) & ADDRESS_MAXWRAP( realregs ),
                         pcode, perc );
            else
            {
                char percname[32];
                perc2name( perc, percname, sizeof( percname ));

                // "Processor %s%02X: PER event: code %4.4X perc %2.2X=%s addr "F_VADR
                WRMSG( HHC00802, "I", PTYPSTR( regs->cpuad ), regs->cpuad,
                    pcode, perc, percname,
                    (realregs->psw.IA - ilc) & ADDRESS_MAXWRAP( realregs ));
            }
        }

        realregs->perc |= OPEN_IC_PER( realregs ) >> ((32 - IC_CR9_SHIFT) - 16);

        /* Positions 14 and 15 contain zeros
           if a storage alteration event was not indicated
        */
        if (0
            || !OPEN_IC_PER_SA(    realregs )
            ||  OPEN_IC_PER_STURA( realregs )
        )
            realregs->perc &= 0xFFFC;

        STORE_HW( psa->perint, realregs->perc   );
        STORE_W(  psa->peradr, realregs->peradr );

        if (IS_IC_PER_SA( realregs ) && ACCESS_REGISTER_MODE( &realregs->psw ))
            psa->perarid = realregs->peraid;

#if defined( _FEATURE_SIE )
        /* Reset PER pending indication */
        if (!intercept)
            OFF_IC_PER( realregs );
#endif
    }
    else
    {
        pcode &= ~(PGM_PER_EVENT);
    }
#endif /* defined( _FEATURE_PER ) */

#if defined( FEATURE_BCMODE )
    /* For ECMODE, store extended interrupt information in PSA */
    if (ECMODE( &realregs->psw ))
#endif
    {
        PTT_PGM( "PGM ec", 0, 0, 0 );

        /* Store the program interrupt code at PSA+X'8C' */
        psa->pgmint[0] = 0;
        psa->pgmint[1] = ilc;

        STORE_HW( psa->pgmint + 2, pcode );

        /* Store the exception access identification at PSA+160 */
        if (0
            || code == PGM_PAGE_TRANSLATION_EXCEPTION
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
            || code == PGM_ASTE_INSTANCE_EXCEPTION
            || code == PGM_EXTENDED_AUTHORITY_EXCEPTION

#if defined( FEATURE_SUPPRESSION_ON_PROTECTION )
            || code == PGM_PROTECTION_EXCEPTION
#endif
        )
        {
            psa->excarid = regs->excarid;
            // FIXME: this conditional will ALWAYS be true!!
            if (regs->TEA | TEA_MVPG)
                psa->opndrid = regs->opndrid;
            realregs->opndrid = 0;
        }

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /* Store the translation exception address at PSA+168 */
        if (0
            || code == PGM_PAGE_TRANSLATION_EXCEPTION
            || code == PGM_SEGMENT_TRANSLATION_EXCEPTION
            || code == PGM_ASCE_TYPE_EXCEPTION
            || code == PGM_REGION_FIRST_TRANSLATION_EXCEPTION
            || code == PGM_REGION_SECOND_TRANSLATION_EXCEPTION
            || code == PGM_REGION_THIRD_TRANSLATION_EXCEPTION

#if defined( FEATURE_SUPPRESSION_ON_PROTECTION )
            || code == PGM_PROTECTION_EXCEPTION
#endif
        )
        {
            STORE_DW( psa->TEA_G, regs->TEA );
        }

        /* For z, store translation exception address at PSA+172 */
        if (0
            || code == PGM_AFX_TRANSLATION_EXCEPTION
            || code == PGM_ASX_TRANSLATION_EXCEPTION
            || code == PGM_PRIMARY_AUTHORITY_EXCEPTION
            || code == PGM_SECONDARY_AUTHORITY_EXCEPTION
            || code == PGM_SPACE_SWITCH_EVENT
            || code == PGM_LX_TRANSLATION_EXCEPTION
            || code == PGM_LFX_TRANSLATION_EXCEPTION
            || code == PGM_LSX_TRANSLATION_EXCEPTION
            || code == PGM_LSTE_SEQUENCE_EXCEPTION
            || code == PGM_EX_TRANSLATION_EXCEPTION
        )
        {
            STORE_FW( psa->TEA_L, regs->TEA );
        }

#else /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

        /* For 370/390, store translation exception address at PSA+144 */
        if (0
            || code == PGM_PAGE_TRANSLATION_EXCEPTION
            || code == PGM_SEGMENT_TRANSLATION_EXCEPTION
            || code == PGM_AFX_TRANSLATION_EXCEPTION
            || code == PGM_ASX_TRANSLATION_EXCEPTION
            || code == PGM_PRIMARY_AUTHORITY_EXCEPTION
            || code == PGM_SECONDARY_AUTHORITY_EXCEPTION
            || code == PGM_SPACE_SWITCH_EVENT
            || code == PGM_LX_TRANSLATION_EXCEPTION
            || code == PGM_EX_TRANSLATION_EXCEPTION

#if defined( FEATURE_SUPPRESSION_ON_PROTECTION )
            || code == PGM_PROTECTION_EXCEPTION
#endif
        )
        {
            STORE_FW( psa->tea, regs->TEA );
        }
#endif /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

        realregs->TEA = 0;

        /* Store Data exception code in PSA */
        if (code == PGM_DATA_EXCEPTION)
        {
            STORE_FW( psa->DXC, regs->dxc );

#if defined( FEATURE_BASIC_FP_EXTENSIONS )
            /* Load data exception code into FPC register byte 2 */
            if (regs->CR(0) & CR0_AFP)
            {
                regs->fpc &= ~(FPC_DXC);
                regs->fpc |= ((regs->dxc << 8)) & FPC_DXC;
            }
#endif
        }

        /* Store the monitor class and event code */
        if (code == PGM_MONITOR_EVENT)
        {
            STORE_HW( psa->monclass, regs->monclass );

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
            STORE_DW( zmoncode, regs->MONCODE );
#else
            STORE_W( psa->moncode, regs->MONCODE );
#endif
        }

#if defined( FEATURE_PER3 )
        /* Store the breaking event address register in the PSA */
        STORE_W( psa->bea, regs->bear );
        PTT_PGM( "PGM bear", regs->bear, 0, 0 );
#endif

    } /* end if(ECMODE) */

#if defined( _FEATURE_PROTECTION_INTERCEPTION_CONTROL )
    realregs->hostint = 0;
#endif

    /* Normal (non-intercepted) program interrupt? */
#if defined( _FEATURE_SIE )
    if (!intercept)
#endif
    {
        PSW pgmold, pgmnew;
        int pgmintloop = 0;
        int detect_pgmintloop = FACILITY_ENABLED( HERC_DETECT_PGMINTLOOP, realregs );

        /* Store current PSW at PSA+X'28' or PSA+X'150' for ESAME */
        ARCH_DEP( store_psw )( realregs, psa->pgmold );

        /* Save program old psw */
        if (detect_pgmintloop)
        {
            memcpy( &pgmold, &realregs->psw, sizeof( PSW ));
            pgmold.cc      = 0;
            pgmold.intcode = 0;
            pgmold.ilc     = 0;
        }

        /* Load new PSW from PSA+X'68' or PSA+X'1D0' for ESAME */
        if ((code = ARCH_DEP( load_psw )( realregs, psa->pgmnew )))
        {
            /* The load psw failed */
#if defined( _FEATURE_SIE )
            if (SIE_MODE( realregs ))
            {
                PTT_PGM( "*PGM *lpsw", code, 0, 0 );
                PTT_PGM( "PGM progjmp", pcode, 0, 0 );
                longjmp( realregs->progjmp, pcode );
            }
            else
#endif
            {
                /* Invalid pgmnew: ==> program interrupt loop */
                pgmintloop = detect_pgmintloop;
            }
        }
        else if (detect_pgmintloop)
        {
            /* Save program new psw */
            memcpy( &pgmnew, &realregs->psw, sizeof( PSW ));
            pgmnew.cc      = 0;
            pgmnew.intcode = 0;
            pgmnew.ilc     = 0;

            /* Adjust pgmold instruction address */
            pgmold.ia.D -= ilc;

            /* Check for program interrupt loop (old==new) */
            if (memcmp( &pgmold, &pgmnew, sizeof( PSW )) == 0)
                pgmintloop = 1;
        }

        if (pgmintloop)
        {
            char buf[64];
            STR_PSW( realregs, buf );

            if (regs->insttrace && sysblk.traceFILE)
                tf_0803( realregs, buf );

            // "Processor %s%02X: program interrupt loop PSW %s"
            WRMSG( HHC00803, "I", PTYPSTR( realregs->cpuad ),
                realregs->cpuad, buf );

            OBTAIN_INTLOCK( realregs );
            {
                realregs->cpustate = CPUSTATE_STOPPING;
                ON_IC_INTERRUPT( realregs );
            }
            RELEASE_INTLOCK( realregs );
        }

        /*-----------------------------------------------------------*/
        /*  Normal non-intercepted program interrupt: return to      */
        /*  either the run_cpu or run_sie loop and start executing   */
        /*  instructions again, but at Program New PSW instead.      */
        /*-----------------------------------------------------------*/

        PTT_PGM( "PGM !icept", intercept, 0, 0 );
        PTT_PGM( "PGM progjmp", SIE_NO_INTERCEPT, 0, 0 );
        longjmp( realregs->progjmp, SIE_NO_INTERCEPT );
    }

#if defined( _FEATURE_SIE )
    /*---------------------------------------------------------------*/
    /*  We're in SIE mode and SIE host MUST intercept this program   */
    /*  interrupt. Jump back to the run_sie loop with the interrupt  */
    /*  code so it can break out of its instruction execution loop   */
    /*  and exit from run_sie back to sie_exit so the interrupt can  */
    /*  be passed on to the SIE host for handling.                   */
    /*---------------------------------------------------------------*/

    PTT_PGM( "PGM icept", intercept, 0, 0 );
    PTT_PGM( "PGM progjmp", pcode, 0, 0 );
    longjmp( realregs->progjmp, pcode );
#endif

} /* end function ARCH_DEP( program_interrupt ) */

/*-------------------------------------------------------------------*/
/* Load restart new PSW                                              */
/*-------------------------------------------------------------------*/
static void ARCH_DEP(restart_interrupt) (REGS *regs)
{
int     rc;                             /* Return code               */
PSA    *psa;                            /* -> Prefixed storage area  */

    PTT_INF("*RESTART",regs->cpuad,regs->cpustate,regs->psw.IA_L);

    /* Set the main storage reference and change bits */
    ARCH_DEP( or_storage_key )( regs->PX, (STORKEY_REF | STORKEY_CHANGE) );

    /* Zeroize the interrupt code in the PSW */
    regs->psw.intcode = 0;

    /* Point to PSA in main storage */
    psa = (PSA*)(regs->mainstor + regs->PX);

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
    /* Abort any active transaction and then return back to here
       to continue with restart interrupt processing */
    if (regs->txf_tnd)
    {
        PTT_TXF( "*TXF MISC", 0, regs->txf_contran, regs->txf_tnd );
        regs->txf_why |= TXF_WHY_RESTART_INT;
        ABORT_TRANS( regs, ABORT_RETRY_RETURN, TAC_MISC );
    }
#endif
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
DEVBLK *dev;                            /* dev presenting interrupt  */

    /* Test and clear pending I/O interrupt */
    icode = ARCH_DEP( present_io_interrupt )( regs, &ioid, &ioparm, &iointid, csw, &dev );

    /* Exit if no interrupt was presented */
    if (icode == 0) return;

    PTT_IO("*IOINT",ioid,ioparm,iointid);

#if defined(_FEATURE_IO_ASSIST)
    if(SIE_MODE(regs) && icode != SIE_NO_INTERCEPT)
    {
        /* Point to SIE copy of PSA in state descriptor */
        psa = (void*)(HOSTREGS->mainstor + SIE_STATE(regs) + SIE_II_PSA_OFFSET);
        ARCH_DEP( or_storage_key )( SIE_STATE( regs ), (STORKEY_REF | STORKEY_CHANGE) );
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
        ARCH_DEP( or_storage_key )( pfx, (STORKEY_REF | STORKEY_CHANGE) );
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
    if (CPU_STEPPING_OR_TRACING( regs, 0 ) || dev->ccwtrace)
    {
        BYTE* csw = psa->csw;

        if (regs->insttrace && sysblk.traceFILE)
            tf_0804( regs, csw, ioid, SSID_TO_LCSS(ioid >> 16) & 0x07 );
        else
        {
            // "Processor %s%02X: I/O interrupt code %1.1X:%4.4X CSW %2.2X...
            WRMSG( HHC00804, "I", PTYPSTR( regs->cpuad ), regs->cpuad,
                    SSID_TO_LCSS( ioid >> 16 ) & 0x07, ioid,
                    csw[0], csw[1], csw[2], csw[3],
                    csw[4], csw[5], csw[6], csw[7] );
        }
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
    if (CPU_STEPPING_OR_TRACING( regs, 0 ) || dev->ccwtrace)
    {
        if (sysblk.traceFILE)
        {
            if (regs->insttrace || dev->ccwtrace)
            {
                // (handles both HHC00805 and HHC00806)
                tf_0806( regs, ioid, ioparm, iointid );
            }
        }
        else // (!sysblk.traceFILE)
        {
#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) && !defined( _FEATURE_IO_ASSIST )
            // "Processor %s%02X: I/O interrupt code %8.8X parm %8.8X"
            WRMSG (HHC00805, "I", PTYPSTR(regs->cpuad), regs->cpuad, ioid, ioparm);
#else
            // "Processor %s%02X: I/O interrupt code %8.8X parm %8.8X id %8.8X"
            WRMSG (HHC00806, "I", PTYPSTR(regs->cpuad), regs->cpuad, ioid, ioparm, iointid);
#endif
        }
    }
#endif /* FEATURE_CHANNEL_SUBSYSTEM */

#if defined( _FEATURE_IO_ASSIST )
    if (icode == SIE_NO_INTERCEPT)
#endif
    {
#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
        /* Abort any active transaction and then return back to here
           to continue with I/O interrupt processing */
        if (regs->txf_tnd)
        {
            PTT_TXF( "*TXF IO", 0, regs->txf_contran, regs->txf_tnd );
            regs->txf_why |= TXF_WHY_IO_INT;
            ABORT_TRANS( regs, ABORT_RETRY_RETURN, TAC_IO );
        }
#endif
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
    ARCH_DEP( or_storage_key )( regs->PX, (STORKEY_REF | STORKEY_CHANGE) );

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
    {
        if (regs->insttrace && sysblk.traceFILE)
            tf_0807( regs, mcic, fsta, xdmg );
        else
            // "Processor %s%02X: machine check code %16.16"PRIu64
            WRMSG (HHC00807, "I", PTYPSTR(regs->cpuad), regs->cpuad, mcic);
    }

    /* Store the external damage code at PSA+244 */
    STORE_FW(psa->xdmgcode, xdmg);

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    /* Store the failing storage address at PSA+248 */
    STORE_DW(psa->mcstorad, fsta);
#else
    /* Store the failing storage address at PSA+248 */
    STORE_FW(psa->mcstorad, fsta);
#endif

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
    /* Abort any active transaction and then return back to here
       to continue with machine check interrupt processing */
    if (regs->txf_tnd)
    {
        PTT_TXF( "*TXF MCK", 0, regs->txf_contran, regs->txf_tnd );
        regs->txf_why |= TXF_WHY_MCK_INT;
        ABORT_TRANS( regs, ABORT_RETRY_RETURN, TAC_MCK );
    }
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
    regs->breakortrace = (sysblk.instbreak || (sysblk.insttrace && regs->insttrace));

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
        if (1
            && OPEN_IC_EXTPENDING( regs )
#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
            /* Don't interrupt active transaction */
            && (0
                || regs->txf_tnd == 0
                || regs->txf_PPA < PPA_MUCH_HELP_THRESHOLD
               )
#endif
        )
        {
            PERFORM_SERIALIZATION( regs );
            PERFORM_CHKPT_SYNC( regs );
            ARCH_DEP( perform_external_interrupt )( regs );
        }

        /* Process I/O interrupt */
        if (IS_IC_IOPENDING)
        {
            if (1
                && OPEN_IC_IOPENDING( regs )
#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
                /* Don't interrupt active transaction */
                && (0
                    || regs->txf_tnd == 0
                    || regs->txf_PPA < PPA_MUCH_HELP_THRESHOLD
                   )
#endif
            )
            {
                PERFORM_SERIALIZATION( regs );
                PERFORM_CHKPT_SYNC( regs );
                ARCH_DEP( perform_io_interrupt )( regs );
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

        /* Signal that the cpu is now stopped */
        signal_condition( &sysblk.cpucond );

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

            if (regs->insttrace && sysblk.traceFILE)
                tf_0808( regs );

            // "Processor %s%02X: store status completed"
            WRMSG( HHC00808, "I", PTYPSTR( regs->cpuad ), regs->cpuad );

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
        S64 saved_timer = get_cpu_timer(regs);
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
        if (IS_IC_DISABLED_WAIT_PSW( regs ))
        {
            /* Don't log the disabled wait when OSTAILOR VM is active
               unless it is the very last CPU in the configuration. */
            if (0
                || !(sysblk.ostailor & OSTAILOR_VM)
                || !(sysblk.started_mask ^ regs->cpubit)
            )
            {
                char buf[128];
                STR_PSW( regs, buf );

                if (regs->insttrace && sysblk.traceFILE)
                    tf_0809( regs, buf );

                // "Processor %s%02X: disabled wait state %s"
                WRMSG( HHC00809, "I", PTYPSTR( regs->cpuad ),
                    regs->cpuad, buf );

                // "Processor %s%02X: processor %sstopped due to disabled wait"
                WRMSG( HHC00826, "W", PTYPSTR( regs->cpuad ),
                    regs->cpuad, "auto-" );
            }
            regs->cpustate = CPUSTATE_STOPPING;
            RELEASE_INTLOCK( regs );
            longjmp( regs->progjmp, SIE_NO_INTERCEPT );
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

} /* process_interrupt */

/*-------------------------------------------------------------------*/
/* Run CPU                                                           */
/*-------------------------------------------------------------------*/
REGS *ARCH_DEP(run_cpu) (int cpu, REGS *oldregs)
{
const INSTR_FUNC   *current_opcode_table;
register REGS   *regs;
BYTE   *ip;
int     i = 0;
int     aswitch;

    /* Assign new regs if not already assigned */
    regs = sysblk.regs[cpu] ?
           sysblk.regs[cpu] :
           malloc_aligned( ROUND_UP( sizeof( REGS ), _4K ), _4K );

    if (oldregs)
    {
        if (oldregs != regs)
        {
            TXF_FREEMAP( oldregs );
            memcpy (regs, oldregs, sizeof(REGS));
            free_aligned(oldregs);
            regs->blkloc = CSWAP64((U64)((uintptr_t)regs));
            HOSTREGS = regs;
            if (GUESTREGS)
                HOST(GUESTREGS) = regs;
            sysblk.regs[cpu] = regs;
            release_lock(&sysblk.cpulock[cpu]);
            if (regs->insttrace && sysblk.traceFILE)
                tf_0811( regs, get_arch_name( regs ));
            // "Processor %s%02X: architecture mode %s"
            WRMSG( HHC00811, "I", PTYPSTR( cpu ), cpu, get_arch_name( regs ));
        }
    }
    else
    {
        memset(regs, 0, sizeof(REGS));

        if (cpu_init( cpu, regs, NULL ))
            return NULL;

        if (regs->insttrace && sysblk.traceFILE)
            tf_0811( regs, get_arch_name( regs ));

        // "Processor %s%02X: architecture mode %s"
        WRMSG( HHC00811, "I", PTYPSTR( cpu ), cpu, get_arch_name( regs ));

#if defined( FEATURE_S370_S390_VECTOR_FACILITY )
        if (regs->vf->online)
        {
            if (regs->insttrace && sysblk.traceFILE)
                tf_0812( regs );

            // "Processor %s%02X: vector facility online"
            WRMSG( HHC00812, "I", PTYPSTR( cpu ), cpu );
        }
#endif
    }

    regs->program_interrupt = &ARCH_DEP(program_interrupt);

    regs->breakortrace = (sysblk.instbreak || (sysblk.insttrace && regs->insttrace));
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
        setCpuIdregs( regs, -1, -1, -1, -1, true );

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

    /* Initialize Transactional-Execution Facility */
    TXF_ALLOCMAP( regs );

    /* Get pointer to primary opcode table */
    current_opcode_table = regs->ARCH_DEP( runtime_opcode_xxxx );

    /* Signal cpu has started */
    if(!aswitch)
        signal_condition (&sysblk.cpucond);

    RELEASE_INTLOCK(regs);

    /* Establish longjmp destination for program check or
       RETURN_INTCHECK, or SIE_INTERCEPT, or longjmp, etc.
    */
    if (setjmp( regs->progjmp ) && sysblk.ipled)
    {
        /* Our instruction execution loop further below didn't finish
           due to a longjmp(progjmp) having been executed bringing us
           to here, thereby causing the instruction counter to not be
           properly updated. Thus, we need to update it here instead.
       */
        regs->instcount   +=     (i * 2);
        UPDATE_SYSBLK_INSTCOUNT( (i * 2) );

        /* Perform automatic instruction tracing if it's enabled */
        DO_AUTOMATIC_TRACING();
    }

    /* Set `execflag' to 0 in case EXecuted instruction did a longjmp() */
    regs->execflag = 0;

    //--------------------------------------------------------------
    //                    PROGRAMMING NOTE
    //--------------------------------------------------------------
    // The first 'fastest_no_txf_loop' loop below is used when the
    // TXF facility is not enabled, and since facilities cannot be
    // enabled or disabled once the guest system has been IPLed and
    // started, it utilizes our original instruction execution loop
    // which uses the 'EXECUTE_INSTRUCTION' and 'UNROLLED_EXECUTE'
    // macros which do not have any TXF related code in them.
    //
    // The second and third loops below (the 'txf_facility_loop' and
    // 'txf_slower_loop') are used when the TXF facility is enabled,
    // requiring us to check whether or not a transaction is active
    // or not after each instruction is executed.
    //
    // If no transaction is active, the normal 'EXECUTE_INSTRUCTION'
    // and 'UNROLLED_EXECUTE' macros can be used, but a check for an
    // active transaction still needs to be performed after each and
    // every instruction (so we can know which loop we need to use).
    //
    // When a transaction is active, we use the third (slowest) loop
    // called 'txf_slower_loop', using the 'TXF_EXECUTE_INSTRUCTION'
    // and 'TXF_UNROLLED_EXECUTE' macros, which contain code that
    // enforces certain Transaction-Exceution Facility constraints.
    //--------------------------------------------------------------

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
    if (FACILITY_ENABLED( 073_TRANSACT_EXEC, regs ))
        if (regs->CR(0) & CR0_TXC)
            goto txf_facility_loop;
#endif

fastest_no_txf_loop:

    if (INTERRUPT_PENDING( regs ))
        ARCH_DEP( process_interrupt )( regs );

enter_fastest_no_txf_loop:

    ip = INSTRUCTION_FETCH( regs, 0 );
    PROCESS_TRACE( regs, ip, enter_fastest_no_txf_loop );
    EXECUTE_INSTRUCTION( current_opcode_table, ip, regs );
    regs->instcount++;
    UPDATE_SYSBLK_INSTCOUNT( 1 );

    for (i=0; i < MAX_CPU_LOOPS/2; i++)
    {
        UNROLLED_EXECUTE( current_opcode_table, regs );
        UNROLLED_EXECUTE( current_opcode_table, regs );
    }
    regs->instcount   +=     (i * 2);
    UPDATE_SYSBLK_INSTCOUNT( (i * 2) );

    /* Perform automatic instruction tracing if it's enabled */
    DO_AUTOMATIC_TRACING();
    goto fastest_no_txf_loop;

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )

txf_facility_loop:

    if (INTERRUPT_PENDING( regs ))
        ARCH_DEP( process_interrupt )( regs );

    if (regs->txf_tnd)
        goto enter_txf_slower_loop;

enter_txf_faster_loop:

    ip = INSTRUCTION_FETCH( regs, 0 );
    PROCESS_TRACE( regs, ip, enter_txf_faster_loop );
    EXECUTE_INSTRUCTION( current_opcode_table, ip, regs );
    regs->instcount++;
    UPDATE_SYSBLK_INSTCOUNT( 1 );

    for (i=0; i < MAX_CPU_LOOPS/2; i++)
    {
        if (regs->txf_tnd)
            break;

        UNROLLED_EXECUTE( current_opcode_table, regs );

        if (regs->txf_tnd)
            break;

        UNROLLED_EXECUTE( current_opcode_table, regs );
    }
    regs->instcount   +=     (i * 2);
    UPDATE_SYSBLK_INSTCOUNT( (i * 2) );

    /* Perform automatic instruction tracing if it's enabled */
    DO_AUTOMATIC_TRACING();

//txf_slower_loop:

    if (INTERRUPT_PENDING( regs ))
        ARCH_DEP( process_interrupt )( regs );

    if (!regs->txf_tnd)
        goto enter_txf_faster_loop;

enter_txf_slower_loop:

    ip = INSTRUCTION_FETCH( regs, 0 );
    PROCESS_TRACE( regs, ip, enter_txf_slower_loop );
    TXF_EXECUTE_INSTRUCTION( current_opcode_table, ip, regs );
    regs->instcount++;
    UPDATE_SYSBLK_INSTCOUNT( 1 );

    for (i=0; i < MAX_CPU_LOOPS/2; i++)
    {
        if (!regs->txf_tnd)
            break;

        TXF_UNROLLED_EXECUTE( current_opcode_table, regs );

        if (!regs->txf_tnd)
            break;

        TXF_UNROLLED_EXECUTE( current_opcode_table, regs );
    }
    regs->instcount   +=     (i * 2);
    UPDATE_SYSBLK_INSTCOUNT( (i * 2) );

    /* Perform automatic instruction tracing if it's enabled */
    DO_AUTOMATIC_TRACING();
    goto txf_facility_loop;

#endif /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

    UNREACHABLE_CODE( return NULL );

} /* end function run_cpu */

/*-------------------------------------------------------------------*/
/* Process Trace                                                     */
/*-------------------------------------------------------------------*/
void ARCH_DEP( process_trace )( REGS* regs, BYTE* dest )
{
    bool shouldtrace = false;           /* true == Trace instruction */

    /* Test for trace */
    shouldtrace = CPU_TRACING( regs, 0 );

    /* Test for step */
    regs->stepping = CPU_STEPPING( regs, 0 );

    if (regs->stepping)
    {
        if (sysblk.breakasid)
        {
            if (regs->CR_LHL(4) != sysblk.breakasid)
                regs->stepping = false;
        }
    }

    /* Display the instruction */
    if (shouldtrace || regs->stepping)
        ARCH_DEP( display_inst )( regs, dest );

    /* Stop the CPU if instruction stepping */
    if (regs->stepping)
    {
        REGS* hostregs = HOSTREGS;
        S64 saved_timer[2] = {0};

        OBTAIN_INTLOCK( hostregs );
        {
            hostregs->waittod = host_tod();

            /* The CPU timer is not decremented for a CPU that is in
               the manual state (e.g. stopped in single step mode) */

            saved_timer[0] = get_cpu_timer( regs     );
            saved_timer[1] = get_cpu_timer( hostregs );

            hostregs->cpustate = CPUSTATE_STOPPED;
            sysblk.started_mask &= ~hostregs->cpubit;
            hostregs->stepwait = 1;
            sysblk.intowner = LOCK_OWNER_NONE;

            while (hostregs->cpustate == CPUSTATE_STOPPED)
            {
                wait_condition( &hostregs->intcond, &sysblk.intlock );
            }

            sysblk.intowner = hostregs->cpuad;
            hostregs->stepwait = 0;
            sysblk.started_mask |= hostregs->cpubit;

            set_cpu_timer( regs,     saved_timer[0] );
            set_cpu_timer( hostregs, saved_timer[1] );

            hostregs->waittime += host_tod() - hostregs->waittod;
            hostregs->waittod = 0;
        }
        RELEASE_INTLOCK( hostregs );
    }
} /* process_trace */

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "cpu.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "cpu.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section: compiled only ONCE after last arch built   */
/*-------------------------------------------------------------------*/
/*  Note: the last architecture has been built so the normal non-    */
/*  underscore FEATURE values are now #defined according to the      */
/*  LAST built architecture just built (usually zarch = 900). This   */
/*  means from this point onward (to the end of file) you should     */
/*  ONLY be testing the underscore _FEATURE values to see if the     */
/*  given feature was defined for *ANY* of the build architectures.  */
/*-------------------------------------------------------------------*/

/*********************************************************************/
/*                  IMPORTANT PROGRAMMING NOTE                       */
/*********************************************************************/
/*                                                                   */
/* It is CRITICALLY IMPORTANT to not use any architecture dependent  */
/* macros anywhere in any of your non-arch_dep functions. This means */
/* you CANNOT use GREG, RADR, VADR, etc. anywhere in your function,  */
/* nor can you call "ARCH_DEP(func)(args)" anywhere in your code!    */
/*                                                                   */
/* Basically you MUST NOT use any architecture dependent macro that  */
/* is #defined in the "feature.h" header.  If you you need to use    */
/* any of them, then your function MUST be an "ARCH_DEP" function    */
/* that is placed within the ARCH_DEP section at the beginning of    */
/* this module where it can be compiled multiple times, once for     */
/* each of the supported architectures so the macro gets #defined    */
/* to its proper value for the architecture. YOU HAVE BEEN WARNED.   */
/*                                                                   */
/*********************************************************************/

/*-------------------------------------------------------------------*/
/* run_cpu jump table -- used by the below "cpu_thread" function to  */
/* call the proper ARCH_DEP(run_cpu) function for the architecture.  */
/*-------------------------------------------------------------------*/
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
            // "Error in function create_thread(): %s"
            WRMSG( HHC00102, "E", strerror( rc ));
            RELEASE_INTLOCK( NULL );
            return NULL;
        }
    }

    /* Set CPU thread priority */
    SET_THREAD_PRIORITY( sysblk.cpuprio, sysblk.qos_user_initiated );

    /* Display thread started message on control panel */

    MSGBUF( thread_name, "Processor %s%02X", PTYPSTR( cpu ), cpu );
    LOG_THREAD_BEGIN( thread_name  );
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
    LOG_THREAD_END( thread_name );

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
    setCpuIdregs( regs, -1, -1, -1, -1, true );

    initialize_condition (&regs->intcond);
    regs->cpulock = &sysblk.cpulock[cpu];

#if defined( _FEATURE_S370_S390_VECTOR_FACILITY )
    regs->vf = &sysblk.vf[cpu];
    regs->vf->online = (cpu < sysblk.numvec);
#endif
    initial_cpu_reset(regs);

    /*****************************************************************/
    /*       Refer to the PROGRAMMING NOTE in hstructs.h             */
    /*****************************************************************/
    /*                                                               */
    /* If 'hostregs' passed as NULL, then 'regs' points to host regs.*/
    /* Otherwise, 'regs' points to guest regs and 'hostregs' points  */
    /* to host regs.                                                 */
    /*                                                               */
    /* The flag 'guest' is only 1 in guest regs and 'host' is only 1 */
    /* in host regs. 'hostregs' always points to host regs in both,  */
    /* and 'guestregs' always points to guest regs in both.          */
    /*                                                               */
    /* HOWEVER, guest regs only exists when SIE is active/running.   */
    /* If SIE is NOT active/running, then 'guestregs' is NULL.       */
    /*                                                               */
    /* 'sie_active' is only 1 in host regs,  never in guest regs.    */
    /* 'sie_mode'   is only 1 in guest regs, never in host regs.     */
    /*                                                               */
    /*****************************************************************/
    /*       Refer to the PROGRAMMING NOTE in hstructs.h             */
    /*****************************************************************/

    if (!hostregs)
    {
        /* regs points to host regs */
        regs->cpustate = CPUSTATE_STOPPING;
        ON_IC_INTERRUPT(regs);
        HOSTREGS = regs;
        regs->host = 1;
        sysblk.regs[cpu] = regs;
        sysblk.config_mask |= regs->cpubit;
        sysblk.started_mask |= regs->cpubit;
    }
    else // SIE mode
    {
        /* regs     points to guest regs.
           hostregs points to host  regs.
        */
        GUEST(hostregs) = regs;
        HOSTREGS = hostregs;
        GUESTREGS = regs;
        regs->guest = 1;
        regs->sie_mode = 1;
        regs->opinterv = 0;
        regs->cpustate = CPUSTATE_STARTED;
    }

    /* Initialize accelerated lookup fields */
    switch (regs->arch_mode)
    {
#if defined(_370)
        case ARCH_370_IDX:
            regs->CR_G( CR_ASD_REAL ) = TLB_REAL_ASD_L;
            break;
#endif
#if defined(_390)
        case ARCH_390_IDX:
            regs->CR_G( CR_ASD_REAL ) = TLB_REAL_ASD_L;
            break;
#endif
#if defined(_900)
        case ARCH_900_IDX:
            regs->CR_G( CR_ASD_REAL ) = TLB_REAL_ASD_G;
            break;
#endif
        default: CRASH();
    }

    for (i=0; i < 16; i++)
        regs->AEA_AR( i )               = CR_ASD_REAL;
    regs->AEA_AR( USE_INST_SPACE      ) = CR_ASD_REAL;
    regs->AEA_AR( USE_REAL_ADDR       ) = CR_ASD_REAL;
    regs->AEA_AR( USE_PRIMARY_SPACE   ) = 1;
    regs->AEA_AR( USE_SECONDARY_SPACE ) = 7;
    regs->AEA_AR( USE_HOME_SPACE      ) = 13;

    /* Initialize opcode table pointers */
    init_regs_runtime_opcode_pointers( regs );

    regs->configured = 1;

    release_lock (&sysblk.cpulock[cpu]);

    return 0;
}

/*-------------------------------------------------------------------*/
/* Uninitialize a CPU                                                */
/*-------------------------------------------------------------------*/
static void *cpu_uninit (int cpu, REGS *regs)
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
        if (GUESTREGS)
            GUESTREGS = (regs == GUESTREGS) ?
                NULL : cpu_uninit( cpu, GUESTREGS );
    }

    destroy_condition(&regs->intcond);

    if (processHostRegs)
    {
#if defined( _FEATURE_S370_S390_VECTOR_FACILITY )
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
    TXF_FREEMAP( regs );
    free_aligned( regs );

    return NULL;
}

/*-------------------------------------------------------------------*/
/* CPU Wait - Core wait routine for both CPU Wait and Stopped States */
/*                                                                   */
/* Locks Held                                                        */
/*      sysblk.intlock                                               */
/*-------------------------------------------------------------------*/
static void CPU_Wait( REGS* regs )
{
    /* Indicate we are giving up intlock */
    sysblk.intowner = LOCK_OWNER_NONE;

    /* Wait while SYNCHRONIZE_CPUS is in progress */
    while (sysblk.syncing)
    {
        sysblk.sync_mask &= ~HOSTREGS->cpubit;
        if (!sysblk.sync_mask)
            signal_condition(&sysblk.all_synced_cond);
        wait_condition (&sysblk.sync_done_cond, &sysblk.intlock);
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
        default: CRASH();
    }
} /* end function copy_psw */

/*-------------------------------------------------------------------*/
/* Display program status word taking loadstate into consideration.  */
/*-------------------------------------------------------------------*/
int display_psw( REGS* regs, char* buf, int buflen )
{
    /* Use the architecture mode from SYSBLK if load indicator
       shows IPL in process, otherwise use archmode from REGS */
    int arch_mode = (regs->loadstate) ? sysblk.arch_mode
                                      :  regs->arch_mode;
    return strlen( str_arch_psw( arch_mode, regs, buf, buflen ));
}

/*-------------------------------------------------------------------*/
/* Print program status word into buffer for default REGS arch_mode  */
/*-------------------------------------------------------------------*/
char* str_psw( REGS* regs, char* buf, int buflen )
{
    return str_arch_psw( regs->arch_mode, regs, buf, buflen );
}

/*-------------------------------------------------------------------*/
/* Print program status word into buffer for specified arch_mode     */
/*-------------------------------------------------------------------*/
char* str_arch_psw( int arch_mode, REGS* regs, char* buf, int buflen )
{
    QWORD   qword   = {0};                /* quadword work area      */

    copy_psw( regs, qword );

    if (arch_mode != ARCH_900_IDX)
    {
        snprintf( buf, buflen,
            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X",
            qword[0], qword[1], qword[2], qword[3],
            qword[4], qword[5], qword[6], qword[7] );
    }
    else
    {
        snprintf( buf, buflen,
            "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X "
            "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
            qword[ 0], qword[ 1], qword[ 2], qword[ 3],
            qword[ 4], qword[ 5], qword[ 6], qword[ 7],
            qword[ 8], qword[ 9], qword[10], qword[11],
            qword[12], qword[13], qword[14], qword[15] );
    }

    return buf;
}

/*-------------------------------------------------------------------*/
/*                      Automatic Tracing                            */
/*-------------------------------------------------------------------*/
void do_automatic_tracing()
{
    static U64  inst_count;         // (current sysblk.instcount)
    static U64  missed_by;          // (how far past trigger we went)
    static U64  too_much;           // (num extra instructions traced)

    bool started = false, stopped = false;
    int cpu;

    /* Return immediately if automatic tracing not enabled or active */
    //if (!sysblk.auto_trace_amt)
    //    return;

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

        /* Enable/disable CPU tracing based on overall trace status */
        for (cpu=0; cpu < sysblk.maxcpu; cpu++)
        {
            if (IS_CPU_ONLINE( cpu ))
                sysblk.regs[ cpu ]->insttrace = sysblk.insttrace;
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

/*-------------------------------------------------------------------*/
/*                        make_psw64                                 */
/*-------------------------------------------------------------------*/
U64 make_psw64( REGS* regs, int arch /*370/390/900*/, bool bc )
{
    return do_make_psw64( &regs->psw, REAL_ILC( regs ), arch, bc );
}

#endif /*!defined(_GEN_ARCH)*/
