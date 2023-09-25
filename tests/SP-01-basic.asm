 TITLE '            SP-01-basic (Test SP instruction)'
***********************************************************************
*
*            SP instruction tests
*
***********************************************************************
***********************************************************************
*
*  This program ONLY tests base cases of the AP instruction.
*
***********************************************************************
*  NOTE: When assembling using SATK, use the "-t S390" option.
***********************************************************************
*
*  Example Hercules Testcase:
*
*        *Testcase SP-01-basic (Test AP instructions)
*        mainsize    1
*        numcpu      1
*        sysclear
*        archlvl     z/Arch
*        *Program    7
*        loadcore    "$(testpath)/AP-01-basic.core"
*        runtest     1
*        *Done
*
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
*        Initiate the SP01 CSECT in the CODE region
*        with the location counter at 0
***********************************************************************
                                                                SPACE
SP01     ASALOAD  REGION=CODE
                                                                SPACE 3
***********************************************************************
*
***********************************************************************
                                                                SPACE
         ORG    SP01+X'28'
PGMOLD   EQU    *
                                                                SPACE
         ORG    SP01+X'68'
         DC     XL4'00080000'
         DC     A(PGMCHK)
                                                                SPACE
         ORG    SP01+X'8E'
PCINTCD  DS     H
         ORG    SP01+X'8C'
PGMCODE  DC     F'0'
                                                                SPACE
OLDPSW   EQU    SP01+X'150'
***********************************************************************
*        Create IPL (restart) PSW
***********************************************************************
                                                                SPACE
         ASAIPL   IA=BEGIN
                                                                EJECT
***********************************************************************
*               The actual "SP01" program itself...
***********************************************************************
*
*  Architecture Mode: 390
*  Addressing Mode:   31-bit
*  Register Usage:
*
*   R0       (work)
*   R1       I/O device used by ENADEV and RAWIO macros
*   R2       First base register
*   R3       IOCB pointer for ENADEV and RAWIO macros
*   R4       IO work register used by ENADEV and RAWIO
*   R5-R7    (work)
*   R8       ORB pointer
*   R9       Second base register
*   R10-R13  (work)
*   R14      Subroutine call
*   R15      Secondary Subroutine call or work
*
***********************************************************************
                                                                SPACE
         USING  ASA,R0          Low core addressability
         USING  BEGIN,R2        FIRST Base Register
         USING  BEGIN+4096,R9   SECOND Base Register
         USING  SP01,R0
                                                                SPACE
BEGIN    BALR  R2,0             Initalize FIRST base register
         BCTR  R2,0             Initalize FIRST base register
         BCTR  R2,0             Initalize FIRST base register
                                                                SPACE
         LA    R9,2048(,R2)     Initalize SECOND base register
         LA    R9,2048(,R9)     Initalize SECOND base register
                                                                SPACE
                                                                EJECT
***********************************************************************
*        Run the test...
***********************************************************************
                                                                SPACE
         BAL    R14,TESTSP01
                                                                SPACE
         L      R1,=FD'0'
         CL     R1,FLERR
         BNE    FAILTEST
                                                                SPACE
         B      EOJ              Yes, then normal completion!
                                                                EJECT
***********************************************************************
*        TESTAS01
***********************************************************************
                                                                SPACE
PGMCHK   ST     R1,SAVER1
                                                                SPACE
         L      R1,PGMCODE
         N      R1,=XL4'0000FFFF'
         ST     R1,SAVEPGN
                                                                SPACE
         CL     R1,EXPINT
         BNE    EXIT
         L      R1,=XL4'0'
         ST     R1,EXPINT
                                                                SPACE
EXIT     L      R1,SAVER1
         LPSW   PGMOLD
                                                                EJECT
***********************************************************************
*        TESTSP01
***********************************************************************
                                                                SPACE
TESTSP01 SP     OPL1,OPR1
         CP     RES1,OPL1
         BE     STEP02
                                                                SPACE
         L      R1,FLERR
         O      R1,MASK1
         ST     R1,FLERR
                                                                SPACE
STEP02   SP     OPL2,OPR2
         CP     RES2,OPL2
         BE     STEP03
                                                                SPACE
         L      R1,FLERR
         O      R1,MASK2
         ST     R1,FLERR
                                                                SPACE
STEP03   SP     OPL3,OPR3
         CP     RES3,OPL3
         BE     STEP04
                                                                SPACE
         L      R1,FLERR
         O      R1,MASK3
         ST     R1,FLERR
                                                                SPACE
STEP04   SP     OPL4,OPR4
         CP     RES4,OPL4
         BE     STEP05
                                                                SPACE
         L      R1,FLERR
         O      R1,MASK4
         ST     R1,FLERR
                                                                SPACE
STEP05   SP     OPL5,OPR5
         CP     RES5,OPL5
         BE     STEP06
                                                                SPACE
         L      R1,FLERR
         O      R1,MASK5
         ST     R1,FLERR
                                                                SPACE
STEP06   L      R1,=X'A'
         ST     R1,OPL6
         L      R1,=XL4'07'
         ST     R1,EXPINT
                                                                SPACE
         SP     OPL6,OPR6
                                                                SPACE
         L      R1,=FD'0'
         CL     R1,EXPINT
         BE     TESTEND
                                                                SPACE
         L      R1,FLERR
         O      R1,MASK6
         ST     R1,FLERR
                                                                SPACE
TESTEND  BR     R14
                                                                EJECT
***********************************************************************
*        Normal completion or Abnormal termination PSWs
***********************************************************************
                                                                SPACE 5
EOJ      DWAITEND LOAD=YES             Normal completion
                                                                SPACE 5
FAILTEST DWAIT LOAD=YES,CODE=BAD       Abnormal termination
                                                                EJECT
***********************************************************************
*
***********************************************************************
                                                                SPACE 2
         LTORG ,
SAVER1   DC    F'0'
SAVEPGN  DC    F'0'
EXPINT   DC    F'0'
FLERR    DC    F'0'
OPL1     DC    P'0000000000000000000000000000000'
OPR1     DC    P'0000000000000000000000000000000'
RES1     DC    X'0000000000000000000000000000000C'
MASK1    DC    X'00000001'
                                                                SPACE
OPL2     DC    P'1111111111111111111111111111111'
OPR2     DC    P'0000000000000000000000000000000'
RES2     DC    P'1111111111111111111111111111111'
MASK2    DC    X'00000010'
                                                                SPACE
OPL3     DC    P'1111111111111111111111111111111'
OPR3     DC    P'1111111111111111111111111111111'
RES3     DC    P'0000000000000000000000000000000'
MASK3    DC    X'00000100'
                                                                SPACE
OPL4     DC    P'0000000000000100000000000000010'
OPR4     DC    P'0000000000000090000000000000009'
RES4     DC    P'0000000000000010000000000000001'
MASK4    DC    X'00001000'
                                                                SPACE
OPL5     DC    P'1000000000000000000000000000000'
OPR5     DC    P'0000000000000000000000000000001'
RES5     DC    X'0999999999999999999999999999999C'
MASK5    DC    X'00010000'
                                                                SPACE
OPL6     DC    P'0000000000000000000000000000000'
OPR6     DC    P'0000000000000000000000000000000'
RES6     DC    P'0000000000000000000000000000000'
MASK6    DC    X'00100000'
                                                                EJECT
***********************************************************************
*        IOCB DSECT
***********************************************************************
                                                                SPACE
         DSECTS NAME=IOCB
                                                                EJECT
***********************************************************************
*        ORB DSECT
***********************************************************************
                                                                SPACE
         DSECTS NAME=ORB
                                                                EJECT
***********************************************************************
*        IRB DSECT
***********************************************************************
                                                                SPACE
         DSECTS NAME=IRB
                                                                EJECT
***********************************************************************
*        SCSW DSECT
***********************************************************************
                                                                SPACE
         DSECTS NAME=SCSW
                                                                EJECT
***********************************************************************
*        (other DSECTS needed by SATK)
***********************************************************************
                                                                SPACE
         DSECTS PRINT=OFF,NAME=(ASA,SCHIB,CCW0,CCW1,CSW)
         PRINT ON
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
                                                                SPACE 4
         END
