 TITLE '             TXFPER  --  Test PER Tracing of TXF Transactions'
***********************************************************************
*                          TXFPER.ASM
***********************************************************************
*
*  This program performs a PER instruction trace of TXF transactions.
*  It enables PER instruction fetch events for a range of instructions
*  that includes two transactions.
*
*  The first transaction, a constrained transaction, and a separate
*  second transaction being an unconstrained transaction with another
*  unconstrained transaction nested within it.
*
*  Two tests are performed: the first test is performed with both the
*  Instruction Fetch (IF) and Event-Suppression (ES) PER flags set.
*  It should trace all instructions except for the instructions that
*  comprise the actual transactions themselves (i.e. the instructions
*  from, and including, the outermost TBEGIN/C instruction through
*  the TEND instruction that ends the transaction, are NOT traced).
*
*  The second test adds the IFetch Nullification (IFNUL) and TEND PER
*  Event flags to the mix. The second test should trace everything
*  the first test traced, but in addition, should also trace both the
*  TBEGIN/C and TEND instructions themselves too. This is controlled
*  by special Program Interrupt handling logic.
*
***********************************************************************
                                                                SPACE
TXFPER   START 0
         USING TXFPER,R0
                                                                SPACE 3
         ORG   TXFPER+X'8C'         Program interrupt code
PGMCODE  DC    F'0'                 Program interrupt code
PGM_PER_EVENT  EQU  X'80'           PER Event program interrupt code

                                                                SPACE
         ORG   TXFPER+X'96'         PER interrupt fields
PERCODE  DC    XL2'00'              PER interrupt code
PERADDR  DC    AD(0)                PER interrupt address
                                                                SPACE 3
PGMOPSW  EQU   TXFPER+X'150'        z Program Old PSW
                                                                SPACE
         ORG   TXFPER+X'1A0'        z Restart New PSW
         DC    X'0000000180000000'
         DC    AD(GO)
                                                                SPACE
         ORG   TXFPER+X'1D0'        z Program New PSW
         DC    X'0000000180000000'
         DC    AD(PGMRUPT)
                                                                EJECT
***********************************************************************
*                  Start of actual program...
***********************************************************************
                                                                SPACE
         ORG   TXFPER+X'200'
                                                                SPACE
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
                                                                SPACE
***********************************************************************
*                       Enable TXF
***********************************************************************
                                                                SPACE
         STCTG R0,R0,CTL0         Save CR0
         LG    R0,CTL0            Load into GR0
         OIHH  R0,CR0TXF          Enable TXF flag
         STG   R0,CTL0            Save GR0
         LCTLG R0,R0,CTL0         Load CR0
                                                                SPACE
***********************************************************************
*                       Begin tests...
***********************************************************************
                                                                SPACE
         LCTLG R9,R11,PERCTL      Init CR9-CR11 PER Control Registers
         SSM   ENPER              Enable Program Event Recording
         BAL   R14,CTRANS         Execute a Constrained Transaction
         BAL   R14,UTRANS         Execute an Unconstrained Transaction
                                                                SPACE
         MVI   MSGCMD+14,C'2'     Test #2...
         MVC   PERCTL+4(4),=A(CR9_IF+CR9_IFNUL+CR9_SUPPRESS+CR9_TEND)
         LCTLG R9,R11,PERCTL      New CR9-CR11 PER Control Registers
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
UFAILCC1 MVI   BADPSW+16-1,6      Unexpected CC1
         J     FAILURE            FAIL test
UFAILCC3 MVI   BADPSW+16-1,7      Unexpected CC3
         J     FAILURE            FAIL test
                                                                SPACE 4
ENDRANGE EQU   *                  End of PER Range
                                                                EJECT
***********************************************************************
*        Issue Hercules MESSAGE pointed to by R1, length in R0
***********************************************************************
                                                                SPACE
MSG      LTR   R0,R0                Do we even HAVE a message?
         BZR   R15                  No, ignore
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
MSGCMD   DC    C'MSGNOH * Test 1: '
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
                                                                SPACE
TEST2BR  NOP   PGMTEST2                 Branch ==> Test 2 logic
         TM    PERCODE,X'01'            CR9_IFNUL = Test 2 yet?
         BO    BEGTEST2                 Yes, Test 2 has begun
         BAL   R14,ITRACE               Trace executed instruction
         B     PGMRET                   (still Test 1)
                                                                SPACE
BEGTEST2 MVI   TEST2BR+1,X'F0'          Activate Test 2 logic
PGMTEST2 LG    R1,PERADDR               R1 --> instruction
         CLC   0(2,R1),=XL2'E560'       TBEGIN?
         BE    PGMTBEG                  Yes
         CLC   0(2,R1),=XL2'E561'       TBEGINC?
         BE    PGMTBEG                  Yes
         TM    PERCODE,X'02'            TEND PER event?
         BO    PGMTEND                  Yes
                                                                SPACE
         TM    PERCODE,X'01'            CR9_IFNUL event?
         BZ    NOTIFNUL                 No, turn it back on
         BAL   R14,ITRACE               Trace fetched instruction
         MVC   PERCTL+4(4),=A(CR9_IF+CR9_SUPPRESS+CR9_TEND)
         LCTLG R9,R11,PERCTL            Turn off Nullify
         B     PGMRET                   Go EXECUTE this instruction
                                                                SPACE
NOTIFNUL MVC   PERCTL+4(4),=A(CR9_IF+CR9_IFNUL+CR9_SUPPRESS+CR9_TEND)
         LCTLG R9,R11,PERCTL            Turn Nullify back on again
         B     PGMRET                   Go TRACE next instruction
                                                                SPACE
PGMTBEG  TM    PERCODE,X'01'            CR9_IFNUL event?
         BO    PGMTBEG2                 Yes, expected
         MVI   BADPSW+16-1,X'99'        NO!? UNEXPECTED!!
         J     FAILURE
PGMTBEG2 BAL   R14,ITRACE               Trace the TBEGIN...
         MVC   PERCTL+4(4),=A(CR9_IF+CR9_SUPPRESS+CR9_TEND)
         LCTLG R9,R11,PERCTL            Switch to TXSUSPEND mode
         B     PGMRET                   Go execute the transaction
                                                                SPACE
PGMTEND  BAL   R14,ITRACE               Trace the TEND...
         MVC   PERCTL+4(4),=A(CR9_IF+CR9_IFNUL+CR9_SUPPRESS+CR9_TEND)
         LCTLG R9,R11,PERCTL            Switch back to NULLIFY mode
*        B     PGMRET                   Go trace next instruction
                                                                SPACE
PGMRET   LMG   R0,R15,PGMREGS           Restore caller's registers
         LPSWE PGMOPSW                  Return to caller...
                                                                SPACE
PGMREGS  DC    16D'0'                   Saved GR registers 0 - 15
                                                                EJECT
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
                                                                SPACE 5
***********************************************************************
*                      WORKING STORAGE
***********************************************************************
                                                                SPACE
CR9_IF         EQU  X'40000000'   Instruction Fetch PER event
CR9_TEND       EQU  X'02000000'   TEND Instruction PER event
CR9_IFNUL      EQU  X'01000000'   IFetch Nullification PER event
CR9_SUPPRESS   EQU  X'00400000'   TXF Event-Suppression PER event
                                                                SPACE
PERCTL   DC    AD(CR9_IF+CR9_SUPPRESS)  TEST 1 PER events
         DC    AD(BEGRANGE)             CR10 = Range begining address
         DC    AD(ENDRANGE)             CR11 = Range ending   address
                                                                SPACE
GOODPSW  DC    XL8'0002000180000000'
         DC    XL4'00000000',A(X'00000000')
                                                                SPACE
BADPSW   DC    XL8'0002000180000000'
         DC    XL4'0000DEAD',A(X'000000FF')  (FF = Reason for Failure)
                                                                SPACE
ENPER    DC    B'01000000'              Enable PER bit in PSW
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
ZAFACNUM EQU   2                  z/Arch mode
ZAFACBYT EQU   X'00'
ZAFACBIT EQU   X'20'
ZAFAIL   MVI   BADPSW+16-1,1
         J     FAILURE
                                                                SPACE
PAFACNUM EQU   49                 PPA (Processor-Assist)
PAFACBYT EQU   X'06'
PAFACBIT EQU   X'40'
PAFAIL   MVI   BADPSW+16-1,2
         J     FAILURE
                                                                SPACE
CTFACNUM EQU   50                 Constrained TXF
CTFACBYT EQU   X'06'
CTFACBIT EQU   X'20'
CTFAIL   MVI   BADPSW+16-1,3
         J     FAILURE
                                                                SPACE
TXFACNUM EQU   73                 TXF
TXFACBYT EQU   X'09'
TXFACBIT EQU   X'40'
TXFAIL   MVI   BADPSW+16-1,4
         J     FAILURE
                                                                EJECT
***********************************************************************
*        Literals Pool
***********************************************************************
                                                                SPACE
         LTORG ,
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
