  TITLE 'bfp-022-multsub: Test IEEE Multiply And Subtract'
***********************************************************************
*
*Testcase IEEE MULTIPLY AND SUBTRACT
*  Test case capability includes IEEE exceptions trappable and
*  otherwise. Test results, FPCR flags, the Condition code, and any
*  DXC are saved for all tests.
*
*  This test program is focused on the four fused Multiply And Subtract
*  instructions.  Standard Multiply and Multiply to longer precision
*  are tested in other programs.
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
*                       bfp-022-multsub.asm
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
*   MULTIPLY AND SUBTRACT (short BFP, RRE)
*   MULTIPLY AND SUBTRACT (long BFP, RRE)
*   MULTIPLY AND SUBTRACT (short BFP, RXE)
*   MULTIPLY AND SUBTRACT (long BFP, RXE)
*
*
* Test data is compiled into this program. The program itself verifies
* the resulting status of registers and condition codes via a series of
* simple CLC comparisons.
*
* Test Case Order
* 1) Short BFP basic tests, including traps and NaN propagation
* 2) Short BFP finite number tests, including traps and scaling
* 3) Short BFP FPC-controlled rounding mode exhaustive tests
* 4) Long BFP basic tests, including traps and NaN propagation
* 5) Long BFP finite number tests, including traps and scaling
* 6) Long BFP FPC-controlled rounding mode exhaustive tests
*
* Three input test sets are provided each for short and long BFP
*   inputs.  Test values are the same for each precision for most
*   tests.  Overflow and underflow each require precision-
*   dependent test values.
*
* Review of Softfloat code for multiply and add shows that the
* multiplication and addition are performed in precision-independent
* format.  Overflow, underflow, inexact, and incremented are detected
* upon conversion from precision-independent format to the target
* format.  As a result, it should not matter whether overflow etc is
* caused by the multiplication or the addition.  We will include
* a few test cases where this differs in the finite testing section,
* but that's all.
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
BFPMULS  START 0
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
         BAS   R13,SBFPNF    Multiply short BFP non-finites
         LA    R10,SHORTF    Point to short BFP finite inputs
         BAS   R13,SBFPF     Multiply short BFP finites
         LA    R10,RMSHORTS  Point to short BFP rounding mode tests
         BAS   R13,SBFPRM    Multiply short BFP for rounding tests
*
         LA    R10,LONGNF    Point to long BFP non-finite inputs
         BAS   R13,LBFPNF    Multiply long BFP non-finites
         LA    R10,LONGF     Point to long BFP finite inputs
         BAS   R13,LBFPF     Multiply long BFP finites
         LA    R10,RMLONGS   Point to long BFP rounding mode tests
         BAS   R13,LBFPRM    Multiply long BFP for rounding tests
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
         EJECT
***********************************************************************
*
* Perform Multiply And Subtract using provided short BFP inputs.  This
* set of tests checks NaN propagation, operations on values that are
* not finite numbers, and other basic tests.  This set generates
* results that can be validated against Figure 19-24 on page 19-39 of
* SA22-7832-10.
*
* Four results are generated for each input: one RRE with all
* exceptions non-trappable, a second RRE with all exceptions trappable,
* a third RXE with all exceptions non-trappable, a fourth RXE with all
* exceptions trappable.
*
* Because this is a three-operand instruction, validation against
* Figure 19-24, effectively an 8 x 8 x 8 table, will generate a
* phenomonal set of results.  Namely 512 results of 16 bytes each
* plus 512 FPCR contents of 16 bytes each.
*
* The product and FPCR are stored for each result.
*
***********************************************************************
         SPACE 2
SBFPNF   DS    0H            BFP Short non-finite values tests
         LM    R2,R3,0(R10)  Get count and addr of multiplicand values
         LM    R8,R9,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
*
SBFPNFLP DS    0H            Top of outer loop - Multiplicand
         LM    R4,R5,0(R10)  Get count and start of multiplier values
*                            ..which are the same as the multiplicands
         BASR  R12,0         Set top of middle loop
*
         DS    0H            Top of middle loop - multiplier
         LM    R6,R7,0(R10)  Get count and start of subtrahend values
*                            ..which are the same as the multiplicands
         BASR  R1,0          Set top of inner loop - subtrahend
*
* Multiply and Add: R1 = R3 x R2 + R1
*
         LE    FPR4,0(,R3)   Get short BFP multiplicand
         LE    FPR1,0(,R5)   Get short BFP multiplier
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LE    FPR8,0(,R7)   Get short BFP subtrahend
         MSEBR FPR8,FPR4,FPR1   Multiply FPR4 by FPR1, add FPR8 RRE
         STE   FPR8,0(,R8)   Store short BFP product-difference
         STFPC 0(R9)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LE    FPR8,0(,R7)   Get short BFP subtrahend
         MSEBR FPR8,FPR4,FPR1   Multiply FPR4 by FPR1, add FPR8 RRE
         STE   FPR8,4(,R8)   Store short BFP product-difference
         STFPC 4(R9)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LE    FPR8,0(,R7)   Get short BFP subtrahend
         MSEB  FPR8,FPR4,0(,R5)  Mult. FPR4 by multiplier, add FPR8 RXE
         STE   FPR8,8(,R8)   Store short BFP product-difference
         STFPC 8(R9)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LE    FPR8,0(,R7)   Get short BFP subtrahend
         MSEB  FPR8,FPR4,0(,R5)  Mult. FPR4 by multiplier, add FPR8 RXE
         STE   FPR8,12(,R8)  Store short BFP product-difference
         STFPC 12(R9)        Store resulting FPCR flags and DXC
*
         LA    R8,4*4(,R8)   Point to next product-diff. result area
         LA    R9,4*4(,R9)   Point to next FPCR contents area
         LA    R7,4(,R7)     Point to next subtrahend value
         BCTR  R6,R1         Loop through subtrahend values
*
         LA    R5,4(,R5)     Point to next multiplier
         BCTR  R4,R12        Loop through multiplier values
*
         LA    R3,4(,R3)     Point to next multiplicand
         BCT   R2,SBFPNFLP   Loop through multiplicand values
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Multiply And Subtract using provided short BFP input triples.
* This set of tests triggers IEEE exceptions Overflow, Underflow, and
* Inexact and collects both trap and non-trap results.
*
* Four results are generated for each input: one RRE with all
* exceptions non-trappable, a second RRE with all exceptions trappable,
* a third RXE with all exceptions non-trappable, a fourth RXE with all
* exceptions trappable,
*
* The product and FPCR are stored for each result.
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
         LE    FPR4,0(,R3)   Get short BFP multiplicand
         LE    FPR1,1*4(,R3) Get short BFP multiplier
         LE    FPR8,2*4(,R3) Get short BFP subtrahend
         MSEBR FPR8,FPR4,FPR1   Multiply FPR4 by FPR1, add FPR8 RRE
         STE   FPR8,0(,R7)   Store short BFP product-difference
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LE    FPR8,2*4(,R3) Reload short BFP subtrahend
*                            ..multiplier is still in FPR1,
*                            ..multiplicand is still in FPR4
         MSEBR FPR8,FPR4,FPR1   Multiply short FPR8 by FPR1 RRE
         STE   FPR8,1*4(,R7) Store short BFP product-difference
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LE    FPR8,2*4(,R3) Reload short BFP subtrahend
*                            ..multiplicand is still in FPR4
         MSEB  FPR8,FPR4,4(,R3)  Mult. FPR4 by multiplier, add FPR8 RXE
         STE   FPR8,2*4(,R7) Store short BFP product
         STFPC 8(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LE    FPR8,2*4(,R3) Reload short BFP subtrahend
*                            ..multiplicand is still in FPR4
         MSEB  FPR8,FPR4,4(,R3)  Mult. FPR4 by multiplier, add FPR8 RXE
         STE   FPR8,3*4(,R7) Store short BFP product
         STFPC 12(R8)        Store resulting FPCR flags and DXC
*
         LA    R3,3*4(,R3)   Point to next input value trible
         LA    R7,4*4(,R7)   Point to next product result set
         LA    R8,4*4(,R8)   Point to next FPCR result set
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Multiply And Subtract using provided short BFP input triples.
* This set of tests exhaustively tests all rounding modes available for
* Multiply And Subtract.  The rounding mode can only be specified in
* the FPC.
*
* All five FPC rounding modes are tested because the preceeding tests,
* using rounding mode RNTE, do not often create results that require
* rounding.
*
* Two results are generated for each input and rounding mode: one RRE
* and one RXE.  Traps are disabled for all rounding mode tests.
*
* The product and FPCR are stored for each test.
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
         LE    FPR4,0(,R3)   Get short BFP multiplicand
         LE    FPR1,4(,R3)   Get short BFP multiplier
         LE    FPR8,8(,R3)   Get short BFP subtrahend
         MSEBR FPR8,FPR4,FPR1   Multiply FPR4 by FPR1, add FPR8 RRE
         STE   FPR8,0(,R7)   Store short BFP product-difference
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 0(R1)         Set FPC Rounding Mode
         LE    FPR8,8(,R3)   Get short BFP subtrahend
*                            ..multiplicand is still in FPR4
         MSEB  FPR8,FPR4,4(,R3)  Mult. FPR4 by multiplier, add FPR8 RXE
         STE   FPR8,4(,R7)   Store short BFP product-difference
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LA    R7,2*4(,R7)   Point to next product result set
         LA    R8,2*4(,R8)   Point to next FPCR result area
*
         BCTR  R5,R9         Iterate to next FPC mode for this input
*
* End of FPC modes to be tested.  Advance to next test case.  We will
* skip eight bytes of result area so that each set of five result
* value pairs starts at a memory address ending in zero for the
* convenience of memory dump review.
*
         LA    R3,3*4(,R3)   Point to next input value pair triple
         LA    R7,8(,R7)     Skip to start of next result set
         LA    R8,8(,R8)     Skip to start of next FPCR result set
         BCTR  R2,R12        Advance to the next input pair
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Multiply And Subtract using provided long BFP inputs.  This
* set of tests checks NaN propagation, operations on values that are
* not finite numbers, and other basic tests.  This set generates
* results that can be validated against Figure 19-24 on page 19-39 of
* SA22-7832-10.
*
* Four results are generated for each input: one RRE with all
* exceptions non-trappable, a second RRE with all exceptions trappable,
* a third RXE with all exceptions non-trappable, a fourth RXE with all
* exceptions trappable.
*
* Because this is a three-operand instruction, validation against
* Figure 19-24, effectively an 8 x 8 x 8 table, will generate a
* phenomonal set of results.  Namely 512 results of 32 bytes each
* plus 512 FPCR contents of 16 bytes each.
*
* The product and FPCR are stored for each result.
*
***********************************************************************
         SPACE 2
LBFPNF   DS    0H            BFP long non-finite values tests
         LM    R2,R3,0(R10)  Get count and addr of multiplicand values
         LM    R8,R9,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
*
LBFPNFLP DS    0H            Top of outer loop - Multiplicand
         LM    R4,R5,0(R10)  Get count and start of multiplier values
*                            ..which are the same as the multiplicands
         BASR  R12,0         Set top of middle loop
*
         DS    0H            Top of middle loop - multiplier
         LM    R6,R7,0(R10)  Get count and start of subtrahend values
*                            ..which are the same as the multiplicands
         BASR  R1,0          Set top of inner loop - subtrahend
*
* Multiply and Add: R1 = R3 x R2 + R1
*
         LE    FPR4,0(,R3)   Get long BFP multiplicand
         LE    FPR1,0(,R5)   Get long BFP multiplier
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LD    FPR8,0(,R7)   Get long BFP subtrahend
         MSDBR FPR8,FPR4,FPR1   Multiply FPR4 by FPR1, add FPR8 RRE
         STD   FPR8,0(,R8)   Store long BFP product-difference
         STFPC 0(R9)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LE    FPR8,0(,R7)   Get long BFP subtrahend
         MSDBR FPR8,FPR4,FPR1   Multiply FPR4 by FPR1, add FPR8 RRE
         STD   FPR8,1*8(,R8) Store long BFP product-difference
         STFPC 1*4(R9)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LE    FPR8,0(,R7)   Get long BFP subtrahend
         MSDB  FPR8,FPR4,0(,R5)  Mult. FPR4 by multiplier, add FPR8 RXE
         STD   FPR8,2*8(,R8) Store long BFP product-difference
         STFPC 2*4(R9)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LE    FPR8,0(,R7)   Get long BFP subtrahend
         MSDB  FPR8,FPR4,0(,R5)  Mult. FPR4 by multiplier, add FPR8 RXE
         STD   FPR8,3*8(,R8) Store long BFP product-difference
         STFPC 3*4(R9)       Store resulting FPCR flags and DXC
*
         LA    R8,4*8(,R8)   Point to next product-diff. result area
         LA    R9,4*4(,R9)   Point to next FPCR contents area
         LA    R7,8(,R7)     Point to next subtrahend value
         BCTR  R6,R1         Loop through subtrahend values
*
         LA    R5,8(,R5)     Point to next multiplier
         BCTR  R4,R12        Loop through multiplier values
*
         LA    R3,8(,R3)     Point to next multiplicand
         BCT   R2,LBFPNFLP   Loop through multiplicand values
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Multiply And Subtract using provided long BFP input triples.
* This set of tests triggers IEEE exceptions Overflow, Underflow, and
* Inexact and collects non-trap and trap results.
*
* Four results are generated for each input: one RRE with all
* exceptions non-trappable, a second RRE with all exceptions trappable,
* a third RXE with all exceptions non-trappable, a fourth RXE with all
* exceptions trappable,
*
* The product and FPCR are stored for each result.
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
         LD    FPR4,0(,R3)   Get long BFP multiplicand
         LD    FPR1,8(,R3)   Get long BFP multiplier
         LD    FPR8,16(,R3)  Get long BFP subtrahend
         MSDBR FPR8,FPR4,FPR1   Multiply FPR4 by FPR1, add FPR8 RRE
         STD   FPR8,0(,R7)   Store long BFP product
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LD    FPR8,16(,R3)  Reload long BFP subtrahend
*                            ..multiplier is still in FPR1,
*                            ..multiplicand is still in FFR4
         MSDBR FPR8,FPR4,FPR1   Multiply FPR4 by FPR1, add FPR8 RRE
         STD   FPR8,8(,R7)   Store long BFP product-difference
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LD    FPR8,16(,R3)  Reload long BFP subtrahend
*                            ..multiplicand is still in FFR4
         MSDB  FPR8,FPR4,8(,R3)  Mult. FPR4 by multiplier, add FPR8 RXE
         STD   FPR8,2*8(,R7) Store long BFP product-difference
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LD    FPR8,16(,R3)  Reload long BFP subtrahend
*                            ..multiplicand is still in FFR4
         MSDB  FPR8,FPR4,8(,R3)  Mult. FPR4 by multiplier, add FPR8 RXE
         STD   FPR8,3*8(,R7) Store long BFP product-difference
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
*
         LA    R3,3*8(,R3)   Point to next input value triple
         LA    R7,4*8(,R7)   Point to next product-diff. result set
         LA    R8,4*4(,R8)   Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Multiply using provided long BFP input pairs.  This set of
* tests exhaustively tests all rounding modes available for Multiply.
* The rounding mode can only be specified in the FPC.
*
* All five FPC rounding modes are tested because the preceeding tests,
* using rounding mode RNTE, do not often create results that require
* rounding.
*
* Two results are generated for each input and rounding mode: one RRE
* and one RXE.  Traps are disabled for all rounding mode tests.
*
* The product and FPCR are stored for each result.
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
         LD    FPR4,0(,R3)   Get long BFP multiplicand
         LD    FPR1,8(,R3)   Get long BFP multiplier
         LD    FPR8,16(,R3)  Get long BFP subtrahend
         MSDBR FPR8,FPR4,FPR1   Multiply FPR4 by FPR1, add FPR8 RRE
         STD   FPR8,0(,R7)   Store long BFP product-difference
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 0(R1)         Set FPC Rounding Mode
         LD    FPR8,16(,R3)  Reload long BFP subtrahend
         MSDB  FPR8,FPR4,8(,R3)  Multiply long FPR8 by multiplier RXE
         STD   FPR8,8(,R7)   Store long BFP product-difference
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LA    R7,2*8(,R7)   Point to next product result set
         LA    R8,2*4(,R8)   Point to next FPCR result area
*
         BCTR  R5,R9         Iterate to next FPC mode
*
* End of FPC modes to be tested.  Advance to next test case.  We will
* skip eight bytes of FPCR result area so that each set of five result
* FPCR contents pairs starts at a memory address ending in zero for the
* convenience of memory dump review.
*
         LA    R3,3*8(,R3)   Point to next input value triple
         LA    R8,8(,R8)     Skip to start of next FPCR result area
         BCTR  R2,R12        Multiply next input value lots of times
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Table of FPC rounding modes to test product rounding modes.
*
* The Set BFP Rounding Mode does allow specification of the FPC
* rounding mode as an address, so we shall index into a table of
* BFP rounding modes without bothering with Execute.
*
***********************************************************************
         SPACE 2
*
* Rounding modes that may be set in the FPCR.  The FPCR controls
* rounding of the product.
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
* Short BFP test data sets for Multiply And Subtract testing.
*
* The first test data set is used for tests of basic functionality,
* NaN propagation, and results from operations involving other than
* finite numbers.  The same set of eight values is used as the
* multiplicand, multiplier, and subtrahend, resulting in 8 x 8 x 8 or
* 512 test cases.
*
* The second test data set is used for testing boundary conditions
* using two finite non-zero values.  Each possible condition code
* and type of result (normal, scaled, etc) is created by members of
* this test data set.
*
* The third test data set is used for exhaustive testing of final
* results across the five rounding modes available for the Multiply
* instruction.
*
* The strategy for predictable rounding mode testing is to use a
* multiplicand with some one-bits in the low-order byte and multiply
* that by 1/16 (0.0625).  In BFP, this will have the effect of shifting
* the low-order byte out of the target precision representation and
* into the high-order portion of the bits that control rounding.  The
* input low-order byte will be determined by the rounding desired.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
* First input test data set, to test operations using non-finite or
* zero inputs.  Member values chosen to validate Figure 19-24 on page
* 19-39 of SA22-7832-10.  Each value in this table is used as the
* multiplicand, multiplier, and subtrahend.  Eight entries menas 512
* result sets.
*
***********************************************************************
         SPACE 2
SBFPNFIN DS    0F                Inputs for short BFP non-finite tests
         DC    X'FF800000'         -inf
         DC    X'C0000000'         -2.0
         DC    X'80000000'         -0
         DC    X'00000000'         +0
         DC    X'40000000'         +2.0
         DC    X'7F800000'         +inf
         DC    X'FFCB0000'         -QNaN
         DC    X'7F8A0000'         +SNaN
SBFPNFCT EQU   (*-SBFPNFIN)/4    Count of short BFP in list
         SPACE 3
***********************************************************************
*
* Second input test data set.  These are finite triples intended to
* trigger overflow, underflow, and inexact exceptions.  Each triple is
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
* Overflow on multiplication two ways - once on the multiply, once
* on the addition following the multiplication.
*
         DC    X'7F7FFFFF'         +Nmax  multiplicand
         DC    X'FF7FFFFF'         -Nmax  multiplier
         DC    X'7F7FFFFF'         Big positive value, won't show up.
*
         DC    X'7EFFFFFF'         +Nmax / 2 multiplicand
         DC    X'C0000000'         -2.0 multiplier
         DC    X'7F7FFFFF'         +Nmax  subtrahend, triggers overflow
*
* Underflow from product of normals.  We will multiply two small
* normals to generate a subnormal, and then subtract a large subnormal.
*
         DC    X'00800000'         +Nmin
         DC    X'00800000'         +Nmin
         DC    X'00400001'         large subnormal
*
* Underflow from the product of a subnormal and a normal.
*
         DC    X'3F000000'         +0.5
         DC    X'007FFFFF'         +Dmax Subnormal
         DC    X'00000001'         +Dmin, will appear in result
*
* We cannot generate a normal result from product of subnormals
* because the result will be smaller than both the multiplicand and the
* multiplier.  So we'll try multiplying +Dmax by 2.  The result should
* be +Nmin plus the subtrahend.
*
         DC    X'007FFFFF'         +Dmax
         DC    X'40000000'         +2.0
         DC    X'00400000'         +Dmax
*
* Multiply a value from 1.0 such that the added digits are to the right
* of the right-most bit in the stored significand. The result will be
* inexact, and incremented will be determined by the value of the
* bits in the multiplier.  We will add 0.5 to this product because
* that value will not cause renormalization.  Renormalization would
* shift the rounding bits one to the right, messing up the expected
* rounding.
*
         DC    X'3F80000C'   Multiplicand 1.000001430511474609375
         DC    X'BF880000'   Multiplier -1.0625  (1 + 1/16)
         DC    X'3F000000'   Minus 0.5
*..nearest is away from zero, incremented.
*
         DC    X'3F800007'   Multiplicand 1.00000083446502685546875
         DC    X'BF880000'   Multiplier -1.0625  (1 + 1/16)
         DC    X'3F000000'   Minus 0.5
*..nearest is toward zero, truncated
*
SBFPCT   EQU   (*-SBFPIN)/4/3    Count of short BFP in list
         SPACE 3
***********************************************************************
*
* Third input test data set.  These are finite triples intended to
* test all combinations of rounding mode for the product and the
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
* Multiply a value from 1.0 such that the added digits are to the right
* of the right-most bit in the stored significand. The result will be
* inexact, and incremented will be determined by the value of the
* bits in the multiplier.
*
         DC    X'3F800007'   Multiplicand +1.00000083446502685546875
         DC    X'3F880000'   Multiplier 1.0625  (1/16)
         DC    X'BF000000'   Subtrahend -0.5
         DC    X'BF800007'   Multiplicand -1.00000083446502685546875
         DC    X'3F880000'   Multiplier 1.0625  (1/16)
         DC    X'3F000000'   Subtrahend +0.5
*..nearest is toward zero, truncated
*
         DC    X'3F80000C'   Multiplicand +1.000001430511474609375
         DC    X'3F880000'   Multiplier 1.0625  (1/16)
         DC    X'BF000000'   Subtrahend -0.5
         DC    X'BF80000C'   Multiplicand -1.000001430511474609375
         DC    X'3F880000'   Multiplier 1.0625  (1/16)
         DC    X'3F000000'   Subtrahend +0.5
*..nearest is away from zero, incremented.
*
         DC    X'3F800008'   Multiplicand +1.000000476837158203125
         DC    X'3F880000'   Multiplier 1.0625  (1/16)
         DC    X'BF000000'   Subtrahend -0.5
         DC    X'BF800008'   Multiplicand -1.000000476837158203125
         DC    X'3F880000'   Multiplier 1.0625  (1/16)
         DC    X'3F000000'   Subtrahend +0.5
*..nearest is a tie, nearest even has lower magnitude
*
         DC    X'3F800018'   Multiplicand +1.000002384185791015625
         DC    X'3F880000'   Multiplier 1.0625  (1/16)
         DC    X'BF000000'   Subtrahend -0.5
         DC    X'BF800018'   Multiplicand -1.000002384185791015625
         DC    X'3F880000'   Multiplier 1.0625  (1/16)
         DC    X'3F000000'   Subtrahend +0.5
*..nearest is a tie, nearest even has greater magnitude
*
SBFPRMCT EQU   (*-SBFPINRM)/4/3  Count of short BFP rounding tests
         EJECT
***********************************************************************
*
* Long BFP test data sets for Multiply And Subtract testing.
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
* See the Short BFP test cases header for a discussion of test case
* selection for rounding mode test case values.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
* First input test data set, to test operations using non-finite or
* zero inputs.  Member values chosen to validate Figure 19-24 on page
* 19-39 of SA22-7832-10.  Each value in this table is used as the
* multiplicand, multiplier, and subtrahend.  Eight entries menas 512
* result sets.
*
***********************************************************************
         SPACE 2
LBFPNFIN DS    0F                Inputs for long BFP testing
         DC    X'FFF0000000000000'         -inf
         DC    X'C000000000000000'         -2.0
         DC    X'8000000000000000'         -0
         DC    X'0000000000000000'         +0
         DC    X'4000000000000000'         +2.0
         DC    X'7FF0000000000000'         +inf
         DC    X'FFF8B00000000000'         -QNaN
         DC    X'7FF0A00000000000'         +SNaN
LBFPNFCT EQU   (*-LBFPNFIN)/8     Count of long BFP in list
         SPACE 3
***********************************************************************
*
* Second input test data set.  These are finite triples intended to
* trigger overflow, underflow, and inexact exceptions.  Each triples is
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
* Overflow on multiplication two ways.  Once on the muliplication step,
* and then a second time on the addition step.
*
         DC    X'7FEFFFFFFFFFFFFF'  +Nmax
         DC    X'FFEFFFFFFFFFFFFF'  -Nmax
         DC    X'3FF0000000000000'  +1.0
*
         DC    X'7FDFFFFFFFFFFFFF'  +Nmax / 2
         DC    X'C000000000000000'  -2.0
         DC    X'7FEFFFFFFFFFFFFF'  +Nmax
*
* Underflow from product of normals.  We will multiply two small
* normals to generate a subnormal, and then subtract a large subnormal.
*
         DC    X'0010000000000000'  +Nmin
         DC    X'0010000000000000'  +Nmin
         DC    X'0008000000000001'  A very large subnormal
*
* Underflow from the product of a subnormal and a normal.
*
         DC    X'3FE0000000000000'  +0.5
         DC    X'000FFFFFFFFFFFFF'  +Dmax subnormal
         DC    X'0000000000000001'  +Dmin, will appear in result
*
* We cannot generate a normal result from product of subnormals
* because the result will be smaller than both the multiplicand and the
* multiplier.  So we'll try multiplying +Dmax by 2.  The result should
* be +Nmin
*
         DC    X'000FFFFFFFFFFFFF'  +Dmax
         DC    X'4000000000000000'  +2.0, result should be normal
         DC    X'0008000000000000'  A large subnormal
*
* Multiply a value from 1.0 such that the added digits are to the right
* of the right-most bit in the stored significand. The result will be
* inexact, and incremented will be determined by the value of the
* bits in the multiplier.
*
         DC    X'3FF000000000000C'  Multiplicand +1, aka 1.0b0
         DC    X'3FF1000000000000'  Multiplier 1.0625  (1/16)
         DC    X'BFE0000000000000'  -0.5
*..nearest is away from zero, incremented.
*
         DC    X'3FF0000000000007'  Multiplicand +1, aka 1.0b0
         DC    X'3FF1000000000000'  Multiplier 1.0625  (1/16)
         DC    X'BFE0000000000000'  -0.5
*..nearest is toward zero, truncated.
*
LBFPCT   EQU   (*-LBFPIN)/8/3   Count of long BFP triples in list
         SPACE 3
***********************************************************************
*
* Third input test data set.  These are finite triples intended to
* test all combinations of rounding mode for the product and the
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
* Multiply a value from 1.0 such that the added digits are to the right
* of the right-most bit in the stored significand. The result will be
* inexact, and incremented will be determined by the value of the
* bits in the multiplier.
*
         DC    X'3FF0000000000007'  Multiplicand
         DC    X'3FF1000000000000'  Multiplier 1.0625  (1/16)
         DC    X'BFE0000000000000'  -0.5
         DC    X'BFF0000000000007'  Multiplicand
         DC    X'3FF1000000000000'  Multiplier 1.0625  (1/16)
         DC    X'3FE0000000000000'  +0.5
*..nearest is toward zero, truncated.
*
         DC    X'3FF000000000000C'  Multiplicand
         DC    X'3FF1000000000000'  Multiplier 1.0625  (1/16)
         DC    X'BFE0000000000000'  -0.5
         DC    X'BFF000000000000C'  Multiplicand
         DC    X'3FF1000000000000'  Multiplier 1.0625  (1/16)
         DC    X'3FE0000000000000'  +0.5
*..nearest is away from zero, incremented.
*
         DC    X'3FF0000000000008'  Multiplicand
         DC    X'3FF1000000000000'  Multiplier 1.0625  (1/16)
         DC    X'BFE0000000000000'  -0.5
         DC    X'BFF0000000000008'  Multiplicand
         DC    X'3FF1000000000000'  Multiplier 1.0625  (1/16)
         DC    X'3FE0000000000000'  +0.5
*..nearest is a tie, nearest even has lower magnitude
*
         DC    X'3FF0000000000018'  Multiplicand +1, aka +1.0b0
         DC    X'3FF1000000000000'  Multiplier 1.0625  (1/16)
         DC    X'BFE0000000000000'  -0.5
         DC    X'BFF0000000000018'  Multiplicand -1, aka -1.0b0
         DC    X'3FF1000000000000'  Multiplier 1.0625  (1/16)
         DC    X'3FE0000000000000'  +0.5
*..nearest is a tie, nearest even has greater magnitude
*
LBFPRMCT EQU   (*-LBFPINRM)/8/3  Count of long BFP rounding tests
         EJECT
***********************************************************************
*                 ACTUAL results saved here
***********************************************************************
*
*               Locations for ACTUAL results
*
SBFPNFOT EQU   STRTLABL+X'1000'    Short non-finite BFP results
*                                  ..room for 512 tests, 512 used
SBFPNFFL EQU   STRTLABL+X'3000'    FPCR flags and DXC from short BFP
*                                  ..room for 512 tests, 512 used
*
SBFPOUT  EQU   STRTLABL+X'5000'    Short BFP finite results
*                                  ..room for 16 tests, 7 used
SBFPFLGS EQU   STRTLABL+X'5100'    FPCR flags and DXC from short BFP
*                                  ..room for 16 tests, 7 used
*
SBFPRMO  EQU   STRTLABL+X'5200'    Short BFP rounding mode test results
*                                  ..Room for 16, 8 used.
SBFPRMOF EQU   STRTLABL+X'5500'    Short BFP rounding mode FPCR results
*                                  ..Room for 16, 8 used.
*                                  ..next location starts at X'5800'
*
LBFPNFOT EQU   STRTLABL+X'6000'    Long non-finite BFP results
*                                  ..room for 512 tests, 512 used
LBFPNFFL EQU   STRTLABL+X'A000'    FPCR flags and DXC from long BFP
*                                  ..room for 512 tests, 512 used
*
LBFPOUT  EQU   STRTLABL+X'C000'    Long BFP finite results
*                                  ..room for 16 tests, 7 used
LBFPFLGS EQU   STRTLABL+X'C200'    FPCR flags and DXC from long BFP
*                                  ..room for 16 tests, 7 used
*
LBFPRMO  EQU   STRTLABL+X'C500'    Long BFP rounding mode test results
*                                  ..Room for 16, 8 used.
LBFPRMOF EQU   STRTLABL+X'CA00'    Long BFP rounding mode FPCR results
*                                  ..Room for 16, 8 used.
*                                  ..next location starts at X'CD00'
         EJECT
***********************************************************************
*                    EXPECTED results
***********************************************************************
*
         ORG   STRTLABL+X'10000'   (far past end of actual results)
*
SBFPNFOT_GOOD EQU *                        MSEBR/MSEB NF...
 DC CL48'... -inf/-inf/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -inf/-inf/-2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -inf/-inf/-0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -inf/-inf/+0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -inf/-inf/+2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -inf/-inf/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... -inf/-inf/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -inf/-inf/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -inf/-2.0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -inf/-2.0/-2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -inf/-2.0/-0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -inf/-2.0/+0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -inf/-2.0/+2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -inf/-2.0/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... -inf/-2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -inf/-2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -inf/-0/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... -inf/-0/-2.0'
 DC XL16'7FC00000C00000007FC00000C0000000'
 DC CL48'... -inf/-0/-0'
 DC XL16'7FC00000800000007FC0000080000000'
 DC CL48'... -inf/-0/+0'
 DC XL16'7FC00000000000007FC0000000000000'
 DC CL48'... -inf/-0/+2.0'
 DC XL16'7FC00000400000007FC0000040000000'
 DC CL48'... -inf/-0/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... -inf/-0/-QNaN'
 DC XL16'7FC00000FFCB00007FC00000FFCB0000'
 DC CL48'... -inf/-0/+SNaN'
 DC XL16'7FC000007F8A00007FC000007F8A0000'
 DC CL48'... -inf/+0/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... -inf/+0/-2.0'
 DC XL16'7FC00000C00000007FC00000C0000000'
 DC CL48'... -inf/+0/-0'
 DC XL16'7FC00000800000007FC0000080000000'
 DC CL48'... -inf/+0/+0'
 DC XL16'7FC00000000000007FC0000000000000'
 DC CL48'... -inf/+0/+2.0'
 DC XL16'7FC00000400000007FC0000040000000'
 DC CL48'... -inf/+0/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... -inf/+0/-QNaN'
 DC XL16'7FC00000FFCB00007FC00000FFCB0000'
 DC CL48'... -inf/+0/+SNaN'
 DC XL16'7FC000007F8A00007FC000007F8A0000'
 DC CL48'... -inf/+2.0/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... -inf/+2.0/-2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -inf/+2.0/-0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -inf/+2.0/+0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -inf/+2.0/+2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -inf/+2.0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -inf/+2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -inf/+2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -inf/+inf/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... -inf/+inf/-2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -inf/+inf/-0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -inf/+inf/+0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -inf/+inf/+2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -inf/+inf/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -inf/+inf/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -inf/+inf/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -inf/-QNaN/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -inf/-QNaN/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -inf/-QNaN/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -inf/-QNaN/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -inf/-QNaN/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -inf/-QNaN/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -inf/-QNaN/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -inf/-QNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -inf/+SNaN/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... -inf/+SNaN/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... -inf/+SNaN/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... -inf/+SNaN/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... -inf/+SNaN/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... -inf/+SNaN/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... -inf/+SNaN/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... -inf/+SNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -2.0/-inf/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -2.0/-inf/-2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -2.0/-inf/-0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -2.0/-inf/+0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -2.0/-inf/+2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -2.0/-inf/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... -2.0/-inf/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -2.0/-inf/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -2.0/-2.0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -2.0/-2.0/-2.0'
 DC XL16'40C0000040C0000040C0000040C00000'
 DC CL48'... -2.0/-2.0/-0'
 DC XL16'40800000408000004080000040800000'
 DC CL48'... -2.0/-2.0/+0'
 DC XL16'40800000408000004080000040800000'
 DC CL48'... -2.0/-2.0/+2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... -2.0/-2.0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -2.0/-2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -2.0/-2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -2.0/-0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -2.0/-0/-2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... -2.0/-0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... -2.0/-0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... -2.0/-0/+2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... -2.0/-0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -2.0/-0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -2.0/-0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -2.0/+0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -2.0/+0/-2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... -2.0/+0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... -2.0/+0/+0'
 DC XL16'80000000800000008000000080000000'
 DC CL48'... -2.0/+0/+2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... -2.0/+0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -2.0/+0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -2.0/+0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -2.0/+2.0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -2.0/+2.0/-2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... -2.0/+2.0/-0'
 DC XL16'C0800000C0800000C0800000C0800000'
 DC CL48'... -2.0/+2.0/+0'
 DC XL16'C0800000C0800000C0800000C0800000'
 DC CL48'... -2.0/+2.0/+2.0'
 DC XL16'C0C00000C0C00000C0C00000C0C00000'
 DC CL48'... -2.0/+2.0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -2.0/+2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -2.0/+2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -2.0/+inf/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... -2.0/+inf/-2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -2.0/+inf/-0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -2.0/+inf/+0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -2.0/+inf/+2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -2.0/+inf/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -2.0/+inf/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -2.0/+inf/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -2.0/-QNaN/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -2.0/-QNaN/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -2.0/-QNaN/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -2.0/-QNaN/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -2.0/-QNaN/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -2.0/-QNaN/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -2.0/-QNaN/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -2.0/-QNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -2.0/+SNaN/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... -2.0/+SNaN/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... -2.0/+SNaN/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... -2.0/+SNaN/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... -2.0/+SNaN/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... -2.0/+SNaN/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... -2.0/+SNaN/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... -2.0/+SNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -0/-inf/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... -0/-inf/-2.0'
 DC XL16'7FC00000C00000007FC00000C0000000'
 DC CL48'... -0/-inf/-0'
 DC XL16'7FC00000800000007FC0000080000000'
 DC CL48'... -0/-inf/+0'
 DC XL16'7FC00000000000007FC0000000000000'
 DC CL48'... -0/-inf/+2.0'
 DC XL16'7FC00000400000007FC0000040000000'
 DC CL48'... -0/-inf/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... -0/-inf/-QNaN'
 DC XL16'7FC00000FFCB00007FC00000FFCB0000'
 DC CL48'... -0/-inf/+SNaN'
 DC XL16'7FC000007F8A00007FC000007F8A0000'
 DC CL48'... -0/-2.0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -0/-2.0/-2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... -0/-2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... -0/-2.0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... -0/-2.0/+2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... -0/-2.0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -0/-2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -0/-2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -0/-0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -0/-0/-2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... -0/-0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... -0/-0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... -0/-0/+2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... -0/-0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -0/-0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -0/-0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -0/+0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -0/+0/-2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... -0/+0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... -0/+0/+0'
 DC XL16'80000000800000008000000080000000'
 DC CL48'... -0/+0/+2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... -0/+0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -0/+0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -0/+0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -0/+2.0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... -0/+2.0/-2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... -0/+2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... -0/+2.0/+0'
 DC XL16'80000000800000008000000080000000'
 DC CL48'... -0/+2.0/+2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... -0/+2.0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... -0/+2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -0/+2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -0/+inf/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... -0/+inf/-2.0'
 DC XL16'7FC00000C00000007FC00000C0000000'
 DC CL48'... -0/+inf/-0'
 DC XL16'7FC00000800000007FC0000080000000'
 DC CL48'... -0/+inf/+0'
 DC XL16'7FC00000000000007FC0000000000000'
 DC CL48'... -0/+inf/+2.0'
 DC XL16'7FC00000400000007FC0000040000000'
 DC CL48'... -0/+inf/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... -0/+inf/-QNaN'
 DC XL16'7FC00000FFCB00007FC00000FFCB0000'
 DC CL48'... -0/+inf/+SNaN'
 DC XL16'7FC000007F8A00007FC000007F8A0000'
 DC CL48'... -0/-QNaN/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -0/-QNaN/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -0/-QNaN/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -0/-QNaN/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -0/-QNaN/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -0/-QNaN/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -0/-QNaN/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -0/-QNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -0/+SNaN/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... -0/+SNaN/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... -0/+SNaN/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... -0/+SNaN/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... -0/+SNaN/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... -0/+SNaN/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... -0/+SNaN/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... -0/+SNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +0/-inf/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... +0/-inf/-2.0'
 DC XL16'7FC00000C00000007FC00000C0000000'
 DC CL48'... +0/-inf/-0'
 DC XL16'7FC00000800000007FC0000080000000'
 DC CL48'... +0/-inf/+0'
 DC XL16'7FC00000000000007FC0000000000000'
 DC CL48'... +0/-inf/+2.0'
 DC XL16'7FC00000400000007FC0000040000000'
 DC CL48'... +0/-inf/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... +0/-inf/-QNaN'
 DC XL16'7FC00000FFCB00007FC00000FFCB0000'
 DC CL48'... +0/-inf/+SNaN'
 DC XL16'7FC000007F8A00007FC000007F8A0000'
 DC CL48'... +0/-2.0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +0/-2.0/-2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... +0/-2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... +0/-2.0/+0'
 DC XL16'80000000800000008000000080000000'
 DC CL48'... +0/-2.0/+2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... +0/-2.0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +0/-2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +0/-2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +0/-0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +0/-0/-2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... +0/-0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... +0/-0/+0'
 DC XL16'80000000800000008000000080000000'
 DC CL48'... +0/-0/+2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... +0/-0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +0/-0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +0/-0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +0/+0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +0/+0/-2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... +0/+0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... +0/+0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... +0/+0/+2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... +0/+0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +0/+0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +0/+0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +0/+2.0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +0/+2.0/-2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... +0/+2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... +0/+2.0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... +0/+2.0/+2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... +0/+2.0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +0/+2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +0/+2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +0/+inf/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... +0/+inf/-2.0'
 DC XL16'7FC00000C00000007FC00000C0000000'
 DC CL48'... +0/+inf/-0'
 DC XL16'7FC00000800000007FC0000080000000'
 DC CL48'... +0/+inf/+0'
 DC XL16'7FC00000000000007FC0000000000000'
 DC CL48'... +0/+inf/+2.0'
 DC XL16'7FC00000400000007FC0000040000000'
 DC CL48'... +0/+inf/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... +0/+inf/-QNaN'
 DC XL16'7FC00000FFCB00007FC00000FFCB0000'
 DC CL48'... +0/+inf/+SNaN'
 DC XL16'7FC000007F8A00007FC000007F8A0000'
 DC CL48'... +0/-QNaN/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +0/-QNaN/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +0/-QNaN/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +0/-QNaN/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +0/-QNaN/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +0/-QNaN/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +0/-QNaN/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +0/-QNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +0/+SNaN/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... +0/+SNaN/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... +0/+SNaN/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... +0/+SNaN/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... +0/+SNaN/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... +0/+SNaN/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... +0/+SNaN/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... +0/+SNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +2.0/-inf/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... +2.0/-inf/-2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +2.0/-inf/-0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +2.0/-inf/+0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +2.0/-inf/+2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +2.0/-inf/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +2.0/-inf/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +2.0/-inf/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +2.0/-2.0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +2.0/-2.0/-2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... +2.0/-2.0/-0'
 DC XL16'C0800000C0800000C0800000C0800000'
 DC CL48'... +2.0/-2.0/+0'
 DC XL16'C0800000C0800000C0800000C0800000'
 DC CL48'... +2.0/-2.0/+2.0'
 DC XL16'C0C00000C0C00000C0C00000C0C00000'
 DC CL48'... +2.0/-2.0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +2.0/-2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +2.0/-2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +2.0/-0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +2.0/-0/-2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... +2.0/-0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... +2.0/-0/+0'
 DC XL16'80000000800000008000000080000000'
 DC CL48'... +2.0/-0/+2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... +2.0/-0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +2.0/-0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +2.0/-0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +2.0/+0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +2.0/+0/-2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... +2.0/+0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... +2.0/+0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'... +2.0/+0/+2.0'
 DC XL16'C0000000C0000000C0000000C0000000'
 DC CL48'... +2.0/+0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +2.0/+0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +2.0/+0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +2.0/+2.0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +2.0/+2.0/-2.0'
 DC XL16'40C0000040C0000040C0000040C00000'
 DC CL48'... +2.0/+2.0/-0'
 DC XL16'40800000408000004080000040800000'
 DC CL48'... +2.0/+2.0/+0'
 DC XL16'40800000408000004080000040800000'
 DC CL48'... +2.0/+2.0/+2.0'
 DC XL16'40000000400000004000000040000000'
 DC CL48'... +2.0/+2.0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +2.0/+2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +2.0/+2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +2.0/+inf/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +2.0/+inf/-2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +2.0/+inf/-0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +2.0/+inf/+0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +2.0/+inf/+2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +2.0/+inf/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... +2.0/+inf/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +2.0/+inf/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +2.0/-QNaN/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +2.0/-QNaN/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +2.0/-QNaN/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +2.0/-QNaN/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +2.0/-QNaN/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +2.0/-QNaN/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +2.0/-QNaN/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +2.0/-QNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +2.0/+SNaN/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... +2.0/+SNaN/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... +2.0/+SNaN/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... +2.0/+SNaN/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... +2.0/+SNaN/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... +2.0/+SNaN/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... +2.0/+SNaN/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... +2.0/+SNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +inf/-inf/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... +inf/-inf/-2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +inf/-inf/-0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +inf/-inf/+0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +inf/-inf/+2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +inf/-inf/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +inf/-inf/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +inf/-inf/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +inf/-2.0/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... +inf/-2.0/-2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +inf/-2.0/-0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +inf/-2.0/+0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +inf/-2.0/+2.0'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +inf/-2.0/+inf'
 DC XL16'FF800000FF800000FF800000FF800000'
 DC CL48'... +inf/-2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +inf/-2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +inf/-0/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... +inf/-0/-2.0'
 DC XL16'7FC00000C00000007FC00000C0000000'
 DC CL48'... +inf/-0/-0'
 DC XL16'7FC00000800000007FC0000080000000'
 DC CL48'... +inf/-0/+0'
 DC XL16'7FC00000000000007FC0000000000000'
 DC CL48'... +inf/-0/+2.0'
 DC XL16'7FC00000400000007FC0000040000000'
 DC CL48'... +inf/-0/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... +inf/-0/-QNaN'
 DC XL16'7FC00000FFCB00007FC00000FFCB0000'
 DC CL48'... +inf/-0/+SNaN'
 DC XL16'7FC000007F8A00007FC000007F8A0000'
 DC CL48'... +inf/+0/-inf'
 DC XL16'7FC00000FF8000007FC00000FF800000'
 DC CL48'... +inf/+0/-2.0'
 DC XL16'7FC00000C00000007FC00000C0000000'
 DC CL48'... +inf/+0/-0'
 DC XL16'7FC00000800000007FC0000080000000'
 DC CL48'... +inf/+0/+0'
 DC XL16'7FC00000000000007FC0000000000000'
 DC CL48'... +inf/+0/+2.0'
 DC XL16'7FC00000400000007FC0000040000000'
 DC CL48'... +inf/+0/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... +inf/+0/-QNaN'
 DC XL16'7FC00000FFCB00007FC00000FFCB0000'
 DC CL48'... +inf/+0/+SNaN'
 DC XL16'7FC000007F8A00007FC000007F8A0000'
 DC CL48'... +inf/+2.0/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +inf/+2.0/-2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +inf/+2.0/-0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +inf/+2.0/+0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +inf/+2.0/+2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +inf/+2.0/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... +inf/+2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +inf/+2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +inf/+inf/-inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +inf/+inf/-2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +inf/+inf/-0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +inf/+inf/+0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +inf/+inf/+2.0'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'... +inf/+inf/+inf'
 DC XL16'7FC000007F8000007FC000007F800000'
 DC CL48'... +inf/+inf/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +inf/+inf/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +inf/-QNaN/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +inf/-QNaN/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +inf/-QNaN/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +inf/-QNaN/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +inf/-QNaN/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +inf/-QNaN/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +inf/-QNaN/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... +inf/-QNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +inf/+SNaN/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... +inf/+SNaN/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... +inf/+SNaN/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... +inf/+SNaN/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... +inf/+SNaN/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... +inf/+SNaN/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... +inf/+SNaN/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... +inf/+SNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -QNaN/-inf/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-inf/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-inf/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-inf/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-inf/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-inf/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-inf/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-inf/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -QNaN/-2.0/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-2.0/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-2.0/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-2.0/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-2.0/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-2.0/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -QNaN/-0/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-0/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-0/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-0/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-0/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-0/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -QNaN/+0/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+0/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+0/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+0/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+0/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+0/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -QNaN/+2.0/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+2.0/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+2.0/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+2.0/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+2.0/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+2.0/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+2.0/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -QNaN/+inf/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+inf/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+inf/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+inf/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+inf/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+inf/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+inf/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/+inf/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -QNaN/-QNaN/-inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-QNaN/-2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-QNaN/-0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-QNaN/+0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-QNaN/+2.0'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-QNaN/+inf'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-QNaN/-QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'... -QNaN/-QNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... -QNaN/+SNaN/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... -QNaN/+SNaN/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... -QNaN/+SNaN/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... -QNaN/+SNaN/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... -QNaN/+SNaN/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... -QNaN/+SNaN/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... -QNaN/+SNaN/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... -QNaN/+SNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +SNaN/-inf/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... +SNaN/-inf/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... +SNaN/-inf/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... +SNaN/-inf/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... +SNaN/-inf/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... +SNaN/-inf/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... +SNaN/-inf/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... +SNaN/-inf/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +SNaN/-2.0/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... +SNaN/-2.0/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... +SNaN/-2.0/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... +SNaN/-2.0/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... +SNaN/-2.0/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... +SNaN/-2.0/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... +SNaN/-2.0/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... +SNaN/-2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +SNaN/-0/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... +SNaN/-0/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... +SNaN/-0/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... +SNaN/-0/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... +SNaN/-0/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... +SNaN/-0/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... +SNaN/-0/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... +SNaN/-0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +SNaN/+0/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... +SNaN/+0/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... +SNaN/+0/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... +SNaN/+0/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... +SNaN/+0/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... +SNaN/+0/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... +SNaN/+0/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... +SNaN/+0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +SNaN/+2.0/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... +SNaN/+2.0/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... +SNaN/+2.0/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... +SNaN/+2.0/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... +SNaN/+2.0/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... +SNaN/+2.0/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... +SNaN/+2.0/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... +SNaN/+2.0/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +SNaN/+inf/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... +SNaN/+inf/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... +SNaN/+inf/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... +SNaN/+inf/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... +SNaN/+inf/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... +SNaN/+inf/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... +SNaN/+inf/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... +SNaN/+inf/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +SNaN/-QNaN/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... +SNaN/-QNaN/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... +SNaN/-QNaN/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... +SNaN/-QNaN/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... +SNaN/-QNaN/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... +SNaN/-QNaN/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... +SNaN/-QNaN/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... +SNaN/-QNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
 DC CL48'... +SNaN/+SNaN/-inf'
 DC XL16'7FCA0000FF8000007FCA0000FF800000'
 DC CL48'... +SNaN/+SNaN/-2.0'
 DC XL16'7FCA0000C00000007FCA0000C0000000'
 DC CL48'... +SNaN/+SNaN/-0'
 DC XL16'7FCA0000800000007FCA000080000000'
 DC CL48'... +SNaN/+SNaN/+0'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'... +SNaN/+SNaN/+2.0'
 DC XL16'7FCA0000400000007FCA000040000000'
 DC CL48'... +SNaN/+SNaN/+inf'
 DC XL16'7FCA00007F8000007FCA00007F800000'
 DC CL48'... +SNaN/+SNaN/-QNaN'
 DC XL16'7FCA0000FFCB00007FCA0000FFCB0000'
 DC CL48'... +SNaN/+SNaN/+SNaN'
 DC XL16'7FCA00007F8A00007FCA00007F8A0000'
SBFPNFOT_NUM EQU (*-SBFPNFOT_GOOD)/64
*
*
SBFPNFFL_GOOD EQU *                        MSEBR/MSEB NF...
 DC CL48'... -inf/-inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/-inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/-2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-2.0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/-0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/-0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/-0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/-0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/-0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/-0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/-0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+2.0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/+2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/+inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/+inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/+inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/+inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/+inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -inf/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -inf/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/-inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/-inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/-2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/-0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/+0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/+2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/+inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/+inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -2.0/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -2.0/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/-inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/-inf/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/-inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/-inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/-inf/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/-inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/-inf/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/-2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/-0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+inf/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+inf/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+inf/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -0/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -0/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/-inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/-inf/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/-inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/-inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/-inf/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/-inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/-inf/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/-2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/-0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+inf/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+inf/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+inf/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +0/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +0/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/-inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/-inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/-2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/-0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/+0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/+2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/+inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/+inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +2.0/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +2.0/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/-inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/-inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/-2.0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/-0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/-0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/-0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/-0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/-0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/-0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/-0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/+2.0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/+inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/+inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/+inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/+inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/+inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... +inf/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +inf/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/-inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/-2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/-0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/+0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/+2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/+inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'... -QNaN/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... -QNaN/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-inf/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-inf/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-inf/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-2.0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-2.0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-2.0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-2.0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-2.0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-2.0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-2.0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+2.0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+2.0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+2.0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+2.0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+2.0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+2.0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+2.0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+inf/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+inf/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+inf/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-QNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-QNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-QNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-QNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-QNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-QNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-QNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'... +SNaN/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
SBFPNFFL_NUM EQU (*-SBFPNFFL_GOOD)/64
*
*
SBFPOUT_GOOD EQU *
 DC CL48'MSEBR/MSEB F Ovfl 1'
 DC XL16'FF800000DF7FFFFEFF800000DF7FFFFE'
 DC CL48'MSEBR/MSEB F Ovfl 2'
 DC XL16'FF8000009FFFFFFFFF8000009FFFFFFF'
 DC CL48'MSEBR/MSEB F Ufl 1'
 DC XL16'80400001E000000280400001E0000002'
 DC CL48'MSEBR/MSEB F Ufl 2'
 DC XL16'003FFFFE5FFFFFFA003FFFFE5FFFFFFA'
 DC CL48'MSEBR/MSEB F Nmin'
 DC XL16'00BFFFFE00BFFFFE00BFFFFE00BFFFFE'
 DC CL48'MSEBR/MSEB F Incr'
 DC XL16'BFC8000DBFC8000DBFC8000DBFC8000D'
 DC CL48'MSEBR/MSEB F Trun'
 DC XL16'BFC80007BFC80007BFC80007BFC80007'
SBFPOUT_NUM EQU (*-SBFPOUT_GOOD)/64
*
*
SBFPFLGS_GOOD EQU *
 DC CL48'MSEBR/MSEB F Ovfl 1 FPCR'
 DC XL16'00280000F800280000280000F8002800'
 DC CL48'MSEBR/MSEB F Ovfl 2 FPCR'
 DC XL16'00280000F800200000280000F8002000'
 DC CL48'MSEBR/MSEB F Ufl 1 FPCR'
 DC XL16'00180000F8001C0000180000F8001C00'
 DC CL48'MSEBR/MSEB F Ufl 2 FPCR'
 DC XL16'00180000F800100000180000F8001000'
 DC CL48'MSEBR/MSEB F Nmin FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSEBR/MSEB F Incr FPCR'
 DC XL16'00080000F8000C0000080000F8000C00'
 DC CL48'MSEBR/MSEB F Trun FPCR'
 DC XL16'00080000F800080000080000F8000800'
SBFPFLGS_NUM EQU (*-SBFPFLGS_GOOD)/64
*
*
SBFPRMO_GOOD EQU *
 DC CL48'MSEBR/MSEB RM +NZ RNTE, RZ'
 DC XL16'3FC800073FC800073FC800073FC80007'
 DC CL48'MSEBR/MSEB RM +NZ RP, RM'
 DC XL16'3FC800083FC800083FC800073FC80007'
 DC CL48'MSEBR/MSEB RM +NZ RFS'
 DC XL16'3FC800073FC800070000000000000000'
 DC CL48'MSEBR/MSEB RM -NZ RNTE, RZ'
 DC XL16'BFC80007BFC80007BFC80007BFC80007'
 DC CL48'MSEBR/MSEB RM -NZ RP, RM'
 DC XL16'BFC80007BFC80007BFC80008BFC80008'
 DC CL48'MSEBR/MSEB RM -NZ RFS'
 DC XL16'BFC80007BFC800070000000000000000'
 DC CL48'MSEBR/MSEB RM +NA RNTE, RZ'
 DC XL16'3FC8000D3FC8000D3FC8000C3FC8000C'
 DC CL48'MSEBR/MSEB RM +NA RP, RM'
 DC XL16'3FC8000D3FC8000D3FC8000C3FC8000C'
 DC CL48'MSEBR/MSEB RM +NA RFS'
 DC XL16'3FC8000D3FC8000D0000000000000000'
 DC CL48'MSEBR/MSEB RM -NA RNTE, RZ'
 DC XL16'BFC8000DBFC8000DBFC8000CBFC8000C'
 DC CL48'MSEBR/MSEB RM -NA RP, RM'
 DC XL16'BFC8000CBFC8000CBFC8000DBFC8000D'
 DC CL48'MSEBR/MSEB RM -NA RFS'
 DC XL16'BFC8000DBFC8000D0000000000000000'
 DC CL48'MSEBR/MSEB RM +TZ RNTE, RZ'
 DC XL16'3FC800083FC800083FC800083FC80008'
 DC CL48'MSEBR/MSEB RM +TZ RP, RM'
 DC XL16'3FC800093FC800093FC800083FC80008'
 DC CL48'MSEBR/MSEB RM +TZ RFS'
 DC XL16'3FC800093FC800090000000000000000'
 DC CL48'MSEBR/MSEB RM -TZ RNTE, RZ'
 DC XL16'BFC80008BFC80008BFC80008BFC80008'
 DC CL48'MSEBR/MSEB RM -TZ RP, RM'
 DC XL16'BFC80008BFC80008BFC80009BFC80009'
 DC CL48'MSEBR/MSEB RM -TZ RFS'
 DC XL16'BFC80009BFC800090000000000000000'
 DC CL48'MSEBR/MSEB RM +TA RNTE, RZ'
 DC XL16'3FC8001A3FC8001A3FC800193FC80019'
 DC CL48'MSEBR/MSEB RM +TA RP, RM'
 DC XL16'3FC8001A3FC8001A3FC800193FC80019'
 DC CL48'MSEBR/MSEB RM +TA RFS'
 DC XL16'3FC800193FC800190000000000000000'
 DC CL48'MSEBR/MSEB RM -TA RNTE, RZ'
 DC XL16'BFC8001ABFC8001ABFC80019BFC80019'
 DC CL48'MSEBR/MSEB RM -TA RP, RM'
 DC XL16'BFC80019BFC80019BFC8001ABFC8001A'
 DC CL48'MSEBR/MSEB RM -TA RFS'
 DC XL16'BFC80019BFC800190000000000000000'
SBFPRMO_NUM EQU (*-SBFPRMO_GOOD)/64
*
*
SBFPRMOF_GOOD EQU *
 DC CL48'MSEBR/MSEB RM +NZ RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSEBR/MSEB RM +NZ RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSEBR/MSEB RM +NZ RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSEBR/MSEB RM -NZ RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSEBR/MSEB RM -NZ RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSEBR/MSEB RM -NZ RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSEBR/MSEB RM +NA RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSEBR/MSEB RM +NA RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSEBR/MSEB RM +NA RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSEBR/MSEB RM -NA RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSEBR/MSEB RM -NA RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSEBR/MSEB RM -NA RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSEBR/MSEB RM +TZ RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSEBR/MSEB RM +TZ RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSEBR/MSEB RM +TZ RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSEBR/MSEB RM -TZ RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSEBR/MSEB RM -TZ RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSEBR/MSEB RM -TZ RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSEBR/MSEB RM +TA RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSEBR/MSEB RM +TA RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSEBR/MSEB RM +TA RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSEBR/MSEB RM -TA RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSEBR/MSEB RM -TA RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSEBR/MSEB RM -TA RFS FPCR'
 DC XL16'00080007000800070000000000000000'
SBFPRMOF_NUM EQU (*-SBFPRMOF_GOOD)/64
*
*
LBFPNFOT_GOOD EQU *
 DC CL48'MSDBR NF -inf/-inf/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/-inf/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/-inf/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/-inf/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/-inf/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/-inf/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/-inf/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/-inf/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/-inf/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/-inf/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/-inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/-inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/-inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/-inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/-inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -inf/-inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -inf/-2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/-2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/-2.0/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/-2.0/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/-2.0/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/-2.0/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/-2.0/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/-2.0/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/-2.0/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/-2.0/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/-2.0/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/-2.0/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -inf/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -inf/-0/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/-0/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/-0/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDB NF -inf/-0/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDBR NF -inf/-0/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDB NF -inf/-0/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDBR NF -inf/-0/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDB NF -inf/-0/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDBR NF -inf/-0/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDB NF -inf/-0/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDBR NF -inf/-0/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/-0/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/-0/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/-0/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/-0/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDB NF -inf/-0/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDBR NF -inf/+0/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+0/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+0/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDB NF -inf/+0/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDBR NF -inf/+0/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDB NF -inf/+0/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDBR NF -inf/+0/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDB NF -inf/+0/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDBR NF -inf/+0/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDB NF -inf/+0/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDBR NF -inf/+0/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/+0/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/+0/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/+0/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/+0/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDB NF -inf/+0/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDBR NF -inf/+2.0/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+2.0/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+2.0/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+2.0/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+2.0/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+2.0/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+2.0/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+2.0/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+2.0/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+2.0/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -inf/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -inf/+inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+inf/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+inf/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+inf/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+inf/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+inf/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+inf/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+inf/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+inf/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+inf/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+inf/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/+inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/+inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -inf/+inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -inf/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -inf/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -inf/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF -inf/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF -inf/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF -inf/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF -inf/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF -inf/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF -inf/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF -inf/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF -inf/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF -inf/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF -inf/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF -inf/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF -inf/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF -inf/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -inf/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -inf/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -2.0/-inf/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -2.0/-inf/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -2.0/-inf/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -2.0/-inf/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -2.0/-inf/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -2.0/-inf/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -2.0/-inf/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -2.0/-inf/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -2.0/-inf/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -2.0/-inf/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -2.0/-inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF -2.0/-inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF -2.0/-inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/-inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/-inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -2.0/-inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -2.0/-2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -2.0/-2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -2.0/-2.0/-2.0'
 DC XL16'40180000000000004018000000000000'
 DC CL48'MSDB NF -2.0/-2.0/-2.0'
 DC XL16'40180000000000004018000000000000'
 DC CL48'MSDBR NF -2.0/-2.0/-0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'MSDB NF -2.0/-2.0/-0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'MSDBR NF -2.0/-2.0/+0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'MSDB NF -2.0/-2.0/+0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'MSDBR NF -2.0/-2.0/+2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF -2.0/-2.0/+2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF -2.0/-2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -2.0/-2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -2.0/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -2.0/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -2.0/-0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -2.0/-0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -2.0/-0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF -2.0/-0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF -2.0/-0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF -2.0/-0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF -2.0/-0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF -2.0/-0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF -2.0/-0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF -2.0/-0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF -2.0/-0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -2.0/-0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -2.0/-0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/-0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/-0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -2.0/-0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -2.0/+0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -2.0/+0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -2.0/+0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF -2.0/+0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF -2.0/+0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF -2.0/+0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF -2.0/+0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MSDB NF -2.0/+0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MSDBR NF -2.0/+0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF -2.0/+0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF -2.0/+0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -2.0/+0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -2.0/+0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/+0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/+0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -2.0/+0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -2.0/+2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -2.0/+2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -2.0/+2.0/-2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF -2.0/+2.0/-2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF -2.0/+2.0/-0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'MSDB NF -2.0/+2.0/-0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'MSDBR NF -2.0/+2.0/+0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'MSDB NF -2.0/+2.0/+0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'MSDBR NF -2.0/+2.0/+2.0'
 DC XL16'C018000000000000C018000000000000'
 DC CL48'MSDB NF -2.0/+2.0/+2.0'
 DC XL16'C018000000000000C018000000000000'
 DC CL48'MSDBR NF -2.0/+2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -2.0/+2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -2.0/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -2.0/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -2.0/+inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF -2.0/+inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF -2.0/+inf/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -2.0/+inf/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -2.0/+inf/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -2.0/+inf/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -2.0/+inf/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -2.0/+inf/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -2.0/+inf/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -2.0/+inf/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -2.0/+inf/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -2.0/+inf/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -2.0/+inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/+inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/+inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -2.0/+inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -2.0/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -2.0/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -2.0/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF -2.0/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF -2.0/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF -2.0/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF -2.0/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF -2.0/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF -2.0/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF -2.0/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF -2.0/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF -2.0/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF -2.0/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF -2.0/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF -2.0/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF -2.0/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -2.0/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -2.0/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -0/-inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF -0/-inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF -0/-inf/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDB NF -0/-inf/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDBR NF -0/-inf/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDB NF -0/-inf/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDBR NF -0/-inf/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDB NF -0/-inf/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDBR NF -0/-inf/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDB NF -0/-inf/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDBR NF -0/-inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF -0/-inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF -0/-inf/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/-inf/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/-inf/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDB NF -0/-inf/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDBR NF -0/-2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -0/-2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -0/-2.0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF -0/-2.0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF -0/-2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF -0/-2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF -0/-2.0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF -0/-2.0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF -0/-2.0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF -0/-2.0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF -0/-2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -0/-2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -0/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -0/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -0/-0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -0/-0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -0/-0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF -0/-0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF -0/-0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF -0/-0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF -0/-0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF -0/-0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF -0/-0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF -0/-0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF -0/-0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -0/-0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -0/-0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/-0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/-0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -0/-0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -0/+0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -0/+0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -0/+0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF -0/+0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF -0/+0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF -0/+0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF -0/+0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MSDB NF -0/+0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MSDBR NF -0/+0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF -0/+0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF -0/+0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -0/+0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -0/+0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/+0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/+0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -0/+0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -0/+2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF -0/+2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF -0/+2.0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF -0/+2.0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF -0/+2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF -0/+2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF -0/+2.0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MSDB NF -0/+2.0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MSDBR NF -0/+2.0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF -0/+2.0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF -0/+2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF -0/+2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF -0/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -0/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -0/+inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF -0/+inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF -0/+inf/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDB NF -0/+inf/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDBR NF -0/+inf/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDB NF -0/+inf/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDBR NF -0/+inf/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDB NF -0/+inf/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDBR NF -0/+inf/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDB NF -0/+inf/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDBR NF -0/+inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF -0/+inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF -0/+inf/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/+inf/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/+inf/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDB NF -0/+inf/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDBR NF -0/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -0/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -0/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF -0/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF -0/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF -0/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF -0/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF -0/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF -0/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF -0/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF -0/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF -0/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF -0/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF -0/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF -0/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF -0/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -0/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -0/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +0/-inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF +0/-inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF +0/-inf/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDB NF +0/-inf/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDBR NF +0/-inf/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDB NF +0/-inf/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDBR NF +0/-inf/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDB NF +0/-inf/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDBR NF +0/-inf/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDB NF +0/-inf/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDBR NF +0/-inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF +0/-inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF +0/-inf/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/-inf/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/-inf/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDB NF +0/-inf/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDBR NF +0/-2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +0/-2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +0/-2.0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF +0/-2.0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF +0/-2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF +0/-2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF +0/-2.0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MSDB NF +0/-2.0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MSDBR NF +0/-2.0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF +0/-2.0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF +0/-2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +0/-2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +0/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +0/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +0/-0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +0/-0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +0/-0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF +0/-0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF +0/-0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF +0/-0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF +0/-0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MSDB NF +0/-0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MSDBR NF +0/-0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF +0/-0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF +0/-0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +0/-0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +0/-0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/-0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/-0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +0/-0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +0/+0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +0/+0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +0/+0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF +0/+0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF +0/+0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF +0/+0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF +0/+0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF +0/+0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF +0/+0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF +0/+0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF +0/+0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +0/+0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +0/+0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/+0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/+0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +0/+0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +0/+2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +0/+2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +0/+2.0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF +0/+2.0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF +0/+2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF +0/+2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF +0/+2.0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF +0/+2.0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF +0/+2.0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF +0/+2.0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF +0/+2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +0/+2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +0/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +0/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +0/+inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF +0/+inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF +0/+inf/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDB NF +0/+inf/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDBR NF +0/+inf/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDB NF +0/+inf/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDBR NF +0/+inf/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDB NF +0/+inf/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDBR NF +0/+inf/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDB NF +0/+inf/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDBR NF +0/+inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF +0/+inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF +0/+inf/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/+inf/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/+inf/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDB NF +0/+inf/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDBR NF +0/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +0/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +0/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF +0/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF +0/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF +0/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF +0/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF +0/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF +0/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF +0/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF +0/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF +0/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF +0/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF +0/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF +0/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF +0/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +0/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +0/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +2.0/-inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF +2.0/-inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF +2.0/-inf/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +2.0/-inf/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +2.0/-inf/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +2.0/-inf/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +2.0/-inf/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +2.0/-inf/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +2.0/-inf/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +2.0/-inf/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +2.0/-inf/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +2.0/-inf/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +2.0/-inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/-inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/-inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +2.0/-inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +2.0/-2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +2.0/-2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +2.0/-2.0/-2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF +2.0/-2.0/-2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF +2.0/-2.0/-0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'MSDB NF +2.0/-2.0/-0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'MSDBR NF +2.0/-2.0/+0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'MSDB NF +2.0/-2.0/+0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'MSDBR NF +2.0/-2.0/+2.0'
 DC XL16'C018000000000000C018000000000000'
 DC CL48'MSDB NF +2.0/-2.0/+2.0'
 DC XL16'C018000000000000C018000000000000'
 DC CL48'MSDBR NF +2.0/-2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +2.0/-2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +2.0/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +2.0/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +2.0/-0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +2.0/-0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +2.0/-0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF +2.0/-0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF +2.0/-0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF +2.0/-0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF +2.0/-0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MSDB NF +2.0/-0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MSDBR NF +2.0/-0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF +2.0/-0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF +2.0/-0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +2.0/-0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +2.0/-0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/-0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/-0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +2.0/-0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +2.0/+0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +2.0/+0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +2.0/+0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF +2.0/+0/-2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF +2.0/+0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF +2.0/+0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF +2.0/+0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDB NF +2.0/+0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MSDBR NF +2.0/+0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDB NF +2.0/+0/+2.0'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'MSDBR NF +2.0/+0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +2.0/+0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +2.0/+0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/+0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/+0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +2.0/+0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +2.0/+2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +2.0/+2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +2.0/+2.0/-2.0'
 DC XL16'40180000000000004018000000000000'
 DC CL48'MSDB NF +2.0/+2.0/-2.0'
 DC XL16'40180000000000004018000000000000'
 DC CL48'MSDBR NF +2.0/+2.0/-0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'MSDB NF +2.0/+2.0/-0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'MSDBR NF +2.0/+2.0/+0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'MSDB NF +2.0/+2.0/+0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'MSDBR NF +2.0/+2.0/+2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDB NF +2.0/+2.0/+2.0'
 DC XL16'40000000000000004000000000000000'
 DC CL48'MSDBR NF +2.0/+2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +2.0/+2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +2.0/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +2.0/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +2.0/+inf/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +2.0/+inf/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +2.0/+inf/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +2.0/+inf/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +2.0/+inf/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +2.0/+inf/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +2.0/+inf/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +2.0/+inf/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +2.0/+inf/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +2.0/+inf/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +2.0/+inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF +2.0/+inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF +2.0/+inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/+inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/+inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +2.0/+inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +2.0/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +2.0/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +2.0/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF +2.0/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF +2.0/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF +2.0/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF +2.0/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF +2.0/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF +2.0/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF +2.0/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF +2.0/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF +2.0/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF +2.0/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF +2.0/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF +2.0/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF +2.0/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +2.0/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +2.0/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +inf/-inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/-inf/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/-inf/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/-inf/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/-inf/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/-inf/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/-inf/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/-inf/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/-inf/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/-inf/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/-inf/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/-inf/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/-inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/-inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/-inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +inf/-inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +inf/-2.0/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/-2.0/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/-2.0/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/-2.0/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/-2.0/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/-2.0/-0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/-2.0/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/-2.0/+0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/-2.0/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/-2.0/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/-2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/-2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +inf/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +inf/-0/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/-0/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/-0/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDB NF +inf/-0/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDBR NF +inf/-0/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDB NF +inf/-0/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDBR NF +inf/-0/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDB NF +inf/-0/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDBR NF +inf/-0/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDB NF +inf/-0/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDBR NF +inf/-0/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/-0/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/-0/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/-0/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/-0/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDB NF +inf/-0/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDBR NF +inf/+0/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/+0/-inf'
 DC XL16'7FF8000000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/+0/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDB NF +inf/+0/-2.0'
 DC XL16'7FF8000000000000C000000000000000'
 DC CL48'MSDBR NF +inf/+0/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDB NF +inf/+0/-0'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MSDBR NF +inf/+0/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDB NF +inf/+0/+0'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MSDBR NF +inf/+0/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDB NF +inf/+0/+2.0'
 DC XL16'7FF80000000000004000000000000000'
 DC CL48'MSDBR NF +inf/+0/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+0/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+0/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/+0/-QNaN'
 DC XL16'7FF8000000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/+0/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDB NF +inf/+0/+SNaN'
 DC XL16'7FF80000000000007FF0A00000000000'
 DC CL48'MSDBR NF +inf/+2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+2.0/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+2.0/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+2.0/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+2.0/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+2.0/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+2.0/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+2.0/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+2.0/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+2.0/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+2.0/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +inf/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +inf/+inf/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+inf/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+inf/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+inf/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+inf/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+inf/-0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+inf/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+inf/+0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+inf/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+inf/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+inf/+inf'
 DC XL16'7FF80000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/+inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/+inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +inf/+inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +inf/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +inf/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +inf/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF +inf/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF +inf/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF +inf/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF +inf/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF +inf/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF +inf/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF +inf/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF +inf/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF +inf/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF +inf/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF +inf/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF +inf/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF +inf/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +inf/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +inf/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -QNaN/-inf/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-inf/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-inf/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-inf/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-inf/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-inf/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-inf/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-inf/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-inf/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-inf/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-inf/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-inf/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -QNaN/-inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -QNaN/-2.0/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-2.0/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-2.0/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-2.0/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-2.0/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-2.0/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-2.0/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-2.0/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-2.0/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-2.0/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-2.0/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-2.0/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -QNaN/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -QNaN/-0/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-0/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-0/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-0/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-0/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-0/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-0/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-0/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-0/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-0/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-0/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-0/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -QNaN/-0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -QNaN/+0/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+0/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+0/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+0/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+0/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+0/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+0/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+0/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+0/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+0/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+0/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+0/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -QNaN/+0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -QNaN/+2.0/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+2.0/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+2.0/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+2.0/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+2.0/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+2.0/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+2.0/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+2.0/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+2.0/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+2.0/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+2.0/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+2.0/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+2.0/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -QNaN/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -QNaN/+inf/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+inf/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+inf/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+inf/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+inf/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+inf/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+inf/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+inf/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+inf/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+inf/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+inf/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+inf/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+inf/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -QNaN/+inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -QNaN/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-QNaN/-inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-QNaN/-2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-QNaN/-0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-QNaN/+0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-QNaN/+2.0'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-QNaN/+inf'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/-QNaN/-QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -QNaN/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF -QNaN/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF -QNaN/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF -QNaN/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF -QNaN/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF -QNaN/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF -QNaN/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF -QNaN/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF -QNaN/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF -QNaN/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF -QNaN/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF -QNaN/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF -QNaN/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF -QNaN/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF -QNaN/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF -QNaN/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF -QNaN/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +SNaN/-inf/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF +SNaN/-inf/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF +SNaN/-inf/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF +SNaN/-inf/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF +SNaN/-inf/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF +SNaN/-inf/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF +SNaN/-inf/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF +SNaN/-inf/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF +SNaN/-inf/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF +SNaN/-inf/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF +SNaN/-inf/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF +SNaN/-inf/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF +SNaN/-inf/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF +SNaN/-inf/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +SNaN/-inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +SNaN/-inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +SNaN/-2.0/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF +SNaN/-2.0/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF +SNaN/-2.0/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF +SNaN/-2.0/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF +SNaN/-2.0/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF +SNaN/-2.0/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF +SNaN/-2.0/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF +SNaN/-2.0/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF +SNaN/-2.0/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF +SNaN/-2.0/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF +SNaN/-2.0/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF +SNaN/-2.0/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF +SNaN/-2.0/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF +SNaN/-2.0/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +SNaN/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +SNaN/-2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +SNaN/-0/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF +SNaN/-0/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF +SNaN/-0/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF +SNaN/-0/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF +SNaN/-0/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF +SNaN/-0/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF +SNaN/-0/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF +SNaN/-0/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF +SNaN/-0/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF +SNaN/-0/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF +SNaN/-0/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF +SNaN/-0/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF +SNaN/-0/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF +SNaN/-0/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +SNaN/-0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +SNaN/-0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +SNaN/+0/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF +SNaN/+0/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF +SNaN/+0/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF +SNaN/+0/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF +SNaN/+0/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF +SNaN/+0/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF +SNaN/+0/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF +SNaN/+0/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF +SNaN/+0/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF +SNaN/+0/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF +SNaN/+0/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF +SNaN/+0/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF +SNaN/+0/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF +SNaN/+0/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +SNaN/+0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +SNaN/+0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +SNaN/+2.0/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF +SNaN/+2.0/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF +SNaN/+2.0/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF +SNaN/+2.0/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF +SNaN/+2.0/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF +SNaN/+2.0/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF +SNaN/+2.0/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF +SNaN/+2.0/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF +SNaN/+2.0/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF +SNaN/+2.0/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF +SNaN/+2.0/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF +SNaN/+2.0/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF +SNaN/+2.0/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF +SNaN/+2.0/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +SNaN/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +SNaN/+2.0/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +SNaN/+inf/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF +SNaN/+inf/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF +SNaN/+inf/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF +SNaN/+inf/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF +SNaN/+inf/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF +SNaN/+inf/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF +SNaN/+inf/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF +SNaN/+inf/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF +SNaN/+inf/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF +SNaN/+inf/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF +SNaN/+inf/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF +SNaN/+inf/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF +SNaN/+inf/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF +SNaN/+inf/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +SNaN/+inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +SNaN/+inf/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +SNaN/-QNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF +SNaN/-QNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF +SNaN/-QNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF +SNaN/-QNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF +SNaN/-QNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF +SNaN/-QNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF +SNaN/-QNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF +SNaN/-QNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF +SNaN/-QNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF +SNaN/-QNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF +SNaN/-QNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF +SNaN/-QNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF +SNaN/-QNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF +SNaN/-QNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +SNaN/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +SNaN/-QNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDBR NF +SNaN/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDB NF +SNaN/+SNaN/-inf'
 DC XL16'7FF8A00000000000FFF0000000000000'
 DC CL48'MSDBR NF +SNaN/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDB NF +SNaN/+SNaN/-2.0'
 DC XL16'7FF8A00000000000C000000000000000'
 DC CL48'MSDBR NF +SNaN/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDB NF +SNaN/+SNaN/-0'
 DC XL16'7FF8A000000000008000000000000000'
 DC CL48'MSDBR NF +SNaN/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDB NF +SNaN/+SNaN/+0'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'MSDBR NF +SNaN/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDB NF +SNaN/+SNaN/+2.0'
 DC XL16'7FF8A000000000004000000000000000'
 DC CL48'MSDBR NF +SNaN/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDB NF +SNaN/+SNaN/+inf'
 DC XL16'7FF8A000000000007FF0000000000000'
 DC CL48'MSDBR NF +SNaN/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDB NF +SNaN/+SNaN/-QNaN'
 DC XL16'7FF8A00000000000FFF8B00000000000'
 DC CL48'MSDBR NF +SNaN/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
 DC CL48'MSDB NF +SNaN/+SNaN/+SNaN'
 DC XL16'7FF8A000000000007FF0A00000000000'
LBFPNFOT_NUM EQU (*-LBFPNFOT_GOOD)/64
*
*
LBFPNFFL_GOOD EQU *
 DC CL48'MSDBR/MSDB NF -inf/-inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/-inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/-2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-2.0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/-0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/-0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/-0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/-0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/-0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/-0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/-0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+2.0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/+2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/+inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/+inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/+inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/+inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/+inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -inf/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -inf/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/-inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/-inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/-2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/-0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/+0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/+2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/+inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/+inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -2.0/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -2.0/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/-inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/-inf/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/-inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/-inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/-inf/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/-inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/-inf/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/-2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/-0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+inf/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+inf/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+inf/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -0/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -0/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/-inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/-inf/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/-inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/-inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/-inf/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/-inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/-inf/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/-2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/-0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+inf/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+inf/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+inf/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +0/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +0/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/-inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/-inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/-2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/-0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/+0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/+2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/+inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/+inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +2.0/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +2.0/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/-inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/-inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/-2.0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/-0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/-0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/-0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/-0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/-0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/-0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/-0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/+2.0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/+inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/+inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/+inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/+inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/+inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF +inf/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +inf/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/-inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/-2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/-0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/+0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/+2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/+inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+inf/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+inf/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/-QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB NF -QNaN/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF -QNaN/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-inf/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-inf/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-inf/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-2.0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-2.0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-2.0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-2.0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-2.0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-2.0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-2.0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+2.0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+2.0/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+2.0/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+2.0/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+2.0/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+2.0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+2.0/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+inf/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+inf/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+inf/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+inf/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+inf/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-QNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-QNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-QNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-QNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-QNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-QNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-QNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/-QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MSDBR/MSDB NF +SNaN/+SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
LBFPNFFL_NUM EQU (*-LBFPNFFL_GOOD)/64
*
*
LBFPOUT_GOOD EQU *
 DC CL48'MSDBR F Ovfl 1'
 DC XL16'FFF0000000000000DFEFFFFFFFFFFFFE'
 DC CL48'MSDB F Ovfl 1'
 DC XL16'FFF0000000000000DFEFFFFFFFFFFFFE'
 DC CL48'MSDBR F Ovfl 2'
 DC XL16'FFF00000000000009FFFFFFFFFFFFFFF'
 DC CL48'MSDB F Ovfl 2'
 DC XL16'FFF00000000000009FFFFFFFFFFFFFFF'
 DC CL48'MSDBR F Ufl 1'
 DC XL16'8008000000000001E000000000000002'
 DC CL48'MSDB F Ufl 1'
 DC XL16'8008000000000001E000000000000002'
 DC CL48'MSDBR F Ufl 2'
 DC XL16'0007FFFFFFFFFFFE5FFFFFFFFFFFFFFA'
 DC CL48'MSDB F Ufl 2'
 DC XL16'0007FFFFFFFFFFFE5FFFFFFFFFFFFFFA'
 DC CL48'MSDBR F Nmin'
 DC XL16'0017FFFFFFFFFFFE0017FFFFFFFFFFFE'
 DC CL48'MSDB F Nmin'
 DC XL16'0017FFFFFFFFFFFE0017FFFFFFFFFFFE'
 DC CL48'MSDBR F Incr'
 DC XL16'3FF900000000000D3FF900000000000D'
 DC CL48'MSDB F Incr'
 DC XL16'3FF900000000000D3FF900000000000D'
 DC CL48'MSDBR F Trun'
 DC XL16'3FF90000000000073FF9000000000007'
 DC CL48'MSDB F Trun'
 DC XL16'3FF90000000000073FF9000000000007'
LBFPOUT_NUM EQU (*-LBFPOUT_GOOD)/64
*
*
LBFPFLGS_GOOD EQU *
 DC CL48'MSDBR/MSDB F Ovfl 1 FPCR'
 DC XL16'00280000F800280000280000F8002800'
 DC CL48'MSDBR/MSDB F Ovfl 2 FPCR'
 DC XL16'00280000F800200000280000F8002000'
 DC CL48'MSDBR/MSDB F Ufl 1 FPCR'
 DC XL16'00180000F8001C0000180000F8001C00'
 DC CL48'MSDBR/MSDB F Ufl 2 FPCR'
 DC XL16'00180000F800100000180000F8001000'
 DC CL48'MSDBR/MSDB F Nmin FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MSDBR/MSDB F Incr FPCR'
 DC XL16'00080000F8000C0000080000F8000C00'
 DC CL48'MSDBR/MSDB F Trun FPCR'
 DC XL16'00080000F800080000080000F8000800'
LBFPFLGS_NUM EQU (*-LBFPFLGS_GOOD)/64
*
*
LBFPRMO_GOOD EQU *
 DC CL48'MSDBR/MSDB RM +NZ RNTE'
 DC XL16'3FF90000000000073FF9000000000007'
 DC CL48'MSDBR/MSDB RM +NZ RZ'
 DC XL16'3FF90000000000073FF9000000000007'
 DC CL48'MSDBR/MSDB RM +NZ RP'
 DC XL16'3FF90000000000083FF9000000000008'
 DC CL48'MSDBR/MSDB RM +NZ RM'
 DC XL16'3FF90000000000073FF9000000000007'
 DC CL48'MSDBR/MSDB RM +NZ RFS'
 DC XL16'3FF90000000000073FF9000000000007'
 DC CL48'MSDBR/MSDB RM -NZ RNTE'
 DC XL16'BFF9000000000007BFF9000000000007'
 DC CL48'MSDBR/MSDB RM -NZ RZ'
 DC XL16'BFF9000000000007BFF9000000000007'
 DC CL48'MSDBR/MSDB RM -NZ RP'
 DC XL16'BFF9000000000007BFF9000000000007'
 DC CL48'MSDBR/MSDB RM -NZ RM'
 DC XL16'BFF9000000000008BFF9000000000008'
 DC CL48'MSDBR/MSDB RM -NZ RFS'
 DC XL16'BFF9000000000007BFF9000000000007'
 DC CL48'MSDBR/MSDB RM +NA RNTE'
 DC XL16'3FF900000000000D3FF900000000000D'
 DC CL48'MSDBR/MSDB RM +NA RZ'
 DC XL16'3FF900000000000C3FF900000000000C'
 DC CL48'MSDBR/MSDB RM +NA RP'
 DC XL16'3FF900000000000D3FF900000000000D'
 DC CL48'MSDBR/MSDB RM +NA RM'
 DC XL16'3FF900000000000C3FF900000000000C'
 DC CL48'MSDBR/MSDB RM +NA RFS'
 DC XL16'3FF900000000000D3FF900000000000D'
 DC CL48'MSDBR/MSDB RM -NA RNTE'
 DC XL16'BFF900000000000DBFF900000000000D'
 DC CL48'MSDBR/MSDB RM -NA RZ'
 DC XL16'BFF900000000000CBFF900000000000C'
 DC CL48'MSDBR/MSDB RM -NA RP'
 DC XL16'BFF900000000000CBFF900000000000C'
 DC CL48'MSDBR/MSDB RM -NA RM'
 DC XL16'BFF900000000000DBFF900000000000D'
 DC CL48'MSDBR/MSDB RM -NA RFS'
 DC XL16'BFF900000000000DBFF900000000000D'
 DC CL48'MSDBR/MSDB RM +TZ RNTE'
 DC XL16'3FF90000000000083FF9000000000008'
 DC CL48'MSDBR/MSDB RM +TZ RZ'
 DC XL16'3FF90000000000083FF9000000000008'
 DC CL48'MSDBR/MSDB RM +TZ RP'
 DC XL16'3FF90000000000093FF9000000000009'
 DC CL48'MSDBR/MSDB RM +TZ RM'
 DC XL16'3FF90000000000083FF9000000000008'
 DC CL48'MSDBR/MSDB RM +TZ RFS'
 DC XL16'3FF90000000000093FF9000000000009'
 DC CL48'MSDBR/MSDB RM -TZ RNTE'
 DC XL16'BFF9000000000008BFF9000000000008'
 DC CL48'MSDBR/MSDB RM -TZ RZ'
 DC XL16'BFF9000000000008BFF9000000000008'
 DC CL48'MSDBR/MSDB RM -TZ RP'
 DC XL16'BFF9000000000008BFF9000000000008'
 DC CL48'MSDBR/MSDB RM -TZ RM'
 DC XL16'BFF9000000000009BFF9000000000009'
 DC CL48'MSDBR/MSDB RM -TZ RFS'
 DC XL16'BFF9000000000009BFF9000000000009'
 DC CL48'MSDBR/MSDB RM +TA RNTE'
 DC XL16'3FF900000000001A3FF900000000001A'
 DC CL48'MSDBR/MSDB RM +TA RZ'
 DC XL16'3FF90000000000193FF9000000000019'
 DC CL48'MSDBR/MSDB RM +TA RP'
 DC XL16'3FF900000000001A3FF900000000001A'
 DC CL48'MSDBR/MSDB RM +TA RM'
 DC XL16'3FF90000000000193FF9000000000019'
 DC CL48'MSDBR/MSDB RM +TA RFS'
 DC XL16'3FF90000000000193FF9000000000019'
 DC CL48'MSDBR/MSDB RM -TA RNTE'
 DC XL16'BFF900000000001ABFF900000000001A'
 DC CL48'MSDBR/MSDB RM -TA RZ'
 DC XL16'BFF9000000000019BFF9000000000019'
 DC CL48'MSDBR/MSDB RM -TA RP'
 DC XL16'BFF9000000000019BFF9000000000019'
 DC CL48'MSDBR/MSDB RM -TA RM'
 DC XL16'BFF900000000001ABFF900000000001A'
 DC CL48'MSDBR/MSDB RM -TA RFS'
 DC XL16'BFF9000000000019BFF9000000000019'
LBFPRMO_NUM EQU (*-LBFPRMO_GOOD)/64
*
*
LBFPRMOF_GOOD EQU *
 DC CL48'MSDBR/MSDB RM +NZ RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSDBR/MSDB RM +NZ RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSDBR/MSDB RM +NZ RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSDBR/MSDB RM -NZ RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSDBR/MSDB RM -NZ RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSDBR/MSDB RM -NZ RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSDBR/MSDB RM +NA RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSDBR/MSDB RM +NA RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSDBR/MSDB RM +NA RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSDBR/MSDB RM -NA RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSDBR/MSDB RM -NA RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSDBR/MSDB RM -NA RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSDBR/MSDB RM +TZ RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSDBR/MSDB RM +TZ RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSDBR/MSDB RM +TZ RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSDBR/MSDB RM -TZ RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSDBR/MSDB RM -TZ RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSDBR/MSDB RM -TZ RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSDBR/MSDB RM +TA RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSDBR/MSDB RM +TA RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSDBR/MSDB RM +TA RFS FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'MSDBR/MSDB RM -TA RNTE, RZ FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'MSDBR/MSDB RM -TA RP, RM FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'MSDBR/MSDB RM -TA RFS FPCR'
 DC XL16'00080007000800070000000000000000'
LBFPRMOF_NUM EQU (*-LBFPRMOF_GOOD)/64
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
VERIFLEN EQU   (*-VERIFTAB)/12    #of entries in verify table
                                                                EJECT
         END
