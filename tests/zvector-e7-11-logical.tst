*Testcase zvector-e7-11-logical
*
*   Zvector E7 instruction tests for VRR-c encoded:
*
*   E768 VN     - Vector AND
*   E769 VNC    - Vector AND with Complement
*   E76A VO     - Vector OR
*   E76B VNO    - Vector NOR
*   E76C VNX    - Vector Not Exclusive OR
*   E76D VX     - Vector Exclusive OR
*   E76E VNN    - Vector NAND
*   E76F VOC    - Vector OR with Complement
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

loadcore    "$(testpath)/zvector-e7-11-logical.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5
diag8cmd    disable   # (reset back to default)

*Done
