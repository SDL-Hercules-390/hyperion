*Testcase zvector-e7-17-AddSub
*
*   Zvector E7 instruction tests for VRR-c encoded:
*
*   E7F1 VACC   - Vector Add Compute Carry
*   E7F3 VA     - Vector Add
*   E7F5 VSCBI  - Vector Subtract Compute Borrow Indication
*   E7F7 VS     - Vector Subtract
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

loadcore    "$(testpath)/zvector-e7-17-AddSub.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5
diag8cmd    disable   # (reset back to default)

*Done
