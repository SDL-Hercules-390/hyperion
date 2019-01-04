 TITLE 'Perform floating point operation. ESA and z mode.'
***********************************************************************
*                           PFPO.ASM
***********************************************************************
*
*   This file was put into the public domain 2016-05-05
*   by John P. Hartmann. You can use it for anything you like,
*   as long as this notice remains.
*
*   Note that this test runs in problem state. As a result STFL is
*   not a good idea; nor is LPSW for that matter.
*
*   So we terminate by SVC, and we determine architecture mode
*   by seeing where the Restart Old PSW was stored.
*
***********************************************************************
*
*   Temporarily modified by Fish to do NOTHING and test NOTHING!
*   Temporarily modified by Fish to do NOTHING and test NOTHING!
*   Temporarily modified by Fish to do NOTHING and test NOTHING!
*
***********************************************************************
                                                                SPACE 5
***********************************************************************
*                       Low Core PSWs...
***********************************************************************
                                                                SPACE 2
PFPO     START 0
         USING PFPO,R15
                                                                SPACE
         DC    A(X'00090000',GO)    ESA Restart New PSW
ROPSW390 DS    0XL8                 ESA/390 Restart Old PSW
         DC    X'FFFFFFFFFFFFFFFF'  (if not ones then ESA/390 mode)
                                                                SPACE
         ORG   PFPO+X'60'           ESA SVC New PSW
         DC    X'000A0000',A(X'0')  (normal EOJ PSW)
                                                                SPACE
         ORG   PFPO+X'68'           ESA Program New PSW
         DC    X'000A0000',A(X'DEADDEAD')
                                                                SPACE
         ORG   PFPO+X'120'
ROPSWZ   DS    0XL16                z Restart Old PSW
         DC    16X'FF'              (if not ones then z mode)
                                                                SPACE
         ORG   PFPO+X'1A0'          z Restart New PSW
         DC    X'0001000180000000',AD(GO)
                                                                SPACE
         ORG   PFPO+X'1C0'          z SVC New PSW
         DC    X'0002000180000000',AD(0)
                                                                SPACE
         ORG   PFPO+X'1D0'          z Program New PSW
         DC    X'0002000180000000',AD(X'DEADDEAD')
                                                                EJECT
***********************************************************************
*                  Start of actual program...
***********************************************************************
                                                                SPACE 2
         ORG   PFPO+X'200'
GO       DS    0H
                                                                SPACE
         CLI   ROPSW390,X'FF'       Running in ESA/390 mode?
         BNE   EOJ                  Yes, quick EOJ (no PFPO support!)







***********************************************************************
***********************************************************************
**
**                       PROGRAMMING NOTE
**
**    Until such time as we can code a PROPER test for this
**    instruction, this test has been purposely neutered.
**
**    It currently does NOTHING and tests NOTHING.
**
**
***********************************************************************
***********************************************************************

         B     EOJ                  *** (See PROGRAMMING NOTE) ***






         SLR   R3,R3                Clear R3 for later CC store
         L     R0,=A(X'80000000')   Test for invalid function
                                                                SPACE
         PFPO  ,                    Perform Floating Point Operation
                                                                SPACE
         IPM   R3                   Collect condition code from PSW
         ST    R3,CONDCODE          Save CC
                                                                SPACE
EOJ      DS    0H
         SVC   0                    Load hardwait psw
                                                                SPACE 3
         LTORG ,
                                                                SPACE 3
         ORG   PFPO+X'300'          Condition Code save area @ X'300'
CONDCODE DC    F'0'                 Condition Code from PFPO
                                                                EJECT
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
