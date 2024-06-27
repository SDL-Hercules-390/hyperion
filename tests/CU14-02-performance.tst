*Testcase CU14-02-performance (Test CU14 instruction)

# ------------------------------------------------------------------------------
#  This ONLY tests the performance of the CU14 instruction.
#
#  The default is to NOT run performance tests. To enable this performance
#  test, uncomment the "#r 408=ff   # (enable timing tests)" line below.
#
#     Tests:
#
#           All tests are 'CU14  R0,R2' ie. M3=0
#
#           1. CU14 with CC=0 - no crossed pages
#                               source: 61 bytes (28 UTF8 Chars)
#
#           2. CU14 with CC=0 - source cross page
#                               source: 61 bytes (28 UTF8 Chars)
#
#           3. CU14 with CC=0 - target cross page
#                               source: 61 bytes (28 UTF8 Chars)
#
#           4. CU14 with CC=0 - both arguments crossed pages
#                               source: 61 bytes (28 UTF8 Chars)
#
#           5. CU14 with CC=3 - both arguments crossed pages
#                               source: 13,738 bytes only 4095+
#                               processed
#
#     Output:
#
#         For each test, a console line will the generated with timing
#         results, as follows:
#
#              1,000,000 iterations of CU14  took     211,100 microseconds
#              1,000,000 iterations of CU14  took     208,633 microseconds
#              1,000,000 iterations of CU14  took     239,662 microseconds
#              1,000,000 iterations of CU14  took  11,811,615 microseconds
# ------------------------------------------------------------------------------

mainsize    8
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/CU14-02-performance.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
#r           408=ff    # (enable timing tests)
runtest     300       # (test duration, depends on host)
diag8cmd    disable   # (reset back to default)

*Done
