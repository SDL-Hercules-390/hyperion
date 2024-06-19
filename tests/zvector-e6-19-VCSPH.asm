 TITLE 'zvector-e6-19-VCSPH (Zvector E6 VRR-j)'
***********************************************************************
*
*        Zvector E6 instruction tests for VRR-j encoded:
*
*        E67D VCSPH   - VECTOR CONVERT HFP TO SCALED DECIMAL
*
*        James Wekel June 2024
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E6 VRR-j vector
*  convert HFP to scaled decimaal instruction.
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*  A cross-check test is performed if the rounding mode is zero,
*  and the shifted packed decimal source can be converted to a 64-bit
*  fixed value without overflow. The cross-check test converts the
*  packed decimal source, uses CEGR, CDGR or CXGR to convert to
*  HFP. This result is compared to VCSPH result. An XCHECK test
*  error message will be issued if there is a difference.
*
***********************************************************************
*
*   *Testcase zvector-e6-19-VCSPH: VECTOR E6 VRR-j VCSPH instruction
*   *
*   *        Zvector E6 instruction tests for VRR-j encoded:
*   *
*   *        E67D VCSPH   - VECTOR CONVERT HFP TO SCALED DECIMAL
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
*   loadcore    "$(testpath)/zvector-e6-19-VCSPH.core" 0x0
*
*   diag8cmd    enable    # (needed for messages to Hercules console)
*   runtest 2
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
         XGR   R1,R1          Only Xcheck when shift=0
         IC    R1,SCALE          get scale
         LTR   R1,R1
         BNZ   0(R15)         a scale/shit, so exit
*
*        convert source extended float to fixed (R0)
*
         VST   V2,XCV2        copy source
         LD    FPR4,XCV2      load extended HFP
         LD    FPR6,XCV2+8

         XGR   R1,R1          Is Rounding Mode = 0?
         IC    R1,M4          get M4
         NILL  r1,1           RM : bit 3
         LTR   R1,R1
         BNE   XCR01
*                             no rounding (to 0)
         CGXR  R0,0,FPR4
         BCR   1,15           cc=3: overflow: ignore and return
         B     XCR02
*                             Round to nearest with ties away from 0
XCR01    DS    0H
         CGXR  R0,1,FPR4
         BCR   1,15           cc=3: overflow: ignore and return
*
*        result to fixed (R1)
*
XCR02    DS    0H
         VCVBG R1,V1,1,8
         BCR   1,15           cc=3: overflow: ignore and return
*
*        values match?
*
         SGRK  R2,R0,R1       check difference
         BZ    0(R15)         Ok, exit

* xcheck failed message

         LH    R2,TNUM              get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   XCPTNUM(3),PRT3+13   fill in message with test #

         MVC   XCPNAME,OPNAME       fill in message with instruction

         XGR   R2,R2                get m4 as U8
         IC    R2,M4
         CVD   R2,DECNUM            and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   XCPM4(2),PRT3+14     fill in message with m4 field

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
         IC    R2,M4                get m4 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM4(2),PRT3+14     fill in message with m4 field
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
         DC    C' with m4='
PRTM4    DC    C'xx'
         DC    C', with scale='
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
         DC    C' with m4='
XCPM4    DC    C'xx'
         DC    C', with scale='
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
M4       DC    HL1'00'        m4 used
SCALE    DC    HL1'00'        scale used
V2ADDR   DC    A(0)           address of v2: 16-byte packed decimal
OPNAME   DC    CL8' '         E6 name
RELEN    DC    A(0)           result length
READDR   DC    A(0)           expected result address

**
*        test routine will be here (from VRR-j macro)
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
         VRR_J &INST,&M4,&SCALE
.*                               &INST - VRR-j instruction under test
.*                               &m4   - m4 field
         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
         DC    HL1'&M4'          m4
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

         &INST V1,V2,V3,&M4    test instruction

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
*        E6 VRR-j tests
***********************************************************************
         PRINT DATA
*
*        E67D VCSPH   - VECTOR CONVERT HFP TO SCALED DECIMAL
*
*----------------------------------------------------------------------
* VCSPH   - VECTOR CONVERT HFP TO SCALED DECIMAL
*----------------------------------------------------------------------
*   VRR-j   instruction, m4, scale(0-31)
*   followed by
*              followed by
*              v1 - 16 byte expected result
*              v2 - 16 byte extended HFP
*----------------------------------------------------------------------
* No Round - NO Shift
*----------------------------------------------------------------------
* +0
         VRR_J VCSPH,0,0
         DC    XL16'0000000000000000000000000000000C'
         DC    XL16'00000000000000000000000000000000'

* +1
         VRR_J VCSPH,0,0
         DC    XL16'0000000000000000000000000000001C'
         DC    XL16'41100000000000003300000000000000'

* -1
         VRR_J VCSPH,0,0
         DC    XL16'0000000000000000000000000000001D'
         DC    XL16'C110000000000000B300000000000000'

* +9000000000000001
         VRR_J VCSPH,0,0
         DC    XL16'0000000000000009000000000000001C'
         DC    XL16'4E1FF973CAFA80014000000000000000'


* -9223372036854775808
         VRR_J VCSPH,0,0
         DC    XL16'0000000000009223372036854775808D'
         DC    XL16'D080000000000000C200000000000000'


*  9223372036854775807
         VRR_J VCSPH,0,0
         DC    XL16'0000000000009223372036854775807C'
         DC    XL16'507FFFFFFFFFFFFF42FF000000000000'


* 18446744073709551615
         VRR_J VCSPH,0,0
         DC    XL16'0000000000018446744073709551615C'
         DC    XL16'50FFFFFFFFFFFFFF42FF000000000000'

* +1.25
         VRR_J VCSPH,0,0
         DC    XL16'0000000000000000000000000000001C'
         DC    XL16'41140000000000003300000000000000'

* +1.5
         VRR_J VCSPH,0,0
         DC    XL16'0000000000000000000000000000001C'
         DC    XL16'41180000000000003300000000000000'

* +1.75
         VRR_J VCSPH,0,0
         DC    XL16'0000000000000000000000000000001C'
         DC    XL16'411C0000000000003300000000000000'
*----------------------------------------------------------------------
* NO Round - with shifts
*----------------------------------------------------------------------

* +0
         VRR_J VCSPH,0,1
         DC    XL16'0000000000000000000000000000000C'
         DC    XL16'00000000000000000000000000000000'

* +1
         VRR_J VCSPH,0,1
         DC    XL16'0000000000000000000000000000010C'
         DC    XL16'41100000000000003300000000000000'

* -1
         VRR_J VCSPH,0,1
         DC    XL16'0000000000000000000000000000010D'
         DC    XL16'C110000000000000B300000000000000'


* +9000000000000001
         VRR_J VCSPH,0,2
         DC    XL16'0000000000000900000000000000100C'
         DC    XL16'4E1FF973CAFA80014000000000000000'


* -9223372036854775808
         VRR_J VCSPH,0,2
         DC    XL16'0000000000922337203685477580800D'
         DC    XL16'D080000000000000C200000000000000'


*  9223372036854775807
         VRR_J VCSPH,0,2
         DC    XL16'0000000000922337203685477580700C'
         DC    XL16'507FFFFFFFFFFFFF42FF000000000000'


* 18446744073709551615
         VRR_J VCSPH,0,2
         DC    XL16'0000000001844674407370955161500C'
         DC    XL16'50FFFFFFFFFFFFFF42FF000000000000'

* +1.25
         VRR_J VCSPH,0,1
         DC    XL16'0000000000000000000000000000012C'
         DC    XL16'41140000000000003300000000000000'

* +1.5
         VRR_J VCSPH,0,1
         DC    XL16'0000000000000000000000000000015C'
         DC    XL16'41180000000000003300000000000000'

* +1.75
         VRR_J VCSPH,0,1
         DC    XL16'0000000000000000000000000000017C'
         DC    XL16'411C0000000000003300000000000000'

*----------------------------------------------------------------------
* ROUND - NO Shift
*----------------------------------------------------------------------
* +0
         VRR_J VCSPH,1,0
         DC    XL16'0000000000000000000000000000000C'
         DC    XL16'00000000000000000000000000000000'


* +1
         VRR_J VCSPH,1,0
         DC    XL16'0000000000000000000000000000001C'
         DC    XL16'41100000000000003300000000000000'

* -1
         VRR_J VCSPH,1,0
         DC    XL16'0000000000000000000000000000001D'
         DC    XL16'C110000000000000B300000000000000'


* +9000000000000001
         VRR_J VCSPH,1,0
         DC    XL16'0000000000000009000000000000001C'
         DC    XL16'4E1FF973CAFA80014000000000000000'


* -9223372036854775808
         VRR_J VCSPH,1,0
         DC    XL16'0000000000009223372036854775808D'
         DC    XL16'D080000000000000C200000000000000'


*  9223372036854775807
         VRR_J VCSPH,1,0
         DC    XL16'0000000000009223372036854775807C'
         DC    XL16'507FFFFFFFFFFFFF42FF000000000000'


* 18446744073709551615
         VRR_J VCSPH,1,0
         DC    XL16'0000000000018446744073709551615C'
         DC    XL16'50FFFFFFFFFFFFFF42FF000000000000'


* 9009000000018446744073709551615
         VRR_J VCSPH,1,0
         DC    XL16'9009000000018446744073709551615C'
         DC    XL16'5A71B5A6237518704CDF6067FFFFFF00'


* 9999999990018446744073709551615
         VRR_J VCSPH,1,0
         DC    XL16'9999999990018446744073709551615C'
         DC    XL16'5A7E37BE1E05A6B04C816BCDBFFFFF00'

* +1.25
         VRR_J VCSPH,1,0
         DC    XL16'0000000000000000000000000000001C'
         DC    XL16'41140000000000003300000000000000'

* +1.5
         VRR_J VCSPH,1,0
         DC    XL16'0000000000000000000000000000002C'
         DC    XL16'41180000000000003300000000000000'

* +1.75
         VRR_J VCSPH,1,0
         DC    XL16'0000000000000000000000000000002C'
         DC    XL16'411C0000000000003300000000000000'

*----------------------------------------------------------------------
* ROUND - with shifts
*----------------------------------------------------------------------
* +0
         VRR_J VCSPH,1,1
         DC    XL16'0000000000000000000000000000000C'
         DC    XL16'00000000000000000000000000000000'


* +1
         VRR_J VCSPH,1,1
         DC    XL16'0000000000000000000000000000010C'
         DC    XL16'41100000000000003300000000000000'

* -1
         VRR_J VCSPH,1,1
         DC    XL16'0000000000000000000000000000010D'
         DC    XL16'C110000000000000B300000000000000'


* +9000000000000001
         VRR_J VCSPH,1,2
         DC    XL16'0000000000000900000000000000100C'
         DC    XL16'4E1FF973CAFA80014000000000000000'


* -9223372036854775808
         VRR_J VCSPH,1,2
         DC    XL16'0000000000922337203685477580800D'
         DC    XL16'D080000000000000C200000000000000'


*  9223372036854775807
         VRR_J VCSPH,1,2
         DC    XL16'0000000000922337203685477580700C'
         DC    XL16'507FFFFFFFFFFFFF42FF000000000000'


* 18446744073709551615
         VRR_J VCSPH,1,2
         DC    XL16'0000000001844674407370955161500C'
         DC    XL16'50FFFFFFFFFFFFFF42FF000000000000'


* 9009000000018446744073709551615
         VRR_J VCSPH,1,3
         DC    XL16'9000000018446744073709551615000C'
         DC    XL16'5A71B5A6237518704CDF6067FFFFFF00'


* 9999999990018446744073709551615
         VRR_J VCSPH,1,3
         DC    XL16'9999990018446744073709551615000C'
         DC    XL16'5A7E37BE1E05A6B04C816BCDBFFFFF00'


* +1.25
         VRR_J VCSPH,1,1
         DC    XL16'0000000000000000000000000000013C'
         DC    XL16'41140000000000003300000000000000'

* +1.5
         VRR_J VCSPH,1,1
         DC    XL16'0000000000000000000000000000015C'
         DC    XL16'41180000000000003300000000000000'

* +1.75
         VRR_J VCSPH,1,1
         DC    XL16'0000000000000000000000000000018C'
         DC    XL16'411C0000000000003300000000000000'

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
