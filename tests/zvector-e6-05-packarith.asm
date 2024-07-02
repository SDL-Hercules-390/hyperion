 TITLE 'zvector-e6-05-packarith (Zvector E6 VRI-f packed arithmetic)'
***********************************************************************
*
*        Zvector E6 instruction tests for VRI-f encoded:
*
*        E671 VAP    - VECTOR ADD DECIMAL
*        E673 VSP    - VECTOR SUBTRACT DECIMAL
*        E678 VMP    - VECTOR MULTIPLY DECIMAL
*        E679 VSDP   - VECTOR MULTIPLY AND SHIFT DECIMAL
*        E67A VDP    - VECTOR DIVIDE DECIMAL
*        E67B VRP    - VECTOR REMAINDER DECIMAL
*        E67E VSDP   - VECTOR SHIFT AND DIVIDE DECIMAL
*
*        James Wekel June 2024
***********************************************************************

***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E6 VRI-f vector
*  packed decimal arithmetic instructions. Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*  *Testcase VECTOR E6 VRI-f packed arithmetic instructions
*  *
*  *   Zvector E6 tests for VRI-f encoded packed decimal
*  *   arithmetic instructions:
*  *
*  *   E671 VAP    - VECTOR ADD DECIMAL
*  *   E673 VSP    - VECTOR SUBTRACT DECIMAL
*  *   E678 VMP    - VECTOR MULTIPLY DECIMAL
*  *   E679 VMSP   - VECTOR MULTIPLY AND SHIFT DECIMAL
*  *   E67A VDP    - VECTOR DIVIDE DECIMAL
*  *   E67B VRP    - VECTOR REMAINDER DECIMAL
*  *   E67E VSDP   - VECTOR SHIFT AND DIVIDE DECIMAL
*  *
*  *   # ------------------------------------------------------------
*  *   #  This tests only the basic function of the instruction.
*  *   #  Exceptions are NOT tested.
*  *   # ------------------------------------------------------------
*  *
*  mainsize    2
*  numcpu      1
*  sysclear
*  archlvl     z/Arch
*
*  loadcore    "$(testpath)/zvector-e6-05-packarith.core" 0x0
*
*  diag8cmd    enable    # (needed for messages to Hercules console)
*  runtest     2
*  diag8cmd    disable   # (reset back to default)
*
*  *Done
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
                                                                SPACE
         LA    R9,2048(,R8)     Initalize SECOND base register
         LA    R9,2048(,R9)     Initalize SECOND base register
                                                                SPACE
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

         L     R12,=A(E6TESTS)       get table of test addresses

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
         MVC   CCPRTGOT(1),PRT3+15    fill in message with ccfound

         LA    R0,CCPRTLNG            message length
         LA    R1,CCPRTLINE           messagfe address
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
         L     R1,FAILED            did a test fail?
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
         STM   R0,R2,RPTDWSAV     save regs used by MSG
         BAL   R2,MSG             call Hercules console MSG display
         LM    R0,R2,RPTDWSAV     restore regs
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
         DS    XL16                                        gap
V1OUTPUT DS    XL16                                      V1 OUTPUT
         DS    XL16                                        gap
V1FUDGE  DC    XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    V1 FUDGE
V1INPUT  DC    CL16'1234567890123456'                    V1 input
         DC    CL14'78901234567890'
         DC    X'D9'
V2PACKED DS    XL16                    packed version of macro v2
V3PACKED DS    XL16                    packed version of macro v3
         DS    XL16
                                                                EJECT
***********************************************************************
*        E6TEST DSECT
***********************************************************************
                                                                SPACE 2
E6TEST   DSECT ,
TSUB     DC    A(0)             pointer  to test
TNUM     DC    H'00'            Test Number
         DC    X'00'
I4       DC    HL1'00'          I4 used
M5       DC    HL1'00'          M5 used
CC       DC    HL1'00'          cc
CCMASK   DC    HL1'00'          not expected CC mask

V2VALUE  DC    FD'0'
V3VALUE  DC    FD'0'

OPNAME   DC    CL8' '           E6 name

RELEN    DC    A(0)             result length
READDR   DC    A(0)             expected result address

*        test routine will be here (from VRI_F macro)
*
*        followed by
*              EXPECTED RESULT
                                                                EJECT
***********************************************************************
*     Macros to help build test tables
*----------------------------------------------------------------------
*     VRI_F Macro to help build test tables
***********************************************************************
         MACRO
         VRI_F &INST,&V2,&V3,&I4,&M5,&CC
.*                               &INST - VRI-f instruction under test
.*                               &v2   - binary DW value for V2
.*                               &v3   - binary DW value for V3
.*                               &i4   - i4 field
.*                               &m5   - m5 field
.*                               &CC   - expected CC
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
         DC    HL1'&I4'          i4
         DC    HL1'&M5'          m5
         DC    HL1'&CC'          cc
         DC    HL1'&XCC(&CC+1)'  cc failed mask
V2_&TNUM DC    FD'&V2'           binary value for v2 packed decimal
V3_&TNUM DC    FD'&V3'           binary value for v3 packed decimal
         DC    CL8'&INST'        instruction name
         DC    A(16)             result length
REA&TNUM DC    A(RE&TNUM)        result address
.*
*                                INSTRUCTION UNDER TEST ROUTINE
X&TNUM   DS    0F
         LG    R2,V2_&TNUM       convert v2
         CVDG  R2,V2PACKED
         VL    V2,V2PACKED

         LG    R2,V3_&TNUM       convert v3
         CVDG  R2,V3PACKED
         VL    V3,V3PACKED

         &INST V1,V2,V3,&I4,&M5  test instruction

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
*        E6 VRI_F tests
***********************************************************************
ZVE6TST  CSECT ,
         DS    0F
                                                                SPACE 2
         PRINT DATA
*
*        E671 VAP    - VECTOR ADD DECIMAL
*        E673 VSP    - VECTOR SUBTRACT DECIMAL
*        E678 VMP    - VECTOR MULTIPLY DECIMAL
*        E679 VMSP   - VECTOR MULTIPLY AND SHIFT DECIMAL
*        E67A VDP    - VECTOR DIVIDE DECIMAL
*        E67B VRP    - VECTOR REMAINDER DECIMAL
*        E67E VSDP   - VECTOR SHIFT AND DIVIDE DECIMAL

*        VRI_F instr,v2,v3,i4,m5,cc
*              followed by 16 byte expected result

*---------------------------------------------------------------------
*  VAP    - VECTOR ADD DECIMAL
*---------------------------------------------------------------------
* VAP simple + CC checks
         VRI_F VAP,+10,+12,7,1,2
         DC    XL16'0000000000000000000000000000022C'

         VRI_F VAP,-10,+12,7,1,2
         DC    XL16'0000000000000000000000000000002C'

         VRI_F VAP,+10,-12,7,1,1
         DC    XL16'0000000000000000000000000000002D'

         VRI_F VAP,-10,-12,7,1,1
         DC    XL16'0000000000000000000000000000022D'

         VRI_F VAP,-10,+10,7,1,0
         DC    XL16'0000000000000000000000000000000C'

         VRI_F VAP,+10000000000,+10,135,1,3     i4=135(iom=1 & rdc=7)
         DC    XL16'0000000000000000000000000000010C'

* VAP larger #'s , i4=159(iom=1 & rdc=31)
         VRI_F VAP,+99999999999999999,+1,159,1,2
         DC    XL16'0000000000000100000000000000000C'

         VRI_F VAP,+99999999999999999,+10000000000000000,159,1,2
         DC    XL16'0000000000000109999999999999999C'

         VRI_F VAP,-9999999999999999,-1,159,1,1
         DC    XL16'0000000000000010000000000000000D'

* VAP larger #'s , i4=159(iom=1 & rdc=31)              CS=1 for all m5
* check forced positive
         VRI_F VAP,-99999999999999999,+1,159,9,2      m5=9(P2=1)
         DC    XL16'0000000000000100000000000000000C'

         VRI_F VAP,-99999999999999999,-10000000000000000,159,13,2
         DC    XL16'0000000000000109999999999999999C' m5=13(P2=1,P3=1)

         VRI_F VAP,-9999999999999999,-1,159,3,2       m5=3(P1=1)
         DC    XL16'0000000000000010000000000000000F'

*---------------------------------------------------------------------
*  VSP    - VECTOR SUBTRACT DECIMAL
*---------------------------------------------------------------------
* VSP simple + CC checks
         VRI_F VSP,+10,+12,7,1,1
         DC    XL16'0000000000000000000000000000002D'

         VRI_F VSP,-10,+12,7,1,1
         DC    XL16'0000000000000000000000000000022D'

         VRI_F VSP,+10,-12,1,1,3
         DC    XL16'0000000000000000000000000000002C'

         VRI_F VSP,+10,-12,7,1,2
         DC    XL16'0000000000000000000000000000022C'

         VRI_F VSP,-10,-12,7,1,2
         DC    XL16'0000000000000000000000000000002C'

         VRI_F VSP,-10,-10,7,1,0
         DC    XL16'0000000000000000000000000000000C'

         VRI_F VSP,+10000000000,+10,135,1,3     i4=135(iom=1 & rdc=7)
         DC    XL16'0000000000000000000000009999990C'  note RDC

* VSP larger #'s , i4=159(iom=1 & rdc=31)
         VRI_F VSP,+99999999999999999,+1,159,1,2
         DC    XL16'0000000000000099999999999999998C'

         VRI_F VSP,+99999999999999999,+10000000000000000,159,1,2
         DC    XL16'0000000000000089999999999999999C'

         VRI_F VSP,-9999999999999999,-1,159,1,1
         DC    XL16'0000000000000009999999999999998D'

         VRI_F VSP,-9999999999999999,-1,135,1,3  i4=135(iom=1 & rdc=7)
         DC    XL16'0000000000000000000000009999998D'

* VSP larger #'s , i4=159(iom=1 & rdc=31)             CS=1 for all m5
* check forced positive
         VRI_F VSP,-99999999999999999,+1,159,9,2      m5=9(P2=1)
         DC    XL16'0000000000000099999999999999998C'

         VRI_F VSP,-99999999999999999,-10000000000000000,159,13,2
         DC    XL16'0000000000000089999999999999999C' m5=13(P2=1,P3=1)

         VRI_F VSP,-9999999999999999,-1,159,3,2       m5=3(P1=1)
         DC    XL16'0000000000000009999999999999998F'

         VRI_F VSP,-9999999999999999,-1,135,3,3  i4=135(iom=1 & rdc=7)
         DC    XL16'0000000000000000000000009999998F' m5=3(P1=1)

*---------------------------------------------------------------------
*  VMP    - VECTOR MULTIPLY DECIMAL
*---------------------------------------------------------------------
* VMP simple + CC checks
         VRI_F VMP,+10,+12,7,1,2
         DC    XL16'0000000000000000000000000000120C'

         VRI_F VMP,-10,+12,7,1,1
         DC    XL16'0000000000000000000000000000120D'

         VRI_F VMP,+10,-12,1,1,3                        note rdc=1
         DC    XL16'0000000000000000000000000000000C'

         VRI_F VMP,+10,-12,7,1,1
         DC    XL16'0000000000000000000000000000120D'

         VRI_F VMP,-10,-12,7,1,2
         DC    XL16'0000000000000000000000000000120C'

         VRI_F VMP,-10,-10,7,1,2
         DC    XL16'0000000000000000000000000000100C'

         VRI_F VMP,+10000000010,+10,135,1,3     i4=135(iom=1 & rdc=7)
         DC    XL16'0000000000000000000000000000100C'  note RDC

* VMP larger #'s , i4=159(iom=1 & rdc=31)
         VRI_F VMP,+99999999999999999,+1,159,1,2
         DC    XL16'0000000000000099999999999999999C'

         VRI_F VMP,+99999999999999999,+10000000000000000,159,1,3
         DC    XL16'9999999999999990000000000000000C'    overflowed

         VRI_F VMP,-9999999999999999,-1,159,1,2
         DC    XL16'0000000000000009999999999999999C'

         VRI_F VMP,-9999999999999999,-1,135,1,3  i4=135(iom=1 & rdc=7)
         DC    XL16'0000000000000000000000009999999C'    overflow RDC

         VRI_F VMP,+9999999999999,+10000000000000,159,1,2
         DC    XL16'0000099999999999990000000000000C'

* VMP larger #'s , i4=159(iom=1 & rdc=31)             CS=1 for all m5
* check forced positive
         VRI_F VMP,-99999999999999999,+1,159,9,2      m5=9(P2=1)
         DC    XL16'0000000000000099999999999999999C'

*                                                     m5=13(P2=1,P3=1)
         VRI_F VMP,-99999999999999999,-10000000000000000,159,13,3
         DC    XL16'9999999999999990000000000000000C'    overflowed

         VRI_F VMP,-9999999999999999,-1,159,3,2       m5=3(P1=1)
         DC    XL16'0000000000000009999999999999999F'

         VRI_F VMP,-9999999999999999,-1,135,13,3  i4=135(iom=1 & rdc=7)
*                                                     m5=13(P2=1,P3=1)
         DC    XL16'0000000000000000000000009999999C'    overflow RDC

         VRI_F VMP,+9999999999999,+10000000000000,159,3,2  m5=3(P1=1)
         DC    XL16'0000099999999999990000000000000F'

*---------------------------------------------------------------------
*  VDP    - VECTOR DIVIDE DECIMAL
*---------------------------------------------------------------------
* VDP simple + CC checks
         VRI_F VDP,+10,+12,7,1,0
         DC    XL16'0000000000000000000000000000000C'

         VRI_F VDP,-100,+12,7,1,1
         DC    XL16'0000000000000000000000000000008D'

         VRI_F VDP,+100,-12,1,1,1                        note rdc=1
         DC    XL16'0000000000000000000000000000008D'

         VRI_F VDP,+100,-12,7,1,1
         DC    XL16'0000000000000000000000000000008D'

         VRI_F VDP,-100,-12,7,1,2
         DC    XL16'0000000000000000000000000000008C'

         VRI_F VDP,-100,-10,7,1,2
         DC    XL16'0000000000000000000000000000010C'

         VRI_F VDP,+10000000010,+10,135,1,3     i4=135(iom=1 & rdc=7)
         DC    XL16'0000000000000000000000000000001C'  note RDC

* VDP larger #'s , i4=159(iom=1 & rdc=31)
         VRI_F VDP,+99999999999999999,+1,159,1,2
         DC    XL16'0000000000000099999999999999999C'

         VRI_F VDP,-99999999999999999,+1000,159,1,1
         DC    XL16'0000000000000000099999999999999D'

         VRI_F VDP,-9999999999999999,-1,159,1,2
         DC    XL16'0000000000000009999999999999999C'

         VRI_F VDP,-9999999999999999,-1,135,1,3  i4=135(iom=1 & rdc=7)
         DC    XL16'0000000000000000000000009999999C'    overflow RDC

         VRI_F VDP,+9999999999999,+1234,159,1,2
         DC    XL16'0000000000000000000008103727714C'

         VRI_F VDP,+999999999999999999,+1234,159,1,2
         DC    XL16'0000000000000000810372771474878C'

* VDP larger #'s , i4=159(iom=1 & rdc=31)             CS=1 for all m5
* check forced positive
         VRI_F VDP,-99999999999999999,+1,159,9,2      m5=9(P2=1)
         DC    XL16'0000000000000099999999999999999C'

         VRI_F VDP,+99999999999999999,-1000,159,13,2 m5=13(P2=1,P3=1)
         DC    XL16'0000000000000000099999999999999C'

         VRI_F VDP,-9999999999999999,-1,159,3,2       m5=3(P1=1)
         DC    XL16'0000000000000009999999999999999F'

*                                                     m5=13(P2=1,P3=1)
         VRI_F VDP,+9999999999999999,-1,135,13,3  i4=135(iom=1 & rdc=7)
         DC    XL16'0000000000000000000000009999999C'    overflow RDC

         VRI_F VDP,+9999999999999,+1234,159,3,2        m5=3(P1=1)
         DC    XL16'0000000000000000000008103727714F'

*                                              m5=15(P1=1,P2=1,P3=1)
         VRI_F VDP,-999999999999999999,-1234,159,15,2
         DC    XL16'0000000000000000810372771474878F'

*---------------------------------------------------------------------
*  VRP    - VECTOR REMAINDER DECIMAL
*---------------------------------------------------------------------
* VRP simple + CC checks
         VRI_F VRP,+10,+12,7,1,2
         DC    XL16'0000000000000000000000000000010C'

         VRI_F VRP,-100,+12,7,1,1
         DC    XL16'0000000000000000000000000000004D'

         VRI_F VRP,+100,-12,1,1,2                        note rdc=1
         DC    XL16'0000000000000000000000000000004C'

         VRI_F VRP,+100,-12,7,1,2
         DC    XL16'0000000000000000000000000000004C'

         VRI_F VRP,-100,-12,7,1,1
         DC    XL16'0000000000000000000000000000004D'

         VRI_F VRP,-100,-10,7,1,0
         DC    XL16'0000000000000000000000000000000C'   NOTE Plus 0

         VRI_F VRP,+1000001111,+10,135,1,2    i4=135(iom=1 & rdc=7)
         DC    XL16'0000000000000000000000000000001C'  note RDC

* VRP larger #'s , i4=159(iom=1 & rdc=31)
         VRI_F VRP,+99999999999999999,+13,159,1,2
         DC    XL16'0000000000000000000000000000003C'

         VRI_F VRP,-99999999999999999,+1000,159,1,1
         DC    XL16'0000000000000000000000000000999D'

         VRI_F VRP,-9999999999999999,-47,159,1,1
         DC    XL16'0000000000000000000000000000023D'

         VRI_F VRP,-999999999999,-123456,135,1,1  i4=135(iom=1 & rdc=7)
         DC    XL16'0000000000000000000000000103743D'

         VRI_F VRP,+9999999999999,+1234,159,1,2
         DC    XL16'0000000000000000000000000000923C'

         VRI_F VRP,+999999999999999999,+1234,159,1,2
         DC    XL16'0000000000000000000000000000547C'

* VRP larger #'s , i4=159(iom=1 & rdc=31)             CS=1 for all m5
* check forced positive
         VRI_F VRP,-99999999999999999,+13,159,9,2      m5=9(P2=1)
         DC    XL16'0000000000000000000000000000003C'

         VRI_F VRP,-99999999999999999,-1000,159,13,2   m5=13(P2=1,P3=1)
         DC    XL16'0000000000000000000000000000999C'

         VRI_F VRP,-9999999999999999,-47,159,3,2       m5=3(P1=1)
         DC    XL16'0000000000000000000000000000023F'

*                                                     m5=13(P2=1,P3=1)
         VRI_F VRP,-999999999999,-123456,135,13,2 i4=135(iom=1 & rdc=7)
         DC    XL16'0000000000000000000000000103743C'

         VRI_F VRP,+9999999999999,+1234,159,3,2        m5=3(P1=1)
         DC    XL16'0000000000000000000000000000923F'

         VRI_F VRP,+999999999999999999,+1234,159,3,2   m5=3(P1=1)
         DC    XL16'0000000000000000000000000000547F'

*---------------------------------------------------------------------
*  VMSP   - VECTOR MULTIPLY AND SHIFT DECIMAL
*---------------------------------------------------------------------
* VMSP simple + CC checks
*                                         i4=128(iom=1 & shamt=0)
*                                         i4=129(iom=1 & shamt=1)
*                                         i4=132(iom=1 & shamt=4)
*                                         i4=135(iom=1 & shamt=7)
*                                         i4=142(iom=1 & shamt=14)
*                                         i4=159(iom=1 & shamt=31)

         VRI_F VMSP,+10,+12,129,1,2                      shamt=1
         DC    XL16'0000000000000000000000000000012C'

         VRI_F VMSP,-100,+12,129,1,1                     shamt=1
         DC    XL16'0000000000000000000000000000120D'

         VRI_F VMSP,+100,-12,128,1,1                     shamt=0
         DC    XL16'0000000000000000000000000001200D'

         VRI_F VMSP,+100,-12,7,1,0                       shamt=7
         DC    XL16'0000000000000000000000000000000C'    NOTE Plus 0

         VRI_F VMSP,-100,-12,0,1,2                       shamt=0
         DC    XL16'0000000000000000000000000001200C'

         VRI_F VMSP,-100,-10,0,1,2                       shamt=0
         DC    XL16'0000000000000000000000000001000C'

         VRI_F VMSP,+1000001111,+10,135,1,2              shamt=7
         DC    XL16'0000000000000000000000000001000C'

* VMSP larger #'s
         VRI_F VMSP,+99999999999999999,+1,142,1,2        shamt=14
         DC    XL16'0000000000000000000000000000999C'

*                                                        shamt=14
         VRI_F VMSP,+99999999999999999,+10000000000000000,142,1,2
         DC    XL16'0000000000009999999999999999900C'

         VRI_F VMSP,-9999999999999999,-1,159,1,0         shamt=31
         DC    XL16'0000000000000000000000000000000C'

         VRI_F VMSP,-9999999999999999,-1,135,1,2         shamt=7
         DC    XL16'0000000000000000000000999999999C'

         VRI_F VMSP,-9999999999999,+10000000000000,159,1,0 shamt=31
         DC    XL16'0000000000000000000000000000000C'  NOTE Plus 0

* VMSP larger #'s , i4=159(iom=1 & rdc=31)             CS=1 for all m5
* check forced positive
*                                                      shamt=1
         VRI_F VMSP,-99999999999999999,+1,129,9,2      m5=9(P2=1)
         DC    XL16'0000000000000009999999999999999C'

*                                                     shamt=7
*                                                     m5=13(P2=1,P3=1)
         VRI_F VMSP,-99999999999999999,-10000000000000000,135,13,2
         DC    XL16'0000099999999999999999000000000C'

*                                                      shamt=7
         VRI_F VMSP,-9999999999999999,-1,135,3,2       m5=3(P1=1)
         DC    XL16'0000000000000000000000999999999F'

*                                                      shamt=4
*                                                      m5=13(P2=1,P3=1)
         VRI_F VMSP,-9999999999999999,-1,132,13,2
         DC    XL16'0000000000000000000999999999999C'

*                                                      shamt=31
*                                                      m5=3(P1=1)
         VRI_F VMSP,+99999999999999999,+10000000000000000,159,3,2
         DC    XL16'0000000000000000000000000000099F'

*---------------------------------------------------------------------
* VSDP   - VECTOR SHIFT AND DIVIDE DECIMAL
*---------------------------------------------------------------------
* VSDP simple + CC checks
*                                         i4=128(iom=1 & shamt=0)
*                                         i4=129(iom=1 & shamt=1)
*                                         i4=132(iom=1 & shamt=4)
*                                         i4=135(iom=1 & shamt=7)
*                                         i4=142(iom=1 & shamt=14)
*                                         i4=159(iom=1 & shamt=31)

         VRI_F VSDP,+10,+12,128,1,0                      shamt=0
         DC    XL16'0000000000000000000000000000000C'

         VRI_F VSDP,+10,+12,129,1,2                      shamt=1
         DC    XL16'0000000000000000000000000000008C'

         VRI_F VSDP,-100,+12,129,1,1                     shamt=1
         DC    XL16'0000000000000000000000000000083D'

         VRI_F VSDP,+100,-12,132,1,1                     shamt=4
         DC    XL16'0000000000000000000000000083333D'

         VRI_F VSDP,+100,-12,128,1,1                     shamt=0
         DC    XL16'0000000000000000000000000000008D'

         VRI_F VSDP,-100,-12,129,1,2                     shamt=1
         DC    XL16'0000000000000000000000000000083C'

         VRI_F VSDP,-100,-10,135,1,2                     shamt=7
         DC    XL16'0000000000000000000000100000000C'

         VRI_F VSDP,+10000000010,+10,135,1,2             shamt=7
         DC    XL16'0000000000000010000000010000000C'

* VSDP larger #'s
         VRI_F VSDP,+99999999999999999,+1,132,1,2        shamt=4
         DC    XL16'0000000000999999999999999990000C'

         VRI_F VSDP,-99999999999999999,+1000,128,1,1     shamt=0
         DC    XL16'0000000000000000099999999999999D'

         VRI_F VSDP,-9999999999999999,-1,128,1,2         shamt=0
         DC    XL16'0000000000000009999999999999999C'

         VRI_F VSDP,-9999999999999999,-1,142,1,2         shamt=14
         DC    XL16'0999999999999999900000000000000C'

         VRI_F VSDP,+9999999999999,+1234,129,1,2         shamt=1
         DC    XL16'0000000000000000000081037277147C'

         VRI_F VSDP,+999999999999999,+12345,129,1,2      shamt=1
         DC    XL16'0000000000000000000810044552450C'

* VSDP larger #'s                                     CS=1 for all m5
* check forced positive
*                                                      shamt=1
         VRI_F VSDP,-99999999999999999,+1,129,9,2      m5=9(P2=1)
         DC    XL16'0000000000000999999999999999990C'

*                                                      shamt=3
         VRI_F VSDP,+99999999999999999,-1000,131,13,2  m5=13(P2=1,P3=1)
         DC    XL16'00000000000000099999999999999999C'

*                                                      shamt=3
         VRI_F VSDP,-9999999999999999,-1,131,3,2       m5=3(P1=1)
         DC    XL16'00000000000009999999999999999000F'

*                                                      shamt=7
*                                                      m5=13(P2=1,P3=1)
         VRI_F VSDP,+9999999999999999,-1,135,13,2
         DC    XL16'0000000099999999999999990000000C'

*                                                      shamt=14
         VRI_F VSDP,+9999999999999,+3,142,3,2       m5=3(P1=1)
         DC    XL16'0000333333333333300000000000000F'

*                                                      shamt=31
*                                              m5=15(P1=1,P2=1,P3=1)
         VRI_F VSDP,-999999999999999999,-3,159,15,3
         DC    XL16'0000000000000000000000000000000F'


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
