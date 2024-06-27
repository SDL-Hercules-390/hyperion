 TITLE 'bfp-012-loadtest: Test IEEE Test Data Class, Load And Test'
***********************************************************************
*
*Testcase IEEE Test Data Classs and Load And Test
*  Exhaustively test results from the Test Data Class instruction.
*  Exhaustively test condition code setting from Load And Test.
*  The Condition Code, the only result from either instruction, is
*  saved for comparison with reference values.
*
*
*                      ********************
*                      **   IMPORTANT!   **
*                      ********************
*
*        This test uses the Hercules Diagnose X'008' interface
*        to display messages and thus your .tst runtest script
*        MUST contain a "DIAG8CMD ENABLE" statement within it!
*
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*                       bfp-012-loadtest.asm
*
*        This assembly-language source file is part of the
*        Hercules Binary Floating Point Validation Package
*                        by Stephen R. Orso
*
* Copyright 2016 by Stephen R Orso.
* Runtest *Compare dependency removed by Fish on 2022-08-16
* PADCSECT macro/usage removed by Fish on 2022-08-16
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
         EJECT
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
         USING HELPERS,R12
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
         ORG   STRTLABL+X'1D0'     z/Arch Program check NEW PSW
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
                                                                SPACE
PCNOTDTA STM   R0,R15,SAVEREGS  Save registers
         L     R12,AHELPERS     Get address of helper subroutines
         BAS   R13,PGMCK        Report this unexpected program check
         LM    R0,R15,SAVEREGS  Restore registers
                                                                SPACE
         LTR   R14,R14        Return address provided?
         BNZR  R14            Yes, return to z/CMS test rig.
         LPSWE PROGPSW        Not data exception, enter disabled wait
PROGPSW  DC    0D'0',X'0002000000000000',XL6'00',X'DEAD' Abnormal end
FAIL     LPSWE FAILPSW        Not data exception, enter disabled wait
SAVEREGS DC    16F'0'         Registers save area
AHELPERS DC    A(HELPERS)     Address of helper subroutines
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
***********************************************************************
*                   Verify test results...
***********************************************************************
*
         L     R12,AHELPERS     Get address of helper subroutines
         BAS   R13,VERISUB      Go verify results
         LTR   R14,R14          Was return address provided?
         BNZR  R14              Yes, return to z/CMS test rig.
         LPSWE GOODPSW          Load SUCCESS PSW
                                                                EJECT
         DS    0D            Ensure correct alignment for PSW
GOODPSW  DC    X'0002000000000000',AD(0)  Normal end - disabled wait
FAILPSW  DC    X'0002000000000000',XL6'00',X'0BAD' Abnormal end
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
         EJECT
***********************************************************************
*                 ACTUAL results saved here
***********************************************************************
*
*               Locations for ACTUAL results
*
*
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
         EJECT
***********************************************************************
*                    EXPECTED results
***********************************************************************
*
         ORG   STRTLABL+X'4000'   (past end of actual results)
*
SBFPOUT_GOOD EQU *
 DC CL48'LTEBR +/-0'
 DC XL16'00000000000000008000000080000000'
 DC CL48'LTEBR +/-1'
 DC XL16'3F8000003F800000BF800000BF800000'
 DC CL48'LTEBR +/-tiny'
 DC XL16'007FFFFF007FFFFF807FFFFF807FFFFF'
 DC CL48'LTEBR +/-inf'
 DC XL16'7F8000007F800000FF800000FF800000'
 DC CL48'LTEBR +/-QNaN'
 DC XL16'7FC000007FC00000FFC00000FFC00000'
 DC CL48'LTEBR +/-SNaN'
 DC XL16'7FC100000000DEADFFC100000000DEAD'
SBFPOUT_NUM EQU (*-SBFPOUT_GOOD)/64
*
*
SBFPOCC_GOOD EQU *
 DC CL48'TCEB CC +0'
 DC XL16'00000001000000000000000000000000'
 DC CL48'TCEB CC -0'
 DC XL16'00000000010000000000000000000000'
 DC CL48'TCEB CC +1'
 DC XL16'02020000000100000000000000000000'
 DC CL48'TCEB CC -1'
 DC XL16'01010000000001000000000000000000'
 DC CL48'TCEB CC +tiny'
 DC XL16'02020000000000010000000000000000'
 DC CL48'TCEB CC -tiny'
 DC XL16'01010000000000000100000000000000'
 DC CL48'TCEB CC +inf'
 DC XL16'02020000000000000001000000000000'
 DC CL48'TCEB CC -inf'
 DC XL16'01010000000000000000010000000000'
 DC CL48'TCEB CC +QNaN'
 DC XL16'03030000000000000000000100000000'
 DC CL48'TCEB CC -QNaN'
 DC XL16'03030000000000000000000001000000'
 DC CL48'TCEB CC +SNaN'
 DC XL16'03028000000000000000000000010000'
 DC CL48'TCEB CC -SNaN'
 DC XL16'03028000000000000000000000000100'
SBFPOCC_NUM EQU (*-SBFPOCC_GOOD)/64
*
*
LBFPOUT_GOOD EQU *
 DC CL48'LTDBR +0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LTDBR -0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'LTDBR +1'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'LTDBR -1'
 DC XL16'BFF0000000000000BFF0000000000000'
 DC CL48'LTDBR +tiny'
 DC XL16'000FFFFFFFFFFFFF000FFFFFFFFFFFFF'
 DC CL48'LTDBR -tiny'
 DC XL16'800FFFFFFFFFFFFF800FFFFFFFFFFFFF'
 DC CL48'LTDBR +inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'LTDBR -inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'LTDBR +QNaN'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'LTDBR -QNaN'
 DC XL16'FFF8000000000000FFF8000000000000'
 DC CL48'LTDBR +SNaN'
 DC XL16'7FF81000000000000000DEAD00000000'
 DC CL48'LTDBR -SNaN'
 DC XL16'FFF81000000000000000DEAD00000000'
LBFPOUT_NUM EQU (*-LBFPOUT_GOOD)/64
*
*
LBFPOCC_GOOD EQU *
 DC CL48'TCDB CC +0'
 DC XL16'00000001000000000000000000000000'
 DC CL48'TCDB CC -0'
 DC XL16'00000000010000000000000000000000'
 DC CL48'TCDB CC +1'
 DC XL16'02020000000100000000000000000000'
 DC CL48'TCDB CC -1'
 DC XL16'01010000000001000000000000000000'
 DC CL48'TCDB CC +tiny'
 DC XL16'02020000000000010000000000000000'
 DC CL48'TCDB CC -tiny'
 DC XL16'01010000000000000100000000000000'
 DC CL48'TCDB CC +inf'
 DC XL16'02020000000000000001000000000000'
 DC CL48'TCDB CC -inf'
 DC XL16'01010000000000000000010000000000'
 DC CL48'TCDB CC +QNaN'
 DC XL16'03030000000000000000000100000000'
 DC CL48'TCDB CC -QNaN'
 DC XL16'03030000000000000000000001000000'
 DC CL48'TCDB CC +SNaN'
 DC XL16'03018000000000000000000000010000'
 DC CL48'TCDB CC -SNaN'
 DC XL16'03018000000000000000000000000100'
LBFPOCC_NUM EQU (*-LBFPOCC_GOOD)/64
*
*
XBFPOUT_GOOD EQU *
 DC CL48'LTXBR +/0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LTXBR +/0 TR'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LTXBR -0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'LTXBR -0 TR'
 DC XL16'80000000000000000000000000000000'
 DC CL48'LTXBR +1 NT'
 DC XL16'3FFF0000000000000000000000000000'
 DC CL48'LTXBR +1 TR'
 DC XL16'3FFF0000000000000000000000000000'
 DC CL48'LTXBR -1 NT'
 DC XL16'BFFF0000000000000000000000000000'
 DC CL48'LTXBR -1 TR'
 DC XL16'BFFF0000000000000000000000000000'
 DC CL48'LTXBR +tiny NT'
 DC XL16'0000FFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL48'LTXBR +tiny'
 DC XL16'0000FFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL48'LTXBR -tiny NT'
 DC XL16'8000FFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL48'LTXBR -tiny TR'
 DC XL16'8000FFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL48'LTXBR +inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'LTXBR +inf TR'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'LTXBR -inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'LTXBR -inf TR'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'LTXBR +QNaN NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'LTXBR +QNaN'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'LTXBR -QNaN NT'
 DC XL16'FFFF8000000000000000000000000000'
 DC CL48'LTXBR -QNaN TR'
 DC XL16'FFFF8000000000000000000000000000'
 DC CL48'LTXBR +SNaN NT'
 DC XL16'7FFF8100000000000000000000000000'
 DC CL48'LTXBR +SNaN TR'
 DC XL16'0000DEAD000000000000000000000000'
 DC CL48'LTXBR -SNaN NT'
 DC XL16'FFFF8100000000000000000000000000'
 DC CL48'LTXBR -SNaN TR'
 DC XL16'0000DEAD000000000000000000000000'
XBFPOUT_NUM EQU (*-XBFPOUT_GOOD)/64
*
*
XBFPOCC_GOOD EQU *
 DC CL48'TCXB CC +0'
 DC XL16'00000001000000000000000000000000'
 DC CL48'TCXB CC -0'
 DC XL16'00000000010000000000000000000000'
 DC CL48'TCXB CC +1'
 DC XL16'02020000000100000000000000000000'
 DC CL48'TCXB CC -1'
 DC XL16'01010000000001000000000000000000'
 DC CL48'TCXB CC +tiny'
 DC XL16'02020000000000010000000000000000'
 DC CL48'TCXB CC -tiny'
 DC XL16'01010000000000000100000000000000'
 DC CL48'TCXB CC +inf'
 DC XL16'02020000000000000001000000000000'
 DC CL48'TCXB CC -inf'
 DC XL16'01010000000000000000010000000000'
 DC CL48'TCXB CC +QNaN'
 DC XL16'03030000000000000000000100000000'
 DC CL48'TCXB CC -QNaN'
 DC XL16'03030000000000000000000001000000'
 DC CL48'TCXB CC +SNaN'
 DC XL16'03008000000000000000000000010000'
 DC CL48'TCXB CC -SNaN'
 DC XL16'03008000000000000000000000000100'
XBFPOCC_NUM EQU (*-XBFPOCC_GOOD)/64
                                                                EJECT
HELPERS  DS    0H       (R12 base of helper subroutines)
                                                                SPACE
***********************************************************************
*               REPORT UNEXPECTED PROGRAM CHECK
***********************************************************************
                                                                SPACE
PGMCK    DS    0H
         UNPK  PROGCODE(L'PROGCODE+1),PCINTCD(L'PCINTCD+1)
         MVI   PGMCOMMA,C','
         TR    PROGCODE,HEXTRTAB
                                                                SPACE
         UNPK  PGMPSW+(0*9)(9),PCOLDPSW+(0*4)(5)
         MVI   PGMPSW+(0*9)+8,C' '
         TR    PGMPSW+(0*9)(8),HEXTRTAB
                                                                SPACE
         UNPK  PGMPSW+(1*9)(9),PCOLDPSW+(1*4)(5)
         MVI   PGMPSW+(1*9)+8,C' '
         TR    PGMPSW+(1*9)(8),HEXTRTAB
                                                                SPACE
         UNPK  PGMPSW+(2*9)(9),PCOLDPSW+(2*4)(5)
         MVI   PGMPSW+(2*9)+8,C' '
         TR    PGMPSW+(2*9)(8),HEXTRTAB
                                                                SPACE
         UNPK  PGMPSW+(3*9)(9),PCOLDPSW+(3*4)(5)
         MVI   PGMPSW+(3*9)+8,C' '
         TR    PGMPSW+(3*9)(8),HEXTRTAB
                                                                SPACE
         LA    R0,L'PROGMSG     R0 <== length of message
         LA    R1,PROGMSG       R1 --> the message text itself
         BAL   R2,MSG           Go display this message

         BR    R13              Return to caller
                                                                SPACE 4
PROGMSG  DS   0CL66
         DC    CL20'PROGRAM CHECK! CODE '
PROGCODE DC    CL4'hhhh'
PGMCOMMA DC    CL1','
         DC    CL5' PSW '
PGMPSW   DC    CL36'hhhhhhhh hhhhhhhh hhhhhhhh hhhhhhhh '
                                                                EJECT
***********************************************************************
*                    VERIFICATION ROUTINE
***********************************************************************
                                                                SPACE
VERISUB  DS    0H
*
**       Loop through the VERIFY TABLE...
*
                                                                SPACE
         LA    R1,VERIFTAB      R1 --> Verify table
         LA    R2,VERIFLEN      R2 <== Number of entries
         BASR  R3,0             Set top of loop
                                                                SPACE
         LM    R4,R6,0(R1)      Load verify table values
         BAS   R7,VERIFY        Verify results
         LA    R1,12(,R1)       Next verify table entry
         BCTR  R2,R3            Loop through verify table
                                                                SPACE
         CLI   FAILFLAG,X'00'   Did all tests verify okay?
         BER   R13              Yes, return to caller
         B     FAIL             No, load FAILURE disabled wait PSW
                                                                SPACE 6
*
**       Loop through the ACTUAL / EXPECTED results...
*
                                                                SPACE
VERIFY   BASR  R8,0             Set top of loop
                                                                SPACE
         CLC   0(16,R4),48(R5)  Actual results == Expected results?
         BNE   VERIFAIL         No, show failure
VERINEXT LA    R4,16(,R4)       Next actual result
         LA    R5,64(,R5)       Next expected result
         BCTR  R6,R8            Loop through results
                                                                SPACE
         BR    R7               Return to caller
                                                                EJECT
***********************************************************************
*                    Report the failure...
***********************************************************************
                                                                SPACE
VERIFAIL STM   R0,R5,SAVER0R5   Save registers
         MVI   FAILFLAG,X'FF'   Remember verification failure
*
**       First, show them the description...
*
         MVC   FAILDESC,0(R5)   Save results/test description
         LA    R0,L'FAILMSG1    R0 <== length of message
         LA    R1,FAILMSG1      R1 --> the message text itself
         BAL   R2,MSG           Go display this message
*
**       Save address of actual and expected results
*
         ST    R4,AACTUAL       Save A(actual results)
         LA    R5,48(,R5)       R5 ==> expected results
         ST    R5,AEXPECT       Save A(expected results)
*
**       Format and show them the EXPECTED ("Want") results...
*
         MVC   WANTGOT,=CL6'Want: '
         UNPK  FAILADR(L'FAILADR+1),AEXPECT(L'AEXPECT+1)
         MVI   BLANKEQ,C' '
         TR    FAILADR,HEXTRTAB
                                                                SPACE
         UNPK  FAILVALS+(0*9)(9),(0*4)(5,R5)
         MVI   FAILVALS+(0*9)+8,C' '
         TR    FAILVALS+(0*9)(8),HEXTRTAB
                                                                SPACE
         UNPK  FAILVALS+(1*9)(9),(1*4)(5,R5)
         MVI   FAILVALS+(1*9)+8,C' '
         TR    FAILVALS+(1*9)(8),HEXTRTAB
                                                                SPACE
         UNPK  FAILVALS+(2*9)(9),(2*4)(5,R5)
         MVI   FAILVALS+(2*9)+8,C' '
         TR    FAILVALS+(2*9)(8),HEXTRTAB
                                                                SPACE
         UNPK  FAILVALS+(3*9)(9),(3*4)(5,R5)
         MVI   FAILVALS+(3*9)+8,C' '
         TR    FAILVALS+(3*9)(8),HEXTRTAB
                                                                SPACE
         LA    R0,L'FAILMSG2    R0 <== length of message
         LA    R1,FAILMSG2      R1 --> the message text itself
         BAL   R2,MSG           Go display this message
                                                                EJECT
*
**       Format and show them the ACTUAL ("Got") results...
*
         MVC   WANTGOT,=CL6'Got:  '
         UNPK  FAILADR(L'FAILADR+1),AACTUAL(L'AACTUAL+1)
         MVI   BLANKEQ,C' '
         TR    FAILADR,HEXTRTAB
                                                                SPACE
         UNPK  FAILVALS+(0*9)(9),(0*4)(5,R4)
         MVI   FAILVALS+(0*9)+8,C' '
         TR    FAILVALS+(0*9)(8),HEXTRTAB
                                                                SPACE
         UNPK  FAILVALS+(1*9)(9),(1*4)(5,R4)
         MVI   FAILVALS+(1*9)+8,C' '
         TR    FAILVALS+(1*9)(8),HEXTRTAB
                                                                SPACE
         UNPK  FAILVALS+(2*9)(9),(2*4)(5,R4)
         MVI   FAILVALS+(2*9)+8,C' '
         TR    FAILVALS+(2*9)(8),HEXTRTAB
                                                                SPACE
         UNPK  FAILVALS+(3*9)(9),(3*4)(5,R4)
         MVI   FAILVALS+(3*9)+8,C' '
         TR    FAILVALS+(3*9)(8),HEXTRTAB
                                                                SPACE
         LA    R0,L'FAILMSG2    R0 <== length of message
         LA    R1,FAILMSG2      R1 --> the message text itself
         BAL   R2,MSG           Go display this message
                                                                SPACE
         LM    R0,R5,SAVER0R5   Restore registers
         B     VERINEXT         Continue with verification...
                                                                SPACE 3
FAILMSG1 DS   0CL68
         DC    CL20'COMPARISON FAILURE! '
FAILDESC DC    CL48'(description)'
                                                                SPACE 2
FAILMSG2 DS   0CL53
WANTGOT  DC    CL6' '           'Want: ' -or- 'Got:  '
FAILADR  DC    CL8'AAAAAAAA'
BLANKEQ  DC    CL3' = '
FAILVALS DC    CL36'hhhhhhhh hhhhhhhh hhhhhhhh hhhhhhhh '
                                                                SPACE 2
AEXPECT  DC    F'0'             ==> Expected ("Want") results
AACTUAL  DC    F'0'             ==> Actual ("Got") results
SAVER0R5 DC    6F'0'            Registers R0 - R5 save area
CHARHEX  DC    CL16'0123456789ABCDEF'
HEXTRTAB EQU   CHARHEX-X'F0'    Hexadecimal translation table
FAILFLAG DC    X'00'            FF = Fail, 00 = Success
                                                                EJECT
***********************************************************************
*        Issue HERCULES MESSAGE pointed to by R1, length in R0
***********************************************************************
                                                                SPACE
MSG      CH    R0,=H'0'               Do we even HAVE a message?
         BNHR  R2                     No, ignore
                                                                SPACE
         STM   R0,R2,MSGSAVE          Save registers
                                                                SPACE
         CH    R0,=AL2(L'MSGMSG)      Message length within limits?
         BNH   MSGOK                  Yes, continue
         LA    R0,L'MSGMSG            No, set to maximum
                                                                SPACE
MSGOK    LR    R2,R0                  Copy length to work register
         BCTR  R2,0                   Minus-1 for execute
         EX    R2,MSGMVC              Copy message to O/P buffer
                                                                SPACE
         LA    R2,1+L'MSGCMD(,R2)     Calculate true command length
         LA    R1,MSGCMD              Point to true command
                                                                SPACE
         DC    X'83',X'12',X'0008'    Issue Hercules Diagnose X'008'
         BZ    MSGRET                 Return if successful
         DC    H'0'                   CRASH for debugging purposes
                                                                SPACE
MSGRET   LM    R0,R2,MSGSAVE          Restore registers
         BR    R2                     Return to caller
                                                                SPACE 6
MSGSAVE  DC    3F'0'                  Registers save area
MSGMVC   MVC   MSGMSG(0),0(R1)        Executed instruction
                                                                SPACE 2
MSGCMD   DC    C'MSGNOH * '           *** HERCULES MESSAGE COMMAND ***
MSGMSG   DC    CL95' '                The message text to be displayed
                                                                EJECT
***********************************************************************
*                         VERIFY TABLE
***********************************************************************
*
*        A(actual results), A(expected results), A(#of results)
*
***********************************************************************
                                                                SPACE
VERIFTAB DC    0F'0'
         DC    A(SBFPOUT)
         DC    A(SBFPOUT_GOOD)
         DC    A(SBFPOUT_NUM)
*
         DC    A(SBFPOCC)
         DC    A(SBFPOCC_GOOD)
         DC    A(SBFPOCC_NUM)
*
         DC    A(LBFPOUT)
         DC    A(LBFPOUT_GOOD)
         DC    A(LBFPOUT_NUM)
*
         DC    A(LBFPOCC)
         DC    A(LBFPOCC_GOOD)
         DC    A(LBFPOCC_NUM)
*
         DC    A(XBFPOUT)
         DC    A(XBFPOUT_GOOD)
         DC    A(XBFPOUT_NUM)
*
         DC    A(XBFPOCC)
         DC    A(XBFPOCC_GOOD)
         DC    A(XBFPOCC_NUM)
*
VERIFLEN EQU   (*-VERIFTAB)/12    #of entries in verify table
                                                                EJECT
         END
