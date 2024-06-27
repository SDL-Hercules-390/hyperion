 TITLE '                Test S/390 and z/Arch STORAGE KEY Instructions'
***********************************************************************
*
*                            SKEY390Z
*
***********************************************************************
*
*   This program verifies proper functioning of the following
*   System/390 and z/Architecture Storage Key instructions:
*
*       ISKE, IVSK, RRBE, SSKE, TB, TPROT     (both S/390 & z/Arch)
*       IRBM, RRBM, PFMF                      (z/Architecture only)
*
*   NOTE: due to varying support for certain instructions under
*   certain situations, some tests may crash at certain points.
*   If the crash is expected, then the crash is ignored and the
*   test that was being attempted is simply skipped.
*
*   PLEASE ALSO NOTE the program is purposely designed to branch to
*   an odd address should any test fail (such as the condition code
*   not being the expected value). The Program Check handler routine
*   when it notices the Program Old PSW is an odd address, backs up
*   the address by 5 bytes and uses that as the test's failing PSW.
*
*   Thus when any test fails, the disabled wait PSW points directly
*   to the failing instruction (i.e. the branch following the failed
*   comparison). ALSO NOTE that Hercules also issues a "Instruction
*   fetch error" message to its hardware console too whenever this
*   occurs (due to the PSW address being odd causing it to be unable
*   to fetch the next instruction), which is expected.
*
*   FINALLY, in order to support successfully running on non-Hercules
*   systems, we utilize the Hercules "CPUVERID xx FORCE" statement
*   to allow us to detect if we're running under Hercules. On "real
*   iron" (including  zPDT and RD&T) the CPUID "Version code" (which
*   they're now calling the "Environment" field) will be either 00,
*   C1 or D3, whereas on Hercules it will be the value specified on
*   your "CPUVERID xx FORCE" statement (which is C8 for SKEY390Z).
*
***********************************************************************
                                                                EJECT
***********************************************************************
*                         LOW CORE
***********************************************************************
                                                                SPACE 2
TEST     START 0
         USING TEST,0                 Use absolute addressing
                                                                SPACE
         ORG   TEST+X'00'             S/390 Restart new PSW
         DC    XL4'00080000'          S/390 Restart new PSW
         DC    A(BEGIN)               S/390 Restart new PSW
                                                                SPACE
         ORG   TEST+X'28'             S/390 Program old PSW
PGMOLD   EQU   *                      S/390 Program old PSW
                                                                SPACE
         ORG   TEST+X'68'             S/390 Program new PSW
         DC    XL4'00080000'          S/390 Program new PSW
         DC    A(PGMCHK)              S/390 Program new PSW
                                                                SPACE 3
         ORG   TEST+X'8C'             Program interrupt code
PGMCODE  DC    F'0'                   Program interrupt code
                                                                SPACE
PGM_OPERATION_EXCEPTION     EQU       X'0001'
PGM_SPECIFICATION_EXCEPTION EQU       X'0006'
                                                                SPACE 3
         ORG   TEST+X'150'            z/Arch Program OLD PSW
ZPGMOLD  EQU   *                      z/Arch Program OLD PSW
                                                                SPACE
         ORG   TEST+X'1A0'            z/Arch Restart new PSW
         DC    XL4'00000001'          z/Arch Restart new PSW
         DC    XL4'80000000'          z/Arch Restart new PSW
         DC    XL4'00000000'          z/Arch Restart new PSW
         DC    A(ZARCH)               z/Arch Restart new PSW
                                                                SPACE
         ORG   TEST+X'1D0'            z/Arch Program new PSW
         DC    XL4'00000001'          z/Arch Program new PSW
         DC    XL4'80000000'          z/Arch Program new PSW
         DC    XL4'00000000'          z/Arch Program new PSW
         DC    A(ZPGMCHK)             z/Arch Program new PSW
                                                                EJECT
***********************************************************************
*                       INITIALIZATION
***********************************************************************
                                                                SPACE 2
         ORG   TEST+X'200'    Start of test program
                                                                SPACE 3
BEGIN    STIDP CPUID          Save CPU ID (for later test for VM)
*
*       The below STFL will fail with an Operation Exception on those
*       operating systems that were not written with support for the
*       'N3' series of instructions (i.e. z/Architecture instructions
*       backported to ESA/390, such as the STFL instruction itself),
*       such as VM/ESA (which doesn't support 'N3' instructions).
*
         STFL  0              Store Facility List (just for fun)
STFLPC   MVC   STFL390,X'C8'  Save STFL results for posterity
                                                                SPACE 3
*        Fall through to begin S/390 mode tests.......
                                                                SPACE 4
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             |               |              |
*             V               V              V
                                                                EJECT
***********************************************************************
*                   SSKE, ISKE, RRBE (390 mode)
***********************************************************************
                                                                SPACE
         ICM   R1,B'0001',=X'1C'
         L     R2,=A((4*_4K)+X'900')
         SSKE  R1,R2
         ICM   R1,B'0001',=X'26'
         L     R2,=A((5*_4K)+X'A00')
         SSKE  R1,R2
         ICM   R1,B'0001',=X'4E'
         L     R2,=A((6*_4K)+X'B00')
         SSKE  R1,R2
*****************************************
         L     R2,=A((4*_4K)+X'900')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'1C'
         BNE   *+1
         L     R2,=A((5*_4K)+X'A00')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'26'
         BNE   *+1
         L     R2,=A((6*_4K)+X'B00')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'4E'
         BNE   *+1
*****************************************
         L     R2,=A((4*_4K)+X'900')
         RRBE  R0,R2
         BC    B'1101',*+1              NOT CC2 = was REF 1, CHG 0
         L     R2,=A((5*_4K)+X'A00')
         RRBE  R0,R2
         BC    B'1110',*+1              NOT CC3 = was REF 1, CHG 1
         L     R2,=A((6*_4K)+X'B00')
         RRBE  R0,R2
         BC    B'1110',*+1              NOT CC3 = was REF 1, CHG 1
*****************************************
         L     R2,=A((4*_4K)+X'900')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'18'
         BNE   *+1
         L     R2,=A((5*_4K)+X'A00')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'22'
         BNE   *+1
         L     R2,=A((6*_4K)+X'B00')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'4A'
         BNE   *+1
                                                                EJECT
***********************************************************************
*                   IVSK, TPROT, TB (390 mode)
***********************************************************************
                                                                SPACE
         ICM   R1,B'0001',=X'6E'
         L     R2,=A((7*_4K)+X'900')
         SSKE  R1,R2
*  The below could fail on systems with "ESA/390 Compatibility
*  Mode" installed/enabled, which does not support ESA/390 DAT.
         LCTL  R0,R1,CR0_1_39           Configure DAT
         SSM   =X'04'                   Enable DAT
SSMPC    IVSK  R1,R2
         SSM   =X'00'                   Disable DAT
         CLM   R1,B'0001',=X'68'
         BNE   *+1
*****************************************
SKIPIVSK ICM   R1,B'0001',=X'10'
         L     R2,=A(4*_4K)
         SSKE  R1,R2
         L     R1,=A((4*_4K)+X'900')
         ICM   R2,B'0001',=X'10'
         TPROT 0(R1),0(R2)
         BC    B'0111',*+1              NOT CC0 = FETCH OK, STORE OK
         ICM   R2,B'0001',=X'20'
         TPROT 0(R1),0(R2)
         BC    B'1011',*+1              NOT CC1 = FETCH OK, STORE NO
         ICM   R1,B'0001',=X'18'        (set fetch protect)
         L     R2,=A(4*_4K)
         SSKE  R1,R2
         L     R1,=A(4*_4K)
         ICM   R2,B'0001',=X'20'
         TPROT 0(R1),0(R2)
         BC    B'1101',*+1              NOT CC2 = FETCH NO, STORE NO
*  We must skip the 'TB' (Test Block) test if we're running under
*  VM or are NOT running under Hercules since neither environment
*  has any command to sets block(s) of storage to "unusuable".
         CLI   CPUID,X'FF'              Are we running under VM?
         BE    SKIPTB39                 Yes, then skip 'TB' tests
         CLI   CPUID,X'C8'              Are we running under Hercules?
         BNE   SKIPTB39                 No, then skip 'TB' tests
         SLR   R0,R0                    Required by TB instruction
         L     R2,=A((10*_4K)+X'DEF')   Requires Herc 'f- A000' cmd
         TB    R1,R2
         BC    B'1011',*+1              NOT CC1 = Unusable/BAD block
         SLR   R0,R0                    Required by TB instruction
         L     R2,=A((11*_4K)+X'FED')   Requires Herc 'f- B000' cmd
         TB    R1,R2
         BC    B'1011',*+1              NOT CC1 = Unusable/BAD block
SKIPTB39 EQU   *
                                                                EJECT
                                                                SPACE 2
***********************************************************************
*                 Switch to z/Architecture mode...
***********************************************************************
                                                                SPACE 2
         SLR   R0,R0                Start clean
         LA    R1,1                 Request z/Arch mode
         SLR   R2,R2                Start clean
         SLR   R3,R3                Start clean
                                                                SPACE
         SIGP  R0,R2,X'12'          Request z/Arch mode
         BE    ZARCH                Success! Begin z/Arch tests
                                                                SPACE
         LPSW  GOODPSW              No z/Arch? Then we're done!
                                                                SPACE
GOODPSW  DC    0D'0',XL4'000A0000'  S/390 SUCCESS disabled wait PSW
         DC    A(0)                 S/390 SUCCESS disabled wait PSW
                                                                SPACE 4
ZARCH    STIDP CPUID                Save CPU ID (for later test for VM)
         STFL  0                    Store Facility List (just for fun)
         MVC   STFLZ,X'C8'          Save STFL results for posterity
         LA    R0,(L'FACLIST/8)-1   Store Facility List Extended
         STFLE FACLIST              Store Facility List Extended
                                                                SPACE 2
*        Fall through to begin z/Architecture mode tests...
                                                                SPACE 3
*                |               |              |
*                |               |              |
*                |               |              |
*                |               |              |
*                |               |              |
*                |               |              |
*                |               |              |
*                |               |              |
*                |               |              |
*                |               |              |
*                |               |              |
*                |               |              |
*                |               |              |
*                |               |              |
*                |               |              |
*                V               V              V
                                                                EJECT
***********************************************************************
*                   SSKE, ISKE, RRBE (z/Arch mode)
***********************************************************************
                                                                SPACE
         ICM   R1,B'0001',=X'1C'
         L     R2,=A((4*_4K)+X'900')
         SSKE  R1,R2
         ICM   R1,B'0001',=X'26'
         L     R2,=A((5*_4K)+X'A00')
         SSKE  R1,R2
         ICM   R1,B'0001',=X'4E'
         L     R2,=A((6*_4K)+X'B00')
         SSKE  R1,R2
*****************************************
         L     R2,=A((4*_4K)+X'900')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'1C'
         BNE   *+1
         L     R2,=A((5*_4K)+X'A00')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'26'
         BNE   *+1
         L     R2,=A((6*_4K)+X'B00')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'4E'
         BNE   *+1
*****************************************
         L     R2,=A((4*_4K)+X'900')
         RRBE  R0,R2
         BC    B'1101',*+1              NOT CC2 = was REF 1, CHG 0
         L     R2,=A((5*_4K)+X'A00')
         RRBE  R0,R2
         BC    B'1110',*+1              NOT CC3 = was REF 1, CHG 1
         L     R2,=A((6*_4K)+X'B00')
         RRBE  R0,R2
         BC    B'1110',*+1              NOT CC3 = was REF 1, CHG 1
*****************************************
         L     R2,=A((4*_4K)+X'900')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'18'
         BNE   *+1
         L     R2,=A((5*_4K)+X'A00')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'22'
         BNE   *+1
         L     R2,=A((6*_4K)+X'B00')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'4A'
         BNE   *+1
                                                                 EJECT
***********************************************************************
*                   IVSK, TPROT, TB (z/Arch mode)
***********************************************************************
                                                                SPACE
         ICM   R1,B'0001',=X'6E'
         L     R2,=A((7*_4K)+X'900')
         SSKE  R1,R2
         LCTLG R1,R1,CR1_Z              Configure DAT
         SSM   =X'04'                   Enable DAT
         IVSK  R1,R2
         SSM   =X'00'                   Disable DAT
         CLM   R1,B'0001',=X'68'
         BNE   *+1
*****************************************
         ICM   R1,B'0001',=X'10'
         L     R2,=A(6*_4K)
         SSKE  R1,R2
         L     R1,=A((6*_4K)+X'900')
         ICM   R2,B'0001',=X'10'
         TPROT 0(R1),0(R2)
         BC    B'0111',*+1              NOT CC0 = FETCH OK, STORE OK
         ICM   R2,B'0001',=X'20'
         TPROT 0(R1),0(R2)
         BC    B'1011',*+1              NOT CC1 = FETCH OK, STORE NO
         ICM   R1,B'0001',=X'18'        (set fetch protect)
         L     R2,=A(6*_4K)
         SSKE  R1,R2
         L     R1,=A(6*_4K)
         ICM   R2,B'0001',=X'20'
         TPROT 0(R1),0(R2)
         BC    B'1101',*+1              NOT CC2 = FETCH NO, STORE NO
*  We must skip the 'TB' (Test Block) test if we're running under
*  VM or are NOT running under Hercules since neither environment
*  has any command to sets block(s) of storage to "unusuable".
         CLI   CPUID,X'FF'              Are we running under VM?
         BE    SKIPTBZ                  Yes, then skip 'TB' tests
         CLI   CPUID,X'C8'              Are we running under Hercules?
         BNE   SKIPTBZ                  No, then skip 'TB' tests
         SLR   R0,R0                    Required by TB instruction
         L     R2,=A((10*_4K)+X'DEF')   Requires Herc 'f- A000' cmd
         TB    R1,R2
         BC    B'1011',*+1              NOT CC1 = Unusable/BAD block
         SLR   R0,R0                    Required by TB instruction
         L     R2,=A((11*_4K)+X'FED')   Requires Herc 'f- B000' cmd
         TB    R1,R2
         BC    B'1011',*+1              NOT CC1 = Unusable/BAD block
SKIPTBZ  EQU   *
                                                                EJECT
***********************************************************************
*                     SSKE with mask (z/Arch mode)
***********************************************************************
                                                                SPACE
         ICM   R1,B'0001',=X'33'    Low-order X'01' bit s/b ignored
         L     R2,=A(_1M-(3*_4K)+7) (with some low-order bits set too)
         LA    R0,2                 (set CC2...)
         SLL   R0,32-4              (shift into proper position)
         SPM   R0                   (set Condition Code 2 in PSW)
         SSKE  R1,R2,SSKE_MB        Now do Multi-block SSKE
         BC    B'1101',*+1          FAIL if not still CC2!!
         L     R3,=A(_1M+7)         Check if R2 now has expected value
         CLR   R2,R3                Does it?
         BNE   *+1                  FAIL if not
         L     R2,=A(_1M-(3*_4K)+7)
         ISKE  R1,R2
         CLM   R1,B'0001',=X'32'
         BNE   *+1
         L     R2,=A(_1M-(2*_4K)+7)
         ISKE  R1,R2
         CLM   R1,B'0001',=X'32'
         BNE   *+1
         L     R2,=A(_1M-(1*_4K)+7)
         ISKE  R1,R2
         CLM   R1,B'0001',=X'32'
         BNE   *+1
         L     R2,=A(_1M-(0*_4K)+7)
         ISKE  R1,R2
         CLM   R1,B'0001',=X'00'
         BNE   *+1
*****************************************
         ICM   R1,B'0001',=X'34'
         L     R2,=A(_1M-(1*_4K))
         SSKE  R1,R2
         L     R2,=A(_1M-(1*_4K))
         ISKE  R1,R2
         CLM   R1,B'0001',=X'34'
         BNE   *+1
         ICM   R1,B'0001',=X'36'      Ref + Chg
         L     R2,=A(_1M-(2*_4K))
         LA    R0,2
         SLL   R0,32-4
         SPM   R0
         SSKE  R1,R2,SSKE_MB+SSKE_MR  Multi-block MR=1, MC=0
         BC    B'1110',*+1            NOT CC3 = MB + MR/MC success?
         L     R3,=A(_1M)
         CLR   R2,R3
         BNE   *+1
         L     R2,=A(_1M-(2*_4K))
         ISKE  R1,R2
         CLM   R1,B'0001',=X'32'      Should still be X'32' due to
         BNE   *+1                    MR ignore = same = no change
         L     R2,=A(_1M-(1*_4K))
         ISKE  R1,R2
         CLM   R1,B'0001',=X'36'      But this one should be changed
         BNE   *+1
                                                                EJECT
***********************************************************************
*                     PFMF (z/Arch mode)
***********************************************************************
                                                                SPACE
*        UNCONDITIONALLY set all keys (in 2nd 1MB) to X'F4'...
*
*        Since they're all currently X'00' (different from the value
*        we want) and neither conditional option bit is on (meaning
*        the decision whether to update the key or not should be done
*        SOLELY on whether or not the key and fetch bits are already
*        the value we want them to be or not, i.e. no conditional
*        reference or change bit ignoring is requested)
*
         L     R1,PFMF_1            KEY=F4 MR=0 MC=0
         L     R2,=A(2*_1M)
         PFMF  R1,R2
         L     R2,=A(2*_1M)
         ISKE  R1,R2
         CLM   R1,B'0001',=X'F4'    (spot check)
         BNE   *+1
         L     R2,=A(2*_1M+(128*_4K))
         ISKE  R1,R2
         CLM   R1,B'0001',=X'F4'    (spot check)
         BNE   *+1
         L     R2,=A(2*_1M+_1M-1)
         ISKE  R1,R2
         CLM   R1,B'0001',=X'F4'    (spot check)
         BNE   *+1
*
*        Now set them all to X'F2' by specifying the option to ignore
*        any differences in the reference bit and only update the key
*        if the CHANGE bit is different from what we want (since the
*        key and fetch bits are already what we want)
*
         L     R1,PFMF_2            KEY=F2 MR=1 MC=0
         L     R2,=A(2*_1M)
         PFMF  R1,R2
         L     R2,=A(2*_1M)
         ISKE  R1,R2
         CLM   R1,B'0001',=X'F2'    (spot check)
         BNE   *+1
         L     R2,=A(2*_1M+(128*_4K))
         ISKE  R1,R2
         CLM   R1,B'0001',=X'F2'    (spot check)
         BNE   *+1
         L     R2,=A(2*_1M+_1M-1)
         ISKE  R1,R2
         CLM   R1,B'0001',=X'F2'    (spot check)
         BNE   *+1
                                                                EJECT
*
*        Finally, set them all back to X'F4' again by specifying the
*        option to ignore any differences in the change bit and only
*        update the key if the REFERENCE bit is different from what
*        we want.
*
         L     R1,PFMF_3            KEY=F4 MR=0 MC=1
         L     R2,=A(2*_1M)
         PFMF  R1,R2
         L     R2,=A(2*_1M)
         ISKE  R1,R2
         CLM   R1,B'0001',=X'F4'    (spot check)
         BNE   *+1
         L     R2,=A(2*_1M+(128*_4K))
         ISKE  R1,R2
         CLM   R1,B'0001',=X'F4'    (spot check)
         BNE   *+1
         L     R2,=A(2*_1M+_1M-1)
         ISKE  R1,R2
         CLM   R1,B'0001',=X'F4'    (spot check)
         BNE   *+1
                                                                EJECT
***********************************************************************
*                     IRBM, RRBM (z/Arch mode)
***********************************************************************
                                                                SPACE
*  We must skip IRBM and RRBM tests if we're running under VM due to
*  unreliability under z/VM. Refer to the PROGRAMMING NOTES for the
*  IRBM, ISKE, RRBE and RRBM instructions on page 10-30, 10-31, 10-119
*  and 10-120 of the SA22-7832-12 Principles of Operation manual where
*  it clearly states in no uncertain terms that the results of these
*  instructions are, amazingly, UNRELIABLE!
*
*  When the intruction is executed NATIVELY by Hercules, it ensures
*  the results are always consistent and reliable, but when z/VM
*  intercepts and simulates the instruction itself, the results are
*  unfortunately ALWAYS completely and totally inaccurate! (WTF?!)
                                                                SPACE
         CLI   CPUID,X'FF'                  Are we running under VM?
         BE    SKIPRRBM                     Yes, then skip both tests
                                                                SPACE
         TM    FACLIST+IRBMBYT,IRBMBIT      Is facility available?
         BZ    SKIPIRBM                     No, then skip this test
         SLGR  R1,R1
         L     R2,=A(2*_1M)
         IRBM  R1,R2
         CG    R1,=XL8'FFFFFFFFFFFFFFFF'
         BNE   *+1
                                                                SPACE
*  For some odd reason, z/VM never indicates to the guest that the
*  RRBM facility is available even when it actually is! That is to say,
*  when the RRBM facility actually *IS* available to the host (i.e. to
*  z/VM itself), z/VM always lies to the guest and tells it that it's
*  *NOT* available! (WTF?!)
                                                                SPACE
SKIPIRBM TM    FACLIST+RRBMBYT,RRBMBIT      Is facility available?
         BZ    SKIPRRBM                     No, then skip this test
         SLGR  R1,R1
         L     R2,=A(2*_1M)
         RRBM  R1,R2
         CG    R1,=XL8'FFFFFFFFFFFFFFFF'
         BNE   *+1
         RRBM  R1,R2
         CG    R1,=XL8'0000000000000000'
         BNE   *+1
SKIPRRBM EQU   *
                                                                EJECT
                                                                SPACE 6
***********************************************************************
***********************************************************************
**********                                                   **********
**********        E N D   O F   A L L   T E S T S            **********
**********                                                   **********
***********************************************************************
***********************************************************************
                                                                SPACE 2
         LPSWE GOODPSWZ               Load SUCCESS disabled wait PSW
                                                                SPACE 3
GOODPSWZ DC    0D'0',XL4'00020001'    z/Arch SUCCESS disabled wait PSW
         DC    XL4'80000000'          z/Arch SUCCESS disabled wait PSW
         DC    XL4'00000000'          z/Arch SUCCESS disabled wait PSW
         DC    A(0)                   z/Arch SUCCESS disabled wait PSW
                                                                SPACE 2
FAILZ    LPSWE FAILPSWZ               Load FAILURE disabled wait PSW
*                                     (currently unused but available
*                                      for future debugging purposes)
                                                                SPACE 3
FAILPSWZ DC    0D'0',XL4'00020001'    z/Arch FAILURE disabled wait PSW
         DC    XL4'80000000'          z/Arch FAILURE disabled wait PSW
         DC    XL4'00000000'          z/Arch FAILURE disabled wait PSW
         DC    XL4'EEEEEEEE'          z/Arch FAILURE disabled wait PSW
                                                                EJECT
***********************************************************************
*                 ESA/390 PROGRAM CHECK ROUTINE
***********************************************************************
                                                                SPACE
PGMCHK   ST    R1,SAVER1          Save original R1
         LA    R1,OKPGMS          R1 --> Expected PGMCHKs table
                                                                SPACE
         TM    PGMOLD+8-1,X'01'   Test failure? (odd branch address?)
         BZ    PGMTAB             No, something else; check table
                                                                SPACE
         L     R1,PGMOLD+4        Yes, get program check address
         SH    R1,=H'5'           Backup to failing branch instruction
         ST    R1,PGMOLD+4        Put back into PGM OLD PSW
         B     PGMFAIL            Go load disabled wait PSW
                                                                SPACE
PGMNEXT  LA    R1,12(,R1)         Bump to next entry
PGMTAB   CLC   0(12,R1),=12X'00'  End of table?
         BE    PGMFAIL            Yes, bonafide program check!
         CLC   0(2,R1),PGMCODE+2  Expected Program Interrupt Code?
         BNE   PGMNEXT            No, try next entry
         CLC   4(4,R1),PGMOLD+4   Expected Program Interrupt Address?
         BNE   PGMNEXT            No, try next entry
                                                                SPACE
         MVC   PGMOLD+4(4),8(R1)  Yes! Move continue address into PSW
         NI    PGMOLD,X'FF'-X'04' Turn off DAT in case it's on
         L     R1,SAVER1          Restore original R1
         LPSW  PGMOLD             Ignore the crash and continue
                                                                SPACE
PGMFAIL  OI    PGMOLD+1,X'02'     Convert to disabled wait PSW
         L     R1,SAVER1          Restore original R1
         LPSW  PGMOLD             Load disabled wait crash PSW
                                                                SPACE 3
***********************************************************************
*                 Table of allowable program checks...
***********************************************************************
                                                                SPACE
OKPGMS   DC    0D'0'
         DC    2AL2(PGM_OPERATION_EXCEPTION),A(STFLPC),A(STFLPC)
         DC    2AL2(PGM_SPECIFICATION_EXCEPTION),A(SSMPC),A(SKIPIVSK)
         DC    2AL2(0),A(0),A(0)  End of table
                                                                EJECT
***********************************************************************
*             z/Architecture PROGRAM CHECK ROUTINE
***********************************************************************
                                                                SPACE
ZPGMCHK  STG   R1,SAVER1          Save original R1
         LA    R1,ZOKPGMS         R1 --> Expected PGMCHKs table
                                                                SPACE
         TM    ZPGMOLD+16-1,X'01' Test failure? (odd branch address?)
         BZ    ZPGMTAB            No, something else; check table
                                                                SPACE
         L     R1,ZPGMOLD+12      Yes, get program check address
         SH    R1,=H'5'           Backup to failing branch instruction
         ST    R1,ZPGMOLD+12      Put back into PGM OLD PSW
         B     ZPGMFAIL           Go load disabled wait PSW
                                                                SPACE
ZPGMNEXT LA    R1,12(,R1)         Bump to next entry
ZPGMTAB  CLC   0(12,R1),=12X'00'  End of table?
         BE    ZPGMFAIL           Yes, bonafide program check!
         CLC   0(2,R1),PGMCODE+2  Expected Program Interrupt Code?
         BNE   ZPGMNEXT           No, try next entry
         CLC   4(4,R1),ZPGMOLD+12 Expected Program Interrupt Address?
         BNE   ZPGMNEXT           No, try next entry
                                                                SPACE
         MVC   ZPGMOLD+12(4),8(R1) Yes! Move continue address into PSW
         NI    ZPGMOLD,X'FF'-X'04' Turn off DAT in case it's on
         LG    R1,SAVER1          Restore original R1
         LPSWE ZPGMOLD            Ignore the crash and continue
                                                                SPACE
ZPGMFAIL OI    ZPGMOLD+1,X'02'    Convert to disabled wait PSW
         LG    R1,SAVER1          Restore original R1
         LPSWE ZPGMOLD            Load disabled wait crash PSW
                                                                SPACE 3
***********************************************************************
*                 Table of allowable program checks...
***********************************************************************
                                                                SPACE
ZOKPGMS  DC    0D'0'
         DC    2AL2(0),A(0),A(0)  End of table
                                                                EJECT
***********************************************************************
*                      Working storage
***********************************************************************
                                                                SPACE
_4K      EQU   4096               Constant 4K
_1M      EQU   (1024*1024)        Constant 1M
*
CPUID    DC    D'0'               CPU Identification
SAVER1   DC    D'0'               Saved original R1 value
STFL390  DC    XL4'99999999'      Saved S/390  STFL results
STFLZ    DC    XL4'99999999'      Saved z/Arch STFL results
                                                                SPACE
FACLIST  DC    0XL256'00',256X'99'  Extended Facilities List...
                                                                SPACE
RRBMBYT  EQU   X'08'              Facility 66  byte (RRBM instruction)
RRBMBIT  EQU   X'20'              Facility 66  bit
IRBMBYT  EQU   X'12'              Facility 145 byte (IRBM instruction)
IRBMBIT  EQU   X'40'              Facility 145 bit
                                                                SPACE
CR0_1_39 DC    0F'0',XL4'00B00000',A(SEGTAB39)    ESA/390 ASD
CR1_Z    DC    0D'0',A(0),A(SEGTABZ)              z/Arch. ASD
                                                                SPACE
PFMF_1   DC    A(PFMF_SK+PFMF_1M+X'F4')
PFMF_2   DC    A(PFMF_SK+PFMF_1M+PFMF_MR+X'F2')
PFMF_3   DC    A(PFMF_SK+PFMF_1M+PFMF_MC+X'F4')
                                                                SPACE
PFMF_SK  EQU   X'00020000'        Set the storage key
PFMF_CF  EQU   X'00010000'        Zero page frame too
                                                                SPACE
PFMF_4K  EQU   X'00000000'        Process just one single 4K page
PFMF_1M  EQU   X'00001000'        Process 1MB frame of 4K pages
                                                                SPACE
PFMF_MR  EQU   X'00000400'        Don't compare Reference bits
PFMF_MC  EQU   X'00000200'        Don't compare Change    bits
                                                                SPACE
SSKE_MR  EQU   X'04'              Reference Bit Update Mask
SSKE_MC  EQU   X'02'              Change    Bit Update Mask
SSKE_MB  EQU   X'01'              Multiple Blocks Option
                                                                EJECT
         LTORG ,                                Literals pool
                                                                EJECT
***********************************************************************
*                      390 DAT tables
***********************************************************************
                                                                SPACE
         ORG   TEST+X'1000'
                                                                SPACE
SEGTAB39 DC    A(PAGTAB39)
         DC    15XL4'00000020'
                                                                SPACE 3
         ORG   TEST+X'1800'
                                                                SPACE
PAGTAB39 DC    A(0*_4K)
         DC    A(1*_4K)
         DC    A(2*_4K)
         DC    A(3*_4K)
         DC    A(4*_4K)
         DC    A(5*_4K)
         DC    A(6*_4K)
         DC    A(7*_4K)
         DC    A(8*_4K)
         DC    A(9*_4K)
         DC    A(10*_4K)
         DC    A(11*_4K)
         DC    A(12*_4K)
         DC    A(13*_4K)
         DC    A(14*_4K)
         DC    A(15*_4K)
                                                                EJECT
***********************************************************************
*                      z/Arch DAT tables
***********************************************************************
                                                                SPACE
         ORG   TEST+X'2000'
                                                                SPACE
SEGTABZ  DC    AD(PAGTABZ)
         DC    511AD(X'20')
                                                                SPACE 3
         ORG   TEST+X'3000'
                                                                SPACE
PAGTABZ  DC    AD(0*_4K)
         DC    AD(1*_4K)
         DC    AD(2*_4K)
         DC    AD(3*_4K)
         DC    AD(4*_4K)
         DC    AD(5*_4K)
         DC    AD(6*_4K)
         DC    AD(7*_4K)
         DC    AD(8*_4K)
         DC    AD(9*_4K)
         DC    AD(10*_4K)
         DC    AD(11*_4K)
         DC    AD(12*_4K)
         DC    AD(13*_4K)
         DC    AD(14*_4K)
         DC    AD(15*_4K)
                                                                EJECT
***********************************************************************
*                      Register equates
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
