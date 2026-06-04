*Testcase zvector-e7-30-DivideRem
*
*   Zvector E7 instruction tests for VRR-c encoded:
*
*   E7B0 VDL    - VECTOR DIVIDE LOGICAL
*   E7B1 VRL    - VECTOR REMAINDER LOGICAL
*   E7B2 VD     - VECTOR DIVIDE
*   E7B3 VR     - VECTOR REMAINDER
*
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

loadcore    "$(testpath)/zvector-e7-30-DivideRem.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5
diag8cmd    disable   # (reset back to default)

*Done
