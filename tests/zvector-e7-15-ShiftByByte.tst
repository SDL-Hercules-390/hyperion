*Testcase zvector-e7-15-ShiftByByte
*
*   Zvector E7 instruction tests for VRR-c encoded:
*
*   E775 VSLB   - Vector Shift Left By Byte
*   E77D VSRLB  - Vector Shift Right Logical By Byte
*   E77F VSRAB  - Vector Shift Right Arithmetic By Byte
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

loadcore    "$(testpath)/zvector-e7-15-ShiftByByte.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5
diag8cmd    disable   # (reset back to default)

*Done
