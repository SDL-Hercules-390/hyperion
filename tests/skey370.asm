 TITLE '                           Test S/370 STORAGE KEY Instructions'
***********************************************************************
*
*                            SKEY370
*
***********************************************************************
*
*   This program verifies proper functioning of the following
*   System/370 Storage Key instructions:
*
*       ISK,  RRB,  SSK                     (GA22-7000-04 Sep 75)
*       ISKE, RRBE, SSKE, IVSK, TPROT, TB   (GA22-7000-10 Sep 87)
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
*   FINALLY, when running under VM (high-order byte of CPUID = X'FF')
*   the 'TB' (Test Block) test is always skipped since VM doesn't
*   support the ability to mark frames of storage as "bad" (whereas
*   Hercules does via its "f-" command).
*
***********************************************************************
                                                                EJECT
***********************************************************************
*                            LOW CORE
***********************************************************************
                                                                SPACE 4
TEST     START 0
         USING TEST,0                 Use absolute addressing
                                                                SPACE 3
         ORG   TEST+X'00'             S/370 Restart new PSW
         DC    XL4'00080000'          S/370 Restart new PSW
         DC    A(BEGIN)               S/370 Restart new PSW
                                                                SPACE 2
         ORG   TEST+X'28'             S/370 Program old PSW
PGMOLD   EQU   *                      S/370 Program old PSW
                                                                SPACE 2
         ORG   TEST+X'68'             S/370 Program new PSW
         DC    XL4'00080000'          S/370 Program new PSW
         DC    A(PGMCHK)              S/370 Program new PSW
                                                                SPACE 2
         ORG   TEST+X'8C'             Program interrupt code
PGMCODE  DC    F'0'                   Program interrupt code
                                                                SPACE 3
PGM_OPERATION_EXCEPTION           EQU   X'0001'
PGM_SPECIFICATION_EXCEPTION       EQU   X'0006'
PGM_SPECIAL_OPERATION_EXCEPTION   EQU   X'0013'
                                                                SPACE 7
         ORG   TEST+X'200'        Start of test program
                                                                SPACE
BEGIN    STIDP CPUID              Save CPU ID (for later test for VM)
         B     TEST370            Go get started...
                                                                EJECT
***********************************************************************
*               Determine Instruction Availability
***********************************************************************
*
*   The ISKE/RRBE/SSKE/IVSK/TPROT/TB instructions didn't exist on
*   early System/370 machines. They were introduced much later. If
*   we're running under e.g. VM/370 (which was written for earlier
*   versions of System/370) then we cannot execute any of those
*   instructions since VM/370 doesn't support them. Hercules does
*   support them, but VM/370 itself doesn't, and as VM/370 didn't
*   use SIE and had to simulate all control instructions itself,
*   such instructions would cause a program check. Note that we do
*   not have to test for support for all of them. A simple test of
*   the RRBE instruction for example, if it fails, is good enough
*   to let us know that very likely none of the other instructions
*   are supported either.
*
***********************************************************************
                                                                SPACE 2
TEST370  LCTL  R0,R1,CR0_1_2K     2K pages + StorKey Except. Ctl.
         SLR   R2,R2              Page address unimportant
         RRBE  R0,R2              Can we execute RRBE instructions?
RRBE_PC  EQU   *                  (possible program check here...)
                                                                SPACE
         MVI   _NEW370,X'FF'      It worked! Must be newer System/370
         B     TST4KBBF           Continue with initialization
                                                                SPACE
NO_RRBE  MVI   _NEW370,X'00'      It failed! Must be older System/370
         B     TST4KBBF           Continue with initialization
                                                                EJECT
***********************************************************************
*             Determine 4KBBF (4K-Byte-Block Facility)
***********************************************************************
*
*   Determine whether 4KBBF (4K-Byte-Block Facility) is installed
*   or not. The 4K-Byte-Block Facility is basically hardware that
*   only supports 4K page frames, each with a single storage key.
*
*   When installed, the SSK/ISK/RRB instructions cannot be executed
*   unless the CR0 Storage Key Exception Control bit is one, which
*   allows them to execute, but of course causes them to only operate
*   on the single-keyed 4K page; it is NOT possible to set different
*   keys for each of of the 2K pages within the 4K frame when 4KBBF
*   is installed.
*
*   When 4KBBF is NOT installed, the Storage Key Exception Control
*   bit in CR0 is ignored and SSK/ISK/RRB execute normally, and
*   the storage key for each 2K page frame can be different.
*
***********************************************************************
                                                                SPACE 2
TST4KBBF LCTL  R0,R1,CR0_1_2K     Set 2K page mode
         ICM   R1,B'0001',=X'F0'  Arbitrary non-zero key value
         L     R2,=A(50*_2K)      Beginning of 4K page
         SSK   R1,R2              Set key for this SUPPOSED 2K page
         L     R2,=A(51*_2K)      Middle of same 4K page
         ISK   R1,R2              Get key for this SUPPOSED 2K page
                                                                SPACE
         CLM   R1,B'0001',=X'F0'  Was it's key changed too?
         BNE   BEGX4K             No, then all pages are indeed 2K!
         MVI   _4KBBF,X'FF'       Yes, then all pages are really 4K!
         B     BEG4K              Run only 4KBBF tests
                                                                EJECT
***********************************************************************
*                      non-4KBBF tests
***********************************************************************
                                                                SPACE
BEGX4K   LCTL  R0,R1,CR0_1_4K     Set 4K page mode
                                                                SPACE
         BAL   R14,XSSK4K         SSK/ISK/RRB
         CLI   _NEW370,X'FF'      Is this a newer model System/370?
         BNE   SKIPX4K            No, skip newer System/370 tests
         BAL   R14,XSSKE4K        SSKE/ISKE/RRBE
         BAL   R14,XIVSK4K        IVSK/TPROT/TB
                                                                SPACE
SKIPX4K  LCTL  R0,R1,CR0_1_2K     Set 2K page mode
                                                                SPACE
         BAL   R14,XSSK2K         SSK/ISK/RRB
         CLI   _NEW370,X'FF'      Is this a newer model S/370?
         BNE   SKIPX2K            No, skip newer System/370 tests
         BAL   R14,XSSKE2K        SSKE/ISKE/RRBE
         BAL   R14,XIVSK2K        IVSK/TPROT/TB
                                                                SPACE
SKIPX2K  B     SUCCESS            Done! All tests succeeded!
                                                                SPACE
***********************************************************************
*                       4KBBF tests
***********************************************************************
                                                                SPACE
BEG4K    LCTL  R0,R1,CR0_1_4K     Set 4K page mode
                                                                SPACE
         BAL   R14,SSK4K          SSK/ISK/RRB
         CLI   _NEW370,X'FF'      Is this a newer model Sustem/370?
         BNE   SKIP4K             No, skip newer System/370 tests
         BAL   R14,SSKE4K         SSKE/ISKE/RRBE
         BAL   R14,IVSK4K         IVSK/TPROT/TB
                                                                SPACE
SKIP4K   LCTL  R0,R1,CR0_1_2K     Set 2K page mode
                                                                SPACE
         BAL   R14,SSK2K          SSK/ISK/RRB
         CLI   _NEW370,X'FF'      Is this a newer model System/370?
         BNE   SKIP2K             No, skip newer System/370 tests
         BAL   R14,SSKE2K         SSKE/ISKE/RRBE
         BAL   R14,IVSK2K         IVSK/TPROT/TB
                                                                SPACE
SKIP2K   B     SUCCESS            Done! All tests succeeded!
                                                                SPACE 2
***********************************************************************
*                        SUCCESS!
***********************************************************************
                                                                SPACE
SUCCESS  LPSW  GOODPSW                Load  SUCCESS disabled wait PSW
GOODPSW  DC    0D'0',XL4'000A0000'    S/370 SUCCESS disabled wait PSW
         DC    A(0)                   S/370 SUCCESS disabled wait PSW
                                                                EJECT
***********************************************************************
*                   SSK/ISK/RRB (non-4KBBF -- 4K mode)
***********************************************************************
                                                                SPACE
XSSK4K   ICM   R1,B'0001',=X'16'
         L     R2,=A(6*_2K)
         SSK   R1,R2
         ICM   R1,B'0001',=X'24'
         L     R2,=A(7*_2K)
         SSK   R1,R2
         ICM   R1,B'0001',=X'4C'
         L     R2,=A(8*_2K)
         SSK   R1,R2
*****************************************
         L     R2,=A(6*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'16'
         BNE   *+1
         L     R2,=A(7*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'24'
         BNE   *+1
         L     R2,=A(8*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'4C'
         BNE   *+1
*****************************************
         L     R2,=A(6*_2K)
         RRB   0(R2)
         BC    B'1110',*+1      NOT CC3 = was REF 1, CHG 1
         L     R2,=A(7*_2K)
         RRB   0(R2)
         BC    B'1101',*+1      NOT CC2 = was REF 1, CHG 0
         L     R2,=A(8*_2K)
         RRB   0(R2)
         BC    B'1101',*+1      NOT CC2 = was REF 1, CHG 0
*****************************************
         L     R2,=A(6*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'12'
         BNE   *+1
         L     R2,=A(7*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'20'
         BNE   *+1
         L     R2,=A(8*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'48'
         BNE   *+1
         BR    R14
                                                                EJECT
***********************************************************************
*                   SSKE/ISKE/RRBE (non-4KBBF -- 4K mode)
***********************************************************************
                                                                SPACE
XSSKE4K  ICM   R1,B'0001',=X'1C'
         L     R2,=A((6*_2K)+X'100')
         SSKE  R1,R2
         ICM   R1,B'0001',=X'26'
         L     R2,=A((7*_2K)+X'200')
         SSKE  R1,R2
         ICM   R1,B'0001',=X'4E'
         L     R2,=A((8*_2K)+X'300')
         SSKE  R1,R2
*****************************************
         L     R2,=A((6*_2K)+X'100')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'26'
         BNE   *+1
         L     R2,=A((7*_2K)+X'200')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'26'
         BNE   *+1
         L     R2,=A((8*_2K)+X'300')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'4E'
         BNE   *+1
*****************************************
         L     R2,=A((6*_2K)+X'100')
         RRBE  R0,R2
         BC    B'1110',*+1              NOT CC3 = was REF 1, CHG 1
         L     R2,=A((7*_2K)+X'200')
         RRBE  R0,R2
         BC    B'1011',*+1              NOT CC1 = was REF 0, CHG 1
         L     R2,=A((8*_2K)+X'300')
         RRBE  R0,R2
         BC    B'1110',*+1              NOT CC3 = was REF 1, CHG 1
*****************************************
         L     R2,=A((6*_2K)+X'100')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'22'
         BNE   *+1
         L     R2,=A((7*_2K)+X'200')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'22'
         BNE   *+1
         L     R2,=A((8*_2K)+X'300')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'4A'
         BNE   *+1
         BR    R14
                                                                EJECT
***********************************************************************
*                   IVSK/TPROT/TB (non-4KBBF -- 4K mode)
***********************************************************************
                                                                SPACE
XIVSK4K  ICM   R1,B'0001',=X'6E'
         L     R2,=A((9*_2K)+X'400')
         SSK   R1,R2
*
         SSM   =X'04'                   (enable DAT)
         IVSK  R1,R2
         SSM   =X'00'                   (disable DAT again)
         CLM   R1,B'0001',=X'68'
         BNE   *+1
*****************************************
         ICM   R1,B'0001',=X'10'
         L     R2,=A(6*_2K)
         SSK   R1,R2
         L     R1,=A((6*_2K)+X'100')
         ICM   R2,B'0001',=X'10'
         TPROT 0(R1),0(R2)
         BC    B'0111',*+1              NOT CC0 = FETCH OK, STORE OK
         ICM   R2,B'0001',=X'20'
         TPROT 0(R1),0(R2)
         BC    B'1011',*+1              NOT CC1 = FETCH OK, STORE NO
         ICM   R1,B'0001',=X'18'        (set fetch protect)
         L     R2,=A(6*_2K)
         SSK   R1,R2
         L     R1,=A(6*_2K)
         ICM   R2,B'0001',=X'20'
         TPROT 0(R1),0(R2)
         BC    B'1101',*+1              NOT CC2 = FETCH NO, STORE NO
*****************************************
         CLI   CPUID,X'FF'              Are we running under VM?
         BE    XSKPTB4K                 Yes, then skip 'TB' tests
         SLR   R0,R0                    Required by TB instruction
         L     R2,=A((10*_2K)+X'500')   Requires Herc 'f- 5000' cmd
         TB    R1,R2
         BC    B'1011',*+1              NOT CC1 = Unusable/BAD block
         SLR   R0,R0                    Required by TB instruction
         L     R2,=A((11*_2K)+X'600')   Requires Herc 'f- 5800' cmd
         TB    R1,R2
         BC    B'1011',*+1              NOT CC1 = Unusable/BAD block
XSKPTB4K EQU   *
         BR    R14
                                                                EJECT
***********************************************************************
*                   SSK/ISK/RRB (non-4KBBF -- 2K mode)
***********************************************************************
                                                                SPACE
XSSK2K   ICM   R1,B'0001',=X'16'
         L     R2,=A(6*_2K)
         SSK   R1,R2
         ICM   R1,B'0001',=X'24'
         L     R2,=A(7*_2K)
         SSK   R1,R2
         ICM   R1,B'0001',=X'4C'
         L     R2,=A(8*_2K)
         SSK   R1,R2
*****************************************
         L     R2,=A(6*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'16'
         BNE   *+1
         L     R2,=A(7*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'24'
         BNE   *+1
         L     R2,=A(8*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'4C'
         BNE   *+1
*****************************************
         L     R2,=A(6*_2K)
         RRB   0(R2)
         BC    B'1110',*+1      NOT CC3 = was REF 1, CHG 1
         L     R2,=A(7*_2K)
         RRB   0(R2)
         BC    B'1101',*+1      NOT CC2 = was REF 1, CHG 0
         L     R2,=A(8*_2K)
         RRB   0(R2)
         BC    B'1101',*+1      NOT CC2 = was REF 1, CHG 0
*****************************************
         L     R2,=A(6*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'12'
         BNE   *+1
         L     R2,=A(7*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'20'
         BNE   *+1
         L     R2,=A(8*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'48'
         BNE   *+1
         BR    R14
                                                                EJECT
***********************************************************************
*                   SSKE/ISKE/RRBE (non-4KBBF -- 2K mode)
***********************************************************************
                                                                SPACE
XSSKE2K  ICM   R1,B'0001',=X'1C'
         L     R2,=A((6*_2K)+X'100')
         SSKE  R1,R2
         ICM   R1,B'0001',=X'26'
         L     R2,=A((7*_2K)+X'200')
         SSKE  R1,R2
         ICM   R1,B'0001',=X'4E'
         L     R2,=A((8*_2K)+X'300')
         SSKE  R1,R2
*****************************************
         L     R2,=A((6*_2K)+X'100')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'26'
         BNE   *+1
         L     R2,=A((7*_2K)+X'200')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'26'
         BNE   *+1
         L     R2,=A((8*_2K)+X'300')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'4E'
         BNE   *+1
*****************************************
         L     R2,=A((6*_2K)+X'100')
         RRBE  R0,R2
         BC    B'1110',*+1              NOT CC3 = was REF 1, CHG 1
         L     R2,=A((7*_2K)+X'200')
         RRBE  R0,R2
         BC    B'1011',*+1              NOT CC1 = was REF 0, CHG 1
         L     R2,=A((8*_2K)+X'300')
         RRBE  R0,R2
         BC    B'1110',*+1              NOT CC3 = was REF 1, CHG 1
*****************************************
         L     R2,=A((6*_2K)+X'100')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'22'
         BNE   *+1
         L     R2,=A((7*_2K)+X'200')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'22'
         BNE   *+1
         L     R2,=A((8*_2K)+X'300')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'4A'
         BNE   *+1
         BR    R14
                                                                EJECT
***********************************************************************
*                   IVSK/TPROT/TB (non-4KBBF -- 2K mode)
***********************************************************************
                                                                SPACE
XIVSK2K  ICM   R1,B'0001',=X'6E'
         L     R2,=A((9*_2K)+X'400')
         SSK   R1,R2
*
         SSM   =X'04'                   (enable DAT)
         IVSK  R1,R2
         SSM   =X'00'                   (disable DAT again)
         CLM   R1,B'0001',=X'68'
         BNE   *+1
*****************************************
         ICM   R1,B'0001',=X'10'
         L     R2,=A(6*_2K)
         SSK   R1,R2
         L     R1,=A((6*_2K)+X'100')
         ICM   R2,B'0001',=X'10'
         TPROT 0(R1),0(R2)
         BC    B'0111',*+1              NOT CC0 = FETCH OK, STORE OK
         ICM   R2,B'0001',=X'20'
         TPROT 0(R1),0(R2)
         BC    B'1011',*+1              NOT CC1 = FETCH OK, STORE NO
         ICM   R1,B'0001',=X'18'        (set fetch protect)
         L     R2,=A(6*_2K)
         SSK   R1,R2
         L     R1,=A(6*_2K)
         ICM   R2,B'0001',=X'20'
         TPROT 0(R1),0(R2)
         BC    B'1101',*+1              NOT CC2 = FETCH NO, STORE NO
*****************************************
         CLI   CPUID,X'FF'              Are we running under VM?
         BE    XSKPTB2K                 Yes, then skip 'TB' tests
         SLR   R0,R0                    Required by TB instruction
         L     R2,=A((10*_2K)+X'500')   Requires Herc 'f- 5000' cmd
         TB    R1,R2
         BC    B'1011',*+1              NOT CC1 = Unusable/BAD block
         SLR   R0,R0                    Required by TB instruction
         L     R2,=A((11*_2K)+X'600')   Requires Herc 'f- 5800' cmd
         TB    R1,R2
         BC    B'1011',*+1              NOT CC1 = Unusable/BAD block
XSKPTB2K EQU   *
         BR    R14
                                                                EJECT
***********************************************************************
*                   SSK/ISK/RRB (4KBBF -- 4K mode)
***********************************************************************
                                                                SPACE
SSK4K    ICM   R1,B'0001',=X'16'
         L     R2,=A(6*_2K)
         SSK   R1,R2
         ICM   R1,B'0001',=X'24'
         L     R2,=A(7*_2K)
         SSK   R1,R2
         ICM   R1,B'0001',=X'4E'
         L     R2,=A(8*_2K)
         SSK   R1,R2
*****************************************
         L     R2,=A(6*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'24'
         BNE   *+1
         L     R2,=A(7*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'24'
         BNE   *+1
         L     R2,=A(8*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'4E'
         BNE   *+1
*****************************************
         L     R2,=A(6*_2K)
         RRB   0(R2)
         BC    B'1101',*+1      NOT CC2 = was REF 1, CHG 0
         L     R2,=A(7*_2K)
         RRB   0(R2)
         BC    B'0111',*+1      NOT CC0 = was REF 0, CHG 0
         L     R2,=A(8*_2K)
         RRB   0(R2)
         BC    B'1110',*+1      NOT CC3 = was REF 1, CHG 1
*****************************************
         L     R2,=A(6*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'20'
         BNE   *+1
         L     R2,=A(7*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'20'
         BNE   *+1
         L     R2,=A(8*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'4A'
         BNE   *+1
         BR    R14
                                                                EJECT
***********************************************************************
*                   SSKE/ISKE/RRBE (4KBBF -- 4K mode)
***********************************************************************
                                                                SPACE
SSKE4K   ICM   R1,B'0001',=X'16'
         L     R2,=A((6*_2K)+X'100')
         SSKE  R1,R2
         ICM   R1,B'0001',=X'24'
         L     R2,=A((7*_2K)+X'200')
         SSKE  R1,R2
         ICM   R1,B'0001',=X'4E'
         L     R2,=A((8*_2K)+X'300')
         SSKE  R1,R2
*****************************************
         L     R2,=A((6*_2K)+X'100')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'24'
         BNE   *+1
         L     R2,=A((7*_2K)+X'200')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'24'
         BNE   *+1
         L     R2,=A((8*_2K)+X'300')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'4E'
         BNE   *+1
*****************************************
         L     R2,=A((6*_2K)+X'100')
         RRBE  R0,R2
         BC    B'1101',*+1              NOT CC2 = was REF 1, CHG 0
         L     R2,=A((7*_2K)+X'200')
         RRBE  R0,R2
         BC    B'0111',*+1              NOT CC0 = was REF 0, CHG 0
         L     R2,=A((8*_2K)+X'300')
         RRBE  R0,R2
         BC    B'1110',*+1              NOT CC3 = was REF 1, CHG 1
*****************************************
         L     R2,=A((6*_2K)+X'100')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'20'
         BNE   *+1
         L     R2,=A((7*_2K)+X'200')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'20'
         BNE   *+1
         L     R2,=A((8*_2K)+X'300')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'4A'
         BNE   *+1
         BR    R14
                                                                EJECT
***********************************************************************
*                   IVSK/TPROT/TB (4KBBF -- 4K mode)
***********************************************************************
                                                                SPACE
IVSK4K   ICM   R1,B'0001',=X'6E'
         L     R2,=A((9*_2K)+X'400')
         SSK   R1,R2
*
         SSM   =X'04'                   (enable DAT)
         IVSK  R1,R2
         SSM   =X'00'                   (disable DAT again)
         CLM   R1,B'0001',=X'68'
         BNE   *+1
*****************************************
         ICM   R1,B'0001',=X'10'
         L     R2,=A(6*_2K)
         SSK   R1,R2
         L     R1,=A((6*_2K)+X'100')
         ICM   R2,B'0001',=X'10'
         TPROT 0(R1),0(R2)
         BC    B'0111',*+1              NOT CC0 = FETCH OK, STORE OK
         ICM   R2,B'0001',=X'20'
         TPROT 0(R1),0(R2)
         BC    B'1011',*+1              NOT CC1 = FETCH OK, STORE NO
         ICM   R1,B'0001',=X'18'        (set fetch protect)
         L     R2,=A(6*_2K)
         SSK   R1,R2
         L     R1,=A(6*_2K)
         ICM   R2,B'0001',=X'20'
         TPROT 0(R1),0(R2)
         BC    B'1101',*+1              NOT CC2 = FETCH NO, STORE NO
*****************************************
         CLI   CPUID,X'FF'              Are we running under VM?
         BE    SKPTB4K                  Yes, then skip 'TB' tests
         SLR   R0,R0                    Required by TB instruction
         L     R2,=A((10*_2K)+X'500')   Requires Herc 'f- 5000' cmd
         TB    R1,R2
         BC    B'1011',*+1              NOT CC1 = Unusable/BAD block
         SLR   R0,R0                    Required by TB instruction
         L     R2,=A((11*_2K)+X'600')   Requires Herc 'f- 5800' cmd
         TB    R1,R2
         BC    B'1011',*+1              NOT CC1 = Unusable/BAD block
SKPTB4K  EQU   *
         BR    R14
                                                                EJECT
***********************************************************************
*                   SSK/ISK/RRB (4KBBF -- 2K mode)
***********************************************************************
                                                                SPACE
SSK2K    ICM   R1,B'0001',=X'16'
         L     R2,=A(6*_2K)
         SSK   R1,R2
         ICM   R1,B'0001',=X'24'
         L     R2,=A(7*_2K)
         SSK   R1,R2
         ICM   R1,B'0001',=X'4E'
         L     R2,=A(8*_2K)
         SSK   R1,R2
*****************************************
         L     R2,=A(6*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'24'
         BNE   *+1
         L     R2,=A(7*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'24'
         BNE   *+1
         L     R2,=A(8*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'4E'
         BNE   *+1
*****************************************
         L     R2,=A(6*_2K)
         RRB   0(R2)
         BC    B'1101',*+1      NOT CC2 = was REF 1, CHG 0
         L     R2,=A(7*_2K)
         RRB   0(R2)
         BC    B'0111',*+1      NOT CC0 = was REF 0, CHG 0
         L     R2,=A(8*_2K)
         RRB   0(R2)
         BC    B'1110',*+1      NOT CC3 = was REF 1, CHG 1
*****************************************
         L     R2,=A(6*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'20'
         BNE   *+1
         L     R2,=A(7*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'20'
         BNE   *+1
         L     R2,=A(8*_2K)
         ISK   R1,R2
         CLM   R1,B'0001',=X'4A'
         BNE   *+1
         BR    R14
                                                                EJECT
***********************************************************************
*                   SSKE/ISKE/RRBE (4KBBF -- 2K mode)
***********************************************************************
                                                                SPACE
SSKE2K   ICM   R1,B'0001',=X'16'
         L     R2,=A((6*_2K)+X'100')
         SSKE  R1,R2
         ICM   R1,B'0001',=X'24'
         L     R2,=A((7*_2K)+X'200')
         SSKE  R1,R2
         ICM   R1,B'0001',=X'4E'
         L     R2,=A((8*_2K)+X'300')
         SSKE  R1,R2
*****************************************
         L     R2,=A((6*_2K)+X'100')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'24'
         BNE   *+1
         L     R2,=A((7*_2K)+X'200')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'24'
         BNE   *+1
         L     R2,=A((8*_2K)+X'300')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'4E'
         BNE   *+1
*****************************************
         L     R2,=A((6*_2K)+X'100')
         RRBE  R0,R2
         BC    B'1101',*+1              NOT CC2 = was REF 1, CHG 0
         L     R2,=A((7*_2K)+X'200')
         RRBE  R0,R2
         BC    B'0111',*+1              NOT CC0 = was REF 0, CHG 0
         L     R2,=A((8*_2K)+X'300')
         RRBE  R0,R2
         BC    B'1110',*+1              NOT CC3 = was REF 1, CHG 1
*****************************************
         L     R2,=A((6*_2K)+X'100')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'20'
         BNE   *+1
         L     R2,=A((7*_2K)+X'200')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'20'
         BNE   *+1
         L     R2,=A((8*_2K)+X'300')
         ISKE  R1,R2
         CLM   R1,B'0001',=X'4A'
         BNE   *+1
         BR    R14
                                                                EJECT
***********************************************************************
*                   IVSK/TPROT/TB (4KBBF -- 2K mode)
***********************************************************************
                                                                SPACE
IVSK2K   ICM   R1,B'0001',=X'6E'
         L     R2,=A((9*_2K)+X'400')
         SSK   R1,R2
*
         SSM   =X'04'                   (enable DAT)
         IVSK  R1,R2
         SSM   =X'00'                   (disable DAT again)
         CLM   R1,B'0001',=X'68'
         BNE   *+1
*****************************************
         ICM   R1,B'0001',=X'10'
         L     R2,=A(6*_2K)
         SSK   R1,R2
         L     R1,=A((6*_2K)+X'100')
         ICM   R2,B'0001',=X'10'
         TPROT 0(R1),0(R2)
         BC    B'0111',*+1              NOT CC0 = FETCH OK, STORE OK
         ICM   R2,B'0001',=X'20'
         TPROT 0(R1),0(R2)
         BC    B'1011',*+1              NOT CC1 = FETCH OK, STORE NO
         ICM   R1,B'0001',=X'18'        (set fetch protect)
         L     R2,=A(6*_2K)
         SSK   R1,R2
         L     R1,=A(6*_2K)
         ICM   R2,B'0001',=X'20'
         TPROT 0(R1),0(R2)
         BC    B'1101',*+1              NOT CC2 = FETCH NO, STORE NO
*****************************************
         CLI   CPUID,X'FF'              Are we running under VM?
         BE    SKPTB2K                  Yes, then skip 'TB' tests
         SLR   R0,R0                    Required by TB instruction
         L     R2,=A((10*_2K)+X'500')   Requires Herc 'f- 5000' cmd
         TB    R1,R2
         BC    B'1011',*+1              NOT CC1 = Unusable/BAD block
         SLR   R0,R0                    Required by TB instruction
         L     R2,=A((11*_2K)+X'600')   Requires Herc 'f- 5800' cmd
         TB    R1,R2
         BC    B'1011',*+1              NOT CC1 = Unusable/BAD block
SKPTB2K  EQU   *
         BR    R14
                                                                EJECT
***********************************************************************
*                 System/370 PROGRAM CHECK ROUTINE
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
OKPGMS   DC    0D'0'              Table of allowable program checks
         DC   2AL2(PGM_OPERATION_EXCEPTION),A(RRBE_PC),A(NO_RRBE)
         DC   2AL2(0),A(0),A(0)   End of table
                                                                EJECT
***********************************************************************
*                      Working storage
***********************************************************************
                                                                SPACE
CPUID    DC    D'0'                         CPU Identification
SAVER1   DC    D'0'                         Saved original R1 value
*
_2K      EQU   2048                         ("_2K" shorter than "_2K")
_4K      EQU   4096                         ("_4K" shorter than "_4K")
*
CR0_2K   EQU   X'40'                        2K pages mode CR0 flag
CR0_4K   EQU   X'80'                        4K pages mode CR0 flag
*
CR0_1_2K DC    (0*2)F'0'                    CR0/CR1 for 2K pages mode
         DC    AL1(CR0_SKEC),AL1(CR0_2K),AL2(0)
         DC    A(SEGTAB2K)
*
CR0_1_4K DC    (0*2)F'0'                    CR0/CR1 for 4K pages mode
         DC    AL1(CR0_SKEC),AL1(CR0_4K),AL2(0)
         DC    A(SEGTAB4K)
*
CR0_SKEC EQU   X'01'                        Storage-Key Exception Ctl.
_4KBBF   DC    X'00'                        4K-Byte-Block Facility flag
*                                           FF = installed, 00 = not
_NEW370  DC    X'00'                        SSKE/etc supported?
*                                           FF = yes, 00 = not.
                                                                EJECT
         LTORG ,                            literals pool
                                                                EJECT
***********************************************************************
*                      DAT tables
***********************************************************************
                                                                SPACE
         ORG   TEST+X'1000'
                                                                SPACE
SEGTAB2K DC    AL1((16-1)*16),AL3(PAGTAB2K)
         DC    15XL4'00000001'
*
PAGTAB2K DC    AL2((0*_2K)/256)
         DC    AL2((1*_2K)/256)
         DC    AL2((2*_2K)/256)
         DC    AL2((3*_2K)/256)
         DC    AL2((4*_2K)/256)
         DC    AL2((5*_2K)/256)
         DC    AL2((6*_2K)/256)
         DC    AL2((7*_2K)/256)
         DC    AL2((8*_2K)/256)
         DC    AL2((9*_2K)/256)
         DC    AL2((10*_2K)/256)
         DC    AL2((11*_2K)/256)
         DC    AL2((12*_2K)/256)
         DC    AL2((13*_2K)/256)
         DC    AL2((14*_2K)/256)
         DC    AL2((15*_2K)/256)
                                                                SPACE 3
         ORG   TEST+X'1200'
                                                                SPACE
SEGTAB4K DC    AL1((16-1)*16),AL3(PAGTAB4K)
         DC    15XL4'00000001'
*
PAGTAB4K DC    AL2((0*_4K)/256)
         DC    AL2((1*_4K)/256)
         DC    AL2((2*_4K)/256)
         DC    AL2((3*_4K)/256)
         DC    AL2((4*_4K)/256)
         DC    AL2((5*_4K)/256)
         DC    AL2((6*_4K)/256)
         DC    AL2((7*_4K)/256)
         DC    AL2((8*_4K)/256)
         DC    AL2((9*_4K)/256)
         DC    AL2((10*_4K)/256)
         DC    AL2((11*_4K)/256)
         DC    AL2((12*_4K)/256)
         DC    AL2((13*_4K)/256)
         DC    AL2((14*_4K)/256)
         DC    AL2((15*_4K)/256)
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
         END   TEST
