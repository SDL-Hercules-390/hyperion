 TITLE '            TRE-01-basic (Test TRE instructions)'
***********************************************************************
*
*            TRE instruction tests
*
*        NOTE: This test is based the CLCL-et-al Test
*              modified to only test the TRE instruction.
*
*        James Wekel August 2022
***********************************************************************
***********************************************************************
*
*            TRE basic instruction tests
*
***********************************************************************
*
*  This program tests proper functioning of the TRE
*  instructions.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*  Example Hercules Testcase:
*
*
*     *Testcase TRE-01-basic (Test TRE instructions)
*
*     archlvl     390
*     mainsize    3
*     numcpu      1
*     sysclear
*
*     loadcore    "$(testpath)/TRE-01-basic" 0x0
*
*     runtest     1         # (NON-timing test duration)
*
*     *Done
*
*
***********************************************************************
                                                                EJECT
         PRINT OFF
         COPY  'satk.mac'
         PRINT ON
                                                                SPACE
***********************************************************************
*        SATK prolog stuff...
***********************************************************************
                                                                SPACE
         ARCHLVL  ZARCH=NO,MNOTE=NO
                                                                EJECT
***********************************************************************
*        Initiate the TRE01TST CSECT in the CODE region
*        with the location counter at 0
***********************************************************************
                                                                SPACE
TRE01TST ASALOAD  REGION=CODE
                                                                SPACE 3
***********************************************************************
*        Create IPL (restart) PSW
***********************************************************************
                                                                SPACE
         ASAIPL   IA=BEGIN
                                                                EJECT
***********************************************************************
*               The actual "TRE01TST" program itself...
***********************************************************************
*
*  Architecture Mode: 390
*  Addressing Mode:   31-bit
*  Register Usage:
*
*   R0       (work)
*   R1       I/O device used by ENADEV and RAWIO macros
*   R2       First base register
*   R3       IOCB pointer for ENADEV and RAWIO macros
*   R4       IO work register used by ENADEV and RAWIO
*   R5-R7    (work)
*   R8       ORB pointer
*   R9       Second base register
*   R10-R13  (work)
*   R14      Subroutine call
*   R15      Secondary Subroutine call or work
*
***********************************************************************
                                                                SPACE
         USING  ASA,R0          Low core addressability
         USING  BEGIN,R2        FIRST Base Register
         USING  BEGIN+4096,R9   SECOND Base Register
                                                              SPACE
BEGIN    BALR  R2,0             Initalize FIRST base register
         BCTR  R2,0             Initalize FIRST base register
         BCTR  R2,0             Initalize FIRST base register
         ST    R2,SAVER2
                                                                SPACE
         LA    R9,2048(,R2)     Initalize SECOND base register
         LA    R9,2048(,R9)     Initalize SECOND base register
                                                                SPACE 2
***********************************************************************
*        Run the tests...
***********************************************************************
         BAL   R14,TEST01       Test TRE   instruction
                                                                SPACE 2
***********************************************************************
*         Test for normal or unexpected test completion...
***********************************************************************
                                                                SPACE
         CLI   TESTNUM,X'0A'    Did we end on expected test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         CLI   SUBTEST,X'02'    Did we end on expected SUB-test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         B     EOJ              Yes, then normal completion!
                                                                EJECT
***********************************************************************
*        TEST01                   Test TRE instruction
***********************************************************************
                                                                SPACE
TEST01   DS    0F
                                                                SPACE
         ST    R1,SAVER1          Save register 1
         LR    R15,R2             Save first base register
                                                                SPACE
         DROP  R2                 Temporarily drop addressability
         USING BEGIN,R15          Establish temporary addressability
                                                                SPACE
         LA    R5,TRECTL          Point R5 --> testing control table
         USING TRETEST,R5         What each table entry looks like
                                                                SPACE
TST1LOOP EQU   *
                                                                SPACE
         IC    R6,TNUM            Set test number
         STC   r6,TESTNUM
*
**       Initialize operand data  (move data to testing address)
*
         L     R10,OP1WHERE       Where to move operand-1 data to
         L     R11,OP1LEN         Get operand-1 length
         L     R6,OP1DATA         Where op1 data is right now
         L     R7,OP1LEN          How much of it there is
         MVCL  R10,R6
                                                                SPACE
         L     R12,OP2WHERE       Where to move operand-2 data to
         L     R13,=A(OP2LEN)     How much of it there is
         L     R6,OP2DATA         Where op2 data is right now
         L     R7,=A(OP2LEN)      How much of it there is
         MVCL  R12,R6
                                                                SPACE
         IC    R0,TBYTE           Set test byte
                                                                SPACE 2
*
**       Execute TRE instruction and check for expected condition code
*
         MVI   SUBTEST,X'00'      (primary TRE)
                                                                SPACE
         L     R7,FAILMASK        (failure CC)
         SLL   R7,4               (shift to BC instr CC position)
                                                                SPACE
         LM    R10,R12,OPSWHERE   Get TRE operands
TREMORE  TRE   R10,R12              TRE...
         EX    R7,TREBC                Fail if..not an expected cc?
         BC    B'0001',TREMORE             Not finished
                                                                EJECT
*
**       Verify end conditions R10 and expected main store
**       (last 4 bytes)
*
         L     R6,ENDREG
                                                                SPACE
         MVI   SUBTEST,X'01'      (R10 result)
         CLR   R10,R6             R10 correct?
         BNE   TREFAIL            No, FAILTEST!
                                                                SPACE
         MVI   SUBTEST,X'02'      (end store)
         LR    R6,R10
         S     R6,=F'4'
         CLC   ENDSTOR,0(R6)      End storage correct?
         BNE   TREFAIL            No, FAILTEST!
                                                                SPACE
         LA    R5,TRENEXT         Go on to next table entry
         CLC   =F'0',0(R5)        End of table?
         BNE   TST1LOOP           No, loop...
         B     TREDONE            Done! (success!)
                                                                SPACE
TREFAIL  LA    R14,FAILTEST       Unexpected results!
TREDONE  L     R1,SAVER1          Restore register 1
         LR    R2,R15             Restore first base register
         BR    R14                Return to caller or FAILTEST
                                                                SPACE
TREBC    BC    0,TREFAIL          (fail if unexpected condition code)
                                                                SPACE
SAVER1   DC    F'0'
SAVER2   DC    F'0'
SAVER5   DC    F'0'
SAVETRT  DC    D'0'               (saved R1/R2 from TRT results)
                                                                SPACE
         DROP  R5
         DROP  R15
         USING BEGIN,R2
                                                                EJECT
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 2
EOJ      DWAITEND LOAD=YES          Normal completion
                                                                SPACE 4
FAILDEV  DWAIT LOAD=YES,CODE=01     ENADEV failed
                                                                SPACE 4
FAILIO   DWAIT LOAD=YES,CODE=02     RAWIO failed
                                                                SPACE 4
FAILTEST DWAIT LOAD=YES,CODE=BAD    Abnormal termination
                                                                SPACE 4
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE
         LTORG ,                Literals pool
                                                                SPACE
K        EQU   1024             One KB
PAGE     EQU   (4*K)            Size of one page
K64      EQU   (64*K)           64 KB
MB       EQU   (K*K)             1 MB
                                                                SPACE
TESTADDR EQU   (2*PAGE+X'200'-2)  Where test/subtest numbers will go
                                                                EJECT
***********************************************************************
*        TRETEST DSECT
***********************************************************************
                                                                SPACE 2
TRETEST  DSECT ,
                                                                SPACE 2
TNUM     DC    X'00'          TRE table Number
TBYTE    DC    X'00'          TRE Testbyte
         DC    X'00'
         DC    X'00'
                                                                SPACE 2
OP1DATA  DC    A(0)           Pointer to Operand-1 data
OP2DATA  DC    A(0)           Pointer to Operand-2 data
                                                                SPACE
OPSWHERE EQU   *              Where TRE Operands are located
OP1WHERE DC    A(0)           Where Operand-1 data should be placed
OP1LEN   DC    F'0'           How much data is there - 1
OP2WHERE DC    A(0)           Where Operand-2 data should be placed
OP2LEN   EQU   256            Operand-2 is always 256
                                                                SPACE 2
FAILMASK DC    A(0)           Failure Branch on Condition mask
                                                                SPACE 2
ENDREG   DC    A(0)           Ending R10 register value
ENDSTOR  DC    XL4'00'        Ending TRE main store value
                                                                SPACE 2
TRENEXT  EQU   *              Start of next table entry...
                                                                EJECT
***********************************************************************
*        TRE Testing Control tables   (ref: TRETEST DSECT)
***********************************************************************
         PRINT DATA
TRE01TST CSECT ,
TRECTL   DC    0A(0)    start of table
                                                                SPACE
                                                                SPACE 2
TRE1     DS    0F
         DC    X'01',X'00',X'00',X'00'
         DC    A(TRTOP10),A(TRTOP20)
         DC    A(00+(1*K64)),A(001),A(MB+(1*K64))
         DC    A(7) CC0
         DC    A(00+(1*K64)+01),X'00000000'
                                                                SPACE 4
TRE2     DS    0F
         DC    X'02',X'00',X'00',X'00'
         DC    A(TRTOP10),A(TRTOP20)
         DC    A(00+(2*K64)),A(002),A(MB+(2*K64))
         DC    A(7) CC0
         DC    A(00+(2*K64)+02),X'00000000'
                                                                SPACE 4
TRE4     DS    0F
         DC    X'03',X'00',X'00',X'00'
         DC    A(TRTOP10),A(TRTOP20)
         DC    A(00+(3*K64)),A(004),A(MB+(3*K64))
         DC    A(7) CC0
         DC    A(00+(3*K64)+04),X'00000000'
                                                                SPACE 4
TRE8     DS    0F
         DC    X'04',X'00',X'00',X'00'
         DC    A(TRTOP10),A(TRTOP20)
         DC    A(00+(4*K64)),A(008),A(MB+(4*K64))
         DC    A(7) CC0
         DC    A(00+(4*K64)+08),X'00000000'
                                                                EJECT
TRE256   DS    0F
         DC    X'05',X'00',X'00',X'00'
         DC    A(TRTOP10),A(TRTOP20)
         DC    A(00+(5*K64)),A(256),A(MB+(5*K64))
         DC    A(7) CC0
         DC    A(00+(5*K64)+256),X'00000000'
                                                                SPACE 4
TREBTH   DS    0F
         DC    X'06',X'11',X'00',X'00'
         DC    A(TRTOP111),A(TRTOP211)
         DC    A(00+(6*K64)-12),A(256),A(MB+(6*K64)-34) both cross page
         DC    A(10)   CC1 = stop, scan incomplete or CC=3
         DC    A(00+(6*K64)-12+X'11'),X'00000000'
                                                                SPACE 4
TREOP1   DS    0F
         DC    X'07',X'F0',X'00',X'00'
         DC    A(TRTOP1F0),A(TRTOP2F0)
         DC    A(00+(7*K64)-12),A(256),A(MB+(7*K64)) only op1 crosses
         DC    A(10) CC1 = stopped on last byte  or CC=3
         DC    A(00+(7*K64)-12+255),X'00000000'
                                                                SPACE 4
TREOP2   DS    0F
         DC    X'08',X'11',X'00',X'00'
         DC    A(TRTOP111),A(TRTOP211)
         DC    A(00+(8*K64)),A(256),A(MB+(8*K64)-34)   only op2 crosses
         DC    A(10) CC1 = stop, scan incomplete or CC=3
         DC    A(00+(8*K64)+X'11'),X'00000000'
                                                                EJECT
TRELOP1  DS    0F
         DC    X'09',X'00',X'00',X'00'
         DC    A(TRTOP10),A(TRELOP20)
         DC    A(00+(9*K64)-12),A(512),A(MB+(9*K64)) only op1 crosses
         DC    A(6)  CC0 or CC=3
         DC    A(00+(9*K64)-12+512),X'00000000'
                                                                SPACE 4
TRELOP2  DS    0F
         DC    X'0A',X'00',X'00',X'00'
         DC    A(TRTOP10),A(TRELOP21)
         DC    A(00+(10*K64)-12),A(4600),A(MB+(10*K64)) op1 crosses 2X
         DC    A(6)  CC0 or CC=3
         DC    A(00+(10*K64)-12+4600),X'FFFFFFFF'
                                                               SPACE 7
         DC    A(0)     end of table
         DC    A(0)     end of table
                                                                EJECT
***********************************************************************
*        TRE op1 scan data...
***********************************************************************
                                                                SPACE
TRTOP10  DC    1150XL4'78125634'
                                                                SPACE
TRTOP111 DC    04XL4'78125634',X'00110000',59XL4'78125634'    (CC1)
                                                                SPACE
TRTOP1F0 DC    63XL4'78125634',X'000000F0'    (CC1)
                                                                SPACE
                                                                SPACE
                                                                EJECT
***********************************************************************
*        TRE op2 stop tables...
***********************************************************************
                                                                SPACE
TRTOP20  DC    256X'00'                   no stop
                                                                SPACE
TRTOP211 DC    17X'00',X'11',238X'00'     stop on X'11'
                                                                SPACE
TRTOP2F0 DC    240X'00',X'F0',15X'00'     stop on X'F0'
                                                                SPACE
TRELOP20 DC    X'FF',255X'00'
                                                                SPACE
TRELOP21 DC    256X'FF'
                                                                EJECT
***********************************************************************
*        Fixed storage locations
***********************************************************************
                                                                SPACE 2
         ORG   TRE01TST+TESTADDR      (s/b @ X'21FE', X'21FF')
                                                                SPACE
TESTNUM  DC    X'00'      Test number of active test
SUBTEST  DC    X'00'      Active test sub-test number
                                                                SPACE 3
***********************************************************************
*        (other DSECTS needed by SATK)
***********************************************************************
                                                                SPACE
         DSECTS PRINT=OFF,NAME=(ASA,SCHIB,CCW0,CCW1,CSW)
         PRINT ON
***********************************************************************
*        Register equates
***********************************************************************
                                                                SPACE
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
                                                                SPACE 2
         END
