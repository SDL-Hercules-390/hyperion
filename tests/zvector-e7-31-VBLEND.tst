*Testcase zvector-e7-31-VBLEND
*
*   Zvector E7 instruction tests for VRR-d encoded:
*
*   E789 VBLEND - Vector Blend
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

loadcore    "$(testpath)/zvector-e7-31-VBLEND.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5         #
diag8cmd    disable   # (reset back to default)

*Done
