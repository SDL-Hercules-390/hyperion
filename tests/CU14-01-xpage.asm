 TITLE '       CU14-01-xpage (Test cross page CU14 instruction)'
***********************************************************************
*
*        CU14 cross page boundary instruction tests
*
*        NOTE: This test is based the CLCL-et-al Test
*              modified to only test the CU14 instruction.
*
*        James Wekel February 2024
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*            CU14 cross page instruction tests
*
***********************************************************************
*  This program tests functioning of the CU14 instruction
*  across page boundaties. Only M3=0 is tested and CC=0 is expected.
*  Specification exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*  Example Hercules Testcase:
*
*        *Testcase CU14-01-xpage (Test cross page CU14 instruction )
*
*        # ------------------------------------------------------------
*        #  This tests only the function of the CU14 instruction where
*        #  operands cross page boundaries.
*        #  Specification Exceptions are NOT tested.
*        # ------------------------------------------------------------
*
*        mainsize    16
*        numcpu      1
*        sysclear
*        archlvl     z/Arch
*
*        loadcore    "$(testpath)/CU14-01-xpage.core" 0x0
*
*        runtest     2
*
*        *Done
*
***********************************************************************
                                                                EJECT
***********************************************************************
*
*              Low Core PSWs
*
***********************************************************************
                                                                SPACE 2
CU14TST START 0
         USING CU14TST,R0            Low core addressability
                                                                SPACE 2
         ORG   CU14TST+X'1A0'        z/Architecure RESTART PSW
         DC    X'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE 2
         ORG   CU14TST+X'1D0'        z/Architecure PROGRAM CHECK PSW
         DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                SPACE 3
         ORG   CU14TST+X'200'        Start of actual test program...
                                                                 EJECT
***********************************************************************
*               The actual "CU14TST" program itself...
***********************************************************************
*
*  Architecture Mode: z/Arch
*  Register Usage:
*
*   R0       interation count for current test
*   R1       current target address
*   R2       CU14 - First-Operand Address  - target
*   R3       CU14 - First-Operand Length
*   R4       CU14 - Second-Operand Address  - source
*   R5       CU14 - Second-Operand length
*   R6       (work)
*   R7       CU14CTL base
*   R8       First base register
*   R9       Second base register
*   R10-R13  (work) (copy source)
*   R14      Subroutine call
*   R15      Secondary Subroutine call or current source address
*
***********************************************************************
                                                                SPACE
         USING  BEGIN,R8        FIRST Base Register
         USING  BEGIN+4096,R9   SECOND Base Register
                                                                SPACE
BEGIN    BALR  R8,0             Initalize FIRST base register
         BCTR  R8,0             Initalize FIRST base register
         BCTR  R8,0             Initalize FIRST base register
                                                                SPACE
         LA    R9,2048(,R8)     Initalize SECOND base register
         LA    R9,2048(,R9)     Initalize SECOND base register
*
**       Run the tests...
*
         BAL   R14,TEST01       Test CU14   instruction
*
                                                                EJECT
***********************************************************************
*         Test for normal or unexpected test completion...
***********************************************************************
                                                                SPACE
         CLI   TESTNUM,X'01'    Did we end on expected test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         CLI   SUBTEST,X'04'    Did we end on expected SUB-test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                               SPACE
         B     EOJ              Yes, then normal completion!
                                                               SPACE 4
***********************************************************************
*        Fixed test storage locations ...
***********************************************************************
                                                               SPACE 2
         ORG   BEGIN+X'200'

TESTADDR DS    0D         Where test/subtest numbers will go
TESTNUM  DC    X'99'      Test number of active test
SUBTEST  DC    X'99'      Active test sub-test number
                                                               SPACE 2
         ORG   *+X'100'
                                                               EJECT
***********************************************************************
*        TEST01                   Test CU14 instruction
***********************************************************************
                                                                SPACE
TEST01   MVI   TESTNUM,X'01'

         LA    R7,CU14CTL          Point R7 --> testing control table
         USING CU14TEST,R7         What each table entry looks like

TST1LOOP EQU   *
         IC    R6,TNUM            Set test number
         STC   R6,TESTNUM

         L     R0,OP2LEN          source length
*
         L     R15,OP1WHERE       Calculate Target address
         SR    R15,R0
         LA    R15,1(,R15)
*
         L     R1,OP2WHERE         Calculate source address
         SR    R1,R0
         LA    R1,1(,R1)
*
**       Initialize source operand data (move data to testing address)
*
TST1INIT EQU   *
*                                   Source
         LR    R10,R1               Where to move operand-2 data to
         L     R11,OP2LEN             How much of it there is
         L     R12,OP2DATA          Where op2 data is right now
         L     R13,OP2LEN             How much of it there is
         MVCL  R10,R12

                                                               SPACE 3
*       Execute CU14 instruction and check for expected condition code
                                                               SPACE 1
         LR    R2,R15              Target
         L     R3,OP1LEN              target length
         LR    R4,R1               source
         L     R5,OP2LEN              source length

         SR    R6,R6              get M3 bits for CU14
         IC    R6,M3              (M3)
         STC   R6,CU14MOD+2       DYNAMICALLY MODIFIED CODE

         L     R11,FAILMASK       (failure CC)
         SLL   R11,4              (shift to BC instr CC position)

         MVI   SUBTEST,X'00'      (primary CU14)
CU14MOD  CU14  R2,R4              Start with CU14 and m3=0
         BC    B'0001',CU14MOD     cc=3, not finished

         EX    R11,CU14BC         fail if...
                                                               EJECT
**       Verify R3,R5  contain (or still contain!) expected values
         MVI   SUBTEST,X'01'      (R3 result - TARGET remaining len)
         C     R3,ENDLN1              R3 correct?
         BNE   CU14FAIL           No, FAILTEST!

         MVI   SUBTEST,X'02'      (R5 result - SOURCE remaining len)
         C     R5,ENDLN2              R5 correct
         BNE   CU14FAIL           No, FAILTEST!

         MVI   SUBTEST,X'03'      (TARGET IS CORRECT?)
         LR    R2,R15             conversion result
         L     R3,OP1LEN
         L     R4,OP1DATA         expected result
         L     R5,OP1LEN
         CLCL  R2,R4
         BC    B'0001',*-4          not finished?
         BNE   CU14FAIL          No, FAILTEST!
*
*        shift source/target addresses and try again to
*        ensure multiple cross page bounday tests
*
         LA    R1,1(,R1)
         LA    R15,1(,R15)
         BCT   R0,TST1INIT

         LA    R7,CU14NEXT        Go on to next table entry
         CLC   =F'0',0(R7)        End of table?
         BNE   TST1LOOP           No, loop...

         MVI   SUBTEST,X'04'       Done
         B     CU14DONE            Done! (success!)
                                                                SPACE 2
CU14FAIL LA    R14,FAILTEST       Unexpected results!
CU14DONE BR    R14                Return to caller or FAILTEST
                                                                SPACE 2
CU14BC   BC    0,CU14FAIL          (fail if unexpected condition code)
                                                                SPACE 2
         DROP  R7
         USING BEGIN,R8
                                                                EJECT
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 5
EOJPSW   DC    0D'0',X'0002000180000000',AD(0)
                                                                SPACE
EOJ      LPSWE EOJPSW               Normal completion
                                                                SPACE 5
FAILPSW  DC    0D'0',X'0002000180000000',AD(X'BAD')
                                                                SPACE
FAILTEST LPSWE FAILPSW              Abnormal termination
                                                                SPACE 7
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE 2
         LTORG ,                Literals pool
                                                                SPACE
K        EQU   1024             One KB
PAGE     EQU   (4*K)            Size of one page
K16      EQU   (16*K)           16 KB
K32      EQU   (32*K)           32 KB
K64      EQU   (64*K)           64 KB
MB       EQU   (K*K)             1 MB
                                                                EJECT
CU14TST CSECT  ,
                                                                SPACE 2
***********************************************************************
*        CU14TEST DSECT
***********************************************************************
                                                                SPACE 2
CU14TEST DSECT ,
TNUM     DC    X'00'          CU14 test number
         DC    X'00'
         DC    X'00'
M3       DC    X'00'          M3 byte stored into CU14 instruction
                                                                SPACE 3

OP1DATA  DC    A(0)           Pointer to Operand 1       - result
OP1LEN   DC    F'0'             length                   - result
OP2DATA  DC    A(0)           Pointer to Operand-2 data  - source
OP2LEN   DC    F'0'             length                   - source
                                                               SPACE 2
OPSWHERE EQU   *
OP1WHERE DC    A(0)           result - Where should be placed
OP2WHERE DC    A(0)           source - Where should be placed
                                                               SPACE 2
FAILMASK DC    A(0)           Failure Branch on Condition mask
                                                               SPACE 2
*                             Ending register values
ENDLN1   DC    A(0)              target length
ENDLN2   DC    A(0)              source length

                                                                SPACE 2
CU14NEXT EQU   *              Start of next table entry...
                                                                SPACE 6
                                                                EJECT
CU14TST CSECT  ,
                                                                SPACE 2
***********************************************************************
*        CU14 Testing Control tables   (ref: CU14TEST DSECT)
***********************************************************************
         PRINT DATA
CU14CTL  DC    0A(0)    start of table
***********************************************************************
*        tests with CC=0 M3=0
***********************************************************************
                                                                SPACE 2
CC0T1    DS    0F
         DC    X'01'                       Test Num
         DC    X'00',X'00'
         DC    X'00'                       M3
*
         DC    A(UTF32A),A(UTF32AED-UTF32A)   target - Op1 & length
         DC    A(UTF8A),A(UTF8AEND-UTF8A)     Source - Op2 & length

         DC    A(1*MB+(0*K16))                target
         DC    A(2*MB+(0*K16))                source
*
         DC    A(7)                           FailCC - not CC0
         DC    A(0)                           Result - target len
         DC    A(0)                           Result - source len
                                                                SPACE 2
         DC    A(0)     end of table
         DC    A(0)     end of table
         DC    A(0)     end of table

                                                                EJECT
***********************************************************************
*        CU14  UTF-8 test data
***********************************************************************
                                                                SPACE
         DC    C'UTF8:  '                 eye catcher
UTF8ALN  DC    A(UTF8AEND-UTF8A)
UTF8A    DS    0H
         DC    XL1'00'  first UTF-8 1 Byte character
         DC    XL1'31'  1
         DC    XL1'39'  9
         DC    XL1'40'  @
         DC    XL1'41'  A
         DC    XL1'42'  B
         DC    XL1'7F'  last UTF-8 1 Byte character
                                                                SPACE
         DC    XL2'C280'  first UTF-8 2 Byte character
         DC    XL2'C380'  c3 80	LATIN CAPITAL LETTER A WITH GRAVE
         DC    XL2'C3B8'  c3 b8	LATIN SMALL LETTER O WITH STROKE
         DC    XL2'D09C'  D0 9C	М	Cyrillic Capital Letter Em
         DC    XL2'DFBF'  last UTF-8 2 Byte character DF BF	߿
                                                                SPACE
         DC    XL1'43'  C
                                                                SPACE
         DC    XL3'E0A080'  first UTF-8 3 Byte character
*                           E0 A0 80 ࠀ Samaritan Letter Alaf
         DC    XL3'E0A18D'  E0 A1 8D ࡍ Mandaic Letter An
         DC    XL3'EA9FBD'  EA 9F BD ꟽ Latin Epigraphic Inverted M
         DC    XL3'EFbf87'  EF BF 87 ￇ Halfwidth Hangul Letter E
         DC    XL3'EFBFBF'  last UTF-8 3 Byte character EF BF BF
                                                                SPACE
         DC    XL1'44'  D
                                                                SPACE
         DC    XL4'F0908080'  first UTF-8 4 Byte character
*                           F0 90 80 80	𐀀	Linear B Syllable B008 A
         DC    XL4'F0908487' F0 90 84 87	𐄇	Aegean Number One
         DC    XL4'F09294B5' F0 92 94 B5 Cuneiform Sign She Plus Sar
         DC    XL4'F09082B8'  F0 90 82 B8 𐂸 Linear B Ideogram B177
         DC    XL4'F096AB83'  F0 96 A8 83 𖨃 Bamum Letter Phase-f Ka
         DC    XL4'F0989A9F'  last UTF-8 4 Byte character
                                                                SPACE
         DC    XL1'45'  E
         DC    XL1'4E'  N
         DC    XL1'44'  D
UTF8AEND DS    0X
                                                                 EJECT
***********************************************************************
*        CU14  UTF-32 Result
***********************************************************************
         DC    C'UTF32: '                 eye catcher
UTF32ALN DC    A(UTF32AED-UTF32A)
UTF32A   DC    0X
         DC    X'00000000'
         DC    X'00000031'
         DC    X'00000039'
         DC    X'00000040'
         DC    X'00000041'
         DC    X'00000042'
         DC    X'0000007F'
         DC    X'00000080'
         DC    X'000000C0'
         DC    X'000000F8'
         DC    X'0000041C'
         DC    X'000007FF'
         DC    X'00000043'
         DC    X'00000800'
         DC    X'0000084D'
         DC    X'0000A7FD'
         DC    X'0000FFC7'
         DC    X'0000FFFF'
         DC    X'00000044'
         DC    X'00010000'
         DC    X'00010107'
         DC    X'00012535'
         DC    X'000100B8'
         DC    X'00016AC3'
         DC    X'0001869F'
         DC    X'00000045'
         DC    X'0000004E'
         DC    X'00000044'
UTF32AED DS    0X

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
                                                                SPACE 8
         END
