*Testcase zvector-e6-06-VPKZR: VECTOR E6 VPKZR: packed zoned register instruction
*
*   Zvector E6 tests for VRI-f encoded pack instructions:
*
*   E670 VPKZR  - VECTOR PACK ZONED REGISTER VPKZR
*
*   # ------------------------------------------------------------
*   #  This tests only the basic function of the instruction.
*   #  Exceptions are NOT tested.
*   # ------------------------------------------------------------
*
mainsize    2
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/zvector-e6-06-VPKZR.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
