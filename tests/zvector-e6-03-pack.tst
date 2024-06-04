*Testcase zvector-e6-03-pack: VECTOR E6 VSI pack/load instructions
*
*   Zvector E6 instruction tests for VSI encoded:
*
*   E634 VPKZ    - VECTOR PACK ZONED
*   E635 VLRL    - VECTOR LOAD RIGHTMOST WITH LENGTH
*
*   # ------------------------------------------------------------
*   #  This tests only the basic function of the instruction.
*   #  Specification Exceptions are NOT tested.
*   # ------------------------------------------------------------
*
mainsize    2
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/zvector-e6-03-pack.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
