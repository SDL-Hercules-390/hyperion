 TITLE '            TRTR-02-performance (Test TRTR instructions)'
***********************************************************************
*
*              TRTR instruction tests
*
*        NOTE: This test is based the CLCL-et-al Test
*              modified to only test the Performance
*              of the TRTR instruction.
*
*              The MSG routine is from the Hercules Binary
*              Floating Point Validation Package by Stephen R. Orso

*                         ********************
*                         **   IMPORTANT!   **
*                         ********************
*
*              This test uses the Hercules Diagnose X'008' interface
*              to display messages and thus your .tst runtest script
*              MUST contain a "DIAG8CMD ENABLE" statement within it!
*
*        James Wekel November 2022
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*            TRTR Performance instruction tests
*
***********************************************************************
*
*  This program ONLY tests the performance of the TRTR
*  instructions.
*
*     Tests:
*
*           All tests are ' TRTR  0(255,R3),0(R5) '
*           where operand-1 is 256 bytes.
*
*           1. TRTR with CC=0 - no crossed pages
*           2. TRTR with CC=1 - both operand-1 and
*                  function code table cross page boundaries.
*           3. TRTR with CC=2 - both operand-1 and
*                  function code table cross page boundaries.
*
***********************************************************************
                                                                EJECT
***********************************************************************
*
*  Example Hercules Testcase:
*
*
*        *Testcase TRTR-02-performance (Test TRTR instructions)
*        mainsize   16
*        numcpu     1
*        sysclear
*        archlvl    z/Arch
*
*        loadcore   "$(testpath)/TRTR-02-performance.core" 0x0
*
*        diag8cmd   enable  # (needed for messages to Hercules console)
*        #r         408=ff  # (enable timing tests)
*        runtest    300     # (test duration, depends on host)
*        diag8cmd   disable # (reset back to default)
*
*        *Done
*
*
***********************************************************************
                                                                SPACE 4
***********************************************************************
*               Low Core Definitions
***********************************************************************
*
TRTR2TST START 0
         USING TRTR2TST,R0            Low core addressability
                                                                SPACE 3
         ORG   TRTR2TST+X'1A0'        z/Architecure RESTART PSW
         DC    X'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE 2
         ORG   TRTR2TST+X'1D0'        z/Architecure PROGRAM CHECK PSW
         DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                SPACE 3
         ORG   TRTR2TST+X'200'        Start of actual test program...
                                                                EJECT
***********************************************************************
*               The actual "TRTR2TST" program itself...
***********************************************************************
*
*  Architecture Mode: 370
*  Register Usage:
*
*   R0       (work)
*   R1       (work)
*   R2       (work) or MSG subroutine call
*   R3       (work)
*   R4       (work)
*   R5       TRTRTEST Base (of current test)
*   R5-R7    (work)
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
         USING  BEGIN+4096,R9    SECOND Base Register
                                                                SPACE
BEGIN    BALR  R8,0             Initalize FIRST base register
         BCTR  R8,0             Initalize FIRST base register
         BCTR  R8,0             Initalize FIRST base register
                                                                SPACE
         LA    R9,2048(,R8)     Initalize SECOND base register
         LA    R9,2048(,R9)     Initalize SECOND base register
                                                                SPACE
*
**       Run the performance tests...
*
         BAL   R14,TEST91       Time TRTR   instruction  (speed test)
                                                                EJECT
***********************************************************************
*         Test for normal or unexpected test completion...
***********************************************************************
                                                                SPACE
         CLI   TIMEOPT,X'FF'    Was this a timing run?
         BNE   EOJ              No, timing run; just go end normally
                                                                SPACE
         CLI   TESTNUM,X'25'    Did we end on expected test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         CLI   SUBTEST,X'99'    Did we end on expected SUB-test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         B     EOJ              Yes, then normal completion!
                                                                SPACE 4
***********************************************************************
*        Fixed test storage locations ...
***********************************************************************
                                                               SPACE 2
         ORG   BEGIN+X'200'

TESTADDR DS    0D                Where test/subtest numbers will go
TESTNUM  DC    X'99'      Test number of active test
SUBTEST  DC    X'99'      Active test sub-test number
                                                               SPACE 2
         DS    0D
TIMEOPT  DC    X'00'      Set to non-zero to run timing tests
                                                               SPACE 2
         DS    0D
SAVE3T5  DC    4F'0'
SAVER2   DC    F'0'
SAVER13   DC    F'0'
                                                               SPACE 2
         ORG   *+X'100'
                                                                EJECT
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
         LM    R3,R5,OPSPERF        Get TRTR operands
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
         LM    R3,R5,OPSPERF        Load TRTR operands
         TRTR  0(255,R3),0(R5)      Do TRTR
.*
&CTR     SETA  &CTR-1
         AIF   (&CTR GT 0).LOOP
         MEND
                                                                EJECT
***********************************************************************
*        TEST91                 Time TRTR instruction  (speed test)
***********************************************************************
                                                                SPACE
TEST91   TM    TIMEOPT,X'FF'    Is timing tests option enabled?
         BZR   R14              No, skip timing tests
                                                                SPACE
         LA    R13,TRTRPERF         Point R13 --> testing control table
         USING TRTRTEST,R13         What each table entry looks like
*
TST91LOP EQU   *
         ST    R13,SAVER13          save current pref table base
*
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
         L     R7,OP2LEN            How much of it there is
         MVCL  R10,R6
                                                                SPACE 2
         LM    R1,R5,OPSWHERE     get TRTR input; set OP addr to end
         AR    R3,R4              add OP length -1
         BCTR  R3,0
         STM   R3,R5,OPSPERF      save for preformance test
                                                                EJECT
***********************************************************************
*        Next, time the overhead...
***********************************************************************
                                                                SPACE
         L     R7,NUMLOOPS
         STCK  BEGCLOCK
         STM   R3,R5,SAVE3T5
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
*
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
                                                               SPACE 2
         BCTR  R7,R6
         STCK  ENDCLOCK
                                                                SPACE 2
         LM    R3,R5,SAVE3T5
         MVC   PRTLINE+33(5),=CL5'TRTR'
         BAL   R15,RPTSPEED
*
*  more performance tests?
*
         L     R13,SAVER13          restore perf table base
         LA    R13,TRTRNEXT        Go on to next table entry
         CLC   =F'0',0(R13)        End of table?
         BNE   TST91LOP           No, loop...
         BR    R14                Return to caller or FAILTEST
                                                                SPACE 2
OPSPERF  DS    4D                 Performance test R3-R5
                                                                EJECT
***********************************************************************
*        RPTSPEED                 Report instruction speed
***********************************************************************
                                                               SPACE
RPTSPEED ST    R15,RPTSAVE        Save return address
         ST    R5,RPTSVR5         Save R5
*
         BAL   R15,CALCDUR        Calculate duration
*
         LA    R5,OVERHEAD        Subtract overhead
         LA    R6,DURATION        From raw timing
         LA    R7,DURATION        Yielding true instruction timing
         BAL   R15,SUBDWORD       Do it
*
         LM    R10,R11,DURATION   Convert to...
         SRDL  R10,12             ... microseconds
*
         CVD   R10,TICKSAAA       convert HIGH part to decimal
         CVD   R11,TICKSBBB       convert LOW  part to decimal
*
         ZAP   TICKSTOT,TICKSAAA            Calculate...
         MP    TICKSTOT,=P'4294967296'      ...decimal...
         AP    TICKSTOT,TICKSBBB            ...microseconds
*
         MVC   PRTLINE+43(L'EDIT),EDIT          (edit into...
         ED    PRTLINE+43(L'EDIT),TICKSTOT+3     ...print line)
                                                               SPACE 3
*
*        Use Hercules Diagnose for Message to console
*
         STM   R0,R2,RPTDWSAV       save regs used by MSG
         LA    R0,PRTLNG            message length
         LA    R1,PRTLINE           messagfe address
         BAL   R2,MSG               call Hercules console MSG display
         LM    R0,R2,RPTDWSAV       restore regs
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
SUBDWORD STM   R1,R4,SUBDWSAV   Save registers
*
         LM    R1,R2,0(R5)        Subtrahend  (value to subtract)
         LM    R3,R4,0(R6)        Minuend     (what to subtract FROM)
         SLR   R4,R2              Subtract LOW part
         BNM   *+4+4                (branch if no borrow)
         SL    R3,=F'1'             (otherwise do borrow)
         SLR   R3,R1              Subtract HIGH part
         STM   R3,R4,0(R7)        Store results
*
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
                                                                SPACE 5
EOJPSW   DC    0D'0',X'0002000180000000',AD(0)
                                                                SPACE
EOJ      LPSWE EOJPSW               Normal completion
                                                                SPACE 5
FAILPSW  DC    0D'0',X'0002000180000000',AD(X'BAD')
                                                                SPACE
FAILTEST LPSWE FAILPSW              Abnormal termination
                                                                EJECT
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
TRTR2TST CSECT ,
***********************************************************************
*        TRTR Performace Test data...
***********************************************************************
TRTRPERF DC    0A(0)      start of table
                                                                SPACE 3
***********************************************************************
*        tests with op-1 length 256
***********************************************************************
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
                                                                SPACE 4
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
                                                                EJECT
CC2T5    DS    0F
         DC    X'25'                       Test Num
         DC    X'00',X'00'
         DC    X'00'
*
         DC    A(TRTOP1F1),A(256)          Source - Op 1 & length
         DC    A(TRTOP8F1),A(256)          Source - FC Table & length
*                                          Target -
         DC    A(REG2PATT),A(REG2PATT)               GR1, GR2
         DC    A(5*MB+(4*K16)-33),A(0),A(6*MB+(4*K16)-41)  Op1,.., FCT
*
         DC    A(13)                        not CC2
         DC    A(5*MB+(4*K16)-33),XL4'AABBCCF1' FC address, Code
                                                                SPACE 2

         DC    A(0)        end of table
         DC    A(0)        end of table
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
