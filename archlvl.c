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
    const char*  name;             /* Archlvl Name                   */
    const int    archmode;         /* Architecture Mode              */
    const int    alslevel;         /* Architecture Level             */

#define ALS0     0x01              /* S/370                          */
#define ALS1     0x02              /* ESA/390                        */
#define ALS2     0x04              /* Z/ARCH                         */
#define ALS3     0x08              /* ARCHLVL 3                      */
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
    const char*  name;             /* Facility Name                  */
    const BYTE   bitno;            /* Bit number                     */
    const BYTE   mode;             /* Mode indicator                 */

#define S370     0x01              /* S/370 feature                  */
#define ESA390   0x02              /* ESA/390 feature                */
#define ZARCH    0x04              /* ESAME feature                  */
#define Z390     (ZARCH|ESA390)    /* Exists in ESAME only, but
                                      is also indicated in ESA390    */

    const BYTE   fixed;            /* Mandatory for                  */

#define NONE     0x00

    const BYTE   supported;        /* Supported in                   */
    const BYTE   alslevel;         /* alslevel grouping              */
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
static FACTAB factab[] =
{
/*        Facility          Default       Mandatory  Supported      Group        */
#if defined(_FEATURE_ESAME_N3_ESA390)
FACILITY( N3,               ESA390|ZARCH, NONE,      ESA390|ZARCH,  ALS1|ALS2|ALS3 )
#endif

#if defined(_FEATURE_ESAME)
FACILITY( ESAME_INSTALLED,  ESA390|ZARCH, NONE,      ESA390|ZARCH,       ALS2|ALS3 )
FACILITY( ESAME_ACTIVE,     ZARCH,        ZARCH,     ZARCH,              ALS2|ALS3 )
#endif

#if defined(_FEATURE_DAT_ENHANCEMENT)
FACILITY( IDTE_INSTALLED,   Z390,         NONE,      Z390,               ALS2|ALS3 )
FACILITY( IDTE_SC_SEGTAB,   0, /*ZARCH*/  NONE,      0, /*ZARCH*/        ALS2|ALS3 )
FACILITY( IDTE_SC_REGTAB,   0, /*ZARCH*/  NONE,      0, /*ZARCH*/        ALS2|ALS3 )
#endif

#if defined(_FEATURE_ASN_AND_LX_REUSE)
FACILITY( ASN_LX_REUSE,     0, /*Z390*/   NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_STORE_FACILITY_LIST_EXTENDED)
FACILITY( STFL_EXTENDED,    ESA390|ZARCH, NONE,      ESA390|ZARCH,       ALS2|ALS3 )
#endif

#if defined(_FEATURE_ENHANCED_DAT_FACILITY)
FACILITY( ENHANCED_DAT,     Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_SENSE_RUNNING_STATUS)
FACILITY( SENSE_RUN_STATUS, Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_CONDITIONAL_SSKE)
FACILITY( CONDITIONAL_SSKE, Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_CONFIGURATION_TOPOLOGY_FACILITY)
FACILITY( CONFIG_TOPOLOGY,  Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_IPTE_RANGE_FACILITY)
FACILITY( IPTE_RANGE,       Z390,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_NONQUIESCING_KEY_SETTING_FACILITY)
FACILITY( NONQ_KEY_SET,     Z390,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
FACILITY( TRAN_FAC2,        ESA390|ZARCH, NONE,      ESA390|ZARCH,       ALS2|ALS3 )
#endif

#if defined(_FEATURE_MESSAGE_SECURITY_ASSIST)
FACILITY( MSG_SECURITY,     ESA390|ZARCH, NONE,      ESA390|ZARCH,       ALS2|ALS3 )
#endif

#if defined(_FEATURE_LONG_DISPLACEMENT)
FACILITY( LONG_DISPL_INST,  Z390,         NONE,      Z390,               ALS2|ALS3 )
FACILITY( LONG_DISPL_HPERF, ZARCH,        NONE,      ZARCH,              ALS2|ALS3 )
#endif

#if defined(_FEATURE_HFP_MULTIPLY_ADD_SUBTRACT)
FACILITY( HFP_MULT_ADD_SUB, ESA390|ZARCH, NONE,      ESA390|ZARCH,       ALS2|ALS3 )
#endif

#if defined(_FEATURE_EXTENDED_IMMEDIATE)
FACILITY( EXTENDED_IMMED,   Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_EXTENDED_TRANSLATION_FACILITY_3)
FACILITY( TRAN_FAC3,        Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_HFP_UNNORMALIZED_EXTENSION)
FACILITY( HFP_UNNORM_EXT,   Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_ETF2_ENHANCEMENT)
FACILITY( ETF2_ENHANCEMENT, ESA390|ZARCH, NONE,      ESA390|ZARCH,       ALS2|ALS3 )
#endif

#if defined(_FEATURE_STORE_CLOCK_FAST)
FACILITY( STORE_CLOCK_FAST, Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

FACILITY( PARSING_ENHANCE,  Z390,         NONE,      Z390,               ALS2|ALS3 )
#if defined(_FEATURE_MOVE_WITH_OPTIONAL_SPECIFICATIONS)
FACILITY( MVCOS,            Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_TOD_CLOCK_STEERING)
FACILITY( TOD_CLOCK_STEER,  Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_ETF3_ENHANCEMENT)
FACILITY( ETF3_ENHANCEMENT, Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_EXTRACT_CPU_TIME)
FACILITY( EXTRACT_CPU_TIME, Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_COMPARE_AND_SWAP_AND_STORE)
FACILITY( CSSF,             Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_COMPARE_AND_SWAP_AND_STORE_FACILITY_2)
FACILITY( CSSF2,            Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY)
FACILITY( GEN_INST_EXTN,    Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_EXECUTE_EXTENSIONS_FACILITY)
FACILITY( EXECUTE_EXTN,     Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_ENHANCED_MONITOR_FACILITY)
FACILITY( ENH_MONITOR,      Z390,         NONE,      Z390,                    ALS3 )
#endif

//FACILITY( FP_EXTENSION,     ZARCH,        NONE,      ZARCH,                 ALS3 )

#if defined(_FEATURE_LOAD_PROGRAM_PARAMETER_FACILITY)
FACILITY( LOAD_PROG_PARAM,  Z390,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_FPS_ENHANCEMENT)
FACILITY( FPS_ENHANCEMENT,  Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_DECIMAL_FLOATING_POINT)
FACILITY( DECIMAL_FLOAT,    Z390,         NONE,      Z390,               ALS2|ALS3 )
FACILITY( DFP_HPERF,        Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_PFPO)
FACILITY( PFPO,             Z390,         NONE,      Z390,               ALS2|ALS3 )
#endif

#if defined(_FEATURE_FAST_BCR_SERIALIZATION_FACILITY)
FACILITY( FAST_BCR_SERIAL,  Z390,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_CMPSC_ENHANCEMENT_FACILITY)
FACILITY( CMPSC_ENH,        ZARCH,        NONE,      ZARCH,                   ALS3 )
#endif

#if defined(_FEATURE_RESET_REFERENCE_BITS_MULTIPLE_FACILITY)
FACILITY( RES_REF_BITS_MUL, Z390,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_CPU_MEASUREMENT_COUNTER_FACILITY)
FACILITY( CPU_MEAS_COUNTER, Z390,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_CPU_MEASUREMENT_SAMPLING_FACILITY)
FACILITY( CPU_MEAS_SAMPLNG, Z390,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_ACCESS_EXCEPTION_FETCH_STORE_INDICATION)
FACILITY( ACC_EX_FS_INDIC,  Z390,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_3)
FACILITY( MSA_EXTENSION_3,  Z390,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_4)
FACILITY( MSA_EXTENSION_4,  Z390,         NONE,      Z390,                    ALS3 )
#endif

//------------------------------------------------------------------------------------
// Note that this facility is available in ESA mode too (SIE)

#if CAN_IAF2 != IAF2_ATOMICS_UNAVAILABLE
FACILITY( INTERLOCKED_ACCESS_2, Z390,     NONE,      Z390,          ALS1|ALS2|ALS3 )
#endif

//------------------------------------------------------------------------------------
// The Following entries are not part of STFL(E) but do indicate the availability of facilities

FACILITY( MOVE_INVERSE,     S370|ESA390|ZARCH, ZARCH, S370|ESA390|ZARCH, ALS0|ALS1|ALS2|ALS3 )
#if defined(_FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_1)
FACILITY( MSA_EXTENSION_1,  Z390,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_2)
FACILITY( MSA_EXTENSION_2,  Z390,         NONE,      Z390,                    ALS3 )
#endif

//------------------------------------------------------------------------------------
#if defined(_FEATURE_HERCULES_DIAGCALLS)
FACILITY( PROBSTATE_DIAGF08,NONE,         NONE,      S370|ESA390|ZARCH,       NONE )
FACILITY( SIGP_SETARCH_S370,NONE,         NONE,      S370|ESA390|ZARCH,       NONE )

#if defined(_FEATURE_HOST_RESOURCE_ACCESS_FACILITY)
FACILITY( HOST_RESOURCE_ACCESS,NONE,      NONE,      S370|ESA390|ZARCH,       NONE )
#endif
#endif /*defined(_FEATURE_HERCULES_DIAGCALLS)*/
//------------------------------------------------------------------------------------

#if defined(_FEATURE_QEBSM)
FACILITY( QEBSM,            Z390,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_QDIO_THININT)
FACILITY( QDIO_THININT,     Z390,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_QDIO_TDD)
FACILITY( QDIO_TDD,         NONE,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_SVS)
FACILITY( SVS,              Z390,         NONE,      Z390,                    ALS3 )
#endif

#if defined(_FEATURE_HYPERVISOR)
FACILITY( LOGICAL_PARTITION,S370|ESA390|ZARCH, NONE, S370|ESA390|ZARCH, ALS0|ALS1|ALS2|ALS3 )
#endif

#if defined(_FEATURE_EMULATE_VM)
FACILITY( VIRTUAL_MACHINE,  NONE,         NONE,      S370|ESA390|ZARCH,       NONE )
#endif

// #if defined(_FEATURE_QDIO_ASSIST)
FACILITY( QDIO_ASSIST,      NONE,         NONE,      Z390,                    ALS3 )
// #endif

#if defined(_FEATURE_INTERVAL_TIMER)
FACILITY( INTERVAL_TIMER,   S370|ESA390|ZARCH, ESA390|ZARCH, S370|ESA390|ZARCH, ALS0|ALS1|ALS2|ALS3 )
#endif

FACILITY( DETECT_PGMINTLOOP,S370|ESA390|ZARCH, NONE, S370|ESA390|ZARCH, ALS0|ALS1|ALS2|ALS3 )

{ NULL, 0, 0, 0, 0, 0 }
};

/*-------------------------------------------------------------------*/
/*                          init_als                                 */
/*-------------------------------------------------------------------*/
void init_als( REGS* regs )
{
    int  i;
    for (i=0; i < STFL_HBYTESIZE; i++)
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

    for(i = 0; i < STFL_HBYTESIZE; i++)
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
static const char* get_facname( int bitno )
{
    FACTAB*      ft;
    static char  name[8];

    for (ft = factab; ft->name; ft++)
    {
        if (ft->bitno == bitno)
            return ft->name;
    }

    snprintf( name, sizeof( name ), "bit%d", bitno );
    return name;
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
        // "Facility(%s) %sabled for archmode %s"
        WRMSG( HHC00898, "I", get_facname( bitno ), endis, archname );
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
        && bitno <= STFL_HMAX
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
                && bitno <= STFL_HMAX
            )
            {
                fbyte = bitno / 8;
                fbit  = 0x80 >> (bitno % 8);

                // "Facility( %-20s ) %sabled"
                WRMSG( HHC00890, "I", get_facname( bitno ),
                    sysblk.facility_list[ sysblk.arch_mode ][ fbyte ] & fbit ?
                    "En" : "Dis" );
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
