  TITLE 'bfp-001-divtoint.asm: Test IEEE Divide To Integer'
***********************************************************************
*
*Testcase IEEE DIVIDE TO INTEGER
*  Test case capability includes IEEE exceptions trappable and 
*  otherwise. Test results, FPCR flags, and any DXC are saved for all 
*  tests.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*                       bfp-001-divtoint.asm 
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
         JNE   PCNOTDTA       ..no, hardwait
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
         LTR   R14,R14        Return address provided?
         BNZR  R14            ..Yes, return to z/CMS test rig.  
         LPSWE WAITPSW        All done
*
         DS    0D             Ensure correct alignment for psw
WAITPSW  DC    X'0002000000000000',AD(0)  Normal end - disabled wait
HARDWAIT DC    X'0002000000000000',XL6'00',X'DEAD' Abnormal end
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
SBFPNFCT EQU   (*-SBFPNFIN)4/2     Count of short BFP in list
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
*
*
*  Locations for results
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
LBFPOUT  EQU   STRTLABL+X'B000'    Integer long BFP finite results
*                                  ..room for 64 tests, 6 used
LBFPFLGS EQU   STRTLABL+X'AC00'    FPCR flags and DXC from long BFP
*                                  ..room for 64 tests, 6 used
*
*
ENDLABL  EQU   STRTLABL+X'C000'
         PADCSECT ENDLABL
         END
