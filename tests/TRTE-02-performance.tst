*Testcase TRTE-02-performance (Test TRTE instruction)

#  ----------------------------------------------------------------------------------
#  This ONLY tests the performance of the TRTE instruction.
#
#  The default is to NOT run performance tests. To enable
#  performance test, uncomment the "#r           408=ff   # (enable timing tests) "
#  line below.
#
#        Tests:
#              All tests are ' TRTE  R2,R4,12 '
#              where the FC table is 128K in length, FC is 2 bytes and
#              an argument length of 2 bytes.
#
#              1. TRTE of 512 bytes
#              2. TRTE of 512 bytes that crosses a page boundary,
#                 which results in a CC=3, and a branch back
#                 to complete the TRTE instruction.
#              3. TRTE of 2048 bytes
#              4. TRTE of 2048 bytes that crosses a page boundary,
#                 which results in a CC=3, and a branch back
#                 to complete the TRTE instruction
#
#        Output:
#               For each test, a console line will the generated with timing results,
#               as follows:
#                        1,000,000 iterations of TRTE  took     906,809 microseconds
#                        1,000,000 iterations of TRTE  took   1,406,504 microseconds
#                        1,000,000 iterations of TRTE  took   2,369,744 microseconds
#                        1,000,000 iterations of TRTE  took   2,774,618 microseconds
#  ----------------------------------------------------------------------------------

diag8cmd enable         #used for message to Hercules console

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
loadcore    "$(testpath)/TRTE-02-performance.core" 0x0

r           408=ff   # (enable timing tests)
runtest     20        # (test duration, depends on host)

diag8cmd disable

*Done

