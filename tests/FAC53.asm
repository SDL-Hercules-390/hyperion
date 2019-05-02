 TITLE '                                 Test Facility 53 Instructions'

FAC53TST START 0

START    EQU   *




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




         USING START,R0



         ORG   START+X'1A0'                     z/Arch Restart PSW
         DC    XL12'000000018000000000000000'
         DC    A(BEGIN)



         ORG   START+X'1D0'                     z/Arch Program New PSW
         DC    XL12'000000018000000000000000'
         DC    A(PROGCHK)
                                                                EJECT
***********************************************************************
*                         TEST PROGRAM
***********************************************************************

         ORG      START+X'200'    START OF TEST PROGRAM

BEGIN    LMG      R0,R2,TESTVALS Load test registers

         LGR      R3,R0
         LGR      R4,R0
         LGR      R5,R0
         LGR      R6,R0
         LGR      R7,R0
         LGR      R8,R0
         LGR      R9,R0
         LGR      R10,R0
         LGR      R11,R0
         LGR      R12,R0
         LGR      R13,R0
         LGR      R14,R0
         LGR      R15,R0



*----------------------------------------------------------------------
*        LOAD AND ZERO RIGHTMOST BYTE (64)
*----------------------------------------------------------------------

         LZRG     R3,ONES         E3r00aaa002A
         CLG      R3,LZRB64       Correct value loaded?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort

*----------------------------------------------------------------------
*        LOAD AND ZERO RIGHTMOST BYTE (32)
*----------------------------------------------------------------------

         LZRF     R4,ONES         E3r00aaa003B
         CLG      R4,LZRB32       Correct value loaded?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort

*----------------------------------------------------------------------
*        LOAD LOGICAL AND ZERO RIGHTMOST BYTE (64 <- 32)
*----------------------------------------------------------------------

         LLZRGF   R5,ONES         E3r00aaa003A
         CLG      R5,LZRB64L      Correct value loaded?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort
                                                                EJECT
*----------------------------------------------------------------------
*        Initialize condition code...
*----------------------------------------------------------------------

         LTGR     R0,R0           Set condition code 0  (i.e. '8')

*----------------------------------------------------------------------
*        LOAD HALFWORD HIGH IMMEDIATE ON CONDITION (32 <- 16)
*----------------------------------------------------------------------

         LOCHHI   R6,-1,7         ECrmiiii004E
         CLG      R6,ZEROS        Correct value loaded?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort

         LOCHHI   R6,-1,8         ECrmiiii004E
         CLG      R6,LOC32H       Correct value loaded?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort

*----------------------------------------------------------------------
*        LOAD HALFWORD IMMEDIATE ON CONDITION (32 <- 16)
*----------------------------------------------------------------------

         LOCHI    R7,-1,7         ECrmiiii0042
         CLG      R7,ZEROS        Correct value loaded?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort

         LOCHI    R7,-1,8         ECrmiiii0042
         CLG      R7,LOC32        Correct value loaded?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort

*----------------------------------------------------------------------
*        LOAD HALFWORD IMMEDIATE ON CONDITION (64 <- 16)
*----------------------------------------------------------------------

         LOCGHI   R8,-1,7         ECrmiiii0046
         CLG      R8,ZEROS        Correct value loaded?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort

         LOCGHI   R8,-1,8         ECrmiiii0046
         CLG      R8,LOC64        Correct value loaded?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort
                                                                EJECT
*----------------------------------------------------------------------
*        LOAD HIGH ON CONDITION (32)
*----------------------------------------------------------------------

         LOCFHR   R9,R1,7         B9E0m0xy
         CLG      R9,ZEROS        Correct value loaded?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort

         LOCFHR   R9,R1,8         B9E0m0xy
         CLG      R9,LOC3211S     Correct value loaded?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort

*----------------------------------------------------------------------
*        LOAD HIGH ON CONDITION (32)
*----------------------------------------------------------------------

         LOCFH    R10,ONES,7      EBrm0aaa00E0
         CLG      R10,ZEROS       Correct value loaded?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort

         LOCFH    R10,ONES,8      EBrm0aaa00E0
         CLG      R10,LOC3211S    Correct value loaded?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort

*----------------------------------------------------------------------
*        STORE HIGH ON CONDITION
*----------------------------------------------------------------------

         STOCFH   R1,STOCFH,7     EBrm0aaa00E1
         CLG      R2,STOCFH       Correct value stored?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort


         STOCFH   R1,STOCFH,8     EBrm0aaa00E1
         CLC      STOCFH,STOCFH1F Correct value stored?
         JE       *+8             Yes, continue on to next test
         JAS      R15,FAILURE     No, abort
                                                                EJECT
*----------------------------------------------------------------------
*                 SUCCESSFUL END OF TEST
*----------------------------------------------------------------------

         J        SUCCESS         SUCCESSFUL END OF TEST




***********************************************************************
*                      WORKING STORAGE
***********************************************************************

SUCCESS  LPSWE    GOODPSW         Load test completed successfully PSW
FAILURE  LPSWE    BADPSW          Load the test FAILED somewhere!! PSW
PROGCHK  LPSWE    DEADBEEF        Load "A PROGRAM-CHECK OCCURRED!" PSW


         ORG   START+X'400'


GOODPSW  DC    XL8'0002000180000000'
         DC    XL4'00000000',A(X'00000000')


BADPSW   DC    XL8'0002000180000000'
         DC    XL4'00000000',A(X'00000BAD')


DEADBEEF DC    XL8'0002000180000000'
         DC    XL4'00000000',A(X'DEADBEEF')




TESTVALS EQU   *
ZEROS    DC    XL8'0000000000000000'
ONES     DC    XL8'1111111111111111'
HEXFFS   DC    XL8'FFFFFFFFFFFFFFFF'


LZRB64   DC    XL8'1111111111111100'
LZRB64L  DC    XL8'0000000011111100'
LZRB32   DC    XL8'0000000011111100'
LOC32H   DC    XL8'FFFFFFFF00000000'
LOC32    DC    XL8'00000000FFFFFFFF'
LOC64    DC    XL8'FFFFFFFFFFFFFFFF'
LOC3211S DC    XL8'1111111100000000'
STOCFH   DC    XL8'FFFFFFFFFFFFFFFF'
STOCFH1F DC    XL8'11111111FFFFFFFF'



         END
