 TITLE 'zvector-e6-18-VSCSHP (Zvector E6 VRR-b)'
***********************************************************************
*
*        Zvector E6 instruction tests for VRR-b encoded:
*
*        E67C VSCSHP - DECIMAL SCALE AND CONVERT AND SPLIT TO HFP
*
*        James Wekel June 2024
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E6 VRR-b decimal
*  scale adn convert and split to HFP instruction.
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*  A cross-check test is performed if the shifted packed decimal
*  source can be converted to a 64-bit fixed value without overflow.
*  The cross-check test converts the 64-bit fixed value to short
*  float (CEGR) to compare with the high result. The high result is
*  converted back to fixed, subtracted from source 64-bit fixed, and
*  converted to long float (CDGR) for create the low result.
*  This low result is compared to VSCSHP low result. An XCHECK test
*  error message will be issued if there is a difference.
*
***********************************************************************
*
*  *Testcase zvector-e6-18-VSCSHP: VECTOR E6 VRR-b VSCSHP instruction
*  *
*  *        Zvector E6 instruction tests for VRR-b encoded:
*  *
*  *        E67C VSCSHP - DECIMAL SCALE AND CONVERT AND SPLIT TO HFP
*  *
*  *        # --------------------------------------------------------
*  *        #  This tests only the basic function of the instruction.
*  *        #  Exceptions are NOT tested.
*  *        # --------------------------------------------------------
*  *
*  mainsize    2
*  numcpu      1
*  sysclear
*  archlvl     z/Arch
*
*  loadcore    "$(testpath)/zvector-e6-18-VSCSHP.core" 0x0
*
*  diag8cmd    enable    # (needed for messages to Hercules console)
*  runtest 2
*  diag8cmd    disable   # (reset back to default)
*
*  *Done
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
* Is Vector-packed-decimal-enhancement facility 2 installed  (bit 192)
***********************************************************************

         FCHECK 192,'vector-packed-decimal-enhancement facility 2'

                                                                EJECT
***********************************************************************
*              Do tests in the E6TESTS table
***********************************************************************

         L     R12,=A(E6TESTS)       get table of test addresses

NEXTE6   EQU   *
         L     R5,0(0,R12)       get test address
         LTR   R5,R5                have a test?
         BZ    ENDTEST                 done?

         USING E6TEST,R5

         LH    R0,TNUM           save current test number
         ST    R0,TESTING        for easy reference

         L     R11,TSUB          get address of test routine
         BALR  R11,R11           do test

         VST   V1,V1OUTPUT       save result

         BAL   R15,XCHECK

         LGF   R1,READDR         expected result address
         CLC   V1OUTPUT,0(R1)
         BNE   FAILMSG           no, issue failed message

         LA    R12,4(0,R12)      next test address
         B     NEXTE6
                                                                 EJECT
*----------------------------------------------------------------------
* For small (19 digit) values, cross check result
* if rounding mode = 0 and convertion to 64-bit does not overflow
*
*        R15 - RETURN
*
*        v1,v2,v3 have result, source, scale
*----------------------------------------------------------------------
XCHECK   EQU   *

         VLR   V11,V2         copy source
         VSRPR V10,V11,V3,159,1    shift
         BCR   1,R15          cc=3: overflow: ignore and return

         VCVBG R0,V10,1,8     get 64-bit binary value
         BCR   1,15           cc=3: overflow: ignore and return

         VLLEBRZ V4,=F'0',2   zero V4 (FPR4)
         VLLEBRZ V6,=F'0',2   zero V6 (FPR6)
*
* convert R0 to appropriate short HFP format (high result)
*
         CEGR  FPR4,R0        convert r0 to short hfp
         VST   V4,XCV4
         VST   V1,XCV1
         CLC   XCV1(8),XCV4   compare short (long) HFP
         BNE   XCFAIL
*
* build low result & compare
*
         CGER  R1,1,FPR4       convert high result back to fixed
         SGRK  R2,R0,R1        split ie. subtract high result
         CDGR  FPR4,R2         convert r2 to long hfp
         VST   V4,XCV4
         CLC   XCV1+8(8),XCV4  compare low result long HFP
         BNE   XCFAIL
         BR    R15             Ok, exit

* xcheck failed message
XCFAIL   DS    0H
         LH    R2,TNUM              get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   XCPTNUM(3),PRT3+13   fill in message with test #

         MVC   XCPNAME,OPNAME       fill in message with instruction

         XGR   R2,R2                get scale as U8
         IC    R2,SCALE                and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   XCPSCALE(3),PRT3+13  fill in message with scale field

         ST    R15,XCR15            save r15
         LA    R0,XCPLNG            message length
         LA    R1,XCPLINE           messagfe address
         BAL   R15,RPTERROR

         L     R15,XCR15
         BR    R15         return from xcheck

         DS    0FD
XCV4     DS    XL16
XCV1     DS    XL16
XCR15    DS    FD

                                                                 EJECT
***********************************************************************
* result not as expected:
*        issue message with test number, instruction under test
*              and instruction i3
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
         IC    R2,SCALE             get scale and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTSCALE(3),PRT3+13     fill in message with scale

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
         DC    C' with scale='
PRTSCALE DC    C'xxx'
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
         DC    C' with scale='
XCPSCALE DC    C'xxx'
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
         DC    C'V1 Output ===>'
         DS    0F
V1OUTPUT DS    XL16                                      V1 OUTPUT
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
TSUB     DC    A(0)           pointer  to test
TNUM     DC    H'00'          Test Number
         DC    X'00'
SCALE    DC    HL1'00'        scale used
V2ADDR   DC    A(0)           address of v2: 16-byte packed decimal
OPNAME   DC    CL8' '         E6 name
RELEN    DC    A(0)           result length
READDR   DC    A(0)           expected result address

**
*        test routine will be here (from VRR-b macro)
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
         VRR_B &INST,&SCALE
.*                               &INST - VRR-b instruction under test

         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
V3_&TNUM DC    HL1'&SCALE'       scale
V2_&TNUM DC    A(RE&TNUM+16)     address of v2: 16-byte packed decimal
         DC    CL8'&INST'        instruction name
         DC    A(16)             result length
         DC    A(RE&TNUM)        address of expected result
.*
*
X&TNUM   DS    0F
         VL    V1,V1FUDGE        fudge V1

         LGF   R2,V2_&TNUM       get v2
         VL    V2,0(R2)

         VLEB  V3,V3_&TNUM,7     get v3 scale

         &INST V1,V2,V3          test instruction

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
*        E6 VRR-b tests
***********************************************************************
         PRINT DATA
*
*         E67C VSCSHP - DECIMAL SCALE AND CONVERT AND SPLIT TO HFP
*
*----------------------------------------------------------------------
* VSCSHP - DECIMAL SCALE AND CONVERT AND SPLIT TO HFP
*----------------------------------------------------------------------
*   VRR-b   instruction
*   followed by
*              followed by
*              v1 - 16 byte expected result
*              v2 - 16 byte zoned decimal (operand)
*----------------------------------------------------------------------
* NO Shift/Scale
*----------------------------------------------------------------------

* +0
         VRR_B VSCSHP,0
         DC    XL16'00000000000000000000000000000000'
         DC    XL16'0000000000000000000000000000000C'
* -0
         VRR_B VSCSHP,0
         DC    XL16'00000000000000000000000000000000'
         DC    XL16'0000000000000000000000000000000D'
* +1
         VRR_B VSCSHP,0
         DC    XL16'41100000000000000000000000000000'
         DC    XL16'0000000000000000000000000000001C'
* -1
         VRR_B VSCSHP,0
         DC    XL16'C1100000000000000000000000000000'
         DC    XL16'0000000000000000000000000000001D'

* -36650385409
         VRR_B VSCSHP,0
         DC    XL16'C988888800000000C110000000000000'
         DC    XL16'0000000000000000000036650385409D'

* +9000000000000001
         VRR_B VSCSHP,0
         DC    XL16'4E1FF9730000000048CAFA8001000000'
         DC    XL16'0000000000000009000000000000001C'

* -9223372036854775808
         VRR_B VSCSHP,0
         DC    XL16'D0800000000000000000000000000000'
         DC    XL16'0000000000009223372036854775808D'

*  9223372036854775807
         VRR_B VSCSHP,0
         DC    XL16'507FFFFF000000004AFFFFFFFFFF0000'
         DC    XL16'0000000000009223372036854775807C'

* 18446744073709551615
         VRR_B VSCSHP,0
         DC    XL16'50FFFFFF000000004AFFFFFFFFFF0000'
         DC    XL16'0000000000018446744073709551615C'

* 9009000000018446744073709551615
         VRR_B VSCSHP,0
         DC    XL16'5A71B5A6000000005423751870DF6067'
         DC    XL16'9009000000018446744073709551615C'

* 9999999990018446744073709551615
         VRR_B VSCSHP,0
         DC    XL16'5A7E37BE00000000541E05A6B0816BCD'
         DC    XL16'9999999990018446744073709551615C'

*----------------------------------------------------------------------
* With Shift/Scale
*----------------------------------------------------------------------

* +0
         VRR_B VSCSHP,1
         DC    XL16'00000000000000000000000000000000'
         DC    XL16'0000000000000000000000000000000C'
* -0
         VRR_B VSCSHP,1
         DC    XL16'00000000000000000000000000000000'
         DC    XL16'0000000000000000000000000000000D'
* +1
         VRR_B VSCSHP,1
         DC    XL16'41A00000000000000000000000000000'
         DC    XL16'0000000000000000000000000000001C'
* -1
         VRR_B VSCSHP,1
         DC    XL16'C1A00000000000000000000000000000'
         DC    XL16'0000000000000000000000000000001D'

* -36650385409
         VRR_B VSCSHP,2
         DC    XL16'CB35555500000000C520064000000000'
         DC    XL16'0000000000000000000036650385409D'

* +9000000000000001
         VRR_B VSCSHP,2
         DC    XL16'4FC7D7130000000049B49DA006400000'
         DC    XL16'0000000000000009000000000000001C'

* -9223372036854775808
         VRR_B VSCSHP,2
         DC    XL16'D2320000000000000000000000000000'
         DC    XL16'0000000000009223372036854775808D'

*  9223372036854775807
         VRR_B VSCSHP,2
         DC    XL16'5231FFFF000000004CFFFFFFFFFF9C00'
         DC    XL16'0000000000009223372036854775807C'

* 18446744073709551615
         VRR_B VSCSHP,2
         DC    XL16'5263FFFF000000004CFFFFFFFFFF9C00'
         DC    XL16'0000000000018446744073709551615C'

* 9009000000018446744073709551615
         VRR_B VSCSHP,3
         DC    XL16'5D1BC2D90000000056FA816778E89096'
         DC    XL16'9009000000018446744073709551615C'

* 9999999990018446744073709551615
         VRR_B VSCSHP,3
         DC    XL16'5D1ED09B0000000057EA5461321798D1'
         DC    XL16'9999999990018446744073709551615C'


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
