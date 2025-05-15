*Testcase zvector-e7-27-VERIM
*
*   Zvector E7 instruction tests for VRI-d encoded:
*
*   E772 VERIM  - Vector Element Rotate and Insert Under Mask
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

loadcore    "$(testpath)/zvector-e7-27-VERIM.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
