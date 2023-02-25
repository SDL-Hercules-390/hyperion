 TITLE '                  VERY Simple Basic HFP Floating Point Tests'
***********************************************************************
*                            FLOAT
***********************************************************************
*
*   This program performs a few EXTREMELY simple Floating Point
*   tests based on the instruction-use examples in Appendix A.
*   "Number Representation and Instruction-Use Examples" of the
*   SA22-7200-00 "ESA/370 Principles of Operation" manual.
*
*   Unlike other S/370 tests, this test runs in BC mode not EC mode
*   for no other reason than that's the mode that MVT runs in, and
*   at the time, there was some concern as to whether floating point
*   instructions were being executed properly (GitHub Issue #546).
*
*   UPDATE: This test has now been updated to also support running
*   in z/Arcitecture mode as well, since z/Arcitecture also supports
*   HFP instructions.
*
***********************************************************************
                                                                SPACE 2
***********************************************************************
*                          LOW CORE
***********************************************************************
                                                                SPACE 2
TEST     START 0
         USING TEST,0                 Use absolute addressing
                                                                SPACE 2
         PRINT DATA
                                                                SPACE
         ORG   TEST+X'00'             S/370 Restart new PSW
         DC    XL4'00000000',A(BEGIN)
                                                                SPACE 2
         ORG   TEST+X'68'             S/370 Program new PSW
         DC    XL4'00020000',A(X'DEAD')
                                                                SPACE 2
         ORG   TEST+X'1A0'            z Restart New PSW
         DC    0D'0',X'0000000180000000',AD(BEGINZ)
                                                                SPACE 2
         ORG   TEST+X'1D0'            z Program New PSW
         DC    0D'0',X'0002000180000000',AD(X'DEAD')
                                                                SPACE
         PRINT NODATA
                                                                EJECT
***********************************************************************
*                          MAINLINE
***********************************************************************
                                                                SPACE
         ORG   TEST+X'200'            Start of test program
                                                                SPACE 3
BEGIN    MVI   RUNMODE,MODE370        370 mode
         BAL   R15,DOTESTS
         B     SUCCESS
                                                                SPACE 3
BEGINZ   MVI   RUNMODE,ZMODE          z/Architecture mode
         BAL   R15,DOTESTS
         B     SUCCESS
                                                                SPACE 4
DOTESTS  DS    0H                     Perform all tests...
                                                                SPACE
         BAL   R14,TEST1
         BAL   R14,TEST2
         BAL   R14,TEST3
         BAL   R14,TEST4
         BAL   R14,TEST5
         BAL   R14,TEST6
                                                                SPACE 2
         MVI   TESTNUM,0              No test has failed
         MVI   SUBTEST,0              No sub-test has failed either
                                                                SPACE
         BR    R15                    Return to caller
                                                                EJECT
***********************************************************************
*                         END OF JOB
***********************************************************************
                                                                SPACE 2
SUCCESS  CLI   RUNMODE,MODE370
         BNE   ZSUCCESS
         LPSW  OKPSW
                                                                SPACE 2
BADCC    CLI   RUNMODE,MODE370
         BNE   ZBADCC
         LPSW  CCPSW
                                                                SPACE 2
BADGOT   CLI   RUNMODE,MODE370
         BNE   ZBADGOT
         LPSW  GOTPSW
                                                                SPACE 3
ZSUCCESS LPSWE ZOKPSW
ZBADCC   LPSWE ZCCPSW
ZBADGOT  LPSWE ZGOTPSW
                                                                SPACE 6
         PRINT DATA
                                                                SPACE
OKPSW    DC    0D'0',XL4'00020000',A(0)
CCPSW    DC    0D'0',XL4'00020000',A(X'BADCC')
GOTPSW   DC    0D'0',XL4'00020000',A(X'BADBAD')
                                                                SPACE 3
ZOKPSW   DC    0D'0',XL8'0002000180000000',AD(0)
ZCCPSW   DC    0D'0',XL8'0002000180000000',AD(X'BADCC')
ZGOTPSW  DC    0D'0',XL8'0002000180000000',AD(X'BADBAD')
                                                                SPACE
         PRINT NODATA
                                                                EJECT
***********************************************************************
*                TEST 1:  AE/AD  (Add Normalized)
***********************************************************************
                                                                SPACE
TEST1    MVI   TESTNUM,X'F1'
         MVI   SUBTEST,0
*
*  Add Normalized (AD, ADR, AE, AER, AXR)
*
*    FPR6 contains
*    C3 08 21 00 00 00 00 00
*
*    Storage location contains
*    41 12 34 56 00 00 00 00
*
*
*    Machine Format
*
*      Op Code R1 X2 B2 D2
*        7A     6  0  D 000
*
*
*    Assembler Format
*
*      Op Code R1,D2(X2,B2)
*        AE    6,0(0,13)
*
*
*  the result (left half of FPR6) is
*  C2 80 EC BB.
*
*  The right half of FPR6 is unchanged.
*
*  Condition code 1 is set (result less than zero).
*
*  If the long-precision instruction 'AD' were used,
*  the result in FPR6 would be
*  C2 80 BC BA A0 00 00 00.
*
         LD    FPR6,T1_FPR6
         AE    FPR6,T1_STRG
         BC    B'1011',BADCC  (not CC1)
         STD   FPR6,T1_GOT
         CLC   T1_GOT,T1_WANT
         BNE   BADGOT
         BR    R14
                                                                EJECT
***********************************************************************
*                 TEST 2:  AU  (Add Unnormalized)
***********************************************************************
                                                                SPACE
TEST2    MVI   TESTNUM,X'F2'
         MVI   SUBTEST,0
*
*  Add Unnormalized (AU, AUR, AW, AWR)
*
*  using the the same operands as in the
*  previous ADD NORMALIZED example:
*
*  FPR6 contains
*  C3 08 21 00 00 00 00 00
*
*  Storage location contains
*  41 12 34 56 00 00 00 00
*
*
*    Machine Format
*
*      Op Code R1 X2 B2 D2
*        7E     6  0  D 0000
*
*
*    Assembler Format
*
*      Op Code R1,D2(X2,B2)
*        AU    6,0(0,13)
*
*
*  result in FPR6
*  C3 08 0E CB 00 00 00 00
*
*  Condition code 1 is set (result less than zero).
*
         LD    FPR6,T2_FPR6
         AU    FPR6,T2_STRG
         BC    B'1011',BADCC  (not CC1)
         STD   FPR6,T2_GOT
         CLC   T2_GOT,T2_WANT
         BNE   BADGOT
         BR    R14
                                                                EJECT
***********************************************************************
*                  TEST 3:  CDR  (Compare)
***********************************************************************
                                                                SPACE
TEST3    MVI   TESTNUM,X'F3'
         MVI   SUBTEST,0
*
*  Compare (CD, CDR, CE, CER)
*
*  FPR4 contains
*  43 00 00 00 00 00 00 00 (zero)
*
*  FPR6 contains
*  35 12 34 56 78 9A BC DE (positive number).
*
*
*    Machine Format
*
*      Op Code R1 R2
*        29     4  6
*
*
*    Assembler Format
*
*      Op Code R1,R2
*       CDR     4,6
*
*
*  Condition code 1 is set (FPR4 less than FPR6).
*
*  If FPR6 instead contained
*  34 12 34 56 78 9A BC DE
*
*  Condition code 0 (equal) would instead be set.
*
*  As another example
*  41 00 12 34 56 78 9A BC
*
*  compares equal to all numbers of the form:
*  3F 12 34 56 78 9A BC 0X
*
*  where X represents any hexadecimal digit.
*
         LD    FPR4,T3_FPR4
         LD    FPR6,T3_FPR6
         CDR   FPR4,FPR6
         BC    B'1011',BADCC  (not CC1)
         LD    FPR6,T3_FPR6A
         CDR   FPR4,FPR6
         BC    B'0111',BADCC  (not CC0)
         LD    FPR4,T3_FPR4A
         LA    R1,T3_FPR6X
         LA    R2,T3_NUMX
T3_XLOOP LD    FPR6,0(,R1)
         CDR   FPR4,FPR6
         BC    B'0111',BADCC  (not CC0)
         LA    R1,8(,R1)
         BCT   R2,T3_XLOOP
         BR    R14
                                                                EJECT
***********************************************************************
*                   TEST 4:  DER  (Divide)
***********************************************************************
                                                                SPACE
TEST4    MVI   TESTNUM,X'F4'
         MVI   SUBTEST,0
*
*  Divide (DD, DDR, DE, DER)
*
*  first operand  = dividend
*  second operand = divisor
*  resulting quotient = replaces first operand
*
*
*    Machine Format
*
*      Op Code R1 R2
*        3D     2  0
*
*
*    Assembler Format
*
*      Op Code R1,R2
*       DER     2,0
*
*
*         FPR2 Before  FPR0          FPR2 After
*  Case   (Dividend)   (Divisor)     (Quotient)
*
*   A     -43 082100   +43 001234    -42 72522F
*   B     +42 101010   +45 111111    +3D F0F0F0
*   C     +48 30000F   +41 400000    +47 C0003C
*   D     +48 30000F   +41 200000    +48 180007
*   E     +48 180007   +41 200000    +47 C00038
*
         LA    R1,T4_A
         LA    R2,T4_NUMT
T4_LOOP  LE    FPR2,0(,R1)
         LE    FPR0,4(,R1)
         DER   FPR2,FPR0
         STE   FPR2,T4_GOT
         CLC   T4_GOT,8(R1)
         BNE   BADGOT
         LA    R1,3*4(,R1)
         BCT   R2,T4_LOOP
         BR    R14
                                                                EJECT
***********************************************************************
*                   TEST 5:  HDR  (Halve)
***********************************************************************
                                                                SPACE
TEST5    MVI   TESTNUM,X'F5'
         MVI   SUBTEST,0

*
*  Halve (HDR, HER)
*
*  HALVE produces the same result as floating-point
*  DIVIDE with a divisor of 2.0.
*
*  FPR2 contains
*  + 48 30 00 00 00 00 00 0F
*
*  FPR2 result
*  + 48 18 00 00 00 00 00 07
*
*
*    Machine Format
*
*      Op Code R1 R2
*        24     2  2
*
*    Assembler Format
*
*      Op Code R1,R2
*       HDR     2,2
*
         LD    FPR2,T5_FPR2
         HDR   FPR2,FPR2
         STD   FPR2,T5_GOT
         CLC   T5_GOT,T5_WANT
         BNE   BADGOT
         BR    R14
                                                                EJECT
***********************************************************************
*                   TEST 6:  MDR  (Multiply)
***********************************************************************
                                                                SPACE
TEST6    MVI   TESTNUM,X'F6'
         MVI   SUBTEST,0
*
*  Multiply (MD, MDR, ME, MER, MXD, MXDR, MXR)
*
*  FPR0: -33 606060 60606060
*  FPR2: -5A 200000 20000020
*
*
*    Machine Format
*
*      Op Code R1 R2
*        2C     0  2
*
*    Assembler Format
*
*      Op Code R1,R2
*        MDR    0,2
*
*
*  Final FPR0 result:
*  +4C C0C0C1 81818241
*
         LD    FPR0,T6_FPR0
         LD    FPR2,T6_FPR2
         MDR   FPR0,FPR2
         STD   FPR0,T6_GOT
         CLC   T6_GOT,T6_WANT
         BNE   BADGOT
         BR    R14
                                                                EJECT
***********************************************************************
*                      Working storage
***********************************************************************
                                                                SPACE
         LTORG ,                        Literals Pool
                                                                SPACE 2
RUNMODE  DC    C' '                     Run mode
MODE370  EQU   C'3'                     370 run mode
ZMODE    EQU   C'Z'                     z/Architecture run mode
                                                                SPACE 2
         DC    0D'0'
T1_FPR6  DC    XL8'C3 08 21 00 00 00 00 00'
T1_STRG  DC    XL8'41 12 34 56 00 00 00 00'
T1_GOT   DC    XL8'00'
T1_WANT  DC    XL8'C2 80 EC BB 00 00 00 00'
                                                                SPACE 2
T2_FPR6  DC    XL8'C3 08 21 00 00 00 00 00'
T2_STRG  DC    XL8'41 12 34 56 00 00 00 00'
T2_GOT   DC    XL8'00'
T2_WANT  DC    XL8'C3 08 0E CB 00 00 00 00'
                                                                SPACE 2
T3_FPR4  DC    XL8'43 00 00 00 00 00 00 00'
T3_FPR6  DC    XL8'35 12 34 56 78 9A BC DE'
T3_FPR6A DC    XL8'34 12 34 56 78 9A BC DE'
T3_FPR4A DC    XL8'41 00 12 34 56 78 9A BC'
T3_FPR6X DC    XL8'3F 12 34 56 78 9A BC 00'
         DC    XL8'3F 12 34 56 78 9A BC 01'
         DC    XL8'3F 12 34 56 78 9A BC 02'
         DC    XL8'3F 12 34 56 78 9A BC 03'
         DC    XL8'3F 12 34 56 78 9A BC 04'
         DC    XL8'3F 12 34 56 78 9A BC 05'
         DC    XL8'3F 12 34 56 78 9A BC 06'
         DC    XL8'3F 12 34 56 78 9A BC 07'
         DC    XL8'3F 12 34 56 78 9A BC 08'
         DC    XL8'3F 12 34 56 78 9A BC 09'
         DC    XL8'3F 12 34 56 78 9A BC 0A'
         DC    XL8'3F 12 34 56 78 9A BC 0B'
         DC    XL8'3F 12 34 56 78 9A BC 0C'
         DC    XL8'3F 12 34 56 78 9A BC 0D'
         DC    XL8'3F 12 34 56 78 9A BC 0E'
         DC    XL8'3F 12 34 56 78 9A BC 0F'
T3_NUMX  EQU   (*-T3_FPR6X)/8
                                                                EJECT
         PRINT DATA
                                                                SPACE
T4_A     DC    XL4'C3 082100',XL4'43 001234',XL4'C2 72522F'
T4_B     DC    XL4'42 101010',XL4'45 111111',XL4'3D F0F0F0'
T4_C     DC    XL4'48 30000F',XL4'41 400000',XL4'47 C0003C'
T4_D     DC    XL4'48 30000F',XL4'41 200000',XL4'48 180007'
T4_E     DC    XL4'48 180007',XL4'41 200000',XL4'47 C00038'
                                                                SPACE
T4_NUMT  EQU   (*-T4_A)/(3*4)
T4_GOT   DC    XL4'00'
                                                                SPACE
         PRINT NODATA
                                                                SPACE 3
         DC    0D'0'            (alignment)
                                                                SPACE 4
T5_FPR2  DC    XL8'48 30 00 00 00 00 00 0F'
T5_GOT   DC    XL8'00'
T5_WANT  DC    XL8'48 18 00 00 00 00 00 07'
                                                                SPACE 2
T6_FPR0  DC    XL8'B3 606060 60606060'
T6_FPR2  DC    XL8'DA 200000 20000020'
T6_GOT   DC    XL8'00'
T6_WANT  DC    XL8'4C C0C0C1 81818241'
                                                                SPACE 3
***********************************************************************
*                      Test Flags
***********************************************************************
                                                                SPACE
         ORG   TEST+X'600'              Test flags
                                                                SPACE
TESTNUM  DC    X'00'                    Test number that failed
SUBTEST  DC    X'00'                    Sub-test number that failed
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
                                                                SPACE
FPR0     EQU   0
FPR2     EQU   2
FPR4     EQU   4
FPR6     EQU   6
                                                                SPACE 2
         END   TEST
