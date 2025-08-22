/* ZVECTOR.H    (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) Copyright Roger Bowler, 1999-2012                */
/*              z/Arch Vector Operations                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef __ZVECTOR_H_
#define __ZVECTOR_H_

/*----------------------------------------------------------------------------*/
/* See zvector.c for the following Vector instructions.                       */
/*----------------------------------------------------------------------------*/
/* E700 VLEB   - VECTOR LOAD ELEMENT (8)                              [VRX]   */
/* E701 VLEH   - VECTOR LOAD ELEMENT (16)                             [VRX]   */
/* E702 VLEG   - VECTOR LOAD ELEMENT (64)                             [VRX]   */
/* E703 VLEF   - VECTOR LOAD ELEMENT (32)                             [VRX]   */
/* E704 VLLEZ  - VECTOR LOAD LOGICAL ELEMENT AND ZERO                 [VRX]   */
/* E705 VLREP  - VECTOR LOAD AND REPLICATE                            [VRX]   */
/* E706 VL     - VECTOR LOAD                                          [VRX]   */
/* E707 VLBB   - VECTOR LOAD TO BLOCK BOUNDARY                        [VRX]   */
/* E708 VSTEB  - VECTOR STORE ELEMENT (8)                             [VRX]   */
/* E709 VSTEH  - VECTOR STORE ELEMENT (16)                            [VRX]   */
/* E70A VSTEG  - VECTOR STORE ELEMENT (64)                            [VRX]   */
/* E70B VSTEF  - VECTOR STORE ELEMENT (32)                            [VRX]   */
/* E70E VST    - VECTOR STORE                                         [VRX]   */
/* E712 VGEG   - VECTOR GATHER ELEMENT (64)                           [VRV]   */
/* E713 VGEF   - VECTOR GATHER ELEMENT (32)                           [VRV]   */
/* E71A VSCEG  - VECTOR SCATTER ELEMENT (64)                          [VRV]   */
/* E71B VSCEF  - VECTOR SCATTER ELEMENT (32)                          [VRV]   */
/* E721 VLGV   - VECTOR LOAD GR FROM VR ELEMENT                       [VRS-c] */
/* E722 VLVG   - VECTOR LOAD VR ELEMENT FROM GR                       [VRS-b] */
/* E727 LCBB   - LOAD COUNT TO BLOCK BOUNDARY                         [RXE]   */
/* E730 VESL   - VECTOR ELEMENT SHIFT LEFT                            [VRS-a] */
/* E733 VERLL  - VECTOR ELEMENT ROTATE LEFT LOGICAL                   [VRS-a] */
/* E736 VLM    - VECTOR LOAD MULTIPLE                                 [VRS-a] */
/* E737 VLL    - VECTOR LOAD WITH LENGTH                              [VRS-b] */
/* E738 VESRL  - VECTOR ELEMENT SHIFT RIGHT LOGICAL                   [VRS-a] */
/* E73A VESRA  - VECTOR ELEMENT SHIFT RIGHT ARITHMETIC                [VRS-a] */
/* E73E VSTM   - VECTOR STORE MULTIPLE                                [VRS-a] */
/* E73F VSTL   - VECTOR STORE WITH LENGTH                             [VRS-b] */
/* E740 VLEIB  - VECTOR LOAD ELEMENT IMMEDIATE (8)                    [VRI-a] */
/* E741 VLEIH  - VECTOR LOAD ELEMENT IMMEDIATE (16)                   [VRI-a] */
/* E742 VLEIG  - VECTOR LOAD ELEMENT IMMEDIATE (64)                   [VRI-a] */
/* E743 VLEIF  - VECTOR LOAD ELEMENT IMMEDIATE (32)                   [VRI-a] */
/* E744 VGBM   - VECTOR GENERATE BYTE MASK                            [VRI-a] */
/* E745 VREPI  - VECTOR REPLICATE IMMEDIATE                           [VRI-a] */
/* E746 VGM    - VECTOR GENERATE MASK                                 [VRI-b] */
/* E74D VREP   - VECTOR REPLICATE                                     [VRI-c] */
/* E750 VPOPCT - VECTOR POPULATION COUNT                              [VRR-a] */
/* E752 VCTZ   - VECTOR COUNT TRAILING ZEROS                          [VRR-a] */
/* E753 VCLZ   - VECTOR COUNT LEADING ZEROS                           [VRR-a] */
/* E754 VGEM   - VECTOR GENERATE ELEMENT MASKS                        [VRR-a] */
/* E756 VLR    - VECTOR LOAD VECTOR                                   [VRR-a] */
/* E75C VISTR  - VECTOR ISOLATE STRING                                [VRR-a] */
/* E75F VSEG   - VECTOR SIGN EXTEND TO DOUBLEWORD                     [VRR-a] */
/* E760 VMRL   - VECTOR MERGE LOW                                     [VRR-c] */
/* E761 VMRH   - VECTOR MERGE HIGH                                    [VRR-c] */
/* E762 VLVGP  - VECTOR LOAD VR FROM GRS DISJOINT                     [VRR-f] */
/* E764 VSUM   - VECTOR SUM ACROSS WORD                               [VRR-c] */
/* E765 VSUMG  - VECTOR SUM ACROSS DOUBLEWORD                         [VRR-c] */
/* E766 VCKSM  - VECTOR CHECKSUM                                      [VRR-c] */
/* E767 VSUMQ  - VECTOR SUM ACROSS QUADWORD                           [VRR-c] */
/* E768 VN     - VECTOR AND                                           [VRR-c] */
/* E769 VNC    - VECTOR AND WITH COMPLEMENT                           [VRR-c] */
/* E76A VO     - VECTOR OR                                            [VRR-c] */
/* E76B VNO    - VECTOR NOR                                           [VRR-c] */
/* E76C VNX    - VECTOR NOT EXCLUSIVE OR                              [VRR-c] */
/* E76D VX     - VECTOR EXCLUSIVE OR                                  [VRR-c] */
/* E76E VNN    - VECTOR NAND                                          [VRR-c] */
/* E76F VOC    - VECTOR OR WITH COMPLEMENT                            [VRR-c] */
/* E770 VESLV  - VECTOR ELEMENT SHIFT LEFT VECTOR                     [VRR-c] */
/* E772 VERIM  - VECTOR ELEMENT ROTATE AND INSERT UNDER MASK          [VRI-d] */
/* E773 VERLLV - VECTOR ELEMENT ROTATE LEFT LOGICAL VECTOR            [VRR-c] */
/* E774 VSL    - VECTOR SHIFT LEFT                                    [VRR-c] */
/* E775 VSLB   - VECTOR SHIFT LEFT BY BYTE                            [VRR-c] */
/* E777 VSLDB  - VECTOR SHIFT LEFT DOUBLE BY BYTE                     [VRI-d] */
/* E778 VESRLV - VECTOR ELEMENT SHIFT RIGHT LOGICAL VECTOR            [VRR-c] */
/* E77A VESRAV - VECTOR ELEMENT SHIFT RIGHT ARITHMETIC VECTOR         [VRR-c] */
/* E77C VSRL   - VECTOR SHIFT RIGHT LOGICAL                           [VRR-c] */
/* E77D VSRLB  - VECTOR SHIFT RIGHT LOGICAL BY BYTE                   [VRR-c] */
/* E77E VSRA   - VECTOR SHIFT RIGHT ARITHMETIC                        [VRR-c] */
/* E77F VSRAB  - VECTOR SHIFT RIGHT ARITHMETIC BY BYTE                [VRR-c] */
/* E780 VFEE   - VECTOR FIND ELEMENT EQUAL                            [VRR-b] */
/* E781 VFENE  - VECTOR FIND ELEMENT NOT EQUAL                        [VRR-b] */
/* E782 VFAE   - VECTOR FIND ANY ELEMENT EQUAL                        [VRR-b] */
/* E784 VPDI   - VECTOR PERMUTE DOUBLEWORD IMMEDIATE                  [VRR-c] */
/* E785 VBPERM - VECTOR BIT PERMUTE                                   [VRR-c] */
/* E786 VSLD   - VECTOR SHIFT LEFT DOUBLE BY BIT                      [VRI-d] */
/* E787 VSRD   - VECTOR SHIFT RIGHT DOUBLE BY BIT                     [VRI-d] */
/* E788 VEVAL  - VECTOR EVALUATE                                      [VRI-k] */
/* E789 VBLEND - VECTOR BLEND                                         [VRR-d] */
/* E78A VSTRC  - VECTOR STRING RANGE COMPARE                          [VRR-d] */
/* E78B VSTRS  - VECTOR STRING SEARCH                                 [VRR-d] */
/* E78C VPERM  - VECTOR PERMUTE                                       [VRR-e] */
/* E78D VSEL   - VECTOR SELECT                                        [VRR-e] */
/* E794 VPK    - VECTOR PACK                                          [VRR-c] */
/* E795 VPKLS  - VECTOR PACK LOGICAL SATURATE                         [VRR-b] */
/* E797 VPKS   - VECTOR PACK SATURATE                                 [VRR-b] */
/* E7A1 VMLH   - VECTOR MULTIPLY LOGICAL HIGH                         [VRR-c] */
/* E7A2 VML    - VECTOR MULTIPLY LOW                                  [VRR-c] */
/* E7A3 VMH    - VECTOR MULTIPLY HIGH                                 [VRR-c] */
/* E7A4 VMLE   - VECTOR MULTIPLY LOGICAL EVEN                         [VRR-c] */
/* E7A5 VMLO   - VECTOR MULTIPLY LOGICAL ODD                          [VRR-c] */
/* E7A6 VME    - VECTOR MULTIPLY EVEN                                 [VRR-c] */
/* E7A7 VMO    - VECTOR MULTIPLY ODD                                  [VRR-c] */
/* E7A9 VMALH  - VECTOR MULTIPLY AND ADD LOGICAL HIGH                 [VRR-d] */
/* E7AA VMAL   - VECTOR MULTIPLY AND ADD LOW                          [VRR-d] */
/* E7AB VMAH   - VECTOR MULTIPLY AND ADD HIGH                         [VRR-d] */
/* E7AC VMALE  - VECTOR MULTIPLY AND ADD LOGICAL EVEN                 [VRR-d] */
/* E7AD VMALO  - VECTOR MULTIPLY AND ADD LOGICAL ODD                  [VRR-d] */
/* E7AE VMAE   - VECTOR MULTIPLY AND ADD EVEN                         [VRR-d] */
/* E7AF VMAO   - VECTOR MULTIPLY AND ADD ODD                          [VRR-d] */
/* E7B0 VDL    - VECTOR DIVIDE LOGICAL                                [VRR-c] */
/* E7B1 VRL    - VECTOR REMAINDER LOGICAL                             [VRR-c] */
/* E7B2 VD     - VECTOR DIVIDE                                        [VRR-c] */
/* E7B3 VR     - VECTOR REMAINDER                                     [VRR-c] */
/* E7B4 VGFM   - VECTOR GALOIS FIELD MULTIPLY SUM                     [VRR-c] */
/* E7B8 VMSL   - VECTOR MULTIPLY SUM LOGICAL                          [VRR-d] */
/* E7B9 VACCC  - VECTOR ADD WITH CARRY COMPUTE CARRY                  [VRR-d] */
/* E7BB VAC    - VECTOR ADD WITH CARRY                                [VRR-d] */
/* E7BC VGFMA  - VECTOR GALOIS FIELD MULTIPLY SUM AND ACCUMULATE      [VRR-d] */
/* E7BD VSBCBI - VECTOR SUBTRACT WITH BORROW COMPUTE BORROW INDICATION  [VRR-d] */
/* E7BF VSBI   - VECTOR SUBTRACT WITH BORROW INDICATION               [VRR-d] */
/* E7D4 VUPLL  - VECTOR UNPACK LOGICAL LOW                            [VRR-a] */
/* E7D5 VUPLH  - VECTOR UNPACK LOGICAL HIGH                           [VRR-a] */
/* E7D6 VUPL   - VECTOR UNPACK LOW                                    [VRR-a] */
/* E7D7 VUPH   - VECTOR UNPACK HIGH                                   [VRR-a] */
/* E7D8 VTM    - VECTOR TEST UNDER MASK                               [VRR-a] */
/* E7D9 VECL   - VECTOR ELEMENT COMPARE LOGICAL                       [VRR-a] */
/* E7DB VEC    - VECTOR ELEMENT COMPARE                               [VRR-a] */
/* E7DE VLC    - VECTOR LOAD COMPLEMENT                               [VRR-a] */
/* E7DF VLP    - VECTOR LOAD POSITIVE                                 [VRR-a] */
/* E7F0 VAVGL  - VECTOR AVERAGE LOGICAL                               [VRR-c] */
/* E7F1 VACC   - VECTOR ADD COMPUTE CARRY                             [VRR-c] */
/* E7F2 VAVG   - VECTOR AVERAGE                                       [VRR-c] */
/* E7F3 VA     - VECTOR ADD                                           [VRR-c] */
/* E7F5 VSCBI  - VECTOR SUBTRACT COMPUTE BORROW INDICATION            [VRR-c] */
/* E7F7 VS     - VECTOR SUBTRACT                                      [VRR-c] */
/* E7F8 VCEQ   - VECTOR COMPARE EQUAL                                 [VRR-b] */
/* E7F9 VCHL   - VECTOR COMPARE HIGH LOGICAL                          [VRR-b] */
/* E7FB VCH    - VECTOR COMPARE HIGH                                  [VRR-b] */
/* E7FC VMNL   - VECTOR MINIMUM LOGICAL                               [VRR-c] */
/* E7FD VMXL   - VECTOR MAXIMUM LOGICAL                               [VRR-c] */
/* E7FE VMN    - VECTOR MINIMUM                                       [VRR-c] */
/* E7FF VMX    - VECTOR MAXIMUM                                       [VRR-c] */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* See zvector2.c for the following Vector instructions.                      */
/*----------------------------------------------------------------------------*/
/* E601 VLEBRH  - VECTOR LOAD BYTE REVERSED ELEMENT (16)              [VRX]   */
/* E602 VLEBRG  - VECTOR LOAD BYTE REVERSED ELEMENT (64)              [VRX]   */
/* E603 VLEBRF  - VECTOR LOAD BYTE REVERSED ELEMENT (32)              [VRX]   */
/* E604 VLLEBRZ - VECTOR LOAD BYTE REVERSED ELEMENT AND ZERO          [VRX]   */
/* E605 VLBRREP - VECTOR LOAD BYTE REVERSED ELEMENT AND REPLICATE     [VRX]   */
/* E606 VLBR    - VECTOR LOAD BYTE REVERSED ELEMENTS                  [VRX]   */
/* E607 VLER    - VECTOR LOAD ELEMENTS REVERSED                       [VRX]   */
/* E609 VSTEBRH - VECTOR STORE BYTE REVERSED ELEMENT (16)             [VRX]   */
/* E60A VSTEBRG - VECTOR STORE BYTE REVERSED ELEMENT (64)             [VRX]   */
/* E60B VSTEBRF - VECTOR STORE BYTE REVERSED ELEMENT (32)             [VRX]   */
/* E60E VSTBR   - VECTOR STORE BYTE REVERSED ELEMENTS                 [VRX]   */
/* E60F VSTER   - VECTOR STORE ELEMENTS REVERSED                      [VRX]   */
/* E634 VPKZ    - VECTOR PACK ZONED                                   [VSI]   */
/* E635 VLRL    - VECTOR LOAD RIGHTMOST WITH LENGTH                   [VSI]   */
/* E637 VLRLR   - VECTOR LOAD RIGHTMOST WITH LENGTH (reg)             [VRS-d] */
/* E63C VUPKZ   - VECTOR UNPACK ZONED                                 [VSI]   */
/* E63D VSTRL   - VECTOR STORE RIGHTMOST WITH LENGTH                  [VSI]   */
/* E63F VSTRLR  - VECTOR STORE RIGHTMOST WITH LENGTH (reg)            [VRS-d] */
/* E649 VLIP    - VECTOR LOAD IMMEDIATE DECIMAL                       [VRI-h] */
/* E64A VCVDQ   - VECTOR CONVERT TO DECIMAL (128)                     [VRI-j] */
/* E64E VCVBQ   - VECTOR CONVERT TO BINARY (128)                      [VRR-k] */
/* E650 VCVB    - VECTOR CONVERT TO BINARY (32)                       [VRR-i] */
/* E651 VCLZDP  - VECTOR COUNT LEADING ZERO DIGITS                    [VRR-k] */
/* E652 VCVBG   - VECTOR CONVERT TO BINARY (64)                       [VRR-i] */
/* E654 VUPKZH  - VECTOR UNPACK ZONED HIGH                            [VRR-k] */
/* E658 VCVD    - VECTOR CONVERT TO DECIMAL (32)                      [VRI-i] */
/* E659 VSRP    - VECTOR SHIFT AND ROUND DECIMAL                      [VRi-g] */
/* E65A VCVDG   - VECTOR CONVERT TO DECIMAL (64)                      [VRI-i] */
/* E65B VPSOP   - VECTOR PERFORM SIGN OPERATION DECIMAL               [VRI-g] */
/* E65C VUPKZL  - VECTOR UNPACK ZONED LOW                             [VRR-k] */
/* E65F VTP     - VECTOR TEST DECIMAL                                 [VRR-g] */
/* E670 VPKZR   - VECTOR PACK ZONED REGISTER                          [VRI-f] */
/* E671 VAP     - VECTOR ADD DECIMAL                                  [VRI-f] */
/* E672 VSRPR   - VECTOR SHIFT AND ROUND DECIMAL REGISTER             [VRI-f] */
/* E673 VSP     - VECTOR SUBTRACT DECIMAL                             [VRI-f] */
/* E674 VSCHP   - DECIMAL SCALE AND CONVERT TO HFP                    [VRR-b] */
/* E677 VCP     - VECTOR COMPARE DECIMAL                              [VRR-h] */
/* E678 VMP     - VECTOR MULTIPLY DECIMAL                             [VRI-f] */
/* E679 VMSP    - VECTOR MULTIPLY AND SHIFT DECIMAL                   [VRI-f] */
/* E67A VDP     - VECTOR DIVIDE DECIMAL                               [VRI-f] */
/* E67B VRP     - VECTOR REMAINDER DECIMAL                            [VRI-f] */
/* E67C VSCSHP  - DECIMAL SCALE AND CONVERT AND SPLIT TO HFP          [VRR-b] */
/* E67D VCSPH   - VECTOR CONVERT HFP TO SCALED DECIMAL                [VRR-j] */
/* E67E VSDP    - VECTOR SHIFT AND DIVIDE DECIMAL                     [VRI-f] */
/* E67F VTZ     - VECTOR TEST ZONED                                   [VRI-l] */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* See zvector3.c for the following Vector Floating-Point instructions.       */
/*----------------------------------------------------------------------------*/
/* E74A VFTCI  - VECTOR FP TEST DATA CLASS IMMEDIATE                  [VRI-e] */
/* E78E VFMS   - VECTOR FP MULTIPLY AND SUBTRACT                      [VRR-e] */
/* E78F VFMA   - VECTOR FP MULTIPLY AND ADD                           [VRR-e] */
/* E79E VFNMS  - VECTOR FP NEGATIVE MULTIPLY AND SUBTRACT             [VRR-e] */
/* E79F VFNMA  - VECTOR FP NEGATIVE MULTIPLY AND ADD                  [VRR-e] */
/* E7C0 VCLFP  - VECTOR FP CONVERT TO LOGICAL (SHORT BFP TO 32)       [VRR-a] */
/* E7C0 VCLGD  - VECTOR FP CONVERT TO LOGICAL (LONG BFP TO 64)        [VRR-a] */
/* E7C1 VCFPL  - VECTOR FP CONVERT FROM LOGICAL (32 TO SHORT BFP)     [VRR-a] */
/* E7C1 VCDLG  - VECTOR FP CONVERT FROM LOGICAL (64 TO LONG BFP)      [VRR-a] */
/* E7C2 VCSFP  - VECTOR FP CONVERT TO FIXED (SHORT BFP TO 32)         [VRR-a] */
/* E7C2 VCGD   - VECTOR FP CONVERT TO FIXED (LONG BFP TO 64)          [VRR-a] */
/* E7C3 VCFPS  - VECTOR FP CONVERT FROM FIXED (32 TO SHORT BFP)       [VRR-a] */
/* E7C3 VCDG   - VECTOR FP CONVERT FROM FIXED (64 TO LONG BFP)        [VRR-a] */
/* E7C4 VFLL   - VECTOR FP LOAD LENGTHENED                            [VRR-a] */
/* E7C5 VFLR   - VECTOR FP LOAD ROUNDED                               [VRR-a] */
/* E7C7 VFI    - VECTOR LOAD FP INTEGER                               [VRR-a] */
/* E7CA WFK    - VECTOR FP COMPARE AND SIGNAL SCALAR                  [VRR-a] */
/* E7CB WFC    - VECTOR FP COMPARE SCALAR                             [VRR-a] */
/* E7CC VFPSO  - VECTOR FP PERFORM SIGN OPERATION                     [VRR-a] */
/* E7CE VFSQ   - VECTOR FP SQUARE ROOT                                [VRR-a] */
/* E7E2 VFS    - VECTOR FP SUBTRACT                                   [VRR-c] */
/* E7E3 VFA    - VECTOR FP ADD                                        [VRR-c] */
/* E7E5 VFD    - VECTOR FP DIVIDE                                     [VRR-c] */
/* E7E7 VFM    - VECTOR FP MULTIPLY                                   [VRR-c] */
/* E7E8 VFCE   - VECTOR FP COMPARE EQUAL                              [VRR-c] */
/* E7EA VFCHE  - VECTOR FP COMPARE HIGH OR EQUAL                      [VRR-c] */
/* E7EB VFCH   - VECTOR FP COMPARE HIGH                               [VRR-c] */
/* E7EE VFMIN  - VECTOR FP MINIMUM                                    [VRR-c] */
/* E7EF VFMAX  - VECTOR FP MAXIMUM                                    [VRR-c] */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* See nnpa.c for the following Specialized-Function-Assist instructions.     */
/*----------------------------------------------------------------------------*/
/* E655 VCNF   - VECTOR FP CONVERT TO NNP                             [VRR-a] */
/* E656 VCLFNH - VECTOR FP CONVERT AND LENGTHEN FROM NNP HIGH         [VRR_a] */
/* E65D VCFN   - VECTOR FP CONVERT FROM NNP                           [VRR-a] */
/* E65E VCLFNL - VECTOR FP CONVERT AND LENGTHEN FROM NNP LOW          [VRR-a] */
/* E675 VCRNF  - VECTOR FP CONVERT AND ROUND TO NNP                   [VRR-c] */
/*----------------------------------------------------------------------------*/

/*===================================================================*/
/* 128 BIT TYPES                                                     */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* are the compiler 128 bit types available?                         */
/*-------------------------------------------------------------------*/
#if defined( __SIZEOF_INT128__ )
    #define _USE_128_
#endif

/*-------------------------------------------------------------------*/
/* U128                                                              */
/*-------------------------------------------------------------------*/
typedef union {
        QW   Q;
#if defined( _USE_128_ )
    unsigned __int128 u_128;
#endif
        U64  u_64[2];
        U32  u_32[4];
        U16  u_16[8];
        U8   u_8[16];

#if defined( _USE_128_ )
    __int128 s_128;
#endif
        S64  s_64[2];
        S32  s_32[4];
        S16  s_16[8];
        S8   s_8[16];

#if defined( FEATURE_V128_SSE )
        __m128i V;      // intrinsic type vector
#endif

}  U128  ;

/*===================================================================*/
/* Function Prototypes                                               */
/*===================================================================*/
static inline void U128_logmsg(const char * msg, U128 u);

static inline U128 U128_zero( );
static inline U128 U128_one( );
static inline U128 U128_minus_one( );
static inline bool U128_isZero( U128 a );

static inline U128 U128_add( U128 a, U128 b );
static inline U128 U128_sub( U128 a, U128 b );
static inline U128 U128_divrem(U128 dividend, U128 divisor, U128 *remainder );
static inline U128 U128_div(U128 a, U128 b );
static inline U128 U128_rem( U128 a, U128 b );
static inline void U128_mul( U128 a, U128 b, U128* hi128, U128* lo128 );
static inline U128 U128_mul_64 (U64 aa, U64 bb );
static inline U128 U128_mul_32( U128 a, U32 b );

static inline U128 U128_shl( U128 a, U32 shift );
static inline U128 U128_shrl( U128 a, U32 shift );
static inline U128 U128_shra( U128 a, U32 shift );

static inline U128 U128_S64( U64 a );
static inline U128 U128_U64( U64 a );

static inline int U128_cmp( U128 a, U128 b );
static inline int S128_cmp( U128 a, U128 b );

static inline U128 S128_add( U128 a, U128 b );
static inline U128 S128_sub( U128 a, U128 b );
static inline U128 S128_div(U128 a, U128 b );
static inline U128 S128_rem( U128 a, U128 b );
static inline U128 S128_mul_64 (U64 a, U64 b );
static inline void S128_mul( U128 a, U128 b, U128* hi128, U128* lo128 );
static inline U128 S128_neg( U128 a );
static inline bool S128_isNeg( U128 a );

static inline void U256_add_U128( U128 *ahi, U128 *alo, U128 b, U128 *chi, U128 *clo );
static inline void S256_add_S128( U128 *ahi, U128 *alo, U128 b, U128 *chi, U128 *clo );

static inline U64 gf_mul_32( U32 m1, U32 m2 );
static inline void gf_mul_64( U64 m1, U64 m2, U64* accu128h, U64* accu128l );

static inline U128 U128_and( U128 a, U128 b );
static inline U128 U128_or( U128 a, U128 b );
static inline U128 U128_xor( U128 a, U128 b );
static inline U128 U128_nand( U128 a, U128 b );
static inline U128 U128_nor( U128 a, U128 b );
static inline U128 U128_nxor( U128 a, U128 b );
static inline U128 U128_not( U128 a );
static inline bool U128_biton( U128 a, U128 b );

/*-------------------------------------------------------------------*/
/* Debug helper for U128                                             */
/*                                                                   */
/* Input:                                                            */
/*      msg     pointer to logmsg context string                     */
/*      u       U128 number                                          */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline void U128_logmsg(const char * msg, U128 u)
{
    printf("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", msg, u.Q.D.H.D, u.Q.D.L.D);
}

/*===================================================================*/
/* U128 Constants (zero, one)                                        */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* U128_zero: return (U128) 0                                        */
/*-------------------------------------------------------------------*/
static inline U128 U128_zero( )
{
    U128 temp;

    temp.Q.D.H.D = 0;
    temp.Q.D.L.D = 0;
    return temp;
}

/*-------------------------------------------------------------------*/
/* U128_one: return (U128) 1                                         */
/*-------------------------------------------------------------------*/
static inline U128 U128_one( )
{
    U128 temp;

    temp.Q.D.H.D = 0;
    temp.Q.D.L.D = 1;
    return temp;
}

/*-------------------------------------------------------------------*/
/* U128_minus_one: return (U128) -1                                  */
/*-------------------------------------------------------------------*/
static inline U128 U128_minus_one( )
{
    U128 temp;

    temp.Q.D.H.D = -1;
    temp.Q.D.L.D = -1;
    return temp;
}

/*-------------------------------------------------------------------*/
/* U128_isZero: if (U128) a == 0; return true                        */
/*-------------------------------------------------------------------*/
static inline bool U128_isZero( U128 a )
{
    if ( a.Q.D.H.D == 0  && a.Q.D.L.D == 0 )
        return true;

    return false;
}

/*===================================================================*/
/* U128 Arithmetic (add, sub, div, mul, rem)                         */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* U128 Add: return a + b                                            */
/*-------------------------------------------------------------------*/
static inline U128 U128_add( U128 a, U128 b)
{
#if defined( _USE_128_ )
    U128 temp;                           /* temp (return) value      */

    temp.u_128 =  a.u_128 + b.u_128;
    return temp;

#else
    U128 temp;                           /* temp (return) value      */

    temp.Q.D.H.D =  a.Q.D.H.D + b.Q.D.H.D;
    temp.Q.D.L.D =  a.Q.D.L.D + b.Q.D.L.D;
    if (temp.Q.D.L.D < b.Q.D.L.D) temp.Q.D.H.D++;
    return temp;
#endif
}

/*-------------------------------------------------------------------*/
/* U128 Subtract: return a - b                                       */
/*-------------------------------------------------------------------*/
static inline U128 U128_sub( U128 a, U128 b)
{
#if defined( _USE_128_ )
    U128 temp;                           /* temp (return) value      */

    temp.u_128 =  a.u_128 - b.u_128;
    return temp;

#else
    U128 temp;                           /* temp (return) value      */

    temp.Q.D.H.D =  a.Q.D.H.D - b.Q.D.H.D;
    if (a.Q.D.L.D < b.Q.D.L.D) temp.Q.D.H.D--;
    temp.Q.D.L.D =  a.Q.D.L.D - b.Q.D.L.D;

    return temp;
#endif
}

static inline U128 U128_divrem(U128 dividend, U128 divisor, U128 *remainder)
/* long-division, most-significant bit first */
{
    U128 rem, quo;
    int i;

    if(U128_cmp(dividend, divisor) < 0) {            /* trivial small case  */
        if(remainder) *remainder = dividend;
        return U128_zero();
    }
    if(U128_cmp(dividend, divisor) == 0) {           /* trivial equal case  */
        if(remainder) *remainder = U128_zero();
        return U128_one();
    }

    rem = U128_zero();
    quo = U128_zero();

    /* step through all 128 bits, MSB→LSB */
    for (i = 127; i >= 0; --i) {
        /* left-shift remainder, pull next dividend bit in ----------------*/
        rem = U128_shl(rem, 1);
        rem.Q.D.L.D |= (i >= 64)
                ? (dividend.Q.D.H.D >> (i-64)) & 1ULL
                : (dividend.Q.D.L.D >>  i    ) & 1ULL;

        /* compare & subtract --------------------------------------------*/
        if (U128_cmp(rem, divisor) >= 0) {
            rem = U128_sub(rem, divisor);

            /* set bit i of quotient */
            if (i >= 64)
                quo.Q.D.H.D |= 1ULL << (i-64);
            else
                quo.Q.D.L.D |= 1ULL <<  i;
        }
    }

    if (remainder) *remainder = rem;
    return quo;
}

/*-------------------------------------------------------------------*/
/* U128 Divide: return  a /  b                                       */
/*-------------------------------------------------------------------*/
static inline U128 U128_div(U128 a, U128 b)
{
#if defined( _USE_128_ )
    U128 temp;                           /* temp (return) value      */

    temp.u_128 =  a.u_128 / b.u_128;
    return temp;

#else
    U128 quo, rem;

    quo = U128_divrem( a, b, &rem);
    return quo;

#endif
}

/*-------------------------------------------------------------------*/
/* U128 remainder: return  a %  b                                    */
/*-------------------------------------------------------------------*/
static inline U128 U128_rem( U128 a, U128 b)
{
#if defined( _USE_128_)
    U128 temp;                           /* temp (return) value      */

    temp.u_128 =  a.u_128 % b.u_128;
    return temp;

#else
    U128 quo, rem;

    quo = U128_divrem( a, b, &rem);
    return rem;
#endif
}

/*-------------------------------------------------------------------*/
/* U128_mul: 128-bit multiply -> 256-bit result                      */
/*      return:  hi128 : high 128 bits                               */
/*               lo128 : low  128 bits                               */
/*-------------------------------------------------------------------*/
static inline void U128_mul( U128 a, U128 b, U128* hi128, U128* lo128)
{
    U128 temp, temp1, temp2, temp3, temp4;
    U128 acc128hi, acc128mid, acc128lo;
    U64  carry;

    /* initialize temp accumulators result */
    acc128hi.Q.D.H.D = 0;
    acc128hi.Q.D.L.D = 0;

    acc128mid.Q.D.H.D = 0;
    acc128mid.Q.D.L.D = 0;

    acc128lo.Q.D.H.D = 0;
    acc128lo.Q.D.L.D = 0;

    /*  a or b == 0? */
    if ( ( a.Q.D.H.D  == 0 && a.Q.D.L.D  == 0 ) || ( b.Q.D.H.D  == 0 && b.Q.D.L.D  == 0 ) )
    {
       *hi128 = acc128hi;
       *lo128 = acc128lo;
       return;
    }

    /* intermediate results */
    temp1 = U128_mul_64 ( a.Q.D.L.D, b.Q.D.L.D);
    temp2 = U128_mul_64 ( a.Q.D.H.D, b.Q.D.L.D);
    temp3 = U128_mul_64 ( a.Q.D.L.D, b.Q.D.H.D);
    temp4 = U128_mul_64 ( a.Q.D.H.D, b.Q.D.H.D);

    // U128_logmsg("   temp1: ", temp1 );
    // U128_logmsg("   temp2: ", temp2 );
    // U128_logmsg("   temp3: ", temp3 );
    // U128_logmsg("   temp4: ", temp4 );

    /* middle 128-bits */
    acc128mid.Q.D.L.D = temp1.Q.D.H.D;

    carry = 0;
    acc128mid = U128_add( acc128mid, temp2);
    if ( U128_cmp(acc128mid, temp1) == -1 ) carry++;

    acc128mid = U128_add( acc128mid, temp3);
    if ( U128_cmp(acc128mid, temp2) == -1 ) carry++;

    temp.Q.D.L.D = 0;
    temp.Q.D.H.D = temp4.Q.D.L.D;
    acc128mid = U128_add( acc128mid, temp);
    if ( U128_cmp(acc128mid, temp) == -1 ) carry++;

    /* repackage 64-bit parts of 256-bit result */
    acc128lo.Q.D.L.D = temp1.Q.D.L.D;
    acc128lo.Q.D.H.D = acc128mid.Q.D.L.D;

    acc128hi.Q.D.L.D = acc128mid.Q.D.H.D;
    acc128hi.Q.D.H.D = temp4.Q.D.H.D + carry;

    *hi128 = acc128hi;
    *lo128 = acc128lo;
    return;
}

/*-------------------------------------------------------------------*/
/* U128: U64 * U64 Multiply: return a * b (overflow ignored)         */
/*                                                                   */
/* Very simple, standard approach to arithmetic multiply             */
/*                                                                   */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline U128 U128_mul_64 (U64 aa, U64 bb)
{
#if defined( _USE_128_)
    U128 temp;                           /* temp (return) value      */

    temp.u_128 =  (unsigned __int128) aa * bb;
    return temp;

#else
    DW a;                                /* arg 'aa' as DW            */
    DW b;                                /* arg 'bb' as DW            */
    DW t64;                              /* temp                      */
    U128 r;                              /* U128 multiply result      */
    U128 t128;                           /* temp                      */

    /* initialize result */
    r.Q.D.H.D = 0UL;
    r.Q.D.L.D = 0UL;

    /* zero check */
    if (aa == 0 || bb == 0) return r;

    /* arguments as DWs */
    a.D = aa;
    b.D = bb;

    /* a low 32 x b low 32 */
    if ( a.F.L.F != 0 && b.F.L.F!= 0 )
    {
        r.Q.D.L.D = (U64) a.F.L.F * (U64) b.F.L.F;
    }

    /* a high 32 x b low 32 */
    if ( a.F.H.F != 0 && b.F.L.F!= 0 )
    {
        t64.D =  (U64) a.F.H.F * (U64) b.F.L.F;
        t128.Q.D.H.D = 0UL;
        t128.Q.D.L.D = 0UL;
        t128.Q.D.H.F.L.F = t64.F.H.F;
        t128.Q.D.L.F.H.F = t64.F.L.F;
        r = U128_add( r, t128 );
    }

    /* a low 32 x b high 32 */
    if ( a.F.L.F != 0 && b.F.H.F!= 0 )
    {
        t64.D =  (U64) a.F.L.F * (U64) b.F.H.F;
        t128.Q.D.H.D = 0UL;
        t128.Q.D.L.D = 0UL;
        t128.Q.D.H.F.L.F = t64.F.H.F;
        t128.Q.D.L.F.H.F = t64.F.L.F;
        r = U128_add( r, t128 );
    }

    /* a high 32 x b high 32 */
    if ( a.F.H.F != 0 && b.F.H.F!= 0 )
    {
        t64.D =  (U64) a.F.H.F * (U64) b.F.H.F;
        t128.Q.D.H.D = 0UL;
        t128.Q.D.L.D = 0UL;
        t128.Q.D.H.F.L.F = t64.F.L.F;
        t128.Q.D.H.F.H.F = t64.F.H.F;
        r = U128_add( r, t128 );
    }

    return r;
#endif

}

/*-------------------------------------------------------------------*/
/* U128 * U32 Multiply: return a * b (overflow ignored)              */
/*                                                                   */
/* Very simple, standard approach to arithmetic multiply             */
/*                                                                   */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline U128 U128_mul_32( U128 a, U32 b)
{
#if defined( _USE_128_ )
    U128 temp;                           /* temp (return) value      */

    temp.u_128 =  a.u_128 * b;
    return temp;

#else
    U128 r;                           /* return value                */
    U64 t;                            /* temp                        */


    /* initialize result */
    r.Q.D.H.D = 0UL;
    r.Q.D.L.D = 0UL;

    if (b == 0) return r;

    /* 1st 32 bits : LL */
    if (a.Q.F.LL.F != 0) r.Q.D.L.D = (U64) a.Q.F.LL.F * (U64) b;

    /* 2nd 32 bits : LH */
    if( a.Q.F.LH.F != 0)
    {
        t = (U64) a.Q.F.LH.F  * (U64) b  +  (U64) r.Q.F.LH.F;
        r.Q.F.LH.F = t & 0xFFFFFFFFUL;
        r.Q.F.HL.F = t >> 32;
    }

    /* 3rd 32 bits : HL */
    if( a.Q.F.HL.F != 0)
    {
        t = (U64) a.Q.F.HL.F  * (U64) b  +  (U64) r.Q.F.HL.F;
        r.Q.F.HL.F = t & 0xFFFFFFFFUL;
        r.Q.F.HH.F = t >> 32;
    }

    /* 4th 32 bits : HH */
    if( a.Q.F.HH.F != 0)
    {
        t = (U64) a.Q.F.HH.F  * (U64) b  +  (U64) r.Q.F.HH.F;
        r.Q.F.HH.F = t & 0xFFFFFFFFUL;
    }
    return r;
#endif
}

/*===================================================================*/
/* U128 Shift: left, right logical, right arithmetic                 */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* U128_shl: shift left                                              */
/*      return a  << n                                               */
/*-------------------------------------------------------------------*/
static inline U128 U128_shl( U128 a, U32 shift)
{
    U128 temp;

#if defined( _USE_128_ )
    temp.u_128 =   a.u_128 << shift;
    return temp;

#else
    /* shift 0 or negative (undefined) */
    if ( shift <= 0)
        return a;

    /* shift 1 to 63 */
    if ( shift < 64 )
    {
        temp.Q.D.H.D = a.Q.D.H.D << shift | a.Q.D.L.D >> (64 - shift);
        temp.Q.D.L.D = a.Q.D.L.D << shift;
        return temp;
    }

    /* shift 64 */
    if ( shift == 64 )
    {
        temp.Q.D.H.D = a.Q.D.L.D;
        temp.Q.D.L.D = 0;
        return temp;
    }

    /* shift 65 to 127 */
    if ( shift < 128 )
    {
        temp.Q.D.H.D = a.Q.D.L.D << (shift - 64);
        temp.Q.D.L.D = 0;
        return temp;
    }

    /* shift 128 or greater */
    temp.Q.D.H.D = 0;
    temp.Q.D.L.D = 0;
    return temp;

#endif
}

/*-------------------------------------------------------------------*/
/* U128_shrl: shift right logical                                    */
/*      return a  >> shift                                           */
/*-------------------------------------------------------------------*/
static inline U128 U128_shrl( U128 a, U32 shift)
{
    U128 temp;

#if defined( _USE_128_ )
    temp.u_128 =   a.u_128 >> shift;
    return temp;

#else
    /* shift 0 or negative (undefined) */
    if ( shift <= 0)
        return a;

    /* shift 1 to 63 */
    if ( shift < 64 )
    {
        temp.Q.D.H.D = a.Q.D.H.D >> shift;
        temp.Q.D.L.D = a.Q.D.L.D >> shift | a.Q.D.H.D << (64 - shift);
        return temp;
    }

    /* shift 64 */
    if ( shift == 64 )
    {
        temp.Q.D.H.D = 0;
        temp.Q.D.L.D = a.Q.D.H.D;
        return temp;
    }

    /* shift 65 to 127 */
    if ( shift < 128 )
    {
        temp.Q.D.H.D = 0;
        temp.Q.D.L.D = a.Q.D.H.D >> (shift - 64);
        return temp;
    }

    /* shift 128 or greater */
    temp.Q.D.H.D = 0;
    temp.Q.D.L.D = 0;
    return temp;

#endif
}

/*-------------------------------------------------------------------*/
/* U128_shra: shift right arithmetic                                 */
/*      return (S128)a  >> shift                                     */
/*-------------------------------------------------------------------*/
static inline U128 U128_shra( U128 a, U32 shift)
{
    U128 temp;

#if defined( _USE_128_ )
    temp.u_128 =   a.s_128 >> shift;
    return temp;

#else
    S64 sign_bit;

    /* shift 0 or negative (undefined) */
    if ( shift <= 0)
        return a;

    sign_bit = a.Q.D.H.D >> 63;

    /* shift 1 to 63 */
    if ( shift < 64 )
    {
        temp.Q.D.H.D = (S64) a.Q.D.H.D >> shift;
        temp.Q.D.L.D = a.Q.D.L.D >> shift | a.Q.D.H.D << (64 - shift);
        return temp;
    }

    /* shift 64 */
    if ( shift == 64 )
    {
        temp.Q.D.H.D = (sign_bit == 0) ? 0 : -1;
        temp.Q.D.L.D = a.Q.D.H.D;
        return temp;
    }

    /* shift 65 to 127 */
    if ( shift < 128 )
    {
        temp.Q.D.H.D = (sign_bit == 0) ? 0 : -1;
        temp.Q.D.L.D = (S64) a.Q.D.H.D >> (shift - 64);
        return temp;
    }

    /* shift 128 or greater */
    temp.Q.D.H.D = (sign_bit == 0) ? 0 : -1;
    temp.Q.D.L.D = (sign_bit == 0) ? 0 : -1;
    return temp;

#endif
}

/*===================================================================*/
/* U128 Cast                                                         */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* U128_S64: return (U128) (S64) a                                   */
/*-------------------------------------------------------------------*/
static inline U128 U128_S64( U64 a )
{
    U128 temp;

    temp.Q.D.H.D = a;
    temp.Q.D.L.D = 0;

    return U128_shra( temp, 64 );
}

/*-------------------------------------------------------------------*/
/* U128_U64: return (U128) a                                         */
/*-------------------------------------------------------------------*/
static inline U128 U128_U64( U64 a )
{
    U128 temp;

    temp.Q.D.H.D = 0;
    temp.Q.D.L.D = a;

    return temp;
}

/*===================================================================*/
/* U128 Compare                                                      */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* U128 Compare: (U128) a to (U128) b                                */
/*      return -1:  a <  b                                           */
/*              0:  a == b                                           */
/*              1:  a >  b                                           */
/*-------------------------------------------------------------------*/
static inline int U128_cmp( U128 a, U128 b)
{
#if defined( _USE_128_ )
    if ( a.u_128 < b.u_128 )
        return -1;
    else if ( a.u_128 > b.u_128 )
        return 1;
    else
        return 0;

#else
    if ( a.Q.D.H.D < b.Q.D.H.D )
        return -1;
    else if ( a.Q.D.H.D > b.Q.D.H.D )
        return 1;
    else
    {
        if ( a.Q.D.L.D < b.Q.D.L.D )
            return -1;
        else if ( a.Q.D.L.D > b.Q.D.L.D )
            return 1;
        else
            return 0;
    }

#endif
}

/*-------------------------------------------------------------------*/
/* S128 Compare: (S128) a to (S128) b                                */
/*      return -1:  a <  b                                           */
/*              0:  a == b                                           */
/*              1:  a >  b                                           */
/*-------------------------------------------------------------------*/
static inline int S128_cmp( U128 a, U128 b)
{
#if defined( _USE_128_ )
    if ( a.s_128 < b.s_128 )
        return -1;
    else if ( a.s_128 > b.s_128 )
        return 1;
    else
        return 0;

#else
    if ( (S64) a.Q.D.H.D < (S64) b.Q.D.H.D )
        return -1;

    if ( (S64) a.Q.D.H.D > (S64) b.Q.D.H.D )
        return 1;

    if ( a.Q.D.L.D == b.Q.D.L.D )
        return 0;

    if ( a.Q.D.L.D < b.Q.D.L.D )
        return -1;

    return 1;
#endif
}

/*===================================================================*/
/* S128 Arithmetic (add, sub, div, mul, neg, rem)                    */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* S128 Add: return (s128) a + (s128) b                              */
/*-------------------------------------------------------------------*/
static inline U128 S128_add( U128 a, U128 b)
{
#if defined( _USE_128_ )
    U128 temp;                           /* temp (return) value      */

    temp.s_128 =  a.s_128 + b.s_128;
    return temp;

#else
    U128 temp;                           /* temp (return) value      */

    temp.Q.D.H.D =  (S64) a.Q.D.H.D + (S64) b.Q.D.H.D;
    temp.Q.D.L.D =  a.Q.D.L.D + b.Q.D.L.D;
    if (temp.Q.D.L.D < a.Q.D.L.D) temp.Q.D.H.D++;
    return temp;
#endif
}

/*-------------------------------------------------------------------*/
/* S128 Subtract: return (s128) a - (s128) b                         */
/*-------------------------------------------------------------------*/
static inline U128 S128_sub( U128 a, U128 b)
{
#if defined( _USE_128_ )
    U128 temp;                           /* temp (return) value      */

    temp.s_128 =  a.s_128 - b.s_128;
    return temp;

#else
    U128 temp;                           /* temp (return) value      */

    temp.Q.D.H.D =  (S64) a.Q.D.H.D - (S64) b.Q.D.H.D;
    temp.Q.D.L.D =  a.Q.D.L.D - b.Q.D.L.D;
    if (temp.Q.D.L.D > a.Q.D.L.D) temp.Q.D.H.D--;
    return temp;
#endif
}

/*-------------------------------------------------------------------*/
/* S128 Divide: return  (S128) a /  (S128) b                         */
/*-------------------------------------------------------------------*/
static inline U128 S128_div( U128 a, U128 b)
{
#if defined( _USE_128_ )
    U128 temp;                           /* temp (return) value      */

    temp.s_128 =  a.s_128 / b.s_128;
    return temp;

#else
    U128 temp_a, temp_b, tempquo, temprem;
    bool neg_a, neg_b;

    /* check a,b for negative values */
    neg_a =  a.Q.D.H.D >> 63;
    neg_b =  b.Q.D.H.D >> 63;

    temp_a = ( neg_a ) ? S128_neg (a) : a;
    temp_b = ( neg_b ) ? S128_neg (b) : b;

    tempquo = U128_divrem( temp_a, temp_b, &temprem);

    /* quotant is negative if either dividend/divisor is negative and the other operand is positive */
    if( neg_a ^ neg_b )
    {
        tempquo = S128_neg (tempquo);
    }

    return tempquo;
#endif
}

/*-------------------------------------------------------------------*/
/* S128 remainder: return  (S128) a % (S128) b                       */
/*-------------------------------------------------------------------*/
static inline U128 S128_rem( U128 a, U128 b)
{
#if defined( _USE_128_ )
    U128 temp;                           /* temp (return) value      */

    temp.s_128 =  a.s_128 % b.s_128;
    return temp;

#else
    U128 temp_a, temp_b, tempquo, temprem;
    bool neg_a, neg_b;

    /* check a,b for negative values */
    neg_a =  a.Q.D.H.D >> 63;
    neg_b =  b.Q.D.H.D >> 63;

    temp_a = ( neg_a ) ? S128_neg (a) : a;
    temp_b = ( neg_b ) ? S128_neg (b) : b;

    tempquo = U128_divrem( temp_a, temp_b, &temprem);

    /* remainder is negative is dividend (a) is negative */
    if( neg_a  )
    {
        temprem = S128_neg (temprem);
    }

    return temprem;
#endif
}

/*-------------------------------------------------------------------*/
/* U128: S64 * S64 Multiply: return (s64) a * (s64) b                */
/*              (overflow ignored)                                   */
/* Very simple, standard approach to arithmetic multiply             */
/*                                                                   */
/*-------------------------------------------------------------------*/
static inline U128 S128_mul_64 (U64 a, U64 b)
{
#if defined( _USE_128_)
    U128 temp;                           /* temp (return) value      */

    temp.s_128 =  (__int128) (S64) a * (__int128) (S64) b;
    return temp;

#else
    U128    temp128 ;
    U64     temp_a, temp_b;
    bool    neg_a, neg_b;

    /* check a,b for negative values */
    neg_a =  ( (S64) a < 0) ? 1 : 0;
    neg_b =  ( (S64) b < 0) ? 1 : 0;

    /* convert neg values to positive */
    temp_a = ( neg_a ) ? - (S64) a : a;
    temp_b = ( neg_b ) ? - (S64) b : b;

    temp128 = U128_mul_64( temp_a, temp_b );

    /* is the resulty negative? */
    if( neg_a ^ neg_b )
    {
        /* negate result */
        temp128 = S128_neg( temp128 );
    }

    return temp128;
#endif
}

/*-------------------------------------------------------------------*/
/* S128_mul: signed 128-bit multiply -> signed 256-bit result        */
/*      return:  hi128 : high 128 bits                               */
/*               lo128 : low  128 bits                               */
/*-------------------------------------------------------------------*/
static inline void S128_mul( U128 a, U128 b, U128* hi128, U128* lo128)
{
    U128 temp, temp_a, temp_b, temphi, templo;
    bool neg_a, neg_b;

    /* check a,b for negative values */
    neg_a =  a.Q.D.H.D >> 63;
    neg_b =  b.Q.D.H.D >> 63;

    temp_a = ( neg_a ) ? S128_neg (a) : a;
    temp_b = ( neg_b ) ? S128_neg (b) : b;

    U128_mul( temp_a, temp_b, &temphi, &templo);
            // U128_logmsg("mul   temphi: ", temphi );
            // U128_logmsg("mul   templo: ", templo );

    if( neg_a ^ neg_b )
    {
        /* negate result */
        temphi.Q.D.H.D = ~ temphi.Q.D.H.D;
        temphi.Q.D.L.D = ~ temphi.Q.D.L.D;

        templo.Q.D.H.D = ~ templo.Q.D.H.D;
        templo.Q.D.L.D = ~ templo.Q.D.L.D;

        temp = U128_add( templo, U128_one() );
        if (U128_cmp( temp, templo ) == -1)
            temphi = U128_add( temphi, U128_one() );
        templo = temp;
    }

    *hi128 = temphi;
    *lo128 = templo;
}

/*-------------------------------------------------------------------*/
/* S128_neg: negate signed a                                         */
/*      return:  - (S128) a                                          */
/*-------------------------------------------------------------------*/
static inline U128 S128_neg( U128 a )
{
    U128 temp;

#if defined( _USE_128_)
    temp.s_128 =   - a.s_128 ;
    return temp;

#else
    temp.Q.D.H.D = ~ a.Q.D.H.D;
    temp.Q.D.L.D = ~ a.Q.D.L.D;
    temp = U128_add( temp, U128_one() );
    return temp;
#endif
}

/*-------------------------------------------------------------------*/
/* S128_isNeg: if (S128) a < 0; return true                          */
/*-------------------------------------------------------------------*/
static inline bool S128_isNeg( U128 a )
{
    if ( (S64) a.Q.D.H.D < 0 )
        return true;

    return false;
}

/*===================================================================*/
/* U256/S256 Arithmetic (add)                                        */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* U256 add: return  (U256) a + (u128) b                             */
/*-------------------------------------------------------------------*/
static inline void U256_add_U128( U128 *ahi, U128 *alo, U128 b, U128 *chi, U128 *clo )
{
    U128    temphi, templo;

    temphi = *ahi;
    templo = U128_add ( *alo, b);

    if ( U128_cmp( templo, *alo) == -1)
        temphi = U128_add( temphi, U128_one() );

    chi->Q = temphi.Q;
    clo->Q = templo.Q;
}

/*-------------------------------------------------------------------*/
/* S256 add: return  (S256) a + (S128) b                             */
/*-------------------------------------------------------------------*/
static inline void S256_add_S128( U128 *ahi, U128 *alo, U128 b, U128 *chi, U128 *clo )
{
    U128    temphi, templo;

    temphi = *ahi;

    if ( S128_isNeg ( b ) )
        temphi = S128_sub( temphi, U128_one() );

    templo = U128_add ( *alo, b);

    if ( U128_cmp( templo, *alo) == -1)
        temphi = U128_add( temphi, U128_one() );

    chi->Q = temphi.Q;
    clo->Q = templo.Q;
}

/*-------------------------------------------------------------------*/
/* Galois Field Multiply                                             */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Galois Field(2) 32-bit Multiply                                   */
/*                                                                   */
/* Input:                                                            */
/*      m1      32-bit multiply operand                              */
/*      m2      32-bit multiply operand                              */
/*                                                                   */
/* Returns:                                                          */
/*              64-bit GF(2) multiply result                         */
/*                                                                   */
/* version depends on whether intrinsics are being used              */
/*-------------------------------------------------------------------*/
static inline U64 gf_mul_32( U32 m1, U32 m2)
{
#if defined( FEATURE_V128_SSE ) && defined( FEATURE_HW_CLMUL )

    if (sysblk.have_PCLMULQDQ)
    {
        /* intrinsic GF 64-bit multiply */
        QW  mm1;                      /* U128 m1                       */
        QW  mm2;                      /* U128 m2                       */
        QW  acc;                      /* U128 accumulator              */

        mm1.v = _mm_setzero_si128();
        mm1.D.L.D = m1;
            //logmsg("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", "gf_mul_32 mm1.v", mm1.D.H.D, mm1.D.L.D);

        mm2.v = _mm_setzero_si128();
        mm2.D.L.D = m2;
            //logmsg("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", "gf_mul_32 mm2.v", mm2.D.H.D, mm2.D.L.D);

        acc.v =  _mm_clmulepi64_si128 ( mm1.v, mm2.v, 0);
            //logmsg("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", "gf_mul_32 acc.v", acc.D.H.D, acc.D.L.D);

        return acc.D.L.D;
    }
    else

#endif  //  !(defined( FEATURE_V128_SSE ) && defined( FEATURE_HW_CLMUL )), or
        // "PCLMULQDQ" instruction unavailable
    {
        int     i;                    /* loop index                      */
        U32     myerU32;              /* multiplier                      */
        U64     mcandU64;             /* multiplicand                    */
        U64     accu64;               /* accumulator                     */

        accu64 = 0;

        /* select muliplier with fewest 'right most' bits */
        /* to exit loop soonest                           */
        if (m1 < m2)
        {
            mcandU64 = m2;
            myerU32  = m1;
        }
        else
        {
            mcandU64 = m1;
            myerU32  = m2;
        }

        /* galois multiply - no overflow */
        for (i=0; i < 32 && myerU32 !=0; i++)
        {
            if ( myerU32 & 0x01 )
                accu64 ^= mcandU64;
            myerU32  >>= 1;
            mcandU64 <<=1;
        }

        return accu64;
    }
}

/*-------------------------------------------------------------------*/
/* Galois Field(2) 64-bit Multiply                                   */
/*                                                                   */
/* Input:                                                            */
/*      m1          64-bit multiply operand                          */
/*      m2          64-bit multiply operand                          */
/*      accu128h    pointer to high 64 bits of result                */
/*      accu128l    pointer to low 64 bits of result                 */
/*                                                                   */
/* version depends on whether intrinsics are being used              */
/*-------------------------------------------------------------------*/
static inline void gf_mul_64( U64 m1, U64 m2, U64* accu128h, U64* accu128l)
{
#if defined( FEATURE_V128_SSE ) && defined( FEATURE_HW_CLMUL )

    if (sysblk.have_PCLMULQDQ)
    {
        /* intrinsic GF 64-bit multiply */
        QW  mm1;                      /* U128 m1                       */
        QW  mm2;                      /* U128 m2                       */
        QW  acc;                      /* U128 accumulator              */

        mm1.v = _mm_setzero_si128();
        mm1.D.L.D = m1;
            //logmsg("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", "gf_mul_64 mm1.v", mm1.D.H.D, mm1.D.L.D);

        mm2.v = _mm_setzero_si128();
        mm2.D.L.D = m2;
            //logmsg("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", "gf_mul_64 mm2.v", mm2.D.H.D, mm2.D.L.D);

        acc.v =  _mm_clmulepi64_si128 ( mm1.v, mm2.v, 0);
            //logmsg("%s: u128=%16.16"PRIX64".%16.16"PRIX64" \n", "gf_mul_64 acc.v", acc.D.H.D, acc.D.L.D);

        *accu128h = acc.D.H.D;
        *accu128l = acc.D.L.D;
    }
    else

#endif  //  !(defined( FEATURE_V128_SSE ) && defined( FEATURE_HW_CLMUL )), or
        // "PCLMULQDQ" instruction unavailable
    {
        /* portable C: GF 64-bit multiply */
        int     i;                    /* loop index                      */
        U64     myerU64;              /* doublewword multiplier          */
        U64     mcandU128h;           /* doublewword multiplicand - high */
        U64     mcandU128l;           /* doublewword multiplicand - low  */

        *accu128h = 0;
        *accu128l = 0;

        /* select muliplier with fewest 'right most' bits */
        /* to exit loop soonest                           */
        if (m1 < m2)
        {
            mcandU128h  = 0;
            mcandU128l = m2;
            myerU64  = m1;
        }
        else
        {
            mcandU128h  = 0;
            mcandU128l = m1;
            myerU64  = m2;
        }

        /* galois multiply - no overflow */
        for (i=0; i < 64 && myerU64 !=0; i++)
        {
            if ( myerU64 & 0x01 )
            {
                *accu128h ^= mcandU128h;
                *accu128l ^= mcandU128l;
            }
            myerU64  >>= 1;

            /* U128: shift left 1 bit*/
            mcandU128h = (mcandU128h << 1) | (mcandU128l >> 63);
            mcandU128l <<= 1;
        }
    }
}

/*===================================================================*/
/* U128 bitwise boolean                                              */
/*===================================================================*/
/*-------------------------------------------------------------------*/
/* U128 AND: return a & b                                            */
/*-------------------------------------------------------------------*/
static inline U128 U128_and( U128 a, U128 b )
{
    U128 temp;                           /* temp (return) value      */

    temp.Q.D.H.D =  a.Q.D.H.D  &  b.Q.D.H.D;
    temp.Q.D.L.D =  a.Q.D.L.D  &  b.Q.D.L.D;
    return temp;
}
/*-------------------------------------------------------------------*/
/* U128 OR: return a | b                                             */
/*-------------------------------------------------------------------*/
static inline U128 U128_or( U128 a, U128 b )
{
    U128 temp;                           /* temp (return) value      */

    temp.Q.D.H.D =  a.Q.D.H.D  |  b.Q.D.H.D;
    temp.Q.D.L.D =  a.Q.D.L.D  |  b.Q.D.L.D;
    return temp;
}
/*-------------------------------------------------------------------*/
/* U128 XOR: return a ^ b                                            */
/*-------------------------------------------------------------------*/
static inline U128 U128_xor( U128 a, U128 b )
{
    U128 temp;                           /* temp (return) value      */

    temp.Q.D.H.D =  a.Q.D.H.D  ^  b.Q.D.H.D;
    temp.Q.D.L.D =  a.Q.D.L.D  ^  b.Q.D.L.D;
    return temp;
}
/*-------------------------------------------------------------------*/
/* U128 NAND: return ~(a & b)                                        */
/*-------------------------------------------------------------------*/
static inline U128 U128_nand( U128 a, U128 b )
{
    U128 temp;                           /* temp (return) value      */

    temp.Q.D.H.D =  ~(a.Q.D.H.D  &  b.Q.D.H.D);
    temp.Q.D.L.D =  ~(a.Q.D.L.D  &  b.Q.D.L.D);
    return temp;
}
/*-------------------------------------------------------------------*/
/* U128 NOR: return ~(a | b)                                         */
/*-------------------------------------------------------------------*/
static inline U128 U128_nor( U128 a, U128 b )
{
    U128 temp;                           /* temp (return) value      */

    temp.Q.D.H.D =  ~(a.Q.D.H.D  |  b.Q.D.H.D);
    temp.Q.D.L.D =  ~(a.Q.D.L.D  |  b.Q.D.L.D);
    return temp;
}
/*-------------------------------------------------------------------*/
/* U128 NXOR: return ~(a ^ b)                                        */
/*-------------------------------------------------------------------*/
static inline U128 U128_nxor( U128 a, U128 b )
{
    U128 temp;                           /* temp (return) value      */

    temp.Q.D.H.D =  ~(a.Q.D.H.D  ^  b.Q.D.H.D);
    temp.Q.D.L.D =  ~(a.Q.D.L.D  ^  b.Q.D.L.D);
    return temp;
}
/*-------------------------------------------------------------------*/
/* U128 NOT: return ~a                                               */
/*-------------------------------------------------------------------*/
static inline U128 U128_not( U128 a )
{
    U128 temp;                           /* temp (return) value      */

    temp.Q.D.H.D =  ~(a.Q.D.H.D);
    temp.Q.D.L.D =  ~(a.Q.D.L.D);
    return temp;
}
/*-------------------------------------------------------------------*/
/* U128 BITON: if (a & b) return true or false                       */
/*-------------------------------------------------------------------*/
static inline bool U128_biton( U128 a, U128 b )
{
    if ((a.Q.D.H.D & b.Q.D.H.D) || (a.Q.D.L.D & b.Q.D.L.D))
        return TRUE;

    return FALSE;
}

/*===================================================================*/
/* Utility Helpers                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* Set 16 bytes (128 bits) to zero                                   */
/*                                                                   */
/* Input:                                                            */
/*      addr    address of 16-byte asrea to zero                     */
/*                                                                   */
/* version depends on whether intrinsics are being used              */
/*-------------------------------------------------------------------*/
#if defined(__V128_SSE__)

static inline void SetZero_128 (void* addr)
{
    __m128i v = _mm_setzero_si128();
    memcpy( addr, &v, sizeof( __m128i ));
}

#else

static inline void SetZero_128 (void* addr)
{
    memset( addr, 0, sizeof(U128) );
}

#endif

#endif // __ZVECTOR_H_
