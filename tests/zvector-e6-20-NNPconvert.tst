*Testcase zvector-e6-20-NNPconvert
*
*        EXPERIMENTAL pending further PoP definition
*
*        Zvector E6 instruction tests for VRR-a encoded:
*
*        E655 VCNF     - VECTOR FP CONVERT TO NNP
*        E656 VCLFNH   - VECTOR FP CONVERT AND LENGTHEN FROM NNP HIGH
*        E65D VCFN     - VECTOR FP CONVERT FROM NNP
*        E65E VCLFNL   - VECTOR FP CONVERT AND LENGTHEN FROM NNP LOW
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

loadcore    "$(testpath)/zvector-e6-20-NNPconvert.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest 5
diag8cmd    disable   # (reset back to default)

*Done
