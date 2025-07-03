*Testcase zvector-e7-23-VLREP
*
*   Zvector E7 instruction tests for VRX encoded:
*
*   E705 VLREP  - Vector Load and Replicate
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

loadcore    "$(testpath)/zvector-e7-23-VLREP.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5
diag8cmd    disable   # (reset back to default)

*Done
