*Testcase zvector-e6-14-testdecimal: VECTOR E6 VRR-g instruction
*
*   Zvector E6 tests for VRR-g encoded instruction:
*
*   E65F VTP     - VECTOR TEST DECIMAL
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

loadcore    "$(testpath)/zvector-e6-14-testdecimal.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
