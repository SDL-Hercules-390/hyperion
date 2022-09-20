 TITLE '            CLCL-et-al (Test CLCL, MVCIN and TRT instructions)'
***********************************************************************
*
*            CLC, CLCL, MVCIN and TRT instruction tests
*
***********************************************************************
*
*  This program tests proper functioning of the CLCL, MVCIN and TRT
*  instructions.  It also optionally times them.
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
*     *Testcase CLCL-et-al (Test CLCL, MVCIN and TRT instructions)
*
*     archlvl     390
*     mainsize    2
*     numcpu      1
*     sysclear
*
*     loadcore    "$(testpath)/CLCL-et-al.core"
*
*     ##r           21fd=ff   # (enable timing tests too!)
*     ##runtest     150       # (TIMING too test duration)
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
*        Initiate the CLCLetal CSECT in the CODE region
*        with the location counter at 0
***********************************************************************
                                                                SPACE
CLCLetal ASALOAD  REGION=CODE
                                                                SPACE 3
***********************************************************************
*        Create IPL (restart) PSW
***********************************************************************
                                                                SPACE
         ASAIPL   IA=BEGIN
                                                                EJECT
***********************************************************************
*               The actual "CLCLetal" program itself...
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
         USING  IOCB,R3         SATK Device I/O Control Block
         USING  ORB,R8          ESA/390 Operation Request Block
                                                                SPACE
BEGIN    BALR  R2,0             Initalize FIRST base register
         BCTR  R2,0             Initalize FIRST base register
         BCTR  R2,0             Initalize FIRST base register
                                                                SPACE
         LA    R9,2048(,R2)     Initalize SECOND base register
         LA    R9,2048(,R9)     Initalize SECOND base register
                                                                SPACE
         BAL   R14,INIT         Initalize Program
*
**       Run the tests...
*
         BAL   R14,TEST01       Test CLC   instruction
         BAL   R14,TEST02       Test CLCL  instruction
         BAL   R14,TEST03       Test MVCIN instruction
         BAL   R14,TEST04       Test TRT   instruction
*
         BAL   R14,TEST91       Time CLC   instruction  (speed test)
         BAL   R14,TEST92       Time CLCL  instruction  (speed test)
         BAL   R14,TEST93       Time MVCIN instruction  (speed test)
         BAL   R14,TEST94       Time TRT   instruction  (speed test)
*
         BAL   R14,TEST95       Test CLCL page fault handling
                                                                EJECT
***********************************************************************
*         Test for normal or unexpected test completion...
***********************************************************************
                                                                SPACE
         CLI   TIMEOPT,X'00'    Normal (non-timing) run?
         BNE   EOJ              No, timing run; just go end normally
                                                                SPACE
         CLI   TESTNUM,X'95'    Did we end on expected test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         CLI   SUBTEST,X'10'    Did we end on expected SUB-test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         B     EOJ              Yes, then normal completion!
                                                                EJECT
***********************************************************************
*        TEST01                 Test CLC instruction
***********************************************************************
                                                                SPACE
TEST01   MVI   TESTNUM,X'01'
*
**       Initialize test parameters...
*
         L      R5,CLC4         Operand-1 address
         MVI    3(R5),X'FF'     Force unequal compare (op1 high)
         L      R5,CLC256       (same thing for CLC256)
         MVI    255(R5),X'FF'   (same thing for CLC256)
         L      R5,CLCOP1       (same thing for CLCOP1)
         MVI    255(R5),X'FF'   (same thing for CLCOP1)
         L      R6,CLC8+4       OPERAND-2(!) address
         MVI    7(R6),X'FF'     Force OPERAND-2 to be high! (op1 LOW!)
*
**       Neither cross (one byte)
*
         MVI   SUBTEST,X'01'
         LM    R5,R6,CLC1
         CLC   0(1,R5),0(R6)
         BNE   FAILTEST
*
**       Neither cross (two bytes)
*
         MVI   SUBTEST,X'02'
         LM    R5,R6,CLC2
         CLC   0(2,R5),0(R6)
         BNE   FAILTEST
*
**       Neither cross (four bytes)
*
         MVI   SUBTEST,X'04'
         LM    R5,R6,CLC4
         CLC   0(4,R5),0(R6)
         BNH   FAILTEST                 (see INIT; CLC4:    op1 > op2)
*
**       Neither cross (eight bytes)
*
         MVI   SUBTEST,X'08'
         LM    R5,R6,CLC8
         CLC   0(8,R5),0(R6)
         BNL   FAILTEST                 (see INIT; CLC8:    op1 < op2)
                                                                EJECT
*
**       Neither cross (256 bytes)
*
         MVI   SUBTEST,X'FF'
         LM    R5,R6,CLC256
         CLC   0(256,R5),0(R6)
         BNH   FAILTEST                 (see INIT; CLC256:  op1 > op2)
*
**       Both cross
*
         MVI   SUBTEST,X'22'
         LM    R5,R6,CLCBOTH
         CLC   0(256,R5),0(R6)
         BNE   FAILTEST
*
**       Only op1 crosses
*
         MVI   SUBTEST,X'10'
         LM    R5,R6,CLCOP1
         CLC   0(256,R5),0(R6)
         BNH   FAILTEST                 (see INIT; CLCOP1:  op1 > op2)
*
**       Only op2 crosses
*
         MVI   SUBTEST,X'20'
         LM    R5,R6,CLCOP2
         CLC   0(256,R5),0(R6)
         BNE   FAILTEST
*
         BR    R14
                                                                EJECT
***********************************************************************
*        TEST02                   Test CLCL instruction
***********************************************************************
                                                                SPACE
TEST02   MVI   TESTNUM,X'02'
*
**       Initialize test parameters...
*
         LM     R5,R6,CLCL4     CLCL4 test Op1 address and length
         ALR    R5,R6           Point past last byte
         BCTR   R5,0            Backup to last byte
         MVI    0(R5),X'FF'     Force unequal compare (op1 high)
*
         LM     R5,R6,CLCLOP1   (same thing for CLCLOP1 test)
         ALR    R5,R6                        "
         BCTR   R5,0                         "
         MVI    0(R5),X'FF'                  "
*
         LM     R5,R6,CLCL8+8   CLCL8 test ===> OP2 <===
         ALR    R5,R6
         BCTR   R5,0
         MVI    0(R5),X'FF'     ===> OPERAND-2 high (OP1 LOW) <===
*
**       Neither cross (one byte)
*
         MVI   SUBTEST,X'01'
         LM    R10,R13,CLCL1
         CLCL  R10,R12
         BNE   FAILTEST
         LA    R5,ECLCL1
         BAL   R15,ENDCLCL
*
**       Neither cross (two bytes)
*
         MVI   SUBTEST,X'02'
         LM    R10,R13,CLCL2
         CLCL  R10,R12
         BNE   FAILTEST
         LA    R5,ECLCL2
         BAL   R15,ENDCLCL
*
**       Neither cross (four bytes)
**       (inequality on last byte of op1)
*
         MVI   SUBTEST,X'04'
         LM    R10,R13,CLCL4
         CLCL  R10,R12
         BNH   FAILTEST                 (see INIT; CLCL4:   op1 > op2)
         LA    R5,ECLCL4
         BAL   R15,ENDCLCL
                                                                EJECT
*
**       Neither cross (eight bytes)
**       (inequality on last byte of op2)
*
         MVI   SUBTEST,X'08'
         LM    R10,R13,CLCL8
         CLCL  R10,R12
         BNL   FAILTEST                 (see INIT; CLCL8:   op1 < op2)
         LA    R5,ECLCL8
         BAL   R15,ENDCLCL
*
**       Neither cross (1K bytes)
*
         MVI   SUBTEST,X'00'
         LM    R10,R13,CLCL1K
         CLCL  R10,R12
         BNE   FAILTEST
         LA    R5,ECLCL1K
         BAL   R15,ENDCLCL
*
**       Both cross
*
         MVI   SUBTEST,X'22'
         LM    R10,R13,CLCLBOTH
         CLCL  R10,R12
         BNE   FAILTEST
         LA    R5,ECLCLBTH
         BAL   R15,ENDCLCL
*
**       Only op1 crosses
**       (inequality on last byte of op1)
*
         MVI   SUBTEST,X'10'
         LM    R10,R13,CLCLOP1
         CLCL  R10,R12
         BNH   FAILTEST                 (see INIT; CLCLOP1: op1 > op2)
         LA    R5,ECLCLOP1
         BAL   R15,ENDCLCL
*
**       Only op2 crosses
*
         MVI   SUBTEST,X'20'
         LM    R10,R13,CLCLOP2
         CLCL  R10,R12
         BNE   FAILTEST
         LA    R5,ECLCLOP2
         BAL   R15,ENDCLCL
*
         BR    R14
                                                                EJECT
***********************************************************************
*        TEST03                   Test MVCIN instruction
***********************************************************************
                                                                SPACE
TEST03   MVI   TESTNUM,X'03'
*
**       Neither cross (one byte)
*
         LA    R5,INV1
         BAL   R15,MVCINTST
*
**       Neither cross (two bytes)
*
         LA    R5,INV2
         BAL   R15,MVCINTST
*
**       Neither cross (four bytes)
*
         LA    R5,INV4
         BAL   R15,MVCINTST
*
**       Neither cross (eight bytes)
*
         LA    R5,INV8
         BAL   R15,MVCINTST
*
**       Neither cross (256 bytes)
*
         LA    R5,INV256
         BAL   R15,MVCINTST
*
**       Both cross
*
         LA    R5,INVBOTH
         BAL   R15,MVCINTST
*
**       Only op1 crosses
*
         LA    R5,INVOP1
         BAL   R15,MVCINTST
*
**       Only op2 crosses
*
         LA    R5,INVOP2
         BAL   R15,MVCINTST
*
         BR    R14
                                                                EJECT
***********************************************************************
*        TEST04                   Test TRT instruction
***********************************************************************
                                                                SPACE
TEST04   MVI   TESTNUM,X'04'

         ST    R1,SAVER1          Save register 1
         LR    R15,R2             Save first base register

         DROP  R2                 Temporarily drop addressability
         USING BEGIN,R15          Establish temporary addressability

         LA    R5,TRTCTL          Point R5 --> testing control table
         USING TRTTEST,R5         What each table entry looks like

TST4LOOP EQU   *
*
**       Initialize operand data  (move data to testing address)
*
         L     R10,OP1WHERE       Where to move operand-1 data to
         L     R12,OP2WHERE       Where to move operand-2 data to

         L     R6,OP1DATA         Where op1 data is right now
         L     R7,OP1LEN          How much of it there is
         EX    R7,TRTMVC1         Move op1 data to testing location

         L     R6,OP2DATA         Where op1 data is right now
         L     R7,OP2LEN          How much of it there is
         EX    R7,TRTMVC2         Move op1 data to testing location
                                                              EJECT
*
**       Initialize R1/R2...      (TRT non-zero CC updates R1/R2!)
*
         SLR   R1,R1              (known value)
         L     R2,=A(REG2PATT)    (known value)
*
**       Execute TRT instruction and check for expected condition code
*
         L     R7,EXLEN           (len-1)
         L     R11,FAILMASK       (failure CC)
         SLL   R11,4              (shift to BC instr CC position)

         MVI   SUBTEST,X'00'      (primary TRT)
         EX    R7,TRT             TRT...
         STM   R1,R2,SAVETRT      (save R1/R2 results)
         EX    R11,TRTBC          fail if...
*
**       Verify R1/R2 now contain (or still contain!) expected values
*
         LM    R6,R7,ENDREGS

         MVI   SUBTEST,X'01'      (R1 result)
         CLR   R1,R6              R1 correct?
         BNE   TRTFAIL            No, FAILTEST!

         MVI   SUBTEST,X'02'      (R2 result)
         CLR   R2,R7              R2 correct?
         BNE   TRTFAIL            No, FAILTEST!

         LA    R5,TRTNEXT         Go on to next table entry
         CLC   =F'0',0(R5)        End of table?
         BNE   TST4LOOP           No, loop...
         B     TRTDONE            Done! (success!)

TRTFAIL  LA    R14,FAILTEST       Unexpected results!
TRTDONE  L     R1,SAVER1          Restore register 1
         LR    R2,R15             Restore first base register
         BR    R14                Return to caller or FAILTEST

TRTMVC1  MVC   0(0,R10),0(R6)     (move op1 to where it should be)
TRTMVC2  MVC   0(0,R12),0(R6)     (move op2 to where it should be)

TRT      TRT   0(0,R10),0(R12)    (TRT op1,op2)
TRTBC    BC    0,TRTFAIL          (fail if unexpected condition code)

SAVER1   DC    F'0'
SAVETRT  DC    D'0'               (saved R1/R2 from TRT results)

         DROP  R5
         DROP  R15
         USING BEGIN,R2
                                                                EJECT
***********************************************************************
*        TEST91                 Time CLC instruction  (speed test)
***********************************************************************
                                                                SPACE
TEST91   TM    TIMEOPT,X'FF'    Is timing tests option enabled?
         BZR   R14              No, skip timing tests
                                                                SPACE
         MVI   TESTNUM,X'91'
         MVI   SUBTEST,X'01'
*
**       First, make sure we start clean!
*
         LM    R10,R13,CLCL256        (Yes, "CLCL256", not "CLC256"!)
         MVC   0(256,R10),0(R12)      (forces full equal comparison)
*
**       Next, time the overhead...
*
         L     R5,NUMLOOPS
         STCK  BEGCLOCK
         BALR  R6,0
         BCTR  R5,R6
         STCK  ENDCLOCK
         BAL   R15,CALCDUR
         MVC   OVERHEAD,DURATION
*
**       Now do the actual timing run...
*
         L     R5,NUMLOOPS
         STCK  BEGCLOCK
         BALR  R6,0
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
*        ..........ETC..........
         PRINT OFF
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         PRINT ON
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         CLC   0(256,R10),0(R12)
         BCTR  R5,R6
         STCK  ENDCLOCK
*
         MVC   PRTLINE+33(5),=CL5'CLC'
         BAL   R15,RPTSPEED
         BR    R14
                                                                EJECT
***********************************************************************
*        TEST92                 Time CLCL instruction  (speed test)
***********************************************************************
                                                                SPACE
TEST92   TM    TIMEOPT,X'FF'    Is timing tests option enabled?
         BZR   R14              No, skip timing tests
                                                                SPACE
         MVI   TESTNUM,X'92'
         MVI   SUBTEST,X'01'
*
**       First, make sure we start clean!
*
         LM    R10,R13,CLCL256
         MVC   0(256,R10),0(R12)          (forces full comparison)
*
**       Next, time the overhead...
*
         L     R5,NUMLOOPS
         STCK  BEGCLOCK
         BALR  R6,0
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
*        .........ETC.........
         PRINT OFF
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         PRINT ON
         LM    R10,R13,CLCL256
         LM    R10,R13,CLCL256
         BCTR  R5,R6
         STCK  ENDCLOCK
         BAL   R15,CALCDUR
         MVC   OVERHEAD,DURATION
*
**       Now do the actual timing run...
*
         L     R5,NUMLOOPS
         STCK  BEGCLOCK
         BALR  R6,0
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
*        .........ETC.........
         PRINT OFF
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         PRINT ON
         LM    R10,R13,CLCL256
         CLCL  R10,R12
         BCTR  R5,R6
         STCK  ENDCLOCK
*
         MVC   PRTLINE+33(5),=CL5'CLCL'
         BAL   R15,RPTSPEED
         BR    R14
                                                                EJECT
***********************************************************************
*        TEST93                 Time MVCIN instruction  (speed test)
***********************************************************************
                                                                SPACE
TEST93   TM    TIMEOPT,X'FF'    Is timing tests option enabled?
         BZR   R14              No, skip timing tests
                                                                SPACE
         MVI   TESTNUM,X'93'
         MVI   SUBTEST,X'01'
*
**       First, make sure we start clean!
*
         LM    R10,R13,INV256
         MVC   0(256,R13),MVCININ     (doesn't really matter, but...)
*
**       Next, time the overhead...
*
         L     R5,NUMLOOPS
         STCK  BEGCLOCK
         BALR  R6,0
         BCTR  R5,R6
         STCK  ENDCLOCK
         BAL   R15,CALCDUR
         MVC   OVERHEAD,DURATION
*
**       Now do the actual timing run...
*
         L     R5,NUMLOOPS
         STCK  BEGCLOCK
         BALR  R6,0
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
*        .........ETC.........
         PRINT OFF
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         PRINT ON
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         MVCIN 0(256,R10),0(R11)
         BCTR  R5,R6
         STCK  ENDCLOCK
*
         MVC   PRTLINE+33(5),=CL5'MVCIN'
         BAL   R15,RPTSPEED
         BR    R14
                                                                EJECT
***********************************************************************
*        TEST94                 Time TRT instruction  (speed test)
***********************************************************************
                                                                SPACE
TEST94   TM    TIMEOPT,X'FF'    Is timing tests option enabled?
         BZR   R14              No, skip timing tests
                                                                SPACE
         MVI   TESTNUM,X'94'
         MVI   SUBTEST,X'01'
*
**       First, make sure we start clean!
*
         L     R10,=A(00+(5*K64))
         MVC   0(256,R10),TRTOP10
         L     R12,=A(MB+(5*K64))
         MVC   0(256,R12),TRTOP20     (no stop = full op1 processing)
*
**       Next, time the overhead...
*
         L     R5,NUMLOOPS
         STCK  BEGCLOCK
         BALR  R6,0
         BCTR  R5,R6
         STCK  ENDCLOCK
         BAL   R15,CALCDUR
         MVC   OVERHEAD,DURATION
*
**       Now do the actual timing run...
*
         L     R5,NUMLOOPS
         STCK  BEGCLOCK
         BALR  R6,0
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
*        .........ETC.........
         PRINT OFF
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         PRINT ON
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         TRT   0(256,R10),0(R12)
         BCTR  R5,R6
         STCK  ENDCLOCK
*
         MVC   PRTLINE+33(5),=CL5'TRT'
         BAL   R15,RPTSPEED
         BR    R14
                                                                EJECT
***********************************************************************
*        TEST95                   Test CLCL page fault handling
***********************************************************************
                                                                SPACE
TEST95   MVI   TESTNUM,X'95'
         MVI   SUBTEST,X'00'
*
**       First, make sure we start clean!
*
         LM    R10,R13,CLCLPF     Retrieve CLCL PF test parameters
         MVCL  R10,R12            (forces full comparison)
*
**       Initialize Dynamic Address Translation tables...
*
         L     R10,=A(SEGTABLS)   Segment Tables Origin
         LA    R11,NUMPGTBS       Number of Segment Table Entries
         L     R12,=A(PAGETABS)   Page Tables Origin
         SLR   R0,R0              First Page Frame Address
         LA    R6,4               Size of one table entry
         L     R7,=A(PAGE)        Size of one Page Frame
                                                                SPACE
SEGLOOP  ST    R12,0(,R10)        Seg Table Entry <= Page Table Origin
         OI    3(R10),X'0F'       Seg Table Entry <= Page Table Length
         ALR   R10,R6             Bump to next Segment Table Entry
                                                                SPACE
         LA    R13,16             Page Table Entries per Page Table
PAGELOOP ST    R0,0(,R12)         Page Table Entry = Page Frame Address
         ALR   R0,R7              Increment to next Page Frame Address
         ALR   R12,R6             Bump to next Page Table Entry
         BCT   R13,PAGELOOP       Loop until Page table is complete
                                                                SPACE
         BCT   R11,SEGLOOP        Loop until all Segment Table Entries built
*
**       Update desired page table entry to cause page fault
*
         LM    R10,R13,CLCLPF     Retrieve CLCL PF test parameters
         LR    R5,R10             R5 --> Operand-1
         AL    R5,=A(PFPGBYTS)    R5 --> Operand-1 Page Fault address
         LR    R6,R5              R6 --> Address where PF should occur
         SRL   R5,12              R5 = Page Frame number
         SLL   R5,2               R5 = Page Table Entry number
                                                                SPACE
         MVI   SUBTEST,X'04'
         AL    R5,=A(PAGETABS)    R5 --> Page Table Entry
         OI    2(R5),X'04'        Mark this page invalid
                                                                EJECT
*
**       Install program check routine to catch the page fault
*
         MVI   SUBTEST,X'02'
         MVC   SVPGMNEW,PGMNPSW   Save original Program New PSW
         LA    R0,MYPGMNEW        Point to temporary Pgm New routine
         ST    R0,PGMNPSW+4       Point Program New PSW to our routine
         MVI   PGMNPSW+1,X'08'    Make it a non-disabled-wait PSW!
*
**       Run the test: should cause a page fault
*
         MVI   SUBTEST,X'0F'
         LCTL  R0,R0,CRLREG0      Switch to DAT mode
         LCTL  R1,R1,CTLREG1      Switch to DAT mode
         LPSW  DATONPSW           Switch to DAT mode
BEGDATON NOP   *                  (pad)
         NOP   *                  (pad)
         PTLB  ,                  Purge Translation Lookaside Buffer
PFINSADR CLCL  R10,R12            Page Fault should occur on this instr
         CNOP  0,8                        (align to doubleword)
LOGICERR DC    D'0'                       We should never reach here!
SVPGMNEW DC    D'0'                       Original Program New PSW
DATONPSW DC    XL4'04080000',A(BEGDATON)  Enable DAT PSW
*
**       Temporary Program New routine:
**       Restore original Program New PSW
*
MYPGMNEW MVC   PGMNPSW,SVPGMNEW   Restore original Program New PSW
*
**       Verify Program Check occurred on expected instruction
*
         MVI   SUBTEST,X'68'
         CLC   =A(PFINSADR),PGMOPSW+4   Program Check where expected?
         BNE   FAILTEST                 No?! Something is VERY WRONG!
*
**       Verify Program Check was indeed a page fault
*
         MVI   SUBTEST,X'11'
         CLI   PGMICODE+1,X'11'   Verify it's a Page Fault interrupt
         BNE   FAILTEST           If not then something is VERY WRONG!
                                                                EJECT
*
**       Verify Page Fault occurred on expected Page
*
         MVI   SUBTEST,X'05'
         L     R0,PGMTRX          Get where Page Fault occurred
         SRL   R0,12
         SLL   R0,12
                                                                SPACE
         SRL   R6,12              Where Page Fault is expected
         SLL   R6,12
                                                                SPACE
         CLR   R0,R6              Page Fault occur on expected Page?
         BNE   FAILTEST           No? Then something is very wrong!
*
**       Verify CLCL instruction registers were updated as expected
*
         MVI   SUBTEST,X'06'
         CL    R10,CLCLPF         (op1 greater than starting value?)
         BNH   FAILTEST
         CL    R12,CLCLPF+4+4     (op2 greater than starting value?)
         BNH   FAILTEST
                                                                SPACE
         MVI   SUBTEST,X'07'
         CLR   R11,R13            (same remaining lengths?)
         BNE   FAILTEST
         CL    R11,CLCLPF+4       (op1 len less than starting value?)
         BNL   FAILTEST
         CL    R13,CLCLPF+4+4+4   (op2 len less than starting value?)
         BNL   FAILTEST
                                                                SPACE
         MVI   SUBTEST,X'08'
         CL    R10,ECLCLPF        (stop before end?)
         BNL   FAILTEST
                                                                SPACE
         MVI   SUBTEST,X'09'
         CLR   R10,R6             (stop at or before expected page?)
         BH    FAILTEST
                                                                SPACE
         MVI   SUBTEST,X'10'
         LR    R7,R10             (op1 stopped address)
         ALR   R7,R11             (add remaining length)
         CLR   R7,R6              (would remainder reach PF page?)
         BNH   FAILTEST
                                                                SPACE
         BR    R14                Success!
                                                                EJECT
***********************************************************************
*        RPTSPEED                 Report instruction speed
***********************************************************************
                                                                SPACE
RPTSPEED ST    R15,RPTSAVE        Save return address
         BAL   R15,CALCDUR        Calculate duration
*
         LA    R5,OVERHEAD        Subtract overhead
         LA    R6,DURATION        From raw timing
         LA    R7,DURATION        Yielding true instruction timing
         BAL   R15,SUBDWORD       Do it
*
         LM    R12,R13,DURATION   Convert to...
         SRDL  R12,12             ... microseconds
*
         CVD   R12,TICKSAAA       convert HIGH part to decimal
         CVD   R13,TICKSBBB       convert LOW  part to decimal
*
         ZAP   TICKSTOT,TICKSAAA            Calculate...
         MP    TICKSTOT,=P'4294967296'      ...decimal...
         AP    TICKSTOT,TICKSBBB            ...microseconds
*
         MVC   PRTLINE+43(L'EDIT),EDIT          (edit into...
         ED    PRTLINE+43(L'EDIT),TICKSTOT+3     ...print line)
                                                                SPACE 6
         RAWIO 4,FAIL=FAILIO      Print elapsed time on console
                                                                SPACE 2
         L     R15,RPTSAVE        Restore return address
         BR    R15                Return to caller
                                                                SPACE
RPTSAVE  DC    F'0'               R15 save area
                                                                EJECT
***********************************************************************
*        CALCDUR                  Calculate DURATION
***********************************************************************
                                                                SPACE
CALCDUR  ST    R15,CALCRET        Save return address
         STM   R5,R7,CALCWORK     Save work registers
*
         LM    R6,R7,BEGCLOCK     Remove CPU number from clock value
         SRDL  R6,6                            "
         SLDL  R6,6                            "
         STM   R6,R7,BEGCLOCK                  "
*
         LM    R6,R7,ENDCLOCK     Remove CPU number from clock value
         SRDL  R6,6                            "
         SLDL  R6,6                            "
         STM   R6,R7,ENDCLOCK                  "
*
         LA    R5,BEGCLOCK        Starting time
         LA    R6,ENDCLOCK        Ending time
         LA    R7,DURATION        Difference
         BAL   R15,SUBDWORD       Calculate duration
*
         LM    R5,R7,CALCWORK     Restore work registers
         L     R15,CALCRET        Restore return address
         BR    R15                Return to caller
                                                                SPACE
CALCRET  DC    F'0'               R15 save area
CALCWORK DC    3F'0'              R5-R7 save area
                                                                SPACE 4
***********************************************************************
*        SUBDWORD                 Subtract two doublewords
*        R5 --> subtrahend, R6 --> minuend, R7 --> result
***********************************************************************
                                                                SPACE
SUBDWORD STM   R10,R13,SUBDWSAV   Save registers
*
         LM    R10,R11,0(R5)      Subtrahend  (value to subtract)
         LM    R12,R13,0(R6)      Minuend     (what to subtract FROM)
         SLR   R13,R11            Subtract LOW part
         BNM   *+4+4              (branch if no borrow)
         SL    R12,=F'1'          (otherwise do borrow)
         SLR   R12,R10            Subtract HIGH part
         STM   R12,R13,0(R7)      Store results
*
         LM    R10,R13,SUBDWSAV   Restore registers
         BR    R15                Return to caller
                                                                SPACE
SUBDWSAV DC    2D'0'              R10-R13 save area
                                                                EJECT
***********************************************************************
*        Program Initialization
***********************************************************************
                                                                SPACE
INIT     DS     0H              Program Initialization
                                                                SPACE
         LA     R3,IOCB_009     Point to IOCB
         L      R8,IOCBORB      Point to ORB
                                                                SPACE
         BAL    R15,IOINIT      Initialize the CPU for I/O operations
         BAL    R15,ENADEV      Enable our device making ready for use
         BR     R14             Return to caller
                                                                SPACE 5
***********************************************************************
*        Verify CLCL ending register values
* R10-R12 = actual ending values, R5 --> expected ending values
***********************************************************************
                                                                SPACE
ENDCLCL  STM    R10,R13,CLCLEND     Save actual ending register values
         CLC    0(4*4,R5),CLCLEND   Do they have the expected values?
         BNE    FAILTEST            If not then the test has failed
         BR     R15                 Otherwise return to caller
                                                                SPACE 5
***********************************************************************
*        MVCINTST
***********************************************************************
                                                                SPACE
MVCINTST LM    R10,R13,0(R5)        a(dst),a(src+(len-1)),a(len-1),a(src)
         LA    R6,MVCININ+256-1     Point to end of source
         SLR   R6,R12               Backup by length amount
         EX    R12,MVCINSRC         Initialize source data
         EX    R12,MVCINMVC         Do the Move Inverse
         EX    R12,MVCINCLC         Compare with expected results
         BNE   FAILTEST             FAIL if not the expected value
         BR    R15                  Otherwise return to caller
                                                                SPACE 3
MVCINSRC MVC   0(0,R13),0(R6)       Executed Instruction
MVCINMVC MVCIN 0(0,R10),0(R11)      Executed Instruction
MVCINCLC CLC   0(0,R10),MVCINOUT    Executed Instruction
                                                                EJECT
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 5
EOJ      DWAITEND LOAD=YES          Normal completion
                                                                SPACE 5
FAILDEV  DWAIT LOAD=YES,CODE=01     ENADEV failed
                                                                SPACE 5
FAILIO   DWAIT LOAD=YES,CODE=02     RAWIO failed
                                                                SPACE 5
FAILTEST DWAIT LOAD=YES,CODE=BAD    Abnormal termination
                                                                EJECT
***********************************************************************
*        Initialize the CPU for I/O operations
***********************************************************************
                                                                SPACE 2
IOINIT   IOINIT ,
                                                                SPACE 2
         BR    R15                Return to caller
                                                                SPACE 6
***********************************************************************
*        Enable the device, making it ready for use
***********************************************************************
                                                                SPACE 2
ENADEV   ENADEV ENAOKAY,FAILDEV,REG=4
                                                                SPACE 2
ENAOKAY  BR    R15                Return to caller
                                                                EJECT
***********************************************************************
*         Structure used by RAWIO identifying
*         the device and operation being performed
***********************************************************************
                                                                SPACE 2
IOCB_009 IOCB  X'009',CCW=CONPGM
                                                                EJECT
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE 2
         LTORG ,                Literals pool
                                                                SPACE
K        EQU   1024             One KB
PAGE     EQU   (4*K)            Size of one page
K64      EQU   (64*K)           64 KB
MB       EQU   (K*K)             1 MB
                                                                SPACE
TESTADDR EQU   (2*PAGE+X'200'-2)  Where test/subtest numbers will go
TIMEADDR EQU   (TESTADDR-1)       Address of timing tests option flag
                                                                SPACE
MAINSIZE EQU   (2*MB)                   Minimum required storage size
NUMPGTBS EQU   ((MAINSIZE+K64-1)/K64)   Number of Page Tables needed
NUMSEGTB EQU   ((NUMPGTBS*4)/(16*4))    Number of Segment Tables
SEGTABLS EQU   (3*PAGE)                 Segment Tables Origin
PAGETABS EQU   (SEGTABLS+(NUMPGTBS*4))  Page Tables Origin
CRLREG0  DC    0A(0),XL4'00B00060'      Control Register 0
CTLREG1  DC    A(SEGTABLS+NUMSEGTB)     Control Register 1
                                                                SPACE
NUMLOOPS DC    F'10000'         10,000 * 100 = 1,000,000
                                                                SPACE
BEGCLOCK DC    0D'0',8X'BB'     Begin
ENDCLOCK DC    0D'0',8X'EE'     End
DURATION DC    0D'0',8X'DD'     Diff
OVERHEAD DC    0D'0',8X'FF'     Overhead
                                                                SPACE
TICKSAAA DC    PL8'0'           Clock ticks high part
TICKSBBB DC    PL8'0'           Clock ticks low part
TICKSTOT DC    PL8'0'           Total clock ticks
                                                                SPACE
CONPGM   CCW1  X'09',PRTLINE,0,L'PRTLINE
PRTLINE  DC    C'         1,000,000 iterations of XXXXX took 999,999,999 microseconds'
EDIT     DC    X'402020206B2020206B202120'
                                                                EJECT
***********************************************************************
*        CLC Test Parameters:   A(operand-1),A(operand-2)
***********************************************************************
                                                                SPACE
CLC1     DC    A(1*K64),A(MB+(1*K64))                       both equal
CLC2     DC    A(1*K64),A(MB+(1*K64))                       both equal
CLCBOTH  DC    A(1*K64-12),A(MB+(1*K64)-34)                 both equal
CLCOP2   DC    A(1*K64),A(MB+(1*K64)-34)                    both equal
                                                                SPACE
CLC4     DC    A(2*K64),A(MB+(2*K64))                         op1 HIGH
CLC8     DC    A(3*K64),A(MB+(3*K64))                         op1 LOW!
CLC256   DC    A(4*K64),A(MB+(4*K64))                         op1 HIGH
CLCOP1   DC    A(5*K64-12),A(MB+(5*K64))                      op1 HIGH
                                                                SPACE 2
***********************************************************************
*        MVCIN Test Parameters
***********************************************************************
         PRINT DATA
INV1     DC    A(1*K64),A(MB+(1*K64)+1-1),A(1-1),A(MB+(1*K64))
INV2     DC    A(2*K64),A(MB+(2*K64)+2-1),A(2-1),A(MB+(2*K64))
INV4     DC    A(3*K64),A(MB+(3*K64)+4-1),A(4-1),A(MB+(3*K64))
INV8     DC    A(4*K64),A(MB+(4*K64)+8-1),A(8-1),A(MB+(4*K64))
INV256   DC    A(5*K64),A(MB+(5*K64)+256-1),A(256-1),A(MB+(5*K64))
                                                                SPACE
INVBOTH  DC    A(6*K64-12),A(MB+(6*K64)-34+256-1),A(256-1),A(MB+(6*K64)-34)
INVOP1   DC    A(7*K64-12),A(MB+(7*K64)+256-1),A(256-1),A(MB+(7*K64))
INVOP2   DC    A(8*K64),A(MB+(8*K64)-34+256-1),A(256-1),A(MB+(8*K64)-34)
         PRINT NODATA
MVCININ  DC    0XL256'00'
         DC    XL16'000102030405060708090A0B0C0D0E0F'
         DC    XL16'101112131415161718191A1B1C1D1E1F'
         DC    XL16'202122232425262728292A2B2C2D2E2F'
         DC    XL16'303132333435363738393A3B3C3D3E3F'
         PRINT OFF
         DC    XL16'404142434445464748494A4B4C4D4E4F'
         DC    XL16'505152535455565758595A5B5C5D5E5F'
         DC    XL16'606162636465666768696A6B6C6D6E6F'
         DC    XL16'707172737475767778797A7B7C7D7E7F'
         DC    XL16'808182838485868788898A8B8C8D8E8F'
         DC    XL16'909192939495969798999A9B9C9D9E9F'
         DC    XL16'A0A1A2A3A4A5A6A7A8A9AAABACADAEAF'
         DC    XL16'B0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF'
         DC    XL16'C0C1C2C3C4C5C6C7C8C9CACBCCCDCECF'
         DC    XL16'D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF'
         DC    XL16'E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF'
         DC    XL16'F0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF'
         PRINT ON
MVCINOUT DC    0XL256'00'
         DC    XL16'FFFEFDFCFBFAF9F8F7F6F5F4F3F2F1F0'
         DC    XL16'EFEEEDECEBEAE9E8E7E6E5E4E3E2E1E0'
         DC    XL16'DFDEDDDCDBDAD9D8D7D6D5D4D3D2D1D0'
         DC    XL16'CFCECDCCCBCAC9C8C7C6C5C4C3C2C1C0'
         PRINT OFF
         DC    XL16'BFBEBDBCBBBAB9B8B7B6B5B4B3B2B1B0'
         DC    XL16'AFAEADACABAAA9A8A7A6A5A4A3A2A1A0'
         DC    XL16'9F9E9D9C9B9A99989796959493929190'
         DC    XL16'8F8E8D8C8B8A89888786858483828180'
         DC    XL16'7F7E7D7C7B7A79787776757473727170'
         DC    XL16'6F6E6D6C6B6A69686766656463626160'
         DC    XL16'5F5E5D5C5B5A59585756555453525150'
         DC    XL16'4F4E4D4C4B4A49484746454443424140'
         DC    XL16'3F3E3D3C3B3A39383736353433323130'
         DC    XL16'2F2E2D2C2B2A29282726252423222120'
         DC    XL16'1F1E1D1C1B1A19181716151413121110'
         DC    XL16'0F0E0D0C0B0A09080706050403020100'
         PRINT ON
                                                                EJECT
***********************************************************************
*        TRTTEST DSECT
***********************************************************************
                                                                SPACE 2
TRTTEST  DSECT ,
                                                                SPACE 3
OP1DATA  DC    A(0)           Pointer to Operand-1 data
OP1LEN   DC    F'0'           How much data is there - 1
OP1WHERE DC    A(0)           Where Operand-1 data should be placed
                                                                SPACE 2
OP2DATA  DC    A(0)           Pointer to Operand-2 data
OP2LEN   DC    F'0'           How much data is there - 1
OP2WHERE DC    A(0)           Where Operand-2 data should be placed
                                                                SPACE 2
EXLEN    DC    F'0'           Operand-1 test length (EX instruction)
FAILMASK DC    A(0)           Failure Branch on Condition mask
                                                                SPACE 2
ENDREGS  DC    A(0),XL4'00'   Ending R1/R2 register values
                                                                SPACE 2
TRTNEXT  EQU   *              Start of next table entry...
                                                                SPACE 6
REG2PATT EQU   X'AABBCCDD'    Register 2 starting/ending CC0 value
REG2LOW  EQU         X'DD'    (last byte above)
                                                                SPACE 8
CLCLetal CSECT ,
                                                                EJECT
***********************************************************************
*        TRT Testing Control tables   (ref: TRTDSECT)
***********************************************************************
         PRINT DATA
TRTCTL   DC    0A(0)    start of table
                                                                SPACE 4
TRT1     DC    A(TRTOP10),A(001-1),A(00+(1*K64))
         DC    A(TRTOP20),A(256-1),A(MB+(1*K64))
         DC               A(001-1),A(7) CC0
         DC                        A(0),A(REG2PATT)
                                                                SPACE 4
TRT2     DC    A(TRTOP10),A(002-2),A(00+(2*K64))
         DC    A(TRTOP20),A(256-1),A(MB+(2*K64))
         DC               A(002-1),A(7) CC0
         DC                        A(0),A(REG2PATT)
                                                                SPACE 4
TRT4     DC    A(TRTOP10),A(004-1),A(00+(3*K64))
         DC    A(TRTOP20),A(256-1),A(MB+(3*K64))
         DC               A(004-1),A(7) CC0
         DC                        A(0),A(REG2PATT)
                                                                SPACE 4
TRT8     DC    A(TRTOP10),A(008-1),A(00+(4*K64))
         DC    A(TRTOP20),A(256-1),A(MB+(4*K64))
         DC               A(008-1),A(7) CC0
         DC                        A(0),A(REG2PATT)
                                                                EJECT
TRT256   DC    A(TRTOP10),A(256-1),A(00+(5*K64))
         DC    A(TRTOP20),A(256-1),A(MB+(5*K64))
         DC               A(256-1),A(7) CC0
         DC                        A(0),A(REG2PATT)
                                                                SPACE 4
TRTBTH   DC    A(TRTOP111),A(256-1),A(00+(6*K64)-12)  both cross page
         DC    A(TRTOP211),A(256-1),A(MB+(6*K64)-34)  both cross page
         DC                A(256-1),A(11) CC1 = stop, scan incomplete
         DC                         A(00+(6*K64)-12+X'11'),A(REG2PATT-REG2LOW+X'11')
                                                                SPACE 4
TRTOP1   DC    A(TRTOP1F0),A(256-1),A(00+(7*K64)-12)  only op1 crosses
         DC    A(TRTOP2F0),A(256-1),A(MB+(7*K64))
         DC                A(256-1),A(13) CC2 = stopped on last byte
         DC                         A(00+(7*K64)-12+255),A(REG2PATT-REG2LOW+X'F0')
                                                                SPACE 4
TRTOP2   DC    A(TRTOP111),A(256-1),A(00+(8*K64))
         DC    A(TRTOP211),A(256-1),A(MB+(8*K64)-34)  only op2 crosses
         DC                A(256-1),A(11) CC1 = stop, scan incomplete
         DC                         A(00+(8*K64)+X'11'),A(REG2PATT-REG2LOW+X'11')
                                                                SPACE 7
         DC    A(0)     end of table
                                                                EJECT
***********************************************************************
*        TRT op1 scan data...
***********************************************************************
                                                                SPACE
TRTOP10  DC    64XL4'78125634'    (CC0)
                                                                SPACE
TRTOP111 DC    04XL4'78125634',X'00110000',59XL4'78125634'    (CC1)
                                                                SPACE
TRTOP1F0 DC    63XL4'78125634',X'000000F0'    (CC2)
                                                                EJECT
***********************************************************************
*        TRT op2 stop tables...
***********************************************************************
                                                                SPACE
TRTOP20  DC    256X'00'     no stop
                                                                SPACE
TRTOP211 DC    17X'00',X'11',238X'00'     stop on X'11'
                                                                SPACE
TRTOP2F0 DC    240X'00',X'F0',15X'00'     stop on X'F0'
                                                                EJECT
***********************************************************************
*        CLCL Test Parameters
***********************************************************************
                                                                SPACE
CLCL1    DC    A(6*K64),A(1),A(MB+(6*K64)),A(1)             both equal
                                                                SPACE 2
CLCL2    DC    A(6*K64),A(2),A(MB+(6*K64)),A(2)             both equal
                                                                SPACE 2
CLCL256  DC    A(6*K64),A(256),A(MB+(6*K64)),A(256)         both equal
                                                                SPACE 2
CLCL1K   DC    A(6*K64),A(K),A(MB+(6*K64)),A(K)             both equal
                                                                SPACE 2
CLCLBOTH DC    A(6*K64-12),A(K64),A(MB+(6*K64)-34),A(K64)   both equal
                                                                SPACE 2
CLCLOP2  DC    A(6*K64),A(PAGE),A(MB+(6*K64)-34),A(K64)     both equal
                                                                SPACE 2
CLCL4    DC    A(7*K64),A(4),A(MB+(7*K64)),A(4)               op1 HIGH
                                                                SPACE 2
CLCL8    DC    A(8*K64),A(8),A(MB+(8*K64)),A(8)               op1 LOW!
                                                                SPACE 2
CLCLOP1  DC    A(9*K64-12),A(K64),A(MB+(9*K64)),A(PAGE)       op1 HIGH
                                                                SPACE 2
CLCLPF   DC    A(10*K64),A(K64),A(MB+(10*K64)),A(K64)       page fault
                                                                EJECT
***********************************************************************
*        CLCL Expected Ending Register Values
***********************************************************************
                                                                SPACE
ECLCL1   DC    A(6*K64+1),A(0),A(MB+(6*K64)+1),A(0)         both equal
                                                                SPACE 2
ECLCL2   DC    A(6*K64+2),A(0),A(MB+(6*K64)+2),A(0)         both equal
                                                                SPACE 2
ECLCL256 DC    A(6*K64+256),A(0),A(MB+(6*K64)+256),A(0)     both equal
                                                                SPACE 2
ECLCL1K  DC    A(6*K64+K),A(0),A(MB+(6*K64)+K),A(0)         both equal
                                                                SPACE 2
ECLCLBTH DC    A(6*K64-12+K64),A(0),A(MB+(6*K64)-34+K64),A(0) bth equl
                                                                SPACE 2
ECLCLOP2 DC    A(6*K64+PAGE),A(0),A(MB+(6*K64)-34+K64),A(0) both equal
                                                                SPACE 2
ECLCL4   DC    A(7*K64+4-1),A(1),A(MB+(7*K64)+4-1),A(1)       op1 HIGH
                                                                SPACE 2
ECLCL8   DC    A(8*K64+8-1),A(1),A(MB+(8*K64)+8-1),A(1)       op1 LOW!
                                                                SPACE 2
ECLCLOP1 DC    A(9*K64-12+K64-1),A(1),A(MB+(9*K64)+PAGE),A(0) op1 HIGH
                                                                SPACE 2
ECLCLPF  DC    A(10*K64+K64),A(0),A(MB+(10*K64)+K64),A(0)   page fault
                                                                SPACE 3
CLCLEND  DC    4F'0'          (actual ending register values)
PFPAGE   EQU   5              (page the Page Fault should occur on)
PFPGBYTS EQU   (PFPAGE*PAGE)  (number of bytes into operand-1)
                                                                EJECT
***********************************************************************
*        Fixed storage locations
***********************************************************************
                                                                SPACE 5
         ORG   CLCLetal+TIMEADDR      (s/b @ X'21FD')
                                                                SPACE
TIMEOPT  DC    X'00'      Set to non-zero to run timing tests
                                                                SPACE 9
         ORG   CLCLetal+TESTADDR      (s/b @ X'21FE', X'21FF')
                                                                SPACE
TESTNUM  DC    X'00'      Test number of active test
SUBTEST  DC    X'00'      Active test sub-test number
                                                                SPACE 9
         ORG   CLCLetal+SEGTABLS      (s/b @ X'3000')
                                                                SPACE
DATTABS  DC    X'00'      Segment and Page Tables will go here...
                                                                EJECT
***********************************************************************
*        IOCB DSECT
***********************************************************************
                                                                SPACE
         DSECTS NAME=IOCB
                                                                EJECT
***********************************************************************
*        ORB DSECT
***********************************************************************
                                                                SPACE
         DSECTS NAME=ORB
                                                                EJECT
***********************************************************************
*        IRB DSECT
***********************************************************************
                                                                SPACE
         DSECTS NAME=IRB
                                                                EJECT
***********************************************************************
*        SCSW DSECT
***********************************************************************
                                                                SPACE
         DSECTS NAME=SCSW
                                                                EJECT
***********************************************************************
*        (other DSECTS needed by SATK)
***********************************************************************
                                                                SPACE
         DSECTS PRINT=OFF,NAME=(ASA,SCHIB,CCW0,CCW1,CSW)
         PRINT ON
                                                                SPACE
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
