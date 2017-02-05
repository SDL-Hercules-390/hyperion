   TITLE 'bfp-010-cvtfrfix.asm: Test IEEE Convert From Fixed (int-32)'
***********************************************************************
*
*Testcase IEEE CONVERT FROM FIXED 32
*  Test case capability includes IEEE exceptions trappable and 
*  otherwise.  Test result, FPC flags, and DXC saved for all tests.  
*  Convert From Fixed does not set the condition code.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*                       bfp-010-cvtfrfix.asm 
*
*        This assembly-language source file is part of the
*        Hercules Binary Floating Point Validation Package 
*                        by Stephen R. Orso
*
* Copyright 2016 by Stephen R Orso.
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
* Tests the following six conversion instructions
*   CONVERT FROM FIXED (32 to short BFP, RRE)
*   CONVERT FROM FIXED (32 to long BFP, RRE) 
*   CONVERT FROM FIXED (32 to extended BFP, RRE)  
*   CONVERT FROM FIXED (32 to short BFP, RRF-e)
*   CONVERT FROM FIXED (32 to long BFP, RRF-e) 
*   CONVERT FROM FIXED (32 to extended BFP, RRF-e)  
*
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R 
* commands.
* 
* Test Case Order
* 1) Int-32 to Short BFP
* 2) Int-32 to Short BFP with all rounding modes
* 3) Int-32 to Long BFP
* 4) Int-32 to Extended BFP
*
* Provided test data is 1, 2, 4, -2, 2 147 483 647, -2 147 483 647.
*   The last two values will trigger inexact exceptions when converted 
*   to short BFP.  The last two values are also used to test rounding
*   mode and inexact supression in the CEFBRA instruction.  
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
BFPCVTFF START 0
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
         LA    R10,SHORTS     Point to integer test inputs
         BAS   R13,CEFBR      Convert values from fixed to short BFP
*
         LA    R10,RMSHORTS   Point to inputs for rounding mode tests
         BAS   R13,CEFBRA     Convert using all rounding mode options
*
         LA    R10,LONGS      Point to integer test inputs
         BAS   R13,CDFBR      Convert values from fixed to long BFP
*
         LA    R10,EXTDS      Point to integer test inputs
         BAS   R13,CXFBR      Convert values from fixed to extended
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
FPCREGNT DC    X'00000000'    FPC Reg IEEE exceptions Not Trappable
FPCREGTR DC    X'F8000000'    FPC Reg IEEE exceptions TRappable
*
* Input values parameter list, four fullwords: 
*      1) Count, 
*      2) Address of inputs, 
*      3) Address to place results, and
*      4) Address to place DXC/Flags/cc values.  
*
         ORG   STRTLABL+X'300'
SHORTS   DS    0F
         DC    A(INTCOUNT)
         DC    A(INTIN)
         DC    A(SBFPOUT)
         DC    A(SBFPFLGS)
*
LONGS    DS    0F           int-32 inputs for long BFP testing
         DC    A(INTCOUNT)
         DC    A(INTIN)
         DC    A(LBFPOUT)
         DC    A(LBFPFLGS)
*
EXTDS    DS    0F           int-32 inputs for Extended BFP testing
         DC    A(INTCOUNT)
         DC    A(INTIN)
         DC    A(XBFPOUT)
         DC    A(XBFPFLGS)
*
RMSHORTS DC    A(INTRMCT)
         DC    A(INTINRM)   Last two int-32 are only concerns
         DC    A(SBFPRMO)   Space for rounding mode tests
         DC    A(SBFPRMOF)  Space for rounding mode test flags
         EJECT
***********************************************************************
*
* Convert int-32 to short BFP format.  A pair of results is generated
* for each input: one with all exceptions non-trappable, and the second 
* with all exceptions trappable.   The FPCR is stored for each result.
*
***********************************************************************
         SPACE 3
CEFBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         L     R1,0(,R3)     Get integer test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CEFBR FPR8,R1       Cvt Int in GPR1 to float in FPR8
         STE   FPR8,0(,R7)   Store short BFP result
         STFPC 0(R8)         Store resulting FPC flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         CEFBR FPR8,R1       Cvt Int in GPR1 to float in FPR8
         STE   FPR8,4(,R7)   Store short BFP result
         STFPC 4(R8)         Store resulting FPC flags and DXC
         LA    R3,4(,R3)     Foint to next input values
         LA    R7,8(,R7)     Point to next short BFP converted values
         LA    R8,8(,R8)     Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert int-32 to short BFP format using each possible rounding mode.  
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPC with the 
* IEEE Inexact exception supressed.  SRNM (2-bit) is used  for the
* first two FPCR-controlled tests and SRNMB (3-bit) is used for the
* last two to get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section; 
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
CEFBRA   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         L     R1,0(,R3)     Get integer test value
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  1             SET FPCR to RZ, towards zero
         CEFBRA FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STE   FPR8,0*4(,R7) Store short BFP result
         STFPC 0(R8)         Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPCR to RP, to +infinity
         CEFBRA FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STE   FPR8,1*4(,R7) Store short BFP result
         STFPC 1*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, to -infinity
         CEFBRA FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STE   FPR8,2*4(,R7) Store short BFP result
         STFPC 2*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RFS, Prepare For Shorter Precision
         CEFBRA FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STE   FPR8,3*4(,R7) Store short BFP result
         STFPC 3*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CEFBRA FPR8,1,R1,B'0000'  RNTA, to nearest, ties away
         STE   FPR8,4*4(,R7) Store short BFP result
         STFPC 4*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CEFBRA FPR8,3,R1,B'0000'  RPS, prepare for shorter precision
         STE   FPR8,5*4(,R7) Store short BFP result
         STFPC 5*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CEFBRA FPR8,4,R1,B'0000'  RNTE to nearest, ties to even
         STE   FPR8,6*4(,R7) Store short BFP result
         STFPC 6*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CEFBRA FPR8,5,R1,B'0000'  RZ, toward zero
         STE   FPR8,7*4(,R7) Store short BFP result
         STFPC 7*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CEFBRA FPR8,6,R1,B'0000'  RP, to +inf
         STE   FPR8,8*4(,R7) Store short BFP result
         STFPC 8*4(R8)       Store resulting FPC flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CEFBRA FPR8,7,R1,B'0000'  RM, to -inf
         STE   FPR8,9*4(,R7) Store short BFP result
         STFPC 9*4(R8)       Store resulting FPC flags and DXC
*
         LA    R3,4(,R3)     Point to next input values
         LA    R7,12*4(,R7)  Point to next short BFP converted values
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert int-32 to long BFP format.  A pair of results is generated
* for each input: one with all exceptions non-trappable, and the second 
* with all exceptions trappable.   The FPCR is stored for each result.
* Conversion of a 32-bit integer to long is always exact; no exceptions
* are expected
*
***********************************************************************
         SPACE 2
CDFBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         L     R1,0(,R3)     Get integer test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CDFBR FPR8,R1       Cvt Int in GPR1 to float in FPR8
         STD   FPR8,0(,R7)   Store long BFP result
         STFPC 0(R8)         Store resulting FPC flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         CDFBR FPR8,R1       Cvt Int in GPR1 to float in FPR8
         STD   FPR8,8(,R7)   Store long BFP result
         STFPC 4(R8)         Store resulting FPC flags and DXC
*
         LA    R3,4(,R3)     Point to next input values
         LA    R7,16(,R7)    Point to next long BFP converted value
         LA    R8,8(,R8)     Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert int-32 to extended BFP format.  A pair of results is 
* generated for each input: one with all exceptions non-trappable, 
* and the second with all exceptions trappable.   The FPCR is 
* stored for each result.  Conversion of a 32-bit integer to 
* extended is always exact; no exceptions are expected
*
***********************************************************************
         SPACE 2
CXFBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         L     R1,0(,R3)     Get integer test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CXFBR FPR8,R1       Cvt Int in GPR1 to float in FPR8-FPR10
         STD   FPR8,0(,R7)   Store extended BFP result part 1
         STD   FPR10,8(,R7)  Store extended BFP result part 2
         STFPC 0(R8)         Store resulting FPC flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         CXFBR FPR8,R1       Cvt Int in GPR1 to float in FPR8-FPR10
         STD   FPR8,16(,R7)  Store extended BFP result part 1
         STD   FPR10,24(,R7) Store extended BFP result part 2
         STFPC 4(R8)         Store resulting FPC flags and DXC
*
         LA    R3,4(,R3)     Point to next input values
         LA    R7,32(,R7)    Point to next extended BFP converted value
         LA    R8,8(,R8)     Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Short integer inputs for Convert From Fixed testing.  The same set of 
* inputs are used for short, long, and extended formats.  The last two 
* values are used for rounding mode tests for short only; conversion of 
* int-32 to long or extended are always exact.  
*
***********************************************************************
         SPACE 3
INTIN    DS    0F
         DC    F'1'
         DC    F'2'
         DC    F'4'
         DC    F'-2'
         DC    F'2147483647'  should compile to X'7FFFFFFF' - inexact
         DC    F'-2147483647' should compile to X'80000001' - inexact
         DC    XL4'7FFFFF80'  Fits in short BFP
INTCOUNT EQU   (*-INTIN)/4    Count of integers in list
*
* Short BFP Exhaustive rounding mode tests.  int-32 always fits in
* long BFP or extended BFP with no loss of precision, so no basis for
* exhaustive rounding tests for long or extended
* 
INTINRM  DS    0F
         DC    XL4'7FFFFFE0'  Inexact, normally rounds up
         DC    XL4'7FFFFFC0'  Inexact, Tie
         DC    XL4'7FFFFFA0'  Inexact, normally rounds down
INTRMCT  EQU   (*-INTINRM)/4  Count of rounding mode test inputs
*
SBFPOUT  EQU   STRTLABL+X'1000'    Short BFP results from Int-32 inputs
*                                  ..6 pairs used, room for 32 pairs
SBFPFLGS EQU   STRTLABL+X'1100'    FPCR flags and DXC from short BFP
*                                  ..6 pairs used, room for 32 pairs
SBFPRMO  EQU   STRTLABL+X'1200'    Short BFP rounding mode results
*                                  ..2 sets used, room for 16
SBFPRMOF EQU   STRTLABL+X'1500'    Short BFP rndg mode FPCR contents
*                                  ..2 sets used, room for +16
*                                  ..next set at X'1800'
*
LBFPOUT  EQU   STRTLABL+X'2000'    Long BFP results from Int-32 inputs
*                                  ..6 pairs used, room for 16 pairs
LBFPFLGS EQU   STRTLABL+X'2100'    Long BFP FPCR contents
*                                  ..6 pairs used, room for 32+ pairs
*                                  ..next pair at X'2200'
*
XBFPOUT  EQU   STRTLABL+X'3000'    Extended BFP results from Int-32 
*                                  ..6 pairs used, room for 16 pairs
XBFPFLGS EQU   STRTLABL+X'3200'    Extended BFP FPCR contents
*                                  ..6 pairs used, room for 16 pairs
*
ENDLABL  EQU   STRTLABL+X'3300'    Next result area available
         PADCSECT ENDLABL
         END
