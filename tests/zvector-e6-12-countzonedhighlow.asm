 TITLE 'zvector-e6-12-countzonedhighlow (Zvector E6 VRR-k)'
***********************************************************************
*
*        Zvector E6 instruction tests for VRR-k encoded:
*
*        E651 VCLZDP  - VECTOR COUNT LEADING ZERO DIGITS
*        E654 VUPKZH  - VECTOR UNPACK ZONED HIGH
*        E65C VUPKZL  - VECTOR UNPACK ZONED LOW
*
*        James Wekel June 2024
***********************************************************************

***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E6 VRR-k vector
*  count leading zero digits, unpack zoned high and low instructions.
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*    *Testcase zvector-e6-12-countzonedhighlow: VECTOR E6 VRR-k
*    *   Zvector E6 tests for VRR-k encoded instruction:
*    *
*    *   E651 VCLZDP  - VECTOR COUNT LEADING ZERO DIGITS
*    *   E654 VUPKZH  - VECTOR UNPACK ZONED HIGH
*    *   E65C VUPKZL  - VECTOR UNPACK ZONED LOW
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
*    diag8cmd    enable    # (needed for messages to Hercules console)
*    loadcore    "$(testpath)/zvector-e6-12-countzonedhighlow.core" 0x0
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
* Is vector-packed-decimal-enhancement facility 2 installed  (bit 192)
***********************************************************************

         FCHECK 192,'vector-packed-decimal-enhancement facility 2'
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
         LB    R1,M3          m3 has CS bit
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
*              and instruction l2
***********************************************************************
FAILMSG  EQU   *
         LH    R2,TNUM              get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTNUM(3),PRT3+13    fill in message with test #

         MVC   PRTNAME,OPNAME       fill in message with instruction

         XGR   R2,R2                get M3 as U8
         IC    R2,M3                and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM3(2),PRT3+14     fill in message with m3 field

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
TESTING  DC    F'0'                     current test #
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
         DC    C' with m3='
PRTM3    DC    C'xx'
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
M3       DC    HL1'00'        M3
CC       DC    HL1'00'        cc
CCMASK   DC    HL1'00'        not expected CC mask

OPNAME   DC    CL8' '         E6 name

RELEN    DC    A(0)           RESULT LENGTH
READDR   DC    A(0)           expected result address

**
*        test routine will be here (from VRS_D macro)
* followed by
*        16-byte EXPECTED RESULT
*        16-byte source
                                                                EJECT
***********************************************************************
*     Macros to help build test tables
*----------------------------------------------------------------------
*     VRR_K Macro to help build test tables
***********************************************************************
         MACRO
         VRR_K &INST,&M3,&CC
.*                               &INST  - instruction under test
.*                               &M3
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
         DC    HL1'&M3'          &M3
         DC    HL1'&CC'          cc
         DC    HL1'&XCC(&CC+1)'  cc failed mask

         DC    CL8'&INST'        instruction name
         DC    A(16)             result length
REA&TNUM DC    A(RE&TNUM)        result address
.*
*                                INSTRUCTION UNDER TEST ROUTINE
X&TNUM   DS    0F
         VL    V1,V1FUDGE        pollute V1
         VL    V2,RE&TNUM+16     get V2 source

         &INST V1,V2,&M3         test instruction

         VST   V1,V1OUTPUT       save
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
*        E6 VRR_K tests
***********************************************************************
ZVE6TST  CSECT ,
         DS    0F
                                                                SPACE 2
         PRINT DATA
*
*        E651 VCLZDP  - VECTOR COUNT LEADING ZERO DIGITS
*        E654 VUPKZH  - VECTOR UNPACK ZONED HIGH
*        E65C VUPKZL  - VECTOR UNPACK ZONED LOW
*
*        VRR_K instr, m3
*              followed by
*              v1     - expected result (16 bytes)
*              v2     - 16 byte packed decimal source

*---------------------------------------------------------------------
* VCLZDP  - VECTOR COUNT LEADING ZERO DIGITS
*---------------------------------------------------------------------
* VCLZDP simple                m3= 1 ( NV=0, NZ=0 , CS=1)
*                              m3= 3 ( NV=0, NZ=1 , CS=1)
*                              m3= 5 ( NV=1, NZ=0 , CS=1)
*                              m3= 7 ( NV=1, NZ=1 , CS=1)
         VRR_K VCLZDP,1,2
         DC    XL16'000000000000001D0000000000000000'   V1 result
         DC    XL16'0000000000000000000000000000010C'   V2 source

         VRR_K VCLZDP,1,2
         DC    XL16'000000000000000F0000000000000000'   V1 result
         DC    XL16'0000000000000001110000000000010C'   V2 source

         VRR_K VCLZDP,1,1
         DC    XL16'000000000000000F0000000000000000'   V1 result
         DC    XL16'0000000000000001110000000000010D'   V2 source

         VRR_K VCLZDP,1,0
         DC    XL16'000000000000001F0000000000000000'   V1 result
         DC    XL16'0000000000000000000000000000000C'   V2 source

         VRR_K VCLZDP,1,0
         DC    XL16'000000000000001F0000000000000000'   V1 result
         DC    XL16'0000000000000000000000000000000D'   V2 source

* VCLZDP with                  m3= 3 ( NV=0, NZ=1 , CS=1)

         VRR_K VCLZDP,3,2
         DC    XL16'000000000000001D0000000000000000'   V1 result
         DC    XL16'0000000000000000000000000000010C'   V2 source

         VRR_K VCLZDP,3,2
         DC    XL16'000000000000000F0000000000000000'   V1 result
         DC    XL16'0000000000000001110000000000010C'   V2 source

         VRR_K VCLZDP,3,1
         DC    XL16'000000000000000F0000000000000000'   V1 result
         DC    XL16'0000000000000001110000000000010D'   V2 source

         VRR_K VCLZDP,3,0
         DC    XL16'000000000000001F0000000000000000'   V1 result
         DC    XL16'0000000000000000000000000000000C'   V2 source

         VRR_K VCLZDP,3,1
         DC    XL16'000000000000001F0000000000000000'   V1 result
         DC    XL16'0000000000000000000000000000000D'   V2 source

* VCLZDP with                   m3= 5 ( NV=1, NZ=0 , CS=1)

         VRR_K VCLZDP,5,2
         DC    XL16'000000000000001D0000000000000000'   V1 result
         DC    XL16'0000000000000000000000000000010C'   V2 source

         VRR_K VCLZDP,5,2
         DC    XL16'000000000000000F0000000000000000'   V1 result
         DC    XL16'0000000000000001110000000000010C'   V2 source

         VRR_K VCLZDP,5,1
         DC    XL16'000000000000000F0000000000000000'   V1 result
         DC    XL16'0000000000000001110000000000010D'   V2 source

         VRR_K VCLZDP,5,0
         DC    XL16'000000000000001F0000000000000000'   V1 result
         DC    XL16'0000000000000000000000000000000C'   V2 source

         VRR_K VCLZDP,5,0
         DC    XL16'000000000000001F0000000000000000'   V1 result
         DC    XL16'0000000000000000000000000000000D'   V2 source

         VRR_K VCLZDP,5,3
         DC    XL16'00000000000000020000000000000000'   V1 result
         DC    XL16'00AAA00000000000000000000000000D'   V2 source

* VCLZDP with                   m3= 7 ( NV=1, NZ=1 , CS=1)

         VRR_K VCLZDP,7,2
         DC    XL16'000000000000001D0000000000000000'   V1 result
         DC    XL16'0000000000000000000000000000010C'   V2 source

         VRR_K VCLZDP,7,2
         DC    XL16'000000000000000F0000000000000000'   V1 result
         DC    XL16'0000000000000001110000000000010C'   V2 source

         VRR_K VCLZDP,7,1
         DC    XL16'000000000000000F0000000000000000'   V1 result
         DC    XL16'0000000000000001110000000000010D'   V2 source

         VRR_K VCLZDP,7,0
         DC    XL16'000000000000001F0000000000000000'   V1 result
         DC    XL16'0000000000000000000000000000000C'   V2 source

         VRR_K VCLZDP,7,1
         DC    XL16'000000000000001F0000000000000000'   V1 result
         DC    XL16'0000000000000000000000000000000D'   V2 source

         VRR_K VCLZDP,7,3
         DC    XL16'00000000000000020000000000000000'   V1 result
         DC    XL16'00AAA00000000000000000000000000D'   V2 source

*---------------------------------------------------------------------
* VUPKZH  - VECTOR UNPACK ZONED HIGH
*---------------------------------------------------------------------
* NOTE NOTE: VUPKZH does NOT set the condition code!
*        m3 bit 3 should be ZERO (which matches the CS bit of VCLZDP
*              so CC checking should be skipped!! )
*---------------------------------------------------------------------
* VUPKZH simple                m3= 0  ( NSV=0, NV=0 , fake CS=0)
*                              m3= 4  ( NSV=0, NV=1 , fake CS=0)
*                              m3= 8  ( NSV=1, NV=0 , fake CS=0)
*                              m3= 12 ( NSV=1, NV=1 , fake CS=0)
         VRR_K VUPKZH,0,0
         DC    XL16'F0F1F2F3F4F5F6F7F8F9F0F1F2F3F4F5'   V1 result
         DC    XL16'1234567890123456789012345678901C'   V2 source

         VRR_K VUPKZH,0,0
         DC    XL16'F0F5F6F7F8F9F0F1F2F3F4F5F6F7F8F9'   V1 result
         DC    XL16'5678901234567890123456789012345D'   V2 source

* VUPKZH                       m3= 4  ( NSV=0, NV=1 , fake CS=0)
         VRR_K VUPKZH,4,0
         DC    XL16'F0F1FFFFFFF5F6F7F8F9F0F1F2F3F4F5'   V1 result
         DC    XL16'1FFF567890123456789012345678901C'   V2 source

         VRR_K VUPKZH,4,0
         DC    XL16'F0F5F6F7F8F9F0F1F2F3F4F5F6F7F8F9'   V1 result
         DC    XL16'56789012345678901234567890123459'   V2 source

* VUPKZH                       m3= 8  ( NSV=1, NV=0 , fake CS=0)
         VRR_K VUPKZH,8,0
         DC    XL16'F0F1F2F3F4F5F6F7F8F9F0F1F2F3F4F5'   V1 result
         DC    XL16'1234567890123456789012345678901C'   V2 source

         VRR_K VUPKZH,8,0
         DC    XL16'F0F5F6F7F8F9F0F1F2F3F4F5F6F7F8F9'   V1 result
         DC    XL16'56789012345678901234567890123459'   V2 source

* VUPKZH                      m3= 12 ( NSV=1, NV=1 , fake CS=0)
         VRR_K VUPKZH,12,0
         DC    XL16'F0F1FFFFFFF5F6F7F8F9F0F1F2F3F4F5'   V1 result
         DC    XL16'1FFF567890123456789012345678901C'   V2 source

         VRR_K VUPKZH,12,0
         DC    XL16'F0F5F6F7F8F9F0F1F2F3F4F5F6F7F8F9'   V1 result
         DC    XL16'56789012345678901234567890123459'   V2 source

*---------------------------------------------------------------------
* VUPKZL  - VECTOR UNPACK ZONED LOW
*---------------------------------------------------------------------
* NOTE NOTE: VUPKZL does NOT set the condition code!
*        m3 bit 3 should be ZERO (which matches the CS bit of VCLZDP
*              so CC checking should be skipped!! )
*---------------------------------------------------------------------
* VUPKZL simple             m3= 0  ( NSV=0, NV=0 , P1=0, fake CS=0)
*                           m3= 2  ( NSV=0, NV=1 , P1=1, fake CS=0)
*                           m3= 4  ( NSV=0, NV=1 , P1=0, fake CS=0)
*                           m3= 6  ( NSV=0, NV=1 , P1=1, fake CS=0)
*                           m3= 8  ( NSV=1, NV=0 , P1=0, fake CS=0)
*                           m3= 10 ( NSV=1, NV=0 , P1=1, fake CS=0)
*                           m3= 12 ( NSV=1, NV=1 , P1=0, fake CS=0)
*                           m3= 14 ( NSV=1, NV=1 , P1=1, fake CS=0)
         VRR_K VUPKZL,0,0
         DC    XL16'F6F7F8F9F0F1F2F3F4F5F6F7F8F9F0C1'   V1 result
         DC    XL16'1234567890123456789012345678901C'   V2 source

         VRR_K VUPKZL,0,0
         DC    XL16'F0F1F2F3F4F5F6F7F8F9F0F1F2F3F4D5'   V1 result
         DC    XL16'5678901234567890123456789012345D'   V2 source

* VUPKZL                    m3= 2  ( NSV=0, NV=1 , P1=1, fake CS=0)
         VRR_K VUPKZL,2,0
         DC    XL16'F6F7F8F9F0F1F2F3F4F5F6F7F8F9F0F1'   V1 result
         DC    XL16'1234567890123456789012345678901C'   V2 source

         VRR_K VUPKZL,2,0
         DC    XL16'F0F1F2F3F4F5F6F7F8F9F0F1F2F3F4F5'   V1 result
         DC    XL16'5678901234567890123456789012345D'   V2 source

* VUPKZL                    m3= 4  ( NSV=0, NV=1 , P1=0, fake CS=0)
         VRR_K VUPKZL,4,0
         DC    XL16'F6F7F8F9F0F1F2F3F4F5F6F7F8F9F091'   V1 result
         DC    XL16'12345678901234567890123456789019'   V2 source

         VRR_K VUPKZL,4,0
         DC    XL16'F0F1F2F3F4F5F6F7F8F9FFF1F2F3F4D5'   V1 result
         DC    XL16'5678901234567890123456789F12345D'   V2 source

* VUPKZL                     m3= 6  ( NSV=0, NV=1 , P1=1, fake CS=0
         VRR_K VUPKZL,6,0
         DC    XL16'F6F7F8F9F0F1F2F3F4F5F6F7F8F9F0F1'   V1 result
         DC    XL16'12345678901234567890123456789019'   V2 source

         VRR_K VUPKZL,6,0
         DC    XL16'F0F1F2F3F4F5F6F7F8F9FFF1F2F3F4F5'   V1 result
         DC    XL16'5678901234567890123456789F12345D'   V2 source

* VUPKZL                      m3= 8  ( NSV=1, NV=0 , P1=0, fake CS=0)
         VRR_K VUPKZL,8,0
         DC    XL16'F6F7F8F9F0F1F2F3F4F5F6F7F8F9F091'   V1 result
         DC    XL16'12345678901234567890123456789019'   V2 source

         VRR_K VUPKZL,8,0
         DC    XL16'F0F1F2F3F4F5F6F7F8F9F0F1F2F3F4A5'   V1 result
         DC    XL16'5678901234567890123456789012345A'   V2 source

* VUPKZL                      m3= 10 ( NSV=1, NV=0 , P1=1, fake CS=0)
         VRR_K VUPKZL,10,0
         DC    XL16'F6F7F8F9F0F1F2F3F4F5F6F7F8F9F0F1'   V1 result
         DC    XL16'12345678901234567890123456789019'   V2 source

         VRR_K VUPKZL,10,0
         DC    XL16'F0F1F2F3F4F5F6F7F8F9F0F1F2F3F4F5'   V1 result
         DC    XL16'5678901234567890123456789012345A'   V2 source

* VUPKZL                      m3= 12 ( NSV=1, NV=1 , P1=0, fake CS=0)
         VRR_K VUPKZL,12,0
         DC    XL16'F6F7F8F9FDF1F2F3F4F5F6F7F8F9F091'   V1 result
         DC    XL16'1234567890123456789D123456789019'   V2 source

         VRR_K VUPKZL,12,0
         DC    XL16'F0F1F2F3F4F5F6F7F8F9FBF1F2F3F415'   V1 result
         DC    XL16'5678901234567890123456789B123451'   V2 source

* VUPKZL                      m3= 14 ( NSV=1, NV=1 , P1=1, fake CS=0)
         VRR_K VUPKZL,14,0
         DC    XL16'F6F7F8F9FDF1F2F3F4F5F6F7F8F9F0F1'   V1 result
         DC    XL16'1234567890123456789D123456789019'   V2 source

         VRR_K VUPKZL,14,0
         DC    XL16'F0F1F2F3F4F5F6F7F8F9FBF1F2F3F4F5'   V1 result
         DC    XL16'5678901234567890123456789B123451'   V2 source

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
