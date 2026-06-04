 TITLE 'zvector-e7-30-DivideRem'
***********************************************************************
*
*    Zvector E7 instruction tests for VRR-c encoded:
*
*        E7B0 VDL    - VECTOR DIVIDE LOGICAL
*        E7B1 VRL    - VECTOR REMAINDER LOGICAL
*        E7B2 VD     - VECTOR DIVIDE
*        E7B3 VR     - VECTOR REMAINDER
*
*    and partial testing of
*
*        E7AA VMAL    - VECTOR MULTIPLY AND ADD LOW
*
*    during cross check tests.
*
*        James Wekel March 2026
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E7 VRR-c
*  Vector Divide Logical, Vector Remainder Logical, Vector Divide,
*  and Vector Remainder instructions.
*
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*    *Testcase zvector-e7-30-DivideRem
*    *
*    *   Zvector E7 instruction tests for VRR-c encoded:
*    *
*    *   E7B0 VDL    - VECTOR DIVIDE LOGICAL
*    *   E7B1 VRL    - VECTOR REMAINDER LOGICAL
*    *   E7B2 VD     - VECTOR DIVIDE
*    *   E7B3 VR     - VECTOR REMAINDER
*    *
*    *
*    *   # ------------------------------------------------------------
*    *   #  This tests only the basic function of the instructions.
*    *   #  Exceptions are NOT tested.
*    *   # ------------------------------------------------------------
*    *
*    mainsize    2
*    numcpu      1
*    sysclear
*    archlvl     z/Arch
*
*    loadcore    "$(testpath)/zvector-e7-30-DivideRem.core" 0x0
*
*    diag8cmd    enable    # (needed for messages to Hercules console)
*    runtest     5
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
*     Instruction Macros
***********************************************************************
***********************************************************************
* (pending E7B0 VDL    - VECTOR DIVIDE LOGICAL
*  inclusion in SATK ASAM)
*
*     VDL Macro to help build VDL instruction
*        VDL   m4, m5
*
*        Note: v1, v2, v3 are fixed vector registers:
*                         v1 = 1; v2 = 2, v3 = 3
*              m4 are specified in hex eg. x'0'
*              m5 are specified in hex eg. x'0'
***********************************************************************
         MACRO
         VDL   &M4,&M5
.*                                     &M4  - m4 for VDL instruction
.*                                     &M5  - m5 for VDL instruction
         LCLA  &M4M5
&M4M5    SETA  +((+&M5*256)+(+&M4*16))
                                                               SPACE 1
         DS    0H                      E7B0 VDL
         DC    X'E7'                   - VECTOR DIVIDE LOGICAL
         DC    X'12'                    v1, v2
         DC    X'30'                    v3, reserved
         DC    HL2'&M4M5'               reserved, m5, m4, RXB
         DC    X'B0'
                                                               SPACE 1
         MEND
                                                               SPACE 2
***********************************************************************
* (pending E7B1 VRL    - VECTOR REMAINDER LOGICAL
*  inclusion in SATK ASAM)
*
*     VDL Macro to help build VDL instruction
*        VDL   m4, m5
*
*        Note: v1, v2, v3 are fixed vector registers:
*                         v1 = 1; v2 = 2, v3 = 3
*              m4 are specified in hex eg. x'0'
*              m5 are specified in hex eg. x'0'
***********************************************************************
         MACRO
         VRL   &M4,&M5
.*                                     &M4  - m4 for VRL instruction
.*                                     &M5  - m5 for VRL instruction
         LCLA  &M4M5
&M4M5    SETA  +((+&M5*256)+(+&M4*16))
                                                               SPACE 1
         DS    0H                      E7B1 VRL
         DC    X'E7'                   - VECTOR REMAINDER LOGICAL
         DC    X'12'                    v1, v2
         DC    X'30'                    v3, reserved
         DC    HL2'&M4M5'               reserved, m5, m4, RXB
         DC    X'B1'
                                                               SPACE 1
         MEND
                                                               EJECT
***********************************************************************
* (pending E7B2 VD     - VECTOR DIVIDE
*  inclusion in SATK ASAM)
*
*     VDL Macro to help build VD instruction
*        VDL   m4, m5
*
*        Note: v1, v2, v3 are fixed vector registers:
*                         v1 = 1; v2 = 2, v3 = 3
*              m4 are specified in hex eg. x'0'
*              m5 are specified in hex eg. x'0'
***********************************************************************
         MACRO
         VD    &M4,&M5
.*                                     &M4  - m4 for VD instruction
.*                                     &M5  - m5 for VD instruction
         LCLA  &M4M5
&M4M5    SETA  +((+&M5*256)+(+&M4*16))
                                                               SPACE 1
         DS    0H                      E7B2 VD
         DC    X'E7'                   - VECTOR DIVIDE
         DC    X'12'                    v1, v2
         DC    X'30'                    v3, reserved
         DC    HL2'&M4M5'               reserved, m5, m4, RXB
         DC    X'B2'
                                                               SPACE 1
         MEND
                                                               SPACE 2
***********************************************************************
* (pending E7B3 VR     - VECTOR REMAINDER
*  inclusion in SATK ASAM)
*
*     VDL Macro to help build VDL instruction
*        VDL   m4, m5
*
*        Note: v1, v2, v3 are fixed vector registers:
*                         v1 = 1; v2 = 2, v3 = 3
*              m4 are specified in hex eg. x'0'
*              m5 are specified in hex eg. x'0'
***********************************************************************
         MACRO
         VR    &M4,&M5
.*                                     &M4  - m4 for VR instruction
.*                                     &M5  - m5 for VR instruction
         LCLA  &M4M5
&M4M5    SETA  +((+&M5*256)+(+&M4*16))
                                                               SPACE 1
         DS    0H                      E7B3 VR
         DC    X'E7'                   - VECTOR REMAINDER
         DC    X'12'                    v1, v2
         DC    X'30'                    v3, reserved
         DC    HL2'&M4M5'               reserved, m5, m4, RXB
         DC    X'B3'
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
                                                                EJECT
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

* do cross check
         BAL   R15,XCHECK

*  validate results

         LGF   R1,READDR         get address of expected result
         CLC   V1OUTPUT,0(R1)    valid?
         BNE   FAILMSG              no, issue failed message

         LA    R12,4(0,R12)      next test address
         B     NEXTE7
                                                                 EJECT
*----------------------------------------------------------------------
* cross check that the result can be converted back to the source
* Note: skip xcheck - if skip requested (S or Y)
*                   - if instruction is not VDL, VRL, VD, VR
*----------------------------------------------------------------------
XCHECK   EQU   *
         MVC   SKIPXC,=CL8'SKIP XC '
         CLC   XCSKIP,=CL1'S'         skip xcheck requested
         BE    0(R15)                    skip == S, so exit
         CLC   XCSKIP,=CL1'Y'         skip xcheck requested
         BE    0(R15)                    skip == Y, so exit
*
*        cross check depends on the instruction
*
         MVC   SKIPXC,=CL8'        '
         CLC   OPNAME,=CL8'VDL     '
         BE    XCVDL
         CLC   OPNAME,=CL8'VRL     '
         BE    XCVRL
         CLC   OPNAME,=CL8'VD      '
         BE    XCVD
         CLC   OPNAME,=CL8'VR      '
         BE    XCVR
*                                      Not part of this xcheck test
         BR    R15                       so return
* ---------------------------------------------------------------------
* cross check result in V20, saved V1 in V21
*
* do cross check: VDL    - VECTOR DIVIDE LOGICAL
*
XCVDL    DS    0H
         VLR   V21,V1               save VDL result

         CLC   M4,=X'2'
         BNE   XCVDL3
         VRL   X'2',x'8'
         VMAL  V20,V21,V3,V1,x'2'
         B     XCCONT

XCVDL3   DS    0H
         CLC   M4,=X'3'
         BNE   XCVDL4
         VRL   x'3',x'8'
         VMAL  V20,V21,V3,V1,x'3'
         B     XCCONT

XCVDL4   DS    0H
         CLC   M4,=X'4'
         BNE   0(R15)
         VRL   X'4',x'8'
         VMAL  V20,V21,V3,V1,x'4'
         B     XCCONT
*
* do cross check: VRL    - VECTOR REMAINDER LOGICAL
*
XCVRL    DS    0H
         VLR   V21,V1               save VRL result

         CLC   M4,=X'2'
         BNE   XCVRL3
         VDL   X'2',x'8'
         VMAL  V20,V1,V3,V21,x'2'
         B     XCCONT

XCVRL3   DS    0H
         CLC   M4,=X'3'
         BNE   XCVRL4
         VDL   x'3',x'8'
         VMAL  V20,V1,V3,V21,x'3'
         B     XCCONT

XCVRL4   DS    0H
         CLC   M4,=X'4'
         BNE   0(R15)
         VDL   X'4',x'8'
         VMAL  V20,V1,V3,V21,x'4'
         B     XCCONT
*
* do cross check: VD    - VECTOR DIVIDE
*
XCVD     DS    0H
         VLR   V21,V1               save VDL result

         CLC   M4,=X'2'
         BNE   XCVD3
         VR    X'2',x'8'
         VMAL  V20,V21,V3,V1,x'2'
         B     XCCONT

XCVD3    DS    0H
         CLC   M4,=X'3'
         BNE   XCVD4
         VR    x'3',x'8'
         VMAL  V20,V21,V3,V1,x'3'
         B     XCCONT

XCVD4    DS    0H
         CLC   M4,=X'4'
         BNE   0(R15)
         VR    X'4',x'8'
         VMAL  V20,V21,V3,V1,x'4'
         B     XCCONT
*
* do cross check: VR     - VECTOR REMAINDER
*
XCVR     DS    0H
         VLR   V21,V1               save VR  result

         CLC   M4,=X'2'
         BNE   XCVR3
         VD    X'2',x'8'
         VMAL  V20,V1,V3,V21,x'2'
         B     XCCONT

XCVR3    DS    0H
         CLC   M4,=X'3'
         BNE   XCVR4
         VD    x'3',x'8'
         VMAL  V20,V1,V3,V21,x'3'
         B     XCCONT

XCVR4    DS    0H
         CLC   M4,=X'4'
         BNE   0(R15)
         VD    X'4',x'8'
         VMAL  V20,V1,V3,V21,x'4'
         B     XCCONT

* ---------------------------------------------------------------------
* check result of cross check
*
XCCONT DS   0H
         VST   V20,XCOUTPUT
         VLR   V1,V21               restore V1 for expected result test

         LGF   R1,V2ADDR            expected source address
         CLC   XCOUTPUT(16),0(R1)
         BNE   XCFAILMSG
         BR    R15                  return from xcheck


* xcheck failed message
XCFAILMSG DS   0H
         LH    R2,TNUM              get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   XCPTNUM(3),PRT3+13   fill in message with test #

         MVC   XCPNAME,OPNAME       fill in message with instruction

         XGR   R2,R2
         IC    R2,M4                get i3 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   XCPM4(2),PRT3+14     fill in message with m4 field

         XGR   R2,R2
         IC    R2,M5                get m4 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   XCPM5(2),PRT3+14     fill in message with m5 field
*
         ST    R15,XCR15            save r15
         LA    R0,XCPLNG            message length
         LA    R1,XCPLINE           messagfe address
         BAL   R15,RPTERROR
         L     R15,XCR15

         L     R0,=F'1'             set failed test indicator
         ST    R0,FAILED
         BR    R15                  return from xcheck

         DS    0FD
XCRESULT DS    XL16
XCV1     DS    XL16
XCV2     DS    XL16
XCR15    DS    FD
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
         LB    R2,m4                get m4 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM4(2),PRT3+14     fill in message with m4 field
*
         XGR   R2,R2
         IC    R2,M5                get m5 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM5(2),PRT3+14     fill in message with m5 field
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
PRTLINE  DC    C'      Test # '
PRTNUM   DC    C'xxx'
         DC    c' failed: wrong result for instruction '
PRTNAME  DC    CL8'xxxxxxxx'
         DC    C' with m4='
PRTM4    DC    C'xx'
         DC    C','
         DC    C' m5='
PRTM5    DC    C'xx'
         DC    C'.'
PRTLNG   EQU   *-PRTLINE
                                                               SPACE 2
***********************************************************************
*        TEST failed : XCHECK
***********************************************************************
*
*        XCHECK failed message
*
XCPLINE  DC    C'      Test # '
XCPTNUM  DC    C'xxx'
         DC    c' failed: cross check  for instruction '
XCPNAME  DC    CL8'xxxxxxxx'
         DC    C' with m4='
XCPM4    DC    C'xx'
         DC    C','
         DC    C' m5='
XCPM5    DC    C'xx'
         DC    C'.'
XCPLNG   EQU   *-XCPLINE
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
XCSKIP   DC    CL1' '            Y or S = skip cross check
M4       DC    HL1'00'        m4 used
M5       DC    HL1'00'        m5 used

OPNAME   DC    CL8' '         E7 name
V2ADDR   DC    A(0)           address of v2 source
V3ADDR   DC    A(0)           address of v3 source
RELEN    DC    A(0)           RESULT LENGTH
READDR   DC    A(0)           result (expected) address
         DS    FD                gap
V1OUTPUT DS    XL16           V1 Output
         DS    FD                gap
SKIPXC   DC    CL8' '         was cross check skipped?
         DS    FD                gap
XCOUTPUT DS    XL16           Cross check Output
         DS    FD                gap

*        test routine will be here (from VRR-c macro)
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
         VRR_C &INST,&M4,&M5,&SKIP
.*                               &INST   - VRR-c instruction under test
.*                               &m4     - m4 field

         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
         DC    CL1'&SKIP'        S or Y = skip cross check

         DC    &M4                m4
         DC    &M5                m5

         DC    CL8'&INST'        instruction name
         DC    A(RE&TNUM+16)     address of v2 source
         DC    A(RE&TNUM+32)     address of v3 source
         DC    A(16)             result length
REA&TNUM DC    A(RE&TNUM)        result address
         DS    FD                   gap
V1O&TNUM DS    XL16              V1 output
         DS    FD                   gap
         DC    CL8' '            was cross check skipped?
         DS    FD                   gap
XCO&TNUM DS    XL16              Cross check Output
         DS    FD                   gap
.*
*
X&TNUM   DS    0F
         LGF   R1,V2ADDR         load v2 source
         VL    v2,0(R1)

         LGF   R1,V3ADDR         load v3 source
         VL    v3,0(R1)

         &INST &M4,&m5           test instruction
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
*        E7 VRR-c tests
***********************************************************************
         PRINT DATA
*
*   E7B0 VDL    - VECTOR DIVIDE LOGICAL
*   E7B1 VRL    - VECTOR REMAINDER LOGICAL
*   E7B2 VD     - VECTOR DIVIDE
*   E7B3 VR     - VECTOR REMAINDER
*
*        VRR-c instruction, m4, m5
*              followed by
*                 16 byte expected result (V1)
*                 16 byte V2 source
*                 16 byte V3 source
*
*---------------------------------------------------------------------
*   VDL    - VECTOR DIVIDE LOGICAL
*---------------------------------------------------------------------
*Word    M4:  2, m5= 8 (IDC=1)

         VRR_C VDL,X'2',X'8',N
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'00000000 00000000 00000000 00000000'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VDL,X'2',X'8',N
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'01020303 05060707 090A0B0B 0D0E0F0F'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VDL,X'2',X'8',N
         DC    XL16'00000001 00000001 00000001 00000001'   result
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VDL,X'2',X'8',N
         DC    XL16'FFFFFFFF 7FFFFFFF 19999999 11111111'   result
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   v2
         DC    XL16'00000001 00000002 0000000A 0000000F'   v3

         VRR_C VDL,X'2',X'8',N
         DC    XL16'000000FE 00000032 0000001C 00000013'   result
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

*Doubleword    M4:  3, m5= 8 (IDC=1)

         VRR_C VDL,X'3',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VDL,X'3',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0102030305060707 090A0B0B0D0E0F0F'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VDL,X'3',X'8',N
         DC    XL16'0000000000000001 0000000000000001'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VDL,X'3',X'8',N
         DC    XL16'FFFFFFFFFFFFFFFF 7FFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000001 0000000000000002'   v3

         VRR_C VDL,X'3',X'8',N
         DC    XL16'1999999999999999 1111111111111111'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'000000000000000A 000000000000000F'   v3

         VRR_C VDL,X'3',X'8',N
         DC    XL16'000000000000007F 000000000000000E'   result
         DC    XL16'7FFFFFFFFFFFFFFF 7FFFFFFFFFFFFFFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

*Quadword      M4:  4, m5= 8 (IDC=1)

         VRR_C VDL,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VDL,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0102030305060707 090A0B0B0D0E0F0F'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VDL,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VDL,X'4',X'8',N
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000001'   v3

         VRR_C VDL,X'4',X'8',N
         DC    XL16'7FFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000002'   v3

         VRR_C VDL,X'4',X'8',N
         DC    XL16'1999999999999999 9999999999999999'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 000000000000000A'   v3

         VRR_C VDL,X'4',X'8',N
         DC    XL16'1111111111111111 1111111111111111'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 000000000000000F'   v3

         VRR_C VDL,X'4',X'8',N
         DC    XL16'0000000000000000 000000000000007F'   result
         DC    XL16'7FFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VDL,X'4',X'8',N
         DC    XL16'000000000000000E 29164CB5F714841E'   result
         DC    XL16'7FFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 090A0B0C0D0E0F10'   v3

*---------------------------------------------------------------------
*   VRL    - VECTOR REMAINDER LOGICAL
*---------------------------------------------------------------------
*Word    M4:  2, m5= 8 (IDC=1)

         VRR_C VRL,X'2',X'8',N
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'00000000 00000000 00000000 00000000'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VRL,X'2',X'8',N
         DC    XL16'01020303 05060707 090A0B0B 0D0E0F0F'   result
         DC    XL16'01020303 05060707 090A0B0B 0D0E0F0F'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VRL,X'2',X'8',N
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VRL,X'2',X'8',N
         DC    XL16'00000000 00000001 00000005 00000000'   result
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   v2
         DC    XL16'00000001 00000002 0000000A 0000000F'   v3

         VRR_C VRL,X'2',X'8',N
         DC    XL16'00010207 04D2A06F 02E6CAAF 07F4E1CF'   result
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

*Doubleword    M4:  3, m5= 8 (IDC=1)

         VRR_C VRL,X'3',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VRL,X'3',X'8',N
         DC    XL16'0102030305060707 090A0B0B0D0E0F0F'   result
         DC    XL16'0102030305060707 090A0B0B0D0E0F0F'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VRL,X'3',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VRL,X'3',X'8',N
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000001 0000000000000002'   v3

         VRR_C VRL,X'3',X'8',N
         DC    XL16'0000000000000005 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'000000000000000A 000000000000000F'   v3

         VRR_C VRL,X'3',X'8',N
         DC    XL16'0000810182028307 01736557493B2D1F'   result
         DC    XL16'7FFFFFFFFFFFFFFF 7FFFFFFFFFFFFFFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

*Quadword      M4:  4, m5= 8 (IDC=1)

         VRR_C VRL,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VRL,X'4',X'8',N
         DC    XL16'0102030305060707 090A0B0B0D0E0F0F'   result
         DC    XL16'0102030305060707 090A0B0B0D0E0F0F'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VRL,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VRL,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000001'   v3

         VRR_C VRL,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000002'   v3

         VRR_C VRL,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000005'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 000000000000000A'   v3

         VRR_C VRL,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 000000000000000F'   v3

         VRR_C VRL,X'4',X'8',N
         DC    XL16'0000810182028303 840485058606870F'   result
         DC    XL16'7FFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VRL,X'4',X'8',N
         DC    XL16'0000000000000000 02CD81E99B55FC1F'   result
         DC    XL16'7FFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000000 090A0B0C0D0E0F10'   v3

*---------------------------------------------------------------------
*   VD    - VECTOR DIVIDE
*---------------------------------------------------------------------
*Word    M4:  2, m5= 8 (IDC=1)

         VRR_C VD,X'2',X'8',N
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'00000000 00000000 00000000 00000000'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VD,X'2',X'8',N
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'01020303 05060707 090A0B0B 0D0E0F0F'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VD,X'2',X'8',N
         DC    XL16'00000001 00000001 00000001 00000001'   result
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VD,X'2',X'8',N
         DC    XL16'7FFFFFFF 3FFFFFFF 0CCCCCCC 08888888'   result
         DC    XL16'7FFFFFFF 7FFFFFFF 7FFFFFFF 7FFFFFFF'   v2
         DC    XL16'00000001 00000002 0000000A 0000000F'   v3

         VRR_C VD,X'2',X'8',N
         DC    XL16'0000007F 00000019 0000000E 00000009'   result
         DC    XL16'7FFFFFFF 7FFFFFFF 7FFFFFFF 7FFFFFFF'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VD,X'2',X'8',N
         DC    XL16'FFFFFFFF 00000000 00000000 00000000'   result
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   v2
         DC    XL16'00000001 00000002 0000000A 0000000F'   v3

         VRR_C VD,X'2',X'8',N
         DC    XL16'FFFFFF81 FFFFFFE7 FFFFFFF2 FFFFFFF7'   result
         DC    XL16'80000000 80000000 80000000 80000000'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VD,X'2',X'8',N
         DC    XL16'7FFFFFFF 3FFFFFFF 0CCCCCCC 08888888'   result
         DC    XL16'80000001 80000001 80000001 80000001'   v2
         DC    XL16'FFFFFFFF FFFFFFFE FFFFFFF6 FFFFFFF1'   v3

         VRR_C VD,X'2',X'8',N
         DC    XL16'00000001 00000001 00000001 00000001'   result
         DC    XL16'80000000 80000000 80000000 80000000'   v2
         DC    XL16'81020304 85060708 890A0B0C 8D0E0F10'   v3

* skip xc
         VRR_C VD,X'2',X'8',S
         DC    XL16'00000000 40000000 0CCCCCCC 08888888'   result
         DC    XL16'80000000 80000000 80000000 80000000'   v2
         DC    XL16'FFFFFFFF FFFFFFFE FFFFFFF6 FFFFFFF1'   v3

*Doubleword    M4:  3, m5= 8 (IDC=1)

         VRR_C VD,X'3',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VD,X'3',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0102030305060707 090A0B0B0D0E0F0F'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VD,X'3',X'8',N
         DC    XL16'0000000000000001 0000000000000001'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VD,X'3',X'8',N
         DC    XL16'7FFFFFFF7FFFFFFF 3FFFFFFFBFFFFFFF'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0000000000000001 0000000000000002'   v3

         VRR_C VD,X'3',X'8',N
         DC    XL16'0CCCCCCCBFFFFFFF 088888887FFFFFFF'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'000000000000000A 000000000000000F'   v3

         VRR_C VD,X'3',X'8',N
         DC    XL16'000000000000007F 000000000000000E'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VD,X'3',X'8',N
         DC    XL16'FFFFFFFFFFFFFFFF 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000001 0000000000000002'   v3

         VRR_C VD,X'3',X'8',N
         DC    XL16'FFFFFFFFFFFFFF83 FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFF81FFFFFFE7 FFFFFFF2FFFFFFF7'    v2
         DC    XL16'0000000100000002 0000000A0000000F'   v3

         VRR_C VD,X'3',X'8',N
         DC    XL16'FFFFFFFFFFFFFF81 FFFFFFFFFFFFFFF2'   result
         DC    XL16'8000000080000000 8000000080000000'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VD,X'3',X'8',N
         DC    XL16'3FFFFFFF3FFFFFFF 000000000E38E38D'   result
         DC    XL16'8000000180000001 8000000180000001'   v2
         DC    XL16'FFFFFFFFFFFFFFFE FFFFFFF6FFFFFFF1'   v3

         VRR_C VD,X'3',X'8',N
         DC    XL16'0000000000000001 0000000000000001'   result
         DC    XL16'8000000080000000 8000000080000000'   v2
         DC    XL16'8102030485060708 890A0B0C8D0E0F10'   v3

* skip XC
         VRR_C VD,X'3',X'8',S
         DC    XL16'0000000000000000 0888888888888888'   result
         DC    XL16'8000000000000000 8000000000000000'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFF1'   v3

*Quadwordword    M4:  4, m5= 8 (IDC=1)

         VRR_C VD,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0102030305060707 090A0B0B0D0E0F0F'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000001'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'3FFFFFFFBFFFFFFF BFFFFFFFBFFFFFFF'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000002'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'0CCCCCCCBFFFFFFF F333333326666666'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0000000000000000 000000000000000A'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'088888887FFFFFFF F77777776EEEEEEE'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0000000000000000 000000000000000F'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'0000000000000000 000000000000007F'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFF00'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFF00'   v2
         DC    XL16'0000000000000000 0000000000000001'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFF80'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFF00'   v2
         DC    XL16'0000000000000000 0000000000000002'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFE7'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFF00'   v2
         DC    XL16'0000000000000000 000000000000000A'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFEF'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFF00'   v2
         DC    XL16'0000000000000000 000000000000000F'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFF81'   result
         DC    XL16'8000000000000000 00000000000000FF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'007FFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'8000000000000000 00000000000000FF'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFF00'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'8000000000000000 00000000000000FF'   v2
         DC    XL16'8102030485060708 890A0B0C8D0E0F10'   v3

         VRR_C VD,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'8102030485060708 890A0B0C8D0E0F10'   v2
         DC    XL16'8000000000000000 00000000000000FF'   v3

* skip XC
         VRR_C VD,X'4',X'8',S
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'8000000000000000 0000000000000000'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v3

*---------------------------------------------------------------------
*   VR     - VECTOR REMAINDER
*---------------------------------------------------------------------
*Word    M4:  2, m5= 8 (IDC=1)

         VRR_C VR,X'2',X'8',N
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'00000000 00000000 00000000 00000000'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VR,X'2',X'8',N
         DC    XL16'01020303 05060707 090A0B0B 0D0E0F0F'   result
         DC    XL16'01020303 05060707 090A0B0B 0D0E0F0F'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VR,X'2',X'8',N
         DC    XL16'00000000 00000000 00000000 00000000'   result
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VR,X'2',X'8',N
         DC    XL16'00000000 00000001 00000007 00000007'   result
         DC    XL16'7FFFFFFF 7FFFFFFF 7FFFFFFF 7FFFFFFF'   v2
         DC    XL16'00000001 00000002 0000000A 0000000F'   v3

         VRR_C VR,X'2',X'8',N
         DC    XL16'00008103 02695037 01736557 0A81786F'   result
         DC    XL16'7FFFFFFF 7FFFFFFF 7FFFFFFF 7FFFFFFF'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VR,X'2',X'8',N
         DC    XL16'00000000 FFFFFFFF FFFFFFFF FFFFFFFF'   result
         DC    XL16'FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF'   v2
         DC    XL16'00000001 00000002 0000000A 0000000F'   v3

         VRR_C VR,X'2',X'8',N
         DC    XL16'FFFF7EFC FD96AFC8 FE8C9AA8 F57E8790'   result
         DC    XL16'80000000 80000000 80000000 80000000'   v2
         DC    XL16'01020304 05060708 090A0B0C 0D0E0F10'   v3

         VRR_C VR,X'2',X'8',N
         DC    XL16'00000000 FFFFFFFF FFFFFFF9 FFFFFFF9'   result
         DC    XL16'80000001 80000001 80000001 80000001'   v2
         DC    XL16'FFFFFFFF FFFFFFFE FFFFFFF6 FFFFFFF1'   v3

         VRR_C VR,X'2',X'8',N
         DC    XL16'FEFDFCFC FAF9F8F8 F6F5F4F4 F2F1F0F0'   result
         DC    XL16'80000000 80000000 80000000 80000000'   v2
         DC    XL16'81020304 85060708 890A0B0C 8D0E0F10'   v3

* skip xc
         VRR_C VR,X'2',X'8',S
         DC    XL16'00000000 00000000 FFFFFFF8 FFFFFFF8'   result
         DC    XL16'80000000 80000000 80000000 80000000'   v2
         DC    XL16'FFFFFFFF FFFFFFFE FFFFFFF6 FFFFFFF1'   v3

*Doubleword    M4:  3, m5= 8 (IDC=1)

         VRR_C VR,X'3',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VR,X'3',X'8',N
         DC    XL16'0102030305060707 090A0B0B0D0E0F0F'   result
         DC    XL16'0102030305060707 090A0B0B0D0E0F0F'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VR,X'3',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VR,X'3',X'8',N
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0000000000000001 0000000000000002'   v3

         VRR_C VR,X'3',X'8',N
         DC    XL16'0000000000000009 000000000000000E'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'000000000000000A 000000000000000F'   v3

         VRR_C VR,X'3',X'8',N
         DC    XL16'0000810102028307 01736556C93B2D1F'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VR,X'3',X'8',N
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v2
         DC    XL16'0000000000000001 0000000000000002'   v3

         VRR_C VR,X'3',X'8',N
         DC    XL16'FFFFFFFFFFFFFFFA FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFFFFFFFFFF00 FFFFFFFFFFFFFF00'    v2
         DC    XL16'000000000000000A 000000000000000F'   v3

         VRR_C VR,X'3',X'8',N
         DC    XL16'FFFF7EFEFDFD7CF8 FE8C9AA936C4D2E0'   result
         DC    XL16'8000000080000000 8000000080000000'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VR,X'3',X'8',N
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFF9'   result
         DC    XL16'8000000000000001 8000000000000001'   v2
         DC    XL16'FFFFFFFFFFFFFFFE FFFFFFFFFFFFFFF1'   v3

         VRR_C VR,X'3',X'8',N
         DC    XL16'FEFDFCFBFAF9F8F8 F6F5F4F3F2F1F0F0'   result
         DC    XL16'8000000080000000 8000000080000000'   v2
         DC    XL16'8102030485060708 890A0B0C8D0E0F10'   v3

* skip XC
         VRR_C VR,X'3',X'8',S
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'8000000000000000 8000000000000000'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v3

*Quadwordword    M4:  4, m5= 8 (IDC=1)

         VRR_C VR,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0000000000000000 0000000000000000'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'0102030305060707 090A0B0B0D0E0F0F'   result
         DC    XL16'0102030305060707 090A0B0B0D0E0F0F'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000001'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0000000000000000 0000000000000002'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000003'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0000000000000000 000000000000000A'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'0000000000000000 000000000000000D'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0000000000000000 000000000000000F'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'0000810102028303 040485050606870F'   result
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFF00'   v2
         DC    XL16'0000000000000000 0000000000000001'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFF00'   v2
         DC    XL16'0000000000000000 0000000000000002'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFA'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFF00'   v2
         DC    XL16'0000000000000000 000000000000000A'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFF00'   v2
         DC    XL16'0000000000000000 000000000000000F'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'FFFF7EFE7DFD7CFC 7BFB7AFA79F979EF'   result
         DC    XL16'8000000000000000 00000000000000FF'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result
         DC    XL16'8000000000000000 00000000000000FF'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFF00'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'FEFDFCFB7AF9F8F7 76F5F4F372F1F1EF'   result
         DC    XL16'8000000000000000 00000000000000FF'   v2
         DC    XL16'8102030485060708 890A0B0C8D0E0F10'   v3

         VRR_C VR,X'4',X'8',N
         DC    XL16'8102030485060708 890A0B0C8D0E0F10'   result
         DC    XL16'8102030485060708 890A0B0C8D0E0F10'   v2
         DC    XL16'8000000000000000 00000000000000FF'   v3

* skip XC
         VRR_C VR,X'4',X'8',S
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'8000000000000000 0000000000000000'   v2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v3






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
