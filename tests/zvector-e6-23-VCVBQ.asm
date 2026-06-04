 TITLE 'zvector-e6-23-VCVBQ'
***********************************************************************
*
*        Zvector E6 instruction tests for VRR-k encoded:
*
*        E64E VCVBQ    - VECTOR CONVERT TO BINARY (128)
*
*        and partial testing of
*
*        E64A VCVDQ    - VECTOR CONVERT TO DECIMAL (128)
*
*        during cross check tests for VCVBQ
*
*        James Wekel March 2026
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E6 VRR-k vector
*  convert to binary (128), VCVBQ, instruction.
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*   *Testcase zvector-e6-23-VCVBQ
*   *
*   *        Zvector E6 instruction tests for VRR-k encoded:
*   *
*   *        E64E VCVBQ    - VECTOR CONVERT TO BINARY (128)
*   *
*   *        # --------------------------------------------------------
*   *        #  This tests only the basic function of the instruction.
*   *        #  Exceptions are NOT tested.
*   *        # --------------------------------------------------------
*   *
*   mainsize    2
*   numcpu      1
*   sysclear
*   archlvl     z/Arch
*
*   loadcore    "$(testpath)/zvector-e6-23-VCVBQ.core" 0x0
*
*   diag8cmd    enable    # (needed for messages to Hercules console)
*   runtest 5
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
SKT&SYSNDX DC  C'      Skipping tests: '
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
*     Instruction Macros
***********************************************************************
***********************************************************************
* (pending VCVBQ - VECTOR CONVERT TO BINARY (128)
*  inclusion in SATK ASAM)
*
*     VCVBQ Macro to help build VCVBQ instruction
*        VCVBQ   m3
*
*        Note: v1 and v2 are fixed vector registers: v1 = 2; v2 = 2
*              m3 can be specified in hex eg. x'0'
***********************************************************************
         MACRO
         VCVBQ   &M3
.*                                     &M3  - m3 for VCVBQ instruction
         LCLA  &V2M3
&M3ZZ    SETA  +(&M3*16)
                                                               SPACE 1
         DS    0H                      E64E VCVBQ
         DC    X'E6'                   - VECTOR CONVERT TO BINARY (128)
         DC    X'22'                    v2, v2
         DC    X'00'                   reserved
         DC    HL1'&M3ZZ'               m3, reserved
         DC    X'00'                   reserved, RXB
         DC    X'4E'
                                                               SPACE 1
         MEND
                                                               SPACE 2
***********************************************************************
* (pending VCVDQ - VECTOR CONVERT TO DECIMAL (128)
*  inclusion in SATK ASAM)
*
*     VCVDQ Macro to help build VCVBQ instruction
*        VCVDQ &I3,&M4
*
*        Note: v1 and v2 are fixed vector registers: v1=3, v2=2
*              i3 must be specified in hex eg. x'0'
*              m4 must be specified in hex eg. x'0'
***********************************************************************
         MACRO
         VCVDQ &I3,&M4
.*                                     &I3  - i3 for VCVDQ instruction
.*                                     &M4  - M4 for VCVDQ instruction
         LCLA  &I3M4
&M4I3    SETA  +(&M4*4096)+(&I3*16)
                                                               SPACE 1
         DS    0H                      E64A VCVDQ
         DC    X'E6'                    VECTOR CONVERT TO DECIMAL (128)
         DC    X'32'                    v3, v2
         DC    X'00'                   reserved
         DC    HL2'&M4I3'               m4, i3, rXB
         DC    X'4A'
                                                               SPACE 1
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
         USING  BEGIN,R8               FIRST Base Register
         USING  BEGIN+4096,R9          SECOND Base Register
         USING  BEGIN+8192,R10         THIRD Base Register

BEGIN    BALR  R8,0                    Initalize FIRST base register
         BCTR  R8,0                    Initalize FIRST base register
         BCTR  R8,0                    Initalize FIRST base register

         LA    R9,2048(,R8)            Initalize SECOND base register
         LA    R9,2048(,R9)            Initalize SECOND base register

         LA    R10,2048(,R9)           Initalize THIRD base register
         LA    R10,2048(,R10)          Initalize THIRD base register

         STCTL R0,R0,CTLR0             Store CR0 to enable AFP
         OI    CTLR0+1,X'04'           Turn on AFP bit
         OI    CTLR0+1,X'02'           Turn on Vector bit
         LCTL  R0,R0,CTLR0             Reload updated CR0
                                                                EJECT
***********************************************************************
* Is Vector packed-decimal enhancement 3 facility installed  (bit 199)
***********************************************************************

         FCHECK 199,'vector packed-decimal-enhancement 3'
                                                                EJECT
***********************************************************************
*              Do tests in the E6TESTS table
***********************************************************************

         L     R12,=A(E6TESTS)        get table of test addresses

NEXTE6   EQU   *
         L     R5,0(0,R12)            get test address
         LTR   R5,R5                     have a test?
         BZ    ENDTEST                      done?

         USING E6TEST,R5

         LH    R0,TNUM                save current test number
         ST    R0,TESTING             for easy reference

         L     R11,TSUB               get address of test routine
         BALR  R11,R11                do test

         BAL   R15,XCHECK

*  validate results, if not inexact

         LGF   R1,READDR              expected result address
         CLC   V1OUTPUT,0(R1)
         BNE   FAILMSG                no, issue failed message

DONEXT   EQU   *
         LA    R12,4(0,R12)           next test address
         B     NEXTE6
                                                                 EJECT
*----------------------------------------------------------------------
* cross check that the result can be converted back to the source
* Note: there are no exceptions as the original VALID packed decimal
*       can alway be converted to binary without overflow.
*----------------------------------------------------------------------
XCHECK   EQU   *
         MVC   SKIPXC,=CL8'SKIP XC '
         CLC   XCSKIP,=CL1'S'         skip xcheck requested
         BE    0(R15)                 skip = S, so exit
*
*        cross check depends on the instruction
*
         MVC   SKIPXC,=CL8'        '
         CLC   OPNAME,=CL8'VCVBQ   '
         BE    XCVCVDQ


         MVC   SKIPXC,=CL8'SKIP XC '
         BR    R15                  return from xcheck

* cross check: VCVDQ    - VECTOR CONVERT TO DECIMAL (128)
*
XCVCVDQ  DS    0F
         VCVDQ x'1F',x'00'
         VST   V3,XCOUTPUT

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
         IC    R2,M3                get m3 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   XCPM3(2),PRT3+14     fill in message with m3 field
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
         LH    R2,TNUM              get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTNUM(3),PRT3+13    fill in message with test #

         MVC   PRTNAME,OPNAME       fill in message with instruction
*
         XGR   R2,R2
         IC    R2,M3                get m3 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM3(2),PRT3+14     fill in message with m3 field
*
         LA    R0,PRTLNG            message length
         LA    R1,PRTLINE           messagfe address
         BAL   R15,RPTERROR
                                                                SPACE 2
***********************************************************************
* continue after a failed test
***********************************************************************
FAILCONT EQU   *
         L     R0,=F'1'          set failed test indicator
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
FPCINIT  DC    XL4'00000000'    FPC before test
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
TESTING  DC    F'0'                     current test #
                                                               SPACE 2
***********************************************************************
*        TEST failed : result messgae
***********************************************************************
*
*        failed message and associated editting
*
PRTLINE  DC    C'      Test # '
PRTNUM   DC    C'xxx'
         DC    c' failed for instruction '
PRTNAME  DC    CL8'xxxxxxxx'
         DC    C' with m3='
PRTM3    DC    C'xx'
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
         DC    c' XCHECK failed for instruction '
XCPNAME  DC    CL8'xxxxxxxx'
         DC    C' with m3='
XCPM3    DC    C'xx'
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
*
*        Vector instruction results, pollution and input
*
         DS    XL16                                        gap
V1FUDGE  DC    XL16'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'    V1 FUDGE
V1INPUT  DC    XL16'1234567890123456789012345678901D'    V1 input
         DS    XL16
                                                                EJECT
***********************************************************************
*        E6TEST DSECT
***********************************************************************
                                                                SPACE 2
E6TEST   DSECT ,
TSUB     DC    A(0)              pointer  to test
TNUM     DC    H'00'             Test Number
         DC    X'00'
XCSKIP   DC    CL1' '            Y = skip cross check
M3       DC    HL1'00'           m3 used

V2ADDR   DC    A(0)              address of v2: 16-byte packed decimal
OPNAME   DC    CL8' '            E6 name
RELEN    DC    A(0)              result length
READDR   DC    A(0)              expected result address
         DS    FD                   gap
V1OUTPUT DS    XL16              V1 Output
         DS    FD                   gap
SKIPXC   DC    CL8' '            was cross check skipped?
         DS    FD                   gap
XCOUTPUT DS    XL16              Cross check Output
         DS    XL16
         DS    FD                   gap
WK1      DS    F
         DS    0F
**
*        test routine will be here (from VRR-k macro)
                                                                SPACE 2
ZVE6TST  CSECT ,
         DS    0F
                                                                SPACE 2

*
* macro to generate individual test
*
         MACRO
         VRR_K &INST,&M3,&SKIP
.*                               &INST  - VRR-k instruction under test
.*                               &m3    - m3 field
.*                               &SKIP  - S = skip cross check
         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
         DC    CL1'&SKIP'        Y = skip cross check
         DC    &M3               m3
V2_&TNUM DC    A(RE&TNUM+16)     address of v2: 16-byte packed decimal
         DC    CL8'&INST'        instruction name
         DC    A(16)             result length
         DC    A(RE&TNUM)        address of expected resul
         DS    FD                gap
V1O&TNUM DS    XL16              V1 output
         DS    FD                   gap
         DC    CL8' '            was cross check skipped?
         DS    FD                   gap
XCO&TNUM DS    XL16              Cross check Output
         DS    XL16
         DS    FD                   gap
         DS    F
.*
*
X&TNUM   DS    0F

         VL    V1,V1FUDGE

         LGF   R2,V2_&TNUM        get v2 address
         VL    V2,0(R2)

         &INST &M3               test instruction (dest is source)

         VST   V2,V1O&TNUM       save instruction result
         BR    R11               return

RE&TNUM  DS    0F                expected 16 byte result
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
         DC    A(T&CUR)          TEST &CUR
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
*        E6 VRR-k tests
***********************************************************************
         PRINT DATA
*
*        E64E VCVBQ    - VECTOR CONVERT TO BINARY (128)
*
*----------------------------------------------------------------------
*   VRR-k   instruction, m3, SKIP      (Skip cross check if "S")
*   followed by
*              v1 - 16 byte expected result
*              v2 - 16 byte packed decimal source vector
*----------------------------------------------------------------------
*  VCVBQ    - VECTOR CONVERT TO BINARY (128)
*----------------------------------------------------------------------
*        M3:   0   (p2 = 0, lb = 0)
*
         VRR_K VCVBQ,X'0',N
         DC    XL16'0000000000000000 0000000000000000'      v1
         DC    XL16'0000000000000000 000000000000000C'      v2

         VRR_K VCVBQ,X'0',N
         DC    XL16'0000000000000000 0000000000000001'      v1
         DC    XL16'0000000000000000 000000000000001C'      v2

         VRR_K VCVBQ,X'0',N
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'      v1
         DC    XL16'0000000000000000 000000000000001D'      v2

         VRR_K VCVBQ,X'0',N
         DC    XL16'0000000000000000 7FFFFFFFFFFFFFFF'      v1
         DC    XL16'0000000000009223 372036854775807C'      v2

         VRR_K VCVBQ,X'0',N
         DC    XL16'FFFFFFFFFFFFFFFF 8000000000000001'      v1
         DC    XL16'0000000000009223 372036854775807D'      v2

         VRR_K VCVBQ,X'0',N
         DC    XL16'0000007E37BE2022 C0914B267FFFFFFF'      v1
         DC    XL16'9999999999999999 999999999999999C'      v2

         VRR_K VCVBQ,X'0',N
         DC    XL16'FFFFFF81C841DFDD 3F6EB4D980000001'      v1
         DC    XL16'9999999999999999 999999999999999D'      v2

*----------------------------------------------------------------------
*        M3:   2   (p2 = 0, lb = 1)
*
         VRR_K VCVBQ,X'2',N
         DC    XL16'0000000000000000 0000000000000000'      v1
         DC    XL16'0000000000000000 000000000000000C'      v2

         VRR_K VCVBQ,X'2',N
         DC    XL16'0000000000000000 0000000000000001'      v1
         DC    XL16'0000000000000000 000000000000001C'      v2

         VRR_K VCVBQ,X'2',S            SKIP CROSS CHECK
         DC    XL16'0000000000000000 0000000000000001'      v1
         DC    XL16'0000000000000000 000000000000001D'      v2

         VRR_K VCVBQ,X'2',N
         DC    XL16'0000000000000000 7FFFFFFFFFFFFFFF'      v1
         DC    XL16'0000000000009223 372036854775807C'      v2

         VRR_K VCVBQ,X'2',S            SKIP CROSS CHECK
         DC    XL16'0000000000000000 7FFFFFFFFFFFFFFF'      v1
         DC    XL16'0000000000009223 372036854775807D'      v2

         VRR_K VCVBQ,X'2',N
         DC    XL16'0000007E37BE2022 C0914B267FFFFFFF'      v1
         DC    XL16'9999999999999999 999999999999999C'      v2

         VRR_K VCVBQ,X'2',S            SKIP CROSS CHECK
         DC    XL16'0000007E37BE2022 C0914B267FFFFFFF'      v1
         DC    XL16'9999999999999999 999999999999999D'      v2

*----------------------------------------------------------------------
*        M3:   8   (p2 = 1, lb = 0)
*
         VRR_K VCVBQ,X'8',N
         DC    XL16'0000000000000000 0000000000000000'      v1
         DC    XL16'0000000000000000 000000000000000C'      v2

         VRR_K VCVBQ,X'8',N
         DC    XL16'0000000000000000 0000000000000001'      v1
         DC    XL16'0000000000000000 000000000000001C'      v2

         VRR_K VCVBQ,X'8',S            SKIP CROSS CHECK
         DC    XL16'0000000000000000 0000000000000001'      v1
         DC    XL16'0000000000000000 000000000000001D'      v2

         VRR_K VCVBQ,X'8',N
         DC    XL16'0000000000000000 7FFFFFFFFFFFFFFF'      v1
         DC    XL16'0000000000009223 372036854775807C'      v2

         VRR_K VCVBQ,X'8',S            SKIP CROSS CHECK
         DC    XL16'0000000000000000 7FFFFFFFFFFFFFFF'      v1
         DC    XL16'0000000000009223 372036854775807D'      v2

         VRR_K VCVBQ,X'8',N
         DC    XL16'0000007E37BE2022 C0914B267FFFFFFF'      v1
         DC    XL16'9999999999999999 999999999999999C'      v2

         VRR_K VCVBQ,X'8',S            SKIP CROSS CHECK
         DC    XL16'0000007E37BE2022 C0914B267FFFFFFF'      v1
         DC    XL16'9999999999999999 999999999999999D'      v2

* with invalid sign .... SKIP CROSS CHECKS

         VRR_K VCVBQ,X'8',S
         DC    XL16'0000000000000000 0000000000000000'      v1
         DC    XL16'0000000000000000 0000000000000009'      v2

         VRR_K VCVBQ,X'8',S
         DC    XL16'0000000000000000 0000000000000001'      v1
         DC    XL16'0000000000000000 0000000000000019'      v2

         VRR_K VCVBQ,X'8',S
         DC    XL16'0000000000000000 7FFFFFFFFFFFFFFF'      v1
         DC    XL16'0000000000009223 3720368547758079'      v2

         VRR_K VCVBQ,X'8',S
         DC    XL16'0000007E37BE2022 C0914B267FFFFFFF'      v1
         DC    XL16'9999999999999999 9999999999999999'      v2


*----------------------------------------------------------------------
*        M3:   A   (p2 = 1, lb = 1)
*
         VRR_K VCVBQ,X'A',N
         DC    XL16'0000000000000000 0000000000000000'      v1
         DC    XL16'0000000000000000 000000000000000C'      v2

         VRR_K VCVBQ,X'A',N
         DC    XL16'0000000000000000 0000000000000001'      v1
         DC    XL16'0000000000000000 000000000000001C'      v2

         VRR_K VCVBQ,X'A',S            SKIP CROSS CHECK
         DC    XL16'0000000000000000 0000000000000001'      v1
         DC    XL16'0000000000000000 000000000000001D'      v2

         VRR_K VCVBQ,X'A',N
         DC    XL16'0000000000000000 7FFFFFFFFFFFFFFF'      v1
         DC    XL16'0000000000009223 372036854775807C'      v2

         VRR_K VCVBQ,X'A',S            SKIP CROSS CHECK
         DC    XL16'0000000000000000 7FFFFFFFFFFFFFFF'      v1
         DC    XL16'0000000000009223 372036854775807D'      v2

         VRR_K VCVBQ,X'A',N
         DC    XL16'0000007E37BE2022 C0914B267FFFFFFF'      v1
         DC    XL16'9999999999999999 999999999999999C'      v2

         VRR_K VCVBQ,X'A',S            SKIP CROSS CHECK
         DC    XL16'0000007E37BE2022 C0914B267FFFFFFF'      v1
         DC    XL16'9999999999999999 999999999999999D'      v2

* with invalid sign .... SKIP CROSS CHECKS

         VRR_K VCVBQ,X'A',S
         DC    XL16'0000000000000000 0000000000000000'      v1
         DC    XL16'0000000000000000 0000000000000009'      v2

         VRR_K VCVBQ,X'A',S
         DC    XL16'0000000000000000 0000000000000001'      v1
         DC    XL16'0000000000000000 0000000000000019'      v2

         VRR_K VCVBQ,X'A',S
         DC    XL16'0000000000000000 7FFFFFFFFFFFFFFF'      v1
         DC    XL16'0000000000009223 3720368547758079'      v2

         VRR_K VCVBQ,X'A',S
         DC    XL16'0000007E37BE2022 C0914B267FFFFFFF'      v1
         DC    XL16'9999999999999999 9999999999999999'      v2



         DC    F'0'     END OF TABLE
         DC    F'0'
*
* table of pointers to individual tests
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
                                                               SPACE 2
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
