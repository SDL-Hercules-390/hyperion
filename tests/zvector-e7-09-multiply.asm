 TITLE 'zvector-e7-09-multiply'
***********************************************************************
*
*   Zvector E7 instruction tests for VRR-c encoded:
*
*   E7A1 VMLH   - Vector Multiply Logical High
*   E7A2 VML    - Vector Multiply Low
*   E7A3 VMH    - Vector Multiply High
*   E7A4 VMLE   - Vector Multiply Logical Even
*   E7A5 VMLO   - Vector Multiply Logical Odd
*   E7A6 VME    - Vector Multiply Even
*   E7A7 VMO    - Vector Multiply Odd
*
*        James Wekel March 2025
*                    July 2025 - Vector-enhancements facility 3 update
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E7 VRR-c vector
*  multiply (logical high, low, high, logical even, logical odd,
*  even, and odd) instructions.
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*   *Testcase zvector-e7-09-multiply
*   *
*   *   Zvector E7 instruction tests for VRR-c encoded:
*   *
*   *   E7A1 VMLH   - Vector Multiply Logical High
*   *   E7A2 VML    - Vector Multiply Low
*   *   E7A3 VMH    - Vector Multiply High
*   *   E7A4 VMLE   - Vector Multiply Logical Even
*   *   E7A5 VMLO   - Vector Multiply Logical Odd
*   *   E7A6 VME    - Vector Multiply Even
*   *   E7A7 VMO    - Vector Multiply Odd
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
*   loadcore    "$(testpath)/zvector-e7-09-multiply.core" 0x0
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
* Is z/Architecture vector facility installed  (bit 198)
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

*   E7A1 VMLH   - Vector Multiply Logical High
*   E7A2 VML    - Vector Multiply Low
*   E7A3 VMH    - Vector Multiply High
*   E7A4 VMLE   - Vector Multiply Logical Even
*   E7A5 VMLO   - Vector Multiply Logical Odd
*   E7A6 VME    - Vector Multiply Even
*   E7A7 VMO    - Vector Multiply Odd

*        VRR-c instruction, m4
*              followed by
*                 16 byte expected result (V1)
*                 16 byte V2 source
*                 16 byte V3 source
*---------------------------------------------------------------------
*  VMLH   - Vector Multiply Logical High
*---------------------------------------------------------------------
* Byte
         VRR_C VMLH,0
         DC    XL16'FE00000000000002 0000000C000000F4'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3

         VRR_C VMLH,0
         DC    XL16'FE00000000000019 00000038000000FA'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3

         VRR_C VMLH,0
         DC    XL16'FE0000000000000C 0000001C000000FB'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF01010202030328 0405053C060707FE'   v3

         VRR_C VMLH,0
         DC    XL16'FE00000000000003 00000007000000FC'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF0000000000000A 0101010F010101FF'   v3


* Halfword
         VRR_C VMLH,1
         DC    XL16'FFFE000000000000 000000000000DF15'   result
         DC    XL16'FFFF000000000019 000000380000EEFA'   v2
         DC    XL16'FFFF000000000019 000000380000EEFA'   v3

         VRR_C VMLH,1
         DC    XL16'FE04000900190035 00510083009D00EF'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3

         VRR_C VMLH,1
         DC    XL16'FE030003000A0017 0024003C00480077'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF01010202030328 0405053C060707FE'   v3

         VRR_C VMLH,1
         DC    XL16'FE02000000000000 0009000C000C001D'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF0000000000000A 0101010F010101FF'   v3

* Word
         VRR_C VMLH,2
         DC    XL16'FFFFFFFE00000002 00000000DF01235A'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v3

         VRR_C VMLH,2
         DC    XL16'FE05020600193C6D 0051B52B00AA6E58'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v3

         VRR_C VMLH,2
         DC    XL16'FE040103000A1B30 0024558D004EB01D'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'FF01010202030328 0405053C0607073F'   v3

         VRR_C VMLH,2
         DC    XL16'FE03010000000000 0009131E000D1B2B'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'FF0000000000000A 0101010F0101010F'   v3

* Doubleword
         VRR_C VMLH,3
         DC    XL16'FFFFFFFE00032000 0000000000000C77'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VMLH,3
         DC    XL16'01010308111F3396 0051B52F8692B4F6'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VMLH,3
         DC    XL16'00010003050C1344 0024558DB838C862'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VMLH,3
         DC    XL16'0000000000000009 0009131EA8C3DFFE'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

* Quadword
         VRR_C VMLH,4
         DC    XL16'FFFFFFFE00032000 FFFCE0736EDDDD83'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VMLH,4
         DC    XL16'01010308111F3396 72BF86C3CAFA7483'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VMLH,4
         DC    XL16'00010003050C1344 0AD40FD0ABE579A1'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VMLH,4
         DC    XL16'0000000000000009 F714203B2D668781'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

*---------------------------------------------------------------------
*  VML    - Vector Multiply Low
*---------------------------------------------------------------------
* Byte
         VRR_C VML,0
         DC    XL16'0100000000000071 0000004000000024'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3

         VRR_C VML,0
         DC    XL16'0104091019243140 51647990A9C4E100'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VML,0
         DC    XL16'010203080A121520 243237484E626980'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3

         VRR_C VML,0
         DC    XL16'0100000000000008 090A0B0C0D0E0F20'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3

* Halfword
         VRR_C VML,1
         DC    XL16'0001000000000271 00000C400000CC24'   result
         DC    XL16'FFFF000000000019 000000380000EEFA'   v2
         DC    XL16'FFFF000000000019 000000380000EEFA'   v3

         VRR_C VML,1
         DC    XL16'FC0418103C247040 B46408906CC4E100'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VML,1
         DC    XL16'FD020A081B123420 55327E48AF62E880'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3

         VRR_C VML,1
         DC    XL16'FE00000000000708 130A170C1B0E2E20'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3

* Word
         VRR_C VML,2
         DC    XL16'0000000171000000 00000C400FEDCC24'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v3

         VRR_C VML,2
         DC    XL16'04191810A4917040 B56A089046A2E100'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3

         VRR_C VML,2
         DC    XL16'FF0B0A084B453420 CFAF7E489449E880'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3

         VRR_C VML,2
         DC    XL16'FC00000005060708 2A21170C473B2E20'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3

* Doubleword
         VRR_C VML,3
         DC    XL16'FFFCE00271000000 96789F9F4FEDCC24'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VML,3
         DC    XL16'69B556ED77F57900 152B55D498D42101'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VML,3
         DC    XL16'06D2FE7090F71480 B47CD8D5FF5B4941'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VML,3
         DC    XL16'F6141E28323C4920 091C345060616771'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

* Quadword
         VRR_C VML,4
         DC    XL16'02D2AEB6AACD4C77 96789F9F4FEDCC24'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VML,4
         DC    XL16'499AFA6A24295656 152B55D498D42101'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VML,4
         DC    XL16'A5AC7D1492F5ADEA B47CD8D5FF5B4941'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VML,4
         DC    XL16'FD497B95D40238A4 091C345060616771'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

*---------------------------------------------------------------------
*  VMH    - Vector Multiply High
*---------------------------------------------------------------------
* Byte
         VRR_C VMH,0
         DC    XL16'0000000000000002 0000000C00000000'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3

         VRR_C VMH,0
         DC    XL16'FF00000000000019 000000050000003F'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VMH,0
         DC    XL16'000000000000000C 000000020000001F'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VMH,0
         DC    XL16'0000000000000003 0000000000000007'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

* Halfword
         VRR_C VMH,1
         DC    XL16'0000000000000000 0000000000000121'   result
         DC    XL16'FFFF000000000019 000000380000EEFA'   v2
         DC    XL16'FFFF000000000019 000000380000EEFA'   v3

         VRR_C VMH,1
         DC    XL16'FFFF000900190035 0051007E00AA00F0'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VMH,1
         DC    XL16'FFFF0003000A0017 00240039004E0070'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VMH,1
         DC    XL16'0000000000000000 0009000B000D0010'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

* Word
         VRR_C VMH,2
         DC    XL16'0000000000000002 00000000FF012345'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VMH,2
         DC    XL16'FFFF000400193C6D 0051B52F00AA6E58'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VMH,2
         DC    XL16'FFFFFF01000A1B30 0024558D004EB01D'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VMH,2
         DC    XL16'0000000000000000 0009131E000D1B2B'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

* Doubleword
         VRR_C VMH,3
         DC    XL16'0000000000000000 0000000000000C77'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VMH,3
         DC    XL16'FFFF00040C192C46 0051B52F8692B4F6'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VMH,3
         DC    XL16'FFFFFF010309101C 0024558DB838C862'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VMH,3
         DC    XL16'FFFFFFFFFFFFFFFF 0009131EA8C3DFFE'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

* Quadword
         VRR_C VMH,4
         DC    XL16'0000000000000001 FFFCE00270FFFF8F'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VMH,4
         DC    XL16'FFFF00040C192C45 69B57B4BBDEC6504'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VMH,4
         DC    XL16'FFFFFF010309101B 06CF0A94A5DE7262'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VMH,4
         DC    XL16'FFFFFFFFFFFFFFFE F6131F2C2C658672'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

*---------------------------------------------------------------------
*  VMLE   - Vector Multiply Logical Even
*---------------------------------------------------------------------
* Byte
         VRR_C VMLE,0
         DC    XL16'FE01000000000000 0C40000000000000'   result
         DC    XL16'FF00000000000019 38000000000000FA'   v2
         DC    XL16'FF00000000000019 38000000000000FA'   v3

         VRR_C VMLE,0
         DC    XL16'FE01000900190031 00510079009C00D2'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3

         VRR_C VMLE,0
         DC    XL16'FE010003000A0015 0024003700480062'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF01010202030328 0405053C060707FE'   v3

         VRR_C VMLE,0
         DC    XL16'FE01000000000000 0009000B000C000E'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF0000000000000A 0101010F010101FF'   v3

* Halfword
         VRR_C VMLE,1
         DC    XL16'FFFE000100000000 00000C4008000000'   result
         DC    XL16'FFFF000000000019 003800001000EEFA'   v2
         DC    XL16'FFFF000000000019 003800038000EEFA'   v3

         VRR_C VMLE,1
         DC    XL16'FE04FC0400193C24 0051B464009D51B6'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3

         VRR_C VMLE,1
         DC    XL16'FE03FD02000A1B12 002455320048A25B'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF01010202030328 0405053C060707FE'   v3

         VRR_C VMLE,1
         DC    XL16'FE02FE0000000000 0009130A000C190D'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF0000000000000A 0101010F010101FF'   v3

* Word
         VRR_C VMLE,2
         DC    XL16'FFFFFFFE00000001 0000000000000C40'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VMLE,2
         DC    XL16'FE05020604191810 0051B52F85A6B1A0'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0F7F'   v3

         VRR_C VMLE,2
         DC    XL16'FE040103FF0B0A08 0024558DB7CDD2D0'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'FF01010202030328 0405053C0607073F'   v3

         VRR_C VMLE,2
         DC    XL16'FE030100FC000000 0009131EA8ADB1B4'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'FF0000000000000A 0101010F0101010F'   v3

* Doubleword
         VRR_C VMLE,3
         DC    XL16'FFFFFFFE00032000 FFFCE00271000000'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VMLE,3
         DC    XL16'01010308111F3396 69B556ED77F57900'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VMLE,3
         DC    XL16'00010003050C1344 06D2FE7090F71480'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VMLE,3
         DC    XL16'0000000000000009 F6141E28323C4920'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

         VRR_C VMLE,3
         DC    XL16'0009131EA8C3DFFE 091C345060616771'   result
         DC    XL16'090A0B0C0D0E0F7F FF02030405060750'   v2
         DC    XL16'0101010F0101010F 000000000000000A'   v3

*---------------------------------------------------------------------
*  VMLO   - Vector Multiply Logical Odd
*---------------------------------------------------------------------
* Byte
         VRR_C VMLO,0
         DC    XL16'0000000000000271 000000000000F424'   result
         DC    XL16'FF00000000000019 38000000000000FA'   v2
         DC    XL16'FF00000000000019 38000000000000FA'   v3

         VRR_C VMLO,0
         DC    XL16'0004001000241900 0064384000B6FA09'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3

         VRR_C VMLO,0
         DC    XL16'0002000800120C80 00321C20005BFB06'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF01010202030328 0405053C060707FE'   v3

         VRR_C VMLO,0
         DC    XL16'0000000000000320 000A0708000DFC03'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF0000000000000A 0101010F010101FF'   v3

* Halfword
         VRR_C VMLO,1
         DC    XL16'0000000000000271 00000000DF15CC24'   result
         DC    XL16'FFFF000000000019 003800001000EEFA'   v2
         DC    XL16'FFFF000000000019 003800038000EEFA'   v3

         VRR_C VMLO,1
         DC    XL16'0009181000357900 0083884000EFA309'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3

         VRR_C VMLO,1
         DC    XL16'00030A0800171480 003C08200077CA06'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF01010202030328 0405053C060707FE'   v3

         VRR_C VMLO,1
         DC    XL16'0000000000004920 000C2408001DEB03'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF0000000000000A 0101010F010101FF'   v3

* Word
         VRR_C VMLO,2
         DC    XL16'0000000271000000 0DF0123F4FEDCC24'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VMLO,2
         DC    XL16'00193C6D77F57900 00AA6E5898D42101'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0F7F'   v3

         VRR_C VMLO,2
         DC    XL16'000A1B3090F71480 004EB01DFF5B4941'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'FF01010202030328 0405053C0607073F'   v3

         VRR_C VMLO,2
         DC    XL16'00000000323C4920 000D1B2B60616771'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'FF0000000000000A 0101010F0101010F'   v3

* Doubleword
         VRR_C VMLO,3
         DC    XL16'0000000000000C77 96789F9F4FEDCC24'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VMLO,3
         DC    XL16'0051B52F8692B4F6 152B55D498D42101'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VMLO,3
         DC    XL16'0024558DB838C862 B47CD8D5FF5B4941'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VMLO,3
         DC    XL16'0009131EA8C3DFFE 091C345060616771'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

         VRR_C VMLO,3
         DC    XL16'0000000000000009 F6141E28323C4920'   result
         DC    XL16'090A0B0C0D0E0F7F FF02030405060750'   v2
         DC    XL16'0101010F0101010F 000000000000000A'   v3

*---------------------------------------------------------------------
*  VME    - Vector Multiply Even
*---------------------------------------------------------------------
* Byte
         VRR_C VME,0
         DC    XL16'0001000000000000 0C40000000000000'   result
         DC    XL16'FF00000000000019 38000000000000FA'   v2
         DC    XL16'FF00000000000019 38000000000000FA'   v3

         VRR_C VME,0
         DC    XL16'0001000900190031 00510079009C00D2'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3

         VRR_C VME,0
         DC    XL16'00010003000A0015 0024003700480062'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF01010202030328 0405053C060707FE'   v3

         VRR_C VME,0
         DC    XL16'0001000000000000 0009000B000C000E'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF0000000000000A 0101010F010101FF'   v3

* Halfword
         VRR_C VME,1
         DC    XL16'0000000100000000 00000C40F8000000'   result
         DC    XL16'FFFF000000000019 003800001000EEFA'   v2
         DC    XL16'FFFF000000000019 003800038000EEFA'   v3

         VRR_C VME,1
         DC    XL16'0000FC0400193C24 0051B464009D51B6'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3

         VRR_C VME,1
         DC    XL16'0000FD02000A1B12 002455320048A25B'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF01010202030328 0405053C060707FE'   v3

         VRR_C VME,1
         DC    XL16'0000FE0000000000 0009130A000C190D'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF0000000000000A 0101010F010101FF'   v3

* Word
         VRR_C VME,2
         DC    XL16'0000000000000001 0000000000000C40'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VME,2
         DC    XL16'FFFF00040C191810 0051B52F85A6B1A0'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VME,2
         DC    XL16'FFFFFF01030B0A08 0024558DB7CDD2D0'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VME,2
         DC    XL16'0000000000000000 0009131EA8ADB1B4'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

* Doubleword
         VRR_C VME,3
         DC    XL16'0000000000000000 FFFCE00271000000'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VME,3
         DC    XL16'FFFF00040C192C46 69B556ED77F57900'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VME,3
         DC    XL16'FFFFFF010309101C 06D2FE7090F71480'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VME,3
         DC    XL16'FFFFFFFFFFFFFFFF F6141E28323C4920'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

         VRR_C VME,3
         DC    XL16'0009131EA8C3DFFE 091C345060616771'   result
         DC    XL16'090A0B0C0D0E0F7F FF02030405060750'   v2
         DC    XL16'0101010F0101010F 000000000000000A'   v3

*---------------------------------------------------------------------
*  VMO    - Vector Multiply Odd
*---------------------------------------------------------------------
* Byte
         VRR_C VMO,0
         DC    XL16'0000000000000271 0000000000000024'   result
         DC    XL16'FF00000000000019 38000000000000FA'   v2
         DC    XL16'FF00000000000019 38000000000000FA'   v3

         VRR_C VMO,0
         DC    XL16'0004001000241900 0064384000B60009'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3

         VRR_C VMO,0
         DC    XL16'0002000800120C80 00321C20005B0006'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF01010202030328 0405053C060707FE'   v3

         VRR_C VMO,0
         DC    XL16'0000000000000320 000A0708000D0003'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF0000000000000A 0101010F010101FF'   v3

* Halfword
         VRR_C VMO,1
         DC    XL16'0000000000000271 000000000121CC24'   result
         DC    XL16'FFFF000000000019 003800001000EEFA'   v2
         DC    XL16'FFFF000000000019 003800038000EEFA'   v3

         VRR_C VMO,1
         DC    XL16'0009181000357900 0083884000EFA309'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3

         VRR_C VMO,1
         DC    XL16'00030A0800171480 003C08200077CA06'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF01010202030328 0405053C060707FE'   v3

         VRR_C VMO,1
         DC    XL16'0000000000004920 000C2408001DEB03'   result
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF0000000000000A 0101010F010101FF'   v3

* Word
         VRR_C VMO,2
         DC    XL16'0000000271000000 FF0123454FEDCC24'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VMO,2
         DC    XL16'00193C6D77F57900 00AA6E5898D42101'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0F7F'   v3

         VRR_C VMO,2
         DC    XL16'000A1B3090F71480 004EB01DFF5B4941'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'FF01010202030328 0405053C0607073F'   v3

         VRR_C VMO,2
         DC    XL16'00000000323C4920 000D1B2B60616771'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'FF0000000000000A 0101010F0101010F'   v3

* Doubleword
         VRR_C VMO,3
         DC    XL16'0000000000000C77 96789F9F4FEDCC24'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3

         VRR_C VMO,3
         DC    XL16'0051B52F8692B4F6 152B55D498D42101'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3

         VRR_C VMO,3
         DC    XL16'0024558DB838C862 B47CD8D5FF5B4941'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3

         VRR_C VMO,3
         DC    XL16'0009131EA8C3DFFE 091C345060616771'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3

         VRR_C VMO,3
         DC    XL16'FFFFFFFFFFFFFFFF F6141E28323C4920'   result
         DC    XL16'090A0B0C0D0E0F7F FF02030405060750'   v2
         DC    XL16'0101010F0101010F 000000000000000A'   v3


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
