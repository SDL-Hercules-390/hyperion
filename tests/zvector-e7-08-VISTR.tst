*Testcase zvector-e7-08-VISTR
*
*   Zvector E7 instruction tests for VRR-a encoded:
*
*   E75C VISTR  - Vector Isolate String
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

loadcore    "$(testpath)/zvector-e7-08-VISTR.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     10        #
diag8cmd    disable   # (reset back to default)

*Done
