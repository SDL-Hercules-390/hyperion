/* FEATCHK.H    (c) Copyright Jan Jaeger, 2000-2012                  */
/*              Feature definition consistency checks                */

/*-------------------------------------------------------------------*/
/*  Perform various checks on feature combinations, and set          */
/*  additional flags to percolate certain features such as           */
/*  SIE down to lower architecture levels such that these            */
/*  can include emulation support                                    */
/*                                                                   */
/*  FEATURE_XXXX is defined per architecture mode, and               */
/*  _FEATURE_XXXX is defined for ALL architecture modes when         */
/*  FEATURE_XXXX is defined for ANY architecture mode.               */
/*                                                                   */
/*-------------------------------------------------------------------*/

#if defined(FEATCHK_CHECK_ALL)

/* FEATURE_INTERPRETIVE_EXECUTION is related to host related issues
   _FEATURE_SIE is related to guest (emulation) related issues.  This
   because if FEATURE_INTERPRETIVE_EXECUTION is defined for say 390
   mode, then _FEATURE_SIE will also need to be in 370 in order to
   support 370 mode SIE emulation */
#if defined(FEATURE_INTERPRETIVE_EXECUTION)
 #define _FEATURE_SIE
 #if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
  #define _FEATURE_ZSIE
 #endif
 #if defined(FEATURE_PROTECTION_INTERCEPTION_CONTROL)
  #define _FEATURE_PROTECTION_INTERCEPTION_CONTROL
 #endif
#endif

/* _FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE is used for host
   related processing issues, FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE
   is defined only in ESA/390 mode. MCDS is an ESA/390
   feature that is supported under z/Architecture SIE */
#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)
 #define _FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE
#endif

#if defined(FEATURE_2K_STORAGE_KEYS)
 #define _FEATURE_2K_STORAGE_KEYS
#endif

#if defined(FEATURE_INTERVAL_TIMER)
 #define _FEATURE_INTERVAL_TIMER
#endif

#if defined(FEATURE_ECPSVM)
 #define _FEATURE_ECPSVM
#endif

#if defined(FEATURE_S370_S390_VECTOR_FACILITY)
 #define   _FEATURE_S370_S390_VECTOR_FACILITY
#endif

#if defined(FEATURE_CHANNEL_SUBSYSTEM)
 #define _FEATURE_CHANNEL_SUBSYSTEM
#endif

#if defined(FEATURE_SYSTEM_CONSOLE)
 #define _FEATURE_SYSTEM_CONSOLE
#endif

#if defined(FEATURE_EXPANDED_STORAGE)
 #define _FEATURE_EXPANDED_STORAGE
#endif

#if defined(FEATURE_ECPSVM)
 #define _FEATURE_ECPSVM
#endif

#if defined(_FEATURE_SIE) && defined(FEATURE_STORAGE_KEY_ASSIST)
 #define _FEATURE_STORAGE_KEY_ASSIST
#endif

#if defined(FEATURE_CPU_RECONFIG)
 #define _FEATURE_CPU_RECONFIG
#endif

#if defined(FEATURE_PER)
 #define _FEATURE_PER
#endif

#if defined(FEATURE_PER2)
 #define _FEATURE_PER2
#endif

#if defined(FEATURE_EXPEDITED_SIE_SUBSET)
 #define _FEATURE_EXPEDITED_SIE_SUBSET
#endif

#if defined(FEATURE_REGION_RELOCATE)
 #define _FEATURE_REGION_RELOCATE
#endif

#if defined(FEATURE_IO_ASSIST)
 #define _FEATURE_IO_ASSIST
#endif

#if defined(FEATURE_WAITSTATE_ASSIST)
 #define _FEATURE_WAITSTATE_ASSIST
#endif

#if defined(FEATURE_EXTERNAL_INTERRUPT_ASSIST)
 #define _FEATURE_EXTERNAL_INTERRUPT_ASSIST
#endif

#if defined(FEATURE_017_MSA_FACILITY)
 #define _FEATURE_017_MSA_FACILITY
#endif

#if defined(FEATURE_006_ASN_LX_REUSE_FACILITY)
 #define _FEATURE_006_ASN_LX_REUSE_FACILITY
#endif

#if defined(FEATURE_INTEGRATED_3270_CONSOLE)
 #define _FEATURE_INTEGRATED_3270_CONSOLE
#endif

#if defined(FEATURE_INTEGRATED_ASCII_CONSOLE)
 #define _FEATURE_INTEGRATED_ASCII_CONSOLE
#endif

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
 #define _FEATURE_001_ZARCH_INSTALLED_FACILITY
#endif

#if defined(FEATURE_000_N3_INSTR_FACILITY)
 #define _FEATURE_000_N3_INSTR_FACILITY
#endif

#if defined(FEATURE_003_DAT_ENHANCE_FACILITY_1)
 #define _FEATURE_003_DAT_ENHANCE_FACILITY_1
#endif

#if defined(FEATURE_007_STFL_EXTENDED_FACILITY)
 #define _FEATURE_007_STFL_EXTENDED_FACILITY
#endif

#if defined(FEATURE_008_ENHANCED_DAT_FACILITY_1)
 #define _FEATURE_008_ENHANCED_DAT_FACILITY_1
#endif

#if defined(FEATURE_009_SENSE_RUN_STATUS_FACILITY)
 #define _FEATURE_009_SENSE_RUN_STATUS_FACILITY
#endif

#if defined(FEATURE_010_CONDITIONAL_SSKE_FACILITY)
 #define _FEATURE_010_CONDITIONAL_SSKE_FACILITY
#endif

#if defined(FEATURE_011_CONFIG_TOPOLOGY_FACILITY)
 #define _FEATURE_011_CONFIG_TOPOLOGY_FACILITY
#endif

#if defined(FEATURE_013_IPTE_RANGE_FACILITY)
 #define _FEATURE_013_IPTE_RANGE_FACILITY
#endif

#if defined(FEATURE_014_NONQ_KEY_SET_FACILITY)
 #define _FEATURE_014_NONQ_KEY_SET_FACILITY
#endif

#if defined(FEATURE_016_EXT_TRANSL_FACILITY_2)
 #define _FEATURE_016_EXT_TRANSL_FACILITY_2
#endif

#if defined(FEATURE_017_MSA_FACILITY)
 #define _FEATURE_017_MSA_FACILITY
#endif

#if defined(FEATURE_018_LONG_DISPL_INST_FACILITY)
 #define _FEATURE_018_LONG_DISPL_INST_FACILITY
#endif

#if defined(FEATURE_020_HFP_MULT_ADD_SUB_FACILITY)
 #define FEATURE_020_HFP_MULT_ADD_SUB_FACILITY
#endif

#if defined(FEATURE_021_EXTENDED_IMMED_FACILITY)
 #define _FEATURE_021_EXTENDED_IMMED_FACILITY
#endif

#if defined(FEATURE_022_EXT_TRANSL_FACILITY_3)
 #define _FEATURE_022_EXT_TRANSL_FACILITY_3
#endif

#if defined(FEATURE_023_HFP_UNNORM_EXT_FACILITY)
 #define _FEATURE_023_HFP_UNNORM_EXT_FACILITY
#endif

#if defined(FEATURE_024_ETF2_ENHANCEMENT_FACILITY)
 #define _FEATURE_024_ETF2_ENHANCEMENT_FACILITY
#endif

#if defined(FEATURE_025_STORE_CLOCK_FAST_FACILITY)
 #define _FEATURE_025_STORE_CLOCK_FAST_FACILITY
#endif

#if defined(FEATURE_027_MVCOS_FACILITY)
 #define _FEATURE_027_MVCOS_FACILITY
#endif

#if defined(FEATURE_028_TOD_CLOCK_STEER_FACILITY)
 #define _FEATURE_028_TOD_CLOCK_STEER_FACILITY
#endif

#if defined(FEATURE_030_ETF3_ENHANCEMENT_FACILITY)
 #define _FEATURE_030_ETF3_ENHANCEMENT_FACILITY
#endif

#if defined(FEATURE_031_EXTRACT_CPU_TIME_FACILITY)
 #define _FEATURE_031_EXTRACT_CPU_TIME_FACILITY
#endif

#if defined(FEATURE_032_CSS_FACILITY)
 #define _FEATURE_032_CSS_FACILITY
#endif

#if defined(FEATURE_033_CSS_FACILITY_2)
 #define _FEATURE_033_CSS_FACILITY_2
#endif

#if defined(FEATURE_034_GEN_INST_EXTN_FACILITY)
 #define _FEATURE_034_GEN_INST_EXTN_FACILITY
#endif

#if defined(FEATURE_035_EXECUTE_EXTN_FACILITY)
 #define _FEATURE_035_EXECUTE_EXTN_FACILITY
#endif

#if defined(FEATURE_036_ENH_MONITOR_FACILITY)
 #define _FEATURE_036_ENH_MONITOR_FACILITY
#endif

#if defined(FEATURE_040_LOAD_PROG_PARAM_FACILITY)
 #define _FEATURE_040_LOAD_PROG_PARAM_FACILITY
#endif

#if defined(FEATURE_041_FPS_ENHANCEMENTS_FACILITY)
 #define _FEATURE_041_FPS_ENHANCEMENTS_FACILITY
#endif

#if defined(FEATURE_042_DECIMAL_FLOAT_FACILITY)
 #define _FEATURE_042_DECIMAL_FLOAT_FACILITY
#endif

#if defined(FEATURE_044_PFPO_FACILITY)
 #define _FEATURE_044_PFPO_FACILITY
#endif

#if defined(FEATURE_045_FAST_BCR_SERIAL_FACILITY)
 #define _FEATURE_045_FAST_BCR_SERIAL_FACILITY
#endif

#if defined(FEATURE_066_RES_REF_BITS_MULT_FACILITY)
 #define _FEATURE_066_RES_REF_BITS_MULT_FACILITY
#endif

#if defined(FEATURE_067_CPU_MEAS_COUNTER_FACILITY)
 #define _FEATURE_067_CPU_MEAS_COUNTER_FACILITY
#endif

#if defined(FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY)
 #define _FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY
#endif

#if defined(FEATURE_075_ACC_EX_FS_INDIC_FACILITY)
 #define _FEATURE_075_ACC_EX_FS_INDIC_FACILITY
#endif

#if defined(FEATURE_MSA_EXTENSION_FACILITY_1)
 #define _FEATURE_MSA_EXTENSION_FACILITY_1
#endif

#if defined(FEATURE_MSA_EXTENSION_FACILITY_2)
 #define _FEATURE_MSA_EXTENSION_FACILITY_2
#endif

#if defined(FEATURE_076_MSA_EXTENSION_FACILITY_3)
 #define _FEATURE_076_MSA_EXTENSION_FACILITY_3
#endif

#if defined(FEATURE_077_MSA_EXTENSION_FACILITY_4)
 #define   _FEATURE_077_MSA_EXTENSION_FACILITY_4
#endif

#if defined(FEATURE_VM_BLOCKIO)
 #define _FEATURE_VM_BLOCKIO
#endif

#if defined(FEATURE_QEBSM)
 #define _FEATURE_QEBSM
#endif

#if defined(FEATURE_QDIO_THININT)
 #define _FEATURE_QDIO_THININT
#endif

#if defined(FEATURE_SVS)
 #define _FEATURE_SVS
#endif

#if defined(FEATURE_QDIO_TDD)
 #define _FEATURE_QDIO_TDD
#endif

#if defined(FEATURE_HYPERVISOR)
 #define _FEATURE_HYPERVISOR
#endif

#if defined(FEATURE_SCSI_IPL)
 #define _FEATURE_SCSI_IPL
#endif

#if defined(FEATURE_HARDWARE_LOADER)
 #define _FEATURE_HARDWARE_LOADER
#endif

#if defined(FEATURE_HERCULES_DIAGCALLS)
 #define _FEATURE_HERCULES_DIAGCALLS
 #if defined(FEATURE_HOST_RESOURCE_ACCESS_FACILITY)
  #define _FEATURE_HOST_RESOURCE_ACCESS_FACILITY
 #endif
#endif

#if defined(FEATURE_EMULATE_VM)
 #define _FEATURE_EMULATE_VM
#endif

#if defined(FEATURE_047_CMPSC_ENH_FACILITY)
 #define   _FEATURE_047_CMPSC_ENH_FACILITY
#endif

#undef _VSTORE_C_STATIC
#if defined(OPTION_INLINE_VSTORE)
 #define _VSTORE_C_STATIC           static inline
 #define _VSTORE_FULL_C_STATIC      static
#else
 #ifndef _VSTORE_C
  #ifndef _HENGINE_DLL_
   #define _VSTORE_C_STATIC         DLL_IMPORT
   #define _VSTORE_FULL_C_STATIC    DLL_IMPORT
  #else
   #define _VSTORE_C_STATIC         extern
   #define _VSTORE_FULL_C_STATIC    extern
  #endif
 #else
  #define  _VSTORE_C_STATIC         DLL_EXPORT
  #define  _VSTORE_FULL_C_STATIC    DLL_EXPORT
 #endif
#endif

#undef _VFETCH_C_STATIC
#if defined(OPTION_INLINE_IFETCH)
 #define _VFETCH_C_STATIC           static inline
#else
 #define _VFETCH_C_STATIC
#endif

#undef _DAT_C_STATIC
#if defined(OPTION_INLINE_DAT)
 #define _DAT_C_STATIC              static inline
#else
 #define _DAT_C_STATIC
#endif

#undef _LOGICAL_C_STATIC
#if defined(OPTION_INLINE_LOGICAL)
 #define _LOGICAL_C_STATIC          static inline
#else
 #ifndef _DAT_C
  #ifndef _HENGINE_DLL_
   #define _LOGICAL_C_STATIC        DLL_IMPORT
  #else
   #define _LOGICAL_C_STATIC        extern
  #endif
 #else
  #define  _LOGICAL_C_STATIC        DLL_EXPORT
 #endif
#endif

/* _XXX (_370, _390, _900) are architectures */
/*      present in the build                 */
/* _ARCH_XXX are the index for each arch     */
/*      within the decode table              */
/* _ARCHMODEX are indicators for self        */
/*      inclusion for architecture dependent */
/*      compilation units                    */

#if !defined(OPTION_370_MODE) \
  && !defined(OPTION_390_MODE) \
  && !defined(OPTION_900_MODE)
 #error No Architecture mode
#endif
#if defined(OPTION_370_MODE)
 #define _370
 #define _ARCHMODE1 370
 #define ARCH_370 0
#endif

#if defined(OPTION_390_MODE)
 #define _390
 #if !defined(_ARCHMODE1)
  #define _ARCHMODE1 390
  #define ARCH_390 0
 #else
  #define _ARCHMODE2 390
  #define ARCH_390 1
 #endif
#endif

#if defined(OPTION_900_MODE)
 #define _900
 #if !defined(_ARCHMODE2)
  #define _ARCHMODE2 900
  #define ARCH_900 1
 #else
  #define _ARCHMODE3 900
  #define ARCH_900 2
 #endif
#endif

#if !defined(ARCH_370)
 #define ARCH_370 -1
#endif
#if !defined(ARCH_390)
 #define ARCH_390 -1
#endif
#if !defined(ARCH_900)
 #define ARCH_900 -1
#endif

/* Change this if more entries in the opcode tables */
/* are needed for disassembly                       */
#define INSTRUCTION_DECODE_ENTRIES      2

#if defined(_ARCHMODE3)
 #define GEN_ARCHCOUNT  3
#elif defined(_ARCHMODE2)
 #define GEN_ARCHCOUNT  2
#else
 #define GEN_ARCHCOUNT  1
#endif

/* FIXME : GEN_MAXARCH is misnamed */
/*        Should be something like */
/*        OPCODE_TABLE_ENTRY_SIZE  */
#define GEN_MAXARCH     GEN_ARCHCOUNT+INSTRUCTION_DECODE_ENTRIES

#if defined(_900) && !defined(_390)
 #error OPTION_390_MODE must be enabled for OPTION_900_MODE
#endif


#else /*!defined(FEATCHK_CHECK_ALL)*/

/* When ESAME is installed then all instructions
   marked N3 in the reference are also available
   in ESA/390 mode */
#if defined(_900) && (__GEN_ARCH == 390)
 #define FEATURE_000_N3_INSTR_FACILITY
#endif

#if !defined(FEATURE_2K_STORAGE_KEYS) \
 && !defined(FEATURE_4K_STORAGE_KEYS)
 #error Storage keys must be 2K or 4K
#endif

#if defined(FEATURE_2K_STORAGE_KEYS) \
 && defined(FEATURE_4K_STORAGE_KEYS)
 #error Storage keys must be 2K or 4K
#endif

#if defined(FEATURE_EXTENDED_STORAGE_KEYS)
 #if !defined(FEATURE_S370E_EXTENDED_ADDRESSING)
  #define FEATURE_S370E_EXTENDED_ADDRESSING
 #endif
#endif

#if defined(FEATURE_EXPANDED_STORAGE) \
 && !defined(FEATURE_4K_STORAGE_KEYS)
 #error Expanded storage cannot be defined with 2K storage keys
#endif

#if defined(_900) && defined(FEATURE_S370_S390_VECTOR_FACILITY)
 #error Vector Facility not supported on ESAME capable processors
#endif

#if !defined(FEATURE_S370_CHANNEL) && !defined(FEATURE_CHANNEL_SUBSYSTEM)
 #error Either S/370 Channel or Channel Subsystem must be defined
#endif

#if defined(FEATURE_S370_CHANNEL) && defined(FEATURE_CHANNEL_SUBSYSTEM)
 #error S/370 Channel and Channel Subsystem cannot both be defined
#endif

#if defined(FEATURE_CANCEL_IO_FACILITY) \
 && !defined(FEATURE_CHANNEL_SUBSYSTEM)
 #error Cancel I/O facility requires Channel Subsystem
#endif

#if defined(FEATURE_MOVE_PAGE_FACILITY_2) \
 && !defined(FEATURE_4K_STORAGE_KEYS)
 #error Move page facility cannot be defined with 2K storage keys
#endif

#if defined(FEATURE_FAST_SYNC_DATA_MOVER) \
 && !defined(FEATURE_MOVE_PAGE_FACILITY_2)
 #error Fast sync data mover facility requires Move page facility
#endif

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY) \
  && defined(FEATURE_INTERPRETIVE_EXECUTION) \
  && !defined(_FEATURE_SIE)
 #error ESA/390 SIE must be defined when defining ESAME SIE
#endif

#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE) \
 && !defined(FEATURE_INTERPRETIVE_EXECUTION)
 #error MCDS only supported with SIE
#endif

#if defined(FEATURE_PROTECTION_INTERCEPTION_CONTROL) \
 && !defined(FEATURE_INTERPRETIVE_EXECUTION)
 #error Protection Interception Control only supported with SIE
#endif

#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE) \
 && !defined(FEATURE_STORAGE_KEY_ASSIST)
 #error MCDS requires storage key assist
#endif

#if defined(FEATURE_SIE) && defined(FEATURE_001_ZARCH_INSTALLED_FACILITY) \
 && !defined(FEATURE_STORAGE_KEY_ASSIST)
 #error ESAME SIE requires storage key assist
#endif

#if defined(FEATURE_STORAGE_KEY_ASSIST) \
 && !defined(FEATURE_INTERPRETIVE_EXECUTION)
 #error Storage Key assist only supported with SIE
#endif

#if defined(FEATURE_REGION_RELOCATE) \
 && !defined(FEATURE_INTERPRETIVE_EXECUTION)
 #error Region Relocate Facility only supported with SIE
#endif

#if defined(FEATURE_IO_ASSIST) \
 && !defined(_FEATURE_SIE)
 #error I/O Assist Feature only supported with SIE
#endif

#if defined(FEATURE_IO_ASSIST) \
 && !defined(_FEATURE_REGION_RELOCATE)
 #error Region Relocate Facility required for IO Assist
#endif

#if defined(FEATURE_EXTERNAL_INTERRUPT_ASSIST) \
 && !defined(_FEATURE_SIE)
 #error External Interruption assist only supported with SIE
#endif

#if defined(FEATURE_EXPEDITED_SIE_SUBSET) \
 && !defined(_FEATURE_SIE)
 #error Expedited SIE Subset only supported with SIE
#endif

#if defined(FEATURE_006_ASN_LX_REUSE_FACILITY)
 #if !defined(FEATURE_DUAL_ADDRESS_SPACE)
  #error ASN-and-LX-Reuse requires Dual Address-Space feature
 #endif
 #if !defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
  #error ASN-and-LX-Reuse is only supported with ESAME
 #endif
#endif

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY) \
 && defined(FEATURE_S370_S390_VECTOR_FACILITY)
 #error Vector Facility not supported in ESAME mode
#endif

#if defined(FEATURE_BINARY_FLOATING_POINT) \
 && defined(NO_IEEE_SUPPORT)
 #undef FEATURE_037_FP_EXTENSIONS_FACILITY
 #undef FEATURE_BINARY_FLOATING_POINT
 #undef FEATURE_FPS_EXTENSIONS
#endif

#if defined(FEATURE_BINARY_FLOATING_POINT) \
 && !defined(FEATURE_BASIC_FP_EXTENSIONS)
 #error Binary floating point requires basic FP extensions
#endif

#if defined(FEATURE_042_DECIMAL_FLOAT_FACILITY) \
 && !defined(FEATURE_BASIC_FP_EXTENSIONS)
 #error Decimal floating point requires basic FP extensions
#endif

#if defined(FEATURE_BASIC_FP_EXTENSIONS) \
 && !defined(FEATURE_HEXADECIMAL_FLOATING_POINT)
 #error Basic FP extensions requires hexadecimal floating point
#endif

#if !defined(FEATURE_BASIC_FP_EXTENSIONS)
 #if defined(FEATURE_HFP_EXTENSIONS) \
  || defined(FEATURE_FPS_EXTENSIONS)
  #error Floating point extensions require basic FP extensions
 #endif
#endif

#if defined(FEATURE_FPS_EXTENSIONS) \
 && !defined(FEATURE_BINARY_FLOATING_POINT)
 #error FP support extensions requires binary floating point
#endif

#if defined(FEATURE_020_HFP_MULT_ADD_SUB_FACILITY) \
 && !defined(FEATURE_HEXADECIMAL_FLOATING_POINT)
 #error HFP multiply add/subtract requires hexadecimal floating point
#endif

#if defined(FEATURE_023_HFP_UNNORM_EXT_FACILITY) \
 && !defined(FEATURE_HEXADECIMAL_FLOATING_POINT)
 #error HFP unnormalized extension requires hexadecimal floating point
#endif

#if defined(FEATURE_037_FP_EXTENSIONS_FACILITY) \
 && !defined(FEATURE_BINARY_FLOATING_POINT)
 #error Floating point extension facility requires binary floating point
#endif

#if defined(FEATURE_PER2) && !defined(FEATURE_PER)
 #error FEATURE_PER must be defined when using FEATURE_PER2
#endif

#if defined(FEATURE_PER3) && !defined(FEATURE_PER)
 #error FEATURE_PER must be defined when using FEATURE_PER3
#endif

#if defined(FEATURE_033_CSS_FACILITY_2) && !defined(FEATURE_032_CSS_FACILITY)
 #error FEATURE_032_CSS_FACILITY must be defined when using FEATURE_033_CSS_FACILITY_2
#endif

#if defined(FEATURE_SCSI_IPL) && !defined(FEATURE_HARDWARE_LOADER)
 #error SCSI IPL requires FEATURE_HARDWARE_LOADER
#endif

#if defined(FEATURE_INTEGRATED_3270_CONSOLE) && !defined(FEATURE_SYSTEM_CONSOLE)
 #error Integrated 3270 console requires FEATURE_SYSTEM_CONSOLE
#endif

#if defined(FEATURE_INTEGRATED_ASCII_CONSOLE) && !defined(FEATURE_SYSTEM_CONSOLE)
 #error Integrated ASCII console requires FEATURE_SYSTEM_CONSOLE
#endif

#if defined(FEATURE_VM_BLOCKIO) && !defined(FEATURE_EMULATE_VM)
 #error VM Standard Block I/O DIAGNOSE 0x250 requires FEATURE_EMULATE_VM
#endif

#if defined(FEATURE_HOST_RESOURCE_FACILITY) && !defined(_FEATURE_HERCULES_DIAGCALLS)
 #error Hercules Host Resource Access DIAGNOSE 0xF18 requires FEATURE_HERCULES_DIAGCALLS
#endif

#if defined(FEATURE_017_MSA_FACILITY)
 #if defined(_370)
  #define _370_FEATURE_017_MSA_FACILITY
 #endif
 #if defined(_390)
  #define _390_FEATURE_017_MSA_FACILITY
 #endif
 #if defined(_900)
  #define _900_FEATURE_017_MSA_FACILITY
 #endif
#endif /*defined(FEATURE_017_MSA_FACILITY)*/

#if defined(FEATURE_007_STFL_EXTENDED_FACILITY)
  #if !defined(FEATURE_STORE_FACILITY_LIST)
    #error STFLE requires STFL (FEATURE_007_STFL_EXTENDED_FACILITY requires FEATURE_STORE_FACILITY_LIST)
  #endif
#endif

#if defined(FEATURE_075_ACC_EX_FS_INDIC_FACILITY)         /*810*/
  #if !defined(FEATURE_ENHANCED_SUPPRESSION_ON_PROTECTION)
    #error Access-Exception Fetch/Store Indication facility requires Enhanced Suppression on Protection
  #endif
#endif

#if defined(FEATURE_ENHANCED_SUPPRESSION_ON_PROTECTION)              /*208*/
  #if !defined(FEATURE_SUPPRESSION_ON_PROTECTION)
    #error Enhanced Suppression on Protection facility requires Suppression on Protection
  #endif
#endif

#if defined(FEATURE_067_CPU_MEAS_COUNTER_FACILITY)\
    || defined(FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY)
  #if !defined(FEATURE_040_LOAD_PROG_PARAM_FACILITY)
    #error CPU Measurement facilities requires Load Program Parameter facility
  #endif
#endif

#endif /*!defined(FEATALL_CHECKALL)*/

/* end of FEATCHK.H */
