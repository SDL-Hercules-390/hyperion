 TITLE 'zvector-e7-25-VSTRS'
***********************************************************************
*
*   Zvector E7 instruction tests for VRR-d encoded:
*
*   E78B VSTRS  - Vector String Search
*
*        James Wekel March 2025
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E7 VRR-d
*  Vector String Search instruction.
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*   *Testcase zvector-e7-25-VSTRS
*   *
*   *   Zvector E7 instruction tests for VRR-d encoded:
*   *
*   *   E78B VSTRS  - Vector String Search
*   *
*   *   # ------------------------------------------------------------
*   *   #  This tests only the basic function of the instruction.
*   *   #  Exceptions are NOT tested.
*   *   # ------------------------------------------------------------
*   *
*   mainsize    2
*   numcpu      1
*   sysclear
*   archlvl     z/Arch
*
*   loadcore    "$(testpath)/zvector-e7-25-VSTRS.core" 0x0
*
*   diag8cmd    enable    # (needed for messages to Hercules console)
*   runtest     5         #
*   diag8cmd    disable   # (reset back to default)
*
*   *Done
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
         USING  BEGIN,R8        FIRST Base Register
         USING  BEGIN+4096,R9   SECOND Base Register
         USING  BEGIN+8192,R10  THIRD Base Register
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
* Is z/Architecture vector facility installed  (bit 129)
***********************************************************************

         FCHECK 129,'z/Architecture vector facility'
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

         LB    R1,CCMASK               (failure CC mask)
         SLL   R1,4                    (shift to BC instr CC position)
         EX    R1,TESTCC               fail if...

TESTREST EQU   *
         LGF   R1,READDR         get address of expected result
         CLC   V1OUTPUT,0(R1)    valid?
         BNE   FAILMSG              no, issue failed message

         LA    R12,4(0,R12)      next test address
         B     NEXTE7

TESTCC   BC    0,CCMSG                 (unexpected condition code?)
                                                                EJECT
***********************************************************************
* cc was not as expected
***********************************************************************
CCMSG    EQU   *
*
* extract CC from extracted PSW
*
         L     R1,CCPSW
         SRL   R1,12
         N     R1,=XL4'3'
         STC   R1,CCFOUND              save cc
*
* FILL IN MESSAGE
*
         LH    R2,TNUM                 get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   CCPRTNUM(3),PRT3+13     fill in message with test #

         MVC   CCPRTNAME,OPNAME        fill in message with instruction

         XGR   R2,R2                   get CC as U8
         IC    R2,CC
         CVD   R2,DECNUM               and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   CCPRTEXP(1),PRT3+15     fill in message with CC field

         XGR   R2,R2                   get CCFOUND as U8
         IC    R2,CCFOUND
         CVD   R2,DECNUM               and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   CCPRTGOT(1),PRT3+15     fill in message with ccfound

         LA    R0,CCPRTLNG             message length
         LA    R1,CCPRTLINE            messagfe address
         BAL   R15,RPTERROR

         L     R0,=F'1'                set failed test indicator
         ST    R0,FAILED

         B     TESTREST
                                                                  EJECT
***********************************************************************
* result not as expected:
*        issue message with test number, instruction under test
*              and instruction m5, m6
***********************************************************************
FAILMSG  EQU   *
         LH    R2,TNUM              get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTNUM(3),PRT3+13    fill in message with test #

         MVC   PRTNAME,OPNAME       fill in message with instruction

         XGR   R2,R2                get M5 as U8
         IC    R2,M5
         CVD   R2,DECNUM            and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM5(2),PRT3+14     fill in message with M5 field

         XGR   R2,R2                get M6 as U8
         IC    R2,M6
         CVD   R2,DECNUM            and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM6(2),PRT3+14     fill in message with M6 field

         LA    R0,PRTLNG            message length
         LA    R1,PRTLINE           messagfe address
         BAL   R15,RPTERROR
                                                               SPACE 2
***********************************************************************
* continue after a failed test
***********************************************************************
FAILCONT EQU   *
         L     R0,=F'1'                set failed test indicator
         ST    R0,FAILED

         LA    R12,4(0,R12)            next test address
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
*        Use Hercules Diagnose for Message to console
*
         STM   R0,R2,RPTDWSAV       save regs used by MSG
         BAL   R2,MSG               call Hercules console MSG display
         LM    R0,R2,RPTDWSAV       restore regs
                                                               SPACE 2
         L     R5,RPTSVR5           Restore R5
         L     R15,RPTSAVE          Restore return address
         BR    R15                  Return to caller
                                                               SPACE
RPTSAVE  DC    F'0'                 R15 save area
RPTSVR5  DC    F'0'                 R5 save area
                                                               SPACE
RPTDWSAV DC    2D'0'                R0-R2 save area for MSG call
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
         DC    C' with m5='
PRTM5    DC    C'xx'
         DC    C','
         DC    C' with m6='
PRTM6    DC    C'xx'
         DC    C'.'
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
         DS    0F
         DS    XL16                                        gap
V1FUDGE  DC    XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    V1 FUDGE
         DS    XL16
                                                                EJECT
***********************************************************************
*        E7TEST DSECT
***********************************************************************
                                                                SPACE 2
E7TEST   DSECT ,
TSUB     DC    A(0)           pointer  to test
TNUM     DC    H'00'          Test Number
         DC    X'00'
M5       DC    HL1'00'                 m5 used
M6       DC    HL1'00'                 m6 used
CC       DC    HL1'00'                 cc expected
CCMASK   DC    HL1'00'                 not expected CC mask
*
*        CC extrtaction
*
CCPSW    DS    2F                      extract PSW after test (has CC)
CCFOUND  DS    X                       extracted cc

OPNAME   DC    CL8' '         E7 name
V2ADDR   DC    A(0)           address of v2 source
V3ADDR   DC    A(0)           address of v3 source
V4ADDR   DC    A(0)           address of v4 source
RELEN    DC    A(0)           RESULT LENGTH
READDR   DC    A(0)           result (expected) address
         DS    FD                gap
V1OUTPUT DS    XL16           V1 Output
         DS    FD                gap

*        test routine will be here (from VRR-d macro)
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
         VRR_D &INST,&M5,&M6,&CC
.*                               &INST   - VRR-d instruction under test
.*                               &M5     - m5 field - element size
.*                               &M6     - m6 field - ZS
.*                               &CC     - expected CC

         LCLA  &XCC(4)  &XCC has mask values for FAILED condition codes
&XCC(1)  SETA  7                 CC != 0
&XCC(2)  SETA  11                CC != 1
&XCC(3)  SETA  13                CC != 2
&XCC(4)  SETA  14                CC != 3

         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
         DC    HL1'&M5'                m5 used
         DC    HL1'&M6'                m6 used
         DC    HL1'&CC'           CC
         DC    HL1'&XCC(&CC+1)'   CC failed mask

         DS    2F                 extracted PSW after test (has CC)
         DC    X'FF'              extracted CC, if test failed

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
         LGF   R1,V2ADDR         load v2 source
         VL    v22,0(R1)         use v22 to test decoder

         LGF   R1,V3ADDR         load v3 source
         VL    v23,0(R1)         use v23 to test decoder

         LGF   R1,V4ADDR         load v4 source
         VL    v24,0(R1)         use v24 to test decoder

         &INST V22,V22,V23,V24,&M5,&M6  instruction (dest is a source)

         EPSW  R2,R0              extract psw
         ST    R2,CCPSW              to save CC

         VST   V22,V1O&TNUM       save v1 output

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
*        E7 VRR-d tests
***********************************************************************
         PRINT DATA
         DS    FD
*
*   E78B VSTRS  - Vector String Search
*
*        VRR-d instruction, m5, M6, CC
*              followed by
*                 16 byte expected result (V1)
*                 16 byte V2 source
*                 16 byte V3 source
*                 16 byte V4 source
*---------------------------------------------------------------------
* VSTRS  - Vector String Search
*---------------------------------------------------------------------

*---------------------------------------------------------------------
* case 0 - test: ZS=0
*---------------------------------------------------------------------
* test - ZS=0 CC=0
*NO Match
*Byte
         VRR_D VSTRS,0,0,0                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFBFCFDFEFF'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,0,0                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFBFCFDFEFF'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,0,0                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFBFCFDFEFF'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

*Full Match CC=2
*Byte
         VRR_D VSTRS,0,0,2                               full match
         DC    XL16'0000000000000008 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 01020304AAFDFEFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,0,2                               full match
         DC    XL16'0000000000000008 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 01020304AAFDFEFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,0,2                               full match
         DC    XL16'0000000000000008 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 0102030405AAAAFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Partial Match CC=3
*Byte
         VRR_D VSTRS,0,0,3                               partial match
         DC    XL16'000000000000000D 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 AAAAFDFEFF010203'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,0,3                               partial match
         DC    XL16'000000000000000A 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 AAFF010203040506'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,0,3                               partial match
         DC    XL16'0000000000000008 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'000000000000000C 0000000000000000'   v4

                                                                EJECT
*---------------------------------------------------------------------
* case 1 - test: ZS=1
*---------------------------------------------------------------------
*NO Match ZS=1 CC=0
*Byte
         VRR_D VSTRS,0,2,0                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFBFCFDFEFF'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,0                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102030405060008 090A0B0C0D0E0F10'   v2
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFBFCFDFEFF'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,0                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102030405060008 090A0B0C0D0E0F10'   v2
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFBFCFDFEFF'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

*NO Match, Zero char  ZS=1 CC=1
*Byte
         VRR_D VSTRS,0,2,1                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102000405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFBFCFDFEFF'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,1                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102000005060708 090A0B0C0D0E0F10'   v2
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFBFCFDFEFF'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,1                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102030400000000 090A0B0C0D0E0F10'   v2
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFBFCFDFEFF'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

*Full Match  ZS=1 CC=2
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000008 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 01020304AAFDFEFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000008 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 01020304AAFDFEFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000008 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 0102030405AAAAFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Partial Match  ZS=1 CC=3
*Byte
         VRR_D VSTRS,0,2,3                               partial match
         DC    XL16'000000000000000D 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 AAAAFDFEFF010203'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,3                               partial match
         DC    XL16'000000000000000A 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 AAFF010203040506'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,3                               partial match
         DC    XL16'0000000000000008 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'000000000000000C 0000000000000000'   v4

                                                                EJECT
*---------------------------------------------------------------------
* case 2 - full match tests: ZS=1 CC=2
*---------------------------------------------------------------------
*Full Match: at beginning of vector
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 01020304AAFDFEFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 01020304AAFDFEFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 0102030405AAAAFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Full Match: at end of vector
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'000000000000000C 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFB01020304'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'000000000000000C 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFB01020304'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'000000000000000C 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFB01020304'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Full Match: at middle of vector
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000006 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F50102 0304FAFBFCFDFEFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000006 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F50102 0304FAFBFCFDFEFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000004 0000000000000000'   V1
         DC    XL16'F0F1F2F301020304 F4F5FAFBFCFDFEFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Full Match: at beginning of vector (and at end of vector)
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 AAFDFEFF01020304'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 AAFDFEFF01020304'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 AAFDFEFF01020304'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Full Match: at beginning of vector (and partial at end of vector)
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 AAFDFEFFBB010203'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 AAFDFEFFBBBB0102'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'0102030405060708 AAFDFEFF01020304'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

                                                                EJECT
*---------------------------------------------------------------------
* case 3 - full match; V3 substr length from ZS: ZS=1 CC=2
*---------------------------------------------------------------------
*Full Match: at beginning of vector
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 01020304AAFDFEFF'   v2
         DC    XL16'0102030005060700 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 01020304AAFDFEFF'   v2
         DC    XL16'0102000005060000 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 0102030405AAAAFF'   v2
         DC    XL16'0102030400000000 000000000D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Full Match: at end of vector
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'000000000000000C 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFB01020304'   v2
         DC    XL16'0102030005060700 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'000000000000000C 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFB01020304'   v2
         DC    XL16'0102000005060000 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'000000000000000C 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F5F6F7 F8F9FAFB01020304'   v2
         DC    XL16'0102030400000000 000000000D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Full Match: at middle of vector
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000006 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F50102 0304FAFBFCFDFEFF'   v2
         DC    XL16'0102030005060700 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000006 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F50102 0304FAFBFCFDFEFF'   v2
         DC    XL16'0102000005060000 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000004 0000000000000000'   V1
         DC    XL16'F0F1F2F301020304 F4F5FAFBFCFDFEFF'   v2
         DC    XL16'0102030400000000 000000000D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Full Match: at beginning of vector (and at end of vector)
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 AAFDFEFF01020304'   v2
         DC    XL16'0102030005060700 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 AAFDFEFF01020304'   v2
         DC    XL16'0102000005060000 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 AAFDFEFF01020304'   v2
         DC    XL16'0102030400000000 000000000D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Full Match: at beginning of vector (and partial at end of vector)
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 AAFDFEFFBB010203'   v2
         DC    XL16'0102030005060700 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 AAFDFEFFBBBB0102'   v2
         DC    XL16'0102000005060000 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'0102030405060708 AAFDFEFF01020304'   v2
         DC    XL16'0102030400000000 000000000D0E0F10'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

                                                                EJECT
*---------------------------------------------------------------------
* case 4 - full match; V2 str length from ZS: ZS=1 CC=2
*---------------------------------------------------------------------
*Full Match: at beginning of vector
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 00020304AAFDFEFF'   v2
         DC    XL16'0102030005060700 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 00000304AAFDFEFF'   v2
         DC    XL16'0102000005060000 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 0000000005AAAAFF'   v2
         DC    XL16'0102030400000000 000000000D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Full Match: at middle of vector
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000006 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F50102 0300FAFBFCFDFEFF'   v2
         DC    XL16'0102030005060700 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000006 0000000000000000'   V1
         DC    XL16'F0F1F2F3F4F50102 0000FAFBFCFDFEFF'   v2
         DC    XL16'0102000005060000 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000004 0000000000000000'   V1
         DC    XL16'F0F1F2F301020304 00000000FCFDFEFF'   v2
         DC    XL16'0102030400000000 000000000D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Full Match: at beginning of vector (and at end of vector)
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 00FDFEFF01020304'   v2
         DC    XL16'0102030005060700 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 0000FEFF01020304'   v2
         DC    XL16'0102000005060000 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 0000000001020304'   v2
         DC    XL16'0102030400000000 000000000D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Full Match: at beginning of vector (and partial at end of vector)
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 00FDFEFFBB010203'   v2
         DC    XL16'0102030005060700 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 0000FEFFBBBB0102'   v2
         DC    XL16'0102000005060000 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'0102030405060708 0000000001020304'   v2
         DC    XL16'0102030400000000 000000000D0E0F10'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

                                                                EJECT
*---------------------------------------------------------------------
* case 5 - zero length  - full match tests: ZS=1 CC=2
*---------------------------------------------------------------------
*Full Match: zero length in V4
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 01020304AAFDFEFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 01020304AAFDFEFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 0102030405AAAAFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

*Full Match: zero length from ZS
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 01020304AAFDFEFF'   v2
         DC    XL16'0002030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 01020304AAFDFEFF'   v2
         DC    XL16'0000030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'0000000000000000 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 0102030405AAAAFF'   v2
         DC    XL16'0000000005060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000008 0000000000000000'   v4

                                                                EJECT
*---------------------------------------------------------------------
* case 6 - MODEL DEPENDENT - Hercules / z15
*---------------------------------------------------------------------
*No Match = bad substring length
*Halfword
         VRR_D VSTRS,1,0,0                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 01020304AAFDFEFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000003 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,0,0                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'01020304F4F5F6F7 0102030405AAAAFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000005 0000000000000000'   v4

                                                                EJECT
*---------------------------------------------------------------------
* case 7 - misc
*---------------------------------------------------------------------
*Full Match: many common substrings
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000008 0000000000000000'   V1
         DC    XL16'0102030102010201 0102030401020304'   v2
         DC    XL16'0102030400506070 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'000000000000000C 0000000000000000'   V1
         DC    XL16'0102030102010201 0102030501020304'   v2
         DC    XL16'0102030405060000 090A0B0C0D0E0F10'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'000000000000000C 0000000000000000'   V1
         DC    XL16'0102030101030400 0102010401020304'   v2
         DC    XL16'0102030405060708 0000000001020304'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Full Match: ZS=1; v4 substring length > 16; many common substrings
*Byte
         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000008 0000000000000000'   V1
         DC    XL16'0102030102010201 0102030401020304'   v2
         DC    XL16'0102030400506070 090A0B0C0D0E0F10'   v3
         DC    XL16'00000000000000FF 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,2,2                               full match
         DC    XL16'000000000000000C 0000000000000000'   V1
         DC    XL16'0102030102010201 0102030501020304'   v2
         DC    XL16'0102030400000700 090A0B0C0D0E0F10'   v3
         DC    XL16'00000000000000FF 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,2,2                               full match
         DC    XL16'000000000000000C 0000000000000000'   V1
         DC    XL16'0102030101030400 0102010401020304'   v2
         DC    XL16'0102030400000000 0000000001020304'   v3
         DC    XL16'00000000000000FF 0000000000000000'   v4

*No Match: ZS=0; v4 substring length > 16; many common substrings
*Byte
         VRR_D VSTRS,0,0,0                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102030102010201 0102030401020300'   v2
         DC    XL16'0102030400506070 090A0B0C0D0E0F10'   v3
         DC    XL16'00000000000000FF 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,0,0                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102030102010201 0102030501020300'   v2
         DC    XL16'0102030400000700 090A0B0C0D0E0F10'   v3
         DC    XL16'00000000000000FF 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,0,0                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102030101030400 0102010401020300'   v2
         DC    XL16'0102030400000000 0000000001020304'   v3
         DC    XL16'00000000000000FF 0000000000000000'   v4

*Partial Match: ZS=0; v4 substring length > 16; many common substrings
*Note: substr length is a multiple of element size!
*Byte
         VRR_D VSTRS,0,0,3                               partial match
         DC    XL16'000000000000000C 0000000000000000'   V1
         DC    XL16'0102030102010201 0102030401020304'   v2
         DC    XL16'0102030400506070 090A0B0C0D0E0F10'   v3
         DC    XL16'00000000000000FF 0000000000000000'   v4

*Halfword
         VRR_D VSTRS,1,0,3                               partial match
         DC    XL16'000000000000000C 0000000000000000'   V1
         DC    XL16'0102030102010201 0102030501020304'   v2
         DC    XL16'0102030400000700 090A0B0C0D0E0F10'   v3
         DC    XL16'00000000000000FE 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,0,3                               partial match
         DC    XL16'000000000000000C 0000000000000000'   V1
         DC    XL16'0102030101030400 0102010401020304'   v2
         DC    XL16'0102030400000000 0000000001020304'   v3
         DC    XL16'00000000000000FC 0000000000000000'   v4

*No Match: ZS=0; v4 substring length > 16; many common substrings
*Note: substr length is a NOT a multiple of element size!
*      --> Model dependent result: no match
*Halfword
         VRR_D VSTRS,1,0,0                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102030102010201 0102030501020304'   v2
         DC    XL16'0102030400000700 090A0B0C0D0E0F10'   v3
         DC    XL16'00000000000000FF 0000000000000000'   v4

*Word
         VRR_D VSTRS,2,0,0                               no match
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'0102030101030400 0102010401020304'   v2
         DC    XL16'0102030400000000 0000000001020304'   v3
         DC    XL16'00000000000000FF 0000000000000000'   v4


*---------------------------------------------------------------------
* case 8 - validate zvector-e7-25-VSTRS-performance test cases
*---------------------------------------------------------------------
*Byte : zs=1
*Not Found CC=0

         VRR_D VSTRS,0,2,0                               not found
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'3030303030320A6D 6E745F69643A0931'   v2
         DC    XL16'5069643A00007069 646664203E3D2030'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

         VRR_D VSTRS,0,2,0                               not found
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'207368617265643A 3231206D61737465'   v2
         DC    XL16'202D20006D6F756E 74696E666F207061'   v3
         DC    XL16'0000000000000003 0000000000000000'   v4

*Not Found CC=1

         VRR_D VSTRS,0,2,1                               not found
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'2F6770696F2F6472 6976657273000000'   v2
         DC    XL16'2F64726976657273 2F002F7379732F66'   v3
         DC    XL16'0000000000000009 0000000000000000'   v4

         VRR_D VSTRS,0,2,1                               not found
         DC    XL16'0000000000000010 0000000000000000'   V1
         DC    XL16'6D2F647269766572 7300000000000000'   v2
         DC    XL16'2F64726976657273 2F002F7379732F66'   v3
         DC    XL16'0000000000000009 0000000000000000'   v4
*Full Match CC=2

         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'0000000000000003 0000000000000000'   V1
         DC    XL16'723A33202D206465 7670747320646576'   v2
         DC    XL16'202D20006D6F756E 74696E666F207061'   v3
         DC    XL16'0000000000000003 0000000000000000'   v4

         VRR_D VSTRS,0,2,2                               full match
         DC    XL16'000000000000000A 0000000000000000'   V1
         DC    XL16'360A696E6F3A0935 390A5069643A0939'   v2
         DC    XL16'5069643A00007069 646664203E3D2030'   v3
         DC    XL16'0000000000000004 0000000000000000'   v4

*Partial Match CC=3

         VRR_D VSTRS,0,2,3                               partial match
         DC    XL16'000000000000000F 0000000000000000'   V1
         DC    XL16'6E6F646576097365 6375726974796673'   v2
         DC    XL16'73656C696E757866 73002F7379732F66'   v3
         DC    XL16'0000000000000009 0000000000000000'   v4

         VRR_D VSTRS,0,2,3                               partial match
         DC    XL16'0000000000000009 0000000000000000'   V1
         DC    XL16'2F6275732F677069 6F2F647269766572'   v2
         DC    XL16'2F64726976657273 2F002F7379732F66'   v3
         DC    XL16'0000000000000009 0000000000000000'   v4

         VRR_D VSTRS,0,2,3                               partial match
         DC    XL16'000000000000000F 0000000000000000'   V1
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4



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
