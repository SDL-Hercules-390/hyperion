*Testcase zvector-e7-07-VGBM
*
*   Zvector E7 instruction tests for VRI-a instruction:
*
*   E744 VGBM   - Vector Generate Byte Mask
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

loadcore    "$(testpath)/zvector-e7-07-VGBM.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     10        #
diag8cmd    disable   # (reset back to default)

*Done
