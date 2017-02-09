/* S37X.C       (c) Copyright Ivan S. Warren 2010                    */
/*              Optional S370 Extensions                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"
#include "hercules.h"
#include "hdl.h"
#include "opcode.h"

#if defined(_370)

#if defined(OPTION_370_EXTENSION)

#if 0
static void    *(*resolver)(char *ep);
#endif

/**********************************************************/
/* Table structures                                       */
/**********************************************************/

/* This is the entry for an instruction to be replaced */
struct  s37x_inst_table_entry
{
    zz_func newfun;   /* New function */
    zz_func oldfun;   /* Save of old function */
    int index;          /* Index in table */
};

struct  s37x_inst_table_table_entry
{
    struct s37x_inst_table_entry *tbl; /* Ptr to table of instructions to replace */
};

struct  s37x_inst_table_header {
    struct  s37x_inst_table_entry   *tbl;
    int     ix;
};

/**********************************************************/
/* Definition macros                                      */
/**********************************************************/
/* INST37X_TABLE_START
   Place at the start of each series of opcodes
   opcode 0 is for 1st level table (opcode_table)
   others for corresponding (eg: c0 is for opcode_table_c0xx */
#define INST37X_TABLE_START(_tbn) \
static struct s37x_inst_table_entry s37x_table_ ## _tbn[] = {

/* INST37X
   The instruction itself. 1st parm is function address
   2nd parm is opcode or opcode extension */

#define INST37X( _ifun, _ix ) \
    { s370_ ## _ifun, NULL, (_ix) },

/* INST37X_TABLE_END
   Closes an opcode table. Defines the properties
   of that table (the array of opcode entries and
   the actual opcode used */
#define INST37X_TABLE_END(_tbn) \
    { NULL, NULL, -1 }}; \
static struct s37x_inst_table_header s37x_inst_table_header_ ## _tbn = { \
    s37x_table_ ## _tbn, \
    0x ## _tbn };

/* INST37X_INSERT_TABLE_START
   The table of opcode tables */
#define INST37X_INSERT_TABLE_START \
static struct s37x_inst_table_header *s37x_table_table[] = {

/* The table references */
#define INST37X_INSERT_TABLE( _tbl ) \
    &s37x_inst_table_header_ ## _tbl, \

/* End of table */
#define INST37X_INSERT_TABLE_END \
    NULL };

/* STRUCTURE :
   s37x_table_table is a NULL terminated  array of pointers to
        s37x_inst_table_header structures

   Each s37x_inst_table_header (called s37x_inst_table_header_XX)
        is a structure that contain the relevant opcode extension
        for multibyte opcodes or 0 for the top level opcode table and
        a pointer to an array.

   This array pointed to by the header is an array of structures
       which contain
       - The new instruction function address
       - An area to store the old function address
       - The opcode the function refers to (or extension thereof)
       The array is terminated by a structure that contains a NULL
       function address for the new instruction function */



/**********************************************************/
/* The actual instruction tables                          */
/**********************************************************/
INST37X_TABLE_START(00)
 /*71*/   INST37X (multiply_single,0x71)
 /*84*/   INST37X (branch_relative_on_index_high,0x84)
 /*85*/   INST37X (branch_relative_on_index_low_or_equal,0x85)
 /*A8*/   INST37X (move_long_extended,0xA8)
 /*A9*/   INST37X (compare_logical_long_extended,0xA9)
 /*D0*/   INST37X (translate_and_test_reverse,0xd0)
 /*E1*/   INST37X (pack_unicode,0xe1)
 /*E2*/   INST37X (unpack_unicode,0xe2)
 /*E9*/   INST37X (pack_ascii,0xe9)
 /*EA*/   INST37X (unpack_ascii,0xea)
INST37X_TABLE_END(00)

INST37X_TABLE_START(a7)
 /*A7x0*/ INST37X (test_under_mask_high,0)
 /*A7x1*/ INST37X (test_under_mask_low,1)
 /*A7x4*/ INST37X (branch_relative_on_condition,4)
 /*A7x5*/ INST37X (branch_relative_and_save,5)
 /*A7x6*/ INST37X (branch_relative_on_count,6)
 /*A7x8*/ INST37X (load_halfword_immediate,8)
 /*A7xA*/ INST37X (add_halfword_immediate,0xa)
 /*A7xC*/ INST37X (multiply_halfword_immediate,0xc)
 /*A7xE*/ INST37X (compare_halfword_immediate,0xe)
INST37X_TABLE_END(a7)


INST37X_TABLE_START(b2)
 /*B241*/ INST37X (checksum,0x41)
 /*B244*/ INST37X (squareroot_float_long_reg,0x44)
 /*B245*/ INST37X (squareroot_float_short_reg,0x45)
 /*B252*/ INST37X (multiply_single_register,0x52)
 /*B255*/ INST37X (move_string,0x55)
 /*B257*/ INST37X (compare_until_substring_equal,0x57)
 /*B25D*/ INST37X (compare_logical_string,0x5d)
 /*B25E*/ INST37X (search_string,0x5e)
 /*B263*/ INST37X (cmpsc_2012,0x63)
#if defined(FEATURE_BINARY_FLOATING_POINT)
 /*B299*/ INST37X (set_bfp_rounding_mode_2bit,0x99)
 /*B29C*/ INST37X (store_fpc,0x9c)
 /*B29D*/ INST37X (load_fpc,0x9d)
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/
 /*B2A5*/ INST37X (translate_extended,0xa5)
 /*B2A6*/ INST37X (convert_utf16_to_utf8,0xa6)
 /*B2A7*/ INST37X (convert_utf8_to_utf16,0xa7)
#if defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)         /*810*/
// /*B2B8*/ INST37X (set_bfp_rounding_mode_3bit,0xb8)             /*810*/
#endif /*defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)*/  /*810*/
// /*B2BD*/ INST37X (load_fpc_and_signal,0xbd)
INST37X_TABLE_END(b2)

INST37X_TABLE_START(b3)
#if defined(FEATURE_BINARY_FLOATING_POINT)
 /*B300*/ INST37X (load_positive_bfp_short_reg,0x00)
 /*B301*/ INST37X (load_negative_bfp_short_reg,0x01)
 /*B302*/ INST37X (load_and_test_bfp_short_reg,0x02)
 /*B303*/ INST37X (load_complement_bfp_short_reg,0x03)
 /*B304*/ INST37X (load_lengthened_bfp_short_to_long_reg,0x04)
 /*B305*/ INST37X (load_lengthened_bfp_long_to_ext_reg,0x05)
 /*B306*/ INST37X (load_lengthened_bfp_short_to_ext_reg,0x06)
 /*B307*/ INST37X (multiply_bfp_long_to_ext_reg,0x07)
 /*B308*/ INST37X (compare_and_signal_bfp_short_reg,0x08)
 /*B309*/ INST37X (compare_bfp_short_reg,0x09)
 /*B30A*/ INST37X (add_bfp_short_reg,0x0a)
 /*B30B*/ INST37X (subtract_bfp_short_reg,0x0b)
 /*B30C*/ INST37X (multiply_bfp_short_to_long_reg,0x0c)
 /*B30D*/ INST37X (divide_bfp_short_reg,0x0d)
 /*B30E*/ INST37X (multiply_add_bfp_short_reg,0x0e)
 /*B30F*/ INST37X (multiply_subtract_bfp_short_reg,0x0f)
 /*B310*/ INST37X (load_positive_bfp_long_reg,0x10)
 /*B311*/ INST37X (load_negative_bfp_long_reg,0x11)
 /*B312*/ INST37X (load_and_test_bfp_long_reg,0x12)
 /*B313*/ INST37X (load_complement_bfp_long_reg,0x13)
 /*B314*/ INST37X (squareroot_bfp_short_reg,0x14)
 /*B315*/ INST37X (squareroot_bfp_long_reg,0x15)
 /*B316*/ INST37X (squareroot_bfp_ext_reg,0x16)
 /*B317*/ INST37X (multiply_bfp_short_reg,0x17)
 /*B318*/ INST37X (compare_and_signal_bfp_long_reg,0x18)
 /*B319*/ INST37X (compare_bfp_long_reg,0x19)
 /*B31A*/ INST37X (add_bfp_long_reg,0x1a)
 /*B31B*/ INST37X (subtract_bfp_long_reg,0x1b)
 /*B31C*/ INST37X (multiply_bfp_long_reg,0x1c)
 /*B31D*/ INST37X (divide_bfp_long_reg,0x1d)
 /*B31E*/ INST37X (multiply_add_bfp_long_reg,0x1e)
 /*B31F*/ INST37X (multiply_subtract_bfp_long_reg,0x1f)
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/
 /*B324*/ INST37X (load_lengthened_float_short_to_long_reg,0x24)
 /*B325*/ INST37X (load_lengthened_float_long_to_ext_reg,0x25)
 /*B326*/ INST37X (load_lengthened_float_short_to_ext_reg,0x26)
 /*B32E*/ INST37X (multiply_add_float_short_reg,0x2e)
 /*B32F*/ INST37X (multiply_subtract_float_short_reg,0x2f)
 /*B336*/ INST37X (squareroot_float_ext_reg,0x36)
 /*B337*/ INST37X (multiply_float_short_reg,0x37)
 /*B338*/ INST37X (multiply_add_unnormal_float_long_to_ext_low_reg,0x38)
 /*B339*/ INST37X (multiply_unnormal_float_long_to_ext_low_reg,0x39)
 /*B33A*/ INST37X (multiply_add_unnormal_float_long_to_ext_reg,0x3a)
 /*B33B*/ INST37X (multiply_unnormal_float_long_to_ext_reg,0x3b)
 /*B33C*/ INST37X (multiply_add_unnormal_float_long_to_ext_high_reg,0x3c)
 /*B33D*/ INST37X (multiply_unnormal_float_long_to_ext_high_reg,0x3d)
 /*B33E*/ INST37X (multiply_add_float_long_reg,0x3e)
 /*B33F*/ INST37X (multiply_subtract_float_long_reg,0x3f)
#if defined(FEATURE_BINARY_FLOATING_POINT)
 /*B340*/ INST37X (load_positive_bfp_ext_reg,0x40)
 /*B341*/ INST37X (load_negative_bfp_ext_reg,0x41)
 /*B342*/ INST37X (load_and_test_bfp_ext_reg,0x42)
 /*B343*/ INST37X (load_complement_bfp_ext_reg,0x43)
 /*B344*/ INST37X (load_rounded_bfp_long_to_short_reg,0x44)
 /*B345*/ INST37X (load_rounded_bfp_ext_to_long_reg,0x45)
 /*B346*/ INST37X (load_rounded_bfp_ext_to_short_reg,0x46)
 /*B347*/ INST37X (load_fp_int_bfp_ext_reg,0x47)
 /*B348*/ INST37X (compare_and_signal_bfp_ext_reg,0x48)
 /*B349*/ INST37X (compare_bfp_ext_reg,0x49)
 /*B34A*/ INST37X (add_bfp_ext_reg,0x4a)
 /*B34B*/ INST37X (subtract_bfp_ext_reg,0x4b)
 /*B34C*/ INST37X (multiply_bfp_ext_reg,0x4c)
 /*B34D*/ INST37X (divide_bfp_ext_reg,0x4d)
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/
#if defined(FEATURE_FPS_EXTENSIONS)
// /*B350*/ INST37X (convert_float_long_to_bfp_short_reg,0x50)
// /*B351*/ INST37X (convert_float_long_to_bfp_long_reg,0x51)
// /*B358*/ INST37X (convert_bfp_short_to_float_long_reg,0x58)
// /*B359*/ INST37X (convert_bfp_long_to_float_long_reg,0x59)
// /*B365*/ INST37X (load_float_ext_reg,0x65)
// /*B374*/ INST37X (load_zero_float_short_reg,0x74)
// /*B375*/ INST37X (load_zero_float_long_reg,0x75)
// /*B376*/ INST37X (load_zero_float_ext_reg,0x76)
#endif // defined(FEATURE_FPS_EXTENSIONS)
#if defined(FEATURE_BINARY_FLOATING_POINT)
 /*B353*/ INST37X (divide_integer_bfp_short_reg,0x53)
 /*B357*/ INST37X (load_fp_int_bfp_short_reg,0x57)
 /*B35B*/ INST37X (divide_integer_bfp_long_reg,0x5b)
 /*B35F*/ INST37X (load_fp_int_bfp_long_reg,0x5f)
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/
 /*B360*/ INST37X (load_positive_float_ext_reg,0x60)
 /*B361*/ INST37X (load_negative_float_ext_reg,0x61)
 /*B362*/ INST37X (load_and_test_float_ext_reg,0x62)
 /*B363*/ INST37X (load_complement_float_ext_reg,0x63)
 /*B366*/ INST37X (load_rounded_float_ext_to_short_reg,0x66)
 /*B367*/ INST37X (load_fp_int_float_ext_reg,0x67)
 /*B369*/ INST37X (compare_float_ext_reg,0x69)
// /*B370*/ INST37X (load_positive_fpr_long_reg,0x70)
// /*B371*/ INST37X (load_negative_fpr_long_reg,0x71)
// /*B372*/ INST37X (copy_sign_fpr_long_reg,0x72)
// /*B373*/ INST37X (load_complement_fpr_long_reg,0x73)
 /*B377*/ INST37X (load_fp_int_float_short_reg,0x77)
 /*B37F*/ INST37X (load_fp_int_float_long_reg,0x7f)
#if defined(FEATURE_BINARY_FLOATING_POINT)
 /*B384*/ INST37X (set_fpc,0x84)
 /*B38C*/ INST37X (extract_fpc,0x8c)
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/
// /*B385*/ INST37X (set_fpc_and_signal,0x85)
#if defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)         /*810*/
// /*B390*/ INST37X (convert_u32_to_bfp_short_reg,0x90)           /*810*/
// /*B391*/ INST37X (convert_u32_to_bfp_long_reg,0x91)            /*810*/
// /*B392*/ INST37X (convert_u32_to_bfp_ext_reg,0x92)             /*810*/
// /*B39C*/ INST37X (convert_bfp_short_to_u32_reg,0x9c)           /*810*/
// /*B39D*/ INST37X (convert_bfp_long_to_u32_reg,0x9d)            /*810*/
// /*B39E*/ INST37X (convert_bfp_ext_to_u32_reg,0x9e)             /*810*/
#endif /*defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)*/  /*810*/
#if defined(FEATURE_BINARY_FLOATING_POINT)
/*B394*/ INST37X (convert_fix32_to_bfp_short_reg,0x94)
 /*B395*/ INST37X (convert_fix32_to_bfp_long_reg,0x95)
 /*B396*/ INST37X (convert_fix32_to_bfp_ext_reg,0x96)
 /*B398*/ INST37X (convert_bfp_short_to_fix32_reg,0x98)
 /*B399*/ INST37X (convert_bfp_long_to_fix32_reg,0x99)
 /*B39A*/ INST37X (convert_bfp_ext_to_fix32_reg,0x9a)
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/
 /*B3B4*/ INST37X (convert_fixed_to_float_short_reg,0xb4)
 /*B3B5*/ INST37X (convert_fixed_to_float_long_reg,0xb5)
 /*B3B6*/ INST37X (convert_fixed_to_float_ext_reg,0xb6)
 /*B3B8*/ INST37X (convert_float_short_to_fixed_reg,0xb8)
 /*B3B9*/ INST37X (convert_float_long_to_fixed_reg,0xb9)
 /*B3BA*/ INST37X (convert_float_ext_to_fixed_reg,0xba)
INST37X_TABLE_END(b3)

INST37X_TABLE_START(b9)
// /*B91E*/ INST37X (compute_message_authentication_code,0x1e)
 /*B926*/ INST37X (load_byte_register,0x26)
 /*B927*/ INST37X (load_halfword_register,0x27)
 /*B972*/ INST37X (compare_and_trap_register,0x72)
 /*B973*/ INST37X (compare_logical_and_trap_register,0x73)
 /*B98D*/ INST37X (extract_psw,0x8d)
 /*B990*/ INST37X (translate_two_to_two,0x90)
 /*B991*/ INST37X (translate_two_to_one,0x91)
 /*B992*/ INST37X (translate_one_to_two,0x92)
 /*B993*/ INST37X (translate_one_to_one,0x93)
 /*B994*/ INST37X (load_logical_character_register,0x94)
 /*B995*/ INST37X (load_logical_halfword_register,0x95)
 /*B996*/ INST37X (multiply_logical_register,0x96)
 /*B997*/ INST37X (divide_logical_register,0x97)
 /*B998*/ INST37X (add_logical_carry_register,0x98)
 /*B999*/ INST37X (subtract_logical_borrow_register,0x99)
 /*B9B0*/ INST37X (convert_utf8_to_utf32,0xb0)
 /*B9B1*/ INST37X (convert_utf16_to_utf32,0xb1)
 /*B9B2*/ INST37X (convert_utf32_to_utf8,0xb2)
 /*B9B3*/ INST37X (convert_utf32_to_utf16,0xb3)
 /*B9BD*/ INST37X (translate_and_test_reverse_extended,0xbd)
 /*B9BE*/ INST37X (search_string_unicode,0xbe)
 /*B9BF*/ INST37X (translate_and_test_extended,0xbf)
// /*B9F2*/ INST37X (load_on_condition_register,0xf2)
// /*B9F4*/ INST37X (and_distinct_register,0xf4)
// /*B9F6*/ INST37X (or_distinct_register,0xf6)
// /*B9F7*/ INST37X (exclusive_or_distinct_register,0xf7)
// /*B9F8*/ INST37X (add_distinct_register,0xf8)
// /*B9F9*/ INST37X (subtract_distinct_register,0xf9)
// /*B9FA*/ INST37X (add_logical_distinct_register,0xfa)
// /*B9FB*/ INST37X (subtract_logical_distinct_register,0xfb)
INST37X_TABLE_END(b9)

INST37X_TABLE_START(c0)
 /*C0x0*/ INST37X (load_address_relative_long,0x0)
 /*C0x4*/ INST37X (branch_relative_on_condition_long,0x4)
 /*C0x5*/ INST37X (branch_relative_and_save_long,0x5)
INST37X_TABLE_END(c0)

INST37X_TABLE_START(c2)
 /*C2x1*/ INST37X (multiply_single_immediate_fullword,0x1)
 /*C2x5*/ INST37X (subtract_logical_fullword_immediate,0x5)
 /*C2x9*/ INST37X (add_fullword_immediate,0x9)
 /*C2xB*/ INST37X (add_logical_fullword_immediate,0xb)
 /*C2xD*/ INST37X (compare_fullword_immediate,0xd)
 /*C2xF*/ INST37X (compare_logical_fullword_immediate,0xf)
INST37X_TABLE_END(c2)

INST37X_TABLE_START(c4)
 /*C4x2*/ INST37X (load_logical_halfword_relative_long,0x2)
 /*C4x5*/ INST37X (load_halfword_relative_long,0x5)
 /*C4x7*/ INST37X (store_halfword_relative_long,0x7)
 /*C4xD*/ INST37X (load_relative_long,0xd)
 /*C4xF*/ INST37X (store_relative_long,0xf)
INST37X_TABLE_END(c4)

INST37X_TABLE_START(c6)
// /*C6x0*/ INST37X (execute_relative_long,0x0)
 /*C6x2*/ INST37X (prefetch_data_relative_long,0x2)
 /*C6x5*/ INST37X (compare_halfword_relative_long,0x5)
 /*C6x7*/ INST37X (compare_logical_relative_long_halfword,0x7)
 /*C6xD*/ INST37X (compare_relative_long,0xd)
 /*C6xF*/ INST37X (compare_logical_relative_long,0xf)
INST37X_TABLE_END(c6)

INST37X_TABLE_START(c8)
// /*C8x4*/ INST37X (load_pair_disjoint,0x4)
INST37X_TABLE_END(c8)

INST37X_TABLE_START(e3)
 /*E312*/ INST37X (load_and_test,0x12)
 /*E336*/ INST37X (prefetch_data,0x36)
 /*E394*/ INST37X (load_logical_character,0x94)
 /*E395*/ INST37X (load_logical_halfword,0x95)
 /*E396*/ INST37X (multiply_logical,0x96)
 /*E397*/ INST37X (divide_logical,0x97)
 /*E398*/ INST37X (add_logical_carry,0x98)
 /*E399*/ INST37X (subtract_logical_borrow,0x99)
INST37X_TABLE_END(e3)

INST37X_TABLE_START(e5)
 /*E544*/ INST37X (move_halfword_from_halfword_immediate,0x44)
 /*E548*/ INST37X (move_long_from_halfword_immediate,0x48)
 /*E54C*/ INST37X (move_fullword_from_halfword_immediate,0x4c)
 /*E554*/ INST37X (compare_halfword_immediate_halfword_storage,0x54)
 /*E555*/ INST37X (compare_logical_immediate_halfword_storage,0x55)
 /*E558*/ INST37X (compare_halfword_immediate_long_storage,0x58)
 /*E559*/ INST37X (compare_logical_immediate_long_storage,0x59)
 /*E55C*/ INST37X (compare_halfword_immediate_storage,0x5c)
 /*E55D*/ INST37X (compare_logical_immediate_fullword_storage,0x5d)
INST37X_TABLE_END(e5)

INST37X_TABLE_START(eb)
 /*EB1D*/ INST37X (rotate_left_single_logical,0x1d)
 /*EB6A*/ INST37X (add_immediate_storage,0x6a)
 /*EB6E*/ INST37X (add_logical_with_signed_immediate,0x6e)
 /*EB7A*/ INST37X (add_immediate_long_storage,0x7a)
 /*EB7E*/ INST37X (add_logical_with_signed_immediate_long,0x7e)
 /*EB8E*/ INST37X (move_long_unicode,0x8e)
 /*EB8F*/ INST37X (compare_logical_long_unicode,0x8f)
 /*EBC0*/ INST37X (test_decimal,0xc0)
// /*EBDC*/ INST37X (shift_right_single_distinct,0xdc)
// /*EBDD*/ INST37X (shift_left_single_distinct,0xdd)
// /*EBDE*/ INST37X (shift_right_single_logical_distinct,0xde)
// /*EBDF*/ INST37X (shift_left_single_logical_distinct,0xdf)
// /*EBF2*/ INST37X (load_on_condition,0xf2)
// /*EBF3*/ INST37X (store_on_condition,0xf3)
// /*EBF4*/ INST37X (load_and_and,0xf4)
// /*EBF6*/ INST37X (load_and_or,0xf6)
// /*EBF7*/ INST37X (load_and_exclusive_or,0xf7)
// /*EBF8*/ INST37X (load_and_add,0xf8)
// /*EBFA*/ INST37X (load_and_add_logical,0xfa)
INST37X_TABLE_END(eb)

INST37X_TABLE_START(ec)
 /*EC72*/ INST37X (compare_immediate_and_trap,0x72)
 /*EC73*/ INST37X (compare_logical_immediate_and_trap_fullword,0x73)
 /*EC76*/ INST37X (compare_and_branch_relative_register,0x76)
 /*EC77*/ INST37X (compare_logical_and_branch_relative_register,0x77)
 /*EC7E*/ INST37X (compare_immediate_and_branch_relative,0x7e)
 /*EC7F*/ INST37X (compare_logical_immediate_and_branch_relative,0x7f)
// /*ECD8*/ INST37X (add_distinct_halfword_immediate,0xd8)
// /*ECDA*/ INST37X (add_logical_distinct_signed_halfword_immediate,0xda)
 /*ECF6*/ INST37X (compare_and_branch_register,0xf6)
 /*ECF7*/ INST37X (compare_logical_and_branch_register,0xf7)
 /*ECFE*/ INST37X (compare_immediate_and_branch,0xfe)
 /*ECFF*/ INST37X (compare_logical_immediate_and_branch,0xff)
INST37X_TABLE_END(ec)

INST37X_TABLE_START(ed)
#if defined(FEATURE_BINARY_FLOATING_POINT)
 /*ED04*/ INST37X (load_lengthened_bfp_short_to_long,0x04)
 /*ED05*/ INST37X (load_lengthened_bfp_long_to_ext,0x05)
 /*ED06*/ INST37X (load_lengthened_bfp_short_to_ext,0x06)
 /*ED07*/ INST37X (multiply_bfp_long_to_ext,0x07)
 /*ED08*/ INST37X (compare_and_signal_bfp_short,0x08)
 /*ED09*/ INST37X (compare_bfp_short,0x09)
 /*ED0A*/ INST37X (add_bfp_short,0x0a)
 /*ED0B*/ INST37X (subtract_bfp_short,0x0b)
 /*ED0C*/ INST37X (multiply_bfp_short_to_long,0x0c)
 /*ED0D*/ INST37X (divide_bfp_short,0x0d)
 /*ED0E*/ INST37X (multiply_add_bfp_short,0x0e)
 /*ED0F*/ INST37X (multiply_subtract_bfp_short,0x0f)
 /*ED10*/ INST37X (test_data_class_bfp_short,0x10)
 /*ED11*/ INST37X (test_data_class_bfp_long,0x11)
 /*ED12*/ INST37X (test_data_class_bfp_ext,0x12)
 /*ED14*/ INST37X (squareroot_bfp_short,0x14)
 /*ED15*/ INST37X (squareroot_bfp_long,0x15)
 /*ED17*/ INST37X (multiply_bfp_short,0x17)
 /*ED18*/ INST37X (compare_and_signal_bfp_long,0x18)
 /*ED19*/ INST37X (compare_bfp_long,0x19)
 /*ED1A*/ INST37X (add_bfp_long,0x1a)
 /*ED1B*/ INST37X (subtract_bfp_long,0x1b)
 /*ED1C*/ INST37X (multiply_bfp_long,0x1c)
 /*ED1D*/ INST37X (divide_bfp_long,0x1d)
 /*ED1E*/ INST37X (multiply_add_bfp_long,0x1e)
 /*ED1F*/ INST37X (multiply_subtract_bfp_long,0x1f)
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/
 /*ED24*/ INST37X (load_lengthened_float_short_to_long,0x24)
 /*ED25*/ INST37X (load_lengthened_float_long_to_ext,0x25)
 /*ED26*/ INST37X (load_lengthened_float_short_to_ext,0x26)
 /*ED2E*/ INST37X (multiply_add_float_short,0x2e)
 /*ED2F*/ INST37X (multiply_subtract_float_short,0x2f)
 /*ED34*/ INST37X (squareroot_float_short,0x34)
 /*ED35*/ INST37X (squareroot_float_long,0x35)
 /*ED37*/ INST37X (multiply_float_short,0x37)
 /*ED38*/ INST37X (multiply_add_unnormal_float_long_to_ext_low,0x38)
 /*ED39*/ INST37X (multiply_unnormal_float_long_to_ext_low,0x39)
 /*ED3A*/ INST37X (multiply_add_unnormal_float_long_to_ext,0x3a)
 /*ED3B*/ INST37X (multiply_unnormal_float_long_to_ext,0x3b)
 /*ED3C*/ INST37X (multiply_add_unnormal_float_long_to_ext_high,0x3c)
 /*ED3D*/ INST37X (multiply_unnormal_float_long_to_ext_high,0x3d)
 /*ED3E*/ INST37X (multiply_add_float_long,0x3e)
 /*ED3F*/ INST37X (multiply_subtract_float_long,0x3f)
INST37X_TABLE_END(ed)

/**********************************************************/
/* The table of tables                                    */
/**********************************************************/
INST37X_INSERT_TABLE_START
INST37X_INSERT_TABLE(00)
INST37X_INSERT_TABLE(a7)
INST37X_INSERT_TABLE(b2)
INST37X_INSERT_TABLE(b3)
INST37X_INSERT_TABLE(b9)
INST37X_INSERT_TABLE(c0)
INST37X_INSERT_TABLE(c2)
INST37X_INSERT_TABLE(c4)
INST37X_INSERT_TABLE(c6)
INST37X_INSERT_TABLE(c8)
INST37X_INSERT_TABLE(e3)
INST37X_INSERT_TABLE(e5)
INST37X_INSERT_TABLE(eb)
INST37X_INSERT_TABLE(ec)
INST37X_INSERT_TABLE(ed)
INST37X_INSERT_TABLE_END

/* Replace an actual instruction function.
   code is 0 for the top level table
   set is 1 to set the function, 0 to restore */
static  void    s37x_replace_opcode(struct s37x_inst_table_entry *tb,int code,int set)
{
    int i;
    int code1,code2;
    for(i=0;tb[i].newfun!=NULL;i++)
    {
        if(code==0)
        {
            code1=tb[i].index;
            code2=-1;
        }
        else
        {
            code1=code;
            code2=tb[i].index;
        }
        if(set)
        {
            tb[i].oldfun=replace_opcode(ARCH_370, tb[i].newfun,code1,code2);
//            logmsg("Replacing Opcode %2.2X%2.2X; Old = %p, New=%p\n",(unsigned char)code1,(unsigned char)code2,tb[i].oldfun,tb[i].newfun);
        }
        else
        {
            zz_func old;
            old=replace_opcode(ARCH_370, tb[i].oldfun,code1,code2);
            UNREFERENCED(old);
//            logmsg("Restoring Opcode %2.2X%2.2X; Old = %p, New=%p\n",(unsigned char)code1,(unsigned char)code2,old,tb[i].oldfun);
        }
    }
}

/* scan the 1st level table */
DLL_EXPORT void s37x_replace_opcode_scan(int set)
{
    int i;
    struct  s37x_inst_table_header   *tb;
    for(i=0;s37x_table_table[i];i++)
    {
        tb=s37x_table_table[i];
        s37x_replace_opcode(tb->tbl,tb->ix,set);
    }
}
#endif
#endif // defined(_370)
