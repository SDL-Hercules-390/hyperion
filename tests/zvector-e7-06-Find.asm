 TITLE 'zvector-e7-06-Find (Zvector E7 VRR-b instruction)'
***********************************************************************
*
*   Zvector E7 instruction tests for VRR-b encoded:
*
*   E780 VFEE   - Vector Find Element Equal
*   E781 VFENE  - Vector Find Element Not Equal
*   E782 VFAE   - Vector Find Any Element Equal
*
*        James Wekel February 2025
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E7 VRR-b
*  Vector Find Element Equal, Vector Find Element Not Equal and
*  Vector Find Any Element Equal instructions.
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*    *Testcase zvector-e7-06-Find
*    *
*    *   Zvector E7 instruction tests for VRR-b encoded:
*    *
*    *   E780 VFEE   - Vector Find Element Equal
*    *   E781 VFENE  - Vector Find Element Not Equal
*    *   E782 VFAE   - Vector Find Any Element Equal
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
*    loadcore    "$(testpath)/zvector-e7-06-Find.core" 0x0
*
*    diag8cmd    enable    # (needed for messages to Hercules console)
*    runtest     10        #
*    diag8cmd    disable   # (reset back to default)
*
*    *Done
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
         DS    FD                         gap
V1OUTPUT DS    XL16                    V1 Output
         DS    FD                         gap

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
         VRR_B &INST,&M4,&M5,&CC
.*                               &INST   - VRR-b instruction under test
.*                               &M4     - m4 field - element size
.*                               &M5     - m5 field - IN, RT, ZS, CS
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
         DC    HL1'&M5'                m5 used
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
         DS    FD                 gap
V1O&TNUM DS    XL16               V1 output
         DS    FD                 gap
.*
*
X&TNUM   DS    0F
         LGF   R1,V2ADDR          load v2 source
         VL    v22,0(R1)          use v21 to test decoder
         LGF   R1,V3ADDR          load v3 source
         VL    v23,0(R1)          use v22 to test decoder

         &INST V21,V22,V23,&M4,&M5            test instruction

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
*   E780 VFEE   - Vector Find Element Equal
*   E781 VFENE  - Vector Find Element Not Equal
*   E782 VFAE   - Vector Find Any Element Equal
*
*        VRR-b instruction,
*              M4,                     element size
*              M5,                     IN, RT, ZS, CS
*              CC                      expected condition code
*
*              followed by
*                 16 byte V1 result
*                 16 byte V2 source
*                 16 byte V3 source

*---------------------------------------------------------------------
* VFEE   - Vector Find Element Equal
*---------------------------------------------------------------------

*---------------------------------------------------------------------
* case 0 -  simple debug               CC=1                ZS=0, CS=1
*---------------------------------------------------------------------
*
         VRR_B VFEE,0,1,1
         DC    XL16'00000000000000000000000000000000'   V1
         DC    XL16'00000000000000000000000000000000'   v2
         DC    XL16'00000000000000000000000000000000'   v3

*---------------------------------------------------------------------
* case 1 - Hardware Verified
*---------------------------------------------------------------------
* Byte, No equal, no zero              cc=3
         VRR_B VFEE,0,3,3
         DC    XL16'00000000000000100000000000000000'   V1
         DC    XL16'5D3A58595A53595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Byte, No equal, with zero            cc=0
         VRR_B VFEE,0,3,0
         DC    XL16'00000000000000090000000000000000'   V1
         DC    XL16'5D3A58595A53595354004D445F444546'   v2
         DC    XL16'25252525252525252500252525252525'   v3

* Byte, Equal, no zero                 cc=1
         VRR_B VFEE,0,3,1
         DC    XL16'00000000000000050000000000000000'   V1
         DC    XL16'5D3A58595A25595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Byte, Equal before zero              cc=2
         VRR_B VFEE,0,3,2
         DC    XL16'00000000000000050000000000000000'   V1
         DC    XL16'5D3A58595A25595354004D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Byte, Equal after zero               cc=0
         VRR_B VFEE,0,3,0
         DC    XL16'00000000000000050000000000000000'   V1
         DC    XL16'5D3A58595A00595354254D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Halfword, No equal, no zero          cc=3
         VRR_B VFEE,1,3,3
         DC    XL16'00000000000000100000000000000000'   V1
         DC    XL16'5D3A58595A53595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Halfword, Equal, no zero             cc=1
         VRR_B VFEE,1,3,1
         DC    XL16'00000000000000060000000000000000'   V1
         DC    XL16'5D3A58595A53252554454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Halfword, Equal before zero          cc=2
         VRR_B VFEE,1,3,2
         DC    XL16'00000000000000060000000000000000'   V1
         DC    XL16'5D3A58595A532525544500005F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Halfword, No equal, no zero          cc=3
         VRR_B VFEE,1,3,3
         DC    XL16'00000000000000100000000000000000'   V1
         DC    XL16'5D3A58595A53595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Halfword, Equal, no zero             cc=1
         VRR_B VFEE,1,3,1
         DC    XL16'00000000000000060000000000000000'   V1
         DC    XL16'5D3A58595A53252554454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Halfword, Equal before zero          cc=2
         VRR_B VFEE,1,3,2
         DC    XL16'00000000000000060000000000000000'   V1
         DC    XL16'5D3A58595A532525544500005F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Halfword, Equal before zero          cc=0
         VRR_B VFEE,1,3,0
         DC    XL16'00000000000000060000000000000000'   V1
         DC    XL16'5D3A58595A530000544525255F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Word, No equal, no zero              cc=3
         VRR_B VFEE,2,3,3
         DC    XL16'00000000000000100000000000000000'   V1
         DC    XL16'5D3A58595A53595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Word, Equal, no zero                 cc=1
         VRR_B VFEE,2,3,1
         DC    XL16'00000000000000040000000000000000'   V1
         DC    XL16'5D3A58592525252554454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Word, Equal before zero              cc=2
         VRR_B VFEE,2,3,2
         DC    XL16'00000000000000040000000000000000'   V1
         DC    XL16'5D3A585925252525000000005F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Word, Equal before zero              cc=0
         VRR_B VFEE,2,3,0
         DC    XL16'00000000000000040000000000000000'   V1
         DC    XL16'5D3A585900000000252525255F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3
                                                                EJECT
*---------------------------------------------------------------------
* VFENE  - Vector Find Element Not Equal
*---------------------------------------------------------------------

*---------------------------------------------------------------------
* case 0 -  simple debug               CC=1                ZS=0, CS=1
*---------------------------------------------------------------------
*
         VRR_B VFENE,0,1,3
         DC    XL16'00000000000000100000000000000000'   V1
         DC    XL16'00000000000000000000000000000000'   v2
         DC    XL16'00000000000000000000000000000000'   v3

*---------------------------------------------------------------------
* case 1 - Hardware Verified
*---------------------------------------------------------------------
* Byte, All equal, no zero             cc=3
         VRR_B VFENE,0,3,3
         DC    XL16'00000000000000100000000000000000'   V1
         DC    XL16'5D3A58595A53595354454D445F444546'   v2
         DC    XL16'5D3A58595A53595354454D445F444546'   v3

* Byte, All equal, with zero           cc=0
         VRR_B VFENE,0,3,0
         DC    XL16'00000000000000090000000000000000'   V1
         DC    XL16'5D3A58595A53595354004D445F444546'   v2
         DC    XL16'5D3A58595A53595354004D445F444546'   v3

* Byte, Not Equal, no zero             cc=1
         VRR_B VFENE,0,3,1
         DC    XL16'00000000000000050000000000000000'   V1
         DC    XL16'5D3A58595A25595354454D445F444546'   v2
         DC    XL16'5D3A58595A53595354454D445F444546'   v3

* Byte, Not Equal before zero          cc=1
         VRR_B VFENE,0,3,1
         DC    XL16'00000000000000050000000000000000'   V1
         DC    XL16'5D3A58595A25595354004D445F444546'   v2
         DC    XL16'5D3A58595A53595354454D445F444546'   v3

* Byte, Not Equal after zero           cc=1
         VRR_B VFENE,0,3,1
         DC    XL16'00000000000000050000000000000000'   V1
         DC    XL16'5D3A58595A00595354254D445F444546'   v2
         DC    XL16'5D3A58595A53595354454D445F444546'   v3

* Halfword, All equal, no zero         cc=3
         VRR_B VFENE,1,3,3
         DC    XL16'00000000000000100000000000000000'   V1
         DC    XL16'5D3A58595A53595354454D445F444546'   v2
         DC    XL16'5D3A58595A53595354454D445F444546'   v3

* Halfword, Not Equal, no zero         cc=1
         VRR_B VFENE,1,3,1
         DC    XL16'00000000000000060000000000000000'   V1
         DC    XL16'5D3A58595A53252554454D445F444546'   v2
         DC    XL16'5D3A58595A53595354454D445F444546'   v3

* Halfword, Not Equal before zero      cc=1
         VRR_B VFENE,1,3,1
         DC    XL16'00000000000000060000000000000000'   V1
         DC    XL16'5D3A58595A532525544500005F444546'   v2
         DC    XL16'5D3A58595A53595354454D445F444546'   v3

* Halfword, Not Equal after zero       cc=1
         VRR_B VFENE,1,3,1
         DC    XL16'00000000000000060000000000000000'   V1
         DC    XL16'5D3A58595A530000544525255F444546'   v2
         DC    XL16'5D3A58595A53595354454D445F444546'   v3

* Word, All equal, no zero             cc=3
         VRR_B VFENE,2,3,3
         DC    XL16'00000000000000100000000000000000'   V1
         DC    XL16'5D3A58595A53595354454D445F444546'   v2
         DC    XL16'5D3A58595A53595354454D445F444546'   v3

* Word, Not Equal, no zero             cc=1
         VRR_B VFENE,2,3,1
         DC    XL16'00000000000000040000000000000000'   V1
         DC    XL16'5D3A58592525252554454D445F444546'   v2
         DC    XL16'5D3A58595A53595354454D445F444546'   v3

* Word, Not Equal before zero          cc=1
         VRR_B VFENE,2,3,1
         DC    XL16'00000000000000040000000000000000'   V1
         DC    XL16'5D3A585925252525000000005F444546'   v2
         DC    XL16'5D3A58595A53595354454D445F444546'   v3

* Word, Not Equal after zero           cc=1
         VRR_B VFENE,2,3,1
         DC    XL16'00000000000000040000000000000000'   V1
         DC    XL16'5D3A585900000000252525255F444546'   v2
         DC    XL16'5D3A58595A53595354454D445F444546'   v3
                                                                EJECT
*---------------------------------------------------------------------
* VFAE   - Vector Find Any Element Equal
*---------------------------------------------------------------------

*---------------------------------------------------------------------
* case 0 -  simple debug               CC=1  IN=0, RT=0, ZS=0, CS=1
*---------------------------------------------------------------------
*
         VRR_B VFAE,0,1,1
         DC    XL16'00000000000000000000000000000000'   V1
         DC    XL16'00000000000000000000000000000000'   v2
         DC    XL16'00000000000000000000000000000000'   v3

*---------------------------------------------------------------------
* case 1 - Hardware Verified
*---------------------------------------------------------------------
* Byte, No equal, no zero
         VRR_B VFAE,0,3,3              cc=3 M5=2+1               ZS CS
         DC    XL16'00000000000000100000000000000000'   V1
         DC    XL16'5D3A58595A53595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Byte, No equal, no zero
         VRR_B VFAE,0,7,3              cc=3 M5=4+2+1          RT ZS CS
         DC    XL16'00000000000000000000000000000000'   V1
         DC    XL16'5D3A58595A53595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Byte, No equal, no zero
         VRR_B VFAE,0,15,1             cc=1 M5=8+4+2+1     IN RT ZS CS
         DC    XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'   V1
         DC    XL16'5D3A58595A53595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Byte, No equal, no zero
         VRR_B VFAE,0,5,3              cc=3 M5=4+1            RT    CS
         DC    XL16'00000000000000000000000000000000'   V1
         DC    XL16'5D3A58595A53595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Byte, No equal, no zero
         VRR_B VFAE,0,13,1              cc=1 M5=8=4+1       IN RT   CS
         DC    XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'   V1
         DC    XL16'5D3A58595A53595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3
                                                                EJECT
*---------------------------------------------------------------------
* Byte, Equal, no zero
         VRR_B VFAE,0,3,1              cc=1 M5=2+1              ZS CS
         DC    XL16'00000000000000050000000000000000'   V1
         DC    XL16'5D3A58595A25595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Byte, Equal, no zero
         VRR_B VFAE,0,7,1              cc=1 M5=4+2+1         RT ZS CS
         DC    XL16'0000000000FF00000000000000000000'   V1
         DC    XL16'5D3A58595A25595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Byte, Equal, no zero
         VRR_B VFAE,0,15,1             cc=1 M5=8+4+2+1    IN RT ZS CS
         DC    XL16'FFFFFFFFFF00FFFFFFFFFFFFFFFFFFFF'   V1
         DC    XL16'5D3A58595A25595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Byte, Equal, no zero
         VRR_B VFAE,0,1,1              cc=1 M5=1                   CS
         DC    XL16'00000000000000050000000000000000'   V1
         DC    XL16'5D3A58595A25595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Byte, Equal, no zero
         VRR_B VFAE,0,5,1              cc=1 M5=4+1           RT    CS
         DC    XL16'0000000000FF00000000000000000000'   V1
         DC    XL16'5D3A58595A25595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3

* Byte, Equal, no zero
         VRR_B VFAE,0,13,1             cc=1 M5=8+4+1      IN RT    CS
         DC    XL16'FFFFFFFFFF00FFFFFFFFFFFFFFFFFFFF'   V1
         DC    XL16'5D3A58595A25595354454D445F444546'   v2
         DC    XL16'25252525252525252525252525252525'   v3
                                                                EJECT
*---------------------------------------------------------------------
* Byte, Equal before zero
         VRR_B VFAE,0,3,2              cc=2 M5=2+1              ZS CS
         DC    XL16'00000000 00000005 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A255953 54004D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Byte, Equal before zero
         VRR_B VFAE,0,7,2              cc=2 M5=4+2+1         RT ZS CS
         DC    XL16'00000000 00FF0000 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A255953 54004D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Byte, Equal before zero
         VRR_B VFAE,0,15,2             cc=2 M5=8+4+2+1    IN RT ZS CS
         DC    XL16'FFFFFFFF FF00FFFF FFFFFFFF FFFFFFFF'   V1
         DC    XL16'5D3A5859 5A255953 54004D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Byte, Equal before zero
         VRR_B VFAE,0,1,1              cc=1 M5=1                   CS
         DC    XL16'00000000 00000005 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A255953 54004D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Byte, Equal before zero
         VRR_B VFAE,0,5,1              cc=1 M5=4+1           RT    CS
         DC    XL16'00000000 00FF0000 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A255953 54004D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Byte, Equal before zero
         VRR_B VFAE,0,13,1              cc=1 M5=8+4+1      IN RT    CS
         DC    XL16'FFFFFFFF FF00FFFF FFFFFFFF FFFFFFFF'   V1
         DC    XL16'5D3A5859 5A255953 54004D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3
                                                                EJECT
*---------------------------------------------------------------------
* Byte Equal after zero
         VRR_B VFAE,0,3,0              cc=0 M5=2+1             ZS CS
         DC    XL16'00000000 00000005 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A005953 54254D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Byte Equal after zero
         VRR_B VFAE,0,7,0              cc=0 M5=4+2+1        RT ZS CS
         DC    XL16'00000000 00000000 00FF0000 00000000'   V1
         DC    XL16'5D3A5859 5A005953 54254D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Byte Equal after zero
         VRR_B VFAE,0,15,2             cc=2 M5=8+4+2+1   IN RT ZS CS
         DC    XL16'FFFFFFFF FFFFFFFF FF00FFFF FFFFFFFF'   V1
         DC    XL16'5D3A5859 5A005953 54254D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Byte Equal after zero
         VRR_B VFAE,0,1,1              cc=1 M5=1                  CS
         DC    XL16'00000000 00000009 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A005953 54254D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Byte Equal after zero
         VRR_B VFAE,0,5,1              cc=1 M5=4+1          RT    CS
         DC    XL16'00000000 00000000 00FF0000 00000000'   V1
         DC    XL16'5D3A5859 5A005953 54254D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Byte Equal after zero
         VRR_B VFAE,0,13,1              cc=1 M5=8+4+1     IN RT    CS
         DC    XL16'FFFFFFFF FFFFFFFF FF00FFFF FFFFFFFF'   V1
         DC    XL16'5D3A5859 5A005953 54254D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3
                                                                EJECT
*---------------------------------------------------------------------
* Halfword, No equal, no zero
         VRR_B VFAE,1,3,3              cc=3 M5=2+1              ZS CS
         DC    XL16'00000000 00000010 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A535953 54454D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Halfword, No equal, no zero
         VRR_B VFAE,1,7,3              cc=3 M5=4+2+1         RT ZS CS
         DC    XL16'00000000 00000000 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A535953 54454D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Halfword, No equal, no zero
         VRR_B VFAE,1,15,1             cc=1 M5=8+4+2+1    IN RT ZS CS
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   V1
         DC    XL16'5D3A5859 5A535953 54454D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Halfword, Equal, no zero
         VRR_B VFAE,1,3,1              cc=1 M5=2+1              ZS CS
         DC    XL16'00000000 00000006 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A532525 54454D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Halfword, Equal, no zero
         VRR_B VFAE,1,7,1              cc=1 M5=4+2+1         RT ZS CS
         DC    XL16'00000000 0000FFFF 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A532525 54454D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Halfword, Equal, no zero
         VRR_B VFAE,1,15,1             cc=1 M5=8+4+2+1    IN RT ZS CS
         DC    XL16'FFFFFFFF FFFF0000 FFFFFFFF FFFFFFFF'   V1
         DC    XL16'5D3A5859 5A532525 54454D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Halfword, Equal before zero
         VRR_B VFAE,1,3,2              cc=2 M5=2+1              ZS CS
         DC    XL16'00000000 00000006 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A532525 54450000 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Halfword, Equal before zero
         VRR_B VFAE,1,7,2              cc=2 M5=4+2+1         RT ZS CS
         DC    XL16'00000000 0000FFFF 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A532525 54450000 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Halfword, Equal before zero
         VRR_B VFAE,1,15,2             cc=2 M5=8+4+2+1    IN RT ZS CS
         DC    XL16'FFFFFFFF FFFF0000 FFFFFFFF FFFFFFFF'   V1
         DC    XL16'5D3A5859 5A532525 54450000 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Halfword, Equal after zero
         VRR_B VFAE,1,3,0              cc=0 M5=2+1              ZS CS
         DC    XL16'00000000 00000006 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A530000 54452525 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Halfword, Equal after zero
         VRR_B VFAE,1,7,0              cc=0 M5=4+2+1         RT ZS CS
         DC    XL16'00000000 00000000 0000FFFF 00000000'   V1
         DC    XL16'5D3A5859 5A530000 54452525 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Halfword, Equal after zero
         VRR_B VFAE,1,15,2             cc=2 M5=8+4+2+1    IN RT ZS CS
         DC    XL16'FFFFFFFF FFFFFFFF FFFF0000 FFFFFFFF'   V1
         DC    XL16'5D3A5859 5A530000 54452525 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3
                                                                EJECT
*---------------------------------------------------------------------
* Word, No equal, no zero
         VRR_B VFAE,2,3,3              cc=3 M5=2+1              ZS CS
         DC    XL16'00000000 00000010 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A535953 54454D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Word, No equal, no zero
         VRR_B VFAE,2,7,3              cc=3 M5=4+2+1         RT ZS CS
         DC    XL16'00000000 00000000 00000000 00000000'   V1
         DC    XL16'5D3A5859 5A535953 54454D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Word, No equal, no zero
         VRR_B VFAE,2,15,1              cc=1 M5=8+4+2+1   IN RT ZS CS
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   V1
         DC    XL16'5D3A5859 5A535953 54454D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Word, Equal, no zero
         VRR_B VFAE,2,3,1              cc=1 M5=2+1              ZS CS
         DC    XL16'00000000 00000004 00000000 00000000'   V1
         DC    XL16'5D3A5859 25252525 54454D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Word, Equal, no zero
         VRR_B VFAE,2,7,1              cc=1 M5=4+2+1         RT ZS CS
         DC    XL16'00000000 FFFFFFFF 00000000 00000000'   V1
         DC    XL16'5D3A5859 25252525 54454D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Word, Equal, no zero
         VRR_B VFAE,2,15,1             cc=1 M5=8+4+2+1    IN RT ZS CS
         DC    XL16'FFFFFFFF 00000000 FFFFFFFF FFFFFFFF'   V1
         DC    XL16'5D3A5859 25252525 54454D44 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Word, Equal before zero
         VRR_B VFAE,2,3,2              cc=2 M5=2+1              ZS CS
         DC    XL16'00000000 00000004 00000000 00000000'   V1
         DC    XL16'5D3A5859 25252525 00000000 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Word, Equal before zero
         VRR_B VFAE,2,7,2              cc=2 M5=4+2+1         RT ZS CS
         DC    XL16'00000000 FFFFFFFF 00000000 00000000'   V1
         DC    XL16'5D3A5859 25252525 00000000 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Word, Equal before zero
         VRR_B VFAE,2,15,2             cc=2 M5=8+4+2+1    IN RT ZS CS
         DC    XL16'FFFFFFFF 00000000 FFFFFFFF FFFFFFFF'   V1
         DC    XL16'5D3A5859 25252525 00000000 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Word, Equal after zero
         VRR_B VFAE,2,3,0              cc=0 M5=2+1              ZS CS
         DC    XL16'00000000 00000004 00000000 00000000'   V1
         DC    XL16'5D3A5859 00000000 25252525 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Word, Equal after zero
         VRR_B VFAE,2,7,0              cc=0 M5=4+2+1         RT ZS CS
         DC    XL16'00000000 00000000 FFFFFFFF 00000000'   V1
         DC    XL16'5D3A5859 00000000 25252525 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3

* Word, Equal after zero
         VRR_B VFAE,2,15,2             cc=2 M5=8+4+2+1    IN RT ZS CS
         DC    XL16'FFFFFFFF FFFFFFFF 00000000 FFFFFFFF'   V1
         DC    XL16'5D3A5859 00000000 25252525 5F444546'   v2
         DC    XL16'25252525 25252525 25252525 25252525'   v3



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
