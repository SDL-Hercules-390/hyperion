/* ARCHLVL.C    (c) Copyright Jan Jaeger,   2010-2012                */
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
/*                  Architecture bit-masks                           */
/*-------------------------------------------------------------------*/

#define NONE     0x00               /* NO architectures or disabled  */
#define S370     0x80               /* S/370 architecture            */
#define E390     0x40               /* ESA/390 architecture          */
#define Z900     0x20               /* z/Arch architecture           */

#define Z390     (E390|Z900)        /* BOTH ESA/390 and z/Arch       */
#define MALL     (S370|Z390)        /* All architectures             */

/*-------------------------------------------------------------------*/
/*                  Architecture Table structure                     */
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

/*-------------------------------------------------------------------*/
/*                Architecture Level Table (archtab)                 */
/*-------------------------------------------------------------------*/
static ARCHTAB archtab[] =
{
    //---------------------------------------------------------
    //
    //                  PROGRAMMING NOTE
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

    { NULL, 0, 0 }          // (end of table)
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
    // An "Architecture mode" (archmode or arch_mode) is the same
    // thing as an "Architecture number" (archnum or arch_num),
    // which is simply an index value determined at build time
    // depending on which architectures Hercules was built with
    // support for (the "ARCH_nnn_IDX" value #defined by featchk.h)
    // and is used to index into architecture dependent tables such
    // as sysblk.facility_list and the opcode tables in opcode.c

    const ARCHTAB* at;
    for (at = archtab; at->name; at++)
        if (at->num == archnum)
            return at;
    return NULL;
}

/*-------------------------------------------------------------------*/
/*                    get_archtab_by_name                            */
/*-------------------------------------------------------------------*/
static const ARCHTAB* get_archtab_by_name( const char* name )
{
    const ARCHTAB* at;
    for (at = archtab; at->name; at++)
        if (strcasecmp( at->name, name ) == 0)
            return at;
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
/*                  Facility Table structure                         */
/*-------------------------------------------------------------------*/
struct FACTAB
{
    const char*  name;              /* Facility Name                 */
    const int    bitno;             /* Bit number                    */
    const int    supmask;           /* Which archs support it        */
    const int    defmask;           /* Default for which archs       */
    const int    reqmask;           /* Which archs require it        */
};
typedef struct FACTAB   FACTAB;

#define FT( _sup, _def, _req, _name )                               \
{                                                                   \
    QSTR( _name ),                                                  \
    STFL_ ## _name,  /* esa390.h 'STFL_XXX' bit number #define */   \
                                                                    \
    (_sup),                                                         \
    (_def),                                                         \
    (_req)                                                          \
},

/*-------------------------------------------------------------------*/
/*                      Facility Table (factab)                      */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The facility names used in the below FT macro invocations are,    */
/* when prefixed with 'STFL_', the facility bit number as #defined   */
/* in header esa390.h (e.g. FT( 044_PFPO, ..) ==> STFL_044_PFPO 44)  */
/*                                                                   */
/* Also note that the entries in the below table do NOT have to be   */
/* in any facility bit sequence as it is always seached serially.    */
/* However, it is greatly preferred that it be kept in sequence.     */
/*                                                                   */
/* PROGRAMMING NOTE: All facilities, regardless of whether they are  */
/* currently supported or not, should be defined in the below table  */
/* and guarded by #if defined statements.  If the facility is not    */
/* currently supported/implemented, the table entry should specify   */
/* "NONE" in the Supported, Default and Required columns.  Once      */
/* support for the facility is eventually implemented, then simply   */
/* update the previously mentioned columns as appropriate, thereby   */
/* allowing the facility to be enabled whenever desired (either at   */
/* startup by default according to architecture, or dynamically on   */
/* demand via the "ARCHLVL ENABLE <facility-name>" command).         */
/*                                                                   */
/*-------------------------------------------------------------------*/

static FACTAB factab[] = {

/*-------------------------------------------------------------------*/
/*  Sup   Def   Req   Short Name...                                  */
/*-------------------------------------------------------------------*/

#if defined( _FEATURE_000_N3_INSTR_FACILITY )
FT( Z390, Z390, NONE, 000_N3_ESA390 )
#endif

#if defined( _FEATURE_001_ZARCH_INSTALLED_FACILITY )
FT( Z390, Z390, NONE, 001_ZARCH_INSTALLED )
FT( Z900, Z900, Z900, 002_ZARCH_ACTIVE )
#endif

#if defined( _FEATURE_003_DAT_ENHANCE_FACILITY_1 )
FT( Z390, Z390, NONE, 003_DAT_ENHANCE_1 )
#endif

#if defined( _FEATURE_004_IDTE_SC_SEGTAB_FACILITY )
FT( NONE, NONE, NONE, 004_IDTE_SC_SEGTAB )
#endif

#if defined( _FEATURE_005_IDTE_SC_REGTAB_FACILITY )
FT( NONE, NONE, NONE, 005_IDTE_SC_REGTAB )
#endif

#if defined( _FEATURE_006_ASN_LX_REUSE_FACILITY )
FT( Z900, Z900, NONE, 006_ASN_LX_REUSE )
#endif

#if defined( _FEATURE_007_STFL_EXTENDED_FACILITY )
FT( Z900, Z900, NONE, 007_STFL_EXTENDED )
#endif

#if defined( _FEATURE_008_ENHANCED_DAT_FACILITY_1 )
FT( Z900, Z900, NONE, 008_EDAT_1 )
#endif

#if defined( _FEATURE_009_SENSE_RUN_STATUS_FACILITY )
FT( Z900, Z900, NONE, 009_SENSE_RUN_STATUS )
#endif

#if defined( _FEATURE_010_CONDITIONAL_SSKE_FACILITY )
FT( Z900, Z900, NONE, 010_CONDITIONAL_SSKE )
#endif

#if defined( _FEATURE_011_CONFIG_TOPOLOGY_FACILITY )
FT( Z900, Z900, NONE, 011_CONFIG_TOPOLOGY )
#endif

#if defined( _FEATURE_013_IPTE_RANGE_FACILITY )
FT( Z900, Z900, NONE, 013_IPTE_RANGE )
#endif

#if defined( _FEATURE_014_NONQ_KEY_SET_FACILITY )
FT( Z900, Z900, NONE, 014_NONQ_KEY_SET )
#endif

#if defined( _FEATURE_016_EXT_TRANSL_FACILITY_2 )
FT( Z390, Z390, NONE, 016_EXT_TRANSL_2 )
#endif

#if defined( _FEATURE_017_MSA_FACILITY )
FT( Z390, Z390, NONE, 017_MSA )
#endif

#if defined( _FEATURE_018_LONG_DISPL_INST_FACILITY )
FT( Z390, Z390, NONE, 018_LONG_DISPL_INST )
#endif

#if defined( _FEATURE_019_LONG_DISPL_HPERF_FACILITY )
FT( Z390, Z390, NONE, 019_LONG_DISPL_HPERF )
#endif

#if defined( _FEATURE_020_HFP_MULT_ADD_SUB_FACILITY )
FT( Z390, Z390, NONE, 020_HFP_MULT_ADD_SUB )
#endif

#if defined( _FEATURE_021_EXTENDED_IMMED_FACILITY )
FT( Z900, Z900, NONE, 021_EXTENDED_IMMED )
#endif

#if defined( _FEATURE_022_EXT_TRANSL_FACILITY_3 )
FT( Z900, Z900, NONE, 022_EXT_TRANSL_3 )
#endif

#if defined( _FEATURE_023_HFP_UNNORM_EXT_FACILITY )
FT( Z900, Z900, NONE, 023_HFP_UNNORM_EXT )
#endif

#if defined( _FEATURE_024_ETF2_ENHANCEMENT_FACILITY )
FT( Z900, Z900, NONE, 024_ETF2_ENHANCEMENT )
#endif

#if defined( _FEATURE_025_STORE_CLOCK_FAST_FACILITY )
FT( Z900, Z900, NONE, 025_STORE_CLOCK_FAST )
#endif

#if defined( _FEATURE_026_PARSING_ENHANCE_FACILITY )
FT( Z900, Z900, NONE, 026_PARSING_ENHANCE )
#endif

#if defined( _FEATURE_027_MVCOS_FACILITY )
FT( Z900, Z900, NONE, 027_MVCOS )
#endif

#if defined( _FEATURE_028_TOD_CLOCK_STEER_FACILITY )
FT( Z900, Z900, NONE, 028_TOD_CLOCK_STEER )
#endif

#if defined( _FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
FT( Z900, Z900, NONE, 030_ETF3_ENHANCEMENT )
#endif

#if defined( _FEATURE_031_EXTRACT_CPU_TIME_FACILITY )
FT( Z900, Z900, NONE, 031_EXTRACT_CPU_TIME )
#endif

#if defined( _FEATURE_032_CSS_FACILITY )
FT( Z900, Z900, NONE, 032_CSSF )
#endif

#if defined( _FEATURE_033_CSS_FACILITY_2 )
FT( Z900, Z900, NONE, 033_CSSF2 )
#endif

#if defined( _FEATURE_034_GEN_INST_EXTN_FACILITY )
FT( Z900, Z900, NONE, 034_GEN_INST_EXTN )
#endif

#if defined( _FEATURE_035_EXECUTE_EXTN_FACILITY )
FT( Z900, Z900, NONE, 035_EXECUTE_EXTN )
#endif

#if defined( _FEATURE_036_ENH_MONITOR_FACILITY )
FT( Z900, Z900, NONE, 036_ENH_MONITOR )
#endif

#if defined( _FEATURE_037_FP_EXTENSION_FACILITY )
FT( Z900, Z900, NONE, 037_FP_EXTENSION )
#endif

#if defined( _FEATURE_038_OP_CMPSC_FACILITY )
FT( NONE, NONE, NONE, 038_OP_CMPSC )
#endif

#if defined( _FEATURE_040_LOAD_PROG_PARAM_FACILITY )
FT( Z900, Z900, NONE, 040_LOAD_PROG_PARAM )
#endif

//------------------------------------------------------------------------------

#if defined( _FEATURE_041_FPS_ENHANCEMENT_FACILITY )

FT( Z900, Z900, NONE, 041_FPS_ENHANCEMENT )

#if defined( _FEATURE_041_DFP_ROUNDING_FACILITY )
FT( Z900, Z900, NONE, 041_DFP_ROUNDING )
#endif

#if defined( _FEATURE_041_FPR_GR_TRANSFER_FACILITY )
FT( Z900, Z900, NONE, 041_FPR_GR_TRANSFER )
#endif

#if defined( _FEATURE_041_FPS_SIGN_HANDLING_FACILITY )
FT( Z900, Z900, NONE, 041_FPS_SIGN_HANDLING )
#endif

#if defined( _FEATURE_041_IEEE_EXCEPT_SIM_FACILITY )
FT( Z900, Z900, NONE, 041_IEEE_EXCEPT_SIM )
#endif

#endif /* defined( _FEATURE_041_FPS_ENHANCEMENT_FACILITY ) */

//------------------------------------------------------------------------------

#if defined( _FEATURE_042_DFP_FACILITY )
FT( Z900, Z900, NONE, 042_DFP )
#endif

#if defined( _FEATURE_043_DFP_HPERF_FACILITY )
FT( Z900, Z900, NONE, 043_DFP_HPERF )
#endif

//------------------------------------------------------------------------------
// Allow the PFPO facility to be manually enabled Until proper support for it
// is coded since some modern operating systems require it (z/OS and z/VM).

//#if defined( _FEATURE_044_PFPO_FACILITY )
FT( Z900, NONE, NONE, 044_PFPO )
//#endif

//------------------------------------------------------------------------------

#if defined( _FEATURE_045_FAST_BCR_SERIAL_FACILITY )
FT( Z900, Z900, NONE, 045_FAST_BCR_SERIAL )
#endif

#if defined( _FEATURE_047_CMPSC_ENH_FACILITY )
FT( Z900, Z900, NONE, 047_CMPSC_ENH )
#endif

#if defined( _FEATURE_048_DFP_ZONE_CONV_FACILITY )
FT( Z900, Z900, NONE, 048_DFP_ZONE_CONV )
#endif

#if defined( _FEATURE_049_EXECUTION_HINT_FACILITY )
FT( Z900, Z900, NONE, 049_EXECUTION_HINT )
#endif

#if defined( _FEATURE_049_LOAD_AND_TRAP_FACILITY )
FT( Z900, Z900, NONE, 049_LOAD_AND_TRAP )
#endif

#if defined( _FEATURE_049_PROCESSOR_ASSIST_FACILITY )
FT( Z900, Z900, NONE, 049_PROCESSOR_ASSIST )
#endif

#if defined( _FEATURE_049_MISC_INSTR_EXT_FACILITY_1 )
FT( Z900, Z900, NONE, 049_MISC_INSTR_EXT_1 )
#endif

#if defined( _FEATURE_050_CONSTR_TRANSACT_FACILITY )
FT( NONE, NONE, NONE, 050_CONSTR_TRANSACT )
#endif

#if defined( _FEATURE_051_LOCAL_TLB_CLEARING_FACILITY )
FT( NONE, NONE, NONE, 051_LOCAL_TLB_CLEARING )
#endif

//------------------------------------------------------------------------------

#if defined( _FEATURE_052_INTERLOCKED_ACCESS_FACILITY_2 )

#if CAN_IAF2 != IAF2_ATOMICS_UNAVAILABLE
FT( Z900, Z900, NONE, 052_INTERLOCKED_ACCESS_2 )
#endif

#endif /* defined( _FEATURE_052_INTERLOCKED_ACCESS_FACILITY_2 ) */

//------------------------------------------------------------------------------

#if defined( _FEATURE_053_LOAD_STORE_ON_COND_FACILITY_2 )
FT( NONE, NONE, NONE, 053_LOAD_STORE_ON_COND_2 )
#endif

#if defined( _FEATURE_053_LOAD_ZERO_RIGHTMOST_FACILITY )
FT( NONE, NONE, NONE, 053_LOAD_ZERO_RIGHTMOST )
#endif

#if defined( _FEATURE_054_EE_CMPSC_FACILITY )
FT( NONE, NONE, NONE, 054_EE_CMPSC )
#endif

#if defined( _FEATURE_057_MSA_EXTENSION_FACILITY_5 )
FT( NONE, NONE, NONE, 057_MSA_EXTENSION_5 )
#endif

#if defined( _FEATURE_058_MISC_INSTR_EXT_FACILITY_2 )
FT( NONE, NONE, NONE, 058_MISC_INSTR_EXT_2 )
#endif

#if defined( _FEATURE_066_RES_REF_BITS_MULT_FACILITY )
FT( Z900, Z900, NONE, 066_RES_REF_BITS_MULT )
#endif

#if defined( _FEATURE_067_CPU_MEAS_COUNTER_FACILITY )
FT( Z900, Z900, NONE, 067_CPU_MEAS_COUNTER )
#endif

#if defined( _FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY )
FT( Z900, Z900, NONE, 068_CPU_MEAS_SAMPLNG )
#endif

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
FT( NONE, NONE, NONE, 073_TRANSACT_EXEC )
#endif

#if defined( _FEATURE_074_STORE_HYPER_INFO_FACILITY )
FT( NONE, NONE, NONE, 074_STORE_HYPER_INFO )
#endif

#if defined( _FEATURE_075_ACC_EX_FS_INDIC_FACILITY )
FT( Z900, Z900, NONE, 075_ACC_EX_FS_INDIC )
#endif

#if defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 )
FT( Z900, Z900, NONE, 076_MSA_EXTENSION_3 )
#endif

#if defined( _FEATURE_077_MSA_EXTENSION_FACILITY_4 )
FT( Z900, Z900, NONE, 077_MSA_EXTENSION_4 )
#endif

#if defined( _FEATURE_078_ENHANCED_DAT_FACILITY_2 )
FT( NONE, NONE, NONE, 078_EDAT_2 )
#endif

#if defined( _FEATURE_080_DFP_PACK_CONV_FACILITY )
FT( NONE, NONE, NONE, 080_DFP_PACK_CONV )
#endif

#if defined( _FEATURE_129_ZVECTOR_FACILITY )
FT( NONE, NONE, NONE, 129_ZVECTOR )
#endif

#if defined( _FEATURE_130_INSTR_EXEC_PROT_FACILITY )
FT( NONE, NONE, NONE, 130_INSTR_EXEC_PROT )
#endif

#if defined( _FEATURE_131_SIDE_EFFECT_ACCESS_FACILITY )
FT( NONE, NONE, NONE, 131_SIDE_EFFECT_ACCESS )
#endif

#if defined( _FEATURE_133_GUARDED_STORAGE_FACILITY )
FT( NONE, NONE, NONE, 133_GUARDED_STORAGE )
#endif

#if defined( _FEATURE_134_ZVECTOR_PACK_DEC_FACILITY )
FT( NONE, NONE, NONE, 134_ZVECTOR_PACK_DEC )
#endif

#if defined( _FEATURE_135_ZVECTOR_ENH_FACILITY_1 )
FT( NONE, NONE, NONE, 135_ZVECTOR_ENH_1 )
#endif

#if defined( _FEATURE_138_CONFIG_ZARCH_MODE_FACILITY )
FT( NONE, NONE, NONE, 138_CONFIG_ZARCH_MODE )
#endif

#if defined( _FEATURE_139_MULTIPLE_EPOCH_FACILITY )
FT( NONE, NONE, NONE, 139_MULTIPLE_EPOCH )
#endif

#if defined( _FEATURE_142_ST_CPU_COUNTER_MULT_FACILITY )
FT( NONE, NONE, NONE, 142_ST_CPU_COUNTER_MULT )
#endif

#if defined( _FEATURE_144_TEST_PEND_EXTERNAL_FACILITY )
FT( NONE, NONE, NONE, 144_TEST_PEND_EXTERNAL )
#endif

#if defined( _FEATURE_145_INS_REF_BITS_MULT_FACILITY )
FT( NONE, NONE, NONE, 145_INS_REF_BITS_MULT )
#endif

#if defined( _FEATURE_146_MSA_EXTENSION_FACILITY_8 )
FT( NONE, NONE, NONE, 146_MSA_EXTENSION_8 )
#endif

#if defined( _FEATURE_168_ESA390_COMPAT_MODE_FACILITY )
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

FT( MALL, MALL, Z900, HERC_MOVE_INVERSE )

#if defined(      _FEATURE_MSA_EXTENSION_FACILITY_1 )
FT( Z390, Z390, NONE, HERC_MSA_EXTENSION_1 )
#endif

#if defined(      _FEATURE_MSA_EXTENSION_FACILITY_2 )
FT( Z390, Z390, NONE, HERC_MSA_EXTENSION_2 )
#endif

#if defined( _FEATURE_HERCULES_DIAGCALLS )
FT( MALL, NONE, NONE, HERC_PROBSTATE_DIAGF08 )
FT( MALL, NONE, NONE, HERC_SIGP_SETARCH_S370 )
#if defined(      _FEATURE_HOST_RESOURCE_ACCESS_FACILITY )
FT( MALL, NONE, NONE, HERC_HOST_RESOURCE_ACCESS )
#endif
#endif

#if defined(      _FEATURE_QEBSM )
FT( Z390, Z390, NONE, HERC_QEBSM )
#endif

#if defined(      _FEATURE_QDIO_THININT )
FT( Z390, Z390, NONE, HERC_QDIO_THININT )
#endif

#if defined(      _FEATURE_QDIO_TDD )
FT( Z390, NONE, NONE, HERC_QDIO_TDD )
#endif

#if defined(      _FEATURE_SVS )
FT( Z390, Z390, NONE, HERC_SVS )
#endif

#if defined(         _FEATURE_HYPERVISOR )
FT( MALL, MALL, NONE, HERC_LOGICAL_PARTITION )
#endif

#if defined(         _FEATURE_EMULATE_VM )
FT( MALL, NONE, NONE, HERC_VIRTUAL_MACHINE )
#endif

FT( Z390, NONE, NONE, HERC_QDIO_ASSIST )

#if defined(      _FEATURE_INTERVAL_TIMER )
FT( MALL, MALL, Z390, HERC_INTERVAL_TIMER )
#endif

FT( MALL, MALL, NONE, HERC_DETECT_PGMINTLOOP )

{ NULL, 0, 0, 0, 0 }
};

/*-------------------------------------------------------------------*/
/*                   init_facilities_lists                  (public) */
/*-------------------------------------------------------------------*/
void init_facilities_lists()
{
    // Called ONCE by bldcfg.c 'build_config' during system startup.

    const ARCHTAB* at;
    const FACTAB*  ft;
    int arch, fbyte, fbit;

    // Clear all architectures' facilities lists to ZEROS

    for (arch = 0; arch < NUM_GEN_ARCHS; arch++)
        for (fbyte = 0; fbyte < (int) STFL_HERC_BY_SIZE; fbyte++)
            sysblk.facility_list[ arch ][ fbyte ] = 0;

    // Initialize all architectures' facilities lists to their default

    for (arch = 0; arch < NUM_GEN_ARCHS; arch++)
    {
        at = get_archtab_by_arch( arch );

        for (ft = factab; ft->name; ft++)
        {
            // Initialize this architecture's facility bit only if
            // this facility applies to this architecture and it's
            // either a required facility or defaults to enabled.

            if (1
                && (ft->supmask & at->amask)      // (applies?)
                && (0
                    || (ft->reqmask & at->amask)  // (required?)
                    || (ft->defmask & at->amask)  // (default?)
                   )
            )
            {
                fbyte =         (ft->bitno / 8);
                fbit  = 0x80 >> (ft->bitno % 8);

                sysblk.facility_list[ arch ][ fbyte ] |= fbit;
            }
        }
    }
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
    const FACTAB* ft;
    for (ft = factab; ft->name; ft++)
        if (strcasecmp( ft->name, name ) == 0)
            return ft;
    return NULL;
}

/*-------------------------------------------------------------------*/
/*                      get_factab_by_bitno                          */
/*-------------------------------------------------------------------*/
static const FACTAB* get_factab_by_bitno( int bitno )
{
    const FACTAB* ft;

    if (0
        || bitno < 0
        || bitno > (int) STFL_HERC_LAST_BIT
    )
        return NULL;

    for (ft = factab; ft->name; ft++)
        if (ft->bitno == bitno)
            return ft;
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
        *name = strdup( ft->name );
    return *name;
}

/*-------------------------------------------------------------------*/
/*                     archlvl_enable_disable              (boolean) */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*   ARCHLVL  ENABLE | DISABLE  <facility>  [S/370|ESA/390|Z/ARCH]   */
/*                                                                   */
/*-------------------------------------------------------------------*/
static bool archlvl_enable_disable( int argc, char* argv[] )
{
    const ARCHTAB*  at;
    const FACTAB*   ft;

    int   bitno, fbyte, fbit;
    bool  enable, forced = false;

    ASSERT( argc >= 3 );

    strupper( argv[0], argv[0] );

    if (argc < 2 || argc > 4)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return false;
    }

    if      (CMD( argv[1], ENABLE,  3 )) enable = true;
    else if (CMD( argv[1], DISABLE, 4 )) enable = false;
    else
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[1], "" );
        return false;
    }

    if (argc < 3)
    {
        // "Facility name not specified"
        WRMSG( HHC00892, "E" );
        return false;
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
            // "Archmode '%s' is invalid"
            WRMSG( HHC00895, "E", argv[3] );
            return false;
        }
    }

    /* Get pointer to specified Facility Table entry */
    if (!(ft = get_factab_by_name( argv[2] )))
    {
        char  c;

        if (0
            || strncasecmp( "BIT", argv[2], 3 ) != 0
            || !isdigit( *( argv[2] + 3))
            || sscanf(      argv[2] + 3, "%d%c", &bitno, &c ) != 1
            || bitno < 0
            || bitno > (int) STFL_HERC_LAST_BIT
            || !(ft = get_factab_by_bitno( bitno ))
        )
        {
            // "Facility( %s ) does not exist"
            WRMSG( HHC00893, "E", argv[2] );
            return false;
        }

        forced = true;  // (bypasses "required" check)
    }
    else
        bitno = ft->bitno;

    /* Verify facility is supported for this architecture */
    if (!(ft->supmask & at->amask))
    {
        // "Facility( %s ) not supported for archmode '%s'"
        WRMSG( HHC00896, "E", ft->name, at->name );
        return false;
    }

    /* Enable or disabled specified facility bit if allowed */

    fbyte  =         (bitno / 8);
    fbit   = 0x80 >> (bitno % 8);

    if (enable)
    {
        sysblk.facility_list[ at->num ][fbyte ] |= fbit;
    }
    else // disable
    {
        /* Should we allow disabling this bit? */
        if (!forced && ft->reqmask & at->amask)
        {
            // HHC00897 "Facility( %s ) is required for archmode '%s'"
            WRMSG( HHC00897, "E", ft->name, at->name );
            return false;
        }

        sysblk.facility_list[ at->num ][fbyte] &= ~fbit;
    }

    if (MLVL( VERBOSE ))
    {
        const char*  endis  = enable ? "en" : "dis";
        // "Facility( %s ) %sabled for archmode %s"
        WRMSG( HHC00898, "I", ft->name, endis, at->name );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*                        archlvl_cmd                       (public) */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*   ARCHLVL  S/370  |  ESA/390  |  Z/ARCH                           */
/*   ARCHLVL  ENABLE |  DISABLE  <facility>  [S/370|ESA/390|Z/ARCH]  */
/*   ARCHLVL  QUERY    [ ALL  |  <facility> ]                       */
/*                                                                   */
/*-------------------------------------------------------------------*/
int archlvl_cmd( int argc, char* argv[], char* cmdline )
{
    bool storage_reset = false;

    UNREFERENCED( cmdline );

    strupper( argv[0], argv[0] );

    // Display current architecture mode?

    if (argc < 2)
    {
        // "%-14s: %s"
        WRMSG( HHC02203, "I", "ARCHLVL", get_arch_name( NULL ));
        return 0;
    }

    // Too many arguments?

    if (argc > 4)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    // Query facility/facilities...

    if (CMD( argv[1], QUERY, 1 ))
    {
        FACTAB*  ft;
        int      fbyte, fbit, bitno;
        char     c;

        if (argc > 3)
        {
            // "Invalid command usage. Type 'help %s' for assistance."
            WRMSG( HHC02299, "E", argv[0] );
            return -1;
        }

        // Check if they queried a specific bit number (e.g. "BIT44")

        if (1
            && strncasecmp( "BIT", argv[2],  3 ) == 0
            && isdigit(         *( argv[2] + 3 ))
            && sscanf(             argv[2] + 3, "%d%c", &bitno, &c ) == 1
            &&                                           bitno >= 0
            &&                                           bitno <= (int) STFL_HERC_LAST_BIT
        )
        {
            // Get actual facility name
            const char* name;
            name = get_facname_by_bitno( bitno, &name );

            // Calculate facility byte and bit mask
            fbyte =         (bitno / 8);
            fbit  = 0x80 >> (bitno % 8);

            // "Facility( %-27s ) %sabled"
            WRMSG( HHC00890, "I", name,
                sysblk.facility_list[ sysblk.arch_mode ][ fbyte ] & fbit ?
                "En" : "Dis" );

            free( (void*) name );
        }
        else
        {
            // Either they're querying a specific facility, or they're
            // querying 'ALL' facilities, or else their "BITnn" query
            // was out of range.

            bool all = (argc < 3 || CMD( argv[2], ALL, 3 ));

            for (ft = factab; ft->name; ft++)
            {
                // Display this facility's setting
                if (all || strcasecmp( argv[2], ft->name ) == 0)
                {
                    // Calculate facility byte and bit mask
                    fbyte =         (ft->bitno / 8);
                    fbit  = 0x80 >> (ft->bitno % 8);

                    // "Facility( %-27s ) %sabled"
                    WRMSG( HHC00890, "I", ft->name,
                        sysblk.facility_list[ sysblk.arch_mode ][ fbyte ] & fbit ?
                        "En" : "Dis" );

                    if (!all)       // (querying specific facility?)
                        break;      // (if so then we have found it)
                }
            }

            if (!all && !ft->name)  // (specific facility not found?)
            {
                // "Facility( %s ) does not exist"
                WRMSG( HHC00893, "E", argv[2] );
                return -1;
            }
        }

        return 0;   // (success)
    }

    // Set architecture mode or enable/disable facility...

    /* Make sure all CPUs are deconfigured or stopped */
    if (are_any_cpus_started())
    {
        // "All CPU's must be stopped to change architecture"
        WRMSG( HHC02253, "E" );
        return HERRCPUONL;
    }

    /* Try setting the architecture mode first */
    if (!set_archmode_by_name( argv[1] ))
    {
        /* That didn't work so it's not an architecture name.
           It's probably an ENABLE/DISABLE facility command.
        */
        int rc = archlvl_enable_disable( argc, argv ) ? 0 : -1;
        return rc;
    }

    /* Update dummy regs with possibly new archmode */
    sysblk.dummyregs.arch_mode = sysblk.arch_mode;

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
            const bool clear = false, ipl = false;
            system_reset( sysblk.arch_mode, clear, ipl, sysblk.pcpu );
        }
        RELEASE_INTLOCK( NULL );
    }

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
