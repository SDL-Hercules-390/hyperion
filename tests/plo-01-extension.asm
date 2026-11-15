 TITLE 'plo-01-extension'
***********************************************************************
*
*        PLO-Extension Facility function tests
*
*        James Wekel November 2026
***********************************************************************
                                                                SPACE 2
***********************************************************************
*
*        basic instruction tests
*
***********************************************************
*  This program tests proper functioning of the PLO-Extension
*  facility functions. Specification exceptions and access-register
*  mode are not tested.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
*----------------------------------------------------------------------
* Programmer's Note:
*
* The following tests have a LOT of duplicate (copy/paste) code
* to allow for minor modifications specific to each test.
*----------------------------------------------------------------------
*
***********************************************************************
*
*  Example Hercules Testcase:
*
*    *Testcase plo-01-extension
*    *
*    *   PLO-Extension Functions Facility tests
*    *
*    *   # ------------------------------------------------------------
*    *   #  This tests only the basic function of the instruction.
*    *   #  Exceptions are NOT tested.
*    *   # ------------------------------------------------------------
*    *
*    mainsize    2
*    numcpu      1
*    sysclear
*    archlvl     z/Arch
*
*    loadcore    "$(testpath)/plo-01-extension.core" 0x0
*
*    diag8cmd    enable    # (needed for messages to Hercules console)
*    runtest     5         #
*    diag8cmd    disable   # (reset back to default)
*
*    *Done
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
         LCLA  &FBBYTE             Facility bit in Byte
         LCLA  &FBBIT              Facility bit within Byte

         LCLA  &L(8)
&L(1)    SetA  128,64,32,16,8,4,2,1  bit positions within byte

&FBBYTE  SETA  &BITNO/8
&FBBIT   SETA  &L((&BITNO-(&FBBYTE*8))+1)
.*       MNOTE 0,'checking Bit=&BITNO: FBBYTE=&FBBYTE, FBBIT=&FBBIT'

         B     X&SYSNDX
*                                  Fcheck data area
*                                  skip messgae
SKT&SYSNDX DC  C'          Skipping tests: '
         DC    C&NOTSETMSG
         DC    C' facility (bit &BITNO) is not installed.'
SKL&SYSNDX EQU *-SKT&SYSNDX
*                                  facility bits
         DS    FD                  gap
FB&SYSNDX DS   4FD
         DS    FD                  gap
*
X&SYSNDX EQU *
         LA    R0,((X&SYSNDX-FB&SYSNDX)/8)-1
         STFLE FB&SYSNDX           get facility bits

         XGR   R0,R0
         IC    R0,FB&SYSNDX+&FBBYTE     get fbit byte
         N     R0,=F'&FBBIT'            is bit set?
         BNZ   XC&SYSNDX
*
* facility bit not set, issue message and exit
*
         LA    R0,SKL&SYSNDX       message length
         LA    R1,SKT&SYSNDX       message address
         BAL   R2,MSG

         B     EOJ
XC&SYSNDX EQU *
         MEND

                                                                EJECT
* =====================================================================
* LOW CORE
* =====================================================================
*
PLOETST  START 0
A000LABL EQU   *
         USING PLOETST,R0          Low core addressability

SVOLDPSW EQU   PLOETST+X'140'      z/Arch Supervisor call old PSW
                                                                SPACE 2
         ORG   PLOETST+X'1A0'      z/Architecure RESTART PSW
         DC    X'0000000180000000'
         DC    AD(BEGIN)
                                                                SPACE 2
         ORG   PLOETST+X'1D0'      z/Architecure PROGRAM CHECK PSW
         DC    X'0002000180000000'
         DC    AD(X'DEAD')
                                                                EJECT
***********************************************************************
*               The actual "PLOETST" program itself...
***********************************************************************
*
*  Architecture Mode: z/Arch
*  Register Usage:
*
*   R0-R4    (work)
*   R5       Testing control table - base current entry
*   R6-R7    (work)
*   R8       First base register
*   R9       Second base register
*   R10      Third base register
*   R11-R13  (work)
*   R14      Subroutine call return
*   R15      Secondary Subroutine call return or work
*
***********************************************************************
                                                                SPACE 2
         ORG   PLOETST+X'2000'      Start of actual test program...
                                                                SPACE
         USING  BEGIN,R8           FIRST Base Register
         USING  BEGIN+4096,R9      SECOND Base Register
         USING  BEGIN+8192,R10     THIRD Base Register
                                                                SPACE
BEGIN    BALR  R8,0                Initalize FIRST base register
         BCTR  R8,0                Initalize FIRST base register
         BCTR  R8,0                Initalize FIRST base register
                                                                SPACE
         LA    R9,2048(,R8)        Initalize SECOND base register
         LA    R9,2048(,R9)        Initalize SECOND base register

         LA    R10,2048(,R9)       Initalize THIRD base register
         LA    R10,2048(,R10)      Initalize THIRD base register

         STCTL R0,R0,CTLR0         Store CR0 to enable AFP
         OI    CTLR0+1,X'04'       Turn on AFP bit
         OI    CTLR0+1,X'02'       Turn on Vector bit
         LCTL  R0,R0,CTLR0         Reload updated CR0
                                                                EJECT
***********************************************************************
* Is PLO-extension facility installed  (bit 87)
***********************************************************************

         FCHECK 87,'PLO-extension'
                                                                EJECT
***********************************************************************
*    Do tests in the PLOETST table
***********************************************************************

         L     R12,=A(PLOTESTS)    get table of test address

NEXTTST  EQU   *
         L     R5,0(0,R12)         get test address
         LTR   R5,R5                have a test?
         BZ    ENDTEST                 done?

         USING PLOETEST,R5

* save before a test
         STMG  R0,R15,SVTST        Save regs before test

* start a test
         LGH   R0,TNUM             Save current test number
         ST    R0,TESTING

         L     R11,TSUB            get address of test routine
         BALR  R11,R11             do test

* restore after a test
         LMG   R0,R15,SVTST        restore regs after test

         LA    R12,4(0,R12)        next test address
         B     NEXTTST

SVTST    DS    16D                 save area
                                                                SPACE 3
***********************************************************************
* end of testing; set ending psw
***********************************************************************
ENDTEST  EQU   *
         L     R1,FAILED           did a test fail?
         LTR   R1,R1
         BZ    EOJ                 No, exit
         B     FAILTEST            Yes, exit with BAD PSW
                                                                 EJECT
***********************************************************************
* CCMSG:       cc was not as expected
*              R5  = address of test control table
*              R15 = return address
***********************************************************************
CCMSG    EQU   *
*
** check for ignore cc as PLO did not update CC
*
         CLI   CC,99               99 -->  cc not updated
         BE    0(R15)

         L     R0,=F'1'            set GLOBAL failed test indicator
         ST    R0,FAILED
*
* extract CC extracted PSW
*
         L     R1,CCPSW
         SRL   R1,12
         N     R1,=XL4'3'
         STC   R1,CCFOUND          save cc
*
* FILL IN MESSAGE
*
         LH    R2,TNUM             get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   CCPRTNUM(3),PRT3+13 fill in message with test #

         XGR   R2,R2               get FUN as U8
         IC    R2,FUN
         CVD   R2,DECNUM           and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   CCPRTFUN(2),PRT3+14 fill in message with CC field

         XGR   R2,R2               get CC as U8
         IC    R2,CC
         CVD   R2,DECNUM           and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   CCPRTEXP(1),PRT3+15 fill in message with CC field

         XGR   R2,R2               get CCFOUND as U8
         IC    R2,CCFOUND
         CVD   R2,DECNUM           and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   CCPRTGOT(1),PRT3+15 fill in message with ccfound
*
*        Use Hercules Diagnose for Message to console
*
         LA    R0,CCPRTLNG         message length
         LA    R1,CCPRTLINE        message address

         STMG  R15,R5,FCCSAV       save regs r15 to R5

         BAL   R2,MSG

         LMG   R15,R5,FCCSAV       restore regs
         BR    R15                 Return to caller
                                                                 SPACE
FCCSAV   DC     16D'0'             save area for MSG call
                                                                 EJECT
***********************************************************************
* URMSG:       UR was not as expected
*              R5  = address of test control table
*              R15 = return address
*
*              note: UR 99 - UR was not updated
***********************************************************************
URMSG    EQU   *
*
** check for ignore UR as PLO did not update UR
*
         CLI   UR,99               99 -->  UR not updated
         BE    0(R15)

         L     R0,=F'1'            set GLOBAL failed test indicator
         ST    R0,FAILED
*
* FILL IN MESSAGE
*
         LH    R2,TNUM             get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   URPRTNUM(3),PRT3+13 fill in message with test #

         XGR   R2,R2               get FUN as U8
         IC    R2,FUN
         CVD   R2,DECNUM           and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   URPRTFUN(2),PRT3+14 fill in message with Fun field

         XGR   R2,R2               get UR as U8
         IC    R2,UR
         CVD   R2,DECNUM           and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   URPRTEXP(1),PRT3+15 fill in message with UR field

         XGR   R2,R2               get URFOUND as U8
         IC    R2,URFOUND
         CVD   R2,DECNUM           and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   URPRTGOT(1),PRT3+15 fill in message with urfound
*
*        Use Hercules Diagnose for Message to console
*
         LA    R0,URPRTLNG         message length
         LA    R1,URPRTLINE        messagfe address

         STMG  R15,R5,URSAV        save regs r15 to R5

         BAL   R2,MSG

         LMG   R15,R5,URSAV       restore regs
         BR    R15                 Return to caller
                                                                 SPACE
URSAV    DC     16D'0'             save area for MSG call
                                                                 EJECT
***********************************************************************
* Fail Message to console
*              R5  = address of test control table
*              R15 = return address
***********************************************************************
FAILMSG  EQU   *
         L     R0,=F'1'            set GLOBAL failed test indicator
         ST    R0,FAILED

         LH    R2,TNUM             get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   FAILTNUM(3),PRT3+13 fill in message with test #

         MVC   FAILTXT(L'FAILTXT),=CL128' '  set to blanks
         XGR   R2,R2
         L     R2,FAILTXTA
         MVC   FAILTXT(L'FAILTXT),0(R2)      load message text

*
*        Use Hercules Diagnose for Message to console
*
         LA    R0,FAILLNG          message length
         LA    R1,FAILLINE         messagfe address

         STMG  R15,R5,FLSAV        save regs r15 to R5

         BAL   R2,MSG

         LMG   R15,R5,FLSAV        restore regs
         BR    R15                 Return to caller
                                                                 SPACE
FLSAV    DC     16D'0'             save area for MSG call
                                                                 EJECT
***********************************************************************
* Test: Not Available Message
*              R5  = address of test control table
*              R15 = return address
***********************************************************************
NAMSG    EQU   *
         L     R0,=F'1'            set GLOBAL failed test indicator
         ST    R0,FAILED

         USING PLOAVAL,R5

         LH    R2,ATNUM            get test number and convert
         CVD   R2,DECNUM
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   NAPRTNUM(3),PRT3+13 fill in message with test #

         XGR   R2,R2               get FUN as U8
         IC    R2,AFUN
         CVD   R2,DECNUM           and convert
         MVC   PRT3,EDIT
         ED    PRT3,DECNUM
         MVC   NAPRTFUN(2),PRT3+14 fill in message with Fun field

         DROP R5
*
*        Use Hercules Diagnose for Message to console
*
         LA    R0,NAPRTLNG         message length
         LA    R1,NAPRTLINE        messagfe address

         STMG  R15,R5,NACCSAV      save regs r15 to R5

         BAL   R2,MSG

         LMG   R15,R5,NACCSAV      restore regs
         BR    R15                 Return to caller
                                                                 SPACE
NACCSAV  DC     16D'0'             save area for MSG call
                                                                 EJECT
***********************************************************************
*        Issue HERCULES MESSAGE
*              R0 = length of message
*              R1 = message address
*              R2 = return address
***********************************************************************
                                                                 SPACE
MSG      CH    R0,=H'0'            Do we even HAVE a message?
         BNHR  R2                  No, ignore
                                                                 SPACE
         STM   R0,R2,MSGSAVE       Save registers
                                                                 SPACE
         CH    R0,=AL2(L'MSGMSG)   Message length within limits?
         BNH   MSGOK               Yes, continue
         LA    R0,L'MSGMSG         No, set to maximum
                                                                 SPACE
MSGOK    LR    R2,R0               Copy length to work register
         BCTR  R2,0                Minus-1 for execute
         EX    R2,MSGMVC           Copy message to O/P buffer
                                                                 SPACE
         LA    R2,1+L'MSGCMD(,R2)  Calculate true command length
         LA    R1,MSGCMD           Point to true command
                                                                 SPACE
         DC    X'83',X'12',X'0008' Issue Hercules Diagnose X'008'
         BZ    MSGRET              Return if successful

         LTR   R2,R2               Is Diag8 Ry (R2) 0?
         BZ    MSGRET                an error occurred but coninue

         DC    H'0'                CRASH for debugging purposes
                                                                 SPACE
MSGRET   LM    R0,R2,MSGSAVE       Restore registers
         BR    R2                  Return to caller
                                                                SPACE 6
MSGSAVE  DC    3F'0'               Registers save area

MSGMVC   MVC   MSGMSG(0),0(R1)     Executed instruction
                                                                SPACE 2
MSGCMD   DC    C'MSGNOH * '        *** HERCULES MESSAGE COMMAND ***
MSGMSG   DC    CL95' '             The message text to be displayed

         DS    0D
                                                                EJECT
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 5
EOJPSW   DC    0D'0',X'0002000180000000',AD(0)
                                                                SPACE
EOJ      LPSWE EOJPSW              Normal completion
                                                                SPACE 5
FAILPSW  DC    0D'0',X'0002000180000000',AD(X'BAD')
                                                                SPACE
FAILTEST LPSWE FAILPSW             Abnormal termination
                                                                SPACE 7
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE 2
         LTORG ,                   Literals pool

CTLR0    DS    F
         DS    F

* Constants

K        EQU   1024                One KB
PAGE     EQU   (4*K)               Size of one page
K64      EQU   (64*K)              64 KB
MB       EQU   (K*K)               1 MB
                                                                 EJECT
*======================================================================
*
*  NOTE: start data on an address that is easy to display
*        within Hercules
*
*======================================================================

         ORG   PLOETST+X'4000'
FAILED   DC    F'0'                some test failed?
TESTING  DC    F'0'                current test number
                                                                SPACE 4
***********************************************************************
*        TEST failed : instruction data
***********************************************************************
*
*        failed message and associated editting
*
FAILLINE DC    C'         Test # '
FAILTNUM DC    C'xxx'
         DC    C': '
FAILHLNG EQU   *-FAILLINE          fail line header length

FAILTXT  DC    CL100' '             Test fail message
FAILLNG  EQU   *-FAILLINE

                                                                SPACE 4
***********************************************************************
*        TEST failed : CC message
***********************************************************************
*
*        failed message and associated editting
*
CCPRTLINE DC   C'         Test # '
CCPRTNUM DC    C'xxx'
         DC    c': wrong cc for PLO instruction'
         DC    c' (function '
CCPRTFUN DC    CL2'xx'
         DC    c')'
         DC    C' expected: cc='
CCPRTEXP DC    C'x'
         DC    C','
         DC    C' received: cc='
CCPRTGOT DC    C'x'
         DC    C'.'
CCPRTLNG   EQU   *-CCPRTLINE
                                                                 EJECT
***********************************************************************
*        TEST failed : unequal results (UR) field message
***********************************************************************
*
*        failed message and associated editting
*
URPRTLINE DC   C'         Test # '
URPRTNUM DC    C'xxx'
         DC    c': wrong UR for PLO instruction'
         DC    c' (function '
URPRTFUN DC    CL2'xx'
         DC    c')'
         DC    C' expected: UR='
URPRTEXP DC    C'x'
         DC    C','
         DC    C' received: UR='
URPRTGOT DC    C'x'
         DC    C'.'
URPRTLNG   EQU   *-URPRTLINE
                                                                SPACE 4
***********************************************************************
*        TEST failed : PLO function is Not Available
***********************************************************************
*
*        failed message and associated editting
*
NAPRTLINE DC   C'         Test # '
NAPRTNUM DC    C'xxx'
         DC    C': PLO function '
NAPRTFUN DC    CL2'xx'
         DC    C' is NOT installed.'

NAPRTLNG EQU   *-NAPRTLINE

                                                                EJECT
***********************************************************************
*        TEST failed : message working storge
***********************************************************************
EDIT     DC    XL18'402120202020202020202020202020202020'

         DC    C'===>'
PRT3     DC    CL18' '
         DC    C'<==='
DECNUM   DS    CL16

* ---------------------------------------------------------------------
* Random Bytes for Test Data (1 KB)
* ---------------------------------------------------------------------
         DS    0D
RANDX    DC    XL16'7A 1F C3 08 9D E2 46 B1 5C F7 20 8B 64 DA 03 91'
         DC    XL16'BE 55 0C A7 3E D8 72 19 F4 6B 80 2D C9 14 EF 5A'
         DC    XL16'31 AC 77 E0 4B 96 0F D3 68 B9 25 FE 52 8D 17 C6'
         DC    XL16'EB 40 A1 6E 09 D5 83 2A F8 5F BC 12 74 C1 9A 36'
         DC    XL16'0D E7 59 B2 43 FC 86 1B D0 6C 28 A5 93 4E F1 7D'
         DC    XL16'C8 11 65 DE 3B 90 FA 27 54 AD 02 79 E4 B7 4C 88'
         DC    XL16'16 D9 70 A3 5E CB 34 F5 81 0A BD 62 E9 47 9F 23'
         DC    XL16'D6 58 01 AF 75 CA 3D E1 92 6F 18 B4 50 FD 87 2C'
         DC    XL16'A8 44 DB 0E 69 C2 35 F0 9B 56 ED 21 7F B8 04 D4'
         DC    XL16'63 AA 1D E6 48 95 CF 30 F9 82 5B 13 DC 76 A0 2F'
         DC    XL16'8E 41 F3 B5 0B 67 CE 24 99 DA 53 E8 1A 7C C7 32'
         DC    XL16'F6 85 49 B0 1E D2 6A 94 3F EB 07 5D A9 70 C4 26'
         DC    XL16'DD 10 84 F2 38 AB 61 C5 0C 97 4A E3 7B B6 29 D1'
         DC    XL16'5F 8A 15 CC 73 F8 00 A6 42 9C E5 37 BD 68 02 D7'
         DC    XL16'90 4D EA 22 7E C0 59 F1 0F A4 6D DB 35 88 B3 1C'
         DC    XL16'E0 71 2B C9 46 9E F4 60 AD 18 D5 83 3A BF 54 07'
         DC    XL16'A7 3C F1 08 6D B2 49 DE 15 83 5A C6 2F 91 E4 70'
         DC    XL16'D3 0B 58 AC E9 34 7F 12 C1 66 9D 42 F8 25 B7 5E'
         DC    XL16'81 DA 37 04 9A E2 6B C5 18 F3 4D 76 A0 2C BF 53'
         DC    XL16'E7 90 1D 68 C8 3F A5 72 0E DB 46 B1 59 F6 23 8C'
         DC    XL16'35 CE 7A 11 E0 94 5F B8 27 D4 63 0A F9 41 AD 86'
         DC    XL16'C3 1A 75 EC 50 9F 28 D7 62 B4 0D E8 39 87 FA 14'
         DC    XL16'6F C0 32 95 4A BE 07 D1 7C E5 21 89 56 F2 A9 3D'
         DC    XL16'B6 48 DF 10 64 9C 2A F5 73 C7 0F 82 ED 31 A4 5B'
         DC    XL16'D9 26 90 4F B3 6A E1 0C 78 C2 55 AF 13 F7 84 3E'
         DC    XL16'69 D5 20 9B 47 EA 71 06 BD 52 F0 38 8E C9 16 A2'
         DC    XL16'5D E6 2B 97 40 BC 03 7E D0 19 A8 65 F4 33 8A C4'
         DC    XL16'1F 74 EB 57 92 0A D8 43 B5 6C F1 2E 80 CD 39 A6'
         DC    XL16'E3 51 9E 24 7B C8 0D B0 67 F5 32 8F DA 14 46 AC'
         DC    XL16'79 E9 05 93 2D C6 58 B1 7F 30 ED 84 12 A5 6E D2'
         DC    XL16'4C F8 21 9A 63 BE 07 D6 50 C3 88 1B F0 35 A9 72'
         DC    XL16'DB 44 8D 17 E2 69 B7 2C 95 0F C1 5A F6 38 AE 73'
         DC    XL16'4F A8 D2 1C 9B 67 3E F0 85 14 C9 72 AD 5F E1 38'
         DC    XL16'B7 2D 94 FE 61 0A C3 57 88 D1 4B AE 23 F9 76 10'
         DC    XL16'D8 45 9A 3C E7 81 16 BF 52 CC 09 74 A1 ED 68 35'
         DC    XL16'FA 20 B3 5D 8E C7 41 9C 6B F4 17 DA 80 2E 53 BD'
         DC    XL16'31 CE 7A 05 E8 92 4F B1 6D 23 FC 58 A7 10 D9 84'
         DC    XL16'65 B8 E3 1F 90 4A C6 7D 2B F5 18 A2 DE 73 0C 49'
         DC    XL16'87 D4 3F B9 21 EC 56 08 C1 7A AD 34 F2 6E 95 10'
         DC    XL16'CB 42 F8 1D 73 A6 0E D5 89 2C B7 50 EF 14 68 93'
         DC    XL16'3A D1 86 FC 27 5B E0 9D 41 B8 12 C7 6F A4 35 EE'
         DC    XL16'90 4C DB 18 75 A2 F6 3E C9 07 B1 5D E8 24 7A CF'
         DC    XL16'16 E3 58 8B D0 32 FA 6D 97 41 BC 0F 74 A5 E1 2C'
         DC    XL16'F7 89 13 CE 5A D4 20 B6 7F E2 49 9C 35 A8 01 DD'
         DC    XL16'68 F1 2B 95 C0 7E 14 BA 53 EF 86 39 D7 42 AC 10'
         DC    XL16'B4 6D E8 21 F9 57 0C C3 7A DE 45 98 12 FB 64 3E'
         DC    XL16'8D 30 C7 5B E1 A4 19 F2 76 0F D8 43 BE 95 28 6A'
         DC    XL16'F3 5E 80 CA 17 6D B9 24 E7 41 0A DC 58 92 AF 35'
         DC    XL16'34 C1 7D E9 52 AB 06 F8 1E 93 6C D4 27 B0 5F EA'
         DC    XL16'79 14 CE 63 A8 2D F1 45 9B 30 E7 5A 8C D6 12 BF'
         DC    XL16'E2 58 0F B4 71 DA 29 83 C5 1C F7 6E 90 3D A1 4B'
         DC    XL16'0D FE 67 38 C2 95 1A B8 4F E3 72 DC 06 A9 54 21'
         DC    XL16'BC 47 D1 8E 23 F5 60 9A 14 C8 7D E0 35 B2 6F 91'
         DC    XL16'A6 19 FB 42 D7 0C 85 3E E1 74 28 CA 5D 90 B3 67'
         DC    XL16'12 E8 53 AC 39 F6 0B D1 7C A5 24 EF 68 93 4D B7'
         DC    XL16'F0 2A 9E 65 C3 18 DB 47 81 5C EA 36 72 0F B9 D4'
         DC    XL16'58 A1 3C E7 94 2F D0 6B C5 19 FE 43 87 B2 0D 7A'
         DC    XL16'CD 70 16 F9 4A B5 28 E3 8D 51 AC 37 F0 64 9B 12'
         DC    XL16'26 DE 81 5B C4 F7 30 9D EA 14 68 A2 3F D1 75 BC'
         DC    XL16'91 0E F4 67 2B C8 53 AD 18 E7 45 9A D0 36 FB 72'
         DC    XL16'AF 3D C1 84 59 E2 17 B6 F8 20 D7 6A 93 4C 0F E1'
         DC    XL16'73 B9 24 DA 05 8E C7 41 F2 6D 10 AC 57 E8 3B 96'
         DC    XL16'1F 62 D5 A8 34 EB 79 0C C0 4A 97 F3 2E BD 61 85'
         DC    XL16'E4 37 8A D2 5C F0 19 B7 46 CE 23 9D 70 1B A5 F8'

* ---------------------------------------------------------------------
* Test Table is Located here .....
* ---------------------------------------------------------------------
PLOTESTS DS    0D
*
*        PTTABLE will be located here
*
         ORG   *+(256*4)+8         1K for pointers for up to 256 tests
*                                       plus 8 bytes for table end
                                                                 EJECT
***********************************************************************
*        PLOETEST DSECT
***********************************************************************
                                                                SPACE 2
PLOETEST DSECT ,
TSUB     DC    A(0)                pointer  to test
TNUM     DC    H'00'               Test Number
         DC    XL1'00'
FUN      DC    HL1'00'             Function
UR       DC    HL1'00'             expected unequal results (UR)
URFOUND  DS    HL1'00'             extracted UR from PL

CC       DC    HL1'00'             cc
CCMASK   DC    HL1'00'             not expected CC mask

CCFOUND  DS    X                   extracted cc
CCPSW    DS    2F                  extract PSW after test (has CC)

FAILTXTA DC    A(0)                address of test fail message
OPSRCA   DC    A(0)                address of operand source
OPREPA   EQU   *                   address of operand replacement
OPDSTA   DC    A(0)                address of operand destination
PLA      DC    A(0)                address of parameter list
RANDXA   DC    A(0)                address of random source data
                                                                SPACE 2
***********************************************************************
*        PLOEAVL DSECT
***********************************************************************
                                                                SPACE 2
PLOAVAL  DSECT ,
ATSUB    DC    A(0)                pointer  to test
ATNUM    DC    H'00'               Test Number
         DC    XL1'00'
AFUN     DC    HL1'00'             Function
         DS    0F
                                                                 EJECT
***********************************************************************
*     Macros to help build test tables
***********************************************************************
                                                                SPACE 3
*----------------------------------------------------------------------
* TSTPLO : test whether a PLO function is available/installed.
*          If a functionm is installed, CC=0. Not installed, CC=3.
*          - &fun: the function number
*----------------------------------------------------------------------
         MACRO
         TSTPLO  &FUN
         GBLA  &TNUM
&TNUM    SETA  &TNUM+1
*
* test if PLO function is available
*
         USING *,R5                base for test data and test routine
         USING PLOAVAL,R5

T&TNUM   DC    A(CODE&TNUM)           address of test routine
         DC    H'&TNUM'            test number
         DC    X'00'
         DC    HL1'&FUN'           Function Number
*
CODE&TNUM DS    0F
         LA    R1,(256+&FUN)       T bit 55  (+256)
         LR    R0,R1
         PLO   0,0,0,0
         BZ    0(R11)              Function is installed, return

         BAL   R15,NAMSG           Function not available message
         BR    R11                 return
         DROP  R5
*
         MEND

*----------------------------------------------------------------------
* TSTLBL : insert a label with a test number suffix
*          - &name: the label name
*----------------------------------------------------------------------
         MACRO
         TSTLBL  &NAME
         GBLA  &TNUM
         LCLC  &X
&X       SETC  '&NAME&TNUM'
         DC    CL32'>>>&X<<<'      eye catcher
&X       DS  0C
         MEND
                                                                 EJECT
*----------------------------------------------------------------------
* TSTEND : end a test
*----------------------------------------------------------------------
         MACRO
         TSTEND  &NAME
         GBLA  &TNUM
         DC    CL32'END of test: &TNUM <<<'
         MEND

*----------------------------------------------------------------------
* TSTCODE : The code for the test follows the label
*----------------------------------------------------------------------
         MACRO
         TSTCODE  &NAME
         GBLA  &TNUM
         LCLC  &X
&X       SETC  'CODE&TNUM'
&X       DS    0F
         MEND

*----------------------------------------------------------------------
* TSTDATA: The data for the test follows.
*          Data is aligned to the 1.5K boundary after the TSTHDR
*----------------------------------------------------------------------
         MACRO
         TSTDATA  &NAME
         GBLA  &THDRNUM
         ORG   PLOETST+(128*K)+&THDRNUM*(3*K)+(K+512)
         MEND
                                                                 EJECT
*----------------------------------------------------------------------
* TSTHDR: The start of a PLOE function test
*         Test is aligned to 3K boundary starting at 128K
*              (1.5K data/code; 1.5K instruction test data)
*
*        Note: PLOETEST DSECT needs to be in-sync with TSTHDR
*----------------------------------------------------------------------
         MACRO
         TSTHDR  &CC,&UR,&FUN
.*                      &CC    - expected CC
.*                      &UR    - expected unequal results
.*                      &FUN   - function

         LCLA  &XCC(4)  &CC has mask values for FAILED condition codes
&XCC(1)  SETA  7                   CC != 0
&XCC(2)  SETA  11                  CC != 1
&XCC(3)  SETA  13                  CC != 2
&XCC(4)  SETA  14                  CC != 3

         GBLA  &TNUM
&TNUM    SETA  &TNUM+1

         GBLA  &THDRNUM
&THDRNUM SETA  &THDRNUM+1

         ORG   PLOETST+(128*K)+&THDRNUM*(3*K)
         DC    CL32'START of test: &TNUM'

         USING *,R5                base for test data and test routine
         USING PLOETEST,R5

T&TNUM   DC    A(CODE&TNUM)        address of test code
         DC    H'&TNUM'            test number
         DC    X'00'
         DC    HL1'&FUN'           Function
         DC    HL1'&UR'            expected unequal results (UR)
         DC    HL1'00'             found UR

         DC    HL1'&CC'            cc
         AIF   ('&CC' EQ '99').NOCC
         DC    HL1'&XCC(&CC+1)'    not expected CC mask
         AGO   .CCCONT
.NOCC    ANOP
         DC    HL1'00'             cc not checked
.CCCONT  ANOP

         DS    X                   extracted cc
         DS    2F                  extract PSW after test (has CC)
         DC    A(FMSG&TNUM)        address of test fail message
         DC    A(OPSRC&TNUM)       address of operand source
         DC    A(OPDST&TNUM)       address of operand destination
         DC    A(PL&TNUM)          address of parameter list
         DC    A(RANDX+&TNUM)      address of random source data
.*
         MEND
                                                               SPACE 3
*----------------------------------------------------------------------
* PTTABLE: generate table of pointers to individual tests
*----------------------------------------------------------------------

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
         DC    A(0)                END OF TABLE
         DC    A(0)
.*
         MEND

                                                                EJECT
***********************************************************************
*        PLO-Ext tests
***********************************************************************
PLOETST  CSECT
         PRINT DATA
* ---------------------------------------------------------------------
* Test that non-PLO_Extension functions are available/installed
* ---------------------------------------------------------------------
         TSTPLO 1
         TSTPLO 16
* ---------------------------------------------------------------------
* Test PLO_Extension functions are available/installed
* ---------------------------------------------------------------------
         TSTPLO 24
         TSTPLO 25
         TSTPLO 26
         TSTPLO 27
         TSTPLO 28
         TSTPLO 29
         TSTPLO 30
         TSTPLO 31
         TSTPLO 32
         TSTPLO 33
         TSTPLO 34
         TSTPLO 35
         TSTPLO 36
         TSTPLO 37
         TSTPLO 38
         TSTPLO 39
         TSTPLO 40
         TSTPLO 41
         TSTPLO 42
         TSTPLO 43
         TSTPLO 44
         TSTPLO 45
         TSTPLO 46
         TSTPLO 47
         TSTPLO 48
         TSTPLO 49
         TSTPLO 50
         TSTPLO 51
         TSTPLO 52
         TSTPLO 53
         TSTPLO 54
         TSTPLO 55
         TSTPLO 56
         TSTPLO 57
         TSTPLO 58
         TSTPLO 59
         TSTPLO 60
         TSTPLO 61
                                                                EJECT
* ---------------------------------------------------------------------
* NOTE:  A test is structured as follows:
*
*        TSTHDR
*        TSTCODE
*
*        ... code to test the function
*
*        TSTDATA                   test data is located here
*        TSTLBL OPDST              Operand Destination
*        DC    XL128'0'
*
*        TSTLBL OPSRC              Operand Source
*        DC    XL128'0'
*
*        TSTLBL PL                 Parameter List
*        DC    XL256'00'
*
*        TSTLBL FMSG               fail message
*        DC    C'failed PLO-Extension '
*        DC    CLxx'.......'
*
*        TSTEND
* ---------------------------------------------------------------------

*======================================================================
*       Quadruple Load / Store Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Quadruple Load(QLO: 48) / Store(QSTO: 61) instructions
* ---------------------------------------------------------------------
         TSTHDR 99,99,99

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA

         MVC   0(128,R1),0(R2)
*
* Quadruple Load(QLO: 48)
*
         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,32(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,48
         PLO   R1,0,0,0(R3)        Quadruple Load(QLO: 48)
*
* Quadruple Store(QSTO: 61)
*
         LGF   R1,OPDSTA           Destination address
         LGF   R3,PLA

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,32(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,61
         PLO   R1,0,0,0(R3)        Quadruple Store(QSTO: 61)
*
* compare results
*
         LGF   R1,OPSRCA           source
         LGF   R3,OPDSTA           Destination

         CLC   0(32,R1),0(R3)
         BNE   F4861               OP1 Different, error
         CLC   32(32,R1),32(R3)
         BNE   F4861               OP2 Different, error
         CLC   64(32,R1),64(R3)
         BNE   F4861               OP3 Different, error
         CLC   96(32,R1),96(R3)
         BE    0(R11)              OP4 same, return

F4861    EQU   *
         BAL   R15,FAILMSG
         BR    R11

         TSTDATA
         TSTLBL OPDST              Operand Destination (Results)
         DC    XL128'0'

         TSTLBL OPSRC              Operand Source
         DC    XL128'0'

         TSTLBL PL                 Parameter List
         DC    XL256'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Load(QLO: 48) / Store(QSTO: 61)        '

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Load(QLX: 47) / Store(QSTX: 60) instructions
* ---------------------------------------------------------------------
         TSTHDR 99,99,99

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA

         MVC   0(128,R1),0(R2)
*
* Quadruple Load(QLX: 47)
*
         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,32(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,47
         PLO   R1,0,0,0(R3)        Quadruple Load(QLX: 47)
*
* Quadruple Store(QSTX: 60)
*
         LGF   R1,OPDSTA           Destination address
         LGF   R3,PLA

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,32(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,60
         PLO   R1,0,0,0(R3)        Quadruple Store(QSTX: 60)
*
* compare results
*
         LGF   R1,OPSRCA           source
         LGF   R3,OPDSTA           Destination

         CLC   0(16,R1),0(R3)
         BNE   F4760               OP1 Different, error
         CLC   32(16,R1),32(R3)
         BNE   F4760               OP2 Different, error
         CLC   64(16,R1),64(R3)
         BNE   F4760               OP3 Different, error
         CLC   96(16,R1),96(R3)
         BE    0(R11)              OP4 same, return

F4760    EQU   *
         BAL   R15,FAILMSG
         BR    R11

         TSTDATA
         TSTLBL OPDST              Operand Destination (Results)
         DC    XL128'0'

         TSTLBL OPSRC              Operand Source
         DC    XL128'0'

         TSTLBL PL                 Parameter List
         DC    XL256'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Load(QLX: 47) / Store(QSTX: 60)'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Load(QLG: 46) / Store(QSTG: 59) instructions
* ---------------------------------------------------------------------
         TSTHDR 99,99,99

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA

         MVC   0(128,R1),0(R2)
*
* Quadruple Load(QLX: 46)
*
         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,32(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,46
         PLO   R1,0,0,0(R3)        Quadruple Load(QLG: 46)
*
* Quadruple Store(QSTX: 59)
*
         LGF   R1,OPDSTA           Destination address
         LGF   R3,PLA

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,32(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,59
         PLO   R1,0,0,0(R3)        Quadruple Store(QSTG: 59)
*
* compare results
*
         LGF   R1,OPSRCA           source
         LGF   R3,OPDSTA           Destination

         CLC   0(8,R1),0(R3)
         BNE   F4659               OP1 Different, error
         CLC   32(8,R1),32(R3)
         BNE   F4659               OP2 Different, error
         CLC   64(8,R1),64(R3)
         BNE   F4659               OP3 Different, error
         CLC   96(8,R1),96(R3)
         BE    0(R11)              OP4 same, return

F4659    EQU   *
         BAL   R15,FAILMSG
         BR    R11

         TSTDATA
         TSTLBL OPDST              Operand Destination (Results)
         DC    XL128'0'

         TSTLBL OPSRC              Operand Source
         DC    XL128'0'

         TSTLBL PL                 Parameter List
         DC    XL256'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Load(QLG: 46) / Store(QSTG: 59)'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Load(QL: 45) / Store(QST: 58) instructions
* ---------------------------------------------------------------------
         TSTHDR 99,99,99

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA

         MVC   0(128,R1),0(R2)
*
* Quadruple Load(QLX: 45`
*
         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,32(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,45
         PLO   R1,0,0,0(R3)        Quadruple Load(QL: 45)
*
* Quadruple Store(QST: 58)
*
         LGF   R1,OPDSTA           Destination address
         LGF   R3,PLA

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,32(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,58
         PLO   R1,0,0,0(R3)        Quadruple Store(QST: 58)
*
* compare results
*
         LGF   R1,OPSRCA           source
         LGF   R3,OPDSTA           Destination

         CLC   0(4,R1),0(R3)
         BNE   F4558               OP1 Different, error
         CLC   32(4,R1),32(R3)
         BNE   F4558               OP2 Different, error
         CLC   64(4,R1),64(R3)
         BNE   F4558               OP3 Different, error
         CLC   96(4,R1),96(R3)
         BE    0(R11)              OP4 same, return

F4558    EQU   *
         BAL   R15,FAILMSG
         BR    R11

         TSTDATA
         TSTLBL OPDST              Operand Destination (Results)
         DC    XL128'0'

         TSTLBL OPSRC              Operand Source
         DC    XL128'0'

         TSTLBL PL                 Parameter List
         DC    XL256'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Load(QL: 45) / Store(QST: 58)'

         TSTEND
                                                                EJECT
*======================================================================
*       Triple Load / Store Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Triple Load(TLO: 44) / Store(TSTO: 57) instructions
* ---------------------------------------------------------------------
         TSTHDR 99,99,99

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA

         MVC   0(128,R1),0(R2)
*
* Triple Load(TLO: 44)
*
         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,44
         PLO   R1,0,0,0(R3)        Triple Load(TLO: 44)
*
* Triple Store(TSTO: 57)
*
         LGF   R1,OPDSTA           Destination address
         LGF   R3,PLA

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,57
         PLO   R1,0,0,0(R3)        Triple Store(TSTO: 57)
*
* compare results
*
         LGF   R1,OPSRCA           source
         LGF   R3,OPDSTA           Destination

         CLC   0(32,R1),0(R3)
         BNE   F4457               OP1 Different, error
         CLC   32(32,R1),32(R3)
         BNE   F4457               OP2 Different, error
         CLC   64(32,R1),64(R3)
         BE    0(R11)              OP3 same, return

F4457    EQU   *
         BAL   R15,FAILMSG
         BR    R11

         TSTDATA
         TSTLBL OPDST              Operand Destination (Results)
         DC    XL128'0'

         TSTLBL OPSRC              Operand Source
         DC    XL128'0'

         TSTLBL PL                 Parameter List
         DC    XL256'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Load(TLO: 44) / Store(TSTO: 57)'

         TSTEND

* ---------------------------------------------------------------------
* Test Triple Load(TLX: 43) / Store(TSTX: 56) instructions
* ---------------------------------------------------------------------
         TSTHDR 99,99,99

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA

         MVC   0(128,R1),0(R2)
*
* Triple Load(TLX: 43)
*
         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,43
         PLO   R1,0,0,0(R3)        Triple Load(TLX: 43)
*
* Triple Store(TSTX: 56)
*
         LGF   R1,OPDSTA           Destination address
         LGF   R3,PLA

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,56
         PLO   R1,0,0,0(R3)        Triple Store(TSTX: 56)
*
* compare results
*
         LGF   R1,OPSRCA           source
         LGF   R3,OPDSTA           Destination

         CLC   0(16,R1),0(R3)
         BNE   F4356               OP1 Different, error
         CLC   32(16,R1),32(R3)
         BNE   F4356               OP2 Different, error
         CLC   64(16,R1),64(R3)
         BE    0(R11)              OP3 same, return

F4356    EQU   *
         BAL   R15,FAILMSG
         BR    R11

         TSTDATA
         TSTLBL OPDST              Operand Destination (Results)
         DC    XL128'0'

         TSTLBL OPSRC              Operand Source
         DC    XL128'0'

         TSTLBL PL                 Parameter List
         DC    XL256'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Load(TLX: 43) / Store(TSTX: 56)'

         TSTEND

* ---------------------------------------------------------------------
* Test Triple Load(TLG: 42) / Store(TSTG: 55) instructions
* ---------------------------------------------------------------------
         TSTHDR 99,99,99

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA

         MVC   0(128,R1),0(R2)
*
* Triple Load(TLG: 42)
*
         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,42
         PLO   R1,0,0,0(R3)        Triple Load(TLG: 42)
*
* Triple Store(TSTG: 55)
*
         LGF   R1,OPDSTA           Destination address
         LGF   R3,PLA

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,55
         PLO   R1,0,0,0(R3)        Triple Store(TSTG: 55)
*
* compare results
*
         LGF   R1,OPSRCA           source
         LGF   R3,OPDSTA           Destination

         CLC   0(8,R1),0(R3)
         BNE   F4255               OP1 Different, error
         CLC   32(8,R1),32(R3)
         BNE   F4255               OP2 Different, error
         CLC   64(8,R1),64(R3)
         BE    0(R11)              OP3 same, return

F4255    EQU   *
         BAL   R15,FAILMSG
         BR    R11

         TSTDATA
         TSTLBL OPDST              Operand Destination (Results)
         DC    XL128'0'

         TSTLBL OPSRC              Operand Source
         DC    XL128'0'

         TSTLBL PL                 Parameter List
         DC    XL256'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Load(TLG: 42) / Store(TSTG: 55)'

         TSTEND

* ---------------------------------------------------------------------
* Test Triple Load(TL: 41) / Store(TST: 54) instructions
* ---------------------------------------------------------------------
         TSTHDR 99,99,99

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA

         MVC   0(128,R1),0(R2)
*
* Triple Load(TL: 41)
*
         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,41
         PLO   R1,0,0,0(R3)        Triple Load(TL: 41)
*
* Triple Store(TST: 54)
*
         LGF   R1,OPDSTA           Destination address
         LGF   R3,PLA

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,54
         PLO   R1,0,0,0(R3)        Triple Store(TST: 54)
*
* compare results
*
         LGF   R1,OPSRCA           source
         LGF   R3,OPDSTA           Destination

         CLC   0(4,R1),0(R3)
         BNE   F4154               OP1 Different, error
         CLC   32(4,R1),32(R3)
         BNE   F4154               OP2 Different, error
         CLC   64(4,R1),64(R3)
         BE    0(R11)              OP3 same, return

F4154    EQU   *
         BAL   R15,FAILMSG
         BR    R11

         TSTDATA
         TSTLBL OPDST              Operand Destination (Results)
         DC    XL128'0'

         TSTLBL OPSRC              Operand Source
         DC    XL128'0'

         TSTLBL PL                 Parameter List
         DC    XL256'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Load(TL: 41) / Store(TST: 54)'

         TSTEND
                                                                EJECT
*======================================================================
*       Double Load / Store Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Double Load(DLO: 40) / Store(DSTO: 53) instructions
* ---------------------------------------------------------------------
         TSTHDR 99,99,99

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA

         MVC   0(128,R1),0(R2)
*
* Double Load(DLO: 40)
*
         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         XGR   R0,R0
         LA    R0,40
         PLO   R1,0,0,0(R3)        Double Load(DLO: 40)
*
* Double Store(DSTO: 53)
*
         LGF   R1,OPDSTA           Destination address
         LGF   R3,PLA

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         XGR   R0,R0
         LA    R0,53
         PLO   R1,0,0,0(R3)        Double Store(DSTO: 53)
*
* compare results
*
         LGF   R1,OPSRCA           source
         LGF   R3,OPDSTA           Destination

         CLC   0(32,R1),0(R3)
         BNE   F4053               OP1 Different, error
         CLC   32(32,R1),32(R3)
         BE    0(R11)              OP2 same, return

F4053    EQU   *
         BAL   R15,FAILMSG
         BR    R11

         TSTDATA
         TSTLBL OPDST              Operand Destination (Results)
         DC    XL128'0'

         TSTLBL OPSRC              Operand Source
         DC    XL128'0'

         TSTLBL PL                 Parameter List
         DC    XL256'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Double Load(DLO: 40) / Store(DSTO: 53)'

         TSTEND

* ---------------------------------------------------------------------
* Test Double Load(DLX: 39) / Store(DSTX: 52) instructions
* ---------------------------------------------------------------------
         TSTHDR 99,99,99

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA

         MVC   0(128,R1),0(R2)
*
* Double Load(DLX: 39)
*
         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         XGR   R0,R0
         LA    R0,43
         PLO   R1,0,0,0(R3)        Double Load(DLX: 39)
*
* Double Store(DSTX: 52)
*
         LGF   R1,OPDSTA           Destination address
         LGF   R3,PLA

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         XGR   R0,R0
         LA    R0,52
         PLO   R1,0,0,0(R3)        Double Store(DSTX: 52)
*
* compare results
*
         LGF   R1,OPSRCA           source
         LGF   R3,OPDSTA           Destination

         CLC   0(16,R1),0(R3)
         BNE   F4352               OP1 Different, error
         CLC   32(16,R1),32(R3)
         BE    0(R11)              OP2 same, return

F4352    EQU   *
         BAL   R15,FAILMSG
         BR    R11

         TSTDATA
         TSTLBL OPDST              Operand Destination (Results)
         DC    XL128'0'

         TSTLBL OPSRC              Operand Source
         DC    XL128'0'

         TSTLBL PL                 Parameter List
         DC    XL256'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Double Load(DLX: 39) / Store(DSTX: 52)'

         TSTEND
                                                                EJECT
*======================================================================
*       Double Store Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Double Store(DSTG: 51) instruction
* ---------------------------------------------------------------------
         TSTHDR 99,99,51

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA

         MVC   0(128,R1),0(R2)
*
* load of op1-data, op2-data
*
         LG    R2,0(R1)            Get OP1 from src
         STG   R2,64(R3)           load OP1-Data in PL

         LG    R2,8(R1)            Get OP2 from src
         STG   R2,72(R3)           load OP2-Data in PL
*
* Double Store(DSTG: 51)
*
         LGF   R1,OPDSTA           Destination address
         LGF   R3,PLA

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         XGR   R0,R0
         LA    R0,51
         PLO   R1,0,0,0(R3)        Double Store(DSTG: 51)
*
* compare results
*
         LGF   R1,OPSRCA           source
         LGF   R3,OPDSTA           Destination

         CLC   0(8,R1),0(R3)
         BNE   FXX51               OP1 Different, error
         CLC   8(8,R1),32(R3)
         BE    0(R11)              OP2 same, return

FXX51    EQU   *
         BAL   R15,FAILMSG
         BR    R11

         TSTDATA
         TSTLBL OPDST              Operand Destination (Results)
         DC    XL128'0'

         TSTLBL OPSRC              Operand Source
         DC    XL128'0'

         TSTLBL PL                 Parameter List
         DC    XL256'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Double Store(DSTX: 51)'

         TSTEND

* ---------------------------------------------------------------------
* Test Double Store(DSTG: 50) instruction
* ---------------------------------------------------------------------
         TSTHDR 99,99,50

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA

         MVC   0(128,R1),0(R2)
*
* load of op1-data, op2-data
*
         L     R2,0(R1)            Get OP1 from src
         ST    R2,64(R3)           load OP1-Data in PL

         L     R2,8(R1)            Get OP2 from src
         ST    R2,72(R3)           load OP2-Data in PL
*
* Double Store(DSTG: 50
*
         LGF   R1,OPDSTA           Destination address
         LGF   R3,PLA

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         XGR   R0,R0
         LA    R0,50
         PLO   R1,0,0,0(R3)        Double Store(DSTG: 50)
*
* compare results
*
         LGF   R1,OPSRCA           source
         LGF   R3,OPDSTA           Destination

         CLC   0(4,R1),0(R3)
         BNE   FXX50               OP1 Different, error
         CLC   8(4,R1),32(R3)
         BE    0(R11)              OP2 same, return

FXX50    EQU   *
         BAL   R15,FAILMSG
         BR    R11

         TSTDATA
         TSTLBL OPDST              Operand Destination (Results)
         DC    XL128'0'

         TSTLBL OPSRC              Operand Source
         DC    XL128'0'

         TSTLBL PL                 Parameter List
         DC    XL256'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Double Store(DSTG: 50)'

         TSTEND

                                                                EJECT
*======================================================================
*       Load / Store Test
*======================================================================

* ---------------------------------------------------------------------
* Test Load(LO: 38) / Store(STO: 49) instructions
* ---------------------------------------------------------------------
         TSTHDR 99,99,99

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA

         MVC   0(128,R1),0(R2)
*
* Load(LO: 38)
*
         STG   R1,0(R3)            load OP1 address in PL

         XGR   R0,R0
         LA    R0,38
         PLO   R1,0,0,0(R3)        Load(LO: 38)
*
* Store(STO: 49)
*
         LGF   R1,OPDSTA           Destination address
         LGF   R3,PLA

         STG   R1,0(R3)            load OP1 address in PL

         XGR   R0,R0
         LA    R0,49
         PLO   R1,0,0,0(R3)        Store(STO: 49)
*
* compare results
*
         LGF   R1,OPSRCA           source
         LGF   R3,OPDSTA           Destination

         CLC   0(32,R1),0(R3)
         BE    0(R11)              OP1 same, return

         BAL   R15,FAILMSG
         BR    R11

         TSTDATA
         TSTLBL OPDST              Operand Destination (Results)
         DC    XL128'0'

         TSTLBL OPSRC              Operand Source
         DC    XL128'0'

         TSTLBL PL                 Parameter List
         DC    XL256'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Load(LO: 38) / Store(STO: 49)'

         TSTEND

                                                                EJECT
*======================================================================
*       Quadruple Compare and Swap Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSO: 37) instruction : 1st NE
* ---------------------------------------------------------------------
         TSTHDR 2,1,37             1st Operand NOT Equal (cc=2, UR=1)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSO: 37)
* Build Parambter List
*

         MVC   0(128,R1),0(R2)     load Op1-Op4 data in source
         MVC   64(128,R3),0(R1)    copy to PL OP1-Op4 Compare

         MVC   0(128,R4),128(R2)   load initial Op1-Op4 data
         MVC   192(128,R3),0(R4)   copy to PL Op1-Op4 Replacement
****
* force a failure by changing the first operand
****
         MVC   0(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,32(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,37
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSO: 37)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F371TSTCC            fail if...

F371CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F371CKRES
         BAL   R15,URMSG
*
* compare results
*
F371CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(32,R1),64(R3)
         BNE   FXX137              OP1C Different, error
         CLC   32(32,R1),96(R3)
         BNE   FXX137              OP2C Different, error
         CLC   64(32,R1),128(R3)
         BNE   FXX137              OP3C Different, error
         CLC   96(32,R1),160(R3)
         BE    0(R11)              OP4Csame, return

FXX137    EQU   *
         BAL   R15,FAILMSG
         BR    R11

F371TSTCC BC    0,F371CCMSG        fail if unexpected condition code
F371CCMSG EQU   *
         BAL   R15,CCMSG
         B     F371CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSO: 37): 1st NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSO: 37) instruction : 2nd NE
* ---------------------------------------------------------------------
         TSTHDR 2,2,37             2nd Operand NOT Equal (cc=2, UR=2)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSO: 37)
* Build Parambter List
*
         MVC   0(128,R1),0(R2)     load Op1-Op4 data in source
         MVC   64(128,R3),0(R1)    copy to PL OP1-Op4 Compare

         MVC   0(128,R4),128(R2)   load initial Op1-Op4 data
         MVC   192(128,R3),0(R4)   copy to PL Op1-Op4 Replacement
****
* force a failure by changing the 2nd operand
****
         MVC   32(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,32(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,37
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSO: 37)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F372TSTCC            fail if...

F372CKUR  EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F372CKRES
         BAL   R15,URMSG
*
* compare results
*
F372CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(32,R1),64(R3)
         BNE   FXX237              OP1C Different, error
         CLC   32(32,R1),96(R3)
         BNE   FXX237              OP2C Different, error
         CLC   64(32,R1),128(R3)
         BNE   FXX237              OP3C Different, error
         CLC   96(32,R1),160(R3)
         BE    0(R11)              OP4Csame, return

FXX237   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F372TSTCC BC    0,F372CCMSG        fail if unexpected condition code
F372CCMSG EQU   *
         BAL   R15,CCMSG
         B     F372CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSO: 37): 2nd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSO: 37) instruction : 3rd NE
* ---------------------------------------------------------------------
         TSTHDR 2,3,37             3rd Operand NOT Equal (cc=2, UR=3)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSO: 37)
* Build Parambter List
*
         MVC   0(128,R1),0(R2)     load Op1-Op4 data in source
         MVC   64(128,R3),0(R1)    copy to PL OP1-Op4 Compare

         MVC   0(128,R4),128(R2)   load initial Op1-Op4 data
         MVC   192(128,R3),0(R4)   copy to PL Op1-Op4 Replacement
****
* force a failure by changing the 3nd operand
****
         MVC   64(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,32(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,37
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSO: 37)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F373TSTCC            fail if...

F373CKUR  EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F373CKRES
         BAL   R15,URMSG
*
* compare results
*
F373CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(32,R1),64(R3)
         BNE   F373XX              OP1C Different, error
         CLC   32(32,R1),96(R3)
         BNE   F373XX              OP2C Different, error
         CLC   64(32,R1),128(R3)
         BNE   F373XX              OP3C Different, error
         CLC   96(32,R1),160(R3)
         BE    0(R11)              OP4Csame, return

F373XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F373TSTCC BC    0,F373CCMSG        fail if unexpected condition code
F373CCMSG EQU   *
         BAL   R15,CCMSG
         B     F373CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSO: 37): 3rd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSO: 37) instruction : 4th NE
* ---------------------------------------------------------------------
         TSTHDR 2,4,37             4th Operand NOT Equal (cc=2, UR=4)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSO: 37)
* Build Parambter List
*
         MVC   0(128,R1),0(R2)     load Op1-Op4 data in source
         MVC   64(128,R3),0(R1)    copy to PL OP1-Op4 Compare

         MVC   0(128,R4),128(R2)   load initial Op1-Op4 data
         MVC   192(128,R3),0(R4)   copy to PL Op1-Op4 Replacement
****
* force a failure by changing the 4th operand
****
         MVC   96(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,32(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,37
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSO: 37)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F374TSTCC            fail if...

F374CKUR  EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F374CKRES
         BAL   R15,URMSG
*
* compare results
*
F374CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(32,R1),64(R3)
         BNE   F374XX              OP1C Different, error
         CLC   32(32,R1),96(R3)
         BNE   F374XX              OP2C Different, error
         CLC   64(32,R1),128(R3)
         BNE   F374XX              OP3C Different, error
         CLC   96(32,R1),160(R3)
         BE    0(R11)              OP4Csame, return

F374XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F374TSTCC BC   0,F374CCMSG        fail if unexpected condition code
F374CCMSG EQU  *
         BAL   R15,CCMSG
         B     F374CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSO: 37): 4th NE'

         TSTEND


* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSO: 37) instruction : All EQ
* ---------------------------------------------------------------------
         TSTHDR 0,99,37             All Equal (cc=0, UR= unchanged)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSO: 37)
* Build Parambter List
*
         MVC   0(128,R1),0(R2)     load Op1-Op4 data in source
         MVC   64(128,R3),0(R1)    copy to PL OP1-Op4 Compare

         MVC   0(128,R4),128(R2)   load initial Op1R-Op4R data
         MVC   192(128,R3),0(R4)   copy to PL Op1-Op4 Replacement
****
* Do NOT force a failure by changing an operand
****
*         MVC   96(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,32(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,37
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSO: 37)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F375TSTCC            fail if...

F375CKUR  EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F375CKRES
         BAL   R15,URMSG
*
* compare results
*
F375CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

* All equal; replacement values copied to operand source

         CLC   0(32,R1),192(R3)
         BNE   F375XX              OP1R Different, error
         CLC   32(32,R1),224(R3)
         BNE   F375XX              OP2R Different, error
         CLC   64(32,R1),256(R3)
         BNE   F375XX              OP3R Different, error
         CLC   96(32,R1),288(R3)
         BE    0(R11)              OP4R same, return

F375XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F375TSTCC BC    0,F375CCMSG        fail if unexpected condition code
F375CCMSG EQU   *
         BAL   R15,CCMSG
         B     F375CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSO: 37): All EQ'

         TSTEND
                                                                EJECT
*======================================================================
*       Quadruple Compare and Swap Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSX: 36) instruction : 1st NE
* ---------------------------------------------------------------------
         TSTHDR 2,1,36             1st Operand NOT Equal (cc=2, UR=1)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSX: 36)
* Build Parambter List
*
         MVC   0(64,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(64,R3),0(R1)     copy to PL OP1-Op4 Compare

         MVC   0(64,R4),128(R2)    load initial Op1-Op4 data
         MVC   192(64,R3),0(R4)    copy to PL Op1-Op4 Replacement
****
* force a failure by changing the first operand
****
         MVC   0(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,16(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,16(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,16(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,36
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSX: 36)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F361TSTCC            fail if...

F361CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F361CKRES
         BAL   R15,URMSG
*
* compare results
*
F361CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(16,R1),64(R3)
         BNE   F361XX              OP1C Different, error
         CLC   16(16,R1),80(R3)
         BNE   F361XX              OP2C Different, error
         CLC   32(16,R1),96(R3)
         BNE   F361XX              OP3C Different, error
         CLC   48(16,R1),112(R3)
         BE    0(R11)              OP4Csame, return

F361XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F361TSTCC BC    0,F361CCMSG        fail if unexpected condition code
F361CCMSG EQU   *
         BAL   R15,CCMSG
         B     F361CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSX: 36): 1st NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSX: 36) instruction : 2nd NE
* ---------------------------------------------------------------------
         TSTHDR 2,2,36             2nd Operand NOT Equal (cc=2, UR=2)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSX: 36)
* Build Parambter List
*
         MVC   0(64,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(64,R3),0(R1)     copy to PL OP1-Op4 Compare

         MVC   0(64,R4),128(R2)    load initial Op1-Op4 data
         MVC   192(64,R3),0(R4)    copy to PL Op1-Op4 Replacement
****
* force a failure by changing the 2nd operand
****
         MVC   16(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,16(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,16(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,16(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,36
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSX: 36)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F362TSTCC            fail if...

F362CKUR  EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F362CKRES
         BAL   R15,URMSG
*
* compare results
*
F362CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(16,R1),64(R3)
         BNE   F362XX              OP1C Different, error
         CLC   16(16,R1),80(R3)
         BNE   F362XX              OP2C Different, error
         CLC   32(16,R1),96(R3)
         BNE   F362XX              OP3C Different, error
         CLC   48(16,R1),112(R3)
         BE    0(R11)              OP4Csame, return

F362XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F362TSTCC BC    0,F362CCMSG        fail if unexpected condition code
F362CCMSG EQU   *
         BAL   R15,CCMSG
         B     F362CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSX: 36): 2nd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSX: 36) instruction : 3rd NE
* ---------------------------------------------------------------------
         TSTHDR 2,3,36             3rd Operand NOT Equal (cc=2, UR=3)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSX: 36)
* Build Parambter List
*
         MVC   0(64,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(64,R3),0(R1)     copy to PL OP1-Op4 Compare

         MVC   0(64,R4),128(R2)    load initial Op1-Op4 data
         MVC   192(64,R3),0(R4)    copy to PL Op1-Op4 Replacement
****
* force a failure by changing the 3nd operand
****
         MVC   32(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,16(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,16(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,16(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,36
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSX: 36)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F363TSTCC            fail if...

F363CKUR  EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F363CKRES
         BAL   R15,URMSG
*
* compare results
*
F363CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(16,R1),64(R3)
         BNE   F363XX              OP1C Different, error
         CLC   16(16,R1),80(R3)
         BNE   F363XX              OP2C Different, error
         CLC   32(16,R1),96(R3)
         BNE   F363XX              OP3C Different, error
         CLC   48(16,R1),112(R3)
         BE    0(R11)              OP4Csame, return

F363XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F363TSTCC BC    0,F363CCMSG        fail if unexpected condition code
F363CCMSG EQU   *
         BAL   R15,CCMSG
         B     F363CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSX: 36): 3rd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSX: 36) instruction : 4th NE
* ---------------------------------------------------------------------
         TSTHDR 2,4,36             4th Operand NOT Equal (cc=2, UR=4)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSX: 36)
* Build Parambter List
*
         MVC   0(64,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(64,R3),0(R1)     copy to PL OP1-Op4 Compare

         MVC   0(64,R4),128(R2)    load initial Op1-Op4 data
         MVC   192(64,R3),0(R4)    copy to PL Op1-Op4 Replacement
****
* force a failure by changing the 4th operand
****
         MVC   48(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,16(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,16(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,16(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,36
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSX: 36)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F364TSTCC            fail if...

F364CKUR  EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F364CKRES
         BAL   R15,URMSG
*
* compare results
*
F364CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(16,R1),64(R3)
         BNE   FXX437              OP1C Different, error
         CLC   16(16,R1),80(R3)
         BNE   FXX437              OP2C Different, error
         CLC   32(16,R1),96(R3)
         BNE   FXX437              OP3C Different, error
         CLC   48(16,R1),112(R3)
         BE    0(R11)              OP4Csame, return

FXX437   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F364TSTCC BC    0,F364CCMSG        fail if unexpected condition code
F364CCMSG EQU   *
         BAL   R15,CCMSG
         B     F364CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSX: 36): 4th NE'

         TSTEND


* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSX: 36) instruction : All EQ
* ---------------------------------------------------------------------
         TSTHDR 0,99,36             All Equal (cc=0, UR= unchanged)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSX: 36)
* Build Parambter List
*
         MVC   0(64,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(64,R3),0(R1)     copy to PL OP1-Op4 Compare

         MVC   0(64,R4),128(R2)    load initial Op1R-Op4R data
         MVC   192(64,R3),0(R4)    copy to PL Op1-Op4 Replacement
****
* Do NOT force a failure by changing an operand
****
*         MVC   96(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,16(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,16(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,16(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,36
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSX: 36)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F365TSTCC            fail if...

F365CKUR  EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F365CKRES
         BAL   R15,URMSG
*
* compare results
*
F365CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

* All equal; replacement values copied to operand source

         CLC   0(16,R1),128(R3)
         BNE   FXX537              OP1R Different, error
         CLC   16(16,R1),144(R3)
         BNE   FXX537              OP2R Different, error
         CLC   32(16,R1),160(R3)
         BNE   FXX537              OP3R Different, error
         CLC   48(16,R1),176(R3)
         BE    0(R11)              OP4R same, return

FXX537   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F365TSTCC BC    0,F365CCMSG        fail if unexpected condition code
F365CCMSG EQU   *
         BAL   R15,CCMSG
         B     F365CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSX: 36): All EQ'

         TSTEND
                                                                EJECT
*======================================================================
*       Quadruple Compare and Swap Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSG: 35) instruction : 1st NE
* ---------------------------------------------------------------------
         TSTHDR 2,1,35             1st Operand NOT Equal (cc=2, UR=1)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSG: 35)
* Build Parambter List
*
         MVC   0(32,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(32,R3),0(R1)     copy to PL OP1-Op4 Compare

         MVC   0(32,R4),128(R2)    load initial Op1-Op4 data
         MVC   128(32,R3),0(R4)    copy to PL Op1-Op4 Replacement
****
* force a failure by changing the first operand
****
         MVC   0(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,8(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,8(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,8(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,35
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSG: 35)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F351TSTCC            fail if...

F351CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F351CKRES
         BAL   R15,URMSG
*
* compare results
*
F351CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(8,R1),64(R3)
         BNE   F351XX              OP1C Different, error
         CLC   8(8,R1),72(R3)
         BNE   F351XX              OP2C Different, error
         CLC   16(8,R1),80(R3)
         BNE   F351XX              OP3C Different, error
         CLC   24(8,R1),88(R3)
         BE    0(R11)              OP4Csame, return

F351XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F351TSTCC BC    0,F351CCMSG        fail if unexpected condition code
F351CCMSG EQU   *
         BAL   R15,CCMSG
         B     F351CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSG: 35): 1st NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSG: 35) instruction : 2nd NE
* ---------------------------------------------------------------------
         TSTHDR 2,2,35             2nd Operand NOT Equal (cc=2, UR=2)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSG: 35)
* Build Parambter List
*
         MVC   0(32,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(32,R3),0(R1)     copy to PL OP1-Op4 Compare

         MVC   0(32,R4),128(R2)    load initial Op1-Op4 data
         MVC   128(32,R3),0(R4)    copy to PL Op1-Op4 Replacement
****
* force a failure by changing the 2nd operand
****
         MVC   8(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,8(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,8(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,8(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,35
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSG: 35)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F352TSTCC            fail if...

F352CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F352CKRES
         BAL   R15,URMSG
*
* compare results
*
F352CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(8,R1),64(R3)
         BNE   F352XX              OP1C Different, error
         CLC   8(8,R1),72(R3)
         BNE   F352XX              OP2C Different, error
         CLC   16(8,R1),80(R3)
         BNE   F352XX              OP3C Different, error
         CLC   24(8,R1),88(R3)
         BE    0(R11)              OP4Csame, return

F352XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F352TSTCC BC    0,F352CCMSG        fail if unexpected condition code
F352CCMSG EQU   *
         BAL   R15,CCMSG
         B     F352CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSG: 35): 2nd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSG: 35) instruction : 3rd NE
* ---------------------------------------------------------------------
         TSTHDR 2,3,35             3rd Operand NOT Equal (cc=2, UR=3)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSG: 35)
* Build Parambter List
*
         MVC   0(32,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(32,R3),0(R1)     copy to PL OP1-Op4 Compare

         MVC   0(32,R4),128(R2)    load initial Op1-Op4 data
         MVC   128(32,R3),0(R4)    copy to PL Op1-Op4 Replacement
****
* force a failure by changing the 3rd operand
****
         MVC   16(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,8(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,8(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,8(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,35
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSG: 35)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F353TSTCC            fail if...

F353CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F353CKRES
         BAL   R15,URMSG
*
* compare results
*
F353CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(8,R1),64(R3)
         BNE   F353XX              OP1C Different, error
         CLC   8(8,R1),72(R3)
         BNE   F353XX              OP2C Different, error
         CLC   16(8,R1),80(R3)
         BNE   F353XX              OP3C Different, error
         CLC   24(8,R1),88(R3)
         BE    0(R11)              OP4Csame, return

F353XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F353TSTCC BC   0,F353CCMSG          fail if unexpected condition code
F353CCMSG EQU   *
         BAL   R15,CCMSG
         B     F353CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSG: 35): 3rd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSG: 35) instruction : 4th NE
* ---------------------------------------------------------------------
         TSTHDR 2,4,35             4th Operand NOT Equal (cc=2, UR=4)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSG: 35)
* Build Parambter List
*
         MVC   0(32,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(32,R3),0(R1)     copy to PL OP1-Op4 Compare

         MVC   0(32,R4),128(R2)    load initial Op1-Op4 data
         MVC   128(32,R3),0(R4)    copy to PL Op1-Op4 Replacement
****
* force a failure by changing the 3rd operand
****
         MVC   24(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,8(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,8(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,8(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,35
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSG: 35)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F354TSTCC            fail if...

F354CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F354CKRES
         BAL   R15,URMSG
*
* compare results
*
F354CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(8,R1),64(R3)
         BNE   F354XX              OP1C Different, error
         CLC   8(8,R1),72(R3)
         BNE   F354XX              OP2C Different, error
         CLC   16(8,R1),80(R3)
         BNE   F354XX              OP3C Different, error
         CLC   24(8,R1),88(R3)
         BE    0(R11)              OP4Csame, return

F354XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F354TSTCC BC   0,F354CCMSG         fail if unexpected condition code
F354CCMSG EQU   *
         BAL   R15,CCMSG
         B     F354CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSG: 35): 4th NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCSG: 35) instruction : All EQ
* ---------------------------------------------------------------------
         TSTHDR 0,99,35             ALL EQ Equal (cc=0, UR=99

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCSG: 35)
* Build Parambter List
*
         MVC   0(32,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(32,R3),0(R1)     copy to PL OP1-Op4 Compare

         MVC   0(32,R4),128(R2)    load initial Op1-Op4 data
         MVC   128(32,R3),0(R4)    copy to PL Op1-Op4 Replacement
****
* force a failure by changing the 3rd operand
****
*        MVC   24(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,8(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,8(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,8(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,35
         PLO   0,0,0,0(R3)         Quadruple C/S (QCSG: 35)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F355TSTCC            fail if...

F355CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F355CKRES
         BAL   R15,URMSG
*
* compare results
*
F355CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

* All equal; replacement values copied to operand source

         CLC   0(8,R1),96(R3)
         BNE   F355XX              OP1R Different, error
         CLC   8(8,R1),104(R3)
         BNE   F355XX              OP2R Different, error
         CLC   16(8,R1),112(R3)
         BNE   F355XX              OP3R Different, error
         CLC   24(8,R1),120(R3)
         BE    0(R11)              OP4R same, return

F355XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F355TSTCC BC   0,F355CCMSG         fail if unexpected condition code
F355CCMSG EQU   *
         BAL   R15,CCMSG
         B     F355CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCSG: 35): All EQ'

         TSTEND
                                                                EJECT
*======================================================================
*       Quadruple Compare and Swap Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCS: 34) instruction : 1st NE
* ---------------------------------------------------------------------
         TSTHDR 2,1,34             1st Operand NOT Equal (cc=2, UR=1)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCS: 34)
* Build Parambter List
*
         MVC   0(16,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(4,R3),0(R1)      copy to PL OP1-Compare
         MVC   72(4,R3),4(R1)      copy to PL OP2-Compare
         MVC   80(4,R3),8(R1)      copy to PL OP3-Compare
         MVC   88(4,R3),12(R1)     copy to PL OP4-Compare

         MVC   0(32,R4),128(R2)    load initial Op1-Op4 data
         MVC   96(4,R3),0(R4)      copy to PL OP1-Replacement
         MVC   104(4,R3),4(R4)     copy to PL OP2-Replacement
         MVC   112(4,R3),8(R4)     copy to PL OP3-Replacement
         MVC   120(4,R3),12(R4)    copy to PL OP4-Replacement
****
* force a failure by changing the first operand
****
         MVC   0(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,4(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,4(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,4(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,34
         PLO   0,0,0,0(R3)         Quadruple C/S (QCS: 34)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F341TSTCC            fail if...

F341CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F341CKRES
         BAL   R15,URMSG
*
* compare results
*
F341CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(4,R1),64(R3)
         BNE   F341XX              OP1C Different, error
         CLC   4(4,R1),72(R3)
         BNE   F341XX              OP2C Different, error
         CLC   8(4,R1),80(R3)
         BNE   F341XX              OP3C Different, error
         CLC   12(4,R1),88(R3)
         BE    0(R11)              OP4Csame, return

F341XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F341TSTCC BC    0,F341CCMSG        fail if unexpected condition code
F341CCMSG EQU   *
         BAL   R15,CCMSG
         B     F341CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCS: 34): 1st NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCS: 34) instruction : 2nd NE
* ---------------------------------------------------------------------
         TSTHDR 2,2,34             2nd Operand NOT Equal (cc=2, UR=2)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCS: 34)
* Build Parambter List
*
         MVC   0(16,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(4,R3),0(R1)      copy to PL OP1-Compare
         MVC   72(4,R3),4(R1)      copy to PL OP2-Compare
         MVC   80(4,R3),8(R1)      copy to PL OP3-Compare
         MVC   88(4,R3),12(R1)     copy to PL OP4-Compare

         MVC   0(32,R4),128(R2)    load initial Op1-Op4 data
         MVC   96(4,R3),0(R4)      copy to PL OP1-Replacement
         MVC   104(4,R3),4(R4)     copy to PL OP2-Replacement
         MVC   112(4,R3),8(R4)     copy to PL OP3-Replacement
         MVC   120(4,R3),12(R4)    copy to PL OP4-Replacement
****
* force a failure by changing the 2nd operand
****
         MVC   4(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,4(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,4(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,4(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,34
         PLO   0,0,0,0(R3)         Quadruple C/S (QCS: 34)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F342TSTCC            fail if...

F342CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F342CKRES
         BAL   R15,URMSG
*
* compare results
*
F342CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(4,R1),64(R3)
         BNE   F342XX              OP1C Different, error
         CLC   4(4,R1),72(R3)
         BNE   F342XX              OP2C Different, error
         CLC   8(4,R1),80(R3)
         BNE   F342XX              OP3C Different, error
         CLC   12(4,R1),88(R3)
         BE    0(R11)              OP4Csame, return

F342XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F342TSTCC BC    0,F342CCMSG        fail if unexpected condition code
F342CCMSG EQU   *
         BAL   R15,CCMSG
         B     F342CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCS: 34): 2nd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCS: 34) instruction : 3rd NE
* ---------------------------------------------------------------------
         TSTHDR 2,3,34             3rd Operand NOT Equal (cc=2, UR=3)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCS: 34)
* Build Parambter List
*
         MVC   0(16,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(4,R3),0(R1)      copy to PL OP1-Compare
         MVC   72(4,R3),4(R1)      copy to PL OP2-Compare
         MVC   80(4,R3),8(R1)      copy to PL OP3-Compare
         MVC   88(4,R3),12(R1)     copy to PL OP4-Compare

         MVC   0(32,R4),128(R2)    load initial Op1-Op4 data
         MVC   96(4,R3),0(R4)      copy to PL OP1-Replacement
         MVC   104(4,R3),4(R4)     copy to PL OP2-Replacement
         MVC   112(4,R3),8(R4)     copy to PL OP3-Replacement
         MVC   120(4,R3),12(R4)    copy to PL OP4-Replacement
****
* force a failure by changing the 3rd operand
****
         MVC   8(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,4(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,4(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,4(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,34
         PLO   0,0,0,0(R3)         Quadruple C/S (QCS: 34)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F343TSTCC            fail if...

F343CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F343CKRES
         BAL   R15,URMSG
*
* compare results
*
F343CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(4,R1),64(R3)
         BNE   F343XX              OP1C Different, error
         CLC   4(4,R1),72(R3)
         BNE   F343XX              OP2C Different, error
         CLC   8(4,R1),80(R3)
         BNE   F343XX              OP3C Different, error
         CLC   12(4,R1),88(R3)
         BE    0(R11)              OP4Csame, return

F343XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F343TSTCC BC    0,F343CCMSG        fail if unexpected condition code
F343CCMSG EQU   *
         BAL   R15,CCMSG
         B     F343CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCS: 34): 3rd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCS: 34) instruction : 4th NE
* ---------------------------------------------------------------------
         TSTHDR 2,4,34             4th Operand NOT Equal (cc=2, UR=4)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCS: 34)
* Build Parambter List
*
         MVC   0(16,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(4,R3),0(R1)      copy to PL OP1-Compare
         MVC   72(4,R3),4(R1)      copy to PL OP2-Compare
         MVC   80(4,R3),8(R1)      copy to PL OP3-Compare
         MVC   88(4,R3),12(R1)     copy to PL OP4-Compare

         MVC   0(32,R4),128(R2)    load initial Op1-Op4 data
         MVC   96(4,R3),0(R4)      copy to PL OP1-Replacement
         MVC   104(4,R3),4(R4)     copy to PL OP2-Replacement
         MVC   112(4,R3),8(R4)     copy to PL OP3-Replacement
         MVC   120(4,R3),12(R4)    copy to PL OP4-Replacement
****
* force a failure by changing the 4th operand
****
         MVC   12(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,4(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,4(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,4(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,34
         PLO   0,0,0,0(R3)         Quadruple C/S (QCS: 34)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F344TSTCC            fail if...

F344CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F344CKRES
         BAL   R15,URMSG
*
* compare results
*
F344CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(4,R1),64(R3)
         BNE   F344XX              OP1C Different, error
         CLC   4(4,R1),72(R3)
         BNE   F344XX              OP2C Different, error
         CLC   8(4,R1),80(R3)
         BNE   F344XX              OP3C Different, error
         CLC   12(4,R1),88(R3)
         BE    0(R11)              OP4Csame, return

F344XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F344TSTCC BC    0,F344CCMSG        fail if unexpected condition code
F344CCMSG EQU   *
         BAL   R15,CCMSG
         B     F344CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCS: 34): 4th NE'

         TSTEND


* ---------------------------------------------------------------------
* Test Quadruple Compare and Swap (QCS: 34) instruction : All EQ
* ---------------------------------------------------------------------
         TSTHDR 0,99,34             All EQ (cc=0, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Quadruple Compare and Swap (QCS: 34)
* Build Parambter List
*
         MVC   0(16,R1),0(R2)      load Op1-Op4 data in source
         MVC   64(4,R3),0(R1)      copy to PL OP1-Compare
         MVC   72(4,R3),4(R1)      copy to PL OP2-Compare
         MVC   80(4,R3),8(R1)      copy to PL OP3-Compare
         MVC   88(4,R3),12(R1)     copy to PL OP4-Compare

         MVC   0(32,R4),128(R2)    load initial Op1-Op4 data
         MVC   96(4,R3),0(R4)      copy to PL OP1-Replacement
         MVC   104(4,R3),4(R4)     copy to PL OP2-Replacement
         MVC   112(4,R3),8(R4)     copy to PL OP3-Replacement
         MVC   120(4,R3),12(R4)    copy to PL OP4-Replacement
****
* ALL EQ: do not force a failure by changing an operand
****
*         MVC   12(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,4(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,4(R1)
         STG   R1,16(R3)           load OP3 address in PL

         LA    R1,4(R1)
         STG   R1,24(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,34
         PLO   0,0,0,0(R3)         Quadruple C/S (QCS: 34)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F345TSTCC            fail if...
*
* compare results
*
F345CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

* All equal; replacement values copied to operand source

         CLC   0(4,R1),96(R3)
         BNE   F345XX              OP1R Different, error
         CLC   4(4,R1),104(R3)
         BNE   F345XX              OP2R Different, error
         CLC   8(4,R1),112(R3)
         BNE   F345XX              OP3R Different, error
         CLC   12(4,R1),120(R3)
         BE    0(R11)              OP4R same, return

F345XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F345TSTCC BC    0,F345CCMSG        fail if unexpected condition code
F345CCMSG EQU   *
         BAL   R15,CCMSG
         B     F345CKRES

         TSTDATA
         TSTLBL OPDST              Dest/Data Op1-R, Op2-R, Op3-R, Op4-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP4
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Quadruple Compare and Swap (QCS: 34): All EQ'

         TSTEND
                                                                EJECT
*======================================================================
*       Triple Compare and Swap Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCSO: 33) instruction : 1st NE
* ---------------------------------------------------------------------
         TSTHDR 2,1,33             1st Operand NOT Equal (cc=2, UR=1)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCSO: 33)
* Build Parambter List
*
         MVC   0(96,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(96,R3),0(R1)     copy to PL OP1-Op3 Compare

         MVC   0(96,R4),128(R2)    load initial Op1-Op3 data
         MVC   160(96,R3),0(R4)    copy to PL Op1-Op3 Replacement
****
* force a failure by changing the first operand
****
         MVC   0(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,33
         PLO   0,0,0,0(R3)         Triple C/S (TCSO: 33)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F331TSTCC            fail if...

F331CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F331CKRES
         BAL   R15,URMSG
*
* compare results
*
F331CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(32,R1),64(R3)
         BNE   F331XX              OP1C Different, error
         CLC   32(32,R1),96(R3)
         BNE   F331XX              OP2C Different, error
         CLC   64(32,R1),128(R3)
         BE    0(R11)              OP3C same, return

F331XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F331TSTCC BC   0,F331CCMSG         fail if unexpected condition code
F331CCMSG EQU  *
         BAL   R15,CCMSG
         B     F331CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCSO: 33): 1st NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCSO: 33) instruction : 2nd NE
* ---------------------------------------------------------------------
         TSTHDR 2,2,33             2nd Operand NOT Equal (cc=2, UR=2)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCSO: 33)
* Build Parambter List
*
         MVC   0(96,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(96,R3),0(R1)     copy to PL OP1-Op3 Compare

         MVC   0(96,R4),128(R2)    load initial Op1-Op3 data
         MVC   160(96,R3),0(R4)    copy to PL Op1-Op3 Replacement
****
* force a failure by changing the 2nd operand
****
         MVC   32(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,33
         PLO   0,0,0,0(R3)         Triple C/S (TCSO: 33)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F332TSTCC            fail if...

F332CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F332CKRES
         BAL   R15,URMSG
*
* compare results
*
F332CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(32,R1),64(R3)
         BNE   F332XX              OP1C Different, error
         CLC   32(32,R1),96(R3)
         BNE   F332XX              OP2C Different, error
         CLC   64(32,R1),128(R3)
         BE    0(R11)              OP3C same, return

F332XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F332TSTCC BC   0,F332CCMSG         fail if unexpected condition code
F332CCMSG EQU  *
         BAL   R15,CCMSG
         B     F332CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCSO: 33): 2nd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCSO: 33) instruction : 3rd NE
* ---------------------------------------------------------------------
         TSTHDR 2,3,33             3rd Operand NOT Equal (cc=2, UR=3)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCSO: 33)
* Build Parambter List
*
         MVC   0(96,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(96,R3),0(R1)     copy to PL OP1-Op3 Compare

         MVC   0(96,R4),128(R2)    load initial Op1-Op3 data
         MVC   160(96,R3),0(R4)    copy to PL Op1-Op3 Replacement
****
* force a failure by changing the 3rd operand
****
         MVC   64(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,33
         PLO   0,0,0,0(R3)         Triple C/S (TCSO: 33)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F333TSTCC            fail if...

F333CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F333CKRES
         BAL   R15,URMSG
*
* compare results
*
F333CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(32,R1),64(R3)
         BNE   F333XX              OP1C Different, error
         CLC   32(32,R1),96(R3)
         BNE   F333XX              OP2C Different, error
         CLC   64(32,R1),128(R3)
         BE    0(R11)              OP3C same, return

F333XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F333TSTCC BC   0,F333CCMSG         fail if unexpected condition code
F333CCMSG EQU  *
         BAL   R15,CCMSG
         B     F333CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCSO: 33): 3rd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCSO: 33) instruction : ALL EQ
* ---------------------------------------------------------------------
         TSTHDR 0,99,33             ALL EQ (cc=0, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCSO: 33)
* Build Parambter List
*
         MVC   0(96,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(96,R3),0(R1)     copy to PL OP1-Op3 Compare

         MVC   0(96,R4),128(R2)    load initial Op1-Op3 data
         MVC   160(96,R3),0(R4)    copy to PL Op1-Op3 Replacement
****
* Do NOTforce a failure by changing an operand
****
*        MVC   64(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,32(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,32(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,33
         PLO   0,0,0,0(R3)         Triple C/S (TCSO: 33)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F334TSTCC            fail if...
*
* compare results
*
F334CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

* All equal; replacement values copied to operand source

         CLC   0(32,R1),160(R3)
         BNE   F334XX              OP1R Different, error
         CLC   32(32,R1),192(R3)
         BNE   F334XX              OP2R Different, error
         CLC   64(32,R1),224(R3)
         BE    0(R11)

F334XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F334TSTCC BC   0,F334CCMSG         fail if unexpected condition code
F334CCMSG EQU  *
         BAL   R15,CCMSG
         B     F334CKRES

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCSO: 33):  ALL EQ'

         TSTEND
                                                                EJECT
*======================================================================
*       Triple Compare and Swap Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCSX: 32) instruction : 1st NE
* ---------------------------------------------------------------------
         TSTHDR 2,1,32             1st Operand NOT Equal (cc=2, UR=1)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCSX: 32)
* Build Parambter List
*
         MVC   0(48,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(48,R3),0(R1)     copy to PL OP1-Op3 Compare

         MVC   0(48,R4),128(R2)    load initial Op1-Op3 data
         MVC   160(48,R3),0(R4)    copy to PL Op1-Op3 Replacement
****
* force a failure by changing the 1st operand
****
         MVC   0(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,16(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,16(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,32
         PLO   0,0,0,0(R3)         Triple C/S (TCSX: 32)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F321TSTCC            fail if...

F321CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F321CKRES
         BAL   R15,URMSG
*
* compare results
*
F321CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(16,R1),64(R3)
         BNE   F321XX              OP1C Different, error
         CLC   16(16,R1),80(R3)
         BNE   F321XX              OP2C Different, error
         CLC   32(16,R1),96(R3)
         BE    0(R11)              OP3C same, return

F321XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F321TSTCC BC   0,F321CCMSG         fail if unexpected condition code
F321CCMSG EQU  *
         BAL   R15,CCMSG
         B     F321CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCSX: 32): 1st NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCSX: 32) instruction : 2nd NE
* ---------------------------------------------------------------------
         TSTHDR 2,2,32             2nd Operand NOT Equal (cc=2, UR=2)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCSX: 32)
* Build Parambter List
*
         MVC   0(48,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(48,R3),0(R1)     copy to PL OP1-Op3 Compare

         MVC   0(48,R4),128(R2)    load initial Op1-Op3 data
         MVC   160(48,R3),0(R4)    copy to PL Op1-Op3 Replacement
****
* force a failure by changing the 2nd operand
****
         MVC   16(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,16(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,16(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,32
         PLO   0,0,0,0(R3)         Triple C/S (TCSX: 32)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F322TSTCC            fail if...

F322CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F322CKRES
         BAL   R15,URMSG
*
* compare results
*
F322CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(16,R1),64(R3)
         BNE   F322XX              OP1C Different, error
         CLC   16(16,R1),80(R3)
         BNE   F322XX              OP2C Different, error
         CLC   32(16,R1),96(R3)
         BE    0(R11)              OP3C same, return

F322XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F322TSTCC BC   0,F322CCMSG         fail if unexpected condition code
F322CCMSG EQU  *
         BAL   R15,CCMSG
         B     F322CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCSX: 32): 2nd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCSX: 32) instruction : 3nd NE
* ---------------------------------------------------------------------
         TSTHDR 2,3,32             3nd Operand NOT Equal (cc=2, UR=3)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCSX: 32)
* Build Parambter List
*
         MVC   0(48,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(48,R3),0(R1)     copy to PL OP1-Op3 Compare

         MVC   0(48,R4),128(R2)    load initial Op1-Op3 data
         MVC   160(48,R3),0(R4)    copy to PL Op1-Op3 Replacement
****
* force a failure by changing the 3nd operand
****
         MVC   32(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,16(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,16(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,32
         PLO   0,0,0,0(R3)         Triple C/S (TCSX: 32)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F323TSTCC            fail if...

F323CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F323CKRES
         BAL   R15,URMSG
*
* compare results
*
F323CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(16,R1),64(R3)
         BNE   F323XX              OP1C Different, error
         CLC   16(16,R1),80(R3)
         BNE   F323XX              OP2C Different, error
         CLC   32(16,R1),96(R3)
         BE    0(R11)              OP3C same, return

F323XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F323TSTCC BC   0,F323CCMSG         fail if unexpected condition code
F323CCMSG EQU  *
         BAL   R15,CCMSG
         B     F323CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCSX: 32): 3nd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCSX: 32) instruction : All EQ
* ---------------------------------------------------------------------
         TSTHDR 0,99,32             All Equal (cc=0, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCSX: 32)
* Build Parambter List
*
         MVC   0(48,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(48,R3),0(R1)     copy to PL OP1-Op3 Compare

         MVC   0(48,R4),128(R2)    load initial Op1-Op3 data
         MVC   160(48,R3),0(R4)    copy to PL Op1-Op3 Replacement
****
* Do Not force a failure by changing the 3nd operand
****
*         MVC   32(8,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,16(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,16(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,32
         PLO   0,0,0,0(R3)         Triple C/S (TCSX: 32)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F324TSTCC            fail if...
*
* compare results
*
F324CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

* All equal; replacement values copied to operand source

         CLC   0(16,R1),112(R3)
         BNE   F324XX              OP1R Different, error
         CLC   16(16,R1),128(R3)
         BNE   F324XX              OP2R Different, error
         CLC   32(16,R1),144(R3)
         BE    0(R11)

F324XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F324TSTCC BC   0,F324CCMSG         fail if unexpected condition code
F324CCMSG EQU  *
         BAL   R15,CCMSG
         B     F324CKRES

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCSX: 32): All EQ'

         TSTEND

                                                                EJECT
*======================================================================
*       Triple Compare and Swap Tests
*=======================================================================

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCSG: 31) instruction : 1st NE
* ---------------------------------------------------------------------
         TSTHDR 2,1,31             1st Operand NOT Equal (cc=2, UR=1)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCSG: 31)
* Build Parambter List
*
         MVC   0(24,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(24,R3),0(R1)     copy to PL OP1-Op3 Compare

         MVC   0(24,R4),128(R2)    load initial Op1-Op3 data
         MVC   88(24,R3),0(R4)     copy to PL Op1-Op3 Replacement
****
* force a failure by changing the 1st operand
****
         MVC   0(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,8(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,8(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,31
         PLO   0,0,0,0(R3)         Triple C/S (TCSG: 31)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F311TSTCC            fail if...

F311CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F311CKRES
         BAL   R15,URMSG
*
* compare results
*
F311CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(8,R1),64(R3)
         BNE   F311XX              OP1C Different, error
         CLC   8(8,R1),72(R3)
         BNE   F311XX              OP2C Different, error
         CLC   16(8,R1),80(R3)
         BE    0(R11)              OP3C same, return

F311XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F311TSTCC BC   0,F311CCMSG         fail if unexpected condition code
F311CCMSG EQU  *
         BAL   R15,CCMSG
         B     F311CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCSG: 31): 1st NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCSG: 31) instruction : 2nd NE
* ---------------------------------------------------------------------
         TSTHDR 2,2,31             2nd Operand NOT Equal (cc=2, UR=2)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCSG: 31)
* Build Parambter List
*
         MVC   0(24,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(24,R3),0(R1)     copy to PL OP1-Op3 Compare

         MVC   0(24,R4),128(R2)    load initial Op1-Op3 data
         MVC   88(24,R3),0(R4)     copy to PL Op1-Op3 Replacement
****
* force a failure by changing the 2nd operand
****
         MVC   8(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,8(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,8(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,31
         PLO   0,0,0,0(R3)         Triple C/S (TCSG: 31)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F312TSTCC            fail if...

F312CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F312CKRES
         BAL   R15,URMSG
*
* compare results
*
F312CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(8,R1),64(R3)
         BNE   F312XX              OP1C Different, error
         CLC   8(8,R1),72(R3)
         BNE   F312XX              OP2C Different, error
         CLC   16(8,R1),80(R3)
         BE    0(R11)              OP3C same, return

F312XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F312TSTCC BC   0,F312CCMSG         fail if unexpected condition code
F312CCMSG EQU  *
         BAL   R15,CCMSG
         B     F312CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCSG: 31): 2nd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCSG: 31) instruction : 3rd NE
* ---------------------------------------------------------------------
         TSTHDR 2,3,31             3rd Operand NOT Equal (cc=2, UR=3)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCSG: 31)
* Build Parambter List
*
         MVC   0(24,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(24,R3),0(R1)     copy to PL OP1-Op3 Compare

         MVC   0(24,R4),128(R2)    load initial Op1-Op3 data
         MVC   88(24,R3),0(R4)     copy to PL Op1-Op3 Replacement
****
* force a failure by changing the 3rd operand
****
         MVC   16(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,8(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,8(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,31
         PLO   0,0,0,0(R3)         Triple C/S (TCSG: 31)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F313TSTCC            fail if...

F313CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F313CKRES
         BAL   R15,URMSG
*
* compare results
*
F313CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(8,R1),64(R3)
         BNE   F313XX              OP1C Different, error
         CLC   8(8,R1),72(R3)
         BNE   F313XX              OP2C Different, error
         CLC   16(8,R1),80(R3)
         BE    0(R11)              OP3C same, return

F313XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F313TSTCC BC   0,F313CCMSG         fail if unexpected condition code
F313CCMSG EQU  *
         BAL   R15,CCMSG
         B     F313CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCSG: 31): 3rd NE'

         TSTEND

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCSG: 31) instruction : All EQ
* ---------------------------------------------------------------------
         TSTHDR 0,99,31            All Operands Equal (cc=0, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCSG: 31)
* Build Parambter List
*
         MVC   0(24,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(24,R3),0(R1)     copy to PL OP1-Op3 Compare

         MVC   0(24,R4),128(R2)    load initial Op1-Op3 data
         MVC   88(24,R3),0(R4)     copy to PL Op1-Op3 Replacement
****
* do not force a failure by changing the 3rd operand
****
*         MVC   16(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,8(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,8(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,31
         PLO   0,0,0,0(R3)         Triple C/S (TCSG: 31)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F314TSTCC            fail if...
*
* compare results
*
F314CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

* All equal; replacement values copied to operand source

         CLC   0(8,R1),88(R3)
         BNE   F314XX              OP1R Different, error
         CLC   8(8,R1),96(R3)
         BNE   F314XX              OP2R Different, error
         CLC   16(8,R1),104(R3)
         BE    0(R11)

F314XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F314TSTCC BC   0,F314CCMSG         fail if unexpected condition code
F314CCMSG EQU  *
         BAL   R15,CCMSG
         B     F314CKRES

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCSG: 31): All EQ'

         TSTEND
                                                                EJECT
*======================================================================
*       Triple Compare and Swap Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCS: 30) instruction : 1st NE
* ---------------------------------------------------------------------
         TSTHDR 2,1,30             1st Operand NOT Equal (cc=2, UR=1)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCS: 30)
* Build Parambter List
*
         MVC   0(12,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(4,R3),0(R1)      copy to PL OP1-Compare
         MVC   72(4,R3),4(R1)      copy to PL OP2-Compare
         MVC   80(4,R3),8(R1)      copy to PL OP3-Compare

         MVC   0(12,R4),128(R2)    load initial Op1-Op3 data
         MVC   88(4,R3),0(R4)      copy to PL OP1-Replacement
         MVC   96(4,R3),4(R4)      copy to PL OP2-Replacement
         MVC   104(4,R3),8(R4)     copy to PL OP3-Replacement

****
* force a failure by changing the 1st operand
****
         MVC   0(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,4(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,4(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,30
         PLO   0,0,0,0(R3)         Triple C/S (TCS: 30)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F301TSTCC            fail if...

F301CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F301CKRES
         BAL   R15,URMSG
*
* compare results
*
F301CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(4,R1),64(R3)
         BNE   F301XX              OP1C Different, error
         CLC   4(4,R1),72(R3)
         BNE   F301XX              OP2C Different, error
         CLC   8(4,R1),80(R3)
         BE    0(R11)              OP3C same, return

F301XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F301TSTCC BC   0,F301CCMSG         fail if unexpected condition code
F301CCMSG EQU  *
         BAL   R15,CCMSG
         B     F301CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCS: 30): 1st NE'
         TSTEND

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCS: 30) instruction : 2nd NE
* ---------------------------------------------------------------------
         TSTHDR 2,2,30             2nd Operand NOT Equal (cc=2, UR=2)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCS: 30)
* Build Parambter List
*
         MVC   0(12,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(4,R3),0(R1)      copy to PL OP1-Compare
         MVC   72(4,R3),4(R1)      copy to PL OP2-Compare
         MVC   80(4,R3),8(R1)      copy to PL OP3-Compare

         MVC   0(12,R4),128(R2)    load initial Op1-Op3 data
         MVC   88(4,R3),0(R4)      copy to PL OP1-Replacement
         MVC   96(4,R3),4(R4)      copy to PL OP2-Replacement
         MVC   104(4,R3),8(R4)     copy to PL OP3-Replacement

****
* force a failure by changing the 2nd operand
****
         MVC   4(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,4(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,4(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,30
         PLO   0,0,0,0(R3)         Triple C/S (TCS: 30)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F302TSTCC            fail if...

F302CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F302CKRES
         BAL   R15,URMSG
*
* compare results
*
F302CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(4,R1),64(R3)
         BNE   F302XX              OP1C Different, error
         CLC   4(4,R1),72(R3)
         BNE   F302XX              OP2C Different, error
         CLC   8(4,R1),80(R3)
         BE    0(R11)              OP3C same, return

F302XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F302TSTCC BC   0,F302CCMSG         fail if unexpected condition code
F302CCMSG EQU  *
         BAL   R15,CCMSG
         B     F302CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCS: 30): 2nd NE'
         TSTEND

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCS: 30) instruction : 3rd NE
* ---------------------------------------------------------------------
         TSTHDR 2,3,30             3rd Operand NOT Equal (cc=2, UR=3)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCS: 30)
* Build Parambter List
*
         MVC   0(12,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(4,R3),0(R1)      copy to PL OP1-Compare
         MVC   72(4,R3),4(R1)      copy to PL OP2-Compare
         MVC   80(4,R3),8(R1)      copy to PL OP3-Compare

         MVC   0(12,R4),128(R2)    load initial Op1-Op3 data
         MVC   88(4,R3),0(R4)      copy to PL OP1-Replacement
         MVC   96(4,R3),4(R4)      copy to PL OP2-Replacement
         MVC   104(4,R3),8(R4)     copy to PL OP3-Replacement

****
* force a failure by changing the 3rd operand
****
         MVC   8(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,4(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,4(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,30
         PLO   0,0,0,0(R3)         Triple C/S (TCS: 30)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F303TSTCC            fail if...

F303CKUR EQU   *
         MVC   URFOUND,48(R3)      Extract UR
         CLC   UR,48(R3)
         BE    F303CKRES
         BAL   R15,URMSG
*
* compare results
*
F303CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

         CLC   0(4,R1),64(R3)
         BNE   F303XX              OP1C Different, error
         CLC   4(4,R1),72(R3)
         BNE   F303XX              OP2C Different, error
         CLC   8(4,R1),80(R3)
         BE    0(R11)              OP3C same, return

F303XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F303TSTCC BC   0,F303CCMSG         fail if unexpected condition code
F303CCMSG EQU  *
         BAL   R15,CCMSG
         B     F303CKUR

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCS: 30): 3rd NE'
         TSTEND

* ---------------------------------------------------------------------
* Test Triple Compare and Swap (TCS: 30) instruction : All EQ
* ---------------------------------------------------------------------
         TSTHDR 0,99,30            All Operand Equal (cc=0, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Triple Compare and Swap (TCS: 30)
* Build Parambter List
*
         MVC   0(12,R1),0(R2)      load Op1-Op3 data in source
         MVC   64(4,R3),0(R1)      copy to PL OP1-Compare
         MVC   72(4,R3),4(R1)      copy to PL OP2-Compare
         MVC   80(4,R3),8(R1)      copy to PL OP3-Compare

         MVC   0(12,R4),128(R2)    load initial Op1-Op3 data
         MVC   88(4,R3),0(R4)      copy to PL OP1-Replacement
         MVC   96(4,R3),4(R4)      copy to PL OP2-Replacement
         MVC   104(4,R3),8(R4)     copy to PL OP3-Replacement

****
* Do NOT force a failure by changing the 3rd operand
****
*         MVC   8(4,R1),0(R4)
****

         STG   R1,0(R3)            load OP1 address in PL

         LA    R1,4(R1)
         STG   R1,8(R3)            load OP2 address in PL

         LA    R1,4(R1)
         STG   R1,16(R3)           load OP3 address in PL

         XGR   R0,R0
         LA    R0,30
         PLO   0,0,0,0(R3)         Triple C/S (TCS: 30)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F304TSTCC            fail if...
*
* compare results
*
F304CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA

* All equal; replacement values copied to operand source

         CLC   0(4,R1),88(R3)
         BNE   F304XX              OP1R Different, error
         CLC   4(4,R1),96(R3)
         BNE   F304XX              OP2R Different, error
         CLC   8(4,R1),104(R3)
         BE    0(R11)              OP4R same, return

F304XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F304TSTCC BC   0,F304CCMSG         fail if unexpected condition code
F304CCMSG EQU  *
         BAL   R15,CCMSG
         B     F304CKRES

         TSTDATA
         TSTLBL OPDST              Dest/Data: Op1-R, Op2-R, Op3-R
         DC    XL256'0'

         TSTLBL OPSRC              Source: OP1-OP3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    CL96'Triple Compare and Swap (TCS: 30): All EQ'
         TSTEND
                                                                EJECT
*======================================================================
*       Compare and Swap and Triple Store Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Compare and Swap and Triple Store (CSTSTO: 29) instruction : NE
* ---------------------------------------------------------------------
         TSTHDR 1,99,29            1st Operand NOT Equal (cc=1, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA
*
* Compare and Swap and Triple Store (CSTSTO: 29)
* Build Parambter List
*
         MVC   0(160,R1),0(R2)     load Op1-C, Op3, Op5, Op7, Op1-R
         MVC   0(32,R3),0(R1)      copy to PL OP1-C
         MVC   144(32,R3),32(R1)   copy to PL OP3
         MVC   176(32,R3),64(R1)   copy to PL OP5
         MVC   208(32,R3),96(R1)   copy to PL OP7
         MVC   32(32,R3),128(R1)   copy to PL OP1-R

         MVC   0(128,R4),128(R2)   load initial Op2, Op4, Op6, Op8

****
* force a failure by changing the Op1-c
****
         MVC   0(4,R3),33(R2)
****
         XGR   R0,R0
         LA    R0,32(R4)
         STG   R0,72(R3)           load OP4 address in PL

         LA    R0,64(R4)
         STG   R0,104(R3)          load OP6 address in PL

         LA    R0,96(R4)
         STG   R0,136(R3)          load OP8 address in PL

         XGR   R0,R0
         LA    R0,29
         PLO   0,0(R4),0,0(R3)     C/S Triple Store (CSTSTO: 29)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F291TSTCC            fail if...
*
* compare results
*
F291CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA           Op1, Op3, Op5, Op7
         LGF   R2,RANDXA
         LGF   R3,PLA              PL
         LGF   R4,OPREPA           OP2, Op4, Op6, Op8

         CLC   0(32,R4),0(R3)      Op2 == OP1C?
         BE    0(R11)              same, return

F291XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F291TSTCC BC   0,F291CCMSG         fail if unexpected condition code
F291CCMSG EQU  *
         BAL   R15,CCMSG
         B     F291CKRES

         TSTDATA
         TSTLBL OPDST              OP2, Op4, Op6, Op8
         DC    XL256'0'

         TSTLBL OPSRC              Op1-C, Op3, Op5, Op7, Op1-R
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    C'Compare and Swap and Triple Store (CSTSTO: 29) : NE'
         DC    CL64' '
         TSTEND

* ---------------------------------------------------------------------
* Test Compare and Swap and Triple Store (CSTSTO: 29) instruction : EQ
* ---------------------------------------------------------------------
         TSTHDR 0,99,29            1st Operand NOT Equal (cc=1, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA           Op1-C, Op3, Op5, Op7, Op1-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4, Op6, Op8
*
* Compare and Swap and Triple Store (CSTSTO: 29)
* Build Parambter List
*
         MVC   0(160,R1),0(R2)     load Op1-C, Op3, Op5, Op7, Op1-R
         MVC   0(32,R3),0(R1)      copy to PL OP1-C
         MVC   144(32,R3),32(R1)   copy to PL OP3
         MVC   176(32,R3),64(R1)   copy to PL OP5
         MVC   208(32,R3),96(R1)   copy to PL OP7
         MVC   32(32,R3),128(R1)   copy to PL OP1-R

         MVC   0(128,R4),128(R2)   load initial Op2, Op4, Op6, Op8

****
* Force a EQ by changing the Op1-C == Op2
****
         MVC   0(32,R3),0(R4)
****
         XGR   R0,R0
         LA    R0,32(R4)
         STG   R0,72(R3)           load OP4 address in PL

         LA    R0,64(R4)
         STG   R0,104(R3)          load OP6 address in PL

         LA    R0,96(R4)
         STG   R0,136(R3)          load OP8 address in PL

         XGR   R0,R0
         LA    R0,29
         PLO   0,0(R4),0,0(R3)     C/S Triple Store (CSTSTO: 29)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F292TSTCC            fail if...
*
* compare results
*
F292CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA           Op1, Op3, Op5, Op7
         LGF   R2,RANDXA
         LGF   R3,PLA              PL
         LGF   R4,OPREPA           OP2, Op4, Op6, Op8

         CLC   32(32,R4),144(R3)   OP4 ==  PL Op3
         BNE   F292XX                   different, error
         CLC   64(32,R4),176(R3)   OP6 ==  PL Op5
         BNE   F292XX                   different, error
         CLC   96(32,R4),208(R3)   OP8 ==  PL Op7
         BNE   F292XX                   different, error
         CLC   0(32,R4),32(R3)     OP2 ==  PL Op-R
         BE    0(R11)                   same, return

F292XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F292TSTCC BC   0,F292CCMSG         fail if unexpected condition code
F292CCMSG EQU  *
         BAL   R15,CCMSG
         B     F292CKRES

         TSTDATA
         TSTLBL OPDST              OP2, Op4, Op6, Op8
         DC    XL256'0'

         TSTLBL OPSRC              Op1-C, Op3, Op5, Op7, Op1-R
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    C'Compare and Swap and Triple Store (CSTSTO: 29) : EQ'
         DC    CL64' '
         TSTEND

                                                                EJECT
*======================================================================
*       Compare and Swap and Double Store Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Compare and Swap and Double Store (CSDSTO: 28) instruction : NE
* ---------------------------------------------------------------------
         TSTHDR 1,99,28            1Op1-C != OP2 (cc=1, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA           Op1-C, Op3, Op5, Op1-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4, Op6
*
* Compare and Swap and Double Store (CSDSTO: 28)
* Build Parambter List
*
         MVC   0(128,R1),0(R2)     load Op1-C, Op3, Op5, Op1-R
         MVC   0(32,R3),0(R1)      copy to PL OP1-C
         MVC   112(32,R3),32(R1)   copy to PL OP3
         MVC   144(32,R3),64(R1)   copy to PL OP5
         MVC   32(32,R3),96(R1)    copy to PL OP1-R

         MVC   0(96,R4),128(R2)    load initial Op2, Op4, Op6

****
* force a failure by changing the Op1-c
****
         MVC   0(4,R3),33(R2)
****
         XGR   R0,R0
         LA    R0,32(R4)
         STG   R0,72(R3)           load OP4 address in PL

         LA    R0,64(R4)
         STG   R0,104(R3)          load OP6 address in PL

         XGR   R0,R0
         LA    R0,28
         PLO   0,0(R4),0,0(R3)     C/S Double Store (CSDSTO: 28)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F281TSTCC            fail if...
*
* compare results
*
F281CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA           Op1, Op3, Op5
         LGF   R2,RANDXA
         LGF   R3,PLA              PL
         LGF   R4,OPREPA           OP2, Op4, Op6

         CLC   0(32,R4),0(R3)      Op2 == OP1C?
         BE    0(R11)              same, return

F281XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F281TSTCC BC   0,F281CCMSG         fail if unexpected condition code
F281CCMSG EQU  *
         BAL   R15,CCMSG
         B     F281CKRES

         TSTDATA
         TSTLBL OPDST              OP2, Op4, Op6
         DC    XL256'0'

         TSTLBL OPSRC              Op1-C, Op3, Op5, Op1-R
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    C'Compare and Swap and Double Store (CSDSTO: 28) : NE'
         DC    CL64' '
         TSTEND

* ---------------------------------------------------------------------
* Test Compare and Swap and Double Store (CSDSTO: 28) instruction : EQ
* ---------------------------------------------------------------------
         TSTHDR 0,99,28            Op1-C == OP2 (cc=0, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA           Op1-C, Op3, Op5, Op1-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4, Op6
*
* Compare and Swap and Double Store (CSDSTO: 28)
* Build Parambter List
*
         MVC   0(128,R1),0(R2)     load Op1-C, Op3, Op5, Op1-R
         MVC   0(32,R3),0(R1)      copy to PL OP1-C
         MVC   112(32,R3),32(R1)   copy to PL OP3
         MVC   144(32,R3),64(R1)   copy to PL OP5
         MVC   32(32,R3),96(R1)    copy to PL OP1-R

         MVC   0(96,R4),128(R2)    load initial Op2, Op4, Op6

****
* Force a EQ by changing the Op1-C == Op2
****
         MVC   0(32,R3),0(R4)
****
         XGR   R0,R0
         LA    R0,32(R4)
         STG   R0,72(R3)           load OP4 address in PL

         LA    R0,64(R4)
         STG   R0,104(R3)          load OP6 address in PL

         XGR   R0,R0
         LA    R0,28
         PLO   0,0(R4),0,0(R3)     C/S Double Store (CSDSTO: 28)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F282TSTCC            fail if...
*
* compare results
*
F282CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA           Op1, Op3, Op5
         LGF   R2,RANDXA
         LGF   R3,PLA              PL
         LGF   R4,OPREPA           OP2, Op4, Op6

         CLC   32(32,R4),112(R3)   OP4 ==  PL Op3
         BNE   F282XX                   different, error
         CLC   64(32,R4),144(R3)   OP6 ==  PL Op5
         BNE   F282XX                   different, error
         CLC   0(32,R4),32(R3)     OP2 ==  PL Op-R
         BE    0(R11)                   same, return

F282XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F282TSTCC BC   0,F282CCMSG         fail if unexpected condition code
F282CCMSG EQU  *
         BAL   R15,CCMSG
         B     F282CKRES

         TSTDATA
         TSTLBL OPDST              OP2, Op4, Op6
         DC    XL256'0'

         TSTLBL OPSRC              Op1-C, Op3, Op5, Op1-R
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    C'Compare and Swap and Double Store (CSDSTO: 28) : EQ'
         DC    CL64' '
         TSTEND

                                                                EJECT
*======================================================================
*       Compare and Swap and Store Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Compare and Swap and Store (CSSTO: 27) instruction : NE
* ---------------------------------------------------------------------
         TSTHDR 1,99,26            Op1-C != OP2 (cc=1, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA           Op1-C, Op3, Op1-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4
*
* Compare and Swap and Store (CSSTO: 27)
* Build Parambter List
*
         MVC   0(96,R1),0(R2)      load Op1-C, Op3, Op1-R
         MVC   0(32,R3),0(R1)      copy to PL OP1-C
         MVC   80(32,R3),32(R1)    copy to PL OP3
         MVC   32(32,R3),96(R1)    copy to PL OP1-R

         MVC   0(64,R4),128(R2)    load initial Op2, Op4

****
* force a failure by changing the Op1-c
****
         MVC   0(4,R3),33(R2)
****
         XGR   R0,R0
         LA    R0,32(R4)
         STG   R0,72(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,27
         PLO   0,0(R4),0,0(R3)     C/S Store (CSSTO: 27)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F271TSTCC            fail if...
*
* compare results
*
F271CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA           Op1-C, Op3, Op1-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4

         CLC   0(32,R4),0(R3)      Op2 == OP1C?
         BE    0(R11)              same, return

F271XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F271TSTCC BC   0,F271CCMSG         fail if unexpected condition code
F271CCMSG EQU  *
         BAL   R15,CCMSG
         B     F271CKRES

         TSTDATA
         TSTLBL OPDST              OP2, Op4
         DC    XL256'0'

         TSTLBL OPSRC              Op1-C, Op3, Op1-R
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    C'Compare and Swap and Store (CSSTO: 27) : NE'
         DC    CL64' '
         TSTEND

* ---------------------------------------------------------------------
* Test Compare and Swap and Store (CSSTO: 27) instruction : EQ
* ---------------------------------------------------------------------
         TSTHDR 0,99,27            Op1-C == OP2 (cc=0, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA           Op1-C, Op3, Op1-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4
*
* Compare and Swap and Store (CSSTO: 27)
* Build Parambter List
*
         MVC   0(96,R1),0(R2)      load Op1-C, Op3, Op1-R
         MVC   0(32,R3),0(R1)      copy to PL OP1-C
         MVC   80(32,R3),32(R1)    copy to PL OP3
         MVC   32(32,R3),96(R1)    copy to PL OP1-R

         MVC   0(64,R4),128(R2)    load initial Op2, Op4

****
* Force a EQ by changing the Op1-C == Op2
****
         MVC   0(32,R3),0(R4)
****
         XGR   R0,R0
         LA    R0,32(R4)
         STG   R0,72(R3)           load OP4 address in PL

         LA    R0,64(R4)
         STG   R0,104(R3)          load OP6 address in PL

         XGR   R0,R0
         LA    R0,27
         PLO   0,0(R4),0,0(R3)     C/S Store (CSSTO: 27)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F272TSTCC            fail if...
*
* compare results
*
F272CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA           Op1-C, Op3, Op1-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4

         CLC   32(32,R4),80(R3)    OP4 ==  PL Op3
         BNE   F272XX                   different, error
         CLC   0(32,R4),32(R3)     OP2 ==  PL Op-R
         BE    0(R11)                   same, return

F272XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F272TSTCC BC   0,F272CCMSG         fail if unexpected condition code
F272CCMSG EQU  *
         BAL   R15,CCMSG
         B     F272CKRES

         TSTDATA
         TSTLBL OPDST              OP2, Op4
         DC    XL256'0'

         TSTLBL OPSRC              Op1-C, Op3, Op1-R
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    C'Compare and Swap and Store (CSSTO: 27) : EQ'
         DC    CL64' '
         TSTEND

                                                                EJECT
*======================================================================
*       Double Compare and Swap Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Double Compare and Swap (DCSO: 26) instruction : NE Op1!=Op2
* ---------------------------------------------------------------------
         TSTHDR 1,99,26            Op1-C != OP2 (cc=1, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA           Op1-C, Op3, Op1-R, Op3-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4
*
* Double Compare and Swap (DCSO: 26)
* Build Parambter List
*
         MVC   0(128,R1),0(R2)     load Op1-C, Op3, Op1-R, Op3-R
         MVC   0(32,R3),0(R1)      copy to PL OP1-C
         MVC   80(32,R3),32(R1)    copy to PL OP3
         MVC   32(32,R3),64(R1)    copy to PL OP1-R
         MVC   112(32,R3),96(R1)   copy to PL OP3-R

         MVC   0(64,R4),128(R2)    load initial Op2, Op4

****
* force a failure by changing the Op1-c
****
         MVC   0(4,R3),33(R2)
****
         XGR   R0,R0
         LA    R0,32(R4)
         STG   R0,72(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,26
         PLO   0,0(R4),0,0(R3)     Double C/S (DCSO: 26)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F261TSTCC            fail if...
*
* compare results
*
F261CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA           Op1-C, Op3, Op1-R, Op3-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4

         CLC   0(32,R4),0(R3)      Op2 == OP1C?
         BE    0(R11)              same, return

F261XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F261TSTCC BC   0,F261CCMSG         fail if unexpected condition code
F261CCMSG EQU  *
         BAL   R15,CCMSG
         B     F261CKRES

         TSTDATA
         TSTLBL OPDST              OP2, Op4
         DC    XL256'0'

         TSTLBL OPSRC              Op1-C, Op3, Op1-R, Op3-R
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    C'Double Compare and Swap (DCSO: 26) : NE Op1 != Op2'
         DC    CL64' '
         TSTEND

* ---------------------------------------------------------------------
* Test Double Compare and Swap (DCSO: 26) instruction : NE Op3!=Op4
* ---------------------------------------------------------------------
         TSTHDR 1,99,26            Op3 != OP4 (cc=2, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA           Op1-C, Op3, Op1-R, Op3-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4
*
* Double Compare and Swap (DCSO: 26)
* Build Parambter List
*
         MVC   0(128,R1),0(R2)     load Op1-C, Op3, Op1-R, Op3-R
         MVC   0(32,R3),0(R1)      copy to PL OP1-C
         MVC   80(32,R3),32(R1)    copy to PL OP3
         MVC   32(32,R3),64(R1)    copy to PL OP1-R
         MVC   112(32,R3),96(R1)   copy to PL OP3-R

         MVC   0(64,R4),128(R2)    load initial Op2, Op4

****
* force a failure by changing the Op3-C
****
         MVC   80(4,R3),33(R2)
****
         XGR   R0,R0
         LA    R0,32(R4)
         STG   R0,72(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,26
         PLO   0,0(R4),0,0(R3)     Double C/S (DCSO: 26)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F262TSTCC            fail if...
*
* compare results
*
F262CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA           Op1-C, Op3, Op1-R, Op3-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4

         CLC   0(32,R4),0(R3)      Op2 == OP1C?
         BNE   F262XX
         CLC   80(32,R3),32(R4)    Op3-C == OP4
         BNE    0(R11)                NE, matches test Op3 != OP4

F262XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F262TSTCC BC   0,F262CCMSG         fail if unexpected condition code
F262CCMSG EQU  *
         BAL   R15,CCMSG
         B     F262CKRES

         TSTDATA
         TSTLBL OPDST              OP2, Op4
         DC    XL256'0'

         TSTLBL OPSRC              Op1-C, Op3, Op1-R, Op3-R
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    C'Double Compare and Swap (DCSO: 26) : Op3 != OP4'
         DC    CL64' '
         TSTEND

* ---------------------------------------------------------------------
* Test Double Compare and Swap (DCSO: 26) instruction : EQ
* ---------------------------------------------------------------------
         TSTHDR 0,99,28            Op1-C == OP2 && Op3 == OP4
*                                       (cc=0, UR=99)
         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA           Op1-C, Op3, Op1-R, Op3-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4
*
* Compare and Swap and Double Store (CSDSTO: 28)
* Build Parambter List
*
         MVC   0(128,R1),0(R2)     load Op1-C, Op3, Op1-R, Op3-R
         MVC   0(32,R3),0(R1)      copy to PL OP1-C
         MVC   80(32,R3),32(R1)    copy to PL OP3
         MVC   32(32,R3),64(R1)    copy to PL OP1-R
         MVC   112(32,R3),96(R1)   copy to PL OP3-R

         MVC   0(64,R4),128(R2)    load initial Op2, Op4

****
* Force a EQ by changing Op1-C == OP2 && Op3 == OP4
****
         MVC   0(32,R3),0(R4)      Op1-C <-- Op2
         MVC   80(32,R3),32(R4)    Op3-C <-- Op4
****
         XGR   R0,R0
         LA    R0,32(R4)
         STG   R0,72(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,26
         PLO   0,0(R4),0,0(R3)     C/S Double Store (CSDSTO: 28)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F263TSTCC            fail if...
*
* compare results
*
F263CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA           Op1-C, Op3, Op1-R, Op3-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4

         CLC   32(32,R4),112(R3)   OP4 ==  PL Op3-R
         BNE   F263XX                   different, error
         CLC   0(32,R4),32(R3)     OP2 ==  PL Op1-R
         BE    0(R11)                   same, return

F263XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F263TSTCC BC   0,F263CCMSG         fail if unexpected condition code
F263CCMSG EQU  *
         BAL   R15,CCMSG
         B     F263CKRES

         TSTDATA
         TSTLBL OPDST              OP2, Op4
         DC    XL256'0'

         TSTLBL OPSRC              Op1-C, Op3, Op1-R, Op3-R
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    C'Double Compare and Swap (DCSO: 26) : EQ'
         DC    CL64' '
         TSTEND


                                                                EJECT
*======================================================================
*       Compare and Swap Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Compare and Swap (CSO: 25) instruction : NE Op1-C!=Op2
* ---------------------------------------------------------------------
         TSTHDR 1,99,26            Op1-C != OP2 (cc=1, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA           Op1-C, Op1-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2
*
* Compare and Swap (CSO: 25)
* Build Parambter List
*
         MVC   0(64,R1),0(R2)      load Op1-C, Op1-R
         MVC   0(32,R3),0(R1)      copy to PL OP1-C
         MVC   32(32,R3),32(R1)    copy to PL OP1-R

         MVC   0(32,R4),128(R2)    load initial Op2

****
* force a NE by changing the Op1-c
****
         MVC   0(4,R3),33(R2)
****
         XGR   R0,R0
         LA    R0,25
         PLO   0,0(R4),0,0(R3)     C/S (CSO: 25)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F251TSTCC            fail if...
*
* compare results
*
F251CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA           Op1-C, Op1-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2

         CLC   0(32,R4),0(R3)      Op2 == OP1-C?
         BE    0(R11)              same, return

F251XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F251TSTCC BC   0,F251CCMSG         fail if unexpected condition code
F251CCMSG EQU  *
         BAL   R15,CCMSG
         B     F251CKRES

         TSTDATA
         TSTLBL OPDST              OP2
         DC    XL256'0'

         TSTLBL OPSRC              Op1-C, Op1-R
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    C'Compare and Swap (CSO: 25) : NE Op1-C != Op2'
         DC    CL64' '
         TSTEND

* ---------------------------------------------------------------------
* Test Compare and Swap (DCSO: 25) instruction : EQ Op1-C == OP2
* ---------------------------------------------------------------------
         TSTHDR 0,99,25            EQ Op1-C == OP2 (cc=0, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA           Op1-C, Op1-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2
*
* Compare and Swap (DCSO: 25)
* Build Parambter List
*
         MVC   0(64,R1),0(R2)      load Op1-C, Op1-R
         MVC   0(32,R3),0(R1)      copy to PL OP1-C
         MVC   32(32,R3),32(R1)    copy to PL OP1-R

         MVC   0(32,R4),128(R2)    load initial Op2

****
* force an EQ by changing the Op1-C
****
         MVC   0(32,R3),0(R4)      Op1-C <-- Op2
****
         XGR   R0,R0
         LA    R0,25
         PLO   0,0(R4),0,0(R3)     C/S (CSO: 25)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F252TSTCC            fail if...
*
* compare results
*
F252CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA           Op1-C, Op1-R
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2

         CLC   32(32,R3),0(R4)     Op1-R == OP2
         BE    0(R11)              same, return

F252XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F252TSTCC BC   0,F252CCMSG         fail if unexpected condition code
F252CCMSG EQU  *
         BAL   R15,CCMSG
         B     F252CKRES

         TSTDATA
         TSTLBL OPDST              OP2
         DC    XL256'0'

         TSTLBL OPSRC              Op1-C, Op1-R
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    C'Compare and Swap (CSO: 25) : EQ Op1-C == OP2'
         DC    CL64' '
         TSTEND
                                                                EJECT
*======================================================================
*       Compare and Load Tests
*======================================================================

* ---------------------------------------------------------------------
* Test Compare and Load (CLO: 24) instruction : NE Op1-C!=Op2
* ---------------------------------------------------------------------
         TSTHDR 1,99,25            Op1-C != OP2 (cc=1, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA           Op1-C, Op3
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4
*
* Compare and Load (CLO: 24)
* Build Parambter List
*
         MVC   0(64,R1),0(R2)      load Op1-C, Op3
         MVC   0(32,R3),0(R1)      copy to PL Op1-C
         MVC   32(32,R3),32(R1)    copy to PL Op3

         MVC   0(64,R4),128(R2)    load initial Op2, Op4

****
* force a NE by changing the Op1-c
****
         MVC   0(4,R3),33(R2)
****
         XGR   R0,R0
         LA    R0,32(R4)
         STG   R0,72(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,24
         PLO   0,0(R4),0,0(R3)     C/L (CLO: 24)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F241TSTCC            fail if...
*
* compare results
*
F241CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA           Op1-C, Op3
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4

         CLC   0(32,R4),0(R3)      Op2 == OP1-C?
         BE    0(R11)              same, return

F241XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F241TSTCC BC   0,F241CCMSG         fail if unexpected condition code
F241CCMSG EQU  *
         BAL   R15,CCMSG
         B     F241CKRES

         TSTDATA
         TSTLBL OPDST              OP2, Op4
         DC    XL256'0'

         TSTLBL OPSRC              Op1-C, Op3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    C'Compare and Load (CLO: 24) : NE Op1-C != Op2'
         DC    CL64' '
         TSTEND

* ---------------------------------------------------------------------
* Test Compare and Load (CLO: 24) instruction : EQ Op1-C == OP2
* ---------------------------------------------------------------------
         TSTHDR 0,99,25            EQ Op1-C == OP2 (cc=0, UR=99)

         TSTCODE
*                                  load random source
         LGF   R1,OPSRCA           Op1-C, Op3
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4
*
* Compare and Swap (DCSO: 25)
* Build Parambter List
*
         MVC   0(64,R1),0(R2)      load Op1-C, Op3
         MVC   0(32,R3),0(R1)      copy to PL Op1-C
         MVC   32(32,R3),32(R1)    copy to PL Op3

         MVC   0(64,R4),128(R2)    load initial Op2, Op4

****
* force an EQ by changing the Op1-C
****
         MVC   0(32,R3),0(R4)      Op1-C <-- Op2
****
         XGR   R0,R0
         LA    R0,32(R4)
         STG   R0,72(R3)           load OP4 address in PL

         XGR   R0,R0
         LA    R0,24
         PLO   0,0(R4),0,0(R3)     C/S (CLO: 24)

         EPSW  R2,R0               extract psw
         ST    R2,CCPSW              to save CC
         LB    R1,CCMASK           (failure CC mask)
         SLL   R1,4                (shift to BC instr CC position)
         EX    R1,F242TSTCC            fail if...
*
* compare results
*
F242CKRES EQU   *
*                                  reset adresses
         LGF   R1,OPSRCA           Op1-C, Op3
         LGF   R2,RANDXA
         LGF   R3,PLA
         LGF   R4,OPREPA           OP2, Op4

         CLC   32(32,R3),32(R4)    Op3 == OP4
         BE    0(R11)              same, return

F242XX   EQU   *
         BAL   R15,FAILMSG
         BR    R11

F242TSTCC BC   0,F242CCMSG         fail if unexpected condition code
F242CCMSG EQU  *
         BAL   R15,CCMSG
         B     F242CKRES

         TSTDATA
         TSTLBL OPDST              OP2, Op4
         DC    XL256'0'

         TSTLBL OPSRC              Op1-C, Op3
         DC    XL256'0'

         TSTLBL PL                 Parameter List
         DC    XL352'00'

         TSTLBL FMSG               fail message
         DC    C'failed PLO-Extension '
         DC    C'Compare and Load (CLO: 24) : EQ Op1-C == OP2'
         DC    CL64' '
         TSTEND

***********************************************************************
* table of pointers to individual tests
***********************************************************************

         ORG   PLOTESTS            (2k reserved for test pointers)
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
