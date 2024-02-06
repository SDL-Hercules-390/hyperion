 TITLE '                              ICKDSF related changes'
***********************************************************************
*
*                    ICKDSF related changes #615
*
***********************************************************************
*
*   This program verifies proper handling of various ICKDSF related
*   CCW chains related to home address alternate track handling as
*   documented in SDL-Hyperion GitHub Issue #615. A big thank you to
*   Anders Edlund for the actual tests. All I (Fish) did was to make
*   them into a formal QA (Quality Assurance) 'runtest' test.
*
***********************************************************************
*
*   Example Hercules Testcase:
*
*   NOTE: the 'attach' statements are actually very long, spanning
*   well past column 71, so they have been split into multiple lines
*   in the below example. In the actual test script they should each
*   be one long line.
*
*       *Testcase GH615 ICKDSF related changes
*
*       mainsize    2
*       numcpu      1
*       sysclear
*       archmode    S/370
*
*       attach      0333 3330 "$(testpath)/3330.cckd64" ro ...
*                   ... sf="$(testpath)/3330-shadow_*.cckd64"
*
*       attach      0338 3380 "$(testpath)/3380.cckd64" ro ...
*                   ... sf="$(testpath)/3380-shadow_*.cckd64"
*
*       attach      0339 3390 "$(testpath)/3390.cckd64" ro ...
*                   ... sf="$(testpath)/3390-shadow_*.cckd64"
*
*       sf+333
*       sf+338
*       sf+339
*
*       loadcore    "$(testpath)/GH615.core"
*       runtest     1.0
*
*       sf-333 nomerge
*       sf-338 nomerge
*       sf-339 nomerge
*
*       detach 0333
*       detach 0338
*       detach 0339
*
*       *Done
*
***********************************************************************
                                                                EJECT
***********************************************************************
*                         LOW CORE
***********************************************************************
                                                                SPACE 3
TEST     START 0
         USING TEST,0           Use absolute addressing
                                                                SPACE 2
         ORG   TEST+X'00'       Restart new PSW
         DC    XL4'00080000'
         DC    A(BEGIN)         --> Restart routine = program begin
                                                                SPACE 5
         ORG   TEST+X'44'       CSW unit/channel status
CSWUS    DC    X'00'            unit-status
CSWCS    DC    X'00'            channel-status
                                                                SPACE 5
         ORG   TEST+X'48'       CAW
CAW      DC    A(0)             --> Channel program
                                                                SPACE 5
         ORG   TEST+X'68'       Program new psw
         DC    XL4'000A0000'
         DC    A(X'DEAD')
                                                                EJECT
***********************************************************************
*                 MAIN TESTS EXECUTION LOOP...
***********************************************************************
                                                                SPACE
         ORG   TEST+X'200'
                                                                SPACE 2
*                   Register Usage...
*
*       R0  <==  Constant zero
*       R1  -->  Tests table
*
*       R2  <==  CUU to use for test
*       R3  -->  Test's Channel Program
*       R4  -->  Test's Verification Routine
*       R5  <==  Test's expectation: 0 = normal, 1 = I/O error
*       R6  <==  Test number
*
*       R13 -->  Where the failing test failed
*       R14 -->  Subroutine calling and return
                                                                SPACE 2
         USING TESTTAB,R1       TESTS table entry layout
                                                                SPACE 2
BEGIN    SLR   R0,R0            R0 <== constant zero
         L     R1,=A(TESTS)     R1 --> Tests Table
                                                                SPACE
TESTLOOP LH    R2,CUU           R2 <== CUU of device
         LM    R3,R4,ACHPROG    R3 --> Channel Program
*                               R4 --> verify Routine
         IC    R5,EXPECT        R5 <== Expectation
         IC    R6,TESTNUM       R6 <== Test number
                                                                SPACE
         MVC   BUFFER,=256X'FF' (Re-)Initialize generic buffer
         BAL   R14,DOTESTIO     Perform this test's I/O...
         BALR  R14,R4           Verify this test's results...
                                                                SPACE
         LA    R1,TESTNEXT      R1 --> Next table entry
                                                                SPACE
         CL    R0,0(,R1)        End of table?
         BNE   TESTLOOP         No, looooop...
         B     GOODEOJ          ALL TESTS SUCCEEDED!
                                                                SPACE 3
GOODEOJ  LPSW  GOODPSW          Load successful completion PSW
                                                                SPACE
FAILEOJ  STC   R6,FAILTEST      Plug test# into failure PSW
         SH    R13,=H'4'        Backup to actual failure location
         LPSW  FAILPSW          Load abnormal termination PSW
                                                                EJECT
***********************************************************************
*            Subroutine to perform an individual test
***********************************************************************
                                                                SPACE
DOTESTIO ST    R3,CAW           CAW --> channel program
                                                                SPACE
STARTIO  SIO   0(R2)            Start the I/O to the device...
         BC    B'0001',FAILIO   CC3 (not operational)
         BC    B'0010',STARTIO  CC2 (busy)
         BC    B'0100',CHECKIO  CC1 (CSW stored)
         BC    B'1000',TESTIO   CC0 (started)
                                                                SPACE
TESTIO   TIO   0(R2)            Test the I/O's progress...
         BC    B'0001',FAILIO   CC3 (not operational)
         BC    B'0010',TESTIO   CC2 (busy)
         BC    B'0100',CHECKIO  CC1 (CSW stored)
         BC    B'1000',FAILIO   CC0 (available)
                                                                SPACE
FAILIO   IC    R6,=X'33'        Indicate CUU error
         BAL   R13,FAILEOJ      TEST FAILED!
                                                                SPACE 3
CHECKIO  TM    CSWUS,X'02'      Check if this I/O had an error
         BNZ   ERRORIO          Go issue sense if it did
                                                                SPACE
         CLI   ERRFLAG,X'0E'    Was this the sense I/O?
         MVI   ERRFLAG,X'00'    Reset error flag
         BNER  R14              No, TEST SUCCESS! Return to caller
                                                                SPACE
         LTR   R5,R5            Was I/O error expected?
         BNZR  R14              Yes, TEST SUCCESS! Return to caller
                                                                SPACE
         BAL   R13,FAILEOJ      No, TEST FAILED!
                                                                SPACE 3
ERRORIO  MVI   ERRFLAG,X'0E'    Set I/O error flag in failure PSW
         LA    R3,SNSPGM        R3 --> sense channel program
         B     DOTESTIO         Go issue sense I/O
                                                                EJECT
***********************************************************************
*                 Test 1 verification routine
***********************************************************************
                                                                SPACE
VERIFY1  DS    0H
*
*     The last data area will contain the R0 of the track, i.e:
*
*           04590000 00000008 00000000 00000000
*
*     after the chain is complete.
*
                                                                SPACE 2
         MVC   RHADATA1,WHADATA1    (copy what test1 wrote)
         MVI   RHADATA1,X'00'       (but with leading 01 to 00 instead)
                                                                SPACE
         CLC   RHADATA1,BUFFER
         BE    VERIFY12
                                                                SPACE
         BAL   R13,FAILEOJ
                                                                SPACE 2
VERIFY12 CLC   WR0DATA1,BUFFER+16
         BER   R14
                                                                SPACE
         BAL   R13,FAILEOJ
                                                                EJECT
***********************************************************************
*                 Test 2 verification routine
***********************************************************************
                                                                SPACE
VERIFY2  DS    0H
*
*     The data area of X'0A' (DRHA = Diagnostic Read Home Address)
*     should contain the HA of 0004590000 at offset 19 decimal.
*
*     The X'16' command (RR0 = Read Record 0) should read the R0
*     of 04590000 00000008 00000000 00000000.
*
                                                                SPACE 2
         MVC   DHA219,WHADATA1    (copy what test1 wrote)
         MVI   DHA219,X'00'       (but with leading 01 to 00 instead)
                                                                SPACE
         CLC   DHA219,BUFFER+19
         BE    VERIFY22
                                                                SPACE
         BAL   R13,FAILEOJ
                                                                SPACE 2
VERIFY22 CLC   WR0DATA1,BUFFER+32
         BER   R14
                                                                SPACE
         BAL   R13,FAILEOJ
                                                                EJECT
***********************************************************************
*                 Test 3 verification routine
***********************************************************************
                                                                SPACE
VERIFY3  DS    0H
*
*     Should return a CMDREJ (80) sense with byte 07 set to 01.
*     Without the fix 02 would be returned.
*
                                                                SPACE 2
         CLI   SNSBYTES+7,X'01'   Message code 01?
         BER   R14                Yes, all is well
                                                                SPACE
         BAL   R13,FAILEOJ        No?! TEST FAILED!
                                                                EJECT
***********************************************************************
*                 Test 4 verification routine
***********************************************************************
                                                                SPACE
VERIFY4  DS    0H
*
*     Well it is really a change for 3340 or 3350 from 5,
*     which is for all older and newer!
*
                                                                SPACE 2
         MVC   RHADATA4,WHADATA4+6  (copy what was written)
         MVI   RHADATA4,X'00'       (but with leading 01 to 00 instead)
                                                                SPACE
         CLC   RHADATA4,BUFFER
         BE    VERIFY42
                                                                SPACE
         BAL   R13,FAILEOJ
                                                                SPACE 2
VERIFY42 CLC   WR0DATA4,BUFFER+16
         BER   R14
                                                                SPACE
         BAL   R13,FAILEOJ
                                                                EJECT
***********************************************************************
*                      WORKING STORAGE
***********************************************************************
                                                                SPACE
GOODPSW  DC    0D'0'            All tests succeeded PSW
         DC    XL4'000A0000'
         DC    XL4'00000000'
                                                                SPACE 3
FAILPSW  DC    0D'0'            Test failure PSW
         DC    XL4'000A0000'
         DC    XL1'00'
         DC    XL1'00'
ERRFLAG  DC    XL1'00'          if 0E = I/O error occurred
FAILTEST DC    XL1'FF'          Test number or X'33' = CUU error
                                                                SPACE 3
         LTORG ,                literals pool
                                                                EJECT
***********************************************************************
*                        EQUATES
***********************************************************************
                                                                SPACE
*
**                  CCW Flag Equates...
*
CD       EQU   X'80'            Chain data
CC       EQU   X'40'            Chain command
SLI      EQU   X'20'            Suppress length indication
                                                                SPACE 3
*
**                 CCW Command Equates...
*
SENSE    EQU   X'04'            Basic Sense
SEEK     EQU   X'07'            Seek
DRHA     EQU   X'0A'            Diagnostic Read Home Address
WR0      EQU   X'15'            Write Record 0
RR0      EQU   X'16'            Read Record 0
WHA      EQU   X'19'            Write Home Address
RHA      EQU   X'1A'            Read Home Address
SFM      EQU   X'1F'            Set File Mask
SETSECT  EQU   X'23'            Set Sector
SHAEQ    EQU   X'39'            Search Home Address Equal
LR       EQU   X'47'            Locate Record
DX       EQU   X'63'            Define Extent
RDC      EQU   X'64'            Read Device Characteristics                                                                EJECT
                                                                EJECT
***********************************************************************
*                        TESTS TABLE
***********************************************************************
                                                                SPACE
*             ------------------------------------
*             |  Table of tests to be performed  |
*             ------------------------------------
                                                                SPACE
TESTS    DC    0D'0'
                                                                SPACE
         DC    A(TEST1),A(VERIFY1),AL1(0),AL1(1),AL2(X'339')
         DC    A(TEST2),A(VERIFY2),AL1(0),AL1(2),AL2(X'339')
         DC    A(TEST3),A(VERIFY3),AL1(1),AL1(3),AL2(X'333')  (*)
         DC    A(TEST4),A(VERIFY4),AL1(0),AL1(4),AL2(X'338')
                                                                SPACE
         DC    A(0)     ZERO = End of table
                                                                SPACE 2
*                   (*)     I/O Error expected!
                                                                SPACE 9
*        Basic Sense channel program in case of I/O error
                                                                SPACE
         DC    0D'0'                  (alignment)
SNSPGM   DC    AL1(SENSE),AL3(SNSBYTES),AL1(SLI),AL1(0),AL2(L'SNSBYTES)
SNSBYTES DC    0XL32'FF',32X'FF'
                                                                SPACE 5
         DC    0D'0'                  (alignment)
BUFFER   DC    0XL256'FF',256X'FF'    Generic 256-byte data buffer
                                                                EJECT
***********************************************************************
*              TEST 1: Write HA in a ECKD chain (X'19')
***********************************************************************
                                                                SPACE
TEST1    DC    0D'0'
         DC    AL1(DX),AL3(DXDATA1),AL1(CC),AL1(0),AL2(L'DXDATA1)
         DC    AL1(LR),AL3(LRDATA1),AL1(CC),AL1(0),AL2(L'LRDATA1)
         DC    AL1(WHA),AL3(WHADATA1),AL1(CC),AL1(0),AL2(L'WHADATA1)
         DC    AL1(WR0),AL3(WR0DATA1),AL1(CC),AL1(0),AL2(L'WR0DATA1)
         DC    AL1(SETSECT),AL3(SECT1),AL1(CC),AL1(0),AL2(L'SECT1)
         DC    AL1(RHA),AL3(BUFFER),AL1(CC),AL1(0),AL2(5)
         DC    AL1(RR0),AL3(BUFFER+16),AL1(0),AL1(0),AL2(16)
                                                                SPACE 3
TEST1DAT DC    0D'0'
DXDATA1  DC    XL16'C2C4000000000000045900000459000E'
LRDATA1  DC    XL16'43000002045900000459000000000000'
WHADATA1 DC    XL5'0104590000'
WR0DATA1 DC    XL16'04590000000000080000000000000000'
SECT1    DC    XL1'00'
RHADATA1 DC    XL5'0004590000'
                                                                EJECT
***********************************************************************
*              TEST 2: Diagnostic read HA (X'0A')
***********************************************************************
                                                                SPACE
TEST2    DC    0D'0'
         DC    AL1(DX),AL3(DXDATA2),AL1(CC),AL1(0),AL2(L'DXDATA2)
         DC    AL1(LR),AL3(LRDATA2),AL1(CC),AL1(0),AL2(L'LRDATA2)
         DC    AL1(DRHA),AL3(BUFFER),AL1(CC),AL1(0),AL2(28)
         DC    AL1(RR0),AL3(BUFFER+32),AL1(0),AL1(0),AL2(16)
                                                                SPACE 3
TEST2DAT DC    0D'0'
DXDATA2  DC    XL16'06C4000000000000045900000459000E'
LRDATA2  DC    XL16'D6000002045900000459000000000000'
DHA219   DC    XL5'0004590000' (same as WHADATA1 but with 00 not 01)
                                                                EJECT
***********************************************************************
*        TEST 3: Read Device Characteristic on pre-3380
***********************************************************************
                                                                SPACE
TEST3    DC    0D'0'
         DC    AL1(RDC),AL3(BUFFER),AL1(0),AL1(0),AL2(64)
                                                                EJECT
***********************************************************************
*         TEST 4: Actual byte count for Write HA on 3380
***********************************************************************
                                                                SPACE
TEST4    DC    0D'0'
         DC    AL1(SEEK),AL3(SEEKADR4),AL1(CC),AL1(0),AL2(L'SEEKADR4)
         DC    AL1(SFM),AL3(FMASK4),AL1(CC),AL1(0),AL2(L'FMASK4)
         DC    AL1(SETSECT),AL3(SECT41),AL1(CC),AL1(0),AL2(L'SECT41)
         DC    AL1(SHAEQ),AL3(SRCHHA4),AL1(CC),AL1(0),AL2(L'SRCHHA4)
         DC    AL1(WHA),AL3(WHADATA4),AL1(CC),AL1(0),AL2(L'WHADATA4)
         DC    AL1(WR0),AL3(WR0DATA4),AL1(CC),AL1(0),AL2(L'WR0DATA4)
         DC    AL1(SETSECT),AL3(SECT42),AL1(CC),AL1(0),AL2(L'SECT42)
         DC    AL1(RHA),AL3(BUFFER),AL1(CC),AL1(0),AL2(5)
         DC    AL1(RR0),AL3(BUFFER+16),AL1(0),AL1(0),AL2(16)
                                                                SPACE 3
TEST4DAT DC    0D'0'
SEEKADR4 DC    XL6'000003750000'
FMASK4   DC    XL1'C0'
SECT41   DC    XL1'00'
SECT42   DC    XL1'00'
SRCHHA4  DC    XL4'03750000'
WHADATA4 DC    XL11'0000000000000103750000'
WR0DATA4 DC    XL16'03750000000000080000000000000000'
RHADATA4 DC    XL5'0003750000'

                                                                EJECT
***********************************************************************
*                    Tests Table DSECT
***********************************************************************
                                                                SPACE
TESTTAB  DSECT
                                                                SPACE
ACHPROG  DS    A        Address of channel program
AVERIFY  DS    A        Address of verification routine
EXPECT   DS    X        0 = normal completion, 1 = I/O error expected
TESTNUM  DS    X        Test number
CUU      DS    H        CUU to be used for this test
                                                                SPACE
TESTNEXT EQU   *        Next table entry...
                                                                SPACE 5
                                                                SPACE 5
R0       EQU   0                Register 0
R1       EQU   1                Register 1
R2       EQU   2                Register 2
R3       EQU   3                Register 3
R4       EQU   4                Register 4
R5       EQU   5                Register 5
R6       EQU   6                Register 6
R7       EQU   7                Register 7
R8       EQU   8                Register 8
R9       EQU   9                Register 9
R10      EQU   10               Register 10
R11      EQU   11               Register 11
R12      EQU   12               Register 12
R13      EQU   13               Register 13
R14      EQU   14               Register 14
R15      EQU   15               Register 15
                                                                SPACE 5
         END   TEST
