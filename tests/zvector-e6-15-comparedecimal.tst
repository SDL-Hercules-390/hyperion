*Testcase zvector-e6-15-comparedecimal: VECTOR E6 VRR-h instruction
*
*   Zvector E6 tests for VRR-h encoded instruction:
*
*   E677 VCP     - VECTOR COMPARE DECIMAL
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

loadcore    "$(testpath)/zvector-e6-15-comparedecimal.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
