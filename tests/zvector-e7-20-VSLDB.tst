*Testcase zvector-e7-20-VSLDB
*
*   Zvector E7 instruction tests for VRI-d encoded:
*
*   E777 VSLDB  - Vector Shift Left Double By Byte
*
*   # ------------------------------------------------------------
*   #  This tests only the basic function of the instructions.
*   #  Exceptions are NOT tested.
*   # ------------------------------------------------------------
*
mainsize    2
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/zvector-e7-20-VSLDB.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
