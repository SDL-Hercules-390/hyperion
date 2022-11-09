 TITLE '            CUSE-01-basic (Test CUSE instruction)'
***********************************************************************
*
*                   CUSE basic instruction tests
*
***********************************************************************
*
*  This program tests proper functioning of the CUSE instruction.
*  Specification Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of the instruction.
*
*  NOTE: This test is based on the CLCL-et-al Test but modified to
*        only test the CUSE instruction.  --  James Wekel November 2022
*
***********************************************************************
*
*  Example Hercules Testcase:
*
*
*      *Testcase CUSE-01-basic (Test CUSE instructions)
*
*      # ------------------------------------------------------------
*      #  This tests only the basic function of the CUSE instruction.
*      #  Specification Exceptions are NOT tested.
*      # ------------------------------------------------------------
*
*      mainsize    16
*      numcpu      1
*      sysclear
*      archlvl     z/Arch
*      loadcore    "$(testpath)/CUSE-01-basic.core" 0x0
*      runtest     1
*      *Done
*
*
***********************************************************************
                                                                SPACE 2
CUSE1TST START 0
         USING CUSE1TST,R0            Low core addressability
                                                                SPACE 2
         ORG   CUSE1TST+X'1A0'        z/Architecure RESTART PSW
         DC    X'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE 2
         ORG   CUSE1TST+X'1D0'        z/Architecure PROGRAM CHECK PSW
         DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                SPACE 3
         ORG   CUSE1TST+X'200'        Start of actual test program...
                                                                EJECT
***********************************************************************
*               The actual "CUSE1TST" program itself...
***********************************************************************
*
*  Architecture Mode: z/Arch
*  Register Usage:
*
*   R0       CUSE - SS length
*   R1       CUSE - Pad byte
*   R2       CUSE - First-Operand Address
*   R3       CUSE - First-Operand Length
*   R4       CUSE - Second-Operand Address
*   R5       CUSE - Second-Operand Length
*   R6       Testing control table - base current entry
*   R7       (work)
*   R8       First base register
*   R9       Second base register
*   R10-R13  (work)
*   R14      Subroutine call
*   R15      Secondary Subroutine call or work
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
                                                                SPACE 2
***********************************************************************
*        Run the test(s)...
***********************************************************************
                                                                SPACE
         BAL   R14,TEST01       Test CUSE instruction
                                                                SPACE 2
***********************************************************************
*         Test for normal or unexpected test completion...
***********************************************************************
                                                                SPACE
         CLI   TESTNUM,X'F4'    Did we end on expected test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         CLI   SUBTEST,X'04'    Did we end on expected SUB-test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         B     EOJ              Yes, then normal completion!
                                                                EJECT
***********************************************************************
*        Fixed test storage locations ...
***********************************************************************
                                                                SPACE 2
         ORG   CUSE1TST+X'400'
                                                                SPACE 4
TESTADDR DS    0D               Where test/subtest numbers will go
TESTNUM  DC    X'99'            Test number of active test
SUBTEST  DC    X'99'            Active test sub-test number
                                                                SPACE 4
         ORG   *+X'100'
                                                                EJECT
***********************************************************************
*        TEST01                   Test CUSE instruction
***********************************************************************
                                                                SPACE
TEST01   MVI   TESTNUM,X'01'
                                                                SPACE
         LA    R6,CUSECTL         Point R6 --> testing control table
         USING CUSETEST,R6        What each table entry looks like
                                                                SPACE
TST1LOOP EQU   *
         IC    R10,TNUM            Set test number
         STC   R10,TESTNUM
*
**       Initialize operand data  (move data to testing address)
*
*              Build Operand-1
                                                                SPACE
         L     R2,OP1WHERE        Where to move operand-1 data to
         L     R3,OP1LEN          Get operand-1 length
         L     R10,SS1ADDR        Calculate OP 1 starting
         SR    R10,R3               address
         A     R10,SS1LEN
         L     R11,OP1LEN
         MVCL  R2,R10
                                                                SPACE
         BCTR  R2,0               less one for last char addr
         MVC   0(0,R2),SS1LAST    set last char
                                                                SPACE
*              Build Operand-2
                                                                SPACE
         L     R4,OP2WHERE        Where to move operand-1 data to
         L     R5,OP2LEN          Get operand-1 length
         L     R10,SS2ADDR        Calculate OP 2 starting
         SR    R10,R5               address
         A     R10,SS2LEN
         L     R11,OP2LEN
         MVCL  R4,R10
                                                                SPACE
         BCTR  R4,0               less one for last char addr
         MVC   0(0,R4),SS2LAST    set last char
                                                                SPACE 2
**       Execute CUSE instruction and check for expected condition code
                                                                SPACE
         L     R11,FAILMASK       (failure CC)
         SLL   R11,4              (shift to BC instr CC position)
                                                                SPACE
         IC    R0,SSLEN           Set SS length
         IC    R1,PAD             Set SS Pad byte
                                                                SPACE
         LM    R2,R5,OPSWHERE
                                                                SPACE
         MVI   SUBTEST,X'00'      (primary test)
DOAGAIN  CUSE  R2,R4              Do Test
                                                                SPACE
         EX    R11,CUSEBC         fail if...
         BC    B'0001',DOAGAIN    cc=3, not finished
                                                                SPACE 2
*
**       Verify R2,R3,R4,R5 contain (or still contain!) expected values
*
         LM    R10,R11,ENDOP1    end OP-1 address and length
                                                                SPACE
         MVI   SUBTEST,X'01'      (R2 result - op1 found addr)
         CLR   R2,R10             R2 correct?
         BNE   CUSEFAIL           No, FAILTEST!
                                                                SPACE
         MVI   SUBTEST,X'02'      (R3 result - op1 remaining len)
         CLR   R3,R11             R3 correct
         BNE   CUSEFAIL           No, FAILTEST!
                                                                SPACE
         LM    R10,R11,ENDOP2    end OP-2 address and length
                                                                SPACE
         MVI   SUBTEST,X'03'      (R4 result - op2 found addr)
         CLR   R4,R10             R4 correct
         BNE   CUSEFAIL           No, FAILTEST!
                                                                SPACE
         MVI   SUBTEST,X'04'      (R3 result - op2 remaining len)
         CLR   R5,R11             R5 correct
         BNE   CUSEFAIL           No, FAILTEST!
                                                                SPACE
         LA    R6,CUSENEXT        Go on to next table entry
         CLC   =F'0',0(R6)        End of table?
         BNE   TST1LOOP           No, loop...
         B     CUSEDONE           Done! (success!)
                                                                SPACE 2
CUSEFAIL LA    R14,FAILTEST       Unexpected results!
CUSEDONE BR    R14                Return to caller or FAILTEST
                                                                SPACE 2
CUSEBC   BC    0,CUSEFAIL         (fail if unexpected condition code)
                                                                SPACE 2
         DROP  R6
         DROP  R15
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
                                                                SPACE 3
K        EQU   1024             One KB
PAGE     EQU   (4*K)            Size of one page
K4       EQU   (4*K)            4 KB
K32      EQU   (32*K)           32 KB
K64      EQU   (64*K)           64 KB
MB       EQU   (K*K)             1 MB
                                                                EJECT
CUSE1TST CSECT ,
                                                                SPACE 2
***********************************************************************
*        CUSETEST DSECT
***********************************************************************
                                                                SPACE 2
CUSETEST DSECT ,
TNUM     DC    X'00'          CUSE table number
         DC    XL3'00'
                                                                SPACE 2
SSLEN    DC    AL1(0)         CUSE - SS length
PAD      DC    X'00'          CUSE - Pad byte
SS1LAST  DC    X'00'          First-Operand SS last byte
SS2LAST  DC    X'00'          Second-Operand SS last byte
                                                                SPACE 2
SS1ADDR  DC    A(0)           First-Operand SS Address
SS1LEN   DC    A(0)           First-Operand SS length
SS2ADDR  DC    A(0)           Second-Operand SS Address
SS2LEN   DC    A(0)           Second-Operand SS length
                                                                SPACE 2
OPSWHERE EQU   *
OP1WHERE DC    A(0)           Where Operand-1 data should be placed
OP1LEN   DC    F'0'           CUSE - First-Operand Length
OP2WHERE DC    A(0)           Where Operand-2 data should be placed
OP2LEN   DC    F'0'           CUSE - Second-Operand Length

                                                                SPACE 2
FAILMASK DC    A(0)           Failure Branch on Condition mask
                                                                SPACE 2
*                             Ending register values
ENDOP1   DC    A(0)              Operand 1 address
         DC    A(0)              Operand 1 length
ENDOP2   DC    A(0)              Operand 2 address
         DC    A(0)              Operand 2 length
                                                                SPACE 2
CUSENEXT EQU   *              Start of next table entry...
                                                                SPACE 5
REG2PATT EQU   X'AABBCCDD'    Polluted Register pattern
REG2LOW  EQU         X'DD'    (last byte above)
                                                                EJECT
***********************************************************************
*        CUSE Testing Control tables   (ref: CUSETEST DSECT)
***********************************************************************
                                                                SPACE
CUSE1TST CSECT ,
CUSECTL  DC    0A(0)    start of table
                                                                SPACE
***********************************************************************
*        tests with   CC=0
***********************************************************************
                                                                SPACE
CC0T1    DS    0F
         DC    X'01'                       Test Num
         DC    XL3'00'
*
         DC    AL1(1)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'AA'                       First-Operand SS last byte
         DC    X'AA'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(001)               Op-1 SS & length
         DC    A(COP2A),A(001)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(1*K32)),A(1)          Op-1 & length
         DC    A(2*MB+(1*K32)),A(1)          Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(1*K32)+000),A(001)  OP-1
         DC    A(2*MB+(1*K32)+000),A(001)  OP-2
                                                                SPACE 2
CC0T2    DS    0F
         DC    X'02'                       Test Num
         DC    XL3'00'
*
         DC    AL1(1)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'BB'                       First-Operand SS last byte
         DC    X'BB'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(001)               Op-1 SS & length
         DC    A(COP2A),A(001)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(2*K32)),A(2)          Op-1 & length
         DC    A(2*MB+(2*K32)),A(2)          Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(2*K32)+001),A(001)  OP-1
         DC    A(2*MB+(2*K32)+001),A(001)  OP-2
                                                                SPACE 2
CC0T3    DS    0F
         DC    X'03'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'CC'                       First-Operand SS last byte
         DC    X'CC'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(004)               Op-1 SS & length
         DC    A(COP2A),A(004)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(3*K32)),A(8)          Op-1 & length
         DC    A(2*MB+(3*K32)),A(8)          Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(3*K32)+(8-4)),A(004)  OP-1
         DC    A(2*MB+(3*K32)+(8-4)),A(004)  OP-2
                                                                SPACE 2
CC0T4    DS    0F
         DC    X'04'                       Test Num
         DC    XL3'00'
*
         DC    AL1(13)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'DD'                       First-Operand SS last byte
         DC    X'DD'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(013)               Op-1 SS & length
         DC    A(COP2A),A(013)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(4*K32)),A(63)          Op-1 & length
         DC    A(2*MB+(4*K32)),A(63)          Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(4*K32)+(63-13)),A(013)  OP-1
         DC    A(2*MB+(4*K32)+(63-13)),A(013)  OP-2
                                                                SPACE 2
CC0T5    DS    0F
         DC    X'05'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'EE'                       First-Operand SS last byte
         DC    X'EE'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(5*K32)),A(512)          Op-1 & length
         DC    A(2*MB+(5*K32)),A(512)          Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(5*K32)+(512-62)),A(062)  OP-1
         DC    A(2*MB+(5*K32)+(512-62)),A(062)  OP-2
                                                                SPACE 2
CC0T6    DS    0F
         DC    X'06'                       Test Num
         DC    XL3'00'
*
         DC    AL1(127)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'FF'                       First-Operand SS last byte
         DC    X'FF'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(127)               Op-1 SS & length
         DC    A(COP2A),A(127)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(6*K32)),A(2048)          Op-1 & length
         DC    A(2*MB+(6*K32)),A(2048)          Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(6*K32)+(2048-127)),A(127)  OP-1
         DC    A(2*MB+(6*K32)+(2048-127)),A(127)  OP-2
                                                                SPACE
*        Cross page bounday tests
                                                                SPACE
*        Cross page bounday - operand-1
                                                                SPACE
CC0T7    DS    0F
         DC    X'07'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'55'                       First-Operand SS last byte
         DC    X'55'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(7*K32)-128),A(512)      Op-1 & length
         DC    A(2*MB+(7*K32)),A(512)          Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(7*K32)+(512-62)-128),A(062)  OP-1
         DC    A(2*MB+(7*K32)+(512-62)),A(062)      OP-2
                                                                SPACE
*        Cross page bounday - operand-2
                                                                SPACE
CC0T8    DS    0F
         DC    X'08'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'66'                       First-Operand SS last byte
         DC    X'66'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(8*K32)),A(512)        Op-1 & length
         DC    A(2*MB+(8*K32)-128),A(512)    Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(8*K32)+(512-62)),A(062)       OP-1
         DC    A(2*MB+(8*K32)+(512-62)-128),A(062)   OP-2
                                                                SPACE
*        Cross page bounday - operand-1 and operand-2
                                                                SPACE
CC0T9    DS    0F
         DC    X'09'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(9*K32)-96),A(512)     Op-1 & length
         DC    A(2*MB+(9*K32)-128),A(512)    Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(9*K32)+(512-62)-96),A(062)    OP-1
         DC    A(2*MB+(9*K32)+(512-62)-128),A(062)   OP-2
                                                                SPACE
*        PAD tests
                                                                SPACE
*        Pad - operand-1
                                                                SPACE
CC0TA    DS    0F
         DC    X'0A'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(10*K32)),A(500)          Op-1 & length
         DC    A(2*MB+(10*K32)),A(512)          Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(10*K32)+(512-62)),A(062-(512-500))  OP-1
         DC    A(2*MB+(10*K32)+(512-62)),A(062)            OP-2
                                                                SPACE
*        Pad - operand-2
                                                                SPACE
CC0TB    DS    0F
         DC    X'0B'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(11*K32)),A(512)          Op-1 & length
         DC    A(2*MB+(11*K32)),A(500)          Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(11*K32)+(512-62)),A(062)              OP-1
         DC    A(2*MB+(11*K32)+(512-62)),A(062-(512-500))    OP-2
                                                                SPACE
*        PAD and Cross page bounday tests
                                                                SPACE
*        Pad - operand-1 ; Cross page bounday - operand-1
                                                                SPACE
CC0TC    DS    0F
         DC    X'0C'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(12*K32)-96),A(500)       Op-1 & length
         DC    A(2*MB+(12*K32)),A(512)          Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(12*K32)+(512-62)-96),A(062-(512-500))  OP-1
         DC    A(2*MB+(12*K32)+(512-62)),A(062)               OP-2
                                                                SPACE
*        Pad - operand-1 ; Cross page bounday - operand-2
                                                                SPACE
CC0TD    DS    0F
         DC    X'0D'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(13*K32)),A(500)             Op-1 & length
         DC    A(2*MB+(13*K32)-96),A(512)          Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(13*K32)+(512-62)),A(062-(512-500))  OP-1
         DC    A(2*MB+(13*K32)+(512-62)-96),A(062)         OP-2
                                                                SPACE
*        Pad - operand-2 ; Cross page bounday - operand-1
                                                                SPACE
CC0TE    DS    0F
         DC    X'0E'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(14*K32)-96),A(512)       Op-1 & length
         DC    A(2*MB+(14*K32)),A(500)          Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(14*K32)+(512-62)-96),A(062)           OP-1
         DC    A(2*MB+(14*K32)+(512-62)),A(062-(512-500))    OP-2
                                                                SPACE
*        Pad - operand-2 ; Cross page bounday - operand-2
                                                                SPACE
CC0TF    DS    0F
         DC    X'0F'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(1*MB+(15*K32)),A(512)          Op-1 & length
         DC    A(2*MB+(15*K32)-96),A(500)       Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(1*MB+(15*K32)+(512-62)),A(062)                 OP-1
         DC    A(2*MB+(15*K32)+(512-62)-96),A(062-(512-500))    OP-2
                                                                EJECT
***********************************************************************
*        tests with   CC=1
***********************************************************************
                                                                SPACE
CC1T1    DS    0F
         DC    X'11'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'11'                       Pad Byte
         DC    X'11'                       First-Operand SS last byte
         DC    X'11'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(001)               Op-1 SS & length
         DC    A(COP2A),A(001)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(1*K32)),A(1)          Op-1 & length
         DC    A(4*MB+(1*K32)),A(1)          Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(1*K32)+000),A(001)  OP-1
         DC    A(4*MB+(1*K32)+000),A(001)  OP-2
                                                                SPACE 2
CC1T2    DS    0F
         DC    X'12'                       Test Num
         DC    XL3'00'
*
         DC    AL1(2)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'BB'                       First-Operand SS last byte
         DC    X'BB'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(001)               Op-1 SS & length
         DC    A(COP2A),A(001)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(2*K32)),A(2)          Op-1 & length
         DC    A(4*MB+(2*K32)),A(2)          Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(2*K32)+001),A(001)  OP-1
         DC    A(4*MB+(2*K32)+001),A(001)  OP-2
                                                                SPACE 2
CC1T3    DS    0F
         DC    X'13'                       Test Num
         DC    XL3'00'
*
         DC    AL1(6)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'CC'                       First-Operand SS last byte
         DC    X'CC'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(004)               Op-1 SS & length
         DC    A(COP2A),A(004)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(3*K32)),A(8)          Op-1 & length
         DC    A(4*MB+(3*K32)),A(8)          Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(3*K32)+(8-4)),A(004)  OP-1
         DC    A(4*MB+(3*K32)+(8-4)),A(004)  OP-2
                                                                SPACE 2
CC1T4    DS    0F
         DC    X'14'                       Test Num
         DC    XL3'00'
*
         DC    AL1(18)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'DD'                       First-Operand SS last byte
         DC    X'DD'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(013)               Op-1 SS & length
         DC    A(COP2A),A(013)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(4*K32)),A(63)          Op-1 & length
         DC    A(4*MB+(4*K32)),A(63)          Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(4*K32)+(63-13)),A(013)  OP-1
         DC    A(4*MB+(4*K32)+(63-13)),A(013)  OP-2
                                                                SPACE 2
CC1T5    DS    0F
         DC    X'15'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'EE'                       First-Operand SS last byte
         DC    X'EE'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(5*K32)),A(512)          Op-1 & length
         DC    A(4*MB+(5*K32)),A(512)          Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(5*K32)+(512-62)),A(062)  OP-1
         DC    A(4*MB+(5*K32)+(512-62)),A(062)  OP-2
                                                                SPACE 2
CC1T6    DS    0F
         DC    X'16'                       Test Num
         DC    XL3'00'
*
         DC    AL1(128)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'FF'                       First-Operand SS last byte
         DC    X'FF'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(127)               Op-1 SS & length
         DC    A(COP2A),A(127)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(6*K32)),A(2048)          Op-1 & length
         DC    A(4*MB+(6*K32)),A(2048)          Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(6*K32)+(2048-127)),A(127)  OP-1
         DC    A(4*MB+(6*K32)+(2048-127)),A(127)  OP-2
                                                                SPACE
*        Cross page bounday tests
                                                                SPACE
*        Cross page bounday - operand-1
                                                                SPACE
CC1T7    DS    0F
         DC    X'17'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'55'                       First-Operand SS last byte
         DC    X'55'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(7*K32)-128),A(512)      Op-1 & length
         DC    A(4*MB+(7*K32)),A(512)          Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(7*K32)+(512-62)-128),A(062)  OP-1
         DC    A(4*MB+(7*K32)+(512-62)),A(062)      OP-2
                                                                SPACE
*        Cross page bounday - operand-2
                                                                SPACE
CC1T8    DS    0F
         DC    X'18'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'66'                       First-Operand SS last byte
         DC    X'66'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(8*K32)),A(512)        Op-1 & length
         DC    A(4*MB+(8*K32)-128),A(512)    Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(8*K32)+(512-62)),A(062)       OP-1
         DC    A(4*MB+(8*K32)+(512-62)-128),A(062)   OP-2
                                                                SPACE
*        Cross page bounday - operand-1 and operand-2
                                                                SPACE
CC1T9    DS    0F
         DC    X'19'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(9*K32)-96),A(512)     Op-1 & length
         DC    A(4*MB+(9*K32)-128),A(512)    Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(9*K32)+(512-62)-96),A(062)    OP-1
         DC    A(4*MB+(9*K32)+(512-62)-128),A(062)   OP-2
                                                                SPACE
*        PAD tests
                                                                SPACE
*        Pad - operand-1
                                                                SPACE
CC1TA    DS    0F
         DC    X'1A'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(10*K32)),A(500)          Op-1 & length
         DC    A(4*MB+(10*K32)),A(512)          Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(10*K32)+(512-62)),A(062-(512-500))  OP-1
         DC    A(4*MB+(10*K32)+(512-62)),A(062)            OP-2
                                                                SPACE
*        Pad - operand-2
                                                                SPACE
CC1TB    DS    0F
         DC    X'1B'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(11*K32)),A(512)          Op-1 & length
         DC    A(4*MB+(11*K32)),A(500)          Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(11*K32)+(512-62)),A(062)              OP-1
         DC    A(4*MB+(11*K32)+(512-62)),A(062-(512-500))    OP-2
                                                                SPACE
*        PAD and Cross page bounday tests
                                                                SPACE
*        Pad - operand-1 ; Cross page bounday - operand-1
                                                                SPACE
CC1TC    DS    0F
         DC    X'1C'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(12*K32)-96),A(500)       Op-1 & length
         DC    A(4*MB+(12*K32)),A(512)          Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(12*K32)+(512-62)-96),A(062-(512-500))  OP-1
         DC    A(4*MB+(12*K32)+(512-62)),A(062)               OP-2
                                                                SPACE
*        Pad - operand-1 ; Cross page bounday - operand-2
                                                                SPACE
CC1TD    DS    0F
         DC    X'1D'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(13*K32)),A(500)             Op-1 & length
         DC    A(4*MB+(13*K32)-96),A(512)          Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(13*K32)+(512-62)),A(062-(512-500))  OP-1
         DC    A(4*MB+(13*K32)+(512-62)-96),A(062)         OP-2
                                                                SPACE
*        Pad - operand-2 ; Cross page bounday - operand-1
                                                                SPACE
CC1TE    DS    0F
         DC    X'1E'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(14*K32)-96),A(512)       Op-1 & length
         DC    A(4*MB+(14*K32)),A(500)          Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(14*K32)+(512-62)-96),A(062)           OP-1
         DC    A(4*MB+(14*K32)+(512-62)),A(062-(512-500))    OP-2
                                                                SPACE
*        Pad - operand-2 ; Cross page bounday - operand-2
                                                                SPACE
CC1TF    DS    0F
         DC    X'1F'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(3*MB+(15*K32)),A(512)          Op-1 & length
         DC    A(4*MB+(15*K32)-96),A(500)       Op-2 & length
*
         DC    A(11) CC1                   Fail mask
*                                          Ending register values
         DC    A(3*MB+(15*K32)+(512-62)),A(062)                 OP-1
         DC    A(4*MB+(15*K32)+(512-62)-96),A(062-(512-500))    OP-2
                                                                EJECT
***********************************************************************
*        tests with   CC=2
***********************************************************************
                                                                SPACE
CC2T1    DS    0F
         DC    X'21'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'11'                       Pad Byte
         DC    X'11'                       First-Operand SS last byte
         DC    X'12'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(001)               Op-1 SS & length
         DC    A(COP2A),A(001)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(1*K32)),A(1)          Op-1 & length
         DC    A(6*MB+(1*K32)),A(1)          Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(1*K32)+001),A(000)  OP-1
         DC    A(6*MB+(1*K32)+001),A(000)  OP-2
                                                                SPACE 2
CC2T2    DS    0F
         DC    X'22'                       Test Num
         DC    XL3'00'
*
         DC    AL1(2)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'BB'                       First-Operand SS last byte
         DC    X'BC'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(001)               Op-1 SS & length
         DC    A(COP2A),A(001)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(2*K32)),A(2)          Op-1 & length
         DC    A(6*MB+(2*K32)),A(2)          Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(2*K32)+002),A(000)  OP-1
         DC    A(6*MB+(2*K32)+002),A(000)  OP-2
                                                                SPACE 2
CC2T3    DS    0F
         DC    X'23'                       Test Num
         DC    XL3'00'
*
         DC    AL1(6)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'CC'                       First-Operand SS last byte
         DC    X'CD'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(004)               Op-1 SS & length
         DC    A(COP2A),A(004)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(3*K32)),A(8)          Op-1 & length
         DC    A(6*MB+(3*K32)),A(8)          Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(3*K32)+8),A(000)  OP-1
         DC    A(6*MB+(3*K32)+8),A(000)  OP-2
                                                                SPACE 2
CC2T4    DS    0F
         DC    X'24'                       Test Num
         DC    XL3'00'
*
         DC    AL1(18)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'DD'                       First-Operand SS last byte
         DC    X'DE'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(013)               Op-1 SS & length
         DC    A(COP2A),A(013)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(4*K32)),A(63)          Op-1 & length
         DC    A(6*MB+(4*K32)),A(63)          Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(4*K32)+63),A(000)  OP-1
         DC    A(6*MB+(4*K32)+63),A(000)  OP-2
                                                                SPACE 2
CC2T5    DS    0F
         DC    X'25'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'EE'                       First-Operand SS last byte
         DC    X'EF'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(5*K32)),A(512)          Op-1 & length
         DC    A(6*MB+(5*K32)),A(512)          Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(5*K32)+512),A(000)  OP-1
         DC    A(6*MB+(5*K32)+512),A(000)  OP-2
                                                                SPACE 2
CC2T6    DS    0F
         DC    X'26'                       Test Num
         DC    XL3'00'
*
         DC    AL1(128)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'FF'                       First-Operand SS last byte
         DC    X'F0'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(127)               Op-1 SS & length
         DC    A(COP2A),A(127)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(6*K32)),A(2048)          Op-1 & length
         DC    A(6*MB+(6*K32)),A(2048)          Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(6*K32)+2048),A(000)  OP-1
         DC    A(6*MB+(6*K32)+2048),A(000)  OP-2
                                                                SPACE
*        Cross page bounday tests
                                                                SPACE
*        Cross page bounday - operand-1
                                                                SPACE
CC2T7    DS    0F
         DC    X'27'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'55'                       First-Operand SS last byte
         DC    X'56'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(7*K32)-128),A(512)      Op-1 & length
         DC    A(6*MB+(7*K32)),A(512)          Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(7*K32)+512-128),A(000)  OP-1
         DC    A(6*MB+(7*K32)+512),A(000)      OP-2
                                                                SPACE
*        Cross page bounday - operand-2
                                                                SPACE
CC2T8    DS    0F
         DC    X'28'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'67'                       First-Operand SS last byte
         DC    X'66'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(8*K32)),A(512)        Op-1 & length
         DC    A(6*MB+(8*K32)-128),A(512)    Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(8*K32)+512),A(000)       OP-1
         DC    A(6*MB+(8*K32)+512-128),A(000)   OP-2
                                                                SPACE
*        Cross page bounday - operand-1 and operand-2
                                                                SPACE
CC2T9    DS    0F
         DC    X'29'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'78'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(9*K32)-96),A(512)     Op-1 & length
         DC    A(6*MB+(9*K32)-128),A(512)    Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(9*K32)+512-96),A(000)    OP-1
         DC    A(6*MB+(9*K32)+512-128),A(000)   OP-2
                                                                SPACE
*        PAD tests
                                                                SPACE
*        Pad - operand-1
                                                                SPACE
CC2TA    DS    0F
         DC    X'2A'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'41'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(10*K32)),A(500)          Op-1 & length
         DC    A(6*MB+(10*K32)),A(512)          Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(10*K32)+500),A(000)      OP-1
         DC    A(6*MB+(10*K32)+512),A(000)      OP-2
                                                                SPACE
*        Pad - operand-2
                                                                SPACE
CC2TB    DS    0F
         DC    X'2B'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'41'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(11*K32)),A(512)          Op-1 & length
         DC    A(6*MB+(11*K32)),A(500)          Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(11*K32)+512),A(000)      OP-1
         DC    A(6*MB+(11*K32)+500),A(000)      OP-2
                                                                SPACE
*        PAD and Cross page bounday tests
                                                                SPACE
*        Pad - operand-1 ; Cross page bounday - operand-1
                                                                SPACE
CC2TC    DS    0F
         DC    X'2C'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'41'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(12*K32)-96),A(500)       Op-1 & length
         DC    A(6*MB+(12*K32)),A(512)          Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(12*K32)+500-96),A(000)   OP-1
         DC    A(6*MB+(12*K32)+512),A(000)      OP-2
                                                                SPACE
*        Pad - operand-1 ; Cross page bounday - operand-2
                                                                SPACE
CC2TD    DS    0F
         DC    X'2D'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'41'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(13*K32)),A(500)             Op-1 & length
         DC    A(6*MB+(13*K32)-96),A(512)          Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(13*K32)+500),A(000)      OP-1
         DC    A(6*MB+(13*K32)+512-96),A(000)   OP-2
                                                                SPACE
*        Pad - operand-2 ; Cross page bounday - operand-1
                                                                SPACE
CC2TE    DS    0F
         DC    X'2E'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'41'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(14*K32)-96),A(512)       Op-1 & length
         DC    A(6*MB+(14*K32)),A(500)          Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(14*K32)+512-96),A(000)   OP-1
         DC    A(6*MB+(14*K32)+500),A(000)      OP-2
                                                                SPACE
*        Pad - operand-2 ; Cross page bounday - operand-2
                                                                SPACE
CC2TF    DS    0F
         DC    X'2F'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'41'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(062)               Op-1 SS & length
         DC    A(COP2B),A(062)               OP-2 SS & length
*                                          Target
         DC    A(5*MB+(15*K32)),A(512)          Op-1 & length
         DC    A(6*MB+(15*K32)-96),A(500)       Op-2 & length
*
         DC    A(13)  not CC2              Fail mask
*                                          Ending register values
         DC    A(5*MB+(15*K32)+512),A(000)       OP-1
         DC    A(6*MB+(15*K32)+500-96),A(000)    OP-2
                                                                EJECT
***********************************************************************
*        tests with   CC=3
***********************************************************************
                                                                SPACE
CC3T1    DS    0F
         DC    X'31'                       Test Num
         DC    XL3'00'
*
         DC    AL1(1)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'AA'                       First-Operand SS last byte
         DC    X'AA'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(1)                    Op-1 SS & length
         DC    A(COP2A),A(1)                    OP-2 SS & length
*                                          Target
         DC    A(7*MB+(1*K32)),A(4096+128)      Op-1 & length
         DC    A(8*MB+(1*K32)),A(4096+128)      Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(7*MB+(1*K32)+4096+128-1),A(001)  OP-1
         DC    A(8*MB+(1*K32)+4096+128-1),A(001)  OP-2
                                                                SPACE 2
CC3T3    DS    0F
         DC    X'33'                       Test Num
         DC    XL3'00'
*
         DC    AL1(6)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'CC'                       First-Operand SS last byte
         DC    X'CC'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(004)               Op-1 SS & length
         DC    A(COP2A),A(004)               OP-2 SS & length
*                                          Target
         DC    A(7*MB+(3*K32)),A(4096+128)   Op-1 & length
         DC    A(8*MB+(3*K32)),A(4096+128)   Op-2 & length
*
         DC    A(10)  not CC1 or CC3       Fail mask
*                                          Ending register values
         DC    A(7*MB+(3*K32)+(4096+128-4)),A(004)  OP-1
         DC    A(8*MB+(3*K32)+(4096+128-4)),A(004)  OP-2
                                                                SPACE 2
CC3T4    DS    0F
         DC    X'34'                       Test Num
         DC    XL3'00'
*
         DC    AL1(18)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'DD'                       First-Operand SS last byte
         DC    X'DE'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(013)               Op-1 SS & length
         DC    A(COP2A),A(013)               OP-2 SS & length
*                                          Target
         DC    A(7*MB+(4*K32)),A(4096+63)   Op-1 & length
         DC    A(8*MB+(4*K32)),A(4096+63)   Op-2 & length
*
         DC    A(12)  not CC2 or CC3      Fail mask
*                                          Ending register values
         DC    A(7*MB+(4*K32)+4096+63),A(000)  OP-1
         DC    A(8*MB+(4*K32)+4096+63),A(000)  OP-2
                                                                SPACE
*        Cross page bounday tests
                                                                SPACE
*        Cross page bounday - operand-1
                                                                SPACE
CC3T7    DS    0F
         DC    X'37'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'55'                       First-Operand SS last byte
         DC    X'55'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(7*MB+(7*K32)-128),A(4096+128)  Op-1 & length
         DC    A(8*MB+(7*K32)),A(4096+128)      Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(7*MB+(7*K32)+(4096+128-62)-128),A(062)  OP-1
         DC    A(8*MB+(7*K32)+(4096+128-62)),A(062)      OP-2
                                                                SPACE
*        Cross page bounday - operand-2
                                                                SPACE
CC3T8    DS    0F
         DC    X'38'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'66'                       First-Operand SS last byte
         DC    X'66'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(7*MB+(8*K32)),A(4096+128)        Op-1 & length
         DC    A(8*MB+(8*K32)-128),A(4096+128)    Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(7*MB+(8*K32)+(4096+128-62)),A(062)       OP-1
         DC    A(8*MB+(8*K32)+(4096+128-62)-128),A(062)   OP-2
                                                                SPACE
*        Cross page bounday - operand-1 and operand-2
                                                                SPACE
CC3T9    DS    0F
         DC    X'39'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(7*MB+(9*K32)-96),A(4096+128)     Op-1 & length
         DC    A(8*MB+(9*K32)-128),A(4096+128)    Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(7*MB+(9*K32)+(4096+128-62)-96),A(062)    OP-1
         DC    A(8*MB+(9*K32)+(4096+128-62)-128),A(062)   OP-2
                                                                EJECT
***********************************************************************
*        tests - special pad test
***********************************************************************
                                                                SPACE
*                          Op-1 - length=0
PAD4T1   DS    0F
         DC    X'41'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(000)               Op-1 SS & length
         DC    A(COP2B),A(4)                 OP-2 SS & length
*                                          Target
         DC    A(9*MB+(1*K32)),A(000)        Op-1 & length
         DC    A(10*MB+(1*K32)),A(512)       Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(9*MB+(1*K32)),A(000)            OP-1
         DC    A(10*MB+(1*K32)+(512-4)),A(004)   OP-2
                                                                SPACE 2
*                          Op-2 - length=0
PAD4T2   DS    0F
         DC    X'42'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                     SS Length
         DC    X'40'                       Pad Byte
         DC    X'40'                       First-Operand SS last byte
         DC    X'40'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(4)                 Op-1 SS & length
         DC    A(COP2B),A(000)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(2*K32)),A(512)          Op-1 & length
         DC    A(10*MB+(2*K32)),A(0)           Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(9*MB+(2*K32)+(512-4)),A(004)   OP-1
         DC    A(10*MB+(2*K32)),A(000)          OP-2
                                                                EJECT
***********************************************************************
*        tests for Special Cases Optimizations
***********************************************************************
                                                                SPACE 2
*        tests for Special Cases Optimizations
                                                                SPACE
SC5T1    DS    0F
         DC    X'51'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1C),A(032)               Op-1 SS & length
         DC    A(COP2C),A(032)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(7*K32)-96),A(512)      Op-1 & length
         DC    A(10*MB+(7*K32)-128),A(512)    Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(9*MB+(7*K32)+(512-32)-96-3),A(032+3)    OP-1
         DC    A(10*MB+(7*K32)+(512-32)-128-3),A(032+3)  OP-2
                                                                SPACE
SC5T2    DS    0F
         DC    X'52'                       Test Num
         DC    XL3'00'
*
         DC    AL1(7)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1C),A(027)               Op-1 SS & length
         DC    A(COP2C),A(027)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(8*K32)-96),A(512)      Op-1 & length
         DC    A(10*MB+(8*K32)-128),A(512)    Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(9*MB+(8*K32)+(512-27)-96-3),A(027+3)      OP-1
         DC    A(10*MB+(8*K32)+(512-27)-128-3),A(027+3)    OP-2
                                                                SPACE
SC5T3    DS    0F
         DC    X'53'                       Test Num
         DC    XL3'00'
*
         DC    AL1(1)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1B),A(027)               Op-1 SS & length
         DC    A(COP2B),A(027)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(9*K32)-96),A(512)      Op-1 & length
         DC    A(10*MB+(9*K32)-128),A(512)    Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(9*MB+(9*K32)+(512-27)-96),A(027)      OP-1
         DC    A(10*MB+(9*K32)+(512-27)-128),A(027)    OP-2
                                                                SPACE
SC5T4    DS    0F
         DC    X'54'                       Test Num
         DC    XL3'00'
*
         DC    AL1(3)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1D),A(027)               Op-1 SS & length
         DC    A(COP2D),A(027)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(10*K32)-96),A(512)      Op-1 & length
         DC    A(10*MB+(10*K32)-128),A(512)    Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(9*MB+(10*K32)+(512-27)-96),A(27)      OP-1
         DC    A(10*MB+(10*K32)+(512-27)-128),A(27)    OP-2
                                                                SPACE
*        subtring starts on a page boundary
                                                                SPACE
SC5T5    DS    0F
         DC    X'55'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'CC'                       First-Operand SS last byte
         DC    X'CC'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(004)               Op-1 SS & length
         DC    A(COP2A),A(004)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(11*K32)-4),A(8)          Op-1 & length
         DC    A(10*MB+(11*K32)-4),A(8)         Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(9*MB+(11*K32)-4+(8-4)),A(004)   OP-1
         DC    A(10*MB+(11*K32)-4+(8-4)),A(004)  OP-2
                                                                SPACE
*        subtring starts on a byte before page boundary
                                                                SPACE
SC5T6    DS    0F
         DC    X'56'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'CC'                       First-Operand SS last byte
         DC    X'CC'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(004)               Op-1 SS & length
         DC    A(COP2A),A(004)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(12*K32)-5),A(8)          Op-1 & length
         DC    A(10*MB+(12*K32)-5),A(8)         Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(9*MB+(12*K32)-5+(8-4)),A(004)   OP-1
         DC    A(10*MB+(12*K32)-5+(8-4)),A(004)  OP-2
                                                                SPACE
*        subtring starts on a byte after page boundary
                                                                SPACE
SC5T7    DS    0F
         DC    X'57'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'CC'                       First-Operand SS last byte
         DC    X'CC'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(004)               Op-1 SS & length
         DC    A(COP2A),A(004)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(13*K32)-3),A(8)          Op-1 & length
         DC    A(10*MB+(13*K32)-3),A(8)         Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(9*MB+(13*K32)-3+(8-4)),A(004)   OP-1
         DC    A(10*MB+(13*K32)-3+(8-4)),A(004)  OP-2
                                                                SPACE
*        Strings with multiple equal bytes
*        subtring starts on a page boundary
                                                                SPACE
SC5T8    DS    0F
         DC    X'58'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'CC'                       First-Operand SS last byte
         DC    X'CC'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1C),A(004)               Op-1 SS & length
         DC    A(COP2C),A(004)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(14*K32)-4),A(8)          Op-1 & length
         DC    A(10*MB+(14*K32)-4),A(8)         Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(9*MB+(14*K32)-4+(8-7)),A(007)   OP-1
         DC    A(10*MB+(14*K32)-4+(8-7)),A(007)  OP-2
                                                                SPACE
*        subtring starts on a byte before page boundary
                                                                SPACE
SC5T9    DS    0F
         DC    X'59'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'CC'                       First-Operand SS last byte
         DC    X'CC'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1C),A(004)               Op-1 SS & length
         DC    A(COP2C),A(004)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(15*K32)-5),A(8)          Op-1 & length
         DC    A(10*MB+(15*K32)-5),A(8)         Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(9*MB+(15*K32)-5+(8-7)),A(007)   OP-1
         DC    A(10*MB+(15*K32)-5+(8-7)),A(007)  OP-2
                                                                SPACE
*        subtring starts on a byte after page boundary
                                                                SPACE
SC5TA    DS    0F
         DC    X'5A'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'CC'                       First-Operand SS last byte
         DC    X'CC'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1C),A(004)               Op-1 SS & length
         DC    A(COP2C),A(004)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(16*K32)-3),A(8)          Op-1 & length
         DC    A(10*MB+(16*K32)-3),A(8)         Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(9*MB+(16*K32)-3+(8-7)),A(007)   OP-1
         DC    A(10*MB+(16*K32)-3+(8-7)),A(007)  OP-2
                                                                SPACE
*        Strings with multiple equal bytes
*        subtring starts on a page boundary
                                                                SPACE
SC5TB    DS    0F
         DC    X'5B'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'CC'                       First-Operand SS last byte
         DC    X'CC'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1D),A(004)               Op-1 SS & length
         DC    A(COP2D),A(004)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(17*K32)-4),A(8)          Op-1 & length
         DC    A(10*MB+(17*K32)-4),A(8)         Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(9*MB+(17*K32)-4+(8-4)),A(004)   OP-1
         DC    A(10*MB+(17*K32)-4+(8-4)),A(004)  OP-2
                                                                SPACE
*        subtring starts on a byte before page boundary
                                                                SPACE
SC5TC    DS    0F
         DC    X'5C'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'CC'                       First-Operand SS last byte
         DC    X'CC'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1D),A(004)               Op-1 SS & length
         DC    A(COP2D),A(004)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(18*K32)-5),A(8)          Op-1 & length
         DC    A(10*MB+(18*K32)-5),A(8)         Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(9*MB+(18*K32)-5+(8-4)),A(004)   OP-1
         DC    A(10*MB+(18*K32)-5+(8-4)),A(004)  OP-2
                                                                SPACE
*        subtring starts on a byte after page boundary
                                                                SPACE
SC5TD    DS    0F
         DC    X'5D'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'CC'                       First-Operand SS last byte
         DC    X'CC'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1D),A(004)               Op-1 SS & length
         DC    A(COP2D),A(004)               OP-2 SS & length
*                                          Target
         DC    A(9*MB+(19*K32)-3),A(8)          Op-1 & length
         DC    A(10*MB+(19*K32)-3),A(8)         Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(9*MB+(19*K32)-3+(8-4)),A(004)   OP-1
         DC    A(10*MB+(19*K32)-3+(8-4)),A(004)  OP-2

                                                                EJECT
***********************************************************************
*        potential tests for CUSE-02-performance
***********************************************************************
                                                                SPACE
*        Cross page bounday - operand-1 and operand-2
                                                                SPACE
PTE1     DS    0F
         DC    X'E1'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'EE'                       First-Operand SS last byte
         DC    X'EE'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(004)               Op-1 SS & length
         DC    A(COP2A),A(004)               OP-2 SS & length
*                                          Target
         DC    A(11*MB+(1*K32)-63),A(512)        Op-1 & length
         DC    A(12*MB+(1*K32)-56),A(512)        Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(11*MB+(1*K32)-63+(512-4)),A(004)   OP-1
         DC    A(12*MB+(1*K32)-56+(512-4)),A(004)   OP-2
                                                                SPACE
PTE2     DS    0F
         DC    X'E2'                       Test Num
         DC    XL3'00'
*
         DC    AL1(8)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(008)               Op-1 SS & length
         DC    A(COP2A),A(008)               OP-2 SS & length
*                                          Target
         DC    A(11*MB+(2*K32)-96),A(512)    Op-1 & length
         DC    A(12*MB+(2*K32)-128),A(512)   Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(11*MB+(2*K32)+(512-8)-96),A(008)     OP-1
         DC    A(12*MB+(2*K32)+(512-8)-128),A(008)    OP-2
                                                                SPACE 2
PTE3     DS    0F
         DC    X'E3'                       Test Num
         DC    XL3'00'
*
         DC    AL1(16)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(016)               Op-1 SS & length
         DC    A(COP2A),A(016)               OP-2 SS & length
*                                          Target
         DC    A(11*MB+(3*K32)-96),A(512)    Op-1 & length
         DC    A(12*MB+(3*K32)-128),A(512)   Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(11*MB+(3*K32)+(512-16)-96),A(016)     OP-1
         DC    A(12*MB+(3*K32)+(512-16)-128),A(016)    OP-2
                                                                SPACE 2
PTE4     DS    0F
         DC    X'E4'                       Test Num
         DC    XL3'00'
*
         DC    AL1(32)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(032)               Op-1 SS & length
         DC    A(COP2A),A(032)               OP-2 SS & length
*                                          Target
         DC    A(11*MB+(4*K32)-96),A(512)    Op-1 & length
         DC    A(12*MB+(4*K32)-128),A(512)   Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(11*MB+(4*K32)+(512-32)-96),A(032)    OP-1
         DC    A(12*MB+(4*K32)+(512-32)-128),A(032)   OP-2
                                                                SPACE 2
PTE5     DS    0F
         DC    X'E5'                       Test Num
         DC    XL3'00'
*
         DC    AL1(64)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(064)               Op-1 SS & length
         DC    A(COP2A),A(064)               OP-2 SS & length
*                                          Target
         DC    A(11*MB+(5*K32)-96),A(512)    Op-1 & length
         DC    A(12*MB+(5*K32)-128),A(512)   Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(11*MB+(5*K32)+(512-64)-96),A(064)    OP-1
         DC    A(12*MB+(5*K32)+(512-64)-128),A(064)   OP-2
                                                                SPACE 2
PTE6     DS    0F
         DC    X'E6'                       Test Num
         DC    XL3'00'
*
         DC    AL1(1)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(032)               Op-1 SS & length
         DC    A(COP2A),A(032)               OP-2 SS & length
*                                          Target
         DC    A(11*MB+(6*K32)-96),A(512)     Op-1 & length
         DC    A(12*MB+(6*K32)-128),A(512)    Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(11*MB+(6*K32)+(512-32)-96),A(032)    OP-1
         DC    A(12*MB+(6*K32)+(512-32)-128),A(032)   OP-2
                                                                SPACE 2
PTE7     DS    0F
         DC    X'E7'                       Test Num
         DC    XL3'00'
*
         DC    AL1(4)                      SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1C),A(032)               Op-1 SS & length
         DC    A(COP2C),A(032)               OP-2 SS & length
*                                          Target
         DC    A(11*MB+(7*K32)-96),A(512)     Op-1 & length
         DC    A(12*MB+(7*K32)-128),A(512)    Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(11*MB+(7*K32)+(512-32)-96-3),A(032+3)    OP-1
         DC    A(12*MB+(7*K32)+(512-32)-128-3),A(032+3)   OP-2
                                                                EJECT
***********************************************************************
*        potential tests for CUSE-02-performance
***********************************************************************
                                                                SPACE
PTF1     DS    0F
         DC    X'F1'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'EE'                       First-Operand SS last byte
         DC    X'EE'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(13*MB+(1*K32)),A(512)         Op-1 & length
         DC    A(14*MB+(1*K32)),A(512)         Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(13*MB+(1*K32)+(512-62)),A(062)  OP-1
         DC    A(14*MB+(1*K32)+(512-62)),A(062)  OP-2
                                                                SPACE
*        Cross page bounday - operand-1 and operand-2
                                                                SPACE
PTF2     DS    0F
         DC    X'F2'                       Test Num
         DC    XL3'00'
*
         DC    AL1(32)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(032)               Op-1 SS & length
         DC    A(COP2A),A(032)               OP-2 SS & length
*                                          Target
         DC    A(13*MB+(2*K32)-96),A(512)     Op-1 & length
         DC    A(14*MB+(2*K32)-128),A(512)    Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(13*MB+(2*K32)+(512-32)-96),A(032)    OP-1
         DC    A(14*MB+(2*K32)+(512-32)-128),A(032)   OP-2
                                                                SPACE 2
PTF3     DS    0F
         DC    X'F3'                       Test Num
         DC    XL3'00'
*
         DC    AL1(62)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(062)               Op-1 SS & length
         DC    A(COP2A),A(062)               OP-2 SS & length
*                                          Target
         DC    A(13*MB+(3*K32)-96),A(2048)     Op-1 & length
         DC    A(14*MB+(3*K32)-128),A(2048)    Op-2 & length
*
         DC    A(7) CC0                    Fail mask
*                                          Ending register values
         DC    A(13*MB+(3*K32)+(2048-62)-96),A(062)    OP-1
         DC    A(14*MB+(3*K32)+(2048-62)-128),A(062)   OP-2
                                                                SPACE 2
PTF4     DS    0F
         DC    X'F4'                       Test Num
         DC    XL3'00'
*
         DC    AL1(32)                     SS Length
         DC    X'00'                       Pad Byte
         DC    X'77'                       First-Operand SS last byte
         DC    X'77'                       Second-Operand SS last byte
*                                          Source
         DC    A(COP1A),A(032)               Op-1 SS & length
         DC    A(COP2A),A(032)               OP-2 SS & length
*                                          Target
         DC    A(13*MB+(4*K32)-96),A(4096-128)     Op-1 & length
         DC    A(14*MB+(4*K32)-128),A(4096-128)    Op-2 & length
*
         DC    A(6)   not CC0 or CC3       Fail mask
*                                          Ending register values
         DC    A(13*MB+(4*K32)+(4096-128-32)-96),A(032)     OP-1
         DC    A(14*MB+(4*K32)+(4096-128-32)-128),A(032)    OP-2
                                                                SPACE 3
         DC    A(0)     end of table
         DC    A(0)     end of table
                                                                EJECT
***********************************************************************
*        CUSE Operand-1 scan data...
***********************************************************************
                                                                SPACE
         DS    0F
         DC    2048XL4'98765432'
COP1A    DC    256XL4'111111F0'
                                                                SPACE
         DS    0F
         DC    2048XL4'98765432'
COP1B    DC    256XL4'40404040'
                                                                SPACE
         DS    0F
         DC    2048XL4'11223344'
COP1C    DC    256XL4'40404040'
                                                                SPACE
         DS    0F
         DC    2048XL4'11223344'
COP1D    DC    256XL4'40404040'
                                                                SPACE 4
***********************************************************************
*        CUSE Operand-2 scan data
***********************************************************************
                                                                SPACE
         DS    0F
         DC    2048XL4'89ABCDEF'
COP2A    DC    256XL4'111111F0'
                                                                SPACE
         DS    0F
         DC    2048XL4'89ABCDEF'
COP2B    DC    256XL4'40404040'
                                                                SPACE
         DS    0F
         DC    2048XL4'FF1223344'
COP2C    DC    256XL4'40404040'
                                                                SPACE
         DS    0F
         DC    2048XL4'FF223377'
COP2D    DC    256XL4'40404040'
                                                                SPACE 4
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
