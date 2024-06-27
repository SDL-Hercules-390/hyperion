 TITLE '       CLCLE-01-unaligned-buffers  (Test CLCLE instructions)'
***********************************************************************
*
*                     CLCLE Unaligned Buffers Test
*
*        NOTE: This is a copy of the CLCL Unaligned Buffers Test
*              modified to test the CLCLE instruction.
*              James Wekel August 2022
***********************************************************************
*
*  This program tests proper functioning of the CLCLE instruction's
*  optimization logic (specifically, the "mem_cmp" function that the
*  CLCLE instruction makes use of) to ensure the location of the in-
*  equality is properly reported.
*
*  Depending on the alignment of the two operands being compared, if
*  the length of the compare is large enough that it would cause the
*  comparison to cross a page boundary for both operands and the in-
*  equality occurs at an offset past the distance each operand is
*  from its respective page boundary added together, then the address
*  of the inequality that CLCLEreturns would be off by the shorter
*  of the two distances.
*
*  For example, if the operand addresses were X'123456' and X'456789'
*  (and the page size was X'800') and the inequality was at (or past)
*  X'123877', then CLCLE would incorrectly report the address of the
*  inequality as being at address X'123877' - X'77' = X'123800':
*
*  X'123456' is X'3AA' bytes from the end of its page boundary.
*  X'456789' is X'77'  bytes from the end of its page boundary.
*  The true inequality is at X'123877' (X'123456' + X'77' + X'3AA').
*
*  The optimization logic would perform three separate compares: the
*  first starting at X'123456' for a length of X'77'. The second one
*  at address X'1234CD' (X'123456' + X'77') for a length of X'3AA',
*  and the third and final compare at address X'123877' (X'123456' +
*  X'77' + X'3AA') for a length of at least one byte.
*
*  Due to a bug in the original optimization logic however the length
*  of the first compare would not be added to the calculated offset of
*  where the inequality was located at.  That is to say, the offset of
*  the inequality would be calculated as operand-1 + X'3AA' instead of
*  operand-1 + X'77' + X'3AA'.  The X'77' offset would get erroneously
*  lost, thereby causing the location of the inequality to be reported
*  X'77' bytes BEFORE where the actual inequality was actually located
*  at! (Oops!)
*
*  The bug has since been fixed of course but to ensure such does not
*  occur again, this standalone runtest test performs a series of CLCL
*  comparisons whose parameters are such that they end up tripping the
*  bug as described. Thank you to Dave Kreiss for reporting the bug.
*
***********************************************************************
                                                                EJECT
***********************************************************************
*
*                  EXAMPLE RUNTEST TEST CASE
*
*        *Testcase CLCLE-01-unaligned-buffers Test
*
*        archlvl     390
*        mainsize    3
*        numcpu      1
*        sysclear
*
*        loadcore    "$(testpath)/CLCLE-01-unaligned-buffers.core"
*
*        runtest     0.1
*
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
*        Initiate the CLCLE CSECT in the CODE region
*        with the location counter at 0
***********************************************************************
                                                                SPACE
CLCLE    ASALOAD  REGION=CODE
                                                                SPACE
***********************************************************************
*        Create IPL (restart) PSW
***********************************************************************
                                                                SPACE
         ASAIPL   IA=BEGIN
                                                                SPACE 2
***********************************************************************
*               The actual "CLCLE" program itself...
***********************************************************************
*
*  Architecture Mode:   ESA/390
*
*  Addressing Mode:     31-bit
*
*  Register Usage:      R12 - R13     Base registers
*                       R0  - R1      CLCLE Operand-1
*                       R14 - R15     CLCLE Operand-2
*                       R2  - R11     Work registers
*
***********************************************************************
                                                                SPACE
         USING  BEGIN,R12       FIRST Base Register
         USING  BEGIN+4096,R13  SECOND Base Register
                                                                SPACE
BEGIN    BALR  R12,0            Initalize FIRST base register
         BCTR  R12,0            Initalize FIRST base register
         BCTR  R12,0            Initalize FIRST base register
                                                                SPACE
         LA    R13,2048(,R12)   Initalize SECOND base register
         LA    R13,2048(,R13)   Initalize SECOND base register
                                                                EJECT
***********************************************************************
*        Compare DATA1 and DATA2 one BUFFSIZE at a time...
***********************************************************************
                                                                SPACE
*                         R4      R5    R6     R7      R8       R9
         LM    R4,R9,=A(BUFFER1,DATA1,BUFFER2,DATA2,BUFFSIZE,DATASIZE)
                                                                SPACE
         CLR   R9,R8            DATASIZE greater than BUFFSIZE?
         BNL   CHNKLOOP         Yes, get started...
         LR    R8,R9            No, only compare however much we have!
                                                                SPACE
*             Fill buffers with next chunk of data...
                                                                SPACE
CHNKLOOP LR    R0,R4            R0 --> BUFFER1
         LR    R2,R5            R2 --> DATA1
         LR    R1,R8            R1 <== BUFFSIZE
         LR    R3,R8            R3 <== BUFFSIZE
         MVCL  R0,R2            Copy into BUFFER1 <== next DATA1 chunk
                                                                SPACE
         LR    R0,R6            R0 --> BUFFER2
         LR    R2,R7            R2 --> DATA2
         LR    R1,R8            R1 <== BUFFSIZE
         LR    R3,R8            R3 <== BUFFSIZE
         MVCL  R0,R2            Copy into BUFFER2 <== next DATA2 chunk
                                                                SPACE
*                   Prepare for CLCLE...
                                                                SPACE
         LR    R0,R4            R0  --> BUFFER1
         LR    R14,R6           R14 --> BUFFER2
         LR    R1,R8            R1  <== BUFFSIZE
         LR    R15,R8           R15 <== BUFFSIZE
                                                                SPACE
*                 Compare the two buffers...
                                                                SPACE
*                          Compare BUFFER1 with BUFFER2..
CONTINUE CLCLE R0,R14,0    with padding x'00'
         BC    B'0001',CONTINUE      CC=3, not finished
         BE    NXTCHUNK         Equal: Buffer compare complete
                                                                SPACE
*           Inequality found: VERIFY ITS ACCURACY!
                                                                SPACE
         LR    R10,R0           R10 --> Supposed unequal byte
         CLC   0(1,R10),0(R14)  Valid inequality?
         BE    FAILURE          Bogus inequality!  CLCLE BUG!  FAIL!
                                                                SPACE
*           CLCLE was correct.  Get past inequality
*           and finish comparing the buffer data if
*           there is any data remaining in the buffer
*           that we haven't compared yet...
                                                                SPACE
         AH    R0,=H'1'         Get past unequal byte
         AH    R14,=H'1'        Get past unequal byte
         BCTR  R1,0             Get past unequal byte
         BCT   R15,CONTINUE     Go finish buffer if any bytes remain...
                                                                SPACE
*          Go on to next chunk of data  -- if there is one.
                                                                SPACE
NXTCHUNK ALR   R5,R8            R5 --> Next DATA1 chunk
         ALR   R7,R8            R7 --> Next DATA2 chunk
                                                                SPACE
         SR    R9,R8            Decrement DATA bytes remaining
         BZ    SUCCESS          None: We're done...
         BP    CHNKLOOP         Lots: Go compare next chunk...
         LPR   R8,R9            Some: Make R8 <== positive remaining
         B     CHNKLOOP         Go compare final chunk...
                                                                EJECT
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 3
SUCCESS  DWAITEND LOAD=YES          Normal completion
                                                                SPACE 3
FAILURE  DWAIT LOAD=YES,CODE=BAD    Abnormal termination
                                                                EJECT
***********************************************************************
*        Working Storage
***********************************************************************
*
*        The specific bug that was reported:
*
*        4DE   787FE B54   87F46 B54
*        4DF   787FF B53   87F47 B53
*
*        F32   79252 100   8899A 100    (BOGUS!)
*
*        FEA   7930A 048   88A52 048
*        FEB   7930B 047   88A53 047
*
***********************************************************************
                                                                SPACE
         LTORG ,                Literals pool
                                                                SPACE
BUFFSIZE EQU   (8*1024)
DATASIZE EQU   X'1032'
                                                                SPACE
BUFF1OFF EQU   X'320'
BUFF2OFF EQU   X'A68'
                                                                SPACE
         ORG   CLCLE+(1*(128*1024))+BUFF1OFF
BUFFER1  DC    (BUFFSIZE/8)XL8'00'
                                                                SPACE
         ORG   CLCLE+(2*(128*1024))+BUFF2OFF
BUFFER2  DC    (BUFFSIZE/8)XL8'00'
                                                                SPACE
         ORG   CLCLE+(3*(128*1024))    X'60000'
DATA1    DC    (DATASIZE)X'00'        X'60000'
                                                                SPACE
         ORG   CLCLE+(4*(128*1024))    X'80000'
DATA2    DC    (DATASIZE)X'00'        X'80000'
                                                                SPACE
         ORG   DATA2+X'04DE'
         DC    X'FF'
                                                                SPACE
         ORG   DATA2+X'04DF'
         DC    X'FF'
                                                                SPACE
         ORG   DATA2+X'0FEA'
         DC    X'FF'
                                                                SPACE
         ORG   DATA2+X'0FEB'
         DC    X'FF'
                                                                SPACE
         ORG   DATA2+DATASIZE
                                                                EJECT
***********************************************************************
*        Register equates
***********************************************************************
                                                                SPACE
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
         END
