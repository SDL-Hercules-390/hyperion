 TITLE '            CLCE-03-basic (Test CLCLE instructions)'
***********************************************************************
*
*        CLCE  instruction tests
*
*        NOTE: This is a copy of the CLCL-et-al Test
*              modified to only test the CLCLE instruction.
*              Specifically, instuction
*
*              CLCL  R10,R12
*
*              was changed to
*
*              CLCLE R10,R12,0
*              BC    B'0001',*-4          not finished?
*
*
*        James Wekel August 2022
***********************************************************************
***********************************************************************
*
*  This program tests proper functioning of the CLCLE instructions.
*
*  PLEASE NOTE that the tests are very SIMPLE TESTS designed to catch
*  obvious coding errors.  None of the tests are thorough.  They are
*  NOT designed to test all aspects of any of the instructions.
*
***********************************************************************
*
*  Example Hercules Testcase:
*
*
*        *Testcase CLCE-03-basic (Test CLCLE instructions)
*
*        archlvl     390
*        mainsize    3
*        numcpu      1
*        sysclear
*
*        loadcore    "$(testpath)/CLCLE-03-basic.core" 0x0
*
*        runtest     1
*        *Done
*
***********************************************************************
                                                                EJECT
         PRINT OFF
         COPY  'satk.mac'
         PRINT ON
                                                                SPACE
***********************************************************************
*        SATK prolog stuff...
***********************************************************************
                                                                SPACE
         ARCHLVL  ZARCH=NO,MNOTE=NO
                                                                EJECT
***********************************************************************
*        Initiate the CLCLE03 CSECT in the CODE region
*        with the location counter at 0
***********************************************************************
                                                                SPACE
CLCLE03  ASALOAD  REGION=CODE
                                                                SPACE 3
***********************************************************************
*        Create IPL (restart) PSW
***********************************************************************
                                                                SPACE
         ASAIPL   IA=BEGIN
                                                                EJECT
***********************************************************************
*               The actual "CLCLE03" program itself...
***********************************************************************
*
*  Architecture Mode: 390
*  Addressing Mode:   31-bit
*  Register Usage:
*
*        R0       (work)
*        R1
*        R2       First base register
*        R3
*        R4
*        R5-R7    (work)
*        R8
*        R9       Second base register
*        R10-R13  (work)
*        R14      Subroutine call
*        R15      Secondary Subroutine call or work
*
***********************************************************************
                                                                SPACE
         USING  ASA,R0          Low core addressability
         USING  BEGIN,R2        FIRST Base Register
         USING  BEGIN+4096,R9   SECOND Base Register
                                                                SPACE
BEGIN    BALR  R2,0             Initalize FIRST base register
         BCTR  R2,0             Initalize FIRST base register
         BCTR  R2,0             Initalize FIRST base register
                                                                SPACE
         LA    R9,2048(,R2)     Initalize SECOND base register
         LA    R9,2048(,R9)     Initalize SECOND base register
                                                                SPACE
*
**       Run the tests...
*
         BAL   R14,TEST01       Test CLCLE  instruction
*
         BAL   R14,TEST91       Test CLCLE page fault handling
                                                                EJECT
***********************************************************************
*         Test for normal or unexpected test completion...
***********************************************************************
                                                                SPACE
         CLI   TESTNUM,X'91'    Did we end on expected test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         CLI   SUBTEST,X'10'    Did we end on expected SUB-test?
         BNE   FAILTEST         No?! Then FAIL the test!
                                                                SPACE
         B     EOJ              Yes, then normal completion!
                                                                EJECT
***********************************************************************
*        TEST01                   Test CLCLE instruction
***********************************************************************
                                                                SPACE
TEST01   MVI   TESTNUM,X'01'
*
**       Initialize test parameters...
*
         LM     R5,R6,CLCL4     CLCL4 test Op1 address and length
         ALR    R5,R6           Point past last byte
         BCTR   R5,0            Backup to last byte
         MVI    0(R5),X'FF'     Force unequal compare (op1 high)
*
         LM     R5,R6,CLCLOP1   (same thing for CLCLOP1 test)
         ALR    R5,R6                        "
         BCTR   R5,0                         "
         MVI    0(R5),X'FF'                  "
*
         LM     R5,R6,CLCL8+8   CLCL8 test ===> OP2 <===
         ALR    R5,R6
         BCTR   R5,0
         MVI    0(R5),X'FF'     ===> OPERAND-2 high (OP1 LOW) <===
*
**       Neither cross (one byte)
*
         MVI   SUBTEST,X'01'
         LM    R10,R13,CLCL1
         CLCLE R10,R12,0
         BC    B'0001',*-4          not finished?
         BNE   FAILTEST
         LA    R5,ECLCL1
         BAL   R15,ENDCLCL
*
**       Neither cross (two bytes)
*
         MVI   SUBTEST,X'02'
         LM    R10,R13,CLCL2
         CLCLE R10,R12,0
         BC    B'0001',*-4          not finished?
         BNE   FAILTEST
         LA    R5,ECLCL2
         BAL   R15,ENDCLCL
*
**       Neither cross (four bytes)
**       (inequality on last byte of op1)
*
         MVI   SUBTEST,X'04'
         LM    R10,R13,CLCL4
         CLCLE R10,R12,0
         BC    B'0001',*-4          not finished?
         BNH   FAILTEST                 (see INIT; CLCL4:   op1 > op2)
         LA    R5,ECLCL4
         BAL   R15,ENDCLCL
                                                                EJECT
*
**       Neither cross (eight bytes)
**       (inequality on last byte of op2)
*
         MVI   SUBTEST,X'08'
         LM    R10,R13,CLCL8
         CLCLE R10,R12,0
         BC    B'0001',*-4          not finished?
         BNL   FAILTEST                 (see INIT; CLCL8:   op1 < op2)
         LA    R5,ECLCL8
         BAL   R15,ENDCLCL
*
**       Neither cross (1K bytes)
*
         MVI   SUBTEST,X'00'
         LM    R10,R13,CLCL1K
         CLCLE R10,R12,0
         BC    B'0001',*-4          not finished?
         BNE   FAILTEST
         LA    R5,ECLCL1K
         BAL   R15,ENDCLCL
*
**       Both cross
*
         MVI   SUBTEST,X'22'
         LM    R10,R13,CLCLBOTH
         CLCLE R10,R12,0
         BC    B'0001',*-4          not finished?
         BNE   FAILTEST
         LA    R5,ECLCLBTH
         BAL   R15,ENDCLCL
*
**       Only op1 crosses
**       (inequality on last byte of op1)
*
         MVI   SUBTEST,X'10'
         LM    R10,R13,CLCLOP1
         CLCLE R10,R12,0
         BC    B'0001',*-4          not finished?
         BNH   FAILTEST                 (see INIT; CLCLOP1: op1 > op2)
         LA    R5,ECLCLOP1
         BAL   R15,ENDCLCL
*
**       Only op2 crosses
*
         MVI   SUBTEST,X'20'
         LM    R10,R13,CLCLOP2
         CLCLE R10,R12,0
         BC    B'0001',*-4          not finished?
         BNE   FAILTEST
         LA    R5,ECLCLOP2
         BAL   R15,ENDCLCL
*
         BR    R14
                                                                EJECT
***********************************************************************
*        TEST91                   Test CLCLE page fault handling
***********************************************************************
                                                                SPACE
TEST91   MVI   TESTNUM,X'91'
         MVI   SUBTEST,X'00'
*
**       First, make sure we start clean!
*
         LM    R10,R13,CLCLPF     Retrieve CLCLE PF test parameters
         MVCL  R10,R12            (forces full comparison)
*
**       Initialize Dynamic Address Translation tables...
*
         L     R10,=A(SEGTABLS)   Segment Tables Origin
         LA    R11,NUMPGTBS       Number of Segment Table Entries
         L     R12,=A(PAGETABS)   Page Tables Origin
         SLR   R0,R0              First Page Frame Address
         LA    R6,4               Size of one table entry
         L     R7,=A(PAGE)        Size of one Page Frame
                                                                SPACE
SEGLOOP  ST    R12,0(,R10)        Seg Table Entry <= Page Table Origin
         OI    3(R10),X'0F'       Seg Table Entry <= Page Table Length
         ALR   R10,R6             Bump to next Segment Table Entry
                                                                SPACE
         LA    R13,16             Page Table Entries per Page Table
PAGELOOP ST    R0,0(,R12)         Page Table Entry = Page Frame Address
         ALR   R0,R7              Increment to next Page Frame Address
         ALR   R12,R6             Bump to next Page Table Entry
         BCT   R13,PAGELOOP       Loop until Page table is complete
                                                                SPACE
         BCT   R11,SEGLOOP        Loop until all
*                                    Segment Table Entries built
*
**       Update desired page table entry to cause page fault
*
         LM    R10,R13,CLCLPF     Retrieve CLCLE PF test parameters
         LR    R5,R10             R5 --> Operand-1
         AL    R5,=A(PFPGBYTS)    R5 --> Operand-1 Page Fault address
         LR    R6,R5              R6 --> Address where PF should occur
         SRL   R5,12              R5 = Page Frame number
         SLL   R5,2               R5 = Page Table Entry number
                                                                SPACE
         MVI   SUBTEST,X'04'
         AL    R5,=A(PAGETABS)    R5 --> Page Table Entry
         OI    2(R5),X'04'        Mark this page invalid
                                                                EJECT
*
**       Install program check routine to catch the page fault
*
         MVI   SUBTEST,X'02'
         MVC   SVPGMNEW,PGMNPSW   Save original Program New PSW
         LA    R0,MYPGMNEW        Point to temporary Pgm New routine
         ST    R0,PGMNPSW+4       Point Program New PSW to our routine
         MVI   PGMNPSW+1,X'08'    Make it a non-disabled-wait PSW!
*
**       Run the test: should cause a page fault
*
         MVI   SUBTEST,X'0F'
         LCTL  R0,R0,CRLREG0      Switch to DAT mode
         LCTL  R1,R1,CTLREG1      Switch to DAT mode
         LPSW  DATONPSW           Switch to DAT mode
BEGDATON NOP   *                  (pad)
         NOP   *                  (pad)
         PTLB  ,                  Purge Translation Lookaside Buffer
PFINSADR CLCLE R10,R12,0          Page Fault should occur on this instr
         BC    B'0001',*-4        not finished?
         CNOP  0,8                        (align to doubleword)
LOGICERR DC    D'0'                       We should never reach here!
SVPGMNEW DC    D'0'                       Original Program New PSW
DATONPSW DC    XL4'04080000',A(BEGDATON)  Enable DAT PSW
*
**       Temporary Program New routine:
**       Restore original Program New PSW
*
MYPGMNEW MVC   PGMNPSW,SVPGMNEW   Restore original Program New PSW
*
**       Verify Program Check occurred on expected instruction
*
         MVI   SUBTEST,X'68'
         CLC   =A(PFINSADR),PGMOPSW+4   Program Check where expected?
         BNE   FAILTEST                 No?! Something is VERY WRONG!
*
**       Verify Program Check was indeed a page fault
*
         MVI   SUBTEST,X'11'
         CLI   PGMICODE+1,X'11'   Verify it's a Page Fault interrupt
         BNE   FAILTEST           If not then something is VERY WRONG!
                                                                EJECT
*
**       Verify Page Fault occurred on expected Page
*
         MVI   SUBTEST,X'05'
         L     R0,PGMTRX          Get where Page Fault occurred
         SRL   R0,12
         SLL   R0,12
                                                                SPACE
         SRL   R6,12              Where Page Fault is expected
         SLL   R6,12
                                                                SPACE
         CLR   R0,R6              Page Fault occur on expected Page?
         BNE   FAILTEST           No? Then something is very wrong!
*
**       Verify CLCLE instruction registers were updated as expected
*
         MVI   SUBTEST,X'06'
         CL    R10,CLCLPF         (op1 greater than starting value?)
         BNH   FAILTEST
         CL    R12,CLCLPF+4+4     (op2 greater than starting value?)
         BNH   FAILTEST
                                                                SPACE
         MVI   SUBTEST,X'07'
         CLR   R11,R13            (same remaining lengths?)
         BNE   FAILTEST
         CL    R11,CLCLPF+4       (op1 len less than starting value?)
         BNL   FAILTEST
         CL    R13,CLCLPF+4+4+4   (op2 len less than starting value?)
         BNL   FAILTEST
                                                                SPACE
         MVI   SUBTEST,X'08'
         CL    R10,ECLCLPF        (stop before end?)
         BNL   FAILTEST
                                                                SPACE
         MVI   SUBTEST,X'09'
         CLR   R10,R6             (stop at or before expected page?)
         BH    FAILTEST
                                                                SPACE
         MVI   SUBTEST,X'10'
         LR    R7,R10             (op1 stopped address)
         ALR   R7,R11             (add remaining length)
         CLR   R7,R6              (would remainder reach PF page?)
         BNH   FAILTEST
                                                                SPACE
         BR    R14                Success!
                                                                EJECT
***********************************************************************
*        Verify CLCLE ending register values
* R10-R12 = actual ending values, R5 --> expected ending values
***********************************************************************
                                                                SPACE
ENDCLCL  STM    R10,R13,CLCLEND     Save actual ending register values
         CLC    0(4*4,R5),CLCLEND   Do they have the expected values?
         BNE    FAILTEST            If not then the test has failed
         BR     R15                 Otherwise return to caller
                                                                SPACE
                                                                EJECT
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 5
EOJ      DWAITEND LOAD=YES          Normal completion
                                                                SPACE 5
FAILTEST DWAIT LOAD=YES,CODE=BAD    Abnormal termination
                                                                EJECT
***********************************************************************
*        Working Storage
***********************************************************************
                                                                SPACE 2
         LTORG ,                Literals pool
                                                                SPACE
K        EQU   1024             One KB
PAGE     EQU   (4*K)            Size of one page
K64      EQU   (64*K)           64 KB
MB       EQU   (K*K)             1 MB
                                                                SPACE
TESTADDR EQU   (2*PAGE+X'200'-2)  Where test/subtest numbers will go
                                                                SPACE
MAINSIZE EQU   (2*MB)                   Minimum required storage size
NUMPGTBS EQU   ((MAINSIZE+K64-1)/K64)   Number of Page Tables needed
NUMSEGTB EQU   ((NUMPGTBS*4)/(16*4))    Number of Segment Tables
SEGTABLS EQU   (3*PAGE)                 Segment Tables Origin
PAGETABS EQU   (SEGTABLS+(NUMPGTBS*4))  Page Tables Origin
CRLREG0  DC    0A(0),XL4'00B00060'      Control Register 0
CTLREG1  DC    A(SEGTABLS+NUMSEGTB)     Control Register 1
                                                                SPACE
                                                                EJECT
***********************************************************************
*        CLCLE Test Parameters:   A(operand-1),A(operand-2)
***********************************************************************
                                                                SPACE
CLC1     DC    A(1*K64),A(MB+(1*K64))                       both equal
CLC2     DC    A(1*K64),A(MB+(1*K64))                       both equal
CLCBOTH  DC    A(1*K64-12),A(MB+(1*K64)-34)                 both equal
CLCOP2   DC    A(1*K64),A(MB+(1*K64)-34)                    both equal
                                                                SPACE
CLC4     DC    A(2*K64),A(MB+(2*K64))                         op1 HIGH
CLC8     DC    A(3*K64),A(MB+(3*K64))                         op1 LOW!
CLC256   DC    A(4*K64),A(MB+(4*K64))                         op1 HIGH
CLCOP1   DC    A(5*K64-12),A(MB+(5*K64))                      op1 HIGH
                                                                SPACE 2
CLCLE03 CSECT ,
                                                                EJECT
***********************************************************************
*        CLCLE Test Parameters
***********************************************************************
                                                                SPACE
CLCL1    DC    A(6*K64),A(1),A(MB+(6*K64)),A(1)             both equal
                                                                SPACE 2
CLCL2    DC    A(6*K64),A(2),A(MB+(6*K64)),A(2)             both equal
                                                                SPACE 2
CLCL256  DC    A(6*K64),A(256),A(MB+(6*K64)),A(256)         both equal
                                                                SPACE 2
CLCL1K   DC    A(6*K64),A(K),A(MB+(6*K64)),A(K)             both equal
                                                                SPACE 2
CLCLBOTH DC    A(6*K64-12),A(K64),A(MB+(6*K64)-34),A(K64)   both equal
                                                                SPACE 2
CLCLOP2  DC    A(6*K64),A(PAGE),A(MB+(6*K64)-34),A(K64)     both equal
                                                                SPACE 2
CLCL4    DC    A(7*K64),A(4),A(MB+(7*K64)),A(4)               op1 HIGH
                                                                SPACE 2
CLCL8    DC    A(8*K64),A(8),A(MB+(8*K64)),A(8)               op1 LOW!
                                                                SPACE 2
CLCLOP1  DC    A(9*K64-12),A(K64),A(MB+(9*K64)),A(PAGE)       op1 HIGH
                                                                SPACE 2
CLCLPF   DC    A(10*K64),A(K64),A(MB+(10*K64)),A(K64)       page fault
                                                                EJECT
***********************************************************************
*        CLCLE Expected Ending Register Values
***********************************************************************
                                                                SPACE
ECLCL1   DC    A(6*K64+1),A(0),A(MB+(6*K64)+1),A(0)         both equal
                                                                SPACE 2
ECLCL2   DC    A(6*K64+2),A(0),A(MB+(6*K64)+2),A(0)         both equal
                                                                SPACE 2
ECLCL256 DC    A(6*K64+256),A(0),A(MB+(6*K64)+256),A(0)     both equal
                                                                SPACE 2
ECLCL1K  DC    A(6*K64+K),A(0),A(MB+(6*K64)+K),A(0)         both equal
                                                                SPACE 2
ECLCLBTH DC    A(6*K64-12+K64),A(0),A(MB+(6*K64)-34+K64),A(0) bth equl
                                                                SPACE 2
ECLCLOP2 DC    A(6*K64+PAGE),A(0),A(MB+(6*K64)-34+K64),A(0) both equal
                                                                SPACE 2
ECLCL4   DC    A(7*K64+4-1),A(1),A(MB+(7*K64)+4-1),A(1)       op1 HIGH
                                                                SPACE 2
ECLCL8   DC    A(8*K64+8-1),A(1),A(MB+(8*K64)+8-1),A(1)       op1 LOW!
                                                                SPACE 2
ECLCLOP1 DC    A(9*K64-12+K64-1),A(1),A(MB+(9*K64)+PAGE),A(0) op1 HIGH
                                                                SPACE 2
ECLCLPF  DC    A(10*K64+K64),A(0),A(MB+(10*K64)+K64),A(0)   page fault
                                                                SPACE 3
CLCLEND  DC    4F'0'          (actual ending register values)
PFPAGE   EQU   5              (page the Page Fault should occur on)
PFPGBYTS EQU   (PFPAGE*PAGE)  (number of bytes into operand-1)
                                                                EJECT
***********************************************************************
*        Fixed storage locations
***********************************************************************
                                                                SPACE 5
         ORG   CLCLE03+TESTADDR      (s/b @ X'21FE', X'21FF')
                                                                SPACE
TESTNUM  DC    X'00'      Test number of active test
SUBTEST  DC    X'00'      Active test sub-test number
                                                                SPACE 9
         ORG   CLCLE03+SEGTABLS      (s/b @ X'3000')
                                                                SPACE
DATTABS  DC    X'00'      Segment and Page Tables will go here...
                                                                EJECT
***********************************************************************
*        (other DSECTS needed by SATK)
***********************************************************************
                                                                SPACE
         DSECTS PRINT=OFF,NAME=(ASA,SCHIB)
         PRINT ON
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
         END
