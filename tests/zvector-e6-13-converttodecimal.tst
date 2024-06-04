*Testcase zvector-e6-13-converttodecimal: VECTOR E6 VRI-i instructions
*
*    Zvector E6 tests for VRI-i encoded instructions:
*
*    E658 VCVD    - VECTOR CONVERT TO DECIMAL (32)
*    E65A VCVDG   - VECTOR CONVERT TO DECIMAL (64)
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

loadcore    "$(testpath)/zvector-e6-13-converttodecimal.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
