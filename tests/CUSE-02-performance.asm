 TITLE '            CUSE-02-performance (Test CUSE instructions)'
***********************************************************************
*
*                CUSE Performance instruction tests
*
***********************************************************************
*
*  This program ONLY tests the performance of the CUSE instructions.
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
*        only test the CUSE instruction. -- James Wekel November 2022
*
***********************************************************************
*
*  Example Hercules Testcase:
*
*
*      *Testcase CUSE-02-performance (Test CUSE instructions)
*
*      mainsize    16
*      numcpu      1
*      sysclear
*      archlvl     z/Arch
*      loadcore    "$(testpath)/CUSE-02-performance.core" 0x0
*      diag8cmd    enable   # (needed for messages to Hercules console)
*      #r           408=ff  # (enable timing tests)
*      runtest     500      # (test duration, depends on host)
*      diag8cmd    disable  # (reset back to default)
*      *Done
*
*
***********************************************************************
                                                                EJECT
***********************************************************************
*
*  Tests:
*        Both opernand-1 and operand-2 cross a page boundary.
*
*        1. CUSE of 512 bytes - substring length 1
*        2. CUSE of 512 bytes - substring length 4
*        3. CUSE of 512 bytes - substring length 8
*        4. CUSE of 512 bytes - substring length 32
*        5. CUSE of 512 bytes - substring length 32 (different strings)
*        6. CUSE of 4160 (4096+64) bytes - substring length 32
*            which results in a CC=3, and a branch back
*            to complete the CUSE instruction.
*
***********************************************************************
                                                                SPACE 3
CUSE2TST START 0
         USING CUSE2TST,R0            Low core addressability
                                                                SPACE 4
         ORG   CUSE2TST+X'1A0'        z/Architecure RESTART PSW
         DC    X'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE 2
         ORG   CUSE2TST+X'1D0'        z/Architecure PROGRAM CHECK PSW
         DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                SPACE 4
         ORG   CUSE2TST+X'200'        Start of actual test program...
                                                                EJECT
***********************************************************************
*               The actual "CUSE2TST" program itself...
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
*   R6       CUSETEST Base (of current test)
*   R7       (work)
*   R8       First base register
*   R9       Second base register
*   R10-R13  (work)
*   R14      Subroutine call
*   R15      Secondary Subroutine call or work
*
***********************************************************************
                                                                SPACE
         USING  BEGIN,R8        FIRST Base Register
         USING  BEGIN+4096,R9    SECOND Base Register
                                                                SPACE
BEGIN    BALR  R8,0              Initalize FIRST base register
         BCTR  R8,0              Initalize FIRST base register
         BCTR  R8,0              Initalize FIRST base register
                                                                SPACE
         LA    R9,2048(,R8)      Initalize SECOND base register
         LA    R9,2048(,R9)      Initalize SECOND base register
                                                                SPACE
***********************************************************************
*        Run the performance test(s)...
***********************************************************************
                                                                SPACE
         BAL   R14,TEST91       Time CUSE instruction (speed test)
                                                                SPACE 2
***********************************************************************
*         Test for normal or unexpected test completion...
***********************************************************************
                                                                SPACE
         CLI   TIMEOPT,X'FF'    Was this a timing run?
         BNE   EOJ              No, timing run; just go end normally
                                                                SPACE
         CLI   TESTNUM,X'F4'    Did we end on expected test?
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
         ORG   CUSE2TST+X'400'
                                                                SPACE 4
TESTADDR DS    0D               Where test/subtest numbers will go
TESTNUM  DC    X'99'            Test number of active test
SUBTEST  DC    X'99'            Active test sub-test number
                                                                SPACE 2
         DS    0D
TIMEOPT  DC    X'00'            Set to non-zero to run timing tests
                                                                SPACE 2
         DS    0D
SAVE2T5  DC    4F'0'
SAVER2   DC    F'0'
SAVER6   DC    F'0'
                                                                SPACE 4
         ORG   *+X'100'
                                                                EJECT
***********************************************************************
*        TEST91                 Time CUSE instruction  (speed test)
***********************************************************************
                                                                SPACE
TEST91   TM    TIMEOPT,X'FF'        Is timing tests option enabled?
         BZR   R14                  No, skip timing tests
                                                                SPACE
         LA    R6,CUSEPERF          Point R5 --> testing control table
         USING CUSETEST,R6          What each table entry looks like
                                                                SPACE
TST91LOP EQU   *
         ST    R6,SAVER6            Save current pref table base
                                                                SPACE
         IC    R7,TNUM              Set test number
         STC   R7,TESTNUM
*
**       Initialize operand data  (move data to testing address)
*
*              Build Operand-1
                                                                SPACE
         L     R2,OP1WHERE        Where to move operand-1 data to
         L     R3,OP1LEN          Get operand-1 length
         L     R10,SS1ADDR        Calculate OP 1 starting
         SR    R10,R3               address
         A     R10,SS1LEN
         L     R11,OP1LEN
         MVCL  R2,R10
                                                                SPACE
         BCTR  R2,0               less one for last char addr
         MVC   0(0,R2),SS1LAST    set last char
                                                                SPACE
*              Build Operand-2
                                                                SPACE
         L     R4,OP2WHERE        Where to move operand-1 data to
         L     R5,OP2LEN          Get operand-1 length
         L     R10,SS2ADDR        Calculate OP 2 starting
         SR    R10,R5               address
         A     R10,SS2LEN
         L     R11,OP2LEN
         MVCL  R4,R10
                                                                SPACE
         BCTR  R4,0               less one for last char addr
         MVC   0(0,R4),SS2LAST    set last char
                                                                SPACE
*              Set Substring length and pad byte
                                                                SPACE
         IC    R0,SSLEN           Set SS length
         IC    R1,PAD             Set SS Pad byte
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
         LM    R2,R5,OPSWHERE
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
         LM    R2,R5,OPSWHERE
         CUSE  R2,R4
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
         STM   R2,R5,SAVE2T5
         BALR  R10,0
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
         BCTR  R7,R10
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
         BALR  R10,0
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
         BCTR  R7,R10
         STCK  ENDCLOCK
                                                                SPACE
         LM    R2,R5,SAVE2T5
         MVC   PRTLINE+33(5),=CL5'CUSE'
         BAL   R15,RPTSPEED
*
**       More performance tests?
*
         L     R6,SAVER6          Restore perf table base
         LA    R6,CUSENEXT        Go on to next table entry
         CLC   =F'0',0(R6)        End of table?
         BNE   TST91LOP           No, loop...
         BR    R14                Return to caller or FAILTEST
                                                                EJECT
***********************************************************************
*        RPTSPEED                 Report instruction speed
***********************************************************************
                                                                SPACE
RPTSPEED ST    R15,RPTSAVE        Save return address
         STM   R5,R7,RPTSVR5T7    Save R5-7
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
         LM    R5,R7,RPTSVR5T7    Restore R5-7
         L     R15,RPTSAVE        Restore return address
         BR    R15                Return to caller
                                                                SPACE
RPTSAVE  DC    F'0'               R15 save area
RPTSVR5T7  DC  3F'0'              R5-R7 save area
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
***********************************************************************
*        CUSETEST DSECT
***********************************************************************
                                                                SPACE 2
CUSETEST DSECT ,
TNUM     DC    X'00'          CUSE table number
         DC    XL3'00'
                                                                SPACE 2
SSLEN    DC    AL1(0)         CUSE - SS length
PAD      DC    X'00'          CUSE - Pad byte
SS1LAST  DC    X'00'          First-Operand SS last byte
SS2LAST  DC    X'00'          Second-Operand SS last byte
                                                                SPACE 2
SS1ADDR  DC    A(0)           First-Operand SS Address
SS1LEN   DC    A(0)           First-Operand SS length
SS2ADDR  DC    A(0)           Second-Operand SS Address
SS2LEN   DC    A(0)           Second-Operand SS length
                                                                SPACE 2
OPSWHERE EQU   *
OP1WHERE DC    A(0)           Where Operand-1 data should be placed
OP1LEN   DC    F'0'           CUSE - First-Operand Length
OP2WHERE DC    A(0)           Where Operand-2 data should be placed
OP2LEN   DC    F'0'           CUSE - Second-Operand Length

                                                                SPACE 2
FAILMASK DC    A(0)           Failure Branch on Condition mask
                                                                SPACE 2
*                             Ending register values
ENDOP1   DC    A(0)              Operand 1 address
         DC    A(0)              Operand 1 length
ENDOP2   DC    A(0)              Operand 2 address
         DC    A(0)              Operand 2 length
                                                                SPACE 2
CUSENEXT EQU   *              Start of next table entry...
                                                                SPACE 6
REG2PATT EQU   X'AABBCCDD'    Polluted Register pattern
REG2LOW  EQU         X'DD'    (last byte above)
                                                                EJECT
***********************************************************************
*        CUSE Performace Test data...
***********************************************************************
                                                                SPACE
CUSE2TST CSECT ,
CUSEPERF DC    0A(0)                      Start of table
                                                                SPACE 2
***********************************************************************
*        performance test data
***********************************************************************
                                                                SPACE
*        Cross page bounday - operand-1 and operand-2
                                                                SPACE
PTE6     DS    0F
         DC    X'E6'                       Test Num
         DC    XL3'00'
*
         DC    AL1(1)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(032)               Op-1 SS & length
         DC    A(COP2A),A(032)               OP-2 SS & length
*                                          Target
         DC    A(11*MB+(6*K32)-96),A(512)     Op-1 & length
         DC    A(12*MB+(6*K32)-128),A(512)    Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(11*MB+(6*K32)+(512-32)-96),A(032)    OP-1
         DC    A(12*MB+(6*K32)+(512-32)-128),A(032)   OP-2
                                                                SPACE 2
PTE1     DS    0F
         DC    X'E1'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'EE'                       First-Operand SS last byte
         DC    X'EE'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(004)               Op-1 SS & length
         DC    A(COP2A),A(004)               OP-2 SS & length
*                                          Target
         DC    A(11*MB+(1*K32)-63),A(512)        Op-1 & length
         DC    A(12*MB+(1*K32)-56),A(512)        Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(11*MB+(1*K32)-63+(512-4)),A(004)   OP-1
         DC    A(12*MB+(1*K32)-56+(512-4)),A(004)   OP-2
                                                                SPACE
PTE2     DS    0F
         DC    X'E2'                       Test Num
         DC    XL3'00'
*
         DC    AL1(8)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(008)               Op-1 SS & length
         DC    A(COP2A),A(008)               OP-2 SS & length
*                                          Target
         DC    A(11*MB+(2*K32)-96),A(512)    Op-1 & length
         DC    A(12*MB+(2*K32)-128),A(512)   Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(11*MB+(2*K32)+(512-8)-96),A(008)     OP-1
         DC    A(12*MB+(2*K32)+(512-8)-128),A(008)    OP-2
                                                                SPACE 2
PTF2     DS    0F
         DC    X'F2'                       Test Num
         DC    XL3'00'
*
         DC    AL1(32)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(032)               Op-1 SS & length
         DC    A(COP2A),A(032)               OP-2 SS & length
*                                          Target
         DC    A(13*MB+(2*K32)-96),A(512)     Op-1 & length
         DC    A(14*MB+(2*K32)-128),A(512)    Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(13*MB+(2*K32)+(512-32)-96),A(032)    OP-1
         DC    A(14*MB+(2*K32)+(512-32)-128),A(032)   OP-2
                                                                SPACE 2
PTE7     DS    0F
         DC    X'E7'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1C),A(032)               Op-1 SS & length
         DC    A(COP2C),A(032)               OP-2 SS & length
*                                          Target
         DC    A(11*MB+(7*K32)-96),A(512)      Op-1 & length
         DC    A(12*MB+(7*K32)-128),A(512)    Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(11*MB+(7*K32)+(512-32)-96-3),A(032+3)    OP-1
         DC    A(12*MB+(7*K32)+(512-32)-128-3),A(032+3)   OP-2
                                                                SPACE 2
PTF4     DS    0F
         DC    X'F4'                       Test Num
         DC    XL3'00'
*
         DC    AL1(32)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(032)               Op-1 SS & length
         DC    A(COP2A),A(032)               OP-2 SS & length
*                                          Target
         DC    A(13*MB+(4*K32)-96),A(4096-128)     Op-1 & length
         DC    A(14*MB+(4*K32)-128),A(4096-128)    Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(13*MB+(4*K32)+(4096-128-32)-96),A(032)     OP-1
         DC    A(14*MB+(4*K32)+(4096-128-32)-128),A(032)    OP-2
                                                                SPACE 3
         DC    A(0)        end of table
         DC    A(0)        end of table
                                                                EJECT
***********************************************************************
*        CUSE Operand-1 scan data...
***********************************************************************
                                                                SPACE
         DS    0F
         DC    2048XL4'98765432'
COP1A    DC    256XL4'111111F0'
                                                                SPACE
         DS    0F
         DC    2048XL4'98765432'
COP1B    DC    256XL4'40404040'
                                                                SPACE
         DS    0F
         DC    2048XL4'11223344'
COP1C    DC    256XL4'40404040'
                                                                SPACE 4
***********************************************************************
*        CUSE Operand-2 scan data
***********************************************************************
                                                                SPACE
         DS    0F
         DC    2048XL4'89ABCDEF'
COP2A    DC    256XL4'111111F0'
                                                                SPACE
         DS    0F
         DC    2048XL4'89ABCDEF'
COP2B    DC    256XL4'40404040'
                                                                SPACE
         DS    0F
         DC    2048XL4'FF223344'
COP2C    DC    256XL4'40404040'
                                                                SPACE 4
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
                                                                SPACE 2
         END
