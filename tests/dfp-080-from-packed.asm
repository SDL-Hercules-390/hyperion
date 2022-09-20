  TITLE 'dfp-080-from-packed.asm: Test Convert From Packed'
***********************************************************************
*
*Testcase: Convert From Packed (Long and Extended)
*  Test results, Program interrupt code, and
*  DXC saved for all tests.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*                    dfp-080-from-packed.asm
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
*Testcase Convert From Packed (Long and Extended)
*  Test results, Program interrupt code, DXC saved for all tests.
*
* Tests the following two DFP packed conversion instructions
*   CONVERT FROM PACKED
*        CDPT (packed to long DFP)
*        CXPT (packed to extended DFP)
*
* This routine tests both signed and unsigned packed decimal fields
* and ignore sign flag.
*
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R
* commands.
*
* Test Case Order
* 1) Long signed packed to long DFP tests
* 2) Long unsigned packed to long DFP tests
* 3) Extended signed packed to extended DFP tests
* 4) Extended unsigned packed to extended DFP tests
*
*
* Also tests the following floating point support instructions
*   LOAD  (Long)
*   STORE (Long)
*
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
         LA    R10,ARGLSP     Test CDPT for signed packed fields
         BAS   R13,TESTLSP
*
         LA    R10,ARGLUP     Test CDPT for unsigned packed fields
         BAS   R13,TESTLUP
*
         LA    R10,ARGXSP     Test CXPT for signed packed fields
         BAS   R13,TESTXSP
*
         LA    R10,ARGXUP     Test CXPT for unsigned packed fields
         BAS   R13,TESTXUP

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
* Test sets for Convert From Packed : CDPT and CXDT
*
ARGLSP   DS    0F             Input for long signed packed
         DC    A(LSPADRS)
         DC    A(LSPOUT)
         DC    A(LSPOUTCC)

ARGLUP   DS    0F             Input for long unsigned packed
         DC    A(LUPADRS)
         DC    A(LUPOUT)
         DC    A(LUPOUTCC)


ARGXSP   DS    0F             Input for extended signed packed
         DC    A(XSPADRS)
         DC    A(XSPOUT)
         DC    A(XSPOUTCC)

ARGXUP   DS    0F             Input for extended unsigned packed
         DC    A(XUPADRS)
         DC    A(XUPOUT)
         DC    A(XUPOUTCC)
*
         EJECT
***********************************************************************
*
* Perform CONVERT FROM PACKED for signed packed tests.
* This includes 14 packed fields with two CDPT executions
* per field. CDPT The condition code remained unchanged.
*
*        R0    work register
*        R1    length of packed field (R5 ->)
*        R2    pointer to input address list
*
*        R5    pointer to packed signed field
*        R7    pointer to results area
*        R8    pointer to prog Check, DXC, CC area
*
*        R12   Arg processing loop return
*        R13   RETURN ADDRESS
*        R15   Base register
*
*        FPR0  work register - result of CDPT and CXDT instructions
***********************************************************************
         SPACE 2
TESTLSP  DS    0H             Test long signed packed input
         L     R2,0(R10)      Get pointer to address of test inputs
         LM    R7,R8,4(R10)   Get address of result and CC areas.
         LTR   R2,R2          Any test cases?
         BZR   R13            ..No, return to caller

         BASR  R12,0          Set TOP of LOOP
*
         L     R5,0(R2)       Get address of packed field
         LTR   R5,R5          End of arg list?
         BZR   R13              yes, return

         LR    R1,R5          packed field address
         S     R1,=F'2'       packed field length address
         LH    R1,0(R1)       get packed field length
         BCTR  R1,0              less one (for Execute)

         LD    FPR0,LFPINVL   Ensure an unchanged FPR0 is detectable
         MVC   LASTPCIND,=x'FFFF' ensure Prg Check detectable

         EX    R1,LSPEX       do CDPT into FPR0

         MVC   0(L'LASTPCIND,R8),LASTPCIND
         EFPC  R0             Extract FPC contents to R0
         STCM  R0,B'0010',2(R8)  Store any DXC code
         STD   FPR0,0(R7)     save conversion
         LA    R7,8(R7)       move to next result
         LA    R8,4(R8)       move to next result exception

         LD    FPR0,LFPINVL   Ensure an unchanged FPR0 is detectable
         MVC   LASTPCIND,=x'FFFF' ensure Prg Check detectable

         EX    R1,LSPEX+6     do 2nd CDPT into FPR0

         MVC   0(L'LASTPCIND,R8),LASTPCIND
         EFPC  R0             Extract FPC contents to R0
         STCM  R0,B'0010',2(R8)  Store any DXC code
         STD   FPR0,0(R7)     save conversion
         LA    R7,8(R7)       move to next result
         LA    R8,4(R8)       move to next result exception

         LA    R2,4(R2)       Point to next arg list pointer
         BR    R12            next test
*
*        CDPT EXECUTE instructions
*
LSPEX    CDPT  FPR0,0(,R5),8      SIGNED PACKED
         CDPT  FPR0,0(,R5),9      SIGNED PACKED - ignore sign
         EJECT
***********************************************************************
*
* Perform CONVERT FROM PACKED for unsigned packed tests.
* This includes 10 packed fields with two CDPT executions
* per field. CDPT The condition code remained unchanged.
*
*        R0    work register
*        R1    length of packed field (R5 ->)
*        R2    pointer to input address list
*
*        R5    pointer to packed unsigned field
*        R7    pointer to results area
*        R8    pointer to prog Check, DXC, CC area
*
*        R12   Arg processing loop return
*        R13   RETURN ADDRESS
*        R15   Base register
*
*        FPR0  work register - result of CDPT and CXDT instructions
***********************************************************************
         SPACE 2
TESTLUP  DS    0H             Test long unsigned packed input
         L     R2,0(R10)      Get pointer to address of test inputs
         LM    R7,R8,4(R10)   Get address of result and CC areas.
         LTR   R2,R2          Any test cases?
         BZR   R13            ..No, return to caller

         BASR  R12,0          Set TOP of LOOP
*
         L     R5,0(R2)       Get address of packed field
         LTR   R5,R5          End of arg list?
         BZR   R13              yes, return

         LR    R1,R5          packed field address
         S     R1,=F'2'       packed field length address
         LH    R1,0(R1)       get packed field length
         BCTR  R1,0              less one (for Execute)

         LD    FPR0,LFPINVL   Ensure an unchanged FPR0 is detectable
         MVC   LASTPCIND,=x'FFFF' ensure Prg Check detectable

         EX    R1,LUPEX       do CDPT into FPR0

         MVC   0(L'LASTPCIND,R8),LASTPCIND
         EFPC  R0             Extract FPC contents to R0
         STCM  R0,B'0010',2(R8)  Store any DXC code
         STD   FPR0,0(R7)     save conversion
         LA    R7,8(R7)       move to next result
         LA    R8,4(R8)       move to next result exception

         LD    FPR0,LFPINVL   Ensure an unchanged FPR0 is detectable
         MVC   LASTPCIND,=x'FFFF' ensure Prg Check detectable

         EX    R1,LUPEX+6     do 2nd CDPT into FPR0

         MVC   0(L'LASTPCIND,R8),LASTPCIND
         EFPC  R0             Extract FPC contents to R0
         STCM  R0,B'0010',2(R8)  Store any DXC code
         STD   FPR0,0(R7)     save conversion
         LA    R7,8(R7)       move to next result
         LA    R8,4(R8)       move to next result exception

         LA    R2,4(R2)       Point to next arg list pointer
         BR    R12            next test
*
*        CDPT EXECUTE instructions
*
LUPEX    CDPT  FPR0,0(,R5),0        UNSIGNED PACKED
         CDPT  FPR0,0(,R5),1      UNSIGNED PACKED - ignore sign
         EJECT
***********************************************************************
*
* Perform CONVERT FROM PACKED for signed packed tests.
* This includes 22 packed fields with two CXPT executions
* per field. For CXPT, the condition code remained unchanged.
*
*        R0    work register
*        R1    length of packed field (R5 ->)
*        R2    pointer to input address list
*
*        R5    pointer to packed signed field
*        R7    pointer to results area
*        R8    pointer to prog Check, DXC, CC area
*
*        R12   Arg processing loop return
*        R13   RETURN ADDRESS
*        R15   Base register
*
*        FPR0  work register - result of CDPT and CXDT instructions
***********************************************************************
         SPACE 2
TESTXSP  DS    0H              Test extended unsigned packed input
         L     R2,0(R10)       Get pointer to address of test inputs
         LM    R7,R8,4(R10)    Get address of result and CC areas.
         LTR   R2,R2           Any test cases?
         BZR   R13              ..No, return to caller

         BASR  R12,0           Set TOP of LOOP
*
         L     R5,0(R2)        Get address of packed field
         LTR   R5,R5           End of arg list?
         BZR   R13              yes, return

         LR    R1,R5           packed field address
         S     R1,=F'2'        packed field length address
         LH    R1,0(R1)        get packed field length
         BCTR  R1,0              less one (for Execute)

         LD    FPR0,LFPINVL    Ensure an unchanged FPR0 is detectable
         LD    FPR2,LFPINVL    Ensure an unchanged FPR2 is detectable
         MVC   LASTPCIND,=x'FFFF' ensure Prg Check detectable

         EX    R1,XSPEX        do CDPT into FPR0

         MVC   0(L'LASTPCIND,R8),LASTPCIND
         EFPC  R0              Extract FPC contents to R0
         STCM  R0,B'0010',2(R8)  Store any DXC code
         STD   FPR0,0(R7)      save conversion - first half
         STD   FPR2,8(R7)      save conversion - second half
         LA    R7,16(R7)       move to next result
         LA    R8,4(R8)        move to next result exception

         LD    FPR0,LFPINVL    Ensure an unchanged FPR0 is detectable
         LD    FPR2,LFPINVL    Ensure an unchanged FPR2 is detectable
         MVC   LASTPCIND,=x'FFFF' ensure Prg Check detectable

         EX    R1,XSPEX+6      do 2nd CDPT into FPR0

         MVC   0(L'LASTPCIND,R8),LASTPCIND
         EFPC  R0              Extract FPC contents to R0
         STCM  R0,B'0010',2(R8)  Store any DXC code
         STD   FPR0,0(R7)      save conversion - first half
         STD   FPR2,8(R7)      save conversion - second half
         LA    R7,16(R7)       move to next result
         LA    R8,4(R8)        move to next result exception

         LA    R2,4(R2)       Point to next arg list pointer
         BR    R12            next test
*
*        CDPT EXECUTE instructions
*
XSPEX    CXPT  FPR0,0(,R5),8       SIGNED PACKED
         CXPT  FPR0,0(,R5),9       SIGNED PACKED - ignore sign

         EJECT
***********************************************************************
*
* Perform CONVERT FROM PACKED for unsigned packed tests.
* This includes 17 packed fields with two CXPT executions
* per field. For CXPT, the condition code remained unchanged.
*
*        R0    work register
*        R1    length of packed field (R5 ->)
*        R2    pointer to input address list
*
*        R5    pointer to packed unsigned field
*        R7    pointer to results area
*        R8    pointer to prog Check, DXC, CC area
*
*        R12   Arg processing loop return
*        R13   RETURN ADDRESS
*        R15   Base register
*
*        FPR0  work register - result of CDPT and CXDT instructions
***********************************************************************
         SPACE 2
TESTXUP  DS    0H              Test extended unsigned packed input
         L     R2,0(R10)       Get pointer to address of test inputs
         LM    R7,R8,4(R10)    Get address of result and CC areas.
         LTR   R2,R2           Any test cases?
         BZR   R13              ..No, return to caller

         BASR  R12,0           Set TOP of LOOP
*
         L     R5,0(R2)        Get address of packed field
         LTR   R5,R5           End of arg list?
         BZR   R13              yes, return

         LR    R1,R5           packed field address
         S     R1,=F'2'        packed field length address
         LH    R1,0(R1)        get packed field length
         BCTR  R1,0              less one (for Execute)

         LD    FPR0,LFPINVL    Ensure an unchanged FPR0 is detectable
         LD    FPR2,LFPINVL    Ensure an unchanged FPR2 is detectable
         MVC   LASTPCIND,=x'FFFF' ensure Prg Check detectable

         EX    R1,XUPEX        do CDPT into FPR0

         MVC   0(L'LASTPCIND,R8),LASTPCIND
         EFPC  R0              Extract FPC contents to R0
         STCM  R0,B'0010',2(R8)  Store any DXC code
         STD   FPR0,0(R7)      save conversion - first half
         STD   FPR2,8(R7)      save conversion - second half
         LA    R7,16(R7)       move to next result
         LA    R8,4(R8)        move to next result exception

         LD    FPR0,LFPINVL    Ensure an unchanged FPR0 is detectable
         LD    FPR2,LFPINVL    Ensure an unchanged FPR2 is detectable
         MVC   LASTPCIND,=x'FFFF' ensure Prg Check detectable

         EX    R1,XUPEX+6      do 2nd CDPT into FPR0

         MVC   0(L'LASTPCIND,R8),LASTPCIND
         EFPC  R0              Extract FPC contents to R0
         STCM  R0,B'0010',2(R8)  Store any DXC code
         STD   FPR0,0(R7)      save conversion - first half
         STD   FPR2,8(R7)      save conversion - second half
         LA    R7,16(R7)       move to next result
         LA    R8,4(R8)        move to next result exception

         LA    R2,4(R2)       Point to next arg list pointer
         BR    R12            next test
*
*        CXPT EXECUTE instructions
*
XUPEX    CXPT  FPR0,0(,R5),0       UNSIGNED PACKED
         CXPT  FPR0,0(,R5),1       UNSIGNED PACKED - ignore sign
         LTORG
         EJECT
***********************************************************************
         SPACE 3
***********************************************************************
*
         LTORG
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
*  Input: Address list of Signed Long Pack fields
*
***********************************************************************
LSPADRS  DS    0F
         DC    A(LSP01)
         DC    A(LSP02)
         DC    A(LSP03)
         DC    A(LSP04)
         DC    A(LSP05)
         DC    A(LSP06)
         DC    A(LSP07)
         DC    A(LSP08)
         DC    A(LSP09)
         DC    A(LSP10)
         DC    A(LSP11)
         DC    A(LSP12)
         DC    A(LSP13)
         DC    A(LSP14)
*
         DC    F'0'                 end of address list
         SPACE 1
***********************************************************************
*
*  Input: Signed Long Pack test fields.
*              Length (2 bytes) of packed field preceeds packed test
*
***********************************************************************
         DS    0H
         DC    AL2(L'LSP01)
LSP01    DC    P'+0'
         DS    0H
         DC    AL2(L'LSP02)
LSP02    DC    P'-0'
         DS    0H
         DC    AL2(L'LSP03)
LSP03    DC    P'+12345'
         DS    0H
         DC    AL2(L'LSP04)
LSP04    DC    P'-12345'
         DS    0H
         DC    AL2(L'LSP05)
LSP05    DC    P'+271828182'
         DS    0H
         DC    AL2(L'LSP06)
LSP06    DC    P'-271828182'
         DS    0H
         DC    AL2(L'LSP07)
LSP07    DC    P'+314159265358979'
         DS    0H
         DC    AL2(L'LSP08)
LSP08    DC    P'-314159265358979'
         DS    0H
         DC    AL2(L'LSP09)
LSP09    DC    P'+3141592653589793'
         DS    0H
         DC    AL2(L'LSP10)
LSP10    DC    P'-3141592653589793'
         DS    0H
         DC    AL2(L'LSP11)
LSP11    DC    P'+93141592653589793'   error - too long - 17 digits
         DS    0H
         DC    AL2(L'LSP12)
LSP12    DC    P'-93141592653589793'   error - too long - 17 digits
         DS    0H
         DC    AL2(L'LSP13)
LSP13    DC    P'+993141592653589793'  Program Specification Exception
         DS    0H
         DC    AL2(L'LSP14)
LSP14    DC    P'-993141592653589793'  Program Specification Exception
         EJECT
***********************************************************************
*
*  Input: Address list of Unsigned Long Pack fields
*
***********************************************************************
LUPADRS  DS    0F
         DC    A(LUP01)
         DC    A(LUP02)
         DC    A(LUP03)
         DC    A(LUP04)
         DC    A(LUP05)
         DC    A(LUP06)
         DC    A(LUP07)
         DC    A(LUP08)
         DC    A(LUP09)
         DC    A(LUP10)
*
         DC    F'0'                 end of address list
         SPACE 1
***********************************************************************
*
*  Input: Unsigned Long Pack test fields.
*              Length (2 bytes) of packed field preceeds packed test
*
***********************************************************************
         DS    0H
         DC    AL2(L'LUP01)
LUP01    DC    X'0'
         DS    0H
         DC    AL2(L'LUP02)
LUP02    DC    X'1'
         DS    0H
         DC    AL2(L'LUP03)
LUP03    DC    X'12345'
         DS    0H
         DC    AL2(L'LUP04)
LUP04    DC    X'123456'
         DS    0H
         DC    AL2(L'LUP05)
LUP05    DC    X'271828182'
         DS    0H
         DC    AL2(L'LUP06)
LUP06    DC    X'2718281828'
         DS    0H
         DC    AL2(L'LUP07)
LUP07    DC    X'3141592653589793'
         DS    0H
         DC    AL2(L'LUP08)
LUP08    DC    X'93141592653589793'     error - too long - 17 digits
         DS    0H
         DC    AL2(L'LUP09)
LUP09    DC    X'993141592653589793'    error - too long - 18 digits
         DS    0H
         DC    AL2(L'LUP10)
LUP10    DC    X'9993141592653589793'  Program Specification Exception
         EJECT
***********************************************************************
*
*  Input: Address list of Signed Extended Pack fields
*              - use long packed test fields
*              - add extended fields with 17-34 digits
*
***********************************************************************
XSPADRS  DS    0F
*                             reuse long pack signed fields
*                             .. no exceptions
         DC    A(LSP01)
         DC    A(LSP02)
         DC    A(LSP03)
         DC    A(LSP04)
         DC    A(LSP05)
         DC    A(LSP06)
         DC    A(LSP07)
         DC    A(LSP08)
         DC    A(LSP09)
         DC    A(LSP10)
         DC    A(LSP11)
         DC    A(LSP12)
         DC    A(LSP13)
         DC    A(LSP14)
*                             additional extended signed packed fields
         DC    A(XSP01)
         DC    A(XSP02)
         DC    A(XSP03)
         DC    A(XSP04)
         DC    A(XSP05)
         DC    A(XSP06)
         DC    A(XSP07)
         DC    A(XSP08)
*
         DC    F'0'                 end of address list
***********************************************************************
*
*  Input: Signed Extended Pack test fields: 17-34 digits.
*              Length (2 bytes) of packed field preceeds packed test
*
***********************************************************************
         DS    0H
         DC    AL2(L'XSP01)
XSP01    DC    P'+1234567890123456789012345'
         DS    0H
         DC    AL2(L'XSP02)
XSP02    DC    P'-1234567890123456789012345'
         DS    0H
         DC    AL2(L'XSP03)
XSP03    DC    P'+3141592653589793238462643383279502'
         DS    0H
         DC    AL2(L'XSP04)
XSP04    DC    P'-3141592653589793238462643383279502'
         DS    0H
         DC    AL2(L'XSP05)            error - too long - 35 digits
XSP05    DC    P'+93141592653589793238462643383279502'
         DS    0H
         DC    AL2(L'XSP06)            error - too long - 35 digits
XSP06    DC    P'-93141592653589793238462643383279502'
         DS    0H
         DC    AL2(L'XSP07)            Program Specification Exception
XSP07    DC    P'+993141592653589793238462643383279502'
         DS    0H
         DC    AL2(L'XSP08)            Program Specification Exception
XSP08    DC    P'-993141592653589793238462643383279502'
         EJECT
***********************************************************************
*
*  Input: Address list of Signed Extended Pack fields
*              - use long packed test fields
*              - add extended fields with 17-34 digits
*
***********************************************************************
XUPADRS  DS    0F
*                             reuse long pack signed fields
*                             .. no exceptions
         DC    A(LUP01)
         DC    A(LUP02)
         DC    A(LUP03)
         DC    A(LUP04)
         DC    A(LUP05)
         DC    A(LUP06)
         DC    A(LUP07)
         DC    A(LUP08)
         DC    A(LUP09)
         DC    A(LUP10)
*                             additional extended signed packed fields
         DC    A(XUP01)
         DC    A(XUP02)
         DC    A(XUP03)
         DC    A(XUP04)
         DC    A(XUP05)
         DC    A(XUP06)
         DC    A(XUP07)
*
         DC    F'0'                 end of address list
***********************************************************************
*
*  Input: Signed Extended Pack test fields: 17-34 digits.
*              Length (2 bytes) of packed field preceeds packed test
*
***********************************************************************
         DS    0H
         DC    AL2(L'XUP01)
XUP01    DC    X'12345678901234567890'
         DS    0H
         DC    AL2(L'XUP02)
XUP02    DC    X'1234567890123456789012345'
         DS    0H
         DC    AL2(L'XUP03)
XUP03    DC    X'3141592653589793238462643'
         DS    0H
         DC    AL2(L'XUP04)
XUP04    DC    X'3141592653589793238462643383279502'
         DS    0H
         DC    AL2(L'XUP05)            error - too long - 35 digits
XUP05    DC    X'93141592653589793238462643383279502'
         DS    0H
         DC    AL2(L'XUP06)            error - too long - 36 digits
XUP06    DC    X'993141592653589793238462643383279502'
         DS    0H
         DC    AL2(L'XUP07)            Program Specification Exception
XUP07    DC    X'9993141592653589793238462643383279502'
*
         SPACE 2
***********************************************************************
*
*  Locations for from-packed results
*
***********************************************************************
         SPACE 2
LSPOUT   EQU   STRTLABL+X'1000'  Long DFP results: CDPT signed
LSPOUTCC EQU   STRTLABL+X'1200'  ... Pgm Check Ind + DXC
*
LUPOUT   EQU   STRTLABL+X'1300'  Long DFP results: CDPT unsigned
LUPOUTCC EQU   STRTLABL+X'1400'   ... Pgm Check Ind + DXC
*
XSPOUT   EQU   STRTLABL+X'1600'  Extended DFP results: CXPT signed
XSPOUTCC EQU   STRTLABL+X'2000'   ... Pgm Check Ind + DXC
*
XUPOUT   EQU   STRTLABL+X'2100'  Extended DFP results: CXPT unsigned
XUPOUTCC EQU   STRTLABL+X'2500'   ... Pgm Check Ind + DXC
*
ENDLABL  EQU   STRTLABL+X'2600'
         PADCSECT ENDLABL
*
         END