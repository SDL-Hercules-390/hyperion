*Testcase zvector-e7-03-VGFMA
*
*   Zvector E7 instruction tests for VRR-d encoded:
*
*   E7BC VGFMA  - VECTOR GALOIS FIELD MULTIPLY SUM AND ACCUMULATE
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

loadcore    "$(testpath)/zvector-e7-03-VGFMA.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     10        # (2 secs if intrinsic used, 10 otherwise!)
diag8cmd    disable   # (reset back to default)

*Done
