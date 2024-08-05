*Testcase zvector-e6-21-VCRNF
*
*        EXPERIMENTAL pending further PoP definition
*
*        Zvector E6 instruction tests for VRR-c encoded:
*
*        E675 VCRNF   - VECTOR FP CONVERT AND ROUND TO NNP
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

loadcore    "$(testpath)/zvector-e6-21-VCRNF.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest 5
diag8cmd    disable   # (reset back to default)

*Done
