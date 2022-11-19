 TITLE '            TRTR-01-basic (Test TRTR instructions)'
***********************************************************************
*
*            TRTR instruction tests
*
*        NOTE: This test is based the CLCL-et-al Test
*              modified to only test the TRTR instruction.
*
*        James Wekel November 2022
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*            TRTR basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the TRTR
*  instructions. Specification exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of the TRTR instruction.
*
***********************************************************************
*
*  Example Hercules Testcase:
*
*        *Testcase TRTR-01-basic (Test TRTR instructions)
*
*        # ------------------------------------------------------------
*        # This tests only the basic function of the TRTR instruction.
*        # Specification Exceptions are NOT tested.
*        # ------------------------------------------------------------
*        mainsize    16
*        numcpu      1
*        sysclear
*        archlvl     z/Arch
*
*        loadcore    "$(testpath)/TRTR-01-basic" 0x0
*
*        runtest     1
*
*        *Done
*
***********************************************************************
                                                                EJECT
***********************************************************************
*               Low Core Definitions
***********************************************************************
*
TRTR1TST START 0
         USING TRTR1TST,R0            Low core addressability
                                                                SPACE 2
         ORG   TRTR1TST+X'1A0'        z/Architecure RESTART PSW
         DC    X'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE 2
         ORG   TRTR1TST+X'1D0'        z/Architecure PROGRAM CHECK PSW
         DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                SPACE 3
         ORG   TRTR1TST+X'200'        Start of actual test program...
                                                                EJECT
***********************************************************************
*               The actual "TRTR1TST" program itself...
***********************************************************************
*
*  Architecture Mode: z/Arch
*  Register Usage:
*
*   R0       (work)
*   R1       TRTR - Function-Code Address
*   R2       TRTR - Function-Code
*   R3       TRTR - First-Operand Address
*   R4       TRTR - First-Operand Length
*   R5       TRTR - Function-Code Table Address
*   R6-R7    (work)
*   R8       First base register
*   R9       Second base register
*   R10-R12  (work)
*   R13      Testing control table - base current entry
*   R14      Subroutine call
*   R15      Secondary Subroutine call or work
*
***********************************************************************
                                                                SPACE
         USING  BEGIN,R8        FIRST Base Register
         USING  BEGIN+4096,R9   SECOND Base Register
                                                                SPACE
BEGIN    BALR  R8,0             Initalize FIRST base register
         BCTR  R8,0             Initalize FIRST base register
         BCTR  R8,0             Initalize FIRST base register
                                                                SPACE
         LA    R9,2048(,R8)     Initalize SECOND base register
         LA    R9,2048(,R9)     Initalize SECOND base register
*
**       Run the tests...
*
         BAL   R14,TEST01       Test TRTR   instruction
*
                                                                SPACE 4
***********************************************************************
*         Test for normal or unexpected test completion...
***********************************************************************
                                                                SPACE
         CLI   TESTNUM,X'26'    Did we end on expected test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         CLI   SUBTEST,X'02'    Did we end on expected SUB-test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         B     EOJ              Yes, then normal completion!
                                                                EJECT
***********************************************************************
*        Fixed test storage locations ...
***********************************************************************
                                                                SPACE 2
         ORG   BEGIN+X'200'

TESTADDR DS    0D                Where test/subtest numbers will go
TESTNUM  DC    X'99'      Test number of active test
SUBTEST  DC    X'99'      Active test sub-test number
                                                                SPACE 2
         ORG   *+X'100'
                                                                EJECT
***********************************************************************
*        TEST01                   Test TRTR instruction
***********************************************************************
                                                                SPACE
TEST01   MVI   TESTNUM,X'01'

         LA    R13,TRTRCTL          Point R6 --> testing control table
         USING TRTRTEST,R13         What each table entry looks like

TST1LOOP EQU   *
         IC    R6,TNUM            Set test number
         STC   R6,TESTNUM
*
**       Initialize operand data  (move data to testing address)
*
         L     R10,OP1WHERE         Where to move operand-1 data to
         L     R11,OP1LEN           operand-1 length
         ST    R11,OP1WLEN            and save for later
         L     R6,OP1DATA           Where op1 data is right now
         L     R7,OP1LEN            How much of it there is
         MVCL  R10,R6
*
         L     R10,OP2WHERE         Where to move operand-2 data to
         L     R11,OP2LEN           How much of it there is
         L     R6,OP2DATA           Where op2 data is right now
         L     R7,OP2LEN             How much of it there is
         MVCL  R10,R6
                                                                SPACE 3
* Setup for TRTR instruction: adjust OP address
                                                                SPACE 1
         L     R11,FAILMASK       (failure CC)
         SLL   R11,4              (shift to BC instr CC position)
                                                                SPACE 1
         MVI   SUBTEST,X'00'      (primary TRTR)
                                                                SPACE 2
         LM    R1,R5,OPSWHERE     get TRTR input; set OP addr to end
         AR    R3,R4              add OP length -1
         BCTR  R3,0
                                                                EJECT
* Execute TRTR instruction and check for expected condition code
                                                                SPACE 2
         LR    R6,R4             get op-1 length -1 for EX
         BCTR  R6,0
         EX    R6,TRTREX          'TRTR  0(0,R3),0(R5)'
                                                                SPACE 2
         EX    R11,TRTRBC         fail if...
                                                                SPACE 2
**       Verify R1,R2 contain (or still contain!) expected values
                                                                SPACE 1
         LM    R10,R12,ENDREGS
                                                                SPACE 1
         MVI   SUBTEST,X'01'      (R2 result - op1 found addr)
         CLR   R1,R10              R2 correct?
         BNE   TRTRFAIL           No, FAILTEST!
                                                                SPACE 1
         MVI   SUBTEST,X'02'      (R3 result - op1 remaining len)
         CLR   R2,R11              R3 correct
         BNE   TRTRFAIL           No, FAILTEST!
                                                                SPACE 1
         LA    R13,TRTRNEXT        Go on to next table entry
         CLC   =F'0',0(R13)        End of table?
         BNE   TST1LOOP           No, loop...
         B     TRTRDONE            Done! (success!)
                                                                SPACE 2
TRTRFAIL LA    R14,FAILTEST       Unexpected results!
TRTRDONE BR    R14                Return to caller or FAILTEST
                                                                SPACE 2
TRTRBC   BC    0,TRTRFAIL          (fail if unexpected condition code)
                                                                SPACE 2
TRTREX   TRTR  0(0,R3),0(R5)

         DROP  R13
         DROP  R15
         USING BEGIN,R8
                                                                EJECT
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 3
EOJPSW   DC    0D'0',X'0002000180000000',AD(0)
                                                                SPACE
EOJ      LPSWE EOJPSW               Normal completion
                                                                SPACE 5
FAILPSW  DC    0D'0',X'0002000180000000',AD(X'BAD')
                                                                SPACE
FAILTEST LPSWE FAILPSW              Abnormal termination
                                                                SPACE 7
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE 2
         LTORG ,                Literals pool
                                                                SPACE
K        EQU   1024             One KB
PAGE     EQU   (4*K)            Size of one page
K16      EQU   (16*K)           16 KB
K32      EQU   (32*K)           32 KB
K64      EQU   (64*K)           64 KB
MB       EQU   (K*K)             1 MB
                                                                EJECT
TRTR1TST CSECT ,
                                                                SPACE 2
***********************************************************************
*        TRTRTEST DSECT
***********************************************************************
                                                                SPACE 2
TRTRTEST DSECT ,
TNUM     DC    X'00'          TRTR table Number
         DC    X'00'
         DC    X'00'
         DC    X'00'
                                                                SPACE 3
OP1DATA  DC    A(0)           Pointer to Operand-1 data
OP1LEN   DC    F'0'           How much data is there - 1
OP2DATA  DC    A(0)           Pointer to FC table data
OP2LEN   DC    F'0'           How much data is there - FC Table
                                                                SPACE 2
OPSWHERE EQU   *
GR1PATT  DC    A(0)           GR1 - Polluted Register pattern
GR2PATT  DC    A(0)           GR2 - Polluted Register pattern
OP1WHERE DC    A(0)           Where Operand-1 data should be placed
OP1WLEN  DC    F'0'           How much data is there - 1
OP2WHERE DC    A(0)           Where FC Table  data should be placed
                                                                SPACE 2
FAILMASK DC    A(0)           Failure Branch on Condition mask
                                                                SPACE 2
*                             Ending register values
ENDREGS  DC    A(0)              GR1 - FC address
         DC    A(0)              GR2 - Function Code
                                                                SPACE 2
TRTRNEXT EQU   *              Start of next table entry...
                                                                SPACE 6
REG2PATT EQU   X'AABBCCDD'    Polluted Register pattern
REG2LOW  EQU         X'DD'    (last byte above)
                                                                EJECT
TRTR1TST CSECT ,
                                                                SPACE 2
***********************************************************************
*        TRTR Testing Control tables   (ref: TRTRTEST DSECT)
***********************************************************************
         PRINT DATA
TRTRCTL  DC    0A(0)    start of table
                                                                SPACE 2
***********************************************************************
*        tests with CC=0
***********************************************************************
                                                                SPACE 2
CC0T1    DS    0F
         DC    X'01'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP10),A(001)           Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(1*MB+(0*K16)),A(0),A(2*MB+(0*K16))  Op1, Op1L, FCT
*
         DC    A(7)                         not CC0
         DC    A(REG2PATT),A(REG2PATT)      FC address, Code
                                                                SPACE 2
CC0T2    DS    0F
         DC    X'02'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP10),A(004)           Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(1*MB+(1*K16)),A(0),A(2*MB+(1*K16))  Op1, Op1L, FCT
*
         DC    A(7)                         not CC0
         DC    A(REG2PATT),A(REG2PATT)      FC address, Code
                                                                SPACE 2
CC0T3    DS    0F
         DC    X'03'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP10),A(064)           Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(1*MB+(2*K16)),A(0),A(2*MB+(2*K16))  Op1, Op1L, FCT
*
         DC    A(7)                         not CC0
         DC    A(REG2PATT),A(REG2PATT)      FC address, Code
                                                                SPACE 2
CC0T4    DS    0F
         DC    X'04'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP10),A(256)           Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(1*MB+(3*K16)),A(0),A(2*MB+(3*K16))  Op1, Op1L, FCT
*
         DC    A(7)                         not CC0
         DC    A(REG2PATT),A(REG2PATT)      FC address, Code
                                                                SPACE 2
                                                                EJECT
***********************************************************************
*        tests with CC=1
***********************************************************************
                                                                SPACE 2
CC1T1    DS    0F
         DC    X'11'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP1F0),A(004)          Source - Op 1 & length
         DC    A(TRTOP2F0),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(3*MB+(0*K16)),A(0),A(4*MB+(0*K16))  Op1, Op1L, FCT
*
         DC    A(11)                        not CC1
         DC    A(3*MB+(0*K16)+1),XL4'AABBCCF0' FC address, Code
                                                                SPACE 2
CC1T2    DS    0F
         DC    X'12'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP1F0),A(016)          Source - Op 1 & length
         DC    A(TRTOP2F0),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(3*MB+(1*K16)),A(0),A(4*MB+(1*K16))  Op1, Op1L, FCT
*
         DC    A(11)                        not CC1
         DC    A(3*MB+(1*K16)+1),XL4'AABBCCF0' FC address, Code
                                                                SPACE 2
CC1T3    DS    0F
         DC    X'13'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP1F0),A(256)          Source - Op 1 & length
         DC    A(TRTOP2F0),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(3*MB+(2*K16)),A(0),A(4*MB+(2*K16))  Op1, Op1L, FCT
*
         DC    A(11)                        not CC1
         DC    A(3*MB+(2*K16)+1),XL4'AABBCCF0' FC address, Code
                                                                SPACE 2
*        cross page tests
                                                                SPACE 2
CC1T4    DS    0F
         DC    X'14'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP1F0),A(016)          Source - Op 1 & length
         DC    A(TRTOP2F0),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(3*MB+(3*K16)-9),A(0),A(4*MB+(3*K16)-9)  Op1, Op1L, FCT
*
         DC    A(11)                        not CC1
         DC    A(3*MB+(3*K16)-9+1),XL4'AABBCCF0' FC address, Code
                                                                SPACE 2
CC1T5    DS    0F
         DC    X'15'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP1F0),A(256)          Source - Op 1 & length
         DC    A(TRTOP2F0),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(3*MB+(4*K16)-13),A(0),A(4*MB+(4*K16)-29)  Op1,.., FCT
*
         DC    A(11)                        not CC1
         DC    A(3*MB+(4*K16)-13+1),XL4'AABBCCF0' FC address, Code
                                                                SPACE 2
                                                                EJECT
***********************************************************************
*        tests with CC=2
***********************************************************************
                                                                SPACE 2
CC2T1    DS    0F
         DC    X'21'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP1F1),A(004)          Source - Op 1 & length
         DC    A(TRTOP8F1),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(5*MB+(0*K16)),A(0),A(6*MB+(0*K16))  Op1, Op1L, FCT
*
         DC    A(13)                        not CC2
         DC    A(5*MB+(0*K16)),XL4'AABBCCF1' FC address, Code
                                                                SPACE 2
CC2T2    DS    0F
         DC    X'22'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP1F1),A(016)          Source - Op 1 & length
         DC    A(TRTOP8F1),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(5*MB+(1*K16)),A(0),A(6*MB+(1*K16))  Op1, Op1L, FCT
*
         DC    A(13)                        not CC2
         DC    A(5*MB+(1*K16)),XL4'AABBCCF1' FC address, Code
                                                                SPACE 2
CC2T3    DS    0F
         DC    X'23'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP1F1),A(256)          Source - Op 1 & length
         DC    A(TRTOP8F1),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(5*MB+(2*K16)),A(0),A(6*MB+(2*K16))  Op1, Op1L, FCT
*
         DC    A(13)                        not CC2
         DC    A(5*MB+(2*K16)),XL4'AABBCCF1' FC address, Code
                                                                SPACE 2
*        cross page tests
                                                                SPACE 2
CC2T4    DS    0F
         DC    X'24'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP1F1),A(016)          Source - Op 1 & length
         DC    A(TRTOP8F1),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(5*MB+(3*K16)-9),A(0),A(6*MB+(3*K16)-9)  Op1, Op1L, FCT
*
         DC    A(13)                        not CC2
         DC    A(5*MB+(3*K16)-9),XL4'AABBCCF1' FC address, Code
                                                                SPACE 2
CC2T5    DS    0F
         DC    X'25'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP1F1),A(256)          Source - Op 1 & length
         DC    A(TRTOP8F1),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(5*MB+(4*K16)-13),A(0),A(6*MB+(4*K16)-29)  Op1,.., FCT
*
         DC    A(13)                        not CC2
         DC    A(5*MB+(4*K16)-13),XL4'AABBCCF1' FC address, Code
                                                                SPACE 2
CC2T6    DS    0F
         DC    X'26'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP1F1),A(256)          Source - Op 1 & length
         DC    A(TRTOP8F1),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(5*MB+(5*K16)-1),A(0),A(6*MB+(5*K16)+29)  Op1,.., FCT
*
         DC    A(13)                        not CC2
         DC    A(5*MB+(5*K16)-1),XL4'AABBCCF1' FC address, Code
                                                                SPACE 2
         DC    A(0)     end of table
         DC    A(0)     end of table
                                                                SPACE 2
         PRINT NODATA
                                                                EJECT
***********************************************************************
*        TRTR op1 scan data...
***********************************************************************
                                                                SPACE 2
TRTOP10  DC    64XL4'78125634'    (CC0)
                                                                SPACE
TRTOP1F0 DC    X'00F00000',63XL4'78125634'    (CC1)
                                                                SPACE
TRTOP1F1 DC    X'F1000000',63XL4'78125634'    (CC2)
                                                                SPACE 2
***********************************************************************
*        Function Code (FC) Tables
***********************************************************************
                                                                SPACE 2
TRTOP20  DC    256X'00'            no stop
         DS    D
                                                                SPACE
TRTOP2F0 DC    240X'00',X'F0',15X'00'     stop on X'F0'
         DS    D
                                                                SPACE
TRTOP8F1 DC    240X'00',X'00',X'F1',14X'00'     stop on X'F1'
         DS    D
                                                                SPACE 5
***********************************************************************
*        Register equates
***********************************************************************
                                                                SPACE 2
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
                                                                SPACE 8
         END
