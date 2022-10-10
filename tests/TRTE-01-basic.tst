*Testcase TRTE-01-basic (Test TRTE instruction)

# ---------------------------------------------------------------------
#  This tests only the basic function of the TRTE instruction.
#  Specification Exceptions are NOT tested.
# ---------------------------------------------------------------------

mainsize    16
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/TRTE-01-basic.core" 0x0

runtest     1

*Done
