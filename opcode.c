/* OPCODE.C     (C) Copyright Jan Jaeger, 2000-2012                  */
/*              (C) Copyright Roger Bowler, 2010-2011                */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*              (C) and others 2013-2023                             */
/*              Instruction decoding functions                       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

#include "hstdinc.h"

#define _OPCODE_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"

DISABLE_GCC_UNUSED_FUNCTION_WARNING;

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*-------------------------------------------------------------------*/
/* In this section you should only test FEATURE_xxx values without   */
/* the underscore since they're defined differently for each build   */
/* architecture.  That is, you should only be testing whether the    */
/* given feature is enabled for the CURRENT build architecture.      */
/*-------------------------------------------------------------------*/

#undef  UNDEF_INST
#define UNDEF_INST( _x ) \
    DEF_INST( _x ) { ARCH_DEP( operation_exception )( inst, regs ); }

/*-------------------------------------------------------------------*/
/*      FEATUREs with STFL/STFLE facility bits defined               */
/*-------------------------------------------------------------------*/

#if !defined( FEATURE_000_N3_INSTR_FACILITY )
 UNDEF_INST( add_logical_carry )
 UNDEF_INST( add_logical_carry_register )
 UNDEF_INST( branch_relative_and_save_long )
 UNDEF_INST( branch_relative_on_condition_long )
 UNDEF_INST( divide_logical )
 UNDEF_INST( divide_logical_register )
 UNDEF_INST( extract_psw )
 UNDEF_INST( load_address_relative_long )
 UNDEF_INST( load_reversed )
 UNDEF_INST( load_reversed_half )
 UNDEF_INST( load_reversed_register )
 UNDEF_INST( multiply_logical )
 UNDEF_INST( multiply_logical_register )
 UNDEF_INST( rotate_left_single_logical )
 UNDEF_INST( set_addressing_mode_24 )
 UNDEF_INST( set_addressing_mode_31 )
 UNDEF_INST( store_facility_list )
 UNDEF_INST( store_reversed )
 UNDEF_INST( store_reversed_half )
 UNDEF_INST( subtract_logical_borrow )
 UNDEF_INST( subtract_logical_borrow_register )
 UNDEF_INST( test_addressing_mode )
#endif

#if !defined( FEATURE_003_DAT_ENHANCE_FACILITY_1 )
 UNDEF_INST( compare_and_swap_and_purge_long )
 UNDEF_INST( invalidate_dat_table_entry )
#endif

#if !defined( FEATURE_006_ASN_LX_REUSE_FACILITY )
 UNDEF_INST( extract_primary_asn_and_instance )
 UNDEF_INST( extract_secondary_asn_and_instance )
 UNDEF_INST( program_transfer_with_instance )
 UNDEF_INST( set_secondary_asn_with_instance )
#endif

#if !defined( FEATURE_007_STFL_EXTENDED_FACILITY )
 UNDEF_INST( store_facility_list_extended )
#endif

#if !defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 )
 UNDEF_INST( perform_frame_management_function )
#endif

#if !defined( FEATURE_011_CONFIG_TOPOLOGY_FACILITY )
 UNDEF_INST( perform_topology_function )
#endif

#if !defined( FEATURE_016_EXT_TRANSL_FACILITY_2 )
 UNDEF_INST( compare_logical_long_unicode )
 UNDEF_INST( move_long_unicode )
 UNDEF_INST( pack_ascii )
 UNDEF_INST( pack_unicode )
 UNDEF_INST( test_decimal )
 UNDEF_INST( translate_one_to_one )
 UNDEF_INST( translate_one_to_two )
 UNDEF_INST( translate_two_to_one )
 UNDEF_INST( translate_two_to_two )
 UNDEF_INST( unpack_ascii )
 UNDEF_INST( unpack_unicode )
#endif

#if !defined( FEATURE_017_MSA_FACILITY ) || defined( DYNINST_017_MSA_FACILITY )
 UNDEF_INST( cipher_message )
 UNDEF_INST( cipher_message_with_chaining )
 UNDEF_INST( compute_intermediate_message_digest )
 UNDEF_INST( compute_last_message_digest )
 UNDEF_INST( compute_message_authentication_code )
#endif

#if !defined( FEATURE_018_LONG_DISPL_INST_FACILITY )
 UNDEF_INST( add_y )
 UNDEF_INST( add_halfword_y )
 UNDEF_INST( add_logical_y )
 UNDEF_INST( and_immediate_y )
 UNDEF_INST( and_y )
 UNDEF_INST( compare_y )
 UNDEF_INST( compare_and_swap_y )
 UNDEF_INST( compare_double_and_swap_y )
 UNDEF_INST( compare_halfword_y )
 UNDEF_INST( compare_logical_y )
 UNDEF_INST( compare_logical_immediate_y )
 UNDEF_INST( compare_logical_characters_under_mask_y )
 UNDEF_INST( convert_to_binary_y )
 UNDEF_INST( convert_to_decimal_y )
 UNDEF_INST( exclusive_or_immediate_y )
 UNDEF_INST( exclusive_or_y )
 UNDEF_INST( insert_character_y )
 UNDEF_INST( insert_characters_under_mask_y )
 UNDEF_INST( load_y )
 UNDEF_INST( load_address_y )
 UNDEF_INST( load_byte )
 UNDEF_INST( load_byte_long )
 UNDEF_INST( load_halfword_y )
 UNDEF_INST( load_multiple_y )
 UNDEF_INST( load_real_address_y )
 UNDEF_INST( move_immediate_y )
 UNDEF_INST( multiply_single_y )
 UNDEF_INST( or_immediate_y )
 UNDEF_INST( or_y )
 UNDEF_INST( store_y )
 UNDEF_INST( store_character_y )
 UNDEF_INST( store_characters_under_mask_y )
 UNDEF_INST( store_halfword_y )
 UNDEF_INST( store_multiple_y )
 UNDEF_INST( subtract_y )
 UNDEF_INST( subtract_halfword_y )
 UNDEF_INST( subtract_logical_y )
 UNDEF_INST( test_under_mask_y )
#endif /*!defined( FEATURE_018_LONG_DISPL_INST_FACILITY )*/

#if !defined( FEATURE_018_LONG_DISPL_INST_FACILITY ) || !defined( FEATURE_ACCESS_REGISTERS )
 UNDEF_INST( load_access_multiple_y )
 UNDEF_INST( store_access_multiple_y )
#endif

#if !defined( FEATURE_018_LONG_DISPL_INST_FACILITY ) || !defined( FEATURE_HEXADECIMAL_FLOATING_POINT )
 UNDEF_INST( load_float_long_y )
 UNDEF_INST( load_float_short_y )
 UNDEF_INST( store_float_long_y )
 UNDEF_INST( store_float_short_y )
#endif

#if !defined( FEATURE_020_HFP_MULT_ADD_SUB_FACILITY )
 UNDEF_INST( multiply_add_float_short_reg )
 UNDEF_INST( multiply_add_float_long_reg )
 UNDEF_INST( multiply_add_float_short )
 UNDEF_INST( multiply_add_float_long )
 UNDEF_INST( multiply_subtract_float_short_reg )
 UNDEF_INST( multiply_subtract_float_long_reg )
 UNDEF_INST( multiply_subtract_float_short )
 UNDEF_INST( multiply_subtract_float_long )
#endif

#if !defined( FEATURE_021_EXTENDED_IMMED_FACILITY )
 UNDEF_INST( add_fullword_immediate )
 UNDEF_INST( add_logical_fullword_immediate )
 UNDEF_INST( add_logical_long_fullword_immediate )
 UNDEF_INST( add_long_fullword_immediate )
 UNDEF_INST( and_immediate_high_fullword )
 UNDEF_INST( and_immediate_low_fullword )
 UNDEF_INST( compare_fullword_immediate )
 UNDEF_INST( compare_logical_fullword_immediate )
 UNDEF_INST( compare_logical_long_fullword_immediate )
 UNDEF_INST( compare_long_fullword_immediate )
 UNDEF_INST( exclusive_or_immediate_high_fullword )
 UNDEF_INST( exclusive_or_immediate_low_fullword )
 UNDEF_INST( find_leftmost_one_long_register )
 UNDEF_INST( insert_immediate_high_fullword )
 UNDEF_INST( insert_immediate_low_fullword )
 UNDEF_INST( load_and_test )
 UNDEF_INST( load_and_test_long )
 UNDEF_INST( load_byte_register )
 UNDEF_INST( load_halfword_register )
 UNDEF_INST( load_logical_character )
 UNDEF_INST( load_logical_character_register )
 UNDEF_INST( load_logical_halfword )
 UNDEF_INST( load_logical_halfword_register )
 UNDEF_INST( load_logical_immediate_high_fullword )
 UNDEF_INST( load_logical_immediate_low_fullword )
 UNDEF_INST( load_logical_long_character_register )
 UNDEF_INST( load_logical_long_halfword_register )
 UNDEF_INST( load_long_byte_register )
 UNDEF_INST( load_long_fullword_immediate )
 UNDEF_INST( load_long_halfword_register )
 UNDEF_INST( or_immediate_high_fullword )
 UNDEF_INST( or_immediate_low_fullword )
 UNDEF_INST( subtract_logical_fullword_immediate )
 UNDEF_INST( subtract_logical_long_fullword_immediate )
#endif /*!defined( FEATURE_021_EXTENDED_IMMED_FACILITY )*/

#if !defined( FEATURE_022_EXT_TRANSL_FACILITY_3 )
 UNDEF_INST( convert_utf16_to_utf32 )
 UNDEF_INST( convert_utf32_to_utf16 )
 UNDEF_INST( convert_utf32_to_utf8 )
 UNDEF_INST( convert_utf8_to_utf32 )
 UNDEF_INST( search_string_unicode )
 UNDEF_INST( translate_and_test_reverse )
#endif

#if !defined( FEATURE_023_HFP_UNNORM_EXT_FACILITY )
 UNDEF_INST( multiply_add_unnormal_float_long_to_ext )
 UNDEF_INST( multiply_add_unnormal_float_long_to_ext_high )
 UNDEF_INST( multiply_add_unnormal_float_long_to_ext_high_reg )
 UNDEF_INST( multiply_add_unnormal_float_long_to_ext_low )
 UNDEF_INST( multiply_add_unnormal_float_long_to_ext_low_reg )
 UNDEF_INST( multiply_add_unnormal_float_long_to_ext_reg )
 UNDEF_INST( multiply_unnormal_float_long_to_ext )
 UNDEF_INST( multiply_unnormal_float_long_to_ext_high )
 UNDEF_INST( multiply_unnormal_float_long_to_ext_high_reg )
 UNDEF_INST( multiply_unnormal_float_long_to_ext_low )
 UNDEF_INST( multiply_unnormal_float_long_to_ext_low_reg )
 UNDEF_INST( multiply_unnormal_float_long_to_ext_reg )
#endif

#if !defined( FEATURE_025_STORE_CLOCK_FAST_FACILITY )
 UNDEF_INST( store_clock_fast )
#else
 #define z900_store_clock_fast    z900_store_clock
#endif

#if !defined( FEATURE_026_PARSING_ENHANCE_FACILITY )
 UNDEF_INST( translate_and_test_extended )
 UNDEF_INST( translate_and_test_reverse_extended )
#endif

#if !defined( FEATURE_027_MVCOS_FACILITY )
 UNDEF_INST( move_with_optional_specifications )
#endif

#if !defined( FEATURE_028_TOD_CLOCK_STEER_FACILITY )
 UNDEF_INST( perform_timing_facility_function )
#endif

#if !defined( FEATURE_031_EXTRACT_CPU_TIME_FACILITY )
 UNDEF_INST( extract_cpu_time )
#endif

#if !defined( FEATURE_032_CSS_FACILITY )
 UNDEF_INST( compare_and_swap_and_store )
#endif

#if !defined( FEATURE_034_GEN_INST_EXTN_FACILITY )
 UNDEF_INST( add_immediate_long_storage )
 UNDEF_INST( add_immediate_storage )
 UNDEF_INST( add_logical_with_signed_immediate )
 UNDEF_INST( add_logical_with_signed_immediate_long )
 UNDEF_INST( compare_and_branch_register )
 UNDEF_INST( compare_and_branch_long_register )
 UNDEF_INST( compare_and_branch_relative_register )
 UNDEF_INST( compare_and_branch_relative_long_register )
 UNDEF_INST( compare_and_trap_long_register )
 UNDEF_INST( compare_and_trap_register )
 UNDEF_INST( compare_halfword_immediate_halfword_storage )
 UNDEF_INST( compare_halfword_immediate_long_storage )
 UNDEF_INST( compare_halfword_immediate_storage )
 UNDEF_INST( compare_halfword_long )
 UNDEF_INST( compare_halfword_relative_long )
 UNDEF_INST( compare_halfword_relative_long_long )
 UNDEF_INST( compare_immediate_and_branch )
 UNDEF_INST( compare_immediate_and_branch_long )
 UNDEF_INST( compare_immediate_and_branch_relative )
 UNDEF_INST( compare_immediate_and_branch_relative_long )
 UNDEF_INST( compare_immediate_and_trap )
 UNDEF_INST( compare_immediate_and_trap_long )
 UNDEF_INST( compare_logical_and_branch_long_register )
 UNDEF_INST( compare_logical_and_branch_register )
 UNDEF_INST( compare_logical_and_branch_relative_long_register )
 UNDEF_INST( compare_logical_and_branch_relative_register )
 UNDEF_INST( compare_logical_and_trap_long_register )
 UNDEF_INST( compare_logical_and_trap_register )
 UNDEF_INST( compare_logical_immediate_and_branch )
 UNDEF_INST( compare_logical_immediate_and_branch_long )
 UNDEF_INST( compare_logical_immediate_and_branch_relative )
 UNDEF_INST( compare_logical_immediate_and_branch_relative_long )
 UNDEF_INST( compare_logical_immediate_and_trap_fullword )
 UNDEF_INST( compare_logical_immediate_and_trap_long )
 UNDEF_INST( compare_logical_immediate_fullword_storage )
 UNDEF_INST( compare_logical_immediate_halfword_storage )
 UNDEF_INST( compare_logical_immediate_long_storage )
 UNDEF_INST( compare_logical_relative_long )
 UNDEF_INST( compare_logical_relative_long_halfword )
 UNDEF_INST( compare_logical_relative_long_long )
 UNDEF_INST( compare_logical_relative_long_long_fullword )
 UNDEF_INST( compare_logical_relative_long_long_halfword )
 UNDEF_INST( compare_relative_long )
 UNDEF_INST( compare_relative_long_long )
 UNDEF_INST( compare_relative_long_long_fullword )
 UNDEF_INST( extract_cpu_attribute )
 UNDEF_INST( load_address_extended_y )
 UNDEF_INST( load_and_test_long_fullword )
 UNDEF_INST( load_halfword_relative_long )
 UNDEF_INST( load_halfword_relative_long_long )
 UNDEF_INST( load_logical_halfword_relative_long )
 UNDEF_INST( load_logical_halfword_relative_long_long )
 UNDEF_INST( load_logical_relative_long_long_fullword )
 UNDEF_INST( load_relative_long )
 UNDEF_INST( load_relative_long_long )
 UNDEF_INST( load_relative_long_long_fullword )
 UNDEF_INST( move_fullword_from_halfword_immediate )
 UNDEF_INST( move_halfword_from_halfword_immediate )
 UNDEF_INST( move_long_from_halfword_immediate )
 UNDEF_INST( multiply_halfword_y )
 UNDEF_INST( multiply_single_immediate_fullword )
 UNDEF_INST( multiply_single_immediate_long_fullword )
 UNDEF_INST( multiply_y )
 UNDEF_INST( prefetch_data )
 UNDEF_INST( prefetch_data_relative_long )
 UNDEF_INST( rotate_then_and_selected_bits_long_reg )
 UNDEF_INST( rotate_then_exclusive_or_selected_bits_long_reg )
 UNDEF_INST( rotate_then_insert_selected_bits_long_reg )
 UNDEF_INST( rotate_then_or_selected_bits_long_reg )
 UNDEF_INST( store_halfword_relative_long )
 UNDEF_INST( store_relative_long )
 UNDEF_INST( store_relative_long_long )
#endif /*!defined( FEATURE_034_GEN_INST_EXTN_FACILITY )*/

#if !defined( FEATURE_035_EXECUTE_EXTN_FACILITY )
 UNDEF_INST( execute_relative_long )
#endif

#if !defined( FEATURE_037_FP_EXTENSION_FACILITY )
 UNDEF_INST( convert_bfp_ext_to_u32_reg )
 UNDEF_INST( convert_bfp_ext_to_u64_reg )
 UNDEF_INST( convert_bfp_long_to_u32_reg )
 UNDEF_INST( convert_bfp_long_to_u64_reg )
 UNDEF_INST( convert_bfp_short_to_u32_reg )
 UNDEF_INST( convert_bfp_short_to_u64_reg )
 UNDEF_INST( convert_dfp_ext_to_fix32_reg )
 UNDEF_INST( convert_dfp_ext_to_u32_reg )
 UNDEF_INST( convert_dfp_ext_to_u64_reg )
 UNDEF_INST( convert_dfp_long_to_fix32_reg )
 UNDEF_INST( convert_dfp_long_to_u32_reg )
 UNDEF_INST( convert_dfp_long_to_u64_reg )
 UNDEF_INST( convert_fix32_to_dfp_ext_reg )
 UNDEF_INST( convert_fix32_to_dfp_long_reg )
 UNDEF_INST( convert_u32_to_bfp_ext_reg )
 UNDEF_INST( convert_u32_to_bfp_long_reg )
 UNDEF_INST( convert_u32_to_bfp_short_reg )
 UNDEF_INST( convert_u32_to_dfp_ext_reg )
 UNDEF_INST( convert_u32_to_dfp_long_reg )
 UNDEF_INST( convert_u64_to_bfp_ext_reg )
 UNDEF_INST( convert_u64_to_bfp_long_reg )
 UNDEF_INST( convert_u64_to_bfp_short_reg )
 UNDEF_INST( convert_u64_to_dfp_ext_reg )
 UNDEF_INST( convert_u64_to_dfp_long_reg )
 UNDEF_INST( set_bfp_rounding_mode_3bit )
#endif /* !defined( FEATURE_037_FP_EXTENSION_FACILITY ) */

#if !defined( FEATURE_040_LOAD_PROG_PARAM_FACILITY )
 UNDEF_INST( load_program_parameter )
#endif

#if !defined( FEATURE_041_FPS_ENHANCEMENT_FACILITY )

#if !defined( FEATURE_041_DFP_ROUNDING_FACILITY )
 UNDEF_INST( set_dfp_rounding_mode )
#endif

#if !defined( FEATURE_041_FPR_GR_TRANSFER_FACILITY )
 UNDEF_INST( load_fpr_from_gr_long_reg )
 UNDEF_INST( load_gr_from_fpr_long_reg )
#endif

#if !defined( FEATURE_041_FPS_SIGN_HANDLING_FACILITY )
 UNDEF_INST( copy_sign_fpr_long_reg )
 UNDEF_INST( load_complement_fpr_long_reg )
 UNDEF_INST( load_negative_fpr_long_reg )
 UNDEF_INST( load_positive_fpr_long_reg )
#endif

#if !defined( FEATURE_041_IEEE_EXCEPT_SIM_FACILITY )
 UNDEF_INST( load_fpc_and_signal )
 UNDEF_INST( set_fpc_and_signal )
#endif

#endif /* !defined( FEATURE_041_FPS_ENHANCEMENT_FACILITY ) */

#if !defined( FEATURE_042_DFP_FACILITY )
 UNDEF_INST( add_dfp_ext_reg )
 UNDEF_INST( add_dfp_long_reg )
 UNDEF_INST( compare_dfp_ext_reg )
 UNDEF_INST( compare_dfp_long_reg )
 UNDEF_INST( compare_and_signal_dfp_ext_reg )
 UNDEF_INST( compare_and_signal_dfp_long_reg )
 UNDEF_INST( compare_exponent_dfp_ext_reg )
 UNDEF_INST( compare_exponent_dfp_long_reg )
 UNDEF_INST( convert_fix64_to_dfp_ext_reg )
 UNDEF_INST( convert_fix64_to_dfp_long_reg )
 UNDEF_INST( convert_sbcd128_to_dfp_ext_reg )
 UNDEF_INST( convert_sbcd64_to_dfp_long_reg )
 UNDEF_INST( convert_ubcd128_to_dfp_ext_reg )
 UNDEF_INST( convert_ubcd64_to_dfp_long_reg )
 UNDEF_INST( convert_dfp_ext_to_fix64_reg )
 UNDEF_INST( convert_dfp_long_to_fix64_reg )
 UNDEF_INST( convert_dfp_ext_to_sbcd128_reg )
 UNDEF_INST( convert_dfp_long_to_sbcd64_reg )
 UNDEF_INST( convert_dfp_ext_to_ubcd128_reg )
 UNDEF_INST( convert_dfp_long_to_ubcd64_reg )
 UNDEF_INST( divide_dfp_ext_reg )
 UNDEF_INST( divide_dfp_long_reg )
 UNDEF_INST( extract_biased_exponent_dfp_ext_to_fix64_reg )
 UNDEF_INST( extract_biased_exponent_dfp_long_to_fix64_reg )
 UNDEF_INST( extract_significance_dfp_ext_reg )
 UNDEF_INST( extract_significance_dfp_long_reg )
 UNDEF_INST( insert_biased_exponent_fix64_to_dfp_ext_reg )
 UNDEF_INST( insert_biased_exponent_fix64_to_dfp_long_reg )
 UNDEF_INST( load_and_test_dfp_ext_reg )
 UNDEF_INST( load_and_test_dfp_long_reg )
 UNDEF_INST( load_fp_int_dfp_ext_reg )
 UNDEF_INST( load_fp_int_dfp_long_reg )
 UNDEF_INST( load_lengthened_dfp_long_to_ext_reg )
 UNDEF_INST( load_lengthened_dfp_short_to_long_reg )
 UNDEF_INST( load_rounded_dfp_ext_to_long_reg )
 UNDEF_INST( load_rounded_dfp_long_to_short_reg )
 UNDEF_INST( multiply_dfp_ext_reg )
 UNDEF_INST( multiply_dfp_long_reg )
 UNDEF_INST( quantize_dfp_ext_reg )
 UNDEF_INST( quantize_dfp_long_reg )
 UNDEF_INST( reround_dfp_ext_reg )
 UNDEF_INST( reround_dfp_long_reg )
 UNDEF_INST( shift_coefficient_left_dfp_ext )
 UNDEF_INST( shift_coefficient_left_dfp_long )
 UNDEF_INST( shift_coefficient_right_dfp_ext )
 UNDEF_INST( shift_coefficient_right_dfp_long )
 UNDEF_INST( subtract_dfp_ext_reg )
 UNDEF_INST( subtract_dfp_long_reg )
 UNDEF_INST( test_data_class_dfp_ext )
 UNDEF_INST( test_data_class_dfp_long )
 UNDEF_INST( test_data_class_dfp_short )
 UNDEF_INST( test_data_group_dfp_ext )
 UNDEF_INST( test_data_group_dfp_long )
 UNDEF_INST( test_data_group_dfp_short )
#endif /*!defined( FEATURE_042_DFP_FACILITY )*/

#if !defined( FEATURE_044_PFPO_FACILITY )
 UNDEF_INST( perform_floating_point_operation )
#endif

#if !defined( FEATURE_045_DISTINCT_OPERANDS_FACILITY )
 UNDEF_INST( add_distinct_register )
 UNDEF_INST( add_distinct_long_register )
 UNDEF_INST( add_distinct_halfword_immediate )
 UNDEF_INST( add_distinct_long_halfword_immediate )
 UNDEF_INST( add_logical_distinct_register )
 UNDEF_INST( add_logical_distinct_long_register )
 UNDEF_INST( add_logical_distinct_signed_halfword_immediate )
 UNDEF_INST( add_logical_distinct_long_signed_halfword_immediate )
 UNDEF_INST( and_distinct_register )
 UNDEF_INST( and_distinct_long_register )
 UNDEF_INST( exclusive_or_distinct_register )
 UNDEF_INST( exclusive_or_distinct_long_register )
 UNDEF_INST( or_distinct_register )
 UNDEF_INST( or_distinct_long_register )
 UNDEF_INST( shift_left_single_distinct )
 UNDEF_INST( shift_left_single_logical_distinct )
 UNDEF_INST( shift_right_single_distinct )
 UNDEF_INST( shift_right_single_logical_distinct )
 UNDEF_INST( subtract_distinct_register )
 UNDEF_INST( subtract_distinct_long_register )
 UNDEF_INST( subtract_logical_distinct_register )
 UNDEF_INST( subtract_logical_distinct_long_register )
#endif /*!defined( FEATURE_045_DISTINCT_OPERANDS_FACILITY )*/

#if !defined( FEATURE_045_HIGH_WORD_FACILITY )
 UNDEF_INST( add_high_high_high_register )
 UNDEF_INST( add_high_high_low_register )
 UNDEF_INST( add_high_immediate )
 UNDEF_INST( add_logical_high_high_high_register )
 UNDEF_INST( add_logical_high_high_low_register )
 UNDEF_INST( add_logical_with_signed_immediate_high )
 UNDEF_INST( add_logical_with_signed_immediate_high_n )
 UNDEF_INST( branch_relative_on_count_high )
 UNDEF_INST( compare_high_high_register )
 UNDEF_INST( compare_high_low_register )
 UNDEF_INST( compare_high_fullword )
 UNDEF_INST( compare_high_immediate )
 UNDEF_INST( compare_logical_high_high_register )
 UNDEF_INST( compare_logical_high_low_register )
 UNDEF_INST( compare_logical_high_fullword )
 UNDEF_INST( compare_logical_high_immediate )
 UNDEF_INST( load_byte_high )
 UNDEF_INST( load_fullword_high )
 UNDEF_INST( load_halfword_high )
 UNDEF_INST( load_logical_character_high )
 UNDEF_INST( load_logical_halfword_high )
 UNDEF_INST( rotate_then_insert_selected_bits_high_long_reg )
 UNDEF_INST( rotate_then_insert_selected_bits_low_long_reg )
 UNDEF_INST( store_character_high )
 UNDEF_INST( store_fullword_high )
 UNDEF_INST( store_halfword_high )
 UNDEF_INST( subtract_high_high_high_register )
 UNDEF_INST( subtract_high_high_low_register )
 UNDEF_INST( subtract_logical_high_high_high_register )
 UNDEF_INST( subtract_logical_high_high_low_register )
#endif /*!defined( FEATURE_045_HIGH_WORD_FACILITY )*/

#if !defined( FEATURE_045_INTERLOCKED_ACCESS_FACILITY_1 )
 UNDEF_INST( load_and_add )
 UNDEF_INST( load_and_add_logical )
 UNDEF_INST( load_and_and )
 UNDEF_INST( load_and_exclusive_or )
 UNDEF_INST( load_and_or )
 UNDEF_INST( load_pair_disjoint )
 UNDEF_INST( load_and_add_logical_long )
 UNDEF_INST( load_and_add_long )
 UNDEF_INST( load_and_and_long )
 UNDEF_INST( load_and_exclusive_or_long )
 UNDEF_INST( load_and_or_long )
 UNDEF_INST( load_pair_disjoint_long )
#endif /*!defined( FEATURE_045_INTERLOCKED_ACCESS_FACILITY_1 )*/

#if !defined( FEATURE_045_LOAD_STORE_ON_COND_FACILITY_1 )
 UNDEF_INST( load_on_condition_register )
 UNDEF_INST( load_on_condition_long_register )
 UNDEF_INST( load_on_condition )
 UNDEF_INST( load_on_condition_long )
 UNDEF_INST( store_on_condition )
 UNDEF_INST( store_on_condition_long )
#endif

#if !defined( FEATURE_045_POPULATION_COUNT_FACILITY )
 UNDEF_INST( population_count )
#endif

#if !defined( FEATURE_048_DFP_ZONE_CONV_FACILITY )
 UNDEF_INST( convert_zoned_to_dfp_long )
 UNDEF_INST( convert_zoned_to_dfp_ext )
 UNDEF_INST( convert_dfp_long_to_zoned )
 UNDEF_INST( convert_dfp_ext_to_zoned )
#endif

#if !defined( FEATURE_049_EXECUTION_HINT_FACILITY )
 UNDEF_INST( branch_prediction_preload )
 UNDEF_INST( branch_prediction_relative_preload )
 UNDEF_INST( next_instruction_access_intent )
#endif

#if !defined( FEATURE_049_LOAD_AND_TRAP_FACILITY )
 UNDEF_INST( load_and_trap )
 UNDEF_INST( load_long_and_trap )
 UNDEF_INST( load_fullword_high_and_trap )
 UNDEF_INST( load_logical_long_fullword_and_trap )
 UNDEF_INST( load_logical_long_thirtyone_and_trap )
#endif

#if !defined( FEATURE_049_MISC_INSTR_EXT_FACILITY_1 )
 UNDEF_INST( compare_logical_and_trap )
 UNDEF_INST( compare_logical_and_trap_long )
 UNDEF_INST( rotate_then_insert_selected_bits_long_reg_n )
#endif

#if !defined( FEATURE_049_PROCESSOR_ASSIST_FACILITY )
 UNDEF_INST( perform_processor_assist );
#endif

#if !defined( FEATURE_050_CONSTR_TRANSACT_FACILITY )
 UNDEF_INST( transaction_begin_constrained )
#endif

#if !defined( FEATURE_053_LOAD_STORE_ON_COND_FACILITY_2 )
 UNDEF_INST( load_halfword_high_immediate_on_condition );
 UNDEF_INST( load_halfword_immediate_on_condition );
 UNDEF_INST( load_halfword_immediate_on_condition_grande );
 UNDEF_INST( load_high_on_condition );
 UNDEF_INST( load_high_on_condition_register );
 UNDEF_INST( store_high_on_condition );
#endif

#if !defined( FEATURE_053_LOAD_ZERO_RIGHTMOST_FACILITY )
 UNDEF_INST( load_and_zero_rightmost_byte_grande );
 UNDEF_INST( load_logical_and_zero_rightmost_byte );
 UNDEF_INST( load_and_zero_rightmost_byte );
#endif

#if !defined( FEATURE_058_MISC_INSTR_EXT_FACILITY_2 )
 UNDEF_INST( branch_indirect_on_condition )
 UNDEF_INST( add_long_halfword )
 UNDEF_INST( subtract_long_halfword )
 UNDEF_INST( multiply_long_register )
 UNDEF_INST( multiply_long )
 UNDEF_INST( multiply_long_halfword )
 UNDEF_INST( multiply_single_register_cc )
 UNDEF_INST( multiply_single_cc )
 UNDEF_INST( multiply_single_long_register_cc )
 UNDEF_INST( multiply_single_long_cc )
#endif

#if !defined( FEATURE_061_MISC_INSTR_EXT_FACILITY_3 )
 UNDEF_INST( and_register_with_complement )
 UNDEF_INST( and_register_long_with_complement )
 UNDEF_INST( nand_register )
 UNDEF_INST( nand_register_long )
 UNDEF_INST( not_xor_register )
 UNDEF_INST( not_xor_register_long )
 UNDEF_INST( nor_register )
 UNDEF_INST( nor_register_long )
 UNDEF_INST( or_register_with_complement )
 UNDEF_INST( or_register_long_with_complement )
 UNDEF_INST( select_register )
 UNDEF_INST( select_register_long )
 UNDEF_INST( select_fullword_high_register )
 UNDEF_INST( move_right_to_left )
#endif

#if !defined( FEATURE_066_RES_REF_BITS_MULT_FACILITY )
 UNDEF_INST( reset_reference_bits_multiple )
#endif

#if !defined( FEATURE_067_CPU_MEAS_COUNTER_FACILITY )
 UNDEF_INST( extract_coprocessor_group_address )
 UNDEF_INST( extract_cpu_counter )
 UNDEF_INST( extract_peripheral_counter )
 UNDEF_INST( load_cpu_counter_set_controls )
 UNDEF_INST( load_peripheral_counter_set_controls )
 UNDEF_INST( query_counter_information )
 UNDEF_INST( set_cpu_counter )
 UNDEF_INST( set_peripheral_counter )
#endif

#if !defined( FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY )
 UNDEF_INST( load_sampling_controls )
 UNDEF_INST( query_sampling_information )
#endif

#if !defined( FEATURE_073_TRANSACT_EXEC_FACILITY)
  UNDEF_INST( transaction_begin )
  UNDEF_INST( transaction_abort )
  UNDEF_INST( transaction_end )
  UNDEF_INST( nontransactional_store )
  UNDEF_INST( extract_transaction_nesting_depth )
#endif

#if !defined( FEATURE_074_STORE_HYPER_INFO_FACILITY )
 UNDEF_INST( store_hypervisor_information )
#endif

#if !defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 ) || defined( DYNINST_076_MSA_EXTENSION_FACILITY_3 )
 UNDEF_INST( perform_cryptographic_key_management_operation )
#endif

#if !defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 ) || defined( DYNINST_077_MSA_EXTENSION_FACILITY_4 )
 UNDEF_INST( cipher_message_with_cipher_feedback )
 UNDEF_INST( cipher_message_with_output_feedback )
 UNDEF_INST( cipher_message_with_counter )
 UNDEF_INST( perform_cryptographic_computation )
#endif

#if !defined( FEATURE_080_DFP_PACK_CONV_FACILITY )
 UNDEF_INST(convert_packed_to_dfp_long)
 UNDEF_INST(convert_packed_to_dfp_ext)
 UNDEF_INST(convert_dfp_long_to_packed)
 UNDEF_INST(convert_dfp_ext_to_packed)
#endif

#if !defined( FEATURE_145_INS_REF_BITS_MULT_FACILITY )
 UNDEF_INST( insert_reference_bits_multiple )
#endif

#if !defined( FEATURE_193_BEAR_ENH_FACILITY )
 UNDEF_INST( load_bear )
 UNDEF_INST( store_bear )
 UNDEF_INST( load_program_status_word_extended_y )
#endif

/*-------------------------------------------------------------------*/
/*      FEATUREs that DON'T have any facility bits defined           */
/*-------------------------------------------------------------------*/

#if !defined( FEATURE_ACCESS_REGISTERS )
 UNDEF_INST( copy_access )
 UNDEF_INST( extract_access_register )
 UNDEF_INST( load_access_multiple )
 UNDEF_INST( purge_accesslist_lookaside_buffer )
 UNDEF_INST( set_access_register )
 UNDEF_INST( store_access_multiple )
 UNDEF_INST( test_access )
#endif

#if !defined( FEATURE_BASIC_STORAGE_KEYS )
 UNDEF_INST( insert_storage_key )
 UNDEF_INST( reset_reference_bit )
 UNDEF_INST( set_storage_key )
#endif

#if !(defined( FEATURE_BIMODAL_ADDRESSING ) || defined( FEATURE_370_EXTENSION ))
 UNDEF_INST( branch_and_set_mode )
 UNDEF_INST( branch_and_save_and_set_mode )
#endif

#if !defined( FEATURE_BINARY_FLOATING_POINT )
 UNDEF_INST( extract_fpc )
 UNDEF_INST( load_fpc )
 UNDEF_INST( set_bfp_rounding_mode_2bit )
 UNDEF_INST( set_fpc )
 UNDEF_INST( store_fpc )
#endif

#if !defined( FEATURE_BINARY_FLOATING_POINT )
 UNDEF_INST( add_bfp_ext_reg )
 UNDEF_INST( add_bfp_long )
 UNDEF_INST( add_bfp_long_reg )
 UNDEF_INST( add_bfp_short )
 UNDEF_INST( add_bfp_short_reg )
 UNDEF_INST( compare_and_signal_bfp_ext_reg )
 UNDEF_INST( compare_and_signal_bfp_long )
 UNDEF_INST( compare_and_signal_bfp_long_reg )
 UNDEF_INST( compare_and_signal_bfp_short )
 UNDEF_INST( compare_and_signal_bfp_short_reg )
 UNDEF_INST( compare_bfp_ext_reg )
 UNDEF_INST( compare_bfp_long )
 UNDEF_INST( compare_bfp_long_reg )
 UNDEF_INST( compare_bfp_short )
 UNDEF_INST( compare_bfp_short_reg )
 UNDEF_INST( convert_bfp_ext_to_fix32_reg )
 UNDEF_INST( convert_bfp_long_to_fix32_reg )
 UNDEF_INST( convert_bfp_short_to_fix32_reg )
 UNDEF_INST( convert_fix32_to_bfp_ext_reg )
 UNDEF_INST( convert_fix32_to_bfp_long_reg )
 UNDEF_INST( convert_fix32_to_bfp_short_reg )
 UNDEF_INST( convert_fix64_to_bfp_ext_reg )
 UNDEF_INST( convert_fix64_to_bfp_long_reg )
 UNDEF_INST( convert_fix64_to_bfp_short_reg )
 UNDEF_INST( convert_bfp_ext_to_fix64_reg )
 UNDEF_INST( convert_bfp_long_to_fix64_reg )
 UNDEF_INST( convert_bfp_short_to_fix64_reg )
 UNDEF_INST( divide_bfp_ext_reg )
 UNDEF_INST( divide_bfp_long )
 UNDEF_INST( divide_bfp_long_reg )
 UNDEF_INST( divide_bfp_short )
 UNDEF_INST( divide_bfp_short_reg )
 UNDEF_INST( divide_integer_bfp_long_reg )
 UNDEF_INST( divide_integer_bfp_short_reg )
 UNDEF_INST( load_and_test_bfp_ext_reg )
 UNDEF_INST( load_and_test_bfp_long_reg )
 UNDEF_INST( load_and_test_bfp_short_reg )
 UNDEF_INST( load_fp_int_bfp_ext_reg )
 UNDEF_INST( load_fp_int_bfp_long_reg )
 UNDEF_INST( load_fp_int_bfp_short_reg )
 UNDEF_INST( load_complement_bfp_ext_reg )
 UNDEF_INST( load_complement_bfp_long_reg )
 UNDEF_INST( load_complement_bfp_short_reg )
 UNDEF_INST( load_negative_bfp_ext_reg )
 UNDEF_INST( load_negative_bfp_long_reg )
 UNDEF_INST( load_negative_bfp_short_reg )
 UNDEF_INST( load_positive_bfp_ext_reg )
 UNDEF_INST( load_positive_bfp_long_reg )
 UNDEF_INST( load_positive_bfp_short_reg )
 UNDEF_INST( load_lengthened_bfp_short_to_long )
 UNDEF_INST( load_lengthened_bfp_short_to_long_reg )
 UNDEF_INST( load_lengthened_bfp_long_to_ext )
 UNDEF_INST( load_lengthened_bfp_long_to_ext_reg )
 UNDEF_INST( load_lengthened_bfp_short_to_ext )
 UNDEF_INST( load_lengthened_bfp_short_to_ext_reg )
 UNDEF_INST( load_rounded_bfp_long_to_short_reg )
 UNDEF_INST( load_rounded_bfp_ext_to_long_reg )
 UNDEF_INST( load_rounded_bfp_ext_to_short_reg )
 UNDEF_INST( multiply_bfp_ext_reg )
 UNDEF_INST( multiply_bfp_long_to_ext_reg )
 UNDEF_INST( multiply_bfp_long_to_ext )
 UNDEF_INST( multiply_bfp_long )
 UNDEF_INST( multiply_bfp_long_reg )
 UNDEF_INST( multiply_bfp_short_to_long_reg )
 UNDEF_INST( multiply_bfp_short_to_long )
 UNDEF_INST( multiply_bfp_short )
 UNDEF_INST( multiply_bfp_short_reg )
 UNDEF_INST( multiply_add_bfp_long_reg )
 UNDEF_INST( multiply_add_bfp_long )
 UNDEF_INST( multiply_add_bfp_short_reg )
 UNDEF_INST( multiply_add_bfp_short )
 UNDEF_INST( multiply_subtract_bfp_long_reg )
 UNDEF_INST( multiply_subtract_bfp_long )
 UNDEF_INST( multiply_subtract_bfp_short_reg )
 UNDEF_INST( multiply_subtract_bfp_short )
 UNDEF_INST( squareroot_bfp_ext_reg )
 UNDEF_INST( squareroot_bfp_long )
 UNDEF_INST( squareroot_bfp_long_reg )
 UNDEF_INST( squareroot_bfp_short )
 UNDEF_INST( squareroot_bfp_short_reg )
 UNDEF_INST( subtract_bfp_ext_reg )
 UNDEF_INST( subtract_bfp_long )
 UNDEF_INST( subtract_bfp_long_reg )
 UNDEF_INST( subtract_bfp_short )
 UNDEF_INST( subtract_bfp_short_reg )
 UNDEF_INST( test_data_class_bfp_short )
 UNDEF_INST( test_data_class_bfp_long )
 UNDEF_INST( test_data_class_bfp_ext )
#endif /*!defined( FEATURE_BINARY_FLOATING_POINT )*/

#if !defined( FEATURE_BRANCH_AND_SET_AUTHORITY )
 UNDEF_INST( branch_and_set_authority )
#endif

#if !defined( FEATURE_BROADCASTED_PURGING )
 UNDEF_INST( compare_and_swap_and_purge )
#endif

#if !defined( FEATURE_CANCEL_IO_FACILITY )
 UNDEF_INST( cancel_subchannel )
#endif

#if !defined( FEATURE_CHANNEL_SUBSYSTEM )
 UNDEF_INST( clear_subchannel )
 UNDEF_INST( halt_subchannel )
 UNDEF_INST( modify_subchannel )
 UNDEF_INST( reset_channel_path )
 UNDEF_INST( resume_subchannel )
 UNDEF_INST( set_address_limit )
 UNDEF_INST( set_channel_monitor )
 UNDEF_INST( start_subchannel )
 UNDEF_INST( store_channel_path_status )
 UNDEF_INST( store_channel_report_word )
 UNDEF_INST( store_subchannel )
 UNDEF_INST( test_pending_interruption )
 UNDEF_INST( test_subchannel )
#endif /*!defined( FEATURE_CHANNEL_SUBSYSTEM )*/

#if !defined( FEATURE_CHANNEL_SWITCHING )
 UNDEF_INST( connect_channel_set )
 UNDEF_INST( disconnect_channel_set )
#endif

#if !defined( FEATURE_CHECKSUM_INSTRUCTION )
 UNDEF_INST( checksum )
#endif

#if !defined( FEATURE_CHSC )
 UNDEF_INST( channel_subsystem_call )
#endif

#if !defined( FEATURE_COMPARE_AND_MOVE_EXTENDED )
 UNDEF_INST( compare_logical_long_extended )
 UNDEF_INST( move_long_extended )
#endif

#if !defined( FEATURE_CMPSC )
 UNDEF_INST( cmpsc_2012 )
#endif

#if !defined( FEATURE_DAT_ENHANCEMENT_FACILITY_2 )
 UNDEF_INST( load_page_table_entry_address )
#endif

#if !defined( FEATURE_DUAL_ADDRESS_SPACE )
 UNDEF_INST( extract_primary_asn )
 UNDEF_INST( extract_secondary_asn )
 UNDEF_INST( insert_address_space_control )
 UNDEF_INST( insert_virtual_storage_key )
 UNDEF_INST( load_address_space_parameters )
 UNDEF_INST( move_to_primary )
 UNDEF_INST( move_to_secondary )
 UNDEF_INST( move_with_key )
 UNDEF_INST( program_call )
 UNDEF_INST( program_transfer )
 UNDEF_INST( set_address_space_control )
 UNDEF_INST( set_secondary_asn )
#endif /*!defined( FEATURE_DUAL_ADDRESS_SPACE )*/

#if !defined( FEATURE_ECPSVM )
 UNDEF_INST( ecpsvm_basic_freex )
 UNDEF_INST( ecpsvm_basic_fretx )
 UNDEF_INST( ecpsvm_comm_ccwproc )
 UNDEF_INST( ecpsvm_decode_first_ccw )
 UNDEF_INST( ecpsvm_decode_next_ccw )
 UNDEF_INST( ecpsvm_disp1 )
 UNDEF_INST( ecpsvm_disp2 )
 UNDEF_INST( ecpsvm_dispatch_main )
 UNDEF_INST( ecpsvm_extended_freex )
 UNDEF_INST( ecpsvm_extended_fretx )
 UNDEF_INST( ecpsvm_free_ccwstor )
 UNDEF_INST( ecpsvm_inval_ptable )
 UNDEF_INST( ecpsvm_inval_segtab )
 UNDEF_INST( ecpsvm_loc_chgshrpg )
 UNDEF_INST( ecpsvm_locate_rblock )
 UNDEF_INST( ecpsvm_locate_vblock )
 UNDEF_INST( ecpsvm_lock_page )
 UNDEF_INST( ecpsvm_prefmach_assist )
 UNDEF_INST( ecpsvm_store_level )
 UNDEF_INST( ecpsvm_tpage )
 UNDEF_INST( ecpsvm_tpage_lock )
 UNDEF_INST( ecpsvm_unlock_page )
 UNDEF_INST( ecpsvm_unxlate_ccw )
#endif /*!defined( FEATURE_ECPSVM )*/

#if !defined( FEATURE_EMULATE_VM )
 UNDEF_INST( inter_user_communication_vehicle )
#endif

#if !defined( FEATURE_EXPANDED_STORAGE )
 UNDEF_INST( page_in )
 UNDEF_INST( page_out )
#endif

#if !defined( FEATURE_EXTENDED_STORAGE_KEYS )
 UNDEF_INST( insert_storage_key_extended )
 UNDEF_INST( reset_reference_bit_extended )
 UNDEF_INST( set_storage_key_extended )
#endif

#if !defined( FEATURE_EXTENDED_TOD_CLOCK )
 UNDEF_INST( set_clock_programmable_field )
 UNDEF_INST( store_clock_extended )
#endif

#if !defined( FEATURE_EXTENDED_TRANSLATION_FACILITY_1 )
 UNDEF_INST( convert_utf8_to_utf16 )
 UNDEF_INST( convert_utf16_to_utf8 )
 UNDEF_INST( translate_extended )
#endif

#if !defined( FEATURE_FPS_EXTENSIONS )
 UNDEF_INST( convert_bfp_long_to_float_long_reg )
 UNDEF_INST( convert_bfp_short_to_float_long_reg )
 UNDEF_INST( convert_float_long_to_bfp_long_reg )
 UNDEF_INST( convert_float_long_to_bfp_short_reg )
 UNDEF_INST( load_float_ext_reg )
 UNDEF_INST( load_zero_float_ext_reg )
 UNDEF_INST( load_zero_float_long_reg )
 UNDEF_INST( load_zero_float_short_reg )
#endif /*!defined( FEATURE_FPS_EXTENSIONS )*/

#if !defined( FEATURE_HEXADECIMAL_FLOATING_POINT )
 UNDEF_INST( add_float_ext_reg )
 UNDEF_INST( add_float_long )
 UNDEF_INST( add_float_long_reg )
 UNDEF_INST( add_float_short )
 UNDEF_INST( add_float_short_reg )
 UNDEF_INST( add_unnormal_float_long )
 UNDEF_INST( add_unnormal_float_long_reg )
 UNDEF_INST( add_unnormal_float_short )
 UNDEF_INST( add_unnormal_float_short_reg )
 UNDEF_INST( compare_float_long )
 UNDEF_INST( compare_float_long_reg )
 UNDEF_INST( compare_float_short )
 UNDEF_INST( compare_float_short_reg )
 UNDEF_INST( divide_float_ext_reg )
 UNDEF_INST( divide_float_long )
 UNDEF_INST( divide_float_long_reg )
 UNDEF_INST( divide_float_short )
 UNDEF_INST( divide_float_short_reg )
 UNDEF_INST( halve_float_long_reg )
 UNDEF_INST( halve_float_short_reg )
 UNDEF_INST( load_and_test_float_long_reg )
 UNDEF_INST( load_and_test_float_short_reg )
 UNDEF_INST( load_complement_float_long_reg )
 UNDEF_INST( load_complement_float_short_reg )
 UNDEF_INST( load_float_long )
 UNDEF_INST( load_float_long_reg )
 UNDEF_INST( load_float_short )
 UNDEF_INST( load_float_short_reg )
 UNDEF_INST( load_negative_float_long_reg )
 UNDEF_INST( load_negative_float_short_reg )
 UNDEF_INST( load_positive_float_long_reg )
 UNDEF_INST( load_positive_float_short_reg )
 UNDEF_INST( load_rounded_float_long_reg )
 UNDEF_INST( load_rounded_float_short_reg )
 UNDEF_INST( multiply_float_ext_reg )
 UNDEF_INST( multiply_float_long )
 UNDEF_INST( multiply_float_long_reg )
 UNDEF_INST( multiply_float_long_to_ext )
 UNDEF_INST( multiply_float_long_to_ext_reg )
 UNDEF_INST( multiply_float_short_to_long )
 UNDEF_INST( multiply_float_short_to_long_reg )
 UNDEF_INST( store_float_long )
 UNDEF_INST( store_float_short )
 UNDEF_INST( subtract_float_ext_reg )
 UNDEF_INST( subtract_float_long )
 UNDEF_INST( subtract_float_long_reg )
 UNDEF_INST( subtract_float_short )
 UNDEF_INST( subtract_float_short_reg )
 UNDEF_INST( subtract_unnormal_float_long )
 UNDEF_INST( subtract_unnormal_float_long_reg )
 UNDEF_INST( subtract_unnormal_float_short )
 UNDEF_INST( subtract_unnormal_float_short_reg )
#endif /*!defined( FEATURE_HEXADECIMAL_FLOATING_POINT )*/

#if !defined( FEATURE_HFP_EXTENSIONS )
 UNDEF_INST( compare_float_ext_reg )
 UNDEF_INST( convert_fixed_to_float_ext_reg )
 UNDEF_INST( convert_fixed_to_float_long_reg )
 UNDEF_INST( convert_fixed_to_float_short_reg )
 UNDEF_INST( convert_float_ext_to_fixed_reg )
 UNDEF_INST( convert_float_long_to_fixed_reg )
 UNDEF_INST( convert_float_short_to_fixed_reg )
 UNDEF_INST( load_and_test_float_ext_reg )
 UNDEF_INST( load_complement_float_ext_reg )
 UNDEF_INST( load_fp_int_float_ext_reg )
 UNDEF_INST( load_fp_int_float_long_reg )
 UNDEF_INST( load_fp_int_float_short_reg )
 UNDEF_INST( load_lengthened_float_long_to_ext )
 UNDEF_INST( load_lengthened_float_long_to_ext_reg )
 UNDEF_INST( load_lengthened_float_short_to_ext )
 UNDEF_INST( load_lengthened_float_short_to_ext_reg )
 UNDEF_INST( load_lengthened_float_short_to_long )
 UNDEF_INST( load_lengthened_float_short_to_long_reg )
 UNDEF_INST( load_negative_float_ext_reg )
 UNDEF_INST( load_positive_float_ext_reg )
 UNDEF_INST( load_rounded_float_ext_to_short_reg )
 UNDEF_INST( multiply_float_short )
 UNDEF_INST( multiply_float_short_reg )
 UNDEF_INST( squareroot_float_ext_reg )
 UNDEF_INST( squareroot_float_long )
 UNDEF_INST( squareroot_float_short )
#endif /*!defined( FEATURE_HFP_EXTENSIONS )*/

#if !defined( FEATURE_IMMEDIATE_AND_RELATIVE )
 UNDEF_INST( add_halfword_immediate )
 UNDEF_INST( branch_relative_and_save )
 UNDEF_INST( branch_relative_on_condition )
 UNDEF_INST( branch_relative_on_count )
 UNDEF_INST( branch_relative_on_index_high )
 UNDEF_INST( branch_relative_on_index_low_or_equal )
 UNDEF_INST( compare_halfword_immediate )
 UNDEF_INST( load_halfword_immediate )
 UNDEF_INST( multiply_halfword_immediate )
 UNDEF_INST( multiply_single )
 UNDEF_INST( multiply_single_register )
 UNDEF_INST( test_under_mask_high )
 UNDEF_INST( test_under_mask_low )
#endif /*!defined( FEATURE_IMMEDIATE_AND_RELATIVE )*/

#if !defined( FEATURE_SIE )
 UNDEF_INST( start_interpretive_execution )
#endif

#if !defined( FEATURE_IO_ASSIST )
 UNDEF_INST( test_pending_zone_interrupt )
#endif

#if !defined( FEATURE_LINKAGE_STACK )
 UNDEF_INST( branch_and_stack )
 UNDEF_INST( extract_stacked_registers )
 UNDEF_INST( extract_stacked_state )
 UNDEF_INST( modify_stacked_state )
 UNDEF_INST( program_return )
 UNDEF_INST( trap2 )
 UNDEF_INST( trap4 )
#endif

#if !defined( FEATURE_LOCK_PAGE )
 UNDEF_INST( lock_page )
#endif

#if !defined( FEATURE_MOVE_PAGE_FACILITY_2 )
 UNDEF_INST( move_page )
 UNDEF_INST( invalidate_expanded_storage_block_entry )
#endif

#if !defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS ) // 'N' instructions
 UNDEF_INST( add_logical_carry_long )
 UNDEF_INST( add_logical_carry_long_register )
 UNDEF_INST( add_logical_long )
 UNDEF_INST( add_logical_long_fullword )
 UNDEF_INST( add_logical_long_fullword_register )
 UNDEF_INST( add_logical_long_register )
 UNDEF_INST( add_long )
 UNDEF_INST( add_long_fullword )
 UNDEF_INST( add_long_fullword_register )
 UNDEF_INST( add_long_halfword_immediate )
 UNDEF_INST( add_long_register )
 UNDEF_INST( and_immediate_high_high )
 UNDEF_INST( and_immediate_high_low )
 UNDEF_INST( and_immediate_low_high )
 UNDEF_INST( and_immediate_low_low )
 UNDEF_INST( and_long )
 UNDEF_INST( and_long_register )
 UNDEF_INST( branch_on_count_long )
 UNDEF_INST( branch_on_count_long_register )
 UNDEF_INST( branch_on_index_high_long )
 UNDEF_INST( branch_on_index_low_or_equal_long )
 UNDEF_INST( branch_relative_on_count_long )
 UNDEF_INST( branch_relative_on_index_high_long )
 UNDEF_INST( branch_relative_on_index_low_or_equal_long )
 UNDEF_INST( compare_and_swap_long )
 UNDEF_INST( compare_double_and_swap_long )
 UNDEF_INST( compare_logical_characters_under_mask_high )
 UNDEF_INST( compare_logical_long )
 UNDEF_INST( compare_logical_long_fullword )
 UNDEF_INST( compare_logical_long_fullword_register )
 UNDEF_INST( compare_logical_long_register )
 UNDEF_INST( compare_long )
 UNDEF_INST( compare_long_fullword )
 UNDEF_INST( compare_long_fullword_register )
 UNDEF_INST( compare_long_halfword_immediate )
 UNDEF_INST( compare_long_register )
 UNDEF_INST( convert_fix64_to_float_ext_reg )
 UNDEF_INST( convert_fix64_to_float_long_reg )
 UNDEF_INST( convert_fix64_to_float_short_reg )
 UNDEF_INST( convert_float_ext_to_fix64_reg )
 UNDEF_INST( convert_float_long_to_fix64_reg )
 UNDEF_INST( convert_float_short_to_fix64_reg )
 UNDEF_INST( convert_to_binary_long )
 UNDEF_INST( convert_to_decimal_long )
 UNDEF_INST( divide_logical_long )
 UNDEF_INST( divide_logical_long_register )
 UNDEF_INST( divide_single_long )
 UNDEF_INST( divide_single_long_fullword )
 UNDEF_INST( divide_single_long_fullword_register )
 UNDEF_INST( divide_single_long_register )
 UNDEF_INST( exclusive_or_long )
 UNDEF_INST( exclusive_or_long_register )
 UNDEF_INST( extract_and_set_extended_authority )
 UNDEF_INST( extract_stacked_registers_long )
 UNDEF_INST( insert_characters_under_mask_high )
 UNDEF_INST( insert_immediate_high_high )
 UNDEF_INST( insert_immediate_high_low )
 UNDEF_INST( insert_immediate_low_high )
 UNDEF_INST( insert_immediate_low_low )
 UNDEF_INST( load_and_test_long_fullword_register )
 UNDEF_INST( load_and_test_long_register )
 UNDEF_INST( load_complement_long_fullword_register )
 UNDEF_INST( load_complement_long_register )
 UNDEF_INST( load_control_long )
 UNDEF_INST( load_logical_immediate_high_high )
 UNDEF_INST( load_logical_immediate_high_low )
 UNDEF_INST( load_logical_immediate_low_high )
 UNDEF_INST( load_logical_immediate_low_low )
 UNDEF_INST( load_logical_long_character )
 UNDEF_INST( load_logical_long_fullword )
 UNDEF_INST( load_logical_long_fullword_register )
 UNDEF_INST( load_logical_long_halfword )
 UNDEF_INST( load_logical_long_thirtyone )
 UNDEF_INST( load_logical_long_thirtyone_register )
 UNDEF_INST( load_long )
 UNDEF_INST( load_long_fullword )
 UNDEF_INST( load_long_fullword_register )
 UNDEF_INST( load_long_halfword )
 UNDEF_INST( load_long_halfword_immediate )
 UNDEF_INST( load_long_register )
 UNDEF_INST( load_multiple_disjoint )
 UNDEF_INST( load_multiple_high )
 UNDEF_INST( load_multiple_long )
 UNDEF_INST( load_negative_long_fullword_register )
 UNDEF_INST( load_negative_long_register )
 UNDEF_INST( load_pair_from_quadword )
 UNDEF_INST( load_positive_long_fullword_register )
 UNDEF_INST( load_positive_long_register )
 UNDEF_INST( load_program_status_word_extended )
 UNDEF_INST( load_real_address_long )
 UNDEF_INST( load_reversed_long )
 UNDEF_INST( load_reversed_long_register )
 UNDEF_INST( load_using_real_address_long )
 UNDEF_INST( multiply_logical_long )
 UNDEF_INST( multiply_logical_long_register )
 UNDEF_INST( multiply_long_halfword_immediate )
 UNDEF_INST( multiply_single_long )
 UNDEF_INST( multiply_single_long_fullword )
 UNDEF_INST( multiply_single_long_fullword_register )
 UNDEF_INST( multiply_single_long_register )
 UNDEF_INST( or_immediate_high_high )
 UNDEF_INST( or_immediate_high_low )
 UNDEF_INST( or_immediate_low_high )
 UNDEF_INST( or_immediate_low_low )
 UNDEF_INST( or_long )
 UNDEF_INST( or_long_register )
 UNDEF_INST( rotate_left_single_logical_long )
 UNDEF_INST( set_addressing_mode_64 )
 UNDEF_INST( shift_left_single_logical_long )
 UNDEF_INST( shift_left_single_long )
 UNDEF_INST( shift_right_single_logical_long )
 UNDEF_INST( shift_right_single_long )
 UNDEF_INST( store_characters_under_mask_high )
 UNDEF_INST( store_control_long )
 UNDEF_INST( store_long )
 UNDEF_INST( store_multiple_high )
 UNDEF_INST( store_multiple_long )
 UNDEF_INST( store_pair_to_quadword )
 UNDEF_INST( store_real_address )
 UNDEF_INST( store_reversed_long )
 UNDEF_INST( store_using_real_address_long )
 UNDEF_INST( subtract_logical_borrow_long )
 UNDEF_INST( subtract_logical_borrow_long_register )
 UNDEF_INST( subtract_logical_long )
 UNDEF_INST( subtract_logical_long_fullword )
 UNDEF_INST( subtract_logical_long_fullword_register )
 UNDEF_INST( subtract_logical_long_register )
 UNDEF_INST( subtract_long )
 UNDEF_INST( subtract_long_fullword )
 UNDEF_INST( subtract_long_fullword_register )
 UNDEF_INST( subtract_long_register )
 UNDEF_INST( test_under_mask_high_high )
 UNDEF_INST( test_under_mask_high_low )
 UNDEF_INST( trace_long )
#endif /* !defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS ) */

#if !defined( FEATURE_PERFORM_LOCKED_OPERATION )
 UNDEF_INST( perform_locked_operation )
#endif

#if !defined( FEATURE_QEBSM )
 UNDEF_INST( set_queue_buffer_state )
 UNDEF_INST( extract_queue_buffer_state )
#endif

#if !defined( FEATURE_QUEUED_DIRECT_IO )
 UNDEF_INST( signal_adapter )
#endif

#if !defined( FEATURE_REGION_RELOCATE )
 UNDEF_INST( store_zone_parameter )
 UNDEF_INST( set_zone_parameter )
#endif

#if !defined( FEATURE_RESUME_PROGRAM )
 UNDEF_INST( resume_program )
#endif

#if !defined( FEATURE_S370_CHANNEL )
 UNDEF_INST( start_io )
 UNDEF_INST( test_io )
 UNDEF_INST( halt_io )
 UNDEF_INST( test_channel )
 UNDEF_INST( store_channel_id )
#endif

#if !defined( FEATURE_S370_S390_VECTOR_FACILITY )
 UNDEF_INST( v_and_to_vmr )
 UNDEF_INST( v_clear_vr )
 UNDEF_INST( v_complement_vmr )
 UNDEF_INST( v_count_left_zeros_in_vmr )
 UNDEF_INST( v_count_ones_in_vmr )
 UNDEF_INST( v_exclusive_or_to_vmr )
 UNDEF_INST( v_extract_vct )
 UNDEF_INST( v_extract_vector_modes )
 UNDEF_INST( v_load_vct_from_address )
 UNDEF_INST( v_load_vix_from_address )
 UNDEF_INST( v_load_vmr )
 UNDEF_INST( v_load_vmr_complement )
 UNDEF_INST( v_or_to_vmr )
 UNDEF_INST( v_restore_vac )
 UNDEF_INST( v_restore_vmr )
 UNDEF_INST( v_restore_vr )
 UNDEF_INST( v_restore_vsr )
 UNDEF_INST( v_save_changed_vr )
 UNDEF_INST( v_save_vac )
 UNDEF_INST( v_save_vmr )
 UNDEF_INST( v_save_vr )
 UNDEF_INST( v_save_vsr )
 UNDEF_INST( v_set_vector_mask_mode )
 UNDEF_INST( v_store_vector_parameters )
 UNDEF_INST( v_store_vmr )
 UNDEF_INST( v_test_vmr )
#else
  #if !defined( _GEN_ARCH )
    #if defined( _370 )
      #define _VEC_FAC_DEFINED_FOR_370
    #else
      #define _VEC_FAC_DEFINED_FOR_390
    #endif
  #else
    #if ( _GEN_ARCH == 390 )
      #define _VEC_FAC_DEFINED_FOR_390
    #endif
  #endif
#endif /* !defined( FEATURE_S370_S390_VECTOR_FACILITY ) */

#if !defined( FEATURE_SERVICE_PROCESSOR )
 UNDEF_INST( service_call )
#endif

#if !defined( FEATURE_SET_ADDRESS_SPACE_CONTROL_FAST )
 UNDEF_INST( set_address_space_control_fast )
#else
 #define s390_set_address_space_control_fast    s390_set_address_space_control
 #define z900_set_address_space_control_fast    z900_set_address_space_control
#endif

#if !defined( FEATURE_SQUARE_ROOT )
 UNDEF_INST( squareroot_float_long_reg )
 UNDEF_INST( squareroot_float_short_reg )
#endif

#if !defined( FEATURE_STORE_CPU_MULTIPLE_COUNTER_FACILITY )
 UNDEF_INST( store_cpu_counter_multiple )
#endif

#if !defined( FEATURE_STORE_SYSTEM_INFORMATION )
 UNDEF_INST( store_system_information )
#endif

#if !defined( FEATURE_STRING_INSTRUCTION )
 UNDEF_INST( compare_logical_string )
 UNDEF_INST( compare_until_substring_equal )
 UNDEF_INST( move_string )
 UNDEF_INST( search_string )
#endif

#if !defined( FEATURE_SUBSPACE_GROUP )
 UNDEF_INST( branch_in_subspace_group )
#endif

#if !defined( FEATURE_SVS )
 UNDEF_INST( set_vector_summary )
#endif

#if !defined( FEATURE_TCPIP_EXTENSION )
 UNDEF_INST( tcpip )
#endif

#if !defined( FEATURE_ZVM_ESSA )
 UNDEF_INST( extract_and_set_storage_attributes )
#endif

/*----------------------------------------------------------------------------*/
/*      Non-standard Extended Opcode intermediate jump "instructions"         */
/*----------------------------------------------------------------------------*/
/*                                                                            */
/* The following are actually "instruction functions" (note the "DEF_INST")   */
/* that the instruction dispatcher calls for "non-standard" extended opcode   */
/* instructions where the extended opcode is not in the first two bytes of    */
/* the instruction. Instead, the extended opcode is in the very last byte,    */
/* making it impossible for the dispatcher to directly jump to the actual     */
/* instruction.                                                               */
/*                                                                            */
/* It jumps instead to one of the below "instructions" which then jumps to    */
/* the ACTUAL instruction based on the extended opcode (which it must first   */
/* extract from the last byte of the instruction).                            */
/*                                                                            */
/* For example, if you look at the main "gen_opcode_table" for opcodes EB,    */
/* EC and ED, you will see the following:                                     */
/*                                                                            */
/*                                                                            */
/*     / EB /  GENx370x390x900 ( ..., execute_opcode_eb________xx ),          */
/*     / EC /  GENx370x390x900 ( ..., execute_opcode_ec________xx ),          */
/*     / ED /  GENx370x390x900 ( ..., execute_opcode_ed________xx ),          */
/*                                                                            */
/*                                                                            */
/* indicating that the 'execute_opcode_eb________xx' "instruction" should     */
/* be called for opcode EB.                                                   */
/*                                                                            */
/* Then if you look at the below 'execute_opcode_eb________xx' "instruction"  */
/* you can see that it simply indexes into our secondary jump table called    */
/* "runtime_opcode_eb________xx" (defined for the EB instructions) based on   */
/* the 6th byte of the instruction (inst[5]), to then perform a second jump   */
/* directly to the ACTUAL (true) instruction for that given extended opcode.  */
/*                                                                            */
/* Thankfully these routines can be optimized by the compiler to become an    */
/* indexed jump, leaving the stack frame untouched since the called routine   */
/* has the same arguments and the routine exits immediately after the call.   */
/* That is to say the below functions only add a SINGLE HOST INSTRUCTION to   */
/* to the actual instruction path in order to reach the actual instruction,   */
/* with that ONE extra host instruction being a simple indexed jump.          */
/*                                                                            */
/*----------------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* E3xx ???? - "instruction" to jump to actual instruction    [????] */
/*-------------------------------------------------------------------*/
DEF_INST( execute_opcode_e3________xx )
{
  regs->ARCH_DEP( runtime_opcode_e3________xx )[inst[5]](inst, regs);
}

#if defined( OPTION_OPTINST ) && !defined( OPTION_NO_E3_OPTINST )
/*-------------------------------------------------------------------*/
/* E3_0 ???? - "instruction" to jump to actual instruction    [????] */
/*-------------------------------------------------------------------*/
DEF_INST( E3_0 )
{
  regs->ARCH_DEP( runtime_opcode_e3_0______xx )[inst[5]](inst, regs);
}
#endif

/*-------------------------------------------------------------------*/
/* EBxx ???? - "instruction" to jump to actual instruction    [????] */
/*-------------------------------------------------------------------*/
DEF_INST( execute_opcode_eb________xx )
{
  regs->ARCH_DEP( runtime_opcode_eb________xx )[inst[5]](inst, regs);
}

/*-------------------------------------------------------------------*/
/* ECxx ???? - "instruction" to jump to actual instruction    [????] */
/*-------------------------------------------------------------------*/
DEF_INST( execute_opcode_ec________xx )
{
  regs->ARCH_DEP( runtime_opcode_ec________xx )[inst[5]](inst, regs);
}

/*-------------------------------------------------------------------*/
/* EDxx ???? - "instruction" to jump to actual instruction    [????] */
/*-------------------------------------------------------------------*/
DEF_INST( execute_opcode_ed________xx )
{
  regs->ARCH_DEP( runtime_opcode_ed________xx )[inst[5]](inst, regs);
}

/*-------------------------------------------------------------------*/
/* 00   ???? - Operation Exception "instruction"              [????] */
/*-------------------------------------------------------------------*/
DEF_INST( operation_exception )
{
    INST_UPDATE_PSW (regs, ILC(inst[0]), ILC(inst[0]));
    ARCH_DEP( program_interrupt )(regs, PGM_OPERATION_EXCEPTION);
}

/*-------------------------------------------------------------------*/
/*  (functions/declarations needed by below ARCH_DEP opcode table)   */
/*-------------------------------------------------------------------*/
#ifndef COMPILE_THIS_ONLY_ONCE
#define COMPILE_THIS_ONLY_ONCE

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP Operation Exception function used in opcode tables  */
/*-------------------------------------------------------------------*/
static void ( ATTR_REGPARM(2) operation_exception )( BYTE inst[], REGS* regs )
{
    INST_UPDATE_PSW( regs, ILC( inst[0] ), ILC( inst[0] ));
    regs->program_interrupt( regs, PGM_OPERATION_EXCEPTION );
}

/*-------------------------------------------------------------------*/
/*  Forward references to instruction tracing functions needed       */
/*  by the below ARCH_DEP master opcode table(s).                    */
/*-------------------------------------------------------------------*/
#define FWD_REF_IPRINT_FUNC( _asmfmt )  \
  static int iprint_ ## _asmfmt( REGS* regs, BYTE inst[], char mnemonic[], char* prtbuf )

FWD_REF_IPRINT_FUNC( ASMFMT_none );
FWD_REF_IPRINT_FUNC( ASMFMT_E );
FWD_REF_IPRINT_FUNC( ASMFMT_IE );
FWD_REF_IPRINT_FUNC( ASMFMT_MII_A );
FWD_REF_IPRINT_FUNC( ASMFMT_RR );
FWD_REF_IPRINT_FUNC( ASMFMT_RR_R1 );
FWD_REF_IPRINT_FUNC( ASMFMT_RR_SVC );
FWD_REF_IPRINT_FUNC( ASMFMT_RRE );
FWD_REF_IPRINT_FUNC( ASMFMT_RRE_R1 );
FWD_REF_IPRINT_FUNC( ASMFMT_RRF_R );
FWD_REF_IPRINT_FUNC( ASMFMT_RRF_M );
FWD_REF_IPRINT_FUNC( ASMFMT_RRF_M3 );
FWD_REF_IPRINT_FUNC( ASMFMT_RRF_M4 );
FWD_REF_IPRINT_FUNC( ASMFMT_RRF_MM );
FWD_REF_IPRINT_FUNC( ASMFMT_RRF_RM );
FWD_REF_IPRINT_FUNC( ASMFMT_RRR );
FWD_REF_IPRINT_FUNC( ASMFMT_RRF_A );
FWD_REF_IPRINT_FUNC( ASMFMT_RX );
FWD_REF_IPRINT_FUNC( ASMFMT_RXE );
FWD_REF_IPRINT_FUNC( ASMFMT_RXY );
FWD_REF_IPRINT_FUNC( ASMFMT_RXF );
FWD_REF_IPRINT_FUNC( ASMFMT_RS );
FWD_REF_IPRINT_FUNC( ASMFMT_RS_R1D2B2 );
FWD_REF_IPRINT_FUNC( ASMFMT_RSY );
FWD_REF_IPRINT_FUNC( ASMFMT_RSY_M3 );
FWD_REF_IPRINT_FUNC( ASMFMT_RSL );
FWD_REF_IPRINT_FUNC( ASMFMT_RSL_RM );
FWD_REF_IPRINT_FUNC( ASMFMT_RSI );
FWD_REF_IPRINT_FUNC( ASMFMT_RI );
FWD_REF_IPRINT_FUNC( ASMFMT_RI_B );
FWD_REF_IPRINT_FUNC( ASMFMT_RIE );
FWD_REF_IPRINT_FUNC( ASMFMT_RIE_G );
FWD_REF_IPRINT_FUNC( ASMFMT_RIE_RRI );
FWD_REF_IPRINT_FUNC( ASMFMT_RIE_RIM );
FWD_REF_IPRINT_FUNC( ASMFMT_RIE_RRIM );
FWD_REF_IPRINT_FUNC( ASMFMT_RIE_RMII );
FWD_REF_IPRINT_FUNC( ASMFMT_RIE_RRIII );
FWD_REF_IPRINT_FUNC( ASMFMT_RIL );
FWD_REF_IPRINT_FUNC( ASMFMT_RIL_A );
FWD_REF_IPRINT_FUNC( ASMFMT_RIS );
FWD_REF_IPRINT_FUNC( ASMFMT_RRS );
FWD_REF_IPRINT_FUNC( ASMFMT_SI );
FWD_REF_IPRINT_FUNC( ASMFMT_SIY );
FWD_REF_IPRINT_FUNC( ASMFMT_SIL );
FWD_REF_IPRINT_FUNC( ASMFMT_SMI_A );
FWD_REF_IPRINT_FUNC( ASMFMT_S );
FWD_REF_IPRINT_FUNC( ASMFMT_SS );
FWD_REF_IPRINT_FUNC( ASMFMT_SS_L );
FWD_REF_IPRINT_FUNC( ASMFMT_SS_L2 );
FWD_REF_IPRINT_FUNC( ASMFMT_SS_R );
FWD_REF_IPRINT_FUNC( ASMFMT_SS_R3 );
FWD_REF_IPRINT_FUNC( ASMFMT_SS_RSRS );
FWD_REF_IPRINT_FUNC( ASMFMT_SS_I );
FWD_REF_IPRINT_FUNC( ASMFMT_SSE );
FWD_REF_IPRINT_FUNC( ASMFMT_SSF );
FWD_REF_IPRINT_FUNC( ASMFMT_SSF_RSS );
FWD_REF_IPRINT_FUNC( ASMFMT_VS );

#endif // COMPILE_THIS_ONLY_ONCE

/*----------------------------------------------------------------------------*/
/*        *ARCH_DEP*  master opcode table(s)  (compiled multiple times)       */
/*----------------------------------------------------------------------------*/
static INSTR_FUNC ARCH_DEP( gen_opcode_b2xx )[256][NUM_INSTR_TAB_PTRS] =
{
#if __GEN_ARCH == 370
 /*B200*/ AD_GENx370x___x___ ( "CONCS"     , S    , ASMFMT_S       , connect_channel_set                     ),
 /*B201*/ AD_GENx370x___x___ ( "DISCS"     , S    , ASMFMT_S       , disconnect_channel_set                  ),
#elif __GEN_ARCH == 390
 /*B200*/ AD_GENx___x___x___ ,
 /*B201*/ AD_GENx___x___x___ ,
#else // __GEN_ARCH == 900
 /*B200*/ AD_GENx___x___x900 ( "LBEAR"     , S    , ASMFMT_S       , load_bear                               ),
 /*B201*/ AD_GENx___x___x900 ( "STBEAR"    , S    , ASMFMT_S       , store_bear                              ),
#endif
 /*B202*/ AD_GENx370x390x900 ( "STIDP"     , S    , ASMFMT_S       , store_cpu_id                            ),
 /*B203*/ AD_GENx370x___x___ ( "STIDC"     , S    , ASMFMT_S       , store_channel_id                        ),
 /*B204*/ AD_GENx370x390x900 ( "SCK"       , S    , ASMFMT_S       , set_clock                               ),
 /*B205*/ AD_GENx370x390x900 ( "STCK"      , S    , ASMFMT_S       , store_clock                             ),
 /*B206*/ AD_GENx370x390x900 ( "SCKC"      , S    , ASMFMT_S       , set_clock_comparator                    ),
 /*B207*/ AD_GENx370x390x900 ( "STCKC"     , S    , ASMFMT_S       , store_clock_comparator                  ),
 /*B208*/ AD_GENx370x390x900 ( "SPT"       , S    , ASMFMT_S       , set_cpu_timer                           ),
 /*B209*/ AD_GENx370x390x900 ( "STPT"      , S    , ASMFMT_S       , store_cpu_timer                         ),
 /*B20A*/ AD_GENx370x390x900 ( "SPKA"      , S    , ASMFMT_S       , set_psw_key_from_address                ),
 /*B20B*/ AD_GENx370x390x900 ( "IPK"       , S    , ASMFMT_none    , insert_psw_key                          ),
 /*B20C*/ AD_GENx___x___x___ ,
 /*B20D*/ AD_GENx370x390x900 ( "PTLB"      , S    , ASMFMT_none    , purge_translation_lookaside_buffer      ),
 /*B20E*/ AD_GENx___x___x___ ,
 /*B20F*/ AD_GENx___x___x___ ,
 /*B210*/ AD_GENx370x390x900 ( "SPX"       , S    , ASMFMT_S       , set_prefix                              ),
 /*B211*/ AD_GENx370x390x900 ( "STPX"      , S    , ASMFMT_S       , store_prefix                            ),
 /*B212*/ AD_GENx370x390x900 ( "STAP"      , S    , ASMFMT_S       , store_cpu_address                       ),
 /*B213*/ AD_GENx370x___x___ ( "RRB"       , S    , ASMFMT_S       , reset_reference_bit                     ),
 /*B214*/ AD_GENx___x390x900 ( "SIE"       , S    , ASMFMT_S       , start_interpretive_execution            ),
 /*B215*/ AD_GENx___x___x___ ,
 /*B216*/ AD_GENx___x___x___ , /*%SETR /SSYN  */
 /*B217*/ AD_GENx___x___x___ , /*%STETR/STSYN */
 /*B218*/ AD_GENx370x390x900 ( "PC"        , S    , ASMFMT_S       , program_call                            ),
 /*B219*/ AD_GENx370x390x900 ( "SAC"       , S    , ASMFMT_S       , set_address_space_control               ),
 /*B21A*/ AD_GENx37Xx390x900 ( "CFC"       , S    , ASMFMT_S       , compare_and_form_codeword               ),
 /*B21B*/ AD_GENx___x___x___ ,
 /*B21C*/ AD_GENx___x___x___ ,
 /*B21D*/ AD_GENx___x___x___ ,
 /*B21E*/ AD_GENx___x___x___ ,
 /*B21F*/ AD_GENx___x___x___ ,
 /*B220*/ AD_GENx___x390x900 ( "SERVC"     , RRE  , ASMFMT_RRE     , service_call                            ),
 /*B221*/ AD_GENx370x390x900 ( "IPTE"      , RRF_a, ASMFMT_RRR     , invalidate_page_table_entry             ),
 /*B222*/ AD_GENx370x390x900 ( "IPM"       , RRE  , ASMFMT_RRE_R1  , insert_program_mask                     ),
 /*B223*/ AD_GENx370x390x900 ( "IVSK"      , RRE  , ASMFMT_RRE     , insert_virtual_storage_key              ),
 /*B224*/ AD_GENx370x390x900 ( "IAC"       , RRE  , ASMFMT_RRE_R1  , insert_address_space_control            ),
 /*B225*/ AD_GENx370x390x900 ( "SSAR"      , RRE  , ASMFMT_RRE_R1  , set_secondary_asn                       ),
 /*B226*/ AD_GENx370x390x900 ( "EPAR"      , RRE  , ASMFMT_RRE_R1  , extract_primary_asn                     ),
 /*B227*/ AD_GENx370x390x900 ( "ESAR"      , RRE  , ASMFMT_RRE_R1  , extract_secondary_asn                   ),
 /*B228*/ AD_GENx370x390x900 ( "PT"        , RRE  , ASMFMT_RRE     , program_transfer                        ),
 /*B229*/ AD_GENx370x390x900 ( "ISKE"      , RRE  , ASMFMT_RRE     , insert_storage_key_extended             ),
 /*B22A*/ AD_GENx370x390x900 ( "RRBE"      , RRE  , ASMFMT_RRE     , reset_reference_bit_extended            ),
 /*B22B*/ AD_GENx370x390x900 ( "SSKE"      , RRF_c, ASMFMT_RRF_M   , set_storage_key_extended                ),
 /*B22C*/ AD_GENx370x390x900 ( "TB"        , RRE  , ASMFMT_RRE     , test_block                              ),
 /*B22D*/ AD_GENx370x390x900 ( "DXR"       , RRE  , ASMFMT_RRE     , divide_float_ext_reg                    ),
 /*B22E*/ AD_GENx___x390x900 ( "PGIN"      , RRE  , ASMFMT_RRE     , page_in                                 ),
 /*B22F*/ AD_GENx___x390x900 ( "PGOUT"     , RRE  , ASMFMT_RRE     , page_out                                ),
 /*B230*/ AD_GENx___x390x900 ( "CSCH"      , S    , ASMFMT_none    , clear_subchannel                        ),
 /*B231*/ AD_GENx___x390x900 ( "HSCH"      , S    , ASMFMT_none    , halt_subchannel                         ),
 /*B232*/ AD_GENx___x390x900 ( "MSCH"      , S    , ASMFMT_S       , modify_subchannel                       ),
 /*B233*/ AD_GENx___x390x900 ( "SSCH"      , S    , ASMFMT_S       , start_subchannel                        ),
 /*B234*/ AD_GENx___x390x900 ( "STSCH"     , S    , ASMFMT_S       , store_subchannel                        ),
 /*B235*/ AD_GENx___x390x900 ( "TSCH"      , S    , ASMFMT_S       , test_subchannel                         ),
 /*B236*/ AD_GENx___x390x900 ( "TPI"       , S    , ASMFMT_S       , test_pending_interruption               ),
 /*B237*/ AD_GENx___x390x900 ( "SAL"       , S    , ASMFMT_none    , set_address_limit                       ),
 /*B238*/ AD_GENx___x390x900 ( "RSCH"      , S    , ASMFMT_none    , resume_subchannel                       ),
 /*B239*/ AD_GENx___x390x900 ( "STCRW"     , S    , ASMFMT_S       , store_channel_report_word               ),
 /*B23A*/ AD_GENx___x390x900 ( "STCPS"     , S    , ASMFMT_S       , store_channel_path_status               ),
 /*B23B*/ AD_GENx___x390x900 ( "RCHP"      , S    , ASMFMT_none    , reset_channel_path                      ),
 /*B23C*/ AD_GENx___x390x900 ( "SCHM"      , S    , ASMFMT_none    , set_channel_monitor                     ),
 /*B23D*/ AD_GENx___x390x900 ( "STZP"      , S    , ASMFMT_S       , store_zone_parameter                    ),
 /*B23E*/ AD_GENx___x390x900 ( "SZP"       , S    , ASMFMT_S       , set_zone_parameter                      ),
 /*B23F*/ AD_GENx___x390x900 ( "TPZI"      , S    , ASMFMT_S       , test_pending_zone_interrupt             ),
 /*B240*/ AD_GENx___x390x900 ( "BAKR"      , RRE  , ASMFMT_RRE     , branch_and_stack                        ),
 /*B241*/ AD_GENx37Xx390x900 ( "CKSM"      , RRE  , ASMFMT_RRE     , checksum                                ),
 /*B242*/ AD_GENx370x___x___ ( "Assist"    , SSE  , ASMFMT_RRE     , add_frr                                             ),
 /*B243*/ AD_GENx___x___x___ , /*#MA      */
 /*B244*/ AD_GENx37Xx390x900 ( "SQDR"      , RRE  , ASMFMT_RRE     , squareroot_float_long_reg               ),
 /*B245*/ AD_GENx37Xx390x900 ( "SQER"      , RRE  , ASMFMT_RRE     , squareroot_float_short_reg              ),
 /*B246*/ AD_GENx___x390x900 ( "STURA"     , RRE  , ASMFMT_RRE     , store_using_real_address                ),
 /*B247*/ AD_GENx___x390x900 ( "MSTA"      , RRE  , ASMFMT_RRE_R1  , modify_stacked_state                    ),
 /*B248*/ AD_GENx___x390x900 ( "PALB"      , RRE  , ASMFMT_none    , purge_accesslist_lookaside_buffer       ),
 /*B249*/ AD_GENx___x390x900 ( "EREG"      , RRE  , ASMFMT_RRE     , extract_stacked_registers               ),
 /*B24A*/ AD_GENx___x390x900 ( "ESTA"      , RRE  , ASMFMT_RRE     , extract_stacked_state                   ),
 /*B24B*/ AD_GENx___x390x900 ( "LURA"      , RRE  , ASMFMT_RRE     , load_using_real_address                 ),
 /*B24C*/ AD_GENx___x390x900 ( "TAR"       , RRE  , ASMFMT_RRE     , test_access                             ),
 /*B24D*/ AD_GENx___x390x900 ( "CPYA"      , RRE  , ASMFMT_RRE     , copy_access                             ),
 /*B24E*/ AD_GENx___x390x900 ( "SAR"       , RRE  , ASMFMT_RRE     , set_access_register                     ),
 /*B24F*/ AD_GENx___x390x900 ( "EAR"       , RRE  , ASMFMT_RRE     , extract_access_register                 ),
 /*B250*/ AD_GENx___x390x900 ( "CSP"       , RRE  , ASMFMT_RRE     , compare_and_swap_and_purge              ),
 /*B251*/ AD_GENx___x___x___ ,
 /*B252*/ AD_GENx37Xx390x900 ( "MSR"       , RRE  , ASMFMT_RRE     , multiply_single_register                ),
 /*B253*/ AD_GENx___x___x___ ,
 /*B254*/ AD_GENx___x390x900 ( "MVPG"      , RRE  , ASMFMT_RRE     , move_page                               ),
 /*B255*/ AD_GENx37Xx390x900 ( "MVST"      , RRE  , ASMFMT_RRE     , move_string                             ),
 /*B256*/ AD_GENx___x___x900 ( "STHYI"     , RRE  , ASMFMT_RRE     , store_hypervisor_information            ),
 /*B257*/ AD_GENx37Xx390x900 ( "CUSE"      , RRE  , ASMFMT_RRE     , compare_until_substring_equal           ),
 /*B258*/ AD_GENx___x390x900 ( "BSG"       , RRE  , ASMFMT_RRE     , branch_in_subspace_group                ),
 /*B259*/ AD_GENx___x390x900 ( "IESBE"     , RRE  , ASMFMT_RRE     , invalidate_expanded_storage_block_entry ),
 /*B25A*/ AD_GENx___x390x900 ( "BSA"       , RRE  , ASMFMT_RRE     , branch_and_set_authority                ),
 /*B25B*/ AD_GENx___x___x___ , /*%PGXIN  */
 /*B25C*/ AD_GENx___x___x___ , /*%PGXOUT */
 /*B25D*/ AD_GENx37Xx390x900 ( "CLST"      , RRE  , ASMFMT_RRE     , compare_logical_string                  ),
 /*B25E*/ AD_GENx37Xx390x900 ( "SRST"      , RRE  , ASMFMT_RRE     , search_string                           ),
 /*B25F*/ AD_GENx___x390x900 ( "CHSC"      , RRE  , ASMFMT_RRE     , channel_subsystem_call                  ),
 /*B260*/ AD_GENx___x___x___ , /* Sysplex */
 /*B261*/ AD_GENx___x___x___ , /* Sysplex */
 /*B262*/ AD_GENx___x390x900 ( "LKPG"      , RRE  , ASMFMT_RRE     , lock_page                               ),
 /*B263*/ AD_GENx37Xx390x900 ( "CMPSC"     , RRE  , ASMFMT_RRE     , cmpsc_2012                              ),
 /*B264*/ AD_GENx___x___x___ , /* Sysplex */
 /*B265*/ AD_GENx___x___x900 ( "SVS"       , RRE  , ASMFMT_RRE     , set_vector_summary                      ),
 /*B266*/ AD_GENx___x___x___ , /* Sysplex */
 /*B267*/ AD_GENx___x___x___ , /* Sysplex */
 /*B268*/ AD_GENx___x___x___ , /* Sysplex */ /* DV - Define Vector */
 /*B269*/ AD_GENx___x___x___ , /* Crypto  */
 /*B26A*/ AD_GENx___x___x___ , /* Crypto  */
 /*B26B*/ AD_GENx___x___x___ , /* Crypto  */
 /*B26C*/ AD_GENx___x___x___ , /* Crypto  */
 /*B26D*/ AD_GENx___x___x___ , /* Crypto  */
 /*B26E*/ AD_GENx___x___x___ , /* Crypto  */
 /*B26F*/ AD_GENx___x___x___ , /* Crypto  */
 /*B270*/ AD_GENx___x___x___ , /*%SPCS    */
 /*B271*/ AD_GENx___x___x___ , /*%STPCS   */
 /*B272*/ AD_GENx___x___x___ , /* Sysplex */
 /*B273*/ AD_GENx___x___x___ ,
 /*B274*/ AD_GENx___x390x900 ( "SIGA"      , S    , ASMFMT_S       , signal_adapter                          ),
 /*B275*/ AD_GENx___x___x___ ,
 /*B276*/ AD_GENx___x390x900 ( "XSCH"      , S    , ASMFMT_none    , cancel_subchannel                       ),
 /*B277*/ AD_GENx___x390x900 ( "RP"        , S    , ASMFMT_S       , resume_program                          ),
 /*B278*/ AD_GENx___x390x900 ( "STCKE"     , S    , ASMFMT_S       , store_clock_extended                    ),
 /*B279*/ AD_GENx___x390x900 ( "SACF"      , S    , ASMFMT_S       , set_address_space_control               ),
 /*B27A*/ AD_GENx___x___x___ , /* Sysplex */
 /*B27B*/ AD_GENx___x___x___ , /* TFF/Sysplx */
 /*B27C*/ AD_GENx___x___x900 ( "STCKF"     , S    , ASMFMT_S       , store_clock                             ),
 /*B27D*/ AD_GENx370x390x900 ( "STSI"      , S    , ASMFMT_S       , store_system_information                ),
 /*B27E*/ AD_GENx___x___x___ , /* Sysplex */
 /*B27F*/ AD_GENx___x___x___ , /* Sysplex */
 /*B280*/ AD_GENx___x___x900 ( "LPP"       , S    , ASMFMT_S       , load_program_parameter                  ),
 /*B281*/ AD_GENx___x___x___ , /*#LN S  */
 /*B282*/ AD_GENx___x___x___ , /*#EXP L */
 /*B283*/ AD_GENx___x___x___ , /*#EXP S */
 /*B284*/ AD_GENx___x___x___ ,
 /*B285*/ AD_GENx___x___x___ ,
 /*B286*/ AD_GENx___x___x___ ,
 /*B287*/ AD_GENx___x___x___ ,
 /*B288*/ AD_GENx___x___x___ , /*#SIN L */
 /*B289*/ AD_GENx___x___x___ , /*#SIN S */
 /*B28A*/ AD_GENx___x___x___ , /*#COS L */
 /*B28B*/ AD_GENx___x___x___ , /*#COS S */
 /*B28C*/ AD_GENx___x___x___ ,
 /*B28D*/ AD_GENx___x___x___ ,
 /*B28E*/ AD_GENx___x___x___ ,
 /*B28F*/ AD_GENx___x___x___ ,
 /*B290*/ AD_GENx___x___x___ ,
 /*B291*/ AD_GENx___x___x___ ,
 /*B292*/ AD_GENx___x___x___ ,
 /*B293*/ AD_GENx___x___x___ ,
 /*B294*/ AD_GENx___x___x___ , /*#ARCTAN L */
 /*B295*/ AD_GENx___x___x___ , /*#ARCTAN S */
 /*B296*/ AD_GENx___x___x___ ,
 /*B297*/ AD_GENx___x___x___ ,
 /*B298*/ AD_GENx___x___x___ ,
 /*B299*/ AD_GENx37Xx390x900 ( "SRNM"      , S    , ASMFMT_S       , set_bfp_rounding_mode_2bit              ),
 /*B29A*/ AD_GENx___x___x___ ,
 /*B29B*/ AD_GENx___x___x___ ,
 /*B29C*/ AD_GENx37Xx390x900 ( "STFPC"     , S    , ASMFMT_S       , store_fpc                               ),
 /*B29D*/ AD_GENx37Xx390x900 ( "LFPC"      , S    , ASMFMT_S       , load_fpc                                ),
 /*B29E*/ AD_GENx___x___x___ ,
 /*B29F*/ AD_GENx___x___x___ ,
 /*B2A0*/ AD_GENx___x___x___ ,
 /*B2A1*/ AD_GENx___x___x___ ,
 /*B2A2*/ AD_GENx___x___x___ ,
 /*B2A3*/ AD_GENx___x___x___ ,
 /*B2A4*/ AD_GENx___x___x___ , /*Sysplex*/ /* MCBDM - Move Channel Buffer Data Multiple */
 /*B2A5*/ AD_GENx37Xx390x900 ( "TRE"       , RRE  , ASMFMT_RRE     , translate_extended                      ),
 /*B2A6*/ AD_GENx37Xx390x900 ( "CU21"      , RRF_c, ASMFMT_RRF_M3  , convert_utf16_to_utf8                   ),
 /*B2A7*/ AD_GENx37Xx390x900 ( "CU12"      , RRF_c, ASMFMT_RRF_M3  , convert_utf8_to_utf16                   ),
 /*B2A8*/ AD_GENx___x___x___ ,                                     /* Sysplex   */
 /*B2A9*/ AD_GENx___x___x___ ,
 /*B2AA*/ AD_GENx___x___x___ ,
 /*B2AB*/ AD_GENx___x___x___ ,
 /*B2AC*/ AD_GENx___x___x___ ,
 /*B2AD*/ AD_GENx___x___x___ ,
 /*B2AE*/ AD_GENx___x___x___ ,
 /*B2AF*/ AD_GENx___x___x___ ,
 /*B2B0*/ AD_GENx___x390x900 ( "STFLE"     , S    , ASMFMT_S       , store_facility_list_extended            ), /*!SARCH */
 /*B2B1*/ AD_GENx___x390x900 ( "STFL"      , S    , ASMFMT_S       , store_facility_list                     ),
 /*B2B2*/ AD_GENx___x___x900 ( "LPSWE"     , S    , ASMFMT_S       , load_program_status_word_extended       ),
 /*B2B3*/ AD_GENx___x___x___ , /* STEAI - Store ETR Attachment Information */
 /*B2B4*/ AD_GENx___x___x___ ,
 /*B2B5*/ AD_GENx___x___x___ ,
 /*B2B6*/ AD_GENx___x___x___ ,
 /*B2B7*/ AD_GENx___x___x___ ,
 /*B2B8*/ AD_GENx37Xx390x900 ( "SRNMB"     , S    , ASMFMT_S       , set_bfp_rounding_mode_3bit              ),
 /*B2B9*/ AD_GENx___x390x900 ( "SRNMT"     , S    , ASMFMT_S       , set_dfp_rounding_mode                   ),
 /*B2BA*/ AD_GENx___x___x___ ,
 /*B2BB*/ AD_GENx___x___x___ ,
 /*B2BC*/ AD_GENx___x___x___ ,
 /*B2BD*/ AD_GENx37Xx390x900 ( "LFAS"      , S    , ASMFMT_S       , load_fpc_and_signal                     ),
 /*B2BE*/ AD_GENx___x___x___ ,
 /*B2BF*/ AD_GENx___x___x___ ,
 /*B2C0*/ AD_GENx___x___x___ , /*$ADRN */
 /*B2C1*/ AD_GENx___x___x___ , /*$AERN */
 /*B2C2*/ AD_GENx___x___x___ , /*$SDRN */
 /*B2C3*/ AD_GENx___x___x___ , /*$SERN */
 /*B2C4*/ AD_GENx___x___x___ , /*$MDRN */
 /*B2C5*/ AD_GENx___x___x___ , /*$MERN */
 /*B2C6*/ AD_GENx___x___x___ , /*$DDRN */
 /*B2C7*/ AD_GENx___x___x___ , /*$DERN */
 /*B2C8*/ AD_GENx___x___x___ , /*$LERN */
 /*B2C9*/ AD_GENx___x___x___ ,
 /*B2CA*/ AD_GENx___x___x___ ,
 /*B2CB*/ AD_GENx___x___x___ ,
 /*B2CC*/ AD_GENx___x___x___ ,
 /*B2CD*/ AD_GENx___x___x___ ,
 /*B2CE*/ AD_GENx___x___x___ ,
 /*B2CF*/ AD_GENx___x___x___ ,
 /*B2D0*/ AD_GENx___x___x___ , /*$AACDR */
 /*B2D1*/ AD_GENx___x___x___ , /*$AACER */
 /*B2D2*/ AD_GENx___x___x___ , /*$SACDR */
 /*B2D3*/ AD_GENx___x___x___ , /*$SACER */
 /*B2D4*/ AD_GENx___x___x___ , /*$MACD  */
 /*B2D5*/ AD_GENx___x___x___ ,
 /*B2D6*/ AD_GENx___x___x___ , /*$RACD  */
 /*B2D7*/ AD_GENx___x___x___ , /*$RACE  */
 /*B2D8*/ AD_GENx___x___x___ , /*$AACAC */
 /*B2D9*/ AD_GENx___x___x___ , /*$SACAC */
 /*B2DA*/ AD_GENx___x___x___ , /*$CLAC  */
 /*B2DB*/ AD_GENx___x___x___ ,
 /*B2DC*/ AD_GENx___x___x___ ,
 /*B2DD*/ AD_GENx___x___x___ ,
 /*B2DE*/ AD_GENx___x___x___ ,
 /*B2DF*/ AD_GENx___x___x___ ,
 /*B2E0*/ AD_GENx___x___x___ ,
 /*B2E1*/ AD_GENx___x___x___ ,
 /*B2E2*/ AD_GENx___x___x___ ,
 /*B2E3*/ AD_GENx___x___x___ ,
 /*B2E4*/ AD_GENx___x___x___ ,
 /*B2E5*/ AD_GENx___x___x___ ,
 /*B2E6*/ AD_GENx___x___x___ ,
 /*B2E7*/ AD_GENx___x___x___ ,
 /*B2E8*/ AD_GENx___x___x900 ( "PPA"       , RRF_c, ASMFMT_RRF_M   , perform_processor_assist                ),
 /*B2E9*/ AD_GENx___x___x___ ,
 /*B2EA*/ AD_GENx___x___x___ ,
 /*B2EB*/ AD_GENx___x___x___ ,
 /*B2EC*/ AD_GENx___x___x900 ( "ETND"      , RRE  , ASMFMT_RRE     , extract_transaction_nesting_depth       ),
 /*B2ED*/ AD_GENx___x___x___ ,
 /*B2EE*/ AD_GENx___x___x___ ,
 /*B2EF*/ AD_GENx___x___x___ ,
 /*B2F0*/ AD_GENx370x390x900 ( "IUCV"      , S    , ASMFMT_S       , inter_user_communication_vehicle        ),
 /*B2F1*/ AD_GENx___x___x___ , /* Sysplex */
 /*B2F2*/ AD_GENx___x___x___ ,
 /*B2F3*/ AD_GENx___x___x___ ,
 /*B2F4*/ AD_GENx___x___x___ ,
 /*B2F5*/ AD_GENx___x___x___ ,
 /*B2F6*/ AD_GENx___x___x___ , /* Sysplex */
 /*B2F7*/ AD_GENx___x___x___ ,
 /*B2F8*/ AD_GENx___x___x900 ( "TEND"      , S    , ASMFMT_S       , transaction_end                         ) ,
 /*B2F9*/ AD_GENx___x___x___ ,
 /*B2FA*/ AD_GENx___x___x900 ( "NIAI"      , IE   , ASMFMT_IE      , next_instruction_access_intent          ),
 /*B2FB*/ AD_GENx___x___x___ ,
 /*B2FC*/ AD_GENx___x___x900 ( "TABORT"    , S    , ASMFMT_S       , transaction_abort                       ),
 /*B2FD*/ AD_GENx___x___x___ ,
 /*B2FE*/ AD_GENx___x___x___ ,
 /*B2FF*/ AD_GENx___x390x900 ( "TRAP4"     , S    , ASMFMT_S       , trap4                                   )
};

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "opcode.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "opcode.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section: compiled only ONCE after last arch built   */
/*-------------------------------------------------------------------*/
/*  Note: the last architecture has been built so the normal non-    */
/*  underscore FEATURE values are now #defined according to the      */
/*  LAST built architecture just built (usually zarch = 900). This   */
/*  means from this point onward (to the end of file) you should     */
/*  ONLY be testing the underscore _FEATURE values to see if the     */
/*  given feature was defined for *ANY* of the build architectures.  */
/*                                                                   */
/*  This is the code block where you would also place all of your    */
/*  non-ARCHDEP static functions and tables.                         */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*          "generated" opcode table forward references.             */
/*-------------------------------------------------------------------*/
/* The below tables are forward references to the tables that get    */
/* built by the compiler from our GENx370x390x900 et al. macros      */
/* further below. They need to be declared here so that the various  */
/* IPRINT_ROUT2 instruction tracing/printing routing functions       */
/* defined below can use them to jump to the proper print function.  */
/* (The IPRINT functions need to come BEFORE the GENx370x390x900     */
/* macros since the GENx370x390x900 tables contains a pointer to     */
/* the various iprint instruction tracing/printing functions.)       */
/*-------------------------------------------------------------------*/

static INSTR_FUNC gen_opcode_table[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_01xx[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_a5_x[16][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_a7_x[16][NUM_INSTR_TAB_PTRS];
//static INSTR_FUNC gen_opcode_b2xx[256][NUM_INSTR_TAB_PTRS]; // (ARCH_DEP)
static INSTR_FUNC gen_opcode_b3xx[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_b9xx[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_c0_x[16][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_c2_x[16][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_c4_x[16][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_c6_x[16][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_c8_x[16][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_cc_x[16][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_e3xx[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_e5xx[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_e6xx[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_ebxx[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_ecxx[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_edxx[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_v_opcode_a4xx[256][NUM_INSTR_TAB_PTRS];
#if defined( _FEATURE_S370_S390_VECTOR_FACILITY )
static INSTR_FUNC gen_v_opcode_a5xx[256][NUM_INSTR_TAB_PTRS];
#endif
static INSTR_FUNC gen_v_opcode_a6xx[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_v_opcode_e4xx[256][NUM_INSTR_TAB_PTRS];

#ifdef OPTION_OPTINST

static INSTR_FUNC gen_opcode_15__[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_18__[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_1E__[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_1F__[256][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_41_0[16][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_47_0[16][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_50_0[16][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_55_0[16][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_58_0[16][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_91xx[8][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_BF_x[3][NUM_INSTR_TAB_PTRS];

#if !defined( OPTION_NO_E3_OPTINST )

static INSTR_FUNC gen_opcode_E3_0[1][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_E3_0______04[1][NUM_INSTR_TAB_PTRS];
static INSTR_FUNC gen_opcode_E3_0______24[1][NUM_INSTR_TAB_PTRS];

#endif

#endif /* OPTION_OPTINST */

/*---------------------------------------------------------------------------*/
/* Following function will be resolved within the runtime opcode tables      */
/* and set with function init_runtime_opcode_tables.                         */
/*---------------------------------------------------------------------------*/
#define execute_opcode_01xx     operation_exception
#define execute_opcode_a4xx     operation_exception
#define execute_opcode_a5_x     operation_exception
#define execute_opcode_a6xx     operation_exception
#define execute_opcode_a7_x     operation_exception
#define execute_opcode_b2xx     operation_exception
#define execute_opcode_b3xx     operation_exception
#define execute_opcode_b9xx     operation_exception
#define execute_opcode_c0_x     operation_exception
#define execute_opcode_c2_x     operation_exception
#define execute_opcode_c4_x     operation_exception
#define execute_opcode_c6_x     operation_exception
#define execute_opcode_c8_x     operation_exception
#define execute_opcode_cc_x     operation_exception
#define execute_opcode_e4xx     operation_exception
#define execute_opcode_e5xx     operation_exception
#define execute_opcode_e6xx     operation_exception

/*----------------------------------------------------------------------------*/
/*                   Instruction Disassembly Functions                        */
/*                   (used during instruction tracing)                        */
/*----------------------------------------------------------------------------*/
/*    Note that the GENx370x390x900 instruction routing table entries         */
/*    much further below reference the below functions. Therefore all         */
/*    DISASM_xxx functions must come BEFORE the GENx370x390x900 tables.       */
/*----------------------------------------------------------------------------*/

typedef int IPRTFUNC(int arch_mode, BYTE inst[], char unused[], char *prtbuf);
                                        /* instruction printing function type */

/*----------------------------------------------------------------------------*/
/*      ROUTE_IPRINT  -  common instruction print routing logic               */
/*----------------------------------------------------------------------------*/
#define ROUTE_IPRINT( _arch_mode, _opcode_tabname, _opcode_idx )               \
                                                                               \
    IPRTFUNC*  iprt_asmfmt_func;  /* Ptr to instruction printing function */   \
    char*      iprt_info_string;  /* mnemonic + instruction function name */   \
                                                                               \
    UNREFERENCED( unused );                                                    \
                                                                               \
    /* Extract our parameters directly from the instruction opcode table */    \
                                                                               \
    iprt_asmfmt_func = (void*) gen_opcode_ ## _opcode_tabname [ inst _opcode_idx ][ NUM_INSTR_TAB_PTRS - 2 ];  \
    iprt_info_string = (void*) gen_opcode_ ## _opcode_tabname [ inst _opcode_idx ][ NUM_INSTR_TAB_PTRS - 1 ];  \
                                                                               \
    /* Trace this instruction */                                               \
    return iprt_asmfmt_func( (_arch_mode), inst, iprt_info_string, prtbuf );

/*----------------------------------------------------------------------------*/
/*                           iprint_router_func                               */
/*----------------------------------------------------------------------------*/
/* This is the primary instruction printing function called by instruction    */
/* tracing. It either calls the actual print function directly for the case   */
/* of single opcode instructions or jumps to one of the below IPRINT_ROUT2    */
/* functions to route the called based on the instruction's extended opcode.  */
/*----------------------------------------------------------------------------*/
DLL_EXPORT int iprint_router_func( int arch_mode, BYTE inst[], char unused[], char* prtbuf )
{
    ROUTE_IPRINT( arch_mode, table, [0] );   /* Route based on primary opcode [0] */
}
/*----------------------------------------------------------------------------*/
/*   IPRINT_ROUT2 - do second level jump to reach actual printing function    */
/*----------------------------------------------------------------------------*/

#define IPRINT_ROUT2( _opcode_tabname, _opcode_idx )                           \
                                                                               \
    int iprint_ASMFMT_ ## _opcode_tabname( int arch_mode, BYTE inst[], char unused[], char* prtbuf ) \
    {                                                                          \
        /* Route based on specified extended opcode */                         \
        ROUTE_IPRINT( arch_mode, _opcode_tabname, _opcode_idx );                \
    }

/*----------------------------------------------------------------------------*/
/*   Second level iprint jump function for ARCH_DEP( gen_opcode_b2xx ) table  */
/*----------------------------------------------------------------------------*/
int iprint_ASMFMT_b2xx( int arch_mode, BYTE inst[], char unused[], char* prtbuf )
{
    /* This function is designed to be identical to the code generated by
       the IPRINT_ROUT2 and ROUTE_IPRINT macros, but with additional code
       necessary to extract its pointers from the opcode table based on
       the current runtime architecture.
    */
    IPRTFUNC*  iprt_asmfmt_func = NULL;  /* Ptr to instr printing function */
    char*      iprt_info_string = NULL;  /* mnemonic + instr function name */

    UNREFERENCED( unused );

    /* Extract our parameters directly from the instruction opcode table */
    /* NOTE - we use hard coded 'inst[1]' since this is the 'b2xx' table */
    /* NOTE - we use a hard coded opcode table name for the same reason  */

    if (ARCH_370_IDX == arch_mode)
    {
        iprt_asmfmt_func = (void*) s370_gen_opcode_b2xx [ inst[1] ][ NUM_INSTR_TAB_PTRS - 2 ];
        iprt_info_string = (void*) s370_gen_opcode_b2xx [ inst[1] ][ NUM_INSTR_TAB_PTRS - 1 ];
    }
    else if (ARCH_390_IDX == arch_mode)
    {
        iprt_asmfmt_func = (void*) s390_gen_opcode_b2xx [ inst[1] ][ NUM_INSTR_TAB_PTRS - 2 ];
        iprt_info_string = (void*) s390_gen_opcode_b2xx [ inst[1] ][ NUM_INSTR_TAB_PTRS - 1 ];
    }
    else if (ARCH_900_IDX == arch_mode)
    {
        iprt_asmfmt_func = (void*) z900_gen_opcode_b2xx [ inst[1] ][ NUM_INSTR_TAB_PTRS - 2 ];
        iprt_info_string = (void*) z900_gen_opcode_b2xx [ inst[1] ][ NUM_INSTR_TAB_PTRS - 1 ];
    }
    else CRASH();

    /* Trace this instruction */
    return iprt_asmfmt_func( arch_mode, inst, iprt_info_string, prtbuf );
}

/*----------------------------------------------------------------------------*/
/*         The second level instruction printing routing functions            */
/*----------------------------------------------------------------------------*/

IPRINT_ROUT2( 01xx, [1] )
IPRINT_ROUT2( a5_x, [1] & 0x0F )
IPRINT_ROUT2( a7_x, [1] & 0x0F )
//IPRINT_ROUT2( b2xx, [1] )         // (see 'iprint_ASMFMT_b2xx' function)
IPRINT_ROUT2( b3xx, [1] )
IPRINT_ROUT2( b9xx, [1] )
IPRINT_ROUT2( c0_x, [1] & 0x0F )
IPRINT_ROUT2( c2_x, [1] & 0x0F )
IPRINT_ROUT2( c4_x, [1] & 0x0F )
IPRINT_ROUT2( c6_x, [1] & 0x0F )
IPRINT_ROUT2( c8_x, [1] & 0x0F )
IPRINT_ROUT2( cc_x, [1] & 0x0F )
IPRINT_ROUT2( e3xx, [5] )
IPRINT_ROUT2( e5xx, [1] )
IPRINT_ROUT2( e6xx, [1] )
IPRINT_ROUT2( ebxx, [5] )
IPRINT_ROUT2( ecxx, [5] )
IPRINT_ROUT2( edxx, [5] )

#if defined( _FEATURE_S370_S390_VECTOR_FACILITY )

 #define gen_opcode_a4xx            gen_v_opcode_a4xx
 IPRINT_ROUT2(  a4xx,[1] )
 #undef  gen_opcode_a4xx

 #define gen_opcode_a6xx            gen_v_opcode_a6xx
 IPRINT_ROUT2(  a6xx,[1] )
 #undef  gen_opcode_a6xx

 #define gen_opcode_e4xx            gen_v_opcode_e4xx
 IPRINT_ROUT2(  e4xx,[1] )
 #undef  gen_opcode_e4xx

#else /* !defined( _FEATURE_S370_S390_VECTOR_FACILITY ) */

 #define iprint_ASMFMT_a4xx    iprint_ASMFMT_none
 #define iprint_ASMFMT_a6xx    iprint_ASMFMT_none
 #define iprint_ASMFMT_e4xx    iprint_ASMFMT_none

#endif /* defined( _FEATURE_S370_S390_VECTOR_FACILITY ) */

/*----------------------------------------------------------------------------*/
/*   IPRINT_FUNC - instruction printing logic part 1:  function entry         */
/*----------------------------------------------------------------------------*/

#define IPRINT_FUNC( _asmfmt ) /* FIRST PART OF INSTRUCTION PRINT FUNCTION */ \
                                                                              \
static int iprint_ ## _asmfmt( REGS* regs, BYTE inst[], char mnemonic[], char* prtbuf )   \
{                                                                             \
    char* iname;               /* Pointer to instruction's function name  */  \
    char  opers[64]            /* Buffer to format its assembler operands */

/*----------------------------------------------------------------------------*/
/*   IPRINT_PRINT - instruction printing logic part 2:  print and return      */
/*----------------------------------------------------------------------------*/

#define IPRINT_PRINT(...)   /* LAST PART OF INSTRUCTION PRINT FUNCTION */     \
                                                                              \
    iname = mnemonic+1;     /* Start at 2nd character of mnemonic */          \
    while (*iname++);       /* Find start of instruction's name   */          \
                                                                              \
    /* Format the instruction's assembler operands */                         \
    MSGBUF (opers, ## __VA_ARGS__ );                                          \
                                                                              \
    /* Print assembler mnemonic + operands and full name of instruction */    \
    return sprintf( prtbuf, "%-5s %-19s    %s", mnemonic, opers, iname );     \
}

/*----------------------------------------------------------------------------*/
/*                       iprint_ASMFMT_xxxx                                   */
/*----------------------------------------------------------------------------*/
/*  The individual instruction printing functions themselves. These are the   */
/*  functions that are ultimately called during instruction tracing and are   */
/*  responsible for formatting the instruction trace statement according to   */
/*  the instruction's defined ASSEMBLER STATEMENT FORMAT (syntax), which is   */
/*  completely different from the instruction's actual machine format.        */
/*----------------------------------------------------------------------------*/

IPRINT_FUNC( ASMFMT_none );
    UNREFERENCED( regs );
    UNREFERENCED(inst);
    IPRINT_PRINT("%c",',')

IPRINT_FUNC( ASMFMT_E );
    UNREFERENCED( regs );
    UNREFERENCED(inst);
    IPRINT_PRINT("%c",',')

IPRINT_FUNC( ASMFMT_IE );
    int i1, i2;
    UNREFERENCED( regs );
    i1 = inst[3] >> 4;
    i2 = inst[3] & 0x0F;
    IPRINT_PRINT("%d,%d",i1,i2)

IPRINT_FUNC( ASMFMT_MII_A );
    int m1,i2,i3;
    const S64 Two_S64=2;
    UNREFERENCED( regs );
    m1 = inst[1] >> 4;
    i2 = (S32)(((U32)inst[1] << 8) | (U32)inst[2]);
    i3 = (S32)(((U32)inst[3] << 16) | ((U32)inst[4] << 8)
               | (U32)inst[5]);
    IPRINT_PRINT("%d,*%+"I64_FMT"d,*%+"I64_FMT"d",m1,i2*Two_S64,i3*Two_S64)

IPRINT_FUNC( ASMFMT_RR );
    int r1, r2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    r2 = inst[1] & 0x0F;
    IPRINT_PRINT("%d,%d",r1,r2)


// "Mnemonic   R1"
IPRINT_FUNC( ASMFMT_RR_R1 );
    int r1;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    IPRINT_PRINT("%d",r1)

IPRINT_FUNC( ASMFMT_RR_SVC );
    UNREFERENCED( regs );
    IPRINT_PRINT("%d",inst[1])

IPRINT_FUNC( ASMFMT_RRE );
    int r1, r2;
    UNREFERENCED( regs );
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    IPRINT_PRINT("%d,%d",r1,r2)

// "Mnemonic   R1"
IPRINT_FUNC( ASMFMT_RRE_R1 );
    int r1;
    UNREFERENCED( regs );
    r1 = inst[3] >> 4;
    IPRINT_PRINT("%d",r1)

IPRINT_FUNC( ASMFMT_RRF_R );
    int r1,r3,r2;
    UNREFERENCED( regs );
    r1 = inst[2] >> 4;
    r3 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    IPRINT_PRINT("%d,%d,%d",r1,r3,r2)

IPRINT_FUNC( ASMFMT_RRF_M );
    int m3,r1,r2;
    UNREFERENCED( regs );
    m3 = inst[2] >> 4;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    IPRINT_PRINT("%d,%d,%d",r1,m3,r2)

IPRINT_FUNC( ASMFMT_RRF_M3 );
    int m3,r1,r2;
    UNREFERENCED( regs );
    m3 = inst[2] >> 4;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    IPRINT_PRINT("%d,%d,%d",r1,r2,m3)

IPRINT_FUNC( ASMFMT_RRF_M4 );
    int m4,r1,r2;
    UNREFERENCED( regs );
    m4 = inst[2] & 0x0F;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    IPRINT_PRINT("%d,%d,%d",r1,r2,m4)

IPRINT_FUNC( ASMFMT_RRF_MM );
    int m3,m4,r1,r2;
    UNREFERENCED( regs );
    m3 = inst[2] >> 4;
    m4 = inst[2] & 0x0F;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    IPRINT_PRINT("%d,%d,%d,%d",r1,m3,r2,m4)

IPRINT_FUNC( ASMFMT_RRF_RM );
    int r3,m4,r1,r2;
    UNREFERENCED( regs );
    r3 = inst[2] >> 4;
    m4 = inst[2] & 0x0F;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    IPRINT_PRINT("%d,%d,%d,%d",r1,r3,r2,m4)

IPRINT_FUNC( ASMFMT_RRR );
    int r1,r2,r3;
    UNREFERENCED( regs );
    r3 = inst[2] >> 4;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    IPRINT_PRINT("%d,%d,%d",r1,r2,r3)

IPRINT_FUNC( ASMFMT_RRF_A );
    int r3,m4,r1,r2;
    UNREFERENCED( regs );
    r3 = inst[2] >> 4;
    m4 = inst[2] & 0x0F;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    IPRINT_PRINT("%d,%d,%d,%d",r1,r2,r3,m4)

IPRINT_FUNC( ASMFMT_RX );
    int r1,x2,b2,d2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    x2 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    IPRINT_PRINT("%d,%d(%d,%d)",r1,d2,x2,b2)

IPRINT_FUNC( ASMFMT_RXE );
    int r1,x2,b2,d2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    x2 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    IPRINT_PRINT("%d,%d(%d,%d)",r1,d2,x2,b2)

IPRINT_FUNC( ASMFMT_RXY );
    int r1,x2,b2,d2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    x2 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (((S8)inst[4]) << 12) | (inst[2] & 0x0F) << 8 | inst[3];
    IPRINT_PRINT("%d,%d(%d,%d)",r1,d2,x2,b2)

IPRINT_FUNC( ASMFMT_RXF );
    int r1,r3,x2,b2,d2;
    UNREFERENCED( regs );
    r1 = inst[4] >> 4;
    r3 = inst[1] >> 4;
    x2 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    IPRINT_PRINT("%d,%d,%d(%d,%d)",r1,r3,d2,x2,b2)

IPRINT_FUNC( ASMFMT_RS );
    int r1,r3,b2,d2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    IPRINT_PRINT("%d,%d,%d(%d)",r1,r3,d2,b2)

// "Mnemonic   R1,D2(B2)"
IPRINT_FUNC( ASMFMT_RS_R1D2B2 );
    int r1,b2,d2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    IPRINT_PRINT("%d,%d(%d)",r1,d2,b2)

IPRINT_FUNC( ASMFMT_RSY );
    int r1,r3,b2,d2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (((S8)inst[4]) << 12) | (inst[2] & 0x0F) << 8 | inst[3];
    IPRINT_PRINT("%d,%d,%d(%d)",r1,r3,d2,b2)

IPRINT_FUNC( ASMFMT_RSY_M3 );
    int r1,b2,d2,m3;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    m3 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (((S8)inst[4]) << 12) | (inst[2] & 0x0F) << 8 | inst[3];
    IPRINT_PRINT("%d,%d(%d),%d",r1,d2,b2,m3)

IPRINT_FUNC( ASMFMT_RSL );
    int l1,b1,d1;
    UNREFERENCED( regs );
    l1 = inst[1] >> 4;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    IPRINT_PRINT("%d(%d,%d)",d1,l1+1,b1)

IPRINT_FUNC( ASMFMT_RSL_RM );
    int r1,l2,b2,d2,m3;
    UNREFERENCED( regs );
    l2 = inst[1];
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    r1 = inst[4] >> 4;
    m3 = inst[4] & 0x0F;
    IPRINT_PRINT("%d,%d(%d,%d),%d",r1,d2,l2+1,b2,m3)

IPRINT_FUNC( ASMFMT_RSI );
    int r1,r3,i2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    i2 = (S16)(((U16)inst[2] << 8) | inst[3]);
    IPRINT_PRINT("%d,%d,*%+d",r1,r3,i2*2)

IPRINT_FUNC( ASMFMT_RI );
    int r1,i2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    i2 = (S16)(((U16)inst[2] << 8) | inst[3]);
    IPRINT_PRINT("%d,%d",r1,i2)

IPRINT_FUNC( ASMFMT_RI_B );
    int r1,i2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    i2 = (S16)(((U16)inst[2] << 8) | inst[3]);
    IPRINT_PRINT("%d,*%+d",r1,i2*2)

IPRINT_FUNC( ASMFMT_RIE );
    int r1,r3,i2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    i2 = (S16)(((U16)inst[2] << 8) | inst[3]);
    IPRINT_PRINT("%d,%d,*%+d",r1,r3,i2*2)

IPRINT_FUNC( ASMFMT_RIE_G );
    int r1,i2, m3;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    m3 = inst[1] & 0x0F;
    i2 = (S16)(((U16)inst[2] << 8) | inst[3]);
    IPRINT_PRINT("%d,%d,%d",r1,i2,m3)

IPRINT_FUNC( ASMFMT_RIE_RRI );
    int r1,r3,i2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    i2 = (S16)(((U16)inst[2] << 8) | inst[3]);
    IPRINT_PRINT("%d,%d,%d",r1,r3,i2)

IPRINT_FUNC( ASMFMT_RIE_RIM );
    int r1,i2,m3;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    i2 = (S16)(((U16)inst[2] << 8) | inst[3]);
    m3 = inst[4] >> 4;
    IPRINT_PRINT("%d,%d,%d",r1,i2,m3)

IPRINT_FUNC( ASMFMT_RIE_RRIM );
    int r1,r2,i4,m3;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    r2 = inst[1] & 0x0F;
    i4 = (S16)(((U16)inst[2] << 8) | inst[3]);
    m3 = inst[4] >> 4;
    IPRINT_PRINT("%d,%d,%d,*%+d",r1,r2,m3,i4*2)

IPRINT_FUNC( ASMFMT_RIE_RMII );
    int r1,m3,i4,i2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    m3 = inst[1] & 0x0F;
    i4 = (S16)(((U16)inst[2] << 8) | inst[3]);
    i2 = inst[4];
    IPRINT_PRINT("%d,%d,%d,*%+d",r1,i2,m3,i4*2)

IPRINT_FUNC( ASMFMT_RIE_RRIII );
    int r1,r2,i3,i4,i5;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    r2 = inst[1] & 0x0F;
    i3 = inst[2];
    i4 = inst[3];
    i5 = inst[4];
    IPRINT_PRINT("%d,%d,%d,%d,%d",r1,r2,i3,i4,i5)

IPRINT_FUNC( ASMFMT_RIL );
    int r1,i2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    i2 = (S32)((((U32)inst[2] << 24) | ((U32)inst[3] << 16)
       | ((U32)inst[4] << 8)) | inst[5]);
    IPRINT_PRINT("%d,%"PRId32,r1,i2)

IPRINT_FUNC( ASMFMT_RIL_A );
    int r1,i2;
    const S64 Two_S64=2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    i2 = (S32)((((U32)inst[2] << 24) | ((U32)inst[3] << 16)
       | ((U32)inst[4] << 8)) | inst[5]);
    IPRINT_PRINT("%d,*%+"PRId64,r1,i2*Two_S64)

IPRINT_FUNC( ASMFMT_RIS );
    int r1,i2,m3,b4,d4;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    m3 = inst[1] & 0x0F;
    b4 = inst[2] >> 4;
    d4 = (inst[2] & 0x0F) << 8 | inst[3];
    i2 = inst[4];
    IPRINT_PRINT("%d,%d,%d,%d(%d)",r1,i2,m3,d4,b4)

IPRINT_FUNC( ASMFMT_RRS );
    int r1,r2,m3,b4,d4;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    r2 = inst[1] & 0x0F;
    b4 = inst[2] >> 4;
    d4 = (inst[2] & 0x0F) << 8 | inst[3];
    m3 = inst[4] >> 4;
    IPRINT_PRINT("%d,%d,%d,%d(%d)",r1,r2,m3,d4,b4)

IPRINT_FUNC( ASMFMT_SI );
    int i2,b1,d1;
    UNREFERENCED( regs );
    i2 = inst[1];
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    IPRINT_PRINT("%d(%d),%d",d1,b1,i2)

IPRINT_FUNC( ASMFMT_SIY );
    int i2,b1,d1;
    UNREFERENCED( regs );
    i2 = inst[1];
    b1 = inst[2] >> 4;
    d1 = (((S8)inst[4]) << 12) | (inst[2] & 0x0F) << 8 | inst[3];
    IPRINT_PRINT("%d(%d),%d",d1,b1,i2)

IPRINT_FUNC( ASMFMT_SIL );
    int b1,d1,i2;
    UNREFERENCED( regs );
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    i2 = (S16)(((U16)inst[4] << 8) | inst[5]);
    IPRINT_PRINT("%d(%d),%d",d1,b1,i2)

IPRINT_FUNC( ASMFMT_SMI_A );
    int m1,i2,b3,d3;
    const S64 Two_S64=2;
    UNREFERENCED( regs );
    m1 = inst[1] >> 4;
    b3 = inst[2] >> 4;
    d3 = (inst[2] & 0x0F) << 8 | inst[3];
    i2 = (S32)(((U32)inst[4] << 8) | (U32)inst[5]);
    IPRINT_PRINT("%d,*%+"I64_FMT"d,%d(%d)",m1,i2*Two_S64,d3,b3)

IPRINT_FUNC( ASMFMT_S );
    int d2,b2;
    UNREFERENCED( regs );
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    IPRINT_PRINT("%d(%d)",d2,b2)

IPRINT_FUNC( ASMFMT_SS );
    int l1,l2,b1,d1,b2,d2;
    UNREFERENCED( regs );
    l1 = inst[1] >> 4;
    l2 = inst[1] & 0x0F;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    IPRINT_PRINT("%d(%d,%d),%d(%d,%d)",d1,l1+1,b1,d2,l2+1,b2)

IPRINT_FUNC( ASMFMT_SS_L );
    int l1,b1,d1,b2,d2;
    UNREFERENCED( regs );
    l1 = inst[1];
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    IPRINT_PRINT("%d(%d,%d),%d(%d)",d1,l1+1,b1,d2,b2)

// "Mnemonic   D1(B1),D2(L2,B2)"
IPRINT_FUNC( ASMFMT_SS_L2 );
    int l2,b1,d1,b2,d2;
    UNREFERENCED( regs );
    l2 = inst[1];
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    IPRINT_PRINT("%d(%d),%d(%d,%d)",d1,b1,d2,l2+1,b2)

IPRINT_FUNC( ASMFMT_SS_R );
    int r1,r3,b2,d2,b4,d4;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    b4 = inst[4] >> 4;
    d4 = (inst[4] & 0x0F) << 8 | inst[5];
    IPRINT_PRINT("%d,%d,%d(%d),%d(%d)",r1,r3,d2,b2,d4,b4)

// "Mnemonic   D1(R1,B1),D2(B2),R3"
IPRINT_FUNC( ASMFMT_SS_R3 );
    int r1,r3,b1,d1,b2,d2;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    IPRINT_PRINT("%d(%d,%d),%d(%d),%d",d1,r1,b1,d2,b2,r3)

// "Mnemonic   R1,D2(B2),R3,D4(B4)"
IPRINT_FUNC( ASMFMT_SS_RSRS );
    int r1,r3,b2,d2,b4,d4;
    UNREFERENCED( regs );
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    b4 = inst[4] >> 4;
    d4 = (inst[4] & 0x0F) << 8 | inst[5];
    IPRINT_PRINT("%d,%d(%d),%d,%d(%d)",r1,d2,b2,r3,d4,b4)

// "Mnemonic   D1(L1,B1),D2(B2),I3"
IPRINT_FUNC( ASMFMT_SS_I );
    int l1,i3,b1,d1,b2,d2;
    UNREFERENCED( regs );
    l1 = inst[1] >> 4;
    i3 = inst[1] & 0x0F;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    IPRINT_PRINT("%d(%d,%d),%d(%d),%d",d1,l1,b1,d2,b2,i3)

IPRINT_FUNC( ASMFMT_SSE );
    int b1,d1,b2,d2;
    UNREFERENCED( regs );
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    IPRINT_PRINT("%d(%d),%d(%d)",d1,b1,d2,b2)

IPRINT_FUNC( ASMFMT_SSF );
    int r3,b1,d1,b2,d2;
    UNREFERENCED( regs );
    r3 = inst[1] >> 4;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    IPRINT_PRINT("%d(%d),%d(%d),%d",d1,b1,d2,b2,r3)

IPRINT_FUNC( ASMFMT_SSF_RSS );
    int r3,b1,d1,b2,d2;
    UNREFERENCED( regs );
    r3 = inst[1] >> 4;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    IPRINT_PRINT("%d,%d(%d),%d(%d)",r3,d1,b1,d2,b2)

IPRINT_FUNC( ASMFMT_VS );
    int rs2;
    UNREFERENCED( regs );
    rs2 = inst[3] & 0x0F;
    IPRINT_PRINT("%d",rs2)

/*----------------------------------------------------------------------------*/
/*          'GENx___x___x900' instruction opcode jump tables                  */
/*----------------------------------------------------------------------------*/
/* PROGRAMMING NOTE: second argument in the below "GENx" macros ('_ifmt') is  */
/* currently not being used for anything at the moment. At some point in the  */
/* future however, it will be used to define a pointer to the instruction de- */
/* coding function called to decode the instruction before it is dispatched.  */
/*----------------------------------------------------------------------------*/
static INSTR_FUNC gen_opcode_table[256][NUM_INSTR_TAB_PTRS] =
{
 /*00*/   GENx___x___x___ ,
 /*01*/   GENx370x390x900 ( ""          , E    , ASMFMT_01xx     , execute_opcode_01xx                                 ),
 /*02*/   GENx___x___x___ ,
 /*03*/   GENx___x___x___ ,
 /*04*/   GENx370x390x900 ( "SPM"       , RR   , ASMFMT_RR_R1    , set_program_mask                                    ),
 /*05*/   GENx370x390x900 ( "BALR"      , RR   , ASMFMT_RR       , branch_and_link_register                            ),
 /*06*/   GENx370x390x900 ( "BCTR"      , RR   , ASMFMT_RR       , branch_on_count_register                            ),
 /*07*/   GENx370x390x900 ( "BCR"       , RR   , ASMFMT_RR       , branch_on_condition_register                        ),
 /*08*/   GENx370x___x___ ( "SSK"       , RR   , ASMFMT_RR       , set_storage_key                                     ),
 /*09*/   GENx370x___x___ ( "ISK"       , RR   , ASMFMT_RR       , insert_storage_key                                  ),
 /*0A*/   GENx370x390x900 ( "SVC"       , I    , ASMFMT_RR_SVC   , supervisor_call                                     ),
 /*0B*/   GENx37Xx390x900 ( "BSM"       , RR   , ASMFMT_RR       , branch_and_set_mode                                 ),
 /*0C*/   GENx37Xx390x900 ( "BASSM"     , RR   , ASMFMT_RR       , branch_and_save_and_set_mode                        ),
 /*0D*/   GENx370x390x900 ( "BASR"      , RR   , ASMFMT_RR       , branch_and_save_register                            ),
 /*0E*/   GENx370x390x900 ( "MVCL"      , RR   , ASMFMT_RR       , move_long                                           ),
 /*0F*/   GENx370x390x900 ( "CLCL"      , RR   , ASMFMT_RR       , compare_logical_character_long                      ),
 /*10*/   GENx370x390x900 ( "LPR"       , RR   , ASMFMT_RR       , load_positive_register                              ),
 /*11*/   GENx370x390x900 ( "LNR"       , RR   , ASMFMT_RR       , load_negative_register                              ),
 /*12*/   GENx370x390x900 ( "LTR"       , RR   , ASMFMT_RR       , load_and_test_register                              ),
 /*13*/   GENx370x390x900 ( "LCR"       , RR   , ASMFMT_RR       , load_complement_register                            ),
 /*14*/   GENx370x390x900 ( "NR"        , RR   , ASMFMT_RR       , and_register                                        ),
 /*15*/   GENx370x390x900 ( "CLR"       , RR   , ASMFMT_RR       , compare_logical_register                            ),
 /*16*/   GENx370x390x900 ( "OR"        , RR   , ASMFMT_RR       , or_register                                         ),
 /*17*/   GENx370x390x900 ( "XR"        , RR   , ASMFMT_RR       , exclusive_or_register                               ),
 /*18*/   GENx370x390x900 ( "LR"        , RR   , ASMFMT_RR       , load_register                                       ),
 /*19*/   GENx370x390x900 ( "CR"        , RR   , ASMFMT_RR       , compare_register                                    ),
 /*1A*/   GENx370x390x900 ( "AR"        , RR   , ASMFMT_RR       , add_register                                        ),
 /*1B*/   GENx370x390x900 ( "SR"        , RR   , ASMFMT_RR       , subtract_register                                   ),
 /*1C*/   GENx370x390x900 ( "MR"        , RR   , ASMFMT_RR       , multiply_register                                   ),
 /*1D*/   GENx370x390x900 ( "DR"        , RR   , ASMFMT_RR       , divide_register                                     ),
 /*1E*/   GENx370x390x900 ( "ALR"       , RR   , ASMFMT_RR       , add_logical_register                                ),
 /*1F*/   GENx370x390x900 ( "SLR"       , RR   , ASMFMT_RR       , subtract_logical_register                           ),
 /*20*/   GENx370x390x900 ( "LPDR"      , RR   , ASMFMT_RR       , load_positive_float_long_reg                        ),
 /*21*/   GENx370x390x900 ( "LNDR"      , RR   , ASMFMT_RR       , load_negative_float_long_reg                        ),
 /*22*/   GENx370x390x900 ( "LTDR"      , RR   , ASMFMT_RR       , load_and_test_float_long_reg                        ),
 /*23*/   GENx370x390x900 ( "LCDR"      , RR   , ASMFMT_RR       , load_complement_float_long_reg                      ),
 /*24*/   GENx370x390x900 ( "HDR"       , RR   , ASMFMT_RR       , halve_float_long_reg                                ),
 /*25*/   GENx370x390x900 ( "LDXR"      , RR   , ASMFMT_RR       , load_rounded_float_long_reg                         ),
 /*26*/   GENx370x390x900 ( "MXR"       , RR   , ASMFMT_RR       , multiply_float_ext_reg                              ),
 /*27*/   GENx370x390x900 ( "MXDR"      , RR   , ASMFMT_RR       , multiply_float_long_to_ext_reg                      ),
 /*28*/   GENx370x390x900 ( "LDR"       , RR   , ASMFMT_RR       , load_float_long_reg                                 ),
 /*29*/   GENx370x390x900 ( "CDR"       , RR   , ASMFMT_RR       , compare_float_long_reg                              ),
 /*2A*/   GENx370x390x900 ( "ADR"       , RR   , ASMFMT_RR       , add_float_long_reg                                  ),
 /*2B*/   GENx370x390x900 ( "SDR"       , RR   , ASMFMT_RR       , subtract_float_long_reg                             ),
 /*2C*/   GENx370x390x900 ( "MDR"       , RR   , ASMFMT_RR       , multiply_float_long_reg                             ),
 /*2D*/   GENx370x390x900 ( "DDR"       , RR   , ASMFMT_RR       , divide_float_long_reg                               ),
 /*2E*/   GENx370x390x900 ( "AWR"       , RR   , ASMFMT_RR       , add_unnormal_float_long_reg                         ),
 /*2F*/   GENx370x390x900 ( "SWR"       , RR   , ASMFMT_RR       , subtract_unnormal_float_long_reg                    ),
 /*30*/   GENx370x390x900 ( "LPER"      , RR   , ASMFMT_RR       , load_positive_float_short_reg                       ),
 /*31*/   GENx370x390x900 ( "LNER"      , RR   , ASMFMT_RR       , load_negative_float_short_reg                       ),
 /*32*/   GENx370x390x900 ( "LTER"      , RR   , ASMFMT_RR       , load_and_test_float_short_reg                       ),
 /*33*/   GENx370x390x900 ( "LCER"      , RR   , ASMFMT_RR       , load_complement_float_short_reg                     ),
 /*34*/   GENx370x390x900 ( "HER"       , RR   , ASMFMT_RR       , halve_float_short_reg                               ),
 /*35*/   GENx370x390x900 ( "LEDR"      , RR   , ASMFMT_RR       , load_rounded_float_short_reg                        ),
 /*36*/   GENx370x390x900 ( "AXR"       , RR   , ASMFMT_RR       , add_float_ext_reg                                   ),
 /*37*/   GENx370x390x900 ( "SXR"       , RR   , ASMFMT_RR       , subtract_float_ext_reg                              ),
 /*38*/   GENx370x390x900 ( "LER"       , RR   , ASMFMT_RR       , load_float_short_reg                                ),
 /*39*/   GENx370x390x900 ( "CER"       , RR   , ASMFMT_RR       , compare_float_short_reg                             ),
 /*3A*/   GENx370x390x900 ( "AER"       , RR   , ASMFMT_RR       , add_float_short_reg                                 ),
 /*3B*/   GENx370x390x900 ( "SER"       , RR   , ASMFMT_RR       , subtract_float_short_reg                            ),
 /*3C*/   GENx370x390x900 ( "MDER"      , RR   , ASMFMT_RR       , multiply_float_short_to_long_reg                    ),
 /*3D*/   GENx370x390x900 ( "DER"       , RR   , ASMFMT_RR       , divide_float_short_reg                              ),
 /*3E*/   GENx370x390x900 ( "AUR"       , RR   , ASMFMT_RR       , add_unnormal_float_short_reg                        ),
 /*3F*/   GENx370x390x900 ( "SUR"       , RR   , ASMFMT_RR       , subtract_unnormal_float_short_reg                   ),
 /*40*/   GENx370x390x900 ( "STH"       , RX_a , ASMFMT_RX       , store_halfword                                      ),
 /*41*/   GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , load_address                                        ),
 /*42*/   GENx370x390x900 ( "STC"       , RX_a , ASMFMT_RX       , store_character                                     ),
 /*43*/   GENx370x390x900 ( "IC"        , RX_a , ASMFMT_RX       , insert_character                                    ),
 /*44*/   GENx370x390x900 ( "EX"        , RX_a , ASMFMT_RX       , execute                                             ),
 /*45*/   GENx370x390x900 ( "BAL"       , RX_a , ASMFMT_RX       , branch_and_link                                     ),
 /*46*/   GENx370x390x900 ( "BCT"       , RX_a , ASMFMT_RX       , branch_on_count                                     ),
 /*47*/   GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , branch_on_condition                                 ),
 /*48*/   GENx370x390x900 ( "LH"        , RX_a , ASMFMT_RX       , load_halfword                                       ),
 /*49*/   GENx370x390x900 ( "CH"        , RX_a , ASMFMT_RX       , compare_halfword                                    ),
 /*4A*/   GENx370x390x900 ( "AH"        , RX_a , ASMFMT_RX       , add_halfword                                        ),
 /*4B*/   GENx370x390x900 ( "SH"        , RX_a , ASMFMT_RX       , subtract_halfword                                   ),
 /*4C*/   GENx370x390x900 ( "MH"        , RX_a , ASMFMT_RX       , multiply_halfword                                   ),
 /*4D*/   GENx370x390x900 ( "BAS"       , RX_a , ASMFMT_RX       , branch_and_save                                     ),
 /*4E*/   GENx370x390x900 ( "CVD"       , RX_a , ASMFMT_RX       , convert_to_decimal                                  ),
 /*4F*/   GENx370x390x900 ( "CVB"       , RX_a , ASMFMT_RX       , convert_to_binary                                   ),
 /*50*/   GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , store                                               ),
 /*51*/   GENx___x390x900 ( "LAE"       , RX_a , ASMFMT_RX       , load_address_extended                               ),
 /*52*/   GENx___x___x___ ,
 /*53*/   GENx___x___x___ ,
 /*54*/   GENx370x390x900 ( "N"         , RX_a , ASMFMT_RX       , and                                                 ),
 /*55*/   GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , compare_logical                                     ),
 /*56*/   GENx370x390x900 ( "O"         , RX_a , ASMFMT_RX       , or                                                  ),
 /*57*/   GENx370x390x900 ( "X"         , RX_a , ASMFMT_RX       , exclusive_or                                        ),
 /*58*/   GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , load                                                ),
 /*59*/   GENx370x390x900 ( "C"         , RX_a , ASMFMT_RX       , compare                                             ),
 /*5A*/   GENx370x390x900 ( "A"         , RX_a , ASMFMT_RX       , add                                                 ),
 /*5B*/   GENx370x390x900 ( "S"         , RX_a , ASMFMT_RX       , subtract                                            ),
 /*5C*/   GENx370x390x900 ( "M"         , RX_a , ASMFMT_RX       , multiply                                            ),
 /*5D*/   GENx370x390x900 ( "D"         , RX_a , ASMFMT_RX       , divide                                              ),
 /*5E*/   GENx370x390x900 ( "AL"        , RX_a , ASMFMT_RX       , add_logical                                         ),
 /*5F*/   GENx370x390x900 ( "SL"        , RX_a , ASMFMT_RX       , subtract_logical                                    ),
 /*60*/   GENx370x390x900 ( "STD"       , RX_a , ASMFMT_RX       , store_float_long                                    ),
 /*61*/   GENx___x___x___ ,
 /*62*/   GENx___x___x___ ,
 /*63*/   GENx___x___x___ ,
 /*64*/   GENx___x___x___ ,
 /*65*/   GENx___x___x___ ,
 /*66*/   GENx___x___x___ ,
 /*67*/   GENx370x390x900 ( "MXD"       , RX_a , ASMFMT_RX       , multiply_float_long_to_ext                          ),
 /*68*/   GENx370x390x900 ( "LD"        , RX_a , ASMFMT_RX       , load_float_long                                     ),
 /*69*/   GENx370x390x900 ( "CD"        , RX_a , ASMFMT_RX       , compare_float_long                                  ),
 /*6A*/   GENx370x390x900 ( "AD"        , RX_a , ASMFMT_RX       , add_float_long                                      ),
 /*6B*/   GENx370x390x900 ( "SD"        , RX_a , ASMFMT_RX       , subtract_float_long                                 ),
 /*6C*/   GENx370x390x900 ( "MD"        , RX_a , ASMFMT_RX       , multiply_float_long                                 ),
 /*6D*/   GENx370x390x900 ( "DD"        , RX_a , ASMFMT_RX       , divide_float_long                                   ),
 /*6E*/   GENx370x390x900 ( "AW"        , RX_a , ASMFMT_RX       , add_unnormal_float_long                             ),
 /*6F*/   GENx370x390x900 ( "SW"        , RX_a , ASMFMT_RX       , subtract_unnormal_float_long                        ),
 /*70*/   GENx370x390x900 ( "STE"       , RX_a , ASMFMT_RX       , store_float_short                                   ),
 /*71*/   GENx37Xx390x900 ( "MS"        , RX_a , ASMFMT_RX       , multiply_single                                     ),
 /*72*/   GENx___x___x___ ,
 /*73*/   GENx___x___x___ ,
 /*74*/   GENx___x___x___ ,
 /*75*/   GENx370x390x900 ( "TCPIP"     , RX_a , ASMFMT_RX       , tcpip                                               ),
 /*76*/   GENx___x___x___ ,
 /*77*/   GENx___x___x___ ,
 /*78*/   GENx370x390x900 ( "LE"        , RX_a , ASMFMT_RX       , load_float_short                                    ),
 /*79*/   GENx370x390x900 ( "CE"        , RX_a , ASMFMT_RX       , compare_float_short                                 ),
 /*7A*/   GENx370x390x900 ( "AE"        , RX_a , ASMFMT_RX       , add_float_short                                     ),
 /*7B*/   GENx370x390x900 ( "SE"        , RX_a , ASMFMT_RX       , subtract_float_short                                ),
 /*7C*/   GENx370x390x900 ( "MDE"       , RX_a , ASMFMT_RX       , multiply_float_short_to_long                        ),
 /*7D*/   GENx370x390x900 ( "DE"        , RX_a , ASMFMT_RX       , divide_float_short                                  ),
 /*7E*/   GENx370x390x900 ( "AU"        , RX_a , ASMFMT_RX       , add_unnormal_float_short                            ),
 /*7F*/   GENx370x390x900 ( "SU"        , RX_a , ASMFMT_RX       , subtract_unnormal_float_short                       ),
 /*80*/   GENx370x390x900 ( "SSM"       , SI   , ASMFMT_S        , set_system_mask                                     ),
 /*81*/   GENx___x___x___ ,
 /*82*/   GENx370x390x900 ( "LPSW"      , SI   , ASMFMT_S        , load_program_status_word                            ),
 /*83*/   GENx370x390x900 ( "DIAG"      , RS_a , ASMFMT_RS       , diagnose                                            ),
 /*84*/   GENx37Xx390x900 ( "BRXH"      , RSI  , ASMFMT_RSI      , branch_relative_on_index_high                       ),
 /*85*/   GENx37Xx390x900 ( "BRXLE"     , RSI  , ASMFMT_RSI      , branch_relative_on_index_low_or_equal               ),
 /*86*/   GENx370x390x900 ( "BXH"       , RS_a , ASMFMT_RS       , branch_on_index_high                                ),
 /*87*/   GENx370x390x900 ( "BXLE"      , RS_a , ASMFMT_RS       , branch_on_index_low_or_equal                        ),
 /*88*/   GENx370x390x900 ( "SRL"       , RS_a , ASMFMT_RS_R1D2B2, shift_right_single_logical                          ),
 /*89*/   GENx370x390x900 ( "SLL"       , RS_a , ASMFMT_RS_R1D2B2, shift_left_single_logical                           ),
 /*8A*/   GENx370x390x900 ( "SRA"       , RS_a , ASMFMT_RS_R1D2B2, shift_right_single                                  ),
 /*8B*/   GENx370x390x900 ( "SLA"       , RS_a , ASMFMT_RS_R1D2B2, shift_left_single                                   ),
 /*8C*/   GENx370x390x900 ( "SRDL"      , RS_a , ASMFMT_RS_R1D2B2, shift_right_double_logical                          ),
 /*8D*/   GENx370x390x900 ( "SLDL"      , RS_a , ASMFMT_RS_R1D2B2, shift_left_double_logical                           ),
 /*8E*/   GENx370x390x900 ( "SRDA"      , RS_a , ASMFMT_RS_R1D2B2, shift_right_double                                  ),
 /*8F*/   GENx370x390x900 ( "SLDA"      , RS_a , ASMFMT_RS_R1D2B2, shift_left_double                                   ),
 /*90*/   GENx370x390x900 ( "STM"       , RS_a , ASMFMT_RS       , store_multiple                                      ),
 /*91*/   GENx370x390x900 ( "TM"        , SI   , ASMFMT_SI       , test_under_mask                                     ),
 /*92*/   GENx370x390x900 ( "MVI"       , SI   , ASMFMT_SI       , move_immediate                                      ),
 /*93*/   GENx370x390x900 ( "TS"        , SI   , ASMFMT_S        , test_and_set                                        ),
 /*94*/   GENx370x390x900 ( "NI"        , SI   , ASMFMT_SI       , and_immediate                                       ),
 /*95*/   GENx370x390x900 ( "CLI"       , SI   , ASMFMT_SI       , compare_logical_immediate                           ),
 /*96*/   GENx370x390x900 ( "OI"        , SI   , ASMFMT_SI       , or_immediate                                        ),
 /*97*/   GENx370x390x900 ( "XI"        , SI   , ASMFMT_SI       , exclusive_or_immediate                              ),
 /*98*/   GENx370x390x900 ( "LM"        , RS_a , ASMFMT_RS       , load_multiple                                       ),
 /*99*/   GENx___x390x900 ( "TRACE"     , RS_a , ASMFMT_RS       , trace                                               ),
 /*9A*/   GENx___x390x900 ( "LAM"       , RS_a , ASMFMT_RS       , load_access_multiple                                ),
 /*9B*/   GENx___x390x900 ( "STAM"      , RS_a , ASMFMT_RS       , store_access_multiple                               ),
 /*9C*/   GENx370x___x___ ( "SIO"       , S    , ASMFMT_S        , start_io                                            ),
 /*9D*/   GENx370x___x___ ( "TIO"       , S    , ASMFMT_S        , test_io                                             ),
 /*9E*/   GENx370x___x___ ( "HIO"       , S    , ASMFMT_S        , halt_io                                             ),
 /*9F*/   GENx370x___x___ ( "TCH"       , S    , ASMFMT_S        , test_channel                                        ),
 /*A0*/   GENx___x___x___ ,
 /*A1*/   GENx___x___x___ ,
 /*A2*/   GENx___x___x___ ,
 /*A3*/   GENx___x___x___ ,
 /*A4*/   GENx370x390x900 ( ""          , a4xx , ASMFMT_a4xx     , execute_opcode_a4xx                                 ),
 /*A5*/   GENx370x390x900 ( ""          , a5_x , ASMFMT_a5_x     , execute_opcode_a5_x                                 ),
 /*A6*/   GENx370x390x900 ( ""          , a6xx , ASMFMT_a6xx     , execute_opcode_a6xx                                 ),
 /*A7*/   GENx370x390x900 ( ""          , a7_x , ASMFMT_a7_x     , execute_opcode_a7_x                                 ),
 /*A8*/   GENx37Xx390x900 ( "MVCLE"     , RS_a , ASMFMT_RS       , move_long_extended                                  ),
 /*A9*/   GENx37Xx390x900 ( "CLCLE"     , RS_a , ASMFMT_RS       , compare_logical_long_extended                       ),
 /*AA*/   GENx___x___x___ ,
 /*AB*/   GENx___x___x___ ,
 /*AC*/   GENx370x390x900 ( "STNSM"     , SI   , ASMFMT_SI       , store_then_and_system_mask                          ),
 /*AD*/   GENx370x390x900 ( "STOSM"     , SI   , ASMFMT_SI       , store_then_or_system_mask                           ),
 /*AE*/   GENx370x390x900 ( "SIGP"      , RS_a , ASMFMT_RS       , signal_processor                                    ),
 /*AF*/   GENx370x390x900 ( "MC"        , SI   , ASMFMT_SI       , monitor_call                                        ),
 /*B0*/   GENx___x___x___ ,
 /*B1*/   GENx370x390x900 ( "LRA"       , RX_a , ASMFMT_RX       , load_real_address                                   ),
 /*B2*/   GENx370x390x900 ( ""          , b2xx , ASMFMT_b2xx     , execute_opcode_b2xx                                 ),
 /*B3*/   GENx370x390x900 ( ""          , b3xx , ASMFMT_b3xx     , execute_opcode_b3xx                                 ),
 /*B4*/   GENx___x___x___ ,
 /*B5*/   GENx___x___x___ ,
 /*B6*/   GENx370x390x900 ( "STCTL"     , RS_a , ASMFMT_RS       , store_control                                       ),
 /*B7*/   GENx370x390x900 ( "LCTL"      , RS_a , ASMFMT_RS       , load_control                                        ),
 /*B8*/   GENx___x___x___ ,
 /*B9*/   GENx370x390x900 ( ""          , b9xx , ASMFMT_b9xx     , execute_opcode_b9xx                                 ),
 /*BA*/   GENx370x390x900 ( "CS"        , RS_a , ASMFMT_RS       , compare_and_swap                                    ),
 /*BB*/   GENx370x390x900 ( "CDS"       , RS_a , ASMFMT_RS       , compare_double_and_swap                             ),
 /*BC*/   GENx___x___x___ ,
 /*BD*/   GENx370x390x900 ( "CLM"       , RS_b , ASMFMT_RS       , compare_logical_characters_under_mask               ),
 /*BE*/   GENx370x390x900 ( "STCM"      , RS_b , ASMFMT_RS       , store_characters_under_mask                         ),
 /*BF*/   GENx370x390x900 ( "ICM"       , RS_b , ASMFMT_RS       , insert_characters_under_mask                        ),
 /*C0*/   GENx370x390x900 ( ""          , c0_x , ASMFMT_c0_x     , execute_opcode_c0_x                                 ),
 /*C1*/   GENx___x___x___ ,
 /*C2*/   GENx370x390x900 ( ""          , c2_x , ASMFMT_c2_x     , execute_opcode_c2_x                                 ),
 /*C3*/   GENx___x___x___ ,
 /*C4*/   GENx370x390x900 ( ""          , c4_x , ASMFMT_c4_x     , execute_opcode_c4_x                                 ),
 /*C5*/   GENx___x___x900 ( "BPRP"      , MII  , ASMFMT_MII_A    , branch_prediction_relative_preload                  ),
 /*C6*/   GENx370x390x900 ( ""          , c6_x , ASMFMT_c6_x     , execute_opcode_c6_x                                 ),
 /*C7*/   GENx___x___x900 ( "BPP"       , SMI  , ASMFMT_SMI_A    , branch_prediction_preload                           ),
 /*C8*/   GENx370x390x900 ( ""          , c8_x , ASMFMT_c8_x     , execute_opcode_c8_x                                 ),
 /*C9*/   GENx___x___x___ ,
 /*CA*/   GENx___x___x___ ,
 /*CB*/   GENx___x___x___ ,
 /*CC*/   GENx370x390x900 ( ""          , cc_x , ASMFMT_cc_x     , execute_opcode_cc_x                                 ),
 /*CD*/   GENx___x___x___ ,
 /*CE*/   GENx___x___x___ ,
 /*CF*/   GENx___x___x___ ,
 /*D0*/   GENx37Xx390x900 ( "TRTR"      , SS_a , ASMFMT_SS_L     , translate_and_test_reverse                          ),
 /*D1*/   GENx370x390x900 ( "MVN"       , SS_a , ASMFMT_SS_L     , move_numerics                                       ),
 /*D2*/   GENx370x390x900 ( "MVC"       , SS_a , ASMFMT_SS_L     , move_character                                      ),
 /*D3*/   GENx370x390x900 ( "MVZ"       , SS_a , ASMFMT_SS_L     , move_zones                                          ),
 /*D4*/   GENx370x390x900 ( "NC"        , SS_a , ASMFMT_SS_L     , and_character                                       ),
 /*D5*/   GENx370x390x900 ( "CLC"       , SS_a , ASMFMT_SS_L     , compare_logical_character                           ),
 /*D6*/   GENx370x390x900 ( "OC"        , SS_a , ASMFMT_SS_L     , or_character                                        ),
 /*D7*/   GENx370x390x900 ( "XC"        , SS_a , ASMFMT_SS_L     , exclusive_or_character                              ),
 /*D8*/   GENx___x___x___ ,
 /*D9*/   GENx370x390x900 ( "MVCK"      , SS_d , ASMFMT_SS_R3    , move_with_key                                       ),
 /*DA*/   GENx370x390x900 ( "MVCP"      , SS_d , ASMFMT_SS_R3    , move_to_primary                                     ),
 /*DB*/   GENx370x390x900 ( "MVCS"      , SS_d , ASMFMT_SS_R3    , move_to_secondary                                   ),
 /*DC*/   GENx370x390x900 ( "TR"        , SS_a , ASMFMT_SS_L     , translate                                           ),
 /*DD*/   GENx370x390x900 ( "TRT"       , SS_a , ASMFMT_SS_L     , translate_and_test                                  ),
 /*DE*/   GENx370x390x900 ( "ED"        , SS_a , ASMFMT_SS_L     , edit_x_edit_and_mark                                ),
 /*DF*/   GENx370x390x900 ( "EDMK"      , SS_a , ASMFMT_SS_L     , edit_x_edit_and_mark                                ),
 /*E0*/   GENx___x___x___ ,
 /*E1*/   GENx37Xx390x900 ( "PKU"       , SS_f , ASMFMT_SS_L2    , pack_unicode                                        ),
 /*E2*/   GENx37Xx390x900 ( "UNPKU"     , SS_a , ASMFMT_SS_L     , unpack_unicode                                      ),
 /*E3*/   GENx370x390x900 ( ""          , e3xx , ASMFMT_e3xx     , execute_opcode_e3________xx                         ),
 /*E4*/   GENx370x390x900 ( ""          , e4xx , ASMFMT_e4xx     , execute_opcode_e4xx                                 ),
 /*E5*/   GENx370x390x900 ( ""          , e5xx , ASMFMT_e5xx     , execute_opcode_e5xx                                 ),
 /*E6*/   GENx370x390x900 ( ""          , e6xx , ASMFMT_e6xx     , execute_opcode_e6xx                                 ),
 /*E7*/   GENx___x___x___ ,
 /*E8*/   GENx370x390x900 ( "MVCIN"     , SS_a , ASMFMT_SS_L     , move_inverse                                        ),
 /*E9*/   GENx37Xx390x900 ( "PKA"       , SS_f , ASMFMT_SS_L2    , pack_ascii                                          ),
 /*EA*/   GENx37Xx390x900 ( "UNPKA"     , SS_a , ASMFMT_SS_L     , unpack_ascii                                        ),
 /*EB*/   GENx370x390x900 ( ""          , ebxx , ASMFMT_ebxx     , execute_opcode_eb________xx                         ),
 /*EC*/   GENx370x390x900 ( ""          , ecxx , ASMFMT_ecxx     , execute_opcode_ec________xx                         ),
 /*ED*/   GENx370x390x900 ( ""          , edxx , ASMFMT_edxx     , execute_opcode_ed________xx                         ),
 /*EE*/   GENx___x390x900 ( "PLO"       , SS_e , ASMFMT_SS_RSRS  , perform_locked_operation                            ),
 /*EF*/   GENx___x___x900 ( "LMD"       , SS_e , ASMFMT_SS_R     , load_multiple_disjoint                              ),
 /*F0*/   GENx370x390x900 ( "SRP"       , SS_c , ASMFMT_SS_I     , shift_and_round_decimal                             ),
 /*F1*/   GENx370x390x900 ( "MVO"       , SS_b , ASMFMT_SS       , move_with_offset                                    ),
 /*F2*/   GENx370x390x900 ( "PACK"      , SS_b , ASMFMT_SS       , pack                                                ),
 /*F3*/   GENx370x390x900 ( "UNPK"      , SS_b , ASMFMT_SS       , unpack                                              ),
 /*F4*/   GENx___x___x___ ,
 /*F5*/   GENx___x___x___ ,
 /*F6*/   GENx___x___x___ ,
 /*F7*/   GENx___x___x___ ,
 /*F8*/   GENx370x390x900 ( "ZAP"       , SS_b , ASMFMT_SS       , zero_and_add                                        ),
 /*F9*/   GENx370x390x900 ( "CP"        , SS_b , ASMFMT_SS       , compare_decimal                                     ),
 /*FA*/   GENx370x390x900 ( "AP"        , SS_b , ASMFMT_SS       , add_decimal                                         ),
 /*FB*/   GENx370x390x900 ( "SP"        , SS_b , ASMFMT_SS       , subtract_decimal                                    ),
 /*FC*/   GENx370x390x900 ( "MP"        , SS_b , ASMFMT_SS       , multiply_decimal                                    ),
 /*FD*/   GENx370x390x900 ( "DP"        , SS_b , ASMFMT_SS       , divide_decimal                                      ),
 /*FE*/   GENx___x___x___ ,
 /*FF*/   GENx___x___x___
};

static INSTR_FUNC gen_opcode_01xx[256][NUM_INSTR_TAB_PTRS] =
{
 /*0100*/ GENx___x___x___ ,
 /*0101*/ GENx___x390x900 ( "PR"        , E    , ASMFMT_E        , program_return                                      ),
 /*0102*/ GENx37Xx390x900 ( "UPT"       , E    , ASMFMT_E        , update_tree                                         ),
 /*0103*/ GENx___x___x___ ,
 /*0104*/ GENx___x___x900 ( "PTFF"      , E    , ASMFMT_E        , perform_timing_facility_function                    ),
 /*0105*/ GENx___x___x___ ,/* CMSG - Clear Message */
 /*0106*/ GENx___x___x___ ,/* TMSG - Test Message */
 /*0107*/ GENx___x390x900 ( "SCKPF"     , E    , ASMFMT_E        , set_clock_programmable_field                        ),
 /*0108*/ GENx___x___x___ ,/* TMPS - Test Message Path State */
 /*0109*/ GENx___x___x___ ,/* CMPS - Clear Message Path State */
 /*010A*/ GENx___x___x900 ( "PFPO"      , E    , ASMFMT_E        , perform_floating_point_operation                    ),
 /*010B*/ GENx37Xx390x900 ( "TAM"       , E    , ASMFMT_E        , test_addressing_mode                                ),
 /*010C*/ GENx37Xx390x900 ( "SAM24"     , E    , ASMFMT_E        , set_addressing_mode_24                              ),
 /*010D*/ GENx37Xx390x900 ( "SAM31"     , E    , ASMFMT_E        , set_addressing_mode_31                              ),
 /*010E*/ GENx___x___x900 ( "SAM64"     , E    , ASMFMT_E        , set_addressing_mode_64                              ),
 /*010F*/ GENx___x___x___ ,
 /*0110*/ GENx___x___x___ ,
 /*0111*/ GENx___x___x___ ,
 /*0112*/ GENx___x___x___ ,
 /*0113*/ GENx___x___x___ ,
 /*0114*/ GENx___x___x___ ,
 /*0115*/ GENx___x___x___ ,
 /*0116*/ GENx___x___x___ ,
 /*0117*/ GENx___x___x___ ,
 /*0118*/ GENx___x___x___ ,
 /*0119*/ GENx___x___x___ ,
 /*011A*/ GENx___x___x___ ,
 /*011B*/ GENx___x___x___ ,
 /*011C*/ GENx___x___x___ ,
 /*011D*/ GENx___x___x___ ,
 /*011E*/ GENx___x___x___ ,
 /*011F*/ GENx___x___x___ ,
 /*0120*/ GENx___x___x___ ,
 /*0121*/ GENx___x___x___ ,
 /*0122*/ GENx___x___x___ ,
 /*0123*/ GENx___x___x___ ,
 /*0124*/ GENx___x___x___ ,
 /*0125*/ GENx___x___x___ ,
 /*0126*/ GENx___x___x___ ,
 /*0127*/ GENx___x___x___ ,
 /*0128*/ GENx___x___x___ ,
 /*0129*/ GENx___x___x___ ,
 /*012A*/ GENx___x___x___ ,
 /*012B*/ GENx___x___x___ ,
 /*012C*/ GENx___x___x___ ,
 /*012D*/ GENx___x___x___ ,
 /*012E*/ GENx___x___x___ ,
 /*012F*/ GENx___x___x___ ,
 /*0130*/ GENx___x___x___ ,
 /*0131*/ GENx___x___x___ ,
 /*0132*/ GENx___x___x___ ,
 /*0133*/ GENx___x___x___ ,
 /*0134*/ GENx___x___x___ ,
 /*0135*/ GENx___x___x___ ,
 /*0136*/ GENx___x___x___ ,
 /*0137*/ GENx___x___x___ ,
 /*0138*/ GENx___x___x___ ,
 /*0139*/ GENx___x___x___ ,
 /*013A*/ GENx___x___x___ ,
 /*013B*/ GENx___x___x___ ,
 /*013C*/ GENx___x___x___ ,
 /*013D*/ GENx___x___x___ ,
 /*013E*/ GENx___x___x___ ,
 /*013F*/ GENx___x___x___ ,
 /*0140*/ GENx___x___x___ ,
 /*0141*/ GENx___x___x___ ,
 /*0142*/ GENx___x___x___ ,
 /*0143*/ GENx___x___x___ ,
 /*0144*/ GENx___x___x___ ,
 /*0145*/ GENx___x___x___ ,
 /*0146*/ GENx___x___x___ ,
 /*0147*/ GENx___x___x___ ,
 /*0148*/ GENx___x___x___ ,
 /*0149*/ GENx___x___x___ ,
 /*014A*/ GENx___x___x___ ,
 /*014B*/ GENx___x___x___ ,
 /*014C*/ GENx___x___x___ ,
 /*014D*/ GENx___x___x___ ,
 /*014E*/ GENx___x___x___ ,
 /*014F*/ GENx___x___x___ ,
 /*0150*/ GENx___x___x___ ,
 /*0151*/ GENx___x___x___ ,
 /*0152*/ GENx___x___x___ ,
 /*0153*/ GENx___x___x___ ,
 /*0154*/ GENx___x___x___ ,
 /*0155*/ GENx___x___x___ ,
 /*0156*/ GENx___x___x___ ,
 /*0157*/ GENx___x___x___ ,
 /*0158*/ GENx___x___x___ ,
 /*0159*/ GENx___x___x___ ,
 /*015A*/ GENx___x___x___ ,
 /*015B*/ GENx___x___x___ ,
 /*015C*/ GENx___x___x___ ,
 /*015D*/ GENx___x___x___ ,
 /*015E*/ GENx___x___x___ ,
 /*015F*/ GENx___x___x___ ,
 /*0160*/ GENx___x___x___ ,
 /*0161*/ GENx___x___x___ ,
 /*0162*/ GENx___x___x___ ,
 /*0163*/ GENx___x___x___ ,
 /*0164*/ GENx___x___x___ ,
 /*0165*/ GENx___x___x___ ,
 /*0166*/ GENx___x___x___ ,
 /*0167*/ GENx___x___x___ ,
 /*0168*/ GENx___x___x___ ,
 /*0169*/ GENx___x___x___ ,
 /*016A*/ GENx___x___x___ ,
 /*016B*/ GENx___x___x___ ,
 /*016C*/ GENx___x___x___ ,
 /*016D*/ GENx___x___x___ ,
 /*016E*/ GENx___x___x___ ,
 /*016F*/ GENx___x___x___ ,
 /*0170*/ GENx___x___x___ ,
 /*0171*/ GENx___x___x___ ,
 /*0172*/ GENx___x___x___ ,
 /*0173*/ GENx___x___x___ ,
 /*0174*/ GENx___x___x___ ,
 /*0175*/ GENx___x___x___ ,
 /*0176*/ GENx___x___x___ ,
 /*0177*/ GENx___x___x___ ,
 /*0178*/ GENx___x___x___ ,
 /*0179*/ GENx___x___x___ ,
 /*017A*/ GENx___x___x___ ,
 /*017B*/ GENx___x___x___ ,
 /*017C*/ GENx___x___x___ ,
 /*017D*/ GENx___x___x___ ,
 /*017E*/ GENx___x___x___ ,
 /*017F*/ GENx___x___x___ ,
 /*0180*/ GENx___x___x___ ,
 /*0181*/ GENx___x___x___ ,
 /*0182*/ GENx___x___x___ ,
 /*0183*/ GENx___x___x___ ,
 /*0184*/ GENx___x___x___ ,
 /*0185*/ GENx___x___x___ ,
 /*0186*/ GENx___x___x___ ,
 /*0187*/ GENx___x___x___ ,
 /*0188*/ GENx___x___x___ ,
 /*0189*/ GENx___x___x___ ,
 /*018A*/ GENx___x___x___ ,
 /*018B*/ GENx___x___x___ ,
 /*018C*/ GENx___x___x___ ,
 /*018D*/ GENx___x___x___ ,
 /*018E*/ GENx___x___x___ ,
 /*018F*/ GENx___x___x___ ,
 /*0190*/ GENx___x___x___ ,
 /*0191*/ GENx___x___x___ ,
 /*0192*/ GENx___x___x___ ,
 /*0193*/ GENx___x___x___ ,
 /*0194*/ GENx___x___x___ ,
 /*0195*/ GENx___x___x___ ,
 /*0196*/ GENx___x___x___ ,
 /*0197*/ GENx___x___x___ ,
 /*0198*/ GENx___x___x___ ,
 /*0199*/ GENx___x___x___ ,
 /*019A*/ GENx___x___x___ ,
 /*019B*/ GENx___x___x___ ,
 /*019C*/ GENx___x___x___ ,
 /*019D*/ GENx___x___x___ ,
 /*019E*/ GENx___x___x___ ,
 /*019F*/ GENx___x___x___ ,
 /*01A0*/ GENx___x___x___ ,
 /*01A1*/ GENx___x___x___ ,
 /*01A2*/ GENx___x___x___ ,
 /*01A3*/ GENx___x___x___ ,
 /*01A4*/ GENx___x___x___ ,
 /*01A5*/ GENx___x___x___ ,
 /*01A6*/ GENx___x___x___ ,
 /*01A7*/ GENx___x___x___ ,
 /*01A8*/ GENx___x___x___ ,
 /*01A9*/ GENx___x___x___ ,
 /*01AA*/ GENx___x___x___ ,
 /*01AB*/ GENx___x___x___ ,
 /*01AC*/ GENx___x___x___ ,
 /*01AD*/ GENx___x___x___ ,
 /*01AE*/ GENx___x___x___ ,
 /*01AF*/ GENx___x___x___ ,
 /*01B0*/ GENx___x___x___ ,
 /*01B1*/ GENx___x___x___ ,
 /*01B2*/ GENx___x___x___ ,
 /*01B3*/ GENx___x___x___ ,
 /*01B4*/ GENx___x___x___ ,
 /*01B5*/ GENx___x___x___ ,
 /*01B6*/ GENx___x___x___ ,
 /*01B7*/ GENx___x___x___ ,
 /*01B8*/ GENx___x___x___ ,
 /*01B9*/ GENx___x___x___ ,
 /*01BA*/ GENx___x___x___ ,
 /*01BB*/ GENx___x___x___ ,
 /*01BC*/ GENx___x___x___ ,
 /*01BD*/ GENx___x___x___ ,
 /*01BE*/ GENx___x___x___ ,
 /*01BF*/ GENx___x___x___ ,
 /*01C0*/ GENx___x___x___ ,
 /*01C1*/ GENx___x___x___ ,
 /*01C2*/ GENx___x___x___ ,
 /*01C3*/ GENx___x___x___ ,
 /*01C4*/ GENx___x___x___ ,
 /*01C5*/ GENx___x___x___ ,
 /*01C6*/ GENx___x___x___ ,
 /*01C7*/ GENx___x___x___ ,
 /*01C8*/ GENx___x___x___ ,
 /*01C9*/ GENx___x___x___ ,
 /*01CA*/ GENx___x___x___ ,
 /*01CB*/ GENx___x___x___ ,
 /*01CC*/ GENx___x___x___ ,
 /*01CD*/ GENx___x___x___ ,
 /*01CE*/ GENx___x___x___ ,
 /*01CF*/ GENx___x___x___ ,
 /*01D0*/ GENx___x___x___ ,
 /*01D1*/ GENx___x___x___ ,
 /*01D2*/ GENx___x___x___ ,
 /*01D3*/ GENx___x___x___ ,
 /*01D4*/ GENx___x___x___ ,
 /*01D5*/ GENx___x___x___ ,
 /*01D6*/ GENx___x___x___ ,
 /*01D7*/ GENx___x___x___ ,
 /*01D8*/ GENx___x___x___ ,
 /*01D9*/ GENx___x___x___ ,
 /*01DA*/ GENx___x___x___ ,
 /*01DB*/ GENx___x___x___ ,
 /*01DC*/ GENx___x___x___ ,
 /*01DD*/ GENx___x___x___ ,
 /*01DE*/ GENx___x___x___ ,
 /*01DF*/ GENx___x___x___ ,
 /*01E0*/ GENx___x___x___ ,
 /*01E1*/ GENx___x___x___ ,
 /*01E2*/ GENx___x___x___ ,
 /*01E3*/ GENx___x___x___ ,
 /*01E4*/ GENx___x___x___ ,
 /*01E5*/ GENx___x___x___ ,
 /*01E6*/ GENx___x___x___ ,
 /*01E7*/ GENx___x___x___ ,
 /*01E8*/ GENx___x___x___ ,
 /*01E9*/ GENx___x___x___ ,
 /*01EA*/ GENx___x___x___ ,
 /*01EB*/ GENx___x___x___ ,
 /*01EC*/ GENx___x___x___ ,
 /*01ED*/ GENx___x___x___ ,
 /*01EE*/ GENx___x___x___ ,
 /*01EF*/ GENx___x___x___ ,
 /*01F0*/ GENx___x___x___ ,
 /*01F1*/ GENx___x___x___ ,
 /*01F2*/ GENx___x___x___ ,
 /*01F3*/ GENx___x___x___ ,
 /*01F4*/ GENx___x___x___ ,
 /*01F5*/ GENx___x___x___ ,
 /*01F6*/ GENx___x___x___ ,
 /*01F7*/ GENx___x___x___ ,
 /*01F8*/ GENx___x___x___ ,
 /*01F9*/ GENx___x___x___ ,
 /*01FA*/ GENx___x___x___ ,
 /*01FB*/ GENx___x___x___ ,
 /*01FC*/ GENx___x___x___ ,
 /*01FD*/ GENx___x___x___ ,
 /*01FE*/ GENx___x___x___ ,
 /*01FF*/ GENx___x390x900 ( "TRAP2"     , E    , ASMFMT_E        , trap2                                               )
};

static INSTR_FUNC gen_opcode_a5_x[16][NUM_INSTR_TAB_PTRS] =
{
 /*A5x0*/ GENx___x___x900 ( "IIHH"      , RI_a , ASMFMT_RI       , insert_immediate_high_high                          ),
 /*A5x1*/ GENx___x___x900 ( "IIHL"      , RI_a , ASMFMT_RI       , insert_immediate_high_low                           ),
 /*A5x2*/ GENx37Xx___x900 ( "IILH"      , RI_a , ASMFMT_RI       , insert_immediate_low_high                           ),
 /*A5x3*/ GENx37Xx___x900 ( "IILL"      , RI_a , ASMFMT_RI       , insert_immediate_low_low                            ),
 /*A5x4*/ GENx___x___x900 ( "NIHH"      , RI_a , ASMFMT_RI       , and_immediate_high_high                             ),
 /*A5x5*/ GENx___x___x900 ( "NIHL"      , RI_a , ASMFMT_RI       , and_immediate_high_low                              ),
 /*A5x6*/ GENx37Xx___x900 ( "NILH"      , RI_a , ASMFMT_RI       , and_immediate_low_high                              ),
 /*A5x7*/ GENx37Xx___x900 ( "NILL"      , RI_a , ASMFMT_RI       , and_immediate_low_low                               ),
 /*A5x8*/ GENx___x___x900 ( "OIHH"      , RI_a , ASMFMT_RI       , or_immediate_high_high                              ),
 /*A5x9*/ GENx___x___x900 ( "OIHL"      , RI_a , ASMFMT_RI       , or_immediate_high_low                               ),
 /*A5xA*/ GENx37Xx___x900 ( "OILH"      , RI_a , ASMFMT_RI       , or_immediate_low_high                               ),
 /*A5xB*/ GENx37Xx___x900 ( "OILL"      , RI_a , ASMFMT_RI       , or_immediate_low_low                                ),
 /*A5xC*/ GENx___x___x900 ( "LLIHH"     , RI_a , ASMFMT_RI       , load_logical_immediate_high_high                    ),
 /*A5xD*/ GENx___x___x900 ( "LLIHL"     , RI_a , ASMFMT_RI       , load_logical_immediate_high_low                     ),
 /*A5xE*/ GENx37Xx___x900 ( "LLILH"     , RI_a , ASMFMT_RI       , load_logical_immediate_low_high                     ),
 /*A5xF*/ GENx37Xx___x900 ( "LLILL"     , RI_a , ASMFMT_RI       , load_logical_immediate_low_low                      )
};

static INSTR_FUNC gen_opcode_a7_x[16][NUM_INSTR_TAB_PTRS] =
{
 /*A7x0*/ GENx37Xx390x900 ( "TMLH"      , RI_a , ASMFMT_RI       , test_under_mask_high                                ),
 /*A7x1*/ GENx37Xx390x900 ( "TMLL"      , RI_a , ASMFMT_RI       , test_under_mask_low                                 ),
 /*A7x2*/ GENx___x___x900 ( "TMHH"      , RI_a , ASMFMT_RI       , test_under_mask_high_high                           ),
 /*A7x3*/ GENx___x___x900 ( "TMHL"      , RI_a , ASMFMT_RI       , test_under_mask_high_low                            ),
 /*A7x4*/ GENx37Xx390x900 ( "BRC"       , RI_c , ASMFMT_RI_B     , branch_relative_on_condition                        ),
 /*A7x5*/ GENx37Xx390x900 ( "BRAS"      , RI_b , ASMFMT_RI_B     , branch_relative_and_save                            ),
 /*A7x6*/ GENx37Xx390x900 ( "BRCT"      , RI_b , ASMFMT_RI_B     , branch_relative_on_count                            ),
 /*A7x7*/ GENx___x___x900 ( "BRCTG"     , RI_b , ASMFMT_RI_B     , branch_relative_on_count_long                       ),
 /*A7x8*/ GENx37Xx390x900 ( "LHI"       , RI_a , ASMFMT_RI       , load_halfword_immediate                             ),
 /*A7x9*/ GENx___x___x900 ( "LGHI"      , RI_a , ASMFMT_RI       , load_long_halfword_immediate                        ),
 /*A7xA*/ GENx37Xx390x900 ( "AHI"       , RI_a , ASMFMT_RI       , add_halfword_immediate                              ),
 /*A7xB*/ GENx___x___x900 ( "AGHI"      , RI_a , ASMFMT_RI       , add_long_halfword_immediate                         ),
 /*A7xC*/ GENx37Xx390x900 ( "MHI"       , RI_a , ASMFMT_RI       , multiply_halfword_immediate                         ),
 /*A7xD*/ GENx___x___x900 ( "MGHI"      , RI_a , ASMFMT_RI       , multiply_long_halfword_immediate                    ),
 /*A7xE*/ GENx37Xx390x900 ( "CHI"       , RI_a , ASMFMT_RI       , compare_halfword_immediate                          ),
 /*A7xF*/ GENx___x___x900 ( "CGHI"      , RI_a , ASMFMT_RI       , compare_long_halfword_immediate                     )
};

static INSTR_FUNC gen_opcode_b3xx[256][NUM_INSTR_TAB_PTRS] =
{
 /*B300*/ GENx37Xx390x900 ( "LPEBR"     , RRE  , ASMFMT_RRE      , load_positive_bfp_short_reg                         ),
 /*B301*/ GENx37Xx390x900 ( "LNEBR"     , RRE  , ASMFMT_RRE      , load_negative_bfp_short_reg                         ),
 /*B302*/ GENx37Xx390x900 ( "LTEBR"     , RRE  , ASMFMT_RRE      , load_and_test_bfp_short_reg                         ),
 /*B303*/ GENx37Xx390x900 ( "LCEBR"     , RRE  , ASMFMT_RRE      , load_complement_bfp_short_reg                       ),
 /*B304*/ GENx37Xx390x900 ( "LDEBR"     , RRE  , ASMFMT_RRE      , load_lengthened_bfp_short_to_long_reg               ),
 /*B305*/ GENx37Xx390x900 ( "LXDBR"     , RRE  , ASMFMT_RRE      , load_lengthened_bfp_long_to_ext_reg                 ),
 /*B306*/ GENx37Xx390x900 ( "LXEBR"     , RRE  , ASMFMT_RRE      , load_lengthened_bfp_short_to_ext_reg                ),
 /*B307*/ GENx37Xx390x900 ( "MXDBR"     , RRE  , ASMFMT_RRE      , multiply_bfp_long_to_ext_reg                        ),
 /*B308*/ GENx37Xx390x900 ( "KEBR"      , RRE  , ASMFMT_RRE      , compare_and_signal_bfp_short_reg                    ),
 /*B309*/ GENx37Xx390x900 ( "CEBR"      , RRE  , ASMFMT_RRE      , compare_bfp_short_reg                               ),
 /*B30A*/ GENx37Xx390x900 ( "AEBR"      , RRE  , ASMFMT_RRE      , add_bfp_short_reg                                   ),
 /*B30B*/ GENx37Xx390x900 ( "SEBR"      , RRE  , ASMFMT_RRE      , subtract_bfp_short_reg                              ),
 /*B30C*/ GENx37Xx390x900 ( "MDEBR"     , RRE  , ASMFMT_RRE      , multiply_bfp_short_to_long_reg                      ),
 /*B30D*/ GENx37Xx390x900 ( "DEBR"      , RRE  , ASMFMT_RRE      , divide_bfp_short_reg                                ),
 /*B30E*/ GENx37Xx390x900 ( "MAEBR"     , RRD  , ASMFMT_RRF_R    , multiply_add_bfp_short_reg                          ),
 /*B30F*/ GENx37Xx390x900 ( "MSEBR"     , RRD  , ASMFMT_RRF_R    , multiply_subtract_bfp_short_reg                     ),
 /*B310*/ GENx37Xx390x900 ( "LPDBR"     , RRE  , ASMFMT_RRE      , load_positive_bfp_long_reg                          ),
 /*B311*/ GENx37Xx390x900 ( "LNDBR"     , RRE  , ASMFMT_RRE      , load_negative_bfp_long_reg                          ),
 /*B312*/ GENx37Xx390x900 ( "LTDBR"     , RRE  , ASMFMT_RRE      , load_and_test_bfp_long_reg                          ),
 /*B313*/ GENx37Xx390x900 ( "LCDBR"     , RRE  , ASMFMT_RRE      , load_complement_bfp_long_reg                        ),
 /*B314*/ GENx37Xx390x900 ( "SQEBR"     , RRE  , ASMFMT_RRE      , squareroot_bfp_short_reg                            ),
 /*B315*/ GENx37Xx390x900 ( "SQDBR"     , RRE  , ASMFMT_RRE      , squareroot_bfp_long_reg                             ),
 /*B316*/ GENx37Xx390x900 ( "SQXBR"     , RRE  , ASMFMT_RRE      , squareroot_bfp_ext_reg                              ),
 /*B317*/ GENx37Xx390x900 ( "MEEBR"     , RRE  , ASMFMT_RRE      , multiply_bfp_short_reg                              ),
 /*B318*/ GENx37Xx390x900 ( "KDBR"      , RRE  , ASMFMT_RRE      , compare_and_signal_bfp_long_reg                     ),
 /*B319*/ GENx37Xx390x900 ( "CDBR"      , RRE  , ASMFMT_RRE      , compare_bfp_long_reg                                ),
 /*B31A*/ GENx37Xx390x900 ( "ADBR"      , RRE  , ASMFMT_RRE      , add_bfp_long_reg                                    ),
 /*B31B*/ GENx37Xx390x900 ( "SDBR"      , RRE  , ASMFMT_RRE      , subtract_bfp_long_reg                               ),
 /*B31C*/ GENx37Xx390x900 ( "MDBR"      , RRE  , ASMFMT_RRE      , multiply_bfp_long_reg                               ),
 /*B31D*/ GENx37Xx390x900 ( "DDBR"      , RRE  , ASMFMT_RRE      , divide_bfp_long_reg                                 ),
 /*B31E*/ GENx37Xx390x900 ( "MADBR"     , RRD  , ASMFMT_RRF_R    , multiply_add_bfp_long_reg                           ),
 /*B31F*/ GENx37Xx390x900 ( "MSDBR"     , RRD  , ASMFMT_RRF_R    , multiply_subtract_bfp_long_reg                      ),
 /*B320*/ GENx___x___x___ ,
 /*B321*/ GENx___x___x___ ,
 /*B322*/ GENx___x___x___ ,
 /*B323*/ GENx___x___x___ ,
 /*B324*/ GENx37Xx390x900 ( "LDER"      , RRE  , ASMFMT_RRE      , load_lengthened_float_short_to_long_reg             ),
 /*B325*/ GENx37Xx390x900 ( "LXDR"      , RRE  , ASMFMT_RRE      , load_lengthened_float_long_to_ext_reg               ),
 /*B326*/ GENx37Xx390x900 ( "LXER"      , RRE  , ASMFMT_RRE      , load_lengthened_float_short_to_ext_reg              ),
 /*B327*/ GENx___x___x___ ,
 /*B328*/ GENx___x___x___ ,
 /*B329*/ GENx___x___x___ ,
 /*B32A*/ GENx___x___x___ ,
 /*B32B*/ GENx___x___x___ ,
 /*B32C*/ GENx___x___x___ ,
 /*B32D*/ GENx___x___x___ ,
 /*B32E*/ GENx37Xx390x900 ( "MAER"      , RRD  , ASMFMT_RRF_R    , multiply_add_float_short_reg                        ),
 /*B32F*/ GENx37Xx390x900 ( "MSER"      , RRD  , ASMFMT_RRF_R    , multiply_subtract_float_short_reg                   ),
 /*B330*/ GENx___x___x___ ,
 /*B331*/ GENx___x___x___ ,
 /*B332*/ GENx___x___x___ ,
 /*B333*/ GENx___x___x___ ,
 /*B334*/ GENx___x___x___ ,
 /*B335*/ GENx___x___x___ ,
 /*B336*/ GENx37Xx390x900 ( "SQXR"      , RRE  , ASMFMT_RRE      , squareroot_float_ext_reg                            ),
 /*B337*/ GENx37Xx390x900 ( "MEER"      , RRE  , ASMFMT_RRE      , multiply_float_short_reg                            ),
 /*B338*/ GENx37Xx___x900 ( "MAYLR"     , RRD  , ASMFMT_RRF_R    , multiply_add_unnormal_float_long_to_ext_low_reg     ),
 /*B339*/ GENx37Xx___x900 ( "MYLR"      , RRD  , ASMFMT_RRF_R    , multiply_unnormal_float_long_to_ext_low_reg         ),
 /*B33A*/ GENx37Xx___x900 ( "MAYR"      , RRD  , ASMFMT_RRF_R    , multiply_add_unnormal_float_long_to_ext_reg         ),
 /*B33B*/ GENx37Xx___x900 ( "MYR"       , RRD  , ASMFMT_RRF_R    , multiply_unnormal_float_long_to_ext_reg             ),
 /*B33C*/ GENx37Xx___x900 ( "MAYHR"     , RRD  , ASMFMT_RRF_R    , multiply_add_unnormal_float_long_to_ext_high_reg    ),
 /*B33D*/ GENx37Xx___x900 ( "MYHR"      , RRD  , ASMFMT_RRF_R    , multiply_unnormal_float_long_to_ext_high_reg        ),
 /*B33E*/ GENx37Xx390x900 ( "MADR"      , RRD  , ASMFMT_RRF_R    , multiply_add_float_long_reg                         ),
 /*B33F*/ GENx37Xx390x900 ( "MSDR"      , RRD  , ASMFMT_RRF_R    , multiply_subtract_float_long_reg                    ),
 /*B340*/ GENx37Xx390x900 ( "LPXBR"     , RRE  , ASMFMT_RRE      , load_positive_bfp_ext_reg                           ),
 /*B341*/ GENx37Xx390x900 ( "LNXBR"     , RRE  , ASMFMT_RRE      , load_negative_bfp_ext_reg                           ),
 /*B342*/ GENx37Xx390x900 ( "LTXBR"     , RRE  , ASMFMT_RRE      , load_and_test_bfp_ext_reg                           ),
 /*B343*/ GENx37Xx390x900 ( "LCXBR"     , RRE  , ASMFMT_RRE      , load_complement_bfp_ext_reg                         ),
 /*B344*/ GENx37Xx390x900 ( "LEDBR"     , RRF_e, ASMFMT_RRE      , load_rounded_bfp_long_to_short_reg                  ),
 /*B345*/ GENx37Xx390x900 ( "LDXBR"     , RRF_e, ASMFMT_RRE      , load_rounded_bfp_ext_to_long_reg                    ),
 /*B346*/ GENx37Xx390x900 ( "LEXBR"     , RRF_e, ASMFMT_RRE      , load_rounded_bfp_ext_to_short_reg                   ),
 /*B347*/ GENx37Xx390x900 ( "FIXBR"     , RRF_e, ASMFMT_RRF_M    , load_fp_int_bfp_ext_reg                             ),
 /*B348*/ GENx37Xx390x900 ( "KXBR"      , RRE  , ASMFMT_RRE      , compare_and_signal_bfp_ext_reg                      ),
 /*B349*/ GENx37Xx390x900 ( "CXBR"      , RRE  , ASMFMT_RRE      , compare_bfp_ext_reg                                 ),
 /*B34A*/ GENx37Xx390x900 ( "AXBR"      , RRE  , ASMFMT_RRE      , add_bfp_ext_reg                                     ),
 /*B34B*/ GENx37Xx390x900 ( "SXBR"      , RRE  , ASMFMT_RRE      , subtract_bfp_ext_reg                                ),
 /*B34C*/ GENx37Xx390x900 ( "MXBR"      , RRE  , ASMFMT_RRE      , multiply_bfp_ext_reg                                ),
 /*B34D*/ GENx37Xx390x900 ( "DXBR"      , RRE  , ASMFMT_RRE      , divide_bfp_ext_reg                                  ),
 /*B34E*/ GENx___x___x___ ,
 /*B34F*/ GENx___x___x___ ,
 /*B350*/ GENx37Xx390x900 ( "TBEDR"     , RRF_e, ASMFMT_RRF_M    , convert_float_long_to_bfp_short_reg                 ),
 /*B351*/ GENx37Xx390x900 ( "TBDR"      , RRF_e, ASMFMT_RRF_M    , convert_float_long_to_bfp_long_reg                  ),
 /*B352*/ GENx___x___x___ ,
 /*B353*/ GENx37Xx390x900 ( "DIEBR"     , RRF_b, ASMFMT_RRF_RM   , divide_integer_bfp_short_reg                        ),
 /*B354*/ GENx___x___x___ ,
 /*B355*/ GENx___x___x___ ,
 /*B356*/ GENx___x___x___ ,
 /*B357*/ GENx37Xx390x900 ( "FIEBR"     , RRF_e, ASMFMT_RRF_M    , load_fp_int_bfp_short_reg                           ),
 /*B358*/ GENx37Xx390x900 ( "THDER"     , RRE  , ASMFMT_RRE      , convert_bfp_short_to_float_long_reg                 ),
 /*B359*/ GENx37Xx390x900 ( "THDR"      , RRE  , ASMFMT_RRE      , convert_bfp_long_to_float_long_reg                  ),
 /*B35A*/ GENx___x___x___ ,
 /*B35B*/ GENx37Xx390x900 ( "DIDBR"     , RRF_b, ASMFMT_RRF_RM   , divide_integer_bfp_long_reg                         ),
 /*B35C*/ GENx___x___x___ ,
 /*B35D*/ GENx___x___x___ ,
 /*B35E*/ GENx___x___x___ ,
 /*B35F*/ GENx37Xx390x900 ( "FIDBR"     , RRF_e, ASMFMT_RRF_M    , load_fp_int_bfp_long_reg                            ),
 /*B360*/ GENx37Xx390x900 ( "LPXR"      , RRE  , ASMFMT_RRE      , load_positive_float_ext_reg                         ),
 /*B361*/ GENx37Xx390x900 ( "LNXR"      , RRE  , ASMFMT_RRE      , load_negative_float_ext_reg                         ),
 /*B362*/ GENx37Xx390x900 ( "LTXR"      , RRE  , ASMFMT_RRE      , load_and_test_float_ext_reg                         ),
 /*B363*/ GENx37Xx390x900 ( "LCXR"      , RRE  , ASMFMT_RRE      , load_complement_float_ext_reg                       ),
 /*B364*/ GENx___x___x___ ,
 /*B365*/ GENx37Xx390x900 ( "LXR"       , RRE  , ASMFMT_RRE      , load_float_ext_reg                                  ),
 /*B366*/ GENx37Xx390x900 ( "LEXR"      , RRE  , ASMFMT_RRE      , load_rounded_float_ext_to_short_reg                 ),
 /*B367*/ GENx37Xx390x900 ( "FIXR"      , RRE  , ASMFMT_RRE      , load_fp_int_float_ext_reg                           ),
 /*B368*/ GENx___x___x___ ,
 /*B369*/ GENx37Xx390x900 ( "CXR"       , RRE  , ASMFMT_RRE      , compare_float_ext_reg                               ),
 /*B36A*/ GENx___x___x___ ,
 /*B36B*/ GENx___x___x___ ,
 /*B36C*/ GENx___x___x___ ,
 /*B36D*/ GENx___x___x___ ,
 /*B36E*/ GENx___x___x___ ,
 /*B36F*/ GENx___x___x___ ,
 /*B370*/ GENx37Xx390x900 ( "LPDFR"     , RRE  , ASMFMT_RRE      , load_positive_fpr_long_reg                          ),
 /*B371*/ GENx37Xx390x900 ( "LNDFR"     , RRE  , ASMFMT_RRE      , load_negative_fpr_long_reg                          ),
 /*B372*/ GENx37Xx390x900 ( "CPSDR"     , RRF_b, ASMFMT_RRF_M    , copy_sign_fpr_long_reg                              ),
 /*B373*/ GENx37Xx390x900 ( "LCDFR"     , RRE  , ASMFMT_RRE      , load_complement_fpr_long_reg                        ),
 /*B374*/ GENx37Xx390x900 ( "LZER"      , RRE  , ASMFMT_RRE_R1   , load_zero_float_short_reg                           ),
 /*B375*/ GENx37Xx390x900 ( "LZDR"      , RRE  , ASMFMT_RRE_R1   , load_zero_float_long_reg                            ),
 /*B376*/ GENx37Xx390x900 ( "LZXR"      , RRE  , ASMFMT_RRE_R1   , load_zero_float_ext_reg                             ),
 /*B377*/ GENx37Xx390x900 ( "FIER"      , RRE  , ASMFMT_RRE      , load_fp_int_float_short_reg                         ),
 /*B378*/ GENx___x___x___ ,
 /*B379*/ GENx___x___x___ ,
 /*B37A*/ GENx___x___x___ ,
 /*B37B*/ GENx___x___x___ ,
 /*B37C*/ GENx___x___x___ ,
 /*B37D*/ GENx___x___x___ ,
 /*B37E*/ GENx___x___x___ ,
 /*B37F*/ GENx37Xx390x900 ( "FIDR"      , RRE  , ASMFMT_RRE      , load_fp_int_float_long_reg                          ),
 /*B380*/ GENx___x___x___ ,
 /*B381*/ GENx___x___x___ ,
 /*B382*/ GENx___x___x___ ,
 /*B383*/ GENx___x___x___ ,
 /*B384*/ GENx37Xx390x900 ( "SFPC"      , RRE  , ASMFMT_RRE_R1   , set_fpc                                             ),
 /*B385*/ GENx37Xx390x900 ( "SFASR"     , RRE  , ASMFMT_RRE_R1   , set_fpc_and_signal                                  ),
 /*B386*/ GENx___x___x___ ,
 /*B387*/ GENx___x___x___ ,
 /*B388*/ GENx___x___x___ ,
 /*B389*/ GENx___x___x___ ,
 /*B38A*/ GENx___x___x___ ,
 /*B38B*/ GENx___x___x___ ,
 /*B38C*/ GENx37Xx390x900 ( "EFPC"      , RRE  , ASMFMT_RRE_R1   , extract_fpc                                         ),
 /*B38D*/ GENx___x___x___ ,
 /*B38E*/ GENx___x___x___ ,
 /*B38F*/ GENx___x___x___ ,
 /*B390*/ GENx37Xx390x900 ( "CELFBR"    , RRF_e, ASMFMT_RRF_MM   , convert_u32_to_bfp_short_reg                        ),
 /*B391*/ GENx37Xx390x900 ( "CDLFBR"    , RRF_e, ASMFMT_RRF_MM   , convert_u32_to_bfp_long_reg                         ),
 /*B392*/ GENx37Xx390x900 ( "CXLFBR"    , RRF_e, ASMFMT_RRF_MM   , convert_u32_to_bfp_ext_reg                          ),
 /*B393*/ GENx___x___x___ ,
 /*B394*/ GENx37Xx390x900 ( "CEFBR"     , RRF_e, ASMFMT_RRE      , convert_fix32_to_bfp_short_reg                      ),
 /*B395*/ GENx37Xx390x900 ( "CDFBR"     , RRF_e, ASMFMT_RRE      , convert_fix32_to_bfp_long_reg                       ),
 /*B396*/ GENx37Xx390x900 ( "CXFBR"     , RRF_e, ASMFMT_RRE      , convert_fix32_to_bfp_ext_reg                        ),
 /*B397*/ GENx___x___x___ ,
 /*B398*/ GENx37Xx390x900 ( "CFEBR"     , RRF_e, ASMFMT_RRF_M    , convert_bfp_short_to_fix32_reg                      ),
 /*B399*/ GENx37Xx390x900 ( "CFDBR"     , RRF_e, ASMFMT_RRF_M    , convert_bfp_long_to_fix32_reg                       ),
 /*B39A*/ GENx37Xx390x900 ( "CFXBR"     , RRF_e, ASMFMT_RRF_M    , convert_bfp_ext_to_fix32_reg                        ),
 /*B39B*/ GENx___x___x___ ,
 /*B39C*/ GENx37Xx390x900 ( "CLFEBR"    , RRF_e, ASMFMT_RRF_MM   , convert_bfp_short_to_u32_reg                        ),
 /*B39D*/ GENx37Xx390x900 ( "CLFDBR"    , RRF_e, ASMFMT_RRF_MM   , convert_bfp_long_to_u32_reg                         ),
 /*B39E*/ GENx37Xx390x900 ( "CLFXBR"    , RRF_e, ASMFMT_RRF_MM   , convert_bfp_ext_to_u32_reg                          ),
 /*B39F*/ GENx___x___x___ ,
 /*B3A0*/ GENx___x___x900 ( "CELGBR"    , RRF_e, ASMFMT_RRF_MM   , convert_u64_to_bfp_short_reg                        ),
 /*B3A1*/ GENx___x___x900 ( "CDLGBR"    , RRF_e, ASMFMT_RRF_MM   , convert_u64_to_bfp_long_reg                         ),
 /*B3A2*/ GENx___x___x900 ( "CXLGBR"    , RRF_e, ASMFMT_RRF_MM   , convert_u64_to_bfp_ext_reg                          ),
 /*B3A3*/ GENx___x___x___ ,
 /*B3A4*/ GENx___x___x900 ( "CEGBR"     , RRF_e, ASMFMT_RRE      , convert_fix64_to_bfp_short_reg                      ),
 /*B3A5*/ GENx___x___x900 ( "CDGBR"     , RRF_e, ASMFMT_RRE      , convert_fix64_to_bfp_long_reg                       ),
 /*B3A6*/ GENx___x___x900 ( "CXGBR"     , RRF_e, ASMFMT_RRE      , convert_fix64_to_bfp_ext_reg                        ),
 /*B3A7*/ GENx___x___x___ ,
 /*B3A8*/ GENx___x___x900 ( "CGEBR"     , RRF_e, ASMFMT_RRF_M    , convert_bfp_short_to_fix64_reg                      ),
 /*B3A9*/ GENx___x___x900 ( "CGDBR"     , RRF_e, ASMFMT_RRF_M    , convert_bfp_long_to_fix64_reg                       ),
 /*B3AA*/ GENx___x___x900 ( "CGXBR"     , RRF_e, ASMFMT_RRF_M    , convert_bfp_ext_to_fix64_reg                        ),
 /*B3AB*/ GENx___x___x___ ,
 /*B3AC*/ GENx___x___x900 ( "CLGEBR"    , RRF_e, ASMFMT_RRF_MM   , convert_bfp_short_to_u64_reg                        ),
 /*B3AD*/ GENx___x___x900 ( "CLGDBR"    , RRF_e, ASMFMT_RRF_MM   , convert_bfp_long_to_u64_reg                         ),
 /*B3AE*/ GENx___x___x900 ( "CLGXBR"    , RRF_e, ASMFMT_RRF_MM   , convert_bfp_ext_to_u64_reg                          ),
 /*B3AF*/ GENx___x___x___ ,
 /*B3B0*/ GENx___x___x___ ,
 /*B3B1*/ GENx___x___x___ ,
 /*B3B2*/ GENx___x___x___ ,
 /*B3B3*/ GENx___x___x___ ,
 /*B3B4*/ GENx37Xx390x900 ( "CEFR"      , RRE  , ASMFMT_RRE      , convert_fixed_to_float_short_reg                    ),
 /*B3B5*/ GENx37Xx390x900 ( "CDFR"      , RRE  , ASMFMT_RRE      , convert_fixed_to_float_long_reg                     ),
 /*B3B6*/ GENx37Xx390x900 ( "CXFR"      , RRE  , ASMFMT_RRE      , convert_fixed_to_float_ext_reg                      ),
 /*B3B7*/ GENx___x___x___ ,
 /*B3B8*/ GENx37Xx390x900 ( "CFER"      , RRF_e, ASMFMT_RRF_M    , convert_float_short_to_fixed_reg                    ),
 /*B3B9*/ GENx37Xx390x900 ( "CFDR"      , RRF_e, ASMFMT_RRF_M    , convert_float_long_to_fixed_reg                     ),
 /*B3BA*/ GENx37Xx390x900 ( "CFXR"      , RRF_e, ASMFMT_RRF_M    , convert_float_ext_to_fixed_reg                      ),
 /*B3BB*/ GENx___x___x___ ,
 /*B3BC*/ GENx___x___x___ ,
 /*B3BD*/ GENx___x___x___ ,
 /*B3BE*/ GENx___x___x___ ,
 /*B3BF*/ GENx___x___x___ ,
 /*B3C0*/ GENx___x___x___ ,
 /*B3C1*/ GENx___x___x900 ( "LDGR"      , RRE  , ASMFMT_RRE      , load_fpr_from_gr_long_reg                           ),
 /*B3C2*/ GENx___x___x___ ,
 /*B3C3*/ GENx___x___x___ ,
 /*B3C4*/ GENx___x___x900 ( "CEGR"      , RRE  , ASMFMT_RRE      , convert_fix64_to_float_short_reg                    ),
 /*B3C5*/ GENx___x___x900 ( "CDGR"      , RRE  , ASMFMT_RRE      , convert_fix64_to_float_long_reg                     ),
 /*B3C6*/ GENx___x___x900 ( "CXGR"      , RRE  , ASMFMT_RRE      , convert_fix64_to_float_ext_reg                      ),
 /*B3C7*/ GENx___x___x___ ,
 /*B3C8*/ GENx___x___x900 ( "CGER"      , RRF_e, ASMFMT_RRF_M    , convert_float_short_to_fix64_reg                    ),
 /*B3C9*/ GENx___x___x900 ( "CGDR"      , RRF_e, ASMFMT_RRF_M    , convert_float_long_to_fix64_reg                     ),
 /*B3CA*/ GENx___x___x900 ( "CGXR"      , RRF_e, ASMFMT_RRF_M    , convert_float_ext_to_fix64_reg                      ),
 /*B3CB*/ GENx___x___x___ ,
 /*B3CC*/ GENx___x___x___ ,
 /*B3CD*/ GENx___x___x900 ( "LGDR"      , RRE  , ASMFMT_RRE      , load_gr_from_fpr_long_reg                           ),
 /*B3CE*/ GENx___x___x___ ,
 /*B3CF*/ GENx___x___x___ ,
 /*B3D0*/ GENx___x390x900 ( "MDTR"      , RRF_a, ASMFMT_RRR      , multiply_dfp_long_reg                               ),
 /*B3D1*/ GENx___x390x900 ( "DDTR"      , RRF_a, ASMFMT_RRR      , divide_dfp_long_reg                                 ),
 /*B3D2*/ GENx___x390x900 ( "ADTR"      , RRF_a, ASMFMT_RRR      , add_dfp_long_reg                                    ),
 /*B3D3*/ GENx___x390x900 ( "SDTR"      , RRF_a, ASMFMT_RRR      , subtract_dfp_long_reg                               ),
 /*B3D4*/ GENx___x390x900 ( "LDETR"     , RRF_d, ASMFMT_RRF_M4   , load_lengthened_dfp_short_to_long_reg               ),
 /*B3D5*/ GENx___x390x900 ( "LEDTR"     , RRF_e, ASMFMT_RRF_MM   , load_rounded_dfp_long_to_short_reg                  ),
 /*B3D6*/ GENx___x390x900 ( "LTDTR"     , RRE  , ASMFMT_RRE      , load_and_test_dfp_long_reg                          ),
 /*B3D7*/ GENx___x390x900 ( "FIDTR"     , RRF_e, ASMFMT_RRF_MM   , load_fp_int_dfp_long_reg                            ),
 /*B3D8*/ GENx___x390x900 ( "MXTR"      , RRF_a, ASMFMT_RRR      , multiply_dfp_ext_reg                                ),
 /*B3D9*/ GENx___x390x900 ( "DXTR"      , RRF_a, ASMFMT_RRR      , divide_dfp_ext_reg                                  ),
 /*B3DA*/ GENx___x390x900 ( "AXTR"      , RRF_a, ASMFMT_RRR      , add_dfp_ext_reg                                     ),
 /*B3DB*/ GENx___x390x900 ( "SXTR"      , RRF_a, ASMFMT_RRR      , subtract_dfp_ext_reg                                ),
 /*B3DC*/ GENx___x390x900 ( "LXDTR"     , RRF_d, ASMFMT_RRF_M4   , load_lengthened_dfp_long_to_ext_reg                 ),
 /*B3DD*/ GENx___x390x900 ( "LDXTR"     , RRF_e, ASMFMT_RRF_MM   , load_rounded_dfp_ext_to_long_reg                    ),
 /*B3DE*/ GENx___x390x900 ( "LTXTR"     , RRE  , ASMFMT_RRE      , load_and_test_dfp_ext_reg                           ),
 /*B3DF*/ GENx___x390x900 ( "FIXTR"     , RRF_e, ASMFMT_RRF_MM   , load_fp_int_dfp_ext_reg                             ),
 /*B3E0*/ GENx___x390x900 ( "KDTR"      , RRE  , ASMFMT_RRE      , compare_and_signal_dfp_long_reg                     ),
 /*B3E1*/ GENx___x390x900 ( "CGDTR"     , RRF_e, ASMFMT_RRF_M    , convert_dfp_long_to_fix64_reg                       ),
 /*B3E2*/ GENx___x390x900 ( "CUDTR"     , RRE  , ASMFMT_RRE      , convert_dfp_long_to_ubcd64_reg                      ),
 /*B3E3*/ GENx___x390x900 ( "CSDTR"     , RRF_d, ASMFMT_RRF_M4   , convert_dfp_long_to_sbcd64_reg                      ),
 /*B3E4*/ GENx___x390x900 ( "CDTR"      , RRE  , ASMFMT_RRE      , compare_dfp_long_reg                                ),
 /*B3E5*/ GENx___x390x900 ( "EEDTR"     , RRE  , ASMFMT_RRE      , extract_biased_exponent_dfp_long_to_fix64_reg       ),
 /*B3E6*/ GENx___x___x___ ,
 /*B3E7*/ GENx___x390x900 ( "ESDTR"     , RRE  , ASMFMT_RRE      , extract_significance_dfp_long_reg                   ),
 /*B3E8*/ GENx___x390x900 ( "KXTR"      , RRE  , ASMFMT_RRE      , compare_and_signal_dfp_ext_reg                      ),
 /*B3E9*/ GENx___x390x900 ( "CGXTR"     , RRF_e, ASMFMT_RRF_M    , convert_dfp_ext_to_fix64_reg                        ),
 /*B3EA*/ GENx___x390x900 ( "CUXTR"     , RRE  , ASMFMT_RRE      , convert_dfp_ext_to_ubcd128_reg                      ),
 /*B3EB*/ GENx___x390x900 ( "CSXTR"     , RRF_d, ASMFMT_RRF_M4   , convert_dfp_ext_to_sbcd128_reg                      ),
 /*B3EC*/ GENx___x390x900 ( "CXTR"      , RRE  , ASMFMT_RRE      , compare_dfp_ext_reg                                 ),
 /*B3ED*/ GENx___x390x900 ( "EEXTR"     , RRE  , ASMFMT_RRE      , extract_biased_exponent_dfp_ext_to_fix64_reg        ),
 /*B3EE*/ GENx___x___x___ ,
 /*B3EF*/ GENx___x390x900 ( "ESXTR"     , RRE  , ASMFMT_RRE      , extract_significance_dfp_ext_reg                    ),
 /*B3F0*/ GENx___x___x___ ,
 /*B3F1*/ GENx___x390x900 ( "CDGTR"     , RRF_e, ASMFMT_RRE      , convert_fix64_to_dfp_long_reg                       ),
 /*B3F2*/ GENx___x390x900 ( "CDUTR"     , RRE  , ASMFMT_RRE      , convert_ubcd64_to_dfp_long_reg                      ),
 /*B3F3*/ GENx___x390x900 ( "CDSTR"     , RRE  , ASMFMT_RRE      , convert_sbcd64_to_dfp_long_reg                      ),
 /*B3F4*/ GENx___x390x900 ( "CEDTR"     , RRE  , ASMFMT_RRE      , compare_exponent_dfp_long_reg                       ),
 /*B3F5*/ GENx___x390x900 ( "QADTR"     , RRF_b, ASMFMT_RRF_RM   , quantize_dfp_long_reg                               ),
 /*B3F6*/ GENx___x390x900 ( "IEDTR"     , RRF_b, ASMFMT_RRF_M    , insert_biased_exponent_fix64_to_dfp_long_reg        ),
 /*B3F7*/ GENx___x390x900 ( "RRDTR"     , RRF_b, ASMFMT_RRF_RM   , reround_dfp_long_reg                                ),
 /*B3F8*/ GENx___x___x___ ,
 /*B3F9*/ GENx___x390x900 ( "CXGTR"     , RRF_e, ASMFMT_RRE      , convert_fix64_to_dfp_ext_reg                        ),
 /*B3FA*/ GENx___x390x900 ( "CXUTR"     , RRE  , ASMFMT_RRE      , convert_ubcd128_to_dfp_ext_reg                      ),
 /*B3FB*/ GENx___x390x900 ( "CXSTR"     , RRE  , ASMFMT_RRE      , convert_sbcd128_to_dfp_ext_reg                      ),
 /*B3FC*/ GENx___x390x900 ( "CEXTR"     , RRE  , ASMFMT_RRE      , compare_exponent_dfp_ext_reg                        ),
 /*B3FD*/ GENx___x390x900 ( "QAXTR"     , RRF_b, ASMFMT_RRF_RM   , quantize_dfp_ext_reg                                ),
 /*B3FE*/ GENx___x390x900 ( "IEXTR"     , RRF_b, ASMFMT_RRF_M    , insert_biased_exponent_fix64_to_dfp_ext_reg         ),
 /*B3FF*/ GENx___x390x900 ( "RRXTR"     , RRF_b, ASMFMT_RRF_RM   , reround_dfp_ext_reg                                 )
};

static INSTR_FUNC gen_opcode_b9xx[256][NUM_INSTR_TAB_PTRS] =
{
 /*B900*/ GENx___x___x900 ( "LPGR"      , RRE  , ASMFMT_RRE      , load_positive_long_register                         ),
 /*B901*/ GENx___x___x900 ( "LNGR"      , RRE  , ASMFMT_RRE      , load_negative_long_register                         ),
 /*B902*/ GENx___x___x900 ( "LTGR"      , RRE  , ASMFMT_RRE      , load_and_test_long_register                         ),
 /*B903*/ GENx___x___x900 ( "LCGR"      , RRE  , ASMFMT_RRE      , load_complement_long_register                       ),
 /*B904*/ GENx___x___x900 ( "LGR"       , RRE  , ASMFMT_RRE      , load_long_register                                  ),
 /*B905*/ GENx___x___x900 ( "LURAG"     , RRE  , ASMFMT_RRE      , load_using_real_address_long                        ),
 /*B906*/ GENx___x___x900 ( "LGBR"      , RRE  , ASMFMT_RRE      , load_long_byte_register                             ),
 /*B907*/ GENx___x___x900 ( "LGHR"      , RRE  , ASMFMT_RRE      , load_long_halfword_register                         ),
 /*B908*/ GENx___x___x900 ( "AGR"       , RRE  , ASMFMT_RRE      , add_long_register                                   ),
 /*B909*/ GENx___x___x900 ( "SGR"       , RRE  , ASMFMT_RRE      , subtract_long_register                              ),
 /*B90A*/ GENx___x___x900 ( "ALGR"      , RRE  , ASMFMT_RRE      , add_logical_long_register                           ),
 /*B90B*/ GENx___x___x900 ( "SLGR"      , RRE  , ASMFMT_RRE      , subtract_logical_long_register                      ),
 /*B90C*/ GENx___x___x900 ( "MSGR"      , RRE  , ASMFMT_RRE      , multiply_single_long_register                       ),
 /*B90D*/ GENx___x___x900 ( "DSGR"      , RRE  , ASMFMT_RRE      , divide_single_long_register                         ),
 /*B90E*/ GENx___x___x900 ( "EREGG"     , RRE  , ASMFMT_RRE      , extract_stacked_registers_long                      ),
 /*B90F*/ GENx___x___x900 ( "LRVGR"     , RRE  , ASMFMT_RRE      , load_reversed_long_register                         ),
 /*B910*/ GENx___x___x900 ( "LPGFR"     , RRE  , ASMFMT_RRE      , load_positive_long_fullword_register                ),
 /*B911*/ GENx___x___x900 ( "LNGFR"     , RRE  , ASMFMT_RRE      , load_negative_long_fullword_register                ),
 /*B912*/ GENx___x___x900 ( "LTGFR"     , RRE  , ASMFMT_RRE      , load_and_test_long_fullword_register                ),
 /*B913*/ GENx___x___x900 ( "LCGFR"     , RRE  , ASMFMT_RRE      , load_complement_long_fullword_register              ),
 /*B914*/ GENx___x___x900 ( "LGFR"      , RRE  , ASMFMT_RRE      , load_long_fullword_register                         ),
 /*B915*/ GENx___x___x___ ,
 /*B916*/ GENx___x___x900 ( "LLGFR"     , RRE  , ASMFMT_RRE      , load_logical_long_fullword_register                 ),
 /*B917*/ GENx___x___x900 ( "LLGTR"     , RRE  , ASMFMT_RRE      , load_logical_long_thirtyone_register                ),
 /*B918*/ GENx___x___x900 ( "AGFR"      , RRE  , ASMFMT_RRE      , add_long_fullword_register                          ),
 /*B919*/ GENx___x___x900 ( "SGFR"      , RRE  , ASMFMT_RRE      , subtract_long_fullword_register                     ),
 /*B91A*/ GENx___x___x900 ( "ALGFR"     , RRE  , ASMFMT_RRE      , add_logical_long_fullword_register                  ),
 /*B91B*/ GENx___x___x900 ( "SLGFR"     , RRE  , ASMFMT_RRE      , subtract_logical_long_fullword_register             ),
 /*B91C*/ GENx___x___x900 ( "MSGFR"     , RRE  , ASMFMT_RRE      , multiply_single_long_fullword_register              ),
 /*B91D*/ GENx___x___x900 ( "DSGFR"     , RRE  , ASMFMT_RRE      , divide_single_long_fullword_register                ),
 /*B91E*/ GENx37Xx390x900 ( "KMAC"      , RRE  , ASMFMT_RRE      , compute_message_authentication_code                 ),
 /*B91F*/ GENx37Xx390x900 ( "LRVR"      , RRE  , ASMFMT_RRE      , load_reversed_register                              ),
 /*B920*/ GENx___x___x900 ( "CGR"       , RRE  , ASMFMT_RRE      , compare_long_register                               ),
 /*B921*/ GENx___x___x900 ( "CLGR"      , RRE  , ASMFMT_RRE      , compare_logical_long_register                       ),
 /*B922*/ GENx___x___x___ ,
 /*B923*/ GENx___x___x___ ,
 /*B924*/ GENx___x___x___ ,
 /*B925*/ GENx___x___x900 ( "STURG"     , RRE  , ASMFMT_RRE      , store_using_real_address_long                       ),
 /*B926*/ GENx37Xx390x900 ( "LBR"       , RRE  , ASMFMT_RRE      , load_byte_register                                  ),
 /*B927*/ GENx37Xx390x900 ( "LHR"       , RRE  , ASMFMT_RRE      , load_halfword_register                              ),
 /*B928*/ GENx37Xx390x900 ( "PCKMO"     , RRE  , ASMFMT_RRE      , perform_cryptographic_key_management_operation      ),
 /*B929*/ GENx___x___x___ ,
 /*B92A*/ GENx37Xx390x900 ( "KMF"       , RRE  , ASMFMT_RRE      , cipher_message_with_cipher_feedback                 ),
 /*B92B*/ GENx37Xx390x900 ( "KMO"       , RRE  , ASMFMT_RRE      , cipher_message_with_output_feedback                 ),
 /*B92C*/ GENx37Xx390x900 ( "PCC"       , RRE  , ASMFMT_none     , perform_cryptographic_computation                   ),
 /*B92D*/ GENx37Xx390x900 ( "KMCTR"     , RRF_b, ASMFMT_RRF_M    , cipher_message_with_counter                         ),
 /*B92E*/ GENx37Xx390x900 ( "KM"        , RRE  , ASMFMT_RRE      , cipher_message                                      ),
 /*B92F*/ GENx37Xx390x900 ( "KMC"       , RRE  , ASMFMT_RRE      , cipher_message_with_chaining                        ),
 /*B930*/ GENx___x___x900 ( "CGFR"      , RRE  , ASMFMT_RRE      , compare_long_fullword_register                      ),
 /*B931*/ GENx___x___x900 ( "CLGFR"     , RRE  , ASMFMT_RRE      , compare_logical_long_fullword_register              ),
 /*B932*/ GENx___x___x___ ,
 /*B933*/ GENx___x___x___ ,
 /*B934*/ GENx___x___x___ ,
 /*B935*/ GENx___x___x___ ,
 /*B936*/ GENx___x___x___ ,
 /*B937*/ GENx___x___x___ ,
 /*B938*/ GENx___x___x___ ,
 /*B939*/ GENx___x___x___ ,
 /*B93A*/ GENx___x___x___ ,
 /*B93B*/ GENx___x___x___ ,
 /*B93C*/ GENx___x___x___ ,
 /*B93D*/ GENx___x___x___ ,
 /*B93E*/ GENx37Xx390x900 ( "KIMD"      , RRE  , ASMFMT_RRE      , compute_intermediate_message_digest                 ),
 /*B93F*/ GENx37Xx390x900 ( "KLMD"      , RRE  , ASMFMT_RRE      , compute_last_message_digest                         ),
 /*B940*/ GENx___x___x___ ,
 /*B941*/ GENx___x390x900 ( "CFDTR"     , RRF_e, ASMFMT_RRF_MM   , convert_dfp_long_to_fix32_reg                       ),
 /*B942*/ GENx___x___x900 ( "CLGDTR"    , RRF_e, ASMFMT_RRF_MM   , convert_dfp_long_to_u64_reg                         ),
 /*B943*/ GENx___x390x900 ( "CLFDTR"    , RRF_e, ASMFMT_RRF_MM   , convert_dfp_long_to_u32_reg                         ),
 /*B944*/ GENx___x___x___ ,
 /*B945*/ GENx___x___x___ ,
 /*B946*/ GENx___x___x900 ( "BCTGR"     , RRE  , ASMFMT_RRE      , branch_on_count_long_register                       ),
 /*B947*/ GENx___x___x___ ,
 /*B948*/ GENx___x___x___ ,
 /*B949*/ GENx___x390x900 ( "CFXTR"     , RRF_e, ASMFMT_RRF_MM   , convert_dfp_ext_to_fix32_reg                        ),
 /*B94A*/ GENx___x___x900 ( "CLGXTR"    , RRF_e, ASMFMT_RRF_MM   , convert_dfp_ext_to_u64_reg                          ),
 /*B94B*/ GENx___x390x900 ( "CLFXTR"    , RRF_e, ASMFMT_RRF_MM   , convert_dfp_ext_to_u32_reg                          ),
 /*B94C*/ GENx___x___x___ ,
 /*B94D*/ GENx___x___x___ ,
 /*B94E*/ GENx___x___x___ ,
 /*B94F*/ GENx___x___x___ ,
 /*B950*/ GENx___x___x___ ,
 /*B951*/ GENx___x390x900 ( "CDFTR"     , RRE  , ASMFMT_RRF_MM   , convert_fix32_to_dfp_long_reg                       ),
 /*B952*/ GENx___x___x900 ( "CDLGTR"    , RRF_e, ASMFMT_RRF_MM   , convert_u64_to_dfp_long_reg                         ),
 /*B953*/ GENx___x390x900 ( "CDLFTR"    , RRF_e, ASMFMT_RRF_MM   , convert_u32_to_dfp_long_reg                         ),
 /*B954*/ GENx___x___x___ ,
 /*B955*/ GENx___x___x___ ,
 /*B956*/ GENx___x___x___ ,
 /*B957*/ GENx___x___x___ ,
 /*B958*/ GENx___x___x___ ,
 /*B959*/ GENx___x390x900 ( "CXFTR"     , RRE  , ASMFMT_RRF_MM   , convert_fix32_to_dfp_ext_reg                        ),
 /*B95A*/ GENx___x___x900 ( "CXLGTR"    , RRF_e, ASMFMT_RRF_MM   , convert_u64_to_dfp_ext_reg                          ),
 /*B95B*/ GENx___x390x900 ( "CXLFTR"    , RRF_e, ASMFMT_RRF_MM   , convert_u32_to_dfp_ext_reg                          ),
 /*B95C*/ GENx___x___x___ ,
 /*B95D*/ GENx___x___x___ ,
 /*B95E*/ GENx___x___x___ ,
 /*B95F*/ GENx___x___x___ ,
 /*B960*/ GENx___x___x900 ( "CGRT"      , RRF_c, ASMFMT_RRF_M3   , compare_and_trap_long_register                      ),
 /*B961*/ GENx___x___x900 ( "CLGRT"     , RRF_c, ASMFMT_RRF_M3   , compare_logical_and_trap_long_register              ),
 /*B962*/ GENx___x___x___ ,
 /*B963*/ GENx___x___x___ ,
 /*B964*/ GENx___x___x900 ( "NNGRK"     , RRF_a, ASMFMT_RRR      , nand_register_long                                  ),
 /*B965*/ GENx___x___x900 ( "OCGRK"     , RRF_a, ASMFMT_RRR      , or_register_long_with_complement                    ),
 /*B966*/ GENx___x___x900 ( "NOGRK"     , RRF_a, ASMFMT_RRR      , nor_register_long                                   ),
 /*B967*/ GENx___x___x900 ( "NXGRK"     , RRF_a, ASMFMT_RRR      , not_xor_register_long                               ),
 /*B968*/ GENx___x___x___ ,
 /*B969*/ GENx___x___x___ ,
 /*B96A*/ GENx___x___x___ ,
 /*B96B*/ GENx___x___x___ ,
 /*B96C*/ GENx___x___x___ ,
 /*B96D*/ GENx___x___x___ ,
 /*B96E*/ GENx___x___x___ ,
 /*B96F*/ GENx___x___x___ ,
 /*B970*/ GENx___x___x___ ,
 /*B971*/ GENx___x___x___ ,
 /*B972*/ GENx37Xx390x900 ( "CRT"       , RRF_c, ASMFMT_RRF_M3   , compare_and_trap_register                           ),
 /*B973*/ GENx37Xx390x900 ( "CLRT"      , RRF_c, ASMFMT_RRF_M3   , compare_logical_and_trap_register                   ),
 /*B974*/ GENx___x___x900 ( "NNRK"      , RRF_a, ASMFMT_RRR      , nand_register                                       ),
 /*B975*/ GENx___x___x900 ( "OCRK"      , RRF_a, ASMFMT_RRR      , or_register_with_complement                         ),
 /*B976*/ GENx___x___x900 ( "NORK"      , RRF_a, ASMFMT_RRR      , nor_register                                        ),
 /*B977*/ GENx___x___x900 ( "NXRK"      , RRF_a, ASMFMT_RRR      , not_xor_register                                    ),
 /*B978*/ GENx___x___x___ ,
 /*B979*/ GENx___x___x___ ,
 /*B97A*/ GENx___x___x___ ,
 /*B97B*/ GENx___x___x___ ,
 /*B97C*/ GENx___x___x___ ,
 /*B97D*/ GENx___x___x___ ,
 /*B97E*/ GENx___x___x___ ,
 /*B97F*/ GENx___x___x___ ,
 /*B980*/ GENx___x___x900 ( "NGR"       , RRE  , ASMFMT_RRE      , and_long_register                                   ),
 /*B981*/ GENx___x___x900 ( "OGR"       , RRE  , ASMFMT_RRE      , or_long_register                                    ),
 /*B982*/ GENx___x___x900 ( "XGR"       , RRE  , ASMFMT_RRE      , exclusive_or_long_register                          ),
 /*B983*/ GENx___x___x900 ( "FLOGR"     , RRE  , ASMFMT_RRE      , find_leftmost_one_long_register                     ),
 /*B984*/ GENx___x___x900 ( "LLGCR"     , RRE  , ASMFMT_RRE      , load_logical_long_character_register                ),
 /*B985*/ GENx___x___x900 ( "LLGHR"     , RRE  , ASMFMT_RRE      , load_logical_long_halfword_register                 ),
 /*B986*/ GENx___x___x900 ( "MLGR"      , RRE  , ASMFMT_RRE      , multiply_logical_long_register                      ),
 /*B987*/ GENx___x___x900 ( "DLGR"      , RRE  , ASMFMT_RRE      , divide_logical_long_register                        ),
 /*B988*/ GENx___x___x900 ( "ALCGR"     , RRE  , ASMFMT_RRE      , add_logical_carry_long_register                     ),
 /*B989*/ GENx___x___x900 ( "SLBGR"     , RRE  , ASMFMT_RRE      , subtract_logical_borrow_long_register               ),
 /*B98A*/ GENx___x___x900 ( "CSPG"      , RRE  , ASMFMT_RRE      , compare_and_swap_and_purge_long                     ),
 /*B98B*/ GENx___x___x___ ,
 /*B98C*/ GENx___x___x___ ,
 /*B98D*/ GENx37Xx390x900 ( "EPSW"      , RRE  , ASMFMT_RRE      , extract_psw                                         ),
 /*B98E*/ GENx___x___x900 ( "IDTE"      , RRF_b, ASMFMT_RRF_R    , invalidate_dat_table_entry                          ),
 /*B98F*/ GENx___x___x___ ,
 /*B990*/ GENx37Xx390x900 ( "TRTT"      , RRF_c, ASMFMT_RRF_M3   , translate_two_to_two                                ),
 /*B991*/ GENx37Xx390x900 ( "TRTO"      , RRF_c, ASMFMT_RRF_M3   , translate_two_to_one                                ),
 /*B992*/ GENx37Xx390x900 ( "TROT"      , RRF_c, ASMFMT_RRF_M3   , translate_one_to_two                                ),
 /*B993*/ GENx37Xx390x900 ( "TROO"      , RRF_c, ASMFMT_RRF_M3   , translate_one_to_one                                ),
 /*B994*/ GENx37Xx390x900 ( "LLCR"      , RRE  , ASMFMT_RRE      , load_logical_character_register                     ),
 /*B995*/ GENx37Xx390x900 ( "LLHR"      , RRE  , ASMFMT_RRE      , load_logical_halfword_register                      ),
 /*B996*/ GENx37Xx390x900 ( "MLR"       , RRE  , ASMFMT_RRE      , multiply_logical_register                           ),
 /*B997*/ GENx37Xx390x900 ( "DLR"       , RRE  , ASMFMT_RRE      , divide_logical_register                             ),
 /*B998*/ GENx37Xx390x900 ( "ALCR"      , RRE  , ASMFMT_RRE      , add_logical_carry_register                          ),
 /*B999*/ GENx37Xx390x900 ( "SLBR"      , RRE  , ASMFMT_RRE      , subtract_logical_borrow_register                    ),
 /*B99A*/ GENx___x___x900 ( "EPAIR"     , RRE  , ASMFMT_RRE_R1   , extract_primary_asn_and_instance                    ),
 /*B99B*/ GENx___x___x900 ( "ESAIR"     , RRE  , ASMFMT_RRE_R1   , extract_secondary_asn_and_instance                  ),
 /*B99C*/ GENx___x___x900 ( "EQBS"      , RRF_a, ASMFMT_RRF_RM   , extract_queue_buffer_state                          ),
 /*B99D*/ GENx___x___x900 ( "ESEA"      , RRE  , ASMFMT_RRE_R1   , extract_and_set_extended_authority                  ),
 /*B99E*/ GENx___x___x900 ( "PTI"       , RRE  , ASMFMT_RRE      , program_transfer_with_instance                      ),
 /*B99F*/ GENx___x___x900 ( "SSAIR"     , RRE  , ASMFMT_RRE_R1   , set_secondary_asn_with_instance                     ),
 /*B9A0*/ GENx___x___x___ ,
 /*B9A1*/ GENx___x___x___ ,
 /*B9A2*/ GENx___x___x900 ( "PTF"       , RRE  , ASMFMT_RRE      , perform_topology_function                           ),
 /*B9A3*/ GENx___x___x___ ,
 /*B9B9*/ GENx___x___x___ ,
 /*B9A5*/ GENx___x___x___ ,
 /*B9A6*/ GENx___x___x___ ,
 /*B9A7*/ GENx___x___x___ ,
 /*B9A8*/ GENx___x___x___ ,
 /*B9A9*/ GENx___x___x___ ,
 /*B9AA*/ GENx___x___x900 ( "LPTEA"     , RRF_b, ASMFMT_RRF_RM   , load_page_table_entry_address                       ),
 /*B9AB*/ GENx___x___x900 ( "ESSA "     , RRF_c, ASMFMT_RRF_M    , extract_and_set_storage_attributes                  ),
 /*B9AC*/ GENx___x___x900 ( "IRBM"      , RRE  , ASMFMT_RRE      , insert_reference_bits_multiple                      ),
 /*B9AD*/ GENx___x___x___ ,
 /*B9AE*/ GENx___x___x900 ( "RRBM"      , RRE  , ASMFMT_RRE      , reset_reference_bits_multiple                       ),
 /*B9AF*/ GENx___x___x900 ( "PFMF"      , RRE  , ASMFMT_RRE      , perform_frame_management_function                   ),
 /*B9B0*/ GENx37Xx390x900 ( "CU14"      , RRF_c, ASMFMT_RRF_M3   , convert_utf8_to_utf32                               ),
 /*B9B1*/ GENx37Xx390x900 ( "CU24"      , RRF_c, ASMFMT_RRF_M3   , convert_utf16_to_utf32                              ),
 /*B9B2*/ GENx37Xx390x900 ( "CU41"      , RRE  , ASMFMT_RRE      , convert_utf32_to_utf8                               ),
 /*B9B3*/ GENx37Xx390x900 ( "CU42"      , RRE  , ASMFMT_RRE      , convert_utf32_to_utf16                              ),
 /*B9B4*/ GENx___x___x___ ,
 /*B9B5*/ GENx___x___x___ ,
 /*B9B6*/ GENx___x___x___ ,
 /*B9B7*/ GENx___x___x___ ,
 /*B9B8*/ GENx___x___x___ ,
 /*B9B9*/ GENx___x___x___ ,
 /*B9BA*/ GENx___x___x___ ,
 /*B9BB*/ GENx___x___x___ ,
 /*B9BC*/ GENx___x___x___ ,
 /*B9BD*/ GENx37Xx390x900 ( "TRTRE"     , RRF_c, ASMFMT_RRF_M3   , translate_and_test_reverse_extended                 ),
 /*B9BE*/ GENx37Xx390x900 ( "SRSTU"     , RRE  , ASMFMT_RRE      , search_string_unicode                               ),
 /*B9BF*/ GENx37Xx390x900 ( "TRTE"      , RRF_c, ASMFMT_RRF_M3   , translate_and_test_extended                         ),
 /*B9C0*/ GENx___x___x900 ( "SELFHR"    , RRF_a, ASMFMT_RRF_A    , select_fullword_high_register                       ),
 /*B9C1*/ GENx___x___x___ ,
 /*B9C2*/ GENx___x___x___ ,
 /*B9C3*/ GENx___x___x___ ,
 /*B9C4*/ GENx___x___x___ ,
 /*B9C5*/ GENx___x___x___ ,
 /*B9C6*/ GENx___x___x___ ,
 /*B9C7*/ GENx___x___x___ ,
 /*B9C8*/ GENx___x___x900 ( "AHHHR"     , RRF_a, ASMFMT_RRF_M3   , add_high_high_high_register                         ),
 /*B9C9*/ GENx___x___x900 ( "SHHHR"     , RRF_a, ASMFMT_RRF_M3   , subtract_high_high_high_register                    ),
 /*B9CA*/ GENx___x___x900 ( "ALHHHR"    , RRF_a, ASMFMT_RRF_M3   , add_logical_high_high_high_register                 ),
 /*B9CB*/ GENx___x___x900 ( "SLHHHR"    , RRF_a, ASMFMT_RRF_M3   , subtract_logical_high_high_high_register            ),
 /*B9CC*/ GENx___x___x___ ,
 /*B9CD*/ GENx___x___x900 ( "CHHR"      , RRE  , ASMFMT_RRE      , compare_high_high_register                          ),
 /*B9CE*/ GENx___x___x___ ,
 /*B9CF*/ GENx___x___x900 ( "CLHHR"     , RRE  , ASMFMT_RRE      , compare_logical_high_high_register                  ),
 /*B9D0*/ GENx___x___x___ ,
 /*B9D1*/ GENx___x___x___ ,
 /*B9D2*/ GENx___x___x___ ,
 /*B9D3*/ GENx___x___x___ ,
 /*B9D4*/ GENx___x___x___ ,
 /*B9D5*/ GENx___x___x___ ,
 /*B9D6*/ GENx___x___x___ ,
 /*B9D7*/ GENx___x___x___ ,
 /*B9D8*/ GENx___x___x900 ( "AHHLR"     , RRF_a, ASMFMT_RRF_M3   , add_high_high_low_register                          ),
 /*B9D9*/ GENx___x___x900 ( "SHHLR"     , RRF_a, ASMFMT_RRF_M3   , subtract_high_high_low_register                     ),
 /*B9DA*/ GENx___x___x900 ( "ALHHLR"    , RRF_a, ASMFMT_RRF_M3   , add_logical_high_high_low_register                  ),
 /*B9DB*/ GENx___x___x900 ( "SLHHLR"    , RRF_a, ASMFMT_RRF_M3   , subtract_logical_high_high_low_register             ),
 /*B9DC*/ GENx___x___x___ ,
 /*B9DD*/ GENx___x___x900 ( "CHLR"      , RRE  , ASMFMT_RRE      , compare_high_low_register                           ),
 /*B9DE*/ GENx___x___x___ ,
 /*B9DF*/ GENx___x___x900 ( "CLHLR"     , RRE  , ASMFMT_RRE      , compare_logical_high_low_register                   ),
 /*B9E0*/ GENx___x___x900 ( "LOCFHR"    , RRF_c, ASMFMT_RRF_M3   , load_high_on_condition_register                     ),
 /*B9E1*/ GENx___x___x900 ( "POPCNT"    , RRF_c, ASMFMT_RRE      , population_count                                    ),
 /*B9E2*/ GENx___x___x900 ( "LOCGR"     , RRF_c, ASMFMT_RRF_M3   , load_on_condition_long_register                     ),
 /*B9E3*/ GENx___x___x900 ( "SELGR"     , RRF_a, ASMFMT_RRF_A    , select_register_long                                ),
 /*B9E4*/ GENx___x___x900 ( "NGRK"      , RRF_a, ASMFMT_RRR      , and_distinct_long_register                          ),
 /*B9E5*/ GENx___x___x900 ( "NCGRK"     , RRF_a, ASMFMT_RRR      , and_register_long_with_complement                                ),
 /*B9E6*/ GENx___x___x900 ( "OGRK"      , RRF_a, ASMFMT_RRR      , or_distinct_long_register                           ),
 /*B9E7*/ GENx___x___x900 ( "XGRK"      , RRF_a, ASMFMT_RRR      , exclusive_or_distinct_long_register                 ),
 /*B9E8*/ GENx___x___x900 ( "AGRK"      , RRF_a, ASMFMT_RRR      , add_distinct_long_register                          ),
 /*B9E9*/ GENx___x___x900 ( "SGRK"      , RRF_a, ASMFMT_RRR      , subtract_distinct_long_register                     ),
 /*B9EA*/ GENx___x___x900 ( "ALGRK"     , RRF_a, ASMFMT_RRR      , add_logical_distinct_long_register                  ),
 /*B9EB*/ GENx___x___x900 ( "SLGRK"     , RRF_a, ASMFMT_RRR      , subtract_logical_distinct_long_register             ),
 /*B9EC*/ GENx___x___x900 ( "MGRK"      , RRF_a, ASMFMT_RRR      , multiply_long_register                              ),
 /*B9ED*/ GENx___x___x900 ( "MSGRKC"    , RRF_a, ASMFMT_RRR      , multiply_single_long_register_cc                    ),
 /*B9EE*/ GENx___x___x___ ,
 /*B9EF*/ GENx___x___x___ ,
 /*B9F0*/ GENx___x___x900 ( "SELR"      , RRF_a, ASMFMT_RRF_A    , select_register                                     ),
 /*B9F1*/ GENx___x___x___ ,
 /*B9F2*/ GENx37Xx390x900 ( "LOCR"      , RRF_c, ASMFMT_RRF_M3   , load_on_condition_register                          ),
 /*B9F3*/ GENx___x___x___ ,
 /*B9F4*/ GENx37Xx390x900 ( "NRK"       , RRF_a, ASMFMT_RRR      , and_distinct_register                               ),
 /*B9F5*/ GENx___x___x900 ( "NCRK"      , RRF_a, ASMFMT_RRR      , and_register_with_complement                        ),
 /*B9F6*/ GENx37Xx390x900 ( "ORK"       , RRF_a, ASMFMT_RRR      , or_distinct_register                                ),
 /*B9F7*/ GENx37Xx390x900 ( "XRK"       , RRF_a, ASMFMT_RRR      , exclusive_or_distinct_register                      ),
 /*B9F8*/ GENx37Xx390x900 ( "ARK"       , RRF_a, ASMFMT_RRR      , add_distinct_register                               ),
 /*B9F9*/ GENx37Xx390x900 ( "SRK"       , RRF_a, ASMFMT_RRR      , subtract_distinct_register                          ),
 /*B9FA*/ GENx37Xx390x900 ( "ALRK"      , RRF_a, ASMFMT_RRR      , add_logical_distinct_register                       ),
 /*B9FB*/ GENx37Xx390x900 ( "SLRK"      , RRF_a, ASMFMT_RRR      , subtract_logical_distinct_register                  ),
 /*B9FC*/ GENx___x___x___ ,
 /*B9FD*/ GENx___x___x900 ( "MSRKC"     , RRF_a, ASMFMT_RRR      , multiply_single_register_cc                         ),
 /*B9FE*/ GENx___x___x___ ,
 /*B9FF*/ GENx___x___x___
};

static INSTR_FUNC gen_opcode_c0_x[16][NUM_INSTR_TAB_PTRS] =
{
 /*C0x0*/ GENx37Xx390x900 ( "LARL"      , RIL_b, ASMFMT_RIL_A    , load_address_relative_long                          ),
 /*C0x1*/ GENx___x___x900 ( "LGFI"      , RIL_a, ASMFMT_RIL      , load_long_fullword_immediate                        ),
 /*C0x2*/ GENx___x___x___ ,
 /*C0x3*/ GENx___x___x___ ,
 /*C0x4*/ GENx37Xx390x900 ( "BRCL"      , RIL_c, ASMFMT_RIL_A    , branch_relative_on_condition_long                   ),
 /*C0x5*/ GENx37Xx390x900 ( "BRASL"     , RIL_b, ASMFMT_RIL_A    , branch_relative_and_save_long                       ),
 /*C0x6*/ GENx___x___x900 ( "XIHF"      , RIL_a, ASMFMT_RIL      , exclusive_or_immediate_high_fullword                ),
 /*C0x7*/ GENx37Xx___x900 ( "XILF"      , RIL_a, ASMFMT_RIL      , exclusive_or_immediate_low_fullword                 ),
 /*C0x8*/ GENx___x___x900 ( "IIHF"      , RIL_a, ASMFMT_RIL      , insert_immediate_high_fullword                      ),
 /*C0x9*/ GENx37Xx___x900 ( "IILF"      , RIL_a, ASMFMT_RIL      , insert_immediate_low_fullword                       ),
 /*C0xA*/ GENx___x___x900 ( "NIHF"      , RIL_a, ASMFMT_RIL      , and_immediate_high_fullword                         ),
 /*C0xB*/ GENx37Xx___x900 ( "NILF"      , RIL_a, ASMFMT_RIL      , and_immediate_low_fullword                          ),
 /*C0xC*/ GENx___x___x900 ( "OIHF"      , RIL_a, ASMFMT_RIL      , or_immediate_high_fullword                          ),
 /*C0xD*/ GENx37Xx___x900 ( "OILF"      , RIL_a, ASMFMT_RIL      , or_immediate_low_fullword                           ),
 /*C0xE*/ GENx___x___x900 ( "LLIHF"     , RIL_a, ASMFMT_RIL      , load_logical_immediate_high_fullword                ),
 /*C0xF*/ GENx37Xx___x900 ( "LLILF"     , RIL_a, ASMFMT_RIL      , load_logical_immediate_low_fullword                 )
};

static INSTR_FUNC gen_opcode_c2_x[16][NUM_INSTR_TAB_PTRS] =
{
 /*C2x0*/ GENx___x___x900 ( "MSGFI"     , RIL_a, ASMFMT_RIL      , multiply_single_immediate_long_fullword             ),
 /*C2x1*/ GENx37Xx390x900 ( "MSFI"      , RIL_a, ASMFMT_RIL      , multiply_single_immediate_fullword                  ),
 /*C2x2*/ GENx___x___x___ ,
 /*C2x3*/ GENx___x___x___ ,
 /*C2x4*/ GENx___x___x900 ( "SLGFI"     , RIL_a, ASMFMT_RIL      , subtract_logical_long_fullword_immediate            ),
 /*C2x5*/ GENx37Xx390x900 ( "SLFI"      , RIL_a, ASMFMT_RIL      , subtract_logical_fullword_immediate                 ),
 /*C2x6*/ GENx___x___x___ ,
 /*C2x7*/ GENx___x___x___ ,
 /*C2x8*/ GENx___x___x900 ( "AGFI"      , RIL_a, ASMFMT_RIL      , add_long_fullword_immediate                         ),
 /*C2x9*/ GENx37Xx390x900 ( "AFI"       , RIL_a, ASMFMT_RIL      , add_fullword_immediate                              ),
 /*C2xA*/ GENx___x___x900 ( "ALGFI"     , RIL_a, ASMFMT_RIL      , add_logical_long_fullword_immediate                 ),
 /*C2xB*/ GENx37Xx390x900 ( "ALFI"      , RIL_a, ASMFMT_RIL      , add_logical_fullword_immediate                      ),
 /*C2xC*/ GENx___x___x900 ( "CGFI"      , RIL_a, ASMFMT_RIL      , compare_long_fullword_immediate                     ),
 /*C2xD*/ GENx37Xx390x900 ( "CFI"       , RIL_a, ASMFMT_RIL      , compare_fullword_immediate                          ),
 /*C2xE*/ GENx___x___x900 ( "CLGFI"     , RIL_a, ASMFMT_RIL      , compare_logical_long_fullword_immediate             ),
 /*C2xF*/ GENx37Xx390x900 ( "CLFI"      , RIL_a, ASMFMT_RIL      , compare_logical_fullword_immediate                  )
};

static INSTR_FUNC gen_opcode_c4_x[16][NUM_INSTR_TAB_PTRS] =
{
 /*C4x0*/ GENx___x___x___ ,
 /*C4x1*/ GENx___x___x___ ,
 /*C4x2*/ GENx37Xx390x900 ( "LLHRL"     , RIL_b, ASMFMT_RIL_A    , load_logical_halfword_relative_long                 ),
 /*C4x3*/ GENx___x___x___ ,
 /*C4x4*/ GENx___x___x900 ( "LGHRL"     , RIL_b, ASMFMT_RIL_A    , load_halfword_relative_long_long                    ),
 /*C4x5*/ GENx37Xx390x900 ( "LHRL"      , RIL_b, ASMFMT_RIL_A    , load_halfword_relative_long                         ),
 /*C4x6*/ GENx___x___x900 ( "LLGHRL"    , RIL_b, ASMFMT_RIL_A    , load_logical_halfword_relative_long_long            ),
 /*C4x7*/ GENx37Xx390x900 ( "STHRL"     , RIL_b, ASMFMT_RIL_A    , store_halfword_relative_long                        ),
 /*C4x8*/ GENx___x___x900 ( "LGRL"      , RIL_b, ASMFMT_RIL_A    , load_relative_long_long                             ),
 /*C4x9*/ GENx___x___x___ ,
 /*C4xA*/ GENx___x___x___ ,
 /*C4xB*/ GENx___x___x900 ( "STGRL"     , RIL_b, ASMFMT_RIL_A    , store_relative_long_long                            ),
 /*C4xC*/ GENx___x___x900 ( "LGFRL"     , RIL_b, ASMFMT_RIL_A    , load_relative_long_long_fullword                    ),
 /*C4xD*/ GENx37Xx390x900 ( "LRL"       , RIL_b, ASMFMT_RIL_A    , load_relative_long                                  ),
 /*C4xE*/ GENx___x___x900 ( "LLGFRL"    , RIL_b, ASMFMT_RIL_A    , load_logical_relative_long_long_fullword            ),
 /*C4xF*/ GENx37Xx390x900 ( "STRL"      , RIL_b, ASMFMT_RIL_A    , store_relative_long                                 )
};

static INSTR_FUNC gen_opcode_c6_x[16][NUM_INSTR_TAB_PTRS] =
{
 /*C6x0*/ GENx37Xx390x900 ( "EXRL"      , RIL_b, ASMFMT_RIL_A    , execute_relative_long                               ),
 /*C6x1*/ GENx___x___x___ ,
 /*C6x2*/ GENx37Xx390x900 ( "PFDRL"     , RIL_c, ASMFMT_RIL_A    , prefetch_data_relative_long                         ),
 /*C6x3*/ GENx___x___x___ ,
 /*C6x4*/ GENx___x___x900 ( "CGHRL"     , RIL_b, ASMFMT_RIL_A    , compare_halfword_relative_long_long                 ),
 /*C6x5*/ GENx37Xx390x900 ( "CHRL"      , RIL_b, ASMFMT_RIL_A    , compare_halfword_relative_long                      ),
 /*C6x6*/ GENx___x___x900 ( "CLGHRL"    , RIL_b, ASMFMT_RIL_A    , compare_logical_relative_long_long_halfword         ),
 /*C6x7*/ GENx37Xx390x900 ( "CLHRL"     , RIL_b, ASMFMT_RIL_A    , compare_logical_relative_long_halfword              ),
 /*C6x8*/ GENx___x___x900 ( "CGRL"      , RIL_b, ASMFMT_RIL_A    , compare_relative_long_long                          ),
 /*C6x9*/ GENx___x___x___ ,
 /*C6xA*/ GENx___x___x900 ( "CLGRL"     , RIL_b, ASMFMT_RIL_A    , compare_logical_relative_long_long                  ),
 /*C6xB*/ GENx___x___x___ ,
 /*C6xC*/ GENx___x___x900 ( "CGFRL"     , RIL_b, ASMFMT_RIL_A    , compare_relative_long_long_fullword                 ),
 /*C6xD*/ GENx37Xx390x900 ( "CRL"       , RIL_b, ASMFMT_RIL_A    , compare_relative_long                               ),
 /*C6xE*/ GENx___x___x900 ( "CLGFRL"    , RIL_b, ASMFMT_RIL_A    , compare_logical_relative_long_long_fullword         ),
 /*C6xF*/ GENx37Xx390x900 ( "CLRL"      , RIL_b, ASMFMT_RIL_A    , compare_logical_relative_long                       )
};

static INSTR_FUNC gen_opcode_c8_x[16][NUM_INSTR_TAB_PTRS] =
{
 /*C8x0*/ GENx___x___x900 ( "MVCOS"     , SSF  , ASMFMT_SSF      , move_with_optional_specifications                   ),
 /*C8x1*/ GENx___x___x900 ( "ECTG"      , SSF  , ASMFMT_SSF      , extract_cpu_time                                    ),
 /*C8x2*/ GENx37Xx___x900 ( "CSST"      , SSF  , ASMFMT_SSF      , compare_and_swap_and_store                          ),
 /*C8x3*/ GENx___x___x___ ,
 /*C8x4*/ GENx37Xx390x900 ( "LPD"       , SSF  , ASMFMT_SSF_RSS  , load_pair_disjoint                                  ),
 /*C8x5*/ GENx___x___x900 ( "LPDG"      , SSF  , ASMFMT_SSF_RSS  , load_pair_disjoint_long                             ),
 /*C8x6*/ GENx___x___x___ ,
 /*C8x7*/ GENx___x___x___ ,
 /*C8x8*/ GENx___x___x___ ,
 /*C8x9*/ GENx___x___x___ ,
 /*C8xA*/ GENx___x___x___ ,
 /*C8xB*/ GENx___x___x___ ,
 /*C8xC*/ GENx___x___x___ ,
 /*C8xD*/ GENx___x___x___ ,
 /*C8xE*/ GENx___x___x___ ,
 /*C8xF*/ GENx___x___x___
};

static INSTR_FUNC gen_opcode_cc_x[16][NUM_INSTR_TAB_PTRS] =
{
 /*CCx0*/ GENx___x___x___ ,
 /*CCx1*/ GENx___x___x___ ,
 /*CCx2*/ GENx___x___x___ ,
 /*CCx3*/ GENx___x___x___ ,
 /*CCx4*/ GENx___x___x___ ,
 /*CCx5*/ GENx___x___x___ ,
 /*CCx6*/ GENx___x___x900 ( "BRCTH"     , RIL_b, ASMFMT_RIL      , branch_relative_on_count_high                       ),
 /*CCx7*/ GENx___x___x___ ,
 /*CCx8*/ GENx___x___x900 ( "AIH"       , RIL_a, ASMFMT_RIL      , add_high_immediate                                  ),
 /*CCx9*/ GENx___x___x___ ,
 /*CCxA*/ GENx___x___x900 ( "ALSIH"     , RIL_a, ASMFMT_RIL      , add_logical_with_signed_immediate_high              ),
 /*CCxB*/ GENx___x___x900 ( "ALSIHN"    , RIL_a, ASMFMT_RIL      , add_logical_with_signed_immediate_high_n            ),
 /*CCxC*/ GENx___x___x___ ,
 /*CCxD*/ GENx___x___x900 ( "CIH"       , RIL_a, ASMFMT_RIL      , compare_high_immediate                              ),
 /*CCxE*/ GENx___x___x___ ,
 /*CCxF*/ GENx___x___x900 ( "CLIH"      , RIL_a, ASMFMT_RIL      , compare_logical_high_immediate                      )
};

static INSTR_FUNC gen_opcode_e3xx[256][NUM_INSTR_TAB_PTRS] =
{
 /*E300*/ GENx___x___x___ ,
 /*E301*/ GENx___x___x___ ,
 /*E302*/ GENx___x___x900 ( "LTG"       , RXY_a, ASMFMT_RXY      , load_and_test_long                                  ),
 /*E303*/ GENx___x___x900 ( "LRAG"      , RXY_a, ASMFMT_RXY      , load_real_address_long                              ),
 /*E304*/ GENx___x___x900 ( "LG"        , RXY_a, ASMFMT_RXY      , load_long                                           ),
 /*E305*/ GENx___x___x___ ,
 /*E306*/ GENx37Xx___x900 ( "CVBY"      , RXY_a, ASMFMT_RXY      , convert_to_binary_y                                 ),
 /*E307*/ GENx___x___x___ ,
 /*E308*/ GENx___x___x900 ( "AG"        , RXY_a, ASMFMT_RXY      , add_long                                            ),
 /*E309*/ GENx___x___x900 ( "SG"        , RXY_a, ASMFMT_RXY      , subtract_long                                       ),
 /*E30A*/ GENx___x___x900 ( "ALG"       , RXY_a, ASMFMT_RXY      , add_logical_long                                    ),
 /*E30B*/ GENx___x___x900 ( "SLG"       , RXY_a, ASMFMT_RXY      , subtract_logical_long                               ),
 /*E30C*/ GENx___x___x900 ( "MSG"       , RXY_a, ASMFMT_RXY      , multiply_single_long                                ),
 /*E30D*/ GENx___x___x900 ( "DSG"       , RXY_a, ASMFMT_RXY      , divide_single_long                                  ),
 /*E30E*/ GENx___x___x900 ( "CVBG"      , RXY_a, ASMFMT_RXY      , convert_to_binary_long                              ),
 /*E30F*/ GENx___x___x900 ( "LRVG"      , RXY_a, ASMFMT_RXY      , load_reversed_long                                  ),
 /*E310*/ GENx___x___x___ ,
 /*E311*/ GENx___x___x___ ,
 /*E312*/ GENx37Xx390x900 ( "LT"        , RXY_a, ASMFMT_RXY      , load_and_test                                       ),
 /*E313*/ GENx___x___x900 ( "LRAY"      , RXY_a, ASMFMT_RXY      , load_real_address_y                                 ),
 /*E314*/ GENx___x___x900 ( "LGF"       , RXY_a, ASMFMT_RXY      , load_long_fullword                                  ),
 /*E315*/ GENx___x___x900 ( "LGH"       , RXY_a, ASMFMT_RXY      , load_long_halfword                                  ),
 /*E316*/ GENx___x___x900 ( "LLGF"      , RXY_a, ASMFMT_RXY      , load_logical_long_fullword                          ),
 /*E317*/ GENx___x___x900 ( "LLGT"      , RXY_a, ASMFMT_RXY      , load_logical_long_thirtyone                         ),
 /*E318*/ GENx___x___x900 ( "AGF"       , RXY_a, ASMFMT_RXY      , add_long_fullword                                   ),
 /*E319*/ GENx___x___x900 ( "SGF"       , RXY_a, ASMFMT_RXY      , subtract_long_fullword                              ),
 /*E31A*/ GENx___x___x900 ( "ALGF"      , RXY_a, ASMFMT_RXY      , add_logical_long_fullword                           ),
 /*E31B*/ GENx___x___x900 ( "SLGF"      , RXY_a, ASMFMT_RXY      , subtract_logical_long_fullword                      ),
 /*E31C*/ GENx___x___x900 ( "MSGF"      , RXY_a, ASMFMT_RXY      , multiply_single_long_fullword                       ),
 /*E31D*/ GENx___x___x900 ( "DSGF"      , RXY_a, ASMFMT_RXY      , divide_single_long_fullword                         ),
 /*E31E*/ GENx37Xx390x900 ( "LRV"       , RXY_a, ASMFMT_RXY      , load_reversed                                       ),
 /*E31F*/ GENx37Xx390x900 ( "LRVH"      , RXY_a, ASMFMT_RXY      , load_reversed_half                                  ),
 /*E320*/ GENx___x___x900 ( "CG"        , RXY_a, ASMFMT_RXY      , compare_long                                        ),
 /*E321*/ GENx___x___x900 ( "CLG"       , RXY_a, ASMFMT_RXY      , compare_logical_long                                ),
 /*E322*/ GENx___x___x___ ,
 /*E323*/ GENx___x___x___ ,
 /*E324*/ GENx___x___x900 ( "STG"       , RXY_a, ASMFMT_RXY      , store_long                                          ),
 /*E325*/ GENx___x___x900 ( "NTSTG"     , RXY_a, ASMFMT_RXY      , nontransactional_store                              ),
 /*E326*/ GENx37Xx___x900 ( "CVDY"      , RXY_a, ASMFMT_RXY      , convert_to_decimal_y                                ),
 /*E327*/ GENx___x___x___ ,
 /*E328*/ GENx___x___x___ ,
 /*E329*/ GENx___x___x___ ,
 /*E32A*/ GENx___x___x900 ( "LZRG"      , RXY_a, ASMFMT_RXY      , load_and_zero_rightmost_byte_grande                 ),
 /*E32B*/ GENx___x___x___ ,
 /*E32C*/ GENx___x___x___ ,
 /*E32D*/ GENx___x___x___ ,
 /*E32E*/ GENx___x___x900 ( "CVDG"      , RXY_a, ASMFMT_RXY      , convert_to_decimal_long                             ),
 /*E32F*/ GENx___x___x900 ( "STRVG"     , RXY_a, ASMFMT_RXY      , store_reversed_long                                 ),
 /*E330*/ GENx___x___x900 ( "CGF"       , RXY_a, ASMFMT_RXY      , compare_long_fullword                               ),
 /*E331*/ GENx___x___x900 ( "CLGF"      , RXY_a, ASMFMT_RXY      , compare_logical_long_fullword                       ),
 /*E332*/ GENx___x___x900 ( "LTGF"      , RXY_a, ASMFMT_RXY      , load_and_test_long_fullword                         ),
 /*E333*/ GENx___x___x___ ,
 /*E334*/ GENx___x___x900 ( "CGH"       , RXY_a, ASMFMT_RXY      , compare_halfword_long                               ),
 /*E335*/ GENx___x___x___ ,
 /*E336*/ GENx37Xx390x900 ( "PFD"       , RXY_b, ASMFMT_RXY      , prefetch_data                                       ),
 /*E337*/ GENx___x___x___ ,
 /*E338*/ GENx___x___x900 ( "AGH"       , RXY_a, ASMFMT_RXY      , add_long_halfword                                   ),
 /*E339*/ GENx___x___x900 ( "SGH"       , RXY_a, ASMFMT_RXY      , subtract_long_halfword                              ),
 /*E33A*/ GENx___x___x900 ( "LLZRGF"    , RXY_a, ASMFMT_RXY      , load_logical_and_zero_rightmost_byte                ),
 /*E33B*/ GENx___x___x900 ( "LZRF"      , RXY_a, ASMFMT_RXY      , load_and_zero_rightmost_byte                        ),
 /*E33C*/ GENx___x___x900 ( "MGH"       , RXY_a, ASMFMT_RXY      , multiply_long_halfword                              ),
 /*E33D*/ GENx___x___x___ ,
 /*E33E*/ GENx37Xx390x900 ( "STRV"      , RXY_a, ASMFMT_RXY      , store_reversed                                      ),
 /*E33F*/ GENx37Xx390x900 ( "STRVH"     , RXY_a, ASMFMT_RXY      , store_reversed_half                                 ),
 /*E340*/ GENx___x___x___ ,
 /*E341*/ GENx___x___x___ ,
 /*E342*/ GENx___x___x___ ,
 /*E343*/ GENx___x___x___ ,
 /*E344*/ GENx___x___x___ ,
 /*E345*/ GENx___x___x___ ,
 /*E346*/ GENx___x___x900 ( "BCTG"      , RXY_a, ASMFMT_RXY      , branch_on_count_long                                ),
 /*E347*/ GENx___x___x900 ( "BIC"       , RXY_b, ASMFMT_RXY      , branch_indirect_on_condition                        ),
 /*E348*/ GENx___x___x___ ,
 /*E349*/ GENx___x___x___ ,
 /*E34A*/ GENx___x___x___ ,
 /*E34B*/ GENx___x___x___ ,
 /*E34C*/ GENx___x___x___ ,
 /*E34D*/ GENx___x___x___ ,
 /*E34E*/ GENx___x___x___ ,
 /*E34F*/ GENx___x___x___ ,
 /*E350*/ GENx37Xx___x900 ( "STY"       , RXY_a, ASMFMT_RXY      , store_y                                             ),
 /*E351*/ GENx37Xx___x900 ( "MSY"       , RXY_a, ASMFMT_RXY      , multiply_single_y                                   ),
 /*E352*/ GENx___x___x___ ,
 /*E353*/ GENx___x___x900 ( "MSC"       , RXY_a, ASMFMT_RXY      , multiply_single_cc                                  ),
 /*E354*/ GENx37Xx___x900 ( "NY"        , RXY_a, ASMFMT_RXY      , and_y                                               ),
 /*E355*/ GENx37Xx___x900 ( "CLY"       , RXY_a, ASMFMT_RXY      , compare_logical_y                                   ),
 /*E356*/ GENx37Xx___x900 ( "OY"        , RXY_a, ASMFMT_RXY      , or_y                                                ),
 /*E357*/ GENx37Xx___x900 ( "XY"        , RXY_a, ASMFMT_RXY      , exclusive_or_y                                      ),
 /*E358*/ GENx37Xx___x900 ( "LY"        , RXY_a, ASMFMT_RXY      , load_y                                              ),
 /*E359*/ GENx37Xx___x900 ( "CY"        , RXY_a, ASMFMT_RXY      , compare_y                                           ),
 /*E35A*/ GENx37Xx___x900 ( "AY"        , RXY_a, ASMFMT_RXY      , add_y                                               ),
 /*E35B*/ GENx37Xx___x900 ( "SY"        , RXY_a, ASMFMT_RXY      , subtract_y                                          ),
 /*E35C*/ GENx37Xx___x900 ( "MFY"       , RXY_a, ASMFMT_RXY      , multiply_y                                          ),
 /*E35D*/ GENx___x___x___ ,
 /*E35E*/ GENx37Xx___x900 ( "ALY"       , RXY_a, ASMFMT_RXY      , add_logical_y                                       ),
 /*E35F*/ GENx37Xx___x900 ( "SLY"       , RXY_a, ASMFMT_RXY      , subtract_logical_y                                  ),
 /*E360*/ GENx___x___x___ ,
 /*E361*/ GENx___x___x___ ,
 /*E362*/ GENx___x___x___ ,
 /*E363*/ GENx___x___x___ ,
 /*E364*/ GENx___x___x___ ,
 /*E365*/ GENx___x___x___ ,
 /*E366*/ GENx___x___x___ ,
 /*E367*/ GENx___x___x___ ,
 /*E368*/ GENx___x___x___ ,
 /*E369*/ GENx___x___x___ ,
 /*E36A*/ GENx___x___x___ ,
 /*E36B*/ GENx___x___x___ ,
 /*E36C*/ GENx___x___x___ ,
 /*E36D*/ GENx___x___x___ ,
 /*E36E*/ GENx___x___x___ ,
 /*E36F*/ GENx___x___x___ ,
 /*E370*/ GENx___x___x900 ( "STHY"      , RXY_a, ASMFMT_RXY      , store_halfword_y                                    ),
 /*E371*/ GENx___x___x900 ( "LAY"       , RXY_a, ASMFMT_RXY      , load_address_y                                      ),
 /*E372*/ GENx___x___x900 ( "STCY"      , RXY_a, ASMFMT_RXY      , store_character_y                                   ),
 /*E373*/ GENx___x___x900 ( "ICY"       , RXY_a, ASMFMT_RXY      , insert_character_y                                  ),
 /*E374*/ GENx___x___x___ ,
 /*E375*/ GENx___x___x900 ( "LAEY"      , RXY_a, ASMFMT_RXY      , load_address_extended_y                             ),
 /*E376*/ GENx37Xx___x900 ( "LB"        , RXY_a, ASMFMT_RXY      , load_byte                                           ),
 /*E377*/ GENx___x___x900 ( "LGB"       , RXY_a, ASMFMT_RXY      , load_byte_long                                      ),
 /*E378*/ GENx37Xx___x900 ( "LHY"       , RXY_a, ASMFMT_RXY      , load_halfword_y                                     ),
 /*E379*/ GENx37Xx___x900 ( "CHY"       , RXY_a, ASMFMT_RXY      , compare_halfword_y                                  ),
 /*E37A*/ GENx37Xx___x900 ( "AHY"       , RXY_a, ASMFMT_RXY      , add_halfword_y                                      ),
 /*E37B*/ GENx37Xx___x900 ( "SHY"       , RXY_a, ASMFMT_RXY      , subtract_halfword_y                                 ),
 /*E37C*/ GENx37Xx___x900 ( "MHY"       , RXY_a, ASMFMT_RXY      , multiply_halfword_y                                 ),
 /*E37D*/ GENx___x___x___ ,
 /*E37E*/ GENx___x___x___ ,
 /*E37F*/ GENx___x___x___ ,
 /*E380*/ GENx___x___x900 ( "NG"        , RXY_a, ASMFMT_RXY      , and_long                                            ),
 /*E381*/ GENx___x___x900 ( "OG"        , RXY_a, ASMFMT_RXY      , or_long                                             ),
 /*E382*/ GENx___x___x900 ( "XG"        , RXY_a, ASMFMT_RXY      , exclusive_or_long                                   ),
 /*E383*/ GENx___x___x900 ( "MSGC"      , RXY_a, ASMFMT_RXY      , multiply_single_long_cc                             ),
 /*E384*/ GENx___x___x900 ( "MG"        , RXY_a, ASMFMT_RXY      , multiply_long                                       ),
 /*E385*/ GENx___x___x900 ( "LGAT"      , RXY_a, ASMFMT_RXY      , load_long_and_trap                                  ),
 /*E386*/ GENx___x___x900 ( "MLG"       , RXY_a, ASMFMT_RXY      , multiply_logical_long                               ),
 /*E387*/ GENx___x___x900 ( "DLG"       , RXY_a, ASMFMT_RXY      , divide_logical_long                                 ),
 /*E388*/ GENx___x___x900 ( "ALCG"      , RXY_a, ASMFMT_RXY      , add_logical_carry_long                              ),
 /*E389*/ GENx___x___x900 ( "SLBG"      , RXY_a, ASMFMT_RXY      , subtract_logical_borrow_long                        ),
 /*E38A*/ GENx___x___x___ ,
 /*E38B*/ GENx___x___x___ ,
 /*E38C*/ GENx___x___x___ ,
 /*E38D*/ GENx___x___x___ ,
 /*E38E*/ GENx___x___x900 ( "STPQ"      , RXY_a, ASMFMT_RXY      , store_pair_to_quadword                              ),
 /*E38F*/ GENx___x___x900 ( "LPQ"       , RXY_a, ASMFMT_RXY      , load_pair_from_quadword                             ),
 /*E390*/ GENx___x___x900 ( "LLGC"      , RXY_a, ASMFMT_RXY      , load_logical_long_character                         ),
 /*E391*/ GENx___x___x900 ( "LLGH"      , RXY_a, ASMFMT_RXY      , load_logical_long_halfword                          ),
 /*E392*/ GENx___x___x___ ,
 /*E393*/ GENx___x___x___ ,
 /*E394*/ GENx37Xx390x900 ( "LLC"       , RXY_a, ASMFMT_RXY      , load_logical_character                              ),
 /*E395*/ GENx37Xx390x900 ( "LLH"       , RXY_a, ASMFMT_RXY      , load_logical_halfword                               ),
 /*E396*/ GENx37Xx390x900 ( "ML"        , RXY_a, ASMFMT_RXY      , multiply_logical                                    ),
 /*E397*/ GENx37Xx390x900 ( "DL"        , RXY_a, ASMFMT_RXY      , divide_logical                                      ),
 /*E398*/ GENx37Xx390x900 ( "ALC"       , RXY_a, ASMFMT_RXY      , add_logical_carry                                   ),
 /*E399*/ GENx37Xx390x900 ( "SLB"       , RXY_a, ASMFMT_RXY      , subtract_logical_borrow                             ),
 /*E39A*/ GENx___x___x___ ,
 /*E39B*/ GENx___x___x___ ,
 /*E39C*/ GENx___x___x900 ( "LLGTAT"    , RXY_a, ASMFMT_RXY      , load_logical_long_thirtyone_and_trap                ),
 /*E39D*/ GENx___x___x900 ( "LLGFAT"    , RXY_a, ASMFMT_RXY      , load_logical_long_fullword_and_trap                 ),
 /*E39E*/ GENx___x___x___ ,
 /*E39F*/ GENx___x___x900 ( "LAT"       , RXY_a, ASMFMT_RXY      , load_and_trap                                       ),
 /*E3A0*/ GENx___x___x___ ,
 /*E3A1*/ GENx___x___x___ ,
 /*E3A2*/ GENx___x___x___ ,
 /*E3A3*/ GENx___x___x___ ,
 /*E3E3*/ GENx___x___x___ ,
 /*E3A5*/ GENx___x___x___ ,
 /*E3A6*/ GENx___x___x___ ,
 /*E3A7*/ GENx___x___x___ ,
 /*E3A8*/ GENx___x___x___ ,
 /*E3A9*/ GENx___x___x___ ,
 /*E3AA*/ GENx___x___x___ ,
 /*E3AB*/ GENx___x___x___ ,
 /*E3AC*/ GENx___x___x___ ,
 /*E3AD*/ GENx___x___x___ ,
 /*E3AE*/ GENx___x___x___ ,
 /*E3AF*/ GENx___x___x___ ,
 /*E3B0*/ GENx___x___x___ ,
 /*E3B1*/ GENx___x___x___ ,
 /*E3B2*/ GENx___x___x___ ,
 /*E3B3*/ GENx___x___x___ ,
 /*E3B4*/ GENx___x___x___ ,
 /*E3B5*/ GENx___x___x___ ,
 /*E3B6*/ GENx___x___x___ ,
 /*E3B7*/ GENx___x___x___ ,
 /*E3B8*/ GENx___x___x___ ,
 /*E3E3*/ GENx___x___x___ ,
 /*E3BA*/ GENx___x___x___ ,
 /*E3BB*/ GENx___x___x___ ,
 /*E3BC*/ GENx___x___x___ ,
 /*E3BD*/ GENx___x___x___ ,
 /*E3BE*/ GENx___x___x___ ,
 /*E3BF*/ GENx___x___x___ ,
 /*E3C0*/ GENx___x___x900 ( "LBH"       , RXY_a, ASMFMT_RXY      , load_byte_high                                      ),
 /*E3C1*/ GENx___x___x___ ,
 /*E3C2*/ GENx___x___x900 ( "LLCH"      , RXY_a, ASMFMT_RXY      , load_logical_character_high                         ),
 /*E3C3*/ GENx___x___x900 ( "STCH"      , RXY_a, ASMFMT_RXY      , store_character_high                                ),
 /*E3C4*/ GENx___x___x900 ( "LHH"       , RXY_a, ASMFMT_RXY      , load_halfword_high                                  ),
 /*E3C5*/ GENx___x___x___ ,
 /*E3C6*/ GENx___x___x900 ( "LLHH"      , RXY_a, ASMFMT_RXY      , load_logical_halfword_high                          ),
 /*E3C7*/ GENx___x___x900 ( "STHH"      , RXY_a, ASMFMT_RXY      , store_halfword_high                                 ),
 /*E3C8*/ GENx___x___x900 ( "LFHAT"     , RXY_a, ASMFMT_RXY      , load_fullword_high_and_trap                         ),
 /*E3C9*/ GENx___x___x___ ,
 /*E3CA*/ GENx___x___x900 ( "LFH"       , RXY_a, ASMFMT_RXY      , load_fullword_high                                  ),
 /*E3CB*/ GENx___x___x900 ( "STFH"      , RXY_a, ASMFMT_RXY      , store_fullword_high                                 ),
 /*E3CC*/ GENx___x___x___ ,
 /*E3CD*/ GENx___x___x900 ( "CHF"       , RXY_a, ASMFMT_RXY      , compare_high_fullword                               ),
 /*E3CE*/ GENx___x___x___ ,
 /*E3CF*/ GENx___x___x900 ( "CLHF"      , RXY_a, ASMFMT_RXY      , compare_logical_high_fullword                       ),
 /*E3D0*/ GENx___x___x___ ,
 /*E3D1*/ GENx___x___x___ ,
 /*E3D2*/ GENx___x___x___ ,
 /*E3D3*/ GENx___x___x___ ,
 /*E3D4*/ GENx___x___x___ ,
 /*E3D5*/ GENx___x___x___ ,
 /*E3D6*/ GENx___x___x___ ,
 /*E3D7*/ GENx___x___x___ ,
 /*E3D8*/ GENx___x___x___ ,
 /*E3D9*/ GENx___x___x___ ,
 /*E3DA*/ GENx___x___x___ ,
 /*E3DB*/ GENx___x___x___ ,
 /*E3DC*/ GENx___x___x___ ,
 /*E3DD*/ GENx___x___x___ ,
 /*E3DE*/ GENx___x___x___ ,
 /*E3DF*/ GENx___x___x___ ,
 /*E3E0*/ GENx___x___x___ ,
 /*E3E1*/ GENx___x___x___ ,
 /*E3E2*/ GENx___x___x___ ,
 /*E3E3*/ GENx___x___x___ ,
 /*E3E4*/ GENx___x___x___ ,
 /*E3E5*/ GENx___x___x___ ,
 /*E3E6*/ GENx___x___x___ ,
 /*E3E7*/ GENx___x___x___ ,
 /*E3E8*/ GENx___x___x___ ,
 /*E3E9*/ GENx___x___x___ ,
 /*E3EA*/ GENx___x___x___ ,
 /*E3EB*/ GENx___x___x___ ,
 /*E3EC*/ GENx___x___x___ ,
 /*E3ED*/ GENx___x___x___ ,
 /*E3EE*/ GENx___x___x___ ,
 /*E3EF*/ GENx___x___x___ ,
 /*E3F0*/ GENx___x___x___ ,
 /*E3F1*/ GENx___x___x___ ,
 /*E3F2*/ GENx___x___x___ ,
 /*E3F3*/ GENx___x___x___ ,
 /*E3F4*/ GENx___x___x___ ,
 /*E3F5*/ GENx___x___x___ ,
 /*E3F6*/ GENx___x___x___ ,
 /*E3F7*/ GENx___x___x___ ,
 /*E3F8*/ GENx___x___x___ ,
 /*E3F9*/ GENx___x___x___ ,
 /*E3FA*/ GENx___x___x___ ,
 /*E3FB*/ GENx___x___x___ ,
 /*E3FC*/ GENx___x___x___ ,
 /*E3FD*/ GENx___x___x___ ,
 /*E3FE*/ GENx___x___x___ ,
 /*E3FF*/ GENx___x___x___
};

static INSTR_FUNC gen_opcode_e5xx[256][NUM_INSTR_TAB_PTRS] =
{
 /*E500*/ GENx370x390x900 ( "LASP"      , SSE  , ASMFMT_SSE      , load_address_space_parameters                       ),
 /*E501*/ GENx370x390x900 ( "TPROT"     , SSE  , ASMFMT_SSE      , test_protection                                     ),



/*-------------------------------------------------------------------*/
/*       The following opcode has been re-used in z/Arch             */
/*-------------------------------------------------------------------*/

#define s370_store_real_address     s370_fix_page

 /*E502*/ GENx370x___x900 ( "STRAG"     , SSE  , ASMFMT_SSE      , store_real_address                                  ),

/*-------------------------------------------------------------------*/



 /*E503*/ GENx370x390x900 ( "Assist"    , SSE  , ASMFMT_SSE      , svc_assist                                          ),
 /*E504*/ GENx370x390x900 ( "Assist"    , SSE  , ASMFMT_SSE      , obtain_local_lock                                   ),
 /*E505*/ GENx370x390x900 ( "Assist"    , SSE  , ASMFMT_SSE      , release_local_lock                                  ),
 /*E506*/ GENx370x390x900 ( "Assist"    , SSE  , ASMFMT_SSE      , obtain_cms_lock                                     ),
 /*E507*/ GENx370x390x900 ( "Assist"    , SSE  , ASMFMT_SSE      , release_cms_lock                                    ),
 /*E508*/ GENx370x___x___ ( "Assist"    , SSE  , ASMFMT_SSE      , trace_svc_interruption                              ),
 /*E509*/ GENx370x___x___ ( "Assist"    , SSE  , ASMFMT_SSE      , trace_program_interruption                          ),



/*-------------------------------------------------------------------*/
/*       The following opcode has been re-used in z/Arch             */
/*-------------------------------------------------------------------*/

#define s370_move_right_to_left     s370_trace_initial_srb_dispatch

 /*E50A*/ GENx370x___x900 ( "MVCRL"     , SSE  , ASMFMT_SSE      , move_right_to_left                                  ),

/*-------------------------------------------------------------------*/



 /*E50B*/ GENx370x___x___ ( "Assist"    , SSE  , ASMFMT_SSE      , trace_io_interruption                               ),
 /*E50C*/ GENx370x___x___ ( "Assist"    , SSE  , ASMFMT_SSE      , trace_task_dispatch                                 ),
 /*E50D*/ GENx370x___x___ ( "Assist"    , SSE  , ASMFMT_SSE      , trace_svc_return                                    ),
 /*E50E*/ GENx___x390x900 ( "MVCSK"     , SSE  , ASMFMT_SSE      , move_with_source_key                                ),
 /*E50F*/ GENx___x390x900 ( "MVCDK"     , SSE  , ASMFMT_SSE      , move_with_destination_key                           ),
 /*E510*/ GENx___x___x___ ,
 /*E511*/ GENx___x___x___ ,
 /*E512*/ GENx___x___x___ ,
 /*E513*/ GENx___x___x___ ,
 /*E514*/ GENx___x___x___ ,
 /*E515*/ GENx___x___x___ ,
 /*E516*/ GENx___x___x___ ,
 /*E517*/ GENx___x___x___ ,
 /*E518*/ GENx___x___x___ ,
 /*E519*/ GENx___x___x___ ,
 /*E51A*/ GENx___x___x___ ,
 /*E51B*/ GENx___x___x___ ,
 /*E51C*/ GENx___x___x___ ,
 /*E51D*/ GENx___x___x___ ,
 /*E51E*/ GENx___x___x___ ,
 /*E51F*/ GENx___x___x___ ,
 /*E520*/ GENx___x___x___ ,
 /*E521*/ GENx___x___x___ ,
 /*E522*/ GENx___x___x___ ,
 /*E523*/ GENx___x___x___ ,
 /*E524*/ GENx___x___x___ ,
 /*E525*/ GENx___x___x___ ,
 /*E526*/ GENx___x___x___ ,
 /*E527*/ GENx___x___x___ ,
 /*E528*/ GENx___x___x___ ,
 /*E529*/ GENx___x___x___ ,
 /*E52A*/ GENx___x___x___ ,
 /*E52B*/ GENx___x___x___ ,
 /*E52C*/ GENx___x___x___ ,
 /*E52D*/ GENx___x___x___ ,
 /*E52E*/ GENx___x___x___ ,
 /*E52F*/ GENx___x___x___ ,
 /*E530*/ GENx___x___x___ ,
 /*E531*/ GENx___x___x___ ,
 /*E532*/ GENx___x___x___ ,
 /*E533*/ GENx___x___x___ ,
 /*E534*/ GENx___x___x___ ,
 /*E535*/ GENx___x___x___ ,
 /*E536*/ GENx___x___x___ ,
 /*E537*/ GENx___x___x___ ,
 /*E538*/ GENx___x___x___ ,
 /*E539*/ GENx___x___x___ ,
 /*E53A*/ GENx___x___x___ ,
 /*E53B*/ GENx___x___x___ ,
 /*E53C*/ GENx___x___x___ ,
 /*E53D*/ GENx___x___x___ ,
 /*E53E*/ GENx___x___x___ ,
 /*E53F*/ GENx___x___x___ ,
 /*E540*/ GENx___x___x___ ,
 /*E541*/ GENx___x___x___ ,
 /*E542*/ GENx___x___x___ ,
 /*E543*/ GENx___x___x___ ,
 /*E544*/ GENx37Xx390x900 ( "MVHHI"     , SIL  , ASMFMT_SIL      , move_halfword_from_halfword_immediate               ),
 /*E545*/ GENx___x___x___ ,
 /*E546*/ GENx___x___x___ ,
 /*E547*/ GENx___x___x___ ,
 /*E548*/ GENx37Xx390x900 ( "MVGHI"     , SIL  , ASMFMT_SIL      , move_long_from_halfword_immediate                   ),
 /*E549*/ GENx___x___x___ ,
 /*E54A*/ GENx___x___x___ ,
 /*E54B*/ GENx___x___x___ ,
 /*E54C*/ GENx37Xx390x900 ( "MVHI"      , SIL  , ASMFMT_SIL      , move_fullword_from_halfword_immediate               ),
 /*E54D*/ GENx___x___x___ ,
 /*E54E*/ GENx___x___x___ ,
 /*E54F*/ GENx___x___x___ ,
 /*E550*/ GENx___x___x___ ,
 /*E551*/ GENx___x___x___ ,
 /*E552*/ GENx___x___x___ ,
 /*E553*/ GENx___x___x___ ,
 /*E554*/ GENx37Xx390x900 ( "CHHSI"     , SIL  , ASMFMT_SIL      , compare_halfword_immediate_halfword_storage         ),
 /*E555*/ GENx37Xx390x900 ( "CLHHSI"    , SIL  , ASMFMT_SIL      , compare_logical_immediate_halfword_storage          ),
 /*E556*/ GENx___x___x___ ,
 /*E557*/ GENx___x___x___ ,
 /*E558*/ GENx37Xx390x900 ( "CGHSI"     , SIL  , ASMFMT_SIL      , compare_halfword_immediate_long_storage             ),
 /*E559*/ GENx37Xx390x900 ( "CLGHSI"    , SIL  , ASMFMT_SIL      , compare_logical_immediate_long_storage              ),
 /*E55A*/ GENx___x___x___ ,
 /*E55B*/ GENx___x___x___ ,
 /*E55C*/ GENx37Xx390x900 ( "CHSI"      , SIL  , ASMFMT_SIL      , compare_halfword_immediate_storage                  ),
 /*E55D*/ GENx37Xx390x900 ( "CLFHSI"    , SIL  , ASMFMT_SIL      , compare_logical_immediate_fullword_storage          ),
 /*E55E*/ GENx___x___x___ ,
 /*E55F*/ GENx___x___x___ ,
 /*E560*/ GENx___x___x900 ( "TBEGIN"    , SIL  , ASMFMT_SIL      , transaction_begin                                   ),
 /*E561*/ GENx___x___x900 ( "TBEGINC"   , SIL  , ASMFMT_SIL      , transaction_begin_constrained                       ),
 /*E562*/ GENx___x___x___ ,
 /*E563*/ GENx___x___x___ ,
 /*E564*/ GENx___x___x___ ,
 /*E565*/ GENx___x___x___ ,
 /*E566*/ GENx___x___x___ ,
 /*E567*/ GENx___x___x___ ,
 /*E568*/ GENx___x___x___ ,
 /*E569*/ GENx___x___x___ ,
 /*E56A*/ GENx___x___x___ ,
 /*E56B*/ GENx___x___x___ ,
 /*E56C*/ GENx___x___x___ ,
 /*E56D*/ GENx___x___x___ ,
 /*E56E*/ GENx___x___x___ ,
 /*E56F*/ GENx___x___x___ ,
 /*E570*/ GENx___x___x___ ,
 /*E571*/ GENx___x___x___ ,
 /*E572*/ GENx___x___x___ ,
 /*E573*/ GENx___x___x___ ,
 /*E574*/ GENx___x___x___ ,
 /*E575*/ GENx___x___x___ ,
 /*E576*/ GENx___x___x___ ,
 /*E577*/ GENx___x___x___ ,
 /*E578*/ GENx___x___x___ ,
 /*E579*/ GENx___x___x___ ,
 /*E57A*/ GENx___x___x___ ,
 /*E57B*/ GENx___x___x___ ,
 /*E57C*/ GENx___x___x___ ,
 /*E57D*/ GENx___x___x___ ,
 /*E57E*/ GENx___x___x___ ,
 /*E57F*/ GENx___x___x___ ,
 /*E580*/ GENx___x___x___ ,
 /*E581*/ GENx___x___x___ ,
 /*E582*/ GENx___x___x___ ,
 /*E583*/ GENx___x___x___ ,
 /*E584*/ GENx___x___x___ ,
 /*E585*/ GENx___x___x___ ,
 /*E586*/ GENx___x___x___ ,
 /*E587*/ GENx___x___x___ ,
 /*E588*/ GENx___x___x___ ,
 /*E589*/ GENx___x___x___ ,
 /*E58A*/ GENx___x___x___ ,
 /*E58B*/ GENx___x___x___ ,
 /*E58C*/ GENx___x___x___ ,
 /*E58D*/ GENx___x___x___ ,
 /*E58E*/ GENx___x___x___ ,
 /*E58F*/ GENx___x___x___ ,
 /*E590*/ GENx___x___x___ ,
 /*E591*/ GENx___x___x___ ,
 /*E592*/ GENx___x___x___ ,
 /*E593*/ GENx___x___x___ ,
 /*E594*/ GENx___x___x___ ,
 /*E595*/ GENx___x___x___ ,
 /*E596*/ GENx___x___x___ ,
 /*E597*/ GENx___x___x___ ,
 /*E598*/ GENx___x___x___ ,
 /*E599*/ GENx___x___x___ ,
 /*E59A*/ GENx___x___x___ ,
 /*E59B*/ GENx___x___x___ ,
 /*E59C*/ GENx___x___x___ ,
 /*E59D*/ GENx___x___x___ ,
 /*E59E*/ GENx___x___x___ ,
 /*E59F*/ GENx___x___x___ ,
 /*E5A0*/ GENx___x___x___ ,
 /*E5A1*/ GENx___x___x___ ,
 /*E5A2*/ GENx___x___x___ ,
 /*E5A3*/ GENx___x___x___ ,
 /*E5A4*/ GENx___x___x___ ,
 /*E5A5*/ GENx___x___x___ ,
 /*E5A6*/ GENx___x___x___ ,
 /*E5A7*/ GENx___x___x___ ,
 /*E5A8*/ GENx___x___x___ ,
 /*E5A9*/ GENx___x___x___ ,
 /*E5AA*/ GENx___x___x___ ,
 /*E5AB*/ GENx___x___x___ ,
 /*E5AC*/ GENx___x___x___ ,
 /*E5AD*/ GENx___x___x___ ,
 /*E5AE*/ GENx___x___x___ ,
 /*E5AF*/ GENx___x___x___ ,
 /*E5B0*/ GENx___x___x___ ,
 /*E5B1*/ GENx___x___x___ ,
 /*E5B2*/ GENx___x___x___ ,
 /*E5B3*/ GENx___x___x___ ,
 /*E5B4*/ GENx___x___x___ ,
 /*E5B5*/ GENx___x___x___ ,
 /*E5B6*/ GENx___x___x___ ,
 /*E5B7*/ GENx___x___x___ ,
 /*E5B8*/ GENx___x___x___ ,
 /*E5B9*/ GENx___x___x___ ,
 /*E5BA*/ GENx___x___x___ ,
 /*E5BB*/ GENx___x___x___ ,
 /*E5BC*/ GENx___x___x___ ,
 /*E5BD*/ GENx___x___x___ ,
 /*E5BE*/ GENx___x___x___ ,
 /*E5BF*/ GENx___x___x___ ,
 /*E5C0*/ GENx___x___x___ ,
 /*E5C1*/ GENx___x___x___ ,
 /*E5C2*/ GENx___x___x___ ,
 /*E5C3*/ GENx___x___x___ ,
 /*E5C4*/ GENx___x___x___ ,
 /*E5C5*/ GENx___x___x___ ,
 /*E5C6*/ GENx___x___x___ ,
 /*E5C7*/ GENx___x___x___ ,
 /*E5C8*/ GENx___x___x___ ,
 /*E5C9*/ GENx___x___x___ ,
 /*E5CA*/ GENx___x___x___ ,
 /*E5CB*/ GENx___x___x___ ,
 /*E5CC*/ GENx___x___x___ ,
 /*E5CD*/ GENx___x___x___ ,
 /*E5CE*/ GENx___x___x___ ,
 /*E5CF*/ GENx___x___x___ ,
 /*E5D0*/ GENx___x___x___ ,
 /*E5D1*/ GENx___x___x___ ,
 /*E5D2*/ GENx___x___x___ ,
 /*E5D3*/ GENx___x___x___ ,
 /*E5D4*/ GENx___x___x___ ,
 /*E5D5*/ GENx___x___x___ ,
 /*E5D6*/ GENx___x___x___ ,
 /*E5D7*/ GENx___x___x___ ,
 /*E5D8*/ GENx___x___x___ ,
 /*E5D9*/ GENx___x___x___ ,
 /*E5DA*/ GENx___x___x___ ,
 /*E5DB*/ GENx___x___x___ ,
 /*E5DC*/ GENx___x___x___ ,
 /*E5DD*/ GENx___x___x___ ,
 /*E5DE*/ GENx___x___x___ ,
 /*E5DF*/ GENx___x___x___ ,
 /*E5E0*/ GENx___x___x___ ,
 /*E5E1*/ GENx___x___x___ ,
 /*E5E2*/ GENx___x___x___ ,
 /*E5E3*/ GENx___x___x___ ,
 /*E5E4*/ GENx___x___x___ ,
 /*E5E5*/ GENx___x___x___ ,
 /*E5E6*/ GENx___x___x___ ,
 /*E5E7*/ GENx___x___x___ ,
 /*E5E8*/ GENx___x___x___ ,
 /*E5E9*/ GENx___x___x___ ,
 /*E5EA*/ GENx___x___x___ ,
 /*E5EB*/ GENx___x___x___ ,
 /*E5EC*/ GENx___x___x___ ,
 /*E5ED*/ GENx___x___x___ ,
 /*E5EE*/ GENx___x___x___ ,
 /*E5EF*/ GENx___x___x___ ,
 /*E5F0*/ GENx___x___x___ ,
 /*E5F1*/ GENx___x___x___ ,
 /*E5F2*/ GENx___x___x___ ,
 /*E5F3*/ GENx___x___x___ ,
 /*E5F4*/ GENx___x___x___ ,
 /*E5F5*/ GENx___x___x___ ,
 /*E5F6*/ GENx___x___x___ ,
 /*E5F7*/ GENx___x___x___ ,
 /*E5F8*/ GENx___x___x___ ,
 /*E5F9*/ GENx___x___x___ ,
 /*E5FA*/ GENx___x___x___ ,
 /*E5FB*/ GENx___x___x___ ,
 /*E5FC*/ GENx___x___x___ ,
 /*E5FD*/ GENx___x___x___ ,
 /*E5FE*/ GENx___x___x___ ,
 /*E5FF*/ GENx___x___x___
};

static INSTR_FUNC gen_opcode_e6xx[256][NUM_INSTR_TAB_PTRS] =
{
 /*E600*/ GENx370x___x___ ( "FREE"      , SSE  , ASMFMT_SSE      , ecpsvm_basic_freex                                  ),
 /*E601*/ GENx370x___x___ ( "FRET"      , SSE  , ASMFMT_SSE      , ecpsvm_basic_fretx                                  ),
 /*E602*/ GENx370x___x___ ( "VLKPG"     , SSE  , ASMFMT_SSE      , ecpsvm_lock_page                                    ),
 /*E603*/ GENx370x___x___ ( "VULKP"     , SSE  , ASMFMT_SSE      , ecpsvm_unlock_page                                  ),
 /*E604*/ GENx370x___x___ ( "DNCCW"     , SSE  , ASMFMT_SSE      , ecpsvm_decode_next_ccw                              ),
 /*E605*/ GENx370x___x___ ( "FCCWS"     , SSE  , ASMFMT_SSE      , ecpsvm_free_ccwstor                                 ),
 /*E606*/ GENx370x___x___ ( "SCNVU"     , SSE  , ASMFMT_SSE      , ecpsvm_locate_vblock                                ),
 /*E607*/ GENx370x___x___ ( "ECPS_DISP1", SSE  , ASMFMT_SSE      , ecpsvm_disp1                                        ),
 /*E608*/ GENx370x___x___ ( "ECPS_TRBRG", SSE  , ASMFMT_SSE      , ecpsvm_tpage                                        ),
 /*E609*/ GENx370x___x___ ( "TRLCK"     , SSE  , ASMFMT_SSE      , ecpsvm_tpage_lock                                   ),
 /*E60A*/ GENx370x___x___ ( "VIST"      , SSE  , ASMFMT_SSE      , ecpsvm_inval_segtab                                 ),
 /*E60B*/ GENx370x___x___ ( "VIPT"      , SSE  , ASMFMT_SSE      , ecpsvm_inval_ptable                                 ),
 /*E60C*/ GENx370x___x___ ( "DFCCW"     , SSE  , ASMFMT_SSE      , ecpsvm_decode_first_ccw                             ),
 /*E60D*/ GENx370x___x___ ( "DISP0"     , SSE  , ASMFMT_SSE      , ecpsvm_dispatch_main                                ),
 /*E60E*/ GENx370x___x___ ( "SCNRU"     , SSE  , ASMFMT_SSE      , ecpsvm_locate_rblock                                ),
 /*E60F*/ GENx370x___x___ ( "CCWGN"     , SSE  , ASMFMT_SSE      , ecpsvm_comm_ccwproc                                 ),
 /*E610*/ GENx370x___x___ ( "UXCCW"     , SSE  , ASMFMT_SSE      , ecpsvm_unxlate_ccw                                  ),
 /*E611*/ GENx370x___x___ ( "DISP2"     , SSE  , ASMFMT_SSE      , ecpsvm_disp2                                        ),
 /*E612*/ GENx370x___x___ ( "STEVL"     , SSE  , ASMFMT_SSE      , ecpsvm_store_level                                  ),
 /*E613*/ GENx370x___x___ ( "LCSPG"     , SSE  , ASMFMT_SSE      , ecpsvm_loc_chgshrpg                                 ),
 /*E614*/ GENx370x___x___ ( "FREEX"     , SSE  , ASMFMT_SSE      , ecpsvm_extended_freex                               ),
 /*E615*/ GENx370x___x___ ( "FRETX"     , SSE  , ASMFMT_SSE      , ecpsvm_extended_fretx                               ),
 /*E616*/ GENx370x___x___ ( "PRFMA"     , SSE  , ASMFMT_SSE      , ecpsvm_prefmach_assist                              ),
 /*E617*/ GENx___x___x___ ,
 /*E618*/ GENx___x___x___ ,
 /*E619*/ GENx___x___x___ ,
 /*E61A*/ GENx___x___x___ ,
 /*E61B*/ GENx___x___x___ ,
 /*E61C*/ GENx___x___x___ ,
 /*E61D*/ GENx___x___x___ ,
 /*E61E*/ GENx___x___x___ ,
 /*E61F*/ GENx___x___x___ ,
 /*E620*/ GENx___x___x___ ,
 /*E621*/ GENx___x___x___ ,
 /*E622*/ GENx___x___x___ ,
 /*E623*/ GENx___x___x___ ,
 /*E624*/ GENx___x___x___ ,
 /*E625*/ GENx___x___x___ ,
 /*E626*/ GENx___x___x___ ,
 /*E627*/ GENx___x___x___ ,
 /*E628*/ GENx___x___x___ ,
 /*E629*/ GENx___x___x___ ,
 /*E62A*/ GENx___x___x___ ,
 /*E62B*/ GENx___x___x___ ,
 /*E62C*/ GENx___x___x___ ,
 /*E62D*/ GENx___x___x___ ,
 /*E62E*/ GENx___x___x___ ,
 /*E62F*/ GENx___x___x___ ,
 /*E630*/ GENx___x___x___ ,
 /*E631*/ GENx___x___x___ ,
 /*E632*/ GENx___x___x___ ,
 /*E633*/ GENx___x___x___ ,
 /*E634*/ GENx___x___x___ ,
 /*E635*/ GENx___x___x___ ,
 /*E636*/ GENx___x___x___ ,
 /*E637*/ GENx___x___x___ ,
 /*E638*/ GENx___x___x___ ,
 /*E639*/ GENx___x___x___ ,
 /*E63A*/ GENx___x___x___ ,
 /*E63B*/ GENx___x___x___ ,
 /*E63C*/ GENx___x___x___ ,
 /*E63D*/ GENx___x___x___ ,
 /*E63E*/ GENx___x___x___ ,
 /*E63F*/ GENx___x___x___ ,
 /*E640*/ GENx___x___x___ ,
 /*E641*/ GENx___x___x___ ,
 /*E642*/ GENx___x___x___ ,
 /*E643*/ GENx___x___x___ ,
 /*E644*/ GENx___x___x___ ,
 /*E645*/ GENx___x___x___ ,
 /*E646*/ GENx___x___x___ ,
 /*E647*/ GENx___x___x___ ,
 /*E648*/ GENx___x___x___ ,
 /*E649*/ GENx___x___x___ ,
 /*E64A*/ GENx___x___x___ ,
 /*E64B*/ GENx___x___x___ ,
 /*E64C*/ GENx___x___x___ ,
 /*E64D*/ GENx___x___x___ ,
 /*E64E*/ GENx___x___x___ ,
 /*E64F*/ GENx___x___x___ ,
 /*E650*/ GENx___x___x___ ,
 /*E651*/ GENx___x___x___ ,
 /*E652*/ GENx___x___x___ ,
 /*E653*/ GENx___x___x___ ,
 /*E654*/ GENx___x___x___ ,
 /*E655*/ GENx___x___x___ ,
 /*E656*/ GENx___x___x___ ,
 /*E657*/ GENx___x___x___ ,
 /*E658*/ GENx___x___x___ ,
 /*E659*/ GENx___x___x___ ,
 /*E65A*/ GENx___x___x___ ,
 /*E65B*/ GENx___x___x___ ,
 /*E65C*/ GENx___x___x___ ,
 /*E65D*/ GENx___x___x___ ,
 /*E65E*/ GENx___x___x___ ,
 /*E65F*/ GENx___x___x___ ,
 /*E660*/ GENx___x___x___ ,
 /*E661*/ GENx___x___x___ ,
 /*E662*/ GENx___x___x___ ,
 /*E663*/ GENx___x___x___ ,
 /*E664*/ GENx___x___x___ ,
 /*E665*/ GENx___x___x___ ,
 /*E666*/ GENx___x___x___ ,
 /*E667*/ GENx___x___x___ ,
 /*E668*/ GENx___x___x___ ,
 /*E669*/ GENx___x___x___ ,
 /*E66A*/ GENx___x___x___ ,
 /*E66B*/ GENx___x___x___ ,
 /*E66C*/ GENx___x___x___ ,
 /*E66D*/ GENx___x___x___ ,
 /*E66E*/ GENx___x___x___ ,
 /*E66F*/ GENx___x___x___ ,
 /*E670*/ GENx___x___x___ ,
 /*E671*/ GENx___x___x___ ,
 /*E672*/ GENx___x___x___ ,
 /*E673*/ GENx___x___x___ ,
 /*E674*/ GENx___x___x___ ,
 /*E675*/ GENx___x___x___ ,
 /*E676*/ GENx___x___x___ ,
 /*E677*/ GENx___x___x___ ,
 /*E678*/ GENx___x___x___ ,
 /*E679*/ GENx___x___x___ ,
 /*E67A*/ GENx___x___x___ ,
 /*E67B*/ GENx___x___x___ ,
 /*E67C*/ GENx___x___x___ ,
 /*E67D*/ GENx___x___x___ ,
 /*E67E*/ GENx___x___x___ ,
 /*E67F*/ GENx___x___x___ ,
 /*E680*/ GENx___x___x___ ,
 /*E681*/ GENx___x___x___ ,
 /*E682*/ GENx___x___x___ ,
 /*E683*/ GENx___x___x___ ,
 /*E684*/ GENx___x___x___ ,
 /*E685*/ GENx___x___x___ ,
 /*E686*/ GENx___x___x___ ,
 /*E687*/ GENx___x___x___ ,
 /*E688*/ GENx___x___x___ ,
 /*E689*/ GENx___x___x___ ,
 /*E68A*/ GENx___x___x___ ,
 /*E68B*/ GENx___x___x___ ,
 /*E68C*/ GENx___x___x___ ,
 /*E68D*/ GENx___x___x___ ,
 /*E68E*/ GENx___x___x___ ,
 /*E68F*/ GENx___x___x___ ,
 /*E690*/ GENx___x___x___ ,
 /*E691*/ GENx___x___x___ ,
 /*E692*/ GENx___x___x___ ,
 /*E693*/ GENx___x___x___ ,
 /*E694*/ GENx___x___x___ ,
 /*E695*/ GENx___x___x___ ,
 /*E696*/ GENx___x___x___ ,
 /*E697*/ GENx___x___x___ ,
 /*E698*/ GENx___x___x___ ,
 /*E699*/ GENx___x___x___ ,
 /*E69A*/ GENx___x___x___ ,
 /*E69B*/ GENx___x___x___ ,
 /*E69C*/ GENx___x___x___ ,
 /*E69D*/ GENx___x___x___ ,
 /*E69E*/ GENx___x___x___ ,
 /*E69F*/ GENx___x___x___ ,
 /*E6A0*/ GENx___x___x___ ,
 /*E6A1*/ GENx___x___x___ ,
 /*E6A2*/ GENx___x___x___ ,
 /*E6A3*/ GENx___x___x___ ,
 /*E6A4*/ GENx___x___x___ ,
 /*E6A5*/ GENx___x___x___ ,
 /*E6A6*/ GENx___x___x___ ,
 /*E6A7*/ GENx___x___x___ ,
 /*E6A8*/ GENx___x___x___ ,
 /*E6A9*/ GENx___x___x___ ,
 /*E6AA*/ GENx___x___x___ ,
 /*E6AB*/ GENx___x___x___ ,
 /*E6AC*/ GENx___x___x___ ,
 /*E6AD*/ GENx___x___x___ ,
 /*E6AE*/ GENx___x___x___ ,
 /*E6AF*/ GENx___x___x___ ,
 /*E6B0*/ GENx___x___x___ ,
 /*E6B1*/ GENx___x___x___ ,
 /*E6B2*/ GENx___x___x___ ,
 /*E6B3*/ GENx___x___x___ ,
 /*E6B4*/ GENx___x___x___ ,
 /*E6B5*/ GENx___x___x___ ,
 /*E6B6*/ GENx___x___x___ ,
 /*E6B7*/ GENx___x___x___ ,
 /*E6B8*/ GENx___x___x___ ,
 /*E6B9*/ GENx___x___x___ ,
 /*E6BA*/ GENx___x___x___ ,
 /*E6BB*/ GENx___x___x___ ,
 /*E6BC*/ GENx___x___x___ ,
 /*E6BD*/ GENx___x___x___ ,
 /*E6BE*/ GENx___x___x___ ,
 /*E6BF*/ GENx___x___x___ ,
 /*E6C0*/ GENx___x___x___ ,
 /*E6C1*/ GENx___x___x___ ,
 /*E6C2*/ GENx___x___x___ ,
 /*E6C3*/ GENx___x___x___ ,
 /*E6C4*/ GENx___x___x___ ,
 /*E6C5*/ GENx___x___x___ ,
 /*E6C6*/ GENx___x___x___ ,
 /*E6C7*/ GENx___x___x___ ,
 /*E6C8*/ GENx___x___x___ ,
 /*E6C9*/ GENx___x___x___ ,
 /*E6CA*/ GENx___x___x___ ,
 /*E6CB*/ GENx___x___x___ ,
 /*E6CC*/ GENx___x___x___ ,
 /*E6CD*/ GENx___x___x___ ,
 /*E6CE*/ GENx___x___x___ ,
 /*E6CF*/ GENx___x___x___ ,
 /*E6D0*/ GENx___x___x___ ,
 /*E6D1*/ GENx___x___x___ ,
 /*E6D2*/ GENx___x___x___ ,
 /*E6D3*/ GENx___x___x___ ,
 /*E6D4*/ GENx___x___x___ ,
 /*E6D5*/ GENx___x___x___ ,
 /*E6D6*/ GENx___x___x___ ,
 /*E6D7*/ GENx___x___x___ ,
 /*E6D8*/ GENx___x___x___ ,
 /*E6D9*/ GENx___x___x___ ,
 /*E6DA*/ GENx___x___x___ ,
 /*E6DB*/ GENx___x___x___ ,
 /*E6DC*/ GENx___x___x___ ,
 /*E6DD*/ GENx___x___x___ ,
 /*E6DE*/ GENx___x___x___ ,
 /*E6DF*/ GENx___x___x___ ,
 /*E6E0*/ GENx___x___x___ ,
 /*E6E1*/ GENx___x___x___ ,
 /*E6E2*/ GENx___x___x___ ,
 /*E6E3*/ GENx___x___x___ ,
 /*E6E4*/ GENx___x___x___ ,
 /*E6E5*/ GENx___x___x___ ,
 /*E6E6*/ GENx___x___x___ ,
 /*E6E7*/ GENx___x___x___ ,
 /*E6E8*/ GENx___x___x___ ,
 /*E6E9*/ GENx___x___x___ ,
 /*E6EA*/ GENx___x___x___ ,
 /*E6EB*/ GENx___x___x___ ,
 /*E6EC*/ GENx___x___x___ ,
 /*E6ED*/ GENx___x___x___ ,
 /*E6EE*/ GENx___x___x___ ,
 /*E6EF*/ GENx___x___x___ ,
 /*E6F0*/ GENx___x___x___ ,
 /*E6F1*/ GENx___x___x___ ,
 /*E6F2*/ GENx___x___x___ ,
 /*E6F3*/ GENx___x___x___ ,
 /*E6F4*/ GENx___x___x___ ,
 /*E6F5*/ GENx___x___x___ ,
 /*E6F6*/ GENx___x___x___ ,
 /*E6F7*/ GENx___x___x___ ,
 /*E6F8*/ GENx___x___x___ ,
 /*E6F9*/ GENx___x___x___ ,
 /*E6FA*/ GENx___x___x___ ,
 /*E6FB*/ GENx___x___x___ ,
 /*E6FC*/ GENx___x___x___ ,
 /*E6FD*/ GENx___x___x___ ,
 /*E6FE*/ GENx___x___x___ ,
 /*E6FF*/ GENx___x___x___
};

static INSTR_FUNC gen_opcode_ebxx[256][NUM_INSTR_TAB_PTRS] =
{
 /*EB00*/ GENx___x___x___ ,
 /*EB01*/ GENx___x___x___ ,
 /*EB02*/ GENx___x___x___ ,
 /*EB03*/ GENx___x___x___ ,
 /*EB04*/ GENx___x___x900 ( "LMG"       , RSY_a, ASMFMT_RSY      , load_multiple_long                                  ),
 /*EB05*/ GENx___x___x___ ,
 /*EB06*/ GENx___x___x___ ,
 /*EB07*/ GENx___x___x___ ,
 /*EB08*/ GENx___x___x___ ,
 /*EB09*/ GENx___x___x___ ,
 /*EB0A*/ GENx___x___x900 ( "SRAG"      , RSY_a, ASMFMT_RSY      , shift_right_single_long                             ),
 /*EB0B*/ GENx___x___x900 ( "SLAG"      , RSY_a, ASMFMT_RSY      , shift_left_single_long                              ),
 /*EB0C*/ GENx___x___x900 ( "SRLG"      , RSY_a, ASMFMT_RSY      , shift_right_single_logical_long                     ),
 /*EB0D*/ GENx___x___x900 ( "SLLG"      , RSY_a, ASMFMT_RSY      , shift_left_single_logical_long                      ),
 /*EB0E*/ GENx___x___x___ ,
 /*EB0F*/ GENx___x___x900 ( "TRACG"     , RSY_a, ASMFMT_RSY      , trace_long                                          ),
 /*EB10*/ GENx___x___x___ ,
 /*EB11*/ GENx___x___x___ ,
 /*EB12*/ GENx___x___x___ ,
 /*EB13*/ GENx___x___x___ ,
 /*EB14*/ GENx___x___x900 ( "CSY"       , RSY_a, ASMFMT_RSY      , compare_and_swap_y                                  ),
 /*EB15*/ GENx___x___x___ ,
 /*EB16*/ GENx___x___x___ ,
 /*EB17*/ GENx___x___x___ ,
 /*EB18*/ GENx___x___x___ ,
 /*EB19*/ GENx___x___x___ ,
 /*EB1A*/ GENx___x___x___ ,
 /*EB1B*/ GENx___x___x___ ,
 /*EB1C*/ GENx___x___x900 ( "RLLG"      , RSY_a, ASMFMT_RSY      , rotate_left_single_logical_long                     ),
 /*EB1D*/ GENx37Xx390x900 ( "RLL"       , RSY_a, ASMFMT_RSY      , rotate_left_single_logical                          ),
 /*EB1E*/ GENx___x___x___ ,
 /*EB1F*/ GENx___x___x___ ,
 /*EB20*/ GENx___x___x900 ( "CLMH"      , RSY_b, ASMFMT_RSY      , compare_logical_characters_under_mask_high          ),
 /*EB21*/ GENx___x___x900 ( "CLMY"      , RSY_b, ASMFMT_RSY      , compare_logical_characters_under_mask_y             ),
 /*EB22*/ GENx___x___x___ ,
 /*EB23*/ GENx___x___x900 ( "CLT"       , RSY_b, ASMFMT_RSY      , compare_logical_and_trap                            ),
 /*EB24*/ GENx___x___x900 ( "STMG"      , RSY_a, ASMFMT_RSY      , store_multiple_long                                 ),
 /*EB25*/ GENx___x___x900 ( "STCTG"     , RSY_a, ASMFMT_RSY      , store_control_long                                  ),
 /*EB26*/ GENx___x___x900 ( "STMH"      , RSY_a, ASMFMT_RSY      , store_multiple_high                                 ),
 /*EB27*/ GENx___x___x___ ,
 /*EB28*/ GENx___x___x___ ,
 /*EB29*/ GENx___x___x___ ,
 /*EB2A*/ GENx___x___x___ ,
 /*EB2B*/ GENx___x___x900 ( "CLGT"      , RSY_b, ASMFMT_RSY      , compare_logical_and_trap_long                       ),
 /*EB2C*/ GENx___x___x900 ( "STCMH"     , RSY_b, ASMFMT_RSY      , store_characters_under_mask_high                    ),
 /*EB2D*/ GENx___x___x900 ( "STCMY"     , RSY_b, ASMFMT_RSY      , store_characters_under_mask_y                       ),
 /*EB2E*/ GENx___x___x___ ,
 /*EB2F*/ GENx___x___x900 ( "LCTLG"     , RSY_a, ASMFMT_RSY      , load_control_long                                   ),
 /*EB30*/ GENx___x___x900 ( "CSG"       , RSY_a, ASMFMT_RSY      , compare_and_swap_long                               ),
 /*EB31*/ GENx___x___x900 ( "CDSY"      , RSY_a, ASMFMT_RSY      , compare_double_and_swap_y                           ),
 /*EB32*/ GENx___x___x___ ,
 /*EB33*/ GENx___x___x___ ,
 /*EB34*/ GENx___x___x___ ,
 /*EB35*/ GENx___x___x___ ,
 /*EB36*/ GENx___x___x___ ,
 /*EB37*/ GENx___x___x___ ,
 /*EB38*/ GENx___x___x___ ,
 /*EB39*/ GENx___x___x___ ,
 /*EB3A*/ GENx___x___x___ ,
 /*EB3B*/ GENx___x___x___ ,
 /*EB3C*/ GENx___x___x___ ,
 /*EB3D*/ GENx___x___x___ ,
 /*EB3E*/ GENx___x___x900 ( "CDSG"      , RSY_a, ASMFMT_RSY      , compare_double_and_swap_long                        ),
 /*EB3F*/ GENx___x___x___ ,
 /*EB40*/ GENx___x___x___ ,
 /*EB41*/ GENx___x___x___ ,
 /*EB42*/ GENx___x___x___ ,
 /*EB43*/ GENx___x___x___ ,
 /*EB44*/ GENx___x___x900 ( "BXHG"      , RSY_a, ASMFMT_RSY      , branch_on_index_high_long                           ),
 /*EB45*/ GENx___x___x900 ( "BXLEG"     , RSY_a, ASMFMT_RSY      , branch_on_index_low_or_equal_long                   ),
 /*EB46*/ GENx___x___x___ ,
 /*EB47*/ GENx___x___x___ ,
 /*EB48*/ GENx___x___x___ ,
 /*EB49*/ GENx___x___x___ ,
 /*EB4A*/ GENx___x___x___ ,
 /*EB4B*/ GENx___x___x___ ,
 /*EB4C*/ GENx___x___x900 ( "ECAG"      , RSY_a, ASMFMT_RSY      , extract_cpu_attribute                               ),
 /*EB4D*/ GENx___x___x___ ,
 /*EB4E*/ GENx___x___x___ ,
 /*EB4F*/ GENx___x___x___ ,
 /*EB50*/ GENx___x___x___ ,
 /*EB51*/ GENx___x___x900 ( "TMY"       , SIY  , ASMFMT_SIY      , test_under_mask_y                                   ),
 /*EB52*/ GENx___x___x900 ( "MVIY"      , SIY  , ASMFMT_SIY      , move_immediate_y                                    ),
 /*EB53*/ GENx___x___x___ ,
 /*EB54*/ GENx___x___x900 ( "NIY"       , SIY  , ASMFMT_SIY      , and_immediate_y                                     ),
 /*EB55*/ GENx___x___x900 ( "CLIY"      , SIY  , ASMFMT_SIY      , compare_logical_immediate_y                         ),
 /*EB56*/ GENx___x___x900 ( "OIY"       , SIY  , ASMFMT_SIY      , or_immediate_y                                      ),
 /*EB57*/ GENx___x___x900 ( "XIY"       , SIY  , ASMFMT_SIY      , exclusive_or_immediate_y                            ),
 /*EB58*/ GENx___x___x___ ,
 /*EB59*/ GENx___x___x___ ,
 /*EB5A*/ GENx___x___x___ ,
 /*EB5B*/ GENx___x___x___ ,
 /*EB5C*/ GENx___x___x___ ,
 /*EB5D*/ GENx___x___x___ ,
 /*EB5E*/ GENx___x___x___ ,
 /*EB5F*/ GENx___x___x___ ,
 /*EB60*/ GENx___x___x___ ,
 /*EB61*/ GENx___x___x___ ,
 /*EB62*/ GENx___x___x___ ,
 /*EB63*/ GENx___x___x___ ,
 /*EB64*/ GENx___x___x___ ,
 /*EB65*/ GENx___x___x___ ,
 /*EB66*/ GENx___x___x___ ,
 /*EB67*/ GENx___x___x___ ,
 /*EB68*/ GENx___x___x___ ,
 /*EB69*/ GENx___x___x___ ,
 /*EB6A*/ GENx37Xx390x900 ( "ASI"       , SIY  , ASMFMT_SIY      , add_immediate_storage                               ),
 /*EB6B*/ GENx___x___x___ ,
 /*EB6C*/ GENx___x___x___ ,
 /*EB6D*/ GENx___x___x___ ,
 /*EB6E*/ GENx37Xx390x900 ( "ALSI"      , SIY  , ASMFMT_SIY      , add_logical_with_signed_immediate                   ),
 /*EB6F*/ GENx___x___x___ ,
 /*EB70*/ GENx___x___x___ ,
 /*EB71*/ GENx___x___x900 ( "LPSWEY"    , SIY  , ASMFMT_SIY      , load_program_status_word_extended_y                 ),
 /*EB72*/ GENx___x___x___ ,
 /*EB73*/ GENx___x___x___ ,
 /*EB74*/ GENx___x___x___ ,
 /*EB75*/ GENx___x___x___ ,
 /*EB76*/ GENx___x___x___ ,
 /*EB77*/ GENx___x___x___ ,
 /*EB78*/ GENx___x___x___ ,
 /*EB79*/ GENx___x___x___ ,
 /*EB7A*/ GENx37Xx390x900 ( "AGSI"      , SIY  , ASMFMT_SIY      , add_immediate_long_storage                          ),
 /*EB7B*/ GENx___x___x___ ,
 /*EB7C*/ GENx___x___x___ ,
 /*EB7D*/ GENx___x___x___ ,
 /*EB7E*/ GENx37Xx390x900 ( "ALGSI"     , SIY  , ASMFMT_SIY      , add_logical_with_signed_immediate_long              ),
 /*EB7F*/ GENx___x___x___ ,
 /*EB80*/ GENx___x___x900 ( "ICMH"      , RSY_b, ASMFMT_RSY      , insert_characters_under_mask_high                   ),
 /*EB81*/ GENx___x___x900 ( "ICMY"      , RSY_b, ASMFMT_RSY      , insert_characters_under_mask_y                      ),
 /*EB82*/ GENx___x___x___ ,
 /*EB83*/ GENx___x___x___ ,
 /*EB84*/ GENx___x___x___ ,
 /*EB85*/ GENx___x___x___ ,
 /*EB86*/ GENx___x___x___ ,
 /*EB87*/ GENx___x___x___ ,
 /*EB88*/ GENx___x___x___ ,
 /*EB89*/ GENx___x___x___ ,
 /*EB8A*/ GENx___x___x900 ( "SQBS"      , RSY_a, ASMFMT_RSY      , set_queue_buffer_state                              ),
 /*EB8B*/ GENx___x___x___ ,
 /*EB8C*/ GENx___x___x___ ,
 /*EB8D*/ GENx___x___x___ ,
 /*EB8E*/ GENx37Xx390x900 ( "MVCLU"     , RSY_a, ASMFMT_RSY      , move_long_unicode                                   ),
 /*EB8F*/ GENx37Xx390x900 ( "CLCLU"     , RSY_a, ASMFMT_RSY      , compare_logical_long_unicode                        ),
 /*EB90*/ GENx___x___x900 ( "STMY"      , RSY_a, ASMFMT_RSY      , store_multiple_y                                    ),
 /*EB91*/ GENx___x___x___ ,
 /*EB92*/ GENx___x___x___ ,
 /*EB93*/ GENx___x___x___ ,
 /*EB94*/ GENx___x___x___ ,
 /*EB95*/ GENx___x___x___ ,
 /*EB96*/ GENx___x___x900 ( "LMH"       , RSY_a, ASMFMT_RSY      , load_multiple_high                                  ),
 /*EB97*/ GENx___x___x___ ,
 /*EB98*/ GENx___x___x900 ( "LMY"       , RSY_a, ASMFMT_RSY      , load_multiple_y                                     ),
 /*EB99*/ GENx___x___x___ ,
 /*EB9A*/ GENx___x___x900 ( "LAMY"      , RSY_a, ASMFMT_RSY      , load_access_multiple_y                              ),
 /*EB9B*/ GENx___x___x900 ( "STAMY"     , RSY_a, ASMFMT_RSY      , store_access_multiple_y                             ),
 /*EB9C*/ GENx___x___x___ ,
 /*EB9D*/ GENx___x___x___ ,
 /*EB9E*/ GENx___x___x___ ,
 /*EB9F*/ GENx___x___x___ ,
 /*EBA0*/ GENx___x___x___ ,
 /*EBA1*/ GENx___x___x___ ,
 /*EBA2*/ GENx___x___x___ ,
 /*EBA3*/ GENx___x___x___ ,
 /*EBEB*/ GENx___x___x___ ,
 /*EBA5*/ GENx___x___x___ ,
 /*EBA6*/ GENx___x___x___ ,
 /*EBA7*/ GENx___x___x___ ,
 /*EBA8*/ GENx___x___x___ ,
 /*EBA9*/ GENx___x___x___ ,
 /*EBAA*/ GENx___x___x___ ,
 /*EBAB*/ GENx___x___x___ ,
 /*EBAC*/ GENx___x___x___ ,
 /*EBAD*/ GENx___x___x___ ,
 /*EBAE*/ GENx___x___x___ ,
 /*EBAF*/ GENx___x___x___ ,
 /*EBB0*/ GENx___x___x___ ,
 /*EBB1*/ GENx___x___x___ ,
 /*EBB2*/ GENx___x___x___ ,
 /*EBB3*/ GENx___x___x___ ,
 /*EBB4*/ GENx___x___x___ ,
 /*EBB5*/ GENx___x___x___ ,
 /*EBB6*/ GENx___x___x___ ,
 /*EBB7*/ GENx___x___x___ ,
 /*EBB8*/ GENx___x___x___ ,
 /*EBEB*/ GENx___x___x___ ,
 /*EBBA*/ GENx___x___x___ ,
 /*EBBB*/ GENx___x___x___ ,
 /*EBBC*/ GENx___x___x___ ,
 /*EBBD*/ GENx___x___x___ ,
 /*EBBE*/ GENx___x___x___ ,
 /*EBBF*/ GENx___x___x___ ,
 /*EBC0*/ GENx37Xx390x900 ( "TP"        , RSL_a, ASMFMT_RSL      , test_decimal                                        ),
 /*EBC1*/ GENx___x___x___ ,
 /*EBC2*/ GENx___x___x___ ,
 /*EBC3*/ GENx___x___x___ ,
 /*EBC4*/ GENx___x___x___ ,
 /*EBC5*/ GENx___x___x___ ,
 /*EBC6*/ GENx___x___x___ ,
 /*EBC7*/ GENx___x___x___ ,
 /*EBC8*/ GENx___x___x___ ,
 /*EBC9*/ GENx___x___x___ ,
 /*EBCA*/ GENx___x___x___ ,
 /*EBCB*/ GENx___x___x___ ,
 /*EBCC*/ GENx___x___x___ ,
 /*EBCD*/ GENx___x___x___ ,
 /*EBCE*/ GENx___x___x___ ,
 /*EBCF*/ GENx___x___x___ ,
 /*EBD0*/ GENx___x___x___ ,
 /*EBD1*/ GENx___x___x___ ,
 /*EBD2*/ GENx___x___x___ ,
 /*EBD3*/ GENx___x___x___ ,
 /*EBD4*/ GENx___x___x___ ,
 /*EBD5*/ GENx___x___x___ ,
 /*EBD6*/ GENx___x___x___ ,
 /*EBD7*/ GENx___x___x___ ,
 /*EBD8*/ GENx___x___x___ ,
 /*EBD9*/ GENx___x___x___ ,
 /*EBDA*/ GENx___x___x___ ,
 /*EBDB*/ GENx___x___x___ ,
 /*EBDC*/ GENx37Xx390x900 ( "SRAK"      , RSY_a, ASMFMT_RSY      , shift_right_single_distinct                         ),
 /*EBDD*/ GENx37Xx390x900 ( "SLAK"      , RSY_a, ASMFMT_RSY      , shift_left_single_distinct                          ),
 /*EBDE*/ GENx37Xx390x900 ( "SRLK"      , RSY_a, ASMFMT_RSY      , shift_right_single_logical_distinct                 ),
 /*EBDF*/ GENx37Xx390x900 ( "SLLK"      , RSY_a, ASMFMT_RSY      , shift_left_single_logical_distinct                  ),
 /*EBE0*/ GENx___x___x900 ( "LOCFH"     , RSY_b, ASMFMT_RSY_M3   , load_high_on_condition                              ),
 /*EBE1*/ GENx___x___x900 ( "STOCFH"    , RSY_b, ASMFMT_RSY_M3   , store_high_on_condition                             ),
 /*EBE2*/ GENx___x___x900 ( "LOCG"      , RSY_b, ASMFMT_RSY_M3   , load_on_condition_long                              ),
 /*EBE3*/ GENx___x___x900 ( "STOCG"     , RSY_b, ASMFMT_RSY_M3   , store_on_condition_long                             ),
 /*EBE4*/ GENx___x___x900 ( "LANG"      , RSY_a, ASMFMT_RSY      , load_and_and_long                                   ),
 /*EBE5*/ GENx___x___x___ ,
 /*EBE6*/ GENx___x___x900 ( "LAOG"      , RSY_a, ASMFMT_RSY      , load_and_or_long                                    ),
 /*EBE7*/ GENx___x___x900 ( "LAXG"      , RSY_a, ASMFMT_RSY      , load_and_exclusive_or_long                          ),
 /*EBE8*/ GENx___x___x900 ( "LAAG"      , RSY_a, ASMFMT_RSY      , load_and_add_long                                   ),
 /*EBE9*/ GENx___x___x___ ,
 /*EBEA*/ GENx___x___x900 ( "LAALG"     , RSY_a, ASMFMT_RSY      , load_and_add_logical_long                           ),
 /*EBEB*/ GENx___x___x___ ,
 /*EBEC*/ GENx___x___x___ ,
 /*EBED*/ GENx___x___x___ ,
 /*EBEE*/ GENx___x___x___ ,
 /*EBEF*/ GENx___x___x___ ,
 /*EBF0*/ GENx___x___x___ ,
 /*EBF1*/ GENx___x___x___ ,
 /*EBF2*/ GENx37Xx390x900 ( "LOC"       , RSY_b, ASMFMT_RSY_M3   , load_on_condition                                   ),
 /*EBF3*/ GENx37Xx390x900 ( "STOC"      , RSY_b, ASMFMT_RSY_M3   , store_on_condition                                  ),
 /*EBF4*/ GENx37Xx390x900 ( "LAN"       , RSY_a, ASMFMT_RSY      , load_and_and                                        ),
 /*EBF5*/ GENx___x___x___ ,
 /*EBF6*/ GENx37Xx390x900 ( "LAO"       , RSY_a, ASMFMT_RSY      , load_and_or                                         ),
 /*EBF7*/ GENx37Xx390x900 ( "LAX"       , RSY_a, ASMFMT_RSY      , load_and_exclusive_or                               ),
 /*EBF8*/ GENx37Xx390x900 ( "LAA"       , RSY_a, ASMFMT_RSY      , load_and_add                                        ),
 /*EBF9*/ GENx___x___x___ ,
 /*EBFA*/ GENx37Xx390x900 ( "LAAL"      , RSY_a, ASMFMT_RSY      , load_and_add_logical                                ),
 /*EBFB*/ GENx___x___x___ ,
 /*EBFC*/ GENx___x___x___ ,
 /*EBFD*/ GENx___x___x___ ,
 /*EBFE*/ GENx___x___x___ ,
 /*EBFF*/ GENx___x___x___
};

static INSTR_FUNC gen_opcode_ecxx[256][NUM_INSTR_TAB_PTRS] =
{
 /*EC00*/ GENx___x___x___ ,
 /*EC01*/ GENx___x___x___ ,
 /*EC02*/ GENx___x___x___ ,
 /*EC03*/ GENx___x___x___ ,
 /*EC04*/ GENx___x___x___ ,
 /*EC05*/ GENx___x___x___ ,
 /*EC06*/ GENx___x___x___ ,
 /*EC07*/ GENx___x___x___ ,
 /*EC08*/ GENx___x___x___ ,
 /*EC09*/ GENx___x___x___ ,
 /*EC0A*/ GENx___x___x___ ,
 /*EC0B*/ GENx___x___x___ ,
 /*EC0C*/ GENx___x___x___ ,
 /*EC0D*/ GENx___x___x___ ,
 /*EC0E*/ GENx___x___x___ ,
 /*EC0F*/ GENx___x___x___ ,
 /*EC10*/ GENx___x___x___ ,
 /*EC11*/ GENx___x___x___ ,
 /*EC12*/ GENx___x___x___ ,
 /*EC13*/ GENx___x___x___ ,
 /*EC14*/ GENx___x___x___ ,
 /*EC15*/ GENx___x___x___ ,
 /*EC16*/ GENx___x___x___ ,
 /*EC17*/ GENx___x___x___ ,
 /*EC18*/ GENx___x___x___ ,
 /*EC19*/ GENx___x___x___ ,
 /*EC1A*/ GENx___x___x___ ,
 /*EC1B*/ GENx___x___x___ ,
 /*EC1C*/ GENx___x___x___ ,
 /*EC1D*/ GENx___x___x___ ,
 /*EC1E*/ GENx___x___x___ ,
 /*EC1F*/ GENx___x___x___ ,
 /*EC20*/ GENx___x___x___ ,
 /*EC21*/ GENx___x___x___ ,
 /*EC22*/ GENx___x___x___ ,
 /*EC23*/ GENx___x___x___ ,
 /*EC24*/ GENx___x___x___ ,
 /*EC25*/ GENx___x___x___ ,
 /*EC26*/ GENx___x___x___ ,
 /*EC27*/ GENx___x___x___ ,
 /*EC28*/ GENx___x___x___ ,
 /*EC29*/ GENx___x___x___ ,
 /*EC2A*/ GENx___x___x___ ,
 /*EC2B*/ GENx___x___x___ ,
 /*EC2C*/ GENx___x___x___ ,
 /*EC2D*/ GENx___x___x___ ,
 /*EC2E*/ GENx___x___x___ ,
 /*EC2F*/ GENx___x___x___ ,
 /*EC30*/ GENx___x___x___ ,
 /*EC31*/ GENx___x___x___ ,
 /*EC32*/ GENx___x___x___ ,
 /*EC33*/ GENx___x___x___ ,
 /*EC34*/ GENx___x___x___ ,
 /*EC35*/ GENx___x___x___ ,
 /*EC36*/ GENx___x___x___ ,
 /*EC37*/ GENx___x___x___ ,
 /*EC38*/ GENx___x___x___ ,
 /*EC39*/ GENx___x___x___ ,
 /*EC3A*/ GENx___x___x___ ,
 /*EC3B*/ GENx___x___x___ ,
 /*EC3C*/ GENx___x___x___ ,
 /*EC3D*/ GENx___x___x___ ,
 /*EC3E*/ GENx___x___x___ ,
 /*EC3F*/ GENx___x___x___ ,
 /*EC40*/ GENx___x___x___ ,
 /*EC41*/ GENx___x___x___ ,
 /*EC42*/ GENx___x___x900 ( "LOCHI"     , RIE_g, ASMFMT_RIE_G    , load_halfword_immediate_on_condition                ),
 /*EC43*/ GENx___x___x___ ,
 /*EC44*/ GENx___x___x900 ( "BRXHG"     , RIE_e, ASMFMT_RIE      , branch_relative_on_index_high_long                  ),
 /*EC45*/ GENx___x___x900 ( "BRXLG"     , RIE_e, ASMFMT_RIE      , branch_relative_on_index_low_or_equal_long          ),
 /*EC46*/ GENx___x___x900 ( "LOCGHI"    , RIE_g, ASMFMT_RIE_G    , load_halfword_immediate_on_condition_grande         ),
 /*EC47*/ GENx___x___x___ ,
 /*EC48*/ GENx___x___x___ ,
 /*EC49*/ GENx___x___x___ ,
 /*EC4A*/ GENx___x___x___ ,
 /*EC4B*/ GENx___x___x___ ,
 /*EC4C*/ GENx___x___x___ ,
 /*EC4D*/ GENx___x___x___ ,
 /*EC4E*/ GENx___x___x900 ( "LOCHHI"    , RIE_g, ASMFMT_RIE_G    , load_halfword_high_immediate_on_condition           ),
 /*EC4F*/ GENx___x___x___ ,
 /*EC50*/ GENx___x___x___ ,
 /*EC51*/ GENx___x___x900 ( "RISBLG"    , RIE_f, ASMFMT_RIE_RRIII, rotate_then_insert_selected_bits_low_long_reg       ),
 /*EC52*/ GENx___x___x___ ,
 /*EC53*/ GENx___x___x___ ,
 /*EC54*/ GENx___x___x900 ( "RNSBG"     , RIE_f, ASMFMT_RIE_RRIII, rotate_then_and_selected_bits_long_reg              ),
 /*EC55*/ GENx___x___x900 ( "RISBG"     , RIE_f, ASMFMT_RIE_RRIII, rotate_then_insert_selected_bits_long_reg           ),
 /*EC56*/ GENx___x___x900 ( "ROSBG"     , RIE_f, ASMFMT_RIE_RRIII, rotate_then_or_selected_bits_long_reg               ),
 /*EC57*/ GENx___x___x900 ( "RXSBG"     , RIE_f, ASMFMT_RIE_RRIII, rotate_then_exclusive_or_selected_bits_long_reg     ),
 /*EC58*/ GENx___x___x___ ,
 /*EC59*/ GENx___x___x900 ( "RISBGN"    , RIE_f, ASMFMT_RIE_RRIII, rotate_then_insert_selected_bits_long_reg_n         ),
 /*EC5A*/ GENx___x___x___ ,
 /*EC5B*/ GENx___x___x___ ,
 /*EC5C*/ GENx___x___x___ ,
 /*EC5D*/ GENx___x___x900 ( "RISBHG"    , RIE_f, ASMFMT_RIE_RRIII, rotate_then_insert_selected_bits_high_long_reg      ),
 /*EC5E*/ GENx___x___x___ ,
 /*EC5F*/ GENx___x___x___ ,
 /*EC60*/ GENx___x___x___ ,
 /*EC61*/ GENx___x___x___ ,
 /*EC62*/ GENx___x___x___ ,
 /*EC63*/ GENx___x___x___ ,
 /*EC64*/ GENx___x___x900 ( "CGRJ"      , RIE_b, ASMFMT_RIE_RRIM , compare_and_branch_relative_long_register           ),
 /*EC65*/ GENx___x___x900 ( "CLGRJ"     , RIE_b, ASMFMT_RIE_RRIM , compare_logical_and_branch_relative_long_register   ),
 /*EC66*/ GENx___x___x___ ,
 /*EC67*/ GENx___x___x___ ,
 /*EC68*/ GENx___x___x___ ,
 /*EC69*/ GENx___x___x___ ,
 /*EC6A*/ GENx___x___x___ ,
 /*EC6B*/ GENx___x___x___ ,
 /*EC6C*/ GENx___x___x___ ,
 /*EC6D*/ GENx___x___x___ ,
 /*EC6E*/ GENx___x___x___ ,
 /*EC6F*/ GENx___x___x___ ,
 /*EC70*/ GENx___x___x900 ( "CGIT"      , RIE_a, ASMFMT_RIE_RIM  , compare_immediate_and_trap_long                     ),
 /*EC71*/ GENx___x___x900 ( "CLGIT"     , RIE_a, ASMFMT_RIE_RIM  , compare_logical_immediate_and_trap_long             ),
 /*EC72*/ GENx37Xx390x900 ( "CIT"       , RIE_a, ASMFMT_RIE_RIM  , compare_immediate_and_trap                          ),
 /*EC73*/ GENx37Xx390x900 ( "CLFIT"     , RIE_a, ASMFMT_RIE_RIM  , compare_logical_immediate_and_trap_fullword         ),
 /*EC74*/ GENx___x___x___ ,
 /*EC75*/ GENx___x___x___ ,
 /*EC76*/ GENx37Xx390x900 ( "CRJ"       , RIE_b, ASMFMT_RIE_RRIM , compare_and_branch_relative_register                ),
 /*EC77*/ GENx37Xx390x900 ( "CLRJ"      , RIE_b, ASMFMT_RIE_RRIM , compare_logical_and_branch_relative_register        ),
 /*EC78*/ GENx___x___x___ ,
 /*EC79*/ GENx___x___x___ ,
 /*EC7A*/ GENx___x___x___ ,
 /*EC7B*/ GENx___x___x___ ,
 /*EC7C*/ GENx___x___x900 ( "CGIJ"      , RIE_c, ASMFMT_RIE_RMII , compare_immediate_and_branch_relative_long          ),
 /*EC7D*/ GENx___x___x900 ( "CLGIJ"     , RIE_c, ASMFMT_RIE_RMII , compare_logical_immediate_and_branch_relative_long  ),
 /*EC7E*/ GENx37Xx390x900 ( "CIJ"       , RIE_c, ASMFMT_RIE_RMII , compare_immediate_and_branch_relative               ),
 /*EC7F*/ GENx37Xx390x900 ( "CLIJ"      , RIE_c, ASMFMT_RIE_RMII , compare_logical_immediate_and_branch_relative       ),
 /*EC80*/ GENx___x___x___ ,
 /*EC81*/ GENx___x___x___ ,
 /*EC82*/ GENx___x___x___ ,
 /*EC83*/ GENx___x___x___ ,
 /*EC84*/ GENx___x___x___ ,
 /*EC85*/ GENx___x___x___ ,
 /*EC86*/ GENx___x___x___ ,
 /*EC87*/ GENx___x___x___ ,
 /*EC88*/ GENx___x___x___ ,
 /*EC89*/ GENx___x___x___ ,
 /*EC8A*/ GENx___x___x___ ,
 /*EC8B*/ GENx___x___x___ ,
 /*EC8C*/ GENx___x___x___ ,
 /*EC8D*/ GENx___x___x___ ,
 /*EC8E*/ GENx___x___x___ ,
 /*EC8F*/ GENx___x___x___ ,
 /*EC90*/ GENx___x___x___ ,
 /*EC91*/ GENx___x___x___ ,
 /*EC92*/ GENx___x___x___ ,
 /*EC93*/ GENx___x___x___ ,
 /*EC94*/ GENx___x___x___ ,
 /*EC95*/ GENx___x___x___ ,
 /*EC96*/ GENx___x___x___ ,
 /*EC97*/ GENx___x___x___ ,
 /*EC98*/ GENx___x___x___ ,
 /*EC99*/ GENx___x___x___ ,
 /*EC9A*/ GENx___x___x___ ,
 /*EC9B*/ GENx___x___x___ ,
 /*EC9C*/ GENx___x___x___ ,
 /*EC9D*/ GENx___x___x___ ,
 /*EC9E*/ GENx___x___x___ ,
 /*EC9F*/ GENx___x___x___ ,
 /*ECA0*/ GENx___x___x___ ,
 /*ECA1*/ GENx___x___x___ ,
 /*ECA2*/ GENx___x___x___ ,
 /*ECA3*/ GENx___x___x___ ,
 /*ECA4*/ GENx___x___x___ ,
 /*ECA5*/ GENx___x___x___ ,
 /*ECA6*/ GENx___x___x___ ,
 /*ECA7*/ GENx___x___x___ ,
 /*ECA8*/ GENx___x___x___ ,
 /*ECA9*/ GENx___x___x___ ,
 /*ECAA*/ GENx___x___x___ ,
 /*ECAB*/ GENx___x___x___ ,
 /*ECAC*/ GENx___x___x___ ,
 /*ECAD*/ GENx___x___x___ ,
 /*ECAE*/ GENx___x___x___ ,
 /*ECAF*/ GENx___x___x___ ,
 /*ECB0*/ GENx___x___x___ ,
 /*ECB1*/ GENx___x___x___ ,
 /*ECB2*/ GENx___x___x___ ,
 /*ECB3*/ GENx___x___x___ ,
 /*ECB4*/ GENx___x___x___ ,
 /*ECB5*/ GENx___x___x___ ,
 /*ECB6*/ GENx___x___x___ ,
 /*ECB7*/ GENx___x___x___ ,
 /*ECB8*/ GENx___x___x___ ,
 /*ECB9*/ GENx___x___x___ ,
 /*ECBA*/ GENx___x___x___ ,
 /*ECBB*/ GENx___x___x___ ,
 /*ECBC*/ GENx___x___x___ ,
 /*ECBD*/ GENx___x___x___ ,
 /*ECBE*/ GENx___x___x___ ,
 /*ECBF*/ GENx___x___x___ ,
 /*ECC0*/ GENx___x___x___ ,
 /*ECC1*/ GENx___x___x___ ,
 /*ECC2*/ GENx___x___x___ ,
 /*ECC3*/ GENx___x___x___ ,
 /*ECC4*/ GENx___x___x___ ,
 /*ECC5*/ GENx___x___x___ ,
 /*ECC6*/ GENx___x___x___ ,
 /*ECC7*/ GENx___x___x___ ,
 /*ECC8*/ GENx___x___x___ ,
 /*ECC9*/ GENx___x___x___ ,
 /*ECCA*/ GENx___x___x___ ,
 /*ECCB*/ GENx___x___x___ ,
 /*ECCC*/ GENx___x___x___ ,
 /*ECCD*/ GENx___x___x___ ,
 /*ECCE*/ GENx___x___x___ ,
 /*ECCF*/ GENx___x___x___ ,
 /*ECD0*/ GENx___x___x___ ,
 /*ECD1*/ GENx___x___x___ ,
 /*ECD2*/ GENx___x___x___ ,
 /*ECD3*/ GENx___x___x___ ,
 /*ECD4*/ GENx___x___x___ ,
 /*ECD5*/ GENx___x___x___ ,
 /*ECD6*/ GENx___x___x___ ,
 /*ECD7*/ GENx___x___x___ ,
 /*ECD8*/ GENx37Xx390x900 ( "AHIK"      , RIE_d, ASMFMT_RIE_RRI  , add_distinct_halfword_immediate                     ),
 /*ECD9*/ GENx___x___x900 ( "AGHIK"     , RIE_d, ASMFMT_RIE_RRI  , add_distinct_long_halfword_immediate                ),
 /*ECDA*/ GENx37Xx390x900 ( "ALHSIK"    , RIE_d, ASMFMT_RIE_RRI  , add_logical_distinct_signed_halfword_immediate      ),
 /*ECDB*/ GENx___x___x900 ( "AGLHSIK"   , RIE_d, ASMFMT_RIE_RRI  , add_logical_distinct_long_signed_halfword_immediate ),
 /*ECDC*/ GENx___x___x___ ,
 /*ECDD*/ GENx___x___x___ ,
 /*ECDE*/ GENx___x___x___ ,
 /*ECDF*/ GENx___x___x___ ,
 /*ECE0*/ GENx___x___x___ ,
 /*ECE1*/ GENx___x___x___ ,
 /*ECE2*/ GENx___x___x___ ,
 /*ECE3*/ GENx___x___x___ ,
 /*ECE4*/ GENx___x___x900 ( "CGRB"      , RRS  , ASMFMT_RRS      , compare_and_branch_long_register                    ),
 /*ECE5*/ GENx___x___x900 ( "CLGRB"     , RRS  , ASMFMT_RRS      , compare_logical_and_branch_long_register            ),
 /*ECE6*/ GENx___x___x___ ,
 /*ECE7*/ GENx___x___x___ ,
 /*ECE8*/ GENx___x___x___ ,
 /*ECE9*/ GENx___x___x___ ,
 /*ECEA*/ GENx___x___x___ ,
 /*ECEB*/ GENx___x___x___ ,
 /*ECEC*/ GENx___x___x___ ,
 /*ECED*/ GENx___x___x___ ,
 /*ECEE*/ GENx___x___x___ ,
 /*ECEF*/ GENx___x___x___ ,
 /*ECF0*/ GENx___x___x___ ,
 /*ECF1*/ GENx___x___x___ ,
 /*ECF2*/ GENx___x___x___ ,
 /*ECF3*/ GENx___x___x___ ,
 /*ECF4*/ GENx___x___x___ ,
 /*ECF5*/ GENx___x___x___ ,
 /*ECF6*/ GENx37Xx390x900 ( "CRB"       , RRS  , ASMFMT_RRS      , compare_and_branch_register                         ),
 /*ECF7*/ GENx37Xx390x900 ( "CLRB"      , RRS  , ASMFMT_RRS      , compare_logical_and_branch_register                 ),
 /*ECF8*/ GENx___x___x___ ,
 /*ECF9*/ GENx___x___x___ ,
 /*ECFA*/ GENx___x___x___ ,
 /*ECFB*/ GENx___x___x___ ,
 /*ECFC*/ GENx___x___x900 ( "CGIB"      , RIS  , ASMFMT_RIS      , compare_immediate_and_branch_long                   ),
 /*ECFD*/ GENx___x___x900 ( "CLGIB"     , RIS  , ASMFMT_RIS      , compare_logical_immediate_and_branch_long           ),
 /*ECFE*/ GENx37Xx390x900 ( "CIB"       , RIS  , ASMFMT_RIS      , compare_immediate_and_branch                        ),
 /*ECFF*/ GENx37Xx390x900 ( "CLIB"      , RIS  , ASMFMT_RIS      , compare_logical_immediate_and_branch                )
};

static INSTR_FUNC gen_opcode_edxx[256][NUM_INSTR_TAB_PTRS] =
{
 /*ED00*/ GENx___x___x___ ,
 /*ED01*/ GENx___x___x___ ,
 /*ED02*/ GENx___x___x___ ,
 /*ED03*/ GENx___x___x___ ,
 /*ED04*/ GENx37Xx390x900 ( "LDEB"      , RXE  , ASMFMT_RXE      , load_lengthened_bfp_short_to_long                   ),
 /*ED05*/ GENx37Xx390x900 ( "LXDB"      , RXE  , ASMFMT_RXE      , load_lengthened_bfp_long_to_ext                     ),
 /*ED06*/ GENx37Xx390x900 ( "LXEB"      , RXE  , ASMFMT_RXE      , load_lengthened_bfp_short_to_ext                    ),
 /*ED07*/ GENx37Xx390x900 ( "MXDB"      , RXE  , ASMFMT_RXE      , multiply_bfp_long_to_ext                            ),
 /*ED08*/ GENx37Xx390x900 ( "KEB"       , RXE  , ASMFMT_RXE      , compare_and_signal_bfp_short                        ),
 /*ED09*/ GENx37Xx390x900 ( "CEB"       , RXE  , ASMFMT_RXE      , compare_bfp_short                                   ),
 /*ED0A*/ GENx37Xx390x900 ( "AEB"       , RXE  , ASMFMT_RXE      , add_bfp_short                                       ),
 /*ED0B*/ GENx37Xx390x900 ( "SEB"       , RXE  , ASMFMT_RXE      , subtract_bfp_short                                  ),
 /*ED0C*/ GENx37Xx390x900 ( "MDEB"      , RXE  , ASMFMT_RXE      , multiply_bfp_short_to_long                          ),
 /*ED0D*/ GENx37Xx390x900 ( "DEB"       , RXE  , ASMFMT_RXE      , divide_bfp_short                                    ),
 /*ED0E*/ GENx37Xx390x900 ( "MAEB"      , RXF  , ASMFMT_RXF      , multiply_add_bfp_short                              ),
 /*ED0F*/ GENx37Xx390x900 ( "MSEB"      , RXF  , ASMFMT_RXF      , multiply_subtract_bfp_short                         ),
 /*ED10*/ GENx37Xx390x900 ( "TCEB"      , RXE  , ASMFMT_RXE      , test_data_class_bfp_short                           ),
 /*ED11*/ GENx37Xx390x900 ( "TCDB"      , RXE  , ASMFMT_RXE      , test_data_class_bfp_long                            ),
 /*ED12*/ GENx37Xx390x900 ( "TCXB"      , RXE  , ASMFMT_RXE      , test_data_class_bfp_ext                             ),
 /*ED13*/ GENx___x___x___ ,
 /*ED14*/ GENx37Xx390x900 ( "SQEB"      , RXE  , ASMFMT_RXE      , squareroot_bfp_short                                ),
 /*ED15*/ GENx37Xx390x900 ( "SQDB"      , RXE  , ASMFMT_RXE      , squareroot_bfp_long                                 ),
 /*ED16*/ GENx___x___x___ ,
 /*ED17*/ GENx37Xx390x900 ( "MEEB"      , RXE  , ASMFMT_RXE      , multiply_bfp_short                                  ),
 /*ED18*/ GENx37Xx390x900 ( "KDB"       , RXE  , ASMFMT_RXE      , compare_and_signal_bfp_long                         ),
 /*ED19*/ GENx37Xx390x900 ( "CDB"       , RXE  , ASMFMT_RXE      , compare_bfp_long                                    ),
 /*ED1A*/ GENx37Xx390x900 ( "ADB"       , RXE  , ASMFMT_RXE      , add_bfp_long                                        ),
 /*ED1B*/ GENx37Xx390x900 ( "SDB"       , RXE  , ASMFMT_RXE      , subtract_bfp_long                                   ),
 /*ED1C*/ GENx37Xx390x900 ( "MDB"       , RXE  , ASMFMT_RXE      , multiply_bfp_long                                   ),
 /*ED1D*/ GENx37Xx390x900 ( "DDB"       , RXE  , ASMFMT_RXE      , divide_bfp_long                                     ),
 /*ED1E*/ GENx37Xx390x900 ( "MADB"      , RXF  , ASMFMT_RXF      , multiply_add_bfp_long                               ),
 /*ED1F*/ GENx37Xx390x900 ( "MSDB"      , RXF  , ASMFMT_RXF      , multiply_subtract_bfp_long                          ),
 /*ED20*/ GENx___x___x___ ,
 /*ED21*/ GENx___x___x___ ,
 /*ED22*/ GENx___x___x___ ,
 /*ED23*/ GENx___x___x___ ,
 /*ED24*/ GENx37Xx390x900 ( "LDE"       , RXE  , ASMFMT_RXE      , load_lengthened_float_short_to_long                 ),
 /*ED25*/ GENx37Xx390x900 ( "LXD"       , RXE  , ASMFMT_RXE      , load_lengthened_float_long_to_ext                   ),
 /*ED26*/ GENx37Xx390x900 ( "LXE"       , RXE  , ASMFMT_RXE      , load_lengthened_float_short_to_ext                  ),
 /*ED27*/ GENx___x___x___ ,
 /*ED28*/ GENx___x___x___ ,
 /*ED29*/ GENx___x___x___ ,
 /*ED2A*/ GENx___x___x___ ,
 /*ED2B*/ GENx___x___x___ ,
 /*ED2C*/ GENx___x___x___ ,
 /*ED2D*/ GENx___x___x___ ,
 /*ED2E*/ GENx37Xx390x900 ( "MAE"       , RXF  , ASMFMT_RXF      , multiply_add_float_short                            ),
 /*ED2F*/ GENx37Xx390x900 ( "MSE"       , RXF  , ASMFMT_RXF      , multiply_subtract_float_short                       ),
 /*ED30*/ GENx___x___x___ ,
 /*ED31*/ GENx___x___x___ ,
 /*ED32*/ GENx___x___x___ ,
 /*ED33*/ GENx___x___x___ ,
 /*ED34*/ GENx37Xx390x900 ( "SQE"       , RXE  , ASMFMT_RXE      , squareroot_float_short                              ),
 /*ED35*/ GENx37Xx390x900 ( "SQD"       , RXE  , ASMFMT_RXE      , squareroot_float_long                               ),
 /*ED36*/ GENx___x___x___ ,
 /*ED37*/ GENx37Xx390x900 ( "MEE"       , RXE  , ASMFMT_RXE      , multiply_float_short                                ),
 /*ED38*/ GENx37Xx___x900 ( "MAYL"      , RXF  , ASMFMT_RXF      , multiply_add_unnormal_float_long_to_ext_low         ),
 /*ED39*/ GENx37Xx___x900 ( "MYL"       , RXF  , ASMFMT_RXF      , multiply_unnormal_float_long_to_ext_low             ),
 /*ED3A*/ GENx37Xx___x900 ( "MAY"       , RXF  , ASMFMT_RXF      , multiply_add_unnormal_float_long_to_ext             ),
 /*ED3B*/ GENx37Xx___x900 ( "MY"        , RXF  , ASMFMT_RXF      , multiply_unnormal_float_long_to_ext                 ),
 /*ED3C*/ GENx37Xx___x900 ( "MAYH"      , RXF  , ASMFMT_RXF      , multiply_add_unnormal_float_long_to_ext_high        ),
 /*ED3D*/ GENx37Xx___x900 ( "MYH"       , RXF  , ASMFMT_RXF      , multiply_unnormal_float_long_to_ext_high            ),
 /*ED3E*/ GENx37Xx390x900 ( "MAD"       , RXF  , ASMFMT_RXF      , multiply_add_float_long                             ),
 /*ED3F*/ GENx37Xx390x900 ( "MSD"       , RXF  , ASMFMT_RXF      , multiply_subtract_float_long                        ),
 /*ED40*/ GENx___x390x900 ( "SLDT"      , RXF  , ASMFMT_RXF      , shift_coefficient_left_dfp_long                     ),
 /*ED41*/ GENx___x390x900 ( "SRDT"      , RXF  , ASMFMT_RXF      , shift_coefficient_right_dfp_long                    ),
 /*ED42*/ GENx___x___x___ ,
 /*ED43*/ GENx___x___x___ ,
 /*ED44*/ GENx___x___x___ ,
 /*ED45*/ GENx___x___x___ ,
 /*ED46*/ GENx___x___x___ ,
 /*ED47*/ GENx___x___x___ ,
 /*ED48*/ GENx___x390x900 ( "SLXT"      , RXF  , ASMFMT_RXF      , shift_coefficient_left_dfp_ext                      ),
 /*ED49*/ GENx___x390x900 ( "SRXT"      , RXF  , ASMFMT_RXF      , shift_coefficient_right_dfp_ext                     ),
 /*ED4A*/ GENx___x___x___ ,
 /*ED4B*/ GENx___x___x___ ,
 /*ED4C*/ GENx___x___x___ ,
 /*ED4D*/ GENx___x___x___ ,
 /*ED4E*/ GENx___x___x___ ,
 /*ED4F*/ GENx___x___x___ ,
 /*ED50*/ GENx___x390x900 ( "TDCET"     , RXE  , ASMFMT_RXE      , test_data_class_dfp_short                           ),
 /*ED51*/ GENx___x390x900 ( "TDGET"     , RXE  , ASMFMT_RXE      , test_data_group_dfp_short                           ),
 /*ED52*/ GENx___x___x___ ,
 /*ED53*/ GENx___x___x___ ,
 /*ED54*/ GENx___x390x900 ( "TDCDT"     , RXE  , ASMFMT_RXE      , test_data_class_dfp_long                            ),
 /*ED55*/ GENx___x390x900 ( "TDGDT"     , RXE  , ASMFMT_RXE      , test_data_group_dfp_long                            ),
 /*ED56*/ GENx___x___x___ ,
 /*ED57*/ GENx___x___x___ ,
 /*ED58*/ GENx___x390x900 ( "TDCXT"     , RXE  , ASMFMT_RXE      , test_data_class_dfp_ext                             ),
 /*ED59*/ GENx___x390x900 ( "TDGXT"     , RXE  , ASMFMT_RXE      , test_data_group_dfp_ext                             ),
 /*ED5A*/ GENx___x___x___ ,
 /*ED5B*/ GENx___x___x___ ,
 /*ED5C*/ GENx___x___x___ ,
 /*ED5D*/ GENx___x___x___ ,
 /*ED5E*/ GENx___x___x___ ,
 /*ED5F*/ GENx___x___x___ ,
 /*ED60*/ GENx___x___x___ ,
 /*ED61*/ GENx___x___x___ ,
 /*ED62*/ GENx___x___x___ ,
 /*ED63*/ GENx___x___x___ ,
 /*ED64*/ GENx___x___x900 ( "LEY"       , RXY_a, ASMFMT_RXY      , load_float_short_y                                  ),
 /*ED65*/ GENx___x___x900 ( "LDY"       , RXY_a, ASMFMT_RXY      , load_float_long_y                                   ),
 /*ED66*/ GENx___x___x900 ( "STEY"      , RXY_a, ASMFMT_RXY      , store_float_short_y                                 ),
 /*ED67*/ GENx___x___x900 ( "STDY"      , RXY_a, ASMFMT_RXY      , store_float_long_y                                  ),
 /*ED68*/ GENx___x___x___ ,
 /*ED69*/ GENx___x___x___ ,
 /*ED6A*/ GENx___x___x___ ,
 /*ED6B*/ GENx___x___x___ ,
 /*ED6C*/ GENx___x___x___ ,
 /*ED6D*/ GENx___x___x___ ,
 /*ED6E*/ GENx___x___x___ ,
 /*ED6F*/ GENx___x___x___ ,
 /*ED70*/ GENx___x___x___ ,
 /*ED71*/ GENx___x___x___ ,
 /*ED72*/ GENx___x___x___ ,
 /*ED73*/ GENx___x___x___ ,
 /*ED74*/ GENx___x___x___ ,
 /*ED75*/ GENx___x___x___ ,
 /*ED76*/ GENx___x___x___ ,
 /*ED77*/ GENx___x___x___ ,
 /*ED78*/ GENx___x___x___ ,
 /*ED79*/ GENx___x___x___ ,
 /*ED7A*/ GENx___x___x___ ,
 /*ED7B*/ GENx___x___x___ ,
 /*ED7C*/ GENx___x___x___ ,
 /*ED7D*/ GENx___x___x___ ,
 /*ED7E*/ GENx___x___x___ ,
 /*ED7F*/ GENx___x___x___ ,
 /*ED80*/ GENx___x___x___ ,
 /*ED81*/ GENx___x___x___ ,
 /*ED82*/ GENx___x___x___ ,
 /*ED83*/ GENx___x___x___ ,
 /*ED84*/ GENx___x___x___ ,
 /*ED85*/ GENx___x___x___ ,
 /*ED86*/ GENx___x___x___ ,
 /*ED87*/ GENx___x___x___ ,
 /*ED88*/ GENx___x___x___ ,
 /*ED89*/ GENx___x___x___ ,
 /*ED8A*/ GENx___x___x___ ,
 /*ED8B*/ GENx___x___x___ ,
 /*ED8C*/ GENx___x___x___ ,
 /*ED8D*/ GENx___x___x___ ,
 /*ED8E*/ GENx___x___x___ ,
 /*ED8F*/ GENx___x___x___ ,
 /*ED90*/ GENx___x___x___ ,
 /*ED91*/ GENx___x___x___ ,
 /*ED92*/ GENx___x___x___ ,
 /*ED93*/ GENx___x___x___ ,
 /*ED94*/ GENx___x___x___ ,
 /*ED95*/ GENx___x___x___ ,
 /*ED96*/ GENx___x___x___ ,
 /*ED97*/ GENx___x___x___ ,
 /*ED98*/ GENx___x___x___ ,
 /*ED99*/ GENx___x___x___ ,
 /*ED9A*/ GENx___x___x___ ,
 /*ED9B*/ GENx___x___x___ ,
 /*ED9C*/ GENx___x___x___ ,
 /*ED9D*/ GENx___x___x___ ,
 /*ED9E*/ GENx___x___x___ ,
 /*ED9F*/ GENx___x___x___ ,
 /*EDA0*/ GENx___x___x___ ,
 /*EDA1*/ GENx___x___x___ ,
 /*EDA2*/ GENx___x___x___ ,
 /*EDA3*/ GENx___x___x___ ,
 /*EDA4*/ GENx___x___x___ ,
 /*EDA5*/ GENx___x___x___ ,
 /*EDA6*/ GENx___x___x___ ,
 /*EDA7*/ GENx___x___x___ ,
 /*EDA8*/ GENx___x___x900 ( "CZDT"      , RSL_b, ASMFMT_RSL_RM   , convert_dfp_long_to_zoned                           ),
 /*EDA9*/ GENx___x___x900 ( "CZXT"      , RSL_b, ASMFMT_RSL_RM   , convert_dfp_ext_to_zoned                            ),
 /*EDAA*/ GENx___x___x900 ( "CDZT"      , RSL_b, ASMFMT_RSL_RM   , convert_zoned_to_dfp_long                           ),
 /*EDAB*/ GENx___x___x900 ( "CXZT"      , RSL_b, ASMFMT_RSL_RM   , convert_zoned_to_dfp_ext                            ),
 /*EDAC*/ GENx___x___x900 ( "CPDT"      , RSL_b, ASMFMT_RSL_RM   , convert_dfp_long_to_packed                          ),
 /*EDAD*/ GENx___x___x900 ( "CPXT"      , RSL_b, ASMFMT_RSL_RM   , convert_dfp_ext_to_packed                           ),
 /*EDAE*/ GENx___x___x900 ( "CDPT"      , RSL_b, ASMFMT_RSL_RM   , convert_packed_to_dfp_long                          ),
 /*EDAF*/ GENx___x___x900 ( "CXPT"      , RSL_b, ASMFMT_RSL_RM   , convert_packed_to_dfp_ext                           ),
 /*EDB0*/ GENx___x___x___ ,
 /*EDB1*/ GENx___x___x___ ,
 /*EDB2*/ GENx___x___x___ ,
 /*EDB3*/ GENx___x___x___ ,
 /*EDB4*/ GENx___x___x___ ,
 /*EDB5*/ GENx___x___x___ ,
 /*EDB6*/ GENx___x___x___ ,
 /*EDB7*/ GENx___x___x___ ,
 /*EDB8*/ GENx___x___x___ ,
 /*EDB3*/ GENx___x___x___ ,
 /*EDBA*/ GENx___x___x___ ,
 /*EDBB*/ GENx___x___x___ ,
 /*EDBC*/ GENx___x___x___ ,
 /*EDBD*/ GENx___x___x___ ,
 /*EDBE*/ GENx___x___x___ ,
 /*EDBF*/ GENx___x___x___ ,
 /*EDC0*/ GENx___x___x___ ,
 /*EDC1*/ GENx___x___x___ ,
 /*EDC2*/ GENx___x___x___ ,
 /*EDC3*/ GENx___x___x___ ,
 /*EDC4*/ GENx___x___x___ ,
 /*EDC5*/ GENx___x___x___ ,
 /*EDC6*/ GENx___x___x___ ,
 /*EDC7*/ GENx___x___x___ ,
 /*EDC8*/ GENx___x___x___ ,
 /*EDC9*/ GENx___x___x___ ,
 /*EDCA*/ GENx___x___x___ ,
 /*EDCB*/ GENx___x___x___ ,
 /*EDCC*/ GENx___x___x___ ,
 /*EDCD*/ GENx___x___x___ ,
 /*EDCE*/ GENx___x___x___ ,
 /*EDCF*/ GENx___x___x___ ,
 /*EDD0*/ GENx___x___x___ ,
 /*EDD1*/ GENx___x___x___ ,
 /*EDD2*/ GENx___x___x___ ,
 /*EDD3*/ GENx___x___x___ ,
 /*EDD4*/ GENx___x___x___ ,
 /*EDD5*/ GENx___x___x___ ,
 /*EDD6*/ GENx___x___x___ ,
 /*EDD7*/ GENx___x___x___ ,
 /*EDD8*/ GENx___x___x___ ,
 /*EDD9*/ GENx___x___x___ ,
 /*EDDA*/ GENx___x___x___ ,
 /*EDDB*/ GENx___x___x___ ,
 /*EDDC*/ GENx___x___x___ ,
 /*EDDD*/ GENx___x___x___ ,
 /*EDDE*/ GENx___x___x___ ,
 /*EDDF*/ GENx___x___x___ ,
 /*EDE0*/ GENx___x___x___ ,
 /*EDE1*/ GENx___x___x___ ,
 /*EDE2*/ GENx___x___x___ ,
 /*EDE3*/ GENx___x___x___ ,
 /*EDE4*/ GENx___x___x___ ,
 /*EDE5*/ GENx___x___x___ ,
 /*EDE6*/ GENx___x___x___ ,
 /*EDE7*/ GENx___x___x___ ,
 /*EDE8*/ GENx___x___x___ ,
 /*EDE9*/ GENx___x___x___ ,
 /*EDEA*/ GENx___x___x___ ,
 /*EDEB*/ GENx___x___x___ ,
 /*EDEC*/ GENx___x___x___ ,
 /*EDED*/ GENx___x___x___ ,
 /*EDEE*/ GENx___x___x___ ,
 /*EDEF*/ GENx___x___x___ ,
 /*EDF0*/ GENx___x___x___ ,
 /*EDF1*/ GENx___x___x___ ,
 /*EDF2*/ GENx___x___x___ ,
 /*EDF3*/ GENx___x___x___ ,
 /*EDF4*/ GENx___x___x___ ,
 /*EDF5*/ GENx___x___x___ ,
 /*EDF6*/ GENx___x___x___ ,
 /*EDF7*/ GENx___x___x___ ,
 /*EDF8*/ GENx___x___x___ ,
 /*EDF9*/ GENx___x___x___ ,
 /*EDFA*/ GENx___x___x___ ,
 /*EDFB*/ GENx___x___x___ ,
 /*EDFC*/ GENx___x___x___ ,
 /*EDFD*/ GENx___x___x___ ,
 /*EDFE*/ GENx___x___x___ ,
 /*EDFF*/ GENx___x___x___
};

static INSTR_FUNC gen_v_opcode_a4xx[256][NUM_INSTR_TAB_PTRS] =
{
 /*A400*/ GENx___x___x___ , /* VAE */
 /*A401*/ GENx___x___x___ , /* VSE */
 /*A402*/ GENx___x___x___ , /* VME */
 /*A403*/ GENx___x___x___ , /* VDE */
 /*A404*/ GENx___x___x___ , /* VMAE */
 /*A405*/ GENx___x___x___ , /* VMSE */
 /*A406*/ GENx___x___x___ , /* VMCE */
 /*A407*/ GENx___x___x___ , /* VACE */
 /*A408*/ GENx___x___x___ , /* VCE */
 /*A409*/ GENx___x___x___ , /* VL, VLE */
 /*A40A*/ GENx___x___x___ , /* VLM, VLME */
 /*A40B*/ GENx___x___x___ , /* VLY, VLYE */
 /*A40C*/ GENx___x___x___ ,
 /*A40D*/ GENx___x___x___ , /* VST, VSTE */
 /*A40E*/ GENx___x___x___ , /* VSTM, VSTME */
 /*A40F*/ GENx___x___x___ , /* VSTK, VSTKE */
 /*A410*/ GENx___x___x___ , /* VAD */
 /*A411*/ GENx___x___x___ , /* VSD */
 /*A412*/ GENx___x___x___ , /* VMD */
 /*A413*/ GENx___x___x___ , /* VDD */
 /*A414*/ GENx___x___x___ , /* VMAD */
 /*A415*/ GENx___x___x___ , /* VMSD */
 /*A416*/ GENx___x___x___ , /* VMCD */
 /*A417*/ GENx___x___x___ , /* VACD */
 /*A418*/ GENx___x___x___ , /* VCD */
 /*A419*/ GENx___x___x___ , /* VLD */
 /*A41A*/ GENx___x___x___ , /* VLMD */
 /*A41B*/ GENx___x___x___ , /* VLYD */
 /*A41C*/ GENx___x___x___ ,
 /*A41D*/ GENx___x___x___ , /* VSTD */
 /*A41E*/ GENx___x___x___ , /* VSTMD */
 /*A41F*/ GENx___x___x___ , /* VSTKD */
 /*A420*/ GENx___x___x___ , /* VA */
 /*A421*/ GENx___x___x___ , /* VS */
 /*A422*/ GENx___x___x___ , /* VM */
 /*A423*/ GENx___x___x___ ,
 /*A424*/ GENx___x___x___ , /* VN */
 /*A425*/ GENx___x___x___ , /* VO */
 /*A426*/ GENx___x___x___ , /* VX */
 /*A427*/ GENx___x___x___ ,
 /*A428*/ GENx___x___x___ , /* VC */
 /*A429*/ GENx___x___x___ , /* VLH */
 /*A42A*/ GENx___x___x___ , /* VLINT */
 /*A42B*/ GENx___x___x___ ,
 /*A42C*/ GENx___x___x___ ,
 /*A42D*/ GENx___x___x___ , /* VSTH */
 /*A42E*/ GENx___x___x___ ,
 /*A42F*/ GENx___x___x___ ,
 /*A430*/ GENx___x___x___ ,
 /*A431*/ GENx___x___x___ ,
 /*A432*/ GENx___x___x___ ,
 /*A433*/ GENx___x___x___ ,
 /*A434*/ GENx___x___x___ ,
 /*A435*/ GENx___x___x___ ,
 /*A436*/ GENx___x___x___ ,
 /*A437*/ GENx___x___x___ ,
 /*A438*/ GENx___x___x___ ,
 /*A439*/ GENx___x___x___ ,
 /*A43A*/ GENx___x___x___ ,
 /*A43B*/ GENx___x___x___ ,
 /*A43C*/ GENx___x___x___ ,
 /*A43D*/ GENx___x___x___ ,
 /*A43E*/ GENx___x___x___ ,
 /*A43F*/ GENx___x___x___ ,
 /*A440*/ GENx___x___x___ ,
 /*A441*/ GENx___x___x___ ,
 /*A442*/ GENx___x___x___ ,
 /*A443*/ GENx___x___x___ ,
 /*A444*/ GENx___x___x___ ,
 /*A445*/ GENx___x___x___ ,
 /*A446*/ GENx___x___x___ ,
 /*A447*/ GENx___x___x___ ,
 /*A448*/ GENx___x___x___ ,
 /*A449*/ GENx___x___x___ ,
 /*A44A*/ GENx___x___x___ ,
 /*A44B*/ GENx___x___x___ ,
 /*A44C*/ GENx___x___x___ ,
 /*A44D*/ GENx___x___x___ ,
 /*A44E*/ GENx___x___x___ ,
 /*A44F*/ GENx___x___x___ ,
 /*A450*/ GENx___x___x___ ,
 /*A451*/ GENx___x___x___ ,
 /*A452*/ GENx___x___x___ ,
 /*A453*/ GENx___x___x___ ,
 /*A454*/ GENx___x___x___ ,
 /*A455*/ GENx___x___x___ ,
 /*A456*/ GENx___x___x___ ,
 /*A457*/ GENx___x___x___ ,
 /*A458*/ GENx___x___x___ ,
 /*A459*/ GENx___x___x___ ,
 /*A45A*/ GENx___x___x___ ,
 /*A45B*/ GENx___x___x___ ,
 /*A45C*/ GENx___x___x___ ,
 /*A45D*/ GENx___x___x___ ,
 /*A45E*/ GENx___x___x___ ,
 /*A45F*/ GENx___x___x___ ,
 /*A460*/ GENx___x___x___ ,
 /*A461*/ GENx___x___x___ ,
 /*A462*/ GENx___x___x___ ,
 /*A463*/ GENx___x___x___ ,
 /*A464*/ GENx___x___x___ ,
 /*A465*/ GENx___x___x___ ,
 /*A466*/ GENx___x___x___ ,
 /*A467*/ GENx___x___x___ ,
 /*A468*/ GENx___x___x___ ,
 /*A469*/ GENx___x___x___ ,
 /*A46A*/ GENx___x___x___ ,
 /*A46B*/ GENx___x___x___ ,
 /*A46C*/ GENx___x___x___ ,
 /*A46D*/ GENx___x___x___ ,
 /*A46E*/ GENx___x___x___ ,
 /*A46F*/ GENx___x___x___ ,
 /*A470*/ GENx___x___x___ ,
 /*A471*/ GENx___x___x___ ,
 /*A472*/ GENx___x___x___ ,
 /*A473*/ GENx___x___x___ ,
 /*A474*/ GENx___x___x___ ,
 /*A475*/ GENx___x___x___ ,
 /*A476*/ GENx___x___x___ ,
 /*A477*/ GENx___x___x___ ,
 /*A478*/ GENx___x___x___ ,
 /*A479*/ GENx___x___x___ ,
 /*A47A*/ GENx___x___x___ ,
 /*A47B*/ GENx___x___x___ ,
 /*A47C*/ GENx___x___x___ ,
 /*A47D*/ GENx___x___x___ ,
 /*A47E*/ GENx___x___x___ ,
 /*A47F*/ GENx___x___x___ ,
 /*A480*/ GENx___x___x___ , /* VAES */
 /*A481*/ GENx___x___x___ , /* VSES */
 /*A482*/ GENx___x___x___ , /* VMES */
 /*A483*/ GENx___x___x___ , /* VDES */
 /*A484*/ GENx___x___x___ , /* VMAES */
 /*A485*/ GENx___x___x___ , /* VMSES */
 /*A486*/ GENx___x___x___ ,
 /*A487*/ GENx___x___x___ ,
 /*A488*/ GENx___x___x___ , /* VCES */
 /*A489*/ GENx___x___x___ ,
 /*A48A*/ GENx___x___x___ ,
 /*A48B*/ GENx___x___x___ ,
 /*A48C*/ GENx___x___x___ ,
 /*A48D*/ GENx___x___x___ ,
 /*A48E*/ GENx___x___x___ ,
 /*A48F*/ GENx___x___x___ ,
 /*A490*/ GENx___x___x___ , /* VADS */
 /*A491*/ GENx___x___x___ , /* VSDS */
 /*A492*/ GENx___x___x___ , /* VMDS */
 /*A493*/ GENx___x___x___ , /* VDDS */
 /*A494*/ GENx___x___x___ , /* VMADS */
 /*A495*/ GENx___x___x___ , /* VMSDS */
 /*A496*/ GENx___x___x___ ,
 /*A497*/ GENx___x___x___ ,
 /*A498*/ GENx___x___x___ , /* VCDS */
 /*A499*/ GENx___x___x___ ,
 /*A49A*/ GENx___x___x___ ,
 /*A49B*/ GENx___x___x___ ,
 /*A49C*/ GENx___x___x___ ,
 /*A49D*/ GENx___x___x___ ,
 /*A49E*/ GENx___x___x___ ,
 /*A49F*/ GENx___x___x___ ,
 /*A4A0*/ GENx___x___x___ , /* VAS */
 /*A4A1*/ GENx___x___x___ , /* VSS */
 /*A4A2*/ GENx___x___x___ , /* VMS */
 /*A4A3*/ GENx___x___x___ ,
 /*A4A4*/ GENx___x___x___ , /* VNS */
 /*A4A5*/ GENx___x___x___ , /* VOS */
 /*A4A6*/ GENx___x___x___ , /* VXS */
 /*A4A7*/ GENx___x___x___ ,
 /*A4A8*/ GENx___x___x___ , /* VCS */
 /*A4A9*/ GENx___x___x___ ,
 /*A4AA*/ GENx___x___x___ ,
 /*A4AB*/ GENx___x___x___ ,
 /*A4AC*/ GENx___x___x___ ,
 /*A4AD*/ GENx___x___x___ ,
 /*A4AE*/ GENx___x___x___ ,
 /*A4AF*/ GENx___x___x___ ,
 /*A4B0*/ GENx___x___x___ ,
 /*A4B1*/ GENx___x___x___ ,
 /*A4B2*/ GENx___x___x___ ,
 /*A4B3*/ GENx___x___x___ ,
 /*A4B4*/ GENx___x___x___ ,
 /*A4B5*/ GENx___x___x___ ,
 /*A4B6*/ GENx___x___x___ ,
 /*A4B7*/ GENx___x___x___ ,
 /*A4B8*/ GENx___x___x___ ,
 /*A4B9*/ GENx___x___x___ ,
 /*A4BA*/ GENx___x___x___ ,
 /*A4BB*/ GENx___x___x___ ,
 /*A4BC*/ GENx___x___x___ ,
 /*A4BD*/ GENx___x___x___ ,
 /*A4BE*/ GENx___x___x___ ,
 /*A4BF*/ GENx___x___x___ ,
 /*A4C0*/ GENx___x___x___ ,
 /*A4C1*/ GENx___x___x___ ,
 /*A4C2*/ GENx___x___x___ ,
 /*A4C3*/ GENx___x___x___ ,
 /*A4C4*/ GENx___x___x___ ,
 /*A4C5*/ GENx___x___x___ ,
 /*A4C6*/ GENx___x___x___ ,
 /*A4C7*/ GENx___x___x___ ,
 /*A4C8*/ GENx___x___x___ ,
 /*A4C9*/ GENx___x___x___ ,
 /*A4CA*/ GENx___x___x___ ,
 /*A4CB*/ GENx___x___x___ ,
 /*A4CC*/ GENx___x___x___ ,
 /*A4CD*/ GENx___x___x___ ,
 /*A4CE*/ GENx___x___x___ ,
 /*A4CF*/ GENx___x___x___ ,
 /*A4D0*/ GENx___x___x___ ,
 /*A4D1*/ GENx___x___x___ ,
 /*A4D2*/ GENx___x___x___ ,
 /*A4D3*/ GENx___x___x___ ,
 /*A4D4*/ GENx___x___x___ ,
 /*A4D5*/ GENx___x___x___ ,
 /*A4D6*/ GENx___x___x___ ,
 /*A4D7*/ GENx___x___x___ ,
 /*A4D8*/ GENx___x___x___ ,
 /*A4D9*/ GENx___x___x___ ,
 /*A4DA*/ GENx___x___x___ ,
 /*A4DB*/ GENx___x___x___ ,
 /*A4DC*/ GENx___x___x___ ,
 /*A4DD*/ GENx___x___x___ ,
 /*A4DE*/ GENx___x___x___ ,
 /*A4DF*/ GENx___x___x___ ,
 /*A4E0*/ GENx___x___x___ ,
 /*A4E1*/ GENx___x___x___ ,
 /*A4E2*/ GENx___x___x___ ,
 /*A4E3*/ GENx___x___x___ ,
 /*A4E4*/ GENx___x___x___ ,
 /*A4E5*/ GENx___x___x___ ,
 /*A4E6*/ GENx___x___x___ ,
 /*A4E7*/ GENx___x___x___ ,
 /*A4E8*/ GENx___x___x___ ,
 /*A4E9*/ GENx___x___x___ ,
 /*A4EA*/ GENx___x___x___ ,
 /*A4EB*/ GENx___x___x___ ,
 /*A4EC*/ GENx___x___x___ ,
 /*A4ED*/ GENx___x___x___ ,
 /*A4EE*/ GENx___x___x___ ,
 /*A4EF*/ GENx___x___x___ ,
 /*A4F0*/ GENx___x___x___ ,
 /*A4F1*/ GENx___x___x___ ,
 /*A4F2*/ GENx___x___x___ ,
 /*A4F3*/ GENx___x___x___ ,
 /*A4F4*/ GENx___x___x___ ,
 /*A4F5*/ GENx___x___x___ ,
 /*A4F6*/ GENx___x___x___ ,
 /*A4F7*/ GENx___x___x___ ,
 /*A4F8*/ GENx___x___x___ ,
 /*A4F9*/ GENx___x___x___ ,
 /*A4FA*/ GENx___x___x___ ,
 /*A4FB*/ GENx___x___x___ ,
 /*A4FC*/ GENx___x___x___ ,
 /*A4FD*/ GENx___x___x___ ,
 /*A4FE*/ GENx___x___x___ ,
 /*A4FF*/ GENx___x___x___
};

#if defined( _FEATURE_S370_S390_VECTOR_FACILITY )
static INSTR_FUNC gen_v_opcode_a5xx[256][NUM_INSTR_TAB_PTRS] =
{
 /*A500*/ GENx___x___x___ , /* VAER */
 /*A501*/ GENx___x___x___ , /* VSER */
 /*A502*/ GENx___x___x___ , /* VMER */
 /*A503*/ GENx___x___x___ , /* VDER */
 /*A504*/ GENx___x___x___ ,
 /*A505*/ GENx___x___x___ ,
 /*A506*/ GENx___x___x___ , /* VMCER */
 /*A507*/ GENx___x___x___ , /* VACER */
 /*A508*/ GENx___x___x___ , /* VCER */
 /*A509*/ GENx___x___x___ , /* VLER, VLR */
 /*A50A*/ GENx___x___x___ , /* VLMER, VLMR */
 /*A50B*/ GENx___x___x___ , /* VLZER, VLZR */
 /*A50C*/ GENx___x___x___ ,
 /*A50D*/ GENx___x___x___ ,
 /*A50E*/ GENx___x___x___ ,
 /*A50F*/ GENx___x___x___ ,
 /*A510*/ GENx___x___x___ , /* VADR */
 /*A511*/ GENx___x___x___ , /* VSDR */
 /*A512*/ GENx___x___x___ , /* VMDR */
 /*A513*/ GENx___x___x___ , /* VDDR */
 /*A514*/ GENx___x___x___ ,
 /*A515*/ GENx___x___x___ ,
 /*A516*/ GENx___x___x___ , /* VMCDR */
 /*A517*/ GENx___x___x___ , /* VACDR */
 /*A518*/ GENx___x___x___ , /* VCDR */
 /*A519*/ GENx___x___x___ , /* VLDR */
 /*A51A*/ GENx___x___x___ , /* VLMDR */
 /*A51B*/ GENx___x___x___ , /* VLZDR */
 /*A51C*/ GENx___x___x___ ,
 /*A51D*/ GENx___x___x___ ,
 /*A51E*/ GENx___x___x___ ,
 /*A51F*/ GENx___x___x___ ,
 /*A520*/ GENx___x___x___ , /* VAR */
 /*A521*/ GENx___x___x___ , /* VSR */
 /*A522*/ GENx___x___x___ , /* VMR */
 /*A523*/ GENx___x___x___ ,
 /*A524*/ GENx___x___x___ , /* VNR */
 /*A525*/ GENx___x___x___ , /* VOR */
 /*A526*/ GENx___x___x___ , /* VXR */
 /*A527*/ GENx___x___x___ ,
 /*A528*/ GENx___x___x___ , /* VCR */
 /*A529*/ GENx___x___x___ ,
 /*A52A*/ GENx___x___x___ ,
 /*A52B*/ GENx___x___x___ ,
 /*A52C*/ GENx___x___x___ ,
 /*A52D*/ GENx___x___x___ ,
 /*A52E*/ GENx___x___x___ ,
 /*A52F*/ GENx___x___x___ ,
 /*A530*/ GENx___x___x___ ,
 /*A531*/ GENx___x___x___ ,
 /*A532*/ GENx___x___x___ ,
 /*A533*/ GENx___x___x___ ,
 /*A534*/ GENx___x___x___ ,
 /*A535*/ GENx___x___x___ ,
 /*A536*/ GENx___x___x___ ,
 /*A537*/ GENx___x___x___ ,
 /*A538*/ GENx___x___x___ ,
 /*A539*/ GENx___x___x___ ,
 /*A53A*/ GENx___x___x___ ,
 /*A53B*/ GENx___x___x___ ,
 /*A53C*/ GENx___x___x___ ,
 /*A53D*/ GENx___x___x___ ,
 /*A53E*/ GENx___x___x___ ,
 /*A53F*/ GENx___x___x___ ,
 /*A540*/ GENx___x___x___ , /* VLPER */
 /*A541*/ GENx___x___x___ , /* VLNER */
 /*A542*/ GENx___x___x___ , /* VLCER */
 /*A543*/ GENx___x___x___ ,
 /*A544*/ GENx___x___x___ ,
 /*A545*/ GENx___x___x___ ,
 /*A546*/ GENx___x___x___ ,
 /*A547*/ GENx___x___x___ ,
 /*A548*/ GENx___x___x___ ,
 /*A549*/ GENx___x___x___ ,
 /*A54A*/ GENx___x___x___ ,
 /*A54B*/ GENx___x___x___ ,
 /*A54C*/ GENx___x___x___ ,
 /*A54D*/ GENx___x___x___ ,
 /*A54E*/ GENx___x___x___ ,
 /*A54F*/ GENx___x___x___ ,
 /*A550*/ GENx___x___x___ , /* VLPDR */
 /*A551*/ GENx___x___x___ , /* VLNDR */
 /*A552*/ GENx___x___x___ , /* VLCDR */
 /*A553*/ GENx___x___x___ ,
 /*A554*/ GENx___x___x___ ,
 /*A555*/ GENx___x___x___ ,
 /*A556*/ GENx___x___x___ ,
 /*A557*/ GENx___x___x___ ,
 /*A558*/ GENx___x___x___ ,
 /*A559*/ GENx___x___x___ ,
 /*A55A*/ GENx___x___x___ ,
 /*A55B*/ GENx___x___x___ ,
 /*A55C*/ GENx___x___x___ ,
 /*A55D*/ GENx___x___x___ ,
 /*A55E*/ GENx___x___x___ ,
 /*A55F*/ GENx___x___x___ ,
 /*A560*/ GENx___x___x___ , /* VLPR */
 /*A561*/ GENx___x___x___ , /* VLNR */
 /*A562*/ GENx___x___x___ , /* VLCR */
 /*A563*/ GENx___x___x___ ,
 /*A564*/ GENx___x___x___ ,
 /*A565*/ GENx___x___x___ ,
 /*A566*/ GENx___x___x___ ,
 /*A567*/ GENx___x___x___ ,
 /*A568*/ GENx___x___x___ ,
 /*A569*/ GENx___x___x___ ,
 /*A56A*/ GENx___x___x___ ,
 /*A56B*/ GENx___x___x___ ,
 /*A56C*/ GENx___x___x___ ,
 /*A56D*/ GENx___x___x___ ,
 /*A56E*/ GENx___x___x___ ,
 /*A56F*/ GENx___x___x___ ,
 /*A570*/ GENx___x___x___ ,
 /*A571*/ GENx___x___x___ ,
 /*A572*/ GENx___x___x___ ,
 /*A573*/ GENx___x___x___ ,
 /*A574*/ GENx___x___x___ ,
 /*A575*/ GENx___x___x___ ,
 /*A576*/ GENx___x___x___ ,
 /*A577*/ GENx___x___x___ ,
 /*A578*/ GENx___x___x___ ,
 /*A579*/ GENx___x___x___ ,
 /*A57A*/ GENx___x___x___ ,
 /*A57B*/ GENx___x___x___ ,
 /*A57C*/ GENx___x___x___ ,
 /*A57D*/ GENx___x___x___ ,
 /*A57E*/ GENx___x___x___ ,
 /*A57F*/ GENx___x___x___ ,
 /*A580*/ GENx___x___x___ , /* VAEQ */
 /*A581*/ GENx___x___x___ , /* VSEQ */
 /*A582*/ GENx___x___x___ , /* VMEQ */
 /*A583*/ GENx___x___x___ , /* VDEQ */
 /*A584*/ GENx___x___x___ , /* VMAEQ */
 /*A585*/ GENx___x___x___ , /* VMSEQ */
 /*A586*/ GENx___x___x___ ,
 /*A587*/ GENx___x___x___ ,
 /*A588*/ GENx___x___x___ , /* VCEQ */
 /*A589*/ GENx___x___x___ , /* VLEQ */
 /*A58A*/ GENx___x___x___ , /* VLMEQ */
 /*A58B*/ GENx___x___x___ ,
 /*A58C*/ GENx___x___x___ ,
 /*A58D*/ GENx___x___x___ ,
 /*A58E*/ GENx___x___x___ ,
 /*A58F*/ GENx___x___x___ ,
 /*A590*/ GENx___x___x___ , /* VADQ */
 /*A591*/ GENx___x___x___ , /* VSDQ */
 /*A592*/ GENx___x___x___ , /* VMDQ */
 /*A593*/ GENx___x___x___ , /* VDDQ */
 /*A594*/ GENx___x___x___ , /* VMADQ */
 /*A595*/ GENx___x___x___ , /* VMSDQ */
 /*A596*/ GENx___x___x___ ,
 /*A597*/ GENx___x___x___ ,
 /*A598*/ GENx___x___x___ , /* VCDQ */
 /*A599*/ GENx___x___x___ , /* VLDQ */
 /*A59A*/ GENx___x___x___ , /* VLMDQ */
 /*A59B*/ GENx___x___x___ ,
 /*A59C*/ GENx___x___x___ ,
 /*A59D*/ GENx___x___x___ ,
 /*A59E*/ GENx___x___x___ ,
 /*A59F*/ GENx___x___x___ ,
 /*A5A0*/ GENx___x___x___ , /* VAQ */
 /*A5A1*/ GENx___x___x___ , /* VSQ */
 /*A5A2*/ GENx___x___x___ , /* VMQ */
 /*A5A3*/ GENx___x___x___ ,
 /*A5A4*/ GENx___x___x___ , /* VNQ */
 /*A5A5*/ GENx___x___x___ , /* VOQ */
 /*A5A6*/ GENx___x___x___ , /* VXQ */
 /*A5A7*/ GENx___x___x___ ,
 /*A5A8*/ GENx___x___x___ , /* VCQ */
 /*A5A9*/ GENx___x___x___ , /* VLQ */
 /*A5AA*/ GENx___x___x___ , /* VLMQ */
 /*A5AB*/ GENx___x___x___ ,
 /*A5AC*/ GENx___x___x___ ,
 /*A5AD*/ GENx___x___x___ ,
 /*A5AE*/ GENx___x___x___ ,
 /*A5AF*/ GENx___x___x___ ,
 /*A5B0*/ GENx___x___x___ ,
 /*A5B1*/ GENx___x___x___ ,
 /*A5B2*/ GENx___x___x___ ,
 /*A5B3*/ GENx___x___x___ ,
 /*A5B4*/ GENx___x___x___ ,
 /*A5B5*/ GENx___x___x___ ,
 /*A5B6*/ GENx___x___x___ ,
 /*A5B7*/ GENx___x___x___ ,
 /*A5B8*/ GENx___x___x___ ,
 /*A5B9*/ GENx___x___x___ ,
 /*A5BA*/ GENx___x___x___ ,
 /*A5BB*/ GENx___x___x___ ,
 /*A5BC*/ GENx___x___x___ ,
 /*A5BD*/ GENx___x___x___ ,
 /*A5BE*/ GENx___x___x___ ,
 /*A5BF*/ GENx___x___x___ ,
 /*A5C0*/ GENx___x___x___ ,
 /*A5C1*/ GENx___x___x___ ,
 /*A5C2*/ GENx___x___x___ ,
 /*A5C3*/ GENx___x___x___ ,
 /*A5C4*/ GENx___x___x___ ,
 /*A5C5*/ GENx___x___x___ ,
 /*A5C6*/ GENx___x___x___ ,
 /*A5C7*/ GENx___x___x___ ,
 /*A5C8*/ GENx___x___x___ ,
 /*A5C9*/ GENx___x___x___ ,
 /*A5CA*/ GENx___x___x___ ,
 /*A5CB*/ GENx___x___x___ ,
 /*A5CC*/ GENx___x___x___ ,
 /*A5CD*/ GENx___x___x___ ,
 /*A5CE*/ GENx___x___x___ ,
 /*A5CF*/ GENx___x___x___ ,
 /*A5D0*/ GENx___x___x___ ,
 /*A5D1*/ GENx___x___x___ ,
 /*A5D2*/ GENx___x___x___ ,
 /*A5D3*/ GENx___x___x___ ,
 /*A5D4*/ GENx___x___x___ ,
 /*A5D5*/ GENx___x___x___ ,
 /*A5D6*/ GENx___x___x___ ,
 /*A5D7*/ GENx___x___x___ ,
 /*A5D8*/ GENx___x___x___ ,
 /*A5D9*/ GENx___x___x___ ,
 /*A5DA*/ GENx___x___x___ ,
 /*A5DB*/ GENx___x___x___ ,
 /*A5DC*/ GENx___x___x___ ,
 /*A5DD*/ GENx___x___x___ ,
 /*A5DE*/ GENx___x___x___ ,
 /*A5DF*/ GENx___x___x___ ,
 /*A5E0*/ GENx___x___x___ ,
 /*A5E1*/ GENx___x___x___ ,
 /*A5E2*/ GENx___x___x___ ,
 /*A5E3*/ GENx___x___x___ ,
 /*A5E4*/ GENx___x___x___ ,
 /*A5E5*/ GENx___x___x___ ,
 /*A5E6*/ GENx___x___x___ ,
 /*A5E7*/ GENx___x___x___ ,
 /*A5E8*/ GENx___x___x___ ,
 /*A5E9*/ GENx___x___x___ ,
 /*A5EA*/ GENx___x___x___ ,
 /*A5EB*/ GENx___x___x___ ,
 /*A5EC*/ GENx___x___x___ ,
 /*A5ED*/ GENx___x___x___ ,
 /*A5EE*/ GENx___x___x___ ,
 /*A5EF*/ GENx___x___x___ ,
 /*A5F0*/ GENx___x___x___ ,
 /*A5F1*/ GENx___x___x___ ,
 /*A5F2*/ GENx___x___x___ ,
 /*A5F3*/ GENx___x___x___ ,
 /*A5F4*/ GENx___x___x___ ,
 /*A5F5*/ GENx___x___x___ ,
 /*A5F6*/ GENx___x___x___ ,
 /*A5F7*/ GENx___x___x___ ,
 /*A5F8*/ GENx___x___x___ ,
 /*A5F9*/ GENx___x___x___ ,
 /*A5FA*/ GENx___x___x___ ,
 /*A5FB*/ GENx___x___x___ ,
 /*A5FC*/ GENx___x___x___ ,
 /*A5FD*/ GENx___x___x___ ,
 /*A5FE*/ GENx___x___x___ ,
 /*A5FF*/ GENx___x___x___
};
#endif /* defined( _FEATURE_S370_S390_VECTOR_FACILITY ) */

static INSTR_FUNC gen_v_opcode_a6xx[256][NUM_INSTR_TAB_PTRS] =
{
 /*A600*/ GENx___x___x___ , /* VMXSE */
 /*A601*/ GENx___x___x___ , /* VMNSE */
 /*A602*/ GENx___x___x___ , /* VMXAE */
 /*A603*/ GENx___x___x___ ,
 /*A604*/ GENx___x___x___ ,
 /*A605*/ GENx___x___x___ ,
 /*A606*/ GENx___x___x___ ,
 /*A607*/ GENx___x___x___ ,
 /*A608*/ GENx___x___x___ , /* VLELE */
 /*A609*/ GENx___x___x___ , /* VXELE */
 /*A60A*/ GENx___x___x___ ,
 /*A60B*/ GENx___x___x___ ,
 /*A60C*/ GENx___x___x___ ,
 /*A60D*/ GENx___x___x___ ,
 /*A60E*/ GENx___x___x___ ,
 /*A60F*/ GENx___x___x___ ,
 /*A610*/ GENx___x___x___ , /* VMXSD */
 /*A611*/ GENx___x___x___ , /* VMNSD */
 /*A612*/ GENx___x___x___ , /* VMXAD */
 /*A613*/ GENx___x___x___ ,
 /*A614*/ GENx___x___x___ ,
 /*A615*/ GENx___x___x___ ,
 /*A616*/ GENx___x___x___ ,
 /*A617*/ GENx___x___x___ ,
 /*A618*/ GENx___x___x___ , /* VLELD */
 /*A619*/ GENx___x___x___ , /* VXELD */
 /*A61A*/ GENx___x___x___ , /* VSPSD */
 /*A61B*/ GENx___x___x___ , /* VZPSD */
 /*A61C*/ GENx___x___x___ ,
 /*A61D*/ GENx___x___x___ ,
 /*A61E*/ GENx___x___x___ ,
 /*A61F*/ GENx___x___x___ ,
 /*A620*/ GENx___x___x___ ,
 /*A621*/ GENx___x___x___ ,
 /*A622*/ GENx___x___x___ ,
 /*A623*/ GENx___x___x___ ,
 /*A624*/ GENx___x___x___ ,
 /*A625*/ GENx___x___x___ ,
 /*A626*/ GENx___x___x___ ,
 /*A627*/ GENx___x___x___ ,
 /*A628*/ GENx___x___x___ , /* VLEL */
 /*A629*/ GENx___x___x___ , /* VXEL */
 /*A62A*/ GENx___x___x___ ,
 /*A62B*/ GENx___x___x___ ,
 /*A62C*/ GENx___x___x___ ,
 /*A62D*/ GENx___x___x___ ,
 /*A62E*/ GENx___x___x___ ,
 /*A62F*/ GENx___x___x___ ,
 /*A630*/ GENx___x___x___ ,
 /*A631*/ GENx___x___x___ ,
 /*A632*/ GENx___x___x___ ,
 /*A633*/ GENx___x___x___ ,
 /*A634*/ GENx___x___x___ ,
 /*A635*/ GENx___x___x___ ,
 /*A636*/ GENx___x___x___ ,
 /*A637*/ GENx___x___x___ ,
 /*A638*/ GENx___x___x___ ,
 /*A639*/ GENx___x___x___ ,
 /*A63A*/ GENx___x___x___ ,
 /*A63B*/ GENx___x___x___ ,
 /*A63C*/ GENx___x___x___ ,
 /*A63D*/ GENx___x___x___ ,
 /*A63E*/ GENx___x___x___ ,
 /*A63F*/ GENx___x___x___ ,
 /*A640*/ GENx370x390x___ ( "VTVM"      , RRE  , ASMFMT_RRE      , v_test_vmr                                          ),
 /*A641*/ GENx370x390x___ ( "VCVM"      , RRE  , ASMFMT_RRE      , v_complement_vmr                                    ),
 /*A642*/ GENx370x390x___ ( "VCZVM"     , RRE  , ASMFMT_RRE      , v_count_left_zeros_in_vmr                           ),
 /*A643*/ GENx370x390x___ ( "VCOVM"     , RRE  , ASMFMT_RRE      , v_count_ones_in_vmr                                 ),
 /*A644*/ GENx370x390x___ ( "VXVC"      , RRE  , ASMFMT_RRE      , v_extract_vct                                       ),
 /*A645*/ GENx___x___x___ , /* VLVCU */
 /*A646*/ GENx370x390x___ ( "VXVMM"     , RRE  , ASMFMT_RRE      , v_extract_vector_modes                              ),
 /*A647*/ GENx___x___x___ ,
 /*A648*/ GENx370x390x___ ( "VRRS"      , RRE  , ASMFMT_RRE      , v_restore_vr                                        ),
 /*A649*/ GENx370x390x___ ( "VRSVC"     , RRE  , ASMFMT_RRE      , v_save_changed_vr                                   ),
 /*A64A*/ GENx370x390x___ ( "VRSV"      , RRE  , ASMFMT_RRE      , v_save_vr                                           ),
 /*A64B*/ GENx___x___x___ ,
 /*A64C*/ GENx___x___x___ ,
 /*A64D*/ GENx___x___x___ ,
 /*A64E*/ GENx___x___x___ ,
 /*A64F*/ GENx___x___x___ ,
 /*A650*/ GENx___x___x___ ,
 /*A651*/ GENx___x___x___ ,
 /*A652*/ GENx___x___x___ ,
 /*A653*/ GENx___x___x___ ,
 /*A654*/ GENx___x___x___ ,
 /*A655*/ GENx___x___x___ ,
 /*A656*/ GENx___x___x___ ,
 /*A657*/ GENx___x___x___ ,
 /*A658*/ GENx___x___x___ ,
 /*A659*/ GENx___x___x___ ,
 /*A65A*/ GENx___x___x___ ,
 /*A65B*/ GENx___x___x___ ,
 /*A65C*/ GENx___x___x___ ,
 /*A65D*/ GENx___x___x___ ,
 /*A65E*/ GENx___x___x___ ,
 /*A65F*/ GENx___x___x___ ,
 /*A660*/ GENx___x___x___ ,
 /*A661*/ GENx___x___x___ ,
 /*A662*/ GENx___x___x___ ,
 /*A663*/ GENx___x___x___ ,
 /*A664*/ GENx___x___x___ ,
 /*A665*/ GENx___x___x___ ,
 /*A666*/ GENx___x___x___ ,
 /*A667*/ GENx___x___x___ ,
 /*A668*/ GENx___x___x___ ,
 /*A669*/ GENx___x___x___ ,
 /*A66A*/ GENx___x___x___ ,
 /*A66B*/ GENx___x___x___ ,
 /*A66C*/ GENx___x___x___ ,
 /*A66D*/ GENx___x___x___ ,
 /*A66E*/ GENx___x___x___ ,
 /*A66F*/ GENx___x___x___ ,
 /*A670*/ GENx___x___x___ ,
 /*A671*/ GENx___x___x___ ,
 /*A672*/ GENx___x___x___ ,
 /*A673*/ GENx___x___x___ ,
 /*A674*/ GENx___x___x___ ,
 /*A675*/ GENx___x___x___ ,
 /*A676*/ GENx___x___x___ ,
 /*A677*/ GENx___x___x___ ,
 /*A678*/ GENx___x___x___ ,
 /*A679*/ GENx___x___x___ ,
 /*A67A*/ GENx___x___x___ ,
 /*A67B*/ GENx___x___x___ ,
 /*A67C*/ GENx___x___x___ ,
 /*A67D*/ GENx___x___x___ ,
 /*A67E*/ GENx___x___x___ ,
 /*A67F*/ GENx___x___x___ ,
 /*A680*/ GENx370x390x___ ( "VLVM"      , VS   , ASMFMT_VS       , v_load_vmr                                          ),
 /*A681*/ GENx370x390x___ ( "VLCVM"     , VS   , ASMFMT_VS       , v_load_vmr_complement                               ),
 /*A682*/ GENx370x390x___ ( "VSTVM"     , VS   , ASMFMT_VS       , v_store_vmr                                         ),
 /*A683*/ GENx___x___x___ ,
 /*A684*/ GENx370x390x___ ( "VNVM"      , VS   , ASMFMT_VS       , v_and_to_vmr                                        ),
 /*A685*/ GENx370x390x___ ( "VOVM"      , VS   , ASMFMT_VS       , v_or_to_vmr                                         ),
 /*A686*/ GENx370x390x___ ( "VXVM"      , VS   , ASMFMT_VS       , v_exclusive_or_to_vmr                               ),
 /*A687*/ GENx___x___x___ ,
 /*A688*/ GENx___x___x___ ,
 /*A689*/ GENx___x___x___ ,
 /*A68A*/ GENx___x___x___ ,
 /*A68B*/ GENx___x___x___ ,
 /*A68C*/ GENx___x___x___ ,
 /*A68D*/ GENx___x___x___ ,
 /*A68E*/ GENx___x___x___ ,
 /*A68F*/ GENx___x___x___ ,
 /*A690*/ GENx___x___x___ ,
 /*A691*/ GENx___x___x___ ,
 /*A692*/ GENx___x___x___ ,
 /*A693*/ GENx___x___x___ ,
 /*A694*/ GENx___x___x___ ,
 /*A695*/ GENx___x___x___ ,
 /*A696*/ GENx___x___x___ ,
 /*A697*/ GENx___x___x___ ,
 /*A698*/ GENx___x___x___ ,
 /*A699*/ GENx___x___x___ ,
 /*A69A*/ GENx___x___x___ ,
 /*A69B*/ GENx___x___x___ ,
 /*A69C*/ GENx___x___x___ ,
 /*A69D*/ GENx___x___x___ ,
 /*A69E*/ GENx___x___x___ ,
 /*A69F*/ GENx___x___x___ ,
 /*A6A0*/ GENx___x___x___ ,
 /*A6A1*/ GENx___x___x___ ,
 /*A6A2*/ GENx___x___x___ ,
 /*A6A3*/ GENx___x___x___ ,
 /*A6A4*/ GENx___x___x___ ,
 /*A6A5*/ GENx___x___x___ ,
 /*A6A6*/ GENx___x___x___ ,
 /*A6A7*/ GENx___x___x___ ,
 /*A6A8*/ GENx___x___x___ ,
 /*A6A9*/ GENx___x___x___ ,
 /*A6AA*/ GENx___x___x___ ,
 /*A6AB*/ GENx___x___x___ ,
 /*A6AC*/ GENx___x___x___ ,
 /*A6AD*/ GENx___x___x___ ,
 /*A6AE*/ GENx___x___x___ ,
 /*A6AF*/ GENx___x___x___ ,
 /*A6B0*/ GENx___x___x___ ,
 /*A6B1*/ GENx___x___x___ ,
 /*A6B2*/ GENx___x___x___ ,
 /*A6B3*/ GENx___x___x___ ,
 /*A6B4*/ GENx___x___x___ ,
 /*A6B5*/ GENx___x___x___ ,
 /*A6B6*/ GENx___x___x___ ,
 /*A6B7*/ GENx___x___x___ ,
 /*A6B8*/ GENx___x___x___ ,
 /*A6B9*/ GENx___x___x___ ,
 /*A6BA*/ GENx___x___x___ ,
 /*A6BB*/ GENx___x___x___ ,
 /*A6BC*/ GENx___x___x___ ,
 /*A6BD*/ GENx___x___x___ ,
 /*A6BE*/ GENx___x___x___ ,
 /*A6BF*/ GENx___x___x___ ,
 /*A6C0*/ GENx370x390x___ ( "VSRSV"     , S    , ASMFMT_S        , v_save_vsr                                          ),
 /*A6C1*/ GENx370x390x___ ( "VMRSV"     , S    , ASMFMT_S        , v_save_vmr                                          ),
 /*A6C2*/ GENx370x390x___ ( "VSRRS"     , S    , ASMFMT_S        , v_restore_vsr                                       ),
 /*A6C3*/ GENx370x390x___ ( "VMRRS"     , S    , ASMFMT_S        , v_restore_vmr                                       ),
 /*A6C4*/ GENx370x390x___ ( "VLVCA"     , S    , ASMFMT_S        , v_load_vct_from_address                             ),
 /*A6C5*/ GENx370x390x___ ( "VRCL"      , S    , ASMFMT_S        , v_clear_vr                                          ),
 /*A6C6*/ GENx370x390x___ ( "VSVMM"     , S    , ASMFMT_S        , v_set_vector_mask_mode                              ),
 /*A6C7*/ GENx370x390x___ ( "VLVXA"     , S    , ASMFMT_S        , v_load_vix_from_address                             ),
 /*A6C8*/ GENx370x390x___ ( "VSTVP"     , S    , ASMFMT_S        , v_store_vector_parameters                           ),
 /*A6C9*/ GENx___x___x___ ,
 /*A6CA*/ GENx370x390x___ ( "VACSV"     , S    , ASMFMT_S        , v_save_vac                                          ),
 /*A6CB*/ GENx370x390x___ ( "VACRS"     , S    , ASMFMT_S        , v_restore_vac                                       ),
 /*A6CC*/ GENx___x___x___ ,
 /*A6CD*/ GENx___x___x___ ,
 /*A6CE*/ GENx___x___x___ ,
 /*A6CF*/ GENx___x___x___ ,
 /*A6D0*/ GENx___x___x___ ,
 /*A6D1*/ GENx___x___x___ ,
 /*A6D2*/ GENx___x___x___ ,
 /*A6D3*/ GENx___x___x___ ,
 /*A6D4*/ GENx___x___x___ ,
 /*A6D5*/ GENx___x___x___ ,
 /*A6D6*/ GENx___x___x___ ,
 /*A6D7*/ GENx___x___x___ ,
 /*A6D8*/ GENx___x___x___ ,
 /*A6D9*/ GENx___x___x___ ,
 /*A6DA*/ GENx___x___x___ ,
 /*A6DB*/ GENx___x___x___ ,
 /*A6DC*/ GENx___x___x___ ,
 /*A6DD*/ GENx___x___x___ ,
 /*A6DE*/ GENx___x___x___ ,
 /*A6DF*/ GENx___x___x___ ,
 /*A6E0*/ GENx___x___x___ ,
 /*A6E1*/ GENx___x___x___ ,
 /*A6E2*/ GENx___x___x___ ,
 /*A6E3*/ GENx___x___x___ ,
 /*A6E4*/ GENx___x___x___ ,
 /*A6E5*/ GENx___x___x___ ,
 /*A6E6*/ GENx___x___x___ ,
 /*A6E7*/ GENx___x___x___ ,
 /*A6E8*/ GENx___x___x___ ,
 /*A6E9*/ GENx___x___x___ ,
 /*A6EA*/ GENx___x___x___ ,
 /*A6EB*/ GENx___x___x___ ,
 /*A6EC*/ GENx___x___x___ ,
 /*A6ED*/ GENx___x___x___ ,
 /*A6EE*/ GENx___x___x___ ,
 /*A6EF*/ GENx___x___x___ ,
 /*A6F0*/ GENx___x___x___ ,
 /*A6F1*/ GENx___x___x___ ,
 /*A6F2*/ GENx___x___x___ ,
 /*A6F3*/ GENx___x___x___ ,
 /*A6F4*/ GENx___x___x___ ,
 /*A6F5*/ GENx___x___x___ ,
 /*A6F6*/ GENx___x___x___ ,
 /*A6F7*/ GENx___x___x___ ,
 /*A6F8*/ GENx___x___x___ ,
 /*A6F9*/ GENx___x___x___ ,
 /*A6FA*/ GENx___x___x___ ,
 /*A6FB*/ GENx___x___x___ ,
 /*A6FC*/ GENx___x___x___ ,
 /*A6FD*/ GENx___x___x___ ,
 /*A6FE*/ GENx___x___x___ ,
 /*A6FF*/ GENx___x___x___
};

static INSTR_FUNC gen_v_opcode_e4xx[256][NUM_INSTR_TAB_PTRS] =
{
 /*E400*/ GENx___x___x___ , /* VLI, VLIE */
 /*E401*/ GENx___x___x___ , /* VSTI, VSTIE */
 /*E402*/ GENx___x___x___ ,
 /*E403*/ GENx___x___x___ ,
 /*E404*/ GENx___x___x___ ,
 /*E405*/ GENx___x___x___ ,
 /*E406*/ GENx___x___x___ ,
 /*E407*/ GENx___x___x___ ,
 /*E408*/ GENx___x___x___ ,
 /*E409*/ GENx___x___x___ ,
 /*E40A*/ GENx___x___x___ ,
 /*E40B*/ GENx___x___x___ ,
 /*E40C*/ GENx___x___x___ ,
 /*E40D*/ GENx___x___x___ ,
 /*E40E*/ GENx___x___x___ ,
 /*E40F*/ GENx___x___x___ ,
 /*E410*/ GENx___x___x___ , /* VLID */
 /*E411*/ GENx___x___x___ , /* VSTID */
 /*E412*/ GENx___x___x___ ,
 /*E413*/ GENx___x___x___ ,
 /*E414*/ GENx___x___x___ ,
 /*E415*/ GENx___x___x___ ,
 /*E416*/ GENx___x___x___ ,
 /*E417*/ GENx___x___x___ ,
 /*E418*/ GENx___x___x___ ,
 /*E419*/ GENx___x___x___ ,
 /*E41A*/ GENx___x___x___ ,
 /*E41B*/ GENx___x___x___ ,
 /*E41C*/ GENx___x___x___ ,
 /*E41D*/ GENx___x___x___ ,
 /*E41E*/ GENx___x___x___ ,
 /*E41F*/ GENx___x___x___ ,
 /*E420*/ GENx___x___x___ ,
 /*E421*/ GENx___x___x___ ,
 /*E422*/ GENx___x___x___ ,
 /*E423*/ GENx___x___x___ ,
 /*E424*/ GENx___x___x___ , /* VSRL */
 /*E425*/ GENx___x___x___ , /* VSLL */
 /*E426*/ GENx___x___x___ ,
 /*E427*/ GENx___x___x___ ,
 /*E428*/ GENx___x___x___ , /* VLBIX */
 /*E429*/ GENx___x___x___ ,
 /*E42A*/ GENx___x___x___ ,
 /*E42B*/ GENx___x___x___ ,
 /*E42C*/ GENx___x___x___ ,
 /*E42D*/ GENx___x___x___ ,
 /*E42E*/ GENx___x___x___ ,
 /*E42F*/ GENx___x___x___ ,
 /*E430*/ GENx___x___x___ ,
 /*E431*/ GENx___x___x___ ,
 /*E432*/ GENx___x___x___ ,
 /*E433*/ GENx___x___x___ ,
 /*E434*/ GENx___x___x___ ,
 /*E435*/ GENx___x___x___ ,
 /*E436*/ GENx___x___x___ ,
 /*E437*/ GENx___x___x___ ,
 /*E438*/ GENx___x___x___ ,
 /*E439*/ GENx___x___x___ ,
 /*E43A*/ GENx___x___x___ ,
 /*E43B*/ GENx___x___x___ ,
 /*E43C*/ GENx___x___x___ ,
 /*E43D*/ GENx___x___x___ ,
 /*E43E*/ GENx___x___x___ ,
 /*E43F*/ GENx___x___x___ ,
 /*E440*/ GENx___x___x___ ,
 /*E441*/ GENx___x___x___ ,
 /*E442*/ GENx___x___x___ ,
 /*E443*/ GENx___x___x___ ,
 /*E444*/ GENx___x___x___ ,
 /*E445*/ GENx___x___x___ ,
 /*E446*/ GENx___x___x___ ,
 /*E447*/ GENx___x___x___ ,
 /*E448*/ GENx___x___x___ ,
 /*E449*/ GENx___x___x___ ,
 /*E44A*/ GENx___x___x___ ,
 /*E44B*/ GENx___x___x___ ,
 /*E44C*/ GENx___x___x___ ,
 /*E44D*/ GENx___x___x___ ,
 /*E44E*/ GENx___x___x___ ,
 /*E44F*/ GENx___x___x___ ,
 /*E450*/ GENx___x___x___ ,
 /*E451*/ GENx___x___x___ ,
 /*E452*/ GENx___x___x___ ,
 /*E453*/ GENx___x___x___ ,
 /*E454*/ GENx___x___x___ ,
 /*E455*/ GENx___x___x___ ,
 /*E456*/ GENx___x___x___ ,
 /*E457*/ GENx___x___x___ ,
 /*E458*/ GENx___x___x___ ,
 /*E459*/ GENx___x___x___ ,
 /*E45A*/ GENx___x___x___ ,
 /*E45B*/ GENx___x___x___ ,
 /*E45C*/ GENx___x___x___ ,
 /*E45D*/ GENx___x___x___ ,
 /*E45E*/ GENx___x___x___ ,
 /*E45F*/ GENx___x___x___ ,
 /*E460*/ GENx___x___x___ ,
 /*E461*/ GENx___x___x___ ,
 /*E462*/ GENx___x___x___ ,
 /*E463*/ GENx___x___x___ ,
 /*E464*/ GENx___x___x___ ,
 /*E465*/ GENx___x___x___ ,
 /*E466*/ GENx___x___x___ ,
 /*E467*/ GENx___x___x___ ,
 /*E468*/ GENx___x___x___ ,
 /*E469*/ GENx___x___x___ ,
 /*E46A*/ GENx___x___x___ ,
 /*E46B*/ GENx___x___x___ ,
 /*E46C*/ GENx___x___x___ ,
 /*E46D*/ GENx___x___x___ ,
 /*E46E*/ GENx___x___x___ ,
 /*E46F*/ GENx___x___x___ ,
 /*E470*/ GENx___x___x___ ,
 /*E471*/ GENx___x___x___ ,
 /*E472*/ GENx___x___x___ ,
 /*E473*/ GENx___x___x___ ,
 /*E474*/ GENx___x___x___ ,
 /*E475*/ GENx___x___x___ ,
 /*E476*/ GENx___x___x___ ,
 /*E477*/ GENx___x___x___ ,
 /*E478*/ GENx___x___x___ ,
 /*E479*/ GENx___x___x___ ,
 /*E47A*/ GENx___x___x___ ,
 /*E47B*/ GENx___x___x___ ,
 /*E47C*/ GENx___x___x___ ,
 /*E47D*/ GENx___x___x___ ,
 /*E47E*/ GENx___x___x___ ,
 /*E47F*/ GENx___x___x___ ,
 /*E480*/ GENx___x___x___ ,
 /*E481*/ GENx___x___x___ ,
 /*E482*/ GENx___x___x___ ,
 /*E483*/ GENx___x___x___ ,
 /*E484*/ GENx___x___x___ ,
 /*E485*/ GENx___x___x___ ,
 /*E486*/ GENx___x___x___ ,
 /*E487*/ GENx___x___x___ ,
 /*E488*/ GENx___x___x___ ,
 /*E489*/ GENx___x___x___ ,
 /*E48A*/ GENx___x___x___ ,
 /*E48B*/ GENx___x___x___ ,
 /*E48C*/ GENx___x___x___ ,
 /*E48D*/ GENx___x___x___ ,
 /*E48E*/ GENx___x___x___ ,
 /*E48F*/ GENx___x___x___ ,
 /*E490*/ GENx___x___x___ ,
 /*E491*/ GENx___x___x___ ,
 /*E492*/ GENx___x___x___ ,
 /*E493*/ GENx___x___x___ ,
 /*E494*/ GENx___x___x___ ,
 /*E495*/ GENx___x___x___ ,
 /*E496*/ GENx___x___x___ ,
 /*E497*/ GENx___x___x___ ,
 /*E498*/ GENx___x___x___ ,
 /*E499*/ GENx___x___x___ ,
 /*E49A*/ GENx___x___x___ ,
 /*E49B*/ GENx___x___x___ ,
 /*E49C*/ GENx___x___x___ ,
 /*E49D*/ GENx___x___x___ ,
 /*E49E*/ GENx___x___x___ ,
 /*E49F*/ GENx___x___x___ ,
 /*E4A0*/ GENx___x___x___ ,
 /*E4A1*/ GENx___x___x___ ,
 /*E4A2*/ GENx___x___x___ ,
 /*E4A3*/ GENx___x___x___ ,
 /*E4A4*/ GENx___x___x___ ,
 /*E4A5*/ GENx___x___x___ ,
 /*E4A6*/ GENx___x___x___ ,
 /*E4A7*/ GENx___x___x___ ,
 /*E4A8*/ GENx___x___x___ ,
 /*E4A9*/ GENx___x___x___ ,
 /*E4AA*/ GENx___x___x___ ,
 /*E4AB*/ GENx___x___x___ ,
 /*E4AC*/ GENx___x___x___ ,
 /*E4AD*/ GENx___x___x___ ,
 /*E4AE*/ GENx___x___x___ ,
 /*E4AF*/ GENx___x___x___ ,
 /*E4B0*/ GENx___x___x___ ,
 /*E4B1*/ GENx___x___x___ ,
 /*E4B2*/ GENx___x___x___ ,
 /*E4B3*/ GENx___x___x___ ,
 /*E4B4*/ GENx___x___x___ ,
 /*E4B5*/ GENx___x___x___ ,
 /*E4B6*/ GENx___x___x___ ,
 /*E4B7*/ GENx___x___x___ ,
 /*E4B8*/ GENx___x___x___ ,
 /*E4B9*/ GENx___x___x___ ,
 /*E4BA*/ GENx___x___x___ ,
 /*E4BB*/ GENx___x___x___ ,
 /*E4BC*/ GENx___x___x___ ,
 /*E4BD*/ GENx___x___x___ ,
 /*E4BE*/ GENx___x___x___ ,
 /*E4BF*/ GENx___x___x___ ,
 /*E4C0*/ GENx___x___x___ ,
 /*E4C1*/ GENx___x___x___ ,
 /*E4C2*/ GENx___x___x___ ,
 /*E4C3*/ GENx___x___x___ ,
 /*E4C4*/ GENx___x___x___ ,
 /*E4C5*/ GENx___x___x___ ,
 /*E4C6*/ GENx___x___x___ ,
 /*E4C7*/ GENx___x___x___ ,
 /*E4C8*/ GENx___x___x___ ,
 /*E4C9*/ GENx___x___x___ ,
 /*E4CA*/ GENx___x___x___ ,
 /*E4CB*/ GENx___x___x___ ,
 /*E4CC*/ GENx___x___x___ ,
 /*E4CD*/ GENx___x___x___ ,
 /*E4CE*/ GENx___x___x___ ,
 /*E4CF*/ GENx___x___x___ ,
 /*E4D0*/ GENx___x___x___ ,
 /*E4D1*/ GENx___x___x___ ,
 /*E4D2*/ GENx___x___x___ ,
 /*E4D3*/ GENx___x___x___ ,
 /*E4D4*/ GENx___x___x___ ,
 /*E4D5*/ GENx___x___x___ ,
 /*E4D6*/ GENx___x___x___ ,
 /*E4D7*/ GENx___x___x___ ,
 /*E4D8*/ GENx___x___x___ ,
 /*E4D9*/ GENx___x___x___ ,
 /*E4DA*/ GENx___x___x___ ,
 /*E4DB*/ GENx___x___x___ ,
 /*E4DC*/ GENx___x___x___ ,
 /*E4DD*/ GENx___x___x___ ,
 /*E4DE*/ GENx___x___x___ ,
 /*E4DF*/ GENx___x___x___ ,
 /*E4E0*/ GENx___x___x___ ,
 /*E4E1*/ GENx___x___x___ ,
 /*E4E2*/ GENx___x___x___ ,
 /*E4E3*/ GENx___x___x___ ,
 /*E4E4*/ GENx___x___x___ ,
 /*E4E5*/ GENx___x___x___ ,
 /*E4E6*/ GENx___x___x___ ,
 /*E4E7*/ GENx___x___x___ ,
 /*E4E8*/ GENx___x___x___ ,
 /*E4E9*/ GENx___x___x___ ,
 /*E4EA*/ GENx___x___x___ ,
 /*E4EB*/ GENx___x___x___ ,
 /*E4EC*/ GENx___x___x___ ,
 /*E4ED*/ GENx___x___x___ ,
 /*E4EE*/ GENx___x___x___ ,
 /*E4EF*/ GENx___x___x___ ,
 /*E4F0*/ GENx___x___x___ ,
 /*E4F1*/ GENx___x___x___ ,
 /*E4F2*/ GENx___x___x___ ,
 /*E4F3*/ GENx___x___x___ ,
 /*E4F4*/ GENx___x___x___ ,
 /*E4F5*/ GENx___x___x___ ,
 /*E4F6*/ GENx___x___x___ ,
 /*E4F7*/ GENx___x___x___ ,
 /*E4F8*/ GENx___x___x___ ,
 /*E4F9*/ GENx___x___x___ ,
 /*E4FA*/ GENx___x___x___ ,
 /*E4FB*/ GENx___x___x___ ,
 /*E4FC*/ GENx___x___x___ ,
 /*E4FD*/ GENx___x___x___ ,
 /*E4FE*/ GENx___x___x___ ,
 /*E4FF*/ GENx___x___x___
};

/*-------------------------------------------------------------------*/
/*                  "Optimized" Instructions                         */
/*-------------------------------------------------------------------*/

#ifdef OPTION_OPTINST

// Compare Logical Register

#define CLRgen(r1, r2)    GENx370x390x900 ( "CLR"       , RR   , ASMFMT_RR       , 15 ## r1 ## r2                                      )

#define CLRgenr2(r1) \
  CLRgen(r1, 0), \
  CLRgen(r1, 1), \
  CLRgen(r1, 2), \
  CLRgen(r1, 3), \
  CLRgen(r1, 4), \
  CLRgen(r1, 5), \
  CLRgen(r1, 6), \
  CLRgen(r1, 7), \
  CLRgen(r1, 8), \
  CLRgen(r1, 9), \
  CLRgen(r1, A), \
  CLRgen(r1, B), \
  CLRgen(r1, C), \
  CLRgen(r1, D), \
  CLRgen(r1, E), \
  CLRgen(r1, F)

static INSTR_FUNC gen_opcode_15__[256][NUM_INSTR_TAB_PTRS] =
{
  CLRgenr2(0),
  CLRgenr2(1),
  CLRgenr2(2),
  CLRgenr2(3),
  CLRgenr2(4),
  CLRgenr2(5),
  CLRgenr2(6),
  CLRgenr2(7),
  CLRgenr2(8),
  CLRgenr2(9),
  CLRgenr2(A),
  CLRgenr2(B),
  CLRgenr2(C),
  CLRgenr2(D),
  CLRgenr2(E),
  CLRgenr2(F)
};

// Load Register

#define LRgen(r1, r2)     GENx370x390x900 ( "LR"        , RR   , ASMFMT_RR       , 18 ## r1 ## r2                                      )

#define LRgenr2(r1) \
  LRgen(r1, 0), \
  LRgen(r1, 1), \
  LRgen(r1, 2), \
  LRgen(r1, 3), \
  LRgen(r1, 4), \
  LRgen(r1, 5), \
  LRgen(r1, 6), \
  LRgen(r1, 7), \
  LRgen(r1, 8), \
  LRgen(r1, 9), \
  LRgen(r1, A), \
  LRgen(r1, B), \
  LRgen(r1, C), \
  LRgen(r1, D), \
  LRgen(r1, E), \
  LRgen(r1, F)

static INSTR_FUNC gen_opcode_18__[256][NUM_INSTR_TAB_PTRS] =
{
  LRgenr2(0),
  LRgenr2(1),
  LRgenr2(2),
  LRgenr2(3),
  LRgenr2(4),
  LRgenr2(5),
  LRgenr2(6),
  LRgenr2(7),
  LRgenr2(8),
  LRgenr2(9),
  LRgenr2(A),
  LRgenr2(B),
  LRgenr2(C),
  LRgenr2(D),
  LRgenr2(E),
  LRgenr2(F)
};

// Add Logical Register

#define ALRgen(r1, r2)    GENx370x390x900 ( "ALR"       , RR   , ASMFMT_RR       , 1E ## r1 ## r2                                      )

#define ALRgenr2(r1) \
   ALRgen(r1, 0), \
   ALRgen(r1, 1), \
   ALRgen(r1, 2), \
   ALRgen(r1, 3), \
   ALRgen(r1, 4), \
   ALRgen(r1, 5), \
   ALRgen(r1, 6), \
   ALRgen(r1, 7), \
   ALRgen(r1, 8), \
   ALRgen(r1, 9), \
   ALRgen(r1, A), \
   ALRgen(r1, B), \
   ALRgen(r1, C), \
   ALRgen(r1, D), \
   ALRgen(r1, E), \
   ALRgen(r1, F)

static INSTR_FUNC gen_opcode_1E__[256][NUM_INSTR_TAB_PTRS] =
{
  ALRgenr2(0),
  ALRgenr2(1),
  ALRgenr2(2),
  ALRgenr2(3),
  ALRgenr2(4),
  ALRgenr2(5),
  ALRgenr2(6),
  ALRgenr2(7),
  ALRgenr2(8),
  ALRgenr2(9),
  ALRgenr2(A),
  ALRgenr2(B),
  ALRgenr2(C),
  ALRgenr2(D),
  ALRgenr2(E),
  ALRgenr2(F)
};

// Subtract Logical Register

#define SLRgen(r1, r2)    GENx370x390x900 ( "SLR"       , RR   , ASMFMT_RR       , 1F ## r1 ## r2                                      )

#define SLRgenr2(r1) \
   SLRgen(r1, 0), \
   SLRgen(r1, 1), \
   SLRgen(r1, 2), \
   SLRgen(r1, 3), \
   SLRgen(r1, 4), \
   SLRgen(r1, 5), \
   SLRgen(r1, 6), \
   SLRgen(r1, 7), \
   SLRgen(r1, 8), \
   SLRgen(r1, 9), \
   SLRgen(r1, A), \
   SLRgen(r1, B), \
   SLRgen(r1, C), \
   SLRgen(r1, D), \
   SLRgen(r1, E), \
   SLRgen(r1, F)

static INSTR_FUNC gen_opcode_1F__[256][NUM_INSTR_TAB_PTRS] =
{
  SLRgenr2(0),
  SLRgenr2(1),
  SLRgenr2(2),
  SLRgenr2(3),
  SLRgenr2(4),
  SLRgenr2(5),
  SLRgenr2(6),
  SLRgenr2(7),
  SLRgenr2(8),
  SLRgenr2(9),
  SLRgenr2(A),
  SLRgenr2(B),
  SLRgenr2(C),
  SLRgenr2(D),
  SLRgenr2(E),
  SLRgenr2(F)
};

// Load Address

static INSTR_FUNC gen_opcode_41_0[16][NUM_INSTR_TAB_PTRS] =
{
 /*4100*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 4100                                                ),
 /*4110*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 4110                                                ),
 /*4120*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 4120                                                ),
 /*4134*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 4130                                                ),
 /*4140*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 4140                                                ),
 /*4150*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 4150                                                ),
 /*4160*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 4160                                                ),
 /*4170*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 4170                                                ),
 /*4180*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 4180                                                ),
 /*4190*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 4190                                                ),
 /*41A0*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 41A0                                                ),
 /*41B0*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 41B0                                                ),
 /*41C0*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 41C0                                                ),
 /*41D0*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 41D0                                                ),
 /*41E0*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 41E0                                                ),
 /*41F0*/ GENx370x390x900 ( "LA"        , RX_a , ASMFMT_RX       , 41F0                                                )
};

// Branch on Condition

static INSTR_FUNC gen_opcode_47_0[16][NUM_INSTR_TAB_PTRS] =
{
 /*4700*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , nop4                                                ),
 /*4710*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 4710                                                ),
 /*4720*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 4720                                                ),
 /*4730*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 4730                                                ),
 /*4740*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 4740                                                ),
 /*4750*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 4750                                                ),
 /*4760*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 47_0                                                ),
 /*4770*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 4770                                                ),
 /*4780*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 4780                                                ),
 /*4790*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 47_0                                                ),
 /*47A0*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 47A0                                                ),
 /*47B0*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 47B0                                                ),
 /*47C0*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 47C0                                                ),
 /*47D0*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 47D0                                                ),
 /*47E0*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 47E0                                                ),
 /*47F0*/ GENx370x390x900 ( "BC"        , RX_b , ASMFMT_RX       , 47F0                                                )
};

// Store

static INSTR_FUNC gen_opcode_50_0[16][NUM_INSTR_TAB_PTRS] =
{
 /*5000*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 5000                                                ),
 /*5010*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 5010                                                ),
 /*5020*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 5020                                                ),
 /*5030*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 5030                                                ),
 /*5040*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 5040                                                ),
 /*5050*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 5050                                                ),
 /*5060*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 5060                                                ),
 /*5070*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 5070                                                ),
 /*5080*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 5080                                                ),
 /*5090*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 5090                                                ),
 /*50A0*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 50A0                                                ),
 /*50B0*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 50B0                                                ),
 /*50C0*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 50C0                                                ),
 /*50D0*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 50D0                                                ),
 /*50E0*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 50E0                                                ),
 /*50F0*/ GENx370x390x900 ( "ST"        , RX_a , ASMFMT_RX       , 50F0                                                )
};

// Compare Logical

static INSTR_FUNC gen_opcode_55_0[16][NUM_INSTR_TAB_PTRS] =
{
 /*5500*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 5500                                                ),
 /*5510*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 5510                                                ),
 /*5520*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 5520                                                ),
 /*5530*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 5530                                                ),
 /*5540*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 5540                                                ),
 /*5550*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 5550                                                ),
 /*5560*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 5560                                                ),
 /*5570*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 5570                                                ),
 /*5580*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 5580                                                ),
 /*5590*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 5590                                                ),
 /*55A0*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 55A0                                                ),
 /*55B0*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 55B0                                                ),
 /*55C0*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 55C0                                                ),
 /*55D0*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 55D0                                                ),
 /*55E0*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 55E0                                                ),
 /*55F0*/ GENx370x390x900 ( "CL"        , RX_a , ASMFMT_RX       , 55F0                                                )
};

// Load

static INSTR_FUNC gen_opcode_58_0[16][NUM_INSTR_TAB_PTRS] =
{
 /*5800*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 5800                                                ),
 /*5810*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 5810                                                ),
 /*5820*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 5820                                                ),
 /*5830*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 5830                                                ),
 /*5840*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 5840                                                ),
 /*5850*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 5850                                                ),
 /*5860*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 5860                                                ),
 /*5870*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 5870                                                ),
 /*5880*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 5880                                                ),
 /*5890*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 5890                                                ),
 /*58A0*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 58A0                                                ),
 /*58B0*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 58B0                                                ),
 /*58C0*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 58C0                                                ),
 /*58D0*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 58D0                                                ),
 /*58E0*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 58E0                                                ),
 /*58F0*/ GENx370x390x900 ( "L"         , RX_a , ASMFMT_RX       , 58F0                                                )
};

// Test Under Mask

static INSTR_FUNC gen_opcode_91xx[8][NUM_INSTR_TAB_PTRS] =
{
 /*9180*/ GENx370x390x900 ( "TM"        , SI   , ASMFMT_SI       , 9180                                                ),
 /*9140*/ GENx370x390x900 ( "TM"        , SI   , ASMFMT_SI       , 9140                                                ),
 /*9120*/ GENx370x390x900 ( "TM"        , SI   , ASMFMT_SI       , 9120                                                ),
 /*9110*/ GENx370x390x900 ( "TM"        , SI   , ASMFMT_SI       , 9110                                                ),
 /*9108*/ GENx370x390x900 ( "TM"        , SI   , ASMFMT_SI       , 9108                                                ),
 /*9104*/ GENx370x390x900 ( "TM"        , SI   , ASMFMT_SI       , 9104                                                ),
 /*9102*/ GENx370x390x900 ( "TM"        , SI   , ASMFMT_SI       , 9102                                                ),
 /*9101*/ GENx370x390x900 ( "TM"        , SI   , ASMFMT_SI       , 9101                                                )
};

// Insert Characters Under Mask

static INSTR_FUNC gen_opcode_BF_x[3][NUM_INSTR_TAB_PTRS] =
{
 /*BF_x*/ GENx370x390x900 ( "ICM"       , RS_b , ASMFMT_RS       , BF_x                                                ),
 /*BF_7*/ GENx370x390x900 ( "ICM"       , RS_b , ASMFMT_RS       , BF_7                                                ),
 /*BF_F*/ GENx370x390x900 ( "ICM"       , RS_b , ASMFMT_RS       , BF_F                                                )
};

#if !defined( OPTION_NO_E3_OPTINST )

static INSTR_FUNC gen_opcode_E3_0[1][NUM_INSTR_TAB_PTRS] =
{
 /*E3*/   GENx370x390x900 ( ""          , e3xx , ASMFMT_e3xx     , E3_0                                                )
};

// Load 64-bit

static INSTR_FUNC gen_opcode_E3_0______04[1][NUM_INSTR_TAB_PTRS] =
{
 /*E304*/ GENx___x___x900 ( "LG"        , RXY_a, ASMFMT_RXY      , E3_0______04                                        )
};

// Store 64-bit

static INSTR_FUNC gen_opcode_E3_0______24[1][NUM_INSTR_TAB_PTRS] =
{
 /*E324*/ GENx___x___x900 ( "STG"       , RXY_a, ASMFMT_RXY      , E3_0______24                                        )
};

#endif /* !defined( OPTION_NO_E3_OPTINST ) */

#endif /* OPTION_OPTINST */

/*-------------------------------------------------------------------*/
/*                     runtime opcode tables                         */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The below tables are the opcode tables actually used at runtime.  */
/*                                                                   */
/* They are built during startup by the "init_runtime_opcode_tables" */
/* function further below (called by impl.c during initialization)   */
/* based on the above GENx... "generated" opcode tables.             */
/*                                                                   */
/* Note that while each entry in the ABOVE "gen" opcode tables built */
/* at compile time are indexed by instruction opcode (and contain 5  */
/* pointers, the first 3 of which are pointers to the instruction    */
/* function for the 3 build architectures that Hercules supports),   */
/* the BELOW runtime tables are indexed by BUILD ARCHITECTURE and    */
/* contain only ONE pointer (with that pointer being the pointer to  */
/* the instruction function for that build architecture).            */
/*                                                                   */
/* It is the below runtime tables which are copied to REGS for each  */
/* CPU by the "init_regs_runtime_opcode_pointers" function below,    */
/* which is then used by instruction dispatcher EXECUTE_INSTRUCTION  */
/* macro to jump to the actual instruction function.                 */
/*                                                                   */
/*-------------------------------------------------------------------*/

static INSTR_FUNC runtime_opcode_xxxx[NUM_GEN_ARCHS][256 * 256];

static INSTR_FUNC runtime_opcode_e3________xx[NUM_GEN_ARCHS][256];
static INSTR_FUNC runtime_opcode_eb________xx[NUM_GEN_ARCHS][256];
static INSTR_FUNC runtime_opcode_ec________xx[NUM_GEN_ARCHS][256];
static INSTR_FUNC runtime_opcode_ed________xx[NUM_GEN_ARCHS][256];

#if defined( OPTION_OPTINST ) && !defined( OPTION_NO_E3_OPTINST )
static INSTR_FUNC runtime_opcode_e3_0______xx[NUM_GEN_ARCHS][256];
#endif

/*-------------------------------------------------------------------*/
/*                Opcode Table Replacement Functions                 */
/*-------------------------------------------------------------------*/
/* The below series of functions are used to replace entries in the  */
/* above RUNTIME opcode tables for a given instruction/instructions. */
/* The first few functions are helper functions used by the primary  */
/* functions which follow them further below.                        */
/*-------------------------------------------------------------------*/

static INSTR_FUNC replace_opcode_xx(int arch, INSTR_FUNC inst, int opcode)
{
  int i;
  INSTR_FUNC oldinst;

  if(arch < 0 || arch > NUM_GEN_ARCHS)
    return(NULL);

  if(opcode < 0 || opcode > 0xff)
    return(NULL);

  if(!inst)
    return(NULL);

  oldinst = runtime_opcode_xxxx[arch][opcode * 256];

  for(i = 0; i < 256; i++)
    runtime_opcode_xxxx[arch][opcode * 256 + i] = inst;

  return(oldinst);
}

/*-------------------------------------------------------------------*/

static INSTR_FUNC replace_opcode_xxxx(int arch, INSTR_FUNC inst, int opcode1, int opcode2)
{
  INSTR_FUNC oldinst;

  if(arch < 0 || arch >= NUM_GEN_ARCHS)
    return(NULL);

  if(opcode1 < 0 || opcode1 > 0xff || opcode2 < 0 || opcode2 > 0xff)
    return(NULL);

  if(!inst)
    return(NULL);

  oldinst = runtime_opcode_xxxx[arch][opcode1 * 256 + opcode2];
  runtime_opcode_xxxx[arch][opcode1 * 256 + opcode2] = inst;

  return(oldinst);
}

/*-------------------------------------------------------------------*/

static INSTR_FUNC replace_opcode_xx_x(int arch, INSTR_FUNC inst, int opcode1, int opcode2)
{
  int i;
  INSTR_FUNC oldinst;

  if(arch < 0 || arch >= NUM_GEN_ARCHS)
    return(NULL);

  if(opcode1 < 0 || opcode1 > 0xff || opcode2 < 0 || opcode2 > 0xf)
    return(NULL);

  if(!inst)
    return(NULL);

  oldinst = runtime_opcode_xxxx[arch][opcode1 * 256 + opcode2];

  for(i = 0; i < 16; i++)
    runtime_opcode_xxxx[arch][opcode1 * 256 + i * 16 + opcode2] = inst;

  return(oldinst);
}

/*-------------------------------------------------------------------*/

static INSTR_FUNC replace_opcode_xx________xx( int arch, INSTR_FUNC inst, int opcode1, int opcode2 )
{
  INSTR_FUNC  oldinst   = NULL;

    if (0
        || !inst
        || arch < 0
        || arch >= NUM_GEN_ARCHS
        || opcode2 < 0
        || opcode2 > 0xff
    )
        return NULL;

    switch (opcode1)
    {
        case 0xe3:
        {

#if defined( OPTION_OPTINST) && !defined( OPTION_NO_E3_OPTINST )
  #error E3 instruction optimization is not supported! (yet?)
#endif
            oldinst = runtime_opcode_e3________xx[arch][opcode2];
                      runtime_opcode_e3________xx[arch][opcode2] = inst;
            break;
        }
        case 0xeb:
        {
            oldinst = runtime_opcode_eb________xx[arch][opcode2];
                      runtime_opcode_eb________xx[arch][opcode2] = inst;
            break;
        }
        case 0xec:
        {
            oldinst = runtime_opcode_ec________xx[arch][opcode2];
                      runtime_opcode_ec________xx[arch][opcode2] = inst;
            break;
        }
        case 0xed:
        {
            oldinst = runtime_opcode_ed________xx[arch][opcode2];
                      runtime_opcode_ed________xx[arch][opcode2] = inst;
            break;
        }
        default:
        {
            oldinst = NULL;
            break;
        }
    }
    return oldinst;
}

/*-------------------------------------------------------------------*/
/*               the_real_replace_opcode                             */
/*-------------------------------------------------------------------*/
/* Called by hdl.c for dynamic instruction modules such as dyncrypt  */
/*-------------------------------------------------------------------*/
DLL_EXPORT void* the_real_replace_opcode( int arch, INSTR_FUNC inst, int opcode1, int opcode2 )
{
  switch(opcode1)
  {
    case 0x01:
    case 0xa4:
    case 0xa6:
    case 0xb2:
    case 0xb3:
    case 0xb9:
    case 0xe4:
    case 0xe5:
    case 0xe6:

    {
      return(replace_opcode_xxxx(arch, inst, opcode1, opcode2));
    }

    case 0xa5:
    {
      if(arch == ARCH_900_IDX)
        return(replace_opcode_xx_x(arch, inst, opcode1, opcode2));

      #if (!defined( _VEC_FAC_DEFINED_FOR_370 )) && defined( _FEATURE_370_EXTENSION )
      if(arch == ARCH_370_IDX)
        return(replace_opcode_xx_x(arch, inst, opcode1, opcode2));
      #endif

      return(replace_opcode_xxxx(arch, inst, opcode1, opcode2));
    }

    case 0xa7:
    case 0xc0:
    case 0xc2:
    case 0xc4:
    case 0xc6:
    case 0xc8:
    case 0xcc:

    {
      return(replace_opcode_xx_x(arch, inst, opcode1, opcode2));
    }

    case 0xe3:
    case 0xeb:
    case 0xec:
    case 0xed:

    {
      return(replace_opcode_xx________xx(arch, inst, opcode1, opcode2));
    }

    default:
    {
      return(replace_opcode_xx(arch, inst, opcode1));
    }
  }

  UNREACHABLE_CODE( return NULL );
}

/*-------------------------------------------------------------------*/
/*  init_runtime_opcode_tables  --  called by impl.c function impl   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The below function constructs our RUNTIME opcode table (defined   */
/* further above) for each build architecture, based on our master   */
/* "generated" opcode table that was built by the compiler from all  */
/* of the "GENx370x390x900" et al. macros seen much further above.   */
/*                                                                   */
/* It accomplishes this by leveraging our existing "replace_opcode"  */
/* functions that allows us to "patch" (modify) the actual runtime   */
/* opcode tables when needed (whenever a given facility is enabled   */
/* or disabled for example).                                         */
/*                                                                   */
/* Instead of "patching" only specific instructions however (such    */
/* as what occurs when a given facility is enabled or disabled) it   */
/* instead "patches" (modifies/updates) *ALL* of the instructions.   */
/* It iterates through each and every entry in ALL of "generated"    */
/* opcode tables, calling the appropriate "replace_opcode" function  */
/* for each table entry (i.e. for each and every instruction).       */
/*                                                                   */
/* The end result of this is a RUNTIME opcode table for each build   */
/* architectue whose entries point DIRECTLY to that instruction for  */
/* that particular architecture. This resulting runtime opcode table */
/* can then later be further modified/patched (if needed) for only   */
/* specific instructions whenever a facility is enabled/disabled.    */
/*                                                                   */
/*-------------------------------------------------------------------*/

void init_runtime_opcode_tables()
{
  int arch;
  int i;
#ifdef OPTION_OPTINST
  int bit;
#endif

  for(arch = 0; arch < NUM_GEN_ARCHS; arch++)
  {
    for(i = 0; i < 256; i++)
      replace_opcode_xx(arch, gen_opcode_table[i][arch], i);

    for(i = 0; i < 256; i++)
    {
      replace_opcode_xxxx(arch, gen_opcode_01xx[i][arch], 0x01, i);

      if(arch != ARCH_900_IDX)
      {
        replace_opcode_xxxx(arch, gen_v_opcode_a4xx[i][arch], 0xa4, i);
        #if defined( _VEC_FAC_DEFINED_FOR_370 )
        if(arch == ARCH_370_IDX)
          replace_opcode_xxxx(arch, gen_v_opcode_a5xx[i][arch], 0xa5, i);
        #endif
        #if defined( _VEC_FAC_DEFINED_FOR_390 )
        if(arch == ARCH_390_IDX)
          replace_opcode_xxxx(arch, gen_v_opcode_a5xx[i][arch], 0xa5, i);
        #endif
        replace_opcode_xxxx(arch, gen_v_opcode_a6xx[i][arch], 0xa6, i);
      }

      // b2xx opcode table is ARCH_DEP!

           if (ARCH_370_IDX == arch) replace_opcode_xxxx( arch, s370_gen_opcode_b2xx[i][arch], 0xb2, i );
      else if (ARCH_390_IDX == arch) replace_opcode_xxxx( arch, s390_gen_opcode_b2xx[i][arch], 0xb2, i );
      else if (ARCH_900_IDX == arch) replace_opcode_xxxx( arch, z900_gen_opcode_b2xx[i][arch], 0xb2, i );
      else
          CRASH();

      replace_opcode_xxxx(arch, gen_opcode_b3xx[i][arch], 0xb3, i);
      replace_opcode_xxxx(arch, gen_opcode_b9xx[i][arch], 0xb9, i);
      replace_opcode_xx________xx(arch, gen_opcode_e3xx[i][arch], 0xe3, i);

      if(arch != ARCH_900_IDX)
        replace_opcode_xxxx(arch, gen_v_opcode_e4xx[i][arch], 0xe4, i);

      replace_opcode_xxxx(arch, gen_opcode_e5xx[i][arch], 0xe5, i);
      replace_opcode_xxxx(arch, gen_opcode_e6xx[i][arch], 0xe6, i);
      replace_opcode_xx________xx(arch, gen_opcode_ebxx[i][arch], 0xeb, i);
      replace_opcode_xx________xx(arch, gen_opcode_ecxx[i][arch], 0xec, i);
      replace_opcode_xx________xx(arch, gen_opcode_edxx[i][arch], 0xed, i);
    }

    for(i = 0; i < 16; i++)
    {
      if(arch == ARCH_900_IDX)
        replace_opcode_xx_x(arch, gen_opcode_a5_x[i][arch], 0xa5, i);
      #if (!defined( _VEC_FAC_DEFINED_FOR_370 )) && defined( _FEATURE_370_EXTENSION )
      if(arch == ARCH_370_IDX)
        replace_opcode_xx_x(arch, gen_opcode_a5_x[i][arch], 0xa5, i);
      #endif
      replace_opcode_xx_x(arch, gen_opcode_a7_x[i][arch], 0xa7, i);
      replace_opcode_xx_x(arch, gen_opcode_c0_x[i][arch], 0xc0, i);
      replace_opcode_xx_x(arch, gen_opcode_c2_x[i][arch], 0xc2, i);
      replace_opcode_xx_x(arch, gen_opcode_c4_x[i][arch], 0xc4, i);
      replace_opcode_xx_x(arch, gen_opcode_c6_x[i][arch], 0xc6, i);
      replace_opcode_xx_x(arch, gen_opcode_c8_x[i][arch], 0xc8, i);
      replace_opcode_xx_x(arch, gen_opcode_cc_x[i][arch], 0xcc, i);
    }

    // "Optimized" Instructions

#if defined( OPTION_OPTINST )

    for(i = 0; i < 256; i++)
    {
      replace_opcode_xxxx(arch, gen_opcode_15__[i][arch], 0x15, i); /* Optimized CLR */
      replace_opcode_xxxx(arch, gen_opcode_18__[i][arch], 0x18, i); /* Optimized LR */
      replace_opcode_xxxx(arch, gen_opcode_1E__[i][arch], 0x1e, i); /* Optimized ALR */
      replace_opcode_xxxx(arch, gen_opcode_1F__[i][arch], 0x1f, i); /* Optimized SLR */
      replace_opcode_xxxx(arch, gen_opcode_BF_x[0][arch], 0xbf, i); /* Optimized ICM */
    }

    for(i = 0; i < 16; i++)
    {
      replace_opcode_xxxx(arch, gen_opcode_41_0[i][arch], 0x41, i << 4); /* Optimized LA */
      replace_opcode_xxxx(arch, gen_opcode_47_0[i][arch], 0x47, i << 4); /* Optimized BC */
      replace_opcode_xxxx(arch, gen_opcode_50_0[i][arch], 0x50, i << 4); /* Optimized ST */
      replace_opcode_xxxx(arch, gen_opcode_55_0[i][arch], 0x55, i << 4); /* Optimized CL */
      replace_opcode_xxxx(arch, gen_opcode_58_0[i][arch], 0x58, i << 4); /* Optimized L */
      replace_opcode_xxxx(arch, gen_opcode_BF_x[1][arch], 0xbf, (i << 4) + 0x7); /* Optimized ICM */
      replace_opcode_xxxx(arch, gen_opcode_BF_x[2][arch], 0xbf, (i << 4) + 0xf); /* Optimized ICM */
#if !defined( OPTION_NO_E3_OPTINST )
      replace_opcode_xxxx(arch, gen_opcode_E3_0[0][arch], 0xe3, i << 4);
#endif
    }

    bit = 0x80;

    for(i = 0; i < 8; i++)
    {
      replace_opcode_xxxx(arch, gen_opcode_91xx[i][arch], 0x91, bit); /* Single bit TM */
      bit >>= 1;
    }

#if !defined( OPTION_NO_E3_OPTINST )

    for(i = 0; i < 256; i++)
    {
      switch(i)
      {
        case 0x04:

          runtime_opcode_e3_0______xx[arch][i] = gen_opcode_E3_0______04[0][arch]; /* Optimized LG */
          break;

        case 0x24:

          runtime_opcode_e3_0______xx[arch][i] = gen_opcode_E3_0______24[0][arch]; /* Optimized STG */
          break;

        default:

          runtime_opcode_e3_0______xx[arch][i] = gen_opcode_e3xx[i][arch];
          break;
      }
    }

#endif /* !defined( OPTION_NO_E3_OPTINST ) */

#endif /* defined( OPTION_OPTINST ) */
  }
}

/*-------------------------------------------------------------------*/
/*                init_regs_runtime_opcode_pointers                  */
/*               (called by cpu.c function cpu_init)                 */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The below function simply copies the pointers to our previously   */
/* built "runtime" opcode table (of which there is one for each of   */
/* our build architectures, each one of which which were previously  */
/* initialzed by "init_runtime_opcode_tables" called during impl),   */
/* to the appropriately named variable defined in REGS.              */
/*                                                                   */
/* The "run_cpu" function then simply uses the appropriate runtime   */
/* opcode table from REGS based (on the CPU's current architecture)  */
/* to dispatch that CPU's instructions to the right function based   */
/* on the instruction's opcode.                                      */
/*                                                                   */
/*-------------------------------------------------------------------*/

void init_regs_runtime_opcode_pointers( REGS* regs )
{
  if (!regs)
    return;

  regs->s370_runtime_opcode_xxxx         = runtime_opcode_xxxx        [ARCH_370_IDX];
  regs->s370_runtime_opcode_e3________xx = runtime_opcode_e3________xx[ARCH_370_IDX];
  regs->s370_runtime_opcode_eb________xx = runtime_opcode_eb________xx[ARCH_370_IDX];
  regs->s370_runtime_opcode_ec________xx = runtime_opcode_ec________xx[ARCH_370_IDX];
  regs->s370_runtime_opcode_ed________xx = runtime_opcode_ed________xx[ARCH_370_IDX];

  regs->s390_runtime_opcode_xxxx         = runtime_opcode_xxxx        [ARCH_390_IDX];
  regs->s390_runtime_opcode_e3________xx = runtime_opcode_e3________xx[ARCH_390_IDX];
  regs->s390_runtime_opcode_eb________xx = runtime_opcode_eb________xx[ARCH_390_IDX];
  regs->s390_runtime_opcode_ec________xx = runtime_opcode_ec________xx[ARCH_390_IDX];
  regs->s390_runtime_opcode_ed________xx = runtime_opcode_ed________xx[ARCH_390_IDX];

  regs->z900_runtime_opcode_xxxx         = runtime_opcode_xxxx        [ARCH_900_IDX];
  regs->z900_runtime_opcode_e3________xx = runtime_opcode_e3________xx[ARCH_900_IDX];
  regs->z900_runtime_opcode_eb________xx = runtime_opcode_eb________xx[ARCH_900_IDX];
  regs->z900_runtime_opcode_ec________xx = runtime_opcode_ec________xx[ARCH_900_IDX];
  regs->z900_runtime_opcode_ed________xx = runtime_opcode_ed________xx[ARCH_900_IDX];

#if defined( OPTION_OPTINST ) && !defined( OPTION_NO_E3_OPTINST )
  regs->s370_runtime_opcode_e3_0______xx = runtime_opcode_e3_0______xx[ARCH_370_IDX];
  regs->s390_runtime_opcode_e3_0______xx = runtime_opcode_e3_0______xx[ARCH_390_IDX];
  regs->z900_runtime_opcode_e3_0______xx = runtime_opcode_e3_0______xx[ARCH_900_IDX];
#endif
}

#endif /* !defined( _GEN_ARCH ) */
