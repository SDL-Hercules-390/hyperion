*Testcase zvector-e7-29-ShiftDoubleByBit
*
*   Zvector E7 instruction tests for VRI-d encoded:
*
*   E786 VSLD   - Vector Shift Left Double By Bit
*   E787 VSRD   - Vector Shift Right Double By Bit
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

loadcore    "$(testpath)/zvector-e7-29-ShiftDoubleByBit.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
