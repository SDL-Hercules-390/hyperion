  TITLE 'dfp-080-to-packed.asm: Test Convert To Packed'
***********************************************************************
*
*Testcase: Convert To Packed (Long and Extended)
*  Test results, Program interrupt code, CC, and
*  DXC saved for all tests.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*                    dfp-080-to-packed.asm
*
*        This assembly-language source file is modelled on
*        Hercules Decimal Floating Point Validation Package
*        by Stephen R. Orso, specifically dfp-012-loadtest.asm.
*
*        Note: this has been tested ONLY on Hercules and not z/CMS.
*
*        James Wekel - August 2022
*
*
* Copyright 2017 by Stephen R Orso.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in
*    the documentation and/or other materials provided  with the
*    distribution.
*
* 3. The name of the author may not be used to endorse or promote
*    products derived from this software without specific prior written
*    permission.
*
* DISCLAMER: THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
* HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*
*
*Testcase: Convert To Packed (Long and Extended)
*  Test results, Program interrupt code, CC, and
*  DXC saved for all tests.
*
* Tests the following two DFP packed conversion instructions
*   CONVERT TO PACKED
*        CPDT (long DFP to packed)
*        CPXT (extended DFP to packed)
*
* This routine tests both signed and unsigned packed decimal fields
* and Sign Control (S), Plus-Sign-Code Control (P),
* Force-Plus-Zero Control (F) flags.
*
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R
* commands.
*
* Test Case Order
* 1) Long DFP to signed packed tests
* 2) Long DFP to unsigned packed tests
* 3) Extended DFP to signed packed tests
* 4) Extended DFP to unsigned packed
*
*
* Also tests the following floating point support instructions
*   LOAD  (Long)
*   STORE (Long)
*
***********************************************************************
         SPACE 2
         MACRO
         PADCSECT &ENDLABL
.*
.*  Macro to pad the CSECT to include result data areas if this test
.*  program is not being assembled using asma.  asma generates a core
.*  image that is loaded by the loadcore command, and because the
.*  core image is a binary stored in Github, it makes sense to make
.*  this small effort to keep the core image small.
.*
         AIF   (D'&ENDLABL).GOODPAD
         MNOTE 4,'Missing or invalid CSECT padding label ''&ENDLABL'''
         MNOTE *,'No CSECT padding performed'
         MEXIT
.*
.GOODPAD ANOP            Label valid.  See if we're on asma
         AIF   ('&SYSASM' EQ 'A SMALL MAINFRAME ASSEMBLER').NOPAD
         ORG   &ENDLABL-1   Not ASMA.  Pad CSECT
         MEXIT
.*
.NOPAD   ANOP
         MNOTE *,'asma detected; no CSECT padding performed'
         MEND
*
         MACRO
         SVCALL &CODE
* SVCALL macro - perform supervisory functions
.* Perform various functions as SVCs on Hercules (asma), and by
.* z/CMS friendly equivalents otherwise.
         AIF   ('&SYSASM' EQ 'A SMALL MAINFRAME ASSEMBLER').DOSVC
         AIF   ('&CODE' EQ 'EOJ').ZEOJ
         AIF   ('&CODE' EQ 'AEOJ').ZAEOJ
         AIF   ('&CODE' EQ 'PROBST').ZPROBST
         AIF   ('&CODE' EQ 'SUPVST').ZSUPVST
         MNOTE 4,'Missing or invalid SVCALL code ''&CODE'''
         MNOTE *,'No supervisory funcion performed'
         MEXIT
.ZAEOJ   ANOP
         MNOTE *,'No Abend capability in z/CMS.  Normal EOJ performed.'
         BR    R14           Return to z/CMS test rig.
.ZEOJ    ANOP
         MEXIT
.* Switch between problem and supervisor states
.ZPROBST ANOP
         LA    R0,2          Request problem state
         AGO   .ZSVC
.ZSUPVST ANOP
         LA    R0,3          Request supervisor state
.ZSVC    ANOP
         SVC   255           Invoke z/CMS SVC Handler
         MEXIT
.DOSVC   ANOP
         AIF   ('&CODE' EQ 'EOJ').HEOJ
         AIF   ('&CODE' EQ 'AEOJ').HAEOJ
         AIF   ('&CODE' EQ 'PROBST').HPROBST
         AIF   ('&CODE' EQ 'SUPVST').HSUPVST
         MNOTE 4,'Missing or invalid SVCALL code ''&CODE'''
         MNOTE *,'No supervisory funcion performed'
         MEXIT
.HAEOJ   ANOP
         SVC   1             Abnormal end of job
         MEXIT
.HEOJ    ANOP
         SVC   0             Normal end of job
         MEXIT
.* Switch between problem and supervisor states
.HPROBST ANOP
         LA    R0,2          Request problem state
         AGO   .HSVC
.HSUPVST ANOP
         LA    R0,3          Request supervisor state
.HSVC    ANOP
         SVC   255           Invoke z/CMS SVC Handler
         MEXIT
         MEND
*
*  Note: for compatibility with the z/CMS test rig, do not change
*  or use R11, R14, or R15.  Everything else is fair game.
*
DFPLTTDC START 0
STRTLABL EQU   *
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
R11      EQU   11                  **Reserved for z/CMS test rig
R12      EQU   12
R13      EQU   13                  Mainline return address
R14      EQU   14                  **Return address for z/CMS test rig
R15      EQU   15                  **Base register on z/CMS or Hyperion
*
* Floating Point Register equates to keep the cross reference clean
*
FPR0     EQU   0
FPR1     EQU   1
FPR2     EQU   2
FPR3     EQU   3
FPR4     EQU   4
FPR5     EQU   5
FPR6     EQU   6
FPR7     EQU   7
FPR8     EQU   8
FPR9     EQU   9
FPR10    EQU   10
FPR11    EQU   11
FPR12    EQU   12
FPR13    EQU   13
FPR14    EQU   14
FPR15    EQU   15
*
         USING *,R15
*
* Above works on real iron (R15=0 after sysclear)
* and in z/CMS (R15 points to start of load module)
*
         SPACE 2
***********************************************************************
*
* Low core definitions, Restart PSW, Program Check routine, and
* Supervisor Call routine..
*
***********************************************************************
         ORG   STRTLABL+X'00'     z/Arch IPL PSW
         DC    X'0000000000000000',AD(START)

         SPACE 2
         ORG   STRTLABL+X'88'      Supervisor call interruption area
SVILC    DS    H                   ..Instruction length in bytes
SVINTCD  DS    H                   ..SVC number
*
         ORG   STRTLABL+X'8C'      Program check interruption area
PCILC    DS    H                   ..Instruction length in bytes
PCINTCD  DS    H                   ..PC interruption code
*
SVOLDPSW EQU   STRTLABL+X'140'     z/Arch Supervisor call old PSW
PCOLDPSW EQU   STRTLABL+X'150'     z/Arch Program check old PSW
*
         ORG   STRTLABL+X'1A0'     z/Arch Restart PSW
         DC    X'0000000180000000',AD(START)
*
         ORG   STRTLABL+X'1C0'     z/Arch Supervisor call new PSW
         DC    X'0000000000000000',AD(SVCALL)
*
         ORG   STRTLABL+X'1D0'     z/Arch Program check new PSW
         DC    X'0000000000000000',AD(PROGCHK)
*
* Program check routine.  If Program Specification or Data Exception,
* continue execution at the instruction following the program check.
* Otherwise, hard wait.
* Save Program interrup code to confirm Program Exceptions:
* Data with DXC 0, general operand for 'Unused digits must be zero.'
* checks.
*
         ORG   STRTLABL+X'200'
PROGCHK  DS    0H            Program check occured...
         MVC   LASTPCIND,PCINTCD
         CLI   PCINTCD+1,X'06'  Program Specification Exception?
         JE    PCCONT            ..yes, continue test
         CLI   PCINTCD+1,X'07'  Data Exception?
         JE    PCCONT            ..yes, continue test
*                            ..no, hardwait (not sure if R15 is ok)
PCNOTDTA DS    0H
         SVCALL AEOJ         Signal abend
*
PCCONT   DS    0H
         LPSWE PCOLDPSW      ..yes, resume program execution

*
* Supervisor call routine.  Four SVCs are defined:
* - SVC 0 - Normal end of job
* - SVC 1 - Abnormal end of job
* - SVC 255 - Return control in problem or supervisor state
*
SVCALL   DS    0H            Supervisor call occurred...
         CLI   SVINTCD+1,X'00'  SVC 0?
         BE    EOJ           ..Yes, terminate
         CLI   SVINTCD+1,X'01'  SVC 1?
         BE    EOJ           ..Yes, terminate
         CLI   SVINTCD+1,X'FF'  SVC 255, change privilege?
         BE    SETPRIVS       ..Yes, set requested privilege
         B     EOJ           ..invalid SVC, abend.
*
* Privilege change.  Determine if to problem or to supervisor state
*
SETPRIVS DS    0H            Determine privilege state requested
         CH    R0,=H'2'      Problem state requested?
         BE    SETPROBS      ..Yes, do it
         CH    R0,=H'3'      Supervisor state requested?
         BE    SETSUPVS      ..Yes, do it
         B     EOJ           ..out of choices, time to hard wait.
*
* SVC 255 R0=2: Return to caller in problem state
*
SETPROBS DS    0H            Set problem sat for caller
         OI    SVOLDPSW+1,X'01'  Turn on problem state bit
         LPSWE SVOLDPSW      Resume execution
*
* SVC 255 R0=3: Return to caller in supervisor state
*
SETSUPVS DS    0H            Set problem sat for caller
         NI    SVOLDPSW+1,X'FF'-X'01' Turn off problem state bit
         LPSWE SVOLDPSW      Resume execution
*
* No opportunity to provide a return code to z/CMS, so if on z/CMS,
* just exit.
*
EOJ      LTR   R14,R14        Return address provided?
         BNZR  R14            ..Yes, return to z/CMS test rig.
         CLI   SVINTCD+1,X'00'  Normal End of Job?
         BE    NORMEOJ       ..yes, normal EOJ
         LPSWE HARDWAIT      ..Yes, load Abend hard wait PSW
*
NORMEOJ  LPSWE NORMWAIT      Load abend hard wait PSW
*
         DS    0D             Ensure correct alignment for psw
NORMWAIT DC    X'0002000000000000',AD(0)  Normal end - disabled wait
HARDWAIT DC    X'0002000000000000',XL6'00',X'DEAD' Abnormal end
*
**************************************************************
* saved program interupt code
**************************************************************
         DS    0D
LASTPCIND DS   H              last Interupt code
*
         LTORG ,             Interrupt handler literal pool
*
***********************************************************************
         EJECT
***********************************************************************
*
*  Main program.  Enable Advanced Floating Point, process test cases.
*
***********************************************************************
         SPACE 2
START    DS    0H
         STCTL R0,R0,CTLR0   Store CR0 to enable AFP
         OI    CTLR0+1,X'04' Turn on AFP bit
         LCTL  R0,R0,CTLR0   Reload updated CR0
         SVCALL PROBST       Switch to problem state for tests
*===
         LA    R10,ARGLP     Test CPDT for Long DFP fields
         BAS   R13,TESTLP
*
         LA    R10,ARGXP     Test CPXT for extended DFP fields
         BAS   R13,TESTXP
*

         SVCALL EOJ          Normal end of job
*
         LTORG ,             Literal pool for supervisor routines
*
CTLR0    DS    F
*
* Input values parameter list, three fullwords for each test data set
*      1) Address of inputs,
*      2) Address to place results, and
*      3) Address to place DXC/Flags/cc values.
*
         ORG   STRTLABL+X'300'  Enable run-time replacement
*
* Test sets for Convert To Packed : CDPT and CPXT
*
ARGLP    DS    0F             Input for long DFP fields
         DC    A(LPADRS)
         DC    A(LPOUT)
         DC    A(LPOUTCC)

ARGXP    DS    0F             Input for extended DFP fields
         DC    A(XPADRS)
         DC    A(XPOUT)
         DC    A(XPOUTCC)

         EJECT
***********************************************************************
*
* Perform CONVERT TO PACKED for long DFP field tests.
* This includes 33 long DFP fields with eight (8) CPDT executions
* per field. The condition code is set:
*              0 Source is zero
*              1 Source is not special and is less than zero
*              2 Source is not special and is greater than zero
*              3 Source is special which is infinity, QNaN, SNaN,
*                or a partial result
**
* Tests include Program Specification Execption when the size
* of the packed field is too long (greater than 9).
*
*        R0    work register
*        R1    length of packed field (R5 ->)
*        R2    pointer to input address list
*        R3    Count of 'execute' remaining
*        R4    address of next execute
*
*        R5    pointer to packed signed field
*        R7    pointer to results area
*        R8    pointer to prog Check, DXC, CC area
*        R9    Execute loop return
*        R12   Arg processing loop return
*        R13   RETURN ADDRESS
*        R15   Base register
*
*        FPR0  work register - result of CPDT and CPXT instructions
***********************************************************************
         SPACE 2
TESTLP   DS    0H             Test long DFP to packed
         L     R2,0(R10)      Get pointer to address of test inputs
         LM    R7,R8,4(R10)   Get address of result and CC areas.
         LTR   R2,R2          Any test cases?
         BZR   R13            ..No, return to caller
*
         BASR  R12,0          Set TOP of argument LOOP
*
         L     R5,0(R2)       Get address of arguement
         LTR   R5,R5          End of arg list?
         BZR   R13                  yes, return
*
         LH    R1,0(R5)       packed field length
         BCTR  R1,0              less one (for Execute)
         LD    FPR0,8(R5)     load long DFP test value

         L     R3,=A(LPEXCNT) set up execute loop
         LA    R4,LPEX
         BASR  R9,0           Set TOP of Execute LOOP
*
         MVC   LASTPCIND,=x'FFFF' ensure Prg Check detectable
*
         EX    R1,0(,R4)      do CPDT

         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         N     R0,=XL4'00000003'    just the CC
         STC   R0,3(R8)      save CC
         MVC   0(L'LASTPCIND,R8),LASTPCIND
         EFPC  R0             Extract FPC contents to R0
         STCM  R0,B'0010',2(R8)  Store any DXC code
*
*                             setup for next execute
         LA    R4,6(,R4)      move to next CPDT execute
         LA    R7,1(R7,R1)    move to next result (packed length)
         LA    R8,4(,R8)      move to next result exception
*
         BCTR  R3,R9          next execute
*
         LA    R2,4(,R2)      Point to next arg list pointer
         BR    R12            next test
*
*        CPDT EXECUTE instructions
*
LPEX     DS    0H
         CPDT  FPR0,0(,R7),0      UNSIGNED PACKED
         CPDT  FPR0,0(,R7),1      UNSIGNED PACKED - Force Plus zero
         CPDT  FPR0,0(,R7),2      UNSIGNED PACKED - Plus-Sign-Code
         CPDT  FPR0,0(,R7),3      UNSIGNED PACKED - Force Plus zero +
*                                                 .. Plus-Sign-Code
         CPDT  FPR0,0(,R7),8      SIGNED PACKED
         CPDT  FPR0,0(,R7),9      SIGNED PACKED - Force Plus zero
         CPDT  FPR0,0(,R7),10     SIGNED PACKED - Plus-Sign-Code
         CPDT  FPR0,0(,R7),11     SIGNED PACKED - Force Plus zero +
*                                                 .. Plus-Sign-Code
LPEXCNT  EQU   (*-LPEX)/6         # of CPDT executes
         LTORG
         EJECT
***********************************************************************
*
* Perform CONVERT TO PACKED for extended DFP fields.
* This includes 33 extended DFP fields with eight (8) CPDT executions
* per field. The condition code is set:
*              0 Source is zero
*              1 Source is not special and is less than zero
*              2 Source is not special and is greater than zero
*              3 Source is special which is infinity, QNaN, SNaN,
*                or a partial result
*
* Tests include Program Specification Execption when the size
* of the packed field is too long (greater than 18).

*        R0    work register
*        R1    length of packed field (R5 ->)
*        R2    pointer to input address list
*        R3    Count of 'execute' remaining
*        R4    address of next execute
*
*        R5    pointer to packed signed field
*        R7    pointer to results area
*        R8    pointer to prog Check, DXC, CC area
*        R9    Execute loop return
*        R12   Arg processing loop return
*        R13   RETURN ADDRESS
*        R15   Base register
*
*        FPR0  work register - result of CPDT and CPXT instructions
***********************************************************************
         SPACE 2
TESTXP   DS    0H             Test Extended DFP to packed
         L     R2,0(R10)      Get pointer to address of test inputs
         LM    R7,R8,4(R10)   Get address of result and CC areas.
         LTR   R2,R2          Any test cases?
         BZR   R13            ..No, return to caller
*
         BASR  R12,0          Set TOP of argument LOOP
*
         L     R5,0(R2)       Get address of arguement
         LTR   R5,R5          End of arg list?
         BZR   R13                  yes, return
*
         LH    R1,0(R5)       packed field length
         BCTR  R1,0              less one (for Execute)
         LD    FPR0,8(R5)     extended DFP test value - first 1/2
         LD    FPR2,16(R5)       ... second 1/2

         L     R3,=A(XPEXCNT) set up execute loop
         LA    R4,XPEX
         BASR  R9,0           Set TOP of Execute LOOP
*
         MVC   LASTPCIND,=x'FFFF' ensure Prg Check detectable

         EX    R1,0(,R4)      do CPXT

         IPM   R0            Get condition code and program mask
         SRL   R0,28         Isolate CC in low order byte
         N     R0,=XL4'00000003'    just the CC
         STC   R0,3(R8)      save CC
         MVC   0(L'LASTPCIND,R8),LASTPCIND
         EFPC  R0             Extract FPC contents to R0
         STCM  R0,B'0010',2(R8)  Store any DXC code

*                             setup for next execute
         LA    R4,6(,R4)      move to next CPDT execute
         LA    R7,1(R7,R1)    move to next result (packed length)
         LA    R8,4(,R8)      move to next result exception

         BCTR  R3,R9          next execute

         LA    R2,4(,R2)      Point to next arg list pointer
         BR    R12            next test
*
*        CPXT EXECUTE instructions
*
XPEX     DS    0H
         CPXT  FPR0,0(,R7),0      UNSIGNED PACKED
         CPXT  FPR0,0(,R7),1      UNSIGNED PACKED - Force Plus zero
         CPXT  FPR0,0(,R7),2      UNSIGNED PACKED - Plus-Sign-Code
         CPXT  FPR0,0(,R7),3      UNSIGNED PACKED - Force Plus zero +
*                                                 .. Plus-Sign-Code
         CPXT  FPR0,0(,R7),8      SIGNED PACKED
         CPXT  FPR0,0(,R7),9      SIGNED PACKED - Force Plus zero
         CPXT  FPR0,0(,R7),10     SIGNED PACKED - Plus-Sign-Code
         CPXT  FPR0,0(,R7),11     SIGNED PACKED - Force Plus zero +
*                                                 .. Plus-Sign-Code
XPEXCNT  EQU   (*-XPEX)/6         # of CPDT executes
         LTORG
         EJECT
***********************************************************************
*
*  END OF TEST CASE BASE REGISTER ADDRESSABLE STORAGE.
*
*  Everything after this point is addressed by address constants.
*
***********************************************************************
         EJECT
***********************************************************************
*
*  Invalid FPR results ...
*
***********************************************************************
LFPINVL  DC    X'FFFFFFFFFFFFFFFF'  Invalid result, used to
*                                ..polute result FPR
XFPINVL  DC    X'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'  Invalid result,
*                                ..used to polute result EXTENDED FPR
         EJECT
***********************************************************************
*
*  Input: Address list of packed result length and long DFP fields
*
***********************************************************************
LPADRS   DS    0F
         DC    A(LDFP01)
         DC    A(LDFP02)
         DC    A(LDFP03)
         DC    A(LDFP04)
         DC    A(LDFP05)
         DC    A(LDFP06)
         DC    A(LDFP07)
         DC    A(LDFP08)
         DC    A(LDFP09)
         DC    A(LDFP10)
         DC    A(LDFP11)
         DC    A(LDFP12)
         DC    A(LDFP13)
         DC    A(LDFP14)
         DC    A(LDFP15)
         DC    A(LDFP16)
         DC    A(LDFP17)
         DC    A(LDFP18)
         DC    A(LDFP19)
         DC    A(LDFP20)
         DC    A(LDFP21)
         DC    A(LDFP22)
         DC    A(LDFP23)
         DC    A(LDFP24)
         DC    A(LDFP25)
         DC    A(LDFP26)
         DC    A(LDFP27)
         DC    A(LDFP28)
         DC    A(LDFP29)
         DC    A(LDFP30)
         DC    A(LDFP31)
         DC    A(LDFP32)
         DC    A(LDFP33)
*
         DC    F'0'                 end of address list
         SPACE 1
***********************************************************************
*
*  Input: Long DFP to Signed Pack fields.
*              - Length (2 bytes) of packed field
*              - Long DFP field
*
*NOTE: DO NOT add any prefix '0' within DD'999.99' definitions as
*        this may cause normalization and rounding
***********************************************************************
         DS    0DD         force DD alignment
LDFP01   DC    H'1'
         DC    DD'+0'
LDFP02   DC    H'1'
         DC    DD'-0'
LDFP03   DC    H'1'
         DC    DD'+7'
LDFP04   DC    H'1'
         DC    DD'-7'
LDFP05   DC    H'3'
         DC    DD'+12345'
LDFP06   DC    H'3'
         DC    DD'-12345'
LDFP07   DC    H'6'
         DC    DD'+271828182'
LDFP08   DC    H'6'
         DC    DD'-271828182'
LDFP09   DC    H'8'
         DC    DD'+3.14159265358979'
LDFP10   DC    H'8'
         DC    DD'-3.14159265358979'
LDFP11   DC    H'2'
         DC    DD'+0E368'     +Zero, non-extreme exponent
LDFP12   DC    H'4'
         DC    DD'+0E-397'   ..
LDFP13   DC    H'5'
         DC    DD'-0E368'     -Zero, non-extreme exponent
LDFP14   DC    H'7'
         DC    DD'-0E-397'   ..
LDFP15   DC    H'8'
         DC    DD'+123456789012345E369'  +non-zero, extreme exponent
LDFP16   DC    H'8'
         DC    DD'+123456789012345E-398' ..
LDFP17   DC    H'8'
         DC    DD'-123456789012345E369'  -non-zero, extreme exponent
LDFP18   DC    H'8'
         DC    DD'-123456789012345E-398' ..
LDFP19   DC    H'8'
         DC    DD'+1234567890123456E368'  +non-zero, non-extr., lmd > 0
LDFP20   DC    H'8'
         DC    DD'+1234567890123456E-397' ..
LDFP21   DC    H'8'
         DC    DD'-1234567890123456E368'  -non-zero, non-extr., lmd > 0
LDFP22   DC    H'8'
         DC    DD'-1234567890123456E-397' ..
LDFP23   DC    H'8'
         DC    DD'+(INF)'
LDFP24    DC    H'8'
         DC    DD'-(INF)'
LDFP25    DC    H'8'
         DC    DD'+(QNAN)'
LDFP26    DC    H'8'
         DC    DD'-(QNAN)'
LDFP27    DC    H'8'
         DC    DD'+(SNAN)'
LDFP28    DC    H'8'
         DC    DD'-(SNAN)'
LDFP29   DC    H'6'
         DC    DD'+(DMIN)'
LDFP30   DC    H'6'
         DC    DD'-(DMIN)'
LDFP31   DC    H'10'                 Program Specification Exception
         DC    DD'+3.14159265358979'
LDFP32   DC    H'10'                 Program Specification Exception
         DC    DD'-3.14159265358979'
LDFP33   DC    H'8'                  end of data
         DC    DD'1111111111111111'
         EJECT
***********************************************************************
*
*  Input: Address list of packed result length and long DFP fields
*
***********************************************************************
XPADRS   DS    0F
         DC    A(XDFP01)
         DC    A(XDFP02)
         DC    A(XDFP03)
         DC    A(XDFP04)
         DC    A(XDFP05)
         DC    A(XDFP06)
         DC    A(XDFP07)
         DC    A(XDFP08)
         DC    A(XDFP09)
         DC    A(XDFP10)
         DC    A(XDFP11)
         DC    A(XDFP12)
         DC    A(XDFP13)
         DC    A(XDFP14)
         DC    A(XDFP15)
         DC    A(XDFP16)
         DC    A(XDFP17)
         DC    A(XDFP18)
         DC    A(XDFP19)
         DC    A(XDFP20)
         DC    A(XDFP21)
         DC    A(XDFP22)
         DC    A(XDFP23)
         DC    A(XDFP24)
         DC    A(XDFP25)
         DC    A(XDFP26)
         DC    A(XDFP27)
         DC    A(XDFP28)
         DC    A(XDFP29)
         DC    A(XDFP30)
         DC    A(XDFP31)
         DC    A(XDFP32)
         DC    A(XDFP33)
*
         DC    F'0'                 end of address list
         SPACE 1
***********************************************************************
*
*  Input: Long DFP to Signed Pack fields.
*              - Length (2 bytes) of packed field
*              - Long DFP field
*
*NOTE: DO NOT add any prefix '0' within LD'999.99' definitions as
*        this may cause normalization and rounding
***********************************************************************
         DS    0LD         force LD alignment
XDFP01   DC    H'1'
         DC    LD'+0'
XDFP02   DC    H'1'
         DC    LD'-0'
XDFP03   DC    H'1'
         DC    LD'+7'
XDFP04   DC    H'1'
         DC    LD'-7'
XDFP05   DC    H'3'
         DC    LD'+12345'
XDFP06   DC    H'3'
         DC    LD'-12345'
XDFP07   DC    H'6'
         DC    LD'+271828182'
XDFP08   DC    H'6'
         DC    LD'-271828182'
XDFP09   DC    H'8'
         DC    LD'+3.14159265358979'
XDFP10   DC    H'8'
         DC    LD'-3.14159265358979'
XDFP11   DC    H'2'
         DC    LD'+0E368'     +Zero, non-extreme exponent
XDFP12   DC    H'4'
         DC    LD'+0E-397'   ..
XDFP13   DC    H'5'
         DC    LD'-0E368'     -Zero, non-extreme exponent
XDFP14   DC    H'7'
         DC    LD'-0E-397'   ..
XDFP15   DC    H'8'
         DC    LD'+123456789012345E369'  +non-zero, extreme exponent
XDFP16   DC    H'8'
         DC    LD'+123456789012345E-398' ..
XDFP17   DC    H'8'
         DC    LD'-123456789012345E369'  -non-zero, extreme exponent
XDFP18   DC    H'8'
         DC    LD'-123456789012345E-398' ..
XDFP19   DC    H'8'
         DC    LD'+1234567890123456E368'  +non-zero, non-extr., lmd > 0
XDFP20   DC    H'8'
         DC    LD'+1234567890123456E-397' ..
*                             -non-zero, non-extreme, lmd > 0
XDFP21   DC    H'8'
         DC    LD'-1234567890123456789012345678901234E6110'
XDFP22   DC    H'8'
         DC    LD'-1234567890123456789012345678901234E-6175'
XDFP23   DC    H'8'
         DC    LD'+(INF)'
XDFP24   DC    H'8'
         DC    LD'-(INF)'
XDFP25   DC    H'8'
         DC    LD'+(QNAN)'
XDFP26   DC    H'8'
         DC    LD'-(QNAN)'
XDFP27   DC    H'8'
         DC    LD'+(SNAN)'
XDFP28   DC    H'8'
         DC    LD'-(SNAN)'
XDFP29   DC    H'6'
         DC    LD'+(DMIN)'
XDFP30   DC    H'6'
         DC    LD'-(DMIN)'
XDFP31   DC    H'19'                Program Specification Exception
         DC    LD'+3.14159265358979'
XDFP32   DC    H'19'                Program Specification Exception
         DC    LD'-3.14159265358979'
XDFP33   DC    H'16'                end of data
         DC    LD'11111111111111111111111111111111'
         EJECT
***********************************************************************
*
*  Locations for from-packed results
*
***********************************************************************
         SPACE 2
LPOUT    EQU   STRTLABL+X'2000'  Long DFP results: CDPT signed
LPOUTCC  EQU   STRTLABL+X'2700'  ... Pgm Check Ind + DXC
*
XPOUT    EQU   STRTLABL+X'3000'  Extended DFP results: CXPT signed
XPOUTCC  EQU   STRTLABL+X'3A00'   ... Pgm Check Ind + DXC
*
ENDLABL  EQU   STRTLABL+X'4000'
         PADCSECT ENDLABL
*
         END