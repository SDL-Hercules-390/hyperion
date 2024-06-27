  TITLE 'bfp-001-divtoint: Test IEEE Divide To Integer'
***********************************************************************
*
*Testcase IEEE DIVIDE TO INTEGER
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
*                       bfp-001-divtoint.asm
*
*        This assembly-language source file is part of the
*        Hercules Binary Floating Point Validation Package
*                        by Stephen R. Orso
*
* Copyright 2016 by Stephen R Orso.
* Runtest *Compare dependency removed by Fish on 2022-03-08
* PADCSECT macro/usage removed by Fish on 2022-03-08
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
*
*Outstanding Issues:
* - 'A' versions of instructions are not tested.  Space for these added
*   results has not been allowed for in the results areas. Eight
*   additional results are needed per input pair.
* - Initial execution on real hardware shows no inexact / truncated on
*   underflow; not sure this case can be created on Add.  Finite
*   rounding mode test cases must be reviewed.
* - The quantum exception is not tested.  This is only available in the
*   'A' mode instructions, and only the finite tests will detect a
*   quantum trap.  This has implications for the test case selection
*   and the selection of the instruction used for the test.  Note: the
*   M4 rounding mode used with the 'A' instructions must be in the
*   range 8-15.
* - Note that the test case values selected for the rounding mode tests
*   will never trigger the quantum flag.
*
* - If Quantum exceptions can be created, they will be tested in the
*   Finite tests.
* - A fourth test run will perform pathlength validation on the M4
*   Rounding Mode tests, rather than run 16 additional tests on each of
*   8 (at present) rounding mode test pairs.  A pair of tests is
*   sufficient: a positive RNTE odd and a negative RNTE even.  (Or the
*   other way around.)
*
*
*
***********************************************************************
          SPACE 2
***********************************************************************
*
* Tests the following three conversion instructions
*   DIVIDE TO INTEGER (short BFP, RRE)
*   DIVIDE TO INTEGER (long BFP, RRE)
*
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R
* commands.
*
* Test Case Order
* 1) Short BFP basic tests, including traps and NaN propagation
* 2) Short BFP finite number tests, incl. partial and final results
* 3) Short BFP rounding, tests of quotient and remainder rounding
* 4) Long BFP basic tests, including traps and NaN propagation
* 5) Long BFP finite number tests, incl. partial and final results
* 6) Short BFP rounding, tests of quotient and remainder rounding
*
* Three input test sets are provided each for short and long
*   BFP inputs.  Test values are conceptually the same for each
*   precision, but the values differ between precisions.  Overflow,
*   for example, is triggered by different values in short and long,
*   but each test set includes overflow tests.
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
*
*  Note: for compatibility with the z/CMS test rig, do not change
*  or use R11, R14, or R15.  Everything else is fair game.
*
BFPDV2NT START 0
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
         JNE   PCNOTDTA       ..no, hardwait
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
         LA    R10,SHORTNF    Point to short BFP non-finite inputs
         BAS   R13,DIEBRNF    Divide to Integer short BFP non-finite
         LA    R10,SHORTF     Point to short BFP finite inputs
         BAS   R13,DIEBRF     Divide to Integer short BFP finites
         LA    R10,RMSHORTS   Point to short BFP rounding mode tests
         BAS   R13,DIEBRRM    Convert using all rounding mode options
*
         LA    R10,LONGNF     Point to long BFP non-finite inputs
         BAS   R13,DIDBRNF    Divide to Integer long BFP basic
         LA    R10,LONGF      Point to long BFP finite inputs
         BAS   R13,DIDBRF     Divide to Integer long BFP basic
         LA    R10,RMLONGS    Point to long  BFP rounding mode tests
         BAS   R13,DIDBRRM    Convert using all rounding mode options
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
         EJECT
***********************************************************************
*
* Perform Divide to Integer using provided short BFP input pairs.  This
* set of tests checks NaN propagation and operations on values that are
* not finite numbers.
*
* A pair of results is generated for each input: one with all
* exceptions non-trappable, and the second with all exceptions
* trappable.   The FPCR and condition code is stored for each result.
*
***********************************************************************
         SPACE 2
DIEBRNF  DS    0H            BFP Short non-finite values tests
         LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LE    FPR0,0(,R3)   Get short BFP dividend
         LE    FPR1,4(,R3)   Get short BFP divisor
         LZER  FPR2          Zero remainder register
         DIEBR FPR0,FPR2,FPR1,0 Div to Int FPR0/FPR1, M4=use FPCR
*                            Quotient in FPR2, remainder in FPR0
         STE   FPR0,0(,R7)   Store short BFP remainder
         STE   FPR2,4(,R7)   Store short BFP quotient
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,3(0,R8)    Store in last byte of FPCR
*
         LFPC  FPCREGTR      Set exceptions trappable
         LE    FPR0,0(,R3)   Get short BFP dividend
         LE    FPR1,4(,R3)   Get short BFP divisor
         LZER  FPR2          Zero remainder register
         DIEBR FPR0,FPR2,FPR1,0  Div to Int FPR0/FPR1, M4=use FPCR
*                            Quotient in FPR2, remainder in FPR0
         STE   FPR0,8(,R7)   Store short BFP remainder
         STE   FPR2,12(,R7)  Store short BFP quotient
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,7(,R8)     Store in last byte of FPCR
*
         LA    R3,8(,R3)     Point to next input value pair
         LA    R7,16(,R7)    Point to next quo&rem result value pair
         LA    R8,8(,R8)     Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Divide to Integer using provided short BFP input pairs.  This
* set of tests performs basic checks of Divide To Integer emulation
* where both inputs are finite non-zero numbers.
*
* Four results (six values) are generated for each input:
*  1) Divide to integer with all exceptions non-trappable (two values)
*  2) Multiply integer quotient by divisor, add remainder (one value)
*  3) Divide to integerwith all exceptions trappable (two values)
*  4) Multiply integer quotient by divisor, add remainder (one value)
*
* The FPCR and condition code is stored for each result.  Note: the
* Multiply and Add instruction does not set the condition code.
*
* Results two and four (multiply and add) validate the calculation
* of the integer quotient and remainder.
*
***********************************************************************
         SPACE 2
DIEBRF   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LE    FPR0,0(,R3)   Get short BFP dividend
         LE    FPR1,4(,R3)   Get short BFP divisor
         LZER  FPR2          Zero remainder register
         DIEBR FPR0,FPR2,FPR1,0  Div to Int FPR0/FPR1, M4=use FPCR
*                            Quotient in FPR2, remainder in FPR0
         STE   FPR0,0(,R7)   Store short BFP remainder
         STE   FPR2,4(,R7)   Store short BFP quotient
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,3(,R8)     Store in last byte of FPCR
*
* FPR1 still has divisor, FPR0 has remainder, FPR2 has integer quotient
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         MAEBR FPR0,FPR2,FPR1 Multiply and add to recreate inputs
*                            Sum of product and remainder in FPR0
         STE   FPR0,8(,R7)   Store short BFP product-sum
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,7(,R8)     Store in last byte of FPCR
*
         LFPC  FPCREGTR      Set exceptions trappable
         LE    FPR0,0(,R3)   Get short BFP dividend
         LE    FPR1,4(,R3)   Get short BFP divisor
         LZER  FPR2          Zero remainder register
         DIEBR FPR0,FPR2,FPR1,0 Div to Int FPR0/FPR1, M4=use FPCR
*                            Quotient in FPR2, remainder in FPR0
         STE   FPR0,16(,R7)  Store short BFP remainder
         STE   FPR2,20(,R7)  Store short BFP quotient
         STFPC 8(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,11(,R8)    Store in last byte of FPCR
*
* FPR1 still has divisor, FPR0 has remainder, FPR2 has integer quotient
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         MAEBR FPR0,FPR2,FPR1 Multiply and add to recreate inputs
*                            Sum of product and remainder in FPR0
         STE   FPR0,24(,R7)  Store short BFP remainder
         STFPC 12(R8)        Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,15(,R8)    Store in last byte of FPCR

*
         LA    R3,8(,R3)     Point to next input value pair
         LA    R7,32(,R7)    Point to next quo&rem result value pair
         LA    R8,16(,R8)    Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* The next tests operate on finite number input pairs and exhastively
* test rounding modes and partial and final results.
*
* Two rounding modes can be specified for each operation: one for the
* quotient, specified in the M4 field, and the second for the
* remainder, specified in the FPCR.
*
* Because six unique rounding modes can be specified in for the
* quotient and four for the remainder, there are a lot of results that
* need to be evaluated.  Note: M4 rounding mode zero, use FPCR rounding
* mode, is not tested because it duplicates one of the six explicit
* M4 rounding modes.  Which one depends on the current FPCR setting.
*
* The M4 rounding mode is assembled into the instruction.  Back in the
* day, this would be a perfect candidate for an Execute instructoin.
* But the M4 field is not located such that it can be modified by
* an Execute instruction.  So we will still use Execute, but only to
* select one of six DIEBR instructions for execution.  That way we can
* build an outer loop to iterate through the four FPCR modes, and an
* inner loop to use each of the six M4-specified rounding modes.
*
***********************************************************************
         SPACE 2
DIEBRRM  LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         XR    R1,R1         Zero register 1 for use in IC/STC/indexing
         BASR  R12,0         Set top of test case loop

         LA    R5,FPCMCT     Get count of FPC modes to be tested
         BASR  R9,0          Set top of rounding mode outer loop
*
* Update model FPC register settings with the BFP rounding mode for
* this iteration of the loop.
*
         IC    R1,FPCMODES-L'FPCMODES(R5)  Get next FPC mode
*
         LA    R4,D2IMCT     Get count of M4 modes to be tested
         BASR  R6,0          Set top of rounding mode inner loop
*
* Non-trap execution of the instruction.
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         IC    R1,FPCMODES-L'FPCMODES(R5)  Get next FPC mode
         SRNMB 0(R1)         Set FPC Rounding Mode
         LE    FPR0,0(,R3)   Get short BFP dividend
         LE    FPR1,4(,R3)   Get short BFP divisor
         LZER  FPR2          Zero remainder register
         IC    R1,D2IMODES-L'D2IMODES(R4)  Get index DIEBR inst table
         EX    0,DIEBRTAB(R1) Execute Divide to Integer
         STE   FPR0,0(,R7)   Store short BFP remainder
         STE   FPR2,4(,R7)   Store short BFP quotient
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,3(,R8)     Store in last byte of FPCR
*
* Trap-enabled execution of the instruction.
*
         LFPC  FPCREGTR      Set exceptions trappable, clear flags
         IC    R1,FPCMODES-L'FPCMODES(R5)  Get next FPC mode
         SRNMB 0(R1)         Set FPC Rounding Mode
         LE    FPR0,0(,R3)   Get short BFP dividend
         LE    FPR1,4(,R3)   Get short BFP divisor
         LZER  FPR2          Zero remainder register
         IC    R1,D2IMODES-L'D2IMODES(R4)  Get index DIEBR inst table
         EX    0,DIEBRTAB(R1) Execute Divide to Integer
         STE   FPR0,8(,R7)   Store short BFP remainder
         STE   FPR2,12(,R7)  Store short BFP quotient
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,7(,R8)     Store in last byte of FPCR
*
         LA    R7,16(,R7)    Point to next quo&rem result value pair
         LA    R8,8(,R8)     Point to next FPCR result area
*
         BCTR  R4,R6         Iterate inner loop
*
* End of M4 modes to be tested.
*
         BCTR  R5,R9         Iterate outer loop
*
* End of FPC modes to be tested with each M4 mode.  Advance to
* next test case.
*
         LA    R3,8(,R3)     Point to next input value pair
         BCTR  R2,R12        Divide next input value lots of times
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Divide to Integer using provided long BFP input pairs.  This
* set of tests checks NaN propagation and operations on values that are
* not finite numbers.
*
* A pair of results is generated for each input: one with all
* exceptions non-trappable, and the second with all exceptions
* trappable.   The FPCR and condition code is stored for each result.
*
***********************************************************************
         SPACE 2
DIDBRNF  LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LD    FPR0,0(,R3)   Get long BFP dividend
         LD    FPR1,8(,R3)   Get long BFP divisor
         LZDR  FPR2          Zero remainder register
         DIDBR FPR0,FPR2,FPR1,0 Div to Int FPR0/FPR1, M4=use FPCR
*                            Quotient in FPR2, remainder in FPR0
         STD   FPR0,0(,R7)   Store long BFP remainder
         STD   FPR2,8(,R7)   Store long BFP quotient
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,3(,R8)     Store in last byte of FPCR
*
         LFPC  FPCREGTR      Set exceptions trappable
         LD    FPR0,0(,R3)   Get long BFP dividend
         LD    FPR1,8(,R3)   Get long BFP divisor
         LZDR  FPR2          Zero remainder register
         DIDBR FPR0,FPR2,FPR1,0 Div to Int FPR0/FPR1, M4=use FPCR
*                            Quotient in FPR2, remainder in FPR0
         STD   FPR0,16(,R7)  Store long BFP remainder
         STD   FPR2,24(,R7)  Store long BFP quotient
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,7(,R8)     Store in last byte of FPCR
*
         LA    R3,16(,R3)    Point to next input value pair
         LA    R7,32(,R7)    Point to next quo&rem result value pair
         LA    R8,8(,R8)     Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Perform Divide to Integer using provided long BFP input pairs.  This
* set of tests performs basic checks of Divide To Integer emulation
* where both inputs are finite non-zero numbers.
*
* Four results (six values) are generated for each input:
*  1) Divide to integer with all exceptions non-trappable (two values)
*  2) Multiply integer quotient by divisor, add remainder (one value)
*  3) Divide to integerwith all exceptions trappable (two values)
*  4) Multiply integer quotient by divisor, add remainder (one value)
*
* The FPCR and condition code is stored for each result.  Note: the
* Multiply and Add instruction does not set the condition code.
*
* Results two and four (multiply and add) validate the calculation
* of the integer quotient and remainder.
*
***********************************************************************
         SPACE 2
DIDBRF   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LD    FPR0,0*32+0(,R3) Get long BFP dividend
         LD    FPR1,0*32+8(,R3) Get long BFP divisor
         LZDR  FPR2            Zero remainder register
         DIDBR FPR0,FPR2,FPR1,0 Div to Int FPR0/FPR1, M4=use FPCR
*                            Quotient in FPR2, remainder in FPR0
         STD   FPR0,0*32+0(,R7) Store long BFP remainder
         STD   FPR2,0*32+8(,R7) Store long BFP quotient
         STFPC 0*4(R8)       Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,0*4+3(,R8) Store in last byte of FPCR
*
* FPR1 still has divisor, FPR0 has remainder, FPR2 has integer quotient
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         MADBR FPR0,FPR2,FPR1 Multiply and add to recreate inputs
*                            Sum of product and remainder in FPR0
         STD   FPR0,0*32+16(,R7) Store short BFP product-sum
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,1*4+3(,R8) Store in last byte of FPCR
*
         LFPC  FPCREGTR      Set exceptions trappable
         LD    FPR0,0(,R3)   Get long BFP dividend
         LD    FPR1,8(,R3)   Get long BFP divisor
         LZDR  FPR2          Zero remainder register
         DIDBR FPR0,FPR2,FPR1,0 Div to Int FPR0/FPR1, M4=use FPCR
*                            Quotient in FPR2, remainder in FPR0
         STD   FPR0,1*32+0(,R7) Store long BFP remainder
         STD   FPR2,1*32+8(,R7) Store long BFP quotient
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,2*4+3(,R8) Store in last byte of FPCR
*
* FPR1 still has divisor, FPR0 has remainder, FPR2 has integer quotient
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         MADBR FPR0,FPR2,FPR1 Multiply and add to recreate inputs
*                            Sum of product and remainder in FPR0
         STD   FPR0,1*32+16(,R7) Store short BFP product-sum
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,3*4+3(,R8) Store in last byte of FPCR
*
         LA    R3,16(,R3)    Point to next input value pair
         LA    R7,64(,R7)    Point to next quo&rem result value pair
         LA    R8,16(,R8)    Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* The next tests operate on finite number input pairs and exhastively
* test rounding modes and partial and final results.
*
* Two rounding modes can be specified for each operation: one for the
* quotient, specified in the M4 field, and the second for the
* remainder, specified in the FPCR.
*
* Because six unique rounding modes can be specified in for the
* quotient and four for the remainder, there are a lot of results that
* need to be evaluated.  Note: M4 rounding mode zero, use FPCR rounding
* mode, is not tested because it duplicates one of the six explicit
* M4 rounding modes.  Which one depends on the current FPCR setting.
*
* The M4 rounding mode is assembled into the instruction.  Back in the
* day, this would be a perfect candidate for an Execute instructoin.
* But the M4 field is not located such that it can be modified by
* an Execute instruction.  So we will still use Execute, but only to
* select one of six DIEBR instructions for execution.  That way we can
* build an outer loop to iterate through the four FPCR modes, and an
* inner loop to use each of the six M4-specified rounding modes.
*
***********************************************************************
         SPACE 2
DIDBRRM  LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         XR    R1,R1         Zero register 1 for use in IC/STC/indexing
         BASR  R12,0         Set top of test case loop

         LA    R5,FPCMCT     Get count of FPC modes to be tested
         BASR  R9,0          Set top of rounding mode outer loop
*
* Update model FPC register settings with the BFP rounding mode for
* this iteration of the loop.
*
         IC    R1,FPCMODES-L'FPCMODES(R5)  Get next FPC mode
         STC   R1,FPCREGNT+3 Update non-trap register settings
         STC   R1,FPCREGTR+3 Update trap-enabled register settings
*
         LA    R4,D2IMCT     Get count of M4 modes to be tested
         BASR  R6,0          Set top of rounding mode inner loop
*
* Non-trap execution of the instruction.
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         IC    R1,FPCMODES-L'FPCMODES(R5)  Get next FPC mode
         SRNMB 0(R1)         Set FPC Rounding Mode
         LD    FPR0,0(,R3)   Get short BFP dividend
         LD    FPR1,8(,R3)   Get short BFP divisor
         LZDR  FPR2          Zero remainder register
         IC    R1,D2IMODES-L'D2IMODES(R4)  Get index DIEBR inst table
         EX    0,DIDBRTAB(R1) Execute Divide to Integer
         STD   FPR0,0(,R7)   Store short BFP remainder
         STD   FPR2,8(,R7)   Store short BFP quotient
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,3(,R8)     Store in last byte of FPCR
*
* Trap-enabled execution of the instruction.
*
         LFPC  FPCREGTR      Set exceptions trappable, clear flags
         IC    R1,FPCMODES-L'FPCMODES(R5)  Get next FPC mode
         SRNMB 0(R1)         Set FPC Rounding Mode
         LD    FPR0,0(,R3)   Get short BFP dividend
         LD    FPR1,8(,R3)   Get short BFP divisor
         LZER  FPR2          Zero remainder register
         IC    R1,D2IMODES-L'D2IMODES(R4)  Get index DIEBR inst table
         EX    0,DIDBRTAB(R1) Execute Divide to Integer
         STD   FPR0,16(,R7)  Store short BFP remainder
         STD   FPR2,24(,R7)  Store short BFP quotient
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0, dump prog mask
         STC   R0,7(,R8)     Store in last byte of FPCR
*
         LA    R7,32(,R7)    Point to next quo&rem result value pair
         LA    R8,8(,R8)     Point to next FPCR result area
*
         BCTR  R4,R6         Iterate inner loop
*
* End of M4 modes to be tested.
*
         BCTR  R5,R9         Iterate outer loop
*
* End of FPC modes to be tested with each M4 mode.  Advance to
* next test case.
*
         LA    R3,16(,R3)     Point to next input value pair
         BCTR  R2,R12        Divide next input value lots of times
*
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Tables and indices used to exhaustively test remainder and quotient
* rounding modes.
*
* The Execute instruction with an appropriate index * is used to
* execute the correct DIEBR/DIDBR instruction.  Because * the quotient
* rounding mode is encoded in the DIxBR instruction in the wrong place
* to use Execute to dynamically modify the rounding mode, we will just
* use it to select the correct instruction.
*
* The Set BFP Rounding Mode does allow specification of the FPC
* rounding mode as an address, so we shall index into a table of
* BFP rounding modes without bothering with Execute.
*
***********************************************************************
         SPACE 2
*
* Rounding modes that may be set in the FPCR.  The FPCR controls
* rounding of the quotient.  The same table is used for both DIEBR
* and DIDBR instruction testing.
*
* These are indexed directly by the loop counter, which counts down.
* So the modes are listed in reverse order here.
*
FPCMODES DS    0C
         DC    AL1(7)              RFS, Round for shorter precision
         DC    AL1(3)              RM, Round to -infinity
         DC    AL1(2)              RP, Round to +infinity
         DC    AL1(1)              RZ, Round to zero
***      DC    AL1(0)              RNTE, Round to Nearest, ties to even
FPCMCT   EQU   *-FPCMODES          Count of FPC Modes to be tested
*
* Table of indices into table of DIDBR/DIEBR instructions.  The table
* is used for both DIDBR and DIEBR, with the table value being used
* as the index register of an Execute instruction that points to
* either the DIDBR or DIEBR instruction list.
*
* These are indexed directly by the loop counter, which counts down.
* So the instruction indices are listed in reverse order here.
*
D2IMODES DS    0C
         DC    AL1(6*4)            RM, Round to -infinity
         DC    AL1(5*4)            RP, Round to +infinity
         DC    AL1(4*4)            RZ, Round to zero
         DC    AL1(3*4)            RNTE, Round to Nearest, ties to even
         DC    AL1(2*4)            RFS, Round for Shorter Precision
         DC    AL1(1*4)            RNTA, Round to Nearest, ties away
***      DC    AL1(0*4)            Use FPCR rounding mode
D2IMCT   EQU   *-D2IMODES          Count of M4 Modes to be tested
*
* List of DIEBR instructions, each with a different rounding mode.
* These are Execute'd by the rounding mode test routing using an index
* obtained from the D2IMODES table above.
*
* This table and the DIDBRTAB table below should remain in the same
* sequence, or you'll be scratching your head keeping the result order
* straight between short and long results.
*
DIEBRTAB DS    0F                  Table of DIEBR instructions
         DIEBR FPR0,FPR2,FPR1,0 Div to Int FPR0/FPR1, M4=use FPCR
* Above is not used
         DIEBR FPR0,FPR2,FPR1,1 Div to Int FPR0/FPR1, M4=RNTA
         DIEBR FPR0,FPR2,FPR1,3 Div to Int FPR0/FPR1, M4=RFS
         DIEBR FPR0,FPR2,FPR1,4 Div to Int FPR0/FPR1, M4=RNTE
         DIEBR FPR0,FPR2,FPR1,5 Div to Int FPR0/FPR1, M4=RZ
         DIEBR FPR0,FPR2,FPR1,6 Div to Int FPR0/FPR1, M4=RP
         DIEBR FPR0,FPR2,FPR1,7 Div to Int FPR0/FPR1, M4=RM
*
* List of DIDBR instructions, each with a different rounding mode.
* These are Execute'd by the rounding mode test routing using an index
* obtained from the D2IMODES table above.
*
* This table and the DIEBRTAB table above should remain in the same
* sequence, or you'll be scratching your head keeping the result order
* straight between short and long results.
*
DIDBRTAB DS    0F                  Table of DIDBR instructions
         DIDBR FPR0,FPR2,FPR1,0 Div to Int FPR0/FPR1, M4=use FPCR
* Above is not used
         DIDBR FPR0,FPR2,FPR1,1 Div to Int FPR0/FPR1, M4=RNTA
         DIDBR FPR0,FPR2,FPR1,3 Div to Int FPR0/FPR1, M4=RFS
         DIDBR FPR0,FPR2,FPR1,4 Div to Int FPR0/FPR1, M4=RNTE
         DIDBR FPR0,FPR2,FPR1,5 Div to Int FPR0/FPR1, M4=RZ
         DIDBR FPR0,FPR2,FPR1,6 Div to Int FPR0/FPR1, M4=RP
         DIDBR FPR0,FPR2,FPR1,7 Div to Int FPR0/FPR1, M4=RM
*
         EJECT
***********************************************************************
*
* Short integer test data sets for Divide to Integer testing.
*
* Each test data set member consists of two values, the dividend and
* the divisor, in that order.
*
* The first test data set is used for tests of basic functionality,
* NaN propagation, and results from operations involving other than
* finite numbers.
*
* The secondd test data set is used for testing boundary conditions
* using two finite non-zero values.  Each possible condition code
* and type of result (normal, scaled, etc) is created by members of
* this test data set.
*
* The third test data set is used for exhaustive testing of final
* results across the panoply of rounding mode combinations available
* for Divide to Integer (five for the remainder, seven for the
* quotient).
*
***********************************************************************
         SPACE 2
*
* First input test data set, to test operations using non-finite or
* zero inputs.  Member values chosen to validate part 1 of Figure 19-21
* on page 19-29 of SA22-7832-10.
*
SBFPNFIN DS    0F                Inputs for short BFP non-finite tests
*
* NaN propagation tests  (Tests 1-4)
*
         DC    X'7F8A0000'         SNaN
         DC    X'7F8B0000'         SNaN
*
         DC    X'7FCA0000'         QNaN
         DC    X'7FCB0000'         QNaN
*
         DC    X'40000000'         Finite number
         DC    X'7FCB0000'         QNaN
*
         DC    X'7FCA0000'         QNaN
         DC    X'7F8B0000'         SNaN
*
* Dividend is -inf  (Tests 5-10)
*
         DC    X'FF800000'         -inf
         DC    X'FF800000'         -inf
*
         DC    X'FF800000'         -inf
         DC    X'C0000000'         -2.0
*
         DC    X'FF800000'         -inf
         DC    X'80000000'         -0
*
         DC    X'FF800000'         -inf
         DC    X'00000000'         +0
*
         DC    X'FF800000'         -inf
         DC    X'40000000'         +2.0
*
         DC    X'FF800000'         -inf
         DC    X'7F800000'         +inf
*
* Dividend is +inf  (Tests 11-16)
*
         DC    X'7F800000'         +inf
         DC    X'FF800000'         -inf
*
         DC    X'7F800000'         +inf
         DC    X'C0000000'         -2.0
*
         DC    X'7F800000'         +inf
         DC    X'80000000'         -0
*
         DC    X'7F800000'         +inf
         DC    X'00000000'         +0
*
         DC    X'7F800000'         +inf
         DC    X'40000000'         +2.0
*
         DC    X'7F800000'         +inf
         DC    X'7F800000'         +inf
*
* Divisor is -0.  (+/-inf dividend tested above)
*                 (Tests 17-20)
*
         DC    X'C0000000'         -2.0
         DC    X'80000000'         -0
*
         DC    X'80000000'         -0
         DC    X'80000000'         -0
*
         DC    X'00000000'         +0
         DC    X'80000000'         -0
*
         DC    X'40000000'         +2.0
         DC    X'80000000'         -0
*
* Divisor is +0.  (+/-inf dividend tested above)
*                 (Tests 21-24)
*
         DC    X'C0000000'         -2.0
         DC    X'00000000'         +0
*
         DC    X'80000000'         -0
         DC    X'00000000'         +0
*
         DC    X'00000000'         +0
         DC    X'00000000'         +0
*
         DC    X'40000000'         +2.0
         DC    X'00000000'         +0
*
* Divisor is -inf.  (+/-inf dividend tested above)
*                 (Tests 25-28)
*
         DC    X'C0000000'         -2.0
         DC    X'FF800000'         -inf
*
         DC    X'80000000'         -0
         DC    X'FF800000'         -inf
*
         DC    X'00000000'         +0
         DC    X'FF800000'         -inf
*
         DC    X'40000000'         +2.0
         DC    X'FF800000'         -inf
*
* Divisor is +inf.  (+/-inf dividend tested above)
*                 (Tests 29-32)
*
         DC    X'C0000000'         -2.0
         DC    X'7F800000'         +inf
*
         DC    X'80000000'         -0
         DC    X'7F800000'         +inf
*
         DC    X'00000000'         +0
         DC    X'7F800000'         +inf
*
         DC    X'40000000'         +2.0
         DC    X'7F800000'         +inf
*
SBFPNFCT EQU   (*-SBFPNFIN)/4/2     Count of short BFP in list
         SPACE 3
***********************************************************************
*
* Second input test data set.  These are finite pairs intended to
* test all combinations of finite values and results (final
* results due to remainder zero, final results due to quotient
* within range, and partial results.
*
***********************************************************************
         SPACE 2
SBFPIN   DS    0F                Inputs for short BFP finite tests
*
* Dividend and Divisor are both finite numbers.
*
* Remainder tests from SA22-7832-10, Figure 19-7 on page 19-6
* (Finite tests 1-16; negative divisor)
*
         DC    X'C1000000'         -8
         DC    X'C0800000'         -4
*
         DC    X'C0E00000'         -7
         DC    X'C0800000'         -4
*
         DC    X'C0C00000'         -6
         DC    X'C0800000'         -4
*
         DC    X'C0A00000'         -5
         DC    X'C0800000'         -4
*
         DC    X'C0800000'         -4
         DC    X'C0800000'         -4
*
         DC    X'C0400000'         -3
         DC    X'C0800000'         -4
*
         DC    X'C0000000'         -2
         DC    X'C0800000'         -4
*
         DC    X'BF800000'         -1
         DC    X'C0800000'         -4
*
*  The +/- zero - +/- zero cases are handled above and skipped here
*
         DC    X'3F800000'         +1
         DC    X'C0800000'         -4
*
         DC    X'40000000'         +2
         DC    X'C0800000'         -4
*
         DC    X'40400000'         +3
         DC    X'C0800000'         -4
*
         DC    X'40800000'         +4
         DC    X'C0800000'         -4
*
         DC    X'40A00000'         +5
         DC    X'C0800000'         -4
*
         DC    X'40C00000'         +6
         DC    X'C0800000'         -4
*
         DC    X'40E00000'         +7
         DC    X'C0800000'         -4
*
         DC    X'41000000'         +8
         DC    X'C0800000'         -4
*
* Finite tests 17-32; positive divisor
*
         DC    X'C1000000'         -8
         DC    X'40800000'         +4
*
         DC    X'C0E00000'         -7
         DC    X'40800000'         +4
*
         DC    X'C0C00000'         -6
         DC    X'40800000'         +4
*
         DC    X'C0A00000'         -5
         DC    X'40800000'         +4
*
         DC    X'C0800000'         -4
         DC    X'40800000'         +4
*
         DC    X'C0400000'         -3
         DC    X'40800000'         +4
*
         DC    X'C0000000'         -2
         DC    X'40800000'         +4
*
         DC    X'3F800000'         -1
         DC    X'40800000'         +4
*
         DC    X'3F800000'         +1
         DC    X'40800000'         +4
*
         DC    X'40000000'         +2
         DC    X'40800000'         +4
*
         DC    X'40400000'         +3
         DC    X'40800000'         +4
*
         DC    X'40800000'         +4
         DC    X'40800000'         +4
*
         DC    X'40A00000'         +5
         DC    X'40800000'         +4
*
         DC    X'40C00000'         +6
         DC    X'40800000'         +4
*
         DC    X'40E00000'         +7
         DC    X'40800000'         +4
*
         DC    X'41000000'         +8
         DC    X'40800000'         +4
*
* Finite value boundary condition tests
*  Tests 17-22
*
         DC    X'42200000'         +40.0
         DC    X'C1100000'         -9.0
*
* Following forces quotient overflow, remainder zero.
* Final result, scaled quotient, cc1
         DC    X'7F7FFFFF'         +maxvalue
         DC    X'00000001'         +minvalue (tiny)
*
         DC    X'00FFFFFF'         near +minvalue normal
         DC    X'00FFFFFE'         almost above
*
* Following forces partial results without quotient overflow
* Partial result, scaled quotient, normal remainder, cc2
         DC    X'4C800000'         +2^26th
         DC    X'40400000'         +3.0
* Expected results from above case:  remainder < 3, quotient mismatch
* z12 actual results: remainder 4, quotient match.
*
* Following forces zero quotient, remainder = divisor.
*
         DC    X'40100000'         +2.25
         DC    X'41200000'         +10
*
* Following five tests force quotient overflow.  Four have non-zero
* Remainder.  All five return partial results.
*
* Note: +minvalue+11 is the smallest divisor that generates a
* remainder.
*
         DC    X'7F7FFFFF'         +maxvalue
         DC    X'0000000B'         +minvalue + 11 (tiny)
*
         DC    X'7F7FFFFE'         +maxvalue
         DC    X'0000000A'         +minvalue + 11 (tiny)
*
         DC    X'7F7FFFFF'         +maxvalue
         DC    X'0000000C'         +minvalue + 11 (tiny)
*
         DC    X'7F7FFFFF'         +maxvalue
         DC    X'00000013'         +minvalue + 11 (tiny)
*
         DC    X'7F7FFFFF'         +maxvalue
         DC    X'3F000000'         +0.5
*
         DC    X'40400000'         +3
         DC    X'40000000'         +2
*
SBFPCT   EQU   (*-SBFPIN)/4/2     Count of short BFP in list
         SPACE 3
***********************************************************************
*
* Third input test data set.  These are finite pairs intended to
* test all combinations of rounding mode for the quotient and the
* remainder.
*
* The quotient/remainder pairs are for Round to Nearest, Ties to Even.
* Other rounding modes have different results.
*
***********************************************************************
         SPACE 2
SBFPINRM DS    0F                Inputs for short BFP rounding testing
         DC    X'C1980000'         -19 / 0.5 = -9.5, -9 rem +1
         DC    X'40000000'         ...+2.0
         DC    X'C1300000'         -11 / 0.5 = -5.5, -5 rem +1
         DC    X'40000000'         ...+2.0
         DC    X'C0A00000'         -5 / 0.5 = -2.5
         DC    X'40000000'         ...+2.0
         DC    X'C0400000'         -3 / 0.5 = -1.5
         DC    X'40000000'         ...+2.0
         DC    X'BF800000'         -1 / 0.5 = -0.5
         DC    X'40000000'         ...+2.0
         DC    X'3F800000'         +1 / 0.5 = +0.5
         DC    X'40000000'         ...+2.0
         DC    X'40400000'         +3 / 0.5 = +1.5
         DC    X'40000000'         ...+2.0
         DC    X'40A00000'         +5 / 0.5 = +2.5
         DC    X'40000000'         ...+2.0
         DC    X'41300000'         +11 / 0.5 = +5.5
         DC    X'40000000'         ...+2.0
         DC    X'41980000'         +19 / 0.5 = +9.5
         DC    X'40000000'         ...+2.0
         DC    X'40000000'         2 / 2 = 1
         DC    X'40000000'         ...+2.0
         DC    X'40400000'         +3 / 5 = +0.6, 0 rem 3
         DC    X'40A00000'         ...+5.0
SBFPRMCT EQU   (*-SBFPINRM)/4/2   Count of short BFP rounding tests
         EJECT
***********************************************************************
*
* Long integer test data sets for Divide to Integer testing.
*
* Each test data set member consists of two values, the dividend and
* the divisor, in that order.
*
* The first test data set is used for tests of basic functionality,
* NaN propagation, and results from operations involving other than
* finite numbers.
*
* The secondd test data set is used for testing boundary conditions
* using two finite non-zero values.  Each possible condition code
* and type of result (normal, scaled, etc) is created by members of
* this test data set.
*
* The third test data set is used for exhaustive testing of final
* results across the panoply of rounding mode combinations available
* for Divide to Integer (five for the remainder, seven for the
* quotient).
*
***********************************************************************
*
LBFPNFIN DS    0F                Inputs for long BFP testing
*
* NaN propagation tests
*
         DC    X'7FF0A00000000000'         SNaN
         DC    X'7FF0B00000000000'         SNaN
*
         DC    X'7FF8A00000000000'         QNaN
         DC    X'7FF8B00000000000'         QNaN
*
         DC    X'4000000000000000'         Finite number
         DC    X'7FF8B00000000000'         QNaN
*
         DC    X'7FF8A00000000000'         QNaN
         DC    X'7FF0B00000000000'         SNaN
*
* Dividend is -inf
*
         DC    X'FFF0000000000000'         -inf
         DC    X'FFF0000000000000'         -inf
*
         DC    X'FFF0000000000000'         -inf
         DC    X'C000000000000000'         -2.0
*
         DC    X'FFF0000000000000'         -inf
         DC    X'8000000000000000'         -0
*
         DC    X'FFF0000000000000'         -inf
         DC    X'0000000000000000'         +0
*
         DC    X'FFF0000000000000'         -inf
         DC    X'4000000000000000'         +2.0
*
         DC    X'FFF0000000000000'         -inf
         DC    X'7FF0000000000000'         +inf
*
* Dividend is +inf
*
         DC    X'7FF0000000000000'         +inf
         DC    X'FFF0000000000000'         -inf
*
         DC    X'7FF0000000000000'         +inf
         DC    X'C000000000000000'         -2.0
*
         DC    X'7FF0000000000000'         +inf
         DC    X'8000000000000000'         -0
*
         DC    X'7FF0000000000000'         +inf
         DC    X'0000000000000000'         +0
*
         DC    X'7FF0000000000000'         +inf
         DC    X'4000000000000000'         +2.0
*
         DC    X'7FF0000000000000'         +inf
         DC    X'7FF0000000000000'         +inf
*
* Divisor is -0.  (+/-inf dividend tested above)
*
         DC    X'C000000000000000'         -2.0
         DC    X'8000000000000000'         -0
*
         DC    X'8000000000000000'         -0
         DC    X'8000000000000000'         -0
*
         DC    X'0000000000000000'         +0
         DC    X'8000000000000000'         -0
*
         DC    X'4000000000000000'         +2.0
         DC    X'8000000000000000'         -0
*
* Divisor is +0.  (+/-inf dividend tested above)
*
         DC    X'C000000000000000'         -2.0
         DC    X'0000000000000000'         +0
*
         DC    X'8000000000000000'         -0
         DC    X'0000000000000000'         +0
*
         DC    X'0000000000000000'         +0
         DC    X'0000000000000000'         +0
*
         DC    X'4000000000000000'         +2.0
         DC    X'0000000000000000'         +0
*
* Divisor is -inf.  (+/-inf dividend tested above)
*
         DC    X'C000000000000000'         -2.0
         DC    X'FFF0000000000000'         -inf
*
         DC    X'8000000000000000'         -0
         DC    X'FFF0000000000000'         -inf
*
         DC    X'0000000000000000'         +0
         DC    X'FFF0000000000000'         -inf
*
         DC    X'4000000000000000'         +2.0
         DC    X'FFF0000000000000'         -inf
*
* Divisor is +inf.  (+/-inf dividend tested above)
*
         DC    X'C000000000000000'         -2.0
         DC    X'7FF0000000000000'         +inf
*
         DC    X'8000000000000000'         -0
         DC    X'7FF0000000000000'         +inf
*
         DC    X'0000000000000000'         +0
         DC    X'7FF0000000000000'         +inf
*
         DC    X'4000000000000000'         +2.0
         DC    X'7FF0000000000000'         +inf
LBFPNFCT EQU   (*-LBFPNFIN)/8/2     Count of long BFP in list
         SPACE 3
***********************************************************************
*
* Second set of test inputs.  These are finite pairs intended to
* test all combinations of finite values and results (final
* results due to remainder zero, final results due to quotient
* within range, and partial results.
*
***********************************************************************
         SPACE 2
LBFPIN   DS    0F                Inputs for long BFP finite tests
*
* Dividend and Divisor are both finite numbers.
*
* Remainder tests from SA22-7832-10, Figure 19-7 on page 19-6
*                 (Finite tests 1-32)
*
         DC    X'C020000000000000'         -8
         DC    X'C010000000000000'         -4
*
         DC    X'C01C000000000000'         -7
         DC    X'C010000000000000'         -4
*
         DC    X'C018000000000000'         -6
         DC    X'C010000000000000'         -4
*
         DC    X'C014000000000000'         -5
         DC    X'C010000000000000'         -4
*
         DC    X'C010000000000000'         -4
         DC    X'C010000000000000'         -4
*
         DC    X'C008000000000000'         -3
         DC    X'C010000000000000'         -4
*
         DC    X'C000000000000000'         -2
         DC    X'C010000000000000'         -4
*
         DC    X'BFF0000000000000'         -1
         DC    X'C010000000000000'         -4
*
*  The +/- zero - +/- zero cases are handled above and skipped here
*
         DC    X'3FF0000000000000'         +1
         DC    X'C010000000000000'         -4
*
         DC    X'4000000000000000'         +2
         DC    X'C010000000000000'         -4
*
         DC    X'4008000000000000'         +3
         DC    X'C010000000000000'         -4
*
         DC    X'4010000000000000'         +4
         DC    X'C010000000000000'         -4
*
         DC    X'4014000000000000'         +5
         DC    X'C010000000000000'         -4
*
         DC    X'4018000000000000'         +6
         DC    X'C010000000000000'         -4
*
         DC    X'401C000000000000'         +7
         DC    X'C010000000000000'         -4
*
         DC    X'4020000000000000'         +8
         DC    X'C010000000000000'         -4
*
         DC    X'C020000000000000'         -8
         DC    X'4010000000000000'         +4
*
         DC    X'C01C000000000000'         -7
         DC    X'4010000000000000'         +4
*
         DC    X'C018000000000000'         -6
         DC    X'4010000000000000'         +4
*
         DC    X'C014000000000000'         -5
         DC    X'4010000000000000'         +4
*
         DC    X'C010000000000000'         -4
         DC    X'4010000000000000'         +4
*
         DC    X'C008000000000000'         -3
         DC    X'4010000000000000'         +4
*
         DC    X'C000000000000000'         -2
         DC    X'4010000000000000'         +4
*
         DC    X'3FF0000000000000'         -1
         DC    X'4010000000000000'         +4
*
         DC    X'3FF0000000000000'         +1
         DC    X'4010000000000000'         +4
*
         DC    X'4000000000000000'         +2
         DC    X'4010000000000000'         +4
*
         DC    X'4008000000000000'         +3
         DC    X'4010000000000000'         +4
*
         DC    X'4010000000000000'         +4
         DC    X'4010000000000000'         +4
*
         DC    X'4014000000000000'         +5
         DC    X'4010000000000000'         +4
*
         DC    X'4018000000000000'         +6
         DC    X'4010000000000000'         +4
*
         DC    X'401C000000000000'         +7
         DC    X'4010000000000000'         +4
*
         DC    X'4020000000000000'         +8
         DC    X'4010000000000000'         +4
**
* Dividend and Divisor are both finite numbers.
*                 (Tests 33-38)
*
         DC    X'4044000000000000'         +40.0
         DC    X'C022000000000000'         -9.0
*
* Following forces quotient overflow, remainder zero.
* Final result, scaled quotient, cc1
         DC    X'7FEFFFFFFFFFFFFF'         +maxvalue
         DC    X'0000000000000001'         +minvalue (tiny)
*
* Following forces quotient overflow, remainder non-zero.
* Partial result, scaled quotient, tiny remainder, cc3
* Note: +minvalue+2 is the smallest divisor that
* generates a non-zero remainder.
         DC    X'7FEFFFFFFFFFFFFF'         +maxvalue
         DC    X'0000000000000003'         +minvalue (tiny)
*
         DC    X'000FFFFFFFFFFFFF' near +minvalue normal
         DC    X'000FFFFFFFFFFFFE' almost above
*
* Following forces partial results without quotient overflow
* Partial result, scaled quotient, normal remainder, cc2
         DC    X'4370000000000000'         +2^56th
         DC    X'4008000000000000'         +3.0
* Expected results from above case:  remainder < 3, quotient mismatch
* z12 actual results: remainder 4, quotient match.
*
* Following forces zero quotient, remainder = divisor.
*
         DC    X'4002000000000000'         +2.25
         DC    X'4024000000000000'         +10
*
LBFPCT   EQU   (*-LBFPIN)/8/2     Count of long BFP in list
         SPACE 3
***********************************************************************
*
* Third input test data set.  These are finite pairs intended to
* test all combinations of rounding mode for the quotient and the
* remainder.
*
* The quotient/remainder pairs are for Round to Nearest, Ties to Even.
* Other rounding modes have different results.
*
***********************************************************************
         SPACE 2
LBFPINRM DS    0F
         DC    X'C023000000000000'         -9.5, -9 rem 1
         DC    X'4000000000000000'         +2
*
         DC    X'C016000000000000'         -5.5
         DC    X'4000000000000000'         +2
*
         DC    X'C004000000000000'         -2.5
         DC    X'4000000000000000'         +2
*
         DC    X'BFF8000000000000'         -1.5
         DC    X'4000000000000000'         +2
*
         DC    X'BFE0000000000000'         -0.5
         DC    X'4000000000000000'         +2
*
         DC    X'3FE0000000000000'         +0.5
         DC    X'4000000000000000'         +2
*
         DC    X'3FF8000000000000'         +1.5
         DC    X'4000000000000000'         +2
*
         DC    X'4004000000000000'         +2.5
         DC    X'4000000000000000'         +2
*
         DC    X'4016000000000000'         +5.5
         DC    X'4000000000000000'         +2
*
         DC    X'4023000000000000'         +9.5
         DC    X'4000000000000000'         +2
*
         DC    X'4000000000000000'         +2
         DC    X'4000000000000000'         +2
*
         DC    X'4008000000000000'         +3
         DC    X'4014000000000000'         +5
*
LBFPRMCT EQU   (*-LBFPINRM)/8/2   Count of long BFP rounding tests
         EJECT
***********************************************************************
*                 ACTUAL results saved here
***********************************************************************
*
*               Locations for ACTUAL results
*
*
SBFPNFOT EQU   STRTLABL+X'1000'    Integer short non-finite BFP results
*                                  ..room for 32 tests, 32 used
SBFPNFFL EQU   STRTLABL+X'1200'    FPCR flags and DXC from short BFP
*                                  ..room for 32 tests, 32 used
*
LBFPNFOT EQU   STRTLABL+X'1300'    Integer long non-finite BFP results
*                                  ..room for 32 tests, 32 used
LBFPNFFL EQU   STRTLABL+X'1700'    FPCR flags and DXC from long BFP
*                                  ..room for 32 tests, 32 used
*
SBFPRMO  EQU   STRTLABL+X'2000'    Short BFP rounding mode test results
*                                  ..Room for 20, 10 used.
SBFPRMOF EQU   STRTLABL+X'4000'    Short BFP rounding mode FPCR results
*                                  ..Room for 20, 10 used.
*
LBFPRMO  EQU   STRTLABL+X'5000'    Long BFP rounding mode test results
*                                  ..Room for 20, 10 used.
LBFPRMOF EQU   STRTLABL+X'9000'    Long BFP rounding mode FPCR results
*                                  ..Room for 20, 10 used.
*
SBFPOUT  EQU   STRTLABL+X'A000'    Integer short BFP finite results
*                                  ..room for 64 tests, 38 used
SBFPFLGS EQU   STRTLABL+X'A800'    FPCR flags and DXC from short BFP
*                                  ..room for 64 tests, 6 used
*
LBFPFLGS EQU   STRTLABL+X'AC00'    FPCR flags and DXC from long BFP
*                                  ..room for 64 tests, 6 used
LBFPOUT  EQU   STRTLABL+X'B000'    Integer long BFP finite results
*                                  ..room for 64 tests, 6 used
         EJECT
***********************************************************************
*                    EXPECTED results
***********************************************************************
*
         ORG   STRTLABL+X'C000'   (past end of actual results)
*
SBFPNFOT_GOOD EQU *
 DC CL48'DIEBR test 1 NaN'
 DC XL16'7FCA00007FCA00007F8A000000000000'
 DC CL48'DIEBR test 2 NaN'
 DC XL16'7FCA00007FCA00007FCA00007FCA0000'
 DC CL48'DIEBR test 3 NaN'
 DC XL16'7FCB00007FCB00007FCB00007FCB0000'
 DC CL48'DIEBR test 4 NaN'
 DC XL16'7FCB00007FCB00007FCA000000000000'
 DC CL48'DIEBR test 5 -inf dividend'
 DC XL16'7FC000007FC00000FF80000000000000'
 DC CL48'DIEBR test 6 -inf dividend'
 DC XL16'7FC000007FC00000FF80000000000000'
 DC CL48'DIEBR test 7 -inf dividend'
 DC XL16'7FC000007FC00000FF80000000000000'
 DC CL48'DIEBR test 8 -inf dividend'
 DC XL16'7FC000007FC00000FF80000000000000'
 DC CL48'DIEBR test 9 -inf dividend'
 DC XL16'7FC000007FC00000FF80000000000000'
 DC CL48'DIEBR test 10 -inf dividend'
 DC XL16'7FC000007FC00000FF80000000000000'
 DC CL48'DIEBR test 11 +inf dividend'
 DC XL16'7FC000007FC000007F80000000000000'
 DC CL48'DIEBR test 12 +inf dividend'
 DC XL16'7FC000007FC000007F80000000000000'
 DC CL48'DIEBR test 13 +inf dividend'
 DC XL16'7FC000007FC000007F80000000000000'
 DC CL48'DIEBR test 14 +inf dividend'
 DC XL16'7FC000007FC000007F80000000000000'
 DC CL48'DIEBR test 15 +inf dividend'
 DC XL16'7FC000007FC000007F80000000000000'
 DC CL48'DIEBR test 16 +inf dividend'
 DC XL16'7FC000007FC000007F80000000000000'
 DC CL48'DIEBR test 17 -0 divisor'
 DC XL16'7FC000007FC00000C000000000000000'
 DC CL48'DIEBR test 18 -0 divisor'
 DC XL16'7FC000007FC000008000000000000000'
 DC CL48'DIEBR test 19 -0 divisor'
 DC XL16'7FC000007FC000000000000000000000'
 DC CL48'DIEBR test 20 -0 divisor'
 DC XL16'7FC000007FC000004000000000000000'
 DC CL48'DIEBR test 21 +0 divisor'
 DC XL16'7FC000007FC00000C000000000000000'
 DC CL48'DIEBR test 22 +0 divisor'
 DC XL16'7FC000007FC000008000000000000000'
 DC CL48'DIEBR test 23 +0 divisor'
 DC XL16'7FC000007FC000000000000000000000'
 DC CL48'DIEBR test 24 +0 divisor'
 DC XL16'7FC000007FC000004000000000000000'
 DC CL48'DIEBR test 25 -inf divisor'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'DIEBR test 26 -inf divisor'
 DC XL16'80000000000000008000000000000000'
 DC CL48'DIEBR test 27 -inf divisor'
 DC XL16'00000000800000000000000080000000'
 DC CL48'DIEBR test 28 -inf divisor'
 DC XL16'40000000800000004000000080000000'
 DC CL48'DIEBR test 29 +inf divisor'
 DC XL16'C000000080000000C000000080000000'
 DC CL48'DIEBR test 30 +inf divisor'
 DC XL16'80000000800000008000000080000000'
 DC CL48'DIEBR test 31 +inf divisor'
 DC XL16'00000000000000000000000000000000'
 DC CL48'DIEBR test 32 +inf divisor'
 DC XL16'40000000000000004000000000000000'
SBFPNFOT_NUM EQU (*-SBFPNFOT_GOOD)/64
*
*
SBFPNFFL_GOOD EQU *
 DC CL48'DIEBR FPCR pair NaN 1-2'
 DC XL16'00800001F800800100000001F8000001'
 DC CL48'DIEBR FPCR pair NaN 4-4'
 DC XL16'00000001F800000100800001F8008001'
 DC CL48'DIEBR FPCR pair -inf 5-6'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIEBR FPCR pair -inf 7-8'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIEBR FPCR pair -inf 9-10'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIEBR FPCR pair +inf 11-12'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIEBR FPCR pair +inf 13-14'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIEBR FPCR pair +inf 15-16'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIEBR FPCR pair -0 17-18'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIEBR FPCR pair -0 19-20'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIEBR FPCR pair +0 21-22'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIEBR FPCR pair +0 23-24'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIEBR FPCR pair div -inf 25-26'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR FPCR pair div -inf 27-28'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR FPCR pair div +inf 29-30'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR FPCR pair div +inf 31-32'
 DC XL16'00000000F800000000000000F8000000'
SBFPNFFL_NUM EQU (*-SBFPNFFL_GOOD)/64
*
*
LBFPNFOT_GOOD EQU *
 DC CL48'DIDBR test 1a NaN'
 DC XL16'7FF8A000000000007FF8A00000000000'
 DC CL48'DIDBR test 1b NaN'
 DC XL16'7FF0A000000000000000000000000000'
 DC CL48'DIDBR test 2a NaN'
 DC XL16'7FF8A000000000007FF8A00000000000'
 DC CL48'DIDBR test 2b NaN'
 DC XL16'7FF8A000000000007FF8A00000000000'
 DC CL48'DIDBR test 3a NaN'
 DC XL16'7FF8B000000000007FF8B00000000000'
 DC CL48'DIDBR test 3b NaN'
 DC XL16'7FF8B000000000007FF8B00000000000'
 DC CL48'DIDBR test 4a NaN'
 DC XL16'7FF8B000000000007FF8B00000000000'
 DC CL48'DIDBR test 4b NaN'
 DC XL16'7FF8A000000000000000000000000000'
 DC CL48'DIDBR test 5a -inf dividend'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 5b -inf dividend'
 DC XL16'FFF00000000000000000000000000000'
 DC CL48'DIDBR test 6a -inf dividend'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 6b -inf dividend'
 DC XL16'FFF00000000000000000000000000000'
 DC CL48'DIDBR test 7a -inf dividend'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 7b -inf dividend'
 DC XL16'FFF00000000000000000000000000000'
 DC CL48'DIDBR test 8a -inf dividend'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 8b -inf dividend'
 DC XL16'FFF00000000000000000000000000000'
 DC CL48'DIDBR test 9a -inf dividend'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 9b -inf dividend'
 DC XL16'FFF00000000000000000000000000000'
 DC CL48'DIDBR test 10a -inf dividend'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 10b -inf dividend'
 DC XL16'FFF00000000000000000000000000000'
 DC CL48'DIDBR test 11a +inf dividend'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 11b +inf dividend'
 DC XL16'7FF00000000000000000000000000000'
 DC CL48'DIDBR test 12a +inf dividend'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 12b +inf dividend'
 DC XL16'7FF00000000000000000000000000000'
 DC CL48'DIDBR test 13a +inf dividend'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 13b +inf dividend'
 DC XL16'7FF00000000000000000000000000000'
 DC CL48'DIDBR test 14a +inf dividend'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 14b +inf dividend'
 DC XL16'7FF00000000000000000000000000000'
 DC CL48'DIDBR test 15a +inf dividend'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 15b +inf dividend'
 DC XL16'7FF00000000000000000000000000000'
 DC CL48'DIDBR test 16a +inf dividend'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 16b +inf dividend'
 DC XL16'7FF00000000000000000000000000000'
 DC CL48'DIDBR test 17a -0 divisor'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 17b -0 divisor'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'DIDBR test 18a -0 divisor'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 18b -0 divisor'
 DC XL16'80000000000000000000000000000000'
 DC CL48'DIDBR test 19a -0 divisor'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 19b -0 divisor'
 DC XL16'00000000000000000000000000000000'
 DC CL48'DIDBR test 20a -0 divisor'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 20b -0 divisor'
 DC XL16'40000000000000000000000000000000'
 DC CL48'DIDBR test 21a +0 divisor'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 21b +0 divisor'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'DIDBR test 22a +0 divisor'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 22b +0 divisor'
 DC XL16'80000000000000000000000000000000'
 DC CL48'DIDBR test 23a +0 divisor'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 23b +0 divisor'
 DC XL16'00000000000000000000000000000000'
 DC CL48'DIDBR test 24a +0 divisor'
 DC XL16'7FF80000000000007FF8000000000000'
 DC CL48'DIDBR test 24b +0 divisor'
 DC XL16'40000000000000000000000000000000'
 DC CL48'DIDBR test 25a -inf divisor'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'DIDBR test 25b -inf divisor'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'DIDBR test 26a -inf divisor'
 DC XL16'80000000000000000000000000000000'
 DC CL48'DIDBR test 26b -inf divisor'
 DC XL16'80000000000000000000000000000000'
 DC CL48'DIDBR test 27a -inf divisor'
 DC XL16'00000000000000008000000000000000'
 DC CL48'DIDBR test 27b -inf divisor'
 DC XL16'00000000000000008000000000000000'
 DC CL48'DIDBR test 28a -inf divisor'
 DC XL16'40000000000000008000000000000000'
 DC CL48'DIDBR test 28b -inf divisor'
 DC XL16'40000000000000008000000000000000'
 DC CL48'DIDBR test 29a +inf divisor'
 DC XL16'C0000000000000008000000000000000'
 DC CL48'DIDBR test 29b +inf divisor'
 DC XL16'C0000000000000008000000000000000'
 DC CL48'DIDBR test 30a +inf divisor'
 DC XL16'80000000000000008000000000000000'
 DC CL48'DIDBR test 30b +inf divisor'
 DC XL16'80000000000000008000000000000000'
 DC CL48'DIDBR test 31a +inf divisor'
 DC XL16'00000000000000000000000000000000'
 DC CL48'DIDBR test 31b +inf divisor'
 DC XL16'00000000000000000000000000000000'
 DC CL48'DIDBR test 32a +inf divisor'
 DC XL16'40000000000000000000000000000000'
 DC CL48'DIDBR test 32b +inf divisor'
 DC XL16'40000000000000000000000000000000'
LBFPNFOT_NUM EQU (*-LBFPNFOT_GOOD)/64
*
*
LBFPNFFL_GOOD EQU *
 DC CL48'DIDBR FPCR pair 1-2'
 DC XL16'00800001F800800100000001F8000001'
 DC CL48'DIDBR FPCR pair 3-4'
 DC XL16'00000001F800000100800001F8008001'
 DC CL48'DIDBR FPCR pair 5-6'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIDBR FPCR pair 7-8'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIDBR FPCR pair 9-10'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIDBR FPCR pair 11-12'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIDBR FPCR pair 13-14'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIDBR FPCR pair 15-16'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIDBR FPCR pair 17-18'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIDBR FPCR pair 19-20'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIDBR FPCR pair 21-22'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIDBR FPCR pair 23-24'
 DC XL16'00800001F800800100800001F8008001'
 DC CL48'DIDBR FPCR pair 25-26'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR FPCR pair 27-28'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR FPCR pair 29-30'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR FPCR pair 31-32'
 DC XL16'00000000F800000000000000F8000000'
LBFPNFFL_NUM EQU (*-LBFPNFFL_GOOD)/64
*
*
SBFPRMO_GOOD EQU *
 DC CL48'DIEBR Rounding case 1a'
 DC XL16'3F800000C12000003F800000C1200000'
 DC CL48'DIEBR Rounding case 1b'
 DC XL16'BF800000C1100000BF800000C1100000'
 DC CL48'DIEBR Rounding case 1c'
 DC XL16'3F800000C12000003F800000C1200000'
 DC CL48'DIEBR Rounding case 1d'
 DC XL16'BF800000C1100000BF800000C1100000'
 DC CL48'DIEBR Rounding case 1e'
 DC XL16'BF800000C1100000BF800000C1100000'
 DC CL48'DIEBR Rounding case 1f'
 DC XL16'3F800000C12000003F800000C1200000'
 DC CL48'DIEBR Rounding case 1g'
 DC XL16'3F800000C12000003F800000C1200000'
 DC CL48'DIEBR Rounding case 1h'
 DC XL16'BF800000C1100000BF800000C1100000'
 DC CL48'DIEBR Rounding case 1i'
 DC XL16'3F800000C12000003F800000C1200000'
 DC CL48'DIEBR Rounding case 1j'
 DC XL16'BF800000C1100000BF800000C1100000'
 DC CL48'DIEBR Rounding case 1k'
 DC XL16'BF800000C1100000BF800000C1100000'
 DC CL48'DIEBR Rounding case 1l'
 DC XL16'3F800000C12000003F800000C1200000'
 DC CL48'DIEBR Rounding case 1m'
 DC XL16'3F800000C12000003F800000C1200000'
 DC CL48'DIEBR Rounding case 1n'
 DC XL16'BF800000C1100000BF800000C1100000'
 DC CL48'DIEBR Rounding case 1o'
 DC XL16'3F800000C12000003F800000C1200000'
 DC CL48'DIEBR Rounding case 1p'
 DC XL16'BF800000C1100000BF800000C1100000'
 DC CL48'DIEBR Rounding case 1q'
 DC XL16'BF800000C1100000BF800000C1100000'
 DC CL48'DIEBR Rounding case 1r'
 DC XL16'3F800000C12000003F800000C1200000'
 DC CL48'DIEBR Rounding case 1s'
 DC XL16'3F800000C12000003F800000C1200000'
 DC CL48'DIEBR Rounding case 1t'
 DC XL16'BF800000C1100000BF800000C1100000'
 DC CL48'DIEBR Rounding case 1u'
 DC XL16'3F800000C12000003F800000C1200000'
 DC CL48'DIEBR Rounding case 1v'
 DC XL16'BF800000C1100000BF800000C1100000'
 DC CL48'DIEBR Rounding case 1w'
 DC XL16'BF800000C1100000BF800000C1100000'
 DC CL48'DIEBR Rounding case 1x'
 DC XL16'3F800000C12000003F800000C1200000'
 DC CL48'DIEBR Rounding case 2a'
 DC XL16'3F800000C0C000003F800000C0C00000'
 DC CL48'DIEBR Rounding case 2b'
 DC XL16'BF800000C0A00000BF800000C0A00000'
 DC CL48'DIEBR Rounding case 2c'
 DC XL16'3F800000C0C000003F800000C0C00000'
 DC CL48'DIEBR Rounding case 2d'
 DC XL16'BF800000C0A00000BF800000C0A00000'
 DC CL48'DIEBR Rounding case 2e'
 DC XL16'BF800000C0A00000BF800000C0A00000'
 DC CL48'DIEBR Rounding case 2f'
 DC XL16'3F800000C0C000003F800000C0C00000'
 DC CL48'DIEBR Rounding case 2g'
 DC XL16'3F800000C0C000003F800000C0C00000'
 DC CL48'DIEBR Rounding case 2h'
 DC XL16'BF800000C0A00000BF800000C0A00000'
 DC CL48'DIEBR Rounding case 2i'
 DC XL16'3F800000C0C000003F800000C0C00000'
 DC CL48'DIEBR Rounding case 2j'
 DC XL16'BF800000C0A00000BF800000C0A00000'
 DC CL48'DIEBR Rounding case 2k'
 DC XL16'BF800000C0A00000BF800000C0A00000'
 DC CL48'DIEBR Rounding case 2l'
 DC XL16'3F800000C0C000003F800000C0C00000'
 DC CL48'DIEBR Rounding case 2m'
 DC XL16'3F800000C0C000003F800000C0C00000'
 DC CL48'DIEBR Rounding case 2n'
 DC XL16'BF800000C0A00000BF800000C0A00000'
 DC CL48'DIEBR Rounding case 2o'
 DC XL16'3F800000C0C000003F800000C0C00000'
 DC CL48'DIEBR Rounding case 2p'
 DC XL16'BF800000C0A00000BF800000C0A00000'
 DC CL48'DIEBR Rounding case 2q'
 DC XL16'BF800000C0A00000BF800000C0A00000'
 DC CL48'DIEBR Rounding case 2r'
 DC XL16'3F800000C0C000003F800000C0C00000'
 DC CL48'DIEBR Rounding case 2s'
 DC XL16'3F800000C0C000003F800000C0C00000'
 DC CL48'DIEBR Rounding case 2t'
 DC XL16'BF800000C0A00000BF800000C0A00000'
 DC CL48'DIEBR Rounding case 2u'
 DC XL16'3F800000C0C000003F800000C0C00000'
 DC CL48'DIEBR Rounding case 2v'
 DC XL16'BF800000C0A00000BF800000C0A00000'
 DC CL48'DIEBR Rounding case 2w'
 DC XL16'BF800000C0A00000BF800000C0A00000'
 DC CL48'DIEBR Rounding case 2x'
 DC XL16'3F800000C0C000003F800000C0C00000'
 DC CL48'DIEBR Rounding case 3a'
 DC XL16'3F800000C04000003F800000C0400000'
 DC CL48'DIEBR Rounding case 3b'
 DC XL16'3F800000C04000003F800000C0400000'
 DC CL48'DIEBR Rounding case 3c'
 DC XL16'BF800000C0000000BF800000C0000000'
 DC CL48'DIEBR Rounding case 3d'
 DC XL16'BF800000C0000000BF800000C0000000'
 DC CL48'DIEBR Rounding case 3e'
 DC XL16'BF800000C0000000BF800000C0000000'
 DC CL48'DIEBR Rounding case 3f'
 DC XL16'3F800000C04000003F800000C0400000'
 DC CL48'DIEBR Rounding case 3g'
 DC XL16'3F800000C04000003F800000C0400000'
 DC CL48'DIEBR Rounding case 3h'
 DC XL16'3F800000C04000003F800000C0400000'
 DC CL48'DIEBR Rounding case 3i'
 DC XL16'BF800000C0000000BF800000C0000000'
 DC CL48'DIEBR Rounding case 3j'
 DC XL16'BF800000C0000000BF800000C0000000'
 DC CL48'DIEBR Rounding case 3k'
 DC XL16'BF800000C0000000BF800000C0000000'
 DC CL48'DIEBR Rounding case 3l'
 DC XL16'3F800000C04000003F800000C0400000'
 DC CL48'DIEBR Rounding case 3m'
 DC XL16'3F800000C04000003F800000C0400000'
 DC CL48'DIEBR Rounding case 3n'
 DC XL16'3F800000C04000003F800000C0400000'
 DC CL48'DIEBR Rounding case 3o'
 DC XL16'BF800000C0000000BF800000C0000000'
 DC CL48'DIEBR Rounding case 3p'
 DC XL16'BF800000C0000000BF800000C0000000'
 DC CL48'DIEBR Rounding case 3q'
 DC XL16'BF800000C0000000BF800000C0000000'
 DC CL48'DIEBR Rounding case 3r'
 DC XL16'3F800000C04000003F800000C0400000'
 DC CL48'DIEBR Rounding case 3s'
 DC XL16'3F800000C04000003F800000C0400000'
 DC CL48'DIEBR Rounding case 3t'
 DC XL16'3F800000C04000003F800000C0400000'
 DC CL48'DIEBR Rounding case 3u'
 DC XL16'BF800000C0000000BF800000C0000000'
 DC CL48'DIEBR Rounding case 3v'
 DC XL16'BF800000C0000000BF800000C0000000'
 DC CL48'DIEBR Rounding case 3w'
 DC XL16'BF800000C0000000BF800000C0000000'
 DC CL48'DIEBR Rounding case 3x'
 DC XL16'3F800000C04000003F800000C0400000'
 DC CL48'DIEBR Rounding case 4a'
 DC XL16'3F800000C00000003F800000C0000000'
 DC CL48'DIEBR Rounding case 4b'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'DIEBR Rounding case 4c'
 DC XL16'3F800000C00000003F800000C0000000'
 DC CL48'DIEBR Rounding case 4d'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'DIEBR Rounding case 4e'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'DIEBR Rounding case 4f'
 DC XL16'3F800000C00000003F800000C0000000'
 DC CL48'DIEBR Rounding case 4g'
 DC XL16'3F800000C00000003F800000C0000000'
 DC CL48'DIEBR Rounding case 4h'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'DIEBR Rounding case 4i'
 DC XL16'3F800000C00000003F800000C0000000'
 DC CL48'DIEBR Rounding case 4j'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'DIEBR Rounding case 4k'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'DIEBR Rounding case 4l'
 DC XL16'3F800000C00000003F800000C0000000'
 DC CL48'DIEBR Rounding case 4m'
 DC XL16'3F800000C00000003F800000C0000000'
 DC CL48'DIEBR Rounding case 4n'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'DIEBR Rounding case 4o'
 DC XL16'3F800000C00000003F800000C0000000'
 DC CL48'DIEBR Rounding case 4p'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'DIEBR Rounding case 4q'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'DIEBR Rounding case 4r'
 DC XL16'3F800000C00000003F800000C0000000'
 DC CL48'DIEBR Rounding case 4s'
 DC XL16'3F800000C00000003F800000C0000000'
 DC CL48'DIEBR Rounding case 4t'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'DIEBR Rounding case 4u'
 DC XL16'3F800000C00000003F800000C0000000'
 DC CL48'DIEBR Rounding case 4v'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'DIEBR Rounding case 4w'
 DC XL16'BF800000BF800000BF800000BF800000'
 DC CL48'DIEBR Rounding case 4x'
 DC XL16'3F800000C00000003F800000C0000000'
 DC CL48'DIEBR Rounding case 5a'
 DC XL16'3F800000BF8000003F800000BF800000'
 DC CL48'DIEBR Rounding case 5b'
 DC XL16'3F800000BF8000003F800000BF800000'
 DC CL48'DIEBR Rounding case 5c'
 DC XL16'BF80000080000000BF80000080000000'
 DC CL48'DIEBR Rounding case 5d'
 DC XL16'BF80000080000000BF80000080000000'
 DC CL48'DIEBR Rounding case 5e'
 DC XL16'BF80000080000000BF80000080000000'
 DC CL48'DIEBR Rounding case 5f'
 DC XL16'3F800000BF8000003F800000BF800000'
 DC CL48'DIEBR Rounding case 5g'
 DC XL16'3F800000BF8000003F800000BF800000'
 DC CL48'DIEBR Rounding case 5h'
 DC XL16'3F800000BF8000003F800000BF800000'
 DC CL48'DIEBR Rounding case 5i'
 DC XL16'BF80000080000000BF80000080000000'
 DC CL48'DIEBR Rounding case 5j'
 DC XL16'BF80000080000000BF80000080000000'
 DC CL48'DIEBR Rounding case 5k'
 DC XL16'BF80000080000000BF80000080000000'
 DC CL48'DIEBR Rounding case 5l'
 DC XL16'3F800000BF8000003F800000BF800000'
 DC CL48'DIEBR Rounding case 5m'
 DC XL16'3F800000BF8000003F800000BF800000'
 DC CL48'DIEBR Rounding case 5n'
 DC XL16'3F800000BF8000003F800000BF800000'
 DC CL48'DIEBR Rounding case 5o'
 DC XL16'BF80000080000000BF80000080000000'
 DC CL48'DIEBR Rounding case 5p'
 DC XL16'BF80000080000000BF80000080000000'
 DC CL48'DIEBR Rounding case 5q'
 DC XL16'BF80000080000000BF80000080000000'
 DC CL48'DIEBR Rounding case 5r'
 DC XL16'3F800000BF8000003F800000BF800000'
 DC CL48'DIEBR Rounding case 5s'
 DC XL16'3F800000BF8000003F800000BF800000'
 DC CL48'DIEBR Rounding case 5t'
 DC XL16'3F800000BF8000003F800000BF800000'
 DC CL48'DIEBR Rounding case 5u'
 DC XL16'BF80000080000000BF80000080000000'
 DC CL48'DIEBR Rounding case 5v'
 DC XL16'BF80000080000000BF80000080000000'
 DC CL48'DIEBR Rounding case 5w'
 DC XL16'BF80000080000000BF80000080000000'
 DC CL48'DIEBR Rounding case 5x'
 DC XL16'3F800000BF8000003F800000BF800000'
 DC CL48'DIEBR Rounding case 6a'
 DC XL16'BF8000003F800000BF8000003F800000'
 DC CL48'DIEBR Rounding case 6b'
 DC XL16'BF8000003F800000BF8000003F800000'
 DC CL48'DIEBR Rounding case 6c'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR Rounding case 6d'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR Rounding case 6e'
 DC XL16'BF8000003F800000BF8000003F800000'
 DC CL48'DIEBR Rounding case 6f'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR Rounding case 6g'
 DC XL16'BF8000003F800000BF8000003F800000'
 DC CL48'DIEBR Rounding case 6h'
 DC XL16'BF8000003F800000BF8000003F800000'
 DC CL48'DIEBR Rounding case 6i'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR Rounding case 6j'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR Rounding case 6k'
 DC XL16'BF8000003F800000BF8000003F800000'
 DC CL48'DIEBR Rounding case 6l'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR Rounding case 6m'
 DC XL16'BF8000003F800000BF8000003F800000'
 DC CL48'DIEBR Rounding case 6n'
 DC XL16'BF8000003F800000BF8000003F800000'
 DC CL48'DIEBR Rounding case 6o'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR Rounding case 6p'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR Rounding case 6q'
 DC XL16'BF8000003F800000BF8000003F800000'
 DC CL48'DIEBR Rounding case 6r'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR Rounding case 6s'
 DC XL16'BF8000003F800000BF8000003F800000'
 DC CL48'DIEBR Rounding case 6t'
 DC XL16'BF8000003F800000BF8000003F800000'
 DC CL48'DIEBR Rounding case 6u'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR Rounding case 6v'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR Rounding case 6w'
 DC XL16'BF8000003F800000BF8000003F800000'
 DC CL48'DIEBR Rounding case 6x'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR Rounding case 7a'
 DC XL16'BF80000040000000BF80000040000000'
 DC CL48'DIEBR Rounding case 7b'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'DIEBR Rounding case 7c'
 DC XL16'BF80000040000000BF80000040000000'
 DC CL48'DIEBR Rounding case 7d'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'DIEBR Rounding case 7e'
 DC XL16'BF80000040000000BF80000040000000'
 DC CL48'DIEBR Rounding case 7f'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'DIEBR Rounding case 7g'
 DC XL16'BF80000040000000BF80000040000000'
 DC CL48'DIEBR Rounding case 7h'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'DIEBR Rounding case 7i'
 DC XL16'BF80000040000000BF80000040000000'
 DC CL48'DIEBR Rounding case 7j'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'DIEBR Rounding case 7k'
 DC XL16'BF80000040000000BF80000040000000'
 DC CL48'DIEBR Rounding case 7l'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'DIEBR Rounding case 7m'
 DC XL16'BF80000040000000BF80000040000000'
 DC CL48'DIEBR Rounding case 7n'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'DIEBR Rounding case 7o'
 DC XL16'BF80000040000000BF80000040000000'
 DC CL48'DIEBR Rounding case 7p'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'DIEBR Rounding case 7q'
 DC XL16'BF80000040000000BF80000040000000'
 DC CL48'DIEBR Rounding case 7r'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'DIEBR Rounding case 7s'
 DC XL16'BF80000040000000BF80000040000000'
 DC CL48'DIEBR Rounding case 7t'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'DIEBR Rounding case 7u'
 DC XL16'BF80000040000000BF80000040000000'
 DC CL48'DIEBR Rounding case 7v'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'DIEBR Rounding case 7w'
 DC XL16'BF80000040000000BF80000040000000'
 DC CL48'DIEBR Rounding case 7x'
 DC XL16'3F8000003F8000003F8000003F800000'
 DC CL48'DIEBR Rounding case 8a'
 DC XL16'BF80000040400000BF80000040400000'
 DC CL48'DIEBR Rounding case 8b'
 DC XL16'BF80000040400000BF80000040400000'
 DC CL48'DIEBR Rounding case 8c'
 DC XL16'3F800000400000003F80000040000000'
 DC CL48'DIEBR Rounding case 8d'
 DC XL16'3F800000400000003F80000040000000'
 DC CL48'DIEBR Rounding case 8e'
 DC XL16'BF80000040400000BF80000040400000'
 DC CL48'DIEBR Rounding case 8f'
 DC XL16'3F800000400000003F80000040000000'
 DC CL48'DIEBR Rounding case 8g'
 DC XL16'BF80000040400000BF80000040400000'
 DC CL48'DIEBR Rounding case 8h'
 DC XL16'BF80000040400000BF80000040400000'
 DC CL48'DIEBR Rounding case 8i'
 DC XL16'3F800000400000003F80000040000000'
 DC CL48'DIEBR Rounding case 8j'
 DC XL16'3F800000400000003F80000040000000'
 DC CL48'DIEBR Rounding case 8k'
 DC XL16'BF80000040400000BF80000040400000'
 DC CL48'DIEBR Rounding case 8l'
 DC XL16'3F800000400000003F80000040000000'
 DC CL48'DIEBR Rounding case 8m'
 DC XL16'BF80000040400000BF80000040400000'
 DC CL48'DIEBR Rounding case 8n'
 DC XL16'BF80000040400000BF80000040400000'
 DC CL48'DIEBR Rounding case 8o'
 DC XL16'3F800000400000003F80000040000000'
 DC CL48'DIEBR Rounding case 8p'
 DC XL16'3F800000400000003F80000040000000'
 DC CL48'DIEBR Rounding case 8q'
 DC XL16'BF80000040400000BF80000040400000'
 DC CL48'DIEBR Rounding case 8r'
 DC XL16'3F800000400000003F80000040000000'
 DC CL48'DIEBR Rounding case 8s'
 DC XL16'BF80000040400000BF80000040400000'
 DC CL48'DIEBR Rounding case 8t'
 DC XL16'BF80000040400000BF80000040400000'
 DC CL48'DIEBR Rounding case 8u'
 DC XL16'3F800000400000003F80000040000000'
 DC CL48'DIEBR Rounding case 8v'
 DC XL16'3F800000400000003F80000040000000'
 DC CL48'DIEBR Rounding case 8w'
 DC XL16'BF80000040400000BF80000040400000'
 DC CL48'DIEBR Rounding case 8x'
 DC XL16'3F800000400000003F80000040000000'
 DC CL48'DIEBR Rounding case 9a'
 DC XL16'BF80000040C00000BF80000040C00000'
 DC CL48'DIEBR Rounding case 9b'
 DC XL16'3F80000040A000003F80000040A00000'
 DC CL48'DIEBR Rounding case 9c'
 DC XL16'BF80000040C00000BF80000040C00000'
 DC CL48'DIEBR Rounding case 9d'
 DC XL16'3F80000040A000003F80000040A00000'
 DC CL48'DIEBR Rounding case 9e'
 DC XL16'BF80000040C00000BF80000040C00000'
 DC CL48'DIEBR Rounding case 9f'
 DC XL16'3F80000040A000003F80000040A00000'
 DC CL48'DIEBR Rounding case 9g'
 DC XL16'BF80000040C00000BF80000040C00000'
 DC CL48'DIEBR Rounding case 9h'
 DC XL16'3F80000040A000003F80000040A00000'
 DC CL48'DIEBR Rounding case 9i'
 DC XL16'BF80000040C00000BF80000040C00000'
 DC CL48'DIEBR Rounding case 9j'
 DC XL16'3F80000040A000003F80000040A00000'
 DC CL48'DIEBR Rounding case 9k'
 DC XL16'BF80000040C00000BF80000040C00000'
 DC CL48'DIEBR Rounding case 9l'
 DC XL16'3F80000040A000003F80000040A00000'
 DC CL48'DIEBR Rounding case 9m'
 DC XL16'BF80000040C00000BF80000040C00000'
 DC CL48'DIEBR Rounding case 9n'
 DC XL16'3F80000040A000003F80000040A00000'
 DC CL48'DIEBR Rounding case 9o'
 DC XL16'BF80000040C00000BF80000040C00000'
 DC CL48'DIEBR Rounding case 9p'
 DC XL16'3F80000040A000003F80000040A00000'
 DC CL48'DIEBR Rounding case 9q'
 DC XL16'BF80000040C00000BF80000040C00000'
 DC CL48'DIEBR Rounding case 9r'
 DC XL16'3F80000040A000003F80000040A00000'
 DC CL48'DIEBR Rounding case 9s'
 DC XL16'BF80000040C00000BF80000040C00000'
 DC CL48'DIEBR Rounding case 9t'
 DC XL16'3F80000040A000003F80000040A00000'
 DC CL48'DIEBR Rounding case 9u'
 DC XL16'BF80000040C00000BF80000040C00000'
 DC CL48'DIEBR Rounding case 9v'
 DC XL16'3F80000040A000003F80000040A00000'
 DC CL48'DIEBR Rounding case 9w'
 DC XL16'BF80000040C00000BF80000040C00000'
 DC CL48'DIEBR Rounding case 9x'
 DC XL16'3F80000040A000003F80000040A00000'
 DC CL48'DIEBR Rounding case 10a'
 DC XL16'BF80000041200000BF80000041200000'
 DC CL48'DIEBR Rounding case 10b'
 DC XL16'3F800000411000003F80000041100000'
 DC CL48'DIEBR Rounding case 10c'
 DC XL16'BF80000041200000BF80000041200000'
 DC CL48'DIEBR Rounding case 10d'
 DC XL16'3F800000411000003F80000041100000'
 DC CL48'DIEBR Rounding case 10e'
 DC XL16'BF80000041200000BF80000041200000'
 DC CL48'DIEBR Rounding case 10f'
 DC XL16'3F800000411000003F80000041100000'
 DC CL48'DIEBR Rounding case 10g'
 DC XL16'BF80000041200000BF80000041200000'
 DC CL48'DIEBR Rounding case 10h'
 DC XL16'3F800000411000003F80000041100000'
 DC CL48'DIEBR Rounding case 10i'
 DC XL16'BF80000041200000BF80000041200000'
 DC CL48'DIEBR Rounding case 10j'
 DC XL16'3F800000411000003F80000041100000'
 DC CL48'DIEBR Rounding case 10k'
 DC XL16'BF80000041200000BF80000041200000'
 DC CL48'DIEBR Rounding case 10l'
 DC XL16'3F800000411000003F80000041100000'
 DC CL48'DIEBR Rounding case 10m'
 DC XL16'BF80000041200000BF80000041200000'
 DC CL48'DIEBR Rounding case 10n'
 DC XL16'3F800000411000003F80000041100000'
 DC CL48'DIEBR Rounding case 10o'
 DC XL16'BF80000041200000BF80000041200000'
 DC CL48'DIEBR Rounding case 10p'
 DC XL16'3F800000411000003F80000041100000'
 DC CL48'DIEBR Rounding case 10q'
 DC XL16'BF80000041200000BF80000041200000'
 DC CL48'DIEBR Rounding case 10r'
 DC XL16'3F800000411000003F80000041100000'
 DC CL48'DIEBR Rounding case 10s'
 DC XL16'BF80000041200000BF80000041200000'
 DC CL48'DIEBR Rounding case 10t'
 DC XL16'3F800000411000003F80000041100000'
 DC CL48'DIEBR Rounding case 10u'
 DC XL16'BF80000041200000BF80000041200000'
 DC CL48'DIEBR Rounding case 10v'
 DC XL16'3F800000411000003F80000041100000'
 DC CL48'DIEBR Rounding case 10w'
 DC XL16'BF80000041200000BF80000041200000'
 DC CL48'DIEBR Rounding case 10x'
 DC XL16'3F800000411000003F80000041100000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 11'
 DC XL16'000000003F800000000000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'40400000000000004040000000000000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'40400000000000004040000000000000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'40400000000000004040000000000000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'40400000000000004040000000000000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'40400000000000004040000000000000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'40400000000000004040000000000000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'40400000000000004040000000000000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'C00000003F800000C00000003F800000'
 DC CL48'DIEBR Rounding case 12'
 DC XL16'40400000000000004040000000000000'
SBFPRMO_NUM EQU (*-SBFPRMO_GOOD)/64
*
*
SBFPRMOF_GOOD EQU *
 DC CL48'DIEBR Rounding FPCR 1ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 1cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 1ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 1gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 1ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 1kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 1mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 1op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 1qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 1st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 1uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 1wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 2ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 2cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 2ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 2gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 2ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 2kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 2mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 2op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 2qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 2st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 2uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 2wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 3ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 3cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 3ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 3gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 3ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 3kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 3mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 3op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 3qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 3st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 3uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 3wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 4ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 4cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 4ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 4gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 4ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 4kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 4mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 4op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 4qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 4st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 4uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 4wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 5ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 5cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 5ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 5gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 5ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 5kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 5mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 5op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 5qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 5st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 5uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 5wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 6ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 6cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 6ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 6gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 6ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 6kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 6mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 6op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 6qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 6st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 6uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 6wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 7ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 7cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 7ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 7gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 7ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 7kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 7mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 7op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 7qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 7st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 7uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 7wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 8ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 8cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 8ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 8gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 8ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 8kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 8mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 8op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 8qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 8st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 8uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 8wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 9ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 9cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 9ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 9gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 9ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 9kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 9mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 9op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 9qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 9st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 9uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 9wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 10ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 10cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 10ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 10gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 10ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 10kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 10mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 10op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 10qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 10st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 10uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 10wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 11ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 11cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 11ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 11gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 11ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 11kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 11mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 11op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 11qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 11st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 11uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 11wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 12ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 12cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 12ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 12gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 12ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 12kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 12mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 12op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 12qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 12st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 12uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIEBR Rounding FPCR 12wx'
 DC XL16'00000000F800000000000000F8000000'
SBFPRMOF_NUM EQU (*-SBFPRMOF_GOOD)/64
*
*
LBFPRMO_GOOD EQU *
 DC CL48'DIDBR rounding test 1a NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1a TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1b NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1b TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1c NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1c TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1d NT'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1d TR'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1e NT'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1e TR'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1f NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1f TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1g NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1g TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1h NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1h TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1i NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1i TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1j NT'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1j TR'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1k NT'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1k TR'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1l NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1l TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1m NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1m TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1n NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1n TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1o NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1o TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1p NT'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1p TR'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1q NT'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1q TR'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1r NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1r TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1s NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1s TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1t NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1t TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1u NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1u TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1v NT'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1v TR'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1w NT'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1w TR'
 DC XL16'BFF8000000000000C010000000000000'
 DC CL48'DIDBR rounding test 1x NT'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 1x TR'
 DC XL16'3FE0000000000000C014000000000000'
 DC CL48'DIDBR rounding test 2a NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2a TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2b NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2b TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2c NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2c TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2d NT'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2d TR'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2e NT'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2e TR'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2f NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2f TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2g NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2g TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2h NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2h TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2i NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2i TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2j NT'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2j TR'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2k NT'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2k TR'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2l NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2l TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2m NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2m TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2n NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2n TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2o NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2o TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2p NT'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2p TR'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2q NT'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2q TR'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2r NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2r TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2s NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2s TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2t NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2t TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2u NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2u TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2v NT'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2v TR'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2w NT'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2w TR'
 DC XL16'BFF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 2x NT'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 2x TR'
 DC XL16'3FE0000000000000C008000000000000'
 DC CL48'DIDBR rounding test 3a NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3a TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3b NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3b TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3c NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3c TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3d NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3d TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3e NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3e TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3f NT'
 DC XL16'3FF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 3f TR'
 DC XL16'3FF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 3g NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3g TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3h NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3h TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3i NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3i TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3j NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3j TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3k NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3k TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3l NT'
 DC XL16'3FF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 3l TR'
 DC XL16'3FF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 3m NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3m TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3n NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3n TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3o NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3o TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3p NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3p TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3q NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3q TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3r NT'
 DC XL16'3FF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 3r TR'
 DC XL16'3FF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 3s NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3s TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3t NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3t TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3u NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3u TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3v NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3v TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3w NT'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3w TR'
 DC XL16'BFE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 3x NT'
 DC XL16'3FF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 3x TR'
 DC XL16'3FF8000000000000C000000000000000'
 DC CL48'DIDBR rounding test 4a NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4a TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4b NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4b TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4c NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4c TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4d NT'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4d TR'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4e NT'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4e TR'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4f NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4f TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4g NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4g TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4h NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4h TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4i NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4i TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4j NT'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4j TR'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4k NT'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4k TR'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4l NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4l TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4m NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4m TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4n NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4n TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4o NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4o TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4p NT'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4p TR'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4q NT'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4q TR'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4r NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4r TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4s NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4s TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4t NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4t TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4u NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4u TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4v NT'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4v TR'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4w NT'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4w TR'
 DC XL16'BFF80000000000008000000000000000'
 DC CL48'DIDBR rounding test 4x NT'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 4x TR'
 DC XL16'3FE0000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5a NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5a TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5b NT'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5b TR'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5c NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5c TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5d NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5d TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5e NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5e TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5f NT'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5f TR'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5g NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5g TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5h NT'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5h TR'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5i NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5i TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5j NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5j TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5k NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5k TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5l NT'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5l TR'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5m NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5m TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5n NT'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5n TR'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5o NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5o TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5p NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5p TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5q NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5q TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5r NT'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5r TR'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5s NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5s TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5t NT'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5t TR'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5u NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5u TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5v NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5v TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5w NT'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5w TR'
 DC XL16'BFE00000000000008000000000000000'
 DC CL48'DIDBR rounding test 5x NT'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 5x TR'
 DC XL16'3FF8000000000000BFF0000000000000'
 DC CL48'DIDBR rounding test 6a NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6a TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6b NT'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6b TR'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6c NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6c TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6d NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6d TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6e NT'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6e TR'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6f NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6f TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6g NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6g TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6h NT'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6h TR'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6i NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6i TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6j NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6j TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6k NT'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6k TR'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6l NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6l TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6m NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6m TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6n NT'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6n TR'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6o NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6o TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6p NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6p TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6q NT'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6q TR'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6r NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6r TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6s NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6s TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6t NT'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6t TR'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6u NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6u TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6v NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6v TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6w NT'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6w TR'
 DC XL16'BFF80000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 6x NT'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 6x TR'
 DC XL16'3FE00000000000000000000000000000'
 DC CL48'DIDBR rounding test 7a NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7a TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7b NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7b TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7c NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7c TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7d NT'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7d TR'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7e NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7e TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7f NT'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7f TR'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7g NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7g TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7h NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7h TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7i NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7i TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7j NT'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7j TR'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7k NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7k TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7l NT'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7l TR'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7m NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7m TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7n NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7n TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7o NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7o TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7p NT'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7p TR'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7q NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7q TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7r NT'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7r TR'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7s NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7s TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7t NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7t TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7u NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7u TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7v NT'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7v TR'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7w NT'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7w TR'
 DC XL16'BFE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 7x NT'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 7x TR'
 DC XL16'3FF80000000000000000000000000000'
 DC CL48'DIDBR rounding test 8a NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8a TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8b NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8b TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8c NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8c TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8d NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8d TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8e NT'
 DC XL16'BFF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 8e TR'
 DC XL16'BFF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 8f NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8f TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8g NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8g TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8h NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8h TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8i NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8i TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8j NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8j TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8k NT'
 DC XL16'BFF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 8k TR'
 DC XL16'BFF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 8l NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8l TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8m NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8m TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8n NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8n TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8o NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8o TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8p NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8p TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8q NT'
 DC XL16'BFF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 8q TR'
 DC XL16'BFF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 8r NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8r TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8s NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8s TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8t NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8t TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8u NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8u TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8v NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8v TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8w NT'
 DC XL16'BFF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 8w TR'
 DC XL16'BFF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 8x NT'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 8x TR'
 DC XL16'3FE00000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 9a NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9a TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9b NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9b TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9c NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9c TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9d NT'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9d TR'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9e NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9e TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9f NT'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9f TR'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9g NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9g TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9h NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9h TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9i NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9i TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9j NT'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9j TR'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9k NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9k TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9l NT'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9l TR'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9m NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9m TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9n NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9n TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9o NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9o TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9p NT'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9p TR'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9q NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9q TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9r NT'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9r TR'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9s NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9s TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9t NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9t TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9u NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9u TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9v NT'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9v TR'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9w NT'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9w TR'
 DC XL16'BFE00000000000004008000000000000'
 DC CL48'DIDBR rounding test 9x NT'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 9x TR'
 DC XL16'3FF80000000000004000000000000000'
 DC CL48'DIDBR rounding test 10a NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10a TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10b NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10b TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10c NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10c TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10d NT'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10d TR'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10e NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10e TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10f NT'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10f TR'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10g NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10g TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10h NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10h TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10i NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10i TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10j NT'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10j TR'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10k NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10k TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10l NT'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10l TR'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10m NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10m TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10n NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10n TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10o NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10o TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10p NT'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10p TR'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10q NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10q TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10r NT'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10r TR'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10s NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10s TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10t NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10t TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10u NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10u TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10v NT'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10v TR'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10w NT'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10w TR'
 DC XL16'BFE00000000000004014000000000000'
 DC CL48'DIDBR rounding test 10x NT'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 10x TR'
 DC XL16'3FF80000000000004010000000000000'
 DC CL48'DIDBR rounding test 11a NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11a TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11b NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11b TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11c NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11c TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11d NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11d TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11e NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11e TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11f NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11f TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11g NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11g TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11h NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11h TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11i NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11i TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11j NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11j TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11k NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11k TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11l NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11l TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11m NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11m TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11n NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11n TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11o NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11o TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11p NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11p TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11q NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11q TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11r NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11r TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11s NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11s TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11t NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11t TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11u NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11u TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11v NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11v TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11w NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11w TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11x NT'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 11x TR'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12a NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12a TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12b NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12b TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12c NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12c TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12d NT'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12d TR'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12e NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12e TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12f NT'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12f TR'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12g NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12g TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12h NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12h TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12i NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12i TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12j NT'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12j TR'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12k NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12k TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12l NT'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12l TR'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12m NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12m TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12n NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12n TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12o NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12o TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12p NT'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12p TR'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12q NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12q TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12r NT'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12r TR'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12s NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12s TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12t NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12t TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12u NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12u TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12v NT'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12v TR'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12w NT'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12w TR'
 DC XL16'C0000000000000003FF0000000000000'
 DC CL48'DIDBR rounding test 12x NT'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR rounding test 12x TR'
 DC XL16'40080000000000000000000000000000'
LBFPRMO_NUM EQU (*-LBFPRMO_GOOD)/64
*
*
LBFPRMOF_GOOD EQU *
 DC CL48'DIDBR Rounding FPCR 1ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 1cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 1ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 1gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 1ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 1kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 1mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 1op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 1qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 1st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 1uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 1wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 2ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 2cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 2ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 2gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 2ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 2kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 2mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 2op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 2qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 2st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 2uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 2wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 3ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 3cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 3ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 3gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 3ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 3kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 3mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 3op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 3qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 3st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 3uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 3wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 4ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 4cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 4ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 4gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 4ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 4kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 4mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 4op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 4qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 4st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 4uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 4wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 5ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 5cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 5ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 5gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 5ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 5kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 5mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 5op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 5qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 5st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 5uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 5wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 6ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 6cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 6ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 6gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 6ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 6kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 6mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 6op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 6qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 6st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 6uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 6wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 7ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 7cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 7ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 7gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 7ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 7kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 7mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 7op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 7qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 7st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 7uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 7wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 8ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 8cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 8ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 8gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 8ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 8kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 8mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 8op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 8qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 8st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 8uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 8wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 9ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 9cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 9ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 9gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 9ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 9kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 9mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 9op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 9qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 9st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 9uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 9wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 10ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 10cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 10ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 10gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 10ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 10kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 10mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 10op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 10qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 10st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 10uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 10wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 11ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 11cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 11ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 11gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 11ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 11kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 11mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 11op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 11qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 11st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 11uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 11wx'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 12ab'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 12cd'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 12ef'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 12gh'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 12ij'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 12kl'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 12mn'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 12op'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 12qr'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 12st'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 12uv'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'DIDBR Rounding FPCR 12wx'
 DC XL16'00000000F800000000000000F8000000'
LBFPRMOF_NUM EQU (*-LBFPRMOF_GOOD)/64
*
*
SBFPOUT_GOOD EQU *
 DC CL48'DIEBR finite test -8/-4 1a'
 DC XL16'8000000040000000C100000000000000'
 DC CL48'DIEBR finite test -8/-4 1b'
 DC XL16'8000000040000000C100000000000000'
 DC CL48'DIEBR finite test -7/-4 2a'
 DC XL16'3F80000040000000C0E0000000000000'
 DC CL48'DIEBR finite test -7/-4 2b'
 DC XL16'3F80000040000000C0E0000000000000'
 DC CL48'DIEBR finite test -6/-4 3a'
 DC XL16'4000000040000000C0C0000000000000'
 DC CL48'DIEBR finite test -6/-4 3b'
 DC XL16'4000000040000000C0C0000000000000'
 DC CL48'DIEBR finite test -5/-4 4a'
 DC XL16'BF8000003F800000C0A0000000000000'
 DC CL48'DIEBR finite test -5/-4 4b'
 DC XL16'BF8000003F800000C0A0000000000000'
 DC CL48'DIEBR finite test -4/-4 5a'
 DC XL16'800000003F800000C080000000000000'
 DC CL48'DIEBR finite test -4/-4 5b'
 DC XL16'800000003F800000C080000000000000'
 DC CL48'DIEBR finite test -3/-4 6a'
 DC XL16'3F8000003F800000C040000000000000'
 DC CL48'DIEBR finite test -3/-4 6b'
 DC XL16'3F8000003F800000C040000000000000'
 DC CL48'DIEBR finite test -2/-4 7a'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'DIEBR finite test -2/-4 7b'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'DIEBR finite test -1/-4 8a'
 DC XL16'BF80000000000000BF80000000000000'
 DC CL48'DIEBR finite test -1/-4 8b'
 DC XL16'BF80000000000000BF80000000000000'
 DC CL48'DIEBR finite test +1/-4 9a'
 DC XL16'3F800000800000003F80000000000000'
 DC CL48'DIEBR finite test +1/-4 9b'
 DC XL16'3F800000800000003F80000000000000'
 DC CL48'DIEBR finite test +2/-4 10a'
 DC XL16'40000000800000004000000000000000'
 DC CL48'DIEBR finite test +2/-4 10b'
 DC XL16'40000000800000004000000000000000'
 DC CL48'DIEBR finite test +3/-4 11a'
 DC XL16'BF800000BF8000004040000000000000'
 DC CL48'DIEBR finite test +3/-4 11b'
 DC XL16'BF800000BF8000004040000000000000'
 DC CL48'DIEBR finite test +4/-4 12a'
 DC XL16'00000000BF8000004080000000000000'
 DC CL48'DIEBR finite test +4/-4 12b'
 DC XL16'00000000BF8000004080000000000000'
 DC CL48'DIEBR finite test +5/-4 13a'
 DC XL16'3F800000BF80000040A0000000000000'
 DC CL48'DIEBR finite test +5/-4 13b'
 DC XL16'3F800000BF80000040A0000000000000'
 DC CL48'DIEBR finite test +6/-4 14a'
 DC XL16'C0000000C000000040C0000000000000'
 DC CL48'DIEBR finite test +6/-4 14b'
 DC XL16'C0000000C000000040C0000000000000'
 DC CL48'DIEBR finite test 15a'
 DC XL16'BF800000C000000040E0000000000000'
 DC CL48'DIEBR finite test 15b'
 DC XL16'BF800000C000000040E0000000000000'
 DC CL48'DIEBR finite test 16a'
 DC XL16'00000000C00000004100000000000000'
 DC CL48'DIEBR finite test 16b'
 DC XL16'00000000C00000004100000000000000'
 DC CL48'DIEBR finite test 17a'
 DC XL16'80000000C0000000C100000000000000'
 DC CL48'DIEBR finite test 17b'
 DC XL16'80000000C0000000C100000000000000'
 DC CL48'DIEBR finite test 18a'
 DC XL16'3F800000C0000000C0E0000000000000'
 DC CL48'DIEBR finite test 18b'
 DC XL16'3F800000C0000000C0E0000000000000'
 DC CL48'DIEBR finite test 19a'
 DC XL16'40000000C0000000C0C0000000000000'
 DC CL48'DIEBR finite test 19b'
 DC XL16'40000000C0000000C0C0000000000000'
 DC CL48'DIEBR finite test 20a'
 DC XL16'BF800000BF800000C0A0000000000000'
 DC CL48'DIEBR finite test 20b'
 DC XL16'BF800000BF800000C0A0000000000000'
 DC CL48'DIEBR finite test 21a'
 DC XL16'80000000BF800000C080000000000000'
 DC CL48'DIEBR finite test 21b'
 DC XL16'80000000BF800000C080000000000000'
 DC CL48'DIEBR finite test 22a'
 DC XL16'3F800000BF800000C040000000000000'
 DC CL48'DIEBR finite test 22b'
 DC XL16'3F800000BF800000C040000000000000'
 DC CL48'DIEBR finite test 23a'
 DC XL16'C000000080000000C000000000000000'
 DC CL48'DIEBR finite test 23b'
 DC XL16'C000000080000000C000000000000000'
 DC CL48'DIEBR finite test 24a'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR finite test 24b'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR finite test 25a'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR finite test 25b'
 DC XL16'3F800000000000003F80000000000000'
 DC CL48'DIEBR finite test 26a'
 DC XL16'40000000000000004000000000000000'
 DC CL48'DIEBR finite test 26b'
 DC XL16'40000000000000004000000000000000'
 DC CL48'DIEBR finite test 27a'
 DC XL16'BF8000003F8000004040000000000000'
 DC CL48'DIEBR finite test 27b'
 DC XL16'BF8000003F8000004040000000000000'
 DC CL48'DIEBR finite test 28a'
 DC XL16'000000003F8000004080000000000000'
 DC CL48'DIEBR finite test 28b'
 DC XL16'000000003F8000004080000000000000'
 DC CL48'DIEBR finite test 29a'
 DC XL16'3F8000003F80000040A0000000000000'
 DC CL48'DIEBR finite test 29b'
 DC XL16'3F8000003F80000040A0000000000000'
 DC CL48'DIEBR finite test 30a'
 DC XL16'C00000004000000040C0000000000000'
 DC CL48'DIEBR finite test 30b'
 DC XL16'C00000004000000040C0000000000000'
 DC CL48'DIEBR finite test 31a'
 DC XL16'BF8000004000000040E0000000000000'
 DC CL48'DIEBR finite test 31b'
 DC XL16'BF8000004000000040E0000000000000'
 DC CL48'DIEBR finite test 32a'
 DC XL16'00000000400000004100000000000000'
 DC CL48'DIEBR finite test 32b'
 DC XL16'00000000400000004100000000000000'
 DC CL48'DIEBR test 33a two finites'
 DC XL16'40800000C08000004220000000000000'
 DC CL48'DIEBR test 33b two finites'
 DC XL16'40800000C08000004220000000000000'
 DC CL48'DIEBR test 34a two finites'
 DC XL16'0000000069FFFFFF1F7FFFFF00000000'
 DC CL48'DIEBR test 34b two finites'
 DC XL16'0000000069FFFFFF1F7FFFFF00000000'
 DC CL48'DIEBR test 35a two finites'
 DC XL16'000000013F80000000FFFFFF00000000'
 DC CL48'DIEBR test 35b two finites'
 DC XL16'550000003F8000005500000000000000'
 DC CL48'DIEBR test 36a two finites'
 DC XL16'408000004BAAAAAA4C80000000000000'
 DC CL48'DIEBR test 36b two finites'
 DC XL16'408000004BAAAAAA4C80000000000000'
 DC CL48'DIEBR test 37a two finites'
 DC XL16'40100000000000004010000000000000'
 DC CL48'DIEBR test 37b two finites'
 DC XL16'40100000000000004010000000000000'
 DC CL48'DIEBR test 38a two finites'
 DC XL16'73A00000683A2E8A73A0000000000000'
 DC CL48'DIEBR test 38a two finites'
 DC XL16'73A00000683A2E8A73A0000000000000'
SBFPOUT_NUM EQU (*-SBFPOUT_GOOD)/64
*
*
SBFPFLGS_GOOD EQU *
 DC CL48'DIEBR FPCR finite test -8/-4 1'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test -7/-4 2'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test -6/-4 3'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test -5/-4 4'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test -4/-4 5'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test -3/-4 6'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test -2/-4 7'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test -1/-4 8'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test +1/-4 9'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test +2/-4 10'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test +3/-4 11'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test +4/-4 12'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test +5/-4 13'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test +6/-4 14'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 15'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 16'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 17'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 18'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 19'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 20'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 21'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 22'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 23'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 24'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 25'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 26'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 27'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 28'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 29'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 30'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 31'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 32'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 33'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 34'
 DC XL16'0000000100000001F800000100000001'
 DC CL48'DIEBR FPCR finite test 35'
 DC XL16'0000000000000000F800100000080000'
 DC CL48'DIEBR FPCR finite test 36'
 DC XL16'0000000200000002F800000200000002'
 DC CL48'DIEBR FPCR finite test 37'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIEBR FPCR finite test 38'
 DC XL16'0000000300080003F800000300080003'
SBFPFLGS_NUM EQU (*-SBFPFLGS_GOOD)/64
*
*
LBFPFLGS_GOOD EQU *
 DC CL48'DIDBR FPCR finite test -8/-4 1'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test -7/-4 2'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test -6/-4 3'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test -5/-4 4'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test -4/-4 5'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test -3/-4 6'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test -2/-4 7'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test -1/-4 8'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test +1/-4 9'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test +2/-4 10'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test +3/-4 10'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test +4/-4 12'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test +5/-4 13'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test +6/-4 14'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 15'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 16'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 17'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 18'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 19'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 20'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 21'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 22'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 23'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 24'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 25'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 26'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 27'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 28'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 29'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 30'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 31'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 32'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 33'
 DC XL16'0000000000000000F800000000000000'
 DC CL48'DIDBR FPCR finite test 34'
 DC XL16'0000000100000001F800000100000001'
 DC CL48'DIDBR FPCR finite test 35'
 DC XL16'0000000300080003F800000300080003'
 DC CL48'DIDBR FPCR finite test 36'
 DC XL16'0000000000000000F800100000080000'
 DC CL48'DIDBR FPCR finite test 37'
 DC XL16'0000000200000002F800000200000002'
 DC CL48'DIDBR FPCR finite test 38'
 DC XL16'0000000000000000F800000000000000'
LBFPFLGS_NUM EQU (*-LBFPFLGS_GOOD)/64
*
*
LBFPOUT_GOOD EQU *
 DC CL48'DIDBR finite test -8/-4 1a'
 DC XL16'80000000000000004000000000000000'
 DC CL48'DIDBR finite test -8/-4 1a'
 DC XL16'C0200000000000000000000000000000'
 DC CL48'DIDBR finite test -8/-4 1b'
 DC XL16'80000000000000004000000000000000'
 DC CL48'DIDBR finite test -8/-4 1b'
 DC XL16'C0200000000000000000000000000000'
 DC CL48'DIDBR finite test -7/-4 2a'
 DC XL16'3FF00000000000004000000000000000'
 DC CL48'DIDBR finite test -7/-4 2a'
 DC XL16'C01C0000000000000000000000000000'
 DC CL48'DIDBR finite test -7/-4 2b'
 DC XL16'3FF00000000000004000000000000000'
 DC CL48'DIDBR finite test -7/-4 2b'
 DC XL16'C01C0000000000000000000000000000'
 DC CL48'DIDBR finite test -6/-4 3a'
 DC XL16'40000000000000004000000000000000'
 DC CL48'DIDBR finite test -6/-4 3a'
 DC XL16'C0180000000000000000000000000000'
 DC CL48'DIDBR finite test -6/-4 3b'
 DC XL16'40000000000000004000000000000000'
 DC CL48'DIDBR finite test -6/-4 3b'
 DC XL16'C0180000000000000000000000000000'
 DC CL48'DIDBR finite test -5/-4 4a'
 DC XL16'BFF00000000000003FF0000000000000'
 DC CL48'DIDBR finite test -5/-4 4a'
 DC XL16'C0140000000000000000000000000000'
 DC CL48'DIDBR finite test -5/-4 4b'
 DC XL16'BFF00000000000003FF0000000000000'
 DC CL48'DIDBR finite test -5/-4 4b'
 DC XL16'C0140000000000000000000000000000'
 DC CL48'DIDBR finite test -4/-4 5a'
 DC XL16'80000000000000003FF0000000000000'
 DC CL48'DIDBR finite test -4/-4 5a'
 DC XL16'C0100000000000000000000000000000'
 DC CL48'DIDBR finite test -4/-4 5b'
 DC XL16'80000000000000003FF0000000000000'
 DC CL48'DIDBR finite test -4/-4 5b'
 DC XL16'C0100000000000000000000000000000'
 DC CL48'DIDBR finite test -3/-4 6a'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'DIDBR finite test -3/-4 6a'
 DC XL16'C0080000000000000000000000000000'
 DC CL48'DIDBR finite test -3/-4 6b'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'DIDBR finite test -3/-4 6b'
 DC XL16'C0080000000000000000000000000000'
 DC CL48'DIDBR finite test -2/-4 7a'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'DIDBR finite test -2/-4 7a'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'DIDBR finite test -2/-4 7b'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'DIDBR finite test -2/-4 7b'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'DIDBR finite test -1/-4 8a'
 DC XL16'BFF00000000000000000000000000000'
 DC CL48'DIDBR finite test -1/-4 8a'
 DC XL16'BFF00000000000000000000000000000'
 DC CL48'DIDBR finite test -1/-4 8b'
 DC XL16'BFF00000000000000000000000000000'
 DC CL48'DIDBR finite test -1/-4 8b'
 DC XL16'BFF00000000000000000000000000000'
 DC CL48'DIDBR finite test +1/-4 9a'
 DC XL16'3FF00000000000008000000000000000'
 DC CL48'DIDBR finite test +1/-4 9a'
 DC XL16'3FF00000000000000000000000000000'
 DC CL48'DIDBR finite test +1/-4 9b'
 DC XL16'3FF00000000000008000000000000000'
 DC CL48'DIDBR finite test +1/-4 9b'
 DC XL16'3FF00000000000000000000000000000'
 DC CL48'DIDBR finite test +2/-4 10a'
 DC XL16'40000000000000008000000000000000'
 DC CL48'DIDBR finite test +2/-4 10a'
 DC XL16'40000000000000000000000000000000'
 DC CL48'DIDBR finite test +2/-4 10b'
 DC XL16'40000000000000008000000000000000'
 DC CL48'DIDBR finite test +2/-4 10b'
 DC XL16'40000000000000000000000000000000'
 DC CL48'DIDBR finite test +3/-4 11a'
 DC XL16'BFF0000000000000BFF0000000000000'
 DC CL48'DIDBR finite test +3/-4 11a'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR finite test +3/-4 11b'
 DC XL16'BFF0000000000000BFF0000000000000'
 DC CL48'DIDBR finite test +3/-4 11b'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR finite test +4/-4 12a'
 DC XL16'0000000000000000BFF0000000000000'
 DC CL48'DIDBR finite test +4/-4 12a'
 DC XL16'40100000000000000000000000000000'
 DC CL48'DIDBR finite test +4/-4 12b'
 DC XL16'0000000000000000BFF0000000000000'
 DC CL48'DIDBR finite test +4/-4 12b'
 DC XL16'40100000000000000000000000000000'
 DC CL48'DIDBR finite test +5/-4 13a'
 DC XL16'3FF0000000000000BFF0000000000000'
 DC CL48'DIDBR finite test +5/-4 13a'
 DC XL16'40140000000000000000000000000000'
 DC CL48'DIDBR finite test +5/-4 13b'
 DC XL16'3FF0000000000000BFF0000000000000'
 DC CL48'DIDBR finite test +5/-4 13b'
 DC XL16'40140000000000000000000000000000'
 DC CL48'DIDBR finite test +6/-4 14a'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'DIDBR finite test +6/-4 14a'
 DC XL16'40180000000000000000000000000000'
 DC CL48'DIDBR finite test +6/-4 14b'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'DIDBR finite test +6/-4 14b'
 DC XL16'40180000000000000000000000000000'
 DC CL48'DIDBR finite test 15a'
 DC XL16'BFF0000000000000C000000000000000'
 DC CL48'DIDBR finite test 15a'
 DC XL16'401C0000000000000000000000000000'
 DC CL48'DIDBR finite test 15b'
 DC XL16'BFF0000000000000C000000000000000'
 DC CL48'DIDBR finite test 15b'
 DC XL16'401C0000000000000000000000000000'
 DC CL48'DIDBR finite test 16a'
 DC XL16'0000000000000000C000000000000000'
 DC CL48'DIDBR finite test 16a'
 DC XL16'40200000000000000000000000000000'
 DC CL48'DIDBR finite test 16b'
 DC XL16'0000000000000000C000000000000000'
 DC CL48'DIDBR finite test 16b'
 DC XL16'40200000000000000000000000000000'
 DC CL48'DIDBR finite test 17a'
 DC XL16'8000000000000000C000000000000000'
 DC CL48'DIDBR finite test 17a'
 DC XL16'C0200000000000000000000000000000'
 DC CL48'DIDBR finite test 17b'
 DC XL16'8000000000000000C000000000000000'
 DC CL48'DIDBR finite test 17b'
 DC XL16'C0200000000000000000000000000000'
 DC CL48'DIDBR finite test 18a'
 DC XL16'3FF0000000000000C000000000000000'
 DC CL48'DIDBR finite test 18a'
 DC XL16'C01C0000000000000000000000000000'
 DC CL48'DIDBR finite test 18b'
 DC XL16'3FF0000000000000C000000000000000'
 DC CL48'DIDBR finite test 18b'
 DC XL16'C01C0000000000000000000000000000'
 DC CL48'DIDBR finite test 19a'
 DC XL16'4000000000000000C000000000000000'
 DC CL48'DIDBR finite test 19a'
 DC XL16'C0180000000000000000000000000000'
 DC CL48'DIDBR finite test 19b'
 DC XL16'4000000000000000C000000000000000'
 DC CL48'DIDBR finite test 19b'
 DC XL16'C0180000000000000000000000000000'
 DC CL48'DIDBR finite test 20a'
 DC XL16'BFF0000000000000BFF0000000000000'
 DC CL48'DIDBR finite test 20a'
 DC XL16'C0140000000000000000000000000000'
 DC CL48'DIDBR finite test 20b'
 DC XL16'BFF0000000000000BFF0000000000000'
 DC CL48'DIDBR finite test 20b'
 DC XL16'C0140000000000000000000000000000'
 DC CL48'DIDBR finite test 21a'
 DC XL16'8000000000000000BFF0000000000000'
 DC CL48'DIDBR finite test 21a'
 DC XL16'C0100000000000000000000000000000'
 DC CL48'DIDBR finite test 21b'
 DC XL16'8000000000000000BFF0000000000000'
 DC CL48'DIDBR finite test 21b'
 DC XL16'C0100000000000000000000000000000'
 DC CL48'DIDBR finite test 22a'
 DC XL16'3FF0000000000000BFF0000000000000'
 DC CL48'DIDBR finite test 22a'
 DC XL16'C0080000000000000000000000000000'
 DC CL48'DIDBR finite test 22b'
 DC XL16'3FF0000000000000BFF0000000000000'
 DC CL48'DIDBR finite test 22b'
 DC XL16'C0080000000000000000000000000000'
 DC CL48'DIDBR finite test 23a'
 DC XL16'C0000000000000008000000000000000'
 DC CL48'DIDBR finite test 23a'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'DIDBR finite test 23b'
 DC XL16'C0000000000000008000000000000000'
 DC CL48'DIDBR finite test 23b'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'DIDBR finite test 24a'
 DC XL16'3FF00000000000000000000000000000'
 DC CL48'DIDBR finite test 24a'
 DC XL16'3FF00000000000000000000000000000'
 DC CL48'DIDBR finite test 24b'
 DC XL16'3FF00000000000000000000000000000'
 DC CL48'DIDBR finite test 24b'
 DC XL16'3FF00000000000000000000000000000'
 DC CL48'DIDBR finite test 25a'
 DC XL16'3FF00000000000000000000000000000'
 DC CL48'DIDBR finite test 25a'
 DC XL16'3FF00000000000000000000000000000'
 DC CL48'DIDBR finite test 25b'
 DC XL16'3FF00000000000000000000000000000'
 DC CL48'DIDBR finite test 25b'
 DC XL16'3FF00000000000000000000000000000'
 DC CL48'DIDBR finite test 26a'
 DC XL16'40000000000000000000000000000000'
 DC CL48'DIDBR finite test 26a'
 DC XL16'40000000000000000000000000000000'
 DC CL48'DIDBR finite test 26b'
 DC XL16'40000000000000000000000000000000'
 DC CL48'DIDBR finite test 26b'
 DC XL16'40000000000000000000000000000000'
 DC CL48'DIDBR finite test 27a'
 DC XL16'BFF00000000000003FF0000000000000'
 DC CL48'DIDBR finite test 27a'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR finite test 27b'
 DC XL16'BFF00000000000003FF0000000000000'
 DC CL48'DIDBR finite test 27b'
 DC XL16'40080000000000000000000000000000'
 DC CL48'DIDBR finite test 28a'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR finite test 28a'
 DC XL16'40100000000000000000000000000000'
 DC CL48'DIDBR finite test 28b'
 DC XL16'00000000000000003FF0000000000000'
 DC CL48'DIDBR finite test 28b'
 DC XL16'40100000000000000000000000000000'
 DC CL48'DIDBR finite test 29a'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'DIDBR finite test 29a'
 DC XL16'40140000000000000000000000000000'
 DC CL48'DIDBR finite test 29b'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'DIDBR finite test 29b'
 DC XL16'40140000000000000000000000000000'
 DC CL48'DIDBR finite test 30a'
 DC XL16'C0000000000000004000000000000000'
 DC CL48'DIDBR finite test 30a'
 DC XL16'40180000000000000000000000000000'
 DC CL48'DIDBR finite test 30b'
 DC XL16'C0000000000000004000000000000000'
 DC CL48'DIDBR finite test 30b'
 DC XL16'40180000000000000000000000000000'
 DC CL48'DIDBR finite test 31a'
 DC XL16'BFF00000000000004000000000000000'
 DC CL48'DIDBR finite test 31a'
 DC XL16'401C0000000000000000000000000000'
 DC CL48'DIDBR finite test 31b'
 DC XL16'BFF00000000000004000000000000000'
 DC CL48'DIDBR finite test 31b'
 DC XL16'401C0000000000000000000000000000'
 DC CL48'DIDBR finite test 32a'
 DC XL16'00000000000000004000000000000000'
 DC CL48'DIDBR finite test 32a'
 DC XL16'40200000000000000000000000000000'
 DC CL48'DIDBR finite test 32b'
 DC XL16'00000000000000004000000000000000'
 DC CL48'DIDBR finite test 32b'
 DC XL16'40200000000000000000000000000000'
 DC CL48'DIDBR finite test 33a'
 DC XL16'4010000000000000C010000000000000'
 DC CL48'DIDBR finite test 33a'
 DC XL16'40440000000000000000000000000000'
 DC CL48'DIDBR finite test 33b'
 DC XL16'4010000000000000C010000000000000'
 DC CL48'DIDBR finite test 33b'
 DC XL16'40440000000000000000000000000000'
 DC CL48'DIDBR finite test 34a'
 DC XL16'0000000000000000630FFFFFFFFFFFFF'
 DC CL48'DIDBR finite test 34a'
 DC XL16'1FEFFFFFFFFFFFFF0000000000000000'
 DC CL48'DIDBR finite test 34b'
 DC XL16'0000000000000000630FFFFFFFFFFFFF'
 DC CL48'DIDBR finite test 34b'
 DC XL16'1FEFFFFFFFFFFFFF0000000000000000'
 DC CL48'DIDBR finite test 35a'
 DC XL16'7CA000000000000062F5555555555554'
 DC CL48'DIDBR finite test 35a'
 DC XL16'7CA00000000000000000000000000000'
 DC CL48'DIDBR finite test 35b'
 DC XL16'7CA000000000000062F5555555555554'
 DC CL48'DIDBR finite test 35b'
 DC XL16'7CA00000000000000000000000000000'
 DC CL48'DIDBR finite test 36a'
 DC XL16'00000000000000013FF0000000000000'
 DC CL48'DIDBR finite test 36a'
 DC XL16'000FFFFFFFFFFFFF0000000000000000'
 DC CL48'DIDBR finite test 36b'
 DC XL16'5CD00000000000003FF0000000000000'
 DC CL48'DIDBR finite test 36b'
 DC XL16'5CD00000000000000000000000000000'
 DC CL48'DIDBR finite test 37a'
 DC XL16'40100000000000004355555555555555'
 DC CL48'DIDBR finite test 37a'
 DC XL16'43700000000000000000000000000000'
 DC CL48'DIDBR finite test 37b'
 DC XL16'40100000000000004355555555555555'
 DC CL48'DIDBR finite test 37b'
 DC XL16'43700000000000000000000000000000'
 DC CL48'DIDBR finite test 38a'
 DC XL16'40020000000000000000000000000000'
 DC CL48'DIDBR finite test 38a'
 DC XL16'40020000000000000000000000000000'
 DC CL48'DIDBR finite test 38b'
 DC XL16'40020000000000000000000000000000'
 DC CL48'DIDBR finite test 38b'
 DC XL16'40020000000000000000000000000000'
LBFPOUT_NUM EQU (*-LBFPOUT_GOOD)/64
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
         DC    A(LBFPNFOT)
         DC    A(LBFPNFOT_GOOD)
         DC    A(LBFPNFOT_NUM)
*
         DC    A(LBFPNFFL)
         DC    A(LBFPNFFL_GOOD)
         DC    A(LBFPNFFL_NUM)
*
         DC    A(SBFPRMO)
         DC    A(SBFPRMO_GOOD)
         DC    A(SBFPRMO_NUM)
*
         DC    A(SBFPRMOF)
         DC    A(SBFPRMOF_GOOD)
         DC    A(SBFPRMOF_NUM)
*
         DC    A(LBFPRMO)
         DC    A(LBFPRMO_GOOD)
         DC    A(LBFPRMO_NUM)
*
         DC    A(LBFPRMOF)
         DC    A(LBFPRMOF_GOOD)
         DC    A(LBFPRMOF_NUM)
*
         DC    A(SBFPOUT)
         DC    A(SBFPOUT_GOOD)
         DC    A(SBFPOUT_NUM)
*
         DC    A(SBFPFLGS)
         DC    A(SBFPFLGS_GOOD)
         DC    A(SBFPFLGS_NUM)
*
         DC    A(LBFPFLGS)
         DC    A(LBFPFLGS_GOOD)
         DC    A(LBFPFLGS_NUM)
*
         DC    A(LBFPOUT)
         DC    A(LBFPOUT_GOOD)
         DC    A(LBFPOUT_NUM)
*
VERIFLEN EQU   (*-VERIFTAB)/12        #of table entries
                                                                EJECT
         END
