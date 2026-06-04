*Testcase zvector-e6-14-testdecimal
*
*   Zvector E6 tests for VRR-g encoded instruction:
*
*   E65F VTP     - VECTOR TEST DECIMAL
*
*   # -------------------------------------------------------
*   #  This tests only the basic function of the instruction.
*   #  Exceptions are NOT tested.
*   #
*   # Note: errors may indicate VTPX as the instruction in
*   #       error. This is the VTPX macro used to generate
*   #       VTP instructions with a non-zero I3
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
