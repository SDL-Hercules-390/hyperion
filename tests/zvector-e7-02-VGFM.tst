*Testcase zvector-e7-02-VGFM
*
*   Zvector E7 instruction tests for VRR-c encoded:
*
*   E7B4 VGFM   - VECTOR GALOIS FIELD MULTIPLY SUM
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

loadcore    "$(testpath)/zvector-e7-02-VGFM.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     10        # (2 secs if intrinsic used, 10 otherwise!)
diag8cmd    disable   # (reset back to default)

*Done
