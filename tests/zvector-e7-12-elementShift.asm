 TITLE 'zvector-e7-12-elementShift'
***********************************************************************
*
*        Zvector E7 tests for VRS-a encoded instructions:
*
*        E730 VESL   - Vector Element Shift Left
*        E733 VERLL  - Vector Element Rotate Left Logical
*        E738 VESRL  - Vector Element Shift Right Logical
*        E73A VESRA  - Vector Element Shift Right Arithmetic
*
*        James Wekel March 2025
***********************************************************************

***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E7 VRS-a vector
*  element shift instructions (shift left, rotate left logical,
*  shift right logical, shift right arithmetic).
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*    *Testcase zvector-e7-12-elementShift
*    *
*    *        Zvector E7 tests for VRS-a encoded instructions:
*    *
*    *        E730 VESL   - Vector Element Shift Left
*    *        E733 VERLL  - Vector Element Rotate Left Logical
*    *        E738 VESRL  - Vector Element Shift Right Logical
*    *        E73A VESRA  - Vector Element Shift Right Arithmetic
*    *
*    *   # -------------------------------------------------------
*    *   #  This tests only the basic function of the instruction.
*    *   #  Exceptions are NOT tested.
*    *   # -------------------------------------------------------
*    *
*    mainsize    2
*    numcpu      1
*    sysclear
*    archlvl     z/Arch
*
*    loadcore    "$(testpath)/zvector-e7-12-elementShift.core" 0x0
*
*    diag8cmd    enable    # (needed for messages to Hercules console)
*    runtest     2
*    diag8cmd    disable   # (reset back to default)
*
*    *Done
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
SKT&SYSNDX DC  C'          Skipping tests: '
         DC    C&NOTSETMSG
         DC    C' facility (bit &BITNO) is not installed.'
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
                                                               SPACE 2
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

                                                                EJECT
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

BEGIN    BALR  R8,0             Initalize FIRST base register
         BCTR  R8,0             Initalize FIRST base register
         BCTR  R8,0             Initalize FIRST base register

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

         VL    V22,V1FUDGE        fudge input

         L     R11,TSUB          get address of test routine
         BALR  R11,R11           do test

TESTREST EQU   *
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
         L     R2,D2                get D2 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTD2(4),PRT3+12     fill in message with d2 field
*
         LB    R2,m4                get m4 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM4(2),PRT3+14     fill in message with m4 field
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
EOJ      LPSWE EOJPSW               Normal completion
                                                                SPACE 4
FAILPSW  DC    0D'0',X'0002000180000000',AD(X'BAD')
                                                                SPACE
FAILTEST LPSWE FAILPSW              Abnormal termination
                                                                SPACE 4
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
         DC    C' with d2='
PRTD2    DC    C'xxxx'
         DC    C', with m4='
PRTM4    DC    C'xx'
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
*
*        CC extrtaction
*
CCPSW    DS    2F          extract PSW after test (has CC)
CCFOUND  DS    X           extracted cc
                                                                SPACE 2
***********************************************************************
*        Vector instruction results, pollution and input
***********************************************************************
         DS    0FD
V1FUDGE  DC    XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    V1 FUDGE
V1FUDGEB DC    XL16'BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB'    V1 FUDGE b
V1INPUT  DC    CL16'1234567890123456'                    V1 input
         DC    CL14'78901234567890'
         DC    X'D9'

         DS    XL16                                          gap
                                                                EJECT
***********************************************************************
*        E7TEST DSECT
***********************************************************************
                                                                SPACE 2
E7TEST   DSECT ,
TSUB     DC    A(0)           pointer  to test
TNUM     DC    H'00'          Test Number
         DC    X'00'
M4       DC    HL1'00'        m4 used
D2       DC    F'00'          D2 used

OPNAME   DC    CL8' '         E7 name
V3ADDR   DC    A(0)           address of v3 source

RELEN    DC    A(0)           RESULT LENGTH
READDR   DC    A(0)           expected result address
         DS    2FD                gap
V1OUTPUT DS    XL16           V1 Output
         DS    2FD                gap

*
*        test routine will be here (from VRS_A macro)
*
* followed by
*        16-byte EXPECTED RESULT
*        16-byte source
                                                                SPACE 2
ZVE7TST  CSECT ,
         DS    0F
                                                                SPACE 2
***********************************************************************
*     Macros to help build test tables
***********************************************************************
         MACRO
         VRS_A &INST,&D2,&M4
.*                               &INST  - VRS-a instruction under test
.*                               &M4    - m4 field
.*                               &D2    - length (loaded into reg)
.*
         LCLA  &XCC(4)  &CC has mask values for FAILED condition codes
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
         DC    HL1'&M4'          m4
         DC    F'&D2'            D2
         DC    CL8'&INST'        instruction name
         DC    A(RE&TNUM+16)     address of v3 source
         DC    A(16)             result length
REA&TNUM DC    A(RE&TNUM)        result address
         DS    2FD                gap
V1O&TNUM DS    XL16              V1 output
         DS    2FD                gap
.*
*
X&TNUM   DS    0F
         LGF   R1,V3ADDR         load v3 source
         VL    v23,0(R1)         use v22 to test decoder

         &INST V22,V23,&D2,&M4   test instruction (dest is a source)
         VST   V22,V1O&TNUM       save v1 output

         BR    R11               return

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
         DC    A(T&CUR)          address of test
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
*        E7 VRS_A tests
***********************************************************************
ZVE7TST  CSECT ,
         DS    0D
                                                               SPACE 2
         PRINT DATA
*
*        E730 VESL   - Vector Element Shift Left
*        E733 VERLL  - Vector Element Rotate Left Logical
*        E738 VESRL  - Vector Element Shift Right Logical
*        E73A VESRA  - Vector Element Shift Right Arithmetic
*
*        VRS_A instr,d2,m4
*              followed by
*              v1     - 16 byte expected result
*              source - 16 byte source from which to get

*---------------------------------------------------------------------
* VESL   - Vector Element Shift Left
*---------------------------------------------------------------------
* Byte

         VRS_A VESL,0,0
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,1,0
         DC    XL16'02040810 20408000 22448810 5498BAFE'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,4,0
         DC    XL16'10204080 00000000 10204080 A0C0D0F0'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,7,0
         DC    XL16'80000000 00000000 80000000 00008080'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,8,0
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,9,0
         DC    XL16'02040810 20408000 22448810 5498BAFE'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

* Halfword
         VRS_A VESL,0,1
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,1,1
         DC    XL16'02040810 20408100 22448910 5598BBFE'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,4,1
         DC    XL16'10204080 02000800 12204880 ACC0DFF0'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,7,1
         DC    XL16'81000400 10004000 91004400 6600FF80'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,8,1
         DC    XL16'02000800 20008000 22008800 CC00FF00'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,9,1
         DC    XL16'04001000 40000000 44001000 9800FE00'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,16,1
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,17,1
         DC    XL16'02040810 20408100 22448910 5598BBFE'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

* Word
         VRS_A VESL,0,2
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,1,2
         DC    XL16'02040810 20408100 22448910 5599BBFE'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,4,2
         DC    XL16'10204080 02040800 12244880 ACCDDFF0'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,7,2
         DC    XL16'81020400 10204000 91224400 666EFF80'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,8,2
         DC    XL16'02040800 20408000 22448800 CCDDFF00'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,9,2
         DC    XL16'04081000 40810000 44891000 99BBFE00'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,16,2
         DC    XL16'04080000 40800000 44880000 DDFF0000'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,17,2
         DC    XL16'08100000 81000000 89100000 BBFE0000'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,32,2
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,33,2
         DC    XL16'02040810 20408100 22448910 5599BBFE'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

* Doubleword
         VRS_A VESL,0,3
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,1,3
         DC    XL16'02040810 20408100 22448911 5599BBFE'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,4,3
         DC    XL16'10204081 02040800 1224488A ACCDDFF0'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,7,3
         DC    XL16'81020408 10204000 91224455 666EFF80'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,8,3
         DC    XL16'02040810 20408000 224488AA CCDDFF00'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,9,3
         DC    XL16'04081020 40810000 44891155 99BBFE00'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,16,3
         DC    XL16'04081020 40800000 4488AACC DDFF0000'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,17,3
         DC    XL16'08102040 81000000 89115599 BBFE0000'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,32,3
         DC    XL16'10204080 00000000 AACCDDFF 00000000'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,33,3
         DC    XL16'20408100 00000000 5599BBFE 00000000'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,64,3
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESL,65,3
         DC    XL16'02040810 20408100 22448911 5599BBFE'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

*---------------------------------------------------------------------
* VERLL  - Vector Element Rotate Left Logical
*---------------------------------------------------------------------
* Byte

         VRS_A VERLL,0,0
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,1,0
         DC    XL16'02040810 20408001 22448811 5599BBFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,4,0
         DC    XL16'10204080 01020408 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,7,0
         DC    XL16'80010204 08102040 88112244 5566EEFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,8,0
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,9,0
         DC    XL16'02040810 20408001 22448811 5599BBFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2


* Halfword
         VRS_A VERLL,0,1
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,1,1
         DC    XL16'02040810 20408100 22448910 5599BBFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,4,1
         DC    XL16'10204080 02010804 12214884 ACCADFFD'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,7,1
         DC    XL16'81000402 10084020 91084422 6655FFEE'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,8,1
         DC    XL16'02010804 20108040 22118844 CCAAFFDD'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,9,1
         DC    XL16'04021008 40200081 44221089 9955FFBB'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,16,1
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,17,1
         DC    XL16'02040810 20408100 22448910 5599BBFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

* Word
         VRS_A VERLL,0,2
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,1,2
         DC    XL16'02040810 20408100 22448910 5599BBFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,4,2
         DC    XL16'10204080 02040801 12244881 ACCDDFFA'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,7,2
         DC    XL16'81020400 10204008 91224408 666EFFD5'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,8,2
         DC    XL16'02040801 20408010 22448811 CCDDFFAA'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,9,2
         DC    XL16'04081002 40810020 44891022 99BBFF55'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,16,2
         DC    XL16'04080102 40801020 44881122 DDFFAACC'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,17,2
         DC    XL16'08100204 81002040 89102244 BBFF5599'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,32,2
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,33,2
         DC    XL16'02040810 20408100 22448910 5599BBFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

* Doubleword
         VRS_A VERLL,0,3
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,1,3
         DC    XL16'02040810 20408100 22448911 5599BBFE'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,4,3
         DC    XL16'10204081 02040800 1224488A ACCDDFF1'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,7,3
         DC    XL16'81020408 10204000 91224455 666EFF88'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,8,3
         DC    XL16'02040810 20408001 224488AA CCDDFF11'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,9,3
         DC    XL16'04081020 40810002 44891155 99BBFE22'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,16,3
         DC    XL16'04081020 40800102 4488AACC DDFF1122'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,17,3
         DC    XL16'08102040 81000204 89115599 BBFE2244'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,32,3
         DC    XL16'10204080 01020408 AACCDDFF 11224488'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,33,3
         DC    XL16'20408100 02040810 5599BBFE 22448911'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,64,3
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VERLL,65,3
         DC    XL16'02040810 20408100 22448911 5599BBFE'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

*---------------------------------------------------------------------
* VESRL  - Vector Element Shift Right Logical
*---------------------------------------------------------------------
* Byte

         VRS_A VESRL,0,0
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,1,0
         DC    XL16'00010204 08102040 08112244 55666E7F'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,4,0
         DC    XL16'00000000 01020408 01020408 0A0C0D0F'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,7,0
         DC    XL16'00000000 00000001 00000001 01010101'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,8,0
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,9,0
         DC    XL16'00010204 08102040 08112244 55666E7F'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

* Halfword
         VRS_A VESRL,0,1
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,1,1
         DC    XL16'00810204 08102040 08912244 55666EFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,4,1
         DC    XL16'00100040 01020408 01120448 0AAC0DDF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,7,1
         DC    XL16'00020008 00200081 00220089 015501BB'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,8,1
         DC    XL16'00010004 00100040 00110044 00AA00DD'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,9,1
         DC    XL16'00000002 00080020 00080022 0055006E'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,16,1
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,17,1
         DC    XL16'00810204 08102040 08912244 55666EFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

* Word
         VRS_A VESRL,0,2
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,1,2
         DC    XL16'00810204 08102040 08912244 55666EFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,4,2
         DC    XL16'00102040 01020408 01122448 0AACCDDF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,7,2
         DC    XL16'00020408 00204081 00224489 015599BB'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,8,2
         DC    XL16'00010204 00102040 00112244 00AACCDD'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,9,2
         DC    XL16'00008102 00081020 00089122 0055666E'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,16,2
         DC    XL16'00000102 00001020 00001122 0000AACC'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,17,2
         DC    XL16'00000081 00000810 00000891 00005566'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,32,2
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,33,2
         DC    XL16'00810204 08102040 08912244 55666EFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

* Doubleword
         VRS_A VESRL,0,3
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,1,3
         DC    XL16'00810204 08102040 08912244 55666EFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,4,3
         DC    XL16'00102040 81020408 01122448 8AACCDDF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,7,3
         DC    XL16'00020408 10204081 00224489 115599BB'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,8,3
         DC    XL16'00010204 08102040 00112244 88AACCDD'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,9,3
         DC    XL16'00008102 04081020 00089122 4455666E'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,16,3
         DC    XL16'00000102 04081020 00001122 4488AACC'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,17,3
         DC    XL16'00000081 02040810 00000891 22445566'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,32,3
         DC    XL16'00000000 01020408 00000000 11224488'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,33,3
         DC    XL16'00000000 00810204 00000000 08912244'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,64,3
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRL,65,3
         DC    XL16'00810204 08102040 08912244 55666EFF'   result
         DC    XL16'01020408 10204080 11224488 AACCDDFF'   v2

*---------------------------------------------------------------------
* VESRA  - Vector Element Shift Right Arithmetic
*---------------------------------------------------------------------
* Byte

         VRS_A VESRA,0,0
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,1,0
         DC    XL16'C0010204 081020C0 081122C4 D5E6EEFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,4,0
         DC    XL16'F8000000 010204F8 010204F8 FAFCFDFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,7,0
         DC    XL16'FF000000 000000FF 000000FF FFFFFFFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,8,0
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,9,0
         DC    XL16'C0010204 081020C0 081122C4 D5E6EEFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

* Halfword
         VRS_A VESRA,0,1
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,1,1
         DC    XL16'C0810204 08102040 08912244 D566EEFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,4,1
         DC    XL16'F8100040 01020408 01120448 FAACFDDF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,7,1
         DC    XL16'FF020008 00200081 00220089 FF55FFBB'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,8,1
         DC    XL16'FF810004 00100040 00110044 FFAAFFDD'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,9,1
         DC    XL16'FFC00002 00080020 00080022 FFD5FFEE'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,16,1
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,17,1
         DC    XL16'C0810204 08102040 08912244 D566EEFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

* Word
         VRS_A VESRA,0,2
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,1,2
         DC    XL16'C0810204 08102040 08912244 D5666EFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,4,2
         DC    XL16'F8102040 01020408 01122448 FAACCDDF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,7,2
         DC    XL16'FF020408 00204081 00224489 FF5599BB'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,8,2
         DC    XL16'FF810204 00102040 00112244 FFAACCDD'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,9,2
         DC    XL16'FFC08102 00081020 00089122 FFD5666E'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,16,2
         DC    XL16'FFFF8102 00001020 00001122 FFFFAACC'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,17,2
         DC    XL16'FFFFC081 00000810 00000891 FFFFD566'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,32,2
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,33,2
         DC    XL16'C0810204 08102040 08912244 D5666EFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

* Doubleword
         VRS_A VESRA,0,3
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,1,3
         DC    XL16'C0810204 08102040 08912244 55666EFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,4,3
         DC    XL16'F8102040 81020408 01122448 8AACCDDF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,7,3
         DC    XL16'FF020408 10204081 00224489 115599BB'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,8,3
         DC    XL16'FF810204 08102040 00112244 88AACCDD'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,9,3
         DC    XL16'FFC08102 04081020 00089122 4455666E'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,16,3
         DC    XL16'FFFF8102 04081020 00001122 4488AACC'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,17,3
         DC    XL16'FFFFC081 02040810 00000891 22445566'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,32,3
         DC    XL16'FFFFFFFF 81020408 00000000 11224488'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,33,3
         DC    XL16'FFFFFFFF C0810204 00000000 08912244'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,64,3
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2

         VRS_A VESRA,65,3
         DC    XL16'C0810204 08102040 08912244 55666EFF'   result
         DC    XL16'81020408 10204080 11224488 AACCDDFF'   v2



         DC    F'0'     END OF TABLE
         DC    F'0'
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
