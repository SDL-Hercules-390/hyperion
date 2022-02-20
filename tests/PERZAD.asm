 TITLE '             PERZAD  --  Quick PER Zero-Address Detection test'
***********************************************************************
*                          PERZAD.ASM
***********************************************************************
*
*  This program performs a quick test of PER Zero-Address Detection.
*
*  It is NOT an exhaustive test. It only tests a few instructions
*  to verify the PER Zero-Address Detection event either does, or
*  does not occur, for only a few of the more popular instructions.
*
*  Refer to pages 4-38 and 4-39 ("Zero-Address Detection") of the
*  SA22-7832-12 "z/Architecture Principles of Operation" manual for
*  more information about the PER Zero-Address Detection Facility.
*
***********************************************************************
                                                                EJECT
***********************************************************************
*                           Low Core
***********************************************************************
                                                                SPACE 3
PERZAD   START 0
         USING PERZAD,R0
                                                                SPACE 5
         ORG   PERZAD+X'8C'         Program interrupt code
PGMCODE  DC    F'0'                 Program interrupt code
PGM_PER_EVENT  EQU  X'80'           PER Event program interrupt code

                                                                SPACE 3
         ORG   PERZAD+X'96'         PER interrupt fields
PERCODE  DC    XL2'00'              PER interrupt code
PERADDR  DC    AD(0)                PER interrupt address
                                                                SPACE 3
PGMOPSW  EQU   PERZAD+X'150'        z Program Old PSW
                                                                SPACE 3
         ORG   PERZAD+X'1A0'        z Restart New PSW
         DC    X'0000000180000000'
         DC    AD(GO)
                                                                SPACE 3
         ORG   PERZAD+X'1D0'        z Program New PSW
         DC    X'0000000180000000'
         DC    AD(PGMRUPT)
                                                                SPACE 6
         ORG   PERZAD+X'200'        Start of actual program...
                                                                EJECT
***********************************************************************
*                       Begin tests...
***********************************************************************
                                                                SPACE
GO       LCTLG R9,R11,PERCTL      Init CR9-CR11 PER Control Registers
         SSM   ENPER              Enable Program Event Recording
                                                                SPACE
         MVC   0(2,R0),=XL2'0700'     (just go on to next instruction)
         MVC   2(4,R0),=XL4'47F00000' (to be fixed by tests before use)

BEGRANGE EQU   *                  Begin of PER Range
                                                                EJECT
***********************************************************************
*    Instructions that should NEVER cause a PER ZAD event...
***********************************************************************
                                                                SPACE
         LMG   R0,R15,ZEROREGS    Initialize all registers to zero
                                                                SPACE
         LA    R1,0
         LR    R1,R1
         ALR   R1,R1
         SLR   R1,R1
         CLR   R1,R1
         LTR   R1,R1
                                                                SPACE
         LA    R2,2               R2 --> branch instruction in low core
         MVC   2(2,R2),=S(B)
         B     0(,R1)
B        EQU   *
                                                                SPACE
         MVC   2(2,R2),=S(BR)
         BR    R1
BR       EQU   *
                                                                SPACE
         MVC   2(2,R2),=S(BCTR)
         LA    R3,3
         BCTR  R3,R1
BCTR     EQU   *
                                                                SPACE
         MVC   2(2,R2),=S(EX)
         LR    R3,R0
         EX    R3,0(,R1)
EX       EQU   *
                                                                EJECT
***********************************************************************
*    Instructions that should ALWAYS cause a PER ZAD event...
***********************************************************************
                                                                SPACE
         LMG   R0,R15,ZEROREGS    Reset all registers back to zero
                                                                SPACE
ZAD01    L     R0,0(,R1)
                                                                SPACE
ZAD02    ST    R0,0(,R1)
                                                                SPACE
ZAD03    TM    0(R1),X'80'
                                                                SPACE
ZAD04    MVC   0(1,R1),0(R2)
                                                                SPACE
ZAD05    CLC   0(1,R1),0(R2)
                                                                SPACE
ZAD06    CL    R0,0(,R1)
                                                                SPACE
ZAD07    CLI   0(R1),X'00'
                                                                SPACE
ZAD08    ICM   R0,15,0(R1)
                                                                SPACE
ZAD09    IC    R0,0(,R1)
                                                                SPACE
ZAD10    LM    R0,R15,0(R1)
                                                                SPACE
ZAD11    LH    R0,0(,R1)
                                                                SPACE
ZAD12    OI    0(R1),X'80'
                                                                SPACE
         LA    R3,1               Destination length must be non-zero
         LA    R5,X'FF'           Pad char to make src len reg non-zero
         SLL   R5,24              Move into high-order byte position
                                                                SPACE
ZAD13    MVCL  R2,R4
                                                                SPACE
ZAD14    CLCL  R2,R4
                                                                EJECT
***********************************************************************
*          Verify we've seen ALL expected PER ZAD events...
***********************************************************************
                                                                SPACE
ENDRANGE EQU   *                        End of PER Range
                                                                SPACE
         LA    R1,ZADOKTAB              R1 --> table
         LA    R2,NUMZADS               R2 <== number of table entries
                                                                SPACE
DONELOOP CLI   1(R1),X'FF'              Have we seen this event?
         JNE   DONEFAIL                 No?! ** FAIL!! **
                                                                SPACE
         LA    R1,L'ZADOKTAB(,R1)       Bump to next table entry
         BCT   R2,DONELOOP              Loooop... through all entries
                                                                SPACE
         J     SUCCESS                  Done! Successful Test!
                                                                SPACE 5
***********************************************************************
*           FAIL!  Missing PER Zero-Address Detection Event!
***********************************************************************
                                                                SPACE
DONEFAIL MVC   BADPSW+8+2(2),=XL2'0BAD' Indicate test failure
         MVI   BADPSW+16-1,BADNOZAD     Indicate failure code
         J     FAILURE                  ** FAIL!! **
                                                                EJECT
***********************************************************************
*                Program Interrupt Handler...
***********************************************************************
                                                                SPACE
PGMRUPT  TM    PGMCODE+3,PGM_PER_EVENT  Expected interrupt?
         JZ    ABORT                    No?! ** ABORT!! **
                                                                SPACE
         CLI   PERCODE,ZADEVENT         Zero-Address Detection event?
         BE    ZADCHECK                 Yes, go check event address
                                                                SPACE
         MVC   BADPSW+8+2(2),=XL2'0BAD' Indicate test failure
         MVI   BADPSW+16-1,BADPER       Indicate failure code
         J     FAILURE                  ** FAIL!! **
                                                                SPACE
ZADCHECK STMG  R0,R15,PGMREGS           Save caller's registers
         LA    R1,ZADOKTAB              R1 --> table
         LA    R2,NUMZADS               R2 <== number of table entries
                                                                SPACE
ZADLOOP  L     R3,4(,R1)                R3 <== Expected Event Address
         CL    R3,PERADDR+4             Expected Event Address?
         JNE   ZADNEXT                  No, try next entry
                                                                SPACE
         MVI   1(R1),X'FF'              Yes, flag as having been seen
         LMG   R0,R15,PGMREGS           Restore caller's registers
         LPSWE PGMOPSW                  Return to caller...
                                                                SPACE
ZADNEXT  LA    R1,L'ZADOKTAB(,R1)       Bump to next table entry
         BCT   R2,ZADLOOP               Loooop... to try next entry
                                                                SPACE
         MVC   BADPSW+8+2(2),=XL2'0BAD' Indicate test failure
         MVI   BADPSW+16-1,BADZAD       Indicate failure code
         J     FAILURE                  ** FAIL!! **
                                                                SPACE 2
PGMREGS  DC    16D'0'                   Saved GR registers 0 - 15
                                                                SPACE 4
***********************************************************************
*                   Test FAILURE codes...
***********************************************************************
                                                                SPACE
BADPER   EQU   X'01'    Unexpected PER Event Code
BADZAD   EQU   X'02'    Unexpected PER ZAD Event
BADNOZAD EQU   X'03'    Missing PER ZAD Event
                                                                EJECT
***********************************************************************
*          Table of expected PER Zero-Address Detection events
***********************************************************************
                                                                SPACE
         ORG   PERZAD+X'800'                 Fixed table location
ZADOKTAB DC    0D'0'                         PER ZAD Addresses Table
                                                                SPACE
*                 (nn)                        = test# ("ZADnn" label#)
*                  ||  X'FF'                  = event was detected
*                  ||    ||          A(xxxx)  = PER event address
*                  ||    ||            ||||
*                  ||    ||            ||||
*                  VV    VV            VVVV
                                                                SPACE
         DC    AL1(01),X'00',XL2'00',A(ZAD01)
         DC    AL1(02),X'00',XL2'00',A(ZAD02)
         DC    AL1(03),X'00',XL2'00',A(ZAD03)
         DC    AL1(04),X'00',XL2'00',A(ZAD04)
         DC    AL1(05),X'00',XL2'00',A(ZAD05)
         DC    AL1(06),X'00',XL2'00',A(ZAD06)
         DC    AL1(07),X'00',XL2'00',A(ZAD07)
         DC    AL1(08),X'00',XL2'00',A(ZAD08)
         DC    AL1(09),X'00',XL2'00',A(ZAD09)
         DC    AL1(10),X'00',XL2'00',A(ZAD10)
         DC    AL1(11),X'00',XL2'00',A(ZAD11)
         DC    AL1(12),X'00',XL2'00',A(ZAD12)
         DC    AL1(13),X'00',XL2'00',A(ZAD13)
         DC    AL1(14),X'00',XL2'00',A(ZAD14)
NUMZADS  EQU   (*-ZADOKTAB)/8           Number of table entries
                                                                SPACE 4
***********************************************************************
*          ABORT test run due to unexpected program interrupt
***********************************************************************
                                                                SPACE
ABORT    MVC   BADPSW+8+2(2),=XL2'DEAD'
         MVC   BADPSW+16-L'PGMCODE(L'PGMCODE),PGMCODE
         J     FAILURE
                                                                SPACE 4
***********************************************************************
*          Successful completion / Abnormal termination
***********************************************************************
                                                                SPACE
SUCCESS  LPSWE    GOODPSW         Load test completed successfully PSW
FAILURE  LPSWE    BADPSW          Load the test FAILED somewhere!! PSW
                                                                EJECT
***********************************************************************
*                      WORKING STORAGE
***********************************************************************
                                                                SPACE
ZEROREGS DC    16D'0'                   ZEROED GR registers 0 - 15
                                                                SPACE
GOODPSW  DC    XL8'0002000180000000'
         DC    XL4'00000000',A(X'00000000')
                                                                SPACE
BADPSW   DC    XL8'0002000180000000'
         DC    XL4'0000DEAD',A(X'000000FF')  (FF = Reason for Failure)
                                                                SPACE
CR9_ZEROADDR   EQU  X'04000000'         Zero-address Detection
ZADEVENT EQU   X'04'                    Zero-address Detection event
                                                                SPACE
PERCTL   DC    AD(CR9_ZEROADDR)         PER events
         DC    AD(BEGRANGE)             CR10 = Range begining address
         DC    AD(ENDRANGE)             CR11 = Range ending   address
                                                                SPACE
ENPER    DC    B'01000000'              Enable PER bit in PSW
                                                                SPACE
         LTORG ,                        Literals Pool
                                                                SPACE
R0       EQU   0                        Register equates
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
                                                                SPACE
         END
