/* SIE.C        (C) Copyright Jan Jaeger, 1999-2012                  */
/*              Interpretive Execution                               */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */
/*                                                                   */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*      This module contains the SIE instruction as                  */
/*      described in IBM S/370 Extended Architecture                 */
/*      Interpretive Execution, SA22-7095-01                         */
/*      and                                                          */
/*      Enterprise Systems Architecture / Extended Configuration     */
/*      Principles of Operation, SC24-5594-02 and SC24-5965-00       */


#include "hstdinc.h"

// #define SIE_DEBUG

#define _SIE_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

DISABLE_GCC_UNUSED_SET_WARNING;

#if defined(_FEATURE_SIE)

#if !defined(_SIE_C)

#define _SIE_C

static int   s370_run_sie( REGS* regs );
static int   s390_run_sie( REGS* regs );
#if defined( _900 )
static int   z900_run_sie( REGS* regs );
#endif

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

#define GUESTREGS (regs->guestregs)
#define STATEBK   ((SIEBK *)GUESTREGS->siebk)

#define SIE_I_STOP(_guestregs) \
        ((_guestregs)->siebk->v & SIE_V_STOP)

#define SIE_I_EXT(_guestregs) \
        (((_guestregs)->siebk->v & SIE_V_EXT) \
          && ((_guestregs)->psw.sysmask & PSW_EXTMASK))

#define SIE_I_HOST(_hostregs) IC_INTERRUPT_CPU(_hostregs)

#if defined(SIE_DEBUG_PERFMON)
#define SIE_PERF_MAXNEG 0x1F
static int sie_perfmon[0x41 + SIE_PERF_MAXNEG];
#define SIE_PERFMON(_code) \
    do { \
        sie_perfmon[(_code) + SIE_PERF_MAXNEG] ++; \
    } while(0)
#define SIE_PERF_PGMINT \
    (code <= 0 ? code : (((code-1) & 0x3F)+1))
void *sie_perfmon_disp()
{
static char *dbg_name[] = {
        /* -31 */       "SIE re-dispatch state descriptor",
        /* -30 */       "SIE exit",
        /* -29 */       "SIE run",
        /* -28 */       "SIE runloop 1",
        /* -27 */       "SIE runloop 2",
        /* -26 */       "SIE interrupt check",
        /* -25 */       "SIE execute instruction",
        /* -24 */       "SIE unrolled execute",
        /* -23 */       NULL,
        /* -22 */       NULL,
        /* -21 */       NULL,
        /* -20 */       NULL,
        /* -19 */       NULL,
        /* -18 */       NULL,
        /* -17 */       "SIE intercept I/O instruction",
        /* -16 */       "SIE intercept I/O interrupt pending",
        /* -15 */       "SIE intercept I/O interrupt",
        /* -14 */       "SIE intercept PER interrupt",
        /* -13 */       "SIE validity check",
        /* -12 */       "SIE intercept external interrupt pending",
        /* -11 */       "SIE intercept machine check interrupt",
        /* -10 */       "SIE intercept restart interrupt",
        /* -9 */        "SIE intercept stop request",
        /* -8 */        "SIE intercept virtual machine wait",
        /* -7 */        "SIE intercept I/O interrupt request",
        /* -6 */        "SIE intercept external interrupt request",
        /* -5 */        "SIE intercept instruction completion",
        /* -4 */        "SIE intercept instruction",
        /* -3 */        "SIE host program interrupt",
        /* -2 */        "SIE host interrupt",
        /* -1 */        "SIE no intercept",
        /*  0 */        "SIE entry",
        /* 01 */        "SIE intercept Operation exception",
        /* 02 */        "SIE intercept Privileged-operation exception",
        /* 03 */        "SIE intercept Execute exception",
        /* 04 */        "SIE intercept Protection exception",
        /* 05 */        "SIE intercept Addressing exception",
        /* 06 */        "SIE intercept Specification exception",
        /* 07 */        "SIE intercept Data exception",
        /* 08 */        "SIE intercept Fixed-point-overflow exception",
        /* 09 */        "SIE intercept Fixed-point-divide exception",
        /* 0A */        "SIE intercept Decimal-overflow exception",
        /* 0B */        "SIE intercept Decimal-divide exception",
        /* 0C */        "SIE intercept HFP-exponent-overflow exception",
        /* 0D */        "SIE intercept HFP-exponent-underflow exception",
        /* 0E */        "SIE intercept HFP-significance exception",
        /* 0F */        "SIE intercept HFP-floating-point-divide exception",
        /* 10 */        "SIE intercept Segment-translation exception",
        /* 11 */        "SIE intercept Page-translation exception",
        /* 12 */        "SIE intercept Translation-specification exception",
        /* 13 */        "SIE intercept Special-operation exception",
        /* 14 */        "SIE intercept Pseudo-page-fault exception",
        /* 15 */        "SIE intercept Operand exception",
        /* 16 */        "SIE intercept Trace-table exception",
        /* 17 */        "SIE intercept ASN-translation exception",
        /* 18 */        "SIE intercept Page access exception",
        /* 19 */        "SIE intercept Vector/Crypto operation exception",
        /* 1A */        "SIE intercept Page state exception",
        /* 1B */        "SIE intercept Page transition exception",
        /* 1C */        "SIE intercept Space-switch event",
        /* 1D */        "SIE intercept Square-root exception",
        /* 1E */        "SIE intercept Unnormalized-operand exception",
        /* 1F */        "SIE intercept PC-translation specification exception",
        /* 20 */        "SIE intercept AFX-translation exception",
        /* 21 */        "SIE intercept ASX-translation exception",
        /* 22 */        "SIE intercept LX-translation exception",
        /* 23 */        "SIE intercept EX-translation exception",
        /* 24 */        "SIE intercept Primary-authority exception",
        /* 25 */        "SIE intercept LFX-translation exception",
        /* 26 */        "SIE intercept LSX-translation exception",
        /* 27 */        "SIE intercept Control-switch exception",
        /* 28 */        "SIE intercept ALET-specification exception",
        /* 29 */        "SIE intercept ALEN-translation exception",
        /* 2A */        "SIE intercept ALE-sequence exception",
        /* 2B */        "SIE intercept ASTE-validity exception",
        /* 2C */        "SIE intercept ASTE-sequence exception",
        /* 2D */        "SIE intercept Extended-authority exception",
        /* 2E */        "SIE intercept LSTE-sequence exception",
        /* 2F */        "SIE intercept ASTE-instance exception",
        /* 30 */        "SIE intercept Stack-full exception",
        /* 31 */        "SIE intercept Stack-empty exception",
        /* 32 */        "SIE intercept Stack-specification exception",
        /* 33 */        "SIE intercept Stack-type exception",
        /* 34 */        "SIE intercept Stack-operation exception",
        /* 35 */        NULL,
        /* 36 */        NULL,
        /* 37 */        NULL,
        /* 38 */        "SIE intercept ASCE-type exception",
        /* 39 */        "SIE intercept Region-first-translation exception",
        /* 3A */        "SIE intercept Region-second-translation exception",
        /* 3B */        "SIE intercept Region-third-translation exception",
        /* 3C */        NULL,
        /* 3D */        NULL,
        /* 3E */        NULL,
        /* 3F */        NULL,
        /* 40 */        "SIE intercept Monitor event" };

    if(sie_perfmon[SIE_PERF_ENTER+SIE_PERF_MAXNEG])
    {
        int i;
        for(i = 0; i < 0x61; i++)
            if(sie_perfmon[i])
                WRMSG( HHC02285, "I" ,sie_perfmon[i], dbg_name[i] );
        WRMSG( HHC02286, "I",
            (sie_perfmon[SIE_PERF_EXEC+SIE_PERF_MAXNEG] +
             sie_perfmon[SIE_PERF_EXEC_U+SIE_PERF_MAXNEG]*7) /
             sie_perfmon[SIE_PERF_ENTER+SIE_PERF_MAXNEG] );
    }
    else
        WRMSG( HHC02287, "I" );
}
#else
#define SIE_PERFMON(_code)
#endif

#endif /*!defined(_SIE_C)*/

#undef SIE_I_WAIT
#if defined(_FEATURE_WAITSTATE_ASSIST)
#define SIE_I_WAIT(_guestregs) \
        (WAITSTATE(&(_guestregs)->psw) && !((_guestregs)->siebk->ec[0] & SIE_EC0_WAIA))
#else
#define SIE_I_WAIT(_guestregs) \
        (WAITSTATE(&(_guestregs)->psw))
#endif

#undef SIE_I_IO
#if defined(FEATURE_BCMODE)
#define SIE_I_IO(_guestregs) \
        (((_guestregs)->siebk->v & SIE_V_IO) \
           && ((_guestregs)->psw.sysmask \
                 & (ECMODE(&(_guestregs)->psw) ? PSW_IOMASK : 0xFE) ))
#else /*!defined(FEATURE_BCMODE)*/
#define SIE_I_IO(_guestregs) \
        (((_guestregs)->siebk->v & SIE_V_IO) \
           && ((_guestregs)->psw.sysmask & PSW_IOMASK ))
#endif /*!defined(FEATURE_BCMODE)*/

#endif /*defined(_FEATURE_SIE)*/
#if defined(FEATURE_INTERPRETIVE_EXECUTION)

/*-------------------------------------------------------------------*/
/* B214 SIE   - Start Interpretive Execution                     [S] */
/*-------------------------------------------------------------------*/
DEF_INST( start_interpretive_execution )
{
int     b2;                             /* Values of R fields        */
RADR    effective_addr2;                /* address of state desc.    */
int     n;                              /* Loop counter              */
U16     lhcpu;                          /* Last Host CPU address     */
volatile int icode;                     /* Interception code         */
U64     dreg;

    S( inst, regs, b2, effective_addr2 );

    SIE_INTERCEPT( regs );

    PRIV_CHECK( regs );

    PTT_SIE( "SIE", regs->GR_L(14), regs->GR_L(15), (U32)(effective_addr2 & 0xffffffff));

    SIE_PERFMON( SIE_PERF_ENTER );

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) && !defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
    if (!regs->psw.amode || !PRIMARY_SPACE_MODE(&(regs->psw)))
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIAL_OPERATION_EXCEPTION );
#endif

    if (0
        || (effective_addr2 & (sizeof(SIEBK)-1)) != 0

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )

        || (effective_addr2 & 0xFFFFFFFFFFFFF000ULL) == 0
        || (effective_addr2 & 0xFFFFFFFFFFFFF000ULL) == regs->PX
#else
        || (effective_addr2 & 0x7FFFF000) == 0
        || (effective_addr2 & 0x7FFFF000) == regs->PX
#endif
    )
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Perform serialization and checkpoint synchronization */
    PERFORM_SERIALIZATION( regs );
    PERFORM_CHKPT_SYNC( regs );

#if defined( SIE_DEBUG )
    LOGMSG( "SIE: state descriptor " F_RADR "\n", effective_addr2 );
    ARCH_DEP( display_inst )( regs, regs->instinvalid ? NULL : regs->ip );
#endif

    if (effective_addr2 > regs->mainlim - (sizeof(SIEBK)-1))
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

    /*
     * As long as regs->sie_active is off, no serialization is
     * required for GUESTREGS.  sie_active should always be off here.
     * Any other thread looking at sie_active holds the intlock.
     */
    if (regs->sie_active)
    {
        OBTAIN_INTLOCK( regs );
        regs->sie_active = 0;
        RELEASE_INTLOCK( regs );
    }

    /* Initialize guestregs if first time */
    if (!GUESTREGS)
    {
        if (!(GUESTREGS = calloc_aligned( sizeof( REGS ), 4096 )))
        {
            // "Processor %s%02X: error in function %s: %s"
            WRMSG( HHC00813, "E", PTYPSTR( regs->cpuad ), regs->cpuad, "calloc()", strerror( errno ));
            return;
        }
        cpu_init( regs->cpuad, GUESTREGS, regs );
        ALLOC_TXFMAP( GUESTREGS );
    }

    /* Direct pointer to state descriptor block */
    GUESTREGS->siebk = (void*)(regs->mainstor + effective_addr2);

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    if (STATEBK->mx & SIE_MX_ESAME)
    {
        /* If GUESTREGS was initialized for a different architecture,
           then we need to initialize it for the new architecture */
        if (GUESTREGS->arch_mode != ARCH_900_IDX)
        {
            /* REGS 'arch_mode' MUST be set BEFORE calling cpu_init! */
            GUESTREGS->arch_mode = ARCH_900_IDX;
            cpu_init( regs->cpuad, GUESTREGS, regs );
        }
        GUESTREGS->program_interrupt = &z900_program_interrupt;
        icode = z900_load_psw( GUESTREGS, STATEBK->psw );
    }
#else /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
    if (STATEBK->m & SIE_M_370)
    {
#if defined(_370)
        GUESTREGS->arch_mode = ARCH_370_IDX;
        GUESTREGS->program_interrupt = &s370_program_interrupt;
        icode = s370_load_psw(GUESTREGS, STATEBK->psw);
#else
        /* Validity intercept when 370 mode not installed */
        SIE_SET_VI(SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT,
          SIE_VI_WHY_370NI, GUESTREGS);
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
        icode = s390_load_psw(GUESTREGS, STATEBK->psw);
    }
#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    else
    {
        /* Validity intercept for invalid mode */
        SIE_SET_VI(SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT,
                   SIE_VI_WHY_MODE, GUESTREGS);
        STATEBK->c = SIE_C_VALIDITY;
        return;
    }
#endif

    /* Prefered guest indication */
    GUESTREGS->sie_pref = (STATEBK->m & SIE_M_VR) ? 1 : 0;

    /* Load prefix from state descriptor */
    FETCH_FW(GUESTREGS->PX, STATEBK->prefix);
    GUESTREGS->PX &=
#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
                     PX_MASK;
#else
                     (GUESTREGS->arch_mode == ARCH_900_IDX) ? PX_MASK : 0x7FFFF000;
#endif

#if defined( FEATURE_REGION_RELOCATE )
    if(STATEBK->mx & SIE_MX_RRF)
    {
    RADR mso, msl, eso = 0, esl = 0;

        if(STATEBK->zone >= FEATURE_SIE_MAXZONES)
        {
            /* Validity intercept for invalid zone */
            SIE_SET_VI(SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT,
              SIE_VI_WHY_AZNNI, GUESTREGS);
            STATEBK->c = SIE_C_VALIDITY;
            return;
        }

        mso = (sysblk.zpb[STATEBK->zone].mso & 0xFFFFF) << 20;
        msl = (sysblk.zpb[STATEBK->zone].msl & 0xFFFFF) << 20;
        eso = (sysblk.zpb[STATEBK->zone].eso & 0xFFFFF) << 20;
        esl = (sysblk.zpb[STATEBK->zone].esl & 0xFFFFF) << 20;

        if(mso > msl)
        {
            /* Validity intercept for invalid zone */
            SIE_SET_VI(SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT,
              SIE_VI_WHY_MSDEF, GUESTREGS);
            STATEBK->c = SIE_C_VALIDITY;
            return;
        }

        /* Ensure addressing exceptions on incorrect zone defs */
        if(mso > regs->mainlim || msl > regs->mainlim)
            mso = msl = 0;


#if defined( SIE_DEBUG )
        LOGMSG( "SIE: zone %d: mso=" F_RADR " msl=" F_RADR "\n", STATEBK->zone, mso, msl );
#endif

        GUESTREGS->sie_pref = 1;
        GUESTREGS->sie_mso = 0;
        GUESTREGS->mainstor = &(sysblk.mainstor[mso]);
        GUESTREGS->mainlim = msl - mso;
        GUESTREGS->storkeys = &(STORAGE_KEY(mso, &sysblk));
        GUESTREGS->sie_xso = eso;
        GUESTREGS->sie_xsl = esl;
        GUESTREGS->sie_xso *= (XSTORE_INCREMENT_SIZE >> XSTORE_PAGESHIFT);
        GUESTREGS->sie_xsl *= (XSTORE_INCREMENT_SIZE >> XSTORE_PAGESHIFT);
    }
    else
#endif /* defined( FEATURE_REGION_RELOCATE ) */
    {
        GUESTREGS->mainstor = sysblk.mainstor;
        GUESTREGS->storkeys = sysblk.storkeys;

        if(STATEBK->zone)
        {
            /* Validity intercept for invalid zone */
            SIE_SET_VI(SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT,
                       SIE_VI_WHY_AZNNZ, GUESTREGS);
            STATEBK->c = SIE_C_VALIDITY;
            return;
        }

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /* Load main storage origin */
        FETCH_DW(GUESTREGS->sie_mso,STATEBK->mso);
        GUESTREGS->sie_mso &= SIE2_MS_MASK;

        /* Load main storage extend */
        FETCH_DW(GUESTREGS->mainlim,STATEBK->mse);
        GUESTREGS->mainlim |= ~SIE2_MS_MASK;

        if(GUESTREGS->sie_mso > GUESTREGS->mainlim)
        {
            SIE_SET_VI(SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT,
                       SIE_VI_WHY_MSDEF, GUESTREGS);
            STATEBK->c = SIE_C_VALIDITY;
            return;
        }

        /* Calculate main storage size */
        GUESTREGS->mainlim -= GUESTREGS->sie_mso;

#else /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */
        /* Load main storage origin */
        FETCH_HW(GUESTREGS->sie_mso,STATEBK->mso);
        GUESTREGS->sie_mso <<= 16;

        /* Load main storage extend */
        FETCH_HW(GUESTREGS->mainlim,STATEBK->mse);
        GUESTREGS->mainlim = ((GUESTREGS->mainlim + 1) << 16) - 1;
#endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

        /* Load expanded storage origin */
        GUESTREGS->sie_xso = STATEBK->xso[0] << 16
                           | STATEBK->xso[1] << 8
                           | STATEBK->xso[2];
        GUESTREGS->sie_xso *= (XSTORE_INCREMENT_SIZE >> XSTORE_PAGESHIFT);

        /* Load expanded storage limit */
        GUESTREGS->sie_xsl = STATEBK->xsl[0] << 16
                           | STATEBK->xsl[1] << 8
                           | STATEBK->xsl[2];
        GUESTREGS->sie_xsl *= (XSTORE_INCREMENT_SIZE >> XSTORE_PAGESHIFT);
    }

    /* Validate Guest prefix */
    if(GUESTREGS->PX > GUESTREGS->mainlim)
    {
        SIE_SET_VI(SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT,
                   SIE_VI_WHY_PFOUT, GUESTREGS);
        STATEBK->c = SIE_C_VALIDITY;
        return;
    }

    /* System Control Area Origin */
    FETCH_FW(GUESTREGS->sie_scao, STATEBK->scao);
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    {
    U32 sie_scaoh;
        /* For ESAME insert the high word of the address */
        FETCH_FW(sie_scaoh, STATEBK->scaoh);
        GUESTREGS->sie_scao |= (RADR)sie_scaoh << 32;
    }
#endif

    if(GUESTREGS->sie_scao > regs->mainlim)
    {
        SIE_SET_VI(SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT,
                   SIE_VI_WHY_SCADR, GUESTREGS);
        STATEBK->c = SIE_C_VALIDITY;
        return;
    }

    /* Validate MSO */
    if (GUESTREGS->sie_mso)
    {
        /* Preferred guest must have zero MSO */
        if (GUESTREGS->sie_pref)
        {
            SIE_SET_VI(SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT,
              SIE_VI_WHY_MSONZ, GUESTREGS);
            STATEBK->c = SIE_C_VALIDITY;
            return;
        }

        /* MCDS guest must have zero MSO */
        if (STATEBK->mx & SIE_MX_XC)
        {
            SIE_SET_VI(SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT,
                       SIE_VI_WHY_MSODS, GUESTREGS);
            STATEBK->c = SIE_C_VALIDITY;
            return;
        }
    }

#if defined( FEATURE_VIRTUAL_ARCHITECTURE_LEVEL )
    /* Set Virtual Architecture Level (Facility List) */
    {
        U32    fld;     /* SIE Facility List Designator */

        /* SIE guest facilities by default start out same as host's */
        memcpy( GUESTREGS->facility_list, regs->facility_list, STFL_HERC_BY_SIZE );

        /* Fetch address of optional SIE guest facility list mask */
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

        /* If a facility list mask was provided then use it to
           clear SIE guest facility bits which shouldn't be on */
        if (fld)
        {
            int    i;
            BYTE   facilities_mask[ STFL_HERC_BY_SIZE ];

            /* Copy mask bits to work area */
            memcpy( facilities_mask, &regs->mainstor[ fld ], STFL_HERC_BY_SIZE );

            /* Prevent certain facility bits from being masked */
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
            BIT_ARRAY_SET( facilities_mask, STFL_001_ZARCH_INSTALLED );
#endif
            /* Mask SIE guest facility bits as requested */
            for (i=0; i < (int) STFL_IBM_BY_SIZE; i++)
               GUESTREGS->facility_list[i] &= facilities_mask[i];
        }
    }
#endif /* defined( FEATURE_VIRTUAL_ARCHITECTURE_LEVEL ) */

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    /* Reference and Change Preservation Origin */
    FETCH_FW( GUESTREGS->sie_rcpo, STATEBK->rcpo );
    if (!GUESTREGS->sie_rcpo && !GUESTREGS->sie_pref)
    {
        SIE_SET_VI( SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT, SIE_VI_WHY_RCZER, GUESTREGS );
        STATEBK->c = SIE_C_VALIDITY;
        return;
    }
#endif

    /* Load the CPU timer */
    FETCH_DW(dreg, STATEBK->cputimer);
    set_cpu_timer(GUESTREGS, dreg);

    /* Load the TOD clock offset for this guest */
    FETCH_DW(GUESTREGS->sie_epoch, STATEBK->epoch);
    GUESTREGS->tod_epoch = regs->tod_epoch + tod2etod(GUESTREGS->sie_epoch);

    /* Load the clock comparator */
    FETCH_DW(GUESTREGS->clkc, STATEBK->clockcomp);
    GUESTREGS->clkc = tod2etod(GUESTREGS->clkc);    /* Internal Hercules format */

    /* Load TOD Programmable Field */
    FETCH_HW(GUESTREGS->todpr, STATEBK->todpf);

    /* Load the guest registers */
    memcpy(GUESTREGS->gr, regs->gr, 14 * sizeof(regs->gr[0]));
    memcpy(GUESTREGS->ar, regs->ar, 16 * sizeof(regs->ar[0]));
    memcpy(GUESTREGS->fpr, regs->fpr, 32 * sizeof(regs->fpr[0]));
#if defined( FEATURE_BINARY_FLOATING_POINT )
    GUESTREGS->fpc =  regs->fpc;
#endif

    /* Load GR14 */
    FETCH_W(GUESTREGS->GR(14), STATEBK->gr14);

    /* Load GR15 */
    FETCH_W(GUESTREGS->GR(15), STATEBK->gr15);

    /* Load control registers */
    for(n = 0;n < 16; n++)
        FETCH_W(GUESTREGS->CR(n), STATEBK->cr[n]);

    FETCH_HW(lhcpu, STATEBK->lhcpu);
    /*
     * If this is not the last host cpu that dispatched this state
     * descriptor then clear the guest TLB entries
     */
    if (regs->cpuad != lhcpu
     || SIE_STATE(GUESTREGS) != effective_addr2)
    {
        SIE_PERFMON(SIE_PERF_ENTER_F);

        /* Absolute address of state descriptor block */
        SIE_STATE(GUESTREGS) = effective_addr2;

        /* Update Last Host CPU address */
        STORE_HW(STATEBK->lhcpu, regs->cpuad);

    }
    /* Purge guest TLB entries */
    /* FIXME - Force TLB/ALB purge of guest context since there
       seem to be some areas in hercules where the guest TLB/ALB
       is not properly maintained */
    ARCH_DEP(purge_tlb) (GUESTREGS);
    ARCH_DEP(purge_alb) (GUESTREGS);

    /* Initialize interrupt mask and state */
    SET_IC_MASK(GUESTREGS);
    SET_IC_INITIAL_STATE(GUESTREGS);
    SET_IC_PER(GUESTREGS);

    /* Initialize accelerated address lookup values */
    SET_AEA_MODE(GUESTREGS);
    SET_AEA_COMMON(GUESTREGS);
    INVALIDATE_AIA(GUESTREGS);

    GUESTREGS->tracing = regs->tracing;

    /*
     * Do setjmp(progjmp) because translate_addr() may result in
     * longjmp(progjmp) for addressing exceptions.
     */
    if(!setjmp(GUESTREGS->progjmp))
    {
        /*
         * Set sie_active to 1.  This means other threads may now
         * access guestregs when holding intlock.
         * This is the *only* place sie_active is set to one.
         */
        OBTAIN_INTLOCK(regs);
        regs->sie_active = 1;
        RELEASE_INTLOCK(regs);

        /* Get PSA pointer and ensure PSA is paged in */
        if(GUESTREGS->sie_pref)
        {
            GUESTREGS->psa = (PSA_3XX*)(GUESTREGS->mainstor + GUESTREGS->PX);
            GUESTREGS->sie_px = GUESTREGS->PX;
        }
        else
        {
            if (ARCH_DEP(translate_addr) (GUESTREGS->sie_mso + GUESTREGS->PX,
                                          USE_PRIMARY_SPACE, regs, ACCTYPE_SIE))
            {
                SIE_SET_VI(SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT,
                           SIE_VI_WHY_PFACC, GUESTREGS);
                STATEBK->c = SIE_C_VALIDITY;
                OBTAIN_INTLOCK(regs);
                regs->sie_active = 0;
                RELEASE_INTLOCK(regs);
                return;
            }

            /* Convert host real address to host absolute address */
            GUESTREGS->sie_px = APPLY_PREFIXING (regs->dat.raddr, regs->PX);

            if (regs->dat.protect || GUESTREGS->sie_px > regs->mainlim)
            {
                SIE_SET_VI(SIE_VI_WHO_CPU, SIE_VI_WHEN_SIENT,
                           SIE_VI_WHY_PFACC, GUESTREGS);
                STATEBK->c = SIE_C_VALIDITY;
                OBTAIN_INTLOCK(regs);
                regs->sie_active = 0;
                RELEASE_INTLOCK(regs);
                return;
            }
            GUESTREGS->psa = (PSA_3XX*)(GUESTREGS->mainstor + GUESTREGS->sie_px);
        }

        /* Intialize guest timers */
        OBTAIN_INTLOCK(regs);

        /* CPU timer */
        if (CPU_TIMER( GUESTREGS ) < 0)
            ON_IC_PTIMER( GUESTREGS );

        /* Clock comparator */
        if( TOD_CLOCK(GUESTREGS) > GUESTREGS->clkc )
            ON_IC_CLKC(GUESTREGS);

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /* Interval timer if S/370 and timer is enabled */
        if((STATEBK->m & SIE_M_370) && !(STATEBK->m & SIE_M_ITMOF))
        {
            S32 itimer, olditimer;
            U32 residue;

            /* Set the interval timer pending according to the T bit
               in the state control */
            if(STATEBK->s & SIE_S_T)
                ON_IC_ITIMER(GUESTREGS);

            /* Fetch the residu from the state descriptor */
            FETCH_FW(residue,STATEBK->residue);

            /* Fetch the timer value from location 80 */
            FETCH_FW(olditimer,GUESTREGS->psa->inttimer);

            /* Bit position 23 of the interval timer is decremented
               once for each multiple of 3,333 usecs containded in
               bit position 0-19 of the residue counter */
            itimer = olditimer - ((residue / 3333) >> 4);

            /* Store the timer back */
            STORE_FW(GUESTREGS->psa->inttimer, itimer);

            /* Set interrupt flag and interval timer interrupt pending
               if the interval timer went from positive to negative */
            if (itimer < 0 && olditimer >= 0)
                ON_IC_ITIMER(GUESTREGS);
        }

#endif /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

        RELEASE_INTLOCK(regs);

        /* Early exceptions associated with the guest load_psw() */
        if(icode)
            GUESTREGS->program_interrupt (GUESTREGS, icode);

        /* Run SIE in guests architecture mode */
        icode = run_sie[GUESTREGS->arch_mode] (regs);

    } /* if (setjmp(GUESTREGS->progjmp)) */

    ARCH_DEP(sie_exit) (regs, icode);

    /* Perform serialization and checkpoint synchronization */
    PERFORM_SERIALIZATION (regs);
    PERFORM_CHKPT_SYNC (regs);

    longjmp(regs->progjmp, SIE_NO_INTERCEPT);
}


/* Exit SIE state, restore registers and update the state descriptor */
void ARCH_DEP(sie_exit) (REGS *regs, int code)
{
int     n;

    if(pttclass & PTT_CL_SIE)
    {
    U32  nt1 = 0, nt2 = 0;
    BYTE *ip;
        if(!GUESTREGS->instinvalid)
        {
            if(GUESTREGS->ip[0] == 0x44
#if defined( FEATURE_035_EXECUTE_EXTN_FACILITY )
               || (GUESTREGS->ip[0] == 0xc6 && !(GUESTREGS->ip[1] & 0x0f))
#endif
                                                                           )
            {
                ip = GUESTREGS->exinst;
                nt2 = (GUESTREGS->ip[0] == 0x44) ? 0x44 : ((0xc6 << 8) | (GUESTREGS->ip[1] & 0x0f));
            }
            else
                ip = GUESTREGS->ip;
            nt1 = (ip[0] << 24) | (ip[1] << 16);
            if(ILC(ip[0]) > 2)
               nt1 |= (ip[2] << 8) | ip[3];
            if(ILC(ip[0]) > 4)
               nt2 |= (ip[4] << 24) | (ip[5] << 16);
        }

        PTT_SIE("*SIE", nt1, nt2, code);
    }

#if defined( SIE_DEBUG )
    LOGMSG( "SIE: interception code %d\n", code );
    ARCH_DEP( display_inst )( GUESTREGS, GUESTREGS->instinvalid ? NULL : GUESTREGS->ip );
#endif

    SIE_PERFMON(SIE_PERF_EXIT);
    SIE_PERFMON(SIE_PERF_PGMINT);

    /* Indicate we have left SIE mode */
    OBTAIN_INTLOCK(regs);
    regs->sie_active = 0;
    RELEASE_INTLOCK(regs);

    /* zeroize interception status */
    STATEBK->f = 0;

    switch(code > 0 ? code & ~PGM_PER_EVENT : code)
    {
        case SIE_HOST_INTERRUPT:
           /* If a host interrupt is pending
              then backup the psw and exit */
            SET_PSW_IA(regs);
            UPD_PSW_IA (regs, regs->psw.IA -REAL_ILC(regs));
            break;
        case SIE_HOST_PGMINT:
            break;
        case SIE_INTERCEPT_INST:
            STATEBK->c = SIE_C_INST;
            break;
        case SIE_INTERCEPT_PER:
            STATEBK->f |= SIE_F_IF;
            /*fallthru*/
        case SIE_INTERCEPT_INSTCOMP:
            STATEBK->c = SIE_C_PGMINST;
            break;
        case SIE_INTERCEPT_WAIT:
            STATEBK->c = SIE_C_WAIT;
            break;
        case SIE_INTERCEPT_STOPREQ:
            STATEBK->c = SIE_C_STOPREQ;
            break;
        case SIE_INTERCEPT_IOREQ:
            STATEBK->c = SIE_C_IOREQ;
            break;
        case SIE_INTERCEPT_EXTREQ:
            STATEBK->c = SIE_C_EXTREQ;
            break;
        case SIE_INTERCEPT_EXT:
            STATEBK->c = SIE_C_EXTINT;
            break;
        case SIE_INTERCEPT_VALIDITY:
            STATEBK->c = SIE_C_VALIDITY;
            break;
        case SIE_INTERCEPT_IOINT:
        case SIE_INTERCEPT_IOINTP:
            STATEBK->c = SIE_C_IOINT;
            break;
        case SIE_INTERCEPT_IOINST:
            STATEBK->c = SIE_C_IOINST;
            break;
        case PGM_OPERATION_EXCEPTION:
            STATEBK->c = SIE_C_OPEREXC;
            break;
        default:
            STATEBK->c = SIE_C_PGMINT;
            break;
    }

    /* Save CPU timer  */
    STORE_DW(STATEBK->cputimer, cpu_timer(GUESTREGS));

    /* Save clock comparator */
    STORE_DW(STATEBK->clockcomp, etod2tod(GUESTREGS->clkc));

#if defined( _FEATURE_INTERVAL_TIMER ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    /* If this is a S/370 guest, and the interval timer is enabled
       then save the timer state control bit */
    if( (STATEBK->m & SIE_M_370)
     && !(STATEBK->m & SIE_M_ITMOF))
    {
        /* Save the shadow interval timer */
        s370_store_int_timer(regs);

        if(IS_IC_ITIMER(GUESTREGS))
            STATEBK->s |= SIE_S_T;
        else
            STATEBK->s &= ~SIE_S_T;
    }
#endif /* defined( _FEATURE_INTERVAL_TIMER ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

    /* Save TOD Programmable Field */
    STORE_HW(STATEBK->todpf, GUESTREGS->todpr);

    /* Save GR14 */
    STORE_W(STATEBK->gr14, GUESTREGS->GR(14));

    /* Save GR15 */
    STORE_W(STATEBK->gr15, GUESTREGS->GR(15));

    /* Store the PSW */
    if(GUESTREGS->arch_mode == ARCH_390_IDX)
        s390_store_psw (GUESTREGS, STATEBK->psw);
#if defined( _370 ) || defined( _900 )
    else
#endif
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        z900_store_psw (GUESTREGS, STATEBK->psw);
#else
#if defined( _370 )
        s370_store_psw (GUESTREGS, STATEBK->psw);
#endif
#endif

    /* save control registers */
    for(n = 0;n < 16; n++)
        STORE_W(STATEBK->cr[n], GUESTREGS->CR(n));

    /* Update the approprate host registers */
    memcpy(regs->gr, GUESTREGS->gr, 14 * sizeof(regs->gr[0]));
    memcpy(regs->ar, GUESTREGS->ar, 16 * sizeof(regs->ar[0]));
    memcpy(regs->fpr, GUESTREGS->fpr, 32 * sizeof(regs->fpr[0]));
#if defined( FEATURE_BINARY_FLOATING_POINT )
    regs->fpc =  GUESTREGS->fpc;
#endif
    INVALIDATE_AIA(regs);
    SET_AEA_MODE(regs);

    /* Zeroize the interruption parameters */
    memset(STATEBK->ipa, 0, 10);

    if( STATEBK->c == SIE_C_INST
     || STATEBK->c == SIE_C_PGMINST
     || STATEBK->c == SIE_C_OPEREXC
     || STATEBK->c == SIE_C_IOINST )
    {
        /* Indicate interception format 2 */
        STATEBK->f |= SIE_F_IN;

#if defined( _FEATURE_PER )
        /* Handle PER or concurrent PER event */
        if( OPEN_IC_PER(GUESTREGS)
          && ECMODE(&GUESTREGS->psw)
          && (GUESTREGS->psw.sysmask & PSW_PERMODE) )
        {
        PSA *psa;
#if defined( _FEATURE_PER2 )
            GUESTREGS->perc |= OPEN_IC_PER(GUESTREGS) >> ((32 - IC_CR9_SHIFT) - 16);
            /* Positions 14 and 15 contain zeros if a storage alteration
               event was not indicated */
            if( !(OPEN_IC_PER_SA(GUESTREGS))
              || (OPEN_IC_PER_STURA(GUESTREGS)) )
                GUESTREGS->perc &= 0xFFFC;

#endif /* defined( _FEATURE_PER2 ) */
            /* Point to PSA fields in state descriptor */
            psa = (void*)(regs->mainstor + SIE_STATE(GUESTREGS) + SIE_IP_PSA_OFFSET);
            STORE_HW(psa->perint, GUESTREGS->perc);
            STORE_W(psa->peradr, GUESTREGS->peradr);
        }

        if (IS_IC_PER_IF(GUESTREGS))
            STATEBK->f |= SIE_F_IF;

        /* Reset any pending PER indication */
        OFF_IC_PER(GUESTREGS);
#endif /* defined( _FEATURE_PER ) */

        /* Backup to the previous instruction */
        GUESTREGS->ip -= REAL_ILC(GUESTREGS);
        if (GUESTREGS->ip < GUESTREGS->aip)
            GUESTREGS->ip = GUESTREGS->inst;

        /* Update interception parameters in the state descriptor */
        if(GUESTREGS->ip[0] == 0x44
#if defined( FEATURE_035_EXECUTE_EXTN_FACILITY )
           || (GUESTREGS->ip[0] == 0xc6 && !(GUESTREGS->ip[1] & 0x0f))
#endif
                                                                       )
        {
        int exilc;
            STATEBK->f |= SIE_F_EX;
            exilc = ILC(GUESTREGS->exinst[0]);
            memcpy(STATEBK->ipa, GUESTREGS->exinst, exilc);
        }
        else
        {
            if(!GUESTREGS->instinvalid)
                memcpy(STATEBK->ipa, GUESTREGS->ip, ILC(GUESTREGS->ip[0]));
        }

    }

}
#endif /*defined(FEATURE_INTERPRETIVE_EXECUTION)*/


#if defined( _FEATURE_SIE )

/* Execute guest instructions... */

static int ARCH_DEP( run_sie )( REGS* regs )
{
    BYTE*  ip;      /* instruction pointer        */
    int    icode;   /* SIE longjmp intercept code */
    int    i;
    const INSTR_FUNC*  current_opcode_table;

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

        if (!(icode = setjmp( GUESTREGS->progjmp )))
        {
            do
            {
                SIE_PERFMON( SIE_PERF_RUNLOOP_2 );

                /* Set `execflag' to 0 in case EXecuted instruction did progjmp */
                GUESTREGS->execflag = 0;

                if (0
                    || SIE_I_STOP ( GUESTREGS )
                    || SIE_I_EXT  ( GUESTREGS )
                    || SIE_I_IO   ( GUESTREGS )
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

                    if (OPEN_IC_EXTPENDING( GUESTREGS ))
                        ARCH_DEP( perform_external_interrupt )( GUESTREGS );

                    if (1
                        && (0
                            || (STATEBK->ec[0] & SIE_EC0_IOA)
                            || (STATEBK->ec[3] & SIE_EC3_SIGAA)
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
                                     wait_condition( &sysblk.sync_bc_cond, &sysblk.intlock );
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

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) && defined( OPTION_TXF_SLOWLOOP )
                if (GUESTREGS->txf_tnd)
                    goto slowloop;
#endif
                ip = INSTRUCTION_FETCH( GUESTREGS, 0 );
                current_opcode_table = GUESTREGS->ARCH_DEP( runtime_opcode_xxxx );

#if defined( SIE_DEBUG )
                ARCH_DEP( display_inst )( GUESTREGS, GUESTREGS->instinvalid ? NULL : ip );
#endif
                SIE_PERFMON( SIE_PERF_EXEC );

                EXECUTE_INSTRUCTION( current_opcode_table, ip, GUESTREGS );
                regs->instcount++;
                UPDATE_SYSBLK_INSTCOUNT( 1 );

                SIE_PERFMON( SIE_PERF_EXEC_U );

                /* BHe: I have tried several settings. But 2 unrolled
                   executes gives (core i7 at my place) the best results.

                   Even a 'do { } while(0);' with several unrolled executes
                   and without the 'i' was slower.

                   That surprised me.
                */
                for (i=0; i < 128; i++)
                {
#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) && defined( OPTION_TXF_SLOWLOOP )
                    if (GUESTREGS->txf_tnd)
                        break;
#endif
                    UNROLLED_EXECUTE( current_opcode_table, GUESTREGS );

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) && defined( OPTION_TXF_SLOWLOOP )
                    if (GUESTREGS->txf_tnd)
                        break;
#endif
                    UNROLLED_EXECUTE( current_opcode_table, GUESTREGS );
                }
                regs->instcount += 1 + (i * 2);

                /* Update system-wide sysblk.instcount instruction counter */
                UPDATE_SYSBLK_INSTCOUNT( 1 + (i * 2) );

                /* Perform automatic instruction tracing if it's enabled */
                do_automatic_tracing();
                goto endloop;

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) && defined( OPTION_TXF_SLOWLOOP )

slowloop:
                ip = INSTRUCTION_FETCH( GUESTREGS, 0 );
                current_opcode_table = GUESTREGS->ARCH_DEP( runtime_opcode_xxxx );

#if defined( SIE_DEBUG )
                ARCH_DEP( display_inst )( GUESTREGS, GUESTREGS->instinvalid ? NULL : ip );
#endif
                SIE_PERFMON( SIE_PERF_EXEC );

                TXF_EXECUTE_INSTRUCTION( current_opcode_table, ip, GUESTREGS );
                regs->instcount++;
                UPDATE_SYSBLK_INSTCOUNT( 1 );

                SIE_PERFMON( SIE_PERF_EXEC_U );

                /* BHe: I have tried several settings. But 2 unrolled
                   executes gives (core i7 at my place) the best results.

                   Even a 'do { } while(0);' with several unrolled executes
                   and without the 'i' was slower.

                   That surprised me.
                */
                for (i=0; i < 128; i++)
                {
                    if (!GUESTREGS->txf_tnd)
                        break;

                    TXF_UNROLLED_EXECUTE( current_opcode_table, GUESTREGS );

                    if (!GUESTREGS->txf_tnd)
                        break;

                    TXF_UNROLLED_EXECUTE( current_opcode_table, GUESTREGS );
                }
                regs->instcount += 1 + (i * 2);

                /* Update system-wide sysblk.instcount instruction counter */
                UPDATE_SYSBLK_INSTCOUNT( 1 + (i * 2) );

                /* Perform automatic instruction tracing if it's enabled */
                do_automatic_tracing();

#endif /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) && defined( OPTION_TXF_SLOWLOOP ) */

endloop:        ; // (nop to make compiler happy)
            }
            /******************************************/
            /* Remain in SIE (above loop) until ...   */
            /*  - A Host Interrupt is made pending    */
            /*  - A Sie defined irpt becomes enabled  */
            /*  - A guest interrupt is made pending   */
            /******************************************/
            while (unlikely
            (1
                && !SIE_I_HOST            (    regs   )
                && !SIE_I_WAIT            ( GUESTREGS )
                && !SIE_I_EXT             ( GUESTREGS )
                && !SIE_I_IO              ( GUESTREGS )
                && !SIE_INTERRUPT_PENDING ( GUESTREGS )
            ));
        }

        if (!icode || SIE_NO_INTERCEPT == icode)
        {
            /* Check PER first, higher priority */
            if (OPEN_IC_PER( GUESTREGS ))
                ARCH_DEP( program_interrupt )( GUESTREGS, PGM_PER_EVENT );

                 if (SIE_I_EXT  ( GUESTREGS )) icode = SIE_INTERCEPT_EXTREQ;
            else if (SIE_I_IO   ( GUESTREGS )) icode = SIE_INTERCEPT_IOREQ;
            else if (SIE_I_STOP ( GUESTREGS )) icode = SIE_INTERCEPT_STOPREQ;
            else if (SIE_I_WAIT ( GUESTREGS )) icode = SIE_INTERCEPT_WAIT;
            else if (SIE_I_HOST (   regs    )) icode = SIE_HOST_INTERRUPT;
        }
    }
    while (!icode || icode == SIE_NO_INTERCEPT);

    return icode;
} /* end function run_sie */


#if defined(FEATURE_INTERPRETIVE_EXECUTION)
#if defined(FEATURE_REGION_RELOCATE)
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
#endif /*defined(FEATURE_REGION_RELOCATE)*/


#if defined(FEATURE_IO_ASSIST)
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

    /* Obtain the device lock */
    obtain_lock (&dev->lock);

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

    /* Release the device lock */
    release_lock (&dev->lock);
}
#endif /*defined(FEATURE_IO_ASSIST)*/
#endif /*defined(FEATURE_INTERPRETIVE_EXECUTION)*/


#endif


#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "sie.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "sie.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
