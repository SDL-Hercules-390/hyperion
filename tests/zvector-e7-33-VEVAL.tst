*Testcase zvector-e7-33-VEVAL
*
*   Zvector E7 instruction tests for VRI-k encoded:
*
*   E788 VEVAL  - Vector Evaluate
*
*   # ------------------------------------------------------------
*   #  This tests only the basic function of the instruction.
*   #  Exceptions are NOT tested.
*   # ------------------------------------------------------------
*
mainsize    1
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/zvector-e7-33-VEVAL.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5         #
diag8cmd    disable   # (reset back to default)

*Done
