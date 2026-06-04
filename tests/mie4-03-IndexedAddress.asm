 TITLE 'mie4-03-IndexedAddress'
***********************************************************************
*
*   Miscellaneous-instruction-extensions facility 4 instruction tests
*   for RXY-c encoded:
*
*   E360  LXAB  - load indexed address (shift left 0)          [RXY-c]
*   E361  LLXAB - load logical indexed address (shift left 0)  [RXY-c]
*   E362  LXAH  - load indexed address (shift left 1)          [RXY-c]
*   E363  LLXAH - load logical indexed address (shift left 1)  [RXY-c]
*   E364  LXAF  - load indexed address (shift left 2)          [RXY-c]
*   E365  LLXAF - load logical indexed address (shift left 2)  [RXY-c]
*   E366  LXAG  - load indexed address (shift left 3)          [RXY-c]
*   E367  LLXAG - load logical indexed address (shift left 3)  [RXY-c]
*   E368  LXAQ  - load indexed address (shift left 4)          [RXY-c]
*   E369  LLXAQ - load logical indexed address (shift left 4)  [RXY-c]
*
*        James Wekel March 2026
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************************
*  This program tests proper functioning of the Miscellaneous
*  Instruction Extensions facility 4 RXY-c load indexed address
*  and load logical indexed address instructions.
*  Exceptions are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*  *Testcase mie4-IndexAddress
*  *
*  *   Miscellaneous-instruction-extensions facility 4 instruction
*  *   tests for RXY-c encoded:
*  *
*  * E360  LXAB  - load indexed address (shift left 0)         [RXY-c]
*  * E361  LLXAB - load logical indexed address (shift left 0) [RXY-c]
*  * E362  LXAH  - load indexed address (shift left 1)         [RXY-c]
*  * E363  LLXAH - load logical indexed address (shift left 1) [RXY-c]
*  * E364  LXAF  - load indexed address (shift left 2)         [RXY-c]
*  * E365  LLXAF - load logical indexed address (shift left 2) [RXY-c]
*  * E366  LXAG  - load indexed address (shift left 3)         [RXY-c]
*  * E367  LLXAG - load logical indexed address (shift left 3) [RXY-c]
*  * E368  LXAQ  - load indexed address (shift left 4)         [RXY-c]
*  * E369  LLXAQ - load logical indexed address (shift left 4) [RXY-c]
*  *
*  *   # ------------------------------------------------------------
*  *   #  This tests only the basic function of the instruction.
*  *   #  Exceptions are NOT tested.
*  *   # ------------------------------------------------------------
*  *
*   mainsize    1
*   numcpu      1
*   sysclear
*   archlvl     z/Arch
*
*   loadcore    "$(testpath)/mie4-IndexAddress.core" 0x0
*
*   diag8cmd    enable    # (needed for messages to Hercules console)
*   runtest     5         #
*   diag8cmd    disable   # (reset back to default)
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
* (pending  E360  LXAB  - load indexed address (shift left 0)
*  inclusion in SATK ASAM)
***********************************************************************
         MACRO
         LXAB  &X2,&B2,&D2,&NEGD2
         LXA   60,&X2,&B2,&D2,&NEGD2
         MEND

***********************************************************************
* (pending  E361  LLXAB  - load logical indexed address (shift left 0)
*  inclusion in SATK ASAM)
***********************************************************************
         MACRO
         LLXAB &X2,&B2,&D2,&NEGD2
         LXA   61,&X2,&B2,&D2,&NEGD2
         MEND
                                                                 EJECT
***********************************************************************
* (pending  E362  LXAH  - load indexed address (shift left 1)
*  inclusion in SATK ASAM)
***********************************************************************
         MACRO
         LXAH  &X2,&B2,&D2,&NEGD2
         LXA   62,&X2,&B2,&D2,&NEGD2
         MEND

***********************************************************************
* (pending  E363  LLXAH  - load logical indexed address (shift left 1)
*  inclusion in SATK ASAM)
***********************************************************************
         MACRO
         LLXAH &X2,&B2,&D2,&NEGD2
         LXA   63,&X2,&B2,&D2,&NEGD2
         MEND
                                                                 EJECT
***********************************************************************
* (pending  E364  LXAF  - load indexed address (shift left 2)
*  inclusion in SATK ASAM)
***********************************************************************
         MACRO
         LXAF  &X2,&B2,&D2,&NEGD2
         LXA   64,&X2,&B2,&D2,&NEGD2
         MEND

***********************************************************************
* (pending  E365  LLXAF  - load logical indexed address (shift left 2)
*  inclusion in SATK ASAM)
***********************************************************************
         MACRO
         LLXAF &X2,&B2,&D2,&NEGD2
         LXA   65,&X2,&B2,&D2,&NEGD2
         MEND
                                                                 EJECT
***********************************************************************
* (pending  E366  LXAG  - load indexed address (shift left 3)
*  inclusion in SATK ASAM)
***********************************************************************
         MACRO
         LXAG  &X2,&B2,&D2,&NEGD2
         LXA   66,&X2,&B2,&D2,&NEGD2
         MEND

***********************************************************************
* (pending  E367  LLXAG  - load logical indexed address (shift left 3)
*  inclusion in SATK ASAM)
***********************************************************************
         MACRO
         LLXAG &X2,&B2,&D2,&NEGD2
         LXA   67,&X2,&B2,&D2,&NEGD2
         MEND
                                                                 EJECT
***********************************************************************
* (pending  E368  LXAQ  - load indexed address (shift left 4)
*  inclusion in SATK ASAM)
***********************************************************************
         MACRO
         LXAQ  &X2,&B2,&D2,&NEGD2
         LXA   68,&X2,&B2,&D2,&NEGD2
         MEND

***********************************************************************
* (pending  E369  LLXAQ  - load logical indexed address (shift left 4)
*  inclusion in SATK ASAM)
***********************************************************************
         MACRO
         LLXAQ &X2,&B2,&D2,&NEGD2
         LXA   69,&X2,&B2,&D2,&NEGD2
         MEND
                                                                 EJECT
***********************************************************************
* (pending LXA - load indexed address helper
*  inclusion in SATK ASAM)
*
*     LXA Macro to help build LLXA.. abs LXA.. instructions
*        LXA   &opcode,&x2,&b2,&dx2,&negd2
*
*        &opcode = last byte of lXA../LLXA.. instruction
*        &x2     = index register number 0-15
*        &b2     = base register number 0-15
*        &dx2    = 20-bit displacement (positive number)
*        &nexd2  = - if DX2 is negative
*
*        Note: R1 is a fixed register:
*                         r1 = 1;
***********************************************************************
         MACRO
         LXA   &OPCODE,&X2,&B2,&D2,&NEGD2
         LCLA  &DX2
         LCLA  &R1X2
         LCLA  &DXH2
         LCLA  &DXL2
         LCLA  &B2DXL2

.*        MNOTE *,'&&D2 == &D2'
.*      MNOTE *,'&&NEGD2 == &NEGD2'

         AIF   ('&NEGD2' EQ  '-').NEGDX2
.*        MNOTE *,'Positive D2'
.*        MNOTE *,'-----------'

.* positive DX2
&DX2     SETA  &D2
&R1X2    SETA  +((1*16)+&X2)

&DXH2    SETA  +(&DX2/4096)
&DXL2    SETA  +(&DX2-(&DXH2*4096))
&B2DXL2  SETA  +((4096*&B2)+&DXL2)
         AGO   .BUILD

.NEGDX2  ANOP
.*        MNOTE *,'Negative D2'
.*        MNOTE *,'-----------'

.* negative DX2
&DX2     SETA  &D2
&R1X2    SETA  +((1*16)+&X2)

         AIF   (&DX2 LE  4095).NEG4095
.*       dx2 is -4096 to ...
&DXH2    SETA  +(0-(&DX2/4096))
&DXL2    SETA  (0-(&DX2-((&DX2/4096)*4096)))
&B2DXL2  SETA  +((4096*&B2)+&DXL2)
.*         MNOTE *,'>>>> &&DXH2 == &DXH2'
.*         MNOTE *,'>>>> &&DXL2 == &DXL2'

         AGO   .BUILD
.*       dx2 is -1 to -4095
.NEG4095  ANOP
&DXH2    SETA  X'FF'
&DXL2    SETA  ((0-&DX2)-X'FFFFF000')  just the last 12 bits
&B2DXL2  SETA  +((4096*&B2)+&DXL2)
.*         MNOTE *,'>> &&DXH2 == &DXH2'
.*         MNOTE *,'>> &&DXL2 == &DXL2'

.BUILD   ANOP
.*         MNOTE *,'&&B2 == &B2'
.*         MNOTE *,'&&X2 == &X2'
.*         MNOTE *,'&&DX2 == &DX2'
.*         MNOTE *,'&&R1X22 == &R1X2'
.*         MNOTE *,'&&DXH2 == &DXH2'
.*         MNOTE *,'&&DXL2 == &DXL2'
.*         MNOTE *,'&&B2DXL2 == &B2DXL2'

         DS    0H                      LLXA../LLXA.  r1,&dx2(&x2,&b2)
         DC    XL1'E3'                 LLXA../LLXA.
         DC    HL1'&R1X2'              r1, x2
         DC    HL2'&B2DXL2'            b2, dxl2
         DC    HL1'&DXH2'              dxh2
         DC    XL1'&OPCODE'            LLXA../LLXA.. opcode

         MEND
                                                                EJECT
***********************************************************************
*        Low core PSWs
***********************************************************************
MIE4TST  START 0
         USING MIE4TST,R0            Low core addressability

SVOLDPSW EQU   MIE4TST+X'140'        z/Arch Supervisor call old PSW
                                                                SPACE 2
         ORG   MIE4TST+X'1A0'        z/Architecure RESTART PSW
         DC    X'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE 2
         ORG   MIE4TST+X'1D0'        z/Architecure PROGRAM CHECK PSW
         DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                SPACE 3
         ORG   MIE4TST+X'200'        Start of actual test program...
                                                                SPACE 2
***********************************************************************
*               The actual "MIE4TST" program itself...
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
*   R11      M4TEST call return
*   R12      M4TESTS register
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
* Is Miscellaneous-instruction-extensions facility 4  (bit 84)
***********************************************************************

         FCHECK 84,'Miscellaneous-instruction-extensions facility 4'
                                                                EJECT
***********************************************************************
*              Do tests in the M4TESTS table
***********************************************************************

         L     R12,=A(M4TESTS)       get table of test addresses

NEXTM4   EQU   *
         L     R5,0(0,R12)       get test address
         LTR   R5,R5                have a test?
         BZ    ENDTEST                 done?

         USING M4TEST,R5

         LH    R0,TNUM           save current test number
         ST    R0,TESTING        for easy reference

         LG    R1,R1FUDGE
         L     R11,TSUB          get address of test routine
         BALR  R11,R11           do test

         LGF   R1,READDR         get address of expected result
         CLC   R1OUTPUT,0(R1)    valid?
         BNE   FAILMSG              no, issue failed message

         LA    R12,4(0,R12)      next test address
         B     NEXTM4
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
         B     NEXTM4
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

         ORG   MIE4TST+X'1000'
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
*        pollution
***********************************************************************
         DS    0F
         DS    XL16                        gap
R1FUDGE  DC    CL16'FudgeFudgeFudge-'
         DS    XL16                        gap
                                                                EJECT
***********************************************************************
*        M4TEST DSECT
***********************************************************************
                                                                SPACE 2
M4TEST   DSECT ,
TSUB     DC    A(0)           pointer  to test
TNUM     DC    H'00'          Test Number
         DC    X'00'

OPNAME   DC    CL8' '         E7 name
X2ADDR   DC    A(0)           address of X2 source
B2ADDR   DC    A(0)           address of B2 source
RELEN    DC    A(0)           RESULT LENGTH
READDR   DC    A(0)           result (expected) address
         DS    FD                gap
R1OUTPUT DS    D              r1 Output
         DS    FD                gap

*        test routine will be here (from RXY-c macro)
*
*        followed by
*              EXPECTED RESULT
                                                                SPACE 2
MIE4TST  CSECT ,
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
         RXY_C &INST,&X2,&B2,&DX2,&NEGDX2
.*                               &INST   - RXY-c instruction under test
.*                               &X2     - RXY-c index register
.*                               &B2     - RXY-c base register
.*                               &DX2    - RXY-c 20-bit displacement
.*                               &NEGDX2 - RXY-c sign of DX2

.*         MNOTE *,'&&NEGDX2== &NEGDX2'

         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    X'00'
         DC    CL8'&INST'        instruction name
         DC    A(RE&TNUM+8)      address of R2 source
         DC    A(RE&TNUM+16)     address of R3 source
         DC    A(8)              result length
REA&TNUM DC    A(RE&TNUM)        result address
         DS    FD                gap
R1O&TNUM DS    D                 R1 output
         DS    FD                gap
.*
*
X&TNUM   DS    0F
         LG    R1,R1FUDGE

         XGR   &X2,&X2
         L     &X2,X2ADDR        load x2 source address
         LG    &X2,0(&X2)        load source

         XGR   &B2,&B2
         L     &B2,B2ADDR        load b2 source address
         LG    &B2,0(&B2)        load source

         &INST &X2,&B2,&DX2,&NEGDX2      test instruction
         STG   R1,R1O&TNUM       save R1 output

         BR    R11               return

RE&TNUM  DC    0F                xl8 expected result

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
*        MIE4  RXY-c tests
***********************************************************************
         PRINT DATA
         DS    FD
*
*   E360  LXAB  - load indexed address (shift left 0)          [RXY-c]
*   E361  LLXAB - load logical indexed address (shift left 0)  [RXY-c]
*   E362  LXAH  - load indexed address (shift left 1)          [RXY-c]
*   E363  LLXAH - load logical indexed address (shift left 1)  [RXY-c]
*   E364  LXAF  - load indexed address (shift left 2)          [RXY-c]
*   E365  LLXAF - load logical indexed address (shift left 2)  [RXY-c]
*   E366  LXAG  - load indexed address (shift left 3)          [RXY-c]
*   E367  LLXAG - load logical indexed address (shift left 3)  [RXY-c]
*   E368  LXAQ  - load indexed address (shift left 4)          [RXY-c]
*   E369  LLXAQ - load logical indexed address (shift left 4)  [RXY-c]
*
*        RXY-c instruction,x2 (reg), b2 (reg),
*                 dx2 (20-bit positive int), negdx2 (+ or -)
*              followed by
*                 8 byte expected result (R1)
*                 8 byte X2 source
*                 8 byte B2 source

*---------------------------------------------------------------------
*  LXAB  - load indexed address (shift left 0)
*---------------------------------------------------------------------
*
         RXY_C LXAB,0,0,1,+
         DC    XL8'0000 0000 0000 0001'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,0,0,4096,+
         DC    XL8'0000 0000 0000 1000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,0,0,1,-
         DC    XL8'FFFF FFFF FFFF FFFF'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,0,0,4096,-
         DC    XL8'FFFF FFFF FFFF F000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* X2 defined - positive
*
         RXY_C LXAB,3,0,0,+
         DC    XL8'0000 0000 0000 0100'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,0,1,+
         DC    XL8'0000 0000 0000 0101'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,0,4096,+
         DC    XL8'0000 0000 0000 1100'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,0,1,-
         DC    XL8'0000 0000 0000 00FF'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,0,4096,-
         DC    XL8'FFFF FFFF FFFF F100'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* X2 defined - negative
*
         RXY_C LXAB,3,0,0,+
         DC    XL8'FFFF FFFF FFFF FF00'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,0,1,+
         DC    XL8'FFFF FFFF FFFF FF01'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,0,4096,+
         DC    XL8'0000 0000 0000 0F00'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,0,1,-
         DC    XL8'FFFF FFFF FFFF FEFF'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,0,4096,-
         DC    XL8'FFFF FFFF FFFF EF00'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2
*
* B2 defined
*
         RXY_C LXAB,0,4,0,+
         DC    XL8'AAAA AAAA AAAA AAAA'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,0,4,1,+
         DC    XL8'AAAA AAAA AAAA AAAB'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,0,4,4096,+
         DC    XL8'AAAA AAAA AAAA BAAA'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,0,4,1,-
         DC    XL8'AAAA AAAA AAAA AAA9'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,0,4,4096,-
         DC    XL8'AAAA AAAA AAAA 9AAA'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - positive
*
         RXY_C LXAB,3,4,0,+
         DC    XL8'AAAA AAAA AAAA ABAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,4,1,+
         DC    XL8'AAAA AAAA AAAA ABAB'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,4,4096,+
         DC    XL8'AAAA AAAA AAAA BBAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,4,1,-
         DC    XL8'AAAA AAAA AAAA ABA9'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,4,4096,-
         DC    XL8'AAAA AAAA AAAA 9BAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - negative
*
         RXY_C LXAB,3,4,0,+
         DC    XL8'AAAA AAAA AAAA A9AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,4,1,+
         DC    XL8'AAAA AAAA AAAA A9AB'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,4,4096,+
         DC    XL8'AAAA AAAA AAAA B9AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,4,1,-
         DC    XL8'AAAA AAAA AAAA A9A9'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAB,3,4,4096,-
         DC    XL8'AAAA AAAA AAAA 99AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*---------------------------------------------------------------------
*  LLXAB - load logical indexed address (shift left 0)
*---------------------------------------------------------------------
*
         RXY_C LLXAB,0,0,1,+
         DC    XL8'0000 0000 0000 0001'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,0,0,4096,+
         DC    XL8'0000 0000 0000 1000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,0,0,1,-
         DC    XL8'0000 0000 FFFF FFFF'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,0,0,4096,-
         DC    XL8'0000 0000 FFFF F000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* X2 defined - positive
*
         RXY_C LLXAB,3,0,0,+
         DC    XL8'0000 0000 0000 0100'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,0,1,+
         DC    XL8'0000 0000 0000 0101'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,0,4096,+
         DC    XL8'0000 0000 0000 1100'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,0,1,-
         DC    XL8'0000 0000 0000 00FF'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,0,4096,-
         DC    XL8'0000 0000 FFFF F100'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* X2 defined - negative
*
         RXY_C LLXAB,3,0,0,+
         DC    XL8'0000 0000 FFFF FF00'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,0,1,+
         DC    XL8'0000 0000 FFFF FF01'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,0,4096,+
         DC    XL8'0000 0000 0000 0F00'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,0,1,-
         DC    XL8'0000 0000 FFFF FEFF'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,0,4096,-
         DC    XL8'0000 0000 FFFF EF00'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2
*
* B2 defined
*
         RXY_C LLXAB,0,4,0,+
         DC    XL8'AAAA AAAA AAAA AAAA'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,0,4,1,+
         DC    XL8'AAAA AAAA AAAA AAAB'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,0,4,4096,+
         DC    XL8'AAAA AAAA AAAA BAAA'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,0,4,1,-
         DC    XL8'AAAA AAAB AAAA AAA9'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,0,4,4096,-
         DC    XL8'AAAA AAAB AAAA 9AAA'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - positive
*
         RXY_C LLXAB,3,4,0,+
         DC    XL8'AAAA AAAA AAAA ABAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,4,1,+
         DC    XL8'AAAA AAAA AAAA ABAB'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,4,4096,+
         DC    XL8'AAAA AAAA AAAA BBAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,4,1,-
         DC    XL8'AAAA AAAA AAAA ABA9'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,4,4096,-
         DC    XL8'AAAA AAAB AAAA 9BAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - negative
*
         RXY_C LLXAB,3,4,0,+
         DC    XL8'AAAA AAAB AAAA A9AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,4,1,+
         DC    XL8'AAAA AAAB AAAA A9AB'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,4,4096,+
         DC    XL8'AAAA AAAA AAAA B9AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,4,1,-
         DC    XL8'AAAA AAAB AAAA A9A9'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAB,3,4,4096,-
         DC    XL8'AAAA AAAB AAAA 99AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*---------------------------------------------------------------------
*  LXAH  - load indexed address (shift left 1)
*---------------------------------------------------------------------
*
         RXY_C LXAH,0,0,1,+
         DC    XL8'0000 0000 0000 0002'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,0,0,4096,+
         DC    XL8'0000 0000 0000 2000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,0,0,1,-
         DC    XL8'FFFF FFFF FFFF FFFE'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,0,0,4096,-
         DC    XL8'FFFF FFFF FFFF E000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* X2 defined - positive
*
         RXY_C LXAH,3,0,0,+
         DC    XL8'0000 0000 0000 0200'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,0,1,+
         DC    XL8'0000 0000 0000 0202'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,0,4096,+
         DC    XL8'0000 0000 0000 2200'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,0,1,-
         DC    XL8'0000 0000 0000 01FE'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,0,4096,-
         DC    XL8'FFFF FFFF FFFF E200'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* X2 defined - negative
*
         RXY_C LXAH,3,0,0,+
         DC    XL8'FFFF FFFF FFFF FE00'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,0,1,+
         DC    XL8'FFFF FFFF FFFF FE02'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,0,4096,+
         DC    XL8'0000 0000 0000 1E00'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,0,1,-
         DC    XL8'FFFF FFFF FFFF FDFE'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,0,4096,-
         DC    XL8'FFFF FFFF FFFF DE00'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2
*
* B2 defined
*
         RXY_C LXAH,0,4,0,+
         DC    XL8'AAAA AAAA AAAA AAAA'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,0,4,1,+
         DC    XL8'AAAA AAAA AAAA AAAC'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,0,4,4096,+
         DC    XL8'AAAA AAAA AAAA CAAA'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,0,4,1,-
         DC    XL8'AAAA AAAA AAAA AAA8'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,0,4,4096,-
         DC    XL8'AAAA AAAA AAAA 8AAA'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - positive
*
         RXY_C LXAH,3,4,0,+
         DC    XL8'AAAA AAAA AAAA ACAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,4,1,+
         DC    XL8'AAAA AAAA AAAA ACAC'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,4,4096,+
         DC    XL8'AAAA AAAA AAAA CCAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,4,1,-
         DC    XL8'AAAA AAAA AAAA ACA8'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,4,4096,-
         DC    XL8'AAAA AAAA AAAA 8CAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - negative
*
         RXY_C LXAH,3,4,0,+
         DC    XL8'AAAA AAAA AAAA A8AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,4,1,+
         DC    XL8'AAAA AAAA AAAA A8AC'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,4,4096,+
         DC    XL8'AAAA AAAA AAAA C8AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,4,1,-
         DC    XL8'AAAA AAAA AAAA A8A8'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAH,3,4,4096,-
         DC    XL8'AAAA AAAA AAAA 88AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*---------------------------------------------------------------------
*  LLXAH - load logical indexed address (shift left 1)
*---------------------------------------------------------------------
*
         RXY_C LLXAH,0,0,1,+
         DC    XL8'0000 0000 0000 0002'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,0,0,4096,+
         DC    XL8'0000 0000 0000 2000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,0,0,1,-
         DC    XL8'0000 0001 FFFF FFFE'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,0,0,4096,-
         DC    XL8'0000 0001 FFFF E000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* X2 defined - positive
*
         RXY_C LLXAH,3,0,0,+
         DC    XL8'0000 0000 0000 0200'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,0,1,+
         DC    XL8'0000 0000 0000 0202'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,0,4096,+
         DC    XL8'0000 0000 0000 2200'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,0,1,-
         DC    XL8'0000 0000 0000 01FE'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,0,4096,-
         DC    XL8'0000 0001 FFFF E200'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* X2 defined - negative
*
         RXY_C LLXAH,3,0,0,+
         DC    XL8'0000 0001 FFFF FE00'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,0,1,+
         DC    XL8'0000 0001 FFFF FE02'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,0,4096,+
         DC    XL8'0000 0000 0000 1E00'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,0,1,-
         DC    XL8'0000 0001 FFFF FDFE'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,0,4096,-
         DC    XL8'0000 0001 FFFF DE00'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2
*
* B2 defined
*
         RXY_C LLXAH,0,4,0,+
         DC    XL8'AAAA AAAA AAAA AAAA'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,0,4,1,+
         DC    XL8'AAAA AAAA AAAA AAAC'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,0,4,4096,+
         DC    XL8'AAAA AAAA AAAA CAAA'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,0,4,1,-
         DC    XL8'AAAA AAAC AAAA AAA8'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,0,4,4096,-
         DC    XL8'AAAA AAAC AAAA 8AAA'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - positive
*
         RXY_C LLXAH,3,4,0,+
         DC    XL8'AAAA AAAA AAAA ACAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,4,1,+
         DC    XL8'AAAA AAAA AAAA ACAC'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,4,4096,+
         DC    XL8'AAAA AAAA AAAA CCAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,4,1,-
         DC    XL8'AAAA AAAA AAAA ACA8'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,4,4096,-
         DC    XL8'AAAA AAAC AAAA 8CAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - negative
*
         RXY_C LLXAH,3,4,0,+
         DC    XL8'AAAA AAAC AAAA A8AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,4,1,+
         DC    XL8'AAAA AAAC AAAA A8AC'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,4,4096,+
         DC    XL8'AAAA AAAA AAAA C8AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,4,1,-
         DC    XL8'AAAA AAAC AAAA A8A8'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAH,3,4,4096,-
         DC    XL8'AAAA AAAC AAAA 88AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*---------------------------------------------------------------------
*  LXAF  - load indexed address (shift left 2)
*---------------------------------------------------------------------
*
         RXY_C LXAF,0,0,1,+
         DC    XL8'0000 0000 0000 0004'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAF,0,0,4096,+
         DC    XL8'0000 0000 0000 4000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAF,0,0,1,-
         DC    XL8'FFFF FFFF FFFF FFFC'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAF,0,0,4096,-
         DC    XL8'FFFF FFFF FFFF C000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2
*
* B2 defined; X2 defined - positive
*
         RXY_C LXAF,3,4,0,+
         DC    XL8'AAAA AAAA AAAA AEAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAF,3,4,1,+
         DC    XL8'AAAA AAAA AAAA AEAE'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAF,3,4,4096,+
         DC    XL8'AAAA AAAA AAAA EEAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAF,3,4,1,-
         DC    XL8'AAAA AAAA AAAA AEA6'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAF,3,4,4096,-
         DC    XL8'AAAA AAAA AAAA 6EAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - negative
*
         RXY_C LXAF,3,4,0,+
         DC    XL8'AAAA AAAA AAAA A6AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAF,3,4,1,+
         DC    XL8'AAAA AAAA AAAA A6AE'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAF,3,4,4096,+
         DC    XL8'AAAA AAAA AAAA E6AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAF,3,4,1,-
         DC    XL8'AAAA AAAA AAAA A6A6'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAF,3,4,4096,-
         DC    XL8'AAAA AAAA AAAA 66AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*---------------------------------------------------------------------
*  LLXAF - load logical indexed address (shift left 2)
*---------------------------------------------------------------------
*
         RXY_C LLXAF,0,0,1,+
         DC    XL8'0000 0000 0000 0004'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAF,0,0,4096,+
         DC    XL8'0000 0000 0000 4000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAF,0,0,1,-
         DC    XL8'0000 0003 FFFF FFFC'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAF,0,0,4096,-
         DC    XL8'0000 0003 FFFF C000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - positive
*
         RXY_C LLXAF,3,4,0,+
         DC    XL8'AAAA AAAA AAAA AEAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAF,3,4,1,+
         DC    XL8'AAAA AAAA AAAA AEAE'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAF,3,4,4096,+
         DC    XL8'AAAA AAAA AAAA EEAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAF,3,4,1,-
         DC    XL8'AAAA AAAA AAAA AEA6'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAF,3,4,4096,-
         DC    XL8'AAAA AAAE AAAA 6EAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - negative
*
         RXY_C LLXAF,3,4,0,+
         DC    XL8'AAAA AAAE AAAA A6AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAF,3,4,1,+
         DC    XL8'AAAA AAAE AAAA A6AE'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAF,3,4,4096,+
         DC    XL8'AAAA AAAA AAAA E6AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAF,3,4,1,-
         DC    XL8'AAAA AAAE AAAA A6A6'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAF,3,4,4096,-
         DC    XL8'AAAA AAAE AAAA 66AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*---------------------------------------------------------------------
*  LXAG  - load indexed address (shift left 3)
*---------------------------------------------------------------------
*
         RXY_C LXAG,0,0,1,+
         DC    XL8'0000 0000 0000 0008'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAG,0,0,4096,+
         DC    XL8'0000 0000 0000 8000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAG,0,0,1,-
         DC    XL8'FFFF FFFF FFFF FFF8'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAG,0,0,4096,-
         DC    XL8'FFFF FFFF FFFF 8000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - positive
*
         RXY_C LXAG,3,4,0,+
         DC    XL8'AAAA AAAA AAAA B2AA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAG,3,4,1,+
         DC    XL8'AAAA AAAA AAAA B2B2'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAG,3,4,4096,+
         DC    XL8'AAAA AAAA AAAB 32AA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAG,3,4,1,-
         DC    XL8'AAAA AAAA AAAA B2A2'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAG,3,4,4096,-
         DC    XL8'AAAA AAAA AAAA 32AA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - negative
*
         RXY_C LXAG,3,4,0,+
         DC    XL8'AAAA AAAA AAAA A2AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAG,3,4,1,+
         DC    XL8'AAAA AAAA AAAA A2B2'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAG,3,4,4096,+
         DC    XL8'AAAA AAAA AAAB 22AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAG,3,4,1,-
         DC    XL8'AAAA AAAA AAAA A2A2'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAG,3,4,4096,-
         DC    XL8'AAAA AAAA AAAA 22AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*---------------------------------------------------------------------
*  LLXAG - load logical indexed address (shift left 3)
*---------------------------------------------------------------------
*
         RXY_C LLXAG,0,0,1,+
         DC    XL8'0000 0000 0000 0008'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAG,0,0,4096,+
         DC    XL8'0000 0000 0000 8000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAG,0,0,1,-
         DC    XL8'0000 0007 FFFF FFF8'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAG,0,0,4096,-
         DC    XL8'0000 0007 FFFF 8000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - positive
*
         RXY_C LLXAG,3,4,0,+
         DC    XL8'AAAA AAAA AAAA B2AA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAG,3,4,1,+
         DC    XL8'AAAA AAAA AAAA B2B2'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAG,3,4,4096,+
         DC    XL8'AAAA AAAA AAAB 32AA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAG,3,4,1,-
         DC    XL8'AAAA AAAA AAAA B2A2'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAG,3,4,4096,-
         DC    XL8'AAAA AAB2 AAAA 32AA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - negative
*
         RXY_C LLXAG,3,4,0,+
         DC    XL8'AAAA AAB2 AAAA A2AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAG,3,4,1,+
         DC    XL8'AAAA AAB2 AAAA A2B2'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAG,3,4,4096,+
         DC    XL8'AAAA AAAA AAAB 22AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAG,3,4,1,-
         DC    XL8'AAAA AAB2 AAAA A2A2'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAG,3,4,4096,-
         DC    XL8'AAAA AAB2 AAAA 22AA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*---------------------------------------------------------------------
*  LXAQ  - load indexed address (shift left 4)
*---------------------------------------------------------------------
*
         RXY_C LXAQ,0,0,1,+
         DC    XL8'0000 0000 0000 0010'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAQ,0,0,4096,+
         DC    XL8'0000 0000 0001 0000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAQ,0,0,1,-
         DC    XL8'FFFF FFFF FFFF FFF0'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAQ,0,0,4096,-
         DC    XL8'FFFF FFFF FFFF 0000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - positive
*
         RXY_C LXAQ,3,4,0,+
         DC    XL8'AAAA AAAA AAAA BAAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAQ,3,4,1,+
         DC    XL8'AAAA AAAA AAAA BABA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAQ,3,4,4096,+
         DC    XL8'AAAA AAAA AAAB BAAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAQ,3,4,1,-
         DC    XL8'AAAA AAAA AAAA BA9A'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAQ,3,4,4096,-
         DC    XL8'AAAA AAAA AAA9 BAAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - negative
*
         RXY_C LXAQ,3,4,0,+
         DC    XL8'AAAA AAAA AAAA 9AAA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAQ,3,4,1,+
         DC    XL8'AAAA AAAA AAAA 9ABA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAQ,3,4,4096,+
         DC    XL8'AAAA AAAA AAAB 9AAA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAQ,3,4,1,-
         DC    XL8'AAAA AAAA AAAA 9A9A'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LXAQ,3,4,4096,-
         DC    XL8'AAAA AAAA AAA9 9AAA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*---------------------------------------------------------------------
*  LLXAQ - load logical indexed address (shift left 4)
*---------------------------------------------------------------------
*
         RXY_C LLXAQ,0,0,1,+
         DC    XL8'0000 0000 0000 0010'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAQ,0,0,4096,+
         DC    XL8'0000 0000 0001 0000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAQ,0,0,1,-
         DC    XL8'0000 000F FFFF FFF0'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAQ,0,0,4096,-
         DC    XL8'0000 000F FFFF 0000'   result
         DC    XL8'AAAA AAAA AAAA AAAA'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - positive
*
         RXY_C LLXAQ,3,4,0,+
         DC    XL8'AAAA AAAA AAAA BAAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAQ,3,4,1,+
         DC    XL8'AAAA AAAA AAAA BABA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAQ,3,4,4096,+
         DC    XL8'AAAA AAAA AAAB BAAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAQ,3,4,1,-
         DC    XL8'AAAA AAAA AAAA BA9A'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAQ,3,4,4096,-
         DC    XL8'AAAA AABA AAA9 BAAA'   result
         DC    XL8'AAAA 0000 0000 0100'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

*
* B2 defined; X2 defined - negative
*
         RXY_C LLXAQ,3,4,0,+
         DC    XL8'AAAA AABA AAAA 9AAA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAQ,3,4,1,+
         DC    XL8'AAAA AABA AAAA 9ABA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAQ,3,4,4096,+
         DC    XL8'AAAA AAAA AAAB 9AAA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAQ,3,4,1,-
         DC    XL8'AAAA AABA AAAA 9A9A'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2

         RXY_C LLXAQ,3,4,4096,-
         DC    XL8'AAAA AABA AAA9 9AAA'   result
         DC    XL8'AAAA 0000 FFFF FF00'   X2
         DC    XL8'AAAA AAAA AAAA AAAA'   b2


         DC    F'0'     END OF TABLE
         DC    F'0'
                                                                 EJECT
*
* table of pointers to individual load test
*
M4TESTS  DS    0F
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
