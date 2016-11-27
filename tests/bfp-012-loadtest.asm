 TITLE 'bfp-012-loadtest.asm: Test IEEE Test Data Class, Load And Test'
***********************************************************************
*
*Testcase IEEE Test Data Classs and Load And Test
*  Exhaustively test results from the Test Data Class instruction.  
*  Exhaustively test condition code setting from Load And Test.
*  The Condition Code, the only result from either instruction, is 
*  saved for comparison with reference values.  
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*                       bfp-012-loadtest.asm 
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
* Neither Load And Test nor Test Data Class can result in IEEE
* exceptions.  All tests are performed with the FPC set to not trap
* on any exception.  
*
* The same test data are used for both Load And Test and Test Data 
* Class.  
*
* For Load And Test, the result value and condition code are stored.
* For all but SNaN inputs, the result should be the same as the input.
* For SNaN inputs, the result is the corresponding QNaN.
*
* For Test Data Class, 13 Condition codes are stored.  The  first 
* 12 correspond to a one-bit in each of 12 positions of the Test Data
* class second operand mask, and the thirteenth is generated with a
* mask of zero.  Test Data Class mask bits:
*
*           1 0 0 0 | 0 0 0 0 | 0 0 0 0  + zero
*           0 1 0 0 | 0 0 0 0 | 0 0 0 0  - zero
*           0 0 1 0 | 0 0 0 0 | 0 0 0 0  + finite
*           0 0 0 1 | 0 0 0 0 | 0 0 0 0  - finite
*           0 0 0 0 | 1 0 0 0 | 0 0 0 0  + tiny
*           0 0 0 0 | 0 1 0 0 | 0 0 0 0  - tiny
*           0 0 0 0 | 0 0 1 0 | 0 0 0 0  + inf
*           0 0 0 0 | 0 0 0 1 | 0 0 0 0  - inf
*           0 0 0 0 | 0 0 0 0 | 1 0 0 0  + QNaN 
*           0 0 0 0 | 0 0 0 0 | 0 1 0 0  - QNaN 
*           0 0 0 0 | 0 0 0 0 | 0 0 1 0  + SNaN
*           0 0 0 0 | 0 0 0 0 | 0 0 0 1  - SNaN
*
* Tests 3 LOAD AND TEST and 3 TEST DATA CLASS instructions
*   LOAD AND TEST (BFP short, RRE) LTEBR
*   LOAD AND TEST (BFP long, RRE) LTDBR
*   LOAD AND TEST (BFP extended, RRE) LTXBR
*   TEST DATA CLASS (BFP short, RRE) LTEBR
*   TEST DATA CLASS (BFP long, RRE) LTDBR
*   TEST DATA CLASS (BFP extended, RRE) LTXBR
*
* Also tests the following floating point support instructions
*   EXTRACT FPC 
*   LOAD  (Short)
*   LOAD  (Long)
*   LOAD ZERO (Long)
*   STORE (Short)
*   STORE (Long)
*   SET FPC
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
BFPLTTDC START 0
STRTLABL EQU   *
R0       EQU   0                   Work register for cc extraction
R1       EQU   1                   Current Test Data Class mask
R2       EQU   2                   Holds count of test input values
R3       EQU   3                   Points to next test input value(s)
R4       EQU   4                   Available
R5       EQU   5                   Available
R6       EQU   6                   Test Data Class top of loop
R7       EQU   7                   Ptr to next result for Load And Test
R8       EQU   8                   Ptr to next CC for Load And Test
R9       EQU   9                   Ptr to next CC for Test Data Class
R10      EQU   10                  Pointer to test address list
R11      EQU   11                  **Reserved for z/CMS test rig
R12      EQU   12                  Test value top of loop
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
         SPACE 2 
***********************************************************************
*
* Low core definitions, Restart PSW, and Program Check Routine.
*
***********************************************************************
         SPACE 2
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
START    DS    0H
         STCTL R0,R0,CTLR0   Store CR0 to enable AFP
         OI    CTLR0+1,X'04' Turn on AFP bit
         LCTL  R0,R0,CTLR0   Reload updated CR0
*
         LA    R10,SHORTS    Point to short BFP test inputs
         BAS   R13,TESTSBFP  Perform short BFP tests
*
         LA    R10,LONGS     Point to long BFP test inputs
         BAS   R13,TESTLBFP  Perform long BFP tests
*
         LA    R10,EXTDS     Point to extended BFP test inputs
         BAS   R13,TESTXBFP  Perform short BFP tests
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
FPCREGNT DC    X'00000000'   FPCR, trap no IEEE exceptions, zero flags
FPCREGTR DC    X'F8000000'   FPCR, trap all IEEE exceptions, zero flags
*
* Input values parameter list, four fullwords for each test data set 
*      1) Count, 
*      2) Address of inputs, 
*      3) Address to place results, and
*      4) Address to place DXC/Flags/cc values.  
*
         ORG   STRTLABL+X'300'  Enable run-time replacement
SHORTS   DS    0F           Input pairs for short BFP ests
         DC    A(SBFPINCT)
         DC    A(SBFPIN)
         DC    A(SBFPOUT)
         DC    A(SBFPOCC)
*
LONGS    DS    0F           Input pairs for long BFP testing
         DC    A(LBFPINCT)
         DC    A(LBFPIN)
         DC    A(LBFPOUT)
         DC    A(LBFPOCC)
*
EXTDS    DS    0F           Input pairs for extendedd BFP testing
         DC    A(XBFPINCT)
         DC    A(XBFPIN)
         DC    A(XBFPOUT)
         DC    A(XBFPOCC)
         EJECT
***********************************************************************
* Perform Short BFP Tests.  This includes one execution of Load And 
* Test, followed by 13 executions of Test Data Class.  The result value
* and Condition code are saved for Load And Test, and the Condition 
* Code is saved for each execution of Test Data Class.  
*
***********************************************************************
         SPACE 2
TESTSBFP DS    0H            Test short BFP input values
         LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result and CC areas.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR8,0(,R3)   Get short BFP test value
*                            Polute the CC result area.  Correct
*                            ..results will clean it up.
         MVC   0(16,R8),=X'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LE    FPR1,SBFPINVL Ensure an unchanged FPR1 is detectable
         IPM   R0            Get current program mask and CC
         N     R0,=X'CFFFFFFF' Turn off condition code bits
         O     R0,=X'20000000' Force condition code two
         SPM   R0            Set PSW CC to two
         LTEBR FPR1,FPR8     Load and Test into FPR1
         STE   FPR1,0(,R7)   Store short BFP result
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,0(,R8)     Store in CC result area
*
         LFPC  FPCREGTR      Set exceptions non-trappable
         LE    FPR1,SBFPINVL Ensure an unchanged FPR1 is detectable
         IPM   R0            Get current program mask and CC
         N     R0,=X'CFFFFFFF' Turn off condition code bits
         O     R0,=X'20000000' Force condition code two
         SPM   R0            Set PSW CC to two
         LTEBR FPR1,FPR8     Load and Test into FPR1
         STE   FPR1,4(,R7)   Store short BFP result
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,1(,R8)     Store in CC result area
         EFPC  R0            Extract FPC contents to R0
         STCM  R0,B'0010',2(R8)  Store any DXC code 
*
         LHI   R1,4096       Load Test Data Class mask starting point
         LA    R9,3(,R8)     Point to first Test Data Class CC
         BASR  R6,0          Set start of Test Data Class loop
*
         SRL   R1,1          Shift to get next class mask value
         TCEB  FPR8,0(,R1)   Test value against class mask
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,0(,R9)     Store in CC result area
         LA    R9,1(,R9)     Point to next CC slot
         LTR   R1,R1         Have we tested all masks including zero
         BNZR  R6            ..no, at least one more to test

         LA    R3,4(,R3)     Point to next short BFP test value
         LA    R7,8(,R7)     Point to next Load And Test result pair
         LA    R8,16(,R8)    Point to next CC result set
         BCTR  R2,R12        Loop through all test cases
*
         BR    R13           Tests done, return to mainline
         EJECT
***********************************************************************
* Perform long BFP Tests.  This includes one execution of Load And 
* Test, followed by 13 executions of Test Data Class.  The result value
* and Condition code are saved for Load And Test, and the Condition 
* Code is saved for each execution of Test Data Class.  
*
***********************************************************************
         SPACE 2
TESTLBFP DS    0H            Test long BFP input values
         LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result and CC areas.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR8,0(,R3)   Get long BFP test value
*                            Polute the CC result area.  Correct
*                            ..results will clean it up.
         MVC   0(16,R8),=X'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LD    FPR1,LBFPINVL Ensure an unchanged FPR1 is detectable
         IPM   R0            Get current program mask and CC
         N     R0,=X'CFFFFFFF' Turn off condition code bits
         O     R0,=X'10000000' Force condition code one
         SPM   R0            Set PSW CC to one
         LTDBR FPR1,FPR8     Load and Test into FPR1
         STD   FPR1,0(,R7)   Store long BFP result
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,0(,R8)     Store in CC result area
*
         LFPC  FPCREGTR      Set exceptions trappable
         LD    FPR1,LBFPINVL Ensure an unchanged FPR1 is detectable
         IPM   R0            Get current program mask and CC
         N     R0,=X'CFFFFFFF' Turn off condition code bits
         O     R0,=X'10000000' Force condition code one
         SPM   R0            Set PSW CC to one
         LTDBR FPR1,FPR8     Load and Test into FPR1
         STD   FPR1,8(,R7)   Store long BFP result
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,1(,R8)     Store in CC result area
         EFPC  R0            Extract FPC contents to R0
         STCM  R0,B'0010',2(R8)  Store any DXC code 
*
         LHI   R1,4096       Load Test Data Class mask starting point
         LA    R9,3(,R8)     Point to first Test Data Class CC
         BASR  R6,0          Set start of Test Data Class loop

         SRL   R1,1          Shift to get next class mask value
         TCDB  FPR8,0(,R1)   Test value against class mask
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,0(,R9)     Store in CC result area
         LA    R9,1(,R9)     Point to next CC slot
         LTR   R1,R1         Have we tested all masks including zero
         BNZR  R6            ..no, at least one more to test

         LA    R3,8(,R3)     Point to next long BFP test value
         LA    R7,16(,R7)    Point to next Load And Test result pair
         LA    R8,16(,R8)    Point to next CC result set
         BCTR  R2,R12        Loop through all test cases
*
         BR    R13           Tests done, return to mainline
         EJECT
***********************************************************************
* Perform extended BFP Tests.  This includes one execution of Load And 
* Test, followed by 13 executions of Test Data Class.  The result value
* and Condition code are saved for Load And Test, and the Condition 
* Code is saved for each execution of Test Data Class.  
*
***********************************************************************
         SPACE 2
TESTXBFP DS    0H            Test extended BFP input values
         LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result and CC areas.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR8,0(,R3)   Get extended BFP test value part 1
         LD    FPR10,8(,R3)  Get extended BFP test value part 2
*                            Polute the CC result area.  Correct
*                            ..results will clean it up.
         MVC   0(16,R8),=X'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LD    FPR1,XBFPINVL   Ensure an unchanged FPR1-3 is detectable
         LD    FPR3,XBFPINVL+8 ..part 2 of load FPR pair
         IPM   R0            Get current program mask and CC
         N     R0,=X'CFFFFFFF' Turn off condition code bits
         SPM   R0            Set PSW CC to zero
         LTXBR FPR1,FPR8     Load and Test into FPR1
         STD   FPR1,0(,R7)   Store extended BFP result part 1
         STD   FPR3,8(,R7)   Store extended BFP result part 2
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,0(,R8)     Store in CC result area
*
         LFPC  FPCREGTR      Set exceptions trappable
         LD    FPR1,XBFPINVL   Ensure an unchanged FPR1-3 is detectable
         LD    FPR3,XBFPINVL+8 ..part 2 of load FPR pair
         IPM   R0            Get current program mask and CC
         N     R0,=X'CFFFFFFF' Turn off condition code bits
         SPM   R0            Set PSW CC to zero
         LTXBR FPR1,FPR8     Load and Test into FPR1
         STD   FPR1,16(,R7)  Store extended BFP result part 1
         STD   FPR3,24(,R7)  Store extended BFP result part 2
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,1(,R8)     Store in CC result area
         EFPC  R0            Extract FPC contents to R0
         STCM  R0,B'0010',2(R8)  Store any DXC code 
*
         LHI   R1,4096       Load Test Data Class mask starting point
         LA    R9,3(,R8)     Point to first Test Data Class CC
         BASR  R6,0          Set start of Test Data Class loop

         SRL   R1,1          Shift to get next class mask value
         TCXB  FPR8,0(,R1)   Test value against class mask
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,0(,R9)     Store in last byte of FPCR
         LA    R9,1(,R9)     Point to next CC slot
         LTR   R1,R1         Have we tested all masks including zero
         BNZR  R6            ..no, at least one more to test

         LA    R3,16(,R3)    Point to next extended BFP test value
         LA    R7,32(,R7)    Point to next Load And Test result pair
         LA    R8,16(,R8)    Point to next CC result set
         BCTR  R2,R12        Loop through all test cases
*
         BR    R13           Tests done, return to mainline
*
         LTORG
         EJECT
***********************************************************************
*
* Short integer inputs for Load And Test and Test Data Class.  The same
* values are used for short, long, and extended formats.  
*
***********************************************************************
         SPACE 2
SBFPIN   DS    0F            Ensure fullword alignment for input table
         DC    X'00000000'      +0
         DC    X'80000000'      -0
         DC    X'3F800000'      +1 
         DC    X'BF800000'      -1
         DC    X'007FFFFF'      +subnormal
         DC    X'807FFFFF'      -subnormal
         DC    X'7F800000'      +infinity
         DC    X'FF800000'      -infinity
         DC    X'7FC00000'      +QNaN
         DC    X'FFC00000'      -QNaN
         DC    X'7F810000'      +SNaN
         DC    X'FF810000'      -SNaN
SBFPINCT EQU   (*-SBFPIN)/4  count of short BFP test values
*
SBFPINVL DC    X'0000DEAD'   Invalid result, used to polute result FPR
         SPACE 3
***********************************************************************
*
* Long integer inputs for Load And Test and Test Data Class.  The same
* values are used for short, long, and extended formats.  
*
***********************************************************************
         SPACE 2
LBFPIN   DS    0D
         DC    X'0000000000000000'      +0
         DC    X'8000000000000000'      -0
         DC    X'3FF0000000000000'      +1 
         DC    X'BFF0000000000000'      -1
         DC    X'000FFFFFFFFFFFFF'      +subnormal
         DC    X'800FFFFFFFFFFFFF'      -subnormal
         DC    X'7FF0000000000000'      +infinity
         DC    X'FFF0000000000000'      -infinity
         DC    X'7FF8000000000000'      +QNaN
         DC    X'FFF8000000000000'      -QNaN
         DC    X'7FF0100000000000'      +SNaN
         DC    X'FFF0100000000000'      -SNaN
LBFPINCT EQU   (*-LBFPIN)/8  count of long BFP test values
*
LBFPINVL DC    X'0000DEAD00000000'  Invalid result, used to 
*                                   ..polute result FPR
         SPACE 3
***********************************************************************
*
* Extended integer inputs for Load And Test and Test Data Class.  The
* same values are used for short, long, and extended formats.  
*
***********************************************************************
         SPACE 2
XBFPIN   DS    0D
         DC    X'00000000000000000000000000000000' +0
         DC    X'80000000000000000000000000000000' -0
         DC    X'3FFF0000000000000000000000000000' +1 
         DC    X'BFFF0000000000000000000000000000' -1
         DC    X'0000FFFFFFFFFFFFFFFFFFFFFFFFFFFF' +subnormal
         DC    X'8000FFFFFFFFFFFFFFFFFFFFFFFFFFFF' -subnormal
         DC    X'7FFF0000000000000000000000000000' +infinity
         DC    X'FFFF0000000000000000000000000000' -infinity
         DC    X'7FFF8000000000000000000000000000' +QNaN
         DC    X'FFFF8000000000000000000000000000' -QNaN
         DC    X'7FFF0100000000000000000000000000' +SNaN
         DC    X'FFFF0100000000000000000000000000' -SNaN
XBFPINCT EQU   (*-XBFPIN)/16  count of extended BFP test values
*
XBFPINVL DC    X'0000DEAD000000000000000000000000'  Invalid result, used to 
*                                   ..used to polute result FPR
         SPACE 3
***********************************************************************
*
*  Locations for results
*
***********************************************************************
         SPACE 2
SBFPOUT  EQU   STRTLABL+X'1000'    Integer short BFP Load and Test
*                                  ..12 used, room for 60 tests
SBFPOCC  EQU   STRTLABL+X'1100'    Condition codes from Load and Test
*                                  ..and Test Data Class.
*                                  ..12 sets used, room for 60 sets
*                                  ..next available is X'1500'
*
LBFPOUT  EQU   STRTLABL+X'2000'    Integer long BFP Load And Test
*                                  ..12 used, room for 32 tests, 
LBFPOCC  EQU   STRTLABL+X'2100'     Condition codes from Load and Test
*                                  ..and Test Data Class.
*                                  ..12 sets used, room for 32 sets
*                                  ..next available is X'2300'
*
XBFPOUT  EQU   STRTLABL+X'3000'    Integer extended BFP Load And Test
*                                  ..12 used, room for 32 tests, 
XBFPOCC  EQU   STRTLABL+X'3200'    Condition codes from Load and Test
*                                  ..and Test Data Class.
*                                  ..12 sets used, room for 32 sets
*                                  ..next available is X'3400'
*
ENDLABL  EQU   STRTLABL+X'3400'
         PADCSECT ENDLABL
*
         END