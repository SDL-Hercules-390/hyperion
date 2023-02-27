 TITLE '                  370 BC mode PSW ILC Handling'
***********************************************************************
*                            BC ILC
***********************************************************************
*
*   This program verifies proper handling of the 370 BC mode PSW
*   ILC field. The ILC field in a 370 BC mode PSW is in the high-
*   order 2 bits of the second word of the PSW (bits 32 and 33).
*   An ILC value of 00 (binary) indicates an ILC of 0 (zero). An
*   ILC value of 01 indicates an ILC of 2 (two). An ILC value of
*   10 indicates an ILC of 4 (four). An ILC value of 11 indicates
*   an ILC of 6 (six).
*
*   The technique used is to force a program check interruption
*   on instructions of different lengths (a 2 byte instruction,
*   a 4 byte instruction and a 6 byte instruction), copying the
*   high-order byte of the second word of the resulting Program
*   Old PSW from the Program Check, and then comparing it with
*   our expected value. For ILC 0, we use a LPSW of an invalid
*   EC mode PSW, thus causing an early Specification exception.
*   For ILCs 2, 4 and 6 we use a CLCL, CLI and MVC instruction
*   with a base register value causing an Addressing Exception.
*
***********************************************************************
                                                                SPACE 3
***********************************************************************
*                          LOW CORE
***********************************************************************
                                                                SPACE 2
TEST     START 0
         USING TEST,0                 Use absolute addressing
                                                                SPACE 2
         ORG   TEST+X'00'             S/370 Restart New PSW
         DC    XL4'00000000',A(BEGIN)
                                                                SPACE 2
         ORG   TEST+X'28'             S/370 Program Old PSW
PGMOLD   DC    XL4'00000000',A(0)
                                                                SPACE 2
         ORG   TEST+X'68'             S/370 Program New PSW
PGMNEW   DC    XL4'00000000',A(0)
                                                                SPACE 2
         ORG   TEST+X'90'             S/370 TEA (*not* 390/zArch DXC!)
TEA_DXC  DC    XL4'00000000'
                                                                EJECT
***********************************************************************
*                          MAINLINE
***********************************************************************
                                                                SPACE
         ORG   TEST+X'200'            Start of test program
                                                                SPACE
BEGIN    BAL   R14,ILC0TEST
         BAL   R14,ILC2TEST
         BAL   R14,ILC4TEST
         BAL   R14,ILC6TEST
                                                                SPACE
         LPSW  GOODPSW
FAIL     LPSW  FAILPSW
                                                                SPACE 2
GOODPSW  DC    0D'0',XL4'00020000',A(0)
FAILPSW  DC    0D'0',XL4'00020000',A(X'BAD')
                                                                EJECT
***********************************************************************
*                       ILC 0
***********************************************************************
                                                                SPACE
ILC0TEST LA    R0,ILC0CONT            R0 --> continue
         STCM  R0,B'0111',PGMNEW+4+1  Program New --> continue
                                                                SPACE
         LPSW  BADECPSW               Specification Exception!
                                                                SPACE
ILC0CONT MVC   ILC0ACT,PGMOLD+4       Save Program Old ILC byte
         NI    ILC0ACT,B'11000000'    Discard unwanted bits
         CLC   ILC0ACT,ILC0EXP        Actual = Expected?
         BER   R14                    Yes, return
         LPSW  FAILPSW                No?! FAIL!
                                                                EJECT
***********************************************************************
*                       ILC 2
***********************************************************************
                                                                SPACE
ILC2TEST LA    R0,ILC2CONT            R0 --> continue
         STCM  R0,B'0111',PGMNEW+4+1  Program New --> continue
         L     R12,MAXADDR            R12 <== X'00FFFFFF'
         LR    R13,R12                R13 <== X'00FFFFFF'
                                                                SPACE
         CLCL  R12,R12                Addressing Exception!
                                                                SPACE
ILC2CONT MVC   ILC2ACT,PGMOLD+4       Save Program Old ILC byte
         NI    ILC2ACT,B'11000000'    Discard unwanted bits
         CLC   ILC2ACT,ILC2EXP        Actual = Expected?
         BER   R14                    Yes, return
         LPSW  FAILPSW                No?! FAIL!
                                                                EJECT
***********************************************************************
*                       ILC 4
***********************************************************************
                                                                SPACE
ILC4TEST LA    R0,ILC4CONT            R0 --> continue
         STCM  R0,B'0111',PGMNEW+4+1  Program New --> continue
         L     R12,MAXADDR            R12 <== X'00FFFFFF'
                                                                SPACE
         CLI   0(R12),0               Addressing Exception!
                                                                SPACE
ILC4CONT MVC   ILC4ACT,PGMOLD+4       Save Program Old ILC byte
         NI    ILC4ACT,B'11000000'    Discard unwanted bits
         CLC   ILC4ACT,ILC4EXP        Actual = Expected?
         BER   R14                    Yes, return
         LPSW  FAILPSW                No?! FAIL!
                                                                EJECT
***********************************************************************
*                       ILC 6
***********************************************************************
                                                                SPACE
ILC6TEST LA    R0,ILC6CONT            R0 --> continue
         STCM  R0,B'0111',PGMNEW+4+1  Program New --> continue
         L     R12,MAXADDR            R12 <== X'00FFFFFF'
                                                                SPACE
         CP    BADPACK,BADPACK        Data Exception!
                                                                SPACE
ILC6CONT MVC   ILC6ACT,PGMOLD+4       Save Program Old ILC byte
         NI    ILC6ACT,B'11000000'    Discard unwanted bits
         CLC   ILC6ACT,ILC6EXP        Actual = Expected?
         BNE   FAIL                   No?! FAIL!
         CL    R7,TEA_DXC             TEA should still be zero
         BER   R14                    Yes, return
         LPSW  FAILPSW                No?! FAIL!
                                                                EJECT
***********************************************************************
*                      Working storage
***********************************************************************
                                                                SPACE 3
*        Invalid EC mode PSW...  (bits 24-31 s/b zero but they're not!)
                                                                SPACE
BADECPSW DC    0D'0',XL4'000800FF',XL4'00FFFFFF'
                                                                SPACE 3
*        Invalid storage address...
                                                                SPACE
MAXADDR  EQU   BADECPSW+4,4
                                                                SPACE 3
*        Invalid packed data...
                                                                SPACE
BADPACK  EQU   MAXADDR+1,3
                                                                SPACE 3
*        Expected values...
                                                                SPACE
ILC0EXP  DC    X'00'
ILC2EXP  DC    X'40'
ILC4EXP  DC    X'80'
ILC6EXP  DC    X'C0'
                                                                SPACE 3
*        Actual values...
         ORG   TEST+X'300'
                                                                SPACE
ILC0ACT  DC    X'FF'
ILC2ACT  DC    X'FF'
ILC4ACT  DC    X'FF'
ILC6ACT  DC    X'FF'
                                                                EJECT
***********************************************************************
*                      Register equates
***********************************************************************
                                                                SPACE
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
         END   TEST
