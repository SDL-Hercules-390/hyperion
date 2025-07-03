 TITLE 'zvector-e7-16-PackCompare'
***********************************************************************
*
*   Zvector E7 instruction tests for VRR-b encoded:
*
*   E795 VPKLS  - Vector Pack Logical Saturate
*   E797 VPKS   - Vector Pack Saturate
*   E7F8 VCEQ   - Vector Compare Equal
*   E7F9 VCHL   - Vector Compare High Logical
*   E7FB VCH    - Vector Compare High
*
*        James Wekel March 2025
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E7 VRR-b
*  Pack Logical Saturate, Pack Saturate, Compare, Compare Equal,
*  Compare High Logical instructions.
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*   *Testcase zvector-e7-16-PackCompare
*   *
*   *   Zvector E7 instruction tests for VRR-b encoded:
*   *
*   *   E795 VPKLS  - Vector Pack Logical Saturate
*   *   E797 VPKS   - Vector Pack Saturate
*   *   E7F8 VCEQ   - Vector Compare Equal
*   *   E7F9 VCHL   - Vector Compare High Logical
*   *   E7FB VCH    - Vector Compare High
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
*   loadcore    "$(testpath)/zvector-e7-16-PackCompare.core" 0x0
*
*   diag8cmd    enable    # (needed for messages to Hercules console)
*   runtest     5
*   diag8cmd    disable   # (reset back to default)
*
*   *Done
*
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
         USING BEGIN,R8         FIRST Base Register
         USING BEGIN+4096,R9    SECOND Base Register
         USING BEGIN+8192,R10   THIRD Base Register
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

         L     R12,=A(E7TESTS)         get table of test addresses

NEXTE7   EQU   *
         L     R5,0(0,R12)             get test address
         LTR   R5,R5                      have a test?
         BZ    ENDTEST                       done?

         USING E7TEST,R5

         LH    R0,TNUM                 save current test number
         ST    R0,TESTING              for easy reference

         VL    V22,V1FUDGE             using V22 as v1 for instruction
         L     R11,TSUB                get address of test routine
         BALR  R11,R11                 do test

         LB    R1,CCMASK               (failure CC mask)
         SLL   R1,4                    (shift to BC instr CC position)
         EX    R1,TESTCC               fail if...

TESTREST EQU   *
         LGF   R1,READDR               get address of expected result
         CLC   V1OUTPUT,0(R1)          valid?
         BNE   FAILMSG                    no, issue failed message

         LA    R12,4(0,R12)            next test address
         B     NEXTE7

TESTCC   BC    0,CCMSG                 (unexpected condition code?)
                                                                EJECT
***********************************************************************
* cc was not as expected
***********************************************************************
CCMSG    EQU   *
*
* is CS set by test?
*
         LB    R1,M5                   Get M5
         NG    R1,=D'1'                  issolate CS
         BZ    TESTREST                    not set?
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
*              and instruction m4, m5
***********************************************************************
FAILMSG  EQU   *
         LH    R2,TNUM              get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTNUM(3),PRT3+13    fill in message with test #

         MVC   PRTNAME,OPNAME       fill in message with instruction

         XGR   R2,R2                get M4 as U8
         IC    R2,M4
         CVD   R2,DECNUM            and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM4(3),PRT3+13     fill in message with M4 field

         XGR   R2,R2                get M5 as U8
         IC    R2,M5
         CVD   R2,DECNUM            and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM5(3),PRT3+13     fill in message with M5 field

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
         L     R1,FAILED               did a test fail?
         LTR   R1,R1
         BZ    EOJ                     No, exit
         B     FAILTEST                Yes, exit with BAD PSW
                                                               EJECT
***********************************************************************
*        RPTERROR          Report instruction test in error
*                             R0 = MESSGAE LENGTH
*                             R1 = ADDRESS OF MESSAGE
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
                                                               SPACE 2
***********************************************************************
*        Issue HERCULES MESSAGE pointed to by R1, length in R0
*              R2 = return address
***********************************************************************
                                                                SPACE
MSG      CH    R0,=H'0'                Do we even HAVE a message?
         BNHR  R2                      No, ignore
                                                                SPACE
         STM   R0,R2,MSGSAVE           Save registers
                                                                SPACE
         CH    R0,=AL2(L'MSGMSG)       Message length within limits?
         BNH   MSGOK                   Yes, continue
         LA    R0,L'MSGMSG             No, set to maximum
                                                                SPACE
MSGOK    LR    R2,R0                   Copy length to work register
         BCTR  R2,0                    Minus-1 for execute
         EX    R2,MSGMVC               Copy message to O/P buffer
                                                                SPACE
         LA    R2,1+L'MSGCMD(,R2)      Calculate true command length
         LA    R1,MSGCMD               Point to true command
                                                                SPACE
         DC    X'83',X'12',X'0008'     Issue Hercules Diagnose X'008'
         BZ    MSGRET                  Return if successful

         LTR   R2,R2                   Is Diag8 Ry (R2) 0?
         BZ    MSGRET                    an error occurred but coninue

         DC    H'0'                    CRASH for debugging purposes
                                                                SPACE
MSGRET   LM    R0,R2,MSGSAVE           Restore registers
         BR    R2                      Return to caller
                                                                SPACE 6
MSGSAVE  DC    3F'0'                   Registers save area
MSGMVC   MVC   MSGMSG(0),0(R1)         Executed instruction
                                                                SPACE 2
MSGCMD   DC    C'MSGNOH * '            *** HERCULES MESSAGE COMMAND ***
MSGMSG   DC    CL95' '                 The message text to be displayed

                                                                EJECT
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 3
EOJPSW   DC    0D'0',X'0002000180000000',AD(0)
                                                                SPACE
EOJ      LPSWE EOJPSW                  Normal completion
                                                                SPACE 3
FAILPSW  DC    0D'0',X'0002000180000000',AD(X'BAD')
                                                                SPACE
FAILTEST LPSWE FAILPSW                 Abnormal termination
                                                                SPACE 3
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE 2
CTLR0    DS    F                       CR0
         DS    F
                                                                SPACE
         LTORG ,                       Literals pool

*        some constants

K        EQU   1024                    One KB
PAGE     EQU   (4*K)                   Size of one page
K64      EQU   (64*K)                  64 KB
MB       EQU   (K*K)                    1 MB

REG2PATT EQU   X'AABBCCDD'             Polluted Register pattern
REG2LOW  EQU         X'DD'             (last byte above)
                                                                EJECT
*======================================================================
*
*  NOTE: start data on an address that is easy to display
*        within Hercules
*
*======================================================================

         ORG   ZVE7TST+X'1000'
FAILED   DC    F'0'                    some test failed?
TESTING  DC    F'0'                    current test number
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
         DC    C' with m4='
PRTM4    DC    C'xxx'
         DC    C','
         DC    C' with m5='
PRTM5    DC    C'xxx'
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
                                                                SPACE 3
***********************************************************************
*        E7TEST DSECT
***********************************************************************
                                                                SPACE 2
E7TEST   DSECT ,
TSUB     DC    A(0)                    pointer  to test
TNUM     DC    H'00'                   Test Number
         DC    X'00'
M4       DC    HL1'00'                 m4 used
M5       DC    HL1'00'                 m5 used
CC       DC    HL1'00'                 cc expected
CCMASK   DC    HL1'00'                 not expected CC mask
*
*        CC extrtaction
*
CCPSW    DS    2F                      extract PSW after test (has CC)
CCFOUND  DS    X                       extracted cc

OPNAME   DC    CL8' '                  E7 name
V1ADDR   DC    A(0)                    address of v1 result
V2ADDR   DC    A(0)                    address of v2 source
V3ADDR   DC    A(0)                    address of v3 source
RELEN    DC    A(0)                    RESULT LENGTH
READDR   DC    A(0)                    result (expected) address
         DS    2FD                         gap
V1OUTPUT DS    XL16                    V1 Output
         DS    2FD                         gap

*        test routine will be here (from VRR-b macro)
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
                                                                SPACE 2
*
* macro to generate individual test
*
         MACRO
         VRR_B &INST,&M4,&CC
.*                               &INST   - VRR-b instruction under test
.*                               &M4     - m4 field - element size
.*                               &CC     - expected CC

         LCLA  &XCC(4)  &XCC has mask values for FAILED condition codes
&XCC(1)  SETA  7                 CC != 0
&XCC(2)  SETA  11                CC != 1
&XCC(3)  SETA  13                CC != 2
&XCC(4)  SETA  14                CC != 3

         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5               base for test data and test routine

T&TNUM   DC    A(X&TNUM)          address of test routine
         DC    H'&TNUM'           test number
         DC    X'00'
         DC    HL1'&M4'                m4 used
         DC    HL1'1'                  m5 used
         DC    HL1'&CC'           CC
         DC    HL1'&XCC(&CC+1)'   CC failed mask

         DS    2F                 extracted PSW after test (has CC)
         DC    X'FF'              extracted CC, if test failed

         DC    CL8'&INST'         instruction name
         DC    A(RE&TNUM)         address of v1 result
         DC    A(RE&TNUM+16)      address of v2 source
         DC    A(RE&TNUM+32)      address of v3 source
         DC    A(16)              result length
REA&TNUM DC    A(RE&TNUM)         result address
         DS    2FD                 gap
V1O&TNUM DS    XL16               V1 output
         DS    2FD                 gap
.*
*
X&TNUM   DS    0F
         LGF   R1,V2ADDR          load v2 source
         VL    v22,0(R1)          use v21 to test decoder
         LGF   R1,V3ADDR          load v3 source
         VL    v23,0(R1)          use v22 to test decoder

         &INST V21,V22,V23,&M4,1            test instruction

         EPSW  R2,R0              extract psw
         ST    R2,CCPSW              to save CC

         VST   V21,V1O&TNUM       save v1 output

         BR    R11                return

RE&TNUM  DC    0F                 V1 for this test

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
         DC    A(T&CUR)                test address
.*
&CUR     SETA  &CUR+1
         AIF   (&CUR LE &TNUM).LOOP
*
         DC    A(0)                    end of table
         DC    A(0)                    end of table
.*
         MEND

                                                                EJECT
***********************************************************************
*        E7 VRR-b tests
***********************************************************************
         PRINT DATA
*
*
*   E795 VPKLS  - Vector Pack Logical Saturate
*   E797 VPKS   - Vector Pack Saturate
*   E7F8 VCEQ   - Vector Compare Equal
*   E7F9 VCHL   - Vector Compare High Logical
*   E7FB VCH    - Vector Compare High
*
*        VRR-b instruction,
*              M4,                     element size
*              CC                      expected condition code
*
*              followed by
*                 16 byte V1 result
*                 16 byte V2 source
*                 16 byte V3 source
*
* NOTE:  M5 is preset to 1; Condition Code Set (CS)
*
*---------------------------------------------------------------------
* VPKLS  - Vector Pack Logical Saturate
*---------------------------------------------------------------------
* cc=0: No saturation
* cc=1: At least one but not all elements saturated
* cc=3: Saturation on all elements
*---------------------------------------------------------------------
* case -  simple cc debug
*---------------------------------------------------------------------
*Halfword
         VRR_B VPKLS,1,0
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VPKLS,1,1
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v3

         VRR_B VPKLS,1,3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v3

*Word
         VRR_B VPKLS,2,0
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VPKLS,2,1
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v3

         VRR_B VPKLS,2,3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v3

*DoubleWord
         VRR_B VPKLS,3,0
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VPKLS,3,1
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v3

         VRR_B VPKLS,3,3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v3

*---------------------------------------------------------------------
* case -  general
*---------------------------------------------------------------------
*Halfword
         VRR_B VPKLS,1,0
         DC    XL16'1133557799BBDDFF FEFDFCFBFAF9F8F7'   result
         DC    XL16'0011003300550077 009900BB00DD00FF'   v2
         DC    XL16'00FE00FD00FC00FB 00FA00F900F800F7'   v3

         VRR_B VPKLS,1,0
         DC    XL16'FEFDFCFBFAF9F8F7 1133557799BBDDFF'   result
         DC    XL16'00FE00FD00FC00FB 00FA00F900F800F7'   v2
         DC    XL16'0011003300550077 009900BB00DD00FF'   v3

         VRR_B VPKLS,1,1
         DC    XL16'01FFFFFFFFFFFFFF 1133557799BBDDFF'   result
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'0011003300550077 009900BB00DD00FF'   v3

         VRR_B VPKLS,1,1
         DC    XL16'1133557799BBDDFF 01FFFFFFFFFFFFFF'   result
         DC    XL16'0011003300550077 009900BB00DD00FF'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VPKLS,1,3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0111013301550177 019901BB01DD01FF'   v2
         DC    XL16'0101020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VPKLS,1,3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0101020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'0111013301550177 019901BB01DD01FF'   v3

*Word
         VRR_B VPKLS,2,0
         DC    XL16'1133557799BBDDFF FEFDFCFBFAF9F8F7'   result
         DC    XL16'0000113300005577 000099BB0000DDFF'   v2
         DC    XL16'0000FEFD0000FCFB 0000FAF90000F8F7'   v3

         VRR_B VPKLS,2,0
         DC    XL16'FEFDFCFBFAF9F8F7 1133557799BBDDFF'   result
         DC    XL16'0000FEFD0000FCFB 0000FAF90000F8F7'   v2
         DC    XL16'0000113300005577 000099BB0000DDFF'   v3

         VRR_B VPKLS,2,1
         DC    XL16'1203FFFFFFFFFFFF 1133557799BBDDFF'   result
         DC    XL16'0000120304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'0000113300005577 000099BB0000DDFF'   v3

         VRR_B VPKLS,2,1
         DC    XL16'1133557799BBDDFF 1203FFFFFFFFFFFF'   result
         DC    XL16'0000113300005577 000099BB0000DDFF'   v2
         DC    XL16'0000120304050607 08090A0B0C0D0E0F'   v3

         VRR_B VPKLS,2,3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0111013301550177 019901BB01DD01FF'   v2
         DC    XL16'0101020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VPKLS,2,3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0101020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'0111013301550177 019901BB01DD01FF'   v3

*Doubleword
         VRR_B VPKLS,3,0
         DC    XL16'1133557799BBDDFF FEFDFCFBFAF9F8F7'   result
         DC    XL16'0000000011335577 0000000099BBDDFF'   v2
         DC    XL16'00000000FEFDFCFB 00000000FAF9F8F7'   v3

         VRR_B VPKLS,3,0
         DC    XL16'FEFDFCFBFAF9F8F7 1133557799BBDDFF'   result
         DC    XL16'00000000FEFDFCFB 00000000FAF9F8F7'   v2
         DC    XL16'0000000011335577 0000000099BBDDFF'   v3

         VRR_B VPKLS,3,1
         DC    XL16'FEFDFCFBFAF9F8F7 FFFFFFFFFFFFFFFF'   result
         DC    XL16'00000000FEFDFCFB 00000000FAF9F8F7'   v2
         DC    XL16'0000113300005577 000099BB0000DDFF'   v3

         VRR_B VPKLS,3,1
         DC    XL16'FFFFFFFFFFFFFFFF FEFDFCFBFAF9F8F7'   result
         DC    XL16'0000113300005577 000099BB0000DDFF'   v2
         DC    XL16'00000000FEFDFCFB 00000000FAF9F8F7'   v3

         VRR_B VPKLS,3,3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0111013301550177 019901BB01DD01FF'   v2
         DC    XL16'0101020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VPKLS,3,3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0101020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'0111013301550177 019901BB01DD01FF'   v3
*---------------------------------------------------------------------
* misc...
*---------------------------------------------------------------------
         VRR_B VPKLS,1,0
         DC    XL16'51535557 595B5D5F 61636567 696B6D6F'   result
         DC    XL16'00510053 00550057 0059005B 005D005F'   v2
         DC    XL16'00610063 00650067 0069006B 006D006F'   v3

         VRR_B VPKLS,2,0
         DC    XL16'52535657 5A5B5E5F 62636667 6A6B6E6F'   result
         DC    XL16'00005253 00005657 00005A5B 00005E5F'   v2
         DC    XL16'00006263 00006667 00006A6B 00006E6F'   v3

         VRR_B VPKLS,3,0
         DC    XL16'54555657 5C5D5E5F 64656667 6C6D6E6F'   result
         DC    XL16'00000000 54555657 00000000 5C5D5E5F'   v2
         DC    XL16'00000000 64656667 00000000 6C6D6E6F'   v3

*---------------------------------------------------------------------
* VPKS   - Vector Pack Saturate
*---------------------------------------------------------------------
* cc=0: No saturation
* cc=1: At least one but not all elements saturated
* cc=3: Saturation on all elements
*---------------------------------------------------------------------
* case -  simple cc debug
*---------------------------------------------------------------------
*Halfword
         VRR_B VPKS,1,0
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VPKS,1,1
         DC    XL16'0000000000000000 7F7F7F7F80808080'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0FFF0FFF0FFF0FFF 8FFF8FFF8FFF8FFF'   v3

         VRR_B VPKS,1,3
         DC    XL16'7F7F7F7F80808080 7F7F7F7F80808080'   result
         DC    XL16'0FFF0FFF0FFF0FFF 8FFF8FFF8FFF8FFF'   v2
         DC    XL16'0FFF0FFF0FFF0FFF 8FFF8FFF8FFF8FFF'   v3

*Word
         VRR_B VPKS,2,0
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VPKS,2,1
         DC    XL16'0000000000000000 7FFF7FFF80008000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000FFFF0000FFFF 8FFF8FFF8FFF8FFF'   v3

         VRR_B VPKS,2,3
         DC    XL16'7FFF7FFF80008000 7FFF7FFF80008000'   result
         DC    XL16'0000FFFF0000FFFF 8FFF8FFF8FFF8FFF'   v2
         DC    XL16'0000FFFF0000FFFF 8FFF8FFF8FFF8FFF'   v3

*DoubleWord
         VRR_B VPKS,3,0
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VPKS,3,1
         DC    XL16'000000007FFFFFFF 7FFFFFFF80000000'   result
         DC    XL16'0000000000000000 0FFFFFFFFFFFFFFF'   v2
         DC    XL16'00000000FFFFFFFF 8FFFFFFFFFFFFFFF'   v3

         VRR_B VPKS,3,3
         DC    XL16'7FFFFFFF80000000 7FFFFFFF80000000'   result
         DC    XL16'000000FFFFFFFFFF 8FFFFFFFFFFFFFFF'   v2
         DC    XL16'000000FFFFFFFFFF 8FFFFFFFFFFFFFFF'   v3

*---------------------------------------------------------------------
* case -  general
*---------------------------------------------------------------------
*Halfword
         VRR_B VPKS,1,0
         DC    XL16'1133557722446608 FEFDFCFBFAF9F8F7'   result
         DC    XL16'0011003300550077 0022004400660008'   v2
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v3

         VRR_B VPKS,1,0
         DC    XL16'FEFDFCFBFAF9F8F7 1133557722446608'   result
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v2
         DC    XL16'0011003300550077 0022004400660008'   v3

         VRR_B VPKS,1,1
         DC    XL16'017F7F7F7F7F7F80 1133557722446600'   result
         DC    XL16'0001020304050607 08090A0B0C0DFE0F'   v2
         DC    XL16'0011003300550077 0022004400660000'   v3

         VRR_B VPKS,1,1
         DC    XL16'1133557722446600 017F7F7F7F7F7F80'   result
         DC    XL16'0011003300550077 0022004400660000'   v2
         DC    XL16'0001020304050607 08090A0B0C0DFE0F'   v3

         VRR_B VPKS,1,3
         DC    XL16'7F7F7F7F7F7F7F7F 7F7F7F7F7F7F7F7F'   result
         DC    XL16'0111013301550177 019901BB01DD01FF'   v2
         DC    XL16'0101020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VPKS,1,3
         DC    XL16'7F7F7F7F7F7F7F7F 7F7F7F7F7F7F7F7F'   result
         DC    XL16'0101020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'0111013301550177 019901BB01DD01FF'   v3

         VRR_B VPKS,1,3
         DC    XL16'8080808080808080 8080808080808080'   result
         DC    XL16'F111F133F155F177 F199F1BBF1DDF1FF'   v2
         DC    XL16'F101F203F405F607 F809FAFBFCFDFE0F'   v3

         VRR_B VPKS,1,3
         DC    XL16'8080808080808080 8080808080808080'   result
         DC    XL16'F101F203F405F607 F809FAFBFCFDFE0F'   v2
         DC    XL16'F111F133F155F177 F199F1BBF1DDF1FF'   v3

*Word
         VRR_B VPKS,2,0
         DC    XL16'1133557722446688 FEFDFCFBFAF9F8F7'   result
         DC    XL16'0000113300005577 0000224400006688'   v2
         DC    XL16'FFFFFEFDFFFFFCFB FFFFFAF9FFFFF8F7'   v3

         VRR_B VPKS,2,0
         DC    XL16'FEFDFCFBFAF9F8F7 1133557722446688'   result
         DC    XL16'FFFFFEFDFFFFFCFB FFFFFAF9FFFFF8F7'   v2
         DC    XL16'0000113300005577 0000224400006688'   v3

         VRR_B VPKS,2,1
         DC    XL16'12037FFF7FFF7FFF 1133557719BB2DFF'   result
         DC    XL16'0000120304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'0000113300005577 000019BB00002DFF'   v3

         VRR_B VPKS,2,1
         DC    XL16'1133557719BB2DFF 12037FFF7FFF7FFF'   result
         DC    XL16'0000113300005577 000019BB00002DFF'   v2
         DC    XL16'0000120304050607 08090A0B0C0D0E0F'   v3

         VRR_B VPKS,2,3
         DC    XL16'7FFF7FFF7FFF7FFF 7FFF7FFF7FFF7FFF'   result
         DC    XL16'0111013301550177 019901BB01DD01FF'   v2
         DC    XL16'0101020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VPKS,2,3
         DC    XL16'7FFF7FFF7FFF7FFF 7FFF7FFF7FFF7FFF'   result
         DC    XL16'0101020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'0111013301550177 019901BB01DD01FF'   v3

         VRR_B VPKS,2,3
         DC    XL16'8000800080008000 8000800080008000'   result
         DC    XL16'F111F133F155F177 F199F1BBF1DDF1FF'   v2
         DC    XL16'F101F203F405F607 F809FAFBFCFDFE0F'   v3

         VRR_B VPKS,2,3
         DC    XL16'8000800080008000 8000800080008000'   result
         DC    XL16'F101F203F405F607 F809FAFBFCFDFE0F'   v2
         DC    XL16'F111F133F155F177 F199F1BBF1DDF1FF'   v3

*Doubleword
         VRR_B VPKS,3,0
         DC    XL16'1133557722446688 FEFDFCFBFAF9F8F7'   result
         DC    XL16'0000000011335577 0000000022446688'   v2
         DC    XL16'FFFFFFFFFEFDFCFB FFFFFFFFFAF9F8F7'   v3

         VRR_B VPKS,3,0
         DC    XL16'FEFDFCFBFAF9F8F7 1133557722446688 '  result
         DC    XL16'FFFFFFFFFEFDFCFB FFFFFFFFFAF9F8F7'   v2
         DC    XL16'0000000011335577 0000000022446688'   v3

         VRR_B VPKS,3,1
         DC    XL16'120304057FFFFFFF 1133557719BB2DFF'   result
         DC    XL16'0000000012030405 08090A0B0C0D0E0F'   v2
         DC    XL16'0000000011335577 0000000019BB2DFF'   v3

         VRR_B VPKS,3,1
         DC    XL16'1133557719BB2DFF 120304057FFFFFFF'   result
         DC    XL16'0000000011335577 0000000019BB2DFF'   v2
         DC    XL16'0000000012030405 08090A0B0C0D0E0F'   v3

         VRR_B VPKS,3,3
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   result
         DC    XL16'0111013301550177 019901BB01DD01FF'   v2
         DC    XL16'0101020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VPKS,3,3
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   result
         DC    XL16'0101020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'0111013301550177 019901BB01DD01FF'   v3

         VRR_B VPKS,3,3
         DC    XL16'8000000080000000 8000000080000000'   result
         DC    XL16'F111F133F155F177 F199F1BBF1DDF1FF'   v2
         DC    XL16'F101F203F405F607 F809FAFBFCFDFE0F'   v3

         VRR_B VPKS,3,3
         DC    XL16'8000000080000000 8000000080000000'   result
         DC    XL16'F101F203F405F607 F809FAFBFCFDFE0F'   v2
         DC    XL16'F111F133F155F177 F199F1BBF1DDF1FF'   v3

*---------------------------------------------------------------------
* misc...
*---------------------------------------------------------------------
         VRR_B VPKS,1,0
         DC    XL16'51535557 595B5D5F 61636567 696B6D6F'   result
         DC    XL16'00510053 00550057 0059005B 005D005F'   v2
         DC    XL16'00610063 00650067 0069006B 006D006F'   v3

         VRR_B VPKS,2,0
         DC    XL16'52535657 5A5B5E5F 62636667 6A6B6E6F'   result
         DC    XL16'00005253 00005657 00005A5B 00005E5F'   v2
         DC    XL16'00006263 00006667 00006A6B 00006E6F'   v3

         VRR_B VPKS,3,0
         DC    XL16'54555657 5C5D5E5F 64656667 6C6D6E6F'   result
         DC    XL16'00000000 54555657 00000000 5C5D5E5F'   v2
         DC    XL16'00000000 64656667 00000000 6C6D6E6F'   v3

*---------------------------------------------------------------------
* VCEQ   - Vector Compare Equal
*---------------------------------------------------------------------
* cc=0: All elements equal
* cc=1: At least one, but not all elements equal
* cc=3: No element equal
*---------------------------------------------------------------------
* case -  simple cc debug
*---------------------------------------------------------------------
*Byte
         VRR_B VCEQ,0,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VCEQ,0,1
         DC    XL16'FFFFFFFFFFFFFFFF 00000000FFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 8FFF8FFF00000000'   v3

         VRR_B VCEQ,0,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

*Halfword
         VRR_B VCEQ,1,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VCEQ,1,1
         DC    XL16'FFFFFFFFFFFFFFFF 00000000FFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 8FFF8FFF00000000'   v3

         VRR_B VCEQ,1,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

*Word
         VRR_B VCEQ,2,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VCEQ,2,1
         DC    XL16'FFFFFFFFFFFFFFFF 00000000FFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 8FFF8FFF00000000'   v3

         VRR_B VCEQ,2,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

*Doubleword
         VRR_B VCEQ,3,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VCEQ,3,1
         DC    XL16'FFFFFFFFFFFFFFFF 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 8FFF8FFF00000000'   v3

         VRR_B VCEQ,3,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

*---------------------------------------------------------------------
* case -  general
*---------------------------------------------------------------------
*Byte
         VRR_B VCEQ,0,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0011003300550077 0022004400660008'   v2
         DC    XL16'0011003300550077 0022004400660008'   v3

         VRR_B VCEQ,0,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v2
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v3

         VRR_B VCEQ,0,1
         DC    XL16'FF00000000000000 FFFFFFFFFFFFFFFF'   result
         DC    XL16'0001020304050607 08090A0B0C0DFE0F'   v2
         DC    XL16'0011003300550077 08090A0B0C0DFE0F'   v3

         VRR_B VCEQ,0,1
         DC    XL16'FFFFFFFFFFFFFFFF FF00000000000000'   result
         DC    XL16'08090A0B0C0DFE0F 0001020304050607'   v2
         DC    XL16'08090A0B0C0DFE0F 0011003300550077'   v3

         VRR_B VCEQ,0,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0111013301550177 019901BB01DD01FF'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCEQ,0,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'0111013301550177 019901BB01DD01FF'   v3

*Halfword
         VRR_B VCEQ,1,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0011003300550077 0022004400660008'   v2
         DC    XL16'0011003300550077 0022004400660008'   v3

         VRR_B VCEQ,1,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v2
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v3

         VRR_B VCEQ,1,1
         DC    XL16'FFFF000000000000 FFFFFFFFFFFFFFFF'   result
         DC    XL16'0001020304050607 08090A0B0C0DFE0F'   v2
         DC    XL16'0001003300550077 08090A0B0C0DFE0F'   v3

         VRR_B VCEQ,1,1
         DC    XL16'FFFFFFFFFFFFFFFF 000000000000FFFF'   result
         DC    XL16'08090A0B0C0DFE0F 0001020304050607'   v2
         DC    XL16'08090A0B0C0DFE0F 0011003300550607'   v3

         VRR_B VCEQ,1,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0111013301550177 019901BB01DD01FF'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCEQ,1,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'0111013301550177 019901BB01DD01FF'   v3

*Word
         VRR_B VCEQ,2,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0011003300550077 0022004400660008'   v2
         DC    XL16'0011003300550077 0022004400660008'   v3

         VRR_B VCEQ,2,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v2
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v3

         VRR_B VCEQ,2,1
         DC    XL16'FFFFFFFF00000000 FFFFFFFFFFFFFFFF'   result
         DC    XL16'0001020304050607 08090A0B0C0DFE0F'   v2
         DC    XL16'0001020300550077 08090A0B0C0DFE0F'   v3

         VRR_B VCEQ,2,1
         DC    XL16'FFFFFFFFFFFFFFFF 00000000FFFFFFFF'   result
         DC    XL16'08090A0B0C0DFE0F 0001020304050607'   v2
         DC    XL16'08090A0B0C0DFE0F 0011003304050607'   v3

         VRR_B VCEQ,2,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0111013301550177 019901BB01DD01FF'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCEQ,2,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'0111013301550177 019901BB01DD01FF'   v3

*Doubleword
         VRR_B VCEQ,3,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0011003300550077 0022004400660008'   v2
         DC    XL16'0011003300550077 0022004400660008'   v3

         VRR_B VCEQ,3,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v2
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v3

         VRR_B VCEQ,3,1
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   result
         DC    XL16'0001020304050607 08090A0B0C0DFE0F'   v2
         DC    XL16'0011003300550077 08090A0B0C0DFE0F'   v3

         VRR_B VCEQ,3,1
         DC    XL16'FFFFFFFFFFFFFFFF 0000000000000000'   result
         DC    XL16'08090A0B0C0DFE0F 0001020304050607'   v2
         DC    XL16'08090A0B0C0DFE0F 0011003300550077'   v3

         VRR_B VCEQ,3,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0111013301550177 019901BB01DD01FF'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCEQ,3,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'0111013301550177 019901BB01DD01FF'   v3

*---------------------------------------------------------------------
* VCHL   - Vector Compare High Logical
*---------------------------------------------------------------------
* cc=0: All elements highl
* cc=1: Some elements high
* cc=3: No element high
*---------------------------------------------------------------------
* case -  simple cc debug
*---------------------------------------------------------------------
*Byte
         VRR_B VCHL,0,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VCHL,0,1
         DC    XL16'0000000000000000 FFFFFFFF00000000'   result
         DC    XL16'0000000000000000 8FFF8FFF00000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VCHL,0,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

*Halfword
         VRR_B VCHL,1,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VCHL,1,1
         DC    XL16'0000000000000000 FFFFFFFF00000000'   result
         DC    XL16'0000000000000000 8FFF8FFF00000000'   v3
         DC    XL16'0000000000000000 0000000000000000'   v2

         VRR_B VCHL,1,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

*Word
         VRR_B VCHL,2,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VCHL,2,1
         DC    XL16'0000000000000000 FFFFFFFF00000000'   result
         DC    XL16'0000000000000000 8FFF8FFF00000000'   v3
         DC    XL16'0000000000000000 0000000000000000'   v2

         VRR_B VCHL,2,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

*Doubleword
         VRR_B VCHL,3,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VCHL,3,1
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   result
         DC    XL16'0000000000000000 8FFF8FFF00000000'   v3
         DC    XL16'0000000000000000 0000000000000000'   v2

         VRR_B VCHL,3,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

*---------------------------------------------------------------------
* case -  general
*---------------------------------------------------------------------
*Byte
         VRR_B VCHL,0,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCHL,0,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCHL,0,1
         DC    XL16'00FF00FF00FF00FF 00000000000000FF'   result
         DC    XL16'0011003300550077 08090A0B0C0DFE1F'   v2
         DC    XL16'0001020304050607 08090A0B0C0DFE0F'   v3

         VRR_B VCHL,0,1
         DC    XL16'00000000000000FF 00FF00FF00FF00FF'   result
         DC    XL16'08090A0B0C0DFE1F 0011003300550077'   v2
         DC    XL16'08090A0B0C0DFE0F 0001020304050607'   v3

         VRR_B VCHL,0,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0001000304050607 00090A0B0C0D0E0F'   v2
         DC    XL16'0111023311550677 119911BBF1DD11FF'   v3

         VRR_B VCHL,0,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'08090A0B0C0D0E0F 0001020304050607'   v2
         DC    XL16'119911BBF1DD11FF 0111023311550677'   v3

*Halfword
         VRR_B VCHL,1,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCHL,1,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCHL,1,1
         DC    XL16'FFFFFFFFFFFFFFFF 000000000000FFFF'   result
         DC    XL16'0011003300550077 08090A0B0C0DFE1F'   v2
         DC    XL16'0001002300450067 08090A0B0C0DFE0F'   v3

         VRR_B VCHL,1,1
         DC    XL16'000000000000FFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'08090A0B0C0DFE1F 0011003300550077'   v2
         DC    XL16'08090A0B0C0DFE0F 0001002300450067'   v3

         VRR_B VCHL,1,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0001000304050607 00090A0B0C0D0E0F'   v2
         DC    XL16'0111023311550677 119911BBF1DD11FF'   v3

         VRR_B VCHL,1,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'08090A0B0C0D0E0F 0001020304050607'   v2
         DC    XL16'119911BBF1DD11FF 0111023311550677'   v3

*Word
         VRR_B VCHL,2,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCHL,2,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCHL,2,1
         DC    XL16'FFFFFFFFFFFFFFFF 00000000FFFFFFFF'   result
         DC    XL16'0011003300550077 08090A0B0C0DFE1F'   v2
         DC    XL16'0001002300450067 08090A0B0C0DFE0F'   v3

         VRR_B VCHL,2,1
         DC    XL16'00000000FFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'08090A0B0C0DFE1F 0011003300550077'   v2
         DC    XL16'08090A0B0C0DFE0F 0001002300450067'   v3

         VRR_B VCHL,2,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0001000304050607 00090A0B0C0D0E0F'   v2
         DC    XL16'0111023311550677 119911BBF1DD11FF'   v3

         VRR_B VCHL,2,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'08090A0B0C0D0E0F 0001020304050607'   v2
         DC    XL16'119911BBF1DD11FF 0111023311550677'   v3

*Doubleword
         VRR_B VCHL,3,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCHL,3,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCHL,3,1
         DC    XL16'FFFFFFFFFFFFFFFF 0000000000000000'   result
         DC    XL16'0011003300550077 08090A0B0C0DFE0F'   v2
         DC    XL16'0001002300450067 08090A0B0C0DFE1F'   v3

         VRR_B VCHL,3,1
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   result
         DC    XL16'08090A0B0C0DFE0F 0011003300550077'   v2
         DC    XL16'08090A0B0C0DFE1F 0001002300450067'   v3

         VRR_B VCHL,3,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0001000304050607 00090A0B0C0D0E0F'   v2
         DC    XL16'0111023311550677 119911BBF1DD11FF'   v3

         VRR_B VCHL,3,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'08090A0B0C0D0E0F 0001020304050607'   v2
         DC    XL16'119911BBF1DD11FF 0111023311550677'   v3

*---------------------------------------------------------------------
* VCH    - Vector Compare High
*---------------------------------------------------------------------
* cc=0: All elements highl
* cc=1: Some elements high
* cc=3: No element high
*---------------------------------------------------------------------
* case -  simple cc debug
*---------------------------------------------------------------------
*Byte
         VRR_B VCH,0,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v3

         VRR_B VCH,0,1
         DC    XL16'0000000000000000 FFFFFFFF00000000'   result
         DC    XL16'0000000000000000 7F017F0200000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VCH,0,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

*Halfword
         VRR_B VCH,1,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v3

         VRR_B VCH,1,1
         DC    XL16'0000000000000000 FFFFFFFF00000000'   result
         DC    XL16'0000000000000000 7F017F0200000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VCH,1,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

*Word
         VRR_B VCH,2,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v3

         VRR_B VCH,2,1
         DC    XL16'0000000000000000 FFFFFFFF00000000'   result
         DC    XL16'0000000000000000 7F017F0200000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VCH,2,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

*Doubleword
         VRR_B VCH,3,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v3

         VRR_B VCH,3,1
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   result
         DC    XL16'0000000000000000 7F017F0200000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

         VRR_B VCH,3,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0000000000000000 0000000000000000'   v3

*---------------------------------------------------------------------
* case -  general
*---------------------------------------------------------------------
*Byte
         VRR_B VCH,0,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCH,0,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v3

         VRR_B VCH,0,1
         DC    XL16'00FF00FF00FF00FF 00000000000000FF'   result
         DC    XL16'0011003300550077 08090A0B0C0DFE1F'   v2
         DC    XL16'0001020304050607 08090A0B0C0DFE0F'   v3

         VRR_B VCH,0,1
         DC    XL16'00000000000000FF 00FF00FF00FF00FF'   result
         DC    XL16'08090A0B0C0DFE1F 0011003300550077'   v2
         DC    XL16'08090A0B0C0DFE0F 0001020304050607'   v3

         VRR_B VCH,0,1
         DC    XL16'FFFFFFFFFFFFFFFF 0000000000000000'   result
         DC    XL16'0001020304050607 FFFAFFF9FFF8FFF7'   v2
         DC    XL16'FFFEFFFDFFFCFFFB 08090A0B0C0D0E0F'   v3

         VRR_B VCH,0,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0001000304050607 00090A0B0C0D0E0F'   v2
         DC    XL16'0111023311550677 1179116B514D312F'   v3

         VRR_B VCHL,0,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'08090A0B0C0D0E0F 0001020304050607'   v2
         DC    XL16'1179116B514D312F 0111023311550677'   v3

         VRR_B VCH,0,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v2
         DC    XL16'0111023311550677 08090A0B0C0D0E0F'   v3

*Halfword
         VRR_B VCH,1,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCH,1,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v3

         VRR_B VCH,1,1
         DC    XL16'FFFF00000000FFFF 000000000000FFFF'   result
         DC    XL16'0011003300550077 08090A0B0C0DFE1F'   v2
         DC    XL16'0001020304050067 08090A0B0C0DFE0F'   v3

         VRR_B VCH,1,1
         DC    XL16'000000000000FFFF FFFF00000000FFFF'   result
         DC    XL16'08090A0B0C0DFE1F 0011003300550077'   v2
         DC    XL16'08090A0B0C0DFE0F 0001020304050067'   v3

         VRR_B VCH,1,1
         DC    XL16'FFFFFFFFFFFFFFFF 0000000000000000'   result
         DC    XL16'0001020304050607 FFFAFFF9FFF8FFF7'   v2
         DC    XL16'FFFEFFFDFFFCFFFB 08090A0B0C0D0E0F'   v3

         VRR_B VCH,1,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0001000304050607 00090A0B0C0D0E0F'   v2
         DC    XL16'0111023311550677 1179116B514D312F'   v3

         VRR_B VCHL,1,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'08090A0B0C0D0E0F 0001020304050607'   v2
         DC    XL16'1179116B514D312F 0111023311550677'   v3

         VRR_B VCH,1,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v2
         DC    XL16'0111023311550677 08090A0B0C0D0E0F'   v3

*Word
         VRR_B VCH,2,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCH,2,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v3

         VRR_B VCH,2,1
         DC    XL16'FFFFFFFF00000000 00000000FFFFFFFF'   result
         DC    XL16'0011003300550077 08090A0B0C0DFE1F'   v2
         DC    XL16'0001020304050067 08090A0B0C0DFE0F'   v3

         VRR_B VCH,2,1
         DC    XL16'00000000FFFFFFFF FFFFFFFF00000000'   result
         DC    XL16'08090A0B0C0DFE1F 0011003300550077'   v2
         DC    XL16'08090A0B0C0DFE0F 0001020304050067'   v3

         VRR_B VCH,2,1
         DC    XL16'FFFFFFFFFFFFFFFF 0000000000000000'   result
         DC    XL16'0001020304050607 FFFAFFF9FFF8FFF7'   v2
         DC    XL16'FFFEFFFDFFFCFFFB 08090A0B0C0D0E0F'   v3

         VRR_B VCH,2,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0001000304050607 00090A0B0C0D0E0F'   v2
         DC    XL16'0111023311550677 1179116B514D312F'   v3

         VRR_B VCHL,2,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'08090A0B0C0D0E0F 0001020304050607'   v2
         DC    XL16'1179116B514D312F 0111023311550677'   v3

         VRR_B VCH,2,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v2
         DC    XL16'0111023311550677 08090A0B0C0D0E0F'   v3

*Doubleword
         VRR_B VCH,3,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3

         VRR_B VCH,3,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v2
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v3

         VRR_B VCH,3,1
         DC    XL16'FFFFFFFFFFFFFFFF 0000000000000000'   result
         DC    XL16'0011003300550077 08090A0B0C0DFE0F'   v2
         DC    XL16'0001020304050067 08090A0B0C0DFE1F'   v3

         VRR_B VCH,3,1
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   result
         DC    XL16'08090A0B0C0DFE0F 0011003300550077'   v2
         DC    XL16'08090A0B0C0DFE1F 0001020304050067'   v3

         VRR_B VCH,3,1
         DC    XL16'FFFFFFFFFFFFFFFF 0000000000000000'   result
         DC    XL16'0001020304050607 FFFAFFF9FFF8FFF7'   v2
         DC    XL16'FFFEFFFDFFFCFFFB 08090A0B0C0D0E0F'   v3

         VRR_B VCH,3,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0001000304050607 00090A0B0C0D0E0F'   v2
         DC    XL16'0111023311550677 1179116B514D312F'   v3

         VRR_B VCHL,3,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'08090A0B0C0D0E0F 0001020304050607'   v2
         DC    XL16'1179116B514D312F 0111023311550677'   v3

         VRR_B VCH,3,3
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'FFFEFFFDFFFCFFFB FFFAFFF9FFF8FFF7'   v2
         DC    XL16'0111023311550677 08090A0B0C0D0E0F'   v3


         DC    F'0'     END OF TABLE
         DC    F'0'
                                                                EJECT
*
* table of pointers to individual tests
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
