  TITLE 'bfp-020-multlonger.asm: Test IEEE Multiply'
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
***********************************************************************
         SPACE 2
***********************************************************************
*
*                      bfp-020-multlonger.asm 
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
*
*  Locations for results
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
ENDLABL  EQU   STRTLABL+X'3400'
         PADCSECT ENDLABL
         END
