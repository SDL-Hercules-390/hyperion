/* FACILITY.C   (C) Copyright "Fish" (David B. Trout), 2018-2019     */
/*              (C) and others 2013-2021                             */
/*                  Facility bit functions                           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _FACILITY_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"             // Need DEF_INST

/*-------------------------------------------------------------------*/
/*                   Architecture bit-masks                          */
/*-------------------------------------------------------------------*/

#ifndef    _NONE
#define    _NONE     0x00           /* NO architectures or disabled  */
#define    _S370     0x80           /* S/370 architecture            */
#define    _E390     0x40           /* ESA/390 architecture          */
#define    _Z900     0x20           /* z/Arch architecture           */
#define    _Z390     (_E390|_Z900)  /* BOTH ESA/390 and z/Arch       */
#define    _MALL     (_S370|_Z390)  /* All architectures             */
#endif

/*-------------------------------------------------------------------*/
/*                ARCH_DEP Architecture bit-masks                    */
/*-------------------------------------------------------------------*/

#undef NONE
#undef S370
#undef E390
#undef Z900
#undef Z390
#undef MALL

#if   __GEN_ARCH == 370             /*    (building for S/370?)      */

    #define S370     _S370          /* S/370 only                    */
    #define E390     _NONE          /* ESA/390 only                  */
    #define Z900     _NONE          /* z/Arch only                   */

#elif __GEN_ARCH == 390             /*    (building for S/390?)      */

    #define S370     _NONE          /* S/370 only                    */
    #define E390     _E390          /* ESA/390 only                  */
    #define Z900     _NONE          /* z/Arch only                   */

#elif __GEN_ARCH == 900             /*    (building for z/Arch?)     */

    #define S370     _NONE          /* S/370 only                    */
    #define E390     _NONE          /* ESA/390 only                  */
    #define Z900     _Z900          /* z/Arch only                   */

#endif

    #define NONE     _NONE          /* NO architectures or disabled  */
    #define Z390     (E390|Z900)    /* BOTH ESA/390 and z/Arch       */
    #define MALL     (S370|Z390)    /* All architectures             */

/*-------------------------------------------------------------------*/
/*          (HERC_370_EXTENSION pseudo-facility support)             */
/*-------------------------------------------------------------------*/

#undef Z39X                         /* E390 + Z900 + optionally S370 */
#undef Z90X                         /* Z900        + optionally S370 */

#if defined( FEATURE_370_EXTENSION )
    #define Z39X     (Z390|S370)    /* E390 + Z900 + optionally S370 */
    #define Z90X     (Z900|S370)    /* Z900        + optionally S370 */
#else
    #define Z39X     (Z390|NONE)    /* E390 + Z900                   */
    #define Z90X     (Z900|NONE)    /* Z900                          */
#endif

/*-------------------------------------------------------------------*/
/*              Temporary ARCH_DEP Facility Table                    */
/*-------------------------------------------------------------------*/
/* The facility names used in the below FT macro invocations are,    */
/* when prefixed with 'STFL_', the facility bit number as #defined   */
/* in header stfl.h (e.g. FT( 044_PFPO, ..) ==> STFL_044_PFPO 44).   */
/*                                                                   */
/* Also note that the entries in the below table do NOT have to be   */
/* in any facility bit sequence as it is always seached serially.    */
/* However, it is greatly preferred that it be kept in sequence.     */
/*                                                                   */
/* Sup (Supported) means the facility is supported by Hercules for   */
/* the given architectures.  Def (Default) indicates the facility    */
/* defaults to enabled for the specified architectures.  The Req     */
/* (Required) field indicates the facility is a REQUIRED facility    */
/* for the architecture and enabling or disabling is NOT allowed.    */
/*                                                                   */
/* ALL known facilities should ALWAYS be defined in the below table  */
/* REGARDLESS of whether or not it has been implemented yet within   */
/* Hercules.  If the facility is not implemented in Hercules yet,    */
/* (i.e. no code has been written to support it yet), then simply    */
/* specify "NONE" in all three Sup/Def/Req columns.  Once support    */
/* for the facility has been coded, then just update the columns     */
/* as appropriate so the guest can start using them.                 */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*                     ***  CRITICAL!   ***                          */
/*                                                                   */
/* All of the below FT macro invokations should be wrapped with an   */
/* #if defined( FEATURE_nnn... ) statement WITHOUT the underscore!   */
/*                                                                   */
/* This is because the below tables are ARCH_DEP tables which are    */
/* compiled differently depending on the architecture is currently   */
/* being built.                                                      */
/*                                                                   */
/*-------------------------------------------------------------------*/

static FACTAB ARCH_DEP( facs_tab )[] =      /* Arch-DEPENDENT table  */
{
/*-------------------------------------------------------------------*/
/*  Sup   Def   Req   Short Name...                                  */
/*-------------------------------------------------------------------*/

#if defined(  FEATURE_000_N3_INSTR_FACILITY )
FT( Z39X, Z390, NONE, 000_N3_INSTR )
#endif

/*-------------------------------------------------------------------*/
/*                       PROGRAMMING NOTE                            */
/*                                                                   */
/* Please note the below use of underscore _FEATURE test.  This is   */
/* the ONLY facility that uses the underscore _FEATURE test instead  */
/* of the normal ARCH_DEP non-underscore FEATURE test.               */
/*                                                                   */
/* THIS IS VERY IMPORTANT since the feat390.h header doesn't #define */
/* "FEATURE_001_ZARCH_INSTALLED_FACILITY" by default (to allow the   */
/* building of versions of Hercules WITHOUT z/Architecture support,  */
/* i.e. with ONLY ESA/390 support).                                  */
/*                                                                   */
/* We don't want to lie and tell ESA/390 mode that z/Architecture    */
/* is installed when it really isn't, but we also need to tell the   */
/* truth too, and tell ESA/390 mode that z/Architecture IS in fact   */
/* installed if it really is (when OPTION_900_MODE is specified).    */
/*                                                                   */
/* By using the underscore _FEATURE test we handle BOTH situations.  */
/*-------------------------------------------------------------------*/

#if defined(    _FEATURE_001_ZARCH_INSTALLED_FACILITY )
FT( _Z390, _Z390, _Z390, 001_ZARCH_INSTALLED )
#endif

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_002_ZARCH_ACTIVE_FACILITY )
FT( Z900, Z900, Z900, 002_ZARCH_ACTIVE )
#endif

#if defined(  FEATURE_003_DAT_ENHANCE_FACILITY_1 )
FT( Z900, Z900, NONE, 003_DAT_ENHANCE_1 )
#endif

#if defined(  FEATURE_004_IDTE_SC_SEGTAB_FACILITY )
FT( NONE, NONE, NONE, 004_IDTE_SC_SEGTAB )
#endif

#if defined(  FEATURE_005_IDTE_SC_REGTAB_FACILITY )
FT( NONE, NONE, NONE, 005_IDTE_SC_REGTAB )
#endif

#if defined(  FEATURE_006_ASN_LX_REUSE_FACILITY )
FT( Z900, Z900, NONE, 006_ASN_LX_REUSE )
#endif

#if defined(  FEATURE_007_STFL_EXTENDED_FACILITY )
FT( Z390, Z390, NONE, 007_STFL_EXTENDED )
#endif

#if defined(  FEATURE_008_ENHANCED_DAT_FACILITY_1 )
FT( Z900, Z900, NONE, 008_EDAT_1 )
#endif

#if defined(  FEATURE_009_SENSE_RUN_STATUS_FACILITY )
FT( Z900, Z900, NONE, 009_SENSE_RUN_STATUS )
#endif

#if defined(  FEATURE_010_CONDITIONAL_SSKE_FACILITY )
FT( Z900, Z900, NONE, 010_CONDITIONAL_SSKE )
#endif

#if defined(  FEATURE_011_CONFIG_TOPOLOGY_FACILITY )
FT( Z900, Z900, NONE, 011_CONFIG_TOPOLOGY )
#endif

FT( NONE, NONE, NONE, 012_IBM_INTERNAL )

#if defined(  FEATURE_013_IPTE_RANGE_FACILITY )
FT( Z900, Z900, NONE, 013_IPTE_RANGE )
#endif

#if defined(  FEATURE_014_NONQ_KEY_SET_FACILITY )
FT( Z900, Z900, NONE, 014_NONQ_KEY_SET )
#endif

FT( NONE, NONE, NONE, 015_IBM_INTERNAL )

#if defined(  FEATURE_016_EXT_TRANSL_FACILITY_2 )
FT( Z39X, Z390, NONE, 016_EXT_TRANSL_2 )
#endif

#if defined(  FEATURE_017_MSA_FACILITY )
FT( Z39X, Z390, NONE, 017_MSA )
#endif

#if defined(  FEATURE_018_LONG_DISPL_INST_FACILITY )
FT( Z90X, Z900, Z900, 018_LONG_DISPL_INST )
#endif

#if defined(  FEATURE_019_LONG_DISPL_HPERF_FACILITY )
FT( Z900, Z900, NONE, 019_LONG_DISPL_HPERF )
#endif

#if defined(  FEATURE_020_HFP_MULT_ADD_SUB_FACILITY )
FT( Z39X, Z390, NONE, 020_HFP_MULT_ADD_SUB )
#endif

#if defined(  FEATURE_021_EXTENDED_IMMED_FACILITY )
FT( Z90X, Z900, NONE, 021_EXTENDED_IMMED )
#endif

#if defined(  FEATURE_022_EXT_TRANSL_FACILITY_3 )
FT( Z90X, Z900, NONE, 022_EXT_TRANSL_3 )
#endif

#if defined(  FEATURE_023_HFP_UNNORM_EXT_FACILITY )
FT( Z90X, Z900, NONE, 023_HFP_UNNORM_EXT )
#endif

#if defined(  FEATURE_024_ETF2_ENHANCEMENT_FACILITY )
FT( Z390, Z390, NONE, 024_ETF2_ENHANCEMENT )
#endif

#if defined(  FEATURE_025_STORE_CLOCK_FAST_FACILITY )
FT( Z900, Z900, NONE, 025_STORE_CLOCK_FAST )
#endif

#if defined(  FEATURE_026_PARSING_ENHANCE_FACILITY )
FT( Z90X, Z900, NONE, 026_PARSING_ENHANCE )
#endif

#if defined(  FEATURE_027_MVCOS_FACILITY )
FT( Z900, Z900, NONE, 027_MVCOS )
#endif

#if defined(  FEATURE_028_TOD_CLOCK_STEER_FACILITY )
FT( Z900, Z900, NONE, 028_TOD_CLOCK_STEER )
#endif

FT( NONE, NONE, NONE, 029_UNDEFINED )

#if defined(  FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
FT( Z900, Z900, NONE, 030_ETF3_ENHANCEMENT )
#endif

#if defined(  FEATURE_031_EXTRACT_CPU_TIME_FACILITY )
FT( Z900, Z900, NONE, 031_EXTRACT_CPU_TIME )
#endif

#if defined(  FEATURE_032_CSS_FACILITY )
FT( Z90X, Z900, NONE, 032_CSSF )
#endif

#if defined(  FEATURE_033_CSS_FACILITY_2 )
FT( Z900, Z900, NONE, 033_CSSF2 )
#endif

#if defined(  FEATURE_034_GEN_INST_EXTN_FACILITY )
FT( Z90X, Z900, NONE, 034_GEN_INST_EXTN )
#endif

#if defined(  FEATURE_035_EXECUTE_EXTN_FACILITY )
FT( Z90X, Z900, NONE, 035_EXECUTE_EXTN )
#endif

#if defined(  FEATURE_036_ENH_MONITOR_FACILITY )
FT( Z900, Z900, NONE, 036_ENH_MONITOR )
#endif

#if defined(  FEATURE_037_FP_EXTENSION_FACILITY )
FT( Z90X, Z900, NONE, 037_FP_EXTENSION )
#endif

#if defined(  FEATURE_038_OP_CMPSC_FACILITY )
FT( NONE, NONE, NONE, 038_OP_CMPSC )
#endif

FT( NONE, NONE, NONE, 039_IBM_INTERNAL )

#if defined(  FEATURE_040_LOAD_PROG_PARAM_FACILITY )
FT( Z900, Z900, NONE, 040_LOAD_PROG_PARAM )
#endif

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_041_FPS_ENHANCEMENT_FACILITY )

FT( Z90X, Z900, NONE, 041_FPS_ENHANCEMENT )

#if defined(  FEATURE_041_DFP_ROUNDING_FACILITY )
FT( Z90X, Z900, NONE, 041_DFP_ROUNDING )
#endif

#if defined(  FEATURE_041_FPR_GR_TRANSFER_FACILITY )
FT( Z90X, Z900, NONE, 041_FPR_GR_TRANSFER )
#endif

#if defined(  FEATURE_041_FPS_SIGN_HANDLING_FACILITY )
FT( Z90X, Z900, NONE, 041_FPS_SIGN_HANDLING )
#endif

#if defined(  FEATURE_041_IEEE_EXCEPT_SIM_FACILITY )
FT( Z90X, Z900, NONE, 041_IEEE_EXCEPT_SIM )
#endif

#endif /* defined(  FEATURE_041_FPS_ENHANCEMENT_FACILITY ) */

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_042_DFP_FACILITY )
FT( Z90X, Z900, NONE, 042_DFP )
#endif

#if defined(  FEATURE_043_DFP_HPERF_FACILITY )
FT( Z90X, Z900, NONE, 043_DFP_HPERF )
#endif

#if defined(  FEATURE_044_PFPO_FACILITY )
FT( Z90X, Z900, NONE, 044_PFPO )
#endif

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_045_DISTINCT_OPERANDS_FACILITY )
FT( Z90X, Z900, NONE, 045_DISTINCT_OPERANDS )
#endif

#if defined(  FEATURE_045_FAST_BCR_SERIAL_FACILITY )
FT( Z90X, Z900, NONE, 045_FAST_BCR_SERIAL )
#endif

#if defined(  FEATURE_045_HIGH_WORD_FACILITY )
FT( Z90X, Z900, NONE, 045_HIGH_WORD )
#endif

#if defined(  FEATURE_045_INTERLOCKED_ACCESS_FACILITY_1 )
FT( Z90X, Z900, NONE, 045_INTERLOCKED_ACCESS_1 )
#endif

#if defined(  FEATURE_045_LOAD_STORE_ON_COND_FACILITY_1 )
FT( Z90X, Z900, NONE, 045_LOAD_STORE_ON_COND_1 )
#endif

#if defined(  FEATURE_045_POPULATION_COUNT_FACILITY )
FT( Z90X, Z900, NONE, 045_POPULATION_COUNT )
#endif

/*-------------------------------------------------------------------*/

FT( NONE, NONE, NONE, 046_IBM_INTERNAL )

#if defined(  FEATURE_047_CMPSC_ENH_FACILITY )
FT( Z900, Z900, NONE, 047_CMPSC_ENH )
#endif

#if defined(  FEATURE_048_DFP_ZONE_CONV_FACILITY )
FT( Z900, Z900, NONE, 048_DFP_ZONE_CONV )
#endif

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_049_EXECUTION_HINT_FACILITY )
FT( Z900, Z900, NONE, 049_EXECUTION_HINT )
#endif

#if defined(  FEATURE_049_LOAD_AND_TRAP_FACILITY )
FT( Z900, Z900, NONE, 049_LOAD_AND_TRAP )
#endif

#if defined(  FEATURE_049_PROCESSOR_ASSIST_FACILITY )
FT( Z900, Z900, NONE, 049_PROCESSOR_ASSIST )
#endif

#if defined(  FEATURE_049_MISC_INSTR_EXT_FACILITY_1 )
FT( Z900, Z900, NONE, 049_MISC_INSTR_EXT_1 )
#endif

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_050_CONSTR_TRANSACT_FACILITY )
FT( Z900, Z900, NONE, 050_CONSTR_TRANSACT )
#endif

#if defined(  FEATURE_051_LOCAL_TLB_CLEARING_FACILITY )
FT( Z900, Z900, NONE, 051_LOCAL_TLB_CLEARING )
#endif

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_052_INTERLOCKED_ACCESS_FACILITY_2 )
#if IAF2_ATOMICS_UNAVAILABLE == CAN_IAF2
FT( NONE, NONE, NONE, 052_INTERLOCKED_ACCESS_2 )
#else
FT( Z390, Z390, NONE, 052_INTERLOCKED_ACCESS_2 )
#endif
#endif /* defined(  FEATURE_052_INTERLOCKED_ACCESS_FACILITY_2 ) */

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_053_LOAD_STORE_ON_COND_FACILITY_2 )
FT( Z900, Z900, NONE, 053_LOAD_STORE_ON_COND_2 )
#endif

#if defined(  FEATURE_053_LOAD_ZERO_RIGHTMOST_FACILITY )
FT( Z900, Z900, NONE, 053_LOAD_ZERO_RIGHTMOST )
#endif

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_054_EE_CMPSC_FACILITY )
FT( NONE, NONE, NONE, 054_EE_CMPSC )
#endif

FT( NONE, NONE, NONE, 055_IBM_INTERNAL )

FT( NONE, NONE, NONE, 056_UNDEFINED )

#if defined(  FEATURE_057_MSA_EXTENSION_FACILITY_5 )
FT( NONE, NONE, NONE, 057_MSA_EXTENSION_5 )
#endif

#if defined(  FEATURE_058_MISC_INSTR_EXT_FACILITY_2 )
FT( Z900, Z900, NONE, 058_MISC_INSTR_EXT_2 )
#endif

FT( NONE, NONE, NONE, 059_IBM_INTERNAL )
FT( NONE, NONE, NONE, 060_IBM_INTERNAL )

#if defined(  FEATURE_061_MISC_INSTR_EXT_FACILITY_3 )
FT( Z900, Z900, NONE, 061_MISC_INSTR_EXT_3 )
#endif

FT( NONE, NONE, NONE, 062_IBM_INTERNAL )
FT( NONE, NONE, NONE, 063_IBM_INTERNAL )
FT( NONE, NONE, NONE, 064_IBM_INTERNAL )
FT( NONE, NONE, NONE, 065_IBM_INTERNAL )

#if defined(  FEATURE_066_RES_REF_BITS_MULT_FACILITY )
FT( Z900, Z900, NONE, 066_RES_REF_BITS_MULT )
#endif

#if defined(  FEATURE_067_CPU_MEAS_COUNTER_FACILITY )
FT( NONE, NONE, NONE, 067_CPU_MEAS_COUNTER )
#endif

#if defined(  FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY )
FT( NONE, NONE, NONE, 068_CPU_MEAS_SAMPLNG )
#endif

FT( NONE, NONE, NONE, 069_IBM_INTERNAL )
FT( NONE, NONE, NONE, 070_IBM_INTERNAL )
FT( NONE, NONE, NONE, 071_IBM_INTERNAL )
FT( NONE, NONE, NONE, 072_IBM_INTERNAL )

#if defined(  FEATURE_073_TRANSACT_EXEC_FACILITY )
FT( Z900, Z900, NONE, 073_TRANSACT_EXEC )
#endif

#if defined(  FEATURE_074_STORE_HYPER_INFO_FACILITY )
FT( Z900, NONE, NONE, 074_STORE_HYPER_INFO ) // (defaults to OFF/disabled)
#endif

#if defined(  FEATURE_075_ACC_EX_FS_INDIC_FACILITY )
FT( Z900, Z900, NONE, 075_ACC_EX_FS_INDIC )
#endif

#if defined(  FEATURE_076_MSA_EXTENSION_FACILITY_3 )
FT( Z90X, Z900, NONE, 076_MSA_EXTENSION_3 )
#endif

#if defined(  FEATURE_077_MSA_EXTENSION_FACILITY_4 )
FT( Z90X, Z900, NONE, 077_MSA_EXTENSION_4 )
#endif

#if defined(  FEATURE_078_ENHANCED_DAT_FACILITY_2 )
FT( NONE, NONE, NONE, 078_EDAT_2 )
#endif

FT( NONE, NONE, NONE, 079_UNDEFINED )

#if defined(  FEATURE_080_DFP_PACK_CONV_FACILITY )
FT( Z900, Z900, NONE, 080_DFP_PACK_CONV )
#endif

#if defined(  FEATURE_081_PPA_IN_ORDER_FACILITY )
FT( Z900, Z900, NONE, 081_PPA_IN_ORDER )
#endif

FT( NONE, NONE, NONE, 082_IBM_INTERNAL )

FT( NONE, NONE, NONE, 083_UNDEFINED )
FT( NONE, NONE, NONE, 084_UNDEFINED )
FT( NONE, NONE, NONE, 085_UNDEFINED )
FT( NONE, NONE, NONE, 086_UNDEFINED )
FT( NONE, NONE, NONE, 087_UNDEFINED )
FT( NONE, NONE, NONE, 088_UNDEFINED )
FT( NONE, NONE, NONE, 089_UNDEFINED )
FT( NONE, NONE, NONE, 090_UNDEFINED )
FT( NONE, NONE, NONE, 091_UNDEFINED )
FT( NONE, NONE, NONE, 092_UNDEFINED )
FT( NONE, NONE, NONE, 093_UNDEFINED )
FT( NONE, NONE, NONE, 094_UNDEFINED )
FT( NONE, NONE, NONE, 095_UNDEFINED )
FT( NONE, NONE, NONE, 096_UNDEFINED )
FT( NONE, NONE, NONE, 097_UNDEFINED )
FT( NONE, NONE, NONE, 098_UNDEFINED )
FT( NONE, NONE, NONE, 099_UNDEFINED )
FT( NONE, NONE, NONE, 100_UNDEFINED )
FT( NONE, NONE, NONE, 101_UNDEFINED )
FT( NONE, NONE, NONE, 102_UNDEFINED )
FT( NONE, NONE, NONE, 103_UNDEFINED )
FT( NONE, NONE, NONE, 104_UNDEFINED )
FT( NONE, NONE, NONE, 105_UNDEFINED )
FT( NONE, NONE, NONE, 106_UNDEFINED )
FT( NONE, NONE, NONE, 107_UNDEFINED )
FT( NONE, NONE, NONE, 108_UNDEFINED )
FT( NONE, NONE, NONE, 109_UNDEFINED )
FT( NONE, NONE, NONE, 110_UNDEFINED )
FT( NONE, NONE, NONE, 111_UNDEFINED )
FT( NONE, NONE, NONE, 112_UNDEFINED )
FT( NONE, NONE, NONE, 113_UNDEFINED )
FT( NONE, NONE, NONE, 114_UNDEFINED )
FT( NONE, NONE, NONE, 115_UNDEFINED )
FT( NONE, NONE, NONE, 116_UNDEFINED )
FT( NONE, NONE, NONE, 117_UNDEFINED )
FT( NONE, NONE, NONE, 118_UNDEFINED )
FT( NONE, NONE, NONE, 119_UNDEFINED )
FT( NONE, NONE, NONE, 120_UNDEFINED )
FT( NONE, NONE, NONE, 121_UNDEFINED )
FT( NONE, NONE, NONE, 122_UNDEFINED )
FT( NONE, NONE, NONE, 123_UNDEFINED )
FT( NONE, NONE, NONE, 124_UNDEFINED )
FT( NONE, NONE, NONE, 125_UNDEFINED )
FT( NONE, NONE, NONE, 126_UNDEFINED )
FT( NONE, NONE, NONE, 127_UNDEFINED )

FT( NONE, NONE, NONE, 128_IBM_INTERNAL )

#if defined(  FEATURE_129_ZVECTOR_FACILITY )
FT( Z900, Z900, NONE, 129_ZVECTOR )
#endif

#if defined(  FEATURE_130_INSTR_EXEC_PROT_FACILITY )
FT( NONE, NONE, NONE, 130_INSTR_EXEC_PROT )
#endif

#if defined(  FEATURE_131_ENH_SUPP_ON_PROT_2_FACILITY )
FT( NONE, NONE, NONE, 131_ENH_SUPP_ON_PROT_2 )
#endif

#if defined(  FEATURE_131_SIDE_EFFECT_ACCESS_FACILITY )
FT( NONE, NONE, NONE, 131_SIDE_EFFECT_ACCESS )
#endif

FT( NONE, NONE, NONE, 132_UNDEFINED )

#if defined(  FEATURE_133_GUARDED_STORAGE_FACILITY )
FT( NONE, NONE, NONE, 133_GUARDED_STORAGE )
#endif

#if defined(  FEATURE_134_ZVECTOR_PACK_DEC_FACILITY )
FT( Z900, Z900, NONE, 134_ZVECTOR_PACK_DEC )
#endif

#if defined(  FEATURE_135_ZVECTOR_ENH_FACILITY_1 )
FT( Z900, Z900, NONE, 135_ZVECTOR_ENH_1 )
#endif

FT( NONE, NONE, NONE, 136_UNDEFINED )
FT( NONE, NONE, NONE, 137_UNDEFINED )

#if defined(  FEATURE_138_CONFIG_ZARCH_MODE_FACILITY )
FT( NONE, NONE, NONE, 138_CONFIG_ZARCH_MODE )
#endif

#if defined(  FEATURE_139_MULTIPLE_EPOCH_FACILITY )
FT( NONE, NONE, NONE, 139_MULTIPLE_EPOCH )
#endif

FT( NONE, NONE, NONE, 140_IBM_INTERNAL )
FT( NONE, NONE, NONE, 141_IBM_INTERNAL )

#if defined(  FEATURE_142_ST_CPU_COUNTER_MULT_FACILITY )
FT( NONE, NONE, NONE, 142_ST_CPU_COUNTER_MULT )
#endif

FT( NONE, NONE, NONE, 143_UNDEFINED )

#if defined(  FEATURE_144_TEST_PEND_EXTERNAL_FACILITY )
FT( NONE, NONE, NONE, 144_TEST_PEND_EXTERNAL )
#endif

#if defined(  FEATURE_145_INS_REF_BITS_MULT_FACILITY )
FT( Z900, Z900, NONE, 145_INS_REF_BITS_MULT )
#endif

#if defined(  FEATURE_146_MSA_EXTENSION_FACILITY_8 )
FT( NONE, NONE, NONE, 146_MSA_EXTENSION_8 )
#endif

FT( NONE, NONE, NONE, 147_IBM_RESERVED )

#if defined(  FEATURE_148_VECTOR_ENH_FACILITY_2 )
FT( Z900, Z900, NONE, 148_VECTOR_ENH_2 )
#endif

#if defined(  FEATURE_149_MOVEPAGE_SETKEY_FACILITY )
FT( NONE, NONE, NONE, 149_MOVEPAGE_SETKEY )
#endif

#if defined(  FEATURE_150_ENH_SORT_FACILITY )
FT( NONE, NONE, NONE, 150_ENH_SORT )
#endif

#if defined(  FEATURE_151_DEFLATE_CONV_FACILITY )
FT( NONE, NONE, NONE, 151_DEFLATE_CONV )
#endif

#if defined(  FEATURE_152_VECT_PACKDEC_ENH_FACILITY )
FT( Z900, Z900, NONE, 152_VECT_PACKDEC_ENH )
#endif

FT( NONE, NONE, NONE, 153_IBM_INTERNAL )
FT( NONE, NONE, NONE, 154_UNDEFINED )

#if defined(  FEATURE_155_MSA_EXTENSION_FACILITY_9 )
FT( NONE, NONE, NONE, 155_MSA_EXTENSION_9 )
#endif

FT( NONE, NONE, NONE, 156_IBM_INTERNAL )
FT( NONE, NONE, NONE, 157_UNDEFINED )

#if defined(  FEATURE_158_ULTRAV_CALL_FACILITY )
FT( NONE, NONE, NONE, 158_ULTRAV_CALL )
#endif

FT( NONE, NONE, NONE, 159_UNDEFINED )
FT( NONE, NONE, NONE, 160_UNDEFINED )

#if defined(  FEATURE_161_SEC_EXE_UNPK_FACILITY )
FT( NONE, NONE, NONE, 161_SEC_EXE_UNPK )
#endif

FT( NONE, NONE, NONE, 162_UNDEFINED )
FT( NONE, NONE, NONE, 163_UNDEFINED )
FT( NONE, NONE, NONE, 164_UNDEFINED )

#if defined(  FEATURE_165_NNET_ASSIST_FACILITY )
FT( Z900, Z900, NONE, 165_NNET_ASSIST )
#endif

FT( NONE, NONE, NONE, 166_UNDEFINED )
FT( NONE, NONE, NONE, 167_UNDEFINED )

#if defined(  FEATURE_168_ESA390_COMPAT_MODE_FACILITY )
FT( NONE, NONE, NONE, 168_ESA390_COMPAT_MODE )
#endif

#if defined(  FEATURE_169_SKEY_REMOVAL_FACILITY )
FT( NONE, NONE, NONE, 169_SKEY_REMOVAL )
#endif

FT( NONE, NONE, NONE, 170_UNDEFINED )
FT( NONE, NONE, NONE, 171_UNDEFINED )
FT( NONE, NONE, NONE, 172_UNDEFINED )
FT( NONE, NONE, NONE, 173_UNDEFINED )
FT( NONE, NONE, NONE, 174_UNDEFINED )
FT( NONE, NONE, NONE, 175_UNDEFINED )
FT( NONE, NONE, NONE, 176_UNDEFINED )
FT( NONE, NONE, NONE, 177_UNDEFINED )
FT( NONE, NONE, NONE, 178_UNDEFINED )
FT( NONE, NONE, NONE, 179_UNDEFINED )
FT( NONE, NONE, NONE, 180_UNDEFINED )
FT( NONE, NONE, NONE, 181_UNDEFINED )
FT( NONE, NONE, NONE, 182_UNDEFINED )
FT( NONE, NONE, NONE, 183_UNDEFINED )
FT( NONE, NONE, NONE, 184_UNDEFINED )
FT( NONE, NONE, NONE, 185_UNDEFINED )
FT( NONE, NONE, NONE, 186_UNDEFINED )
FT( NONE, NONE, NONE, 187_UNDEFINED )
FT( NONE, NONE, NONE, 188_UNDEFINED )
FT( NONE, NONE, NONE, 189_UNDEFINED )
FT( NONE, NONE, NONE, 190_UNDEFINED )
FT( NONE, NONE, NONE, 191_UNDEFINED )

#if defined(  FEATURE_192_VECT_PACKDEC_ENH_2_FACILITY )
FT( Z900, Z900, NONE, 192_VECT_PACKDEC_ENH_2 )
#endif

#if defined(  FEATURE_193_BEAR_ENH_FACILITY )
FT( Z900, Z900, NONE, 193_BEAR_ENH )
#endif

#if defined(  FEATURE_194_RESET_DAT_PROT_FACILITY )
FT( NONE, NONE, NONE, 194_RESET_DAT_PROT )
#endif

FT( NONE, NONE, NONE, 195_UNDEFINED )

#if defined(  FEATURE_196_PROC_ACT_FACILITY)
FT( NONE, NONE, NONE, 196_PROC_ACT )
#endif

#if defined(  FEATURE_197_PROC_ACT_EXT_1_FACILITY)
FT( NONE, NONE, NONE, 197_PROC_ACT_EXT_1 )
#endif

FT( NONE, NONE, NONE, 198_UNDEFINED )
FT( NONE, NONE, NONE, 199_UNDEFINED )
FT( NONE, NONE, NONE, 200_UNDEFINED )

/*-------------------------------------------------------------------*/
/*                      Hercules Facility bits                       */
/*-------------------------------------------------------------------*/
/* The below facility bits are HERCULES SPECIFIC and not part of the */
/* architecture.  They are placed here for the convenience of being  */
/* able to use the Virtual Architecture Level facility (i.e. the     */
/* FACILITY_CHECK and FACILITY_ENABLED macros).                      */
/*                                                                   */
/* Note that Hercules's facility bits start at the first bit of the  */
/* first byte of the first double-word (DW) immediately following    */
/* the IBM defined bits, and are inaccessible to the guest. Both of  */
/* the STFLE and SIE instruction functions only reference/use the    */
/* STFL_IBM_BY_SIZE value in their code thus preventing guest access */
/* to Hercules's facility bits. Only the facility command functions  */
/* can access the Hercules facility bits and only Hercules itself    */
/* uses them internally.                                             */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Sup (Supported) means the facility is supported by Hercules for   */
/* the given architectures.  Def (Default) indicates the facility    */
/* defaults to enabled for the specified architectures.  The Req     */
/* (Required) field indicates the facility is a REQUIRED facility    */
/* for the architecture and enabling/disabling it isn't allowed.     */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*  Sup   Def   Req   Short Name...                                  */
/*-------------------------------------------------------------------*/

#if defined(       FEATURE_370_EXTENSION )
FT( S370, NONE, NONE, HERC_370_EXTENSION )
#endif

FT( MALL, MALL, NONE, HERC_DETECT_PGMINTLOOP )

#if defined( FEATURE_HERCULES_DIAGCALLS )
#if defined(       FEATURE_HOST_RESOURCE_ACCESS_FACILITY )
FT( MALL, NONE, NONE, HERC_HOST_RESOURCE_ACCESS )
#endif
#endif

#if defined(       FEATURE_INTERVAL_TIMER )
FT( MALL, MALL, Z390, HERC_INTERVAL_TIMER )
#endif

#if defined( FEATURE_HYPERVISOR )
FT( MALL, MALL, NONE, HERC_LOGICAL_PARTITION )
#endif

FT( MALL, MALL, Z900, HERC_MOVE_INVERSE )

#if defined(       FEATURE_MSA_EXTENSION_FACILITY_1 )
FT( Z39X, Z390, NONE, HERC_MSA_EXTENSION_1 )
#endif

#if defined(       FEATURE_MSA_EXTENSION_FACILITY_2 )
FT( Z39X, Z390, NONE, HERC_MSA_EXTENSION_2 )
#endif

#if defined( FEATURE_HERCULES_DIAGCALLS )
FT( MALL, NONE, NONE, HERC_PROBSTATE_DIAGF08 )
#endif

FT( Z390, NONE, NONE, HERC_QDIO_ASSIST )

#if defined(       FEATURE_QDIO_TDD )
FT( Z390, NONE, NONE, HERC_QDIO_TDD )
#endif

#if defined(       FEATURE_QDIO_THININT )
FT( Z390, Z390, NONE, HERC_QDIO_THININT )
#endif

#if defined(       FEATURE_QEBSM )
FT( Z390, Z390, NONE, HERC_QEBSM )
#endif

#if defined( FEATURE_HERCULES_DIAGCALLS )
FT( MALL, NONE, NONE, HERC_SIGP_SETARCH_S370 )
#endif

#if defined(       FEATURE_SVS )
FT( Z390, Z390, NONE, HERC_SVS )
#endif

#if defined( FEATURE_EMULATE_VM )
FT( MALL, NONE, NONE, HERC_VIRTUAL_MACHINE )
#endif

#if defined( FEATURE_TCPIP_EXTENSION )
FT( MALL, NONE, NONE, HERC_TCPIP_EXTENSION )
FT( MALL, NONE, NONE, HERC_TCPIP_PROB_STATE )
#endif

#if defined( FEATURE_ZVM_ESSA )
FT( Z900, NONE, NONE, HERC_ZVM_ESSA ) // z/VM ESSA Extract and Set Storage Attributes
                                      // (defaults to OFF/disabled)
#endif
};

/*-------------------------------------------------------------------*/
/*          Facility Not Enabled Program Check function              */
/*-------------------------------------------------------------------*/
/* The following intruction function is used to force an immediate   */
/* Operation Exception Program Check interruption for instructions   */
/* pertaining to a disabled facility. When a facility is disabled,   */
/* the opcode table for instructions associated with that facility   */
/* is updated to point all of its instructions to this function.     */
/* This relieves each individual instruction from having to always   */
/* perform a FACILITY_CHECK macro to manually check if the facility  */
/* is enabled or not each time it is executed, thereby allowing it   */
/* to be more efficient (execute faster) since if the facility is    */
/* disabled the instruction function is never even called. Instead,  */
/* the below instruction function is directly called instead.        */
/*-------------------------------------------------------------------*/

static DEF_INST( facility_not_enabled ) {
       ARCH_DEP( operation_exception )( inst, regs ); }

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined(   _GEN_ARCH )

  // _ARCH_NUM_0 has been built.
  // Now build the other architectures...

  #if defined(                _ARCH_NUM_1 )
    #define     _GEN_ARCH     _ARCH_NUM_1
    #include    "facility.c"
  #endif

  #if defined(                _ARCH_NUM_2 )
    #undef      _GEN_ARCH
    #define     _GEN_ARCH     _ARCH_NUM_2
    #include    "facility.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*   Facility Bit Modification Check Functions forward references    */
/*-------------------------------------------------------------------*/

static  bool  mod000    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod002    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod003    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod004    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod005    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod007    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod008    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod010    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod014    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod018    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod019    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod025    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod028    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod037    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod040    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod042    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod043    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod045    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod048    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod049    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod050    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod051    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod061    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod066    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod067    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod068    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod073    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod076    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod077    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod078    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod080    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod081    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod129    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod134    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod135    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod139    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod142    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod145    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod146    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod148    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod149    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod152    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod155    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod165    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod168    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod169    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod192    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod194    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod196    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );
static  bool  mod197    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );

static  bool  modtcp    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* opp_actioning, const char* target_facname );

/*-------------------------------------------------------------------*/
/*   Facility Opcode Table Update Functions forward references       */
/*-------------------------------------------------------------------*/

static  void  instr0    ( int arch, bool enable );
static  void  instr3    ( int arch, bool enable );
static  void  instr6    ( int arch, bool enable );
static  void  instr7    ( int arch, bool enable );
static  void  instr8    ( int arch, bool enable );
static  void  instr11   ( int arch, bool enable );
static  void  instr16   ( int arch, bool enable );
static  void  instr17   ( int arch, bool enable );
static  void  instr18   ( int arch, bool enable );
static  void  instr20   ( int arch, bool enable );
static  void  instr21   ( int arch, bool enable );
static  void  instr22   ( int arch, bool enable );
static  void  instr23   ( int arch, bool enable );
static  void  instr25   ( int arch, bool enable );
static  void  instr26   ( int arch, bool enable );
static  void  instr27   ( int arch, bool enable );
static  void  instr28   ( int arch, bool enable );
static  void  instr31   ( int arch, bool enable );
static  void  instr32   ( int arch, bool enable );
static  void  instr34   ( int arch, bool enable );
static  void  instr35   ( int arch, bool enable );
static  void  instr37   ( int arch, bool enable );
static  void  instr40   ( int arch, bool enable );
static  void  instr41   ( int arch, bool enable );
static  void  instr42   ( int arch, bool enable );
static  void  instr44   ( int arch, bool enable );
static  void  instr45   ( int arch, bool enable );
static  void  instr48   ( int arch, bool enable );
static  void  instr49   ( int arch, bool enable );
static  void  instr50   ( int arch, bool enable );
static  void  instr53   ( int arch, bool enable );
static  void  instr57   ( int arch, bool enable );
static  void  instr58   ( int arch, bool enable );
static  void  instr61   ( int arch, bool enable );
static  void  instr66   ( int arch, bool enable );
static  void  instr67   ( int arch, bool enable );
static  void  instr68   ( int arch, bool enable );
static  void  instr73   ( int arch, bool enable );
static  void  instr74   ( int arch, bool enable );
static  void  instr76   ( int arch, bool enable );
static  void  instr77   ( int arch, bool enable );
static  void  instr78   ( int arch, bool enable );
static  void  instr80   ( int arch, bool enable );
static  void  instr129  ( int arch, bool enable );
static  void  instr133  ( int arch, bool enable );
static  void  instr134  ( int arch, bool enable );
static  void  instr142  ( int arch, bool enable );
static  void  instr144  ( int arch, bool enable );
static  void  instr145  ( int arch, bool enable );
static  void  instr146  ( int arch, bool enable );
static  void  instr148  ( int arch, bool enable );
static  void  instr150  ( int arch, bool enable );
static  void  instr151  ( int arch, bool enable );
static  void  instr155  ( int arch, bool enable );
static  void  instr165  ( int arch, bool enable );
static  void  instr192  ( int arch, bool enable );
static  void  instr193  ( int arch, bool enable );
static  void  instr194  ( int arch, bool enable );
static  void  instr196  ( int arch, bool enable );

static  void  hercmvcin ( int arch, bool enable );
static  void  hercsvs   ( int arch, bool enable );
static  void  herc37X   ( int arch, bool enable );
static  void  herctcp   ( int arch, bool enable );
static  void  hercessa  ( int arch, bool enable );

/*-------------------------------------------------------------------*/
/* The ACTUAL facilities table, initialized by init_facilities_lists */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  The individual ARCH_DEP( facs_tab ) tables are merged into this  */
/*  table to yield the actual facilities table the system will use.  */
/*  Refer to init_facilities_lists() function for how this is done.  */
/*                                                                   */
/*  Refer to the README.FACILITIES document for both instructions on */
/*  how to update Hercules facilities as well as for a handy table   */
/*  that documents interfacility dependencies and incompatibilities. */
/*                                                                   */
/*-------------------------------------------------------------------*/

static FACTAB factab[] =
{
/*----------------------------------------------------------------------------*/
/*   (func)    (func)      Short Name...               Long Description...    */
/*----------------------------------------------------------------------------*/
FT2( mod000,    instr0,    000_N3_INSTR,               "N3 Instructions are installed" )
FT2( NULL,      NULL,      001_ZARCH_INSTALLED,        "z/Architecture architectural mode is installed" )
FT2( mod002,    NULL,      002_ZARCH_ACTIVE,           "z/Architecture architectural mode is active" )
FT2( mod003,    instr3,    003_DAT_ENHANCE_1,          "DAT-Enhancement Facility 1" )
FT2( mod004,    NULL,      004_IDTE_SC_SEGTAB,         "IDTE selective clearing when segment-table invalidated" )
FT2( mod005,    NULL,      005_IDTE_SC_REGTAB,         "IDTE selective clearing when region-table invalidated" )
FT2( NULL,      instr6,    006_ASN_LX_REUSE,           "ASN-and-LX-Reuse Facility" )
FT2( mod007,    instr7,    007_STFL_EXTENDED,          "Store-Facility-List-Extended Facility" )
FT2( mod008,    instr8,    008_EDAT_1,                 "Enhanced-DAT Facility 1" )
FT2( NULL,      NULL,      009_SENSE_RUN_STATUS,       "Sense-Running-Status Facility" )
FT2( mod010,    NULL,      010_CONDITIONAL_SSKE,       "Conditional-SSKE Facility" )
FT2( NULL,      instr11,   011_CONFIG_TOPOLOGY,        "Configuration-Topology Facility" )
FT2( NULL,      NULL,      012_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      013_IPTE_RANGE,             "IPTE-Range Facility" )
FT2( mod014,    NULL,      014_NONQ_KEY_SET,           "Nonquiescing Key-Setting Facility" )
FT2( NULL,      NULL,      015_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      instr16,   016_EXT_TRANSL_2,           "Extended-Translation Facility 2" )
FT2( NULL,      instr17,   017_MSA,                    "Message-Security Assist" )
FT2( mod018,    instr18,   018_LONG_DISPL_INST,        "Long-Displacement Facility" )
FT2( mod019,    NULL,      019_LONG_DISPL_HPERF,       "Long-Displacement Facility Has High Performance" )
FT2( NULL,      instr20,   020_HFP_MULT_ADD_SUB,       "HFP-Multiply-and-Add/Subtract Facility" )
FT2( NULL,      instr21,   021_EXTENDED_IMMED,         "Extended-Immediate Facility" )
FT2( NULL,      instr22,   022_EXT_TRANSL_3,           "Extended-Translation Facility 3" )
FT2( NULL,      instr23,   023_HFP_UNNORM_EXT,         "HFP-Unnormalized-Extensions Facility" )
FT2( NULL,      NULL,      024_ETF2_ENHANCEMENT,       "ETF2-Enhancement Facility" )
FT2( mod025,    instr25,   025_STORE_CLOCK_FAST,       "Store-Clock-Fast Facility" )
FT2( NULL,      instr26,   026_PARSING_ENHANCE,        "Parsing-Enhancement Facility" )
FT2( NULL,      instr27,   027_MVCOS,                  "Move-with-Optional-Specifications Facility" )
FT2( mod028,    instr28,   028_TOD_CLOCK_STEER,        "TOD-Clock-Steering Facility" )
FT2( NULL,      NULL,      029_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      030_ETF3_ENHANCEMENT,       "ETF3-Enhancement Facility" )
FT2( NULL,      instr31,   031_EXTRACT_CPU_TIME,       "Extract-CPU-Time Facility" )
FT2( NULL,      instr32,   032_CSSF,                   "Compare-and-Swap-and-Store Facility 1" )
FT2( NULL,      NULL,      033_CSSF2,                  "Compare-and-Swap-and-Store Facility 2" )
FT2( NULL,      instr34,   034_GEN_INST_EXTN,          "General-Instructions-Extension Facility" )
FT2( NULL,      instr35,   035_EXECUTE_EXTN,           "Execute-Extensions Facility" )
FT2( NULL,      NULL,      036_ENH_MONITOR,            "Enhanced-Monitor Facility" )
FT2( mod037,    instr37,   037_FP_EXTENSION,           "Floating-Point-Extension Facility" )
FT2( NULL,      NULL,      038_OP_CMPSC,               "Order-Preserving-Compression Facility" )
FT2( NULL,      NULL,      039_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( mod040,    instr40,   040_LOAD_PROG_PARAM,        "Load-Program-Parameter Facility" )
FT2( NULL,      instr41,   041_FPS_ENHANCEMENT,        "Floating-Point-Support-Enhancement Facility" )
FT2( NULL,      instr41,   041_DFP_ROUNDING,           "Decimal-Floating-Point-Rounding Facility" )
FT2( NULL,      instr41,   041_FPR_GR_TRANSFER,        "FPR-GR-Transfer Facility" )
FT2( NULL,      instr41,   041_FPS_SIGN_HANDLING,      "Floating-Point-Support-Sign-Handling Facility" )
FT2( NULL,      instr41,   041_IEEE_EXCEPT_SIM,        "IEEE-Exception-Simulation Facility" )
FT2( mod042,    instr42,   042_DFP,                    "Decimal-Floating-Point Facility" )
FT2( mod043,    NULL,      043_DFP_HPERF,              "Decimal-Floating-Point Facility Has High Performance" )
FT2( NULL,      instr44,   044_PFPO,                   "PFPO (Perform Floating-Point Operation) Facility" )
FT2( mod045,    instr45,   045_DISTINCT_OPERANDS,      "Distinct-Operands Facility" )
FT2( mod045,    instr45,   045_FAST_BCR_SERIAL,        "Fast-BCR-Serialization Facility" )
FT2( mod045,    instr45,   045_HIGH_WORD,              "High-Word Facility" )
FT2( mod045,    instr45,   045_INTERLOCKED_ACCESS_1,   "Interlocked-Access Facility 1" )
FT2( mod045,    instr45,   045_LOAD_STORE_ON_COND_1,   "Load/Store-on-Condition Facility 1" )
FT2( mod045,    instr45,   045_POPULATION_COUNT,       "Population-Count Facility" )
FT2( NULL,      NULL,      046_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      047_CMPSC_ENH,              "CMPSC-Enhancement Facility" )
FT2( mod048,    instr48,   048_DFP_ZONE_CONV,          "Decimal-Floating-Point-Zoned-Conversion Facility" )
FT2( mod049,    instr49,   049_EXECUTION_HINT,         "Execution-Hint Facility" )
FT2( mod049,    instr49,   049_LOAD_AND_TRAP,          "Load-and-Trap Facility" )
FT2( mod049,    instr49,   049_PROCESSOR_ASSIST,       "Processor-Assist Facility" )
FT2( mod049,    instr49,   049_MISC_INSTR_EXT_1,       "Miscellaneous-Instruction-Extensions Facility 1" )
FT2( mod050,    instr50,   050_CONSTR_TRANSACT,        "Constrained-Transactional-Execution Facility" )
FT2( mod051,    NULL,      051_LOCAL_TLB_CLEARING,     "Local-TLB-Clearing Facility" )
FT2( NULL,      NULL,      052_INTERLOCKED_ACCESS_2,   "Interlocked-Access Facility 2" )
FT2( NULL,      instr53,   053_LOAD_STORE_ON_COND_2,   "Load/Store-on-Condition Facility 2" )
FT2( NULL,      instr53,   053_LOAD_ZERO_RIGHTMOST,    "Load-and-Zero-Rightmost-Byte Facility" )
FT2( NULL,      NULL,      054_EE_CMPSC,               "Entropy-Encoding-Compression Facility" )
FT2( NULL,      NULL,      055_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      056_UNDEFINED,              "Undefined" )
FT2( NULL,      instr57,   057_MSA_EXTENSION_5,        "Message-Security-Assist Extension 5" )
FT2( NULL,      instr58,   058_MISC_INSTR_EXT_2,       "Miscellaneous-Instruction-Extensions Facility 2" )
FT2( NULL,      NULL,      059_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      060_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( mod061,    instr61,   061_MISC_INSTR_EXT_3,       "Miscellaneous-Instruction-Extensions Facility 3" )
FT2( NULL,      NULL,      062_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      063_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      064_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      065_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( mod066,    instr66,   066_RES_REF_BITS_MULT,      "Reset-Reference-Bits-Multiple Facility" )
FT2( mod067,    instr67,   067_CPU_MEAS_COUNTER,       "CPU-Measurement Counter Facility" )
FT2( mod068,    instr68,   068_CPU_MEAS_SAMPLNG,       "CPU-Measurement Sampling Facility" )
FT2( NULL,      NULL,      069_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      070_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      071_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      072_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( mod073,    instr73,   073_TRANSACT_EXEC,          "Transactional-Execution Facility" )
FT2( NULL,      instr74,   074_STORE_HYPER_INFO,       "Store-Hypervisor-Information Facility" )
FT2( NULL,      NULL,      075_ACC_EX_FS_INDIC,        "Access-Exception-Fetch/Store-Indication Facility" )
FT2( mod076,    instr76,   076_MSA_EXTENSION_3,        "Message-Security-Assist Extension 3" )
FT2( mod077,    instr77,   077_MSA_EXTENSION_4,        "Message-Security-Assist Extension 4" )
FT2( mod078,    instr78,   078_EDAT_2,                 "Enhanced-DAT Facility 2" )
FT2( NULL,      NULL,      079_UNDEFINED,              "Undefined" )
FT2( mod080,    instr80,   080_DFP_PACK_CONV,          "Decimal-Floating-Point-Packed-Conversion Facility" )
FT2( mod081,    NULL,      081_PPA_IN_ORDER,           "PPA-in-order Facility" )
FT2( NULL,      NULL,      082_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      083_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      084_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      085_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      086_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      087_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      088_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      089_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      090_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      091_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      092_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      093_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      094_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      095_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      096_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      097_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      098_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      099_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      100_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      101_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      102_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      103_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      104_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      105_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      106_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      107_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      108_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      109_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      110_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      111_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      112_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      113_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      114_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      115_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      116_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      117_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      118_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      119_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      120_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      121_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      122_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      123_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      124_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      125_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      126_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      127_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      128_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( mod129,    instr129,  129_ZVECTOR,                "Vector Facility for z/Architecture" )
FT2( NULL,      NULL,      130_INSTR_EXEC_PROT,        "Instruction-Execution-Protection Facility" )
FT2( NULL,      NULL,      131_SIDE_EFFECT_ACCESS,     "Side-Effect-Access Facility" )
FT2( NULL,      NULL,      131_ENH_SUPP_ON_PROT_2,     "Enhanced-Suppression-on-Protection Facility 2" )
FT2( NULL,      NULL,      132_UNDEFINED,              "Undefined" )
FT2( NULL,      instr133,  133_GUARDED_STORAGE,        "Guarded-Storage Facility" )
FT2( mod134,    instr134,  134_ZVECTOR_PACK_DEC,       "Vector Packed-Decimal Facility" )
FT2( mod135,    NULL,      135_ZVECTOR_ENH_1,          "Vector-Enhancements Facility 1" )
FT2( NULL,      NULL,      136_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      137_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      138_CONFIG_ZARCH_MODE,      "CZAM Facility (Configuration-z/Architecture-Architectural-Mode)" )
FT2( mod139,    NULL,      139_MULTIPLE_EPOCH,         "Multiple-Epoch Facility" )
FT2( NULL,      NULL,      140_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      141_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( mod142,    instr142,  142_ST_CPU_COUNTER_MULT,    "Store-CPU-Counter-Multiple Facility" )
FT2( NULL,      NULL,      143_UNDEFINED,              "Undefined" )
FT2( NULL,      instr144,  144_TEST_PEND_EXTERNAL,     "Test-Pending-External-Interruption Facility" )
FT2( mod145,    instr145,  145_INS_REF_BITS_MULT,      "Insert-Reference-Bits-Multiple Facility" )
FT2( mod146,    instr146,  146_MSA_EXTENSION_8,        "Message-Security-Assist Extension 8" )
FT2( NULL,      NULL,      147_IBM_RESERVED,           "Reserved for IBM use" )
FT2( mod148,    instr148,  148_VECTOR_ENH_2,           "Vector-Enhancements Facility 2" )
FT2( mod149,    NULL,      149_MOVEPAGE_SETKEY,        "Move-Page-and-Set-Key Facility" )
FT2( NULL,      instr150,  150_ENH_SORT,               "Enhanced-Sort Facility" )
FT2( NULL,      instr151,  151_DEFLATE_CONV,           "DEFLATE-Conversion Facility" )
FT2( mod152,    NULL,      152_VECT_PACKDEC_ENH,       "Vector-Packed-Decimal-Enhancement Facility" )
FT2( NULL,      NULL,      153_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      154_UNDEFINED,              "Undefined" )
FT2( mod155,    instr155,  155_MSA_EXTENSION_9,        "Message-Security-Assist Extension 9" )
FT2( NULL,      NULL,      156_IBM_INTERNAL,           "Assigned to IBM internal use" )
FT2( NULL,      NULL,      157_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      158_ULTRAV_CALL,            "Ultravisor-Call facility" )
FT2( NULL,      NULL,      159_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      160_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      161_SEC_EXE_UNPK,           "Secure-Execution-Unpack Facility" )
FT2( NULL,      NULL,      162_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      163_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      164_UNDEFINED,              "Undefined" )
FT2( mod165,    instr165,  165_NNET_ASSIST,            "Neural-Network-Processing-Assist Facility" )
FT2( NULL,      NULL,      166_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      167_UNDEFINED,              "Undefined" )
FT2( mod168,    NULL,      168_ESA390_COMPAT_MODE,     "ESA/390-Compatibility-Mode Facility" )
FT2( mod169,    NULL,      169_SKEY_REMOVAL,           "Storage-Key-Removal Facility" )
FT2( NULL,      NULL,      170_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      171_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      172_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      173_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      174_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      175_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      176_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      177_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      178_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      179_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      180_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      181_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      182_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      183_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      184_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      185_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      186_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      187_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      188_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      189_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      190_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      191_UNDEFINED,              "Undefined" )
FT2( mod192,    instr192,  192_VECT_PACKDEC_ENH_2,     "Vector-Packed-Decimal-Enhancement Facility 2" )
FT2( NULL,      instr193,  193_BEAR_ENH,               "BEAR-Enhancement Facility" )
FT2( mod194,    instr194,  194_RESET_DAT_PROT,         "Reset-DAT-Protection Facility" )
FT2( NULL,      NULL,      195_UNDEFINED,              "Undefined" )
FT2( mod196,    instr196,  196_PROC_ACT,               "Processor-Activity-Instrumentation Facility" )
FT2( mod197,    NULL,      197_PROC_ACT_EXT_1,         "Processor-Activity-Instrumentation Extension 1 Facility" )
FT2( NULL,      NULL,      198_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      199_UNDEFINED,              "Undefined" )
FT2( NULL,      NULL,      200_UNDEFINED,              "Undefined" )

/*-------------------------------------------------------------------*/
/*                   Hercules facilities                             */
/*-------------------------------------------------------------------*/

FT2( NULL,      herc37X,   HERC_370_EXTENSION,         "Hercules S/370 Instruction Extension Facility" )
FT2( NULL,      NULL,      HERC_DETECT_PGMINTLOOP,     "Hercules Detect-Program-Interrupt-Loop Support" )
FT2( NULL,      NULL,      HERC_HOST_RESOURCE_ACCESS,  "Hercules Host Resource Access Support" )
FT2( NULL,      NULL,      HERC_INTERVAL_TIMER,        "Hercules Interval Timer Support" )
FT2( NULL,      NULL,      HERC_LOGICAL_PARTITION,     "Hercules Logical Partition (LPAR) Support" )
FT2( NULL,      hercmvcin, HERC_MOVE_INVERSE,          "Hercules MVCIN Move Inverse Instruction Support" )
FT2( NULL,      NULL,      HERC_MSA_EXTENSION_1,       "Hercules Message-Security-Assist Extension 1 Support" )
FT2( NULL,      NULL,      HERC_MSA_EXTENSION_2,       "Hercules Message-Security-Assist Extension 2 Support" )
FT2( NULL,      NULL,      HERC_PROBSTATE_DIAGF08,     "Hercules Problem-State Diagnose X'F08' Support" )
FT2( NULL,      NULL,      HERC_QDIO_ASSIST,           "Hercules QDIO-Assist Support" )
FT2( NULL,      NULL,      HERC_QDIO_TDD,              "Hercules QDIO Time-Delayed-Dispatching Support" )
FT2( NULL,      NULL,      HERC_QDIO_THININT,          "Hercules QDIO Thin-Interrupts Support" )
FT2( NULL,      NULL,      HERC_QEBSM,                 "Hercules QDIO Enhanced Buffer-State Management Support" )
FT2( NULL,      NULL,      HERC_SIGP_SETARCH_S370,     "Hercules SIGP Set Architecture S/370 Support" )
FT2( NULL,      hercsvs,   HERC_SVS,                   "Hercules SVS Set Vector Summary Instruction Support" )
FT2( NULL,      NULL,      HERC_VIRTUAL_MACHINE,       "Hercules Emulate Virtual Machine Support" )
FT2( modtcp,    herctcp,   HERC_TCPIP_EXTENSION,       "Hercules Access Host TCP/IP Stack Through X'75' Instruction" )
FT2( modtcp,    NULL,      HERC_TCPIP_PROB_STATE,      "Hercules Enable X'75' As Problem State Instruction" )
FT2( NULL,      hercessa,  HERC_ZVM_ESSA,              "Hercules z/VM ESSA Extract and Set Storage Attributes instruction" )
};

/*-------------------------------------------------------------------*/
/*   Table of Facility Table POINTERS for each gen'ed architecture   */
/*-------------------------------------------------------------------*/

static const FACTAB* arch_factab[ NUM_GEN_ARCHS ] =
{
#if defined( _ARCH_NUM_0 )
  #if        _ARCH_NUM_0 == 370
                           s370_facs_tab,
  #elif      _ARCH_NUM_0 == 390
                           s390_facs_tab,
  #else //   _ARCH_NUM_0 == 900
                           z900_facs_tab,
  #endif
#endif
#if defined( _ARCH_NUM_1 )
  #if        _ARCH_NUM_1 == 370
                           s370_facs_tab,
  #elif      _ARCH_NUM_1 == 390
                           s390_facs_tab,
  #else //   _ARCH_NUM_1 == 900
                           z900_facs_tab,
  #endif
#endif
#if defined( _ARCH_NUM_2 )
  #if        _ARCH_NUM_2 == 370
                           s370_facs_tab,
  #elif      _ARCH_NUM_2 == 390
                           s390_facs_tab,
  #else //   _ARCH_NUM_2 == 900
                           z900_facs_tab,
  #endif
#endif
};

/*-------------------------------------------------------------------*/
/*   Table of Facility Table SIZES for each gen'ed architecture      */
/*-------------------------------------------------------------------*/

static const size_t num_arch_ft_entries[ NUM_GEN_ARCHS ] =
{
#if defined( _ARCH_NUM_0 )
  #if        _ARCH_NUM_0 == 370
                 _countof( s370_facs_tab ),
  #elif      _ARCH_NUM_0 == 390
                 _countof( s390_facs_tab ),
  #else //   _ARCH_NUM_0 == 900
                 _countof( z900_facs_tab ),
  #endif
#endif
#if defined( _ARCH_NUM_1 )
  #if        _ARCH_NUM_1 == 370
                 _countof( s370_facs_tab ),
  #elif      _ARCH_NUM_1 == 390
                 _countof( s390_facs_tab ),
  #else //   _ARCH_NUM_1 == 900
                 _countof( z900_facs_tab ),
  #endif
#endif
#if defined( _ARCH_NUM_2 )
  #if        _ARCH_NUM_2 == 370
                 _countof( s370_facs_tab ),
  #elif      _ARCH_NUM_2 == 390
                 _countof( s390_facs_tab ),
  #else //   _ARCH_NUM_2 == 900
                 _countof( z900_facs_tab ),
  #endif
#endif
};

static const FACTAB* get_factab_by_bitno( int bitno );  /* (fwd ref) */

/*-------------------------------------------------------------------*/
/*                   init_facilities_lists                  (public) */
/*-------------------------------------------------------------------*/
bool init_facilities_lists()
{
    /* Called ONCE by bldcfg.c 'build_config' during system startup. */

    const ARCHTAB* at;
    const FACTAB* ft;
    const char* archname;
    size_t i, n, bitno;
    int arch, fbyte, fbit;
    bool enable, rc = true;

    /* Merge each individual ARCH_DEP table entry into system factab */

    for (i=0; i < _countof( factab ); i++)
    {
        for (arch=0; arch < NUM_GEN_ARCHS; arch++)
        {
            for (n=0; n < num_arch_ft_entries[ arch ]; n++)
            {
                if (arch_factab[ arch ][n].bitno == factab[i].bitno)
                {
                    /* Merge this arch's masks into system factab */
                    factab[i].supmask |= arch_factab[ arch ][n].supmask;
                    factab[i].defmask |= arch_factab[ arch ][n].defmask;
                    factab[i].reqmask |= arch_factab[ arch ][n].reqmask;
                }
            }
        }
    }

    /* Clear each architectures' sysblk facilities lists to ZEROS */

    for (arch = 0; arch < NUM_GEN_ARCHS; arch++)
        for (fbyte = 0; fbyte < (int) STFL_HERC_BY_SIZE; fbyte++)
            sysblk.facility_list[ arch ][ fbyte ] = 0;

    /* Init each architectures' sysblk facilities to their default */

    for (arch = 0; arch < NUM_GEN_ARCHS; arch++)
    {
        at = get_archtab_by_arch( arch );

        for (i=0; i < _countof( factab ); i++)
        {
            /*  Initialize this architecture's facility bit only if
            **  this facility applies to this architecture and it's
            **  either a required facility or defaults to enabled.
            */
            if (1
                && (factab[i].supmask & at->amask)      // (applies?)
                && (0
                    || (factab[i].reqmask & at->amask)  // (required?)
                    || (factab[i].defmask & at->amask)  // (default?)
                   )
            )
            {
                fbyte =         (factab[i].bitno / 8);
                fbit  = 0x80 >> (factab[i].bitno % 8);

                sysblk.facility_list[ arch ][ fbyte ] |= fbit;
            }
        }
    }

    /* Sanity check each architectures' sysblk facilities list */

    for (arch = 0; arch < NUM_GEN_ARCHS; arch++)
    {
        archname = get_arch_name_by_arch( arch );

        for (bitno = 0; bitno <= STFL_HERC_LAST_BIT; bitno++)
        {
            /* Does a sanity check function exist for this facility? */
            if (1
                && (ft = get_factab_by_bitno( bitno ))
                && (ft->modokfunc)
            )
            {
                fbyte =         (bitno / 8);
                fbit  = 0x80 >> (bitno % 8);

                enable = (sysblk.facility_list[ arch ][ fbyte ] & fbit) ?
                    true : false;

                /* Sanity check this facility's setting */
                if (!ft->modokfunc( enable, bitno, arch, NULL, NULL, NULL, NULL ))
                {
                    // "%s facility %s fails consistency check"
                    WRMSG( HHC00899, "S", archname, ft->name );
                    rc = false;
                }
            }
        }
    }

    /* Exit immediately if any sanity/consistency checks failed */
    if (!rc)
        return rc;

    /* Enable or disable each facility's instructions as needed */

    for (arch = 0; arch < NUM_GEN_ARCHS; arch++)
    {
        at = get_archtab_by_arch( arch );

        for (bitno = 0; bitno <= STFL_HERC_LAST_BIT; bitno++)
        {
            /* Are there instructions associated with this facility? */
            if (1
                && (ft = get_factab_by_bitno( bitno ))
                && (ft->supmask & at->amask) // (applies to this arch?)
                && (ft->updinstrs)
            )
            {
                fbyte =         (bitno / 8);
                fbit  = 0x80 >> (bitno % 8);

                enable = (sysblk.facility_list[ arch ][ fbyte ] & fbit) ?
                    true : false;

                /* Enable or disable this facility's instructions */
                ft->updinstrs( arch, enable );
            }
        }
    }

    return rc;  // (true or false, success or failure)
}

/*-------------------------------------------------------------------*/
/*                      init_cpu_facilities                 (public) */
/*-------------------------------------------------------------------*/
void init_cpu_facilities( REGS* regs )
{
    // Called by ipl.c 'cpu_reset' and cpu.c 'run_cpu'.

    int  fbyte;
    for (fbyte = 0; fbyte < (int) STFL_HERC_BY_SIZE; fbyte++)
        regs->facility_list[ fbyte ] =
            sysblk.facility_list[ regs->arch_mode ][ fbyte ];
}

/*-------------------------------------------------------------------*/
/*                      get_factab_by_name                           */
/*-------------------------------------------------------------------*/
static const FACTAB* get_factab_by_name( const char* name )
{
    size_t i;
    for (i=0; i < _countof( factab ); i++)
        if (strcasecmp( factab[i].name, name ) == 0)
            return &factab[i];
    return NULL;
}

/*-------------------------------------------------------------------*/
/*                      get_factab_by_bitno                          */
/*-------------------------------------------------------------------*/
static const FACTAB* get_factab_by_bitno( int bitno )
{
    size_t i;

    if (0
        || bitno < 0
        || bitno > (int) STFL_HERC_LAST_BIT
    )
        return NULL;

    for (i=0; i < _countof( factab ); i++)
        if (factab[i].bitno == bitno)
            return &factab[i];
    return NULL;
}

/*-------------------------------------------------------------------*/
/*                     get_facname_by_bitno                          */
/*-------------------------------------------------------------------*/
static const char* get_facname_by_bitno( int bitno, const char** name )
{
    const FACTAB*  ft;
    if (!(ft = get_factab_by_bitno( bitno )))
        *name = NULL;
    else
        *name = ft->name;
    return *name;
}

/*-------------------------------------------------------------------*/
/*                     (qsort functions)                             */
/*-------------------------------------------------------------------*/
static int sort_ftpp_by_bit_number( const void* p1, const void* p2 )
{
    const FACTAB* f1 = *((const FACTAB**) p1);
    const FACTAB* f2 = *((const FACTAB**) p2);
    int rc = (f1->bitno - f2->bitno);
    return rc ? rc : strcasecmp( f1->name, f2->name );
}
static int sort_ftpp_by_long_name( const void* p1, const void* p2 )
{
    const FACTAB* f1 = *((const FACTAB**) p1);
    const FACTAB* f2 = *((const FACTAB**) p2);
    int rc = strcasecmp( f1->long_name, f2->long_name );
    return rc ? rc : (f1->bitno - f2->bitno);
}

/*-------------------------------------------------------------------*/
/*                          Query Type                               */
/*-------------------------------------------------------------------*/

enum eQueryType
{
    eQueryAll       = 0,
    eQueryEnabled   = 1,
    eQueryDisabled  = 2
};
typedef enum eQueryType EQUERY;

#define QUERY_ENABLED( e )      (e == eQueryEnabled )
#define QUERY_DISABLED( e )     (e == eQueryDisabled)

/*-------------------------------------------------------------------*/
/*                     facility_query_all                            */
/*-------------------------------------------------------------------*/

static bool facility_query_all( const ARCHTAB* at, const EQUERY eQType,
                                const bool sort_by_long )
{
    const FACTAB*   ft;         // ptr to FACTAB entry
    const FACTAB**  ftpp;       // ptr to ptr to FACTAB entry
    void*           ptr_array;  // ptr to array of FACTAB entry ptrs
    size_t  num;
    int     fbyte, fbit;
    bool    enabled;
    char    sup, def, req, cur, mod;
    const char* sev;

    /* Allocate an array of FACTAB entry pointers */

    if (!(ptr_array = malloc( sizeof( const FACTAB* ) * _countof( factab ))))
    {
        // "Out of memory"
        WRMSG( HHC00152, "E" );
        return false;
    }

    /* Populate array of FACTAB entry pointers */

    for (num = _countof( factab ), ftpp = ptr_array, ft = factab; num; num--, ftpp++, ft++)
        *ftpp = ft;

    /* Sort the array into the desired sequence */

    qsort( ptr_array, _countof( factab ), sizeof( FACTAB* ),
        sort_by_long ? sort_ftpp_by_long_name
                     : sort_ftpp_by_bit_number );

    /* Display column headers... */

    LOGMSG( "HHC00891I\n" );
    LOGMSG( "HHC00891I                             %s Facility Table\n", at->name );
    LOGMSG( "HHC00891I\n" );
    LOGMSG( "HHC00891I           SRDC* = Supported, Required, Default, Current, Modified.\n" );
    LOGMSG( "HHC00891I\n" );
    LOGMSG( "HHC00891I Bit By Bi SRDC* Facility                    Description\n" );
    LOGMSG( "HHC00891I --- -- -- ----- --------------------------- ---------------------------------------------------------\n" );
    LOGMSG( "HHC00891I\n" );

    /* Display FACTAB entries for current architecture */

    for (num = _countof( factab ), ftpp = ptr_array; num; num--, ftpp++)
    {
        ft = *ftpp;

        fbyte =         (ft->bitno / 8);
        fbit  = 0x80 >> (ft->bitno % 8);

        enabled = sysblk.facility_list[ at->num ][ fbyte ] & fbit;

        if (QUERY_ENABLED ( eQType ) && !enabled) continue;
        if (QUERY_DISABLED( eQType ) &&  enabled) continue;

        sup = (ft->supmask & at->amask)     ? 'Y' : '-';
        req = (ft->reqmask & at->amask)     ? 'Y' : '-';
        def = (ft->defmask & at->amask)     ? '1' : '0';
        cur = (enabled)                     ? '1' : '0';
        mod = (def == '1' && cur == '0') ||
              (def == '0' && cur == '1')    ? '*' : ' ';
        sev = (mod == '*')                  ? "W" : "I";

        // "%3d %02X %02X %c%c%c%c%c %-27s%c%s"
        WRMSG( HHC00891, sev,
            ft->bitno,
            fbyte,
            fbit,
            sup,
            req,
            def,
            cur,
            mod,
            ft->name,
            mod,
            ft->long_name
        );
    }

    LOGMSG( "HHC00891I\n" );
    LOGMSG( "HHC00891I           SRDC* = Supported, Required, Default, Current, Modified.\n" );
    LOGMSG( "HHC00891I\n" );

    free( ptr_array );
    return true;
}

/*-------------------------------------------------------------------*/
/*                     facility_query_raw                            */
/*-------------------------------------------------------------------*/
static void facility_query_raw( const ARCHTAB* at )
{
    char buf[ 128 ] = {0};
    char wrk[  20 ];
    size_t i;

    for (i=0; i < STFL_IBM_DW_SIZE; i++)
    {
        MSGBUF( wrk, "%02X%02X%02X%02X %02X%02X%02X%02X "

            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 0 ]
            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 1 ]
            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 2 ]
            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 3 ]

            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 4 ]
            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 5 ]
            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 6 ]
            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 7 ]
        );
        STRLCAT( buf, wrk );
    }

    RTRIM( buf );
    STRLCAT( buf, ", HERC: " );

    for (; i < STFL_HERC_DW_SIZE; i++)
    {
        MSGBUF( wrk, "%02X%02X%02X%02X %02X%02X%02X%02X "

            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 0 ]
            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 1 ]
            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 2 ]
            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 3 ]

            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 4 ]
            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 5 ]
            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 6 ]
            , sysblk.facility_list[ at->num ][ (i * sizeof( DW )) + 7 ]
        );
        STRLCAT( buf, wrk );
    }

    // "%s facility list: %s"
    WRMSG( HHC00894, "I", at->name, RTRIM( buf ));
}

/*-------------------------------------------------------------------*/
/*                       facility_query                    (boolean) */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  FACILITY QUERY ENABLED | DISABLED    [ LONG ]                    */
/*  FACILITY QUERY SHORT | LONG | ALL                                */
/*  FACILITY QUERY <facility> | bit                                  */
/*  FACILITY QUERY RAW                                               */
/*                                                                   */
/*-------------------------------------------------------------------*/
bool facility_query( int argc, char* argv[] )
{
    const ARCHTAB*  at;
    const FACTAB*   ft;
    const char*     p;
    const char*     sev;
    int             bitno, fbyte, fbit;
    bool            forced, enabled, modified;
    char            c;

    /* Note: we know argc >= 2, otherwise why would we be called? */

    if (argc > 4)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return false;
    }

    /* Get pointer to ARCHTAB entry for current architecture */
    at = get_archtab_by_arch( sysblk.arch_mode );

    /* Query ALL/SHORT/LONG/ENABLED/DISABLED? */

    if (argc == 2 ||                                  // (implicit ALL)
       (argc >= 3 && (0
                      || CMD( argv[2], ALL,      1 )  // (explicit ALL)
                      || CMD( argv[2], SHORT,    1 )  // (default sort)
                      || CMD( argv[2], LONG,     1 )  // (by long name)
                      || CMD( argv[2], ENABLED,  1 )  // (only enabled)
                      || CMD( argv[2], DISABLED, 1 )  // (only disabled)
                     )
    ))
    {
        const EQUERY eQType      = argc  <   3                   ? eQueryAll
                                 : CMD( argv[3-1], ENABLED,  1 ) ? eQueryEnabled
                                 : CMD( argv[3-1], DISABLED, 1 ) ? eQueryDisabled : eQueryAll;

        const bool sort_by_long  = argc  <   3               ? false
                                 : CMD( argv[3-1], LONG, 1 ) ? true
                                 : argc  <   4               ? false
                                 : CMD( argv[4-1], LONG, 1 ) ? true : false;

        return facility_query_all( at, eQType, sort_by_long );
    }

    /* Query RAW? */

    if (argc == 3 && CMD( argv[2], RAW, 1 ))
    {
        facility_query_raw( at );
        return true;
    }

    /* Get pointer to requested facility table entry */

    if (strncasecmp( "BIT", p = argv[2], 3 ) == 0)
        p += 3;

    if (1
        && isdigit( (unsigned char)*p )
        && sscanf( p, "%d%c", &bitno, &c ) == 1
        && bitno >= 0
        && bitno <= (int) STFL_HERC_LAST_BIT
    )
    {
        ft = get_factab_by_bitno( bitno );
        forced = true;
    }
    else
    {
        ft = get_factab_by_name( argv[2] );
        forced = false;
    }

    /* Does this facility exist for the current architecture? */

    if (!ft)
    {
        // "Facility( %s ) does not exist for %s"
        WRMSG( HHC00892, "E", argv[2], at->name );
        return false;
    }

    /* Get byte and bit number */

    fbyte =         (ft->bitno / 8);
    fbit  = 0x80 >> (ft->bitno % 8);

    /* Set some flags */

    enabled  = sysblk.facility_list[ at->num ][ fbyte ] & fbit;

    modified = ( (ft->defmask & at->amask) && !enabled) ||
               (!(ft->defmask & at->amask) &&  enabled);

    /* Is this facility supported for the current architecture? */

    if (1
        && !forced
        && !modified
        && !(ft->supmask & at->amask)
    )
    {
        // "Facility( %s ) not supported for %s"
        WRMSG( HHC00893, "E", ft->name, at->name );
        return false;
    }

    /* Display facility setting */

    sev = modified ? "W" : "I";

    // "Facility( %s ) %s%s for %s"
    WRMSG( HHC00898, sev, ft->name, modified ? "*" : "",
        enabled ? "Enabled" : "Disabled", at->name );

    return true;
}

/*-------------------------------------------------------------------*/
/*                         _hhc00890e                                */
/*-------------------------------------------------------------------*/
/*      "Cannot %s facility %s without first %s facility %s"         */
/*-------------------------------------------------------------------*/
static bool _hhc00890e( int target_bit, const char* target_facname,
                        const char* action, const char* actioning,
                        int prerequisite_bit, const char* file,
                        int line, const char* func )
{
    const char* prerequisite_facname;

    fwritemsg( file, line, func, WRMSG_NORMAL, stdout, MSG( HHC00890, "E",
        action,    get_facname_by_bitno( target_bit,       &target_facname       ),
        actioning, get_facname_by_bitno( prerequisite_bit, &prerequisite_facname )));

    return false;
}

/*-------------------------------------------------------------------*/
/*                    HHC00890E / HHC00890E_OPP                      */
/*-------------------------------------------------------------------*/
/* Helper macro to return false immediately if no error message is   */
/* wanted (action == NULL) or call helper function to issue message  */
/* which always returns false.                                       */
/*-------------------------------------------------------------------*/
#define HHC00890E( prereq_bit )     (!action? false : _hhc00890e    \
    ( bitno, target_facname, action, actioning, (prereq_bit),       \
        __FILE__, __LINE__, __FUNCTION__ ))
#define HHC00890E_OPP( prereq_bit ) (!action? false : _hhc00890e    \
    ( bitno, target_facname, action, opp_actioning, (prereq_bit),   \
        __FILE__, __LINE__, __FUNCTION__ ))

/*-------------------------------------------------------------------*/
/*               Facility Modification Check functions               */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The following series of functions are optionally defined in the   */
/* primary 'FT2' facilities table and check whether that facility    */
/* can be disabled or enabled depending on whether another facility  */
/* is enabled or disabled. The Long-Displacement Facility (bit 18)   */
/* cannot be disabled for example if the Long-Displacement Facility  */
/* Has High Performance facility (bit 19) is still enabled. Rather,  */
/* the Long-Displacement Facility Has High Performance bit 19 must   */
/* be disabled first. The below functions enforce such restrictions. */
/*                                                                   */
/*  Refer to the README.FACILITIES document for both instructions on */
/*  how to update Hercules facilities as well as for a handy table   */
/*  that documents interfacility dependencies and incompatibilities. */
/*                                                                   */
/*-------------------------------------------------------------------*/

#define FAC_MOD_OK_FUNC( name )                                     \
                                                                    \
static bool name( bool         enable,                              \
                  int          bitno,                               \
                  int          archnum,                             \
                  const char*  action,                              \
                  const char*  actioning,                           \
                  const char*  opp_actioning,                       \
                  const char* target_facname )                      \

/*-------------------------------------------------------------------*/
/*                          mod000                                   */
/*-------------------------------------------------------------------*/
/*  STFLE implies STFL, so 7 implies 0. Therefore if 7 is on, then   */
/* you can't disable 0, and can't enable 7 without also enabling 0.  */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod000 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 007_STFL_EXTENDED, archnum ))
            return HHC00890E( STFL_007_STFL_EXTENDED );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod002                                   */
/*-------------------------------------------------------------------*/
/*  bit 2 zero, bit 168 zero  =  ESA/390 Architecture mode           */
/*  bit 2 zero, bit 168 one   =  ESA/390-COMPATIBILITY mode          */
/*  bit 2 one,  bit 168 zero  =  z/Architecture mode                 */
/*  bit 2 one,  bit 168 one   =  INVALID                             */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod002 )
{
    UNREFERENCED( actioning );

    if (enable)
    {
        if (FACILITY_ENABLED_ARCH(     168_ESA390_COMPAT_MODE, archnum ))
            return HHC00890E_OPP( STFL_168_ESA390_COMPAT_MODE );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod003                                   */
/*-------------------------------------------------------------------*/
/*                 bit 3 is one if bit 4 is one                      */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod003 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 004_IDTE_SC_SEGTAB, archnum ))
            return HHC00890E( STFL_004_IDTE_SC_SEGTAB );

        if (FACILITY_ENABLED_ARCH( 005_IDTE_SC_REGTAB, archnum ))
            return HHC00890E( STFL_005_IDTE_SC_REGTAB );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod004                                   */
/*-------------------------------------------------------------------*/
/*                 bit 3 is one if bit 4 is one                      */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod004 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 003_DAT_ENHANCE_1, archnum ))
            return HHC00890E(  STFL_003_DAT_ENHANCE_1 );
    }
    else // disabling
    {
        if (FACILITY_ENABLED_ARCH( 005_IDTE_SC_REGTAB, archnum ))
            return HHC00890E( STFL_005_IDTE_SC_REGTAB );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod005                                   */
/*-------------------------------------------------------------------*/
/*              bits 3 and 4 are ones if bit 5 is one                */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod005 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 003_DAT_ENHANCE_1, archnum ))
            return HHC00890E(  STFL_003_DAT_ENHANCE_1 );

        if (!FACILITY_ENABLED_ARCH( 004_IDTE_SC_SEGTAB, archnum ))
            return HHC00890E(  STFL_004_IDTE_SC_SEGTAB );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod007                                   */
/*-------------------------------------------------------------------*/
/*  STFLE implies STFL, so 7 implies 0. Therefore if 7 is on, then   */
/* you can't disable 0, and can't enable 7 without also enabling 0.  */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod007 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 000_N3_INSTR, archnum ))
            return HHC00890E(  STFL_000_N3_INSTR );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                            mod008                                 */
/*-------------------------------------------------------------------*/
/*               Bit 8 is also one when bit 78 is one.               */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC             ( mod008 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 078_EDAT_2, archnum ))
            return HHC00890E( STFL_078_EDAT_2 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                            mod010                                 */
/*-------------------------------------------------------------------*/
/*                     incompatible with 169                         */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC             ( mod010 )
{
    UNREFERENCED( actioning );

    if (enable)
    {
        if (FACILITY_ENABLED_ARCH(     169_SKEY_REMOVAL, archnum ))
            return HHC00890E_OPP( STFL_169_SKEY_REMOVAL );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                            mod014                                 */
/*-------------------------------------------------------------------*/
/*             required by 149; incompatible with 169                */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC             ( mod014 )
{
    UNREFERENCED( actioning );

    if (enable)
    {
        if (FACILITY_ENABLED_ARCH(     169_SKEY_REMOVAL, archnum ))
            return HHC00890E_OPP( STFL_169_SKEY_REMOVAL );
    }
    else // disabling
    {
        if (FACILITY_ENABLED_ARCH( 149_MOVEPAGE_SETKEY, archnum ))
            return HHC00890E( STFL_149_MOVEPAGE_SETKEY );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod018                                   */
/*-------------------------------------------------------------------*/
/*                       required by 19                              */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod018 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 019_LONG_DISPL_HPERF, archnum ))
            return HHC00890E( STFL_019_LONG_DISPL_HPERF );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod019                                   */
/*-------------------------------------------------------------------*/
/*                     also requires 18                              */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod019 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 018_LONG_DISPL_INST, archnum ))
            return HHC00890E(  STFL_018_LONG_DISPL_INST );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod025                                  */
/*-------------------------------------------------------------------*/
/*             bits 25 and 28 are one when bit 139 is one            */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod025 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 139_MULTIPLE_EPOCH, archnum ))
            return HHC00890E( STFL_139_MULTIPLE_EPOCH );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod028                                  */
/*-------------------------------------------------------------------*/
/*             bits 25 and 28 are one when bit 139 is one            */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod028 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 139_MULTIPLE_EPOCH, archnum ))
            return HHC00890E( STFL_139_MULTIPLE_EPOCH );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                            mod037                                 */
/*-------------------------------------------------------------------*/
/*                    also requires bit 42                           */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC             ( mod037 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 042_DFP, archnum ))
            return HHC00890E(  STFL_042_DFP );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod040                                  */
/*-------------------------------------------------------------------*/
/*                       required by 68                              */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod040 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 068_CPU_MEAS_SAMPLNG, archnum ))
            return HHC00890E( STFL_068_CPU_MEAS_SAMPLNG );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod042                                  */
/*-------------------------------------------------------------------*/
/*                    required by 37 and 43                          */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod042 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 037_FP_EXTENSION, archnum ))
            return HHC00890E( STFL_037_FP_EXTENSION );

        if (FACILITY_ENABLED_ARCH( 043_DFP_HPERF, archnum ))
            return HHC00890E( STFL_043_DFP_HPERF );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod043                                  */
/*-------------------------------------------------------------------*/
/*                    bit 43 implies bit 42                          */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod043 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 042_DFP, archnum ))
            return HHC00890E(  STFL_042_DFP );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                            mod045                                 */
/*-------------------------------------------------------------------*/
/*               Bit 45 is also one when bit 61 is one.              */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC             ( mod045 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 061_MISC_INSTR_EXT_3, archnum ))
            return HHC00890E( STFL_061_MISC_INSTR_EXT_3 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                            mod048                                 */
/*-------------------------------------------------------------------*/
/*                     bit 48 implies bit 42                         */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC             ( mod048 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 042_DFP, archnum ))
            return HHC00890E(  STFL_042_DFP );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod049                                   */
/*-------------------------------------------------------------------*/
/*                     required by 73, 81                            */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod049 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 073_TRANSACT_EXEC, archnum ))
            return HHC00890E( STFL_073_TRANSACT_EXEC );

        if (FACILITY_ENABLED_ARCH( 081_PPA_IN_ORDER, archnum ))
            return HHC00890E( STFL_081_PPA_IN_ORDER );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod050                                   */
/*-------------------------------------------------------------------*/
/*                      also requires 73                             */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod050 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 073_TRANSACT_EXEC, archnum ))
            return HHC00890E(  STFL_073_TRANSACT_EXEC );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod051                                   */
/*-------------------------------------------------------------------*/
/*                      required by 194                              */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod051 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 194_RESET_DAT_PROT, archnum ))
            return HHC00890E( STFL_194_RESET_DAT_PROT );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                            mod061                                 */
/*-------------------------------------------------------------------*/
/*               Bit 45 is also one when bit 61 is one.              */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC             ( mod061 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 045_POPULATION_COUNT, archnum ))
            return HHC00890E(  STFL_045_POPULATION_COUNT );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                            mod066                                 */
/*-------------------------------------------------------------------*/
/*                     incompatible with 169                         */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC             ( mod066 )
{
    UNREFERENCED( actioning );

    if (enable)
    {
        if (FACILITY_ENABLED_ARCH(     169_SKEY_REMOVAL, archnum ))
            return HHC00890E_OPP( STFL_169_SKEY_REMOVAL );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod067                                  */
/*-------------------------------------------------------------------*/
/*                     required by 68, 142                           */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod067 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 068_CPU_MEAS_SAMPLNG, archnum ))
            return HHC00890E( STFL_068_CPU_MEAS_SAMPLNG );

        if (FACILITY_ENABLED_ARCH( 142_ST_CPU_COUNTER_MULT, archnum ))
            return HHC00890E( STFL_142_ST_CPU_COUNTER_MULT );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod068                                  */
/*-------------------------------------------------------------------*/
/*                      also requires 40, 67                         */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod068 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 040_LOAD_PROG_PARAM, archnum ))
            return HHC00890E(  STFL_040_LOAD_PROG_PARAM );

        if (!FACILITY_ENABLED_ARCH( 067_CPU_MEAS_COUNTER, archnum ))
            return HHC00890E(  STFL_067_CPU_MEAS_COUNTER );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod073                                   */
/*-------------------------------------------------------------------*/
/*             also requires 49; required by 50                      */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod073 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 049_PROCESSOR_ASSIST, archnum ))
            return HHC00890E(  STFL_049_PROCESSOR_ASSIST );

        txf_model_warning( true );
        txf_set_timerint( true );
    }
    else // disabling
    {
        if (FACILITY_ENABLED_ARCH( 050_CONSTR_TRANSACT, archnum ))
            return HHC00890E( STFL_050_CONSTR_TRANSACT );

        txf_set_timerint( false );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod076                                  */
/*-------------------------------------------------------------------*/
/*                    required by 146, 155                           */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod076 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 146_MSA_EXTENSION_8, archnum ))
            return HHC00890E( STFL_146_MSA_EXTENSION_8 );

        if (FACILITY_ENABLED_ARCH( 155_MSA_EXTENSION_9, archnum ))
            return HHC00890E( STFL_155_MSA_EXTENSION_9 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod077                                  */
/*-------------------------------------------------------------------*/
/*                      required by 155                              */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod077 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 155_MSA_EXTENSION_9, archnum ))
            return HHC00890E( STFL_155_MSA_EXTENSION_9 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                            mod078                                 */
/*-------------------------------------------------------------------*/
/*               Bit 8 is also one when bit 78 is one.               */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC             ( mod078 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 008_EDAT_1, archnum ))
            return HHC00890E(  STFL_008_EDAT_1 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                            mod080                                 */
/*-------------------------------------------------------------------*/
/*                       also requires 42                            */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC             ( mod080 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 042_DFP, archnum ))
            return HHC00890E(  STFL_042_DFP );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod081                                   */
/*-------------------------------------------------------------------*/
/*                     also requires 49                              */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod081 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 049_PROCESSOR_ASSIST, archnum ))
            return HHC00890E(  STFL_049_PROCESSOR_ASSIST );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod129                                  */
/*-------------------------------------------------------------------*/
/*             required by 134, 135, 148, 152, 165, 192              */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod129 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 134_ZVECTOR_PACK_DEC, archnum ))
            return HHC00890E( STFL_134_ZVECTOR_PACK_DEC );

        if (FACILITY_ENABLED_ARCH( 135_ZVECTOR_ENH_1, archnum ))
            return HHC00890E( STFL_135_ZVECTOR_ENH_1 );

        if (FACILITY_ENABLED_ARCH( 148_VECTOR_ENH_2, archnum ))
            return HHC00890E( STFL_148_VECTOR_ENH_2 );

        if (FACILITY_ENABLED_ARCH( 152_VECT_PACKDEC_ENH, archnum ))
            return HHC00890E( STFL_152_VECT_PACKDEC_ENH );

        if (FACILITY_ENABLED_ARCH( 165_NNET_ASSIST, archnum ))
            return HHC00890E( STFL_165_NNET_ASSIST );

        if (FACILITY_ENABLED_ARCH( 192_VECT_PACKDEC_ENH_2, archnum ))
            return HHC00890E( STFL_192_VECT_PACKDEC_ENH_2 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod134                                  */
/*-------------------------------------------------------------------*/
/*              also requires 129; required by 152, 192              */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod134 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 129_ZVECTOR, archnum ))
            return HHC00890E(  STFL_129_ZVECTOR );
    }
    else // disabling
    {
        if (FACILITY_ENABLED_ARCH( 152_VECT_PACKDEC_ENH, archnum ))
            return HHC00890E( STFL_152_VECT_PACKDEC_ENH );

        if (FACILITY_ENABLED_ARCH( 192_VECT_PACKDEC_ENH_2, archnum ))
            return HHC00890E( STFL_192_VECT_PACKDEC_ENH_2 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod135                                  */
/*-------------------------------------------------------------------*/
/*               also requires 129; required by 148                  */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod135 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 129_ZVECTOR, archnum ))
            return HHC00890E(  STFL_129_ZVECTOR );
    }
    else // disabling
    {
        if (FACILITY_ENABLED_ARCH( 148_VECTOR_ENH_2, archnum ))
            return HHC00890E( STFL_148_VECTOR_ENH_2 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod139                                  */
/*-------------------------------------------------------------------*/
/*                     also requires 25, 28                          */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod139 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 025_STORE_CLOCK_FAST, archnum ))
            return HHC00890E(  STFL_025_STORE_CLOCK_FAST );

        if (!FACILITY_ENABLED_ARCH( 028_TOD_CLOCK_STEER, archnum ))
            return HHC00890E(  STFL_028_TOD_CLOCK_STEER );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod142                                  */
/*-------------------------------------------------------------------*/
/*                       also requires 67                            */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod142 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 067_CPU_MEAS_COUNTER, archnum ))
            return HHC00890E(  STFL_067_CPU_MEAS_COUNTER );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod145                                  */
/*-------------------------------------------------------------------*/
/*                    incompatible with 169                          */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod145 )
{
    UNREFERENCED( actioning );

    if (enable)
    {
        if (FACILITY_ENABLED_ARCH(     169_SKEY_REMOVAL, archnum ))
            return HHC00890E_OPP( STFL_169_SKEY_REMOVAL );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod146                                  */
/*-------------------------------------------------------------------*/
/*                      also requires 76                             */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod146 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 076_MSA_EXTENSION_3, archnum ))
            return HHC00890E(  STFL_076_MSA_EXTENSION_3 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod148                                  */
/*-------------------------------------------------------------------*/
/*                    also requires 129, 135                         */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod148 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 129_ZVECTOR, archnum ))
            return HHC00890E(  STFL_129_ZVECTOR );

        if (!FACILITY_ENABLED_ARCH( 135_ZVECTOR_ENH_1, archnum ))
            return HHC00890E(  STFL_135_ZVECTOR_ENH_1 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod149                                  */
/*-------------------------------------------------------------------*/
/*                      also requires 14                             */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod149 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 014_NONQ_KEY_SET, archnum ))
            return HHC00890E(  STFL_014_NONQ_KEY_SET );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod152                                  */
/*-------------------------------------------------------------------*/
/*               also requires 129, 134; required by 192             */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod152 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 129_ZVECTOR, archnum ))
            return HHC00890E(  STFL_129_ZVECTOR );

        if (!FACILITY_ENABLED_ARCH( 134_ZVECTOR_PACK_DEC, archnum ))
            return HHC00890E(  STFL_134_ZVECTOR_PACK_DEC );
    }
    else // disabling
    {
        if (FACILITY_ENABLED_ARCH( 192_VECT_PACKDEC_ENH_2, archnum ))
            return HHC00890E(  STFL_192_VECT_PACKDEC_ENH_2 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod155                                  */
/*-------------------------------------------------------------------*/
/*                   also requires 76 and 77                         */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod155 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 076_MSA_EXTENSION_3, archnum ))
            return HHC00890E(  STFL_076_MSA_EXTENSION_3 );

        if (!FACILITY_ENABLED_ARCH( 077_MSA_EXTENSION_4, archnum ))
            return HHC00890E(  STFL_077_MSA_EXTENSION_4 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod165                                  */
/*-------------------------------------------------------------------*/
/*                      also requires 129                            */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod165 )
{
    UNREFERENCED( actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH(    129_ZVECTOR, archnum ))
            return HHC00890E_OPP( STFL_129_ZVECTOR );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod168                                   */
/*-------------------------------------------------------------------*/
/*  bit 2 zero, bit 168 zero  =  ESA/390 Architecture mode           */
/*  bit 2 one,  bit 168 zero  =  z/Architecture mode                 */
/*  bit 2 zero, bit 168 one   =  ESA/390-COMPATIBILITY mode          */
/*  bit 2 one,  bit 168 one   =  INVALID                             */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod168 )
{
    UNREFERENCED( actioning );

    if (enable)
    {
        if (FACILITY_ENABLED_ARCH(     002_ZARCH_ACTIVE, archnum ))
            return HHC00890E_OPP( STFL_002_ZARCH_ACTIVE );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod169                                  */
/*-------------------------------------------------------------------*/
/*            incompatible with 010, 014, 066, 145, 149              */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod169 )
{
    UNREFERENCED( actioning );

    if (enable)
    {
        if (FACILITY_ENABLED_ARCH(     010_CONDITIONAL_SSKE, archnum ))
            return HHC00890E_OPP( STFL_010_CONDITIONAL_SSKE );

        if (FACILITY_ENABLED_ARCH(     014_NONQ_KEY_SET, archnum ))
            return HHC00890E_OPP( STFL_014_NONQ_KEY_SET );

        if (FACILITY_ENABLED_ARCH(     066_RES_REF_BITS_MULT, archnum ))
            return HHC00890E_OPP( STFL_066_RES_REF_BITS_MULT );

        if (FACILITY_ENABLED_ARCH(     145_INS_REF_BITS_MULT, archnum ))
            return HHC00890E_OPP( STFL_145_INS_REF_BITS_MULT );

        if (FACILITY_ENABLED_ARCH(     149_MOVEPAGE_SETKEY, archnum ))
            return HHC00890E_OPP( STFL_149_MOVEPAGE_SETKEY );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod192                                  */
/*-------------------------------------------------------------------*/
/*                  also requires 129, 134, 152                      */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod192 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 129_ZVECTOR , archnum ))
            return HHC00890E(  STFL_129_ZVECTOR );

        if (!FACILITY_ENABLED_ARCH( 134_ZVECTOR_PACK_DEC, archnum ))
            return HHC00890E(  STFL_134_ZVECTOR_PACK_DEC );

        if (!FACILITY_ENABLED_ARCH( 152_VECT_PACKDEC_ENH, archnum ))
            return HHC00890E(  STFL_152_VECT_PACKDEC_ENH );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                           mod194                                  */
/*-------------------------------------------------------------------*/
/*                      also requires 51                             */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC            ( mod194 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 051_LOCAL_TLB_CLEARING, archnum ))
            return HHC00890E(  STFL_051_LOCAL_TLB_CLEARING );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod196                                   */
/*-------------------------------------------------------------------*/
/*                     required by 197                             s  */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod196 )
{
    UNREFERENCED( opp_actioning );

    if (!enable) // disabling
    {
        if (FACILITY_ENABLED_ARCH( 197_PROC_ACT_EXT_1, archnum ))
            return HHC00890E( STFL_197_PROC_ACT_EXT_1 );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                          mod197                                   */
/*-------------------------------------------------------------------*/
/*                     also requires 196                             */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC           ( mod197 )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (!FACILITY_ENABLED_ARCH( 196_PROC_ACT, archnum ))
            return HHC00890E(  STFL_196_PROC_ACT );
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                            modtcp                                 */
/*-------------------------------------------------------------------*/
/*     'HERC_TCPIP_PROB_STATE' implies 'HERC_TCPIP_EXTENSION'        */
/*-------------------------------------------------------------------*/
FAC_MOD_OK_FUNC             ( modtcp )
{
    UNREFERENCED( opp_actioning );

    if (enable)
    {
        if (bitno == STFL_HERC_TCPIP_PROB_STATE)
        {
            if (!FACILITY_ENABLED_ARCH( HERC_TCPIP_EXTENSION, archnum ))
                return HHC00890E(  STFL_HERC_TCPIP_EXTENSION );
        }
    }
    else // disabling
    {
        if (bitno == STFL_HERC_TCPIP_EXTENSION)
        {
            if (FACILITY_ENABLED_ARCH( HERC_TCPIP_PROB_STATE, archnum ))
                return HHC00890E( STFL_HERC_TCPIP_PROB_STATE );
        }
    }
    return true;
}

/*-------------------------------------------------------------------*/
/*                        ena_fac_ins                                */
/*-------------------------------------------------------------------*/
/* The following function is called whenever a facility is enabled   */
/* and reverses the effects of the below dis_fac_ins function by     */
/* deleting the chain of instruction overrides thereby restoring     */
/* the opcode table to its default build value.                      */
/*-------------------------------------------------------------------*/
static void ena_fac_ins( int arch, HDLINS* hdl_ins[] )
{
    HDLINS* next_ins;

    /* Restore original function for each instruction disabled */
    while (hdl_ins[ arch ])
    {
        /* (restore original function) */
        hdl_repins( false, hdl_ins[ arch ] );

        /* (discard HDLINS entry) */
        free( hdl_ins[ arch ]->instname );
        next_ins = hdl_ins[ arch ]->next;
        free( hdl_ins[ arch ] );
        hdl_ins[ arch ] = next_ins;
    }
}

/*-------------------------------------------------------------------*/
/*                         dis_fac_ins                               */
/*-------------------------------------------------------------------*/
/* The following function is called whenever a facility is disabled  */
/* and updates the opcode table for a given facility's instruction   */
/* to point instead to our "facility_not_enabled" function further   */
/* above to cause an immediate operation exception program check.    */
/*-------------------------------------------------------------------*/
static void dis_fac_ins( int arch, HDLINS** ppHDLINS, int opcode, const char* instname )
{
    static const INSTR_FUNC pgmck_instr_func_tab[ NUM_GEN_ARCHS ] =
    {
#if defined(       _ARCH_NUM_0 )
   #if      370 == _ARCH_NUM_0
          &s370_facility_not_enabled,

   #elif    390 == _ARCH_NUM_0
          &s390_facility_not_enabled,

   #else // 900 == _ARCH_NUM_0
          &z900_facility_not_enabled,
   #endif
#endif
#if defined(       _ARCH_NUM_1 )
   #if      370 == _ARCH_NUM_1
          &s370_facility_not_enabled,

   #elif    390 == _ARCH_NUM_1
          &s390_facility_not_enabled,

   #else // 900 == _ARCH_NUM_1
          &z900_facility_not_enabled,
   #endif
#endif
#if defined(       _ARCH_NUM_2 )
   #if      370 == _ARCH_NUM_2
          &s370_facility_not_enabled,

   #elif    390 == _ARCH_NUM_2
          &s390_facility_not_enabled,

   #else // 900 == _ARCH_NUM_2
          &z900_facility_not_enabled,
   #endif
#endif
    };

    static const int arch_to_hdl_arch_tab[ NUM_GEN_ARCHS ] =
    {
#if defined( _ARCH_NUM_0 )
  #if        _ARCH_NUM_0 == 370
               HDL_INSTARCH_370,

  #elif      _ARCH_NUM_0 == 390
               HDL_INSTARCH_390,

  #else //   _ARCH_NUM_0 == 900
               HDL_INSTARCH_900,
  #endif
#endif
#if defined( _ARCH_NUM_1 )
  #if        _ARCH_NUM_1 == 370
               HDL_INSTARCH_370,

  #elif      _ARCH_NUM_1 == 390
               HDL_INSTARCH_390,

  #else //   _ARCH_NUM_1 == 900
               HDL_INSTARCH_900,
  #endif
#endif
#if defined( _ARCH_NUM_2 )
  #if        _ARCH_NUM_2 == 370
               HDL_INSTARCH_370,

  #elif      _ARCH_NUM_2 == 390
               HDL_INSTARCH_390,

  #else //   _ARCH_NUM_2 == 900
               HDL_INSTARCH_900,
  #endif
#endif
    };

    /* Allocate a new HDLINS entry for this instruction */
    HDLINS* newins = malloc( sizeof( HDLINS ));

    /* Initialize the entry for this instruction */
    newins->instname  =  strdup( instname );
    newins->hdl_arch  =  arch_to_hdl_arch_tab[ arch ];
    newins->opcode    =  opcode > 0xff ? opcode : (opcode << 8);
    newins->instfunc  =  pgmck_instr_func_tab[ arch ];

    /* Insert it at the head of the chain */
    newins->next = *ppHDLINS;
    *ppHDLINS = newins;

    /* Call HDL's 'replace_opcode' helper that does our grunt work */
    hdl_repins( true, newins );
}

/*-------------------------------------------------------------------*/
/*                     BEG_DIS_FAC_INS_FUNC                          */
/*-------------------------------------------------------------------*/

#define BEG_DIS_FAC_INS_FUNC( name )                                    \
                                                                        \
static void name( int arch, bool enable )                               \
{                                                                       \
    static HDLINS* hdl_ins[ NUM_GEN_ARCHS ] = {0};                      \
                                                                        \
    if (enable)  /* (enable this facility's instructions?) */           \
    {                                                                   \
        ena_fac_ins( arch, hdl_ins );                                   \
    }                                                                   \
    else /* (disable this facility's instructions) */                   \
    {                                                                   \
        if (!hdl_ins[ arch ])   /* (if not already disabled...) */      \
        {                                                               \
            /* Replace each instruction with immediate program check */

/*-------------------------------------------------------------------*/
/*                        DIS_FAC_INS                                */
/*-------------------------------------------------------------------*/

#define DIS_FAC_INS( opcode, name )                                     \
                                                                        \
            dis_fac_ins( arch, &hdl_ins[ arch ], 0x ## opcode, name );

/*-------------------------------------------------------------------*/
/*                    END_DIS_FAC_INS_FUNC                           */
/*-------------------------------------------------------------------*/

#define END_DIS_FAC_INS_FUNC()                                          \
                                                                        \
        }                                                               \
    }                                                                   \
}

/*-------------------------------------------------------------------*/
/*             Disable Facility Instructions functions               */
/*-------------------------------------------------------------------*/
/* The below BEG_DIS_FAC_INS_FUNC, DIS_FAC_INS, END_DIS_FAC_INS_FUNC */
/* functions call into the above 'dis_fac_ins' function to disable   */
/* the instructions associated with a facility whenever the facility */
/* itself is disabled, or re-enable the instructions again whenever  */
/* the facility is enabled. This is controlled by optional function  */
/* pointers defined in the above 'FT2' facilities table which the    */
/* facility_enable_disable function calls when a facility is either  */
/* enabled or disabled.                                              */
/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr0 )
{
    DIS_FAC_INS( B998, "ALCR    B998  ADD LOGICAL WITH CARRY (32)" );
    DIS_FAC_INS( E398, "ALC     E398  ADD LOGICAL WITH CARRY (32)" );
    DIS_FAC_INS( C005, "BRASL   C005  BRANCH RELATIVE AND SAVE LONG" );
    DIS_FAC_INS( C004, "BRCL    C004  BRANCH RELATIVE ON CONDITION LONG" );
    DIS_FAC_INS( B997, "DLR     B997  DIVIDE LOGICAL (32 <- 64)" );
    DIS_FAC_INS( E397, "DL      E397  DIVIDE LOGICAL (32 <- 64)" );
    DIS_FAC_INS( B98D, "EPSW    B98D  EXTRACT PSW" );
    DIS_FAC_INS( C000, "LARL    C000  LOAD ADDRESS RELATIVE LONG" );
    DIS_FAC_INS( B91F, "LRVR    B91F  LOAD REVERSED (32)" );
    DIS_FAC_INS( E31E, "LRV     E31E  LOAD REVERSED (32)" );
    DIS_FAC_INS( E31F, "LRVH    E31F  LOAD REVERSED (16)" );
    DIS_FAC_INS( B996, "MLR     B996  MULTIPLY LOGICAL (64 <- 32)" );
    DIS_FAC_INS( E396, "ML      E396  MULTIPLY LOGICAL (64 <- 32)" );
    DIS_FAC_INS( EB1D, "RLL     EB1D  ROTATE LEFT SINGLE LOGICAL (32)" );
    DIS_FAC_INS( 010C, "SAM24   010C  SET ADDRESSING MODE (24)" );
    DIS_FAC_INS( 010D, "SAM31   010D  SET ADDRESSING MODE (31)" );
    DIS_FAC_INS( B2B1, "STFL    B2B1  STORE FACILITY LIST" );
    DIS_FAC_INS( E33E, "STRV    E33E  STORE REVERSED (32)" );
    DIS_FAC_INS( E33F, "STRVH   E33F  STORE REVERSED (16)" );
    DIS_FAC_INS( B999, "SLBR    B999  SUBTRACT LOGICAL WITH BORROW (32)" );
    DIS_FAC_INS( E399, "SLB     E399  SUBTRACT LOGICAL WITH BORROW (32)" );
    DIS_FAC_INS( 010B, "TAM     010B  TEST ADDRESSING MODE" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr3 )
{
    DIS_FAC_INS( B98A, "CSPG    B98A  COMPARE AND SWAP AND PURGE (64)" );
    DIS_FAC_INS( B98E, "IDTE    B98E  INVALIDATE DAT TABLE ENTRY" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr6 )
{
    DIS_FAC_INS( B99A, "EPAIR   B99A  EXTRACT PRIMARY ASN AND INSTANCE" );
    DIS_FAC_INS( B99B, "ESAIR   B99B  EXTRACT SECONDARY ASN AND INSTANCE" );
    DIS_FAC_INS( B99E, "PTI     B99E  PROGRAM TRANSFER WITH INSTANCE" );
    DIS_FAC_INS( B99F, "SSAIR   B99F  SET SECONDARY ASN WITH INSTANCE" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr7 )
{
    DIS_FAC_INS( B2B0, "STFLE   B2B0  STORE FACILITY LIST EXTENDED" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr8 )
{
    DIS_FAC_INS( B9AF, "PFMF    B9AF  PERFORM FRAME MANAGEMENT FUNCTION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr11 )
{
    DIS_FAC_INS( B9A2, "PTF     B9A2  PERFORM TOPOLOGY FUNCTION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr16 )
{
    DIS_FAC_INS( EB8F, "CLCLU   EB8F  COMPARE LOGICAL LONG UNICODE" );
    DIS_FAC_INS( EB8E, "MVCLU   EB8E  MOVE LONG UNICODE" );
    DIS_FAC_INS( E9,   "PKA     E9    PACK ASCII" );
    DIS_FAC_INS( E1,   "PKU     E1    PACK UNICODE" );
    DIS_FAC_INS( EBC0, "TP      EBC0  TEST DECIMAL" );
    DIS_FAC_INS( B993, "TROO    B993  TRANSLATE ONE TO ONE" );
    DIS_FAC_INS( B992, "TROT    B992  TRANSLATE ONE TO TWO" );
    DIS_FAC_INS( B991, "TRTO    B991  TRANSLATE TWO TO ONE" );
    DIS_FAC_INS( B990, "TRTT    B990  TRANSLATE TWO TO TWO" );
    DIS_FAC_INS( EA,   "UNPKA   EA    UNPACK ASCII" );
    DIS_FAC_INS( E2,   "UNPKU   E2    UNPACK UNICODE" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr17 )
{
    DIS_FAC_INS( B92E, "KM      B92E  CIPHER MESSAGE" );
    DIS_FAC_INS( B92F, "KMC     B92F  CIPHER MESSAGE WITH CHAINING" );
    DIS_FAC_INS( B93E, "KIMD    B93E  COMPUTE INTERMEDIATE MESSAGE DIGEST" );
    DIS_FAC_INS( B93F, "KLMD    B93F  COMPUTE LAST MESSAGE DIGEST" );
    DIS_FAC_INS( B91E, "KMAC    B91E  COMPUTE MESSAGE AUTHENTICATION CODE" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr18 )
{
    DIS_FAC_INS( E35A, "AY      E35A  ADD (32)" );
    DIS_FAC_INS( E37A, "AHY     E37A  ADD HALFWORD (32 <- 16)" );
    DIS_FAC_INS( E35E, "ALY     E35E  ADD LOGICAL (32)" );
    DIS_FAC_INS( E354, "NY      E354  AND (32)" );
    DIS_FAC_INS( EB54, "NIY     EB54  AND (immediate)" );
    DIS_FAC_INS( E359, "CY      E359  COMPARE (32)" );
    DIS_FAC_INS( EB14, "CSY     EB14  COMPARE AND SWAP (32)" );
    DIS_FAC_INS( EB31, "CDSY    EB31  COMPARE DOUBLE AND SWAP (32)" );
    DIS_FAC_INS( E379, "CHY     E379  COMPARE HALFWORD (32 <- 16)" );
    DIS_FAC_INS( E355, "CLY     E355  COMPARE LOGICAL (32)" );
    DIS_FAC_INS( EB55, "CLIY    EB55  COMPARE LOGICAL (immediate)" );
    DIS_FAC_INS( EB21, "CLMY    EB21  COMPARE LOGICAL CHAR. UNDER MASK (low)" );
    DIS_FAC_INS( E306, "CVBY    E306  CONVERT TO BINARY (32)" );
    DIS_FAC_INS( E326, "CVDY    E326  CONVERT TO DECIMAL (32)" );
    DIS_FAC_INS( E357, "XY      E357  EXCLUSIVE OR (32)" );
    DIS_FAC_INS( EB57, "XIY     EB57  EXCLUSIVE OR (immediate)" );
    DIS_FAC_INS( E373, "ICY     E373  INSERT CHARACTER" );
    DIS_FAC_INS( EB81, "ICMY    EB81  INSERT CHARACTERS UNDER MASK (low)" );
    DIS_FAC_INS( E358, "LY      E358  LOAD (32)" );
    DIS_FAC_INS( ED64, "LEY     ED64  LOAD (short)" );
    DIS_FAC_INS( ED65, "LDY     ED65  LOAD (long)" );
    DIS_FAC_INS( EB9A, "LAMY    EB9A  LOAD ACCESS MULTIPLE" );
    DIS_FAC_INS( E371, "LAY     E371  LOAD ADDRESS" );
    DIS_FAC_INS( E376, "LB      E376  LOAD BYTE (32 <- 8)" );
    DIS_FAC_INS( E377, "LGB     E377  LOAD BYTE (64 <- 8)" );
    DIS_FAC_INS( E378, "LHY     E378  LOAD HALFWORD (32 <- 16)" );
    DIS_FAC_INS( EB98, "LMY     EB98  LOAD MULTIPLE (32)" );
    DIS_FAC_INS( E313, "LRAY    E313  LOAD REAL ADDRESS (32)" );
    DIS_FAC_INS( EB52, "MVIY    EB52  MOVE (immediate)" );
    DIS_FAC_INS( E351, "MSY     E351  MULTIPLY SINGLE (32)" );
    DIS_FAC_INS( E356, "OY      E356  OR (32)" );
    DIS_FAC_INS( EB56, "OIY     EB56  OR (immediate)" );
    DIS_FAC_INS( E350, "STY     E350  STORE (32)" );
    DIS_FAC_INS( ED66, "STEY    ED66  STORE (short)" );
    DIS_FAC_INS( ED67, "STDY    ED67  STORE (long)" );
    DIS_FAC_INS( EB9B, "STAMY   EB9B  STORE ACCESS MULTIPLE" );
    DIS_FAC_INS( E372, "STCY    E372  STORE CHARACTER" );
    DIS_FAC_INS( EB2D, "STCMY   EB2D  STORE CHARACTERS UNDER MASK (low)" );
    DIS_FAC_INS( E370, "STHY    E370  STORE HALFWORD (16)" );
    DIS_FAC_INS( EB90, "STMY    EB90  STORE MULTIPLE (32)" );
    DIS_FAC_INS( E35B, "SY      E35B  SUBTRACT (32)" );
    DIS_FAC_INS( E37B, "SHY     E37B  SUBTRACT HALFWORD (32 <- 16)" );
    DIS_FAC_INS( E35F, "SLY     E35F  SUBTRACT LOGICAL (32)" );
    DIS_FAC_INS( EB51, "TMY     EB51  TEST UNDER MASK" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr20 )
{
    DIS_FAC_INS( B32E, "MAER    B32E  MULTIPLY AND ADD (short HFP)" );
    DIS_FAC_INS( B33E, "MADR    B33E  MULTIPLY AND ADD (long HFP)" );
    DIS_FAC_INS( ED2E, "MAE     ED2E  MULTIPLY AND ADD (short HFP)" );
    DIS_FAC_INS( ED3E, "MAD     ED3E  MULTIPLY AND ADD (long HFP)" );
    DIS_FAC_INS( B32F, "MSER    B32F  MULTIPLY AND SUBTRACT (short HFP)" );
    DIS_FAC_INS( B33F, "MSDR    B33F  MULTIPLY AND SUBTRACT (long HFP)" );
    DIS_FAC_INS( ED2F, "MSE     ED2F  MULTIPLY AND SUBTRACT (short HFP)" );
    DIS_FAC_INS( ED3F, "MSD     ED3F  MULTIPLY AND SUBTRACT (long HFP)" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr21 )
{
    DIS_FAC_INS( C208, "AGFI    C208  ADD IMMEDIATE (64 <- 32)" );
    DIS_FAC_INS( C209, "AFI     C209  ADD IMMEDIATE (32)" );
    DIS_FAC_INS( C20A, "ALGFI   C20A  ADD LOGICAL IMMEDIATE (64 <- 32)" );
    DIS_FAC_INS( C20B, "ALFI    C20B  ADD LOGICAL IMMEDIATE (32)" );
    DIS_FAC_INS( C00A, "NIHF    C00A  AND IMMEDIATE (high)" );
    DIS_FAC_INS( C00B, "NILF    C00B  AND IMMEDIATE (low)" );
    DIS_FAC_INS( C20C, "CGFI    C20C  COMPARE IMMEDIATE (64 <- 32)" );
    DIS_FAC_INS( C20D, "CFI     C20D  COMPARE IMMEDIATE (32)" );
    DIS_FAC_INS( C20E, "CLGFI   C20E  COMPARE LOGICAL IMMEDIATE (64 <- 32)" );
    DIS_FAC_INS( C20F, "CLFI    C20F  COMPARE LOGICAL IMMEDIATE (32)" );
    DIS_FAC_INS( C006, "XIHF    C006  EXCLUSIVE OR IMMEDIATE (high)" );
    DIS_FAC_INS( C007, "XILF    C007  EXCLUSIVE OR IMMEDIATE (low)" );
    DIS_FAC_INS( B983, "FLOGR   B983  FIND LEFTMOST ONE" );
    DIS_FAC_INS( C008, "IIHF    C008  INSERT IMMEDIATE (high)" );
    DIS_FAC_INS( C009, "IILF    C009  INSERT IMMEDIATE (low)" );
    DIS_FAC_INS( E302, "LTG     E302  LOAD AND TEST (64)" );
    DIS_FAC_INS( E312, "LT      E312  LOAD AND TEST (32)" );
    DIS_FAC_INS( B906, "LGBR    B906  LOAD BYTE (64 <- 8)" );
    DIS_FAC_INS( B926, "LBR     B926  LOAD BYTE (32 <- 8)" );
    DIS_FAC_INS( B907, "LGHR    B907  LOAD HALFWORD (64 <- 16)" );
    DIS_FAC_INS( B927, "LHR     B927  LOAD HALFWORD (32 <- 16)" );
    DIS_FAC_INS( C001, "LGFI    C001  LOAD IMMEDIATE (64 <- 32)" );
    DIS_FAC_INS( B984, "LLGCR   B984  LOAD LOGICAL CHARACTER (64 <- 8)" );
    DIS_FAC_INS( B994, "LLCR    B994  LOAD LOGICAL CHARACTER (32 <- 8)" );
    DIS_FAC_INS( E394, "LLC     E394  LOAD LOGICAL CHARACTER (32 <- 8)" );
    DIS_FAC_INS( B985, "LLGHR   B985  LOAD LOGICAL HALFWORD (64 <- 16)" );
    DIS_FAC_INS( B995, "LLHR    B995  LOAD LOGICAL HALFWORD (32 <- 16)" );
    DIS_FAC_INS( E395, "LLH     E395  LOAD LOGICAL HALFWORD (32 <- 16)" );
    DIS_FAC_INS( C00E, "LLIHF   C00E  LOAD LOGICAL IMMEDIATE (high)" );
    DIS_FAC_INS( C00F, "LLILF   C00F  LOAD LOGICAL IMMEDIATE (low)" );
    DIS_FAC_INS( C00C, "OIHF    C00C  OR IMMEDIATE (high)" );
    DIS_FAC_INS( C00D, "OILF    C00D  OR IMMEDIATE (low)" );
    DIS_FAC_INS( C204, "SLGFI   C204  SUBTRACT LOGICAL IMMEDIATE (64 <- 32)" );
    DIS_FAC_INS( C205, "SLFI    C205  SUBTRACT LOGICAL IMMEDIATE (32)" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr22 )
{
    DIS_FAC_INS( B9B1, "CU24    B9B1  CONVERT UTF-16 TO UTF-32" );
    DIS_FAC_INS( B9B3, "CU42    B9B3  CONVERT UTF-32 TO UTF-16" );
    DIS_FAC_INS( B9B2, "CU41    B9B2  CONVERT UTF-32 TO UTF-8" );
    DIS_FAC_INS( B9B0, "CU14    B9B0  CONVERT UTF-8 TO UTF-32" );
    DIS_FAC_INS( B9BE, "SRSTU   B9BE  SEARCH STRING UNICODE" );
    DIS_FAC_INS( D0,   "TRTR    D0    TRANSLATE AND TEST REVERSE" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr23 )
{
    DIS_FAC_INS( B33A, "MAYR    B33A  MULTIPLY & ADD UNNORMALIZED (long to ext. HFP)" );
    DIS_FAC_INS( ED3A, "MAY     ED3A  MULTIPLY & ADD UNNORMALIZED (long to ext. HFP)" );
    DIS_FAC_INS( B338, "MAYLR   B338  MULTIPLY AND ADD UNNRM. (long to ext. low HFP)" );
    DIS_FAC_INS( B33C, "MAYHR   B33C  MULTIPLY AND ADD UNNRM. (long to ext. high HFP)" );
    DIS_FAC_INS( ED38, "MAYL    ED38  MULTIPLY AND ADD UNNRM. (long to ext. low HFP)" );
    DIS_FAC_INS( ED3C, "MAYH    ED3C  MULTIPLY AND ADD UNNRM. (long to ext. high HFP)" );
    DIS_FAC_INS( B339, "MYLR    B339  MULTIPLY UNNORM. (long to ext. low HFP)" );
    DIS_FAC_INS( B33D, "MYHR    B33D  MULTIPLY UNNORM. (long to ext. high HFP)" );
    DIS_FAC_INS( ED39, "MYL     ED39  MULTIPLY UNNORM. (long to ext. low HFP)" );
    DIS_FAC_INS( ED3D, "MYH     ED3D  MULTIPLY UNNORM. (long to ext. high HFP)" );
    DIS_FAC_INS( B33B, "MYR     B33B  MULTIPLY UNNORMALIZED (long to ext. HFP)" );
    DIS_FAC_INS( ED3B, "MY      ED3B  MULTIPLY UNNORMALIZED (long to ext. HFP)" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr25 )
{
    DIS_FAC_INS( B27C, "STCKF   B27C  STORE CLOCK FAST" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr26 )
{
    DIS_FAC_INS( B9BF, "TRTE    B9BF  TRANSLATE AND TEST EXTENDED" );
    DIS_FAC_INS( B9BD, "TRTRE   B9BD  TRANSLATE AND TEST REVERSE EXTENDED" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr27 )
{
    DIS_FAC_INS( C800, "MVCOS   C800  MOVE WITH OPTIONAL SPECIFICATIONS" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr28 )
{
    DIS_FAC_INS( 0104, "PTFF    0104  PERFORM TIMING FACILITY FUNCTION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr31 )
{
    DIS_FAC_INS( C801, "ECTG    C801  EXTRACT CPU TIME" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr32 )
{
    DIS_FAC_INS( C802, "CSST    C802  COMPARE AND SWAP AND STORE" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr34 )
{
    DIS_FAC_INS( EB6A, "ASI     EB6A  ADD IMMEDIATE (32 <- 8)" );
    DIS_FAC_INS( EB7A, "AGSI    EB7A  ADD IMMEDIATE (64 <- 8)" );
    DIS_FAC_INS( EB6E, "ALSI    EB6E  ADD LOGICAL WITH SIGNED IMMEDIATE (32 <- 8)" );
    DIS_FAC_INS( EB7E, "ALGSI   EB7E  ADD LOGICAL WITH SIGNED IMMEDIATE (64 <- 8)" );
    DIS_FAC_INS( ECE4, "CGRB    ECE4  COMPARE AND BRANCH (64)" );
    DIS_FAC_INS( ECF6, "CRB     ECF6  COMPARE AND BRANCH (32)" );
    DIS_FAC_INS( EC64, "CGRJ    EC64  COMPARE AND BRANCH RELATIVE (64)" );
    DIS_FAC_INS( EC76, "CRJ     EC76  COMPARE AND BRANCH RELATIVE (32)" );
    DIS_FAC_INS( B960, "CGRT    B960  COMPARE AND TRAP (64)" );
    DIS_FAC_INS( B972, "CRT     B972  COMPARE AND TRAP (32)" );
    DIS_FAC_INS( E334, "CGH     E334  COMPARE HALFWORD (64 <- 16)" );
    DIS_FAC_INS( E554, "CHHSI   E554  COMPARE HALFWORD IMMEDIATE (16 <- 16)" );
    DIS_FAC_INS( E558, "CGHSI   E558  COMPARE HALFWORD IMMEDIATE (64 <- 16)" );
    DIS_FAC_INS( E55C, "CHSI    E55C  COMPARE HALFWORD IMMEDIATE (32 <- 16)" );
    DIS_FAC_INS( C604, "CGHRL   C604  COMPARE HALFWORD RELATIVE LONG (64 <- 16)" );
    DIS_FAC_INS( C605, "CHRL    C605  COMPARE HALFWORD RELATIVE LONG (32 <- 16)" );
    DIS_FAC_INS( ECFC, "CGIB    ECFC  COMPARE IMMEDIATE AND BRANCH (64 <- 8)" );
    DIS_FAC_INS( ECFE, "CIB     ECFE  COMPARE IMMEDIATE AND BRANCH (32 <- 8)" );
    DIS_FAC_INS( EC7C, "CGIJ    EC7C  COMPARE IMMEDIATE AND BRANCH RELATIVE (64 <- 8)" );
    DIS_FAC_INS( EC7E, "CIJ     EC7E  COMPARE IMMEDIATE AND BRANCH RELATIVE (32 <- 8)" );
    DIS_FAC_INS( EC70, "CGIT    EC70  COMPARE IMMEDIATE AND TRAP (64 <- 16)" );
    DIS_FAC_INS( EC72, "CIT     EC72  COMPARE IMMEDIATE AND TRAP (32 <- 16)" );
    DIS_FAC_INS( ECE5, "CLGRB   ECE5  COMPARE LOGICAL AND BRANCH (64)" );
    DIS_FAC_INS( ECF7, "CLRB    ECF7  COMPARE LOGICAL AND BRANCH (32)" );
    DIS_FAC_INS( EC65, "CLGRJ   EC65  COMPARE LOGICAL AND BRANCH RELATIVE (64)" );
    DIS_FAC_INS( EC77, "CLRJ    EC77  COMPARE LOGICAL AND BRANCH RELATIVE (32)" );
    DIS_FAC_INS( B961, "CLGRT   B961  COMPARE LOGICAL AND TRAP (64)" );
    DIS_FAC_INS( B973, "CLRT    B973  COMPARE LOGICAL AND TRAP (32)" );
    DIS_FAC_INS( E555, "CLHHSI  E555  COMPARE LOGICAL IMMEDIATE (16 <- 16)" );
    DIS_FAC_INS( E559, "CLGHSI  E559  COMPARE LOGICAL IMMEDIATE (64 <- 16)" );
    DIS_FAC_INS( E55D, "CLFHSI  E55D  COMPARE LOGICAL IMMEDIATE (32 <- 16)" );
    DIS_FAC_INS( ECFD, "CLGIB   ECFD  COMPARE LOGICAL IMMEDIATE AND BRANCH (64 <- 8)" );
    DIS_FAC_INS( ECFF, "CLIB    ECFF  COMPARE LOGICAL IMMEDIATE AND BRANCH (32 <- 8)" );
    DIS_FAC_INS( EC7D, "CLGIJ   EC7D  COMPARE LOGICAL IMMEDIATE AND BRANCH RELATIVE (64 <- 8)" );
    DIS_FAC_INS( EC7F, "CLIJ    EC7F  COMPARE LOGICAL IMMEDIATE AND BRANCH RELATIVE (32 <- 8)" );
    DIS_FAC_INS( EC71, "CLGIT   EC71  COMPARE LOGICAL IMMEDIATE AND TRAP (64 <- 16)" );
    DIS_FAC_INS( EC73, "CLFIT   EC73  COMPARE LOGICAL IMMEDIATE AND TRAP (32 <- 16)" );
    DIS_FAC_INS( C606, "CLGHRL  C606  COMPARE LOGICAL RELATIVE LONG (64 <- 16)" );
    DIS_FAC_INS( C607, "CLHRL   C607  COMPARE LOGICAL RELATIVE LONG (32 <- 16)" );
    DIS_FAC_INS( C60A, "CLGRL   C60A  COMPARE LOGICAL RELATIVE LONG (64)" );
    DIS_FAC_INS( C60E, "CLGFRL  C60E  COMPARE LOGICAL RELATIVE LONG (64 <- 32)" );
    DIS_FAC_INS( C60F, "CLRL    C60F  COMPARE LOGICAL RELATIVE LONG (32)" );
    DIS_FAC_INS( C608, "CGRL    C608  COMPARE RELATIVE LONG (64)" );
    DIS_FAC_INS( C60C, "CGFRL   C60C  COMPARE RELATIVE LONG (64 <- 32)" );
    DIS_FAC_INS( C60D, "CRL     C60D  COMPARE RELATIVE LONG (32)" );
    DIS_FAC_INS( EB4C, "ECAG    EB4C  EXTRACT CPU ATTRIBUTE" );
    DIS_FAC_INS( E375, "LAEY    E375  LOAD ADDRESS EXTENDED" );
    DIS_FAC_INS( E332, "LTGF    E332  LOAD AND TEST (64 <- 32)" );
    DIS_FAC_INS( C404, "LGHRL   C404  LOAD HALFWORD RELATIVE LONG (64 <- 16)" );
    DIS_FAC_INS( C405, "LHRL    C405  LOAD HALFWORD RELATIVE LONG (32 <- 16)" );
    DIS_FAC_INS( C402, "LLHRL   C402  LOAD LOGICAL HALFWORD RELATIVE LONG (32 <- 16)" );
    DIS_FAC_INS( C406, "LLGHRL  C406  LOAD LOGICAL HALFWORD RELATIVE LONG (64 <- 16)" );
    DIS_FAC_INS( C40E, "LLGFRL  C40E  LOAD LOGICAL RELATIVE LONG (64 <- 32)" );
    DIS_FAC_INS( C408, "LGRL    C408  LOAD RELATIVE LONG (64)" );
    DIS_FAC_INS( C40C, "LGFRL   C40C  LOAD RELATIVE LONG (64 <- 32)" );
    DIS_FAC_INS( C40D, "LRL     C40D  LOAD RELATIVE LONG (32)" );
    DIS_FAC_INS( E544, "MVHHI   E544  MOVE (16 <- 16)" );
    DIS_FAC_INS( E548, "MVGHI   E548  MOVE (64 <- 16)" );
    DIS_FAC_INS( E54C, "MVHI    E54C  MOVE (32 <- 16)" );
    DIS_FAC_INS( E35C, "MFY     E35C  MULTIPLY (64 <- 32)" );
    DIS_FAC_INS( E37C, "MHY     E37C  MULTIPLY HALFWORD (32 <- 16)" );
    DIS_FAC_INS( C200, "MSGFI   C200  MULTIPLY SINGLE IMMEDIATE (64 <- 32)" );
    DIS_FAC_INS( C201, "MSFI    C201  MULTIPLY SINGLE IMMEDIATE (32)" );
    DIS_FAC_INS( E336, "PFD     E336  PREFETCH DATA" );
    DIS_FAC_INS( C602, "PFDRL   C602  PREFETCH DATA RELATIVE LONG" );
    DIS_FAC_INS( EC54, "RNSBG   EC54  ROTATE THEN AND SELECTED BITS (64)" );
    DIS_FAC_INS( EC57, "RXSBG   EC57  ROTATE THEN EXCLUSIVE OR SELECT. BITS (64)" );
    DIS_FAC_INS( EC55, "RISBG   EC55  ROTATE THEN INSERT SELECTED BITS (64)" );
    DIS_FAC_INS( EC56, "ROSBG   EC56  ROTATE THEN OR SELECTED BITS (64)" );
    DIS_FAC_INS( C407, "STHRL   C407  STORE HALFWORD RELATIVE LONG (16)" );
    DIS_FAC_INS( C40B, "STGRL   C40B  STORE RELATIVE LONG (64)" );
    DIS_FAC_INS( C40F, "STRL    C40F  STORE RELATIVE LONG (32)" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr35 )
{
    DIS_FAC_INS( C600, "EXRL    C600  EXECUTE RELATIVE LONG" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr37 )
{
    DIS_FAC_INS( B951, "CDFTR   B951  CONVERT FROM FIXED (32 to long DFP)" );
    DIS_FAC_INS( B959, "CXFTR   B959  CONVERT FROM FIXED (32 to extended DFP)" );
    DIS_FAC_INS( B390, "CELFBR  B390  CONVERT FROM LOGICAL (32 to short BFP)" );
    DIS_FAC_INS( B391, "CDLFBR  B391  CONVERT FROM LOGICAL (32 to long BFP)" );
    DIS_FAC_INS( B392, "CXLFBR  B392  CONVERT FROM LOGICAL (32 to extended BFP)" );
    DIS_FAC_INS( B3A0, "CELGBR  B3A0  CONVERT FROM LOGICAL (64 to short BFP)" );
    DIS_FAC_INS( B3A1, "CDLGBR  B3A1  CONVERT FROM LOGICAL (64 to long BFP)" );
    DIS_FAC_INS( B3A2, "CXLGBR  B3A2  CONVERT FROM LOGICAL (64 to extended BFP)" );
    DIS_FAC_INS( B952, "CDLGTR  B952  CONVERT FROM LOGICAL (64 to long DFP)" );
    DIS_FAC_INS( B953, "CDLFTR  B953  CONVERT FROM LOGICAL (32 to long DFP)" );
    DIS_FAC_INS( B95A, "CXLGTR  B95A  CONVERT FROM LOGICAL (64 to extended DFP)" );
    DIS_FAC_INS( B95B, "CXLFTR  B95B  CONVERT FROM LOGICAL (32 to extended DFP)" );
    DIS_FAC_INS( B941, "CFDTR   B941  CONVERT TO FIXED (long DFP to 32)" );
    DIS_FAC_INS( B949, "CFXTR   B949  CONVERT TO FIXED (extended DFP to 32)" );
    DIS_FAC_INS( B39C, "CLFEBR  B39C  CONVERT TO LOGICAL (short BFP to 32)" );
    DIS_FAC_INS( B39D, "CLFDBR  B39D  CONVERT TO LOGICAL (long BFP to 32)" );
    DIS_FAC_INS( B39E, "CLFXBR  B39E  CONVERT TO LOGICAL (extended BFP to 32)" );
    DIS_FAC_INS( B3AC, "CLGEBR  B3AC  CONVERT TO LOGICAL (short BFP to 64)" );
    DIS_FAC_INS( B3AD, "CLGDBR  B3AD  CONVERT TO LOGICAL (long BFP to 64)" );
    DIS_FAC_INS( B3AE, "CLGXBR  B3AE  CONVERT TO LOGICAL (extended BFP to 64)" );
    DIS_FAC_INS( B942, "CLGDTR  B942  CONVERT TO LOGICAL (long DFP to 64)" );
    DIS_FAC_INS( B943, "CLFDTR  B943  CONVERT TO LOGICAL (long DFP to 32)" );
    DIS_FAC_INS( B94A, "CLGXTR  B94A  CONVERT TO LOGICAL (extended DFP to 64)" );
    DIS_FAC_INS( B94B, "CLFXTR  B94B  CONVERT TO LOGICAL (extended DFP to 32)" );
    DIS_FAC_INS( B2B8, "SRNMB   B2B8  SET BFP ROUNDING MODE (3 bit)" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr40 )
{
    DIS_FAC_INS( B280, "LPP     B280  LOAD PROGRAM PARAMETER" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr41 )
{
    DIS_FAC_INS( B2B9, "SRNMT   B2B9  SET DFP ROUNDING MODE" );

    DIS_FAC_INS( B3C1, "LDGR    B3C1  LOAD FPR FROM GR (64 to long)" );
    DIS_FAC_INS( B3CD, "LGDR    B3CD  LOAD GR FROM FPR (long to 64)" );

    DIS_FAC_INS( B372, "CPSDR   B372  COPY SIGN (long)" );
    DIS_FAC_INS( B373, "LCDFR   B373  LOAD COMPLEMENT (long)" );
    DIS_FAC_INS( B371, "LNDFR   B371  LOAD NEGATIVE (long)" );
    DIS_FAC_INS( B370, "LPDFR   B370  LOAD POSITIVE (long)" );

    DIS_FAC_INS( B2BD, "LFAS    B2BD  LOAD FPC AND SIGNAL" );
    DIS_FAC_INS( B385, "SFASR   B385  SET FPC AND SIGNAL" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr42 )
{
    DIS_FAC_INS( B3D2, "ADTRA   B3D2  ADD (long DFP)" );
    DIS_FAC_INS( B3DA, "AXTRA   B3DA  ADD (extended DFP)" );
    DIS_FAC_INS( B3E4, "CDTR    B3E4  COMPARE (long DFP)" );
    DIS_FAC_INS( B3EC, "CXTR    B3EC  COMPARE (extended DFP)" );
    DIS_FAC_INS( B3E0, "KDTR    B3E0  COMPARE AND SIGNAL (long DFP)" );
    DIS_FAC_INS( B3E8, "KXTR    B3E8  COMPARE AND SIGNAL (extended DFP)" );
    DIS_FAC_INS( B3F4, "CEDTR   B3F4  COMPARE BIASED EXPONENT (long DFP)" );
    DIS_FAC_INS( B3FC, "CEXTR   B3FC  COMPARE BIASED EXPONENT (extended DFP)" );
    DIS_FAC_INS( B3F1, "CDGTRA  B3F1  CONVERT FROM FIXED (64 to long DFP)" );
    DIS_FAC_INS( B3F9, "CXGTRA  B3F9  CONVERT FROM FIXED (64 to extended DFP)" );
    DIS_FAC_INS( B3F3, "CDSTR   B3F3  CONVERT FROM SIGNED PACKED (64 to long DFP)" );
    DIS_FAC_INS( B3FB, "CXSTR   B3FB  CONVERT FROM SIGNED PACKED (128 to extended DFP)" );
    DIS_FAC_INS( B3F2, "CDUTR   B3F2  CONVERT FROM UNSIGNED PACKED (64 to long DFP)" );
    DIS_FAC_INS( B3FA, "CXUTR   B3FA  CONVERT FROM UNSIGNED PACKED (128 to ext. DFP)" );
    DIS_FAC_INS( B3E1, "CGDTRA  B3E1  CONVERT TO FIXED (long DFP to 64)" );
    DIS_FAC_INS( B3E9, "CGXTRA  B3E9  CONVERT TO FIXED (extended DFP to 64)" );
    DIS_FAC_INS( B3E3, "CSDTR   B3E3  CONVERT TO SIGNED PACKED (long DFP to 64)" );
    DIS_FAC_INS( B3EB, "CSXTR   B3EB  CONVERT TO SIGNED PACKED (extended DFP to128)" );
    DIS_FAC_INS( B3E2, "CUDTR   B3E2  CONVERT TO UNSIGNED PACKED (long DFP to64)" );
    DIS_FAC_INS( B3EA, "CUXTR   B3EA  CONVERT TO UNSIGNED PACKED (extended DFP to 128)" );
    DIS_FAC_INS( B3D1, "DDTRA   B3D1  DIVIDE (long DFP)" );
    DIS_FAC_INS( B3D9, "DXTRA   B3D9  DIVIDE (extended DFP)" );
    DIS_FAC_INS( B3E5, "EEDTR   B3E5  EXTRACT BIASED EXPONENT (long DFP to 64)" );
    DIS_FAC_INS( B3ED, "EEXTR   B3ED  EXTRACT BIASED EXPONENT (extended DFP to64)" );
    DIS_FAC_INS( B3E7, "ESDTR   B3E7  EXTRACT SIGNIFICANCE (long DFP to 64)" );
    DIS_FAC_INS( B3EF, "ESXTR   B3EF  EXTRACT SIGNIFICANCE (extended DFP to 64)" );
    DIS_FAC_INS( B3F6, "IEDTR   B3F6  INSERT BIASED EXPONENT (64 to long DFP)" );
    DIS_FAC_INS( B3FE, "IEXTR   B3FE  INSERT BIASED EXPONENT (64 to extended DFP)" );
    DIS_FAC_INS( B3D6, "LTDTR   B3D6  LOAD AND TEST (long DFP)" );
    DIS_FAC_INS( B3DE, "LTXTR   B3DE  LOAD AND TEST (extended DFP)" );
    DIS_FAC_INS( B3D7, "FIDTR   B3D7  LOAD FP INTEGER (long DFP)" );
    DIS_FAC_INS( B3DF, "FIXTR   B3DF  LOAD FP INTEGER (extended DFP)" );
    DIS_FAC_INS( B3D4, "LDETR   B3D4  LOAD LENGTHENED (short to long DFP)" );
    DIS_FAC_INS( B3DC, "LXDTR   B3DC  LOAD LENGTHENED (long to extended DFP)" );
    DIS_FAC_INS( B3D5, "LEDTR   B3D5  LOAD ROUNDED (long to short DFP)" );
    DIS_FAC_INS( B3DD, "LDXTR   B3DD  LOAD ROUNDED (extended to long DFP)" );
    DIS_FAC_INS( B3D0, "MDTRA   B3D0  MULTIPLY (long DFP)" );
    DIS_FAC_INS( B3D8, "MXTRA   B3D8  MULTIPLY (extended DFP)" );
    DIS_FAC_INS( B3F5, "QADTR   B3F5  QUANTIZE (long DFP)" );
    DIS_FAC_INS( B3FD, "QAXTR   B3FD  QUANTIZE (extended DFP)" );
    DIS_FAC_INS( B3F7, "RRDTR   B3F7  REROUND (long DFP)" );
    DIS_FAC_INS( B3FF, "RRXTR   B3FF  REROUND (extended DFP)" );
    DIS_FAC_INS( ED40, "SLDT    ED40  SHIFT SIGNIFICAND LEFT (long DFP)" );
    DIS_FAC_INS( ED48, "SLXT    ED48  SHIFT SIGNIFICAND LEFT (extended DFP)" );
    DIS_FAC_INS( ED41, "SRDT    ED41  SHIFT SIGNIFICAND RIGHT (long DFP)" );
    DIS_FAC_INS( ED49, "SRXT    ED49  SHIFT SIGNIFICAND RIGHT (extended DFP)" );
    DIS_FAC_INS( B3D3, "SDTRA   B3D3  SUBTRACT (long DFP)" );
    DIS_FAC_INS( B3DB, "SXTRA   B3DB  SUBTRACT (extended DFP)" );
    DIS_FAC_INS( ED50, "TDCET   ED50  TEST DATA CLASS (short DFP)" );
    DIS_FAC_INS( ED54, "TDCDT   ED54  TEST DATA CLASS (long DFP)" );
    DIS_FAC_INS( ED58, "TDCXT   ED58  TEST DATA CLASS (extended DFP)" );
    DIS_FAC_INS( ED51, "TDGET   ED51  TEST DATA GROUP (short DFP)" );
    DIS_FAC_INS( ED55, "TDGDT   ED55  TEST DATA GROUP (long DFP)" );
    DIS_FAC_INS( ED59, "TDGXT   ED59  TEST DATA GROUP (extended DFP)" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr44 )
{
    DIS_FAC_INS( 010A, "PFPO    010A  PERFORM FLOATING-POINT OPERATION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr45 )
{
    DIS_FAC_INS( B9E8, "AGRK    B9E8  ADD (64)" );
    DIS_FAC_INS( B9F8, "ARK     B9F8  ADD (32)" );
    DIS_FAC_INS( ECD8, "AHIK    ECD8  ADD IMMEDIATE (32 <- 16)" );
    DIS_FAC_INS( ECD9, "AGHIK   ECD9  ADD IMMEDIATE (64 <- 16)" );
    DIS_FAC_INS( B9EA, "ALGRK   B9EA  ADD LOGICAL (64)" );
    DIS_FAC_INS( B9FA, "ALRK    B9FA  ADD LOGICAL (32)" );
    DIS_FAC_INS( ECDA, "ALHSIK  ECDA  ADD LOGICAL WITH SIGNED IMMEDIATE (32 <- 16)" );
    DIS_FAC_INS( ECDB, "ALGHSIK ECDB  ADD LOGICAL WITH SIGNED IMMEDIATE (64 <- 16)" );
    DIS_FAC_INS( B9E4, "NGRK    B9E4  AND (64)" );
    DIS_FAC_INS( B9F4, "NRK     B9F4  AND (32)" );
    DIS_FAC_INS( B9E7, "XGRK    B9E7  EXCLUSIVE OR (64)" );
    DIS_FAC_INS( B9F7, "XRK     B9F7  EXCLUSIVE OR (32)" );
    DIS_FAC_INS( B9E6, "OGRK    B9E6  OR (64)" );
    DIS_FAC_INS( B9F6, "ORK     B9F6  OR (32)" );
    DIS_FAC_INS( EBDD, "SLAK    EBDD  SHIFT LEFT SINGLE (32)" );
    DIS_FAC_INS( EBDF, "SLLK    EBDF  SHIFT LEFT SINGLE LOGICAL (32)" );
    DIS_FAC_INS( EBDC, "SRAK    EBDC  SHIFT RIGHT SINGLE (32)" );
    DIS_FAC_INS( EBDE, "SRLK    EBDE  SHIFT RIGHT SINGLE LOGICAL (32)" );
    DIS_FAC_INS( B9E9, "SGRK    B9E9  SUBTRACT (64)" );
    DIS_FAC_INS( B9F9, "SRK     B9F9  SUBTRACT (32)" );
    DIS_FAC_INS( B9EB, "SLGRK   B9EB  SUBTRACT LOGICAL (64)" );
    DIS_FAC_INS( B9FB, "SLRK    B9FB  SUBTRACT LOGICAL (32)" );

    DIS_FAC_INS( B9C8, "AHHHR   B9C8  ADD HIGH (32)" );
    DIS_FAC_INS( B9D8, "AHHLR   B9D8  ADD HIGH (32)" );
    DIS_FAC_INS( CC08, "AIH     CC08  ADD IMMEDIATE HIGH (32)" );
    DIS_FAC_INS( B9CA, "ALHHHR  B9CA  ADD LOGICAL HIGH (32)" );
    DIS_FAC_INS( B9DA, "ALHHLR  B9DA  ADD LOGICAL HIGH (32)" );
    DIS_FAC_INS( CC0A, "ALSIH   CC0A  ADD LOGICAL WITH SIGNED IMMEDIATE HIGH (32)" );
    DIS_FAC_INS( CC0B, "ALSIHN  CC0B  ADD LOGICAL WITH SIGNED IMMEDIATE HIGH (32)" );
    DIS_FAC_INS( CC06, "BRCTH   CC06  BRANCH RELATIVE ON COUNT HIGH (32)" );
    DIS_FAC_INS( B9CD, "CHHR    B9CD  COMPARE HIGH (32)" );
    DIS_FAC_INS( B9DD, "CHLR    B9DD  COMPARE HIGH (32)" );
    DIS_FAC_INS( E3CD, "CHF     E3CD  COMPARE HIGH (32)" );
    DIS_FAC_INS( CC0D, "CIH     CC0D  COMPARE IMMEDIATE HIGH (32)" );
    DIS_FAC_INS( B9CF, "CLHHR   B9CF  COMPARE LOGICAL HIGH (32)" );
    DIS_FAC_INS( B9DF, "CLHLR   B9DF  COMPARE LOGICAL HIGH (32)" );
    DIS_FAC_INS( E3CF, "CLHF    E3CF  COMPARE LOGICAL HIGH (32)" );
    DIS_FAC_INS( CC0F, "CLIH    CC0F  COMPARE LOGICAL IMMEDIATE HIGH (32)" );
    DIS_FAC_INS( E3C0, "LBH     E3C0  LOAD BYTE HIGH (32 <- 8)" );
    DIS_FAC_INS( E3C4, "LHH     E3C4  LOAD HALFWORD HIGH (32 <- 16)" );
    DIS_FAC_INS( E3CA, "LFH     E3CA  LOAD HIGH (32)" );
    DIS_FAC_INS( E3C2, "LLCH    E3C2  LOAD LOGICAL CHARACTER HIGH (32 <- 8)" );
    DIS_FAC_INS( E3C6, "LLHH    E3C6  LOAD LOGICAL HALFWORD HIGH (32 <- 16)" );
    DIS_FAC_INS( EC5D, "RISBHG  EC5D  ROTATE THEN INSERT SELECTED BITS HIGH (64)" );
    DIS_FAC_INS( EC51, "RISBLG  EC51  ROTATE THEN INSERT SELECTED BITS LOW (64)" );
    DIS_FAC_INS( E3C3, "STCH    E3C3  STORE CHARACTER HIGH (8)" );
    DIS_FAC_INS( E3C7, "STHH    E3C7  STORE HALFWORD HIGH (16)" );
    DIS_FAC_INS( E3CB, "STFH    E3CB  STORE HIGH (32)" );
    DIS_FAC_INS( B9C9, "SHHHR   B9C9  SUBTRACT HIGH (32)" );
    DIS_FAC_INS( B9D9, "SHHLR   B9D9  SUBTRACT HIGH (32)" );
    DIS_FAC_INS( B9CB, "SLHHHR  B9CB  SUBTRACT LOGICAL HIGH (32)" );
    DIS_FAC_INS( B9DB, "SLHHLR  B9DB  SUBTRACT LOGICAL HIGH (32)" );

    DIS_FAC_INS( EBE8, "LAAG    EBE8  LOAD AND ADD (64)" );
    DIS_FAC_INS( EBF8, "LAA     EBF8  LOAD AND ADD (32)" );
    DIS_FAC_INS( EBEA, "LAALG   EBEA  LOAD AND ADD LOGICAL (64)" );
    DIS_FAC_INS( EBFA, "LAAL    EBFA  LOAD AND ADD LOGICAL (32)" );
    DIS_FAC_INS( EBE4, "LANG    EBE4  LOAD AND AND (64)" );
    DIS_FAC_INS( EBF4, "LAN     EBF4  LOAD AND AND (32)" );
    DIS_FAC_INS( EBE7, "LAXG    EBE7  LOAD AND EXCLUSIVE OR (64)" );
    DIS_FAC_INS( EBF7, "LAX     EBF7  LOAD AND EXCLUSIVE OR (32)" );
    DIS_FAC_INS( EBE6, "LAOG    EBE6  LOAD AND OR (64)" );
    DIS_FAC_INS( EBF6, "LAO     EBF6  LOAD AND OR (32)" );
    DIS_FAC_INS( C804, "LPD     C804  LOAD PAIR DISJOINT (32)" );
    DIS_FAC_INS( C805, "LPDG    C805  LOAD PAIR DISJOINT (64)" );

    DIS_FAC_INS( B9E2, "LOCGR   B9E2  LOAD ON CONDITION (64)" );
    DIS_FAC_INS( B9F2, "LOCR    B9F2  LOAD ON CONDITION (32)" );
    DIS_FAC_INS( EBE2, "LOCG    EBE2  LOAD ON CONDITION (64)" );
    DIS_FAC_INS( EBF2, "LOC     EBF2  LOAD ON CONDITION (32)" );
    DIS_FAC_INS( EBE3, "STOCG   EBE3  STORE ON CONDITION (64)" );
    DIS_FAC_INS( EBF3, "STOC    EBF3  STORE ON CONDITION (32)" );

    DIS_FAC_INS( B9E1, "POPCNT  B9E1  POPULATION COUNT" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr48 )
{
    DIS_FAC_INS( EDAA, "CDZT    EDAA  CONVERT FROM ZONED (to long DFP)" );
    DIS_FAC_INS( EDAB, "CXZT    EDAB  CONVERT FROM ZONED (to extended DFP)" );
    DIS_FAC_INS( EDA8, "CZDT    EDA8  CONVERT TO ZONED (from long DFP)" );
    DIS_FAC_INS( EDA9, "CZXT    EDA9  CONVERT TO ZONED (from extended DFP)" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr49 )
{
    DIS_FAC_INS( C7,   "BPP     C7    BRANCH PREDICTION PRELOAD" );
    DIS_FAC_INS( C5,   "BPRP    C5    BRANCH PREDICTION RELATIVE PRELOAD" );
    DIS_FAC_INS( B2FA, "NIAI    B2FA  NEXT INSTRUCTION ACCESS INTENT" );

    DIS_FAC_INS( E385, "LGAT    E385  LOAD AND TRAP (64)" );
    DIS_FAC_INS( E39F, "LAT     E39F  LOAD AND TRAP (32L <- 32)" );
    DIS_FAC_INS( E3C8, "LFHAT   E3C8  LOAD HIGH AND TRAP (32H <- 32)" );
    DIS_FAC_INS( E39D, "LLGFAT  E39D  LOAD LOGICAL AND TRAP (64 <- 32)" );
    DIS_FAC_INS( E39C, "LLGTAT  E39C  LOAD LOGICAL THIRTY ONE BITS AND TRAP (64 <- 31)" );

    DIS_FAC_INS( EB23, "CLT     EB23  COMPARE LOGICAL AND TRAP (32)" );
    DIS_FAC_INS( EB2B, "CLGT    EB2B  COMPARE LOGICAL AND TRAP (64)" );
    DIS_FAC_INS( EC59, "RISBGN  EC59  ROTATE THEN INSERT SELECTED BITS (64)" );

    DIS_FAC_INS( B2E8, "PPA     B2E8  PERFORM PROCESSOR ASSIST" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr50 )
{
    DIS_FAC_INS( E561, "TBEGINC E561  TRANSACTION BEGIN (CONSTRAINED)" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr53 )
{
    DIS_FAC_INS( E32A, "LZRG    E32A  LOAD AND ZERO RIGHTMOST BYTE (64)" );
    DIS_FAC_INS( E33B, "LZRF    E33B  LOAD AND ZERO RIGHTMOST BYTE (32)" );
    DIS_FAC_INS( E33A, "LLZRGF  E33A  LOAD LOGICAL AND ZERO RIGHTMOST BYTE (64 <- 32)" );

    DIS_FAC_INS( EC4E, "LOCHHI  EC4E  LOAD HALFWORD HIGH IMMEDIATE ON CONDITION (32 <- 16)" );
    DIS_FAC_INS( EC42, "LOCHI   EC42  LOAD HALFWORD IMMEDIATE ON CONDITION (32 <- 16)" );
    DIS_FAC_INS( EC46, "LOCGHI  EC46  LOAD HALFWORD IMMEDIATE ON CONDITION (64 <- 16)" );
    DIS_FAC_INS( B9E0, "LOCFHR  B9E0  LOAD HIGH ON CONDITION (32)" );
    DIS_FAC_INS( EBE0, "LOCFH   EBE0  LOAD HIGH ON CONDITION (32)" );
    DIS_FAC_INS( EBE1, "STOCFH  EBE1  STORE HIGH ON CONDITION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr57 )
{
    DIS_FAC_INS( B93C, "PRNO    B93C  PERFORM RANDOM NUMBER OPERATION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr58 )
{
    DIS_FAC_INS( E338, "AGH     E338  ADD HALFWORD (64 <- 16)" );
    DIS_FAC_INS( E347, "BIC     E347  BRANCH INDIRECT ON CONDITION" );
    DIS_FAC_INS( B9EC, "MGRK    B9EC  MULTIPLY (128 <- 64)" );
    DIS_FAC_INS( E384, "MG      E384  MULTIPLY (128 <- 64)" );
    DIS_FAC_INS( E33C, "MGH     E33C  MULTIPLY HALFWORD (64 <- 16)" );
    DIS_FAC_INS( B9ED, "MSGRKC  B9ED  MULTIPLY SINGLE (64)" );
    DIS_FAC_INS( B9FD, "MSRKC   B9FD  MULTIPLY SINGLE (32)" );
    DIS_FAC_INS( E353, "MSC     E353  MULTIPLY SINGLE (32)" );
    DIS_FAC_INS( E383, "MSGC    E383  MULTIPLY SINGLE (64)" );
    DIS_FAC_INS( E339, "SGH     E339  SUBTRACT HALFWORD (64 <- 16)" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr61 )
{
    DIS_FAC_INS( B9F5, "NCRK    B9F5  AND WITH COMPLEMENT (32)" );
    DIS_FAC_INS( B9E5, "NCGRK   B9E5  AND WITH COMPLEMENT (64)" );
    DIS_FAC_INS( E50A, "MVCRL   E50A  MOVE RIGHT TO LEFT" );
    DIS_FAC_INS( B974, "NNRK    B974  NAND (32)" );
    DIS_FAC_INS( B964, "NNGRK   B964  NAND (64)" );
    DIS_FAC_INS( B977, "NXRK    B977  NOT EXCLUSIVE OR (32)" );
    DIS_FAC_INS( B967, "NXGRK   B967  NOT EXCLUSIVE OR (64)" );
    DIS_FAC_INS( B976, "NORK    B976  NOR (32)" );
    DIS_FAC_INS( B966, "NOGRK   B966  NOR (64)" );
    DIS_FAC_INS( B975, "OCRK    B975  OR WITH COMPLEMENT (32)" );
    DIS_FAC_INS( B965, "OCGRK   B965  OR WITH COMPLEMENT (64)" );
    DIS_FAC_INS( B9F0, "SELR    B9F0  SELECT (32)" );
    DIS_FAC_INS( B9E3, "SELGR   B9E3  SELECT (64)" );
    DIS_FAC_INS( B9C0, "SELFHR  B9C0  SELECT HIGH" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr66 )
{
    DIS_FAC_INS( B9AE, "RRBM    B9AE  RESET REFERENCE BITS MULTIPLE" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr67 )
{
    DIS_FAC_INS( B2ED, "ECPGA   B2ED  EXTRACT COPROCESSOR-GROUP ADDRESS" );
    DIS_FAC_INS( B2E4, "ECCTR   B2E4  EXTRACT CPU COUNTER" );
    DIS_FAC_INS( B2E5, "EPCTR   B2E5  EXTRACT PERIPHERAL COUNTER" );
    DIS_FAC_INS( B284, "LCCTL   B284  LOAD CPU-COUNTER-SET CONTROLS" );
    DIS_FAC_INS( B285, "LPCTL   B285  LOAD PERIPHERAL-COUNTER-SET CONTROLS" );
    DIS_FAC_INS( B28E, "QCTRI   B28E  QUERY COUNTER INFORMATION" );
    DIS_FAC_INS( B2E0, "SCCTR   B2E0  SET CPU COUNTER" );
    DIS_FAC_INS( B2E1, "SPCTR   B2E1  SET PERIPHERAL COUNTER" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr68 )
{
    DIS_FAC_INS( B287, "LSCTL   B287  LOAD SAMPLING CONTROLS" );
    DIS_FAC_INS( B286, "QSI     B286  QUERY SAMPLING INFORMATION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr73 )
{
    DIS_FAC_INS( B2EC, "ETND    B2EC  EXTRACT TRANSACTION NESTING DEPTH" );
    DIS_FAC_INS( E325, "NTSTG   E325  NONTRANSACTIONAL STORE (64)" );
    DIS_FAC_INS( B2FC, "TABORT  B2FC  TRANSACTION ABORT" );
    DIS_FAC_INS( E560, "TBEGIN  E560  TRANSACTION BEGIN (unconstrained)" );
    DIS_FAC_INS( B2F8, "TEND    B2F8  TRANSACTION END" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr74 )
{
    DIS_FAC_INS( B256, "STHYI   B256  STORE HYPERVISOR INFORMATION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr76 )
{
    DIS_FAC_INS( B928, "PCKMO   B928  PERFORM CRYPTOGRAPHIC KEY MGMT. OPERATIONS" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr77 )
{
    DIS_FAC_INS( B92A, "KMF     B92A  CIPHER MESSAGE WITH CIPHER FEEDBACK" );
    DIS_FAC_INS( B92D, "KMCTR   B92D  CIPHER MESSAGE WITH COUNTER" );
    DIS_FAC_INS( B92B, "KMO     B92B  CIPHER MESSAGE WITH OUTPUT FEEDBACK" );
    DIS_FAC_INS( B92C, "PCC     B92C  PERFORM CRYPTOGRAPHIC COMPUTATION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr78 )
{
    DIS_FAC_INS( B98F, "CRDTE   B98F  COMPARE AND REPLACE DAT TABLE ENTRY" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr80 )
{
    DIS_FAC_INS( EDAE, "CDPT    EDAE  CONVERT FROM PACKED (to long DFP)" );
    DIS_FAC_INS( EDAF, "CXPT    EDAF  CONVERT FROM PACKED (to extended DFP)" );
    DIS_FAC_INS( EDAC, "CPDT    EDAC  CONVERT TO PACKED (from long DFP)" );
    DIS_FAC_INS( EDAD, "CPXT    EDAD  CONVERT TO PACKED (from extended DFP)" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr129 )
{
    DIS_FAC_INS( E727, "LCBB    E727  LOAD COUNT TO BLOCK BOUNDARY" );
    DIS_FAC_INS( E7F3, "VA      E7F3  VECTOR ADD" );
    DIS_FAC_INS( E7F1, "VACC    E7F1  VECTOR ADD COMPUTE CARRY" );
    DIS_FAC_INS( E7BB, "VAC     E7BB  VECTOR ADD WITH CARRY" );
    DIS_FAC_INS( E7B9, "VACCC   E7B9  VECTOR ADD WITH CARRY COMPUTE CARRY" );
    DIS_FAC_INS( E768, "VN      E768  VECTOR AND" );
    DIS_FAC_INS( E769, "VNC     E769  VECTOR AND WITH COMPLEMENT" );
    DIS_FAC_INS( E7F2, "VAVG    E7F2  VECTOR AVERAGE" );
    DIS_FAC_INS( E7F0, "VAVGL   E7F0  VECTOR AVERAGE LOGICAL" );
    DIS_FAC_INS( E766, "VCKSM   E766  VECTOR CHECKSUM" );
    DIS_FAC_INS( E7F8, "VCEQ    E7F8  VECTOR COMPARE EQUAL" );
    DIS_FAC_INS( E7FB, "VCH     E7FB  VECTOR COMPARE HIGH" );
    DIS_FAC_INS( E7F9, "VCHL    E7F9  VECTOR COMPARE HIGH LOGICAL" );
    DIS_FAC_INS( E753, "VCLZ    E753  VECTOR COUNT LEADING ZEROS" );
    DIS_FAC_INS( E752, "VCTZ    E752  VECTOR COUNT TRAILING ZEROS" );
    DIS_FAC_INS( E7DB, "VEC     E7DB  VECTOR ELEMENT COMPARE" );
    DIS_FAC_INS( E7D9, "VECL    E7D9  VECTOR ELEMENT COMPARE LOGICAL" );
    DIS_FAC_INS( E772, "VERIM   E772  VECTOR ELEMENT ROTATE AND INSERT UNDER MASK" );
    DIS_FAC_INS( E733, "VERLL   E733  VECTOR ELEMENT ROTATE LEFT LOGICAL" );
    DIS_FAC_INS( E773, "VERLLV  E773  VECTOR ELEMENT ROTATE LEFT LOGICAL" );
    DIS_FAC_INS( E730, "VESL    E730  VECTOR ELEMENT SHIFT LEFT" );
    DIS_FAC_INS( E770, "VESLV   E770  VECTOR ELEMENT SHIFT LEFT" );
    DIS_FAC_INS( E73A, "VESRA   E73A  VECTOR ELEMENT SHIFT RIGHT ARITHMETIC" );
    DIS_FAC_INS( E77A, "VESRAV  E77A  VECTOR ELEMENT SHIFT RIGHT ARITHMETIC" );
    DIS_FAC_INS( E738, "VESRL   E738  VECTOR ELEMENT SHIFT RIGHT LOGICAL" );
    DIS_FAC_INS( E778, "VESRLV  E778  VECTOR ELEMENT SHIFT RIGHT LOGICAL" );
    DIS_FAC_INS( E76D, "VX      E76D  VECTOR EXCLUSIVE OR" );
    DIS_FAC_INS( E782, "VFAE    E782  VECTOR FIND ANY ELEMENT EQUAL" );
    DIS_FAC_INS( E780, "VFEE    E780  VECTOR FIND ELEMENT EQUAL" );
    DIS_FAC_INS( E781, "VFENE   E781  VECTOR FIND ELEMENT NOT EQUAL" );
    DIS_FAC_INS( E7E3, "VFA     E7E3  VECTOR FP ADD" );
    DIS_FAC_INS( E7CA, "WFK     E7CA  VECTOR FP COMPARE AND SIGNAL SCALAR" );
    DIS_FAC_INS( E7E8, "VFCE    E7E8  VECTOR FP COMPARE EQUAL" );
    DIS_FAC_INS( E7EB, "VFCH    E7EB  VECTOR FP COMPARE HIGH" );
    DIS_FAC_INS( E7EA, "VFCHE   E7EA  VECTOR FP COMPARE HIGH OR EQUAL" );
    DIS_FAC_INS( E7CB, "WFC     E7CB  VECTOR FP COMPARE SCALAR" );
    DIS_FAC_INS( E7C3, "VCDG    E7C3  VECTOR FP CONVERT FROM FIXED 64-BIT" );
    DIS_FAC_INS( E7C1, "VCDLG   E7C1  VECTOR FP CONVERT FROM LOGICAL 64-BIT" );
    DIS_FAC_INS( E7C2, "VCGD    E7C2  VECTOR FP CONVERT TO FIXED 64-BIT" );
    DIS_FAC_INS( E7C0, "VCLGD   E7C0  VECTOR FP CONVERT TO LOGICAL 64-BIT" );
    DIS_FAC_INS( E7E5, "VFD     E7E5  VECTOR FP DIVIDE" );
    DIS_FAC_INS( E7C4, "VFLL    E7C4  VECTOR FP LOAD LENGTHENED" );
    DIS_FAC_INS( E7C5, "VFLR    E7C5  VECTOR FP LOAD ROUNDED" );
    DIS_FAC_INS( E7E7, "VFM     E7E7  VECTOR FP MULTIPLY" );
    DIS_FAC_INS( E78F, "VFMA    E78F  VECTOR FP MULTIPLY AND ADD" );
    DIS_FAC_INS( E78E, "VFMS    E78E  VECTOR FP MULTIPLY AND SUBTRACT" );
    DIS_FAC_INS( E7CC, "VFPSO   E7CC  VECTOR FP PERFORM SIGN OPERATION" );
    DIS_FAC_INS( E7CE, "VFSQ    E7CE  VECTOR FP SQUARE ROOT" );
    DIS_FAC_INS( E7E2, "VFS     E7E2  VECTOR FP SUBTRACT" );
    DIS_FAC_INS( E74A, "VFTCI   E74A  VECTOR FP TEST DATA CLASS IMMEDIATE" );
    DIS_FAC_INS( E7B4, "VGFM    E7B4  VECTOR GALOIS FIELD MULTIPLY SUM" );
    DIS_FAC_INS( E7BC, "VGFMA   E7BC  VECTOR GALOIS FIELD MULTIPLY SUM AND ACCUMULATE" );
    DIS_FAC_INS( E712, "VGEG    E712  VECTOR GATHER ELEMENT (64)" );
    DIS_FAC_INS( E713, "VGEF    E713  VECTOR GATHER ELEMENT (32)" );
    DIS_FAC_INS( E744, "VGBM    E744  VECTOR GENERATE BYTE MASK" );
    DIS_FAC_INS( E746, "VGM     E746  VECTOR GENERATE MASK" );
    DIS_FAC_INS( E75C, "VISTR   E75C  VECTOR ISOLATE STRING" );
    DIS_FAC_INS( E706, "VL      E706  VECTOR LOAD" );
    DIS_FAC_INS( E756, "VLR     E756  VECTOR LOAD" );
    DIS_FAC_INS( E705, "VLREP   E705  VECTOR LOAD AND REPLICATE" );
    DIS_FAC_INS( E7DE, "VLC     E7DE  VECTOR LOAD COMPLEMENT" );
    DIS_FAC_INS( E700, "VLEB    E700  VECTOR LOAD ELEMENT (8)" );
    DIS_FAC_INS( E701, "VLEH    E701  VECTOR LOAD ELEMENT (16)" );
    DIS_FAC_INS( E702, "VLEG    E702  VECTOR LOAD ELEMENT (64)" );
    DIS_FAC_INS( E703, "VLEF    E703  VECTOR LOAD ELEMENT (32)" );
    DIS_FAC_INS( E740, "VLEIB   E740  VECTOR LOAD ELEMENT IMMEDIATE (8)" );
    DIS_FAC_INS( E741, "VLEIH   E741  VECTOR LOAD ELEMENT IMMEDIATE (16)" );
    DIS_FAC_INS( E742, "VLEIG   E742  VECTOR LOAD ELEMENT IMMEDIATE (64)" );
    DIS_FAC_INS( E743, "VLEIF   E743  VECTOR LOAD ELEMENT IMMEDIATE (32)" );
    DIS_FAC_INS( E7C7, "VFI     E7C7  VECTOR LOAD FP INTEGER" );
    DIS_FAC_INS( E721, "VLGV    E721  VECTOR LOAD GR FROM VR ELEMENT" );
    DIS_FAC_INS( E704, "VLLEZ   E704  VECTOR LOAD LOGICAL ELEMENT AND ZERO" );
    DIS_FAC_INS( E736, "VLM     E736  VECTOR LOAD MULTIPLE" );
    DIS_FAC_INS( E7DF, "VLP     E7DF  VECTOR LOAD POSITIVE" );
    DIS_FAC_INS( E707, "VLBB    E707  VECTOR LOAD TO BLOCK BOUNDARY" );
    DIS_FAC_INS( E722, "VLVG    E722  VECTOR LOAD VR ELEMENT FROM GR" );
    DIS_FAC_INS( E762, "VLVGP   E762  VECTOR LOAD VR FROM GRS DISJOINT" );
    DIS_FAC_INS( E737, "VLL     E737  VECTOR LOAD WITH LENGTH" );
    DIS_FAC_INS( E7FF, "VMX     E7FF  VECTOR MAXIMUM" );
    DIS_FAC_INS( E7FD, "VMXL    E7FD  VECTOR MAXIMUM LOGICAL" );
    DIS_FAC_INS( E761, "VMRH    E761  VECTOR MERGE HIGH" );
    DIS_FAC_INS( E760, "VMRL    E760  VECTOR MERGE LOW" );
    DIS_FAC_INS( E7FE, "VMN     E7FE  VECTOR MINIMUM" );
    DIS_FAC_INS( E7FC, "VMNL    E7FC  VECTOR MINIMUM LOGICAL" );
    DIS_FAC_INS( E7AE, "VMAE    E7AE  VECTOR MULTIPLY AND ADD EVEN" );
    DIS_FAC_INS( E7AB, "VMAH    E7AB  VECTOR MULTIPLY AND ADD HIGH" );
    DIS_FAC_INS( E7AC, "VMALE   E7AC  VECTOR MULTIPLY AND ADD LOGICAL EVEN" );
    DIS_FAC_INS( E7A9, "VMALH   E7A9  VECTOR MULTIPLY AND ADD LOGICAL HIGH" );
    DIS_FAC_INS( E7AD, "VMALO   E7AD  VECTOR MULTIPLY AND ADD LOGICAL ODD" );
    DIS_FAC_INS( E7AA, "VMAL    E7AA  VECTOR MULTIPLY AND ADD LOW" );
    DIS_FAC_INS( E7AF, "VMAO    E7AF  VECTOR MULTIPLY AND ADD ODD" );
    DIS_FAC_INS( E7A6, "VME     E7A6  VECTOR MULTIPLY EVEN" );
    DIS_FAC_INS( E7A3, "VMH     E7A3  VECTOR MULTIPLY HIGH" );
    DIS_FAC_INS( E7A4, "VMLE    E7A4  VECTOR MULTIPLY LOGICAL EVEN" );
    DIS_FAC_INS( E7A1, "VMLH    E7A1  VECTOR MULTIPLY LOGICAL HIGH" );
    DIS_FAC_INS( E7A5, "VMLO    E7A5  VECTOR MULTIPLY LOGICAL ODD" );
    DIS_FAC_INS( E7A2, "VML     E7A2  VECTOR MULTIPLY LOW" );
    DIS_FAC_INS( E7A7, "VMO     E7A7  VECTOR MULTIPLY ODD" );
    DIS_FAC_INS( E76B, "VNO     E76B  VECTOR NOR" );
    DIS_FAC_INS( E76A, "VO      E76A  VECTOR OR" );
    DIS_FAC_INS( E794, "VPK     E794  VECTOR PACK" );
    DIS_FAC_INS( E795, "VPKLS   E795  VECTOR PACK LOGICAL SATURATE" );
    DIS_FAC_INS( E797, "VPKS    E797  VECTOR PACK SATURATE" );
    DIS_FAC_INS( E78C, "VPERM   E78C  VECTOR PERMUTE" );
    DIS_FAC_INS( E784, "VPDI    E784  VECTOR PERMUTE DOUBLEWORD IMMEDIATE" );
    DIS_FAC_INS( E750, "VPOPCT  E750  VECTOR POPULATION COUNT" );
    DIS_FAC_INS( E74D, "VREP    E74D  VECTOR REPLICATE" );
    DIS_FAC_INS( E745, "VREPI   E745  VECTOR REPLICATE IMMEDIATE" );
    DIS_FAC_INS( E71A, "VSCEG   E71A  VECTOR SCATTER ELEMENT (64)" );
    DIS_FAC_INS( E71B, "VSCEF   E71B  VECTOR SCATTER ELEMENT (32)" );
    DIS_FAC_INS( E78D, "VSEL    E78D  VECTOR SELECT" );
    DIS_FAC_INS( E774, "VSL     E774  VECTOR SHIFT LEFT" );
    DIS_FAC_INS( E775, "VSLB    E775  VECTOR SHIFT LEFT BY BYTE" );
    DIS_FAC_INS( E777, "VSLDB   E777  VECTOR SHIFT LEFT DOUBLE BY BYTE" );
    DIS_FAC_INS( E77E, "VSRA    E77E  VECTOR SHIFT RIGHT ARITHMETIC" );
    DIS_FAC_INS( E77F, "VSRAB   E77F  VECTOR SHIFT RIGHT ARITHMETIC BY BYTE" );
    DIS_FAC_INS( E77C, "VSRL    E77C  VECTOR SHIFT RIGHT LOGICAL" );
    DIS_FAC_INS( E77D, "VSRLB   E77D  VECTOR SHIFT RIGHT LOGICAL BY BYTE" );
    DIS_FAC_INS( E75F, "VSEG    E75F  VECTOR SIGN EXTEND TO DOUBLEWORD" );
    DIS_FAC_INS( E70E, "VST     E70E  VECTOR STORE" );
    DIS_FAC_INS( E708, "VSTEB   E708  VECTOR STORE ELEMENT (8)" );
    DIS_FAC_INS( E709, "VSTEH   E709  VECTOR STORE ELEMENT (16)" );
    DIS_FAC_INS( E70A, "VSTEG   E70A  VECTOR STORE ELEMENT (64)" );
    DIS_FAC_INS( E70B, "VSTEF   E70B  VECTOR STORE ELEMENT (32)" );
    DIS_FAC_INS( E73E, "VSTM    E73E  VECTOR STORE MULTIPLE" );
    DIS_FAC_INS( E73F, "VSTL    E73F  VECTOR STORE WITH LENGTH" );
    DIS_FAC_INS( E78A, "VSTRC   E78A  VECTOR STRING RANGE COMPARE" );
    DIS_FAC_INS( E78B, "VSTRS   E78B  VECTOR STRING SEARCH" );
    DIS_FAC_INS( E7F7, "VS      E7F7  VECTOR SUBTRACT" );
    DIS_FAC_INS( E7F5, "VSCBI   E7F5  VECTOR SUBTRACT COMPUTE BORROW INDICATION" );
    DIS_FAC_INS( E7BD, "VSBCBI  E7BD  VECTOR SUBTRACT WITH BORROW COMPUTE BORROW INDICATION" );
    DIS_FAC_INS( E7BF, "VSBI    E7BF  VECTOR SUBTRACT WITH BORROW INDICATION" );
    DIS_FAC_INS( E765, "VSUMG   E765  VECTOR SUM ACROSS DOUBLEWORD" );
    DIS_FAC_INS( E767, "VSUMQ   E767  VECTOR SUM ACROSS QUADWORD" );
    DIS_FAC_INS( E764, "VSUM    E764  VECTOR SUM ACROSS WORD" );
    DIS_FAC_INS( E7D8, "VTM     E7D8  VECTOR TEST UNDER MASK" );
    DIS_FAC_INS( E7D7, "VUPH    E7D7  VECTOR UNPACK HIGH" );
    DIS_FAC_INS( E7D5, "VUPLH   E7D5  VECTOR UNPACK LOGICAL HIGH" );
    DIS_FAC_INS( E7D4, "VUPLL   E7D4  VECTOR UNPACK LOGICAL LOW" );
    DIS_FAC_INS( E7D6, "VUPL    E7D6  VECTOR UNPACK LOW" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr133 )
{
    DIS_FAC_INS( E34C, "LGG     E34C  LOAD GUARDED (64)" );
    DIS_FAC_INS( E34D, "LGSC    E34D  LOAD GUARDED STORAGE CONTROLS" );
    DIS_FAC_INS( E348, "LLGFSG  E348  LOAD LOGICAL AND SHIFT GUARDED (64 <- 32)" );
    DIS_FAC_INS( E349, "STGSC   E349  STORE GUARDED STORAGE CONTROLS" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr134 )
{
    DIS_FAC_INS( E671, "VAP     E671  VECTOR ADD DECIMAL" );
    DIS_FAC_INS( E677, "VCP     E677  VECTOR COMPARE DECIMAL" );
    DIS_FAC_INS( E650, "VCVB    E650  VECTOR CONVERT TO BINARY" );
    DIS_FAC_INS( E652, "VCVBG   E652  VECTOR CONVERT TO BINARY" );
    DIS_FAC_INS( E658, "VCVD    E658  VECTOR CONVERT TO DECIMAL" );
    DIS_FAC_INS( E65A, "VCVDG   E65A  VECTOR CONVERT TO DECIMAL" );
    DIS_FAC_INS( E67A, "VDP     E67A  VECTOR DIVIDE DECIMAL" );
    DIS_FAC_INS( E649, "VLIP    E649  VECTOR LOAD IMMEDIATE DECIMAL" );
    DIS_FAC_INS( E635, "VLRL    E635  VECTOR LOAD RIGHTMOST WITH LENGTH" );
    DIS_FAC_INS( E637, "VLRLR   E637  VECTOR LOAD RIGHTMOST WITH LENGTH" );
    DIS_FAC_INS( E679, "VMSP    E679  VECTOR MULTIPLY AND SHIFT DECIMAL" );
    DIS_FAC_INS( E678, "VMP     E678  VECTOR MULTIPLY DECIMAL" );
    DIS_FAC_INS( E634, "VPKZ    E634  VECTOR PACK ZONED" );
    DIS_FAC_INS( E65B, "VPSOP   E65B  VECTOR PERFORM SIGN OPERATION DECIMAL" );
    DIS_FAC_INS( E67B, "VRP     E67B  VECTOR REMAINDER DECIMAL" );
    DIS_FAC_INS( E67E, "VSDP    E67E  VECTOR SHIFT AND DIVIDE DECIMAL" );
    DIS_FAC_INS( E659, "VSRP    E659  VECTOR SHIFT AND ROUND DECIMAL" );
    DIS_FAC_INS( E63D, "VSTRL   E63D  VECTOR STORE RIGHTMOST WITH LENGTH" );
    DIS_FAC_INS( E63F, "VSTRLR  E63F  VECTOR STORE RIGHTMOST WITH LENGTH" );
    DIS_FAC_INS( E673, "VSP     E673  VECTOR SUBTRACT DECIMAL" );
    DIS_FAC_INS( E65F, "VTP     E65F  VECTOR TEST DECIMAL" );
    DIS_FAC_INS( E63C, "VUPKZ   E63C  VECTOR UNPACK ZONED" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr142 )
{
    DIS_FAC_INS( EB17, "STCCTM  EB17  STORE CPU COUNTER MULTIPLE" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr144 )
{
    DIS_FAC_INS( B9A1, "TPEI    B9A1  TEST PENDING EXTERNAL INTERRUPTION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr145 )
{
    DIS_FAC_INS( B9AC, "IRBM    B9AC  INSERT REFERENCE BITS MULTIPLE" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr146 )
{
    DIS_FAC_INS( B929, "KMA     B929  CIPHER MESSAGE WITH AUTHENTICATION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr148 )
{
    DIS_FAC_INS( E785, "VBPERM  E785  VECTOR BIT PERMUTE" );
    DIS_FAC_INS( E7EF, "VFMAX   E7EF  VECTOR FP MAXIMUM" );
    DIS_FAC_INS( E7EE, "VFMIN   E7EE  VECTOR FP MINIMUM" );
    DIS_FAC_INS( E79F, "VFNMA   E79F  VECTOR FP NEGATIVE MULTIPLY AND ADD" );
    DIS_FAC_INS( E79E, "VFNMS   E79E  VECTOR FP NEGATIVE MULTIPLY AND SUBTRACT" );
    DIS_FAC_INS( E7B8, "VMSL    E7B8  VECTOR MULTIPLY SUM LOGICAL" );
    DIS_FAC_INS( E76E, "VNN     E76E  VECTOR NAND" );
    DIS_FAC_INS( E76C, "VNX     E76C  VECTOR NOT EXCLUSIVE OR" );
    DIS_FAC_INS( E76F, "VOC     E76F  VECTOR OR WITH COMPLEMENT" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr150 )
{
    DIS_FAC_INS( B938, "SORTL   B938   SORT LISTS" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr151 )
{
    DIS_FAC_INS( B939, "DFLTCC  B939  DEFLATE CONVERSION CALL" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr155 )
{
    DIS_FAC_INS( B93A, "KDSA    B93A  COMPUTE DIGITAL SIGNATURE AUTHENTICATION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr165 )
{
    DIS_FAC_INS( B93B, "NNPA    B93B  NEURAL NETWORK PROCESSING ASSIST" );
    DIS_FAC_INS( E656, "VCLFNH  E656  VECTOR FP CONVERT AND LENGTHEN FROM NNP HIGH" );
    DIS_FAC_INS( E65E, "VCLFNL  E65E  VECTOR FP CONVERT AND LENGTHEN FROM NNP LOW" );
    DIS_FAC_INS( E675, "VCRNF   E675  VECTOR FP CONVERT AND ROUND TO NNP" );
    DIS_FAC_INS( E65D, "VCFN    E65D  VECTOR FP CONVERT FROM NNP" );
    DIS_FAC_INS( E655, "VCNF    E655  VECTOR FP CONVERT TO NNP" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr192 )
{
    DIS_FAC_INS( E67C, "VSCSHP  E67C  DECIMAL SCALE AND CONVERT AND SPLIT TO HFP" );
    DIS_FAC_INS( E674, "VSCHP   E674  DECIMAL SCALE AND CONVERT TO HFP" );
    DIS_FAC_INS( E67D, "VCSPH   E67D  VECTOR CONVERT HFP TO SCALED DECIMAL" );
    DIS_FAC_INS( E651, "VCLZDP  E651  VECTOR COUNT LEADING ZERO DIGITS" );
    DIS_FAC_INS( E670, "VPKZR   E670  VECTOR PACK ZONED REGISTER" );
    DIS_FAC_INS( E672, "VSRPR   E672  VECTOR SHIFT AND ROUND DECIMAL REGISTER" );
    DIS_FAC_INS( E654, "VUPKZH  E654  VECTOR UNPACK ZONED HIGH" );
    DIS_FAC_INS( E65C, "VUPKZL  E65C  VECTOR UNPACK ZONED LOW" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr193 )
{
    DIS_FAC_INS( B200, "LBEAR   B200  LOAD BEAR" );
    DIS_FAC_INS( EB71, "LPSWEY  EB71  LOAD PSW EXTENDED" );
    DIS_FAC_INS( B201, "STBEAR  B201  STORE BEAR" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr194 )
{
    DIS_FAC_INS( B98B, "RDP     B98B  RESET DAT PROTECTION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( instr196 )
{
    DIS_FAC_INS( B28F, "QPACI   B28F  QUERY PROCESSOR ACTIVITY COUNTER INFORMATION" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( hercmvcin )
{
    DIS_FAC_INS( E8, "MVCIN E8 MOVE INVERSE" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( hercsvs )
{
    DIS_FAC_INS( B265, "SVS B265 Set Vector Summary" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( hercessa )
{
    DIS_FAC_INS( B9AB, "ESSA    B9AB  EXTRACT AND SET STORAGE ATTRIBUTES" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( herc37X )
{
    DIS_FAC_INS( 0B,   "branch_and_set_mode" );
    DIS_FAC_INS( 0C,   "branch_and_save_and_set_mode" );
    DIS_FAC_INS( 0102, "update_tree" );
    DIS_FAC_INS( B21A, "compare_and_form_codeword" );

    DIS_FAC_INS( 71,   "multiply_single" );
    DIS_FAC_INS( 84,   "branch_relative_on_index_high" );
    DIS_FAC_INS( 85,   "branch_relative_on_index_low_or_equal" );
    DIS_FAC_INS( A8,   "move_long_extended" );
    DIS_FAC_INS( A9,   "compare_logical_long_extended" );

    DIS_FAC_INS( A502, "insert_immediate_low_high" );
    DIS_FAC_INS( A503, "insert_immediate_low_low" );
    DIS_FAC_INS( A506, "and_immediate_low_high" );
    DIS_FAC_INS( A507, "and_immediate_low_low" );
    DIS_FAC_INS( A50A, "or_immediate_low_high" );
    DIS_FAC_INS( A50B, "or_immediate_low_low" );
    DIS_FAC_INS( A50E, "load_logical_immediate_low_high" );
    DIS_FAC_INS( A50F, "load_logical_immediate_low_low" );

    DIS_FAC_INS( A700, "test_under_mask_high" );
    DIS_FAC_INS( A701, "test_under_mask_low" );
    DIS_FAC_INS( A704, "branch_relative_on_condition" );
    DIS_FAC_INS( A705, "branch_relative_and_save" );
    DIS_FAC_INS( A706, "branch_relative_on_count" );
    DIS_FAC_INS( A708, "load_halfword_immediate" );
    DIS_FAC_INS( A70A, "add_halfword_immediate" );
    DIS_FAC_INS( A70C, "multiply_halfword_immediate" );
    DIS_FAC_INS( A70E, "compare_halfword_immediate" );

    DIS_FAC_INS( B241, "checksum" );
    DIS_FAC_INS( B244, "squareroot_float_long_reg" );
    DIS_FAC_INS( B245, "squareroot_float_short_reg" );

    DIS_FAC_INS( B252, "multiply_single_register" );
    DIS_FAC_INS( B255, "move_string" );
    DIS_FAC_INS( B257, "compare_until_substring_equal" );
    DIS_FAC_INS( B25D, "compare_logical_string" );
    DIS_FAC_INS( B25E, "search_string" );

    DIS_FAC_INS( B263, "cmpsc_2012" );

    DIS_FAC_INS( B299, "set_bfp_rounding_mode_2bit" );
    DIS_FAC_INS( B29C, "store_fpc" );
    DIS_FAC_INS( B29D, "load_fpc" );

    DIS_FAC_INS( B2A5, "translate_extended" );
    DIS_FAC_INS( B2A6, "convert_utf16_to_utf8" );
    DIS_FAC_INS( B2A7, "convert_utf8_to_utf16" );

    DIS_FAC_INS( B300, "load_positive_bfp_short_reg" );
    DIS_FAC_INS( B301, "load_negative_bfp_short_reg" );
    DIS_FAC_INS( B302, "load_and_test_bfp_short_reg" );
    DIS_FAC_INS( B303, "load_complement_bfp_short_reg" );
    DIS_FAC_INS( B304, "load_lengthened_bfp_short_to_long_reg" );
    DIS_FAC_INS( B305, "load_lengthened_bfp_long_to_ext_reg" );
    DIS_FAC_INS( B306, "load_lengthened_bfp_short_to_ext_reg" );
    DIS_FAC_INS( B307, "multiply_bfp_long_to_ext_reg" );
    DIS_FAC_INS( B308, "compare_and_signal_bfp_short_reg" );
    DIS_FAC_INS( B309, "compare_bfp_short_reg" );
    DIS_FAC_INS( B30A, "add_bfp_short_reg" );
    DIS_FAC_INS( B30B, "subtract_bfp_short_reg" );
    DIS_FAC_INS( B30C, "multiply_bfp_short_to_long_reg" );
    DIS_FAC_INS( B30D, "divide_bfp_short_reg" );
    DIS_FAC_INS( B30E, "multiply_add_bfp_short_reg" );
    DIS_FAC_INS( B30F, "multiply_subtract_bfp_short_reg" );

    DIS_FAC_INS( B310, "load_positive_bfp_long_reg" );
    DIS_FAC_INS( B311, "load_negative_bfp_long_reg" );
    DIS_FAC_INS( B312, "load_and_test_bfp_long_reg" );
    DIS_FAC_INS( B313, "load_complement_bfp_long_reg" );
    DIS_FAC_INS( B314, "squareroot_bfp_short_reg" );
    DIS_FAC_INS( B315, "squareroot_bfp_long_reg" );
    DIS_FAC_INS( B316, "squareroot_bfp_ext_reg" );
    DIS_FAC_INS( B317, "multiply_bfp_short_reg" );
    DIS_FAC_INS( B318, "compare_and_signal_bfp_long_reg" );
    DIS_FAC_INS( B319, "compare_bfp_long_reg" );
    DIS_FAC_INS( B31A, "add_bfp_long_reg" );
    DIS_FAC_INS( B31B, "subtract_bfp_long_reg" );
    DIS_FAC_INS( B31C, "multiply_bfp_long_reg" );
    DIS_FAC_INS( B31D, "divide_bfp_long_reg" );
    DIS_FAC_INS( B31E, "multiply_add_bfp_long_reg" );
    DIS_FAC_INS( B31F, "multiply_subtract_bfp_long_reg" );

    DIS_FAC_INS( B324, "load_lengthened_float_short_to_long_reg" );
    DIS_FAC_INS( B325, "load_lengthened_float_long_to_ext_reg" );
    DIS_FAC_INS( B326, "load_lengthened_float_short_to_ext_reg" );

    DIS_FAC_INS( B336, "squareroot_float_ext_reg" );
    DIS_FAC_INS( B337, "multiply_float_short_reg" );

    DIS_FAC_INS( B340, "load_positive_bfp_ext_reg" );
    DIS_FAC_INS( B341, "load_negative_bfp_ext_reg" );
    DIS_FAC_INS( B342, "load_and_test_bfp_ext_reg" );
    DIS_FAC_INS( B343, "load_complement_bfp_ext_reg" );
    DIS_FAC_INS( B344, "load_rounded_bfp_long_to_short_reg" );
    DIS_FAC_INS( B345, "load_rounded_bfp_ext_to_long_reg" );
    DIS_FAC_INS( B346, "load_rounded_bfp_ext_to_short_reg" );
    DIS_FAC_INS( B347, "load_fp_int_bfp_ext_reg" );
    DIS_FAC_INS( B348, "compare_and_signal_bfp_ext_reg" );
    DIS_FAC_INS( B349, "compare_bfp_ext_reg" );
    DIS_FAC_INS( B34A, "add_bfp_ext_reg" );
    DIS_FAC_INS( B34B, "subtract_bfp_ext_reg" );
    DIS_FAC_INS( B34C, "multiply_bfp_ext_reg" );
    DIS_FAC_INS( B34D, "divide_bfp_ext_reg" );

    DIS_FAC_INS( B350, "convert_float_long_to_bfp_short_reg" );
    DIS_FAC_INS( B351, "convert_float_long_to_bfp_long_reg" );
    DIS_FAC_INS( B353, "divide_integer_bfp_short_reg" );
    DIS_FAC_INS( B357, "load_fp_int_bfp_short_reg" );
    DIS_FAC_INS( B358, "convert_bfp_short_to_float_long_reg" );
    DIS_FAC_INS( B359, "convert_bfp_long_to_float_long_reg" );
    DIS_FAC_INS( B35B, "divide_integer_bfp_long_reg" );
    DIS_FAC_INS( B35F, "load_fp_int_bfp_long_reg" );

    DIS_FAC_INS( B360, "load_positive_float_ext_reg" );
    DIS_FAC_INS( B361, "load_negative_float_ext_reg" );
    DIS_FAC_INS( B362, "load_and_test_float_ext_reg" );
    DIS_FAC_INS( B363, "load_complement_float_ext_reg" );
    DIS_FAC_INS( B365, "load_float_ext_reg" );
    DIS_FAC_INS( B366, "load_rounded_float_ext_to_short_reg" );
    DIS_FAC_INS( B367, "load_fp_int_float_ext_reg" );
    DIS_FAC_INS( B369, "compare_float_ext_reg" );

    DIS_FAC_INS( B374, "load_zero_float_short_reg" );
    DIS_FAC_INS( B375, "load_zero_float_long_reg" );
    DIS_FAC_INS( B376, "load_zero_float_ext_reg" );
    DIS_FAC_INS( B377, "load_fp_int_float_short_reg" );
    DIS_FAC_INS( B37F, "load_fp_int_float_long_reg" );

    DIS_FAC_INS( B384, "set_fpc" );
    DIS_FAC_INS( B38C, "extract_fpc" );

    DIS_FAC_INS( B394, "convert_fix32_to_bfp_short_reg" );
    DIS_FAC_INS( B395, "convert_fix32_to_bfp_long_reg" );
    DIS_FAC_INS( B396, "convert_fix32_to_bfp_ext_reg" );
    DIS_FAC_INS( B398, "convert_bfp_short_to_fix32_reg" );
    DIS_FAC_INS( B399, "convert_bfp_long_to_fix32_reg" );
    DIS_FAC_INS( B39A, "convert_bfp_ext_to_fix32_reg" );

    DIS_FAC_INS( B3B4, "convert_fixed_to_float_short_reg" );
    DIS_FAC_INS( B3B5, "convert_fixed_to_float_long_reg" );
    DIS_FAC_INS( B3B6, "convert_fixed_to_float_ext_reg" );
    DIS_FAC_INS( B3B8, "convert_float_short_to_fixed_reg" );
    DIS_FAC_INS( B3B9, "convert_float_long_to_fixed_reg" );
    DIS_FAC_INS( B3BA, "convert_float_ext_to_fixed_reg" );

    DIS_FAC_INS( ED04, "load_lengthened_bfp_short_to_long" );
    DIS_FAC_INS( ED05, "load_lengthened_bfp_long_to_ext" );
    DIS_FAC_INS( ED06, "load_lengthened_bfp_short_to_ext" );
    DIS_FAC_INS( ED07, "multiply_bfp_long_to_ext" );
    DIS_FAC_INS( ED08, "compare_and_signal_bfp_short" );
    DIS_FAC_INS( ED09, "compare_bfp_short" );
    DIS_FAC_INS( ED0A, "add_bfp_short" );
    DIS_FAC_INS( ED0B, "subtract_bfp_short" );
    DIS_FAC_INS( ED0C, "multiply_bfp_short_to_long" );
    DIS_FAC_INS( ED0D, "divide_bfp_short" );
    DIS_FAC_INS( ED0E, "multiply_add_bfp_short" );
    DIS_FAC_INS( ED0F, "multiply_subtract_bfp_short" );

    DIS_FAC_INS( ED10, "test_data_class_bfp_short" );
    DIS_FAC_INS( ED11, "test_data_class_bfp_long" );
    DIS_FAC_INS( ED12, "test_data_class_bfp_ext" );
    DIS_FAC_INS( ED14, "squareroot_bfp_short" );
    DIS_FAC_INS( ED15, "squareroot_bfp_long" );
    DIS_FAC_INS( ED17, "multiply_bfp_short" );
    DIS_FAC_INS( ED18, "compare_and_signal_bfp_long" );
    DIS_FAC_INS( ED19, "compare_bfp_long" );
    DIS_FAC_INS( ED1A, "add_bfp_long" );
    DIS_FAC_INS( ED1B, "subtract_bfp_long" );
    DIS_FAC_INS( ED1C, "multiply_bfp_long" );
    DIS_FAC_INS( ED1D, "divide_bfp_long" );
    DIS_FAC_INS( ED1E, "multiply_add_bfp_long" );
    DIS_FAC_INS( ED1F, "multiply_subtract_bfp_long" );

    DIS_FAC_INS( ED24, "load_lengthened_float_short_to_long" );
    DIS_FAC_INS( ED25, "load_lengthened_float_long_to_ext" );
    DIS_FAC_INS( ED26, "load_lengthened_float_short_to_ext" );

    DIS_FAC_INS( ED34, "squareroot_float_short" );
    DIS_FAC_INS( ED35, "squareroot_float_long" );
    DIS_FAC_INS( ED37, "multiply_float_short" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/

BEG_DIS_FAC_INS_FUNC( herctcp )
{
    DIS_FAC_INS( 75,   "TCPIP   75    TCPIP" );
}
END_DIS_FAC_INS_FUNC()

/*-------------------------------------------------------------------*/
/*     Special handling for HERC_370_EXTENSION Pseudo-Facility       */
/*-------------------------------------------------------------------*/
static void enable_disable_herc37X( bool enable )
{
    unsigned int orig_msglvl;
    char   cmdbuf[ 256 ];
    char*  argv[ MAX_ARGS ];
    int    argc, i;
    static const int facils[] =      /* List of dependent facilities */
    {
        STFL_000_N3_INSTR,
        STFL_016_EXT_TRANSL_2,
        STFL_017_MSA,
        STFL_018_LONG_DISPL_INST,
        STFL_020_HFP_MULT_ADD_SUB,
        STFL_021_EXTENDED_IMMED,
        STFL_022_EXT_TRANSL_3,
        STFL_023_HFP_UNNORM_EXT,
        STFL_026_PARSING_ENHANCE,
        STFL_032_CSSF,
        STFL_034_GEN_INST_EXTN,
        STFL_035_EXECUTE_EXTN,

        // 42 must be enabled BEFORE 37 or disabled AFTER 37
        STFL_042_DFP,
        STFL_037_FP_EXTENSION,

        STFL_041_FPS_ENHANCEMENT,
        STFL_041_DFP_ROUNDING,
        STFL_041_FPR_GR_TRANSFER,
        STFL_041_FPS_SIGN_HANDLING,
        STFL_041_IEEE_EXCEPT_SIM,
//      STFL_042_DFP,
        STFL_045_DISTINCT_OPERANDS,
        STFL_045_FAST_BCR_SERIAL,
        STFL_045_HIGH_WORD,
        STFL_045_INTERLOCKED_ACCESS_1,
        STFL_045_LOAD_STORE_ON_COND_1,
        STFL_045_POPULATION_COUNT,
        STFL_076_MSA_EXTENSION_3,
        STFL_077_MSA_EXTENSION_4,
    };

    /* Temporarily suppress logging */
    orig_msglvl = sysblk.msglvl;
    sysblk.msglvl &= ~MLVL_VERBOSE;

    /* Enable / disable all dependent facilities in the proper order */
    if (enable)
    {
        for (i=0; i < (int) _countof( facils ); ++i)
        {
            MSGBUF( cmdbuf, "facility enable %d", facils[i] );
            parse_args( cmdbuf, MAX_ARGS, argv, &argc );
            VERIFY( facility_enable_disable( argc, argv ) == 0);
        }
    }
    else // disable
    {
        for (i = _countof( facils ) - 1; i >= 0; --i)
        {
            MSGBUF( cmdbuf, "facility disable %d", facils[i] );
            parse_args( cmdbuf, MAX_ARGS, argv, &argc );
            VERIFY( facility_enable_disable( argc, argv ) == 0);
        }
    }

    /* Restore original msglevel */
    sysblk.msglvl = orig_msglvl;
}

/*-------------------------------------------------------------------*/
/*                    facility_enable_disable                        */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* FACILITY ENABLE | DISABLE <facility> | bit [S/370|ESA/390|z/ARCH] */
/*                                                                   */
/*-------------------------------------------------------------------*/
int facility_enable_disable( int argc, char* argv[] )
{
    const ARCHTAB*  at;
    const FACTAB*   ft;
    const char*     sev;

    int   bitno, fbyte, fbit;
    bool  enable, forced = false;
    bool  enabled, modified;

    UPPER_ARGV_0( argv );

    if (argc < 3 || argc > 4)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    if      (CMD( argv[1], ENABLE,  1 )) enable = true;
    else if (CMD( argv[1], DISABLE, 1 )) enable = false;
    else
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[1], "; use 'ENABLE' or 'DISABLE'" );
        return -1;
    }

    /* The facility list cannot be updated once system is IPLed */
    if (sysblk.ipled)
    {
        // "Available facilities cannot be changed once system is IPLed"
        WRMSG( HHC00889, "E" );
        return -1;
    }

    /* Make sure all CPUs are deconfigured or stopped */
    if (are_any_cpus_started())
    {
        // "All CPU's must be stopped %s"
        WRMSG( HHC02253, "E", "to ENABLE or DISABLE a facility" );
        return HERRCPUONL;
    }

    /* Get pointer to Architecture Table entry */
    if (argc < 4)
    {
        at = get_archtab_by_arch( sysblk.arch_mode );
    }
    else // (argc == 4)
    {
        if (!(at = get_archtab_by_name( argv[3] )))
        {
            // "ARCHLVL '%s' is invalid"
            WRMSG( HHC00895, "E", argv[3] );
            return -1;
        }
    }

    /* Get pointer to specified Facility Table entry */
    if (!(ft = get_factab_by_name( argv[2] )))
    {
        const char* p;
        char  c;

        if (strncasecmp( "BIT", p = argv[2], 3 ) == 0)
            p += 3;

        if (0
            || !isdigit( (unsigned char)*p )
            || sscanf( p, "%d%c", &bitno, &c ) != 1
            || bitno < 0
            || bitno > (int) STFL_HERC_LAST_BIT
            || !(ft = get_factab_by_bitno( bitno ))
        )
        {
            // "Facility( %s ) does not exist for %s"
            WRMSG( HHC00892, "E", argv[2], at->name );
            return -1;
        }

        forced = true;  // (bypass "supported" check)
    }
    else
        bitno = ft->bitno;

    /* Get the bit and byte number */
    fbyte  =         (bitno / 8);
    fbit   = 0x80 >> (bitno % 8);

    /* Set some flags */
    enabled  = sysblk.facility_list[ at->num ][ fbyte ] & fbit;

    modified = ( (ft->defmask & at->amask) && !enabled) ||
               (!(ft->defmask & at->amask) &&  enabled);

    /* Verify facility is supported for this architecture */
    if (1
        && !forced
        && !modified
        && !(ft->supmask & at->amask)   // (supported?)
    )
    {
        // "Facility( %s ) not supported for %s"
        WRMSG( HHC00896, "E", ft->name, at->name );
        return -1;
    }

    /* Check for facility enable/disable pre-requisite violation */
    if (ft->modokfunc)
    {
        const char*  action          = enable? "enable"    : "disable";
        const char*  actioning       = enable? "enabling"  : "disabling";
        const char*  opp_actioning   = enable? "DISABLING" : "ENABLING";
        const char*  target_facname  = get_facname_by_bitno( bitno,
                                                      &target_facname );

        if (!ft->modokfunc( enable, ft->bitno, at->num,
            action, actioning, opp_actioning, target_facname ))
            return -1; // (error msg already issued)
    }

    /* If disabling, don't allow if required */
    if (!enable && (ft->reqmask & at->amask))
    {
        // "Facility( %s ) is required for %s"
        WRMSG( HHC00897, "E", ft->name, at->name );
        return -1;
    }

    /* Special handling for HERC_370_EXTENSION pseudo-facility */
    if (STFL_HERC_370_EXTENSION == bitno)
    {
        /* Enable/disable all dependent facilities first */
        enable_disable_herc37X( enable );
        /* Then fall through to enable/disable 370X itself */
    }

    /* Enable or disable the requested facility in SYSBLK */
    if (enable)
        sysblk.facility_list[ at->num ][ fbyte ] |= fbit;
    else
        sysblk.facility_list[ at->num ][ fbyte ] &= ~fbit;

    /* Refresh each online CPU's facility list with updated list */
    {
        int  cpu;
        for (cpu=0; cpu < sysblk.maxcpu; cpu++)
            if (IS_CPU_ONLINE( cpu ))
                init_cpu_facilities( sysblk.regs[ cpu ] );
    }

    /* Update flags */
    enabled  = sysblk.facility_list[ at->num ][ fbyte ] & fbit;

    modified = ( (ft->defmask & at->amask) && !enabled) ||
               (!(ft->defmask & at->amask) &&  enabled);

    /* Enable/disable instructions associated with this facility */
    if (ft->updinstrs)
        ft->updinstrs( at->num, enable );

    /* Show results if requested */
    if (MLVL( VERBOSE ))
    {
        sev = modified ? "W" : "I";

        // "Facility( %s ) %s%s for %s"
        WRMSG( HHC00898, sev, ft->name, modified ? "*" : "",
            enable ? "Enabled" : "Disabled", at->name );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/*                        facility_cmd                      (public) */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* FACILITY ENABLE | DISABLE <facility> | bit [S/370|ESA/390|z/ARCH] */
/* FACILITY QUERY ENABLED | DISABLED  [LONG]                         */
/* FACILITY QUERY SHORT | LONG | ALL                                 */
/* FACILITY QUERY <facility> | bit                                   */
/* FACILITY QUERY RAW                                                */
/*                                                                   */
/*-------------------------------------------------------------------*/
int facility_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    /* Correct number of arguments? */

    if (argc < 3 || argc > 4)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    /*-----------------------------------------------------*/
    /*                Query Facility?                      */
    /*-----------------------------------------------------*/

    if (CMD( argv[1], QUERY, 1 ))
    {
        int rc = facility_query( argc, argv ) ? 0 : -1;
        return rc;
    }

    /*-----------------------------------------------------*/
    /*             Enable/Disable Facility?                */
    /*-----------------------------------------------------*/

    if (0
        || CMD( argv[1], ENABLE,  3 )
        || CMD( argv[1], DISABLE, 3 )
    )
    {
        if (sysblk.ipled)
        {
            // "Available facilities cannot be changed once system is IPLed"
            WRMSG( HHC00889, "E" );
            return -1;
        }

        if (are_any_cpus_started())
        {
            // "All CPU's must be stopped %s"
            WRMSG( HHC02253, "E", "to modify a facility" );
            return HERRCPUONL;
        }
        return facility_enable_disable( argc, argv );
    }

    // "Invalid command usage. Type 'help %s' for assistance."
    WRMSG( HHC02299, "E", argv[0] );
    return -1;
}

#endif /* !defined( _GEN_ARCH ) */
