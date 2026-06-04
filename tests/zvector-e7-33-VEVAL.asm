 TITLE 'zvector-e7-33-VEVAL'
***********************************************************************
*
*   Zvector E7 instruction tests for VRI-k encoded:
*
*   E788 VEVAL  - Vector Evaluate
*
*        James Wekel March 2026
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E7 VRI-k
*  Vector Evaluate instruction.
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*    *Testcase zzvector-e7-33-VEVAL
*    *
*    *   Zvector E7 instruction tests for VRI-k encoded:
*    *
*    *   E788 VEVAL  - Vector Evaluate
*    **
*    *   # ------------------------------------------------------------
*    *   #  This tests only the basic function of the instruction.
*    *   #  Exceptions are NOT tested.
*    *   # ------------------------------------------------------------
*    *
*    mainsize    1
*    numcpu      1
*    sysclear
*    archlvl     z/Arch
*
*    loadcore    "$(testpath)/zvector-e7-33-VEVAL.core" 0x0
*
*    diag8cmd    enable    # (needed for messages to Hercules console)
*    runtest     5         #
*    diag8cmd    disable   # (reset back to default)
*
*    *Done
*
***********************************************************************
                                                                EJECT
***********************************************************************
*        FCHECK Macro - Is a Facility Bit set?
*
*        If the facility bit is NOT set, an message is issued and
*        the test is skipped.
*
*        Fcheck uses R0, R1 and R2
*
* eg.    FCHECK 134,'vector-packed-decimal'
***********************************************************************
         MACRO
         FCHECK &BITNO,&NOTSETMSG
.*                        &BITNO : facility bit number to check
.*                        &NOTSETMSG : 'facility name'
         LCLA  &FBBYTE           Facility bit in Byte
         LCLA  &FBBIT            Facility bit within Byte

         LCLA  &L(8)
&L(1)    SetA  128,64,32,16,8,4,2,1  bit positions within byte

&FBBYTE  SETA  &BITNO/8
&FBBIT   SETA  &L((&BITNO-(&FBBYTE*8))+1)
.*       MNOTE 0,'checking Bit=&BITNO: FBBYTE=&FBBYTE, FBBIT=&FBBIT'

         B     X&SYSNDX
*                                      Fcheck data area
*                                      skip messgae
SKT&SYSNDX DC  C'    Skipping tests: '
         DC    C&NOTSETMSG
         DC    C' (bit &BITNO) is not installed.'
SKL&SYSNDX EQU *-SKT&SYSNDX
*                                      facility bits
         DS    FD                      gap
FB&SYSNDX DS   4FD
         DS    FD                      gap
*
X&SYSNDX EQU *
         LA    R0,((X&SYSNDX-FB&SYSNDX)/8)-1
         STFLE FB&SYSNDX               get facility bits

         XGR   R0,R0
         IC    R0,FB&SYSNDX+&FBBYTE    get fbit byte
         N     R0,=F'&FBBIT'           is bit set?
         BNZ   XC&SYSNDX
*
* facility bit not set, issue message and exit
*
         LA    R0,SKL&SYSNDX           message length
         LA    R1,SKT&SYSNDX           message address
         BAL   R2,MSG

         B     EOJ
XC&SYSNDX EQU *
         MEND
                                                                 EJECT
***********************************************************************
* (pending     E788 VEVAL  - Vector Evaluate
*  inclusion in SATK ASAM)
*
*     VEVAL Macro to help build VEVAL instruction
*        VEVAL   i5
*
*        Note: v1, v2, v3, v4 are fixed vector registers:
*                         v1 = 1; v2 = 2, v3 = 3, v4 = 4
***********************************************************************
         MACRO
         VEVAL   &I5
.*                                     &i5  - i5 for VEVAL instruction
         LCLA  &II5
&II5     SETA  +(+&I5)
                                                               SPACE 1
         DS    0H                      E788 VEVAL
         DC    X'E7'                   - Vector Evaluate
         DC    X'12'                    v1, v2
         DC    X'30'                    v3, reserved
         DC    HL1'&II5'                i5
         DC    x'40'                    v4, RXB
         DC    X'88'
                                                               SPACE 1
         MEND
                                                                EJECT
***********************************************************************
*        Low core PSWs
***********************************************************************
ZVE7TST  START 0
         USING ZVE7TST,R0            Low core addressability

SVOLDPSW EQU   ZVE7TST+X'140'        z/Arch Supervisor call old PSW
                                                                SPACE 2
         ORG   ZVE7TST+X'1A0'        z/Architecure RESTART PSW
         DC    X'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE 2
         ORG   ZVE7TST+X'1D0'        z/Architecure PROGRAM CHECK PSW
         DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                SPACE 3
         ORG   ZVE7TST+X'200'        Start of actual test program...
                                                                SPACE 2
***********************************************************************
*               The actual "ZVE7TST" program itself...
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
*   R10      Third base register
*   R11      E7TEST call return
*   R12      E7TESTS register
*   R13      (work)
*   R14      Subroutine call
*   R15      Secondary Subroutine call or work
*
***********************************************************************
                                                                SPACE
         USING  BEGIN,R8        FIRST  Base Register
         USING  BEGIN+4096,R9   SECOND Base Register
         USING  BEGIN+8192,R10  THIRD  Base Register
                                                                SPACE
BEGIN    BALR  R8,0             Initalize FIRST base register
         BCTR  R8,0             Initalize FIRST base register
         BCTR  R8,0             Initalize FIRST base register
                                                                SPACE
         LA    R9,2048(,R8)     Initalize SECOND base register
         LA    R9,2048(,R9)     Initalize SECOND base register

         LA    R10,2048(,R9)    Initalize THIRD base register
         LA    R10,2048(,R10)   Initalize THIRD base register

         STCTL R0,R0,CTLR0      Store CR0 to enable AFP
         OI    CTLR0+1,X'04'    Turn on AFP bit
         OI    CTLR0+1,X'02'    Turn on Vector bit
         LCTL  R0,R0,CTLR0      Reload updated CR0

***********************************************************************
* Is z/Architecture Vector-enhancements facility 3 installed  (bit 198)
***********************************************************************

         FCHECK 198,'Vector-enhancements facility 3'
                                                                EJECT
***********************************************************************
*              Do tests in the E7TESTS table
***********************************************************************

         L     R12,=A(E7TESTS)       get table of test addresses

NEXTE7   EQU   *
         L     R5,0(0,R12)       get test address
         LTR   R5,R5                have a test?
         BZ    ENDTEST                 done?

         USING E7TEST,R5

         LH    R0,TNUM           save current test number
         ST    R0,TESTING        for easy reference

         VL    V1,V1FUDGE
         L     R11,TSUB          get address of test routine
         BALR  R11,R11           do test

         LGF   R1,READDR         get address of expected result
         CLC   V1OUTPUT,0(R1)    valid?
         BNE   FAILMSG              no, issue failed message

         LA    R12,4(0,R12)      next test address
         B     NEXTE7
                                                                 EJECT
***********************************************************************
* result not as expected:
*        issue message with test number, instruction under test
*              and instruction m4
***********************************************************************
FAILMSG  EQU   *
         BAL   R15,RPTERROR
                                                                SPACE 2
***********************************************************************
* continue after a failed test
***********************************************************************
FAILCONT EQU   *
         L     R0,=F'1'          set failed test indicator
         ST    R0,FAILED

         LA    R12,4(0,R12)      next test address
         B     NEXTE7
                                                                SPACE 2
***********************************************************************
* end of testing; set ending psw
***********************************************************************
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
         XGR   R2,R2
         IC    R2,I5                get i5 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTI5(3),PRT3+13     fill in message with i5 field
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

         LTR   R2,R2                  Is Diag8 Ry (R2) 0?
         BZ    MSGRET                   an error occurred but coninue

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
                                                                SPACE
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

         ORG   ZVE7TST+X'1000'
FAILED   DC    F'0'                     some test failed?
TESTING  DC    F'0'                     current test number
                                                               SPACE 2
*
*        failed message and associated editting
*
PRTLINE  DC    C'         Test # '
PRTNUM   DC    C'xxx'
         DC    c' failed for instruction '
PRTNAME  DC    CL8'xxxxxxxx'
         DC    C' with i5='
PRTI5    DC    C'xxx'
         DC    C'.'
PRTLNG   EQU   *-PRTLINE
                                                               EJECT
***********************************************************************
*        TEST failed : message working storge
***********************************************************************
EDIT     DC    XL18'402120202020202020202020202020202020'

         DC    C'===>'
PRT3     DC    CL18' '
         DC    C'<==='
DECNUM   DS    CL16
                                                                SPACE 2
***********************************************************************
*        Vector instruction results, pollution and input
***********************************************************************
         DS    0F
         DS    XL16                        gap
V1FUDGE  DC    CL16'-FudgeFudgeFudge'  V1 FUDGE
         DS    XL16                        gap
                                                                EJECT
***********************************************************************
*        E7TEST DSECT
***********************************************************************
                                                                SPACE 2
E7TEST   DSECT ,
TSUB     DC    A(0)           pointer  to test
TNUM     DC    H'00'          Test Number
         DC    X'00'
I5       DC    HL1'00'        m5 used

OPNAME   DC    CL8' '         E7 name
V2ADDR   DC    A(0)           address of v2 source
V3ADDR   DC    A(0)           address of v3 source
V4ADDR   DC    A(0)           address of v4 source
RELEN    DC    A(0)           RESULT LENGTH
READDR   DC    A(0)           result (expected) address
         DS    FD                gap
V1OUTPUT DS    XL16           V1 Output
         DS    FD                gap

*        test routine will be here (from VRI-k macro)
*
*        followed by
*              EXPECTED RESULT
                                                                SPACE 2
ZVE7TST  CSECT ,
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
         VRI_K &INST,&I5
.*                               &INST   - VRI-k instruction under test
.*                               &i5     - i5 field

         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
         DC    HL1'&I5'          i5
         DC    CL8'&INST'        instruction name
         DC    A(RE&TNUM+16)     address of v2 source
         DC    A(RE&TNUM+32)     address of v3 source
         DC    A(RE&TNUM+48)     address of v4 source
         DC    A(16)             result length
REA&TNUM DC    A(RE&TNUM)        result address
         DS    FD                gap
V1O&TNUM DS    XL16              V1 output
         DS    FD                gap
.*
*
X&TNUM   DS    0F
         XGR   R1,R1
         LA    R1,V1FUDGE        load v1 fudge
         VL    v1,0(R1)          use v21 to test decoder

         LGF   R1,V2ADDR         load v2 source
         VL    v2,0(R1)          use v22 to test decoder

         LGF   R1,V3ADDR         load v3 source
         VL    v3,0(R1)          use v23 to test decoder

         LGF   R1,V4ADDR         load v4 source
         VL    v4,0(R1)          use v24 to test decoder

         &INST &I5               test instruction
         VST   V1,V1O&TNUM       save v1 output

         BR    R11               return

RE&TNUM  DC    0F                xl16 expected result

         DROP  R5
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
         DC    A(T&CUR)
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
*        E7 VRI-k tests
***********************************************************************
         PRINT DATA
         DS    FD
*
*   E788 VEVAL  - Vector Evaluate
*
*        VRI-k instruction, I5
*              followed by
*                 16 byte expected result (V1)
*                 16 byte V2 source
*                 16 byte V3 source
*                 16 byte V4 source
*---------------------------------------------------------------------
*  VEVAL  - Vector Evaluate
*---------------------------------------------------------------------
* Test boolean functions defined in PoP SA22-7832-14,
* Figure 22-3. Boolean operations, paged 22-14.
*
* Row 0
* i5 = 1 :     AND(A,B,C)
         VRI_K VEVAL,1
         DC    XL16'11111111 00000000 00000000 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,1
         DC    XL16'22222222 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 6 :     AND(A,XOR(B,C))
         VRI_K VEVAL,6
         DC    XL16'22222222 44444444 99999999 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,6
         DC    XL16'11111111 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 7 :     AND(A,OR(B,C))
         VRI_K VEVAL,7
         DC    XL16'33333333 44444444 99999999 AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,7
         DC    XL16'33333333 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 1
* i5 = 8 :     AND(A,NOR(B,C))
         VRI_K VEVAL,8
         DC    XL16'00000000 11111111 00000000 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,8
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 9 :     AND(A,NXOR(B,C))
         VRI_K VEVAL,9
         DC    XL16'11111111 11111111 00000000 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,9
         DC    XL16'22222222 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 14:     AND(A,NAND(B,C))
         VRI_K VEVAL,14
         DC    XL16'22222222 55555555 99999999 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,14
         DC    XL16'11111111 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v23
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 2
* i5 = 16:     NOR(A,NAND(B,C))
         VRI_K VEVAL,16
         DC    XL16'00000000 22222222 44444444 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,16
         DC    XL16'00000000 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 22:     SEL(A,XOR(B,C),AND(B,C))
         VRI_K VEVAL,22
         DC    XL16'22222222 66666666 DDDDDDDD 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,22
         DC    XL16'11111111 DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 23:     MAJOR(A,B,C)
         VRI_K VEVAL,23
         DC    XL16'33333333 66666666 DDDDDDDD AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,23
         DC    XL16'33333333 DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 3
* i5 = 24:     SEL(A,NOR(B,C),AND(B,C))
         VRI_K VEVAL,24
         DC    XL16'00000000 33333333 44444444 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,24
         DC    XL16'00000000 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 25:     SEL(A,NXOR(B,C),AND(B,C))
         VRI_K VEVAL,25
         DC    XL16'11111111 33333333 44444444 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,25
         DC    XL16'22222222 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 28:     SEL(A,NOT(B),AND(B,C))
         VRI_K VEVAL,28
         DC    XL16'22222222 77777777 DDDDDDDD 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,28
         DC    XL16'00000000 CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 30:     XOR(A,AND(B,C))
         VRI_K VEVAL,30
         DC    XL16'22222222 77777777 DDDDDDDD 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,30
         DC    XL16'11111111 DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 31:     OR(A,AND(B,C))
         VRI_K VEVAL,31
         DC    XL16'33333333 77777777 DDDDDDDD AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,31
         DC    XL16'33333333 DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* Row 4
* Row 5
* Row 6
* Row 7
* Row 8
* Row 9

* Row 10
* i5 = 81:     SEL(A,AND(B,C),C)
         VRI_K VEVAL,81
         DC    XL16'DDDDDDDD AAAAAAAA 44444444 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,81
         DC    XL16'AAAAAAAA 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 11
* i5 = 88:     SEL(A,NOR(B,C),C)
         VRI_K VEVAL,88
         DC    XL16'CCCCCCCC BBBBBBBB 44444444 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,88
         DC    XL16'88888888 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 89:     SEL(A,NXOR(B,C),C)
         VRI_K VEVAL,89
         DC    XL16'DDDDDDDD BBBBBBBB 44444444 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,89
         DC    XL16'AAAAAAAA 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 92:     SEL(A,NOT(B),C)
         VRI_K VEVAL,92
         DC    XL16'EEEEEEEE FFFFFFFF DDDDDDDD 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,92
         DC    XL16'88888888 CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 94:     SEL(A,NAND(B,C),C)
         VRI_K VEVAL,94
         DC    XL16'EEEEEEEE FFFFFFFF DDDDDDDD 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,94
         DC    XL16'99999999 DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 12
* i5 = 96:     NOR(A,NXOR(B,C))
         VRI_K VEVAL,96
         DC    XL16'CCCCCCCC 88888888 00000000 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,96
         DC    XL16'CCCCCCCC 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 97:     SEL(A,AND(B,C),XOR(B,C))
         VRI_K VEVAL,97
         DC    XL16'DDDDDDDD 88888888 00000000 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,97
         DC    XL16'EEEEEEEE 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 99:     SEL(A,B,XOR(B,C))
         VRI_K VEVAL,99
         DC    XL16'DDDDDDDD 88888888 00000000 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,99
         DC    XL16'FFFFFFFF 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 103:    SEL(A,OR(B,C),XOR(B,C))
         VRI_K VEVAL,103
         DC    XL16'FFFFFFFF CCCCCCCC 99999999 BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,103
         DC    XL16'FFFFFFFF 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 13
* i5 = 104:    SEL(A,NOR(B,C),XOR(B,C))
         VRI_K VEVAL,104
         DC    XL16'CCCCCCCC 99999999 00000000 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,104
         DC    XL16'CCCCCCCC 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 105:    XOR(A,B,C)
         VRI_K VEVAL,105
         DC    XL16'DDDDDDDD 99999999 00000000 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,105
         DC    XL16'EEEEEEEE 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 111:    OR(A,XOR(B,C))
         VRI_K VEVAL,111
         DC    XL16'FFFFFFFF DDDDDDDD 99999999 BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,111
         DC    XL16'FFFFFFFF 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 14
* i5 = 112:    NOR(A,NOR(B,C))
         VRI_K VEVAL,112
         DC    XL16'CCCCCCCC AAAAAAAA 44444444 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,112
         DC    XL16'CCCCCCCC AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 113:    SEL(A,AND(B,C),OR(B,C))
         VRI_K VEVAL,113
         DC    XL16'DDDDDDDD AAAAAAAA 44444444 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,113
         DC    XL16'EEEEEEEE AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 115:    SEL(A,B,OR(B,C))
         VRI_K VEVAL,115
         DC    XL16'DDDDDDDD AAAAAAAA 44444444 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,115
         DC    XL16'FFFFFFFF BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 15
* i5 = 120:    XOR(A,OR(B,C))
         VRI_K VEVAL,120
         DC    XL16'CCCCCCCC BBBBBBBB 44444444 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,120
         DC    XL16'CCCCCCCC AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 121:    SEL(A,NXOR(B,C),OR(B,C))
         VRI_K VEVAL,121
         DC    XL16'DDDDDDDD BBBBBBBB 44444444 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,121
         DC    XL16'EEEEEEEE AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 124:    SEL(A,NOT(B),OR(B,C))
         VRI_K VEVAL,124
         DC    XL16'EEEEEEEE FFFFFFFF DDDDDDDD 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,124
         DC    XL16'CCCCCCCC EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 126:    SEL(A,NAND(B,C),OR(B,C))
         VRI_K VEVAL,126
         DC    XL16'EEEEEEEE FFFFFFFF DDDDDDDD 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,126
         DC    XL16'DDDDDDDD FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 127:    OR(A,B,C)
         VRI_K VEVAL,127
         DC    XL16'FFFFFFFF FFFFFFFF DDDDDDDD BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,127
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 16
* i5 = 128:    NOR(A,B,C)
         VRI_K VEVAL,128
         DC    XL16'00000000 00000000 22222222 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,128
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 129:    SEL(A,AND(B,C),NOR(B,C))
         VRI_K VEVAL,129
         DC    XL16'11111111 00000000 22222222 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,129
         DC    XL16'22222222 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 131:    SEL(A,B,NOR(B,C))
         VRI_K VEVAL,131
         DC    XL16'11111111 00000000 22222222 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,131
         DC    XL16'33333333 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 134:    SEL(A,XOR(B,C),NOR(B,C))
         VRI_K VEVAL,134
         DC    XL16'22222222 44444444 BBBBBBBB 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,134
         DC    XL16'11111111 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 135:    NXOR(A,OR(B,C))
         VRI_K VEVAL,135
         DC    XL16'33333333 44444444 BBBBBBBB EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,135
         DC    XL16'33333333 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 17
* i5 = 140:    SEL(A,NOT(B),NOR(B,C))
         VRI_K VEVAL,140
         DC    XL16'22222222 55555555 BBBBBBBB 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,140
         DC    XL16'00000000 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 142:    SEL(A,NAND(B,C),NOR(B,C))
         VRI_K VEVAL,142
         DC    XL16'22222222 55555555 BBBBBBBB 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,142
         DC    XL16'11111111 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 143:    OR(A,NOR(B,C))
         VRI_K VEVAL,143
         DC    XL16'33333333 55555555 BBBBBBBB EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,143
         DC    XL16'33333333 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 18
* i5 = 144:    NOR(A,XOR(B,C))
         VRI_K VEVAL,144
         DC    XL16'00000000 22222222 66666666 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,144
         DC    XL16'00000000 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 150:    NXOR(A,B,C)
         VRI_K VEVAL,150
         DC    XL16'22222222 66666666 FFFFFFFF 66666666'
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,150
         DC    XL16'11111111 DDDDDDDD 77777777 DDDDDDDD'
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 151:    SEL(A,OR(B,C),NXOR(B,C))
         VRI_K VEVAL,151
         DC    XL16'33333333 66666666 FFFFFFFF EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,151
         DC    XL16'33333333 DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 19
* i5 = 152:    SEL(A,NOR(B,C),NXOR(B,C))
         VRI_K VEVAL,152
         DC    XL16'00000000 33333333 66666666 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,152
         DC    XL16'00000000 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 156:    SEL(A,NOT(B),NXOR(B,C))
         VRI_K VEVAL,156
         DC    XL16'22222222 77777777 FFFFFFFF 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,156
         DC    XL16'00000000 CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 158:    SEL(A,NAND(B,C),NXOR(B,C))
         VRI_K VEVAL,158
         DC    XL16'22222222 77777777 FFFFFFFF 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,158
         DC    XL16'11111111 DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 159:    OR(A,NXOR(B,C))
         VRI_K VEVAL,159
         DC    XL16'33333333 77777777 FFFFFFFF EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,159
         DC    XL16'33333333 DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 20
* i5 = 161:    SEL(A,AND(B,C),NOT(C))
         VRI_K VEVAL,161
         DC    XL16'11111111 00000000 22222222 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,161
         DC    XL16'66666666 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 163:    SEL(A,B,NOT(C))
         VRI_K VEVAL,163
         DC    XL16'11111111 00000000 22222222 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,163
         DC    XL16'77777777 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 166:    SEL(A,XOR(B,C),NOT(C)
         VRI_K VEVAL,166
         DC    XL16'22222222 44444444 BBBBBBBB 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,166
         DC    XL16'55555555 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 167:    SEL(A,OR(B,C),NOT(C))
         VRI_K VEVAL,167
         DC    XL16'33333333 44444444 BBBBBBBB EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,167
         DC    XL16'77777777 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 21
* i5 = 172:    SEL(A,NOT(B),NOT(C))
         VRI_K VEVAL,172
         DC    XL16'22222222 55555555 BBBBBBBB 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,172
         DC    XL16'44444444 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 174:    SEL(A,NAND(B,C),NOT(C))
         VRI_K VEVAL,174
         DC    XL16'22222222 55555555 BBBBBBBB 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,174
         DC    XL16'55555555 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* Row 22
* Row 23
* Row 24
* Row 25
* Row 26
* Row 27

* Row 28
* i5 = 224:    NOR(A,AND(B,C))
         VRI_K VEVAL,224
         DC    XL16'CCCCCCCC 88888888 22222222 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,224
         DC    XL16'CCCCCCCC 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 225:    NXOR(A,AND(B,C))
         VRI_K VEVAL,225
         DC    XL16'DDDDDDDD 88888888 22222222 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,225
         DC    XL16'EEEEEEEE2 2222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 227:    SEL(A,B,NAND(B,C))
         VRI_K VEVAL,227
         DC    XL16'DDDDDDDD 88888888 22222222 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,227
         DC    XL16'FFFFFFFF 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 230     SEL(A,XOR(B,C),NAND(B,C))
         VRI_K VEVAL,230
         DC    XL16'EEEEEEEE CCCCCCCC BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,230
         DC    XL16'DDDDDDDD 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 231     SEL(A,OR(B,C),NAND(B,C))
         VRI_K VEVAL,231
         DC    XL16'FFFFFFFF CCCCCCCC BBBBBBBB FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,231
         DC    XL16'FFFFFFFF 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 29
* i5 = 232     MINOR(A,B,C)
         VRI_K VEVAL,232
         DC    XL16'CCCCCCCC 99999999 22222222 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,232
         DC    XL16'CCCCCCCC 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 233     SEL(A,NXOR(B,C),NAND(B,C))
         VRI_K VEVAL,233
         DC    XL16'DDDDDDDD 99999999 22222222 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,233
         DC    XL16'EEEEEEEE 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 239     OR(A,NAND(B,C))
         VRI_K VEVAL,239
         DC    XL16'FFFFFFFF DDDDDDDD BBBBBBBB FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,239
         DC    XL16'FFFFFFFF 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 30
* i5 = 241     NAND(A,NAND(B,C))
         VRI_K VEVAL,241
         DC    XL16'DDDDDDDD AAAAAAAA 66666666 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,241
         DC    XL16'EEEEEEEE AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 246     NAND(A,NXOR(B,C))
         VRI_K VEVAL,246
         DC    XL16'EEEEEEEE EEEEEEEE FFFFFFFF 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,246
         DC    XL16'DDDDDDDD FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 247     NAND(A,NOR(B,C))
         VRI_K VEVAL,247
         DC    XL16'FFFFFFFF EEEEEEEE FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,247
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 31
* i5 = 248     NAND(A,OR(B,C))
         VRI_K VEVAL,248
         DC    XL16'CCCCCCCC BBBBBBBB 66666666 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,248
         DC    XL16'CCCCCCCC AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 249     NAND(A,XOR(B,C))
         VRI_K VEVAL,249
         DC    XL16'DDDDDDDD BBBBBBBB 66666666 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,249
         DC    XL16'EEEEEEEE AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 254     NAND(A,B,C)
         VRI_K VEVAL,254
         DC    XL16'EEEEEEEE FFFFFFFF FFFFFFFF 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,254
         DC    XL16'DDDDDDDD FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

                                                                EJECT
*---------------------------------------------------------------------
*---------------------------------------------------------------------
* Test boolean functions NOT defined in PoP SA22-7832-14,
* Figure 22-3. Boolean operations, paged 22-14.
* i.e.
* - The entry is a duplicate function of an existing vector
*   instruction.
* - The entry is a duplicate of another entry in the table
*   for a boolean operation with a different order of A, B, C.
*---------------------------------------------------------------------
*---------------------------------------------------------------------
* Row 0
* i5 = 0       /* 00000 000  +++  */
         VRI_K VEVAL,0
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,0
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 2      /* 00000 010  +++  */
         VRI_K VEVAL,2
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,2
         DC    XL16'11111111 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 3      /* 00000 011  |||  */
         VRI_K VEVAL,3
         DC    XL16'11111111 00000000 00000000 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,3
         DC    XL16'33333333 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 4       /* 00000 100  +++  */
         VRI_K VEVAL,4
         DC    XL16'22222222 44444444 99999999 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,4
         DC    XL16'00000000 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 5       /* 00000 101  |||  */
         VRI_K VEVAL,5
         DC    XL16'33333333 44444444 99999999 AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,5
         DC    XL16'22222222 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 1
* i5 = 10      /* 00001 010  |||  */
         VRI_K VEVAL,10
         DC    XL16'00000000 11111111 00000000 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,10
         DC    XL16'11111111 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 11      /* 00001 011  +++  */
         VRI_K VEVAL,11
         DC    XL16'11111111 11111111 00000000 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,11
         DC    XL16'33333333 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 12      /* 00001 100  |||  */
         VRI_K VEVAL,12
         DC    XL16'22222222 55555555 99999999 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,12
         DC    XL16'00000000 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 13      /* 00001 101  +++  */
         VRI_K VEVAL,13
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,13
         DC    XL16'22222222 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 15      /* 00001 111  |||  */
         VRI_K VEVAL,15
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,15
         DC    XL16'33333333 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 2
* i5 = 17      /* 00010 001  |||  */
         VRI_K VEVAL,17
         DC    XL16'11111111 22222222 44444444 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,17
         DC    XL16'22222222 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 18      /* 00010 010  +++  */
         VRI_K VEVAL,17
         DC    XL16'11111111 22222222 44444444 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,17
         DC    XL16'22222222 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 19      /* 00010 011  +++  */
         VRI_K VEVAL,17
         DC    XL16'11111111 22222222 44444444 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,17
         DC    XL16'22222222 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 20      /* 00010 100  +++  */
         VRI_K VEVAL,17
         DC    XL16'11111111 22222222 44444444 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,17
         DC    XL16'22222222 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 21      /* 00010 101  +++  */
         VRI_K VEVAL,21
         DC    XL16'33333333 66666666 DDDDDDDD AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,21
         DC    XL16'22222222 CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 3
* i5 =  26:    /* 00011 010  +++  */
         VRI_K VEVAL,26
         DC    XL16'00000000 33333333 44444444 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,26
         DC    XL16'11111111 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  27:    /* 00011 011  |||  */
         VRI_K VEVAL,27
         DC    XL16'11111111 33333333 44444444 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,27
         DC    XL16'33333333 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  29:    /* 00011 101  |||  */
         VRI_K VEVAL,29
         DC    XL16'33333333 77777777 DDDDDDDD AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,29
         DC    XL16'22222222 CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 4
* i5 =  32:    /* 00100 000  +++  */
         VRI_K VEVAL,32
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,32
         DC    XL16'44444444 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  33:    /* 00100 001  +++  */
         VRI_K VEVAL,33
         DC    XL16'11111111 00000000 00000000 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,33
         DC    XL16'66666666 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  34:    /* 00100 010  |||  */
         VRI_K VEVAL,34
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,34
         DC    XL16'55555555 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  35:    /* 00100 011  +++  */
         VRI_K VEVAL,35
         DC    XL16'11111111 00000000 00000000 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,35
         DC    XL16'77777777 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  36:    /* 00100 100  +++  */
         VRI_K VEVAL,36
         DC    XL16'22222222 44444444 99999999 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,36
         DC    XL16'44444444 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  37:    /* 00100 101  +++  */
         VRI_K VEVAL,37
         DC    XL16'33333333 44444444 99999999 AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,37
         DC    XL16'66666666 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  38:    /* 00100 110  +++  */
         VRI_K VEVAL,38
         DC    XL16'22222222 44444444 99999999 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,38
         DC    XL16'55555555 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  39:    /* 00100 111  |||  */
         VRI_K VEVAL,39
         DC    XL16'33333333 44444444 99999999 AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,39
         DC    XL16'77777777 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 5
* i5 =  40:    /* 00101 000  +++  */
         VRI_K VEVAL,40
         DC    XL16'00000000 11111111 00000000 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,40
         DC    XL16'44444444 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  41:    /* 00101 001  +++  */
         VRI_K VEVAL,41
         DC    XL16'11111111 11111111 00000000 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,41
         DC    XL16'66666666 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  42:    /* 00101 010  +++  */
         VRI_K VEVAL,42
         DC    XL16'00000000 11111111 00000000 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,42
         DC    XL16'55555555 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  43:    /* 00101 011  +++  */
         VRI_K VEVAL,43
         DC    XL16'11111111 11111111 00000000 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,43
         DC    XL16'77777777 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  44:    /* 00101 100  +++  */
         VRI_K VEVAL,44
         DC    XL16'22222222 55555555 99999999 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,44
         DC    XL16'44444444 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  45:    /* 00101 101  +++  */
         VRI_K VEVAL,45
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,45
         DC    XL16'66666666 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  46:    /* 00101 110  +++  */
         VRI_K VEVAL,46
         DC    XL16'22222222 55555555 99999999 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,46
         DC    XL16'55555555 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  47:    /* 00101 111  +++  */
         VRI_K VEVAL,47
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,47
         DC    XL16'77777777 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 6
* i5 =  48:    /* 00110 000  |||  */
         VRI_K VEVAL,48
         DC    XL16'00000000 22222222 44444444 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,48
         DC    XL16'44444444 AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  49:    /* 00110 001  +++  */
         VRI_K VEVAL,49
         DC    XL16'11111111 22222222 44444444 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,49
         DC    XL16'66666666 AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  50:    /* 00110 010  +++  */
         VRI_K VEVAL,50
         DC    XL16'00000000 22222222 44444444 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,50
         DC    XL16'55555555 BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  51:    /* 00110 011  |||  */
         VRI_K VEVAL,51
         DC    XL16'11111111 22222222 44444444 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,51
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  52:    /* 00110 100  +++  */
         VRI_K VEVAL,52
         DC    XL16'22222222 66666666 DDDDDDDD 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,52
         DC    XL16'44444444 EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  53:    /* 00110 101  |||  */
         VRI_K VEVAL,53
         DC    XL16'33333333 66666666 DDDDDDDD AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,53
         DC    XL16'66666666 EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  54:    /* 00110 110  +++  */
         VRI_K VEVAL,54
         DC    XL16'22222222 66666666 DDDDDDDD 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,54
         DC    XL16'55555555 FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  55:    /* 00110 111  +++  */
         VRI_K VEVAL,55
         DC    XL16'33333333 66666666 DDDDDDDD AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,55
         DC    XL16'77777777 FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 7
* i5 =  56:    /* 00111 000  +++  */
         VRI_K VEVAL,56
         DC    XL16'00000000 33333333 44444444 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,56
         DC    XL16'44444444 AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  57:    /* 00111 001  +++  */
         VRI_K VEVAL,57
         DC    XL16'11111111 33333333 44444444 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,57
         DC    XL16'66666666 AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  58:    /* 00111 010  +++  */
         VRI_K VEVAL,58
         DC    XL16'00000000 33333333 44444444 00000000'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,58
         DC    XL16'55555555 BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  59:    /* 00111 011  +++  */
         VRI_K VEVAL,59
         DC    XL16'11111111 33333333 44444444 88888888'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,59
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  60:    /* 00111 100  |||  */
         VRI_K VEVAL,60
         DC    XL16'22222222 77777777 DDDDDDDD 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,60
         DC    XL16'44444444 EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  61:    /* 00111 101  +++  */
         VRI_K VEVAL,61
         DC    XL16'33333333 77777777 DDDDDDDD AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,61
         DC    XL16'66666666 EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  62:    /* 00111 110  +++  */
         VRI_K VEVAL,62
         DC    XL16'22222222 77777777 DDDDDDDD 22222222'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,62
         DC    XL16'55555555 FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  63:    /* 00111 111  |||  */
         VRI_K VEVAL,63
         DC    XL16'33333333 77777777 DDDDDDDD AAAAAAAA'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,63
         DC    XL16'77777777 FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 8
* i5 =  64:    /* 01000 000  +++  */
         VRI_K VEVAL,64
         DC    XL16'CCCCCCCC 88888888 00000000 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,64
         DC    XL16'88888888 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  65:    /* 01000 001  +++  */
         VRI_K VEVAL,65
         DC    XL16'DDDDDDDD 88888888 00000000 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,65
         DC    XL16'AAAAAAAA 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  66:    /* 01000 010  +++  */
         VRI_K VEVAL,66
         DC    XL16'CCCCCCCC 88888888 00000000 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,66
         DC    XL16'99999999 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  67:    /* 01000 011  +++  */
         VRI_K VEVAL,67
         DC    XL16'DDDDDDDD 88888888 00000000 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,67
         DC    XL16'BBBBBBBB 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  68:    /* 01000 100  |||  */
         VRI_K VEVAL,68
         DC    XL16'EEEEEEEE CCCCCCCC 99999999 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,68
         DC    XL16'88888888 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  69:    /* 01000 101  +++  */
         VRI_K VEVAL,69
         DC    XL16'FFFFFFFF CCCCCCCC 99999999 BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,69
         DC    XL16'AAAAAAAA 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  70:    /* 01000 110  +++  */
         VRI_K VEVAL,70
         DC    XL16'EEEEEEEE CCCCCCCC 99999999 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,70
         DC    XL16'99999999 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  71:    /* 01000 111  |||  */
         VRI_K VEVAL,71
         DC    XL16'FFFFFFFF CCCCCCCC 99999999 BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,71
         DC    XL16'BBBBBBBB 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 9
* i5 =  72:    /* 01001 000  +++  */
         VRI_K VEVAL,72
         DC    XL16'CCCCCCCC 99999999 00000000 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,72
         DC    XL16'88888888 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  73:    /* 01001 001  +++  */
         VRI_K VEVAL,73
         DC    XL16'DDDDDDDD 99999999 00000000 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,73
         DC    XL16'AAAAAAAA 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  74:    /* 01001 010  +++  */
         VRI_K VEVAL,74
         DC    XL16'CCCCCCCC 99999999 00000000 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,74
         DC    XL16'99999999 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  75:    /* 01001 011  +++  */
         VRI_K VEVAL,75
         DC    XL16'DDDDDDDD 99999999 00000000 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,75
         DC    XL16'BBBBBBBB 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  76:    /* 01001 100  +++  */
         VRI_K VEVAL,76
         DC    XL16'EEEEEEEE DDDDDDDD 99999999 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,76
         DC    XL16'88888888 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  77:    /* 01001 101  +++  */
         VRI_K VEVAL,77
         DC    XL16'FFFFFFFF DDDDDDDD 99999999 BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,77
         DC    XL16'AAAAAAAA 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  78:    /* 01001 110  +++  */
         VRI_K VEVAL,78
         DC    XL16'EEEEEEEE DDDDDDDD 99999999 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,78
         DC    XL16'99999999 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  79:    /* 01001 111  +++  */
         VRI_K VEVAL,79
         DC    XL16'FFFFFFFF DDDDDDDD 99999999 BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,79
         DC    XL16'BBBBBBBB 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 10
* i5 =  80:    /* 01010 000  |||  */
         VRI_K VEVAL,80
         DC    XL16'CCCCCCCC AAAAAAAA 44444444 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,80
         DC    XL16'88888888 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  82:    /* 01010 010  +++  */
         VRI_K VEVAL,82
         DC    XL16'CCCCCCCC AAAAAAAA 44444444 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,82
         DC    XL16'99999999 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  83:    /* 01010 011  |||  */
         VRI_K VEVAL,83
         DC    XL16'DDDDDDDD AAAAAAAA 44444444 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,83
         DC    XL16'BBBBBBBB 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  84:    /* 01010 100  +++  */
         VRI_K VEVAL,84
         DC    XL16'EEEEEEEE EEEEEEEE DDDDDDDD 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,84
         DC    XL16'88888888 CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  85:    /* 01010 101  |||  */
         VRI_K VEVAL,85
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,85
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  86:    /* 01010 110  +++  */
         VRI_K VEVAL,86
         DC    XL16'EEEEEEEE EEEEEEEE DDDDDDDD 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,86
         DC    XL16'99999999 DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  87:    /* 01010 111  +++  */
         VRI_K VEVAL,87
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,87
         DC    XL16'BBBBBBBB DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 11
* i5 =  90:    /* 01011 010  |||  */
         VRI_K VEVAL,90
         DC    XL16'CCCCCCCC BBBBBBBB 44444444 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,90
         DC    XL16'99999999 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  91:    /* 01011 011   +++  */
         VRI_K VEVAL,91
         DC    XL16'DDDDDDDD BBBBBBBB 44444444 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,91
         DC    XL16'BBBBBBBB 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  93:    /* 01011 101  +++  */
         VRI_K VEVAL,93
         DC    XL16'FFFFFFFF FFFFFFFF DDDDDDDD BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,93
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  95:    /* 01011 111  |||  */
         VRI_K VEVAL,95
         DC    XL16'FFFFFFFF FFFFFFFF DDDDDDDD BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,95
         DC    XL16'BBBBBBBB DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 12
* i5 =  98:    /* 01100 010   +++  */
         VRI_K VEVAL,98
         DC    XL16'CCCCCCCC 88888888 00000000 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,98
         DC    XL16'DDDDDDDD 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  100:   /* 01100 100  +++  */
         VRI_K VEVAL,100
         DC    XL16'EEEEEEEE CCCCCCCC 99999999 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,100
         DC    XL16'CCCCCCCC 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  101:   /* 01100 101  +++  */
         VRI_K VEVAL,101
         DC    XL16'FFFFFFFF CCCCCCCC 99999999 BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,101
         DC    XL16'EEEEEEEE 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  102:   /* 01100 110  |||  */
         VRI_K VEVAL,102
         DC    XL16'EEEEEEEE CCCCCCCC 99999999 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,102
         DC    XL16'DDDDDDDD 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 13
* i5 =  106:   /* 01101 010  +++  */
         VRI_K VEVAL,106
         DC    XL16'CCCCCCCC 99999999 00000000 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,106
         DC    XL16'DDDDDDDD 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  107:   /* 01101 011  +++  */
         VRI_K VEVAL,107
         DC    XL16'DDDDDDDD 99999999 00000000 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,107
         DC    XL16'FFFFFFFF 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  108:   /* 01101 100  +++  */
         VRI_K VEVAL,108
         DC    XL16'EEEEEEEE DDDDDDDD 99999999 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,108
         DC    XL16'CCCCCCCC 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  109:   /* 01101 101  +++  */
         VRI_K VEVAL,109
         DC    XL16'FFFFFFFF DDDDDDDD 99999999 BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,109
         DC    XL16'EEEEEEEE 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  110:   /* 01101 110  +++  */
         VRI_K VEVAL,110
         DC    XL16'EEEEEEEE DDDDDDDD 99999999 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,110
         DC    XL16'DDDDDDDD 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 14
* i5 =  114:   /* 01110 010  +++  */
         VRI_K VEVAL,114
         DC    XL16'CCCCCCCC AAAAAAAA 44444444 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,114
         DC    XL16'DDDDDDDD BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 116:   /* 01110 100  +++  */
         VRI_K VEVAL,116
         DC    XL16'EEEEEEEE EEEEEEEE DDDDDDDD 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,116
         DC    XL16'CCCCCCCC EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  117:   /* 01110 101  +++  */
         VRI_K VEVAL,117
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,117
         DC    XL16'EEEEEEEE EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  118:   /* 01110 110  +++  */
         VRI_K VEVAL,118
         DC    XL16'EEEEEEEE EEEEEEEE DDDDDDDD 33333333'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,118
         DC    XL16'DDDDDDDD FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  119:   /* 01110 111  |||  */
         VRI_K VEVAL,119
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,119
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 15
* i5 = 122:    /* 01111 010  +++  */
         VRI_K VEVAL,122
         DC    XL16'CCCCCCCC BBBBBBBB 44444444 11111111'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,122
         DC    XL16'DDDDDDDD BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  123:   /* 01111 011  +++  */
         VRI_K VEVAL,123
         DC    XL16'DDDDDDDD BBBBBBBB 44444444 99999999'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,123
         DC    XL16'FFFFFFFF BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 125
         VRI_K VEVAL,125
         DC    XL16'FFFFFFFF FFFFFFFF DDDDDDDD BBBBBBBB'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,125
         DC    XL16'EEEEEEEE EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 16
* i5 = 130
         VRI_K VEVAL,130
         DC    XL16'00000000 00000000 22222222 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,130
         DC    XL16'11111111 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  132:   /* 10000 100  +++  */
         VRI_K VEVAL,132
         DC    XL16'22222222 44444444 BBBBBBBB 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,132
         DC    XL16'00000000 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 133
         VRI_K VEVAL,133
         DC    XL16'33333333 44444444 BBBBBBBB EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,133
         DC    XL16'22222222 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 17
* i5 =  136:   /* 10001 000  |||  */
         VRI_K VEVAL,136
         DC    XL16'00000000 11111111 22222222 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,136
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  137:   /* 10001 001  +++  */
         VRI_K VEVAL,137
         DC    XL16'11111111 11111111 22222222 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,137
         DC    XL16'22222222 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  138:   /* 10001 010  +++  */
         VRI_K VEVAL,138
         DC    XL16'00000000 11111111 22222222 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,138
         DC    XL16'11111111 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  139:   /* 10001 011  +++  */
         VRI_K VEVAL,139
         DC    XL16'11111111 11111111 22222222 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,139
         DC    XL16'33333333 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  141:   /* 10001 101  +++  */
         VRI_K VEVAL,141
         DC    XL16'33333333 55555555 BBBBBBBB EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,141
         DC    XL16'22222222 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 18
* i5 =  145:   /* 10010 001  +++  */
         VRI_K VEVAL,145
         DC    XL16'11111111 22222222 66666666 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,145
         DC    XL16'22222222 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  146:   /* 10010 010  +++  */
         VRI_K VEVAL,146
         DC    XL16'00000000 22222222 66666666 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,146
         DC    XL16'11111111 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  147:   /* 10010 011  +++  */
         VRI_K VEVAL,147
         DC    XL16'11111111 22222222 66666666 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,147
         DC    XL16'33333333 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  148:   /* 10010 100  +++  */
         VRI_K VEVAL,148
         DC    XL16'22222222 66666666 FFFFFFFF 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,148
         DC    XL16'00000000 CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  149:   /* 10010 101  +++  */
         VRI_K VEVAL,149
         DC    XL16'33333333 66666666 FFFFFFFF EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,149
         DC    XL16'22222222 CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 19
* i5 =  153:   /* 10011 001  |||  */
         VRI_K VEVAL,153
         DC    XL16'11111111 33333333 66666666 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,153
         DC    XL16'22222222 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  154:   /* 10011 010  +++  */
         VRI_K VEVAL,154
         DC    XL16'00000000 33333333 66666666 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,154
         DC    XL16'11111111 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  155:   /* 10011 011  +++  */
         VRI_K VEVAL,155
         DC    XL16'11111111 33333333 66666666 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,155
         DC    XL16'33333333 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  157:   /* 10011 101  +++  */
         VRI_K VEVAL,157
         DC    XL16'33333333 77777777 FFFFFFFF EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,157
         DC    XL16'22222222 CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 20
* i5 =  160:   /* 10100 000 * |||  */
         VRI_K VEVAL,160
         DC    XL16'00000000 00000000 22222222 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,160
         DC    XL16'44444444 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  162:   /* 10100 010  +++  */
         VRI_K VEVAL,162
         DC    XL16'00000000 00000000 22222222 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,162
         DC    XL16'55555555 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  164:   /* 10100 100  +++  */
         VRI_K VEVAL,164
         DC    XL16'22222222 44444444 BBBBBBBB 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,164
         DC    XL16'44444444 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  165:   /* 10100 101  |||  */
         VRI_K VEVAL,165
         DC    XL16'33333333 44444444 BBBBBBBB EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,165
         DC    XL16'66666666 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 21
* i5 =  168:   /* 10101 000  +++  */
         VRI_K VEVAL,168
         DC    XL16'00000000 11111111 22222222 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,168
         DC    XL16'44444444 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  169:   /* 10101 001  +++  */
         VRI_K VEVAL,169
         DC    XL16'11111111 11111111 22222222 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,169
         DC    XL16'66666666 22222222 88888888 22222222'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  170:   /* 10101 010  |||  */
         VRI_K VEVAL,170
         DC    XL16'00000000 11111111 22222222 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,170
         DC    XL16'55555555 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  171:   /* 10101 011  +++  */
         VRI_K VEVAL,171
         DC    XL16'11111111 11111111 22222222 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,171
         DC    XL16'77777777 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  173:   /* 10101 101  +++  */
         VRI_K VEVAL,173
         DC    XL16'33333333 55555555 BBBBBBBB EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,173
         DC    XL16'66666666 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  175:   /* 10101 111  |||  */
         VRI_K VEVAL,175
         DC    XL16'33333333 55555555 BBBBBBBB EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,175
         DC    XL16'77777777 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 22
* i5 =  176:   /* 10110 000  +++  */
         VRI_K VEVAL,176
         DC    XL16'00000000 22222222 66666666 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,176
         DC    XL16'44444444 AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  177:   /* 10110 001  +++  */
         VRI_K VEVAL,177
         DC    XL16'11111111 22222222 66666666 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,177
         DC    XL16'66666666 AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  178:   /* 10110 010  +++  */
         VRI_K VEVAL,178
         DC    XL16'00000000 22222222 66666666 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,178
         DC    XL16'55555555 BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  179:   /* 10110 011  +++  */
         VRI_K VEVAL,179
         DC    XL16'11111111 22222222 66666666 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,179
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  180:   /* 10110 100  +++  */
         VRI_K VEVAL,180
         DC    XL16'22222222 66666666 FFFFFFFF 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,180
         DC    XL16'44444444 EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  181:   /* 10110 101  +++  */
         VRI_K VEVAL,181
         DC    XL16'33333333 66666666 FFFFFFFF EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,181
         DC    XL16'66666666 EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  182:   /* 10110 110  +++  */
         VRI_K VEVAL,182
         DC    XL16'22222222 66666666 FFFFFFFF 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,182
         DC    XL16'55555555 FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  183:   /* 10110 111  +++  */
         VRI_K VEVAL,183
         DC    XL16'33333333 66666666 FFFFFFFF EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,183
         DC    XL16'77777777 FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 23
* i5 =  184:   /* 10111 000  +++  */
         VRI_K VEVAL,184
         DC    XL16'00000000 33333333 66666666 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,184
         DC    XL16'44444444 AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  185:   /* 10111 001  +++  */
         VRI_K VEVAL,185
         DC    XL16'11111111 33333333 66666666 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,185
         DC    XL16'66666666 AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  186:   /* 10111 010  +++  */
         VRI_K VEVAL,186
         DC    XL16'00000000 33333333 66666666 44444444'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,186
         DC    XL16'55555555 BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  187:   /* 10111 011  |||  */
         VRI_K VEVAL,187
         DC    XL16'11111111 33333333 66666666 CCCCCCCC'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,187
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  188:   /* 10111 100  +++  */
         VRI_K VEVAL,188
         DC    XL16'22222222 77777777 FFFFFFFF 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,188
         DC    XL16'44444444 EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  189:   /* 10111 101  +++  */
         VRI_K VEVAL,189
         DC    XL16'33333333 77777777 FFFFFFFF EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,189
         DC    XL16'66666666 EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  190:   /* 10111 110  +++  */
         VRI_K VEVAL,190
         DC    XL16'22222222 77777777 FFFFFFFF 66666666'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,190
         DC    XL16'55555555 FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  191:   /* 10111 111  +++  */
         VRI_K VEVAL,191
         DC    XL16'33333333 77777777 FFFFFFFF EEEEEEEE'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,191
         DC    XL16'77777777 FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 24
* i5 =  192:   /* 11000 000  |||  */
         VRI_K VEVAL,192
         DC    XL16'CCCCCCCC 88888888 22222222 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,192
         DC    XL16'88888888 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  193:   /* 11000 001  +++  */
         VRI_K VEVAL,193
         DC    XL16'DDDDDDDD 88888888 22222222 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,193
         DC    XL16'AAAAAAAA 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  194:   /* 11000 010  +++  */
         VRI_K VEVAL,194
         DC    XL16'CCCCCCCC 88888888 22222222 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,194
         DC    XL16'99999999 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  195:   /* 11000 011  |||  */
         VRI_K VEVAL,195
         DC    XL16'DDDDDDDD 88888888 22222222 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,195
         DC    XL16'BBBBBBBB 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  196:   /* 11000 100  +++  */
         VRI_K VEVAL,196
         DC    XL16'EEEEEEEE CCCCCCCC BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,196
         DC    XL16'88888888 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  197:   /* 11000 101  +++  */
         VRI_K VEVAL,197
         DC    XL16'FFFFFFFF CCCCCCCC BBBBBBBB FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,197
         DC    XL16'AAAAAAAA 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  198:   /* 11000 110  +++  */
         VRI_K VEVAL,198
         DC    XL16'EEEEEEEE CCCCCCCC BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,198
         DC    XL16'99999999 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  199:   /* 11000 111  +++  */
         VRI_K VEVAL,199
         DC    XL16'FFFFFFFF CCCCCCCC BBBBBBBB FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,199
         DC    XL16'BBBBBBBB 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 25
* i5 =  200:   /* 11001 000  +++  */
         VRI_K VEVAL,200
         DC    XL16'CCCCCCCC 99999999 22222222 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,200
         DC    XL16'88888888 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  201:   /* 11001 001  +++  */
         VRI_K VEVAL,201
         DC    XL16'DDDDDDDD 99999999 22222222 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,201
         DC    XL16'AAAAAAAA 00000000 00000000 00000000'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  202:   /* 11001 010  +++  */
         VRI_K VEVAL,202
         DC    XL16'CCCCCCCC 99999999 22222222 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,202
         DC    XL16'99999999 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  203:   /* 11001 011  +++  */
         VRI_K VEVAL,203
         DC    XL16'DDDDDDDD 99999999 22222222 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,203
         DC    XL16'BBBBBBBB 11111111 44444444 88888888'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  204:   /* 11001 100  |||  */
         VRI_K VEVAL,204
         DC    XL16'EEEEEEEE DDDDDDDD BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,204
         DC    XL16'88888888 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  205:   /* 11001 101  +++  */
         VRI_K VEVAL,205
         DC    XL16'FFFFFFFF DDDDDDDD BBBBBBBB FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,205
         DC    XL16'AAAAAAAA 44444444 22222222 11111111'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  206:   /* 11001 110  +++  */
         VRI_K VEVAL,206
         DC    XL16'EEEEEEEE DDDDDDDD BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,206
         DC    XL16'99999999 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  207:   /* 11001 111  ||| */
         VRI_K VEVAL,207
         DC    XL16'FFFFFFFF DDDDDDDD BBBBBBBB FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,207
         DC    XL16'BBBBBBBB 55555555 66666666 99999999'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 26
* i5 =  208:   /* 11010 000  +++  */
         VRI_K VEVAL,208
         DC    XL16'CCCCCCCC AAAAAAAA 66666666 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,208
         DC    XL16'88888888 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  209:   /* 11010 001  +++  */
         VRI_K VEVAL,209
         DC    XL16'DDDDDDDD AAAAAAAA 66666666 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,209
         DC    XL16'AAAAAAAA 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  210:   /* 11010 010  +++  */
         VRI_K VEVAL,210
         DC    XL16'CCCCCCCC AAAAAAAA 66666666 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,210
         DC    XL16'99999999 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  211:   /* 11010 011  +++  */
         VRI_K VEVAL,211
         DC    XL16'DDDDDDDD AAAAAAAA 66666666 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,211
         DC    XL16'BBBBBBBB 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  212:   /* 11010 100  +++  */
         VRI_K VEVAL,212
         DC    XL16'EEEEEEEE EEEEEEEE FFFFFFFF 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,212
         DC    XL16'88888888 CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  213:   /* 11010 101  +++  */
         VRI_K VEVAL,213
         DC    XL16'FFFFFFFF EEEEEEEE FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,213
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  214:   /* 11010 110  +++  */
         VRI_K VEVAL,214
         DC    XL16'EEEEEEEE EEEEEEEE FFFFFFFF 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,214
         DC    XL16'99999999 DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  215:   /* 11010 111  +++  */
         VRI_K VEVAL,215
         DC    XL16'FFFFFFFF EEEEEEEE FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,215
         DC    XL16'BBBBBBBB DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 27
* i5 =  216:   /* 11011 000  +++  */
         VRI_K VEVAL,216
         DC    XL16'CCCCCCCC BBBBBBBB 66666666 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,216
         DC    XL16'88888888 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  217:   /* 11011 001  +++  */
         VRI_K VEVAL,217
         DC    XL16'DDDDDDDD BBBBBBBB 66666666 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,217
         DC    XL16'AAAAAAAA 88888888 11111111 44444444'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  218:   /* 11011 010  +++  */
         VRI_K VEVAL,218
         DC    XL16'CCCCCCCC BBBBBBBB 66666666 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,218
         DC    XL16'99999999 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  219:   /* 11011 011  +++  */
         VRI_K VEVAL,219
         DC    XL16'DDDDDDDD BBBBBBBB 66666666 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,219
         DC    XL16'BBBBBBBB 99999999 55555555 CCCCCCCC'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  220:   /* 11011 100  +++  */
         VRI_K VEVAL,220
         DC    XL16'EEEEEEEE FFFFFFFF FFFFFFFF 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,220
         DC    XL16'88888888 CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  221:   /* 11011 101  |||  */
         VRI_K VEVAL,221
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,221
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  222:   /* 11011 110  +++  */
         VRI_K VEVAL,222
         DC    XL16'EEEEEEEE FFFFFFFF FFFFFFFF 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,222
         DC    XL16'99999999 DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  223:   /* 11011 111  +++  */
         VRI_K VEVAL,223
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,223
         DC    XL16'BBBBBBBB DDDDDDDD 77777777 DDDDDDDD'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 28
* i5 =  226:   /* 11100 010  +++  */
         VRI_K VEVAL,226
         DC    XL16'CCCCCCCC 88888888 22222222 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,226
         DC    XL16'DDDDDDDD 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  228:   /* 11100 100  +++  */
         VRI_K VEVAL,228
         DC    XL16'EEEEEEEE CCCCCCCC BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,228
         DC    XL16'CCCCCCCC 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  229:   /* 11100 101  +++  */
         VRI_K VEVAL,229
         DC    XL16'FFFFFFFF CCCCCCCC BBBBBBBB FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,229
         DC    XL16'EEEEEEEE 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 29
* i5 =  234:   /* 11101 010  +++  */
         VRI_K VEVAL,234
         DC    XL16'CCCCCCCC 99999999 22222222 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,234
         DC    XL16'DDDDDDDD 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  235:   /* 11101 011  +++  */
         VRI_K VEVAL,235
         DC    XL16'DDDDDDDD 99999999 22222222 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,235
         DC    XL16'FFFFFFFF 33333333 CCCCCCCC AAAAAAAA'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  236:   /* 11101 100  +++  */
         VRI_K VEVAL,236
         DC    XL16'EEEEEEEE DDDDDDDD BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,236
         DC    XL16'CCCCCCCC 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  237:   /* 11101 101  +++  */
         VRI_K VEVAL,237
         DC    XL16'FFFFFFFF DDDDDDDD BBBBBBBB FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,237
         DC    XL16'EEEEEEEE 66666666 AAAAAAAA 33333333'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  238:   /* 11101 110  |||  */
         VRI_K VEVAL,238
         DC    XL16'EEEEEEEE DDDDDDDD BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,238
         DC    XL16'DDDDDDDD 77777777 EEEEEEEE BBBBBBBB'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* Row 30
* i5 =  240:   /* 11110 000  |||  */
         VRI_K VEVAL,240
         DC    XL16'CCCCCCCC AAAAAAAA 66666666 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,240
         DC    XL16'CCCCCCCC AAAAAAAA 99999999 66666666'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  242:   /* 11110 010  +++  */
         VRI_K VEVAL,242
         DC    XL16'CCCCCCCC AAAAAAAA 66666666 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,242
         DC    XL16'DDDDDDDD BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 243     /* 11110 011  |||  */
         VRI_K VEVAL,243
         DC    XL16'DDDDDDDD AAAAAAAA 66666666 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,243
         DC    XL16'FFFFFFFF BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  244:   /* 11110 100  +++  */
         VRI_K VEVAL,244
         DC    XL16'EEEEEEEE EEEEEEEE FFFFFFFF 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,244
         DC    XL16'CCCCCCCC EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 245     /* 11110 101  |||  */
         VRI_K VEVAL,245
         DC    XL16'FFFFFFFF EEEEEEEE FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,245
         DC    XL16'EEEEEEEE EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* Row 31
* i5 =  250:   /* 11111 010  |||  */
         VRI_K VEVAL,250
         DC    XL16'CCCCCCCC BBBBBBBB 66666666 55555555'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,250
         DC    XL16'DDDDDDDD BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* i5 = 251     /* 11111 011  +++  */
         VRI_K VEVAL,251
         DC    XL16'DDDDDDDD BBBBBBBB 66666666 DDDDDDDD'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,251
         DC    XL16'FFFFFFFF BBBBBBBB DDDDDDDD EEEEEEEE'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 =  252:   /* 11111 100  |||  */
         VRI_K VEVAL,252
         DC    XL16'EEEEEEEE FFFFFFFF FFFFFFFF 77777777'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,252
         DC    XL16'CCCCCCCC EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


* i5 = 253     /* 11111 101  +++  */
         VRI_K VEVAL,253
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,253
         DC    XL16'EEEEEEEE EEEEEEEE BBBBBBBB 77777777'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4

* i5 = 255
         VRI_K VEVAL,255
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 99999999 AAAAAAAA'   v2
         DC    XL16'11111111 22222222 44444444 88888888'   v3
         DC    XL16'FFFFFFFF EEEEEEEE DDDDDDDD BBBBBBBB'   v4

         VRI_K VEVAL,255
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'33333333 55555555 66666666 99999999'   v2
         DC    XL16'77777777 BBBBBBBB DDDDDDDD EEEEEEEE'   v3
         DC    XL16'AAAAAAAA CCCCCCCC 33333333 55555555'   v4


         DC    F'0'     END OF TABLE
         DC    F'0'
                                                                 EJECT
*
* table of pointers to individual load test
*
E7TESTS  DS    0F
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
