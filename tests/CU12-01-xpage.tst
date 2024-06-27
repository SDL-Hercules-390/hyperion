*Testcase CU12-01-xpage (Test CU12 instruction)

# ---------------------------------------------------------------------
#  This program tests functioning of the CU12 instruction
#  across page boundaties. Only M3=0 is tested and CC=0 is expected.
#  Specification exceptions are not tested.
# ---------------------------------------------------------------------

mainsize    4
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/CU12-01-xpage.core" 0x0

runtest     2

*Done
