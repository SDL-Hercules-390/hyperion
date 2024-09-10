/* FEAT370.H    (C) Copyright Jan Jaeger, 2000-2012                  */
/*              S/370 feature definitions                            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This file defines the architectural features which are included   */
/* at compilation time for S/370 mode                                */
/*-------------------------------------------------------------------*/


/*********************************************************************/
/*********************************************************************/
/**                                                                 **/
/**                     PROGRAMMING NOTE!                           **/
/**                                                                 **/
/**       This file MUST *NOT* contain any #undef statements!       **/
/**                                                                 **/
/*********************************************************************/
/*********************************************************************/


#if !defined( OPTION_370_MODE )
#define _ARCH_370_NAME      ""
#else
#define _ARCH_370_NAME      "S/370"

#define FEATURE_2K_STORAGE_KEYS
#define FEATURE_370_EXTENSION           /* (see further below) */
#define FEATURE_BASIC_STORAGE_KEYS
#define FEATURE_BCMODE
#define FEATURE_CHANNEL_SWITCHING
#define FEATURE_DUAL_ADDRESS_SPACE
#define FEATURE_ECPSVM
#define FEATURE_EMULATE_VM
#define FEATURE_EXTENDED_STORAGE_KEYS
#define FEATURE_HERCULES_DIAGCALLS
#define FEATURE_HEXADECIMAL_FLOATING_POINT
#define FEATURE_HOST_RESOURCE_ACCESS_FACILITY
#define FEATURE_INTERVAL_TIMER
#define FEATURE_PER
#define FEATURE_PER1
#define FEATURE_S370_CHANNEL
//#define FEATURE_S370_S390_VECTOR_FACILITY       /* INCOMPLETE */
#define FEATURE_S370E_EXTENDED_ADDRESSING
#define FEATURE_SEGMENT_PROTECTION
#define FEATURE_TEST_BLOCK
#define FEATURE_VM_BLOCKIO
#define FEATURE_TCPIP_EXTENSION
#define TCPNJE_CDWMERGE_KLUDGE

/*-------------------------------------------------------------------*/
/*          "Hercules S/370 Instruction Extension Facility"          */
/*              S/370 backport of S/390 & z/arch                     */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  The following section defines the ESA/390 and z/Architecture     */
/*  features needed to allow certain ESA/390 and z/Architecture      */
/*  instructions to be backported to the S/370 architecture.         */
/*                                                                   */
/*  The backported instructions are made available to the S/370      */
/*  architectural mode by enabling the "Hercules S/370 Instruction   */
/*  Extension Facility" at runtime, which is disabled by default:    */
/*                                                                   */
/*          archlvl  S/370                                           */
/*          facility enable  HERC_370_EXTENSION                      */
/*                                                                   */
/*  The above commands can either be entered manually at Hercules    */
/*  startup or added to your configuration file to have them run     */
/*  automatically whenever Hercules is first started/initialized.    */
/*                                                                   */
/*-------------------------------------------------------------------*/

#if defined( FEATURE_370_EXTENSION )

    // (facility-bit features needed by S/390 and z/Architetcure)

    #define FEATURE_000_N3_INSTR_FACILITY
    #define FEATURE_016_EXT_TRANSL_FACILITY_2
    #define FEATURE_017_MSA_FACILITY
    #define DYNINST_017_MSA_FACILITY                     /* dyncrypt */
    #define FEATURE_018_LONG_DISPL_INST_FACILITY
    #define FEATURE_020_HFP_MULT_ADD_SUB_FACILITY
    #define FEATURE_021_EXTENDED_IMMED_FACILITY
    #define FEATURE_022_EXT_TRANSL_FACILITY_3
    #define FEATURE_023_HFP_UNNORM_EXT_FACILITY
    #define FEATURE_024_ETF2_ENHANCEMENT_FACILITY
    #define FEATURE_026_PARSING_ENHANCE_FACILITY
    #define FEATURE_030_ETF3_ENHANCEMENT_FACILITY
    #define FEATURE_032_CSS_FACILITY
    #define FEATURE_034_GEN_INST_EXTN_FACILITY
    #define FEATURE_035_EXECUTE_EXTN_FACILITY
    #define FEATURE_037_FP_EXTENSION_FACILITY
    #define FEATURE_041_FPS_ENHANCEMENT_FACILITY
    #define FEATURE_041_DFP_ROUNDING_FACILITY
    #define FEATURE_041_FPR_GR_TRANSFER_FACILITY
    #define FEATURE_041_FPS_SIGN_HANDLING_FACILITY
    #define FEATURE_041_IEEE_EXCEPT_SIM_FACILITY
    #define FEATURE_042_DFP_FACILITY
    #define FEATURE_045_DISTINCT_OPERANDS_FACILITY
    #define FEATURE_045_FAST_BCR_SERIAL_FACILITY
    #define FEATURE_045_HIGH_WORD_FACILITY
    #define FEATURE_045_INTERLOCKED_ACCESS_FACILITY_1
    #define FEATURE_045_LOAD_STORE_ON_COND_FACILITY_1
    #define FEATURE_045_POPULATION_COUNT_FACILITY
    #define FEATURE_057_MSA_EXTENSION_FACILITY_5
    #define DYNINST_057_MSA_EXTENSION_FACILITY_5         /* dyncrypt */
    #define FEATURE_076_MSA_EXTENSION_FACILITY_3
    #define DYNINST_076_MSA_EXTENSION_FACILITY_3         /* dyncrypt */
    #define FEATURE_077_MSA_EXTENSION_FACILITY_4
    #define DYNINST_077_MSA_EXTENSION_FACILITY_4         /* dyncrypt */

    // (non-facility-bit features needed by S/390 and z/Architetcure)

    #define FEATURE_BASIC_FP_EXTENSIONS
    #define FEATURE_BINARY_FLOATING_POINT
    #define FEATURE_CHECKSUM_INSTRUCTION
    #define FEATURE_COMPARE_AND_MOVE_EXTENDED
    #define FEATURE_CMPSC
    #define FEATURE_EXTENDED_TRANSLATION_FACILITY_1
    #define FEATURE_HFP_EXTENSIONS
    #define FEATURE_IMMEDIATE_AND_RELATIVE
    #define FEATURE_MSA_EXTENSION_FACILITY_1
    #define FEATURE_MSA_EXTENSION_FACILITY_2
    #define FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS
    #define FEATURE_SQUARE_ROOT
    #define FEATURE_STRING_INSTRUCTION

#endif /* defined( FEATURE_370_EXTENSION ) */

#endif /*defined(OPTION_370_MODE)*/

/* end of FEAT370.H */
