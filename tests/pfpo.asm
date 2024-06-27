 TITLE 'Quick PFPO DFP to HFP Conversion Test (GitHub Issue #407)'
***********************************************************************
*                           PFPO.ASM
***********************************************************************
*
*   This test converts the number 6.283185307179586476925286766559004
*   from Extended DFP (Decimal Floating-Point) format to a Long HFP
*   (Hexadecimal Floating-Point) format in order to confirm that the
*   bug described in GitHub Issue #407 has indeed been fixed. It does
*   not do anything else. It does NOT test any other function of the
*   PFPO instruction.
*
*   Note that the accompanying .tst runtest script tests two different
*   conversion scenarios: the first confirms that the original bug has
*   been fixed, and the second test confirms conversions of a shorter
*   length values also still works (i.e. that our fixed hasn't broken
*   anything).
*
***********************************************************************
                                                                SPACE 5
***********************************************************************
*                       Low Core PSWs...
***********************************************************************
                                                                SPACE 2
PFPO     START 0
         USING PFPO,0                 Use absolute addressing
                                                                SPACE 3
         ORG   PFPO+X'1A0'            z/Arch Restart new PSW
         DC    XL4'00000001'          z/Arch Restart new PSW
         DC    XL4'80000000'          z/Arch Restart new PSW
         DC    XL4'00000000'          z/Arch Restart new PSW
         DC    A(BEGIN)               z/Arch Restart new PSW
                                                                SPACE 2
         ORG   PFPO+X'1D0'            z/Arch Program new PSW
         DC    XL4'00020001'          z/Arch Program new PSW
         DC    XL4'80000000'          z/Arch Program new PSW
         DC    XL4'00000000'          z/Arch Program new PSW
         DC    XL4'0000DEAD'          z/Arch Program new PSW
                                                                EJECT
***********************************************************************
*                           BEGIN
***********************************************************************
                                                                SPACE
         ORG   PFPO+X'200'            Test code entry point
BEGIN    DS    0H
                                                                SPACE
         LCTLG CR0,CR0,CTL0   Enable AFP-register-control bit
         EFPC  R0             R0 <== FPC
         ST    R0,SAVEDFPC    Save FPC
                                                                SPACE
*   Load the test values.............
                                                                SPACE
         LG    R4,DFPIN_F4    R4 = first 64-bits
         LG    R6,DFPIN_F6    R6 = second 64-bits
                                                                SPACE
         LDGR  FR4,R4         Move to floating point register
         LDGR  FR6,R6         Move to floating point register
                                                                SPACE
*   Do the test............ (i.e. perform the conversion)
                                                                SPACE
         LG    R0,PFPO_R0     Extended DFP ==> Long HFP
         IILF  R1,X'ABCDABCD' Unlikely Return Code Register value
                                                                SPACE
         LA    R15,3          (set CC3...)
         SLL   R15,32-4       (shift into proper position)
         SPM   R15            (set Condition Code 3 in PSW)
                                                                SPACE
         PFPO  ,              Do it!
         JC    B'0001',BADCC  CC=3?!  Impossible!!  FAIL!!
                                                                SPACE
         LTR   R1,R1          Check Return Code Register value
         BNZ   BADGR1         Not zero? FAIL!
                                                                SPACE
*   Save the results..........
                                                                SPACE
         LGDR  R0,FR0         Save actual results (R0 <== FR0)
         STG   R0,HFPOUT      Save actual results (R0 --> save)
                                                                SPACE
*   Check the results..........
                                                                SPACE
         LG    R1,HFPOUTOK    R1 <== Expected
         CGR   R0,R1          Actual = Expected?
         BNE   FAIL           No?! FAIL!
                                                                EJECT
***********************************************************************
*      Try invalid conversion with Test bit set  (should get cc=3)
***********************************************************************
                                                                SPACE
         IILF  R0,X'81090900' Test: Long DFP ==> Long DFP (invalid!)
         IILF  R1,X'ABCDABCD' Unlikely Return Code Register value
                                                                SPACE
         SLR   R15,R15        (set CC0...)
         SLL   R15,32-4       (shift into proper position)
         SPM   R15            (set Condition Code 0 in PSW)
                                                                SPACE
         PFPO  ,              Do it!
         JC    B'1110',BADCC  Not CC=3? FAIL!
                                                                SPACE
         LTR   R1,R1          Check Return Code Register value
         BNZ   BADGR1         Not zero? FAIL!
                                                                SPACE
         LPSWE GOODPSW        Load success PSW
FAIL     LPSWE FAILPSW        Load failure PSW
BADCC    LPSWE BADCCPSW       Load failure PSW
BADGR1   LPSWE BADRCPSW       Load failure PSW
                                                                EJECT
*********************************************************************
*                      Working storage
*********************************************************************
                                                                SPACE
         CNOP  0,16   (alignment solely for storage readability)
                                                                SPACE 2
CTL0     DC    0D'0',XL8'0000000000040000' CR0 AFP-register-control bit
PFPO_R0  DC    XL4'00000000',XL4'01010A00'
SAVEDFPC DC    F'0',F'0',D'0'
                                                                SPACE 2
         ORG   PFPO+X'600'      INPUT @ X'600'
                                                                SPACE
DFPIN_F4 DC    0D'0',XL8'39FFD2B32D873E6E'    (original input)
DFPIN_F6 DC    0D'0',XL8'A9DAAD5ABE6B6404'    (original input)
                                                                SPACE 2
         ORG   PFPO+X'700'      EXPECTED OUTPUT @ X'700'
                                                                SPACE
HFPOUTOK DC    0D'0',XL8'416487ED5110B461'  (expected output)
         DC    D'0'
                                                                SPACE 2
         ORG   PFPO+X'710'      ACTUAL OUTPUT @ X'710'
                                                                SPACE
HFPOUT   DC    0D'0',XL8'00'    (actual output)
         DC    D'0'
                                                                SPACE 2
GOODPSW  DC    0D'0'          Failure PSW
         DC    XL4'00020001'  Failure PSW
         DC    XL4'80000000'  Failure PSW
         DC    XL4'00000000'  Failure PSW
         DC    XL4'00000000'  Failure PSW
                                                                SPACE
FAILPSW  DC    0D'0'          Failure PSW
         DC    XL4'00020001'  Failure PSW
         DC    XL4'80000000'  Failure PSW
         DC    XL4'00000000'  Failure PSW
         DC    XL4'00000BAD'  Failure PSW  (general test failure)
                                                                SPACE
BADRCPSW DC    0D'0'          Failure PSW
         DC    XL4'00020001'  Failure PSW
         DC    XL4'80000000'  Failure PSW
         DC    XL4'00000000'  Failure PSW
         DC    XL4'0000BAD1'  Failure PSW  (bad GR1 Return Code value)
                                                                SPACE
BADCCPSW DC    0D'0'          Failure PSW
         DC    XL4'00020001'  Failure PSW
         DC    XL4'80000000'  Failure PSW
         DC    XL4'00000000'  Failure PSW
         DC    XL4'000BADCC'  Failure PSW  (bad Condition Code)
                                                                EJECT
R0       EQU   0        General Purpose Registers...
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

FR0      EQU   0        Floating-Point Registers...
FR1      EQU   1
FR2      EQU   2
FR3      EQU   3
FR4      EQU   4
FR5      EQU   5
FR6      EQU   6
FR7      EQU   7
FR8      EQU   8
FR9      EQU   9
FR10     EQU   10
FR11     EQU   11
FR12     EQU   12
FR13     EQU   13
FR14     EQU   14
FR15     EQU   15

CR0      EQU   0        Control Registers...
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
                                                                SPACE
         END
