*Testcase zvector-e6-23-VCVBQ
*
*        Zvector E6 instruction tests for VRR-k encoded:
*
*        E64E VCVBQ    - VECTOR CONVERT TO BINARY (128)
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

loadcore    "$(testpath)/zvector-e6-23-VCVBQ.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest 5
diag8cmd    disable   # (reset back to default)

*Done
