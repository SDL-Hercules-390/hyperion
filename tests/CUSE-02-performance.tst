*Testcase CUSE-02-performance (Test CUSE instruction)

# ------------------------------------------------------------------------------
#  This ONLY tests the performance of the CUSE instruction.
#
#  The default is to NOT run performance tests. To enable this performance
#  test, uncomment the "#r 408=ff   # (enable timing tests)" line below.
#
#  Tests:
#       Both opernand-1 and operand-2 cross a page boundary.
#
#        1. CUSE of 512 bytes - substring length 1
#        2. CUSE of 512 bytes - substring length 4
#        3. CUSE of 512 bytes - substring length 8
#        4. CUSE of 512 bytes - substring length 32
#        5. CUSE of 512 bytes - substring length 32 (different strings)
#        6. CUSE of 4160 (4096+64) bytes - substring length 32
#            which results in a CC=3, and a branch back
#            to complete the CUSE instruction.
#
#     Output:
#
#        For each test, a console line will the generated with timing
#        results, as follows:
#
#        1,000,000 iterations of CUSE  took     406,437 microseconds
#        1,000,000 iterations of CUSE  took   2,496,775 microseconds
#        1,000,000 iterations of CUSE  took   1,205,304 microseconds
#        1,000,000 iterations of CUSE  took     340,440 microseconds
#        1,000,000 iterations of CUSE  took   2,210,040 microseconds
#        1,000,000 iterations of CUSE  took   2,313,153 microseconds
# ------------------------------------------------------------------------------

mainsize    16
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/CUSE-02-performance.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
#r           408=ff    # (enable timing tests)
runtest     300       # (test duration, depends on host)
diag8cmd    disable   # (reset back to default)

*Done
