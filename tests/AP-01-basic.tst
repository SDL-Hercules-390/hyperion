*Testcase AP-01-basic (Test AP instruction)
# ---------------------------------------------------------------------
#  This tests only the basic functionality of the AP instruction.
#  Data Exception are tested
# ---------------------------------------------------------------------

mainsize    3
numcpu      1
sysclear
archlvl     390

loadcore    "$(testpath)/AP-01-basic.core" 0x0
*Program    7
runtest     1
*Done
