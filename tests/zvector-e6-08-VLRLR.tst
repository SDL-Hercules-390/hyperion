*Testcase zvector-e6-08-VLRLR: VECTOR E6 VRS-d VLRLR instruction
*
*   Zvector E6 tests for VRS-d encoded instructions:
*
*   E637 VLRLR  - VECTOR LOAD RIGHTMOST WITH LENGTH (reg)
*
*   # -------------------------------------------------------
*   #  This tests only the basic function of the instruction.
*   #  Exceptions are NOT tested.
*   # -------------------------------------------------------
*
mainsize    2
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/zvector-e6-08-VLRLR.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
