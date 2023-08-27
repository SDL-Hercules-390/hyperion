/* SIE.C        (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) and others 2013-2021                             */
/*              Interpretive Execution                               */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */
/*                                                                   */
/*   This module contains the SIE instruction as described in        */
/*   SA22-7095-01 IBM S/370 Extended Architecture Interpretive       */
/*   Execution, and SC24-5594-02 and SC24-5965-00 Enterprise         */
/*   Systems Architecture / Extended Configuration Principles        */
/*   of Operation.                                                   */

#include "hstdinc.h"

// #define SIE_DEBUG

#define _SIE_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "sie.h"

DISABLE_GCC_UNUSED_SET_WARNING;

#if defined( _FEATURE_SIE )

#if !defined( COMPILE_THIS_ONLY_ONCE )
#define       COMPILE_THIS_ONLY_ONCE

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section: due to above COMPILE_THIS_ONLY_ONCE guard, */
/*  the below header section is compiled only ONCE, *before*         */
/*  the very first architecture is ever built.                       */
/*-------------------------------------------------------------------*/
/*  The ARCH_DEP section (compiled multiple times, once for each     */
/*  architecture) follows AFTER the COMPILE_THIS_ONLY_ONCE section.  */
/*-------------------------------------------------------------------*/

#if defined( SIE_DEBUG )
static const char* sie_icode_2str( int icode );
#endif

/*-------------------------------------------------------------------*/
/*              static function forward references                   */
/*-------------------------------------------------------------------*/

static int   s370_run_sie( REGS* regs );
static int   s390_run_sie( REGS* regs );
#if defined( _900 )
static int   z900_run_sie( REGS* regs );
#endif

/*-------------------------------------------------------------------*/
/*                  'run_sie' function table                         */
/*-------------------------------------------------------------------*/
static int (*run_sie[ NUM_GEN_ARCHS ])( REGS* regs) =
{
#if defined(      _ARCH_NUM_0 )
  #if      370 == _ARCH_NUM_0
          s370_run_sie,

  #elif    390 == _ARCH_NUM_0
          s390_run_sie,

  #else // 900 == _ARCH_NUM_0
          z900_run_sie,
  #endif
#endif
#if defined(      _ARCH_NUM_1 )
  #if      370 == _ARCH_NUM_1
          s370_run_sie,

  #elif    390 == _ARCH_NUM_1
          s390_run_sie,

  #else // 900 == _ARCH_NUM_1
          z900_run_sie,
  #endif
#endif
#if defined(      _ARCH_NUM_2 )
  #if      370 == _ARCH_NUM_2
          s370_run_sie,

  #elif    390 == _ARCH_NUM_2
          s390_run_sie,

  #else // 900 == _ARCH_NUM_2
          z900_run_sie,
  #endif
#endif
};

/*-------------------------------------------------------------------*/
/*                     SIE helper macros                             */
/*-------------------------------------------------------------------*/

#define STATEBK     ((SIEBK *)GUESTREGS->siebk)

#define SIE_I_STOP( _guestregs )                \
        ((_guestregs)->siebk->v & SIE_V_STOP)

#define SIE_I_EXT( _guestregs )                 \
        (((_guestregs)->siebk->v & SIE_V_EXT)   \
          && ((_guestregs)->psw.sysmask & PSW_EXTMASK))

#define SIE_I_HOST( _hostregs ) \
        IC_INTERRUPT_CPU( _hostregs )

/*-------------------------------------------------------------------*/
/*                       SIE debugging                               */
/*-------------------------------------------------------------------*/

#if defined( SIE_DEBUG_PERFMON )

  static int sie_perfmon[ 1 + 0x40 + -SIE_MAX_NEG_DEBUG ];

  #define SIE_PERFMON( _code )                          \
    do {                                                \
        sie_perfmon[ (_code) + -SIE_MAX_NEG_DEBUG ]++;  \
    } while(0)

  #define SIE_PERF_PGMINT                               \
    (code <= 0 ? code : (((code-1) & 0x3F)+1))

void* sie_perfmon_disp()
{
    static const char* dbg_name[] =
    {
        /* -31 */   "SIE re-dispatch state descriptor",
        /* -30 */   "SIE exit",
        /* -29 */   "SIE run",
        /* -28 */   "SIE runloop 1",
        /* -27 */   "SIE runloop 2",
        /* -26 */   "SIE interrupt check",
        /* -25 */   "SIE execute instruction",
        /* -24 */   "SIE unrolled execute",
        /* -23 */    NULL,
        /* -22 */    NULL,
        /* -21 */    NULL,
        /* -20 */    NULL,
        /* -19 */    NULL,
        /* -18 */    NULL,
        /* -17 */   "SIE intercept I/O instruction",
        /* -16 */   "SIE intercept I/O interrupt pending",
        /* -15 */   "SIE intercept I/O interrupt",
        /* -14 */   "SIE intercept PER interrupt",
        /* -13 */   "SIE validity check",
        /* -12 */   "SIE intercept external interrupt pending",
        /* -11 */   "SIE intercept machine check interrupt",
        /* -10 */   "SIE intercept restart interrupt",
        /* -9 */    "SIE intercept stop request",
        /* -8 */    "SIE intercept virtual machine wait",
        /* -7 */    "SIE intercept I/O interrupt request",
        /* -6 */    "SIE intercept external interrupt request",
        /* -5 */    "SIE intercept instruction completion",
        /* -4 */    "SIE intercept instruction",
        /* -3 */    "SIE host program interrupt",
        /* -2 */    "SIE host interrupt",
        /* -1 */    "SIE no intercept",
        /*  0 */    "SIE entry",
        /* 01 */    "SIE intercept Operation exception",
        /* 02 */    "SIE intercept Privileged-operation exception",
        /* 03 */    "SIE intercept Execute exception",
        /* 04 */    "SIE intercept Protection exception",
        /* 05 */    "SIE intercept Addressing exception",
        /* 06 */    "SIE intercept Specification exception",
        /* 07 */    "SIE intercept Data exception",
        /* 08 */    "SIE intercept Fixed-point-overflow exception",
        /* 09 */    "SIE intercept Fixed-point-divide exception",
        /* 0A */    "SIE intercept Decimal-overflow exception",
        /* 0B */    "SIE intercept Decimal-divide exception",
        /* 0C */    "SIE intercept HFP-exponent-overflow exception",
        /* 0D */    "SIE intercept HFP-exponent-underflow exception",
        /* 0E */    "SIE intercept HFP-significance exception",
        /* 0F */    "SIE intercept HFP-floating-point-divide exception",
        /* 10 */    "SIE intercept Segment-translation exception",
        /* 11 */    "SIE intercept Page-translation exception",
        /* 12 */    "SIE intercept Translation-specification exception",
        /* 13 */    "SIE intercept Special-operation exception",
        /* 14 */    "SIE intercept Pseudo-page-fault exception",
        /* 15 */    "SIE intercept Operand exception",
        /* 16 */    "SIE intercept Trace-table exception",
        /* 17 */    "SIE intercept ASN-translation exception",
        /* 18 */    "SIE intercept Page access exception",
        /* 19 */    "SIE intercept Vector/Crypto operation exception",
        /* 1A */    "SIE intercept Page state exception",
        /* 1B */    "SIE intercept Page transition exception",
        /* 1C */    "SIE intercept Space-switch event",
        /* 1D */    "SIE intercept Square-root exception",
        /* 1E */    "SIE intercept Unnormalized-operand exception",
        /* 1F */    "SIE intercept PC-translation specification exception",
        /* 20 */    "SIE intercept AFX-translation exception",
        /* 21 */    "SIE intercept ASX-translation exception",
        /* 22 */    "SIE intercept LX-translation exception",
        /* 23 */    "SIE intercept EX-translation exception",
        /* 24 */    "SIE intercept Primary-authority exception",
        /* 25 */    "SIE intercept LFX-translation exception",
        /* 26 */    "SIE intercept LSX-translation exception",
        /* 27 */    "SIE intercept Control-switch exception",
        /* 28 */    "SIE intercept ALET-specification exception",
        /* 29 */    "SIE intercept ALEN-translation exception",
        /* 2A */    "SIE intercept ALE-sequence exception",
        /* 2B */    "SIE intercept ASTE-validity exception",
        /* 2C */    "SIE intercept ASTE-sequence exception",
        /* 2D */    "SIE intercept Extended-authority exception",
        /* 2E */    "SIE intercept LSTE-sequence exception",
        /* 2F */    "SIE intercept ASTE-instance exception",
        /* 30 */    "SIE intercept Stack-full exception",
        /* 31 */    "SIE intercept Stack-empty exception",
        /* 32 */    "SIE intercept Stack-specification exception",
        /* 33 */    "SIE intercept Stack-type exception",
        /* 34 */    "SIE intercept Stack-operation exception",
        /* 35 */     NULL,
        /* 36 */     NULL,
        /* 37 */     NULL,
        /* 38 */    "SIE intercept ASCE-type exception",
        /* 39 */    "SIE intercept Region-first-translation exception",
        /* 3A */    "SIE intercept Region-second-translation exception",
        /* 3B */    "SIE intercept Region-third-translation exception",
        /* 3C */     NULL,
        /* 3D */     NULL,
        /* 3E */     NULL,
        /* 3F */     NULL,
        /* 40 */    "SIE intercept Monitor event"
    };

    if (sie_perfmon[ SIE_PERF_ENTER + -SIE_MAX_NEG_DEBUG ])
    {
        int  i;
        for (i=0; i < 0x61; i++)
            if (sie_perfmon[i])
                // "Counted %5u %s events"
                WRMSG( HHC02285, "I", sie_perfmon[i], dbg_name[i] );

        // "Average instructions / SIE invocation: %5u"
        WRMSG( HHC02286, "I",
            (sie_perfmon[ SIE_PERF_EXEC   + -SIE_MAX_NEG_DEBUG ] +
             sie_perfmon[ SIE_PERF_EXEC_U + -SIE_MAX_NEG_DEBUG ] * 7)
          /  sie_perfmon[ SIE_PERF_ENTER  + -SIE_MAX_NEG_DEBUG ] );
    }
    else
        // "No SIE performance data"
        WRMSG( HHC02287, "I" );
}
#else
  #define SIE_PERFMON(_code)
#endif

#endif /* !defined( COMPILE_THIS_ONLY_ONCE ) */

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

// (some needed  helper macros...)

#undef SIE_I_WAIT
#if defined(_FEATURE_WAITSTATE_ASSIST)

  #define SIE_I_WAIT(_guestregs) \
        (WAITSTATE(&(_guestregs)->psw) && !((_guestregs)->siebk->ec[0] & SIE_EC0_WAIA))
#else

  #define SIE_I_WAIT(_guestregs) \
        (WAITSTATE(&(_guestregs)->psw))
#endif

#undef SIE_I_IO
#if defined( FEATURE_BCMODE )

  #define SIE_I_IO(_guestregs)                                          \
        (((_guestregs)->siebk->v & SIE_V_IO)                            \
           && ((_guestregs)->psw.sysmask                                \
                 & (ECMODE(&(_guestregs)->psw) ? PSW_IOMASK : 0xFE) ))
#else

  #define SIE_I_IO(_guestregs)                                          \
        (((_guestregs)->siebk->v & SIE_V_IO)                            \
           && ((_guestregs)->psw.sysmask & PSW_IOMASK ))
#endif

#endif /* defined( _FEATURE_SIE ) */


#if defined( FEATURE_SIE )
/*-------------------------------------------------------------------*/
/* B214 SIE   - Start Interpretive Execution                     [S] */
/*-------------------------------------------------------------------*/
DEF_INST( start_interpretive_execution )
{
int     b2;                             /* Values of R fields        */
RADR    effective_addr2;                /* address of state desc.    */
int     n;                              /* Loop counter              */
U16     lhcpu;                          /* Last Host CPU address     */
U64     sie_state;                      /* Last SIE state            */
int lpsw_xcode;                         /* xcode from load_psw       */
volatile int icode = 0;                 /* interrupt code            */
                                        /* (why is this volatile?!)  */
bool    same_cpu, same_state;           /* boolean helper flags      */
U64     dreg;

#if defined( FEATURE_VIRTUAL_ARCHITECTURE_LEVEL )
U32     fld;                            /* Facility List Designator  */
#if !defined( OPTION_SIE2BK_FLD_COPY)
int     i;                              /* (work)                    */
#endif
#endif

    //-----------------------------------------------------------
    //             IMPORTANT SIE PROGRAMMING NOTE!
    //-----------------------------------------------------------
    // NOTE: Our execution architectural mode is that of the SIE
    // HOST and our 'regs' variable is pointing the the HOST's
    // registers. Since the GUEST could be running in a completely
    // different architecture from the HOST, if you need to call
    // a ARCH_DEP function for the GUEST (passing it GUESTREGS),
    // you must TAKE SPECIAL CARE to ensure the correct version
    // of that function is called! You cannot simply call the
    // "ARCH_DEP" version of that function as they are for the
    // architectue of the HOST, not the GUEST! (i.e. you cannot
    // call a "z900_xxx" function expecting it to work correctly
    // if the GUEST is supposed to call "s390_xxx" functions!)
    //-----------------------------------------------------------

    S( inst, regs, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK( regs, b2 );

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK( regs );

    SIE_INTERCEPT( regs );

    PTT_SIE( "SIE", regs->GR(14), regs->GR(15), effective_addr2);

    SIE_PERFMON( SIE_PERF_ENTER );

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) && !defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
    if (!regs->psw.amode || !PRIMARY_SPACE_MODE( &regs->psw ))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
#endif

    if (0
        || (effective_addr2 & (sizeof(SIEBK)-1)) != 0

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )

        || (effective_addr2 & PREFIXING_900_MASK) == 0
        || (effective_addr2 & PREFIXING_900_MASK) == regs->PX_900

#elif defined( FEATURE_S390_DAT )
        || (effective_addr2 & PREFIXING_390_MASK) == 0
        || (effective_addr2 & PREFIXING_390_MASK) == regs->PX_390
#else
        || (effective_addr2 & PREFIXING_370_MASK) == 0
        || (effective_addr2 & PREFIXING_370_MASK) == regs->PX_370
#endif
    )
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Perform serialization and checkpoint synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

#if defined( SIE_DEBUG )
    LOGMSG( "SIE: state descriptor " F_RADR "\n", effective_addr2 );
    ARCH_DEP( display_inst )( regs, regs->instinvalid ? NULL : regs->ip );
#endif

    if (effective_addr2 > regs->mainlim - (sizeof( SIEBK ) - 1))
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

    /*
     * As long as regs->sie_active is off, no serialization is
     * required for GUESTREGS.  sie_active should always be off here.
     * Any other thread looking at sie_active holds the intlock.
     */
    PTT_SIE( "SIE h,g,a", regs->host, regs->guest, regs->sie_active );
    if (regs->sie_active)
    {
        OBTAIN_INTLOCK( regs );
        {
            regs->sie_active = 0;
        }
        RELEASE_INTLOCK( regs );
    }

    /* Initialize guestregs if first time */
    if (!GUESTREGS)
    {
        PTT_SIE( "SIE calloc g", 0, 0, 0 );
        if (!(GUESTREGS = calloc_aligned( sizeof( REGS ), 4096 )))
        {
            // "Processor %s%02X: error in function %s: %s"
            WRMSG( HHC00813, "E", PTYPSTR( regs->cpuad ), regs->cpuad, "calloc()", strerror( errno ));
            return;
        }
        cpu_init( regs->cpuad, GUESTREGS, regs );
        TXF_ALLOCMAP( GUESTREGS );
    }

    /* Direct pointer to state descriptor block */
    GUESTREGS->siebk = (void*)(regs->mainstor + effective_addr2);

    /* Set the guest's execution arch_mode and load its PSW */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if (STATEBK->mx & SIE_MX_ESAME)
    {
        GUESTREGS->arch_mode = ARCH_900_IDX;
        GUESTREGS->program_interrupt = &z900_program_interrupt;
        lpsw_xcode = z900_load_psw( GUESTREGS, STATEBK->psw );
    }
#else /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
    if (STATEBK->m & SIE_M_370)
    {
#if defined(_370)
        GUESTREGS->arch_mode = ARCH_370_IDX;
        GUESTREGS->program_interrupt = &s370_program_interrupt;
        lpsw_xcode = s370_load_psw(GUESTREGS, STATEBK->psw);
#else
        /* Validity intercept when 370 mode not installed */
        SIE_SET_VI(SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_370NI, GUESTREGS);
        STATEBK->c = SIE_C_VALIDITY;
        return;
#endif
    }
#endif /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
    else
#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if (STATEBK->m & SIE_M_XA)
#endif
    {
        GUESTREGS->arch_mode = ARCH_390_IDX;
        GUESTREGS->program_interrupt = &s390_program_interrupt;
        lpsw_xcode = s390_load_psw(GUESTREGS, STATEBK->psw);
    }
#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    else
    {
        /* Validity intercept for invalid mode */
        SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_MODE, GUESTREGS );
        STATEBK->c = SIE_C_VALIDITY;
        return;
    }
#endif

    /* Prefered guest indication */
    GUESTREGS->sie_pref = (STATEBK->m & SIE_M_VR) ? 1 : 0;

    /* Load prefix from state descriptor... (Using 'PX_L' is okay
       since prefix is always a FWORD regardless of architecture) */
    FETCH_FW( GUESTREGS->PX_L, STATEBK->prefix );

#if defined( FEATURE_REGION_RELOCATE )

    if (STATEBK->mx & SIE_MX_RRF)
    {
        RADR mso, msl, eso = 0, esl = 0;

        if (STATEBK->zone >= FEATURE_SIE_MAXZONES)
        {
            /* Validity intercept for invalid zone */
            SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_AZNNI, GUESTREGS );
            STATEBK->c = SIE_C_VALIDITY;
            return;
        }

        mso = (sysblk.zpb[ STATEBK->zone ].mso & 0xFFFFF) << 20;
        msl = (sysblk.zpb[ STATEBK->zone ].msl & 0xFFFFF) << 20;
        eso = (sysblk.zpb[ STATEBK->zone ].eso & 0xFFFFF) << 20;
        esl = (sysblk.zpb[ STATEBK->zone ].esl & 0xFFFFF) << 20;

        if (mso > msl)
        {
            /* Validity intercept for invalid zone */
            SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_MSDEF, GUESTREGS );
            STATEBK->c = SIE_C_VALIDITY;
            return;
        }

        /* Ensure addressing exceptions on incorrect zone defs */
        if (0
            || mso > regs->mainlim
            || msl > regs->mainlim
        )
            mso = msl = 0;

#if defined( SIE_DEBUG )
        LOGMSG( "SIE: zone %d: mso=" F_RADR " msl=" F_RADR "\n", STATEBK->zone, mso, msl );
#endif

        GUESTREGS->sie_pref  =  1;
        GUESTREGS->sie_mso   =  0;
        GUESTREGS->mainstor  =  &sysblk.mainstor[mso];
        GUESTREGS->mainlim   =  msl - mso;
        GUESTREGS->storkeys  =  ARCH_DEP( get_ptr_to_storekey )( mso );
        GUESTREGS->sie_xso   =  eso;
        GUESTREGS->sie_xsl   =  esl;
        GUESTREGS->sie_xso  *=  (XSTORE_INCREMENT_SIZE >> XSTORE_PAGESHIFT);
        GUESTREGS->sie_xsl  *=  (XSTORE_INCREMENT_SIZE >> XSTORE_PAGESHIFT);
    }
    else
#endif /* defined( FEATURE_REGION_RELOCATE ) */
    {
        GUESTREGS->mainstor = sysblk.mainstor;
        GUESTREGS->storkeys = sysblk.storkeys;

        if (STATEBK->zone)
        {
            /* Validity intercept for invalid zone */
            SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_AZNNZ, GUESTREGS );
            STATEBK->c = SIE_C_VALIDITY;
            return;
        }

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )

        /* Load main storage origin */
        FETCH_DW( GUESTREGS->sie_mso, STATEBK->mso );
        GUESTREGS->sie_mso &= SIE2_MS_MASK;

        /* Load main storage extent */
        FETCH_DW( GUESTREGS->mainlim, STATEBK->mse );
        GUESTREGS->mainlim |= ~SIE2_MS_MASK;

        if (GUESTREGS->sie_mso > GUESTREGS->mainlim)
        {
            SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_MSDEF, GUESTREGS );
            STATEBK->c = SIE_C_VALIDITY;
            return;
        }

        /* Calculate main storage size */
        GUESTREGS->mainlim -= GUESTREGS->sie_mso;

#else /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

        /* Load main storage origin */
        FETCH_HW( GUESTREGS->sie_mso, STATEBK->mso );
        GUESTREGS->sie_mso <<= 16;

        /* Load main storage extent */
        FETCH_HW( GUESTREGS->mainlim, STATEBK->mse );
        GUESTREGS->mainlim = ((GUESTREGS->mainlim + 1) << 16) - 1;

#endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

        /* Load expanded storage origin */
        GUESTREGS->sie_xso = STATEBK->xso[0] << 16
                           | STATEBK->xso[1] <<  8
                           | STATEBK->xso[2];
        GUESTREGS->sie_xso *= (XSTORE_INCREMENT_SIZE >> XSTORE_PAGESHIFT);

        /* Load expanded storage limit */
        GUESTREGS->sie_xsl = STATEBK->xsl[0] << 16
                           | STATEBK->xsl[1] <<  8
                           | STATEBK->xsl[2];
        GUESTREGS->sie_xsl *= (XSTORE_INCREMENT_SIZE >> XSTORE_PAGESHIFT);
    }

    /* Validate Guest prefix */
    if (GUESTREGS->PX > GUESTREGS->mainlim)
    {
        SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_PFOUT, GUESTREGS );
        STATEBK->c = SIE_C_VALIDITY;
        return;
    }

    /* System Control Area Origin */
    FETCH_FW( GUESTREGS->sie_scao, STATEBK->scao );
    GUESTREGS->sie_scao &= SIEISCAM;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    {
        /* For ESAME insert the high word of the address */
        U32 sie_scaoh;
        FETCH_FW( sie_scaoh, STATEBK->scaoh );
        GUESTREGS->sie_scao |= (RADR)sie_scaoh << 32;
    }
#endif

    if (GUESTREGS->sie_scao > regs->mainlim)
    {
        SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_SCADR, GUESTREGS );
        STATEBK->c = SIE_C_VALIDITY;
        return;
    }

    /* Validate MSO */
    if (GUESTREGS->sie_mso)
    {
        /* Preferred guest must have zero MSO */
        if (GUESTREGS->sie_pref)
        {
            SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_MSONZ, GUESTREGS );
            STATEBK->c = SIE_C_VALIDITY;
            return;
        }

        /* MCDS guest must have zero MSO */
        if (STATEBK->mx & SIE_MX_XC)
        {
            SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_MSODS, GUESTREGS );
            STATEBK->c = SIE_C_VALIDITY;
            return;
        }
    }

    GUESTREGS->sie_fld = false;  // (default until we learn otherwise)

#if defined( FEATURE_VIRTUAL_ARCHITECTURE_LEVEL )

    /* Set Virtual Architecture Level (Facility List) */
    /* SIE guest facilities by default start out same as host's */
    memcpy( GUESTREGS->facility_list, HOSTREGS->facility_list, STFL_HERC_BY_SIZE );

    /* Fetch address of optional SIE guest facility list designator */
    FETCH_FW( fld, STATEBK->fld );

    if (0
        || (U64)fld > regs->mainlim /* (beyond end of main storage?)  */
        || (fld & ~0x7ffffff8)      /* (above 2GB or not DW aligned?) */
    )
    {
        /* ZZ: FIXME
        SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_??ADR, GUESTREGS );
        */
        STATEBK->c = SIE_C_VALIDITY;
        return;
    }

    if (fld)
    {
        GUESTREGS->sie_fld = true;

#if defined( OPTION_SIE2BK_FLD_COPY)

        /* If a facility list designator was provided
           then it defines the SIE guest facility bits.
        */
        memcpy( GUESTREGS->facility_list, &regs->mainstor[ fld ], STFL_HERC_BY_SIZE );

#else /* !defined( OPTION_SIE2BK_FLD_COPY) */

        /* If a facility list designator was provided
           then it's used as a mask to clear the SIE
           guest facility bits which shouldn't be on.
        */
        for (i=0; i < (int) STFL_IBM_BY_SIZE; i++)
            GUESTREGS->facility_list[i] &= regs->mainstor[ fld + i ];

#endif /* defined( OPTION_SIE2BK_FLD_COPY) */
    }

    /* Prevent certain facility bits from being masked */
    BIT_ARRAY_SET( GUESTREGS->facility_list, STFL_001_ZARCH_INSTALLED );

    if (ARCH_900_IDX == GUESTREGS->arch_mode)
        BIT_ARRAY_SET( GUESTREGS->facility_list, STFL_002_ZARCH_ACTIVE );
    else
        BIT_ARRAY_CLR( GUESTREGS->facility_list, STFL_002_ZARCH_ACTIVE );

#endif /* defined( FEATURE_VIRTUAL_ARCHITECTURE_LEVEL ) */

   /* Reference and Change Preservation (RCP) Origin if high-order
      0x80 bit is off. Otherwise (high-order 0x80 bit is on, which
      it usually is for VM/ESA and z/VM), then the field ACTUALLY
      contains various Execution Control flags such as Storage Key
      Assist (SKA), etc.
   */
    FETCH_FW( GUESTREGS->sie_rcpo, STATEBK->rcpo );

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )

    if (!GUESTREGS->sie_rcpo && !GUESTREGS->sie_pref)
    {
        SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_RCZER, GUESTREGS );
        STATEBK->c = SIE_C_VALIDITY;
        return;
    }
#endif

    /* Load the CPU timer */
    FETCH_DW( dreg, STATEBK->cputimer );
    set_cpu_timer( GUESTREGS, dreg );

    /* Load the TOD clock offset for this guest */
    FETCH_DW( GUESTREGS->sie_epoch, STATEBK->epoch );
    GUESTREGS->tod_epoch = regs->tod_epoch + TOD_high64_to_ETOD_high56( GUESTREGS->sie_epoch );

    /* Load the clock comparator */
    FETCH_DW( GUESTREGS->clkc, STATEBK->clockcomp );
    GUESTREGS->clkc = TOD_high64_to_ETOD_high56( GUESTREGS->clkc );  /* Internal Hercules format */

    /* Load TOD Programmable Field */
    FETCH_HW( GUESTREGS->todpr, STATEBK->todpf );

    /* Load the guest registers */
    memcpy( GUESTREGS->gr,  regs->gr,  14 * sizeof( regs->gr [0] ));
    memcpy( GUESTREGS->ar,  regs->ar,  16 * sizeof( regs->ar [0] ));
    memcpy( GUESTREGS->fpr, regs->fpr, 32 * sizeof( regs->fpr[0] ));
#if defined( FEATURE_BINARY_FLOATING_POINT )
    GUESTREGS->fpc =  regs->fpc;
#endif

    /* Load GR14 and GR15 */
    FETCH_W( GUESTREGS->GR(14), STATEBK->gr14 );
    FETCH_W( GUESTREGS->GR(15), STATEBK->gr15 );

    /* Load control registers */
    for (n=0; n < 16; n++)
        FETCH_W( GUESTREGS->CR( n ), STATEBK->cr[ n ]);

    /* Remember whether CPU or state is different from last time */
    FETCH_HW( lhcpu, STATEBK->lhcpu );
    sie_state = SIE_STATE( GUESTREGS );

    same_cpu   = (regs->cpuad == lhcpu);
    same_state = (effective_addr2 == sie_state);

    /* Regardless, set both of them to their proper current value */
    SIE_STATE( GUESTREGS ) = effective_addr2;
    STORE_HW( STATEBK->lhcpu, regs->cpuad );

    /*----------------------------------------*/
    /* Maybe purge the guest's TLB/ALB or not */
    /*----------------------------------------*/
#if !defined( OPTION_SIE_PURGE_DAT_ALWAYS )
    /*
     *   If this is not the same last host cpu that dispatched
     *   this state descriptor then clear the guest TLB entries.
     */
    if (!same_cpu || !same_state)
    {
        SIE_PERFMON( SIE_PERF_ENTER_F );

        /* Purge guest TLB entries */

        switch (GUESTREGS->arch_mode)
        {
        case ARCH_370_IDX: s370_purge_tlb( GUESTREGS );                              break;
        case ARCH_390_IDX: s390_purge_tlb( GUESTREGS ); s390_purge_alb( GUESTREGS ); break;
        case ARCH_900_IDX: z900_purge_tlb( GUESTREGS ); z900_purge_alb( GUESTREGS ); break;
        default: CRASH();
        }
    }
#else // defined( OPTION_SIE_PURGE_DAT_ALWAYS )
    /*
     *   ALWAYS purge guest TLB entries (Ivan 2016-07-30)
     */
    switch (GUESTREGS->arch_mode)
    {
    case ARCH_370_IDX: s370_purge_tlb( GUESTREGS );                              break;
    case ARCH_390_IDX: s390_purge_tlb( GUESTREGS ); s390_purge_alb( GUESTREGS ); break;
    case ARCH_900_IDX: z900_purge_tlb( GUESTREGS ); z900_purge_alb( GUESTREGS ); break;
    default: CRASH();
    }
#endif // defined( OPTION_SIE_PURGE_DAT_ALWAYS )

    /* Initialize interrupt mask and state */
    ARCH_DEP( set_guest_ic_mask )( GUESTREGS );
    SET_IC_INITIAL_STATE( GUESTREGS );
    SET_IC_PER( GUESTREGS );

    /* Initialize accelerated address lookup values */
    ARCH_DEP( set_guest_aea_mode   )( GUESTREGS );
    ARCH_DEP( set_guest_aea_common )( GUESTREGS );
    ARCH_DEP( invalidate_guest_aia )( GUESTREGS );

    GUESTREGS->breakortrace = regs->breakortrace;

    /* Must do setjmp(progjmp) here since the 'translate_addr' further
       below may result in longjmp(progjmp) for addressing exceptions.
    */
    if (!(icode = setjmp( GUESTREGS->progjmp )))
    {
        /*
         * Set sie_active to 1. This means other threads
         * may now access guestregs when holding intlock.
         * This is the *ONLY* place sie_active is set to 1!
         */
        PTT_SIE( "SIE a=1", 0, 0, 0 );
        OBTAIN_INTLOCK( regs );
        {
            regs->sie_active = 1;
        }
        RELEASE_INTLOCK( regs );

        /* Get PSA pointer and ensure PSA is paged in */
        if (GUESTREGS->sie_pref)
        {
            GUESTREGS->psa = (PSA_3XX*)(GUESTREGS->mainstor + GUESTREGS->PX);
            GUESTREGS->sie_px = GUESTREGS->PX;
        }
        else
        {
            /* NOTE: longjmp(progjmp) for addressing exception is possible
               here. Thus the need for doing setjmp(progjmp) further above.
            */
            /* Translate where this SIE guest's absolute storage begins
               (which is a host virtual address) to a host real address.
            */
            if (ARCH_DEP( translate_addr )( GUESTREGS->sie_mso + GUESTREGS->PX,
                                            USE_PRIMARY_SPACE, regs, ACCTYPE_SIE ))
            {
                SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_PFACC, GUESTREGS );
                STATEBK->c = SIE_C_VALIDITY;

                PTT_SIE( "SIE a=0", 0, 0, 0 );
                OBTAIN_INTLOCK( regs );
                {
                    regs->sie_active = 0;
                }
                RELEASE_INTLOCK( regs );
                return;
            }

            /* Convert host real address to host absolute address */
            GUESTREGS->sie_px = apply_host_prefixing( regs, regs->dat.raddr );

            if (regs->dat.protect || GUESTREGS->sie_px > regs->mainlim)
            {
                SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_PFACC, GUESTREGS );
                STATEBK->c = SIE_C_VALIDITY;

                PTT_SIE( "SIE a=0", 0, 0, 0 );
                OBTAIN_INTLOCK( regs );
                {
                    regs->sie_active = 0;
                }
                RELEASE_INTLOCK( regs );
                return;
            }
            GUESTREGS->psa = (PSA_3XX*)(GUESTREGS->mainstor + GUESTREGS->sie_px);
        }

        OBTAIN_INTLOCK( regs );
        {
            /* Intialize guest timers... */

            /* CPU timer */
            if (CPU_TIMER( GUESTREGS ) < 0)
                ON_IC_PTIMER( GUESTREGS );

            /* Clock comparator */
            if (TOD_CLOCK( GUESTREGS ) > GUESTREGS->clkc)
                ON_IC_CLKC( GUESTREGS );

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )

            /* Interval timer if S/370 and timer is enabled */
            if (1
                && (STATEBK->m & SIE_M_370)
                && !(STATEBK->m & SIE_M_ITMOF)
            )
            {
                S32 itimer, olditimer;
                U32 residue;

                /* Set the interval timer pending according to
                   the T bit in the state control
                */
                if (STATEBK->s & SIE_S_T)
                    ON_IC_ITIMER( GUESTREGS );

                /* Fetch the residue from the state descriptor */
                FETCH_FW( residue, STATEBK->residue );

                /* Fetch the timer value from location 80 */
                FETCH_FW( olditimer, GUESTREGS->psa->inttimer );

                /* Bit position 23 of the interval timer is decremented
                   once for each multiple of 3,333 usecs containded in
                   bit position 0-19 of the residue counter.
                */
                itimer = olditimer - ((residue / 3333) >> 4);

                /* Store the timer back */
                STORE_FW( GUESTREGS->psa->inttimer, itimer );

                /* Set interrupt flag and interval timer interrupt pending
                   if the interval timer went from positive to negative
                */
                if (itimer < 0 && olditimer >= 0)
                    ON_IC_ITIMER( GUESTREGS );
            }
#endif /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

        }
        RELEASE_INTLOCK( regs );

        /* Early exceptions associated with the guest load_psw() */
        if (lpsw_xcode)
        {
            PTT_SIE( "*SIE > pgmint", lpsw_xcode, 0, 0 );
            GUESTREGS->program_interrupt( GUESTREGS, lpsw_xcode );
        }

        /* Run SIE in guest's architecture mode */
        PTT_SIE( "SIE > run_sie", GUESTREGS->arch_mode, 0, 0 );
        icode = run_sie[ GUESTREGS->arch_mode ]( regs );
        PTT_SIE( "SIE < run_sie", icode, 0, 0 );

    } /* if (setjmp(GUESTREGS->progjmp)) */

    /* Exit from SIE mode */
    PTT_SIE( "SIE > sie_exit", icode, 0, 0 );
    ARCH_DEP( sie_exit )( regs, icode );
    PTT_SIE( "SIE < sie_exit", 0, 0, 0 );

    /* Perform serialization and checkpoint synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

    /* Return back to host instruction processing */
    PTT_SIE( "SIE progjmp", 0, 0, 0 );
    longjmp( regs->progjmp, SIE_NO_INTERCEPT );

} /* end of start_interpretive_execution instruction */
#endif /* defined( FEATURE_SIE ) */


#if defined( _FEATURE_SIE )
/*-------------------------------------------------------------------*/
/*                          run_sie                                  */
/*-------------------------------------------------------------------*/
/*                   Execute guest instructions...                   */
/*-------------------------------------------------------------------*/
static int ARCH_DEP( run_sie )( REGS* regs )
{
    BYTE*  ip;      /* instruction pointer        */
    int    icode;   /* SIE longjmp intercept code */
    int    i;
    const INSTR_FUNC*  current_opcode_table;

    //---------------------------------------------------------
    //              CRITICAL SIE PROGRAMMING NOTE!
    //---------------------------------------------------------
    //
    //  Our 'regs' variable always points the HOST's regs (i.e.
    //  regs == HOSTREGS && regs->host is always true), even though
    //  we are actually RUNNING (executing) in GUEST architecture
    //  mode!!
    //
    //  That is to say, if the host is, for example, z900 (z/VM)
    //  but the guest it wants to execute is a 390 guest, the above
    //  "start_interpretive_execution" instruction function calls
    //  the "s390_run_sie" function (because it wants to execute
    //  the guest in 390 mode), but it passed its own HOST regs to
    //  this function!
    //
    //  So even though our function's build architecture is 390
    //  (i.e. even though our function's build architecture is that
    //  of the GUEST's), our 'regs' pointer is nevertheless still
    //  pointing to the z/VM HOST's registers!! GUESTREGS must be
    //  used to access the GUEST's register context!!
    //
    //  What this means is that SPECIAL CARE must be taken when
    //  invoking macros or calling functions on behalf of the HOST
    //  (i.e. when using 'regs' or HOSTREGS instead of GUESTREGS)
    //  since many of our macros and functions are architecture
    //  dependent, relying on the regs they were called with to
    //  always match that of the current build architecture, which,
    //  as explained, is NOT necessarily always true in our case!!
    //
    //---------------------------------------------------------

    //-----------------------------------------------------------
    //               IMPORTANT SIE PROGRAMMING NOTE!
    //-----------------------------------------------------------
    // NOTE: Our execution architectural mode is that of the SIE
    // GUEST, not the HOST! If you need to call a function on
    // behalf of the HOST (passing it 'regs'), you must be careful
    // to ensure the correct version of that function is called!
    // You cannot simply call the "ARCH_DEP" version of a function
    // as they are for the architectue of the GUEST, not the HOST!
    // (e.g. you cannot call a "s390_xxxx" function expecting it
    // to work correctly if the HOST function that SHOULD have been
    // called should have been "z900_xxxx"!) YOU HAVE BEEN WARNED!
    //-----------------------------------------------------------

    PTT_SIE( "run_sie h,g,a", regs->host, regs->guest, regs->sie_active );

    SIE_PERFMON( SIE_PERF_RUNSIE );

#if defined( _FEATURE_PER )
    /* Reset any PER pending indication */
    OFF_IC_PER( GUESTREGS );
#endif

#ifdef FEATURE_INTERVAL_TIMER
    /* Load the shadow interval timer */
    {
        S32 itimer;
        FETCH_FW( itimer, GUESTREGS->psa->inttimer );
        set_int_timer( GUESTREGS, itimer );
    }
#endif

    do
    {
        SIE_PERFMON( SIE_PERF_RUNLOOP_1 );

        PTT_SIE( "run_sie setjmp", 0, 0, 0 );

        /* Establish longjmp destination for program check or
           RETURN_INTCHECK, or SIE_INTERCEPT, or longjmp, etc.
        */
        if (!(icode = setjmp( GUESTREGS->progjmp )))
        {
            PTT_SIE( "run_sie run...", 0, 0, 0 );
            do
            {
                SIE_PERFMON( SIE_PERF_RUNLOOP_2 );

                /* Set `execflag' to 0 in case EXecuted instruction did progjmp */
                GUESTREGS->execflag = 0;

                /* Exit from SIE mode when either asked or
                   if External or I/O Interrupt is pending
                */
                if (1
#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
                    /* Don't interrupt active transaction */
                    && (0
                        || GUESTREGS->txf_tnd == 0
                        || GUESTREGS->txf_PPA < PPA_MUCH_HELP_THRESHOLD
                       )
#endif
                    && (0
                        || SIE_I_STOP ( GUESTREGS )
                        || SIE_I_EXT  ( GUESTREGS )
                        || SIE_I_IO   ( GUESTREGS )
                       )
                )
                    break;

                if (SIE_IC_INTERRUPT_CPU( GUESTREGS ))
                {
                    SIE_PERFMON( SIE_PERF_INTCHECK );

                    /* Process PER program interrupts */
                    if (OPEN_IC_PER( GUESTREGS ))
                        ARCH_DEP( program_interrupt )( GUESTREGS, PGM_PER_EVENT );

                    OBTAIN_INTLOCK( regs );
                    OFF_IC_INTERRUPT( GUESTREGS );

                    /* Set psw.IA and invalidate the aia */
                    INVALIDATE_AIA( GUESTREGS );

                    /* Process External Interrupt if one is pending */
                    if (1
                        && OPEN_IC_EXTPENDING( GUESTREGS )
#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
                        /* Don't interrupt active transaction */
                        && (0
                            || GUESTREGS->txf_tnd == 0
                            || GUESTREGS->txf_PPA < PPA_MUCH_HELP_THRESHOLD
                           )
#endif
                    )
                        ARCH_DEP( perform_external_interrupt )( GUESTREGS );

                    /* Process I/O Interrupt if either I/O or SIGA Assist is enabled
                       and an I/O Interrupt is pending.
                    */
                    if (1
                        && (0
                            || (STATEBK->ec[0] & SIE_EC0_IOA)
                            || (STATEBK->ec[3] & SIE_EC3_SIGA)
                           )
                        && OPEN_IC_IOPENDING( GUESTREGS )
                    )
                    {
                        PERFORM_SERIALIZATION ( GUESTREGS );
                        PERFORM_CHKPT_SYNC    ( GUESTREGS );
                        ARCH_DEP( perform_io_interrupt )( GUESTREGS );
                    }

#if defined( _FEATURE_WAITSTATE_ASSIST )

                    /*  Is SIE guest in a wait state
                        AND Wait State Assist enabled?
                    */
                    if (1
                        && WAITSTATE( &GUESTREGS->psw )
                        && (STATEBK->ec[0] & SIE_EC0_WAIA)
                    )
                    {
                        /* Test for disabled wait PSW and issue message */
                        if (IS_IC_DISABLED_WAIT_PSW( GUESTREGS ))
                        {
                            RELEASE_INTLOCK( regs );
                            longjmp( GUESTREGS->progjmp, SIE_INTERCEPT_WAIT );
                        }

                        /* Skip conditions we cannot assist with */
                        if (0
                            || SIE_I_STOP ( GUESTREGS )
                            || SIE_I_EXT  ( GUESTREGS )
                            || SIE_I_IO   ( GUESTREGS )
                            || SIE_I_HOST (    regs   )
                        )
                        {
                            RELEASE_INTLOCK( regs );
                            break;
                        }

                        SET_AEA_MODE( GUESTREGS );

                        /* Assist with the wait... but only briefly */
                        {
                            struct timespec waittime;

                            U64 now = host_tod();

                            waittime.tv_sec  =   (now >> (64-51)) / ONE_MILLION;
                            waittime.tv_nsec = (((now >> (64-51)) % ONE_MILLION) + 3333) * 1000;
                            waittime.tv_sec  += waittime.tv_nsec /  ONE_BILLION;
                            waittime.tv_nsec %=                     ONE_BILLION;

                            regs->waittod = now;

                            sysblk.waiting_mask  |=  regs->cpubit;
                            sysblk.intowner       =  LOCK_OWNER_NONE;
                            {
                                timed_wait_condition( &regs->intcond, &sysblk.intlock, &waittime );

                                while (sysblk.syncing)
                                     wait_condition( &sysblk.sync_done_cond, &sysblk.intlock );
                            }
                            sysblk.intowner       =   regs->cpuad;
                            sysblk.waiting_mask  &=  ~regs->cpubit;

                            regs->waittime += host_tod() - regs->waittod;
                            regs->waittod = 0;
                        }

                        RELEASE_INTLOCK( regs );
                        break;

                    }
#endif /* defined( _FEATURE_WAITSTATE_ASSIST ) */

                    RELEASE_INTLOCK( regs );
                }

                /* Break out of loop if SIE guest is waiting */
                if (SIE_I_WAIT( GUESTREGS ))
                    break;

sie_fetch_instruction:

                ip = INSTRUCTION_FETCH( GUESTREGS, 0 );
                current_opcode_table = GUESTREGS->ARCH_DEP( runtime_opcode_xxxx );

#if defined( SIE_DEBUG )
                ARCH_DEP( display_inst )( GUESTREGS, GUESTREGS->instinvalid ? NULL : ip );
#endif
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
                if (FACILITY_ENABLED( 073_TRANSACT_EXEC, GUESTREGS ))
                    goto txf_facility_loop;
#endif

//fastest_no_txf_loop:

                SIE_PERFMON( SIE_PERF_EXEC );

                PROCESS_TRACE( GUESTREGS, ip, sie_fetch_instruction );
                EXECUTE_INSTRUCTION( current_opcode_table, ip, GUESTREGS );
                regs->instcount++;
                UPDATE_SYSBLK_INSTCOUNT( 1 );
                SIE_PERFMON( SIE_PERF_EXEC_U );

                for (i=0; i < MAX_CPU_LOOPS/2; i++)
                {
                    UNROLLED_EXECUTE( current_opcode_table, GUESTREGS );
                    UNROLLED_EXECUTE( current_opcode_table, GUESTREGS );
                }
                regs->instcount +=  (i * 2);
                UPDATE_SYSBLK_INSTCOUNT( (i * 2) );

                /* Perform automatic instruction tracing if it's enabled */
                DO_AUTOMATIC_TRACING();
                goto endloop;

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )

txf_facility_loop:

                SIE_PERFMON( SIE_PERF_EXEC );

                if (GUESTREGS->txf_tnd)
                    goto txf_slower_loop;

                PROCESS_TRACE( GUESTREGS, ip, sie_fetch_instruction );
                EXECUTE_INSTRUCTION( current_opcode_table, ip, GUESTREGS );
                regs->instcount++;
                UPDATE_SYSBLK_INSTCOUNT( 1 );
                SIE_PERFMON( SIE_PERF_EXEC_U );

                for (i=0; i < MAX_CPU_LOOPS/2; i++)
                {
                    if (GUESTREGS->txf_tnd)
                        break;

                    UNROLLED_EXECUTE( current_opcode_table, GUESTREGS );

                    if (GUESTREGS->txf_tnd)
                        break;

                    UNROLLED_EXECUTE( current_opcode_table, GUESTREGS );
                }
                regs->instcount +=  (i * 2);
                UPDATE_SYSBLK_INSTCOUNT( (i * 2) );

                /* Perform automatic instruction tracing if it's enabled */
                DO_AUTOMATIC_TRACING();
                goto endloop;

txf_slower_loop:

                PROCESS_TRACE( GUESTREGS, ip, sie_fetch_instruction );
                TXF_EXECUTE_INSTRUCTION( current_opcode_table, ip, GUESTREGS );
                regs->instcount++;
                UPDATE_SYSBLK_INSTCOUNT( 1 );
                SIE_PERFMON( SIE_PERF_EXEC_U );

                for (i=0; i < MAX_CPU_LOOPS/2; i++)
                {
                    if (!GUESTREGS->txf_tnd)
                        break;

                    TXF_UNROLLED_EXECUTE( current_opcode_table, GUESTREGS );

                    if (!GUESTREGS->txf_tnd)
                        break;

                    TXF_UNROLLED_EXECUTE( current_opcode_table, GUESTREGS );
                }
                regs->instcount +=  (i * 2);
                UPDATE_SYSBLK_INSTCOUNT( (i * 2) );

                /* Perform automatic instruction tracing if it's enabled */
                DO_AUTOMATIC_TRACING();
                goto endloop;

#endif /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

endloop:        ; // (nop to make compiler happy)
            }
            /******************************************/
            /* Remain in SIE (above loop) as long as: */
            /*  - No Host Interrupt is pending        */
            /*  - No SIE defined Interrupt is pending */
            /*    (Wait, External or I/O)             */
            /*  - No guest interrupt is pending       */
            /******************************************/
            while
            (0
#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
             /* Don't interrupt active transaction */
             || (1
                 && GUESTREGS->txf_tnd > 0
                 && GUESTREGS->txf_PPA >= PPA_MUCH_HELP_THRESHOLD
                )
#endif
             || (1
                 && !SIE_I_HOST            (    regs   )
                 && !SIE_I_WAIT            ( GUESTREGS )
                 && !SIE_I_EXT             ( GUESTREGS )
                 && !SIE_I_IO              ( GUESTREGS )
                 && !SIE_INTERRUPT_PENDING ( GUESTREGS )
                )
            );

            /* Otherwise break out of the above loop
               and check if we should exit from SIE
               (check is done slightly further below)
            */
        }
        else
        {
            /* Our above instruction execution loop didn't finish due
               to a longjmp(progjmp) having been done, bringing us to
               here, thereby causing the instruction counter to not be
               properly updated. Thus, we must update it here instead.
           */
            if (sysblk.ipled)
            {
                regs->instcount += MAX_CPU_LOOPS/2;
                UPDATE_SYSBLK_INSTCOUNT( MAX_CPU_LOOPS/2 );

                /* Perform automatic instruction tracing if it's enabled */
                DO_AUTOMATIC_TRACING();
            }
        }

        PTT_SIE( "run_sie !run", icode, 0, 0 );

        /* Check if we should remain in, or exit from, SIE mode */
        if (!icode || SIE_NO_INTERCEPT == icode)
        {
            /* Check PER first, higher priority */
            if (OPEN_IC_PER( GUESTREGS ))
                ARCH_DEP( program_interrupt )( GUESTREGS, PGM_PER_EVENT );

            /* Check for SIE exit conditions... */

                 if (SIE_I_EXT  ( GUESTREGS )) icode = SIE_INTERCEPT_EXTREQ;
            else if (SIE_I_IO   ( GUESTREGS )) icode = SIE_INTERCEPT_IOREQ;
            else if (SIE_I_STOP ( GUESTREGS )) icode = SIE_INTERCEPT_STOPREQ;
            else if (SIE_I_WAIT ( GUESTREGS )) icode = SIE_INTERCEPT_WAIT;
            else if (SIE_I_HOST (   regs    )) icode = SIE_HOST_INT_PEND;

            /* Otherwise we should remain in SIE mode */
        }

        PTT_SIE( "run_sie !run", icode, 0, 0 );
    }
    /* Try to remain in SIE mode if possible */
    while (!icode || icode == SIE_NO_INTERCEPT);

    PTT_SIE( "run_sie ret", icode, 0, 0 );

    return icode;
} /* end function run_sie */
#endif /* defined( _FEATURE_SIE ) */


#if defined( FEATURE_SIE )
/*-------------------------------------------------------------------*/
/*                         sie_exit                                  */
/*-------------------------------------------------------------------*/
/* Exit SIE state, restore registers and update the state descriptor */
/*-------------------------------------------------------------------*/
void ARCH_DEP( sie_exit )( REGS* regs, int icode )
{
    int n;
#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
    U64  itdba = 0;
    TDB* itdb = NULL;
    bool txf_contran = false;
    BYTE txf_tnd = 0;
#endif
    bool is_exrl = false;

    //-----------------------------------------------------------
    //              IMPORTANT SIE PROGRAMMING NOTE!
    //-----------------------------------------------------------
    // NOTE: Our execution architectural mode is that of the SIE
    // HOST, not the GUEST! If you need to call a function on
    // behalf of the GUEST (passing it 'GUESTREGS'), you must be
    // careful to ensure the correct version of that function is
    // called! You cannot simply call the "ARCH_DEP" version of
    // a function as they are for the architectue of the HOST,
    // not the GUEST! (e.g. you can't call a "z900_xxx" function
    // and expect it to work correctly if the GUEST is actually
    // supposed to be run in s390 mode!) YOU HAVE BEEN WARNED!
    //-----------------------------------------------------------

    PTT_SIE( "sie_xit i,h,g", icode, regs->host, regs->guest );

    /* PTT trace the SIE Exit... */
    if (pttclass & PTT_CL_SIE)
    {
        // (include some instruction information in the trace entry)

        U32    nt1  = 0;        // First 2 or 4 bytes of instruction.
                                // If ilc > 4 then last 2 bytes are
                                // in the first 2 bytes of nt2.

        U32    nt2  = 0;        // First 2 bytes are last 2 bytes of
                                // instruction when ilc = 6. Else 0.
                                // Last 2 bytes is opcode of the "EX"
                                // or "EXRL" instruction if instruction
                                // was being "executed". Else zeros.

        BYTE*  ip   = NULL;     // Work: points to the instruction or,
                                // if execute, the target instruction.

        if (!GUESTREGS->instinvalid)
        {
#if defined( FEATURE_035_EXECUTE_EXTN_FACILITY )
            if (FACILITY_ENABLED( 035_EXECUTE_EXTN, GUESTREGS ))
            {
                /* EXRL = Execute Relative Long instruction? */
                is_exrl =
                (1
                    &&   GUESTREGS->ip[0] == 0xc6
                    && !(GUESTREGS->ip[1] &  0x0f)
                );
            }
#endif
            /* EX or EXRL instruction? */
            if (GUESTREGS->ip[0] == 0x44 || is_exrl)
            {
                ip  =  GUESTREGS->exinst;
                nt2 = (GUESTREGS->ip[0] == 0x44) ? 0x44
                    : ((0xc6 << 8) | GUESTREGS->ip[1]);
            }
            else // not execute
            {
                ip  =  GUESTREGS->ip;
            }

            nt1 = (ip[0] << 24)
                | (ip[1] << 16);

            if (ILC( ip[0]) > 2)  nt1  |=  ((ip[2] <<  8) | (ip[3] <<  0));
            if (ILC( ip[0]) > 4)  nt2  |=  ((ip[4] << 24) | (ip[5] << 16));
        }

        PTT_SIE( "sie_xit inst", icode, nt1, nt2  );

    } // end PTT trace of SIE exit

#if defined( SIE_DEBUG )
    LOGMSG( "SIE: interception code %d = %s\n", icode, sie_icode_2str( icode ));
    ARCH_DEP( display_guest_inst )( GUESTREGS, GUESTREGS->instinvalid ? NULL : GUESTREGS->ip );
#endif

    SIE_PERFMON( SIE_PERF_EXIT   );
    SIE_PERFMON( SIE_PERF_PGMINT );

    {
        /* Obtain INTLOCK (unless we already own it) */
        REGS* realregs = GUEST( sysblk.regs[ regs->cpuad ]);
        if (!IS_INTLOCK_HELD( realregs ))
            OBTAIN_INTLOCK( regs );

        /* Indicate we have left SIE mode */
        PTT_SIE( "sie_xit a=0", 0, 0, 0  );
        regs->sie_active = 0;
        RELEASE_INTLOCK( regs );
    }

    /* Zeroize interception status */
    STATEBK->f = 0;

    /* Set the interception code in the SIE block */
    switch (icode < 0 ? icode : icode & 0x7F)
    {
       /* If host interrupt pending, then backup psw so that the SIE
          instruction gets re-executed again to re-enter SIE mode
          once the interrupt is processed by the host.
        */
        case SIE_HOST_INT_PEND:

            MAYBE_SET_PSW_IA_FROM_IP( regs );
            SET_PSW_IA_AND_MAYBE_IP( regs, regs->psw.IA - REAL_ILC( regs ));
            break;

        case SIE_HOST_PGM_INT:         /* do nothing */             break;
        case SIE_INTERCEPT_INST:       STATEBK->c = SIE_C_INST;     break;
        case SIE_INTERCEPT_PER:        STATEBK->f |= SIE_F_IF;
                                       /* fall through */
        case SIE_INTERCEPT_INSTCOMP:   STATEBK->c = SIE_C_BOTH;     break;
        case SIE_INTERCEPT_WAIT:       STATEBK->c = SIE_C_WAIT;     break;
        case SIE_INTERCEPT_STOPREQ:    STATEBK->c = SIE_C_STOPREQ;  break;
        case SIE_INTERCEPT_IOREQ:      STATEBK->c = SIE_C_IOREQ;    break;
        case SIE_INTERCEPT_EXTREQ:     STATEBK->c = SIE_C_EXTREQ;   break;
        case SIE_INTERCEPT_EXT:        STATEBK->c = SIE_C_EXTINT;   break;
        case SIE_INTERCEPT_VALIDITY:   STATEBK->c = SIE_C_VALIDITY; break;
        case SIE_INTERCEPT_IOINT:      /* fall through */
        case SIE_INTERCEPT_IOINTP:     STATEBK->c = SIE_C_IOINT;    break;
        case SIE_INTERCEPT_IOINST:     STATEBK->c = SIE_C_IOINST;   break;
        case PGM_OPERATION_EXCEPTION:  STATEBK->c = SIE_C_OPEREXC;  break;
        default:                       STATEBK->c = SIE_C_PGMINT;   break;
    }

    PTT_SIE( "sie_xit STA_c", STATEBK->c, 0, 0  );

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )

    /* US 8,880,959 B2, Greiner et al, 17.20:

       "Interception TDB: The 256-byte host real location
        specified by locations 488-495 (x'1E8') of the state
        description."
    */
    FETCH_DW( itdba, STATEBK->itdba );
    if (itdba)
        itdba = APPLY_PREFIXING( itdba, HOSTREGS->PX );

    PTT_TXF( "TXF itdb", itdba, 0, 0 );

    OBTAIN_TXFLOCK( GUESTREGS );
    {
        txf_tnd     = GUESTREGS->txf_tnd;
        txf_contran = GUESTREGS->txf_contran;
    }
    RELEASE_TXFLOCK( GUESTREGS );

    PTT_TXF( "TXF tnd,con", txf_tnd, txf_contran, 0 );

    /* "A transaction may be aborted due to causes that
        are outside the scope of the immediate configuration
        in which it executes. For example, transient events
        recognized by a hypervisor (such as LPAR or z/VM)
        may cause a transaction to be aborted."
    */
    if (txf_tnd)
    {
        PTT_TXF( "TXF abrt", 0, 0, TAC_MISC );

        GUESTREGS->txf_why |= TXF_WHY_SIE_EXIT;
        ABORT_TRANS( GUESTREGS, ABORT_RETRY_RETURN, TAC_MISC );
        itdb = &GUESTREGS->txf_tdb;
    }
    else if (GUESTREGS->txf_UPGM_abort)
    {
        PTT_TXF( "TXF upgm", GUESTREGS->txf_UPGM_abort, GUESTREGS->txf_aborts, 0 );
        itdb = &GUESTREGS->txf_tdb;
    }

    /* Check if we need to fill in an Interception TDB */
    if (STATEBK->c == SIE_C_PGMINT && itdba && itdb)
    {
        PTT_TXF( "TXF itdb <=", itdba, itdb, itdb->tdb_tac );

        /* US 8,880,959 B2, Greiner et al, 17.20:

           "The interception TDB is stored when an aborted
            transaction results in a guest program interruption
            interception (i.e. interception code 8).  When a
            transaction is aborted due to other causes, the
            contents of the interception TDB are unpredictable."
        */
        if (TXF_TRACING())
        {
            // "TXF: %s%02X: SIE: Populating Interception TDB at 0x%16.16"PRIx64
            WRMSG( HHC17714, "D", TXF_CPUAD( GUESTREGS ), itdba );
        }

        memcpy( HOSTREGS->mainstor + itdba, itdb, sizeof( TDB ));
    }
    else if (STATEBK->c == SIE_C_PGMINT && itdb)
    {
        PTT_TXF( "*TXF !itdba", itdba, itdb, itdb->tdb_tac );

        if (TXF_TRACING())
        {
            // "TXF: %s%02X: SIE: Interception TDB address not provided!"
            WRMSG( HHC17716, "D", TXF_CPUAD( GUESTREGS ));
        }
    }
    else if (STATEBK->c != SIE_C_PGMINT && itdba)
    {
        PTT_TXF( "*TXF itdba<=0", itdba, 0, 0 );

        /* The address of an Interception TDB was provided, but no
           transaction was aborted as a result of this intercepted
           program interrupt. For safety, return a "NULL" (empty)
           Interception TDB instead. (Sorry Dan! Could not locate
           Claudia Schiffer's phone number!)
        */
        memset( HOSTREGS->mainstor + itdba, 0, sizeof( TDB ));
    }

#endif /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

    /* Save CPU timer  */
    STORE_DW( STATEBK->cputimer, get_cpu_timer( GUESTREGS ));

    /* Save clock comparator */
    STORE_DW( STATEBK->clockcomp, ETOD_high64_to_TOD_high56( GUESTREGS->clkc ));

#if defined( _FEATURE_INTERVAL_TIMER ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    /* If this is a S/370 guest, and the interval timer is enabled
       then save the timer state control bit
    */
    if (1
        &&  (STATEBK->m & SIE_M_370)
        && !(STATEBK->m & SIE_M_ITMOF)
    )
    {
        /* Save the shadow interval timer */
        s370_store_int_timer( regs );

        if (IS_IC_ITIMER( GUESTREGS )) STATEBK->s |=  SIE_S_T;
        else                           STATEBK->s &= ~SIE_S_T;
    }
#endif /* defined( _FEATURE_INTERVAL_TIMER ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

    /* Save TOD Programmable Field */
    STORE_HW( STATEBK->todpf, GUESTREGS->todpr );

    /* Save GR14 and GR15 */
    STORE_W( STATEBK->gr14, GUESTREGS->GR( 14 ));
    STORE_W( STATEBK->gr15, GUESTREGS->GR( 15 ));

    /* Store the PSW */
    if (GUESTREGS->arch_mode == ARCH_390_IDX)
        s390_store_psw( GUESTREGS, STATEBK->psw );
#if defined( _370 ) || defined( _900 )
    else
#endif
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        z900_store_psw( GUESTREGS, STATEBK->psw );
#else
#if defined( _370 )
        s370_store_psw( GUESTREGS, STATEBK->psw );
#endif
#endif
    /* Save control registers */
    for (n=0; n < 16; n++)
        STORE_W( STATEBK->cr[ n ], GUESTREGS->CR( n ));

    /* Update the approprate host registers */
    memcpy( regs->gr,  GUESTREGS->gr,  14 * sizeof( regs->gr [0] ));
    memcpy( regs->ar,  GUESTREGS->ar,  16 * sizeof( regs->ar [0] ));
    memcpy( regs->fpr, GUESTREGS->fpr, 32 * sizeof( regs->fpr[0] ));
#if defined( FEATURE_BINARY_FLOATING_POINT )
    regs->fpc = GUESTREGS->fpc;
#endif

    /* Invalidate instruction address accelerator */
    INVALIDATE_AIA(regs);
    SET_AEA_MODE( regs );

    /* Zeroize the interruption parameters */
    memset( STATEBK->ipa, 0, 10 );

    /* If format-2 interception, we have more work to do */
    if (0
        || STATEBK->c == SIE_C_INST
        || STATEBK->c == SIE_C_BOTH
        || STATEBK->c == SIE_C_OPEREXC
        || STATEBK->c == SIE_C_IOINST
    )
    {
        /* Indicate interception format 2 */
        STATEBK->f |= SIE_F_IN;

#if defined( _FEATURE_PER )
        /* Handle PER or concurrent PER event */
        if (1
            && OPEN_IC_PER( GUESTREGS )
            && ECMODE( &GUESTREGS->psw )
            && (GUESTREGS->psw.sysmask & PSW_PERMODE)
        )
        {
            PSA* psa;

#if defined( _FEATURE_PER2 )

            GUESTREGS->perc |= OPEN_IC_PER( GUESTREGS ) >> ((32 - IC_CR9_SHIFT) - 16);

            /* Positions 14 and 15 contain zeros
               if a storage alteration event was NOT indicated
            */
            if (0
                || !(OPEN_IC_PER_SA( GUESTREGS ))
                ||  (OPEN_IC_PER_STURA( GUESTREGS ))
            )
                GUESTREGS->perc &= 0xFFFC;

#endif /* defined( _FEATURE_PER2 ) */

            /* Point to PSA fields in state descriptor */
            psa = (void*)(regs->mainstor + SIE_STATE(GUESTREGS) + SIE_IP_PSA_OFFSET);
            STORE_HW( psa->perint, GUESTREGS->perc   );
            STORE_W(  psa->peradr, GUESTREGS->peradr );
        }

        if (IS_IC_PER_IF( GUESTREGS ))
            STATEBK->f |= SIE_F_IF;

        /* Reset any pending PER indication */
        OFF_IC_PER( GUESTREGS );

#endif /* defined( _FEATURE_PER ) */

        /* Backup to the previous instruction */
        GUESTREGS->ip -= REAL_ILC( GUESTREGS );
        if (GUESTREGS->ip < GUESTREGS->aip)
            GUESTREGS->ip = GUESTREGS->inst;

#if defined( FEATURE_035_EXECUTE_EXTN_FACILITY )
        if (FACILITY_ENABLED( 035_EXECUTE_EXTN, GUESTREGS ))
        {
            /* EXRL = Execute Relative Long instruction? */
            is_exrl =
            (1
                &&   GUESTREGS->ip[0] == 0xc6
                && !(GUESTREGS->ip[1] &  0x0f)
            );
        }
#endif
        /* Update interception parameters in the state descriptor */
        if (GUESTREGS->ip[0] == 0x44 || is_exrl)
        {
            int ilc;
            STATEBK->f |= SIE_F_EX;
            ilc = ILC( GUESTREGS->exinst[0] );
#if defined( FEATURE_035_EXECUTE_EXTN_FACILITY )
            if (is_exrl)
                STATEBK->f |= (ilc << 4) & SIE_F_EXL;
#endif
            memcpy( STATEBK->ipa, GUESTREGS->exinst, ilc );
        }
        else
        {
            if (!GUESTREGS->instinvalid)
                memcpy( STATEBK->ipa, GUESTREGS->ip, ILC( GUESTREGS->ip[0] ));
        }
    }

    PTT_SIE( "sie_xit ret", 0, 0, 0  );

} /* end function sie_exit */
#endif /* defined( FEATURE_SIE ) */


#if defined( _FEATURE_SIE )
#if defined( FEATURE_SIE )
#if defined( FEATURE_REGION_RELOCATE )
/*-------------------------------------------------------------------*/
/* B23D STZP  - Store Zone Parameter                             [S] */
/*-------------------------------------------------------------------*/
DEF_INST(store_zone_parameter)
{
int     b2;                             /* Values of R fields        */
RADR    effective_addr2;                /* address of state desc.    */
ZPB     zpb;                            /* Zone Parameter Block      */
int     zone;                           /* Zone number               */

    S(inst, regs, b2, effective_addr2);

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);

    SIE_INTERCEPT(regs);

    PTT_IO("STZP", regs->GR_L(1), regs->GR_L(2),regs->psw.IA_L);

    FW_CHECK(regs->GR(2), regs);

    zone = regs->GR_LHLCL(1);

    if(zone >= FEATURE_SIE_MAXZONES)
    {
        PTT_ERR("*STZP", regs->GR_L(1), regs->GR_L(2),regs->psw.IA_L);
        regs->psw.cc = 3;
        return;
    }

    STORE_W(zpb.mso,sysblk.zpb[zone].mso);
    STORE_W(zpb.msl,sysblk.zpb[zone].msl);
    STORE_W(zpb.eso,sysblk.zpb[zone].eso);
    STORE_W(zpb.esl,sysblk.zpb[zone].esl);

    ARCH_DEP(vstorec(&zpb, sizeof(ZPB)-1,regs->GR(2), 2, regs));

    regs->psw.cc = 0;
}


/*-------------------------------------------------------------------*/
/* B23E SZP   - Set Zone Parameter                               [S] */
/*-------------------------------------------------------------------*/
DEF_INST(set_zone_parameter)
{
int     b2;                             /* Values of R fields        */
RADR    effective_addr2;                /* address of state desc.    */
ZPB     zpb;                            /* Zone Parameter Block      */
int     zone;                           /* Zone number               */
RADR    mso,                            /* Main Storage Origin       */
        msl,                            /* Main Storage Length       */
        eso,                            /* Expanded Storage Origin   */
        esl;                            /* Expanded Storage Length   */

    S(inst, regs, b2, effective_addr2);

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);

    SIE_INTERCEPT(regs);

    PTT_IO("SZP", regs->GR_L(1), regs->GR_L(2),regs->psw.IA_L);

    FW_CHECK(regs->GR(2), regs);

    zone = regs->GR_LHLCL(1);

    if(zone == 0 || zone >= FEATURE_SIE_MAXZONES)
    {
        PTT_ERR("*SZP", regs->GR_L(1), regs->GR_L(2),regs->psw.IA_L);
        regs->psw.cc = 3;
        return;
    }

    ARCH_DEP(vfetchc(&zpb, sizeof(ZPB)-1,regs->GR(2), 2, regs));

    FETCH_W(mso,zpb.mso);
    FETCH_W(msl,zpb.msl);
    FETCH_W(eso,zpb.eso);
    FETCH_W(esl,zpb.esl);

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if(  (mso & ~ZPB2_MS_VALID)
      || (msl & ~ZPB2_MS_VALID)
      || (eso & ~ZPB2_ES_VALID)
      || (esl & ~ZPB2_ES_VALID) )
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);
#endif

    sysblk.zpb[zone].mso = mso;
    sysblk.zpb[zone].msl = msl;
    sysblk.zpb[zone].eso = eso;
    sysblk.zpb[zone].esl = esl;

    regs->psw.cc = 0;
}
#endif /* defined( FEATURE_REGION_RELOCATE ) */


#if defined( FEATURE_IO_ASSIST )
/*-------------------------------------------------------------------*/
/* B23F TPZI  - Test Pending Zone Interrupt                      [S] */
/*-------------------------------------------------------------------*/
DEF_INST(test_pending_zone_interrupt)
{
int     b2;                             /* Values of R fields        */
RADR    effective_addr2;                /* address of state desc.    */
U32     ioid;                           /* I/O interruption address  */
U32     ioparm;                         /* I/O interruption parameter*/
U32     iointid;                        /* I/O interruption ident    */
FWORD   tpziid[3];
int     zone;                           /* Zone number               */

    S(inst, regs, b2, effective_addr2);

    TXF_INSTR_CHECK( regs );
    PRIV_CHECK(regs);

    SIE_INTERCEPT(regs);

    PTT_IO("TPZI", regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff),regs->psw.IA_L);

    FW_CHECK(regs->GR(2), regs);

    /* Perform serialization and checkpoint-synchronization */
    PERFORM_SERIALIZATION (regs);
    PERFORM_CHKPT_SYNC (regs);

    zone = regs->GR_LHLCL(1);

    if(zone >= FEATURE_SIE_MAXZONES)
    {
        PTT_ERR("*TPZI", regs->GR_L(1),(U32)(effective_addr2 & 0xffffffff),regs->psw.IA_L);
        regs->psw.cc = 0;
        return;
    }

    if( IS_IC_IOPENDING )
    {
        /* Obtain the interrupt lock */
        OBTAIN_INTLOCK(regs);

        /* Test (but don't clear!) pending interrupt, and set condition code */
        if( ARCH_DEP(present_zone_io_interrupt) (&ioid, &ioparm,
                                                       &iointid, zone) )

        /* Store the SSID word and I/O parameter if an interrupt was pending */
        {
            /* Store interruption parms */
            STORE_FW(tpziid[0],ioid);
            STORE_FW(tpziid[1],ioparm);
            STORE_FW(tpziid[2],iointid);

            /* Release the interrupt lock */
            RELEASE_INTLOCK(regs);

            ARCH_DEP(vstorec(&tpziid, sizeof(tpziid)-1,regs->GR(2), 2, regs));

            regs->psw.cc = 1;
        }
        else
        {
            /* Release the interrupt lock */
            RELEASE_INTLOCK(regs);
            regs->psw.cc = 0;
        }

    }
    else
        regs->psw.cc = 0;
}


/*-------------------------------------------------------------------*/
/* DIAG 002   - Update Interrupt Interlock Control Bit in PMCW       */
/*-------------------------------------------------------------------*/
void ARCH_DEP(diagnose_002) (REGS *regs, int r1, int r3)
{
DEVBLK *dev;
U32    newgr1;

    /* Program check if the ssid including lcss is invalid */
    SSID_CHECK(regs);

    /* Locate the device block for this subchannel */
    dev = find_device_by_subchan (regs->GR_L(1));

    /* Condition code 3 if subchannel does not exist,
       is not valid, or is not enabled */
    if (dev == NULL
        || (dev->pmcw.flag5 & PMCW5_V) == 0
        || (dev->pmcw.flag5 & PMCW5_E) == 0)
    {
        PTT_ERR("*DIAG002", regs->GR_L(r1),regs->GR_L(r3),regs->GR_L(1));
        regs->psw.cc = 3;
        return;
    }

    OBTAIN_DEVLOCK( dev );
    {
        /* Set newgr1 to the current value of pending and interlock control */
        newgr1 = ((dev->scsw.flag3 & SCSW3_SC_PEND)
                  || (dev->pciscsw.flag3 & SCSW3_SC_PEND)) ? 0x02 : 0;
        if(dev->pmcw.flag27 & PMCW27_I)
            newgr1 |= 0x01;

        /* Do a compare-and-swap operation on the interrupt interlock
           control bit where both interlock and pending bits are
           compared, but only the interlock bit is swapped */
        if((regs->GR_L(r1) & 0x03) == newgr1)
        {
            dev->pmcw.flag27 &= ~PMCW27_I;
            dev->pmcw.flag27 |= (regs->GR_L(r3) & 0x01) ? PMCW27_I : 0;
            regs->psw.cc = 0;
        }
        else
        {
            regs->GR_L(r1) &= ~0x03;
            regs->GR_L(r1) |= newgr1;
            regs->psw.cc = 1;
        }
    }
    RELEASE_DEVLOCK( dev );
}
#endif /* defined( FEATURE_IO_ASSIST ) */


#if defined( FEATURE_074_STORE_HYPER_INFO_FACILITY )
/*-------------------------------------------------------------------*/
/* B256 STHYI - CP Store Hypervisor Information                [RRE] */
/*-------------------------------------------------------------------*/
/* This instruction is part of z/VM's "CMMA" (Collaborative Memory   */
/* Management) facility/feature. It is NOT a valid z/Architecture    */
/* instruction and will always cause a program check when attempted  */
/* to be executed natively. It is a z/VM-ONLY instruction that can   */
/* only be used (executed) by guests running under z/VM via z/VM     */
/* instruction interception and simulation. An operation execption   */
/* program interrupt will always occur if this instruction is not    */
/* intercepted by z/VM.                                              */
/* Ref: page 895 of SC24-6272-03 "zVM 7.1 CP Programming Services"   */
/*-------------------------------------------------------------------*/
DEF_INST( store_hypervisor_information )
{
    int r1, r2;
    RRE( inst, regs, r1, r2 );
    SIE_INTERCEPT( regs );
    ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
}
#endif


#if defined( FEATURE_ZVM_ESSA )
/*-------------------------------------------------------------------*/
/* B9AB ESSA  - CP Extract and Set Storage Attributes        [RRF-c] */
/*-------------------------------------------------------------------*/
/* This instruction is part of z/VM's "CMMA" (Collaborative Memory   */
/* Management) facility/feature. It is NOT a valid z/Architecture    */
/* instruction and will always cause a program check when attempted  */
/* to be executed natively. It is a z/VM-ONLY instruction that can   */
/* only be used (executed) by guests running under z/VM via z/VM     */
/* instruction interception and simulation. An operation execption   */
/* program interrupt will always occur if this instruction is not    */
/* intercepted by z/VM.                                              */
/* Ref: page 870 of SC24-6272-03 "zVM 7.1 CP Programming Services"   */
/*-------------------------------------------------------------------*/
DEF_INST( extract_and_set_storage_attributes )
{
    int r1, r2, m3;
    RRF_M( inst, regs, r1, r2, m3 );
    SIE_INTERCEPT( regs );
    ARCH_DEP( program_interrupt )( regs, PGM_OPERATION_EXCEPTION );
}
#endif
#endif /* defined( FEATURE_SIE ) */


extern inline bool ARCH_DEP( LockUnlockSCALock )( REGS* regs, bool lock, bool trylock );
#if defined( OPTION_USE_SKAIP_AS_LOCK )
extern inline void ARCH_DEP( LockUnlockSKALock )( REGS* regs, bool lock );
#endif
extern inline void ARCH_DEP( LockUnlockRCPLock )( REGS* regs, RCPTE* rcpte, bool lock );
extern inline void ARCH_DEP( LockUnlockKeyLock )( REGS* regs, PGSTE* pgste, RCPTE* rcpte, bool lock );


extern inline PGSTE* ARCH_DEP( GetPGSTE           )( REGS* regs, U64 gabspage );
extern inline PGSTE* ARCH_DEP( GetPGSTEFromPTE    )( REGS* regs, U64 pte );
extern inline RCPTE* ARCH_DEP( GetOldRCP          )( REGS* regs, U64 gabspage );
extern inline void   ARCH_DEP( GetPGSTE_and_RCPTE )( REGS* regs, U64 gabspage, PGSTE** ppPGSTE, RCPTE** ppRCPTE );


#endif /* defined( _FEATURE_SIE ) */

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined(_GEN_ARCH)

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "sie.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "sie.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if defined( _FEATURE_SIE )

#if defined( SIE_DEBUG )
/*-------------------------------------------------------------------*/
/*     Return a text string describing an SIE intercept code         */
/*-------------------------------------------------------------------*/
static const char* sie_icode_2str( int icode )
{
    static const char* icode_names[] =
    {
        "Continue (after pgmint)",      // SIE_NO_INTERCEPT        (-1)
        "Host interrupt pending",       // SIE_HOST_INT_PEND       (-2)
        "Host program interrupt",       // SIE_HOST_PGM_INT        (-3)
        "Instruction interception",     // SIE_INTERCEPT_INST      (-4)
        "Instr. int TS/CS/CDS",         // SIE_INTERCEPT_INSTCOMP  (-5)
        "External interrupt",           // SIE_INTERCEPT_EXTREQ    (-6)
        "I/O interrupt",                // SIE_INTERCEPT_IOREQ     (-7)
        "Wait state loaded",            // SIE_INTERCEPT_WAIT      (-8)
        "STOP reqeust",                 // SIE_INTERCEPT_STOPREQ   (-9)
        "Restart interrupt",            // SIE_INTERCEPT_RESTART  (-10)
        "Machine Check interrupt",      // SIE_INTERCEPT_MCK      (-11)
        "External interrupt pending",   // SIE_INTERCEPT_EXT      (-12)
        "SIE validity check",           // SIE_INTERCEPT_VALIDITY (-13)
        "SIE guest per event",          // SIE_INTERCEPT_PER      (-14)
        "I/O Interruption",             // SIE_INTERCEPT_IOINT    (-15)
        "I/O Interruption pending",     // SIE_INTERCEPT_IOINTP   (-16)
        "I/O Instruction",              // SIE_INTERCEPT_IOINST   (-17)
    };

#if defined( SIE_DEBUG_PERFMON )
    static const char* perfmon_names[] =
    {
//      "SIE performance monitor",      // SIE_PERF_ENTER         ( 0 )

        "Enter Fast (retain state)",    // SIE_PERF_ENTER_F       (-31)
        "SIE exit",                     // SIE_PERF_EXIT          (-30)
        "run_sie entered",              // SIE_PERF_RUNSIE        (-29)
        "run_sie runloop 1",            // SIE_PERF_RUNLOOP_1     (-28)
        "run_sue runloop 2",            // SIE_PERF_RUNLOOP_2     (-27)
        "run_sie intcheck",             // SIE_PERF_INTCHECK      (-26)
        "run_sie execute inst",         // SIE_PERF_EXEC          (-25)
        "run_sie unrolled exec",        // SIE_PERF_EXEC_U        (-24)
    };
#endif

    const char* name;       // (string pointer to be returned)

    if (icode < 0)          // Intercept code?
    {
        if (icode >= SIE_MAX_NEG)
            name = icode_names[ -icode - 1 ];
        else
        {
#if defined( SIE_DEBUG_PERFMON )
            if (1
                && icode <= SIE_MIN_NEG_DEBUG
                && icode >= SIE_MAX_NEG_DEBUG
            )
                name = perfmon_names[ icode + -SIE_MAX_NEG_DEBUG ];
            else
#endif
                name = "???";
        }
    }
    else // (icode >= 0)    // Program interrupt code
    {
#if defined( SIE_DEBUG_PERFMON )
        if (icode == 0)
            name = "SIE performance monitor";
        else
#endif
            name = PIC2Name( icode );
    }

    return name;
}
#endif /* defined( SIE_DEBUG ) */

#endif /* defined( _FEATURE_SIE ) */

#endif /*!defined(_GEN_ARCH)*/
