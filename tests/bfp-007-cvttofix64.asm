  TITLE 'bfp-007-cvttofix64.asm: Test IEEE Convert To Fixed (int-64)'
***********************************************************************
*
*Testcase IEEE CONVERT TO FIXED 64
*  Test case capability includes ieee exceptions trappable and
*  otherwise.  Test result, FPCR flags, and DXC saved for all tests.  
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*                      bfp-007-cvttofix64.asm 
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
* Tests the following six conversion instructions
*   CONVERT TO FIXED (short BFP to int-64, RRE)
*   CONVERT TO FIXED (long BFP to int-64, RRE) 
*   CONVERT TO FIXED (extended BFP to int-64, RRE)  
*   CONVERT TO FIXED (short BFP to int-64, RRF-e)
*   CONVERT TO FIXED (long BFP to int-64, RRF-e)
*   CONVERT TO FIXED (extended BFP to int-64, RRF-e)  
*
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R 
* commands. 
* 
* Test Case Order
* 1) Short BFP to Int-64 
* 2) Short BFP to Int-64 with all rounding modes
* 3) Long BFP Int-64
* 3) Long BFP Int-64 with all rounding modes
* 4) Extended BFP to Int-64 
* 4) Extended BFP to Int-64 with all rounding modes
*
* Provided test data is:
*       1, 2, 4, -2, QNaN, SNaN, 2 147 483 648, -2 147 483 648.
*   The last two values will trigger inexact exceptions when converted
*   to int-64.  Underflow does not get raised during Convert To Fixed.
* Provided test data for rounding tests:
*   -9.5, -5.5, -2.5, -1.5, -0.5, +0.5, +1.5, +2.5, +5.5, +9.5
*   This data is taken from Table 9-11 on page 9-16 of SA22-7832-10.
*   While the table illustrates LOAD FP INTEGER, the same results 
*   should be generated when creating an int-32 or int-64 integer.
*   Additional rounding test cases are provided to test boundary cases.
*
* Note that three input test data sets are provided, one each for 
*   short, long, and extended precision BFP.  All are converted to 
*   int-64. 
*
* Also tests the following floating point support instructions
*   LOAD  (Short)
*   LOAD  (Long)
*   LOAD FPC
*   SET BFP ROUNDING MODE 2-BIT
*   SET BFP ROUNDING MODE 3-BIT
*   STORE (Short)
*   STORE (Long)
*   STORE FPC
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
BFPCVTTF START 0
R0       EQU   0
R1       EQU   1
R2       EQU   2
R3       EQU   3
R4       EQU   4
R5       EQU   5
R6       EQU   6
R7       EQU   7
R8       EQU   8
R9       EQU   9
R10      EQU   10
R11      EQU   11
R12      EQU   12
R13      EQU   13
R14      EQU   14
R15      EQU   15
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
         ORG   BFPCVTTF+X'8E'      Program check interrution code
PCINTCD  DS    H
*
PCOLDPSW EQU   BFPCVTTF+X'150'     z/Arch Program check old PSW
*
         ORG   BFPCVTTF+X'1A0'     z/Arch Restart PSW
         DC    X'0000000180000000',AD(START)   
*
         ORG   BFPCVTTF+X'1D0'     z/Arch Program check old PSW
         DC    X'0000000000000000',AD(PROGCHK)
* 
* Program check routine.  If Data Exception, continue execution at
* the instruction following the program check.  Otherwise, hard wait.
* No need to collect data.  All interesting DXC stuff is captured
* in the FPCR.
*
         ORG   BFPCVTTF+X'200'
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
START    STCTL R0,R0,CTLR0    Store CR0 to enable AFP
         OI    CTLR0+1,X'04'  Turn on AFP bit
         LCTL  R0,R0,CTLR0    Reload updated CR0
*
* Short BFP Input testing
*
         LA    R10,SHORTS     Point to short BFP test inputs
         BAS   R13,CGEBR      Convert values to fixed from short BFP
         LA    R10,RMSHORTS   Point to inputs for rounding mode tests
         BAS   R13,CGEBRA     Convert using all rounding mode options
*
* Short BFP Input testing
*
         LA    R10,LONGS      Point to long BFP test inputs
         BAS   R13,CGDBR      Convert values to fixed from long BFP
         LA    R10,RMLONGS    Point to inputs for rounding mode tests
         BAS   R13,CGDBRA     Convert using all rounding mode options
*
* Short BFP Input testing
*
         LA    R10,EXTDS      Point to extended BFP test inputs
         BAS   R13,CGXBR      Convert values to fixed from extended
         LA    R10,RMEXTDS    Point to inputs for rounding mode tests
         BAS   R13,CGXBRA     Convert using all rounding mode options
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
* Input values parameter list, four fullwords: 
*      1) Count, 
*      2) Address of inputs, 
*      3) Address to place results, and
*      4) Address to place DXC/Flags/cc values.  
*
         ORG   BFPCVTTF+X'300'
SHORTS   DS    0F           Inputs for short BFP testing
         DC    A(SBFPCT/4)
         DC    A(SBFPIN)
         DC    A(SINTOUT)
         DC    A(SINTFLGS)
*
LONGS    DS    0F           Inputs for long BFP testing
         DC    A(LBFPCT/8)
         DC    A(LBFPIN)
         DC    A(LINTOUT)
         DC    A(LINTFLGS)
*
EXTDS    DS    0F           Inputs for Extended BFP testing
         DC    A(XBFPCT/16)
         DC    A(XBFPIN)
         DC    A(XINTOUT)
         DC    A(XINTFLGS)
*
RMSHORTS DC    A(SBFPRMCT/4)
         DC    A(SBFPINRM)  Short BFP rounding mode test inputs
         DC    A(SINTRMO)   Short BFP rounding mode test results
         DC    A(SINTRMOF)  Short BFP rounding mode test flags
*
RMLONGS  DC    A(LBFPRMCT/8)
         DC    A(LBFPINRM)  Long BFP rounding mode test inputs
         DC    A(LINTRMO)   Long BFP rounding mode test results
         DC    A(LINTRMOF)  Long BFP rounding mode test flags
*
RMEXTDS  DC    A(XBFPRMCT/16)
         DC    A(XBFPINRM)  Extended BFP rounding mode test inputs
         DC    A(XINTRMO)   Extended BFP rounding mode test results
         DC    A(XINTRMOF)  Space for rounding mode test flags
         EJECT
***********************************************************************
*
* Convert short BFP to integer-64 format.  A pair of results is 
* generated for each input: one with all exceptions non-trappable, and 
* the second with all exceptions trappable.   The FPCR and condition 
* code is stored for each result.
*
***********************************************************************
          SPACE 2
CGEBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR8,0(,R3)   Get short BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CGEBR R1,0,FPR8     Cvt float in FPR8 to Int in GPR1
         STG   R1,0(,R7)     Store int-64 result
         STFPC 0*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(0*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGTR      Set exceptions trappable
         XGR   R1,R1         Clear any residual result in R1
         SPM   R1            Clear out any residual nz condition code
         CGEBR R1,0,FPR8     Cvt float in FPR8 to Int in GPR1
         STG   R1,8(,R7)     Store short BFP result
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save CC as low byte of FPCR
*
         LA    R3,4(,R3)     Point to next input value
         LA    R7,2*8(,R7)   Point to next int-64 converted value pair
         LA    R8,2*4(,R8)   Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert short BFP to integers using each possible rounding mode.  
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPC with the 
* IEEE Inexact exception supressed.  SRNM (2-bit) is used  for the 
* first two FPCR-controlled tests and SRNMB (3-bit) is used for the 
* last two To get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section; 
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
CGEBRA   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR8,0(,R3)   Get short BFP test value
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 1             SET FPCR to RZ, towards zero.  
         CGEBRA R1,0,FPR8,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,0*8(,R7)   Store integer-64 result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,3(,R8)     Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 2             SET FPCR to RP, to +infinity
         CGEBRA R1,0,FPR8,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,1*8(,R7)   Store integer-64 result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save cccas low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, to -infinity
         CGEBRA R1,0,FPR8,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,2*8(,R7)   Store integer-64 result
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(2*4)+3(,R8)  Save cccas low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RPS, Prepare for Shorter Precision
         CGEBRA R1,0,FPR8,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,3*8(,R7)   Store integer-64 result
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(3*4)+3(,R8)  Save cccas low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGEBRA R1,1,FPR8,B'0000'  RNTA, to nearest, ties away
         STG   R1,4*8(,R7)   Store integer-64 result
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(4*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGEBRA R1,3,FPR8,B'0000'  RFS, prepare for shorter precision
         STG   R1,5*8(,R7)   Store integer-64 result
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(5*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGEBRA R1,4,FPR8,B'0000'  RNTE, to nearest, ties to even
         STG   R1,6*8(,R7)   Store integer-64 result
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(6*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGEBRA R1,5,FPR8,B'0000'  RZ, toward zero
         STG   R1,7*8(,R7)   Store integer-64 result
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(7*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGEBRA R1,6,FPR8,B'0000'  RP, to +inf
         STG   R1,8*8(,R7)   Store integer-64 result
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(8*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGEBRA R1,7,FPR8,B'0000'  RM, to -inf
         STG   R1,9*8(,R7)   Store integer-64 result
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(9*4)+3(,R8)  Save CC as low byte of FPCR
*
         LA    R3,4(,R3)     Point to next input value
         LA    R7,10*8(,R7)  Point to next short BFP converted values
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert long BFP inputs to integer-64.  A pair of results is 
* generated for each input: one with all exceptions non-trappable, and 
* the second with all exceptions trappable.   The FPCR and condition 
* code is stored for each result.  
*
***********************************************************************
         SPACE 2
CGDBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR8,0(,R3)   Get long BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGDBR R1,0,FPR8     Cvt float in FPR8 to Int in GPR1
         STG   R1,0(,R7)     Store integer-64 result
         STFPC 0*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(0*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGTR      Set exceptions trappable, clear flags
         XGR   R1,R1         Clear any residual result in R1
         SPM   R1            Clear out any residual nz condition code
         CGDBR R1,0,FPR8     Cvt float in FPR8 to Int in GPR1
         STG   R1,8(,R7)     Store integer-64 result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save CC as low byte of FPCR
*
         LA    R3,8(,R3)     Point to next input values
         LA    R7,16(,R7)    Point to next int-64 converted value pair
         LA    R8,8(,R8)     Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert long BFP to integers using each possible rounding mode.
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPC with the 
* IEEE Inexact exception supressed.  SRNM (2-bit) is used  for the 
* first two FPCR-controlled tests and SRNMB (3-bit) is used for the 
* last two To get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section; 
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
CGDBRA   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR8,0(,R3)   Get long BFP test value
*
*  Cvt float in FPFPR8 to integer-64
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 1             SET FPCR to RZ, towards zero.  
         CGDBRA R1,0,FPR8,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,0*8(,R7)   Store integer-64 result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,3(,R8)     Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 2             SET FPCR to RP, to +infinity
         CGDBRA R1,0,FPR8,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,1*8(,R7)   Store integer-64 result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, to -infinity
         CGDBRA R1,0,FPR8,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,2*8(,R7)   Store integer-64 result
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(2*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RPS, Prepare for Shorter Precision
         CGDBRA R1,0,FPR8,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,3*8(,R7)   Store integer-64 result
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(3*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGDBRA R1,1,FPR8,B'0000'  RNTA, to nearest, ties away
         STG   R1,4*8(,R7)   Store integer-64 result
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(4*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGDBRA R1,3,FPR8,B'0000'  RFS, prepare for shorter precision
         STG   R1,5*8(,R7)   Store integer-64 result
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(5*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGDBRA R1,4,FPR8,B'0000'  RNTE, to nearest, ties to even
         STG   R1,6*8(,R7)   Store integer-64 result
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(6*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGDBRA R1,5,FPR8,B'0000'  RZ, toward zero
         STG   R1,7*8(,R7)   Store integer-64 result
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(7*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGDBRA R1,6,FPR8,B'0000'  RP, to +inf
         STG   R1,8*8(,R7)   Store integer-64 result
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(8*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGDBRA R1,7,FPR8,B'0000'  RM, to -inf
         STG   R1,9*8(,R7)   Store integer-64 result
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(9*4)+3(,R8)  Save CC as low byte of FPCR
*
         LA    R3,8(,R3)     Point to next input value
         LA    R7,10*8(,R7)  Point to next long BFP converted values
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert extended BFP to integer-64.  A pair of results is generated
* for each input: one with all exceptions non-trappable, and the 
* second with all exceptions trappable.   The FPCR and condition code
* are stored for each result.  
*
***********************************************************************
          SPACE 2
CGXBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR8,0(,R3)   Get extended BFP test value part 1
         LD    FPR10,8(,R3)  Get extended BFP test value part 1
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGXBR R1,0,FPR8     Cvt float in FPR8-FPR10 to Int-64 in GPR1
         STG   R1,0(,R7)     Store integer-64 result
         STFPC (0*4)(R8)     Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(0*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGTR      Set exceptions trappable, clear flags
         XGR   R1,R1         Clear any residual result in R1
         SPM   R1            Clear out any residual nz condition code
         CGXBR R1,0,FPR8     Cvt float in FPR8-FPR10 to Int-64 in GPR1
         STG   R1,8(,R7)     Store integer-64 result
         STFPC (1*4)(R8)     Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save CC as low byte of FPCR
*
         LA    R3,16(,R3)    Point to next extended BFP input value
         LA    R7,16(,R7)    Point to next int-64 converted value pair
         LA    R8,8(,R8)     Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert extended BFP to integers using each possible rounding mode.
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPC with the 
* IEEE Inexact exception supressed.  SRNM (2-bit) is used  for the 
* first two FPCR-controlled tests and SRNMB (3-bit) is used for the 
* last two To get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section; 
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
CGXBRA   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR8,0(,R3)    Get extended BFP test value part 1
         LD    FPR10,8(,R3)    Get extended BFP test value part 2
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 1             SET FPCR to RZ,  towards zero.  
         CGXBRA R1,0,FPR8,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,0*8(,R7)   Store integer-64 result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,3(,R8)     Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 2             SET FPCR to RP,  to +infinity
         CGXBRA R1,0,FPR8,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,1*8(,R7)   Store integer-64 result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, to -infinity
         CGXBRA R1,0,FPR8,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,2*8(,R7)   Store integer-64 result
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(2*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RPS, Prepare for Shorter Precision
         CGXBRA R1,0,FPR8,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,3*8(,R7)   Store integer-64 result
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(3*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGXBRA R1,1,FPR8,B'0000'  RNTA, to nearest, ties away
         STG   R1,4*8(,R7)   Store integer-64 result
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(4*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGXBRA R1,3,FPR8,B'0000'  RFS, prepare for shorter precision
         STG   R1,5*8(,R7)   Store integer-64 result
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(5*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGXBRA R1,4,FPR8,B'0000'  RNTE to nearest, ties to even
         STG   R1,6*8(,R7)   Store integer-64 result
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(6*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGXBRA R1,5,FPR8,B'0000'  RZ toward zero
         STG   R1,7*8(,R7)   Store integer-64 result
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(7*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGXBRA R1,6,FPR8,B'0000'  to +inf
         STG   R1,8*8(,R7)   Store integer-64 result
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(8*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CGXBRA R1,7,FPR8,B'0000'  to -inf
         STG   R1,9*8(,R7)   Store integer-64 result
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(9*4)+3(,R8)  Save CC as low byte of FPCR
*
         LA    R3,16(,R3)   Point to next input value
         LA    R7,10*8(,R7) Point to next long BFP converted values
         LA    R8,12*4(,R8) Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Floating point inputs for Convert From Fixed testing.  The same test 
* values in the appropriate input format are used for short, long, 
* and extended format tests.  The last four values should generate 
* exceptions.
*
***********************************************************************
         SPACE 2
*
* Inputs for basic tests of short BFP to int-64
*
SBFPIN   DS    0F                Inputs for short BFP testing
         DC    X'3F800000'  +1.0
         DC    X'40000000'  +2.0
         DC    X'40800000'  +4.0
         DC    X'C0000000'  -2.0
         DC    X'7F810000'  SNaN
         DC    X'7FC10000'  QNaN
         DC    X'5F000000'  +max int-64 + 1
*                                  +9,223,372,036,854,775,807 + 1
         DC    X'DF000001'  -max int-64 - 2
*                                  -9,223,372,036,854,775,807 - 2
         DC    X'5EFFFFFF'  Largest short BFP that fits in int-64
*                                  +9,223,371,487,098,961,920
*                                  = 0x7FFFFF80000000000
SBFPCT   EQU   *-SBFPIN     Count of short BFP in list * 4
*
* Inputs for exhaustive rounding mode tests of short BFP to int-64
*
SBFPINRM DS    0F
         DC    X'C1180000'         -9.5
         DC    X'C0B00000'         -5.5
         DC    X'C0200000'         -2.5
         DC    X'BFC00000'         -1.5
         DC    X'BF000000'         -0.5
         DC    X'3F000000'         +0.5
         DC    X'3FC00000'         +1.5
         DC    X'40200000'         +2.5
         DC    X'40B00000'         +5.5
         DC    X'41180000'         +9.5
         DC    X'3F400000'         +0.75
         DC    X'3E800000'         +0.25
         DC    X'BF400000'         -0.75
         DC    X'BE800000'         -0.25
SBFPRMCT EQU   *-SBFPINRM   Count of short BFP * 4 for rounding tests
*
* Inputs for basic tests of long BFP to int-64
*
LBFPIN   DS    0F                Inputs for long BFP testing
         DC    X'3FF0000000000000'    +1.0
         DC    X'4000000000000000'    +2.0
         DC    X'4010000000000000'    +4.0
         DC    X'C000000000000000'    -2.0
         DC    X'7FF0100000000000'    SNaN
         DC    X'7FF8100000000000'    QNaN
         DC    X'43E0000000000000'   +max int-64 + 1.  
*                          (+9,223,372,036,854,775,807 + 1)
         DC    X'C3E0000000100000'   -max int-64 - 2
*                          (-9,223,372,036,854,775,807 - 2)
         DC    X'43DFFFFFFFFFFFFF'    Largest long BFP that fits in 
*                          ..int-64: 9,223,372,036,854,774,784,
*                          .. 0x7FFFFFFFFFFFF800
LBFPCT   EQU   *-LBFPIN     Count of long BFP in list * 8
*
* Inputs for exhaustive rounding mode tests of long BFP to int-64
*
LBFPINRM DS    0F
         DC    X'C023000000000000'         -9.5
         DC    X'C016000000000000'         -5.5
         DC    X'C004000000000000'         -2.5
         DC    X'BFF8000000000000'         -1.5
         DC    X'BFE0000000000000'         -0.5
         DC    X'3FE0000000000000'         +0.5
         DC    X'3FF8000000000000'         +1.5
         DC    X'4004000000000000'         +2.5
         DC    X'4016000000000000'         +5.5
         DC    X'4023000000000000'         +9.5
         DC    X'3FE8000000000000'         +0.75
         DC    X'3FD0000000000000'         +0.25
         DC    X'BFE8000000000000'         -0.75
         DC    X'BFD0000000000000'         -0.25
LBFPRMCT EQU   *-LBFPINRM   Count of long BFP * 8 for rounding tests
*
* Inputs for basic tests of extended BFP to int-64
*
XBFPIN   DS    0D                Inputs for long BFP testing
         DC    X'3FFF0000000000000000000000000000'    +1.0
         DC    X'40000000000000000000000000000000'    +2.0
         DC    X'40010000000000000000000000000000'    +4.0
         DC    X'C0000000000000000000000000000000'    -2.0
         DC    X'7FFF0100000000000000000000000000'    SNaN
         DC    X'7FFF8100000000000000000000000000'    QNaN
         DC    X'403E0000000000000000000000000000'   +max int-64 + 1
*                                          (+9223372036854775807 + 1)
         DC    X'C03E0000000100000000000000000000'   -max int-64 - 2
*                                          (-9223372036854775807 - 2)
         DC    X'403DFFFFFFFFFFFFFFFC000000000000'   +max int-64
XBFPCT   EQU   *-XBFPIN     Count of extended BFP in list * 16
*
* Inputs for exhaustive rounding mode tests of extended BFP to int-64
*
XBFPINRM DS    0D
         DC    X'C0023000000000000000000000000000'         -9.5
         DC    X'C0016000000000000000000000000000'         -5.5
         DC    X'C0004000000000000000000000000000'         -2.5
         DC    X'BFFF8000000000000000000000000000'         -1.5
         DC    X'BFFE0000000000000000000000000000'         -0.5
         DC    X'3FFE0000000000000000000000000000'         +0.5
         DC    X'3FFF8000000000000000000000000000'         +1.5
         DC    X'40004000000000000000000000000000'         +2.5
         DC    X'40016000000000000000000000000000'         +5.5
         DC    X'40023000000000000000000000000000'         +9.5
         DC    X'3FFE8000000000000000000000000000'         +0.75
         DC    X'3FFD0000000000000000000000000000'         +0.25
         DC    X'BFFE8000000000000000000000000000'         -0.75
         DC    X'BFFD0000000000000000000000000000'         -0.25
         DC    X'403DFFFFFFFFFFFFFFFE000000000000'   max int-64 + 0.5
*                     9,223,372,036,854,775,807.5, overflows on some
*                     ..but not all rounding modes
XBFPRMCT EQU   *-XBFPINRM   Count of ext'd BFP * 16 for rounding tests
*
*  Locations for results
*
SINTOUT  EQU   BFPCVTTF+X'1000'    Integer-64 values from short BFP
*                                  ..8 pairs used, room for 32
SINTFLGS EQU   BFPCVTTF+X'1200'    FPCR flags and DXC from short BFP
*                                  ..8 pairs used,room for 32
SINTRMO  EQU   BFPCVTTF+X'1300'    Short rounding mode test results
*                                  ..10 sets used, room for 16
SINTRMOF EQU   BFPCVTTF+X'1800'    Short rounding mode FPCR contents
*                                  ..10 sets used, room for 16
*
LINTOUT  EQU   BFPCVTTF+X'2000'    Integer-64 values from long BFP
*                                  ..8 pairs used, room for 32
LINTFLGS EQU   BFPCVTTF+X'2200'    FPCR flags and DXC from long BFP
*                                  ..8 pairs used,room for 32
LINTRMO  EQU   BFPCVTTF+X'2300'    Long rounding mode test results
*                                  ..10 sets used, room for 16
LINTRMOF EQU   BFPCVTTF+X'2800'    Long rounding mode FPCR contents
*                                  ..10 sets used, room for 16
*
XINTOUT  EQU   BFPCVTTF+X'3000'    Integer-64 values from extended BFP
*                                  ..8 pairs used, room for 32
XINTFLGS EQU   BFPCVTTF+X'3200'    FPCR flags and DXC from extended BFP
*                                  ..8 pairs used, room for 32
XINTRMO  EQU   BFPCVTTF+X'3300'    Extended rounding mode test results
*                                  ..10 sets used, room for 16
XINTRMOF EQU   BFPCVTTF+X'3800'    Extended rndg mode FPCR contents
*                                  ..10 sets used, room for 16
*
ENDLABL  EQU   BFPCVTTF+X'3B00'    Next available location
         PADCSECT ENDLABL
         END
