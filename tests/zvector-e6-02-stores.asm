 TITLE 'zvector-e6-02-stores (Zvector E6 VRX stores)'
***********************************************************************
*
*        Zvector E6 instruction tests for VRX encoded:
*
*        E609 VSTEBRH - VECTOR STORE BYTE REVERSED ELEMENT (16)
*        E60A VSTEBRG - VECTOR STORE BYTE REVERSED ELEMENT (64)
*        E60B VSTEBRF - VECTOR STORE BYTE REVERSED ELEMENT (32)
*        E60E VSTBR   - VECTOR STORE BYTE REVERSED ELEMENTS
*        E60F VSTER   - VECTOR STORE ELEMENTS REVERSED
*
*        James Wekel June 2024
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E6 VRX vector
*  store instructions. Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*    *Testcase VECTOR E6 VRX store instructions
*    *
*    *   Zvector E6 instruction tests for VRX encoded:
*    *
*    *   E609 VSTEBRH - VECTOR STORE BYTE REVERSED ELEMENT (16)
*    *   E60A VSTEBRG - VECTOR STORE BYTE REVERSED ELEMENT (64)
*    *   E60B VSTEBRF - VECTOR STORE BYTE REVERSED ELEMENT (32)
*    *   E60E VSTBR   - VECTOR STORE BYTE REVERSED ELEMENTS
*    *   E60F VSTER   - VECTOR STORE ELEMENTS REVERSED
*    *
*    *   # ------------------------------------------------------------
*    *   #  This tests only the basic function of the instruction.
*    *   #  Exceptions are NOT tested.
*    *   # ------------------------------------------------------------
*    *
*    mainsize    2
*    numcpu      1
*    sysclear
*    archlvl     z/Arch
*
*    loadcore    "$(testpath)/zvector-e6-02-stores.core" 0x0
*
*    diag8cmd    enable    # (needed for messages to Hercules console)
*    runtest     2
*    diag8cmd    disable   # (reset back to default)
*
*    *Done
*
***********************************************************************
                                                                SPACE 2
ZVE6TST  START 0
         USING ZVE6TST,R0            Low core addressability

SVOLDPSW EQU   ZVE6TST+X'140'        z/Arch Supervisor call old PSW
                                                                SPACE 2
         ORG   ZVE6TST+X'1A0'        z/Architecure RESTART PSW
         DC    X'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE 2
         ORG   ZVE6TST+X'1D0'        z/Architecure PROGRAM CHECK PSW
         DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                SPACE 3
         ORG   ZVE6TST+X'200'        Start of actual test program...
                                                                EJECT

***********************************************************************
*               The actual "ZVE6TST" program itself...
***********************************************************************
*
*  Architecture Mode: z/Arch
*  Register Usage:
*
*   R0       (work)
*   R1-4     (work)
*   R5       Testing control table - current test base
*   R6-R7    (work)
*   R8       First base register
*   R9       Second base register
*   R10      E6TESTS register
*   R11      E6TEST call return
*   R12-R13  (work)
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

         STCTL R0,R0,CTLR0      Store CR0 to enable AFP
         OI    CTLR0+1,X'04'    Turn on AFP bit
         OI    CTLR0+1,X'02'    Turn on Vector bit
         LCTL  R0,R0,CTLR0      Reload updated CR0

*
         LA    R10,E6TESTS       get table of test addresses

NEXTE6   EQU   *
         L     R5,0(0,R10)       get test address
         LTR   R5,R5                have a test?
         BZ    ENDTEST                 done?

         USING E6TEST,R5
         MVC   V1OUTPUT,V1FUDGE  pollute v1 output (stored)
         VL    V1,V1INPUT
         L     R11,TSUB          get address of test routine
         BALR  R11,R11           do test
         CLC   V1OUTPUT,RESULT    valid?
         BNE   FAILMSG              no, issue failed message

         LA    R10,4(0,R10)      next test address
         B     NEXTE6

*
* test failed: issue message with test number, instruction under test
*              and instruction m3
*
FAILMSG  EQU   *
         BAL   R15,RPTERROR
         L     R0,=F'1'          set failed test indicator
         ST    R0,FAILED

         LA    R10,4(0,R10)      next test address
         B     NEXTE6

ENDTEST  EQU   *
         L     R1,FAILED         did a test fail?
         LTR   R1,R1
         BZ    EOJ                  No, exit
         B     FAILTEST             Yes, exit with BAD PSW

                                                               EJECT
***********************************************************************
*        RPTERROR                 Report instruction test in error
***********************************************************************
                                                               SPACE
RPTERROR ST    R15,RPTSAVE          Save return address
         ST    R5,RPTSVR5           Save R5
*
         LH    R2,TNUM              get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTNUM(3),PRT3+13    fill in message with test #

         MVC   PRTNAME,OPNAME       fill in message with instruction
*
         LB    R2,M3                get m3 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM3(1),PRT3+15     fill in message with m3 field
                                                               SPACE
*
*        Use Hercules Diagnose for Message to console
*
         STM   R0,R2,RPTDWSAV       save regs used by MSG
         LA    R0,PRTLNG            message length
         LA    R1,PRTLINE           messagfe address
         BAL   R2,MSG               call Hercules console MSG display
         LM    R0,R2,RPTDWSAV       restore regs
                                                               SPACE 2
         L     R5,RPTSVR5         Restore R5
         L     R15,RPTSAVE        Restore return address
         BR    R15                Return to caller
                                                               SPACE
RPTSAVE  DC    F'0'               R15 save area
RPTSVR5  DC    F'0'               R5 save area
                                                               SPACE
RPTDWSAV DC    2D'0'              R0-R2 save area for MSG call
                                                               EJECT
***********************************************************************
*        Issue HERCULES MESSAGE pointed to by R1, length in R0
*              R2 = return address
***********************************************************************
                                                                SPACE
MSG      CH    R0,=H'0'               Do we even HAVE a message?
         BNHR  R2                     No, ignore
                                                                SPACE
         STM   R0,R2,MSGSAVE          Save registers
                                                                SPACE
         CH    R0,=AL2(L'MSGMSG)      Message length within limits?
         BNH   MSGOK                  Yes, continue
         LA    R0,L'MSGMSG            No, set to maximum
                                                                SPACE
MSGOK    LR    R2,R0                  Copy length to work register
         BCTR  R2,0                   Minus-1 for execute
         EX    R2,MSGMVC              Copy message to O/P buffer
                                                                SPACE
         LA    R2,1+L'MSGCMD(,R2)     Calculate true command length
         LA    R1,MSGCMD              Point to true command
                                                                SPACE
         DC    X'83',X'12',X'0008'    Issue Hercules Diagnose X'008'
         BZ    MSGRET                 Return if successful
         DC    H'0'                   CRASH for debugging purposes
                                                                SPACE
MSGRET   LM    R0,R2,MSGSAVE          Restore registers
         BR    R2                     Return to caller
                                                                SPACE 6
MSGSAVE  DC    3F'0'                  Registers save area
MSGMVC   MVC   MSGMSG(0),0(R1)        Executed instruction
                                                                SPACE 2
MSGCMD   DC    C'MSGNOH * '           *** HERCULES MESSAGE COMMAND ***
MSGMSG   DC    CL95' '                The message text to be displayed

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
CTLR0    DS    F                CR0
         DS    F
                                                                SPACE 2
         LTORG ,                Literals pool

*        some constants

K        EQU   1024             One KB
PAGE     EQU   (4*K)            Size of one page
K64      EQU   (64*K)           64 KB
MB       EQU   (K*K)             1 MB

REG2PATT EQU   X'AABBCCDD'    Polluted Register pattern
REG2LOW  EQU         X'DD'    (last byte above)
                                                                EJECT
*======================================================================
*
*  NOTE: start data on an address that is easy to display
*        within Hercules
*
*======================================================================

         ORG   ZVE6TST+X'1000'
FAILED   DC    F'0'                     some test failed?
                                                               SPACE 2
*
*        failed message and associated editting
*
PRTLINE  DC    C'         Test # '
PRTNUM   DC    C'xxx'
         DC    c' failed for instruction '
PRTNAME  DC    CL8'xxxxxxxx'
         DC    C' with m3='
PRTM3    DC    C'x'
         DC    C'.'
PRTLNG   EQU   *-PRTLINE
EDIT     DC    XL18'402120202020202020202020202020202020'

         DC    C'===>'
PRT3     DC    CL18' '
         DC    C'<==='
DECNUM   DS    CL16
                                                               SPACE 2
*
*        Vector instruction results, pollution and input
*
         DS    0F
V1OUTPUT DS    XL16                                      V1 OUTPUT
         DS    XL16                                        gap
V1FUDGE  DC    XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    V1 FUDGE
V1INPUT  DC    XL16'00010203040506070809101112131415'    V1 input
         DS    XL16                                         gap
                                                                EJECT
***********************************************************************
*        E6TEST DSECT
***********************************************************************
                                                                SPACE 2
E6TEST   DSECT ,
TSUB     DC    A(0)           pointer  to test
TNUM     DC    H'00'          Test Number
         DC    X'00'
M3       DC    X'00'          M3 used

OPNAME   DC    CL8' '         E6 name
RELEN    DC    A(0)           RESULT LENGTH
RESULT   DC    A(0)
*        EXPECTED RESULT
**
*        test routine will be here (from VRX macro)
                                                                SPACE 2
ZVE6TST  CSECT ,
         DS    0F
                                                                SPACE 2
***********************************************************************
*     Macros to help build test tables
***********************************************************************
                                                                SPACE 3
*
* macro to generate individual test
*
         MACRO
         VRX   &INST,&M3,&RESULT
.*                               &INST    - VRX instruction under test
.*                               &M3      - m3 field
.*                               &RESULT  - XL16 result field
         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
         DC    X'&M3'            M3
         DC    CL8'&INST'        instruction name
         DC    A(X&TNUM-RE&TNUM)  result length
RE&TNUM  DC    &RESULT           expected result
.*
*
         DS    0F
X&TNUM   &INST V1,V1OUTPUT,&M3   test instruction
         BR    R11               return

         DROP  R5
.*
         MEND
                                                               SPACE 3
*
* macro to generate table of pointers to individual tests
*
         MACRO
         PTTABLE
         GBLA  &TNUM
         LCLA  &CUR
&CUR     SETA  1
.*
TTABLE   DS    0F
.LOOP    ANOP
.*
         DC    A(T&CUR)          TEST &CUR
.*
&CUR     SETA  &CUR+1
         AIF   (&CUR LE &TNUM).LOOP
*
         DC    A(0)              END OF TABLE
         DC    A(0)
.*
         MEND
                                                                EJECT
***********************************************************************
*        E6 VRX tests
***********************************************************************
         PRINT DATA

*        E609 VSTEBRH - VECTOR STORE BYTE REVERSED ELEMENT (16)
*        E60A VSTEBRG - VECTOR STORE BYTE REVERSED ELEMENT (64)
*        E60B VSTEBRF - VECTOR STORE BYTE REVERSED ELEMENT (32)
*        E60E VSTBR   - VECTOR STORE BYTE REVERSED ELEMENTS
*        E60F VSTER   - VECTOR STORE ELEMENTS REVERSED

*        VRX   instruction, m3, 16 byte expected result
         VRX   VSTEBRH,0,XL16'0100FFFFFFFFFFFFFFFFFFFFFFFFFFFF'
         VRX   VSTEBRH,1,XL16'0302FFFFFFFFFFFFFFFFFFFFFFFFFFFF'
         VRX   VSTEBRH,2,XL16'0504FFFFFFFFFFFFFFFFFFFFFFFFFFFF'
         VRX   VSTEBRH,3,XL16'0706FFFFFFFFFFFFFFFFFFFFFFFFFFFF'
         VRX   VSTEBRH,4,XL16'0908FFFFFFFFFFFFFFFFFFFFFFFFFFFF'
         VRX   VSTEBRH,5,XL16'1110FFFFFFFFFFFFFFFFFFFFFFFFFFFF'
         VRX   VSTEBRH,6,XL16'1312FFFFFFFFFFFFFFFFFFFFFFFFFFFF'
         VRX   VSTEBRH,7,XL16'1514FFFFFFFFFFFFFFFFFFFFFFFFFFFF'
*
         VRX   VSTEBRG,0,XL16'0706050403020100FFFFFFFFFFFFFFFF'
         VRX   VSTEBRG,1,XL16'1514131211100908FFFFFFFFFFFFFFFF'
*
         VRX   VSTEBRF,0,XL16'03020100FFFFFFFFFFFFFFFFFFFFFFFF'
         VRX   VSTEBRF,1,XL16'07060504FFFFFFFFFFFFFFFFFFFFFFFF'
         VRX   VSTEBRF,2,XL16'11100908FFFFFFFFFFFFFFFFFFFFFFFF'
         VRX   VSTEBRF,3,XL16'15141312FFFFFFFFFFFFFFFFFFFFFFFF'
*
         VRX   VSTBR,1,XL16'01000302050407060908111013121514'
         VRX   VSTBR,2,XL16'03020100070605041110090815141312'
         VRX   VSTBR,3,XL16'07060504030201001514131211100908'
         VRX   VSTBR,4,XL16'15141312111009080706050403020100'
*
         VRX   VSTER,1,XL16'14151213101108090607040502030001'
         VRX   VSTER,2,XL16'12131415080910110405060700010203'
         VRX   VSTER,3,XL16'08091011121314150001020304050607'

         DC    F'0'     END OF TABLE
         DC    F'0'
*
* table of pointers to individual load test
*
E6TESTS  DS    0F
         PTTABLE

         DC    F'0'     END OF TABLE
         DC    F'0'
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
***********************************************************************
*        Register equates
***********************************************************************
                                                                SPACE 2
V0       EQU   0
V1       EQU   1
V2       EQU   2
V3       EQU   3
V4       EQU   4
V5       EQU   5
V6       EQU   6
V7       EQU   7
V8       EQU   8
V9       EQU   9
V10      EQU   10
V11      EQU   11
V12      EQU   12
V13      EQU   13
V14      EQU   14
V15      EQU   15
V16      EQU   16
V17      EQU   17
V18      EQU   18
V19      EQU   19
V20      EQU   20
V21      EQU   21
V22      EQU   22
V23      EQU   23
V24      EQU   24
V25      EQU   25
V26      EQU   26
V27      EQU   27
V28      EQU   28
V29      EQU   29
V30      EQU   30
V31      EQU   31

         END
