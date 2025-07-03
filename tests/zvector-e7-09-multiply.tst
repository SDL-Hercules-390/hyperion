*Testcase zvector-e7-09-multiply
*
*   Zvector E7 instruction tests for VRR-c encoded:
*
*   E7A1 VMLH   - Vector Multiply Logical High
*   E7A2 VML    - Vector Multiply Low
*   E7A3 VMH    - Vector Multiply High
*   E7A4 VMLE   - Vector Multiply Logical Even
*   E7A5 VMLO   - Vector Multiply Logical Odd
*   E7A6 VME    - Vector Multiply Even
*   E7A7 VMO    - Vector Multiply Odd
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

loadcore    "$(testpath)/zvector-e7-09-multiply.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5
diag8cmd    disable   # (reset back to default)

*Done
