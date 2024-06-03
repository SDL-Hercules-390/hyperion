*Testcase zvector-e6-07-VSRPR: VECTOR E6 VSRPR instruction
*
*   Zvector E6 tests for VRI-f encoded pack instructions:
*
*   E672 VSRPR  - VECTOR SHIFT AND ROUND DECIMAL REGISTER
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

loadcore    "$(testpath)/zvector-e6-07-VSRPR.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
