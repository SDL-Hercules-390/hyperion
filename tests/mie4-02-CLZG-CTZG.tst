*Testcase mie4-02-CLZG-CTZG
*
*   Miscellaneous-instruction-extensions facility 4 instruction
*   tests for RRE encoded:
*
*   B968  CLZG  - COUNT LEADING ZEROS
*   B969  CTZG  - COUNT TRAILING ZEROS CTZG
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

loadcore    "$(testpath)/mie4-02-CLZG-CTZG.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5         #
diag8cmd    disable   # (reset back to default)

*Done
