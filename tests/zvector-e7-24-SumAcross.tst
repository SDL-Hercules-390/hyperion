*Testcase zvector-e7-24-SumAcross
*
*   Zvector E7 instruction tests for VRR-c encoded:
*
*   E764 VSUM   - Vector Sum Across Word
*   E765 VSUMG  - Vector Sum Across Doubleword
*   E767 VSUMQ  - Vector Sum Across Quadword
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

loadcore    "$(testpath)/zvector-e7-24-SumAcross.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5
diag8cmd    disable   # (reset back to default)

*Done
