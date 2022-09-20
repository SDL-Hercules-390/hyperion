 TITLE 'FAC5861  --  Qucik Test of Misc. Instr. Ext. Facilities 2 & 3'
***********************************************************************
*                        FAC5861.ASM
***********************************************************************
*
*  This program performs a quick test of the instructions defined by
*  the new Miscellaneous-Instruction-Extensions Facilities 2 and 3.
*  It is NOT a comprehensive test. It does not, for example, check
*  for proper detection of overflow and/or boundary conditions, etc.
*  It simply verifies a simple execution of each instruction produces
*  the expected results. Nothing more.
*
***********************************************************************
                                                                SPACE 2
FAC5861  START 0
         USING FAC5861,R0
                                                                SPACE 4
         ORG   FAC5861+X'1A0'       z Restart New PSW
         DC    X'0000000180000000',AD(BEGIN)
                                                                SPACE 4
         ORG   FAC5861+X'1D0'       z Program New PSW
ZPGMNEW  DC    X'0000000180000000',AD(PGMCHK)
                                                                SPACE 4
***********************************************************************
*                  Start of actual program...
***********************************************************************
                                                                SPACE
         ORG   FAC5861+X'200'     So no base registers needed
                                                                SPACE
BEGIN    LA    R0,(L'FACLIST/8)-1
         STFLE FACLIST
                                                                SPACE
         TM    FACLIST+M2FACBYT,M2FACBIT
         JZ    M2FAIL
                                                                SPACE
         TM    FACLIST+M3FACBYT,M3FACBIT
         JZ    M3FAIL
                                                                EJECT
***********************************************************************
*            Miscellaneous-Instruction-Extensions Facility 2
***********************************************************************
*
**            Add Halfword
*
         MVI   FAILPSW+16-1,X'01'   Test number...
         LG    R0,=XL8'0000000000000001'
         AGH   R0,=H'123'
         BRC   B'1101',FAILURE              not cc2 > 0, no oflow
         CG    R0,=XL8'000000000000007C'
         JNE   FAILURE
*
         LG    R0,=XL8'FFFFFFFFFFFFFFFF'
         AGH   R0,=H'-123'
         BRC   B'1011',FAILURE              not cc1 < 0, no oflow
         CG    R0,=XL8'FFFFFFFFFFFFFF84'
         JNE   FAILURE
*
         LG    R0,=XL8'FFFFFFFFFFFFFFFF'
         AGH   R0,=H'1'
         BRC   B'0111',FAILURE              not cc0 = 0, no oflow
         CG    R0,=XL8'0000000000000000'
         JNE   FAILURE
                                                                SPACE 2
*
**          Subtract Halfword
*
         MVI   FAILPSW+16-1,X'02'   Test number...
         LG    R0,=XL8'0000000000000001'
         SGH   R0,=H'123'
         BRC   B'1011',FAILURE              not cc1 < 0, no oflow
         CG    R0,=XL8'FFFFFFFFFFFFFF86'
         JNE   FAILURE
*
         LG    R0,=XL8'FFFFFFFFFFFFFFFF'
         SGH   R0,=H'-123'
         BRC   B'1101',FAILURE              not cc2 > 0, no oflow
         CG    R0,=XL8'000000000000007A'
         JNE   FAILURE
*
         LG    R0,=XL8'FFFFFFFFFFFFFFFF'
         SGH   R0,=H'-1'
         BRC   B'0111',FAILURE              not cc0 = 0, no oflow
         CG    R0,=XL8'0000000000000000'
         JNE   FAILURE
                                                                EJECT
*
**          Multiply Halfword
*
         MVI   FAILPSW+16-1,X'03'   Test number...
         LG    R0,=XL8'FFFFFFFFFFFFFFFF'
         MGH   R0,=H'123'
         CG    R0,=XL8'FFFFFFFFFFFFFF85'
         JNE   FAILURE
                                                                SPACE 2
*
**          Multiply
*
         MVI   FAILPSW+16-1,X'04'   Test number...
         LG    R2,=XL8'000000000000007B'
         MGRK  R0,R2,R2
         LTGR  R0,R0
         JNZ   FAILURE
         CG    R1,=XL8'0000000000003B19'
         JNE   FAILURE
*
         MVI   FAILPSW+16-1,X'05'   Test number...
         LG    R1,=XL8'000000000000007B'
         MG    R0,=XL8'000000000000007B'
         LTGR  R0,R0
         JNZ   FAILURE
         CG    R1,=XL8'0000000000003B19'
         JNE   FAILURE
                                                                EJECT
*
**          Multiply Single
*
         MVI   FAILPSW+16-1,X'06'   Test number...
         L     R1,=XL4'7FFFFFFF'
         L     R2,=F'34800'
         MSRKC R0,R1,R2
         JNO   FAILURE
         C     R0,=XL4'FFFF7810'
         JNE   FAILURE
*
         MVI   FAILPSW+16-1,X'07'   Test number...
         L     R0,=XL4'7FFFFFFF'
         MSC   R0,=F'34800'
         JNO   FAILURE
         C     R0,=XL4'FFFF7810'
         JNE   FAILURE
*
         MVI   FAILPSW+16-1,X'08'   Test number...
         LG    R1,=XL8'000000007FFFFFFF'
         LG    R2,=D'34800'
         MSGRKC R0,R1,R2
         JNP   FAILURE
         CG    R0,=XL8'000043F7FFFF7810'
         JNE   FAILURE
*
         MVI   FAILPSW+16-1,X'09'   Test number...
         LG    R0,=XL8'000000007FFFFFFF'
         MSGC  R0,=D'34800'
         JNP   FAILURE
         CG    R0,=XL8'000043F7FFFF7810'
         JNE   FAILURE
                                                                SPACE 2
*
**          Branch Indirect On Condition
*
         MVI   FAILPSW+16-1,X'10'   Test number...
         BIC   B'1000',ADDRAAAA
BBBB     J     FAILURE
AAAA     BIC   B'0111',ADDRBBBB
                                                                EJECT
***********************************************************************
*            Miscellaneous-Instruction-Extensions Facility 3
***********************************************************************
*
**          And With Complement
*
         MVI   FAILPSW+16-1,X'11'   Test number...
         L     R1,=XL4'81818181'
         L     R2,=XL4'01010101'
         NCRK  R0,R1,R2
         C     R0,=XL4'80808080'
         JNE   FAILURE
*
         MVI   FAILPSW+16-1,X'12'   Test number...
         LG    R1,=XL8'8181818181818181'
         LG    R2,=XL8'0101010101010101'
         NCGRK R0,R1,R2
         CG    R0,=XL8'8080808080808080'
         JNE   FAILURE
                                                                SPACE 2
*
**          Move Right To Left
*
         MVI   FAILPSW+16-1,X'13'   Test number...
         LA    R0,256-1
         MVCRL DST,SRC
         CLC   DST,SRC
         BNE   FAILURE
                                                                SPACE 2
*
**          Nand
*
         MVI   FAILPSW+16-1,X'14'   Test number...
         L     R1,=XL4'81818181'
         L     R2,=XL4'F0F0F0F0'
         NNRK  R0,R1,R2
         C     R0,=XL4'7F7F7F7F'
         JNE   FAILURE
*
         MVI   FAILPSW+16-1,X'15'   Test number...
         LG    R1,=XL8'8181818181818181'
         LG    R2,=XL8'F0F0F0F0F0F0F0F0'
         NNGRK R0,R1,R2
         CG    R0,=XL8'7F7F7F7F7F7F7F7F'
         JNE   FAILURE
                                                                EJECT
*
**          Nor
*
         MVI   FAILPSW+16-1,X'16'   Test number...
         L     R1,=XL4'80808080'
         L     R2,=XL4'08080808'
         NORK  R0,R1,R2
         C     R0,=XL4'77777777'
         JNE   FAILURE
*
         MVI   FAILPSW+16-1,X'17'   Test number...
         LG    R1,=XL8'8080808080808080'
         LG    R2,=XL8'0808080808080808'
         NOGRK R0,R1,R2
         CG    R0,=XL8'7777777777777777'
         JNE   FAILURE
                                                                SPACE 2
*
**          Not Exclusive Or
*
         MVI   FAILPSW+16-1,X'18'   Test number...
         L     R1,=XL4'81818181'
         L     R2,=XL4'01010101'
         NXRK  R0,R1,R2
         C     R0,=XL4'7F7F7F7F'
         JNE   FAILURE
*
         MVI   FAILPSW+16-1,X'19'   Test number...
         LG    R1,=XL8'8181818181818181'
         LG    R2,=XL8'0101010101010101'
         NXGRK R0,R1,R2
         CG    R0,=XL8'7F7F7F7F7F7F7F7F'
         JNE   FAILURE
                                                                SPACE 2
*
**          Or With Complement
*
         MVI   FAILPSW+16-1,X'20'   Test number...
         L     R1,=XL4'80808080'
         L     R2,=XL4'FEFEFEFE'
         OCRK  R0,R1,R2
         C     R0,=XL4'81818181'
         JNE   FAILURE
*
         MVI   FAILPSW+16-1,X'21'   Test number...
         LG    R1,=XL8'8080808080808080'
         LG    R2,=XL8'FEFEFEFEFEFEFEFE'
         OCGRK R0,R1,R2
         CG    R0,=XL8'8181818181818181'
         JNE   FAILURE
                                                                EJECT
*
**          Select
*
         MVI   FAILPSW+16-1,X'22'   Test number...
         LG    R0,=XL8'1122334455667788'
         LG    R1,=XL8'AAAAAAAA11111111'
         LG    R2,=XL8'BBBBBBBB22222222'
         SELR  R0,R1,R2,B'1000'
         CG    R0,=XL8'1122334411111111'
         BNE   FAILURE
*
         MVI   FAILPSW+16-1,X'23'   Test number...
         LG    R0,=XL8'1122334455667788'
         LG    R1,=XL8'AAAAAAAA11111111'
         LG    R2,=XL8'BBBBBBBB22222222'
         SELGR R0,R1,R2,B'1000'
         CG    R0,=XL8'AAAAAAAA11111111'
         BNE   FAILURE
                                                                SPACE 2
*
**          Select High
*
         MVI   FAILPSW+16-1,X'24'   Test number...
         LG    R0,=XL8'1122334455667788'
         LG    R1,=XL8'AAAAAAAA11111111'
         LG    R2,=XL8'BBBBBBBB22222222'
         SELFHR R0,R1,R2,B'1000'
         CG    R0,=XL8'AAAAAAAA55667788'
         BNE   FAILURE
                                                                SPACE 2
*
**          Population Count
*
         MVI   FAILPSW+16-1,X'25'   Test number...
         LG    R1,=XL8'0102040810204080'
         POPCNT R0,R1,B'1000'
         CG    R0,=D'8'
         BNE   FAILURE
                                                                EJECT
***********************************************************************
*                         SUCCESS!
***********************************************************************
                                                                SPACE
SUCCESS  LPSWE    GOODPSW         Load test completed successfully PSW
                                                                SPACE 5
***********************************************************************
*                         FAILURE!
***********************************************************************
                                                                SPACE
FAILURE  LPSWE    FAILPSW         Load the test FAILED somewhere!! PSW
                                                                SPACE 5
***********************************************************************
*                       PROGRAM CHECK!
***********************************************************************
                                                                SPACE
PGMCHK   MVC    PROGPSW+16-1(1),FAILPSW+16-1  Copy test number to PSW
         LPSWE  PROGPSW                       UNEXPECTED PROGRAM CHECK
                                                                EJECT
***********************************************************************
*                      WORKING STORAGE
***********************************************************************
                                                                SPACE
         ORG   FAC5861+X'FF9'   (odd alignment + page cross)
                                                                SPACE
SRC      DS    0CL256           MVCRL instruction source operand
         DC    256C'A'
                                                                SPACE
         ORG   FAC5861+X'CC1'   (odd alignment)
                                                                SPACE
DST      DS    0CL256           MVCRL instruction destination operand
         DC    256C'B'
                                                                SPACE
         ORG   FAC5861+X'800'   (primary working storage)
                                                                SPACE
FACLIST  DC    XL256'00'        Facility List
                                                                SPACE
M2FACNUM EQU   58       Miscellaneous-Instruction-Extensions Facility 2
M2FACBYT EQU   X'07'
M2FACBIT EQU   X'20'
M2FAIL   MVI   FAILPSW+16-1,X'58'
         J     FAILURE
                                                                SPACE
M3FACNUM EQU   61       Miscellaneous-Instruction-Extensions Facility 3
M3FACBYT EQU   X'07'
M3FACBIT EQU   X'04'
M3FAIL   MVI   FAILPSW+16-1,X'61'
         J     FAILURE
                                                                SPACE
ADDRAAAA DC    AD(AAAA)         BIC instruction branch address
ADDRBBBB DC    AD(BBBB)         BIC instruction branch address
                                                                SPACE
         DC    0D'0'            (alignment)
                                                                SPACE
GOODPSW  DC    XL8'0002000180000000'
         DC    XL4'00000000',A(X'00000000')
                                                                SPACE
FAILPSW   DC    XL8'0002000180000000'
         DC    XL4'00000BAD',A(X'000000FF') FF = code where we failed
                                                                SPACE
PROGPSW  DC    XL8'0002000180000000'
         DC    XL4'0000DEAD',A(X'000000FF') FF = code where we failed
                                                                EJECT
***********************************************************************
*        Literals Pool
***********************************************************************
                                                                SPACE
         LTORG ,
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
