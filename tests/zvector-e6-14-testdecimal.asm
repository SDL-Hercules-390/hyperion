 TITLE 'zvector-e6-14-testdecimal'
***********************************************************************
*
*        Zvector E6 instruction tests for VRR-g encoded:
*
*        E65F VTP     - VECTOR TEST DECIMAL
*
*        James Wekel June 2024
*                   April 2025 - packed-decimal-enhancements facility 3
***********************************************************************

***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E6 VRR-g vector
*  test decimal. Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*   *Testcase zvector-e6-14-testdecimal
*   *
*   *   Zvector E6 tests for VRR-g encoded instruction:
*   *
*   *   E65F VTP     - VECTOR TEST DECIMAL
*   *
*   *   # -------------------------------------------------------
*   *   #  This tests only the basic function of the instruction.
*   *   #  Exceptions are NOT tested.
*   *   #
*   *   # Note: errors may indicate VTPX as the instruction in
*   *   #       error. This is the VTPX macro used to generate
*   *   #       VTP instructions with a non-zero I3
*   *   # -------------------------------------------------------
*   *
*   mainsize    2
*   numcpu      1
*   sysclear
*   archlvl     z/Arch
*
*   loadcore    "$(testpath)/zvector-e6-14-testdecimal.core" 0x0
*
*   diag8cmd    enable    # (needed for messages to Hercules console)
*   runtest     2
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
*                                      skip message
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
         ORG   ZVE6TST+X'1A0'        z/Architecture RESTART PSW
         DC    X'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE 2
         ORG   ZVE6TST+X'1D0'        z/Architecture PROGRAM CHECK PSW
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

BEGIN    BALR  R8,0             Initialize FIRST base register
         BCTR  R8,0             Initialize FIRST base register
         BCTR  R8,0             Initialize FIRST base register

         LA    R9,2048(,R8)     Initialize SECOND base register
         LA    R9,2048(,R9)     Initialize SECOND base register

         LA    R10,2048(,R9)    Initialize THIRD base register
         LA    R10,2048(,R10)   Initialize THIRD base register

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
* Is Vector packed-decimal enhancement 3 facility installed  (bit 199)
***********************************************************************

         FCHECK 199,'vector packed-decimal-enhancement 3'
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

         L     R11,TSUB          get address of test routine
         BALR  R11,R11           do test

         LB    R1,CCMASK         (failure CC mask)
         SLL   R1,4              (shift to BC instr CC position)
         EX    R1,TESTCC            fail if...

         LA    R12,4(0,R12)      next test address
         B     NEXTE6

TESTCC   BC    0,CCMSG          (fail if unexpected condition code)

                                                                 EJECT
***********************************************************************
* cc was not as expected
***********************************************************************
CCMSG    EQU   *
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
*        E6TEST DSECT
***********************************************************************
                                                                SPACE 2
E6TEST   DSECT ,
TSUB     DC    A(0)           pointer  to test
TNUM     DC    H'00'          Test Number
         DC    XL1'00'
CC       DC    HL1'00'        cc
CCMASK   DC    HL1'00'        not expected CC mask

OPNAME   DC    CL8' '         E6 name

RELEN    DC    A(0)           RESULT LENGTH
READDR   DC    A(0)           expected result address

**
*        test routine will be here (from VRR_G macro)
* followed by
*        16-byte EXPECTED RESULT
*        8-byte  byte source
                                                                EJECT
**********************************************************************
* (pending VTP update in SATK ASAM for
*        vector packed-decimal enhancement 3 facility)
*
*     VTPX Macro to help build VTP instruction
*        VTP   V1,i3
*
*        Note: v1 is a fixed vector register
*              i3 can be specified in hex eg. x'1F1F'
***********************************************************************
         MACRO
         VTPX  &I3
.*                                     &I3  - i3 for VTP instruction
         LCLA  &RSI3
&RSI3    SETA  +X'000000'+(16*&I3)
                                                               SPACE 1
         DS    0H                      E65F VTP - Vector Test Decimal
         DC    X'E6'
         DC    X'01'                   v1
         DC    HL3'&RSI3'              0, I3, RXB
         DC    X'5F'
                                                               SPACE 1
         MEND

***********************************************************************
*     Macros to help build test tables
*----------------------------------------------------------------------
*     VRR_G Macro to help build test tables
***********************************************************************
         MACRO
         VRR_G &INST,&I3,&CC
.*                               &INST  - instruction under test
.*                               &I3    - i3 for VTP instruction
.*                               &CC    - expected CC
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
         DC    XL1'00'
         DC    HL1'&CC'          cc
         DC    HL1'&XCC(&CC+1)'  cc failed mask

         DC    CL8'&INST'        instruction name

         DC    A(16)             result length
REA&TNUM DC    A(RE&TNUM)        result address
.*
*                                INSTRUCTION UNDER TEST ROUTINE
X&TNUM   DS    0F
         VL    V1,RE&TNUM        get V1 source

.* Select based on &INST to build appropriate instruction sequence
         AIF   ('&INST' EQ 'VTP').DOVTP
         AIF   ('&INST' EQ 'VTPX').DOVTPX
         MNOTE 1,'Instruction &INST not recognized; skipping test'
         MEXIT

.DOVTP   ANOP
         &INST V1                      test instruction
         AGO   .CONT

.DOVTPX  ANOP
.*       VTP   V1,&i3
         &INST &I3                     test instruction

.CONT    ANOP
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
*        E6 VRR_G tests
***********************************************************************
ZVE6TST  CSECT ,
         DS    0F
                                                                SPACE 2
         PRINT DATA
*
*        E65F VTP     - VECTOR TEST DECIMAL
*             VTPX    - VECTOR TEST DECIMAL
*                          packed-decimal-enhancements facility 3
*        VRR_G instr, i3, cc
*              followed by
*              v1     - 16 byte source
*---------------------------------------------------------------------
* VTP     - VECTOR TEST DECIMAL
*---------------------------------------------------------------------
* VTP simple

* digits valid,  sign valid
         VRR_G VTP,0,0
         DC    XL16'0000000000000000000000000000000C'   V1 source

         VRR_G VTP,0,0
         DC    XL16'0000000000000000001234500000000D'   V1 source

* digits valid,  sign invalid
         VRR_G VTP,0,1
         DC    XL16'00000000000000000000000000000009'   V1 source

         VRR_G VTP,0,1
         DC    XL16'00000000000000000012345000000000'   V1 source

* a digit invalid, sign valid
         VRR_G VTP,0,2
         DC    XL16'000000000FF00000000000000000000C'   V1 source

         VRR_G VTP,0,2
         DC    XL16'F0F0000000000000001234500000000F'   V1 source

* a digit invalid,  sign invalid
         VRR_G VTP,0,3
         DC    XL16'000000000FF000000000000000000009'   V1 source

         VRR_G VTP,0,3
         DC    XL16'F0F00000000000000012345000000002'   V1 source

*---------------------------------------------------------------------
* VTPX    - VECTOR TEST DECIMAL packed-decimal-enhancements facility 3
*---------------------------------------------------------------------
* Enhanced Testing: ET=1
*
* VTP          Byte-Padding Test: BPT
*              Sign-Test Control: STC
*              Digits Count:      DC
*---------------------------------------------------------------------
*  Sign-Test Control: STC=0
*---------------------------------------------------------------------
* BTP=0, STC=0, DC=0                   only sign: A-F valid, no digits
         VRR_G VTPX,X'8000',0
         DC    XL16'0FF0000000000000000000000000000A'   V1 source

         VRR_G VTPX,X'8000',0
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'8000',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'8000',0
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'8000',0
         DC    XL16'0FF0000000000000000000000000000E'   V1 source

         VRR_G VTPX,X'8000',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

         VRR_G VTPX,X'8000',1
         DC    XL16'0FF00000000000000000000000000000'   V1 source

         VRR_G VTPX,X'8000',1
         DC    XL16'0FF00000000000000000000000000009'   V1 source

* BTP=0, STC=0, DC>0 and odd           sign: A-F valid, n digits
         VRR_G VTPX,X'8005',0
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'8007',0
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'8009',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'800B',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'800D',0
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'800F',0
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'800F',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'800F',2
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'800F',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

* BTP=0, STC=0, DC>0 and even           sign: A-F valid,  n digits
         VRR_G VTPX,X'8004',0
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'8006',0
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'8008',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'800A',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'800C',0
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'800E',0
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'800E',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'800E',2
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'800E',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

* BTP=1, STC=0, DC>0 and even           sign: A-F valid, N+1 digits
         VRR_G VTPX,X'C004',0
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'C006',0
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'C008',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'C00A',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'C00C',0
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'C004',0
         DC    XL16'0FF0000000000000000000000001100F'   V1 source

         VRR_G VTPX,X'C004',2
         DC    XL16'0FF0000000000000000000000011100F'   V1 source

         VRR_G VTPX,X'C004',1
         DC    XL16'0FF00000000000000000000000011000'   V1 source

         VRR_G VTPX,X'C004',2
         DC    XL16'0FF0000000000000000000000001F00F'   V1 source

         VRR_G VTPX,X'C004',3
         DC    XL16'0FF00000000000000000000000111009'   V1 source

*---------------------------------------------------------------------
*  Sign-Test Control: STC=1
*---------------------------------------------------------------------
* BTP=0, STC=1, DC=0                   only sign: A-F valid, no digits
         VRR_G VTPX,X'8020',0
         DC    XL16'0FF0000000000000000000000000000A'   V1 source

         VRR_G VTPX,X'8020',0
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'8020',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'8020',0
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'8020',0
         DC    XL16'0FF0000000000000000000000000000E'   V1 source

         VRR_G VTPX,X'8020',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

         VRR_G VTPX,X'8020',1
         DC    XL16'0FF00000000000000000000000000000'   V1 source

         VRR_G VTPX,X'8020',1
         DC    XL16'0FF00000000000000000000000000009'   V1 source

* BTP=0, STC=1, DC>0 and odd
*                                      sign: A,C E,F valid, 0 digits
         VRR_G VTPX,X'8027',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'802B',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'803F',2
         DC    XL16'F000000000000000000000000000000C'   V1 source

*                                      sign: A-F valid, non-0 digits
         VRR_G VTPX,X'8025',0
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'8027',0
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'8029',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'802B',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'802D',0
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'802F',0
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'802F',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'802F',2
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'802F',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

         VRR_G VTPX,X'803F',0
         DC    XL16'1100000000000000000000000000000B'   V1 source

* BTP=0, STC=1, DC>0 and even
*                                      sign: A,C E,F valid, 0 digits
         VRR_G VTPX,X'8026',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'802A',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'803E',0
         DC    XL16'F100000000000000000000000000000E'   V1 source

*                                      sign: A-F valid, non-0 digits
         VRR_G VTPX,X'8024',0
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'8026',0
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'8028',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'802A',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'802C',0
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'802E',0
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'802E',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'802E',2
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'802E',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

         VRR_G VTPX,X'803E',1
         DC    XL16'1000000000000000000000000000000B'   V1 source

* BTP=1, STC=1, DC>0 and even
*                                      sign: A,C E,F valid, 0 digits
         VRR_G VTPX,X'C026',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'C02A',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'C03E',2
         DC    XL16'F000000000000000000000000000000C'   V1 source

*                                      sign: A-F valid, non-0 digits
         VRR_G VTPX,X'C024',0
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'C026',0
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'C028',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'C02A',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'C02C',0
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'C024',0
         DC    XL16'0FF0000000000000000000000001100F'   V1 source

         VRR_G VTPX,X'C024',2
         DC    XL16'0FF0000000000000000000000011100F'   V1 source

         VRR_G VTPX,X'C024',1
         DC    XL16'0FF00000000000000000000000011000'   V1 source

         VRR_G VTPX,X'C024',2
         DC    XL16'0FF0000000000000000000000001F00F'   V1 source

         VRR_G VTPX,X'C024',3
         DC    XL16'0FF00000000000000000000000111009'   V1 source

         VRR_G VTPX,X'C03E',2
         DC    XL16'1000000000000000000000000000000C'   V1 source

*---------------------------------------------------------------------
*  Sign-Test Control: STC=2
*---------------------------------------------------------------------
* BTP=0, STC=2, DC=0                   only sign: C,D valid, no digits
         VRR_G VTPX,X'8040',1
         DC    XL16'0FF0000000000000000000000000000A'   V1 source

         VRR_G VTPX,X'8040',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'8040',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'8040',0
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'8040',1
         DC    XL16'0FF0000000000000000000000000000E'   V1 source

         VRR_G VTPX,X'8040',1
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

         VRR_G VTPX,X'8040',1
         DC    XL16'0FF00000000000000000000000000000'   V1 source

         VRR_G VTPX,X'8040',1
         DC    XL16'0FF00000000000000000000000000009'   V1 source

* BTP=0, STC=2, DC>0 and odd
*                                      sign: C, D  valid, 0 digits
         VRR_G VTPX,X'8047',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'804B',0
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

*                                      sign: C, D valid, non-0 digits
         VRR_G VTPX,X'8045',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'8047',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'8049',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'804B',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'804D',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'804F',1
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'804F',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'804F',3
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'804F',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

* BTP=0, STC=2, DC>0 and even
*                                      sign: C, D valid, 0 digits
         VRR_G VTPX,X'8046',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'804A',0
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

*                                      sign: C, D valid, non-0 digits
         VRR_G VTPX,X'8044',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'8046',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'8048',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'804A',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'804C',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'804E',1
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'804E',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'804E',3
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'804E',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

* BTP=1, STC=2, DC>0 and even
*                                      sign: C, D  valid, 0 digits
         VRR_G VTPX,X'C046',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'C04A',0
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

*                                      sign: A-F valid, non-0 digits
         VRR_G VTPX,X'C044',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'C046',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'C048',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'C04A',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'C04C',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'C044',1
         DC    XL16'0FF0000000000000000000000001100F'   V1 source

         VRR_G VTPX,X'C044',3
         DC    XL16'0FF0000000000000000000000011100F'   V1 source

         VRR_G VTPX,X'C044',1
         DC    XL16'0FF00000000000000000000000011000'   V1 source

         VRR_G VTPX,X'C044',3
         DC    XL16'0FF0000000000000000000000001F00F'   V1 source

         VRR_G VTPX,X'C044',3
         DC    XL16'0FF00000000000000000000000111009'   V1 source

*---------------------------------------------------------------------
*  Sign-Test Control: STC=3
*---------------------------------------------------------------------
* BTP=0, STC=3, DC=0                   only sign: C, D valid, no digits
         VRR_G VTPX,X'8060',1
         DC    XL16'0FF0000000000000000000000000000A'   V1 source

         VRR_G VTPX,X'8060',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'8060',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'8060',0
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'8060',1
         DC    XL16'0FF0000000000000000000000000000E'   V1 source

         VRR_G VTPX,X'8060',1
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

         VRR_G VTPX,X'8060',1
         DC    XL16'0FF00000000000000000000000000000'   V1 source

         VRR_G VTPX,X'8060',1
         DC    XL16'0FF00000000000000000000000000009'   V1 source

* BTP=0, STC=3, DC>0 and odd
*                                      sign: C  valid, 0 digits
         VRR_G VTPX,X'8067',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'806B',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'807F',2
         DC    XL16'F000000000000000000000000000000C'   V1 source

*                                      sign: C, D valid, non-0 digits
         VRR_G VTPX,X'8065',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'8067',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'8069',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'806B',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'806D',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'806F',1
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'806F',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'806F',3
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'806F',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

         VRR_G VTPX,X'807F',1
         DC    XL16'1100000000000000000000000000000B'   V1 source

* BTP=0, STC=3, DC>0 and even
*                                      sign: C valid, 0 digits
         VRR_G VTPX,X'8066',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'806A',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'806A',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

*                                      sign: C, D valid, non-0 digits
         VRR_G VTPX,X'8064',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'8066',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'8068',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'806A',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'806C',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'806E',1
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'806E',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'806E',3
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'806E',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

         VRR_G VTPX,X'807F',1
         DC    XL16'1000000000000000000000000000000B'   V1 source

* BTP=1, STC=3, DC>0 and even
*                                      sign: C    valid, 0 digits
         VRR_G VTPX,X'C066',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'C06A',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'C06A',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

*                                      sign: C, D valid, non-0 digits
         VRR_G VTPX,X'C064',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'C066',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'C068',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'C06A',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'C06C',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'C064',1
         DC    XL16'0FF0000000000000000000000001100F'   V1 source

         VRR_G VTPX,X'C064',3
         DC    XL16'0FF0000000000000000000000011100F'   V1 source

         VRR_G VTPX,X'C064',1
         DC    XL16'0FF00000000000000000000000011000'   V1 source

         VRR_G VTPX,X'C064',3
         DC    XL16'0FF0000000000000000000000001F00F'   V1 source

         VRR_G VTPX,X'C064',3
         DC    XL16'0FF00000000000000000000000111009'   V1 source

         VRR_G VTPX,X'C07E',2
         DC    XL16'1000000000000000000000000000000C'   V1 source
*---------------------------------------------------------------------
*  Sign-Test Control: STC=4
*---------------------------------------------------------------------
* BTP=0, STC=4, DC=0                   only sign: F valid, no digits
         VRR_G VTPX,X'8080',1
         DC    XL16'0FF0000000000000000000000000000A'   V1 source

         VRR_G VTPX,X'8080',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'8080',1
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'8080',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'8080',1
         DC    XL16'0FF0000000000000000000000000000E'   V1 source

         VRR_G VTPX,X'8080',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

         VRR_G VTPX,X'8080',1
         DC    XL16'0FF00000000000000000000000000000'   V1 source

         VRR_G VTPX,X'8080',1
         DC    XL16'0FF00000000000000000000000000009'   V1 source

* BTP=0, STC=4, DC>0 and odd
*                                      sign: F  valid, 0 digits
         VRR_G VTPX,X'8087',1
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'808B',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'808B',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

*                                      sign: F valid, non-0 digits
         VRR_G VTPX,X'8085',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'8087',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'8089',1
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'808B',1
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'808D',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'808F',0
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'808F',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'808F',2
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'808F',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

* BTP=0, STC=4, DC>0 and even
*                                      sign: F valid, 0 digits
         VRR_G VTPX,X'8086',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'808A',1
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'808A',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'808A',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

*                                      sign: F valid, non-0 digits
         VRR_G VTPX,X'8084',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'8086',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'8088',1
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'808A',1
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'808C',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'808E',0
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'808E',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'808E',2
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'808E',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

* BTP=1, STC=4, DC>0 and even
*                                      sign: F    valid, 0 digits
         VRR_G VTPX,X'C086',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'C08A',1
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'C08A',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'C08A',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

*                                      sign: F  valid, non-0 digits
         VRR_G VTPX,X'C084',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'C086',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'C088',1
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'C08A',1
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'C08C',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'C084',0
         DC    XL16'0FF0000000000000000000000001100F'   V1 source

         VRR_G VTPX,X'C084',2
         DC    XL16'0FF0000000000000000000000011100F'   V1 source

         VRR_G VTPX,X'C084',1
         DC    XL16'0FF00000000000000000000000011000'   V1 source

         VRR_G VTPX,X'C084',2
         DC    XL16'0FF0000000000000000000000001F00F'   V1 source

         VRR_G VTPX,X'C084',3
         DC    XL16'0FF00000000000000000000000111009'   V1 source

*---------------------------------------------------------------------
*  Sign-Test Control: STC=5
*---------------------------------------------------------------------
* BTP=0, STC=5, DC=0                   only sign: F valid, no digits
         VRR_G VTPX,X'80A0',1
         DC    XL16'0FF0000000000000000000000000000A'   V1 source

         VRR_G VTPX,X'80A0',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'80A0',1
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'80A0',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'80A0',1
         DC    XL16'0FF0000000000000000000000000000E'   V1 source

         VRR_G VTPX,X'80A0',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

         VRR_G VTPX,X'80A0',1
         DC    XL16'0FF00000000000000000000000000000'   V1 source

         VRR_G VTPX,X'80A0',1
         DC    XL16'0FF00000000000000000000000000009'   V1 source

* BTP=0, STC=5, DC>0 and odd
*                                      sign: F  valid, 0 digits
         VRR_G VTPX,X'80A7',1
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'80AB',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'80AB',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

*                                      sign: F valid, non-0 digits
         VRR_G VTPX,X'80A5',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'80A7',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'80A9',1
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'80AB',1
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'80AD',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'80AF',0
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'80AF',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'80AF',2
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'80AF',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

* BTP=0, STC=5, DC>0 and even
*                                      sign: F valid, 0 digits
         VRR_G VTPX,X'80A6',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'80AA',1
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'80AA',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'80AA',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

*                                      sign: F valid, non-0 digits
         VRR_G VTPX,X'80A4',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'80A6',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'80A8',1
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'80AA',1
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'80AC',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'80AE',0
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'80AE',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'80AE',2
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'80AE',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

* BTP=1, STC=5, DC>0 and even
*                                      sign: F    valid, 0 digits
         VRR_G VTPX,X'C0A6',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'C0AA',1
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'C0AA',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'C0AA',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

*                                      sign: F  valid, non-0 digits
         VRR_G VTPX,X'C0A4',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'C0A6',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'C0A8',1
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'C0AA',1
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'C0AC',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'C0A4',0
         DC    XL16'0FF0000000000000000000000001100F'   V1 source

         VRR_G VTPX,X'C0A4',2
         DC    XL16'0FF0000000000000000000000011100F'   V1 source

         VRR_G VTPX,X'C0A4',1
         DC    XL16'0FF00000000000000000000000011000'   V1 source

         VRR_G VTPX,X'C0A4',2
         DC    XL16'0FF0000000000000000000000001F00F'   V1 source

         VRR_G VTPX,X'C0A4',3
         DC    XL16'0FF00000000000000000000000111009'   V1 source

*---------------------------------------------------------------------
*  Sign-Test Control: STC=6
*---------------------------------------------------------------------
* BTP=0, STC=6, DC=0                   sign: C, D, F valid, no digits
         VRR_G VTPX,X'80C0',1
         DC    XL16'0FF0000000000000000000000000000A'   V1 source

         VRR_G VTPX,X'80C0',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'80C0',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'80C0',0
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'80C0',1
         DC    XL16'0FF0000000000000000000000000000E'   V1 source

         VRR_G VTPX,X'80C0',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

         VRR_G VTPX,X'80C0',1
         DC    XL16'0FF00000000000000000000000000000'   V1 source

         VRR_G VTPX,X'80C0',1
         DC    XL16'0FF00000000000000000000000000009'   V1 source

* BTP=0, STC=6, DC>0 and odd
*                                      sign: C, D, F valid, 0 digits
         VRR_G VTPX,X'80C7',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'80CB',0
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'80CB',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

*                                     sign: C, D, F valid, non-0 digits
         VRR_G VTPX,X'80C5',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'80C7',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'80C9',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'80CB',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'80CD',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'80CF',0
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'80CF',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'80CF',2
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'80CF',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

* BTP=0, STC=6, DC>0 and even
*                                     sign: C, D, F valid, 0 digits
         VRR_G VTPX,X'80C6',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'80CA',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'80CA',0
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'80CA',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

*                                     sign: C, D, F valid, non-0 digits
         VRR_G VTPX,X'80C4',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'80C6',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'80C8',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'80CA',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'80CC',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'80CE',0
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'80CE',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'80CE',2
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'80CE',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

* BTP=1, STC=6, DC>0 and even
*                                     sign: C, D, F valid, 0 digits
         VRR_G VTPX,X'C0C6',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'C0CA',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'C0CA',0
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'C0CA',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

*                                     sign: C, D, F valid, non-0 digits
         VRR_G VTPX,X'C0C4',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'C0C6',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'C0C8',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'C0CA',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'C0CC',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'C0C4',0
         DC    XL16'0FF0000000000000000000000001100F'   V1 source

         VRR_G VTPX,X'C0C4',2
         DC    XL16'0FF0000000000000000000000011100F'   V1 source

         VRR_G VTPX,X'C0C4',1
         DC    XL16'0FF00000000000000000000000011000'   V1 source

         VRR_G VTPX,X'C0C4',2
         DC    XL16'0FF0000000000000000000000001F00F'   V1 source

         VRR_G VTPX,X'C0C4',3
         DC    XL16'0FF00000000000000000000000111009'   V1 source


*---------------------------------------------------------------------
*  Sign-Test Control: STC=7
*---------------------------------------------------------------------
* BTP=0, STC=7, DC=0                   sign: C, D, F valid, no digits
         VRR_G VTPX,X'80E0',1
         DC    XL16'0FF0000000000000000000000000000A'   V1 source

         VRR_G VTPX,X'80E0',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'80E0',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'80E0',0
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'80E0',1
         DC    XL16'0FF0000000000000000000000000000E'   V1 source

         VRR_G VTPX,X'80E0',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

         VRR_G VTPX,X'80E0',1
         DC    XL16'0FF00000000000000000000000000000'   V1 source

         VRR_G VTPX,X'80E0',1
         DC    XL16'0FF00000000000000000000000000009'   V1 source

* BTP=0, STC=7, DC>0 and odd
*                                      sign: C, F valid, 0 digits
         VRR_G VTPX,X'80E7',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'80EB',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'80EB',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

         VRR_G VTPX,X'80FF',2
         DC    XL16'F000000000000000000000000000000C'   V1 source

*                                     sign: C, D, F valid, non-0 digits
         VRR_G VTPX,X'80E5',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'80E7',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'80E9',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'80EB',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'80ED',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'80EF',0
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'80EF',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'80EF',2
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'80EF',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

         VRR_G VTPX,X'80FF',1
         DC    XL16'1100000000000000000000000000000B'   V1 source

* BTP=0, STC=7, DC>0 and even
*                                     sign: C, F valid, 0 digits
         VRR_G VTPX,X'80E6',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'80EA',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'80EA',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'80EA',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

         VRR_G VTPX,X'80FE',1
         DC    XL16'0000000000000000000000000000000E'   V1 source

*                                     sign: C, D, F valid, non-0 digits
         VRR_G VTPX,X'80E4',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'80E6',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'80E8',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'80EA',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'80EC',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'80EE',0
         DC    XL16'0FF0000000000000000000000000100F'   V1 source

         VRR_G VTPX,X'80EE',1
         DC    XL16'0FF00000000000000000000000001000'   V1 source

         VRR_G VTPX,X'80EE',2
         DC    XL16'0FF0000000000000000000000000F00F'   V1 source

         VRR_G VTPX,X'80EE',3
         DC    XL16'0FF0000000000000000000000000F009'   V1 source

         VRR_G VTPX,X'80FE',1
         DC    XL16'1000000000000000000000000000000B'   V1 source

* BTP=1, STC=7, DC>0 and even
*                                     sign: C, F valid, 0 digits
         VRR_G VTPX,X'C0E6',1
         DC    XL16'0FF0000000000000000000000000000B'   V1 source

         VRR_G VTPX,X'C0EA',0
         DC    XL16'0FF0000000000000000000000000000C'   V1 source

         VRR_G VTPX,X'C0EA',1
         DC    XL16'0FF0000000000000000000000000000D'   V1 source

         VRR_G VTPX,X'C0EA',0
         DC    XL16'0FF0000000000000000000000000000F'   V1 source

*                                     sign: C, D, F valid, non-0 digits
         VRR_G VTPX,X'C0E4',1
         DC    XL16'0FF0000000000000000000000000100A'   V1 source

         VRR_G VTPX,X'C0E6',1
         DC    XL16'0FF0000000000000000000000000100B'   V1 source

         VRR_G VTPX,X'C0E8',0
         DC    XL16'0FF0000000000000000000000000100C'   V1 source

         VRR_G VTPX,X'C0EA',0
         DC    XL16'0FF0000000000000000000000000100D'   V1 source

         VRR_G VTPX,X'C0EC',1
         DC    XL16'0FF0000000000000000000000000100E'   V1 source

         VRR_G VTPX,X'C0E4',0
         DC    XL16'0FF0000000000000000000000001100F'   V1 source

         VRR_G VTPX,X'C0E4',2
         DC    XL16'0FF0000000000000000000000011100F'   V1 source

         VRR_G VTPX,X'C0E4',1
         DC    XL16'0FF00000000000000000000000011000'   V1 source

         VRR_G VTPX,X'C0E4',2
         DC    XL16'0FF0000000000000000000000001F00F'   V1 source

         VRR_G VTPX,X'C0E4',3
         DC    XL16'0FF00000000000000000000000111009'   V1 source

         VRR_G VTPX,X'C0FE',2
         DC    XL16'1000000000000000000000000000000C'   V1 source





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
