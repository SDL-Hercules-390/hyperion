 TITLE 'zvector-e7-17-AddSub'
***********************************************************************
*
*   Zvector E7 instruction tests for VRR-c encoded:
*
*   E7F1 VACC   - Vector Add Compute Carry
*   E7F3 VA     - Vector Add
*   E7F5 VSCBI  - Vector Subtract Compute Borrow Indication
*   E7F7 VS     - Vector Subtract
*
*        James Wekel March 2025
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E7 VRR-c Vector
*  Add and Vector Subtract instructions.
*
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*   *Testcase zvector-e7-17-AddSub
*   *
*   *   Zvector E7 instruction tests for VRR-c encoded:
*   *
*   *   E7F1 VACC   - Vector Add Compute Carry
*   *   E7F3 VA     - Vector Add
*   *   E7F5 VSCBI  - Vector Subtract Compute Borrow Indication
*   *   E7F7 VS     - Vector Subtract
*   *
*   *   # ------------------------------------------------------------
*   *   #  This tests only the basic function of the instructions.
*   *   #  Exceptions are NOT tested.
*   *   # ------------------------------------------------------------
*   *
*   mainsize    2
*   numcpu      1
*   sysclear
*   archlvl     z/Arch
*
*   loadcore    "$(testpath)/zvector-e7-17-AddSub.core" 0x0
*
*   diag8cmd    enable    # (needed for messages to Hercules console)
*   runtest     5
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
         USING  BEGIN,R8        FIRST Base Register
         USING  BEGIN+4096,R9   SECOND Base Register
         USING  BEGIN+8192,R10  THIRD Base Register
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
PRTLINE  DC    C'         Test # '
PRTNUM   DC    C'xxx'
         DC    c' failed for instruction '
PRTNAME  DC    CL8'xxxxxxxx'
         DC    C' with m4='
PRTm4    DC    C'xx'
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
M4       DC    HL1'00'        m4 used

OPNAME   DC    CL8' '         E7 name
V2ADDR   DC    A(0)           address of v2 source
V3ADDR   DC    A(0)           address of v3 source
RELEN    DC    A(0)           RESULT LENGTH
READDR   DC    A(0)           result (expected) address
         DS    FD                gap
V1OUTPUT DS    XL16           V1 Output
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
         VRR_C &INST,&M4
.*                               &INST   - VRR-c instruction under test
.*                               &m4     - m4 field

         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
         DC    HL1'&M4'          m4
         DC    CL8'&INST'        instruction name
         DC    A(RE&TNUM+16)     address of v2 source
         DC    A(RE&TNUM+32)     address of v3 source
         DC    A(16)             result length
REA&TNUM DC    A(RE&TNUM)        result address
         DS    FD                gap
V1O&TNUM DS    XL16              V1 output
         DS    FD                gap
.*
*
X&TNUM   DS    0F
         LGF   R1,V2ADDR         load v2 source
         VL    v22,0(R1)         use v22 to test decoder

         LGF   R1,V3ADDR         load v3 source
         VL    v23,0(R1)         use v23 to test decoder

         &INST V22,V22,V23,&M4    test instruction (dest is a source)
         VST   V22,V1O&TNUM       save v1 output

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
*   E7F1 VACC   - Vector Add Compute Carry
*   E7F3 VA     - Vector Add
*   E7F5 VSCBI  - Vector Subtract Compute Borrow Indication
*   E7F7 VS     - Vector Subtract
*
*        VRR-c instruction, m4
*              followed by
*                 16 byte expected result (V1)
*                 16 byte V2 source
*                 16 byte V3 source
*
*---------------------------------------------------------------------
*   VA     - Vector Add
*---------------------------------------------------------------------
*Byte
         VRR_C VA,0
         DC    XL16'0001020304050607 0A0C0E1012141608'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VA,0
         DC    XL16'020406080A0C0E10 08090A0B0C0D0EFF'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FFFFFFFFFFFFFFFF'   v3

         VRR_C VA,0
         DC    XL16'FFFFFFFFFFFFFFFF 07070707070707F7'   result
         DC    XL16'FEFDFCFBFAF9F8F7 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FEFDFCFBFAF9F8F7'   v3

*Halfword
         VRR_C VA,1
         DC    XL16'0101030305050707 0A0C0E1012141608'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VA,1
         DC    XL16'020406080A0C0E10 09090B0B0D0D0EFF'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FFFFFFFFFFFFFFFF'   v3

         VRR_C VA,1
         DC    XL16'FFFFFFFFFFFFFFFF 08070807080707F7'   result
         DC    XL16'FEFDFCFBFAF9F8F7 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FEFDFCFBFAF9F8F7'   v3

*Word
         VRR_C VA,2
         DC    XL16'0102030305060707 0A0C0E1012141608'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VA,2
         DC    XL16'020406080A0C0E10 090A0B0B0D0E0EFF'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FFFFFFFFFFFFFFFF'   v3

         VRR_C VA,2
         DC    XL16'FFFFFFFFFFFFFFFF 08080807080807F7'   result
         DC    XL16'FEFDFCFBFAF9F8F7 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FEFDFCFBFAF9F8F7'   v3

*Doubleword
         VRR_C VA,3
         DC    XL16'0102030405060707 0A0C0E1012141608'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VA,3
         DC    XL16'020406080A0C0E10 090A0B0C0D0E0EFF'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FFFFFFFFFFFFFFFF'   v3

         VRR_C VA,3
         DC    XL16'FFFFFFFFFFFFFFFF 08080808080807F7'   result
         DC    XL16'FEFDFCFBFAF9F8F7 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FEFDFCFBFAF9F8F7'   v3

*Quadword
         VRR_C VA,4
         DC    XL16'45E866049B0AAA33 5E50126078F4F2F6'   result
         DC    XL16'234873256A194F7D 2F2641230FE5EC7D'   v2
         DC    XL16'229FF2DF30F15AB6 2F29D13D690F0679'   v3

         VRR_C VA,4
         DC    XL16'6E609494AA50147A EB9A297D6B603CC4'   result
         DC    XL16'27C4BF697F002B32 78A31FDE2A6E7226'   v2
         DC    XL16'469BD52B2B4FE948 72F7099F40F1CA9E'   v3

         VRR_C VA,4
         DC    XL16'A12EA3D033F9D386 7E18759E5885D28A'   result
         DC    XL16'6F160AB205F59815 4D5234A5064045A4'   v2
         DC    XL16'3218991E2E043B71 30C640F952458CE6'   v3

*---------------------------------------------------------------------
*   VS     - Vector Subtract
*---------------------------------------------------------------------
*Byte
         VRR_C VS,0
         DC    XL16'FEFDFCFBFAF9F8F7 F8F8F8F8F8F8F808'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VS,0
         DC    XL16'0000000000000000 0A0B0C0D0E0F1001'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FFFFFFFFFFFFFFFF'   v3

         VRR_C VS,0
         DC    XL16'FDFBF9F7F5F3F1EF 0B0D0F1113151709'   result
         DC    XL16'FEFDFCFBFAF9F8F7 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FEFDFCFBFAF9F8F7'   v3

*Halfword
         VRR_C VS,1
         DC    XL16'FEFDFCFBFAF9F8F7 F7F8F7F8F7F8F808'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VS,1
         DC    XL16'0000000000000000 090B0B0D0D0F0F01'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FFFFFFFFFFFFFFFF'   v3

         VRR_C VS,1
         DC    XL16'FDFBF9F7F5F3F1EF 0A0D0E1112151609'   result
         DC    XL16'FEFDFCFBFAF9F8F7 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FEFDFCFBFAF9F8F7'   v3

*Word
         VRR_C VS,2
         DC    XL16'FEFDFCFBFAF9F8F7 F7F7F7F8F7F7F808'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VS,2
         DC    XL16'0000000000000000 090A0B0D0D0E0F01'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FFFFFFFFFFFFFFFF'   v3

         VRR_C VS,2
         DC    XL16'FDFBF9F7F5F3F1EF 0A0C0E1112141609'   result
         DC    XL16'FEFDFCFBFAF9F8F7 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FEFDFCFBFAF9F8F7'   v3

*Doubleword
         VRR_C VS,3
         DC    XL16'FEFDFCFBFAF9F8F7 F7F7F7F7F7F7F808'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VS,3
         DC    XL16'0000000000000000 090A0B0C0D0E0F01'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FFFFFFFFFFFFFFFF'   v3

         VRR_C VS,3
         DC    XL16'FDFBF9F7F5F3F1EF 0A0C0E1012141609'   result
         DC    XL16'FEFDFCFBFAF9F8F7 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FEFDFCFBFAF9F8F7'   v3

*Quadword
         VRR_C VS,4
         DC    XL16'C1E547F1479F6DB9 EE80435D2CCEA68C'   result
         DC    XL16'3A6F0F677D7C5F4B 17669C374BC86A57'   v2
         DC    XL16'7889C77635DCF191 28E658DA1EF9C3CB'   v3

         VRR_C VS,4
         DC    XL16'E735DE52C7237496 F7154F42306B40FD'   result
         DC    XL16'2FBDC758100E357D 24E0AC354223EE33'   v2
         DC    XL16'4887E90548EAC0E6 2DCB5CF311B8AD36'   v3

         VRR_C VS,4
         DC    XL16'ED66719B01D1154D 34E3E6EED8D5204F'   result
         DC    XL16'3FC888C965137198 5F4985333946CE5C'   v2
         DC    XL16'5262172E63425C4B 2A659E446071AE0D'   v3

*---------------------------------------------------------------------
*   VSCBI  - Vector Subtract Compute Borrow Indication
*---------------------------------------------------------------------
*Byte
         VRR_C VSCBI,0
         DC    XL16'0101010101010101 0000000000000001'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VSCBI,0
         DC    XL16'0101010101010101 0000000000000000'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FFFFFFFFFFFFFFFF'   v3

         VRR_C VSCBI,0
         DC    XL16'0101000101010100 0000010000000000'   result
         DC    XL16'FEFD01FBFAF9F807 090AFB0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FEFDF0FBFAF9F8F7'   v3

*Halfword
         VRR_C VSCBI,1
         DC    XL16'0001000100010001 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VSCBI,1
         DC    XL16'0001000100010001 0000000000000001'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FFFFFFFFFFFF00FF'   v3

         VRR_C VSCBI,1
         DC    XL16'0001000100000001 0001000000000000'   result
         DC    XL16'FEFDFC0B0AF9F8F7 B90A0B0C0D0E0F00'   v2
         DC    XL16'010203B4B5060708 0EFDFCFBFAF9F8F7'   v3

*Word
         VRR_C VSCBI,2
         DC    XL16'0000000100000001 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VSCBI,2
         DC    XL16'0000000100000001 0000000000000001'   result
         DC    XL16'9102030405060708 090A0B0CAD0E0F00'   v2
         DC    XL16'0102030405060708 FFFFFFFF0FFFFFFF'   v3

         VRR_C VSCBI,2
         DC    XL16'0000000100000000 0000000000000001'   result
         DC    XL16'FEFDFCFB00F9F8F7 090A0B0CFD0E0F00'   v2
         DC    XL16'0102030405060708 FEFDFCFBFAF9F8F7'   v3

*Doubleword
         VRR_C VSCBI,3
         DC    XL16'0000000000000001 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VSCBI,3
         DC    XL16'0000000000000001 0000000000000001'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 00FFFFFFFFFFFFFF'   v3

         VRR_C VSCBI,3
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'00FDFCFBFAF9F8F7 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 00FDFCFBFAF9F8F7'   v3

*Quadword
         VRR_C VSCBI,4
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VSCBI,4
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 00FFFFFFFFFFFFFF'   v3

         VRR_C VSCBI,4
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'00FDFCFBFAF9F8F7 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 00FDFCFBFAF9F8F7'   v3


*---------------------------------------------------------------------
*   VACC   - Vector Add Compute Carry
*---------------------------------------------------------------------
*Byte
         VRR_C VACC,0
         DC    XL16'0101010101010101 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VACC,0
         DC    XL16'0000000000000000 0101010101010100'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FFFFFFFFFFFFFFFF'   v3

         VRR_C VACC,0
         DC    XL16'0000000000000101 0101010101010100'   result
         DC    XL16'FEFD01FBFAF9F8F7 090AFB0C0D0E0F00'   v2
         DC    XL16'010203040506B7F8 FEFDF0FBFAF9F8F7'   v3

*Halfword
         VRR_C VACC,1
         DC    XL16'0001000100010001 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VACC,1
         DC    XL16'0000000000000000 0001000100010000'   result
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 FFFFFFFFFFFF00FF'   v3

         VRR_C VACC,1
         DC    XL16'0000000000010001 0000000100010001'   result
         DC    XL16'FEFDFC0BFAF9F8F7 B90A0B0C0D0E0F00'   v2
         DC    XL16'010203B4B506F708 0EFDFCFBFAF9F8F7'   v3

*Word
         VRR_C VACC,2
         DC    XL16'0000000100000001 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VACC,2
         DC    XL16'0000000000000001 0000000100000000'   result
         DC    XL16'91020304F5060708 090A0B0CAD0E0F00'   v2
         DC    XL16'01020304F5060708 FFFFFFFF0FFFFFFF'   v3

         VRR_C VACC,2
         DC    XL16'0000000000000000 0000000100000001'   result
         DC    XL16'FEFDFCFB00F9F8F7 090A0B0CFD0E0F00'   v2
         DC    XL16'0102030405060708 FEFDFCFBFAF9F8F7'   v3

*Doubleword
         VRR_C VACC,3
         DC    XL16'0000000000000001 0000000000000000'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VACC,3
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'0102030405060708 F90A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 F0FFFFFFFFFFFFFF'   v3

         VRR_C VACC,3
         DC    XL16'0000000000000001 0000000000000000'   result
         DC    XL16'A0FDFCFBFAF9F8F7 090A0B0C0D0E0F00'   v2
         DC    XL16'A102030405060708 00FDFCFBFAF9F8F7'   v3

*Quadword
         VRR_C VACC,4
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'FFFFFFFFFFFFFFFF 0102030405060708'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F00'   v3

         VRR_C VACC,4
         DC    XL16'0000000000000000 0000000000000000'   result
         DC    XL16'0102030405060708 F90A0B0C0D0E0F00'   v2
         DC    XL16'0102030405060708 F0FFFFFFFFFFFFFF'   v3

         VRR_C VACC,4
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'A0FDFCFBFAF9F8F7 090A0B0C0D0E0F00'   v2
         DC    XL16'A102030405060708 00FDFCFBFAF9F8F7'   v3



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
