 TITLE '             TXFPER  --  Test TXF PER Event-Suppression option'
***********************************************************************
*                          TXFPER.ASM
***********************************************************************
*
*  This program performs a PER instruction trace of TXF transactions.
*  It enables PER instruction fetch events for a range of instructions
*  that includes two transactions.
*
*  The first transaction, a CONSTRAINED transaction, and a separate
*  second transaction being an unconstrained transaction with another
*  unconstrained transaction nested within it.
*
*  The test is performed with both the Instruction Fetch and Event-
*  Suppression (ES) PER flags set. It should trace all instructions
*  EXCEPT FOR the instructions comprising the actual transactions.
*
***********************************************************************
                                                                SPACE 2
TXFPER   START 0
         USING TXFPER,R0
                                                                SPACE 4
         ORG   TXFPER+X'8C'         Program interrupt code
PGMCODE  DC    F'0'                 Program interrupt code
PGM_PER_EVENT  EQU  X'80'           PER Event program interrupt code

                                                                SPACE 2
         ORG   TXFPER+X'96'         PER interrupt fields
PERCODE  DC    XL2'00'              PER interrupt code
PERIFNUL EQU   X'01'                PER IFetch Nullification event
PERADDR  DC    AD(0)                PER interrupt address
                                                                SPACE 4
PGMOPSW  EQU   TXFPER+X'150'        z Program Old PSW
                                                                SPACE
         ORG   TXFPER+X'1A0'        z Restart New PSW
         DC    X'0000000180000000'
         DC    AD(GO)
                                                                SPACE 2
         ORG   TXFPER+X'1D0'        z Program New PSW
         DC    X'0000000180000000'
         DC    AD(PGMRUPT)
                                                                EJECT
***********************************************************************
*                  Start of actual program...
***********************************************************************
                                                                SPACE
         ORG   TXFPER+X'200'
                                                                SPACE 2
***********************************************************************
*              Perform basic TXF sanity checks...
***********************************************************************
                                                                SPACE
GO       LA    R0,(L'FACLIST/8)-1           Store Facility List
         STFLE FACLIST
                                                                SPACE
         TM    FACLIST+ZAFACBYT,ZAFACBIT    z/Arch mode?
         JZ    ZAFAIL
                                                                SPACE
         TM    FACLIST+PAFACBYT,PAFACBIT    PPA available?
         JZ    PAFAIL
                                                                SPACE
         TM    FACLIST+TXFACBYT,TXFACBIT    TXF available?
         JZ    TXFAIL
                                                                SPACE
         TM    FACLIST+CTFACBYT,CTFACBIT    Constrained TXF?
         JZ    CTFAIL
                                                                SPACE 3
***********************************************************************
*                       Enable TXF
***********************************************************************
                                                                SPACE
         STCTG R0,R0,CTL0         Save CR0
         LG    R0,CTL0            Load into GR0
         OIHH  R0,CR0TXF          Enable TXF flag
         STG   R0,CTL0            Save GR0
         LCTLG R0,R0,CTL0         Load CR0
                                                                SPACE 5
***********************************************************************
*                       Begin test...
***********************************************************************
                                                                SPACE
         LCTLG R9,R11,PERCTL      Load CR9-CR11 PER Control Registers
         SSM   ENPER              Enable Program Event Recording
         BAL   R14,CTRANS         Execute a Constrained Transaction
         BAL   R14,UTRANS         Execute an Unconstrained Transaction
         J     SUCCESS            Done!
                                                                EJECT
***********************************************************************
*              Dummy Transactions to be Traced...
***********************************************************************
                                                                SPACE
BEGRANGE EQU   *                  Begin of PER Range
                                                                SPACE 4
CTRANS   LA    R1,1(R1,R1)
         TBEGINC 0,0              Begin Constrained Transaction
         LA    R2,2(R2,R2)
         TEND  ,                  End of Transaction
         LA    R3,3(R3,R3)
         BR    R14                Return to caller
                                                                SPACE 3
UTRANS   LGHI  R2,X'2000'         R2 --> TDB
         SLR   R15,R15            R15 <== failure count = none yet
URETRY   TBEGIN 0(R2),X'FE00'     unconstrained, WITH TDB, save R0-R13
         JNZ   UFAILED            CC != 0: aborted or can't be started
         LA    R4,4(R4,R4)
         TBEGIN 0,0               Begin Nested Transaction
         LA    R5,5(R5,R5)
         TEND  ,                  End of Nested Transaction
         LA    R6,6(R6,R6)
         TEND  ,                  End of Outermost Transaction
USKIP    LA    R7,7(R7,R7)
         BR    R14                Return to caller
                                                                SPACE 3
UFAILED  BRC   CC1,UFAILCC1       Indeterminate condition (unexpected)
         BRC   CC3,UFAILCC3       Persistent    condition (unexpected)
                                                                SPACE
         AHI   R15,1              Increment temporary failure count
         CHI   R15,3              Have we reached our maximum retry?
         JNL   USKIP              Yes, then do it the hard way
                                                                SPACE
         PPA   R15,0,1            Otherwise request assistance
         J     URETRY             And try the transaction again
                                                                SPACE
UFAILCC1 MVI   BADPSW+16-1,1      Unexpected CC1
         J     FAILURE            FAIL test
UFAILCC3 MVI   BADPSW+16-1,3      Unexpected CC3
         J     FAILURE            FAIL test
                                                                SPACE 4
ENDRANGE EQU   *                  End of PER Range
                                                                EJECT
***********************************************************************
*        Issue Hercules MESSAGE pointed to by R1, length in R0
***********************************************************************
                                                                SPACE
MSG      CH    R0,=H'0'             Do we even HAVE a message?
         BNHR  R15                  No, ignore
                                                                SPACE
         STM   R0,R2,MSGSAVE        Save registers
         CH    R0,=AL2(L'MSGMSG)    Message length within limits?
         BNH   MSGOK                Yes, continue
         LA    R0,L'MSGMSG          No, set to maximum
                                                                SPACE
MSGOK    LR    R2,R0                Copy length to work register
         BCTR  R2,0                 Minus-1 for execute
         EX    R2,MSGMVC            Copy message to O/P buffer
         LA    R2,1+L'MSGCMD(,R2)   Calculate true command length
         LA    R1,MSGCMD            Point to true command
                                                                SPACE
         DC    X'83',X'12',X'0008'  Issue Hercules Diagnose X'008'
         BZ    MSGRET               Return if successful
         DC    H'0'                 ** CRASH ** otherwise!
                                                                SPACE
MSGRET   LM    R0,R2,MSGSAVE        Restore registers
         BR    R15                  Return to caller
                                                                SPACE
MSGSAVE  DC    3F'0'                Registers save area
MSGMVC   MVC   MSGMSG(0),0(R1)      Executed instruction
                                                                SPACE
MSGCMD   DC    C'MSGNOH * '
MSGMSG   DC    C'12345678 ==> 12345678',C' ' (extra byte for unpk)
                                                                SPACE 3
***********************************************************************
*        Trace instructions that was either fetched or executed
***********************************************************************
                                                                SPACE
ITRACE   UNPK  MSGMSG(9),PERADDR+4(5)   Address of instruction
         MVI   MSGMSG+8,C' '
         TR    MSGMSG(8),HEXCHARS-X'F0'
                                                                SPACE
         L     R1,PERADDR+4             The instruction itself
         UNPK  MSGMSG+13(9),0(5,R1)
         TR    MSGMSG+13(8),HEXCHARS-X'F0'
                                                                SPACE
         LA    R1,MSGMSG
         LA    R0,L'MSGMSG
         BAL   R15,MSG                  "Trace" the instruction
         BR    R14
                                                                SPACE
HEXCHARS DC    CL16'0123456789ABCDEF'
                                                                EJECT
***********************************************************************
*                Program Interrupt Handler...
***********************************************************************
                                                                SPACE
PGMRUPT  TM    PGMCODE+3,PGM_PER_EVENT  Expected interrupt?
         JZ    ABORT                    No?! ** ABORT!! **
         STMG  R0,R15,PGMREGS           Save caller's registers
         BAL   R14,ITRACE               Trace the instruction
         LMG   R0,R15,PGMREGS           Restore caller's registers
         LPSWE PGMOPSW                  Return to caller...
                                                                SPACE
PGMREGS  DC    16D'0'                   Saved GR registers 0 - 15
                                                                SPACE 5
***********************************************************************
*          ABORT test run due to unexpected program interrupt
***********************************************************************
                                                                SPACE
ABORT    MVC   BADPSW+8+2(2),=XL2'DEAD'
         MVC   BADPSW+16-L'PGMCODE(L'PGMCODE),PGMCODE
         J     FAILURE
                                                                SPACE 5
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
         DC    0D'0'              (doubleword boundary)
FACLIST  DC    XL256'00'          Facility List
                                                                SPACE
CTL0     DC    D'0'               Control Register 0
CR0TXF   EQU   X'0080'            CR0 bit 8: TXF Control
CC1      EQU   B'0100'            Condition Code 1
CC3      EQU   B'0001'            Condition Code 3
                                                                SPACE
PERCTL   DC    AD(X'40400000')    CR9  = Ifetch + Event Suppress
         DC    AD(BEGRANGE)       CR10 = Range begining address
         DC    AD(ENDRANGE)       CR11 = Range ending   address
                                                                SPACE
GOODPSW  DC    XL8'0002000180000000'
         DC    XL4'00000000',A(X'00000000')
                                                                SPACE
BADPSW   DC    XL8'0002000180000000'
         DC    XL4'0000DEAD',A(X'000000FF')  (FF = Reason for Failure)
                                                                SPACE
SAVEADDR DC    A(0)               Saved PER Address
SAVEPERC DC    X'00'              Saved PER Code
ENPER    DC    B'01000000'        Enable PER bit in PSW
                                                                SPACE
ZAFACNUM EQU   2                  z/Architecture architectural mode is active
ZAFACBYT EQU   X'00'
ZAFACBIT EQU   X'20'
ZAFAIL   MVI   BADPSW+16-1,1
         J     FAILURE
                                                                SPACE
PAFACNUM EQU   49                 Processor-Assist Facility
PAFACBYT EQU   X'06'
PAFACBIT EQU   X'40'
PAFAIL   MVI   BADPSW+16-1,2
         J     FAILURE
                                                                SPACE
CTFACNUM EQU   50                 Constrained-Transactional-Execution Facility
CTFACBYT EQU   X'06'
CTFACBIT EQU   X'20'
CTFAIL   MVI   BADPSW+16-1,3
         J     FAILURE
                                                                SPACE
TXFACNUM EQU   73                 Transactional-Execution Facility
TXFACBYT EQU   X'09'
TXFACBIT EQU   X'40'
TXFAIL   MVI   BADPSW+16-1,4
         J     FAILURE
                                                                SPACE
         LTORG ,
                                                                SPACE 3
***********************************************************************
*                  Testing option byte
***********************************************************************
                                                                SPACE 2
         ORG   TXFPER+X'FFF'
                                                                SPACE
TESTFLAG DC    AL1(TEST2OPT+TEST3OPT+TEST4OPT+TEST5OPT)
                                                                SPACE 3
TEST1OPT EQU   X'80'    Perform Test 1
TEST2OPT EQU   X'40'    Perform Test 2
TEST3OPT EQU   X'20'    Perform Test 3
TEST4OPT EQU   X'10'    Perform Test 4
TEST5OPT EQU   X'08'    Perform Test 5
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
                                                                SPACE 2
         END
