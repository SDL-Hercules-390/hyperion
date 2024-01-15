 TITLE '                          Various CKD Dasd CCW tests...'
***********************************************************************
*
*                   Various CKD Dasd CCW tests...
*
*                   (mostly GitHub Issue #572)
*
*   This test program simply executes a few selected E7 Prefix CCW
*   channel programs to verify Hercules's E7 Prefix CCW support is
*   working properly. The current list of tests that this program
*   performs is as follows:
*
*     01  Format 2 PFX to obtain subsystem information (no IDA)
*     02  Format 0 PFX with Define Extent Valid bit off (DX CCW
*         chained) (Read 06 IDA)
*     03  Format 0 PFX with Define Extent Valid bit on (DX CCW
*         imbeded) (Read 06 1 IDA)
*     04  Format 2 PFX to obtain control unit information (PFX
*         E7 2 IDA, Read 06 1 IDA)
*     05  Read 06 CCW should fail since LR operation is Read(16)
*         and Read 06 CCW not multi-track (Read 06 1 IDA)
*     06  Same as Test #5, but properly uses multi-track Read
*         (86) (Read 86 1 IDA)
*     07  Peter's z/VM SSI issue (PFX 01 CMDREJ)
*     08  (NEW) Write Data erase remainder of track.
*     09  (NEW) Read record 3 on track 0 (verify test #08 erase)
*         (https://github.com/SDL-Hercules-390/hyperion/issues/601)
*
*
*   By default, all tests in the TESTTAB table are run one after
*   the other. To run just one specific test, in your .tst script,
*   set the TESTONLY byte at X'FFF' to the specific test number.
*
*   All channel programs (except for two of them) are expected to
*   complete normally without error (SCSW = CE+DE = X'0C00').
*
*   Tests #5 and #9 however are purposely designed to always fail
*   in order to verify Hercules properly rejects the invalid channel
*   program and does not mistakenly accept and process it instead.
*   Test #6 is the corrected form of test #5 which, just like all
*   of the other tests (except #9), should always succeed.
*
*   Except for Tests #1 and #7, most of the other tests (#2-#6)
*   also specify IDA (Indirect Data Addressing) in some of their
*   CCWs in order to verify proper Hercules handling of that too.
*
*   Tests #4, #8 and #9 are especially important in that #4 specifies
*   IDA in its E7 Prefix CCW so as to cause its data to be accessed
*   in TWO chunks (i.e. its IDAL contains TWO entries in it), and
*   test #8 and #9 together verify proper track erasure, whereas all
*   of the other IDA usage is only used in the Read 06 and Read 86
*   CCWs where the IDAL only has one entry in it to simply redirect
*   the read to elsewhere.
*
*   Thank you to Aaron Finerman for devising most of these tests.
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
         ARCHLVL ZARCH=YES,ARCHIND=YES,MNOTE=NO
                                                                SPACE 3
***********************************************************************
*        Initiate the E7TEST CSECT in the CODE region
*        with the location counter at 0
***********************************************************************
                                                                SPACE
E7TEST   ASALOAD  REGION=CODE
                                                                EJECT
***********************************************************************
*                 L O W   C O R E
***********************************************************************
                                                                SPACE
         ORG   E7TEST+X'1A0'                    z/Arch Restart New PSW
         DC    0D'0',XL8'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE
         ORG   E7TEST+X'1D0'                    z/Arch Program New PSW
         DC    0D'0',XL8'0002000180000000'
         DC    AD(X'DEAD')
                                                                SPACE 4
***********************************************************************
*                    ENTRY POINT CODE
***********************************************************************
*   R0       (work)
*   R1       (work) (also ENADEV macro's I/O device during startup)
*   R2       (work)
*   R3       IOCB pointer (set by INIT, needed by ENADEV macro)
*   R4       SCHIB pointer (tempoarily used at INIT during ENADEV)
*   R5       SCHSCSW pointer (also temporarily used for CPU register
*            when signaling architecture change during startup)
*   R6,R7    (work) (also used as signaling registers when changing
*            architecture during startup)
*   R8       ORB pointer (set by INIT, used by EXCP subroutine)
*   R9-R15   (work)
***********************************************************************
                                                                SPACE
         USING E7TEST,R0          Low core addressability
         USING ASA,R0             Low core addressability
         USING IOCB,R3            SATK Device I/O-Control Block
         USING SCHIB,R4           ESA/390 Subchannel Information Block
         USING SCSW,R5            ESA/390 Subchannel Status Word
         USING ORB,R8             ESA/390 Operation-Request Block
                                                                SPACE
         ORG   E7TEST+X'200'
BEGIN    EQU   *
         SLR   R0,R0              Start clean (SIGP status register)
         MVI   TESTNUM,0          Initialize Test number
         SLR   R1,R1              Start clean (SIGP parm   register)
         SLR   R2,R2              Start clean
         SLR   R3,R3              Start clean (SIGP target CPU)
                                                                SPACE
         LA    R3,0               Target CPU = CPU #0
         LA    R1,1               Parm register = z/Arch mode
         SIGP  R0,R3,X'12'        Order code    = z/Arch mode
         BC    B'1000',ZARCHOK    CC0 = success: continue
         BC    B'0100',CHKZARCH   CC1 = status stored: check further
         BC    B'0010',FAILCPU0   CC2 = busy: FAIL
         BC    B'0001',FAILCPU0   CC3 = not operational: FAIL
                                                                EJECT
***********************************************************************
*       Ensure test program executes in z/Architecture mode
***********************************************************************
                                                                SPACE
CHKZARCH LA    R4,X'100'          Status X'100' = Same Architecture!
         CLR   R0,R4              Are we already in z/Arch mode?
         JNE   FAILCPU0           Any other status = FAIL
                                                                SPACE
ZARCHOK  LA    R4,BEGIN0          Point to CPU #0 entry point
         STH   R4,X'1AE'          Update Restart PSW
                                                                SPACE
         LA    R3,0               Target CPU = CPU #0
         SIGP  R0,R3,X'6'         Order code = Restart
                                                                SPACE 2
         LPSWE FAILCPU0           WTF?! How did we get here?!
                                                                SPACE 4
***********************************************************************
*    THE ACTUAL (very short and simple) E7TEST TEST PROGRAM ITSELF
***********************************************************************
                                                                SPACE
BEGIN0   BAL   R14,INIT           Initalize Program
                                                                SPACE 3
         LM    R10,R11,ATESTTAB   R10 --> table, R11 <== #of entries
                                                                SPACE
TESTLOOP CLI   TESTONLY,0         Do only specific test?
         BE    TESTTHIS           No, do all tests
         CLC   TESTONLY,3(R10)    Is the test they want?
         BNE   TESTNEXT           No, skip this test
                                                                SPACE
TESTTHIS LM    R0,R1,(TESTLEN-(2*4))(R10)  R0 <== MSG LEN, R1 --> MSG
         BAL   R14,MSG                     Report which test this is
                                                                SPACE
         LM    R0,R2,0(R10)       Load test parms from table
         BAL   R14,DOTEST         Perform this test...
TESTNEXT LA    R10,TESTLEN(,R10)  R10 --> next test table entry
                                                                SPACE
         BCT   R11,TESTLOOP       Loooop... until no more tests
                                                                SPACE 4
         LPSWE GOODPSW            E7TEST SUCCESS!
                                                                EJECT
***********************************************************************
*        Generic TEST subroutine:  R0=test#, R1=chpgm, R2=flag
***********************************************************************
                                                                SPACE
DOTEST   ST    R14,TESTR14        Save return address
                                                                SPACE
         STC   R0,TESTNUM         Save this test's test number
         LR    R0,R1              R0 --> This test's Channel Program
                                                                SPACE
         BAL   R15,EXCP           Execute this Channel Program...
                                                                SPACE
         L     R1,IOCBDID         R1 <== Subchannel
         L     R4,IOCBSIB         R4 <== SCHIB address
                                                                SPACE
         STSCH 0(R4)              Store Subchannel for our device
         BC    B'0111',FAILSCH    FAIL if anything other than CC0
                                                                SPACE
*        Verify correct/expected I/O completion...
                                                                SPACE
         LA    R5,SCHSCSW         R5 --> SCSW
                                                                SPACE
         CLI   SCSWCS,0           Clean channel status?
         BNE   FAILTEST           No?! ALWAYS FAIL THE TEST!
                                                                SPACE
         LTR   R2,R2              I/O error expected for this test?
         BNZ   ERRTEST            Yes, then verify there was an error
                                                                SPACE
         CLI   SCSWUS,SCSWCE+SCSWDE   Check for normal successful I/O
         BNE   FAILTEST               No?! FAIL!
         B     TESTOK                 Yes, then we're done; return
                                                                SPACE
ERRTEST  CLI   SCSWUS,SCSWCE+SCSWDE   Check for normal successful I/O
         BE    FAILTEST               Yes?! UNEXPECTED! FAIL!
         BAL   R15,DOSENSE            Clear the error
                                                                SPACE
TESTOK   L     R14,TESTR14        Restore R14 return address
         BR    R14                Return to caller
                                                                SPACE 2
TESTR14  DC    A(0)     Test subroutine saved R14 return address
                                                                EJECT
***********************************************************************
*                   Disabled Wait PSWs...
***********************************************************************
                                                                SPACE 3
*    Test failure routines to load specific failure PSW...
                                                                SPACE 2
FAILCPU0 LA    R9,BAD66PSW          SIGP failed
         B     FAIL
FAILSCH  LA    R9,BAD77PSW          STSCH failed
         B     FAIL
FAILDEV  LA    R9,BAD88PSW          ENADEV failed
         B     FAIL
FAILIO   LA    R9,BAD99PSW          RAWIO failed
         B     FAIL
FAILTEST LA    R9,FAILPSW           One of our overall tests failed
         B     FAIL
                                                                SPACE 3
FAIL     MVC   16-1(1,R9),TESTNUM   Put failing test# into PSW
         LPSWE 0(R9)                Load failure PSW
                                                                SPACE 4
*
**    Overall test SUCCESS / FAILURE disabled wait PSWs...
*
                                                                SPACE 2
GOODPSW  DC    0D'0',XL8'0002000180000000',AD(X'00000000')
FAILPSW  DC    0D'0',XL8'0002000180000000',AD(X'0BAD0000')
                                                                SPACE 4
*
**    Specific unexpected failure disabled wait PSWs...
*
                                                                SPACE 2
BAD66PSW DC    0D'0',XL8'0002000180000000',AD(X'0BAD6600')
BAD77PSW DC    0D'0',XL8'0002000180000000',AD(X'0BAD7700')
BAD88PSW DC    0D'0',XL8'0002000180000000',AD(X'0BAD8800')
BAD99PSW DC    0D'0',XL8'0002000180000000',AD(X'0BAD9900')
                                                                EJECT
***********************************************************************
*        Program Initialization
***********************************************************************
                                                                SPACE
INIT     LA    R3,IOCB_A80        R3 --> IOCB
         LG    R8,IOCBORB         R8 --> ORB
         BAL   R15,IOINIT         Init CPU for I/O operations
         BAL   R15,ENADEV         Enable device for I/O
         BR    R14                Return to caller
                                                                SPACE 2
***********************************************************************
*        Initialize the CPU for I/O operations
***********************************************************************
                                                                SPACE
IOINIT   IOINIT ,
         BR    R15                Return to caller
                                                                SPACE 3
***********************************************************************
*        Enable the device, making it ready for use
***********************************************************************
                                                                SPACE
ENADEV   ENADEV  ENAOKAY,FAILDEV,REG=4
                                                                SPACE
ENAOKAY  BR    R15                Return to caller if device enabled OK
                                                                EJECT
***********************************************************************
*        Execute the channel program pointed to by R0
***********************************************************************
                                                                SPACE
DOSENSE  LA    R0,SENSEPGM          R0 -> Read SENSE Channel Program
EXCP     ST    R0,ORBCCW            Plug Channel Program into IORB
         LGR   R0,R4                Save SCHIB pointer
         MVI   ORB1_8,ORBF+ORBH     Format-1 CCWs, Format-2 IDAWs
         MVI   ORRB1_24,0           Set all these ORB flags to zero
                                                                SPACE
         RAWIO 4,FAIL=FAILIO
                                                                SPACE
         LGR   R4,R0                Restore SCHIB pointer
         BR    R15                  Return to caller
                                                                EJECT
***********************************************************************
*        Issue HERCULES MESSAGE pointed to by R1, length in R0
***********************************************************************
                                                                SPACE
MSG      CH    R0,=H'0'               Do we even HAVE a message?
         BNHR  R14                    No, ignore
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
         BR    R14                    Return to caller
                                                                SPACE
MSGSAVE  DC    3F'0'                  Registers save area
MSGMVC   MVC   MSGMSG(0),0(R1)        Executed instruction
                                                                SPACE
MSGCMD   DC    C'MSGNOH * '           *** HERCULES MESSAGE COMMAND ***
MSGMSG   DC    CL128' '               The message text to be displayed
                                                                EJECT
***********************************************************************
*                         IOCB
***********************************************************************
*
*         I/O Control Block -- Structure used by RAWIO macro
*         identifying the device and operation being performed
*
***********************************************************************
                                                                SPACE
IOCB_A80 IOCB  X'A80'     I/O Control Block for CCUU device X'A80'
                                                                EJECT
***********************************************************************
*                      WORKING STORAGE
***********************************************************************
                                                                SPACE
WKSTORG  EQU   *
         ORG   E7TEST+X'FFF'
TESTONLY DC    AL1(0)             Only do this one test if non-zero
         ORG   WKSTORG
                                                                SPACE
CC       EQU   X'40'              Chain Command
SLI      EQU   X'20'              Suppress Incorrect Length Indication
IDA      EQU   X'04'              Indirect Data Addressing
                                                                SPACE
SNS      EQU   X'04'              Basic Sense CCW opcode
WD       EQU   X'05'              Write Data
RD       EQU   X'06'              Read Data CCW opcode
SEEK     EQU   X'07'              Seek to BBCCHH
TIC      EQU   X'08'              Transfer in Channel
RSD      EQU   X'3E'              Read Subsystem Data CCW opcode
LR       EQU   X'47'              Locate Record CCW opcode
DX       EQU   X'63'              Define Extent CCW opcode
SIDEQ    EQU   X'31'              Search ID Equal
RDMT     EQU   X'86'              Read Data Multi-track CCW opcode
PFX      EQU   X'E7'              Prefix CCW opcode
                                                                SPACE
ATESTTAB DC    A(TESTTAB,NUMTESTS) Address of testtab & Number of tests
                                                                SPACE
TESTNUM  EQU   X'200'             Current test number (if failure,
*                                 identifies which test failed)
                                                                EJECT
***********************************************************************
*                    TESTS CONTROL TABLE
***********************************************************************
                                                                SPACE
         PRINT DATA
                                                                SPACE
TESTTAB  DC    0A(0)
                                                                SPACE
         DC    A(1,T1_CHPGM,0,T1_MSGLN,T1_DESC)
TESTLEN  EQU   (*-TESTTAB)            Width of each test table entry
                                                                SPACE 2
         DC    A(2,T2_CHPGM,0,T2_MSGLN,T2_DESC)
         DC    A(3,T3_CHPGM,0,T3_MSGLN,T3_DESC)
         DC    A(4,T4_CHPGM,0,T4_MSGLN,T4_DESC)
         DC    A(5,T5_CHPGM,1,T5_MSGLN,T5_DESC)  (1=Expect I/O ERROR)
         DC    A(6,T6_CHPGM,0,T6_MSGLN,T6_DESC)
         DC    A(7,T7_CHPGM,0,T7_MSGLN,T7_DESC)
         DC    A(8,T8_CHPGM,0,T8_MSGLN,T8_DESC)
         DC    A(9,T9_CHPGM,1,T9_MSGLN,T9_DESC)  (1=Expect I/O ERROR)
                                                                SPACE
         PRINT NODATA
                                                                SPACE 2
NUMTESTS EQU   (*-TESTTAB)/TESTLEN    Number of test table entries
                                                                SPACE 4
         LTORG ,        Literals Pool
                                                                EJECT
***********************************************************************
*                 CHANNEL PROGRAMS...
***********************************************************************
                                                                SPACE
         DC    0D'0'
SENSEPGM DC    AL1(SNS),AL1(SLI),AL2(L'SNSBYTES),AL4(SNSBYTES)
                                                                SPACE 5
***********************************************************************
                                                                SPACE
T1_DESC  DC    C'TEST #1: Format 2 PFX to obtain subsystem information (no IDA)'
T1_MSGLN EQU   *-T1_DESC
         DC    0D'0'
T1_CHPGM DC    AL1(PFX),AL1(CC+SLI),AL2(T1_E7LEN),AL4(T1_E7DAT)
         DC    AL1(RSD),AL1(SLI),AL2(L'T1_3EBUF),AL4(T1_3EBUF)
                                                                SPACE 5
***********************************************************************
                                                                SPACE
T2_DESC  DC    C'TEST #2: Format 0 PFX with Define Extent Valid bit off (DX CCW chained) (Read 06 IDA)'
T2_MSGLN EQU   *-T2_DESC
         DC    0D'0'
T2_CHPGM DC    AL1(PFX),AL1(CC+SLI),AL2(L'T2_E7DAT),AL4(T2_E7DAT)
         DC    AL1(DX),AL1(CC+SLI),AL2(L'T2_63DAT),AL4(T2_63DAT)
         DC    AL1(LR),AL1(CC+SLI),AL2(L'T2_47DAT),AL4(T2_47DAT)
         DC    AL1(RD),AL1(SLI+IDA),AL2(L'T2_06BUF),AL4(T2_06IDA)
T2_06IDA DC    AD(T2_06BUF)
                                                                SPACE 5
***********************************************************************
                                                                SPACE
T3_DESC  DC    C'TEST #3: Format 0 PFX with Define Extent Valid bit on (DX CCW imbeded) (Read 06 1 IDA)'
T3_MSGLN EQU   *-T3_DESC
         DC    0D'0'
T3_CHPGM DC    AL1(PFX),AL1(CC+SLI),AL2(L'T3_E7DAT),AL4(T3_E7DAT)
         DC    AL1(LR),AL1(CC+SLI),AL2(L'T3_47DAT),AL4(T3_47DAT)
         DC    AL1(RD),AL1(SLI+IDA),AL2(L'T3_06BUF),AL4(T3_06IDA)
T3_06IDA DC    AD(T3_06BUF)
                                                                EJECT
***********************************************************************
                                                                SPACE
T4_DESC  DC    C'TEST #4: Format 2 PFX to obtain control unit information (PFX E7 2 IDA, Read 06 1 IDA)'
T4_MSGLN EQU   *-T4_DESC
         DC    0D'0'
T4_CHPGM DC    AL1(PFX),AL1(CC+SLI+IDA),AL2(L'T4_E7DAT),AL4(T4_E7IDA)
         DC    AL1(RSD),AL1(SLI+IDA),AL2(L'T4_3EBUF),AL4(T4_3EIDA)
T4_E7IDA DC    AD(T4_E7DAT_PART1)
         DC    AD(T4_E7DAT_PART2)
T4_3EIDA DC    AD(T4_3EBUF)
                                                                SPACE 5
***********************************************************************
                                                                SPACE
T5_DESC  DC    C'TEST #5: Read 06 CCW should fail since LR operation is Read(16) and Read 06 CCW not multi-track (Read 06 1 IDA)'
T5_MSGLN EQU   *-T5_DESC
         DC    0D'0'
T5_CHPGM DC    AL1(PFX),AL1(CC+SLI),AL2(L'T5_E7DAT),AL4(T5_E7DAT)
         DC    AL1(LR),AL1(CC+SLI),AL2(L'T5_47DAT),AL4(T5_47DAT)
         DC    AL1(RD),AL1(SLI+IDA),AL2(L'T5_06BUF),AL4(T5_06IDA)
T5_06IDA DC    AD(T5_06BUF)
                                                                SPACE 5
***********************************************************************
                                                                SPACE
T6_DESC  DC    C'TEST #6: Same as Test #5, but properly uses multi-track Read (86) (Read 86 1 IDA)'
T6_MSGLN EQU   *-T6_DESC
         DC    0D'0'
T6_CHPGM DC    AL1(PFX),AL1(CC+SLI),AL2(L'T6_E7DAT),AL4(T6_E7DAT)
         DC    AL1(LR),AL1(CC+SLI),AL2(L'T6_47DAT),AL4(T6_47DAT)
         DC    AL1(RDMT),AL1(SLI+IDA),AL2(L'T6_86BUF),AL4(T6_86IDA)
T6_86IDA DC    AD(T6_86BUF)
                                                                SPACE 5
***********************************************************************
                                                                SPACE
T7_DESC  DC    C'TEST #7: Peter''s z/VM SSI issue (PFX 01 CMDREJ)'
T7_MSGLN EQU   *-T7_DESC
         DC    0D'0'
T7_CHPGM DC    AL1(PFX),AL1(SLI),AL2(T7_E7LEN),AL4(T7_E7DAT)
                                                                EJECT
***********************************************************************
                                                                SPACE
T8_DESC  DC    C'TEST #8: Write Data erase remainder of track'
T8_MSGLN EQU   *-T8_DESC
         DC    0D'0'
T8_CHPGM DC    AL1(DX),AL1(CC),AL2(T8_DXLEN),AL4(T8_DXDAT)
         DC    AL1(LR),AL1(CC),AL2(T8_LRLEN),AL4(T8_LRDAT)
         DC    AL1(WD),AL1(0),AL2(T8_WDLEN),AL4(T8_WDDAT)
                                                                SPACE 5
***********************************************************************
                                                                SPACE
T9_DESC  DC    C'TEST #9: Read track 0 rec 3 (verify test #08 erase)'
T9_MSGLN EQU   *-T9_DESC
         DC    0D'0'
T9_CHPGM DC    AL1(SEEK),AL1(CC),AL2(T9_SKLEN),AL4(T9_SKDAT)
T9_SICCW DC    AL1(SIDEQ),AL1(CC),AL2(T9_SILEN),AL4(T9_SIDAT)
         DC    AL1(TIC),AL1(0),AL2(0),AL4(T9_SICCW)
         DC    AL1(RD),AL1(SLI),AL2(T9_RDLEN),AL4(T9_RDDAT)
                                                                EJECT
***********************************************************************
*               I/O DATA AND I/O BUFFERS...
***********************************************************************
                                                                SPACE
         DC    0D'0'
SNSBYTES DC    XL32'00'     (Generic SENSE buffer)
                                                                SPACE 3
***********************************************************************
                                                                SPACE 3
T1_E7DAT DC    X'02000000 00000000 00000000'           +00  PFX
         DC    X'00000000 00000000 00000000 00000000'  +12  DEF EXT
         DC    X'00000000 00000000 00000000 00000000'  +28
         DC    X'00000000 00000000 00000000 00000000'  +44  LREC EXD
         DC    X'0000'                                 +60
         DC    X'    1800 00000000 41000000 00000000'  +62  PSF
T1_E7LEN EQU   *-T1_E7DAT
T1_3EBUF DC    XL256'00'  (the subsystem data that was read)
                                                                SPACE 3
***********************************************************************
                                                                SPACE 3
T2_E7DAT DC    XL64'00'
T2_63DAT DC    XL16'40C00000 00000000 00000000 00000000'
T2_47DAT DC    XL16'06000001 00000000 00000000 03000000'
T2_06BUF DC    XL10'00'
                                                                SPACE 3
***********************************************************************
                                                                SPACE 3
T3_E7DAT DS   0XL64
         DC    XL16'00800000 00000000 00000000 40C00000'
         DC    XL16'00000000 00000000 00000000 00000000'
         DC    XL16'00000000 00000000 00000000 00000000'
         DC    XL16'00000000 00000000 00000000 00000000'
T3_47DAT DC    XL16'06000001 00000000 00000000 03000000'
T3_06BUF DC    XL10'00'
                                                                EJECT
                                                                SPACE 2
***********************************************************************
                                                                SPACE
T4_3EBUF            DC    XL256'00'
                    PRINT DATA
T4_ORG              EQU   *
T4_E7DAT_TOTAL_LEN  EQU   76
T4_E7DAT_PART1_LEN  EQU   40
T4_E7DAT_PART2_LEN  EQU   (T4_E7DAT_TOTAL_LEN-T4_E7DAT_PART1_LEN)
                    ORG   E7TEST+X'2000'-T4_E7DAT_PART1_LEN
T4_E7DAT            DS   0XL(T4_E7DAT_TOTAL_LEN)
T4_E7DAT_PART1      DS   0XL(T4_E7DAT_PART1_LEN)
                    DC    XL16'02000000 00000000 00000000 00000000'
                    DC    XL16'00000000 00000000 00000000 00000000'
                    DC    XL8' 00000000 00000000'
T4_E7DAT_PART2      DS   0XL(T4_E7DAT_PART2_LEN)
                    DC    XL8'                   00000000 00000000'
                    DC    XL16'00000000 00000000 00000000 00001800'
                    DC    XL12'00000000 41000000 00000000'
                    ORG   T4_ORG
                    PRINT NODATA
                                                                SPACE
***********************************************************************
                                                                SPACE 2
T5_E7DAT DS   0XL64
         DC    XL16'00800000 00000000 00000000 40C00000'
         DC    XL16'00000000 00000000 00000000 00000000'
         DC    XL16'00000000 00000000 00000000 00000000'
         DC    XL16'00000000 00000000 00000000 00000000'
T5_47DAT DC    XL16'16000001 00000000 00000000 03000000'
T5_06BUF DC    XL10'00'
                                                                SPACE 2
***********************************************************************
                                                                SPACE 2
T6_E7DAT DS   0XL64
         DC    XL16'00800000 00000000 00000000 40C00000'
         DC    XL16'00000000 00000000 00000000 00000000'
         DC    XL16'00000000 00000000 00000000 00000000'
         DC    XL16'00000000 00000000 00000000 00000000'
T6_47DAT DC    XL16'16000001 00000000 00000000 03000000'
T6_86BUF DC    XL10'00'
                                                                EJECT
                                                                SPACE 2
***********************************************************************
                                                                SPACE 2
T7_E7DAT DC    X'01800000 00000000 00000000'           +00  PFX
         DC    X'40C01000 00000042 00020000 00020000'  +12  DEF EXT
         DC    X'00000000 00000000 00000000 00000000'  +28
         DC    X'06000001 00020000 00020000 01290000'  +44  LREC EXD
         DC    X'00000000'                             +60
T7_E7LEN EQU   *-T7_E7DAT
                                                                SPACE 2
***********************************************************************
                                                                SPACE 2
T8_DXDAT DC    X'00C00000 00000000 00000000 00000000'  
T8_DXLEN EQU   *-T8_DXDAT

T8_LRDAT DC    X'0B000001 00000000 00000000 00000000'  
T8_LRLEN EQU   *-T8_LRDAT

T8_WDDAT DC    XL8'00'
T8_WDLEN EQU   *-T8_WDDAT
                                                                SPACE 2
***********************************************************************
                                                                SPACE 2
T9_SKDAT DC    X'000000000000'    BIN=0,CYL=0,HEAD=0
T9_SKLEN EQU   *-T9_SKDAT

T9_SIDAT DC    X'0000000003'      CC=0,HH=0,R=3
T9_SILEN EQU   *-T9_SIDAT

T9_RDDAT DC    CL80' '            Volume Serial
T9_RDLEN EQU   *-T9_RDDAT
                                                                SPACE 2
***********************************************************************
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
*        SCHIB DSECT
***********************************************************************
                                                                SPACE
         DSECTS NAME=SCHIB
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
         DSECTS PRINT=OFF,NAME=(ASA,CCW0,CCW1,CSW)
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
