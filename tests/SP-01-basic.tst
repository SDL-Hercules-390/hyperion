*Testcase SP-01-basic (Test SP instruction)
# ---------------------------------------------------------------------
#  This tests only the basic functionality of the SP instruction.
#  Data Exception are tested
# ---------------------------------------------------------------------

mainsize    3
numcpu      1
sysclear
archlvl     390

loadcore    "$(testpath)/SP-01-basic.core" 0x0
*Program    7
runtest     1
*Done
