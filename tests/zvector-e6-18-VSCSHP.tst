*Testcase zvector-e6-18-VSCSHP: VECTOR E6 VRR-b VSCSHP instruction
*
*        Zvector E6 instruction tests for VRR-b encoded:
*
*        E67C VSCSHP - DECIMAL SCALE AND CONVERT AND SPLIT TO HFP
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

loadcore    "$(testpath)/zvector-e6-18-VSCSHP.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest 2
diag8cmd    disable   # (reset back to default)

*Done
