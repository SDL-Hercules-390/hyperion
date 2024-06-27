 TITLE '            TRTE-02-performance (Test TRTE instructions)'
***********************************************************************
*
*                TRTE Performance instruction tests
*
***********************************************************************
*
*  This program ONLY tests the performance of the TRTE instructions.
*
*
*                     ********************
*                     **   IMPORTANT!   **
*                     ********************
*
*        This test uses the Hercules Diagnose X'008' interface
*        to display messages and thus your .tst runtest script
*        MUST contain a "DIAG8CMD ENABLE" statement within it!
*
*
*  NOTE: This test is based on the CLCL-et-al Test but modified to
*        only test the TRTE instruction. -- James Wekel October 2022
*
***********************************************************************
*
*  Example Hercules Testcase:
*
*
*      *Testcase TRTE-02-performance (Test TRTE instructions)
*
*      mainsize    16
*      numcpu      1
*      sysclear
*      archlvl     z/Arch
*      loadcore    "$(testpath)/TRTE-02-performance.core" 0x0
*      diag8cmd    enable    # (needed for messages to Hercules console)
*      #r           408=ff    # (enable timing tests)
*      runtest     200       # (test duration, depends on host)
*      diag8cmd    disable   # (reset back to default)
*      *Done
*
*
***********************************************************************
                                                                EJECT
***********************************************************************
*
*  Tests:
*
*        All tests are ' TRTE  R2,R4,12 '
*        where the FC table is 128K in length,
*        FC is 2 bytes and an argument length of 2 bytes.
*
*        M3=12 requires page crossover tests for both FC and
*        the argument and has the worst performance compared to
*        M3=0 with the FC table and operand contained within
*        a page. The test should provide a lower bound on
*        performance improvement.
*
*        1. TRTE of 512 bytes
*        2. TRTE of 512 bytes that crosses a page boundary,
*           which results in a CC=3, and a branch back
*           to complete the TRTE instruction.
*        3. TRTE of 2048 bytes
*        4. TRTE of 2048 bytes that crosses a page boundary,
*           which results in a CC=3, and a branch back
*           to complete the TRTE instruction
*
***********************************************************************
                                                                SPACE 3
TRTE2TST START 0
         USING TRTE2TST,R0            Low core addressability
                                                                SPACE 4
         ORG   TRTE2TST+X'1A0'        z/Architecure RESTART PSW
         DC    X'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE 2
         ORG   TRTE2TST+X'1D0'        z/Architecure PROGRAM CHECK PSW
         DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                SPACE 4
         ORG   TRTE2TST+X'200'        Start of actual test program...
                                                                EJECT
***********************************************************************
*               The actual "TRTE2TST" program itself...
***********************************************************************
*
*  Architecture Mode: z/Arch
*  Register Usage:
*
*   R0       (work)
*   R1       (work)
*   R2       (work) or MSG subroutine call
*   R3       (work)
*   R4       (work)
*   R5       TRTETEST Base (of current test)
*   R5-R7    (work)
*   R8       (work)
*   R9       Second base register
*   R10-R12  (work)
*   R13      First base register
*   R14      Subroutine call
*   R15      Secondary Subroutine call or work
*
***********************************************************************
                                                                SPACE
         USING  BEGIN,R13        FIRST Base Register
         USING  BEGIN+4096,R9    SECOND Base Register
                                                                SPACE
BEGIN    BALR  R13,0             Initalize FIRST base register
         BCTR  R13,0             Initalize FIRST base register
         BCTR  R13,0             Initalize FIRST base register
                                                                SPACE
         LA    R9,2048(,R13)     Initalize SECOND base register
         LA    R9,2048(,R9)      Initalize SECOND base register
                                                                SPACE
***********************************************************************
*        Run the performance test(s)...
***********************************************************************
                                                                SPACE
         BAL   R14,TEST91       Time TRTE instruction (speed test)
                                                                SPACE 2
***********************************************************************
*         Test for normal or unexpected test completion...
***********************************************************************
                                                                SPACE
         CLI   TIMEOPT,X'FF'    Was this a timing run?
         BNE   EOJ              No, timing run; just go end normally
                                                                SPACE
         CLI   TESTNUM,X'FC'    Did we end on expected test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         CLI   SUBTEST,X'99'    Did we end on expected SUB-test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         B     EOJ              Yes, then normal completion!
                                                                EJECT
***********************************************************************
*        Fixed test storage locations ...
***********************************************************************
                                                                SPACE 2
         ORG   TRTE2TST+X'400'
                                                                SPACE 4
TESTADDR DS    0D               Where test/subtest numbers will go
TESTNUM  DC    X'99'            Test number of active test
SUBTEST  DC    X'99'            Active test sub-test number
                                                                SPACE 2
         DS    0D
TIMEOPT  DC    X'00'            Set to non-zero to run timing tests
                                                                SPACE 2
         DS    0D
SAVE1T4  DC    4F'0'
SAVER2   DC    F'0'
SAVER5   DC    F'0'
                                                                SPACE 4
         ORG   *+X'100'
                                                                EJECT
***********************************************************************
*        TEST91                 Time TRTE instruction  (speed test)
***********************************************************************
                                                                SPACE
TEST91   TM    TIMEOPT,X'FF'        Is timing tests option enabled?
         BZR   R14                  No, skip timing tests
                                                                SPACE
         LA    R5,TRTEPERF          Point R5 --> testing control table
         USING TRTETEST,R5          What each table entry looks like
                                                                SPACE
TST91LOP EQU   *
         ST    R5,SAVER5            Save current pref table base
                                                                SPACE
         IC    R6,TNUM              Set test number
         STC   R6,TESTNUM
*
**       Initialize operand data  (move data to testing address)
*
         L     R10,OP1WHERE         Where to move operand-1 data to
         L     R11,OP1LEN           Get operand-1 length
         ST    R11,OP1WLEN          and save for later
         L     R6,OP1DATA           Where op1 data is right now
         L     R7,OP1LEN            How much of it there is
         MVCL  R10,R6
                                                                SPACE
         L     R10,OP2WHERE         Where to move operand-2 data to
         L     R11,OP2LEN           How much of it there is
         L     R6,OP2DATA           Where op2 data is right now
         L     R7,OP2LEN            How much of it there is
         MVCL  R10,R6
                                                                                                                                EJECT
                                                                SPACE 3
***********************************************************************
*     Define come helpful macros to ensure our counts are correct
***********************************************************************
                                                                SPACE 4
         MACRO
         OVERONLY &NUM              &NUM = number of sets
         LCLA  &CTR
&CTR     SETA  &NUM
.LOOP    ANOP
.*
*
         LM    R1,R4,OPSWHERE
         BC    B'0001',*+4
.*
&CTR     SETA  &CTR-1
         AIF   (&CTR GT 0).LOOP
         MEND
                                                                SPACE 5
         MACRO
         DOINSTR &NUM               &NUM = number of sets
         LCLA  &CTR
&CTR     SETA  &NUM
.LOOP    ANOP
.*
*
         LM    R1,R4,OPSWHERE
         TRTE  R2,R4,12
         BC    B'0001',*-4
.*
&CTR     SETA  &CTR-1
         AIF   (&CTR GT 0).LOOP
         MEND
                                                                EJECT
***********************************************************************
*        Next, time the overhead...
***********************************************************************
                                                                SPACE
         L     R7,NUMLOOPS
         STCK  BEGCLOCK
         STM   R1,R4,SAVE1T4
         BALR  R6,0
*                                   100 sets of overhead
         OVERONLY 2                 (first 2)
                                                                SPACE
*        .........ETC.........
                                                                SPACE
         PRINT OFF
         OVERONLY 96                (3-98)
         PRINT ON
                                                                SPACE
         OVERONLY 2                 (last 2)
                                                                SPACE
         BCTR  R7,R6
         STCK  ENDCLOCK
         BAL   R15,CALCDUR
         MVC   OVERHEAD,DURATION
                                                                EJECT
***********************************************************************
*        Now do the actual timing run...
***********************************************************************
                                                                SPACE
         L     R7,NUMLOOPS
         STCK  BEGCLOCK
         BALR  R6,0
*                                   100 sets of instructions
         DOINSTR 2                  (first 2)
                                                                SPACE
*        .........ETC.........
                                                                SPACE
         PRINT OFF
         DOINSTR 96                 (3-98)
         PRINT ON
                                                                SPACE
         DOINSTR 2                  (last 2)
                                                                SPACE
         BCTR  R7,R6
         STCK  ENDCLOCK
                                                                SPACE
         LM    R1,R4,SAVE1T4
         MVC   PRTLINE+33(5),=CL5'TRTE'
         BAL   R15,RPTSPEED
*
**       More performance tests?
*
         L     R5,SAVER5          Restore perf table base
         LA    R5,TRTENEXT        Go on to next table entry
         CLC   =F'0',0(R5)        End of table?
         BNE   TST91LOP           No, loop...
         BR    R14                Return to caller or FAILTEST
                                                                EJECT
***********************************************************************
*        RPTSPEED                 Report instruction speed
***********************************************************************
                                                                SPACE
RPTSPEED ST    R15,RPTSAVE        Save return address
         ST    R5,RPTSVR5         Save R5
                                                                SPACE
         BAL   R15,CALCDUR        Calculate duration
                                                                SPACE
         LA    R5,OVERHEAD        Subtract overhead
         LA    R6,DURATION        From raw timing
         LA    R7,DURATION        Yielding true instruction timing
         BAL   R15,SUBDWORD       Do it
                                                                SPACE
         LM    R10,R11,DURATION   Convert to...
         SRDL  R10,12             ... microseconds
                                                                SPACE
         CVD   R10,TICKSAAA       Convert HIGH part to decimal
         CVD   R11,TICKSBBB       Convert LOW  part to decimal
                                                                SPACE
         ZAP   TICKSTOT,TICKSAAA            Calculate...
         MP    TICKSTOT,=P'4294967296'      ...decimal...
         AP    TICKSTOT,TICKSBBB            ...microseconds
                                                                SPACE
         MVC   PRTLINE+43(L'EDIT),EDIT          (edit into...
         ED    PRTLINE+43(L'EDIT),TICKSTOT+3     ...print line)
                                                                SPACE 3
*
*        Use Hercules Diagnose for Message to console
*
         STM   R0,R2,RPTDWSAV     Save regs used by MSG
         LA    R0,PRTLNG          Message length
         LA    R1,PRTLINE         Message address
         BAL   R2,MSG             Call Hercules console MSG display
         LM    R0,R2,RPTDWSAV     Restore regs
                                                                SPACE 2
         L     R5,RPTSVR5         Restore R5
         L     R15,RPTSAVE        Restore return address
         BR    R15                Return to caller
                                                                SPACE
RPTSAVE  DC    F'0'               R15 save area
RPTSVR5  DC    F'0'               R5 save area
                                                                SPACE
RPTDWSAV DC    2D'0'              R0-R2 save area for MSG call
                                                                EJECT
***********************************************************************
*        CALCDUR                  Calculate DURATION
***********************************************************************
                                                                SPACE
CALCDUR  ST    R15,CALCRET        Save return address
         STM   R5,R7,CALCWORK     Save work registers
                                                                SPACE
         LM    R6,R7,BEGCLOCK     Remove CPU number from clock value
         SRDL  R6,6                            "
         SLDL  R6,6                            "
         STM   R6,R7,BEGCLOCK                  "
                                                                SPACE
         LM    R6,R7,ENDCLOCK     Remove CPU number from clock value
         SRDL  R6,6                            "
         SLDL  R6,6                            "
         STM   R6,R7,ENDCLOCK                  "
                                                                SPACE
         LA    R5,BEGCLOCK        Starting time
         LA    R6,ENDCLOCK        Ending time
         LA    R7,DURATION        Difference
         BAL   R15,SUBDWORD       Calculate duration
                                                                SPACE
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
SUBDWORD STM   R1,R4,SUBDWSAV     Save registers
                                                                SPACE
         LM    R1,R2,0(R5)        Subtrahend  (value to subtract)
         LM    R3,R4,0(R6)        Minuend     (what to subtract FROM)
         SLR   R4,R2              Subtract LOW part
         BNM   *+4+4              (branch if no borrow)
         SL    R3,=F'1'           (otherwise do borrow)
         SLR   R3,R1              Subtract HIGH part
         STM   R3,R4,0(R7)        Store results
                                                                SPACE
         LM    R1,R4,SUBDWSAV     Restore registers
         BR    R15                Return to caller
                                                                SPACE
SUBDWSAV DC    2D'0'              R1-R4 save area
                                                                EJECT
***********************************************************************
*        Issue HERCULES MESSAGE pointed to by R1, length in R0
*              R2 = return address
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
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 2
EOJPSW   DC    0D'0',X'0002000180000000',AD(0)
                                                                SPACE
EOJ      LPSWE EOJPSW               Normal completion
                                                                SPACE 3
FAILPSW  DC    0D'0',X'0002000180000000',AD(X'BAD')
                                                                SPACE
FAILTEST LPSWE FAILPSW              Abnormal termination
                                                                SPACE 4
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE
         LTORG ,                Literals pool
                                                                SPACE 2
K        EQU   1024             One KB
PAGE     EQU   (4*K)            Size of one page
K64      EQU   (64*K)           64 KB
MB       EQU   (K*K)             1 MB
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
PRTLINE  DC    C'         1,000,000 iterations of XXXXX'
         DC    C' took 999,999,999 microseconds'
PRTLNG   EQU   *-PRTLINE
EDIT     DC    X'402020206B2020206B202120'
                                                                EJECT
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
*        TRTE Performace Test data...
***********************************************************************
                                                                SPACE
TRTE2TST CSECT ,
TRTEPERF DC    0A(0)                      Start of table
                                                                SPACE 4
***********************************************************************
*        tests with   M3: A=1,F=1,L=0, reserved=0    (12)
*                     FC Table : SIZE: 131,072 (2 BYTE ARGUMENT)
*                                Function Code is 2 bytes
*
*              Note: Op1 length must be a multiple of 2
***********************************************************************
                                                                SPACE 3
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
                                                                SPACE 5
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
                                                                EJECT
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
                                                                SPACE 5
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
                                                                SPACE 6
         DC    A(0)        end of table
         DC    A(0)        end of table
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
                                                                SPACE 4
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
