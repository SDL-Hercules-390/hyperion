 TITLE '            TRE-02-performance (Test TRE instructions)'
***********************************************************************
*
*            TRE instruction tests
*
*        NOTE: This test is based the CLCL-et-al Test
*              modified to only test the Performance
*              of the TRE instruction.
*
*        James Wekel August 2022
***********************************************************************
***********************************************************************
*
*            TRE Performance instruction tests
*
***********************************************************************
*
*  This program ONLY tests the performance of the TRE
*  instructions.
*        Tests:
*              1. TRE of 512 bytes
*              2. TRE of 512 bytes that crosses a page boundary,
*                 which results in a CC=3, and a branch back
*                 to complete the TRE instruction
*              3. TRE of 2048 bytes
*              4. TRE of 2048 bytes that crosses a page boundary,
*                 which results in a CC=3, and a branch back
*                 to complete the TRE instruction
*
***********************************************************************
*  NOTE: When assembling using SATK, use the "-t S390" option.
***********************************************************************
*
*  Example Hercules Testcase:
*
*
*     *Testcase TRE-02-performance (Test TRE instructions)
*
*     archlvl     390
*     mainsize    3
*     numcpu      1
*     sysclear
*
*     loadcore    "$(testpath)/TRE-02-performance"
*
*     #r           21fd=ff   # (uncomment to enable timing tests!)
*     runtest     20        # (depends on the host)
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
*        Initiate the TRE02TST CSECT in the CODE region
*        with the location counter at 0
***********************************************************************
                                                                SPACE
TRE02TST ASALOAD  REGION=CODE
                                                                SPACE 3
***********************************************************************
*        Create IPL (restart) PSW
***********************************************************************
                                                                SPACE
         ASAIPL   IA=BEGIN
                                                                EJECT
***********************************************************************
*               The actual "TRE02TST" program itself...
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
         ST    R2,SAVER2
                                                                SPACE
         LA    R9,2048(,R2)     Initalize SECOND base register
         LA    R9,2048(,R9)     Initalize SECOND base register
                                                                SPACE
         BAL   R14,INIT         Initalize Program
                                                                EJECT
***********************************************************************
*        Run the tests...
***********************************************************************
                                                                SPACE
         BAL   R14,TEST91       Time TRE   instruction  (speed test)
                                                                SPACE 3
***********************************************************************
*         Test for normal or unexpected test completion...
***********************************************************************
                                                                SPACE
         CLI   TIMEOPT,X'FF'    Was this a timing run?
         BNE   EOJ              No, timing run; just go end normally
                                                                SPACE
         CLI   TESTNUM,X'94'    Did we end on expected test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         CLI   SUBTEST,X'00'    Did we end on expected SUB-test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         B     EOJ              Yes, then normal completion!
                                                                SPACE 3
SAVER1   DC    F'0'
SAVER2   DC    F'0'
SAVER5   DC    F'0'
SAVETRT  DC    D'0'               (saved R1/R2 from TRT results)
                                                                SPACE
         DROP  R15
                                                                EJECT
***********************************************************************
*        TEST91                 Time TRE instruction  (speed test)
***********************************************************************
                                                                SPACE
TEST91   TM    TIMEOPT,X'FF'    Is timing tests option enabled?
         BZR   R14              No, skip timing tests
                                                                SPACE
         LA    R5,TREPERF         Point R5 --> testing control table
                                                                SPACE
         USING TRETEST,R5         What each table entry looks like
                                                                SPACE
TST91LOP EQU   *
         ST    R5,SAVER5          save current pref table base
                                                                SPACE
         IC    R6,TNUM            Set test number
         STC   R6,TESTNUM
                                                                SPACE
*
**       Initialize operand data  (move data to testing address)
*
         L     R10,OP1WHERE       Where to move operand-1 data to
         L     R11,OP1LEN           operand-1 length
         L     R6,OP1DATA         Where op1 data is right now
         L     R7,OP1LEN          How much of it there is
         MVCL  R10,R6
                                                                SPACE
         L     R12,OP2WHERE       Where to move operand-2 data to
         L     R13,=A(OP2LEN)     How much of it there is
         L     R6,OP2DATA         Where op2 data is right now
         L     R7,=A(OP2LEN)          How much of it there is
         MVCL  R12,R6
                                                                SPACE
         IC    R0,TBYTE           Set test byte
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
         LM    R10,R12,OPSWHERE
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
         LM    R10,R12,OPSWHERE
         TRE   R10,R12
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
         BALR  R6,0
                                                                SPACE
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
                                                                SPACE
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
         MVC   PRTLINE+33(5),=CL5'TRE'
         BAL   R15,RPTSPEED
*
**       More performance tests?
*
         L     R5,SAVER5          restore perf table base
         LA    R5,TRENEXT         Go on to next table entry
         CLC   =F'0',0(R5)        End of table?
         BNE   TST91LOP           No, loop...
         L     R1,SAVER1          Restore register 1
         L     R2,SAVER2          Restore first base register
         BR    R14                Return to caller or FAILTEST
                                                                EJECT
***********************************************************************
*        RPTSPEED                 Report instruction speed
***********************************************************************
                                                                SPACE
RPTSPEED ST    R15,RPTSAVE        Save return address
         BAL   R15,CALCDUR        Calculate duration
                                                                SPACE
         LA    R5,OVERHEAD        Subtract overhead
         LA    R6,DURATION        From raw timing
         LA    R7,DURATION        Yielding true instruction timing
         BAL   R15,SUBDWORD       Do it
                                                                SPACE
         LM    R12,R13,DURATION   Convert to...
         SRDL  R12,12             ... microseconds
                                                                SPACE
         CVD   R12,TICKSAAA       convert HIGH part to decimal
         CVD   R13,TICKSBBB       convert LOW  part to decimal
                                                                SPACE
         ZAP   TICKSTOT,TICKSAAA            Calculate...
         MP    TICKSTOT,=P'4294967296'      ...decimal...
         AP    TICKSTOT,TICKSBBB            ...microseconds
                                                                SPACE
         MVC   PRTLINE+43(L'EDIT),EDIT          (edit into...
         ED    PRTLINE+43(L'EDIT),TICKSTOT+3     ...print line)
                                                                EJECT
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
SUBDWORD STM   R10,R13,SUBDWSAV   Save registers
                                                                SPACE
         LM    R10,R11,0(R5)      Subtrahend  (value to subtract)
         LM    R12,R13,0(R6)      Minuend     (what to subtract FROM)
         SLR   R13,R11            Subtract LOW part
         BNM   *+4+4              (branch if no borrow)
         SL    R12,=F'1'          (otherwise do borrow)
         SLR   R12,R10            Subtract HIGH part
         STM   R12,R13,0(R7)      Store results
                                                                SPACE
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
                                                                SPACE 4
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
                                                                EJECT
***********************************************************************
*        Initialize the CPU for I/O operations
***********************************************************************
                                                                SPACE 2
IOINIT   IOINIT ,
                                                                SPACE 2
         BR    R15                Return to caller
                                                                SPACE 4
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
CONPGM   CCW1  X'09',PRTLINE,0,PRTLNG
PRTLINE  DC    C'         1,000,000 iterations of XXXXX'
         DC    C' took 999,999,999 microseconds'
PRTLNG   EQU   *-PRTLINE
EDIT     DC    X'402020206B2020206B202120'
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
ENDREGS  DC    A(0),XL4'00'   Ending R1/R2 register values
                                                                SPACE 2
TRENEXT  EQU   *              Start of next table entry...
                                                                SPACE 6
REG2PATT EQU   X'AABBCCDD'    Register 2 starting/ending CC0 value
REG2LOW  EQU         X'DD'    (last byte above)
                                                                EJECT
***********************************************************************
*        TRE Performace Test data...
***********************************************************************
                                                                SPACE
TRE02TST CSECT ,
TREPERF  DC    0A(0)      start of table
                                                                SPACE 2
TREPOP1  DC    X'91',X'99',X'00',X'00'
         DC    A(TRELOP10),A(TRELOP20)
         DC    A(00+(02*K64)),A(512),A(MB+(02*K64))      no crosses
         DC    A(7) CC0
         DC    A(00+(02*K64)+512),A(REG2PATT)
                                                                SPACE 4
TREPOP2  DC    X'92',X'99',X'00',X'00'
         DC    A(TRELOP10),A(TRELOP20)
         DC    A(00+(03*K64)-12),A(512),A(MB+(03*K64))   op1 crosses
         DC    A(7) CC0
         DC    A(00+(03*K64)-12+512),A(REG2PATT)
                                                                SPACE 4
TREPOP3  DC    X'93',X'99',X'00',X'00'
         DC    A(TRELOP10),A(TRELOP20)
         DC    A(00+(04*K64)),A(2048),A(MB+(04*K64))     no crosses
         DC    A(7) CC0
         DC    A(00+(041*K64)+2048),A(REG2PATT)
                                                                SPACE 4
TREPOP4  DC    X'94',X'99',X'00',X'00'
         DC    A(TRELOP10),A(TRELOP20)
         DC    A(00+(04*K64)-12),A(2048),A(MB+(04*K64))  op1 crosses
         DC    A(7) CC0
         DC    A(00+(041*K64)-12+2048),A(REG2PATT)
                                                                SPACE 7
         DC    A(0)        end of table
         DC    A(0)        end of table
                                                                EJECT
***********************************************************************
*        TRE op1 scan data...
***********************************************************************
                                                                SPACE
TRTOP10  DC    64XL4'78125634'                              (CC0)
                                                                SPACE
TRTOP111 DC    04XL4'78125634',X'00110000',59XL4'78125634'  (CC1)
                                                                SPACE
TRTOP1F0 DC    63XL4'78125634',X'000000F0'                  (CC1)
                                                                SPACE
TRELOP10 DC    512XL4'78125634'                             (CC0)
                                                                SPACE 5
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
                                                                EJECT
***********************************************************************
*        Fixed storage locations
***********************************************************************
                                                                SPACE 4
         ORG   TRE02TST+TIMEADDR      (s/b @ X'21FD')
                                                                SPACE 3
TIMEOPT  DC    X'00'      Set to non-zero to run timing tests
                                                                SPACE 5
         ORG   TRE02TST+TESTADDR      (s/b @ X'21FE', X'21FF')
                                                                SPACE 3
TESTNUM  DC    X'00'      Test number of active test
SUBTEST  DC    X'00'      Active test sub-test number
                                                                SPACE 5
         ORG   TRE02TST+SEGTABLS      (s/b @ X'3000')
                                                                SPACE 3
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
                                                                SPACE 3
         END
