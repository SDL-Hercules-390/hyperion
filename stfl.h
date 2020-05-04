/* STFL.H       (C) Copyright Roger Bowler, 1994-2012                */
/*              (C) Copyright "Fish" (David B. Trout), 2018-2019     */
/*              Store Facility List (STFL) bit definitions           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _STFL_H_
#define _STFL_H_

/*-------------------------------------------------------------------*/
/*               Store Facility List definitions                     */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The below are all known facility bits defined by IBM as of the    */
/* latest z/Architecture Pinciples of Operation.  They correspond    */
/* to similarly named 'FEATURE_XXX" #defines listed in featall.h.    */
/*                                                                   */
/* Their names (minus the 'STFL_' prefix) are used by the FT macro   */
/* macro and FACILITY_ENABLED macro.  The FT macro is used within    */
/* facility.c to define the entries in the FACTAB facilities table   */
/* that indicates which individual facilities are available in each  */
/* architecture.                                                     */
/*                                                                   */
/* Always #define a STFL_xxxx and corresponding FEATURE_xxxx define  */
/* for every known/documented facility regardless of whether or not  */
/* the facility is currently supported/enabled or not.  Also define  */
/* multiple names for a given facility bit if the bit is documented  */
/* as pertaining to more than one facility.  This make maintaining   */
/* the facility.c FACTAB facility table easier and allows using the  */
/* actual facility name in your FACILITY_ENABLED macros throughout   */
/* Hercules code making the code easier to understand and maintain   */
/* (see e.g. bits 41, 45, 49 and 53).                                */
/*                                                                   */
/*-------------------------------------------------------------------*/

#define STFL_000_N3_INSTR              0    /* Instructions marked N3
                                               are installed             */
#define STFL_001_ZARCH_INSTALLED       1    /* z/Arch mode is available on
                                               this processor            */
#define STFL_002_ZARCH_ACTIVE          2    /* z/Architecture architecural
                                               mode active. When bit 2 and
                                               168 are both zero, ESA/390
                                               mode is active. When bit 2
                                               is zero and bit 168 is one,
                                               ESA/390-compatability mode
                                               is active.                */
#define STFL_003_DAT_ENHANCE_1         3    /* DAT-Enhancement Facility 1
                                               is installed.             */
#define STFL_004_IDTE_SC_SEGTAB        4    /* IDTE selective clearing
                                               when segtab invalidated. Bit
                                               3 is one if bit 4 is one. */
#define STFL_005_IDTE_SC_REGTAB        5    /* IDTE selective clearing
                                               when regtab invalidated. Bit
                                               3 and 4 one when bit 5 is */
#define STFL_006_ASN_LX_REUSE          6    /* ASN-and-LX-reuse facility
                                               is installed              */
#define STFL_007_STFL_EXTENDED         7    /* Store facility list
                                               extended is installed     */
#define STFL_008_EDAT_1                8    /* Enhanced-DAT facility
                                               is installed              */
#define STFL_009_SENSE_RUN_STATUS      9    /* Sense running status
                                               facility is installed     */
#define STFL_010_CONDITIONAL_SSKE     10    /* Conditional SSKE facility
                                               is installed              */
#define STFL_011_CONFIG_TOPOLOGY      11    /* STSI-enhancement for
                                               configuration topology    */
#define STFL_012_IBM_INTERNAL         12    /* IBM internal use          */

#define STFL_013_IPTE_RANGE           13    /* IPTE-Range facility
                                               installed                 */
#define STFL_014_NONQ_KEY_SET         14    /* Nonquiescing Key-Setting
                                               Facility installed        */
#define STFL_015_IBM_INTERNAL         15    /* IBM internal use          */

#define STFL_016_EXT_TRANSL_2         16    /* Extended translation
                                               facility 2 is installed   */
#define STFL_017_MSA                  17    /* Message security assist
                                               feature is installed      */
#define STFL_018_LONG_DISPL_INST      18    /* Long displacement facility
                                               is installed              */
#define STFL_019_LONG_DISPL_HPERF     19    /* Long displacement facility
                                               has high performance. Bit 18
                                               is one if bit 19 is one.  */
#define STFL_020_HFP_MULT_ADD_SUB     20    /* HFP multiply-add/subtract
                                               facility is installed     */
#define STFL_021_EXTENDED_IMMED       21    /* Extended immediate
                                               facility is installed     */
#define STFL_022_EXT_TRANSL_3         22    /* Extended translation
                                               facility 3 is installed   */
#define STFL_023_HFP_UNNORM_EXT       23    /* HFP unnormalized extension
                                               facility is installed     */
#define STFL_024_ETF2_ENHANCEMENT     24    /* Extended translation
                                               facility 2 enhancement    */
#define STFL_025_STORE_CLOCK_FAST     25    /* Store clock fast
                                               enhancement installed     */
#define STFL_026_PARSING_ENHANCE      26    /* Parsing-Enhancement
                                               facility is installed     */
#define STFL_027_MVCOS                27    /* MVCOS instruction
                                               is installed              */
#define STFL_028_TOD_CLOCK_STEER      28    /* TOD clock steering
                                               facility is installed     */
#define STFL_029_UNASSIGNED           29    /* Unassigned                */

#define STFL_030_ETF3_ENHANCEMENT     30    /* Extended translation
                                               facility 3 enhancement    */
#define STFL_031_EXTRACT_CPU_TIME     31    /* Extract CPU time facility
                                               is installed              */
#define STFL_032_CSSF                 32    /* Compare-and-Swap-and-Store
                                               facility is installed     */
#define STFL_033_CSSF2                33    /* Compare-and-Swap-and-Store
                                               facility 2 is installed   */
#define STFL_034_GEN_INST_EXTN        34    /* General-Instr-Extn
                                               facility is installed     */
#define STFL_035_EXECUTE_EXTN         35    /* Execute-Extensions
                                               facility is installed     */
#define STFL_036_ENH_MONITOR          36    /* Enhanced-Monitor
                                               facility installed        */
#define STFL_037_FP_EXTENSION         37    /* Floating-point extension
                                               facility installed. When bit
                                               37 is one, so is bit 42.  */
#define STFL_038_OP_CMPSC             38    /* Order-preserving-compression
                                               facility is installed     */
#define STFL_039_IBM_INTERNAL         39    /* IBM internal use          */

#define STFL_040_LOAD_PROG_PARAM      40    /* Load-Program-Parameter
                                               facility installed z/Arch */
#define STFL_041_FPS_ENHANCEMENT      41    /* Floating-point-support-
                                               enhancement (DFP-rounding,
                                               FPR-GR-transfer, FPS-sign-
                                               handling and IEEE-exception-
                                               simulator) installed      */
#define STFL_041_DFP_ROUNDING         41    /* Ibid.                     */
#define STFL_041_FPR_GR_TRANSFER      41    /* Ibid.                     */
#define STFL_041_FPS_SIGN_HANDLING    41    /* Ibid.                     */
#define STFL_041_IEEE_EXCEPT_SIM      41    /* Ibid.                     */

#define STFL_042_DFP                  42    /* Decimal-floating-point
                                               (DFP) facility installed. */
#define STFL_043_DFP_HPERF            43    /* DFP has high performance.
                                               Bit 42 is one if bit 43 is*/
#define STFL_044_PFPO                 44    /* PFPO instruction installed*/

#define STFL_045_DISTINCT_OPERANDS    45    /* Distinct-operands, fast-BCR-
                                               serialization, high-word,
                                               interlocked-access1, load-
                                               or-store-on-condition and
                                               population-count facilities
                                               are installed in z/Arch.  */
#define STFL_045_FAST_BCR_SERIAL      45    /* Ibid.                     */
#define STFL_045_HIGH_WORD            45    /* Ibid.                     */
#define STFL_045_INTERLOCKED_ACCESS_1 45    /* Ibid.                     */
#define STFL_045_LOAD_STORE_ON_COND_1 45    /* Ibid.                     */
#define STFL_045_POPULATION_COUNT     45    /* Ibid.                     */

#define STFL_046_IBM_INTERNAL         46    /* IBM internal use          */

#define STFL_047_CMPSC_ENH            47    /* CMPSC-enhancement
                                               Facility installed        */
#define STFL_048_DFP_ZONE_CONV        48    /* Decimal-floating-point-
                                               zoned-conversion facility
                                               installed. Bit 42 is also
                                               one when bit 48 is one.   */
#define STFL_049_EXECUTION_HINT       49    /* Execution-hint, load-and-
                                               trap, processor-assist and
                                               miscellaneous-instruction-
                                               extensions-1 installed    */
#define STFL_049_LOAD_AND_TRAP        49    /* Ibid.                     */
#define STFL_049_PROCESSOR_ASSIST     49    /* Ibid.                     */
#define STFL_049_MISC_INSTR_EXT_1     49    /* Ibid.                     */

#define STFL_050_CONSTR_TRANSACT      50    /* Constrained-transactional-
                                               execution facility. Bit only
                                               meaningful if bit 73 one. */
#define STFL_051_LOCAL_TLB_CLEARING   51    /* Local-TLB-clearing        */

#define STFL_052_INTERLOCKED_ACCESS_2 52    /* Interlocked-access-2      */

#define STFL_053_LOAD_STORE_ON_COND_2 53    /* Load/store-on-condition-2,
                                               load-zero-rightmost-byte  */
#define STFL_053_LOAD_ZERO_RIGHTMOST  53    /* Ibid.                     */

#define STFL_054_EE_CMPSC             54    /* Entropy-encoding compress */

#define STFL_055_IBM_INTERNAL         55    /* IBM internal use          */

#define STFL_056_UNASSIGNED           56    /* Unassigned                */

#define STFL_057_MSA_EXTENSION_5      57    /* Message-security-assist-
                                               extension-5               */
#define STFL_058_MISC_INSTR_EXT_2     58    /* Miscellaneous-instructions-
                                               extension-2               */
#define STFL_059_IBM_INTERNAL         59    /* IBM internal use          */

#define STFL_060_IBM_INTERNAL         60    /* IBM internal use          */

#define STFL_061_MISC_INSTR_EXT_3     61    /* Miscellaneous-instruction-
                                               extensions facility 3 is
                                               installed. Bit 45 is also
                                               one when bit 61 is one.   */

#define STFL_062_IBM_INTERNAL         62    /* IBM internal use          */

#define STFL_063_IBM_INTERNAL         63    /* IBM internal use          */

#define STFL_064_IBM_INTERNAL         64    /* IBM internal use          */

#define STFL_065_IBM_INTERNAL         65    /* IBM internal use          */

#define STFL_066_RES_REF_BITS_MULT    66    /* Reset-Reference-Bits-
                                               Multiple Fac installed    */
#define STFL_067_CPU_MEAS_COUNTER     67    /* CPU-measurement counter
                                               facility installed z/Arch */
#define STFL_068_CPU_MEAS_SAMPLNG     68    /* CPU-measurement sampling
                                               facility installed z/Arch.
                                               Bit 68 requires bit 40.   */

#define STFL_069_IBM_INTERNAL         69    /* IBM internal use          */

#define STFL_070_IBM_INTERNAL         70    /* IBM internal use          */

#define STFL_071_IBM_INTERNAL         71    /* IBM internal use          */

#define STFL_072_IBM_INTERNAL         72    /* IBM internal use          */

#define STFL_073_TRANSACT_EXEC        73    /* Transactional-execution. Bit
                                               49 is one when bit 73 one */
#define STFL_074_STORE_HYPER_INFO     74    /* Store-hypervisor-info     */

#define STFL_075_ACC_EX_FS_INDIC      75    /* Access-exception fetch/store
                                               indication facility       */
#define STFL_076_MSA_EXTENSION_3      76    /* Message Security Assist
                                               Extension 3 installed     */
#define STFL_077_MSA_EXTENSION_4      77    /* Message Security Assist
                                               Extension 4 installed     */
#define STFL_078_EDAT_2               78    /* Enhanced-DAT-2            */

#define STFL_079_UNASSIGNED           79    /* Unassigned                */

#define STFL_080_DFP_PACK_CONV        80    /* Decimal-floating-point
                                               packed-conversion facility
                                               installed. Bit 42 is also
                                               one when bit 80 is one.   */
//efine STFL_nnn_UNASSIGNED       81-127    /* Unassigned or IBM internal*/

#define STFL_128_IBM_INTERNAL        128    /* IBM internal use          */

#define STFL_129_ZVECTOR             129    /* z/Arch Vector facility    */

#define STFL_130_INSTR_EXEC_PROT     130    /* Instruction-execution-
                                               protection facility       */
#define STFL_131_SIDE_EFFECT_ACCESS  131    /* Side-effect-access        */

#define STFL_132_UNASSIGNED          132    /* Unassigned                */

#define STFL_133_GUARDED_STORAGE     133    /* Guarded-storage facility  */

#define STFL_134_ZVECTOR_PACK_DEC    134    /* Vector-packed-decimal. When
                                               bit 134 is one, bit 129 is
                                               also one.                 */
#define STFL_135_ZVECTOR_ENH_1       135    /* Vector-enhancements-1. When
                                               bit 135 is one, bit 129 is
                                               also one.                 */
#define STFL_136_UNASSIGNED          136    /* Unassigned                */

#define STFL_137_UNASSIGNED          137    /* Unassigned                */

#define STFL_138_CONFIG_ZARCH_MODE   138    /* Configuration-z/Architecture-
                                               architectural-mode        */
#define STFL_139_MULTIPLE_EPOCH      139    /* Multiple-epoch facility. If
                                               bit 139 is one, bits 25 and
                                               28 are also one.          */
#define STFL_140_IBM_INTERNAL        140    /* IBM internal use          */

#define STFL_141_IBM_INTERNAL        141    /* IBM internal use          */

#define STFL_142_ST_CPU_COUNTER_MULT 142    /* Store-CPU-counter-multiple
                                               facility is installed. Bit
                                               67 is one when bit 142 is.*/
#define STFL_143_UNASSIGNED          143    /* Unassigned                */

#define STFL_144_TEST_PEND_EXTERNAL  144    /* Test-pending-external-
                                               interruption facility     */
#define STFL_145_INS_REF_BITS_MULT   145    /* Insert-reference-bits-
                                               multiple facility         */
#define STFL_146_MSA_EXTENSION_8     146    /* Message-security-assist-
                                               extension-8. Bit 76 is one
                                               when bit 146 is one.      */
#define STFL_147_IBM_INTERNAL        147    /* IBM internal use          */

#define STFL_148_VECTOR_ENH_2        148    /* Vector-enhancements fac-
                                               ility 2 installed.  Bits
                                               129 and 135 are also one
                                               when bit 148 is one.      */

#define STFL_149_MOVEPAGE_SETKEY     149    /* Move-page-and-set-key fac-
                                               ility installed. Bit 14 is
                                               also on if bit 149 is on. */

#define STFL_150_UNASSIGNED          150    /* Unassigned                */

#define STFL_151_DEFLATE_CONV        151    /* DEFLATE-conversion facil-
                                               ity is installed.         */

#define STFL_152_VECT_PACKDEC_ENH    152    /* Vector-packed-decimal-en-
                                               hancement installed. Bits
                                               129 and 134 are also one
                                               when bit 152 is one.      */

#define STFL_153_UNASSIGNED          153    /* Unassigned                */

#define STFL_154_UNASSIGNED          154    /* Unassigned                */

#define STFL_155_MSA_EXTENSION_9     155    /* Message-security-assist-
                                               extension-9 installed.
                                               Bits 76 and 77 are one
                                               when bit 155 is one.      */

#define STFL_156_IBM_INTERNAL        156    /* IBM internal use          */

//efine STFL_nnn_UNASSIGNED      157-167    /* Unassigned or IBM internal*/

#define STFL_168_ESA390_COMPAT_MODE  168    /* ESA/390-compatibility-mode.
                                               Bit 168 can only be 1 when
                                               bit 2 is zero.            */

#define STFL_IBM_LAST_BIT            168    /* Last defined IBM facility */

#define STFL_IBM_BY_SIZE        (ROUND_UP( STFL_IBM_LAST_BIT, 8 ) / 8)
#define STFL_IBM_DW_SIZE        (ROUND_UP( STFL_IBM_BY_SIZE, sizeof( DW )) / sizeof( DW ))

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
/*                                                                   */
/* When IBM defines new facilities the only thing you need to do is  */
/* define the new bits and then adjust the above STFL_IBM_LAST_BIT   */
/* value as appropriate. Nothing else needs to be done.              */
/*-------------------------------------------------------------------*/

#define STFL_HERC_FIRST_BIT     (STFL_IBM_DW_SIZE * sizeof( DW ) * 8)

#define STFL_HERC_370_EXTENSION          ( STFL_HERC_FIRST_BIT  +   0 )
#define STFL_HERC_DETECT_PGMINTLOOP      ( STFL_HERC_FIRST_BIT  +   1 )
#define STFL_HERC_HOST_RESOURCE_ACCESS   ( STFL_HERC_FIRST_BIT  +   2 )
#define STFL_HERC_INTERVAL_TIMER         ( STFL_HERC_FIRST_BIT  +   3 )
#define STFL_HERC_LOGICAL_PARTITION      ( STFL_HERC_FIRST_BIT  +   4 )
#define STFL_HERC_MOVE_INVERSE           ( STFL_HERC_FIRST_BIT  +   5 )
#define STFL_HERC_MSA_EXTENSION_1        ( STFL_HERC_FIRST_BIT  +   6 )
#define STFL_HERC_MSA_EXTENSION_2        ( STFL_HERC_FIRST_BIT  +   7 )
#define STFL_HERC_PROBSTATE_DIAGF08      ( STFL_HERC_FIRST_BIT  +   8 )
#define STFL_HERC_QDIO_ASSIST            ( STFL_HERC_FIRST_BIT  +   9 )
#define STFL_HERC_QDIO_TDD               ( STFL_HERC_FIRST_BIT  +  10 )
#define STFL_HERC_QDIO_THININT           ( STFL_HERC_FIRST_BIT  +  11 )
#define STFL_HERC_QEBSM                  ( STFL_HERC_FIRST_BIT  +  12 )
#define STFL_HERC_SIGP_SETARCH_S370      ( STFL_HERC_FIRST_BIT  +  13 )
#define STFL_HERC_SVS                    ( STFL_HERC_FIRST_BIT  +  14 )
#define STFL_HERC_VIRTUAL_MACHINE        ( STFL_HERC_FIRST_BIT  +  15 )
#define STFL_HERC_TCPIP_EXTENSION        ( STFL_HERC_FIRST_BIT  +  16 )
#define STFL_HERC_TCPIP_PROB_STATE       ( STFL_HERC_FIRST_BIT  +  17 )

#define STFL_HERC_LAST_BIT               ( STFL_HERC_FIRST_BIT  +  17 )

#define STFL_HERC_BY_SIZE       (ROUND_UP( STFL_HERC_LAST_BIT, 8 ) / 8)
#define STFL_HERC_DW_SIZE       (ROUND_UP( STFL_HERC_BY_SIZE, sizeof( DW )) / sizeof( DW ))

#endif // _STFL_H_
