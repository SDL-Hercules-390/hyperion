/* OPCODE.C     (c) Copyright Jan Jaeger, 2000-2012                  */
/*              (c) Copyright Roger Bowler, 2010-2011                */
/*              (c) Copyright TurboHercules, SAS 2010-2011           */
/*              Instruction decoding functions                       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (c) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (c) Copyright Jan Jaeger, 1999-2012      */

#include "hstdinc.h"

/* A  number  of  dummy functions are defined by UNDEF_INST that are */
/* never  referenced  because the actual instruction does not define */
/* the instruction for a particular architecture.  This applies, for */
/* example  to  z900_insert_storage_key and s390_insert_storage_key. */
/* With  clang,  the z900 instruction is diagnosed, but the s390 one */
/* is  not.   Go  figure.   Trying to fix this through the morass of */
/* recursive #include is not compatible with retaining one's sanity. */
/* Thus, for this module, we shoot the messenger.                    */

DISABLE_GCC_UNUSED_FUNCTION_WARNING

#if !defined(_HENGINE_DLL_)
#define _HENGINE_DLL_
#endif

#if !defined(_OPCODE_C_)
#define _OPCODE_C_
#endif

#include "feature.h"

#if !defined(_GEN_ARCH)

#if defined(_ARCHMODE3)
 #define  _GEN_ARCH _ARCHMODE3
 #include "opcode.c"
 #undef   _GEN_ARCH
#endif

#if defined(_ARCHMODE2)
 #define  _GEN_ARCH _ARCHMODE2
 #include "opcode.c"
 #undef   _GEN_ARCH
#endif

#endif /*!defined(_GEN_ARCH)*/

#include "hercules.h"

#include "opcode.h"

#define UNDEF_INST(_x) \
 static DEF_INST(_x) { ARCH_DEP(operation_exception) \
        (inst,regs); }


#if !defined(FEATURE_CHANNEL_SUBSYSTEM)
 UNDEF_INST(clear_subchannel)
 UNDEF_INST(halt_subchannel)
 UNDEF_INST(modify_subchannel)
 UNDEF_INST(reset_channel_path)
 UNDEF_INST(resume_subchannel)
 UNDEF_INST(set_address_limit)
 UNDEF_INST(set_channel_monitor)
 UNDEF_INST(start_subchannel)
 UNDEF_INST(store_channel_path_status)
 UNDEF_INST(store_channel_report_word)
 UNDEF_INST(store_subchannel)
 UNDEF_INST(test_pending_interruption)
 UNDEF_INST(test_subchannel)
#endif /*!defined(FEATURE_CHANNEL_SUBSYSTEM)*/


#if !defined(FEATURE_S370_CHANNEL)
 UNDEF_INST(start_io)
 UNDEF_INST(test_io)
 UNDEF_INST(halt_io)
 UNDEF_INST(test_channel)
 UNDEF_INST(store_channel_id)
#endif /*!defined(FEATURE_S370_CHANNEL)*/


#if !defined(FEATURE_IMMEDIATE_AND_RELATIVE)
 UNDEF_INST(test_under_mask_high)
 UNDEF_INST(test_under_mask_low)
 UNDEF_INST(branch_relative_on_condition)
 UNDEF_INST(branch_relative_and_save)
 UNDEF_INST(branch_relative_on_count)
 UNDEF_INST(load_halfword_immediate)
 UNDEF_INST(add_halfword_immediate)
 UNDEF_INST(multiply_halfword_immediate)
 UNDEF_INST(compare_halfword_immediate)
 UNDEF_INST(multiply_single_register)
 UNDEF_INST(multiply_single)
 UNDEF_INST(branch_relative_on_index_high)
 UNDEF_INST(branch_relative_on_index_low_or_equal)
#endif /*!defined(FEATURE_IMMEDIATE_AND_RELATIVE)*/


#if !defined(FEATURE_STRING_INSTRUCTION)
 UNDEF_INST(compare_logical_string)
 UNDEF_INST(compare_until_substring_equal)
 UNDEF_INST(move_string)
 UNDEF_INST(search_string)
#endif /*!defined(FEATURE_STRING_INSTRUCTION)*/


#if !defined(FEATURE_COMPARE_AND_MOVE_EXTENDED)
 UNDEF_INST(compare_logical_long_extended)
 UNDEF_INST(move_long_extended)
#endif /*!defined(FEATURE_COMPARE_AND_MOVE_EXTENDED)*/


#if !defined(FEATURE_CHECKSUM_INSTRUCTION)
 UNDEF_INST(checksum)
#endif /*!defined(FEATURE_CHECKSUM_INSTRUCTION)*/


#if !defined(FEATURE_PERFORM_LOCKED_OPERATION)
 UNDEF_INST(perform_locked_operation)
#endif /*!defined(FEATURE_PERFORM_LOCKED_OPERATION)*/


#if !defined(FEATURE_SUBSPACE_GROUP)
 UNDEF_INST(branch_in_subspace_group)
#endif /*!defined(FEATURE_SUBSPACE_GROUP)*/


#if !defined(FEATURE_SET_ADDRESS_SPACE_CONTROL_FAST)
 UNDEF_INST(set_address_space_control_fast)
#else /*!defined(FEATURE_SET_ADDRESS_SPACE_CONTROL_FAST)*/
 #define s390_set_address_space_control_fast s390_set_address_space_control
 #define z900_set_address_space_control_fast z900_set_address_space_control
#endif /*!defined(FEATURE_SET_ADDRESS_SPACE_CONTROL_FAST)*/


#if !defined(FEATURE_BRANCH_AND_SET_AUTHORITY)
 UNDEF_INST(branch_and_set_authority)
#endif /*!defined(FEATURE_BRANCH_AND_SET_AUTHORITY)*/


#if !defined(FEATURE_EXPANDED_STORAGE)
 UNDEF_INST(page_in)
 UNDEF_INST(page_out)
#endif /*!defined(FEATURE_EXPANDED_STORAGE)*/


#if !defined(FEATURE_BROADCASTED_PURGING)
 UNDEF_INST(compare_and_swap_and_purge)
#endif /*!defined(FEATURE_BROADCASTED_PURGING)*/


#if !defined(FEATURE_BIMODAL_ADDRESSING)
 UNDEF_INST(branch_and_set_mode)
 UNDEF_INST(branch_and_save_and_set_mode)
#endif /*!defined(FEATURE_BIMODAL_ADDRESSING)*/


#if !defined(FEATURE_MOVE_PAGE_FACILITY_2)
 UNDEF_INST(move_page)
 UNDEF_INST(invalidate_expanded_storage_block_entry)
#endif /*!defined(FEATURE_MOVE_PAGE_FACILITY_2)*/


#if !defined(FEATURE_BASIC_STORAGE_KEYS)
 UNDEF_INST(insert_storage_key)
 UNDEF_INST(set_storage_key)
 UNDEF_INST(reset_reference_bit)
#endif /*!defined(FEATURE_BASIC_STORAGE_KEYS)*/


#if !defined(FEATURE_LINKAGE_STACK)
 UNDEF_INST(branch_and_stack)
 UNDEF_INST(modify_stacked_state)
 UNDEF_INST(extract_stacked_registers)
 UNDEF_INST(extract_stacked_state)
 UNDEF_INST(program_return)
 UNDEF_INST(trap2)
 UNDEF_INST(trap4)
#endif /*!defined(FEATURE_LINKAGE_STACK)*/


#if !defined(FEATURE_DUAL_ADDRESS_SPACE)
 UNDEF_INST(extract_primary_asn)
 UNDEF_INST(extract_secondary_asn)
 UNDEF_INST(insert_address_space_control)
 UNDEF_INST(insert_virtual_storage_key)
 UNDEF_INST(load_address_space_parameters)
 UNDEF_INST(move_to_primary)
 UNDEF_INST(move_to_secondary)
 UNDEF_INST(move_with_key)
 UNDEF_INST(program_call)
 UNDEF_INST(program_transfer)
 UNDEF_INST(set_address_space_control)
 UNDEF_INST(set_secondary_asn)
#endif /*!defined(FEATURE_DUAL_ADDRESS_SPACE)*/


#if !defined(FEATURE_ASN_AND_LX_REUSE)
 UNDEF_INST(extract_primary_asn_and_instance)
 UNDEF_INST(extract_secondary_asn_and_instance)
 UNDEF_INST(program_transfer_with_instance)
 UNDEF_INST(set_secondary_asn_with_instance)
#endif /*!defined(FEATURE_ASN_AND_LX_REUSE)*/


#if !defined(FEATURE_ACCESS_REGISTERS)
 UNDEF_INST(load_access_multiple)
 UNDEF_INST(store_access_multiple)
 UNDEF_INST(purge_accesslist_lookaside_buffer)
 UNDEF_INST(test_access)
 UNDEF_INST(copy_access)
 UNDEF_INST(set_access_register)
 UNDEF_INST(extract_access_register)
#endif /*!defined(FEATURE_ACCESS_REGISTERS)*/


#if !defined(FEATURE_EXTENDED_STORAGE_KEYS)
 UNDEF_INST(insert_storage_key_extended)
 UNDEF_INST(reset_reference_bit_extended)
 UNDEF_INST(set_storage_key_extended)
#endif /*!defined(FEATURE_EXTENDED_STORAGE_KEYS)*/


#if !defined(FEATURE_TOD_CLOCK_STEERING)
 UNDEF_INST(perform_timing_facility_function)
#endif


#if !defined(FEATURE_EXTENDED_TOD_CLOCK)
 UNDEF_INST(set_clock_programmable_field)
 UNDEF_INST(store_clock_extended)
#endif /*!defined(FEATURE_EXTENDED_TOD_CLOCK)*/


#if !defined(FEATURE_MOVE_WITH_OPTIONAL_SPECIFICATIONS)
 UNDEF_INST(move_with_optional_specifications)
#endif /*!defined(FEATURE_MOVE_WITH_OPTIONAL_SPECIFICATIONS)*/


#if !defined(FEATURE_EXTRACT_CPU_TIME)
 UNDEF_INST(extract_cpu_time)
#endif /*!defined(FEATURE_EXTRACT_CPU_TIME)*/


#if !defined(FEATURE_COMPARE_AND_SWAP_AND_STORE)
 UNDEF_INST(compare_and_swap_and_store)
#endif /*!defined(FEATURE_COMPARE_AND_SWAP_AND_STORE)*/


#if !defined(FEATURE_STORE_SYSTEM_INFORMATION)
 UNDEF_INST(store_system_information)
#endif /*!defined(FEATURE_STORE_SYSTEM_INFORMATION)*/


#if !defined(FEATURE_CONFIGURATION_TOPOLOGY_FACILITY)           /*208*/
 UNDEF_INST(perform_topology_function)                          /*208*/
#endif /*!defined(FEATURE_CONFIGURATION_TOPOLOGY_FACILITY)*/    /*208*/


#if !defined(FEATURE_ENHANCED_DAT_FACILITY)                     /*208*/
 UNDEF_INST(perform_frame_management_function)                  /*208*/
#endif /*!defined(FEATURE_ENHANCED_DAT_FACILITY)*/              /*208*/


#if !defined(FEATURE_EXECUTE_EXTENSIONS_FACILITY)               /*208*/
 UNDEF_INST(execute_relative_long)                              /*208*/
#endif /*!defined(FEATURE_EXECUTE_EXTENSIONS_FACILITY)*/        /*208*/


#if !defined(FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY)   /*208*/
 UNDEF_INST(add_immediate_long_storage)
 UNDEF_INST(add_immediate_storage)
 UNDEF_INST(add_logical_with_signed_immediate)
 UNDEF_INST(add_logical_with_signed_immediate_long)
 UNDEF_INST(compare_and_branch_register)
 UNDEF_INST(compare_and_branch_long_register)
 UNDEF_INST(compare_and_branch_relative_register)
 UNDEF_INST(compare_and_branch_relative_long_register)
 UNDEF_INST(compare_and_trap_long_register)
 UNDEF_INST(compare_and_trap_register)
 UNDEF_INST(compare_halfword_immediate_halfword_storage)
 UNDEF_INST(compare_halfword_immediate_long_storage)
 UNDEF_INST(compare_halfword_immediate_storage)
 UNDEF_INST(compare_halfword_long)
 UNDEF_INST(compare_halfword_relative_long)
 UNDEF_INST(compare_halfword_relative_long_long)
 UNDEF_INST(compare_immediate_and_branch)
 UNDEF_INST(compare_immediate_and_branch_long)
 UNDEF_INST(compare_immediate_and_branch_relative)
 UNDEF_INST(compare_immediate_and_branch_relative_long)
 UNDEF_INST(compare_immediate_and_trap)
 UNDEF_INST(compare_immediate_and_trap_long)
 UNDEF_INST(compare_logical_and_branch_long_register)
 UNDEF_INST(compare_logical_and_branch_register)
 UNDEF_INST(compare_logical_and_branch_relative_long_register)
 UNDEF_INST(compare_logical_and_branch_relative_register)
 UNDEF_INST(compare_logical_and_trap_long_register)
 UNDEF_INST(compare_logical_and_trap_register)
 UNDEF_INST(compare_logical_immediate_and_branch)
 UNDEF_INST(compare_logical_immediate_and_branch_long)
 UNDEF_INST(compare_logical_immediate_and_branch_relative)
 UNDEF_INST(compare_logical_immediate_and_branch_relative_long)
 UNDEF_INST(compare_logical_immediate_and_trap_fullword)
 UNDEF_INST(compare_logical_immediate_and_trap_long)
 UNDEF_INST(compare_logical_immediate_fullword_storage)
 UNDEF_INST(compare_logical_immediate_halfword_storage)
 UNDEF_INST(compare_logical_immediate_long_storage)
 UNDEF_INST(compare_logical_relative_long)
 UNDEF_INST(compare_logical_relative_long_halfword)
 UNDEF_INST(compare_logical_relative_long_long)
 UNDEF_INST(compare_logical_relative_long_long_fullword)
 UNDEF_INST(compare_logical_relative_long_long_halfword)
 UNDEF_INST(compare_relative_long)
 UNDEF_INST(compare_relative_long_long)
 UNDEF_INST(compare_relative_long_long_fullword)
 UNDEF_INST(extract_cache_attribute)
 UNDEF_INST(load_address_extended_y)
 UNDEF_INST(load_and_test_long_fullword)
 UNDEF_INST(load_halfword_relative_long)
 UNDEF_INST(load_halfword_relative_long_long)
 UNDEF_INST(load_logical_halfword_relative_long)
 UNDEF_INST(load_logical_halfword_relative_long_long)
 UNDEF_INST(load_logical_relative_long_long_fullword)
 UNDEF_INST(load_relative_long)
 UNDEF_INST(load_relative_long_long)
 UNDEF_INST(load_relative_long_long_fullword)
 UNDEF_INST(move_fullword_from_halfword_immediate)
 UNDEF_INST(move_halfword_from_halfword_immediate)
 UNDEF_INST(move_long_from_halfword_immediate)
 UNDEF_INST(multiply_halfword_y)
 UNDEF_INST(multiply_single_immediate_fullword)
 UNDEF_INST(multiply_single_immediate_long_fullword)
 UNDEF_INST(multiply_y)
 UNDEF_INST(prefetch_data)
 UNDEF_INST(prefetch_data_relative_long)
 UNDEF_INST(rotate_then_and_selected_bits_long_reg)
 UNDEF_INST(rotate_then_exclusive_or_selected_bits_long_reg)
 UNDEF_INST(rotate_then_insert_selected_bits_long_reg)
 UNDEF_INST(rotate_then_or_selected_bits_long_reg)
 UNDEF_INST(store_halfword_relative_long)
 UNDEF_INST(store_relative_long)
 UNDEF_INST(store_relative_long_long)
#endif /*!defined(FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY)*/ /*208*/


#if !defined(FEATURE_PARSING_ENHANCEMENT_FACILITY)              /*208*/
 UNDEF_INST(translate_and_test_extended)                        /*208*/
 UNDEF_INST(translate_and_test_reverse_extended)                /*208*/
#endif /*!defined(FEATURE_PARSING_ENHANCEMENT_FACILITY)*/       /*208*/


#if !defined(FEATURE_HIGH_WORD_FACILITY)                        /*810*/
 UNDEF_INST(add_high_high_high_register)                        /*810*/
 UNDEF_INST(add_high_high_low_register)                         /*810*/
 UNDEF_INST(add_high_immediate)                                 /*810*/
 UNDEF_INST(add_logical_high_high_high_register)                /*810*/
 UNDEF_INST(add_logical_high_high_low_register)                 /*810*/
 UNDEF_INST(add_logical_with_signed_immediate_high)             /*810*/
 UNDEF_INST(add_logical_with_signed_immediate_high_n)           /*810*/
 UNDEF_INST(branch_relative_on_count_high)                      /*810*/
 UNDEF_INST(compare_high_high_register)                         /*810*/
 UNDEF_INST(compare_high_low_register)                          /*810*/
 UNDEF_INST(compare_high_fullword)                              /*810*/
 UNDEF_INST(compare_high_immediate)                             /*810*/
 UNDEF_INST(compare_logical_high_high_register)                 /*810*/
 UNDEF_INST(compare_logical_high_low_register)                  /*810*/
 UNDEF_INST(compare_logical_high_fullword)                      /*810*/
 UNDEF_INST(compare_logical_high_immediate)                     /*810*/
 UNDEF_INST(load_byte_high)                                     /*810*/
 UNDEF_INST(load_fullword_high)                                 /*810*/
 UNDEF_INST(load_halfword_high)                                 /*810*/
 UNDEF_INST(load_logical_character_high)                        /*810*/
 UNDEF_INST(load_logical_halfword_high)                         /*810*/
 UNDEF_INST(rotate_then_insert_selected_bits_high_long_reg)     /*810*/
 UNDEF_INST(rotate_then_insert_selected_bits_low_long_reg)      /*810*/
 UNDEF_INST(store_character_high)                               /*810*/
 UNDEF_INST(store_fullword_high)                                /*810*/
 UNDEF_INST(store_halfword_high)                                /*810*/
 UNDEF_INST(subtract_high_high_high_register)                   /*810*/
 UNDEF_INST(subtract_high_high_low_register)                    /*810*/
 UNDEF_INST(subtract_logical_high_high_high_register)           /*810*/
 UNDEF_INST(subtract_logical_high_high_low_register)            /*810*/
#endif /*!defined(FEATURE_HIGH_WORD_FACILITY)*/                 /*810*/


#if !defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)               /*810*/
 UNDEF_INST(load_and_add)                                       /*810*/
 UNDEF_INST(load_and_add_logical)                               /*810*/
 UNDEF_INST(load_and_and)                                       /*810*/
 UNDEF_INST(load_and_exclusive_or)                              /*810*/
 UNDEF_INST(load_and_or)                                        /*810*/
 UNDEF_INST(load_pair_disjoint)                                 /*810*/
#if !defined(FEATURE_INTERLOCKED_ACCESS_FACILITY) || !defined(FEATURE_ESAME)
 UNDEF_INST(load_and_add_logical_long)                          /*810*/
 UNDEF_INST(load_and_add_long)                                  /*810*/
 UNDEF_INST(load_and_and_long)                                  /*810*/
 UNDEF_INST(load_and_exclusive_or_long)                         /*810*/
 UNDEF_INST(load_and_or_long)                                   /*810*/
 UNDEF_INST(load_pair_disjoint_long)                            /*810*/
#endif
#endif /*!defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)*/        /*810*/


#if !defined(FEATURE_LOAD_STORE_ON_CONDITION_FACILITY)          /*810*/
 UNDEF_INST(load_on_condition_register)                         /*810*/
 UNDEF_INST(load_on_condition_long_register)                    /*810*/
 UNDEF_INST(load_on_condition)                                  /*810*/
 UNDEF_INST(load_on_condition_long)                             /*810*/
 UNDEF_INST(store_on_condition)                                 /*810*/
 UNDEF_INST(store_on_condition_long)                            /*810*/
#endif /*!defined(FEATURE_LOAD_STORE_ON_CONDITION_FACILITY)*/   /*810*/


#if !defined(FEATURE_DISTINCT_OPERANDS_FACILITY)                /*810*/
 UNDEF_INST(add_distinct_register)                              /*810*/
 UNDEF_INST(add_distinct_long_register)                         /*810*/
 UNDEF_INST(add_distinct_halfword_immediate)                    /*810*/
 UNDEF_INST(add_distinct_long_halfword_immediate)               /*810*/
 UNDEF_INST(add_logical_distinct_register)                      /*810*/
 UNDEF_INST(add_logical_distinct_long_register)                 /*810*/
 UNDEF_INST(add_logical_distinct_signed_halfword_immediate)     /*810*/
 UNDEF_INST(add_logical_distinct_long_signed_halfword_immediate)/*810*/
 UNDEF_INST(and_distinct_register)                              /*810*/
 UNDEF_INST(and_distinct_long_register)                         /*810*/
 UNDEF_INST(exclusive_or_distinct_register)                     /*810*/
 UNDEF_INST(exclusive_or_distinct_long_register)                /*810*/
 UNDEF_INST(or_distinct_register)                               /*810*/
 UNDEF_INST(or_distinct_long_register)                          /*810*/
 UNDEF_INST(shift_left_single_distinct)                         /*810*/
 UNDEF_INST(shift_left_single_logical_distinct)                 /*810*/
 UNDEF_INST(shift_right_single_distinct)                        /*810*/
 UNDEF_INST(shift_right_single_logical_distinct)                /*810*/
 UNDEF_INST(subtract_distinct_register)                         /*810*/
 UNDEF_INST(subtract_distinct_long_register)                    /*810*/
 UNDEF_INST(subtract_logical_distinct_register)                 /*810*/
 UNDEF_INST(subtract_logical_distinct_long_register)            /*810*/
#endif /*!defined(FEATURE_DISTINCT_OPERANDS_FACILITY)*/         /*810*/


#if !defined(FEATURE_POPULATION_COUNT_FACILITY)                 /*810*/
 UNDEF_INST(population_count)                                   /*810*/
#endif /*!defined(FEATURE_POPULATION_COUNT_FACILITY)*/          /*810*/


#if !defined(FEATURE_RESET_REFERENCE_BITS_MULTIPLE_FACILITY)    /*810*/
 UNDEF_INST(reset_reference_bits_multiple)                      /*810*/
#endif /*!defined(FEATURE_RESET_REFERENCE_BITS_MULTIPLE_FACILITY)*/


#if !defined(FEATURE_VECTOR_FACILITY)
 UNDEF_INST(v_test_vmr)
 UNDEF_INST(v_complement_vmr)
 UNDEF_INST(v_count_left_zeros_in_vmr)
 UNDEF_INST(v_count_ones_in_vmr)
 UNDEF_INST(v_extract_vct)
 UNDEF_INST(v_extract_vector_modes)
 UNDEF_INST(v_restore_vr)
 UNDEF_INST(v_save_changed_vr)
 UNDEF_INST(v_save_vr)
 UNDEF_INST(v_load_vmr)
 UNDEF_INST(v_load_vmr_complement)
 UNDEF_INST(v_store_vmr)
 UNDEF_INST(v_and_to_vmr)
 UNDEF_INST(v_or_to_vmr)
 UNDEF_INST(v_exclusive_or_to_vmr)
 UNDEF_INST(v_save_vsr)
 UNDEF_INST(v_save_vmr)
 UNDEF_INST(v_restore_vsr)
 UNDEF_INST(v_restore_vmr)
 UNDEF_INST(v_load_vct_from_address)
 UNDEF_INST(v_clear_vr)
 UNDEF_INST(v_set_vector_mask_mode)
 UNDEF_INST(v_load_vix_from_address)
 UNDEF_INST(v_store_vector_parameters)
 UNDEF_INST(v_save_vac)
 UNDEF_INST(v_restore_vac)
#endif /*!defined(FEATURE_VECTOR_FACILITY)*/


#if !defined(FEATURE_HEXADECIMAL_FLOATING_POINT)
 UNDEF_INST(load_positive_float_long_reg)
 UNDEF_INST(load_negative_float_long_reg)
 UNDEF_INST(load_and_test_float_long_reg)
 UNDEF_INST(load_complement_float_long_reg)
 UNDEF_INST(halve_float_long_reg)
 UNDEF_INST(load_rounded_float_long_reg)
 UNDEF_INST(multiply_float_ext_reg)
 UNDEF_INST(multiply_float_long_to_ext_reg)
 UNDEF_INST(load_float_long_reg)
 UNDEF_INST(compare_float_long_reg)
 UNDEF_INST(add_float_long_reg)
 UNDEF_INST(subtract_float_long_reg)
 UNDEF_INST(multiply_float_long_reg)
 UNDEF_INST(divide_float_long_reg)
 UNDEF_INST(add_unnormal_float_long_reg)
 UNDEF_INST(subtract_unnormal_float_long_reg)
 UNDEF_INST(load_positive_float_short_reg)
 UNDEF_INST(load_negative_float_short_reg)
 UNDEF_INST(load_and_test_float_short_reg)
 UNDEF_INST(load_complement_float_short_reg)
 UNDEF_INST(halve_float_short_reg)
 UNDEF_INST(load_rounded_float_short_reg)
 UNDEF_INST(add_float_ext_reg)
 UNDEF_INST(subtract_float_ext_reg)
 UNDEF_INST(load_float_short_reg)
 UNDEF_INST(compare_float_short_reg)
 UNDEF_INST(add_float_short_reg)
 UNDEF_INST(subtract_float_short_reg)
 UNDEF_INST(multiply_float_short_to_long_reg)
 UNDEF_INST(divide_float_short_reg)
 UNDEF_INST(add_unnormal_float_short_reg)
 UNDEF_INST(subtract_unnormal_float_short_reg)
 UNDEF_INST(store_float_long)
 UNDEF_INST(multiply_float_long_to_ext)
 UNDEF_INST(load_float_long)
 UNDEF_INST(compare_float_long)
 UNDEF_INST(add_float_long)
 UNDEF_INST(subtract_float_long)
 UNDEF_INST(multiply_float_long)
 UNDEF_INST(divide_float_long)
 UNDEF_INST(add_unnormal_float_long)
 UNDEF_INST(subtract_unnormal_float_long)
 UNDEF_INST(store_float_short)
 UNDEF_INST(load_float_short)
 UNDEF_INST(compare_float_short)
 UNDEF_INST(add_float_short)
 UNDEF_INST(subtract_float_short)
 UNDEF_INST(multiply_float_short_to_long)
 UNDEF_INST(divide_float_short)
 UNDEF_INST(add_unnormal_float_short)
 UNDEF_INST(subtract_unnormal_float_short)
 UNDEF_INST(divide_float_ext_reg)
#endif /*!defined(FEATURE_HEXADECIMAL_FLOATING_POINT)*/


#if !defined(FEATURE_HFP_EXTENSIONS)
 UNDEF_INST(load_lengthened_float_short_to_long_reg)
 UNDEF_INST(load_lengthened_float_long_to_ext_reg)
 UNDEF_INST(load_lengthened_float_short_to_ext_reg)
 UNDEF_INST(squareroot_float_ext_reg)
 UNDEF_INST(multiply_float_short_reg)
 UNDEF_INST(load_positive_float_ext_reg)
 UNDEF_INST(load_negative_float_ext_reg)
 UNDEF_INST(load_and_test_float_ext_reg)
 UNDEF_INST(load_complement_float_ext_reg)
 UNDEF_INST(load_rounded_float_ext_to_short_reg)
 UNDEF_INST(load_fp_int_float_ext_reg)
 UNDEF_INST(compare_float_ext_reg)
 UNDEF_INST(load_fp_int_float_short_reg)
 UNDEF_INST(load_fp_int_float_long_reg)
 UNDEF_INST(convert_fixed_to_float_short_reg)
 UNDEF_INST(convert_fixed_to_float_long_reg)
 UNDEF_INST(convert_fixed_to_float_ext_reg)
 UNDEF_INST(convert_float_short_to_fixed_reg)
 UNDEF_INST(convert_float_long_to_fixed_reg)
 UNDEF_INST(convert_float_ext_to_fixed_reg)
 UNDEF_INST(load_lengthened_float_short_to_long)
 UNDEF_INST(load_lengthened_float_long_to_ext)
 UNDEF_INST(load_lengthened_float_short_to_ext)
 UNDEF_INST(squareroot_float_short)
 UNDEF_INST(squareroot_float_long)
 UNDEF_INST(multiply_float_short)
#endif /*!defined(FEATURE_HFP_EXTENSIONS)*/


#if !defined(FEATURE_FPS_EXTENSIONS)
 UNDEF_INST(convert_bfp_long_to_float_long_reg)
 UNDEF_INST(convert_bfp_short_to_float_long_reg)
 UNDEF_INST(convert_float_long_to_bfp_long_reg)
 UNDEF_INST(convert_float_long_to_bfp_short_reg)
 UNDEF_INST(load_float_ext_reg)
 UNDEF_INST(load_zero_float_ext_reg)
 UNDEF_INST(load_zero_float_long_reg)
 UNDEF_INST(load_zero_float_short_reg)
#endif /*!defined(FEATURE_FPS_EXTENSIONS)*/


#if !defined(FEATURE_FPS_ENHANCEMENT)
 UNDEF_INST(copy_sign_fpr_long_reg)
 UNDEF_INST(load_complement_fpr_long_reg)
 UNDEF_INST(load_fpr_from_gr_long_reg)
 UNDEF_INST(load_gr_from_fpr_long_reg)
 UNDEF_INST(load_negative_fpr_long_reg)
 UNDEF_INST(load_positive_fpr_long_reg)
 UNDEF_INST(set_dfp_rounding_mode)
#endif /*!defined(FEATURE_FPS_ENHANCEMENT)*/


#if !defined(FEATURE_IEEE_EXCEPTION_SIMULATION)
 UNDEF_INST(load_fpc_and_signal)
 UNDEF_INST(set_fpc_and_signal)
#endif /*!defined(FEATURE_IEEE_EXCEPTION_SIMULATION)*/


#if !defined(FEATURE_HFP_MULTIPLY_ADD_SUBTRACT)
 UNDEF_INST(multiply_add_float_short_reg)
 UNDEF_INST(multiply_add_float_long_reg)
 UNDEF_INST(multiply_add_float_short)
 UNDEF_INST(multiply_add_float_long)
 UNDEF_INST(multiply_subtract_float_short_reg)
 UNDEF_INST(multiply_subtract_float_long_reg)
 UNDEF_INST(multiply_subtract_float_short)
 UNDEF_INST(multiply_subtract_float_long)
#endif /*!defined(FEATURE_HFP_MULTIPLY_ADD_SUBTRACT)*/


#if !defined(FEATURE_HFP_UNNORMALIZED_EXTENSION)                /*@Z9*/
 UNDEF_INST(multiply_unnormal_float_long_to_ext_reg)            /*@Z9*/
 UNDEF_INST(multiply_unnormal_float_long_to_ext_low_reg)        /*@Z9*/
 UNDEF_INST(multiply_unnormal_float_long_to_ext_high_reg)       /*@Z9*/
 UNDEF_INST(multiply_add_unnormal_float_long_to_ext_reg)        /*@Z9*/
 UNDEF_INST(multiply_add_unnormal_float_long_to_ext_low_reg)    /*@Z9*/
 UNDEF_INST(multiply_add_unnormal_float_long_to_ext_high_reg)   /*@Z9*/
 UNDEF_INST(multiply_unnormal_float_long_to_ext)                /*@Z9*/
 UNDEF_INST(multiply_unnormal_float_long_to_ext_low)            /*@Z9*/
 UNDEF_INST(multiply_unnormal_float_long_to_ext_high)           /*@Z9*/
 UNDEF_INST(multiply_add_unnormal_float_long_to_ext)            /*@Z9*/
 UNDEF_INST(multiply_add_unnormal_float_long_to_ext_low)        /*@Z9*/
 UNDEF_INST(multiply_add_unnormal_float_long_to_ext_high)       /*@Z9*/
#endif /*!defined(FEATURE_HFP_UNNORMALIZED_EXTENSION)*/         /*@Z9*/


#if !defined(FEATURE_BINARY_FLOATING_POINT)
 UNDEF_INST(store_fpc)
 UNDEF_INST(load_fpc)
 UNDEF_INST(set_fpc)
 UNDEF_INST(extract_fpc)
 UNDEF_INST(set_bfp_rounding_mode_2bit)
#endif /*!defined(FEATURE_BINARY_FLOATING_POINT)*/


#if !defined(FEATURE_BINARY_FLOATING_POINT) || defined(NO_IEEE_SUPPORT)
 UNDEF_INST(add_bfp_ext_reg)
 UNDEF_INST(add_bfp_long)
 UNDEF_INST(add_bfp_long_reg)
 UNDEF_INST(add_bfp_short)
 UNDEF_INST(add_bfp_short_reg)
 UNDEF_INST(compare_and_signal_bfp_ext_reg)
 UNDEF_INST(compare_and_signal_bfp_long)
 UNDEF_INST(compare_and_signal_bfp_long_reg)
 UNDEF_INST(compare_and_signal_bfp_short)
 UNDEF_INST(compare_and_signal_bfp_short_reg)
 UNDEF_INST(compare_bfp_ext_reg)
 UNDEF_INST(compare_bfp_long)
 UNDEF_INST(compare_bfp_long_reg)
 UNDEF_INST(compare_bfp_short)
 UNDEF_INST(compare_bfp_short_reg)
 UNDEF_INST(convert_bfp_ext_to_fix32_reg)
 UNDEF_INST(convert_bfp_long_to_fix32_reg)
 UNDEF_INST(convert_bfp_short_to_fix32_reg)
 UNDEF_INST(convert_fix32_to_bfp_ext_reg)
 UNDEF_INST(convert_fix32_to_bfp_long_reg)
 UNDEF_INST(convert_fix32_to_bfp_short_reg)
 UNDEF_INST(convert_fix64_to_bfp_ext_reg)
 UNDEF_INST(convert_fix64_to_bfp_long_reg)
 UNDEF_INST(convert_fix64_to_bfp_short_reg)
 UNDEF_INST(convert_bfp_ext_to_fix64_reg)
 UNDEF_INST(convert_bfp_long_to_fix64_reg)
 UNDEF_INST(convert_bfp_short_to_fix64_reg)
 UNDEF_INST(divide_bfp_ext_reg)
 UNDEF_INST(divide_bfp_long)
 UNDEF_INST(divide_bfp_long_reg)
 UNDEF_INST(divide_bfp_short)
 UNDEF_INST(divide_bfp_short_reg)
 UNDEF_INST(divide_integer_bfp_long_reg)
 UNDEF_INST(divide_integer_bfp_short_reg)
 UNDEF_INST(load_and_test_bfp_ext_reg)
 UNDEF_INST(load_and_test_bfp_long_reg)
 UNDEF_INST(load_and_test_bfp_short_reg)
 UNDEF_INST(load_fp_int_bfp_ext_reg)
 UNDEF_INST(load_fp_int_bfp_long_reg)
 UNDEF_INST(load_fp_int_bfp_short_reg)
 UNDEF_INST(load_complement_bfp_ext_reg)
 UNDEF_INST(load_complement_bfp_long_reg)
 UNDEF_INST(load_complement_bfp_short_reg)
 UNDEF_INST(load_negative_bfp_ext_reg)
 UNDEF_INST(load_negative_bfp_long_reg)
 UNDEF_INST(load_negative_bfp_short_reg)
 UNDEF_INST(load_positive_bfp_ext_reg)
 UNDEF_INST(load_positive_bfp_long_reg)
 UNDEF_INST(load_positive_bfp_short_reg)
 UNDEF_INST(load_lengthened_bfp_short_to_long)
 UNDEF_INST(load_lengthened_bfp_short_to_long_reg)
 UNDEF_INST(load_lengthened_bfp_long_to_ext)
 UNDEF_INST(load_lengthened_bfp_long_to_ext_reg)
 UNDEF_INST(load_lengthened_bfp_short_to_ext)
 UNDEF_INST(load_lengthened_bfp_short_to_ext_reg)
 UNDEF_INST(load_rounded_bfp_long_to_short_reg)
 UNDEF_INST(load_rounded_bfp_ext_to_long_reg)
 UNDEF_INST(load_rounded_bfp_ext_to_short_reg)
 UNDEF_INST(multiply_bfp_ext_reg)
 UNDEF_INST(multiply_bfp_long_to_ext_reg)
 UNDEF_INST(multiply_bfp_long_to_ext)
 UNDEF_INST(multiply_bfp_long)
 UNDEF_INST(multiply_bfp_long_reg)
 UNDEF_INST(multiply_bfp_short_to_long_reg)
 UNDEF_INST(multiply_bfp_short_to_long)
 UNDEF_INST(multiply_bfp_short)
 UNDEF_INST(multiply_bfp_short_reg)
 UNDEF_INST(multiply_add_bfp_long_reg)
 UNDEF_INST(multiply_add_bfp_long)
 UNDEF_INST(multiply_add_bfp_short_reg)
 UNDEF_INST(multiply_add_bfp_short)
 UNDEF_INST(multiply_subtract_bfp_long_reg)
 UNDEF_INST(multiply_subtract_bfp_long)
 UNDEF_INST(multiply_subtract_bfp_short_reg)
 UNDEF_INST(multiply_subtract_bfp_short)
 UNDEF_INST(squareroot_bfp_ext_reg)
 UNDEF_INST(squareroot_bfp_long)
 UNDEF_INST(squareroot_bfp_long_reg)
 UNDEF_INST(squareroot_bfp_short)
 UNDEF_INST(squareroot_bfp_short_reg)
 UNDEF_INST(subtract_bfp_ext_reg)
 UNDEF_INST(subtract_bfp_long)
 UNDEF_INST(subtract_bfp_long_reg)
 UNDEF_INST(subtract_bfp_short)
 UNDEF_INST(subtract_bfp_short_reg)
 UNDEF_INST(test_data_class_bfp_short)
 UNDEF_INST(test_data_class_bfp_long)
 UNDEF_INST(test_data_class_bfp_ext)
#endif /*!defined(FEATURE_BINARY_FLOATING_POINT)*/


#if !defined(FEATURE_DECIMAL_FLOATING_POINT)
 UNDEF_INST(add_dfp_ext_reg)
 UNDEF_INST(add_dfp_long_reg)
 UNDEF_INST(compare_dfp_ext_reg)
 UNDEF_INST(compare_dfp_long_reg)
 UNDEF_INST(compare_and_signal_dfp_ext_reg)
 UNDEF_INST(compare_and_signal_dfp_long_reg)
 UNDEF_INST(compare_exponent_dfp_ext_reg)
 UNDEF_INST(compare_exponent_dfp_long_reg)
 UNDEF_INST(convert_fix64_to_dfp_ext_reg)
 UNDEF_INST(convert_fix64_to_dfp_long_reg)
 UNDEF_INST(convert_sbcd128_to_dfp_ext_reg)
 UNDEF_INST(convert_sbcd64_to_dfp_long_reg)
 UNDEF_INST(convert_ubcd128_to_dfp_ext_reg)
 UNDEF_INST(convert_ubcd64_to_dfp_long_reg)
 UNDEF_INST(convert_dfp_ext_to_fix64_reg)
 UNDEF_INST(convert_dfp_long_to_fix64_reg)
 UNDEF_INST(convert_dfp_ext_to_sbcd128_reg)
 UNDEF_INST(convert_dfp_long_to_sbcd64_reg)
 UNDEF_INST(convert_dfp_ext_to_ubcd128_reg)
 UNDEF_INST(convert_dfp_long_to_ubcd64_reg)
 UNDEF_INST(divide_dfp_ext_reg)
 UNDEF_INST(divide_dfp_long_reg)
 UNDEF_INST(extract_biased_exponent_dfp_ext_to_fix64_reg)
 UNDEF_INST(extract_biased_exponent_dfp_long_to_fix64_reg)
 UNDEF_INST(extract_significance_dfp_ext_reg)
 UNDEF_INST(extract_significance_dfp_long_reg)
 UNDEF_INST(insert_biased_exponent_fix64_to_dfp_ext_reg)
 UNDEF_INST(insert_biased_exponent_fix64_to_dfp_long_reg)
 UNDEF_INST(load_and_test_dfp_ext_reg)
 UNDEF_INST(load_and_test_dfp_long_reg)
 UNDEF_INST(load_fp_int_dfp_ext_reg)
 UNDEF_INST(load_fp_int_dfp_long_reg)
 UNDEF_INST(load_lengthened_dfp_long_to_ext_reg)
 UNDEF_INST(load_lengthened_dfp_short_to_long_reg)
 UNDEF_INST(load_rounded_dfp_ext_to_long_reg)
 UNDEF_INST(load_rounded_dfp_long_to_short_reg)
 UNDEF_INST(multiply_dfp_ext_reg)
 UNDEF_INST(multiply_dfp_long_reg)
 UNDEF_INST(quantize_dfp_ext_reg)
 UNDEF_INST(quantize_dfp_long_reg)
 UNDEF_INST(reround_dfp_ext_reg)
 UNDEF_INST(reround_dfp_long_reg)
 UNDEF_INST(shift_coefficient_left_dfp_ext)
 UNDEF_INST(shift_coefficient_left_dfp_long)
 UNDEF_INST(shift_coefficient_right_dfp_ext)
 UNDEF_INST(shift_coefficient_right_dfp_long)
 UNDEF_INST(subtract_dfp_ext_reg)
 UNDEF_INST(subtract_dfp_long_reg)
 UNDEF_INST(test_data_class_dfp_ext)
 UNDEF_INST(test_data_class_dfp_long)
 UNDEF_INST(test_data_class_dfp_short)
 UNDEF_INST(test_data_group_dfp_ext)
 UNDEF_INST(test_data_group_dfp_long)
 UNDEF_INST(test_data_group_dfp_short)
#endif /*!defined(FEATURE_DECIMAL_FLOATING_POINT)*/


#if !defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)         /*810*/
 UNDEF_INST(convert_bfp_short_to_u32_reg)                       /*810*/
 UNDEF_INST(convert_bfp_long_to_u32_reg)                        /*810*/
 UNDEF_INST(convert_bfp_ext_to_u32_reg)                         /*810*/
 UNDEF_INST(convert_bfp_short_to_u64_reg)                       /*810*/
 UNDEF_INST(convert_bfp_long_to_u64_reg)                        /*810*/
 UNDEF_INST(convert_bfp_ext_to_u64_reg)                         /*810*/
 UNDEF_INST(convert_u32_to_bfp_short_reg)                       /*810*/
 UNDEF_INST(convert_u32_to_bfp_long_reg)                        /*810*/
 UNDEF_INST(convert_u32_to_bfp_ext_reg)                         /*810*/
 UNDEF_INST(convert_u64_to_bfp_short_reg)                       /*810*/
 UNDEF_INST(convert_u64_to_bfp_long_reg)                        /*810*/
 UNDEF_INST(convert_u64_to_bfp_ext_reg)                         /*810*/
 UNDEF_INST(convert_dfp_long_to_fix32_reg)                      /*810*/
 UNDEF_INST(convert_dfp_long_to_u32_reg)                        /*810*/
 UNDEF_INST(convert_dfp_long_to_u64_reg)                        /*810*/
 UNDEF_INST(convert_dfp_ext_to_fix32_reg)                       /*810*/
 UNDEF_INST(convert_dfp_ext_to_u32_reg)                         /*810*/
 UNDEF_INST(convert_dfp_ext_to_u64_reg)                         /*810*/
 UNDEF_INST(convert_fix32_to_dfp_long_reg)                      /*810*/
 UNDEF_INST(convert_fix32_to_dfp_ext_reg)                       /*810*/
 UNDEF_INST(convert_u32_to_dfp_long_reg)                        /*810*/
 UNDEF_INST(convert_u32_to_dfp_ext_reg)                         /*810*/
 UNDEF_INST(convert_u64_to_dfp_long_reg)                        /*810*/
 UNDEF_INST(convert_u64_to_dfp_ext_reg)                         /*810*/
 UNDEF_INST(set_bfp_rounding_mode_3bit)                         /*810*/
#endif /*!defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)*/  /*810*/


#if !defined(FEATURE_PFPO)
 UNDEF_INST(perform_floating_point_operation)
#endif /*!defined(FEATURE_PFPO)*/


#if !defined(FEATURE_EMULATE_VM)
 UNDEF_INST(inter_user_communication_vehicle)
#endif /*!defined(FEATURE_EMULATE_VM)*/


#if !defined(FEATURE_RESUME_PROGRAM)
 UNDEF_INST(resume_program)
#endif /*!defined(FEATURE_RESUME_PROGRAM)*/


#if !defined(FEATURE_COMPRESSION)
 UNDEF_INST(cmpsc_2012)
#endif /*!defined(FEATURE_COMPRESSION)*/


#if !defined(FEATURE_LOCK_PAGE)
 UNDEF_INST(lock_page)
#endif /*!defined(FEATURE_LOCK_PAGE)*/


#if !defined(FEATURE_SQUARE_ROOT)
 UNDEF_INST(squareroot_float_long_reg)
 UNDEF_INST(squareroot_float_short_reg)
#endif /*!defined(FEATURE_SQUARE_ROOT)*/


#if !defined(FEATURE_INTERPRETIVE_EXECUTION)
 UNDEF_INST(start_interpretive_execution)
#endif /*!defined(FEATURE_INTERPRETIVE_EXECUTION)*/


#if !defined(FEATURE_REGION_RELOCATE)
 UNDEF_INST(store_zone_parameter)
 UNDEF_INST(set_zone_parameter)
#endif /*!defined(FEATURE_REGION_RELOCATE)*/


#if !defined(FEATURE_IO_ASSIST)
 UNDEF_INST(test_pending_zone_interrupt)
#endif /*!defined(FEATURE_IO_ASSIST)*/


#if !defined(FEATURE_QUEUED_DIRECT_IO)
 UNDEF_INST(signal_adapter)
#endif /*!defined(FEATURE_QUEUED_DIRECT_IO)*/


#if !defined(FEATURE_QEBSM)
 UNDEF_INST(set_queue_buffer_state)
 UNDEF_INST(extract_queue_buffer_state)
#endif /*!defined(FEATURE_QEBSM)*/


#if !defined(FEATURE_SVS)
 UNDEF_INST(set_vector_summary)
#endif /*!defined(FEATURE_SVS)*/


#if !defined(FEATURE_CHANNEL_SWITCHING)
 UNDEF_INST(connect_channel_set)
 UNDEF_INST(disconnect_channel_set)
#endif /*!defined(FEATURE_CHANNEL_SWITCHING)*/


#if !defined(FEATURE_LOAD_PROGRAM_PARAMETER_FACILITY)
 UNDEF_INST(load_program_parameter)
#endif /* !defined(FEATURE_LOAD_PROGRAM_PARAMETER_FACILITY) */


#if !defined(FEATURE_CPU_MEASUREMENT_COUNTER_FACILITY)
 UNDEF_INST(extract_coprocessor_group_address)
 UNDEF_INST(extract_cpu_counter)
 UNDEF_INST(extract_peripheral_counter)
 UNDEF_INST(load_cpu_counter_set_controls)
 UNDEF_INST(load_peripheral_counter_set_controls)
 UNDEF_INST(query_counter_information)
 UNDEF_INST(set_cpu_counter)
 UNDEF_INST(set_peripheral_counter)
#endif /*!defined(FEATURE_CPU_MEASUREMENT_COUNTER_FACILITY)*/


#if !defined(FEATURE_CPU_MEASUREMENT_SAMPLING_FACILITY)
 UNDEF_INST(load_sampling_controls)
 UNDEF_INST(query_sampling_information)
#endif /*!defined(FEATURE_CPU_MEASUREMENT_SAMPLING_FACILITY)*/

#if !defined(FEATURE_STORE_CPU_MULTIPLE_COUNTER_FACILITY)
 UNDEF_INST(store_cpu_counter_multiple)
#endif /* !defined(FEATURE_STORE_CPU_MULTIPLE_COUNTER_FACILITY) */


#if !defined(FEATURE_EXTENDED_TRANSLATION)
 UNDEF_INST(translate_extended)
 UNDEF_INST(convert_utf16_to_utf8)
 UNDEF_INST(convert_utf8_to_utf16)
#endif /*!defined(FEATURE_EXTENDED_TRANSLATION)*/


#if !defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
 UNDEF_INST(compare_logical_long_unicode)
 UNDEF_INST(move_long_unicode)
 UNDEF_INST(pack_ascii)
 UNDEF_INST(pack_unicode)
 UNDEF_INST(test_decimal)
 UNDEF_INST(translate_one_to_one)
 UNDEF_INST(translate_one_to_two)
 UNDEF_INST(translate_two_to_one)
 UNDEF_INST(translate_two_to_two)
 UNDEF_INST(unpack_ascii)
 UNDEF_INST(unpack_unicode)
#endif /*!defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)*/


#if !defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_3)
 UNDEF_INST(convert_utf16_to_utf32)
 UNDEF_INST(convert_utf32_to_utf16)
 UNDEF_INST(convert_utf32_to_utf8)
 UNDEF_INST(convert_utf8_to_utf32)
 UNDEF_INST(search_string_unicode)
 UNDEF_INST(translate_and_test_reverse)
#endif /*!defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_3)*/


#if !defined(FEATURE_LOAD_REVERSED) && !defined(FEATURE_ESAME_N3_ESA390)
 UNDEF_INST(load_reversed_register)
 UNDEF_INST(load_reversed)
 UNDEF_INST(load_reversed_half)
 UNDEF_INST(store_reversed)
 UNDEF_INST(store_reversed_half)
#if !defined(FEATURE_ESAME)
 UNDEF_INST(load_reversed_long_register)
 UNDEF_INST(store_reversed_long)
#endif /*!defined(FEATURE_ESAME)*/
#endif /*!defined(FEATURE_LOAD_REVERSED) && !defined(FEATURE_ESAME_N3_ESA390)*/


#if !defined(FEATURE_SERVICE_PROCESSOR)
 UNDEF_INST(service_call)
#endif /*!defined(FEATURE_SERVICE_PROCESSOR)*/


#if !defined(FEATURE_CHSC)
 UNDEF_INST(channel_subsystem_call)
#endif /*!defined(FEATURE_CHSC)*/


#if !defined(FEATURE_ESAME_N3_ESA390) && !defined(FEATURE_ESAME)
 UNDEF_INST(add_logical_carry)
 UNDEF_INST(add_logical_carry_register)
 UNDEF_INST(branch_relative_and_save_long)
 UNDEF_INST(branch_relative_on_condition_long)
 UNDEF_INST(divide_logical)
 UNDEF_INST(divide_logical_register)
 UNDEF_INST(extract_psw)
 UNDEF_INST(load_address_relative_long)
 UNDEF_INST(multiply_logical)
 UNDEF_INST(multiply_logical_register)
 UNDEF_INST(rotate_left_single_logical)
 UNDEF_INST(set_addressing_mode_24)
 UNDEF_INST(set_addressing_mode_31)
 UNDEF_INST(subtract_logical_borrow)
 UNDEF_INST(subtract_logical_borrow_register)
 UNDEF_INST(test_addressing_mode)
#endif /*!defined(FEATURE_ESAME_N3_ESA390) && !defined(FEATURE_ESAME)*/


#if !defined(FEATURE_STORE_FACILITY_LIST)
 UNDEF_INST(store_facility_list)
#endif /*!defined(FEATURE_STORE_FACILITY_LIST) */


#if !defined(FEATURE_CANCEL_IO_FACILITY)
 UNDEF_INST(cancel_subchannel)
#endif /*!defined(FEATURE_CANCEL_IO_FACILITY)*/


#if !defined(FEATURE_ECPSVM)
 UNDEF_INST(ecpsvm_basic_freex)
 UNDEF_INST(ecpsvm_basic_fretx)
 UNDEF_INST(ecpsvm_lock_page)
 UNDEF_INST(ecpsvm_unlock_page)
 UNDEF_INST(ecpsvm_decode_next_ccw)
 UNDEF_INST(ecpsvm_free_ccwstor)
 UNDEF_INST(ecpsvm_locate_vblock)
 UNDEF_INST(ecpsvm_disp1)
 UNDEF_INST(ecpsvm_tpage)
 UNDEF_INST(ecpsvm_tpage_lock)
 UNDEF_INST(ecpsvm_inval_segtab)
 UNDEF_INST(ecpsvm_inval_ptable)
 UNDEF_INST(ecpsvm_decode_first_ccw)
 UNDEF_INST(ecpsvm_dispatch_main)
 UNDEF_INST(ecpsvm_locate_rblock)
 UNDEF_INST(ecpsvm_comm_ccwproc)
 UNDEF_INST(ecpsvm_unxlate_ccw)
 UNDEF_INST(ecpsvm_disp2)
 UNDEF_INST(ecpsvm_store_level)
 UNDEF_INST(ecpsvm_loc_chgshrpg)
 UNDEF_INST(ecpsvm_extended_freex)
 UNDEF_INST(ecpsvm_extended_fretx)
 UNDEF_INST(ecpsvm_prefmach_assist)
#endif /*!defined(FEATURE_ECPSVM)*/


#if !defined(FEATURE_LONG_DISPLACEMENT)
 UNDEF_INST(add_y)
 UNDEF_INST(add_halfword_y)
 UNDEF_INST(add_logical_y)
 UNDEF_INST(and_immediate_y)
 UNDEF_INST(and_y)
 UNDEF_INST(compare_y)
 UNDEF_INST(compare_and_swap_y)
 UNDEF_INST(compare_double_and_swap_y)
 UNDEF_INST(compare_halfword_y)
 UNDEF_INST(compare_logical_y)
 UNDEF_INST(compare_logical_immediate_y)
 UNDEF_INST(compare_logical_characters_under_mask_y)
 UNDEF_INST(convert_to_binary_y)
 UNDEF_INST(convert_to_decimal_y)
 UNDEF_INST(exclusive_or_immediate_y)
 UNDEF_INST(exclusive_or_y)
 UNDEF_INST(insert_character_y)
 UNDEF_INST(insert_characters_under_mask_y)
 UNDEF_INST(load_y)
 UNDEF_INST(load_address_y)
 UNDEF_INST(load_byte)
 UNDEF_INST(load_byte_long)
 UNDEF_INST(load_halfword_y)
 UNDEF_INST(load_multiple_y)
 UNDEF_INST(load_real_address_y)
 UNDEF_INST(move_immediate_y)
 UNDEF_INST(multiply_single_y)
 UNDEF_INST(or_immediate_y)
 UNDEF_INST(or_y)
 UNDEF_INST(store_y)
 UNDEF_INST(store_character_y)
 UNDEF_INST(store_characters_under_mask_y)
 UNDEF_INST(store_halfword_y)
 UNDEF_INST(store_multiple_y)
 UNDEF_INST(subtract_y)
 UNDEF_INST(subtract_halfword_y)
 UNDEF_INST(subtract_logical_y)
 UNDEF_INST(test_under_mask_y)
#endif /*!defined(FEATURE_LONG_DISPLACEMENT)*/


#if !defined(FEATURE_LONG_DISPLACEMENT) \
 || !defined(FEATURE_ACCESS_REGISTERS)
 UNDEF_INST(load_access_multiple_y)
 UNDEF_INST(store_access_multiple_y)
#endif /*!defined(FEATURE_LONG_DISPLACEMENT)
 || !defined(FEATURE_ACCESS_REGISTERS)*/


#if !defined(FEATURE_LONG_DISPLACEMENT) \
 || !defined(FEATURE_HEXADECIMAL_FLOATING_POINT)
 UNDEF_INST(load_float_long_y)
 UNDEF_INST(load_float_short_y)
 UNDEF_INST(store_float_long_y)
 UNDEF_INST(store_float_short_y)
#endif /*!defined(FEATURE_LONG_DISPLACEMENT)
 || !defined(FEATURE_HEXADECIMAL_FLOATING_POINT)*/


#if !defined(FEATURE_MESSAGE_SECURITY_ASSIST) || !defined(OPTION_STATIC_CRYPTO)
 UNDEF_INST(cipher_message)
 UNDEF_INST(cipher_message_with_chaining)
 UNDEF_INST(compute_intermediate_message_digest)
 UNDEF_INST(compute_last_message_digest)
 UNDEF_INST(compute_message_authentication_code)
#endif /*!defined(FEATURE_MESSAGE_SECURITY_ASSIST)*/


#if !defined(FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_3) || !defined(OPTION_STATIC_CRYPTO)      /*810*/
 UNDEF_INST(perform_cryptographic_key_management_operation)     /*810*/
#endif /*!defined(FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_3)*/


#if !defined(FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_4) || !defined(OPTION_STATIC_CRYPTO)      /*810*/
 UNDEF_INST(perform_cryptographic_computation)                  /*810*/
 UNDEF_INST(cipher_message_with_cipher_feedback)                /*810*/
 UNDEF_INST(cipher_message_with_output_feedback)                /*810*/
 UNDEF_INST(cipher_message_with_counter)                        /*810*/
#endif /*!defined(FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_4)*/


#if !defined(FEATURE_DAT_ENHANCEMENT)
 UNDEF_INST(compare_and_swap_and_purge_long)
 UNDEF_INST(invalidate_dat_table_entry)
#endif /*!defined(FEATURE_DAT_ENHANCEMENT)*/


#if !defined(FEATURE_EXTENDED_IMMEDIATE)                        /*@Z9*/
 UNDEF_INST(add_fullword_immediate)                             /*@Z9*/
 UNDEF_INST(add_long_fullword_immediate)                        /*@Z9*/
 UNDEF_INST(add_logical_fullword_immediate)                     /*@Z9*/
 UNDEF_INST(add_logical_long_fullword_immediate)                /*@Z9*/
 UNDEF_INST(and_immediate_high_fullword)                        /*@Z9*/
 UNDEF_INST(and_immediate_low_fullword)                         /*@Z9*/
 UNDEF_INST(compare_fullword_immediate)                         /*@Z9*/
 UNDEF_INST(compare_long_fullword_immediate)                    /*@Z9*/
 UNDEF_INST(compare_logical_fullword_immediate)                 /*@Z9*/
 UNDEF_INST(compare_logical_long_fullword_immediate)            /*@Z9*/
 UNDEF_INST(exclusive_or_immediate_high_fullword)               /*@Z9*/
 UNDEF_INST(exclusive_or_immediate_low_fullword)                /*@Z9*/
 UNDEF_INST(insert_immediate_high_fullword)                     /*@Z9*/
 UNDEF_INST(insert_immediate_low_fullword)                      /*@Z9*/
 UNDEF_INST(load_long_fullword_immediate)                       /*@Z9*/
 UNDEF_INST(load_logical_immediate_high_fullword)               /*@Z9*/
 UNDEF_INST(load_logical_immediate_low_fullword)                /*@Z9*/
 UNDEF_INST(or_immediate_high_fullword)                         /*@Z9*/
 UNDEF_INST(or_immediate_low_fullword)                          /*@Z9*/
 UNDEF_INST(subtract_logical_fullword_immediate)                /*@Z9*/
 UNDEF_INST(subtract_logical_long_fullword_immediate)           /*@Z9*/
#endif /*!defined(FEATURE_EXTENDED_IMMEDIATE)*/                 /*@Z9*/


#if !defined(FEATURE_EXTENDED_IMMEDIATE)                        /*@Z9*/
 UNDEF_INST(load_and_test)                                      /*@Z9*/
 UNDEF_INST(load_and_test_long)                                 /*@Z9*/
 UNDEF_INST(load_byte_register)                                 /*@Z9*/
 UNDEF_INST(load_long_byte_register)                            /*@Z9*/
 UNDEF_INST(load_halfword_register)                             /*@Z9*/
 UNDEF_INST(load_long_halfword_register)                        /*@Z9*/
 UNDEF_INST(load_logical_character)                             /*@Z9*/
 UNDEF_INST(load_logical_character_register)                    /*@Z9*/
 UNDEF_INST(load_logical_long_character_register)               /*@Z9*/
 UNDEF_INST(load_logical_halfword)                              /*@Z9*/
 UNDEF_INST(load_logical_halfword_register)                     /*@Z9*/
 UNDEF_INST(load_logical_long_halfword_register)                /*@Z9*/
 UNDEF_INST(find_leftmost_one_long_register)                    /*@Z9*/
#endif /*!defined(FEATURE_EXTENDED_IMMEDIATE)*/                 /*@Z9*/


#if !defined(FEATURE_DAT_ENHANCEMENT_FACILITY_2)                /*@Z9*/
 UNDEF_INST(load_page_table_entry_address)                      /*@Z9*/
#endif /*!defined(FEATURE_DAT_ENHANCEMENT_FACILITY_2)*/         /*@Z9*/


#if !defined(FEATURE_STORE_CLOCK_FAST)
 UNDEF_INST(store_clock_fast)
#else /*!defined(FEATURE_STORE_CLOCK_FAST)*/
 #define z900_store_clock_fast z900_store_clock
#endif /*!defined(FEATURE_STORE_CLOCK_FAST)*/


#if !defined(FEATURE_STORE_FACILITY_LIST_EXTENDED)              /*@Z9*/
 UNDEF_INST(store_facility_list_extended)                       /*@Z9*/
#endif /*!defined(FEATURE_STORE_FACILITY_LIST_EXTENDED)*/       /*@Z9*/


/* The following execute_xxxx routines can be optimized by the
   compiler to an indexed jump, leaving the stack frame untouched
   as the called routine has the same arguments, and the routine
   exits immediately after the call.                             *JJ */


DEF_INST(execute_opcode_e3________xx)
{
  regs->ARCH_DEP(runtime_opcode_e3________xx)[inst[5]](inst, regs);
}

#ifdef OPTION_OPTINST
DEF_INST(E3_0)
{
  regs->ARCH_DEP(runtime_opcode_e3_0______xx)[inst[5]](inst, regs);
}
#endif /* #ifdef OPTION_OPTINST */

DEF_INST(execute_opcode_eb________xx)
{
  regs->ARCH_DEP(runtime_opcode_eb________xx)[inst[5]](inst, regs);
}


DEF_INST(execute_opcode_ec________xx)
{
  regs->ARCH_DEP(runtime_opcode_ec________xx)[inst[5]](inst, regs);
}


DEF_INST(execute_opcode_ed________xx)
{
  regs->ARCH_DEP(runtime_opcode_ed________xx)[inst[5]](inst, regs);
}


DEF_INST(operation_exception)
{
    INST_UPDATE_PSW (regs, ILC(inst[0]), ILC(inst[0]));

#if defined(MODEL_DEPENDENT)
#if defined(_FEATURE_SIE)
    /* The B2XX extended opcodes which are not defined are always
       intercepted by SIE when issued in supervisor state */
    if(!PROBSTATE(&regs->psw) && inst[0] == 0xB2)
        SIE_INTERCEPT(regs);
#endif /*defined(_FEATURE_SIE)*/
#endif /*defined(MODEL_DEPENDENT)*/

    ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);
}


DEF_INST(dummy_instruction)
{
//  logmsg(_("Dummy instruction: ")); ARCH_DEP(display_inst) (regs, inst);
    INST_UPDATE_PSW (regs, ILC(inst[0]), ILC(inst[0]));
}

#if !defined(_GEN_ARCH)


static zz_func opcode_table[0x100][GEN_MAXARCH];
static zz_func opcode_01xx[0x100][GEN_MAXARCH];
static zz_func opcode_a5_x[0x10][GEN_MAXARCH];
static zz_func opcode_a7_x[0x10][GEN_MAXARCH];
static zz_func opcode_b2xx[0x100][GEN_MAXARCH];
static zz_func opcode_b3xx[0x100][GEN_MAXARCH];
static zz_func opcode_b9xx[0x100][GEN_MAXARCH];
static zz_func opcode_c0_x[0x10][GEN_MAXARCH];
static zz_func opcode_c2_x[0x10][GEN_MAXARCH];                      /*@Z9*/
static zz_func opcode_c4_x[0x10][GEN_MAXARCH];                      /*208*/
static zz_func opcode_c6_x[0x10][GEN_MAXARCH];                      /*208*/
static zz_func opcode_c8_x[0x10][GEN_MAXARCH];
static zz_func opcode_cc_x[0x10][GEN_MAXARCH];                      /*810*/
static zz_func opcode_e3xx[0x100][GEN_MAXARCH];
static zz_func opcode_e5xx[0x100][GEN_MAXARCH];
static zz_func opcode_e6xx[0x100][GEN_MAXARCH];
static zz_func opcode_ebxx[0x100][GEN_MAXARCH];
static zz_func opcode_ecxx[0x100][GEN_MAXARCH];
static zz_func opcode_edxx[0x100][GEN_MAXARCH];
static zz_func v_opcode_a4xx[0x100][GEN_MAXARCH];
static zz_func v_opcode_a5xx[0x100][GEN_MAXARCH];
static zz_func v_opcode_a6xx[0x100][GEN_MAXARCH];
static zz_func v_opcode_e4xx[0x100][GEN_MAXARCH];

#ifdef OPTION_OPTINST
static zz_func opcode_15__[0x100][GEN_MAXARCH];
static zz_func opcode_18__[0x100][GEN_MAXARCH];
static zz_func opcode_1E__[0x100][GEN_MAXARCH];
static zz_func opcode_1F__[0x100][GEN_MAXARCH];
static zz_func opcode_41_0[0x10][GEN_MAXARCH];
static zz_func opcode_47_0[0x10][GEN_MAXARCH];
static zz_func opcode_50_0[0x10][GEN_MAXARCH];
static zz_func opcode_55_0[0x10][GEN_MAXARCH];
static zz_func opcode_58_0[0x10][GEN_MAXARCH];
static zz_func opcode_91xx[0x08][GEN_MAXARCH];
static zz_func opcode_A7_4[0x10][GEN_MAXARCH];
static zz_func opcode_BF_x[0x03][GEN_MAXARCH];
static zz_func opcode_D20x[0x01][GEN_MAXARCH];
static zz_func opcode_D50x[0x04][GEN_MAXARCH];
static zz_func opcode_E3_0[0x01][GEN_MAXARCH];
static zz_func opcode_E3_0______04[0x01][GEN_MAXARCH];
static zz_func opcode_E3_0______24[0x01][GEN_MAXARCH];
#endif /* OPTION_OPTINST */

#define DISASM_ROUTE(_table,_route) \
int disasm_ ## _table (BYTE inst[], char unused[], char *p) \
{ \
func disasm_fn; \
char* mnemonic; \
    UNREFERENCED(unused); \
    mnemonic = (void*)opcode_ ## _table [inst _route ][GEN_MAXARCH-1]; \
    disasm_fn = (void*)opcode_ ## _table [inst _route ][GEN_MAXARCH-2]; \
    return disasm_fn(inst, mnemonic, p); \
}


DISASM_ROUTE(table,[0])
DISASM_ROUTE(01xx,[1])
DISASM_ROUTE(a5_x,[1] & 0x0F)
DISASM_ROUTE(a7_x,[1] & 0x0F)
DISASM_ROUTE(b2xx,[1])
DISASM_ROUTE(b3xx,[1])
DISASM_ROUTE(b9xx,[1])
DISASM_ROUTE(c0_x,[1] & 0x0F)
DISASM_ROUTE(c2_x,[1] & 0x0F)                                   /*@Z9*/
DISASM_ROUTE(c4_x,[1] & 0x0F)                                   /*208*/
DISASM_ROUTE(c6_x,[1] & 0x0F)                                   /*208*/
DISASM_ROUTE(c8_x,[1] & 0x0F)
DISASM_ROUTE(cc_x,[1] & 0x0F)                                   /*810*/
DISASM_ROUTE(e3xx,[5])
DISASM_ROUTE(e5xx,[1])
DISASM_ROUTE(e6xx,[1])
DISASM_ROUTE(ebxx,[5])
DISASM_ROUTE(ecxx,[5])
DISASM_ROUTE(edxx,[5])


#if defined(FEATURE_VECTOR_FACILITY)
 #define opcode_a4xx v_opcode_a4xx
 DISASM_ROUTE(a4xx,[1])
 #undef opcode_a4xx
 #define opcode_a6xx v_opcode_a6xx
 DISASM_ROUTE(a6xx,[1])
 #undef opcode_a6xx
 #define opcode_e4xx v_opcode_e4xx
 DISASM_ROUTE(e4xx,[1])
 #undef opcode_e4xx
#else /*defined(FEATURE_VECTOR_FACILITY)*/
 #define disasm_a4xx disasm_none
 #define disasm_a6xx disasm_none
 #define disasm_e4xx disasm_none
#endif /*defined(FEATURE_VECTOR_FACILITY)*/


#define DISASM_TYPE(_type)  \
static int disasm_ ## _type (BYTE inst[], char mnemonic[], char *p) \
{ \
char* name; \
char operands[64]

#define DISASM_PRINT(...) \
    name = mnemonic+1; \
    while(*name++);  \
    snprintf(operands,sizeof(operands), ## __VA_ARGS__ ); \
    return sprintf(p, "%-5s %-19s    %s",mnemonic,operands,name); \
}


DISASM_TYPE(none);
  UNREFERENCED(inst);
  DISASM_PRINT("%c",',')

DISASM_TYPE(E);
    UNREFERENCED(inst);
    DISASM_PRINT("%c",',')


DISASM_TYPE(RR);
int r1, r2;
    r1 = inst[1] >> 4;
    r2 = inst[1] & 0x0F;
    DISASM_PRINT("%d,%d",r1,r2)


// "Mnemonic   R1"
DISASM_TYPE(RR_R1);
int r1;
    r1 = inst[1] >> 4;
    DISASM_PRINT("%d",r1)

DISASM_TYPE(RR_SVC);
    DISASM_PRINT("%d",inst[1])

DISASM_TYPE(RRE);
int r1, r2;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    DISASM_PRINT("%d,%d",r1,r2)

// "Mnemonic   R1"
DISASM_TYPE(RRE_R1);
int r1;
    r1 = inst[3] >> 4;
    DISASM_PRINT("%d",r1)

DISASM_TYPE(RRF_R);
int r1,r3,r2;
    r1 = inst[2] >> 4;
    r3 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    DISASM_PRINT("%d,%d,%d",r1,r3,r2)

DISASM_TYPE(RRF_M);
int m3,r1,r2;
    m3 = inst[2] >> 4;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    DISASM_PRINT("%d,%d,%d",r1,m3,r2)

DISASM_TYPE(RRF_M3);
int m3,r1,r2;
    m3 = inst[2] >> 4;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    DISASM_PRINT("%d,%d,%d",r1,r2,m3)

DISASM_TYPE(RRF_M4);
int m4,r1,r2;
    m4 = inst[2] & 0x0F;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    DISASM_PRINT("%d,%d,%d",r1,r2,m4)

DISASM_TYPE(RRF_MM);
int m3,m4,r1,r2;
    m3 = inst[2] >> 4;
    m4 = inst[2] & 0x0F;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    DISASM_PRINT("%d,%d,%d,%d",r1,m3,r2,m4)

DISASM_TYPE(RRF_RM);
int r3,m4,r1,r2;
    r3 = inst[2] >> 4;
    m4 = inst[2] & 0x0F;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    DISASM_PRINT("%d,%d,%d,%d",r1,r3,r2,m4)

DISASM_TYPE(RRR);
int r1,r2,r3;
    r3 = inst[2] >> 4;
    r1 = inst[3] >> 4;
    r2 = inst[3] & 0x0F;
    DISASM_PRINT("%d,%d,%d",r1,r2,r3)

DISASM_TYPE(RX);
int r1,x2,b2,d2;
    r1 = inst[1] >> 4;
    x2 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d,%d(%d,%d)",r1,d2,x2,b2)

DISASM_TYPE(RXE);
int r1,x2,b2,d2;
    r1 = inst[1] >> 4;
    x2 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d,%d(%d,%d)",r1,d2,x2,b2)

DISASM_TYPE(RXY);
int r1,x2,b2,d2;
    r1 = inst[1] >> 4;
    x2 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (((S8)inst[4]) << 12) | (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d,%d(%d,%d)",r1,d2,x2,b2)

DISASM_TYPE(RXF);
int r1,r3,x2,b2,d2;
    r1 = inst[4] >> 4;
    r3 = inst[1] >> 4;
    x2 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d,%d,%d(%d,%d)",r1,r3,d2,x2,b2)

DISASM_TYPE(RS);
int r1,r3,b2,d2;
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d,%d,%d(%d)",r1,r3,d2,b2)

// "Mnemonic   R1,D2(B2)"
DISASM_TYPE(RS_R1D2B2);
int r1,b2,d2;
    r1 = inst[1] >> 4;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d,%d(%d)",r1,d2,b2)

DISASM_TYPE(RSE);
int r1,r3,b2,d2;
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d,%d,%d(%d)",r1,r3,d2,b2)

DISASM_TYPE(RSY);
int r1,r3,b2,d2;
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (((S8)inst[4]) << 12) | (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d,%d,%d(%d)",r1,r3,d2,b2)

DISASM_TYPE(RSY_M3);
int r1,b2,d2,m3;
    r1 = inst[1] >> 4;
    m3 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (((S8)inst[4]) << 12) | (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d,%d(%d),%d",r1,d2,b2,m3)

DISASM_TYPE(RSL);
int l1,b1,d1;
    l1 = inst[1] >> 4;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d(%d,%d)",d1,l1+1,b1)

DISASM_TYPE(RSI);
int r1,r3,i2;
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    i2 = (S16)(((U16)inst[2] << 8) | inst[3]);
    DISASM_PRINT("%d,%d,*%+d",r1,r3,i2*2)

DISASM_TYPE(RI);
int r1,i2;
    r1 = inst[1] >> 4;
    i2 = (S16)(((U16)inst[2] << 8) | inst[3]);
    DISASM_PRINT("%d,%d",r1,i2)

DISASM_TYPE(RI_B);
int r1,i2;
    r1 = inst[1] >> 4;
    i2 = (S16)(((U16)inst[2] << 8) | inst[3]);
    DISASM_PRINT("%d,*%+d",r1,i2*2)

DISASM_TYPE(RIE);
int r1,r3,i2;
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    i2 = (S16)(((U16)inst[2] << 8) | inst[3]);
    DISASM_PRINT("%d,%d,*%+d",r1,r3,i2*2)

DISASM_TYPE(RIE_RRI);
int r1,r3,i2;
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    i2 = (S16)(((U16)inst[2] << 8) | inst[3]);
    DISASM_PRINT("%d,%d,%d",r1,r3,i2)

DISASM_TYPE(RIE_RIM);
int r1,i2,m3;
    r1 = inst[1] >> 4;
    i2 = (S16)(((U16)inst[2] << 8) | inst[3]);
    m3 = inst[4] >> 4;
    DISASM_PRINT("%d,%d,%d",r1,i2,m3)

DISASM_TYPE(RIE_RRIM);
int r1,r2,i4,m3;
    r1 = inst[1] >> 4;
    r2 = inst[1] & 0x0F;
    i4 = (S16)(((U16)inst[2] << 8) | inst[3]);
    m3 = inst[4] >> 4;
    DISASM_PRINT("%d,%d,%d,*%+d",r1,r2,m3,i4*2)

DISASM_TYPE(RIE_RMII);
int r1,m3,i4,i2;
    r1 = inst[1] >> 4;
    m3 = inst[1] & 0x0F;
    i4 = (S16)(((U16)inst[2] << 8) | inst[3]);
    i2 = inst[4];
    DISASM_PRINT("%d,%d,%d,*%+d",r1,i2,m3,i4*2)

DISASM_TYPE(RIE_RRIII);
int r1,r2,i3,i4,i5;
    r1 = inst[1] >> 4;
    r2 = inst[1] & 0x0F;
    i3 = inst[2];
    i4 = inst[3];
    i5 = inst[4];
    DISASM_PRINT("%d,%d,%d,%d,%d",r1,r2,i3,i4,i5)

DISASM_TYPE(RIL);
int r1,i2;
    r1 = inst[1] >> 4;
    i2 = (S32)((((U32)inst[2] << 24) | ((U32)inst[3] << 16)
       | ((U32)inst[4] << 8)) | inst[5]);
    DISASM_PRINT("%d,%"PRId32,r1,i2)

DISASM_TYPE(RIL_A);
int r1,i2;
    const S64 Two_S64=2;
    r1 = inst[1] >> 4;
    i2 = (S32)((((U32)inst[2] << 24) | ((U32)inst[3] << 16)
       | ((U32)inst[4] << 8)) | inst[5]);
    DISASM_PRINT("%d,*%+"PRId64,r1,i2*Two_S64)

DISASM_TYPE(RIS);
int r1,i2,m3,b4,d4;
    r1 = inst[1] >> 4;
    m3 = inst[1] & 0x0F;
    b4 = inst[2] >> 4;
    d4 = (inst[2] & 0x0F) << 8 | inst[3];
    i2 = inst[4];
    DISASM_PRINT("%d,%d,%d,%d(%d)",r1,i2,m3,d4,b4)

DISASM_TYPE(RRS);
int r1,r2,m3,b4,d4;
    r1 = inst[1] >> 4;
    r2 = inst[1] & 0x0F;
    b4 = inst[2] >> 4;
    d4 = (inst[2] & 0x0F) << 8 | inst[3];
    m3 = inst[4] >> 4;
    DISASM_PRINT("%d,%d,%d,%d(%d)",r1,r2,m3,d4,b4)

DISASM_TYPE(SI);
int i2,b1,d1;
    i2 = inst[1];
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d(%d),%d",d1,b1,i2)

DISASM_TYPE(SIY);
int i2,b1,d1;
    i2 = inst[1];
    b1 = inst[2] >> 4;
    d1 = (((S8)inst[4]) << 12) | (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d(%d),%d",d1,b1,i2)

DISASM_TYPE(SIL);
int b1,d1,i2;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    i2 = (S16)(((U16)inst[4] << 8) | inst[5]);
    DISASM_PRINT("%d(%d),%d",d1,b1,i2)

DISASM_TYPE(S);
int d2,b2;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d(%d)",d2,b2)

DISASM_TYPE(SS);
int l1,l2,b1,d1,b2,d2;
    l1 = inst[1] >> 4;
    l2 = inst[1] & 0x0F;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    DISASM_PRINT("%d(%d,%d),%d(%d,%d)",d1,l1+1,b1,d2,l2+1,b2)

DISASM_TYPE(SS_L);
int l1,b1,d1,b2,d2;
    l1 = inst[1];
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    DISASM_PRINT("%d(%d,%d),%d(%d)",d1,l1+1,b1,d2,b2)

// "Mnemonic   D1(B1),D2(L2,B2)"
DISASM_TYPE(SS_L2);
int l2,b1,d1,b2,d2;
    l2 = inst[1];
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    DISASM_PRINT("%d(%d),%d(%d,%d)",d1,b1,d2,l2+1,b2)

DISASM_TYPE(SS_R);
int r1,r3,b2,d2,b4,d4;
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    b4 = inst[4] >> 4;
    d4 = (inst[4] & 0x0F) << 8 | inst[5];
    DISASM_PRINT("%d,%d,%d(%d),%d(%d)",r1,r3,d2,b2,d4,b4)

// "Mnemonic   D1(R1,B1),D2(B2),R3"
DISASM_TYPE(SS_R3);
int r1,r3,b1,d1,b2,d2;
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    DISASM_PRINT("%d(%d,%d),%d(%d),%d",d1,r1,b1,d2,b2,r3)

// "Mnemonic   R1,D2(B2),R3,D4(B4)"
DISASM_TYPE(SS_RSRS);
int r1,r3,b2,d2,b4,d4;
    r1 = inst[1] >> 4;
    r3 = inst[1] & 0x0F;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    b4 = inst[4] >> 4;
    d4 = (inst[4] & 0x0F) << 8 | inst[5];
    DISASM_PRINT("%d,%d(%d),%d,%d(%d)",r1,d2,b2,r3,d4,b4)

// "Mnemonic   D1(L1,B1),D2(B2),I3"
DISASM_TYPE(SS_I);
int l1,i3,b1,d1,b2,d2;
    l1 = inst[1] >> 4;
    i3 = inst[1] & 0x0F;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    DISASM_PRINT("%d(%d,%d),%d(%d),%d",d1,l1,b1,d2,b2,i3)

DISASM_TYPE(SSE);
int b1,d1,b2,d2;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    DISASM_PRINT("%d(%d),%d(%d)",d1,b1,d2,b2)

DISASM_TYPE(SSF);
int r3,b1,d1,b2,d2;
    r3 = inst[1] >> 4;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    DISASM_PRINT("%d(%d),%d(%d),%d",d1,b1,d2,b2,r3)

DISASM_TYPE(SSF_RSS);
int r3,b1,d1,b2,d2;
    r3 = inst[1] >> 4;
    b1 = inst[2] >> 4;
    d1 = (inst[2] & 0x0F) << 8 | inst[3];
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    DISASM_PRINT("%d,%d(%d),%d(%d)",r3,d1,b1,d2,b2)

DISASM_TYPE(VST);
int vr3,rt2,vr1,rs2;
    vr3 = inst[2] >> 4;
    rt2 = inst[2] & 0x0F;
    vr1 = inst[3] >> 4;
    rs2 = inst[3] & 0x0F;
    DISASM_PRINT("%d,%d,%d(%d)",vr1,vr3,rs2,rt2)

DISASM_TYPE(VR);
int vr1,fr3,gr2;
    fr3 = inst[2] >> 4;
    vr1 = inst[3] >> 4;
    gr2 = inst[3] & 0x0F;
    DISASM_PRINT("%d,%d,%d",vr1,fr3,gr2)

DISASM_TYPE(VS);
int rs2;
    rs2 = inst[3] & 0x0F;
    DISASM_PRINT("%d",rs2)

DISASM_TYPE(VRSE);
int vr1,vr3,d2,b2;
    vr3 = inst[2] >> 4;
    vr1 = inst[3] >> 4;
    b2 = inst[4] >> 4;
    d2 = (inst[4] & 0x0F) << 8 | inst[5];
    DISASM_PRINT("%d,%d,%d(%d)",vr1,vr3,d2,b2)

DISASM_TYPE(S_NW);
int d2,b2;
    b2 = inst[2] >> 4;
    d2 = (inst[2] & 0x0F) << 8 | inst[3];
    DISASM_PRINT("%d(%d)",d2,b2)


/*----------------------------------------------------------------------------*/
/* Two byte runtime opcode table + 4 6 byte opcode tables                     */
/*----------------------------------------------------------------------------*/
static zz_func runtime_opcode_xxxx[GEN_ARCHCOUNT][0x100 * 0x100];
static zz_func runtime_opcode_e3________xx[GEN_ARCHCOUNT][0x100];
static zz_func runtime_opcode_eb________xx[GEN_ARCHCOUNT][0x100];
static zz_func runtime_opcode_ec________xx[GEN_ARCHCOUNT][0x100];
static zz_func runtime_opcode_ed________xx[GEN_ARCHCOUNT][0x100];

#ifdef OPTION_OPTINST
static zz_func runtime_opcode_e3_0______xx[GEN_ARCHCOUNT][0x100];
#endif /* #ifdef OPTION_OPTINST */


/*----------------------------------------------------------------------------*/
/* replace_opcode_xx                                                          */
/*----------------------------------------------------------------------------*/
static zz_func replace_opcode_xx(int arch, zz_func inst, int opcode)
{
  int i;
  zz_func oldinst;

//  logmsg("replace_opcode_xx(%d, %02x)\n", arch, opcode);
  if(arch < 0 || arch > GEN_ARCHCOUNT)
    return(NULL);
  if(opcode < 0 || opcode > 0xff)
    return(NULL);
  if(!inst)
    return(NULL);
  oldinst = runtime_opcode_xxxx[arch][opcode * 0x100];
  for(i = 0; i < 0x100; i++)
    runtime_opcode_xxxx[arch][opcode * 0x100 + i] = inst;
  return(oldinst);
}


/*---------------------------------------------------------------------------*/
/* replace_opcode_xxxx                                                       */
/*---------------------------------------------------------------------------*/
static zz_func replace_opcode_xxxx(int arch, zz_func inst, int opcode1, int opcode2)
{
  zz_func oldinst;

//  logmsg("replace_opcode_xxxx(%d, %02x, %02x)\n", arch, opcode1, opcode2);
  if(arch < 0 || arch >= GEN_ARCHCOUNT)
    return(NULL);
  if(opcode1 < 0 || opcode1 > 0xff || opcode2 < 0 || opcode2 > 0xff)
    return(NULL);
  if(!inst)
    return(NULL);
  oldinst = runtime_opcode_xxxx[arch][opcode1 * 0x100 + opcode2];
  runtime_opcode_xxxx[arch][opcode1 * 0x100 + opcode2] = inst;
  return(oldinst);
}


/*---------------------------------------------------------------------------*/
/* replace_opcode_xx_x                                                       */
/*---------------------------------------------------------------------------*/
static zz_func replace_opcode_xx_x(int arch, zz_func inst, int opcode1, int opcode2)
{
  int i;
  zz_func oldinst;

//  logmsg("replace_opcode_xx_x(%d, %02x, %1x)\n", arch, opcode1, opcode2);
  if(arch < 0 || arch >= GEN_ARCHCOUNT)
    return(NULL);
  if(opcode1 < 0 || opcode1 > 0xff || opcode2 < 0 || opcode2 > 0xf)
    return(NULL);
  if(!inst)
    return(NULL);
  oldinst = runtime_opcode_xxxx[arch][opcode1 * 0x100 + opcode2];
  for(i = 0; i < 0x10; i++)
    runtime_opcode_xxxx[arch][opcode1 * 0x100 + i * 0x10 + opcode2] = inst;
  return(oldinst);
}


/*---------------------------------------------------------------------------*/
/* replace_opcode_xx________xx                                               */
/*---------------------------------------------------------------------------*/
static zz_func replace_opcode_xx________xx(int arch, zz_func inst, int opcode1, int opcode2)
{
  zz_func oldinst;

//  logmsg("replace_opcode_xx________xx(%d, %02x, %02x)\n", arch, opcode1, opcode2);
  if(arch < 0 || arch >= GEN_ARCHCOUNT)
    return(NULL);
  if(opcode2 < 0 || opcode2 > 0xff)
    return(NULL);
  if(!inst)
    return(NULL);
  switch(opcode1)
  {
    case 0xe3:
    {
      oldinst = runtime_opcode_e3________xx[arch][opcode2];
      runtime_opcode_e3________xx[arch][opcode2] = inst;
    }
    case 0xeb:
    {
      oldinst = runtime_opcode_eb________xx[arch][opcode2];
      runtime_opcode_eb________xx[arch][opcode2] = inst;
    }
    case 0xec:
    {
      oldinst = runtime_opcode_ec________xx[arch][opcode2];
      runtime_opcode_ec________xx[arch][opcode2] = inst;
    }
    case 0xed:
    {
      oldinst = runtime_opcode_ed________xx[arch][opcode2];
      runtime_opcode_ed________xx[arch][opcode2] = inst;
    }
    default:
    {
      oldinst = NULL;
    }
  }
  return(oldinst);
}


/*---------------------------------------------------------------------------*/
/* replace_opcode                                                            */
/*---------------------------------------------------------------------------*/
#if defined(OPTION_DYNAMIC_LOAD)
DLL_EXPORT void *replace_opcode_r(int arch, zz_func inst, int opcode1, int opcode2)
#else
void *replace_opcode(int arch, zz_func inst, int opcode1, int opcode2)
#endif
{
//  logmsg("replace_opcode(%d, %02x, %02x)\n", arch, opcode1, opcode2);
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
      if(arch == ARCH_900)
        return(replace_opcode_xx_x(arch, inst, opcode1, opcode2));
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
}


/*----------------------------------------------------------------------------*/
/* init_opcode_tables                                                         */
/*----------------------------------------------------------------------------*/
void init_opcode_tables(void)
{
  int arch;
  int bit;
  int i;

//  logmsg("init_opcode_tables()\n");
  for(arch = 0; arch < GEN_ARCHCOUNT; arch++)
  {
    for(i = 0; i < 0x100; i++)
      replace_opcode_xx(arch, opcode_table[i][arch], i);
    for(i = 0; i < 0x100; i++)
    {
      replace_opcode_xxxx(arch, opcode_01xx[i][arch], 0x01, i);
      if(arch != ARCH_900)
      {
        replace_opcode_xxxx(arch, v_opcode_a4xx[i][arch], 0xa4, i);
        replace_opcode_xxxx(arch, v_opcode_a5xx[i][arch], 0xa5, i);
        replace_opcode_xxxx(arch, v_opcode_a6xx[i][arch], 0xa6, i);
      }
      replace_opcode_xxxx(arch, opcode_b2xx[i][arch], 0xb2, i);
      replace_opcode_xxxx(arch, opcode_b3xx[i][arch], 0xb3, i);
      replace_opcode_xxxx(arch, opcode_b9xx[i][arch], 0xb9, i);
      replace_opcode_xx________xx(arch, opcode_e3xx[i][arch], 0xe3, i);
      if(arch != ARCH_900)
        replace_opcode_xxxx(arch, v_opcode_e4xx[i][arch], 0xe4, i);
      replace_opcode_xxxx(arch, opcode_e5xx[i][arch], 0xe5, i);
      replace_opcode_xxxx(arch, opcode_e6xx[i][arch], 0xe6, i);
      replace_opcode_xx________xx(arch, opcode_ebxx[i][arch], 0xeb, i);
      replace_opcode_xx________xx(arch, opcode_ecxx[i][arch], 0xec, i);
      replace_opcode_xx________xx(arch, opcode_edxx[i][arch], 0xed, i);
    }
    for(i = 0; i < 0x10; i++)
    {
      if(arch == ARCH_900)
        replace_opcode_xx_x(arch, opcode_a5_x[i][arch], 0xa5, i);
      replace_opcode_xx_x(arch, opcode_a7_x[i][arch], 0xa7, i);
      replace_opcode_xx_x(arch, opcode_c0_x[i][arch], 0xc0, i);
      replace_opcode_xx_x(arch, opcode_c2_x[i][arch], 0xc2, i);
      replace_opcode_xx_x(arch, opcode_c4_x[i][arch], 0xc4, i);
      replace_opcode_xx_x(arch, opcode_c6_x[i][arch], 0xc6, i);
      replace_opcode_xx_x(arch, opcode_c8_x[i][arch], 0xc8, i);
      replace_opcode_xx_x(arch, opcode_cc_x[i][arch], 0xcc, i);
    }

#ifdef OPTION_OPTINST
    for(i = 0; i < 0x100; i++)
    {
      replace_opcode_xxxx(arch, opcode_15__[i][arch], 0x15, i); /* Optimized CLR */
      replace_opcode_xxxx(arch, opcode_18__[i][arch], 0x18, i); /* Optimized LR */
      replace_opcode_xxxx(arch, opcode_1E__[i][arch], 0x1e, i); /* Optimized ALR */
      replace_opcode_xxxx(arch, opcode_1F__[i][arch], 0x1f, i); /* Optimized SLR */
      replace_opcode_xxxx(arch, opcode_BF_x[0][arch], 0xbf, i); /* Optimized ICM */
    }
    for(i = 0; i < 0x10; i++)
    {
      replace_opcode_xxxx(arch, opcode_41_0[i][arch], 0x41, i << 4); /* Optimized LA */
      replace_opcode_xxxx(arch, opcode_47_0[i][arch], 0x47, i << 4); /* Optimized BC */
      replace_opcode_xxxx(arch, opcode_50_0[i][arch], 0x50, i << 4); /* Optimized ST */
      replace_opcode_xxxx(arch, opcode_55_0[i][arch], 0x55, i << 4); /* Optimized CL */
      replace_opcode_xxxx(arch, opcode_58_0[i][arch], 0x58, i << 4); /* Optimized L */
      replace_opcode_xxxx(arch, opcode_A7_4[i][arch], 0xa7, (i << 4) + 0x4); /* Optimized BRC */
      replace_opcode_xxxx(arch, opcode_BF_x[1][arch], 0xbf, (i << 4) + 0x7); /* Optimized ICM */
      replace_opcode_xxxx(arch, opcode_BF_x[2][arch], 0xbf, (i << 4) + 0xf); /* Optimized ICM */
      replace_opcode_xxxx(arch, opcode_E3_0[0][arch], 0xe3, i << 4);
    }
    replace_opcode_xxxx(arch, opcode_D20x[0][arch], 0xd2, 0x00); /* Optimized MVC */
    replace_opcode_xxxx(arch, opcode_D50x[0][arch], 0xd5, 0x00); /* Optimized CLC */
    replace_opcode_xxxx(arch, opcode_D50x[1][arch], 0xd5, 0x01); /* Optimized CLC */
    replace_opcode_xxxx(arch, opcode_D50x[2][arch], 0xd5, 0x03); /* Optimized CLC */
    replace_opcode_xxxx(arch, opcode_D50x[3][arch], 0xd5, 0x07); /* Optimized CLC */
    bit = 0x80;
    for(i = 0; i < 8; i++)
    {
      replace_opcode_xxxx(arch, opcode_91xx[i][arch], 0x91, bit); /* Single bit TM */
      bit >>= 1;
    }
    for(i = 0; i < 0x100; i++)
    {
      switch(i)
      {
        case 0x04:
          runtime_opcode_e3_0______xx[arch][i] = opcode_E3_0______04[0][arch]; /* Optimized LG */
          break;
        case 0x24:
          runtime_opcode_e3_0______xx[arch][i] = opcode_E3_0______24[0][arch]; /* Optimized STG */
          break;
        default:
          runtime_opcode_e3_0______xx[arch][i] = opcode_e3xx[i][arch];
          break;
      }
    }
#endif /* OPTION_OPTINST */
  }
}


/*---------------------------------------------------------------------------*/
/* init_opcode_pointers                                                      */
/*---------------------------------------------------------------------------*/
void init_opcode_pointers(REGS *regs)
{
//  logmsg("init_opcode_pointers()\n");
  if(!regs)
    return;
  regs->s370_runtime_opcode_xxxx = runtime_opcode_xxxx[ARCH_370];
  regs->s370_runtime_opcode_e3________xx = runtime_opcode_e3________xx[ARCH_370];
  regs->s370_runtime_opcode_eb________xx = runtime_opcode_eb________xx[ARCH_370];
  regs->s370_runtime_opcode_ec________xx = runtime_opcode_ec________xx[ARCH_370];
  regs->s370_runtime_opcode_ed________xx = runtime_opcode_ed________xx[ARCH_370];
  regs->s390_runtime_opcode_xxxx = runtime_opcode_xxxx[ARCH_390];
  regs->s390_runtime_opcode_e3________xx = runtime_opcode_e3________xx[ARCH_390];
  regs->s390_runtime_opcode_eb________xx = runtime_opcode_eb________xx[ARCH_390];
  regs->s390_runtime_opcode_ec________xx = runtime_opcode_ec________xx[ARCH_390];
  regs->s390_runtime_opcode_ed________xx = runtime_opcode_ed________xx[ARCH_390];
  regs->z900_runtime_opcode_xxxx = runtime_opcode_xxxx[ARCH_900];
  regs->z900_runtime_opcode_e3________xx = runtime_opcode_e3________xx[ARCH_900];
  regs->z900_runtime_opcode_eb________xx = runtime_opcode_eb________xx[ARCH_900];
  regs->z900_runtime_opcode_ec________xx = runtime_opcode_ec________xx[ARCH_900];
  regs->z900_runtime_opcode_ed________xx = runtime_opcode_ed________xx[ARCH_900];

#ifdef OPTION_OPTINST
  regs->s370_runtime_opcode_e3_0______xx = runtime_opcode_e3_0______xx[ARCH_370];
  regs->s390_runtime_opcode_e3_0______xx = runtime_opcode_e3_0______xx[ARCH_390];
  regs->z900_runtime_opcode_e3_0______xx = runtime_opcode_e3_0______xx[ARCH_900];
#endif /* OPTION_OPTINST */
}


/*---------------------------------------------------------------------------*/
/* Following function will be resolved within the runtime opcode tables and  */
/* set with function init_opcode_tables.                                     */
/*---------------------------------------------------------------------------*/
#define execute_opcode_01xx operation_exception
#define execute_opcode_a4xx operation_exception
#define execute_opcode_a5_x operation_exception
#define execute_opcode_a6xx operation_exception
#define execute_opcode_a7_x operation_exception
#define execute_opcode_b2xx operation_exception
#define execute_opcode_b3xx operation_exception
#define execute_opcode_b9xx operation_exception
#define execute_opcode_c0_x operation_exception
#define execute_opcode_c2_x operation_exception
#define execute_opcode_c4_x operation_exception
#define execute_opcode_c6_x operation_exception
#define execute_opcode_c8_x operation_exception
#define execute_opcode_cc_x operation_exception
#define execute_opcode_e4xx operation_exception
#define execute_opcode_e5xx operation_exception
#define execute_opcode_e6xx operation_exception


static zz_func opcode_table[0x100][GEN_MAXARCH] = {
 /*00*/   GENx___x___x___ ,
 /*01*/   GENx370x390x900 (execute_opcode_01xx,01xx,""),
 /*02*/   GENx___x___x___ ,
 /*03*/   GENx___x___x___ ,
 /*04*/   GENx370x390x900 (set_program_mask,RR_R1,"SPM"),
 /*05*/   GENx370x390x900 (branch_and_link_register,RR,"BALR"),
 /*06*/   GENx370x390x900 (branch_on_count_register,RR,"BCTR"),
 /*07*/   GENx370x390x900 (branch_on_condition_register,RR,"BCR"),
 /*08*/   GENx370x___x___ (set_storage_key,RR,"SSK"),
 /*09*/   GENx370x___x___ (insert_storage_key,RR,"ISK"),
 /*0A*/   GENx370x390x900 (supervisor_call,RR_SVC,"SVC"),
 /*0B*/   GENx___x390x900 (branch_and_set_mode,RR,"BSM"),
 /*0C*/   GENx___x390x900 (branch_and_save_and_set_mode,RR,"BASSM"),
 /*0D*/   GENx370x390x900 (branch_and_save_register,RR,"BASR"),
 /*0E*/   GENx370x390x900 (move_long,RR,"MVCL"),
 /*0F*/   GENx370x390x900 (compare_logical_character_long,RR,"CLCL"),
 /*10*/   GENx370x390x900 (load_positive_register,RR,"LPR"),
 /*11*/   GENx370x390x900 (load_negative_register,RR,"LNR"),
 /*12*/   GENx370x390x900 (load_and_test_register,RR,"LTR"),
 /*13*/   GENx370x390x900 (load_complement_register,RR,"LCR"),
 /*14*/   GENx370x390x900 (and_register,RR,"NR"),
 /*15*/   GENx370x390x900 (compare_logical_register,RR,"CLR"),
 /*16*/   GENx370x390x900 (or_register,RR,"OR"),
 /*17*/   GENx370x390x900 (exclusive_or_register,RR,"XR"),
 /*18*/   GENx370x390x900 (load_register,RR,"LR"),
 /*19*/   GENx370x390x900 (compare_register,RR,"CR"),
 /*1A*/   GENx370x390x900 (add_register,RR,"AR"),
 /*1B*/   GENx370x390x900 (subtract_register,RR,"SR"),
 /*1C*/   GENx370x390x900 (multiply_register,RR,"MR"),
 /*1D*/   GENx370x390x900 (divide_register,RR,"DR"),
 /*1E*/   GENx370x390x900 (add_logical_register,RR,"ALR"),
 /*1F*/   GENx370x390x900 (subtract_logical_register,RR,"SLR"),
 /*20*/   GENx370x390x900 (load_positive_float_long_reg,RR,"LPDR"),
 /*21*/   GENx370x390x900 (load_negative_float_long_reg,RR,"LNDR"),
 /*22*/   GENx370x390x900 (load_and_test_float_long_reg,RR,"LTDR"),
 /*23*/   GENx370x390x900 (load_complement_float_long_reg,RR,"LCDR"),
 /*24*/   GENx370x390x900 (halve_float_long_reg,RR,"HDR"),
 /*25*/   GENx370x390x900 (load_rounded_float_long_reg,RR,"LDXR"),
 /*26*/   GENx370x390x900 (multiply_float_ext_reg,RR,"MXR"),
 /*27*/   GENx370x390x900 (multiply_float_long_to_ext_reg,RR,"MXDR"),
 /*28*/   GENx370x390x900 (load_float_long_reg,RR,"LDR"),
 /*29*/   GENx370x390x900 (compare_float_long_reg,RR,"CDR"),
 /*2A*/   GENx370x390x900 (add_float_long_reg,RR,"ADR"),
 /*2B*/   GENx370x390x900 (subtract_float_long_reg,RR,"SDR"),
 /*2C*/   GENx370x390x900 (multiply_float_long_reg,RR,"MDR"),
 /*2D*/   GENx370x390x900 (divide_float_long_reg,RR,"DDR"),
 /*2E*/   GENx370x390x900 (add_unnormal_float_long_reg,RR,"AWR"),
 /*2F*/   GENx370x390x900 (subtract_unnormal_float_long_reg,RR,"SWR"),
 /*30*/   GENx370x390x900 (load_positive_float_short_reg,RR,"LPER"),
 /*31*/   GENx370x390x900 (load_negative_float_short_reg,RR,"LNER"),
 /*32*/   GENx370x390x900 (load_and_test_float_short_reg,RR,"LTER"),
 /*33*/   GENx370x390x900 (load_complement_float_short_reg,RR,"LCER"),
 /*34*/   GENx370x390x900 (halve_float_short_reg,RR,"HER"),
 /*35*/   GENx370x390x900 (load_rounded_float_short_reg,RR,"LEDR"),
 /*36*/   GENx370x390x900 (add_float_ext_reg,RR,"AXR"),
 /*37*/   GENx370x390x900 (subtract_float_ext_reg,RR,"SXR"),
 /*38*/   GENx370x390x900 (load_float_short_reg,RR,"LER"),
 /*39*/   GENx370x390x900 (compare_float_short_reg,RR,"CER"),
 /*3A*/   GENx370x390x900 (add_float_short_reg,RR,"AER"),
 /*3B*/   GENx370x390x900 (subtract_float_short_reg,RR,"SER"),
 /*3C*/   GENx370x390x900 (multiply_float_short_to_long_reg,RR,"MDER"),
 /*3D*/   GENx370x390x900 (divide_float_short_reg,RR,"DER"),
 /*3E*/   GENx370x390x900 (add_unnormal_float_short_reg,RR,"AUR"),
 /*3F*/   GENx370x390x900 (subtract_unnormal_float_short_reg,RR,"SUR"),
 /*40*/   GENx370x390x900 (store_halfword,RX,"STH"),
 /*41*/   GENx370x390x900 (load_address,RX,"LA"),
 /*42*/   GENx370x390x900 (store_character,RX,"STC"),
 /*43*/   GENx370x390x900 (insert_character,RX,"IC"),
 /*44*/   GENx370x390x900 (execute,RX,"EX"),
 /*45*/   GENx370x390x900 (branch_and_link,RX,"BAL"),
 /*46*/   GENx370x390x900 (branch_on_count,RX,"BCT"),
 /*47*/   GENx370x390x900 (branch_on_condition,RX,"BC"),
 /*48*/   GENx370x390x900 (load_halfword,RX,"LH"),
 /*49*/   GENx370x390x900 (compare_halfword,RX,"CH"),
 /*4A*/   GENx370x390x900 (add_halfword,RX,"AH"),
 /*4B*/   GENx370x390x900 (subtract_halfword,RX,"SH"),
 /*4C*/   GENx370x390x900 (multiply_halfword,RX,"MH"),
 /*4D*/   GENx370x390x900 (branch_and_save,RX,"BAS"),
 /*4E*/   GENx370x390x900 (convert_to_decimal,RX,"CVD"),
 /*4F*/   GENx370x390x900 (convert_to_binary,RX,"CVB"),
 /*50*/   GENx370x390x900 (store,RX,"ST"),
 /*51*/   GENx___x390x900 (load_address_extended,RX,"LAE"),
 /*52*/   GENx___x___x___ ,
 /*53*/   GENx___x___x___ ,
 /*54*/   GENx370x390x900 (and,RX,"N"),
 /*55*/   GENx370x390x900 (compare_logical,RX,"CL"),
 /*56*/   GENx370x390x900 (or,RX,"O"),
 /*57*/   GENx370x390x900 (exclusive_or,RX,"X"),
 /*58*/   GENx370x390x900 (load,RX,"L"),
 /*59*/   GENx370x390x900 (compare,RX,"C"),
 /*5A*/   GENx370x390x900 (add,RX,"A"),
 /*5B*/   GENx370x390x900 (subtract,RX,"S"),
 /*5C*/   GENx370x390x900 (multiply,RX,"M"),
 /*5D*/   GENx370x390x900 (divide,RX,"D"),
 /*5E*/   GENx370x390x900 (add_logical,RX,"AL"),
 /*5F*/   GENx370x390x900 (subtract_logical,RX,"SL"),
 /*60*/   GENx370x390x900 (store_float_long,RX,"STD"),
 /*61*/   GENx___x___x___ ,
 /*62*/   GENx___x___x___ ,
 /*63*/   GENx___x___x___ ,
 /*64*/   GENx___x___x___ ,
 /*65*/   GENx___x___x___ ,
 /*66*/   GENx___x___x___ ,
 /*67*/   GENx370x390x900 (multiply_float_long_to_ext,RX,"MXD"),
 /*68*/   GENx370x390x900 (load_float_long,RX,"LD"),
 /*69*/   GENx370x390x900 (compare_float_long,RX,"CD"),
 /*6A*/   GENx370x390x900 (add_float_long,RX,"AD"),
 /*6B*/   GENx370x390x900 (subtract_float_long,RX,"SD"),
 /*6C*/   GENx370x390x900 (multiply_float_long,RX,"MD"),
 /*6D*/   GENx370x390x900 (divide_float_long,RX,"DD"),
 /*6E*/   GENx370x390x900 (add_unnormal_float_long,RX,"AW"),
 /*6F*/   GENx370x390x900 (subtract_unnormal_float_long,RX,"SW"),
 /*70*/   GENx370x390x900 (store_float_short,RX,"STE"),
 /*71*/   GENx37Xx390x900 (multiply_single,RX,"MS"),
 /*72*/   GENx___x___x___ ,
 /*73*/   GENx___x___x___ ,
 /*74*/   GENx___x___x___ ,
 /*75*/   GENx___x___x___ ,
 /*76*/   GENx___x___x___ ,
 /*77*/   GENx___x___x___ ,
 /*78*/   GENx370x390x900 (load_float_short,RX,"LE"),
 /*79*/   GENx370x390x900 (compare_float_short,RX,"CE"),
 /*7A*/   GENx370x390x900 (add_float_short,RX,"AE"),
 /*7B*/   GENx370x390x900 (subtract_float_short,RX,"SE"),
 /*7C*/   GENx370x390x900 (multiply_float_short_to_long,RX,"MDE"),
 /*7D*/   GENx370x390x900 (divide_float_short,RX,"DE"),
 /*7E*/   GENx370x390x900 (add_unnormal_float_short,RX,"AU"),
 /*7F*/   GENx370x390x900 (subtract_unnormal_float_short,RX,"SU"),
 /*80*/   GENx370x390x900 (set_system_mask,S,"SSM"),
 /*81*/   GENx___x___x___ ,
 /*82*/   GENx370x390x900 (load_program_status_word,S,"LPSW"),
 /*83*/   GENx370x390x900 (diagnose,RS,"DIAG"),
 /*84*/   GENx37Xx390x900 (branch_relative_on_index_high,RSI,"BRXH"),
 /*85*/   GENx37Xx390x900 (branch_relative_on_index_low_or_equal,RSI,"BRXLE"),
 /*86*/   GENx370x390x900 (branch_on_index_high,RS,"BXH"),
 /*87*/   GENx370x390x900 (branch_on_index_low_or_equal,RS,"BXLE"),
 /*88*/   GENx370x390x900 (shift_right_single_logical,RS_R1D2B2,"SRL"),
 /*89*/   GENx370x390x900 (shift_left_single_logical,RS_R1D2B2,"SLL"),
 /*8A*/   GENx370x390x900 (shift_right_single,RS_R1D2B2,"SRA"),
 /*8B*/   GENx370x390x900 (shift_left_single,RS_R1D2B2,"SLA"),
 /*8C*/   GENx370x390x900 (shift_right_double_logical,RS_R1D2B2,"SRDL"),
 /*8D*/   GENx370x390x900 (shift_left_double_logical,RS_R1D2B2,"SLDL"),
 /*8E*/   GENx370x390x900 (shift_right_double,RS_R1D2B2,"SRDA"),
 /*8F*/   GENx370x390x900 (shift_left_double,RS_R1D2B2,"SLDA"),
 /*90*/   GENx370x390x900 (store_multiple,RS,"STM"),
 /*91*/   GENx370x390x900 (test_under_mask,SI,"TM"),
 /*92*/   GENx370x390x900 (move_immediate,SI,"MVI"),
 /*93*/   GENx370x390x900 (test_and_set,S,"TS"),
 /*94*/   GENx370x390x900 (and_immediate,SI,"NI"),
 /*95*/   GENx370x390x900 (compare_logical_immediate,SI,"CLI"),
 /*96*/   GENx370x390x900 (or_immediate,SI,"OI"),
 /*97*/   GENx370x390x900 (exclusive_or_immediate,SI,"XI"),
 /*98*/   GENx370x390x900 (load_multiple,RS,"LM"),
 /*99*/   GENx___x390x900 (trace,RS,"TRACE"),
 /*9A*/   GENx___x390x900 (load_access_multiple,RS,"LAM"),
 /*9B*/   GENx___x390x900 (store_access_multiple,RS,"STAM"),
 /*9C*/   GENx370x390x900 (start_io,S,"SIO"),
 /*9D*/   GENx370x390x900 (test_io,S,"TIO"),
 /*9E*/   GENx370x390x900 (halt_io,S,"HIO"),
 /*9F*/   GENx370x390x900 (test_channel,S,"TCH"),
 /*A0*/   GENx___x___x___ ,
 /*A1*/   GENx___x___x___ ,
 /*A2*/   GENx___x___x___ ,
 /*A3*/   GENx___x___x___ ,
 /*A4*/   GENx370x390x900 (execute_opcode_a4xx,a4xx,""), /* Only with vector facility */
 /*A5*/   GENx370x390x900 (execute_opcode_a5_x,a5_x,""), /* execute_opcode_a5xx with vector facility */
 /*A6*/   GENx370x390x900 (execute_opcode_a6xx,a6xx,""),
 /*A7*/   GENx370x390x900 (execute_opcode_a7_x,a7_x,""),
 /*A8*/   GENx37Xx390x900 (move_long_extended,RS,"MVCLE"),
 /*A9*/   GENx37Xx390x900 (compare_logical_long_extended,RS,"CLCLE"),
 /*AA*/   GENx___x___x___ ,
 /*AB*/   GENx___x___x___ ,
 /*AC*/   GENx370x390x900 (store_then_and_system_mask,SI,"STNSM"),
 /*AD*/   GENx370x390x900 (store_then_or_system_mask,SI,"STOSM"),
 /*AE*/   GENx370x390x900 (signal_processor,RS,"SIGP"),
 /*AF*/   GENx370x390x900 (monitor_call,SI,"MC"),
 /*B0*/   GENx___x___x___ ,
 /*B1*/   GENx370x390x900 (load_real_address,RX,"LRA"),
 /*B2*/   GENx370x390x900 (execute_opcode_b2xx,b2xx,""),
 /*B3*/   GENx370x390x900 (execute_opcode_b3xx,b3xx,""),
 /*B4*/   GENx___x___x___ ,
 /*B5*/   GENx___x___x___ ,
 /*B6*/   GENx370x390x900 (store_control,RS,"STCTL"),
 /*B7*/   GENx370x390x900 (load_control,RS,"LCTL"),
 /*B8*/   GENx___x___x___ ,
 /*B9*/   GENx370x390x900 (execute_opcode_b9xx,b9xx,""),
 /*BA*/   GENx370x390x900 (compare_and_swap,RS,"CS"),
 /*BB*/   GENx370x390x900 (compare_double_and_swap,RS,"CDS"),
 /*BC*/   GENx___x___x___ ,
 /*BD*/   GENx370x390x900 (compare_logical_characters_under_mask,RS,"CLM"),
 /*BE*/   GENx370x390x900 (store_characters_under_mask,RS,"STCM"),
 /*BF*/   GENx370x390x900 (insert_characters_under_mask,RS,"ICM"),
 /*C0*/   GENx370x390x900 (execute_opcode_c0_x,c0_x,""),
 /*C1*/   GENx___x___x___ ,
 /*C2*/   GENx370x390x900 (execute_opcode_c2_x,c2_x,""),               /*@Z9*/
 /*C3*/   GENx___x___x___ ,
 /*C4*/   GENx370x390x900 (execute_opcode_c4_x,c4_x,""),               /*208*/
 /*C5*/   GENx___x___x___ ,
 /*C6*/   GENx370x390x900 (execute_opcode_c6_x,c6_x,""),               /*208*/
 /*C7*/   GENx___x___x___ ,
 /*C8*/   GENx370x390x900 (execute_opcode_c8_x,c8_x,""),
 /*C9*/   GENx___x___x___ ,
 /*CA*/   GENx___x___x___ ,
 /*CB*/   GENx___x___x___ ,
 /*CC*/   GENx370x390x900 (execute_opcode_cc_x,cc_x,""),               /*810*/
 /*CD*/   GENx___x___x___ ,
 /*CE*/   GENx___x___x___ ,
 /*CF*/   GENx___x___x___ ,
 /*D0*/   GENx37Xx390x900 (translate_and_test_reverse,SS_L,"TRTR"),
 /*D1*/   GENx370x390x900 (move_numerics,SS_L,"MVN"),
 /*D2*/   GENx370x390x900 (move_character,SS_L,"MVC"),
 /*D3*/   GENx370x390x900 (move_zones,SS_L,"MVZ"),
 /*D4*/   GENx370x390x900 (and_character,SS_L,"NC"),
 /*D5*/   GENx370x390x900 (compare_logical_character,SS_L,"CLC"),
 /*D6*/   GENx370x390x900 (or_character,SS_L,"OC"),
 /*D7*/   GENx370x390x900 (exclusive_or_character,SS_L,"XC"),
 /*D8*/   GENx___x___x___ ,
 /*D9*/   GENx370x390x900 (move_with_key,SS_R3,"MVCK"),
 /*DA*/   GENx370x390x900 (move_to_primary,SS_R3,"MVCP"),
 /*DB*/   GENx370x390x900 (move_to_secondary,SS_R3,"MVCS"),
 /*DC*/   GENx370x390x900 (translate,SS_L,"TR"),
 /*DD*/   GENx370x390x900 (translate_and_test,SS_L,"TRT"),
 /*DE*/   GENx370x390x900 (edit_x_edit_and_mark,SS_L,"ED"),
 /*DF*/   GENx370x390x900 (edit_x_edit_and_mark,SS_L,"EDMK"),
 /*E0*/   GENx___x___x___ ,
 /*E1*/   GENx37Xx390x900 (pack_unicode,SS_L2,"PKU"),
 /*E2*/   GENx37Xx390x900 (unpack_unicode,SS_L,"UNPKU"),
 /*E3*/   GENx370x390x900 (execute_opcode_e3________xx,e3xx,""),
 /*E4*/   GENx370x390x900 (execute_opcode_e4xx,e4xx,""),
 /*E5*/   GENx370x390x900 (execute_opcode_e5xx,e5xx,""),
 /*E6*/   GENx370x390x900 (execute_opcode_e6xx,e6xx,""),
 /*E7*/   GENx___x___x___ ,
 /*E8*/   GENx370x390x900 (move_inverse,SS_L,"MVCIN"),
 /*E9*/   GENx37Xx390x900 (pack_ascii,SS_L2,"PKA"),
 /*EA*/   GENx37Xx390x900 (unpack_ascii,SS_L,"UNPKA"),
 /*EB*/   GENx370x390x900 (execute_opcode_eb________xx,ebxx,""),
 /*EC*/   GENx370x390x900 (execute_opcode_ec________xx,ecxx,""),
 /*ED*/   GENx370x390x900 (execute_opcode_ed________xx,edxx,""),
 /*EE*/   GENx___x390x900 (perform_locked_operation,SS_RSRS,"PLO"),
 /*EF*/   GENx___x___x900 (load_multiple_disjoint,SS_R,"LMD"),
 /*F0*/   GENx370x390x900 (shift_and_round_decimal,SS_I,"SRP"),
 /*F1*/   GENx370x390x900 (move_with_offset,SS,"MVO"),
 /*F2*/   GENx370x390x900 (pack,SS,"PACK"),
 /*F3*/   GENx370x390x900 (unpack,SS,"UNPK"),
 /*F4*/   GENx___x___x___ ,
 /*F5*/   GENx___x___x___ ,
 /*F6*/   GENx___x___x___ ,
 /*F7*/   GENx___x___x___ ,
 /*F8*/   GENx370x390x900 (zero_and_add,SS,"ZAP"),
 /*F9*/   GENx370x390x900 (compare_decimal,SS,"CP"),
 /*FA*/   GENx370x390x900 (add_decimal,SS,"AP"),
 /*FB*/   GENx370x390x900 (subtract_decimal,SS,"SP"),
 /*FC*/   GENx370x390x900 (multiply_decimal,SS,"MP"),
 /*FD*/   GENx370x390x900 (divide_decimal,SS,"DP"),
 /*FE*/   GENx___x___x___ ,
 /*FF*/   GENx___x___x___  };


static zz_func opcode_01xx[0x100][GEN_MAXARCH] = {
 /*0100*/ GENx___x___x___ ,
 /*0101*/ GENx___x390x900 (program_return,E,"PR"),
 /*0102*/ GENx___x390x900 (update_tree,E,"UPT"),
 /*0103*/ GENx___x___x___ ,
 /*0104*/ GENx___x___x900 (perform_timing_facility_function,E,"PTFF"),
 /*0105*/ GENx___x___x___ , /*(clear_message,?,"CMSG"),*/
 /*0106*/ GENx___x___x___ , /*(test_message,?,"TMSG"),*/
 /*0107*/ GENx___x390x900 (set_clock_programmable_field,E,"SCKPF"),
 /*0108*/ GENx___x___x___ , /*(test_message_path_state,?,"TMPS"),*/
 /*0109*/ GENx___x___x___ , /*(clear_message_path_state,?,"CMPS"),*/
 /*010A*/ GENx___x390x900 (perform_floating_point_operation,E,"PFPO"),
 /*010B*/ GENx___x390x900 (test_addressing_mode,E,"TAM"),
 /*010C*/ GENx___x390x900 (set_addressing_mode_24,E,"SAM24"),
 /*010D*/ GENx___x390x900 (set_addressing_mode_31,E,"SAM31"),
 /*010E*/ GENx___x___x900 (set_addressing_mode_64,E,"SAM64"),
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
 /*01FF*/ GENx___x390x900 (trap2,E,"TRAP2") };


static zz_func opcode_a5_x[0x10][GEN_MAXARCH] = {
 /*A5x0*/ GENx___x___x900 (insert_immediate_high_high,RI,"IIHH"),
 /*A5x1*/ GENx___x___x900 (insert_immediate_high_low,RI,"IIHL"),
 /*A5x2*/ GENx___x___x900 (insert_immediate_low_high,RI,"IILH"),
 /*A5x3*/ GENx___x___x900 (insert_immediate_low_low,RI,"IILL"),
 /*A5x4*/ GENx___x___x900 (and_immediate_high_high,RI,"NIHH"),
 /*A5x5*/ GENx___x___x900 (and_immediate_high_low,RI,"NIHL"),
 /*A5x6*/ GENx___x___x900 (and_immediate_low_high,RI,"NILH"),
 /*A5x7*/ GENx___x___x900 (and_immediate_low_low,RI,"NILL"),
 /*A5x8*/ GENx___x___x900 (or_immediate_high_high,RI,"OIHH"),
 /*A5x9*/ GENx___x___x900 (or_immediate_high_low,RI,"OIHL"),
 /*A5xA*/ GENx___x___x900 (or_immediate_low_high,RI,"OILH"),
 /*A5xB*/ GENx___x___x900 (or_immediate_low_low,RI,"OILL"),
 /*A5xC*/ GENx___x___x900 (load_logical_immediate_high_high,RI,"LLIHH"),
 /*A5xD*/ GENx___x___x900 (load_logical_immediate_high_low,RI,"LLIHL"),
 /*A5xE*/ GENx___x___x900 (load_logical_immediate_low_high,RI,"LLILH"),
 /*A5xF*/ GENx___x___x900 (load_logical_immediate_low_low,RI,"LLILL") } ;


static zz_func opcode_a7_x[0x10][GEN_MAXARCH] = {
 /*A7x0*/ GENx37Xx390x900 (test_under_mask_high,RI,"TMLH"),
 /*A7x1*/ GENx37Xx390x900 (test_under_mask_low,RI,"TMLL"),
 /*A7x2*/ GENx___x___x900 (test_under_mask_high_high,RI,"TMHH"),
 /*A7x3*/ GENx___x___x900 (test_under_mask_high_low,RI,"TMHL"),
 /*A7x4*/ GENx37Xx390x900 (branch_relative_on_condition,RI_B,"BRC"),
 /*A7x5*/ GENx37Xx390x900 (branch_relative_and_save,RI_B,"BRAS"),
 /*A7x6*/ GENx37Xx390x900 (branch_relative_on_count,RI_B,"BRCT"),
 /*A7x7*/ GENx___x___x900 (branch_relative_on_count_long,RI_B,"BRCTG"),
 /*A7x8*/ GENx37Xx390x900 (load_halfword_immediate,RI,"LHI"),
 /*A7x9*/ GENx___x___x900 (load_long_halfword_immediate,RI,"LGHI"),
 /*A7xA*/ GENx37Xx390x900 (add_halfword_immediate,RI,"AHI"),
 /*A7xB*/ GENx___x___x900 (add_long_halfword_immediate,RI,"AGHI"),
 /*A7xC*/ GENx37Xx390x900 (multiply_halfword_immediate,RI,"MHI"),
 /*A7xD*/ GENx___x___x900 (multiply_long_halfword_immediate,RI,"MGHI"),
 /*A7xE*/ GENx37Xx390x900 (compare_halfword_immediate,RI,"CHI"),
 /*A7xF*/ GENx___x___x900 (compare_long_halfword_immediate,RI,"CGHI") };


static zz_func opcode_b2xx[0x100][GEN_MAXARCH] = {
 /*B200*/ GENx370x390x900 (connect_channel_set,S,"CONCS"),
 /*B201*/ GENx370x390x900 (disconnect_channel_set,S,"DISCS"),
 /*B202*/ GENx370x390x900 (store_cpu_id,S,"STIDP"),
 /*B203*/ GENx370x390x900 (store_channel_id,S,"STIDC"),
 /*B204*/ GENx370x390x900 (set_clock,S,"SCK"),
 /*B205*/ GENx370x390x900 (store_clock,S,"STCK"),
 /*B206*/ GENx370x390x900 (set_clock_comparator,S,"SCKC"),
 /*B207*/ GENx370x390x900 (store_clock_comparator,S,"STCKC"),
 /*B208*/ GENx370x390x900 (set_cpu_timer,S,"SPT"),
 /*B209*/ GENx370x390x900 (store_cpu_timer,S,"STPT"),
 /*B20A*/ GENx370x390x900 (set_psw_key_from_address,S,"SPKA"),
 /*B20B*/ GENx370x390x900 (insert_psw_key,none,"IPK"),
 /*B20C*/ GENx___x___x___ ,
 /*B20D*/ GENx370x390x900 (purge_translation_lookaside_buffer,none,"PTLB"),
 /*B20E*/ GENx___x___x___ ,
 /*B20F*/ GENx___x___x___ ,
 /*B210*/ GENx370x390x900 (set_prefix,S,"SPX"),
 /*B211*/ GENx370x390x900 (store_prefix,S,"STPX"),
 /*B212*/ GENx370x390x900 (store_cpu_address,S,"STAP"),
 /*B213*/ GENx370x___x___ (reset_reference_bit,S,"RRB"),
 /*B214*/ GENx___x390x900 (start_interpretive_execution,S,"SIE"),
 /*B215*/ GENx___x___x___ ,
 /*B216*/ GENx___x___x___ ,                                     /*%SETR/SSYN */
 /*B217*/ GENx___x___x___ ,                                   /*%STETR/STSYN */
 /*B218*/ GENx370x390x900 (program_call,S,"PC"),
 /*B219*/ GENx370x390x900 (set_address_space_control,S,"SAC"),
 /*B21A*/ GENx___x390x900 (compare_and_form_codeword,S,"CFC"),
 /*B21B*/ GENx___x___x___ ,
 /*B21C*/ GENx___x___x___ ,
 /*B21D*/ GENx___x___x___ ,
 /*B21E*/ GENx___x___x___ ,
 /*B21F*/ GENx___x___x___ ,
 /*B220*/ GENx___x390x900 (service_call,RRE,"SERVC"),
 /*B221*/ GENx370x390x900 (invalidate_page_table_entry,RRR,"IPTE"),
 /*B222*/ GENx370x390x900 (insert_program_mask,RRE_R1,"IPM"),
 /*B223*/ GENx370x390x900 (insert_virtual_storage_key,RRE,"IVSK"),
 /*B224*/ GENx370x390x900 (insert_address_space_control,RRE_R1,"IAC"),
 /*B225*/ GENx370x390x900 (set_secondary_asn,RRE_R1,"SSAR"),
 /*B226*/ GENx370x390x900 (extract_primary_asn,RRE_R1,"EPAR"),
 /*B227*/ GENx370x390x900 (extract_secondary_asn,RRE_R1,"ESAR"),
 /*B228*/ GENx370x390x900 (program_transfer,RRE,"PT"),
 /*B229*/ GENx370x390x900 (insert_storage_key_extended,RRE,"ISKE"),
 /*B22A*/ GENx370x390x900 (reset_reference_bit_extended,RRE,"RRBE"),
 /*B22B*/ GENx370x390x900 (set_storage_key_extended,RRF_M,"SSKE"),
 /*B22C*/ GENx370x390x900 (test_block,RRE,"TB"),
 /*B22D*/ GENx370x390x900 (divide_float_ext_reg,RRE,"DXR"),
 /*B22E*/ GENx___x390x900 (page_in,RRE,"PGIN"),
 /*B22F*/ GENx___x390x900 (page_out,RRE,"PGOUT"),
 /*B230*/ GENx___x390x900 (clear_subchannel,none,"CSCH"),
 /*B231*/ GENx___x390x900 (halt_subchannel,none,"HSCH"),
 /*B232*/ GENx___x390x900 (modify_subchannel,S,"MSCH"),
 /*B233*/ GENx___x390x900 (start_subchannel,S,"SSCH"),
 /*B234*/ GENx___x390x900 (store_subchannel,S,"STSCH"),
 /*B235*/ GENx___x390x900 (test_subchannel,S,"TSCH"),
 /*B236*/ GENx___x390x900 (test_pending_interruption,S,"TPI"),
 /*B237*/ GENx___x390x900 (set_address_limit,none,"SAL"),
 /*B238*/ GENx___x390x900 (resume_subchannel,none,"RSCH"),
 /*B239*/ GENx___x390x900 (store_channel_report_word,S,"STCRW"),
 /*B23A*/ GENx___x390x900 (store_channel_path_status,S,"STCPS"),
 /*B23B*/ GENx___x390x900 (reset_channel_path,none,"RCHP"),
 /*B23C*/ GENx___x390x900 (set_channel_monitor,none,"SCHM"),
 /*B23D*/ GENx___x390x900 (store_zone_parameter,S,"STZP"),
 /*B23E*/ GENx___x390x900 (set_zone_parameter,S,"SZP"),
 /*B23F*/ GENx___x390x900 (test_pending_zone_interrupt,S,"TPZI"),
 /*B240*/ GENx___x390x900 (branch_and_stack,RRE,"BAKR"),
 /*B241*/ GENx37Xx390x900 (checksum,RRE,"CKSM"),
 /*B242*/ GENx___x___x___ ,                                     /**Add FRR   */
 /*B243*/ GENx___x___x___ ,                                     /*#MA        */
 /*B244*/ GENx37Xx390x900 (squareroot_float_long_reg,RRE,"SQDR"),
 /*B245*/ GENx37Xx390x900 (squareroot_float_short_reg,RRE,"SQER"),
 /*B246*/ GENx___x390x900 (store_using_real_address,RRE,"STURA"),
 /*B247*/ GENx___x390x900 (modify_stacked_state,RRE_R1,"MSTA"),
 /*B248*/ GENx___x390x900 (purge_accesslist_lookaside_buffer,none,"PALB"),
 /*B249*/ GENx___x390x900 (extract_stacked_registers,RRE,"EREG"),
 /*B24A*/ GENx___x390x900 (extract_stacked_state,RRE,"ESTA"),
 /*B24B*/ GENx___x390x900 (load_using_real_address,RRE,"LURA"),
 /*B24C*/ GENx___x390x900 (test_access,RRE,"TAR"),
 /*B24D*/ GENx___x390x900 (copy_access,RRE,"CPYA"),
 /*B24E*/ GENx___x390x900 (set_access_register,RRE,"SAR"),
 /*B24F*/ GENx___x390x900 (extract_access_register,RRE,"EAR"),
 /*B250*/ GENx___x390x900 (compare_and_swap_and_purge,RRE,"CSP"),
 /*B251*/ GENx___x___x___ ,
 /*B252*/ GENx37Xx390x900 (multiply_single_register,RRE,"MSR"),
 /*B253*/ GENx___x___x___ ,
 /*B254*/ GENx___x390x900 (move_page,RRE,"MVPG"),
 /*B255*/ GENx37Xx390x900 (move_string,RRE,"MVST"),
 /*B256*/ GENx___x___x___ ,
 /*B257*/ GENx37Xx390x900 (compare_until_substring_equal,RRE,"CUSE"),
 /*B258*/ GENx___x390x900 (branch_in_subspace_group,RRE,"BSG"),
 /*B259*/ GENx___x390x900 (invalidate_expanded_storage_block_entry,RRE,"IESBE"),
 /*B25A*/ GENx___x390x900 (branch_and_set_authority,RRE,"BSA"),
 /*B25B*/ GENx___x___x___ ,                                     /*%PGXIN     */
 /*B25C*/ GENx___x___x___ ,                                     /*%PGXOUT    */
 /*B25D*/ GENx37Xx390x900 (compare_logical_string,RRE,"CLST"),
 /*B25E*/ GENx37Xx390x900 (search_string,RRE,"SRST"),
 /*B25F*/ GENx___x390x900 (channel_subsystem_call,RRE,"CHSC"),
 /*B260*/ GENx___x___x___ ,                                     /* Sysplex   */
 /*B261*/ GENx___x___x___ ,                                     /* Sysplex   */
 /*B262*/ GENx___x390x900 (lock_page,RRE,"LKPG"),
 /*B263*/ GENx37Xx390x900 (cmpsc_2012,RRE,"CMPSC"),
 /*B264*/ GENx___x___x___ ,                                     /* Sysplex   */
 /*B265*/ GENx___x___x900 (set_vector_summary,RRE,"SVS"),    /*           */
 /*B266*/ GENx___x___x___ ,                                     /* Sysplex   */
 /*B267*/ GENx___x___x___ ,                                     /* Sysplex   */
 /*B268*/ GENx___x___x___ , /*(define_vector,?,"DV"),*/         /* Sysplex   */
 /*B269*/ GENx___x___x___ ,                                     /* Crypto    */
 /*B26A*/ GENx___x___x___ ,                                     /* Crypto    */
 /*B26B*/ GENx___x___x___ ,                                     /* Crypto    */
 /*B26C*/ GENx___x___x___ ,                                     /* Crypto    */
 /*B26D*/ GENx___x___x___ ,                                     /* Crypto    */
 /*B26E*/ GENx___x___x___ ,                                     /* Crypto    */
 /*B26F*/ GENx___x___x___ ,                                     /* Crypto    */
 /*B270*/ GENx___x___x___ ,                                     /*%SPCS      */
 /*B271*/ GENx___x___x___ ,                                     /*%STPCS     */
 /*B272*/ GENx___x___x___ ,                                     /* Sysplex   */
 /*B273*/ GENx___x___x___ ,
 /*B274*/ GENx___x390x900 (signal_adapter,S,"SIGA"),
 /*B275*/ GENx___x___x___ ,
 /*B276*/ GENx___x390x900 (cancel_subchannel,none,"XSCH"),
 /*B277*/ GENx___x390x900 (resume_program,S,"RP"),
 /*B278*/ GENx___x390x900 (store_clock_extended,S,"STCKE"),
 /*B279*/ GENx___x390x900 (set_address_space_control_fast,S,"SACF"),
 /*B27A*/ GENx___x___x___ ,                                     /* Sysplex   */
 /*B27B*/ GENx___x___x___ ,                                     /* TFF/Sysplx*/
 /*B27C*/ GENx___x___x900 (store_clock_fast,S,"STCKF"),
 /*B27D*/ GENx370x390x900 (store_system_information,S,"STSI"),
 /*B27E*/ GENx___x___x___ ,                                     /* Sysplex   */
 /*B27F*/ GENx___x___x___ ,                                     /* Sysplex   */
 /*B280*/ GENx___x___x900 (load_program_parameter,S,"LPP"),     /* LPPF     */
 /*B281*/ GENx___x___x___ ,                                     /*#LN S      */
 /*B282*/ GENx___x___x___ ,                                     /*#EXP L     */
 /*B283*/ GENx___x___x___ ,                                     /*#EXP S     */
 /*B284*/ GENx___x___x900 (load_cpu_counter_set_controls,S,"LCCTL"),        /*  CMCF */
 /*B285*/ GENx___x___x900 (load_peripheral_counter_set_controls,S,"LPCTL"), /*  CMCF */
 /*B286*/ GENx___x___x900 (query_sampling_information,S,"QSI"),             /*  CMCF */
 /*B287*/ GENx___x___x900 (load_sampling_controls,S,"LSCTL"),               /*  CMCF */
 /*B288*/ GENx___x___x___ ,                                     /*#SIN L     */
 /*B289*/ GENx___x___x___ ,                                     /*#SIN S     */
 /*B28A*/ GENx___x___x___ ,                                     /*#COS L     */
 /*B28B*/ GENx___x___x___ ,                                     /*#COS S     */
 /*B28C*/ GENx___x___x___ ,
 /*B28D*/ GENx___x___x___ ,
 /*B28E*/ GENx___x___x900 (query_counter_information,S,"QCTRI"),            /*  CMCF */
 /*B28F*/ GENx___x___x___ ,
 /*B290*/ GENx___x___x___ ,
 /*B291*/ GENx___x___x___ ,
 /*B292*/ GENx___x___x___ ,
 /*B293*/ GENx___x___x___ ,
 /*B294*/ GENx___x___x___ ,                                     /*#ARCTAN L  */
 /*B295*/ GENx___x___x___ ,                                     /*#ARCTAN S  */
 /*B296*/ GENx___x___x___ ,
 /*B297*/ GENx___x___x___ ,
 /*B298*/ GENx___x___x___ ,
 /*B299*/ GENx37Xx390x900 (set_bfp_rounding_mode_2bit,S,"SRNM"),
 /*B29A*/ GENx___x___x___ ,
 /*B29B*/ GENx___x___x___ ,
 /*B29C*/ GENx37Xx390x900 (store_fpc,S,"STFPC"),
 /*B29D*/ GENx37Xx390x900 (load_fpc,S,"LFPC"),
 /*B29E*/ GENx___x___x___ ,
 /*B29F*/ GENx___x___x___ ,
 /*B2A0*/ GENx___x___x___ ,
 /*B2A1*/ GENx___x___x___ ,
 /*B2A2*/ GENx___x___x___ ,
 /*B2A3*/ GENx___x___x___ ,
 /*B2A4*/ GENx___x___x___ , /*(move_channel_buffer_data_multiple,?,"MCBDM"),*//*Sysplex*/
 /*B2A5*/ GENx37Xx390x900 (translate_extended,RRE,"TRE"),
 /*B2A6*/ GENx37Xx390x900 (convert_utf16_to_utf8,RRF_M3,"CU21 (CUUTF)"),
 /*B2A7*/ GENx37Xx390x900 (convert_utf8_to_utf16,RRF_M3,"CU12 (CUTFU)"),
 /*B2A8*/ GENx___x___x___ ,                                     /* Sysplex   */
 /*B2A9*/ GENx___x___x___ ,
 /*B2AA*/ GENx___x___x___ ,
 /*B2AB*/ GENx___x___x___ ,
 /*B2AC*/ GENx___x___x___ ,
 /*B2AD*/ GENx___x___x___ ,
 /*B2AE*/ GENx___x___x___ ,
 /*B2AF*/ GENx___x___x___ ,
 /*B2B0*/ GENx___x390x900 (store_facility_list_extended,S,"STFLE"), /*!SARCH */    /*@Z9*/
 /*B2B1*/ GENx___x390x900 (store_facility_list,S,"STFL"),
 /*B2B2*/ GENx___x___x900 (load_program_status_word_extended,S,"LPSWE"),
 /*B2B3*/ GENx___x___x___ , /*(store_etr_attachment_information,?,"STEAI"),*/
 /*B2B4*/ GENx___x___x___ ,
 /*B2B5*/ GENx___x___x___ ,
 /*B2B6*/ GENx___x___x___ ,
 /*B2B7*/ GENx___x___x___ ,
 /*B2B8*/ GENx37Xx390x900 (set_bfp_rounding_mode_3bit,S,"SRNMB"),                  /*810*/
 /*B2B9*/ GENx___x390x900 (set_dfp_rounding_mode,S,"SRNMT"),
 /*B2BA*/ GENx___x___x___ ,
 /*B2BB*/ GENx___x___x___ ,
 /*B2BC*/ GENx___x___x___ ,
 /*B2BD*/ GENx37Xx390x900 (load_fpc_and_signal,S,"LFAS"),
 /*B2BE*/ GENx___x___x___ ,
 /*B2BF*/ GENx___x___x___ ,
 /*B2C0*/ GENx___x___x___ ,                                     /*$ADRN      */
 /*B2C1*/ GENx___x___x___ ,                                     /*$AERN      */
 /*B2C2*/ GENx___x___x___ ,                                     /*$SDRN      */
 /*B2C3*/ GENx___x___x___ ,                                     /*$SERN      */
 /*B2C4*/ GENx___x___x___ ,                                     /*$MDRN      */
 /*B2C5*/ GENx___x___x___ ,                                     /*$MERN      */
 /*B2C6*/ GENx___x___x___ ,                                     /*$DDRN      */
 /*B2C7*/ GENx___x___x___ ,                                     /*$DERN      */
 /*B2C8*/ GENx___x___x___ ,                                     /*$LERN      */
 /*B2C9*/ GENx___x___x___ ,
 /*B2CA*/ GENx___x___x___ ,
 /*B2CB*/ GENx___x___x___ ,
 /*B2CC*/ GENx___x___x___ ,
 /*B2CD*/ GENx___x___x___ ,
 /*B2CE*/ GENx___x___x___ ,
 /*B2CF*/ GENx___x___x___ ,
 /*B2D0*/ GENx___x___x___ ,                                     /*$AACDR     */
 /*B2D1*/ GENx___x___x___ ,                                     /*$AACER     */
 /*B2D2*/ GENx___x___x___ ,                                     /*$SACDR     */
 /*B2D3*/ GENx___x___x___ ,                                     /*$SACER     */
 /*B2D4*/ GENx___x___x___ ,                                     /*$MACD      */
 /*B2D5*/ GENx___x___x___ ,
 /*B2D6*/ GENx___x___x___ ,                                     /*$RACD      */
 /*B2D7*/ GENx___x___x___ ,                                     /*$RACE      */
 /*B2D8*/ GENx___x___x___ ,                                     /*$AACAC     */
 /*B2D9*/ GENx___x___x___ ,                                     /*$SACAC     */
 /*B2DA*/ GENx___x___x___ ,                                     /*$CLAC      */
 /*B2DB*/ GENx___x___x___ ,
 /*B2DC*/ GENx___x___x___ ,
 /*B2DD*/ GENx___x___x___ ,
 /*B2DE*/ GENx___x___x___ ,
 /*B2DF*/ GENx___x___x___ ,
 /*B2E0*/ GENx___x___x900 (set_cpu_counter,RRE,"SCCTR"),                /*  CMCF */
 /*B2E1*/ GENx___x___x900 (set_peripheral_counter,RRE,"SPCTR"),         /*  CMCF */
 /*B2E2*/ GENx___x___x___ ,
 /*B2E3*/ GENx___x___x___ ,
 /*B2E4*/ GENx___x___x900 (extract_cpu_counter,RRE,"ECCTR"),            /*  CMCF */
 /*B2E5*/ GENx___x___x900 (extract_peripheral_counter,RRE,"EPCTR"),     /*  CMCF */
 /*B2E6*/ GENx___x___x___ ,
 /*B2E7*/ GENx___x___x___ ,
 /*B2E8*/ GENx___x___x___ ,
 /*B2E9*/ GENx___x___x___ ,
 /*B2EA*/ GENx___x___x___ ,
 /*B2EB*/ GENx___x___x___ ,
 /*B2EC*/ GENx___x___x___ ,
 /*B2ED*/ GENx___x___x900 (extract_coprocessor_group_address,RRE,"ECPGA"), /*  CMCF */
 /*B2EE*/ GENx___x___x___ ,
 /*B2EF*/ GENx___x___x___ ,
 /*B2F0*/ GENx370x390x900 (inter_user_communication_vehicle,S,"IUCV"),
 /*B2F1*/ GENx___x___x___ ,                                     /* Sysplex   */
 /*B2F2*/ GENx___x___x___ ,
 /*B2F3*/ GENx___x___x___ ,
 /*B2F4*/ GENx___x___x___ ,
 /*B2F5*/ GENx___x___x___ ,
 /*B2F6*/ GENx___x___x___ ,                                     /* Sysplex   */
 /*B2F7*/ GENx___x___x___ ,
 /*B2F8*/ GENx___x___x___ ,
 /*B2F9*/ GENx___x___x___ ,
 /*B2FA*/ GENx___x___x___ ,
 /*B2FB*/ GENx___x___x___ ,
 /*B2FC*/ GENx___x___x___ ,
 /*B2FD*/ GENx___x___x___ ,
 /*B2FE*/ GENx___x___x___ ,
 /*B2FF*/ GENx___x390x900 (trap4,S,"TRAP4") };


static zz_func opcode_b3xx[0x100][GEN_MAXARCH] = {
 /*B300*/ GENx37Xx390x900 (load_positive_bfp_short_reg,RRE,"LPEBR"),
 /*B301*/ GENx37Xx390x900 (load_negative_bfp_short_reg,RRE,"LNEBR"),
 /*B302*/ GENx37Xx390x900 (load_and_test_bfp_short_reg,RRE,"LTEBR"),
 /*B303*/ GENx37Xx390x900 (load_complement_bfp_short_reg,RRE,"LCEBR"),
 /*B304*/ GENx37Xx390x900 (load_lengthened_bfp_short_to_long_reg,RRE,"LDEBR"),
 /*B305*/ GENx37Xx390x900 (load_lengthened_bfp_long_to_ext_reg,RRE,"LXDBR"),
 /*B306*/ GENx37Xx390x900 (load_lengthened_bfp_short_to_ext_reg,RRE,"LXEBR"),
 /*B307*/ GENx37Xx390x900 (multiply_bfp_long_to_ext_reg,RRE,"MXDBR"),
 /*B308*/ GENx37Xx390x900 (compare_and_signal_bfp_short_reg,RRE,"KEBR"),
 /*B309*/ GENx37Xx390x900 (compare_bfp_short_reg,RRE,"CEBR"),
 /*B30A*/ GENx37Xx390x900 (add_bfp_short_reg,RRE,"AEBR"),
 /*B30B*/ GENx37Xx390x900 (subtract_bfp_short_reg,RRE,"SEBR"),
 /*B30C*/ GENx37Xx390x900 (multiply_bfp_short_to_long_reg,RRE,"MDEBR"),
 /*B30D*/ GENx37Xx390x900 (divide_bfp_short_reg,RRE,"DEBR"),
 /*B30E*/ GENx37Xx390x900 (multiply_add_bfp_short_reg,RRF_R,"MAEBR"),
 /*B30F*/ GENx37Xx390x900 (multiply_subtract_bfp_short_reg,RRF_R,"MSEBR"),
 /*B310*/ GENx37Xx390x900 (load_positive_bfp_long_reg,RRE,"LPDBR"),
 /*B311*/ GENx37Xx390x900 (load_negative_bfp_long_reg,RRE,"LNDBR"),
 /*B312*/ GENx37Xx390x900 (load_and_test_bfp_long_reg,RRE,"LTDBR"),
 /*B313*/ GENx37Xx390x900 (load_complement_bfp_long_reg,RRE,"LCDBR"),
 /*B314*/ GENx37Xx390x900 (squareroot_bfp_short_reg,RRE,"SQEBR"),
 /*B315*/ GENx37Xx390x900 (squareroot_bfp_long_reg,RRE,"SQDBR"),
 /*B316*/ GENx37Xx390x900 (squareroot_bfp_ext_reg,RRE,"SQXBR"),
 /*B317*/ GENx37Xx390x900 (multiply_bfp_short_reg,RRE,"MEEBR"),
 /*B318*/ GENx37Xx390x900 (compare_and_signal_bfp_long_reg,RRE,"KDBR"),
 /*B319*/ GENx37Xx390x900 (compare_bfp_long_reg,RRE,"CDBR"),
 /*B31A*/ GENx37Xx390x900 (add_bfp_long_reg,RRE,"ADBR"),
 /*B31B*/ GENx37Xx390x900 (subtract_bfp_long_reg,RRE,"SDBR"),
 /*B31C*/ GENx37Xx390x900 (multiply_bfp_long_reg,RRE,"MDBR"),
 /*B31D*/ GENx37Xx390x900 (divide_bfp_long_reg,RRE,"DDBR"),
 /*B31E*/ GENx37Xx390x900 (multiply_add_bfp_long_reg,RRF_R,"MADBR"),
 /*B31F*/ GENx37Xx390x900 (multiply_subtract_bfp_long_reg,RRF_R,"MSDBR"),
 /*B320*/ GENx___x___x___ ,
 /*B321*/ GENx___x___x___ ,
 /*B322*/ GENx___x___x___ ,
 /*B323*/ GENx___x___x___ ,
 /*B324*/ GENx37Xx390x900 (load_lengthened_float_short_to_long_reg,RRE,"LDER"),
 /*B325*/ GENx37Xx390x900 (load_lengthened_float_long_to_ext_reg,RRE,"LXDR"),
 /*B326*/ GENx37Xx390x900 (load_lengthened_float_short_to_ext_reg,RRE,"LXER"),
 /*B327*/ GENx___x___x___ ,
 /*B328*/ GENx___x___x___ ,
 /*B329*/ GENx___x___x___ ,
 /*B32A*/ GENx___x___x___ ,
 /*B32B*/ GENx___x___x___ ,
 /*B32C*/ GENx___x___x___ ,
 /*B32D*/ GENx___x___x___ ,
 /*B32E*/ GENx37Xx390x900 (multiply_add_float_short_reg,RRF_R,"MAER"),
 /*B32F*/ GENx37Xx390x900 (multiply_subtract_float_short_reg,RRF_R,"MSER"),
 /*B330*/ GENx___x___x___ ,
 /*B331*/ GENx___x___x___ ,
 /*B332*/ GENx___x___x___ ,
 /*B333*/ GENx___x___x___ ,
 /*B334*/ GENx___x___x___ ,
 /*B335*/ GENx___x___x___ ,
 /*B336*/ GENx37Xx390x900 (squareroot_float_ext_reg,RRE,"SQXR"),
 /*B337*/ GENx37Xx390x900 (multiply_float_short_reg,RRE,"MEER"),
 /*B338*/ GENx37Xx___x900 (multiply_add_unnormal_float_long_to_ext_low_reg,RRF_R,"MAYLR"),  /*@Z9*/
 /*B339*/ GENx37Xx___x900 (multiply_unnormal_float_long_to_ext_low_reg,RRF_R,"MYLR"),       /*@Z9*/
 /*B33A*/ GENx37Xx___x900 (multiply_add_unnormal_float_long_to_ext_reg,RRF_R,"MAYR"),       /*@Z9*/
 /*B33B*/ GENx37Xx___x900 (multiply_unnormal_float_long_to_ext_reg,RRF_R,"MYR"),            /*@Z9*/
 /*B33C*/ GENx37Xx___x900 (multiply_add_unnormal_float_long_to_ext_high_reg,RRF_R,"MAYHR"), /*@Z9*/
 /*B33D*/ GENx37Xx___x900 (multiply_unnormal_float_long_to_ext_high_reg,RRF_R,"MYHR"),      /*@Z9*/
 /*B33E*/ GENx37Xx390x900 (multiply_add_float_long_reg,RRF_R,"MADR"),
 /*B33F*/ GENx37Xx390x900 (multiply_subtract_float_long_reg,RRF_R,"MSDR"),
 /*B340*/ GENx37Xx390x900 (load_positive_bfp_ext_reg,RRE,"LPXBR"),
 /*B341*/ GENx37Xx390x900 (load_negative_bfp_ext_reg,RRE,"LNXBR"),
 /*B342*/ GENx37Xx390x900 (load_and_test_bfp_ext_reg,RRE,"LTXBR"),
 /*B343*/ GENx37Xx390x900 (load_complement_bfp_ext_reg,RRE,"LCXBR"),
 /*B344*/ GENx37Xx390x900 (load_rounded_bfp_long_to_short_reg,RRE,"LEDBR"),
 /*B345*/ GENx37Xx390x900 (load_rounded_bfp_ext_to_long_reg,RRE,"LDXBR"),
 /*B346*/ GENx37Xx390x900 (load_rounded_bfp_ext_to_short_reg,RRE,"LEXBR"),
 /*B347*/ GENx37Xx390x900 (load_fp_int_bfp_ext_reg,RRF_M,"FIXBR"),
 /*B348*/ GENx37Xx390x900 (compare_and_signal_bfp_ext_reg,RRE,"KXBR"),
 /*B349*/ GENx37Xx390x900 (compare_bfp_ext_reg,RRE,"CXBR"),
 /*B34A*/ GENx37Xx390x900 (add_bfp_ext_reg,RRE,"AXBR"),
 /*B34B*/ GENx37Xx390x900 (subtract_bfp_ext_reg,RRE,"SXBR"),
 /*B34C*/ GENx37Xx390x900 (multiply_bfp_ext_reg,RRE,"MXBR"),
 /*B34D*/ GENx37Xx390x900 (divide_bfp_ext_reg,RRE,"DXBR"),
 /*B34E*/ GENx___x___x___ ,
 /*B34F*/ GENx___x___x___ ,
 /*B350*/ GENx37Xx390x900 (convert_float_long_to_bfp_short_reg,RRF_M,"TBEDR"),
 /*B351*/ GENx37Xx390x900 (convert_float_long_to_bfp_long_reg,RRF_M,"TBDR"),
 /*B352*/ GENx___x___x___ ,
 /*B353*/ GENx37Xx390x900 (divide_integer_bfp_short_reg,RRF_RM,"DIEBR"),
 /*B354*/ GENx___x___x___ ,
 /*B355*/ GENx___x___x___ ,
 /*B356*/ GENx___x___x___ ,
 /*B357*/ GENx37Xx390x900 (load_fp_int_bfp_short_reg,RRF_M,"FIEBR"),
 /*B358*/ GENx37Xx390x900 (convert_bfp_short_to_float_long_reg,RRE,"THDER"),
 /*B359*/ GENx37Xx390x900 (convert_bfp_long_to_float_long_reg,RRE,"THDR"),
 /*B35A*/ GENx___x___x___ ,
 /*B35B*/ GENx37Xx390x900 (divide_integer_bfp_long_reg,RRF_RM,"DIDBR"),
 /*B35C*/ GENx___x___x___ ,
 /*B35D*/ GENx___x___x___ ,
 /*B35E*/ GENx___x___x___ ,
 /*B35F*/ GENx37Xx390x900 (load_fp_int_bfp_long_reg,RRF_M,"FIDBR"),
 /*B360*/ GENx37Xx390x900 (load_positive_float_ext_reg,RRE,"LPXR"),
 /*B361*/ GENx37Xx390x900 (load_negative_float_ext_reg,RRE,"LNXR"),
 /*B362*/ GENx37Xx390x900 (load_and_test_float_ext_reg,RRE,"LTXR"),
 /*B363*/ GENx37Xx390x900 (load_complement_float_ext_reg,RRE,"LCXR"),
 /*B364*/ GENx___x___x___ ,
 /*B365*/ GENx37Xx390x900 (load_float_ext_reg,RRE,"LXR"),
 /*B366*/ GENx37Xx390x900 (load_rounded_float_ext_to_short_reg,RRE,"LEXR"),
 /*B367*/ GENx37Xx390x900 (load_fp_int_float_ext_reg,RRE,"FIXR"),
 /*B368*/ GENx___x___x___ ,
 /*B369*/ GENx37Xx390x900 (compare_float_ext_reg,RRE,"CXR"),
 /*B36A*/ GENx___x___x___ ,
 /*B36B*/ GENx___x___x___ ,
 /*B36C*/ GENx___x___x___ ,
 /*B36D*/ GENx___x___x___ ,
 /*B36E*/ GENx___x___x___ ,
 /*B36F*/ GENx___x___x___ ,
 /*B370*/ GENx37Xx390x900 (load_positive_fpr_long_reg,RRE,"LPDFR"),
 /*B371*/ GENx37Xx390x900 (load_negative_fpr_long_reg,RRE,"LNDFR"),
 /*B372*/ GENx37Xx390x900 (copy_sign_fpr_long_reg,RRF_M,"CPSDR"),
 /*B373*/ GENx37Xx390x900 (load_complement_fpr_long_reg,RRE,"LCDFR"),
 /*B374*/ GENx37Xx390x900 (load_zero_float_short_reg,RRE_R1,"LZER"),
 /*B375*/ GENx37Xx390x900 (load_zero_float_long_reg,RRE_R1,"LZDR"),
 /*B376*/ GENx37Xx390x900 (load_zero_float_ext_reg,RRE_R1,"LZXR"),
 /*B377*/ GENx37Xx390x900 (load_fp_int_float_short_reg,RRE,"FIER"),
 /*B378*/ GENx___x___x___ ,
 /*B379*/ GENx___x___x___ ,
 /*B37A*/ GENx___x___x___ ,
 /*B37B*/ GENx___x___x___ ,
 /*B37C*/ GENx___x___x___ ,
 /*B37D*/ GENx___x___x___ ,
 /*B37E*/ GENx___x___x___ ,
 /*B37F*/ GENx37Xx390x900 (load_fp_int_float_long_reg,RRE,"FIDR"),
 /*B380*/ GENx___x___x___ ,
 /*B381*/ GENx___x___x___ ,
 /*B382*/ GENx___x___x___ ,
 /*B383*/ GENx___x___x___ ,
 /*B384*/ GENx37Xx390x900 (set_fpc,RRE_R1,"SFPC"),
 /*B385*/ GENx37Xx390x900 (set_fpc_and_signal,RRE_R1,"SFASR"),
 /*B386*/ GENx___x___x___ ,
 /*B387*/ GENx___x___x___ ,
 /*B388*/ GENx___x___x___ ,
 /*B389*/ GENx___x___x___ ,
 /*B38A*/ GENx___x___x___ ,
 /*B38B*/ GENx___x___x___ ,
 /*B38C*/ GENx37Xx390x900 (extract_fpc,RRE_R1,"EFPC"),
 /*B38D*/ GENx___x___x___ ,
 /*B38E*/ GENx___x___x___ ,
 /*B38F*/ GENx___x___x___ ,
 /*B390*/ GENx37Xx390x900 (convert_u32_to_bfp_short_reg,RRF_MM,"CELFBR"),          /*810*/
 /*B391*/ GENx37Xx390x900 (convert_u32_to_bfp_long_reg,RRF_MM,"CDLFBR"),           /*810*/
 /*B392*/ GENx37Xx390x900 (convert_u32_to_bfp_ext_reg,RRF_MM,"CXLFBR"),            /*810*/
 /*B393*/ GENx___x___x___ ,
 /*B394*/ GENx37Xx390x900 (convert_fix32_to_bfp_short_reg,RRE,"CEFBR"),
 /*B395*/ GENx37Xx390x900 (convert_fix32_to_bfp_long_reg,RRE,"CDFBR"),
 /*B396*/ GENx37Xx390x900 (convert_fix32_to_bfp_ext_reg,RRE,"CXFBR"),
 /*B397*/ GENx___x___x___ ,
 /*B398*/ GENx37Xx390x900 (convert_bfp_short_to_fix32_reg,RRF_M,"CFEBR"),
 /*B399*/ GENx37Xx390x900 (convert_bfp_long_to_fix32_reg,RRF_M,"CFDBR"),
 /*B39A*/ GENx37Xx390x900 (convert_bfp_ext_to_fix32_reg,RRF_M,"CFXBR"),
 /*B39B*/ GENx___x___x___ ,
 /*B39C*/ GENx37Xx390x900 (convert_bfp_short_to_u32_reg,RRF_MM,"CLFEBR"),          /*810*/
 /*B39D*/ GENx37Xx390x900 (convert_bfp_long_to_u32_reg,RRF_MM,"CLFDBR"),           /*810*/
 /*B39E*/ GENx37Xx390x900 (convert_bfp_ext_to_u32_reg,RRF_MM,"CLFXBR"),            /*810*/
 /*B39F*/ GENx___x___x___ ,
 /*B3A0*/ GENx___x___x900 (convert_u64_to_bfp_short_reg,RRF_MM,"CELGBR"),          /*810*/
 /*B3A1*/ GENx___x___x900 (convert_u64_to_bfp_long_reg,RRF_MM,"CDLGBR"),           /*810*/
 /*B3A2*/ GENx___x___x900 (convert_u64_to_bfp_ext_reg,RRF_MM,"CXLGBR"),            /*810*/
 /*B3A3*/ GENx___x___x___ ,
 /*B3A4*/ GENx___x___x900 (convert_fix64_to_bfp_short_reg,RRE,"CEGBR"),
 /*B3A5*/ GENx___x___x900 (convert_fix64_to_bfp_long_reg,RRE,"CDGBR"),
 /*B3A6*/ GENx___x___x900 (convert_fix64_to_bfp_ext_reg,RRE,"CXGBR"),
 /*B3A7*/ GENx___x___x___ ,
 /*B3A8*/ GENx___x___x900 (convert_bfp_short_to_fix64_reg,RRF_M,"CGEBR"),
 /*B3A9*/ GENx___x___x900 (convert_bfp_long_to_fix64_reg,RRF_M,"CGDBR"),
 /*B3AA*/ GENx___x___x900 (convert_bfp_ext_to_fix64_reg,RRF_M,"CGXBR"),
 /*B3AB*/ GENx___x___x___ ,
 /*B3AC*/ GENx___x___x900 (convert_bfp_short_to_u64_reg,RRF_MM,"CLGEBR"),          /*810*/
 /*B3AD*/ GENx___x___x900 (convert_bfp_long_to_u64_reg,RRF_MM,"CLGDBR"),           /*810*/
 /*B3AE*/ GENx___x___x900 (convert_bfp_ext_to_u64_reg,RRF_MM,"CLGXBR"),            /*810*/
 /*B3AF*/ GENx___x___x___ ,
 /*B3B0*/ GENx___x___x___ ,
 /*B3B1*/ GENx___x___x___ ,
 /*B3B2*/ GENx___x___x___ ,
 /*B3B3*/ GENx___x___x___ ,
 /*B3B4*/ GENx37Xx390x900 (convert_fixed_to_float_short_reg,RRE,"CEFR"),
 /*B3B5*/ GENx37Xx390x900 (convert_fixed_to_float_long_reg,RRE,"CDFR"),
 /*B3B6*/ GENx37Xx390x900 (convert_fixed_to_float_ext_reg,RRE,"CXFR"),
 /*B3B7*/ GENx___x___x___ ,
 /*B3B8*/ GENx37Xx390x900 (convert_float_short_to_fixed_reg,RRF_M,"CFER"),
 /*B3B9*/ GENx37Xx390x900 (convert_float_long_to_fixed_reg,RRF_M,"CFDR"),
 /*B3BA*/ GENx37Xx390x900 (convert_float_ext_to_fixed_reg,RRF_M,"CFXR"),
 /*B3BB*/ GENx___x___x___ ,
 /*B3BC*/ GENx___x___x___ ,
 /*B3BD*/ GENx___x___x___ ,
 /*B3BE*/ GENx___x___x___ ,
 /*B3BF*/ GENx___x___x___ ,
 /*B3C0*/ GENx___x___x___ ,
 /*B3C1*/ GENx___x___x900 (load_fpr_from_gr_long_reg,RRE,"LDGR"),
 /*B3C2*/ GENx___x___x___ ,
 /*B3C3*/ GENx___x___x___ ,
 /*B3C4*/ GENx___x___x900 (convert_fix64_to_float_short_reg,RRE,"CEGR"),
 /*B3C5*/ GENx___x___x900 (convert_fix64_to_float_long_reg,RRE,"CDGR"),
 /*B3C6*/ GENx___x___x900 (convert_fix64_to_float_ext_reg,RRE,"CXGR"),
 /*B3C7*/ GENx___x___x___ ,
 /*B3C8*/ GENx___x___x900 (convert_float_short_to_fix64_reg,RRF_M,"CGER"),
 /*B3C9*/ GENx___x___x900 (convert_float_long_to_fix64_reg,RRF_M,"CGDR"),
 /*B3CA*/ GENx___x___x900 (convert_float_ext_to_fix64_reg,RRF_M,"CGXR"),
 /*B3CB*/ GENx___x___x___ ,
 /*B3CC*/ GENx___x___x___ ,
 /*B3CD*/ GENx___x___x900 (load_gr_from_fpr_long_reg,RRE,"LGDR"),
 /*B3CE*/ GENx___x___x___ ,
 /*B3CF*/ GENx___x___x___ ,
 /*B3D0*/ GENx___x390x900 (multiply_dfp_long_reg,RRR,"MDTR"),
 /*B3D1*/ GENx___x390x900 (divide_dfp_long_reg,RRR,"DDTR"),
 /*B3D2*/ GENx___x390x900 (add_dfp_long_reg,RRR,"ADTR"),
 /*B3D3*/ GENx___x390x900 (subtract_dfp_long_reg,RRR,"SDTR"),
 /*B3D4*/ GENx___x390x900 (load_lengthened_dfp_short_to_long_reg,RRF_M4,"LDETR"),
 /*B3D5*/ GENx___x390x900 (load_rounded_dfp_long_to_short_reg,RRF_MM,"LEDTR"),
 /*B3D6*/ GENx___x390x900 (load_and_test_dfp_long_reg,RRE,"LTDTR"),
 /*B3D7*/ GENx___x390x900 (load_fp_int_dfp_long_reg,RRF_MM,"FIDTR"),
 /*B3D8*/ GENx___x390x900 (multiply_dfp_ext_reg,RRR,"MXTR"),
 /*B3D9*/ GENx___x390x900 (divide_dfp_ext_reg,RRR,"DXTR"),
 /*B3DA*/ GENx___x390x900 (add_dfp_ext_reg,RRR,"AXTR"),
 /*B3DB*/ GENx___x390x900 (subtract_dfp_ext_reg,RRR,"SXTR"),
 /*B3DC*/ GENx___x390x900 (load_lengthened_dfp_long_to_ext_reg,RRF_M4,"LXDTR"),
 /*B3DD*/ GENx___x390x900 (load_rounded_dfp_ext_to_long_reg,RRF_MM,"LDXTR"),
 /*B3DE*/ GENx___x390x900 (load_and_test_dfp_ext_reg,RRE,"LTXTR"),
 /*B3DF*/ GENx___x390x900 (load_fp_int_dfp_ext_reg,RRF_MM,"FIXTR"),
 /*B3E0*/ GENx___x390x900 (compare_and_signal_dfp_long_reg,RRE,"KDTR"),
 /*B3E1*/ GENx___x390x900 (convert_dfp_long_to_fix64_reg,RRF_M,"CGDTR"),
 /*B3E2*/ GENx___x390x900 (convert_dfp_long_to_ubcd64_reg,RRE,"CUDTR"),
 /*B3E3*/ GENx___x390x900 (convert_dfp_long_to_sbcd64_reg,RRF_M4,"CSDTR"),
 /*B3E4*/ GENx___x390x900 (compare_dfp_long_reg,RRE,"CDTR"),
 /*B3E5*/ GENx___x390x900 (extract_biased_exponent_dfp_long_to_fix64_reg,RRE,"EEDTR"),
 /*B3E6*/ GENx___x___x___ ,
 /*B3E7*/ GENx___x390x900 (extract_significance_dfp_long_reg,RRE,"ESDTR"),
 /*B3E8*/ GENx___x390x900 (compare_and_signal_dfp_ext_reg,RRE,"KXTR"),
 /*B3E9*/ GENx___x390x900 (convert_dfp_ext_to_fix64_reg,RRF_M,"CGXTR"),
 /*B3EA*/ GENx___x390x900 (convert_dfp_ext_to_ubcd128_reg,RRE,"CUXTR"),
 /*B3EB*/ GENx___x390x900 (convert_dfp_ext_to_sbcd128_reg,RRF_M4,"CSXTR"),
 /*B3EC*/ GENx___x390x900 (compare_dfp_ext_reg,RRE,"CXTR"),
 /*B3ED*/ GENx___x390x900 (extract_biased_exponent_dfp_ext_to_fix64_reg,RRE,"EEXTR"),
 /*B3EE*/ GENx___x___x___ ,
 /*B3EF*/ GENx___x390x900 (extract_significance_dfp_ext_reg,RRE,"ESXTR"),
 /*B3F0*/ GENx___x___x___ ,
 /*B3F1*/ GENx___x390x900 (convert_fix64_to_dfp_long_reg,RRE,"CDGTR"),
 /*B3F2*/ GENx___x390x900 (convert_ubcd64_to_dfp_long_reg,RRE,"CDUTR"),
 /*B3F3*/ GENx___x390x900 (convert_sbcd64_to_dfp_long_reg,RRE,"CDSTR"),
 /*B3F4*/ GENx___x390x900 (compare_exponent_dfp_long_reg,RRE,"CEDTR"),
 /*B3F5*/ GENx___x390x900 (quantize_dfp_long_reg,RRF_RM,"QADTR"),
 /*B3F6*/ GENx___x390x900 (insert_biased_exponent_fix64_to_dfp_long_reg,RRF_M,"IEDTR"),
 /*B3F7*/ GENx___x390x900 (reround_dfp_long_reg,RRF_RM,"RRDTR"),
 /*B3F8*/ GENx___x___x___ ,
 /*B3F9*/ GENx___x390x900 (convert_fix64_to_dfp_ext_reg,RRE,"CXGTR"),
 /*B3FA*/ GENx___x390x900 (convert_ubcd128_to_dfp_ext_reg,RRE,"CXUTR"),
 /*B3FB*/ GENx___x390x900 (convert_sbcd128_to_dfp_ext_reg,RRE,"CXSTR"),
 /*B3FC*/ GENx___x390x900 (compare_exponent_dfp_ext_reg,RRE,"CEXTR"),
 /*B3FD*/ GENx___x390x900 (quantize_dfp_ext_reg,RRF_RM,"QAXTR"),
 /*B3FE*/ GENx___x390x900 (insert_biased_exponent_fix64_to_dfp_ext_reg,RRF_M,"IEXTR"),
 /*B3FF*/ GENx___x390x900 (reround_dfp_ext_reg,RRF_RM,"RRXTR") };


static zz_func opcode_b9xx[0x100][GEN_MAXARCH] = {
 /*B900*/ GENx___x___x900 (load_positive_long_register,RRE,"LPGR"),
 /*B901*/ GENx___x___x900 (load_negative_long_register,RRE,"LNGR"),
 /*B902*/ GENx___x___x900 (load_and_test_long_register,RRE,"LTGR"),
 /*B903*/ GENx___x___x900 (load_complement_long_register,RRE,"LCGR"),
 /*B904*/ GENx___x___x900 (load_long_register,RRE,"LGR"),
 /*B905*/ GENx___x___x900 (load_using_real_address_long,RRE,"LURAG"),
 /*B906*/ GENx___x___x900 (load_long_byte_register,RRE,"LGBR"),                    /*@Z9*/
 /*B907*/ GENx___x___x900 (load_long_halfword_register,RRE,"LGHR"),                /*@Z9*/
 /*B908*/ GENx___x___x900 (add_long_register,RRE,"AGR"),
 /*B909*/ GENx___x___x900 (subtract_long_register,RRE,"SGR"),
 /*B90A*/ GENx___x___x900 (add_logical_long_register,RRE,"ALGR"),
 /*B90B*/ GENx___x___x900 (subtract_logical_long_register,RRE,"SLGR"),
 /*B90C*/ GENx___x___x900 (multiply_single_long_register,RRE,"MSGR"),
 /*B90D*/ GENx___x___x900 (divide_single_long_register,RRE,"DSGR"),
 /*B90E*/ GENx___x___x900 (extract_stacked_registers_long,RRE,"EREGG"),
 /*B90F*/ GENx___x___x900 (load_reversed_long_register,RRE,"LRVGR"),
 /*B910*/ GENx___x___x900 (load_positive_long_fullword_register,RRE,"LPGFR"),
 /*B911*/ GENx___x___x900 (load_negative_long_fullword_register,RRE,"LNGFR"),
 /*B912*/ GENx___x___x900 (load_and_test_long_fullword_register,RRE,"LTGFR"),
 /*B913*/ GENx___x___x900 (load_complement_long_fullword_register,RRE,"LCGFR"),
 /*B914*/ GENx___x___x900 (load_long_fullword_register,RRE,"LGFR"),
 /*B915*/ GENx___x___x___ ,
 /*B916*/ GENx___x___x900 (load_logical_long_fullword_register,RRE,"LLGFR"),
 /*B917*/ GENx___x___x900 (load_logical_long_thirtyone_register,RRE,"LLGTR"),
 /*B918*/ GENx___x___x900 (add_long_fullword_register,RRE,"AGFR"),
 /*B919*/ GENx___x___x900 (subtract_long_fullword_register,RRE,"SGFR"),
 /*B91A*/ GENx___x___x900 (add_logical_long_fullword_register,RRE,"ALGFR"),
 /*B91B*/ GENx___x___x900 (subtract_logical_long_fullword_register,RRE,"SLGFR"),
 /*B91C*/ GENx___x___x900 (multiply_single_long_fullword_register,RRE,"MSGFR"),
 /*B91D*/ GENx___x___x900 (divide_single_long_fullword_register,RRE,"DSGFR"),
 /*B91E*/ GENx37Xx390x900 (compute_message_authentication_code,RRE,"KMAC"),
 /*B91F*/ GENx___x390x900 (load_reversed_register,RRE,"LRVR"),
 /*B920*/ GENx___x___x900 (compare_long_register,RRE,"CGR"),
 /*B921*/ GENx___x___x900 (compare_logical_long_register,RRE,"CLGR"),
 /*B922*/ GENx___x___x___ ,
 /*B923*/ GENx___x___x___ ,
 /*B924*/ GENx___x___x___ ,
 /*B925*/ GENx___x___x900 (store_using_real_address_long,RRE,"STURG"),
 /*B926*/ GENx37Xx390x900 (load_byte_register,RRE,"LBR"),                          /*@Z9*/
 /*B927*/ GENx37Xx390x900 (load_halfword_register,RRE,"LHR"),                      /*@Z9*/
 /*B928*/ GENx37Xx390x900 (perform_cryptographic_key_management_operation,RRE,"PCKMO"), /*810*/
 /*B929*/ GENx___x___x___ ,
 /*B92A*/ GENx37Xx390x900 (cipher_message_with_cipher_feedback,RRE,"KMF"),       /*810*/
 /*B92B*/ GENx37Xx390x900 (cipher_message_with_output_feedback,RRE,"KMO"),       /*810*/
 /*B92C*/ GENx37Xx390x900 (perform_cryptographic_computation,none,"PCC"),        /*810*/
 /*B92D*/ GENx37Xx390x900 (cipher_message_with_counter,RRF_M,"KMCTR"),           /*810*/
 /*B92E*/ GENx37Xx390x900 (cipher_message,RRE,"KM"),
 /*B92F*/ GENx37Xx390x900 (cipher_message_with_chaining,RRE,"KMC"),
 /*B930*/ GENx___x___x900 (compare_long_fullword_register,RRE,"CGFR"),
 /*B931*/ GENx___x___x900 (compare_logical_long_fullword_register,RRE,"CLGFR"),
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
 /*B93E*/ GENx37Xx390x900 (compute_intermediate_message_digest,RRE,"KIMD"),
 /*B93F*/ GENx37Xx390x900 (compute_last_message_digest,RRE,"KLMD"),
 /*B940*/ GENx___x___x___ ,
 /*B941*/ GENx___x390x900 (convert_dfp_long_to_fix32_reg,RRF_MM,"CFDTR"),          /*810*/
 /*B942*/ GENx___x___x900 (convert_dfp_long_to_u64_reg,RRF_MM,"CLGDTR"),           /*810*/
 /*B943*/ GENx___x390x900 (convert_dfp_long_to_u32_reg,RRF_MM,"CLFDTR"),           /*810*/
 /*B944*/ GENx___x___x___ ,
 /*B945*/ GENx___x___x___ ,
 /*B946*/ GENx___x___x900 (branch_on_count_long_register,RRE,"BCTGR"),
 /*B947*/ GENx___x___x___ ,
 /*B948*/ GENx___x___x___ ,
 /*B949*/ GENx___x390x900 (convert_dfp_ext_to_fix32_reg,RRF_MM,"CFXTR"),           /*810*/
 /*B94A*/ GENx___x___x900 (convert_dfp_ext_to_u64_reg,RRF_MM,"CLGXTR"),            /*810*/
 /*B94B*/ GENx___x390x900 (convert_dfp_ext_to_u32_reg,RRF_MM,"CLFXTR"),            /*810*/
 /*B94C*/ GENx___x___x___ ,
 /*B94D*/ GENx___x___x___ ,
 /*B94E*/ GENx___x___x___ ,
 /*B94F*/ GENx___x___x___ ,
 /*B950*/ GENx___x___x___ ,
 /*B951*/ GENx___x390x900 (convert_fix32_to_dfp_long_reg,RRF_MM,"CDFTR"),          /*810*/
 /*B952*/ GENx___x___x900 (convert_u64_to_dfp_long_reg,RRF_MM,"CDLGTR"),           /*810*/
 /*B953*/ GENx___x390x900 (convert_u32_to_dfp_long_reg,RRF_MM,"CDLFTR"),           /*810*/
 /*B954*/ GENx___x___x___ ,
 /*B955*/ GENx___x___x___ ,
 /*B956*/ GENx___x___x___ ,
 /*B957*/ GENx___x___x___ ,
 /*B958*/ GENx___x___x___ ,
 /*B959*/ GENx___x390x900 (convert_fix32_to_dfp_ext_reg,RRF_MM,"CXFTR"),           /*810*/
 /*B95A*/ GENx___x___x900 (convert_u64_to_dfp_ext_reg,RRF_MM,"CXLGTR"),            /*810*/
 /*B95B*/ GENx___x390x900 (convert_u32_to_dfp_ext_reg,RRF_MM,"CXLFTR"),            /*810*/
 /*B95C*/ GENx___x___x___ ,
 /*B95D*/ GENx___x___x___ ,
 /*B95E*/ GENx___x___x___ ,
 /*B95F*/ GENx___x___x___ ,
 /*B960*/ GENx___x___x900 (compare_and_trap_long_register,RRF_M3,"CGRT"),          /*208*/
 /*B961*/ GENx___x___x900 (compare_logical_and_trap_long_register,RRF_M3,"CLGRT"), /*208*/
 /*B962*/ GENx___x___x___ ,
 /*B963*/ GENx___x___x___ ,
 /*B964*/ GENx___x___x___ ,
 /*B965*/ GENx___x___x___ ,
 /*B966*/ GENx___x___x___ ,
 /*B967*/ GENx___x___x___ ,
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
 /*B972*/ GENx37Xx390x900 (compare_and_trap_register,RRF_M3,"CRT"),                /*208*/
 /*B973*/ GENx37Xx390x900 (compare_logical_and_trap_register,RRF_M3,"CLRT"),       /*208*/
 /*B974*/ GENx___x___x___ ,
 /*B975*/ GENx___x___x___ ,
 /*B976*/ GENx___x___x___ ,
 /*B977*/ GENx___x___x___ ,
 /*B978*/ GENx___x___x___ ,
 /*B979*/ GENx___x___x___ ,
 /*B97A*/ GENx___x___x___ ,
 /*B97B*/ GENx___x___x___ ,
 /*B97C*/ GENx___x___x___ ,
 /*B97D*/ GENx___x___x___ ,
 /*B97E*/ GENx___x___x___ ,
 /*B97F*/ GENx___x___x___ ,
 /*B980*/ GENx___x___x900 (and_long_register,RRE,"NGR"),
 /*B981*/ GENx___x___x900 (or_long_register,RRE,"OGR"),
 /*B982*/ GENx___x___x900 (exclusive_or_long_register,RRE,"XGR"),
 /*B983*/ GENx___x___x900 (find_leftmost_one_long_register,RRE,"FLOGR"),           /*@Z9*/
 /*B984*/ GENx___x___x900 (load_logical_long_character_register,RRE,"LLGCR"),      /*@Z9*/
 /*B985*/ GENx___x___x900 (load_logical_long_halfword_register,RRE,"LLGHR"),       /*@Z9*/
 /*B986*/ GENx___x___x900 (multiply_logical_long_register,RRE,"MLGR"),
 /*B987*/ GENx___x___x900 (divide_logical_long_register,RRE,"DLGR"),
 /*B988*/ GENx___x___x900 (add_logical_carry_long_register,RRE,"ALCGR"),
 /*B989*/ GENx___x___x900 (subtract_logical_borrow_long_register,RRE,"SLBGR"),
 /*B98A*/ GENx___x___x900 (compare_and_swap_and_purge_long,RRE,"CSPG"),
 /*B98B*/ GENx___x___x___ ,
 /*B98C*/ GENx___x___x___ ,
 /*B98D*/ GENx37Xx390x900 (extract_psw,RRE,"EPSW"),
 /*B98E*/ GENx___x___x900 (invalidate_dat_table_entry,RRF_R,"IDTE"),
 /*B98F*/ GENx___x___x___ ,
 /*B990*/ GENx37Xx390x900 (translate_two_to_two,RRF_M3,"TRTT"),
 /*B991*/ GENx37Xx390x900 (translate_two_to_one,RRF_M3,"TRTO"),
 /*B992*/ GENx37Xx390x900 (translate_one_to_two,RRF_M3,"TROT"),
 /*B993*/ GENx37Xx390x900 (translate_one_to_one,RRF_M3,"TROO"),
 /*B994*/ GENx37Xx390x900 (load_logical_character_register,RRE,"LLCR"),            /*@Z9*/
 /*B995*/ GENx37Xx390x900 (load_logical_halfword_register,RRE,"LLHR"),             /*@Z9*/
 /*B996*/ GENx37Xx390x900 (multiply_logical_register,RRE,"MLR"),
 /*B997*/ GENx37Xx390x900 (divide_logical_register,RRE,"DLR"),
 /*B998*/ GENx37Xx390x900 (add_logical_carry_register,RRE,"ALCR"),
 /*B999*/ GENx37Xx390x900 (subtract_logical_borrow_register,RRE,"SLBR"),
 /*B99A*/ GENx___x___x900 (extract_primary_asn_and_instance,RRE_R1,"EPAIR"),
 /*B99B*/ GENx___x___x900 (extract_secondary_asn_and_instance,RRE_R1,"ESAIR"),
 /*B99C*/ GENx___x___x900 (extract_queue_buffer_state,RRF_RM,"EQBS"),
 /*B99D*/ GENx___x___x900 (extract_and_set_extended_authority,RRE_R1,"ESEA"),
 /*B99E*/ GENx___x___x900 (program_transfer_with_instance,RRE,"PTI"),
 /*B99F*/ GENx___x___x900 (set_secondary_asn_with_instance,RRE_R1,"SSAIR"),
 /*B9A0*/ GENx___x___x___ ,
 /*B9A1*/ GENx___x___x___ ,
 /*B9A2*/ GENx___x___x900 (perform_topology_function,RRE,"PTF"),
 /*B9A3*/ GENx___x___x___ ,
 /*B9B9*/ GENx___x___x___ ,
 /*B9A5*/ GENx___x___x___ ,
 /*B9A6*/ GENx___x___x___ ,
 /*B9A7*/ GENx___x___x___ ,
 /*B9A8*/ GENx___x___x___ ,
 /*B9A9*/ GENx___x___x___ ,
 /*B9AA*/ GENx___x___x900 (load_page_table_entry_address,RRF_RM,"LPTEA"),          /*@Z9*/
 /*B9AB*/ GENx___x___x___ , /*(extract_and_set_storage_attributes,?,"ESSA"),*/
 /*B9AC*/ GENx___x___x___ ,
 /*B9AD*/ GENx___x___x___ ,
 /*B9AE*/ GENx___x___x900 (reset_reference_bits_multiple,RRE,"RRBM"),              /*810*/
 /*B9AF*/ GENx___x___x900 (perform_frame_management_function,RRE,"PFMF"),
 /*B9B0*/ GENx37Xx390x900 (convert_utf8_to_utf32,RRF_M3,"CU14"),
 /*B9B1*/ GENx37Xx390x900 (convert_utf16_to_utf32,RRF_M3,"CU24"),
 /*B9B2*/ GENx37Xx390x900 (convert_utf32_to_utf8,RRE,"CU41"),
 /*B9B3*/ GENx37Xx390x900 (convert_utf32_to_utf16,RRE,"CU42"),
 /*B9B4*/ GENx___x___x___ ,
 /*B9B5*/ GENx___x___x___ ,
 /*B9B6*/ GENx___x___x___ ,
 /*B9B7*/ GENx___x___x___ ,
 /*B9B8*/ GENx___x___x___ ,
 /*B9B9*/ GENx___x___x___ ,
 /*B9BA*/ GENx___x___x___ ,
 /*B9BB*/ GENx___x___x___ ,
 /*B9BC*/ GENx___x___x___ ,
 /*B9BD*/ GENx37Xx390x900 (translate_and_test_reverse_extended,RRF_M3,"TRTRE"),
 /*B9BE*/ GENx37Xx390x900 (search_string_unicode,RRE,"SRSTU"),
 /*B9BF*/ GENx37Xx390x900 (translate_and_test_extended,RRF_M3,"TRTE"),
 /*B9C0*/ GENx___x___x___ ,
 /*B9C1*/ GENx___x___x___ ,
 /*B9C2*/ GENx___x___x___ ,
 /*B9C3*/ GENx___x___x___ ,
 /*B9C4*/ GENx___x___x___ ,
 /*B9C5*/ GENx___x___x___ ,
 /*B9C6*/ GENx___x___x___ ,
 /*B9C7*/ GENx___x___x___ ,
 /*B9C8*/ GENx___x___x900 (add_high_high_high_register,RRF_M3,"AHHHR"),            /*810*/
 /*B9C9*/ GENx___x___x900 (subtract_high_high_high_register,RRF_M3,"SHHHR"),       /*810*/
 /*B9CA*/ GENx___x___x900 (add_logical_high_high_high_register,RRF_M3,"ALHHHR"),   /*810*/
 /*B9CB*/ GENx___x___x900 (subtract_logical_high_high_high_register,RRF_M3,"SLHHHR"), /*810*/
 /*B9CC*/ GENx___x___x___ ,
 /*B9CD*/ GENx___x___x900 (compare_high_high_register,RRE,"CHHR"),                 /*810*/
 /*B9CE*/ GENx___x___x___ ,
 /*B9CF*/ GENx___x___x900 (compare_logical_high_high_register,RRE,"CLHHR"),        /*810*/
 /*B9D0*/ GENx___x___x___ ,
 /*B9D1*/ GENx___x___x___ ,
 /*B9D2*/ GENx___x___x___ ,
 /*B9D3*/ GENx___x___x___ ,
 /*B9D4*/ GENx___x___x___ ,
 /*B9D5*/ GENx___x___x___ ,
 /*B9D6*/ GENx___x___x___ ,
 /*B9D7*/ GENx___x___x___ ,
 /*B9D8*/ GENx___x___x900 (add_high_high_low_register,RRF_M3,"AHHLR"),             /*810*/
 /*B9D9*/ GENx___x___x900 (subtract_high_high_low_register,RRF_M3,"SHHLR"),        /*810*/
 /*B9DA*/ GENx___x___x900 (add_logical_high_high_low_register,RRF_M3,"ALHHLR"),    /*810*/
 /*B9DB*/ GENx___x___x900 (subtract_logical_high_high_low_register,RRF_M3,"SLHHLR"), /*810*/
 /*B9DC*/ GENx___x___x___ ,
 /*B9DD*/ GENx___x___x900 (compare_high_low_register,RRE,"CHLR"),                  /*810*/
 /*B9DE*/ GENx___x___x___ ,
 /*B9DF*/ GENx___x___x900 (compare_logical_high_low_register,RRE,"CLHLR"),         /*810*/
 /*B9E0*/ GENx___x___x___ ,
 /*B9E1*/ GENx___x___x900 (population_count,RRE,"POPCNT"),                         /*810*/
 /*B9E2*/ GENx___x___x900 (load_on_condition_long_register,RRF_M3,"LOCGR"),        /*810*/
 /*B9E3*/ GENx___x___x___ ,
 /*B9E4*/ GENx___x___x900 (and_distinct_long_register,RRR,"NGRK"),                 /*810*/
 /*B9E5*/ GENx___x___x___ ,
 /*B9E6*/ GENx___x___x900 (or_distinct_long_register,RRR,"OGRK"),                  /*810*/
 /*B9E7*/ GENx___x___x900 (exclusive_or_distinct_long_register,RRR,"XGRK"),        /*810*/
 /*B9E8*/ GENx___x___x900 (add_distinct_long_register,RRR,"AGRK"),                 /*810*/
 /*B9E9*/ GENx___x___x900 (subtract_distinct_long_register,RRR,"SGRK"),            /*810*/
 /*B9EA*/ GENx___x___x900 (add_logical_distinct_long_register,RRR,"ALGRK"),        /*810*/
 /*B9EB*/ GENx___x___x900 (subtract_logical_distinct_long_register,RRR,"SLGRK"),   /*810*/
 /*B9EC*/ GENx___x___x___ ,
 /*B9ED*/ GENx___x___x___ ,
 /*B9EE*/ GENx___x___x___ ,
 /*B9EF*/ GENx___x___x___ ,
 /*B9F0*/ GENx___x___x___ ,
 /*B9F1*/ GENx___x___x___ ,
 /*B9F2*/ GENx37Xx390x900 (load_on_condition_register,RRF_M3,"LOCR"),              /*810*/
 /*B9F3*/ GENx___x___x___ ,
 /*B9F4*/ GENx37Xx390x900 (and_distinct_register,RRR,"NRK"),                       /*810*/
 /*B9F5*/ GENx___x___x___ ,
 /*B9F6*/ GENx37Xx390x900 (or_distinct_register,RRR,"ORK"),                        /*810*/
 /*B9F7*/ GENx37Xx390x900 (exclusive_or_distinct_register,RRR,"XRK"),              /*810*/
 /*B9F8*/ GENx37Xx390x900 (add_distinct_register,RRR,"ARK"),                       /*810*/
 /*B9F9*/ GENx37Xx390x900 (subtract_distinct_register,RRR,"SRK"),                  /*810*/
 /*B9FA*/ GENx37Xx390x900 (add_logical_distinct_register,RRR,"ALRK"),              /*810*/
 /*B9FB*/ GENx37Xx390x900 (subtract_logical_distinct_register,RRR,"SLRK"),         /*810*/
 /*B9FC*/ GENx___x___x___ ,
 /*B9FD*/ GENx___x___x___ ,
 /*B9FE*/ GENx___x___x___ ,
 /*B9FF*/ GENx___x___x___  };


static zz_func opcode_c0_x[0x10][GEN_MAXARCH] = {
 /*C0x0*/ GENx37Xx390x900 (load_address_relative_long,RIL_A,"LARL"),
 /*C0x1*/ GENx___x___x900 (load_long_fullword_immediate,RIL,"LGFI"),               /*@Z9*/
 /*C0x2*/ GENx___x___x___ ,
 /*C0x3*/ GENx___x___x___ ,
 /*C0x4*/ GENx37Xx390x900 (branch_relative_on_condition_long,RIL_A,"BRCL"),
 /*C0x5*/ GENx37Xx390x900 (branch_relative_and_save_long,RIL_A,"BRASL"),
 /*C0x6*/ GENx___x___x900 (exclusive_or_immediate_high_fullword,RIL,"XIHF"),       /*@Z9*/
 /*C0x7*/ GENx___x___x900 (exclusive_or_immediate_low_fullword,RIL,"XILF"),        /*@Z9*/
 /*C0x8*/ GENx___x___x900 (insert_immediate_high_fullword,RIL,"IIHF"),             /*@Z9*/
 /*C0x9*/ GENx___x___x900 (insert_immediate_low_fullword,RIL,"IILF"),              /*@Z9*/
 /*C0xA*/ GENx___x___x900 (and_immediate_high_fullword,RIL,"NIHF"),                /*@Z9*/
 /*C0xB*/ GENx___x___x900 (and_immediate_low_fullword,RIL,"NILF"),                 /*@Z9*/
 /*C0xC*/ GENx___x___x900 (or_immediate_high_fullword,RIL,"OIHF"),                 /*@Z9*/
 /*C0xD*/ GENx___x___x900 (or_immediate_low_fullword,RIL,"OILF"),                  /*@Z9*/
 /*C0xE*/ GENx___x___x900 (load_logical_immediate_high_fullword,RIL,"LLIHF"),      /*@Z9*/
 /*C0xF*/ GENx___x___x900 (load_logical_immediate_low_fullword,RIL,"LLILF") };     /*@Z9*/


static zz_func opcode_c2_x[0x10][GEN_MAXARCH] = {                                  /*@Z9*/
 /*C2x0*/ GENx___x___x900 (multiply_single_immediate_long_fullword,RIL,"MSGFI"),   /*208*/
 /*C2x1*/ GENx37Xx390x900 (multiply_single_immediate_fullword,RIL,"MSFI"),         /*208*/
 /*C2x2*/ GENx___x___x___ ,                                                        /*@Z9*/
 /*C2x3*/ GENx___x___x___ ,                                                        /*@Z9*/
 /*C2x4*/ GENx___x___x900 (subtract_logical_long_fullword_immediate,RIL,"SLGFI"),  /*@Z9*/
 /*C2x5*/ GENx37Xx390x900 (subtract_logical_fullword_immediate,RIL,"SLFI"),        /*@Z9*/
 /*C2x6*/ GENx___x___x___ ,                                                        /*@Z9*/
 /*C2x7*/ GENx___x___x___ ,                                                        /*@Z9*/
 /*C2x8*/ GENx___x___x900 (add_long_fullword_immediate,RIL,"AGFI"),                /*@Z9*/
 /*C2x9*/ GENx37Xx390x900 (add_fullword_immediate,RIL,"AFI"),                      /*@Z9*/
 /*C2xA*/ GENx___x___x900 (add_logical_long_fullword_immediate,RIL,"ALGFI"),       /*@Z9*/
 /*C2xB*/ GENx37Xx390x900 (add_logical_fullword_immediate,RIL,"ALFI"),             /*@Z9*/
 /*C2xC*/ GENx___x___x900 (compare_long_fullword_immediate,RIL,"CGFI"),            /*@Z9*/
 /*C2xD*/ GENx37Xx390x900 (compare_fullword_immediate,RIL,"CFI"),                  /*@Z9*/
 /*C2xE*/ GENx___x___x900 (compare_logical_long_fullword_immediate,RIL,"CLGFI"),   /*@Z9*/
 /*C2xF*/ GENx37Xx390x900 (compare_logical_fullword_immediate,RIL,"CLFI") };       /*@Z9*/


static zz_func opcode_c4_x[0x10][GEN_MAXARCH] = {                                  /*208*/
 /*C4x0*/ GENx___x___x___ ,                                                        /*208*/
 /*C4x1*/ GENx___x___x___ ,                                                        /*208*/
 /*C4x2*/ GENx37Xx390x900 (load_logical_halfword_relative_long,RIL_A,"LLHRL"),     /*208*/
 /*C4x3*/ GENx___x___x___ ,                                                        /*208*/
 /*C4x4*/ GENx___x___x900 (load_halfword_relative_long_long,RIL_A,"LGHRL"),        /*208*/
 /*C4x5*/ GENx37Xx390x900 (load_halfword_relative_long,RIL_A,"LHRL"),              /*208*/
 /*C4x6*/ GENx___x___x900 (load_logical_halfword_relative_long_long,RIL_A,"LLGHRL"), /*208*/
 /*C4x7*/ GENx37Xx390x900 (store_halfword_relative_long,RIL_A,"STHRL"),            /*208*/
 /*C4x8*/ GENx___x___x900 (load_relative_long_long,RIL_A,"LGRL"),                  /*208*/
 /*C4x9*/ GENx___x___x___ ,                                                        /*208*/
 /*C4xA*/ GENx___x___x___ ,                                                        /*208*/
 /*C4xB*/ GENx___x___x900 (store_relative_long_long,RIL_A,"STGRL"),                /*208*/
 /*C4xC*/ GENx___x___x900 (load_relative_long_long_fullword,RIL_A,"LGFRL"),        /*208*/
 /*C4xD*/ GENx37Xx390x900 (load_relative_long,RIL_A,"LRL"),                        /*208*/
 /*C4xE*/ GENx___x___x900 (load_logical_relative_long_long_fullword,RIL_A,"LLGFRL"), /*208*/
 /*C4xF*/ GENx37Xx390x900 (store_relative_long,RIL_A,"STRL") };                    /*208*/


static zz_func opcode_c6_x[0x10][GEN_MAXARCH] = {                                  /*208*/
 /*C6x0*/ GENx37Xx390x900 (execute_relative_long,RIL_A,"EXRL"),                    /*208*/
 /*C6x1*/ GENx___x___x___ ,                                                        /*208*/
 /*C6x2*/ GENx37Xx390x900 (prefetch_data_relative_long,RIL_A,"PFDRL"),             /*208*/
 /*C6x3*/ GENx___x___x___ ,                                                        /*208*/
 /*C6x4*/ GENx___x___x900 (compare_halfword_relative_long_long,RIL_A,"CGHRL"),     /*208*/
 /*C6x5*/ GENx37Xx390x900 (compare_halfword_relative_long,RIL_A,"CHRL"),           /*208*/
 /*C6x6*/ GENx___x___x900 (compare_logical_relative_long_long_halfword,RIL_A,"CLGHRL"), /*208*/
 /*C6x7*/ GENx37Xx390x900 (compare_logical_relative_long_halfword,RIL_A,"CLHRL"),  /*208*/
 /*C6x8*/ GENx___x___x900 (compare_relative_long_long,RIL_A,"CGRL"),               /*208*/
 /*C6x9*/ GENx___x___x___ ,                                                        /*208*/
 /*C6xA*/ GENx___x___x900 (compare_logical_relative_long_long,RIL_A,"CLGRL"),      /*208*/
 /*C6xB*/ GENx___x___x___ ,                                                        /*208*/
 /*C6xC*/ GENx___x___x900 (compare_relative_long_long_fullword,RIL_A,"CGFRL"),     /*208*/
 /*C6xD*/ GENx37Xx390x900 (compare_relative_long,RIL_A,"CRL"),                     /*208*/
 /*C6xE*/ GENx___x___x900 (compare_logical_relative_long_long_fullword,RIL_A,"CLGFRL"), /*208*/
 /*C6xF*/ GENx37Xx390x900 (compare_logical_relative_long,RIL_A,"CLRL") };          /*208*/


static zz_func opcode_c8_x[0x10][GEN_MAXARCH] = {
 /*C8x0*/ GENx___x___x900 (move_with_optional_specifications,SSF,"MVCOS"),
 /*C8x1*/ GENx___x___x900 (extract_cpu_time,SSF,"ECTG"),
 /*C8x2*/ GENx___x___x900 (compare_and_swap_and_store,SSF,"CSST"),
 /*C8x3*/ GENx___x___x___ ,
 /*C8x4*/ GENx37Xx390x900 (load_pair_disjoint,SSF_RSS,"LPD"),                      /*810*/
 /*C8x5*/ GENx___x___x900 (load_pair_disjoint_long,SSF_RSS,"LPDG"),                /*810*/
 /*C8x6*/ GENx___x___x___ ,
 /*C8x7*/ GENx___x___x___ ,
 /*C8x8*/ GENx___x___x___ ,
 /*C8x9*/ GENx___x___x___ ,
 /*C8xA*/ GENx___x___x___ ,
 /*C8xB*/ GENx___x___x___ ,
 /*C8xC*/ GENx___x___x___ ,
 /*C8xD*/ GENx___x___x___ ,
 /*C8xE*/ GENx___x___x___ ,
 /*C8xF*/ GENx___x___x___ };


static zz_func opcode_cc_x[0x10][GEN_MAXARCH] = {                                  /*810*/
 /*CCx0*/ GENx___x___x___ ,
 /*CCx1*/ GENx___x___x___ ,
 /*CCx2*/ GENx___x___x___ ,
 /*CCx3*/ GENx___x___x___ ,
 /*CCx4*/ GENx___x___x___ ,
 /*CCx5*/ GENx___x___x___ ,
 /*CCx6*/ GENx___x___x900 (branch_relative_on_count_high,RIL,"BRCTH"),             /*810*/
 /*CCx7*/ GENx___x___x___ ,
 /*CCx8*/ GENx___x___x900 (add_high_immediate,RIL,"AIH"),                          /*810*/
 /*CCx9*/ GENx___x___x___ ,
 /*CCxA*/ GENx___x___x900 (add_logical_with_signed_immediate_high,RIL,"ALSIH"),    /*810*/
 /*CCxB*/ GENx___x___x900 (add_logical_with_signed_immediate_high_n,RIL,"ALSIHN"), /*810*/
 /*CCxC*/ GENx___x___x___ ,
 /*CCxD*/ GENx___x___x900 (compare_high_immediate,RIL,"CIH"),                      /*810*/
 /*CCxE*/ GENx___x___x___ ,
 /*CCxF*/ GENx___x___x900 (compare_logical_high_immediate,RIL,"CLIH") };           /*810*/


static zz_func opcode_e3xx[0x100][GEN_MAXARCH] = {
 /*E300*/ GENx___x___x___ ,
 /*E301*/ GENx___x___x___ ,
 /*E302*/ GENx___x___x900 (load_and_test_long,RXY,"LTG"),                          /*@Z9*/
 /*E303*/ GENx___x___x900 (load_real_address_long,RXY,"LRAG"),
 /*E304*/ GENx___x___x900 (load_long,RXY,"LG"),
 /*E305*/ GENx___x___x___ ,
 /*E306*/ GENx___x___x900 (convert_to_binary_y,RXY,"CVBY"),
 /*E307*/ GENx___x___x___ ,
 /*E308*/ GENx___x___x900 (add_long,RXY,"AG"),
 /*E309*/ GENx___x___x900 (subtract_long,RXY,"SG"),
 /*E30A*/ GENx___x___x900 (add_logical_long,RXY,"ALG"),
 /*E30B*/ GENx___x___x900 (subtract_logical_long,RXY,"SLG"),
 /*E30C*/ GENx___x___x900 (multiply_single_long,RXY,"MSG"),
 /*E30D*/ GENx___x___x900 (divide_single_long,RXY,"DSG"),
 /*E30E*/ GENx___x___x900 (convert_to_binary_long,RXY,"CVBG"),
 /*E30F*/ GENx___x___x900 (load_reversed_long,RXY,"LRVG"),
 /*E310*/ GENx___x___x___ ,
 /*E311*/ GENx___x___x___ ,
 /*E312*/ GENx37Xx390x900 (load_and_test,RXY,"LT"),                                /*@Z9*/
 /*E313*/ GENx___x___x900 (load_real_address_y,RXY,"LRAY"),
 /*E314*/ GENx___x___x900 (load_long_fullword,RXY,"LGF"),
 /*E315*/ GENx___x___x900 (load_long_halfword,RXY,"LGH"),
 /*E316*/ GENx___x___x900 (load_logical_long_fullword,RXY,"LLGF"),
 /*E317*/ GENx___x___x900 (load_logical_long_thirtyone,RXY,"LLGT"),
 /*E318*/ GENx___x___x900 (add_long_fullword,RXY,"AGF"),
 /*E319*/ GENx___x___x900 (subtract_long_fullword,RXY,"SGF"),
 /*E31A*/ GENx___x___x900 (add_logical_long_fullword,RXY,"ALGF"),
 /*E31B*/ GENx___x___x900 (subtract_logical_long_fullword,RXY,"SLGF"),
 /*E31C*/ GENx___x___x900 (multiply_single_long_fullword,RXY,"MSGF"),
 /*E31D*/ GENx___x___x900 (divide_single_long_fullword,RXY,"DSGF"),
 /*E31E*/ GENx___x390x900 (load_reversed,RXY,"LRV"),
 /*E31F*/ GENx___x390x900 (load_reversed_half,RXY,"LRVH"),
 /*E320*/ GENx___x___x900 (compare_long,RXY,"CG"),
 /*E321*/ GENx___x___x900 (compare_logical_long,RXY,"CLG"),
 /*E322*/ GENx___x___x___ ,
 /*E323*/ GENx___x___x___ ,
 /*E324*/ GENx___x___x900 (store_long,RXY,"STG"),
 /*E325*/ GENx___x___x___ ,
 /*E326*/ GENx___x___x900 (convert_to_decimal_y,RXY,"CVDY"),
 /*E327*/ GENx___x___x___ ,
 /*E328*/ GENx___x___x___ ,
 /*E329*/ GENx___x___x___ ,
 /*E32A*/ GENx___x___x___ ,
 /*E32B*/ GENx___x___x___ ,
 /*E32C*/ GENx___x___x___ ,
 /*E32D*/ GENx___x___x___ ,
 /*E32E*/ GENx___x___x900 (convert_to_decimal_long,RXY,"CVDG"),
 /*E32F*/ GENx___x___x900 (store_reversed_long,RXY,"STRVG"),
 /*E330*/ GENx___x___x900 (compare_long_fullword,RXY,"CGF"),
 /*E331*/ GENx___x___x900 (compare_logical_long_fullword,RXY,"CLGF"),
 /*E332*/ GENx___x___x900 (load_and_test_long_fullword,RXY,"LTGF"),                /*208*/
 /*E333*/ GENx___x___x___ ,
 /*E334*/ GENx___x___x900 (compare_halfword_long,RXY,"CGH"),                       /*208*/
 /*E335*/ GENx___x___x___ ,
 /*E336*/ GENx37Xx390x900 (prefetch_data,RXY,"PFD"),                               /*208*/
 /*E337*/ GENx___x___x___ ,
 /*E338*/ GENx___x___x___ ,
 /*E339*/ GENx___x___x___ ,
 /*E33A*/ GENx___x___x___ ,
 /*E33B*/ GENx___x___x___ ,
 /*E33C*/ GENx___x___x___ ,
 /*E33D*/ GENx___x___x___ ,
 /*E33E*/ GENx___x390x900 (store_reversed,RXY,"STRV"),
 /*E33F*/ GENx___x390x900 (store_reversed_half,RXY,"STRVH"),
 /*E340*/ GENx___x___x___ ,
 /*E341*/ GENx___x___x___ ,
 /*E342*/ GENx___x___x___ ,
 /*E343*/ GENx___x___x___ ,
 /*E344*/ GENx___x___x___ ,
 /*E345*/ GENx___x___x___ ,
 /*E346*/ GENx___x___x900 (branch_on_count_long,RXY,"BCTG"),
 /*E347*/ GENx___x___x___ ,
 /*E348*/ GENx___x___x___ ,
 /*E349*/ GENx___x___x___ ,
 /*E34A*/ GENx___x___x___ ,
 /*E34B*/ GENx___x___x___ ,
 /*E34C*/ GENx___x___x___ ,
 /*E34D*/ GENx___x___x___ ,
 /*E34E*/ GENx___x___x___ ,
 /*E34F*/ GENx___x___x___ ,
 /*E350*/ GENx___x___x900 (store_y,RXY,"STY"),
 /*E351*/ GENx___x___x900 (multiply_single_y,RXY,"MSY"),
 /*E352*/ GENx___x___x___ ,
 /*E353*/ GENx___x___x___ ,
 /*E354*/ GENx___x___x900 (and_y,RXY,"NY"),
 /*E355*/ GENx___x___x900 (compare_logical_y,RXY,"CLY"),
 /*E356*/ GENx___x___x900 (or_y,RXY,"OY"),
 /*E357*/ GENx___x___x900 (exclusive_or_y,RXY,"XY"),
 /*E358*/ GENx___x___x900 (load_y,RXY,"LY"),
 /*E359*/ GENx___x___x900 (compare_y,RXY,"CY"),
 /*E35A*/ GENx___x___x900 (add_y,RXY,"AY"),
 /*E35B*/ GENx___x___x900 (subtract_y,RXY,"SY"),
 /*E35C*/ GENx___x___x900 (multiply_y,RXY,"MFY"),                                  /*208*/
 /*E35D*/ GENx___x___x___ ,
 /*E35E*/ GENx___x___x900 (add_logical_y,RXY,"ALY"),
 /*E35F*/ GENx___x___x900 (subtract_logical_y,RXY,"SLY"),
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
 /*E370*/ GENx___x___x900 (store_halfword_y,RXY,"STHY"),
 /*E371*/ GENx___x___x900 (load_address_y,RXY,"LAY"),
 /*E372*/ GENx___x___x900 (store_character_y,RXY,"STCY"),
 /*E373*/ GENx___x___x900 (insert_character_y,RXY,"ICY"),
 /*E374*/ GENx___x___x___ ,
 /*E375*/ GENx___x___x900 (load_address_extended_y,RXY,"LAEY"),                    /*208*/
 /*E376*/ GENx___x___x900 (load_byte,RXY,"LB"),
 /*E377*/ GENx___x___x900 (load_byte_long,RXY,"LGB"),
 /*E378*/ GENx___x___x900 (load_halfword_y,RXY,"LHY"),
 /*E379*/ GENx___x___x900 (compare_halfword_y,RXY,"CHY"),
 /*E37A*/ GENx___x___x900 (add_halfword_y,RXY,"AHY"),
 /*E37B*/ GENx___x___x900 (subtract_halfword_y,RXY,"SHY"),
 /*E37C*/ GENx___x___x900 (multiply_halfword_y,RXY,"MHY"),                         /*208*/
 /*E37D*/ GENx___x___x___ ,
 /*E37E*/ GENx___x___x___ ,
 /*E37F*/ GENx___x___x___ ,
 /*E380*/ GENx___x___x900 (and_long,RXY,"NG"),
 /*E381*/ GENx___x___x900 (or_long,RXY,"OG"),
 /*E382*/ GENx___x___x900 (exclusive_or_long,RXY,"XG"),
 /*E383*/ GENx___x___x___ ,
 /*E384*/ GENx___x___x___ ,
 /*E385*/ GENx___x___x___ ,
 /*E386*/ GENx___x___x900 (multiply_logical_long,RXY,"MLG"),
 /*E387*/ GENx___x___x900 (divide_logical_long,RXY,"DLG"),
 /*E388*/ GENx___x___x900 (add_logical_carry_long,RXY,"ALCG"),
 /*E389*/ GENx___x___x900 (subtract_logical_borrow_long,RXY,"SLBG"),
 /*E38A*/ GENx___x___x___ ,
 /*E38B*/ GENx___x___x___ ,
 /*E38C*/ GENx___x___x___ ,
 /*E38D*/ GENx___x___x___ ,
 /*E38E*/ GENx___x___x900 (store_pair_to_quadword,RXY,"STPQ"),
 /*E38F*/ GENx___x___x900 (load_pair_from_quadword,RXY,"LPQ"),
 /*E390*/ GENx___x___x900 (load_logical_long_character,RXY,"LLGC"),
 /*E391*/ GENx___x___x900 (load_logical_long_halfword,RXY,"LLGH"),
 /*E392*/ GENx___x___x___ ,
 /*E393*/ GENx___x___x___ ,
 /*E394*/ GENx37Xx390x900 (load_logical_character,RXY,"LLC"),                      /*@Z9*/
 /*E395*/ GENx37Xx390x900 (load_logical_halfword,RXY,"LLH"),                       /*@Z9*/
 /*E396*/ GENx37Xx390x900 (multiply_logical,RXY,"ML"),
 /*E397*/ GENx37Xx390x900 (divide_logical,RXY,"DL"),
 /*E398*/ GENx37Xx390x900 (add_logical_carry,RXY,"ALC"),
 /*E399*/ GENx37Xx390x900 (subtract_logical_borrow,RXY,"SLB"),
 /*E39A*/ GENx___x___x___ ,
 /*E39B*/ GENx___x___x___ ,
 /*E39C*/ GENx___x___x___ ,
 /*E39D*/ GENx___x___x___ ,
 /*E39E*/ GENx___x___x___ ,
 /*E39F*/ GENx___x___x___ ,
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
 /*E3C0*/ GENx___x___x900 (load_byte_high,RXY,"LBH"),                              /*810*/
 /*E3C1*/ GENx___x___x___ ,
 /*E3C2*/ GENx___x___x900 (load_logical_character_high,RXY,"LLCH"),                /*810*/
 /*E3C3*/ GENx___x___x900 (store_character_high,RXY,"STCH"),                       /*810*/
 /*E3C4*/ GENx___x___x900 (load_halfword_high,RXY,"LHH"),                          /*810*/
 /*E3C5*/ GENx___x___x___ ,
 /*E3C6*/ GENx___x___x900 (load_logical_halfword_high,RXY,"LLHH"),                 /*810*/
 /*E3C7*/ GENx___x___x900 (store_halfword_high,RXY,"STHH"),                        /*810*/
 /*E3C8*/ GENx___x___x___ ,
 /*E3C9*/ GENx___x___x___ ,
 /*E3CA*/ GENx___x___x900 (load_fullword_high,RXY,"LFH"),                          /*810*/
 /*E3CB*/ GENx___x___x900 (store_fullword_high,RXY,"STFH"),                        /*810*/
 /*E3CC*/ GENx___x___x___ ,
 /*E3CD*/ GENx___x___x900 (compare_high_fullword,RXY,"CHF"),                       /*810*/
 /*E3CE*/ GENx___x___x___ ,
 /*E3CF*/ GENx___x___x900 (compare_logical_high_fullword,RXY,"CLHF"),              /*810*/
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
 /*E3FF*/ GENx___x___x___  };


static zz_func opcode_e5xx[0x100][GEN_MAXARCH] = {
 /*E500*/ GENx370x390x900 (load_address_space_parameters,SSE,"LASP"),
 /*E501*/ GENx370x390x900 (test_protection,SSE,"TPROT"),
 /* The following opcode has been re-used in z/Arch */
#define s370_store_real_address s370_fix_page
 /*E502*/ GENx370x___x900 (store_real_address,SSE,"STRAG"),
 /*E503*/ GENx370x390x900 (svc_assist,SSE,"Assist"),
 /*E504*/ GENx370x390x900 (obtain_local_lock,SSE,"Assist"),
 /*E505*/ GENx370x390x900 (release_local_lock,SSE,"Assist"),
 /*E506*/ GENx370x390x900 (obtain_cms_lock,SSE,"Assist"),
 /*E507*/ GENx370x390x900 (release_cms_lock,SSE,"Assist"),
 /*E508*/ GENx370x___x___ (trace_svc_interruption,SSE,"Assist"),
 /*E509*/ GENx370x___x___ (trace_program_interruption,SSE,"Assist"),
 /*E50A*/ GENx370x___x___ (trace_initial_srb_dispatch,SSE,"Assist"),
 /*E50B*/ GENx370x___x___ (trace_io_interruption,SSE,"Assist"),
 /*E50C*/ GENx370x___x___ (trace_task_dispatch,SSE,"Assist"),
 /*E50D*/ GENx370x___x___ (trace_svc_return,SSE,"Assist"),
 /*E50E*/ GENx___x390x900 (move_with_source_key,SSE,"MVCSK"),
 /*E50F*/ GENx___x390x900 (move_with_destination_key,SSE,"MVCDK"),
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
 /*E544*/ GENx37Xx390x900 (move_halfword_from_halfword_immediate,SIL,"MVHHI"),     /*208*/
 /*E545*/ GENx___x___x___ ,
 /*E546*/ GENx___x___x___ ,
 /*E547*/ GENx___x___x___ ,
 /*E548*/ GENx37Xx390x900 (move_long_from_halfword_immediate,SIL,"MVGHI"),         /*208*/
 /*E549*/ GENx___x___x___ ,
 /*E54A*/ GENx___x___x___ ,
 /*E54B*/ GENx___x___x___ ,
 /*E54C*/ GENx37Xx390x900 (move_fullword_from_halfword_immediate,SIL,"MVHI"),      /*208*/
 /*E54D*/ GENx___x___x___ ,
 /*E54E*/ GENx___x___x___ ,
 /*E54F*/ GENx___x___x___ ,
 /*E550*/ GENx___x___x___ ,
 /*E551*/ GENx___x___x___ ,
 /*E552*/ GENx___x___x___ ,
 /*E553*/ GENx___x___x___ ,
 /*E554*/ GENx37Xx390x900 (compare_halfword_immediate_halfword_storage,SIL,"CHHSI"), /*208*/
 /*E555*/ GENx37Xx390x900 (compare_logical_immediate_halfword_storage,SIL,"CLHHSI"), /*208*/
 /*E556*/ GENx___x___x___ ,
 /*E557*/ GENx___x___x___ ,
 /*E558*/ GENx37Xx390x900 (compare_halfword_immediate_long_storage,SIL,"CGHSI"),   /*208*/
 /*E559*/ GENx37Xx390x900 (compare_logical_immediate_long_storage,SIL,"CLGHSI"),   /*208*/
 /*E55A*/ GENx___x___x___ ,
 /*E55B*/ GENx___x___x___ ,
 /*E55C*/ GENx37Xx390x900 (compare_halfword_immediate_storage,SIL,"CHSI"),         /*208*/
 /*E55D*/ GENx37Xx390x900 (compare_logical_immediate_fullword_storage,SIL,"CLFHSI"), /*208*/
 /*E55E*/ GENx___x___x___ ,
 /*E55F*/ GENx___x___x___ ,
 /*E560*/ GENx___x___x___ ,
 /*E561*/ GENx___x___x___ ,
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
 /*E5FF*/ GENx___x___x___  };


static zz_func opcode_e6xx[0x100][GEN_MAXARCH] = {
 /*E600*/ GENx370x___x___ (ecpsvm_basic_freex,SSE,"FREE"),
 /*E601*/ GENx370x___x___ (ecpsvm_basic_fretx,SSE,"FRET"),
 /*E602*/ GENx370x___x___ (ecpsvm_lock_page,SSE,"VLKPG"),
 /*E603*/ GENx370x___x___ (ecpsvm_unlock_page,SSE,"VULKP"),
 /*E604*/ GENx370x___x___ (ecpsvm_decode_next_ccw,SSE,"DNCCW"),
 /*E605*/ GENx370x___x___ (ecpsvm_free_ccwstor,SSE,"FCCWS"),
 /*E606*/ GENx370x___x___ (ecpsvm_locate_vblock,SSE,"SCNVU"),
 /*E607*/ GENx370x___x___ (ecpsvm_disp1,SSE,"ECPS:DISP1"),
 /*E608*/ GENx370x___x___ (ecpsvm_tpage,SSE,"ECPS:TRBRG"),
 /*E609*/ GENx370x___x___ (ecpsvm_tpage_lock,SSE,"TRLCK"),
 /*E60A*/ GENx370x___x___ (ecpsvm_inval_segtab,SSE,"VIST"),
 /*E60B*/ GENx370x___x___ (ecpsvm_inval_ptable,SSE,"VIPT"),
 /*E60C*/ GENx370x___x___ (ecpsvm_decode_first_ccw,SSE,"DFCCW"),
 /*E60D*/ GENx370x___x___ (ecpsvm_dispatch_main,SSE,"DISP0"),
 /*E60E*/ GENx370x___x___ (ecpsvm_locate_rblock,SSE,"SCNRU"),
 /*E60F*/ GENx370x___x___ (ecpsvm_comm_ccwproc,SSE,"CCWGN"),
 /*E610*/ GENx370x___x___ (ecpsvm_unxlate_ccw,SSE,"UXCCW"),
 /*E611*/ GENx370x___x___ (ecpsvm_disp2,SSE,"DISP2"),
 /*E612*/ GENx370x___x___ (ecpsvm_store_level,SSE,"STEVL"),
 /*E613*/ GENx370x___x___ (ecpsvm_loc_chgshrpg,SSE,"LCSPG"),
 /*E614*/ GENx370x___x___ (ecpsvm_extended_freex,SSE,"FREEX"),
 /*E615*/ GENx370x___x___ (ecpsvm_extended_fretx,SSE,"FRETX"),
 /*E616*/ GENx370x___x___ (ecpsvm_prefmach_assist,SSE,"PRFMA"),
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
 /*E6FF*/ GENx___x___x___  };


static zz_func opcode_ebxx[0x100][GEN_MAXARCH] = {
 /*EB00*/ GENx___x___x___ ,
 /*EB01*/ GENx___x___x___ ,
 /*EB02*/ GENx___x___x___ ,
 /*EB03*/ GENx___x___x___ ,
 /*EB04*/ GENx___x___x900 (load_multiple_long,RSY,"LMG"),
 /*EB05*/ GENx___x___x___ ,
 /*EB06*/ GENx___x___x___ ,
 /*EB07*/ GENx___x___x___ ,
 /*EB08*/ GENx___x___x___ ,
 /*EB09*/ GENx___x___x___ ,
 /*EB0A*/ GENx___x___x900 (shift_right_single_long,RSY,"SRAG"),
 /*EB0B*/ GENx___x___x900 (shift_left_single_long,RSY,"SLAG"),
 /*EB0C*/ GENx___x___x900 (shift_right_single_logical_long,RSY,"SRLG"),
 /*EB0D*/ GENx___x___x900 (shift_left_single_logical_long,RSY,"SLLG"),
 /*EB0E*/ GENx___x___x___ ,
 /*EB0F*/ GENx___x___x900 (trace_long,RSY,"TRACG"),
 /*EB10*/ GENx___x___x___ ,
 /*EB11*/ GENx___x___x___ ,
 /*EB12*/ GENx___x___x___ ,
 /*EB13*/ GENx___x___x___ ,
 /*EB14*/ GENx___x___x900 (compare_and_swap_y,RSY,"CSY"),
 /*EB15*/ GENx___x___x___ ,
 /*EB16*/ GENx___x___x___ ,
 /*EB17*/ GENx___x___x900  (store_cpu_counter_multiple,RSY,"STCCTM"),	/* STCCTM - store-CPU-counter-multiple facility */
 /*EB18*/ GENx___x___x___ ,
 /*EB19*/ GENx___x___x___ ,
 /*EB1A*/ GENx___x___x___ ,
 /*EB1B*/ GENx___x___x___ ,
 /*EB1C*/ GENx___x___x900 (rotate_left_single_logical_long,RSY,"RLLG"),
 /*EB1D*/ GENx37Xx390x900 (rotate_left_single_logical,RSY,"RLL"),
 /*EB1E*/ GENx___x___x___ ,
 /*EB1F*/ GENx___x___x___ ,
 /*EB20*/ GENx___x___x900 (compare_logical_characters_under_mask_high,RSY,"CLMH"),
 /*EB21*/ GENx___x___x900 (compare_logical_characters_under_mask_y,RSY,"CLMY"),
 /*EB22*/ GENx___x___x___ ,
 /*EB23*/ GENx___x___x___ ,
 /*EB24*/ GENx___x___x900 (store_multiple_long,RSY,"STMG"),
 /*EB25*/ GENx___x___x900 (store_control_long,RSY,"STCTG"),
 /*EB26*/ GENx___x___x900 (store_multiple_high,RSY,"STMH"),
 /*EB27*/ GENx___x___x___ ,
 /*EB28*/ GENx___x___x___ ,
 /*EB29*/ GENx___x___x___ ,
 /*EB2A*/ GENx___x___x___ ,
 /*EB2B*/ GENx___x___x___ ,
 /*EB2C*/ GENx___x___x900 (store_characters_under_mask_high,RSY,"STCMH"),
 /*EB2D*/ GENx___x___x900 (store_characters_under_mask_y,RSY,"STCMY"),
 /*EB2E*/ GENx___x___x___ ,
 /*EB2F*/ GENx___x___x900 (load_control_long,RSY,"LCTLG"),
 /*EB30*/ GENx___x___x900 (compare_and_swap_long,RSY,"CSG"),
 /*EB31*/ GENx___x___x900 (compare_double_and_swap_y,RSY,"CDSY"),
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
 /*EB3E*/ GENx___x___x900 (compare_double_and_swap_long,RSY,"CDSG"),
 /*EB3F*/ GENx___x___x___ ,
 /*EB40*/ GENx___x___x___ ,
 /*EB41*/ GENx___x___x___ ,
 /*EB42*/ GENx___x___x___ ,
 /*EB43*/ GENx___x___x___ ,
 /*EB44*/ GENx___x___x900 (branch_on_index_high_long,RSY,"BXHG"),
 /*EB45*/ GENx___x___x900 (branch_on_index_low_or_equal_long,RSY,"BXLEG"),
 /*EB46*/ GENx___x___x___ ,
 /*EB47*/ GENx___x___x___ ,
 /*EB48*/ GENx___x___x___ ,
 /*EB49*/ GENx___x___x___ ,
 /*EB4A*/ GENx___x___x___ ,
 /*EB4B*/ GENx___x___x___ ,
 /*EB4C*/ GENx___x___x900 (extract_cache_attribute,RSY,"ECAG"),                    /*208*/
 /*EB4D*/ GENx___x___x___ ,
 /*EB4E*/ GENx___x___x___ ,
 /*EB4F*/ GENx___x___x___ ,
 /*EB50*/ GENx___x___x___ ,
 /*EB51*/ GENx___x___x900 (test_under_mask_y,SIY,"TMY"),
 /*EB52*/ GENx___x___x900 (move_immediate_y,SIY,"MVIY"),
 /*EB53*/ GENx___x___x___ ,
 /*EB54*/ GENx___x___x900 (and_immediate_y,SIY,"NIY"),
 /*EB55*/ GENx___x___x900 (compare_logical_immediate_y,SIY,"CLIY"),
 /*EB56*/ GENx___x___x900 (or_immediate_y,SIY,"OIY"),
 /*EB57*/ GENx___x___x900 (exclusive_or_immediate_y,SIY,"XIY"),
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
 /*EB6A*/ GENx37Xx390x900 (add_immediate_storage,SIY,"ASI"),                       /*208*/
 /*EB6B*/ GENx___x___x___ ,
 /*EB6C*/ GENx___x___x___ ,
 /*EB6D*/ GENx___x___x___ ,
 /*EB6E*/ GENx37Xx390x900 (add_logical_with_signed_immediate,SIY,"ALSI"),          /*208*/
 /*EB6F*/ GENx___x___x___ ,
 /*EB70*/ GENx___x___x___ ,
 /*EB71*/ GENx___x___x___ ,
 /*EB72*/ GENx___x___x___ ,
 /*EB73*/ GENx___x___x___ ,
 /*EB74*/ GENx___x___x___ ,
 /*EB75*/ GENx___x___x___ ,
 /*EB76*/ GENx___x___x___ ,
 /*EB77*/ GENx___x___x___ ,
 /*EB78*/ GENx___x___x___ ,
 /*EB79*/ GENx___x___x___ ,
 /*EB7A*/ GENx37Xx390x900 (add_immediate_long_storage,SIY,"AGSI"),                 /*208*/
 /*EB7B*/ GENx___x___x___ ,
 /*EB7C*/ GENx___x___x___ ,
 /*EB7D*/ GENx___x___x___ ,
 /*EB7E*/ GENx37Xx390x900 (add_logical_with_signed_immediate_long,SIY,"ALGSI"),    /*208*/
 /*EB7F*/ GENx___x___x___ ,
 /*EB80*/ GENx___x___x900 (insert_characters_under_mask_high,RSY,"ICMH"),
 /*EB81*/ GENx___x___x900 (insert_characters_under_mask_y,RSY,"ICMY"),
 /*EB82*/ GENx___x___x___ ,
 /*EB83*/ GENx___x___x___ ,
 /*EB84*/ GENx___x___x___ ,
 /*EB85*/ GENx___x___x___ ,
 /*EB86*/ GENx___x___x___ ,
 /*EB87*/ GENx___x___x___ ,
 /*EB88*/ GENx___x___x___ ,
 /*EB89*/ GENx___x___x___ ,
 /*EB8A*/ GENx___x___x900 (set_queue_buffer_state,RSY,"SQBS"),
 /*EB8B*/ GENx___x___x___ ,
 /*EB8C*/ GENx___x___x___ ,
 /*EB8D*/ GENx___x___x___ ,
 /*EB8E*/ GENx37Xx390x900 (move_long_unicode,RSY,"MVCLU"),
 /*EB8F*/ GENx37Xx390x900 (compare_logical_long_unicode,RSY,"CLCLU"),
 /*EB90*/ GENx___x___x900 (store_multiple_y,RSY,"STMY"),
 /*EB91*/ GENx___x___x___ ,
 /*EB92*/ GENx___x___x___ ,
 /*EB93*/ GENx___x___x___ ,
 /*EB94*/ GENx___x___x___ ,
 /*EB95*/ GENx___x___x___ ,
 /*EB96*/ GENx___x___x900 (load_multiple_high,RSY,"LMH"),
 /*EB97*/ GENx___x___x___ ,
 /*EB98*/ GENx___x___x900 (load_multiple_y,RSY,"LMY"),
 /*EB99*/ GENx___x___x___ ,
 /*EB9A*/ GENx___x___x900 (load_access_multiple_y,RSY,"LAMY"),
 /*EB9B*/ GENx___x___x900 (store_access_multiple_y,RSY,"STAMY"),
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
 /*EBC0*/ GENx37Xx390x900 (test_decimal,RSL,"TP"),
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
 /*EBDC*/ GENx37Xx390x900 (shift_right_single_distinct,RSY,"SRAK"),                /*810*/
 /*EBDD*/ GENx37Xx390x900 (shift_left_single_distinct,RSY,"SLAK"),                 /*810*/
 /*EBDE*/ GENx37Xx390x900 (shift_right_single_logical_distinct,RSY,"SRLK"),        /*810*/
 /*EBDF*/ GENx37Xx390x900 (shift_left_single_logical_distinct,RSY,"SLLK"),         /*810*/
 /*EBE0*/ GENx___x___x___ ,
 /*EBE1*/ GENx___x___x___ ,
 /*EBE2*/ GENx___x___x900 (load_on_condition_long,RSY_M3,"LOCG"),                  /*810*/
 /*EBE3*/ GENx___x___x900 (store_on_condition_long,RSY_M3,"STOCG"),                /*810*/
 /*EBE4*/ GENx___x___x900 (load_and_and_long,RSY,"LANG"),                          /*810*/
 /*EBE5*/ GENx___x___x___ ,
 /*EBE6*/ GENx___x___x900 (load_and_or_long,RSY,"LAOG"),                           /*810*/
 /*EBE7*/ GENx___x___x900 (load_and_exclusive_or_long,RSY,"LAXG"),                 /*810*/
 /*EBE8*/ GENx___x___x900 (load_and_add_long,RSY,"LAAG"),                          /*810*/
 /*EBE9*/ GENx___x___x___ ,
 /*EBEA*/ GENx___x___x900 (load_and_add_logical_long,RSY,"LAALG"),                 /*810*/
 /*EBEB*/ GENx___x___x___ ,
 /*EBEC*/ GENx___x___x___ ,
 /*EBED*/ GENx___x___x___ ,
 /*EBEE*/ GENx___x___x___ ,
 /*EBEF*/ GENx___x___x___ ,
 /*EBF0*/ GENx___x___x___ ,
 /*EBF1*/ GENx___x___x___ ,
 /*EBF2*/ GENx37Xx390x900 (load_on_condition,RSY_M3,"LOC"),                        /*810*/
 /*EBF3*/ GENx37Xx390x900 (store_on_condition,RSY_M3,"STOC"),                      /*810*/
 /*EBF4*/ GENx37Xx390x900 (load_and_and,RSY,"LAN"),                                /*810*/
 /*EBF5*/ GENx___x___x___ ,
 /*EBF6*/ GENx37Xx390x900 (load_and_or,RSY,"LAO"),                                 /*810*/
 /*EBF7*/ GENx37Xx390x900 (load_and_exclusive_or,RSY,"LAX"),                       /*810*/
 /*EBF8*/ GENx37Xx390x900 (load_and_add,RSY,"LAA"),                                /*810*/
 /*EBF9*/ GENx___x___x___ ,
 /*EBFA*/ GENx37Xx390x900 (load_and_add_logical,RSY,"LAAL"),                       /*810*/
 /*EBFB*/ GENx___x___x___ ,
 /*EBFC*/ GENx___x___x___ ,
 /*EBFD*/ GENx___x___x___ ,
 /*EBFE*/ GENx___x___x___ ,
 /*EBFF*/ GENx___x___x___  };


static zz_func opcode_ecxx[0x100][GEN_MAXARCH] = {
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
 /*EC42*/ GENx___x___x___ ,
 /*EC43*/ GENx___x___x___ ,
 /*EC44*/ GENx___x___x900 (branch_relative_on_index_high_long,RIE,"BRXHG"),
 /*EC45*/ GENx___x___x900 (branch_relative_on_index_low_or_equal_long,RIE,"BRXLG"),
 /*EC46*/ GENx___x___x___ ,
 /*EC47*/ GENx___x___x___ ,
 /*EC48*/ GENx___x___x___ ,
 /*EC49*/ GENx___x___x___ ,
 /*EC4A*/ GENx___x___x___ ,
 /*EC4B*/ GENx___x___x___ ,
 /*EC4C*/ GENx___x___x___ ,
 /*EC4D*/ GENx___x___x___ ,
 /*EC4E*/ GENx___x___x___ ,
 /*EC4F*/ GENx___x___x___ ,
 /*EC50*/ GENx___x___x___ ,
 /*EC51*/ GENx___x___x900 (rotate_then_insert_selected_bits_low_long_reg,RIE_RRIII,"RISBLG"),  /*810*/
 /*EC52*/ GENx___x___x___ ,
 /*EC53*/ GENx___x___x___ ,
 /*EC54*/ GENx___x___x900 (rotate_then_and_selected_bits_long_reg,RIE_RRIII,"RNSBG"),          /*208*/
 /*EC55*/ GENx___x___x900 (rotate_then_insert_selected_bits_long_reg,RIE_RRIII,"RISBG"),       /*208*/
 /*EC56*/ GENx___x___x900 (rotate_then_or_selected_bits_long_reg,RIE_RRIII,"ROSBG"),           /*208*/
 /*EC57*/ GENx___x___x900 (rotate_then_exclusive_or_selected_bits_long_reg,RIE_RRIII,"RXSBG"), /*208*/
 /*EC58*/ GENx___x___x___ ,
 /*EC59*/ GENx___x___x___ ,
 /*EC5A*/ GENx___x___x___ ,
 /*EC5B*/ GENx___x___x___ ,
 /*EC5C*/ GENx___x___x___ ,
 /*EC5D*/ GENx___x___x900 (rotate_then_insert_selected_bits_high_long_reg,RIE_RRIII,"RISBHG"), /*810*/
 /*EC5E*/ GENx___x___x___ ,
 /*EC5F*/ GENx___x___x___ ,
 /*EC60*/ GENx___x___x___ ,
 /*EC61*/ GENx___x___x___ ,
 /*EC62*/ GENx___x___x___ ,
 /*EC63*/ GENx___x___x___ ,
 /*EC64*/ GENx___x___x900 (compare_and_branch_relative_long_register,RIE_RRIM,"CGRJ"), /*208*/
 /*EC65*/ GENx___x___x900 (compare_logical_and_branch_relative_long_register,RIE_RRIM,"CLGRJ"), /*208*/
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
 /*EC70*/ GENx___x___x900 (compare_immediate_and_trap_long,RIE_RIM,"CGIT"),        /*208*/
 /*EC71*/ GENx___x___x900 (compare_logical_immediate_and_trap_long,RIE_RIM,"CLGIT"), /*208*/
 /*EC72*/ GENx37Xx390x900 (compare_immediate_and_trap,RIE_RIM,"CIT"),              /*208*/
 /*EC73*/ GENx37Xx390x900 (compare_logical_immediate_and_trap_fullword,RIE_RIM,"CLFIT"), /*208*/
 /*EC74*/ GENx___x___x___ ,
 /*EC75*/ GENx___x___x___ ,
 /*EC76*/ GENx37Xx390x900 (compare_and_branch_relative_register,RIE_RRIM,"CRJ"),   /*208*/
 /*EC77*/ GENx37Xx390x900 (compare_logical_and_branch_relative_register,RIE_RRIM,"CLRJ"), /*208*/
 /*EC78*/ GENx___x___x___ ,
 /*EC79*/ GENx___x___x___ ,
 /*EC7A*/ GENx___x___x___ ,
 /*EC7B*/ GENx___x___x___ ,
 /*EC7C*/ GENx___x___x900 (compare_immediate_and_branch_relative_long,RIE_RMII,"CGIJ"), /*208*/
 /*EC7D*/ GENx___x___x900 (compare_logical_immediate_and_branch_relative_long,RIE_RMII,"CLGIJ"), /*208*/
 /*EC7E*/ GENx37Xx390x900 (compare_immediate_and_branch_relative,RIE_RMII,"CIJ"),  /*208*/
 /*EC7F*/ GENx37Xx390x900 (compare_logical_immediate_and_branch_relative,RIE_RMII,"CLIJ"), /*208*/
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
 /*ECD8*/ GENx37Xx390x900 (add_distinct_halfword_immediate,RIE_RRI,"AHIK"),        /*810*/
 /*ECD9*/ GENx___x___x900 (add_distinct_long_halfword_immediate,RIE_RRI,"AGHIK"),  /*810*/
 /*ECDA*/ GENx37Xx390x900 (add_logical_distinct_signed_halfword_immediate,RIE_RRI,"ALHSIK"), /*810*/
 /*ECDB*/ GENx___x___x900 (add_logical_distinct_long_signed_halfword_immediate,RIE_RRI,"AGLHSIK"), /*810*/
 /*ECDC*/ GENx___x___x___ ,
 /*ECDD*/ GENx___x___x___ ,
 /*ECDE*/ GENx___x___x___ ,
 /*ECDF*/ GENx___x___x___ ,
 /*ECE0*/ GENx___x___x___ ,
 /*ECE1*/ GENx___x___x___ ,
 /*ECE2*/ GENx___x___x___ ,
 /*ECE3*/ GENx___x___x___ ,
 /*ECE4*/ GENx___x___x900 (compare_and_branch_long_register,RRS,"CGRB"),           /*208*/
 /*ECE5*/ GENx___x___x900 (compare_logical_and_branch_long_register,RRS,"CLGRB"),  /*208*/
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
 /*ECF6*/ GENx37Xx390x900 (compare_and_branch_register,RRS,"CRB"),                 /*208*/
 /*ECF7*/ GENx37Xx390x900 (compare_logical_and_branch_register,RRS,"CLRB"),        /*208*/
 /*ECF8*/ GENx___x___x___ ,
 /*ECF9*/ GENx___x___x___ ,
 /*ECFA*/ GENx___x___x___ ,
 /*ECFB*/ GENx___x___x___ ,
 /*ECFC*/ GENx___x___x900 (compare_immediate_and_branch_long,RIS,"CGIB"),          /*208*/
 /*ECFD*/ GENx___x___x900 (compare_logical_immediate_and_branch_long,RIS,"CLGIB"), /*208*/
 /*ECFE*/ GENx37Xx390x900 (compare_immediate_and_branch,RIS,"CIB"),                /*208*/
 /*ECFF*/ GENx37Xx390x900 (compare_logical_immediate_and_branch,RIS,"CLIB") };     /*208*/


static zz_func opcode_edxx[0x100][GEN_MAXARCH] = {
 /*ED00*/ GENx___x___x___ ,
 /*ED01*/ GENx___x___x___ ,
 /*ED02*/ GENx___x___x___ ,
 /*ED03*/ GENx___x___x___ ,
 /*ED04*/ GENx37Xx390x900 (load_lengthened_bfp_short_to_long,RXE,"LDEB"),
 /*ED05*/ GENx37Xx390x900 (load_lengthened_bfp_long_to_ext,RXE,"LXDB"),
 /*ED06*/ GENx37Xx390x900 (load_lengthened_bfp_short_to_ext,RXE,"LXEB"),
 /*ED07*/ GENx37Xx390x900 (multiply_bfp_long_to_ext,RXE,"MXDB"),
 /*ED08*/ GENx37Xx390x900 (compare_and_signal_bfp_short,RXE,"KEB"),
 /*ED09*/ GENx37Xx390x900 (compare_bfp_short,RXE,"CEB"),
 /*ED0A*/ GENx37Xx390x900 (add_bfp_short,RXE,"AEB"),
 /*ED0B*/ GENx37Xx390x900 (subtract_bfp_short,RXE,"SEB"),
 /*ED0C*/ GENx37Xx390x900 (multiply_bfp_short_to_long,RXE,"MDEB"),
 /*ED0D*/ GENx37Xx390x900 (divide_bfp_short,RXE,"DEB"),
 /*ED0E*/ GENx37Xx390x900 (multiply_add_bfp_short,RXF,"MAEB"),
 /*ED0F*/ GENx37Xx390x900 (multiply_subtract_bfp_short,RXF,"MSEB"),
 /*ED10*/ GENx37Xx390x900 (test_data_class_bfp_short,RXE,"TCEB"),
 /*ED11*/ GENx37Xx390x900 (test_data_class_bfp_long,RXE,"TCDB"),
 /*ED12*/ GENx37Xx390x900 (test_data_class_bfp_ext,RXE,"TCXB"),
 /*ED13*/ GENx___x___x___ ,
 /*ED14*/ GENx37Xx390x900 (squareroot_bfp_short,RXE,"SQEB"),
 /*ED15*/ GENx37Xx390x900 (squareroot_bfp_long,RXE,"SQDB"),
 /*ED16*/ GENx___x___x___ ,
 /*ED17*/ GENx37Xx390x900 (multiply_bfp_short,RXE,"MEEB"),
 /*ED18*/ GENx37Xx390x900 (compare_and_signal_bfp_long,RXE,"KDB"),
 /*ED19*/ GENx37Xx390x900 (compare_bfp_long,RXE,"CDB"),
 /*ED1A*/ GENx37Xx390x900 (add_bfp_long,RXE,"ADB"),
 /*ED1B*/ GENx37Xx390x900 (subtract_bfp_long,RXE,"SDB"),
 /*ED1C*/ GENx37Xx390x900 (multiply_bfp_long,RXE,"MDB"),
 /*ED1D*/ GENx37Xx390x900 (divide_bfp_long,RXE,"DDB"),
 /*ED1E*/ GENx37Xx390x900 (multiply_add_bfp_long,RXF,"MADB"),
 /*ED1F*/ GENx37Xx390x900 (multiply_subtract_bfp_long,RXF,"MSDB"),
 /*ED20*/ GENx___x___x___ ,
 /*ED21*/ GENx___x___x___ ,
 /*ED22*/ GENx___x___x___ ,
 /*ED23*/ GENx___x___x___ ,
 /*ED24*/ GENx37Xx390x900 (load_lengthened_float_short_to_long,RXE,"LDE"),
 /*ED25*/ GENx37Xx390x900 (load_lengthened_float_long_to_ext,RXE,"LXD"),
 /*ED26*/ GENx37Xx390x900 (load_lengthened_float_short_to_ext,RXE,"LXE"),
 /*ED27*/ GENx___x___x___ ,
 /*ED28*/ GENx___x___x___ ,
 /*ED29*/ GENx___x___x___ ,
 /*ED2A*/ GENx___x___x___ ,
 /*ED2B*/ GENx___x___x___ ,
 /*ED2C*/ GENx___x___x___ ,
 /*ED2D*/ GENx___x___x___ ,
 /*ED2E*/ GENx37Xx390x900 (multiply_add_float_short,RXF,"MAE"),
 /*ED2F*/ GENx37Xx390x900 (multiply_subtract_float_short,RXF,"MSE"),
 /*ED30*/ GENx___x___x___ ,
 /*ED31*/ GENx___x___x___ ,
 /*ED32*/ GENx___x___x___ ,
 /*ED33*/ GENx___x___x___ ,
 /*ED34*/ GENx37Xx390x900 (squareroot_float_short,RXE,"SQE"),
 /*ED35*/ GENx37Xx390x900 (squareroot_float_long,RXE,"SQD"),
 /*ED36*/ GENx___x___x___ ,
 /*ED37*/ GENx37Xx390x900 (multiply_float_short,RXE,"MEE"),
 /*ED38*/ GENx37Xx___x900 (multiply_add_unnormal_float_long_to_ext_low,RXF,"MAYL"),  /*@Z9*/
 /*ED39*/ GENx37Xx___x900 (multiply_unnormal_float_long_to_ext_low,RXF,"MYL"),       /*@Z9*/
 /*ED3A*/ GENx37Xx___x900 (multiply_add_unnormal_float_long_to_ext,RXF,"MAY"),       /*@Z9*/
 /*ED3B*/ GENx37Xx___x900 (multiply_unnormal_float_long_to_ext,RXF,"MY"),            /*@Z9*/
 /*ED3C*/ GENx37Xx___x900 (multiply_add_unnormal_float_long_to_ext_high,RXF,"MAYH"), /*@Z9*/
 /*ED3D*/ GENx37Xx___x900 (multiply_unnormal_float_long_to_ext_high,RXF,"MYH"),      /*@Z9*/
 /*ED3E*/ GENx37Xx390x900 (multiply_add_float_long,RXF,"MAD"),
 /*ED3F*/ GENx37Xx390x900 (multiply_subtract_float_long,RXF,"MSD"),
 /*ED40*/ GENx___x390x900 (shift_coefficient_left_dfp_long,RXF,"SLDT"),
 /*ED41*/ GENx___x390x900 (shift_coefficient_right_dfp_long,RXF,"SRDT"),
 /*ED42*/ GENx___x___x___ ,
 /*ED43*/ GENx___x___x___ ,
 /*ED44*/ GENx___x___x___ ,
 /*ED45*/ GENx___x___x___ ,
 /*ED46*/ GENx___x___x___ ,
 /*ED47*/ GENx___x___x___ ,
 /*ED48*/ GENx___x390x900 (shift_coefficient_left_dfp_ext,RXF,"SLXT"),
 /*ED49*/ GENx___x390x900 (shift_coefficient_right_dfp_ext,RXF,"SRXT"),
 /*ED4A*/ GENx___x___x___ ,
 /*ED4B*/ GENx___x___x___ ,
 /*ED4C*/ GENx___x___x___ ,
 /*ED4D*/ GENx___x___x___ ,
 /*ED4E*/ GENx___x___x___ ,
 /*ED4F*/ GENx___x___x___ ,
 /*ED50*/ GENx___x390x900 (test_data_class_dfp_short,RXE,"TDCET"),
 /*ED51*/ GENx___x390x900 (test_data_group_dfp_short,RXE,"TDGET"),
 /*ED52*/ GENx___x___x___ ,
 /*ED53*/ GENx___x___x___ ,
 /*ED54*/ GENx___x390x900 (test_data_class_dfp_long,RXE,"TDCDT"),
 /*ED55*/ GENx___x390x900 (test_data_group_dfp_long,RXE,"TDGDT"),
 /*ED56*/ GENx___x___x___ ,
 /*ED57*/ GENx___x___x___ ,
 /*ED58*/ GENx___x390x900 (test_data_class_dfp_ext,RXE,"TDCXT"),
 /*ED59*/ GENx___x390x900 (test_data_group_dfp_ext,RXE,"TDGXT"),
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
 /*ED64*/ GENx___x___x900 (load_float_short_y,RXY,"LEY"),
 /*ED65*/ GENx___x___x900 (load_float_long_y,RXY,"LDY"),
 /*ED66*/ GENx___x___x900 (store_float_short_y,RXY,"STEY"),
 /*ED67*/ GENx___x___x900 (store_float_long_y,RXY,"STDY"),
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
 /*EDA8*/ GENx___x___x___ ,
 /*EDA9*/ GENx___x___x___ ,
 /*EDAA*/ GENx___x___x___ ,
 /*EDAB*/ GENx___x___x___ ,
 /*EDAC*/ GENx___x___x___ ,
 /*EDAD*/ GENx___x___x___ ,
 /*EDAE*/ GENx___x___x___ ,
 /*EDAF*/ GENx___x___x___ ,
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
 /*EDFF*/ GENx___x___x___  };


static zz_func v_opcode_a4xx[0x100][GEN_MAXARCH] = {
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
 /*A4FF*/ GENx___x___x___  };


static zz_func v_opcode_a5xx[0x100][GEN_MAXARCH] = {
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
 /*A5FF*/ GENx___x___x___  };


static zz_func v_opcode_a6xx[0x100][GEN_MAXARCH] = {
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
 /*A640*/ GENx370x390x___ (v_test_vmr,RRE,"VTVM"),
 /*A641*/ GENx370x390x___ (v_complement_vmr,RRE,"VCVM"),
 /*A642*/ GENx370x390x___ (v_count_left_zeros_in_vmr,RRE,"VCZVM"),
 /*A643*/ GENx370x390x___ (v_count_ones_in_vmr,RRE,"VCOVM"),
 /*A644*/ GENx370x390x___ (v_extract_vct,RRE,"VXVC"),
 /*A645*/ GENx___x___x___ , /* VLVCU */
 /*A646*/ GENx370x390x___ (v_extract_vector_modes,RRE,"VXVMM"),
 /*A647*/ GENx___x___x___ ,
 /*A648*/ GENx370x390x___ (v_restore_vr,RRE,"VRRS"),
 /*A649*/ GENx370x390x___ (v_save_changed_vr,RRE,"VRSVC"),
 /*A64A*/ GENx370x390x___ (v_save_vr,RRE,"VRSV"),
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
 /*A680*/ GENx370x390x___ (v_load_vmr,VS,"VLVM"),
 /*A681*/ GENx370x390x___ (v_load_vmr_complement,VS,"VLCVM"),
 /*A682*/ GENx370x390x___ (v_store_vmr,VS,"VSTVM"),
 /*A683*/ GENx___x___x___ ,
 /*A684*/ GENx370x390x___ (v_and_to_vmr,VS,"VNVM"),
 /*A685*/ GENx370x390x___ (v_or_to_vmr,VS,"VOVM"),
 /*A686*/ GENx370x390x___ (v_exclusive_or_to_vmr,VS,"VXVM"),
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
 /*A6C0*/ GENx370x390x___ (v_save_vsr,S,"VSRSV"),
 /*A6C1*/ GENx370x390x___ (v_save_vmr,S,"VMRSV"),
 /*A6C2*/ GENx370x390x___ (v_restore_vsr,S,"VSRRS"),
 /*A6C3*/ GENx370x390x___ (v_restore_vmr,S,"VMRRS"),
 /*A6C4*/ GENx370x390x___ (v_load_vct_from_address,S,"VLVCA"),
 /*A6C5*/ GENx370x390x___ (v_clear_vr,S,"VRCL"),
 /*A6C6*/ GENx370x390x___ (v_set_vector_mask_mode,S,"VSVMM"),
 /*A6C7*/ GENx370x390x___ (v_load_vix_from_address,S,"VLVXA"),
 /*A6C8*/ GENx370x390x___ (v_store_vector_parameters,S,"VSTVP"),
 /*A6C9*/ GENx___x___x___ ,
 /*A6CA*/ GENx370x390x___ (v_save_vac,S,"VACSV"),
 /*A6CB*/ GENx370x390x___ (v_restore_vac,S,"VACRS"),
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
 /*A6FF*/ GENx___x___x___  };


static zz_func v_opcode_e4xx[0x100][GEN_MAXARCH] = {
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
 /*E4FF*/ GENx___x___x___  };

#ifdef OPTION_OPTINST
#define CLRgen(r1, r2) GENx370x390x900 (15 ## r1 ## r2,RR,"LR")
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

static zz_func opcode_15__[0x100][GEN_MAXARCH] = {
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
  CLRgenr2(F) };

#define LRgen(r1, r2) GENx370x390x900 (18 ## r1 ## r2,RR,"LR")
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

static zz_func opcode_18__[0x100][GEN_MAXARCH] = {
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
  LRgenr2(F) };

#define ALRgen(r1, r2) GENx370x390x900 (1E ## r1 ## r2,RR,"ALR")
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

static zz_func opcode_1E__[0x100][GEN_MAXARCH] = {
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
  ALRgenr2(F) };

#define SLRgen(r1, r2) GENx370x390x900 (1F ## r1 ## r2,RR,"SLR")
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

static zz_func opcode_1F__[0x100][GEN_MAXARCH] = {
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
  SLRgenr2(F) };

static zz_func opcode_41_0[0x10][GEN_MAXARCH] = {
 /*4100*/ GENx370x390x900 (4100,RX,"LA"),
 /*4110*/ GENx370x390x900 (4110,RX,"LA"),
 /*4120*/ GENx370x390x900 (4120,RX,"LA"),
 /*4134*/ GENx370x390x900 (4130,RX,"LA"),
 /*4140*/ GENx370x390x900 (4140,RX,"LA"),
 /*4150*/ GENx370x390x900 (4150,RX,"LA"),
 /*4160*/ GENx370x390x900 (4160,RX,"LA"),
 /*4170*/ GENx370x390x900 (4170,RX,"LA"),
 /*4180*/ GENx370x390x900 (4180,RX,"LA"),
 /*4190*/ GENx370x390x900 (4190,RX,"LA"),
 /*41A0*/ GENx370x390x900 (41A0,RX,"LA"),
 /*41B0*/ GENx370x390x900 (41B0,RX,"LA"),
 /*41C0*/ GENx370x390x900 (41C0,RX,"LA"),
 /*41D0*/ GENx370x390x900 (41D0,RX,"LA"),
 /*41E0*/ GENx370x390x900 (41E0,RX,"LA"),
 /*41F0*/ GENx370x390x900 (41F0,RX,"LA") };

static zz_func opcode_47_0[0x10][GEN_MAXARCH] = {
 /*4700*/ GENx370x390x900 (nop4,RX,"BC"),
 /*4710*/ GENx370x390x900 (4710,RX,"BC"),
 /*4720*/ GENx370x390x900 (4720,RX,"BC"),
 /*4734*/ GENx370x390x900 (4730,RX,"BC"),
 /*4740*/ GENx370x390x900 (4740,RX,"BC"),
 /*4750*/ GENx370x390x900 (4750,RX,"BC"),
 /*4760*/ GENx370x390x900 (47_0,RX,"BC"),
 /*4770*/ GENx370x390x900 (4770,RX,"BC"),
 /*4780*/ GENx370x390x900 (4780,RX,"BC"),
 /*4790*/ GENx370x390x900 (47_0,RX,"BC"),
 /*47A0*/ GENx370x390x900 (47A0,RX,"BC"),
 /*47B0*/ GENx370x390x900 (47B0,RX,"BC"),
 /*47C0*/ GENx370x390x900 (47C0,RX,"BC"),
 /*47D0*/ GENx370x390x900 (47D0,RX,"BC"),
 /*47E0*/ GENx370x390x900 (47E0,RX,"BC"),
 /*47F0*/ GENx370x390x900 (47F0,RX,"BC") };

static zz_func opcode_50_0[0x10][GEN_MAXARCH] = {
 /*5000*/ GENx370x390x900 (5000,RX,"ST"),
 /*5010*/ GENx370x390x900 (5010,RX,"ST"),
 /*5020*/ GENx370x390x900 (5020,RX,"ST"),
 /*5030*/ GENx370x390x900 (5030,RX,"ST"),
 /*5040*/ GENx370x390x900 (5040,RX,"ST"),
 /*5050*/ GENx370x390x900 (5050,RX,"ST"),
 /*5060*/ GENx370x390x900 (5060,RX,"ST"),
 /*5070*/ GENx370x390x900 (5070,RX,"ST"),
 /*5080*/ GENx370x390x900 (5080,RX,"ST"),
 /*5090*/ GENx370x390x900 (5090,RX,"ST"),
 /*50A0*/ GENx370x390x900 (50A0,RX,"ST"),
 /*50B0*/ GENx370x390x900 (50B0,RX,"ST"),
 /*50C0*/ GENx370x390x900 (50C0,RX,"ST"),
 /*50D0*/ GENx370x390x900 (50D0,RX,"ST"),
 /*50E0*/ GENx370x390x900 (50E0,RX,"ST"),
 /*50F0*/ GENx370x390x900 (50F0,RX,"ST") };

static zz_func opcode_55_0[0x10][GEN_MAXARCH] = {
 /*5500*/ GENx370x390x900 (5500,RX,"CL"),
 /*5510*/ GENx370x390x900 (5510,RX,"CL"),
 /*5520*/ GENx370x390x900 (5520,RX,"CL"),
 /*5530*/ GENx370x390x900 (5530,RX,"CL"),
 /*5540*/ GENx370x390x900 (5540,RX,"CL"),
 /*5550*/ GENx370x390x900 (5550,RX,"CL"),
 /*5560*/ GENx370x390x900 (5560,RX,"CL"),
 /*5570*/ GENx370x390x900 (5570,RX,"CL"),
 /*5580*/ GENx370x390x900 (5580,RX,"CL"),
 /*5590*/ GENx370x390x900 (5590,RX,"CL"),
 /*55A0*/ GENx370x390x900 (55A0,RX,"CL"),
 /*55B0*/ GENx370x390x900 (55B0,RX,"CL"),
 /*55C0*/ GENx370x390x900 (55C0,RX,"CL"),
 /*55D0*/ GENx370x390x900 (55D0,RX,"CL"),
 /*55E0*/ GENx370x390x900 (55E0,RX,"CL"),
 /*55F0*/ GENx370x390x900 (55F0,RX,"CL") };

static zz_func opcode_58_0[0x10][GEN_MAXARCH] = {
 /*5800*/ GENx370x390x900 (5800,RX,"L"),
 /*5810*/ GENx370x390x900 (5810,RX,"L"),
 /*5820*/ GENx370x390x900 (5820,RX,"L"),
 /*5830*/ GENx370x390x900 (5830,RX,"L"),
 /*5840*/ GENx370x390x900 (5840,RX,"L"),
 /*5850*/ GENx370x390x900 (5850,RX,"L"),
 /*5860*/ GENx370x390x900 (5860,RX,"L"),
 /*5870*/ GENx370x390x900 (5870,RX,"L"),
 /*5880*/ GENx370x390x900 (5880,RX,"L"),
 /*5890*/ GENx370x390x900 (5890,RX,"L"),
 /*58A0*/ GENx370x390x900 (58A0,RX,"L"),
 /*58B0*/ GENx370x390x900 (58B0,RX,"L"),
 /*58C0*/ GENx370x390x900 (58C0,RX,"L"),
 /*58D0*/ GENx370x390x900 (58D0,RX,"L"),
 /*58E0*/ GENx370x390x900 (58E0,RX,"L"),
 /*58F0*/ GENx370x390x900 (58F0,RX,"L") };

static zz_func opcode_91xx[0x08][GEN_MAXARCH] = {
 /*9180*/ GENx370x390x900 (9180,SI,"TM"),
 /*9140*/ GENx370x390x900 (9140,SI,"TM"),
 /*9120*/ GENx370x390x900 (9120,SI,"TM"),
 /*9110*/ GENx370x390x900 (9110,SI,"TM"),
 /*9108*/ GENx370x390x900 (9108,SI,"TM"),
 /*9104*/ GENx370x390x900 (9104,SI,"TM"),
 /*9102*/ GENx370x390x900 (9102,SI,"TM"),
 /*9101*/ GENx370x390x900 (9101,SI,"TM") }; /* Single bit TM */

static zz_func opcode_A7_4[0x10][GEN_MAXARCH] = {
 /*A704*/ GENx370x390x900 (nop4,RX,"BRC"),
 /*A714*/ GENx370x390x900 (A714,RX,"BRC"),
 /*A724*/ GENx370x390x900 (A724,RX,"BRC"),
 /*A734*/ GENx370x390x900 (A734,RX,"BRC"),
 /*A744*/ GENx370x390x900 (A744,RX,"BRC"),
 /*A754*/ GENx370x390x900 (A754,RX,"BRC"),
 /*A764*/ GENx370x390x900 (branch_relative_on_condition,RI_B,"BRC"),
 /*A774*/ GENx370x390x900 (A774,RX,"BRC"),
 /*A784*/ GENx370x390x900 (A784,RX,"BRC"),
 /*A794*/ GENx370x390x900 (branch_relative_on_condition,RI_B,"BRC"),
 /*A7A4*/ GENx370x390x900 (A7A4,RX,"BRC"),
 /*A7B4*/ GENx370x390x900 (A7B4,RX,"BRC"),
 /*A7C4*/ GENx370x390x900 (A7C4,RX,"BRC"),
 /*A7D4*/ GENx370x390x900 (A7D4,RX,"BRC"),
 /*A7E4*/ GENx370x390x900 (A7E4,RX,"BRC"),
 /*A7F4*/ GENx370x390x900 (A7F4,RX,"BRC") };

static zz_func opcode_BF_x[0x03][GEN_MAXARCH] = {
 /*BF_x*/ GENx370x390x900 (BF_x,RS,"ICM"),
 /*BF_7*/ GENx370x390x900 (BF_7,RS,"ICM"),
 /*BF_F*/ GENx370x390x900 (BF_F,RS,"ICM") };

static zz_func opcode_D20x[0x01][GEN_MAXARCH] = {
 /*D200*/ GENx370x390x900 (D200,SS,"MVC") };

static zz_func opcode_D50x[0x04][GEN_MAXARCH] = {
 /*D500*/ GENx370x390x900 (D500,SS,"CLC"),
 /*D501*/ GENx370x390x900 (D501,SS,"CLC"),
 /*D503*/ GENx370x390x900 (D503,SS,"CLC"),
 /*D507*/ GENx370x390x900 (D507,SS,"CLC") };

static zz_func opcode_E3_0[0x01][GEN_MAXARCH] = {
 /*E3*/   GENx370x390x900 (E3_0,e3xx,"") };

static zz_func opcode_E3_0______04[0x01][GEN_MAXARCH] = {
 /*E304*/ GENx___x___x900 (E3_0______04,RXY,"LG") };

static zz_func opcode_E3_0______24[0x01][GEN_MAXARCH] = {
 /*E324*/ GENx___x___x900 (E3_0______24,RXY,"STG") };

#endif /* OPTION_OPTINST */

#endif /*!defined (_GEN_ARCH)*/

/* end of OPCODE.C */
