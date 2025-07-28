 TITLE 'zvector-e7-10-multiplyAdd'
***********************************************************************
*
*   Zvector E7 instruction tests for VRR-d encoded:
*
*   E7A9 VMALH  - Vector Multiply and Add Logical High
*   E7AA VMAL   - Vector Multiply and Add Low
*   E7AB VMAH   - Vector Multiply and Add High
*   E7AC VMALE  - Vector Multiply and Add Logical Even
*   E7AD VMALO  - Vector Multiply and Add Logical Odd
*   E7AE VMAE   - Vector Multiply and Add Even
*   E7AF VMAO   - Vector Multiply and Add Odd
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
*  This program tests proper functioning of the z/arch E7 VRR-d vector
*  multiply and add (logical high, low, high, logical even,
*  logical odd, even, and odd) instructions.
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*   *Testcase zvector-e7-10-multiplyAdd
*   *
*   *   Zvector E7 instruction tests for VRR-d encoded:
*   *
*   *   E7A9 VMALH  - Vector Multiply and Add Logical High
*   *   E7AA VMAL   - Vector Multiply and Add Low
*   *   E7AB VMAH   - Vector Multiply and Add High
*   *   E7AC VMALE  - Vector Multiply and Add Logical Even
*   *   E7AD VMALO  - Vector Multiply and Add Logical Odd
*   *   E7AE VMAE   - Vector Multiply and Add Even
*   *   E7AF VMAO   - Vector Multiply and Add Odd
*   *
*   *   # ------------------------------------------------------------
*   *   #  This tests only the basic function of the instruction.
*   *   #  Exceptions are NOT tested.
*   *   # ------------------------------------------------------------
*   *
*   mainsize    2
*   numcpu      1
*   sysclear
*   archlvl     z/Arch
*
*   loadcore    "$(testpath)/zvector-e7-10-multiplyAdd.core" 0x0
*
*   diag8cmd    enable    # (needed for messages to Hercules console)
*   runtest     10        # (2 secs if intrinsic used, 10 otherwise!)
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
         LB    R2,m5                get m5 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM5(2),PRT3+14     fill in message with m4 field
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
         DC    C' with m5='
PRTM5    DC    C'xx'
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
M5       DC    HL1'00'        m4 used

OPNAME   DC    CL8' '         E7 name
V2ADDR   DC    A(0)           address of v2 source
V3ADDR   DC    A(0)           address of v3 source
V4ADDR   DC    A(0)           address of v4 source
RELEN    DC    A(0)           RESULT LENGTH
READDR   DC    A(0)           result (expected) address
         DS    FD                gap
V1OUTPUT DS    XL16           V1 Output
         DS    FD                gap

*        test routine will be here (from VRR-d macro)
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
         VRR_D &INST,&M5
.*                               &INST   - VRR-d instruction under test
.*                               &m5     - m5 field

         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
         DC    HL1'&M5'          m5
         DC    CL8'&INST'        instruction name
         DC    A(RE&TNUM+16)     address of v2 source
         DC    A(RE&TNUM+32)     address of v3 source
         DC    A(RE&TNUM+48)     address of v4 source
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

         LGF   R1,V4ADDR         load v4 source
         VL    v24,0(R1)         use v24 to test decoder

         &INST V22,V22,V23,V24,&M5  test instruction (dest is a source)
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
*        E7 VRR-d tests
***********************************************************************
         PRINT DATA
         DS    FD
*
*   E7A9 VMALH  - Vector Multiply and Add Logical High
*   E7AA VMAL   - Vector Multiply and Add Low
*   E7AB VMAH   - Vector Multiply and Add High
*   E7AC VMALE  - Vector Multiply and Add Logical Even
*   E7AD VMALO  - Vector Multiply and Add Logical Odd
*   E7AE VMAE   - Vector Multiply and Add Even
*   E7AF VMAO   - Vector Multiply and Add Odd
*
*        VRR-d instruction, m5
*              followed by
*                 16 byte expected result (V1)
*                 16 byte V2 source
*                 16 byte V3 source
*                 16 byte V4 source
*---------------------------------------------------------------------
*  VMALH  - Vector Multiply and Add Logical High
*---------------------------------------------------------------------
* Byte
         VRR_D VMALH,0
         DC    XL16'FE00000000000002 0000000C000000F4'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMALH,0
         DC    XL16'FE00000100000006 0000000C000000F4'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000000010000002F 0000000000000002'   v4

         VRR_D VMALH,0
         DC    XL16'FF00000000000000 0000000000000001'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALH,0
         DC    XL16'FF00000000000000 0000000000000000'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALH,0
         DC    XL16'FF00000000000000 0000000000000000'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Halfword
         VRR_D VMALH,1
         DC    XL16'FE01000000000000 0000000000000000'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMALH,1
         DC    XL16'FE01000000000000 0000000000000000'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000000010000002F 0000000000000002'   v4

         VRR_D VMALH,1
         DC    XL16'FE05000900190031 0051007A00AA00E2'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALH,1
         DC    XL16'FE040003000A0015 00240037004E0069'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALH,1
         DC    XL16'FE03000000000000 0009000B000D000F'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Word
         VRR_D VMALH,2
         DC    XL16'FE01000000000000 0000000000000000'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMALH,2
         DC    XL16'FE0100FF00000000 0000000000000000'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000000010000002F 0000000000000002'   v4

         VRR_D VMALH,2
         DC    XL16'FE05020700193C6A 0051B52B00AA6E4D'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALH,2
         DC    XL16'FE040104000A1B2F 0024558B004EB018'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALH,2
         DC    XL16'FE03010100000000 0009131E000D1B2A'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Doubleword
         VRR_D VMALH,3
         DC    XL16'FFFFFFFE00032000 0000000000000C77'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMALH,3
         DC    XL16'01010308111F3397 0051B52F8692B4F6'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3
         DC    XL16'FF00000000000000 2000000000000000'   v4

         VRR_D VMALH,3
         DC    XL16'00010003050C1344 0024558DB838C863'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3
         DC    XL16'7FFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v4

         VRR_D VMALH,3
         DC    XL16'000000000000000A 0009131EA8C3DFFE'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3
         DC    XL16'FFFFFFFFFFFFFFFF 7FFFFFFFFFFFFFFF'   v4

* Quadword
         VRR_D VMALH,4
         DC    XL16'FFFFFFFE00032000 FFFCE0736EDDDD83'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMALH,4
         DC    XL16'01010308111F3396 72BF86C3CAFA7484'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3
         DC    XL16'FF00000000000000 2000000000000000'   v4

         VRR_D VMALH,4
         DC    XL16'00010003050C1344 0AD40FD0ABE579A2'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3
         DC    XL16'7FFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v4

         VRR_D VMALH,4
         DC    XL16'0000000000000009 F714203B2D668782'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3
         DC    XL16'FFFFFFFFFFFFFFFF 7FFFFFFFFFFFFFFF'   v4

*---------------------------------------------------------------------
*  VMAL   - Vector Multiply and Add Low
*---------------------------------------------------------------------
* Byte
         VRR_D VMAL,0
         DC    XL16'0100000000000071 0000004000000024'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAL,0
         DC    XL16'01000000000000C0 0000004300000026'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000000010000002F 0000000300000002'   v4

         VRR_D VMAL,0
         DC    XL16'00060C141E2A3848 5A6E849CB6D2F010'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAL,0
         DC    XL16'0004060C0F181C28 2D3C42545B707890'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAL,0
         DC    XL16'0002030405060710 121416181A1C1E30'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Halfword
         VRR_D VMAL,1
         DC    XL16'0000000000000271 00000C400000F424'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAL,1
         DC    XL16'00000100000006C0 00000C430000F426'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000000010000002F 0000000300000002'   v4

         VRR_D VMAL,1
         DC    XL16'FB061B14412A7748 BD6E139C79D2F010'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAL,1
         DC    XL16'FC040D0C20183B28 5E3C8954BC70F790'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAL,1
         DC    XL16'FD02030405060E10 1C142218281C3D30'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Word
         VRR_D VMAL,2
         DC    XL16'0000000000000271 00000C400000F424'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAL,2
         DC    XL16'00000100000006C0 00000C430000F426'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000000010000002F 0000000300000002'   v4

         VRR_D VMAL,2
         DC    XL16'031B1B14A9977748 BE74139C53B0F010'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAL,2
         DC    XL16'FE0D0D0C504B3B28 D8B98954A157F790'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAL,2
         DC    XL16'FB0203040A0C0E10 332B221854493D30'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Doubleword
         VRR_D VMAL,3
         DC    XL16'FFFCE00271000000 96789F9F4FEDCC24'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAL,3
         DC    XL16'69B556ED77F57901 152B55D498D42102'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3
         DC    XL16'0000000000000001 0000000000000001'   v4

         VRR_D VMAL,3
         DC    XL16'26D2FE7090F71480 B47CD8D5FF5B4940'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3
         DC    XL16'2000000000000000 FFFFFFFFFFFFFFFF'   v4

         VRR_D VMAL,3
         DC    XL16'F6142E28323C4921 191C345060616772'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3
         DC    XL16'0000100000000001 1000000000000001'   v4

* Quadword
         VRR_D VMAL,4
         DC    XL16'02D2AEB6AACD4C77 96789F9F4FEDCC24'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAL,4
         DC    XL16'599AFA6A24295657 252B55D498D42102'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3
         DC    XL16'1000000000000001 1000000000000001'   v4

         VRR_D VMAL,4
         DC    XL16'A5AC7D1492F5ADEA B47CD8D5FF5B4940'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v4

         VRR_D VMAL,4
         DC    XL16'FD497B95D40238A4 091C34506061676E'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFD'   v4

*---------------------------------------------------------------------
*  VMAH   - Vector Multiply and Add High
*---------------------------------------------------------------------
* Byte
         VRR_D VMAH,0
         DC    XL16'0000000000000002 0000000C00000000'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAH,0
         DC    XL16'0000000000000006 0000000C00000000'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000000010000002F 0000000300000002'   v4

         VRR_D VMAH,0
         DC    XL16'FF00000000000000 00000000000000FF'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0FF0'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAH,0
         DC    XL16'FF00000000000000 00000000000000FF'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0FF0'   v2
         DC    XL16'0001010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAH,0
         DC    XL16'FF00000000000000 00000000000000FF'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0FF0'   v2
         DC    XL16'0000000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Halfword
         VRR_D VMAH,1
         DC    XL16'0001000000000000 0000000000000000'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAH,1
         DC    XL16'0001000000000000 0000000000000000'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000000010000002F 0000000300000002'   v4

         VRR_D VMAH,1
         DC    XL16'FFFE000900190031 0051007AFF57FF1F'   result
         DC    XL16'FF02030405060708 090A0B0CF30EF110'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAH,1
         DC    XL16'FFFF0003000A0015 00240037FFB2FF97'   result
         DC    XL16'FF02030405060708 090A0B0CF30EF110'   v2
         DC    XL16'0001010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAH,1
         DC    XL16'FFFF000000000000 0009000BFFF3FFF1'   result
         DC    XL16'FF02030405060708 090A0B0CF30EF110'   v2
         DC    XL16'0000000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Word
         VRR_D VMAH,2
         DC    XL16'0001000000000000 0000000000000000'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAH,2
         DC    XL16'0000FFFF00000000 0000000000000000'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000000010000002F 0000000300000002'   v4

         VRR_D VMAH,2
         DC    XL16'FFFF000400193C6A 0051B52BFF5700C5'   result
         DC    XL16'FF02030405060708 090A0B0CF30E0F10'   v2
         DC    XL16'0102030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAH,2
         DC    XL16'FFFFFF01000A1B2F 0024558BFFB1F961'   result
         DC    XL16'FF02030405060708 090A0B0CF30E0F10'   v2
         DC    XL16'0001010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAH,2
         DC    XL16'FFFFFFFF00000000 0009131EFFF30110'   result
         DC    XL16'FF02030405060708 090A0B0CF30E0F10'   v2
         DC    XL16'0000000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Doubleword
         VRR_D VMAH,3
         DC    XL16'0000000000000000 0000000000000C77'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAH,3
         DC    XL16'FFFF00040C192C46 0051B52F8692B4F6'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3
         DC    XL16'0000000000000001 0000000000000001'   v4

         VRR_D VMAH,3
         DC    XL16'FFFFFF010309101C 0024558DB838C862'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3
         DC    XL16'2000000000000000 FFFFFFFFFFFFFFFF'   v4

         VRR_D VMAH,3
         DC    XL16'FFFFFFFFFFFFFFFF 0009131EA8C3DFFE'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3
         DC    XL16'0000100000000001 1000000000000001'   v4

* Quadword
         VRR_D VMAH,4
         DC    XL16'0000000000000001 FFFCE00270FFFF8F'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAH,4
         DC    XL16'FFFF00040C192C45 69B57B4BBDEC6504'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3
         DC    XL16'1000000000000001 1000000000000001'   v4

         VRR_D VMAH,4
         DC    XL16'FFFFFF010309101B 06CF0A94A5DE7262'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v4

         VRR_D VMAH,4
         DC    XL16'FFFFFFFFFFFFFFFE F6131F2C2C658673'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3
         DC    XL16'7FFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v4

*---------------------------------------------------------------------
*  VMALE  - Vector Multiply and Add Logical Even
*---------------------------------------------------------------------
* Byte
         VRR_D VMALE,0
         DC    XL16'FE01000000000000 0000000000000000'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMALE,0
         DC    XL16'FE0300010000002F 0000000300000002'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000200010000002F 0000000300000002'   v4

         VRR_D VMALE,0
         DC    XL16'FD03030D051F0739 095B0B850DB70FF1'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALE,0
         DC    XL16'FD0303070510071D 092E0B430D5C0F79'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALE,0
         DC    XL16'FD03030405060708 09130B170D1B0F1F'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Halfword
         VRR_D VMALE,1
         DC    XL16'FE01000000000000 0000000000000000'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMALE,1
         DC    XL16'FE0300010000002F 0000000300000002'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000200010000002F 0000000300000002'   v4

         VRR_D VMALE,1
         DC    XL16'FD06FF08051F432C 095BBF700DB87BD4'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALE,1
         DC    XL16'FD0600060510221A 092E603E0D5CBE72'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALE,1
         DC    XL16'FD05010405060708 09131E160D1B2A1E'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Word
         VRR_D VMALE,2
         DC    XL16'FE01000000000000 0000000000000C40'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMALE,2
         DC    XL16'FE0301000000012E 0000000300000C42'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000200010000002F 0000000300000002'   v4

         VRR_D VMALE,2
         DC    XL16'FD07050A091F1F18 095BC037C27817A0'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALE,2
         DC    XL16'FD06040804111110 092E6097DCBD8D58'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALE,2
         DC    XL16'FD05040501060708 09131E2A372F261C'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Doubleword
         VRR_D VMALE,3
         DC    XL16'FFFFFFFE00032000 FFFCE00271000000'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMALE,3
         DC    XL16'11010308111F3397 79B556ED77F57901'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3
         DC    XL16'1000000000000001 1000000000000001'   v4

         VRR_D VMALE,3
         DC    XL16'EE010003050C1352 E6D2FE7090F7148E'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3
         DC    XL16'EE0000000000000E E00000000000000E'   v4

         VRR_D VMALE,3
         DC    XL16'FFF0000000000009 F6141E28323C492F'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3
         DC    XL16'FFF0000000000000 000000000000000F'   v4

         VRR_D VMALE,3
         DC    XL16'1009131EA8C3DFFF 191C345060616772'   result
         DC    XL16'090A0B0C0D0E0F7F FF02030405060750'   v2
         DC    XL16'0101010F0101010F 000000000000000A'   v3
         DC    XL16'1000000000000001 1000000000000001'   v4

*---------------------------------------------------------------------
*  VMALO  - Vector Multiply and Add Logical Odd
*---------------------------------------------------------------------
* Byte
         VRR_D VMALO,0
         DC    XL16'0000000000000271 00000C400000F424'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMALO,0
         DC    XL16'00020100000006C0 00000C430000F426'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000200010000002F 0000000300000002'   v4

         VRR_D VMALO,0
         DC    XL16'FF060314052A0748 096E0B9C0DD21010'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALO,0
         DC    XL16'FF04030C05180728 093C0B540D700F90'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALO,0
         DC    XL16'FF02030405060710 09140B180D1C0F30'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Halfword
         VRR_D VMALO,1
         DC    XL16'0000000000000271 00000C400000F424'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMALO,1
         DC    XL16'00020100000006C0 00000C430000F426'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000200010000002F 0000000300000002'   v4

         VRR_D VMALO,1
         DC    XL16'FF0B1B1405377748 0984139C0DF0F010'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALO,1
         DC    XL16'FF050D0C051B3B28 094189540D77F790'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALO,1
         DC    XL16'FF02030405060E10 091522180D1D3D30'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Word
         VRR_D VMALO,2
         DC    XL16'0000000000000271 000000000000F424'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMALO,2
         DC    XL16'00020001000006C0 000000030000F426'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000200010000002F 0000000300000002'   v4

         VRR_D VMALO,2
         DC    XL16'FF1B3F6EA9977748 09B4795953B0F010'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALO,2
         DC    XL16'FF0C1E33504B3B28 0958BB24A157F790'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMALO,2
         DC    XL16'FF0203040A0C0E10 0917263654493D30'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Doubleword
         VRR_D VMALO,3
         DC    XL16'0000000000000C77 96789F9F4FEDCC24'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMALO,3
         DC    XL16'1051B52F8692B4F7 252B55D498D42102'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3
         DC    XL16'1000000000000001 1000000000000001'   v4

         VRR_D VMALO,3
         DC    XL16'F024558DB838C872 A47CD8D5FF5B4950'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3
         DC    XL16'F00000000000000F F00000000000000F'   v4

         VRR_D VMALO,3
         DC    XL16'DD09131EA8C3DFFE D91C345D6D616771'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3
         DC    XL16'DD00000000000000 D000000D0D000000'   v4

         VRR_D VMALO,3
         DC    XL16'FED0000000000009 F6141E28323C492C'   result
         DC    XL16'090A0B0C0D0E0F7F FF02030405060750'   v2
         DC    XL16'0101010F0101010F 000000000000000A'   v3
         DC    XL16'FED0000000000000 000000000000000C'   v4

*---------------------------------------------------------------------
* VMAE   - Vector Multiply and Add Even
*---------------------------------------------------------------------
* Byte
         VRR_D VMAE,0
         DC    XL16'0001000000000000 0000000000000000'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAE,0
         DC    XL16'000300010000002F 0000000300000002'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000200010000002F 0000000300000002'   v4

         VRR_D VMAE,0
         DC    XL16'FF03030D051F0739 095B0B850DB70FF1'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAE,0
         DC    XL16'FF0303070510071D 092E0B430D5C0F79'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAE,0
         DC    XL16'FF03030405060708 09130B170D1B0F1F'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Halfword
         VRR_D VMAE,1
         DC    XL16'0001000000000000 0000000000000000'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAE,1
         DC    XL16'000300010000002F 0000000300000002'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000200010000002F 0000000300000002'   v4

         VRR_D VMAE,1
         DC    XL16'FF02FF08051F432C 095BBF700DB87BD4'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAE,1
         DC    XL16'FF0300060510221A 092E603E0D5CBE72'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAE,1
         DC    XL16'FF03010405060708 09131E160D1B2A1E'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Word
         VRR_D VMAE,2
         DC    XL16'0001000000000000 0000000000000C40'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAE,2
         DC    XL16'000300000000012E 0000000300000C42'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000200010000002F 0000000300000002'   v4

         VRR_D VMAE,2
         DC    XL16'FF02FF02091F1F18 095BC037C27817A0'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAE,2
         DC    XL16'FF03000204111110 092E6097DCBD8D58'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAE,2
         DC    XL16'FF03010101060708 09131E2A372F261C'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Doubleword
         VRR_D VMAE,3
         DC    XL16'0000000000000000 FFFCE00271000000'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAE,3
         DC    XL16'FFFF00040C192C46 69B556ED77F578FF'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v4

         VRR_D VMAE,3
         DC    XL16'FFFFFF010309101D 06D2FE7090F71480'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3
         DC    XL16'0000000000000001 0000000000000000'   v4

         VRR_D VMAE,3
         DC    XL16'FFFFFFFFFFFFFFFF F6141E28323C491C'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFC'   v4

         VRR_D VMAE,3
         DC    XL16'7709131EA8C3DFFE F91C345060616771'   result
         DC    XL16'090A0B0C0D0E0F7F FF02030405060750'   v2
         DC    XL16'0101010F0101010F 000000000000000A'   v3
         DC    XL16'7700000000000000 F000000000000000'   v4

*---------------------------------------------------------------------
* VMAO   - Vector Multiply and Add Odd
*---------------------------------------------------------------------
* Byte
         VRR_D VMAO,0
         DC    XL16'0000000000000271 00000C4000000024'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAO,0
         DC    XL16'00020000000006C0 00000C4300000026'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000200010000002F 0000000300000002'   v4

         VRR_D VMAO,0
         DC    XL16'FF060314052A0748 096E0B9C0DD21010'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAO,0
         DC    XL16'FF04030C05180728 093C0B540D700F90'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAO,0
         DC    XL16'FF02030405060710 09140B180D1C0F30'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Halfword
         VRR_D VMAO,1
         DC    XL16'0000000000000271 00000C400000F424'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAO,1
         DC    XL16'00020100000006C0 00000C430000F426'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000200010000002F 0000000300000002'   v4

         VRR_D VMAO,1
         DC    XL16'FF0B1B1405377748 0984139C0DF0F010'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAO,1
         DC    XL16'FF050D0C051B3B28 094189540D77F790'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAO,1
         DC    XL16'FF02030405060E10 091522180D1D3D30'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Word
         VRR_D VMAO,2
         DC    XL16'0000000000000271 000000000000F424'   result
         DC    XL16'FF00000000000019 00000038000000FA'   v2
         DC    XL16'FF00000000000019 00000038000000FA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAO,2
         DC    XL16'00020001000006C0 000000030000F426'   result
         DC    XL16'FF0000FF00000029 00000038000000FA'   v2
         DC    XL16'FF00000100000029 00000038000000FA'   v3
         DC    XL16'000200010000002F 0000000300000002'   v4

         VRR_D VMAO,2
         DC    XL16'FF1B3F6EA9977748 09B4795953B0F010'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAO,2
         DC    XL16'FF0C1E33504B3B28 0958BB24A157F790'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF01010202030304 0405050606070708'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

         VRR_D VMAO,2
         DC    XL16'FF0203040A0C0E10 0917263654493D30'   result
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF00000000000001 0101010101010102'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

* Doubleword
         VRR_D VMAO,3
         DC    XL16'0000000000000C77 96789F9F4FEDCC24'   result
         DC    XL16'FFFFFFFF00019000 00000038EEEEEEFA'   v2
         DC    XL16'FFFFFFFF00019000 000000380EEEEEFA'   v3
         DC    XL16'0000000000000000 0000000000000000'   v4

         VRR_D VMAO,3
         DC    XL16'7051B52F8692B4F6 152B55D498D421F1'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0102030405060750 090A0B780D0E0F7F'   v3
         DC    XL16'7000000000000000 00000000000000F0'   v4

         VRR_D VMAO,3
         DC    XL16'0024558DB838C862 B47CD8D5FF5B4940'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'0001010202030328 0405053C0607073F'   v3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   v4

         VRR_D VMAO,3
         DC    XL16'7009131EA8C3DFFE F91C345060616771'   result
         DC    XL16'FF02030405060750 090A0B0C0D0E0F7F'   v2
         DC    XL16'000000000000000A 0101010F0101010F'   v3
         DC    XL16'7000000000000000 F000000000000000'   v4

         VRR_D VMAO,3
         DC    XL16'FFFFFFFFFFFFFFFF F6141E28323C491C'   result
         DC    XL16'090A0B0C0D0E0F7F FF02030405060750'   v2
         DC    XL16'0101010F0101010F 000000000000000A'   v3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFC'   v4


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
