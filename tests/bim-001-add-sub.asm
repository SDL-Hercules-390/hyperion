  TITLE 'bim-001-add-sub.asm: Test Basic Integer Math Add & Subtract'
***********************************************************************
*
*Testcase bim-001-add-sub
*  Test case capability includes condition codes and fixed point
*  overflow interruptions.
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
*                        bim-001-add-sub.asm
*
* Copyright 2018 by Stephen R Orso.
*
* Distributed under the Boost Software License, Version 1.0.  See
* accompanying file BOOST_LICENSE_1_0.txt or a copy at:
*
*      http://www.boost.org/LICENSE_1_0.txt)
*
* Adapted from the original bim-001-add by Peter J. Jansen.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
* Tests the following ADD and SUB instructions, except those marked (*)
* as these are not (yet) implemented.
*   ADD REGISTER RR    AR   32-bit sum, augend, addend
*            (*) RRF-a ARK  32-bit sum, augend, addend, 3 operand
*                RRE   AGR  64-bit sum, augend, addend
*            (*) RRF-a ARGK 64-bit sum, augend, addend, 3 operand
*            (*) RRE   AGFR 64-bit augend, sum, 32-bit addend
*   ADD      (*) RX-a  A    32-bit sum, augend, addend
*            (*) RXY-a AY   32-bit sum, augend, addend
*            (*) RXY-a AG   64-bit sum, augend, addend
*            (*) RXY-a AGF  64-bit augend, sum, 32-bit addend
*   SUB REGISTER RR    SR   32-bit sum, minuend, subtrahend
*            (*) RRF-a SRK  32-bit sum, minuend, subtrahend, 3 operand
*                RRE   SGR  64-bit sum, minuend, subtrahend
*            (*) RRF-a SRGK 64-bit sum, minuend, subtrahend, 3 operand
*            (*) RRE   SGFR 64-bit minuend, sum, 32-bit subtrahend
*   SUB      (*) RX-a  S    32-bit sum, minuend, subtrahend
*            (*) RXY-a SY   32-bit sum, minuend, subtrahend
*            (*) RXY-a SG   64-bit sum, minuend, subtrahend
*            (*) RXY-a SGF  64-bit minuend, sum, 32-bit subtrahend
*
* Instructions are test against the definition in the z/Architecture
*   Principles of Operation, SA22-7832-11 (September, 2017), p. 7-27
*   and p. 7-387
         EJECT
*
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules 'r'
* commands.
*
* Basic 32-bit test data is:    2147483647(=M), 0, 1, -1,
*            -2147483647(=-M), -2147483648(=-M-1=-(M+1))
*
* Basic 64-bit test data is:    9223372036854775807(=G) , 0, 1, -1,
*   -9223372036854775807(=-G), -9223372036854775808(=-G-1=-(G+1))
*
* Test Case Order
* 1) AR,  RR, 32-bit addition
* 2) AGR, RR, 64-bit addition
* 3) SR,  RR, 32-bit subtraction
* 4) SGR, RR, 64-bit subtraction
*
* Routines have not been coded for the other ADD / SUB instructions.
*   It would not be hard, and no additional test data would be needed.
*
* Each value is added / subtracted to every value twice, once with
*   interruptions suppressed, once with interruptions enabled.
*   72 results are generated for each such test case.
*
* Opportunities for future development:
* - Use SIGP to change the processor mode and verify correct operation
*   in 390 and 370 mode, including verification that ADD varients
*   that are unsupported generate the expected operation exceptions
* - Add the remaining RR* and the RX* instructions.
*
***********************************************************************
         EJECT
*
BIMADSUB START 0
STRTLABL EQU   *
R0       EQU   0                   Work register for cc extraction
*                                  ..also augend / minuend and result
*                                  ..register for two-operand add and
*                                  ..subtract
R1       EQU   1                   addend / subtrahend register for
*                                  ..RR two-operand variants.
R2       EQU   2                   Count of test augends / minuends
*                                  ..remaining
R3       EQU   3                   Pointer to next test augend /
*                                  ..minuend
R4       EQU   4                   Count of test addends /
*                                  ..subtrahends remaining
R5       EQU   5                   Pointer to next test addend /
*                                  ..subtrahend
R6       EQU   6                   Size of each augend / minuend and
*                                  ..addend / subtrahend
R7       EQU   7                   Pointer to next result value(s)
R8       EQU   8                   Pointer to next cc/fixed point ovfl
R9       EQU   9                   Top of inner loop address in tests
R10      EQU   10                  Pointer to test address list
R11      EQU   11                  **Reserved for z/CMS test rig
R12      EQU   12                  Top of outer loop address in tests
R13      EQU   13                  Return address to mainline
R14      EQU   14                  **Return address for z/CMS test rig
R15      EQU   15                  **Base register on z/CMS or Hyperion

*
         USING *,R15
         USING HELPERS,R12
*
* Above is assumed to works on real iron (R15=0 after sysclear) and in
* John's z/CMS test rig (R15 points to start of load module).
*
* Selective z/Arch low core layout
*
         ORG   STRTLABL+X'8C'      Program check interrution code
PCINTCD  DS    F
*
PCOLDPSW EQU   STRTLABL+X'150'     z/Arch Program check old PSW
*
         ORG   STRTLABL+X'1A0'     z/Arch Restart PSW
         DC    X'0000000180000000',AD(START)
*
         ORG   STRTLABL+X'1D0'     z/Arch Program check NEW PSW
         DC    X'0000000000000000',AD(PROGCHK)
         EJECT
*
* Program check routine.  If Data Exception, continue execution at
* the instruction following the program check.  Otherwise, hard wait.
* No need to collect data.
*
         ORG   STRTLABL+X'200'
PROGCHK  DS    0H               Program check occured...
         CLI   PCINTCD+3,X'08'  Fixed Point Overflow?
         JNE   PCNOTDTA         ..no, fail the test
         LPSWE PCOLDPSW         ..yes, resume program execution
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
START    DS    0H
*
* ADD REGISTER (32-bit operands, two operand)
*
         LA    R10,ARTABL    32-bit test table
         BAS   R13,ARTEST    AR, add register 32-bit
*
* ADD REGISTER (64-bit operands, two operand)
*
         LA    R10,AGRTABL   64-bit test table
         BAS   R13,AGRTEST   AGR, add register 64-bit
*
* SUB REGISTER (32-bit operands, two operand)
*
         LA    R10,SRTABL    32-bit test table
         BAS   R13,SRTEST    SR, subtract register 32-bit
*
* SUB REGISTER (64-bit operands, two operand)
*
         LA    R10,SGRTABL   64-bit test table
         BAS   R13,SGRTEST   SGR, subtract register 64-bit
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
                                                                SPACE
         DS    0D            Ensure correct alignment for PSW
GOODPSW  DC    X'0002000000000000',AD(0)  Normal end - disabled wait
FAILPSW  DC    X'0002000000000000',XL6'00',X'0BAD' Abnormal end
         EJECT
*
* Input values parameter list, six fullwords:
*      1) Count of augends / minuends (and addends / subtrahends)
*      2) Address of augends / minuends
*      3) Address of addends / subtrahends
*      4) Address to place sums / differences
*      5) Address to place condition code and interruption code
*      6) Size of augends / minuends, addends / subtrahends
*         ..and sums / differences
*
ARTABL   DS    0F           Inputs for 32-bit/32-bit tests
         DC    A(VALCT/4)
         DC    A(A32VALS)   Address of augends
         DC    A(A32VALS)   Address of addends
         DC    A(ARSUM)     Address to store sums
         DC    A(ARFLG)     Address to store cc, int code
         DC    A(4)         4 byte augends, addends and sums
*
AGRTABL  DS    0F           Inputs for 64-bit/64-bit tests
         DC    A(VALCT64/8)
         DC    A(A64VALS)   Address of augends
         DC    A(A64VALS)   Address of addends
         DC    A(AGRSUM)    Address to store sums
         DC    A(AGRFLG)    Address to store cc, int code
         DC    A(8)         8 byte augends, addends and sums
*
SRTABL   DS    0F           Inputs for 32-bit/32-bit tests
         DC    A(VALCT/4)
         DC    A(A32VALS)   Address of minuends
         DC    A(A32VALS)   Address of subtrahends
         DC    A(SRSUM)     Address to store differences
         DC    A(SRFLG)     Address to store cc, int code
         DC    A(4)         4 byte minuends, subtrahends and
*                           ..differences
*
SGRTABL  DS    0F           Inputs for 64-bit/64-bit tests
         DC    A(VALCT64/8)
         DC    A(A64VALS)   Address of minuends
         DC    A(A64VALS)   Address of addends
         DC    A(SGRSUM)    Address to store differences
         DC    A(SGRFLG)    Address to store cc, int code
         DC    A(8)         8 byte minuends, subtrahends and
*                           ..differences
         EJECT
***********************************************************************
*
* ADD REGISTER (AR, RRE) - 32-bit addend, 32-augend, 32-bit sum.
* Result replaces augend in operand 1.
*
***********************************************************************
         SPACE 2
ARTEST   IPM   R0            Get cc, program mask
         ST    R0,ARCCPM     Save for later disable of ints
         ST    R0,ARCCPMOV   Save for overflow enablement
         OI    ARCCPMOV,X'08'  Enable fixed point overflow ints
         LM    R2,R3,0(R10)  Get count and addresses of augends
         LM    R7,R8,12(R10) Get address of result area and flag area.
         L     R6,20(,R10)   Get size of augends, addends and results.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
* Top of outer loop.  Process next augend
*
         L     R4,0(,R10)    Get count of addends
         L     R5,8(,R10)    Get address of addend table
         BASR  R9,0          Set top of loop
*
         L     R0,0(,R3)     Initialize augend
         L     R1,0(,R5)     Initialize addend
         AR    R0,R1         Replace augend with sum
         ST    R0,0(,R7)     Store sum
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0
         STC   R0,0(,R8)     Store condition code
         LA    R7,0(R6,R7)   Point to next sum slot
         LA    R8,4(,R8)     Point to next cc-int code slot
*
* Repeat the instruction with Fixed Point Overflow interruptions
* enabled.
*
         L     R0,ARCCPMOV   Get cc/program mask for overflow ints
         SPM   R0            Enable Fixed Point Overflow inter.
         XC    PCINTCD,PCINTCD    Zero out PC interruption code
*
         L     R0,0(,R3)     Initialize augend
         L     R1,0(,R5)     Initialize addend
         AR    R0,R1         Replace augend with sum
         ST    R0,0(,R7)     Store sum
         MVC   1(3,R8),PCINTCD+1   Save interruption code
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0
         STC   R0,0(,R8)     Store condition code
*
         L     R0,ARCCPM     Get cc/program mask for no o'flow ints
         SPM   R0            Disable Fixed Point Overflow inter.
*
         LA    R5,0(R6,R5)   Point to next addend
         LA    R7,0(R6,R7)   Point to next sum slot
         LA    R8,4(,R8)     Point to next cc-int code slot
         BCTR  R4,R9         Loop through addends
*
* End of addends.  Process next augend
*
         LA    R3,0(R6,R3)   Point to next augend
         BCTR  R2,R12        Loop through augends
*
         BR    R13           All converted; return.
*
***********************************************************************
*
* ADD REGISTER (AGR, RRE) - 64-bit addend, 64-augend, 64-bit sum.
* Result replaces augend in operand 1.
*
***********************************************************************
         SPACE 2
AGRTEST  IPM   R0            Get cc, program mask
         ST    R0,ARCCPM     Save for later disable of ints
         ST    R0,ARCCPMOV   Save for overflow enablement
         OI    ARCCPMOV,X'08'  Enable fixed point overflow ints
         LM    R2,R3,0(R10)  Get count and addresses of augends
         LM    R7,R8,12(R10) Get address of result area and flag area.
         L     R6,20(,R10)   Get size of augends, addends and results.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
* Top of outer loop.  Process next augend
*
         L     R4,0(,R10)    Get count of addends
         L     R5,8(,R10)    Get address of addend table
         BASR  R9,0          Set top of loop
*
         LG    R0,0(,R3)     Initialize augend
         LG    R1,0(,R5)     Initialize addend
         AGR   R0,R1         Replace augend with sum
         STG   R0,0(,R7)     Store sum
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0
         STC   R0,0(,R8)     Store condition code
         LA    R7,0(R6,R7)   Point to next sum slot
         LA    R8,4(,R8)     Point to next cc-int code slot
*
* Repeat the instruction with Fixed Point Overflow interruptions
* enabled.
*
         L     R0,ARCCPMOV   Get cc/program mask for overflow ints
         SPM   R0            Enable Fixed Point Overflow inter.
         XC    PCINTCD,PCINTCD    Zero out PC interruption code
*
         LG    R0,0(,R3)     Initialize augend
         LG    R1,0(,R5)     Initialize addend
         AGR   R0,R1         Replace augend with sum
         STG   R0,0(,R7)     Store sum
         MVC   1(3,R8),PCINTCD+1   Save interruption code
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0
         STC   R0,0(,R8)     Store condition code
*
         L     R0,ARCCPM     Get cc/program mask for no o'flow ints
         SPM   R0            Disable Fixed Point Overflow inter.
*
         LA    R5,0(R6,R5)   Point to next addend
         LA    R7,0(R6,R7)   Point to next sum slot
         LA    R8,4(,R8)     Point to next cc-int code slot
         BCTR  R4,R9         Loop through addends
*
* End of addends.  Process next augend
*
         LA    R3,0(R6,R3)   Point to next augend
         BCTR  R2,R12        Loop through augends
*
         BR    R13           All converted; return.
*
***********************************************************************
*
* SUB REGISTER (SR, RRE) - 32-bit subtrahend, 32-minuend, 32-bit sum.
* Result replaces subtrahend in operand 1.
*
***********************************************************************
         SPACE 2
SRTEST   IPM   R0            Get cc, program mask
         ST    R0,ARCCPM     Save for later disable of ints
         ST    R0,ARCCPMOV   Save for overflow enablement
         OI    ARCCPMOV,X'08'  Enable fixed point overflow ints
         LM    R2,R3,0(R10)  Get count and addresses of minuends
         LM    R7,R8,12(R10) Get address of result area and flag area.
         L     R6,20(,R10)   Get size of minuends, subtrahends
*                            ..and results.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
* Top of outer loop.  Process next minuend
*
         L     R4,0(,R10)    Get count of subtrahends
         L     R5,8(,R10)    Get address of subtrahend table
         BASR  R9,0          Set top of loop
*
         L     R0,0(,R3)     Initialize minuend
         L     R1,0(,R5)     Initialize subtrahend
         SR    R0,R1         Replace minuend with difference
         ST    R0,0(,R7)     Store difference
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0
         STC   R0,0(,R8)     Store condition code
         LA    R7,0(R6,R7)   Point to next sum slot
         LA    R8,4(,R8)     Point to next cc-int code slot
*
* Repeat the instruction with Fixed Point Overflow interruptions
* enabled.
*
         L     R0,ARCCPMOV   Get cc/program mask for overflow ints
         SPM   R0            Enable Fixed Point Overflow inter.
         XC    PCINTCD,PCINTCD    Zero out PC interruption code
*
         L     R0,0(,R3)     Initialize minuend
         L     R1,0(,R5)     Initialize subtrahend
         SR    R0,R1         Replace minuend with difference
         ST    R0,0(,R7)     Store difference
         MVC   1(3,R8),PCINTCD+1   Save interruption code
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0
         STC   R0,0(,R8)     Store condition code
*
         L     R0,ARCCPM     Get cc/program mask for no o'flow ints
         SPM   R0            Disable Fixed Point Overflow inter.
*
         LA    R5,0(R6,R5)   Point to next subtrahend
         LA    R7,0(R6,R7)   Point to next difference slot
         LA    R8,4(,R8)     Point to next cc-int code slot
         BCTR  R4,R9         Loop through subtrahends
*
* End of subtrahends.  Process next minuend
*
         LA    R3,0(R6,R3)   Point to next minuend
         BCTR  R2,R12        Loop through minuends
*
         BR    R13           All converted; return.
*
***********************************************************************
*
* SUB REGISTER (SGR, RRE) - 64-bit subtrahend, 64-minuend, 64-bit sum.
* Result replaces subtrahend in operand 1.
*
***********************************************************************
         SPACE 2
SGRTEST  IPM   R0            Get cc, program mask
         ST    R0,ARCCPM     Save for later disable of ints
         ST    R0,ARCCPMOV   Save for overflow enablement
         OI    ARCCPMOV,X'08'  Enable fixed point overflow ints
         LM    R2,R3,0(R10)  Get count and addresses of minuends
         LM    R7,R8,12(R10) Get address of result area and flag area.
         L     R6,20(,R10)   Get size of minuends, subtrahends and
*                            ..and results.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
* Top of outer loop.  Process next minuend
*
         L     R4,0(,R10)    Get count of subtrahends
         L     R5,8(,R10)    Get address of subtrahend table
         BASR  R9,0          Set top of loop
*
         LG    R0,0(,R3)     Initialize minuend
         LG    R1,0(,R5)     Initialize subtrahend
         SGR   R0,R1         Replace minuend with difference
         STG   R0,0(,R7)     Store difference
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0
         STC   R0,0(,R8)     Store condition code
         LA    R7,0(R6,R7)   Point to next difference slot
         LA    R8,4(,R8)     Point to next cc-int code slot
*
* Repeat the instruction with Fixed Point Overflow interruptions
* enabled.
*
         L     R0,ARCCPMOV   Get cc/program mask for overflow ints
         SPM   R0            Enable Fixed Point Overflow inter.
         XC    PCINTCD,PCINTCD    Zero out PC interruption code
*
         LG    R0,0(,R3)     Initialize minuend
         LG    R1,0(,R5)     Initialize subtrahend
         SGR   R0,R1         Replace minuend with difference
         STG   R0,0(,R7)     Store difference
         MVC   1(3,R8),PCINTCD+1   Save interruption code
         IPM   R0            Retrieve condition code
         SRL   R0,28         Move CC to low-order r0
         STC   R0,0(,R8)     Store condition code
*
         L     R0,ARCCPM     Get cc/program mask for no o'flow ints
         SPM   R0            Disable Fixed Point Overflow inter.
*
         LA    R5,0(R6,R5)   Point to next subtrahend
         LA    R7,0(R6,R7)   Point to next difference slot
         LA    R8,4(,R8)     Point to next cc-int code slot
         BCTR  R4,R9         Loop through subtrahends
*
* End of subtrahends.  Process next minuend
*
         LA    R3,0(R6,R3)   Point to next minuend
         BCTR  R2,R12        Loop through minuends
*
         BR    R13           All converted; return.
*
ARCCPM   DC    F'0'          Savearea for cc/program mask
ARCCPMOV DC    F'0'          cc/program mask with interupts enabled
         EJECT
***********************************************************************
*
* Integer inputs.  The same values are used for 32-bit addends /
* subtrahends and augends / minuends.  Each addend is added to each
* augend, and each subtrahend is subtracted from each minuend.
*
* N.B., the number of 32-bit and 64-bit test values must be the same.
*
***********************************************************************
          SPACE 2
*
* 32-bit test inputs.
*
A32VALS  DS    0D                      32-bit operands
         DC    F'2147483647'           32-bit max pos. int.     ( M)
         DC    F'1'
         DC    F'0'
         DC    F'-1'
         DC    F'-2147483647'          32-bit max neg. int. + 1 (-M)
         DC    F'-2147483648'          32-bit max neg. int.     (-M-1)
*
VALCT    EQU   *-A32VALS               Count of integers in list * 4
         SPACE 3
*
* 64-bit test inputs.
*
A64VALS  DS    0D                      64-bit operands
         DC    D'9223372036854775807'  64-bit max pos. int.     ( G)
         DC    D'1'
         DC    D'0'
         DC    D'-1'
         DC    D'-9223372036854775807' 64-bit max neg. int. + 1 (-G)
         DC    D'-9223372036854775808' 64-bit max neg. int.     (-G-1)
VALCT64  EQU   *-A64VALS               Count of integers in list * 8
         EJECT
***********************************************************************
*                 ACTUAL results saved here
***********************************************************************
*
*               Locations for ACTUAL results
*
ARSUM    EQU   STRTLABL+X'1000'    AR results
*                                  ..72 results used
ARFLG    EQU   STRTLABL+X'2000'    Condition and interrupt codes
*                                  ..72 results used
AGRSUM   EQU   STRTLABL+X'1400'    AGR results
*                                  ..72 results used
AGRFLG   EQU   STRTLABL+X'2400'    Condition and interrupt codes
*                                  ..72 results used
SRSUM    EQU   STRTLABL+X'1800'    SR results
*                                  ..72 results used
SRFLG    EQU   STRTLABL+X'2800'    Condition and interrupt codes
*                                  ..72 results used
SGRSUM   EQU   STRTLABL+X'1C00'    SGR results
*                                  ..72 results used
SGRFLG   EQU   STRTLABL+X'2C00'    Condition and interrupt codes
*                                  ..72 results used
ENDRES   EQU   STRTLABL+X'3000'    next location for results
         EJECT
***********************************************************************
*                    EXPECTED results
***********************************************************************
*
         ORG   STRTLABL+X'10000'   (FAR past end of actual results)
*
ARSUM_GOOD EQU *
 DC CL64'M=7F..FF+   M=7F..FF,   M=7F..FF+   1=00..01'
 DC XL16'FFFFFFFEFFFFFFFE8000000080000000'
 DC CL64'M=7F..FF+   0=00..00,   M=7F..FF+  -1=FF..FF'
 DC XL16'7FFFFFFF7FFFFFFF7FFFFFFE7FFFFFFE'
 DC CL64'M=7F..FF+-M  =80..01,   M=7F..FF+-M-1=80..00'
 DC XL16'0000000000000000FFFFFFFFFFFFFFFF'
 DC CL64'1=00..01+   M=7F..FF,   1=00..01+   1=00..01'
 DC XL16'80000000800000000000000200000002'
 DC CL64'1=00..01+   0=00..00,   1=00..01+  -1=FF..FF'
 DC XL16'00000001000000010000000000000000'
 DC CL64'1=00..01+-M  =80..01,   1=00..01+-M-1=80..00'
 DC XL16'80000002800000028000000180000001'
 DC CL64'0=00..00+   M=7F..FF,   0=00..00+   1=00..01'
 DC XL16'7FFFFFFF7FFFFFFF0000000100000001'
 DC CL64'0=00..00+   0=00..00,   0=00..00+  -1=FF..FF'
 DC XL16'0000000000000000FFFFFFFFFFFFFFFF'
 DC CL64'0=00..00+-M  =80..01,   0=00..00+-M-1=80..00'
 DC XL16'80000001800000018000000080000000'
 DC CL64'-1=FF..FF+   M=7F..FF,  -1=FF..FF+   1=00..01'
 DC XL16'7FFFFFFE7FFFFFFE0000000000000000'
 DC CL64'-1=FF..FF+   0=00..00,  -1=FF..FF+  -1=FF..FF'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFE'
 DC CL64'-1=FF..FF+-M  =80..01,  -1=FF..FF+-M-1=80..00'
 DC XL16'80000000800000007FFFFFFF7FFFFFFF'
 DC CL64'-M  =80..01+   M=7F..FF,-M  =80..01+   1=00..01'
 DC XL16'00000000000000008000000280000002'
 DC CL64'-M  =80..01+   0=00..00,-M  =80..01+  -1=FF..FF'
 DC XL16'80000001800000018000000080000000'
 DC CL64'-M  =80..01+-M  =80..01,-M  =80..01+-M-1=80..00'
 DC XL16'00000002000000020000000100000001'
 DC CL64'-M-1=80..00+   M=7F..FF,-M-1=80..00+  +1=00..01'
 DC XL16'FFFFFFFFFFFFFFFF8000000180000001'
 DC CL64'-M-1=80..00+   0=00..00,-M-1=80..00+  -1=FF..FF'
 DC XL16'80000000800000007FFFFFFF7FFFFFFF'
 DC CL64'-M-1=80..00+-M  =80..01,-M-1=80..00+-M-1=80..00'
 DC XL16'00000001000000010000000000000000'
ARSUM_NUM EQU (*-ARSUM_GOOD)/80
*
*
ARFLG_GOOD EQU *
 DC CL64'cc/fpo    M=EF..FF+   M=7F..FF,   M=EF..FF+   1=00..01'
 DC XL16'03000000030200080300000003020008'
 DC CL64'cc/fpo    M=EF..FF+   0=00..00,   M=EF..FF+  -1=FF..FF'
 DC XL16'02000000020000000200000002000000'
 DC CL64'cc/fpo    M=EF..FF+-M  =80..01,   M=EF..FF+-M-1=80..00'
 DC XL16'00000000000000000100000001000000'
 DC CL64'cc/fpo    1=00..01+   M=7F..FF,   1=00..01+   1=00..01'
 DC XL16'03000000030200080200000002000000'
 DC CL64'cc/fpo    1=00..01+   0=00..00,   1=00..01+  -1=FF..FF'
 DC XL16'02000000020000000000000000000000'
 DC CL64'cc/fpo    1=00..01+-M  =80..01,   1=00..01+-M-1=80..00'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo    0=00..00+   M=7F..FF,   0=00..00+   1=00..01'
 DC XL16'02000000020000000200000002000000'
 DC CL64'cc/fpo    0=00..00+   0=00..00,   0=00..00+  -1=FF..FF'
 DC XL16'00000000000000000100000001000000'
 DC CL64'cc/fpo    0=00..00+-M  =80..01,   0=00..00+-M-1=80..00'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo   -1=FF..FF+   M=7F..FF,  -1=FF..FF+   1=00..01'
 DC XL16'02000000020000000000000000000000'
 DC CL64'cc/fpo   -1=FF..FF+   0=00..00,  -1=FF..FF+  -1=FF..FF'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo   -1=FF..FF+-M  =80..01,  -1=FF..FF+-M-1=80..00'
 DC XL16'01000000010000000300000003020008'
 DC CL64'cc/fpo -M  =80..01+   M=7F..FF,-M  =80..01+   1=00..01'
 DC XL16'00000000000000000100000001000000'
 DC CL64'cc/fpo -M  =80..01+   0=00..00,-M  =80..01+  -1=FF..FF'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo -M  =80..01+-M  =80..01,-M  =80..01+-M-1=80..00'
 DC XL16'03000000030200080300000003020008'
 DC CL64'cc/fpo -M-1=80..00+   M=7F..FF,-M-1=80..00+  +1=00..01'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo -M-1=80..00+   0=00..00,-M-1=80..00+  -1=FF..FF'
 DC XL16'01000000010000000300000003020008'
 DC CL64'cc/fpo -M-1=80..00+-M  =80..01,-M-1=80..00+-M-1=80..00'
 DC XL16'03000000030200080300000003020008'
ARFLG_NUM EQU (*-ARFLG_GOOD)/80
*
*
AGRSUM_GOOD EQU *
 DC CL64'G=7F..FF+   G=7F..FF'
 DC XL16'FFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFE'
 DC CL64'G=7F..FF+   1=00..01'
 DC XL16'80000000000000008000000000000000'
 DC CL64'G=7F..FF+   0=00..00'
 DC XL16'7FFFFFFFFFFFFFFF7FFFFFFFFFFFFFFF'
 DC CL64'G=7F..FF+  -1=FF..FF'
 DC XL16'7FFFFFFFFFFFFFFE7FFFFFFFFFFFFFFE'
 DC CL64'G=7F..FF+-G  =80..01'
 DC XL16'00000000000000000000000000000000'
 DC CL64'G=7F..FF+-G-1=80..00'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL64'1=00..01+   G=7F..FF'
 DC XL16'80000000000000008000000000000000'
 DC CL64'1=00..01+   1=00..01'
 DC XL16'00000000000000020000000000000002'
 DC CL64'1=00..01+   0=00..00'
 DC XL16'00000000000000010000000000000001'
 DC CL64'1=00..01+  -1=FF..FF'
 DC XL16'00000000000000000000000000000000'
 DC CL64'1=00..01+-G  =80..01'
 DC XL16'80000000000000028000000000000002'
 DC CL64'1=00..01+-G-1=80..00'
 DC XL16'80000000000000018000000000000001'
 DC CL64'0=00..00+   G=7F..FF'
 DC XL16'7FFFFFFFFFFFFFFF7FFFFFFFFFFFFFFF'
 DC CL64'0=00..00+   1=00..01'
 DC XL16'00000000000000010000000000000001'
 DC CL64'0=00..00+   0=00..00'
 DC XL16'00000000000000000000000000000000'
 DC CL64'0=00..00+  -1=FF..FF'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL64'0=00..00+-G  =80..01'
 DC XL16'80000000000000018000000000000001'
 DC CL64'0=00..00+-M-1=80..00'
 DC XL16'80000000000000008000000000000000'
 DC CL64'-1=FF..FF+   G=7F..FF'
 DC XL16'7FFFFFFFFFFFFFFE7FFFFFFFFFFFFFFE'
 DC CL64'-1=FF..FF+   1=00..01'
 DC XL16'00000000000000000000000000000000'
 DC CL64'-1=FF..FF+   0=00..00'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL64'-1=FF..FF+  -1=FF..FF'
 DC XL16'FFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFE'
 DC CL64'-1=FF..FF+-G  =80..01,'
 DC XL16'80000000000000008000000000000000'
 DC CL64'-1=FF..FF+-G-1=80..00'
 DC XL16'7FFFFFFFFFFFFFFF7FFFFFFFFFFFFFFF'
 DC CL64'-G  =80..01+   G=7F..FF'
 DC XL16'00000000000000000000000000000000'
 DC CL64'-G  =80..01+   1=00..01'
 DC XL16'80000000000000028000000000000002'
 DC CL64'-G  =80..01+   0=00..00'
 DC XL16'80000000000000018000000000000001'
 DC CL64'-G  =80..01+  -1=FF..FF'
 DC XL16'80000000000000008000000000000000'
 DC CL64'-G  =80..01+-G  =80..01'
 DC XL16'00000000000000020000000000000002'
 DC CL64'-G  =80..01+-G-1=80..00'
 DC XL16'00000000000000010000000000000001'
 DC CL64'-G-1=80..00+   G=7F..FF'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL64'-G-1=80..00+  +1=00..01'
 DC XL16'80000000000000018000000000000001'
 DC CL64'-G-1=80..00+   0=00..00'
 DC XL16'80000000000000008000000000000000'
 DC CL64'-G-1=80..00+  -1=FF..FF'
 DC XL16'7FFFFFFFFFFFFFFF7FFFFFFFFFFFFFFF'
 DC CL64'-G-1=80..00+-M  =80..01'
 DC XL16'00000000000000010000000000000001'
 DC CL64'-G-1=80..00+-G-1=80..00'
 DC XL16'00000000000000000000000000000000'
AGRSUM_NUM EQU (*-AGRSUM_GOOD)/80
*
*
AGRFLG_GOOD EQU *
 DC CL64'cc/fpo    G=EF..FF+   G=7F..FF,   G=EF..FF+   1=00..01'
 DC XL16'03000000030400080300000003040008'
 DC CL64'cc/fpo    G=EF..FF+   0=00..00,   G=EF..FF+  -1=FF..FF'
 DC XL16'02000000020000000200000002000000'
 DC CL64'cc/fpo    M=EF..FF+-G  =80..01,   G=EF..FF+-G-1=80..00'
 DC XL16'00000000000000000100000001000000'
 DC CL64'cc/fpo    1=00..01+   G=7F..FF,   1=00..01+   1=00..01'
 DC XL16'03000000030400080200000002000000'
 DC CL64'cc/fpo    1=00..01+   0=00..00,   1=00..01+  -1=FF..FF'
 DC XL16'02000000020000000000000000000000'
 DC CL64'cc/fpo    1=00..01+-G  =80..01,   1=00..01+-G-1=80..00'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo    0=00..00+   M=7F..FF,   0=00..00+   1=00..01'
 DC XL16'02000000020000000200000002000000'
 DC CL64'cc/fpo    0=00..00+   0=00..00,   0=00..00+  -1=FF..FF'
 DC XL16'00000000000000000100000001000000'
 DC CL64'cc/fpo    0=00..00+-G  =80..01,   0=00..00+-G-1=80..00'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo   -1=FF..FF+   M=7F..FF,  -1=FF..FF+   1=00..01'
 DC XL16'02000000020000000000000000000000'
 DC CL64'cc/fpo   -1=FF..FF+   0=00..00,  -1=FF..FF+  -1=FF..FF'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo   -1=FF..FF+-G  =80..01,  -1=FF..FF+-G-1=80..00'
 DC XL16'01000000010000000300000003040008'
 DC CL64'cc/fpo -G  =80..01+   M=7F..FF,-G  =80..01+   1=00..01'
 DC XL16'00000000000000000100000001000000'
 DC CL64'cc/fpo -G  =80..01+   0=00..00,-G  =80..01+  -1=FF..FF'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo -G  =80..01+-G  =80..01,-G  =80..01+-G-1=80..00'
 DC XL16'03000000030400080300000003040008'
 DC CL64'cc/fpo -G-1=80..00+   M=7F..FF,-G-1=80..00+  +1=00..01'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo -G-1=80..00+   0=00..00,-G-1=80..00+  -1=FF..FF'
 DC XL16'01000000010000000300000003040008'
 DC CL64'cc/fpo -G-1=80..00+-G  =80..01,-G-1=80..00+-G-1=80..00'
 DC XL16'03000000030400080300000003040008'
AGRFLG_NUM EQU (*-AGRFLG_GOOD)/80
*
*
SRSUM_GOOD EQU *
 DC CL64'M=7F..FF-   M=7F..FF,   M=7F..FF-   1=00..01'
 DC XL16'00000000000000007FFFFFFE7FFFFFFE'
 DC CL64'M=7F..FF-   0=00..00,   M=7F..FF-  -1=FF..FF'
 DC XL16'7FFFFFFF7FFFFFFF8000000080000000'
 DC CL64'M=7F..FF--M  =80..01,   M=7F..FF--M-1=80..00'
 DC XL16'FFFFFFFEFFFFFFFEFFFFFFFFFFFFFFFF'
 DC CL64'1=00..01-   M=7F..FF,   1=00..01-   1=00..01'
 DC XL16'80000002800000020000000000000000'
 DC CL64'1=00..01-   0=00..00,   1=00..01-  -1=FF..FF'
 DC XL16'00000001000000010000000200000002'
 DC CL64'1=00..01--M  =80..01,   1=00..01--M-1=80..00'
 DC XL16'80000000800000008000000180000001'
 DC CL64'0=00..00-   M=7F..FF,   0=00..00-   1=00..01'
 DC XL16'8000000180000001FFFFFFFFFFFFFFFF'
 DC CL64'0=00..00-   0=00..00,   0=00..00-  -1=FF..FF'
 DC XL16'00000000000000000000000100000001'
 DC CL64'0=00..00--M  =80..01,   0=00..00--M-1=80..00'
 DC XL16'7FFFFFFF7FFFFFFF8000000080000000'
 DC CL64'-1=FF..FF-   M=7F..FF,  -1=FF..FF-   1=00..01'
 DC XL16'8000000080000000FFFFFFFEFFFFFFFE'
 DC CL64'-1=FF..FF-   0=00..00,  -1=FF..FF-  -1=FF..FF'
 DC XL16'FFFFFFFFFFFFFFFF0000000000000000'
 DC CL64'-1=FF..FF--M  =80..01,  -1=FF..FF--M-1=80..00'
 DC XL16'7FFFFFFE7FFFFFFE7FFFFFFF7FFFFFFF'
 DC CL64'-M  =80..01-   M=7F..FF,-M  =80..01-   1=00..01'
 DC XL16'00000002000000028000000080000000'
 DC CL64'-M  =80..01-   0=00..00,-M  =80..01-  -1=FF..FF'
 DC XL16'80000001800000018000000280000002'
 DC CL64'-M  =80..01--M  =80..01,-M  =80..01--M-1=80..00'
 DC XL16'00000000000000000000000100000001'
 DC CL64'-M-1=80..00-   M=7F..FF,-M-1=80..00-  +1=00..01'
 DC XL16'00000001000000017FFFFFFF7FFFFFFF'
 DC CL64'-M-1=80..00-   0=00..00,-M-1=80..00-  -1=FF..FF'
 DC XL16'80000000800000008000000180000001'
 DC CL64'-M-1=80..00--M  =80..01,-M-1=80..00--M-1=80..00'
 DC XL16'FFFFFFFFFFFFFFFF0000000000000000'
SRSUM_NUM EQU (*-SRSUM_GOOD)/80
*
*
SRFLG_GOOD EQU *
 DC CL64'cc/fpo    M=EF..FF-   M=7F..FF,   M=EF..FF-   1=00..01'
 DC XL16'00000000000000000200000002000000'
 DC CL64'cc/fpo    M=EF..FF-   0=00..00,   M=EF..FF-  -1=FF..FF'
 DC XL16'02000000020000000300000003020008'
 DC CL64'cc/fpo    M=EF..FF--M  =80..01,   M=EF..FF--M-1=80..00'
 DC XL16'03000000030200080300000003020008'
 DC CL64'cc/fpo    1=00..01-   M=7F..FF,   1=00..01-   1=00..01'
 DC XL16'01000000010000000000000000000000'
 DC CL64'cc/fpo    1=00..01-   0=00..00,   1=00..01-  -1=FF..FF'
 DC XL16'02000000020000000200000002000000'
 DC CL64'cc/fpo    1=00..01--M  =80..01,   1=00..01--M-1=80..00'
 DC XL16'03000000030200080300000003020008'
 DC CL64'cc/fpo    0=00..00-   M=7F..FF,   0=00..00-   1=00..01'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo    0=00..00-   0=00..00,   0=00..00-  -1=FF..FF'
 DC XL16'00000000000000000200000002000000'
 DC CL64'cc/fpo    0=00..00--M  =80..01,   0=00..00--M-1=80..00'
 DC XL16'02000000020000000300000003020008'
 DC CL64'cc/fpo   -1=FF..FF-   M=7F..FF,  -1=FF..FF-   1=00..01'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo   -1=FF..FF-   0=00..00,  -1=FF..FF-  -1=FF..FF'
 DC XL16'01000000010000000000000000000000'
 DC CL64'cc/fpo   -1=FF..FF--M  =80..01,  -1=FF..FF--M-1=80..00'
 DC XL16'02000000020000000200000002000000'
 DC CL64'cc/fpo -M  =80..01-   M=7F..FF,-M  =80..01-   1=00..01'
 DC XL16'03000000030200080100000001000000'
 DC CL64'cc/fpo -M  =80..01-   0=00..00,-M  =80..01-  -1=FF..FF'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo -M  =80..01--M  =80..01,-M  =80..01--M-1=80..00'
 DC XL16'00000000000000000200000002000000'
 DC CL64'cc/fpo -M-1=80..00-   M=7F..FF,-M-1=80..00-  +1=00..01'
 DC XL16'03000000030200080300000003020008'
 DC CL64'cc/fpo -M-1=80..00-   0=00..00,-M-1=80..00-  -1=FF..FF'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo -M-1=80..00--M  =80..01,-M-1=80..00--M-1=80..00'
 DC XL16'01000000010000000000000000000000'
SRFLG_NUM EQU (*-SRFLG_GOOD)/80
*
*
SGRSUM_GOOD EQU *
 DC CL64'G=7F..FF-   G=7F..FF'
 DC XL16'00000000000000000000000000000000'
 DC CL64'G=7F..FF-   1=00..01'
 DC XL16'7FFFFFFFFFFFFFFE7FFFFFFFFFFFFFFE'
 DC CL64'G=7F..FF-   0=00..00'
 DC XL16'7FFFFFFFFFFFFFFF7FFFFFFFFFFFFFFF'
 DC CL64'G=7F..FF-  -1=FF..FF'
 DC XL16'80000000000000008000000000000000'
 DC CL64'G=7F..FF--G  =80..01'
 DC XL16'FFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFE'
 DC CL64'G=7F..FF--G-1=80..00'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL64'1=00..01-   G=7F..FF'
 DC XL16'80000000000000028000000000000002'
 DC CL64'1=00..01-   1=00..01'
 DC XL16'00000000000000000000000000000000'
 DC CL64'1=00..01-   0=00..00'
 DC XL16'00000000000000010000000000000001'
 DC CL64'1=00..01-  -1=FF..FF'
 DC XL16'00000000000000020000000000000002'
 DC CL64'1=00..01--G  =80..01'
 DC XL16'80000000000000008000000000000000'
 DC CL64'1=00..01--G-1=80..00'
 DC XL16'80000000000000018000000000000001'
 DC CL64'0=00..00-   G=7F..FF'
 DC XL16'80000000000000018000000000000001'
 DC CL64'0=00..00-   1=00..01'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL64'0=00..00-   0=00..00'
 DC XL16'00000000000000000000000000000000'
 DC CL64'0=00..00-  -1=FF..FF'
 DC XL16'00000000000000010000000000000001'
 DC CL64'0=00..00--G  =80..01'
 DC XL16'7FFFFFFFFFFFFFFF7FFFFFFFFFFFFFFF'
 DC CL64'0=00..00--G-1=80..00'
 DC XL16'80000000000000008000000000000000'
 DC CL64'-1=FF..FF-   G=7F..FF'
 DC XL16'80000000000000008000000000000000'
 DC CL64'-1=FF..FF-   1=00..01'
 DC XL16'FFFFFFFFFFFFFFFEFFFFFFFFFFFFFFFE'
 DC CL64'-1=FF..FF-   0=00..00'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL64'-1=FF..FF-  -1=FF..FF'
 DC XL16'00000000000000000000000000000000'
 DC CL64'-1=FF..FF--G  =80..01,'
 DC XL16'7FFFFFFFFFFFFFFE7FFFFFFFFFFFFFFE'
 DC CL64'-1=FF..FF--G-1=80..00'
 DC XL16'7FFFFFFFFFFFFFFF7FFFFFFFFFFFFFFF'
 DC CL64'-G  =80..01-   G=7F..FF'
 DC XL16'00000000000000020000000000000002'
 DC CL64'-G  =80..01-   1=00..01'
 DC XL16'80000000000000008000000000000000'
 DC CL64'-G  =80..01-   0=00..00'
 DC XL16'80000000000000018000000000000001'
 DC CL64'-G  =80..01-  -1=FF..FF'
 DC XL16'80000000000000028000000000000002'
 DC CL64'-G  =80..01--G  =80..01'
 DC XL16'00000000000000000000000000000000'
 DC CL64'-G  =80..01--G-1=80..00'
 DC XL16'00000000000000010000000000000001'
 DC CL64'-G-1=80..00-   G=7F..FF'
 DC XL16'00000000000000010000000000000001'
 DC CL64'-G-1=80..00-  +1=00..01'
 DC XL16'7FFFFFFFFFFFFFFF7FFFFFFFFFFFFFFF'
 DC CL64'-G-1=80..00-   0=00..00'
 DC XL16'80000000000000008000000000000000'
 DC CL64'-G-1=80..00-  -1=FF..FF'
 DC XL16'80000000000000018000000000000001'
 DC CL64'-G-1=80..00--G  =80..01'
 DC XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
 DC CL64'-G-1=80..00--G-1=80..00'
 DC XL16'00000000000000000000000000000000'
SGRSUM_NUM EQU (*-SGRSUM_GOOD)/80
*
*
SGRFLG_GOOD EQU *
 DC CL64'cc/fpo    G=EF..FF-   G=7F..FF,   G=EF..FF-   1=00..01'
 DC XL16'00000000000000000200000002000000'
 DC CL64'cc/fpo    G=EF..FF-   0=00..00,   G=EF..FF-  -1=FF..FF'
 DC XL16'02000000020000000300000003040008'
 DC CL64'cc/fpo    G=EF..FF--G  =80..01,   G=EF..FF--G-1=80..00'
 DC XL16'03000000030400080300000003040008'
 DC CL64'cc/fpo    1=00..01-   G=7F..FF,   1=00..01-   1=00..01'
 DC XL16'01000000010000000000000000000000'
 DC CL64'cc/fpo    1=00..01-   0=00..00,   1=00..01-  -1=FF..FF'
 DC XL16'02000000020000000200000002000000'
 DC CL64'cc/fpo    1=00..01--G  =80..01,   1=00..01--G-1=80..00'
 DC XL16'03000000030400080300000003040008'
 DC CL64'cc/fpo    0=00..00-   M=7F..FF,   0=00..00-   1=00..01'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo    0=00..00-   0=00..00,   0=00..00-  -1=FF..FF'
 DC XL16'00000000000000000200000002000000'
 DC CL64'cc/fpo    0=00..00--G  =80..01,   0=00..00--G-1=80..00'
 DC XL16'02000000020000000300000003040008'
 DC CL64'cc/fpo   -1=FF..FF-   G=7F..FF,  -1=FF..FF-   1=00..01'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo   -1=FF..FF-   0=00..00,  -1=FF..FF-  -1=FF..FF'
 DC XL16'01000000010000000000000000000000'
 DC CL64'cc/fpo   -1=FF..FF--G  =80..01,  -1=FF..FF--G-1=80..00'
 DC XL16'02000000020000000200000002000000'
 DC CL64'cc/fpo -G  =80..01-   G=7F..FF,-G  =80..01-   1=00..01'
 DC XL16'03000000030400080100000001000000'
 DC CL64'cc/fpo -G  =80..01-   0=00..00,-G  =80..01-  -1=FF..FF'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo -G  =80..01--G  =80..01,-G  =80..01--G-1=80..00'
 DC XL16'00000000000000000200000002000000'
 DC CL64'cc/fpo -G-1=80..00-   G=7F..FF,-G-1=80..00-  +1=00..01'
 DC XL16'03000000030400080300000003040008'
 DC CL64'cc/fpo -G-1=80..00-   0=00..00,-G-1=80..00-  -1=FF..FF'
 DC XL16'01000000010000000100000001000000'
 DC CL64'cc/fpo -G-1=80..00--G  =80..01,-G-1=80..00--G-1=80..00'
 DC XL16'01000000010000000000000000000000'
SGRFLG_NUM EQU (*-SGRFLG_GOOD)/80
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
PROGMSG  DS   0CL70
         DC    CL20'PROGRAM CHECK! CODE '
PROGCODE DC    CL8'hhhhhhhh'
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
         CLC   0(16,R4),64(R5)  Actual results == Expected results?
         BNE   VERIFAIL         No, show failure
VERINEXT LA    R4,16(,R4)       Next actual result
         LA    R5,80(,R5)       Next expected result
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
         LA    R5,64(,R5)       R5 ==> expected results
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
FAILMSG1 DS   0CL84
         DC    CL20'COMPARISON FAILURE! '
FAILDESC DC    CL64'(description)'
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
         DC    A(ARSUM)
         DC    A(ARSUM_GOOD)
         DC    A(ARSUM_NUM)
*
         DC    A(ARFLG)
         DC    A(ARFLG_GOOD)
         DC    A(ARFLG_NUM)
*
         DC    A(AGRSUM)
         DC    A(AGRSUM_GOOD)
         DC    A(AGRSUM_NUM)
*
         DC    A(AGRFLG)
         DC    A(AGRFLG_GOOD)
         DC    A(AGRFLG_NUM)
*
         DC    A(SRSUM)
         DC    A(SRSUM_GOOD)
         DC    A(SRSUM_NUM)
*
         DC    A(SRFLG)
         DC    A(SRFLG_GOOD)
         DC    A(SRFLG_NUM)
*
         DC    A(SGRSUM)
         DC    A(SGRSUM_GOOD)
         DC    A(SGRSUM_NUM)
*
         DC    A(SGRFLG)
         DC    A(SGRFLG_GOOD)
         DC    A(SGRFLG_NUM)
*
VERIFLEN EQU   (*-VERIFTAB)/12    #of entries in verify table
                                                                EJECT
         END
