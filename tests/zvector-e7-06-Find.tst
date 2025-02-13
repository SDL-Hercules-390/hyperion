*Testcase zvector-e7-06-Find
*
*   Zvector E7 instruction tests for VRR-b encoded:
*
*   E780 VFEE   - Vector Find Element Equal
*   E781 VFENE  - Vector Find Element Not Equal
*   E782 VFAE   - Vector Find Any Element Equal
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

loadcore    "$(testpath)/zvector-e7-06-Find.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     10        #
diag8cmd    disable   # (reset back to default)

*Done
