 TITLE '            TRTE-01-basic (Test TRTE instructions)'
***********************************************************************
*
*                   TRTE basic instruction tests
*
***********************************************************************
*
*  This program tests proper functioning of the TRTE instructions.
*  Specification Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of the instruction.
*
*  NOTE: This test is based on the CLCL-et-al Test but modified to
*        only test the TRTE instruction.  --  James Wekel October 2022
*
***********************************************************************
*
*  Example Hercules Testcase:
*
*
*      *Testcase TRTE-01-basic (Test TRTE instructions)
*
*      # ------------------------------------------------------------
*      #  This tests only the basic function of the TRTE instruction.
*      #  Specification Exceptions are NOT tested.
*      # ------------------------------------------------------------
*
*      mainsize    16
*      numcpu      1
*      sysclear
*      archlvl     z/Arch
*      loadcore    "$(testpath)/TRTE-01-basic.core" 0x0
*      runtest     1
*      *Done
*
*
***********************************************************************
                                                                SPACE 2
TRTE1TST START 0
         USING TRTE1TST,R0            Low core addressability
                                                                SPACE 2
         ORG   TRTE1TST+X'1A0'        z/Architecure RESTART PSW
         DC    X'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE 2
         ORG   TRTE1TST+X'1D0'        z/Architecure PROGRAM CHECK PSW
         DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                SPACE 3
         ORG   TRTE1TST+X'200'        Start of actual test program...
                                                                EJECT
***********************************************************************
*               The actual "TRTE1TST" program itself...
***********************************************************************
*
*  Architecture Mode: z/Arch
*  Register Usage:
*
*   R0       (work)
*   R1       TRTE - Function-Code Table Address
*   R2       TRTE - First-Operand Address
*   R3       TRTE - First-Operand Length
*   R4       TRTE - Function-Code
*   R5       Testing control table - base current entry
*   R6-R7    (work)
*   R8       First base register
*   R9       Second base register
*   R10-R13  (work)
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
                                                                SPACE 2
***********************************************************************
*        Run the test(s)...
***********************************************************************
                                                                SPACE
         BAL   R14,TEST01       Test TRTE instruction
                                                                SPACE 2
***********************************************************************
*         Test for normal or unexpected test completion...
***********************************************************************
                                                                SPACE
         CLI   TESTNUM,X'FC'    Did we end on expected test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         CLI   SUBTEST,X'03'    Did we end on expected SUB-test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         B     EOJ              Yes, then normal completion!
                                                                EJECT
***********************************************************************
*        Fixed test storage locations ...
***********************************************************************
                                                                SPACE 2
         ORG   TRTE1TST+X'400'
                                                                SPACE 4
TESTADDR DS    0D               Where test/subtest numbers will go
TESTNUM  DC    X'99'            Test number of active test
SUBTEST  DC    X'99'            Active test sub-test number
                                                                SPACE 4
         ORG   *+X'100'
                                                                EJECT
***********************************************************************
*        TEST01                   Test TRTE instruction
***********************************************************************
                                                                SPACE
TEST01   MVI   TESTNUM,X'01'
                                                                SPACE
         LA    R5,TRTECTL         Point R5 --> testing control table
         USING TRTETEST,R5        What each table entry looks like
                                                                SPACE
TST1LOOP EQU   *
         IC    R6,TNUM            Set test number
         STC   R6,TESTNUM
*
**       Initialize operand data  (move data to testing address)
*
         L     R10,OP1WHERE       Where to move operand-1 data to
         L     R11,OP1LEN         Get operand-1 length
         ST    R11,OP1WLEN        and save for later
         L     R6,OP1DATA         Where op1 data is right now
         L     R7,OP1LEN          How much of it there is
         MVCL  R10,R6
*
         L     R10,OP2WHERE       Where to move operand-2 data to
         L     R11,OP2LEN         How much of it there is
         L     R6,OP2DATA         Where op2 data is right now
         L     R7,OP2LEN          How much of it there is
         MVCL  R10,R6
                                                                SPACE 3
*
**       Execute TRTE instruction and check for expected condition code
*
         LM    R1,R4,OPSWHERE     get TRTE input
                                                                SPACE
         SR    R7,R7              get M3 bits for TRTE
         IC    R7,M3              (M3)
         STC   R7,TRTEMOD+2       DYNAMICALLY MODIFIED CODE
                                                                SPACE
         L     R11,FAILMASK       (failure CC)
         SLL   R11,4              (shift to BC instr CC position)
                                                                SPACE
         MVI   SUBTEST,X'00'      (primary TRT)
TRTEMOD  TRTE  R2,R4,0            Start with TRTE and m3=0
                                                                SPACE
         STM   R1,R4,SAVETRT      (save R1/R4 results)
         EX    R11,TRTEBC         fail if...
         BC    B'0001',TRTEMOD    cc=3, not finished
                                                                EJECT
*
**       Verify R2,R3,R4 contain (or still contain!) expected values
*
         LM    R10,R12,ENDREGS
                                                                SPACE
         MVI   SUBTEST,X'01'      (R2 result - op1 found addr)
         CLR   R2,R10             R2 correct?
         BNE   TRTEFAIL           No, FAILTEST!
                                                                SPACE
         MVI   SUBTEST,X'02'      (R3 result - op1 remaining len)
         CLR   R3,R11             R3 correct
         BNE   TRTEFAIL           No, FAILTEST!
                                                                SPACE
         MVI   SUBTEST,X'03'      (R4 result - FC code)
         CLR   R4,R12             R4 correct
         BNE   TRTEFAIL           No, FAILTEST!
                                                                SPACE
         LA    R5,TRTENEXT        Go on to next table entry
         CLC   =F'0',0(R5)        End of table?
         BNE   TST1LOOP           No, loop...
         B     TRTEDONE           Done! (success!)
                                                                SPACE 2
TRTEFAIL LA    R14,FAILTEST       Unexpected results!
TRTEDONE BR    R14                Return to caller or FAILTEST
                                                                SPACE 2
TRTEBC   BC    0,TRTEFAIL         (fail if unexpected condition code)
                                                                SPACE 2
SAVETRT  DC    4D'0'              (saved R1/R4 from TRT results)
                                                                SPACE 2
         DROP  R5
         DROP  R15
         USING BEGIN,R8
                                                                EJECT
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 5
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
                                                                SPACE 3
K        EQU   1024             One KB
PAGE     EQU   (4*K)            Size of one page
K64      EQU   (64*K)           64 KB
MB       EQU   (K*K)             1 MB
                                                                EJECT
TRTE1TST CSECT ,
                                                                SPACE 2
***********************************************************************
*        TRTETEST DSECT
***********************************************************************
                                                                SPACE 2
TRTETEST DSECT ,
TNUM     DC    X'00'          TRTE table Number
         DC    X'00'
         DC    X'00'
M3       DC    X'00'          M3 byte stored into TRTE instruction
                                                                SPACE 3
OP1DATA  DC    A(0)           Pointer to Operand-1 data
OP1LEN   DC    F'0'           How much data is there - 1
OP2DATA  DC    A(0)           Pointer to FC table data
OP2LEN   DC    F'0'           How much data is there - FC Table
                                                                SPACE 2
OPSWHERE EQU   *
OP2WHERE DC    A(0)           Where FC Table  data should be placed
OP1WHERE DC    A(0)           Where Operand-1 data should be placed
OP1WLEN  DC    F'0'           How much data is there - 1
         DC    A(0)           pollute - found FC
                                                                SPACE 2
FAILMASK DC    A(0)           Failure Branch on Condition mask
                                                                SPACE 2
*                             Ending register values
ENDREGS  DC    A(0)              Operand 1 address
         DC    A(0)              Operand 1 length
         DC    A(0)              Function Code
                                                                SPACE 2
TRTENEXT EQU   *              Start of next table entry...
                                                                SPACE 6
REG2PATT EQU   X'AABBCCDD'    Polluted Register pattern
REG2LOW  EQU         X'DD'    (last byte above)
                                                                EJECT
***********************************************************************
*        TRTE Testing Control tables   (ref: TRTETEST DSECT)
***********************************************************************
                                                                SPACE
TRTE1TST CSECT ,
         PRINT DATA
TRTECTL  DC    0A(0)    start of table
                                                                SPACE
***********************************************************************
*        tests with   M3: A=0,F=0,L=0, reserved=0    (0)
*                            FC Table = 1 byte
***********************************************************************
                                                                SPACE
M0T1     DS    0F
         DC    X'01'                       Test Num
         DC    X'00',X'00'
         DC    X'00'                       M3: A=0,F=0,L=0
         DC    A(TRTOP10),A(001)           Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(1*MB+(1*K64)),A(2*MB+(1*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(2*MB+(1*K64)+001),A(000),A(0)
                                                                SPACE
M0T2     DS    0F
         DC    X'02'                       Test Num
         DC    X'00',X'00'
         DC    X'00'                       M3: A=0,F=0,L=0
         DC    A(TRTOP10),A(002)           Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(MB+(2*K64)),A(2*MB+(2*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(2*MB+(2*K64)+002),A(000),A(0)
                                                                SPACE
M0T3     DS    0F
         DC    X'03'                       Test Num
         DC    X'00',X'00'
         DC    X'00'                       M3: A=0,F=0,L=0
         DC    A(TRTOP10),A(004)           Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(MB+(3*K64)),A(2*MB+(3*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(2*MB+(3*K64)+004),A(000),A(0)
                                                                EJECT
M0T4     DS    0F
         DC    X'04'                       Test Num
         DC    X'00',X'00'
         DC    X'00'                       M3: A=0,F=0,L=0
         DC    A(TRTOP10),A(008)           Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(MB+(4*K64)),A(2*MB+(4*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(2*MB+(4*K64)+008),A(000),A(0)
                                                                SPACE 4
M0T5     DS    0F
         DC    X'05'                       Test Num
         DC    X'00',X'00'
         DC    X'00'                       M3: A=0,F=0,L=0
         DC    A(TRTOP10),A(256)           Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(MB+(5*K64)),A(2*MB+(5*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(2*MB+(5*K64)+256),A(000),A(0)
                                                                SPACE 4
M0T6     DS    0F
         DC    X'06'                       Test Num
         DC    X'00',X'00'
         DC    X'00'                       M3: A=0,F=0,L=0
         DC    A(TRTOP111),A(256)          Source - Op 1 & length
         DC    A(TRTOP211),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(MB+(6*K64)-32),A(2*MB+(6*K64)-12),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(2*MB+(6*K64)-12+X'11'),A(256-X'11'),XL4'11'
                                                                EJECT
M0T7     DS    0F
         DC    X'07'                       Test Num
         DC    X'00',X'00'
         DC    X'00'                       M3: A=0,F=0,L=0
         DC    A(TRTOP1F0),A(256)          Source - Op 1 & length
         DC    A(TRTOP2F0),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(MB+(7*K64)),A(2*MB+(7*K64)-12),A(0)   FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(2*MB+(7*K64)-12+255),A(256-255),XL4'F0'
                                                                SPACE 4
M0T8     DS    0F
         DC    X'08'                       Test Num
         DC    X'00',X'00'
         DC    X'00'                       M3: A=0,F=0,L=0
         DC    A(TRTOP111),A(256)          Source - Op 1 & length
         DC    A(TRTOP211),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(MB+(8*K64)-32),A(2*MB+(8*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(2*MB+(8*K64)+X'11'),A(256-X'11'),XL4'11'
                                                                SPACE 4
M0T9     DS    0F
         DC    X'09'                       Test Num
         DC    X'00',X'00'
         DC    X'00'                       M3: A=0,F=0,L=0
         DC    A(TRTO1L0),A(2048)          Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(MB+(9*K64)),A(2*MB+(9*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(2*MB+(9*K64)+2048),A(000),A(0)
                                                                EJECT
M0T10    DS    0F
         DC    X'0A'                       Test Num
         DC    X'00',X'00'
         DC    X'00'                       M3: A=0,F=0,L=0
         DC    A(TRTO1L11),A(2048)         Source - Op 1 & length
         DC    A(TRTOP211),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(MB+(10*K64)),A(2*MB+(10*K64)-200),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(2*MB+(10*K64)-200+(4*256)+1),A(1023),Xl4'11'
                                                                SPACE 4
M0T11    DS    0F
         DC    X'0B'                       Test Num
         DC    X'00',X'00'
         DC    X'00'                       M3: A=0,F=0,L=0
         DC    A(TRTO1LF0),A(2048)         Source - Op 1 & length
         DC    A(TRTOP2F0),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(MB+(11*K64)-64),A(2*MB+(11*K64)),A(0) FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(2*MB+(11*K64)+2048-1),A(1),Xl4'F0'
                                                                EJECT
***********************************************************************
*        tests with   M3: A=0,F=1,L=0, reserved=0    (4)
*                            FC Table = 2 bytes
***********************************************************************
                                                                SPACE 2
M4T1     DS    0F
         DC    X'41'                       Test Num
         DC    X'00',X'00'
         DC    X'40'                       M3: A=0,F=1,L=0
         DC    A(TRTOP10),A(001)           Source - Op 1 & length
         DC    A(TRTOP20),A(512)           Source - FC Table & length
*                                          Target -
         DC    A(3*MB+(1*K64)),A(4*MB+(1*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(4*MB+(1*K64)+001),A(000),A(0)
                                                                SPACE 4
M4T2     DS    0F
         DC    X'42'                       Test Num
         DC    X'00',X'00'
         DC    X'40'                       M3: A=0,F=1,L=0
         DC    A(TRTOP10),A(002)           Source - Op 1 & length
         DC    A(TRTOP20),A(512)           Source - FC Table & length
*                                          Target -
         DC    A(3*MB+(2*K64)),A(4*MB+(2*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(4*MB+(2*K64)+002),A(000),A(0)
                                                                SPACE 4
M4T3     DS    0F
         DC    X'43'                       Test Num
         DC    X'00',X'00'
         DC    X'40'                       M3: A=0,F=1,L=0
         DC    A(TRTOP10),A(004)           Source - Op 1 & length
         DC    A(TRTOP20),A(512)           Source - FC Table & length
*                                          Target -
         DC    A(3*MB+(4*K64)),A(4*MB+(4*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(4*MB+(4*K64)+004),A(000),A(0)
                                                                EJECT
M4T4     DS    0F
         DC    X'44'                       Test Num
         DC    X'00',X'00'
         DC    X'40'                       M3: A=0,F=1,L=0
         DC    A(TRTOP10),A(008)           Source - Op 1 & length
         DC    A(TRTOP20),A(512)           Source - FC Table & length
*                                          Target -
         DC    A(3*MB+(4*K64)),A(4*MB+(4*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(4*MB+(4*K64)+008),A(000),A(0)
                                                                SPACE 4
M4T5     DS    0F
         DC    X'45'                       Test Num
         DC    X'00',X'00'
         DC    X'40'                       M3: A=0,F=1,L=0
         DC    A(TRTOP10),A(256)           Source - Op 1 & length
         DC    A(TRTOP20),A(512)           Source - FC Table & length
*                                          Target -
         DC    A(3*MB+(5*K64)),A(4*MB+(5*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(4*MB+(5*K64)+256),A(000),A(0)
                                                                SPACE 4
M4T6     DS    0F
         DC    X'46'                       Test Num
         DC    X'00',X'00'
         DC    X'40'                       M3: A=0,F=1,L=0
         DC    A(TRTOP111),A(256)          Source - Op 1 & length
         DC    A(TRTOP411),A(512)          Source - FC Table & length
*                                          Target -
         DC    A(3*MB+(6*K64)-32),A(4*MB+(6*K64)-12),A(0) FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(4*MB+(6*K64)-12+X'11'),A(256-X'11'),XL4'11'
                                                                EJECT
M4T7     DS    0F
         DC    X'47'                       Test Num
         DC    X'00',X'00'
         DC    X'40'                       M3: A=0,F=1,L=0
         DC    A(TRTOP1F0),A(256)          Source - Op 1 & length
         DC    A(TRTOP4F0),A(512)          Source - FC Table & length
*                                          Target -
         DC    A(3*MB+(7*K64)),A(4*MB+(7*K64)-12),A(0) FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(4*MB+(7*K64)-12+255),A(256-255),XL4'F0'
                                                                SPACE 4
M4T8     DS    0F
         DC    X'48'                       Test Num
         DC    X'00',X'00'
         DC    X'40'                       M3: A=0,F=1,L=0
         DC    A(TRTOP111),A(256)          Source - Op 1 & length
         DC    A(TRTOP411),A(512)          Source - FC Table & length
*                                          Target -
         DC    A(3*MB+(8*K64)-32),A(4*MB+(8*K64)),A(0) FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(4*MB+(8*K64)+X'11'),A(256-X'11'),XL4'11'
                                                                SPACE 4
M4T9     DS    0F
         DC    X'49'                       Test Num
         DC    X'00',X'00'
         DC    X'40'                       M3: A=0,F=1,L=0
         DC    A(TRTO1L0),A(2048)          Source - Op 1 & length
         DC    A(TRTOP20),A(512)           Source - FC Table & length
*                                          Target -
         DC    A(3*MB+(9*K64)),A(4*MB+(9*K64)),A(0) FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(4*MB+(9*K64)+2048),A(000),A(0)
                                                                EJECT
M4T10    DS    0F
         DC    X'4A'                       Test Num
         DC    X'00',X'00'
         DC    X'40'                       M3: A=0,F=1,L=0
         DC    A(TRTO1L11),A(2048)         Source - Op 1 & length
         DC    A(TRTOP411),A(512)          Source - FC Table & length
*                                          Target -
         DC    A(3*MB+(10*K64)),A(4*MB+(10*K64)-200),A(0) FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(4*MB+(10*K64)-200+(4*256)+1),A(1023),XL4'11'
                                                                SPACE 4
M4T11    DS    0F
         DC    X'4B'                       Test Num
         DC    X'00',X'00'
         DC    X'40'                       M3: A=0,F=1,L=0
         DC    A(TRTO1LF0),A(2048)         Source - Op 1 & length
         DC    A(TRTOP4F0),A(512)          Source - FC Table & length
*                                          Target -
         DC    A(3*MB+(11*K64)-64),A(4*MB+(11*K64)),A(0) FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(4*MB+(11*K64)+2048-1),A(1),XL4'F0'
                                                                EJECT
***********************************************************************
*        tests with   M3: A=1,F=0,L=0, reserved=0    (8)
*                     FC Table : SIZE: 65,536 (2 BYTE ARGUMENT)
*
*              Note: Op1 length must be a multiple of 2
***********************************************************************
                                                                SPACE 2
M8T1     DS    0F
         DC    X'81'                       Test Num
         DC    X'00',X'00'
         DC    X'80'                       M3: A=1,F=0,L=0,--=0
         DC    A(TRTOP10),A(002)           Source - Op 1 & length
         DC    A(TRTOP20),A(K64)           Source - FC Table & length
*                                          Target -
         DC    A(5*MB+(1*K64)),A(6*MB+(1*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(6*MB+(1*K64)+002),A(000),A(0)
                                                                SPACE 4
M8T2     DS    0F
         DC    X'82'                       Test Num
         DC    X'00',X'00'
         DC    X'80'                       M3: A=1,F=0,L=0,--=0
         DC    A(TRTOP10),A(004)           Source - Op 1 & length
         DC    A(TRTOP20),A(K64)           Source - FC Table & length
*                                          Target -
         DC    A(5*MB+(2*K64)),A(6*MB+(2*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(6*MB+(2*K64)+004),A(000),A(0)
                                                                SPACE 4
M8T3     DS    0F
         DC    X'83'                       Test Num
         DC    X'00',X'00'
         DC    X'80'                       M3: A=1,F=0,L=0,--=0
         DC    A(TRTOP10),A(008)           Source - Op 1 & length
         DC    A(TRTOP20),A(K64)           Source - FC Table & length
*                                          Target -
         DC    A(5*MB+(3*K64)),A(6*MB+(3*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(6*MB+(3*K64)+008),A(000),A(0)
                                                                EJECT
M8T4     DS    0F
         DC    X'84'                       Test Num
         DC    X'00',X'00'
         DC    X'80'                       M3: A=1,F=0,L=0,--=0
         DC    A(TRTOP10),A(256)           Source - Op 1 & length
         DC    A(TRTOP20),A(K64)           Source - FC Table & length
*                                          Target -
         DC    A(5*MB+(4*K64)),A(6*MB+(4*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(6*MB+(4*K64)+256),A(000),A(0)
                                                                SPACE 4
M8T5     DS    0F
         DC    X'85'                       Test Num
         DC    X'00',X'00'
         DC    X'80'                       M3: A=1,F=0,L=0,--=0
         DC    A(TRTOP111),A(256)          Source - Op 1 & length
         DC    A(TRTOP811),A(K64)          Source - FC Table & length
*                                          Target -
         DC    A(5*MB+(5*K64)),A(6*MB+(5*K64)-12),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(6*MB+(5*K64)-12+X'10'),A(256-X'10'),XL4'11'
                                                                SPACE 4
M8T6     DS    0F
         DC    X'86'                       Test Num
         DC    X'00',X'00'
         DC    X'80'                       M3: A=1,F=0,L=0,--=0
         DC    A(TRTOP1F0),A(256)          Source - Op 1 & length
         DC    A(TRTOP8F0),A(K64)          Source - FC Table & length
*                                          Target -
         DC    A(5*MB+(6*K64)),A(6*MB+(6*K64)-12),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(6*MB+(6*K64)-12+(256-2)),A(2),XL4'F0'
                                                                EJECT
M8T7     DS    0F
         DC    X'87'                       Test Num
         DC    X'00',X'00'
         DC    X'80'                       M3: A=1,F=0,L=0,--=0
         DC    A(TRTOP111),A(256)          Source - Op 1 & length
         DC    A(TRTOP811),A(K64)          Source - FC Table & length
*                                          Target -
         DC    A(5*MB+(8*K64)-32),A(6*MB+(8*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(6*MB+(8*K64)+X'10'),A(256-X'10'),XL4'11'
                                                                SPACE 4
M8T8     DS    0F
         DC    X'88'                       Test Num
         DC    X'00',X'00'
         DC    X'80'                       M3: A=1,F=0,L=0,--=0
         DC    A(TRTOP1F1),A(512)          Source - Op 1 & length
         DC    A(TRTOP8F1),A(K64)          Source - FC Table & length
*                                          Target -
         DC    A(5*MB+(9*K64)-32),A(6*MB+(9*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(6*MB+(9*K64)+510),A(2),XL4'F1'
                                                                SPACE 4
M8T9     DS    0F
         DC    X'89'                       Test Num
         DC    X'00',X'00'
         DC    X'80'                       M3: A=1,F=0,L=0,--=0
         DC    A(TRTO1L0),A(2048)          Source - Op 1 & length
         DC    A(TRTOP20),A(K64)           Source - FC Table & length
*                                          Target -
         DC    A(5*MB+(10*K64)),A(6*MB+(10*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(6*MB+(10*K64)+2048),A(0),XL4'00'
                                                                EJECT
M8T10    DS    0F
         DC    X'8A'                       Test Num
         DC    X'00',X'00'
         DC    X'80'                       M3: A=1,F=0,L=0,--=0
         DC    A(TRTO1L11),A(2048)         Source - Op 1 & length
         DC    A(TRTOP811),A(K64)          Source - FC Table & length
*                                          Target -
         DC    A(5*MB+(12*K64)),A(6*MB+(12*K64)-199),A(0) FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(6*MB+(12*K64)-199+(4*256)),A(1024),XL4'11'
                                                                SPACE 4
M8T11    DS    0F
         DC    X'8B'                       Test Num
         DC    X'00',X'00'
         DC    X'80'                       M3: A=1,F=0,L=0,--=0
         DC    A(TRTO1LF0),A(2048)         Source - Op 1 & length
         DC    A(TRTOP8F0),A(K64)          Source - FC Table & length
*                                          Target -
         DC    A(5*MB+(14*K64)-63),A(6*MB+(14*K64)),A(0) FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(6*MB+(14*K64)+2048-2),A(2),XL4'F0'
                                                                EJECT
***********************************************************************
*        tests with   M3: A=1,F=0,L=1, reserved=0    (10)
*                     FC Table : SIZE: 256 (2 BYTE ARGUMENT)
*                                Function Code is 1 byte
*                                Limit arg to 255
*
*              Note: Op1 length must be a multiple of 2
***********************************************************************
                                                                SPACE 2
M10T1    DS    0F
         DC    X'A1'                       Test Num
         DC    X'00',X'00'
         DC    X'A0'                       M3: A=1,F=0,L=1,--=0
         DC    A(TRTOP10),A(002)           Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(10*MB+(0*K64)),A(11*MB+(0*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(11*MB+(0*K64)+002),A(000),A(0)
                                                                SPACE 3
M10T2    DS    0F
         DC    X'A2'                       Test Num
         DC    X'00',X'00'
         DC    X'A0'                       M3: A=1,F=0,L=1,--=0
         DC    A(TRTOP10),A(004)           Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(10*MB+(1*K64)),A(11*MB+(1*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(11*MB+(1*K64)+004),A(000),A(0)
                                                                SPACE 3
M10T3    DS    0F
         DC    X'A3'                       Test Num
         DC    X'00',X'00'
         DC    X'A0'                       M3: A=1,F=0,L=1,--=0
         DC    A(TRTOP10),A(008)           Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(10*MB+(2*K64)),A(11*MB+(2*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(11*MB+(2*K64)+008),A(000),A(0)
                                                                EJECT
M10T4    DS    0F
         DC    X'A4'                       Test Num
         DC    X'00',X'00'
         DC    X'A0'                       M3: A=1,F=0,L=1,--=0
         DC    A(TRTOP10),A(256)           Source - Op 1 & length
         DC    A(TRTOP20),A(256)           Source - FC Table & length
*                                          Target -
         DC    A(10*MB+(3*K64)),A(11*MB+(3*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(11*MB+(3*K64)+256),A(000),A(0)
                                                                SPACE 4
M10T5    DS    0F
         DC    X'A5'                       Test Num
         DC    X'00',X'00'
         DC    X'A0'                       M3: A=1,F=0,L=1,--=0
         DC    A(TRTOP111),A(256)          Source - Op 1 & length
         DC    A(TRTOP211),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(10*MB+(4*K64)),A(11*MB+(4*K64)-12),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(11*MB+(4*K64)-12+X'10'),A(256-X'10'),XL4'11'
                                                                SPACE 4
M10T6    DS    0F
         DC    X'A6'                       Test Num
         DC    X'00',X'00'
         DC    X'A0'                       M3: A=1,F=0,L=1,--=0
         DC    A(TRTOP1F0),A(256)          Source - Op 1 & length
         DC    A(TRTOP2F0),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(10*MB+(5*K64)),A(11*MB+(5*K64)-12),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(11*MB+(5*K64)-12+(256-2)),A(2),XL4'F0'
                                                                EJECT
M10T7    DS    0F
         DC    X'A7'                       Test Num
         DC    X'00',X'00'
         DC    X'A0'                       M3: A=1,F=0,L=1,--=0
         DC    A(TRTOP111),A(256)          Source - Op 1 & length
         DC    A(TRTOP211),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(10*MB+(6*K64-32)),A(11*MB+(6*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(11*MB+(6*K64)+X'10'),A(256-X'10'),XL4'11'
                                                                SPACE 4
M10T8    DS    0F
         DC    X'A8'                       Test Num
         DC    X'00',X'00'
         DC    X'A0'                       M3: A=1,F=0,L=1,--=0
         DC    A(TRTOP1F1),A(512)          Source - Op 1 & length
         DC    A(TRTOP8F1),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(10*MB+(7*K64)),A(11*MB+(7*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(11*MB+(7*K64)+510),A(2),XL4'F1'
                                                                SPACE 4
M10T9    DS    0F
         DC    X'A9'                       Test Num
         DC    X'00',X'00'
         DC    X'A0'                       M3: A=1,F=0,L=1,--=0
         DC    A(TRTO1L0),A(2048)          Source - Op 1 & length
         DC    A(TRTOP20),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(10*MB+(8*K64)),A(11*MB+(8*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(11*MB+(8*K64)+2048),A(0),XL4'00'
                                                                EJECT
M10T10   DS    0F
         DC    X'AA'                       Test Num
         DC    X'00',X'00'
         DC    X'A0'                       M3: A=1,F=0,L=1,--=0
         DC    A(TRTO1L11),A(2048)         Source - Op 1 & length
         DC    A(TRTOP211),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(10*MB+(9*K64)),A(11*MB+(9*K64)-199),A(0) FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(11*MB+(9*K64)-199+(4*256)),A(1024),XL4'11'
                                                                SPACE 4
M10T11   DS    0F
         DC    X'AB'                       Test Num
         DC    X'00',X'00'
         DC    X'A0'                       M3: A=1,F=0,L=1,--=0
         DC    A(TRTO1LF0),A(2048)         Source - Op 1 & length
         DC    A(TRTOP2F0),A(256)          Source - FC Table & length
*                                          Target - FC, Op1, Op1L
         DC    A(10*MB+(10*K64)-481),A(11*MB+(10*K64)),A(0)
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(11*MB+(10*K64)+2048-2),A(2),XL4'F0'
                                                                EJECT
***********************************************************************
*        tests with   M3: A=1,F=1,L=0, reserved=0    (12)
*                     FC Table : SIZE: 131,072 (2 BYTE ARGUMENT)
*                                Function Code is 2 bytes
*
*              Note: Op1 length must be a multiple of 2
***********************************************************************
                                                                SPACE 2
M12T1    DS    0F
         DC    X'C1'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTOP10),A(002)           Source - Op 1 & length
         DC    A(TRTOP20),A(2*K64)         Source - FC Table & length
*                                          Target -
         DC    A(7*MB+(0*K64)),A(9*MB+(0*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(9*MB+(0*K64)+002),A(000),A(0)
                                                                SPACE 3
M12T2    DS    0F
         DC    X'C2'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTOP10),A(004)           Source - Op 1 & length
         DC    A(TRTOP20),A(2*K64)         Source - FC Table & length
*                                          Target -
         DC    A(7*MB+(2*K64)),A(9*MB+(1*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(9*MB+(1*K64)+004),A(000),A(0)
                                                                SPACE 3
M12T3    DS    0F
         DC    X'C3'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTOP10),A(008)           Source - Op 1 & length
         DC    A(TRTOP20),A(2*K64)         Source - FC Table & length
*                                          Target -
         DC    A(7*MB+(4*K64)),A(9*MB+(2*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(9*MB+(2*K64)+008),A(000),A(0)
                                                                EJECT
M12T4    DS    0F
         DC    X'C4'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTOP10),A(256)           Source - Op 1 & length
         DC    A(TRTOP20),A(2*K64)         Source - FC Table & length
*                                          Target -
         DC    A(7*MB+(6*K64)),A(9*MB+(3*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(9*MB+(3*K64)+256),A(000),A(0)
                                                                SPACE 4
M12T5    DS    0F
         DC    X'C5'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTOP111),A(256)          Source - Op 1 & length
         DC    A(TRTOPC11),A(2*K64)        Source - FC Table & length
*                                          Target -
         DC    A(7*MB+(8*K64)),A(9*MB+(4*K64)-12),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(9*MB+(4*K64)-12+X'10'),A(256-X'10'),XL4'11'
                                                                SPACE 4
M12T6    DS    0F
         DC    X'C6'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTOP1F0),A(256)          Source - Op 1 & length
         DC    A(TRTOPCF0),A(2*K64)        Source - FC Table & length
*                                          Target -
         DC    A(7*MB+(10*K64)),A(9*MB+(5*K64)-12),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(9*MB+(5*K64)-12+(256-2)),A(2),XL4'F0'
                                                                EJECT
M12T7    DS    0F
         DC    X'C7'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTOP111),A(256)          Source - Op 1 & length
         DC    A(TRTOPC11),A(2*K64)        Source - FC Table & length
*                                          Target -
         DC    A(7*MB+(13*K64)-32),A(9*MB+(6*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(9*MB+(6*K64)+X'10'),A(256-X'10'),XL4'11'
                                                                SPACE 4
M12T8    DS    0F
         DC    X'C8'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTOP1F1),A(512)          Source - Op 1 & length
         DC    A(TRTOPCF1),A(2*K64)        Source - FC Table & length
*                                          Target -
         DC    A(7*MB+(15*K64)),A(9*MB+(7*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(9*MB+(7*K64)+510),A(2),XL4'F1'
                                                                SPACE 4
M12T9    DS    0F
         DC    X'C9'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTO1L0),A(2048)          Source - Op 1 & length
         DC    A(TRTOP20),A(2*K64)         Source - FC Table & length
*                                          Target -
         DC    A(7*MB+(17*K64)),A(9*MB+(8*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(9*MB+(8*K64)+2048),A(0),XL4'00'
                                                                EJECT
M12T10   DS    0F
         DC    X'CA'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTO1L11),A(2048)         Source - Op 1 & length
         DC    A(TRTOPC11),A(2*K64)        Source - FC Table & length
*                                          Target -
         DC    A(7*MB+(19*K64)),A(9*MB+(9*K64)-199),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(9*MB+(9*K64)-199+(4*256)),A(1024),XL4'11'
                                                                SPACE 4
M12T11   DS    0F
         DC    X'CB'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTO1LF0),A(2048)         Source - Op 1 & length
         DC    A(TRTOPCF0),A(2*K64)        Source - FC Table & length
*                                          Target -
         DC    A(7*MB+(22*K64)-481),A(9*MB+(10*K64)),A(0) FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(9*MB+(10*K64)+2048-2),A(2),XL4'F0'
                                                                EJECT
***********************************************************************
*        tests with   M3: A=1,F=1,L=1, reserved=0    (14)
*                     FC Table : SIZE: 512 (2 BYTE ARGUMENT)
*                                Function Code is 2 byte
*                                Limit arg to 255
*
*              Note: Op1 length must be a multiple of 2
***********************************************************************
                                                                SPACE 2
M14T1    DS    0F
         DC    X'E1'                       Test Num
         DC    X'00',X'00'
         DC    X'E0'                       M3: A=1,F=1,L=1,--=0
         DC    A(TRTOP10),A(002)           Source - Op 1 & length
         DC    A(TRTOP20),A(512)           Source - FC Table & length
*                                          Target -
         DC    A(11*MB+(0*K64)),A(12*MB+(0*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(12*MB+(0*K64)+002),A(000),A(0)
                                                                SPACE 3
M14T2    DS    0F
         DC    X'E2'                       Test Num
         DC    X'00',X'00'
         DC    X'E0'                       M3: A=1,F=1,L=1,--=0
         DC    A(TRTOP10),A(004)           Source - Op 1 & length
         DC    A(TRTOP20),A(512)           Source - FC Table & length
*                                          Target -
         DC    A(11*MB+(1*K64)),A(12*MB+(1*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(12*MB+(1*K64)+004),A(000),A(0)
                                                                SPACE 3
M14T3    DS    0F
         DC    X'E3'                       Test Num
         DC    X'00',X'00'
         DC    X'E0'                       M3: A=1,F=1,L=1,--=0
         DC    A(TRTOP10),A(008)           Source - Op 1 & length
         DC    A(TRTOP20),A(512)           Source - FC Table & length
*                                          Target -
         DC    A(11*MB+(2*K64)),A(12*MB+(2*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(12*MB+(2*K64)+008),A(000),A(0)
                                                                EJECT
M14T4    DS    0F
         DC    X'E4'                       Test Num
         DC    X'00',X'00'
         DC    X'E0'                       M3: A=1,F=1,L=1,--=0
         DC    A(TRTOP10),A(256)           Source - Op 1 & length
         DC    A(TRTOP20),A(512)           Source - FC Table & length
*                                          Target -
         DC    A(11*MB+(3*K64)),A(12*MB+(3*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(12*MB+(3*K64)+256),A(000),A(0)
                                                                SPACE 4
M14T5    DS    0F
         DC    X'E5'                       Test Num
         DC    X'00',X'00'
         DC    X'E0'                       M3: A=1,F=1,L=1,--=0
         DC    A(TRTOP111),A(256)          Source - Op 1 & length
         DC    A(TRTOP411),A(512)          Source - FC Table & length
*                                          Target -
         DC    A(11*MB+(4*K64)),A(12*MB+(4*K64)-12),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(12*MB+(4*K64)-12+X'10'),A(256-X'10'),XL4'11'
                                                                SPACE 4
M14T6    DS    0F
         DC    X'E6'                       Test Num
         DC    X'00',X'00'
         DC    X'E0'                       M3: A=1,F=1,L=1,--=0
         DC    A(TRTOP1F0),A(256)          Source - Op 1 & length
         DC    A(TRTOP4F0),A(512)          Source - FC Table & length
*                                          Target -
         DC    A(11*MB+(5*K64)),A(12*MB+(5*K64)-12),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(12*MB+(5*K64)-12+254),A(2),XL4'F0'
                                                                EJECT
M14T7    DS    0F
         DC    X'E7'                       Test Num
         DC    X'00',X'00'
         DC    X'E0'                       M3: A=1,F=1,L=1,--=0
         DC    A(TRTOP111),A(256)          Source - Op 1 & length
         DC    A(TRTOP411),A(512)          Source - FC Table & length
*                                          Target -
         DC    A(11*MB+(6*K64)-32),A(12*MB+(6*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(12*MB+(6*K64)+X'10'),A(256-X'10'),XL4'11'
                                                                SPACE 4
M14T8    DS    0F
         DC    X'E8'                       Test Num
         DC    X'00',X'00'
         DC    X'E0'                       M3: A=1,F=1,L=1,--=0
         DC    A(TRTOP1F1),A(512)          Source - Op 1 & length
         DC    A(TRTOPCF1),A(512)          Source - FC Table & length
*                                          Target -
         DC    A(11*MB+(7*K64)),A(12*MB+(7*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(12*MB+(7*K64)+510),A(2),XL4'F1'
                                                                SPACE 4
M14T9    DS    0F
         DC    X'E9'                       Test Num
         DC    X'00',X'00'
         DC    X'E0'                       M3: A=1,F=1,L=1,--=0
         DC    A(TRTO1L0),A(2048)          Source - Op 1 & length
         DC    A(TRTOP20),A(512)           Source - FC Table & length
*                                          Target -
         DC    A(11*MB+(8*K64)),A(12*MB+(8*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(7) CC0
         DC    A(12*MB+(8*K64)+2048),A(000),A(0)
                                                                EJECT
M14T10   DS    0F
         DC    X'EA'                       Test Num
         DC    X'00',X'00'
         DC    X'E0'                       M3: A=1,F=1,L=1,--=0
         DC    A(TRTO1L11),A(2048)         Source - Op 1 & length
         DC    A(TRTOP411),A(512)          Source - FC Table & length
*                                          Target -
         DC    A(11*MB+(9*K64)),A(12*MB+(9*K64)-200),A(0) FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(12*MB+(9*K64)-200+(4*256)),A(1024),XL4'11'
                                                                SPACE 4
M14T11   DS    0F
         DC    X'EB'                       Test Num
         DC    X'00',X'00'
         DC    X'E0'                       M3: A=1,F=1,L=1,--=0
         DC    A(TRTO1LF0),A(2048)         Source - Op 1 & length
         DC    A(TRTOP4F0),A(512)          Source - FC Table & length
*                                          Target -  FC, Op1, Op1L
         DC    A(11*MB+(10*K64)-64),A(12*MB+(10*K64)),A(0)
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(12*MB+(10*K64)+2048-2),A(2),XL4'F0'
                                                                EJECT
***********************************************************************
*        Check performance tests are valid.
*        tests with   M3: A=1,F=1,L=0, reserved=0    (12)
*                     FC Table : SIZE: 131,072 (2 BYTE ARGUMENT)
*                                Function Code is 2 bytes
*
*              Note: Op1 length must be a multiple of 2
***********************************************************************
                                                                SPACE 2
F12T8    DS    0F
         DC    X'F8'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTOP1F1),A(512)          Source - Op 1 & length
         DC    A(TRTOPCF1),A(2*K64)        Source - FC Table & length
*                                          Target -
         DC    A(7*MB+(1*K64)),A(9*MB+(1*K64)),A(0)  FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(9*MB+(1*K64)+510),A(2),XL4'F1'
                                                                SPACE 3
F12T8A   DS    0F
         DC    X'F9'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTOP1F1),A(512)          Source - Op 1 & length
         DC    A(TRTOPCF1),A(2*K64)        Source - FC Table & length
*                                          Target - FC, Op1, Op1L
         DC    A(7*MB+(3*K64)-127),A(9*MB+(3*K64)-127),A(0)
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(9*MB+(3*K64)-127+510),A(2),XL4'F1'
                                                                SPACE 3
F12T11   DS    0F
         DC    X'FB'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTO1LF0),A(2048)         Source - Op 1 & length
         DC    A(TRTOPCF0),A(2*K64)        Source - FC Table & length
*                                          Target -
         DC    A(7*MB+(6*K64)),A(9*MB+(6*K64)),A(0) FC, Op1, Op1L
         DC    A(REG2PATT)
         DC    A(11) CC1
         DC    A(9*MB+(6*K64)+2048-2),A(2),XL4'F0'
                                                                EJECT
F12T11A  DS    0F
         DC    X'FC'                       Test Num
         DC    X'00',X'00'
         DC    X'C0'                       M3: A=1,F=1,L=0,--=0
         DC    A(TRTO1LF0),A(2048)         Source - Op 1 & length
         DC    A(TRTOPCF0),A(2*K64)        Source - FC Table & length
*                                          Target - FC, Op1, Op1L
         DC    A(7*MB+(9*K64)-481),A(9*MB+(9*K64)-481),A(0)
         DC    A(REG2PATT)
         DC    A(10) CC1 or CC3
         DC    A(9*MB+(9*K64)-481+2048-2),A(2),XL4'F0'
                                                                SPACE 2
         DC    A(0)     end of table
         DC    A(0)     end of table
                                                                EJECT
***********************************************************************
*        TRTE op1 scan data...
***********************************************************************
                                                                SPACE
TRTOP10  DC    64XL4'78125634'                               (CC0)
                                                                SPACE
TRTOP111 DC    04XL4'78125634',X'00110000',59XL4'78125634'   (CC1)
                                                                SPACE
TRTOP1F0 DC    63XL4'78125634',X'000000F0'                   (CC1)
                                                                SPACE
TRTOP1F1 DC    127XL4'78125634',X'000000F1'                  (CC1)
                                                                SPACE
TRTO1L0  DC    512XL4'98765432'                              (CC0)
                                                                SPACE
TRTO1L11 DC    256XL4'98765432',X'00110000',255XL4'98765432' (CC1)
                                                                SPACE
TRTO1LF0 DC    511XL4'98765432',X'000000F0'                  (CC1)
                                                                EJECT
***********************************************************************
*        Function Code (FC) Tables (GR1)
***********************************************************************
                                                                SPACE
TRTOP20  DC    256X'00'                           no stop
         ORG   *+2*K64
                                                                SPACE
TRTOP211 DC    17X'00',X'11',238X'00'             stop on X'11'
                                                                SPACE
TRTOP2F0 DC    240X'00',X'F0',15X'00'             stop on X'F0'
                                                                SPACE
TRTOP411 DC    34X'00',X'0011',476X'00'           stop on X'11'
                                                                SPACE
TRTOP4F0 DC    480X'00',X'00F0',30X'00'           stop on X'F0'
                                                                SPACE
TRTOP811 DC    17X'00',X'11',238X'00'             stop on X'11'
         ORG   *+2*K64
                                                                SPACE
TRTOP8F0 DC    240X'00',X'F0',15X'00'             stop on X'F0'
         ORG   *+2*K64
                                                                SPACE
TRTOP8F1 DC    240X'00',X'00',X'F1',14X'00'       stop on X'F1'
         ORG   *+2*K64
                                                                SPACE
TRTOPC11 DC    34X'00',X'0011'                    stop on X'11'
         ORG   *+2*K64
                                                                SPACE
TRTOPCF0 DC    480X'00',X'00F0',28X'00'           stop on X'F0'
         ORG   *+2*K64
                                                                SPACE
TRTOPCF1 DC    480X'00',X'0000',X'00F1',28X'00'   stop on X'F1'
         ORG   *+2*K64
                                                                EJECT
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
                                                                SPACE 4
         END
