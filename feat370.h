/* FEAT370.H    (c) Copyright Jan Jaeger, 2000-2012                  */
/*              S/370 feature definitions                            */

/*-------------------------------------------------------------------*/
/* This file defines the architectural features which are included   */
/* at compilation time for S/370 mode                                */
/*-------------------------------------------------------------------*/

#if !defined( OPTION_370_MODE )
#define _ARCH_370_NAME ""
#else
#define _ARCH_370_NAME "S/370"

/* This file MUST NOT contain #undef statements */
#define FEATURE_2K_STORAGE_KEYS
#define FEATURE_BASIC_STORAGE_KEYS
#define FEATURE_EXTENDED_STORAGE_KEYS
#define FEATURE_BCMODE
#define FEATURE_DUAL_ADDRESS_SPACE
#define FEATURE_EMULATE_VM
#define FEATURE_HERCULES_DIAGCALLS
#define FEATURE_HEXADECIMAL_FLOATING_POINT
#define FEATURE_HOST_RESOURCE_ACCESS_FACILITY
#define FEATURE_PER
#define FEATURE_INTERVAL_TIMER
#define FEATURE_SEGMENT_PROTECTION
#define FEATURE_S370_CHANNEL
#define FEATURE_CHANNEL_SWITCHING
#define FEATURE_S370E_EXTENDED_ADDRESSING
#define FEATURE_TEST_BLOCK
#define FEATURE_ECPSVM
#define FEATURE_VM_BLOCKIO

/* The following section defines the ESA/390 and
   z/Architecture defined features (and their related
   instructions) that are compatible and backported to
   the S/370 architecture. The availability of the
   relevant instructions are enabled by the s37x loadable
   module */

#if defined(OPTION_370_EXTENSION)

/* ESA/390 backported features */
#define FEATURE_BASIC_FP_EXTENSIONS
#define FEATURE_BINARY_FLOATING_POINT
#define FEATURE_CHECKSUM_INSTRUCTION
#define FEATURE_COMPARE_AND_MOVE_EXTENDED
#define FEATURE_COMPRESSION
#define FEATURE_EXTENDED_TRANSLATION
#define FEATURE_EXTENDED_TRANSLATION_FACILITY_2
#define FEATURE_HFP_EXTENSIONS
#define FEATURE_HFP_MULTIPLY_ADD_SUBTRACT
#define FEATURE_HFP_UNNORMALIZED_EXTENSION
#define FEATURE_IMMEDIATE_AND_RELATIVE
#define FEATURE_SQUARE_ROOT
#define FEATURE_STRING_INSTRUCTION

/* z/Architecture  backported features */
#define FEATURE_ESAME_N3_ESA390
#define FEATURE_ETF2_ENHANCEMENT
#define FEATURE_ETF3_ENHANCEMENT
#define FEATURE_EXECUTE_EXTENSIONS_FACILITY
#define FEATURE_EXTENDED_IMMEDIATE
#define FEATURE_EXTENDED_TRANSLATION_FACILITY_3
#define FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY
#define FEATURE_LONG_DISPLACEMENT
#define FEATURE_MESSAGE_SECURITY_ASSIST
#define FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_1
#define FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_2
#define FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_3
#define FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_4
#define FEATURE_PARSING_ENHANCEMENT_FACILITY
//#define FEATURE_FLOATING_POINT_EXTENSION_FACILITY

#endif /* defined(OPTION_370_EXTENSION) */

#endif /*defined(OPTION_370_MODE)*/
/* end of FEAT370.H */
