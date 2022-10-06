*Testcase TRTE-01-basic (Test TRTE instruction)

# --------------------------------------------------------------------
# This tests only the basic function of the TRTE instruction.
# Specification exceptions are NOT tested.
# --------------------------------------------------------------------

# --------------------------------------------------------------------
# need facility bit 26 enabled for:
#       026_PARSING_ENHANCE        *Parsing-Enhancement Facility
#       which is not included in archlvl 390
#       so use backport to 370
# --------------------------------------------------------------------

archlvl  S/370
facility enable  HERC_370_EXTENSION
mainsize    16
numcpu      1

sysclear
loadcore    "$(testpath)/TRTE-01-basic.core" 0x0

runtest     1

*Done
