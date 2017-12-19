/* FEATALL.H    (c) Copyright Jan Jaeger, 2000-2012                  */
/*              Architecture-dependent macro definitions             */

/*-------------------------------------------------------------------*/
/*               Default OPTIONs and FEATUREs                        */
/*    *ALL* existing FEATUREs *must* be #undef-ed further below.     */
/*-------------------------------------------------------------------*/

#if !defined(OPTION_370_MODE) && !defined(NO_370_MODE)
#define OPTION_370_MODE                 /* Generate S/370 support    */
#endif

#if !defined(OPTION_370_EXTENSION) && !defined(NO_370_EXTENSION)
#define OPTION_370_EXTENSION            /* S/370 backport of S/390 & z/arch */
#endif

#if !defined(OPTION_390_MODE) && !defined(NO_390_MODE)
#define OPTION_390_MODE                 /* Generate ESA/390 support  */
#endif

#if !defined(OPTION_900_MODE) && !defined(NO_900_MODE)
#define OPTION_900_MODE                 /* Generate z/Arch support   */
#endif

#if !defined(OPTION_LPP_RESTRICT) && !defined(NO_LPP_RESTRICT)
#define OPTION_LPP_RESTRICT             /* Disable Licensed Software */
#endif

#if !defined(OPTION_SMP) && !defined(NO_SMP)
#define OPTION_SMP                      /* Enable SMP support        */
#endif

#define VECTOR_SECTION_SIZE         128 /* Vector section size       */
#define VECTOR_PARTIAL_SUM_NUMBER     1 /* Vector partial sum number */

#define CKD_MAXFILES                 27 /* Max files per CKD volume  */

#if !defined(OPTION_MIPS_COUNTING) && !defined(NO_MIPS_COUNTING)
#define OPTION_MIPS_COUNTING            /* Display MIPS on ctl panel */
#endif

#define PANEL_REFRESH_RATE              /* Enable panrate feature    */
#define PANEL_REFRESH_RATE_FAST      50 /* Fast refresh rate (msecs) */
#define PANEL_REFRESH_RATE_SLOW     500 /* Slow refresh rate (msecs) */

#ifdef _MSVC_                           /*        (Windows)          */
#define MIN_TOD_UPDATE_USECS        1   /* Min TOD updt freq (usecs) */
#define DEF_TOD_UPDATE_USECS       50   /* Def TOD updt freq (usecs) */
#define MAX_TOD_UPDATE_USECS  1000000   /* Max TOD updt freq (usecs) */
#else                                   /*      (non-Windows)        */
#define MIN_TOD_UPDATE_USECS        1   /* Min TOD updt freq (usecs) */
#define DEF_TOD_UPDATE_USECS       50   /* Def TOD updt freq (usecs) */
#define MAX_TOD_UPDATE_USECS  1000000   /* Max TOD updt freq (usecs) */
#endif

#define MAX_DEVICE_THREAD_IDLE_SECS 300 /* 5 Minute thread timeout   */

/*-------------------------------------------------------------------*/
/*   The following set of default "OPTION_INLINE" settings were      */
/*   determined based on testing performed in late May of 2013:      */
/*      vstore/ifetch: inline,  logical/dat: DON'T inline.           */
/*-------------------------------------------------------------------*/

#if defined(    OPTION_INLINE_VSTORE ) && defined(  NO_OPTION_INLINE_VSTORE )
  #error Either OPTION_INLINE_VSTORE or NO_OPTION_INLINE_VSTORE must be specified, not both
#elif !defined( OPTION_INLINE_VSTORE ) && !defined( NO_OPTION_INLINE_VSTORE )
  #define       OPTION_INLINE_VSTORE
#endif
#if defined(    OPTION_INLINE_IFETCH ) && defined(  NO_OPTION_INLINE_IFETCH )
  #error Either OPTION_INLINE_IFETCH or NO_OPTION_INLINE_IFETCH must be specified, not both
#elif !defined( OPTION_INLINE_IFETCH ) && !defined( NO_OPTION_INLINE_IFETCH )
  #define       OPTION_INLINE_IFETCH
#endif
#if defined(    OPTION_INLINE_LOGICAL ) && defined(  NO_OPTION_INLINE_LOGICAL )
  #error Either OPTION_INLINE_LOGICAL or NO_OPTION_INLINE_LOGICAL must be specified, not both
#elif !defined( OPTION_INLINE_LOGICAL ) && !defined( NO_OPTION_INLINE_LOGICAL )
  #undef        OPTION_INLINE_LOGICAL
#endif
#if defined(    OPTION_INLINE_DAT ) && defined(  NO_OPTION_INLINE_DAT )
  #error Either OPTION_INLINE_DAT or NO_OPTION_INLINE_DAT must be specified, not both
#elif !defined( OPTION_INLINE_DAT ) && !defined( NO_OPTION_INLINE_DAT )
  #undef        OPTION_INLINE_DAT
#endif
/*-------------------------------------------------------------------*/

#define OPTION_SINGLE_CPU_DW            /* Performance option (ia32) */
#define OPTION_IODELAY_KLUDGE           /* IODELAY kludge for linux  */
#define OPTION_MVS_TELNET_WORKAROUND    /* Handle non-std MVS telnet */
//efine OPTION_LONG_HOSTINFO            /* Detailed host & logo info */

#undef  OPTION_FOOTPRINT_BUFFER /* 2048 ** Size must be a power of 2 */
#undef  OPTION_INSTRUCTION_COUNTING     /* First use trace and count */
#define OPTION_CKD_KEY_TRACING          /* Trace CKD search keys     */
#undef  MODEL_DEPENDENT_STCM            /* STCM, STCMH always store  */
#define OPTION_NOP_MODEL158_DIAGNOSE    /* NOP mod 158 specific diags*/
#define FEATURE_ALD_FORMAT            0 /* Use fmt0 Access-lists     */
#define FEATURE_SIE_MAXZONES          8 /* Maximum SIE Zones         */
#define FEATURE_LCSS_MAX              4 /* Number of supported lcss's*/
//efine SIE_DEBUG_PERFMON               /* SIE performance monitor   */
#define OPTION_HTTP_SERVER              /* HTTP server support       */

#if !defined(OPTION_SCP_MSG_PREFIX) && !defined(NO_SCP_MSG_PREFIX)
#define NO_SCP_MSG_PREFIX               /* Prefix scp msg with HHC*  */
#endif

#if !defined(OPTION_WINDOWS_HOST_FILENAMES) && !defined(NO_WINDOWS_HOST_FILENAMES)
#define  NO_WINDOWS_HOST_FILENAMES      /* Format files for display
                                           in native host format
                                           slashes                   */
#endif

#if !defined(OPTION_SHUTDOWN_CONFIRMATION) && !defined(NO_SHUTDOWN_CONFIRMATION)
#define  NO_SHUTDOWN_CONFIRMATION       /* Confirm quit and ssd cmds */
#endif

#define OPTION_OPTINST                  /* Optimized instructions    */

#if !defined(ENABLE_CONFIG_INCLUDE) && !defined(NO_CONFIG_INCLUDE)
#define  ENABLE_CONFIG_INCLUDE          /* enable config file includes */
#endif

#if !defined(ENABLE_SYSTEM_SYMBOLS) && !defined(NO_SYSTEM_SYMBOLS)
#define  ENABLE_SYSTEM_SYMBOLS          /* access to system symbols  */
#endif

#if !defined(ENABLE_BUILTIN_SYMBOLS) && !defined(NO_BUILTIN_SYMBOLS)
#define  ENABLE_BUILTIN_SYMBOLS          /* Internal Symbols          */
#endif

#if defined(ENABLE_BUILTIN_SYMBOLS) && !defined(ENABLE_SYSTEM_SYMBOLS)
  #error ENABLE_BUILTIN_SYMBOLS requires ENABLE_SYMBOLS_SYMBOLS
#endif

#if defined( HAVE_FULL_KEEPALIVE )
  #if !defined( HAVE_PARTIAL_KEEPALIVE ) || !defined( HAVE_BASIC_KEEPALIVE )
    #error Cannot have full TCP keepalive without partial and basic as well
  #endif
#elif defined( HAVE_PARTIAL_KEEPALIVE )
  #if !defined( HAVE_BASIC_KEEPALIVE )
    #error Cannot have partial TCP keepalive without basic as well
  #endif
#endif

#if (CKD_MAXFILES > 35)
  #error CKD_MAXFILES can not exceed design limit of 35
#endif

#if defined( OPTION_SHARED_DEVICES ) && defined( OPTION_NO_SHARED_DEVICES )
  #error Either OPTION_SHARED_DEVICES or OPTION_NO_SHARED_DEVICES must be specified, not both
#elif !defined( OPTION_SHARED_DEVICES ) && !defined( OPTION_NO_SHARED_DEVICES )
  // Neither is #defined.  Use default settings.
  #define OPTION_SHARED_DEVICES
  #define FBA_SHARED
#elif defined( OPTION_NO_SHARED_DEVICES )
  #undef OPTION_SHARED_DEVICES
  #undef FBA_SHARED
#elif defined( OPTION_SHARED_DEVICES )
// Leave FBA_SHARED alone, either #defined or #undefined, as desired.
#endif // OPTION_SHARED_DEVICES

/*-------------------------------------------------------------------*/
/*                  Hercules Mutex Locks Model                       */
/*-------------------------------------------------------------------*/

#define  OPTION_MUTEX_NORMAL       1    /* re-obtain == deadlock     */
#define  OPTION_MUTEX_ERRORCHECK   2    /* re-obtain == error        */
#define  OPTION_MUTEX_RECURSIVE    3    /* re-obtain == allowed      */

#define  OPTION_RWLOCK_SHARED      4    /* public to all processes   */
#define  OPTION_RWLOCK_PRIVATE     5    /* private to this process   */

#define  OPTION_MUTEX_DEFAULT      OPTION_MUTEX_ERRORCHECK
#define  OPTION_RWLOCK_DEFAULT     OPTION_RWLOCK_PRIVATE

/*********************************************************************/
/*                                                                   */
/*                            NOTE!                                  */
/*                                                                   */
/*     All HOST-operating-system-specific features and options       */
/*     should be #defined in the below header (and ONLY in the       */
/*     below header!) Please read the comments there!                */
/*                                                                   */
/*********************************************************************/

#include "hostopts.h"     // (HOST-specific options/feature settings)

// (allow for compiler command-line overrides...)
#if defined(OPTION_370_MODE) && defined(NO_370_MODE)
  #undef    OPTION_370_MODE
#endif
#if defined(OPTION_390_MODE) && defined(NO_390_MODE)
  #undef    OPTION_390_MODE
#endif
#if defined(OPTION_900_MODE) && defined(NO_900_MODE)
  #undef    OPTION_900_MODE
#endif

/*********************************************************************/
/*                                                                   */
/*                            NOTE!                                  */
/*                                                                   */
/*       *ALL* 'FEATURE' constants *MUST* be #undef-ed below!        */
/*                                                                   */
/*********************************************************************/

/*-------------------------------------------------------------------*/
/*      FEATUREs with STFL/STFLE facility bits defined               */
/*-------------------------------------------------------------------*/

#undef  FEATURE_000_N3_INSTR_FACILITY
#undef  FEATURE_001_ZARCH_INSTALLED_FACILITY
#undef  FEATURE_002_ZARCH_ACTIVE_FACILITY       /* NOT A COMPILE-TIME CONSTANT! */
#undef  FEATURE_003_DAT_ENHANCE_FACILITY_1
#undef  FEATURE_004_IDTE_SC_SEGTAB_FACILITY
#undef  FEATURE_005_IDTE_SC_REGTAB_FACILITY
#undef  FEATURE_006_ASN_LX_REUSE_FACILITY
#undef  FEATURE_007_STFL_EXTENDED_FACILITY                      /*@Z9*/
#undef  FEATURE_008_ENHANCED_DAT_FACILITY_1                     /*208*/
#undef  FEATURE_009_SENSE_RUN_STATUS_FACILITY                   /*@Z9*/
#undef  FEATURE_010_CONDITIONAL_SSKE_FACILITY                   /*407*/
#undef  FEATURE_011_CONFIG_TOPOLOGY_FACILITY                    /*208*/
#undef  FEATURE_013_IPTE_RANGE_FACILITY                         /*810*/
#undef  FEATURE_014_NONQ_KEY_SET_FACILITY                       /*810*/
#undef  FEATURE_016_EXT_TRANSL_FACILITY_2
#undef  FEATURE_017_MSA_FACILITY
#undef  FEATURE_018_LONG_DISPL_INST_FACILITY
#undef  FEATURE_019_LONG_DISPL_HPERF_FACILITY
#undef  FEATURE_020_HFP_MULT_ADD_SUB_FACILITY
#undef  FEATURE_021_EXTENDED_IMMED_FACILITY                     /*@Z9*/
#undef  FEATURE_022_EXT_TRANSL_FACILITY_3
#undef  FEATURE_023_HFP_UNNORM_EXT_FACILITY                     /*@Z9*/
#undef  FEATURE_024_ETF2_ENHANCEMENT_FACILITY                   /*@Z9*/
#undef  FEATURE_025_STORE_CLOCK_FAST_FACILITY                   /*@Z9*/
#undef  FEATURE_026_PARSING_ENHANCE_FACILITY                    /*208*/
#undef  FEATURE_027_MVCOS_FACILITY                              /*208*/
#undef  FEATURE_028_TOD_CLOCK_STEER_FACILITY                    /*@Z9*/
#undef  FEATURE_030_ETF3_ENHANCEMENT_FACILITY                   /*@Z9*/
#undef  FEATURE_031_EXTRACT_CPU_TIME_FACILITY                   /*407*/
#undef  FEATURE_032_CSS_FACILITY                                /*407*/
#undef  FEATURE_033_CSS_FACILITY_2                              /*208*/
#undef  FEATURE_034_GEN_INST_EXTN_FACILITY
#undef  FEATURE_035_EXECUTE_EXTN_FACILITY                       /*208*/
#undef  FEATURE_036_ENH_MONITOR_FACILITY                        /*810*/
#undef  FEATURE_037_FP_EXTENSION_FACILITY
#undef  FEATURE_038_OP_CMPSC_FACILITY
#undef  FEATURE_040_LOAD_PROG_PARAM_FACILITY
#undef  FEATURE_041_FPS_ENHANCEMENT_FACILITY                    /*DFP*/
#undef  FEATURE_041_DFP_ROUNDING_FACILITY
#undef  FEATURE_041_FPR_GR_TRANSFER_FACILITY
#undef  FEATURE_041_FPS_SIGN_HANDLING_FACILITY
#undef  FEATURE_041_IEEE_EXCEPT_SIM_FACILITY                    /*407*/
#undef  FEATURE_042_DECIMAL_FLOAT_FACILITY                      /*DFP*/
#undef  FEATURE_043_DFP_HPERF_FACILITY
#undef  FEATURE_044_PFPO_FACILITY                               /*407*/
#undef  FEATURE_045_DISTINCT_OPERANDS_FACILITY                  /*810*/
#undef  FEATURE_045_FAST_BCR_SERIAL_FACILITY                    /*810*/
#undef  FEATURE_045_HIGH_WORD_FACILITY                          /*810*/
#undef  FEATURE_045_INTERLOCKED_ACCESS_FACILITY_1               /*810*/
#undef  FEATURE_045_LOAD_STORE_ON_COND_FACILITY_1               /*810*/
#undef  FEATURE_045_POPULATION_COUNT_FACILITY                   /*810*/
#undef  FEATURE_047_CMPSC_ENH_FACILITY
#undef  FEATURE_048_DFP_ZONE_CONV_FACILITY
#undef  FEATURE_049_EXECUTION_HINT_FACILITY
#undef  FEATURE_049_LOAD_AND_TRAP_FACILITY
#undef  FEATURE_049_PROCESSOR_ASSIST_FACILITY
#undef  FEATURE_049_MISC_INSTR_EXT_FACILITY_1
#undef  FEATURE_050_CONSTR_TRANSACT_FACILITY
#undef  FEATURE_051_LOCAL_TLB_CLEARING_FACILITY
#undef  FEATURE_052_INTERLOCKED_ACCESS_FACILITY_2
#undef  FEATURE_053_LOAD_STORE_ON_COND_FACILITY_2
#undef  FEATURE_053_LOAD_ZERO_RIGHTMOST_FACILITY
#undef  FEATURE_054_EE_CMPSC_FACILITY
#undef  FEATURE_057_MSA_EXTENSION_FACILITY_5
#undef  FEATURE_058_MISC_INSTR_EXT_FACILITY_2
#undef  FEATURE_066_RES_REF_BITS_MULT_FACILITY                  /*810*/
#undef  FEATURE_067_CPU_MEAS_COUNTER_FACILITY
#undef  FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY
#undef  FEATURE_073_TRANSACT_EXEC_FACILITY
#undef  FEATURE_074_STORE_HYPER_INFO_FACILITY
#undef  FEATURE_075_ACC_EX_FS_INDIC_FACILITY                    /*810*/
#undef  FEATURE_076_MSA_EXTENSION_FACILITY_3                    /*810*/
#undef  FEATURE_077_MSA_EXTENSION_FACILITY_4                    /*810*/
#undef  FEATURE_078_ENHANCED_DAT_FACILITY_2
#undef  FEATURE_080_DFP_PACK_CONV_FACILITY
#undef  FEATURE_129_ZVECTOR_FACILITY
#undef  FEATURE_130_INSTR_EXEC_PROT_FACILITY
#undef  FEATURE_131_SIDE_EFFECT_ACCESS_FACILITY
#undef  FEATURE_133_GUARDED_STORAGE_FACILITY
#undef  FEATURE_134_ZVECTOR_PACK_DEC_FACILITY
#undef  FEATURE_135_ZVECTOR_ENH_FACILITY_1
#undef  FEATURE_138_CONFIG_ZARCH_MODE_FACILITY
#undef  FEATURE_139_MULTIPLE_EPOCH_FACILITY
#undef  FEATURE_142_ST_CPU_COUNTER_MULT_FACILITY
#undef  FEATURE_144_TEST_PEND_EXTERNAL_FACILITY
#undef  FEATURE_145_INS_REF_BITS_MULT_FACILITY
#undef  FEATURE_146_MSA_EXTENSION_FACILITY_8
#undef  FEATURE_168_ESA390_COMPAT_MODE_FACILITY

/*-------------------------------------------------------------------*/
/*      FEATUREs that DON'T have any facility bits defined           */
/*-------------------------------------------------------------------*/

#undef  FEATURE_2K_STORAGE_KEYS
#undef  FEATURE_4K_STORAGE_KEYS
#undef  FEATURE_ACCESS_REGISTERS
#undef  FEATURE_ADDRESS_LIMIT_CHECKING
#undef  FEATURE_BASIC_FP_EXTENSIONS
#undef  FEATURE_BASIC_STORAGE_KEYS
#undef  FEATURE_BCMODE
#undef  FEATURE_BIMODAL_ADDRESSING
#undef  FEATURE_BINARY_FLOATING_POINT
#undef  FEATURE_BRANCH_AND_SET_AUTHORITY
#undef  FEATURE_BROADCASTED_PURGING
#undef  FEATURE_CALLED_SPACE_IDENTIFICATION
#undef  FEATURE_CANCEL_IO_FACILITY
#undef  FEATURE_CHANNEL_SUBSYSTEM
#undef  FEATURE_CHANNEL_SWITCHING
#undef  FEATURE_CHECKSUM_INSTRUCTION
#undef  FEATURE_CHSC
#undef  FEATURE_COMPARE_AND_MOVE_EXTENDED
#undef  FEATURE_CMPSC
#undef  FEATURE_CPU_RECONFIG
#undef  FEATURE_DAT_ENHANCEMENT_FACILITY_2                      /*@Z9*/
#undef  FEATURE_DUAL_ADDRESS_SPACE
#undef  FEATURE_ECPSVM
#undef  FEATURE_EMULATE_VM
#undef  FEATURE_ENHANCED_SUPPRESSION_ON_PROTECTION              /*208*/
#undef  FEATURE_EXPANDED_STORAGE
#undef  FEATURE_EXPEDITED_SIE_SUBSET
#undef  FEATURE_EXTENDED_DIAG204
#undef  FEATURE_EXTENDED_STORAGE_KEYS
#undef  FEATURE_EXTENDED_TOD_CLOCK
#undef  FEATURE_EXTENDED_TRANSLATION_FACILITY_1
#undef  FEATURE_EXTERNAL_INTERRUPT_ASSIST
#undef  FEATURE_FAST_SYNC_DATA_MOVER
#undef  FEATURE_FETCH_PROTECTION_OVERRIDE
#undef  FEATURE_FPS_EXTENSIONS
#undef  FEATURE_HARDWARE_LOADER
#undef  FEATURE_HERCULES_DIAGCALLS
#undef  FEATURE_HEXADECIMAL_FLOATING_POINT
#undef  FEATURE_HFP_EXTENSIONS
#undef  FEATURE_HOST_RESOURCE_ACCESS_FACILITY
#undef  FEATURE_HYPERVISOR
#undef  FEATURE_IMMEDIATE_AND_RELATIVE
#undef  FEATURE_INCORRECT_LENGTH_INDICATION_SUPPRESSION
#undef  FEATURE_INTEGRATED_3270_CONSOLE
#undef  FEATURE_INTEGRATED_ASCII_CONSOLE
#undef  FEATURE_INTERPRETIVE_EXECUTION
#undef  FEATURE_INTERVAL_TIMER
#undef  FEATURE_IO_ASSIST
#undef  FEATURE_LINKAGE_STACK
#undef  FEATURE_LOAD_REVERSED
#undef  FEATURE_LOCK_PAGE
#undef  FEATURE_MSA_EXTENSION_FACILITY_1                        /*@Z9*/
#undef  FEATURE_MSA_EXTENSION_FACILITY_2
#undef  FEATURE_MIDAW_FACILITY                                  /*@Z9*/
#undef  FEATURE_MOVE_PAGE_FACILITY_2
#undef  FEATURE_MPF_INFO
#undef  FEATURE_MSSF_CALL
#undef  FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE
#undef  FEATURE_MVS_ASSIST
#undef  FEATURE_PAGE_PROTECTION
#undef  FEATURE_PER
#undef  FEATURE_PER2
#undef  FEATURE_PER3                                            /*@Z9*/
#undef  FEATURE_PERFORM_LOCKED_OPERATION
#undef  FEATURE_PRIVATE_SPACE
#undef  FEATURE_PROGRAM_DIRECTED_REIPL                          /*@Z9*/
#undef  FEATURE_PROTECTION_INTERCEPTION_CONTROL
#undef  FEATURE_QDIO_TDD
#undef  FEATURE_QDIO_THININT
#undef  FEATURE_QEBSM
#undef  FEATURE_QUEUED_DIRECT_IO
#undef  FEATURE_REGION_RELOCATE
#undef  FEATURE_RESTORE_SUBCHANNEL_FACILITY                     /*208*/
#undef  FEATURE_RESUME_PROGRAM
#undef  FEATURE_S370_CHANNEL
#undef  FEATURE_S370_S390_VECTOR_FACILITY
#undef  FEATURE_S370E_EXTENDED_ADDRESSING
#undef  FEATURE_S390_DAT
#undef  FEATURE_SCEDIO
#undef  FEATURE_SCSI_IPL
#undef  FEATURE_SEGMENT_PROTECTION
#undef  FEATURE_SERVICE_PROCESSOR
#undef  FEATURE_SET_ADDRESS_SPACE_CONTROL_FAST
#undef  FEATURE_SQUARE_ROOT
#undef  FEATURE_STORAGE_KEY_ASSIST
#undef  FEATURE_STORAGE_PROTECTION_OVERRIDE
#undef  FEATURE_STORE_FACILITY_LIST
#undef  FEATURE_STORE_SYSTEM_INFORMATION
#undef  FEATURE_STRING_INSTRUCTION
#undef  FEATURE_SUBSPACE_GROUP
#undef  FEATURE_SUPPRESSION_ON_PROTECTION
#undef  FEATURE_SVS
#undef  FEATURE_SYSTEM_CONSOLE
#undef  FEATURE_TEST_BLOCK
#undef  FEATURE_TRACING
#undef  FEATURE_VIRTUAL_ARCHITECTURE_LEVEL
#undef  FEATURE_VM_BLOCKIO
#undef  FEATURE_WAITSTATE_ASSIST

/* end of FEATALL.H */
