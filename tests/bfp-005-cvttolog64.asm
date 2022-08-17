  TITLE 'bfp-005-cvttolog64: Test IEEE Cvt To Logical (uint-64)'
***********************************************************************
*
*Testcase IEEE CONVERT TO LOGICAL 64
*  Test case capability includes ieee exceptions trappable and
*  otherwise.  Test results, FPCR flags, DXC, and condition codes are
*  saved for all tests.
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
*                     bfp-005-cvttolog64.asm
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
*   CONVERT TO LOGICAL (short BFP to uint-64, RRF-e)
*   CONVERT TO LOGICAL (long BFP to uint-64, RRF-e)
*   CONVERT TO LOGICAL (extended BFP to uint-64, RRF-e)
*
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R
* commands.
*
* Test Case Order
* 1) Short BFP to uint-64
* 2) Short BFP to uint-64 with all rounding modes
* 3) Long BFP uint-64
* 3) Long BFP uint-64 with all rounding modes
* 4) Extended BFP to uint-64
* 4) Extended BFP to uint-64 with all rounding modes
*
* Provided test data is:
*      1, 2, 4, -2, QNaN, SNaN, max uint-64 + 1
*   The last value will trigger inexact exceptions when converted
*   to uint-64.
* The same values are provided in each of the three input formats
*   except for the last input.  This is rounded up to the nearest
*   value that can be represented in the input format.  Extended
*   BFP is the only format with an exact representation.
*     Extended BFP: 403F0000000000000000000000000000 =>
*                               18 446 744 073 709 551 616 (exact)
*     Long BFP      43F0000000000001 =>
*                               18 446 744 073 709 555 712
*     Short BFP:    5F800001 => 18 446 746 272 732 807 168
* Provided test data for rounding tests:
*   -1.5, -0.5, +0.5, +1.5, +2.5, +5.5, +9.5, max uint-64
*   This data is taken from Table 9-11 on page 9-16 of SA22-7832-10.
*   While the table illustrates LOAD FP INTEGER, the same results
*   should be generated when creating a uint-32 or uint-64 integer
*   from a floating point value.  The last value, max uint-64,
*   is rounded down (truncated) to the input format.  Extended is
*   the only format with an exact representation.
*     Extended BFP: 403EFFFFFFFFFFFFFFFF000000000000 =>
*                           18 446 744 073 709 551 615.5 (exact)
*     Long BFP      43EFFFFFFFFFFFFF =>
*                           18 446 744 073 709 549 568
*     Short BFP:    5F7FFFFF => 18 446 742 974 197 923 840
*   These values are used so that rounding mode determines whether
*   the result fits in a uint-64.
*
* Also tests the following floating point support instructions
*   LOAD  (Short)
*   LOAD  (Long)
*   LOAD FPC
*   SET BFP ROUNDING MODE 2-bit
*   SET BFP ROUNDING MODE 3-bit
*   STORE (Short)
*   STORE (Long)
*   STORE FPC
*
***********************************************************************
         EJECT
*
*  Note: for compatibility with the z/CMS test rig, do not change
*  or use R11, R14, or R15.  Everything else is fair game.
*
BFPCVTTL START 0
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
         ORG   BFPCVTTL+X'8E'      Program check interrution code
PCINTCD  DS    H
*
PCOLDPSW EQU   BFPCVTTL+X'150'     z/Arch Program check old PSW
*
         ORG   BFPCVTTL+X'1A0'     z/Arch Restart PSW
         DC    X'0000000180000000',AD(START)
*
         ORG   BFPCVTTL+X'1D0'     z/Arch Program check NEW PSW
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
START    STCTL R0,R0,CTLR0    Store CR0 to enable AFP
         OI    CTLR0+1,X'04'  Turn on AFP bit
         LCTL  R0,R0,CTLR0    Reload updated CR0
*
* Short BFP Input testing
*
         LA    R10,SHORTS     Point to short BFP test inputs
         BAS   R13,CLGEBR     Convert values to uint-64 from short BFP
         LA    R10,RMSHORTS   Point to inputs for rounding mode tests
         BAS   R13,CLGEBRA    Convert using all rounding mode options
*
* Short BFP Input testing
*
         LA    R10,LONGS      Point to long BFP test inputs
         BAS   R13,CLGDBR     Convert values to uint-64 from long BFP
         LA    R10,RMLONGS    Point to inputs for rounding mode tests
         BAS   R13,CLGDBRA    Convert using all rounding mode options
*
* Short BFP Input testing
*
         LA    R10,EXTDS      Point to extended BFP test inputs
         BAS   R13,CLGXBR     Convert values to uint-64 from extended
         LA    R10,RMEXTDS    Point to inputs for rounding mode tests
         BAS   R13,CLGXBRA    Convert using all rounding mode options
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
* Input values parameter list, four fullwords:
*      1) Count,
*      2) Address of inputs,
*      3) Address to place results, and
*      4) Address to place DXC/Flags/cc values.
*
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
RMSHORTS DS    0F           Inputs for short BFP rounding testing
         DC    A(SBFPRMCT/4)
         DC    A(SBFPINRM)
         DC    A(SINTRMO)
         DC    A(SINTRMOF)
*
RMLONGS  DS    0F           Inputs for long BFP rounding testing
         DC    A(LBFPRMCT/8)
         DC    A(LBFPINRM)
         DC    A(LINTRMO)
         DC    A(LINTRMOF)
*
RMEXTDS  DS    0F           Inputs for extd BFP rounding testing
         DC    A(XBFPRMCT/16)
         DC    A(XBFPINRM)
         DC    A(XINTRMO)
         DC    A(XINTRMOF)
         EJECT
***********************************************************************
*
* Convert short BFP to uint-64 format.  A pair of results is generated
* for each input: one with all exceptions non-trappable, and the second
* with all exceptions trappable.   The FPCR and condition code is
* stored for each result.  Rounding mode RNTE, round to nearest, ties
* to even is used for each of these tests
*
***********************************************************************
          SPACE 2
CLGEBR   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR0,0(,R3)   Get short BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CLGEBR R1,0,FPR0,0  Cvt float in FPR0 to uint-64 in GPR1
         STG   R1,0(,R7)     Store uint-64 result
         STFPC 0*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(0*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGTR      Set exceptions trappable
         XGR   R1,R1         Clear any residual result in R1
         SPM   R1            Clear out any residual nz condition code
         CLGEBR R1,0,FPR0,0  Cvt float in FPR0 to uint-64 in GPR1
         STG   R1,8(,R7)     Store short BFP result
         STFPC 4(R8)         Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save CC as low byte of FPCR
*
         LA    R3,4(,R3)     Point to next input value
         LA    R7,2*8(,R7)   Point to next uint-64 converted value pair
         LA    R8,2*4(,R8)   Point to next FPCR/CC result pair
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
* The default rounding mode (0 for RNTE) is not tested in this
* section; prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.
*
***********************************************************************
         SPACE 2
CLGEBRA  LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR0,0(,R3)   Get short BFP test value
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  1             SET FPC to RZ, towards zero.
         CLGEBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,0*8(,R7)   Store uint-64 result
         STFPC 0*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(0*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPC to RP, to +infinity
         CLGEBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,1*8(,R7)   Store uint-64 result
         STFPC 1*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPC to RM, to -infinity
         CLGEBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,2*8(,R7)   Store uint-64 result
         STFPC 2*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(2*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RFS, Prepare for Shorter Precision
         CLGEBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,3*8(,R7)   Store uint-64 result
         STFPC 3*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(3*4)+3(,R8)  Save CC as low byte of FPCR
*
* Test cases using rounding mode specified in the instruction M3 field
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGEBR R1,1,FPR0,B'0000'  RNTA, to nearest, ties away
         STG   R1,4*8(,R7)   Store uint-64 result
         STFPC 4*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(4*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGEBR R1,3,FPR0,B'0000'  RFS, prepare for shorter precision
         STG   R1,5*8(,R7)   Store uint-64 result
         STFPC 5*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(5*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGEBR R1,4,FPR0,B'0000'  RNTE, to nearest, ties to even
         STG   R1,6*8(,R7)   Store uint-64 result
         STFPC 6*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(6*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGEBR R1,5,FPR0,B'0000'  RZ, toward zero
         STG   R1,7*8(,R7)   Store uint-64 result
         STFPC 7*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(7*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGEBR R1,6,FPR0,B'0000'  RP, to +inf
         STG   R1,8*8(,R7)   Store uint-64 result
         STFPC 8*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(8*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGEBR R1,7,FPR0,B'0000'  RM, to -inf
         STG   R1,9*8(,R7)   Store uint-64 result
         STFPC 9*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(9*4)+3(,R8)  Save CC as low byte of FPCR
*
         LA    R3,4(,R3)     Point to next input value
         LA    R7,10*8(,R7)  Point to next uint-64 result set
         LA    R8,12*4(,R8)  Point to next FPCR/CC result set
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert long BFP inputs to uint-64.  A pair of results is generated
* for each input: one with all exceptions non-trappable, and the second
* with all exceptions trappable.   The FPCR and condition code is
* stored for each result.
*
***********************************************************************
          SPACE 2
CLGDBR   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)   Get long BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CLGDBR R1,0,FPR0,0  Cvt float in FPR0 to uint-64 in GPR1
         STG   R1,0(,R7)     Store long BFP result
         STFPC 0*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(0*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGTR      Set exceptions trappable
         XGR   R1,R1         Clear any residual result in R1
         SPM   R1            Clear out any residual nz condition code
         CLGDBR R1,0,FPR0,0  Cvt float in FPR0 to uint-64 in GPR1
         STG   R1,8(,R7)     Store uint-64 result
         STFPC 1*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save CC as low byte of FPCR
*
         LA    R3,8(,R3)     Point to next input value
         LA    R7,16(,R7)    Point to next uint-64 result pair
         LA    R8,8(,R8)     Point to next FPCR/CC result pair
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
CLGDBRA  LM    R2,R3,0(R10)  Get count and address of test input values
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
         SRNM  1             SET FPC to RZ, towards zero.
         CLGDBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,0*8(,R7)   Store uint-64 result
         STFPC 0(R8)         Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,3(,R8)     Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPC to RP, to +infinity
         CLGDBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,1*8(,R7)   Store uint-64 result
         STFPC 1*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPC to RM, to -infinity
         CLGDBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,2*8(,R7)   Store uint-64 result
         STFPC 2*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(2*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RFS, Prepare for Shorter Precision
         CLGDBR R1,0,FPR0,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,3*8(,R7)   Store uint-64 result
         STFPC 3*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(3*4)+3(,R8)  Save CC as low byte of FPCR
*
* Test cases using rounding mode specified in the instruction M3 field
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGDBR R1,1,FPR0,B'0000'  RNTA, to nearest, ties away from 0
         STG   R1,4*8(,R7)   Store uint-64 result
         STFPC 4*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(4*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGDBR R1,3,FPR0,B'0000'  RFS, prepare for shorter precision
         STG   R1,5*8(,R7)   Store uint-64 result
         STFPC 5*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(5*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGDBR R1,4,FPR0,B'0000'  RNTE, to nearest, ties to even
         STG   R1,6*8(,R7)   Store uint-64 result
         STFPC 6*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(6*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGDBR R1,5,FPR0,B'0000'  RZ, toward zero
         STG   R1,7*8(,R7)   Store uint-64 result
         STFPC 7*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(7*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGDBR R1,6,FPR0,B'0000'  RP, to +inf
         STG   R1,8*8(,R7)   Store uint-64 result
         STFPC 8*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(8*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGDBR R1,7,FPR0,B'0000'  RM, to -inf
         STG   R1,9*8(,R7)   Store uint-64 result
         STFPC 9*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(9*4)+3(,R8)  Save CC as low byte of FPCR
*
         LA    R3,8(,R3)     Point to next input value
         LA    R7,10*8(,R7)  Point to next uint-64 result set
         LA    R8,12*4(,R8)  Point to next FPCR/CC result set
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert extended BFP to uint-64.  A pair of results is generated
* for each input: one with all exceptions non-trappable, and the
* second with all exceptions trappable.   The FPCR and condition code
* are stored for each result.
*
***********************************************************************
          SPACE 2
CLGXBR   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)    Get extended BFP test value part 1
         LD    R2,8(,R3)     Get extended BFP test value part 1
         LFPC  FPCREGNT      Set exceptions non-trappable
         CLGXBR R1,0,FPR0,0  Cvt float in FPR0-FPR2 to uint-64 in GPR1
         STG   R1,0(,R7)     Store uint-64 result
         STFPC (0*4)(R8)     Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(0*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGTR      Set exceptions trappable
         XGR   R1,R1         Clear any residual result in R1
         SPM   R1            Clear out any residual nz condition code
         CLGXBR R1,0,FPR0,0  Cvt float in FPR0-FPR2 to uint-64 in GPR1
         STG   R1,8(,R7)     Store uint-64 result
         STFPC (1*4)(R8)     Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save CC as low byte of FPCR
*
         LA    R3,16(,R3)    Point to next extended BFP input value
         LA    R7,16(,R7)    Point to next uint-64 result pair
         LA    R8,8(,R8)     Point to next FPCR/CC result pair
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
CLGXBRA  LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    R0,0(,R3)    Get extended BFP test value part 1
         LD    R2,8(,R3)    Get extended BFP test value part 2
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  1             SET FPC to RZ, towards zero.
         CLGXBR R1,0,R0,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,0*8(,R7)   Store uint-64 result
         STFPC 0(R8)         Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,3(,R8)     Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPC to RP, to +infinity
         CLGXBR R1,0,R0,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,1*8(,R7)   Store uint-64 result
         STFPC 1*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(1*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPC to RM, to -infinity
         CLGXBR R1,0,R0,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,2*8(,R7)   Store uint-64 result
         STFPC 2*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(2*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RFS, Prepare for Shorter Precision
         CLGXBR R1,0,R0,B'0100'  FPCR ctl'd rounding, inexact masked
         STG   R1,3*8(,R7)   Store uint-64 result
         STFPC 3*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(3*4)+3(,R8)  Save CC as low byte of FPCR
*
* Test cases using rounding mode specified in the instruction M3 field
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGXBR R1,1,R0,B'0000'  RNTA, to nearest, ties away from zero
         STG   R1,4*8(,R7)   Store uint-64 result
         STFPC 4*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(4*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGXBR R1,3,R0,B'0000'  RFS, to prepare for shorter precision
         STG   R1,5*8(,R7)   Store uint-64 result
         STFPC 5*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(5*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGXBR R1,4,R0,B'0000'  RNTE, to nearest, ties to even
         STG   R1,6*8(,R7)   Store uint-64 result
         STFPC 6*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(6*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGXBR R1,5,R0,B'0000'  RZ, toward zero
         STG   R1,7*8(,R7)   Store uint-64 result
         STFPC 7*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(7*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGXBR R1,6,R0,B'0000'  RP, to +inf
         STG   R1,8*8(,R7)   Store uint-64 result
         STFPC 8*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(8*4)+3(,R8)  Save CC as low byte of FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CLGXBR R1,7,R0,B'0000'  RM, to -inf
         STG   R1,9*8(,R7)   Store uint-64 result
         STFPC 9*4(R8)       Store resulting FPC flags and DXC
         IPM   R1            Get condition code and program mask
         SRL   R1,28         Isolate CC in low order byte
         STC   R1,(9*4)+3(,R8)  Save CC as low byte of FPCR
*
         LA    R3,16(,R3)    Point to next input value
         LA    R7,10*8(,R7)  Point to next uint-64 result set
         LA    R8,12*4(,R8)  Point to next FPCR/CC result pair
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* BFP inputs for Convert To Logical testing.  The same set of values
* are used for short, long, and extended formats, with the exception
* of the last value, which is rounded to fit the input format and
* for the needs of the test (conversion or rounding).
*
***********************************************************************
          SPACE 2
*
* Short integer inputs for Convert From Fixed testing.  The same set of
* inputs are used for short, long, and extended formats.  The last two
* values are used for rounding mode tests for short only; conversion of
* uint-64 to long or extended are always exact.
*
SBFPIN   DS    0F           Inputs for short BFP testing
         DC    X'3F800000'    +1.0
         DC    X'40000000'    +2.0
         DC    X'40800000'    +4.0
         DC    X'7F810000'    SNaN
         DC    X'7FC10000'    QNaN
         DC    X'5F800001'    max uint-64 + 1 rounded up to short BFP
*                                    18 446 746 272 732 807 168
*                             Note: above value rounds to max uint-64.
         DC    X'5F7FFFFF'    max uint-64 rounded down to short BFP
*                                    18 446 742 974 197 923 840
         DC    X'3F400000'    +0.75
         DC    X'3E800000'    +0.25
SBFPCT   EQU   *-SBFPIN     Count of short BFP in list * 4
*
*
SBFPINRM DS    0F           Inputs for short BFP rounding testing
*
* The following values correspond to Figure 9-11 on page 9-16 of the
* z/Arch POP, SA22-7832-10
*
         DC    X'BFC00000'    -1.5
         DC    X'BF000000'    -0.5
         DC    X'3F000000'    +0.5
         DC    X'3FC00000'    +1.5
         DC    X'40200000'    +2.5
         DC    X'40B00000'    +5.5
         DC    X'41180000'    +9.5
*
* The following values ensure correct rounding for values that
* are not ties.
*
         DC    X'5F7FFFFF'    max uint-64 rounded down to short BFP
*                                    18 446 742 974 197 923 840
         DC    X'3F400000'    +0.75
         DC    X'3E800000'    +0.25
SBFPRMCT EQU   *-SBFPINRM   Count of rounding mode test short BFP * 4
*
*
LBFPIN   DS    0F            Inputs for long BFP testing
         DC    X'3FF0000000000000'    +1.0
         DC    X'4000000000000000'    +2.0
         DC    X'4010000000000000'    +4.0
         DC    X'7FF0100000000000'    SNaN
         DC    X'7FF8100000000000'    QNaN
         DC    X'43F0000000000000'   max uint-64 + 1 rounded up
*                                       18 446 744 073 709 555 712
         DC    X'43EFFFFFFFFFFFFF'   max uint-64 rounded down
*                                       18 446 744 073 709 549 568
         DC    X'3FE8000000000000'    +0.75
         DC    X'3FD0000000000000'    +0.25
LBFPCT   EQU   *-LBFPIN     Count of long BFP in list * 8
*
*
LBFPINRM DS    0F            Inputs for long BFP rounding testing
*
* The following values correspond to Figure 9-11 on page 9-16 of the
* z/Arch POP, SA22-7832-10
*
         DC    X'BFF8000000000000'    -1.5
         DC    X'BFE0000000000000'    -0.5
         DC    X'3FE0000000000000'    +0.5
         DC    X'3FF8000000000000'    +1.5
         DC    X'4004000000000000'    +2.5
         DC    X'4016000000000000'    +5.5
         DC    X'4023000000000000'    +9.5
*
* The following values ensure correct rounding for values that
* are not ties.
*
         DC    X'43EFFFFFFFFFFFFF'   max uint-64 rounded down
*                                   18 446 744 073 709 549 568
*        DC    X'3FE8000000000000'    0.75
         DC    X'3FD0000000000000'    +0.25
LBFPRMCT EQU   *-LBFPINRM   Count of roundinf test long BFP * 8
*
*
XBFPIN   DS    0D           Inputs for extended BFP testing
         DC    X'3FFF0000000000000000000000000000'    +1.0
         DC    X'40000000000000000000000000000000'    +2.0
         DC    X'40010000000000000000000000000000'    +4.0
         DC    X'7FFF0100000000000000000000000000'    SNaN
         DC    X'7FFF8100000000000000000000000000'    QNaN
         DC    X'403F0000000000000000000000000000'   max uint-64 + 1
*                                  18 446 744 073 709 551 616 (exact)
         DC    X'403EFFFFFFFFFFFFFFFE000000000000'   max uint-64
*                                  18 446 744 073 709 551 615 (exact)
         DC    X'403EFFFFFFFFFFFFFFFF000000000000'   max uint-64+0.5
*                                  18 446 744 073 709 551 615.5
*    Above is always inexact, and may overflow based on rounding mode
         DC    X'3FFE8000000000000000000000000000'         0.75
         DC    X'3FFD0000000000000000000000000000'         0.25
XBFPCT   EQU   *-XBFPIN     Count of extended BFP in list * 16
*
*
XBFPINRM DS    0D           Inputs for extended BFP rounding testing
*
* The following values correspond to Figure 9-11 on page 9-16 of the
* z/Arch POP, SA22-7832-10
*
         DC    X'BFFF8000000000000000000000000000'         -1.5
         DC    X'BFFE0000000000000000000000000000'         -0.5
         DC    X'3FFE0000000000000000000000000000'         +0.5
         DC    X'3FFF8000000000000000000000000000'         +1.5
         DC    X'40004000000000000000000000000000'         +2.5
         DC    X'40016000000000000000000000000000'         +5.5
         DC    X'40023000000000000000000000000000'         +9.5
*
* The following values ensure correct rounding for values that
* are not ties.
*
         DC    X'403EFFFFFFFFFFFFFFFF000000000000'   max uint-64+0.5
*                                  18 446 744 073 709 551 615.5
*    Above is always inexact, and may overflow based on rounding mode
         DC    X'3FFE8000000000000000000000000000'         0.75
         DC    X'3FFD0000000000000000000000000000'         0.25
XBFPRMCT EQU   *-XBFPINRM   Count of rounding test extd BFP * 16
         EJECT
***********************************************************************
*                 ACTUAL results saved here
***********************************************************************
*
*               Locations for ACTUAL results
*
*
SINTOUT  EQU   BFPCVTTL+X'1000'    Uint-64 values from short BFP
*                                  ..6 pairs used, room for 32
SINTFLGS EQU   BFPCVTTL+X'1200'    FPC flags and DXC from short BFP
*                                  ..6 pairs used, room for 32
SINTRMO  EQU   BFPCVTTL+X'1300'    Short rounding mode test results
*                                  ..8 sets used, room for 16
SINTRMOF EQU   BFPCVTTL+X'1800'    Short rounding mode FPCR contents
*                                  ..8 sets used, room for 16
*
LINTOUT  EQU   BFPCVTTL+X'2000'    Uint-64 values from long BFP
*                                  ..6 pairs used, room for 32
LINTFLGS EQU   BFPCVTTL+X'2200'    FPC flags and DXC from long BFP
*                                  ..6 pairs used, room for 32
LINTRMO  EQU   BFPCVTTL+X'2300'    Long rounding mode test results
*                                  ..8 sets used, room for 16
LINTRMOF EQU   BFPCVTTL+X'2800'    Long rounding mode FPCR contents
*                                  ..8 sets used, room for 16
*
XINTOUT  EQU   BFPCVTTL+X'3000'    Uint-64 values from extd BFP
*                                  ..6 pairs used, room for 32
XINTFLGS EQU   BFPCVTTL+X'3200'    FPC flags and DXC from extd BFP
*                                  ..6 pairs used, room for 32
XINTRMO  EQU   BFPCVTTL+X'3300'    Extended rounding mode test results
*                                  ..8 sets used, room for 16
XINTRMOF EQU   BFPCVTTL+X'3800'    Long rounding mode FPCR contents
*                                  ..8 sets used, room for 16
         EJECT
***********************************************************************
*                    EXPECTED results
***********************************************************************
*
         ORG   BFPCVTTL+X'5000'   (past end of actual results)
*
* I am not satisfied with test case 6. Further investigation needed. It
* would appear that f32_to_uint64 is returning invalid and max uint-64.
*
SINTOUT_GOOD EQU *
 DC CL48'CLGEBR result pair 1'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGEBR result pair 2'
 DC XL16'00000000000000020000000000000002'
 DC CL48'CLGEBR result pair 3'
 DC XL16'00000000000000040000000000000004'
 DC CL48'CLGEBR result pair 4'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR result pair 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR result pair 6'
 DC XL16'FFFFFFFFFFFFFFFF0000000000000000'
 DC CL48'CLGEBR result pair 7'
 DC XL16'FFFFFF0000000000FFFFFF0000000000'
 DC CL48'CLGEBR result pair 8'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGEBR result pair 9'
 DC XL16'00000000000000000000000000000000'
SINTOUT_NUM EQU (*-SINTOUT_GOOD)/64
*
*
SINTFLGS_GOOD EQU *
 DC CL48'CLGEBR FPCR pairs 1-2'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CLGEBR FPCR pairs 3-4'
 DC XL16'00000002F800000200880003F8008000'
 DC CL48'CLGEBR FPCR pairs 5-6'
 DC XL16'00880003F800800000880003F8008000'
 DC CL48'CLGEBR FPCR pairs 7-8'
 DC XL16'00000002F800000200080002F8000C02'
 DC CL48'CLGEBR FPCR pair 9'
 DC XL16'00080002F80008020000000000000000'
SINTFLGS_NUM EQU (*-SINTFLGS_GOOD)/64
*
*
SINTRMO_GOOD EQU *
 DC CL48'CLGEBR -1.5 FPC modes 1, 2'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR -1.5 FPC modes 3, 7'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR -1.5 M3 modes 1, 3'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR -1.5 M3 modes 4, 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR -1.5 M3 modes 6, 7'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR -0.5 FPC modes 1, 2'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR -0.5 FPC modes 3, 7'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR -0.5 M3 modes 1, 3'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR -0.5 M3 modes 4, 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR -0.5 M3 modes 6, 7'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR +0.5 FPC modes 1, 2'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGEBR +0.5 FPC modes 3, 7'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGEBR +0.5 M3 modes 1, 3'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGEBR +0.5 M3 modes 4, 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR +0.5 M3 modes 6, 7'
 DC XL16'00000000000000010000000000000000'
 DC CL48'CLGEBR +1.5 FPC modes 1, 2'
 DC XL16'00000000000000010000000000000002'
 DC CL48'CLGEBR +1.5 FPC modes 3, 7'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGEBR +1.5 M3 modes 1, 3'
 DC XL16'00000000000000020000000000000001'
 DC CL48'CLGEBR +1.5 M3 modes 4, 5'
 DC XL16'00000000000000020000000000000001'
 DC CL48'CLGEBR +1.5 M3 modes 6, 7'
 DC XL16'00000000000000020000000000000001'
 DC CL48'CLGEBR +2.5 FPC modes 1, 2'
 DC XL16'00000000000000020000000000000003'
 DC CL48'CLGEBR +2.5 FPC modes 3, 7'
 DC XL16'00000000000000020000000000000003'
 DC CL48'CLGEBR +2.5 M3 modes 1, 3'
 DC XL16'00000000000000030000000000000003'
 DC CL48'CLGEBR +2.5 M3 modes 4, 5'
 DC XL16'00000000000000020000000000000002'
 DC CL48'CLGEBR +2.5 M3 modes 6, 7'
 DC XL16'00000000000000030000000000000002'
 DC CL48'CLGEBR +5.5 FPC modes 1, 2'
 DC XL16'00000000000000050000000000000006'
 DC CL48'CLGEBR +5.5 FPC modes 3, 7'
 DC XL16'00000000000000050000000000000005'
 DC CL48'CLGEBR +5.5 M3 modes 1, 3'
 DC XL16'00000000000000060000000000000005'
 DC CL48'CLGEBR +5.5 M3 modes 4, 5'
 DC XL16'00000000000000060000000000000005'
 DC CL48'CLGEBR +5.5 M3 modes 6, 7'
 DC XL16'00000000000000060000000000000005'
 DC CL48'CLGEBR +9.5 FPC modes 1, 2'
 DC XL16'0000000000000009000000000000000A'
 DC CL48'CLGEBR +9.5 FPC modes 3, 7'
 DC XL16'00000000000000090000000000000009'
 DC CL48'CLGEBR +9.5 M3 modes 1, 3'
 DC XL16'000000000000000A0000000000000009'
 DC CL48'CLGEBR +9.5 M3 modes 4, 5'
 DC XL16'000000000000000A0000000000000009'
 DC CL48'CLGEBR +9.5 M3 modes 6, 7'
 DC XL16'000000000000000A0000000000000009'
 DC CL48'CLGEBR max FPC modes 1, 2'
 DC XL16'FFFFFF0000000000FFFFFF0000000000'
 DC CL48'CLGEBR max FPC modes 3, 7'
 DC XL16'FFFFFF0000000000FFFFFF0000000000'
 DC CL48'CLGEBR max M3 modes 1, 3'
 DC XL16'FFFFFF0000000000FFFFFF0000000000'
 DC CL48'CLGEBR max M3 modes 4, 5'
 DC XL16'FFFFFF0000000000FFFFFF0000000000'
 DC CL48'CLGEBR max M3 modes 6, 7'
 DC XL16'FFFFFF0000000000FFFFFF0000000000'
 DC CL48'CLGEBR +0.75 FPC modes 1, 2'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGEBR +0.75 FPC modes 3, 7'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGEBR +0.75 M3 modes 1, 3'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGEBR +0.75 M3 modes 4, 5'
 DC XL16'00000000000000010000000000000000'
 DC CL48'CLGEBR +0.75 M3 modes 6, 7'
 DC XL16'00000000000000010000000000000000'
 DC CL48'CLGEBR +0.25 FPC modes 1, 2'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGEBR +0.25 FPC modes 3, 7'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGEBR +0.25 M3 modes 1, 3'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGEBR +0.25 M3 modes 4, 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGEBR +0.25 M3 modes 6, 7'
 DC XL16'00000000000000010000000000000000'
SINTRMO_NUM EQU (*-SINTRMO_GOOD)/64
*
*
SINTRMOF_GOOD EQU *
 DC CL48'CLGEBR -1.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00800003008000030080000300800003'
 DC CL48'CLGEBR -1.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00880003008800030088000300880003'
 DC CL48'CLGEBR -1.5 M3 modes 6, 7 FPCR'
 DC XL16'00880003008800030000000000000000'
 DC CL48'CLGEBR -0.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000001000000010080000300800003'
 DC CL48'CLGEBR -0.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00880003008800030008000100080001'
 DC CL48'CLGEBR -0.5 M3 modes 6, 7 FPCR'
 DC XL16'00080001008800030000000000000000'
 DC CL48'CLGEBR +0.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGEBR +0.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGEBR +0.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGEBR +1.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGEBR +1.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGEBR +1.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGEBR +2.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGEBR +2.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGEBR +2.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGEBR +5.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGEBR +5.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGEBR +5.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGEBR +9.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGEBR +9.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGEBR +9.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGEBR max+1 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGEBR max+1 M3 modes 1, 3-5 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGEBR max+1 M3 modes 6, 7 FPCR'
 DC XL16'00000002000000020000000000000000'
 DC CL48'CLGEBR +0.75 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGEBR +0.75 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGEBR +0.75 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGEBR +0.25 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGEBR +0.25 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGEBR +0.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
SINTRMOF_NUM EQU (*-SINTRMOF_GOOD)/64
*
*
LINTOUT_GOOD EQU *
 DC CL48'CLGDBR result pair 1'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGDBR result pair 2'
 DC XL16'00000000000000020000000000000002'
 DC CL48'CLGDBR result pair 3'
 DC XL16'00000000000000040000000000000004'
 DC CL48'CLGDBR result pair 4'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGDBR result pair 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGDBR result pair 6'
 DC XL16'FFFFFFFFFFFFFFFF0000000000000000'
 DC CL48'CLGDBR result pair 7'
 DC XL16'FFFFFFFFFFFFF800FFFFFFFFFFFFF800'
 DC CL48'CLGDBR result pair 8'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGDBR result pair 9'
 DC XL16'00000000000000000000000000000000'
LINTOUT_NUM EQU (*-LINTOUT_GOOD)/64
*
*
LINTFLGS_GOOD EQU *
 DC CL48'CLGDBR FPCR pairs 1-2'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CLGDBR FPCR pairs 3-4'
 DC XL16'00000002F800000200880003F8008000'
 DC CL48'CLGDBR FPCR pairs 5-6'
 DC XL16'00880003F800800000880003F8008000'
 DC CL48'CLGDBR FPCR pairs 7-8'
 DC XL16'00000002F800000200080002F8000C02'
 DC CL48'CLGDBR FPCR pair 9'
 DC XL16'00080002F80008020000000000000000'
LINTFLGS_NUM EQU (*-LINTFLGS_GOOD)/64
*
*
LINTRMO_GOOD EQU *
 DC CL48'CLGDBR -1.5 FPC modes 1, 2'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGDBR -1.5 FPC modes 3, 7'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGDBR -1.5 M3 modes 1, 3'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGDBR -1.5 M3 modes 4, 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGDBR -1.5 M3 modes 6, 7'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGDBR -0.5 FPC modes 1, 2'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGDBR -0.5 FPC modes 3, 7'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGDBR -0.5 M3 modes 1, 3'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGDBR -0.5 M3 modes 4, 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGDBR -0.5 M3 modes 6, 7'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGDBR +0.5 FPC modes 1, 2'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGDBR +0.5 FPC modes 3, 7'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGDBR +0.5 M3 modes 1, 3'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGDBR +0.5 M3 modes 4, 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGDBR +0.5 M3 modes 6, 7'
 DC XL16'00000000000000010000000000000000'
 DC CL48'CLGDBR +1.5 FPC modes 1, 2'
 DC XL16'00000000000000010000000000000002'
 DC CL48'CLGDBR +1.5 FPC modes 3, 7'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGDBR +1.5 M3 modes 1, 3'
 DC XL16'00000000000000020000000000000001'
 DC CL48'CLGDBR +1.5 M3 modes 4, 5'
 DC XL16'00000000000000020000000000000001'
 DC CL48'CLGDBR +1.5 M3 modes 6, 7'
 DC XL16'00000000000000020000000000000001'
 DC CL48'CLGDBR +2.5 FPC modes 1, 2'
 DC XL16'00000000000000020000000000000003'
 DC CL48'CLGDBR +2.5 FPC modes 3, 7'
 DC XL16'00000000000000020000000000000003'
 DC CL48'CLGDBR +2.5 M3 modes 1, 3'
 DC XL16'00000000000000030000000000000003'
 DC CL48'CLGDBR +2.5 M3 modes 4, 5'
 DC XL16'00000000000000020000000000000002'
 DC CL48'CLGDBR +2.5 M3 modes 6, 7'
 DC XL16'00000000000000030000000000000002'
 DC CL48'CLGDBR +5.5 FPC modes 1, 2'
 DC XL16'00000000000000050000000000000006'
 DC CL48'CLGDBR +5.5 FPC modes 3, 7'
 DC XL16'00000000000000050000000000000005'
 DC CL48'CLGDBR +5.5 M3 modes 1, 3'
 DC XL16'00000000000000060000000000000005'
 DC CL48'CLGDBR +5.5 M3 modes 4, 5'
 DC XL16'00000000000000060000000000000005'
 DC CL48'CLGDBR +5.5 M3 modes 6, 7'
 DC XL16'00000000000000060000000000000005'
 DC CL48'CLGDBR +9.5 FPC modes 1, 2'
 DC XL16'0000000000000009000000000000000A'
 DC CL48'CLGDBR +9.5 FPC modes 3, 7'
 DC XL16'00000000000000090000000000000009'
 DC CL48'CLGDBR +9.5 M3 modes 1, 3'
 DC XL16'000000000000000A0000000000000009'
 DC CL48'CLGDBR +9.5 M3 modes 4, 5'
 DC XL16'000000000000000A0000000000000009'
 DC CL48'CLGDBR +9.5 M3 modes 6, 7'
 DC XL16'000000000000000A0000000000000009'
 DC CL48'CLGDBR max FPC modes 1, 2'
 DC XL16'FFFFFFFFFFFFF800FFFFFFFFFFFFF800'
 DC CL48'CLGDBR max FPC modes 3, 7'
 DC XL16'FFFFFFFFFFFFF800FFFFFFFFFFFFF800'
 DC CL48'CLGDBR max M3 modes 1, 3'
 DC XL16'FFFFFFFFFFFFF800FFFFFFFFFFFFF800'
 DC CL48'CLGDBR max M3 modes 4, 5'
 DC XL16'FFFFFFFFFFFFF800FFFFFFFFFFFFF800'
 DC CL48'CLGDBR max M3 modes 6, 7'
 DC XL16'FFFFFFFFFFFFF800FFFFFFFFFFFFF800'
LINTRMO_NUM EQU (*-LINTRMO_GOOD)/64
*
*
LINTRMOF_GOOD EQU *
 DC CL48'CLGDBR -1.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00800003008000030080000300800003'
 DC CL48'CLGDBR -1.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00880003008800030088000300880003'
 DC CL48'CLGDBR -1.5 M3 modes 6, 7 FPCR'
 DC XL16'00880003008800030000000000000000'
 DC CL48'CLGDBR -0.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000001000000010080000300800003'
 DC CL48'CLGDBR -0.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00880003008800030008000100080001'
 DC CL48'CLGDBR -0.5 M3 modes 6, 7 FPCR'
 DC XL16'00080001008800030000000000000000'
 DC CL48'CLGDBR +0.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGDBR +0.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGDBR +0.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGDBR +1.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGDBR +1.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGDBR +1.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGDBR +2.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGDBR +2.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGDBR +2.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGDBR +5.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGDBR +5.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGDBR +5.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGDBR +9.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGDBR +9.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGDBR +9.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGDBR max FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGDBR max M3 modes 1, 3-5 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGDBR max M3 modes 6, 7 FPCR'
 DC XL16'00000002000000020000000000000000'
LINTRMOF_NUM EQU (*-LINTRMOF_GOOD)/64
*
*
XINTOUT_GOOD EQU *
 DC CL48'CLGXBR result pair 1'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGXBR result pair 2'
 DC XL16'00000000000000020000000000000002'
 DC CL48'CLGXBR result pair 3'
 DC XL16'00000000000000040000000000000004'
 DC CL48'CLGXBR result pair 4'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR result pair 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR result pair 6'
 DC XL16'FFFFFFFFFFFFFFFF0000000000000000'
 DC CL48'CLGXBR result pair 7'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL48'CLGXBR result pair 8'
 DC XL16'FFFFFFFFFFFFFFFF0000000000000000'
 DC CL48'CLGXBR result pair 9'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGXBR result pair 10'
 DC XL16'00000000000000000000000000000000'
XINTOUT_NUM EQU (*-XINTOUT_GOOD)/64
*
*
XINTFLGS_GOOD EQU *
 DC CL48'CLGXBR FPC pairs 1-2'
 DC XL16'00000002F800000200000002F8000002'
 DC CL48'CLGXBR FPC pairs 3-4'
 DC XL16'00000002F800000200880003F8008000'
 DC CL48'CLGXBR FPC pairs 5-6'
 DC XL16'00880003F800800000880003F8008000'
 DC CL48'CLGXBR FPC pairs 7-8'
 DC XL16'00000002F800000200880003F8008000'
 DC CL48'CLGXBR FPC pairs 9-10'
 DC XL16'00080002F8000C0200080002F8000802'
XINTFLGS_NUM EQU (*-XINTFLGS_GOOD)/64
*
*
XINTRMO_GOOD EQU *
 DC CL48'CLGXBR -1.5 FPC modes 1, 2'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR -1.5 FPC modes 3, 7'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR -1.5 M3 modes 1, 3'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR -1.5 M3 modes 4, 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR -1.5 M3 modes 6, 7'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR -0.5 FPC modes 1, 2'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR -0.5 FPC modes 3, 7'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR -0.5 M3 modes 1, 3'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR -0.5 M3 modes 4, 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR -0.5 M3 modes 6, 7'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR +0.5 FPC modes 1, 2'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGXBR +0.5 FPC modes 3, 7'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGXBR +0.5 M3 modes 1, 3'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGXBR +0.5 M3 modes 4, 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR +0.5 M3 modes 6, 7'
 DC XL16'00000000000000010000000000000000'
 DC CL48'CLGXBR +1.5 FPC modes 1, 2'
 DC XL16'00000000000000010000000000000002'
 DC CL48'CLGXBR +1.5 FPC modes 3, 7'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGXBR +1.5 M3 modes 1, 3'
 DC XL16'00000000000000020000000000000001'
 DC CL48'CLGXBR +1.5 M3 modes 4, 5'
 DC XL16'00000000000000020000000000000001'
 DC CL48'CLGXBR +1.5 M3 modes 6, 7'
 DC XL16'00000000000000020000000000000001'
 DC CL48'CLGXBR +2.5 FPC modes 1, 2'
 DC XL16'00000000000000020000000000000003'
 DC CL48'CLGXBR +2.5 FPC modes 3, 7'
 DC XL16'00000000000000020000000000000003'
 DC CL48'CLGXBR +2.5 M3 modes 1, 3'
 DC XL16'00000000000000030000000000000003'
 DC CL48'CLGXBR +2.5 M3 modes 4, 5'
 DC XL16'00000000000000020000000000000002'
 DC CL48'CLGXBR +2.5 M3 modes 6, 7'
 DC XL16'00000000000000030000000000000002'
 DC CL48'CLGXBR +5.5 FPC modes 1, 2'
 DC XL16'00000000000000050000000000000006'
 DC CL48'CLGXBR +5.5 FPC modes 3, 7'
 DC XL16'00000000000000050000000000000005'
 DC CL48'CLGXBR +5.5 M3 modes 1, 3'
 DC XL16'00000000000000060000000000000005'
 DC CL48'CLGXBR +5.5 M3 modes 4, 5'
 DC XL16'00000000000000060000000000000005'
 DC CL48'CLGXBR +5.5 M3 modes 6, 7'
 DC XL16'00000000000000060000000000000005'
 DC CL48'CLGXBR +9.5 FPC modes 1, 2'
 DC XL16'0000000000000009000000000000000A'
 DC CL48'CLGXBR +9.5 FPC modes 3, 7'
 DC XL16'00000000000000090000000000000009'
 DC CL48'CLGXBR +9.5 M3 modes 1, 3'
 DC XL16'000000000000000A0000000000000009'
 DC CL48'CLGXBR +9.5 M3 modes 4, 5'
 DC XL16'000000000000000A0000000000000009'
 DC CL48'CLGXBR +9.5 M3 modes 6, 7'
 DC XL16'000000000000000A0000000000000009'
 DC CL48'CLGXBR max+0.5 FPC modes 1, 2'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL48'CLGXBR max+0.5 FPC modes 3, 7'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL48'CLGXBR max+0.5 M3 modes 1, 3'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL48'CLGXBR max+0.5 M3 modes 4, 5'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL48'CLGXBR max+0.5 M3 modes 6, 7'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL48'CLGXBR +0.75 FPC modes 1, 2'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGXBR +0.75 FPC modes 3, 7'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGXBR +0.75 M3 modes 1, 3'
 DC XL16'00000000000000010000000000000001'
 DC CL48'CLGXBR +0.75 M3 modes 4, 5'
 DC XL16'00000000000000010000000000000000'
 DC CL48'CLGXBR +0.75 M3 modes 6, 7'
 DC XL16'00000000000000010000000000000000'
 DC CL48'CLGXBR +0.25 FPC modes 1, 2'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGXBR +0.25 FPC modes 3, 7'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGXBR +0.25 M3 modes 1, 3'
 DC XL16'00000000000000000000000000000001'
 DC CL48'CLGXBR +0.25 M3 modes 4, 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'CLGXBR +0.25 M3 modes 6, 7'
 DC XL16'00000000000000010000000000000000'
XINTRMO_NUM EQU (*-XINTRMO_GOOD)/64
*
*
XINTRMOF_GOOD EQU *
 DC CL48'CLGXBR -1.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00800003008000030080000300800003'
 DC CL48'CLGXBR -1.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00880003008800030088000300880003'
 DC CL48'CLGXBR -1.5 M3 modes 6, 7 FPCR'
 DC XL16'00880003008800030000000000000000'
 DC CL48'CLGXBR -0.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000001000000010080000300800003'
 DC CL48'CLGXBR -0.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00880003008800030008000100080001'
 DC CL48'CLGXBR -0.5 M3 modes 6, 7 FPCR'
 DC XL16'00080001008800030000000000000000'
 DC CL48'CLGXBR +0.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGXBR +0.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGXBR +0.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGXBR +1.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGXBR +1.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGXBR +1.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGXBR +2.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGXBR +2.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGXBR +2.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGXBR +5.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGXBR +5.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGXBR +5.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGXBR +9.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGXBR +9.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGXBR +9.5 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGXBR max+0.5 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002008000030000000200000002'
 DC CL48'CLGXBR max+0.5 M3 modes 1, 3-5 FPCR'
 DC XL16'00880003000800020088000300080002'
 DC CL48'CLGXBR max+0.5 M3 modes 6, 7 FPCR'
 DC XL16'00880003000800020000000000000000'
 DC CL48'CLGXBR 0.75 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGXBR 0.75 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGXBR 0.75 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
 DC CL48'CLGXBR 0.25 FPC modes 1-3, 7 FPCR'
 DC XL16'00000002000000020000000200000002'
 DC CL48'CLGXBR 0.25 M3 modes 1, 3-5 FPCR'
 DC XL16'00080002000800020008000200080002'
 DC CL48'CLGXBR 0.25 M3 modes 6, 7 FPCR'
 DC XL16'00080002000800020000000000000000'
XINTRMOF_NUM EQU (*-XINTRMOF_GOOD)/64
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
         DC    A(SINTOUT)
         DC    A(SINTOUT_GOOD)
         DC    A(SINTOUT_NUM)
*
         DC    A(SINTFLGS)
         DC    A(SINTFLGS_GOOD)
         DC    A(SINTFLGS_NUM)
*
         DC    A(SINTRMO)
         DC    A(SINTRMO_GOOD)
         DC    A(SINTRMO_NUM)
*
         DC    A(SINTRMOF)
         DC    A(SINTRMOF_GOOD)
         DC    A(SINTRMOF_NUM)
*
         DC    A(LINTOUT)
         DC    A(LINTOUT_GOOD)
         DC    A(LINTOUT_NUM)
*
         DC    A(LINTFLGS)
         DC    A(LINTFLGS_GOOD)
         DC    A(LINTFLGS_NUM)
*
         DC    A(LINTRMO)
         DC    A(LINTRMO_GOOD)
         DC    A(LINTRMO_NUM)
*
         DC    A(LINTRMOF)
         DC    A(LINTRMOF_GOOD)
         DC    A(LINTRMOF_NUM)
*
         DC    A(XINTOUT)
         DC    A(XINTOUT_GOOD)
         DC    A(XINTOUT_NUM)
*
         DC    A(XINTFLGS)
         DC    A(XINTFLGS_GOOD)
         DC    A(XINTFLGS_NUM)
*
         DC    A(XINTRMO)
         DC    A(XINTRMO_GOOD)
         DC    A(XINTRMO_NUM)
*
         DC    A(XINTRMOF)
         DC    A(XINTRMOF_GOOD)
         DC    A(XINTRMOF_NUM)
*
VERIFLEN EQU   (*-VERIFTAB)/12    #of entries in verify table
                                                                EJECT
         END
