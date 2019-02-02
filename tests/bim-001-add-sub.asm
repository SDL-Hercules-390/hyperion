  TITLE 'bim-001-add-sub.asm: Test Basic Integer Math Add & Subtract'
***********************************************************************
*
*Testcase bim-001-add-sub
*  Test case capability includes condition codes and fixed point
*  overflow interruptions.
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
* this program can provide alternative test data through Hercules R
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
*   unsupported generate opreeation exceptions
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
         ORG   STRTLABL+X'1D0'     z/Arch Program check old PSW
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
         JNE   PCNOTDTA         ..no, hardwait
         LPSWE PCOLDPSW         ..yes, resume program execution
PCNOTDTA DS    0H

         LPSWE HARDWAIT         Not fixed point o'flow, disabled wait
         EJECT
***********************************************************************
*
*  Main program.  Enable Advanced Floating Point, process test cases.
*
START    DS    0H
*
* Prolog: Ensure result pages are mapped in TLB.
*
         LAY   R1,ARSUM      Point to first result area
         MVI   0(R1),X'F'    Make a dummy change to the page
*                            ..this forces page into TLB, sets R&C
         LAY   R1,ARFLG      Point to second result area         
         MVI   0(R1),X'F'    Make a dummy change to the page
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
* Epilog:
*
         LPSWE WAITPSW        EOJ, load disabled wait PSW
*
         DS    0D             Ensure correct alignment for psw
WAITPSW  DC    X'0002000000000000',AD(0)  Normal end - disabled wait
HARDWAIT DC    X'0002000000000000',XL6'00',X'DEAD' Abnormal end
*
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
         ORG   STRTLABL+X'300'
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
* augend, and each subtrahend is subtracted from ean minuend.
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
*
         LTORG ,
*
*  Locations for results
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
*
*
         END
