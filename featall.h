/* FEATALL.H    (C) Copyright Jan Jaeger, 2000-2012                  */
/*              (C) and others 2013-2023                             */
/*              Architecture-dependent macro definitions             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*               Default OPTIONs and FEATUREs                        */
/*    *ALL* existing FEATUREs *must* be #undef-ed further below.     */
/*-------------------------------------------------------------------*/

#if !defined( OPTION_370_MODE ) && !defined( NO_370_MODE )
#define OPTION_370_MODE                 /* Generate S/370 support    */
#endif

#if !defined( OPTION_390_MODE ) && !defined( NO_390_MODE )
#define OPTION_390_MODE                 /* Generate ESA/390 support  */
#endif

#if !defined( OPTION_900_MODE ) && !defined( NO_900_MODE )
#define OPTION_900_MODE                 /* Generate z/Arch support   */
#endif

/*-------------------------------------------------------------------*/
/*               Research/Workaround build options                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The following build options either fix or workaround a problem    */
/* we were having where we were unsure exactly what was causing the  */
/* problem (and thus were unsure whether the implemented workaround  */
/* is proper/correct or not) and are thus technically *temporary*    */
/* in nature/spirit until such time as the root cause can be found.  */
/*                                                                   */
/* They're implemented as #define build OPTIONS so the developer is  */
/* able to see both what the original code WAS doing as well as how  */
/* the fix/workaround was implemented. This allow us, at any time,   */
/* to disable the fix/workaround and restore the original code if    */
/* we think we might have discovered the root cause of the original  */
/* problem and wish to test a possible permanent fix for it.         */
/*                                                                   */
/*-------------------------------------------------------------------*/

#define OPTION_USE_SKAIP_AS_LOCK        // Use SKAIP as lock, not RCP
#define OPTION_SIE2BK_FLD_COPY          // SIE2BK 'fld' is NOT a mask
#define OPTION_IODELAY_KLUDGE           // IODELAY kludge for Linux
#define OPTION_MVS_TELNET_WORKAROUND    // Handle non-std MVS telnet
#define OPTION_SIE_PURGE_DAT_ALWAYS     // Ivan 2016-07-30: purge DAT
                                        // ALWAYS at start SIE mode
#define OPTION_NOASYNC_SF_CMDS          // Bypass bug in cache logic
                                        // (see GitHub Issue #618!)

/*-------------------------------------------------------------------*/
/*              Normal default OPTIONs and FEATUREs                  */
/*-------------------------------------------------------------------*/

//efine OPTION_SKEY_ABS_CHECK           /* skey debugging option     */
//efine OPTION_ATOMIC_SKEYS             /* Update skeys atomically   */

#define VECTOR_SECTION_SIZE         128 /* Vector section size       */
#define VECTOR_PARTIAL_SUM_NUMBER     1 /* Vector partial sum number */

#define CKD_MAXFILES                 27 /* Max files per CKD volume  */

#define PANEL_REFRESH_RATE_MIN    (1000 / CLK_TCK)  /* (likely 1ms!) */
#define PANEL_REFRESH_RATE_MAX     5000 /* Arbitrary, but reasonable */
#define PANEL_REFRESH_RATE_FAST      50 /* Fast refresh rate (msecs) */
#define PANEL_REFRESH_RATE_SLOW     500 /* Slow refresh rate (msecs) */

#define MIN_TOD_UPDATE_USECS         50 /* Min TOD updt freq (usecs) */
#define DEF_TOD_UPDATE_USECS         50 /* Def TOD updt freq (usecs) */
#define MAX_TOD_UPDATE_USECS     999999 /* Max TOD updt freq (usecs) */

#define MAX_DEVICE_THREAD_IDLE_SECS 300 /* 5 Minute thread timeout   */
//efine OPTION_LONG_HOSTINFO            /* Detailed host & logo info */
#undef  OPTION_FOOTPRINT_BUFFER /* 2048 ** Size must be a power of 2 */
#undef  OPTION_INSTR_COUNT_AND_TIME     /* First use trace and count */
#undef  MODEL_DEPENDENT_STCM            /* STCM, STCMH always store  */
#define OPTION_NOP_MODEL158_DIAGNOSE    /* NOP mod 158 specific diags*/

#define FEATURE_ALD_FORMAT            0 /* Use fmt0 Access-lists     */
#define FEATURE_SIE_MAXZONES          8 /* Maximum SIE Zones         */
#define FEATURE_LCSS_MAX              4 /* Number of supported lcss's*/
//efine SIE_DEBUG_PERFMON               /* SIE performance monitor   */

#define OPTION_SINGLE_CPU_DW            /* Performance option (ia32) */

#if !defined( OPTION_OPTINST ) && !defined( NO_OPTINST )
#define OPTION_OPTINST                  /* Optimized instructions    */
#endif
#define OPTION_NO_E3_OPTINST            /* Problematic!              */

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

#if defined( OPTION_WATCHDOG ) && defined( OPTION_NO_WATCHDOG )
  #error Either OPTION_WATCHDOG or OPTION_NO_WATCHDOG must be specified, not both
#elif !defined( OPTION_WATCHDOG ) && !defined( OPTION_NO_WATCHDOG )
  #define OPTION_WATCHDOG
#endif

#define OPTION_HARDWARE_SYNC_ALL        // All PERFORM_SERIALIZATION
//#define OPTION_HARDWARE_SYNC_BCR_ONLY   // ONLY the BCR instructions
#if defined( OPTION_HARDWARE_SYNC_ALL ) && defined( OPTION_HARDWARE_SYNC_BCR_ONLY )
  #error OPTION_HARDWARE_SYNC_ALL and OPTION_HARDWARE_SYNC_BCR_ONLY are mutually exclusive!
#endif

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
#if defined( OPTION_370_MODE ) && defined( NO_370_MODE )
  #undef     OPTION_370_MODE
#endif
#if defined( OPTION_390_MODE ) && defined( NO_390_MODE )
  #undef     OPTION_390_MODE
#endif
#if defined( OPTION_900_MODE ) && defined( NO_900_MODE )
  #undef     OPTION_900_MODE
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
#undef  FEATURE_002_ZARCH_ACTIVE_FACILITY
#undef  FEATURE_003_DAT_ENHANCE_FACILITY_1
#undef  FEATURE_004_IDTE_SC_SEGTAB_FACILITY
#undef  FEATURE_005_IDTE_SC_REGTAB_FACILITY
#undef  FEATURE_006_ASN_LX_REUSE_FACILITY
#undef  FEATURE_007_STFL_EXTENDED_FACILITY
#undef  FEATURE_008_ENHANCED_DAT_FACILITY_1
#undef  FEATURE_009_SENSE_RUN_STATUS_FACILITY
#undef  FEATURE_010_CONDITIONAL_SSKE_FACILITY
#undef  FEATURE_011_CONFIG_TOPOLOGY_FACILITY
#undef  FEATURE_013_IPTE_RANGE_FACILITY
#undef  FEATURE_014_NONQ_KEY_SET_FACILITY
#undef  FEATURE_016_EXT_TRANSL_FACILITY_2
#undef  FEATURE_017_MSA_FACILITY
#undef  DYNINST_017_MSA_FACILITY                           /*dyncrypt*/
#undef  FEATURE_018_LONG_DISPL_INST_FACILITY
#undef  FEATURE_019_LONG_DISPL_HPERF_FACILITY
#undef  FEATURE_020_HFP_MULT_ADD_SUB_FACILITY
#undef  FEATURE_021_EXTENDED_IMMED_FACILITY
#undef  FEATURE_022_EXT_TRANSL_FACILITY_3
#undef  FEATURE_023_HFP_UNNORM_EXT_FACILITY
#undef  FEATURE_024_ETF2_ENHANCEMENT_FACILITY
#undef  FEATURE_025_STORE_CLOCK_FAST_FACILITY
#undef  FEATURE_026_PARSING_ENHANCE_FACILITY
#undef  FEATURE_027_MVCOS_FACILITY
#undef  FEATURE_028_TOD_CLOCK_STEER_FACILITY
#undef  FEATURE_030_ETF3_ENHANCEMENT_FACILITY
#undef  FEATURE_031_EXTRACT_CPU_TIME_FACILITY
#undef  FEATURE_032_CSS_FACILITY
#undef  FEATURE_033_CSS_FACILITY_2
#undef  FEATURE_034_GEN_INST_EXTN_FACILITY
#undef  FEATURE_035_EXECUTE_EXTN_FACILITY
#undef  FEATURE_036_ENH_MONITOR_FACILITY
#undef  FEATURE_037_FP_EXTENSION_FACILITY
#undef  FEATURE_038_OP_CMPSC_FACILITY
#undef  FEATURE_040_LOAD_PROG_PARAM_FACILITY
#undef  FEATURE_041_FPS_ENHANCEMENT_FACILITY                    /*DFP*/
#undef  FEATURE_041_DFP_ROUNDING_FACILITY
#undef  FEATURE_041_FPR_GR_TRANSFER_FACILITY
#undef  FEATURE_041_FPS_SIGN_HANDLING_FACILITY
#undef  FEATURE_041_IEEE_EXCEPT_SIM_FACILITY
#undef  FEATURE_042_DFP_FACILITY                                /*DFP*/
#undef  FEATURE_043_DFP_HPERF_FACILITY
#undef  FEATURE_044_PFPO_FACILITY
#undef  FEATURE_045_DISTINCT_OPERANDS_FACILITY
#undef  FEATURE_045_FAST_BCR_SERIAL_FACILITY
#undef  FEATURE_045_HIGH_WORD_FACILITY
#undef  FEATURE_045_INTERLOCKED_ACCESS_FACILITY_1
#undef  FEATURE_045_LOAD_STORE_ON_COND_FACILITY_1
#undef  FEATURE_045_POPULATION_COUNT_FACILITY
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
#undef  FEATURE_061_MISC_INSTR_EXT_FACILITY_3
#undef  FEATURE_066_RES_REF_BITS_MULT_FACILITY
#undef  FEATURE_067_CPU_MEAS_COUNTER_FACILITY
#undef  FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY
#undef  FEATURE_073_TRANSACT_EXEC_FACILITY
#undef  FEATURE_074_STORE_HYPER_INFO_FACILITY
#undef  FEATURE_075_ACC_EX_FS_INDIC_FACILITY
#undef  FEATURE_076_MSA_EXTENSION_FACILITY_3
#undef  DYNINST_076_MSA_EXTENSION_FACILITY_3               /*dyncrypt*/
#undef  FEATURE_077_MSA_EXTENSION_FACILITY_4
#undef  DYNINST_077_MSA_EXTENSION_FACILITY_4               /*dyncrypt*/
#undef  FEATURE_078_ENHANCED_DAT_FACILITY_2
#undef  FEATURE_080_DFP_PACK_CONV_FACILITY
#undef  FEATURE_081_PPA_IN_ORDER_FACILITY
#undef  FEATURE_129_ZVECTOR_FACILITY
#undef  FEATURE_130_INSTR_EXEC_PROT_FACILITY
#undef  FEATURE_131_SIDE_EFFECT_ACCESS_FACILITY
#undef  FEATURE_131_ENH_SUPP_ON_PROT_2_FACILITY
#undef  FEATURE_133_GUARDED_STORAGE_FACILITY
#undef  FEATURE_134_ZVECTOR_PACK_DEC_FACILITY
#undef  FEATURE_135_ZVECTOR_ENH_FACILITY_1
#undef  FEATURE_138_CONFIG_ZARCH_MODE_FACILITY
#undef  FEATURE_139_MULTIPLE_EPOCH_FACILITY
#undef  FEATURE_142_ST_CPU_COUNTER_MULT_FACILITY
#undef  FEATURE_144_TEST_PEND_EXTERNAL_FACILITY
#undef  FEATURE_145_INS_REF_BITS_MULT_FACILITY
#undef  FEATURE_146_MSA_EXTENSION_FACILITY_8
#undef  FEATURE_148_VECTOR_ENH_FACILITY_2
#undef  FEATURE_149_MOVEPAGE_SETKEY_FACILITY
#undef  FEATURE_150_ENH_SORT_FACILITY
#undef  FEATURE_151_DEFLATE_CONV_FACILITY
#undef  FEATURE_152_VECT_PACKDEC_ENH_FACILITY
#undef  FEATURE_155_MSA_EXTENSION_FACILITY_9
#undef  FEATURE_158_ULTRAV_CALL_FACILITY
#undef  FEATURE_161_SEC_EXE_UNPK_FACILITY
#undef  FEATURE_165_NNET_ASSIST_FACILITY
#undef  FEATURE_168_ESA390_COMPAT_MODE_FACILITY
#undef  FEATURE_169_SKEY_REMOVAL_FACILITY
#undef  FEATURE_192_VECT_PACKDEC_ENH_2_FACILITY
#undef  FEATURE_193_BEAR_ENH_FACILITY
#undef  FEATURE_194_RESET_DAT_PROT_FACILITY
#undef  FEATURE_196_PROC_ACT_FACILITY
#undef  FEATURE_197_PROC_ACT_EXT_1_FACILITY

/*-------------------------------------------------------------------*/
/*      FEATUREs that DON'T have any facility bits defined           */
/*-------------------------------------------------------------------*/

#undef  FEATURE_2K_STORAGE_KEYS
#undef  FEATURE_370_EXTENSION
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
#undef  FEATURE_DAT_ENHANCEMENT_FACILITY_2
#undef  FEATURE_DUAL_ADDRESS_SPACE
#undef  FEATURE_ECPSVM
#undef  FEATURE_EMULATE_VM
#undef  FEATURE_ENHANCED_SUPPRESSION_ON_PROTECTION
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
#undef  FEATURE_INTERVAL_TIMER
#undef  FEATURE_IO_ASSIST
#undef  FEATURE_LINKAGE_STACK
#undef  FEATURE_LOCK_PAGE
#undef  FEATURE_MSA_EXTENSION_FACILITY_1
#undef  FEATURE_MSA_EXTENSION_FACILITY_2
#undef  FEATURE_MIDAW_FACILITY
#undef  FEATURE_MOVE_PAGE_FACILITY_2
#undef  FEATURE_MPF_INFO
#undef  FEATURE_MSSF_CALL
#undef  FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE
#undef  FEATURE_MVS_ASSIST
#undef  FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS     // 'N' instructions
#undef  FEATURE_PAGE_PROTECTION
#undef  FEATURE_PER
#undef  FEATURE_PER1
#undef  FEATURE_PER2
#undef  FEATURE_PER3
#undef  FEATURE_PER_STORAGE_KEY_ALTERATION_FACILITY
#undef  FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY
#undef  FEATURE_PERFORM_LOCKED_OPERATION
#undef  FEATURE_PRIVATE_SPACE
#undef  FEATURE_PROGRAM_DIRECTED_REIPL
#undef  FEATURE_PROTECTION_INTERCEPTION_CONTROL
#undef  FEATURE_QDIO_TDD
#undef  FEATURE_QDIO_THININT
#undef  FEATURE_QEBSM
#undef  FEATURE_QUEUED_DIRECT_IO
#undef  FEATURE_REGION_RELOCATE
#undef  FEATURE_RESTORE_SUBCHANNEL_FACILITY
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
#undef  FEATURE_SIE
#undef  FEATURE_SQUARE_ROOT
#undef  FEATURE_STORAGE_KEY_ASSIST
#undef  FEATURE_STORAGE_PROTECTION_OVERRIDE
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
#undef  FEATURE_TCPIP_EXTENSION
#undef  FEATURE_ZVM_ESSA

/* end of FEATALL.H */
