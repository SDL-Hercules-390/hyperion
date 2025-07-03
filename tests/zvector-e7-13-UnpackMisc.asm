 TITLE 'zvector-e7-13-UnpackMisc'
***********************************************************************
*
*   Zvector E7 instruction tests for VRR-a encoded:
*
*   E7D4 VUPLL  - Vector Unpack Logical Low
*   E7D5 VUPLH  - Vector Unpack Logical High
*   E7D6 VUPL   - Vector Unpack Low
*   E7D7 VUPH   - Vector Unpack High
*
*   and
*
*   E75F VSEG   - Vector Sign Extend To Doubleword
*   E7DE VLC    - Vector Load Complement
*   E7DF VLP    - Vector Load Positive
*
*        James Wekel January 2025
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E7 VRR-a
*  unpack instructions (Logical Low, Logical High, Low and High) and
*  miscellaneous instructions (Sign Extend To Doubleword,
*  Load Complement, and Load Positive).
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*   *Testcase zvector-e7-13-UnpackMisc
*   *
*   *   Zvector E7 instruction tests for VRR-a encoded:
*   *
*   *   E7D4 VUPLL  - Vector Unpack Logical Low
*   *   E7D5 VUPLH  - Vector Unpack Logical High
*   *   E7D6 VUPL   - Vector Unpack Low
*   *   E7D7 VUPH   - Vector Unpack High
*   *
*   *   E75F VSEG   - Vector Sign Extend To Doubleword
*   *   E7DE VLC    - Vector Load Complement
*   *   E7DF VLP    - Vector Load Positive
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
*   loadcore    "$(testpath)/zvector-e7-13-UnpackMisc.core" 0x0
*
*   diag8cmd    enable    # (needed for messages to Hercules console)
*   runtest     5         #
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
         USING BEGIN,R8         FIRST Base Register
         USING BEGIN+4096,R9    SECOND Base Register
         USING BEGIN+8192,R10   THIRD Base Register
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

         L     R12,=A(E7TESTS)         get table of test addresses

NEXTE7   EQU   *
         L     R5,0(0,R12)             get test address
         LTR   R5,R5                      have a test?
         BZ    ENDTEST                       done?

         USING E7TEST,R5

         LH    R0,TNUM                 save current test number
         ST    R0,TESTING              for easy reference

         VL    V1,V1FUDGE
         L     R11,TSUB                get address of test routine
         BALR  R11,R11                 do test

         LGF   R1,READDR               get address of expected result
         CLC   V1OUTPUT,0(R1)          valid?
         BNE   FAILMSG                    no, issue failed message

         LA    R12,4(0,R12)            next test address
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
         L     R0,=F'1'                set failed test indicator
         ST    R0,FAILED

         LA    R12,4(0,R12)            next test address
         B     NEXTE7
                                                                SPACE 2
***********************************************************************
* end of testing; set ending psw
***********************************************************************
ENDTEST  EQU   *
         L     R1,FAILED               did a test fail?
         LTR   R1,R1
         BZ    EOJ                     No, exit
         B     FAILTEST                Yes, exit with BAD PSW
                                                               EJECT
***********************************************************************
*        RPTERROR                 Report instruction test in error
***********************************************************************
                                                               SPACE
RPTERROR ST    R15,RPTSAVE             Save return address
         ST    R5,RPTSVR5              Save R5
*
         LH    R2,TNUM                 get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTNUM(3),PRT3+13       fill in message with test #

         MVC   PRTNAME,OPNAME          fill in message with instruction
*
         LB    R2,M3                   get M3 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM3(2),PRT3+14        fill in message with m3 field
                                                               SPACE
*
*        Use Hercules Diagnose for Message to console
*
         STM   R0,R2,RPTDWSAV          save regs used by MSG
         LA    R0,PRTLNG               message length
         LA    R1,PRTLINE              messagfe address
         BAL   R2,MSG                  call Hercules to display MSG
         LM    R0,R2,RPTDWSAV          restore regs
                                                               SPACE 2
         L     R5,RPTSVR5              Restore R5
         L     R15,RPTSAVE             Restore return address
         BR    R15                     Return to caller
                                                               SPACE
RPTSAVE  DC    F'0'                    R15 save area
RPTSVR5  DC    F'0'                    R5 save area
                                                               SPACE
RPTDWSAV DC    2D'0'                   R0-R2 save area for MSG call
                                                               EJECT
***********************************************************************
*        Issue HERCULES MESSAGE pointed to by R1, length in R0
*              R2 = return address
***********************************************************************
                                                                SPACE
MSG      CH    R0,=H'0'               Do we even HAVE a message?
         BNHR  R2                     No, ignore
                                                                SPACE
         STM   R0,R2,MSGSAVE           Save registers
                                                                SPACE
         CH    R0,=AL2(L'MSGMSG)       Message length within limits?
         BNH   MSGOK                   Yes, continue
         LA    R0,L'MSGMSG             No, set to maximum
                                                                SPACE
MSGOK    LR    R2,R0                   Copy length to work register
         BCTR  R2,0                    Minus-1 for execute
         EX    R2,MSGMVC               Copy message to O/P buffer
                                                                SPACE
         LA    R2,1+L'MSGCMD(,R2)      Calculate true command length
         LA    R1,MSGCMD               Point to true command
                                                                SPACE
         DC    X'83',X'12',X'0008'     Issue Hercules Diagnose X'008'
         BZ    MSGRET                  Return if successful

         LTR   R2,R2                   Is Diag8 Ry (R2) 0?
         BZ    MSGRET                    an error occurred but coninue

         DC    H'0'                    CRASH for debugging purposes
                                                                SPACE
MSGRET   LM    R0,R2,MSGSAVE           Restore registers
         BR    R2                      Return to caller
                                                                SPACE 6
MSGSAVE  DC    3F'0'                   Registers save area
MSGMVC   MVC   MSGMSG(0),0(R1)         Executed instruction
                                                                SPACE 2
MSGCMD   DC    C'MSGNOH * '            *** HERCULES MESSAGE COMMAND ***
MSGMSG   DC    CL95' '                 The message text to be displayed

                                                                EJECT
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 5
EOJPSW   DC    0D'0',X'0002000180000000',AD(0)
                                                                SPACE
EOJ      LPSWE EOJPSW                  Normal completion
                                                                SPACE 5
FAILPSW  DC    0D'0',X'0002000180000000',AD(X'BAD')
                                                                SPACE
FAILTEST LPSWE FAILPSW                 Abnormal termination
                                                                SPACE 7
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE 2
CTLR0    DS    F                       CR0
         DS    F
                                                                SPACE
         LTORG ,                       Literals pool

*        some constants

K        EQU   1024                    One KB
PAGE     EQU   (4*K)                   Size of one page
K64      EQU   (64*K)                  64 KB
MB       EQU   (K*K)                    1 MB

REG2PATT EQU   X'AABBCCDD'             Polluted Register pattern
REG2LOW  EQU         X'DD'             (last byte above)
                                                                EJECT
*======================================================================
*
*  NOTE: start data on an address that is easy to display
*        within Hercules
*
*======================================================================

         ORG   ZVE7TST+X'1000'
FAILED   DC    F'0'                    some test failed?
TESTING  DC    F'0'                    current test number
                                                               SPACE 2
*
*        failed message and associated editting
*
PRTLINE  DC    C'         Test # '
PRTNUM   DC    C'xxx'
         DC    c' failed for instruction '
PRTNAME  DC    CL8'xxxxxxxx'
         DC    C' with M3='
PRTM3    DC    C'xx'
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
TSUB     DC    A(0)                    pointer  to test
TNUM     DC    H'00'                   Test Number
         DC    X'00'
M3       DC    HL1'00'                 m4 used

OPNAME   DC    CL8' '                  E6 name
V2ADDR   DC    A(0)                    address of v2 source
RELEN    DC    A(0)                    RESULT LENGTH
READDR   DC    A(0)                    result (expected) address
         DS    FD                         gap
V1OUTPUT DS    XL16                    V1 Output
         DS    FD                         gap

*        test routine will be here (from VRR-a macro)
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
         VRR_A &INST,&M3
.*                               &INST   - VRR-a instruction under test
.*                               &M3     - m3 field

         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5               base for test data and test routine

T&TNUM   DC    A(X&TNUM)          address of test routine
         DC    H'&TNUM'           test number
         DC    X'00'
         DC    HL1'&M3'           M3
         DC    CL8'&INST'         instruction name
         DC    A(RE&TNUM+16)      address of v2 source
         DC    A(16)              result length
REA&TNUM DC    A(RE&TNUM)         result address
         DS    FD                 gap
V1O&TNUM DS    XL16               V1 output
         DS    FD                 gap
.*
*
X&TNUM   DS    0F
         LGF   R1,V2ADDR          load v2 source
         VL    v22,0(R1)          use v22 to test decoder

         &INST V22,V22,&M3        test instruction (dest is a source)
         VST   V22,V1O&TNUM       save v1 output

         BR    R11                return

RE&TNUM  DC    0F                 xl16 expected result

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
         DC    A(0)                    END OF TABLE
         DC    A(0)                    END OF TABLE
.*
         MEND

                                                                EJECT
***********************************************************************
*        E7 VRR-a tests
***********************************************************************
         PRINT DATA
*
*   E7D4 VUPLL  - Vector Unpack Logical Low
*   E7D5 VUPLH  - Vector Unpack Logical High
*   E7D6 VUPL   - Vector Unpack Low
*   E7D7 VUPH   - Vector Unpack High
*
*   and
*
*   E75F VSEG   - Vector Sign Extend To Doubleword
*   E7DE VLC    - Vector Load Complement
*   E7DF VLP    - Vector Load Positive
*
*        VRR-a instruction, M3
*              followed by
*                 16 byte expected result (V1)
*                 16 byte V2 source
*---------------------------------------------------------------------
* VUPLL  - Vector Unpack Logical Low
*---------------------------------------------------------------------
* Byte
         VRR_A VUPLL,0
         DC    XL16'0011002200330044 0055006600770088'   result
         DC    XL16'ABABABABABABABAB 1122334455667788'   v2

         VRR_A VUPLL,0
         DC    XL16'00F1000200C30004 0005000600D700F8'   result
         DC    XL16'ABABABABABABABAB F102C3040506D7F8'   v2

         VRR_A VUPLL,0
         DC    XL16'00F100D200C300F4 00F500C600D700F8'   result
         DC    XL16'ABABABABABABABAB F1D2C3F4F5C6D7F8'   v2

* Halfword
         VRR_A VUPLL,1
         DC    XL16'0000112200003344 0000556600007788'   result
         DC    XL16'ABABABABABABABAB 1122334455667788'   v2

         VRR_A VUPLL,1
         DC    XL16'0000F1020000C304 000005060000D7F8'   result
         DC    XL16'ABABABABABABABAB F102C3040506D7F8'   v2

         VRR_A VUPLL,1
         DC    XL16'0000F1D20000C3F4 0000F5C60000D7F8'   result
         DC    XL16'ABABABABABABABAB F1D2C3F4F5C6D7F8'   v2

* Word
         VRR_A VUPLL,2
         DC    XL16'0000000011223344 0000000055667788'   result
         DC    XL16'ABABABABABABABAB 1122334455667788'   v2

         VRR_A VUPLL,2
         DC    XL16'00000000F102C304 000000000506D7F8'   result
         DC    XL16'ABABABABABABABAB F102C3040506D7F8'   v2

         VRR_A VUPLL,2
         DC    XL16'00000000F1D2C3F4 00000000F5C6D7F8'   result
         DC    XL16'ABABABABABABABAB F1D2C3F4F5C6D7F8'   v2

*---------------------------------------------------------------------
* VUPLH  - Vector Unpack Logical High
*---------------------------------------------------------------------
* Byte
         VRR_A VUPLH,0
         DC    XL16'0011002200330044 0055006600770088'   result
         DC    XL16'1122334455667788 ABABABABABABABAB'   v2

         VRR_A VUPLH,0
         DC    XL16'00F1000200C30004 0005000600D700F8'   result
         DC    XL16'F102C3040506D7F8 ABABABABABABABAB'   v2

         VRR_A VUPLH,0
         DC    XL16'00F100D200C300F4 00F500C600D700F8'   result
         DC    XL16'F1D2C3F4F5C6D7F8 ABABABABABABABAB'   v2

* Halfword
         VRR_A VUPLH,1
         DC    XL16'0000112200003344 0000556600007788'   result
         DC    XL16'1122334455667788 ABABABABABABABAB'   v2

         VRR_A VUPLH,1
         DC    XL16'0000F1020000C304 000005060000D7F8'   result
         DC    XL16'F102C3040506D7F8 ABABABABABABABAB'   v2

         VRR_A VUPLH,1
         DC    XL16'0000F1D20000C3F4 0000F5C60000D7F8'   result
         DC    XL16'F1D2C3F4F5C6D7F8 ABABABABABABABAB'   v2

* Word
         VRR_A VUPLH,2
         DC    XL16'0000000011223344 0000000055667788'   result
         DC    XL16'1122334455667788 ABABABABABABABAB'   v2

         VRR_A VUPLH,2
         DC    XL16'00000000F102C304 000000000506D7F8'   result
         DC    XL16'F102C3040506D7F8 ABABABABABABABAB'   v2

         VRR_A VUPLH,2
         DC    XL16'00000000F1D2C3F4 00000000F5C6D7F8'   result
         DC    XL16'F1D2C3F4F5C6D7F8 ABABABABABABABAB'   v2

*---------------------------------------------------------------------
* VUPL   - Vector Unpack Low
*---------------------------------------------------------------------
* Byte
         VRR_A VUPL,0
         DC    XL16'0011002200330044 005500660077FF88'   result
         DC    XL16'ABABABABABABABAB 1122334455667788'   v2

         VRR_A VUPL,0
         DC    XL16'FFF10002FFC30004 00050006FFD7FFF8'   result
         DC    XL16'ABABABABABABABAB F102C3040506D7F8'   v2

         VRR_A VUPL,0
         DC    XL16'FFF1FFD2FFC3FFF4 FFF5FFC6FFD7FFF8'   result
         DC    XL16'ABABABABABABABAB F1D2C3F4F5C6D7F8'   v2

* Halfword
         VRR_A VUPL,1
         DC    XL16'0000112200003344 0000556600007788'   result
         DC    XL16'ABABABABABABABAB 1122334455667788'   v2

         VRR_A VUPL,1
         DC    XL16'FFFFF102FFFFC304 00000506FFFFD7F8'   result
         DC    XL16'ABABABABABABABAB F102C3040506D7F8'   v2

         VRR_A VUPL,1
         DC    XL16'FFFFF1D2FFFFC3F4 FFFFF5C6FFFFD7F8'   result
         DC    XL16'ABABABABABABABAB F1D2C3F4F5C6D7F8'   v2

* Word
         VRR_A VUPL,2
         DC    XL16'0000000011223344 0000000055667788'   result
         DC    XL16'ABABABABABABABAB 1122334455667788'   v2

         VRR_A VUPL,2
         DC    XL16'FFFFFFFFF102C304 000000000506D7F8'   result
         DC    XL16'ABABABABABABABAB F102C3040506D7F8'   v2

         VRR_A VUPL,2
         DC    XL16'FFFFFFFFF1D2C3F4 FFFFFFFFF5C6D7F8'   result
         DC    XL16'ABABABABABABABAB F1D2C3F4F5C6D7F8'   v2

*---------------------------------------------------------------------
* VUPH   - Vector Unpack High
*---------------------------------------------------------------------
* Byte
         VRR_A VUPH,0
         DC    XL16'0011002200330044 005500660077FF88'   result
         DC    XL16'1122334455667788 ABABABABABABABAB'   v2

         VRR_A VUPH,0
         DC    XL16'FFF10002FFC30004 00050006FFD7FFF8'   result
         DC    XL16'F102C3040506D7F8 ABABABABABABABAB'   v2

         VRR_A VUPH,0
         DC    XL16'FFF1FFD2FFC3FFF4 FFF5FFC6FFD7FFF8'   result
         DC    XL16'F1D2C3F4F5C6D7F8 ABABABABABABABAB'   v2

* Halfword
         VRR_A VUPH,1
         DC    XL16'0000112200003344 0000556600007788'   result
         DC    XL16'1122334455667788 ABABABABABABABAB'   v2

         VRR_A VUPH,1
         DC    XL16'FFFFF102FFFFC304 00000506FFFFD7F8'   result
         DC    XL16'F102C3040506D7F8 ABABABABABABABAB'   v2

         VRR_A VUPH,1
         DC    XL16'FFFFF1D2FFFFC3F4 FFFFF5C6FFFFD7F8'   result
         DC    XL16'F1D2C3F4F5C6D7F8 ABABABABABABABAB'   v2

* Word
         VRR_A VUPH,2
         DC    XL16'0000000011223344 0000000055667788'   result
         DC    XL16'1122334455667788 ABABABABABABABAB'   v2

         VRR_A VUPH,2
         DC    XL16'FFFFFFFFF102C304 000000000506D7F8'   result
         DC    XL16'F102C3040506D7F8 ABABABABABABABAB'   v2

         VRR_A VUPH,2
         DC    XL16'FFFFFFFFF1D2C3F4 FFFFFFFFF5C6D7F8'   result
         DC    XL16'F1D2C3F4F5C6D7F8 ABABABABABABABAB'   v2

*---------------------------------------------------------------------
* VSEG   - Vector Sign Extend To Doubleword
*---------------------------------------------------------------------
* Byte
         VRR_A VSEG,0
         DC    XL16'000000000000000B 0000000000000018'   result
         DC    XL16'ABABABABABABAB0B 1122334455667718'   v2

         VRR_A VSEG,0
         DC    XL16'000000000000000B FFFFFFFFFFFFFF88'   result
         DC    XL16'ABABABABABABAB0B 1122334455667788'   v2

         VRR_A VSEG,0
         DC    XL16'FFFFFFFFFFFFFFAB 0000000000000048'   result
         DC    XL16'ABABABABABABABAB 1122334455667748'   v2

         VRR_A VSEG,0
         DC    XL16'FFFFFFFFFFFFFFAB FFFFFFFFFFFFFFF8'   result
         DC    XL16'ABABABABABABABAB F102C3040506D7F8'   v2

* Halfword
         VRR_A VSEG,1
         DC    XL16'0000000000000B0B 0000000000007718'   result
         DC    XL16'ABABABABABAB0B0B 1122334455667718'   v2

         VRR_A VSEG,1
         DC    XL16'0000000000001B0B FFFFFFFFFFFFD788'   result
         DC    XL16'ABABABABABAB1B0B 112233445566D788'   v2

         VRR_A VSEG,1
         DC    XL16'FFFFFFFFFFFFABAB 0000000000007748'   result
         DC    XL16'ABABABABABABABAB 1122334455667748'   v2

         VRR_A VSEG,1
         DC    XL16'FFFFFFFFFFFFABAB FFFFFFFFFFFFC7F8'   result
         DC    XL16'ABABABABABABABAB F102C3040506C7F8'   v2

* Word
         VRR_A VSEG,2
         DC    XL16'000000000B0B0B0B 0000000055667718'   result
         DC    XL16'ABABABAB0B0B0B0B 1122334455667718'   v2

         VRR_A VSEG,2
         DC    XL16'000000001B0B1B0B FFFFFFFFC566D788'   result
         DC    XL16'ABABABAB1B0B1B0B 11223344C566D788'   v2

         VRR_A VSEG,2
         DC    XL16'FFFFFFFFABABABAB 0000000055667748'   result
         DC    XL16'ABABABABABABABAB 1122334455667748'   v2

         VRR_A VSEG,2
         DC    XL16'FFFFFFFFABABABAB FFFFFFFFB506C7F8'   result
         DC    XL16'ABABABABABABABAB F102C304B506C7F8'   v2

*---------------------------------------------------------------------
* VLC    - Vector Load Complement
*---------------------------------------------------------------------
* Byte
         VRR_A VLC,0
         DC    XL16'0000000000000000 0101010101010101'   result
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   v2

         VRR_A VLC,0
         DC    XL16'55555555555555F5 EFDECDBCAB9A89E8'   result
         DC    XL16'ABABABABABABAB0B 1122334455667718'   v2

         VRR_A VLC,0
         DC    XL16'00FFFEFDFCFBFAF9 100F0E0D0C0B0A09'   result
         DC    XL16'0001020304050607 F0F1F2F3F4F5F6F7'   v2

* Halfword
         VRR_A VLC,1
         DC    XL16'0000000000000000 0001000100010001'   result
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   v2

         VRR_A VLC,1
         DC    XL16'54555455545554F5 EEDECCBCAA9A88E8'   result
         DC    XL16'ABABABABABABAB0B 1122334455667718'   v2

         VRR_A VLC,1
         DC    XL16'FFFFFDFDFBFBF9F9 0F0F0D0D0B0B0909'   result
         DC    XL16'0001020304050607 F0F1F2F3F4F5F6F7'   v2

* Word
         VRR_A VLC,2
         DC    XL16'0000000000000000 0000000100000001'   result
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   v2

         VRR_A VLC,2
         DC    XL16'54545455545454F5 EEDDCCBCAA9988E8'   result
         DC    XL16'ABABABABABABAB0B 1122334455667718'   v2

         VRR_A VLC,2
         DC    XL16'FFFEFDFDFBFAF9F9 0F0E0D0D0B0A0909'   result
         DC    XL16'0001020304050607 F0F1F2F3F4F5F6F7'   v2

* Doubleword
         VRR_A VLC,3
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   v2

         VRR_A VLC,3
         DC    XL16'54545454545454F5 EEDDCCBBAA9988E8'   result
         DC    XL16'ABABABABABABAB0B 1122334455667718'   v2

         VRR_A VLC,3
         DC    XL16'FFFEFDFCFBFAF9F9 0F0E0D0C0B0A0909'   result
         DC    XL16'0001020304050607 F0F1F2F3F4F5F6F7'   v2

*---------------------------------------------------------------------
* VLP    - Vector Load Positive
*---------------------------------------------------------------------
* Byte
         VRR_A VLP,0
         DC    XL16'0000000000000000 0101010101010101'   result
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   v2

         VRR_A VLP,0
         DC    XL16'555555555555550B 1122334455667718'   result
         DC    XL16'ABABABABABABAB0B 1122334455667718'   v2

         VRR_A VLP,0
         DC    XL16'0001020304050607 100F0E0D0C0B0A09'   result
         DC    XL16'0001020304050607 F0F1F2F3F4F5F6F7'   v2

* Halfword
         VRR_A VLP,1
         DC    XL16'0000000000000000 0001000100010001'   result
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   v2

         VRR_A VLP,1
         DC    XL16'54555455545554F5 1122334455667718'   result
         DC    XL16'ABABABABABABAB0B 1122334455667718'   v2

         VRR_A VLP,1
         DC    XL16'0001020304050607 0F0F0D0D0B0B0909'   result
         DC    XL16'0001020304050607 F0F1F2F3F4F5F6F7'   v2

* Word
         VRR_A VLP,2
         DC    XL16'0000000000000000 0000000100000001'   result
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   v2

         VRR_A VLP,2
         DC    XL16'54545455545454F5 1122334455667718'   result
         DC    XL16'ABABABABABABAB0B 1122334455667718'   v2

         VRR_A VLP,2
         DC    XL16'0001020304050607 0F0E0D0D0B0A0909'   result
         DC    XL16'0001020304050607 F0F1F2F3F4F5F6F7'   v2

* Doubleword
         VRR_A VLP,3
         DC    XL16'0000000000000000 0000000000000001'   result
         DC    XL16'0000000000000000 FFFFFFFFFFFFFFFF'   v2

         VRR_A VLP,3
         DC    XL16'54545454545454F5 1122334455667718'   result
         DC    XL16'ABABABABABABAB0B 1122334455667718'   v2

         VRR_A VLP,3
         DC    XL16'0001020304050607 0F0E0D0C0B0A0909'   result
         DC    XL16'0001020304050607 F0F1F2F3F4F5F6F7'   v2



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
