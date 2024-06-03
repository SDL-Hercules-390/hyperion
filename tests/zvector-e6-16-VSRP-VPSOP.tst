*Testcase zvector-e6-16-VSRP-VPSOP: VECTOR E6 VRI-g instructions
*
*    Zvector E6 tests for VRI-g encoded instruction:
*
*    E659 VSRP   - VECTOR SHIFT AND ROUND DECIMAL
*    E65B VPSOP  - VECTOR PERFORM SIGN OPERATION DECIMAL
*
*    # ------------------------------------------------------------
*    #  This tests only the basic function of the instruction.
*    #  Exceptions are NOT tested.
*    # ------------------------------------------------------------
*
mainsize    2
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/zvector-e6-16-VSRP-VPSOP.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
