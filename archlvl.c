/* ARCHLVL.C    (c) Copyright Jan Jaeger,   2010-2012                */
/*                                                                   */

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

#if !defined(_GEN_ARCH)

#if defined(_ARCHMODE3)
 #define  _GEN_ARCH _ARCHMODE3
 #include "archlvl.c"
 #undef   _GEN_ARCH
#endif

#if defined(_ARCHMODE2)
 #define  _GEN_ARCH _ARCHMODE2
 #include "archlvl.c"
 #undef   _GEN_ARCH
#endif

/*-------------------------------------------------------------------*/
/*           ARCHTAB struct  (Architecture Level Table)              */
/*-------------------------------------------------------------------*/
struct ARCHTAB
{
    const char*  name;              /* Archlvl Name                  */
    const int    archmode;          /* Architecture Mode             */
    const int    alslevel;          /* Architecture Level            */

#define ALS0     0x01               /* S/370                         */
#define ALS1     0x02               /* ESA/390                       */
#define ALS2     0x04               /* Z/ARCH                        */
#define ALS3     0x08               /* ARCHLVL 3                     */

#define ALS23    (ALS2|ALS3)        /* ESA/390 + z/Arch              */
#define ALS123   (ALS1|ALS23)       /* S/370 + ESA/390 + z/Arch      */
#define ALSALL   (ALS0|ALS123)      /* ALL Architecture Levels       */

};
typedef struct ARCHTAB  ARCHTAB;

#define ARCHLVL( _name, _mode, _level )     \
    {                                       \
        (_name),                            \
        (_mode),                            \
        (_level)                            \
    },

/*-------------------------------------------------------------------*/
/*                 Architecture Level Table (archtab)                */
/*-------------------------------------------------------------------*/
static ARCHTAB archtab[] =
{
#if defined( _370 )

    /* S/370 - ALS0 */
    ARCHLVL( _ARCH_370_NAME,  ARCH_370, ALS0 )
    ARCHLVL( "370",           ARCH_370, ALS0 )
    ARCHLVL( "S370",          ARCH_370, ALS0 )
    ARCHLVL( "S/370",         ARCH_370, ALS0 )
    ARCHLVL( "ALS0",          ARCH_370, ALS0 )
#endif

/*
    Note that XA and XB are not offered, and
    neither is G3 (debut of relative/immediate).
*/
#if defined( _390 )

    /* ESA/390 - ALS1 */
    ARCHLVL( _ARCH_390_NAME,  ARCH_390, ALS1 )
    ARCHLVL( "ESA",           ARCH_390, ALS1 )
    ARCHLVL( "ESA390",        ARCH_390, ALS1 )
    ARCHLVL( "S/390",         ARCH_390, ALS1 )
    ARCHLVL( "S390",          ARCH_390, ALS1 )
    ARCHLVL( "390",           ARCH_390, ALS1 )
    ARCHLVL( "ALS1",          ARCH_390, ALS1 )
#endif

#if defined( _900 )

    /* z/Arch - ALS2 */
    ARCHLVL( "ESA/ME",        ARCH_900, ALS2 )
    ARCHLVL( "ESAME",         ARCH_900, ALS2 )
    ARCHLVL( "ALS2",          ARCH_900, ALS2 )

    /* z/Arch - ALS3 */
    ARCHLVL( _ARCH_900_NAME,  ARCH_900, ALS3 )
    ARCHLVL( "zArch",         ARCH_900, ALS3 )
    ARCHLVL( "z",             ARCH_900, ALS3 )
    ARCHLVL( "ALS3",          ARCH_900, ALS3 )
#endif

    { NULL, 0, 0 }          // (end of table)
};

/*-------------------------------------------------------------------*/
/*                FACTAB struct  (Facility Table)                    */
/*-------------------------------------------------------------------*/
struct FACTAB
{
    const char*  name;              /* Facility Name                 */
    const int    bitno;             /* Bit number                    */
    const BYTE   mode;              /* Mode indicator                */

#define S370     0x01               /* S/370 feature                 */
#define ESA390   0x02               /* ESA/390 feature               */
#define ZARCH    0x04               /* z/Arch feature                */
#define Z390     (ESA390|ZARCH)     /* Exists in z/Arch only, but
                                      is also indicated in ESA390    */
#define ALLM     (S370|Z390)        /* ALL modes                     */

    const BYTE   fixed;             /* Mandatory for                 */

#define NONE     0x00

    const BYTE   supported;         /* Supported in                  */
    const BYTE   alslevel;          /* alslevel grouping             */
};
typedef struct FACTAB   FACTAB;

#define FACILITY( _name, _mode, _fixed, _supp, _level )         \
    {                                                           \
        QSTR( _name ),                                          \
        (STFL_ ## _name), /* (see 'STFL_XXXX' in "esa390.h") */ \
        (_mode),                                                \
        (_fixed),                                               \
        (_supp),                                                \
        (_level)                                                \
    },

/*-------------------------------------------------------------------*/
/*                      Facility Table (factab)                      */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The facility names used in the below FACILITY macro invocations   */
/* are, when prefixed with 'STFL_', #defined in the esa390.h header. */
/* E.g. FACILITY( 044_PFPO, ... )  ==>  #define STFL_044_PFPO  44.   */
/*                                                                   */
/* Also note that the entries in the below table do NOT have to be   */
/* in any facility bit sequence as it is always seached serially.    */
/* However, it is greatly preferred that it be kept in sequence.     */
/*                                                                   */
/* As a convention, all Hercules-specific facilities that are not    */
/* part of the architecture are defined at the end of the table.     */
/*                                                                   */
/* PROGRAMMING NOTE: All facilities, regardless of whether they are  */
/* currently supported or not, should be defined in the below table  */
/* and guarded by #if defined statements.  If the facility is not    */
/* currently supported/implemented, the table entry should specify   */
/* "NONE" in the Default, Mandatory and Supported columns.  Once     */
/* support for the facility is eventually implemented, then simply   */
/* update the previously mentioned columns as appropriate, thereby   */
/* allowing the facility to be enabled whenever desired (either at   */
/* startup by default according to architecture, or on demand via    */
/* the 'archlvl' command).                                           */
/*                                                                   */
/*-------------------------------------------------------------------*/

static FACTAB factab[] = {

//        Facility                  Default     Mandatory   Supported   Group

#if defined( _FEATURE_000_N3_INSTR_FACILITY )
FACILITY( 000_N3_ESA390,            Z390,       NONE,       Z390,       ALS123 )
#endif

#if defined( _FEATURE_001_ZARCH_INSTALLED_FACILITY )
FACILITY( 001_ZARCH_INSTALLED,      Z390,       NONE,       Z390,       ALS23 )
FACILITY( 002_ZARCH_ACTIVE,         ZARCH,      ZARCH,      ZARCH,      ALS23 )
#endif

#if defined( _FEATURE_003_DAT_ENHANCE_FACILITY_1 )
FACILITY( 003_DAT_ENHANCE_1,        Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_004_IDTE_SC_SEGTAB_FACILITY )
//CILITY( 004_IDTE_SC_SEGTAB,       ZARCH,      NONE,       ZARCH,      ALS23 )
#endif

#if defined( _FEATURE_005_IDTE_SC_REGTAB_FACILITY )
//CILITY( 005_IDTE_SC_REGTAB,       ZARCH,      NONE,       ZARCH,      ALS23 )
#endif

#if defined( _FEATURE_006_ASN_LX_REUSE_FACILITY )
//CILITY( 006_ASN_LX_REUSE,         Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_007_STFL_EXTENDED_FACILITY )
FACILITY( 007_STFL_EXTENDED,        Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_008_ENHANCED_DAT_FACILITY_1 )
FACILITY( 008_EDAT_1,               Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_009_SENSE_RUN_STATUS_FACILITY )
FACILITY( 009_SENSE_RUN_STATUS,     Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_010_CONDITIONAL_SSKE_FACILITY )
FACILITY( 010_CONDITIONAL_SSKE,     Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_011_CONFIG_TOPOLOGY_FACILITY )
FACILITY( 011_CONFIG_TOPOLOGY,      Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_013_IPTE_RANGE_FACILITY )
FACILITY( 013_IPTE_RANGE,           Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_014_NONQ_KEY_SET_FACILITY )
FACILITY( 014_NONQ_KEY_SET,         Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_016_EXT_TRANSL_FACILITY_2 )
FACILITY( 016_EXT_TRANSL_2,         Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_017_MSA_FACILITY )
FACILITY( 017_MSA,                  Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_018_LONG_DISPL_INST_FACILITY )
FACILITY( 018_LONG_DISPL_INST,      Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_019_LONG_DISPL_HPERF_FACILITY )
FACILITY( 019_LONG_DISPL_HPERF,     ZARCH,      NONE,       ZARCH,      ALS23 )
#endif

#if defined( _FEATURE_020_HFP_MULT_ADD_SUB_FACILITY )
FACILITY( 020_HFP_MULT_ADD_SUB,     Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_021_EXTENDED_IMMED_FACILITY )
FACILITY( 021_EXTENDED_IMMED,       Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_022_EXT_TRANSL_FACILITY_3 )
FACILITY( 022_EXT_TRANSL_3,         Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_023_HFP_UNNORM_EXT_FACILITY )
FACILITY( 023_HFP_UNNORM_EXT,       Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_024_ETF2_ENHANCEMENT_FACILITY )
FACILITY( 024_ETF2_ENHANCEMENT,     Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_025_STORE_CLOCK_FAST_FACILITY )
FACILITY( 025_STORE_CLOCK_FAST,     Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_026_PARSING_ENHANCE_FACILITY )
FACILITY( 026_PARSING_ENHANCE,      Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_027_MVCOS_FACILITY )
FACILITY( 027_MVCOS,                Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_028_TOD_CLOCK_STEER_FACILITY )
FACILITY( 028_TOD_CLOCK_STEER,      Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_030_ETF3_ENHANCEMENT_FACILITY )
FACILITY( 030_ETF3_ENHANCEMENT,     Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_031_EXTRACT_CPU_TIME_FACILITY )
FACILITY( 031_EXTRACT_CPU_TIME,     Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_032_CSS_FACILITY )
FACILITY( 032_CSSF,                 Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_033_CSS_FACILITY_2 )
FACILITY( 033_CSSF2,                Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_034_GEN_INST_EXTN_FACILITY )
FACILITY( 034_GEN_INST_EXTN,        Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_035_EXECUTE_EXTN_FACILITY )
FACILITY( 035_EXECUTE_EXTN,         Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_036_ENH_MONITOR_FACILITY )
FACILITY( 036_ENH_MONITOR,          Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_037_FP_EXTENSIONS_FACILITY )
//CILITY( 037_FP_EXTENSIONS,        ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_038_OP_CMPSC_FACILITY )
//CILITY( 038_OP_CMPSC,             ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_040_LOAD_PROG_PARAM_FACILITY )
FACILITY( 040_LOAD_PROG_PARAM,      Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_041_FPS_ENHANCEMENTS_FACILITY )

FACILITY( 041_FPS_ENHANCEMENTS,     ZARCH,      NONE,       ZARCH,      ALS23 )

#if defined( _FEATURE_041_DFP_ROUNDING_FACILITY )
FACILITY( 041_DFP_ROUNDING,         ZARCH,      NONE,       ZARCH,      ALS23 )
#endif

#if defined( _FEATURE_041_FPR_GR_TRANSFER_FACILITY )
FACILITY( 041_FPR_GR_TRANSFER,      ZARCH,      NONE,       ZARCH,      ALS23 )
#endif

#if defined( _FEATURE_041_FPS_SIGN_HANDLING_FACILITY )
FACILITY( 041_FPS_SIGN_HANDLING,    ZARCH,      NONE,       ZARCH,      ALS23 )
#endif

#if defined( _FEATURE_041_IEEE_EXCEPT_SIM_FACILITY )
FACILITY( 041_IEEE_EXCEPT_SIM,      ZARCH,      NONE,       ZARCH,      ALS23 )
#endif
#endif /* defined( _FEATURE_041_FPS_ENHANCEMENTS_FACILITY ) */

#if defined( _FEATURE_042_DECIMAL_FLOAT_FACILITY )
FACILITY( 042_DECIMAL_FLOAT,        Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_043_DFP_HPERF_FACILITY )
FACILITY( 043_DFP_HPERF,            Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_044_PFPO_FACILITY )
FACILITY( 044_PFPO,                 Z390,       NONE,       Z390,       ALS23 )
#endif

#if defined( _FEATURE_045_FAST_BCR_SERIAL_FACILITY )
FACILITY( 045_FAST_BCR_SERIAL,      Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_047_CMPSC_ENH_FACILITY )
FACILITY( 047_CMPSC_ENH,            ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_048_DFP_ZONE_CONV_FACILITY )
FACILITY( 048_DFP_ZONE_CONV,        ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_049_EXECUTION_HINT_FACILITY )
FACILITY( 049_EXECUTION_HINT,       ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_049_LOAD_AND_TRAP_FACILITY )
FACILITY( 049_LOAD_AND_TRAP,        ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_049_PROCESSOR_ASSIST_FACILITY )
FACILITY( 049_PROCESSOR_ASSIST,     ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_049_MISC_INSTR_EXT_FACILITY_1 )
FACILITY( 049_MISC_INSTR_EXT_1,     ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_050_CONSTR_TRANSACT_FACILITY )
//CILITY( 050_CONSTR_TRANSACT,      ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_051_LOCAL_TLB_CLEARING_FACILITY )
//CILITY( 051_LOCAL_TLB_CLEARING,   ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_052_INTERLOCKED_ACCESS_FACILITY_2 )
#if CAN_IAF2 != IAF2_ATOMICS_UNAVAILABLE
FACILITY( 052_INTERLOCKED_ACCESS_2, Z390,       NONE,       Z390,       ALS123 )
#endif
#endif

#if defined( _FEATURE_053_LOAD_STORE_ON_COND_FACILITY_2 )
//CILITY( 053_LOAD_STORE_ON_COND_2, ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_053_LOAD_ZERO_RIGHTMOST_FACILITY )
//CILITY( 053_LOAD_ZERO_RIGHTMOST,  ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_054_EE_CMPSC_FACILITY )
//CILITY( 054_EE_CMPSC,             ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_057_MSA_EXTENSION_FACILITY_5 )
//CILITY( 057_MSA_EXTENSION_5,      ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_058_MISC_INSTR_EXT_FACILITY_2 )
//CILITY( 058_MISC_INSTR_EXT_2,     ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_066_RES_REF_BITS_MULT_FACILITY )
FACILITY( 066_RES_REF_BITS_MULT,    Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_067_CPU_MEAS_COUNTER_FACILITY )
FACILITY( 067_CPU_MEAS_COUNTER,     Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY )
FACILITY( 068_CPU_MEAS_SAMPLNG,     Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
//CILITY( 073_TRANSACT_EXEC,        ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_074_STORE_HYPER_INFO_FACILITY )
//CILITY( 074_STORE_HYPER_INFO,     ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_075_ACC_EX_FS_INDIC_FACILITY )
FACILITY( 075_ACC_EX_FS_INDIC,      Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 )
FACILITY( 076_MSA_EXTENSION_3,      Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_077_MSA_EXTENSION_FACILITY_4 )
FACILITY( 077_MSA_EXTENSION_4,      Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_078_ENHANCED_DAT_FACILITY_2 )
//CILITY( 078_EDAT_2,               ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_080_DFP_PACK_CONV_FACILITY )
//CILITY( 080_DFP_PACK_CONV,        ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_129_ZVECTOR_FACILITY )
//CILITY( 129_ZVECTOR,              ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_130_INSTR_EXEC_PROT_FACILITY )
//CILITY( 130_INSTR_EXEC_PROT,      ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_131_SIDE_EFFECT_ACCESS_FACILITY )
//CILITY( 131_SIDE_EFFECT_ACCESS,   ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_133_GUARDED_STORAGE_FACILITY )
//CILITY( 133_GUARDED_STORAGE,      ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_134_ZVECTOR_PACK_DEC_FACILITY )
//CILITY( 134_ZVECTOR_PACK_DEC,     ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_135_ZVECTOR_ENH_FACILITY_1 )
//CILITY( 135_ZVECTOR_ENH_1,        ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_138_CONFIG_ZARCH_MODE_FACILITY )
//CILITY( 138_CONFIG_ZARCH_MODE,    ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_139_MULTIPLE_EPOCH_FACILITY )
//CILITY( 139_MULTIPLE_EPOCH,       ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_142_ST_CPU_COUNTER_MULT_FACILITY )
//CILITY( 142_ST_CPU_COUNTER_MULT,  ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_144_TEST_PEND_EXTERNAL_FACILITY )
//CILITY( 144_TEST_PEND_EXTERNAL,   ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_145_INS_REF_BITS_MULT_FACILITY )
//CILITY( 145_INS_REF_BITS_MULT,    ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_146_MSA_EXTENSION_FACILITY_8 )
//CILITY( 146_MSA_EXTENSION_8,      ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

#if defined( _FEATURE_168_ESA390_COMPAT_MODE_FACILITY )
//CILITY( 168_ESA390_COMPAT_MODE,   ZARCH,      NONE,       ZARCH,      ALS3 )
#endif

//------------------------------------------------------------------------------------
//                              Hercules Facilities                          
//------------------------------------------------------------------------------------
// The below facilities are HERCULES SPECIFIC and not part of the architecture
// but DO indicate the availability of actual facilities and are returned to the
// guest in the actual facilities list returned by the STFLE instruction.
//------------------------------------------------------------------------------------

//        Facility                      Default     Mandatory   Supported   Group

FACILITY( HERC_MOVE_INVERSE,            ALLM,       ZARCH,      ALLM,       ALSALL )

#if defined( _FEATURE_MSA_EXTENSION_FACILITY_1 )
FACILITY( HERC_MSA_EXTENSION_1,         Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_MSA_EXTENSION_FACILITY_2 )
FACILITY( HERC_MSA_EXTENSION_2,         Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_HERCULES_DIAGCALLS )
FACILITY( HERC_PROBSTATE_DIAGF08,       NONE,       NONE,       ALLM,       NONE )
FACILITY( HERC_SIGP_SETARCH_S370,       NONE,       NONE,       ALLM,       NONE )
#if defined( _FEATURE_HOST_RESOURCE_ACCESS_FACILITY )
FACILITY( HERC_HOST_RESOURCE_ACCESS,    NONE,       NONE,       ALLM,       NONE )
#endif
#endif

#if defined( _FEATURE_QEBSM )
FACILITY( HERC_QEBSM,                   Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_QDIO_THININT )
FACILITY( HERC_QDIO_THININT,            Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_QDIO_TDD )
FACILITY( HERC_QDIO_TDD,                NONE,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_SVS )
FACILITY( HERC_SVS,                     Z390,       NONE,       Z390,       ALS3 )
#endif

#if defined( _FEATURE_HYPERVISOR )
FACILITY( HERC_LOGICAL_PARTITION,       ALLM,       NONE,       ALLM,       ALSALL )
#endif

#if defined( _FEATURE_EMULATE_VM )
FACILITY( HERC_VIRTUAL_MACHINE,         NONE,       NONE,       ALLM,       NONE )
#endif

FACILITY( HERC_QDIO_ASSIST,             NONE,       NONE,       Z390,       ALS3 )

#if defined( _FEATURE_INTERVAL_TIMER )
FACILITY( HERC_INTERVAL_TIMER,          ALLM,       Z390,       ALLM,       ALSALL )
#endif

FACILITY( HERC_DETECT_PGMINTLOOP,       ALLM,       NONE,       ALLM,       ALSALL )

{ NULL, 0, 0, 0, 0, 0 }
};

/*-------------------------------------------------------------------*/
/*                          init_als                                 */
/*-------------------------------------------------------------------*/
void init_als( REGS* regs )
{
    int  i;
    for (i=0; i < STFL_HERC_BY_SIZE; i++)
        regs->facility_list[i] =
            sysblk.facility_list[ regs->arch_mode ][i];
}

/*-------------------------------------------------------------------*/
/*                         set_alslevel                              */
/*-------------------------------------------------------------------*/
static void set_alslevel( int alslevel )
{
    FACTAB*  ft;
    int      i, j;

    for(i = 0; i < STFL_HERC_BY_SIZE; i++)
        for(j = 0; j < GEN_MAXARCH; j++)
            sysblk.facility_list[j][i] = 0;

    for(ft = factab; ft->name; ft++)
    {
    int fbyte, fbit;

        fbyte = ft->bitno / 8;
        fbit = 0x80 >> (ft->bitno % 8);

        if(ft->alslevel & alslevel)
        {
#if defined(_370)
            if(ft->mode & S370)
                sysblk.facility_list[ARCH_370][fbyte] |= fbit;
#endif
#if defined(_390)
            if(ft->mode & ESA390)
                sysblk.facility_list[ARCH_390][fbyte] |= fbit;
#endif
#if defined(_900)
            if(ft->mode & ZARCH)
                sysblk.facility_list[ARCH_900][fbyte] |= fbit;
#endif
        }
    }
}

/*-------------------------------------------------------------------*/
/*                         get_archtab                               */
/*-------------------------------------------------------------------*/
static ARCHTAB* get_archtab( const char* name )
{
    ARCHTAB* at;

    for (at = archtab; at->name; at++)
    {
        if (strcasecmp( name, at->name ) == 0)
            return at;
    }
    return NULL;
}

/*-------------------------------------------------------------------*/
/*                          get_facname                              */
/*-------------------------------------------------------------------*/
static const char* get_facname( int bitno, const char** name )
{
    FACTAB*  ft;
    char work[8];

    for (ft = factab; ft->name; ft++)
    {
        if (ft->bitno == bitno)
        {
            *name = strdup( ft->name );
            return *name;
        }
    }

    snprintf( work, sizeof( work ), "bit%d", bitno );
    *name = strdup( work );
    return *name;
}

/*-------------------------------------------------------------------*/
/*                          get_factab                               */
/*-------------------------------------------------------------------*/
static FACTAB* get_factab( const char* name )
{
    FACTAB* ft;

    for (ft = factab; ft->name; ft++)
    {
        if (strcasecmp( ft->name, name ) == 0)
            return ft;
    }
    return NULL;
}

/*-------------------------------------------------------------------*/
/*                          set_archlvl                     (public) */
/*-------------------------------------------------------------------*/
BYTE set_archlvl( const char* name )
{
    ARCHTAB* at;

    if ((at = get_archtab( name )))
    {
        sysblk.arch_mode = at->archmode;
        set_alslevel( at->alslevel );
        return TRUE;
    }
    return FALSE;
}

/*-------------------------------------------------------------------*/
/*                        do_force_facbit                            */
/*-------------------------------------------------------------------*/
static void do_force_facbit( int bitno, BYTE enable, BYTE mode,
                             BYTE archmode, int archnum, const char* archname )
{
    const char*  endis  = enable ? "en" : "dis";

    int  fbyte  =         (bitno / 8);
    int  fbit   = 0x80 >> (bitno % 8);

    // If Hercules was not built with support for this architecture,
    // or the facility doesn't apply for this archmode, just return.

    if (archnum < 0 || !(archmode & mode))
        return;

    // Try to do what they want

    if (enable)
    {
        // If already enabled, nothing to do; return.
        if ((sysblk.facility_list[ archnum ][fbyte] & fbit))
            return;

        sysblk.facility_list[ archnum ][fbyte] |= fbit;
    }
    else // disable
    {
        // If already disabled, nothing to do; return.
        if (!(sysblk.facility_list[ archnum ][fbyte] & fbit))
            return;

        sysblk.facility_list[ archnum ][fbyte] &= ~fbit;
    }

    // Report what we did

    if (MLVL( VERBOSE ))
    {
        const char* name = NULL;
        name = get_facname( bitno, &name );

        // "Facility(%s) %sabled for archmode %s"
        WRMSG( HHC00898, "I", name, endis, archname );

        free( (void*) name );
    }
}

/*-------------------------------------------------------------------*/
/*                          force_facbit                             */
/*-------------------------------------------------------------------*/
static void force_facbit( int bitno, BYTE enable, BYTE mode )
{
#if defined( _370 )
    do_force_facbit( bitno, enable, mode, S370,   ARCH_370, _ARCH_370_NAME );
#endif
#if defined( _390 )
    do_force_facbit( bitno, enable, mode, ESA390, ARCH_390, _ARCH_390_NAME );
#endif
#if defined( _900 )
    do_force_facbit( bitno, enable, mode, ZARCH,  ARCH_900, _ARCH_900_NAME );
#endif
}

/*-------------------------------------------------------------------*/
/*                       do_set_facility                             */
/*-------------------------------------------------------------------*/
static void do_set_facility( FACTAB* ft, BYTE enable, BYTE mode,
                             BYTE archmode, int archnum, const char* archname )
{
    const char*  endis  = enable ? "en" : "dis";

    int  fbyte  =         (ft->bitno / 8);
    int  fbit   = 0x80 >> (ft->bitno % 8);

    // If Hercules was not built with support for this architecture,
    // or the facility doesn't apply for this archmode, just return.

    if (archnum < 0 || !(ft->supported & archmode & mode))
        return;

    // Try to do what they want

    if (enable)
    {
        // If already enabled, nothing to do; return.
        if ((sysblk.facility_list[ archnum ][fbyte] & fbit))
            return;

        sysblk.facility_list[ archnum][fbyte ] |= fbit;
    }
    else // disable
    {
        // Ignore attempts to disable a mandatory facility
        if (ft->fixed & archmode)
            return;

        // If already disabled, nothing to do; return.
        if (!(sysblk.facility_list[ archnum ][fbyte] & fbit))
            return;

        sysblk.facility_list[ archnum ][fbyte] &= ~fbit;
    }

    // Report what we did

    if (MLVL( VERBOSE ))
    {
        // "Facility(%s) %sabled for archmode %s"
        WRMSG( HHC00898, "I", ft->name, endis, archname );
    }
}

/*-------------------------------------------------------------------*/
/*                        set_facility                     (boolean) */
/*-------------------------------------------------------------------*/
static BYTE set_facility( FACTAB* ft, BYTE enable, BYTE mode )
{
    if (!(ft->supported & mode))
    {
        // "Facility(%s) not supported for specfied archmode"
        WRMSG( HHC00896, "E", ft->name );
        return FALSE;
    }

#if defined( _370 )
    do_set_facility( ft, enable, mode, S370,   ARCH_370, _ARCH_370_NAME );
#endif
#if defined( _390 )
    do_set_facility( ft, enable, mode, ESA390, ARCH_390, _ARCH_390_NAME );
#endif
#if defined( _900 )
    do_set_facility( ft, enable, mode, ZARCH,  ARCH_900, _ARCH_900_NAME );
#endif

    return TRUE;
}

/*-------------------------------------------------------------------*/
/*                        update_facility                  (boolean) */
/*-------------------------------------------------------------------*/
static BYTE update_facility( int argc, char* argv[] )
{
    FACTAB*     ft;
    ARCHTAB*    at;
    BYTE        enable;
    int         bitno;
    char        c;


    const BYTE  arch2als[] =
    {
      #if defined( _370 )
            S370
        #if defined( _390 ) || defined( _900 )
            ,
        #endif
      #endif
      #if defined( _390 )
            ESA390
        #if defined( _900 )
            ,
        #endif
      #endif
      #if defined( _900 )
            ZARCH
      #endif
    };


    BYTE als =
      #if defined( _370 )
        S370
        #if defined( _390 ) || defined( _900 )
          |
        #endif
      #endif
      #if defined( _390 )
        ESA390
        #if defined(_900)
          |
        #endif
      #endif
      #if defined( _900 )
        ZARCH
      #endif
    ;


    if      (CMD( argv[1], enable,  3 )) enable = TRUE;
    else if (CMD( argv[1], disable, 4 )) enable = FALSE;
    else                                 return FALSE;

    if (argc < 3)
    {
        // "Facility name not specified"
        WRMSG( HHC00892, "E" );
        return FALSE;
    }

    if (argc == 4)
    {
        if (!(at = get_archtab( argv[3] )))
        {
            // "Archmode %s is invalid"
            WRMSG( HHC00895, "E", argv[3] );
            return FALSE;
        }
        als = arch2als[ at->archmode ];
    }

    if ((ft = get_factab( argv[2] )))
    {
        set_facility( ft, enable, als );
    }
    else if (1
        && strncasecmp( "bit", argv[2], 3 ) == 0
        && isdigit( *( argv[2] + 3))
        && sscanf(     argv[2] + 3, "%d%c", &bitno, &c ) == 1
        && bitno >= 0
        && bitno <= STFL_HERC_LAST_BIT
    )
    {
        force_facbit( bitno, enable, als );
    }
    else
    {
        // "Facility(%s) does not exist"
        WRMSG( HHC00893, "E", argv[2] );
    }

    return TRUE;
}

/*-------------------------------------------------------------------*/
/*       archlvl_cmd  -  set architecture level set         (public) */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Usage:                                                            */
/*                                                                   */
/*   archlvl  S/370   | ALS0                                         */
/*            ESA/390 | ALS1                                         */
/*            ESAME   | ALS2                                         */
/*            Z/ARCH  | ALS3                                         */
/*                                                                   */
/*            ENABLE  | DISABLE  <facility>  [S/370|ESA/390|Z/ARCH]  */
/*                                                                   */
/*            QUERY              <facility> | ALL                    */
/*                                                                   */
/*-------------------------------------------------------------------*/
int archlvl_cmd( int argc, char* argv[], char* cmdline )
{
    BYTE  storage_reset  = FALSE;

    UNREFERENCED( cmdline );

    strupper( argv[0], argv[0] );

    // Display current value

    if (argc < 2)
    {
        // "%-14s: %s"
        WRMSG( HHC02203, "I", "ARCHLVL", get_arch_mode_string( NULL ));
        return 0;
    }

    // Too many arguments?

    if (argc > 4)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    // Query specific facility...

    if (CMD( argv[1], query, 1 ))
    {
        FACTAB*  tb;
        int      fcnt  = 0;
        int      fbyte, fbit;

        if (argc > 3)
        {
            // "Invalid command usage. Type 'help %s' for assistance."
            WRMSG( HHC02299, "E", argv[0] );
            return -1;
        }

        for (tb = factab; tb->name; tb++)
        {
            fbyte = tb->bitno / 8;
            fbit = 0x80 >> (tb->bitno % 8);

            if (0
                || argc < 3
                || CMD( argv[2], all, 3 )
                || strcasecmp( argv[2], tb->name ) == 0
            )
            {
                fcnt++;

                // "Facility( %-20s ) %sabled"
                WRMSG( HHC00890, "I", tb->name,
                    sysblk.facility_list[ sysblk.arch_mode ][ fbyte ] & fbit ?
                    "En" : "Dis" );
            }
        }

        if (!fcnt)
        {
            int   bitno;
            char  c;

            if (1
                && strncasecmp( "bit", argv[2], 3 ) == 0
                && isdigit( *( argv[2] + 3 ))
                && sscanf(     argv[2] + 3, "%d%c", &bitno, &c ) == 1
                && bitno >= 0
                && bitno <= STFL_HERC_LAST_BIT
            )
            {
                const char* name = NULL;

                fbyte = bitno / 8;
                fbit  = 0x80 >> (bitno % 8);

                name = get_facname( bitno, &name );

                // "Facility( %-20s ) %sabled"
                WRMSG( HHC00890, "I", name,
                    sysblk.facility_list[ sysblk.arch_mode ][ fbyte ] & fbit ?
                    "En" : "Dis" );

                free( (void*) name );
            }
            else
            {
                // "Facility(%s) does not exist"
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
    if (!set_archlvl( argv[1] ))
    {
        /* Then it must be enable/disable facility */
        if (!update_facility( argc, argv ))
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], "" );
            return -1;
        }
    }

    /* Update dummy regs with possibly new archmode */
    sysblk.dummyregs.arch_mode = sysblk.arch_mode;

    if (1
        && sysblk.arch_mode > ARCH_370
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
        system_reset( sysblk.pcpu, 0, sysblk.arch_mode );
        RELEASE_INTLOCK( NULL );
    }

    if (argc == 2 && MLVL( VERBOSE ))
    {
        // "%-14s set to %s"
        WRMSG( HHC02204, "I", "ARCHLVL", get_arch_mode_string( NULL ));

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
       if archmode is z/Arch. Else (ARCH_390) leave it alone.
       The user can override this automatic LPARNUM switching
       via their own subsequent LPARNUM stmt/cmd if such auto-
       matic behavior is not desired.
    */
    if (1
        && ARCH_370 == sysblk.arch_mode
        && om_basic != sysblk.operation_mode
    )
        panel_command( "-LPARNUM BASIC" );
    else if (1
        && ARCH_900 == sysblk.arch_mode
        && om_basic == sysblk.operation_mode
    )
    {
        panel_command( "-LPARNUM 1" );
        panel_command( "-CPUIDFMT 0" );
    }

    return 0;
}

#endif /*!defined(_GEN_ARCH)*/
