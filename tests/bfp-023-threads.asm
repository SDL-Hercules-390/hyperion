  TITLE 'bfp-023-threads.asm: Test IEEE/Softfloat 3a Thread Safety'
***********************************************************************
*
*Testcase ieee.c/Softfloat 3a thread safety.
*  This test case dispatches floating point work on four CPUs.  Any
*  variation from expected results, as detected by each CPU, shows a 
*  lack of thread safety.  
*
*  Each CPU is given a single floating point operation to be performed
*  in a loop.  Each CPU has a different operation to perform, with a 
*  different expected result and IEEE flag set from Softfloat.  
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*                       bfp-023-threads.asm
*
*        This assembly-language source file is part of the
*        Hercules Binary Floating Point Validation Package 
*                        by Stephen R. Orso
*
* Copyright 2016 by Stephen R Orso.
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
*Function/Operation
* - Test Case Processing
*   1) The first CPU is started by the runtest restart command.  The
*      first CPU:
*      a) starts the second CPU using SIGP.
*      b) repeats the assigned floating point operation, looking for
*         evidence of pre-emptive multitasking and incorrect results.
*      c) At the end of a set number of calculations, the other three
*         processors are stopped via SIGP.
*      d) A hardwait PSW is loaded
*   2) The second CPU is started by the SIGP restart from the first
*      CPU.  The second CPU:
*      a) starts the third CPU using SIGP.
*      b) repeats the assigned floating point operation, looking for
*         evidence of pre-emptive multitasking and incorrect results.
*      c) The floating point operation is repeated until the CPU is
*         stopped by the first CPU.  
*   3) The third CPU is started by the second, and starts the fourth.
*      Otherwise, it operates in the same way that CPU two does.
*   4) The fourth CPU is started by the second.  It does not start 
*      another CPU.  Otherwise, it operates in the same way that CPUs
*    two and three do.
*
*Entry Points
* - All entries are via the PSW stored in the restart PSW field
* - CPUnBEG, where n is replaced by 0, 1, 2, or 3.  Each is the start
*   of the CPU-specific code for that CPU.  
*
*Input
* - Floating point operands and expected results are compiled into
*   the program.  
*
*Output
* - All outputs are stored starting at real memory location X'2000'
* - Count of trials performed by each CPU.
* - Count of thread pre-emptions (switches) detected by each CPU.
*   Note that not all pre-emptions are detected.
* - Count of trials that returned incorrect results by CPU.
* - Count of trials that returned incorrect FPCR contents by CPU.
* - For each CPU, the last incorrect result and the last incorrect
*   FPCR contents.  Note that the incorrect result and the incorrect
*   FPCR contents may be from different trials.  
*
*External Dependencies
* - This program is intended to be run on Hercules as part of the
*   'runtest' facility.  
*
*Exit
* Normal- via LPSWE of a disabled wait PSW, address zero
* Abnormal- via LPSWE of a disabled wait PSW, address X'DEAD'
*
*Attributes
* - None
*
*Notes
* - Prefixing is used by this program.  
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
*  Note: for compatibility with the z/CMS test rig, do not change
*  or use R11, R14, or R15.  Everything else is fair game.  
*  Although this program's use of four processors likely precludes
*  its validation in z/CMS.  
*
BFPTHRED START 0
STRTLABL EQU   *
R0       EQU   0                   Work register for cc extraction
R1       EQU   1
R2       EQU   2                   Holds count of test input values
R3       EQU   3                   Points to next test input value(s)
R4       EQU   4                   Rounding tests inner loop control
R5       EQU   5                   Rounding tests outer loop control
R6       EQU   6                   Rounding tests top of inner loop
R7       EQU   7                   Pointer to next result value(s)
R8       EQU   8                   Pointer to next FPCR result
R9       EQU   9                   Rounding tests top of outer loop
R10      EQU   10                  Pointer to test address list
R11      EQU   11                  **Reserved for z/CMS test rig
R12      EQU   12                  Holds number of test cases in set
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
CPU1PRE  EQU   X'10000'        CPU 1 prefix area at 64K
CPU2PRE  EQU   CPU1PRE+X'2000' CPU 2 prefix area at 64K + 8K
CPU3PRE  EQU   CPU2PRE+X'2000' CPU 2 prefix area at 64K + 2 * 8K
*
         USING *,R15
*
* Above works on real iron (R15=0 after sysclear) 
* and in z/CMS (R15 points to start of load module)
*
         SPACE 2 
***********************************************************************
*
* Low core definitions, Restart PSW, and Program Check Routine.
*
***********************************************************************
         SPACE 2
         ORG   STRTLABL+X'8E'      Program check interrution code
PCINTCD  DS    H
*
PCOLDPSW EQU   STRTLABL+X'150'     z/Arch Program check old PSW
*
         ORG   STRTLABL+X'1A0'     z/Arch Restart PSW
RESTRPSW DC    X'0000000180000000',AD(CPU0BEG)  64-bit addr, 4k page
RPSWADR  EQU   RESTRPSW-STRTLABL+8 Displacement of restart psw address
*
         ORG   STRTLABL+X'1D0'     z/Arch Program check new PSW
         DC    X'0000000180000000',AD(PROGCHK)
*
******** Following ORG overlays the PC new PSW with a hard wait.  
*
**       ORG   STRTLABL+X'1D0'     z/Arch Program check new PSW
**       DC    X'0002000000000000',XL6'00',X'DEAD' Abnormal end
* 
* Program check routine.  If Data Exception, continue execution at
* the instruction following the program check.  Otherwise, hard wait.
* No need to collect data.  All interesting DXC stuff is captured
* in the FPCR.
*
         ORG   STRTLABL+X'200'
PROGCHK  DS    0H             Program check occured...
         CLI   PCINTCD+1,X'07'  Data Exception?
         JNE   PCNOTDTA       ..no, hardwait (not sure if R15 is ok)
         LPSWE PCOLDPSW       ..yes, resume program execution
PCNOTDTA DS    0H
         LTR   R14,R14        Return address provided?
         BNZR  R14            Yes, return to z/CMS test rig.  
         LPSWE HARDWAIT       Not data exception, enter disabled wait
*
         EJECT
***********************************************************************
*
* Data areas global to all four processors and areas that are per-cpu.
*
* Per-CPU variables are prefixed with 'CPUn' where n is replaced with
* the CPU number, starting with zero.  
*
***********************************************************************
         SPACE 2
*
SIGPREST EQU   6             SIGP order for CPU Restart
SIGPSTOP EQU   5             SIGP order for CPU Stop
SIGPPREF EQU   13            SIGP order for Set Prefix
*
         DS    0D            Ensure correct alignment for psw
WAITPSW  DC    X'0002000000000000',AD(0)  Normal end - disabled wait
HARDWAIT DC    X'0002000000000000',XL6'00',X'DEAD' Abnormal end
*
CPU0CR0  DS    F             CR0, used to turn on AFP
CPU1CR0  DS    F             CR0, used to turn on AFP
CPU2CR0  DS    F             CR0, used to turn on AFP
CPU3CR0  DS    F             CR0, used to turn on AFP
*
CPU0ADR  DS    H             CPU addr returned from STAP, used by SIGP
CPU1ADR  DS    H             CPU addr returned from STAP, used by SIGP
CPU2ADR  DS    H             CPU addr returned from STAP, used by SIGP
CPU3ADR  DS    H             CPU addr returned from STAP, used by SIGP
         EJECT
***********************************************************************
*
* CPU 0 program.  Set prefixes, start all processors, enable Additional
* Floating Point Registers, repetitively perform a floating point
* operation, then stop all three other processors and load a hard wait 
* PSW.  
*
***********************************************************************
         SPACE 2
CPU0BEG  DS    0H            Start of processing for CPU zero
         STAP  CPU0ADR       Store CPU address of this processor
         LH    R1,CPU0ADR    Get CPU address
         LA    R1,1(,R1)     Update to next CPU address
         STH   R1,CPU1ADR    Store next CPU address
         LA    R1,1(,R1)     Update to next CPU address
         STH   R1,CPU2ADR    Store next CPU address
         LA    R1,1(,R1)     Update to next CPU address
         STH   R1,CPU3ADR    Store next CPU address
*
* Set up prefixing for each of the three additional CPUs that will 
* perform floating point operations. Copy the first 8K to appropriate
* locations for each of the CPUs, modify the restart PSW, and issue 
* SIGP Set Prefix for each of them.
*
* Because the entirety of this program fits in less than 8K, prefixing
* is not difficult.
*
*                            Set Prefix and start CPU 1
         LH    R2,CPU1ADR    Get next CPU addresss
         LG    R1,=AD(CPU1PRE)   Get address of CPU1 Prefix area
         XR    R0,R0         Set address of real low core
         MVPG  R1,R0         Make a copy of low core
         SIGP  R1,R2,SIGPPREF Set prefix area for CPU 1
         MVC   RPSWADR(8,R1),=AD(CPU1BEG)  Update restart PSW in prefix
         XR    R1,R1         Zero SIGP parameter register
         SIGP  R0,R2,SIGPREST  Start next CPU using a Restart command
*
*                            Set Prefix and start CPU 2
         LH    R2,CPU2ADR    Get next CPU addresss
         LG    R1,=AD(CPU2PRE)   Get address of CPU1 Prefix area
         XR    R0,R0         Set address of real low core
         MVPG  R1,R0         Make a copy of low core
         SIGP  R1,R2,SIGPPREF Set prefix area for CPU 2
         MVC   RPSWADR(8,R1),=AD(CPU2BEG)  Update restart PSW in prefix
         XR    R1,R1         Zero SIGP parameter register
         SIGP  R0,R2,SIGPREST  Start next CPU using a Restart command
*
*                            Set Prefix and start CPU 3
         LH    R2,CPU3ADR    Get next CPU addresss
         LG    R1,=AD(CPU3PRE)   Get address of CPU1 Prefix area
         XR    R0,R0         Set address of real low core
         MVPG  R1,R0         Make a copy of low core
         SIGP  R1,R2,SIGPPREF Set prefix area for CPU 3
         MVC   RPSWADR(8,R1),=AD(CPU3BEG)  Update restart PSW in prefix
         XR    R1,R1         Zero SIGP parameter register
         SIGP  R0,R2,SIGPREST  Start next CPU using a Restart command
*
* Other processors started.  Now perform floating point operations.  
*
         STCTL R0,R0,CPU0CR0 Store CR0 to enable AFP
         OI    CPU0CR0+1,X'04' Turn on AFP bit
         LCTL  R0,R0,CPU0CR0 Reload updated CR0
*
* Initialize counter for loop control
*
         LARL  R12,RESAREA   Point to shared results area
         USING RESAREA,R12   Make results addressable
         L     R2,=F'10000000' Run loop 10,000,000 times
         XR    R3,R3         Zero count of loop iterations
         LR    R4,R3         Zero count of detected pre-emptions
         LR    R5,R3         Zero count of result errors
         LR    R6,R3         Zero count of FPCR contents errors
         BASR  R13,0         Set top of loop
*
* Top of loop.  Check for pre-emption, do floating point operation, 
* increment count of trips through loop.  
*
         CLC   LASTCPU,CPU0ADR  Has another CPU been dispatched
         BE    CPU0CALC      ..not that we can detect...do calc.
         MVC   LASTCPU,CPU0ADR  Update last dispatched CPU
         LA    R4,1(,R4)     Increment count of pre-emption detections
         ST    R4,CPU0PDET   Store updated count
*
CPU0CALC DS    0H            Perform floating point operation
         LFPC  CPU0FPCR      Reset FPCR to non-trap, RNTE
         LE    FPR2,CPU0OP1  Load operand one
         FIEBR FPR8,0,FPR2   Floating Point Integer
         STE   FPR8,CPU0ARES Store actual result
         STFPC CPU0AFPC      Store actual FPCR contents
*
         CLC   CPU0ARES,CPU0ER1  Did we get expected results?
         BE    CPU0CF1C      ..Yes, go check FPCR contents
         LA    R5,1(,R5)     Increment result error count
         ST    R5,CPU0RECT   Store updated result error count
         MVC   CPU0XRES,CPU0ARES  Save incorrect result
*
CPU0CF1C DS    0H
         CLC   CPU0AFPC,CPU0EF1  Did we get expected results?
         BE    CPU0OPNX      ..Yes, do next operation
         LA    R6,1(,R6)     Increment FPCR contents error count
         ST    R6,CPU0FECT   Store updated FPCR contents error count
         MVC   CPU0XFPC,CPU0AFPC  Save incorrect result
*
CPU0OPNX DS    0H            Do second floating point operation
         LFPC  CPU0FPCR      Reset FPCR to non-trap, RNTE
         LE    FPR2,CPU0OP2  Load operand one
         FIEBR FPR8,0,FPR2   Floating Point Integer
         STE   FPR8,CPU0ARES Store actual result
         STFPC CPU0AFPC      Store actual FPCR contents
*
         CLC   CPU0ARES,CPU0ER2  Did we get expected results?
         BE    CPU0CF2C      ..Yes, go check FPCR contents
         LA    R5,1(,R5)     Increment result error count
         ST    R5,CPU0RECT   Store updated result error count
         MVC   CPU0XRES,CPU0ARES  Save incorrect result
*
CPU0CF2C DS    0H
         CLC   CPU0AFPC,CPU0EF2  Did we get expected results?
         BE    CPU0OPDN      ..Yes, end of iteration
         LA    R6,1(,R6)     Increment FPCR contents error count
         ST    R6,CPU0FECT   Store updated FPCR contents error count
         MVC   CPU0XFPC,CPU0AFPC  Save incorrect result
*
CPU0OPDN DS    0H            FP op and result checks done
         LA    R3,1(,R3)     Increment loop iteration count
         ST    R3,CPU0CTR    Store updated loop iteration count
         BCTR  R2,R13        Perform next iteration
*
* Looping completed.   Stop other processors and load hardwait PSW.
*
         DROP  R12
         XR    R1,R1         Zero SIGP parameter register
*
         LH    R2,CPU1ADR    Get next CPU addresss
         SIGP  R0,R2,SIGPSTOP  Stop the CPU
*
         LH    R2,CPU2ADR    Get next CPU addresss
         SIGP  R0,R2,SIGPSTOP  Stop the CPU
*
         LH    R2,CPU3ADR    Get next CPU addresss
         SIGP  R0,R2,SIGPSTOP  Stop the CPU
*
         LTR   R14,R14       Return address provided?
         BNZR  R14           ..Yes, return to z/CMS test rig.  
         LPSWE WAITPSW       All done
*
* Load Floating Point Integer of 1.5, RNTE.  Expect 2.0 and inexact
*
CPU0FPCR DC    X'00000000'   FPCR, no traps, RNTE
*
CPU0OP1  DC    X'3FC00000'         +1.5
CPU0ER1  DC    X'40000000'         Expected result 2.0
CPU0EF1  DC    X'00080000'         Expected FPCR contents flag inexact
*
CPU0OP2  DC    X'3F800000'         +1.0
CPU0ER2  DC    X'3F800000'         Expected 1.0
CPU0EF2  DC    X'00000000'         Expected FPCR flag-free, RNTE
*
         EJECT
***********************************************************************
*
* CPU 1 program.  Start the next processor, enable Advanced Floating 
* Point, and repetitively perform a floating point operation until 
* this processor is stopped by CPU 0.
*
***********************************************************************
         SPACE 2
CPU1BEG  DS    0H            Start of processing for CPU zero
*
         STCTL R0,R0,CPU1CR0 Store CR0 to enable AFP
         OI    CPU1CR0+1,X'04' Turn on AFP bit
         LCTL  R0,R0,CPU1CR0 Reload updated CR0
*
* Perform repetitive operation in a loop
*
         LARL  R12,RESAREA   Point to shared results area
         USING RESAREA,R12   Make results addressable
         XR    R3,R3         Zero count of loop iterations
         LR    R4,R3         Zero count of detected pre-emptions
         LR    R5,R3         Zero count of result errors
         LR    R6,R3         Zero count of FPCR contents errors
         BASR  R13,0         Set top of loop
*
* Top of loop.  Check for pre-emption, do floating point operation, 
* increment count of trips through loop.  
*
         CLC   LASTCPU,CPU1ADR  Has another CPU been dispatched
         BE    CPU1CALC      ..not that we can detect...do calc.
         MVC   LASTCPU,CPU1ADR  Update last dispatched CPU
         LA    R4,1(,R4)     Increment count of pre-emption detections
         ST    R4,CPU1PDET   Store updated count
*
CPU1CALC DS    0H            Perform floating point operation
         LFPC  CPU1FPCR      Reset FPCR to non-trap, RM
         LE    FPR2,CPU1OP1  Load operand one
         FIEBR FPR8,0,FPR2   Convert to Floating Point integer
         STE   FPR8,CPU1ARES Store actual result
         STFPC CPU1AFPC      Store actual FPCR contents
*
         CLC   CPU1ARES,CPU1ER1  Did we get expected results?
         BE    CPU1CF1C      ..Yes, go check FPCR contents
         LA    R5,1(,R5)     Increment result error count
         ST    R5,CPU1RECT   Store updated loop iteration count
         MVC   CPU1XRES,CPU1ARES  Save incorrect result
*
CPU1CF1C DS    0H
         CLC   CPU1AFPC,CPU1EF1  Did we get expected results?
         BE    CPU1OPNX      ..Yes, end of iteration
         LA    R6,1(,R6)     Increment FPCR contents error count
         ST    R6,CPU1FECT   Store updated loop iteration count
         MVC   CPU1XFPC,CPU1AFPC  Save incorrect result
*
CPU1OPNX DS    0H            FP op and result checks done
         LFPC  CPU1FPCR      Reset FPCR to non-trap, RM
         LE    FPR2,CPU1OP2  Load operand one
         FIEBR FPR8,0,FPR2   Convert to Floating Point integer
         STE   FPR8,CPU1ARES Store actual result
         STFPC CPU1AFPC      Store actual FPCR contents
*
         CLC   CPU1ARES,CPU1ER2  Did we get expected results?
         BE    CPU1CF2C      ..Yes, go check FPCR contents
         LA    R5,1(,R5)     Increment result error count
         ST    R5,CPU1RECT   Store updated loop iteration count
         MVC   CPU1XRES,CPU1ARES  Save incorrect result
*
CPU1CF2C DS    0H
         CLC   CPU1AFPC,CPU1EF2  Did we get expected results?
         BE    CPU1OPDN      ..Yes, end of iteration
         LA    R6,1(,R6)     Increment FPCR contents error count
         ST    R6,CPU1FECT   Store updated loop iteration count
         MVC   CPU1XFPC,CPU1AFPC  Save incorrect result
*
CPU1OPDN DS    0H            FP op and result checks done
         LA    R3,1(,R3)     Increment count of loop iterations
         ST    R3,CPU1CTR    Store updated loop iteration count
         BR    R13           Perform next iteration until CPU stopped
*
         DROP  R12
*
* Load Floating Point Integer of 1.5, RZ.  Expect 1.0 and inexact
*
CPU1FPCR DC    X'00000001'   FPCR, no traps, RZ
*
CPU1OP1  DC    X'3FC00000'         +1.5
CPU1ER1  DC    X'3F800000'         Expected 1.0
CPU1EF1  DC    X'00080001'         Expected FPCR flag inexact, RM
*
CPU1OP2  DC    X'3F800000'         +1.0
CPU1ER2  DC    X'3F800000'         Expected 1.0
CPU1EF2  DC    X'00000001'         Expected FPCR flag-free, RM
*
         EJECT
***********************************************************************
*
* CPU 2 program.  Start the next processor, enable Advanced Floating 
* Point, and repetitively perform a floating point operation until 
* this processor is stopped by CPU 0.
*
***********************************************************************
         SPACE 2
CPU2BEG  DS    0H            Start of processing for CPU zero
*
         STCTL R0,R0,CPU2CR0 Store CR0 to enable AFP
         OI    CPU2CR0+1,X'04' Turn on AFP bit
         LCTL  R0,R0,CPU2CR0 Reload updated CR0
*
* Perform repetitive operation in a loop
*
         LARL  R12,RESAREA   Point to shared results area
         USING RESAREA,R12   Make results addressable
         XR    R3,R3         Zero count of loop iterations
         LR    R4,R3         Zero count of detected pre-emptions
         LR    R5,R3         Zero count of result errors
         LR    R6,R3         Zero count of FPCR contents errors
         BASR  R13,0         Set top of loop
*
* Top of loop.  Check for pre-emption, do floating point operation, 
* increment count of trips through loop.  
*
         CLC   LASTCPU,CPU2ADR  Has another CPU been dispatched
         BE    CPU2CALC      ..not that we can detect...do calc.
         MVC   LASTCPU,CPU2ADR  Update last dispatched CPU
         LA    R4,1(,R4)     Increment count of pre-emption detections
         ST    R4,CPU2PDET   Store updated count
*
CPU2CALC DS    0H            Perform floating point operation
         LFPC  CPU2FPCR      Reset FPCR to non-trap, RM
         LE    FPR2,CPU2OP1  Load operand one
         FIEBR FPR8,0,FPR2   Convert to Floating Point integer
         STE   FPR8,CPU2ARES Store actual result
         STFPC CPU2AFPC      Store actual FPCR contents
*
         CLC   CPU2ARES,CPU2ERES  Did we get expected results?
         BE    CPU2CFPC      ..Yes, go check FPCR contents
         LA    R5,1(,R5)     Increment result error count
         ST    R5,CPU2RECT   Store updated loop iteration count
         MVC   CPU2XRES,CPU2ARES  Save incorrect result
*
CPU2CFPC DS    0H
         CLC   CPU2AFPC,CPU2EFPC  Did we get expected results?
         BE    CPU2OPDN      ..Yes, end of iteration
         LA    R6,1(,R6)     Increment FPCR contents error count
         ST    R6,CPU2FECT   Store updated loop iteration count
         MVC   CPU2XFPC,CPU2AFPC  Save incorrect result
*
CPU2OPDN DS    0H            FP op and result checks done
         LA    R3,1(,R3)     Increment count of loop iterations
         ST    R3,CPU2CTR    Store updated loop iteration count
         BR    R13           Perform next iteration until CPU stopped
*
         DROP  R12
*
* Load Floating Point Integer of 1.5, RP.  Expect 2.0 and inexact
*
CPU2FPCR DC    X'00000002'   FPCR, no traps, RP
*
CPU2OP1  DC    X'3FC00000'         +1.5
*
CPU2ERES DC    X'40000000'         Expected 2.0
CPU2EFPC DC    X'00080002'         Expected FPCR flag inexact, RM
         EJECT
***********************************************************************
*
* CPU 3 program.  Start the next processor, enable Advanced Floating 
* Point, and repetitively perform a floating point operation until 
* this processor is stopped by CPU 0.
*
***********************************************************************
         SPACE 2
CPU3BEG  DS    0H            Start of processing for CPU zero
*
         STCTL R0,R0,CPU3CR0 Store CR0 to enable AFP
         OI    CPU3CR0+1,X'04' Turn on AFP bit
         LCTL  R0,R0,CPU3CR0 Reload updated CR0
*
* Perform repetitive operation in a loop
*
         LARL  R12,RESAREA   Point to shared results area
         USING RESAREA,R12   Make results addressable
         XR    R3,R3         Zero count of loop iterations
         LR    R4,R3         Zero count of detected pre-emptions
         LR    R5,R3         Zero count of result errors
         LR    R6,R3         Zero count of FPCR contents errors
         BASR  R13,0         Set top of loop
*
* Top of loop.  Check for pre-emption, do floating point operation, 
* increment count of trips through loop.  
*
         CLC   LASTCPU,CPU3ADR  Has another CPU been dispatched
         BE    CPU3CALC      ..not that we can detect...do calc.
         MVC   LASTCPU,CPU3ADR  Update last dispatched CPU
         LA    R4,1(,R4)     Increment count of pre-emption detections
         ST    R4,CPU3PDET   Store updated count
*
CPU3CALC DS    0H            Perform floating point operation
         LFPC  CPU3FPCR      Reset FPCR to non-trap, RM
         LE    FPR2,CPU3OP1  Load operand one
         FIEBR FPR8,0,FPR2   Convert to Floating Point integer
         STE   FPR8,CPU3ARES Store actual result
         STFPC CPU3AFPC      Store actual FPCR contents
*
         CLC   CPU3ARES,CPU3ERES  Did we get expected results?
         BE    CPU3CFPC      ..Yes, go check FPCR contents
         LA    R5,1(,R5)     Increment result error count
         ST    R5,CPU3RECT   Store updated loop iteration count
         MVC   CPU3XRES,CPU3ARES  Save incorrect result
*
CPU3CFPC DS    0H
         CLC   CPU3AFPC,CPU3EFPC  Did we get expected FPCR?
         BE    CPU3OPDN      ..Yes, end of iteration
         LA    R6,1(,R6)     Increment FPCR contents error count
         ST    R6,CPU3FECT   Store updated loop iteration count
         MVC   CPU3XFPC,CPU3AFPC  Save incorrect result
*
CPU3OPDN DS    0H            FP op and result checks done
         LA    R3,1(,R3)     Increment count of iterations
         ST    R3,CPU3CTR    Store updated loop iteration count
         BR    R13           Perform next iteration until CPU stopped
         DROP  R12
*
* Load Floating Point Integer of 1.0, RM.  Expect 1.0 and no flags
*
CPU3FPCR DC    X'00000003'   FPCR, no traps, RP
*
CPU3OP1  DC    X'3F800000'         +1.0
*
CPU3ERES DC    X'3F800000'         Expected 1.0
CPU3EFPC DC    X'00000003'         Expected FPCR flag-free, RP
         SPACE 3
         LTORG
         EJECT
*
* Locations for results.  Because all four threads use these 
* values, they must not be in the area affected by prefixing.  
*
         ORG   STRTLABL+X'2000'  Shared tables.
RESAREA  DS    0D            Start of results area
*
*  Loop iteration counter for each CPU
*
CPU0CTR  DC    F'0'
CPU1CTR  DC    F'0'
CPU2CTR  DC    F'0'
CPU3CTR  DC    F'0'
*
*  Pre-emption detection counter for each CPU
*
CPU0PDET DC    F'0'
CPU1PDET DC    F'0'
CPU2PDET DC    F'0'
CPU3PDET DC    F'0'
*
*  Result error detection counter for each CPU
*
CPU0RECT DC    F'0'
CPU1RECT DC    F'0'
CPU2RECT DC    F'0'
CPU3RECT DC    F'0'
*
*  FPCR contents error detection counter for each CPU
*
CPU0FECT DC    F'0'
CPU1FECT DC    F'0'
CPU2FECT DC    F'0'
CPU3FECT DC    F'0'
*
* Actual Results
*
CPU0ARES DC    X'00000000'         Actual results
CPU0AFPC DC    X'00000000'         Actual FPCR contents
*
CPU1ARES DC    X'00000000'         Actual results
CPU1AFPC DC    X'00000000'         Actual FPCR contents
*
CPU2ARES DC    X'00000000'         Actual results
CPU2AFPC DC    X'00000000'         Actual FPCR contents
*
CPU3ARES DC    X'00000000'         Actual results
CPU3AFPC DC    X'00000000'         Actual FPCR contents
*
* Error Results
*
CPU0XRES DC    X'00000000'         Last error results
CPU0XFPC DC    X'00000000'         Last error FPCR contents
*
CPU1XRES DC    X'00000000'         Last error results
CPU1XFPC DC    X'00000000'         Last error FPCR contents
*
CPU2XRES DC    X'00000000'         Last error results
CPU2XFPC DC    X'00000000'         Last error FPCR contents
*
CPU3XRES DC    X'00000000'         Last error results
CPU3XFPC DC    X'00000000'         Last error FPCR contents
*
* Following shared variable used for preemption detection.  Cannot be
* in the prefixed area, lest each CPU have its own copy.
*
LASTCPU  DC    H'0'          Address of last detected dispatched CPU
*
ENDLABL  EQU   STRTLABL+X'2200'
         PADCSECT ENDLABL
         END
