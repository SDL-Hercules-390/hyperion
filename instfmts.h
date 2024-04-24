/* INSTFMTS.H   (C) Copyright Jan Jaeger, 2000-2012                  */
/*              Instruction decoding macros and prototypes           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module defines instruction decoding macros according to the  */
/* instruction formats documented on pages 5-4, 5-5 of IBM reference */
/* manual SA22-7832-12 "The z/Architecture Principles of Operation"  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* PROGRAMMING NOTE: this header purposely does not prevent itself   */
/* from being #included multiple times. This is because it needs to  */
/* be #included by the opcode.h header multiple times, once for each */
/* build architecture.                                               */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*                     Instruction decoders                          */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* A decoder is placed at the start of each instruction.  The pur-   */
/* pose of a decoder is to extract the operand fields according to   */
/* the instruction format, increment the instruction address (IA)    */
/* field of the PSW by 2, 4, or 6 bytes, and to set the instruction  */
/* length code (ILC) field of the PSW in case a program check occurs.*/
/*                                                                   */
/* Certain decoders have an additional form with a "_B" suffix:      */
/*                                                                   */
/*    - The _B version does not update the PSW IA (i.e.'ip')         */
/*                                                                   */
/* The _B versions for some of the decoders are intended for branch  */
/* type instructions where updating the PSW IA to IA+ILC should only */
/* be done after the branch is deemed impossible. The _B decoders    */
/* therefore do not have a INST_UPDATE_PSW() macro as part of their  */
/* definition whereas all other decoders normally do. Instead, such  */
/* instructions manually do the INST_UPDATE_PSW() macro themselves   */
/* in the actual DEF_INST function for the branch not taken case.    */
/*                                                                   */
/* Instruction decoders are DEPENDENT on the build architecture and  */
/* therefore each decoder macro MUST be undefined and then redefined */
/* for each new build architecture and MUST also be located in the   */
/* ARCH_DEP section of the opcode.h header.                          */
/*                                                                   */
/*-------------------------------------------------------------------*/

#undef E
#undef IE
#undef MII_A
#undef RR
#undef RR_B
#undef RR_SVC
#undef RRE
#undef RRE_B
#undef RRF_M
#undef RRF_M4
#undef RRF_RM
#undef RRF_MM
#undef RRR
#undef RX
#undef RX_B
#undef RXX_BC
#undef RXX0RX
#undef RXXx
#undef RXXx_BC
#undef RX_BC
#undef RXE
#undef RXE_M3
#undef RXF
#undef RXY
#undef RXY_B
#undef RXYX
#undef RS
#undef RS_B
#undef RSY
#undef RSY_B
#undef RSL
#undef RSL_RM
#undef RI
#undef RI_B
#undef RIE
#undef RIE_B
#undef RIE_RIM
#undef RIE_RRIM_B
#undef RIE_RMII_B
#undef RIE_RRIII
#undef RIL
#undef RIL_B
#undef RIL_A
#undef RIS_B
#undef RRS_B
#undef SI
#undef SIIX
#undef SIY
#undef SIL
#undef SMI_A
#undef S
#undef SS
#undef SS_L
#undef SSE
#undef SSF
#undef VS
#undef S_NW
#undef VRI_A
#undef VRI_B
#undef VRI_C
#undef VRI_D
#undef VRI_E
#undef VRR_F
#undef VRR_A
#undef VRR_B
#undef VRR_C
#undef VRR_D
#undef VRR_E
#undef VRS_A
#undef VRS_B
#undef VRS_C
#undef VRV
#undef VRX

/*-------------------------------------------------------------------*/
/*            E - implied operands and extended op code              */
/*-------------------------------------------------------------------*/
// This is z/Arch E format.

#define E( _inst,_regs )        E_DECODER( (_inst), (_regs), 2, 2 )

//  0           1           2
//  +-----+-----+-----+-----+
//  |     OP    |    XOP    |         E
//  +-----+-----+-----+-----+
//  0     4     8     12   15

#define E_DECODER( _inst, _regs, _len, _ilc )                       \
{                                                                   \
    UNREFERENCED( _inst );                                          \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*      IE - extended op code with two 4-bit immediate fields        */
/*-------------------------------------------------------------------*/
// This is z/Arch IE format.

#define IE( _inst, _regs, _i1, _i2 )   IE_DECODER( _inst, _regs, _i1, _i2, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |    XOP    | /// | /// |  i1 |  i2 |      IE
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define IE_DECODER( _inst, _regs, _i1, _i2, _len, _ilc )            \
{                                                                   \
    int i = (_inst)[3];                                             \
                                                                    \
    (_i2) = (i >> 0) & 0x0f;                                        \
    (_i1) = (i >> 4) & 0x0f;                                        \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*    MII_A - mask with 12-bit and 24-bit relative address fields    */
/*-------------------------------------------------------------------*/
// This is z/Arch MII format.

#define MII_A( _inst, _regs, _m1, _addr2, _addr3 )   MII_A_DECODER( _inst, _regs, _m1, _addr2, _addr3, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | m1  |       ri2       |                ri3                |    MII
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define MII_A_DECODER( _inst, _regs, _m1, _addr2, _addr3, _len, _ilc )  \
{                                                                   \
    U32 ri2, ri3; S64 offset;                                       \
    U32 temp = fetch_fw( &(_inst)[2] );                             \
                                                                    \
    int i = (_inst)[1];                                             \
                                                                    \
    (_m1)  = (i >> 4) & 0x0F;                                       \
    ri2    = (i << 4) | (temp >> 24);                               \
    ri3    = temp & 0xFFFFFF;                                       \
                                                                    \
    offset = 2LL * (S32) ri2;                                       \
                                                                    \
    (_addr2) = likely( !(_regs)->execflag )                         \
        ? PSW_IA_FROM_IP( (_regs), offset )                         \
        : ((_regs)->ET + offset) & ADDRESS_MAXWRAP( (_regs) );      \
                                                                    \
    offset = 2LL * (S32) ri3;                                       \
                                                                    \
    (_addr3) = likely( !(_regs)->execflag )                         \
        ? PSW_IA_FROM_IP((_regs), offset)                           \
        : ((_regs)->ET + offset) & ADDRESS_MAXWRAP( (_regs) );      \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*                  RR - register to register                        */
/*-------------------------------------------------------------------*/
// This is z/Arch RR format.

#define RR(   _inst, _regs, _r1, _r2 )  RR_DECODER( _inst, _regs, _r1, _r2, 2, 2 )
#define RR_B( _inst, _regs, _r1, _r2 )  RR_DECODER( _inst, _regs, _r1, _r2, 0, 2 )

//  0           1           2
//  +-----+-----+-----+-----+
//  |     OP    | r1  | r2  |       RR
//  +-----+-----+-----+-----+
//  0     4     8     12   15

#define RR_DECODER( _inst, _regs, _r1, _r2, _len, _ilc )            \
{                                                                   \
    int i = (_inst)[1];                                             \
                                                                    \
    (_r2) = (i >> 0) & 0x0f;                                        \
    (_r1) = (i >> 4) & 0x0f;                                        \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*         RR_SVC - special format for SVC instruction               */
/*-------------------------------------------------------------------*/
// This is z/Arch I format.

#define RR_SVC( _inst, _regs, _svc )  RR_SVC_DECODER( _inst, _regs, _svc, 2, 2 )

//  0           1           2
//  +-----+-----+-----+-----+
//  |     OP    |    svc    |       I
//  +-----+-----+-----+-----+
//  0     4     8     12   15

#define RR_SVC_DECODER( _inst, _regs, _svc, _ilc, _len )            \
{                                                                   \
    (_svc) = (_inst)[1];                                            \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_ilc), (_len) );                     \
}

/*-------------------------------------------------------------------*/
/*         RRE - register to register with extended op code          */
/*-------------------------------------------------------------------*/
// This is z/Arch RRE format.

#define RRE(   _inst, _regs, _r1, _r2 )  RRE_DECODER( _inst, _regs, _r1, _r2, 4, 4 )
#define RRE_B( _inst, _regs, _r1, _r2 )  RRE_DECODER( _inst, _regs, _r1, _r2, 0, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |    XOP    | /// | /// | r1  | r2  |     RRE
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RRE_DECODER(_inst, _regs, _r1, _r2, _len, _ilc)             \
{                                                                   \
    int i = (_inst)[3];                                             \
                                                                    \
    (_r2) = (i >> 0) & 0xf;                                         \
    (_r1) = (i >> 4) & 0xf;                                         \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*                HISTORICAL PROGRAMMING NOTE                        */
/*-------------------------------------------------------------------*/
//
// The RRD instruction format doesn't exist in the S/390 architecture.
// Instead it calls that particular instruction format the RRF format,
// which in z/Arch is a completely different instruction format.
//
// That is to say, z/Architecture has both of the RRD and RRF formats
// defined and both are different from each other: in the z/Arch RRD
// format, "r1" is in bit positions 16-19 and "r3" in bit positions
// 24-27, but for the RRF-a and -b formats they are in the complete
// opposite position: in RRF-a and -b "r1" is in bit position 24-27
// and "r3" is in bit positions 16-19 (i.e. r1 and r3 operands are
// in the oppsoite position in the RRD and RRF formats).
//
// This is confusing since Hercules's ORIGINAL "RRF_R" format decoder
// was originally written to decode the S/390 "RRF" format which is
// now called the "RRD" format in z/Architecture.
//
// Therefore to eliminate the confusion our original "RRF_R" format
// decoder was removed and all instructions that were using it were
// fixed to correctly use the z/Architecture "RRD" decoder instead
// (which accomplishes the same thing as our original S/390 "RRF_R"
// decoder originally did).
//
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*       RRD - register to register with additional R3 field         */
/*-------------------------------------------------------------------*/
// This is the z/Arch RRD format.

#define RRD( _inst, _regs, _r1, _r2, _r3 )  RRD_DECODER( _inst, _regs, _r1, _r2, _r3, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |    XOP    | r1  | /// | r3  | r2  |      RRD
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RRD_DECODER( _inst, _regs, _r1, _r2, _r3, _len, _ilc )      \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_r2) = (temp >>  0) & 0xf;                                     \
    (_r3) = (temp >>  4) & 0xf;                                     \
    (_r1) = (temp >> 12) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*  RRF_RM - register to register with additional R3 and M4 fields   */
/*-------------------------------------------------------------------*/
// This is z/Arch RRF-a and -b format.

/*-------------------------------------------------------------------*/
/*                     Programming Note                              */
/*-------------------------------------------------------------------*/
//
// Altough there are only two basic RRF instruction formats (RRF_RM
// and RRF_MM), z/Architecture defines 5 variations (RRF-a and RRF-b,
// and RRF-c to RRF-e) because of the different assembler-language
// syntaxes that are used for the many different RRF instructions:
//
//      Format       Assembler Syntax       Decoder  (?)
//     --------     ------------------     ---------------
//
//      RRF-a        r1,r2[,r3[,m4]]        RRF_RM   RRR?
//      RRF-b        r1,r3,r2[,m4]          RRR      RRF_RM?
//      RRF-c        r1,r2[,m3]             RRF_M    RRF_MM?, RRR?
//      RRF-d        r1,r2,m4               RRF_M4   RRF_MM?
//      RRF-e        r1,m3,r2[,m4]          RRF_MM   RRR?, RRF_M?
//
/*-------------------------------------------------------------------*/

#define RRF_RM( _inst, _regs, _r1, _r2, _r3, _m4 )  RRF_RM_DECODER( _inst, _regs, _r1, _r2, _r3, _m4, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |    XOP    | r3  | m4  | r1  | r2  |   RRF-a, RRF-b
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RRF_RM_DECODER( _inst, _regs, _r1, _r2, _r3, _m4, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_r2) = (temp >>  0) & 0xf;                                     \
    (_r1) = (temp >>  4) & 0xf;                                     \
    (_m4) = (temp >>  8) & 0xf;                                     \
    (_r3) = (temp >> 12) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*  RRF_MM - register to register with additional M3 and M4 fields    */
/*-------------------------------------------------------------------*/
// This is z/Arch RRF-c through -e formats.

#define RRF_MM( _inst, _regs, _r1, _r2, _m3, _m4 )  RRF_MM_DECODER( _inst, _regs, _r1, _r2, _m3, _m4, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |    XOP    | m3  | m4  | r1  | r2  |   RRF-c .. RRF-e
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RRF_MM_DECODER( _inst, _regs, _r1, _r2, _m3, _m4, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_r2) = (temp >>  0) & 0xf;                                     \
    (_r1) = (temp >>  4) & 0xf;                                     \
    (_m4) = (temp >>  8) & 0xf;                                     \
    (_m3) = (temp >> 12) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*    RRF_M - register to register with additional M3 field          */
/*-------------------------------------------------------------------*/
// This is z/Arch RRF-c through -e format, but without any m4 field.

#define RRF_M( _inst, _regs, _r1, _r2, _m3 )  RRF_M_DECODER( _inst, _regs, _r1, _r2, _m3, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |    XOP    | m3  | /// | r1  | r2  |   RRF-c .. RRF-e  (no m4)
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RRF_M_DECODER( _inst, _regs, _r1, _r2, _m3, _len, _ilc )    \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_r2) = (temp >>  0) & 0xf;                                     \
    (_r1) = (temp >>  4) & 0xf;                                     \
 /* (_m4) = (temp >>  8) & 0xf; */                                  \
    (_m3) = (temp >> 12) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*     RRF_M4 - register to register with additional M4 field        */
/*-------------------------------------------------------------------*/
// This is z/Arch RRF-a through -e format, but without any m3 field.

#define RRF_M4( _inst, _regs, _r1, _r2, _m4 )  RRF_M4_DECODER( _inst, _regs, _r1, _r2, _m4, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |    XOP    | /// | m4  | r1  | r2  |   RRF-a .. RRF-e  (no m3)
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RRF_M4_DECODER( _inst, _regs, _r1, _r2, _m4, _len, _ilc )   \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_r2) = (temp >>  0) & 0xf;                                     \
    (_r1) = (temp >>  4) & 0xf;                                     \
    (_m4) = (temp >>  8) & 0xf;                                     \
 /* (_m3) = (temp >> 12) & 0xf; */                                  \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*           RRR - register to register with register                */
/*-------------------------------------------------------------------*/
// This is z/Arch RRF-a and -b format, but without any m4 field.

#define RRR( _inst, _regs, _r1, _r2, _r3 )  RRR_DECODER( _inst, _regs, _r1, _r2, _r3, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |    XOP    | r3  | /// | r1  | r2  |   RRF-a, RRF-b  (no m4)
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RRR_DECODER( _inst, _regs, _r1, _r2, _r3, _len, _ilc )      \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_r2) = (temp >>  0) & 0xf;                                     \
    (_r1) = (temp >>  4) & 0xf;                                     \
 /* (_m4) = (temp >>  8) & 0xf; */                                  \
    (_r3) = (temp >> 12) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*               RX - register and indexed storage                   */
/*-------------------------------------------------------------------*/
// This is z/Arch RX-a format.

#define RX(   _inst, _regs, _r1, _x2, _b2, _effective_addr2 )  RX_DECODER( _inst, _regs, _r1, _x2, _b2, _effective_addr2, 4, 4 )
#define RX_B( _inst, _regs, _r1, _x2, _b2, _effective_addr2 )  RX_DECODER( _inst, _regs, _r1, _x2, _b2, _effective_addr2, 0, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | x2  | b2  |       d2        |     RX-a
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RX_DECODER( _inst, _regs, _r1, _x2, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_x2)              = (temp >> 16) & 0xf;                        \
    (_r1)              = (temp >> 20) & 0xf;                        \
                                                                    \
    if (unlikely( _x2 ))                                            \
        (_effective_addr2) += (_regs)->GR(( _x2 ));                 \
                                                                    \
    (_b2) = (temp >> 12) & 0xf;                                     \
                                                                    \
    if (likely( _b2 ))                                              \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    if (( _len ))                                                   \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*     RX_BC - register and indexed storage - optimized for BC       */
/*-------------------------------------------------------------------*/
// This is z/Arch RX-b format, but without extracting the m1 field.

#define RX_BC( _inst, _regs, _b2, _effective_addr2 )  RX_BC_DECODER( _inst, _regs, _b2, _effective_addr2, 0, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | /// | x2  | b2  |       d2        |     RX-b  (no m1)
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RX_BC_DECODER( _inst, _regs, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_b2)              = (temp >> 16) & 0xf;  /* (actually x2) */   \
                                                                    \
    if (unlikely(( _b2 )))                    /* (actually x2) */   \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    (_b2) = (temp >> 12) & 0xf;               /* (the REAL b2) */   \
                                                                    \
    if (likely(( _b2 )))                      /* (the REAL b2) */   \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
}

#if defined( OPTION_OPTINST )
/*-------------------------------------------------------------------*/
/*    Optimized RX decoder for BRANCHES when X2 known to be ZERO     */
/*-------------------------------------------------------------------*/
// This is z/Arch RX-b format for BRANCHES when x2 known to be ZERO.

#define RXX_BC( _inst, _regs, _b2, _effective_addr2 )  RXX0_BC_DECODER( _inst, _regs, _b2, _effective_addr2, 0, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | /// | '0' | b2  |       d2        |     RX-b  (no m1, presumed x2=0)
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RXX0_BC_DECODER( _inst, _regs, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_b2)              = (temp >> 12) & 0xf;                        \
                                                                    \
    if (likely( _b2 ))                                              \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*  Optimized RX decoder in case of ZERO X2 without r1 calculation   */
/*-------------------------------------------------------------------*/
// This is z/Arch RX-a format without r1 when x2 known to be ZERO.

#define RXX0RX(  _inst, _regs, _b2, _effective_addr2 )  RXX0RX_DECODER( _inst, _regs, _b2, _effective_addr2, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | /// | '0' | b2  |       d2        |     RX-a  (no r1, presumed x2=0)
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RXX0RX_DECODER( _inst, _regs, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_b2)              = (temp >> 12) & 0xf;                        \
                                                                    \
    if (likely( _b2 ))                                              \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    if (( _len ))                                                   \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*          Optimized RX decoder in case of NON-ZERO X2              */
/*-------------------------------------------------------------------*/
// This is z/Arch RX-a format when x2 known to be NON-ZERO.

#define RXXx(  _inst, _regs, _r1, _x2, _b2, _effective_addr2 )  RXXx_DECODER( _inst, _regs, _r1, _x2, _b2, _effective_addr2, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | X2! | b2  |       d2        |     RX-a  (presumed NON-ZERO x2)
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RXXx_DECODER( _inst, _regs, _r1, _x2, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2)  = (temp >>  0) & 0xfff;                     \
    (_r1)               = (temp >> 20) & 0xf;                       \
    (_x2)               = (temp >> 16) & 0xf;                       \
    (_effective_addr2) += (_regs)->GR(( _x2 ));                     \
    (_b2)               = (temp >> 12) & 0xf;                       \
    if (likely(( _b2 )))                                            \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    if (( _len ))                                                   \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*     Optimized RX for BRANCHES when X2 known to be NON-ZERO        */
/*-------------------------------------------------------------------*/
// This is z/Arch RX-b for BRANCHES when x2 known to be NON-ZERO.

#define RXXx_BC( _inst, _regs, _b2, _effective_addr2 )  RXXx_BC_DECODER( _inst, _regs, _b2, _effective_addr2, 0, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | /// | X2! | b2  |       d2        |     RX-b  (no m1, presumed NON-ZERO x2)
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RXXx_BC_DECODER( _inst, _regs, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2)  = (temp >>  0) & 0xfff;                     \
    (_b2)               = (temp >> 16) & 0xf;  /* (actually x2) */  \
    (_effective_addr2) += (_regs)->GR(( _b2 ));/* (actually x2) */  \
    (_b2)              =  (temp >> 12) & 0xf;  /* (the REAL b2) */  \
    if (likely(( _b2 )))                       /* (the REAL b2) */  \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
}
#endif /* defined( OPTION_OPTINST ) */

/*-------------------------------------------------------------------*/
/*     RXE - register and indexed storage with extended op code      */
/*-------------------------------------------------------------------*/
// This is z/Arch RXE format, but without extracting the m3 field.

#define RXE( _inst, _regs, _r1, _x2, _b2, _effective_addr2 )  RXE_DECODER( _inst, _regs, _r1, _x2, _b2, _effective_addr2, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | x2  | b2  |       d2        | /// | /// |    XOP    |    RXE (no m3)
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RXE_DECODER( _inst, _regs, _r1, _x2, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_x2)              = (temp >> 16) & 0xf;                        \
    (_r1)              = (temp >> 20) & 0xf;                        \
                                                                    \
    if (( _x2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _x2 ));                 \
                                                                    \
    (_b2) = (temp >> 12) & 0xf;                                     \
                                                                    \
    if (( _b2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*     RXE_M3 - register and indexed storage with extended op code   */
/*-------------------------------------------------------------------*/
// This is z/Arch RXE format, extracting the m3 field.

#define RXE_M3( _inst, _regs, _r1, _x2, _b2, _effective_addr2, _m3 )  RXE_DECODER_M3( _inst, _regs, _r1, _x2, _b2, _effective_addr2, _m3, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | x2  | b2  |       d2        | m3  | /// |    XOP    |    RXE
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RXE_DECODER_M3( _inst, _regs, _r1, _x2, _b2, _effective_addr2, _m3, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst + 1);                                \
                                                                    \
    (_effective_addr2) = (temp >>  8) & 0xfff;                      \
    (_x2)              = (temp >> 24) & 0xf;                        \
    (_r1)              = (temp >> 28) & 0xf;                        \
    (_m3)              = (temp >>  4) & 0xf;                        \
                                                                    \
    if (( _x2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _x2 ));                 \
                                                                    \
    (_b2) = (temp >> 20) & 0xf;                                     \
                                                                    \
    if (( _b2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*  RXF - register & indexed storage w/ext.opcode and additional R3  */
/*-------------------------------------------------------------------*/
// This is z/Arch RXF format.

#define RXF( _inst, _regs, _r1, _r3, _x2, _b2, _effective_addr2 )  RXF_DECODER( _inst, _regs, _r1, _r3, _x2, _b2, _effective_addr2, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r3  | x2  | b2  |       d2        | r1  | /// |    XOP    |    RXF
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RXF_DECODER( _inst, _regs, _r1, _r3, _x2, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_r1)              = (_inst)[4] >> 4;                           \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_x2)              = (temp >> 16) & 0xf;                        \
    (_r3)              = (temp >> 20) & 0xf;                        \
                                                                    \
    if (( _x2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _x2 ));                 \
                                                                    \
    (_b2) = (temp >> 12) & 0xf;                                     \
                                                                    \
    if (( _b2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*  RXY - register & indexed storage w/ext.opcode and long displ.    */
/*-------------------------------------------------------------------*/
// This is z/Arch RXY-a and -b formats.

#if defined( FEATURE_018_LONG_DISPL_INST_FACILITY )

#define RXY(   _inst, _regs, _r1, _x2, _b2, _effective_addr2 )  RXY_DECODER_LD( _inst, _regs, _r1, _x2, _b2, _effective_addr2, 6, 6 )
#define RXY_B( _inst, _regs, _r1, _x2, _b2, _effective_addr2 )  RXY_DECODER_LD( _inst, _regs, _r1, _x2, _b2, _effective_addr2, 0, 6 )

#else /* !defined( FEATURE_018_LONG_DISPL_INST_FACILITY ) */

#define RXY(   _inst, _regs, _r1, _x2, _b2, _effective_addr2 )  RXY_DECODER(    _inst, _regs, _r1, _x2, _b2, _effective_addr2, 6, 6 )
#define RXY_B( _inst, _regs, _r1, _x2, _b2, _effective_addr2 )  RXY_DECODER(    _inst, _regs, _r1, _x2, _b2, _effective_addr2, 0, 6 )

#endif /* defined( FEATURE_018_LONG_DISPL_INST_FACILITY ) */

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | x2  | b2  |       dl2       |    dh2    |    XOP    |    RXY-a
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | m1  | x2  | b2  |       dl2       |    dh2    |    XOP    |    RXY-b
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RXY_DECODER_LD( _inst, _regs, _r1, _x2, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    S32 disp2; U32 temp = fetch_fw( _inst );                        \
                                                                    \
    (_effective_addr2) = 0;                                         \
                                                                    \
    disp2 = (temp >>  0) & 0xfff;                                   \
    (_x2) = (temp >> 16) & 0xf;                                     \
    (_r1) = (temp >> 20) & 0xf;                                     \
                                                                    \
    if (( _x2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _x2 ));                 \
                                                                    \
    (_b2) = (temp >> 12) & 0xf;                                     \
                                                                    \
    if (( _b2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    if (unlikely((_inst)[4]))       /* long displacement?  */       \
    {                                                               \
        disp2 |= (_inst[4] << 12);                                  \
                                                                    \
        if (disp2 & 0x80000)        /* high order bit on?  */       \
            disp2 |= 0xfff00000;    /* make disp2 negative */       \
    }                                                               \
                                                                    \
    (_effective_addr2) += disp2;                                    \
                                                                    \
    if (( _len ))                                                   \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | x2  | b2  |       dl2       |    dh2    |    XOP    |    RXY-a
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | m1  | x2  | b2  |       dl2       |    dh2    |    XOP    |    RXY-b
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RXY_DECODER( _inst, _regs, _r1, _x2, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_x2)              = (temp >> 16) & 0xf;                        \
    (_r1)              = (temp >> 20) & 0xf;                        \
                                                                    \
    if (( _x2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _x2 ));                 \
                                                                    \
    (_b2) = (temp >> 12) & 0xf;                                     \
                                                                    \
    if (( _b2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    if (( _len ))                                                   \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

#if defined( OPTION_OPTINST )
/*-------------------------------------------------------------------*/
/*        Optimized RXY decoder when X2 is known to be ZERO          */
/*-------------------------------------------------------------------*/
// This is z/Arch RXY-a format when x2 is known to be ZERO.

#ifdef FEATURE_018_LONG_DISPL_INST_FACILITY
  #define RXYX( _inst, _regs, _r1, _b2, _effective_addr2 )  RXYX_DECODER_LD( _inst, _regs, _r1, _b2, _effective_addr2, 6, 6 )
#else
  #define RXYX( _inst, _regs, _r1, _b2, _effective_addr2 )  RXYX_DECODER(    _inst, _regs, _r1, _b2, _effective_addr2, 6, 6 )
#endif

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | '0' | b2  |       dl2       |    dh2    |    XOP    |    RXY-a  (presumed x2=0)
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RXYX_DECODER_LD( _inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    S32 disp2; U32 temp = fetch_fw( _inst );                        \
                                                                    \
    (_effective_addr2) = 0;                                         \
                                                                    \
    disp2 = (temp >>  0) & 0xfff;                                   \
    (_b2) = (temp >> 12) & 0xf;                                     \
    (_r1) = (temp >> 20) & 0xf;                                     \
                                                                    \
    if (( _b2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    if (unlikely((_inst)[4]))       /* long displacement?  */       \
    {                                                               \
        disp2 |= (_inst[4] << 12);                                  \
                                                                    \
        if (disp2 & 0x80000)        /* high order bit on?  */       \
            disp2 |= 0xfff00000;    /* make disp2 negative */       \
    }                                                               \
                                                                    \
    (_effective_addr2) += disp2;                                    \
                                                                    \
    if (( _len ))                                                   \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | '0' | b2  |       d2        | ///////// |    XOP    |    RXY-a  (presumed x2=0)
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RXYX_DECODER( _inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_b2)              = (temp >> 12) & 0xf;                        \
    (_r1)              = (temp >> 20) & 0xf;                        \
                                                                    \
    if (( _b2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    if (( _len ))                                                   \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}
#endif /* defined( OPTION_OPTINST ) */

/*-------------------------------------------------------------------*/
/*    RS - register and storage with additional R3 or M3 field       */
/*-------------------------------------------------------------------*/
// This is z/Arch RS-a and -b formats.

#define RS(   _inst, _regs, _r1, _r3, _b2, _effective_addr2 )  RS_DECODER( _inst, _regs, _r1, _r3, _b2, _effective_addr2, 4, 4 )
#define RS_B( _inst, _regs, _r1, _r3, _b2, _effective_addr2 )  RS_DECODER( _inst, _regs, _r1, _r3, _b2, _effective_addr2, 0, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | r3  | b2  |       d2        |    RS-a
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | m3  | b2  |       d2        |    RS-b
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RS_DECODER( _inst, _regs, _r1, _r3, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_b2)              = (temp >> 12) & 0xf;                        \
    (_r3)              = (temp >> 16) & 0xf;                        \
    (_r1)              = (temp >> 20) & 0xf;                        \
                                                                    \
    if (( _b2 ))                                                    \
    {                                                               \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
        if (( _len ))                                               \
            (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));       \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

#if defined( OPTION_OPTINST )
/*-------------------------------------------------------------------*/
/*          Optimized RS decoder without M3 calculation              */
/*-------------------------------------------------------------------*/
// This is z/Arch RS-b format but without extracting the m3 field.

#define RSMX( _inst, _regs, _r1, _b2, _effective_addr2 )  RSMX_DECODER( _inst, _regs, _r1, _b2, _effective_addr2, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | /// | b2  |       d2        |    RS-b  (no m3)
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RSMX_DECODER( _inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_b2)              = (temp >> 12) & 0xf;                        \
/*  (_m3)              = (temp >> 16) & 0xf;   */                   \
    (_r1)              = (temp >> 20) & 0xf;                        \
                                                                    \
    if (( _b2 ))                                                    \
    {                                                               \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
        if (( _len ))                                               \
            (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));       \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*         Optimized RS decoder without R1/R3 calculation            */
/*-------------------------------------------------------------------*/
// This is z/Arch RS-a format but without extracting r1 or r3 fields.

#define RSRR( _inst, _regs, _b2, _effective_addr2 )  RSRR_DECODER( _inst, _regs, _b2, _effective_addr2, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | /// | /// | b2  |       d2        |    RS-a  (no r1, no r3)
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RSRR_DECODER( _inst, _regs, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_b2)              = (temp >> 12) & 0xf;                        \
/*  (_r3)              = (temp >> 16) & 0xf;   */                   \
/*  (_r1)              = (temp >> 20) & 0xf;   */                   \
                                                                    \
    if (( _b2 ))                                                    \
    {                                                               \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
        if (( _len ))                                               \
            (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));       \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}
#endif /* defined( OPTION_OPTINST ) */

/*-------------------------------------------------------------------*/
/* RSY - register & storage w/ext.opcode, long displ and R3/M3 field */
/*-------------------------------------------------------------------*/
// This is z/Arch RSY-a and -b formats.

#if defined( FEATURE_018_LONG_DISPL_INST_FACILITY )

#define RSY(   _inst, _regs, _r1, _r3, _b2, _effective_addr2 )  RSY_DECODER_LD( _inst, _regs, _r1, _r3, _b2, _effective_addr2, 6, 6 )
#define RSY_B( _inst, _regs, _r1, _r3, _b2, _effective_addr2 )  RSY_DECODER_LD( _inst, _regs, _r1, _r3, _b2, _effective_addr2, 0, 6 )

#else /* !defined( FEATURE_018_LONG_DISPL_INST_FACILITY ) */

#define RSY(   _inst, _regs, _r1, _r3, _b2, _effective_addr2 )  RSY_DECODER(    _inst, _regs, _r1, _r3, _b2, _effective_addr2, 6, 6 )
#define RSY_B( _inst, _regs, _r1, _r3, _b2, _effective_addr2 )  RSY_DECODER(    _inst, _regs, _r1, _r3, _b2, _effective_addr2, 0, 6 )

#endif /* defined( FEATURE_018_LONG_DISPL_INST_FACILITY ) */

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | r3  | b2  |       dl2       |    dh2    |    XOP    |    RSY-a
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | m3  | b2  |       dl2       |    dh2    |    XOP    |    RSY-b
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RSY_DECODER_LD( _inst, _regs, _r1, _r3, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    S32 disp2; U32 temp = fetch_fw( _inst );                        \
                                                                    \
    (_effective_addr2) = 0;                                         \
                                                                    \
    disp2 = (temp >>  0) & 0xfff;                                   \
    (_b2) = (temp >> 12) & 0xf;                                     \
    (_r3) = (temp >> 16) & 0xf;                                     \
    (_r1) = (temp >> 20) & 0xf;                                     \
                                                                    \
    if (( _b2 ))                                                    \
        _effective_addr2 += (_regs)->GR(( _b2 ));                   \
                                                                    \
    if (unlikely((_inst)[4]))       /* long displacement?  */       \
    {                                                               \
        disp2 |= (_inst[4] << 12);                                  \
                                                                    \
        if (disp2 & 0x80000)        /* high order bit on?  */       \
            disp2 |= 0xfff00000;    /* make disp2 negative */       \
    }                                                               \
                                                                    \
    (_effective_addr2) += disp2;                                    \
                                                                    \
    if (( _len ))                                                   \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | r3  | b2  |        d2       | ///////// |    XOP    |    RSY-a,b  (no long disp)
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RSY_DECODER( _inst, _regs, _r1, _r3, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_b2)              = (temp >> 12) & 0xf;                        \
    (_r3)              = (temp >> 16) & 0xf;                        \
    (_r1)              = (temp >> 20) & 0xf;                        \
                                                                    \
    if (( _b2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    if (( _len ))                                                   \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*  RSL - storage operand with extended op code and 4-bit L field    */
/*-------------------------------------------------------------------*/
// This is z/Arch RSL-a format.

#define RSL( _inst, _regs, _l1, _b1, _effective_addr1 )  RSL_DECODER( _inst, _regs, _l1, _b1, _effective_addr1, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | l1  | /// | b1  |       d1        | /// | /// |    XOP    |    RSL-a
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RSL_DECODER( _inst, _regs, _l1, _b1, _effective_addr1, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr1) = (temp >>  0) & 0xfff;                      \
    (_b1)              = (temp >> 12) & 0xf;                        \
    (_l1)              = (temp >> 20) & 0xf;                        \
                                                                    \
    if (( _b1 ))                                                    \
    {                                                               \
        (_effective_addr1) += (_regs)->GR(( _b1 ));                 \
        (_effective_addr1) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/* RSL_RM - register & storage w/ext.opcode, 8-bit L field, and mask */
/*-------------------------------------------------------------------*/
// This is z/Arch RSL-b format.

#define RSL_RM( _inst, _regs, _r1, _l2, _b2, _effective_addr2, _m3 )  RSL_RM_DECODER( _inst, _regs, _r1, _l2, _b2, _effective_addr2, _m3, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |    l2     | b2  |       d2        | r1  | m3  |    XOP    |    RSL-b
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RSL_RM_DECODER( _inst, _regs, _r1, _l2, _b2, _effective_addr2, _m3, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( &(_inst)[1] );                             \
                                                                    \
    (_m3)              = (temp >>  0) & 0xf;                        \
    (_r1)              = (temp >>  4) & 0xf;                        \
    (_effective_addr2) = (temp >>  8) & 0xfff;                      \
    (_b2)              = (temp >> 20) & 0xf;                        \
    (_l2)              = (temp >> 24) & 0xff;                       \
                                                                    \
    if (( _b2 ))                                                    \
    {                                                               \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*     RI - register and immediate with extended 4-bit op code       */
/*-------------------------------------------------------------------*/
// This is z/Arch RI format.

#define RI(   _inst, _regs, _r1, _xop, _i2  )  RI_DECODER( _inst, _regs, _r1, _xop, _i2,  4, 4 )
#define RI_B( _inst, _regs, _r1, _xop, _ri2 )  RI_DECODER( _inst, _regs, _r1, _xop, _ri2, 0, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |    OP     | r1  | XOP |           i2          |      RI-a
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |    OP     | r1  | XOP |          ri2          |      RI-b
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |    OP     | m1  | XOP |          ri2          |      RI-c
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define RI_DECODER( _inst, _regs, _r1, _xop, _i2, _len, _ilc )      \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_i2)  = (temp >>  0) & 0xffff;                                 \
    (_xop) = (temp >> 16) & 0xf;                                    \
    (_r1)  = (temp >> 20) & 0xf;                                    \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*  RIE - register and immediate with ext.opcode and additional R3   */
/*-------------------------------------------------------------------*/
// This is z/Arch RIE-d, -e, -g formats

#define RIE(   _inst, _regs, _r1, _r3, _i2 )  RIE_DECODER( _inst, _regs, _r1, _r3, _i2, 6, 6 )
#define RIE_B( _inst, _regs, _r1, _r3, _i2 )  RIE_DECODER( _inst, _regs, _r1, _r3, _i2, 0, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | r3  |           i2          | ///////// |    XOP    |    RIE-d
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | r3  |          ri2          | ///////// |    XOP    |    RIE-e
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | m3  |           i2          | ///////// |    XOP    |    RIE-g
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RIE_DECODER( _inst, _regs, _r1, _r3, _i2, _len, _ilc )      \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_i2) = (temp >>  0) & 0xffff;                                  \
    (_r3) = (temp >> 16) & 0xf;                                     \
    (_r1) = (temp >> 20) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*          RIE_RIM - register and immediate with mask               */
/*-------------------------------------------------------------------*/
// This is z/Arch RIE-a format.

#define RIE_RIM( _inst, _regs, _r1, _i2, _m3 )  RIE_RIM_DECODER( _inst, _regs, _r1, _i2, _m3, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | /// |           i2          | m3  | /// |    XOP    |    RIE-a
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RIE_RIM_DECODER( _inst, _regs, _r1, _i2, _m3, _len, _ilc )  \
{                                                                   \
    U32 temp = fetch_fw( &(_inst)[1] );                             \
                                                                    \
    (_m3) = (temp >>  4) & 0xf;                                     \
    (_i2) = (temp >>  8) & 0xffff;                                  \
    (_r1) = (temp >> 28) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*     RIE_RRIM_B - register to register with immediate and mask     */
/*-------------------------------------------------------------------*/
// This is z/Arch RIE-b format.

#define RIE_RRIM_B( _inst, _regs, _r1, _r2, _ri4, _m3 )  RIE_RRIM_DECODER( _inst, _regs, _r1, _r2, _ri4, _m3, 0, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | r2  |          ri4          | m3  | /// |    XOP    |    RIE-b
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RIE_RRIM_DECODER( _inst, _regs, _r1, _r2, _ri4, _m3, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( &(_inst)[1] );                             \
                                                                    \
    (_m3)  = (temp >>  4) & 0xf;                                    \
    (_ri4) = (temp >>  8) & 0xffff;                                 \
    (_r2)  = (temp >> 24) & 0xf;                                    \
    (_r1)  = (temp >> 28) & 0xf;                                    \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/* RIE_RMII_B - register and mask with longer immediate and immediate*/
/*-------------------------------------------------------------------*/
// This is z/Arch RIE-c format.

#define RIE_RMII_B( _inst, _regs, _r1, _i2, _m3, _ri4 )  RIE_RMII_DECODER( _inst, _regs, _r1, _i2, _m3, _ri4, 0, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | m3  |          ri4          |     i2    |    XOP    |    RIE-c
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RIE_RMII_DECODER( _inst, _regs, _r1, _i2, _m3, _ri4, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( &(_inst)[1] );                             \
                                                                    \
    (_i2)  = (temp >>  0) & 0xff;                                   \
    (_ri4) = (temp >>  8) & 0xffff;                                 \
    (_m3)  = (temp >> 24) & 0xf;                                    \
    (_r1)  = (temp >> 28) & 0xf;                                    \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*   RIE_RRIII - register to register with three immediate fields    */
/*-------------------------------------------------------------------*/
// This is z/Arch RIE-f format.

#define RIE_RRIII( _inst, _regs, _r1, _r2, _i3, _i4, _i5 )  RIE_RRIII_DECODER( _inst, _regs, _r1, _r2, _i3, _i4, _i5, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | r2  |     i3    |     i4    |     i5    |    XOP    |    RIE-f
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RIE_RRIII_DECODER( _inst, _regs, _r1, _r2, _i3, _i4, _i5, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( &(_inst)[1] );                             \
                                                                    \
    (_i5) = (temp >>  0) & 0xff;                                    \
    (_i4) = (temp >>  8) & 0xff;                                    \
    (_i3) = (temp >> 16) & 0xff;                                    \
    (_r2) = (temp >> 24) & 0xf;                                     \
    (_r1) = (temp >> 28) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*  RIL - register and longer immediate with extended 4 bit op code  */
/*-------------------------------------------------------------------*/
// This is z/Arch RIL-a format or RIL-b/c format for branches.

#define RIL(   _inst, _regs, _r1, _op, _i2 )  RIL_DECODER( _inst, _regs, _r1, _op, _i2, 6, 6 )
#define RIL_B( _inst, _regs, _r1, _op, _i2 )  RIL_DECODER( _inst, _regs, _r1, _op, _i2, 0, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | XOP |                       i2                      |    RIL-a
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RIL_DECODER( _inst, _regs, _r1, _xop, _i2, _len, _ilc )     \
{                                                                   \
    (_i2)  = fetch_fw( &(_inst)[2] );                               \
    (_xop) = ((_inst)[1] >> 0) & 0xf;                               \
    (_r1)  = ((_inst)[1] >> 4) & 0xf;                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*    RIL_A - register and longer immediate RELATIVE address         */
/*-------------------------------------------------------------------*/
// This is z/Arch RIL-b and -c formats.

#define RIL_A( _inst, _regs, _r1, _ri2 )  RIL_A_DECODER( _inst, _regs, _r1, _ri2, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | XOP |                      ri2                      |    RIL-b
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | m1  | XOP |                      ri2                      |    RIL-c
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RIL_A_DECODER( _inst, _regs, _r1, _ri2, _len, _ilc )        \
{                                                                   \
    S64 offset = 2LL * (S32) (fetch_fw( &(_inst)[2] ));             \
                                                                    \
    (_ri2) = likely( !(_regs)->execflag )                           \
        ? PSW_IA_FROM_IP( (_regs), offset )                         \
        : ((_regs)->ET + offset) & ADDRESS_MAXWRAP(( _regs ));      \
                                                                    \
    (_r1) = ((_inst)[1] >> 4) & 0xf;                                \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*         RIS_B - register, immediate, mask, and storage            */
/*-------------------------------------------------------------------*/
// This is z/Arch RIS format.

#define RIS_B( _inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4 )  RIS_DECODER( _inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4, 0, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | m3  | b4  |        d4       |     i2    |    XOP    |    RIS
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RIS_DECODER( _inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_i2)              = (_inst)[4];                                \
    (_effective_addr4) = (temp >>  0) & 0xfff;                      \
    (_b4)              = (temp >> 12) & 0xf;                        \
    (_m3)              = (temp >> 16) & 0xf;                        \
    (_r1)              = (temp >> 20) & 0xf;                        \
                                                                    \
    if (( _b4 ))                                                    \
    {                                                               \
        (_effective_addr4) += (_regs)->GR(( _b4 ));                 \
        (_effective_addr4) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*          RRS_B - register, register, mask, and storage            */
/*-------------------------------------------------------------------*/
// This is z/Arch RRS format.

#define RRS_B( _inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4 )  RRS_DECODER( _inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4, 0, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | r2  | b4  |        d4       | m3  | /// |    XOP    |    RRS
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define RRS_DECODER( _inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_m3)              = ((_inst)[4] >> 4) & 0xf;                   \
    (_effective_addr4) = (temp >>  0) & 0xfff;                      \
    (_b4)              = (temp >> 12) & 0xf;                        \
    (_r2)              = (temp >> 16) & 0xf;                        \
    (_r1)              = (temp >> 20) & 0xf;                        \
                                                                    \
    if (( _b4 ))                                                    \
    {                                                               \
        (_effective_addr4) += (_regs)->GR(( _b4 ));                 \
        (_effective_addr4) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*                 SI - storage and immediate                        */
/*-------------------------------------------------------------------*/
// This is z/Arch SI format.

#define SI( _inst, _regs, _i2, _b1, _effective_addr1 )  SI_DECODER( _inst, _regs, _i2, _b1, _effective_addr1, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |     i2    | b1  |        d1       |      SI
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define SI_DECODER( _inst, _regs, _i2, _b1, _effective_addr1, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr1) = (temp >>  0) & 0xfff;                      \
    (_b1)              = (temp >> 12) & 0xf;                        \
    (_i2)              = (temp >> 16) & 0xff;                       \
                                                                    \
    if (( _b1 ))                                                    \
    {                                                               \
        (_effective_addr1) += (_regs)->GR(( _b1 ));                 \
        (_effective_addr1) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

#if defined( OPTION_OPTINST )
/*-------------------------------------------------------------------*/
/*          Optimized SI decoder without i2 calculation              */
/*-------------------------------------------------------------------*/
// This is z/Arch SI format but without extracting the i2 field.

#define SIIX( _inst, _regs, _b1, _effective_addr1 )  SIIX_DECODER( _inst, _regs, _b1, _effective_addr1, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | ///////// | b1  |        d1       |     SI  (no i2)
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define SIIX_DECODER( _inst, _regs, _b1, _effective_addr1, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr1) = (temp >>  0) & 0xfff;                      \
    (_b1)              = (temp >> 12) & 0xf;                        \
/*  (_i2)              = (temp >> 16) & 0xff;   */                  \
                                                                    \
    if (( _b1 ))                                                    \
    {                                                               \
        (_effective_addr1) += (_regs)->GR(( _b1 ));                 \
        (_effective_addr1) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}
#endif /* defined( OPTION_OPTINST ) */

/*-------------------------------------------------------------------*/
/*       SIY - storage and immediate with long displacement          */
/*-------------------------------------------------------------------*/
// This is z/Arch SIY format.

#if defined( FEATURE_018_LONG_DISPL_INST_FACILITY )

#define SIY( _inst, _regs, _i2, _b1, _effective_addr1 )  SIY_DECODER_LD( _inst, _regs, _i2, _b1, _effective_addr1, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |     i2    | b1  |       dl1       |    dh1    |    XOP    |    SIY
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define SIY_DECODER_LD( _inst, _regs, _i2, _b1, _effective_addr1, _len, _ilc ) \
{                                                                   \
    S32 disp; U32 temp = fetch_fw( _inst );                         \
                                                                    \
    (_effective_addr1) = 0;                                         \
                                                                    \
    disp  = (temp >>  0) & 0xfff;                                   \
    (_b1) = (temp >> 12) & 0xf;                                     \
    (_i2) = (temp >> 16) & 0xff;                                    \
                                                                    \
    if (( _b1 ))                                                    \
        (_effective_addr1) += (_regs)->GR(( _b1 ));                 \
                                                                    \
    if (unlikely((_inst)[4]))       /* long displacement?  */       \
    {                                                               \
        disp |= (_inst[4] << 12);                                   \
                                                                    \
        if (disp & 0x80000)        /* high order bit on? */         \
            disp |= 0xfff00000;    /* make disp negative */         \
    }                                                               \
                                                                    \
    (_effective_addr1) += disp;                                     \
    (_effective_addr1) &= ADDRESS_MAXWRAP(( _regs ));               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

#endif /* defined( FEATURE_018_LONG_DISPL_INST_FACILITY ) */

/*-------------------------------------------------------------------*/
/*             SIL - storage and longer immediate                    */
/*-------------------------------------------------------------------*/
// This is z/Arch SIL format.

#define SIL( _inst, _regs, _i2, _b1, _effective_addr1 )  SIL_DECODER( _inst, _regs, _i2, _b1, _effective_addr1, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |    XOP    | b1  |        d1       |           i2          |    SIL
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define SIL_DECODER( _inst, _regs, _i2, _b1, _effective_addr1, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( &(_inst)[2] );                             \
                                                                    \
    (_i2)              = (temp >>  0) & 0xffff;                     \
    (_effective_addr1) = (temp >> 16) & 0xfff;                      \
    (_b1)              = (temp >> 28) & 0xf;                        \
                                                                    \
    if (( _b1 ))                                                    \
    {                                                               \
        (_effective_addr1) += (_regs)->GR(( _b1 ));                 \
        (_effective_addr1) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*    SMI_A - storage with mask and 16-bit relative address          */
/*-------------------------------------------------------------------*/
// This is z/Arch SMI format.

#define SMI_A( _inst, _regs, _m1, _addr2, _b3, _addr3 )  SMI_A_DECODER( _inst, _regs, _m1, _addr2, _b3, _addr3, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | m1  | /// | b3  |        d3       |          ri2          |    SMI
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define SMI_A_DECODER( _inst, _regs, _m1, _addr2, _b3, _addr3, _len, _ilc ) \
{                                                                   \
    S64 offset; U32 ri2;                                            \
    U32 temp = fetch_fw( &(_inst)[2] );                             \
                                                                    \
    ri2      = (temp >>  0) & 0xffff;                               \
    (_addr3) = (temp >> 16) & 0xfff;                                \
    (_b3)    = (temp >> 28) & 0xf;                                  \
    (_m1)    = ((_inst)[1] >> 4) & 0xf;                             \
                                                                    \
    if (( _b3 ))                                                    \
    {                                                               \
        (_addr3) += (_regs)->GR(( _b3 ));                           \
        (_addr3) &= ADDRESS_MAXWRAP(( _regs ));                     \
    }                                                               \
                                                                    \
    offset = 2LL * (S32) ri2;                                       \
                                                                    \
    (_addr2) = likely( !(_regs)->execflag )                         \
            ? PSW_IA_FROM_IP( (_regs), offset )                     \
            : ((_regs)->ET + offset) & ADDRESS_MAXWRAP(( _regs ));  \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*                 S - storage operand only                          */
/*-------------------------------------------------------------------*/
// This is z/Arch S format.

#define S( _inst, _regs, _b2, _effective_addr2 )  S_DECODER( _inst, _regs, _b2, _effective_addr2, 4, 4 )

//  0           1           2           3           4
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |    XOP    | b2  |       d2        |     S
//  +-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28   31

#define S_DECODER( _inst, _regs, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( _inst );                                   \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_b2)              = (temp >> 12) & 0xf;                        \
                                                                    \
    if (( _b2 ))                                                    \
    {                                                               \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       SS - storage to storage with two 4-bit L or R fields        */
/*-------------------------------------------------------------------*/
// This is z/Arch SS-b to -e formats.

#define SS( _inst, _regs, _r1, _r3, _b1, _effective_addr1, _b2, _effective_addr2 )  SS_DECODER( _inst, _regs, _r1, _r3, _b1, _effective_addr1, _b2, _effective_addr2, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | r3  | b1  |       d1        | b2  |       d2        |    SS-b .. SS-e
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define SS_DECODER( _inst, _regs, _r1, _r3, _b1, _effective_addr1, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 2 );                             \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_b2)              = (temp >> 12) & 0xf;                        \
    (_effective_addr1) = (temp >> 16) & 0xfff;                      \
    (_b1)              = (temp >> 28) & 0xf;                        \
    (_r3)              = ((_inst)[1] >> 0) & 0xf;                   \
    (_r1)              = ((_inst)[1] >> 4) & 0xf;                   \
                                                                    \
    if (( _b1 ))                                                    \
    {                                                               \
        (_effective_addr1) += (_regs)->GR(( _b1 ));                 \
        (_effective_addr1) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    if (( _b2 ))                                                    \
    {                                                               \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*        SS_L - storage to storage with one 8-bit L field           */
/*-------------------------------------------------------------------*/
// This is z/Arch SS-a or -f formats.

#define SS_L( _inst, _regs, _l, _b1, _effective_addr1, _b2, _effective_addr2 )  SS_L_DECODER( _inst, _regs, _l, _b1, _effective_addr1, _b2, _effective_addr2, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |     L     | b1  |        d1       | b2  |       d2        |    SS-a, SS-f
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define SS_L_DECODER(_inst, _regs, _l, _b1, _effective_addr1, _b2, _effective_addr2, _len, _ilc) \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 2 );                             \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_b2)              = (temp >> 12) & 0xf;                        \
    (_effective_addr1) = (temp >> 16) & 0xfff;                      \
    (_b1)              = (temp >> 28) & 0xf;                        \
    (_l)               = (_inst)[1];                                \
                                                                    \
    if (( _b1 ))                                                    \
    {                                                               \
        (_effective_addr1) += (_regs)->GR(( _b1 ));                 \
        (_effective_addr1) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    if (( _b2 ))                                                    \
    {                                                               \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*          SSE - storage to storage with extended op code           */
/*-------------------------------------------------------------------*/
// This is z/Arch SSE format.

#define SSE( _inst, _regs, _b1, _effective_addr1, _b2, _effective_addr2 )  SSE_DECODER( _inst, _regs, _b1, _effective_addr1, _b2, _effective_addr2, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    |    XOP    | b1  |       d1        | b2  |       d2        |    SSE
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define SSE_DECODER( _inst, _regs, _b1, _effective_addr1, _b2, _effective_addr2, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 2 );                             \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_b2)              = (temp >> 12) & 0xf;                        \
    (_effective_addr1) = (temp >> 16) & 0xfff;                      \
    (_b1)              = (temp >> 28) & 0xf;                        \
                                                                    \
    if (( _b1 ))                                                    \
    {                                                               \
        (_effective_addr1) += (_regs)->GR(( _b1 ));                 \
        (_effective_addr1) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    if (( _b2 ))                                                    \
    {                                                               \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       SSF - storage to storage with additional register           */
/*-------------------------------------------------------------------*/
// This is z/Arch SSF format.

#define SSF( _inst, _regs, _b1, _effective_addr1, _b2, _effective_addr2, _r3 )  SSF_DECODER( _inst, _regs, _b1, _effective_addr1, _b2, _effective_addr2, _r3, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r3  | XOP | b1  |       d1        | b2  |       d2        |    SSF
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40    44   47

#define SSF_DECODER( _inst, _regs, _b1, _effective_addr1, _b2, _effective_addr2, _r3, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 2 );                             \
                                                                    \
    (_r3) = ((_inst)[1] >> 4) & 0xf;                                \
                                                                    \
    (_effective_addr2) = (temp >>  0) & 0xfff;                      \
    (_b2)              = (temp >> 12) & 0xf;                        \
    (_effective_addr1) = (temp >> 16) & 0xfff;                      \
    (_b1)              = (temp >> 28) & 0xf;                        \
                                                                    \
    if (( _b1 ))                                                    \
    {                                                               \
        (_effective_addr1) += (_regs)->GR(( _b1 ));                 \
        (_effective_addr1) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    if (( _b2 ))                                                    \
    {                                                               \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
        (_effective_addr2) &= ADDRESS_MAXWRAP(( _regs ));           \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*********************************************************************/
/*********************************************************************/
/**                                                                 **/
/**                 370/390 Vector Facility                         **/
/**                                                                 **/
/*********************************************************************/
/*********************************************************************/

#if defined( FEATURE_S370_S390_VECTOR_FACILITY )

#define VS( _inst, _regs, _rs2 )                                    \
{                                                                   \
    (_rs2) = (_inst)[3] & 0x0F;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), 4, 4 );                               \
}

/* S format instructions where the effective address does not wrap */

#define S_NW( _inst, _regs, _b2, _effective_addr2 )                 \
{                                                                   \
    (_b2) = (_inst)[2] >> 4;                                        \
                                                                    \
    (_effective_addr2) = (((_inst)[2] & 0x0F) << 8) | (_inst)[3];   \
                                                                    \
    if ((_b2) != 0)                                                 \
    {                                                               \
        (_effective_addr2) += (_regs)->GR((_b2));                   \
    }                                                               \
                                                                    \
    INST_UPDATE_PSW( (_regs), 4, 4 );                               \
}

#endif /* defined( FEATURE_S370_S390_VECTOR_FACILITY ) */
/*********************************************************************/
/*********************************************************************/
/**                                                                 **/
/**                 zVector Facility                                **/
/**                                                                 **/
/*********************************************************************/
/*********************************************************************/

#if defined( FEATURE_129_ZVECTOR_FACILITY )
/*-------------------------------------------------------------------*/
/*       VRI_A - vector register-and-immediate operation             */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRI_A( _inst, _regs, _v1, _i2, _m3 )  VRI_A_DECODER( _inst, _regs, _v1, _i2, _m3, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | /// |           i2          | m3  | rxb |    XOP    |    VRI_A
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16                      32    36    40         47

#define VRI_A_DECODER( _inst, _regs, _v1, _i2, _m3, _len, _ilc )    \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_i2) = (temp >> 8) & 0xffff;                                   \
    (_m3) = (temp >>  4) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       VRI_B - vector register-and-immediate operation             */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRI_B( _inst, _regs, _v1, _i2, _i3, _m4 )  VRI_B_DECODER( _inst, _regs, _v1, _i2, _i3, _m4, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | /// |     i2    |     i3    | m4  | rxb |    XOP    |    VRI_B
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16          24          32    36    40         47

#define VRI_B_DECODER( _inst, _regs, _v1, _i2, _i3, _m4, _len, _ilc )    \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_i2) = (temp >> 16) & 0xff;                                    \
    (_i3) = (temp >>  8) & 0xff;                                    \
    (_m4) = (temp >>  4) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       VRI_C - vector register-and-immediate operation             */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRI_C( _inst, _regs, _v1, _v3, _i2, _m4 )  VRI_C_DECODER( _inst, _regs, _v1, _v3, _i2, _m4, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | v3  |           i2          | m4  | rxb |    XOP    |    VRI_C
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16                      32    36    40         47

#define VRI_C_DECODER( _inst, _regs, _v1, _v3, _i2, _m4, _len, _ilc )    \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_v3) = ((temp >> 24) & 0xf) | ((_rxb & 0x4) << 2);             \
    (_i2) = (temp >> 8) & 0xffff;                                   \
    (_m4) = (temp >>  4) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       VRI_D - vector register-and-immediate operation             */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRI_D( _inst, _regs, _v1, _v2, _v3, _i4, _m5 )  VRI_D_DECODER( _inst, _regs, _v1, _v2, _v3, _i4, _m5, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | v2  | v3  | /// |     i4    | m5  | rxb |    XOP    |    VRI_D
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24          32    36    40         47

#define VRI_D_DECODER( _inst, _regs, _v1, _v2, _v3, _i4, _m5, _len, _ilc )    \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_v2) = ((temp >> 24) & 0xf) | ((_rxb & 0x4) << 2);             \
    (_v3) = ((temp >> 20) & 0xf) | ((_rxb & 0x2) << 3);             \
    (_i4) = (temp >> 8) & 0xff;                                     \
    (_m5) = (temp >>  4) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       VRI_E - vector register-and-immediate operation             */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRI_E( _inst, _regs, _v1, _v2, _i3, _m4, _m5 )  VRI_E_DECODER( _inst, _regs, _v1, _v2, _i3, _m4, _m5, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | v2  |        i3       | m5  | m4  | rxb |    XOP    |    VRI_E
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16                28    32    36    40         47

#define VRI_E_DECODER( _inst, _regs, _v1, _v2, _i3, _m4, _m5, _len, _ilc )    \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_v2) = ((temp >> 24) & 0xf) | ((_rxb & 0x4) << 2);             \
    (_i3) = (temp >> 12) & 0x0fff;                                  \
    (_m4) = (temp >>  4) & 0xf;                                     \
    (_m5) = (temp >>  8) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       VRR_A - vector register-and-immediate operation             */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRR_A( _inst, _regs, _v1, _v2, _m3, _m4, _m5 )  VRR_A_DECODER( _inst, _regs, _v1, _v2, _m3, _m4, _m5, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | v2  | ///////// | m5  | m4  | m3  | rxb |    XOP    |    VRR_A
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16          24    28    32    36    40         47

#define VRR_A_DECODER( _inst, _regs, _v1, _v2, _m3, _m4, _m5, _len, _ilc )    \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_v2) = ((temp >> 24) & 0xf) | ((_rxb & 0x4) << 2);             \
    (_m3) = (temp >> 4) & 0xf;                                      \
    (_m4) = (temp >> 8) & 0xf;                                      \
    (_m5) = (temp >> 12) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       VRR_B - vector register-and-immediate operation             */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRR_B( _inst, _regs, _v1, _v2, _v3, _m4, _m5 )  VRR_B_DECODER( _inst, _regs, _v1, _v2, _v3, _m4, _m5, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | v2  | v3  | /// | m5  | /// | m4  | rxb |    XOP    |    VRR_B
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40         47

#define VRR_B_DECODER( _inst, _regs, _v1, _v2, _v3, _m4, _m5, _len, _ilc )    \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_v2) = ((temp >> 24) & 0xf) | ((_rxb & 0x4) << 2);             \
    (_v3) = ((temp >> 20) & 0xf) | ((_rxb & 0x2) << 3);             \
    (_m4) = (temp >> 4) & 0xf;                                      \
    (_m5) = (temp >> 12) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}


/*-------------------------------------------------------------------*/
/*       VRR_C - vector register-and-immediate operation             */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRR_C( _inst, _regs, _v1, _v2, _v3, _m4, _m5, _m6 )  VRR_C_DECODER( _inst, _regs, _v1, _v2, _v3, _m4, _m5, _m6, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | v2  | v3  | /// | m6  | m5  | m4  | rxb |    XOP    |    VRR_C
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40         47

#define VRR_C_DECODER( _inst, _regs, _v1, _v2, _v3, _m4, _m5, _m6, _len, _ilc )    \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_v2) = ((temp >> 24) & 0xf) | ((_rxb & 0x4) << 2);             \
    (_v3) = ((temp >> 20) & 0xf) | ((_rxb & 0x2) << 3);             \
    (_m4) = (temp >> 4) & 0xf;                                      \
    (_m5) = (temp >> 8) & 0xf;                                      \
    (_m6) = (temp >> 12) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}


/*-------------------------------------------------------------------*/
/*       VRR_D - vector register-and-immediate operation             */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRR_D( _inst, _regs, _v1, _v2, _v3, _v4, _m5, _m6 )  VRR_D_DECODER( _inst, _regs, _v1, _v2, _v3, _v4, _m5, _m6, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | v2  | v3  | m5  | m6  | /// | v4  | rxb |    XOP    |    VRR_D
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40         47

#define VRR_D_DECODER( _inst, _regs, _v1, _v2, _v3, _v4, _m5, _m6, _len, _ilc )    \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_v2) = ((temp >> 24) & 0xf) | ((_rxb & 0x4) << 2);             \
    (_v3) = ((temp >> 20) & 0xf) | ((_rxb & 0x2) << 3);             \
    (_v4) = ((temp >>  4) & 0xf) | ((_rxb & 0x1) << 4);             \
    (_m5) = (temp >> 16) & 0xf;                                     \
    (_m6) = (temp >> 12) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       VRR_E - vector register-and-immediate operation             */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRR_E( _inst, _regs, _v1, _v2, _v3, _v4, _m5, _m6 )  VRR_E_DECODER( _inst, _regs, _v1, _v2, _v3, _v4, _m5, _m6, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | v2  | v3  | m6  | /// | m5  | v4  | rxb |    XOP    |    VRR_E
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40         47

#define VRR_E_DECODER( _inst, _regs, _v1, _v2, _v3, _v4, _m5, _m6, _len, _ilc )    \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_v2) = ((temp >> 24) & 0xf) | ((_rxb & 0x4) << 2);             \
    (_v3) = ((temp >> 20) & 0xf) | ((_rxb & 0x2) << 3);             \
    (_v4) = ((temp >>  4) & 0xf) | ((_rxb & 0x1) << 4);             \
    (_m5) = (temp >>  8) & 0xf;                                     \
    (_m6) = (temp >> 16) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       VRR_F - vector register-and-immediate operation             */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRR_F( _inst, _regs, _v1, _r2, _r3 )  VRR_F_DECODER( _inst, _regs, _v1, _r2, _r3, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | r2  | r3  | /// | /// | /// | /// | rxb |    XOP    |    VRR_F
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40         47

#define VRR_F_DECODER( _inst, _regs, _v1, _r2, _r3, _len, _ilc )    \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_r2) = (temp >>  24) & 0xf;                                    \
    (_r3) = (temp >>  20) & 0xf;                                    \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       VRS_A - vector register-and-storage operation               */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRS_A( _inst, _regs, _v1, _v3, _b2, _effective_addr2, _m4 )  VRS_A_DECODER( _inst, _regs, _v1, _v3, _b2, _effective_addr2, _m4, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | v3  | b2  |       d2        | m4  | rxb |    XOP    |    VRS_A
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40         47

#define VRS_A_DECODER( _inst, _regs, _v1, _v3, _b2, _effective_addr2, _m4, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_v3) = ((temp >> 24) & 0xf) | ((_rxb & 0x4) << 2);             \
    (_b2) = (temp >> 20) & 0xf;                                     \
    (_effective_addr2) = (temp >>  8) & 0xfff;                      \
    (_m4) = (temp >>  4) & 0xf;                                     \
                                                                    \
    if (( _b2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    (_effective_addr2) &= ADDRESS_MAXWRAP((_regs));                 \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       VRS_B - vector register-and-storage operation               */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRS_B( _inst, _regs, _v1, _r3, _b2, _effective_addr2, _m4 )  VRS_B_DECODER( _inst, _regs, _v1, _r3, _b2, _effective_addr2, _m4, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | r3  | b2  |       d2        | m4  | rxb |    XOP    |    VRS_B
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40         47

#define VRS_B_DECODER( _inst, _regs, _v1, _r3, _b2, _effective_addr2, _m4, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_r3) = (temp >> 24) & 0xf;                                     \
    (_b2) = (temp >> 20) & 0xf;                                     \
    (_effective_addr2) = (temp >>  8) & 0xfff;                      \
    (_m4) = (temp >>  4) & 0xf;                                     \
                                                                    \
    if (( _b2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    (_effective_addr2) &= ADDRESS_MAXWRAP((_regs));                 \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       VRS_C - vector register-and-storage operation               */
/*               and an extended opcode field.                       */
/*-------------------------------------------------------------------*/

#define VRS_C( _inst, _regs, _r1, _v3, _b2, _effective_addr2, _m4 )  VRS_C_DECODER( _inst, _regs, _r1, _v3, _b2, _effective_addr2, _m4, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | r1  | v3  | b2  |       d2        | m4  | rxb |    XOP    |    VRS_C
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40         47

#define VRS_C_DECODER( _inst, _regs, _r1, _v3, _b2, _effective_addr2, _m4, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_r1) = (temp >> 28) & 0xf;                                     \
    (_v3) = ((temp >> 24) & 0xf) | ((_rxb & 0x4) << 2);             \
    (_b2) = (temp >> 20) & 0xf;                                     \
    (_effective_addr2) = (temp >>  8) & 0xfff;                      \
    (_m4) = (temp >>  4) & 0xf;                                     \
                                                                    \
    if (( _b2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    (_effective_addr2) &= ADDRESS_MAXWRAP((_regs));                 \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

/*-------------------------------------------------------------------*/
/*       VRV - vector register-and-vector-indexstorage               */
/*             operation and an extended op - code field.            */
/*-------------------------------------------------------------------*/

#define VRV( _inst, _regs, _v1, _v2, _b2, _d2, _m3 )  VRV_DECODER( _inst, _regs, _v1, _v2, _b2, _d2, _m3, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | v2  | b2  |       d2        | m3  | rxb |    XOP    |    VRV
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40         47

#define VRV_DECODER( _inst, _regs, _v1, _v2, _b2, _d2, _m3, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_v2) = ((temp >> 24) & 0xf) | ((_rxb & 0x4) << 2);             \
    (_b2) = (temp >> 20) & 0xf;                                     \
    (_d2) = (temp >>  8) & 0xfff;                                   \
    (_m3) = (temp >>  4) & 0xf;                                     \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}


/*-------------------------------------------------------------------*/
/*       VRX - vector register-and-index-storage operation           */
/*             and an extended op-code field.                        */
/*-------------------------------------------------------------------*/

#define VRX( _inst, _regs, _v1, _x2, _b2, _effective_addr2, _m3 )  VRX_DECODER( _inst, _regs, _v1, _x2, _b2, _effective_addr2, _m3, 6, 6 )

//  0           1           2           3           4           5           6
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  |     OP    | v1  | x2  | b2  |       d2        | m3  | rxb |    XOP    |    VRX
//  +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//  0     4     8     12    16    20    24    28    32    36    40         47

#define VRX_DECODER( _inst, _regs, _v1, _x2, _b2, _effective_addr2, _m3, _len, _ilc ) \
{                                                                   \
    U32 temp = fetch_fw( (_inst) + 1);                              \
                                                                    \
    U32 _rxb = (temp >> 0) & 0xf;                                   \
    (_v1) = ((temp >> 28) & 0xf) | ((_rxb & 0x8) << 1);             \
    (_x2) = (temp >> 24) & 0xf;                                     \
    (_b2) = (temp >> 20) & 0xf;                                     \
    (_effective_addr2) = (temp >>  8) & 0xfff;                      \
    (_m3) = (temp >>  4) & 0xf;                                     \
                                                                    \
    if (( _x2 ))                                                    \
        (_effective_addr2) += (_regs)->GR((_x2));                   \
                                                                    \
    if (( _b2 ))                                                    \
        (_effective_addr2) += (_regs)->GR(( _b2 ));                 \
                                                                    \
    (_effective_addr2) &= ADDRESS_MAXWRAP((_regs));                 \
                                                                    \
    INST_UPDATE_PSW( (_regs), (_len), (_ilc) );                     \
}

#endif /* defined( FEATURE_129_ZVECTOR_FACILITY ) */