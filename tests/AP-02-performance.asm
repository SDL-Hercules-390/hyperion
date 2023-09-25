 TITLE '            AP-01-basic (Test AP instruction)'
***********************************************************************
*
*            AP instruction tests
*
***********************************************************************
***********************************************************************
*
*  This program ONLY tests performance of the AP instruction.
*
***********************************************************************
*  NOTE: When assembling using SATK, use the "-t S390" option.
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
*        Initiate the AP02 CSECT in the CODE region
*        with the location counter at 0
***********************************************************************
                                                                SPACE
AP02     ASALOAD  REGION=CODE
                                                                SPACE 3
***********************************************************************
*        Create IPL (restart) PSW
***********************************************************************
                                                                SPACE
         ASAIPL   IA=BEGIN
                                                                EJECT
***********************************************************************
*               The actual "AP02" program itself...
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
         USING  IOCB,R3
         USING  AP02,R0
                                                                SPACE
BEGIN    BALR  R2,0             Initalize FIRST base register
         BCTR  R2,0             Initalize FIRST base register
         BCTR  R2,0             Initalize FIRST base register
                                                                SPACE
         LA    R9,2048(,R2)     Initalize SECOND base register
         LA    R9,2048(,R9)     Initalize SECOND base register
                                                                SPACE
         BAL   R14,INIT
                                                                EJECT
***********************************************************************
*        Run the test...
***********************************************************************
                                                                SPACE
         CLI    ACTIVE,X'FF'
         BNE    EOJ
         BAL    R14,TESTAP02
         B      EOJ              Yes, then normal completion!
                                                                EJECT
***********************************************************************
*        MACRO
***********************************************************************
         MACRO
         OVERONLY &NUM
         LCLA     &CTR
&CTR     SETA     &NUM
.LOOP    ANOP
.*
         NOP
.*
&CTR     SETA     &CTR-1
         AIF      (&CTR GT 0).LOOP
         MEND

         MACRO
         DOINSTR  &NUM
         LCLA     &CTR
&CTR     SETA     &NUM
.LOOP    ANOP
.*
         NOP
         AP       OPL1,OPR1
.*
&CTR     SETA     &CTR-1
         AIF      (&CTR GT 0).LOOP
         MEND
         
***********************************************************************
*        TESTAP02
***********************************************************************
                                                                SPACE
TESTAP02 NOP
         L        R7, NULLOOPS
         STCK     BEGCLOCK

         BALR     R6,0
         OVERONLY 2
         PRINT    OFF
         OVERONLY 96
         PRINT    ON
         OVERONLY 2
                                                                SPACE
         BCTR     R7,R6
         STCK     ENDCLOCK
         BAL      R15,CALCDUR
         MVC      OVERHEAD,DURATION
                                                                SPACE
         L        R7, NULLOOPS
         STCK     BEGCLOCK
                                                                SPACE
         BALR     R6,0
         DOINSTR 2
         PRINT    OFF
         DOINSTR  96
         PRINT    ON
         DOINSTR  2
                                                                SPACE
         BCTR     R7,R6
         STCK     ENDCLOCK
                                                                SPACE
         DROP     R5        RPTSPEED uses R5 as a work register
                                                                SPACE
         MVC      PRTLINE+33(5),=CL6' AP  '
         BAL      R15,RPTSPEED

TESTEND  BR     R14
                                                                EJECT
***********************************************************************
*        CALCDUR               Calc duration
***********************************************************************
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
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 4
EOJ      DWAITEND LOAD=YES             Normal completion
                                                                SPACE 4
FAILDEV  DWAIT LOAD=YES,CODE=01        ENADEV failed
                                                                SPACE 4
FAILIO   DWAIT LOAD=YES,CODE=02        RAWIO failed
                                                                SPACE 4
FAILTEST DWAIT LOAD=YES,CODE=BAD       Abnormal termination
                                                                EJECT
IOCB_009 IOCB  X'009',CCW=CONPGM
***********************************************************************
*
***********************************************************************
                                                                SPACE 2
         LTORG ,
SAVER1   DC    F'0'
SAVEPGN  DC    F'0'
ACTIVE   DC    F'00'
OPL1     DC    X'1000000000000000000000000000000C'
OPR1     DC    X'0000000000000000000000000000001C'
NULLOOPS DC    F'10000'
BEGCLOCK DC    0D'0',8X'BB'
ENDCLOCK DC    0D'0',8X'EE'
DURATION DC    0D'0',8X'DD'
OVERHEAD DC    0D'0',8X'FF'
TICKSAAA DC    PL8'0'
TICKSBBB DC    PL8'0'
TICKSTOT DC    PL8'0'

CONPGM   CCW1  X'09',PRTLINE,0,PRTLNG
PRTLINE  DC    C'         1,000,000 iterations of XXXXX'
         DC    C' took 999,999,999 microseconds'
PRTLNG   EQU   *-PRTLINE
EDIT     DC    X'402020206B2020206B202120'
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
