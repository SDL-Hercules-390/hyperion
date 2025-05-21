 TITLE 'zvector-e7-99-performance'
***********************************************************************
*
* Zvector E7 instruction performance tests for 64 instructions
*
*        James Wekel May 2025
***********************************************************************

***********************************************************************
*
*  Example Hercules Testcase:
*
*  *Testcase zvector-e7-99-performance
*
*  #  -----------------------------------------------------------------
*  #  This ONLY tests the performance of the Zvector instructions.
*  #
*  #  The default is to NOT run performance tests. To enable
*  #  performance tests, uncomment the "#r           500=ff" line
*  #  below and swap the runtest commands.
*  #
*  #  The default test is 30,000,000 iterations of the zvector
*  #  instruction. The iteration count can be modified at address 504.
*  #  For example, "r           504=01312D00" would set the
*  #  iteraction count to 20,000,000.
*  #
*  #  Tests: 64 E7 zvector instructions
*  #
*  #  Output:
*  #  For each test, a console line will the generated with timing
*  #  results, as follows:
*  #
*  #  Test # 001: 30,000,000 iterations of "VLREPB V1,0(R5)"
*  #              took     503,528 microseconds
*  #  Test # 002: 30,000,000 iterations of "VESL   V1,V2,3,0"
*  #              took     145,584 microseconds
*  #  Test # 003: 30,000,000 iterations of "VERLL  V1,V2,3,0"
*  #              took     163,143 microseconds
*  #  Test # 004: 30,000,000 iterations of "VESRL  V1,V2,3,0"
*  #              took     138,747 microseconds
*  #  Test # 005: 30,000,000 iterations of "VESRA  V1,V2,3,0"
*  #              took     153,151 microseconds
*  #  ...
*  #
*  #  Note:
*  #     this test requires diagnose 'F14' for OS microsecond timing.
*  #  -----------------------------------------------------------------
*  mainsize    2
*  numcpu      1
*  sysclear
*  archlvl     z/Arch
*  diag8cmd    enable    # (needed for messages to Hercules console)
*
*  loadcore    "$(testpath)/zvector-e7-99-performance.core" 0x0
*
*  #r           500=ff          # (enable timing tests)
*  #r           504=01312D00    # (20,000,000 iteration count for test)
*  #runtest     120             # (TIMING test duration)
*
*  runtest      0.1       # (NOP TEST)
*
*  diag8cmd    disable   # (reset back to default)
*  *Done
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
PGCK     DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                SPACE 3
         ORG   ZVE7TST+X'200'        Start of actual test program...
                                                                 EJECT
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

         CLI   TIMEOPT,X'FF'    Is this a timing run?
         BNE   EOJ              Not a timing run; just end normally

         STCTL R0,R0,CTLR0      Store CR0 to enable AFP
         OI    CTLR0+1,X'04'    Turn on AFP bit
         OI    CTLR0+1,X'02'    Turn on Vector bit
         LCTL  R0,R0,CTLR0      Reload updated CR0
                                                                 EJECT
***********************************************************************
* Is z/Architecture vector facility installed  (bit 129)
***********************************************************************

         FCHECK 129,'z/Architecture vector facility'
                                                                 EJECT
***********************************************************************
* Is z/Architecture vector-enhancements facility 1 installed  (bit 135)
***********************************************************************

         FCHECK 135,'vector-enhancements facility 1'
                                                                 EJECT
***********************************************************************
* Is z/Architecture vector-enhancements facility 2 installed  (bit 148)
***********************************************************************

         FCHECK 148,'vector-enhancements facility 2'
                                                                 EJECT
***********************************************************************
* Is Diagnose F14: Hercules Get Microsecond Time available?
***********************************************************************
         LG    R1,=D'-1'               fudge R1

         MVC   PGCK(16),SKPWS          PG CK ==> skip message
         DC    X'83',X'12',X'0F14'     Issue Hercules Diagnose X'F14'
         MVC   PGCK(16),PGCKDEAD        restore PG CK ==> DEAD

         CG    R1,=D'-1'               did we get a microsecond time?
         BNE   DOTEST                  OK, Diagnode x'F14' is available
*
*                                      Not available message
SKF14MSG LA    R0,SKLF14               message length
         LA    R1,SKF14                message address
         BAL   R2,MSG

         B     EOJ
*                                      skip messgae
SKF14    DC    C'    Skipping tests: '
         DC    C'Diagnose F14: Hercules Get Microsecond Time'
         DC    C' is not available.'
SKLF14   EQU   *-SKF14

*                                      PROGRAM CHECK: skip message
         DS    0D
SKPWS    DC    X'0000000180000000'
         DC    AD(SKF14MSG)
                                                                SPACE 2
*                                      PROGRAM CHECK: DEAD
         DS    0D
PGCKDEAD DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                EJECT
*
**       Run the performance tests...
*
DOTEST   DS    0F
         BAL   R14,TEST91       Time instructions  (speed test)

***********************************************************************
*         Test for normal or unexpected test completion...
***********************************************************************

         CLI   TIMEOPT,X'FF'    Was this a timing run?
         BNE   EOJ              No, timing run; just go end normally

         L     R1,=A(TTBLNUM)   Get number of tests
         L     R1,0(R1)

         L     R2,TESTING
         CR    R1,R2            Did we end on last performance test?
         BNE   FAILTEST         No?! Then FAIL the test!

         B     EOJ              Yes, then normal completion!
                                                                SPACE 4
***********************************************************************
*        Fixed test storage locations ...
***********************************************************************
                                                               SPACE 2
         ORG   BEGIN+X'400'

TIMEOPT  DC    X'00'            Set to non-zero to run timing tests
         DC    X'00'            gap
ITERCNT  DC    F'30000000'      intstruction test iterations
*
FAILED   DC    F'0'              some test failed?
TESTING  DC    F'0'              current test number

                                                               SPACE 2
         DS    0D
SAVE3T5  DC    4F'0'
SAVER2   DC    F'0'
SAVER5   DC    F'0'

                                                               SPACE 2
         ORG   *+X'100'
                                                                EJECT
***********************************************************************
*        TEST91                 Time zvector instruction  (speed test)
***********************************************************************

TEST91   DS    0F
         L     R12,=A(E7TESTS)   get table of test addresses

***********************************************************************
*        Next, time the tests...
***********************************************************************

NEXTE7   EQU   *
         L     R5,0(0,R12)       get test address
         LTR   R5,R5                have a test?
         BZR   R14                    done?

         USING E7TEST,R5

         LH    R0,TNUM            save current test number
         ST    R0,TESTING         for easy reference
*
*        Initialize operand data  (move source to testing address)
*
         LGF   R1,V2ADDR          load v2 source
         VL    v2,0(R1)

         LGF   R1,V3ADDR          load v3 source
         VL    v3,0(R1)

         LGF   R1,V4ADDR          load v4 source
         VL    v4,0(R1)

***********************************************************************
*        Time the overhead...
***********************************************************************
         L     R7,ITERCNT

         DC    X'83',X'12',X'0F14'     Issue Hercules Diagnose X'F14'
         STG   R1,MSBEGCLK

OHLOOP   EQU  *
         BCT   R7,OHLOOP

         DC    X'83',X'12',X'0F14'     Issue Hercules Diagnose X'F14'
         STG   R1,MSENDCLK

         SG    R1,MSBEGCLK
         STG   R1,MSOVER
                                                                 EJECT
***********************************************************************
*        Now do the actual timing TESTS...
***********************************************************************

         L     R7,ITERCNT              iteration count

         L     R1,INADDR               move instruction for test
         MVC   TSTINSTR(6),0(R1)

         DC    X'83',X'12',X'0F14'     Issue Hercules Diagnose X'F14'
         STG   R1,MSBEGCLK
*
TSTLOOP  EQU   *

TSTINSTR DS    6C                  instruction under performance test

         BCT   R7,TSTLOOP

         DC    X'83',X'12',X'0F14'     Issue Hercules Diagnose X'F14'
         STG   R1,MSENDCLK
         SG    R1,MSBEGCLK
         STG   R1,MSDUR
                                                              SPACE 2
         BAL   R15,RPTSPEED
*
*  more performance tests?
*
DONEXT   ORG   *
         LA    R12,4(0,R12)            next test address
         B     NEXTE7
                                                                EJECT
***********************************************************************
*        RPTSPEED                 Report instruction speed
***********************************************************************
                                                               SPACE
RPTSPEED ST    R15,RPTSAVE             Save return address
         STM   R5,R13,RSAV5T13         Save R5 to R13
*
         L     R2,TESTING              get test number and convert
         CVD   R2,DECNUM
         MVC   PTNUM,EDITTNUM
         ED    PTNUM,DECNUM
         MVC   PRTNUM(3),PTNUM+L'PTNUM-3 fill in message with test #

         L     R2,ITERCNT              get iteration count
         CVD   R2,DECNUM
         MVC   PITER,EDITITER
         ED    PITER,DECNUM
         MVC   PRTITR(10),PITER+L'PITER-10     fill in iteration count

         L     R2,CMTADDR
         MVC   PRTCMT(24),0(R2)
*

         LG    R1,MSDUR
         SG    R1,MSOVER
         CVD   R1,MSAAA

         MVC   PMS,EDITMS
         ED    PMS,MSAAA
         MVC   PRTMS(L'PRTMS),PMS+L'PMS-L'PRTMS
                                                               SPACE 2
*
*        Use Hercules Diagnose for Message to console
*
         STM   R0,R2,RPTDWSAV          save regs used by MSG
         LA    R0,PRTLNG               message length
         LA    R1,PRTLINE              messagfe address
         BAL   R2,MSG                  call console MSG display
         LM    R0,R2,RPTDWSAV          restore regs
                                                               SPACE 2
         LM    R5,R13,RSAV5T13
         L     R15,RPTSAVE             Restore return address
         BR    R15                     Return to caller
                                                               SPACE
RPTSAVE  DC    F'0'                    R15 save area
RPTSVR5  DC    F'0'                    R5 save area
                                                               SPACE
RPTDWSAV DC    2D'0'                   R0-R2 save area for MSG call
RSAV5T13 DC    10F'0'
                                                               EJECT
***********************************************************************
*        Issue HERCULES MESSAGE pointed to by R1, length in R0
*              R2 = return address
***********************************************************************
                                                                SPACE
MSG      CH    R0,=H'0'                Do we even HAVE a message?
         BNHR  R2                      No, ignore
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
         DC    H'0'                    CRASH for debugging purposes
                                                                SPACE
MSGRET   LM    R0,R2,MSGSAVE           Restore registers
         BR    R2                      Return to caller
                                                                SPACE 2
MSGSAVE  DC    3F'0'                   Registers save area
MSGMVC   MVC   MSGMSG(0),0(R1)         Executed instruction
                                                                SPACE 2
MSGCMD   DC    C'MSGNOH * '            *** HERCULES MESSAGE COMMAND ***
MSGMSG   DC    CL95' '                 The message text to be displayed
                                                                EJECT
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 3
EOJPSW   DC    0D'0',X'0002000180000000',AD(0)
                                                                SPACE
EOJ      LPSWE EOJPSW                  Normal completion
                                                                SPACE 3
FAILPSW  DC    0D'0',X'0002000180000000',AD(X'BAD')
                                                                SPACE
FAILTEST LPSWE FAILPSW                 Abnormal termination
                                                                EJECT
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE 2
CTLR0    DS    F                CR0

                                                                SPACE 2
         LTORG ,                Literals pool
                                                                SPACE
K        EQU   1024             One KB
PAGE     EQU   (4*K)            Size of one page
K16      EQU   (16*K)           16 KB
K32      EQU   (32*K)           32 KB
K64      EQU   (64*K)           64 KB
MB       EQU   (K*K)             1 MB
                                                                SPACE

MSOVER   DC    D'0'             MS Overhead
MSBEGCLK DC    D'0'             MS Begin
MSENDCLK DC    D'0'             MS End
MSDUR    DC    D'0'             MS Duration (diff)
         DC    D'0',8X'AA'      gap
MSAAA    DC    PL8'0'           MS packed

PRTLINE  DC    C' Test # '
PRTNUM   DC    C'xxx'
         DC    C': '
PRTITR   DC    C'99,999,999'
         DC    C' iterations of '
PRTCMT   DC    CL24' '
         DC    C' took '
PRTMS    DC    C'999,999,999'
         DC    C' microseconds'
PRTLNG   EQU   *-PRTLINE
*
* edit fields for message line
*
EDITTNUM DC    XL16'40212020202020202020202020202020'
         DS    0D
         DC    C'===>'
PTNUM    DC    CL16' '
         DC    C'<==='

EDITITER DC    XL18'402020202020202020206B2020206B202120'
         DS    0D
         DC    C'===>'
PITER    DC    CL18' '
         DC    C'<==='

EDITMS   DC    Xl18'402020202020202020206B2020206B202120'
         DS    0D
         DC    C'===>'
PMS      DC    CL18' '
         DC    C'<==='

DECNUM   DS    CL21

V1ADDR   DC    A(V1FUDGE)
V1FUDGE  DC    XL16'3030303030320A6D 6E745F69643A0931'
                                                                EJECT
***********************************************************************
*        E7TEST DSECT
***********************************************************************
                                                                SPACE 2
E7TEST   DSECT ,
TSUB     DC    A(0)           pointer  to test
TNUM     DC    H'00'          Test Number
         DC    H'00'
LCMT     DC    H'00'          length of comment
CMTADDR  DC    A(0)           address of comment
INADDR   DC    A(0)           address of instruction under test
V2ADDR   DC    A(0)           address of v2 source
V3ADDR   DC    A(0)           address of v3 source
V4ADDR   DC    A(0)           address of v4 source


*        test routine will be here
*
*        followed by
*              EXPECTED RESULT
                                                                SPACE 2
ZVE7TST  CSECT ,
         DS    0F
                                                                EJECT
***********************************************************************
*     Macros to help build test tables
***********************************************************************

* --------------------------------------------------------------------
* PTEST: macro to generate individual test
* --------------------------------------------------------------------
         MACRO
         PTEST &CMT
.*                               &CMT  - messgae comment to identify
.*                                       test
         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

&C       SETC  '&CMT'
&LCMT    SETA  K'&C

         DS    0FD
         USING *,R5              base for test data and test routine

T&TNUM   DC    A(X&TNUM)         address of test routine
         DC    H'&TNUM'          test number
         DC    H'00'

         DC    H'&LCMT'          length of comment
         DC    A(CMT&TNUM)       address of comment
         DC    A(IN&TNUM)        address of instruction
         DC    A(IN&TNUM+6)      address of v2 source
         DC    A(IN&TNUM+22)     address of v3 source
         DC    A(IN&TNUM+38)     address of v4
.*
*
X&TNUM   DS    0F
.*
CMT&TNUM DC    CL24&C
.*                                 is comment longer than 24 charaters
         AIF   (&LCMT LE 26).SKWN  note: length includes enclosing 's
         MNOTE 4,'Comment truncated to 24 characters'
.SKWN    ANOP

IN&TNUM  DC    0F            zvector instruction for performance test

         DROP  R5
         MEND
                                                                EJECT
* --------------------------------------------------------------------
* PTTABLE: macro to generate table of pointers to individual tests
* --------------------------------------------------------------------
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

TTBLNUM  DC    F'&TNUM'
         DC    A(0)
.*
         MEND
                                                                 EJECT
***********************************************************************
*        Performance tests
***********************************************************************
         PRINT DATA
         DS    FD
*
*        PTEST comment
*              followed by
*                  6 byte vector instruction to test performance
*                 16 byte V2 source
*                 16 byte V3 source
*                 16 byte V4 source

*---------------------------------------------------------------------
* E705 VLREP  - Vector Load and Replicate
*---------------------------------------------------------------------

         PTEST '"VLREPB V1,0(R5)"'
         VLREPB  V1,0(R5)
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E730 VESL   - Vector Element Shift Left
*---------------------------------------------------------------------

         PTEST '"VESL   V1,V2,3,0"'
         VESL  V1,V2,3,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E733 VERLL  - Vector Element Rotate Left Logical
*---------------------------------------------------------------------

         PTEST '"VERLL  V1,V2,3,0"'
         VERLL  V1,V2,3,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E738 VESRL  - Vector Element Shift Right Logical
*---------------------------------------------------------------------

         PTEST '"VESRL  V1,V2,3,0"'
         VESRL  V1,V2,3,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E73A VESRA  - Vector Element Shift Right Arithmetic
*---------------------------------------------------------------------

         PTEST '"VESRA  V1,V2,3,0"'
         VESRA  V1,V2,3,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E744 VGBM   - Vector Generate Byte Mask
*---------------------------------------------------------------------

         PTEST '"VGBM   V1,170"'
         VGBM  V1,170
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E745 VREPI  - Vector Replicate Immediate
*---------------------------------------------------------------------

         PTEST '"VREPI  V1,170,0"'
         VREPI  V1,170,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E746 VGM    - Vector Generate Mask
*---------------------------------------------------------------------

         PTEST '"VGM    V1,2,4,0"'
         VGM   V1,2,4,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E74D VREP   - Vector Replicate
*---------------------------------------------------------------------

         PTEST '"VREP   V1,V2,4,0"'
         VREP   V1,V2,4,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E75C VISTR  - Vector Isolate String
*---------------------------------------------------------------------

         PTEST '"VISTR  V1,V2,0"'
         VISTR   V1,V2,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E75F VSEG   - Vector Sign Extend To Doubleword
*---------------------------------------------------------------------

         PTEST '"VSEG   V1,V2,0"'
         VSEG   V1,V2,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E760 VMRL   - Vector Merge Low
*---------------------------------------------------------------------

         PTEST '"VMRL   V1,V2,V3,0"'
         VMRL   V1,V2,V3,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E761 VMRH   - Vector Merge High
*---------------------------------------------------------------------

         PTEST '"VMRH   V1,V2,V3,0"'
         VMRH   V1,V2,V3,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E764 VSUM   - Vector Sum Across Word
*---------------------------------------------------------------------

         PTEST '"VSUM   V1,V2,V3,0"'
         VSUM   V1,V2,V3,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E765 VSUMG  - Vector Sum Across Doubleword
*---------------------------------------------------------------------

         PTEST '"VSUMG  V1,V2,V3,1"'
         VSUMG  V1,V2,V3,1
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E770 VESLV  - Vector Element Shift Left Vector
*---------------------------------------------------------------------

         PTEST '"VESLV  V1,V2,V3,0"'
         VESLV  V1,V2,V3,0
         DC    XL16'0101010101010101 0101010101010101'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E772 VERIM  - Vector Element Rotate and Insert Under Mask
*---------------------------------------------------------------------

         PTEST '"VERIM  V1,V2,V3,V4,0"'
         VERIM  V1,V2,V3,V4,0
         DC    XL16'0101010101010101 0101010101010101'   v2
         DC    XL16'0001020304050607 08090A0B0C0D0E0F'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E775 VSLB   - Vector Shift Left By Byte
*---------------------------------------------------------------------

         PTEST '"VSLB   V1,V2,V3"'
         VSLB   V1,V2,V3
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'0000000000000002 0000000000000000'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E777 VSLDB  - Vector Shift Left Double By Byte
*---------------------------------------------------------------------

         PTEST '"VSLDB  V1,V2,V3,5"'
         VSLDB  V1,V2,V3,5
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'0000000000000002 0000000000000000'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E77D VSRLB  - Vector Shift Right Logical By Byte
*---------------------------------------------------------------------

         PTEST '"VSRLB  V1,V2,V3"'
         VSRLB  V1,V2,V3
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'0000000000000002 0000000000000000'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E77F VSRAB  - Vector Shift Right Arithmetic By Byte
*---------------------------------------------------------------------

         PTEST '"VSRAB  V1,V2,V3"'
         VSRAB  V1,V2,V3
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'0000000000000002 0000000000000000'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E780 VFEE   - Vector Find Element Equal
*---------------------------------------------------------------------

         PTEST '"VFEE   V1,V2,V3,0"'
         VFEE   V1,V2,V3,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'0000000000000002 00000000000000AA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E781 VFENE  - Vector Find Element Not Equal
*---------------------------------------------------------------------

         PTEST '"VFENE  V1,V2,V3,0"'
         VFENE  V1,V2,V3,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAABB'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E782 VFAE   - Vector Find Any Element Equal
*---------------------------------------------------------------------

         PTEST '"VFAE   V1,V2,V3,0"'
         VFAE   V1,V2,V3,0
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAABB'   v2
         DC    XL16'0000000000000002 00000000000000BB'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E785 VBPERM - Vector Bit Permute
*---------------------------------------------------------------------

         PTEST '"VBPERM V1,V2,V3"'
         VBPERM V1,V2,V3
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'0000000000000002 0000000000000000'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* VSTRS  - Vector String Search
*---------------------------------------------------------------------

         PTEST '"VSTRS  V1,V2,V3,V4,0,2"'
         VSTRS  V1,V2,V3,V4,0,2
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E78C VPERM  - Vector Permute
*---------------------------------------------------------------------

         PTEST '"VPERM  V1,V2,V3,V4"'
         VPERM  V1,V2,V3,v4
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E794 VPK    - Vector Pack
*---------------------------------------------------------------------

         PTEST '"VPK    V1,V2,V3,1"'
         VPK    V1,V2,V3,1
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E795 VPKLS  - Vector Pack Logical Saturate
*---------------------------------------------------------------------

         PTEST '"VPKLS  V1,V2,V3,1,2"'
         VPKLS  V1,V2,V3,1,2
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E797 VPKS   - Vector Pack Saturate
*---------------------------------------------------------------------

         PTEST '"VPKS   V1,V2,V3,1,2"'
         VPKS   V1,V2,V3,1,2
         DC    XL16'AAAAAAAAAAAAAAAA AAAAAAAAAAAAAAAA'   v2
         DC    XL16'AABBBBAAAABBAAAA BBAAAABBAAAABBAA'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E7A1 VMLH   - Vector Multiply Logical High
*---------------------------------------------------------------------

         PTEST '"VMLH   V1,V2,V3,0"'
         VMLH   V1,V2,V3,0
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E7A2 VML    - Vector Multiply Low
*---------------------------------------------------------------------

         PTEST '"VML    V1,V2,V3,0"'
         VML    V1,V2,V3,0
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E7A3 VMH    - Vector Multiply High
*---------------------------------------------------------------------

         PTEST '"VMH    V1,V2,V3,0"'
         VMH    V1,V2,V3,0
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E7A4 VMLE   - Vector Multiply Logical Even
*---------------------------------------------------------------------

         PTEST '"VMLE   V1,V2,V3,0"'
         VMLE   V1,V2,V3,0
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E7A5 VMLO   - Vector Multiply Logical Odd
*---------------------------------------------------------------------

         PTEST '"VMLO   V1,V2,V3,0"'
         VMLO   V1,V2,V3,0
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E7A6 VME    - Vector Multiply Even
*---------------------------------------------------------------------

         PTEST '"VME    V1,V2,V3,0"'
         VME    V1,V2,V3,0
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E7A7 VMO    - Vector Multiply Odd
*---------------------------------------------------------------------

         PTEST '"VMO    V1,V2,V3,0"'
         VMO    V1,V2,V3,0
         DC    XL16'FF02030405060750 090A0B780C0D0EFD'   v2
         DC    XL16'FF02030405060750 090A0B780D0E0FFD'   v3
         DC    XL16'0000000000000002 0000000000000000'   v4

*---------------------------------------------------------------------
* E7A9 VMALH  - Vector Multiply and Add Logical High
*---------------------------------------------------------------------

         PTEST '"VMALH  V1,V2,V3,V4,0"'
         VMALH  V1,V2,V3,v4,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7AA VMAL   - Vector Multiply and Add Low
*---------------------------------------------------------------------

         PTEST '"VMAL   V1,V2,V3,V4,0"'
         VMAL   V1,V2,V3,v4,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7AB VMAH   - Vector Multiply and Add High
*---------------------------------------------------------------------

         PTEST '"VMAH   V1,V2,V3,V4,0"'
         VMAH   V1,V2,V3,v4,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7AC VMALE  - Vector Multiply and Add Logical Even
*---------------------------------------------------------------------

         PTEST '"VMALE  V1,V2,V3,V4,0"'
         VMALE  V1,V2,V3,v4,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7AD VMALO  - Vector Multiply and Add Logical Odd
*---------------------------------------------------------------------

         PTEST '"VMALE  V1,V2,V3,V4,0"'
         VMALE  V1,V2,V3,v4,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
*  E7AE VMAE   - Vector Multiply and Add Even
*---------------------------------------------------------------------

         PTEST '"VMAE   V1,V2,V3,V4,0"'
         VMAE   V1,V2,V3,v4,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
*  E7AF VMAO   - Vector Multiply and Add Odd
*---------------------------------------------------------------------

         PTEST '"VMAO   V1,V2,V3,V4,0"'
         VMAO   V1,V2,V3,v4,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7B4 VGFM   - Vector Galois Field Multiply Sum
*---------------------------------------------------------------------

         PTEST '"VGFM   V1,V2,V3,0"'
         VGFM   V1,V2,V3,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7BC VGFMA  - Vector Galois Field Multiply Sum and Accumulate
*---------------------------------------------------------------------

         PTEST '"VGFMA  V1,V2,V3,V4,0"'
         VGFMA  V1,V2,V3,V4,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7D4 VUPLL  - Vector Unpack Logical Low
*---------------------------------------------------------------------

         PTEST '"VUPLL  V1,V2,0"'
         VUPLL  V1,V2,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7D5 VUPLH  - Vector Unpack Logical High
*---------------------------------------------------------------------

         PTEST '"VUPLH  V1,V2,0"'
         VUPLH  V1,V2,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7D6 VUPL   - Vector Unpack Low
*---------------------------------------------------------------------

         PTEST '"VUPL   V1,V2,0"'
         VUPL   V1,V2,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7D7 VUPH   - Vector Unpack High
*---------------------------------------------------------------------

         PTEST '"VUPH   V1,V2,0"'
         VUPH   V1,V2,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7DE VLC    - Vector Load Complement
*---------------------------------------------------------------------

         PTEST '"VLC    V1,V2,0"'
         VLC    V1,V2,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7DF VLP    - Vector Load Positive
*---------------------------------------------------------------------

         PTEST '"VLP    V1,V2,0"'
         VLP    V1,V2,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7F0 VAVGL  - Vector Average Logical
*---------------------------------------------------------------------

         PTEST '"VAVGL  V1,V2,V3,0"'
         VAVGL  V1,V2,V3,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7F2 VAVG   - Vector Average
*---------------------------------------------------------------------

         PTEST '"VAVG   V1,V2,V3,0"'
         VAVG   V1,V2,V3,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7F3 VA     - Vector Add
*---------------------------------------------------------------------

         PTEST '"VA     V1,V2,V3,0"'
         VA     V1,V2,V3,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7F5 VSCBI  - Vector Subtract Compute Borrow Indication
*---------------------------------------------------------------------

         PTEST '"VSCBI  V1,V2,V3,0"'
         VSCBI  V1,V2,V3,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7F7 VS     - Vector Subtract
*---------------------------------------------------------------------

         PTEST '"VS     V1,V2,V3,0"'
         VS     V1,V2,V3,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7F8 VCEQ   - Vector Compare Equal
*---------------------------------------------------------------------

         PTEST '"VCEQ   V1,V2,V3,0,1"'
         VCEQ   V1,V2,V3,0,1
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7F9 VCHL   - Vector Compare High Logical
*---------------------------------------------------------------------

         PTEST '"VCHL   V1,V2,V3,0,1"'
         VCHL   V1,V2,V3,0,1
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7FB VCH    - Vector Compare High
*---------------------------------------------------------------------

         PTEST '"VCH    V1,V2,V3,0,1"'
         VCH    V1,V2,V3,0,1
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7FC VMNL   - Vector Minimum Logical
*---------------------------------------------------------------------

         PTEST '"VMNL   V1,V2,V3,0"'
         VMNL   V1,V2,V3,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7FD VMXL   - Vector Maximum Logical
*---------------------------------------------------------------------

         PTEST '"VMXL   V1,V2,V3,0"'
         VMXL   V1,V2,V3,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7FE VMN    - Vector Minimum
*---------------------------------------------------------------------

         PTEST '"VMN    V1,V2,V3,0"'
         VMN    V1,V2,V3,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4

*---------------------------------------------------------------------
* E7FF VMX    - Vector Maximum
*---------------------------------------------------------------------

         PTEST '"VMX    V1,V2,V3,0"'
         VMX    V1,V2,V3,0
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v2
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v3
         DC    XL16'FF02030405060708 090A0B0C0D0E0F10'   v4





         DC    F'0'     END OF TABLE
         DC    F'0'
                                                                EJECT
*
* table of pointers to individual load test
*
E7TESTS  DS    0F
         PTTABLE
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