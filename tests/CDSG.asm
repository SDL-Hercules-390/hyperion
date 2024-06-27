 TITLE ' Testcase cmpxchg16 as used by CDSG, STPQ and LPQ instructions'
***********************************************************************
*
*  Testcase cmpxchg16 as used by CDSG, STPQ and LPQ instructions
*
***********************************************************************
*
*  CDSG is one of the instructions that the POP refers to as having
*  Interlocked-Update References, also known as being 'atomic'.  This
*  instruction can be implemented using compiler intrinsics for certain
*  host architectures.  On X86-64 / AMD64 processors, the "cmpxchg16b"
*  instruction can be used when available.
*
*  This means that whilst one CPU performs a CDSG instruction, its
*  memory references are interlocked against those by other CPU's.
*  This test attempts to verify this, with an approach, similar to the
*  one in the CBUC test.
*
*  The STPQ and LPQ instructions also use "cmpxchg" and are tested
*  in here as well.
*
***********************************************************************
                                                                SPACE 3
***********************************************************************
*
*                       Example test scripts
*
*                            (CDSG.tst)
*
* *Testcase for cmpxchg16 as used by CDSG, STPQ and LPQ instructions
* mainsize    1
* numcpu      2
* sysclear
* archlvl     z/Arch
* loadcore    "$(testpath)/CDSG.core"
* runtest     1
* v 900.B0
* *Done
*
***********************************************************************
                                                                EJECT
***********************************************************************
*
*                        PROGRAMMING NOTE
*
*  During initialisation we test the functionality of the Store Pair
*  to Quadword (STPQ) and load Pair from Quadword (LPQ) instructions.
*  We merely do this as these two intructions also rely on the
*  Hercules macro cmpchxg16 (as of the most recent updates), as does
*  the CDSG implementation.  This cmpxchg16 macro may be assisted,
*  depending on the Hercules host architecture.
*
*  After initialisation, a second CPU is started, at which point in
*  time both CPU's perform a loop lasting LOOPMAX iterations.  The
*  first of the loop centers around the CDSG instruction.  The second
*  loop performs 2 CSG instructions, the first one of which overlaps
*  with the 2nd half destination of CDSG.  These overlapping causes
*  CC=1 when it occurs.  As a result, the non-overlapping destinations
*  gets incremented exactly LOOPMAX times, the overlapping part twice
*  that.  This is what is being checked for success by the test.  By
*  inspecting the CDSGCNTR and CSG_CNTR, one can see how many attempts
*  were needed. These are always higher that LOOPMAX. The exact number
*  varies on every test run.
*
*  The DESTination area is 24 bytes long, and is initialised as
*  follows:
*
*     '1A1B2A2B3A3B00004A4B5A5B6A6B00007A7B8A8B9A9B0000'X
*
*  The first loop works against DEST1+DEST2 (the leftmost 16 bytes),
*  the second loop against DEST2+DEST3 (the rightmost 16 bytes).
*  Both loops implement an atomic "increment" with the value:
*
*     '00000000000000010000000000000001'X
*
*  Thus also for the second loop:
*
*                     '00000000000000010000000000000001'X
*
*  The process ends when both loops have incremented LOOPMAX times,
*  and the doubleword in the middle will have been incremented
*  exactly twice that amount -- that is, if the CDSG operation is
*  really atomic.  And that is what will decide a successfull CDSG
*  instruction or not.
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
*        Initiate the CDSG CSECT in the CODE region
*        with the location counter at 0
***********************************************************************
                                                                SPACE
CDSGTEST ASALOAD  REGION=CODE
                                                                SPACE 3
***********************************************************************
*        Define the z/Arch RESTART PSW
***********************************************************************
                                                                SPACE
PREVORG  EQU   *
         ORG   CDSGTEST+X'1A0'
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
*               The actual CDSG program itself...
***********************************************************************
                                                                SPACE
         USING CDSG,R0                  No base registers needed
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
BEGIN2   DS    0H
         LPQ   R4,INIT1                 Load INIT1+INIT2 using LPQ
         CG    R4,INIT1                 Did LPQ high DW work ...
         BNE   LPQFAIL1                 ... or not ?
         CG    R5,INIT2                 Did LPQ low DW work ...
         BNE   LPQFAIL2                 ... or not ?
         STPQ  R4,DEST1                 Store DEST1+DEST2 for CDSG use
*                                       R4+R5 for CDSGLOOP to use
         CG    R4,DEST1                 Did STPQ high DW work ...
         BNE   STPQFAL1                 ... or not ?
         CG    R5,DEST2                 Did STPQ low DW work ...
         BNE   STPQFAL2                 ... or not ?
         LA    R14,TRACE                Initialize CDSG trace pointer
                                                                SPACE
         LGR   R6,R5                    R6+R7 for CSG_LOOP to use
         LG    R7,INIT3                 Load INIT3 to initialize ...
         STG   R7,DEST3                 DEST3 so CSG_CPU can use it
                                                                SPACE
         LA    R2,1                     Second CPU number
         LA    R8,CSG_CPU               Point to its entry point
         STH   R8,X'1AE'                Update restart PSW
         SIGP  R0,R2,X'06'              Restart second CPU
         BNZ   SIG2FAIL                 WTF?! (SIGP failed!)
*        B     CDSG_CPU                 Otherwise get started
                                                                EJECT
***********************************************************************
*  The first loop incremenets the rightmost halfwords of DEST1 and
*  DEST2 with a single atomic CDSG instruction.  But the 2nd CPU will
*  attempt to do the same thing against DEST2 with a CSG instruction,
*  thus causing collisions now and then.  We keep track of the total
*  CDSG's, which is limited to CNTRMAX.  The loop is expected to end
*  before that, when exactly LOOPMAX successful increments (i.e. SWAP
*  operations) have taken place.
***********************************************************************
                                                                SPACE
CDSG_CPU XGR   R12,R12                  Initialise CDSG counter
                                                                SPACE
CDSGLOOP DS    0H
         MVI   3(R14),X'1'              Trace CC=1 from previous CDSG
                                                                SPACE
CDSG_CC0 DS    0H
         CLI   STOPFLAG,X'00'           Are we being asked to stop?
         BNE   GOODEOJ                  Yes, then do so.
                                                                SPACE
         LA    R12,1(R12)               Increment the CDSG counter
         ST    R12,CDSGCNTR             Update CDSG counter
         C     R12,CNTRMAX              CDSG counter overrun
         BH    CDSGCNTO                 Yes, that should never happen
                                                                SPACE
         LA    R8,1(R4)                 Increment DEST1
         LA    R9,1(R5)                 Increment DEST1
                                                                SPACE
         LA    R14,16(R14)              Point to the next TRACE entry
         STH   R9,0(R14)                Trace the CDSG DEST2 update
         STH   R8,4(R14)                Trace the CDSG DEST1 update
                                                                SPACE
         CDSG  R4,R8,DEST1              CDSG to attempt doing it
         BNE   CDSGLOOP                 The CSG_CPU came in between
                                                                SPACE
         CLM   R8,B'0011',LOOPMAX       End value reached ?
         BNL   CDSGEND                  Yes, CDSG incrementing ended
                                                                SPACE
         LGR   R4,R8                    Copy the incremented DEST1
         LGR   R5,R9                    Copy the incremented DEST2
         B     CDSG_CC0                 Go try the next CDSG
                                                                SPACE
CDSGEND  DS    0H
         CLC   DEST3+6(2),LOOPMAX       Is also CSG_LOOP ended yet ?
         BL    CDSGEND                  Spin-loop style waiting for it
*
**       OK, both loops are finished!
*
         CLC   DEST2+6(2),LOOPMAX2      DEST2 must be =2*LOOPMAX
         BE    GOODEOJ                  Yes, then we have success!
         B     FAILEOJ                  No, the test failed!!
                                                                EJECT
***********************************************************************
*  The second loop incremenets the rightmost halfwords of DEST2 and
*  DEST3, both of which use the atomic CSG instruction.  But the one
*  against DEST2 will collide now and then with the CDSG atomic
*  increments of DEST1+DEST2.  We keep track of the total number of
*  CSG's against DEST2, which is limited to CNTRMAX.  The loop is
*  expected to end before that, when DEST2 (and DEST3 also) have been
*  able to increment (i.e. SWAP) exactly LOOPMAX times.
***********************************************************************
                                                                SPACE
CSG_CPU  XGR   R13,R13                  Initialise CSG counter
         LG    R6,DEST2                 Initialise R6 for CSG
         LG    R7,DEST3                 Initialise R7 for CSG
         LA    R15,TRACE+8              Initialize CSG trace pointer
                                                                SPACE
CSG_LOOP DS    0H
         MVI   3(R15),X'1'              Trace CC=1 from previous CSG
                                                                SPACE
CSG_CC0  DS    0H
         CLI   STOPFLAG,X'00'           Are we being asked to stop?
         BNE   ENDNOTOK                 Yes, then do so.
                                                                SPACE
         LA    R13,1(R13)               Increment the CSG counter
         ST    R13,CSG_CNTR             Update CSG counter
         C     R13,CNTRMAX              CSG counter overrun
         BH    CSGCNTO                  Yes, that should never happen
                                                                SPACE
         LA    R10,1(R6)                Increment DEST2
         LA    R15,16(R15)              Point to the next TRACE entry
         STH   R10,0(R15)               Trace the CSG DEST2 update
         STH   R11,4(R15)               Trace the previous DEST3 update
                                                                SPACE
         CSG   R6,R10,DEST2             CSG to attempt doing it
         BNE   CSG_LOOP                 The CDSG_CPU came in between
                                                                SPACE
         LA    R11,1(R7)                Increments DEST3
                                                                SPACE
CSGLOOP2 DS    0H
         CSG   R7,R11,DEST3             CSG to attempt doing it
         BNE   CSGLOOP2                 CDSGEND read came in between
                                                                SPACE
         CLM   R11,B'0011',LOOPMAX      End value reached ?
         BNL   CSG_END                  Yes, CSG incrementing ended
                                                                SPACE
         LGR   R6,R10                   Copy the incremented DEST2
         LGR   R7,R11                   Copy the incremented DEST3
         B     CSG_CC0                  Go try the next CSG
                                                                SPACE
CSG_END  DS    0H
         CLC   DEST1+6(2),LOOPMAX       Is also CDSGLOOP ended yet ?
         BL    CSG_END                  Spin-loop style waiting for it
         B     ENDOK                    OK, both loops are finished
                                                                EJECT
***********************************************************************
*                      GOOD End Of Job
***********************************************************************
                                                                SPACE 3
GOODEOJ  DS       0H
         MVC      STATUS,=CL32'Success! CDSG, STPQ and LPQ: OK!'
                                                                SPACE
         L        R12,CDSGCNTR          Load CSDG counter
         CVD      R12,PACKED            Convert CDSG counter to packed
         MVC      CDSGCMPR,EDIT         Copy EDIT mask in CDSG counter
         ED       CDSGCMPR,PACKED+5     CDSG counter in zoned format
                                                                SPACE
         LH       R12,DEST1+6           Load CDSG SWAPs counter
         CVD      R12,PACKED            Convert it to packed
         MVC      CDSGSWAP,EDIT         Copy EDIT mask in CDSG SWAP ctr
         ED       CDSGSWAP,PACKED+5     CDSG SWAPS counter in zoned
                                                                SPACE
         L        R13,CSG_CNTR          Load CSG counter
         CVD      R13,PACKED            Convert CSG  counter to packed
         MVC      CSG_CMPR,EDIT         Copy EDIT mask in CSG  counter
         ED       CSG_CMPR,PACKED+5     CSG  counter in zoned format
                                                                SPACE
         LH       R13,DEST3+6           Load CSG SWAPs counter
         CVD      R13,PACKED            Convert it to packed
         MVC      CSG_SWAP,EDIT         Copy EDIT mask in CSG SWAP cntr
         ED       CSG_SWAP,PACKED+5     CSG SWAPS counter in zoned
                                                                SPACE
*        B        ENDOK                 Load successful completion PSW
                                                                EJECT
***********************************************************************
*                            PSWs
***********************************************************************
                                                                SPACE 3
ENDOK    DS       0H
         DWAITEND LOAD=YES              Normal completion
                                                                SPACE 5
ENDNOTOK DS       0H
         DWAIT    LOAD=YES,CODE=BAD     Abnormal termination
                                                                SPACE 5
FAILEOJ  MVI      STOPFLAG,X'FF'        Tell the other CPU to stop
         DWAIT    LOAD=YES,CODE=BAD     Abnormal termination
                                                                SPACE 5
SIG1FAIL MVI      STOPFLAG,X'FF'        Tell the other CPU to stop
         DWAIT    LOAD=YES,CODE=111     First SIGP failed
                                                                SPACE 5
SIG2FAIL MVI      STOPFLAG,X'FF'        Tell the other CPU to stop
         DWAIT    LOAD=YES,CODE=222     Second SIGP failed
                                                                EJECT
LPQFAIL1 MVI      STOPFLAG,X'FF'        Tell the other CPU to stop
         MVC      STATUS,=CL32'Failure! LPQ  Hi  does NOT match'
         DWAIT    LOAD=YES,CODE=333     LPQ High part failed
                                                                SPACE 4
LPQFAIL2 MVI      STOPFLAG,X'FF'        Tell the other CPU to stop
         MVC      STATUS,=CL32'Failure! LPQ  Lo  does NOT match'
         DWAIT    LOAD=YES,CODE=444     LPQ Low part failed
                                                                SPACE 4
STPQFAL1 MVI      STOPFLAG,X'FF'        Tell the other CPU to stop
         MVC      STATUS,=CL32'Failure! STPQ Hi  does NOT match'
         DWAIT    LOAD=YES,CODE=555     STPQ High part failes
                                                                SPACE 4
STPQFAL2 MVI      STOPFLAG,X'FF'        Tell the other CPU to stop
         MVC      STATUS,=CL32'Failure! STPQ Lo  does NOT match'
         DWAIT    LOAD=YES,CODE=666     STPQ Low part failed
                                                                SPACE 4
CDSGCNTO MVI      STOPFLAG,X'FF'        Tell the other CPU to stop
         MVC      STATUS,=CL32'Failure! CDSG    Counter Overrun'
         DWAIT    LOAD=YES,CODE=777     CDSG Counter Overrun
                                                                SPACE 4
CSGCNTO  MVI      STOPFLAG,X'FF'        Tell the other CPU to stop
         MVC      STATUS,=CL32'Failure! CBG     Counter Overrun'
         DWAIT    LOAD=YES,CODE=888     CSG Counter Overrun
                                                                EJECT
***********************************************************************
*                       Working Storage
***********************************************************************
                                                                SPACE
         LTORG ,                        Literals pool
                                                                SPACE
         ORG   CDSGTEST+X'900'
         CNOP  0,16                     MUST be quadword ALIGNED!
DEST1    DC    XL8'0000000000000000'    DEST1+DEST2 updated using CDSG
DEST2    DC    XL8'0000000000000000'    DEST2 updated using CSG
DEST3    DC    XL8'0000000000000000'    DEST3 updated whenever CSG
*                                       successfully updates DEST
         CNOP  0,16                     MUST be quadword ALIGNED!
INIT1    DC    XL8'1A1B2A2B3A3B0000'    Initial value for DEST1
INIT2    DC    XL8'4A4B5A5B6A6B0000'    Initial value for DEST2
INIT3    DC    XL8'7A7B8A8B9A9B0000'    Initial value for DEST3
                                                                SPACE
         CNOP  0,16                     So that the output looks better
STATUS   DC    CL32'Failure!'           Overall status message
COUNTERS DC    CL16'Instr. CMP  SWAP'   Title column
CDSG     DC    CL4'CDSG'                CDSG Instruction
CDSGCMPR DC    CL6' '                   CDSG instructions attempted
CDSGSWAP DC    CL6' '                   CDSG successful swaps done
CSG      DC    CL4'CSG '                CSG  Instruction
CSG_CMPR DC    CL6' '                   CSG  DEST2 instructions done
CSG_SWAP DC    CL6' '                   CSG  DEST2 successful swaps
                                                                SPACE
         CNOP  0,16                     So that the output looks better
CDSGCNTL DC    CL8'CDSGCNTR'            CDSG counter label
         DC    CL4' '
CDSGCNTR DC    F'0'                     CDSG instructions attempted
CSG_CNTL DC    CL8'CSG_CNTR'            CSG  counter label
         DC    CL4' '
CSG_CNTR DC    F'0'                     CSG  instructions attempted
                                                                SPACE
CNTRMAX  DC    F'60000'                 Counter Overrun Maximum
LOOPMAX  DC    H'15000'                 Maximum number of loops
LOOPMAX2 DC    H'30000'                 Maximum number of loops * 2
                                                                SPACE
PACKED   DC    PL8'0'                   Packed decimal work area
EDIT     DC    XL6'402020202020'        EDIT mask with leading blank
STOPFLAG DC    X'00'                    Set to non-zero to stop test
                                                                SPACE
         CNOP  0,16                     Beautify trace table looks
TRACE    DC    65535F'0'                Trace area for debugging
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
