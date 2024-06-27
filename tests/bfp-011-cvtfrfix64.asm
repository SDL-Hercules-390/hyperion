   TITLE 'bfp-011-cvtfrfix64: Test IEEE Cvt From Fixed (int-64)'
***********************************************************************
*
*Testcase IEEE CONVERT FROM FIXED 64
*  Test case capability includes ieee exceptions trappable and
*  otherwise.  Test result, FPCR flags, and DXC saved for all tests.
*  Convert From Fixed does not set the condition code.
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
*                     bfp-011-cvtfrfix64.asm
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
* Tests the following six conversion instructions
*   CONVERT FROM FIXED (64 to short BFP, RRE)
*   CONVERT FROM FIXED (64 to long BFP, RRE)
*   CONVERT FROM FIXED (64 to extended BFP, RRE)
*   CONVERT FROM FIXED (64 to short BFP, RRF-e)
*   CONVERT FROM FIXED (64 to long BFP, RRF-e)
*   CONVERT FROM FIXED (64 to extended BFP, RRF-e)
*
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R
* commands.
*
* Test Case Order
* 1) Int-32 to Short BFP
* 2) Int-32 to Short BFP with all rounding modes
* 3) Int-32 to Long BFP
* 4) Int-32 to Long BFP with all rounding modes
* 5) Int-32 to Extended BFP
*
* Provided test data:
*      1, 2, 4, -2,
*      9 223 372 036 854 775 807   (0x7FFFFFFFFFFF)
*      -9 223 372 036 854 775 807  (0x800000000000)
*   The last two values trigger inexact exceptions when converted
*   to long or short BFP and are used to exhaustively test
*   operation of the various rounding modes.  Int-64 to extended
*   BFP is always exact.
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
         EJECT
*
*  Note: for compatibility with the z/CMS test rig, do not change
*  or use R11, R14, or R15.  Everything else is fair game.
*
BFPCVTFF START 0
STRTLABL EQU   *
R0       EQU   0                   Work register for cc extraction
R1       EQU   1
R2       EQU   2                   Holds count of test input values
R3       EQU   3                   Points to next test input value(s)
R4       EQU   4                   Available
R5       EQU   5                   Available
R6       EQU   6                   Available
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
START    STCTL R0,R0,CTLR0    Store CR0 to enable AFP
         OI    CTLR0+1,X'04'  Turn on AFP bit
         LCTL  R0,R0,CTLR0    Reload updated CR0
*
         LA    R10,SHORTS     Point to int-64 test inputs
         BAS   R13,CEGBR      Convert values from fixed to short BFP
         LA    R10,RMSHORTS   Point to inputs for rounding mode tests
         BAS   R13,CEGBRA     Convert to short BFP using rm options
*
         LA    R10,LONGS      Point to int-64 test inputs
         BAS   R13,CDGBR      Convert values from fixed to long BFP
         LA    R10,RMLONGS    Point to inputs for rounding mode tests
         BAS   R13,CDGBRA     Convert to long BFP using rm options
*
         LA    R10,EXTDS      Point to int-64 test inputs
         BAS   R13,CXGBR      Convert values from fixed to extended
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
FPCREGNT DC    X'00000000'    FPCR Reg IEEE exceptions Not Trappable
FPCREGTR DC    X'F8000000'    FPCR Reg IEEE exceptions TRappable
*
* Input values parameter list, four fullwords:
*      1) Count,
*      2) Address of inputs,
*      3) Address to place results, and
*      4) Address to place DXC/Flags/cc values.
*
SHORTS   DS    0F           int-64 inputs for short BFP testing
         DC    A(INTCOUNT)
         DC    A(INTIN)
         DC    A(SBFPOUT)
         DC    A(SBFPFLGS)
*
LONGS    DS    0F           int-64 inputs for long BFP testing
         DC    A(INTCOUNT)
         DC    A(INTIN)
         DC    A(LBFPOUT)
         DC    A(LBFPFLGS)
*
EXTDS    DS    0F           int-64 inputs for Extended BFP testing
         DC    A(INTCOUNT)
         DC    A(INTIN)
         DC    A(XBFPOUT)
         DC    A(XBFPFLGS)
*
RMSHORTS DS    0F           int-64's for short BFP rounding mode tests
         DC    A(SINTRMCT)
         DC    A(SINTRMIN)  Last two int-64 are only concerns
         DC    A(SBFPRMO)   Space for rounding mode results
         DC    A(SBFPRMOF)  Space for rounding mode FPCR contents
*
RMLONGS  DS    0F           int-64's for long BFP rounding mode tests
         DC    A(LINTRMCT)
         DC    A(LINTRMIN)  Last two int-64 are only concerns
         DC    A(LBFPRMO)   Space for rounding mode results
         DC    A(LBFPRMOF)  Space for rounding mode FPCR contents
         EJECT
***********************************************************************
*
* Convert int-64 to short BFP format.  A pair of results is generated
* for each input: one with all exceptions non-trappable, and the second
* with all exceptions trappable.   The FPCR is stored for each result.
*
***********************************************************************
         SPACE 3

CEGBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         BASR  R12,0         Set top of loop
*
         LG    R1,0(,R3)     Get integer test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CEGBR FPR8,R1       Cvt Int in GPR1 to float in FPFPR8
         STE   FPR8,0(,R7)   Store short BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         CEGBR FPR8,R1       Cvt Int in GPR1 to float in FPFPR8
         STE   FPR8,4(,R7)   Store short BFP result
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         LA    R3,8(,R3)     Point to next input values
         LA    R7,8(,R7)     Point to next short BFP converted values
         LA    R8,8(,R8)     Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
*
* Convert int-64 to short BFP format using each possible rounding mode.
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPCR with
* the IEEE Inexact exception supressed.  SRNM (2-bit) is used  for the
* first two FPCR-controlled tests and SRNMB (3-bit) is used for the
* last two to get full coverage of that instruction pair.
*
* The next six results use instruction-specified rounding modes.
*
* The default rounding mode (0 for RNTE) is not tested in this section;
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.
*
CEGBRA   LM    R2,R3,0(R10)  Get count and address of test input values
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         BASR  R12,0         Set top of loop
*
         LG    R1,0(,R3)     Get int-64 test value
*
* Convert the Int-64 in GPR1 to float-64 in FPR8 using the rounding
* mode specified in the FPCR.  Mask inexact exceptions.
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  1             SET FPCR to RZ, towards zero.
         CEGBRA FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,0*4(,R7) Store short BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPCR to RP, to +infinity
         CEGBRA FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,1*4(,R7) Store short BFP result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, to -infinity
         CEGBRA FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,2*4(,R7) Store short BFP result
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RPS, Prepare for Shorter Precision
         CEGBRA FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,3*4(,R7) Store short BFP result
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
*
* Convert the Int-64 in GPR1 to float-64 in FPFPR8 using the rounding
* mode specified in the M3 field of the instruction.
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CEGBRA FPR8,1,R1,B'0000'  RNTA, to nearest, ties away
         STE   FPR8,4*4(,R7) Store short BFP result
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CEGBRA FPR8,3,R1,B'0000'  RPS, prepare for shorter precision
         STE   FPR8,5*4(,R7) Store short BFP result
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CEGBRA FPR8,4,R1,B'0000'  RNTE, to nearest, ties to even
         STE   FPR8,6*4(,R7) Store short BFP result
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CEGBRA FPR8,5,R1,B'0000'  RZ, toward zero
         STE   FPR8,7*4(,R7)  Store short BFP result
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CEGBRA FPR8,6,R1,B'0000'  RP, to +inf
         STE   FPR8,8*4(,R7) Store short BFP result
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CEGBRA FPR8,7,R1,B'0000'  RM, to -inf
         STE   FPR8,9*4(,R7) Store short BFP result
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
*
         LA    R3,8(,R3)     Point to next input value
         LA    R7,12*4(,R7)  Point to next short BFP rounded set
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert int-64 to long BFP format.  A pair of results is generated
* for each input: one with all exceptions non-trappable, and the second
* with all exceptions trappable.   The FPCR is stored for each result.
* Conversion of a 32-bit integer to long is always exact; no exceptions
* are expected
*
***********************************************************************
         SPACE 3
CDGBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LG    R1,0(,R3)    Get integer test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CDGBR FPR8,R1         Cvt Int in GPR1 to float in FPFPR8
         STD   FPR8,0(,R7)    Store long BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         CDGBR FPR8,R1         Cvt Int in GPR1 to float in FPFPR8
         STD   FPR8,8(,R7)    Store long BFP result
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         LA    R3,8(,R3)    point to next input value
         LA    R7,16(,R7)   Point to next long BFP converted value
         LA    R8,8(,R8)    Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
*
* Convert int-64 to long BFP format using each possible rounding mode.
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPCR with
* the IEEE Inexact exception supressed.  SRNM (2-bit) is used  for the
* first two FPCR-controlled tests and SRNMB (3-bit) is used for the
* last two to get full coverage of that instruction pair.
*
* The next six results use instruction-specified rounding modes.
*
* The default rounding mode (0 for RNTE) is not tested in this section;
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.
*
CDGBRA   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LG    R1,0(,R3)     Get int-64 test value
*
* Convert the Int-64 in GPR1 to float-64 in FPR8 using the rounding
* mode specified in the FPCR.  Mask inexact exceptions.
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  1             SET FPCR to RZ, towards zero.
         CDGBRA FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,0*8(,R7) Store long BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPCR to RP, to +infinity
         CDGBRA FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,1*8(,R7) Store long BFP result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, to -infinity
         CDGBRA FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,2*8(,R7) Store long BFP result
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RPS, Prepare for Shorter Precision
         CDGBRA FPR8,0,R1,B'0100'  FPCR ctl'd rounding, inexact masked
         STD   FPR8,3*8(,R7) Store long BFP result
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
*
* Convert the Int-64 in GPR1 to float-64 in FPFPR8 using the rounding
* mode specified in the M3 field of the instruction.
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CDGBRA FPR8,1,R1,B'0000'  RNTA, to nearest, ties away
         STD   FPR8,4*8(,R7) Store long BFP result
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CDGBRA FPR8,3,R1,B'0000'  RPS, prepare for shorter precision
         STD   FPR8,5*8(,R7) Store long BFP result
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CDGBRA FPR8,4,R1,B'0000'  RNTE, to nearest, ties to even
         STD   FPR8,6*8(,R7) Store long BFP result
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CDGBRA FPR8,5,R1,B'0000'  RZ, toward zero
         STD   FPR8,7*8(,R7) Store long BFP result
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CDGBRA FPR8,6,R1,B'0000'  RP, to +inf
         STD   FPR8,8*8(,R7) Store long BFP result
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         CDGBRA FPR8,7,R1,B'0000'  RM, to -inf
         STD   FPR8,9*8(,R7) Store long BFP result
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
*
         LA    R3,8(,R3)     Point to next input value
         LA    R7,10*8(,R7)  Point to next long BFP rounded value set
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert int-64 to extended BFP format.  A pair of results is
* generated * for each input: one with all exceptions non-trappable,
* and the second with all exceptions trappable.   The FPCR is stored
* for each result.  Conversion of an int-64to extended is always exact;
* no exceptions are expected.
*
***********************************************************************
         SPACE 3
CXGBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LG    R1,0(,R3)     Get integer test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         CXGBR FPR8,R1       Cvt Int in GPR1 to float in FPR8-FPR10
         STD   FPR8,0(,R7)   Store extended BFP result part 1
         STD   FPR10,8(,R7)  Store extended BFP result part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         CXGBR FPR8,R1       Cvt Int in GPR1 to float in FPR8-FPR10
         STD   FPR8,16(,R7)  Store extended BFP result part 1
         STD   FPR10,24(,R7) Store extended BFP result part 2
         STFPC 4(R8)         Store resulting FPCR flags and DXC
         LA    R3,8(,R3)     Point to next input value
         LA    R7,32(,R7)    Point to next extended BFP converted value
         LA    R8,8(,R8)     Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Int-64 inputs for Convert From Fixed testing.  The same set of
* inputs are used for short, long, and extended formats.  The last two
* values are used for rounding mode tests for short and long only;
* conversion of int-64 to extended is always exact.
*
***********************************************************************
         SPACE 3
INTIN    DS    0D
         DC    FD'1'
         DC    FD'2'
         DC    FD'4'
         DC    FD'-2'                    X'FFFFFFFF FFFFFFFE'
*
*                            Below inexact and incre. for short & long
         DC    FD'9223372036854775807'   X'7FFFFFFF FFFFFFFF'
         DC    FD'-9223372036854775807'  X'80000000 00000001'
*
*                            Below exact for all
         DC    FD'9223371487098961920'   X'7FFFFF80 00000000'
         DC    FD'-9223371487098961920'  X'80000080 00000000'
*
*                            Below exact for long and extended
         DC    FD'9223372036854774784'   X'7FFFFFFF FFFFFC00'
         DC    FD'-9223372036854774784'  X'80000000 00000400'
INTCOUNT EQU   (*-INTIN)/8    Count of int-64 input values
*
* int-64 inputs for exhaustive short BFP rounding mode tests
*
SINTRMIN DS    0D            Values for short BFP rounding tests
*                            Below rounds nearest away from zero
         DC    FD'9223371899415822336'   X'7FFFFFE0 00000000'
         DC    FD'-9223371899415822336'  X'80000020 00000000'
*                            Below rounds nearest tie
         DC    FD'9223371761976868864'   X'7FFFFFC0 00000000'
         DC    FD'-9223371761976868864'  X'80000040 00000000'
*                            Below rounds nearest toward zero
         DC    FD'9223371624537915392'   X'7FFFFFA0 00000000'
         DC    FD'-9223371624537915392'  X'80000060 00000000'
SINTRMCT EQU   (*-SINTRMIN)/8  Count of int-64 for rounding tests
*
* int-64 inputs for exhaustive long BFP rounding mode tests
*
LINTRMIN DS    0D            Values for short BFP rounding mode tests
*                            Below rounds nearest away from zero
         DC    FD'9223372036854775552'   X'7FFFFFFF FFFFFF00'
         DC    FD'-9223372036854775552'  X'80000000 00000100'
*                            Below rounds nearest tie
         DC    FD'9223372036854775296'   X'7FFFFFFF FFFFFE00'
         DC    FD'-9223372036854775296'  X'80000000 00000200'
*                            Below rounds nearest toward zero
         DC    FD'9223372036854775040'   X'7FFFFFFF FFFFFD00'
         DC    FD'-9223372036854775040'  X'80000000 00000300'
LINTRMCT EQU   (*-LINTRMIN)/8  Count of int-64 for rounding tests
         EJECT
***********************************************************************
*                 ACTUAL results saved here
***********************************************************************
*
*               Locations for ACTUAL results
*
*
SBFPOUT  EQU   STRTLABL+X'1000'    Int-64 results from short BFP inputs
*                                  ..6 pairs used, room for 32 pairs
SBFPFLGS EQU   STRTLABL+X'1100'    FPCR flags and DXC from short BFP
*                                  ..6 pairs used, room for 32 pairs
SBFPRMO  EQU   STRTLABL+X'1200'    Short BFP rounding mode results
*                                  ..2 sets used, room for 16
SBFPRMOF EQU   STRTLABL+X'1500'    Short BFP rndg mode FPCR contents
*                                  ..2 sets used, room for 32
*                                  ..next available location X'1600'
*
LBFPOUT  EQU   STRTLABL+X'2000'    Int-64 results from long BFP inputs
*                                  ..6 pairs used, room for 16 pairs
LBFPFLGS EQU   STRTLABL+X'2100'    Long BFP FPCR contents
*                                  ..6 pairs used, room for 32 pairs
LBFPRMO  EQU   STRTLABL+X'2200'    Long BFP rounding mode results
*                                  ..2 sets used, room for 16 sets
LBFPRMOF EQU   STRTLABL+X'2700'    Long BFP rndg mode FPCR contents
*                                  ..2 sets used, room for 16 sets
*                                  ..next available location X'2A00'
*
XBFPOUT  EQU   STRTLABL+X'3000'    Int-64 results from extended BFP
*                                  ..6 pairs used, room for 16 pairs
XBFPFLGS EQU   STRTLABL+X'3200'    CLXBR restulting FPCR contents
*                                  ..6 pairs used, room for 16 pairs
*                                  ..next available location X'3300'
         EJECT
***********************************************************************
*                    EXPECTED results
***********************************************************************
*
         ORG   STRTLABL+X'4000'   (past end of actual results)
*
SBFPOUT_GOOD EQU *
 DC CL48'CEGBR result pairs 1-2'
 DC XL16'3F8000003F8000004000000040000000'
 DC CL48'CEGBR result pairs 3-4'
 DC XL16'4080000040800000C0000000C0000000'
 DC CL48'CEGBR result pairs 5-6'
 DC XL16'5F0000005F000000DF000000DF000000'
 DC CL48'CEGBR result pairs 7-8'
 DC XL16'5EFFFFFF5EFFFFFFDEFFFFFFDEFFFFFF'
 DC CL48'CEGBR result pairs 9-10'
 DC XL16'5F0000005F000000DF000000DF000000'
SBFPOUT_NUM EQU (*-SBFPOUT_GOOD)/64
*
*
SBFPFLGS_GOOD EQU *
 DC CL48'CEGBR FPCR pairs 1-2'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CEGBR FPCR pairs 3-4'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CEGBR FPCR pairs 5-6'
 DC XL16'00080000F8000C0000080000F8000C00'
 DC CL48'CEGBR FPCR pairs 7-8'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CEGBR FPCR pairs 9-10'
 DC XL16'00080000F8000C0000080000F8000C00'
SBFPFLGS_NUM EQU (*-SBFPFLGS_GOOD)/64
*
*
SBFPRMO_GOOD EQU *
 DC CL48'CEGBRA +away result FPCRmodes 1-3, 7'
 DC XL16'5EFFFFFF5F0000005EFFFFFF5EFFFFFF'
 DC CL48'CEGBRA +away result M3 modes 1, 3-5'
 DC XL16'5F0000005EFFFFFF5F0000005EFFFFFF'
 DC CL48'CEGBRA +away result M3 modes 6, 7'
 DC XL16'5F0000005EFFFFFF0000000000000000'
 DC CL48'CEGBRA -away result FPCRmodes 1-3, 7'
 DC XL16'DEFFFFFFDEFFFFFFDF000000DEFFFFFF'
 DC CL48'CEGBRA -away result M3 modes 1, 3-5'
 DC XL16'DF000000DEFFFFFFDF000000DEFFFFFF'
 DC CL48'CEGBRA -away result M3 modes 6, 7'
 DC XL16'DEFFFFFFDF0000000000000000000000'
 DC CL48'CEGBRA +tie result FPCRmodes 1-3, 7'
 DC XL16'5EFFFFFF5F0000005EFFFFFF5EFFFFFF'
 DC CL48'CEGBRA +tie result M3 modes 1, 3-5'
 DC XL16'5F0000005EFFFFFF5F0000005EFFFFFF'
 DC CL48'CEGBRA +tie result M3 modes 6, 7'
 DC XL16'5F0000005EFFFFFF0000000000000000'
 DC CL48'CEGBRA -tie result FPCRmodes 1-3, 7'
 DC XL16'DEFFFFFFDEFFFFFFDF000000DEFFFFFF'
 DC CL48'CEGBRA -tie result M3 modes 1, 3-5'
 DC XL16'DF000000DEFFFFFFDF000000DEFFFFFF'
 DC CL48'CEGBRA -tie result M3 modes 6, 7'
 DC XL16'DEFFFFFFDF0000000000000000000000'
 DC CL48'CEGBRA +tozero result FPCRmodes 1-3, 7'
 DC XL16'5EFFFFFF5F0000005EFFFFFF5EFFFFFF'
 DC CL48'CEGBRA +tozero result M3 modes 1, 3-5'
 DC XL16'5EFFFFFF5EFFFFFF5EFFFFFF5EFFFFFF'
 DC CL48'CEGBRA +tozero result M3 modes 6, 7'
 DC XL16'5F0000005EFFFFFF0000000000000000'
 DC CL48'CEGBRA -tozero result FPCRmodes 1-3, 7'
 DC XL16'DEFFFFFFDEFFFFFFDF000000DEFFFFFF'
 DC CL48'CEGBRA -tozero result M3 modes 1, 3-5'
 DC XL16'DEFFFFFFDEFFFFFFDEFFFFFFDEFFFFFF'
 DC CL48'CEGBRA -tozero result M3 modes 6, 7'
 DC XL16'DEFFFFFFDF0000000000000000000000'
SBFPRMO_NUM EQU (*-SBFPRMO_GOOD)/64
*
*
SBFPRMOF_GOOD EQU *
 DC CL48'CEGBRA +away FPCRmodes 1-3, 7 FPCR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'CEGBRA +away M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'CEGBRA +away M3 modes 6, 7 FPCR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'CEGBRA -away FPCRmodes 1-3, 7 FPCR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'CEGBRA -away M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'CEGBRA -away M3 modes 6, 7 FPCR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'CEGBRA +tie FPCRmodes 1-3, 7 FPCR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'CEGBRA +tie M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'CEGBRA +tie M3 modes 6, 7 FPCR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'CEGBRA -tie FPCRmodes 1-3, 7 FPCR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'CEGBRA -tie M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'CEGBRA -tie M3 modes 6, 7 FPCR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'CEGBRA +tozero FPCRmodes 1-3, 7 FPCR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'CEGBRA +tozero M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'CEGBRA +tozero M3 modes 6, 7 FPCR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'CEGBRA -tozero FPCRmodes 1-3, 7 FPCR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'CEGBRA -tozero M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'CEGBRA -tozero M3 modes 6, 7 FPCR'
 DC XL16'00080000000800000000000000000000'
SBFPRMOF_NUM EQU (*-SBFPRMOF_GOOD)/64
*
*
LBFPOUT_GOOD EQU *
 DC CL48'CDGBR result pair 1'
 DC XL16'3FF00000000000003FF0000000000000'
 DC CL48'CDGBR result pair 2'
 DC XL16'40000000000000004000000000000000'
 DC CL48'CDGBR result pair 3'
 DC XL16'40100000000000004010000000000000'
 DC CL48'CDGBR result pair 4'
 DC XL16'C000000000000000C000000000000000'
 DC CL48'CDGBR result pair 5'
 DC XL16'43E000000000000043E0000000000000'
 DC CL48'CDGBR result pair 6'
 DC XL16'C3E0000000000000C3E0000000000000'
LBFPOUT_NUM EQU (*-LBFPOUT_GOOD)/64
*
*
LBFPFLGS_GOOD EQU *
 DC CL48'CDGBR FPCR pairs 1-2'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CDGBR FPCR pairs 3-4'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CDGBR FPCR pairs 5-6'
 DC XL16'00080000F8000C0000080000F8000C00'
LBFPFLGS_NUM EQU (*-LBFPFLGS_GOOD)/64
*
*
LBFPRMO_GOOD EQU *
 DC CL48'CDGBRA +away FPCRmodes 1, 2'
 DC XL16'43DFFFFFFFFFFFFF43E0000000000000'
 DC CL48'CDGBRA +away FPCRmodes 3, 7'
 DC XL16'43DFFFFFFFFFFFFF43DFFFFFFFFFFFFF'
 DC CL48'CDGBRA +away M3 modes 1, 3'
 DC XL16'43E000000000000043DFFFFFFFFFFFFF'
 DC CL48'CDGBRA +away M3 modes 4, 5'
 DC XL16'43E000000000000043DFFFFFFFFFFFFF'
 DC CL48'CDGBRA +away M3 modes 6, 7'
 DC XL16'43E000000000000043DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -away FPCRmodes 1, 2'
 DC XL16'C3DFFFFFFFFFFFFFC3DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -away FPCRmodes 3, 7'
 DC XL16'C3E0000000000000C3DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -away M3 modes 1, 3'
 DC XL16'C3E0000000000000C3DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -away M3 modes 4, 5'
 DC XL16'C3E0000000000000C3DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -away M3 modes 6, 7'
 DC XL16'C3DFFFFFFFFFFFFFC3E0000000000000'
 DC CL48'CDGBRA +tie FPCRmodes 1, 2'
 DC XL16'43DFFFFFFFFFFFFF43E0000000000000'
 DC CL48'CDGBRA +tie FPCRmodes 3, 7'
 DC XL16'43DFFFFFFFFFFFFF43DFFFFFFFFFFFFF'
 DC CL48'CDGBRA +tie M3 modes 1, 3'
 DC XL16'43E000000000000043DFFFFFFFFFFFFF'
 DC CL48'CDGBRA +tie M3 modes 4, 5'
 DC XL16'43E000000000000043DFFFFFFFFFFFFF'
 DC CL48'CDGBRA +tie M3 modes 6, 7'
 DC XL16'43E000000000000043DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -tie FPCRmodes 1, 2'
 DC XL16'C3DFFFFFFFFFFFFFC3DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -tie FPCRmodes 3, 7'
 DC XL16'C3E0000000000000C3DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -tie M3 modes 1, 3'
 DC XL16'C3E0000000000000C3DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -tie M3 modes 4, 5'
 DC XL16'C3E0000000000000C3DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -tie M3 modes 6, 7'
 DC XL16'C3DFFFFFFFFFFFFFC3E0000000000000'
 DC CL48'CDGBRA +tozero FPCRmodes 1, 2'
 DC XL16'43DFFFFFFFFFFFFF43E0000000000000'
 DC CL48'CDGBRA +tozero FPCRmodes 3, 7'
 DC XL16'43DFFFFFFFFFFFFF43DFFFFFFFFFFFFF'
 DC CL48'CDGBRA +tozero M3 modes 1, 3'
 DC XL16'43DFFFFFFFFFFFFF43DFFFFFFFFFFFFF'
 DC CL48'CDGBRA +tozero M3 modes 4, 5'
 DC XL16'43DFFFFFFFFFFFFF43DFFFFFFFFFFFFF'
 DC CL48'CDGBRA +tozero M3 modes 6, 7'
 DC XL16'43E000000000000043DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -tozero FPCRmodes 1, 2'
 DC XL16'C3DFFFFFFFFFFFFFC3DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -tozero FPCRmodes 3, 7'
 DC XL16'C3E0000000000000C3DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -tozero M3 modes 1, 3'
 DC XL16'C3DFFFFFFFFFFFFFC3DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -tozero M3 modes 4, 5'
 DC XL16'C3DFFFFFFFFFFFFFC3DFFFFFFFFFFFFF'
 DC CL48'CDGBRA -tozero M3 modes 6, 7'
 DC XL16'C3DFFFFFFFFFFFFFC3E0000000000000'
LBFPRMO_NUM EQU (*-LBFPRMO_GOOD)/64
*
*
LBFPRMOF_GOOD EQU *
 DC CL48'CDGBRA +away FPCRmodes 1-3, 7 FPCR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'CDGBRA +away M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'CDGBRA +away M3 modes 6, 7 FPCR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'CDGBRA -away FPCRmodes 1-3, 7 FPCR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'CDGBRA -away M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'CDGBRA -away M3 modes 6, 7 FPCR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'CDGBRA +tie FPCRmodes 1-3, 7 FPCR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'CDGBRA +tie M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'CDGBRA +tie M3 modes 6, 7 FPCR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'CDGBRA -tie FPCRmodes 1-3, 7 FPCR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'CDGBRA -tie M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'CDGBRA -tie M3 modes 6, 7 FPCR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'CDGBRA +tozero FPCRmodes 1-3, 7 FPCR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'CDGBRA +tozero M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'CDGBRA +tozero M3 modes 6, 7 FPCR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'CDGBRA -tozero FPCRmodes 1-3, 7 FPCR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'CDGBRA -tozero M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'CDGBRA -tozero M3 modes 6, 7 FPCR'
 DC XL16'00080000000800000000000000000000'
LBFPRMOF_NUM EQU (*-LBFPRMOF_GOOD)/64
*
*
XBFPOUT_GOOD EQU *
 DC CL48'CXGBR result 1a'
 DC XL16'3FFF0000000000000000000000000000'
 DC CL48'CXGBR result 1b'
 DC XL16'3FFF0000000000000000000000000000'
 DC CL48'CXGBR result 2a'
 DC XL16'40000000000000000000000000000000'
 DC CL48'CXGBR result 2b'
 DC XL16'40000000000000000000000000000000'
 DC CL48'CXGBR result 3a'
 DC XL16'40010000000000000000000000000000'
 DC CL48'CXGBR result 3b'
 DC XL16'40010000000000000000000000000000'
 DC CL48'CXGBR result 4a'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'CXGBR result 4b'
 DC XL16'C0000000000000000000000000000000'
 DC CL48'CXGBR result 5a'
 DC XL16'403DFFFFFFFFFFFFFFFC000000000000'
 DC CL48'CXGBR result 5b'
 DC XL16'403DFFFFFFFFFFFFFFFC000000000000'
 DC CL48'CXGBR result 6a'
 DC XL16'C03DFFFFFFFFFFFFFFFC000000000000'
 DC CL48'CXGBR result 6b'
 DC XL16'C03DFFFFFFFFFFFFFFFC000000000000'
XBFPOUT_NUM EQU (*-XBFPOUT_GOOD)/64
*
*
XBFPFLGS_GOOD EQU *
 DC CL48'CXGBR FPCRpairs 1-2'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CXGBR FPCRpairs 3-4'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'CXGBR FPCRpairs 5-6'
 DC XL16'00000000F800000000000000F8000000'
XBFPFLGS_NUM EQU (*-XBFPFLGS_GOOD)/64
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
         DC    A(SBFPOUT)
         DC    A(SBFPOUT_GOOD)
         DC    A(SBFPOUT_NUM)
*
         DC    A(SBFPFLGS)
         DC    A(SBFPFLGS_GOOD)
         DC    A(SBFPFLGS_NUM)
*
         DC    A(SBFPRMO)
         DC    A(SBFPRMO_GOOD)
         DC    A(SBFPRMO_NUM)
*
         DC    A(SBFPRMOF)
         DC    A(SBFPRMOF_GOOD)
         DC    A(SBFPRMOF_NUM)
*
         DC    A(LBFPOUT)
         DC    A(LBFPOUT_GOOD)
         DC    A(LBFPOUT_NUM)
*
         DC    A(LBFPFLGS)
         DC    A(LBFPFLGS_GOOD)
         DC    A(LBFPFLGS_NUM)
*
         DC    A(LBFPRMO)
         DC    A(LBFPRMO_GOOD)
         DC    A(LBFPRMO_NUM)
*
         DC    A(LBFPRMOF)
         DC    A(LBFPRMOF_GOOD)
         DC    A(LBFPRMOF_NUM)
*
         DC    A(XBFPOUT)
         DC    A(XBFPOUT_GOOD)
         DC    A(XBFPOUT_NUM)
*
         DC    A(XBFPFLGS)
         DC    A(XBFPFLGS_GOOD)
         DC    A(XBFPFLGS_NUM)
*
VERIFLEN EQU   (*-VERIFTAB)/12    #of entries in verify table
                                                                EJECT
         END
