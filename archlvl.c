/* ARCHLVL.C    (C) Copyright Jan Jaeger,   2010-2012                */
/*                  Architecture Mode and Facilities                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#if !defined(_ARCHLVL_C_)
#define _ARCHLVL_C_
#endif

#if !defined(_HENGINE_DLL_)
#define _HENGINE_DLL_
#endif

#include "hercules.h"
#include "devtype.h"
#include "opcode.h"
#include "hostinfo.h"

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

    #define NONE     _NONE          /* NO architectures or disabled  */
    #define S370     _S370          /* S/370 architecture            */
    #define E390     _NONE          /* ESA/390 architecture          */
    #define Z900     _NONE          /* z/Arch architecture           */
    #define Z390     (E390|Z900)    /* BOTH ESA/390 and z/Arch       */
    #define MALL     (S370|Z390)    /* All architectures             */

#elif __GEN_ARCH == 390             /*    (building for S/390?)      */

    #define NONE     _NONE          /* NO architectures or disabled  */
    #define S370     _NONE          /* S/370 architecture            */
    #define E390     _E390          /* ESA/390 architecture          */
    #define Z900     _NONE          /* z/Arch architecture           */
    #define Z390     (E390|Z900)    /* BOTH ESA/390 and z/Arch       */
    #define MALL     (S370|Z390)    /* All architectures             */

#elif __GEN_ARCH == 900             /*    (building for z/Arch?)     */

    #define NONE     _NONE          /* NO architectures or disabled  */
    #define S370     _NONE          /* S/370 architecture            */
    #define E390     _NONE          /* ESA/390 architecture          */
    #define Z900     _Z900          /* z/Arch architecture           */
    #define Z390     (E390|Z900)    /* BOTH ESA/390 and z/Arch       */
    #define MALL     (S370|Z390)    /* All architectures             */

#endif

/*-------------------------------------------------------------------*/
/*                  Facility Table structure                         */
/*-------------------------------------------------------------------*/

#ifndef FACTAB_STRUCT_DEFINED
#define FACTAB_STRUCT_DEFINED

typedef bool FACMODCHK( bool enable, int bitno, int archnum,
                        const char* action, const char* actioning,
                        const char* target_facname );
struct FACTAB
{
    FACMODCHK*   modokfunc;         /* Modification Check function   */
    const char*  name;              /* Short facility name           */
    const char*  long_name;         /* Long name = Description       */
          int    bitno;             /* Bit number                    */
          int    supmask;           /* Which archs support it        */
          int    defmask;           /* Default for which archs       */
          int    reqmask;           /* Which archs require it        */
};
typedef struct FACTAB   FACTAB;

#define FT( _sup, _def, _req, _name )                               \
{                                                                   \
    NULL,                                                           \
    QSTR( _name ),                                                  \
    NULL,                                                           \
    STFL_ ## _name,  /* esa390.h 'STFL_XXX' bit number #define */   \
    _sup,                                                           \
    _def,                                                           \
    _req                                                            \
},

#define FT2( _modokfunc, _name, _desc )                             \
{                                                                   \
    _modokfunc,                                                     \
    QSTR( _name ),                                                  \
    _desc,                                                          \
    STFL_ ## _name,  /* esa390.h 'STFL_XXX' bit number #define */   \
    0,                                                              \
    0,                                                              \
    0,                                                              \
},
#endif /* FACTAB_STRUCT_DEFINED */

/*-------------------------------------------------------------------*/
/*              Temporary ARCH_DEP Facility Table                    */
/*-------------------------------------------------------------------*/
/* The facility names used in the below FT macro invocations are,    */
/* when prefixed with 'STFL_', the facility bit number as #defined   */
/* in header esa390.h (e.g. FT( 044_PFPO, ..) ==> STFL_044_PFPO 44)  */
/*                                                                   */
/* Also note that the entries in the below table do NOT have to be   */
/* in any facility bit sequence as it is always seached serially.    */
/* However, it is greatly preferred that it be kept in sequence.     */
/*                                                                   */
/* Sup (Supported) means the facility is supported by Hercules for   */
/* the given architectures.  Def (Default) indicates the facility    */
/* defaults to enabled for the specified architectures.  The Req     */
/* (Required) field indicates the facility is a REQUIRED facility    */
/* for the architecture and enabling/disabling it isn't allowed.     */
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
FT( Z390, Z390, NONE, 000_N3_INSTR )
#endif

/* PROGRAMMING NOTE: please note the use of underscore _FEATURE test */
#if defined(    _FEATURE_001_ZARCH_INSTALLED_FACILITY )
FT( _Z390, _Z390, _Z900, 001_ZARCH_INSTALLED )
#endif

#if defined(  FEATURE_002_ZARCH_ACTIVE_FACILITY )
FT( Z900, Z900, Z900, 002_ZARCH_ACTIVE )
#endif

#if defined(  FEATURE_003_DAT_ENHANCE_FACILITY_1 )
FT( Z390, Z390, NONE, 003_DAT_ENHANCE_1 )
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
FT( Z900, Z900, NONE, 007_STFL_EXTENDED )
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

#if defined(  FEATURE_013_IPTE_RANGE_FACILITY )
FT( Z900, Z900, NONE, 013_IPTE_RANGE )
#endif

#if defined(  FEATURE_014_NONQ_KEY_SET_FACILITY )
FT( Z900, Z900, NONE, 014_NONQ_KEY_SET )
#endif

#if defined(  FEATURE_016_EXT_TRANSL_FACILITY_2 )
FT( Z390, Z390, NONE, 016_EXT_TRANSL_2 )
#endif

#if defined(  FEATURE_017_MSA_FACILITY )
FT( Z390, Z390, NONE, 017_MSA )
#endif

#if defined(  FEATURE_018_LONG_DISPL_INST_FACILITY )
FT( Z390, Z390, NONE, 018_LONG_DISPL_INST )
#endif

#if defined(  FEATURE_019_LONG_DISPL_HPERF_FACILITY )
FT( Z390, Z390, NONE, 019_LONG_DISPL_HPERF )
#endif

#if defined(  FEATURE_020_HFP_MULT_ADD_SUB_FACILITY )
FT( Z390, Z390, NONE, 020_HFP_MULT_ADD_SUB )
#endif

#if defined(  FEATURE_021_EXTENDED_IMMED_FACILITY )
FT( Z900, Z900, NONE, 021_EXTENDED_IMMED )
#endif

#if defined(  FEATURE_022_EXT_TRANSL_FACILITY_3 )
FT( Z900, Z900, NONE, 022_EXT_TRANSL_3 )
#endif

#if defined(  FEATURE_023_HFP_UNNORM_EXT_FACILITY )
FT( Z900, Z900, NONE, 023_HFP_UNNORM_EXT )
#endif

#if defined(  FEATURE_024_ETF2_ENHANCEMENT_FACILITY )
FT( Z900, Z900, NONE, 024_ETF2_ENHANCEMENT )
#endif

#if defined(  FEATURE_025_STORE_CLOCK_FAST_FACILITY )
FT( Z900, Z900, NONE, 025_STORE_CLOCK_FAST )
#endif

#if defined(  FEATURE_026_PARSING_ENHANCE_FACILITY )
FT( Z900, Z900, NONE, 026_PARSING_ENHANCE )
#endif

#if defined(  FEATURE_027_MVCOS_FACILITY )
FT( Z900, Z900, NONE, 027_MVCOS )
#endif

#if defined(  FEATURE_028_TOD_CLOCK_STEER_FACILITY )
FT( Z900, Z900, NONE, 028_TOD_CLOCK_STEER )
#endif

#if defined(  FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
FT( Z900, Z900, NONE, 030_ETF3_ENHANCEMENT )
#endif

#if defined(  FEATURE_031_EXTRACT_CPU_TIME_FACILITY )
FT( Z900, Z900, NONE, 031_EXTRACT_CPU_TIME )
#endif

#if defined(  FEATURE_032_CSS_FACILITY )
FT( Z900, Z900, NONE, 032_CSSF )
#endif

#if defined(  FEATURE_033_CSS_FACILITY_2 )
FT( Z900, Z900, NONE, 033_CSSF2 )
#endif

#if defined(  FEATURE_034_GEN_INST_EXTN_FACILITY )
FT( Z900, Z900, NONE, 034_GEN_INST_EXTN )
#endif

#if defined(  FEATURE_035_EXECUTE_EXTN_FACILITY )
FT( Z900, Z900, NONE, 035_EXECUTE_EXTN )
#endif

#if defined(  FEATURE_036_ENH_MONITOR_FACILITY )
FT( Z900, Z900, NONE, 036_ENH_MONITOR )
#endif

#if defined(  FEATURE_037_FP_EXTENSION_FACILITY )
FT( Z900, Z900, NONE, 037_FP_EXTENSION )
#endif

#if defined(  FEATURE_038_OP_CMPSC_FACILITY )
FT( NONE, NONE, NONE, 038_OP_CMPSC )
#endif

#if defined(  FEATURE_040_LOAD_PROG_PARAM_FACILITY )
FT( Z900, Z900, NONE, 040_LOAD_PROG_PARAM )
#endif

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_041_FPS_ENHANCEMENT_FACILITY )

FT( Z900, Z900, NONE, 041_FPS_ENHANCEMENT )

#if defined(  FEATURE_041_DFP_ROUNDING_FACILITY )
FT( Z900, Z900, NONE, 041_DFP_ROUNDING )
#endif

#if defined(  FEATURE_041_FPR_GR_TRANSFER_FACILITY )
FT( Z900, Z900, NONE, 041_FPR_GR_TRANSFER )
#endif

#if defined(  FEATURE_041_FPS_SIGN_HANDLING_FACILITY )
FT( Z900, Z900, NONE, 041_FPS_SIGN_HANDLING )
#endif

#if defined(  FEATURE_041_IEEE_EXCEPT_SIM_FACILITY )
FT( Z900, Z900, NONE, 041_IEEE_EXCEPT_SIM )
#endif

#endif /* defined(  FEATURE_041_FPS_ENHANCEMENT_FACILITY ) */

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_042_DFP_FACILITY )
FT( Z900, Z900, NONE, 042_DFP )
#endif

#if defined(  FEATURE_043_DFP_HPERF_FACILITY )
FT( Z900, Z900, NONE, 043_DFP_HPERF )
#endif

/*-------------------------------------------------------------------*/
/* Since some modern operating systems (e.g. z/OS and z/VM) require  */
/* it, allow the PFPO facility to be forcibly enabled Until we can   */
/* eventually code proper support for it.                            */
/*-------------------------------------------------------------------*/
// FIXME: code proper support for PFPO!
//#if defined(  FEATURE_044_PFPO_FACILITY )
FT( Z900, NONE, NONE, 044_PFPO )
//#endif

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_045_DISTINCT_OPERANDS_FACILITY )
FT( Z900, Z900, NONE, 045_DISTINCT_OPERANDS )
#endif

#if defined(  FEATURE_045_FAST_BCR_SERIAL_FACILITY )
FT( Z900, Z900, NONE, 045_FAST_BCR_SERIAL )
#endif

#if defined(  FEATURE_045_HIGH_WORD_FACILITY )
FT( Z900, Z900, NONE, 045_HIGH_WORD )
#endif

#if defined(  FEATURE_045_INTERLOCKED_ACCESS_FACILITY_1 )
FT( Z900, Z900, NONE, 045_INTERLOCKED_ACCESS_1 )
#endif

#if defined(  FEATURE_045_LOAD_STORE_ON_COND_FACILITY_1 )
FT( Z900, Z900, NONE, 045_LOAD_STORE_ON_COND_1 )
#endif

#if defined(  FEATURE_045_POPULATION_COUNT_FACILITY )
FT( Z900, Z900, NONE, 045_POPULATION_COUNT )
#endif

/*-------------------------------------------------------------------*/

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
FT( NONE, NONE, NONE, 050_CONSTR_TRANSACT )
#endif

#if defined(  FEATURE_051_LOCAL_TLB_CLEARING_FACILITY )
FT( NONE, NONE, NONE, 051_LOCAL_TLB_CLEARING )
#endif

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_052_INTERLOCKED_ACCESS_FACILITY_2 )
#if IAF2_ATOMICS_UNAVAILABLE == CAN_IAF2
FT( NONE, NONE, NONE, 052_INTERLOCKED_ACCESS_2 )
#else
FT( Z900, Z900, NONE, 052_INTERLOCKED_ACCESS_2 )
#endif
#endif /* defined(  FEATURE_052_INTERLOCKED_ACCESS_FACILITY_2 ) */

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_053_LOAD_STORE_ON_COND_FACILITY_2 )
FT( NONE, NONE, NONE, 053_LOAD_STORE_ON_COND_2 )
#endif

#if defined(  FEATURE_053_LOAD_ZERO_RIGHTMOST_FACILITY )
FT( NONE, NONE, NONE, 053_LOAD_ZERO_RIGHTMOST )
#endif

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_054_EE_CMPSC_FACILITY )
FT( NONE, NONE, NONE, 054_EE_CMPSC )
#endif

#if defined(  FEATURE_057_MSA_EXTENSION_FACILITY_5 )
FT( NONE, NONE, NONE, 057_MSA_EXTENSION_5 )
#endif

#if defined(  FEATURE_058_MISC_INSTR_EXT_FACILITY_2 )
FT( NONE, NONE, NONE, 058_MISC_INSTR_EXT_2 )
#endif

#if defined(  FEATURE_066_RES_REF_BITS_MULT_FACILITY )
FT( Z900, Z900, NONE, 066_RES_REF_BITS_MULT )
#endif

#if defined(  FEATURE_067_CPU_MEAS_COUNTER_FACILITY )
FT( Z900, Z900, NONE, 067_CPU_MEAS_COUNTER )
#endif

#if defined(  FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY )
FT( Z900, Z900, NONE, 068_CPU_MEAS_SAMPLNG )
#endif

#if defined(  FEATURE_073_TRANSACT_EXEC_FACILITY )
FT( NONE, NONE, NONE, 073_TRANSACT_EXEC )
#endif

#if defined(  FEATURE_074_STORE_HYPER_INFO_FACILITY )
FT( NONE, NONE, NONE, 074_STORE_HYPER_INFO )
#endif

#if defined(  FEATURE_075_ACC_EX_FS_INDIC_FACILITY )
FT( Z900, Z900, NONE, 075_ACC_EX_FS_INDIC )
#endif

#if defined(  FEATURE_076_MSA_EXTENSION_FACILITY_3 )
FT( Z900, Z900, NONE, 076_MSA_EXTENSION_3 )
#endif

#if defined(  FEATURE_077_MSA_EXTENSION_FACILITY_4 )
FT( Z900, Z900, NONE, 077_MSA_EXTENSION_4 )
#endif

#if defined(  FEATURE_078_ENHANCED_DAT_FACILITY_2 )
FT( NONE, NONE, NONE, 078_EDAT_2 )
#endif

#if defined(  FEATURE_080_DFP_PACK_CONV_FACILITY )
FT( NONE, NONE, NONE, 080_DFP_PACK_CONV )
#endif

#if defined(  FEATURE_129_ZVECTOR_FACILITY )
FT( NONE, NONE, NONE, 129_ZVECTOR )
#endif

#if defined(  FEATURE_130_INSTR_EXEC_PROT_FACILITY )
FT( NONE, NONE, NONE, 130_INSTR_EXEC_PROT )
#endif

#if defined(  FEATURE_131_SIDE_EFFECT_ACCESS_FACILITY )
FT( NONE, NONE, NONE, 131_SIDE_EFFECT_ACCESS )
#endif

#if defined(  FEATURE_133_GUARDED_STORAGE_FACILITY )
FT( NONE, NONE, NONE, 133_GUARDED_STORAGE )
#endif

#if defined(  FEATURE_134_ZVECTOR_PACK_DEC_FACILITY )
FT( NONE, NONE, NONE, 134_ZVECTOR_PACK_DEC )
#endif

#if defined(  FEATURE_135_ZVECTOR_ENH_FACILITY_1 )
FT( NONE, NONE, NONE, 135_ZVECTOR_ENH_1 )
#endif

#if defined(  FEATURE_138_CONFIG_ZARCH_MODE_FACILITY )
FT( NONE, NONE, NONE, 138_CONFIG_ZARCH_MODE )
#endif

#if defined(  FEATURE_139_MULTIPLE_EPOCH_FACILITY )
FT( NONE, NONE, NONE, 139_MULTIPLE_EPOCH )
#endif

#if defined(  FEATURE_142_ST_CPU_COUNTER_MULT_FACILITY )
FT( NONE, NONE, NONE, 142_ST_CPU_COUNTER_MULT )
#endif

#if defined(  FEATURE_144_TEST_PEND_EXTERNAL_FACILITY )
FT( NONE, NONE, NONE, 144_TEST_PEND_EXTERNAL )
#endif

#if defined(  FEATURE_145_INS_REF_BITS_MULT_FACILITY )
FT( NONE, NONE, NONE, 145_INS_REF_BITS_MULT )
#endif

#if defined(  FEATURE_146_MSA_EXTENSION_FACILITY_8 )
FT( NONE, NONE, NONE, 146_MSA_EXTENSION_8 )
#endif

#if defined(  FEATURE_168_ESA390_COMPAT_MODE_FACILITY )
FT( NONE, NONE, NONE, 168_ESA390_COMPAT_MODE )
#endif

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
/* to Hercules's facility bits. Only the archlvl command functions   */
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

FT( MALL, MALL, Z900, HERC_MOVE_INVERSE )

/*-------------------------------------------------------------------*/

#if defined(       FEATURE_MSA_EXTENSION_FACILITY_1 )
FT( Z390, Z390, NONE, HERC_MSA_EXTENSION_1 )
#endif

#if defined(      FEATURE_MSA_EXTENSION_FACILITY_2 )
FT( Z390, Z390, NONE, HERC_MSA_EXTENSION_2 )
#endif

/*-------------------------------------------------------------------*/

#if defined(  FEATURE_HERCULES_DIAGCALLS )

FT( MALL, NONE, NONE, HERC_PROBSTATE_DIAGF08 )
FT( MALL, NONE, NONE, HERC_SIGP_SETARCH_S370 )

#if defined(       FEATURE_HOST_RESOURCE_ACCESS_FACILITY )

FT( MALL, NONE, NONE, HERC_HOST_RESOURCE_ACCESS )

#endif
#endif

/*-------------------------------------------------------------------*/

#if defined(       FEATURE_QEBSM )
FT( Z390, Z390, NONE, HERC_QEBSM )
#endif

#if defined(       FEATURE_QDIO_THININT )
FT( Z390, Z390, NONE, HERC_QDIO_THININT )
#endif

#if defined(       FEATURE_QDIO_TDD )
FT( Z390, NONE, NONE, HERC_QDIO_TDD )
#endif

#if defined(       FEATURE_SVS )
FT( Z390, Z390, NONE, HERC_SVS )
#endif

#if defined(          FEATURE_HYPERVISOR )
FT( MALL, MALL, NONE, HERC_LOGICAL_PARTITION )
#endif

#if defined(         FEATURE_EMULATE_VM )
FT( MALL, NONE, NONE, HERC_VIRTUAL_MACHINE )
#endif

FT( Z390, NONE, NONE, HERC_QDIO_ASSIST )

#if defined(       FEATURE_INTERVAL_TIMER )
FT( MALL, MALL, Z390, HERC_INTERVAL_TIMER )
#endif

FT( MALL, MALL, NONE, HERC_DETECT_PGMINTLOOP )

};

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined(   _GEN_ARCH )

  // _ARCH_NUM_0 has been built.
  // Now build the other architectures...

  #if defined(                _ARCH_NUM_1 )
    #define     _GEN_ARCH     _ARCH_NUM_1
    #include    "archlvl.c"
  #endif

  #if defined(                _ARCH_NUM_2 )
    #undef      _GEN_ARCH
    #define     _GEN_ARCH     _ARCH_NUM_2
    #include    "archlvl.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*                ACTUAL Architecture bit-masks                      */
/*-------------------------------------------------------------------*/

#undef  NONE
#undef  S370
#undef  E390
#undef  Z900
#undef  Z390
#undef  MALL

#define NONE     0x00               /* NO architectures or disabled  */
#define S370     0x80               /* S/370 architecture            */
#define E390     0x40               /* ESA/390 architecture          */
#define Z900     0x20               /* z/Arch architecture           */

#define Z390     (E390|Z900)        /* BOTH ESA/390 and z/Arch       */
#define MALL     (S370|Z390)        /* All architectures             */

/*-------------------------------------------------------------------*/
/*                  Architecture Level Table                         */
/*-------------------------------------------------------------------*/

struct ARCHTAB
{
    const char*  name;              /* Architecture name             */
    const int    num;               /* Architecture number           */
    const int    amask;             /* Architecture bit-mask         */
};
typedef struct ARCHTAB  ARCHTAB;

#define AT( _name, _num, _amask )                                   \
{                                                                   \
    (_name),                                                        \
    (_num),                                                         \
    (_amask)                                                        \
},

static ARCHTAB archtab[] =
{

    //---------------------------------------------------------
    //
    //  The "_ARCH_nnn_NAME" entries MUST come first. The
    //  'get_archtab_by_arch' function expects it.
    //
    //  The FIRST entry is our DEFAULT initial startup archi-
    //  tecture set by the 'init_default_archmode' function.
    //
    //---------------------------------------------------------

#if defined( _900 )
    AT( _ARCH_900_NAME,  ARCH_900_IDX,  Z900 )  //  "z/Arch"  feat900.h
#endif
#if defined( _390 )
    AT( _ARCH_390_NAME,  ARCH_390_IDX,  E390 )  //  "ESA/390" feat390.h
#endif
#if defined( _370 )
    AT( _ARCH_370_NAME,  ARCH_370_IDX,  S370 )  //  "S/370"   feat370.h
#endif

    // Other supported names come afterwards, in any order.

#if defined( _370 )
    AT( "S370",          ARCH_370_IDX,  S370 )
    AT( "370",           ARCH_370_IDX,  S370 )
#endif
#if defined( _390 )
    AT( "ESA390",        ARCH_390_IDX,  E390 )
    AT( "E390",          ARCH_390_IDX,  E390 )
    AT( "E/390",         ARCH_390_IDX,  E390 )
    AT( "ESA",           ARCH_390_IDX,  E390 )
    AT( "S/390",         ARCH_390_IDX,  E390 )
    AT( "OS/390",        ARCH_390_IDX,  E390 )
    AT( "S390",          ARCH_390_IDX,  E390 )
    AT( "OS390",         ARCH_390_IDX,  E390 )
    AT( "390",           ARCH_390_IDX,  E390 )
#endif
#if defined( _900 )
    AT( "zArch",         ARCH_900_IDX,  Z900 )
    AT( "z",             ARCH_900_IDX,  Z900 )
    AT( "900",           ARCH_900_IDX,  Z900 )
    AT( "Z900",          ARCH_900_IDX,  Z900 )
    AT( "z/900",         ARCH_900_IDX,  Z900 )
    AT( "ESAME",         ARCH_900_IDX,  Z900 )
    AT( "ESA/ME",        ARCH_900_IDX,  Z900 )
#endif
};

/*-------------------------------------------------------------------*/
/*   Facility Bit Modification Check Functions forward references    */
/*-------------------------------------------------------------------*/

static  bool  mod0or7   ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* target_facname );
static  bool  modidte   ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* target_facname );
static  bool  modlong   ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* target_facname );
static  bool  modtrans  ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* target_facname );
static  bool  modvec    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* target_facname );
static  bool  modtod    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* target_facname );
static  bool  modcpu    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* target_facname );
static  bool  modmsa    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* target_facname );
static  bool  modbit42  ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* target_facname );
static  bool  moddfphi  ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* target_facname );
static  bool  modfpx    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* target_facname );
static  bool  moddfp    ( bool enable, int bitno, int archnum, const char* action, const char* actioning, const char* target_facname );

/*-------------------------------------------------------------------*/
/* The ACTUAL facilities table, initialized by init_facilities_lists */
/*-------------------------------------------------------------------*/
/*  The individual ARCH_DEP( facs_tab ) tables are merged into this  */
/*  table to yield the actual facilities table the system will use.  */
/*  Refer to init_facilities_lists() function for how this is done.  */
/*-------------------------------------------------------------------*/

static FACTAB factab[] =
{
/*-------------------------------------------------------------------*/
/*  (function)  Short Name...                Long Description...     */
/*-------------------------------------------------------------------*/
FT2( mod0or7,   000_N3_INSTR,               "N3 Instructions are installed" )
FT2( NULL,      001_ZARCH_INSTALLED,        "z/Architecture architectural mode is installed" )
FT2( NULL,      002_ZARCH_ACTIVE,           "z/Architecture architectural mode is active" )
FT2( modidte,   003_DAT_ENHANCE_1,          "DAT-Enhancement Facility 1" )
FT2( modidte,   004_IDTE_SC_SEGTAB,         "IDTE selective clearing when segment-table invalidated" )
FT2( modidte,   005_IDTE_SC_REGTAB,         "IDTE selective clearing when region-table invalidated" )
FT2( NULL,      006_ASN_LX_REUSE,           "ASN-and-LX-Reuse Facility" )
FT2( mod0or7,   007_STFL_EXTENDED,          "Store-Facility-List-Extended Facility" )
FT2( NULL,      008_EDAT_1,                 "Enhanced-DAT Facility 1" )
FT2( NULL,      009_SENSE_RUN_STATUS,       "Sense-Running-Status Facility" )
FT2( NULL,      010_CONDITIONAL_SSKE,       "Conditional-SSKE Facility" )
FT2( NULL,      011_CONFIG_TOPOLOGY,        "Configuration-Topology Facility" )
FT2( NULL,      013_IPTE_RANGE,             "IPTE-Range Facility" )
FT2( NULL,      014_NONQ_KEY_SET,           "Nonquiescing Key-Setting Facility" )
FT2( NULL,      016_EXT_TRANSL_2,           "Extended-Translation Facility 2" )
FT2( NULL,      017_MSA,                    "Message-Security Assist" )
FT2( modlong,   018_LONG_DISPL_INST,        "Long-Displacement Facility" )
FT2( modlong,   019_LONG_DISPL_HPERF,       "Long-Displacement Facility Has High Performance" )
FT2( NULL,      020_HFP_MULT_ADD_SUB,       "HFP-Multiply-and-Add/Subtract Facility" )
FT2( NULL,      021_EXTENDED_IMMED,         "Extended-Immediate Facility" )
FT2( NULL,      022_EXT_TRANSL_3,           "Extended-Translation Facility 3" )
FT2( NULL,      023_HFP_UNNORM_EXT,         "HFP-Unnormalized-Extensions Facility" )
FT2( NULL,      024_ETF2_ENHANCEMENT,       "ETF2-Enhancement Facility" )
FT2( modtod,    025_STORE_CLOCK_FAST,       "Store-Clock-Fast Facility" )
FT2( NULL,      026_PARSING_ENHANCE,        "Parsing-Enhancement Facility" )
FT2( NULL,      027_MVCOS,                  "Move-with-Optional-Specifications Facility" )
FT2( modtod,    028_TOD_CLOCK_STEER,        "TOD-Clock-Steering Facility" )
FT2( NULL,      030_ETF3_ENHANCEMENT,       "ETF3-Enhancement Facility" )
FT2( NULL,      031_EXTRACT_CPU_TIME,       "Extract-CPU-Time Facility" )
FT2( NULL,      032_CSSF,                   "Compare-and-Swap-and-Store Facility" )
FT2( NULL,      033_CSSF2,                  "Compare-and-Swap-and-Store Facility 2" )
FT2( NULL,      034_GEN_INST_EXTN,          "General-Instructions-Extension Facility" )
FT2( NULL,      035_EXECUTE_EXTN,           "Execute-Extensions Facility" )
FT2( NULL,      036_ENH_MONITOR,            "Enhanced-Monitor Facility" )
FT2( modfpx,    037_FP_EXTENSION,           "Floating-Point-Extension Facility" )
FT2( NULL,      038_OP_CMPSC,               "Order-Preserving-Compression Facility" )
FT2( NULL,      040_LOAD_PROG_PARAM,        "Load-Program-Parameter Facility" )
FT2( NULL,      041_FPS_ENHANCEMENT,        "Floating-Point-Support-Enhancement Facility" )
FT2( NULL,      041_DFP_ROUNDING,           "Decimal-Floating-Point-Rounding Facility" )
FT2( NULL,      041_FPR_GR_TRANSFER,        "FPR-GR-Transfer Facility" )
FT2( NULL,      041_FPS_SIGN_HANDLING,      "Floating-Point-Support-Sign-Handling Facility" )
FT2( NULL,      041_IEEE_EXCEPT_SIM,        "IEEE-Exception-Simulation Facility" )
FT2( modbit42,  042_DFP,                    "Decimal-Floating-Point Facility" )
FT2( moddfphi,  043_DFP_HPERF,              "Decimal-Floating-Point Facility Has High Performance" )
FT2( NULL,      044_PFPO,                   "PFPO (Perform Floating-Point Operation) Facility" )
FT2( NULL,      045_DISTINCT_OPERANDS,      "Distinct-Operands Facility" )
FT2( NULL,      045_FAST_BCR_SERIAL,        "Fast-BCR-Serialization Facility" )
FT2( NULL,      045_HIGH_WORD,              "High-Word Facility" )
FT2( NULL,      045_INTERLOCKED_ACCESS_1,   "Interlocked-Access Facility 1" )
FT2( NULL,      045_LOAD_STORE_ON_COND_1,   "Load/Store-on-Condition Facility 1" )
FT2( NULL,      045_POPULATION_COUNT,       "Population-Count Facility" )
FT2( NULL,      047_CMPSC_ENH,              "CMPSC-Enhancement Facility" )
FT2( moddfp,    048_DFP_ZONE_CONV,          "Decimal-Floating-Point-Zoned-Conversion Facility" )
FT2( modtrans,  049_EXECUTION_HINT,         "Execution-Hint Facility" )
FT2( modtrans,  049_LOAD_AND_TRAP,          "Load-and-Trap Facility" )
FT2( modtrans,  049_PROCESSOR_ASSIST,       "Processor-Assist Facility" )
FT2( modtrans,  049_MISC_INSTR_EXT_1,       "Miscellaneous-Instruction-Extensions Facility 1" )
FT2( modtrans,  050_CONSTR_TRANSACT,        "Constrained-Transactional-Execution Facility" )
FT2( NULL,      051_LOCAL_TLB_CLEARING,     "Local-TLB-Clearing Facility" )
FT2( NULL,      052_INTERLOCKED_ACCESS_2,   "Interlocked-Access Facility 2" )
FT2( NULL,      053_LOAD_STORE_ON_COND_2,   "Load/Store-on-Condition Facility 2" )
FT2( NULL,      053_LOAD_ZERO_RIGHTMOST,    "Load-and-Zero-Rightmost-Byte Facility" )
FT2( NULL,      054_EE_CMPSC,               "Entropy-Encoding-Compression Facility" )
FT2( NULL,      057_MSA_EXTENSION_5,        "Message-Security-Assist Extension 5" )
FT2( NULL,      058_MISC_INSTR_EXT_2,       "Miscellaneous-Instruction-Extensions Facility 2" )
FT2( NULL,      066_RES_REF_BITS_MULT,      "Reset-Reference-Bits-Multiple Facility" )
FT2( modcpu,    067_CPU_MEAS_COUNTER,       "CPU-Measurement Counter Facility" )
FT2( NULL,      068_CPU_MEAS_SAMPLNG,       "CPU-Measurement Sampling Facility" )
FT2( modtrans,  073_TRANSACT_EXEC,          "Transactional-Execution Facility" )
FT2( NULL,      074_STORE_HYPER_INFO,       "Store-Hypervisor-Information Facility" )
FT2( NULL,      075_ACC_EX_FS_INDIC,        "Access-Exception-Fetch/Store-Indication Facility" )
FT2( modmsa,    076_MSA_EXTENSION_3,        "Message-Security-Assist Extension 3" )
FT2( NULL,      077_MSA_EXTENSION_4,        "Message-Security-Assist Extension 4" )
FT2( NULL,      078_EDAT_2,                 "Enhanced-DAT Facility 2" )
FT2( moddfp,    080_DFP_PACK_CONV,          "Decimal-Floating-Point-Packed-Conversion Facility" )
FT2( modvec,    129_ZVECTOR,                "Vector Facility for z/Architecture" )
FT2( NULL,      130_INSTR_EXEC_PROT,        "Instruction-Execution-Protection Facility" )
FT2( NULL,      131_SIDE_EFFECT_ACCESS,     "Side-Effect-Access Facility" )
FT2( NULL,      133_GUARDED_STORAGE,        "Guarded-Storage Facility" )
FT2( modvec,    134_ZVECTOR_PACK_DEC,       "Vector Packed-Decimal Facility" )
FT2( modvec,    135_ZVECTOR_ENH_1,          "Vector-Enhancements Facility 1" )
FT2( NULL,      138_CONFIG_ZARCH_MODE,      "CZAM Facility (Configuration-z/Architecture-Architectural-Mode)" )
FT2( modtod,    139_MULTIPLE_EPOCH,         "Multiple-Epoch Facility" )
FT2( modcpu,    142_ST_CPU_COUNTER_MULT,    "Store-CPU-Counter-Multiple Facility" )
FT2( NULL,      144_TEST_PEND_EXTERNAL,     "Test-Pending-External-Interruption Facility" )
FT2( NULL,      145_INS_REF_BITS_MULT,      "Insert-Reference-Bits-Multiple Facility" )
FT2( modmsa,    146_MSA_EXTENSION_8,        "Message-Security-Assist Extension 8" )
FT2( NULL,      168_ESA390_COMPAT_MODE,     "ESA/390-Compatability-Mode Facility" )

FT2( NULL,      HERC_MOVE_INVERSE,          "Hercules MVCIN Move Inverse Instruction Support" )
FT2( NULL,      HERC_MSA_EXTENSION_1,       "Hercules Message-Security-Assist Extension 1 Support" )
FT2( NULL,      HERC_MSA_EXTENSION_2,       "Hercules Message-Security-Assist Extension 2 Support" )
FT2( NULL,      HERC_PROBSTATE_DIAGF08,     "Hercules Problem-State Diagnose X'F08' Support" )
FT2( NULL,      HERC_SIGP_SETARCH_S370,     "Hercules SIGP Set Architecture S/370 Support" )
FT2( NULL,      HERC_HOST_RESOURCE_ACCESS,  "Hercules Host Resource Access Support" )
FT2( NULL,      HERC_QEBSM,                 "Hercules QDIO Enhanced Buffer-State Management Support" )
FT2( NULL,      HERC_QDIO_THININT,          "Hercules QDIO Thin-Interrupts Support" )
FT2( NULL,      HERC_QDIO_TDD,              "Hercules QDIO Time-Delayed-Dispatching Support" )
FT2( NULL,      HERC_SVS,                   "Hercules SVS Set Vector Summary Instruction Support" )
FT2( NULL,      HERC_LOGICAL_PARTITION,     "Hercules Logical Partition (LPAR) Support" )
FT2( NULL,      HERC_VIRTUAL_MACHINE,       "Hercules Emulate Virtual Machine Support" )
FT2( NULL,      HERC_QDIO_ASSIST,           "Hercules QDIO-Assist Support" )
FT2( NULL,      HERC_INTERVAL_TIMER,        "Hercules Interval Timer Support" )
FT2( NULL,      HERC_DETECT_PGMINTLOOP,     "Hercules Detect-Program-Interrupt-Loop Support" )
};

/*-------------------------------------------------------------------*/
/*                   init_default_archmode                  (public) */
/*-------------------------------------------------------------------*/
void init_default_archmode()
{
    // Called ONCE by bldcfg.c 'build_config' during system startup.

    sysblk.arch_mode = archtab[0].num;  // (always the first entry)
}

/*-------------------------------------------------------------------*/
/*                    get_archtab_by_arch                            */
/*-------------------------------------------------------------------*/
static const ARCHTAB* get_archtab_by_arch( int archnum )
{
    // An "Architecture mode" (archmode, arch_mode) is the same
    // thing as an "Architecture number" (archnum, arch_num),
    // which is simply an index value determined at build time
    // depending on which architectures that Hercules was built
    // with support for (ARCH_nnn_IDX #defined by featchk.h) and
    // is used to index into architecture dependent tables such as
    // the sysblk.facility_list and the opcode tables in opcode.c

    size_t i;
    for (i=0; i < _countof( archtab ); i++)
        if (archtab[i].num == archnum)
            return &archtab[i];
    return NULL;
}

/*-------------------------------------------------------------------*/
/*                    get_archtab_by_name                            */
/*-------------------------------------------------------------------*/
static const ARCHTAB* get_archtab_by_name( const char* name )
{
    size_t i;
    for (i=0; i < _countof( archtab ); i++)
        if (strcasecmp( archtab[i].name, name ) == 0)
            return &archtab[i];
    return NULL;
}

/*-------------------------------------------------------------------*/
/*                   get_arch_name_by_arch                           */
/*-------------------------------------------------------------------*/
static const char* get_arch_name_by_arch( int archnum )
{
    const ARCHTAB* at = get_archtab_by_arch( archnum );
    return at ? at->name : NULL;
}

/*-------------------------------------------------------------------*/
/*                   get_arch_mask_by_arch                           */
/*-------------------------------------------------------------------*/
static int get_arch_mask_by_arch( int archnum )
{
    const ARCHTAB* at = get_archtab_by_arch( archnum );
    return at ? at->amask : 0;
}

/*-------------------------------------------------------------------*/
/*                      get_arch_name                       (public) */
/*-------------------------------------------------------------------*/
const char* get_arch_name( REGS* regs )
{
    int archnum = regs ? regs->arch_mode : sysblk.arch_mode;
    return get_arch_name_by_arch( archnum );
}

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

/*-------------------------------------------------------------------*/
/*                   init_facilities_lists                  (public) */
/*-------------------------------------------------------------------*/
static const FACTAB* get_factab_by_bitno( int bitno );  /* (fwd ref) */

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
            if (1
                && (ft = get_factab_by_bitno( bitno ))
                && (ft->modokfunc)
            )
            {
                fbyte =         (bitno / 8);
                fbit  = 0x80 >> (bitno % 8);

                enable = (sysblk.facility_list[ arch ][ fbyte ] & fbit) ?
                    true : false;

                if (!ft->modokfunc( enable, bitno, arch, NULL, NULL, NULL ))
                {
                    // "%s facility %s fails consistency check"
                    WRMSG( HHC00899, "S", archname, ft->name );
                    rc = false;
                }
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
/*                     set_archmode_by_name                          */
/*-------------------------------------------------------------------*/
static bool set_archmode_by_name( const char* archname )
{
    const ARCHTAB* at;
    if (!(at = get_archtab_by_name( archname )))
        return false;
    sysblk.arch_mode = at->num;
    return true;
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
    const FACTAB* f1 = *( (const FACTAB**) p1 );
    const FACTAB* f2 = *( (const FACTAB**) p2 );
    int rc = (f1->bitno - f2->bitno);
    return rc ? rc : strcasecmp( f1->name, f2->name );
}
static int sort_ftpp_by_long_name( const void* p1, const void* p2 )
{
    const FACTAB* f1 = *( (const FACTAB**) p1 );
    const FACTAB* f2 = *( (const FACTAB**) p2 );
    return strcasecmp( f1->long_name, f2->long_name );
}

/*-------------------------------------------------------------------*/
/*                     archlvl_query_all                             */
/*-------------------------------------------------------------------*/
static bool archlvl_query_all( const ARCHTAB* at, bool sort_by_long )
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
    LOGMSG( "HHC00891I           SDRC* = Supported, Default, Required, Current, Modified.\n" );
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

        sup = (ft->supmask & at->amask)     ? 'Y' : '-';
        req = (ft->reqmask & at->amask)     ? 'Y' : '-';
        def = (ft->defmask & at->amask)     ? '1' : '0';
        cur = (enabled)                     ? '1' : '0';
        mod = (def == '1' && cur == '0') ||
              (def == '0' && cur == '1')    ? '*' : ' ';
        sev = (mod == '*')                  ? "W" : "I";

        // "%3d %02X %02X %c%c%c%c%c %-27s %s"
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
            ft->long_name
        );
    }

    LOGMSG( "HHC00891I\n" );
    LOGMSG( "HHC00891I           SDRC* = Supported, Default, Required, Current, Modified.\n" );
    LOGMSG( "HHC00891I\n" );

    free( ptr_array );
    return true;
}

/*-------------------------------------------------------------------*/
/*                     archlvl_query_raw                             */
/*-------------------------------------------------------------------*/
static void archlvl_query_raw( const ARCHTAB* at )
{
    char buf[ 128 ] = {0};
    char wrk[  20 ];
    int  i;

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
/*                        archlvl_query                    (boolean) */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*   ARCHLVL QUERY [ ALL | SHORT | LONG | <facility> | bit | RAW ]   */
/*                                                                   */
/*-------------------------------------------------------------------*/
static bool archlvl_query( int argc, char* argv[] )
{
    const ARCHTAB*  at;
    const FACTAB*   ft;
    const char*     p;
    const char*     sev;
    int             bitno, fbyte, fbit;
    bool            forced, enabled, modified;
    char            c;

    /* Note: we know argc >= 2, otherwise why would we be called? */

    if (argc > 3)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return false;
    }

    /* Get pointer to ARCHTAB entry for current architecture */
    at = get_archtab_by_arch( sysblk.arch_mode );

    /* Query ALL? */

    if (argc == 2 ||                               // (implicit ALL)
       (argc == 3 && (0
                      || CMD( argv[2], ALL,   1 )  // (explicit ALL)
                      || CMD( argv[2], SHORT, 1 )  // (default sort)
                      || CMD( argv[2], LONG,  1 )  // (by long name)
                     )
    ))
    {
        const bool sort_by_long = argc < 3 ? false : CMD( argv[2], LONG, 1 );
        return archlvl_query_all( at, sort_by_long );
    }

    /* Query RAW? */

    if (argc == 3 && CMD( argv[2], RAW, 1 ))
    {
        archlvl_query_raw( at );
        return true;
    }

    /* Get pointer to requested facility table entry */

    if (strncasecmp( "BIT", p = argv[2], 3 ) == 0)
        p += 3;

    if (1
        && isdigit( *p )
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
/*                          HHC00890E                                */
/*-------------------------------------------------------------------*/
/* Helper macro to return false immediately if no error message is   */
/* wanted (action == NULL) or call helper function to issue message  */
/* which always returns false.                                       */
/*-------------------------------------------------------------------*/
#define HHC00890E( prereq_bit )     (!action? false : _hhc00890e    \
    ( bitno, target_facname, action, actioning, (prereq_bit),       \
        __FILE__, __LINE__, __FUNCTION__ ))

/*-------------------------------------------------------------------*/
/*                          mod0or7                                  */
/*-------------------------------------------------------------------*/
/*  STFLE implies STFL so 7 implies 0. Therefore if 7 is on, then    */
/* you can't disable 0, and can't enable 7 without also enabling 0.  */
/*-------------------------------------------------------------------*/
static bool mod0or7( bool enable, int bitno, int archnum,
                     const char* action, const char* actioning,
                     const char* target_facname )
{
    if (enable)
    {
        if (bitno == STFL_007_STFL_EXTENDED)
        {
            if (!FACILITY_ENABLED_ARCH( 000_N3_INSTR, archnum ))
                return HHC00890E(  STFL_000_N3_INSTR );
        }
    }
    else // disabling
    {
        if (bitno == STFL_000_N3_INSTR)
        {
            if (FACILITY_ENABLED_ARCH( 007_STFL_EXTENDED, archnum ))
                return HHC00890E( STFL_007_STFL_EXTENDED );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                          modidte                                  */
/*-------------------------------------------------------------------*/
/*         bit 4 implies bit 3, and bit 5 implies bit 4.             */
/*-------------------------------------------------------------------*/
static bool modidte( bool enable, int bitno, int archnum,
                     const char* action, const char* actioning,
                     const char* target_facname )
{
    if (enable)
    {
        if (bitno == STFL_005_IDTE_SC_REGTAB)
        {
            if (!FACILITY_ENABLED_ARCH( 004_IDTE_SC_SEGTAB, archnum ))
                return HHC00890E(  STFL_004_IDTE_SC_SEGTAB );
        }
    }
    else // disabling
    {
        if (bitno == STFL_003_DAT_ENHANCE_1)
        {
            if (FACILITY_ENABLED_ARCH( 004_IDTE_SC_SEGTAB, archnum ))
                return HHC00890E( STFL_004_IDTE_SC_SEGTAB );
        }
        else if (bitno == STFL_004_IDTE_SC_SEGTAB)
        {
            if (FACILITY_ENABLED_ARCH( 005_IDTE_SC_REGTAB, archnum ))
                return HHC00890E( STFL_005_IDTE_SC_REGTAB );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                          modlong                                  */
/*-------------------------------------------------------------------*/
/*                   bit 19 implies bit 18                           */
/*-------------------------------------------------------------------*/
static bool modlong( bool enable, int bitno, int archnum,
                     const char* action, const char* actioning,
                     const char* target_facname )
{
    if (enable)
    {
        if (bitno == STFL_019_LONG_DISPL_HPERF)
        {
            if (!FACILITY_ENABLED_ARCH( 018_LONG_DISPL_INST, archnum ))
                return HHC00890E(  STFL_018_LONG_DISPL_INST );
        }
    }
    else // disabling
    {
        if (bitno == STFL_018_LONG_DISPL_INST)
        {
            if (FACILITY_ENABLED_ARCH( 019_LONG_DISPL_HPERF, archnum ))
                return HHC00890E( STFL_019_LONG_DISPL_HPERF );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                          modtrans                                 */
/*-------------------------------------------------------------------*/
/*         bit 73 implies bit 49, bit 50 implies bit 73              */
/*-------------------------------------------------------------------*/
static bool modtrans( bool enable, int bitno, int archnum,
                      const char* action, const char* actioning,
                      const char* target_facname )
{
    if (enable)
    {
        if (bitno == STFL_050_CONSTR_TRANSACT)
        {
            if (!FACILITY_ENABLED_ARCH( 073_TRANSACT_EXEC, archnum ))
                return HHC00890E(  STFL_073_TRANSACT_EXEC );
        }
        else if (bitno == STFL_073_TRANSACT_EXEC)
        {
            if (!FACILITY_ENABLED_ARCH( 049_EXECUTION_HINT, archnum ))
                return HHC00890E(  STFL_049_EXECUTION_HINT );
        }
    }
    else // disabling
    {
        if (bitno == STFL_049_EXECUTION_HINT)
        {
            if (FACILITY_ENABLED_ARCH( 073_TRANSACT_EXEC, archnum ))
                return HHC00890E( STFL_073_TRANSACT_EXEC );
        }
        else if (bitno == STFL_073_TRANSACT_EXEC)
        {
            if (FACILITY_ENABLED_ARCH( 050_CONSTR_TRANSACT, archnum ))
                return HHC00890E( STFL_050_CONSTR_TRANSACT );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                           modvec                                  */
/*-------------------------------------------------------------------*/
/*              bits 134 and 135 each imply bit 129                  */
/*-------------------------------------------------------------------*/
static bool modvec( bool enable, int bitno, int archnum,
                    const char* action, const char* actioning,
                    const char* target_facname )
{
    if (enable)
    {
        if (0
            || bitno == STFL_134_ZVECTOR_PACK_DEC
            || bitno == STFL_135_ZVECTOR_ENH_1
        )
        {
            if (!FACILITY_ENABLED_ARCH( 129_ZVECTOR, archnum ))
                return HHC00890E(  STFL_129_ZVECTOR );
        }
    }
    else // disabling
    {
        if (bitno == STFL_129_ZVECTOR)
        {
            if (FACILITY_ENABLED_ARCH( 134_ZVECTOR_PACK_DEC, archnum ))
                return HHC00890E( STFL_134_ZVECTOR_PACK_DEC );

            if (FACILITY_ENABLED_ARCH( 135_ZVECTOR_ENH_1, archnum ))
                return HHC00890E( STFL_135_ZVECTOR_ENH_1 );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                           modtod                                  */
/*-------------------------------------------------------------------*/
/*             bit 139 implies both bits 25 and bit 28               */
/*-------------------------------------------------------------------*/
static bool modtod( bool enable, int bitno, int archnum,
                    const char* action, const char* actioning,
                    const char* target_facname )
{
    if (enable)
    {
        if (bitno == STFL_139_MULTIPLE_EPOCH)
        {
            if (!FACILITY_ENABLED_ARCH( 025_STORE_CLOCK_FAST, archnum ))
                return HHC00890E(  STFL_025_STORE_CLOCK_FAST );

            if (!FACILITY_ENABLED_ARCH( 028_TOD_CLOCK_STEER, archnum ))
                return HHC00890E(  STFL_028_TOD_CLOCK_STEER );
        }
    }
    else // disabling
    {
        if (0
            || bitno == STFL_025_STORE_CLOCK_FAST
            || bitno == STFL_028_TOD_CLOCK_STEER
        )
        {
            if (FACILITY_ENABLED_ARCH( 139_MULTIPLE_EPOCH, archnum ))
                return HHC00890E( STFL_139_MULTIPLE_EPOCH );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                           modcpu                                  */
/*-------------------------------------------------------------------*/
/*                    bit 142 implies bit 67                         */
/*-------------------------------------------------------------------*/
static bool modcpu( bool enable, int bitno, int archnum,
                    const char* action, const char* actioning,
                    const char* target_facname )
{
    if (enable)
    {
        if (bitno == STFL_142_ST_CPU_COUNTER_MULT)
        {
            if (!FACILITY_ENABLED_ARCH( 067_CPU_MEAS_COUNTER, archnum ))
                return HHC00890E(  STFL_067_CPU_MEAS_COUNTER );
        }
    }
    else // disabling
    {
        if (bitno == STFL_067_CPU_MEAS_COUNTER)
        {
            if (FACILITY_ENABLED_ARCH( 142_ST_CPU_COUNTER_MULT, archnum ))
                return HHC00890E( STFL_142_ST_CPU_COUNTER_MULT );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                           modmsa                                  */
/*-------------------------------------------------------------------*/
/*                    bit 146 implies bit 76                         */
/*-------------------------------------------------------------------*/
static bool modmsa( bool enable, int bitno, int archnum,
                    const char* action, const char* actioning,
                    const char* target_facname )
{
    if (enable)
    {
        if (bitno == STFL_146_MSA_EXTENSION_8)
        {
            if (!FACILITY_ENABLED_ARCH( 076_MSA_EXTENSION_3, archnum ))
                return HHC00890E(  STFL_076_MSA_EXTENSION_3 );
        }
    }
    else // disabling
    {
        if (bitno == STFL_076_MSA_EXTENSION_3)
        {
            if (FACILITY_ENABLED_ARCH( 146_MSA_EXTENSION_8, archnum ))
                return HHC00890E( STFL_146_MSA_EXTENSION_8 );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                           modbit42                                */
/*-------------------------------------------------------------------*/
/* bit 42 requires special handling as it interacts with many bits   */
/*-------------------------------------------------------------------*/
static bool modbit42( bool enable, int bitno, int archnum,
                      const char* action, const char* actioning,
                      const char* target_facname )
{
    return
    (1
        && modfpx  ( enable, bitno, archnum, action, actioning, target_facname )
        && moddfp  ( enable, bitno, archnum, action, actioning, target_facname )
        && moddfphi( enable, bitno, archnum, action, actioning, target_facname )
    );
}

/*-------------------------------------------------------------------*/
/*                           moddfphi                                */
/*-------------------------------------------------------------------*/
/*                    bit 43 implies bit 42                          */
/*-------------------------------------------------------------------*/
static bool moddfphi( bool enable, int bitno, int archnum,
                      const char* action, const char* actioning,
                      const char* target_facname )
{
    if (enable)
    {
        if (bitno == STFL_043_DFP_HPERF)
        {
            if (!FACILITY_ENABLED_ARCH( 042_DFP, archnum ))
                return HHC00890E(  STFL_042_DFP );
        }
    }
    else // disabling
    {
        if (bitno == STFL_042_DFP)
        {
            if (FACILITY_ENABLED_ARCH( 043_DFP_HPERF, archnum ))
                return HHC00890E( STFL_043_DFP_HPERF );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                            modfpx                                 */
/*-------------------------------------------------------------------*/
/*                    bit 37 implies bit 42                          */
/*-------------------------------------------------------------------*/
static bool modfpx( bool enable, int bitno, int archnum,
                    const char* action, const char* actioning,
                    const char* target_facname )
{
    if (enable)
    {
        if (bitno == STFL_037_FP_EXTENSION)
        {
            if (!FACILITY_ENABLED_ARCH( 042_DFP, archnum ))
                return HHC00890E(  STFL_042_DFP );
        }
    }
    else // disabling
    {
        if (bitno == STFL_042_DFP)
        {
            if (FACILITY_ENABLED_ARCH( 037_FP_EXTENSION, archnum ))
                return HHC00890E( STFL_037_FP_EXTENSION );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                            moddfp                                 */
/*-------------------------------------------------------------------*/
/*                 bit 80 and bit 48 each imply bit 42               */
/*-------------------------------------------------------------------*/
static bool moddfp( bool enable, int bitno, int archnum,
                    const char* action, const char* actioning,
                    const char* target_facname )
{
    if (enable)
    {
        if (0
            || bitno == STFL_048_DFP_ZONE_CONV
            || bitno == STFL_080_DFP_PACK_CONV
        )
        {
            if (!FACILITY_ENABLED_ARCH( 042_DFP, archnum ))
                return HHC00890E(  STFL_042_DFP );
        }
    }
    else // disabling
    {
        if (bitno == STFL_042_DFP)
        {
            if (FACILITY_ENABLED_ARCH( 048_DFP_ZONE_CONV, archnum ))
                return HHC00890E( STFL_048_DFP_ZONE_CONV );

            if (FACILITY_ENABLED_ARCH( 080_DFP_PACK_CONV, archnum ))
                return HHC00890E( STFL_080_DFP_PACK_CONV );
        }
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                     archlvl_enable_disable              (boolean) */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* ARCHLVL ENABLE | DISABLE <facility> | bit [S/370|ESA/390|z/ARCH]  */
/*                                                                   */
/*-------------------------------------------------------------------*/
static int archlvl_enable_disable( int argc, char* argv[] )
{
    const ARCHTAB*  at;
    const FACTAB*   ft;
    const char*     sev;

    int   bitno, fbyte, fbit;
    bool  enable, forced = false;
    bool  enabled, modified;

    strupper( argv[0], argv[0] );

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
            || !isdigit( *p )
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
        const char*  action          = enable? "enable"   : "disable";
        const char*  actioning       = enable? "enabling" : "disabling";
        const char*  target_facname  = get_facname_by_bitno( bitno,
                                                      &target_facname );

        if (!ft->modokfunc( enable, ft->bitno, at->num,
            action, actioning, target_facname ))
            return -1; // (error msg already issued)
    }

    /* Enable (or disable if allowed) the specified facility */
    if (enable)
    {
        sysblk.facility_list[ at->num ][ fbyte ] |= fbit;
    }
    else // want to disable a facility: check if allowed
    {
        if (ft->reqmask & at->amask)
        {
            // "Facility( %s ) is required for %s"
            WRMSG( HHC00897, "E", ft->name, at->name );
            return -1;
        }

        sysblk.facility_list[ at->num ][ fbyte ] &= ~fbit;
    }

    /* Update flags */
    enabled  = sysblk.facility_list[ at->num ][ fbyte ] & fbit;

    modified = ( (ft->defmask & at->amask) && !enabled) ||
               (!(ft->defmask & at->amask) &&  enabled);

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
/*                        archlvl_cmd                       (public) */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* ARCHLVL S/370 | ESA/390 | z/ARCH                                  */
/* ARCHLVL ENABLE | DISABLE <facility> | bit  [S/370|ESA/390|z/ARCH] */
/* ARCHLVL QUERY [ ALL | SHORT | LONG | <facility> | bit | RAW ]     */
/*                                                                   */
/*-------------------------------------------------------------------*/
int archlvl_cmd( int argc, char* argv[], char* cmdline )
{
    bool any_started, storage_reset = false;

    UNREFERENCED( cmdline );

    strupper( argv[0], argv[0] );

    /*-----------------------------------------------------*/
    /*             Display Architecture Mode?              */
    /*-----------------------------------------------------*/

    if (argc < 2)
    {
        // "%-14s: %s"
        WRMSG( HHC02203, "I", "ARCHLVL", get_arch_name( NULL ));
        return 0;
    }

    /* Too many arguments? */

    if (argc > 4)
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
        int rc = archlvl_query( argc, argv ) ? 0 : -1;
        return rc;
    }

    /*-----------------------------------------------------*/
    /*    Make sure all CPUs are deconfigured or stopped   */
    /*-----------------------------------------------------*/

    any_started = are_any_cpus_started();

    /*-----------------------------------------------------*/
    /*             Enable/Disable Facility?                */
    /*-----------------------------------------------------*/

    if (0
        || CMD( argv[1], ENABLE,  3 )
        || CMD( argv[1], DISABLE, 3 )
    )
    {
        if (any_started)
        {
            // "All CPU's must be stopped %s"
            WRMSG( HHC02253, "E", "to modify a facility" );
            return HERRCPUONL;
        }
        return archlvl_enable_disable( argc, argv );
    }

    /*-----------------------------------------------------*/
    /*                Set Architecture                     */
    /*-----------------------------------------------------*/

    if (any_started)
    {
        // "All CPU's must be stopped %s"
        WRMSG( HHC02253, "E", "to switch architectures" );
        return HERRCPUONL;
    }

    /* Set the architecture mode first */
    if (!set_archmode_by_name( argv[1] ))
    {
        // "ARCHLVL '%s' is invalid"
        WRMSG( HHC00895, "E", argv[1] );
        return -1;
    }

    /* Update the dummy regs to match the new archmode. */
    sysblk.dummyregs.arch_mode = sysblk.arch_mode;

    /* Setting the architecture forces a system reset */
    if (1
        && sysblk.arch_mode > ARCH_370_IDX
        && sysblk.mainsize  > 0
        && sysblk.mainsize  < (1 << SHIFT_MEBIBYTE)
    )
    {
        /* Default main storage to 1M and do initial system reset */
        storage_reset = (configure_storage( 1 << (SHIFT_MEBIBYTE - 12) ) == 0);
    }
    else
    {
        OBTAIN_INTLOCK( NULL );
        {
            static const bool clear = false, ipl = false;
            system_reset( sysblk.arch_mode, clear, ipl, sysblk.pcpu );
        }
        RELEASE_INTLOCK( NULL );
    }

    /* Display results */
    if (argc == 2 && MLVL( VERBOSE ))
    {
        // "%-14s set to %s"
        WRMSG( HHC02204, "I", "ARCHLVL", get_arch_name( NULL ));

        if (storage_reset)
        {
            char memsize[128];
            fmt_memsize_KB( sysblk.mainsize >> SHIFT_KIBIBYTE, memsize, sizeof( memsize ));

            // "%-8s storage is %s (%ssize); storage is %slocked"
            WRMSG( HHC17003, "I", "MAIN", memsize, "main",
                sysblk.mainstor_locked ? "" : "not " );
        }
    }

    /* If S/370 archmode was just set, force LPARNUM to BASIC.
       Else if LPARNUM is BASIC, change it back to LPARNUM 1
       if archmode is z/Arch. Else (ARCH_390_IDX) leave it alone.
       The user can override this automatic LPARNUM switching
       via their own subsequent LPARNUM stmt/cmd if such auto-
       matic behavior is not desired.
    */
    if (1
        && ARCH_370_IDX == sysblk.arch_mode
        && om_basic != sysblk.operation_mode
    )
        panel_command( "-LPARNUM BASIC" );
    else if (1
        && ARCH_900_IDX == sysblk.arch_mode
        && om_basic == sysblk.operation_mode
    )
    {
        panel_command( "-LPARNUM 1" );
        panel_command( "-CPUIDFMT 0" );
    }

    return 0;
}

#endif /* !defined( _GEN_ARCH ) */
