*Testcase TRTR-01-basic (Test TRTR instruction)

# ---------------------------------------------------------------------
#  This tests only the basic function of the TRTR instruction.
#  Specification Exceptions are NOT tested.
# ---------------------------------------------------------------------

mainsize    16
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/TRTR-01-basic.core" 0x0

runtest     1

*Done
