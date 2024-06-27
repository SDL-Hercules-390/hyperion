*Testcase TRTR-02-performance (Test TRTR instruction)

# ------------------------------------------------------------------------------
#  This ONLY tests the performance of the TRTR instruction.
#
#  The default is to NOT run performance tests. To enable this performance
#  test, uncomment the "#r 408=ff   # (enable timing tests)" line below.
#
#     Tests:
#
#           All tests are ' TRTR  0(255,R3),0(R5) '
#           where operand-1 is 256 bytes.
#
#           1. TRTR with CC=0 - no crossed pages
#           2. TRTR with CC=1 - both operand-1 and
#                  function code table cross page boundaries.
#           3. TRTR with CC=2 - both operand-1 and
#                  function code table cross page boundaries.
#
#     Output:
#
#         For each test, a console line will the generated with timing
#         results, as follows:
#
#              1,000,000 iterations of TRTR  took     213,296 microseconds
#              1,000,000 iterations of TRTR  took     244,180 microseconds
#              1,000,000 iterations of TRTR  took     253,886 microseconds
# ------------------------------------------------------------------------------

mainsize    16
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/TRTR-02-performance.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
#r           408=ff    # (enable timing tests)
runtest     300       # (test duration, depends on host)
diag8cmd    disable   # (reset back to default)

*Done
