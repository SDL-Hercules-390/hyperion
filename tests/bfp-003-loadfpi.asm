  TITLE 'bfp-003-loadfpi.asm: Test IEEE Load FP Integer'
***********************************************************************
*
*Testcase IEEE LOAD FP INTEGER
*  Test case capability includes IEEE exceptions trappable and 
*  otherwise. Test results, FPCR flags, and any DXC are saved for all 
*  tests.  Load FP Integer does not set the condition code.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*                       bfp-003-loadfpi.asm 
*
*        This assembly-language source file is part of the
*        Hercules Binary Floating Point Validation Package 
*                        by Stephen R. Orso
*
* Copyright 2016 by Stephen R Orso.
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
*   LOAD FP INTEGER (short BFP, RRE)
*   LOAD FP INTEGER (long BFP, RRE) 
*   LOAD FP INTEGER (extended BFP, RRE)  
*   LOAD FP INTEGER (short BFP, RRF-e)
*   LOAD FP INTEGER (long BFP, RRF-e) 
*   LOAD FP INTEGER (extended BFP, RRF-e)  
* 
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R 
* commands.
* 
* Test Case Order
* 1) Short BFP inexact masking/trapping & SNaN/QNaN tests
* 2) Short BFP rounding mode tests
* 3) Long BFP inexact masking/trapping & SNaN/QNaN tests
* 4) Long BFP rounding mode tests
* 5) Extended BFP inexact masking/trapping & SNaN/QNaN tests
* 6) Extended BFP rounding mode tests
*
* Provided test data is 1, 1.5, SNaN, and QNaN.
*   The second value will trigger an inexact exception when LOAD FP 
*   INTEGER is executed.  The final value will trigger an invalid 
*   exception.  
* Provided test data for rounding tests is 
*      -9.5, -5.5, -2.5, -1.5, -0.5, +0.5, +1.5, +2.5, +5.5, +9.5
*   This data is taken from Table 9-11 on page 9-16 of SA22-7832-10.
*
* Three input test data sets are provided, one each for short, long,
*   and extended precision BFP inputs.  
*
* Also tests the following floating point support instructions
*   LOAD  (Short)
*   LOAD  (Long)
*   LFPC  (Load Floating Point Control Register)
*   SRNMB (Set BFP Rounding Mode 2-bit)
*   SRNMB (Set BFP Rounding Mode 3-bit)
*   STORE (Short)
*   STORE (Long)
*   STFPC (Store Floating Point Control Register)
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
BFPLDFPI START 0
R0       EQU   0                   Work register for cc extraction
R1       EQU   1                   Available
R2       EQU   2                   Holds count of test input values
R3       EQU   3                   Points to next test input value(s)
R4       EQU   4                   Available
R5       EQU   5                   Available
R6       EQU   6                   Available
R7       EQU   7                   Pointer to next result value(s)
R8       EQU   8                   Pointer to next FPCR result
R9       EQU   9                   Available
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
* Above works on real iron (R15=0 after sysclear) 
* and in z/CMS (R15 points to start of load module)
*
         SPACE 2 
***********************************************************************
*
* Low core definitions, Restart PSW, and Program Check Routine.
*
***********************************************************************
         SPACE 2
         ORG   BFPLDFPI+X'8E'      Program check interrution code
PCINTCD  DS    H
*
PCOLDPSW EQU   BFPLDFPI+X'150'     z/Arch Program check old PSW
*
         ORG   BFPLDFPI+X'1A0'     z/Arch Restart PSW
         DC    X'0000000180000000',AD(START)   
*
         ORG   BFPLDFPI+X'1D0'     z/Arch Program check old PSW
         DC    X'0000000000000000',AD(PROGCHK)
* 
* Program check routine.  If Data Exception, continue execution at
* the instruction following the program check.  Otherwise, hard wait.
* No need to collect data.  All interesting DXC stuff is captured
* in the FPCR.
*
PROGCHK  DS    0H             Program check occured...
         CLI   PCINTCD+1,X'07'  Data Exception?
         JNE   PCNOTDTA       ..no, hardwait (not sure if R15 is ok)
         LPSWE PCOLDPSW       ..yes, resume program execution
PCNOTDTA DS    0H
         LTR   R14,R14        Return address provided?
         BNZR  R14            Yes, return to z/CMS test rig.  
         LPSWE HARDWAIT       Not data exception, enter disabled wait.
         EJECT
***********************************************************************
*
*  Main program.  Enable Advanced Floating Point, process test cases.
*
***********************************************************************
         SPACE 2
START    DS    0H
         STCTL R0,R0,CTLR0   Store CR0 to enable AFP
         OI    CTLR0+1,X'04' Turn on AFP bit
         LCTL  R0,R0,CTLR0   Reload updated CR0
*
         LA    R10,SHORTS    Point to short BFP test inputs
         BAS   R13,FIEBR     Convert short BFP to integer short BFP
         LA    R10,RMSHORTS  Point to short BFP rounding test data
         BAS   R13,FIEBRA    Convert using all rounding mode options
*
         LA    R10,LONGS     Point to long BFP test inputs
         BAS   R13,FIDBR     Convert long BFP to integer long BFP
         LA    R10,RMLONGS   Point to long BFP rounding test data
         BAS   R13,FIDBRA    Convert using all rounding mode options
*
         LA    R10,EXTDS     Point to extended BFP test inputs
         BAS   R13,FIXBR     Convert extd BFP to integer extd BFP
         LA    R10,RMEXTDS   Point to extended BFP rounding test data
         BAS   R13,FIXBRA    Convert using all rounding mode options
*
         LTR   R14,R14       Return address provided?
         BNZR  R14           ..Yes, return to z/CMS test rig.  
         LPSWE WAITPSW       All done
*
         DS    0D            Ensure correct alignment for psw
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
         ORG   BFPLDFPI+X'300'
SHORTS   DS    0F            Inputs for short BFP testing
         DC    A(SBFPCT/4)
         DC    A(SBFPIN)
         DC    A(SBFPOUT)
         DC    A(SBFPFLGS)
*
LONGS    DS    0F            Inputs for long BFP testing
         DC    A(LBFPCT/8)
         DC    A(LBFPIN)
         DC    A(LBFPOUT)
         DC    A(LBFPFLGS)
*
EXTDS    DS    0F            Inputs for Extended BFP testing
         DC    A(XBFPCT/16)
         DC    A(XBFPIN)
         DC    A(XBFPOUT)
         DC    A(XBFPFLGS)
*
RMSHORTS DS    0F            Inputs for short BFP rounding testing
         DC    A(SBFPRMCT/4)
         DC    A(SBFPINRM)
         DC    A(SBFPRMO)
         DC    A(SBFPRMOF)
*
RMLONGS  DS    0F            Inputs for long  BFP rounding testing
         DC    A(LBFPRMCT/8)
         DC    A(LBFPINRM)
         DC    A(LBFPRMO)
         DC    A(LBFPRMOF)
*
RMEXTDS  DS    0F            Inputs for extd BFP rounding testing
         DC    A(XBFPRMCT/16)
         DC    A(XBFPINRM)
         DC    A(XBFPRMO)
         DC    A(XBFPRMOF)
         EJECT
***********************************************************************
*
* Round short BFP inputs to integer short BFP.  A pair of results is 
* generated for each input: one with all exceptions non-trappable, and 
* the second with all exceptions trappable.   The FPCR is stored for
* each result.
*
***********************************************************************
         SPACE 2
FIEBR    DS    0H            Round short BFP inputs to integer BFP
         LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR0,0(,R3)   Get short BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         FIEBR FPR1,0,FPR0   Cvt float in FPR0 to int float in FPR1
         STE   FPR1,0(,R7)   Store short BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LZER  FPR1          Eliminate any residual results
         FIEBR FPR1,0,FPR0   Cvt float in FPR0 to int float in FPR1
         STE   FPR1,4(,R7)   Store short BFP result
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LA    R3,4(,R3)     Point to next input value
         LA    R7,8(,R7)     Point to next rounded rusult value pair
         LA    R8,8(,R8)     Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert short BFP to integer BFP using each possible rounding mode.
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPCR with
* the IEEE Inexact exception supressed.  SRNM (2-bit) is used  for
* the first two FPCR-controlled tests and SRNMB (3-bit) is used for
* the last two To get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section;
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
FIEBRA   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR0,0(,R3)    Get short BFP test value
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  1             SET FPCR to RZ, towards zero.  
         FIEBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, inexact masked
         STE   FPR1,0*4(,R7) Store integer BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPCR to RP, to +infinity
         FIEBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, inexact masked
         STE   FPR1,1*4(,R7) Store integer BFP result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, to -infinity
         FIEBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, inexact masked
         STE   FPR1,2*4(,R7) Store integer BFP result
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RPS, Prepare for Shorter Precision
         FIEBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, inexact masked
         STE   FPR1,3*4(,R7) Store integer BFP result
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
*
* Test cases using rounding mode specified in the instruction M3 field
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIEBRA FPR1,1,FPR0,B'0000' RNTA, to nearest, ties away
         STE   FPR1,4*4(,R7) Store integer BFP result
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIEBRA FPR1,3,FPR0,B'0000' RFS, prepare for shorter precision
         STE   FPR1,5*4(,R7) Store integer BFP result
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIEBRA FPR1,4,FPR0,B'0000' RNTE, to nearest, ties to even
         STE   FPR1,6*4(,R7) Store integer BFP result
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIEBRA FPR1,5,FPR0,B'0000' RZ, toward zero
         STE   FPR1,7*4(,R7) Store integer BFP result
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIEBRA FPR1,6,FPR0,B'0000' RP, to +inf
         STE   FPR1,8*4(,R7) Store integer BFP result
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIEBRA FPR1,7,FPR0,B'0000' RM, to -inf
         STE   FPR1,9*4(,R7) Store integer BFP result
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
*
         LA    R3,4(,R3)     Point to next input values
         LA    R7,12*4(,R7)  Point to next short BFP converted values
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Round long BFP inputs to integer long BFP.  A pair of results is 
* generated for each input: one with all exceptions non-trappable, and 
* the second with all exceptions trappable.   The FPCR is stored for 
* each result.  
*
***********************************************************************
         SPACE 2
FIDBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)   Get long BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         FIDBR FPR1,0,FPR0   Cvt float in FPR0 to int float in FPR1
         STD   R1,0(,R7)     Store long BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LZDR  FPR1          Eliminate any residual results
         FIDBR FPR1,0,FPR0   Cvt float in FPR0 to int float in FPR1
         STD   FPR1,8(,R7)   Store int-32 result
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LA    R3,8(,R3)     Point to next input value
         LA    R7,16(,R7)    Point to next rounded long BFP result pair
         LA    R8,8(,R8)     Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert long BFP to integers using each possible rounding mode. 
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPCR with
* the IEEE Inexact exception supressed.  SRNM (2-bit) is used  for
* the first two FPCR-controlled tests and SRNMB (3-bit) is used for
* the last two To get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section;
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
FIDBRA   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)    Get long BFP test value
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  1             SET FPCR to RZ, towards zero.  
         FIDBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, inexact masked
         STD   FPR1,0*8(,R7) Store integer BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPCR to RP, to +infinity
         FIDBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, inexact masked
         STD   FPR1,1*8(,R7) Store integer BFP result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, to -infinity
         FIDBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, inexact masked
         STD   FPR1,2*8(,R7) Store integer BFP result
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RPS, Prepare for Shorter Precision
         FIDBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, inexact masked
         STD   FPR1,3*8(,R7) Store integer BFP result
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
*
* Test cases using rounding mode specified in the instruction M3 field
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIDBRA FPR1,1,FPR0,B'0000' RNTA, to nearest, ties away
         STD   FPR1,4*8(,R7) Store integer BFP result
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIDBRA FPR1,3,FPR0,B'0000' RFS, prepare for shorter precision
         STD   FPR1,5*8(,R7) Store integer BFP result
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIDBRA FPR1,4,FPR0,B'0000' RNTE, to nearest, ties to even
         STD   FPR1,6*8(,R7) Store integer BFP result
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIDBRA FPR1,5,FPR0,B'0000' RZ, toward zero
         STD   FPR1,7*8(,R7) Store integer BFP result
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIDBRA FPR1,6,FPR0,B'0000' RP, to +inf
         STD   FPR1,8*8(,R7) Store integer BFP result
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIDBRA FPR1,7,FPR0,B'0000' RM, to -inf
         STD   FPR1,9*8(,R7) Store integer BFP result
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
*
         LA    R3,8(,R3)     Point to next input values
         LA    R7,10*8(,R7)  Point to next long BFP converted values
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Round extended BFP to integer extended BFP.  A pair of results is 
* generated for each input: one with all exceptions non-trappable, and 
* the second with all exceptions trappable.   The FPCR is stored for
* each result.
*
***********************************************************************
         SPACE 2
FIXBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)   Get extended BFP test value part 1
         LD    FPR2,8(,R3)   Get extended BFP test value part 2
         LFPC  FPCREGNT      Set exceptions non-trappable
         FIXBR FPR1,0,FPR0   Cvt FPR0-FPR2 to int float in FPR1-FPR3
         STD   FPR1,0(,R7)   Store integer BFP result part 1
         STD   FPR3,8(,R7)   Store integer BFP result part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LZXR  FPR1          Eliminate any residual results
         FIXBR FPR1,0,FPR0   Cvt FPR0-FPR2 to int float in FPR1-FPR3
         STD   FPR1,16(,R7)  Store integer BFP result part 1
         STD   FPR3,24(,R7)  Store integer BFP result part 2
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LA    R3,16(,R3)    Point to next extended BFP input value
         LA    R7,32(,R7)    Point to next extd BFP rounded result pair
         LA    R8,8(,R8)     Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert extended BFP to integers using each possible rounding mode.
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPCR with
* the IEEE Inexact exception supressed.  SRNM (2-bit) is used  for
* the first two FPCR-controlled tests and SRNMB (3-bit) is used for
* the last two To get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section;
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
FIXBRA   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)   Get extended BFP test value part 1
         LD    FPR2,8(,R3)   Get extended BFP test value part 2
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  1             SET FPCR to RZ, towards zero.  
         FIXBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, inexact masked
         STD   FPR1,0*16(,R7)      Store integer BFP result part 1
         STD   FPR3,(0*16)+8(,R7)  Store integer BFP result part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPCR to RP, to +infinity
         FIXBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, inexact masked
         STD   FPR1,1*16(,R7)      Store integer BFP result part 1
         STD   FPR3,(1*16)+8(,R7)  Store integer BFP result part 2
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, to -infinity
         FIXBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, inexact masked
         STD   FPR1,2*16(,R7)      Store integer BFP result part 1
         STD   FPR3,(2*16)+8(,R7)  Store integer BFP result part 2
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RFS, Prepare for Shorter Precision
         FIXBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, inexact masked
         STD   FPR1,3*16(,R7)      Store integer BFP result part 1
         STD   FPR3,(3*16)+8(,R7)  Store integer BFP result part 2
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
*
* Test cases using rounding mode specified in the instruction M3 field
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIXBRA FPR1,1,FPR0,B'0000'  RNTA, to nearest, ties away
         STD   FPR1,4*16(,R7)      Store integer BFP result part 1
         STD   FPR3,(4*16)+8(,R7)  Store integer BFP result part 2
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIXBRA FPR1,3,FPR0,B'0000'  RFS, prepare for shorter precision
         STD   FPR1,5*16(,R7)      Store integer BFP result part 1
         STD   FPR3,(5*16)+8(,R7)  Store integer BFP result part 2
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIXBRA FPR1,4,FPR0,B'0000'  RNTE, to nearest, ties to even
         STD   FPR1,6*16(,R7)      Store integer BFP result part 1
         STD   FPR3,(6*16)+8(,R7)  Store integer BFP result part 2
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIXBRA FPR1,5,FPR0,B'0000'  RZ, toward zero
         STD   FPR1,7*16(,R7)      Store integer BFP result part 1
         STD   FPR3,(7*16)+8(,R7)  Store integer BFP result part 2
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIXBRA FPR1,6,FPR0,B'0000'  RP, to +inf
         STD   FPR1,8*16(,R7)      Store integer BFP result part 1
         STD   FPR3,(8*16)+8(,R7)  Store integer BFP result part 2
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         FIXBRA FPR1,7,FPR0,B'0000'  RM, to -inf
         STD   FPR1,9*16(,R7)      Store integer BFP result part 1
         STD   FPR3,(9*16)+8(,R7)  Store integer BFP result part 2
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
*
         LA    R3,16(,R3)    Point to next input value
         LA    R7,10*16(,R7) Point to next long BFP converted values
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Short integer inputs for Load FP Integer testing.  The same 
* values are used for short, long, and extended formats.  
*
***********************************************************************
         SPACE 2
SBFPIN   DS    0F                Inputs for short BFP testing
         DC    X'3F800000'         +1.0  Exact
         DC    X'BFC00000'         -1.5  Inexact, incremented
         DC    X'40200000'         +2.5  Inexact only
         DC    X'7F810000'         SNaN
         DC    X'7FC10000'         QNaN
         DC    X'3F400000'         +.75  Inexact, incremented
         DC    X'BE800000'         -.25  Inexact
SBFPCT   EQU   *-SBFPIN     Count of short BFP in list * 4
*
SBFPINRM DS    0F                Inputs for short BFP rounding testing
         DC    X'C1180000'         -9.5
         DC    X'C0B00000'         -5.5
         DC    X'C0200000'         -2.5
         DC    X'BFC00000'         -1.5
         DC    X'BF000000'         -0.5
         DC    X'3F000000'         +0.5
         DC    X'3FC00000'         +1.5
         DC    X'40200000'         +2.5
         DC    X'40B00000'         +5.5
         DC    X'41180000'         +9.5
         DC    X'3F400000'         +.75
         DC    X'BE800000'         -.25
SBFPRMCT EQU   *-SBFPINRM   Count of short BFP rounding tests * 4
*
LBFPIN   DS    0F                Inputs for long BFP testing
         DC    X'3FF0000000000000'         +1.0
         DC    X'BFF8000000000000'         -1.5
         DC    X'4004000000000000'         +2.5
         DC    X'7FF0100000000000'         SNaN
         DC    X'7FF8100000000000'         QNaN
         DC    X'3FE8000000000000'         +.75
         DC    X'BFD0000000000000'         -.25
LBFPCT   EQU   *-LBFPIN     Count of long BFP in list * 8
*
LBFPINRM DS    0F
         DC    X'C023000000000000'         -9.5
         DC    X'C016000000000000'         -5.5
         DC    X'C004000000000000'         -2.5
         DC    X'BFF8000000000000'         -1.5
         DC    X'BFE0000000000000'         -0.5
         DC    X'3FE0000000000000'         +0.5
         DC    X'3FF8000000000000'         +1.5
         DC    X'4004000000000000'         +2.5
         DC    X'4016000000000000'         +5.5
         DC    X'4023000000000000'         +9.5
         DC    X'3FE8000000000000'         +.75
         DC    X'BFD0000000000000'         -.25
LBFPRMCT EQU   *-LBFPINRM   Count of long BFP rounding tests * 8
*
XBFPIN   DS    0D                Inputs for long BFP testing
         DC    X'3FFF0000000000000000000000000000'         +1.0
         DC    X'BFFF8000000000000000000000000000'         -1.5
         DC    X'40004000000000000000000000000000'         +2.5
         DC    X'7FFF0100000000000000000000000000'         SNaN
         DC    X'7FFF8100000000000000000000000000'         QNaN
         DC    X'3FFE8000000000000000000000000000'         +0.75
         DC    X'BFFD0000000000000000000000000000'         -0.25
XBFPCT   EQU   *-XBFPIN     Count of extended BFP in list * 16
*
XBFPINRM DS    0D
         DC    X'C0023000000000000000000000000000'         -9.5
         DC    X'C0016000000000000000000000000000'         -5.5
         DC    X'C0004000000000000000000000000000'         -2.5
         DC    X'BFFF8000000000000000000000000000'         -1.5
         DC    X'BFFE0000000000000000000000000000'         -0.5
         DC    X'3FFE0000000000000000000000000000'         +0.5
         DC    X'3FFF8000000000000000000000000000'         +1.5
         DC    X'40004000000000000000000000000000'         +2.5
         DC    X'40016000000000000000000000000000'         +5.5
         DC    X'40023000000000000000000000000000'         +9.5
         DC    X'3FFE8000000000000000000000000000'         +0.75
         DC    X'BFFD0000000000000000000000000000'         -0.25
XBFPRMCT EQU   *-XBFPINRM   Count of extended BFP rounding tests * 16
*
*  Locations for results
*
SBFPOUT  EQU   BFPLDFPI+X'1000'    Integer short BFP rounded results
*                                  ..7 used, room for 16
SBFPFLGS EQU   BFPLDFPI+X'1080'    FPCR flags and DXC from short BFP
*                                  ..7 used, room for 16 
SBFPRMO  EQU   BFPLDFPI+X'1100'    Short BFP rounding mode test results
*                                  ..12 used, room for 16
SBFPRMOF EQU   BFPLDFPI+X'1400'    Short BFP rounding mode FPCR results
*                                  ..12 used
*
LBFPOUT  EQU   BFPLDFPI+X'2000'    Integer long BFP rounded results
*                                  ..7 used, room for 16 
LBFPFLGS EQU   BFPLDFPI+X'2100'    FPCR flags and DXC from long BFP
*                                  ..7 used, room for 32
LBFPRMO  EQU   BFPLDFPI+X'2200'    Long BFP rounding mode test results
*                                  ..12 used, room for 16
LBFPRMOF EQU   BFPLDFPI+X'2800'    Long BFP rounding mode FPCR results
*                                  ..12 used
*
XBFPOUT  EQU   BFPLDFPI+X'3000'    Integer extended BFP rounded results
*                                  ..7 used, room for 16
XBFPFLGS EQU   BFPLDFPI+X'3200'    FPCR flags and DXC from extended BFP
*                                  ..7 used, room for 32
XBFPRMO  EQU   BFPLDFPI+X'3300'    Extd BFP rounding mode test results
*                                  ..12 used, room for 16
XBFPRMOF EQU   BFPLDFPI+X'3F00'    Extd BFP rounding mode FPCR results
*                                  ..12 used
*
*
ENDLABL  EQU   BFPLDFPI+X'4800'
*  Pad CSECT if not running on ASMA for a stand-alone environment
         PADCSECT ENDLABL
         END
