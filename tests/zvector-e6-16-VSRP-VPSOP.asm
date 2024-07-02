 TITLE ' zvector-e6-16-VSRP-VPSOP (Zvector E6 VRI-g)'
***********************************************************************
*
*        Zvector E6 instruction tests for VRI-g encoded:
*
*        E659 VSRP   - VECTOR SHIFT AND ROUND DECIMAL
*        E65B VPSOP  - VECTOR PERFORM SIGN OPERATION DECIMAL
*
*        James Wekel June 2024
***********************************************************************

***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E6 VRI-g vector
*  shift and round decimal, and perform sign operation decimal
*  instructions. Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*   *Testcase zvector-e6-16-VSRP-VPSOP: VECTOR E6 VRI-g instructions
*   *
*   *    Zvector E6 tests for VRI-g encoded instruction:
*   *
*   *    E659 VSRP   - VECTOR SHIFT AND ROUND DECIMAL
*   *    E65B VPSOP  - VECTOR PERFORM SIGN OPERATION DECIMAL
*   *
*   *    # ------------------------------------------------------------
*   *    #  This tests only the basic function of the instruction.
*   *    #  Exceptions are NOT tested.
*   *    # ------------------------------------------------------------
*   *
*   mainsize    2
*   numcpu      1
*   sysclear
*   archlvl     z/Arch
*
*   diag8cmd    enable    # (needed for messages to Hercules console)
*   loadcore    "$(testpath)/zvector-e6-16-VSRP-VPSOP.core" 0x0
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
*   R10      Third base register
*   R11      E6TEST call return
*   R12      E6TESTS register
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
* Is Vector packed-decimal facility installed  (bit 134)
***********************************************************************

         FCHECK 134,'vector-packed-decimal'
                                                                EJECT
***********************************************************************
*              Do tests in the E6TESTS table
***********************************************************************

         L     R12,=A(E6TESTS)       get table of test addressess

NEXTE6   EQU   *
         L     R5,0(0,R12)       get test address
         LTR   R5,R5                have a test?
         BZ    ENDTEST                 done?

         XGR   R0,R0             no cc error

         USING E6TEST,R5

         LH    R0,TNUM           save current test number
         ST    R0,TESTING        for easy reference

         VL    V1,V1FUDGE
         L     R11,TSUB          get address of test routine
         BALR  R11,R11           do test

         LB    R1,CCMASK         (failure CC mask)
         SLL   R1,4              (shift to BC instr CC position)
         EX    R1,TESTCC            fail if...

TESTREST EQU   *
         LGF   R1,READDR         get address of expected result
         CLC   V1OUTPUT,0(R1)    valid?
         BNE   FAILMSG              no, issue failed message

         LA    R12,4(0,R12)      next test address
         B     NEXTE6

TESTCC   BC    0,CCMSG          (fail if unexpected condition code)
                                                                EJECT
***********************************************************************
* cc was not as expected
***********************************************************************
CCMSG    EQU   *
         XG    R1,R1
         LB    R1,M5          M5 has CS bit
         N     R1,=F'1'       get CS (CC set) bit
         BZ    TESTREST          ignore if not set
*
* extract CC extracted PSW
*
         L     R1,CCPSW
         SRL   R1,12
         N     R1,=XL4'3'
         STC   R1,CCFOUND     save cc
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

         B     FAILCONT
                                                                 EJECT
***********************************************************************
* result not as expected:
*        issue message with test number, instruction under test
*              and instruction m3
***********************************************************************
FAILMSG  EQU   *
         LH    R2,TNUM              get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTNUM(3),PRT3+13    fill in message with test #

         MVC   PRTNAME,OPNAME       fill in message with instruction

         XGR   R2,R2                get i3 as U8
         IC    R2,I3
         CVD   R2,DECNUM            and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTI3(3),PRT3+13     fill in message with i3 field

         XGR   R2,R2                get i4 as U8
         IC    R2,I4
         CVD   R2,DECNUM            and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTI4(3),PRT3+13     fill in message with i4 field

         XGR   R2,R2                get m5 as U8
         IC    R2,M5                and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM5(2),PRT3+14     fill in message with m5 field

         LA    R0,PRTLNG            message length
         LA    R1,PRTLINE           messagfe address
         BAL   R15,RPTERROR
                                                                SPACE 2
***********************************************************************
* continue after a failed test
***********************************************************************
FAILCONT EQU   *
         L     R0,=F'1'          set GLOBAL failed test indicator
         ST    R0,FAILED

         LA    R12,4(0,R12)      next test address
         B     NEXTE6
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

         ORG   ZVE6TST+X'1000'
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
         DC    C' with i3='
PRTI3    DC    C'xxx'
         DC    C','
         DC    C' with i4='
PRTI4    DC    C'xxx'
         DC    C','
         DC    C' with m5='
PRTM5    DC    C'xx'
         DC    C'.'
PRTLNG   EQU   *-PRTLINE
                                                               SPACE 2
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
         DS    XL16                                          gap
V1OUTPUT DS    XL16                                      V1 OUTPUT
         DS    XL16                                          gap
V1FUDGE  DC    XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    V1 FUDGE
V1INPUT  DC    CL16'1234567890123456'                    V1 input
         DC    CL14'78901234567890'
         DC    X'D9'
         DS    XL16                                          gap
                                                                EJECT
***********************************************************************
*        E6TEST DSECT
***********************************************************************
                                                                SPACE 2
E6TEST   DSECT ,
TSUB     DC    A(0)           pointer  to test
TNUM     DC    H'00'          Test Number
         DC    X'00'
I3       DC    HL1'00'        i3 used
I4       DC    HL1'00'        i4 used
M5       DC    HL1'00'        m5 used
CC       DC    HL1'00'        cc
CCMASK   DC    HL1'00'        not expected CC mask

V2VALUE  DC    A(0)

OPNAME   DC    CL8' '         E6 name

RELEN    DC    A(0)           RESULT LENGTH
READDR   DC    A(0)           expected result address

*        EXPECTED RESULT
**
*        test routine will be here (from VRI_G macro)
                                                                EJECT
***********************************************************************
*     Macros to help build test tables
*----------------------------------------------------------------------
*     VRI_G Macro to help build test tables
***********************************************************************
         MACRO
         VRI_G &INST,&I3,&I4,&M5,&CC
.*                               &INST  - VRI-g instruction under test
.*                               &i3    - i3 field
.*                               &i4    - i4 field
.*                               &m5    - m5 field
.*                               &CC    - expected CC
.*
         LCLA  &XCC(4)  &CC has mask values for FAILED condition codes
&XCC(1)  SETA  7                 CC != 0
&XCC(2)  SETA  11                CC != 1
&XCC(3)  SETA  13                CC != 2
&XCC(4)  SETA  14                CC != 3
.*
         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
         DC    HL1'&I3'          i3
         DC    HL1'&I4'          i4
         DC    HL1'&M5'          m5
         DC    HL1'&CC'          cc
         DC    HL1'&XCC(&CC+1)'  cc failed mask
V2_&TNUM DC    A(RE&TNUM+16)     address of v2: 16-byte packed decimal
         DC    CL8'&INST'        instruction name
         DC    A(16)             result length
REA&TNUM DC    A(RE&TNUM)        result address
.*
*                                INSTRUCTION UNDER TEST ROUTINE

X&TNUM   DS    0F
         LGF   R2,V2_&TNUM       get v2
         VL    V2,0(R2)

         &INST V1,V2,&I3,&I4,&M5  test instruction

         VST   V1,V1OUTPUT       save result
         EPSW  R2,R0             exptract psw
         ST    R2,CCPSW              to save CC
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
*        E6 VRI_G tests
***********************************************************************

ZVE6TST  CSECT ,
         DS    0F
                                                                SPACE 2
         PRINT DATA
*
*        E659 VSRP   - VECTOR SHIFT AND ROUND DECIMAL
*        E65B VPSOP  - VECTOR PERFORM SIGN OPERATION DECIMAL
*
*        VRI_G instr,i3,i4,m5,cc
*              followed by
*              v1 - 16 byte expected result
*              v2 - 16 byte zoned decimal (operand)

*---------------------------------------------------------------------
* VSRP  - VECTOR SHIFT AND ROUND DECIMAL REGISTER
*---------------------------------------------------------------------
* VSRP with some I3, I4 and M5's
* I3 tests
*                     i4=129 (iom=1, rdc=1)
*                     i4=132 (iom=1, rdc=4)
*                     i4=135 (iom=1, rdc=7)
*                     i4=142 (iom=1, rdc=14)
*                     i4=159 (iom=1, rdc=31)
* I4
*                     i4=  0 (drd=0, shamt=0)   shift left
*                     i4=  1 (drd=0, shamt=1)   shift left
*                     i4=  4 (drd=0, shamt=4)   shift left
*                     i4=  7 (drd=0, shamt=7)   shift left
*                     i4= 14 (drd=0, shamt=14)  shift left
*                     i4= 30 (drd=0, shamt=30)  shift left
*                     i4= 31 (drd=0, shamt=31)  shift left

*                     i4=96  (drd=0, shamt=-32) shift right
*                     i4=114 (drd=0, shamt=-14) shift right
*                     i4=121 (drd=0, shamt=-7)  shift right
*                     i4=124 (drd=0, shamt=-4)  shift right
*                     i4=127 (drd=0, shamt=-1)  shift right

*                     i4=129 (drd=1, shamt=1)   shift left
*                     i4=132 (drd=1, shamt=4)   shift left
*                     i4=135 (drd=1, shamt=7)   shift left
*                     i4=142 (drd=1, shamt=14)  shift left
*                     i4=159 (drd=1, shamt=31)  shift left

*                     i4=224 (drd=1, shamt=-32) shift right
*                     i4=225 (drd=1, shamt=-31) shift right
*                     i4=242 (drd=1, shamt=-14) shift right
*                     i4=249 (drd=1, shamt=-7)  shift right
*                     i4=252 (drd=1, shamt=-4)  shift right
*                     i4=255 (drd=1, shamt=-1)  shift right

*
* m5 tests (note: cs is always 1)
*                                   m5=1  (p2=0, p1=0, cs=1)
*                                   m5=3  (p2=0, p1=1, cs=1)
*                                   m5=9  (p2=1, p1=0, cs=1)
*                                   m5=11 (p2=1, p1=1, cs=1)

         VRI_G VSRP,159,0,1,2            shamt=0
         DC    XL16'0000000000000000000000000000022C'    V1
         DC    XL16'0000000000000000000000000000022C'    V2

         VRI_G VSRP,159,1,1,2            shamt=1 (left)
         DC    XL16'0000000000000000000000000000220C'    V1
         DC    XL16'0000000000000000000000000000022C'    V2

         VRI_G VSRP,159,7,1,2            shamt=7 (left)
         DC    XL16'0000000000000000000000220000000C'    V1
         DC    XL16'0000000000000000000000000000022C'    V2

         VRI_G VSRP,159,30,1,3            shamt=30 (left) (overflow)
         DC    XL16'2000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000022C'    V2

         VRI_G VSRP,159,31,1,3            shamt=31 (left) (overflow)
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000022D'    V2

         VRI_G VSRP,159,127,1,2           shamt=-1 (right)
         DC    XL16'0000000000000000000000000000002C'    V1
         DC    XL16'0000000000000000000000000000022C'    V2

         VRI_G VSRP,159,255,1,2           shamt=-1 (right) drd=1
         DC    XL16'0000000000000000000000000000003C'    V1
         DC    XL16'0000000000000000000000000000028C'    V2

         VRI_G VSRP,159,252,1,1           shamt=-4 (right) drd=1
         DC    XL16'0000000000000000000000000000002D'    V1
         DC    XL16'0000000000000000000000000015028D'    V2

         VRI_G VSRP,159,249,1,0           shamt=-7 (right) drd=1
         DC    XL16'0000000000000000000000000000000C'    V1 (note: C)
         DC    XL16'0000000000000000000000000000028D'    V2

         VRI_G VSRP,159,225,1,1           shamt=-31 (right) drd=1
         DC    XL16'0000000000000000000000000000001D'    V1 (note: C)
         DC    XL16'9999000000000000000000000000028D'    V2

         VRI_G VSRP,159,224,1,0           shamt=-32 (right) drd=1
         DC    XL16'0000000000000000000000000000000C'    V1 (note: C)
         DC    XL16'9000000000000000000000000000028D'    V2

* m5 tests (note: cs is always 1)
*                                   m5=1  (p2=0, p1=0, cs=1)
*                                   m5=3  (p2=0, p1=1, cs=1)
*                                   m5=9  (p2=1, p1=0, cs=1)
*                                   m5=11 (p2=1, p1=1, cs=1)

         VRI_G VSRP,159,255,3,2     shamt=-1 (right) drd=1 p1=1
         DC    XL16'0000000000000000000000000000003F'    V1
         DC    XL16'0000000000000000000000000000028D'    V2

         VRI_G VSRP,159,255,9,2    shamt=-1 (right) drd=1  p2=1 p1=0
         DC    XL16'0000000000000000000000000000003C'    V1
         DC    XL16'0000000000000000000000000000028D'    V2

         VRI_G VSRP,159,255,11,2    shamt=-1 (right) drd=1 p2=1 p1=1
         DC    XL16'0000000000000000000000000000003F'    V1
         DC    XL16'0000000000000000000000000000028D'    V2

         VRI_G VSRP,159,0,3,2            shamt=0          p2=0 p1=1
         DC    XL16'0000000000000000000000000000022F'    V1
         DC    XL16'0000000000000000000000000000022C'    V2

         VRI_G VSRP,159,1,9,2            shamt=1 (left)   p2=1 p1=0
         DC    XL16'0000000000000000000000000000220C'    V1
         DC    XL16'0000000000000000000000000000022D'    V2

         VRI_G VSRP,159,7,11,2           shamt=7 (left)   p2=1 p1=1
         DC    XL16'0000000000000000000000220000000F'    V1
         DC    XL16'0000000000000000000000000000022D'    V2

*---------------------------------------------------------------------
* VPSOP - VECTOR PERFORM SIGN OPERATION DECIMAL
*---------------------------------------------------------------------
* VPSOP with some I3, I4 and M5's
* I3 tests
*                     i4=129 (iom=1, rdc=1)
*                     i4=132 (iom=1, rdc=4)
*                     i4=135 (iom=1, rdc=7)
*                     i4=142 (iom=1, rdc=14)
*                     i4=159 (iom=1, rdc=31)
* I4 : so=00
*        i4=128 (nv=1, nz=0, //, so=00, pc=0, sv=0)
*        i4=130 (nv=1, nz=0, //, so=00, pc=1, sv=0)
*
*        i4=192 (nv=1, nz=1, //, so=00, pc=0, sv=0)
*        i4=194 (nv=1, nz=1, //, so=00, pc=1, sv=0)

* I4 : so=01
*        i4=132 (nv=1, nz=0, //, so=01, pc=0, sv=0)
*        i4=134 (nv=1, nz=0, //, so=01, pc=1, sv=0)
*
*        i4=196 (nv=1, nz=1, //, so=01, pc=0, sv=0)
*        i4=198 (nv=1, nz=1, //, so=01, pc=1, sv=0)


* I4 : so=10
*        i4=136 (nv=1, nz=0, //, so=10, pc=0, sv=0)
*        i4=138 (nv=1, nz=0, //, so=10, pc=1, sv=0)
*
*        i4=200 (nv=1, nz=1, //, so=10, pc=0, sv=0)
*        i4=202 (nv=1, nz=1, //, so=10, pc=1, sv=0)

* I4 : so=11
*        i4=140 (nv=1, nz=0, //, so=11, pc=0, sv=0)
*        i4=142 (nv=1, nz=0, //, so=11, pc=1, sv=0)
*
*        i4=204 (nv=1, nz=1, //, so=11, pc=0, sv=0)
*        i4=206 (nv=1, nz=1, //, so=11, pc=1, sv=0)

* m5 tests (note: cs is always 1)
*                                   m5=1  (cs=1)
*
* z/Architecture Principles of Operation, SA22-7832-12.
* Figure 25-4. Operation of VECTOR PERFORM SIGN OPERATION DECIMAL
* tests
*
*------------------------------------------------
* SC=00 (maintain): nv=1 to avoid data exceptions
*------------------------------------------------
* V1:nonzero V2:positive   PC='0' NZ='–'              V1_sign=C  CC=2
         VRI_G VPSOP,159,128,1,2         nz=0
         DC    XL16'0000000000000000000000220000000C'    V1
         DC    XL16'0000000000000000000000220000000F'    V2

         VRI_G VPSOP,159,192,1,2        nz=1
         DC    XL16'0000000000000000000000220000000C'    V1
         DC    XL16'0000000000000000000000220000000F'    V2

* V1:nonzero V2:positive   PC='1' NZ='–'              V1_sign=F  CC=2
         VRI_G VPSOP,159,130,1,2         nz=0
         DC    XL16'0000000000000000000000220000000F'    V1
         DC    XL16'0000000000000000000000220000000A'    V2

         VRI_G VPSOP,159,194,1,2        nz=1
         DC    XL16'0000000000000000000000220000000F'    V1
         DC    XL16'0000000000000000000000220000000A'    V2

* V1:nonzero V2:negative   PC='-' NZ='–'              V1_sign=D  CC=1
         VRI_G VPSOP,159,128,1,1         nz=0  pc=0
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000B'    V2

         VRI_G VPSOP,159,130,1,1         nz=0  pc=1
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000B'    V2

         VRI_G VPSOP,159,192,1,1         nz=1  pc=0
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000B'    V2

         VRI_G VPSOP,159,194,1,1         nz=1  pc=1
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000B'    V2

* V1:nonzero V2:invalid   PC='-' NZ='–'              V1_sign=V2  CC=2
         VRI_G VPSOP,159,128,1,2         nz=0  pc=0
         DC    XL16'00000000000000000000002200000009'    V1
         DC    XL16'00000000000000000000002200000009'    V2

         VRI_G VPSOP,159,130,1,2         nz=0  pc=1
         DC    XL16'00000000000000000000002200000009'    V1
         DC    XL16'00000000000000000000002200000009'    V2

         VRI_G VPSOP,159,192,1,2         nz=1  pc=0
         DC    XL16'00000000000000000000002200000009'    V1
         DC    XL16'00000000000000000000002200000009'    V2

         VRI_G VPSOP,159,194,1,2         nz=1  pc=1
         DC    XL16'00000000000000000000002200000009'    V1
         DC    XL16'00000000000000000000002200000009'    V2

* V1:zero  V2:positive   PC='0' NZ='–'              V1_sign=C  CC=0
         VRI_G VPSOP,159,128,1,0         nz=0  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000000F'    V2

         VRI_G VPSOP,159,192,1,0         nz=1  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000000F'    V2

* V1:zero  V2:positive   PC='1' NZ='–'              V1_sign=F  CC=0
         VRI_G VPSOP,159,130,1,0         nz=0  pc=1
         DC    XL16'0000000000000000000000000000000F'    V1
         DC    XL16'0000000000000000000000000000000C'    V2

         VRI_G VPSOP,159,194,1,0         nz=1  pc=1
         DC    XL16'0000000000000000000000000000000F'    V1
         DC    XL16'0000000000000000000000000000000C'    V2

* V1:zero  V2:negative   PC='0' NZ='0'              V1_sign=C  CC=0
         VRI_G VPSOP,159,128,1,0         nz=0  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000000D'    V2

* V1:zero  V2:negative   PC='1' NZ='0'              V1_sign=F  CC=0
         VRI_G VPSOP,159,130,1,0         nz=0  pc=1
         DC    XL16'0000000000000000000000000000000F'    V1
         DC    XL16'0000000000000000000000000000000D'    V2

* V1:zero  V2:negative   PC='-' NZ='1'              V1_sign=D  CC=0
         VRI_G VPSOP,159,192,1,0         nz=1  pc=0
         DC    XL16'0000000000000000000000000000000D'    V1
         DC    XL16'0000000000000000000000000000000B'    V2

         VRI_G VPSOP,159,194,1,0         nz=1  pc=1
         DC    XL16'0000000000000000000000000000000D'    V1
         DC    XL16'0000000000000000000000000000000B'    V2

* V1:zero  V2:invalid   PC='-' NZ='–'              V1_sign=V2  CC=0
         VRI_G VPSOP,159,128,1,0         nz=0  pc=0
         DC    XL16'00000000000000000000000000000009'    V1
         DC    XL16'00000000000000000000000000000009'    V2

         VRI_G VPSOP,159,130,1,0         nz=0  pc=1
         DC    XL16'00000000000000000000000000000009'    V1
         DC    XL16'00000000000000000000000000000009'    V2

         VRI_G VPSOP,159,192,1,0         nz=1  pc=0
         DC    XL16'00000000000000000000000000000009'    V1
         DC    XL16'00000000000000000000000000000009'    V2

         VRI_G VPSOP,159,194,1,0         nz=1  pc=1
         DC    XL16'00000000000000000000000000000009'    V1
         DC    XL16'00000000000000000000000000000009'    V2

*--------------------------------------------------
* SC=01 (complement): nv=1 to avoid data exceptions
*--------------------------------------------------

* V1:nonzero V2:positive   PC='-' NZ='–'              V1_sign=D  CC=1
         VRI_G VPSOP,159,132,1,1         nz=0  pc=0
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000C'    V2

         VRI_G VPSOP,159,134,1,1         nz=0  pc=1
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000A'    V2

         VRI_G VPSOP,159,196,1,1         nz=1  pc=0
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000F'    V2

         VRI_G VPSOP,159,198,1,1         nz=1  pc=1
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000E'    V2

* V1:nonzero V2:negative   PC='0' NZ='–'              V1_sign=C  CC=2
         VRI_G VPSOP,159,132,1,2         nz=0  pc=0
         DC    XL16'0000000000000000000000220000000C'    V1
         DC    XL16'0000000000000000000000220000000D'    V2

         VRI_G VPSOP,159,196,1,2         nz=1  pc=0
         DC    XL16'0000000000000000000000220000000C'    V1
         DC    XL16'0000000000000000000000220000000D'    V2

* V1:nonzero V2:negative   PC='1' NZ='–'              V1_sign=F  CC=2
         VRI_G VPSOP,159,134,1,2         nz=0  pc=1
         DC    XL16'0000000000000000000000220000000F'    V1
         DC    XL16'0000000000000000000000220000000D'    V2

         VRI_G VPSOP,159,198,1,2         nz=1  pc=1
         DC    XL16'0000000000000000000000220000000F'    V1
         DC    XL16'0000000000000000000000220000000D'    V2

* V1:------ V2:invalid   PC='-' NZ='–'                V1_sign=-  CC=-
*        ???? test without exceptions?


* V1:zero   V2:positive   PC='0' NZ='0'               V1_sign=C  CC=0
         VRI_G VPSOP,159,132,1,0         nz=0  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000000A'    V2

* V1:zero   V2:positive   PC='1' NZ='0'               V1_sign=F  CC=0
         VRI_G VPSOP,159,134,1,0         nz=0  pc=1
         DC    XL16'0000000000000000000000000000000F'    V1
         DC    XL16'0000000000000000000000000000000A'    V2

* V1:zero   V2:positive   PC='-' NZ='1'               V1_sign=D  CC=0
         VRI_G VPSOP,159,196,1,0         nz=1  pc=0
         DC    XL16'0000000000000000000000000000000D'    V1
         DC    XL16'0000000000000000000000000000000A'    V2

         VRI_G VPSOP,159,198,1,0         nz=1  pc=1
         DC    XL16'0000000000000000000000000000000D'    V1
         DC    XL16'0000000000000000000000000000000C'    V2

* V1:zero   V2:negative   PC='0' NZ='-'               V1_sign=C  CC=0
         VRI_G VPSOP,159,132,1,0         nz=0  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000000D'    V2

         VRI_G VPSOP,159,196,1,0         nz=1  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000000B'    V2

* V1:zero   V2:negative   PC='1' NZ='-'               V1_sign=F  CC=0
         VRI_G VPSOP,159,134,1,0         nz=0  pc=1
         DC    XL16'0000000000000000000000000000000F'    V1
         DC    XL16'0000000000000000000000000000000D'    V2

         VRI_G VPSOP,159,198,1,0         nz=1  pc=1
         DC    XL16'0000000000000000000000000000000F'    V1
         DC    XL16'0000000000000000000000000000000B'    V2

*------------------------------------------------------
* SC=10 (force positive): nv=1 to avoid data exceptions
*------------------------------------------------------

* V1:nonzero V2:------   PC='0' NZ='–'              V1_sign=C  CC=2
* V2:positive
         VRI_G VPSOP,159,136,1,2         nz=0  pc=0
         DC    XL16'0000000000000000000000220000000C'    V1
         DC    XL16'0000000000000000000000220000000A'    V2

         VRI_G VPSOP,159,200,1,2         nz=1  pc=0
         DC    XL16'0000000000000000000000220000000C'    V1
         DC    XL16'0000000000000000000000220000000A'    V2
* V2:negative
         VRI_G VPSOP,159,136,1,2         nz=0  pc=0
         DC    XL16'0000000000000000000000220000000C'    V1
         DC    XL16'0000000000000000000000220000000D'    V2

         VRI_G VPSOP,159,200,1,2         nz=1  pc=0
         DC    XL16'0000000000000000000000220000000C'    V1
         DC    XL16'0000000000000000000000220000000B'    V2

* V1:nonzero V2:------   PC='1' NZ='–'              V1_sign=F  CC=2
* V2:positive
         VRI_G VPSOP,159,138,1,2         nz=0  pc=1
         DC    XL16'0000000000000000000000220000000F'    V1
         DC    XL16'0000000000000000000000220000000A'    V2

         VRI_G VPSOP,159,202,1,2         nz=1  pc=1
         DC    XL16'0000000000000000000000220000000F'    V1
         DC    XL16'0000000000000000000000220000000A'    V2
* V2:negative
         VRI_G VPSOP,159,138,1,2         nz=0  pc=1
         DC    XL16'0000000000000000000000220000000F'    V1
         DC    XL16'0000000000000000000000220000000D'    V2

         VRI_G VPSOP,159,202,1,2         nz=1  pc=1
         DC    XL16'0000000000000000000000220000000F'    V1
         DC    XL16'0000000000000000000000220000000B'    V2

* V1:zero V2:------   PC='0' NZ='–'              V1_sign=C  CC=0
* V2:positive
         VRI_G VPSOP,159,136,1,0         nz=0  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000000A'    V2

         VRI_G VPSOP,159,200,1,0         nz=1  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000000A'    V2
* V2:negative
         VRI_G VPSOP,159,136,1,0         nz=0  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000000D'    V2

         VRI_G VPSOP,159,200,1,0         nz=1  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000000B'    V2

* V1:zero V2:------   PC='1' NZ='–'              V1_sign=F  CC=0
* V2:positive
         VRI_G VPSOP,159,138,1,0         nz=0  pc=1
         DC    XL16'0000000000000000000000000000000F'    V1
         DC    XL16'0000000000000000000000000000000A'    V2

         VRI_G VPSOP,159,202,1,0         nz=1  pc=1
         DC    XL16'0000000000000000000000000000000F'    V1
         DC    XL16'0000000000000000000000000000000A'    V2
* V2:negative
         VRI_G VPSOP,159,138,1,0         nz=0  pc=1
         DC    XL16'0000000000000000000000000000000F'    V1
         DC    XL16'0000000000000000000000000000000D'    V2

         VRI_G VPSOP,159,202,1,0         nz=1  pc=1
         DC    XL16'0000000000000000000000000000000F'    V1
         DC    XL16'0000000000000000000000000000000B'    V2

*------------------------------------------------------
* SC=11 (force negative): nv=1 to avoid data exceptions
*------------------------------------------------------

* V1:nonzero V2:------   PC='-' NZ='–'              V1_sign=D  CC=1
* V2:positive  PC=0
         VRI_G VPSOP,159,140,1,1         nz=0  pc=0
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000A'    V2

         VRI_G VPSOP,159,204,1,1         nz=1  pc=0
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000A'    V2
* V2:negative  PC=0
         VRI_G VPSOP,159,140,1,1         nz=0  pc=0
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000D'    V2

         VRI_G VPSOP,159,204,1,1         nz=1  pc=0
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000B'    V2
* V2:positive  PC=1
         VRI_G VPSOP,159,142,1,1         nz=0  pc=1
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000A'    V2

         VRI_G VPSOP,159,206,1,1         nz=1  pc=1
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000A'    V2
* V2:negative  PC=1
         VRI_G VPSOP,159,142,1,1         nz=0  pc=1
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000D'    V2

         VRI_G VPSOP,159,206,1,1         nz=1  pc=1
         DC    XL16'0000000000000000000000220000000D'    V1
         DC    XL16'0000000000000000000000220000000B'    V2

* V1:zero    V2:------   PC='0' NZ='0'              V1_sign=C  CC=0
* V2:positive
         VRI_G VPSOP,159,140,1,0         nz=0  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000000A'    V2
* V2:negative
         VRI_G VPSOP,159,140,1,0         nz=0  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000000000000000000D'    V2

* V1:zero    V2:------   PC='1' NZ='0'              V1_sign=F  CC=0
* V2:positive
         VRI_G VPSOP,159,142,1,0         nz=0  pc=1
         DC    XL16'0000000000000000000000000000000F'    V1
         DC    XL16'0000000000000000000000000000000A'    V2
* V2:negative
         VRI_G VPSOP,159,142,1,0         nz=0  pc=1
         DC    XL16'0000000000000000000000000000000F'    V1
         DC    XL16'0000000000000000000000000000000D'    V2

* V1:zero    V2:------   PC='-' NZ='1'              V1_sign=d  CC=0
* V2:positive
         VRI_G VPSOP,159,204,1,0         nz=1  pc=0
         DC    XL16'0000000000000000000000000000000D'    V1
         DC    XL16'0000000000000000000000000000000A'    V2
* V2:negative
         VRI_G VPSOP,159,204,1,0         nz=1  pc=0
         DC    XL16'0000000000000000000000000000000D'    V1
         DC    XL16'0000000000000000000000000000000B'    V2

* V2:positive
         VRI_G VPSOP,159,206,1,0         nz=1  pc=1
         DC    XL16'0000000000000000000000000000000D'    V1
         DC    XL16'0000000000000000000000000000000A'    V2
* V2:negative
         VRI_G VPSOP,159,206,1,0         nz=1  pc=1
         DC    XL16'0000000000000000000000000000000D'    V1
         DC    XL16'0000000000000000000000000000000B'    V2

*--------------------------------------------------
* SOME cc=3 (overflow) tests with rdc=4
*--------------------------------------------------
* V1:zero  V2:positive   PC='0' NZ='–'             V1_sign=C  CC=0
         VRI_G VPSOP,132,128,1,3         nz=0  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000009990000000000000000F'    V2

* SC=01 (complement): nv=1 to avoid data exceptions
* V1:zero   V2:positive   PC='0' NZ='0'            V1_sign=C  CC=0
         VRI_G VPSOP,132,132,1,3         nz=0  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000009990000000000000000A'    V2

* SC=10 (force positive): nv=1 to avoid data exceptions
* V1:zero V2:------   PC='0' NZ='–'                V1_sign=C  CC=0
* V2:positive
         VRI_G VPSOP,132,136,1,3         nz=0  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000999000000000000000A'    V2

* SC=11 (force negative): nv=1 to avoid data exceptions
* V1:zero    V2:------   PC='0' NZ='0'              V1_sign=C  CC=0
* V2:positive
         VRI_G VPSOP,132,140,1,3         nz=0  pc=0
         DC    XL16'0000000000000000000000000000000C'    V1
         DC    XL16'0000000000000000999000000000000A'    V2

*--------------------------------------------------
* SOME cc=3 (overflow) tests with rdc=7
*--------------------------------------------------
* SC=00 (maintain): nv=1 to avoid data exceptions
* V1:nonzero V2:positive   PC='0' NZ='–'            V1_sign=C  CC=2
         VRI_G VPSOP,135,128,1,3         nz=0
         DC    XL16'0000000000000000000000002000000C'    V1
         DC    XL16'0000000000000000000000222000000F'    V2

* SC=01 (complement): nv=1 to avoid data exceptions
* V1:nonzero V2:positive   PC='-' NZ='–'            V1_sign=D  CC=1
         VRI_G VPSOP,135,132,1,3         nz=0  pc=0
         DC    XL16'0000000000000000000000002000000D'    V1
         DC    XL16'0000000000000000000000222000000C'    V2

* SC=10 (force positive): nv=1 to avoid data exceptions
* V1:nonzero V2:------   PC='0' NZ='–'              V1_sign=C  CC=2
* V2:positive
         VRI_G VPSOP,135,136,1,3         nz=0  pc=0
         DC    XL16'0000000000000000000000002000000C'    V1
         DC    XL16'0000000000000000000000222000000A'    V2

* SC=11 (force negative): nv=1 to avoid data exceptions
* V1:nonzero V2:------   PC='-' NZ='–'              V1_sign=D  CC=1
* V2:positive  PC=0
         VRI_G VPSOP,135,140,1,3         nz=0  pc=0
         DC    XL16'0000000000000000000000002000000D'    V1
         DC    XL16'0000000000000000000000222000000A'    V2

*+++++++++++++++++++++++++++++++++++
*test

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
