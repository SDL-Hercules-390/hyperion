  TITLE 'bfp-015-sqrt: Test IEEE Square Root'
***********************************************************************
*
*Testcase IEEE SQUARE ROOT
*  Test case capability includes IEEE exceptions trappable and
*  otherwise. Test results, FPCR flags, and any DXC are saved for all
*  tests.
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
*                         bfp-015-sqrt.asm
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
*
* Tests five square root instructions:
*   SQUARE ROOT (extended BFP, RRE)
*   SQUARE ROOT (long BFP, RRE)
*   SQUARE ROOT (long BFP, RXE)
*   SQUARE ROOT (short BFP, RRE)
*   SQUARE ROOT (short BFP, RXE)
*
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R
* commands.
*
* Test Case Order
* 1) Short BFP basic tests, including traps and NaN propagation
* 2) Short BFP FPC-controlled rounding mode exhaustive tests
* 3) Long BFP basic tests, including traps and NaN propagation
* 4) Long BFP FPC-controlled rounding mode exhaustive tests
* 5) Extended BFP basic tests, including traps and NaN propagation
* 6) Extended BFP FPC-controlled rounding mode exhaustive tests
*
* Two input test sets are provided each for short, long, and extended
* BFP inputs.  The first set covers non-finites, negatives, NaNs, and
* traps on inexact and inexact-incremented.  The second more limited
* set exhaustively tests Square Root with each rounding mode that can
* be specified in the FPC.  Test values are the same for each
* precision.  Interestingly, the square root of 3 is nearer to a lower
* magnitude for all three precisions, while the square root of 5 is
* nearer to a larger magnitude for all three precisions.  The square
* root of 2 does not have this property.
*
* Note: Square Root recognizes only the IEEE exceptions Invalid and
* Inexact.  Neither overflow nor underflow can occur with Square Root.
* For values greater than 1, the result from Square Root is smaller
* than the input.  For values less than 1 and greater than zero, the
* result from square root is larger than the input value.
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
*
***********************************************************************
         EJECT
*
*  Note: for compatibility with the z/CMS test rig, do not change
*  or use R11, R14, or R15.  Everything else is fair game.
*
BFPSQRTS START 0
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
R9       EQU   9                   Rounding tests top of inner loop
R10      EQU   10                  Pointer to test address list
R11      EQU   11                  **Reserved for z/CMS test rig
R12      EQU   12                  Top of outer loop address in tests
R13      EQU   13                  Mainline return address
R14      EQU   14                  **Return address for z/CMS test rig
R15      EQU   15                  **Base register on z/CMS or Hyperion
*
* Floating Point Register equates to keep the cross reference clean
*
FPR0     EQU   0
FPR1     EQU   1             Input value for RRE instructions
FPR2     EQU   2
FPR3     EQU   3             ..Paired with FPR1 for RRE extended
FPR4     EQU   4
FPR5     EQU   5
FPR6     EQU   6
FPR7     EQU   7
FPR8     EQU   8             Result value from Square Root
FPR9     EQU   9
FPR10    EQU   10            ..Paired with FPR8 for RRE extended
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
         LA    R10,SHORTB    Point to short BFP inputs
         BAS   R13,SBFPB     Take square root of short BFP values
         LA    R10,RMSHORTS  Point to short BFP rounding mode tests
         BAS   R13,SBFPRM    Take sqrt of short BFP for rounding tests
*
         LA    R10,LONGB     Point to long BFP inputs
         BAS   R13,LBFPB     Take square root of long BFP values
         LA    R10,RMLONGS   Point to long  BFP rounding mode tests
         BAS   R13,LBFPRM    Take sqrt of long BFP for rounding tests
*
         LA    R10,XTNDB     Point to extended BFP inputs
         BAS   R13,XBFPB    Take square root of extended BFP values
         LA    R10,RMXTNDS   Point to ext'd BFP rounding mode tests
         BAS   R13,XBFPRM    Take sqrt of ext'd BFP for rounding tests
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
SHORTB   DS    0F           Input pairs for short BFP basic tests
         DC    A(SBFPBCT)
         DC    A(SBFPBIN)
         DC    A(SBFPBOT)
         DC    A(SBFPBFL)
*
RMSHORTS DS    0F           Input pairs for short BFP rounding testing
         DC    A(SBFPRMCT)
         DC    A(SBFPINRM)
         DC    A(SBFPRMO)
         DC    A(SBFPRMOF)
*
LONGB    DS    0F           Input pairs for long BFP basic testing
         DC    A(LBFPBCT)
         DC    A(LBFPBIN)
         DC    A(LBFPBOT)
         DC    A(LBFPBFL)
*
RMLONGS  DS    0F           Input pairs for long BFP rounding testing
         DC    A(LBFPRMCT)
         DC    A(LBFPINRM)
         DC    A(LBFPRMO)
         DC    A(LBFPRMOF)
*
XTNDB    DS    0F           Inputs for extended BFP basic testing
         DC    A(XBFPBCT)
         DC    A(XBFPBIN)
         DC    A(XBFPBOT)
         DC    A(XBFPBFL)
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
* Take square roots using provided short BFP inputs.  This set of tests
* checks NaN propagation, operations on values that are not finite
* numbers, and other basic tests.  This set generates results that can
* be validated against Figure 19-17 on page 19-21 of SA22-7832-10.
*
* Four results are generated for each input: one RRE with all
* exceptions non-trappable, a second RRE with all exceptions trappable,
* a third RXE with all exceptions non-trappable, a fourth RXE with all
* exceptions trappable,
*
* The square root and FPCR are stored for each result.
*
***********************************************************************
         SPACE 2
SBFPB    DS    0H            BFP Short basic tests
         LM    R2,R3,0(R10)  Get count and address of dividendd values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LZER  FPR8          Zero result register
         LE    FPR1,0(,R3)   Get short BFP input
         LFPC  FPCREGNT      Set exceptions non-trappable
         SQEBR FPR8,FPR1     Take square root of FPR1 into FPR8 RRE
         STE   FPR8,0(,R7)   Store short BFP square root
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LZER  FPR8          Zero result register
         LFPC  FPCREGTR      Set exceptions trappable
         SQEBR FPR8,FPR1     Take square root of FPR1 into FPR8 RRE
         STE   FPR8,4(,R7)   Store short BFP square root
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LZER  FPR8          Zero result register
         LFPC  FPCREGNT      Set exceptions non-trappable
         SQEB  FPR8,0(,R3)   Take square root, place in FPR8 RXE
         STE   FPR8,8(,R7)   Store short BFP square root
         STFPC 8(R8)         Store resulting FPCR flags and DXC
*
         LZER  FPR8          Zero result register
         LFPC  FPCREGTR      Set exceptions trappable
         SQEB  FPR8,0(,R3)   Take square root, place in FPR8 RXE
         STE   FPR8,12(,R7)  Store short BFP square root
         STFPC 12(R8)        Store resulting FPCR flags and DXC
*
         LA    R7,16(,R7)    Point to next Square Root result area
         LA    R8,16(,R8)    Point to next Square Root FPCR area
         LA    R3,4(,R3)     Point to next input value
         BCTR  R2,R12        Convert next input value.
*
         BR    R13           All converted; return.
*
         EJECT
***********************************************************************
*
* Perform Square Root using provided short BFP inputs.  This set of
* tests exhaustively tests all rounding modes available for Square
* Root. The rounding mode can only be specified in the FPC.
*
* All five FPC rounding modes are tested because the preceeding tests,
* using rounding mode RNTE, do not often create results that require
* rounding.
*
* Two results are generated for each input and rounding mode: one RRE
* and one RXE.  Traps are disabled for all rounding mode tests.
*
* The quotient and FPCR contents are stored for each test.
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
         LE    FPR1,0(,R3)   Get short BFP input value
         SQEBR FPR8,FPR1     Take square root of FPR1 into FPR8 RRE
         STE   FPR8,0(,R7)   Store short BFP quotient
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 0(R1)         Set FPC Rounding Mode
         SQEB  FPR8,0(,R3)   Take square root of value into FPR8 RXE
         STE   FPR8,4(,R7)   Store short BFP quotient
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LA    R7,8(,R7)     Point to next square root result
         LA    R8,8(,R8)     Point to next FPCR result area
*
         BCTR  R5,R9         Iterate to next FPC mode
*
* End of FPC modes to be tested.  Advance to next test case.
*
         LA    R3,4(,R3)     Point to next input value
         LA    R7,8(,R7)     Skip to start of next result area
         LA    R8,8(,R8)     Skip to start of next FPCR result area
         BCTR  R2,R12        Divide next input value lots of times
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Take square roots using provided long BFP inputs.  This set of tests
* checks NaN propagation, operations on values that are not finite
* numbers, and other basic tests.  This set generates results that can
* be validated against Figure 19-17 on page 19-21 of SA22-7832-10.
*
* Four results are generated for each input: one RRE with all
* exceptions non-trappable, a second RRE with all exceptions trappable,
* a third RXE with all exceptions non-trappable, a fourth RXE with all
* exceptions trappable,
*
* The square root and FPCR are stored for each result.
*
***********************************************************************
         SPACE 2
LBFPB    DS    0H            BFP long basic tests
         LM    R2,R3,0(R10)  Get count and address of dividendd values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LZDR  FPR8          Zero result register
         LD    FPR1,0(,R3)   Get long BFP input
         LFPC  FPCREGNT      Set exceptions non-trappable
         SQDBR FPR8,FPR1     Take square root of FPR1 into FPR8 RRE
         STD   FPR8,0(,R7)   Store long BFP square root
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LZDR  FPR8          Zero result register
         LFPC  FPCREGTR      Set exceptions trappable
         SQDBR FPR8,FPR1     Take square root of FPR1 into FPR8 RRE
         STD   FPR8,8(,R7)   Store long BFP square root
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LZDR  FPR8          Zero result register
         LFPC  FPCREGNT      Set exceptions non-trappable
         SQDB  FPR8,0(,R3)   Take square root, place in FPR8 RXE
         STD   FPR8,16(,R7)  Store long BFP square root
         STFPC 8(R8)         Store resulting FPCR flags and DXC
*
         LZDR  FPR8          Zero result register
         LFPC  FPCREGTR      Set exceptions trappable
         SQDB  FPR8,0(,R3)   Take square root, place in FPR8 RXE
         STD   FPR8,24(,R7)  Store short BFP square root
         STFPC 12(R8)        Store resulting FPCR flags and DXC
*
         LA    R7,32(,R7)    Point to next Square Root result area
         LA    R8,16(,R8)    Point to next Square Root FPCR area
         LA    R3,8(,R3)     Point to next input value
         BCTR  R2,R12        Convert next input value.
*
         BR    R13           All converted; return.
*
         EJECT
***********************************************************************
*
* Perform Square Root using provided long BFP inputs.  This set of
* tests exhaustively tests all rounding modes available for Square
* Root. The rounding mode can only be specified in the FPC.
*
* All five FPC rounding modes are tested because the preceeding tests,
* using rounding mode RNTE, do not often create results that require
* rounding.
*
* Two results are generated for each input and rounding mode: one RRE
* and one RXE.  Traps are disabled for all rounding mode tests.
*
* The quotient and FPCR contents are stored for each test.
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
         BASR  R9,0          Set top of rounding mode outer loop
*
         IC    R1,FPCMODES-L'FPCMODES(R5)  Get next FPC mode
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 0(R1)         Set FPC Rounding Mode
         LD    FPR1,0(,R3)   Get long BFP input value
         SQDBR FPR8,FPR1     Take square root of FPR1 into FPR8 RRE
         STD   FPR8,0(,R7)   Store long BFP quotient
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 0(R1)         Set FPC Rounding Mode
         SQDB  FPR8,0(,R3)   Take square root of value into FPR8 RXE
         STD   FPR8,8(,R7)   Store short BFP quotient
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LA    R7,16(,R7)    Point to next square root result
         LA    R8,8(,R8)     Point to next FPCR result area
*
         BCTR  R5,R9         Iterate to next FPC mode
*
* End of FPC modes to be tested.  Advance to next test case.
*
         LA    R3,8(,R3)     Point to next input value
         LA    R8,8(,R8)     Skip to start of next FPCR result area
         BCTR  R2,R12        Divide next input value lots of times
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Take square roots using provided extended BFP inputs.  This set of
* tests checks NaN propagation, operations on values that are not
* finite numbers, and other basic tests.  This set generates results
* that can be validated against Figure 19-17 on page 19-21 of
*  SA22-7832-10.
*
* Four results are generated for each input: one RRE with all
* exceptions non-trappable, a second RRE with all exceptions trappable,
* a third RXE with all exceptions non-trappable, a fourth RXE with all
* exceptions trappable,
*
* The square root and FPCR are stored for each result.
*
***********************************************************************
         SPACE 2
XBFPB    DS    0H            BFP extended basic tests
         LM    R2,R3,0(R10)  Get count and address of dividendd values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LZXR  FPR8          Zero result register
         LD    FPR1,0(,R3)   Get extended BFP input part 1
         LD    FPR3,8(,R3)   Get extended BFP input part 2
         LFPC  FPCREGNT      Set exceptions non-trappable
         SQXBR FPR8,FPR1     Take square root of FPR1 into FPR8-10 RRE
         STD   FPR8,0(,R7)   Store extended BFP square root part 1
         STD   FPR10,8(,R7)  Store extended BFP square root part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LZXR  FPR8          Zero result register
         LFPC  FPCREGTR      Set exceptions trappable
         SQXBR FPR8,FPR1     Take square root of FPR1 into FPR8-10 RRE
         STD   FPR8,16(,R7)  Store extended BFP square root part 1
         STD   FPR10,24(,R7) Store extended BFP square root part 2
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LA    R7,32(,R7)    Point to next Square Root result area
         LA    R8,16(,R8)    Point to next Square Root FPCR area
         LA    R3,16(,R3)    Point to next input value
         BCTR  R2,R12        Convert next input value.
*
         BR    R13           All converted; return.
*
         EJECT
***********************************************************************
*
* Perform Square Root using provided extended BFP inputs.  This set of
* tests exhaustively tests all rounding modes available for Square
* Root. The rounding mode can only be specified in the FPC.
*
* All five FPC rounding modes are tested because the preceeding tests,
* using rounding mode RNTE, do not often create results that require
* rounding.
*
* Two results are generated for each input and rounding mode: one RRE
* and one RXE.  Traps are disabled for all rounding mode tests.
*
* The quotient and FPCR contents are stored for each test.
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
         BASR  R9,0          Set top of rounding mode outer loop
*
         IC    R1,FPCMODES-L'FPCMODES(R5)  Get next FPC mode
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 0(R1)         Set FPC Rounding Mode
         LD    FPR1,0(,R3)   Get long BFP input value part 1
         LD    FPR3,8(,R3)   Get long BFP input value part 2
         SQXBR FPR8,FPR1     Take square root of FPR1 into FPR8 RRE
         STD   FPR8,0(,R7)   Store extended BFP root part 1
         STD   FPR10,8(,R7)  Store extended BFP root part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LA    R7,16(,R7)    Point to next square root result
         LA    R8,4(,R8)     Point to next FPCR result area
*
         BCTR  R5,R9         Iterate to next FPC mode
*
* End of FPC modes to be tested.  Advance to next test case.
*
         LA    R3,16(,R3)     Point to next input value
         LA    R8,12(,R8)    Skip to start of next FPCR result area
         BCTR  R2,R12        Divide next input value lots of times
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Table of FPC rounding modes to test quotient rounding modes.
*
* The Set BFP Rounding Mode does allow specification of the FPC
* rounding mode as an address, so we shall index into a table of
* BFP rounding modes without bothering with Execute.
*
***********************************************************************
         SPACE 2
*
* Rounding modes that may be set in the FPCR.  The FPCR controls
* rounding of the quotient.
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
* Short BFP test data sets for Square Root testing.
*
* The first test data set is used for tests of basic functionality,
* NaN propagation, and results from operations involving other than
* finite numbers.
*
* The second test data set is used for testing boundary conditions
* using finite non-zero values.  Each possible type of result (normal,
* scaled, etc) is created by members of this test data set.
*
* The third test data set is used for exhaustive testing of final
* results across the five rounding modes available for the Square Root
* instruction.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
* First input test data set, to test operations using non-finite or
* zero inputs.  Member values chosen to validate part 1 of Figure 19-17
* on page 19-21 of SA22-7832-10.
*
***********************************************************************
         SPACE 2
SBFPBIN  DS    0F                Inputs for short BFP basic tests
         DC    X'FF800000'         -inf
         DC    X'C0800000'         -4.0
         DC    X'80000000'         -0
         DC    X'00000000'         +0
         DC    X'40800000'         +4.0
         DC    X'7F800000'         +inf
         DC    X'FFCB0000'         -QNaN
         DC    X'7F8A0000'         +SNaN
         DC    X'40400000'         +3.0    Inexact, truncated
         DC    X'40A00000'         +5.0    Inexact, incremented
         DC    X'3D800000'         +0.0625 exact, expect 0.25
SBFPBCT  EQU   (*-SBFPBIN)/4     Count of short BFP in list
         SPACE 3
***********************************************************************
*
* Second input test data set.  These are finite positive values
* intended to test all combinations of rounding mode for a given
* result.  Values are chosen to create a requirement to round to the
* target precision after the computation
*
***********************************************************************
         SPACE 2
SBFPINRM DS    0F                Inputs for short BFP rounding testing
*
         DC    X'40400000'         +3.0    Inexact, truncated
         DC    X'40A00000'         +5.0    Inexact, incremented
*
SBFPRMCT EQU   (*-SBFPINRM)/4    Count of short BFP rounding tests
         EJECT
***********************************************************************
*
* Long BFP test data sets for Divide testing.
*
* The first test data set is used for tests of basic functionality,
* NaN propagation, and results from operations involving other than
* finite numbers.
*
* The second test data set is used for testing boundary conditions
* using finite non-zero values.  Each possible type of result (normal,
* scaled, etc) is created by members of this test data set.
*
* The third test data set is used for exhaustive testing of final
* results across the five rounding modes available for the Square Root
* instruction.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
* First input test data set, to test operations using non-finite or
* zero inputs.  Member values chosen to validate part 1 of Figure 19-17
* on page 19-21 of SA22-7832-10.
*
***********************************************************************
         SPACE 2
LBFPBIN  DS    0D                   Inputs for long BFP testing
         DC    X'FFF0000000000000'         -inf
         DC    X'C010000000000000'         -4.0
         DC    X'8000000000000000'         -0
         DC    X'0000000000000000'         +0
         DC    X'4010000000000000'         +4.0
         DC    X'7FF0000000000000'         +inf
         DC    X'FFF8B00000000000'         -QNaN
         DC    X'7FF0A00000000000'         +SNaN
         DC    X'4008000000000000'         +3.0 Inexact, truncated
         DC    X'4014000000000000'         +5.0 Inexact, incremented
         DC    X'3FB0000000000000'         +0.0625 exact, expect 0.25
LBFPBCT  EQU   (*-LBFPBIN)/8        Count of long BFP in list
         SPACE 3
***********************************************************************
*
* Second input test data set.  These are finite positive values
* intended to test all combinations of rounding mode for a given
* result.  Values are chosen to create a requirement to round to the
* target precision after the computation
*
***********************************************************************
         SPACE 2
LBFPINRM DS    0F
*
         DC    X'4008000000000000'         +3.0 Inexact, truncated
         DC    X'4014000000000000'         +5.0 Inexact, incremented
*
LBFPRMCT EQU   (*-LBFPINRM)/8    Count of long BFP rounding tests * 8
         EJECT
***********************************************************************
*
* Extended BFP test data sets for Divide testing.
*
* The first test data set is used for tests of basic functionality,
* NaN propagation, and results from operations involving other than
* finite numbers.
*
* The second test data set is used for testing boundary conditions
* using finite non-zero values.  Each possible type of result (normal,
* scaled, etc) is created by members of this test data set.
*
* The third test data set is used for exhaustive testing of final
* results across the five rounding modes available for the Square Root
* instruction.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
* First input test data set, to test operations using non-finite or
* zero inputs.  Member values chosen to validate part 1 of Figure 19-17
* on page 19-21 of SA22-7832-10.
*
***********************************************************************
         SPACE 2
XBFPBIN  DS    0D               Inputs for extended BFP testing
         DC    X'FFFF0000000000000000000000000000' -inf
         DC    X'C0010000000000000000000000000000' -4.0
         DC    X'80000000000000000000000000000000' -0
         DC    X'00000000000000000000000000000000' +0
         DC    X'40010000000000000000000000000000' +4.0
         DC    X'7FFF0000000000000000000000000000' +inf
         DC    X'FFFF8B00000000000000000000000000' -QNaN
         DC    X'7FFF0A00000000000000000000000000' +SNaN
         DC    X'40008000000000000000000000000000' +3.0 Inexact, trunc.
         DC    X'40014000000000000000000000000000' +5.0 Inexact, incre.
         DC    X'3FFB0000000000000000000000000000' +0.0625 expect 0.25
XBFPBCT  EQU   (*-XBFPBIN)/16   Count of extended BFP in list
         SPACE 3
***********************************************************************
*
* Second input test data set.  These are finite positive values
* intended to test all combinations of rounding mode for a given
* result.  Values are chosen to create a requirement to round to the
* target precision after the computation
*
***********************************************************************
         SPACE 2
XBFPINRM DS    0D
*
         DC    X'40008000000000000000000000000000' +3.0 Inexact, trunc.
         DC    X'40014000000000000000000000000000' +5.0 Inexact, incre.
*
XBFPRMCT EQU   (*-XBFPINRM)/16    Count of long BFP rounding tests
         EJECT
***********************************************************************
*                 ACTUAL results saved here
***********************************************************************
*
*               Locations for ACTUAL results
*
*
SBFPBOT  EQU   STRTLABL+X'1000'    Integer short non-finite BFP results
*                                  ..room for 16 tests, 8 used
SBFPBFL  EQU   STRTLABL+X'1100'    FPCR flags and DXC from short BFP
*                                  ..room for 16 tests, 8 used
*
SBFPRMO  EQU   STRTLABL+X'1200'    Short BFP rounding mode test results
*                                  ..Room for 16, 2 used.
SBFPRMOF EQU   STRTLABL+X'1400'    Short BFP rounding mode FPCR results
*                                  ..Room for 16, 2 used.
*                                  ..next location starts at X'1600'
*
LBFPBOT  EQU   STRTLABL+X'3000'    Integer long non-finite BFP results
*                                  ..room for 16 tests, 8 used
LBFPBFL  EQU   STRTLABL+X'3200'    FPCR flags and DXC from long BFP
*                                  ..room for 16 tests, 8 used
*
LBFPRMO  EQU   STRTLABL+X'3400'    Long BFP rounding mode test results
*                                  ..Room for 16, 4 used.
LBFPRMOF EQU   STRTLABL+X'3700'    Long BFP rounding mode FPCR results
*                                  ..Room for 16, 4 used.
*                                  ..next location starts at X'3800'
*
XBFPBOT  EQU   STRTLABL+X'5000'    Integer ext'd non-finite BFP results
*                                  ..room for 16 tests, 8 used
XBFPBFL  EQU   STRTLABL+X'5400'    FPCR flags and DXC from ext'd BFP
*                                  ..room for 16 tests, 8 used
*
XBFPRMO  EQU   STRTLABL+X'5500'    Ext'd BFP rounding mode test results
*                                  ..Room for 16, 4 used.
XBFPRMOF EQU   STRTLABL+X'5A00'    Ext'd BFP rounding mode FPCR results
*                                  ..Room for 16, 4 used.
*                                  ..next location starts at X'5B00'
         EJECT
***********************************************************************
*                    EXPECTED results
***********************************************************************
*
         ORG   STRTLABL+X'6000'   (past end of actual results)
*
SBFPBOT_GOOD EQU *
 DC CL48'SQEBR/SQEB -inf'
 DC XL16'7FC00000000000007FC0000000000000'
 DC CL48'SQEBR/SQEB -4'
 DC XL16'7FC00000000000007FC0000000000000'
 DC CL48'SQEBR/SQEB -0'
 DC XL16'80000000800000008000000080000000'
 DC CL48'SQEBR/SQEB +0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'SQEBR/SQEB +4'
 DC XL16'40000000400000004000000040000000'
 DC CL48'SQEBR/SQEB +inf'
 DC XL16'7F8000007F8000007F8000007F800000'
 DC CL48'SQEBR/SQEB -QNaN'
 DC XL16'FFCB0000FFCB0000FFCB0000FFCB0000'
 DC CL48'SQEBR/SQEB +SNaN'
 DC XL16'7FCA0000000000007FCA000000000000'
 DC CL48'SQEBR/SQEB +3'
 DC XL16'3FDDB3D73FDDB3D73FDDB3D73FDDB3D7'
 DC CL48'SQEBR/SQEB +5'
 DC XL16'400F1BBD400F1BBD400F1BBD400F1BBD'
 DC CL48'SQEBR/SQEB +0.0625'
 DC XL16'3E8000003E8000003E8000003E800000'
SBFPBOT_NUM EQU (*-SBFPBOT_GOOD)/64
*
*
SBFPBFL_GOOD EQU *
 DC CL48'SQEBR/SQEB -inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'SQEBR/SQEB -4 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'SQEBR/SQEB -0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'SQEBR/SQEB +0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'SQEBR/SQEB +4 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'SQEBR/SQEB +inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'SQEBR/SQEB -QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'SQEBR/SQEB +SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'SQEBR/SQEB +3 FPCR'
 DC XL16'00080000F800080000080000F8000800'
 DC CL48'SQEBR/SQEB +5 FPCR'
 DC XL16'00080000F8000C0000080000F8000C00'
 DC CL48'SQEBR/SQEB +0.0625 FPCR'
 DC XL16'00000000F800000000000000F8000000'
SBFPBFL_NUM EQU (*-SBFPBFL_GOOD)/64
*
*
SBFPRMO_GOOD EQU *
 DC CL48'SQEBR/SQEB RM RNTE,RZ +3'
 DC XL16'3FDDB3D73FDDB3D73FDDB3D73FDDB3D7'
 DC CL48'SQEBR/SQEB RM RP,RM +3'
 DC XL16'3FDDB3D83FDDB3D83FDDB3D73FDDB3D7'
 DC CL48'SQEBR/SQEB RM RFS +3'
 DC XL16'3FDDB3D73FDDB3D70000000000000000'
 DC CL48'SQEBR/SQEB RM RNTE,RZ +5'
 DC XL16'400F1BBD400F1BBD400F1BBC400F1BBC'
 DC CL48'SQEBR/SQEB RM RP,RM +5'
 DC XL16'400F1BBD400F1BBD400F1BBC400F1BBC'
 DC CL48'SQEBR/SQEB RM RFS +5'
 DC XL16'400F1BBD400F1BBD0000000000000000'
SBFPRMO_NUM EQU (*-SBFPRMO_GOOD)/64
*
*
SBFPRMOF_GOOD EQU *
 DC CL48'SQEBR/SQEB RM RNTE,RZ +3 FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'SQEBR/SQEB RM RP,RM +3 FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'SQEBR/SQEB RM RFS +3 FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'SQEBR/SQEB RM RNTE,RZ +5 FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'SQEBR/SQEB RM RP,RM +5 FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'SQEBR/SQEB RM RFS +5 FPCR'
 DC XL16'00080007000800070000000000000000'
SBFPRMOF_NUM EQU (*-SBFPRMOF_GOOD)/64
*
*
LBFPBOT_GOOD EQU *
 DC CL48'SQDBR -inf'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'SQDB  -inf'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'SQDBR -4'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'SQDB  -4'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'SQDBR -0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'SQDB  -0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'SQDBR +0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'SQDB  +0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'SQDBR +4'
 DC XL16'40000000000000004000000000000000'
 DC CL48'SQDB  +4'
 DC XL16'40000000000000004000000000000000'
 DC CL48'SQDBR +inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'SQDB  +inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'SQDBR -QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'SQDB  -QNaN'
 DC XL16'FFF8B00000000000FFF8B00000000000'
 DC CL48'SQDBR +SNaN'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'SQDB  +SNaN'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'SQDBR +3'
 DC XL16'3FFBB67AE8584CAA3FFBB67AE8584CAA'
 DC CL48'SQDB  +3'
 DC XL16'3FFBB67AE8584CAA3FFBB67AE8584CAA'
 DC CL48'SQDBR +5'
 DC XL16'4001E3779B97F4A84001E3779B97F4A8'
 DC CL48'SQDB  +5'
 DC XL16'4001E3779B97F4A84001E3779B97F4A8'
 DC CL48'SQDBR 0.0625'
 DC XL16'3FD00000000000003FD0000000000000'
 DC CL48'SQDB  0.0625'
 DC XL16'3FD00000000000003FD0000000000000'
LBFPBOT_NUM EQU (*-LBFPBOT_GOOD)/64
*
*
LBFPBFL_GOOD EQU *
 DC CL48'SQDBR/SQDB -inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'SQDBR/SQDB -4 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'SQDBR/SQDB -0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'SQDBR/SQDB +0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'SQDBR/SQDB +4 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'SQDBR/SQDB +inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'SQDBR/SQDB -QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'SQDBR/SQDB +SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'SQDBR/SQDB +3 FPCR'
 DC XL16'00080000F800080000080000F8000800'
 DC CL48'SQDBR/SQDB +5 FPCR'
 DC XL16'00080000F8000C0000080000F8000C00'
 DC CL48'SQDBR/SQDB +0.0625 FPCR'
 DC XL16'00000000F800000000000000F8000000'
LBFPBFL_NUM EQU (*-LBFPBFL_GOOD)/64
*
*
LBFPRMO_GOOD EQU *
 DC CL48'SQDBR/SQDB RM RNTE +3'
 DC XL16'3FFBB67AE8584CAA3FFBB67AE8584CAA'
 DC CL48'SQDBR/SQDB RM RZ +3'
 DC XL16'3FFBB67AE8584CAA3FFBB67AE8584CAA'
 DC CL48'SQDBR/SQDB RM RP +3'
 DC XL16'3FFBB67AE8584CAB3FFBB67AE8584CAB'
 DC CL48'SQDBR/SQDB RM RM +3'
 DC XL16'3FFBB67AE8584CAA3FFBB67AE8584CAA'
 DC CL48'SQDBR/SQDB RM RFS +3'
 DC XL16'3FFBB67AE8584CAB3FFBB67AE8584CAB'
 DC CL48'SQDBR/SQDB RM RNTE +5'
 DC XL16'4001E3779B97F4A84001E3779B97F4A8'
 DC CL48'SQDBR/SQDB RM RZ +5'
 DC XL16'4001E3779B97F4A74001E3779B97F4A7'
 DC CL48'SQDBR/SQDB RM RP +5'
 DC XL16'4001E3779B97F4A84001E3779B97F4A8'
 DC CL48'SQDBR/SQDB RM RM +5'
 DC XL16'4001E3779B97F4A74001E3779B97F4A7'
 DC CL48'SQDBR/SQDB RM RFS +5'
 DC XL16'4001E3779B97F4A74001E3779B97F4A7'
LBFPRMO_NUM EQU (*-LBFPRMO_GOOD)/64
*
*
LBFPRMOF_GOOD EQU *
 DC CL48'SQEBR/SQEB RM RNTE,RZ +3 FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'SQEBR/SQEB RM RP,RM +3 FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'SQEBR/SQEB RM RFS +3 FPCR'
 DC XL16'00080007000800070000000000000000'
 DC CL48'SQEBR/SQEB RM RNTE,RZ +5 FPCR'
 DC XL16'00080000000800000008000100080001'
 DC CL48'SQEBR/SQEB RM RP,RM +5 FPCR'
 DC XL16'00080002000800020008000300080003'
 DC CL48'SQEBR/SQEB RM RFS +5 FPCR'
 DC XL16'00080007000800070000000000000000'
LBFPRMOF_NUM EQU (*-LBFPRMOF_GOOD)/64
*
*
XBFPBOT_GOOD EQU *
 DC CL48'SQXBR NT -inf'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'SQXBR Tr -inf'
 DC XL16'00000000000000000000000000000000'
 DC CL48'SQXBR NT -4'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'SQXBR Tr -4'
 DC XL16'00000000000000000000000000000000'
 DC CL48'SQXBR NT -0'
 DC XL16'80000000000000000000000000000000'
 DC CL48'SQXBR Tr -0'
 DC XL16'80000000000000000000000000000000'
 DC CL48'SQXBR NT +0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'SQXBR Tr +0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'SQXBR NT +4'
 DC XL16'40000000000000000000000000000000'
 DC CL48'SQXBR Tr +4'
 DC XL16'40000000000000000000000000000000'
 DC CL48'SQXBR NT +inf'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'SQXBR Tr +inf'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'SQXBR NT -QNaN'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'SQXBR Tr -QNaN'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'SQXBR NT +SNaN'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'SQXBR Tr +SNaN'
 DC XL16'00000000000000000000000000000000'
 DC CL48'SQXBR NT +3'
 DC XL16'3FFFBB67AE8584CAA73B25742D7078B8'
 DC CL48'SQXBR Tr +3'
 DC XL16'3FFFBB67AE8584CAA73B25742D7078B8'
 DC CL48'SQXBR NT +5'
 DC XL16'40001E3779B97F4A7C15F39CC0605CEE'
 DC CL48'SQXBR Tr +5'
 DC XL16'40001E3779B97F4A7C15F39CC0605CEE'
 DC CL48'SQXBR NT 0.0625'
 DC XL16'3FFD0000000000000000000000000000'
 DC CL48'SQXBR Tr 0.0625'
 DC XL16'3FFD0000000000000000000000000000'
XBFPBOT_NUM EQU (*-XBFPBOT_GOOD)/64
*
*
XBFPBFL_GOOD EQU *
 DC CL48'SQXBR -inf FPCR'
 DC XL16'00800000F80080000000000000000000'
 DC CL48'SQXBR -4 FPCR'
 DC XL16'00800000F80080000000000000000000'
 DC CL48'SQXBR -0 FPCR'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'SQXBR +0 FPCR'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'SQXBR +4 FPCR'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'SQXBR +inf FPCR'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'SQXBR -QNaN FPCR'
 DC XL16'00000000F80000000000000000000000'
 DC CL48'SQXBR +SNaN FPCR'
 DC XL16'00800000F80080000000000000000000'
 DC CL48'SQXBR +3 FPCR'
 DC XL16'00080000F80008000000000000000000'
 DC CL48'SQXBR +5 FPCR'
 DC XL16'00080000F8000C000000000000000000'
 DC CL48'SQXBR +0.0625 FPCR'
 DC XL16'00000000F80000000000000000000000'
XBFPBFL_NUM EQU (*-XBFPBFL_GOOD)/64
*
*
XBFPRMO_GOOD EQU *
 DC CL48'SQXBR RM RNTE +3'
 DC XL16'3FFFBB67AE8584CAA73B25742D7078B8'
 DC CL48'SQXBR RM RZ +3'
 DC XL16'3FFFBB67AE8584CAA73B25742D7078B8'
 DC CL48'SQXBR RM RP +3'
 DC XL16'3FFFBB67AE8584CAA73B25742D7078B9'
 DC CL48'SQXBR RM RM +3'
 DC XL16'3FFFBB67AE8584CAA73B25742D7078B8'
 DC CL48'SQXBR RM RFS +3'
 DC XL16'3FFFBB67AE8584CAA73B25742D7078B9'
 DC CL48'SQXBR RM RNTE +5'
 DC XL16'40001E3779B97F4A7C15F39CC0605CEE'
 DC CL48'SQXBR RM RZ +5'
 DC XL16'40001E3779B97F4A7C15F39CC0605CED'
 DC CL48'SQXBR RM RP +5'
 DC XL16'40001E3779B97F4A7C15F39CC0605CEE'
 DC CL48'SQXBR RM RM +5'
 DC XL16'40001E3779B97F4A7C15F39CC0605CED'
 DC CL48'SQXBR RM RFS +5'
 DC XL16'40001E3779B97F4A7C15F39CC0605CED'
XBFPRMO_NUM EQU (*-XBFPRMO_GOOD)/64
*
*
XBFPRMOF_GOOD EQU *
 DC CL48'SQXBR RM RTNE,RZ,RP,RM +3 FPCR'
 DC XL16'00080000000800010008000200080003'
 DC CL48'SQXBR RM RFS +3 FPCR'
 DC XL16'00080007000000000000000000000000'
 DC CL48'SQXBR RM RTNE,RZ,RP,RM +5 FPCR'
 DC XL16'00080000000800010008000200080003'
 DC CL48'SQXBR RM RFS +5 FPCR'
 DC XL16'00080007000000000000000000000000'
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
         DC    A(SBFPBOT)
         DC    A(SBFPBOT_GOOD)
         DC    A(SBFPBOT_NUM)
*
         DC    A(SBFPBFL)
         DC    A(SBFPBFL_GOOD)
         DC    A(SBFPBFL_NUM)
*
         DC    A(SBFPRMO)
         DC    A(SBFPRMO_GOOD)
         DC    A(SBFPRMO_NUM)
*
         DC    A(SBFPRMOF)
         DC    A(SBFPRMOF_GOOD)
         DC    A(SBFPRMOF_NUM)
*
         DC    A(LBFPBOT)
         DC    A(LBFPBOT_GOOD)
         DC    A(LBFPBOT_NUM)
*
         DC    A(LBFPBFL)
         DC    A(LBFPBFL_GOOD)
         DC    A(LBFPBFL_NUM)
*
         DC    A(LBFPRMO)
         DC    A(LBFPRMO_GOOD)
         DC    A(LBFPRMO_NUM)
*
         DC    A(LBFPRMOF)
         DC    A(LBFPRMOF_GOOD)
         DC    A(LBFPRMOF_NUM)
*
         DC    A(XBFPBOT)
         DC    A(XBFPBOT_GOOD)
         DC    A(XBFPBOT_NUM)
*
         DC    A(XBFPBFL)
         DC    A(XBFPBFL_GOOD)
         DC    A(XBFPBFL_NUM)
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
