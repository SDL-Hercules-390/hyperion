*Testcase TRTRE-02-performance (Test TRTRE instruction)

# ------------------------------------------------------------------------------
#  This ONLY tests the performance of the TRTRE instruction.
#
#  The default is to NOT run performance tests. To enable this performance
#  test, uncomment the "#r 408=ff   # (enable timing tests)" line below.
#
#     Tests:
#
#         All tests are ' TRTRE  R2,R4,12 '
#         where the FC table is 128K in length, FC is 2 bytes and
#         an argument length of 2 bytes.
#
#         1. TRTRE of 512 bytes
#         2. TRTRE of 512 bytes that crosses a page boundary,
#            which results in a CC=3, and a branch back
#            to complete the TRTRE instruction.
#         3. TRTRE of 2048 bytes
#         4. TRTRE of 2048 bytes that crosses a page boundary,
#            which results in a CC=3, and a branch back
#            to complete the TRTRE instruction
#
#     Output:
#
#         For each test, a console line will the generated with timing
#         results, as follows:
#
#              1,000,000 iterations of TRTRE took     881,291 microseconds
#              1,000,000 iterations of TRTRE took   1,258,548 microseconds
#              1,000,000 iterations of TRTRE took   2,567,087 microseconds
#              1,000,000 iterations of TRTRE took   2,944,614 microseconds
# ------------------------------------------------------------------------------

mainsize    16
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/TRTRE-02-performance.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
#r           408=ff    # (enable timing tests)
runtest     200       # (test duration, depends on host)
diag8cmd    disable   # (reset back to default)

*Done
