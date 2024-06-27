 TITLE '                        CCW Incorrect Length Suppression Test'
***********************************************************************
*
*                CCW Incorrect Length Suppression Test
*
***********************************************************************
*
*  This program verifies proper Hercules channel subsystem handling
*  of immediate CCWs (e.g. 0x03 NOP CCW) with a non-zero length field
*  and WITHOUT the SLI flag set, for both Format-0 and Format-1, and
*  both with and without the ORB 'L' Incorrect Length Suppression Mode
*  flag.
*
***********************************************************************
*
*   Example Hercules Testcase:
*
*       *Testcase CCW-ILS (CCW Incorrect Length Suppression)
*
*       # Prepare test environment
*
*       mainsize    1
*       numcpu      1
*       sysclear
*       archlvl     z/Arch
*       detach      390
*       attach      390  3390  "$(testpath)/CCWILS.3390-1.comp-z"
*       loadcore               "$(testpath)/CCW-ILS.core"
*
*       t+390                   # (trace device CCWs)
*       o+390                   # (trace device ORBs)
*
*       # Run the test...
*       runtest   0.25          # (plenty of time)
*
*
*       # Clean up afterwards
*       detach    390           # (no longer needed)
*
*       *Compare
*       r FFF.1
*       *Want "Ending test number" 03
*
*       *Done
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
         ARCHLVL  MNOTE=NO
                                                                EJECT
***********************************************************************
*        Initiate the CCWILS CSECT in the CODE region
*        with the location counter at 0
***********************************************************************
                                                                SPACE
CCWILS   ASALOAD  REGION=CODE
                                                                SPACE 3
***********************************************************************
*        Define the z/Arch RESTART PSW
***********************************************************************
                                                                SPACE
PREVORG  EQU   *
         ORG   CCWILS+X'1A0'
*        PSWZ  <sys>,<key>,<mwp>,<prog>,<addr>[,amode]
         PSWZ  0,0,0,0,X'200',64
         ORG   PREVORG
                                                                SPACE 3
***********************************************************************
*        Create IPL (restart) PSW
***********************************************************************
                                                                SPACE
         ASAIPL   IA=BEGIN
                                                                EJECT
***********************************************************************
*               The actual CCWILS program itself...
***********************************************************************
*
*  Architecture Mode: z/Arch
*  Addressing Mode:   64-bit
*  Register Usage:
*
*   R0       (work)
*   R1       I/O device used by ENADEV and RAWIO macros
*   R2       Program base register
*   R3       IOCB pointer for ENADEV and RAWIO macros
*   R4       IO work register used by ENADEV and RAWIO
*   R5       Used for CPU register when signaling architecture change
*   R6,R7    Signaling registers when changing architecture
*   R8       ORB pointer
*   R9       SCSW pointer
*   R10-R15  (work)
*
***********************************************************************
                                                                SPACE 2
         USING  ASA,R0          Low core addressability
         USING  BEGIN,R2        Program Addressability
         USING  IOCB,R3         SATK Device I/O Control Block
         USING  ORB,R8          ESA/390 Operation Request Block
         USING  SCSW,R9         ESA/390 Subchannel Status Word
                                                                SPACE 2
BEGIN    BALR  R2,0             Initalize Base Register
         BCTR  R2,0             Initalize Base Register
         BCTR  R2,0             Initalize Base Register
                                                                SPACE
         BAL   R14,INIT         Initalize Program
*
**       Run the tests...
*
         BAL   R14,TEST01       Format-0
         BAL   R14,TEST02       Format-1, without ORB ILS flag
         BAL   R14,TEST03       Format-1, with    ORB ILS flag
*
         B     EOJ              Normal completion
                                                                EJECT
***********************************************************************
*        TEST01:          Format-0
***********************************************************************
                                                                SPACE
TEST01   MVI   TESTNUM,X'01'          Initialize test number
                                                                SPACE
         MVI   ORB1_8,0               Initialize ORB flags
         MVI   ORRB1_24,0             Initialize ORB flags
*
         NI    ORB1_8,X'FF'-ORBF      Format-0 CCWs
         NI    ORRB1_24,X'FF'-ORBL    (ILS mode irrelevant)
                                                                SPACE
         LA    R0,NOPPROG             No-Operation channel program
         BAL   R15,EXCP               Do the I/O
                                                                SPACE
         MVC   TESTCCWA,SCSWCCW       Save Ending CCW Address
         MVC   TESTUS,SCSWUS          Save Unit Status
         MVC   TESTCS,SCSWCS          Save Channel Status
         MVC   TESTRES,SCSWCNT        Save Residual
                                                                SPACE
         CLC   TESTRSLT,GOODRSLT      Is results what we expected?
         BNE   FAILTEST               No, FAIL the test
         BR    R14                    Yes, test SUCCESS
                                                                EJECT
***********************************************************************
*        TEST02:          Format-1, without ORB ILS flag
***********************************************************************
                                                                SPACE
TEST02   MVI   TESTNUM,X'02'          Initialize test number
                                                                SPACE
         MVI   ORB1_8,0               Initialize ORB flags
         MVI   ORRB1_24,0             Initialize ORB flags
*
         OI    ORB1_8,ORBF            Format-1 CCWs
         NI    ORRB1_24,X'FF'-ORBL    ILS mode off
                                                                SPACE
         LA    R0,NOPPROG             No-Operation channel program
         BAL   R15,EXCP               Do the I/O
                                                                SPACE
         MVC   TESTCCWA,SCSWCCW       Save Ending CCW Address
         MVC   TESTUS,SCSWUS          Save Unit Status
         MVC   TESTCS,SCSWCS          Save Channel Status
         MVC   TESTRES,SCSWCNT        Save Residual
                                                                SPACE
         CLC   TESTRSLT,BADRSLT       Is results what we expected?
         BNE   FAILTEST               No, FAIL the test
         BR    R14                    Yes, test SUCCESS
                                                                EJECT
***********************************************************************
*        TEST03:          Format-1, with ORB ILS flag
***********************************************************************
                                                                SPACE
TEST03   MVI   TESTNUM,X'03'          Initialize test number
                                                                SPACE
         MVI   ORB1_8,0               Initialize ORB flags
         MVI   ORRB1_24,0             Initialize ORB flags
*
         OI    ORB1_8,ORBF            Format-1 CCWs
         OI    ORRB1_24,ORBL          ILS mode on
                                                                SPACE
         LA    R0,NOPPROG             No-Operation channel program
         BAL   R15,EXCP               Do the I/O
                                                                SPACE
         MVC   TESTCCWA,SCSWCCW       Save Ending CCW Address
         MVC   TESTUS,SCSWUS          Save Unit Status
         MVC   TESTCS,SCSWCS          Save Channel Status
         MVC   TESTRES,SCSWCNT        Save Residual
                                                                SPACE
         CLC   TESTRSLT,GOODRSLT      Is results what we expected?
         BNE   FAILTEST               No, FAIL the test
         BR    R14                    Yes, test SUCCESS
                                                                EJECT
***********************************************************************
*        Program Initialization
***********************************************************************
                                                                SPACE
INIT     DS    0H               Program Initialization
                                                                SPACE
         LA     R3,IOCB_390     Point to IOCB
         $L     R8,IOCBORB      Point to ORB
         $L     R15,IOCBIRB     Point to IRB
         USING  IRB,R15         Temporary addressability
         LA     R9,IRBSCSW      Point to SCSW
         DROP   R15             Done with IRB
                                                                SPACE
         BAL    R15,IOINIT      Initialize the CPU for I/O operations
         BAL    R15,ENADEV      Enable our device making ready for use
                                                                SPACE
         BR     R14             Return to caller
                                                                SPACE 3
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE
EOJ      DWAITEND LOAD=YES          Normal completion
                                                                SPACE
FAILDEV  DWAIT LOAD=YES,CODE=01     ENADEV failed
                                                                SPACE
FAILIO   DWAIT LOAD=YES,CODE=02     RAWIO failed
                                                                SPACE
FAILTEST DWAIT LOAD=YES,CODE=BAD    Abnormal termination
                                                                EJECT
***********************************************************************
*        Initialize the CPU for I/O operations
***********************************************************************
                                                                SPACE
IOINIT   IOINIT ,
         BR    R15              Return to caller
                                                                SPACE 3
***********************************************************************
*        Enable the device, making it ready for use
***********************************************************************
                                                                SPACE
ENADEV   ENADEV  ENAOKAY,FAILDEV,REG=4
*
ENAOKAY  BR    R15            Return to caller if device enabled OK
                                                                EJECT
***********************************************************************
*        Execute the channel program pointed to by R0
***********************************************************************
                                                                SPACE
EXCP     ST    R0,ORBCCW      Plug Channel Program address into IORB
                                                                SPACE
         RAWIO 4,FAIL=FAILIO
                                                                SPACE
         BR    R15            Return to caller
                                                                EJECT
***********************************************************************
*         Structure used by RAWIO identifying
*         the device and operation being performed
***********************************************************************
                                                                SPACE
IOCB_390 IOCB  X'390'
                                                                EJECT
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE
         LTORG ,          Literals pool
                                                                SPACE
K        EQU   1024       One kilobyte  (OK! OK! "Kibibyte!" Sheesh!)
                                                                SPACE
HEX500   EQU   X'500'     NOP CCW buffer address and buffer length
RESLTADR EQU   (2*K)      Address where test results will be placed
TESTADDR EQU   (4*K)-1    Address where test number will be placed
                                                                SPACE 2
***********************************************************************
*        Format-0/1 Neutral NOP CCW Channel Program
***********************************************************************
                                                                SPACE
         ORG   CCWILS+HEX500          (s/b @ X'0500')
                                                                SPACE
NOOP     EQU   X'03'                  No Operation CCW opcode
                                                                SPACE
NOPPROG  DC    AL1(NOOP),AL1(0),AL2(HEX500),AL1(0),AL1(0),AL2(HEX500)
                                                                EJECT
***********************************************************************
*        Fixed storage locations
***********************************************************************
                                                                SPACE
         ORG   CCWILS+RESLTADR        (s/b @ X'0800')
                                                                SPACE
TESTRSLT DS   0XL8                    Saved Test Results...
TESTCCWA DC    A(0)                   Ending CCW Address
TESTUS   DC    X'00'                  Unit Status
TESTCS   DC    X'00'                  Channel Status
TESTRES  DC    H'0'                   Residual
                                                                SPACE
GOODRSLT DC    XL4'00000508'
         DC    AL1(SCSWCE+SCSWDE),AL1(0),AL2(1280)
                                                                SPACE
BADRSLT  DC    XL4'00000508'
         DC    AL1(SCSWCE+SCSWDE),AL1(SCSWIL),AL2(1280)
                                                                SPACE 4
         ORG   CCWILS+TESTADDR        (s/b @ X'0FFF')
                                                                SPACE
TESTNUM  DC    X'00'      Test number of active test
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
                                                                SPACE 3
         END
