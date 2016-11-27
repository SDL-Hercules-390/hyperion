  TITLE 'bfp-015-sqrt.asm: Test IEEE Square Root'
***********************************************************************
*
*Testcase IEEE SQUARE ROOT
*  Test case capability includes IEEE exceptions trappable and 
*  otherwise. Test results, FPCR flags, and any DXC are saved for all 
*  tests.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*                         bfp-015-sqrt.asm 
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
         LTR   R14,R14       Return address provided?
         BNZR  R14           ..Yes, return to z/CMS test rig.  
         LPSWE WAITPSW        All done, load enabled wait PSW
*
*
         DS    0D            Ensure correct alignment for PSW
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
         ORG   BFPSQRTS+X'480'
         ORG   STRTLABL+X'300'  Enable run-time replacement
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
*
*  Locations for results
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
*
ENDLABL  EQU   STRTLABL+X'5B00'
         PADCSECT ENDLABL
*
         END