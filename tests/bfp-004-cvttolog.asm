  TITLE 'bfp-004-cvttolog.asm: Test IEEE Convert To Logical (uint-32)'
***********************************************************************
*
*Testcase IEEE CONVERT TO LOGICAL 32
*  Test case capability includes ieee exceptions trappable and 
*  otherwise.  Test results, FPCR flags, DXC, and condition codes are 
*  saved for all tests. 
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*                       bfp-004-cvttolog.asm 
*
*        This assembly-language source file is part of the
*        Hercules Binary Floating Point Validation Package 
*                        by Stephen R. Orso
*
* Copyright 2016 by Stephen R Orso.
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
*   CONVERT TO LOGICAL (short BFP to uint-32, RRF-e)
*   CONVERT TO LOGICAL (long BFP to uint-32, RRF-e) 
*   CONVERT TO LOGICAL (extended BFP to uint-32, RRF-e)  
*
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R 
* commands.
* 
* Test Case Order
* 1) Short BFP to uint-32 
* 2) Short BFP to uint-32 with all rounding modes
* 3) Long BFP uint-32
* 3) Long BFP uint-32 with all rounding modes
* 4) Extended BFP to uint-32 
* 4) Extended BFP to uint-32 with all rounding modes
*
* Three input test data sets are provided, one each for short, long,
*   and extended precision BFP.  All are converted to uint-32. 
*
* Provided test data is 1, 2, 4, 9, QNaN, SNaN, 4294967295.5.
*   The last three values will trigger inexact exceptions when 
*   converted to uint-32.  The last value is present only in the long 
*   and extended BFP test cases and should overflow a uint-32.  
* Provided test data for rounding tests is taken from Table 9-11 on 
*   page 9-16 of SA22-7832-10.  
*        -1.5, -0.5, +0.5, +1.5, +2.5, +5.5, +9.5.
*   While the table illustrates LOAD FP INTEGER, the same results 
*   should be generated when creating a uint-32 or uint-64 from BFP.  
* For long BFP and extended BFP rounding mode tests, ond additional 
*   test case is included: 4294967294.5.  This case rounds down to a 
*   maximum uint-32 and rounds up to overflow; it tests the case where 
*   the input is greater than a maximum uint-32 but rounds to a maximum
*   uint-32.  See Table 19-19 on page 19-26 of SA22-7832-10 for details
*   on this boundary condition test.
*
* Also tests the following floating point support instructions
*   LOAD  (Short)
*   LOAD  (Long)
*   LOAD FPC
*   SRNMB (Set BFP Rounding Mode 2-bit)
*   SRNMB (Set BFP Rounding Mode 3-bit)
*   STORE (Short)
*   STORE (Long)
*   STORE FPC
*
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
BFPCVTTL START 0
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
         ORG   BFPCVTTL+X'8E'      Program check interrution code
PCINTCD  DS    H
*
PCOLDPSW EQU   BFPCVTTL+X'150'     z/Arch Program check old PSW
*
         ORG   BFPCVTTL+X'1A0'     z/Arch Restart PSW
         DC    X'0000000180000000',AD(START)   
*
         ORG   BFPCVTTL+X'1D0'     z/Arch Program check old PSW
         DC    X'0000000000000000',AD(PROGCHK)
* 
* Program check routine.  If Data Exception, continue execution at
* the instruction following the program check.  Otherwise, hard wait.
* No need to collect data.  All interesting DXC stuff is captured
* in the FPCR.
*
         ORG   BFPCVTTL+X'200'
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
START    STCTL R0,R0,CTLR0    Store CR0 to enable AFP
         OI    CTLR0+1,X'04'  Turn on AFP bit
         LCTL  R0,R0,CTLR0    Reload updated CR0
*
* Short BFP Input testing
*
         LA    R10,SHORTS     Point to short BFP test inputs
         BAS   R13,CLFEBR     Convert values to uint-32 from short BFP
         LA    R10,RMSHORTS   Point to inputs for rounding mode tests
         BAS   R13,CLFEBRA    Convert using all rounding mode options
*
* Short BFP Input testing
*
         LA    R10,LONGS      Point to long BFP test inputs
         BAS   R13,CLFDBR     Convert values to uint-32 from long BFP
         LA    R10,RMLONGS    Point to inputs for rounding mode tests
         BAS   R13,CLFDBRA    Convert using all rounding mode options
*
* Short BFP Input testing
*
         LA    R10,EXTDS      Point to extended BFP test inputs
         BAS   R13,CLFXBR     Convert values to uint-32 from extended
         LA    R10,RMEXTDS    Point to inputs for rounding mode tests
         BAS   R13,CLFXBRA    Convert using all rounding mode options
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
         ORG   BFPCVTTL+X'300'
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
         DC    A(XINTRMOF)  Extended BFP rounding mode test flags
         EJECT
***********************************************************************
*
* Convert short BFP to uint-32 format.  A pair of results is generated
* for each input: one with all exceptions non-trappable, and the second
* with all exceptions trappable.   The FPCR and condition code is 
* stored for each result.
*
***********************************************************************
         SPACE 2
CLFEBR   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR0,0(,R3)   Get short BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CLFEBR R1,0,FPR0,0  Cvt float in FPR0 to uint-32 in GPR1
         ST    R1,0(,R7)     Store int-32 result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,3(,R8)     Save condition code as low byte of FPCR
*
         LFPC  FPCREGTR      Set exceptions trappable
         XR    R1,R1         Clear any residual result in R1
         SPM   R1            Clear out any residual nz condition code
         CLFEBR R1,0,FPR0,0  Cvt float in FPR0 to uint-32 in GPR1
         ST    R1,4(,R7)     Store short BFP result
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,7(,R8)     Save condition code as low byte of FPCR
*
         LA    R3,4(,R3)     Point to next input values
         LA    R7,8(,R7)     Point to next int-32 converted value pair
         LA    R8,8(,R8)     Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert short BFP to integers using each possible rounding mode.  
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPCR with
* the IEEE Inexact exception supressed.  SRNM (2-bit) is used  for
* the first two FPCR-controlled tests and SRNMB (3-bit) is used for
* the last two To get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section;
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
CLFEBRA  LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR0,0(,R3)    Get short BFP test value
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  1             SET FPCR to RZ, towards zero.  
         CLFEBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         ST    R1,0*4(,R7)   Store uint-32 result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,3(,R8)     Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPCR to RP, to +infinity
         CLFEBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         ST    R1,1*4(,R7)   Store uint-32 result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, to -infinity
         CLFEBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         ST    R1,2*4(,R7)   Store uint-32 result
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(2*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RFS, Prepare for Shorter Precision
         CLFEBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         ST    R1,3*4(,R7)   Store uint-32 result
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(3*4)+3(,R8)  Save condition code as low byte of FPCR
*
* Test cases using rounding mode specified in the instruction M3 field
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFEBR R1,1,FPR0,B'0000'  RNTA, to nearest, ties away
         ST    R1,4*4(,R7)   Store uint-32 result
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(4*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFEBR R1,3,FPR0,B'0000'  RFS, prepare for shorter precision
         ST    R1,5*4(,R7)   Store uint-32 result
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(5*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFEBR R1,4,FPR0,B'0000'  RNTE, to nearest, ties to even
         ST    R1,6*4(,R7)   Store uint-32 result
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(6*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFEBR R1,5,FPR0,B'0000'  RZ, toward zero
         ST    R1,7*4(,R7)   Store uint-32 result
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(7*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFEBR R1,6,FPR0,B'0000'  RP, to +inf
         ST    R1,8*4(,R7)   Store uint-32 result
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(8*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFEBR R1,7,FPR0,B'0000'  RM, to -inf
         ST    R1,9*4(,R7)   Store uint-32 result
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(9*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LA    R3,4(,R3)     Point to next input value
         LA    R7,12*4(,R7)  Point to next uint-32 result set
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert long BFP inputs to uint-32.  A pair of results is generated
* for each input: one with all exceptions non-trappable, and the second
* with all exceptions trappable.   The FPCR and condition code is 
* stored for each result.  
*
***********************************************************************
         SPACE 2
CLFDBR   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)   Get long BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CLFDBR R1,0,FPR0,0  Cvt float in FPR0 to uint-32 in GPR1
         ST    R1,0(,R7)     Store long BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,3(,R8)     Save condition code as low byte of FPCR
*
         LFPC  FPCREGTR      Set exceptions trappable
         XR    R1,R1         Clear any residual result in R1
         SPM   R1            Clear out any residual nz condition code
         CLFDBR R1,0,FPR0,0  Cvt float in FPR0 to uint-32 in GPR1
         ST    R1,4(,R7)     Store int-32 result
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,7(,R8)     Save condition code as low byte of FPCR
*
         LA    R3,8(,R3)     Point to next input values
         LA    R7,8(,R7)     Point to next uint-32 converted value pair
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
* The first four tests use rounding modes specified in the FPCR with
* the IEEE Inexact exception supressed.  SRNM (2-bit) is used  for
* thefirst two FPCR-controlled tests and SRNMB (3-bit) is used for
* the last two To get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section;
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
CLFDBRA  LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)    Get long BFP test value
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  1             SET FPCR to RZ, towards zero.  
         CLFDBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         ST    R1,0*4(,R7)   Store uint-32 result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,3(,R8)     Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPCR to RP, to +infinity
         CLFDBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         ST    R1,1*4(,R7)   Store uint-32 result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, to -infinity
         CLFDBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         ST    R1,2*4(,R7)   Store uint-32 result
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(2*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RFS, Prepare for Shorter Precision
         CLFDBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         ST    R1,3*4(,R7)   Store uint-32 result
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(3*4)+3(,R8)  Save condition code as low byte of FPCR
*
* Test cases using rounding mode specified in the instruction M3 field
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFDBR R1,1,FPR0,B'0000'  RNTA, to nearest, ties away
         ST    R1,4*4(,R7)   Store uint-32 result
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(4*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFDBR R1,3,FPR0,B'0000'  RFS, prepare for shorter precision
         ST    R1,5*4(,R7)   Store uint-32 result
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(5*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFDBR R1,4,FPR0,B'0000'  RNTE, to nearest, ties to even
         ST    R1,6*4(,R7)   Store uint-32 result
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(6*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFDBR R1,5,FPR0,B'0000'  RZ, toward zero
         ST    R1,7*4(,R7)   Store uint-32 result
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(7*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFDBR R1,6,FPR0,B'0000'  RP, to +inf
         ST    R1,8*4(,R7)   Store uint-32 result
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(8*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFDBR R1,7,FPR0,B'0000'  RM, to -inf
         ST    R1,9*4(,R7)   Store uint-32 result
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(9*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LA    R3,8(,R3)     Point to next input values
         LA    R7,12*4(,R7)  Point to next long BFP converted values
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert extended BFP to uint-32.  A pair of results is generated
* for each input: one with all exceptions non-trappable, and the 
* second with all exceptions trappable.   The FPCR and condition code
* are stored for each result.  
*
***********************************************************************
         SPACE 2
CLFXBR   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)   Get extended BFP test value part 1
         LD    FPR2,8(,R3)   Get extended BFP test value part 1
         LFPC  FPCREGNT      Set exceptions non-trappable
         CLFXBR R1,0,FPR0,0  Cvt float in FPR0-FPR2 to uint-32 in GPR1
         ST    R1,0(,R7)     Store uint-32 result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,3(,R8)     Save condition code as low byte of FPCR
*
         LFPC  FPCREGTR      Set exceptions trappable
         XR    R1,R1         Clear any residual result in R1
         SPM   R1            Clear out any residual nz condition code
         CLFXBR R1,0,FPR0,0  Cvt float in FPR0-FPR2 to uint-32 in GPR1
         ST    R1,4(,R7)     Store uint-32 result
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,7(,R8)     Save condition code as low byte of FPCR
*
         LA    R3,16(,R3)    Point to next extended BFP input value
         LA    R7,8(,R7)     Point to next uint-32 converted value pair
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
* The first four tests use rounding modes specified in the FPCR with
* the IEEE Inexact exception supressed.  SRNM (2-bit) is used  for the 
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
CLFXBRA  LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)   Get extended BFP test value part 1
         LD    FPR2,8(,R3)   Get extended BFP test value part 2
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  1             Set FPCR to RZ, towards zero.
         CLFXBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         ST    R1,0*4(,R7)   Store uint-32 result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,3(,R8)     Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPCR to RP, to +infinity
         CLFXBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         ST    R1,1*4(,R7)   Store uint-32 result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, to -infinity
         CLFXBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         ST    R1,2*4(,R7)   Store uint-32 result
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(2*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RFS, Prepare for Shorter Precision
         CLFXBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         ST    R1,3*4(,R7)   Store uint-32 result
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(3*4)+3(,R8)  Save condition code as low byte of FPCR
*
* Test cases using rounding mode specified in the instruction M3 field
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFXBR R1,1,FPR0,B'0000'  RNTA, to nearest, ties away
         ST    R1,4*4(,R7)   Store uint-32 result
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(4*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFXBR R1,3,FPR0,B'0000'  RFS, prepare for shorter precision
         ST    R1,5*4(,R7)   Store uint-32 result
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(5*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFXBR R1,4,FPR0,B'0000'  RNTE, to nearest, ties to even
         ST    R1,6*4(,R7)   Store uint-32 result
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(6*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFXBR R1,5,FPR0,B'0000'  RZ, toward zero
         ST    R1,7*4(,R7)   Store uint-32 result
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(7*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFXBR R1,6,FPR0,B'0000'  RP, to +inf
         ST    R1,8*4(,R7)   Store uint-32 result
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(8*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLFXBR R1,7,FPR0,B'0000'  RM, to -inf
         ST    R1,9*4(,R7)   Store uint-32 result
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(9*4)+3(,R8)  Save condition code as low byte of FPCR
*
         LA    R3,16(,R3)    Point to next input value
         LA    R7,12*4(,R7)  Point to next long BFP converted values
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
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
* Inputs for short BFP testing of trap and non-trap results
*
SBFPIN   DS    0F            Inputs for short BFP testing
         DC    X'3F800000'    +1.0
         DC    X'40000000'    +2.0
         DC    X'40800000'    +4.0
         DC    X'7F810000'    SNaN
         DC    X'7FC10000'    QNaN
         DC    X'4F800000'    Max uint-32 + 1 rounded to short BFP
*                             ...4 294 967 296
         DC    X'4F7FFFFF'    Max uint-32 value representable 
*                             ...in short bfp (4 294 967 040)
         DC    X'3F400000'    +0.75
         DC    X'3E800000'    +0.25
SBFPCT   EQU   *-SBFPIN     Count of short BFP in list * 4
*
* Inputs for short BFP exhaustive rounding mode testing.
*
SBFPINRM DS    0F
         DC    X'BFC00000'    -1.5
         DC    X'BF000000'    -0.5
         DC    X'3F000000'    +0.5
         DC    X'3FC00000'    +1.5
         DC    X'40200000'    +2.5
         DC    X'40B00000'    +5.5
         DC    X'41180000'    +9.5
         DC    X'4F7FFFFF'    largest uint-32 value representable 
*                             ...in short bfp (4 294 967 040)
         DC    X'3F400000'    +0.75
         DC    X'3E800000'    +0.25
SBFPRMCT EQU   *-SBFPINRM   Count of short BFP for rounding tests * 4
*
* Inputs for long BFP testing of trap and non-trap results
*
LBFPIN   DS    0F                Inputs for long BFP testing
         DC    X'3FF0000000000000'    +1.0
         DC    X'4000000000000000'    +2.0
         DC    X'4010000000000000'    +4.0
         DC    X'7FF0100000000000'    SNaN
         DC    X'7FF8100000000000'    QNaN
         DC    X'41EFFFFFFFF00000'    max uint-32 + 0.5
*                                     ...4 294 967 295.5
*                                     Rounds up on RNTE, overflows
         DC    X'41EFFFFFFFECCCCD'    max uint-32 + 0.4
*                                     ...4 294 967 295.4, exceeds
*                                     uint-32, but rounds down to fit
         DC    X'3FE8000000000000'    +0.75
         DC    X'3FD0000000000000'    +0.25
LBFPCT   EQU   *-LBFPIN      Count of long BFP in list * 8
*
* Inputs for long BFP exhaustive rounding mode testing.
*
LBFPINRM DS    0F
         DC    X'BFF8000000000000'    -1.5
         DC    X'BFE0000000000000'    -0.5
         DC    X'3FE0000000000000'    +0.5
         DC    X'3FF8000000000000'    +1.5
         DC    X'4004000000000000'    +2.5
         DC    X'4016000000000000'    +5.5
         DC    X'4023000000000000'    +9.5
         DC    X'41EFFFFFFFF00000'    max uint-32 + 0.5
*                                     ...4 294 967 295.5
         DC    X'3FE8000000000000'    +0.75
         DC    X'3FD0000000000000'    +0.25
LBFPRMCT EQU   *-LBFPINRM   Count of long BFP for rounding tests * 8 
*
* Inputs for short BFP testing of trap and non-trap results
*
XBFPIN   DS    0D                Inputs for long BFP testing
         DC    X'3FFF0000000000000000000000000000'    +1.0
         DC    X'40000000000000000000000000000000'    +2.0
         DC    X'40010000000000000000000000000000'    +4.0
         DC    X'7FFF0100000000000000000000000000'    SNaN
         DC    X'7FFF8100000000000000000000000000'    QNaN
         DC    X'401EFFFFFFFF00000000000000000000'    max uint-32+0.5
*                                     ...4 294 967 295.5
*                                     Rounds up on RNTE, overflows
         DC    X'401EFFFFFFFECCCCCCCCCCCCCCCCCCCD'    max uint-32+0.4
*                                     ...4 294 967 295.4, exceeds
*                                     uint-32, but rounds down to fit
         DC    X'3FFE8000000000000000000000000000'         0.75
         DC    X'3FFD0000000000000000000000000000'         0.25
XBFPCT   EQU   *-XBFPIN     Count of extended BFP in list * 16
*
* Inputs for extended BFP exhaustive rounding mode testing.
*
XBFPINRM DS    0D
         DC    X'BFFF8000000000000000000000000000'   -1.5
         DC    X'BFFE0000000000000000000000000000'   -0.5
         DC    X'3FFE0000000000000000000000000000'   +0.5
         DC    X'3FFF8000000000000000000000000000'   +1.5
         DC    X'40004000000000000000000000000000'   +2.5
         DC    X'40016000000000000000000000000000'   +5.5
         DC    X'40023000000000000000000000000000'   +9.5
         DC    X'401EFFFFFFFF00000000000000000000'   max uint-32 + 0.5
*                                                (+4 294 967 295.5)
*    Above is always inexact, and may overflow based on rounding mode
         DC    X'3FFE8000000000000000000000000000'         0.75
         DC    X'3FFD0000000000000000000000000000'         0.25
XBFPRMCT EQU   *-XBFPINRM   Count of extended BFP rounding tests * 16
*
*  Locations for results
*
SINTOUT  EQU   BFPCVTTL+X'1000'    uint-32 values from short BFP
*                                  ..9 pairs used, room for 16
SINTFLGS EQU   BFPCVTTL+X'1100'    FPCR flags and DXC from short BFP
*                                  ..9 pairs used, room for 16
SINTRMO  EQU   BFPCVTTL+X'1200'    Short rounding mode test results
*                                  ..10 sets used, room for 20
SINTRMOF EQU   BFPCVTTL+X'1600'    Short rounding mode FPCR contents
*                                  ..10 sets used, room for 20
*
LINTOUT  EQU   BFPCVTTL+X'2000'    uint-32 values from long BFP
*                                  ..9 pairs used, room for 16
LINTFLGS EQU   BFPCVTTL+X'2100'    FPCR flags and DXC from long BFP
*                                  ..9 pairs used, room for 16
LINTRMO  EQU   BFPCVTTL+X'2200'    Long rounding mode test results
*                                  ..10 sets used, room for 20
LINTRMOF EQU   BFPCVTTL+X'2600'    Long rounding mode FPCR contents
*                                  ..10 sets used, room for 20
*
XINTOUT  EQU   BFPCVTTL+X'3000'    uint-32 values from extended BFP
*                                  ..9 pairs used, room for 16
XINTFLGS EQU   BFPCVTTL+X'3100'    FPCR flags and DXC from extended BFP
*                                  ..9 pairs used, room for 16
XINTRMO  EQU   BFPCVTTL+X'3200'    Extended rounding mode test results
*                                  ..10 sets used, room for 20
XINTRMOF EQU   BFPCVTTL+X'3600'    Extended rounding mode FPCR contents
*                                  ..10 sets used, room for 20
*
ENDLABL  EQU   BFPCVTTL+X'3A00'    end of test program
         PADCSECT ENDLABL          pad csect for non-asma assembly
         END
