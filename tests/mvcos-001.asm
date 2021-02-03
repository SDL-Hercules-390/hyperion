 TITLE '                   mvcos-001.asm        Test MVCOS Instruction'
*Testcase mvsos001:   MVCOS
* Created and placed into the public domain
* 27 JAN 2021 by Bob Polmanter.
                                                                SPACE
R0       EQU   0          General Purpose Registers
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
AR0      EQU   0          Access Registers
AR1      EQU   1
AR2      EQU   2
AR3      EQU   3
AR4      EQU   4
AR5      EQU   5
AR6      EQU   6
AR7      EQU   7
AR8      EQU   8
AR9      EQU   9
AR10     EQU   10
AR11     EQU   11
AR12     EQU   12
AR13     EQU   13
AR14     EQU   14
AR15     EQU   15
CR0      EQU   0          Control Registers
CR1      EQU   1
CR2      EQU   2
CR3      EQU   3
CR4      EQU   4
CR5      EQU   5
CR6      EQU   6
CR7      EQU   7
CR8      EQU   8
CR9      EQU   9
CR10     EQU   10
CR11     EQU   11
CR12     EQU   12
CR13     EQU   13
CR14     EQU   14
CR15     EQU   15
                                                                EJECT
***********************************************************************
*
*    These tests and this programming were validated on a z114
*    using a z/VM 6.4 virtual machine on 27 January 2021.
*
***********************************************************************
*
* Tests performed with MVCOS:
*
* 1.  Execute the MVCOS instruction iteratively, each time trying a
*     different combination of machine state, address space control
*     mode, MVCOS operand 1 control modes, MVCOS operand 2 control
*     modes, and key enablement in both operand 1 and then operand 2.
*     These individual tests are nested in a series of loops, so that
*     each state or mode is tested with each other combination of
*     states or modes in an exhaustive way.  A visual description of
*     the nested loops and their related tests is shown below.
*
*     After execution of each MVCOS instruction, the results of the
*     actual data moved are checked to determine if the data fetched
*     indeed came from the specified address space, and if the data
*     stored was indeed placed into the specified address space, as
*     determined by the settings in R0 for operand 2 and operand 1,
*     respectively.
*
* Upon success this is the number of tests performed:
*
*   Successfully completed MVCOS executions:             384
*   Expected protection check events:                  1,152
*   Expected special operation exception events:          92
*   TOTAL TESTS:                                       1,328
*
* MVCOS Results Failure:      Disabled Wait PSW X'BAD1'
* Overall Test Failure:       Disabled Wait PSW X'BAD9'
* Overall Test Success:       Disabled Wait PSW X'0000'
* Unexpected Program Check:   Disabled Wait PSW X'DEAD'
*
* The expected protection checks arise from enabling key controlled
* protection in register 0 as specified by the instruction. Keys
* have been deliberately set to allow some accesses and to fail some
* access attempts. Because either MVCOS operands (or both) could be
* using a failing key, there is more of these failures, whereas
* success requires both operands to have the right key setting.
*
* The expected special operation exceptions arise when the Home
* address space control mode is selected in Register 0 -AND- the
* PSW is in the problem state. This applies to operand 1 OAC only.
* This tests that MVCOS is honoring that specification as documented
* in the Principles.
*
***********************************************************************
                                                                EJECT
***********************************************************************
*
*                           TEST METHOD
*
* 1. Setup.
*
* Three address spaces are created: Primary, Secondary, and Home.
* Literals are placed into a page frame belonging to each one of the
* address spaces identifying them by PRI, SEC, and HOM. Those pages
* are the targets of MVCOS operand 1.  A second set of
* literals is placed into another page frame belonging to each
* one of the address spaces identifying them also as FROMPRI, FROMSEC,
* and FROMHOM. These pages are fetched by MVCOS operand 2.
*
* The literals in the target page frames of each address space are
* located at virtual 00010FF0 in each space.
*
* The literals in the 'from' page frames of each address space are
* located at virtual 00012FF8 in each space.
*
* The locations are set to cause MVCOS to move data across page
* boundaries.
*
* The nested loops are entered to set the register 0 MVCOS controls.
*
*
* 2. Executing MVCOS.
*
* A FROM literal is moved to the target page by MVCOS. The address
* space fetched from and the address space target are of course
* determined by the MVCOS controls in Register 0.
*
*
* 3. Validation.
*
* After the MVCOS, the register 0 controls are extracted and used
* to determine programatically which address space literal was
* requested to be moved to which target, and the results are compared
* to determine if those literals are actually where they are supposed
* to be.
*
* After successful validation, the original placement of the literals
* is restored, and the next loop iteration advances to the next test.
*
***********************************************************************
                                                                EJECT
***********************************************************************
*
*                    DEBUGGING THIS TEST PROGRAM
*
* If any MVCOS test fails (data from the specified address space is
* not identified as expected), the machine will be halted to preserve
* results and a disabled wait PSW of X'BAD1' will be loaded. Use
* register 0 and the address space control value in byte PSWASC to
* determine what should have been moved to where.  View the literals
* at virtual location X'00010FF0' to determine which space you are
* viewing and what FROM literal was actually  moved to that space.
* Compare that to the R0 controls and PSWASC mode to validate whether
* MVCOS moved correctly.  You can also use the memory map listed
* below to view the real pages by address to inspect the literals in
* each address space.
*
* If an unexpected program check occurs, the PSW will be loaded with
* a disabled wait code X'DEAD'. The machine is halted immediately
* upon occurance and no registers are altered; hence their values
* reflect the state of the failure.
*
*
***********************************************************************
                                                                EJECT
***********************************************************************
*
*                    Memory Map - REAL STORAGE
*
*         Hex
* RAddr   Len     Description
* -----   ----    -----------------------------------------------------
*     0   2000  - Absolute page 0 and 1
*  2000   2000  - Program code
*  4000   1000  - Segment table, primary space
*  5000   1000  - Segment table, secondary space
*  6000   1000  - Segment table, home space
*  7000    800  - Page tables, primary space
*  7800    800  - Page tables, secondary space
*  8000    800  - Page tables, home space
*  9000   1000  - Primary ASTE, DUCT, DU-AL, ALE blocks
*  A000   1000  - Home ASTE block
*  B000   5000  - unused; available
* 10000  10000  - Pages backing Home virtual space at vaddr 10000
* 20000  10000  - Pages backing Primary virtual space at vaddr 10000
* 30000  10000  - Pages backing Secondary virtual space at vaddr 10000
*
*
*                    Memory Map - VIRTUAL STORAGE
*
* VAddr   Len   Raddr Key   Description
* -----   ----   ---- ---   ---------------------------------------
* 00000  10000  00000  00 - Common V=R storage (all address spaces)
* 10000  10000  10000  00 - Home space storage
* 10000  10000  20000  40 - Primary space storage
* 10000  10000  30000  80 - Secondary space storage
*
*
***********************************************************************
*                    IN EACH ADDRESS SPACE:
*
* VADDR 10FF0 length 32:  Literal identifying the space target
*                          (e.g., CL16'PRI-PG1',CL16'PRI-PG2'
*
* VADDR 12FF8 length 16:  Literal identifying the space source
*                          (e.g., CL16'FROMPRI1FROMPRI2'
*
* After a successful MVCOS, the storage at location 10FF0 would look
* like this example with a move from secondary to primary:
*
* VADDR 10FF0   CL32'PRI-PG1FROMSEC1FROMSEC2        '
*
* thus showing that the target area is still named PRI,and the data
* came from two pages in the secondary space.
*
*
***********************************************************************
                                                                EJECT
***********************************************************************
*
*             VISUAL DESCRIPTION OF NESTED LOOP TESTS
*
* The sequence of loops nested below allows each combination to be
* tested one at a time.  OAC1 and OAC2 are the Operand Access
* Control 1 and 2, respectively, in Register 0 that control MVCOS.
*
* Loop  iterations  Description
*   1        2      Supervisor state, then problem state
*   2        4      Cycle through each PSW ASC mode P,AR,S,H
*   3        2      OAC1 A validity bit off, then on
*   4        4      When OAC1 A=1, cycle through each ASC mode in OAC1
*   5        3      When OAC1 A=1 & OAC1 is AR, cycle ALETs 0,1,2 oper1
*   6        2      OAC2 A validity bit off, then on
*   7        4      When OAC2 A=1, cycle through each ASC mode in OAC2
*   8        3      When OAC2 A=1 & OAC2 is AR, cycle ALETs 0,1,2 oper2
*   9        2      OAC1 K validity bit off, then on
*  10        2      OAC2 K validity bit off, then on
*
*                   Execute MVCOS
*
*                   Check results; PSW=X'BAD1' if failed
*                   Next loop 10
*                   Next loop 9
*                   Next loop 8
*                   Next loop 7
*                   Next loop 6
*                   Next loop 5
*                   Next loop 4
*                   Next loop 3
*                   Next loop 2
*                   Next loop 1
*                   Terminate with success, PSW=X'000'
*
* Note on loop 4 & 7: when either OAC A validity bit is 0, then the
* PSW ASC mode is used by MVCOS and the iteration count is 1.
*
* Note on loop 5 & 8: when either OAC A validity bit is 0 -AND- the
* PSW ASC mode is P,S,or H, then the PSW ASC mode is used by MVCOS and
* the iteration count is 1.  However, if the PSW ASC mode is AR, then
* the iteration count remains 3 for these loops so the ALETs can be
* cycled through each test for each operand 1 or operand 2, in turn.
*
***********************************************************************
                                                                EJECT
***********************************************************************
*                     Low Core / Prefix Area
***********************************************************************
*
MVCOS001 START 0
STRTLABL EQU   *
         USING STRTLABL,0
                                                                SPACE 3
*             Selected z/Arch low core layout
*
         ORG   STRTLABL+X'88'      interrupt code area EC mode
SVCINTC  DC    X'00000000'         SVC interrupt code area
PGMINTC  DC    X'00000000'         Prog check interrupt code area
*
         ORG   STRTLABL+X'140'
SVCOPSW  DS    XL16                SVC old PSW
PGMOPSW  DS    XL16                Program check old PSW
*
         ORG   STRTLABL+X'1A0'     New PSWs
RESTART  DC    X'00000000',X'80000000',A(0),A(START)    DAT OFF
EXTNPSW  DC    XL16'00'
SVCNPSW  DC    X'04004000',X'80000000',A(0),A(SVCFLIH)  DAT ON, AR MODE
PGMNPSW  DC    X'04004000',X'80000000',A(0),A(PGMFLIH)  DAT ON, AR MODE
                                                                SPACE 3
*                     Test Counters
*
         ORG   STRTLABL+X'200'  Test counters
MVCOSOK  DC    PL4'0'           # of successful MVCOS   =     384
PIC04    DC    PL4'0'           # of Pchecks 04         =   1,152
PIC13    DC    PL4'0'           # of Pchecks 13         =      92
         EJECT
***********************************************************************
*                       Main program
***********************************************************************
*
         ORG   STRTLABL+X'2000'
START    BASR  R15,0
         BCTR  R15,0
         BCTR  R15,0
         USING START,R15
*
         SR    R2,R2                   STATUS REG SET TO 0
         LA    R3,1                    R3=1 MEANS SET Z/ARCH MODE
*                                      R3=0 MEANS SET ESA/390 MODE
         SR    R4,R4                   CPU Addr = 0
         SIGP  R2,R4,X'12'             X'12' = SET ARCHITECTURE
*
         LA    R0,X'08'                Set KEY=0 fetch prot enabled
         LA    R2,64                   # of real pages to set
         SR    R1,R1                   Starting addr
*
SET000   SSKE  R0,R1                   Set the key
         AHI   R1,4096                 Bump to next page
         BCT   R2,SET000
*
         LAM   AR0,AR15,AREGS          Clear all ARs
         LCTLG CR0,CR15,CREGS          Load all the CRs
         SSM   =X'04'                  Turn on DAT
*
         L     R5,VADDRTO              Get vaddr in 1st virtual page
         L     R6,VADDRFRM             Copy
*
         MVC   0(32,R5),PRIPG1         Set literal identifier in pages
         MVC   0(16,R6),FROMPRI        Set literal identifier in pages
*
         SAC   SECMODE                 Secondary mode
         MVC   0(32,R5),SECPG1         Set literal identifier in pages
         MVC   0(16,R6),FROMSEC        Set literal identifier in pages
*
         SAC   HOMEMODE                Home space mode
         MVC   0(32,R5),HOMPG1         Set literal identifier in pages
         MVC   0(16,R6),FROMHOM        Set literal identifier in pages
*
         SAC   ARMODE                  Enter AR mode
         L     R0,=X'10031003'         Initialize MVCOS controls; on
*                                       first pass below this will be
*                                       set to X'00000000' for 1st test
         MVI   PSWASC,X'0C'            Initialize PSW ASC ctl byte; on
*                                       first pass below this will be
*                                       set to X'00' (PRI) for 1st test
         MVI   PSWSTATE,X'01'          Initialize PSW state control; on
*                                       first pass below this will be
*                                       set to X'00',SUPRV for 1st test
                                                                EJECT
***********************************************************************
         LA    R14,2                   # of PSW state tests
STATE000 EQU   *
         XI    PSWSTATE,X'01'          Flip current PSW state ctl byte
         IC    R1,PSWSTATE             Get new state ctl byte
         EX    R1,SVCSTATE             Flip to next PSW state
         B     SKIP001                 Continue test prep
*
SVCSTATE SVC   0                       Executed instruction
SKIP001  EQU   *
                                                                SPACE 3
***********************************************************************
         LA    R13,4                   # of PSW ASC tests
ASC010   EQU   *
         IC    R1,PSWASC               Get last mode used
         LA    R1,X'04'(,R1)           Increment bit 5 to next ASC mode
         N     R1,=X'0000000C'         Keep only bits 4 and 5
         STC   R1,PSWASC               Set new mode to use
                                                                SPACE 3
***********************************************************************
         LA    R12,2                   # From A validity tests
TOA000   EQU   *
         LR    R1,R0                   Copy control bits
         N     R1,=A(OAC1A)            Keep only bits we want
         X     R1,=A(OAC1A)            Flip these bits
         N     R0,=A(X'FFFFFFFF'-OAC1A) Force these bits off
         OR    R0,R1                   Set ctl based on flip results
         N     R0,=A(X'FFFFFFFF'-(ASCHOM*65536)) Force bits off in OAC1
         LA    R11,1                   Assume 1 test if A=0 (using PSW)
         TMLH  R0,ASCA                 Was A set on or off?
         BZ    TOAS200                 A is off, use PSW ASC
*
*                                      A=1: rotate thru OAC1AS modes
         O     R0,=A(ASCHOM*65536)     Force on; will wrap to 00 next
                                                                EJECT
***********************************************************************
         LA    R11,4                   4 test to rotate thru OAC1 ASCs
TOAS000  EQU   *
         TMLH  R0,ASCA                 Was A set on or off?
         BZ    TOAS200                 A is off, use PSW ASC
*                                      A is on, do OAC1 AS changes
         LR    R1,R0                   Copy control bits
         A     R1,=X'00400000'         Increment bit to next ASC mode
         N     R1,=A(ASCHOM*65536)     Keep only the bits we want
         N     R0,=A(X'FFFFFFFF'-(ASCHOM*65536)) Force bits off in OAC1
         OR    R0,R1                   Set ctl based on flip results
*
         LA    R10,1                   1 test required if ASC is P,S,H
         LR    R1,R0                   Copy control bits
         N     R1,=A(OAC1A+ASCHOM*65536) Keep only these bits
         CL    R1,=A(OAC1A+ASCAR*65536) Using MVCOS control AR mode?
         BNE   TOAS290                 No. Use 1 test in R10 for P,S,H
         B     TOAS210                 Yes. 3 tests in R10 for AR
*
TOAS200  EQU   *
         LA    R10,1                   1 test required if PSW is P,S,H
         CLI   PSWASC,X'04'            Using ASC=AR ?
         BNE   TOAS290                 No. only 1 test per ASC mode
*
TOAS210  EQU   *
                                                                SPACE 3
***********************************************************************
         LA    R10,3                   3 tests required for ASC=AR
TOAS220  EQU   *
         L     R1,TALET                Get from ALET
         SAR   AR5,R1                  Set in from AR
         LA    R1,1(,R1)               Bump ALET
         C     R1,=F'3'                Exceeded max of 2?
         BL    TOAS230                 No
         SR    R1,R1                   Restart back at ALET 0
*
TOAS230  EQU   *
         ST    R1,TALET                Save updated ALET
*
TOAS290  EQU   *
                                                                EJECT
***********************************************************************
         LA    R9,2                    # From A validity tests
FRMA000  EQU   *
         LR    R1,R0                   Copy control bits
         N     R1,=A(OAC2A)            Keep only bits we want
         X     R1,=A(OAC2A)            Flip these bits
         N     R0,=A(X'FFFFFFFF'-OAC2A) Force these bits off
         OR    R0,R1                   Set ctl based on flip results
         N     R0,=A(X'FFFFFFFF'-ASCHOM) Force these bits off in OAC2
         LA    R8,1                    Assume 1 test if A=0 (using PSW)
         TMLL  R0,ASCA                 Was A set on or off?
         BZ    FRMAS200                A is off, use PSW ASC
*
*                                      A=1: rotate thru OAC2AS modes
         O     R0,=A(ASCHOM)           Force on; will wrap to 00 next
                                                                SPACE 3
***********************************************************************
         LA    R8,4                    4 test to rotate thru OAC2 ASCs
FRMAS000 EQU   *
         TMLL  R0,ASCA                 Was A set on or off?
         BZ    FRMAS200                A is off, use PSW ASC
*                                      A is on, do OAC2 AS changes
         LR    R1,R0                   Copy control bits
         LA    R1,B'01000000'(,R1)     Increment bit 1 to next ASC mode
         N     R1,=A(ASCHOM)           Keep only the bits we want
         N     R0,=A(X'FFFFFFFF'-ASCHOM) Force these bits off in OAC2
         OR    R0,R1                   Set ctl based on flip results
*
         LA    R7,1                    1 test required if ASC is P,S,H
         LR    R1,R0                   Copy control bits
         N     R1,=A(OAC2A+ASCHOM)     Keep only these bits
         CL    R1,=A(OAC2A+ASCAR)      Using MVCOS control AR mode?
         BNE   FRMAS290                No. Use 1 test in R7 for P,S,H
         B     FRMAS210                Yes. 3 tests in R7 for AR
*
FRMAS200 EQU   *
         LA    R7,1                    1 test required if PSW is P,S,H
         CLI   PSWASC,X'04'            Using ASC=AR ?
         BNE   FRMAS290                No. only 1 test per ASC mode
*
FRMAS210 EQU   *
                                                                EJECT
***********************************************************************
         LA    R7,3                    3 tests required for ASC=AR
FRMAS220 EQU   *
         L     R1,FALET                Get from ALET
         SAR   AR6,R1                  Set in from AR
         LA    R1,1(,R1)               Bump ALET
         C     R1,=F'3'                Exceeded max of 2?
         BL    FRMAS230                No
         SR    R1,R1                   Restart back at ALET 0
*
FRMAS230 EQU   *
         ST    R1,FALET                Save updated ALET
*
FRMAS290 EQU   *
                                                                SPACE 3
***********************************************************************
         LA    R4,2                    # To Key tests
TKEY000  EQU   *                       To Key
         LR    R1,R0                   Copy control bits
         N     R1,=A(OAC1KEY+OAC1K)    Keep only bits we want
         X     R1,=A(OAC1KEY+OAC1K)    Flip these bits
         N     R0,=A(X'FFFFFFFF'-OAC1KEY-OAC1K) Force these bits off
         OR    R0,R1                   Set ctl based on flip results
                                                                SPACE 3
***********************************************************************
         LA    R3,2                    # From Key tests
FKEY000  EQU   *                       From Key
         LR    R1,R0                   Copy control bits
         N     R1,=A(OAC2KEY+OAC2K)    Keep only bits we want
         X     R1,=A(OAC2KEY+OAC2K)    Flip these bits
         N     R0,=A(X'FFFFFFFF'-OAC2KEY-OAC2K) Force these bits off
         OR    R0,R1                   Set ctl based on flip results
*
         SR    R2,R2                   Clear for ICM
         IC    R2,PSWASC               Get ASC we need to test
         TM    SVCINTC+3,X'01'         Are we in problem state?
         BZ    BEGIN000                No.  Do every test
         CLM   R2,1,=X'0C'             Entering HOME ASC mode?
         BE    NEXTTEST                Y, not permitted in prob state
                                                                EJECT
***********************************************************************
*
*     Now do the actual 'MVCOS' and check the results afterwards
*
***********************************************************************
*
BEGIN000 EQU   *
         L     R1,SACIDX(R2)           Get corresponding SAC bits
         SAC   0(R1)                   Put machine in mode we need
*
         LA    R1,16                   Length to move
         MVCOS 8(R5),0(R6),R1
MVCOS    EQU   *                       Addr after the instruction
*
         SAC   ARMODE                  Resume AR mode
         SR    R1,R1                   Clear for IC
         IC    R1,PSWASC               Get PSW ASC mode bits
         TMLH  R0,ASCA                 Is AS setting in OAC1 valid?
         BZ    CHK010                  No, use the PSW ASC in R1
         LR    R1,R0                   Copy current setting
         N     R1,=X'00C00000'         Keep OAC1 AS bits
         SRL   R1,20                   Make AS value a 4-byte index
*
CHK010   EQU   *
         EX    R0,SETAR(R1)            Set AR1 to access the right AS
         L     R1,VADDRTO              -> literal target area
*
         SR    R2,R2                   Clear for IC
         IC    R2,PSWASC               Get PSW ASC mode bits
         TMLL  R0,ASCA                 Is AS setting in OAC2 valid?
         BZ    CHK020                  No, use the PSW ASC in R2
*
         LR    R2,R0                   Copy current setting
         N     R2,=X'000000C0'         Keep OAC2 AS bits
         SRL   R2,4                    Make AS value a 4-byte index
*
CHK020   EQU   *
         EX    R0,GETALET(R2)          Get the MVCOS operand 2 ALET
         SLL   R2,4                    Multiply by 16 to make index
         LA    R2,FROMPRI(R2)          -> space identifier literal
*
         CLC   8(16,R1),0(R2)          Check if MVCOS worked
         BE    CHK100                  TEST SUCCESS
         LPSWE BADMVCOS                Stop machine if test failed
*
CHK100   EQU   *
         AP    MVCOSOK,=P'1'           Increment # successful tests
         EAR   R2,AR1                  Get the ALET we loaded into AR1
         SLL   R2,5                    Multiply by 32 to make index
         LA    R2,PRIPG1(R2)           -> space identifier literal
         MVC   0(32,R1),0(R2)          Restore original id literal
                                                                EJECT
***********************************************************************
*                    Loop through all tests
***********************************************************************
NEXTTEST EQU   *
         BCT   R3,FKEY000              Vary OAC2 K bit
         BCT   R4,TKEY000              Vary OAC1 K bit
         BCT   R7,FRMAS220             Vary from ALETs when OAC2 ASC=AR
         BCT   R8,FRMAS000             Cycle through OAC2 ASC modes
         BCT   R9,FRMA000              Vary OAC2 A bit
         BCT   R10,TOAS220             Vary To ALETs when OAC1 ASC=AR
         BCT   R11,TOAS000             Cycle through OAC1 ASC modes
         BCT   R12,TOA000              Vary OAC1 A bit
         BCT   R13,ASC010              Switch to next PSW ASC mode
         BCT   R14,STATE000            Switch to next PSW state
                                                                SPACE 2
***********************************************************************
*                         END OF TEST
***********************************************************************
         SVC   0                       Back to supervisor state
         CP    MVCOSOK,=P'384'         Expected count?
         BNE   FAILTEST                No, test failure
         CP    PIC04,=P'1152'          Expected count?
         BNE   FAILTEST                No, test failure
         CP    PIC13,=P'92'            Expected count?
         BNE   FAILTEST                No, test failure
         LPSWE TESTGOOD                Test SUCCESS
FAILTEST LPSWE TESTBAD                 Test FAILURE
                                                                SPACE 2
***********************************************************************
*        SETAR and GETALET are blocks of EXecuted instructions
***********************************************************************
*
SETAR    LAM   AR1,AR1,=F'0'   AS=00   Primary, set ALET=0
         CPYA  AR1,AR5            01   AR, set AR1 to MVCOS Operand 1
         LAM   AR1,AR1,=F'1'      10   Secondary, set ALET=1
         LAM   AR1,AR1,=F'2'      11   Home set ALET=2
                                                                SPACE 1
GETALET  LA    R2,0            AS=00   Primary, set ALET=0
         EAR   R2,AR6             01   AR, set R2 to MVCOS Operand 2 AR
         LA    R2,1               10   Secondary, set ALET=1
         LA    R2,2               11   Home set ALET=2
                                                                SPACE 2
***********************************************************************
* SVC 0: Set supervisor state in PSW, SVC 1: Set problem state in PSW
***********************************************************************
SVCFLIH  EQU   *                       SVC Interruption Routine
         MVC   SVCOPSW+1(1),SVCINTC+3  Set state based on SVC num
         LPSWE SVCOPSW                 Resume execution
                                                                EJECT
***********************************************************************
*                 HERE FOR PROGRAM CHECKS
***********************************************************************
*
PGMFLIH  EQU   *                       Program check interruptions
         CLI   PGMINTC+3,X'13'         Was this a special op exception?
         BE    PGM13                   Yes, use microscope
         CLI   PGMINTC+3,X'04'         Was this a protection exception?
         BNE   PGMSTOP                 No, stop immediate
*
PGM04    EQU   *                       Examine PIC 4
         ST    R0,WORK                 Save current control bits
         NC    WORK,=A(OAC1K+OAC2K)    Either key validity bit = 1?
         BZ    PGMSTOP                 N, PIC 04 from something else
         CLC   PGMOPSW+12(4),=A(MVCOS) Was it the MVCOS that failed?
         BNE   PGMSTOP                 Nope, halt the machine
         AP    PIC04,=P'1'             Increment # successful tests
         B     PGMEXIT                 Exit FLIH
*
PGM13    EQU   *
         TM    SVCINTC+3,X'01'         Were we in problem state?
         BZ    PGMSTOP                 No,error! PIC 13 shouldnt happen
         CLC   PGMOPSW+12(4),=A(MVCOS) Was it the MVCOS that failed?
         BNE   PGMSTOP                 Nope, halt the machine
         ST    R0,WORK                 Save current control bits
         NC    WORK,=A(OAC1A+OAC2A)    Either Access validity bit = 1?
         BZ    PGMSTOP                 N, PIC 13 from something else
         TMLH  R0,ASCHOM               Operand 1 ASC is HOME?
         BO    PGM13CT                 Yes, PIC13 is ok in prob state
         TMLL  R0,ASCHOM               Operand 2 ASC is HOME?
         BZ    PGMSTOP                 No. Something wrong
*
PGM13CT  EQU   *
         AP    PIC13,=P'1'             Increment # successful tests
*
PGMEXIT  EQU   *                       Exit from FLIH everything OK
         TM    SVCINTC+3,X'01'         Were we in problem state?
         BZ    NEXTTEST                No, proceed to next test
         SVC   1                       Return to problem state
         B     NEXTTEST                And return to next test
                                                                SPACE 3
***********************************************************************
*                   UNEXPECTED PROGRAM CHECK
***********************************************************************
PGMSTOP  EQU   *                       Halt if something wrong
         LPSWE HALT                    Here for unexpected prog checks
                                                                EJECT
***********************************************************************
*                        WORKING STORAGE
***********************************************************************
*
         LTORG
                                                                SPACE
         DC    0D'0'
PRIPG1   DC    CL16'PRI-PG1'            Eyecatcher
PRIPG2   DC    CL16'PRI-PG2'            Eyecatcher
SECPG1   DC    CL16'SEC-PG1'            Eyecatcher
SECPG2   DC    CL16'SEC-PG2'            Eyecatcher
HOMPG1   DC    CL16'HOM-PG1'            Eyecatcher
HOMPG2   DC    CL16'HOM-PG2'            Eyecatcher
*
FROMPRI  DC    CL16'FROMPRI1FROMPRI2'   Eyecatcher
FROMSEC  DC    CL16'FROMSEC1FROMSEC2'   Eyecatcher
FROMHOM  DC    CL16'FROMHOM1FROMHOM2'   Eyecatcher
                                                                SPACE 3
BADMVCOS DC    X'0402400080000000',XL4'00',X'0000BAD1'  MVCOS failed
TESTBAD  DC    X'0402400080000000',XL4'00',X'0000BAD9'  Test Failure
TESTGOOD DC    X'0402400080000000',XL4'00',X'00000000'  Test Success
HALT     DC    X'0402400080000000',XL4'00',X'0000DEAD'  Test Crashed!
                                                                SPACE 2
*        Control registers
*
CREGS    DC    0D'0'
CREG0    DC    X'00000000',X'04000000' Secondary space control bit 37
CREG1    DC    X'00000000',A(SEGPRI)   Primary ASCE
CREG2    DC    X'00000000',A(DUCT)     Dispatchable Unit Ctl Table
CREG3    DC    X'00000000',X'C0000001' PKM=C000, Secondary ASN=1
CREG4    DC    X'00000000',X'00000000' Primary ASN=0
CREG5    DC    X'00000000',A(PASTEO)   Primary ASTE Origin
CREG6    DC    X'00000000',X'00000000'
CREG7    DC    X'00000000',A(SEGSEC)   Secondary ASCE
CREG8    DC    X'00000000',X'00000000'
CREG9    DC    X'00000000',X'00000000'
CREG10   DC    X'00000000',X'00000000'
CREG11   DC    X'00000000',X'00000000'
CREG12   DC    X'00000000',X'00000000'
CREG13   DC    X'00000000',A(SEGHOM)   Home ASCE
CREG14   DC    X'00000000',X'00000000'
CREG15   DC    X'00000000',X'00000000'
                                                                SPACE 2
AREGS    DC    16F'0'                  Init for Access Registers
WORK     DC    F'0'                    Work area
FALET    DC    F'0'                    FROM ALET
TALET    DC    F'0'                    TO ALET
*
VADDRTO  DC    X'00010FF0'             Virtual addr within all 3 addr
*                                       space where the identifying
*                                       space literal is placed
*
VADDRFRM DC    X'00012FF8'             Virtual addr within all 3 addr
*                                       space where the 'from'
*                                       identifying literal is placed
                                                                SPACE 2
CONTROL  DC    0F'0'                   R0 MVCOS Control bits
OAC1     DC    H'0'                     1st OAC
OAC1KEY  EQU   X'10000000'               1st key
OAC1K    EQU   X'00020000'               1st key validity bit
OAC1A    EQU   X'00010000'               1st ASC validity bit
OAC2     DC    H'0'                     2nd OAC
OAC2KEY  EQU   X'00001000'               2nd key
OAC2K    EQU   X'00000002'               2nd key validity bit
OAC2A    EQU   X'00000001'               2nd ASC validity bit
                                                                EJECT
*        Bits in OAC1,OAC2 control bytes
*
KEY      EQU   X'0100' .... ...1 .... ....  Key 1 set
ASCPRI   EQU   X'0000' .... .... 00.. ....  Primary space
ASCAR    EQU   X'0040' .... .... 01.. ....  AR
ASCSEC   EQU   X'0080' .... .... 10.. ....  Secondary space
ASCHOM   EQU   X'00C0' .... .... 11.. ....  Home space
ASCK     EQU   X'0002' .... .... .... ..1.  Specified key valid
ASCA     EQU   X'0001' .... .... .... ...1  Specified AS valid
                                                                SPACE 3
SACIDX   DC    X'00000000'        Pri   SAC bits to set PSW 16-17=00
         DC    X'00000200'        AR                              01
         DC    X'00000100'        Sec                             10
         DC    X'00000300'        Home                            11
*
PSWSTATE DC    X'00'                    Tracks suprv/prob state in PSW
PSWASC   DC    X'00'                    Tracks ASC Mode setting of PSW
*                                        in bits 4-5 of this byte
*                                        during the MVCOS execution
                                                                SPACE 3
         ORG   STRTLABL+X'4000'
SEGPRI   DC    X'00000000',A(PAGPRI)
*
         ORG   STRTLABL+X'5000'
SEGSEC   DC    X'00000000',A(PAGSEC)
*
         ORG   STRTLABL+X'6000'
SEGHOM   DC    X'00000000',A(PAGHOM)
                                                                EJECT
         ORG   STRTLABL+X'7000'
PAGPRI   EQU   *                        Primary Space Page Tables
*
* Addresses 0-FFFF common to all addresses spaces, VIRT=REAL
         DC    X'00000000',X'00000000'  R=00000  V=00000
         DC    X'00000000',X'00001000'  R=01000  V=01000
         DC    X'00000000',X'00002000'  R=02000  V=02000
         DC    X'00000000',X'00003000'  R=03000  V=03000
         DC    X'00000000',X'00004000'  R=04000  V=04000
         DC    X'00000000',X'00005000'  R=05000  V=05000
         DC    X'00000000',X'00006000'  R=06000  V=06000
         DC    X'00000000',X'00007000'  R=07000  V=07000
         DC    X'00000000',X'00008000'  R=08000  V=08000
         DC    X'00000000',X'00009000'  R=09000  V=09000
         DC    X'00000000',X'0000A000'  R=0A000  V=0A000
         DC    X'00000000',X'0000B000'  R=0B000  V=0B000
         DC    X'00000000',X'0000C000'  R=0C000  V=0C000
         DC    X'00000000',X'0000D000'  R=0D000  V=0D000
         DC    X'00000000',X'0000E000'  R=0E000  V=0E000
         DC    X'00000000',X'0000F000'  R=0F000  V=0F000
                                                                SPACE 3
* Begin primary space only storage V-addrs 10000-1FFFF
         DC    X'00000000',X'00020000'  R=20000  V=10000
         DC    X'00000000',X'00021000'  R=21000  V=11000
         DC    X'00000000',X'00022000'  R=22000  V=12000
         DC    X'00000000',X'00023000'  R=23000  V=13000
         DC    X'00000000',X'00024000'  R=24000  V=14000
         DC    X'00000000',X'00025000'  R=25000  V=15000
         DC    X'00000000',X'00026000'  R=26000  V=16000
         DC    X'00000000',X'00027000'  R=27000  V=17000
         DC    X'00000000',X'00028000'  R=28000  V=18000
         DC    X'00000000',X'00029000'  R=29000  V=19000
         DC    X'00000000',X'0002A000'  R=2A000  V=1A000
         DC    X'00000000',X'0002B000'  R=2B000  V=1B000
         DC    X'00000000',X'0002C000'  R=2C000  V=1C000
         DC    X'00000000',X'0002D000'  R=2D000  V=1D000
         DC    X'00000000',X'0002E000'  R=2E000  V=1E000
         DC    X'00000000',X'0002F000'  R=2F000  V=1F000
                                                                EJECT
         ORG   STRTLABL+X'7800'
PAGSEC   EQU   *                        Secondary Space Page Tables
*
* Addresses 0-FFFF common to all addresses spaces, VIRT=REAL
         DC    X'00000000',X'00000000'  R=00000  V=00000
         DC    X'00000000',X'00001000'  R=01000  V=01000
         DC    X'00000000',X'00002000'  R=02000  V=02000
         DC    X'00000000',X'00003000'  R=03000  V=03000
         DC    X'00000000',X'00004000'  R=04000  V=04000
         DC    X'00000000',X'00005000'  R=05000  V=05000
         DC    X'00000000',X'00006000'  R=06000  V=06000
         DC    X'00000000',X'00007000'  R=07000  V=07000
         DC    X'00000000',X'00008000'  R=08000  V=08000
         DC    X'00000000',X'00009000'  R=09000  V=09000
         DC    X'00000000',X'0000A000'  R=0A000  V=0A000
         DC    X'00000000',X'0000B000'  R=0B000  V=0B000
         DC    X'00000000',X'0000C000'  R=0C000  V=0C000
         DC    X'00000000',X'0000D000'  R=0D000  V=0D000
         DC    X'00000000',X'0000E000'  R=0E000  V=0E000
         DC    X'00000000',X'0000F000'  R=0F000  V=0F000
                                                                SPACE 3
* Begin secondary space only storage V-addrs 10000-1FFFF
         DC    X'00000000',X'00030000'  R=30000  V=10000
         DC    X'00000000',X'00031000'  R=31000  V=11000
         DC    X'00000000',X'00032000'  R=32000  V=12000
         DC    X'00000000',X'00033000'  R=33000  V=13000
         DC    X'00000000',X'00034000'  R=34000  V=14000
         DC    X'00000000',X'00035000'  R=35000  V=15000
         DC    X'00000000',X'00036000'  R=36000  V=16000
         DC    X'00000000',X'00037000'  R=37000  V=17000
         DC    X'00000000',X'00038000'  R=38000  V=18000
         DC    X'00000000',X'00039000'  R=39000  V=19000
         DC    X'00000000',X'0003A000'  R=3A000  V=1A000
         DC    X'00000000',X'0003B000'  R=3B000  V=1B000
         DC    X'00000000',X'0003C000'  R=3C000  V=1C000
         DC    X'00000000',X'0003D000'  R=3D000  V=1D000
         DC    X'00000000',X'0003E000'  R=3E000  V=1E000
         DC    X'00000000',X'0003F000'  R=3F000  V=1F000
                                                                EJECT
         ORG   STRTLABL+X'8000'
PAGHOM   EQU   *                        Home Space Page Tables
*
* Addresses 0-FFFF common to all addresses spaces, VIRT=REAL
         DC    X'00000000',X'00000000'  R=00000  V=00000
         DC    X'00000000',X'00001000'  R=01000  V=01000
         DC    X'00000000',X'00002000'  R=02000  V=02000
         DC    X'00000000',X'00003000'  R=03000  V=03000
         DC    X'00000000',X'00004000'  R=04000  V=04000
         DC    X'00000000',X'00005000'  R=05000  V=05000
         DC    X'00000000',X'00006000'  R=06000  V=06000
         DC    X'00000000',X'00007000'  R=07000  V=07000
         DC    X'00000000',X'00008000'  R=08000  V=08000
         DC    X'00000000',X'00009000'  R=09000  V=09000
         DC    X'00000000',X'0000A000'  R=0A000  V=0A000
         DC    X'00000000',X'0000B000'  R=0B000  V=0B000
         DC    X'00000000',X'0000C000'  R=0C000  V=0C000
         DC    X'00000000',X'0000D000'  R=0D000  V=0D000
         DC    X'00000000',X'0000E000'  R=0E000  V=0E000
         DC    X'00000000',X'0000F000'  R=0F000  V=0F000
                                                                SPACE 3
* Begin home space only storage V-addrs 10000-1FFFF (V=R)
         DC    X'00000000',X'00010000'  R=10000  V=10000
         DC    X'00000000',X'00011000'  R=11000  V=11000
         DC    X'00000000',X'00012000'  R=12000  V=12000
         DC    X'00000000',X'00013000'  R=13000  V=13000
         DC    X'00000000',X'00014000'  R=14000  V=14000
         DC    X'00000000',X'00015000'  R=15000  V=15000
         DC    X'00000000',X'00016000'  R=16000  V=16000
         DC    X'00000000',X'00017000'  R=17000  V=17000
         DC    X'00000000',X'00018000'  R=18000  V=18000
         DC    X'00000000',X'00019000'  R=19000  V=19000
         DC    X'00000000',X'0001A000'  R=1A000  V=1A000
         DC    X'00000000',X'0001B000'  R=1B000  V=1B000
         DC    X'00000000',X'0001C000'  R=1C000  V=1C000
         DC    X'00000000',X'0001D000'  R=1D000  V=1D000
         DC    X'00000000',X'0001E000'  R=1E000  V=1E000
         DC    X'00000000',X'0001F000'  R=1F000  V=1F000
                                                                EJECT
         ORG   STRTLABL+X'9000'
PASTEO   DS    0XL64                    Primary ASN Second Table Entry
         DC    A(0)           +0        ATO
         DC    A(0)            4        AX,ATL
         DC    A(0),A(SEGPRI)  8        Primary ASCE (same as CREG1)
         DC    A(0)           16        ALD
         DC    A(0)           20        ASTESN
         DC    A(0)           24        LTD
         DC    A(0)           28        Ctl prog use
         DC    A(0)           32        Ctl prog use
         DC    A(0)           36        Ctl prog use
         DC    A(0)           40        unassigned
         DC    A(0)           44        ASTEIN
         DC    A(0)           48        unassigned
         DC    A(0)           58        unassigned
         DC    A(0)           56        unassigned
         DC    A(0)           60        unassigned
                                                                SPACE 3
*
*       Dispatchable Unit Control Table (DUCT)
*
*         This DUCT is used by the primary space programming
*         when in Access Register mode in order to use the DU-AL.
*
         ORG   STRTLABL+X'9100'
DUCT     DS    0XL64                    Dispatchable Unit Control Tbl
         DC    A(0)     +0              BASTEO
         DC    A(0)      4              SSASTEO
         DC    A(0)      8              unassigned
         DC    A(0)     12              SSASTESN
DUALD    DC    A(DUAL)  16              DU-AL origin
         DC    A(0)     20              PSW key masks
         DC    A(0)     24              unassigned
         DC    A(0)     28              unassigned
         DC    A(0)     32              Return addr high
         DC    A(0)     36              Return addr low
         DC    A(0)     40              unassigned
         DC    A(0)     44              TRCB
         DC    A(0)     48              unassigned
         DC    A(0)     52              unassigned
         DC    A(0)     56              unassigned
         DC    A(0)     60              unassigned
                                                                EJECT
*
*        Dispatchable Unit - Access List (DU-AL)
*
*         8 access list entries, only entry 2 is valid (AR ALET = 2)
*
         ORG   STRTLABL+X'9200'
DUAL     EQU   *                        DU Access List
ALE0     DC    X'80',15X'00'            ALE 0 invalid
ALE1     DC    X'80',15X'00'            ALE 1 invalid
*
ALE2     DS    0XL16                    ALE 2 -> HOME space
         DC    A(0)                     I,FO,P,ALESN,ALEAX all 0
         DC    A(0)                     unassigned
         DC    A(HASTEO)                Home space ASTE Origin
         DC    A(0)                     ASTESN seq # set to 0
*
ALE3     DC    X'80',15X'00'            ALE 3 invalid
ALE4     DC    X'80',15X'00'            ALE 4 invalid
ALE5     DC    X'80',15X'00'            ALE 5 invalid
ALE6     DC    X'80',15X'00'            ALE 6 invalid
ALE7     DC    X'80',15X'00'            ALE 7 invalid
                                                                SPACE 3
*        The HASTE is needed for ALET 2's ALE entry above
*
         ORG   STRTLABL+X'A000'
HASTEO   DS    0XL64                    Home ASN Second Table Entry
         DC    A(0)           +0        ATO
         DC    A(0)            4        AX,ATL
         DC    A(0),A(SEGHOM)  8        Home ASCE (same as CREG13)
         DC    A(0)           16        ALD
         DC    A(0)           20        ASTESN
         DC    A(0)           24        LTD
         DC    A(0)           28        Ctl prog use
         DC    A(0)           32        Ctl prog use
         DC    A(0)           36        Ctl prog use
         DC    A(0)           40        unassigned
         DC    A(0)           44        ASTEIN
         DC    A(0)           48        unassigned
         DC    A(0)           58        unassigned
         DC    A(0)           56        unassigned
         DC    A(0)           60        unassigned
                                                                SPACE 3
SECMODE  EQU   256      Secondary mode
HOMEMODE EQU   768      Home space mode
ARMODE   EQU   512      Enter AR mode
                                                                SPACE 2
         END START
