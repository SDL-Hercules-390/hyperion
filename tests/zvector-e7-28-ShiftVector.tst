*Testcase zvector-e7-28-ShiftVector
*
*   Zvector E7 instruction tests for VRR-c encoded:
*
*   E770 VESLV  - Vector Element Shift Left Vector
*   E778 VESRLV - Vector Element Shift Right Logical Vector
*   E77A VESRAV - Vector Element Shift Right Arithmetic Vector
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

loadcore    "$(testpath)/zvector-e7-28-ShiftVector.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5
diag8cmd    disable   # (reset back to default)

*Done
