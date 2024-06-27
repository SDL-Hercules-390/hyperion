 TITLE '               Concurrent Block Update Consistency (CBUC) test'
***********************************************************************
*
*          Concurrent Block Update Consistency (CBUC) test
*
***********************************************************************
*
*  According to the POP, when storing a doubleword into a doubleword
*  using a memory copy operations, the destination storage area as
*  seen by other CPUs should ALWAYS present the complete operation,
*  and not any intermediate value.
*
*  What this means is, if the destination doubleword is 111... and
*  another CPU moves 222... to that area, any CPU that accesses the
*  destination doubleword should ALWAYS see either all 111... or all
*  222... but NEVER any intermediate value such as 1122111122221122.
*
*  Even though the 'MVC' and other instructions behave as if they
*  were moving one byte at a time, the hardware ensures that all
*  "Block Updates" (doubleword updates) are always CONSISTENT (i.e.
*  atomic), such that all bytes of a block are always updated at the
*  same time and never piecemeal.
*
*  This test attempts to detect any discrepancy in this area.
*
***********************************************************************
*
*                       Example test scripts
*
*                            (CBUC.tst)
*
* *Testcase CBUC (Concurrent Block Update Consistency)
* defsym      testdur  30   # (maximum test duration in seconds)
* mainsize    1
* numcpu      2
* sysclear
* archlvl     z/Arch
* loadcore    "$(testpath)/CBUC.core"
* script      "$(testpath)/CBUC.subtst"  &     # ('&' = async thread!)
* runtest     300                              # (subtst will stop it)
* *Done
* numcpu 1
*
*                          (CBUC.subtst)
*
* # CBUC test 'stop' thread...
* # This script is designed to run in a separate thread!
* pause $(testdur)    # Sleep for desired number of seconds
* r 500=FF            # And then force our test to stop
*
*
***********************************************************************
                                                                EJECT
***********************************************************************
*
*                        PROGRAMMING NOTE
*
*  The below loop values do NOT determine our test duration.  Rather,
*  it is our asynchronous 'cbuc.subtst' script that controls how long
*  our test runs by sleeping for the desired test duration number of
*  seconds and then sets the 'STOPFLAG' to a non-zero value to force
*  our test to end.  Using a value of zero for our loop value ensures
*  we can always support the maximum possible test duration.
*
***********************************************************************
                                                                SPACE
WRLOOPS  EQU   0                    Number of writer thread loops
RDLOOPS  EQU   0                    Number of reader thread loops
                                                                SPACE 2
***********************************************************************
*
*  CPU 1, in a tight loop, moves to the test area, using, in turn,
*  MVC, MVCL, and MVCLE, two alternate values: X'1111111111111111'
*  and X'2222222222222222'.
*
*  At the same time, CPU 0, also in a tight loop, using MVC, copies
*  the test area to a separate work area and verifies that the value
*  is either X'1111111111111111' or X'2222222222222222'. If any other
*  value is seen, then the test fails.
*
*  For the test to be relevant, it is best to perform this test on a
*  host system with more than one processor core. The more processors
*  (cores) that host system has, the better.
*
*  CPU 0:
*
*     MVC    WORK(8),DEST
*     CLC    WORK(4),WORK+4
*     BNE    FAIL
*     CLC    WORK(4),SRC1
*     BE     OK
*     CLC    WORK(4),SRC2
*     BNE    FAIL
*
*  CPU 1:
*
*     MVC    DEST(8),SRC1
*     MVCL   DEST(8),SRC2
*     MVCLE  DEST(8),SRC1
*     MVC    DEST(8),SRC2
*     MVCL   DEST(8),SRC1
*     MVCLE  DEST(8),SRC2
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
         ARCHLVL  MNOTE=NO
                                                                EJECT
***********************************************************************
*        Initiate the CBUC CSECT in the CODE region
*        with the location counter at 0
***********************************************************************
                                                                SPACE
CBUC     ASALOAD  REGION=CODE
                                                                SPACE 3
***********************************************************************
*        Define the z/Arch RESTART PSW
***********************************************************************
                                                                SPACE
PREVORG  EQU   *
         ORG   CBUC+X'1A0'
*        PSWZ  <sys>,<key>,<mwp>,<prog>,<addr>[,amode]
         PSWZ  0,0,0,0,X'200',64
         ORG   PREVORG
                                                                SPACE 3
***********************************************************************
*        Create IPL (restart) PSW
***********************************************************************
                                                                SPACE
         ASAIPL   IA=BEGIN
                                                                EJECT
***********************************************************************
*               The actual CBUC program itself...
***********************************************************************
                                                                SPACE
         USING CBUC,R0                  No base registers needed
                                                                SPACE
BEGIN    SLR   R0,R0                    Start clean
         LA    R1,1                     Request z/Arch mode
         SLR   R2,R2                    Start clean
         SLR   R3,R3                    Start clean
         SIGP  R0,R2,X'12'              Request z/Arch mode
                                                                SPACE
         SLR   R1,R1                    Start clean
         LA    R2,0                     Get our CPU number
         LA    R4,BEGIN2                Our restart entry point
         STH   R4,X'1AE'                Update restart PSW
         SIGP  R0,R2,X'06'              Restart our CPU
         B     SIG1FAIL                 WTF?! How did we get here?!
                                                                SPACE
BEGIN2   LA    R2,1                     Second CPU number
         LA    R4,WRITER                Point to its entry point
         STH   R4,X'1AE'                Update restart PSW
         SIGP  R0,R2,X'06'              Restart second CPU
         BNZ   SIG2FAIL                 WTF?! (SIGP failed!)
                                                                SPACE
         STCKF BEGCLOCK                 Get entry TOD
         NC    BEGCLOCK,=X'FFFFFFFFC0000000'  (0.25 seconds)
                                                                SPACE
WAITLOOP STCKF NOWCLOCK                 Get current TOD
         NC    NOWCLOCK,=X'FFFFFFFFC0000000'  (0.25 seconds)
         CLC   NOWCLOCK,BEGCLOCK        Has 0.25 seconds passed yet?
         BE    WAITLOOP                 Not yet. Keep waiting.
                                                                SPACE 5
READER   L     R0,RDCOUNT               R0 <== loop count
READLOOP CLI   STOPFLAG,X'00'           Are we being asked to stop?
         BNE   STOPTEST                 Yes, then do so.
                                                                SPACE
         MVC   WORK,READDEST            Grab copy of test value
                                                                SPACE
         CLC   WORK,PATTERN1            Is it all the first pattern?
         BNE   READ2                    No, check if second pattern
         BCT   R0,READLOOP              Otherwise keep looping...
         B     STOPTEST                 Done!
                                                                SPACE
READ2    CLC   WORK,PATTERN2            Is it all the second pattern?
         BNE   FAILTEST                 No?! Then *FAIL* immediately!
         BCT   R0,READLOOP              Otherwise keep looping...
         B     STOPTEST                 Done!
                                                                EJECT
WRITER   L     R0,WRCOUNT               R0 <== loop count
WRITLOOP CLI   STOPFLAG,X'00'           Are we being asked to stop?
         BNE   STOPTEST                 Yes, then do so.

         TM    OPTFLAG,OPTMVC
         BZ             NOMVC1
         MVC   WRITDEST,PATTERN1        Move 1st pattern to target
NOMVC1   EQU   *
                                                                SPACE
         TM    OPTFLAG,OPTMVCL
         BZ             NOMVCL1
         LA    R6,WRITDEST              R6 --> destination
         LA    R7,L'WRITDEST            R7 <== destination length
         LA    R8,PATTERN2              R8 --> source
         LR    R9,R7                    R9 <== source length
         MVCL  R6,R8                    move source to destination
NOMVCL1  EQU   *
                                                                SPACE
         TM    OPTFLAG,OPTMVCLE
         BZ             NOMVCLE1
         LA    R6,WRITDEST              R6 --> destination
         LA    R7,L'WRITDEST            R7 <== destination length
         LA    R8,PATTERN1              R8 --> source
         LR    R9,R7                    R9 <== source length
         MVCLE R6,R8,0                  move source to destination
NOMVCLE1 EQU   *
                                                                SPACE
         TM    OPTFLAG,OPTMVC
         BZ             NOMVC2
         MVC   WRITDEST,PATTERN2        Move 1st pattern to target
NOMVC2   EQU   *
                                                                SPACE
         TM    OPTFLAG,OPTMVCL
         BZ             NOMVCL2
         LA    R6,WRITDEST              R6 --> destination
         LA    R7,L'WRITDEST            R7 <== destination length
         LA    R8,PATTERN1              R8 --> source
         LR    R9,R7                    R9 <== source length
         MVCL  R6,R8                    move source to destination
NOMVCL2  EQU   *
                                                                SPACE
         TM    OPTFLAG,OPTMVCLE
         BZ             NOMVCLE2
         LA    R6,WRITDEST              R6 --> destination
         LA    R7,L'WRITDEST            R7 <== destination length
         LA    R8,PATTERN2              R8 --> source
         LR    R9,R7                    R9 <== source length
         MVCLE R6,R8,0                  move source to destination
NOMVCLE2 EQU   *
                                                                SPACE
         BCT   R0,WRITLOOP              Otherwise keep looping...
         B     STOPTEST                 Done.
                                                                EJECT
***********************************************************************
*                            PSWs
***********************************************************************
                                                                SPACE 3
FAILFLAG DC       X'00'                 X'FF' == test has failed
                                                                SPACE
STOPTEST CLI      FAILFLAG,X'00'        Should test end normally?
         BNE      FAILTEST              No! Test has failed!
                                                                SPACE
         MVI      STOPFLAG,X'FF'        Tell the other CPU to stop
         DWAITEND LOAD=YES              Normal completion
                                                                SPACE 4
FAILTEST MVI      FAILFLAG,X'FF'        Indicate test has failed!
         MVI      STOPFLAG,X'FF'        Tell the other CPU to stop
         DWAIT    LOAD=YES,CODE=BAD     Abnormal termination
                                                                SPACE 4
SIG1FAIL MVI      STOPFLAG,X'FF'        Tell the other CPU to stop
         DWAIT    LOAD=YES,CODE=111     First SIGP failed
                                                                SPACE 4
SIG2FAIL MVI      STOPFLAG,X'FF'        Tell the other CPU to stop
         DWAIT    LOAD=YES,CODE=222     Second SIGP failed
                                                                EJECT
***********************************************************************
*                       Working Storage
***********************************************************************
                                                                SPACE
RDCOUNT  DC    A(RDLOOPS)               Number of reader thread loops
WRCOUNT  DC    A(WRLOOPS)               Number of writer thread loops
                                                                SPACE
         LTORG ,                        Literals pool
                                                                SPACE 4
         ORG   CBUC+X'400'-8
                                                                SPACE
         DC    XL5'0000000000'          Unaligned writer destination
WRITDEST DS    0CL16                    Writer thread destination
         DC    CL3'AAA'
READDEST DC    CL8'BBBBBBBB'            MUST be doubleword ALIGNED!
         DC    CL5'AAAAA'
                                                                SPACE
BEGCLOCK DC    D'0'                     CPU 0 entry TOD
NOWCLOCK DC    D'0'                     CPU 0 start TOD
                                                                SPACE 4
         ORG   CBUC+X'500'              Fixed address of 'stop' flag
                                                                SPACE
STOPFLAG DC    X'00'                    Set to non-zero to stop test
PATTERN1 DC    CL16'AAAAAAAAAAAAAAAA'   Should be unaligned
         DC    XL2'0000'
PATTERN2 DC    CL16'BBBBBBBBBBBBBBBB'   Should also be unaligned
                                                                SPACE 4
         ORG   CBUC+X'600'              Fixed address of 'option' flag
                                                                SPACE
OPTMVC   EQU   X'80'                    Use 'MVC'   in write loop
OPTMVCL  EQU   X'40'                    Use 'MVCL'  in write loop
OPTMVCLE EQU   X'20'                    Use 'MVCLE' in write loop
                                                                SPACE
OPTFLAG  DC    AL1(OPTMVC+OPTMVCL+OPTMVCLE)   Test options flag
                                                                SPACE 4
         ORG   CBUC+X'800'
                                                                SPACE
WORK     DC    CL8' '                   MUST be doubleword ALIGNED!
                                                                EJECT
                                                                SPACE 3
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
                                                                SPACE 4
         END
