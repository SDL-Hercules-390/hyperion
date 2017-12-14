/* FEAT390.H    (c) Copyright Jan Jaeger, 2000-2012                  */
/*              ESA/390 feature definitions                          */

/*-------------------------------------------------------------------*/
/* This file defines the architectural features which are included   */
/* at compilation time for ESA/390 mode                              */
/*-------------------------------------------------------------------*/

/*********************************************************************/
/*                      PROGRAMMING NOTE!                            */
/*        This file MUST *NOT* contain any #undef statements!        */
/*********************************************************************/

#if !defined( OPTION_390_MODE )
#define _ARCH_390_NAME ""
#else
#define _ARCH_390_NAME "ESA/390"

/*-------------------------------------------------------------------*/
/*          FEATUREs with STFL facility bits defined                 */
/*-------------------------------------------------------------------*/

// Note that ESA/390 only supports STFL, not STFLE. Thus there are
// only 32 facility bits that pertain to ESA/390 (bits 0-31). Also
// not that only bits 0-5, 16-20 are currently defined for ESA/390.
// Bits 6-15, 21-31 are reserved and currently stored as zeros.

#define FEATURE_000_N3_INSTR_FACILITY
//efine FEATURE_001_ZARCH_INSTALLED_FACILITY
//efine FEATURE_002_ZARCH_ACTIVE_FACILITY       /* NOT A COMPILE-TIME CONSTANT! */
#define FEATURE_003_DAT_ENHANCE_FACILITY_1
//efine FEATURE_004_IDTE_SC_SEGTAB_FACILITY
//efine FEATURE_005_IDTE_SC_REGTAB_FACILITY
//efine FEATURE_006_ASN_LX_REUSE_FACILITY
//efine FEATURE_007_STFL_EXTENDED_FACILITY
//efine FEATURE_008_ENHANCED_DAT_FACILITY_1                     /*208*/
//efine FEATURE_009_SENSE_RUN_STATUS_FACILITY                   /*@Z9*/
//efine FEATURE_010_CONDITIONAL_SSKE_FACILITY                   /*407*/
//efine FEATURE_011_CONFIG_TOPOLOGY_FACILITY                    /*208*/
//efine FEATURE_013_IPTE_RANGE_FACILITY                         /*810*/
//efine FEATURE_014_NONQ_KEY_SET_FACILITY                       /*810*/
#define FEATURE_016_EXT_TRANSL_FACILITY_2
#define FEATURE_017_MSA_FACILITY
#define FEATURE_018_LONG_DISPL_INST_FACILITY
#define FEATURE_019_LONG_DISPL_HPERF_FACILITY
#define FEATURE_020_HFP_MULT_ADD_SUB_FACILITY
//efine FEATURE_021_EXTENDED_IMMED_FACILITY                     /*@Z9*/
//efine FEATURE_022_EXT_TRANSL_FACILITY_3                       /*@ZA*/
//efine FEATURE_023_HFP_UNNORM_EXT_FACILITY                     /*@Z9*/
//efine FEATURE_024_ETF2_ENHANCEMENT_FACILITY                   /*@ZA*/
//efine FEATURE_025_STORE_CLOCK_FAST_FACILITY                   /*@Z9*/
//efine FEATURE_026_PARSING_ENHANCE_FACILITY                    /*208*/
//efine FEATURE_027_MVCOS_FACILITY                              /*208*/
//efine FEATURE_028_TOD_CLOCK_STEER_FACILITY                    /*@Z9*/
//efine FEATURE_030_ETF3_ENHANCEMENT_FACILITY                   /*@ZA*/
//efine FEATURE_031_EXTRACT_CPU_TIME_FACILITY                   /*407*/

/*-------------------------------------------------------------------*/
/*      FEATUREs that DON'T have any facility bits defined           */
/*-------------------------------------------------------------------*/

#define FEATURE_4K_STORAGE_KEYS
#define FEATURE_ACCESS_REGISTERS
#define FEATURE_ADDRESS_LIMIT_CHECKING
#define FEATURE_BASIC_FP_EXTENSIONS
#define FEATURE_BIMODAL_ADDRESSING
#define FEATURE_BINARY_FLOATING_POINT
#define FEATURE_BRANCH_AND_SET_AUTHORITY
#define FEATURE_BROADCASTED_PURGING
#define FEATURE_CALLED_SPACE_IDENTIFICATION
#define FEATURE_CANCEL_IO_FACILITY              /* comment if FEATURE_S370_CHANNEL used */
#define FEATURE_CHANNEL_SUBSYSTEM               /* comment if FEATURE_S370_CHANNEL used */
//efine FEATURE_CHANNEL_SWITCHING               /* comment if FEATURE_CHANNEL_SUBSYSTEM used */
#define FEATURE_CHECKSUM_INSTRUCTION
#define FEATURE_CHSC
#define FEATURE_COMPARE_AND_MOVE_EXTENDED
#define FEATURE_COMPRESSION
#define FEATURE_CPU_RECONFIG
#define FEATURE_DUAL_ADDRESS_SPACE
#define FEATURE_EMULATE_VM
#define FEATURE_EXPANDED_STORAGE
#define FEATURE_EXPEDITED_SIE_SUBSET
#define FEATURE_EXTENDED_STORAGE_KEYS
#define FEATURE_EXTENDED_TOD_CLOCK
#define FEATURE_EXTENDED_TRANSLATION_FACILITY_1
#define FEATURE_EXTERNAL_INTERRUPT_ASSIST
#define FEATURE_FAST_SYNC_DATA_MOVER
#define FEATURE_FETCH_PROTECTION_OVERRIDE
#define FEATURE_FPS_EXTENSIONS
#define FEATURE_HARDWARE_LOADER
#define FEATURE_HERCULES_DIAGCALLS
#define FEATURE_HEXADECIMAL_FLOATING_POINT
#define FEATURE_HFP_EXTENSIONS
#define FEATURE_HOST_RESOURCE_ACCESS_FACILITY
#define FEATURE_HYPERVISOR
#define FEATURE_IMMEDIATE_AND_RELATIVE
#define FEATURE_INCORRECT_LENGTH_INDICATION_SUPPRESSION
#define FEATURE_INTEGRATED_3270_CONSOLE
//efine FEATURE_INTEGRATED_ASCII_CONSOLE
#define FEATURE_INTERPRETIVE_EXECUTION
#define FEATURE_IO_ASSIST
#define FEATURE_LINKAGE_STACK
#define FEATURE_LOCK_PAGE
#define FEATURE_MSA_EXTENSION_FACILITY_1
#define FEATURE_MSA_EXTENSION_FACILITY_2
#define FEATURE_MOVE_PAGE_FACILITY_2
#define FEATURE_MPF_INFO
#define FEATURE_MSSF_CALL
#define FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE
#define FEATURE_MVS_ASSIST
#define FEATURE_PAGE_PROTECTION
#define FEATURE_PER
#define FEATURE_PER2
#define FEATURE_PERFORM_LOCKED_OPERATION
#define FEATURE_PRIVATE_SPACE
#define FEATURE_PROTECTION_INTERCEPTION_CONTROL
#define FEATURE_QUEUED_DIRECT_IO
#define FEATURE_REGION_RELOCATE
#define FEATURE_RESUME_PROGRAM
//efine FEATURE_S370_CHANNEL                    /* comment if FEATURE_CHANNEL_SUBSYSTEM used */
//efine FEATURE_S370_S390_VECTOR_FACILITY
#define FEATURE_S390_DAT
#define FEATURE_SCEDIO
#define FEATURE_SCSI_IPL
#define FEATURE_SERVICE_PROCESSOR
#define FEATURE_SET_ADDRESS_SPACE_CONTROL_FAST
#define FEATURE_SQUARE_ROOT
#define FEATURE_STORAGE_KEY_ASSIST
#define FEATURE_STORAGE_PROTECTION_OVERRIDE
#define FEATURE_STORE_FACILITY_LIST
#define FEATURE_STORE_SYSTEM_INFORMATION
#define FEATURE_STRING_INSTRUCTION
#define FEATURE_SUBSPACE_GROUP
#define FEATURE_SUPPRESSION_ON_PROTECTION
#define FEATURE_SYSTEM_CONSOLE
#define FEATURE_TEST_BLOCK
#define FEATURE_TRACING
#define FEATURE_VM_BLOCKIO
#define FEATURE_WAITSTATE_ASSIST

#endif /*defined(OPTION_390_MODE)*/

/* end of FEAT390.H */
