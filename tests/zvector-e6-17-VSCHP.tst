*Testcase zvector-e6-17-VSCHP: VECTOR E6 VRR-b VSCHP instruction
*
*        Zvector E6 instruction tests for VRR-b encoded:
*
*        E674 VSCHP   - DECIMAL SCALE AND CONVERT TO HFP
*
*        # ------------------------------------------------------------
*        #  This tests only the basic function of the instruction.
*        #  Exceptions are NOT tested.
*        # ------------------------------------------------------------
*
mainsize    2
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/zvector-e6-17-VSCHP.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest 2
diag8cmd    disable   # (reset back to default)

*Done
