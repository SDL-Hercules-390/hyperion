 TITLE 'bfp-013-comps.asm: Test IEEE Compare, Compare And Signal'
***********************************************************************
*
*Testcase IEEE Compare and Compare And Signal.
*  Exhaustively test results from the Compare and Compare And Signal 
*  instructions.  The Condition Code and FPC flags are saved for each
*  test value pair.  If an IEEE trap occurs, the DXC code is saved
*  instead of the Condition Code.
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
         LA    R10,SHORTC    Point to short BFP parameters
         BAS   R13,SBFPCOMP  Perform short BFP Compare
*
         LA    R10,LONGC    Point to long BFP parameters
         BAS   R13,LBFPCOMP  Perform long BFP Compare
*
         LA    R10,XTNDC     Point to extended BFP parameters
         BAS   R13,XBFPCOMP  Perform extended BFP Compare
*
         LTR   R14,R14       Return address provided?
         BNZR  R14           ..Yes, return to z/CMS test rig.  
         LPSWE WAITPSW       All done
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
*
* Locations for test results
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
*
ENDLABL  EQU   STRTLABL+X'3400'
         PADCSECT ENDLABL
*
         END