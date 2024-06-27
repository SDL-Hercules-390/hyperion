 TITLE 'bfp-013-comps: Test IEEE Compare, Compare And Signal'
***********************************************************************
*
*Testcase IEEE Compare and Compare And Signal.
*  Exhaustively test results from the Compare and Compare And Signal
*  instructions.  The Condition Code and FPC flags are saved for each
*  test value pair.  If an IEEE trap occurs, the DXC code is saved
*  instead of the Condition Code.
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
*                        bfp-013-comps.asm
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
*  Each possible comparison class is tested, for a total of 64 test
*  value pairs for each of the five instruction precisions and formats.
*  Each instruction precision and format is tested twice, once with
*  exceptions non-trappable and once with exceptions trappable.
*
*  One list of input values is provided.  Each value is tested against
*  every other value in the same list.
*
*  Each result is two bytes, one for the CC and one for FPC flags.  If
*  a trap occurs, the DXC code replaces the CC.
*
* Tests 5 COMPARE, 5 COMPARE AND SIGNAL
*   COMPARE (BFP short, RRE) CEBR
*   COMPARE (BFP short, RXE) CEB
*   COMPARE (BFP long, RRE) CDBR
*   COMPARE (BFP long, RXE) CDB
*   COMPARE (BFP extended, RRE) CXBR
*   COMPARE AND SIGNAL (BFP short, RRE) KEBR
*   COMPARE AND SIGNAL (BFP short, RXE) KEB
*   COMPARE AND SIGNAL (BFP long, RRE) KDBR
*   COMPARE AND SIGNAL (BFP long, RXE) KDB
*   COMPARE AND SIGNAL (BFP extended, RRE) KXBR
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
BFPCOMPS START 0
STRTLABL EQU   *
R0       EQU   0                   Work register for cc extraction
R1       EQU   1                   Current Test Data Class mask
R2       EQU   2                   Holds count of test input values
R3       EQU   3                   Points to next test input value(s)
R4       EQU   4                   Available
R5       EQU   5                   Available
R6       EQU   6                   Available
R7       EQU   7                   Ptr to next Compare result
R8       EQU   8                   Ptr to next Compare and Signal reslt
R9       EQU   9                   Available
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
         STCTL R0,R0,CTLR0    Store CR0 to enable AFP
         OI    CTLR0+1,X'04'  Turn on AFP bit
         LCTL  R0,R0,CTLR0    Reload updated CR0
*
         LA    R10,SHORTC    Point to short BFP parameters
         BAS   R13,SBFPCOMP  Perform short BFP Compare
*
         LA    R10,LONGC    Point to long BFP parameters
         BAS   R13,LBFPCOMP  Perform long BFP Compare
*
         LA    R10,XTNDC     Point to extended BFP parameters
         BAS   R13,XBFPCOMP  Perform extended BFP Compare
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
FPCREGNT DC    X'00000000'  FPCR, trap all IEEE exceptions, zero flags
FPCREGTR DC    X'F8000000'  FPCR, trap no IEEE exceptions, zero flags
*
* Input values parameter list, four fullwords for each test data set
*      1) Count,
*      2) Address of inputs,
*      3) Address to place results, and
*      4) Address to place DXC/Flags/cc values.
*
         ORG   STRTLABL+X'300'  Enable run-time replacement
SHORTC   DS    0F            Inputs for short BFP Compare
         DC    A(SBFPCT)
         DC    A(SBFPIN)
         DC    A(SBFPCCC)
         DC    A(SBFPCSCC)
*
LONGC    DS    0F            Inputs for long BFP Compare
         DC    A(LBFPCT)
         DC    A(LBFPIN)
         DC    A(LBFPCCC)
         DC    A(LBFPCSCC)
*
XTNDC    DS    0F            Inputs for extended BFP Compare
         DC    A(XBFPCT)
         DC    A(XBFPIN)
         DC    A(XBFPCCC)
         DC    A(XBFPCSCC)
         EJECT
***********************************************************************
*
* Compare short BFP inputs to each possible class of short BFP.  Eight
* pairs of results are generated for each input: one with all
* exceptions non-trappable, and the second with all exceptions
* trappable.   The CC and FPC flags are stored for each result that
* does not cause a trap.  The DXC code and  FPC flags are stored for
* each result that traps.
*
***********************************************************************
         SPACE 2
SBFPCOMP DS    0H            Compare short BFP inputs
         LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR8,0(,R3)   Get short BFP left-hand test value
         LM    R4,R5,0(R10)  Get count and start of right-hand side
         XR    R9,R9         Reference zero value for Set Program Mask
         BASR  R6,0          Set top of inner loop
*
* top of loop to test left-hand value against each input
*
         LE    FPR1,0(,R5)   Get right-hand side of compare
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         SPM   R9            Clear condition code
         CEBR  FPR8,FPR1     Compare And Signal floating point nrs RRE
         STFPC 0(R7)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R7)     Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         SPM   R9            Clear condition code
         CEBR  FPR8,FPR1     Compare And Signal floating point nrs RRE
         STFPC 4(R7)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R7)     Save condition code in results table
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         SPM   R9            Clear condition code
         CEB   FPR8,0(,R5)   Compare And Signal floating point nrs RXE
         STFPC 8(R7)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,11(,R7)    Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         SPM   R9            Clear condition code
         CEB   FPR8,0(,R5)   Compare And Signal floating point nrs RXE
         STFPC 12(R7)        Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,15(,R7)    Save condition code in results table
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         SPM   R9            Clear condition code
         KEBR  FPR8,FPR1     Compare And Signal floating point nrs RRE
         STFPC 0(R8)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R8)     Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         SPM   R9            Clear condition code
         KEBR  FPR8,FPR1     Compare And Signal floating point nrs RRE
         STFPC 4(R8)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R8)     Save condition code in results table
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         SPM   R9            Clear condition code
         KEB   FPR8,0(,R5)   Compare And Signal floating point nrs RXE
         STFPC 8(R8)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,11(,R8)    Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         SPM   R9            Clear condition code
         KEB   FPR8,0(,R5)   Compare And Signal floating point nrs RXE
         STFPC 12(R8)        Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,15(,R8)    Save condition code in results table
*
         LA    R5,4(,R5)     Point to next right-hand value
         LA    R7,16(,R7)    Point to next CC/DXC/FPR CEB result area
         LA    R8,16(,R8)    Point to next CC/DXC/FPR KEB result area
         BCTR  R4,R6         Loop through right-hand values
*
         LA    R3,4(,R3)     Point to next left-hand value
         BCTR  R2,R12        Loop through left-hand values
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Compare long BFP inputs to each possible class of long BFP.  Eight
* pairs of results are generated for each input: one with all
* exceptions non-trappable, and the second with all exceptions
* trappable.   The CC and FPC flags are stored for each result that
* does not cause a trap.  The DXC code and  FPC flags are stored for
* each result that traps.
*
***********************************************************************
         SPACE 2
LBFPCOMP DS    0H            Compare long BFP inputs
         LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR8,0(,R3)   Get long BFP left-hand test value
         LM    R4,R5,0(R10)  Get count and start of right-hand side
         XR    R9,R9         Reference zero value for Set Program Mask
         BASR  R6,0          Set top of inner loop
*
* top of loop to test left-hand value against each input
*
         LD    FPR1,0(,R5)   Get right-hand side of compare
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         SPM   R9            Clear condition code
         CDBR  FPR8,FPR1     Compare And Signal floating point nrs RRE
         STFPC 0(R7)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R7)     Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         SPM   R9            Clear condition code
         CDBR  FPR8,FPR1     Compare And Signal floating point nrs RRE
         STFPC 4(R7)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R7)     Save condition code in results table
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         SPM   R9            Clear condition code
         CDB   FPR8,0(,R5)   Compare And Signal floating point nrs RXE
         STFPC 8(R7)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,11(,R7)    Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         SPM   R9            Clear condition code
         CDB   FPR8,0(,R5)   Compare And Signal floating point nrs RXE
         STFPC 12(R7)        Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,15(,R7)    Save condition code in results table
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         SPM   R9            Clear condition code
         KDBR  FPR8,FPR1     Compare And Signal floating point nrs RRE
         STFPC 0(R8)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R8)     Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         SPM   R9            Clear condition code
         KDBR  FPR8,FPR1     Compare And Signal floating point nrs RRE
         STFPC 4(R8)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R8)     Save condition code in results table
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         SPM   R9            Clear condition code
         KDB   FPR8,0(,R5)   Compare And Signal floating point nrs RXE
         STFPC 8(R8)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,11(,R8)    Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         SPM   R9            Clear condition code
         KDB   FPR8,0(,R5)   Compare And Signal floating point nrs RXE
         STFPC 12(R8)        Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,15(,R8)    Save condition code in results table
*
         LA    R5,8(,R5)     Point to next right-hand value
         LA    R7,16(,R7)    Point to next CC/DXC/FPR CDB result area
         LA    R8,16(,R8)    Point to next CC/DXC/FPR KDB result area
         BCTR  R4,R6         Loop through right-hand values
*
         LA    R3,8(,R3)     Point to next left-hand value
         BCTR  R2,R12        Loop through left-hand values
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Compare long BFP inputs to each possible class of long BFP.  Eight
* pairs of results are generated for each input: one with all
* exceptions non-trappable, and the second with all exceptions
* trappable.   The CC and FPC flags are stored for each result that
* does not cause a trap.  The DXC code and  FPC flags are stored for
* each result that traps.
*
***********************************************************************
         SPACE 2
XBFPCOMP DS    0H            Compare long BFP inputs
         LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR8,0(,R3)   Get long BFP left-hand test value part 1
         LD    FPR10,8(,R3)  Get long BFP left-hand test value part 2
         LM    R4,R5,0(R10)  Get count and start of right-hand side
         XR    R9,R9         Reference zero value for Set Program Mask
         BASR  R6,0          Set top of inner loop
*
* top of loop to test left-hand value against each input
*
         LD    FPR1,0(,R5)   Get right-hand side of compare part 1
         LD    FPR3,8(,R5)   Get right-hand side of compare part 2
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         SPM   R9            Clear condition code
         CXBR  FPR8,FPR1     Compare And Signal floating point nrs RRE
         STFPC 0(R7)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R7)     Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         SPM   R9            Clear condition code
         CXBR  FPR8,FPR1     Compare And Signal floating point nrs RRE
         STFPC 4(R7)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R7)     Save condition code in results table
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         SPM   R9            Clear condition code
         KXBR  FPR8,FPR1     Compare And Signal floating point nrs RRE
         STFPC 0(R8)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R8)     Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         SPM   R9            Clear condition code
         KXBR  FPR8,FPR1     Compare And Signal floating point nrs RRE
         STFPC 4(R8)         Store FPC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R8)     Save condition code in results table
*
         LA    R5,16(,R5)    Point to next right-hand value
         LA    R7,16(,R7)    Point to next CC/DXC/FPR CDB result area
         LA    R8,16(,R8)    Point to next CC/DXC/FPR KDB result area
         BCTR  R4,R6         Loop through right-hand values
*
         LA    R3,16(,R3)    Point to next left-hand value
         BCTR  R2,R12        Loop through left-hand values
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Input test values.  Each input is tested against every input in the
* list, which means the eight values result in 64 tests.
*
***********************************************************************
         SPACE 2
*
*  Short BFP Input values
*
SBFPIN   DS    0F            Ensure fullword alignment for input table
         DC    X'FF800000'      -infinity
         DC    X'BF800000'      -1
         DC    X'80000000'      -0
         DC    X'00000000'      +0
         DC    X'3F800000'      +1
         DC    X'7F800000'      +infinity
         DC    X'FFC00000'      -QNaN
         DC    X'7F810000'      +SNaN
SBFPCT   EQU   (*-SBFPIN)/4  Count of input values
*
*  Long BFP Input values
*
LBFPIN   DS    0D            Ensure doubleword alignment for inputs
         DC    X'FFF0000000000000'      -infinity
         DC    X'BFF0000000000000'      -1
         DC    X'8000000000000000'      -0
         DC    X'0000000000000000'      +0
         DC    X'3FF0000000000000'      +1
         DC    X'7FF0000000000000'      +infinity
         DC    X'7FF8000000000000'      -QNaN
         DC    X'7FF0100000000000'      +SNaN
LBFPCT   EQU   (*-LBFPIN)/8  Count of input values
*
*  Long BFP Input values
*
XBFPIN   DS    0D            Ensure doubleword alignment for inputs
         DC    X'FFFF0000000000000000000000000000'      -infinity
         DC    X'BFFF0000000000000000000000000000'      -1
         DC    X'80000000000000000000000000000000'      -0
         DC    X'00000000000000000000000000000000'      +0
         DC    X'3FFF0000000000000000000000000000'      +1
         DC    X'7FFF0000000000000000000000000000'      +infinity
         DC    X'FFFF8000000000000000000000000000'      -QNaN
         DC    X'7FFF0100000000000000000000000000'      +SNaN
XBFPCT   EQU   (*-XBFPIN)/16  Count of input values
         EJECT
***********************************************************************
*                 ACTUAL results saved here
***********************************************************************
*
*               Locations for ACTUAL results
*
*
* For each test result, four bytes are generated as follows
*  1 - non-trap CC
*  2 - non-trap FPC flags
*  3 - trappable CC
*  4 - trappable FPC flags
*
* Only for Compare involving SNaN and Compare And Signal involving any
* NaN will the trappable and non-trap results be different.
*
* For short and long instruction precisions, the RRE format is tested
* first, followed by the RXE format.  Extended precision only exists in
* RRE format.
*
SBFPCCC  EQU   STRTLABL+X'1000'    Integer short Compare results
*                                  ..room for 64 tests, all used
*
SBFPCSCC EQU   STRTLABL+X'1400'    Integer short Compare & Sig. results
*                                  ..room for 64 tests, all used
*
LBFPCCC  EQU   STRTLABL+X'2000'    Integer long Compare results
*                                  ..room for 64 tests, all used
*
LBFPCSCC EQU   STRTLABL+X'2400'    Integer lon Compare & Sig. results
*                                  ..room for 64 tests, all used
*
XBFPCCC  EQU   STRTLABL+X'3000'    Integer extended Compare results
*                                  ..room for 64 tests, all used
*
XBFPCSCC EQU   STRTLABL+X'3400'    Integer ext'd Compare & Sig. results
*                                  ..room for 64 tests, all used
*
         EJECT
***********************************************************************
*                    EXPECTED results
***********************************************************************
*
         ORG   STRTLABL+X'4000'   (past end of actual results)
*
SBFPCCC_GOOD EQU *
 DC CL48'CEBR/CEB -infinity / -infinity'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CEBR/CEB -infinity / -1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB -infinity / -0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB -infinity / +0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB -infinity / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB -infinity / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB -infinity / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CEBR/CEB -infinity / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB -1 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB -1 / -1'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CEBR/CEB -1 / -0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB -1 / +0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB -1 / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB -1 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB -1 / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CEBR/CEB -1 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB -0 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB -0 / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB -0 / -0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CEBR/CEB -0 / +0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CEBR/CEB -0 / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB -0 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB -0 / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CEBR/CEB -0 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB +0 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB +0 / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB +0 / -0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CEBR/CEB +0 / +0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CEBR/CEB +0 / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB +0 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB +0 / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CEBR/CEB +0 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB +1 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB +1 / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB +1 / -0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB +1 / +0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB +1 / +1'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CEBR/CEB +1 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CEBR/CEB +1 / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CEBR/CEB +1 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB +infinity / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB +infinity / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB +infinity / -0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB +infinity / +0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB +infinity / +1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CEBR/CEB +infinity / +infinity'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CEBR/CEB +infinity / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CEBR/CEB +infinity / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB -QNaN / -infinity'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CEBR/CEB -QNaN / -1'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CEBR/CEB -QNaN / -0'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CEBR/CEB -QNaN / +0'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CEBR/CEB -QNaN / +1'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CEBR/CEB -QNaN / +infinity'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CEBR/CEB -QNaN / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CEBR/CEB -QNaN / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB +SNaN / -infinity'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB +SNaN / -1'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB +SNaN / -0'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB +SNaN / +0'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB +SNaN / +1'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB +SNaN / +infinity'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB +SNaN / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CEBR/CEB +SNaN / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
SBFPCCC_NUM EQU (*-SBFPCCC_GOOD)/64
*
*
SBFPCSCC_GOOD EQU *
 DC CL48'KEBR/KEB -infinity / -infinity'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KEBR/KEB -infinity / -1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB -infinity / -0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB -infinity / +0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB -infinity / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB -infinity / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB -infinity / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB -infinity / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB -1 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB -1 / -1'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KEBR/KEB -1 / -0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB -1 / +0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB -1 / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB -1 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB -1 / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB -1 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB -0 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB -0 / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB -0 / -0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KEBR/KEB -0 / +0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KEBR/KEB -0 / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB -0 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB -0 / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB -0 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +0 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB +0 / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB +0 / -0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KEBR/KEB +0 / +0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KEBR/KEB +0 / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB +0 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB +0 / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +0 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +1 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB +1 / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB +1 / -0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB +1 / +0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB +1 / +1'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KEBR/KEB +1 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KEBR/KEB +1 / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +1 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +infinity / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB +infinity / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB +infinity / -0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB +infinity / +0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB +infinity / +1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KEBR/KEB +infinity / +infinity'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KEBR/KEB +infinity / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +infinity / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB -QNaN / -infinity'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB -QNaN / -1'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB -QNaN / -0'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB -QNaN / +0'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB -QNaN / +1'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB -QNaN / +infinity'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB -QNaN / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB -QNaN / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +SNaN / -infinity'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +SNaN / -1'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +SNaN / -0'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +SNaN / +0'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +SNaN / +1'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +SNaN / +infinity'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +SNaN / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KEBR/KEB +SNaN / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
SBFPCSCC_NUM EQU (*-SBFPCSCC_GOOD)/64
*
*
LBFPCCC_GOOD EQU *
 DC CL48'CDBR/CDB -infinity / -infinity'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CDBR/CDB -infinity / -1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB -infinity / -0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB -infinity / +0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB -infinity / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB -infinity / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB -infinity / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CDBR/CDB -infinity / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB -1 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB -1 / -1'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CDBR/CDB -1 / -0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB -1 / +0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB -1 / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB -1 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB -1 / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CDBR/CDB -1 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB -0 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB -0 / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB -0 / -0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CDBR/CDB -0 / +0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CDBR/CDB -0 / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB -0 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB -0 / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CDBR/CDB -0 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB +0 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB +0 / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB +0 / -0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CDBR/CDB +0 / +0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CDBR/CDB +0 / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB +0 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB +0 / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CDBR/CDB +0 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB +1 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB +1 / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB +1 / -0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB +1 / +0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB +1 / +1'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CDBR/CDB +1 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'CDBR/CDB +1 / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CDBR/CDB +1 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB +infinity / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB +infinity / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB +infinity / -0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB +infinity / +0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB +infinity / +1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CDBR/CDB +infinity / +infinity'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CDBR/CDB +infinity / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CDBR/CDB +infinity / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB -QNaN / -infinity'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CDBR/CDB -QNaN / -1'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CDBR/CDB -QNaN / -0'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CDBR/CDB -QNaN / +0'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CDBR/CDB -QNaN / +1'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CDBR/CDB -QNaN / +infinity'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CDBR/CDB -QNaN / -QNaN'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'CDBR/CDB -QNaN / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB +SNaN / -infinity'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB +SNaN / -1'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB +SNaN / -0'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB +SNaN / +0'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB +SNaN / +1'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB +SNaN / +infinity'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB +SNaN / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'CDBR/CDB +SNaN / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
LBFPCCC_NUM EQU (*-LBFPCCC_GOOD)/64
*
*
LBFPCSCC_GOOD EQU *
 DC CL48'KDBR/KDB -infinity / -infinity'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KDBR/KDB -infinity / -1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB -infinity / -0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB -infinity / +0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB -infinity / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB -infinity / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB -infinity / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB -infinity / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB -1 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB -1 / -1'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KDBR/KDB -1 / -0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB -1 / +0'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB -1 / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB -1 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB -1 / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB -1 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB -0 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB -0 / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB -0 / -0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KDBR/KDB -0 / +0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KDBR/KDB -0 / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB -0 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB -0 / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB -0 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +0 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB +0 / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB +0 / -0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KDBR/KDB +0 / +0'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KDBR/KDB +0 / +1'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB +0 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB +0 / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +0 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +1 / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB +1 / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB +1 / -0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB +1 / +0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB +1 / +1'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KDBR/KDB +1 / +infinity'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'KDBR/KDB +1 / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +1 / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +infinity / -infinity'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB +infinity / -1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB +infinity / -0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB +infinity / +0'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB +infinity / +1'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'KDBR/KDB +infinity / +infinity'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'KDBR/KDB +infinity / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +infinity / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB -QNaN / -infinity'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB -QNaN / -1'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB -QNaN / -0'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB -QNaN / +0'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB -QNaN / +1'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB -QNaN / +infinity'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB -QNaN / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB -QNaN / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +SNaN / -infinity'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +SNaN / -1'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +SNaN / -0'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +SNaN / +0'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +SNaN / +1'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +SNaN / +infinity'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +SNaN / -QNaN'
 DC XL16'00800003F800800000800003F8008000'
 DC CL48'KDBR/KDB +SNaN / +SNaN'
 DC XL16'00800003F800800000800003F8008000'
LBFPCSCC_NUM EQU (*-LBFPCSCC_GOOD)/64
*
*
XBFPCCC_GOOD EQU *
 DC CL48'CXBR -infinity / -infinity'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'CXBR -infinity / -1'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR -infinity / -0'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR -infinity / +0'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR -infinity / +1'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR -infinity / +infinity'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR -infinity / -QNaN'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'CXBR -infinity / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR -1 / -infinity'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR -1 / -1'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'CXBR -1 / -0'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR -1 / +0'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR -1 / +1'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR -1 / +infinity'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR -1 / -QNaN'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'CXBR -1 / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR -0 / -infinity'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR -0 / -1'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR -0 / -0'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'CXBR -0 / +0'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'CXBR -0 / +1'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR -0 / +infinity'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR -0 / -QNaN'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'CXBR -0 / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR +0 / -infinity'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR +0 / -1'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR +0 / -0'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'CXBR +0 / +0'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'CXBR +0 / +1'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR +0 / +infinity'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR +0 / -QNaN'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'CXBR +0 / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR +1 / -infinity'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR +1 / -1'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR +1 / -0'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR +1 / +0'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR +1 / +1'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'CXBR +1 / +infinity'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'CXBR +1 / -QNaN'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'CXBR +1 / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR +infinity / -infinity'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR +infinity / -1'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR +infinity / -0'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR +infinity / +0'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR +infinity / +1'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'CXBR +infinity / +infinity'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'CXBR +infinity / -QNaN'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'CXBR +infinity / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR -QNaN / -infinity'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'CXBR -QNaN / -1'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'CXBR -QNaN / -0'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'CXBR -QNaN / +0'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'CXBR -QNaN / +1'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'CXBR -QNaN / +infinity'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'CXBR -QNaN / -QNaN'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'CXBR -QNaN / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR +SNaN / -infinity'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR +SNaN / -1'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR +SNaN / -0'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR +SNaN / +0'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR +SNaN / +1'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR +SNaN / +infinity'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR +SNaN / -QNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'CXBR +SNaN / +SNaN'
 DC XL16'00800003F80080000000000000000000'
XBFPCCC_NUM EQU (*-XBFPCCC_GOOD)/64
*
*
XBFPCSCC_GOOD EQU *
 DC CL48'KXBR -infinity / -infinity'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'KXBR -infinity / -1'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR -infinity / -0'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR -infinity / +0'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR -infinity / +1'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR -infinity / +infinity'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR -infinity / -QNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR -infinity / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR -1 / -infinity'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR -1 / -1'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'KXBR -1 / -0'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR -1 / +0'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR -1 / +1'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR -1 / +infinity'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR -1 / -QNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR -1 / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR -0 / -infinity'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR -0 / -1'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR -0 / -0'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'KXBR -0 / +0'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'KXBR -0 / +1'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR -0 / +infinity'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR -0 / -QNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR -0 / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +0 / -infinity'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR +0 / -1'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR +0 / -0'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'KXBR +0 / +0'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'KXBR +0 / +1'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR +0 / +infinity'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR +0 / -QNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +0 / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +1 / -infinity'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR +1 / -1'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR +1 / -0'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR +1 / +0'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR +1 / +1'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'KXBR +1 / +infinity'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'KXBR +1 / -QNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +1 / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +infinity / -infinity'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR +infinity / -1'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR +infinity / -0'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR +infinity / +0'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR +infinity / +1'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'KXBR +infinity / +infinity'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'KXBR +infinity / -QNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +infinity / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR -QNaN / -infinity'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR -QNaN / -1'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR -QNaN / -0'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR -QNaN / +0'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR -QNaN / +1'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR -QNaN / +infinity'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR -QNaN / -QNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR -QNaN / +SNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +SNaN / -infinity'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +SNaN / -1'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +SNaN / -0'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +SNaN / +0'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +SNaN / +1'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +SNaN / +infinity'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +SNaN / -QNaN'
 DC XL16'00800003F80080000000000000000000'
 DC CL48'KXBR +SNaN / +SNaN'
 DC XL16'00800003F80080000000000000000000'
XBFPCSCC_NUM EQU (*-XBFPCSCC_GOOD)/64
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
         DC    A(SBFPCCC)
         DC    A(SBFPCCC_GOOD)
         DC    A(SBFPCCC_NUM)
*
         DC    A(SBFPCSCC)
         DC    A(SBFPCSCC_GOOD)
         DC    A(SBFPCSCC_NUM)
*
         DC    A(LBFPCCC)
         DC    A(LBFPCCC_GOOD)
         DC    A(LBFPCCC_NUM)
*
         DC    A(LBFPCSCC)
         DC    A(LBFPCSCC_GOOD)
         DC    A(LBFPCSCC_NUM)
*
         DC    A(XBFPCCC)
         DC    A(XBFPCCC_GOOD)
         DC    A(XBFPCCC_NUM)
*
         DC    A(XBFPCSCC)
         DC    A(XBFPCSCC_GOOD)
         DC    A(XBFPCSCC_NUM)
*
VERIFLEN EQU   (*-VERIFTAB)/12    #of entries in verify table
                                                                EJECT
         END
