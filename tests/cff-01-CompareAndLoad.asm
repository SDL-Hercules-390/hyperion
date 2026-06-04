 TITLE 'cff-01-CompareAndLoad'
***********************************************************************
*
*        Concurrent-functions facility tests
*        for SSF encoded instructions:
*
*        C8x6 CAL   - Compare And Load
*        C8x7 CALG  - Compare And Load Long
*        C8xF CALGF - Compare And Load Long Fullword
*
*        James Wekel March 2026
***********************************************************************

***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch
*  Concurrent-functions facility compare and load instructions
*  (CAL, CALG, CALGF). Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*   *Testcase cff-01-CompareAndLoad
*   *
*   * Concurrent-functions facility tests for SSF encoded instructions:
*   *
*   *        C8x6 CAL   - Compare And Load
*   *        C8x7 CALG  - Compare And Load Long
*   *        C8xF CALGF - Compare And Load Long Fullword
*   *
*   *   # -------------------------------------------------------
*   *   #  This tests only the basic function of the instruction.
*   *   #  Exceptions are NOT tested.
*   *   # -------------------------------------------------------
*   *
*   mainsize    2
*   numcpu      1
*   sysclear
*   archlvl     z/Arch
*
*   loadcore    "$(testpath)/cff-01-CompareAndLoad.core" 0x0
*
*   diag8cmd    enable    # (needed for messages to Hercules console)
*   runtest     2
*   diag8cmd    disable   # (reset back to default)
*
*   *Done

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
         LCLA  &FBBYTE                Facility bit in Byte
         LCLA  &FBBIT                 Facility bit within Byte

         LCLA  &L(8)
&L(1)    SetA  128,64,32,16,8,4,2,1   bit positions within byte

&FBBYTE  SETA  &BITNO/8
&FBBIT   SETA  &L((&BITNO-(&FBBYTE*8))+1)
.*       MNOTE 0,'checking Bit=&BITNO: FBBYTE=&FBBYTE, FBBIT=&FBBIT'

         B     X&SYSNDX
*                                     Fcheck data area
*                                     skip messgae
SKT&SYSNDX DC  C'      Skipping tests: '
         DC    C&NOTSETMSG
         DC    C' facility (bit &BITNO) is not installed.'
SKL&SYSNDX EQU *-SKT&SYSNDX
*                                     facility bits
         DS    FD                     gap
FB&SYSNDX DS   4FD
         DS    FD                     gap
*
X&SYSNDX EQU *
         LA    R0,((X&SYSNDX-FB&SYSNDX)/8)-1
         STFLE FB&SYSNDX              get facility bits

         XGR   R0,R0
         IC    R0,FB&SYSNDX+&FBBYTE   get fbit byte
         N     R0,=F'&FBBIT'          is bit set?
         BNZ   XC&SYSNDX
*
* facility bit not set, issue message and exit
*
         LA    R0,SKL&SYSNDX          message length
         LA    R1,SKT&SYSNDX          message address
         BAL   R2,MSG

         B     EOJ
XC&SYSNDX EQU *
         MEND
                                                               EJECT
***********************************************************************
*        Low core PSWs
***********************************************************************
                                                                SPACE 2
CFFTST   START 0
         USING CFFTST,R0              Low core addressability

SVOLDPSW EQU   CFFTST+X'140'          z/Arch Supervisor call old PSW
                                                                SPACE 2
         ORG   CFFTST+X'1A0'          z/Architecure RESTART PSW
         DC    X'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE 2
         ORG   CFFTST+X'1D0'          z/Architecure PROGRAM CHECK PSW
         DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                SPACE 3
         ORG   CFFTST+X'200'          Start of actual test program...

                                                                EJECT
***********************************************************************
*               The actual "CFFTST" program itself...
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
*   R11      CFFTEST call return
*   R12      CFFTESTS register
*   R13      (work)
*   R14      Subroutine call
*   R15      Secondary Subroutine call or work
*
***********************************************************************
                                                                SPACE
         USING BEGIN,R8               FIRST Base Register
         USING BEGIN+4096,R9          SECOND Base Register
         USING BEGIN+8192,R10         THIRD Base Register

BEGIN    BALR  R8,0                   Initalize FIRST base register
         BCTR  R8,0                   Initalize FIRST base register
         BCTR  R8,0                   Initalize FIRST base register

         LA    R9,2048(,R8)           Initalize SECOND base register
         LA    R9,2048(,R9)           Initalize SECOND base register

         LA    R10,2048(,R9)          Initalize THIRD base register
         LA    R10,2048(,R10)         Initalize THIRD base register

         STCTL R0,R0,CTLR0            Store CR0 to enable AFP
         OI    CTLR0+1,X'04'          Turn on AFP bit
         OI    CTLR0+1,X'02'          Turn on Vector bit
         LCTL  R0,R0,CTLR0            Reload updated CR0
                                                                EJECT
***********************************************************************
* Is Concurrent-functions facility installed  (bit 201)
***********************************************************************
         FCHECK 201,'concurrent-functions facility'
                                                                EJECT
***********************************************************************
*              Do tests in the CFFTESTS table
***********************************************************************

         L     R12,=A(CFFTESTS)       get table of test addresses

NEXTCFF  EQU   *
         L     R5,0(0,R12)            get test address
         LTR   R5,R5                     have a test?
         BZ    ENDTEST                      done?

         USING CFFTEST,R5

         L     R11,TSUB               get address of test routine
         BALR  R11,R11                do test

         LB    R1,CCMASK              (failure CC mask)
         SLL   R1,4                   (shift to BC instr CC position)
         EX    R1,TESTCC                 fail if...

TESTREST EQU   *
         LG    R1,READDR              get adress of expected result
         CLC   R3OUTPUT,0(R1)         valid?
         BNE   FAILMSG                   no issue failed message

         LA    R12,4(0,R12)           next test address
         B     NEXTCFF

TESTCC   BC    0,CCMSG                fail if unexpected condition code
                                                                EJECT
**********************************************************************
* cc was not as expecte
**********************************************************************
CCMSG    EQU   *

* extract CC from saved PSW
         L     R1,CCPSW
         SRL   R1,12
         N     R1,=XL4'3'
         STC   R1,CCFOUND             save actual cc

* fill in message
         LH    R2,TNUM                get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   CCPRTNUM(3),PRT3+13    fill in message with test #
         MVC   CCPRTNAME,OPNAME       fill in message with instruction

* fill in expected CC
         XGR   R2,R2                  get CC as U8
         IC    R2,CC
         CVD   R2,DECNUM              and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   CCPRTEXP(1),PRT3+15    fill in message with CC field

* fill in found CC
         XGR   R2,R2                  get CCFOUND as U8
         IC    R2,CCFOUND
         CVD   R2,DECNUM              and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   CCPRTGOT(1),PRT3+15    fill in message with CCFOUND

         LA    R0,CCPRTLNG            message length
         LA    R1,CCPRTLINE           messagfe address
         BAL   R15,RPTERROR

         L     R0,=F'1'               set GLOBAL failed test indicator
         ST    R0,FAILED

         B     TESTREST               continue with test result check
                                                                 EJECT
***********************************************************************
* result not as expected:
*        issue message with test number, instruction under test
*              and instruction m5, m6
***********************************************************************
FAILMSG  EQU   *
         LH    R2,TNUM                get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTNUM(3),PRT3+13      fill in message with test #

         MVC   PRTNAME,OPNAME         fill in message with instruction

         LA    R0,PRTLNG              message length
         LA    R1,PRTLINE             messagfe address
         BAL   R15,RPTERROR
                                                               SPACE 2
***********************************************************************
* continue after a failed test
***********************************************************************
FAILCONT EQU   *
         L     R0,=F'1'               set GLOBAL failed test indicator
         ST    R0,FAILED

         LA    R12,4(0,R12)           next test address
         B     NEXTCFF
                                                                SPACE 2
***********************************************************************
* end of testing; set ending psw
***********************************************************************
ENDTEST  EQU   *
         L     R1,FAILED              did a test fail?
         LTR   R1,R1
         BZ    EOJ                    No, exit
         B     FAILTEST               Yes, exit with BAD PSW

                                                                EJECT
***********************************************************************
*        RPTERROR          Report instruction test in error
*                             R0 = MESSGAE LENGTH
*                             R1 = ADDRESS OF MESSAGE
***********************************************************************
                                                               SPACE
RPTERROR ST    R15,RPTSAVE            Save return address
         ST    R5,RPTSVR5             Save R5
*
*        Use Hercules Diagnose for Message to console
*
         STM   R0,R2,RPTDWSAV         save regs used by MSG
         BAL   R2,MSG                 call Hercules console MSG display
         LM    R0,R2,RPTDWSAV         restore regs
                                                                SPACE 2
         L     R5,RPTSVR5             Restore R5
         L     R15,RPTSAVE            Restore return address
         BR    R15                    Return to caller
                                                                 SPACE
RPTSAVE  DC    F'0'                   R15 save area
RPTSVR5  DC    F'0'                   R5 save area
                                                                 SPACE
RPTDWSAV DC    2D'0'                  R0-R2 save area for MSG call
                                                               EJECT
***********************************************************************
*        Issue HERCULES MESSAGE pointed to by R1, length in R0
*              R2 = return address
***********************************************************************

MSG      CH    R0,=H'0'               Do we even HAVE a message?
         BNHR  R2                     No, ignore

         STM   R0,R2,MSGSAVE          Save registers

         CH    R0,=AL2(L'MSGMSG)      Message length within limits?
         BNH   MSGOK                  Yes, continue
         LA    R0,L'MSGMSG            No, set to maximum

MSGOK    LR    R2,R0                  Copy length to work register
         BCTR  R2,0                   Minus-1 for execute
         EX    R2,MSGMVC              Copy message to O/P buffer

         LA    R2,1+L'MSGCMD(,R2)     Calculate true command length
         LA    R1,MSGCMD              Point to true command

         DC    X'83',X'12',X'0008'    Issue Hercules Diagnose X'008'
         BZ    MSGRET                 Return if successful

         LTR   R2,R2                  Is Diag8 Ry (R2) 0?
         BZ    MSGRET                   an error occurred but coninue

         DC    H'0'                   CRASH for debugging purposes

MSGRET   LM    R0,R2,MSGSAVE          Restore registers
         BR    R2                     Return to caller
                                                                SPACE 4
MSGSAVE  DC    3F'0'                  Registers save area
MSGMVC   MVC   MSGMSG(0),0(R1)        Executed instruction
                                                                SPACE 2
MSGCMD   DC    C'MSGNOH * '           *** HERCULES MESSAGE COMMAND ***
MSGMSG   DC    CL95' '                The message text to be displayed

                                                                EJECT
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 4
EOJPSW   DC    0D'0',X'0002000180000000',AD(0)
                                                                SPACE
EOJ      LPSWE EOJPSW            Normal completion
                                                                SPACE 4
FAILPSW  DC    0D'0',X'0002000180000000',AD(X'BAD')
                                                                SPACE
FAILTEST LPSWE FAILPSW           Abnormal termination
                                                                SPACE 4
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE 2
CTLR0    DS    F                  CR0
         DS    F
                                                                  SPACE 2
         LTORG ,                  Literals pool

*        some constants

K        EQU   1024               One KB
PAGE     EQU   (4*K)              Size of one page
K64      EQU   (64*K)             64 KB
MB       EQU   (K*K)               1 MB


REG2PATT EQU   X'AABBCCDD'        Polluted Register pattern
REG2LOW  EQU         X'DD'        (last byte above)
                                                                EJECT
*======================================================================
*
*  NOTE: start data on an address that is easy to display
*        within Hercules
*
*======================================================================

         ORG   CFFTST+X'1000'
FAILED   DC    F'0'               some test failed?
TESTING  DC    F'0'               current test number
                                                               SPACE 2
                                                               SPACE 2
***********************************************************************
*        TEST failed : result messgae
***********************************************************************
*
*        failed message and associated editting
*
PRTLINE  DC    C'         Test # '
PRTNUM   DC    C'xxx'
         DC    C' failed for instruction '
PRTNAME  DC    CL8'xxxxxxxx'
PRTLNG   EQU   *-PRTLINE


***********************************************************************
*        TEST failed : CC message
***********************************************************************
*
*        failed message and associated editting
*
CCPRTLINE DC   C'         Test # '
CCPRTNUM DC    C'xxx'
         DC    c' wrong cc for instruction '
CCPRTNAME DC    CL8'xxxxxxxx'
         DC    C' expected: cc='
CCPRTEXP DC    C'x'
         DC    C','
         DC    C' received: cc='
CCPRTGOT DC    C'x'
         DC    C'.'
CCPRTLNG   EQU   *-CCPRTLINE
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
         DS    0FD
R1FUDGE  DC    XL8'AABBCCDDEEFFAABB'                     R1 FUDGE
         DS    XL16                                          gap
V1OUTPUT DS    XL16                                      V1 OUTPUT
         DS    XL16                                          gap
R1OUTPUT DS    FD                                        R1 OUTPUT
         DS    XL16                                          gap
V1FUDGE  DC    XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    V1 FUDGE
V1FUDGEB DC    XL16'BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB'    V1 FUDGE b
V1INPUT  DC    CL16'1234567890123456'                    V1 input
         DC    CL14'78901234567890'
         DC    X'D9'
         DS    XL16                                          gap
                                                                EJECT
***********************************************************************
*        CFFTEST DSECT
***********************************************************************
                                                                SPACE 2
CFFTEST   DSECT ,
TSUB     DC    A(0)               pointer  to test
TNUM     DC    H'00'              Test Number

         DC    XL1'00'
CC       DC    HL1'00'            cc
CCMASK   DC    HL1'00'            not expected CC mask
         DC    XL1'00'

OPNAME   DC    CL8' '             Concurrent-functions instruction name
O1ADDR   DC    AD(0)               address of op1 source
O2ADDR   DC    AD(0)               address of op2 source
O3ADDR   DC    AD(0)               address of op3/r3 source
RELEN    DC    A(0)               RESULT LENGTH
READDR   DC    AD(0)               expected result address
         DS    FD                    gap
R3OUTPUT DS    D                  Op3/R3 Output
         DS    FD                    gap
CCPSW    DS    2F                 extract PSW after test (has CC)
CCFOUND  DS    X                  extracted cc
         DS    FD
                                                               EJECT
***********************************************************************
*     Macros to help build test tables
*----------------------------------------------------------------------
* (pending     C8x6 CAL   - Compare And Load
*  inclusion in SATK ASAM)
*
*     CAL Macro to help build CAL instruction
*        Note: R1, R2, R3 are fixed registers:
*                         r1 = 1; r2 = 2; r3 = 3
***********************************************************************
         MACRO
         CAL
                                                               SPACE 1
         DS    0H
         DC    XL6'C83610002000'  CAL r3,0(R1),0(R2)
                                                               SPACE 1
         MEND
                                                               EJECT
***********************************************************************
* (pending     C8x7 CALG  - Compare And Load Long
*  inclusion in SATK ASAM)
*
*     CALG Macro to help build CALG instruction
*        Note: R1, R2, R3 are fixed registers:
*                         r1 = 1; r2 = 2; r3 = 3
***********************************************************************
         MACRO
         CALG
                                                               SPACE 1
         DS    0H
         DC    XL6'C83710002000'  CALG r3,0(R1),0(R2)
                                                               SPACE 1
         MEND
                                                               EJECT
***********************************************************************
* (pending     C8xF CALGF - Compare And Load Long Fullword
*  inclusion in SATK ASAM)
*
*     CALGF Macro to help build CALGF instruction
*        Note: R1, R2, R3 are fixed registers:
*                         r1 = 1; r2 = 2; r3 = 3
***********************************************************************
         MACRO
         CALGF
                                                               SPACE 1
         DS    0H
         DC    XL6'C83F10002000'  CALGF r3,0(R1),0(R2)
                                                               SPACE 1
         MEND
                                                               EJECT
***********************************************************************
*     SSF Macro to help build test tables
***********************************************************************
         MACRO
         SSF   &INST,&CC
.*                                &INST  - instruction under test
.*                                &CC    - expected CC
.*
         LCLA  &XCC(4)  &CC has mask values for FAILED condition codes
&XCC(1)  SETA  7                  CC != 0
&XCC(2)  SETA  11                 CC != 1
&XCC(3)  SETA  13                 CC != 2
&XCC(4)  SETA  14                 CC != 3

         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5               base for test data and test routine

T&TNUM   DC    A(X&TNUM)          address of test routine
         DC    H'&TNUM'           test number
         DC    XL1'00'
         DC    HL1'&CC'           cc
         DC    HL1'&XCC(&CC+1)'   cc failed mask
         DC    XL1'00'

         DC    CL8'&INST'         instruction name
O1A&TNUM DC    AD(RE&TNUM+8)      address of op1 source
O2A&TNUM DC    AD(RE&TNUM+16)     address of op2 source
O3A&TNUM DC    AD(RE&TNUM+24)     address of op3/R3 source
         DC    A(8)               result length
REA&TNUM DC    AD(RE&TNUM)        result address
         DS    FD                 gap
R3O&TNUM DS    D                  Op3/R3 Output
         DS    FD                 gap
CCP&TNUM DS    2F                 extract PSW after test (has CC)
CCF&TNUM DS    X                  extracted cc
         DS    FD
.*
*                                 INSTRUCTION UNDER TEST ROUTINE
X&TNUM   DS    0F
         LG     R1,O1A&TNUM       get R1 source
         LG     R2,O2A&TNUM       get R2 source
         LG     R3,O3A&TNUM       get R3 source address
         LG     R3,0(R3)          get R3 source value

         &INST                    test instruction

         STG   R3,R3O&TNUM

         EPSW  R4,R0              extract psw
         ST    R4,CCP&TNUM           to save CC

         BR    R11

RE&TNUM  DC    0F
         DROP  R5

         MEND
                                                               EJECT
***********************************************************************
*     PTTABLE Macro to generate table of pointers to individual tests
***********************************************************************

         MACRO
         PTTABLE
         GBLA  &TNUM
         LCLA  &CUR
&CUR     SETA  1
.*
TTABLE   DS    0F
.LOOP    ANOP
.*
         DC    A(T&CUR)           address of test
.*
&CUR     SETA  &CUR+1
         AIF   (&CUR LE &TNUM).LOOP
*
         DC    A(0)               END OF TABLE
         DC    A(0)
.*
         MEND
                                                                EJECT
***********************************************************************
*        Concurrent-functions facility tests
***********************************************************************
CFFTST   CSECT ,
         DS    0F
                                                                SPACE 2
         PRINT DATA
*
*        C8x6 CAL   - Compare And Load
*        C8x7 CALG  - Compare And Load Long
*        C8xF CALGF - Compare And Load Long Fullword
*
*        SSF   instr, cc
*              followed by:
*                 result  - 8 bytes
*                 op1     - 8 byte source
*                 op2     - 8 byte source
*                 r3      - 8 byte source
*---------------------------------------------------------------------
*  C8x6 CAL   - Compare And Load (32 bits)
*---------------------------------------------------------------------
* CC = 0 (equal) if r3 = op1; r3 = op2
         SSF   CAL,0
         DC    XL8'FF000000 00000004'   result
         DC    XL8'00000001 000000AA'   op1 (first 32 bits)
         DC    XL8'00000004 000000BB'   op2 (first 32 bits)
         DC    XL8'FF000000 00000001'   r3

         SSF   CAL,0
         DC    XL8'FFFFFFFF A0000004'   result
         DC    XL8'00000001 AAAAAAAA'   op1 (first 32 bits)
         DC    XL8'A0000004 BBBBBBBB'   op2 (first 32 bits)
         DC    XL8'FFFFFFFF 00000001'   r3

         SSF   CAL,0
         DC    XL8'FFFFFFFF 99999999'   result
         DC    XL8'00000001 AAAAAAAA'   op1 (first 32 bits)
         DC    XL8'99999999 BBBBBBBB'   op2 (first 32 bits)
         DC    XL8'FFFFFFFF 00000001'   r3

* CC = 1 (not equal) if r3 != op1; r3 = op1
         SSF   CAL,1
         DC    XL8'FF000000 00000001'   result
         DC    XL8'00000001 000000FF'   op1 (first 32 bits)
         DC    XL8'00000004 000000FF'   op2 (first 32 bits)
         DC    XL8'FF000000 00000000'   r3

         SSF   CAL,1
         DC    XL8'FFFFFFFF 00000DDD'   result
         DC    XL8'00000DDD AAAAAAAA'   op1 (first 32 bits)
         DC    XL8'A0000004 BBBBBBBB'   op2 (first 32 bits)
         DC    XL8'FFFFFFFF 0000CCCC'   r3

         SSF   CAL,1
         DC    XL8'FFFFFFFF 0000CCCC'   result
         DC    XL8'0000CCCC AAAAAAAA'   op1 (first 32 bits)
         DC    XL8'99999999 BBBBBBBB'   op2 (first 32 bits)
         DC    XL8'FFFFFFFF 00009999'   r3

*---------------------------------------------------------------------
*  C8x7 CALG  - Compare And Load Long (64 bits)
*---------------------------------------------------------------------
* CC = 0 (equal) if r3 = op1; r3 = op2
         SSF   CALG,0
         DC    XL8'00000004 000000BB'   result
         DC    XL8'FF000000 00000001'   op1
         DC    XL8'00000004 000000BB'   op2
         DC    XL8'FF000000 00000001'   r3

         SSF   CALG,0
         DC    XL8'A0000004 BBBBBBBB'   result
         DC    XL8'FFFFFFFF 00000001'   op1
         DC    XL8'A0000004 BBBBBBBB'   op2
         DC    XL8'FFFFFFFF 00000001'   r3

         SSF   CALG,0
         DC    XL8'99999999 BBBBBBBB'   result
         DC    XL8'FFFFFF00 00000CCC'   op1
         DC    XL8'99999999 BBBBBBBB'   op2
         DC    XL8'FFFFFF00 00000CCC'   r3

* CC = 1 (not equal) if r3 != op1; r3 = op1
         SSF   CALG,1
         DC    XL8'00000001 000000FF'   result
         DC    XL8'00000001 000000FF'   op1
         DC    XL8'00000004 000000FF'   op2
         DC    XL8'FF000000 00000000'   r3

         SSF   CALG,1
         DC    XL8'00000DDD AAAAAAAA'   result
         DC    XL8'00000DDD AAAAAAAA'   op1
         DC    XL8'A0000004 BBBBBBBB'   op2
         DC    XL8'FFFFFFFF 0000CCCC'   r3

         SSF   CALG,1
         DC    XL8'CCCC0000 AAAAAAAA'   result
         DC    XL8'CCCC0000 AAAAAAAA'   op1
         DC    XL8'99999999 BBBBBBBB'   op2
         DC    XL8'FFFFFFFF 00009999'   r3

*---------------------------------------------------------------------
*  C8xF CALGF - Compare And Load Long Fullword (64 <- 32)
*---------------------------------------------------------------------
* CC = 0 (equal) if r3 = op1; r3 = op2
         SSF   CALGF,0
         DC    XL8'00000004 000000BB'   result
         DC    XL8'00000001 000000AA'   op1 (first 32 bits)
         DC    XL8'00000004 000000BB'   op2
         DC    XL8'FF000000 00000001'   r3

         SSF   CALGF,0
         DC    XL8'A0000004 BBBBBBBB'   result
         DC    XL8'000000CC AAAAAAAA'   op1 (first 32 bits)
         DC    XL8'A0000004 BBBBBBBB'   op2
         DC    XL8'FFFFFFFF 000000CC'   r3

         SSF   CALGF,0
         DC    XL8'99999999 BBBBBBBB'   result
         DC    XL8'0000DDDD AAAAAAAA'   op1 (first 32 bits)
         DC    XL8'99999999 BBBBBBBB'   op2
         DC    XL8'FFFFFFFF 0000DDDD'   r3

* CC = 1 (not equal) if r3 != op1; r3 = op1
         SSF   CALGF,1
         DC    XL8'00000000 00000001'   result
         DC    XL8'00000001 000000FF'   op1 (first 32 bits)
         DC    XL8'00000004 000000FF'   op2
         DC    XL8'FF0000FF 11111111'   r3

         SSF   CALGF,1
         DC    XL8'00000000 00000DDD'   result
         DC    XL8'00000DDD AAAAAAAA'   op1 (first 32 bits)
         DC    XL8'A0000004 BBBBBBBB'   op2
         DC    XL8'FFFFFFFF 0000CCCC'   r3

         SSF   CALGF,1
         DC    XL8'00000000 0000CCCC'   result
         DC    XL8'0000CCCC AAAAAAAA'   op1 (first 32 bits)
         DC    XL8'99999999 BBBBBBBB'   op2
         DC    XL8'FFFFFFFF 00009999'   r3


         DC    F'0'                    END OF TABLE
         DC    F'0'
*
* table of pointers to individual tests
*
CFFTESTS  DS    0F
         PTTABLE
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
