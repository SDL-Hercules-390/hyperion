 TITLE '     ShiftLeft   --   Test Algebraic "Shift Left" Instructions'
***********************************************************************
*
*                           ShiftLeft
*
***********************************************************************
*
*  This program tests the algebraic "Shift Left" instructions
*
*                   SLA, SLDA, SLAK, SLAG
*
*  to ensure proper results and setting of Condition Code.
*
*  The original implementation of these instructions in Hercules was
*  determined to be relatively inefficient, so efforts were made to
*  try and speed them up. This test verifies that the instructions
*  still produce correct results.
*
***********************************************************************
                                                                EJECT
***********************************************************************
*                           LOW CORE
***********************************************************************
                                                                SPACE
SHIFTEST START 0
                                                                SPACE
         USING *,R0               Use absolute addressing
                                                                SPACE 4
         ORG   SHIFTEST+X'1A0'    z/Arch Restart new PSW
                                                                SPACE
         DC    XL4'00000001'
         DC    XL4'80000000'
         DC    XL4'00000000'
         DC    A(BEGIN)
                                                                SPACE 3
         ORG   SHIFTEST+X'1D0'    z/Arch Program new PSW
                                                                SPACE
         DC    XL4'00020001'
         DC    XL4'80000000'
         DC    XL4'00000000'
         DC    A(X'DEAD')
                                                                EJECT
***********************************************************************
*                      Main Program
***********************************************************************
                                                                SPACE
BEGIN    DS    0H
                                                                SPACE
         BAL   R14,SLA            Test Shift Left Single
         BAL   R14,SLDA           Test Shift Left Double
         BAL   R14,SLAK           Test Shift Left Single Distinct
         BAL   R14,SLAG           Test Shift Left Single Long
                                                                SPACE
         LPSWE GOODPSW            Success! All tests passed!
                                                                SPACE 4
FAILTEST SH    R13,=H'4'          Backup to actual failure location
         LPSWE FAILPSW            Abnormal termination disabled wait
                                                                SPACE 6
GOODPSW  DC    0D'0'              Test SUCCESS disabled wait PSW
         DC    XL4'00020001'
         DC    XL4'80000000'
         DC    AD(0)
                                                                SPACE 4
FAILPSW  DC    0D'0'              Test FAILURE disabled wait PSW
         DC    XL4'00020001'
         DC    XL4'80000000'
         DC    AD(X'BAD')
                                                                EJECT
***********************************************************************
*  8B   SLA   - Shift Left Single                             [RS-a]
***********************************************************************
*
*    SHIFT LEFT SINGLE (SLA)
*
*    The SHIFT LEFT SINGLE instruction is similar to
*    SHIFT LEFT DOUBLE, except that it shifts only the
*    31 numeric bits of a single register. Therefore, this
*    instruction performs an algebraic left shift of a 32-bit
*    signed binary integer.
*
*    For example, if the contents of register 2 are:
*
*    00 7F 0A 72 = 00000000 01111111 00001010 01110010
*
*    The instruction:
*
*    Machine Format
*
*      0           1           2           3           4
*      +-----+-----+-----+-----+-----+-----+-----+-----+
*      |     8B    |  2  | /// |  0  |      008        |    RS-a
*      +-----+-----+-----+-----+-----+-----+-----+-----+
*
*    Assembler Format
*
*      Op Code  R1,D2(B2)
*      ------------------
*        SLA     2,8(0)
*      
*    results in register 2 being shifted left eight bit
*    positions so that its new contents are:
*
*    7F 0A 72 00 =
*
*      01111111 00001010 01110010 00000000
*
*    Condition code 2 is set to indicate that the result is
*    greater than zero.
*
*    If a left shift of nine places had been specified, a
*    significant bit would have been shifted out of bit
*    position 1. Condition code 3 would have been set to
*    indicate this overflow and, if the fixed-point-overflow
*    mask bit in the PSW were one, a fixed-point overflow
*    interruption would have occurred.
*
***********************************************************************
                                                                EJECT
         USING TTAB32,R1
                                                                SPACE
SLA      L     R1,=A(TST32TAB)      R1 --> test table
                                                                SPACE
SLA1     LM    R2,R5,0(R1)          Load parameters
         IC    R4,BCMASKS(R4)       Get BC instruction mask
                                                                SPACE
         SLA   R2,0(R3)             Do the shift
         EX    R4,SLACC             Expected CC?
         BAL   R13,FAILTEST         Unexpected CC! FAIL!
                                                                SPACE
SLA2     CLR   R2,R5                Expected results?
         BE    SLA3                 Yes, continue
         BAL   R13,FAILTEST         No! Unexpected results! FAIL!
                                                                SPACE
SLA3     LA    R1,TT32NEXT          Next test table entry
         CLC   =CL4'END!',0(R1)     End of test table?
         BNE   SLA1                 No, loop...
         BR    R14                  Yes, return to caller
                                                                SPACE
SLACC    BC    0,SLA2               Expected condition code?
                                                                SPACE
         DROP  R1
                                                                EJECT
***********************************************************************
*  8F   SLDA  - Shift Left Double                             [RS-a]
***********************************************************************
*
*    SHIFT LEFT DOUBLE (SLDA)
*
*    The SHIFT LEFT DOUBLE instruction shifts the 63
*    numeric bits of an even-odd register pair to the left,
*    leaving the sign bit unchanged. Thus, the instruction
*    performs an algebraic left shift of a 64-bit signed
*    binary integer.
*
*    For example, if the contents of registers 2 and 3 are:
*
*    00 7F 0A 72   FE DC BA 98 =
*
*      00000000 01111111 00001010 01110010
*      11111110 11011100 10111010 10011000
*
*
*    The instruction:
*
*    Machine Format
*
*      0           1           2           3           4
*      +-----+-----+-----+-----+-----+-----+-----+-----+
*      |     8F    |  2  | /// |  0  |      01F        |    RS-a
*      +-----+-----+-----+-----+-----+-----+-----+-----+
*
*    Assembler Format
*
*      Op Code  R1,D2(B2)
*      ------------------
*        SLDA    2,31(0)
*
*    results in registers 2 and 3 both being left-shifted 31
*    bit positions, so that their new contents are:
*
*    7F 6E 5D 4C   00 00 00 00 =
*
*      01111111 01101110 01011101 01001100
*      00000000 00000000 00000000 00000000
*
*    Because significant bits are shifted out of bit position
*    1 of register 2, overflow is indicated by setting
*    condition code 3, and, if the fixed-point-overflow mask bit
*    in the PSW is one, a fixed-point-overflow program
*    interruption occurs.
*
***********************************************************************
                                                                EJECT
         USING TTAB64,R1
                                                                SPACE
SLDA     L     R1,=A(TST64TAB)      R1 --> test table
                                                                SPACE
SLDA1    LM    R2,R7,0(R1)          Load parameters
         IC    R5,BCMASKS(R5)       Get BC instruction mask
                                                                SPACE
         SLDA  R2,0(R4)             Do the shift
         EX    R5,SLDACC            Expected CC?
         BAL   R13,FAILTEST         Unexpected CC! FAIL!
                                                                SPACE
SLDA2    CLR   R2,R6                Expected results?
         BE    SLDA3                Yes, continue
         BAL   R13,FAILTEST         No! Unexpected results! FAIL!
                                                                SPACE
SLDA3    CLR   R3,R7                Expected results?
         BE    SLDA4                Yes, continue
         BAL   R13,FAILTEST         No! Unexpected results! FAIL!

SLDA4    LA    R1,TT64NEXT          Next test table entry
         CLC   =CL4'END!',0(R1)     End of test table?
         BNE   SLDA1                No, loop...
         BR    R14                  Yes, return to caller
                                                                SPACE
SLDACC   BC    0,SLDA2              Expected condition code?
                                                                SPACE
         DROP  R1
                                                                EJECT
***********************************************************************
*  EBDD SLAK  - Shift Left Single Distinct                   [RSY-a]
***********************************************************************
*
*    SHIFT LEFT SINGLE DISTINCT (SLAK)
*
*
*      Op Code  R1,R3,D2(B2)
*      ------------------
*        SLAK    2,3,8(0)
*
*
*    This instruction is basically identical to SLA except that
*    the value TO BE shifted is held in R3 and remains unchanged,
*    with the results of the 31-bit shift being placed into R1.
*
***********************************************************************
                                                                SPACE 3
         USING TTAB32,R1
                                                                SPACE
SLAK     L     R1,=A(TST32TAB)      R1 --> test table
                                                                SPACE
SLAK1    LM    R2,R5,0(R1)          Load parameters
         LR    R6,R2                Load beginning value
         SLR   R2,R2                Clear target register
         IC    R4,BCMASKS(R4)       Get BC instruction mask
                                                                SPACE
         SLAK  R2,R6,0(R3)          Do the shift
         EX    R4,SLAKCC            Expected CC?
         BAL   R13,FAILTEST         NOT CC2! FAIL!
                                                                SPACE
SLAK2    CLR   R2,R5                Expected results?
         BE    SLAK3                Yes, continue
         BAL   R13,FAILTEST         No! Unexpected results! FAIL!
                                                                SPACE
SLAK3    CL    R6,BEGVAL32          Input register unchanged?
         BE    SLAK4                Yes, continue
         BAL   R13,FAILTEST         No! Unexpected results! FAIL!
                                                                SPACE
SLAK4    LA    R1,TT32NEXT          Next test table entry
         CLC   =CL4'END!',0(R1)     End of test table?
         BNE   SLAK1                No, loop...
         BR    R14                  Yes, return to caller
                                                                SPACE
SLAKCC   BC    0,SLAK2              Expected condition code?
                                                                SPACE
         DROP  R1
                                                                EJECT
***********************************************************************
*  EB0B SLAG  - Shift Left Single Long                       [RSY-a]
***********************************************************************
*
*    SHIFT LEFT SINGLE LONG (SLAG)
*
*
*    Assembler Format
*
*      Op Code  R1,R3,D2(B2)
*      ------------------
*        SLAG    2,3,31(0)
*
*
*    This instruction is identical to SLAK except that the shift is a
*    63-bit shift instead of a 31-bit shift.
*
***********************************************************************
                                                                SPACE 2
         USING TTAB64,R1
                                                                SPACE
SLAG     L     R1,=A(TST64TAB)      R1 --> test table
                                                                SPACE
SLAG1    SLGR  R2,R2                Clear target register
         LG    R3,BEGVAL64          Load beginning value
         L     R4,SHIFT64           Get shift amount
         L     R5,CC64              Get expected CC
         IC    R5,BCMASKS(R5)       Get BC instruction mask
         LG    R6,ENDVAL64          Load expected ending value
                                                                SPACE
         SLAG  R2,R3,0(R4)          Do the shift
         EX    R5,SLAGCC            Expected CC?
         BAL   R13,FAILTEST         Unexpected CC! FAIL!
                                                                SPACE
SLAG2    CLGR  R2,R6                Expected results?
         BE    SLAG3                Yes, continue
         BAL   R13,FAILTEST         No! Unexpected results! FAIL!
                                                                SPACE
SLAG3    CLG   R3,BEGVAL64          Input register unchanged?
         BE    SLAG4                Yes, continue
         BAL   R13,FAILTEST         No! Unexpected results! FAIL!
                                                                SPACE
SLAG4    LA    R1,TT64NEXT          Next test table entry
         CLC   =CL4'END!',0(R1)     End of test table?
         BNE   SLAG1                No, loop...
         BR    R14                  Yes, return to caller
                                                                SPACE
SLAGCC   BC    0,SLAG2              Expected condition code?
                                                                SPACE
         DROP  R1
                                                                EJECT
***********************************************************************
*                      Working Storage
***********************************************************************
                                                                SPACE
BCMASKS  DC    X'80',X'40',X'20',X'10'    CC 0, 1, 2, 3
                                                                SPACE
         LTORG ,        Literals Pool
                                                                SPACE
TST32TAB DC    0D'0'
**********************************************
*   mixed significant bits positive OVERFLOW
*                             shift CC
         DC    A(X'22000000'),A(8),A(3)
         DC    A(X'00000000')
*
**********************************************
*   mixed significant bits negative OVERFLOW
*                             shift CC
         DC    A(X'A2000000'),A(8),A(3)
         DC    A(X'80000000')
*
**********************************************
*        old way slowest possible positive
*                             shift CC
         DC    A(X'00000001'),A(30),A(2)
         DC    A(X'40000000')
*
**********************************************
*        old way slowest possible negative
*                             shift CC
         DC    A(X'FFFFFFFF'),A(31),A(1)
         DC    A(X'80000000')
*
**********************************************
*        positive, 0 bits
*                             shift CC
         DC    A(X'00000123'),A(0),A(2)
         DC    A(X'00000123')
*
**********************************************
*        negative, 0 bits
*                             shift CC
         DC    A(X'80000123'),A(0),A(1)
         DC    A(X'80000123')
*
**********************************************
*        max positive, 1 bit
*                             shift CC
         DC    A(X'7FFFFFFF'),A(1),A(3)
         DC    A(X'7FFFFFFE')
*
                                                                EJECT
**********************************************
*        max negative, 1 bit
*                             shift CC
         DC    A(X'80000000'),A(1),A(3)
         DC    A(X'80000000')
*
**********************************************
*        positive, 1 bit
*                             shift CC
         DC    A(X'22222222'),A(1),A(2)
         DC    A(X'44444444')
*
**********************************************
*        negative, 1 bit
*                             shift CC
         DC    A(X'CAAAAAAA'),A(1),A(1)
         DC    A(X'95555554')
*
**********************************************
*        positive, 1 bit, OVERFLOW
*                             shift CC
         DC    A(X'77777777'),A(1),A(3)
         DC    A(X'6EEEEEEE')
*
**********************************************
*        negative, 1 bit, OVERFLOW
*                             shift CC
         DC    A(X'88888888'),A(1),A(3)
         DC    A(X'91111110')
*
**********************************************
*        (original POPS test 1)
*                             shift CC
         DC    A(X'007F0A72'),A(8),A(2)
         DC    A(X'7F0A7200')
*
**********************************************
*        (original POPS test 2)
*                             shift CC
         DC    A(X'007F0A72'),A(9),A(3)
         DC    A(X'7E14E400')
*
**********************************************
         DC    CL4'END!'
                                                                EJECT
TST64TAB DC    0D'0'
**********************************************
*   mixed significant bits positive OVERFLOW
*                                            shift CC
         DC    A(X'22000000'),A(X'00000000'),A(8),A(3)
         DC    A(X'00000000'),A(X'00000000')
*
**********************************************
*   mixed significant bits negative OVERFLOW
*                                            shift CC
         DC    A(X'A2000000'),A(X'00000000'),A(8),A(3)
         DC    A(X'80000000'),A(X'00000000')
*
**********************************************
*        old way slowest possible positive
*                                            shift CC
         DC    A(X'00000000'),A(X'00000001'),A(62),A(2)
         DC    A(X'40000000'),A(X'00000000')
*
**********************************************
*        old way slowest possible negative
*                                            shift CC
         DC    A(X'FFFFFFFF'),A(X'FFFFFFFF'),A(63),A(1)
         DC    A(X'80000000'),A(X'00000000')
*
**********************************************
*        positive, 0 bits
*                                            shift CC
         DC    A(X'00000000'),A(X'00000123'),A(0),A(2)
         DC    A(X'00000000'),A(X'00000123')
*
**********************************************
*        negative, 0 bits
*                                            shift CC
         DC    A(X'80000000'),A(X'00000123'),A(0),A(1)
         DC    A(X'80000000'),A(X'00000123')
*
**********************************************
*        max positive, 1 bit
*                                            shift CC
         DC    A(X'7FFFFFFF'),A(X'FFFFFFFF'),A(1),A(3)
         DC    A(X'7FFFFFFF'),A(X'FFFFFFFE')
*
**********************************************
*        max negative, 1 bit
*                                            shift CC
         DC    A(X'80000000'),A(X'00000000'),A(1),A(3)
         DC    A(X'80000000'),A(X'00000000')
*
                                                                EJECT
**********************************************
*        positive, 1 bit
*                                            shift CC
         DC    A(X'22222222'),A(X'22222222'),A(1),A(2)
         DC    A(X'44444444'),A(X'44444444')
*
**********************************************
*        negative, 1 bit
*                                            shift CC
         DC    A(X'CAAAAAAA'),A(X'AAAAAAAA'),A(1),A(1)
         DC    A(X'95555555'),A(X'55555554')
*
**********************************************
*        positive, 1 bit, OVERFLOW
*                                            shift CC
         DC    A(X'77777777'),A(X'77777777'),A(1),A(3)
         DC    A(X'6EEEEEEE'),A(X'EEEEEEEE')
*
**********************************************
*        negative, 1 bit, OVERFLOW
*                                            shift CC
         DC    A(X'88888888'),A(X'88888888'),A(1),A(3)
         DC    A(X'91111111'),A(X'11111110')
*
**********************************************
*        (original POPS test 1)
*                                            shift CC
         DC    A(X'007F0A72'),A(X'FEDCBA98'),A(31),A(3)
         DC    A(X'7F6E5D4C'),A(X'00000000')
*
**********************************************
*        (original POPS test 2)
*                                            shift CC
         DC    A(X'007F0A72'),A(X'FEDCBA98'),A(8),A(2)
         DC    A(X'7F0A72FE'),A(X'DCBA9800')
*
**********************************************
         DC    CL4'END!'
                                                                EJECT
***********************************************************************
*                      Test tables DSECTs
***********************************************************************
                                                                SPACE 2
TTAB32   DSECT
BEGVAL32 DS    A            Starting value
SHIFT32  DS    A            shift amount (#of bits to shift)
CC32     DS    A            Expected condition code
ENDVAL32 DS    A            Expected ending value
TT32NEXT EQU   *
                                                                SPACE 3
TTAB64   DSECT
BEGVAL64 DS    A            Starting value (hi 32)
         DS    A            Starting value (lo 32)
SHIFT64  DS    A            shift amount (#of bits to shift)
CC64     DS    A            Expected condition code
ENDVAL64 DS    A            Expected ending value (hi 32)
         DS    A            Expected ending value (lo 32)
TT64NEXT EQU   *
                                                                SPACE 4
***********************************************************************
*                      Register Equates
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
