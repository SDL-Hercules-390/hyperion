  TITLE 'bfp-009-cvtfrlog64.asm: Test IEEE Cvt From Fixed (uint-64)'
***********************************************************************
*
*Testcase IEEE CONVERT FROM LOGICAL 64
*  Test case capability includes ieee exceptions trappable and otherwise.
*  Test result, FPC flags, and DXC saved for all tests.  (Convert From 
*  Logical does not set the condition code.)
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*                       bfp-009-cvtfrlog64.asm 
*
*        This assembly-language source file is part of the
*        Hercules Decimal Floating Point Validation Package 
*                        by Stephen R. Orso
*
* Copyright 2016 by Stephen R Orso.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in
*    the documentation and/or other materials provided  with the
*    distribution.
*
* 3. The name of the author may not be used to endorse or promote
*    products derived from this software without specific prior written
*    permission.
*
* DISCLAMER: THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
* HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
* Tests the following three conversion instructions
*   CONVERT FROM LOGICAL (64 to short BFP, RRF-e)
*   CONVERT FROM LOGICAL (64 to long BFP, RRF-e) 
*   CONVERT FROM LOGICAL (64 to extended BFP, RRF-e)  
*
* Limited test data is compiled into this program.  The test script
* that runs this program can provide alternative test data through
* Hercules R commands.
* 
* Test Case Order
* 1) Uint-64 to Short BFP
* 2) Uint-64 to Short BFP with all rounding modes
* 3) Uint-64 to Long BFP
* 4) Uint-64 to Long BFP with all rounding modes
* 5) Uint-64 to Extended BFP
*
* Provided test data is: 
*        1, 2, 4,
*        9 007 199 254 740 991(0x001FFFFFFFFFFFFF)
*       18 014 398 509 481 983(0x003FFFFFFFFFFFFF)
*   18 446 744 073 709 551 615 (0xFFFFFFFFFFFFFFFF)
*
*   The fourth value oveflows a short BFP but fits in a long BFP.
*   The fifth  value oveflows both short BFP and long BFP.  The
*   last value also overflows both, but fits in an extended BFP.
*
* Also tests the following floating point support instructions
*   LOAD  (Short)
*   LOAD  (Long)
*   LOAD FPC
*   SET BFP ROUNDING MODE 2-BIT
*   SET BFP ROUNDING MODE 3-BIT
*   STORE (Short)
*   STORE (Long)
*   STORE FPC
*
***********************************************************************
         SPACE 3
         SPACE 2
         MACRO
         PADCSECT &ENDLABL
.*
.*  Macro to pad the CSECT to include result data areas if this test
.*  program is not being assembled using asma.  asma generates a core
.*  image that is loaded by the loadcore command, and because the 
.*  core image is a binary stored in Github, it makes sense to make
.*  this small effort to keep the core image small.  
.*
         AIF   (D'&ENDLABL).GOODPAD
         MNOTE 4,'Missing or invalid CSECT padding label ''&ENDLABL'''
         MNOTE *,'No CSECT padding performed'  
         MEXIT
.*
.GOODPAD ANOP            Label valid.  See if we're on asma
         AIF   ('&SYSASM' EQ 'A SMALL MAINFRAME ASSEMBLER').NOPAD
         ORG   &ENDLABL-1   Not ASMA.  Pad CSECT
         MEXIT
.*
.NOPAD   ANOP
         MNOTE *,'asma detected; no CSECT padding performed'  
         MEND
*
*  Note: for compatibility with the z/CMS test rig, do not change
*  or use R11, R14, or R15.  Everything else is fair game.  
*
         SPACE 3
BFPCVTFL START 0
STRTLABL EQU   *
R0       EQU   0                   Work register for cc extraction
R1       EQU   1
R2       EQU   2                   Holds count of test input values
R3       EQU   3                   Points to next test input value(s)
R4       EQU   4                   Available
R5       EQU   5                   Available
R6       EQU   6                   Available
R7       EQU   7                   Pointer to next result value(s)
R8       EQU   8                   Pointer to next FPCR result
R9       EQU   9                   Rounding tests top of outer loop
R10      EQU   10                  Pointer to test address list
R11      EQU   11                  **Reserved for z/CMS test rig
R12      EQU   12                  Holds number of test cases in set
R13      EQU   13                  Mainline return address
R14      EQU   14                  **Return address for z/CMS test rig
R15      EQU   15                  **Base register on z/CMS or Hyperion
*
* Floating Point Register equates to keep the cross reference clean
*
FPR0     EQU   0
FPR1     EQU   1
FPR2     EQU   2
FPR3     EQU   3
FPR4     EQU   4
FPR5     EQU   5
FPR6     EQU   6
FPR7     EQU   7
FPR8     EQU   8
FPR9     EQU   9
FPR10    EQU   10
FPR11    EQU   11
FPR12    EQU   12
FPR13    EQU   13
FPR14    EQU   14
FPR15    EQU   15
*
         USING *,R15
*
* Above works on real iron (R15=0 after sysclear) 
* and in z/CMS (R15 points to start of load module)
*
         ORG   STRTLABL+X'8E'      Program check interrution code
PCINTCD  DS    H
*
PCOLDPSW EQU   STRTLABL+X'150'     z/Arch Program check old PSW
*
         ORG   STRTLABL+X'1A0'     z/Arch Restart PSW
         DC    X'0000000180000000',AD(START)   
*
         ORG   STRTLABL+X'1D0'     z/Arch Program check old PSW
         DC    X'0000000000000000',AD(PROGCHK)
* 
* Program check routine.  If Data Exception, continue execution at
* the instruction following the program check.  Otherwise, hard wait.
* No need to collect data.  All interesting DXC stuff is captured
* in the FPCR.
*
         ORG   STRTLABL+X'200'
PROGCHK  DS    0H             Program check occured...
         CLI   PCINTCD+1,X'07'  Data Exception?
         JNE   PCNOTDTA       ..no, hardwait (not sure if R15 is ok)
         LPSWE PCOLDPSW       ..yes, resume program execution
PCNOTDTA DS    0H
         LTR   R14,R14        Return address provided?
         BNZR  R14            Yes, return to z/CMS test rig.  
         LPSWE HARDWAIT       Not data exception, enter disabled wait
         EJECT
***********************************************************************
*
*  Main program.  Enable Advanced Floating Point, process test cases.
*         
***********************************************************************
         SPACE 2
START    STCTL R0,R0,CTLR0    Store CR0 to enable AFP
         OI    CTLR0+1,X'04'  Turn on AFP bit
         LCTL  R0,R0,CTLR0    Reload updated CR0
*
         LA    R10,SHORTS     Point to uint-64 test inputs
         BAS   R13,CELGBR     Convert values from fixed to short BFP
         LA    R10,RMSHORTS   Point to inputs for rounding mode tests
         BAS   R13,CELGBRA    Convert using all rounding mode options
*
         LA    R10,LONGS      Point to uint-64 test inputs
         BAS   R13,CDLGBR     Convert values from fixed to long
         LA    R10,RMLONGS    Point to inputs for rounding mode tests
         BAS   R13,CDLGBRA    Convert using all rounding mode options
*
         LA    R10,EXTDS      Point to uint-64 test inputs
         BAS   R13,CXLGBR     Convert values from fixed to extended
*
* uint-64 always fits in extended BFP exactly.  No rounding nor 
* loss of precision, so no need for exhaustive rounding tests
*
         LTR   R14,R14        Return address provided?
         BNZR  R14            ..Yes, return to z/CMS test rig.  
         LPSWE WAITPSW        All done
*
         DS    0D             Ensure correct alignment for psw
WAITPSW  DC    X'0002000000000000',AD(0)  Normal end - disabled wait
HARDWAIT DC    X'0002000000000000',XL6'00',X'DEAD' Abnormal end
*
CTLR0    DS    F
FPCREGNT DC    X'00000000'  FPCR, trap all IEEE exceptions, zero flags
FPCREGTR DC    X'F8000000'  FPCR, trap no IEEE exceptions, zero flags
*
* Input values parameter list, four fullwords: 
*      1) Count, 
*      2) Address of inputs, 
*      3) Address to place results, and
*      4) Address to place DXC/Flags/cc values.  
*
         ORG   STRTLABL+X'300'
SHORTS   DS    0F
         DC    A(INTCOUNT/8)
         DC    A(INTIN)
         DC    A(SBFPOUT)
         DC    A(SBFPFLGS)
*
LONGS    DS    0F           uint-64 inputs for long BFP testing
         DC    A(INTCOUNT/8)
         DC    A(INTIN)
         DC    A(LBFPOUT)
         DC    A(LBFPFLGS)
*
EXTDS    DS    0F           uint-64 inputs for Extended BFP testing
         DC    A(INTCOUNT/8)
         DC    A(INTIN)
         DC    A(XBFPOUT)
         DC    A(XBFPFLGS)
*
RMSHORTS DC    A(SINTRMCT/8)
         DC    A(SINTRMIN)
         DC    A(SBFPRMO)   Space for rounding mode tests
         DC    A(SBFPRMOF)  Space for rounding mode test flags
*
RMLONGS  DC    A(LINTRMCT/8)
         DC    A(LINTRMIN)  Last two uint-64 are only concerns
         DC    A(LBFPRMO)   Space for rounding mode tests
         DC    A(LBFPRMOF)  Space for rounding mode test flags
         EJECT
***********************************************************************
*
* Convert integers to short BFP format.  A pair of results is generated
* for each input: one with all exceptions non-trappable, and the second 
* with all exceptions trappable.   The FPCR is stored for each result.
*
***********************************************************************
         SPACE 2
CELGBR   LM    R2,R3,0(R10)  Get count and address of test input values
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         BASR  R12,0         Set top of loop
*
         LG    R1,0(,R3)     Get integer test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CELGBR FPR8,0,R1,0  Cvt uint in GPR1 to float in FPR8
         STE   FPR8,0(,R7)   Store short BFP result
         STFPC 0(R8)         Store resulting FPC flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         CELGBR FPR8,0,R1,0  Cvt uint in GPR1 to float in FPR8
         STE   FPR8,4(,R7)   Store short BFP result
         STFPC 4(R8)         Store resulting FPC flags and DXC
         LA    R3,8(,R3)     Point to next input values
         LA    R7,8(,R7)     Point to next short BFP converted values
         LA    R8,8(,R8)     Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert uint-64 to short BFP format using every rounding mode.
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPCR with 
* the IEEE Inexact exception supressed.  SRNM (2-bit) is used  for the 
* first two FPCR-controlled tests and SRNMB (3-bit) is used for the 
* last two To get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section; 
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
CELGBRA  LM    R2,R3,0(R10)  Get count and address of test input values
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         BASR  R12,0         Set top of loop
*
         LG    R1,0(,R3)     Get uint-64 test value
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 1             SET FPC to RZ, towards zero.  
         CELGBR FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,0*4(,R7) Store short BFP result
         STFPC 0(R8)         Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 2             SET FPC to RP, to +infinity
         CELGBR FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,1*4(,R7) Store short BFP result
         STFPC 1*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPC to RM, to -infinity
         CELGBR FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,2*4(,R7) Store short BFP result
         STFPC 2*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RFS, Prepare for Shorter Precision
         CELGBR FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,3*4(,R7) Store short BFP result
         STFPC 3*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CELGBR FPR8,1,R1,B'0000'  RNTA, to nearest, ties away
         STE   FPR8,4*4(,R7) Store short BFP result
         STFPC 4*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CELGBR FPR8,3,R1,B'0000'  RFS, prepare for shorter precision
         STE   FPR8,5*4(,R7) Store short BFP result
         STFPC 5*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CELGBR FPR8,4,R1,B'0000'  RNTE, to nearest, ties to even
         STE   FPR8,6*4(,R7) Store short BFP result
         STFPC 6*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CELGBR FPR8,5,R1,B'0000'  RZ, toward zero
         STE   FPR8,7*4(,R7) Store short BFP result
         STFPC 7*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CELGBR FPR8,6,R1,B'0000'  RP, to +inf
         STE   FPR8,8*4(,R7) Store short BFP result
         STFPC 8*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CELGBR FPR8,7,R1,B'0000'  RM, to -inf
         STE   FPR8,9*4(,R7) Store short BFP result
         STFPC 9*4(R8)       Store resulting FPC flags and DXC
*
         LA    R3,8(,R3)     Point to next input values
         LA    R7,12*4(,R7)  Point to next short BFP converted values
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert integers to long BFP format.  A pair of results is generated
* for each input: one with all exceptions non-trappable, and the second 
* with all exceptions trappable.   The FPCR is stored for each result.
* Conversion of a 64-bit integer to long is always exact; no exceptions
* are expected
*
***********************************************************************
         SPACE 2
CDLGBR   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LG    R1,0(,R3)     Get integer test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CDLGBR FPR8,0,R1,0  Cvt uint in GPR1 to float in FPR8
         STD   FPR8,0(,R7)   Store long BFP result
         STFPC 0(R8)         Store resulting FPC flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         CDLGBR FPR8,0,R1,0  Cvt uint in GPR1 to float in FPR8
         STD   FPR8,8(,R7)   Store long BFP result
         STFPC 4(R8)         Store resulting FPC flags and DXC
         LA    R3,8(,R3)     Point to next input value
         LA    R7,16(,R7)    Point to next long BFP result pair
         LA    R8,8(,R8)     Point to next FPCR/CC contents pair
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert uint-64 to short BFP format using every rounding mode.
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPCR with 
* the IEEE Inexact exception supressed.  SRNM (2-bit) is used  for the 
* first two FPCR-controlled tests and SRNMB (3-bit) is used for the 
* last two To get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section; 
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
CDLGBRA  LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LG    R1,0(,R3)     Get uint-64 test value
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 1             SET FPC to RZ, towards zero.  
         CDLGBR FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,0*8(,R7) Store short BFP result
         STFPC 0(R8)         Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 2             SET FPC to RP, to +infinity
         CDLGBR FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,1*8(,R7) Store short BFP result
         STFPC 1*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPC to RM, to -infinity
         CDLGBR FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,2*8(,R7) Store short BFP result
         STFPC 2*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RFS, Prepare for Shorter Precision
         CDLGBR FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,3*8(,R7) Store short BFP result
         STFPC 3*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CDLGBR FPR8,1,R1,B'0000'  RNTA, to nearest, ties away
         STD   FPR8,4*8(,R7) Store short BFP result
         STFPC 4*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CDLGBR FPR8,3,R1,B'0000'  RFS, prepare for shorter precision
         STD   FPR8,5*8(,R7) Store short BFP result
         STFPC 5*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CDLGBR FPR8,4,R1,B'0000'  RNTE, to nearest, ties to even
         STD   FPR8,6*8(,R7)  Store short BFP result
         STFPC 6*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CDLGBR FPR8,5,R1,B'0000'  RZ, toward zero
         STD   FPR8,7*8(,R7) Store short BFP result
         STFPC 7*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CDLGBR FPR8,6,R1,B'0000'  RP, to +inf
         STD   FPR8,8*8(,R7) Store short BFP result
         STFPC 8*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CDLGBR FPR8,7,R1,B'0000'  RM, to -inf
         STD   FPR8,9*8(,R7) Store short BFP result
         STFPC 9*4(R8)       Store resulting FPC flags and DXC
*
         LA    R3,8(,R3)     Point to next input values
         LA    R7,10*8(,R7)  Point to next long BFP converted values
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert integers to extended BFP format.  A pair of results is 
* generated for each input: one with all exceptions non-trappable, 
* and the second with all exceptions trappable.   The FPCR is 
* stored for each result.  Conversion of a 64-bit integer to 
* extended is always exact; no exceptions are expected
*
***********************************************************************
         SPACE 2
CXLGBR   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LG    R1,0(,R3)     Get integer test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CXLGBR FPR8,0,R1,0  Cvt uint in GPR1 to float in FPR8-FPR10
         STD   FPR8,0(,R7)   Store extended BFP result part 1
         STD   FPR10,8(,R7)  Store extended BFP result part 2
         STFPC 0(R8)         Store resulting FPC flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         CXLGBR FPR8,0,R1,0  Cvt uint in GPR1 to float in FPR8-FPR10
         STD   FPR8,16(,R7)  Store extended BFP result part 1
         STD   FPR10,24(,R7) Store extended BFP result part 2
         STFPC 4(R8)         Store resulting FPC flags and DXC
         LA    R3,8(,R3)     Point to next input value
         LA    R7,32(,R7)    Point to next extended BFP result pair
         LA    R8,8(,R8)     Point to next FPCR/CC result pair
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* long integer inputs for Convert From Fixed testing.  The same set of 
* inputs are used for short, long, and extended formats.  The last two 
* values are used for rounding mode tests for short and long only; 
* conversion of uint-64 to extended is always exact.  
*
***********************************************************************
         SPACE 2
*
* int-64 inputs for basic tests
*
INTIN    DS    0D
         DC    FD'U1'
         DC    FD'U2'
         DC    FD'U4'
         DC    XL8'FFFFFF0000000000'  Exact long and short BFP
         DC    XL8'FFFFFFFFFFFFF800'  Exact long BFP, inexact short bfp
         DC    XL8'FFFFFFFFFFFFFFFF'  Inexact short & long BFP,
*                                     ..exact extended BFP
INTCOUNT EQU   *-INTIN      Count of integers in list
*
* uint-64 inputs for exhaustive short BFP rounding mode tests
*
SINTRMIN DC    XL8'FFFFFFC000000000'  Rounds nearest up
         DC    XL8'FFFFFF8000000000'  Tie
         DC    XL8'FFFFFF4000000000'  Rounds nearest down
         DS    0F           required by asma for following EQU to work.  
SINTRMCT EQU   *-SINTRMIN   Count of integers for rounding mode tests
*
* uint-64 inputs for exhaustive long BFP rounding mode tests
*
LINTRMIN DC    XL8'FFFFFFFFFFFFFE00'  Rounds nearest up
         DC    XL8'FFFFFFFFFFFFFC00'  Tie
         DC    XL8'FFFFFFFFFFFFFA00'  Rounds nearest down
LINTRMCT EQU   *-LINTRMIN   Count of integers for rounding mode tests
*
SBFPOUT  EQU   STRTLABL+X'1000'    Short BFP values from uint-64
*                                  ..6 pairs used, room for 16 pairs
SBFPFLGS EQU   STRTLABL+X'1100'    FPCR flags and DXC from short BFP
*                                  ..6 pairs used, room for 32 pairs
SBFPRMO  EQU   STRTLABL+X'1200'    Short BFP rounding mode results
*                                  ..2 sets used, room for 16 sets
SBFPRMOF EQU   STRTLABL+X'1500'    Short BFP rounding mode FPCR 
*                                  ..2 sets used, room for 16+ sets
*                                  ..16 sets ends x'2A00'
*
LBFPOUT  EQU   STRTLABL+X'2000'    Long BFP values from uint-64
*                                  ..6 pairs used, room for 16 pairs
LBFPFLGS EQU   STRTLABL+X'2100'    FPCR flags and DXC from long BFP
*                                  ..6 pairs used, room for 32 pairs
LBFPRMO  EQU   STRTLABL+X'2200'    Long BFP rounding mode results
*                                  ..2 sets used, room for 16 sets
LBFPRMOF EQU   STRTLABL+X'2700'    Long BFP rounding mode FPCR
*                                  ..2 sets used, room for 16+ sets
*                                  ..16 sets ends x'2A00'
*
XBFPOUT  EQU   STRTLABL+X'3000'    Extended BFP values from uint-64
*                                  ..6 pairs used, room for 16 pairs
XBFPFLGS EQU   STRTLABL+X'3200'    Extended BFP rounding mode FPCR
*                                  ..6 pairs used, room for 32 pairs
*
*
ENDLABL  EQU   STRTLABL+X'3300'    Next location available for results
         PADCSECT ENDLABL
         END
