*Testcase CU12-02-performance (Test CU12 instruction)

# ------------------------------------------------------------------------------
#  This ONLY tests the performance of the CU12 instruction.
#
#  The default is to NOT run performance tests. To enable this performance
#  test, uncomment the "#r 408=ff   # (enable timing tests)" line below.
#
#     Tests:
#
#           All tests are 'CU12  R0,R2' ie. M3=0
#
#           1. CU12 with CC=0 - no crossed pages
#                               source: 61 bytes (28 UTF8 Chars)
#
#           2. CU12 with CC=0 - source cross page
#                               source: 61 bytes (28 UTF8 Chars)
#
#           3. CU12 with CC=0 - target cross page
#                               source: 61 bytes (28 UTF8 Chars)
#
#           4. CU12 with CC=0 - both arguments crossed pages
#                               source: 61 bytes (28 UTF8 Chars)
#
#           5. CU12 with CC=3 - both arguments crossed pages
#                               source: 13,738 bytes only 4095+
#                               processed
#
#     Output:
#
#         For each test, a console line will the generated with timing
#         results, as follows:
#
#            1,000,000 iterations of CU12  took     174,764 microseconds
#            1,000,000 iterations of CU12  took     203,878 microseconds
#            1,000,000 iterations of CU12  took     206,784 microseconds
#            1,000,000 iterations of CU12  took     247,990 microseconds
#            1,000,000 iterations of CU12  took  18,950,147 microseconds
# ------------------------------------------------------------------------------

mainsize    6
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/CU12-02-performance.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
#r           408=ff    # (enable timing tests)
runtest     300       # (test duration, depends on host)
diag8cmd    disable   # (reset back to default)

*Done
