  TITLE 'bfp-002-loadr: Test IEEE Load Rounded'
***********************************************************************
*
*Testcase IEEE LOAD ROUNDED
*  Test case capability includes IEEE exceptions, trappable and
*  otherwise.  Test result, FPCR flags, and DXC saved for all tests.
*  Load Rounded does not set the condition code.
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
***********************************************************************
         SPACE 2
***********************************************************************
*
*                        bfp-002-loadr.asm
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
***********************************************************************
*
*
*Testcase IEEE LOAD ROUNDED
*  Test case capability includes ieee exceptions trappable and
*  otherwise.  Test result, FPCR flags, and DXC saved for all tests.
*  Load Rounded does not set the condition code.
*
* Tests the following three conversion instructions
*   LOAD ROUNDED (long to short BFP, RRE)
*   LOAD ROUNDED (extended to short BFP, RRE)
*   LOAD ROUNDED (extended to long BFP, RRE)
*   LOAD ROUNDED (long short BFP, RRF-e)
*   LOAD ROUNDED (extended to long BFP, RRF-e)
*   LOAD ROUNDED (extended to short BFP, RRF-e)
*
* This routine exhaustively tests rounding in 32- and 64-bit binary
* floating point.  It is not possible to use Load Rounded to test
* rounding of 128-bit results.  There is no Load Rounded that returns
* a 128-bit result.
*
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R
* commands.
*
* Test Case Order
* 1) Long to short BFP basic tests (exception traps and flags, NaNs)
* 2) Long to short BFP rounding mode tests
* 3) Extended to short BFP basic tests
* 4) Extended to short BFP rounding mode tests
* 5) Extended to long BFP basic tests.
* 6) Extended to long BFP rounding mode tests
* 7) Long to short BFP trappable underflow and overflow tests
* 8) Extended to short BFP trappable underflow and overflow tests
* 9) Extended to Long BFP trappable underflow and overflow tests
*
* Test data is 'white box,' meaning it is keyed to the internal
* characteristics of Softfloat 3a, while expecting results to conform
* to the z/Architecture Principles of Opeartion, SA22-7832-10.
*
* In the discussion below, "stored significand" does not include the
* implicit units digit that is always assumed to be one for a non-
* tiny Binary Floating Point value.
*
* Round long or extended to short: Softfloat uses the left-most 30
*   bits of the long or extended BFP stored significand for
*   rounding, which means 7 'extra' bits participate in the
*   rounding.  If any of the right-hand 22 bits are non-zero, the
*   30-bit pre-rounded value is or'd with 1 in the low-order bit
*   position.  Bit 30 is the "sticky bit."
*
* Round extended to long: Softfloat uses the left-most 62 bits of
*   the extended BFP stored significand for rounding, which means
*   10 'extra' bits participate in the rounding.  If any of the
*   remaining right-hand 50 bits are non-zero, the 62-bit pre-
*   rounded value is or'd with 1 in the low-order bit position.  Bit 62
*   is the "sticky bit."  At least one of the test cases will have one
*   bits in only the low-order 64 bits of the stored significand.
*
* The or'd 1 bit representing the bits not participating in the
*   rounding process prevents false exacts.  False exacts would
*   otherwise occur when the extra 7 or 10 bits that participate
*   in rounding are zero and bits to the right of them are not.
*
* Basic test cases are needed as follows:
*   0, +1.5, -1.5, QNaN, SNaN,
*
* Rounding test cases are needed as follows:
*   Exact results are represented (no rounding needed)
*   Ties are represented, both even (round down) and odd (round up)
*   False exacts are represented
*   Nearest value is toward zero
*   Nearest value is away from zero.
*   Each of the above must be represented in positive and negative.
*
* Because rounding decisions are based on the binary significand,
*   there is limited value to considering test case inputs in
*   decimal form.  The binary representations are all that is
*   important.
*
* If overflow/underflow occur and are trappable, the result should
*   be in the source format but scaled to the target precision.
*   These test cases are handled by both the basic tests to
*   ensure that the non-trap results are correct and again by
*   specific trappable overflow/underflow tests to ensure that the
*   scaled result rounded to target precision is returned in
*   the source format.
*
* Overflow/underflow behavior also means that result registers
*   must be sanitized and allocated in pairs for extended inputs;
*   results must store source format registers.  Basic tests
*   for overflow/underflow only store the target precision, so
*   *Want needs to be coded accordingly.  The trappable
*   overflow/underflow tests store the source format.
*
*   Overflow/underflow test cases include inputs that overflow
*   the target precision and that result in a tiny in the target.
*   Rounding mode for all overflow/underflow testing is Round
*   to Nearest, Ties to Even (RNTE).
*
* Rounding test cases are needed as follows:
*   Exact results are represented (no rounding needed)
*   Ties are represented, both even (round down) and odd (round up)
*   False exacts are represented
*   Nearest value is toward zero
*   Nearest value is away from zero.
*   Each of the above must be represented in positive and negative.
*
* Because rounding decisions are based on the binary significand,
*   there is limited value to considering test case inputs in
*   decimal form.  The binary representations are all that is
*   important.
*
* Three input test data sets are provided, one for long to short, one
*   for extended to short, and one for extended to long.  We cannot use
*   the same extended inputs for long and short results because the
*   rounding points differ for the two result precisions.
*
* Also tests the following floating point support instructions
*   LOAD  (Short)
*   LOAD  (Long)
*   LFPC  (Load Floating Point Control Register)
*   SRNMB (Set BFP Rounding Mode 3-bit)
*   STFPC (Store Floating Point Control Register)
*   STORE (Short)
*   STORE (Long)
*
BFPLDRND START 0
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
         ORG   BFPLDRND+X'8E'      Program check interrution code
PCINTCD  DS    H
*
PCOLDPSW EQU   BFPLDRND+X'150'     z/Arch Program check old PSW
*
         ORG   BFPLDRND+X'1A0'     z/Arch Restart PSW
         DC    X'0000000180000000',AD(START)
*
         ORG   BFPLDRND+X'1D0'     z/Arch Program check NEW PSW
         DC    X'0000000000000000',AD(PROGCHK)
*
* Program check routine.  If Data Exception, continue execution at
* the instruction following the program check.  Otherwise, hard wait.
* No need to collect data.  All interesting DXC stuff is captured
* in the FPCR.
*
         ORG   BFPLDRND+X'200'
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
* Long Load Rounded to short tests
*
         LA    R10,LTOSBAS    Long BFP test inputs
         BAS   R13,LEDBR      Load rounded to short BFP
         LA    R10,LTOSRM     Long BFP inputs for rounding tests
         BAS   R13,LEDBRA     Round to short BFP using rm options
*
* Extended Load Rounded to short tests
*
         LA    R10,XTOSBAS    Point to extended BFP test inputs
         BAS   R13,LEXBR      Load rounded to short BFP
         LA    R10,XTOSRM     Extended BFP inputs for rounding tests
         BAS   R13,LEXBRA     Round to short BFP using rm options
*
* Extended Load Rounded to short tests
*
         LA    R10,XTOLBAS    Point to extended BFP test inputs
         BAS   R13,LDXBR      Load rounded to long BFP
         LA    R10,XTOLRM     Extended BFP inputs for rounding tests
         BAS   R13,LDXBRA     Round to long BFP using rm options
*
* Trappable long to short tests
*
         LA    R10,LTOSOU     Long BFP over/underflow test inputs
         BAS   R13,LEDBROUT   Load rounded to short BFP, trappable
*
* Trappable extended to short tests
*
         LA    R10,XTOSOU     Extended BFP over/underflow test inputs
         BAS   R13,LEXBROUT   Load rounded to short BFP, trappable
*
* Trappable extended to long tests
*
         LA    R10,XTOLOU     Extended BFP over/underflow test inputs
         BAS   R13,LDXBROUT   Load rounded to long BFP, trappable
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
LTOSBAS  DS    0F           Inputs for long to short BFP tests
         DC    A(LTOSCT/8)
         DC    A(LTOSIN)
         DC    A(LTOSOUT)
         DC    A(LTOSFLGS)
*
XTOSBAS  DS    0F           Inputs for extended to short BFP tests
         DC    A(XTOSCT/16)
         DC    A(XTOSIN)
         DC    A(XTOSOUT)
         DC    A(XTOSFLGS)
*
XTOLBAS  DS    0F           Inputs for extended to long BFP tests
         DC    A(XTOLCT/16)
         DC    A(XTOLIN)
         DC    A(XTOLOUT)
         DC    A(XTOLFLGS)
*
LTOSRM   DS    0F       Inputs for long to short BFP rounding tests
         DC    A(LTOSRMCT/8)
         DC    A(LTOSINRM)
         DC    A(LTOSRMO)
         DC    A(LTOSRMOF)
*
XTOSRM   DS    0F       Inputs for extended to short BFP rounding tests
         DC    A(XTOSRMCT/16)
         DC    A(XTOSINRM)
         DC    A(XTOSRMO)
         DC    A(XTOSRMOF)
*
XTOLRM   DS    0F       Inputs for extended to long BFP rounding tests
         DC    A(XTOLRMCT/16)
         DC    A(XTOLINRM)
         DC    A(XTOLRMO)
         DC    A(XTOLRMOF)
*
LTOSOU   DS    0F       Inputs for long to short BFP rounding tests
         DC    A(LTOSOUCT/8)
         DC    A(LTOSINOU)
         DC    A(LTOSOUO)
         DC    A(LTOSOUOF)
*
XTOSOU   DS    0F       Inputs for extended to short BFP rounding tests
         DC    A(XTOSOUCT/16)
         DC    A(XTOSINOU)
         DC    A(XTOSOUO)
         DC    A(XTOSOUOF)
*
XTOLOU   DS    0F       Inputs for extended to long BFP rounding tests
         DC    A(XTOLOUCT/16)
         DC    A(XTOLINOU)
         DC    A(XTOLOUO)
         DC    A(XTOLOUOF)
         EJECT
***********************************************************************
*
* Round long BFP to short BFP.  A pair of results is generated for each
* input: one with all exceptions non-trappable, and the second with all
* exceptions trappable.   The FPCR contents are stored for each result.
*
***********************************************************************
         SPACE 2
LEDBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LZDR  FPR1          Zero FRP1 to clear any residual
         LD    FPR0,0(,R3)   Get long BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         LEDBR FPR1,FPR0     Cvt long in FPR0 to short in FPR1
         STE   FPR1,0(,R7)   Store short BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LZDR  FPR1          Zero FRP1 to clear any residual
         LFPC  FPCREGTR      Set exceptions trappable
         LEDBR FPR1,FPR0     Cvt long in FPR0 to short in FPR1
         STE   FPR1,4(,R7)   Store short BFP result
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LA    R3,8(,R3)     Point to next input value
         LA    R7,8(,R7)     Point to next result pair
         LA    R8,8(,R8)     Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Round long BFP to short BFP.  Inputs are expected to generate
* overflow or underflow exceptions, all of which are trappable.  This
* means a scaled result should be generated rounded to the target
* precision but returned in the source precision.   The FPCR contents
* are stored for each result.
*
***********************************************************************
          SPACE 2
LEDBROUT LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LZDR  FPR1          Zero FRP1 to clear any residual
         LD    FPR0,0(,R3)   Get long BFP test value
         LFPC  FPCREGTR      Set exceptions trappable
         LEDBR FPR1,FPR0     Cvt long in FPR0 into short in FPR1
         STD   FPR1,0(,R7)   Store long scaled BFP trapped result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LA    R3,8(,R3)     Point to next input value
         LA    R7,8(,R7)     Point to next long trapped result value
         LA    R8,4(,R8)     Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert long BFP to rounded short BFP using each possible rounding
* mode.  Ten test results are generated for each input.  A 48-byte test
* result section is used to keep results sets aligned on a quad-double
* word.
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
LEDBRA   LM    R2,R3,0(R10)  Get count and address of test input values
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
         SRNM  1             SET FPCR to RZ, Round towards zero.
         LEDBRA FPR1,0,FPR0,B'0100'  FPCR ctl'd rounding, mask inexact
         STE   FPR1,0*4(,R7) Store shortened rounded BFP result
         STFPC 0(R8)         Store resulting FPCRflags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPCR to RP, Round to +infinity
         LEDBRA FPR1,0,FPR0,B'0100'  FPCR ctl'd rounding, mask inexact
         STE   FPR1,1*4(,R7) Store shortened rounded BFP result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, Round to -infinity
         LEDBRA FPR1,0,FPR0,B'0100'  FPCR ctl'd rounding, mask inexact
         STE   FPR1,2*4(,R7) Store shortened rounded BFP result
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RFS, Round Prepare for Shorter Precision
         LEDBRA FPR1,0,FPR0,B'0100'  FPCR ctl'd rounding, mask inexact
         STE   FPR1,3*4(,R7) Store shortened rounded BFP result
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
*
* Test cases using rounding mode specified in the instruction M3 field
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LEDBRA FPR1,1,FPR0,B'0000'  RNTA, to nearest, ties away
         STE   FPR1,4*4(,R7) Store shortened rounded BFP result
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LEDBRA FPR1,3,FPR0,B'0000'  RFS, prepare for shorter precision
         STE   FPR1,5*4(,R7) Store shortened rounded BFP result
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LEDBRA FPR1,4,FPR0,B'0000'  RNTE, to nearest, ties to even
         STE   FPR1,6*4(,R7) Store shortened rounded BFP result
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LEDBRA FPR1,5,FPR0,B'0000'  RZ, toward zero
         STE   FPR1,7*4(,R7) Store shortened rounded BFP result
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LEDBRA FPR1,6,FPR0,B'0000' RP, to +inf
         STE   FPR1,8*4(,R7) Store shortened rounded BFP result
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LEDBRA FPR1,7,FPR0,B'0000' RM, to -inf
         STE   FPR1,9*4(,R7) Store shortened rounded BFP result
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
*
         LA    R3,8(,R3)     Point to next input value
         LA    R7,12*4(,R7)  Point to next short BFP result pair
         LA    R8,12*4(,R8)  Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Round extended BFP to short BFP.  A pair of results is genearted for
* each input: one with all exceptions non-trappable, and the second
* with all exceptions trappable.   The FPCR contents are stored for
* each result.
*
***********************************************************************
         SPACE 2
LEXBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)   Get extended BFP test value part 1
         LD    R2,8(,R3)     Get extended BFP test value part 2
         LFPC  FPCREGNT      Set exceptions non-trappable
         LEXBR FPR1,FPR0     Cvt extended in FPR0-2 to short in FPR1
         STE   R1,0(,R7)     Store short BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LZDR  FPR1          Eliminate any residual results
         LEXBR FPR1,FPR0     Cvt extended in FPR0-2 to short in FPR1
         STE   FPR1,4(,R7)   Store short BFP result
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LA    R3,16(,R3)    Point to next input value
         LA    R7,8(,R7)     Point to next long rounded value pair
         LA    R8,8(,R8)     Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Round extended BFP to short BFP.  Inputs are expected to generate
* overflow or underflow exceptions, all of which are trappable.  This
* means a scaled result should be generated rounded to the target
* precision but returned in the source precision.   The FPCR contents
* are stored for each result.
*
***********************************************************************
         SPACE 2
LEXBROUT LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)   Get extended BFP test value part 1
         LD    R2,8(,R3)     Get extended BFP test value part 2
         LFPC  FPCREGTR      Set exceptions trappable
         LEXBR FPR1,FPR0     Cvt float in FPR0-2 to scaled in FPR1-3
         STD   FPR1,0(,R7)   Store scaled extended BFP result part 1
         STD   FPR3,8(,R7)   Store scaled extended BFP result part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LA    R3,16(,R3)    Point to next input value
         LA    R7,16(,R7)    Point to next extended trapped result
         LA    R8,4(,R8)     Point to next FPCR result area
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
* The default rounding mode (0 for RNTE) is not tested in this
* section; prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.
*
***********************************************************************
         SPACE 2
LEXBRA   LM    R2,R3,0(R10)  Get count and address of test input values
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
         SRNM  1             SET FPCR to RZ, Round towards zero.
         LEXBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, mask inexact
         STD   FPR1,0*4(,R7) Store shortened rounded BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPCR to RP, Round to +infinity
         LEXBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, mask inexact
         STD   FPR1,1*4(,R7) Store shortened rounded BFP result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, Round to -infinity
         LEXBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, mask inexact
         STD   FPR1,2*4(,R7) Store shortened rounded BFP result
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RFS, Round Prepare for Shorter Precision
         LEXBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, mask inexact
         STD   FPR1,3*4(,R7) Store shortened rounded BFP result
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
*
* Test cases using rounding mode specified in the instruction M3 field
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LEXBRA FPR1,1,FPR0,B'0000' RNTA, to nearest, ties away
         STD   FPR1,4*4(,R7) Store shortened rounded BFP result
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LEXBRA FPR1,3,FPR0,B'0000' RFS, prepare for shorter precision
         STD   FPR1,5*4(,R7) Store shortened rounded BFP result
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LEXBRA FPR1,4,FPR0,B'0000' RNTE, to nearest, ties to even
         STD   FPR1,6*4(,R7) Store shortened rounded BFP result
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LEXBRA FPR1,5,FPR0,B'0000' RZ, toward zero
         STD   FPR1,7*4(,R7) Store shortened rounded BFP result
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LEXBRA FPR1,6,FPR0,B'0000' RP, to +inf
         STD   FPR1,8*4(,R7) Store shortened rounded BFP result
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LEXBRA FPR1,7,FPR0,B'0000' RM, to -inf
         STD   FPR1,9*4(,R7) Store shortened rounded BFP result
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
*
         LA    R3,16(,R3)    Point to next input value
         LA    R7,12*4(,R7)  Point to next long BFP converted values
         LA    R8,12*4(,R8)  Point to next FPCR/CC result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Round extended BFP to long BFP.  A pair of results is generated for
* each input: one with all exceptions non-trappable, and the second
* with all exceptions trappable.   The FPCR contents are stored for
* each result.
*
***********************************************************************
          SPACE 2
LDXBR    LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)   Get extended BFP test value part 1
         LD    R2,8(,R3)     Get extended BFP test value part 1
         LFPC  FPCREGNT      Set exceptions non-trappable
         LDXBR FPR1,FPR0     Round extended in FPR0-2 to long in FPR1
         STD   FPR1,0(,R7)   Store shortened rounded BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LZXR  FPR1          Eliminate any residual results
         LDXBR FPR1,FPR0     Round extended in FPR0-2 to long in FPR1
         STD   FPR1,8(,R7)   Store shortened rounded BFP result
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LA    R3,16(,R3)    Point to next extended BFP input value
         LA    R7,16(,R7)    Point to next long BFP rounded value pair
         LA    R8,8(,R8)     Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Round extended BFP to long BFP.  Inputs are expected to generate
* overflow or underflow exceptions, all of which are trappable.  This
* means a scaled result should be generated rounded to the target
* precision but returned in the source precision.   The FPCR contents
* are stored for each result.
*
***********************************************************************
         SPACE 2
LDXBROUT LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)   Get extended BFP test value part 1
         LD    R2,8(,R3)     Get extended BFP test value part 1
         LFPC  FPCREGTR      Set exceptions trappable
         LDXBR FPR1,FPR0     Round ext'd in FPR0-2 to scaled in FPR1-3
         STD   FPR1,0(,R7)   Store scaled extended BFP result part 1
         STD   FPR3,8(,R7)   Store scaled extended BFP result part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LA    R3,16(,R3)    Point to next extended BFP input value
         LA    R7,16(,R7)    Point to next long BFP rounded value pair
         LA    R8,4(,R8)     Point to next FPCR result area
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
* The default rounding mode (0 for RNTE) is not tested in this
* section; `prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.
*
***********************************************************************
         SPACE 2
LDXBRA   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR0,0(,R3)    Get extended BFP test value part 1
         LD    R2,8(,R3)    Get extended BFP test value part 2
*
* Test cases using rounding mode specified in the FPCR
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  1             SET FPCR to RZ, Round towards zero.
         LDXBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, mask inexact
         STD   FPR1,0*8(,R7) Store shortened rounded BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNM  2             SET FPCR to RP, Round to +infinity
         LDXBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, mask inexact
         STD   FPR1,1*8(,R7) Store shortened rounded BFP result
         STFPC 1*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 3             SET FPCR to RM, Round to -infinity
         LDXBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, mask inexact
         STD   FPR1,2*8(,R7) Store shortened rounded BFP result
         STFPC 2*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         SRNMB 7             RFS, Round Prepare for Shorter Precision
         LDXBRA FPR1,0,FPR0,B'0100' FPCR ctl'd rounding, mask inexact
         STD   FPR1,3*8(,R7) Store shortened rounded BFP result
         STFPC 3*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LDXBRA FPR1,1,FPR0,B'0000' RNTA, to nearest, ties away
         STD   FPR1,4*8(,R7) Store shortened rounded BFP result
         STFPC 4*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LDXBRA FPR1,3,FPR0,B'0000' RFS, prepare for shorter precision
         STD   FPR1,5*8(,R7) Store shortened rounded BFP result
         STFPC 5*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LDXBRA FPR1,4,FPR0,B'0000' RNTE, to nearest, ties to even
         STD   FPR1,6*8(,R7) Store shortened rounded BFP result
         STFPC 6*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LDXBRA FPR1,5,FPR0,B'0000' RZ, toward zero
         STD   FPR1,7*8(,R7) Store shortened rounded BFP result
         STFPC 7*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LDXBRA FPR1,6,FPR0,B'0000' RP, to +inf
         STD   FPR1,8*8(,R7) Store shortened rounded BFP result
         STFPC 8*4(R8)       Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable, clear flags
         LDXBRA FPR1,7,FPR0,B'0000' RM, to -inf
         STD   FPR1,9*8(,R7) Store shortened rounded BFP result
         STFPC 9*4(R8)       Store resulting FPCR flags and DXC
*
         LA    R3,16(,R3)    Point to next input value
         LA    R7,10*8(,R7)  Point to next long BFP rounded result
         LA    R8,12*4(,R8)  Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* BFP inputs.  One set of longs and two sets of extendeds are included.
* Each set includes input values for basic exception testing and input
* values for exhaustive rounding mode testing.  One set of extended
* inputs is used to generate short results, and the other is used to
* generate long results.  The same set cannot be used for both long
* and short because the rounding points are different.
*
* We can cheat and use the same decimal values for long to short and
* and extended to short because the result has the same number of
* bits and the rounding uses the same number of bits in the pre-
* rounded result.
*
***********************************************************************
          SPACE 2
*
* Long to short basic tests, which tests trappable results, NaN
* propagation, and basic functionality.  The second part of this list
* is used for testing trappable results.
*
LTOSIN   DS    0D        Inputs for long to short BFP basic tests
         DC    X'0000000000000000'         +0
         DC    X'3FF8000000000000'         +1.5
         DC    X'BFF8000000000000'         -1.5
         DC    X'7FF0100000000000'         SNaN
         DC    X'7FF8110000000000'         QNaN
* See rounding tests below for details on the following four
LTOSINOU DS    0D            start of under/overflow tests
         DC    X'47EFFFFFFFFFFFFF'    Positive oveflow test
         DC    X'C7EFFFFFFFFFFFFF'    Negative oveflow test
         DC    X'47FFFFFFEFFFFFFF'    Positive oveflow prec. test
         DC    X'C7FFFFFFEFFFFFFF'    Negative oveflow prec. test
         DC    X'3690000000000000'    Positive magnitude underflow
         DC    X'B690000000000000'    Negative magnitude underflow
         DC    X'47F0000000000000'    Positive magnitude overflow
         DC    X'C7F0000000000000'    Negative magnitude overflow
*
LTOSCT   EQU   *-LTOSIN     Count of long BFP in list * 8
LTOSOUCT EQU   *-LTOSINOU   Ct * 8 of trappable over/underflow tests
         SPACE 3
*
* Test cases for exhaustive rounding mode tests of long to short
* Load Rounded.
*
LTOSINRM DS    0D        Inputs for long to short BFP rounding tests
*              x'8000000000000000'    sign bit
*              x'7FF0000000000000'    Biased Exponent
*              x'000FFFFFE0000000'    Significand used in short
*              x'000000001FC00000'    Significand used in rounding
*              x'00000000003FFFFF'    'extra' significand bits
*   Note: in the comments below, 'up' and 'down' mean 'toward
*   higher magnitude' and 'toward lower magnitude' respectively and
*   without regard to the sign, and rounding is to short BFP.
*
* Exact (fits in short BFP)   ..  1.99999988079071044921875
         DC    X'3FFFFFFFE0000000'    Positive exact
         DC    X'BFFFFFFFE0000000'    Negative exact
*
* Tie odd - rounds up    ..  1.999999940395355224609375
* rounds up to                ..  2.0
* rounds down to              ..  1.99999988079071044921875
         DC    X'3FFFFFFFF0000000'    Positive tie odd
         DC    X'BFFFFFFFF0000000'    Negative tie odd
*
* Tie even - rounds down ..  1.999999821186065673828125
* rounds up to                ..  1.99999988079071044921875
* rounds down to              ..  1.9999997615814208984375
         DC    X'3FFFFFFFD0000000'    Positive tie even
         DC    X'BFFFFFFFD0000000'    Negative tie even
*
* False exact 1.9999998817220328017896235905936919152736663818359375
* ..rounds up to       2.0
* ..rounds down to     1.99999988079071044921875
         DC    X'3FFFFFFFE03FFFFF'    Positive false exact
         DC    X'BFFFFFFFE03FFFFF'    Negative false exact
*
* Nearest is towards zero: 1.9999998812563717365264892578125
* ..rounds up to           2.0
* ..rounds down to         1.99999988079071044921875
         DC    X'3FFFFFFFE0200000'    Positive zero closer
         DC    X'BFFFFFFFE0200000'    Negative zero closer
*
* Nearest is away from zero: 1.999999999068677425384521484375
* ..rounds up to             2.0
* ..rounds down to           1.99999988079071044921875
         DC    X'3FFFFFFFFFC00000'    Positive zero further
         DC    X'BFFFFFFFFFC00000'    Negative zero further
*
* Overflow test: 3.40282366920938425684442744474606501888E38
* ..rounds up to    Overflow
* ..rounds down to  3.40282346638528859811704183484516925440E38
         DC    X'47EFFFFFFFFFFFFF'    Positive oveflow test
         DC    X'C7EFFFFFFFFFFFFF'    Negative oveflow test
*
* Underflow test:  7.00649232162408535461864791644958...E-46
* ..rounds up to   1.40129846432481707092372958328991...E-45
*                   represented in short bfp as a tiny.
* ..rounds down to     underflow (but exact)
         DC    X'3690000000000000'    Positive magnitude underflow
         DC    X'B690000000000000'    Negative magnitude underflow
LTOSRMCT EQU   *-LTOSINRM   Count of long BFP rounding tests * 8
         SPACE 3
*
* Extended to short basic tests, which tests trappable results, NaN
* propagation, and basic functionality.  The second part of this list
* is used for testing trappable results.
*
XTOSIN   DS    0D        Inputs for extended to short BFP basic tests
         DC    X'00000000000000000000000000000000'         +0
         DC    X'3FFF8000000000000000000000000000'         +1.5
         DC    X'BFFF8000000000000000000000000000'         -1.5
         DC    X'7FFF0100000000000000000000000000'         SNaN
         DC    X'7FFF8110000000000000000000000000'         QNaN
* See rounding tests below for details on the following four
XTOSINOU DS    0D            start of over/underflow test cases
         DC    X'407EFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    Pos. oveflow test
         DC    X'C07EFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    Neg. oveflow test
         DC    X'407FFFFFFEFFFFFFFFFFFFFFFFFFFFFF'    Pos. oveflow test
         DC    X'C07FFFFFFEFFFFFFFFFFFFFFFFFFFFFF'    Neg. oveflow test
         DC    X'3F690000000000000000000000000000'    Pos. exact uflow
*                                                     ..result is tiny
         DC    X'BF690000000000000000000000000000'    Neg. exact uflow
*                                                     ..result is tiny
         DC    X'407F0000000000000000000000000000'    Pos. exact oflow
         DC    X'C07F0000000000000000000000000000'    Neg. exact oflow
*
XTOSCT   EQU   *-XTOSIN     Count of extended BFP in list * 16
XTOSOUCT EQU   *-XTOSINOU   Ct * 16 of trappable over/underflow tests
         SPACE 3
*
* Test cases for exhaustive rounding mode tests of extended to short
* Load Rounded.
*
XTOSINRM DS    0D     Inputs for extended to short BFP rounding tests
*              x'80000000000000000000000000000000'  sign bit
*              x'7FFF0000000000000000000000000000'  Biased Exponent
*              x'0000FFFFFE0000000000000000000000'  Sig'd used in short
*              x'0000000001FC00000000000000000000'  Sig'd used in rndg
*              x'000000000003FFFFFFFFFFFFFFFFFFFF'  'extra' sig'd bits
*   Note: in the comments below, 'up' and 'down' mean 'toward
*   higher magnitude' and 'toward lower magnitude' respectively and
*   without regard to the sign, and rounding is to short BFP.
*
* Exact (fits in short BFP)   ..  1.99999988079071044921875
         DC    X'3FFFFFFFFE0000000000000000000000'    Pos. exact
         DC    X'BFFFFFFFFE0000000000000000000000'    Neg. exact
*
* Tie odd - rounds up    ..  1.999999940395355224609375
* rounds up to                ..  2.0
* rounds down to              ..  1.99999988079071044921875
         DC    X'3FFFFFFFFF0000000000000000000000'    Pos. tie odd
         DC    X'BFFFFFFFFF0000000000000000000000'    Neg. tie odd
*
* Tie even - rounds down ..  1.999999821186065673828125
* rounds up to                ..  1.99999988079071044921875
* rounds down to              ..  1.9999997615814208984375
         DC    X'3FFFFFFFFD0000000000000000000000'    Pos. tie even
         DC    X'BFFFFFFFFD0000000000000000000000'    Neg. tie even
*
* False exact 1.9999998817220330238342285156249998... (continues)
*               ..07407005561276414694402205741507... (continues)
*               ..2681461898351784611804760061204433441162109375
* ..rounds up to       2.0
* ..rounds down to     1.99999988079071044921875
         DC    X'3FFFFFFFFE03FFFFFFFFFFFFFFFFFFFF'    Pos. false exact
         DC    X'BFFFFFFFFE03FFFFFFFFFFFFFFFFFFFF'    Neg. false exact
*
* Nearest is towards zero: 1.9999998812563717365264892578125
* ..rounds up to           2.0
* ..rounds down to         1.99999988079071044921875
         DC    X'3FFFFFFFFE0200000000000000000000'    Pos. zero closer
         DC    X'BFFFFFFFFE0200000000000000000000'    Neg. zero closer
*
* Nearest is away from zero: 1.999999999068677425384521484375
* ..rounds up to             2.0
* ..rounds down to           1.99999988079071044921875
         DC    X'3FFFFFFFFFFC00000000000000000000'    Pos. zero further
         DC    X'BFFFFFFFFFFC00000000000000000000'    Neg. zero further
*
* Overflow test:    3.40282366920938463463374607431768178688E38
* ..rounds up to    Overflow
* ..rounds down to  3.40282346638528859811704183484516925440E38
         DC    X'407EFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    Pos. oveflow test
         DC    X'C07EFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    Neg. oveflow test
*
* Underflow test:   7.00649232162408535461864791644958...E-46
* ..rounds up to    1.40129846432481707092372958328991...E-45
*                   represented in short bfp as a tiny.
* ..rounds down to  Underflow (but exact)
         DC    X'3F690000000000000000000000000000'    Pos. exact u-flow
*                                                     ..result is tiny
         DC    X'BF690000000000000000000000000000'    Neg. exact u-flow
*                                                     ..result is tiny
XTOSRMCT EQU   *-XTOSINRM   Count of extended BFP rounding tests * 16
*
* Extended to long basic tests, which tests trappable results, NaN
* propagation, and basic functionality.  The second part of this list
* is used for testing trappable results.
*
XTOLIN   DS    0D        Inputs for extended to long BFP basic tests
         DC    X'00000000000000000000000000000000'         +0
         DC    X'3FFF8000000000000000000000000000'         +1.5
         DC    X'BFFF8000000000000000000000000000'         -1.5
         DC    X'7FFF0100000000000000000000000000'         SNaN
         DC    X'7FFF8110000000000000000000000000'         QNaN
* See rounding tests below for details on the following four
XTOLINOU DS    0D       Start of trappable over/underflow test cases
         DC    X'43FEFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    Pos. oveflow test
         DC    X'C3FEFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    Neg. oveflow test
         DC    X'43FFFFFFFFFFFFFFF7FFFFFFFFFFFFFF'    Pos. oveflow test
         DC    X'C3FFFFFFFFFFFFFFF7FFFFFFFFFFFFFF'    Neg. oveflow test
         DC    X'3BCC0000000000000000000000000000'    Pos. underflow
*                                                     ..result is tiny
         DC    X'BBCC0000000000000000000000000000'    Neg. underflow
*                                                     ..result is tiny
         DC    X'43FF0000000000000000000000000000'    Pos. exact o-flow
         DC    X'C3FF0000000000000000000000000000'    Neg. exact o-flow
XTOLCT   EQU   *-XTOLIN     Count of extended BFP in list * 16
XTOLOUCT EQU   *-XTOLINOU   Ct * 16 of trappable over/underflow tests
*
* Test cases for exhaustive rounding mode tests of long to short
* Load Rounded.
*
XTOLINRM DS    0D     Inputs for extended to short BFP rounding tests
*              x'80000000000000000000000000000000'  sign bit
*              x'7FFF0000000000000000000000000000'  Biased Exponent
*              x'0000FFFFFFFFFFFFF000000000000000'  Sig'd used in long
*              x'00000000000000000FFC000000000000'  Sig'd used in rndg
*              x'00000000000000000003FFFFFFFFFFFF'  'extra' sig'd bits
*
*   Note: in the comments below, 'up' and 'down' mean 'toward
*   higher magnitude' and 'toward lower magnitude' respectively and
*   without regard to the sign, and rounding is to short BFP.
*
*
* Exact (fits in short BFP)
* ..  1.9999999999999997779553950749686919152736663818359375
*
         DC    X'3FFFFFFFFFFFFFFFF000000000000000'    Pos. exact
         DC    X'BFFFFFFFFFFFFFFFF000000000000000'    Neg. exact
*
*
* Tie odd - rounds up
* ..  1.99999999999999988897769753748434595763683319091796875
* rounds up to                ..  2.0
* rounds down to
* ..  1.9999999999999997779553950749686919152736663818359375
*
         DC    X'3FFFFFFFFFFFFFFFF800000000000000'    Pos. tie odd
         DC    X'BFFFFFFFFFFFFFFFF800000000000000'    Neg. tie odd
*
*
* Tie even - rounds down
* ..  1.99999999999999966693309261245303787291049957275390625
* rounds up to
* ..  1.9999999999999997779553950749686919152736663818359375
* rounds down to
* ..  1.999999999999999555910790149937383830547332763671875
*
         DC    X'3FFFFFFFFFFFFFFFE800000000000000'    Pos. tie even
         DC    X'BFFFFFFFFFFFFFFFE800000000000000'    Neg. tie even
*
*
* False exact 1.9999998817220330238342285156249998... (continues)
*               ..07407005561276414694402205741507... (continues)
*               ..2681461898351784611804760061204433441162109375
* ..rounds up to       2.0
* ..rounds down to
* ..  1.9999999999999997779553950749686919152736663818359375
*
         DC    X'3FFFFFFFFFFFFFFFF003FFFFFFFFFFFF'    Pos. false exact
         DC    X'BFFFFFFFFFFFFFFFF003FFFFFFFFFFFF'    Neg. false exact
*
*
* Nearest is towards zero:
* ..  1.99999999999999977817223550946579280207515694200992584228515625
* ..rounds up to           2.0
* ..rounds down to
* ..  1.9999999999999997779553950749686919152736663818359375
*
         DC    X'3FFFFFFFFFFFFFFFF004000000000000'    Pos. zero closer
         DC    X'BFFFFFFFFFFFFFFFF004000000000000'    Neg. zero closer
*
* Nearest is away from zero:
* ..  1.9999999999999999722444243843710864894092082977294921875
* ..rounds up to             2.0
* ..rounds down to
* ..  1.999999999999999555910790149937383830547332763671875
*
         DC    X'3FFFFFFFFFFFFFFFFE00000000000000'    Pos. zero further
         DC    X'BFFFFFFFFFFFFFFFFE00000000000000'    Neg. zero further
         DS    0D           required by asma for following EQU to work.
*
* Overflow test:    1.797693134862315708145274237317043...E308
* ..rounds up to    Overflow
* ..rounds down to  1.797693134862315907729305190789024...E308
         DC    X'43FEFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    Pos. oveflow test
         DC    X'C3FEFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    Neg. oveflow test
*
* Underflow test:   2.47032822920623272088284396434110...E-324
* ..rounds up to    4.94065645841246544176568792868221...E-324
* ..rounds down to  Underflow
         DC    X'3BCC0000000000000000000000000000'    Pos. tie odd
*                                                     ..result is tiny
         DC    X'BBCC0000000000000000000000000000'    Neg. tie odd
*                                                     ..result is tiny
XTOLRMCT EQU   *-XTOLINRM   Count of extended BFP rounding tests * 16
         EJECT
***********************************************************************
*                 ACTUAL results saved here
***********************************************************************
*
*               Locations for ACTUAL results
*
*
LTOSOUT  EQU   BFPLDRND+X'1000'    Short BFP rounded from long
*                                  ..9 pairs used, room for 16
LTOSFLGS EQU   BFPLDRND+X'1080'    FPCR flags and DXC from above
*                                  ..9 pairs used, room for 16
LTOSRMO  EQU   BFPLDRND+X'1100'    Short BFP result rounding tests
*                                  ..14 sets used, room for 21
LTOSRMOF EQU   BFPLDRND+X'1500'    FPCR flags and DXC from above
*                                  ..14 sets used, room for 21
*
XTOSOUT  EQU   BFPLDRND+X'1900'    Short BFP rounded from extended
*                                  ..5 pairs used, room for 16
XTOSFLGS EQU   BFPLDRND+X'1980'    FPCR flags and DXC from above
*                                  ..5 pairs used, room for 16
XTOSRMO  EQU   BFPLDRND+X'1A00'    Short BFP rounding tests
*                                  ..14 sets used, room for 21
XTOSRMOF EQU   BFPLDRND+X'1E00'    FPCR flags and DXC from above
*                                  ..14 sets used, room for 21

*
XTOLOUT  EQU   BFPLDRND+X'2200'    Long BFP rounded from extended
*                                  ..5 pairs used, room for 16
XTOLFLGS EQU   BFPLDRND+X'2300'    FPCR flags and DXC from above
*                                  ..5 pairs used, room for 32
XTOLRMO  EQU   BFPLDRND+X'2400'    Long BFP rounding tests
*                                  ..12 results used, room for 22
XTOLRMOF EQU   BFPLDRND+X'2B00'    FPCR flags and DXC from above
*                                  ..12 results used, room for 21
*
LTOSOUO  EQU   BFPLDRND+X'3000'    Long BFP trappable o/uflow tests
*                                  ..4 results used, room for 16
LTOSOUOF EQU   BFPLDRND+X'3080'    FPCR flags and DXC from above
*
XTOSOUO  EQU   BFPLDRND+X'3100'    Extd BFP trappable o/uflow tests
*                                  ..4 results used, room for 8
XTOSOUOF EQU   BFPLDRND+X'3180'    FPCR flags and DXC from above
*
XTOLOUO  EQU   BFPLDRND+X'3200'    Extd BFP trappable o/uflow tests
*                                  ..4 results used, room for 8
XTOLOUOF EQU   BFPLDRND+X'3280'    FPCR flags and DXC from above
         EJECT
***********************************************************************
*                    EXPECTED results
***********************************************************************
*
         ORG   BFPLDRND+X'4000'   (past end of actual results)
*
LTOSOUT_GOOD EQU *
 DC CL48'LEDBR result pairs 1-2'
 DC XL16'00000000000000003FC000003FC00000'
 DC CL48'LEDBR result pairs 3-4'
 DC XL16'BFC00000BFC000007FC0800000000000'
 DC CL48'LEDBR result pairs 5-6'
 DC XL16'7FC088007FC088007F80000027F00000'
 DC CL48'LEDBR result pairs 7-8'
 DC XL16'FF800000A7F000007F80000027FFFFFF'
 DC CL48'LEDBR result pairs 9-10'
 DC XL16'FF800000A7FFFFFF0000000056900000'
 DC CL48'LEDBR result pairs 11-12'
 DC XL16'80000000D69000007F80000027F00000'
 DC CL48'LEDBR result pair 13'
 DC XL16'FF800000A7F000000000000000000000'
LTOSOUT_NUM EQU (*-LTOSOUT_GOOD)/64
*
*
LTOSFLGS_GOOD EQU *
 DC CL48'LEDBR FPCR pairs 1-2'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'LEDBR FPCR pairs 3-4'
 DC XL16'00000000F800000000800000F8008000'
 DC CL48'LEDBR FPCR pairs 5-6'
 DC XL16'00000000F800000000280000F8002C00'
 DC CL48'LEDBR FPCR pairs 7-8'
 DC XL16'00280000F8002C0000280000F8002800'
 DC CL48'LEDBR FPCR pairs 9-10'
 DC XL16'00280000F800280000180000F8001000'
 DC CL48'LEXBR FPCR pairs 11-12'
 DC XL16'00180000F800100000280000F8002000'
 DC CL48'LEXBR FPCR pair 13'
 DC XL16'00280000F80020000000000000000000'
LTOSFLGS_NUM EQU (*-LTOSFLGS_GOOD)/64
*
*
LTOSRMO_GOOD EQU *
 DC CL48'LEDBRA +exact FPCR modes 1-3, 7'
 DC XL16'3FFFFFFF3FFFFFFF3FFFFFFF3FFFFFFF'
 DC CL48'LEDBRA +exact M3 modes 1, 3-5'
 DC XL16'3FFFFFFF3FFFFFFF3FFFFFFF3FFFFFFF'
 DC CL48'LEDBRA +exact M3 modes 6, 7'
 DC XL16'3FFFFFFF3FFFFFFF0000000000000000'
 DC CL48'LEDBRA -exact FPCR modes 1-3, 7'
 DC XL16'BFFFFFFFBFFFFFFFBFFFFFFFBFFFFFFF'
 DC CL48'LEDBRA -exact M3 modes 1, 3-5'
 DC XL16'BFFFFFFFBFFFFFFFBFFFFFFFBFFFFFFF'
 DC CL48'LEDBRA -exact M3 modes 6, 7'
 DC XL16'BFFFFFFFBFFFFFFF0000000000000000'
 DC CL48'LEDBRA +tie odd FPCR modes 1-3, 7'
 DC XL16'3FFFFFFF400000003FFFFFFF3FFFFFFF'
 DC CL48'LEDBRA +tie odd M3 modes 1, 3-5'
 DC XL16'400000003FFFFFFF400000003FFFFFFF'
 DC CL48'LEDBRA +tie odd M3 modes 6, 7'
 DC XL16'400000003FFFFFFF0000000000000000'
 DC CL48'LEDBRA -tie odd FPCR modes 1-3, 7'
 DC XL16'BFFFFFFFBFFFFFFFC0000000BFFFFFFF'
 DC CL48'LEDBRA -tie odd M3 modes 1, 3-5'
 DC XL16'C0000000BFFFFFFFC0000000BFFFFFFF'
 DC CL48'LEDBRA -tie odd M3 modes 6, 7'
 DC XL16'BFFFFFFFC00000000000000000000000'
 DC CL48'LEDBRA +tie even FPCR modes 1-3, 7'
 DC XL16'3FFFFFFE3FFFFFFF3FFFFFFE3FFFFFFF'
 DC CL48'LEDBRA +tie even M3 modes 1, 3-5'
 DC XL16'3FFFFFFF3FFFFFFF3FFFFFFE3FFFFFFE'
 DC CL48'LEDBRA +tie even M3 modes 6, 7'
 DC XL16'3FFFFFFF3FFFFFFE0000000000000000'
 DC CL48'LEDBRA -tie even FPCR modes 1-3, 7'
 DC XL16'BFFFFFFEBFFFFFFEBFFFFFFFBFFFFFFF'
 DC CL48'LEDBRA -tie even M3 modes 1, 3-5'
 DC XL16'BFFFFFFFBFFFFFFFBFFFFFFEBFFFFFFE'
 DC CL48'LEDBRA -tie even M3 modes 6, 7'
 DC XL16'BFFFFFFEBFFFFFFF0000000000000000'
 DC CL48'LEDBRA +false exact FPCR modes 1-3, 7'
 DC XL16'3FFFFFFF400000003FFFFFFF3FFFFFFF'
 DC CL48'LEDBRA +false exact M3 modes 1, 3-5'
 DC XL16'3FFFFFFF3FFFFFFF3FFFFFFF3FFFFFFF'
 DC CL48'LEDBRA +false exact M3 modes 6, 7'
 DC XL16'400000003FFFFFFF0000000000000000'
 DC CL48'LEDBRA -false exact FPCR modes 1-3, 7'
 DC XL16'BFFFFFFFBFFFFFFFC0000000BFFFFFFF'
 DC CL48'LEDBRA -false exact M3 modes 1, 3-5'
 DC XL16'BFFFFFFFBFFFFFFFBFFFFFFFBFFFFFFF'
 DC CL48'LEDBRA -false exact M3 modes 6, 7'
 DC XL16'BFFFFFFFC00000000000000000000000'
 DC CL48'LEDBRA +near zero FPCR modes 1-3, 7'
 DC XL16'3FFFFFFF400000003FFFFFFF3FFFFFFF'
 DC CL48'LEDBRA +near zero M3 modes 1, 3-5'
 DC XL16'3FFFFFFF3FFFFFFF3FFFFFFF3FFFFFFF'
 DC CL48'LEDBRA +near zero M3 modes 6, 7'
 DC XL16'400000003FFFFFFF0000000000000000'
 DC CL48'LEDBRA -near zero FPCR modes 1-3, 7'
 DC XL16'BFFFFFFFBFFFFFFFC0000000BFFFFFFF'
 DC CL48'LEDBRA -near zero M3 modes 1, 3-5'
 DC XL16'BFFFFFFFBFFFFFFFBFFFFFFFBFFFFFFF'
 DC CL48'LEDBRA -near zero M3 modes 6, 7'
 DC XL16'BFFFFFFFC00000000000000000000000'
 DC CL48'LEDBRA +near +inf FPCR modes 1-3, 7'
 DC XL16'3FFFFFFF400000003FFFFFFF3FFFFFFF'
 DC CL48'LEDBRA +near +inf M3 modes 1, 3-5'
 DC XL16'400000003FFFFFFF400000003FFFFFFF'
 DC CL48'LEDBRA +near +inf M3 modes 6, 7'
 DC XL16'400000003FFFFFFF0000000000000000'
 DC CL48'LEDBRA -near -inf FPCR modes 1-3, 7'
 DC XL16'BFFFFFFFBFFFFFFFC0000000BFFFFFFF'
 DC CL48'LEDBRA -near -inf M3 modes 1, 3-5'
 DC XL16'C0000000BFFFFFFFC0000000BFFFFFFF'
 DC CL48'LEDBRA -near -inf M3 modes 6, 7'
 DC XL16'BFFFFFFFC00000000000000000000000'
 DC CL48'LEDBRA +overflow FPCR modes 1-3, 7'
 DC XL16'7F7FFFFF7F8000007F7FFFFF7F7FFFFF'
 DC CL48'LEDBRA +overflow M3 modes 1, 3-5'
 DC XL16'7F8000007F7FFFFF7F8000007F7FFFFF'
 DC CL48'LEDBRA +overflow M3 modes 6, 7'
 DC XL16'7F8000007F7FFFFF0000000000000000'
 DC CL48'LEDBRA -overflow FPCR modes 1-3, 7'
 DC XL16'FF7FFFFFFF7FFFFFFF800000FF7FFFFF'
 DC CL48'LEDBRA -overflow M3 modes 1, 3-5'
 DC XL16'FF800000FF7FFFFFFF800000FF7FFFFF'
 DC CL48'LEDBRA -overflow M3 modes 6, 7'
 DC XL16'FF7FFFFFFF8000000000000000000000'
 DC CL48'LEDBRA +tiny tie odd FPCR modes 1-3, 7'
 DC XL16'00000000000000010000000000000001'
 DC CL48'LEDBRA +tiny tie odd M3 modes 1, 3-5'
 DC XL16'00000001000000010000000000000000'
 DC CL48'LEDBRA +tiny tie odd M3 modes 6, 7'
 DC XL16'00000001000000000000000000000000'
 DC CL48'LEDBRA -tiny tie odd FPCR modes 1-3, 7'
 DC XL16'80000000800000008000000180000001'
 DC CL48'LEDBRA -tiny tie odd M3 modes 1, 3-5'
 DC XL16'80000001800000018000000080000000'
 DC CL48'LEDBRA -tiny tie odd M3 modes 6, 7'
 DC XL16'80000000800000010000000000000000'
LTOSRMO_NUM EQU (*-LTOSRMO_GOOD)/64
*
*
LTOSRMOF_GOOD EQU *
 DC CL48'LEDBRA +exact FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEDBRA +exact M3 modes 1, 3-5 FPCR'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LEDBRA +exact M3 modes 6, 7 FCPR'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LEDBRA -exact FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEDBRA -exact M3 modes 1, 3-5 FPCR'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LEDBRA -exact M3 modes 6, 7 FCPR'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LEDBRA +tie odd FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEDBRA +tie odd M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEDBRA +tie odd M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEDBRA -tie odd FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEDBRA -tie odd M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEDBRA -tie odd M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEDBRA +tie even FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEDBRA +tie even M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEDBRA +tie even M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEDBRA -tie even FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEDBRA -tie even M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEDBRA -tie even M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEDBRA +false exact FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEDBRA +false exact M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEDBRA +false exact M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEDBRA -false exact FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEDBRA -false exact M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEDBRA -false exact M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEDBRA +near zero FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEDBRA +near zero M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEDBRA +near zero M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEDBRA -near zero FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEDBRA -near zero M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEDBRA -near zero M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEDBRA +near +inf FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEDBRA +near +inf M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEDBRA +near +inf M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEDBRA -near -inf FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEDBRA -near -inf M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEDBRA -near -inf M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEDBRA +overflow FPCR modes 1-3, 7 FPCR'
 DC XL16'00000001002000020000000300000007'
 DC CL48'LEDBRA +overflow M3 modes 1, 3-5 FPCR'
 DC XL16'00280000000800000028000000080000'
 DC CL48'LEDBRA +overflow M3 modes 6, 7 FPCR'
 DC XL16'00280000000800000000000000000000'
 DC CL48'LEDBRA -overflow FPCR modes 1-3, 7 FPCR'
 DC XL16'00000001000000020020000300000007'
 DC CL48'LEDBRA -overflow M3 modes 1, 3-5 FPCR'
 DC XL16'00280000000800000028000000080000'
 DC CL48'LEDBRA -overflow M3 modes 6, 7 FPCR'
 DC XL16'00080000002800000000000000000000'
 DC CL48'LEDBRA +tiny tie odd FPCR modes 1-3, 7 FPCR'
 DC XL16'00100001001000020010000300100007'
 DC CL48'LEDBRA +tiny tie odd M3 modes 1, 3-5 FPCR'
 DC XL16'00180000001800000018000000180000'
 DC CL48'LEDBRA +tiny tie odd M3 modes 6, 7 FPCR'
 DC XL16'00180000001800000000000000000000'
 DC CL48'LEDBRA -tiny tie odd FPCR modes 1-3, 7 FPCR'
 DC XL16'00100001001000020010000300100007'
 DC CL48'LEDBRA -tiny tie odd M3 modes 1, 3-5 FPCR'
 DC XL16'00180000001800000018000000180000'
 DC CL48'LEDBRA -tiny tie odd M3 modes 6, 7 FPCR'
 DC XL16'00180000001800000000000000000000'
LTOSRMOF_NUM EQU (*-LTOSRMOF_GOOD)/64
*
*
XTOSOUT_GOOD EQU *
 DC CL48'LEXBR result pairs 1-2'
 DC XL16'00000000000000003FC000003FC00000'
 DC CL48'LEXBR result pairs 3-4'
 DC XL16'BFC00000BFC000007FC0800000000000'
 DC CL48'LEXBR result pairs 5-6'
 DC XL16'7FC088007FC088007F800000207F0000'
 DC CL48'LEXBR result pairs 7-8'
 DC XL16'FF800000A07F00007F800000207FFFFF'
 DC CL48'LEXBR result pairs 9-10'
 DC XL16'FF800000A07FFFFF000000005F690000'
 DC CL48'LEXBR result pair 11-12'
 DC XL16'80000000DF6900007F800000207F0000'
 DC CL48'LEXBR result pair 13'
 DC XL16'FF800000A07F00000000000000000000'
XTOSOUT_NUM EQU (*-XTOSOUT_GOOD)/64
*
*
XTOSFLGS_GOOD EQU *
 DC CL48'LEXBR FPCR pairs 1-2'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'LEXBR FPCR pairs 3-4'
 DC XL16'00000000F800000000800000F8008000'
 DC CL48'LEXBR FPCR pairs 5-6'
 DC XL16'00000000F800000000280000F8002C00'
 DC CL48'LEXBR FPCR pairs 7-8'
 DC XL16'00280000F8002C0000280000F8002800'
 DC CL48'LEXBR FPCR pairs 9-10'
 DC XL16'00280000F800280000180000F8001000'
 DC CL48'LEXBR FPCR pairs 11-12'
 DC XL16'00180000F800100000280000F8002000'
 DC CL48'LEXBR FPCR pair 13'
 DC XL16'00280000F80020000000000000000000'
XTOSFLGS_NUM EQU (*-XTOSFLGS_GOOD)/64
*
*
XTOSRMO_GOOD EQU *
 DC CL48'LEXBRA +exact FPCR modes 1-3, 7'
 DC XL16'3FFFFFFF3FFFFFFF3FFFFFFF3FFFFFFF'
 DC CL48'LEXBRA +exact M3 modes 1, 3-5'
 DC XL16'3FFFFFFF3FFFFFFF3FFFFFFF3FFFFFFF'
 DC CL48'LEXBRA +exact M3 modes 6, 7'
 DC XL16'3FFFFFFF3FFFFFFF0000000000000000'
 DC CL48'LEXBRA -exact FPCR modes 1-3, 7'
 DC XL16'BFFFFFFFBFFFFFFFBFFFFFFFBFFFFFFF'
 DC CL48'LEXBRA -exact M3 modes 1, 3-5'
 DC XL16'BFFFFFFFBFFFFFFFBFFFFFFFBFFFFFFF'
 DC CL48'LEXBRA -exact M3 modes 6, 7'
 DC XL16'BFFFFFFFBFFFFFFF0000000000000000'
 DC CL48'LEXBRA +tie odd FPCR modes 1-3, 7'
 DC XL16'3FFFFFFF400000003FFFFFFF3FFFFFFF'
 DC CL48'LEXBRA +tie odd M3 modes 1, 3-5'
 DC XL16'400000003FFFFFFF400000003FFFFFFF'
 DC CL48'LEXBRA +tie odd M3 modes 6, 7'
 DC XL16'400000003FFFFFFF0000000000000000'
 DC CL48'LEXBRA -tie odd FPCR modes 1-3, 7'
 DC XL16'BFFFFFFFBFFFFFFFC0000000BFFFFFFF'
 DC CL48'LEXBRA -tie odd M3 modes 1, 3-5'
 DC XL16'C0000000BFFFFFFFC0000000BFFFFFFF'
 DC CL48'LEXBRA -tie odd M3 modes 6, 7'
 DC XL16'BFFFFFFFC00000000000000000000000'
 DC CL48'LEXBRA +tie even FPCR modes 1-3, 7'
 DC XL16'3FFFFFFE3FFFFFFF3FFFFFFE3FFFFFFF'
 DC CL48'LEXBRA +tie even M3 modes 1, 3-5'
 DC XL16'3FFFFFFF3FFFFFFF3FFFFFFE3FFFFFFE'
 DC CL48'LEXBRA +tie even M3 modes 6, 7'
 DC XL16'3FFFFFFF3FFFFFFE0000000000000000'
 DC CL48'LEXBRA -tie even FPCR modes 1-3, 7'
 DC XL16'BFFFFFFEBFFFFFFEBFFFFFFFBFFFFFFF'
 DC CL48'LEXBRA -tie even M3 modes 1, 3-5'
 DC XL16'BFFFFFFFBFFFFFFFBFFFFFFEBFFFFFFE'
 DC CL48'LEXBRA -tie even M3 modes 6, 7'
 DC XL16'BFFFFFFEBFFFFFFF0000000000000000'
 DC CL48'LEXBRA +false exact FPCR modes 1-3, 7'
 DC XL16'3FFFFFFF400000003FFFFFFF3FFFFFFF'
 DC CL48'LEXBRA +false exact M3 modes 1, 3-5'
 DC XL16'3FFFFFFF3FFFFFFF3FFFFFFF3FFFFFFF'
 DC CL48'LEXBRA +false exact M3 modes 6, 7'
 DC XL16'400000003FFFFFFF0000000000000000'
 DC CL48'LEXBRA -false exact FPCR modes 1-3, 7'
 DC XL16'BFFFFFFFBFFFFFFFC0000000BFFFFFFF'
 DC CL48'LEXBRA -false exact M3 modes 1, 3-5'
 DC XL16'BFFFFFFFBFFFFFFFBFFFFFFFBFFFFFFF'
 DC CL48'LEXBRA -false exact M3 modes 6, 7'
 DC XL16'BFFFFFFFC00000000000000000000000'
 DC CL48'LEXBRA +near zero FPCR modes 1-3, 7'
 DC XL16'3FFFFFFF400000003FFFFFFF3FFFFFFF'
 DC CL48'LEXBRA +near zero M3 modes 1, 3-5'
 DC XL16'3FFFFFFF3FFFFFFF3FFFFFFF3FFFFFFF'
 DC CL48'LEXBRA +near zero M3 modes 6, 7'
 DC XL16'400000003FFFFFFF0000000000000000'
 DC CL48'LEXBRA -near zero FPCR modes 1-3, 7'
 DC XL16'BFFFFFFFBFFFFFFFC0000000BFFFFFFF'
 DC CL48'LEXBRA -near zero M3 modes 1, 3-5'
 DC XL16'BFFFFFFFBFFFFFFFBFFFFFFFBFFFFFFF'
 DC CL48'LEXBRA -near zero M3 modes 6, 7'
 DC XL16'BFFFFFFFC00000000000000000000000'
 DC CL48'LEXBRA +near +inf FPCR modes 1-3, 7'
 DC XL16'3FFFFFFF400000003FFFFFFF3FFFFFFF'
 DC CL48'LEXBRA +near +inf M3 modes 1, 3-5'
 DC XL16'400000003FFFFFFF400000003FFFFFFF'
 DC CL48'LEXBRA +near +inf M3 modes 6, 7'
 DC XL16'400000003FFFFFFF0000000000000000'
 DC CL48'LEXBRA -near -inf FPCR modes 1-3, 7'
 DC XL16'BFFFFFFFBFFFFFFFC0000000BFFFFFFF'
 DC CL48'LEXBRA -near -inf M3 modes 1, 3-5'
 DC XL16'C0000000BFFFFFFFC0000000BFFFFFFF'
 DC CL48'LEXBRA -near -inf M3 modes 6, 7'
 DC XL16'BFFFFFFFC00000000000000000000000'
 DC CL48'LEXBRA +overflow FPCR modes 1-3, 7'
 DC XL16'7F7FFFFF7F8000007F7FFFFF7F7FFFFF'
 DC CL48'LEXBRA +overflow M3 modes 1, 3-5'
 DC XL16'7F8000007F7FFFFF7F8000007F7FFFFF'
 DC CL48'LEXBRA +overflow M3 modes 6, 7'
 DC XL16'7F8000007F7FFFFF0000000000000000'
 DC CL48'LEXBRA -overflow FPCR modes 1-3, 7'
 DC XL16'FF7FFFFFFF7FFFFFFF800000FF7FFFFF'
 DC CL48'LEXBRA -overflow M3 modes 1, 3-5'
 DC XL16'FF800000FF7FFFFFFF800000FF7FFFFF'
 DC CL48'LEXBRA -overflow M3 modes 6, 7'
 DC XL16'FF7FFFFFFF8000000000000000000000'
 DC CL48'LEXBRA +tiny tie odd FPCR modes 1-3, 7'
 DC XL16'00000000000000010000000000000001'
 DC CL48'LEXBRA +tiny tie odd M3 modes 1, 3-5'
 DC XL16'00000001000000010000000000000000'
 DC CL48'LEXBRA +tiny tie odd M3 modes 6, 7'
 DC XL16'00000001000000000000000000000000'
 DC CL48'LEXBRA -tiny tie odd FPCR modes 1-3, 7'
 DC XL16'80000000800000008000000180000001'
 DC CL48'LEXBRA -tiny tie odd M3 modes 1, 3-5'
 DC XL16'80000001800000018000000080000000'
 DC CL48'LEXBRA -tiny tie odd M3 modes 6, 7'
 DC XL16'80000000800000010000000000000000'
XTOSRMO_NUM EQU (*-XTOSRMO_GOOD)/64
*
*
XTOSRMOF_GOOD EQU *
 DC CL48'LEXBRA +exact FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEXBRA +exact M3 modes 1, 3-5 FPCR'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LEXBRA +exact M3 modes 6, 7 FCPR'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LEXBRA -exact FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEXBRA -exact M3 modes 1, 3-5 FPCR'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LEXBRA -exact M3 modes 6, 7 FCPR'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LEXBRA +tie odd FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEXBRA +tie odd M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEXBRA +tie odd M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEXBRA -tie odd FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEXBRA -tie odd M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEXBRA -tie odd M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEXBRA +tie even FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEXBRA +tie even M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEXBRA +tie even M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEXBRA -tie even FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEXBRA -tie even M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEXBRA -tie even M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEXBRA +false exact FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEXBRA +false exact M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEXBRA +false exact M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEXBRA -false exact FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEXBRA -false exact M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEXBRA -false exact M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEXBRA +near zero FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEXBRA +near zero M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEXBRA +near zero M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEXBRA -near zero FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEXBRA -near zero M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEXBRA -near zero M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEXBRA +near +inf FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEXBRA +near +inf M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEXBRA +near +inf M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEXBRA -near -inf FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LEXBRA -near -inf M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LEXBRA -near -inf M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LEXBRA +overflow FPCR modes 1-3, 7 FPCR'
 DC XL16'00000001002000020000000300000007'
 DC CL48'LEXBRA +overflow M3 modes 1, 3-5 FPCR'
 DC XL16'00280000000800000028000000080000'
 DC CL48'LEXBRA +overflow M3 modes 6, 7 FPCR'
 DC XL16'00280000000800000000000000000000'
 DC CL48'LEXBRA -overflow FPCR modes 1-3, 7 FPCR'
 DC XL16'00000001000000020020000300000007'
 DC CL48'LEXBRA -overflow M3 modes 1, 3-5 FPCR'
 DC XL16'00280000000800000028000000080000'
 DC CL48'LEXBRA -overflow M3 modes 6, 7 FPCR'
 DC XL16'00080000002800000000000000000000'
 DC CL48'LEXBRA +tiny tie odd FPCR modes 1-3, 7 FPCR'
 DC XL16'00100001001000020010000300100007'
 DC CL48'LEXBRA +tiny tie odd M3 modes 1, 3-5 FPCR'
 DC XL16'00180000001800000018000000180000'
 DC CL48'LEXBRA +tiny tie odd M3 modes 6, 7 FPCR'
 DC XL16'00180000001800000000000000000000'
 DC CL48'LEXBRA -tiny tie odd FPCR modes 1-3, 7 FPCR'
 DC XL16'00100001001000020010000300100007'
 DC CL48'LEXBRA -tiny tie odd M3 modes 1, 3-5 FPCR'
 DC XL16'00180000001800000018000000180000'
 DC CL48'LEXBRA -tiny tie odd M3 modes 6, 7 FPCR'
 DC XL16'00180000001800000000000000000000'
XTOSRMOF_NUM EQU (*-XTOSRMOF_GOOD)/64
*
*
XTOLOUT_GOOD EQU *
 DC CL48'LDXBR result pair 1'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LDXBR result pair 2'
 DC XL16'3FF80000000000003FF8000000000000'
 DC CL48'LDXBR result pair 3'
 DC XL16'BFF8000000000000BFF8000000000000'
 DC CL48'LDXBR result pair 4'
 DC XL16'7FF81000000000000000000000000000'
 DC CL48'LDXBR result pair 5'
 DC XL16'7FF81100000000007FF8110000000000'
 DC CL48'LDXBR result pair 6'
 DC XL16'7FF000000000000023FF000000000000'
 DC CL48'LDXBR result pair 7'
 DC XL16'FFF0000000000000A3FF000000000000'
 DC CL48'LDXBR result pair 8'
 DC XL16'7FF000000000000023FFFFFFFFFFFFFF'
 DC CL48'LDXBR result pair 9'
 DC XL16'FFF0000000000000A3FFFFFFFFFFFFFF'
 DC CL48'LDXBR result pair 10'
 DC XL16'00000000000000005BCC000000000000'
 DC CL48'LDXBR result pair 11'
 DC XL16'8000000000000000DBCC000000000000'
 DC CL48'LDXBR result pair 12'
 DC XL16'7FF000000000000023FF000000000000'
 DC CL48'LDXBR result pair 13'
 DC XL16'FFF0000000000000A3FF000000000000'
XTOLOUT_NUM EQU (*-XTOLOUT_GOOD)/64
*
*
XTOLFLGS_GOOD EQU *
 DC CL48'LDXBR FPCR pairs 1-2'
 DC XL16'00000000F800000000000000F8000000'
 DC CL48'LDXBR FPCR pairs 3-4'
 DC XL16'00000000F800000000800000F8008000'
 DC CL48'LDXBR FPCR pairs 5-6'
 DC XL16'00000000F800000000280000F8002C00'
 DC CL48'LDXBR FPCR pairs 7-8'
 DC XL16'00280000F8002C0000280000F8002800'
 DC CL48'LDXBR FPCR pairs 9-10'
 DC XL16'00280000F800280000180000F8001000'
 DC CL48'LDXBR FPCR pairs 11-12'
 DC XL16'00180000F800100000280000F8002000'
 DC CL48'LDXBR FPCR pair 13'
 DC XL16'00280000F80020000000000000000000'
XTOLFLGS_NUM EQU (*-XTOLFLGS_GOOD)/64
*
*
XTOLRMO_GOOD EQU *
 DC CL48'LDXBRA +exact FPC modes 1, 2'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +exact FPC modes 3, 7'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +exact M3 modes 1, 3'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +exact M3 modes 4, 5'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +exact M3 modes 6, 7'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -exact FPC modes 1, 2'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -exact FPC modes 3, 7'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -exact M3 modes 1, 3'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -exact M3 modes 4, 5'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -exact M3 modes 6, 7'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +tie odd FPC modes 1, 2'
 DC XL16'3FFFFFFFFFFFFFFF4000000000000000'
 DC CL48'LDXBRA +tie odd FPC modes 3, 7'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +tie odd M3 modes 1, 3'
 DC XL16'40000000000000003FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +tie odd M3 modes 4, 5'
 DC XL16'40000000000000003FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +tie odd M3 modes 6, 7'
 DC XL16'40000000000000003FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -tie odd FPC modes 1, 2'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -tie odd FPC modes 3, 7'
 DC XL16'C000000000000000BFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -tie odd M3 modes 1, 3'
 DC XL16'C000000000000000BFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -tie odd M3 modes 4, 5'
 DC XL16'C000000000000000BFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -tie odd M3 modes 6, 7'
 DC XL16'BFFFFFFFFFFFFFFFC000000000000000'
 DC CL48'LDXBRA +tie even FPC modes 1, 2'
 DC XL16'3FFFFFFFFFFFFFFE3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +tie even FPC modes 3, 7'
 DC XL16'3FFFFFFFFFFFFFFE3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +tie even M3 modes 1, 3'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +tie even M3 modes 4, 5'
 DC XL16'3FFFFFFFFFFFFFFE3FFFFFFFFFFFFFFE'
 DC CL48'LDXBRA +tie even M3 modes 6, 7'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFE'
 DC CL48'LDXBRA -tie even FPC modes 1, 2'
 DC XL16'BFFFFFFFFFFFFFFEBFFFFFFFFFFFFFFE'
 DC CL48'LDXBRA -tie even FPC modes 3, 7'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -tie even M3 modes 1, 3'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -tie even M3 modes 4, 5'
 DC XL16'BFFFFFFFFFFFFFFEBFFFFFFFFFFFFFFE'
 DC CL48'LDXBRA -tie even M3 modes 6, 7'
 DC XL16'BFFFFFFFFFFFFFFEBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +false exact FPC modes 1, 2'
 DC XL16'3FFFFFFFFFFFFFFF4000000000000000'
 DC CL48'LDXBRA +false exact FPC modes 3, 7'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +false exact M3 modes 1, 3'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +false exact M3 modes 4, 5'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +false exact M3 modes 6, 7'
 DC XL16'40000000000000003FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -false exact FPC modes 1, 2'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -false exact FPC modes 3, 7'
 DC XL16'C000000000000000BFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -false exact M3 modes 1, 3'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -false exact M3 modes 4, 5'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -false exact M3 modes 6, 7'
 DC XL16'BFFFFFFFFFFFFFFFC000000000000000'
 DC CL48'LDXBRA +near zero FPC modes 1, 2'
 DC XL16'3FFFFFFFFFFFFFFF4000000000000000'
 DC CL48'LDXBRA +near zero FPC modes 3, 7'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +near zero M3 modes 1, 3'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +near zero M3 modes 4, 5'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +near zero M3 modes 6, 7'
 DC XL16'40000000000000003FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -near zero FPC modes 1, 2'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -near zero FPC modes 3, 7'
 DC XL16'C000000000000000BFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -near zero M3 modes 1, 3'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -near zero M3 modes 4, 5'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -near zero M3 modes 6, 7'
 DC XL16'BFFFFFFFFFFFFFFFC000000000000000'
 DC CL48'LDXBRA +near +inf FPC modes 1, 2'
 DC XL16'3FFFFFFFFFFFFFFF4000000000000000'
 DC CL48'LDXBRA +near +inf FPC modes 3, 7'
 DC XL16'3FFFFFFFFFFFFFFF3FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +near +inf M3 modes 1, 3'
 DC XL16'40000000000000003FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +near +inf M3 modes 4, 5'
 DC XL16'40000000000000003FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA +near +inf M3 modes 6, 7'
 DC XL16'40000000000000003FFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -near -inf FPC modes 1, 2'
 DC XL16'BFFFFFFFFFFFFFFFBFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -near -inf FPC modes 3, 7'
 DC XL16'C000000000000000BFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -near -inf M3 modes 1, 3'
 DC XL16'C000000000000000BFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -near -inf M3 modes 4, 5'
 DC XL16'C000000000000000BFFFFFFFFFFFFFFF'
 DC CL48'LDXBRA -near -inf M3 modes 6, 7'
 DC XL16'BFFFFFFFFFFFFFFFC000000000000000'
 DC CL48'LDXBRA +overflow FPC modes 1, 2'
 DC XL16'7FEFFFFFFFFFFFFF7FF0000000000000'
 DC CL48'LDXBRA +overflow FPC modes 3, 7'
 DC XL16'7FEFFFFFFFFFFFFF7FEFFFFFFFFFFFFF'
 DC CL48'LDXBRA +overflow M3 modes 1, 3'
 DC XL16'7FF00000000000007FEFFFFFFFFFFFFF'
 DC CL48'LDXBRA +overflow M3 modes 4, 5'
 DC XL16'7FF00000000000007FEFFFFFFFFFFFFF'
 DC CL48'LDXBRA +overflow M3 modes 6, 7'
 DC XL16'7FF00000000000007FEFFFFFFFFFFFFF'
 DC CL48'LDXBRA -overflow FPC modes 1, 2'
 DC XL16'FFEFFFFFFFFFFFFFFFEFFFFFFFFFFFFF'
 DC CL48'LDXBRA -overflow FPC modes 3, 7'
 DC XL16'FFF0000000000000FFEFFFFFFFFFFFFF'
 DC CL48'LDXBRA -overflow M3 modes 1, 3'
 DC XL16'FFF0000000000000FFEFFFFFFFFFFFFF'
 DC CL48'LDXBRA -overflow M3 modes 4, 5'
 DC XL16'FFF0000000000000FFEFFFFFFFFFFFFF'
 DC CL48'LDXBRA -overflow M3 modes 6, 7'
 DC XL16'FFEFFFFFFFFFFFFFFFF0000000000000'
 DC CL48'LDXBRA +tiny tie odd FPC modes 1, 2'
 DC XL16'00000000000000000000000000000001'
 DC CL48'LDXBRA +tiny tie odd FPC modes 3, 7'
 DC XL16'00000000000000000000000000000001'
 DC CL48'LDXBRA +tiny tie odd M3 modes 1, 3'
 DC XL16'00000000000000010000000000000001'
 DC CL48'LDXBRA +tiny tie odd M3 modes 4, 5'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LDXBRA +tiny tie odd M3 modes 6, 7'
 DC XL16'00000000000000010000000000000000'
 DC CL48'LDXBRA -tiny tie odd FPC modes 1, 2'
 DC XL16'80000000000000008000000000000000'
 DC CL48'LDXBRA -tiny tie odd FPC modes 3, 7'
 DC XL16'80000000000000018000000000000001'
 DC CL48'LDXBRA -tiny tie odd M3 modes 1, 3'
 DC XL16'80000000000000018000000000000001'
 DC CL48'LDXBRA -tiny tie odd M3 modes 4, 5'
 DC XL16'80000000000000008000000000000000'
 DC CL48'LDXBRA -tiny tie odd M3 modes 6, 7'
 DC XL16'80000000000000008000000000000001'
XTOLRMO_NUM EQU (*-XTOLRMO_GOOD)/64
*
*
XTOLRMOF_GOOD EQU *
 DC CL48'LDXBRA +exact FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LDXBRA +exact M3 modes 1, 3-5 FPCR'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LDXBRA +exact M3 modes 6, 7 FCPR'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LDXBRA -exact FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LDXBRA -exact M3 modes 1, 3-5 FPCR'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LDXBRA -exact M3 modes 6, 7 FCPR'
 DC XL16'00000000000000000000000000000000'
 DC CL48'LDXBRA +tie odd FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LDXBRA +tie odd M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LDXBRA +tie odd M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LDXBRA -tie odd FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LDXBRA -tie odd M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LDXBRA -tie odd M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LDXBRA +tie even FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LDXBRA +tie even M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LDXBRA +tie even M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LDXBRA -tie even FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LDXBRA -tie even M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LDXBRA -tie even M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LDXBRA +false exact FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LDXBRA +false exact M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LDXBRA +false exact M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LDXBRA -false exact FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LDXBRA -false exact M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LDXBRA -false exact M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LDXBRA +near zero FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LDXBRA +near zero M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LDXBRA +near zero M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LDXBRA -near zero FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LDXBRA -near zero M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LDXBRA -near zero M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LDXBRA +near +inf FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LDXBRA +near +inf M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LDXBRA +near +inf M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LDXBRA -near -inf FPC modes 1-3, 7 FCPR'
 DC XL16'00000001000000020000000300000007'
 DC CL48'LDXBRA -near -inf M3 modes 1, 3-5 FPCR'
 DC XL16'00080000000800000008000000080000'
 DC CL48'LDXBRA -near -inf M3 modes 6, 7 FCPR'
 DC XL16'00080000000800000000000000000000'
 DC CL48'LDXBRA +overflow FPCR modes 1-3, 7 FPCR'
 DC XL16'00000001002000020000000300000007'
 DC CL48'LDXBRA +overflow M3 modes 1, 3-5 FPCR'
 DC XL16'00280000000800000028000000080000'
 DC CL48'LDXBRA +overflow M3 modes 6, 7 FPCR'
 DC XL16'00280000000800000000000000000000'
 DC CL48'LDXBRA -overflow FPCR modes 1-3, 7 FPCR'
 DC XL16'00000001000000020020000300000007'
 DC CL48'LDXBRA -overflow M3 modes 1, 3-5 FPCR'
 DC XL16'00280000000800000028000000080000'
 DC CL48'LDXBRA -overflow M3 modes 6, 7 FPCR'
 DC XL16'00080000002800000000000000000000'
 DC CL48'LDXBRA +tiny tie odd FPCR modes 1-3, 7 FPCR'
 DC XL16'00100001001000020010000300100007'
 DC CL48'LDXBRA +tiny tie odd M3 modes 1, 3-5 FPCR'
 DC XL16'00180000001800000018000000180000'
 DC CL48'LDXBRA +tiny tie odd M3 modes 6, 7 FPCR'
 DC XL16'00180000001800000000000000000000'
 DC CL48'LDXBRA -tiny tie odd FPCR modes 1-3, 7 FPCR'
 DC XL16'00100001001000020010000300100007'
 DC CL48'LDXBRA -tiny tie odd M3 modes 1, 3-5 FPCR'
 DC XL16'00180000001800000018000000180000'
 DC CL48'LDXBRA -tiny tie odd M3 modes 6, 7 FPCR'
 DC XL16'00180000001800000000000000000000'
XTOLRMOF_NUM EQU (*-XTOLRMOF_GOOD)/64
*
*
LTOSOUO_GOOD EQU *
 DC CL48'LEDBR trap results 1-2'
 DC XL16'27F0000000000000A7F0000000000000'
 DC CL48'LEDBR trap results 3-4'
 DC XL16'27FFFFFFE0000000A7FFFFFFE0000000'
 DC CL48'LEDBR trap results 5-6'
 DC XL16'5690000000000000D690000000000000'
 DC CL48'LEDBR trap results 7-8'
 DC XL16'27F0000000000000A7F0000000000000'
LTOSOUO_NUM EQU (*-LTOSOUO_GOOD)/64
*
*
LTOSOUOF_GOOD EQU *
 DC CL48'LEDBR trap FPCR 1-4'
 DC XL16'F8002C00F8002C00F8002800F8002800'
 DC CL48'LEDBR trap FPCR 5-8'
 DC XL16'F8001000F8001000F8002000F8002000'
LTOSOUOF_NUM EQU (*-LTOSOUOF_GOOD)/64
*
*
XTOSOUO_GOOD EQU *
 DC CL48'LEXBR trap results 1'
 DC XL16'207F0000000000000000000000000000'
 DC CL48'LEXBR trap results 2'
 DC XL16'A07F0000000000000000000000000000'
 DC CL48'LEXBR trap results 3'
 DC XL16'207FFFFFFE0000000000000000000000'
 DC CL48'LEXBR trap results 4'
 DC XL16'A07FFFFFFE0000000000000000000000'
 DC CL48'LEXBR trap results 5'
 DC XL16'5F690000000000000000000000000000'
 DC CL48'LEXBR trap results 6'
 DC XL16'DF690000000000000000000000000000'
 DC CL48'LEXBR trap results 7'
 DC XL16'207F0000000000000000000000000000'
 DC CL48'LEXBR trap results 8'
 DC XL16'A07F0000000000000000000000000000'
XTOSOUO_NUM EQU (*-XTOSOUO_GOOD)/64
*
*
XTOSOUOF_GOOD EQU *
 DC CL48'LEXBR trap FPCR 1-4'
 DC XL16'F8002C00F8002C00F8002800F8002800'
 DC CL48'LEXBR trap FPCR 5-8'
 DC XL16'F8001000F8001000F8002000F8002000'
XTOSOUOF_NUM EQU (*-XTOSOUOF_GOOD)/64
*
*
XTOLOUO_GOOD EQU *
 DC CL48'LDXBR trap result 1'
 DC XL16'23FF0000000000000000000000000000'
 DC CL48'LDXBR trap result 2'
 DC XL16'A3FF0000000000000000000000000000'
 DC CL48'LDXBR trap result 3'
 DC XL16'23FFFFFFFFFFFFFFF000000000000000'
 DC CL48'LDXBR trap result 4'
 DC XL16'A3FFFFFFFFFFFFFFF000000000000000'
 DC CL48'LDXBR trap result 5'
 DC XL16'5BCC0000000000000000000000000000'
 DC CL48'LDXBR trap result 6'
 DC XL16'DBCC0000000000000000000000000000'
 DC CL48'LDXBR trap result 7'
 DC XL16'23FF0000000000000000000000000000'
 DC CL48'LDXBR trap result 8'
 DC XL16'A3FF0000000000000000000000000000'
XTOLOUO_NUM EQU (*-XTOLOUO_GOOD)/64
*
*
XTOLOUOF_GOOD EQU *
 DC CL48'LDXBR trap FPCR 1-4'
 DC XL16'F8002C00F8002C00F8002800F8002800'
 DC CL48'LDXBR trap FPCR 5-8'
 DC XL16'F8001000F8001000F8002000F8002000'
XTOLOUOF_NUM EQU (*-XTOLOUOF_GOOD)/64
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
         DC    A(LTOSOUT)
         DC    A(LTOSOUT_GOOD)
         DC    A(LTOSOUT_NUM)
*
         DC    A(LTOSFLGS)
         DC    A(LTOSFLGS_GOOD)
         DC    A(LTOSFLGS_NUM)
*
         DC    A(LTOSRMO)
         DC    A(LTOSRMO_GOOD)
         DC    A(LTOSRMO_NUM)
*
         DC    A(LTOSRMOF)
         DC    A(LTOSRMOF_GOOD)
         DC    A(LTOSRMOF_NUM)
*
         DC    A(XTOSOUT)
         DC    A(XTOSOUT_GOOD)
         DC    A(XTOSOUT_NUM)
*
         DC    A(XTOSFLGS)
         DC    A(XTOSFLGS_GOOD)
         DC    A(XTOSFLGS_NUM)
*
         DC    A(XTOSRMO)
         DC    A(XTOSRMO_GOOD)
         DC    A(XTOSRMO_NUM)
*
         DC    A(XTOSRMOF)
         DC    A(XTOSRMOF_GOOD)
         DC    A(XTOSRMOF_NUM)
*
         DC    A(XTOLOUT)
         DC    A(XTOLOUT_GOOD)
         DC    A(XTOLOUT_NUM)
*
         DC    A(XTOLFLGS)
         DC    A(XTOLFLGS_GOOD)
         DC    A(XTOLFLGS_NUM)
*
         DC    A(XTOLRMO)
         DC    A(XTOLRMO_GOOD)
         DC    A(XTOLRMO_NUM)
*
         DC    A(XTOLRMOF)
         DC    A(XTOLRMOF_GOOD)
         DC    A(XTOLRMOF_NUM)
*
         DC    A(LTOSOUO)
         DC    A(LTOSOUO_GOOD)
         DC    A(LTOSOUO_NUM)
*
         DC    A(LTOSOUOF)
         DC    A(LTOSOUOF_GOOD)
         DC    A(LTOSOUOF_NUM)
*
         DC    A(XTOSOUO)
         DC    A(XTOSOUO_GOOD)
         DC    A(XTOSOUO_NUM)
*
         DC    A(XTOSOUOF)
         DC    A(XTOSOUOF_GOOD)
         DC    A(XTOSOUOF_NUM)
*
         DC    A(XTOLOUO)
         DC    A(XTOLOUO_GOOD)
         DC    A(XTOLOUO_NUM)
*
         DC    A(XTOLOUOF)
         DC    A(XTOLOUOF_GOOD)
         DC    A(XTOLOUOF_NUM)
*
VERIFLEN EQU   (*-VERIFTAB)/12    #of entries in verify table
                                                                EJECT
         END
