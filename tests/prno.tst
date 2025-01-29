*Testcase PRNO

# Jurgen Winkelmann's MSA-5 'PRNO' instruction test
#
#   This module tests the PRNO instruction
#   in a standalone environment.
#
# Operation -
#
#   PRNOTEST exercises PRNO QUERY, DRNG, and TRNG functions
#   and does plausibility checks on the results.
#
#   - If all tests pass, PRNOTEST enters a disabled wait state
#     with a PSW address of X'0000000000000000' (all zeros).
#
#   - If a test fails, the test sequence is aborted
#     and a disabled wait state X'000000000000DEAD' is entered.

sysclear
archlvl z/Arch
loadcore "$(testpath)/prno.core"
runtest 2.0
*Done
