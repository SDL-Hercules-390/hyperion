  TITLE 'bfp-020-multlonger: Test IEEE Multiply'
***********************************************************************
*
*Testcase IEEE MULTIPLY (to longer precision)
*  Test case capability includes IEEE exceptions trappable and
*  otherwise. Test results, FPCR flags, the Condition code, and any
*  DXC are saved for all tests.
*
*  The result precision for each instruction is longer than the input
*  operands.  As a result, the underflow and overflow exceptions will
*  never occur.  Further, the results are always exact.  There is
*  no rounding of the result.
*
*  The fused multiply operations are not included in this test program,
*  nor are the standard multiply instructions.  The former are
*  are excluded to keep test case complexity manageable, and latter
*  because they require a more extensive testing profile (overflow,
*  underflow, rounding).
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
*                      bfp-020-multlonger.asm
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
* Tests the following five conversion instructions
*   MULTIPLY (short BFP, RRE) (short to long)
*   MULTIPLY (long BFP, RRE) (long to extended)
*   MULTIPLY (short BFP, RXE) (short to long)
*   MULTIPLY (long BFP, RXE) (long to extended)
*
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R
* commands.
*
* Test Case Order
* 1) Short BFP basic tests, including traps and NaN propagation
* 2) Long BFP basic tests, including traps and NaN propagation
*
* One input test sets are provided each for short and long BFP inputs.
*   Test values are the same for each precision.
*
* Also tests the following floating point support instructions
*   LOAD  (Short)
*   LOAD  (Long)
*   LFPC  (Load Floating Point Control Register)
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
BFPMUL2L START 0
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
*
         LA    R10,LONGNF    Point to long BFP non-finite inputs
         BAS   R13,LBFPNF    Multiply long BFP non-finites
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
         DC    A(LBFPNFOT)
         DC    A(LBFPNFFL)
*
LONGNF   DS    0F           Input pairs for long BFP non-finite testing
         DC    A(LBFPNFCT)
         DC    A(LBFPNFIN)
         DC    A(XBFPNFOT)
         DC    A(XBFPNFFL)
*
         EJECT
***********************************************************************
*
* Perform Multiply using provided short BFP inputs.  This set of tests
* checks NaN propagation, operations on values that are not finite
* numbers, and other basic tests.  This set generates results that can
* be validated against Figure 19-23 on page 19-28 of SA22-7832-10.
* Each value in this table is tested against every other value in the
* table.  Eight entries means 64 result sets.
*
* Four results are generated for each input: one RRE with all
* exceptions non-trappable, a second RRE with all exceptions trappable,
* a third RXE with all exceptions non-trappable, a fourth RXE with all
* exceptions trappable,
*
* The difference, FPCR, and condition code are stored for each result.
*
***********************************************************************
         SPACE 2
SBFPNF   DS    0H            BFP Short non-finite values tests
         LM    R2,R3,0(R10)  Get count and addr of multiplicand values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LM    R4,R5,0(R10)  Get count and start of multiplier values
*                            ..which are the same as the multiplicands
         BASR  R6,0          Set top of inner loop
*
         LE    FPR8,0(,R3)   Get short BFP multiplicand
         LE    FPR1,0(,R5)   Get short BFP multiplier
         LFPC  FPCREGNT      Set exceptions non-trappable
         MDEBR FPR8,FPR1     Multiply short FPR8 by FPR1 RRE
         STD   FPR8,0(,R7)   Store long BFP product
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LE    FPR8,0(,R3)   Get short BFP multiplicand
         LE    FPR1,0(,R5)   Get short BFP multiplier
         LFPC  FPCREGTR      Set exceptions trappable
         MDEBR FPR8,FPR1     Multiply short FPR8 by FPR1 RRE
         STD   FPR8,8(,R7)   Store long BFP product
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LE    FPR8,0(,R3)   Get short BFP multiplicand
         LFPC  FPCREGNT      Set exceptions non-trappable
         MDEB  FPR8,0(,R5)   Multiply short FPR8 by multiplier RXE
         STD   FPR8,16(,R7)  Store long BFP product
         STFPC 8(R8)         Store resulting FPCR flags and DXC
*
         LE    FPR8,0(,R3)   Get short BFP multiplicand
         LFPC  FPCREGTR      Set exceptions trappable
         MDEB  FPR8,0(,R5)   Multiply short FPR8 by multiplier RXE
         STD   FPR8,24(,R7)  Store long BFP product
         STFPC 12(R8)        Store resulting FPCR flags and DXC
*
         LA    R5,4(,R5)     Point to next multiplier value
         LA    R7,4*8(,R7)   Point to next Multiply result area
         LA    R8,4*4(,R8)   Point to next Multiply FPCR area
         BCTR  R4,R6         Loop through right-hand values
*
         LA    R3,4(,R3)     Point to next input multiplicand
         BCTR  R2,R12        Loop through left-hand values
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Multiply using provided long BFP inputs.  This set of tests
* checks NaN propagation, operations on values that are not finite
* numbers, and other basic tests.  This set generates results that can
* validated against Figure 19-23 on page 19-28 of SA22-7832-10.  Each
* value in this table is tested against every other value in the table.
* Eight entries means 64 result sets.
*
* Four results are generated for each input: one RRE with all
* exceptions non-trappable, a second RRE with all exceptions trappable,
* a third RXE with all exceptions non-trappable, a fourth RXE with all
* exceptions trappable,
*
* The difference, FPCR, and condition code are stored for each result.
*
***********************************************************************
         SPACE 2
LBFPNF   DS    0H            BFP long non-finite values tests
         LM    R2,R3,0(R10)  Get count and addr of multiplicand values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LM    R4,R5,0(R10)  Get count and start of multiplier values
*                            ..which are the same as the multiplicands
         BASR  R6,0          Set top of inner loop
*
         LD    FPR8,0(,R3)   Get long BFP multiplicand
         LD    FPR1,0(,R5)   Get long BFP multiplier
         LFPC  FPCREGNT      Set exceptions non-trappable
         MXDBR FPR8,FPR1     Multiply long FPR8 by FPR1 RRE
         STD   FPR8,0(,R7)   Store extended BFP product part 1
         STD   FPR10,8(,R7)  Store extended BFP product part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LD    FPR8,0(,R3)   Get long BFP multiplicand
         LD    FPR1,0(,R5)   Get long BFP multiplier
         LFPC  FPCREGTR      Set exceptions trappable
         MXDBR FPR8,FPR1     Multiply long multiplier from FPR8 RRE
         STD   FPR8,16(,R7)  Store extended BFP product part 1
         STD   FPR10,24(,R7) Store extended BFP product part 2
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LD    FPR8,0(,R3)   Get long BFP multiplicand
         LFPC  FPCREGNT      Set exceptions non-trappable
         MXDB  FPR8,0(,R5)   Multiply long FPR8 by multiplier RXE
         STD   FPR8,32(,R7)  Store extended BFP product part 1
         STD   FPR10,40(,R7) Store extended BFP product part 2
         STFPC 8(R8)         Store resulting FPCR flags and DXC
*
         LD    FPR8,0(,R3)   Get long BFP multiplicand
         LFPC  FPCREGTR      Set exceptions trappable
         MXDB  FPR8,0(,R5)   Multiply long FPR8 by multiplier RXE
         STD   FPR8,48(,R7)  Store extended BFP product part 1
         STD   FPR10,56(,R7) Store extended BFP product part 2
         STFPC 12(R8)        Store resulting FPCR flags and DXC
*
         LA    R5,8(,R5)     Point to next multiplier value
         LA    R7,4*16(,R7)  Point to next Multiply result area
         LA    R8,4*4(,R8)   Point to next Multiply FPCR area
         BCTR  R4,R6         Loop through right-hand values
*
         LA    R3,8(,R3)     Point to next multiplicand value
         BCTR  R2,R12        Multiply until all cases tested
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Short BFP test data for Multiply to longer precision testing.
*
* The test data set is used for tests of basic functionality, NaN
* propagation, and results from operations involving other than finite
* numbers.
*
* Member values chosen to validate against Figure 19-23 on page 19-28
* of SA22-7832-10.  Each value in this table is tested against every
* other value in the table.  Eight entries means 64 result sets.
*
* Because Multiply to longer precision cannot generate overflow nor
* underflow exceptions and the result is always exact, there are no
* further tests required.  Any more extensive testing would be in
* effect a test of Softfloat, not of the the integration of Softfloat
* to Hercules.
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
        EJECT
***********************************************************************
*
* Long BFP test data for Multiply to longer precision testing.
*
* The test data set is used for tests of basic functionality, NaN
* propagation, and results from operations involving other than finite
* numbers.
*
* Member values chosen to validate against Figure 19-23 on page 19-28
* of SA22-7832-10.  Each value in this table is tested against every
* other value in the table.  Eight entries means 64 result sets.
*
* Because Multiply to longer precision cannot generate overflow nor
* underflow exceptions and the result is always exact, there are no
* further tests required.  Any more extensive testing would be in
* effect a test of Softfloat, not of the the integration of Softfloat
* to Hercules.
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
         EJECT
***********************************************************************
*                 ACTUAL results saved here
***********************************************************************
*
*               Locations for ACTUAL results
*
LBFPNFOT EQU   STRTLABL+X'1000'    Short non-finite BFP results
*                                  ..room for 64 tests, 64 used
LBFPNFFL EQU   STRTLABL+X'1800'    FPCR flags and DXC from short BFP
*                                  ..room for 64 tests, 64 used
*                                  ..next location starts at X'1C00'
*
*
XBFPNFOT EQU   STRTLABL+X'2000'    Long non-finite BFP results
*                                  ..room for 64 tests, 64 used
XBFPNFFL EQU   STRTLABL+X'3000'    FPCR flags and DXC from long BFP
*                                  ..room for 64 tests, 64 used
*                                  ..next location starts at X'3400'
*
         EJECT
***********************************************************************
*                    EXPECTED results
***********************************************************************
*
         ORG   STRTLABL+X'4000'   (past end of actual results)
*
LBFPNFOT_GOOD EQU *
 DC CL48'MDEBR NF -inf/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MDEB NF -inf/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MDEBR NF -inf/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MDEB NF -inf/-2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MDEBR NF -inf/-0'
 DC XL16'7FF8000000000000FF80000000000000'
 DC CL48'MDEB NF -inf/-0'
 DC XL16'7FF8000000000000FF80000000000000'
 DC CL48'MDEBR NF -inf/+0'
 DC XL16'7FF8000000000000FF80000000000000'
 DC CL48'MDEB NF -inf/+0'
 DC XL16'7FF8000000000000FF80000000000000'
 DC CL48'MDEBR NF -inf/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MDEB NF -inf/+2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MDEBR NF -inf/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MDEB NF -inf/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MDEBR NF -inf/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEB NF -inf/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEBR NF -inf/+SNaN'
 DC XL16'7FF9400000000000FF80000000000000'
 DC CL48'MDEB NF -inf/+SNaN'
 DC XL16'7FF9400000000000FF80000000000000'
 DC CL48'MDEBR NF -2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MDEB NF -2.0/-inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MDEBR NF -2.0/-2.0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'MDEB NF -2.0/-2.0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'MDEBR NF -2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MDEB NF -2.0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MDEBR NF -2.0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MDEB NF -2.0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MDEBR NF -2.0/+2.0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'MDEB NF -2.0/+2.0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'MDEBR NF -2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MDEB NF -2.0/+inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MDEBR NF -2.0/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEB NF -2.0/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEBR NF -2.0/+SNaN'
 DC XL16'7FF9400000000000C000000000000000'
 DC CL48'MDEB NF -2.0/+SNaN'
 DC XL16'7FF9400000000000C000000000000000'
 DC CL48'MDEBR NF -0/-inf'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MDEB NF -0/-inf'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MDEBR NF -0/-2.0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MDEB NF -0/-2.0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MDEBR NF -0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MDEB NF -0/-0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MDEBR NF -0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MDEB NF -0/+0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MDEBR NF -0/+2.0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MDEB NF -0/+2.0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MDEBR NF -0/+inf'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MDEB NF -0/+inf'
 DC XL16'7FF80000000000008000000000000000'
 DC CL48'MDEBR NF -0/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEB NF -0/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEBR NF -0/+SNaN'
 DC XL16'7FF94000000000008000000000000000'
 DC CL48'MDEB NF -0/+SNaN'
 DC XL16'7FF94000000000008000000000000000'
 DC CL48'MDEBR NF +0/-inf'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MDEB NF +0/-inf'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MDEBR NF +0/-2.0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MDEB NF +0/-2.0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MDEBR NF +0/-0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MDEB NF +0/-0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MDEBR NF +0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MDEB NF +0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MDEBR NF +0/+2.0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MDEB NF +0/+2.0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MDEBR NF +0/+inf'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MDEB NF +0/+inf'
 DC XL16'7FF80000000000000000000000000000'
 DC CL48'MDEBR NF +0/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEB NF +0/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEBR NF +0/+SNaN'
 DC XL16'7FF94000000000000000000000000000'
 DC CL48'MDEB NF +0/+SNaN'
 DC XL16'7FF94000000000000000000000000000'
 DC CL48'MDEBR NF +2.0/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MDEB NF +2.0/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MDEBR NF +2.0/-2.0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'MDEB NF +2.0/-2.0'
 DC XL16'C010000000000000C010000000000000'
 DC CL48'MDEBR NF +2.0/-0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MDEB NF +2.0/-0'
 DC XL16'80000000000000008000000000000000'
 DC CL48'MDEBR NF +2.0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MDEB NF +2.0/+0'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MDEBR NF +2.0/+2.0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'MDEB NF +2.0/+2.0'
 DC XL16'40100000000000004010000000000000'
 DC CL48'MDEBR NF +2.0/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MDEB NF +2.0/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MDEBR NF +2.0/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEB NF +2.0/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEBR NF +2.0/+SNaN'
 DC XL16'7FF94000000000004000000000000000'
 DC CL48'MDEB NF +2.0/+SNaN'
 DC XL16'7FF94000000000004000000000000000'
 DC CL48'MDEBR NF +inf/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MDEB NF +inf/-inf'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MDEBR NF +inf/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MDEB NF +inf/-2.0'
 DC XL16'FFF0000000000000FFF0000000000000'
 DC CL48'MDEBR NF +inf/-0'
 DC XL16'7FF80000000000007F80000000000000'
 DC CL48'MDEB NF +inf/-0'
 DC XL16'7FF80000000000007F80000000000000'
 DC CL48'MDEBR NF +inf/+0'
 DC XL16'7FF80000000000007F80000000000000'
 DC CL48'MDEB NF +inf/+0'
 DC XL16'7FF80000000000007F80000000000000'
 DC CL48'MDEBR NF +inf/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MDEB NF +inf/+2.0'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MDEBR NF +inf/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MDEB NF +inf/+inf'
 DC XL16'7FF00000000000007FF0000000000000'
 DC CL48'MDEBR NF +inf/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEB NF +inf/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEBR NF +inf/+SNaN'
 DC XL16'7FF94000000000007F80000000000000'
 DC CL48'MDEB NF +inf/+SNaN'
 DC XL16'7FF94000000000007F80000000000000'
 DC CL48'MDEBR NF -QNaN/-inf'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEB NF -QNaN/-inf'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEBR NF -QNaN/-2.0'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEB NF -QNaN/-2.0'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEBR NF -QNaN/-0'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEB NF -QNaN/-0'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEBR NF -QNaN/+0'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEB NF -QNaN/+0'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEBR NF -QNaN/+2.0'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEB NF -QNaN/+2.0'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEBR NF -QNaN/+inf'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEB NF -QNaN/+inf'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEBR NF -QNaN/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEB NF -QNaN/-QNaN'
 DC XL16'FFF9600000000000FFF9600000000000'
 DC CL48'MDEBR NF -QNaN/+SNaN'
 DC XL16'7FF9400000000000FFCB000000000000'
 DC CL48'MDEB NF -QNaN/+SNaN'
 DC XL16'7FF9400000000000FFCB000000000000'
 DC CL48'MDEBR NF +SNaN/-inf'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEB NF +SNaN/-inf'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEBR NF +SNaN/-2.0'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEB NF +SNaN/-2.0'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEBR NF +SNaN/-0'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEB NF +SNaN/-0'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEBR NF +SNaN/+0'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEB NF +SNaN/+0'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEBR NF +SNaN/+2.0'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEB NF +SNaN/+2.0'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEBR NF +SNaN/+inf'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEB NF +SNaN/+inf'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEBR NF +SNaN/-QNaN'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEBR NF +SNaN/-QNaN'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEBR NF +SNaN/+SNaN'
 DC XL16'7FF94000000000007F8A000000000000'
 DC CL48'MDEB NF +SNaN/+SNaN'
 DC XL16'7FF94000000000007F8A000000000000'
LBFPNFOT_NUM EQU (*-LBFPNFOT_GOOD)/64
*
*
LBFPNFFL_GOOD EQU *
 DC CL48'MDBR NF -inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF -inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDB NF -inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF -inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF -inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF -2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF -2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF -2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF -2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF -0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDB NF -0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF -0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF -0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF -0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF +0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDB NF +0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF +0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF +0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF +0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF +0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF +0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF +0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF +2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF +2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF +2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF +2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF +2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF +2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF +2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF +2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF +inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF +inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF +inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDB NF +inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF +inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF +inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF +inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF +inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF -QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF -QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF -QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDBR NF -QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MDB NF -QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF +SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDB NF +SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF +SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDB NF +SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF +SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDB NF +SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDBR NF +SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MDB NF +SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
LBFPNFFL_NUM EQU (*-LBFPNFFL_GOOD)/64
*
*
XBFPNFOT_GOOD EQU *
 DC CL48'MXDBR NF -inf/-inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDBR NF -inf/-inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDB NF -inf/-inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDB NF -inf/-inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDBR NF -inf/-2.0 NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDBR NF -inf/-2.0 Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDB NF -inf/-2.0 NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDB NF -inf/-2.0 Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDBR NF -inf/-0 NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDBR NF -inf/-0 Tr'
 DC XL16'FFF00000000000000000000000000000'
 DC CL48'MXDB NF -inf/-0 NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDB NF -inf/-0 Tr'
 DC XL16'FFF00000000000000000000000000000'
 DC CL48'MXDBR NF -inf/+0 NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDBR NF -inf/+0 Tr'
 DC XL16'FFF00000000000000000000000000000'
 DC CL48'MXDB NF -inf/+0 NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDB NF -inf/+0 Tr'
 DC XL16'FFF00000000000000000000000000000'
 DC CL48'MXDBR NF -inf/+2.0 NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDBR NF -inf/+2.0 Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDB NF -inf/+2.0 NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDB NF -inf/+2.0 Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDBR NF -inf/+inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDBR NF -inf/+inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDB NF -inf/+inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDB NF -inf/+inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDBR NF -inf/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -inf/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -inf/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -inf/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -inf/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF -inf/+SNaN Tr'
 DC XL16'FFF00000000000000000000000000000'
 DC CL48'MXDB NF -inf/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF -inf/+SNaN Tr'
 DC XL16'FFF00000000000000000000000000000'
 DC CL48'MXDBR NF -2.0/-inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDBR NF -2.0/-inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDB NF -2.0/-inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDB NF -2.0/-inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDBR NF -2.0/-2.0 NT'
 DC XL16'40010000000000000000000000000000'
 DC CL48'MXDBR NF -2.0/-2.0 Tr'
 DC XL16'40010000000000000000000000000000'
 DC CL48'MXDB NF -2.0/-2.0 NT'
 DC XL16'40010000000000000000000000000000'
 DC CL48'MXDB NF -2.0/-2.0 Tr'
 DC XL16'40010000000000000000000000000000'
 DC CL48'MXDBR NF -2.0/-0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF -2.0/-0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF -2.0/-0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF -2.0/-0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF -2.0/+0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF -2.0/+0 Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF -2.0/+0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF -2.0/+0 Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF -2.0/+2.0 NT'
 DC XL16'C0010000000000000000000000000000'
 DC CL48'MXDBR NF -2.0/+2.0 Tr'
 DC XL16'C0010000000000000000000000000000'
 DC CL48'MXDB NF -2.0/+2.0 NT'
 DC XL16'C0010000000000000000000000000000'
 DC CL48'MXDB NF -2.0/+2.0 Tr'
 DC XL16'C0010000000000000000000000000000'
 DC CL48'MXDBR NF -2.0/+inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDBR NF -2.0/+inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDB NF -2.0/+inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDB NF -2.0/+inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDBR NF -2.0/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -2.0/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -2.0/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -2.0/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -2.0/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF -2.0/+SNaN Tr'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'MXDB NF -2.0/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF -2.0/+SNaN Tr'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'MXDBR NF -0/-inf NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDBR NF -0/-inf Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF -0/-inf NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDB NF -0/-inf Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF -0/-2.0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF -0/-2.0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF -0/-2.0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF -0/-2.0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF -0/-0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF -0/-0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF -0/-0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF -0/-0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF -0/+0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF -0/+0 Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF -0/+0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF -0/+0 Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF -0/+2.0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF -0/+2.0 Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF -0/+2.0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF -0/+2.0 Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF -0/+inf NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDBR NF -0/+inf Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF -0/+inf NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDB NF -0/+inf Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF -0/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -0/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -0/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -0/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -0/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF -0/+SNaN Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF -0/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF -0/+SNaN Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF +0/-inf NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDBR NF +0/-inf Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF +0/-inf NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDB NF +0/-inf Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF +0/-2.0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF +0/-2.0 Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF +0/-2.0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF +0/-2.0 Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF +0/-0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF +0/-0 Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF +0/-0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF +0/-0 Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF +0/+0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF +0/+0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF +0/+0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF +0/+0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF +0/+2.0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF +0/+2.0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF +0/+2.0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF +0/+2.0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF +0/+inf NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDBR NF +0/+inf Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF +0/+inf NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDB NF +0/+inf Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF +0/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF +0/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF +0/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF +0/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF +0/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF +0/+SNaN Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF +0/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF +0/+SNaN Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF +2.0/-inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDBR NF +2.0/-inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDB NF +2.0/-inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDB NF +2.0/-inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDBR NF +2.0/-2.0 NT'
 DC XL16'C0010000000000000000000000000000'
 DC CL48'MXDBR NF +2.0/-2.0 Tr'
 DC XL16'C0010000000000000000000000000000'
 DC CL48'MXDB NF +2.0/-2.0 NT'
 DC XL16'C0010000000000000000000000000000'
 DC CL48'MXDB NF +2.0/-2.0 Tr'
 DC XL16'C0010000000000000000000000000000'
 DC CL48'MXDBR NF +2.0/-0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF +2.0/-0 Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF +2.0/-0 NT'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDB NF +2.0/-0 Tr'
 DC XL16'80000000000000000000000000000000'
 DC CL48'MXDBR NF +2.0/+0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF +2.0/+0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF +2.0/+0 NT'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDB NF +2.0/+0 Tr'
 DC XL16'00000000000000000000000000000000'
 DC CL48'MXDBR NF +2.0/+2.0 NT'
 DC XL16'40010000000000000000000000000000'
 DC CL48'MXDBR NF +2.0/+2.0 Tr'
 DC XL16'40010000000000000000000000000000'
 DC CL48'MXDB NF +2.0/+2.0 NT'
 DC XL16'40010000000000000000000000000000'
 DC CL48'MXDB NF +2.0/+2.0 Tr'
 DC XL16'40010000000000000000000000000000'
 DC CL48'MXDBR NF +2.0/+inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDBR NF +2.0/+inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDB NF +2.0/+inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDB NF +2.0/+inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDBR NF +2.0/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF +2.0/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF +2.0/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF +2.0/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF +2.0/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF +2.0/+SNaN Tr'
 DC XL16'40000000000000000000000000000000'
 DC CL48'MXDB NF +2.0/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF +2.0/+SNaN Tr'
 DC XL16'40000000000000000000000000000000'
 DC CL48'MXDBR NF +inf/-inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDBR NF +inf/-inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDB NF +inf/-inf NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDB NF +inf/-inf Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDBR NF +inf/-2.0 NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDBR NF +inf/-2.0 Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDB NF +inf/-2.0 NT'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDB NF +inf/-2.0 Tr'
 DC XL16'FFFF0000000000000000000000000000'
 DC CL48'MXDBR NF +inf/-0 NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDBR NF +inf/-0 Tr'
 DC XL16'7FF00000000000000000000000000000'
 DC CL48'MXDB NF +inf/-0 NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDB NF +inf/-0 Tr'
 DC XL16'7FF00000000000000000000000000000'
 DC CL48'MXDBR NF +inf/+0 NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDBR NF +inf/+0 Tr'
 DC XL16'7FF00000000000000000000000000000'
 DC CL48'MXDB NF +inf/+0 NT'
 DC XL16'7FFF8000000000000000000000000000'
 DC CL48'MXDB NF +inf/+0 Tr'
 DC XL16'7FF00000000000000000000000000000'
 DC CL48'MXDBR NF +inf/+2.0 NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDBR NF +inf/+2.0 Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDB NF +inf/+2.0 NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDB NF +inf/+2.0 Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDBR NF +inf/+inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDBR NF +inf/+inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDB NF +inf/+inf NT'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDB NF +inf/+inf Tr'
 DC XL16'7FFF0000000000000000000000000000'
 DC CL48'MXDBR NF +inf/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF +inf/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF +inf/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF +inf/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF +inf/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF +inf/+SNaN Tr'
 DC XL16'7FF00000000000000000000000000000'
 DC CL48'MXDB NF +inf/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF +inf/+SNaN Tr'
 DC XL16'7FF00000000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/-inf NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/-inf Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/-inf NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/-inf Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/-2.0 NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/-2.0 Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/-2.0 NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/-2.0 Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/-0 NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/-0 Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/-0 NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/-0 Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/+0 NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/+0 Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/+0 NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/+0 Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/+2.0 NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/+2.0 Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/+2.0 NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/+2.0 Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/+inf NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/+inf Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/+inf NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/+inf Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/-QNaN NT'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/-QNaN Tr'
 DC XL16'FFFF8B00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF -QNaN/+SNaN Tr'
 DC XL16'FFF8B000000000000000000000000000'
 DC CL48'MXDB NF -QNaN/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF -QNaN/+SNaN Tr'
 DC XL16'FFF8B000000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/-inf NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/-inf Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDB NF +SNaN/-inf NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF +SNaN/-inf Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/-2.0 NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/-2.0 Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDB NF +SNaN/-2.0 NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF +SNaN/-2.0 Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/-0 NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/-0 Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDB NF +SNaN/-0 NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF +SNaN/-0 Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/+0 NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/+0 Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDB NF +SNaN/+0 NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF +SNaN/+0 Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/+2.0 NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/+2.0 Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDB NF +SNaN/+2.0 NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF +SNaN/+2.0 Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/+inf NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/+inf Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDB NF +SNaN/+inf NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF +SNaN/+inf Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/-QNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/-QNaN Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDB NF +SNaN/-QNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF +SNaN/-QNaN Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDBR NF +SNaN/+SNaN Tr'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'MXDB NF +SNaN/+SNaN NT'
 DC XL16'7FFF8A00000000000000000000000000'
 DC CL48'MXDB NF +SNaN/+SNaN Tr'
 DC XL16'7FF0A000000000000000000000000000'
XBFPNFOT_NUM EQU (*-XBFPNFOT_GOOD)/64
*
*
XBFPNFFL_GOOD EQU *
 DC CL48'MXBR NF -inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF -inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF -inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF -2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF -0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF -0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF -0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +0/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +0/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +2.0/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +2.0/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +2.0/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +2.0/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +2.0/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +2.0/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +2.0/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +2.0/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +inf/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +inf/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +inf/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +inf/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +inf/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +inf/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +inf/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF +inf/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF -QNaN/-inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -QNaN/-2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -QNaN/-0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -QNaN/+0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -QNaN/+2.0 FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -QNaN/+inf FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -QNaN/-QNaN FPCR'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'MXBR NF -QNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +SNaN/-inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +SNaN/-2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +SNaN/-0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +SNaN/+0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +SNaN/+2.0 FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +SNaN/+inf FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +SNaN/-QNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
 DC CL48'MXBR NF +SNaN/+SNaN FPCR'
 DC XL16'00800000F800800000800000F8008000'
XBFPNFFL_NUM EQU (*-XBFPNFFL_GOOD)/64
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
         DC    A(LBFPNFOT)
         DC    A(LBFPNFOT_GOOD)
         DC    A(LBFPNFOT_NUM)
*
         DC    A(LBFPNFFL)
         DC    A(LBFPNFFL_GOOD)
         DC    A(LBFPNFFL_NUM)
*
         DC    A(XBFPNFOT)
         DC    A(XBFPNFOT_GOOD)
         DC    A(XBFPNFOT_NUM)
*
         DC    A(XBFPNFFL)
         DC    A(XBFPNFFL_GOOD)
         DC    A(XBFPNFFL_NUM)
*
VERIFLEN EQU   (*-VERIFTAB)/12    #of entries in verify table
                                                                EJECT
         END
