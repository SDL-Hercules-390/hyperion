  TITLE 'bfp-016-add: Test IEEE Add'
***********************************************************************
*
*Testcase IEEE ADD
*  Test case capability includes IEEE exceptions trappable and
*  otherwise. Test results, FPCR flags, the Condition code, and any
*  DXC are saved for all tests.
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
*                         bfp-016-add.asm
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
* Tests the following three conversion instructions
*   ADD (short BFP, RRE)
*   ADD (long BFP, RRE)
*   ADD (extended BFP, RRE)
*   ADD (short BFP, RXE)
*   ADD (long BFP, RXE)
*
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R
* commands.
*
* Test Case Order
* 1) Short BFP basic tests, including traps and NaN propagation
* 2) Short BFP finite number tests, incl. traps and scaling
* 3) Short BFP FPC-controlled rounding mode exhaustive tests
* 4) Long BFP basic tests, including traps and NaN propagation
* 5) Long BFP finite number tests, incl. traps and scaling
* 6) Long BFP FPC-controlled rounding mode exhaustive tests
* 7) Extended BFP basic tests, including traps and NaN propagation
* 8) Extended BFP finite number tests, incl. traps and scaling
* 9) Extended BFP FPC-controlled rounding mode exhaustive tests
*
* Three input test sets are provided each for short, long, and
*   extended BFP inputs.  Test values are the same for each precision
*   for most tests.  Overflow and underflow each require precision-
*   dependent test values.
*
* Also tests the following floating point support instructions
*   LOAD  (Short)
*   LOAD  (Long)
*   LFPC  (Load Floating Point Control Register)
*   SRNMB (Set BFP Rounding Mode 3-bit)
*   STORE (Short)
*   STORE (Long)
*   STFPC (Store Floating Point Control Register)
*
***********************************************************************
         EJECT
*
*  Note: for compatibility with the z/CMS test rig, do not change
*  or use R11, R14, or R15.  Everything else is fair game.
*
BFPADD   START 0
STRTLABL EQU   *
R0       EQU   0                   Work register for cc extraction
R1       EQU   1
R2       EQU   2                   Holds count of test input values
R3       EQU   3                   Points to next test input value(s)
R4       EQU   4                   Rounding tests inner loop control
R5       EQU   5                   Rounding tests outer loop control
R6       EQU   6                   Rounding tests top of inner loop
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
         EJECT
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
         LA    R10,SHORTNF   Point to short BFP non-finite inputs
         BAS   R13,SBFPNF    Add short BFP non-finites
         LA    R10,SHORTF    Point to short BFP finite inputs
         BAS   R13,SBFPF     Add short BFP finites
         LA    R10,RMSHORTS  Point to short BFP rounding mode tests
         BAS   R13,SBFPRM    Add short BFP for rounding tests
*
         LA    R10,LONGNF    Point to long BFP non-finite inputs
         BAS   R13,LBFPNF    Add long BFP non-finites
         LA    R10,LONGF     Point to long BFP finite inputs
         BAS   R13,LBFPF     Add long BFP finites
         LA    R10,RMLONGS   Point to long  BFP rounding mode tests
         BAS   R13,LBFPRM    Add long BFP for rounding tests
*
         LA    R10,XTNDNF    Point to extended BFP non-finite inputs
         BAS   R13,XBFPNF    Add extended BFP non-finites
         LA    R10,XTNDF     Point to ext'd BFP finite inputs
         BAS   R13,XBFPF     Add ext'd BFP finites
         LA    R10,RMXTNDS   Point to ext'd BFP rounding mode tests
         BAS   R13,XBFPRM    Add ext'd BFP for rounding tests
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
SHORTNF  DS    0F           Input pairs for short BFP non-finite tests
         DC    A(SBFPNFCT)
         DC    A(SBFPNFIN)
         DC    A(SBFPNFOT)
         DC    A(SBFPNFFL)
*
SHORTF   DS    0F           Input pairs for short BFP finite tests
         DC    A(SBFPCT)
         DC    A(SBFPIN)
         DC    A(SBFPOUT)
         DC    A(SBFPFLGS)
*
RMSHORTS DS    0F           Input pairs for short BFP rounding testing
         DC    A(SBFPRMCT)
         DC    A(SBFPINRM)
         DC    A(SBFPRMO)
         DC    A(SBFPRMOF)
*
LONGNF   DS    0F           Input pairs for long BFP non-finite testing
         DC    A(LBFPNFCT)
         DC    A(LBFPNFIN)
         DC    A(LBFPNFOT)
         DC    A(LBFPNFFL)
*
LONGF    DS    0F           Input pairs for long BFP finite testing
         DC    A(LBFPCT)
         DC    A(LBFPIN)
         DC    A(LBFPOUT)
         DC    A(LBFPFLGS)
*
RMLONGS  DS    0F           Input pairs for long BFP rounding testing
         DC    A(LBFPRMCT)
         DC    A(LBFPINRM)
         DC    A(LBFPRMO)
         DC    A(LBFPRMOF)
*
XTNDNF   DS    0F           Inputs for ext'd BFP non-finite testing
         DC    A(XBFPNFCT)
         DC    A(XBFPNFIN)
         DC    A(XBFPNFOT)
         DC    A(XBFPNFFL)
*
XTNDF    DS    0F           Inputs for ext'd BFP finite testing
         DC    A(XBFPCT)
         DC    A(XBFPIN)
         DC    A(XBFPOUT)
         DC    A(XBFPFLGS)
*
RMXTNDS  DS    0F           Inputs for ext'd BFP non-finite testing
         DC    A(XBFPRMCT)
         DC    A(XBFPINRM)
         DC    A(XBFPRMO)
         DC    A(XBFPRMOF)
*
         EJECT
***********************************************************************
*
* Perform Add using provided short BFP inputs.  This set of tests
* checks NaN propagation, operations on values that are not finite
* numbers, and other basic tests.  This set generates results that can
* be validated against Figure 19-13 on page 19-16 of SA22-7832-10.
*
* That Figure has separate rows and colums for Normal and Tiny
* operands.  Although the results are effectively the same for Normal
* and Tiny in any combination, the input data includes Normal and
* Tiny values.
*
* Four results are generated for each input: one RRE with all
* exceptions non-trappable, a second RRE with all exceptions trappable,
* a third RXE with all exceptions non-trappable, a fourth RXE with all
* exceptions trappable,
*
* The sum, FPCR, and condition code are stored for each result.
*
***********************************************************************
         SPACE 2
SBFPNF   DS    0H            BFP Short non-finite values tests
         LM    R2,R3,0(R10)  Get count and address of add values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LM    R4,R5,0(R10)  Get count and start of addend values
*                            ..which are the same as the augends
         BASR  R6,0          Set top of inner loop
*
         LE    FPR8,0(,R3)   Get short BFP augend
         LE    FPR1,0(,R5)   Get short BFP addend
         LFPC  FPCREGNT      Set exceptions non-trappable
         AEBR  FPR8,FPR1     Add FPR0/FPR1 RRE
         STE   FPR8,0(,R7)   Store short BFP sum
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R8)     Save condition code in results table
*
         LE    FPR8,0(,R3)   Get short BFP augend
         LE    FPR1,0(,R5)   Get short BFP addend
         LFPC  FPCREGTR      Set exceptions trappable
         AEBR  FPR8,FPR1     Add FPR0/FPR1 RRE
         STE   FPR8,4(,R7)   Store short BFP sum
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R8)     Save condition code in results table
*
         LE    FPR8,0(,R3)   Get short BFP augend
         LE    FPR1,0(,R5)   Get short BFP addend
         LFPC  FPCREGNT      Set exceptions non-trappable
         AEB   FPR8,0(,R5)   Add FPR0/FPR1 RXE
         STE   FPR8,8(,R7)   Store short BFP sum
         STFPC 8(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,11(,R8)    Save condition code in results table
*
         LE    FPR8,0(,R3)   Get short BFP augend
         LFPC  FPCREGTR      Set exceptions trappable
         AEB   FPR8,0(,R5)   Add FPR0/FPR1 RXE
         STE   FPR8,12(,R7)  Store short BFP sum
         STFPC 12(R8)        Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,15(,R8)    Save condition code in results table
*
         LA    R5,4(,R5)     Point to next addend value
         LA    R7,4*4(,R7)   Point to next Add result area
         LA    R8,4*4(,R8)   Point to next Add FPCR area
         BCTR  R4,R6         Loop through right-hand values
*
         LA    R3,4(,R3)     Point to next input augend
         BCTR  R2,R12        Loop through left-hand values
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Add using provided short BFP input pairs.  This set of
* tests triggers IEEE exceptions Overflow, Underflow, and Inexact and
* collects both trap and non-trap results.
*
* Four results are generated for each input: one RRE with all
* exceptions non-trappable, a second RRE with all exceptions trappable,
* a third RXE with all exceptions non-trappable, a fourth RXE with all
* exceptions trappable,
*
* The sum, FPCR, and condition code are stored for each result.
*
***********************************************************************
         SPACE 2
SBFPF    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LE    FPR8,0(,R3)   Get short BFP augend
         LE    FPR1,4(,R3)   Get short BFP addend
         AEBR  FPR8,FPR1     Add FPR8/FPR1 RRE non-trappable
         STE   FPR8,0(,R7)   Store short BFP sum
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R8)     Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         LE    FPR8,0(,R3)   Reload short BFP augend
*                            ..addend is still in FPR1
         AEBR  FPR8,FPR1     Add FPR8/FPR1 RRE trappable
         STE   FPR8,4(,R7)   Store short BFP sum
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R8)     Save condition code in results table
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LE    FPR8,0(,R3)   Reload short BFP augend
         AEB   FPR8,4(,R3)   Add FPR8 by addend RXE non-trappable
         STE   FPR8,8(,R7)   Store short BFP sum
         STFPC 8(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,11(,R8)    Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         LE    FPR8,0(,R3)   Reload short BFP augend
         AEB   FPR8,4(,R3)   Add FPR8 by addend RXE trappable
         STE   FPR8,12(,R7)  Store short BFP sum
         STFPC 12(R8)        Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,15(,R8)    Save condition code in results table
*
         LA    R3,2*4(,R3)   Point to next input value pair
         LA    R7,4*4(,R7)   Point to next sum result set
         LA    R8,4*4(,R8)   Point to next FPCR result set
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Add using provided short BFP input pairs.  This set of
* tests exhaustively tests all rounding modes available for Add.
* The rounding mode can only be specified in the FPC.
*
* All five FPC rounding modes are tested because the preceeding tests,
* using rounding mode RNTE, do not often create results that require
* rounding.
*
* Two results are generated for each input and rounding mode: one RRE
* and one RXE.  Traps are disabled for all rounding mode tests.
*
* The sum, FPCR, and condition code are stored for each test.
*
***********************************************************************
         SPACE 2
SBFPRM   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         XR    R1,R1         Zero register 1 for use in IC/STC/indexing
         BASR  R12,0         Set top of test case loop

         LA    R5,FPCMCT     Get count of FPC modes to be tested
         BASR  R9,0          Set top of rounding mode outer loop
*
         IC    R1,FPCMODES-L'FPCMODES(R5)  Get next FPC mode
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 0(R1)         Set FPC Rounding Mode
         LE    FPR8,0(,R3)   Get short BFP augend
         LE    FPR1,4(,R3)   Get short BFP addend
         AEBR  FPR8,FPR1     Add RRE FPR8/FPR1 non-trappable
         STE   FPR8,0(,R7)   Store short BFP sum
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R8)     Save condition code in results table
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 0(R1)         Set FPC Rounding Mode
         LE    FPR8,0(,R3)   Get short BFP augend
         AEB   FPR8,4(,R3)   Add RXE FPR8 by addend non-trappable
         STE   FPR8,4(,R7)   Store short BFP sum
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R8)     Save condition code in results table
*
         LA    R7,2*4(,R7)   Point to next sum result set
         LA    R8,2*4(,R8)   Point to next FPCR result area
*
         BCTR  R5,R9         Iterate to next FPC mode for this input
*
* End of FPC modes to be tested.  Advance to next test case.  We will
* skip eight bytes of result area so that each set of five result
* value pairs starts at a memory address ending in zero for the
* convenience of memory dump review.
*
         LA    R3,2*4(,R3)   Point to next input value pair
         LA    R7,8(,R7)     Skip to start of next result set
         LA    R8,8(,R8)     Skip to start of next FPCR result set
         BCTR  R2,R12        Advance to the next input pair
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Add using provided long BFP inputs.  This set of tests
* checks NaN propagation, operations on values that are not finite
* numbers, and other basic tests.  This set generates results that can
* be validated against Figure 19-13 on page 19-16 of SA22-7832-10.
*
* That Figure has separate rows and colums for Normal and Tiny
* operands.  Although the results are effectively the same for Normal
* and Tiny in any combination, the input data includes Normal and
* Tiny values.
*
* Four results are generated for each input: one RRE with all
* exceptions non-trappable, a second RRE with all exceptions trappable,
* a third RXE with all exceptions non-trappable, a fourth RXE with all
* exceptions trappable,
*
* The sum, FPCR, and condition code are stored for each result.
*
***********************************************************************
         SPACE 2
LBFPNF   DS    0H            BFP long non-finite values tests
         LM    R2,R3,0(R10)  Get count and address of augend values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LM    R4,R5,0(R10)  Get count and start of addend values
*                            ..which are the same as the augends
         BASR  R6,0          Set top of inner loop
*
         LD    FPR8,0(,R3)   Get long BFP augend
         LD    FPR1,0(,R5)   Get long BFP addend
         LFPC  FPCREGNT      Set exceptions non-trappable
         ADBR  FPR8,FPR1     Add FPR0/FPR1 RRE
         STD   FPR8,0(,R7)   Store long BFP sum
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R8)     Save condition code in results table
*
         LD    FPR8,0(,R3)   Get long BFP augend
         LD    FPR1,0(,R5)   Get long BFP addend
         LFPC  FPCREGTR      Set exceptions trappable
         ADBR  FPR8,FPR1     Add FPR0/FPR1 RRE
         STD   FPR8,8(,R7)   Store long BFP remainder
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R8)     Save condition code in results table
*
         LD    FPR8,0(,R3)   Get long BFP augend
         LFPC  FPCREGNT      Set exceptions non-trappable
         ADB   FPR8,0(,R5)   Add FPR0/FPR1 RXE
         STD   FPR8,16(,R7)  Store long BFP sum
         STFPC 8(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,11(,R8)    Save condition code in results table
*
         LD    FPR8,0(,R3)   Get long BFP augend
         LFPC  FPCREGTR      Set exceptions trappable
         ADB   FPR8,0(,R5)   Add FPR0/FPR1 RXE
         STD   FPR8,24(,R7)  Store long BFP remainder
         STFPC 12(R8)        Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,15(,R8)    Save condition code in results table
*
         LA    R5,8(,R5)     Point to next addend value
         LA    R7,4*8(,R7)   Point to next Add result area
         LA    R8,4*4(,R8)   Point to next Add FPCR area
         BCTR  R4,R6         Loop through right-hand values
*
         LA    R3,8(,R3)     Point to next augend value
         BCTR  R2,R12        Add until all cases tested
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Add using provided long BFP input pairs.  This set of
* tests triggers IEEE exceptions Overflow, Underflow, and Inexact and
* collects non-trap and trap results.
*
* Four results are generated for each input: one RRE with all
* exceptions non-trappable, a second RRE with all exceptions trappable,
* a third RXE with all exceptions non-trappable, a fourth RXE with all
* exceptions trappable,
*
* The sum, FPCR, and condition code are stored for each result.
*
***********************************************************************
         SPACE 2
LBFPF    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LD    FPR8,0(,R3)   Get short BFP augend
         LD    FPR1,8(,R3)   Get short BFP addend
         ADBR  FPR8,FPR1     Add FPR8/FPR1 RRE non-trappable
         STD   FPR8,0(,R7)   Store short BFP sum
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R8)     Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         LD    FPR8,0(,R3)   Reload short BFP augend
*                            ..addend is still in FPR1
         ADBR  FPR8,FPR1     Add FPR8/FPR1 RRE trappable
         STD   FPR8,8(,R7)   Store short BFP sum
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R8)     Save condition code in results table
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LD    FPR8,0(,R3)   Reload short BFP augend
         ADB   FPR8,8(,R3)   Add FPR8/FPR1 RXE non-trappable
         STD   FPR8,16(,R7)  Store short BFP sum
         STFPC 8(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,11(,R8)    Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         LD    FPR8,0(,R3)   Reload short BFP augend
         ADB   FPR8,8(,R3)   Add FPR8/FPR1 RXE trappable
         STD   FPR8,24(,R7)  Store short BFP sum
         STFPC 12(R8)        Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,15(,R8)    Save condition code in results table
*
         LA    R3,2*8(,R3)   Point to next input value pair
         LA    R7,4*8(,R7)   Point to next quotent result pair
         LA    R8,4*4(,R8)   Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Add using provided long BFP input pairs.  This set of
* tests exhaustively tests all rounding modes available for Add.
* The rounding mode can only be specified in the FPC.
*
* All five FPC rounding modes are tested because the preceeding tests,
* using rounding mode RNTE, do not often create results that require
* rounding.
*
* Two results are generated for each input and rounding mode: one RRE
* and one RXE.  Traps are disabled for all rounding mode tests.
*
* The sum, FPCR, and condition code are stored for each result.
*
***********************************************************************
         SPACE 2
LBFPRM   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         XR    R1,R1         Zero register 1 for use in IC/STC/indexing
         BASR  R12,0         Set top of test case loop

         LA    R5,FPCMCT     Get count of FPC modes to be tested
         BASR  R9,0          Set top of rounding mode loop
*
         IC    R1,FPCMODES-L'FPCMODES(R5)  Get next FPC mode
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 0(R1)         Set FPC Rounding Mode
         LD    FPR8,0(,R3)   Get long BFP augend
         LD    FPR1,8(,R3)   Get long BFP addend
         ADBR  FPR8,FPR1     Add RRE FPR8/FPR1 non-trappable
         STD   FPR8,0(,R7)   Store long BFP sum
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R8)     Save condition code in results table
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 0(R1)         Set FPC Rounding Mode
         LD    FPR8,0(,R3)   Reload long BFP augend
         ADB   FPR8,8(,R3)   Add RXE FPR8 by addend non-trappable
         STD   FPR8,8(,R7)   Store long BFP sum
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R8)     Save condition code in results table
*
         LA    R7,2*8(,R7)   Point to next sum result set
         LA    R8,2*4(,R8)   Point to next FPCR result area
*
         BCTR  R5,R9         Iterate to next FPC mode
*
* End of FPC modes to be tested.  Advance to next test case.  We will
* skip eight bytes of FPCR result area so that each set of five result
* FPCR contents pairs starts at a memory address ending in zero for the
* convenience of memory dump review.
*
         LA    R3,2*8(,R3)   Point to next input value pair
         LA    R8,8(,R8)     Skip to start of next FPCR result area
         BCTR  R2,R12        Add next input value lots of times
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Add using provided extended BFP inputs.  This set of tests
* checks NaN propagation, operations on values that are not finite
* numbers, and other basic tests.  This set generates results that can
* be validated against Figure 19-13 on page 19-16 of SA22-7832-10.
*
* That Figure has separate rows and colums for Normal and Tiny
* operands.  Although the results are effectively the same for Normal
* and Tiny in any combination, the input data includes Normal and
* Tiny values.
*
* Two results are generated for each input: one RRE with all
* exceptions non-trappable, and a second RRE with all exceptions
* trappable.  Extended BFP Add does not have an RXE format.
*
* The sum, FPCR, and condition code are stored for each result.
*
***********************************************************************
         SPACE 2
XBFPNF   DS    0H            BFP extended non-finite values tests
         LM    R2,R3,0(R10)  Get count and address of augend values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LM    R4,R5,0(R10)  Get count and start of addend values
*                            ..which are the same as the augends
         BASR  R6,0          Set top of inner loop
*
         LD    FPR8,0(,R3)   Get extended BFP augend part 1
         LD    FPR10,8(,R3)  Get extended BFP augend part 2
         LD    FPR1,0(,R5)   Get extended BFP addend part 1
         LD    FPR3,8(,R5)   Get extended BFP addend part 2
         LFPC  FPCREGNT      Set exceptions non-trappable
         AXBR  FPR8,FPR1     Add FPR0/FPR1 RRE
         STD   FPR8,0(,R7)   Store extended BFP sum part 1
         STD   FPR10,8(,R7)  Store extended BFP sum part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R8)     Save condition code in results table
*
         LD    FPR8,0(,R3)   Get extended BFP augend part 1
         LD    FPR10,8(,R3)  Get extended BFP augend part 2
         LD    FPR1,0(,R5)   Get extended BFP addend part 1
         LD    FPR3,8(,R5)   Get extended BFP addend part 2
         LFPC  FPCREGTR      Set exceptions trappable
         AXBR  FPR8,FPR1     Add FPR0/FPR1 RRE
         STD   FPR8,16(,R7)  Store extended BFP sum part 1
         STD   FPR10,24(,R7) Store extended BFP sum part 2
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R8)     Save condition code in results table
*
         LA    R5,16(,R5)    Point to next addend value
         LA    R7,32(,R7)    Point to next Add result area
         LA    R8,16(,R8)    Point to next Add FPCR area
         BCTR  R4,R6         Loop through right-hand values
*
         LA    R3,16(,R3)    Point to next augend value
         BCTR  R2,R12        Add until all cases tested
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Add using provided extended BFP input pairs.  This set of
* tests triggers IEEE exceptions Overflow, Underflow, and Inexact and
* collects results when the exceptions do not result in a trap and when
* they do.
*
* Two results are generated for each input: one RRE with all
* exceptions non-trappable and a second RRE with all exceptions
* trappable.  There is no RXE format for Add in extended precision.
*
* The sum, FPCR, and condition code are stored for each result.
*
***********************************************************************
         SPACE 2
XBFPF    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LD    FPR8,0(,R3)   Get extended BFP augend part 1
         LD    FPR10,8(,R3)  Get extended BFP augend part 2
         LD    FPR1,16(,R3)  Get extended BFP addend part 1
         LD    FPR3,24(,R3)  Get extended BFP addend part 2
         AXBR  FPR8,FPR1     Add FPR8-10/FPR1-3 RRE non-trappable
         STD   FPR8,0(,R7)   Store extended BFP sum part 1
         STD   FPR10,8(,R7)  Store extended BFP sum part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R8)     Save condition code in results table
*
         LFPC  FPCREGTR      Set exceptions trappable
         LD    FPR8,0(,R3)   Reload extended BFP augend part 1
         LD    FPR10,8(,R3)  Reload extended BFP augend part 2
*                            ..addend is still in FPR1-FPR3
         AXBR  FPR8,FPR1     Add FPR8-10/FPR1-3 RRE trappable
         STD   FPR8,16(,R7)  Store extended BFP sum part 1
         STD   FPR10,24(,R7) Store extended BFP sum part 2
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,7(,R8)     Save condition code in results table
*
         LA    R3,32(,R3)    Point to next input value pair
         LA    R7,32(,R7)    Point to next quotent result pair
         LA    R8,16(,R8)    Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Add using provided extended BFP input pairs.  This set of
* tests exhaustively tests all rounding modes available for Add.
* The rounding mode can only be specified in the FPC.
*
* All five FPC rounding modes are tested because the preceeding tests,
* using rounding mode RNTE, do not often create results that require
* rounding.
*
* Two results are generated for each input and rounding mode: one RRE
* and one RXE.  Traps are disabled for all rounding mode tests.
*
* The sum, FPCR, and condition code are stored for each result.
*
***********************************************************************
         SPACE 2
XBFPRM   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         XR    R1,R1         Zero register 1 for use in IC/STC/indexing
         BASR  R12,0         Set top of test case loop

         LA    R5,FPCMCT     Get count of FPC modes to be tested
         BASR  R9,0          Set top of rounding mode loop
*
         IC    R1,FPCMODES-L'FPCMODES(R5)  Get next FPC mode
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 0(R1)         Set FPC Rounding Mode
         LD    FPR8,0(,R3)   Get extended BFP augend part 1
         LD    FPR10,8(,R3)  Get extended BFP augend part 2
         LD    FPR1,16(,R3)  Get extended BFP addend part 1
         LD    FPR3,24(,R3)  Get extended BFP addend part 2
         AXBR  FPR8,FPR1     Add RRE FPR8/FPR1 non-trappable
         STD   FPR8,0(,R7)   Store extended BFP sum part 1
         STD   FPR10,8(,R7)  Store extended BFP sum part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         STC   R0,3(,R8)     Save condition code in results table
*
         LA    R7,16(,R7)    Point to next sum result set
         LA    R8,4(,R8)     Point to next FPCR result area
*
         BCTR  R5,R9         Iterate to next FPC mode
*
* End of FPC modes to be tested.  Advance to next test case.  We will
* skip eight bytes of FPCR result area so that each set of five result
* FPCR contents pairs starts at a memory address ending in zero for the
* convenience of memory dump review.
*
         LA    R3,2*16(,R3)  Point to next input value pair
         LA    R8,12(,R8)    Skip to start of next FPCR result area
         BCTR  R2,R12        Add next input value lots of times
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Table of FPC rounding modes to test sum rounding modes.
*
* The Set BFP Rounding Mode does allow specification of the FPC
* rounding mode as an address, so we shall index into a table of
* BFP rounding modes without bothering with Execute.
*
***********************************************************************
         SPACE 2
*
* Rounding modes that may be set in the FPCR.  The FPCR controls
* rounding of the sum.
*
* These are indexed directly by the loop counter, which counts down.
* So the modes are listed in reverse order here.
*
FPCMODES DS    0C
         DC    AL1(7)              RFS, Round for shorter precision
         DC    AL1(3)              RM, Round to -infinity
         DC    AL1(2)              RP, Round to +infinity
         DC    AL1(1)              RZ, Round to zero
         DC    AL1(0)              RNTE, Round to Nearest, ties to even
FPCMCT   EQU   *-FPCMODES          Count of FPC Modes to be tested
*
         EJECT
***********************************************************************
*
* Short BFP test data sets for Add testing.
*
* The first test data set is used for tests of basic functionality,
* NaN propagation, and results from operations involving other than
* finite numbers.
*
* The second test data set is used for testing boundary conditions
* using two finite non-zero values.  Each possible condition code
* and type of result (normal, scaled, etc) is created by members of
* this test data set.
*
* The third test data set is used for exhaustive testing of final
* results across the five rounding modes available for the Add
* instruction.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
* First input test data set, to test operations using non-finite or
* zero inputs.  Member values chosen to validate Figure 19-13 on page
* 19-16 of SA22-7832-10.  Each value in this table is tested against
* every other value in the table.  Ten entries means 100 result sets.
*
***********************************************************************
         SPACE 2
SBFPNFIN DS    0F                Inputs for short BFP non-finite tests
         DC    X'FF800000'         -inf
         DC    X'C0000000'         -2.0
         DC    X'80010000'         -Dnice
         DC    X'80000000'         -0
         DC    X'00000000'         +0
         DC    X'00010000'         -Dnice
         DC    X'40000000'         +2.0
         DC    X'7F800000'         +inf
         DC    X'FFCB0000'         -QNaN
         DC    X'7F8A0000'         +SNaN
SBFPNFCT EQU   (*-SBFPNFIN)/4    Count of short BFP in list
         SPACE 3
***********************************************************************
*
* Second input test data set.  These are finite pairs intended to
* trigger overflow, underflow, and inexact exceptions.  Each pair is
* added twice, once non-trappable and once trappable.  Trappable
* overflow or underflow yields a scaled result.  Trappable inexact
* will show whether the Incremented DXC code is returned.
*
* The following test cases are required:
* 1. Overflow
* 2. Underflow - normal inputs
* 3. Underflow - subnormal inputs
* 4. Normal - from subnormal inputs
* 5. Inexact - incremented
* 6. Inexact - truncated
*
***********************************************************************
         SPACE 2
SBFPIN   DS    0F                Inputs for short BFP finite tests
*
* Overflow on addition
*
         DC    X'7F7FFFFF'         +Nmax
         DC    X'7F7FFFFF'         +Nmax
*
* Underflow from sum of normals.  We will add a small normal to a
* negative smaller normal to generate a subnormal.
*
         DC    X'00FFFFFF'         Very small normal number
         DC    X'80800000'         Smaller normal negative
*
* Underflow from sum of subnormals.  We will add two subnormals.
*
         DC    X'00040000'         Subnormal, < +Dmax
         DC    X'00000F0F'         Smaller subnormal
*
* Normal result from sum of subnormals.  We will add two subnormals.
* The result will be greater than +Nmin
*
         DC    X'007FFFFF'         +Dmax
         DC    X'00000001'         +Dmin, result will be +Nmin
*
* Add a value to 1.0 such that the added digits are to the right of
* the right-most bit in the stored significand. The result will be
* inexact, and incremented will be determined by the value of the
* bits in the addend.
*
         DC    X'3F800000'         Augend +1, aka 1.0b0
         DC    X'33F80000'         Addend 1.1111b-24
*                       ..Above addend is 1.154839992523193359375E-7
*                       ..nearest is away from zero, incremented.
*
         DC    X'3F800000'         Augend +1, aka 1.0b0
         DC    X'33780000'         Addend 1.1111b-25
*                       ..Above addend is 5.774199962615966796875E-8
*                       ..nearest is toward zero, truncated
*
SBFPCT   EQU   (*-SBFPIN)/4/2    Count of short BFP in list
         SPACE 3
***********************************************************************
*
* Third input test data set.  These are finite pairs intended to
* test all combinations of rounding mode for the sum and the
* remainder.  Values are chosen to create a requirement to round
* to the target precision after the computation and to generate
* varying results depending on the rounding mode in the FPCR.
*
* The result set will have cases that represent each of the following
*
* 1. Positive, nearest magnitude is toward zero.
* 2. Negative, nearest magnitude is toward zero.
* 3. Positive, nearest magnitude is away from zero.
* 4. Negative, nearest magnitude is away from zero.
* 5. Positive, tie, nearest even has greater magnitude
* 6. Negative, tie, nearest even has greater magnitude
* 7. Positive, tie, nearest even has lower magnitude
* 8. Negative, tie, nearest even has lower magnitude
*
* Round For Shorter precision correctness can be determined from the
* above test cases.
*
***********************************************************************
         SPACE 2
SBFPINRM DS    0F                Inputs for short BFP rounding testing
*
* Add a value to 1.0 such that the added digits are to the right of
* the right-most bit in the stored significand. The result will be
* inexact, and incremented will be determined by the value of the
* bits in the addend.
*
         DC    X'3F800000'         Augend +1, aka +1.0b0
         DC    X'337E0000'         Addend +1.111111b-25
         DC    X'BF800000'         Augend -1, aka -1.0b0
         DC    X'B37E0000'         Addend -1.111111b-25
*                       ..Above addend is 5.91389834880828857421875E-8
*                       ..nearest is toward zero, truncated
*
         DC    X'3F800000'         Augend +1, aka 1.0b0
         DC    X'33FF0000'         Addend +1.1111111b-24
         DC    X'BF800000'         Augend -1, aka -1.0b0
         DC    X'B3FF0000'         Addend -1.1111111b-24
*                       ..Above addend is 1.187436282634735107421875E-7
*                       ..nearest is away from zero, incremented.
*
         DC    X'3F800000'         Augend +1, aka +1.0b0
         DC    X'33800000'         Addend +1.0b-24
         DC    X'BF800000'         Augend -1, aka -1.0b0
         DC    X'B3800000'         Addend -1.0b-24
*                 ..Above addend is 5.9604644775390625E-8
*                 ..nearest is a tie, nearest even has lower magnitude
*
         DC    X'3F800000'         Augend +1, aka +1.0b0
         DC    X'34400000'         Addend +1.1b-23
         DC    X'BF800000'         Augend -1, aka -1.0b0
         DC    X'B4400000'         Addend -1.1b-23
*               ..Above addend is 1.78813934326171875E-7
*               ..nearest is a tie, nearest even has greater magnitude
SBFPRMCT EQU   (*-SBFPINRM)/4/2  Count of short BFP rounding tests
         EJECT
***********************************************************************
*
* Long BFP test data sets for Add testing.
*
* The first test data set is used for tests of basic functionality,
* NaN propagation, and results from operations involving other than
* finite numbers.
*
* The second test data set is used for testing boundary conditions
* using two finite non-zero values.  Each possible condition code
* and type of result (normal, scaled, etc) is created by members of
* this test data set.
*
* The third test data set is used for exhaustive testing of final
* results across the five rounding modes available for the Add
* instruction.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
* First input test data set, to test operations using non-finite or
* zero inputs.  Member values chosen to validate Figure 19-13 on page
* 19-16 of SA22-7832-10.  Each value in this table is tested against
* every other value in the table.  Ten entries means 100 result sets.
*
***********************************************************************
         SPACE 2
LBFPNFIN DS    0F                Inputs for long BFP testing
         DC    X'FFF0000000000000'         -inf
         DC    X'C000000000000000'         -2.0
         DC    X'8001000000000000'         -Dnice
         DC    X'8000000000000000'         -0
         DC    X'0000000000000000'         +0
         DC    X'0001000000000000'         +Dnice
         DC    X'4000000000000000'         +2.0
         DC    X'7FF0000000000000'         +inf
         DC    X'FFF8B00000000000'         -QNaN
         DC    X'7FF0A00000000000'         +SNaN
LBFPNFCT EQU   (*-LBFPNFIN)/8     Count of long BFP in list
         SPACE 3
***********************************************************************
*
* Second input test data set.  These are finite pairs intended to
* trigger overflow, underflow, and inexact exceptions.  Each pair is
* added twice, once non-trappable and once trappable.  Trappable
* overflow or underflow yields a scaled result.  Trappable inexact
* will show whether the Incremented DXC code is returned.
*
* The following test cases are required:
* 1. Overflow
* 2. Underflow - normal inputs
* 3. Underflow - subnormal inputs
* 4. Normal - from subnormal inputs
* 5. Inexact - incremented
* 6. Inexact - truncated
*
***********************************************************************
         SPACE 2
LBFPIN   DS    0D                Inputs for long BFP finite tests
*
* Overflow on addition
*
         DC    X'7FFFFFFFFFFFFFFF'  +Nmax
         DC    X'7FFFFFFFFFFFFFFF'  +Nmax
*
* Underflow from sum of normals.  We will add a small normal to a
* negative smaller normal to generate a subnormal.
*
         DC    X'001FFFFFFFFFFFFF'  Very small normal number
         DC    X'8010000000000000'  Smaller normal negative
*
* Underflow from sum of subnormals.  We will add two subnormals.
*
         DC    X'0008000000000000'  Subnormal, < +Dmax
         DC    X'0000F0F000000000'  Smaller subnormal
*
* Normal result from sum of subnormals.  We will add two subnormals.
* The result will be greater than +Nmin
*
         DC    X'000FFFFFFFFFFFFF'  +Dmax
         DC    X'0000000000000001'  +Dmin, result will be +Nmin
*
* Add a value to 1.0 such that the added digits are to the right of
* the right-most bit in the stored significand. The result will be
* inexact, and incremented will be determined by the value of the
* bits in the addend.
*
         DC    X'3FF0000000000000'  Augend +1, aka 1.0b0
         DC    X'3CAF000000000000'  Addend 1.1111b-53
*    ..Above addend is 2.15105711021124079707078635692596435546875E-16
*    ..nearest is away from zero, incremented.
*
         DC    X'3FF0000000000000'  Augend +1, aka 1.0b0
         DC    X'3C9F000000000000'  Addend 1.1111b-54
*    ..Above addend is 1.075528555105620398535393178462982177734375E-16
*    ..nearest is toward zero, truncated.
*
LBFPCT   EQU   (*-LBFPIN)/8/2   Count of long BFP in list
         SPACE 3
***********************************************************************
*
* Third input test data set.  These are finite pairs intended to
* test all combinations of rounding mode for the sum and the
* remainder.  Values are chosen to create a requirement to round
* to the target precision after the computation and to generate
* varying results depending on the rounding mode in the FPCR.
*
* The result set will have cases that represent each of the following
*
* 1. Positive, nearest magnitude is toward zero.
* 2. Negative, nearest magnitude is toward zero.
* 3. Positive, nearest magnitude is away from zero.
* 4. Negative, nearest magnitude is away from zero.
* 5. Positive, tie, nearest even has greater magnitude
* 6. Negative, tie, nearest even has greater magnitude
* 7. Positive, tie, nearest even has lower magnitude
* 8. Negative, tie, nearest even has lower magnitude
*
* Round For Shorter precision correctness can be determined from the
* above test cases.
*
***********************************************************************
         SPACE 2
LBFPINRM DS    0F
*
* Add a value to 1.0 such that the added digits are to the right of
* the right-most bit in the stored significand. The result will be
* inexact, and incremented will be determined by the value of the
* bits in the addend.
*
         DC    X'3FF0000000000000'  Augend +1, aka +1.0b0
         DC    X'3C9FC00000000000'  Addend +1.111111b-54
         DC    X'BFF0000000000000'  Augend -1, aka -1.0b0
         DC    X'BC9FC00000000000'  Addend -1.111111b-54
* ..Above addend is 1.10154940724527250495157204568386077880859375E-16
* ..nearest is toward zero, truncated.
*
         DC    X'3FF0000000000000'  Augend +1, aka +1.0b0
         DC    X'3CAFE00000000000'  Addend +1.1111111b-53
         DC    X'BFF0000000000000'  Augend -1, aka -1.0b0
         DC    X'BCAFE00000000000'  Addend -1.1111111b-53
* ..Above addend is  2.21177243187042904537520371377468109130859375E-16
* ..nearest is away from zero, incremented.
*
         DC    X'3FF0000000000000'  Augend +1, aka +1.0b0
         DC    X'3CA0000000000000'  Addend +1.0b-53
         DC    X'BFF0000000000000'  Augend -1, aka -1.0b0
         DC    X'BCA0000000000000'  Addend -1.0b-53
*    ..Above addend is 1.1102230246251565404236316680908203125E-16
*    ..nearest is a tie, nearest even has lower magnitude
*
         DC    X'3FF0000000000000'  Augend +1, aka +1.0b0
         DC    X'3CB8000000000000'  Addend +1.1b-52
         DC    X'BFF0000000000000'  Augend -1, aka -1.0b0
         DC    X'BCB8000000000000'  Addend -1.1b-52
*    ..Above addend is 3.3306690738754696212708950042724609375E-16
*    ..nearest is a tie, nearest even has greater magnitude
*
*
LBFPRMCT EQU   (*-LBFPINRM)/8/2  Count of long BFP rounding tests
         EJECT
***********************************************************************
*
* Extended BFP test data sets for Add testing.
*
* The first test data set is used for tests of basic functionality,
* NaN propagation, and results from operations involving other than
* finite numbers.
*
* The second test data set is used for testing boundary conditions
* using two finite non-zero values.  Each possible condition code
* and type of result (normal, scaled, etc) is created by members of
* this test data set.
*
* The third test data set is used for exhaustive testing of final
* results across the five rounding modes available for the Add
* instruction.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
* First input test data set, to test operations using non-finite or
* zero inputs.  Member values chosen to validate Figure 19-13 on page
* 19-16 of SA22-7832-10.  Each value in this table is tested against
* every other value in the table.  Ten entries means 100 result sets.
*
***********************************************************************
         SPACE 2
XBFPNFIN DS    0F                Inputs for extended BFP testing
         DC    X'FFFF0000000000000000000000000000'   -inf
         DC    X'C0000000000000000000000000000000'   -2.0
         DC    X'80001000000000000000000000000000'   -Dnice
         DC    X'80000000000000000000000000000000'   -0
         DC    X'00000000000000000000000000000000'   +0
         DC    X'00001000000000000000000000000000'   +Dnice
         DC    X'40000000000000000000000000000000'   +2.0
         DC    X'7FFF0000000000000000000000000000'   +inf
         DC    X'FFFF8B00000000000000000000000000'   -QNaN
         DC    X'7FFF0A00000000000000000000000000'   +SNaN
XBFPNFCT EQU   (*-XBFPNFIN)/16     Count of extended BFP in list
         SPACE 3
***********************************************************************
*
* Second input test data set.  These are finite pairs intended to
* trigger overflow, underflow, and inexact exceptions.  Each pair is
* added twice, once non-trappable and once trappable.  Trappable
* overflow or underflow yields a scaled result.  Trappable inexact
* will show whether the Incremented DXC code is returned.
*
* The following test cases are required:
* The following test cases are required:
* 1. Overflow
* 2. Underflow - normal inputs
* 3. Underflow - subnormal inputs
* 4. Normal - from subnormal inputs
* 5. Inexact - incremented
* 6. Inexact - truncated
*
***********************************************************************
         SPACE 2
XBFPIN   DS    0F                Inputs for extended BFP finite tests
*
* Overflow on addition
*
         DC    X'7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'  +Nmax
         DC    X'7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'  +Nmax
*
* Underflow from sum of normals.  We will add a small normal to a
* negative smaller normal to generate a subnormal.
*
         DC    X'0001FFFFFFFFFFFFFFFFFFFFFFFFFFFF'  Very small normal
         DC    X'80010000000000000000000000000000'  Smaller normal
*
* Underflow from sum of subnormals.  We will add two subnormals.
*
         DC    X'00008000000000000000000000000000'  Subnormal, < +Dmax
         DC    X'00000F0F000000000000000000000000'  Smaller subnormal
*
* Normal result from sum of subnormals.  We will add two subnormals.
* The result will be greater than +Nmin
*
         DC    X'0000FFFFFFFFFFFFFFFFFFFFFFFFFFFF'  +Dmax
         DC    X'00000000000000000000000000000001'  +Dmin
*                                   ...result will be +Nmin
*
* Add a value to 1.0 such that the added digits are to the right of
* the right-most bit in the stored significand. The result will be
* inexact, and incremented will be determined by the value of the
* bits in the addend.
*
         DC    X'3FFF0000000000000000000000000000'  +1, aka 1.0b0
         DC    X'3F8EF000000000000000000000000000'  1.1111b-113
*    ..Above addend is 1.865744633625134732647978631879148339833785...
*                      ...97170865731413869070820510387420654296875E-34
*    ..nearest is away from zero, incremented.
*
         DC    X'3FFF0000000000000000000000000000'  +1, aka 1.0b0
         DC    X'3F8DF000000000000000000000000000'  1.1111b-114
*    ..Above addend is 9.328723168125673663239893159395741699168929...
*                      ...85854328657069345354102551937103271484375E-35
*    ..nearest is toward zero, truncated
*
XBFPCT   EQU   (*-XBFPIN)/16/2   Count of extended BFP in list
         SPACE 3
***********************************************************************
*
* Third input test data set.  These are finite pairs intended to
* test all combinations of rounding mode for the sum and the
* remainder.  Values are chosen to create a requirement to round
* to the target precision after the computation and to generate
* varying results depending on the rounding mode in the FPCR.
*
* The result set will have cases that represent each of the following
*
* 1. Positive, nearest magnitude is toward zero.
* 2. Negative, nearest magnitude is toward zero.
* 3. Positive, nearest magnitude is away from zero.
* 4. Negative, nearest magnitude is away from zero.
* 5. Positive, tie, nearest even has greater magnitude
* 6. Negative, tie, nearest even has greater magnitude
* 7. Positive, tie, nearest even has lower magnitude
* 8. Negative, tie, nearest even has lower magnitude
*
* Round For Shorter precision correctness can be determined from the
* above test cases.
*
***********************************************************************
         SPACE 2
XBFPINRM DS    0D
*
* Add a value to 1.0 such that the added digits are to the right of
* the right-most bit in the stored significand. The result will be
* inexact, and incremented will be determined by the value of the
* bits in the addend.
*
         DC    X'3FFF0000000000000000000000000000'  +1, aka +1.0b0
         DC    X'3F8DFC00000000000000000000000000'  +1.111111b-114
         DC    X'BFFF0000000000000000000000000000'  -1, aka -1.0b0
         DC    X'BF8DFC00000000000000000000000000'  -1.111111b-114
*    ..Above addend is 9.554418083483552864769890574542412869310113...
*                  ...6454435273748231338686309754848480224609375E-35
*    ..nearest is toward zero
*
         DC    X'3FFF0000000000000000000000000000'  +1, aka +1.0b0
         DC    X'3F8EFE00000000000000000000000000'  +1.1111111b-113
         DC    X'BFFF0000000000000000000000000000'  -1, aka -1.0b0
         DC    X'BF8EFE00000000000000000000000000'  -1.1111111b-113
*    ..Above addend is 1.918406780541973213004978028746704946200062...
*                  ...18865204683510228278464637696743011474609375E-34
*    ..nearest is away from zero
*
         DC    X'3FFF0000000000000000000000000000'  +1, aka +1.0b0
         DC    X'3F8E0000000000000000000000000000'  +1.0000b-113
         DC    X'BFFF0000000000000000000000000000'  -1, aka -1.0b0
         DC    X'BF8E0000000000000000000000000000'  -1.0000b-113
*    ..Above addend is 9.629649721936179265279889712924636592690508...
*                  ...241076940976199693977832794189453125E-35
*    ..nearest is a tie, nearest even has lower magnitude
*
         DC    X'3FFF0000000000000000000000000000'  +1, aka +1.0b0
         DC    X'3F8F8000000000000000000000000000'  +1.1b-112
         DC    X'BFFF0000000000000000000000000000'  -1, aka -1.0b0
         DC    X'BF8F8000000000000000000000000000'  -1.1b-112
*    ..Above addend is 9.629649721936179265279889712924636592690508...
*                  ...241076940976199693977832794189453125E-35
*    ..nearest is a tie, nearest even has greater magnitude
*
XBFPRMCT EQU   (*-XBFPINRM)/16/2  Count of long BFP rounding tests
         EJECT
***********************************************************************
*                 ACTUAL results saved here
***********************************************************************
*
*               Locations for ACTUAL results
*
*
SBFPNFOT EQU   STRTLABL+X'1000'    Integer short non-finite BFP results
*                                  ..room for 110 tests, 100 used
SBFPNFFL EQU   STRTLABL+X'1700'    FPCR flags and DXC from short BFP
*                                  ..room for 110 tests, 100 used
*
SBFPOUT  EQU   STRTLABL+X'1E00'    Integer short BFP finite results
*                                  ..room for 16 tests, 6 used
SBFPFLGS EQU   STRTLABL+X'1F00'    FPCR flags and DXC from short BFP
*                                  ..room for 16 tests, 6 used
*
SBFPRMO  EQU   STRTLABL+X'2000'    Short BFP rounding mode test results
*                                  ..Room for 16, 8 used.
SBFPRMOF EQU   STRTLABL+X'2300'    Short BFP rounding mode FPCR results
*                                  ..Room for 16, 8 used.
*                                  ..next location starts at X'2500'
*
LBFPNFOT EQU   STRTLABL+X'4000'    Integer long non-finite BFP results
*                                  ..room for 100 tests, 100 used
LBFPNFFL EQU   STRTLABL+X'4D00'    FPCR flags and DXC from long BFP
*                                  ..room for 100 tests, 100 used
*
LBFPOUT  EQU   STRTLABL+X'5400'    Integer long BFP finite results
*                                  ..room for 16 tests, 6 used
LBFPFLGS EQU   STRTLABL+X'5600'    FPCR flags and DXC from long BFP
*                                  ..room for 16 tests, 6 used
*
LBFPRMO  EQU   STRTLABL+X'5700'    Long BFP rounding mode test results
*                                  ..Room for 16, 8 used.
LBFPRMOF EQU   STRTLABL+X'5C00'    Long BFP rounding mode FPCR results
*                                  ..Room for 16, 8 used.
*                                  ..next location starts at X'5E00'
*
XBFPNFOT EQU   STRTLABL+X'8000'    Integer ext'd non-finite BFP results
*                                  ..room for 100 tests, 100 used
XBFPNFFL EQU   STRTLABL+X'8D00'    FPCR flags and DXC from ext'd BFP
*                                  ..room for 100 tests, 100 used
*
XBFPOUT  EQU   STRTLABL+X'9400'    Extended BFP finite results
*                                  ..room for 16 tests, 6 used
XBFPFLGS EQU   STRTLABL+X'9600'    FPCR flags and DXC from ext'd BFP
*                                  ..room for 16 tests, 6 used
*
XBFPRMO  EQU   STRTLABL+X'9700'    Ext'd BFP rounding mode test results
*                                  ..Room for 16, 8 used.
XBFPRMOF EQU   STRTLABL+X'9C00'    Ext'd BFP rounding mode FPCR results
*                                  ..Room for 16, 8 used.
*                                  ..next location starts at X'9E00'
*
         EJECT
***********************************************************************
*                    EXPECTED results
***********************************************************************
*
         ORG   STRTLABL+X'A000'   (past end of actual results)
*
SBFPNFOT_GOOD EQU *
 DC CL48'AEBR/AEB NF -inf/-inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'AEBR/AEB NF -inf/-2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'AEBR/AEB NF -inf/-Dnice'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'AEBR/AEB NF -inf/-0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'AEBR/AEB NF -inf/+0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'AEBR/AEB NF -inf/+Dnice'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'AEBR/AEB NF -inf/+2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'AEBR/AEB NF -inf/+inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'AEBR/AEB NF -inf/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF -inf/+SNaN'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'AEBR/AEB NF -2.0/-inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'AEBR/AEB NF -2.0/-2.0'
 DC XL16'C0800000C0800000C0800000C0800000'
 DC CL48'AEBR/AEB NF -2.0/-Dnice'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'AEBR/AEB NF -2.0/-0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'AEBR/AEB NF -2.0/+0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'AEBR/AEB NF -2.0/+Dnice'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'AEBR/AEB NF -2.0/+2.0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AEBR/AEB NF -2.0/+inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'AEBR/AEB NF -2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF -2.0/+SNaN'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'AEBR/AEB NF -Dnice/-inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'AEBR/AEB NF -Dnice/-2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'AEBR/AEB NF -Dnice/-Dnice'
 DC XL16'80020000DD80000080020000DD800000'
 DC CL48'AEBR/AEB NF -Dnice/-0'
 DC XL16'80010000DD00000080010000DD000000'
 DC CL48'AEBR/AEB NF -Dnice/+0'
 DC XL16'80010000DD00000080010000DD000000'
 DC CL48'AEBR/AEB NF -Dnice/+Dnice'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AEBR/AEB NF -Dnice/+2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'AEBR/AEB NF -Dnice/+inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'AEBR/AEB NF -Dnice/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF -Dnice/+SNaN'
 DC XL16'7FCA0000800100007FCA000080010000'
 DC CL48'AEBR/AEB NF -0/-inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'AEBR/AEB NF -0/-2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'AEBR/AEB NF -0/-Dnice'
 DC XL16'80010000DD00000080010000DD000000'
 DC CL48'AEBR/AEB NF -0/-0'
 DC XL16'80000000800000008000000080000000'
 DC CL48'AEBR/AEB NF -0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AEBR/AEB NF -0/+Dnice'
 DC XL16'000100005D000000000100005D000000'
 DC CL48'AEBR/AEB NF -0/+2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'AEBR/AEB NF -0/+inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'AEBR/AEB NF -0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF -0/+SNaN'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'AEBR/AEB NF +0/-inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'AEBR/AEB NF +0/-2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'AEBR/AEB NF +0/-Dnice'
 DC XL16'80010000DD00000080010000DD000000'
 DC CL48'AEBR/AEB NF +0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AEBR/AEB NF +0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AEBR/AEB NF +0/+Dnice'
 DC XL16'000100005D000000000100005D000000'
 DC CL48'AEBR/AEB NF +0/+2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'AEBR/AEB NF +0/+inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'AEBR/AEB NF +0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF +0/+SNaN'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'AEBR/AEB NF +Dnice/-inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'AEBR/AEB NF +Dnice/-2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'AEBR/AEB NF +Dnice/-Dnice'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AEBR/AEB NF +Dnice/-0'
 DC XL16'000100005D000000000100005D000000'
 DC CL48'AEBR/AEB NF +Dnice/+0'
 DC XL16'000100005D000000000100005D000000'
 DC CL48'AEBR/AEB NF +Dnice/+Dnice'
 DC XL16'000200005D800000000200005D800000'
 DC CL48'AEBR/AEB NF +Dnice/+2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'AEBR/AEB NF +Dnice/+inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'AEBR/AEB NF +Dnice/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF +Dnice/+SNaN'
 DC XL16'7FCA0000000100007FCA000000010000'
 DC CL48'AEBR/AEB NF +2.0/-inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'AEBR/AEB NF +2.0/-2.0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AEBR/AEB NF +2.0/-Dnice'
 DC XL16'40000000400000004000000040000000'
 DC CL48'AEBR/AEB NF +2.0/-0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'AEBR/AEB NF +2.0/+0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'AEBR/AEB NF +2.0/+Dnice'
 DC XL16'40000000400000004000000040000000'
 DC CL48'AEBR/AEB NF +2.0/+2.0'
 DC XL16'40800000408000004080000040800000'
 DC CL48'AEBR/AEB NF +2.0/+inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'AEBR/AEB NF +2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF +2.0/+SNaN'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'AEBR/AEB NF +inf/-inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'AEBR/AEB NF +inf/-2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'AEBR/AEB NF +inf/-Dnice'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'AEBR/AEB NF +inf/-0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'AEBR/AEB NF +inf/+0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'AEBR/AEB NF +inf/+Dnice'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'AEBR/AEB NF +inf/+2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'AEBR/AEB NF +inf/+inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'AEBR/AEB NF +inf/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF +inf/+SNaN'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'AEBR/AEB NF -QNaN/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF -QNaN/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF -QNaN/-Dnice'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF -QNaN/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF -QNaN/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF -QNaN/+Dnice'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF -QNaN/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF -QNaN/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF -QNaN/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'AEBR/AEB NF -QNaN/+SNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'AEBR/AEB NF +SNaN/-inf'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'AEBR/AEB NF +SNaN/-2.0'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'AEBR/AEB NF +SNaN/-Dnice'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'AEBR/AEB NF +SNaN/-0'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'AEBR/AEB NF +SNaN/+0'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'AEBR/AEB NF +SNaN/+Dnice'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'AEBR/AEB NF +SNaN/+2.0'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'AEBR/AEB NF +SNaN/+inf'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'AEBR/AEB NF +SNaN/-QNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'AEBR/AEB NF +SNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
SBFPNFOT_NUM EQU (*-SBFPNFOT_GOOD)/64
*
*
SBFPNFFL_GOOD EQU *
 DC CL48'AEBR/AEB NF -inf/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -inf/-2.0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -inf/-Dnice FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -inf/-0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -inf/+0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -inf/+Dnice FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -inf/+2.0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -inf/+inf FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF -inf/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF -inf/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF -2.0/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -2.0/-2.0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -2.0/-Dnice FPCR'
 DC XL16'00080001F800080100080001F8000801'
 DC CL48'AEBR/AEB NF -2.0/-0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -2.0/+0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -2.0/+Dnice FPCR'
 DC XL16'00080001F8000C0100080001F8000C01'
 DC CL48'AEBR/AEB NF -2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'AEBR/AEB NF -2.0/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF -2.0/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF -2.0/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF -Dnice/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -Dnice/-2.0 FPCR'
 DC XL16'00080001F800080100080001F8000801'
 DC CL48'AEBR/AEB NF -Dnice/-Dnice FPCR'
 DC XL16'00000001F800100100000001F8001001'
 DC CL48'AEBR/AEB NF -Dnice/-0 FPCR'
 DC XL16'00000001F800100100000001F8001001'
 DC CL48'AEBR/AEB NF -Dnice/+0 FPCR'
 DC XL16'00000001F800100100000001F8001001'
 DC CL48'AEBR/AEB NF -Dnice/+Dnice FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'AEBR/AEB NF -Dnice/+2.0 FPCR'
 DC XL16'00080002F8000C0200080002F8000C02'
 DC CL48'AEBR/AEB NF -Dnice/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF -Dnice/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF -Dnice/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF -0/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -0/-2.0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF -0/-Dnice FPCR'
 DC XL16'00000001F800100100000001F8001001'
 DC CL48'AEBR/AEB NF -0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'AEBR/AEB NF -0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'AEBR/AEB NF -0/+Dnice FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'AEBR/AEB NF -0/+2.0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF -0/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF -0/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF -0/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +0/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF +0/-2.0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF +0/-Dnice FPCR'
 DC XL16'00000001F800100100000001F8001001'
 DC CL48'AEBR/AEB NF +0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'AEBR/AEB NF +0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'AEBR/AEB NF +0/+Dnice FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'AEBR/AEB NF +0/+2.0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +0/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +0/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF +0/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +Dnice/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF +Dnice/-2.0 FPCR'
 DC XL16'00080001F8000C0100080001F8000C01'
 DC CL48'AEBR/AEB NF +Dnice/-Dnice FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'AEBR/AEB NF +Dnice/-0 FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'AEBR/AEB NF +Dnice/+0 FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'AEBR/AEB NF +Dnice/+Dnice FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'AEBR/AEB NF +Dnice/+2.0 FPCR'
 DC XL16'00080002F800080200080002F8000802'
 DC CL48'AEBR/AEB NF +Dnice/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +Dnice/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF +Dnice/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +2.0/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'AEBR/AEB NF +2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'AEBR/AEB NF +2.0/-Dnice FPCR'
 DC XL16'00080002F8000C0200080002F8000C02'
 DC CL48'AEBR/AEB NF +2.0/-0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +2.0/+0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +2.0/+Dnice FPCR'
 DC XL16'00080002F800080200080002F8000802'
 DC CL48'AEBR/AEB NF +2.0/+2.0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +2.0/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +2.0/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF +2.0/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +inf/-inf FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +inf/-2.0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +inf/-Dnice FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +inf/-0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +inf/+0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +inf/+Dnice FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +inf/+2.0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +inf/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB NF +inf/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF +inf/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF -QNaN/-inf FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF -QNaN/-2.0 FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF -QNaN/-Dnice FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF -QNaN/-0 FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF -QNaN/+0 FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF -QNaN/+Dnice FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF -QNaN/+2.0 FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF -QNaN/+inf FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF -QNaN/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'AEBR/AEB NF -QNaN/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +SNaN/-inf FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +SNaN/-2.0 FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +SNaN/-Dnice FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +SNaN/-0 FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +SNaN/+0 FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +SNaN/+Dnice FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +SNaN/+2.0 FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +SNaN/+inf FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +SNaN/-QNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'AEBR/AEB NF +SNaN/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
SBFPNFFL_NUM EQU (*-SBFPNFFL_GOOD)/64
*
*
SBFPOUT_GOOD EQU *
 DC CL48'AEBR/AEB F Ovfl'
 DC XL16'7F8000001FFFFFFF7F8000001FFFFFFF'
 DC CL48'AEBR/AEB F Ufl 1'
 DC XL16'007FFFFF607FFFFE007FFFFF607FFFFE'
 DC CL48'AEBR/AEB F Ufl 2'
 DC XL16'00040F0F5E01E1E000040F0F5E01E1E0'
 DC CL48'AEBR/AEB F Nmin'
 DC XL16'00800000008000000080000000800000'
 DC CL48'AEBR/AEB F Incr'
 DC XL16'3F8000013F8000013F8000013F800001'
 DC CL48'AEBR/AEB F Trun'
 DC XL16'3F8000003F8000003F8000003F800000'
SBFPOUT_NUM EQU (*-SBFPOUT_GOOD)/64
*
*
SBFPFLGS_GOOD EQU *
 DC CL48'AEBR/AEB F Ovfl FPCR'
 DC XL16'00280002F800200200280002F8002002'
 DC CL48'AEBR/AEB F Ufl 1 FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'AEBR/AEB F Ufl 2 FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'AEBR/AEB F Nmin FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'AEBR/AEB F Incr FPCR'
 DC XL16'00080002F8000C0200080002F8000C02'
 DC CL48'AEBR/AEB F Trun FPCR'
 DC XL16'00080002F800080200080002F8000802'
SBFPFLGS_NUM EQU (*-SBFPFLGS_GOOD)/64
*
*
SBFPRMO_GOOD EQU *
 DC CL48'AEBR/AEB RM +NZ RNTE, RZ'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'AEBR/AEB RM +NZ RP, RM'
 DC XL16'3F8000013F8000013F8000003F800000'
 DC CL48'AEBR/AEB RM +NZ RFS'
 DC XL16'3F8000013F8000010000000000000000'
 DC CL48'AEBR/AEB RM -NZ RNTE, RZ'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'AEBR/AEB RM -NZ RP, RM'
 DC XL16'BF800000BF800000BF800001BF800001'
 DC CL48'AEBR/AEB RM -NZ RFS'
 DC XL16'BF800001BF8000010000000000000000'
 DC CL48'AEBR/AEB RM +NA RNTE, RZ'
 DC XL16'3F8000013F8000013F8000003F800000'
 DC CL48'AEBR/AEB RM +NA RP, RM'
 DC XL16'3F8000013F8000013F8000003F800000'
 DC CL48'AEBR/AEB RM +NA RFS'
 DC XL16'3F8000013F8000010000000000000000'
 DC CL48'AEBR/AEB RM -NA RNTE, RZ'
 DC XL16'BF800001BF800001BF800000BF800000'
 DC CL48'AEBR/AEB RM -NA RP, RM'
 DC XL16'BF800000BF800000BF800001BF800001'
 DC CL48'AEBR/AEB RM -NA RFS'
 DC XL16'BF800001BF8000010000000000000000'
 DC CL48'AEBR/AEB RM +TZ RNTE, RZ'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'AEBR/AEB RM +TZ RP, RM'
 DC XL16'3F8000013F8000013F8000003F800000'
 DC CL48'AEBR/AEB RM +TZ RFS'
 DC XL16'3F8000013F8000010000000000000000'
 DC CL48'AEBR/AEB RM -TZ RNTE, RZ'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'AEBR/AEB RM -TZ RP, RM'
 DC XL16'BF800000BF800000BF800001BF800001'
 DC CL48'AEBR/AEB RM -TZ RFS'
 DC XL16'BF800001BF8000010000000000000000'
 DC CL48'AEBR/AEB RM +TA RNTE, RZ'
 DC XL16'3F8000023F8000023F8000013F800001'
 DC CL48'AEBR/AEB RM +TA RP, RM'
 DC XL16'3F8000023F8000023F8000013F800001'
 DC CL48'AEBR/AEB RM +TA RFS'
 DC XL16'3F8000013F8000010000000000000000'
 DC CL48'AEBR/AEB RM -TA RNTE, RZ'
 DC XL16'BF800002BF800002BF800001BF800001'
 DC CL48'AEBR/AEB RM -TA RP, RM'
 DC XL16'BF800001BF800001BF800002BF800002'
 DC CL48'AEBR/AEB RM -TA RFS'
 DC XL16'BF800001BF8000010000000000000000'
SBFPRMO_NUM EQU (*-SBFPRMO_GOOD)/64
*
*
SBFPRMOF_GOOD EQU *
 DC CL48'AEBR/AEB RM +NZ FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'AEBR/AEB RM +NZ FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'AEBR/AEB RM +NZ FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'AEBR/AEB RM -NZ FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'AEBR/AEB RM -NZ FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'AEBR/AEB RM -NZ FPCR'
 DC XL16'00080001000800010000000000000000'
 DC CL48'AEBR/AEB RM +NA FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'AEBR/AEB RM +NA FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'AEBR/AEB RM +NA FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'AEBR/AEB RM -NA FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'AEBR/AEB RM -NA FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'AEBR/AEB RM -NA FPCR'
 DC XL16'00080001000800010000000000000000'
 DC CL48'AEBR/AEB RM +TZ FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'AEBR/AEB RM +TZ FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'AEBR/AEB RM +TZ FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'AEBR/AEB RM -TZ FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'AEBR/AEB RM -TZ FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'AEBR/AEB RM -TZ FPCR'
 DC XL16'00080001000800010000000000000000'
 DC CL48'AEBR/AEB RM +TA FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'AEBR/AEB RM +TA FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'AEBR/AEB RM +TA FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'AEBR/AEB RM -TA FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'AEBR/AEB RM -TA FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'AEBR/AEB RM -TA FPCR'
 DC XL16'00080001000800010000000000000000'
SBFPRMOF_NUM EQU (*-SBFPRMOF_GOOD)/64
*
*
LBFPNFOT_GOOD EQU *
 DC CL48'ADBR NF -inf/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADB NF -inf/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADBR NF -inf/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADB NF -inf/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADBR NF -inf/-Dnice'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADB NF -inf/-Dnice'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADBR NF -inf/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADB NF -inf/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADBR NF -inf/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADB NF -inf/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADBR NF -inf/+Dnice'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADB NF -inf/+Dnice'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADBR NF -inf/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADB NF -inf/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADBR NF -inf/+inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'ADB NF -inf/+inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'ADBR NF -inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF -inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF -inf/+SNaN'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'ADB NF -inf/+SNaN'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'ADBR NF -2.0/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADB NF -2.0/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADBR NF -2.0/-2.0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'ADB NF -2.0/-2.0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'ADBR NF -2.0/-Dnice'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADB NF -2.0/-Dnice'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADBR NF -2.0/-0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADB NF -2.0/-0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADBR NF -2.0/+0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADB NF -2.0/+0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADBR NF -2.0/+Dnice'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADB NF -2.0/+Dnice'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADBR NF -2.0/+2.0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADB NF -2.0/+2.0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADBR NF -2.0/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADB NF -2.0/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADBR NF -2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF -2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF -2.0/+SNaN'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'ADB NF -2.0/+SNaN'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'ADBR NF -Dnice/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADB NF -Dnice/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADBR NF -Dnice/-2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADB NF -Dnice/-2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADBR NF -Dnice/-Dnice'
 DC XL16'8002000000000000DFE0000000000000'
 DC CL48'ADB NF -Dnice/-Dnice'
 DC XL16'8002000000000000DFE0000000000000'
 DC CL48'ADBR NF -Dnice/-0'
 DC XL16'8001000000000000DFD0000000000000'
 DC CL48'ADB NF -Dnice/-0'
 DC XL16'8001000000000000DFD0000000000000'
 DC CL48'ADBR NF -Dnice/+0'
 DC XL16'8001000000000000DFD0000000000000'
 DC CL48'ADB NF -Dnice/+0'
 DC XL16'8001000000000000DFD0000000000000'
 DC CL48'ADBR NF -Dnice/+Dnice'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADB NF -Dnice/+Dnice'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADBR NF -Dnice/+2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADB NF -Dnice/+2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADBR NF -Dnice/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADB NF -Dnice/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADBR NF -Dnice/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF -Dnice/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF -Dnice/+SNaN'
 DC XL16'7FF8A000000000008001000000000000'
 DC CL48'ADB NF -Dnice/+SNaN'
 DC XL16'7FF8A000000000008001000000000000'
 DC CL48'ADBR NF -0/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADB NF -0/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADBR NF -0/-2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADB NF -0/-2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADBR NF -0/-Dnice'
 DC XL16'8001000000000000DFD0000000000000'
 DC CL48'ADB NF -0/-Dnice'
 DC XL16'8001000000000000DFD0000000000000'
 DC CL48'ADBR NF -0/-0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'ADB NF -0/-0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'ADBR NF -0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADB NF -0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADBR NF -0/+Dnice'
 DC XL16'00010000000000005FD0000000000000'
 DC CL48'ADB NF -0/+Dnice'
 DC XL16'00010000000000005FD0000000000000'
 DC CL48'ADBR NF -0/+2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADB NF -0/+2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADBR NF -0/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADB NF -0/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADBR NF -0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF -0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF -0/+SNaN'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'ADB NF -0/+SNaN'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'ADBR NF +0/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADB NF +0/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADBR NF +0/-2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADB NF +0/-2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADBR NF +0/-Dnice'
 DC XL16'8001000000000000DFD0000000000000'
 DC CL48'ADB NF +0/-Dnice'
 DC XL16'8001000000000000DFD0000000000000'
 DC CL48'ADBR NF +0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADB NF +0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADBR NF +0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADB NF +0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADBR NF +0/+Dnice'
 DC XL16'00010000000000005FD0000000000000'
 DC CL48'ADB NF +0/+Dnice'
 DC XL16'00010000000000005FD0000000000000'
 DC CL48'ADBR NF +0/+2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADB NF +0/+2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADBR NF +0/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADB NF +0/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADBR NF +0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF +0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF +0/+SNaN'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'ADB NF +0/+SNaN'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'ADBR NF +Dnice/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADB NF +Dnice/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADBR NF +Dnice/-2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADB NF +Dnice/-2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'ADBR NF +Dnice/-Dnice'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADB NF +Dnice/-Dnice'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADBR NF +Dnice/-0'
 DC XL16'00010000000000005FD0000000000000'
 DC CL48'ADB NF +Dnice/-0'
 DC XL16'00010000000000005FD0000000000000'
 DC CL48'ADBR NF +Dnice/+0'
 DC XL16'00010000000000005FD0000000000000'
 DC CL48'ADB NF +Dnice/+0'
 DC XL16'00010000000000005FD0000000000000'
 DC CL48'ADBR NF +Dnice/+Dnice'
 DC XL16'00020000000000005FE0000000000000'
 DC CL48'ADB NF +Dnice/+Dnice'
 DC XL16'00020000000000005FE0000000000000'
 DC CL48'ADBR NF +Dnice/+2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADB NF +Dnice/+2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADBR NF +Dnice/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADB NF +Dnice/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADBR NF +Dnice/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF +Dnice/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF +Dnice/+SNaN'
 DC XL16'7FF8A000000000000001000000000000'
 DC CL48'ADB NF +Dnice/+SNaN'
 DC XL16'7FF8A000000000000001000000000000'
 DC CL48'ADBR NF +2.0/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADB NF +2.0/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'ADBR NF +2.0/-2.0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADB NF +2.0/-2.0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'ADBR NF +2.0/-Dnice'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADB NF +2.0/-Dnice'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADBR NF +2.0/-0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADB NF +2.0/-0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADBR NF +2.0/+0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADB NF +2.0/+0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADBR NF +2.0/+Dnice'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADB NF +2.0/+Dnice'
 DC XL16'40000000000000004000000000000000'
 DC CL48'ADBR NF +2.0/+2.0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'ADB NF +2.0/+2.0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'ADBR NF +2.0/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADB NF +2.0/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADBR NF +2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF +2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF +2.0/+SNaN'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'ADB NF +2.0/+SNaN'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'ADBR NF +inf/-inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'ADB NF +inf/-inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'ADBR NF +inf/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADB NF +inf/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADBR NF +inf/-Dnice'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADB NF +inf/-Dnice'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADBR NF +inf/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADB NF +inf/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADBR NF +inf/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADB NF +inf/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADBR NF +inf/+Dnice'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADB NF +inf/+Dnice'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADBR NF +inf/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADB NF +inf/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADBR NF +inf/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADB NF +inf/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'ADBR NF +inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF +inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF +inf/+SNaN'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'ADB NF +inf/+SNaN'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'ADBR NF -QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF -QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF -QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF -QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF -QNaN/-Dnice'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF -QNaN/-Dnice'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF -QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF -QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF -QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF -QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF -QNaN/+Dnice'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF -QNaN/+Dnice'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF -QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF -QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF -QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF -QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF -QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADB NF -QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'ADBR NF -QNaN/+SNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'ADB NF -QNaN/+SNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'ADBR NF +SNaN/-inf'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADB NF +SNaN/-inf'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADBR NF +SNaN/-2.0'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADB NF +SNaN/-2.0'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADBR NF +SNaN/-Dnice'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADB NF +SNaN/-Dnice'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADBR NF +SNaN/-0'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADB NF +SNaN/-0'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADBR NF +SNaN/+0'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADB NF +SNaN/+0'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADBR NF +SNaN/+Dnice'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADB NF +SNaN/+Dnice'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADBR NF +SNaN/+2.0'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADB NF +SNaN/+2.0'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADBR NF +SNaN/+inf'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADB NF +SNaN/+inf'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADBR NF +SNaN/-QNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADB NF +SNaN/-QNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADBR NF +SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'ADB NF +SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
LBFPNFOT_NUM EQU (*-LBFPNFOT_GOOD)/64
*
*
LBFPNFFL_GOOD EQU *
 DC CL48'ADBR/ADB NF -inf/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -inf/-2.0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -inf/-Dnice FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -inf/-0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -inf/+0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -inf/+Dnice FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -inf/+2.0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -inf/+inf FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF -inf/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF -inf/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF -2.0/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -2.0/-2.0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -2.0/-Dnice FPCR'
 DC XL16'00080001F800080100080001F8000801'
 DC CL48'ADBR/ADB NF -2.0/-0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -2.0/+0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -2.0/+Dnice FPCR'
 DC XL16'00080001F8000C0100080001F8000C01'
 DC CL48'ADBR/ADB NF -2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'ADBR/ADB NF -2.0/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF -2.0/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF -2.0/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF -Dnice/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -Dnice/-2.0 FPCR'
 DC XL16'00080001F800080100080001F8000801'
 DC CL48'ADBR/ADB NF -Dnice/-Dnice FPCR'
 DC XL16'00000001F800100100000001F8001001'
 DC CL48'ADBR/ADB NF -Dnice/-0 FPCR'
 DC XL16'00000001F800100100000001F8001001'
 DC CL48'ADBR/ADB NF -Dnice/+0 FPCR'
 DC XL16'00000001F800100100000001F8001001'
 DC CL48'ADBR/ADB NF -Dnice/+Dnice FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'ADBR/ADB NF -Dnice/+2.0 FPCR'
 DC XL16'00080002F8000C0200080002F8000C02'
 DC CL48'ADBR/ADB NF -Dnice/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF -Dnice/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF -Dnice/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF -0/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -0/-2.0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF -0/-Dnice FPCR'
 DC XL16'00000001F800100100000001F8001001'
 DC CL48'ADBR/ADB NF -0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'ADBR/ADB NF -0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'ADBR/ADB NF -0/+Dnice FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'ADBR/ADB NF -0/+2.0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF -0/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF -0/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF -0/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +0/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF +0/-2.0 FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF +0/-Dnice FPCR'
 DC XL16'00000001F800100100000001F8001001'
 DC CL48'ADBR/ADB NF +0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'ADBR/ADB NF +0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'ADBR/ADB NF +0/+Dnice FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'ADBR/ADB NF +0/+2.0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +0/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +0/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF +0/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +Dnice/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF +Dnice/-2.0 FPCR'
 DC XL16'00080001F8000C0100080001F8000C01'
 DC CL48'ADBR/ADB NF +Dnice/-Dnice FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'ADBR/ADB NF +Dnice/-0 FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'ADBR/ADB NF +Dnice/+0 FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'ADBR/ADB NF +Dnice/+Dnice FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'ADBR/ADB NF +Dnice/+2.0 FPCR'
 DC XL16'00080002F800080200080002F8000802'
 DC CL48'ADBR/ADB NF +Dnice/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +Dnice/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF +Dnice/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +2.0/-inf FPCR'
 DC XL16'00000001F800000100000001F8000001'
 DC CL48'ADBR/ADB NF +2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'ADBR/ADB NF +2.0/-Dnice FPCR'
 DC XL16'00080002F8000C0200080002F8000C02'
 DC CL48'ADBR/ADB NF +2.0/-0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +2.0/+0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +2.0/+Dnice FPCR'
 DC XL16'00080002F800080200080002F8000802'
 DC CL48'ADBR/ADB NF +2.0/+2.0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +2.0/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +2.0/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF +2.0/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +inf/-inf FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +inf/-2.0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +inf/-Dnice FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +inf/-0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +inf/+0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +inf/+Dnice FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +inf/+2.0 FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +inf/+inf FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB NF +inf/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF +inf/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF -QNaN/-inf FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF -QNaN/-2.0 FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF -QNaN/-Dnice FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF -QNaN/-0 FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF -QNaN/+0 FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF -QNaN/+Dnice FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF -QNaN/+2.0 FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF -QNaN/+inf FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF -QNaN/-QNaN FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB NF -QNaN/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +SNaN/-inf FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +SNaN/-2.0 FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +SNaN/-Dnice FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +SNaN/-0 FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +SNaN/+0 FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +SNaN/+Dnice FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +SNaN/+2.0 FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +SNaN/+inf FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +SNaN/-QNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
 DC CL48'ADBR/ADB NF +SNaN/+SNaN FPCR'
 DC XL16'00800003F800800300800003F8008003'
LBFPNFFL_NUM EQU (*-LBFPNFFL_GOOD)/64
*
*
LBFPOUT_GOOD EQU *
 DC CL48'ADBR F Ovfl'
 DC XL16'7FFFFFFFFFFFFFFF7FFFFFFFFFFFFFFF'
 DC CL48'ADB F Ovfl'
 DC XL16'7FFFFFFFFFFFFFFF7FFFFFFFFFFFFFFF'
 DC CL48'ADBR F Ufl 1'
 DC XL16'000FFFFFFFFFFFFF600FFFFFFFFFFFFE'
 DC CL48'ADB F Ufl 1'
 DC XL16'000FFFFFFFFFFFFF600FFFFFFFFFFFFE'
 DC CL48'ADBR F Ufl 2'
 DC XL16'0008F0F0000000006001E1E000000000'
 DC CL48'ADB F Ufl 2'
 DC XL16'0008F0F0000000006001E1E000000000'
 DC CL48'ADBR F Nmin'
 DC XL16'00100000000000000010000000000000'
 DC CL48'ADB F Nmin'
 DC XL16'00100000000000000010000000000000'
 DC CL48'ADBR F Incr'
 DC XL16'3FF00000000000013FF0000000000001'
 DC CL48'ADB F Incr'
 DC XL16'3FF00000000000013FF0000000000001'
 DC CL48'ADBR F Trun'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'ADB F Trun'
 DC XL16'3FF00000000000003FF0000000000000'
LBFPOUT_NUM EQU (*-LBFPOUT_GOOD)/64
*
*
LBFPFLGS_GOOD EQU *
 DC CL48'ADBR/ADB F Ovfl FPCR'
 DC XL16'00000003F800000300000003F8000003'
 DC CL48'ADBR/ADB F Ufl 1 FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'ADBR/ADB F Ufl 2 FPCR'
 DC XL16'00000002F800100200000002F8001002'
 DC CL48'ADBR/ADB F Nmin FPCR'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'ADBR/ADB F Incr FPCR'
 DC XL16'00080002F8000C0200080002F8000C02'
 DC CL48'ADBR/ADB F Trun FPCR'
 DC XL16'00080002F800080200080002F8000802'
LBFPFLGS_NUM EQU (*-LBFPFLGS_GOOD)/64
*
*
LBFPRMO_GOOD EQU *
 DC CL48'ADBR/ADB RM +NZ  RNTE'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'ADBR/ADB RM +NZ  RZ'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'ADBR/ADB RM +NZ  RP'
 DC XL16'3FF00000000000013FF0000000000001'
 DC CL48'ADBR/ADB RM +NZ  RM'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'ADBR/ADB RM +NZ  RFS'
 DC XL16'3FF00000000000013FF0000000000001'
 DC CL48'ADBR/ADB RM -NZ  RNTE'
 DC XL16'BFF0000000000000BFF0000000000000'
 DC CL48'ADBR/ADB RM -NZ  RZ'
 DC XL16'BFF0000000000000BFF0000000000000'
 DC CL48'ADBR/ADB RM -NZ  RP'
 DC XL16'BFF0000000000000BFF0000000000000'
 DC CL48'ADBR/ADB RM -NZ  RM'
 DC XL16'BFF0000000000001BFF0000000000001'
 DC CL48'ADBR/ADB RM -NZ  RFS'
 DC XL16'BFF0000000000001BFF0000000000001'
 DC CL48'ADBR/ADB RM +NA  RNTE'
 DC XL16'3FF00000000000013FF0000000000001'
 DC CL48'ADBR/ADB RM +NA  RZ'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'ADBR/ADB RM +NA  RP'
 DC XL16'3FF00000000000013FF0000000000001'
 DC CL48'ADBR/ADB RM +NA  RM'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'ADBR/ADB RM +NA  RFS'
 DC XL16'3FF00000000000013FF0000000000001'
 DC CL48'ADBR/ADB RM -NA  RNTE'
 DC XL16'BFF0000000000001BFF0000000000001'
 DC CL48'ADBR/ADB RM -NA  RZ'
 DC XL16'BFF0000000000000BFF0000000000000'
 DC CL48'ADBR/ADB RM -NA  RP'
 DC XL16'BFF0000000000000BFF0000000000000'
 DC CL48'ADBR/ADB RM -NA  RM'
 DC XL16'BFF0000000000001BFF0000000000001'
 DC CL48'ADBR/ADB RM -NA  RFS'
 DC XL16'BFF0000000000001BFF0000000000001'
 DC CL48'ADBR/ADB RM +TZ  RNTE'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'ADBR/ADB RM +TZ  RZ'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'ADBR/ADB RM +TZ  RP'
 DC XL16'3FF00000000000013FF0000000000001'
 DC CL48'ADBR/ADB RM +TZ  RM'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'ADBR/ADB RM +TZ  RFS'
 DC XL16'3FF00000000000013FF0000000000001'
 DC CL48'ADBR/ADB RM -TZ  RNTE'
 DC XL16'BFF0000000000000BFF0000000000000'
 DC CL48'ADBR/ADB RM -TZ  RZ'
 DC XL16'BFF0000000000000BFF0000000000000'
 DC CL48'ADBR/ADB RM -TZ  RP'
 DC XL16'BFF0000000000000BFF0000000000000'
 DC CL48'ADBR/ADB RM -TZ  RM'
 DC XL16'BFF0000000000001BFF0000000000001'
 DC CL48'ADBR/ADB RM -TZ  RFS'
 DC XL16'BFF0000000000001BFF0000000000001'
 DC CL48'ADBR/ADB RM +TA  RNTE'
 DC XL16'3FF00000000000023FF0000000000002'
 DC CL48'ADBR/ADB RM +TA  RZ'
 DC XL16'3FF00000000000013FF0000000000001'
 DC CL48'ADBR/ADB RM +TA  RP'
 DC XL16'3FF00000000000023FF0000000000002'
 DC CL48'ADBR/ADB RM +TA  RM'
 DC XL16'3FF00000000000013FF0000000000001'
 DC CL48'ADBR/ADB RM +TA  RFS'
 DC XL16'3FF00000000000013FF0000000000001'
 DC CL48'ADBR/ADB RM -TA  RNTE'
 DC XL16'BFF0000000000002BFF0000000000002'
 DC CL48'ADBR/ADB RM -TA  RZ'
 DC XL16'BFF0000000000001BFF0000000000001'
 DC CL48'ADBR/ADB RM -TA  RP'
 DC XL16'BFF0000000000001BFF0000000000001'
 DC CL48'ADBR/ADB RM -TA  RM'
 DC XL16'BFF0000000000002BFF0000000000002'
 DC CL48'ADBR/ADB RM -TA  RFS'
 DC XL16'BFF0000000000001BFF0000000000001'
LBFPRMO_NUM EQU (*-LBFPRMO_GOOD)/64
*
*
LBFPRMOF_GOOD EQU *
 DC CL48'ADBR/ADB RM +NZ RNTE, RZ FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'ADBR/ADB RM +NZ RP, RM FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'ADBR/ADB RM +NZ RFS FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'ADBR/ADB RM +NZ RNTE, RZ FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'ADBR/ADB RM +NZ RP, RM FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'ADBR/ADB RM -NZ RFS FPCR'
 DC XL16'00080001000800010000000000000000'
 DC CL48'ADBR/ADB RM -NZ RNTE, RZ FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'ADBR/ADB RM -NZ RP, RM FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'ADBR/ADB RM -NZ RFS FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'ADBR/ADB RM -NZ RNTE, RZ FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'ADBR/ADB RM -NA RP, RM FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'ADBR/ADB RM -NA RFS FPCR'
 DC XL16'00080001000800010000000000000000'
 DC CL48'ADBR/ADB RM +TZ RNTE, RZ FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'ADBR/ADB RM +TZ RP, RM FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'ADBR/ADB RM +TZ RFS FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'ADBR/ADB RM -TZ RNTE, RZ FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'ADBR/ADB RM -TZ RP, RM FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'ADBR/ADB RM -TZ RFS FPCR'
 DC XL16'00080001000800010000000000000000'
 DC CL48'ADBR/ADB RM +TA RNTE, RZ FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'ADBR/ADB RM +TA RP, RM FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'ADBR/ADB RM +TA RFS FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'ADBR/ADB RM -TA RNTE, RZ FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'ADBR/ADB RM -TA RP, RM FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'ADBR/ADB RM -TA RFS FPCR'
 DC XL16'00080001000800010000000000000000'
LBFPRMOF_NUM EQU (*-LBFPRMOF_GOOD)/64
*
*
XBFPNFOT_GOOD EQU *
 DC CL48'AXBR NF -inf/-inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/-inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/-2.0 NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/-2.0 Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/-Dnice NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/-Dnice Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/-0 NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/-0 Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/+0 NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/+0 Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/+Dnice NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/+Dnice Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/+2.0 NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/+2.0 Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/+inf NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'AXBR NF -inf/+inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -inf/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -inf/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -inf/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF -inf/+SNaN Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -2.0/-inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -2.0/-inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -2.0/-2.0 NT'
 DC XL16'C0010000000000000000000000000000'
 DC CL48'AXBR NF -2.0/-2.0 Tr'
 DC XL16'C0010000000000000000000000000000'
 DC CL48'AXBR NF -2.0/-Dnice NT'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF -2.0/-Dnice Tr'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF -2.0/-0 NT'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF -2.0/-0 Tr'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF -2.0/+0 NT'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF -2.0/+0 Tr'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF -2.0/+Dnice NT'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF -2.0/+Dnice Tr'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF -2.0/+2.0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF -2.0/+2.0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF -2.0/+inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF -2.0/+inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF -2.0/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -2.0/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -2.0/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF -2.0/+SNaN Tr'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/-inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/-inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/-2.0 NT'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/-2.0 Tr'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/-Dnice NT'
 DC XL16'80002000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/-Dnice Tr'
 DC XL16'DFFE0000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/-0 NT'
 DC XL16'80001000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/-0 Tr'
 DC XL16'DFFD0000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/+0 NT'
 DC XL16'80001000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/+0 Tr'
 DC XL16'DFFD0000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/+Dnice NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/+Dnice Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/+2.0 NT'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/+2.0 Tr'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/+inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/+inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF -Dnice/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -Dnice/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -Dnice/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF -Dnice/+SNaN Tr'
 DC XL16'80001000000000000000000000000000'
 DC CL48'AXBR NF -0/-inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -0/-inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF -0/-2.0 NT'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF -0/-2.0 Tr'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF -0/-Dnice NT'
 DC XL16'80001000000000000000000000000000'
 DC CL48'AXBR NF -0/-Dnice Tr'
 DC XL16'DFFD0000000000000000000000000000'
 DC CL48'AXBR NF -0/-0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'AXBR NF -0/-0 Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'AXBR NF -0/+0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF -0/+0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF -0/+Dnice NT'
 DC XL16'00001000000000000000000000000000'
 DC CL48'AXBR NF -0/+Dnice Tr'
 DC XL16'5FFD0000000000000000000000000000'
 DC CL48'AXBR NF -0/+2.0 NT'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF -0/+2.0 Tr'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF -0/+inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF -0/+inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF -0/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -0/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -0/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF -0/+SNaN Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'AXBR NF +0/-inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF +0/-inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF +0/-2.0 NT'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF +0/-2.0 Tr'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF +0/-Dnice NT'
 DC XL16'80001000000000000000000000000000'
 DC CL48'AXBR NF +0/-Dnice Tr'
 DC XL16'DFFD0000000000000000000000000000'
 DC CL48'AXBR NF +0/-0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF +0/-0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF +0/+0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF +0/+0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF +0/+Dnice NT'
 DC XL16'00001000000000000000000000000000'
 DC CL48'AXBR NF +0/+Dnice Tr'
 DC XL16'5FFD0000000000000000000000000000'
 DC CL48'AXBR NF +0/+2.0 NT'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF +0/+2.0 Tr'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF +0/+inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +0/+inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +0/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF +0/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF +0/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +0/+SNaN Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/-inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/-inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/-2.0 NT'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/-2.0 Tr'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/-Dnice NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/-Dnice Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/-0 NT'
 DC XL16'00001000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/-0 Tr'
 DC XL16'5FFD0000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/+0 NT'
 DC XL16'00001000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/+0 Tr'
 DC XL16'5FFD0000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/+Dnice NT'
 DC XL16'00002000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/+Dnice Tr'
 DC XL16'5FFE0000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/+2.0 NT'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/+2.0 Tr'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/+inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/+inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +Dnice/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF +Dnice/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF +Dnice/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +Dnice/+SNaN Tr'
 DC XL16'00001000000000000000000000000000'
 DC CL48'AXBR NF +2.0/-inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF +2.0/-inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'AXBR NF +2.0/-2.0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF +2.0/-2.0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'AXBR NF +2.0/-Dnice NT'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF +2.0/-Dnice Tr'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF +2.0/-0 NT'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF +2.0/-0 Tr'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF +2.0/+0 NT'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF +2.0/+0 Tr'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF +2.0/+Dnice NT'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF +2.0/+Dnice Tr'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF +2.0/+2.0 NT'
 DC XL16'40010000000000000000000000000000'
 DC CL48'AXBR NF +2.0/+2.0 Tr'
 DC XL16'40010000000000000000000000000000'
 DC CL48'AXBR NF +2.0/+inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +2.0/+inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +2.0/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF +2.0/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF +2.0/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +2.0/+SNaN Tr'
 DC XL16'40000000000000000000000000000000'
 DC CL48'AXBR NF +inf/-inf NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'AXBR NF +inf/-inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/-2.0 NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/-2.0 Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/-Dnice NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/-Dnice Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/-0 NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/-0 Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/+0 NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/+0 Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/+Dnice NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/+Dnice Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/+2.0 NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/+2.0 Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/+inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/+inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF +inf/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF +inf/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF +inf/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +inf/+SNaN Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'AXBR NF -QNaN/-inf NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/-inf Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/-2.0 NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/-2.0 Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/-Dnice NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/-Dnice Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/-0 NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/-0 Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/+0 NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/+0 Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/+Dnice NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/+Dnice Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/+2.0 NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/+2.0 Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/+inf NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/+inf Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF -QNaN/+SNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/-inf NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/-inf Tr'
 DC XL16'7FFF0A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/-2.0 NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/-2.0 Tr'
 DC XL16'7FFF0A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/-Dnice NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/-Dnice Tr'
 DC XL16'7FFF0A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/-0 NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/-0 Tr'
 DC XL16'7FFF0A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/+0 NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/+0 Tr'
 DC XL16'7FFF0A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/+Dnice NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/+Dnice Tr'
 DC XL16'7FFF0A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/+2.0 NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/+2.0 Tr'
 DC XL16'7FFF0A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/+inf NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/+inf Tr'
 DC XL16'7FFF0A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/-QNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/-QNaN Tr'
 DC XL16'7FFF0A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'AXBR NF +SNaN/+SNaN Tr'
 DC XL16'7FFF0A00000000000000000000000000'
XBFPNFOT_NUM EQU (*-XBFPNFOT_GOOD)/64
*
*
XBFPNFFL_GOOD EQU *
 DC CL48'AXBR NF -inf/-inf FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -inf/-2.0 FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -inf/-Dnice FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -inf/-0 FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -inf/+0 FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -inf/+Dnice FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -inf/+2.0 FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -inf/+inf FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF -inf/-QNaN FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF -inf/+SNaN FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF -2.0/-inf FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -2.0/-2.0 FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -2.0/-Dnice FPCR'
 DC XL16'00080001F80008010000000000000000'
 DC CL48'AXBR NF -2.0/-0 FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -2.0/+0 FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -2.0/+Dnice FPCR'
 DC XL16'00080001F8000C010000000000000000'
 DC CL48'AXBR NF -2.0/+2.0 FPCR'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'AXBR NF -2.0/+inf FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF -2.0/-QNaN FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF -2.0/+SNaN FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF -Dnice/-inf FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -Dnice/-2.0 FPCR'
 DC XL16'00080001F80008010000000000000000'
 DC CL48'AXBR NF -Dnice/-Dnice FPCR'
 DC XL16'00000001F80010010000000000000000'
 DC CL48'AXBR NF -Dnice/-0 FPCR'
 DC XL16'00000001F80010010000000000000000'
 DC CL48'AXBR NF -Dnice/+0 FPCR'
 DC XL16'00000001F80010010000000000000000'
 DC CL48'AXBR NF -Dnice/+Dnice FPCR'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'AXBR NF -Dnice/+2.0 FPCR'
 DC XL16'00080002F8000C020000000000000000'
 DC CL48'AXBR NF -Dnice/+inf FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF -Dnice/-QNaN FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF -Dnice/+SNaN FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF -0/-inf FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -0/-2.0 FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF -0/-Dnice FPCR'
 DC XL16'00000001F80010010000000000000000'
 DC CL48'AXBR NF -0/-0 FPCR'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'AXBR NF -0/+0 FPCR'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'AXBR NF -0/+Dnice FPCR'
 DC XL16'00000002F80010020000000000000000'
 DC CL48'AXBR NF -0/+2.0 FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF -0/+inf FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF -0/-QNaN FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF -0/+SNaN FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +0/-inf FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF +0/-2.0 FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF +0/-Dnice FPCR'
 DC XL16'00000001F80010010000000000000000'
 DC CL48'AXBR NF +0/-0 FPCR'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'AXBR NF +0/+0 FPCR'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'AXBR NF +0/+Dnice FPCR'
 DC XL16'00000002F80010020000000000000000'
 DC CL48'AXBR NF +0/+2.0 FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +0/+inf FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +0/-QNaN FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF +0/+SNaN FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +Dnice/-inf FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF +Dnice/-2.0 FPCR'
 DC XL16'00080001F8000C010000000000000000'
 DC CL48'AXBR NF +Dnice/-Dnice FPCR'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'AXBR NF +Dnice/-0 FPCR'
 DC XL16'00000002F80010020000000000000000'
 DC CL48'AXBR NF +Dnice/+0 FPCR'
 DC XL16'00000002F80010020000000000000000'
 DC CL48'AXBR NF +Dnice/+Dnice FPCR'
 DC XL16'00000002F80010020000000000000000'
 DC CL48'AXBR NF +Dnice/+2.0 FPCR'
 DC XL16'00080002F80008020000000000000000'
 DC CL48'AXBR NF +Dnice/+inf FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +Dnice/-QNaN FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF +Dnice/+SNaN FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +2.0/-inf FPCR'
 DC XL16'00000001F80000010000000000000000'
 DC CL48'AXBR NF +2.0/-2.0 FPCR'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'AXBR NF +2.0/-Dnice FPCR'
 DC XL16'00080002F8000C020000000000000000'
 DC CL48'AXBR NF +2.0/-0 FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +2.0/+0 FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +2.0/+Dnice FPCR'
 DC XL16'00080002F80008020000000000000000'
 DC CL48'AXBR NF +2.0/+2.0 FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +2.0/+inf FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +2.0/-QNaN FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF +2.0/+SNaN FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +inf/-inf FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +inf/-2.0 FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +inf/-Dnice FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +inf/-0 FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +inf/+0 FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +inf/+Dnice FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +inf/+2.0 FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +inf/+inf FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR NF +inf/-QNaN FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF +inf/+SNaN FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF -QNaN/-inf FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF -QNaN/-2.0 FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF -QNaN/-Dnice FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF -QNaN/-0 FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF -QNaN/+0 FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF -QNaN/+Dnice FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF -QNaN/+2.0 FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF -QNaN/+inf FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF -QNaN/-QNaN FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR NF -QNaN/+SNaN FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +SNaN/-inf FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +SNaN/-2.0 FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +SNaN/-Dnice FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +SNaN/-0 FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +SNaN/+0 FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +SNaN/+Dnice FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +SNaN/+2.0 FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +SNaN/+inf FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +SNaN/-QNaN FPCR'
 DC XL16'00800003F80080030000000000000000'
 DC CL48'AXBR NF +SNaN/+SNaN FPCR'
 DC XL16'00800003F80080030000000000000000'
XBFPNFFL_NUM EQU (*-XBFPNFFL_GOOD)/64
*
*
XBFPOUT_GOOD EQU *
 DC CL48'AXBR F Ovfl NT'
 DC XL16'7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL48'AXBR F Ovfl Tr'
 DC XL16'7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL48'AXBR F Ufl 1 NT'
 DC XL16'0000FFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL48'AXBR F Ufl 1 Tr'
 DC XL16'6000FFFFFFFFFFFFFFFFFFFFFFFFFFFE'
 DC CL48'AXBR F Ufl 2 NT'
 DC XL16'00008F0F000000000000000000000000'
 DC CL48'AXBR F Ufl 2 Tr'
 DC XL16'60001E1E000000000000000000000000'
 DC CL48'AXBR F Nmin NT'
 DC XL16'00010000000000000000000000000000'
 DC CL48'AXBR F Nmin Tr'
 DC XL16'00010000000000000000000000000000'
 DC CL48'AXBR F Incr NT'
 DC XL16'3FFF0000000000000000000000000001'
 DC CL48'AXBR F Incr Tr'
 DC XL16'3FFF0000000000000000000000000001'
 DC CL48'AXBR F Trun NT'
 DC XL16'3FFF0000000000000000000000000000'
 DC CL48'AXBR F Trun Tr'
 DC XL16'3FFF0000000000000000000000000000'
XBFPOUT_NUM EQU (*-XBFPOUT_GOOD)/64
*
*
XBFPFLGS_GOOD EQU *
 DC CL48'AXBR F Ovfl FPCR'
 DC XL16'00000003F80000030000000000000000'
 DC CL48'AXBR F Ufl 1 FPCR'
 DC XL16'00000002F80010020000000000000000'
 DC CL48'AXBR F Ufl 2 FPCR'
 DC XL16'00000002F80010020000000000000000'
 DC CL48'AXBR F Nmin FPCR'
 DC XL16'00000002F80000020000000000000000'
 DC CL48'AXBR F Incr FPCR'
 DC XL16'00080002F8000C020000000000000000'
 DC CL48'AXBR F Trun FPCR'
 DC XL16'00080002F80008020000000000000000'
XBFPFLGS_NUM EQU (*-XBFPFLGS_GOOD)/64
*
*
XBFPRMO_GOOD EQU *
 DC CL48'AXBR RM +NZ RNTE'
 DC XL16'3FFF0000000000000000000000000000'
 DC CL48'AXBR RM +NZ RZ'
 DC XL16'3FFF0000000000000000000000000000'
 DC CL48'AXBR RM +NZ RP'
 DC XL16'3FFF0000000000000000000000000001'
 DC CL48'AXBR RM +NZ RM'
 DC XL16'3FFF0000000000000000000000000000'
 DC CL48'AXBR RM +NZ RFS'
 DC XL16'3FFF0000000000000000000000000001'
 DC CL48'AXBR RM -NZ RNTE'
 DC XL16'BFFF0000000000000000000000000000'
 DC CL48'AXBR RM -NZ RZ'
 DC XL16'BFFF0000000000000000000000000000'
 DC CL48'AXBR RM -NZ RP'
 DC XL16'BFFF0000000000000000000000000000'
 DC CL48'AXBR RM -NZ RM'
 DC XL16'BFFF0000000000000000000000000001'
 DC CL48'AXBR RM -NZ RFS'
 DC XL16'BFFF0000000000000000000000000001'
 DC CL48'AXBR RM +NA RNTE'
 DC XL16'3FFF0000000000000000000000000001'
 DC CL48'AXBR RM +NA RZ'
 DC XL16'3FFF0000000000000000000000000000'
 DC CL48'AXBR RM +NA RP'
 DC XL16'3FFF0000000000000000000000000001'
 DC CL48'AXBR RM +NA RM'
 DC XL16'3FFF0000000000000000000000000000'
 DC CL48'AXBR RM +NA RFS'
 DC XL16'3FFF0000000000000000000000000001'
 DC CL48'AXBR RM -NA RNTE'
 DC XL16'BFFF0000000000000000000000000001'
 DC CL48'AXBR RM -NA RZ'
 DC XL16'BFFF0000000000000000000000000000'
 DC CL48'AXBR RM -NA RP'
 DC XL16'BFFF0000000000000000000000000000'
 DC CL48'AXBR RM -NA RM'
 DC XL16'BFFF0000000000000000000000000001'
 DC CL48'AXBR RM -NA RFS'
 DC XL16'BFFF0000000000000000000000000001'
 DC CL48'AXBR RM +TZ RNTE'
 DC XL16'3FFF0000000000000000000000000000'
 DC CL48'AXBR RM +TZ RZ'
 DC XL16'3FFF0000000000000000000000000000'
 DC CL48'AXBR RM +TZ RP'
 DC XL16'3FFF0000000000000000000000000001'
 DC CL48'AXBR RM +TZ RM'
 DC XL16'3FFF0000000000000000000000000000'
 DC CL48'AXBR RM +TZ RFS'
 DC XL16'3FFF0000000000000000000000000001'
 DC CL48'AXBR RM -TZ RNTE'
 DC XL16'BFFF0000000000000000000000000000'
 DC CL48'AXBR RM -TZ RZ'
 DC XL16'BFFF0000000000000000000000000000'
 DC CL48'AXBR RM -TZ RP'
 DC XL16'BFFF0000000000000000000000000000'
 DC CL48'AXBR RM -TZ RM'
 DC XL16'BFFF0000000000000000000000000001'
 DC CL48'AXBR RM -TZ RFS'
 DC XL16'BFFF0000000000000000000000000001'
 DC CL48'AXBR RM +TA RNTE'
 DC XL16'3FFF0000000000000000000000000002'
 DC CL48'AXBR RM +TA RZ'
 DC XL16'3FFF0000000000000000000000000001'
 DC CL48'AXBR RM +TA RP'
 DC XL16'3FFF0000000000000000000000000002'
 DC CL48'AXBR RM +TA RM'
 DC XL16'3FFF0000000000000000000000000001'
 DC CL48'AXBR RM +TA RFS'
 DC XL16'3FFF0000000000000000000000000001'
 DC CL48'AXBR RM -TA RNTE'
 DC XL16'BFFF0000000000000000000000000002'
 DC CL48'AXBR RM -TA RZ'
 DC XL16'BFFF0000000000000000000000000001'
 DC CL48'AXBR RM -TA RP'
 DC XL16'BFFF0000000000000000000000000001'
 DC CL48'AXBR RM -TA RM'
 DC XL16'BFFF0000000000000000000000000002'
 DC CL48'AXBR RM -TA RFS'
 DC XL16'BFFF0000000000000000000000000001'
XBFPRMO_NUM EQU (*-XBFPRMO_GOOD)/64
*
*
XBFPRMOF_GOOD EQU *
 DC CL48'AXBR RM +NZ FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'AXBR RM +NZ FPCR'
 DC XL16'00080002000000000000000000000000'
 DC CL48'AXBR RM -NZ FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'AXBR RM -NZ FPCR'
 DC XL16'00080001000000000000000000000000'
 DC CL48'AXBR RM +NA FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'AXBR RM +NA FPCR'
 DC XL16'00080002000000000000000000000000'
 DC CL48'AXBR RM -NA FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'AXBR RM -NA FPCR'
 DC XL16'00080001000000000000000000000000'
 DC CL48'AXBR RM +TZ FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'AXBR RM +TZ FPCR'
 DC XL16'00080002000000000000000000000000'
 DC CL48'AXBR RM -TZ FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'AXBR RM -TZ FPCR'
 DC XL16'00080001000000000000000000000000'
 DC CL48'AXBR RM +TA FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'AXBR RM +TA FPCR'
 DC XL16'00080002000000000000000000000000'
 DC CL48'AXBR RM -TA FPCR'
 DC XL16'00080001000800010008000100080001'
 DC CL48'AXBR RM -TA FPCR'
 DC XL16'00080001000000000000000000000000'
XBFPRMOF_NUM EQU (*-XBFPRMOF_GOOD)/64
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
         DC    A(SBFPNFOT)
         DC    A(SBFPNFOT_GOOD)
         DC    A(SBFPNFOT_NUM)
*
         DC    A(SBFPNFFL)
         DC    A(SBFPNFFL_GOOD)
         DC    A(SBFPNFFL_NUM)
*
         DC    A(SBFPOUT)
         DC    A(SBFPOUT_GOOD)
         DC    A(SBFPOUT_NUM)
*
         DC    A(SBFPFLGS)
         DC    A(SBFPFLGS_GOOD)
         DC    A(SBFPFLGS_NUM)
*
         DC    A(SBFPRMO)
         DC    A(SBFPRMO_GOOD)
         DC    A(SBFPRMO_NUM)
*
         DC    A(SBFPRMOF)
         DC    A(SBFPRMOF_GOOD)
         DC    A(SBFPRMOF_NUM)
*
         DC    A(LBFPNFOT)
         DC    A(LBFPNFOT_GOOD)
         DC    A(LBFPNFOT_NUM)
*
         DC    A(LBFPNFFL)
         DC    A(LBFPNFFL_GOOD)
         DC    A(LBFPNFFL_NUM)
*
         DC    A(LBFPOUT)
         DC    A(LBFPOUT_GOOD)
         DC    A(LBFPOUT_NUM)
*
         DC    A(LBFPFLGS)
         DC    A(LBFPFLGS_GOOD)
         DC    A(LBFPFLGS_NUM)
*
         DC    A(LBFPRMO)
         DC    A(LBFPRMO_GOOD)
         DC    A(LBFPRMO_NUM)
*
         DC    A(LBFPRMOF)
         DC    A(LBFPRMOF_GOOD)
         DC    A(LBFPRMOF_NUM)
*
         DC    A(XBFPNFOT)
         DC    A(XBFPNFOT_GOOD)
         DC    A(XBFPNFOT_NUM)
*
         DC    A(XBFPNFFL)
         DC    A(XBFPNFFL_GOOD)
         DC    A(XBFPNFFL_NUM)
*
         DC    A(XBFPOUT)
         DC    A(XBFPOUT_GOOD)
         DC    A(XBFPOUT_NUM)
*
         DC    A(XBFPFLGS)
         DC    A(XBFPFLGS_GOOD)
         DC    A(XBFPFLGS_NUM)
*
         DC    A(XBFPRMO)
         DC    A(XBFPRMO_GOOD)
         DC    A(XBFPRMO_NUM)
*
         DC    A(XBFPRMOF)
         DC    A(XBFPRMOF_GOOD)
         DC    A(XBFPRMOF_NUM)
*
VERIFLEN EQU   (*-VERIFTAB)/12    #of entries in verify table
                                                                EJECT
         END
