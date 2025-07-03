*Testcase zvector-e7-12-elementShift
*
*        Zvector E7 tests for VRS-a encoded instructions:
*
*        E730 VESL   - Vector Element Shift Left
*        E733 VERLL  - Vector Element Rotate Left Logical
*        E738 VESRL  - Vector Element Shift Right Logical
*        E73A VESRA  - Vector Element Shift Right Arithmetic
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

loadcore    "$(testpath)/zvector-e7-12-elementShift.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5
diag8cmd    disable   # (reset back to default)

*Done
