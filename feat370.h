/* FEAT370.H    (c) Copyright Jan Jaeger, 2000-2012                  */
/*              S/370 feature definitions                            */

/*-------------------------------------------------------------------*/
/* This file defines the architectural features which are included   */
/* at compilation time for S/370 mode                                */
/*-------------------------------------------------------------------*/

/*********************************************************************/
/*                      PROGRAMMING NOTE!                            */
/*        This file MUST *NOT* contain any #undef statements!        */
/*********************************************************************/

#if !defined( OPTION_370_MODE )
#define _ARCH_370_NAME      ""
#else
#define _ARCH_370_NAME      "S/370"

#define FEATURE_2K_STORAGE_KEYS
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
#define FEATURE_S370_CHANNEL
#define FEATURE_S370E_EXTENDED_ADDRESSING
#define FEATURE_SEGMENT_PROTECTION
#define FEATURE_TEST_BLOCK
#define FEATURE_VM_BLOCKIO

/*-------------------------------------------------------------------*/
/*              S/370 backport of S/390 & z/arch                     */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  The following section defines the ESA/390 and z/Architecture     */
/*  defined features (and their related instructions) that are       */
/*  compatible and backported to the S/370 architecture.             */
/*                                                                   */
/*  The availability of the relevant instructions are enabled by     */
/*  the s37x loadable module                                         */
/*                                                                   */
/*-------------------------------------------------------------------*/

#if defined( OPTION_370_EXTENSION )

    // ESA/390 & z/Architecture backported features

    // (facility-bit features)

    #define FEATURE_000_N3_INSTR_FACILITY
    #define FEATURE_016_EXT_TRANSL_FACILITY_2
    #define FEATURE_017_MSA_FACILITY
    #define FEATURE_018_LONG_DISPL_INST_FACILITY
    #define FEATURE_020_HFP_MULT_ADD_SUB_FACILITY
    #define FEATURE_021_EXTENDED_IMMED_FACILITY
    #define FEATURE_022_EXT_TRANSL_FACILITY_3
    #define FEATURE_023_HFP_UNNORM_EXT_FACILITY
    #define FEATURE_024_ETF2_ENHANCEMENT_FACILITY
    #define FEATURE_026_PARSING_ENHANCE_FACILITY
    #define FEATURE_030_ETF3_ENHANCEMENT_FACILITY
    #define FEATURE_034_GEN_INST_EXTN_FACILITY
    #define FEATURE_035_EXECUTE_EXTN_FACILITY
    #define FEATURE_076_MSA_EXTENSION_FACILITY_3
    #define FEATURE_077_MSA_EXTENSION_FACILITY_4

    // (non-facility-bit features)

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
    #define FEATURE_SQUARE_ROOT
    #define FEATURE_STRING_INSTRUCTION

#endif /* defined( OPTION_370_EXTENSION ) */

#endif /*defined(OPTION_370_MODE)*/

/* end of FEAT370.H */
