 TITLE 'zvector-e7-26-VGM'
***********************************************************************
*
*   Zvector E7 instruction tests for VRI-b instruction:
*
*   E746 VGM    - Vector Generate Mask
*
*        James Wekel April 2025
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the z/arch E7 VRI-b
*  Vector Generate Mask instruction.
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*   *Testcase zvector-e7-26-VGM
*   *
*   *   Zvector E7 instruction tests for VRI-b instruction:
*   *
*   *   E746 VGM    - Vector Generate Mask
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
*   loadcore    "$(testpath)/zvector-e7-26-VGM.core" 0x0
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
*              and instruction i2
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
         XR    R2,R2
         IC    R2,I2                get i2 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTI2(2),PRT3+14     fill in message with i2 field
*
         XR    R2,R2
         IC    R2,I3                get i3 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTI3(2),PRT3+14     fill in message with i3 field
*
         XR    R2,R2
         IC    R2,M4                  get M4 and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   PRTM4(2),PRT3+14     fill in message with m4 field
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
         DC    C' with i2='
PRTI2    DC    C'xx'
         DC    C', with i3='
PRTI3    DC    C'xx'
         DC    C', with M4='
PRTM4    DC    C'xx'
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
V1FUDGE  DC    XL16'44444444444444444444444444444444'    V1 FUDGE
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

M4       DC    HL1'00'        M4 field
I2       DC    HL1'00'        i2 used
I3       DC    Hl1'00'        i3 used

OPNAME   DC    CL8' '         E7 name
V2ADDR   DC    A(0)           address of v2 source
V3ADDR   DC    A(0)           address of v3 source
RELEN    DC    A(0)           RESULT LENGTH
READDR   DC    A(0)           result (expected) address
         DS    FD                gap
V1OUTPUT DS    XL16           V1 Output
         DS    FD                gap

*        test routine will be here (from VRI-b macro)
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
* macros to generate individual test
*
         MACRO
         VRI_B &INST,&I2,&I3,&M4
.*                               &INST   - VRI-b instruction under test
.*                               &i2     - i2 field (unsigned decimal)
.*                               &i3     - i3 field (unsigned decimal)
.*                               &M4     - element size

         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
         DC    HL1'&M4'          M4 field
         DC    HL1'&I2'          i2 used
         DC    HL1'&I3'          i3 used
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
         VL    V22,V1FUDGE

         &INST V22,&I2,&I3,&M4    test instruction  (dest is a source)

         VST   V22,V1O&TNUM      save v1 output
         BR    R11               return

RE&TNUM  DC    0F                xl16 expected result

         DROP  R5
         MEND
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
*        E7 VRI-b tests
***********************************************************************
         PRINT DATA
*
*   E746 VGM    - Vector Generate Mask
*
*        VRI_B  instruction, I2, I3, M4
*              followed by
*                 16 byte expected result (V1)
*---------------------------------------------------------------------
* VGM    - Vector Generate Mask
*---------------------------------------------------------------------
*Byte: I2<I3; I2=0
         VRI_B VGM,0,0,0
         DC    XL16'8080808080808080 8080808080808080'   result

         VRI_B VGM,0,1,0
         DC    XL16'C0C0C0C0C0C0C0C0 C0C0C0C0C0C0C0C0'   result

         VRI_B VGM,0,2,0
         DC    XL16'E0E0E0E0E0E0E0E0 E0E0E0E0E0E0E0E0'   result

         VRI_B VGM,0,4,0
         DC    XL16'F8F8F8F8F8F8F8F8 F8F8F8F8F8F8F8F8'   result

         VRI_B VGM,0,6,0
         DC    XL16'FEFEFEFEFEFEFEFE FEFEFEFEFEFEFEFE'   result

         VRI_B VGM,0,7,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result


                                                                EJECT
*Byte: I2<I3; I2=1
         VRI_B VGM,1,1,0
         DC    XL16'4040404040404040 4040404040404040'   result

         VRI_B VGM,1,2,0
         DC    XL16'6060606060606060 6060606060606060'   result

         VRI_B VGM,1,4,0
         DC    XL16'7878787878787878 7878787878787878'   result

         VRI_B VGM,1,6,0
         DC    XL16'7E7E7E7E7E7E7E7E 7E7E7E7E7E7E7E7E'   result

         VRI_B VGM,1,7,0
         DC    XL16'7F7F7F7F7F7F7F7F 7F7F7F7F7F7F7F7F'   result


                                                                EJECT
*Byte: I2>I3; I3=0
         VRI_B VGM,1,0,0
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result

         VRI_B VGM,2,0,0
         DC    XL16'BFBFBFBFBFBFBFBF BFBFBFBFBFBFBFBF'   result

         VRI_B VGM,4,0,0
         DC    XL16'8F8F8F8F8F8F8F8F 8F8F8F8F8F8F8F8F'   result

         VRI_B VGM,6,0,0
         DC    XL16'8383838383838383 8383838383838383'   result

         VRI_B VGM,7,0,0
         DC    XL16'8181818181818181 8181818181818181'   result


                                                                EJECT
*Byte: I2>I3; I3=1
         VRI_B VGM,2,1,0
          DC   XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result

         VRI_B VGM,4,1,0
         DC    XL16'CFCFCFCFCFCFCFCF CFCFCFCFCFCFCFCF'   result

         VRI_B VGM,6,1,0
         DC    XL16'C3C3C3C3C3C3C3C3 C3C3C3C3C3C3C3C3'   result

         VRI_B VGM,7,1,0
         DC    XL16'C1C1C1C1C1C1C1C1 C1C1C1C1C1C1C1C1'   result


         VRI_B VGM,0,64,0
         DC    XL16'8080808080808080 8080808080808080'   result

                                                                EJECT
*Halfword: I2<I3; I2=0
         VRI_B VGM,0,0,1
         DC    XL16'8000800080008000 8000800080008000'   result

         VRI_B VGM,0,1,1
         DC    XL16'C000C000C000C000 C000C000C000C000'   result

         VRI_B VGM,0,2,1
         DC    XL16'E000E000E000E000 E000E000E000E000'   result

         VRI_B VGM,0,4,1
         DC    XL16'F800F800F800F800 F800F800F800F800'   result

         VRI_B VGM,0,6,1
         DC    XL16'FE00FE00FE00FE00 FE00FE00FE00FE00'   result

         VRI_B VGM,0,7,1
         DC    XL16'FF00FF00FF00FF00 FF00FF00FF00FF00'   result

         VRI_B VGM,0,8,1
         DC    XL16'FF80FF80FF80FF80 FF80FF80FF80FF80'   result

         VRI_B VGM,0,9,1
         DC    XL16'FFC0FFC0FFC0FFC0 FFC0FFC0FFC0FFC0'   result

         VRI_B VGM,0,11,1
         DC    XL16'FFF0FFF0FFF0FFF0 FFF0FFF0FFF0FFF0'   result

         VRI_B VGM,0,13,1
         DC    XL16'FFFCFFFCFFFCFFFC FFFCFFFCFFFCFFFC'   result

         VRI_B VGM,0,15,1
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result

          VRI_B VGM,0,16,1
         DC    XL16'8000800080008000 8000800080008000'   result

                                                                EJECT
*Halfword: I2<I3; I2=1
         VRI_B VGM,1,1,1
         DC    XL16'4000400040004000 4000400040004000'   result

         VRI_B VGM,1,2,1
         DC    XL16'6000600060006000 6000600060006000'   result

         VRI_B VGM,1,4,1
         DC    XL16'7800780078007800 7800780078007800'   result

         VRI_B VGM,1,6,1
         DC    XL16'7E007E007E007E00 7E007E007E007E00'   result

         VRI_B VGM,1,7,1
         DC    XL16'7F007F007F007F00 7F007F007F007F00'   result

         VRI_B VGM,1,8,1
         DC    XL16'7F807F807F807F80 7F807F807F807F80'   result

         VRI_B VGM,1,9,1
         DC    XL16'7FC07FC07FC07FC0 7FC07FC07FC07FC0'   result

         VRI_B VGM,1,11,1
         DC    XL16'7FF07FF07FF07FF0 7FF07FF07FF07FF0'   result

         VRI_B VGM,1,13,1
         DC    XL16'7FFC7FFC7FFC7FFC 7FFC7FFC7FFC7FFC'   result

         VRI_B VGM,1,15,1
         DC    XL16'7FFF7FFF7FFF7FFF 7FFF7FFF7FFF7FFF'   result

         VRI_B VGM,1,16,1
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result

                                                                EJECT
*Halfword: I2>I3; I3=0
         VRI_B VGM,2,0,1
         DC    XL16'BFFFBFFFBFFFBFFF BFFFBFFFBFFFBFFF'   result

         VRI_B VGM,4,0,1
         DC    XL16'8FFF8FFF8FFF8FFF 8FFF8FFF8FFF8FFF'   result

         VRI_B VGM,6,0,1
         DC    XL16'83FF83FF83FF83FF 83FF83FF83FF83FF'   result

         VRI_B VGM,7,0,1
         DC    XL16'81FF81FF81FF81FF 81FF81FF81FF81FF'   result

         VRI_B VGM,8,0,1
         DC    XL16'80FF80FF80FF80FF 80FF80FF80FF80FF'   result

         VRI_B VGM,9,0,1
         DC    XL16'807F807F807F807F 807F807F807F807F'   result

         VRI_B VGM,11,0,1
         DC    XL16'801F801F801F801F 801F801F801F801F'   result

         VRI_B VGM,13,0,1
         DC    XL16'8007800780078007 8007800780078007'   result

         VRI_B VGM,15,0,1
          DC   XL16'8001800180018001 8001800180018001'   result

          VRI_B VGM,16,0,1
         DC    XL16'8000800080008000 8000800080008000'   result

                                                                EJECT
*Halfword: I2>I3; I3=1
         VRI_B VGM,2,1,1
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result

         VRI_B VGM,4,1,1
         DC    XL16'CFFFCFFFCFFFCFFF CFFFCFFFCFFFCFFF'   result

         VRI_B VGM,6,1,1
         DC    XL16'C3FFC3FFC3FFC3FF C3FFC3FFC3FFC3FF'   result

         VRI_B VGM,7,1,1
         DC    XL16'C1FFC1FFC1FFC1FF C1FFC1FFC1FFC1FF'   result

         VRI_B VGM,8,1,1
         DC    XL16'C0FFC0FFC0FFC0FF C0FFC0FFC0FFC0FF'   result

         VRI_B VGM,9,1,1
         DC    XL16'C07FC07FC07FC07F C07FC07FC07FC07F'   result

         VRI_B VGM,11,1,1
         DC    XL16'C01FC01FC01FC01F C01FC01FC01FC01F'   result

         VRI_B VGM,13,1,1
         DC    XL16'C007C007C007C007 C007C007C007C007'   result

         VRI_B VGM,15,1,1
         DC    XL16'C001C001C001C001 C001C001C001C001'   result

          VRI_B VGM,16,1,1
         DC    XL16'C000C000C000C000 C000C000C000C000'   result

         VRI_B VGM,64,1,1
         DC    XL16'C000C000C000C000 C000C000C000C000'   result

                                                                EJECT
*Word: I2<I3; I2=0
         VRI_B VGM,0,0,2
         DC    XL16'8000000080000000 8000000080000000'   result

         VRI_B VGM,0,1,2
         DC    XL16'C0000000C0000000 C0000000C0000000'   result

         VRI_B VGM,0,2,2
         DC    XL16'E0000000E0000000 E0000000E0000000'   result

         VRI_B VGM,0,4,2
         DC    XL16'F8000000F8000000 F8000000F8000000'   result

         VRI_B VGM,0,6,2
         DC    XL16'FE000000FE000000 FE000000FE000000'   result

         VRI_B VGM,0,7,2
         DC    XL16'FF000000FF000000 FF000000FF000000'   result

         VRI_B VGM,0,8,2
         DC    XL16'FF800000FF800000 FF800000FF800000'   result

         VRI_B VGM,0,9,2
         DC    XL16'FFC00000FFC00000 FFC00000FFC00000'   result

         VRI_B VGM,0,11,2
         DC    XL16'FFF00000FFF00000 FFF00000FFF00000'   result

         VRI_B VGM,0,13,2
         DC    XL16'FFFC0000FFFC0000 FFFC0000FFFC0000'   result

         VRI_B VGM,0,15,2
         DC    XL16'FFFF0000FFFF0000 FFFF0000FFFF0000'   result

          VRI_B VGM,0,16,2
         DC    XL16'FFFF8000FFFF8000 FFFF8000FFFF8000'   result

         VRI_B VGM,0,17,2
         DC    XL16'FFFFC000FFFFC000 FFFFC000FFFFC000'   result

         VRI_B VGM,0,25,2
         DC    XL16'FFFFFFC0FFFFFFC0 FFFFFFC0FFFFFFC0'   result

         VRI_B VGM,0,30,2
         DC    XL16'FFFFFFFEFFFFFFFE FFFFFFFEFFFFFFFE'   result

         VRI_B VGM,0,31,2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result

         VRI_B VGM,0,32,2
         DC    XL16'8000000080000000 8000000080000000'   result

         VRI_B VGM,0,64,2
         DC    XL16'8000000080000000 8000000080000000'   result

                                                                EJECT
*Word: I2<I3; I2=1
         VRI_B VGM,1,1,2
         DC    XL16'4000000040000000 4000000040000000'   result

         VRI_B VGM,1,2,2
         DC    XL16'6000000060000000 6000000060000000'   result

         VRI_B VGM,1,4,2
         DC    XL16'7800000078000000 7800000078000000'   result

         VRI_B VGM,1,6,2
         DC    XL16'7E0000007E000000 7E0000007E000000'   result

         VRI_B VGM,1,7,2
         DC    XL16'7F0000007F000000 7F0000007F000000'   result

         VRI_B VGM,1,8,2
         DC    XL16'7F8000007F800000 7F8000007F800000'   result

         VRI_B VGM,1,9,2
         DC    XL16'7FC000007FC00000 7FC000007FC00000'   result

         VRI_B VGM,1,11,2
         DC    XL16'7FF000007FF00000 7FF000007FF00000'   result

         VRI_B VGM,1,13,2
         DC    XL16'7FFC00007FFC0000 7FFC00007FFC0000'   result

         VRI_B VGM,1,15,2
         DC    XL16'7FFF00007FFF0000 7FFF00007FFF0000'   result

          VRI_B VGM,1,16,2
         DC    XL16'7FFF80007FFF8000 7FFF80007FFF8000'   result

         VRI_B VGM,1,17,2
         DC    XL16'7FFFC0007FFFC000 7FFFC0007FFFC000'   result

         VRI_B VGM,1,25,2
         DC    XL16'7FFFFFC07FFFFFC0 7FFFFFC07FFFFFC0'   result

         VRI_B VGM,1,30,2
         DC    XL16'7FFFFFFE7FFFFFFE 7FFFFFFE7FFFFFFE'   result

         VRI_B VGM,1,31,2
         DC    XL16'7FFFFFFF7FFFFFFF 7FFFFFFF7FFFFFFF'   result

                                                                EJECT
*Word: I2>I3; I2=0
         VRI_B VGM,1,0,2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result

         VRI_B VGM,2,0,2
         DC    XL16'BFFFFFFFBFFFFFFF BFFFFFFFBFFFFFFF'   result

         VRI_B VGM,4,0,2
         DC    XL16'8FFFFFFF8FFFFFFF 8FFFFFFF8FFFFFFF'   result

         VRI_B VGM,6,0,2
         DC    XL16'83FFFFFF83FFFFFF 83FFFFFF83FFFFFF'   result

         VRI_B VGM,7,0,2
         DC    XL16'81FFFFFF81FFFFFF 81FFFFFF81FFFFFF'   result

         VRI_B VGM,8,0,2
         DC    XL16'80FFFFFF80FFFFFF 80FFFFFF80FFFFFF'   result

         VRI_B VGM,9,0,2
         DC    XL16'807FFFFF807FFFFF 807FFFFF807FFFFF'   result

         VRI_B VGM,11,0,2
         DC    XL16'801FFFFF801FFFFF 801FFFFF801FFFFF'   result

         VRI_B VGM,13,0,2
         DC    XL16'8007FFFF8007FFFF 8007FFFF8007FFFF'   result

         VRI_B VGM,15,0,2
         DC    XL16'8001FFFF8001FFFF 8001FFFF8001FFFF'   result

          VRI_B VGM,16,0,2
         DC    XL16'8000FFFF8000FFFF 8000FFFF8000FFFF'   result

         VRI_B VGM,17,0,2
         DC    XL16'80007FFF80007FFF 80007FFF80007FFF'   result

         VRI_B VGM,25,0,2
         DC    XL16'8000007F8000007F 8000007F8000007F'   result

         VRI_B VGM,30,0,2
         DC    XL16'8000000380000003 8000000380000003'   result

         VRI_B VGM,31,0,2
         DC    XL16'8000000180000001 8000000180000001'   result

                                                                EJECT
*Word: I2>I3; I2=1
         VRI_B VGM,2,1,2
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result

         VRI_B VGM,4,1,2
         DC    XL16'CFFFFFFFCFFFFFFF CFFFFFFFCFFFFFFF'   result

         VRI_B VGM,6,1,2
         DC    XL16'C3FFFFFFC3FFFFFF C3FFFFFFC3FFFFFF'   result

         VRI_B VGM,7,1,2
         DC    XL16'C1FFFFFFC1FFFFFF C1FFFFFFC1FFFFFF'   result

         VRI_B VGM,8,1,2
         DC    XL16'C0FFFFFFC0FFFFFF C0FFFFFFC0FFFFFF'   result

         VRI_B VGM,9,1,2
         DC    XL16'C07FFFFFC07FFFFF C07FFFFFC07FFFFF'   result

         VRI_B VGM,11,1,2
         DC    XL16'C01FFFFFC01FFFFF C01FFFFFC01FFFFF'   result

         VRI_B VGM,13,1,2
         DC    XL16'C007FFFFC007FFFF C007FFFFC007FFFF'   result

         VRI_B VGM,15,1,2
         DC    XL16'C001FFFFC001FFFF C001FFFFC001FFFF'   result

          VRI_B VGM,16,1,2
         DC    XL16'C000FFFFC000FFFF C000FFFFC000FFFF'   result

         VRI_B VGM,17,1,2
         DC    XL16'C0007FFFC0007FFF C0007FFFC0007FFF'   result

         VRI_B VGM,25,1,2
         DC    XL16'C000007FC000007F C000007FC000007F'   result

         VRI_B VGM,30,1,2
         DC    XL16'C0000003C0000003 C0000003C0000003'   result

         VRI_B VGM,31,1,2
         DC    XL16'C0000001C0000001 C0000001C0000001'   result

                                                                EJECT
*Doubleword: I2<I3; I2=0
         VRI_B VGM,0,0,3
         DC    XL16'8000000000000000 8000000000000000'   result

         VRI_B VGM,0,1,3
         DC    XL16'C000000000000000 C000000000000000'   result

         VRI_B VGM,0,2,3
         DC    XL16'E000000000000000 E000000000000000'   result

         VRI_B VGM,0,4,3
         DC    XL16'F800000000000000 F800000000000000'   result

         VRI_B VGM,0,7,3
         DC    XL16'FF00000000000000 FF00000000000000'   result

         VRI_B VGM,0,8,3
         DC    XL16'FF80000000000000 FF80000000000000'   result

         VRI_B VGM,0,9,3
         DC    XL16'FFC0000000000000 FFC0000000000000'   result

         VRI_B VGM,0,13,3
         DC    XL16'FFFC000000000000 FFFC000000000000'   result

         VRI_B VGM,0,15,3
         DC    XL16'FFFF000000000000 FFFF000000000000'   result

          VRI_B VGM,0,16,3
         DC    XL16'FFFF800000000000 FFFF800000000000'   result

         VRI_B VGM,0,17,3
         DC    XL16'FFFFC00000000000 FFFFC00000000000'   result

         VRI_B VGM,0,25,3
         DC    XL16'FFFFFFC000000000 FFFFFFC000000000'   result

         VRI_B VGM,0,30,3
         DC    XL16'FFFFFFFE00000000 FFFFFFFE00000000'   result

         VRI_B VGM,0,31,3
         DC    XL16'FFFFFFFF00000000 FFFFFFFF00000000'   result

         VRI_B VGM,0,32,3
         DC    XL16'FFFFFFFF80000000 FFFFFFFF80000000'   result

         VRI_B VGM,0,33,3
         DC    XL16'FFFFFFFFC0000000 FFFFFFFFC0000000'   result

         VRI_B VGM,0,55,3
         DC    XL16'FFFFFFFFFFFFFF00 FFFFFFFFFFFFFF00'   result

         VRI_B VGM,0,64,3
         DC    XL16'8000000000000000 8000000000000000'   result

                                                                EJECT
*Doubleword: I2<I3; I2=1
         VRI_B VGM,1,1,3
         DC    XL16'4000000000000000 4000000000000000'   result

         VRI_B VGM,1,2,3
         DC    XL16'6000000000000000 6000000000000000'   result

         VRI_B VGM,1,4,3
         DC    XL16'7800000000000000 7800000000000000'   result

         VRI_B VGM,1,7,3
         DC    XL16'7F00000000000000 7F00000000000000'   result

         VRI_B VGM,1,8,3
         DC    XL16'7F80000000000000 7F80000000000000'   result

         VRI_B VGM,1,9,3
         DC    XL16'7FC0000000000000 7FC0000000000000'   result

         VRI_B VGM,1,13,3
         DC    XL16'7FFC000000000000 7FFC000000000000'   result

         VRI_B VGM,1,15,3
         DC    XL16'7FFF000000000000 7FFF000000000000'   result

          VRI_B VGM,1,16,3
         DC    XL16'7FFF800000000000 7FFF800000000000'   result

         VRI_B VGM,1,17,3
         DC    XL16'7FFFC00000000000 7FFFC00000000000'   result

         VRI_B VGM,1,25,3
         DC    XL16'7FFFFFC000000000 7FFFFFC000000000'   result

         VRI_B VGM,1,30,3
         DC    XL16'7FFFFFFE00000000 7FFFFFFE00000000'   result

         VRI_B VGM,1,31,3
         DC    XL16'7FFFFFFF00000000 7FFFFFFF00000000'   result

         VRI_B VGM,1,32,3
         DC    XL16'7FFFFFFF80000000 7FFFFFFF80000000'   result

         VRI_B VGM,1,33,3
         DC    XL16'7FFFFFFFC0000000 7FFFFFFFC0000000'   result

         VRI_B VGM,1,55,3
         DC    XL16'7FFFFFFFFFFFFF00 7FFFFFFFFFFFFF00'   result

         VRI_B VGM,1,62,3
         DC    XL16'7FFFFFFFFFFFFFFE 7FFFFFFFFFFFFFFE'   result

                                                                EJECT
*Doubleword: I2>I3; I3=0
         VRI_B VGM,1,0,3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result

         VRI_B VGM,2,0,3
         DC    XL16'BFFFFFFFFFFFFFFF BFFFFFFFFFFFFFFF'   result

         VRI_B VGM,4,0,3
         DC    XL16'8FFFFFFFFFFFFFFF 8FFFFFFFFFFFFFFF'   result

         VRI_B VGM,6,0,3
         DC    XL16'83FFFFFFFFFFFFFF 83FFFFFFFFFFFFFF'   result

         VRI_B VGM,7,0,3
         DC    XL16'81FFFFFFFFFFFFFF 81FFFFFFFFFFFFFF'   result

         VRI_B VGM,8,0,3
         DC    XL16'80FFFFFFFFFFFFFF 80FFFFFFFFFFFFFF'   result

         VRI_B VGM,9,0,3
         DC    XL16'807FFFFFFFFFFFFF 807FFFFFFFFFFFFF'   result

         VRI_B VGM,11,0,3
         DC    XL16'801FFFFFFFFFFFFF 801FFFFFFFFFFFFF'   result

         VRI_B VGM,13,0,3
         DC    XL16'8007FFFFFFFFFFFF 8007FFFFFFFFFFFF'   result

         VRI_B VGM,15,0,3
         DC    XL16'8001FFFFFFFFFFFF 8001FFFFFFFFFFFF'   result

          VRI_B VGM,16,0,3
         DC    XL16'8000FFFFFFFFFFFF 8000FFFFFFFFFFFF'   result

         VRI_B VGM,17,0,3
         DC    XL16'80007FFFFFFFFFFF 80007FFFFFFFFFFF'   result

         VRI_B VGM,25,0,3
         DC    XL16'8000007FFFFFFFFF 8000007FFFFFFFFF'   result

         VRI_B VGM,30,0,3
         DC    XL16'80000003FFFFFFFF 80000003FFFFFFFF'   result

         VRI_B VGM,31,0,3
         DC    XL16'80000001FFFFFFFF 80000001FFFFFFFF'   result

         VRI_B VGM,33,0,3
         DC    XL16'800000007FFFFFFF 800000007FFFFFFF'   result

         VRI_B VGM,55,0,3
         DC    XL16'80000000000001FF 80000000000001FF'   result

         VRI_B VGM,62,0,3
         DC    XL16'8000000000000003 8000000000000003'   result

                                                                EJECT
*Doubleword: I2>I3; I3=1
         VRI_B VGM,2,1,3
         DC    XL16'FFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF'   result

         VRI_B VGM,4,1,3
         DC    XL16'CFFFFFFFFFFFFFFF CFFFFFFFFFFFFFFF'   result

         VRI_B VGM,6,1,3
         DC    XL16'C3FFFFFFFFFFFFFF C3FFFFFFFFFFFFFF'   result

         VRI_B VGM,7,1,3
         DC    XL16'C1FFFFFFFFFFFFFF C1FFFFFFFFFFFFFF'   result

         VRI_B VGM,8,1,3
         DC    XL16'C0FFFFFFFFFFFFFF C0FFFFFFFFFFFFFF'   result

         VRI_B VGM,9,1,3
         DC    XL16'C07FFFFFFFFFFFFF C07FFFFFFFFFFFFF'   result

         VRI_B VGM,11,1,3
         DC    XL16'C01FFFFFFFFFFFFF C01FFFFFFFFFFFFF'   result

         VRI_B VGM,13,1,3
         DC    XL16'C007FFFFFFFFFFFF C007FFFFFFFFFFFF'   result

         VRI_B VGM,15,1,3
         DC    XL16'C001FFFFFFFFFFFF C001FFFFFFFFFFFF'   result

          VRI_B VGM,16,1,3
         DC    XL16'C000FFFFFFFFFFFF C000FFFFFFFFFFFF'   result

         VRI_B VGM,17,1,3
         DC    XL16'C0007FFFFFFFFFFF C0007FFFFFFFFFFF'   result

         VRI_B VGM,25,1,3
         DC    XL16'C000007FFFFFFFFF C000007FFFFFFFFF'   result

         VRI_B VGM,30,1,3
         DC    XL16'C0000003FFFFFFFF C0000003FFFFFFFF'   result

         VRI_B VGM,31,1,3
         DC    XL16'C0000001FFFFFFFF C0000001FFFFFFFF'   result

         VRI_B VGM,33,1,3
         DC    XL16'C00000007FFFFFFF C00000007FFFFFFF'   result

         VRI_B VGM,55,1,3
         DC    XL16'C0000000000001FF C0000000000001FF'   result

         VRI_B VGM,62,1,3
         DC    XL16'C000000000000003 C000000000000003'   result



         DC    F'0'     END OF TABLE
         DC    F'0'
                                                                EJECT
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
