*Testcase zvector-e6-19-VCSPH: VECTOR E6 VRR-j VCSPH instruction
*
*        Zvector E6 instruction tests for VRR-j encoded:
*
*        E67D VCSPH   - VECTOR CONVERT HFP TO SCALED DECIMAL
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

loadcore    "$(testpath)/zvector-e6-19-VCSPH.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest 2
diag8cmd    disable   # (reset back to default)

*Done
