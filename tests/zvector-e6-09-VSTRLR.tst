*Testcase zvector-e6-09-VSTRLR: VECTOR E6 VRS-d VSTRLR instruction
*
*   Zvector E6 tests for VRS-d encoded instructions:
*
*   E63F VECTOR STORE RIGHTMOST WITH LENGTH  (reg)
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

loadcore    "$(testpath)/zvector-e6-09-VSTRLR.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
