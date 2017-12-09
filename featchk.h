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
/*-------------------------------------------------------------------*/

#if defined( FEATCHK_DO_DEFINES )       // (pass 1: do #defines)

/*-------------------------------------------------------------------*/
/*                   PASS 1: remember features                       */
/*-------------------------------------------------------------------*/
/*  In this section of code we simply (for ther most part) #define   */
/*  the underscore '_FEATURE_XXXX' constant if the non-underscored   */
/* 'FEATURE_XXX' constant is #defined.  This allows us to detect     */
/*  the need for a given feature if ANY of the build archtectures    */
/*  should need it.  That is to say, if at least ONE of the build    */
/*  architectures needs support for a given feature, then we must    */
/*  enable the code that provides support for that feature regard-   */
/*  less of whether the current build architecture needs it or not.  */
/*  You should NOT be checking FEATURE combinations here.  That is   */
/*  done during pass 2.  You should only be doing #defines here.     */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*              Some FEATUREs force other FEATUREs                   */
/*-------------------------------------------------------------------*/

#if defined( FEATURE_EXTENDED_STORAGE_KEYS )
  #define    FEATURE_S370E_EXTENDED_ADDRESSING
#endif

/*-------------------------------------------------------------------*/
/*      Some build options override/disable certain FEATUREs         */
/*-------------------------------------------------------------------*/

#if defined( NO_IEEE_SUPPORT )
 #undef FEATURE_037_FP_EXTENSIONS_FACILITY
 #undef FEATURE_BINARY_FLOATING_POINT
 #undef FEATURE_FPS_EXTENSIONS
#endif

/*-------------------------------------------------------------------*/
/*                   Facility-bit FEATUREs                           */
/*-------------------------------------------------------------------*/

#if defined( FEATURE_000_N3_INSTR_FACILITY )
 #define    _FEATURE_000_N3_INSTR_FACILITY
#endif

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
 #define    _FEATURE_001_ZARCH_INSTALLED_FACILITY
#endif

#if defined( FEATURE_003_DAT_ENHANCE_FACILITY_1 )
 #define    _FEATURE_003_DAT_ENHANCE_FACILITY_1
#endif

#if defined( FEATURE_004_IDTE_SC_SEGTAB_FACILITY )
 #define    _FEATURE_004_IDTE_SC_SEGTAB_FACILITY
#endif

#if defined( FEATURE_005_IDTE_SC_REGTAB_FACILITY )
 #define    _FEATURE_005_IDTE_SC_REGTAB_FACILITY
#endif

#if defined( FEATURE_006_ASN_LX_REUSE_FACILITY )
 #define    _FEATURE_006_ASN_LX_REUSE_FACILITY
#endif

#if defined( FEATURE_007_STFL_EXTENDED_FACILITY )
 #define    _FEATURE_007_STFL_EXTENDED_FACILITY
#endif

#if defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 )
 #define    _FEATURE_008_ENHANCED_DAT_FACILITY_1
#endif

#if defined( FEATURE_009_SENSE_RUN_STATUS_FACILITY )
 #define    _FEATURE_009_SENSE_RUN_STATUS_FACILITY
#endif

#if defined( FEATURE_010_CONDITIONAL_SSKE_FACILITY )
 #define    _FEATURE_010_CONDITIONAL_SSKE_FACILITY
#endif

#if defined( FEATURE_011_CONFIG_TOPOLOGY_FACILITY )
 #define    _FEATURE_011_CONFIG_TOPOLOGY_FACILITY
#endif

#if defined( FEATURE_013_IPTE_RANGE_FACILITY )
 #define    _FEATURE_013_IPTE_RANGE_FACILITY
#endif

#if defined( FEATURE_014_NONQ_KEY_SET_FACILITY )
 #define    _FEATURE_014_NONQ_KEY_SET_FACILITY
#endif

#if defined( FEATURE_016_EXT_TRANSL_FACILITY_2 )
 #define    _FEATURE_016_EXT_TRANSL_FACILITY_2
#endif

#if defined( FEATURE_017_MSA_FACILITY )
 #define    _FEATURE_017_MSA_FACILITY
#endif

#if defined( FEATURE_018_LONG_DISPL_INST_FACILITY )
 #define    _FEATURE_018_LONG_DISPL_INST_FACILITY
#endif

#if defined( FEATURE_019_LONG_DISPL_HPERF_FACILITY )
 #define    _FEATURE_019_LONG_DISPL_HPERF_FACILITY
#endif

#if defined( FEATURE_020_HFP_MULT_ADD_SUB_FACILITY )
 #define    _FEATURE_020_HFP_MULT_ADD_SUB_FACILITY
#endif

#if defined( FEATURE_021_EXTENDED_IMMED_FACILITY )
 #define    _FEATURE_021_EXTENDED_IMMED_FACILITY
#endif

#if defined( FEATURE_022_EXT_TRANSL_FACILITY_3 )
 #define    _FEATURE_022_EXT_TRANSL_FACILITY_3
#endif

#if defined( FEATURE_023_HFP_UNNORM_EXT_FACILITY )
 #define    _FEATURE_023_HFP_UNNORM_EXT_FACILITY
#endif

#if defined( FEATURE_024_ETF2_ENHANCEMENT_FACILITY )
 #define    _FEATURE_024_ETF2_ENHANCEMENT_FACILITY
#endif

#if defined( FEATURE_025_STORE_CLOCK_FAST_FACILITY )
 #define    _FEATURE_025_STORE_CLOCK_FAST_FACILITY
#endif

#if defined( FEATURE_026_PARSING_ENHANCE_FACILITY )
 #define    _FEATURE_026_PARSING_ENHANCE_FACILITY
#endif

#if defined( FEATURE_027_MVCOS_FACILITY )
 #define    _FEATURE_027_MVCOS_FACILITY
#endif

#if defined( FEATURE_028_TOD_CLOCK_STEER_FACILITY )
 #define    _FEATURE_028_TOD_CLOCK_STEER_FACILITY
#endif

#if defined( FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
 #define    _FEATURE_030_ETF3_ENHANCEMENT_FACILITY
#endif

#if defined( FEATURE_031_EXTRACT_CPU_TIME_FACILITY )
 #define    _FEATURE_031_EXTRACT_CPU_TIME_FACILITY
#endif

#if defined( FEATURE_032_CSS_FACILITY )
 #define    _FEATURE_032_CSS_FACILITY
#endif

#if defined( FEATURE_033_CSS_FACILITY_2 )
 #define    _FEATURE_033_CSS_FACILITY_2
#endif

#if defined( FEATURE_034_GEN_INST_EXTN_FACILITY )
 #define    _FEATURE_034_GEN_INST_EXTN_FACILITY
#endif

#if defined( FEATURE_035_EXECUTE_EXTN_FACILITY )
 #define    _FEATURE_035_EXECUTE_EXTN_FACILITY
#endif

#if defined( FEATURE_036_ENH_MONITOR_FACILITY )
 #define    _FEATURE_036_ENH_MONITOR_FACILITY
#endif

#if defined( FEATURE_037_FP_EXTENSIONS_FACILITY )
 #define    _FEATURE_037_FP_EXTENSIONS_FACILITY
#endif

#if defined( FEATURE_038_OP_CMPSC_FACILITY )
 #define    _FEATURE_038_OP_CMPSC_FACILITY
#endif

#if defined( FEATURE_040_LOAD_PROG_PARAM_FACILITY )
 #define    _FEATURE_040_LOAD_PROG_PARAM_FACILITY
#endif

#if defined( FEATURE_041_FPS_ENHANCEMENTS_FACILITY )
 #define    _FEATURE_041_FPS_ENHANCEMENTS_FACILITY
#endif

#if defined( FEATURE_041_DFP_ROUNDING_FACILITY )
 #define    _FEATURE_041_DFP_ROUNDING_FACILITY
#endif

#if defined( FEATURE_041_FPR_GR_TRANSFER_FACILITY )
 #define    _FEATURE_041_FPR_GR_TRANSFER_FACILITY
#endif

#if defined( FEATURE_041_FPS_SIGN_HANDLING_FACILITY )
 #define    _FEATURE_041_FPS_SIGN_HANDLING_FACILITY
#endif

#if defined( FEATURE_041_IEEE_EXCEPT_SIM_FACILITY )
 #define    _FEATURE_041_IEEE_EXCEPT_SIM_FACILITY
#endif

#if defined( FEATURE_042_DECIMAL_FLOAT_FACILITY )
 #define    _FEATURE_042_DECIMAL_FLOAT_FACILITY
#endif

#if defined( FEATURE_043_DFP_HPERF_FACILITY )
 #define    _FEATURE_043_DFP_HPERF_FACILITY
#endif

#if defined( FEATURE_044_PFPO_FACILITY )
 #define    _FEATURE_044_PFPO_FACILITY
#endif

#if defined( FEATURE_045_DISTINCT_OPERANDS_FACILITY )
 #define    _FEATURE_045_DISTINCT_OPERANDS_FACILITY
#endif

#if defined( FEATURE_045_FAST_BCR_SERIAL_FACILITY )
 #define    _FEATURE_045_FAST_BCR_SERIAL_FACILITY
#endif

#if defined( FEATURE_045_HIGH_WORD_FACILITY )
 #define    _FEATURE_045_HIGH_WORD_FACILITY
#endif

#if defined( FEATURE_045_INTERLOCKED_ACCESS_FACILITY_1 )
 #define    _FEATURE_045_INTERLOCKED_ACCESS_FACILITY_1
#endif

#if defined( FEATURE_045_LOAD_STORE_ON_COND_FACILITY_1 )
 #define    _FEATURE_045_LOAD_STORE_ON_COND_FACILITY_1
#endif

#if defined( FEATURE_045_POPULATION_COUNT_FACILITY )
 #define    _FEATURE_045_POPULATION_COUNT_FACILITY
#endif

#if defined( FEATURE_047_CMPSC_ENH_FACILITY )
 #define    _FEATURE_047_CMPSC_ENH_FACILITY
#endif

#if defined( FEATURE_048_DFP_ZONE_CONV_FACILITY )
 #define    _FEATURE_048_DFP_ZONE_CONV_FACILITY
#endif

#if defined( FEATURE_049_EXECUTION_HINT_FACILITY )
 #define    _FEATURE_049_EXECUTION_HINT_FACILITY
#endif

#if defined( FEATURE_049_LOAD_AND_TRAP_FACILITY )
 #define    _FEATURE_049_LOAD_AND_TRAP_FACILITY
#endif

#if defined( FEATURE_049_PROCESSOR_ASSIST_FACILITY )
 #define    _FEATURE_049_PROCESSOR_ASSIST_FACILITY
#endif

#if defined( FEATURE_049_MISC_INSTR_EXT_FACILITY_1 )
 #define    _FEATURE_049_MISC_INSTR_EXT_FACILITY_1
#endif

#if defined( FEATURE_050_CONSTR_TRANSACT_FACILITY )
 #define    _FEATURE_050_CONSTR_TRANSACT_FACILITY
#endif

#if defined( FEATURE_051_LOCAL_TLB_CLEARING_FACILITY )
 #define    _FEATURE_051_LOCAL_TLB_CLEARING_FACILITY
#endif

#if defined( FEATURE_052_INTERLOCKED_ACCESS_FACILITY_2 )
 #define    _FEATURE_052_INTERLOCKED_ACCESS_FACILITY_2
#endif

#if defined( FEATURE_053_LOAD_STORE_ON_COND_FACILITY_2 )
 #define    _FEATURE_053_LOAD_STORE_ON_COND_FACILITY_2
#endif

#if defined( FEATURE_053_LOAD_ZERO_RIGHTMOST_FACILITY )
 #define    _FEATURE_053_LOAD_ZERO_RIGHTMOST_FACILITY
#endif

#if defined( FEATURE_054_EE_CMPSC_FACILITY )
 #define    _FEATURE_054_EE_CMPSC_FACILITY
#endif

#if defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 )
 #define    _FEATURE_057_MSA_EXTENSION_FACILITY_5
#endif

#if defined( FEATURE_058_MISC_INSTR_EXT_FACILITY_2 )
 #define    _FEATURE_058_MISC_INSTR_EXT_FACILITY_2
#endif

#if defined( FEATURE_066_RES_REF_BITS_MULT_FACILITY )
 #define    _FEATURE_066_RES_REF_BITS_MULT_FACILITY
#endif

#if defined( FEATURE_067_CPU_MEAS_COUNTER_FACILITY )
 #define    _FEATURE_067_CPU_MEAS_COUNTER_FACILITY
#endif

#if defined( FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY )
 #define    _FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY
#endif

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
 #define    _FEATURE_073_TRANSACT_EXEC_FACILITY
#endif

#if defined( FEATURE_074_STORE_HYPER_INFO_FACILITY )
 #define    _FEATURE_074_STORE_HYPER_INFO_FACILITY
#endif

#if defined( FEATURE_075_ACC_EX_FS_INDIC_FACILITY )
 #define    _FEATURE_075_ACC_EX_FS_INDIC_FACILITY
#endif

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
 #define    _FEATURE_076_MSA_EXTENSION_FACILITY_3
#endif

#if defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 )
 #define    _FEATURE_077_MSA_EXTENSION_FACILITY_4
#endif

#if defined( FEATURE_078_ENHANCED_DAT_FACILITY_2 )
 #define    _FEATURE_078_ENHANCED_DAT_FACILITY_2
#endif

#if defined( FEATURE_080_DFP_PACK_CONV_FACILITY )
 #define    _FEATURE_080_DFP_PACK_CONV_FACILITY
#endif

#if defined( FEATURE_129_ZVECTOR_FACILITY )
 #define    _FEATURE_129_ZVECTOR_FACILITY
#endif

#if defined( FEATURE_130_INSTR_EXEC_PROT_FACILITY )
 #define    _FEATURE_130_INSTR_EXEC_PROT_FACILITY
#endif

#if defined( FEATURE_131_SIDE_EFFECT_ACCESS_FACILITY )
 #define    _FEATURE_131_SIDE_EFFECT_ACCESS_FACILITY
#endif

#if defined( FEATURE_133_GUARDED_STORAGE_FACILITY )
 #define    _FEATURE_133_GUARDED_STORAGE_FACILITY
#endif

#if defined( FEATURE_134_ZVECTOR_PACK_DEC_FACILITY )
 #define    _FEATURE_134_ZVECTOR_PACK_DEC_FACILITY
#endif

#if defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 )
 #define    _FEATURE_135_ZVECTOR_ENH_FACILITY_1
#endif

#if defined( FEATURE_138_CONFIG_ZARCH_MODE_FACILITY )
 #define    _FEATURE_138_CONFIG_ZARCH_MODE_FACILITY
#endif

#if defined( FEATURE_139_MULTIPLE_EPOCH_FACILITY )
 #define    _FEATURE_139_MULTIPLE_EPOCH_FACILITY
#endif

#if defined( FEATURE_142_ST_CPU_COUNTER_MULT_FACILITY )
 #define    _FEATURE_142_ST_CPU_COUNTER_MULT_FACILITY
#endif

#if defined( FEATURE_144_TEST_PEND_EXTERNAL_FACILITY )
 #define    _FEATURE_144_TEST_PEND_EXTERNAL_FACILITY
#endif

#if defined( FEATURE_145_INS_REF_BITS_MULT_FACILITY )
 #define    _FEATURE_145_INS_REF_BITS_MULT_FACILITY
#endif

#if defined( FEATURE_146_MSA_EXTENSION_FACILITY_8 )
 #define    _FEATURE_146_MSA_EXTENSION_FACILITY_8
#endif

#if defined( FEATURE_168_ESA390_COMPAT_MODE_FACILITY )
 #define    _FEATURE_168_ESA390_COMPAT_MODE_FACILITY
#endif

/*-------------------------------------------------------------------*/
/*                  Non-facility-bit FEATUREs                        */
/*-------------------------------------------------------------------*/

#if defined( FEATURE_2K_STORAGE_KEYS )
 #define    _FEATURE_2K_STORAGE_KEYS
#endif

#if defined( FEATURE_CHANNEL_SUBSYSTEM )
 #define    _FEATURE_CHANNEL_SUBSYSTEM
#endif

#if defined( FEATURE_CPU_RECONFIG )
 #define    _FEATURE_CPU_RECONFIG
#endif

#if defined( FEATURE_ECPSVM )
 #define    _FEATURE_ECPSVM
#endif

#if defined( FEATURE_EMULATE_VM )
 #define    _FEATURE_EMULATE_VM
#endif

#if defined( FEATURE_EXPANDED_STORAGE )
 #define    _FEATURE_EXPANDED_STORAGE
#endif

#if defined( FEATURE_EXPEDITED_SIE_SUBSET )
 #define    _FEATURE_EXPEDITED_SIE_SUBSET
#endif

#if defined( FEATURE_EXTERNAL_INTERRUPT_ASSIST )
 #define    _FEATURE_EXTERNAL_INTERRUPT_ASSIST
#endif

#if defined( FEATURE_HARDWARE_LOADER )
 #define    _FEATURE_HARDWARE_LOADER
#endif

#if defined(  FEATURE_HERCULES_DIAGCALLS )
 #define     _FEATURE_HERCULES_DIAGCALLS
#endif

#if defined( FEATURE_HOST_RESOURCE_ACCESS_FACILITY )
 #define    _FEATURE_HOST_RESOURCE_ACCESS_FACILITY
#endif

#if defined( FEATURE_HYPERVISOR )
 #define    _FEATURE_HYPERVISOR
#endif

#if defined( FEATURE_INTEGRATED_3270_CONSOLE )
 #define    _FEATURE_INTEGRATED_3270_CONSOLE
#endif

#if defined( FEATURE_INTEGRATED_ASCII_CONSOLE )
 #define    _FEATURE_INTEGRATED_ASCII_CONSOLE
#endif

#if defined( FEATURE_INTERVAL_TIMER )
 #define    _FEATURE_INTERVAL_TIMER
#endif

#if defined( FEATURE_IO_ASSIST )
 #define    _FEATURE_IO_ASSIST
#endif

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
 #define    _FEATURE_MSA_EXTENSION_FACILITY_1
#endif

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
 #define    _FEATURE_MSA_EXTENSION_FACILITY_2
#endif

#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
 #define    _FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE
#endif

#if defined( FEATURE_PER )
 #define    _FEATURE_PER
#endif

#if defined( FEATURE_PER2 )
 #define    _FEATURE_PER2
#endif

#if defined( FEATURE_QDIO_TDD )
 #define    _FEATURE_QDIO_TDD
#endif

#if defined( FEATURE_QDIO_THININT )
 #define    _FEATURE_QDIO_THININT
#endif

#if defined( FEATURE_QEBSM )
 #define    _FEATURE_QEBSM
#endif

#if defined( FEATURE_REGION_RELOCATE )
 #define    _FEATURE_REGION_RELOCATE
#endif

#if defined( FEATURE_S370_S390_VECTOR_FACILITY )
 #define    _FEATURE_S370_S390_VECTOR_FACILITY
#endif

#if defined( FEATURE_SCSI_IPL )
 #define    _FEATURE_SCSI_IPL
#endif

#if defined( FEATURE_SVS )
 #define    _FEATURE_SVS
#endif

#if defined( FEATURE_SYSTEM_CONSOLE )
 #define    _FEATURE_SYSTEM_CONSOLE
#endif

#if defined( FEATURE_VM_BLOCKIO )
 #define    _FEATURE_VM_BLOCKIO
#endif

#if defined( FEATURE_WAITSTATE_ASSIST )
 #define    _FEATURE_WAITSTATE_ASSIST
#endif

/*-------------------------------------------------------------------*/
/*                     PROGRAMMING NOTE                              */
/*-------------------------------------------------------------------*/
/* Ideally, none of the #defines made in pass 1 should be dependent  */
/* on any other #define (i.e. your #if statements should not have    */
/* multiple conditions ('&&' or '||') or be dependent on the order   */
/* (sequence) that #defines are being made). The below tiny section  */
/* of code pertaining to SIE appears to be an exception to the rule. */
/*-------------------------------------------------------------------*/

#if defined(   FEATURE_INTERPRETIVE_EXECUTION )

  #define     _FEATURE_SIE    // (370/390 SIE)

  #if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    #define   _FEATURE_ZSIE   // (z/Arch SIE)
  #endif

  #if defined( FEATURE_PROTECTION_INTERCEPTION_CONTROL )
    #define   _FEATURE_PROTECTION_INTERCEPTION_CONTROL
  #endif

  #if defined( FEATURE_STORAGE_KEY_ASSIST )
    #define   _FEATURE_STORAGE_KEY_ASSIST
  #endif

#endif

/*-------------------------------------------------------------------*/
/*        Memory accessing and dynamic translation #defines          */
/*-------------------------------------------------------------------*/
/* Ordinarily #defines related to DLL_IMPORT, DLL_EXPORT and extern  */
/* are performed within the 'hexterns.h' header in coordination with */
/* the source member and loadable module itself (see e.g. hsccmd.c   */
/* _HSCCMD_C_ and _HENGINE_DLL_ handshaking with hexterns.h header). */
/* Since guest memory accessing and dynamic address translation are  */
/* common to across ALL build architectures however (and we wish to  */
/* declare such functions 'static inline' for speed, which requires  */
/* that they all be declared identically across all architectures),  */
/* it's easier and more reliable to do the #defines here instead.    */
/* (see for example source files dat.h/dat.c and vstore.h/vstore.c)  */
/*-------------------------------------------------------------------*/

#undef      _VSTORE_C_STATIC
#if defined( OPTION_INLINE_VSTORE )
 #define    _VSTORE_C_STATIC        static inline
 #define    _VSTORE_FULL_C_STATIC   static
#else
 #ifndef    _VSTORE_C
  #ifndef   _HENGINE_DLL_
   #define  _VSTORE_C_STATIC        DLL_IMPORT
   #define  _VSTORE_FULL_C_STATIC   DLL_IMPORT
  #else
   #define  _VSTORE_C_STATIC        extern
   #define  _VSTORE_FULL_C_STATIC   extern
  #endif
 #else
  #define   _VSTORE_C_STATIC        DLL_EXPORT
  #define   _VSTORE_FULL_C_STATIC   DLL_EXPORT
 #endif
#endif

/*-------------------------------------------------------------------*/

#undef      _VFETCH_C_STATIC
#if defined( OPTION_INLINE_IFETCH )
 #define    _VFETCH_C_STATIC        static inline
#else
 #define    _VFETCH_C_STATIC
#endif

/*-------------------------------------------------------------------*/

#undef      _DAT_C_STATIC
#if defined( OPTION_INLINE_DAT )
 #define    _DAT_C_STATIC           static inline
#else
 #define    _DAT_C_STATIC
#endif

/*-------------------------------------------------------------------*/

#undef      _LOGICAL_C_STATIC
#if defined( OPTION_INLINE_LOGICAL )
 #define    _LOGICAL_C_STATIC       static inline
#else
 #ifndef    _DAT_C
  #ifndef   _HENGINE_DLL_
   #define  _LOGICAL_C_STATIC       DLL_IMPORT
  #else
   #define  _LOGICAL_C_STATIC       extern
  #endif
 #else
  #define   _LOGICAL_C_STATIC       DLL_EXPORT
 #endif
#endif

/*-------------------------------------------------------------------*/
/*                 Build architecture #defines                       */
/*-------------------------------------------------------------------*/
/* _nnn (_370, _390, _900) are architectures present in the build.   */
/* _ARCHMODEn controls self including for arch-dependent compiles.   */
/* _ARCH_nnn is the index for each arch within various tables.       */
/*-------------------------------------------------------------------*/

#if  !defined( OPTION_370_MODE ) \
  && !defined( OPTION_390_MODE ) \
  && !defined( OPTION_900_MODE )
 #error No Architecture mode
#endif

/*-------------------------------------------------------------------*/

#if defined(  OPTION_370_MODE )
 #define            _370
 #define _ARCHMODE1  370
 #define        ARCH_370        0
#endif

#if defined(  OPTION_390_MODE )
 #define            _390
 #if !defined( _ARCHMODE1 )
  #define _ARCHMODE1 390
  #define       ARCH_390        0
 #else
  #define _ARCHMODE2 390
  #define       ARCH_390        1
 #endif
#endif

#if defined(  OPTION_900_MODE )
 #define            _900
 #if !defined( _ARCHMODE2 )
  #define _ARCHMODE2 900
  #define       ARCH_900        1
 #else
  #define _ARCHMODE3 900
  #define       ARCH_900        2
 #endif
#endif

/*-------------------------------------------------------------------*/

#if defined(_900) && !defined(_390)
 #error OPTION_900_MODE requires OPTION_390_MODE to be enabled too
#endif

/*-------------------------------------------------------------------*/

#if !defined( ARCH_370 )
 #define      ARCH_370          -1
#endif
#if !defined( ARCH_390 )
 #define      ARCH_390          -1
#endif
#if !defined( ARCH_900 )
 #define      ARCH_900          -1
#endif

/*-------------------------------------------------------------------*/

#if   defined( _ARCHMODE3 )
 #define    GEN_ARCHCOUNT       3
#elif defined( _ARCHMODE2 )
 #define    GEN_ARCHCOUNT       2
#else
 #define    GEN_ARCHCOUNT       1
#endif

#define INSTRUCTION_DECODE_ENTRIES      2
#define GEN_MAXARCH     GEN_ARCHCOUNT + INSTRUCTION_DECODE_ENTRIES

/*-------------------------------------------------------------------*/
#else // (featchk pass 2: check sanity of #defined features)
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*                   PASS 2: check FEATURE sanity                    */
/*-------------------------------------------------------------------*/
/*  In this section of code you should sanity-check your FEATURE     */
/*  combinations.  That is, if one of your features requires that    */
/*  some other feature also be defined, then you would check that    */
/*  combination here and issue an #error message if it wasn't true.  */
/*  IDEALLY, you should not be doing any #defines here.  You should  */
/*  only be doing #if and #error.  All of the #defines should have   */
/*  ideally already been done in the previous section above during   */
/*  the first pass of this header.  This is to prevent any type of   */
/*  dependency on the order/sequence of checking.  *NONE* of the     */
/*  below sanity checks should depend on any prior/previous check.   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*                   Facility-bit FEATUREs                           */
/*-------------------------------------------------------------------*/

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) && defined( FEATURE_INTERPRETIVE_EXECUTION ) \
&& !defined( _FEATURE_SIE )
 #error z/Arch SIE requires ESA/390 SIE too
#endif

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) && defined( _FEATURE_SIE ) \
    && !defined( FEATURE_STORAGE_KEY_ASSIST )
 #error SIE requires storage key assist
#endif

#if defined( FEATURE_004_IDTE_SC_SEGTAB_FACILITY ) \
&& !defined( FEATURE_003_DAT_ENHANCE_FACILITY_1 )
 #error FEATURE_004_IDTE_SC_SEGTAB_FACILITY requires FEATURE_003_DAT_ENHANCE_FACILITY_1
#endif

#if  defined( FEATURE_005_IDTE_SC_REGTAB_FACILITY ) \
&& (!defined( FEATURE_004_IDTE_SC_SEGTAB_FACILITY ) || !defined( FEATURE_003_DAT_ENHANCE_FACILITY_1 ))
 #error FEATURE_005_IDTE_SC_REGTAB_FACILITY requires both FEATURE_004_IDTE_SC_SEGTAB_FACILITY and FEATURE_003_DAT_ENHANCE_FACILITY_1
#endif

#if defined( FEATURE_006_ASN_LX_REUSE_FACILITY ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
 #error ASN-and-LX-Reuse facility is only supported with z/Arch
#endif

#if defined( FEATURE_006_ASN_LX_REUSE_FACILITY ) && !defined( FEATURE_DUAL_ADDRESS_SPACE )
 #error ASN-and-LX-Reuse facility requires Dual Address-Space feature
#endif

#if defined( FEATURE_007_STFL_EXTENDED_FACILITY ) && !defined( FEATURE_STORE_FACILITY_LIST )
 #error Store Facility List Extended Facility requires FEATURE_STORE_FACILITY_LIST
#endif

#if defined( FEATURE_019_LONG_DISPL_HPERF_FACILITY ) && !defined( FEATURE_018_LONG_DISPL_INST_FACILITY )
 #error FEATURE_019_LONG_DISPL_HPERF_FACILITY requires FEATURE_018_LONG_DISPL_INST_FACILITY
#endif

#if defined( FEATURE_020_HFP_MULT_ADD_SUB_FACILITY ) && !defined( FEATURE_HEXADECIMAL_FLOATING_POINT )
 #error HFP multiply add/subtract facility requires Hexadecimal floating point support
#endif

#if defined( FEATURE_023_HFP_UNNORM_EXT_FACILITY ) && !defined( FEATURE_HEXADECIMAL_FLOATING_POINT )
 #error HFP unnormalized extensions facility requires Hexadecimal floating point support
#endif

#if defined( FEATURE_033_CSS_FACILITY_2 ) && !defined( FEATURE_032_CSS_FACILITY )
 #error FEATURE_033_CSS_FACILITY_2 requires FEATURE_032_CSS_FACILITY
#endif

#if defined( FEATURE_037_FP_EXTENSIONS_FACILITY ) && !defined( FEATURE_042_DECIMAL_FLOAT_FACILITY )
 #error Floating point extensions facility requires Decimal floating point facility
#endif

#if defined( FEATURE_037_FP_EXTENSIONS_FACILITY ) && !defined( FEATURE_BINARY_FLOATING_POINT )
 #error Floating point extensions facility requires Binary floating point support
#endif

#if defined( FEATURE_042_DECIMAL_FLOAT_FACILITY ) && !defined( FEATURE_BASIC_FP_EXTENSIONS )
 #error Decimal floating point facility requires Basic FP extensions
#endif

#if defined( FEATURE_043_DFP_HPERF_FACILITY ) && !defined( FEATURE_042_DECIMAL_FLOAT_FACILITY )
 #error DFP has high performance requires Decimal floating point facility
#endif

#if defined( FEATURE_050_CONSTR_TRANSACT_FACILITY ) && !defined( FEATURE_073_TRANSACT_EXEC_FACILITY )
 #error Constrained-transactional-execution facility requires Transactional-execution facility
#endif

#if defined( FEATURE_067_CPU_MEAS_COUNTER_FACILITY ) && !defined( FEATURE_040_LOAD_PROG_PARAM_FACILITY )
 #error CPU Measurement Counter facility requires Load Program Parameter facility
#endif

#if defined( FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY ) && !defined( FEATURE_040_LOAD_PROG_PARAM_FACILITY )
 #error CPU Measurement Sampling facility requires Load Program Parameter facility
#endif

#if  defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) && !defined( FEATURE_049_PROCESSOR_ASSIST_FACILITY )
 #error Transactional-execution facility requires Processor-assist facility
#endif

#if defined( FEATURE_075_ACC_EX_FS_INDIC_FACILITY ) && !defined( FEATURE_ENHANCED_SUPPRESSION_ON_PROTECTION )
 #error Access-Exception Fetch/Store Indication facility requires Enhanced Suppression on Protection feature
#endif

#if defined( FEATURE_048_DFP_ZONE_CONV_FACILITY ) && !defined( FEATURE_042_DECIMAL_FLOAT_FACILITY )
 #error Decimal-floating-point Zoned-conversion facility requires Decimal floating point facility
#endif

#if defined( FEATURE_080_DFP_PACK_CONV_FACILITY ) && !defined( FEATURE_042_DECIMAL_FLOAT_FACILITY )
 #error Decimal-floating-point Packed-conversion facility requires Decimal floating point facility
#endif

#if defined( FEATURE_129_ZVECTOR_FACILITY ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
 #error z/Arch Vector facility only valid for z/Arch mode
#endif

#if defined( FEATURE_134_ZVECTOR_PACK_DEC_FACILITY ) && !defined( FEATURE_129_ZVECTOR_FACILITY )
 #error z/Arch Vector Packed-decimal facility requires z/Arch Vector facility
#endif

#if defined( FEATURE_135_ZVECTOR_ENH_FACILITY_1 ) && !defined( FEATURE_129_ZVECTOR_FACILITY )
 #error z/Arch Vector Enhancements-1 facility requires z/Arch Vector facility
#endif

#if defined( FEATURE_139_MULTIPLE_EPOCH_FACILITY ) && !defined( FEATURE_025_STORE_CLOCK_FAST_FACILITY )
 #error Multiple-epoch facility requires Store clock fast facility
#endif

#if defined( FEATURE_139_MULTIPLE_EPOCH_FACILITY ) && !defined( FEATURE_028_TOD_CLOCK_STEER_FACILITY )
 #error Multiple-epoch facility requires TOD clock steering facility
#endif

#if defined( FEATURE_142_ST_CPU_COUNTER_MULT_FACILITY ) && !defined( FEATURE_067_CPU_MEAS_COUNTER_FACILITY )
 #error Store-CPU-counter-multiple facility requires CPU-measurement counter facility
#endif

#if defined( FEATURE_146_MSA_EXTENSION_FACILITY_8 ) && !defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
 #error MSA Extension Facility 8 requires MSA Extension Facility 3
#endif

/*-------------------------------------------------------------------*/
/*                  Non-facility-bit FEATUREs                        */
/*-------------------------------------------------------------------*/

#if (!defined( FEATURE_2K_STORAGE_KEYS ) && !defined( FEATURE_4K_STORAGE_KEYS ))
 || ( defined( FEATURE_2K_STORAGE_KEYS ) &&  defined( FEATURE_4K_STORAGE_KEYS ))
 #error Storage Keys must be either 2K or 4K
#endif

#if defined( FEATURE_BASIC_FP_EXTENSIONS ) && !defined( FEATURE_HEXADECIMAL_FLOATING_POINT )
 #error Basic FP extensions requires Hexadecimal floating point
#endif

#if defined( FEATURE_BINARY_FLOATING_POINT ) && !defined( FEATURE_BASIC_FP_EXTENSIONS )
 #error Binary floating point requires Basic FP extensions
#endif

#if defined( FEATURE_CANCEL_IO_FACILITY ) && !defined( FEATURE_CHANNEL_SUBSYSTEM )
 #error Cancel I/O facility requires Channel Subsystem
#endif

#if defined( FEATURE_ENHANCED_SUPPRESSION_ON_PROTECTION ) && !defined( FEATURE_SUPPRESSION_ON_PROTECTION )
 #error Enhanced Suppression on Protection facility requires Suppression on Protection feature
#endif

#if defined( FEATURE_EXPANDED_STORAGE ) && !defined( FEATURE_4K_STORAGE_KEYS )
 #error Expanded storage cannot be defined with 2K Storage keys
#endif

#if defined( FEATURE_EXPEDITED_SIE_SUBSET ) && !defined( _FEATURE_SIE )
 #error Expedited SIE Subset only supported with SIE
#endif

#if defined( FEATURE_EXTERNAL_INTERRUPT_ASSIST ) && !defined( _FEATURE_SIE )
 #error External Interruption assist only supported with SIE
#endif

#if defined( FEATURE_FAST_SYNC_DATA_MOVER ) && !defined( FEATURE_MOVE_PAGE_FACILITY_2 )
 #error Fast sync data mover facility requires Move page facility
#endif

#if defined( FEATURE_FPS_EXTENSIONS ) && !defined( FEATURE_BINARY_FLOATING_POINT )
 #error FP support extensions requires Binary floating point
#endif

#if (defined( FEATURE_FPS_EXTENSIONS )|| defined( FEATURE_HFP_EXTENSIONS )) \
 && !defined( FEATURE_BASIC_FP_EXTENSIONS )
 #error Floating point extensions requires Basic FP extensions
#endif

#if defined( FEATURE_HOST_RESOURCE_FACILITY ) && !defined( _FEATURE_HERCULES_DIAGCALLS )
 #error Hercules Host Resource Access DIAGNOSE 0xF18 requires FEATURE_HERCULES_DIAGCALLS
#endif

#if defined( FEATURE_INTEGRATED_3270_CONSOLE ) && !defined( FEATURE_SYSTEM_CONSOLE )
 #error Integrated 3270 console requires FEATURE_SYSTEM_CONSOLE
#endif

#if defined( FEATURE_INTEGRATED_ASCII_CONSOLE ) && !defined( FEATURE_SYSTEM_CONSOLE )
 #error Integrated ASCII console requires FEATURE_SYSTEM_CONSOLE
#endif

#if defined( FEATURE_IO_ASSIST ) && !defined( _FEATURE_REGION_RELOCATE )
 #error IO Assist requires Region Relocate Facility
#endif

#if defined( FEATURE_IO_ASSIST ) && !defined( _FEATURE_SIE )
 #error I/O Assist Feature only supported with SIE
#endif

#if defined( FEATURE_MOVE_PAGE_FACILITY_2 ) && !defined( FEATURE_4K_STORAGE_KEYS )
 #error Move page facility cannot be defined with 2K Storage keys
#endif

#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE ) && !defined( FEATURE_INTERPRETIVE_EXECUTION )
 #error MCDS is only supported with SIE
#endif

#if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE ) && !defined( FEATURE_STORAGE_KEY_ASSIST )
 #error MCDS requires Storage key assist
#endif

#if defined( FEATURE_PER2 ) && !defined( FEATURE_PER )
 #error FEATURE_PER must be defined when using FEATURE_PER2
#endif

#if defined( FEATURE_PER3 ) && !defined( FEATURE_PER )
 #error FEATURE_PER must be defined when using FEATURE_PER3
#endif

#if defined( FEATURE_PROTECTION_INTERCEPTION_CONTROL ) && !defined( FEATURE_INTERPRETIVE_EXECUTION )
 #error Protection Interception Control is only supported with SIE
#endif

#if defined( FEATURE_REGION_RELOCATE ) && !defined( FEATURE_INTERPRETIVE_EXECUTION )
 #error Region Relocate Facility only supported with SIE
#endif

#if !defined( FEATURE_S370_CHANNEL ) && !defined( FEATURE_CHANNEL_SUBSYSTEM )
 #error Either S/370 Channel or Channel Subsystem must be defined
#endif

#if defined( FEATURE_S370_CHANNEL ) && defined( FEATURE_CHANNEL_SUBSYSTEM )
 #error S/370 Channel and Channel Subsystem cannot both be defined
#endif

#if defined( FEATURE_S370_S390_VECTOR_FACILITY ) && defined(_900)
 #error S370/S390 Vector Facility not supported on z/Arch capable processors
#endif

#if defined( FEATURE_S370_S390_VECTOR_FACILITY ) && defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
 #error S370/S390 Vector Facility not supported in z/Arch mode
#endif

#if defined( FEATURE_SCSI_IPL ) && !defined( FEATURE_HARDWARE_LOADER )
 #error SCSI IPL requires FEATURE_HARDWARE_LOADER
#endif

#if defined( FEATURE_STORAGE_KEY_ASSIST ) && !defined( FEATURE_INTERPRETIVE_EXECUTION )
 #error Storage Key assist only supported with SIE
#endif

#if defined( FEATURE_VM_BLOCKIO ) && !defined( FEATURE_EMULATE_VM )
 #error VM Standard Block I/O DIAGNOSE 0x250 requires FEATURE_EMULATE_VM
#endif

#endif /* !defined( FEATALL_CHECKALL ) */

/* end of FEATCHK.H */
