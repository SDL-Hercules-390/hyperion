 TITLE 'zvector-e6-03-pack (Zvector E6 VSI pack/load)'
***********************************************************************
*
*        Zvector E6 instruction tests for VSI encoded:
*
*        E634 VPKZ    - VECTOR PACK ZONED
*        E635 VLRL    - VECTOR LOAD RIGHTMOST WITH LENGTH
*
*        James Wekel June 2024
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E6 VSI vector
*  pack zones and load rightmost instructions.
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*    *Testcase VECTOR E6 VSI pack/load instructions
*    *
*    *   Zvector E6 instruction tests for VSI encoded:
*    *
*    *   E634 VPKZ    - VECTOR PACK ZONED
*    *   E635 VLRL    - VECTOR LOAD RIGHTMOST WITH LENGTH
*    *
*    *   # ------------------------------------------------------------
*    *   #  This tests only the basic function of the instruction.
*    *   #  Specification Exceptions are NOT tested.
*    *   # ------------------------------------------------------------
*    *
*    mainsize    2
*    numcpu      1
*    sysclear
*    archlvl     z/Arch
*
*    loadcore    "$(testpath)/zvector-e6-03-pack.core" 0x0
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
         VL    V1,V1FUDGE
         L     R11,TSUB          get address of test routine
         BALR  R11,R11           do test
         VST   V1,V1OUTPUT
         CLC   V1OUTPUT,RESULT    valid?
         BNE   FAILMSG              no, issue failed message

         LA    R10,4(0,R10)      next test address
         B     NEXTE6

*
* test failed: issue message with test number, instruction under test
*              and instruction I3
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
         LB    R2,I3                get I3 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTI3(2),PRT3+14     fill in message with i3 field
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
         DC    C' with i3='
PRTI3    DC    C'x'
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
V1INPUT  DC    CL16'1234567890123456'                    V1 input
         DC    CL14'78901234567890'
         DC    X'D9'
         DS    XL16
                                                                EJECT
***********************************************************************
*        E6TEST DSECT
***********************************************************************
                                                                SPACE 2
E6TEST   DSECT ,
TSUB     DC    A(0)           pointer  to test
TNUM     DC    H'00'          Test Number
         DC    X'00'
I3       DC    HL1'00'        I3 used

OPNAME   DC    CL8' '         E6 name
RELEN    DC    A(0)           RESULT LENGTH
RESULT   DC    A(0)
*        EXPECTED RESULT
**
*        test routine will be here (from VSI macro)
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
         VSI   &INST,&I3,&RESULT
.*                               &INST   - VSI instruction under test
.*                               &i3     - i3 field
.*                               &RESULT - XL16 result field
         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
         DC    HL1'&I3'          i3
         DC    CL8'&INST'        instruction name
         DC    A(X&TNUM-RE&TNUM) result length
RE&TNUM  DC    &RESULT           expected result
.*
*
X&TNUM   DS    0F
         &INST V1,V1INPUT,&I3    test instruction
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
*        E6 VSI tests
***********************************************************************
         PRINT DATA

*        E634 VPKZ    - VECTOR PACK ZONED
*        E635 VLRL    - VECTOR LOAD RIGHTMOST WITH LENGTH

*        VSI   instruction, i3, 16 byte expected result
         VSI   VPKZ,00,XL16'0000000000000000000000000000001F'
         VSI   VPKZ,01,XL16'0000000000000000000000000000012F'
         VSI   VPKZ,02,XL16'0000000000000000000000000000123F'
         VSI   VPKZ,03,XL16'0000000000000000000000000001234F'
         VSI   VPKZ,04,XL16'0000000000000000000000000012345F'
         VSI   VPKZ,05,XL16'0000000000000000000000000123456F'
         VSI   VPKZ,06,XL16'0000000000000000000000001234567F'
         VSI   VPKZ,07,XL16'0000000000000000000000012345678F'
         VSI   VPKZ,08,XL16'0000000000000000000000123456789F'
         VSI   VPKZ,09,XL16'0000000000000000000001234567890F'
         VSI   VPKZ,10,XL16'0000000000000000000012345678901F'
         VSI   VPKZ,11,XL16'0000000000000000000123456789012F'
         VSI   VPKZ,12,XL16'0000000000000000001234567890123F'
         VSI   VPKZ,13,XL16'0000000000000000012345678901234F'
         VSI   VPKZ,14,XL16'0000000000000000123456789012345F'
         VSI   VPKZ,15,XL16'0000000000000001234567890123456F'
         VSI   VPKZ,16,XL16'0000000000000012345678901234567F'
         VSI   VPKZ,17,XL16'0000000000000123456789012345678F'
         VSI   VPKZ,18,XL16'0000000000001234567890123456789F'
         VSI   VPKZ,19,XL16'0000000000012345678901234567890F'
         VSI   VPKZ,20,XL16'0000000000123456789012345678901F'
         VSI   VPKZ,21,XL16'0000000001234567890123456789012F'
         VSI   VPKZ,22,XL16'0000000012345678901234567890123F'
         VSI   VPKZ,23,XL16'0000000123456789012345678901234F'
         VSI   VPKZ,24,XL16'0000001234567890123456789012345F'
         VSI   VPKZ,25,XL16'0000012345678901234567890123456F'
         VSI   VPKZ,26,XL16'0000123456789012345678901234567F'
         VSI   VPKZ,27,XL16'0001234567890123456789012345678F'
         VSI   VPKZ,28,XL16'0012345678901234567890123456789F'
         VSI   VPKZ,29,XL16'0123456789012345678901234567890F'
         VSI   VPKZ,30,XL16'1234567890123456789012345678909D'  note: D

         VSI   VLRL,00,XL16'000000000000000000000000000000F1'
         VSI   VLRL,01,XL16'0000000000000000000000000000F1F2'
         VSI   VLRL,02,XL16'00000000000000000000000000F1F2F3'
         VSI   VLRL,03,XL16'000000000000000000000000F1F2F3F4'
         VSI   VLRL,04,XL16'0000000000000000000000F1F2F3F4F5'
         VSI   VLRL,05,XL16'00000000000000000000F1F2F3F4F5F6'
         VSI   VLRL,06,XL16'000000000000000000F1F2F3F4F5F6F7'
         VSI   VLRL,07,XL16'0000000000000000F1F2F3F4F5F6F7F8'
         VSI   VLRL,08,XL16'00000000000000F1F2F3F4F5F6F7F8F9'
         VSI   VLRL,09,XL16'000000000000F1F2F3F4F5F6F7F8F9F0'
         VSI   VLRL,10,XL16'0000000000F1F2F3F4F5F6F7F8F9F0F1'
         VSI   VLRL,11,XL16'00000000F1F2F3F4F5F6F7F8F9F0F1F2'
         VSI   VLRL,12,XL16'000000F1F2F3F4F5F6F7F8F9F0F1F2F3'
         VSI   VLRL,13,XL16'0000F1F2F3F4F5F6F7F8F9F0F1F2F3F4'
         VSI   VLRL,14,XL16'00F1F2F3F4F5F6F7F8F9F0F1F2F3F4F5'
         VSI   VLRL,15,XL16'F1F2F3F4F5F6F7F8F9F0F1F2F3F4F5F6'


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
