 TITLE 'zvector-e6-20-NNPconvert (Zvector E6 VRR-c)'
***********************************************************************
*
*        EXPERIMENTAL pending further PoP definition
*
*        Zvector E6 instruction tests for VRR-c encoded:
*
*        E675  VCRNF   - VECTOR FP CONVERT AND ROUND TO NNP
*
*        and partial testing of
*
*        E656 VCLFNH   - VECTOR FP CONVERT AND LENGTHEN FROM NNP HIGH
*        E65E VCLFNL   - VECTOR FP CONVERT AND LENGTHEN FROM NNP LOW
*
*        during cross check tests for VCRNF
*
*        James Wekel August 2024
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests EXPERIMENTAL functioning of the z/arch E6 VRR-c
*  Neural-network-processing-assist facility vector instructions.
*  These test are EXPERIMENTAL pending further PoP definition of
*  NNP-data-type-1.
*
*  If requested and if VXC == 0 after test instruction execution,
*  a cross check test is performed. A cross check uses the result
*  of the instruction test to recreate the test source.
*
*  Exceptions (including trapable IEEE exceptions) are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*   *Testcase zvector-e6-21-VCRNF
*   *
*   *        EXPERIMENTAL pending further PoP definition
*   *
*   *        Zvector E6 instruction tests for VRR-c encoded:
*   *
*   *        E675 VCRNF   - VECTOR FP CONVERT AND ROUND TO NNP
*   *
*   *        # -------------------------------------------------------
*   *        #  This tests only the basic function of the instruction.
*   *        #  Exceptions are NOT tested.
*   *        # -------------------------------------------------------
*   *
*   mainsize    2
*   numcpu      1
*   sysclear
*   archlvl     z/Arch
*
*   loadcore    "$(testpath)/zvector-e6-21-VCRNF.core" 0x0
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

***********************************************************************
* Is Neural-network-processing-assist facility 2 installed  (bit 165)
***********************************************************************

         FCHECK 165,'Neural-network-processing-assist'
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
*
* validate FPC first
*
         CLC   FLG(1),FPC_R+1         expected FPC flags?
         BNE   FAILMSG                no, issue failed message
         CLC   VXC(1),FPC_R+2         expected VXC?
         BNE   FAILMSG                no, issue failed message

* then validate results, if not inexact

         XGR   R1,R1
         IC    R1,FPC_R+1             FPC flags
         N     R1,=XL4'00000008'      check inexact flag
         LTR   R1,R1
         BNZ   DONEXT

         LGF   R1,READDR              expected result address
         CLC   V1OUTPUT,0(R1)
         BNE   FAILMSG                no, issue failed message

DONEXT   EQU   *
         LA    R12,4(0,R12)           next test address
         B     NEXTE6
                                                                 EJECT
*----------------------------------------------------------------------
* cross check that the result can be converted back to the source
* where there were NO exceptions (DXC/VXC = 0)
*----------------------------------------------------------------------
XCHECK   EQU   *
         MVC   SKIPXC,=CL8'SKIP XC '
         CLC   XCSKIP,=CL1'S'         skip xcheck requested
         BE    0(R15)                 skip = S, so exit
         CLI   FPC_R+2,0             Only Xcheck no VXC error
         BNZ   0(R15)                 some VXC error , so exit
*
*        cross check depends on the instruction
*
         MVC   SKIPXC,=CL8'        '
         CLC   OPNAME,=CL8'VCRNF'
         BE    XCVCRNF


         MVC   SKIPXC,=CL8'SKIP XC '
         BR    R15                  return from xcheck

* cross check: VCRNF   - VECTOR FP CONVERT AND ROUND TO NNP
*
XCVCRNF  DS    0F
         LFPC  FPCINIT              initialize FPC

         VL    V16,V1OUTPUT
         VCLFNH V15,V16,2,0
         STFPC FPC_XC1              save 1st FPC
         VST   V15,XCOUTPUT

         LFPC  FPCINIT              initialize FPC

         VL    V16,V1OUTPUT
         VCLFNL V15,V16,2,0
         STFPC FPC_XC2              save 2nd FPC
         VST   V15,XCOUTPUT+16

         CLI   FPC_XC1+2,0          1st FPC
         BNZ   0(R15)               some VXC error , so exit

         CLI   FPC_XC2+2,0          2nd FPC
         BNZ   0(R15)               some VXC error , so exit

         LGF   R1,V2ADDR            expected source address
         CLC   XCOUTPUT(32),0(R1)
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
         IC    R2,M4                get m3 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   XCPM4(2),PRT3+14     fill in message with m3 field
*
         XGR   R2,R2
         IC    R2,M5                get m4 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   XCPM5(2),PRT3+14     fill in message with m4 field

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
         IC    R2,M4                get m3 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM4(2),PRT3+14     fill in message with m3 field
*
         XGR   R2,R2
         IC    R2,M5                get m4 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM5(2),PRT3+14     fill in message with m4 field
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
         DC    C' with m4='
PRTM4    DC    C'xx'
         DC    C','
         DC    C' with m5='
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
         DC    c' XCHECK failed for instruction '
XCPNAME  DC    CL8'xxxxxxxx'
         DC    C' with m4='
XCPM4    DC    C'xx'
         DC    C' with m5='
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
M4       DC    HL1'00'           m4 used
M5       DC    HL1'00'           m5 used
FLG      DC    X'00'             expected FPC flags
VXC      DC    X'00'             VXC expected
V2ADDR   DC    A(0)              address of v2: 16-byte packed decimal
OPNAME   DC    CL8' '            E6 name
RELEN    DC    A(0)              result length
READDR   DC    A(0)              expected result address
         DS    FD                   gap
V1OUTPUT DS    XL16              V1 Output
         DS    FD                   gap
FPC_R    DS    F                 FPC after instruction
         DS    FD                   gap
SKIPXC   DC    CL8' '            was cross check skipped?
         DS    FD                   gap
XCOUTPUT DS    XL16              Cross check Output
         DS    XL16
         DS    FD                   gap
FPC_XC1  DS    F                 1st cross check FPC
FPC_XC2  DS    F                 2nd cross check FPC
         DS    FD                   gap
WK1      DS    F                 debug area
WK2      DS    F
         DS    0F
**
*        test routine will be here (from VRR-c macro)
                                                                SPACE 2
ZVE6TST  CSECT ,
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
         VRR_C &INST,&M4,&M5,&FLAGS,&VXC,&SKIP
.*                               &INST  - VRR-c instruction under test
.*                               &m4    - m4 field
.*                               &m5    - m5 field
.*                               &flags - expected FPC flags
.*                               &VXC   - expected VXC
.*                               &SKIP  - S = skip cross check
         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
         DC    CL1'&SKIP'        Y = skip cross check
         DC    HL1'&M4'          m4
         DC    HL1'&M5'          m5
FLG&TNUM DC    X'&FLAGS'         expected FPC flags
VXC&TNUM DC    X'&VXC'           expected VXC
V2_&TNUM DC    A(RE&TNUM+16)     address of v2: 16-byte packed decimal
         DC    CL8'&INST'        instruction name
         DC    A(16)             result length
         DC    A(RE&TNUM)        address of expected resul
         DS    FD                gap
V1O&TNUM DS    XL16              V1 output
         DS    FD                   gap
FPC_R_&TNUM DS F                 FPC after instruction
         DS    FD                   gap
         DC    CL8' '            was cross check skipped?
         DS    FD                   gap
XCO&TNUM DS    XL16              Cross check Output
         DS    XL16
         DS    FD                   gap
FPC_XC_&TNUM DS F               1st cross check FPC
         DS    F                2nd cross check FPC
         DS    FD                   gap
         DS    F                 debug area
         DS    F
.*
*
X&TNUM   DS    0F
         LFPC  FPCINIT           initialize FPC

         LGF   R2,V2_&TNUM       get v2
         VLM   V22,V23,0(R2)

         &INST V22,V22,V23,&M4,&M5   test instruction (dest is source)

         STFPC FPC_R_&TNUM       save FPC
         VST   V22,V1O&TNUM      save instruction result

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
*        E6 VRR-c tests
***********************************************************************
         PRINT DATA
*
*        E675  VCRNF   - VECTOR FP CONVERT AND ROUND TO NNP
*
*----------------------------------------------------------------------
*   VRR-c   instruction, m4, m5, flags, VXC, SKIP
*   followed by
*              followed by
*              v1 - 16 byte expected result
*              v2 - 32 byte source
*----------------------------------------------------------------------
* VCRNF   - VECTOR FP CONVERT AND ROUND TO NNP
*----------------------------------------------------------------------
* Short Float -> dlfloat  (with cross check dlfloat -> short float)
*
* some of these tests use numbers from PoP SA22-7832-13,
* Figure 9-2. Examples of Floating-Point Numbers ( page 9-6 )
*
* +0     simple instruction test and 'test' test
         VRR_C VCRNF,0,2,00,00,N
         DC    XL16'00000000000000000000000000000000'
         DC    XL16'00000000000000000000000000000000'
         DC    XL16'00000000000000000000000000000000'

* +1, -1
         VRR_C VCRNF,0,2,00,00,N
         DC    XL16'3E00000000000000BE00000000000000'
         DC    XL16'3F800000000000000000000000000000'
         DC    XL16'BF800000000000000000000000000000'

* +.5, -.5
         VRR_C VCRNF,0,2,00,00,N
         DC    XL16'3C00000000000000BC00000000000000'
         DC    XL16'3F000000000000000000000000000000'
         DC    XL16'BF000000000000000000000000000000'

* +1/64, -1/64
         VRR_C VCRNF,0,2,00,00,N
         DC    XL16'3200000000000000B200000000000000'
         DC    XL16'3C800000000000000000000000000000'
         DC    XL16'BC800000000000000000000000000000'

* +0, -0
         VRR_C VCRNF,0,2,00,00,N
         DC    XL16'00000000000000008000000000000000'
         DC    XL16'00000000000000000000000000000000'
         DC    XL16'80000000000000000000000000000000'

* +15, -15
         VRR_C VCRNF,0,2,00,00,N
         DC    XL16'45C0000000000000C5C0000000000000'
         DC    XL16'41700000000000000000000000000000'
         DC    XL16'C1700000000000000000000000000000'

* +20/7, - 20/7
         VRR_C VCRNF,0,2,00,00,S                 skip xc: lost digits
         DC    XL16'40DB000000000000C0DB000000000000'
         DC    XL16'4036DB6E000000000000000000000000'
         DC    XL16'C036DB6E000000000000000000000000'

* +2^(-126), -2^(-126)
         VRR_C VCRNF,0,2,10,44,S                 skip xc: underflow
         DC    XL16'00000000000000008000000000000000'
         DC    XL16'00800000000000000000000000000000'
         DC    XL16'80800000000000000000000000000000'

* +2^(-149), -2^(-149) - subnormal
         VRR_C VCRNF,0,2,10,44,S                 skip xc: underflow
         DC    XL16'00000000000000008000000000000000'
         DC    XL16'00000001000000000000000000000000'
         DC    XL16'80800001000000000000000000000000'

* +2^(128) * (1 - 2^(-24)) , - +2^(128) * (1 - 2^(-24))
         VRR_C VCRNF,0,2,20,43,S                 skip xc: overflow
         DC    XL16'7FFE000000000000FFFE000000000000'
         DC    XL16'7F7FFFFF000000000000000000000000'
         DC    XL16'FF7FFFFF000000000000000000000000'

* NAN, -NAN
         VRR_C VCRNF,0,2,00,00,S                 skip xc: invalid on XC
         DC    XL16'7FFF000000000000FFFF000000000000'
         DC    XL16'7FC00000000000000000000000000000'
         DC    XL16'FFC00000000000000000000000000000'

* bad m4 - inexact
         VRR_C VCRNF,2,2,08,05,S                 skip xc: inexact
         DC    XL16'7FFF000000000000FFFF000000000000'
         DC    XL16'7FC00000000000000000000000000000'
         DC    XL16'FFC00000000000000000000000000000'

* bad m5 - inexact
         VRR_C VCRNF,0,3,08,05,S                 skip xc: inexact
         DC    XL16'7FFF000000000000FFFF000000000000'
         DC    XL16'7FC00000000000000000000000000000'
         DC    XL16'FFC00000000000000000000000000000'


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
FPR0     EQU   0
FPR1     EQU   1
FPR2     EQU   2
FPR3     EQU   3
FPR4     EQU   4
FPR5     EQU   5
FPR6     EQU   6
FPR7     EQU   7
FPR8     EQU   8
FPR9     EQU   9
FPR10    EQU   10
FPR11    EQU   11
FPR12    EQU   12
FPR13    EQU   13
FPR14    EQU   14
FPR15    EQU   15
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
