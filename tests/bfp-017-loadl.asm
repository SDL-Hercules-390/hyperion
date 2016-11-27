  TITLE 'bfp-017-loadl.asm: Test IEEE Load Lengthened'
***********************************************************************
*
*Testcase IEEE LOAD LENGTHENED
*  Test case capability includes IEEE exceptions trappable and 
*  otherwise. Test results, FPCR flags, and any DXC are saved for all 
*  tests.  Load Lengthened does not set the condition code.
*
***********************************************************************
         SPACE 2
***********************************************************************
*
*                        bfp-017-loadl.asm 
*
*        This assembly-language source file is part of the
*        Hercules Decimal Floating Point Validation Package 
*                        by Stephen R. Orso
*
* Copyright 2016 by Stephen R Orso.  All rights reserved.
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
* Tests the following three conversion instructions
*   LOAD LENGTHENED (short BFP, RRE)
*   LOAD LENGTHENED (long BFP, RRE) 
*   LOAD LENGTHENED (extended BFP, RRE)  
*   LOAD LENGTHENED (short BFP, RXE)
*   LOAD LENGTHENED (long BFP, RXE) 
*   LOAD LENGTHENED (extended BFP, RXE)  
* 
* Test data is compiled into this program.  The test script that runs
* this program can provide alternative test data through Hercules R 
* commands.
* 
* Test Case Order
* 1) Short to log BFP non-finite tests, non-trap and trap
* 2) Short to log BFP finite tests, non-trappable
* 3) Long to extended BFP non-finite tests, non-trap and trap
* 4) Long to extended BFP finite tests, non-trappable
* 5) Short to extended BFP non-finite tests, non-trap and trap
* 6) Short to extended BFP finite tests, non-trappable
*
* Two input test data sets are provided, one each for short and long
*   precision BFP inputs.  The same short BFP inputs are used for 
*   short to long testing and short to extended testing.
*
* Also tests the following floating point support instructions
*   LOAD  (Short)
*   LOAD  (Long)
*   LFPC  (Load Floating Point Control Register)
*   STORE (Short)
*   STORE (Long)
*   STFPC (Store Floating Point Control Register)
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
*
BFPLDLEN START 0                   Load Lengthened Testing
STRTLABL EQU   *
R0       EQU   0                   Work register for cc extraction
R1       EQU   1                   Available
R2       EQU   2                   Holds count of test input values
R3       EQU   3                   Points to next test input value(s)
R4       EQU   4                   Available
R5       EQU   5                   Available
R6       EQU   6                   Available
R7       EQU   7                   Pointer to next result value(s)
R8       EQU   8                   Pointer to next FPCR result
R9       EQU   9                   Available
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
         USING *,R15
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
         DC    X'0000000180000000',AD(START)   
*
         ORG   STRTLABL+X'1D0'     z/Arch Program check old PSW
         DC    X'0000000000000000',AD(PROGCHK)
* 
* Program check routine.  If Data Exception, continue execution at
* the instruction following the program check.  Otherwise, hard wait.
* No need to collect data.  All interesting DXC stuff is captured
* in the FPCR.
*
PROGCHK  DS    0H             Program check occured...
         CLI   PCINTCD+1,X'07'  Data Exception?
         JNE   PCNOTDTA       ..no, hardwait (not sure if R15 is ok)
         LPSWE PCOLDPSW       ..yes, resume program execution
PCNOTDTA DS    0H
         LTR   R14,R14        Return address provided?
         BNZR  R14            Yes, return to z/CMS test rig.  
         LPSWE HARDWAIT       Not data exception, enter disabled wait.
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
*
         LA    R10,SHORTNF   Point to short BFP non-finite test data
         BAS   R13,LDEBRNF   Convert short BFP to long BFP
         LA    R10,SHORTF    Point to short BFP finite test data
         BAS   R13,LDEBRF    Convert short BFP to long BFP
*
         LA    R10,LONGNF    Point to long BFP non-finite test data
         BAS   R13,LXDBRNF   Convert long BFP to integer long BFP
         LA    R10,LONGF     Point to long BFP finite test data
         BAS   R13,LXDBRF    Convert using all rounding mode options
*
         LA    R10,XTNDNF    Point to short BFP non-finite test data
         BAS   R13,LXEBRNF   Convert short BFP to extended BFP
         LA    R10,XTNDF     Point to short BFP finite test data
         BAS   R13,LXEBRF    Convert short BFP to extended BFP
*
         LTR   R14,R14       Return address provided?
         BNZR  R14           ..Yes, return to z/CMS test rig.  
         LPSWE WAITPSW       All done
*
         DS    0D            Ensure correct alignment for psw
WAITPSW  DC    X'0002000000000000',AD(0)  Normal end - disabled wait
HARDWAIT DC    X'0002000000000000',XL6'00',X'DEAD' Abnormal end
*
CTLR0    DS    F
FPCREGNT DC    X'00000000'  FPCR, trap all IEEE exceptions, zero flags
FPCREGTR DC    X'F8000000'  FPCR, trap no IEEE exceptions, zero flags
*
* Input values parameter list, four fullwords: 
*      1) Count, 
*      2) Address of inputs, 
*      3) Address to place results, and
*      4) Address to place DXC/Flags/cc values.  
*
         ORG   STRTLABL+X'300'
SHORTNF  DS    0F            Short to long BFP non-finite inputs
         DC    A(SBFPNFCT)
         DC    A(SBFPNFIN)
         DC    A(SBFPNFOT)
         DC    A(SBFPNFFL)
*
SHORTF   DS    0F            Short to long BFP finite inputs
         DC    A(SBFPFCT)
         DC    A(SBFPFIN)
         DC    A(SBFPFOT)
         DC    A(SBFPFOF)
*
LONGNF   DS    0F            Long to extended BFP non-finite inputs
         DC    A(LBFPNFCT)
         DC    A(LBFPNFIN)
         DC    A(LBFPNFOT)
         DC    A(LBFPNFFL)
*
LONGF    DS    0F            Long to extended BFP finite inputs
         DC    A(LBFPFCT)
         DC    A(LBFPFIN)
         DC    A(LBFPFOT)
         DC    A(LBFPFOF)
*
XTNDNF   DS    0F            Short to extended BFP non-finite inputs
         DC    A(SBFPNFCT)
         DC    A(SBFPNFIN)
         DC    A(XBFPNFOT)
         DC    A(XBFPNFFL)
*
XTNDF    DS    0F            Short to extended BFP finite inputs
         DC    A(SBFPFCT)
         DC    A(SBFPFIN)
         DC    A(XBFPFOT)
         DC    A(XBFPFOF)
         EJECT
***********************************************************************
*
* Load Lengthened Short to Long non-finite tests
*
* Lengthen short BFP inputs to long BFP.  Two pairs of results are
* generated, one pair for RRE and one for RXE.  Each pair consists of
* a result with all exceptions non-trappable and a second with all
* exceptions trappable.   The FPCR is stored for each result.  The
* Condition Code in not changed by Load Lengthened and is not stored.  
*
***********************************************************************
         SPACE 2
LDEBRNF  DS    0H            Convert short BFP inputs to long BFP
         LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR8,0(,R3)   Get short BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         LDEBR FPR1,FPR8     Lengthen short in FPR8 to long in FPR1
         STD   FPR1,0(,R7)   Store long BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LZER  FPR1          Eliminate any residual results
         LDEBR FPR1,FPR8     Lengthen short in FPR8 to long in FPR1
         STD   FPR1,8(,R7)   Store long BFP result
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LDEB  FPR1,0(,R3)   Lengthen short BFP to long in FPR1
         STD   FPR1,16(,R7)  Store long BFP result
         STFPC 8(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LZER  FPR1          Eliminate any residual results
         LDEB  FPR1,0(,R3)   Lengthen short BFP to long in FPR1
         STD   FPR1,24(,R7)  Store long BFP result
         STFPC 12(R8)        Store resulting FPCR flags and DXC
*
         LA    R3,4(,R3)     Point to next input value
         LA    R7,4*8(,R7)   Point to next long BFP result set
         LA    R8,4*4(,R8)   Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Load Lengthened Short to Long finite tests
*
* Convert short BFP to integer BFP using each possible rounding mode.
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPCR with
* the IEEE Inexact exception supressed.  SRNM (2-bit) is used  for
* the first two FPCR-controlled tests and SRNMB (3-bit) is used for
* the last two To get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section;
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
LDEBRF   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR8,0(,R3)    Get short BFP test value
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LDEBR FPR1,FPR8     Lengthen short in FPR8 to long in FPR1
         STD   FPR1,0(,R7)   Store long BFP result
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LZER  FPR1          Eliminate any residual results
         LDEB  FPR1,0(,R3)   Lengthen short in FPR8 to long in FPR1
         STD   FPR1,8(,R7)   Store long BFP result
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LA    R3,4(,R3)     Point to next input value
         LA    R7,2*8(,R7)   Point to next long BFP result set
         LA    R8,2*4(,R8)   Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Load Lengthened Long to Extended non-finite tests
*
* Lengthen long BFP inputs to extended BFP.  Two pairs of results are
* generated, one pair for RRE and one for RXE.  Each pair consists of
* a result with all exceptions non-trappable and a second with all
* exceptions trappable.   The FPCR is stored for each result.  The
* Condition Code in not changed by Load Lengthened and is not stored.  
*
***********************************************************************
         SPACE 2
LXDBRNF  LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR8,0(,R3)   Get long BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         LXDBR FPR1,FPR8     Lengthen long in FPR8 to extended in FPR1
         STD   FPR1,0(,R7)   Store extended BFP result part 1
         STD   FPR3,8(,R7)   Store extended BFP result part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LZXR  FPR1          Eliminate any residual results
         LXDBR FPR1,FPR8     Lengthen long in FPR8 to extended in FPR1
         STD   FPR1,16(,R7)  Store extended BFP result part 1
         STD   FPR3,24(,R7)  Store extended BFP result part 2
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LXDB  FPR1,0(,R3)   Lengthen long BFP to extended in FPR1-3
         STD   FPR1,32(,R7)  Store extended BFP result part 1
         STD   FPR3,40(,R7)  Store extended BFP result part 2
         STFPC 8(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LZXR  FPR1          Eliminate any residual results
         LXDB  FPR1,0(,R3)   Lengthen long BFP to extended in FPR1-3
         STD   FPR1,48(,R7)  Store extended BFP result part 1
         STD   FPR3,56(,R7)  Store extended BFP result part 2
         STFPC 12(R8)        Store resulting FPCR flags and DXC
*
         LA    R3,8(,R3)     Point to next input value
         LA    R7,4*16(,R7)  Point to next extended BFP result set
         LA    R8,4*4(,R8)   Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert long BFP to integers using each possible rounding mode. 
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPCR with
* the IEEE Inexact exception supressed.  SRNM (2-bit) is used  for
* the first two FPCR-controlled tests and SRNMB (3-bit) is used for
* the last two To get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section;
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
LXDBRF   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LD    FPR8,0(,R3)   Get long BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         LXDBR FPR1,FPR8     Lengthen long in FPR8 to extended in FPR1
         STD   FPR1,0(,R7)   Store extended BFP result part 1
         STD   FPR3,8(,R7)   Store extended BFP result part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LXDB  FPR1,0(,R3)   Lengthen long BFP to extended in FPR1-3
         STD   FPR1,16(,R7)  Store extended BFP result part 1
         STD   FPR3,24(,R7)  Store extended BFP result part 2
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LA    R3,8(,R3)     Point to next input value
         LA    R7,2*16(,R7)  Point to next extended BFP result set
         LA    R8,2*4(,R8)   Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Load Lengthened Short to Extended non-finite tests
*
* Lengthen short BFP inputs to extended BFP.  Two pairs of results are
* generated, one pair for RRE and one for RXE.  Each pair consists of
* a result with all exceptions non-trappable and a second with all
* exceptions trappable.   The FPCR is stored for each result.  The
* Condition Code in not changed by Load Lengthened and is not stored.  
*
***********************************************************************
         SPACE 2
LXEBRNF  LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR8,0(,R3)   Get long BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         LXEBR FPR1,FPR8     Lengthen long in FPR8 to extended in FPR1
         STD   FPR1,0(,R7)   Store extended BFP result part 1
         STD   FPR3,8(,R7)   Store extended BFP result part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LZXR  FPR1          Eliminate any residual results
         LXEBR FPR1,FPR8     Lengthen long in FPR8 to extended in FPR1
         STD   FPR1,16(,R7)  Store extended BFP result part 1
         STD   FPR3,24(,R7)  Store extended BFP result part 2
         STFPC 4(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LXEB  FPR1,0(,R3)   Lengthen long BFP to extended in FPR1-3
         STD   FPR1,32(,R7)  Store extended BFP result part 1
         STD   FPR3,40(,R7)  Store extended BFP result part 2
         STFPC 8(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGTR      Set exceptions trappable
         LZXR  FPR1          Eliminate any residual results
         LXEB  FPR1,0(,R3)   Lengthen long BFP to extended in FPR1-3
         STD   FPR1,48(,R7)  Store extended BFP result part 1
         STD   FPR3,56(,R7)  Store extended BFP result part 2
         STFPC 12(R8)         Store resulting FPCR flags and DXC
*
         LA    R3,4(,R3)     Point to next input value
         LA    R7,4*16(,R7)  Point to next extended BFP result set
         LA    R8,4*4(,R8)   Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Convert extended BFP to integers using each possible rounding mode.
* Ten test results are generated for each input.  A 48-byte test result
* section is used to keep results sets aligned on a quad-double word.
*
* The first four tests use rounding modes specified in the FPCR with
* the IEEE Inexact exception supressed.  SRNM (2-bit) is used  for
* the first two FPCR-controlled tests and SRNMB (3-bit) is used for
* the last two To get full coverage of that instruction pair.  
*
* The next six results use instruction-specified rounding modes.  
*
* The default rounding mode (0 for RNTE) is not tested in this section;
* prior tests used the default rounding mode.  RNTE is tested
* explicitly as a rounding mode in this section.  
*
***********************************************************************
         SPACE 2
LXEBRF   LM    R2,R3,0(R10)  Get count and address of test input values
         LM    R7,R8,8(R10)  Get address of result area and flag area.
         LTR   R2,R2         Any test cases?
         BZR   R13           ..No, return to caller
         BASR  R12,0         Set top of loop
*
         LE    FPR8,0(,R3)   Get long BFP test value
         LFPC  FPCREGNT      Set exceptions non-trappable
         LXEBR FPR1,FPR8     Lengthen long in FPR8 to extended in FPR1
         STD   FPR1,0(,R7)   Store extended BFP result part 1
         STD   FPR3,8(,R7)   Store extended BFP result part 2
         STFPC 0(R8)         Store resulting FPCR flags and DXC
*
         LFPC  FPCREGNT      Set exceptions non-trappable
         LXEB  FPR1,0(,R3)   Lengthen long BFP to extended in FPR1-3
         STD   FPR1,16(,R7)  Store extended BFP result part 1
         STD   FPR3,24(,R7)  Store extended BFP result part 2
         STFPC 8(R8)         Store resulting FPCR flags and DXC
*
         LA    R3,4(,R3)     Point to next input value
         LA    R7,2*16(,R7)  Point to next extended BFP result set
         LA    R8,2*4(,R8)   Point to next FPCR result area
         BCTR  R2,R12        Convert next input value.  
         BR    R13           All converted; return.
         EJECT
***********************************************************************
*
* Short integer inputs for Load Lengthened testing.  These values
* correspond to the data classes listed in Figure 19-17 on page 19-21
* of SA22-7832-10. The values are tested with and without traps
* enabled.  
*
* The same values are used for long and extended result creation.  
*
***********************************************************************
         SPACE 2
SBFPNFIN DS    0F              Inputs for short BFP non-finite testing
         DC    X'FF800000'         -Infinity
         DC    X'BF800000'         -1.0
         DC    X'80000000'         -0.0
         DC    X'00000000'         +0.0
         DC    X'3F800000'         +1.0
         DC    X'7F800000'         -Infinity
         DC    X'7FC10000'         QNaN
         DC    X'7F820000'         SNaN
SBFPNFCT EQU   (*-SBFPNFIN)/4  Count of short BFP in list
         SPACE 3
***********************************************************************
*
* Short integer inputs for Load Lengthened testing.  These values
* functionally test Load Lengthened operations including preservation 
* of all signficant bits in the result and renormalization of subnormal
* (tiny) values.  
*
* The same values are used for long and extended result creation.  
*
***********************************************************************
         SPACE 2
SBFPFIN  DS    0F              Inputs for short BFP finite testing
         DC    X'FF7FFFFF'         Maximum -normal, all bits test
         DC    X'804FFFFF'         maximum -tiny, all bits test
         DC    X'80000001'         minimum -tiny
         DC    X'BDCCCCCD'         -0.1 rounded away from zero
         DC    X'BDCCCCCC'         -0.1 rounded toward zero
         DC    X'3DCCCCCC'         +0.1 rounded toward zero
         DC    X'3DCCCCCD'         +0.1 rounded away from zero
         DC    X'00000001'         minimum +tiny
         DC    X'004FFFFF'         maximum +tiny, all bits test
         DC    X'7F7FFFFF'         maximum +normal, all bits test
SBFPFCT  EQU   (*-SBFPFIN)/4   Count of short BFP in list 
*
***********************************************************************
*
* Long integer inputs for Load Lengthened testing.  These values
* correspond to the data classes listed in Figure 19-17 on page 19-21
* of SA22-7832-10. The values are tested with and without traps
* enabled.  
*
***********************************************************************
         SPACE 2
LBFPNFIN DS    0D              Inputs for long BFP non-finite testing
         DC    X'FFF0000000000000'         -Infinity
         DC    X'BFF0000000000000'         -1.0
         DC    X'8000000000000000'         -0.0
         DC    X'0000000000000000'         +0.0
         DC    X'3FF0000000000000'         +1.0
         DC    X'7FF0000000000000'         -Infinity
         DC    X'7FF8100000000000'         +QNaN
         DC    X'7FF0200000000000'         +SNaN
LBFPNFCT EQU   (*-LBFPNFIN)/8  Count of long BFP in list
         SPACE 3
***********************************************************************
*
* Long integer inputs for Load Lengthened testing.  These values
* functionally test Load Lengthened operations including preservation 
* of all signficant bits in the result and renormalization of subnormal
* (tiny) values.  
*
***********************************************************************
         SPACE 2
LBFPFIN  DS    0F              Inputs for long BFP finite testing
         DC    X'7FFFFFFFFFFFFFFF'    Maximum -normal, all bits test
         DC    X'800FFFFFFFFFFFFF'    Maximum -tiny, all bits test
         DC    X'8000000000000001'    Minimum -tiny
         DC    X'BFB999999999999A'    -0.1 rounded away from zero
         DC    X'BFB9999999999999'    -0.1 rounded toward zero
         DC    X'3FB9999999999999'    +0.1 rounded toward zero
         DC    X'3FB999999999999A'    +0.1 rounded away from zero
         DC    X'0000000000000001'    Minimum +tiny
         DC    X'000FFFFFFFFFFFFF'    Maximum +tiny, all bits test
         DC    X'7FFFFFFFFFFFFFFF'    Maximum +normal, all bits test
LBFPFCT  EQU   (*-LBFPFIN)/8   Count of long BFP rounding tests * 8
*
*  Locations for results
*
*  Results from short -> long Load Lengthened
*
SBFPNFOT EQU   STRTLABL+X'1000'    Long BFP non-finite results
*                                  ..8 used, room for 8
SBFPNFFL EQU   STRTLABL+X'1100'    FPCR flags and DXC from long BFP
*                                  ..8 used, room for 16 
SBFPFOT  EQU   STRTLABL+X'1200'    Long BFP finite test results
*                                  ..10 used, room for 16
SBFPFOF  EQU   STRTLABL+X'1300'    Long BFP finite FPCR results
*                                  ..10 used, room for 16
*                                   .. next assignment X'1400'
*
*  Results from long -> extended Load Lengthened
*
LBFPNFOT EQU   STRTLABL+X'2000'    Extended BFP non-finite results
*                                  ..8 used, room for 8 
LBFPNFFL EQU   STRTLABL+X'2200'    FPCR flags and DXC from extended BFP
*                                  ..8 used, room for 16
LBFPFOT  EQU   STRTLABL+X'2300'    Extended BFP non-finite test results
*                                  ..10 used, room for 16
LBFPFOF  EQU   STRTLABL+X'2500'    Extended BFP non-finite FPCR results
*                                  ..10 used, room for 16
*                                   .. next assignment X'2600'
*
*  Results from short -> extended Load Lengthened
*
XBFPNFOT EQU   STRTLABL+X'3000'    Extended BFP rounded results
*                                  ..8 used, room for 8
XBFPNFFL EQU   STRTLABL+X'3200'    FPCR flags and DXC from extended BFP
*                                  ..8 used, room for 16
XBFPFOT  EQU   STRTLABL+X'3300'    Extd BFP rounding mode test results
*                                  ..10 used, room for 16
XBFPFOF  EQU   STRTLABL+X'3500'    Extd BFP rounding mode FPCR results
*                                  ..10 used, room for 16
*                                   .. next assignment X'3500'
*
*
ENDLABL  EQU   STRTLABL+X'3600'
*  Pad CSECT if not running on ASMA for a stand-alone environment
         PADCSECT ENDLABL
         END
