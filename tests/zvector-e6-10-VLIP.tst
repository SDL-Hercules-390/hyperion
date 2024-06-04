*Testcase zvector-e6-10-VLIP: VECTOR E6 VRI-h VLIP instruction
*
*   Zvector E6 tests for VRI-h encoded instruction:
*
*   E649 VLIP    - VECTOR LOAD IMMEDIATE DECIMAL
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

loadcore    "$(testpath)/zvector-e6-10-VLIP.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
