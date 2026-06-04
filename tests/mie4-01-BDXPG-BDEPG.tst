*Testcase mie4-01-BDXPG-BDEPG
*
*   Miscellaneous-instruction-extensions facility 4 instruction
*   tests for RRF-a encoded:
*
*   B96C  BEXTG - Bit Extract
*   B96D  BDEPG - Bit Deposit
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

loadcore    "$(testpath)/mie4-01-BDXPG-BDEPG.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5         #
diag8cmd    disable   # (reset back to default)

*Done
