 TITLE '                              Tape Data Chaining Test'
***********************************************************************
*
*                     Tape Data Chaining Test
*
***********************************************************************
*
*   This program verifies proper Hercules tape device handler
*   and/or channel subsystem handling of data-chained CCWs.
*
*   A bug was reported wherein multiple data-chained CCWs were used
*   to read a potentially very large 256K tape block (8 data-chained
*   CCWs, each specifying a 32K buffer), but the "Address of the last
*   CCW processed" and "Residual" SCSW fields of the IRB were wrong,
*   causing the program to calculate an incorrect block size.
*
***********************************************************************
*
*   Example Hercules Testcase:
*
*
*       *Testcase Tape Data Chaining
*
*       # Prepare test environment
*       mainsize  1
*       numcpu    1
*       sysclear
*       archlvl   z/Arch
*       detach    580
*       attach    580 3490 "$(testpath)/tape.aws"
*       loadcore  "$(testpath)/tape.core"
*
*       ## t+                          # (trace instructions)
*       t+580                       # (trace device CCWs)
*
*       # Run the test...
*       runtest   0.25              # (plenty of time)
*
*       # Clean up afterwards
*       detach    580               # (no longer needed)
*
*       *Compare
*       r 800.8
*       *Want "SCSW fields" 00001008 0C403000
*
*       *Done
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
         ARCHLVL  MNOTE=NO
                                                                EJECT
***********************************************************************
*        Initiate the TESTTAPE CSECT in the CODE region
*        with the location counter at 0
***********************************************************************
                                                                SPACE
TESTTAPE ASALOAD  REGION=CODE
                                                                SPACE 3
***********************************************************************
*        Define the z/Arch RESTART PSW
***********************************************************************
                                                                SPACE
PREVORG  EQU   *
         ORG   TESTTAPE+X'1A0'
         DC    XL16'00000001800000000000000000000200'
         ORG   TESTTAPE+X'1A0'
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
*               The actual TESTTAPE program itself...
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
         BAL   R14,TEST01       Data-Chained CCWs > blocksize,
*                               with/without ORB ILS flag
*
         B     EOJ              Normal completion
                                                                EJECT
***********************************************************************
*        TEST01     Data-Chained CCWs test with/without ORB ILS flag
***********************************************************************
                                                                SPACE
TEST01   MVI   TESTNUM,X'01'          Initialize test number
                                                                SPACE
         MVI   ORB1_8,0               Initialize ORB flags
         MVI   ORRB1_24,0             Initialize ORB flags
         OI    ORB1_8,ORBF            Format-1 CCWs
         OI    ORRB1_24,ORBL          SLI mode for Immediate CCWs
                                                                SPACE
         LA    R0,REWPROG             Rewind tape to load point
         BAL   R15,EXCP               Do the I/O
                                                                SPACE
         CLI   SCSWUS,SCSWCE+SCSWDE   Expected Unit Status?
         BNE   FAILREW                No?! FAIL the test!
         CLI   SCSWCS,0               Expected Channel Status?
         BNE   FAILREW                No?! FAIL the test!
                                                                SPACE
***********************************************************************
*     Tape block size is 20,480 bytes, so I/O should end on
*     the very first 32K CCW (but should point to the second
*     one) with a residual value of 12,288 (X'3000') bytes.
***********************************************************************
                                                                SPACE
         LA    R0,READPROG            Read block using data chaining
         BAL   R15,EXCP               Do the I/O
         MVC   TESTCCWA,SCSWCCW       Save Ending CCW Address
         MVC   TESTUS,SCSWUS          Save Unit Status
         MVC   TESTCS,SCSWCS          Save Channel Status
         MVC   TESTRES,SCSWCNT        Save Residual
                                                                SPACE
         CLC   TESTRSLT,GOODRSLT      Is results what we expected?
         BNE   FAILTEST               No, FAIL the test
                                                                SPACE
***********************************************************************
*     Now do the same thing again, but WITHOUT the ORBL flag
*     to verify we still get a normal incorrect length result.
***********************************************************************
                                                                SPACE
         NI    ORRB1_24,255-ORBL      Turn off SLI mode ORB flag
         LA    R0,READPROG            Read block using data chaining
         BAL   R15,EXCP               Do the I/O
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
         LA     R3,IOCB_580     Point to IOCB
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
FAILREW  DWAIT LOAD=YES,CODE=03     REWIND failed
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
IOCB_580 IOCB  X'580'
                                                                EJECT
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE
         LTORG ,          Literals pool
                                                                SPACE
MODE     DC    X'10'      Mode Set argument
                                                                SPACE
K        EQU   1024       One kilobyte  (OK! OK! "Kibibyte!" Sheesh!)
                                                                SPACE
RESLTADR EQU   (2*K)      Address where test results will be placed
TESTADDR EQU   (4*K)-1    Address where test number will be placed
CDCCWADR EQU   (4*K)      Address of data-chained CCWs
IDALADDR EQU   (8*K)      Address of Indirect Data Address Lists
                                                                SPACE
BUFSADDR EQU   (32*K)     Address where first I/O buffer will start
IOBUFLEN EQU   (32*K)     Length of one I/O buffer  (32768 bytes)
BLOCKLEN EQU   (20*K)     Size of tape block        (20480 bytes)
                                                                SPACE
RESIDUAL EQU   (IOBUFLEN-BLOCKLEN)  Expected residual value
                                                                SPACE 2
***********************************************************************
*        CCW opcode equates, etc.
***********************************************************************
                                                                SPACE
CD       EQU   X'80'    Chain Data
CC       EQU   X'40'    Chain Command
SLI      EQU   X'20'    Suppress Incorrect Length Indication
SKIP     EQU   X'10'    Skip Data Transfer
IDA      EQU   X'04'    Indirect Data Address
                                                                SPACE
READ     EQU   X'02'    Read or Read IPL
READFWD  EQU   X'06'    Read Forward (3590 only)
REWIND   EQU   X'07'    Rewind to load point
TIC      EQU   X'08'    Transfer In Channel (branch to another CCW)
MODESET  EQU   X'DB'    Mode Set
                                                                SPACE 2
***********************************************************************
*        Channel Programs
***********************************************************************
                                                                SPACE
REWPROG  CCW1  MODESET,MODE,CC+SLI,1
         CCW1  TIC,REW2LDPT,0,0
                                                                SPACE
READPROG CCW1  MODESET,MODE,CC+SLI,1
         CCW1  TIC,READ256K,0,0
                                                                SPACE
REW2LDPT CCW1  REWIND,0,SLI,1
                                                                EJECT
***********************************************************************
*        Fixed storage locations
***********************************************************************
                                                                SPACE
         ORG   TESTTAPE+RESLTADR      (s/b @ X'0800')
                                                                SPACE
TESTRSLT DS   0XL8                    Saved Test Results...
TESTCCWA DC    A(0)                   Ending CCW Address
TESTUS   DC    X'00'                  Unit Status
TESTCS   DC    X'00'                  Channel Status
TESTRES  DC    H'0'                   Residual
GOODRSLT DC    A(READ256K+8)
         DC    AL1(SCSWCE+SCSWDE),AL1(SCSWIL),AL2(IOBUFLEN-BLOCKLEN)
                                                                SPACE 4
         ORG   TESTTAPE+TESTADDR      (s/b @ X'0FFF')
                                                                SPACE
TESTNUM  DC    X'00'      Test number of active test
                                                                SPACE 4
         ORG   TESTTAPE+CDCCWADR      (s/b @ X'1000')
                                                                SPACE
READ256K CCW1  READ,IDAL1,CD+IDA,IOBUFLEN
         CCW1  READ,IDAL2,CD+IDA,IOBUFLEN
         CCW1  READ,IDAL3,CD+IDA,IOBUFLEN
         CCW1  READ,IDAL4,CD+IDA,IOBUFLEN
         CCW1  READ,IDAL5,CD+IDA,IOBUFLEN
         CCW1  READ,IDAL6,CD+IDA,IOBUFLEN
         CCW1  READ,IDAL7,CD+IDA,IOBUFLEN
         CCW1  READ,IDAL8,IDA,IOBUFLEN
                                                                SPACE 4
***********************************************************************
*        I/O Buffers referenced by IDALs
***********************************************************************
                                                                SPACE
IOBUFFS  EQU   BUFSADDR       Where first I/O buffer will begin
*
IOBUFF1  EQU   IOBUFFS+(0*IOBUFLEN)
IOBUFF2  EQU   IOBUFFS+(1*IOBUFLEN)
IOBUFF3  EQU   IOBUFFS+(2*IOBUFLEN)
IOBUFF4  EQU   IOBUFFS+(3*IOBUFLEN)
IOBUFF5  EQU   IOBUFFS+(4*IOBUFLEN)
IOBUFF6  EQU   IOBUFFS+(5*IOBUFLEN)
IOBUFF7  EQU   IOBUFFS+(6*IOBUFLEN)
IOBUFF8  EQU   IOBUFFS+(7*IOBUFLEN)
                                                                EJECT
***********************************************************************
*        Indirect Data Address Lists 1 - 4
***********************************************************************
                                                                SPACE
         ORG   TESTTAPE+IDALADDR      (s/b @ X'2000')
                                                                SPACE
IDAL1    DC    A(IOBUFF1+(0*(4*K)))
         DC    A(IOBUFF1+(1*(4*K)))
         DC    A(IOBUFF1+(2*(4*K)))
         DC    A(IOBUFF1+(3*(4*K)))
         DC    A(IOBUFF1+(4*(4*K)))
         DC    A(IOBUFF1+(5*(4*K)))
         DC    A(IOBUFF1+(6*(4*K)))
         DC    A(IOBUFF1+(7*(4*K)))
                                                                SPACE 3
IDAL2    DC    A(IOBUFF2+(0*(4*K)))
         DC    A(IOBUFF2+(1*(4*K)))
         DC    A(IOBUFF2+(2*(4*K)))
         DC    A(IOBUFF2+(3*(4*K)))
         DC    A(IOBUFF2+(4*(4*K)))
         DC    A(IOBUFF2+(5*(4*K)))
         DC    A(IOBUFF2+(6*(4*K)))
         DC    A(IOBUFF2+(7*(4*K)))
                                                                SPACE 3
IDAL3    DC    A(IOBUFF3+(0*(4*K)))
         DC    A(IOBUFF3+(1*(4*K)))
         DC    A(IOBUFF3+(2*(4*K)))
         DC    A(IOBUFF3+(3*(4*K)))
         DC    A(IOBUFF3+(4*(4*K)))
         DC    A(IOBUFF3+(5*(4*K)))
         DC    A(IOBUFF3+(6*(4*K)))
         DC    A(IOBUFF3+(7*(4*K)))
                                                                SPACE 3
IDAL4    DC    A(IOBUFF4+(0*(4*K)))
         DC    A(IOBUFF4+(1*(4*K)))
         DC    A(IOBUFF4+(2*(4*K)))
         DC    A(IOBUFF4+(3*(4*K)))
         DC    A(IOBUFF4+(4*(4*K)))
         DC    A(IOBUFF4+(5*(4*K)))
         DC    A(IOBUFF4+(6*(4*K)))
         DC    A(IOBUFF4+(7*(4*K)))
                                                                EJECT
***********************************************************************
*        Indirect Data Address Lists 5 - 8
***********************************************************************
                                                                SPACE 3
IDAL5    DC    A(IOBUFF5+(0*(4*K)))
         DC    A(IOBUFF5+(1*(4*K)))
         DC    A(IOBUFF5+(2*(4*K)))
         DC    A(IOBUFF5+(3*(4*K)))
         DC    A(IOBUFF5+(4*(4*K)))
         DC    A(IOBUFF5+(5*(4*K)))
         DC    A(IOBUFF5+(6*(4*K)))
         DC    A(IOBUFF5+(7*(4*K)))
                                                                SPACE 3
IDAL6    DC    A(IOBUFF6+(0*(4*K)))
         DC    A(IOBUFF6+(1*(4*K)))
         DC    A(IOBUFF6+(2*(4*K)))
         DC    A(IOBUFF6+(3*(4*K)))
         DC    A(IOBUFF6+(4*(4*K)))
         DC    A(IOBUFF6+(5*(4*K)))
         DC    A(IOBUFF6+(6*(4*K)))
         DC    A(IOBUFF6+(7*(4*K)))
                                                                SPACE 3
IDAL7    DC    A(IOBUFF7+(0*(4*K)))
         DC    A(IOBUFF7+(1*(4*K)))
         DC    A(IOBUFF7+(2*(4*K)))
         DC    A(IOBUFF7+(3*(4*K)))
         DC    A(IOBUFF7+(4*(4*K)))
         DC    A(IOBUFF7+(5*(4*K)))
         DC    A(IOBUFF7+(6*(4*K)))
         DC    A(IOBUFF7+(7*(4*K)))
                                                                SPACE 3
IDAL8    DC    A(IOBUFF8+(0*(4*K)))
         DC    A(IOBUFF8+(1*(4*K)))
         DC    A(IOBUFF8+(2*(4*K)))
         DC    A(IOBUFF8+(3*(4*K)))
         DC    A(IOBUFF8+(4*(4*K)))
         DC    A(IOBUFF8+(5*(4*K)))
         DC    A(IOBUFF8+(6*(4*K)))
         DC    A(IOBUFF8+(7*(4*K)))
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
