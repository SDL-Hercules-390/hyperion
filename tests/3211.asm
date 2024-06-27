         TITLE 'Simple 3211 Printer Tests'
***********************************************************************
*
*                 Simple 3211 Printer Tests
*
***********************************************************************
*
*   This program verifies proper Hercules 3211 printer device handler
*   functionality.  It performs a series of I/O operations to a 3211
*   printer device and verifies the outcome (results) is as expected.
*   It is designed to run as a standalone test started via a restart
*   interrupt PSW at absolute address 0.
*
*        --------------------------------------------------
*          ALL TESTS SHOULD BE INDEPENDENT OF ONE ANOTHER!
*          NO TEST SHOULD DEPEND ON THE RESULT OF ANOTHER!
*        --------------------------------------------------
*
*   Each test is basically designed to test one thing, although most
*   tests perform several different variations of a given thing.
*
*   All tests are executed by default, but you can choose at runtime
*   which tests should be run and which should be skipped by setting
*   the corresponding "DOFLAGS" to either zero or non-zero. Setting
*   the DOFLAG to binary zero skips that test. A non-zero value will
*   cause the test to be executed. The "DOFLAGS" field should always
*   be at absolute address X'FF0' (16 bytes before the 2nd 4K page).
*
*        --------------------------------------------------
*          ALL TESTS SHOULD BE INDEPENDENT OF ONE ANOTHER!
*          NO TEST SHOULD DEPEND ON THE RESULT OF ANOTHER!
*        --------------------------------------------------
*
*   Once all tests are finished the resulting "RCFLAGS" are examined.
*   If they are all zero then a normal completion all zeros disabled
*   wait PSW is loaded. If all "RCFLAGS" are not zero then a failure
*   disabled wait PSW (whose instruction address is "BAD") is loaded
*   instead. The "RCFLAGS" field should always be at absolute address
*   X'1000' (i.e. the first 16 bytes of the 2nd 4K page).
*
***********************************************************************
                                                                EJECT
***********************************************************************
*
*   Example Hercules Testcase:
*
*
*         *Testcase 3211 printer
*         mainsize  1
*         numcpu    1
*         sysclear
*         archlvl   390
*         loadcore  "$(testpath)/3211.core"
*         #
*         # NOTE: In addition to the above 3211.core file this test
*         #       also uses an associated "3211.rexx" script too.
*         #
*         detach    00f
*         attach    00f 3211 "3211.txt"
*         diag8cmd  enable noecho   # need diag8 to exec rexx script
*         shcmdopt  enable diag8    # rexx script needs shell access
*         runtest   0.1             # (plenty of time)
*         detach    000f            # (no longer needed)
*         diag8cmd  disable noecho  # (no longer needed)
*         shcmdopt  disable nodiag8 # (no longer needed)
*         *Compare
*         r 1000.10
*         *Want "Return Code flags" 00000000 00000000 00000000 00000000
*         *Done
*
*
*   Refer to comments at label "BEGIN" for register usage.
*
***********************************************************************
                                                                SPACE 5
         PRINT OFF
         COPY  'satk.mac'
***********************************************************************
*   SETARCH macro for conditionally including an arch change or not
***********************************************************************
         MACRO
&LABEL   SETARCH &BASE
.*
         GBLA  &ARCHLVL     Current architecture level
.*
         AIF   ('&LABEL' EQ '').NOLBL
&LABEL   DS    0H
.NOLBL   ANOP
.*
         AIF   (&ARCHLVL LT 8).DONE
         AIF   ('&BASE' EQ '').NOCLR
.*
         ZEROLH  &BASE,1    Make sure bit 32 in 64-bit register,
.*                          is zero after change.
.NOCLR   ANOP
         ZARCH  6,5,SUCCESS=INZMODE,FAIL=FAIL   Change to 64-bit mode
.*                                              if capable.
.*
INZMODE  DS    0H
.DONE    ANOP
         MEND
         PRINT ON
                                                                SPACE
***********************************************************************
*        SATK prolog stuff...
***********************************************************************
                                                                SPACE
         ARCHLVL  ZARCH=NO,MNOTE=NO
                                                                EJECT
***********************************************************************
*        Initiate the TEST3211 CSECT in the CODE region
*        with the location counter at 0
***********************************************************************
                                                                SPACE
TEST3211 ASALOAD  REGION=CODE
                                                                SPACE 5
***********************************************************************
*        Create IPL (restart) PSW
***********************************************************************
                                                                SPACE
         ASAIPL  IA=BEGIN
                                                                EJECT
***********************************************************************
*               The actual TEST3211 program itself...
***********************************************************************
*
*  Architecture Mode: ESA/390
*  Addressing Mode:   24-bit
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
                                                                SPACE 2
         BAL   R14,TEST01       z/VM 6.3 printer 3211 initial sequence
         BAL   R14,TEST02       Skip to nonexistent FCB channel
         BAL   R14,TEST03       Skip to chan we're at = No Skip
         BAL   R14,TEST04       Skip to chan we're at = Should Skip
         BAL   R14,TEST05       Channel  9 crossed
         BAL   R14,TEST06       Channel 12 crossed
         BAL   R14,TEST07       FCB/UCS Load Check
         BAL   R14,TEST08       Diagnostic Read FCB
         BAL   R14,TEST09       Diagnostic Write/Read PLB
                                                                SPACE 2
         OC    RCFLAGS,RCFLAGS  Did all tests succeed? (all zeros?)
         BNZ   FAIL             No, Abnormal termination
         B     EOJ              Yes, Normal completion
                                                                EJECT
***********************************************************************
*        Program Initialization
***********************************************************************
                                                                SPACE
INIT     DS     0H              Program Initialization
                                                                SPACE
         SETARCH  2             Cleanly enter 64-bit mode if sensible
                                                                SPACE
         LA     R3,IOCB_00F     Point to IOCB
         L      R8,IOCBORB      Point to ORB
         L      R15,IOCBIRB     Point to IRB
         USING  IRB,R15         Temporary addressability
         LA     R9,IRBSCSW      Point to SCSW
         DROP   R15             Done with IRB
                                                                SPACE
         BAL    R15,IOINIT      Initialize the CPU for I/O operations
         BAL    R15,ENADEV      Enable our device making ready for use
                                                                SPACE
         MVC    RCFLAGS,DOFLAGS Initialize test return code flags
         BR     R14             Return to caller
                                                                SPACE 4
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE
FAIL     DWAIT LOAD=YES,CODE=BAD    Abnormal termination
                                                                SPACE 2
FAILD8   DWAIT LOAD=YES,CODE=D8     Diagnose X'008' failed
                                                                SPACE 2
EOJ      DWAITEND LOAD=YES          Normal completion
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
ENADEV   ENADEV  ENAOKAY,FAIL,REG=4
*
ENAOKAY  BR    R15            Return to caller if device enabled OK
                                                                EJECT
***********************************************************************
*        TEST01:  z/VM 6.3 printer 3211 initial sequence
***********************************************************************
*
*   A    z/VM 6.3 sequence: 07, 06, 04.
*        06 == encoded current line number.
*
*   B    z/VM 6.3 sequence: 0B, 07, 06, 04.
*        06 == encoded current line number.
*
***********************************************************************
                                                                SPACE
TEST01   CLI   FLAG01,0         Should we do this test?
         BER   R14              No, skip this test
                                                                SPACE
         LA    R0,CHPGM01A      Diagnostic Gate, Check Read
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         CLI   CKRD01A,X'40'    Expected value? (line #1)
         BNER  R14              No, FAIL
                                                                SPACE
         LA    R0,CHPGM01B      Space 1, Diagnostic Gate, Check Read
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         CLI   CKRD01B,X'C0'    Expected value? (line #2)
         BNER  R14              No, FAIL
                                                                SPACE
         MVI   FLAG01,0         Test successful
         BR    R14              Return to caller
                                                                EJECT
***********************************************************************
*        TEST02:  Skip to nonexistent FCB channel
***********************************************************************
*
*   A    Load FCB without channel 2.
*
*   B    Skip to channel 2.
*        Should be error.
*
***********************************************************************
                                                                SPACE
TEST02   CLI   FLAG02,0         Should we do this test?
         BER   R14              No, skip this test
                                                                SPACE
         LA    R0,CHPGM02A      Load the test FCB
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         LA    R0,CHPGM02B      Skip to non-existent channel
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BNOR  R14              No, FAIL
                                                                SPACE
         BAL   R15,EXCPSENS     Get the sense information
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         TM    SENSE+0,SNS0EQCK+SNS0DTCK
         BNOR  R14              Both not set, FAIL
         TM    SENSE+1,SNS1LPCK
         BNOR  R14              Not also set, FAIL
                                                                SPACE
         MVI   FLAG02,0         Test successful
         BR    R14              Return to caller
                                                                EJECT
***********************************************************************
*        TEST03:  Skip to chan we're at = No Skip
***********************************************************************
*
*   A    Skip to channel 12
*        Space n immed to reach channel 1
*
*   B    Skip to channel 1: should NOT skip!
*            (because we're already positioned at the
*            desired channel and nothing was printed)
*
***********************************************************************
                                                                SPACE
TEST03   CLI   FLAG03,0         Should we do this test?
         BER   R14              No, skip this test
                                                                SPACE
         LA    R0,CHPGM03A      Skip to chan 12, Space to chan 1
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         LA    R0,DIAG803A      DIAG8 parameters
         BAL   R15,HCMD         Printer file size BEFORE skip attempt
                                                                SPACE
         LA    R0,CHPGM03B      Skip to channel 1
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         LA    R0,DIAG803B      DIAG8 parameters
         BAL   R15,HCMD         Printer file size AFTER skip attempt
                                                                SPACE
         LM    R11,R12,=A(SIZ03A,SIZ03B)
         CLC   0(L'SIZ03A,R11),0(R12)     Same size?
         BNER  R14                        No, FAIL
                                                                SPACE
         MVI   FLAG03,0         Test successful
         BR    R14              Return to caller
                                                                EJECT
***********************************************************************
*        TEST04:  Skip to chan we're at = Should Skip
***********************************************************************
*
*   A    Print and space 0 (i.e. no spacing)
*
*   B    Skip to channel 1: SHOULD skip this time!
*            (even though we ARE already positioned at
*            channel 1), because something WAS printed!
*
***********************************************************************
                                                                SPACE
TEST04   CLI   FLAG04,0         Should we do this test?
         BER   R14              No, skip this test
                                                                SPACE
         LA    R0,CHPGM04A      Write no spacing (while at chan 1)
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         LA    R0,DIAG804A      DIAG8 parameters
         BAL   R15,HCMD         Printer file size BEFORE skip attempt
                                                                SPACE
         LA    R0,CHPGM04B      Skip to channel 1
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         LA    R0,DIAG804B      DIAG8 parameters
         BAL   R15,HCMD         Printer file size AFTER skip attempt
                                                                SPACE
         LM    R11,R12,=A(SIZ04A,SIZ04B)
         CLC   0(L'SIZ04A,R11),0(R12)     Same size?
         BER   R14                        Yes, FAIL
                                                                SPACE
         MVI   FLAG04,0         Test successful
         BR    R14              Return to caller
                                                                EJECT
***********************************************************************
*        TEST05:  Channel 9 crossed
***********************************************************************
*
*   A    Skip to channel 8 (two lines before channel 9)
*        Print and space 3
*        Should cause Unit Check error, sense = ch9 CROSSED
*
*   B    Skip to channel 8 (two lines before channel 9)
*        Space 2 immed
*        Should cause Unit Check error, sense = ch9 REACHED
*
*   Note: this test depends on the FCB loaded by Test02
*
***********************************************************************
                                                                SPACE
TEST05   CLI   FLAG05,0         Should we do this test?
         BER   R14              No, skip this test
                                                                SPACE
         LA    R0,CHPGM05A      Skip to chan 8, space PAST chan 9
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BNOR  R14              No, FAIL
                                                                SPACE
         BAL   R15,EXCPSENS     Get the sense information
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         TM    SENSE+0,SNS0CH9  Chan9 sense?
         BNOR  R14              Not set, FAIL
                                                                SPACE
         LA    R0,CHPGM05B      Skip to chan 8, space TO chan 9
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BNOR  R14              No, FAIL
                                                                SPACE
         BAL   R15,EXCPSENS     Get the sense information
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         TM    SENSE+0,SNS0CH9  Chan9 sense?
         BNOR  R14              Not set, FAIL
                                                                SPACE
         MVI   FLAG05,0         Test successful
         BR    R14              Return to caller
                                                                EJECT
***********************************************************************
*        TEST06:  Channel 12 crossed
***********************************************************************
*
*   A    Skip to channel 11 (two lines before channel 12)
*        Space 3 immed
*        Should cause Unit Exception in CSW (channel 12 CROSSED)
*
*   B    Skip to channel 11 (two lines before channel 12)
*        Print and space 2
*        Should cause Unit Exception in CSW (channel 12 REACHED)
*
*   Note: this test depends on the FCB loaded by Test02
*
***********************************************************************
                                                                SPACE
TEST06   CLI   FLAG06,0         Should we do this test?
         BER   R14              No, skip this test
                                                                SPACE
         LA    R0,CHPGM06A      Skip to chan 11, space PAST chan 12
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         TM    SCSWUS,SCSWUX    Unit Exception set?
         BNOR  R14              No, FAIL
                                                                SPACE
         LA    R0,CHPGM06B      Skip to chan 11, space TO chan 12
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         TM    SCSWUS,SCSWUX    Unit Exception set?
         BNOR  R14              No, FAIL
                                                                SPACE
         MVI   FLAG06,0         Test successful
         BR    R14              Return to caller
                                                                EJECT
***********************************************************************
*        TEST07:  Load Check
***********************************************************************
*
*   A    Try loading FCB with more than 30 channel stops
*        Should cause Unit Check, SENSE = Load Check
*
*   D    Try loading FCB with channel code > 12.
*        Should cause Unit Check, SENSE = Load Check
*
*   E    Try loading FCB with missing end-of-form flag
*        Should cause Unit Check, SENSE = Load Check
*
*   F    Try loading UCS with less than required #of bytes
*        Should cause Unit Check, SENSE = Load Check,
*        REGARDLESS of SLI bit in CCW.
*
*   G    Try loading FCB with 31st channel stop @ end of form
*        Should NOT cause Unit Check! (Should succeed!)
*
*   H    Try loading FCB w/LESS than required length (w/o SLI!)
*        Should SUCCEED; 3211 never sets incorrect length for Load FCB
*
*   I    Try loading FCB w/MORE than required length (w/o SLI!)
*        Should SUCCEED; 3211 never sets incorrect length for Load FCB
*
***********************************************************************
                                                                SPACE
TEST07   CLI   FLAG07,0         Should we do this test?
         BER   R14              No, skip this test
                                                                SPACE
         LA    R0,CHPGM07A      Load FCB more than 30 channel stops
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BNOR  R14              No, FAIL
                                                                SPACE
         BAL   R15,EXCPSENS     Get the sense information
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         TM    SENSE+0,SNS0LDCK Load Check?
         BNOR  R14              No, FAIL
                                                                SPACE
         LA    R0,CHPGM07D      Load FCB with channel code > 12
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BNOR  R14              No, FAIL
                                                                SPACE
         BAL   R15,EXCPSENS     Get the sense information
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         TM    SENSE+0,SNS0LDCK Load Check?
         BNOR  R14              No, FAIL
                                                                SPACE
         LA    R0,CHPGM07E      Load FCB missing end-of-form flag
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BNOR  R14              No, FAIL
                                                                SPACE
         BAL   R15,EXCPSENS     Get the sense information
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         TM    SENSE+0,SNS0LDCK Load Check?
         BNOR  R14              No, FAIL
                                                                SPACE
         LA    R0,CHPGM07F      Load UCS shorter than required
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BNOR  R14              No, FAIL
                                                                SPACE
         BAL   R15,EXCPSENS     Get the sense information
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         TM    SENSE+0,SNS0LDCK Load Check?
         BNOR  R14              No, FAIL
                                                                SPACE
         LA    R0,CHPGM07G      Load FCB w/31st chan stop @ end of form
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         LA    R0,CHPGM07H      Load FCB shorter than required
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         LA    R0,CHPGM07I      Load FCB longer than required
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         MVI   FLAG07,0         Test successful
         BR    R14              Return to caller
                                                                EJECT
***********************************************************************
*        TEST08:  Diagnostic Read FCB
***********************************************************************
*
*   A    Load FCB (no indexing)
*        Diagnostic Gate (set diagnostic mode)
*        Diagnostic Read FCB
*        Returned data should match the FCB we loaded.
*
*   B    Load FCB (positive indexing)
*        Diagnostic Gate (set diagnostic mode)
*        Diagnostic Read FCB
*        Returned data should match the FCB we loaded.
*
*   C    Load FCB (negative indexing)
*        Diagnostic Gate (set diagnostic mode)
*        Diagnostic Read FCB
*        Returned data should match the FCB we loaded.
*
***********************************************************************
                                                                SPACE
TEST08   CLI   FLAG08,0         Should we do this test?
         BER   R14              No, skip this test
                                                                SPACE
         LA    R0,CHPGM08A      Load FCB (no idx), Diag, Read FCB
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         CLC   FCB08A2,FCB08A   Did we get back what we wrote?
         BNER  R14              Different, FAIL
                                                                SPACE
         LA    R0,CHPGM08B      Load FCB (+index), Diag, Read FCB
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         CLC   FCB08B2,FCB08B   Did we get back what we wrote?
         BNER  R14              Different, FAIL
                                                                SPACE
         LA    R0,CHPGM08C      Load FCB (-index), Diag, Read FCB
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         CLC   FCB08C2,FCB08C   Did we get back what we wrote?
         BNER  R14              Different, FAIL
                                                                SPACE
         MVI   FLAG08,0         Test successful
         BR    R14              Return to caller
                                                                EJECT
***********************************************************************
*        TEST09:  Diagnostic Write/Read PLB
***********************************************************************
*
*   A    Load any valid FCB
*        Normal write and space
*        Diagnostic Read PLB
*        Returned data should match what we wrote.
*
*   B    Diagnostic Write
*        No spacing should occur and NO DATA SHOULD BE WRITTEN.
*        Diagnostic Read PLB
*        Returned data should match what we wrote.
*
***********************************************************************
                                                                SPACE
TEST09   CLI   FLAG09,0         Should we do this test?
         BER   R14              No, skip this test
                                                                SPACE
         LA    R0,CHPGM09A      Write and Space, Diagnostic Read PLB
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         CLC   PLB09A,PRT09A    Did we get back what we wrote?
         BNER  R14              Different, FAIL
                                                                SPACE
         LA    R0,DIAG809A      DIAG8 parameters
         BAL   R15,HCMD         Printer file size BEFORE diag write
                                                                SPACE
         LA    R0,CHPGM09B      Diagnostic Write, Diagnostic Read PLB
         BAL   R15,EXCP         Do the I/O
         TM    SCSWUS,SCSWUC    Unit Check?
         BO    UCFAIL           Yes, FAIL
                                                                SPACE
         CLC   PLB09B,PRT09B    Did we get back what we wrote?
         BNER  R14              Different, FAIL
                                                                SPACE
         LA    R0,DIAG809B      DIAG8 parameters
         BAL   R15,HCMD         Printer file size AFTER diag write
                                                                SPACE
         LM    R11,R12,=A(SIZ09A,SIZ09B)
         CLC   0(L'SIZ09A,R11),0(R12)     Same size?
         BNER  R14                        No, FAIL
                                                                SPACE
         CLC   CKRD09B,CKRD09A  Same line position?
         BNER  R14              No, FAIL
                                                                SPACE
         MVI   FLAG09,0         Test successful
         BR    R14              Return to caller
                                                                EJECT
***********************************************************************
*        Fail test due to unexpected Unit Check condition
***********************************************************************
*
*        Tests which encounter an unexpected Unit Check will
*        branch to here to clear the error and fail their test.
*
***********************************************************************
                                                                SPACE
UCFAIL   BAL   R15,EXCPSENS       Do SENSE to clear Unit Check
         TM    SCSWUS,SCSWUC      Did the SENSE I/O fail?
         BNOR  R14                No, return to fail test
         DC    H'0'               *** SENSE FAILED?! ***
                                                                SPACE 7
***********************************************************************
*        Issue HERCULES DIAG X'008' command pointed to by R0
***********************************************************************
                                                                SPACE
HCMD     STM   R6,R10,HCMDSAVE      Save registers
                                                                SPACE
         LR    R10,R0               R10 -> HCMD parameters
         LM    R6,R9,0(R10)         Load Diag8 registers
         LA    R10,X'40'            X'40 = Use response buffer option
         SLL   R10,32-8             (shift into high-order byte)
         OR    R8,R10               Or option into cmd length reg
                                                                SPACE
         DC    X'83',X'68',X'0008'  Issue Hercules Diagnose X'008'
         BNZ   FAILD8               Abort if unsuccessful
                                                                SPACE
         LM    R6,R10,HCMDSAVE      Restore registers
         BR    R15                  Return to caller
                                                                SPACE
HCMDSAVE DC    5F'0'                Registers save area
                                                                EJECT
***********************************************************************
*        Execute the channel program pointed to by R0
***********************************************************************
                                                                SPACE
EXCPSENS LA    R0,SENSEPGM    R0 -> Retrieve SENSE Channel Program
                                                                SPACE
EXCP     ST    R0,ORBCCW      Plug Channel Program address into IORB
                                                                SPACE
         RAWIO 4,FAIL=FAIL
                                                                SPACE
         BR    R15            Return to caller
                                                                EJECT
***********************************************************************
*         Structure used by RAWIO identifying
*         the device and operation being performed
***********************************************************************
                                                                SPACE
IOCB_00F IOCB  X'00F',CCW=CHPGM01A
                                                                EJECT
***********************************************************************
*        CCW opcode equates, etc.
***********************************************************************
                                                                SPACE
CC       EQU   X'40'    Command Chain
SLI      EQU   X'20'    Suppress Incorrect Length Indication
SKIP     EQU   X'10'    Skip Data Transfer
                                                                SPACE
READPLB  EQU   X'02'    Diagnostic Read PLB
NOPCMD   EQU   X'03'    No Operation
SENSECMD EQU   X'04'    Basic Sense
WRITEPLB EQU   X'05'    Diagnostic Write PLB
CHKREAD  EQU   X'06'    Diagnostic Check Read
DIAGGATE EQU   X'07'    Diagnostic Gate
READUCS  EQU   X'0A'    Diagnostic Read UCB
READFCB  EQU   X'12'    Diagnostic Read FCB
LOADFCB  EQU   X'63'    Load Forms Control Buffer
LOADUCS  EQU   X'FB'    Load Universal Character Set Buffer
                                                                SPACE
FCBL3211 EQU   180      FCB Length for 3211 printer
UCBL3211 EQU   432      UCB Length for 3211 printer
                                                                SPACE
SP0AFTER EQU   X'01'    Write Without Spacing
SP1AFTER EQU   X'09'    Write And Space 1 Lines
SP2AFTER EQU   X'11'    Write And Space 2 Lines
SP3AFTER EQU   X'19'    Write And Space 3 Lines
                                                                SPACE
SP1NOW   EQU   X'0B'    Space 1 Line  Immediate
SP2NOW   EQU   X'13'    Space 2 Lines Immediate
SP3NOW   EQU   X'1B'    Space 3 Lines Immediate
                                                                SPACE
SKP1NOW  EQU   X'8B'    Skip to Channel 1 Immediate
SKP2NOW  EQU   X'93'    Skip to Channel 2 Immediate
SKP8NOW  EQU   X'C3'    Skip to Channel 8 Immediate
SKP11NOW EQU   X'DB'    Skip to Channel 11 Immediate
SKP12NOW EQU   X'E3'    Skip to Channel 12 Immediate
                                                                SPACE
SNS0EQCK EQU   X'10'    Sense byte 0, bit 3: Equipment Check
SNS0DTCK EQU   X'08'    Sense byte 0, bit 4: Data Check
SNS0LDCK EQU   X'02'    Sense byte 0, bit 6: Load Check
SNS0CH9  EQU   X'01'    Sense byte 0, bit 7: Channel 9 Crossed
SNS1LPCK EQU   X'10'    Sense byte 1, bit 3: Line Position Check
                                                                EJECT
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE
SENSEPGM CCW1  SENSECMD,SENSE,SLI,L'SENSE
SENSE    DC    XL2'0000'
TESTFCB  DC    X'000100080009000B000C0010'
                                                                SPACE
CKRD01A  DC    X'00'
CKRD01B  DC    X'00'
                                                                SPACE
DIAG803A DC    A(RXSAYSIZ),A(SIZ03A)
         DC    A(L'RXSAYSIZ),A(L'SIZ03A)
DIAG803B DC    A(RXSAYSIZ),A(SIZ03B)
         DC    A(L'RXSAYSIZ),A(L'SIZ03B)
                                                                SPACE
DIAG804A DC    A(RXSAYSIZ),A(SIZ04A)
         DC    A(L'RXSAYSIZ),A(L'SIZ04A)
PRT04A   DC    C'PRT04A'
DIAG804B DC    A(RXSAYSIZ),A(SIZ04B)
         DC    A(L'RXSAYSIZ),A(L'SIZ04B)
                                                                SPACE
PRT05A   DC    C'PRT05A'
                                                                SPACE
PRT06B   DC    C'PRT06B'
                                                                SPACE
FCB07A   DS   0XL32
         DC    X'0102030405060708090A'
         DC    X'0102030405060708090A'
         DC    X'0102030405060708090A'
         DC    X'0110'
FCB07D   DC    X'00000000000000000000001D'
FCB07E   DC    X'000000000000000000000000'
UCS07F   DC    C'UCS07F'
FCB07G   DS   0XL31
         DC    X'0102030405060708090A'
         DC    X'0102030405060708090A'
         DC    X'0102030405060708090A'
         DC    X'11'
FCB07H   DS    XL(FCBL3211-1)
FCB07I   DS    XL(FCBL3211+1)
         ORG   FCB07H
         DC    X'000100080009000B000C0010'
         ORG   FCB07I
         DC    X'000100080009000B000C0010'
         ORG   FCB07I+L'FCB07I
                                                                EJECT
FCB08A   DC    X'000100080009000B000C0010'
FCB08A2  DC    XL(L'FCB08A)'00'
FCB08B   DC    X'88000100080009000B000C0010'
FCB08B2  DC    XL(L'FCB08B)'00'
FCB08C   DC    X'CF000100080009000B000C0010'
FCB08C2  DC    XL(L'FCB08C)'00'
                                                                SPACE
PRT09A   DC    C'PRT09A'
PLB09A   DC    CL(L'PRT09A)' '
CKRD09A  DC    X'00'
DIAG809A DC    A(RXSAYSIZ),A(SIZ09A)
         DC    A(L'RXSAYSIZ),A(L'SIZ09A)
PRT09B   DC    C'PRT09B'
PLB09B   DC    CL(L'PRT09B)' '
CKRD09B  DC    X'00'
DIAG809B DC    A(RXSAYSIZ),A(SIZ09B)
         DC    A(L'RXSAYSIZ),A(L'SIZ09B)
                                                                EJECT
***********************************************************************
*        Place LARGE BUFFERS past our test flags
***********************************************************************
                                                                SPACE
SAVEORG  EQU   *                    (save where we are)
         ORG   TEST3211+4096+4096   (s/b @ X'2000')
                                                                SPACE
RXSAYSIZ DC    CL256'exec "$(testpath)/3211.rexx" 3211.txt'
                                                                SPACE
SIZ03A   DC    CL256'aa'
SIZ03B   DC    CL(L'SIZ03A)'bb'
                                                                SPACE
SIZ04A   DC    CL256'xx'
SIZ04B   DC    CL(L'SIZ04A)'xx'
                                                                SPACE
SIZ09A   DC    CL256'aa'
SIZ09B   DC    CL(L'SIZ09A)'bb'
                                                                SPACE
         ORG   SAVEORG              (go back to where we were)
                                                                EJECT
***********************************************************************
*        Test Channel Programs
***********************************************************************
                                                                SPACE
CHPGM01A CCW1  DIAGGATE,*,CC+SLI,1
         CCW1  CHKREAD,CKRD01A,CC+SLI,L'CKRD01A
         CCW1  SENSECMD,SENSE,SLI+SKIP,1
CHPGM01B CCW1  SP1NOW,*,CC+SLI,1
         CCW1  DIAGGATE,*,CC+SLI,1
         CCW1  CHKREAD,CKRD01B,CC+SLI,L'CKRD01B
         CCW1  SENSECMD,SENSE,SLI+SKIP,1
                                                                SPACE
CHPGM02A CCW1  LOADFCB,TESTFCB,SLI,L'TESTFCB
CHPGM02B CCW1  SKP2NOW,*,SLI,1
                                                                SPACE
CHPGM03A CCW1  LOADFCB,TESTFCB,CC+SLI,L'TESTFCB
         CCW1  SKP12NOW,*,CC+SLI,1
         CCW1  SP3NOW,*,CC+SLI,1
         CCW1  SP1NOW,*,SLI,1
CHPGM03B CCW1  SKP1NOW,*,SLI,1
                                                                SPACE
CHPGM04A CCW1  LOADFCB,TESTFCB,CC+SLI,L'TESTFCB
         CCW1  SKP12NOW,*,CC+SLI,1
         CCW1  SP3NOW,*,CC+SLI,1
         CCW1  SP1NOW,*,CC+SLI,1
         CCW1  SP0AFTER,PRT04A,SLI,L'PRT04A
CHPGM04B CCW1  SKP1NOW,*,SLI,1
                                                                SPACE
CHPGM05A CCW1  LOADFCB,TESTFCB,CC+SLI,L'TESTFCB
         CCW1  SKP8NOW,*,CC+SLI,1
         CCW1  SP3AFTER,PRT05A,0,L'PRT05A
CHPGM05B CCW1  SKP8NOW,*,CC+SLI,1
         CCW1  SP2NOW,*,SLI,1
                                                                SPACE
CHPGM06A CCW1  LOADFCB,TESTFCB,CC+SLI,L'TESTFCB
         CCW1  SKP11NOW,*,CC+SLI,1
         CCW1  SP3NOW,*,SLI,1
CHPGM06B CCW1  SKP11NOW,*,CC+SLI,1
         CCW1  SP2AFTER,PRT06B,SLI,L'PRT06B
                                                                EJECT
CHPGM07A CCW1  LOADFCB,FCB07A,SLI,L'FCB07A
CHPGM07D CCW1  LOADFCB,FCB07D,SLI,L'FCB07D
CHPGM07E CCW1  LOADFCB,FCB07E,SLI,L'FCB07E
CHPGM07F CCW1  LOADUCS,UCS07F,SLI,L'UCS07F
CHPGM07G CCW1  LOADFCB,FCB07G,SLI,L'FCB07G
CHPGM07H CCW1  LOADFCB,FCB07H,0,L'FCB07H
CHPGM07I CCW1  LOADFCB,FCB07I,0,L'FCB07I
                                                                SPACE
CHPGM08A CCW1  LOADFCB,FCB08A,CC+SLI,L'FCB08A
         CCW1  DIAGGATE,*,CC+SLI,1
         CCW1  READFCB,FCB08A2,SLI,L'FCB08A2
CHPGM08B CCW1  LOADFCB,FCB08B,CC+SLI,L'FCB08B
         CCW1  DIAGGATE,*,CC+SLI,1
         CCW1  READFCB,FCB08B2,SLI,L'FCB08B2
CHPGM08C CCW1  LOADFCB,FCB08C,CC+SLI,L'FCB08C
         CCW1  DIAGGATE,*,CC+SLI,1
         CCW1  READFCB,FCB08C2,SLI,L'FCB08C2
                                                                SPACE
CHPGM09A CCW1  LOADFCB,TESTFCB,CC+SLI,L'TESTFCB
         CCW1  SP1AFTER,PRT09A,CC,L'PRT09A
         CCW1  READPLB,PLB09A,CC+SLI,L'PLB09A
         CCW1  DIAGGATE,*,CC+SLI,1
         CCW1  CHKREAD,CKRD09A,SLI,L'CKRD09A
CHPGM09B CCW1  WRITEPLB,PRT09B,CC,L'PRT09B
         CCW1  READPLB,PLB09B,CC+SLI,L'PLB09B
         CCW1  DIAGGATE,*,CC+SLI,1
         CCW1  CHKREAD,CKRD09B,SLI,L'CKRD09B
                                                                EJECT
***********************************************************************
*        Literals Pool
***********************************************************************
                                                                SPACE
         LTORG ,
                                                                EJECT
***********************************************************************
*        Test control flags:   X'00' = skip test, otherwise do test
***********************************************************************
                                                                SPACE
         ORG   TEST3211+4096-16   (s/b @ X'FF0')
DOFLAGS  DS   0XL16               (s/b @ X'FF0')
                                                                SPACE 2
         DC    X'00'            TEST00
                                                                SPACE
         DC    C'1'             TEST01
         DC    C'2'             TEST02
         DC    C'3'             TEST03
         DC    C'4'             TEST04
         DC    C'5'             TEST05
         DC    C'6'             TEST06
         DC    C'7'             TEST07
         DC    C'8'             TEST08
         DC    C'9'             TEST09
                                                                SPACE
         DC    X'00'            TEST10
         DC    X'00'            TEST11
         DC    X'00'            TEST12
         DC    X'00'            TEST13
         DC    X'00'            TEST14
         DC    X'00'            TEST15
                                                                EJECT
***********************************************************************
*        Test results flags:   X'00' = Success,  Other = Failure
***********************************************************************
                                                                SPACE
         ORG   TEST3211+4096      (s/b @ X'1000')
RCFLAGS  DS   0XL16               (s/b @ X'1000')
         DC      16X'FF'          (s/b @ X'1000')
                                                                SPACE 2
FLAG00   EQU   RCFLAGS+0        TEST00
                                                                SPACE
FLAG01   EQU   RCFLAGS+1        TEST01
FLAG02   EQU   RCFLAGS+2        TEST02
FLAG03   EQU   RCFLAGS+3        TEST03
FLAG04   EQU   RCFLAGS+4        TEST04
FLAG05   EQU   RCFLAGS+5        TEST05
FLAG06   EQU   RCFLAGS+6        TEST06
FLAG07   EQU   RCFLAGS+7        TEST07
FLAG08   EQU   RCFLAGS+8        TEST08
FLAG09   EQU   RCFLAGS+9        TEST09
                                                                SPACE
FLAG10   EQU   RCFLAGS+10       TEST10
FLAG11   EQU   RCFLAGS+11       TEST11
FLAG12   EQU   RCFLAGS+12       TEST12
FLAG13   EQU   RCFLAGS+13       TEST13
FLAG14   EQU   RCFLAGS+14       TEST14
FLAG15   EQU   RCFLAGS+15       TEST15
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
