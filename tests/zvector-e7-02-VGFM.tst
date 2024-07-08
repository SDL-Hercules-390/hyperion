*Testcase zvector-e7-02-VGFM: VECTOR E7 VRR-c instructions
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
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
