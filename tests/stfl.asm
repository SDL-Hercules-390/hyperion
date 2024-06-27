 TITLE   'Store facilities list [extended].'
* This file was put into the public domain 2016-08-20
* by John P. Hartmann.  You can use it for anything you like,
* as long as this notice remains.
*
* MINOR COSMETIC CLEAN UP by Fish
                                                                SPACE
STFL     START 0
         USING *,R15
                                                                SPACE
         ORG   STFL+X'C8'
PRIVFL   DS    F                      Stored in the PSA by STFL
                                                                SPACE 2
         ORG   STFL+X'1A0'            Restart new
         DC    AD(0,GO)
                                                                SPACE
         ORG   STFL+X'1D0'            Program new
         DC    X'00020000',A(0,0,0)   SUCCESS PSW
                                                                SPACE 3
         ORG   STFL+X'200'
SEFL     DC    2D'0'
EFL      DC    16D'0'                 Should last a while
                                                                SPACE 3
GO       EQU   *
         STFL  0                32 bits at 200 decimal = X'C8'
                                                                SPACE
         XGR   R0,R0            Short
         STFLE SEFL
         IPM   R2               Should be 3
         LGR   R3,R0            Should be 1
                                                                SPACE
         LHI   R0,7             Store up to 8
         STFLE EFL
         IPM   R4
         LGR   R5,R0
                                                                SPACE
         STFLE 1                Should specification due to unalignment
                                                                SPACE
         LTR   R14,R14
         BNZR  R14
                                                                SPACE
         LPSWE FAIL
                                                                SPACE
FAIL     DC    0D'0',X'00020000',2A(0),A(X'DEADBEEF')
                                                                EJECT
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
                                                                SPACE
         END
