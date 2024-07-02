*Testcase zvector-e6-04-unpack: VECTOR E6 VSI unpack/store instructions
*
*    Zvector E6 instruction tests for VSI encoded:
*
*    E63C VUPKZ   - VECTOR UNPACK ZONED
*    E63D VSTRL   - VECTOR STORE RIGHTMOST WITH LENGTH
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

loadcore    "$(testpath)/zvector-e6-04-unpack.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
