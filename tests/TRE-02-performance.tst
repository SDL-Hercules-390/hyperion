*Testcase TRE-02-performance (Test TRE instructions)

#  ----------------------------------------------------------------------------------
#  This ONLY tests the performance of the TRE instruction.
#
#  The default is to NOT run performance tests. To enable
#  performance test, uncomment the "#r           21fd=ff "
#  line below.
#
#        Tests:
#              1. TRE of 512 bytes
#              2. TRE of 512 bytes that crosses a page boundary,
#                 which results in CC=3, and a branch back
#                 to complete the TRE instruction. So, 2 TRE
#                 are executed compared to test 1.
#              3. TRE of 2048 bytes
#              4. TRE of 2048 bytes that crosses a page boundary,
#                 which results in CC=3, and a branch back
#                 to complete the TRE instruction
#        Output:
#               For each test, a console line will the generated with timing results,
#               as follows:
#               /         1,000,000 iterations of TRE   took     258,117 microseconds
#               /         1,000,000 iterations of TRE   took     305,606 microseconds
#               /         1,000,000 iterations of TRE   took   1,016,256 microseconds
#               /         1,000,000 iterations of TRE   took   1,056,531 microseconds
#  ----------------------------------------------------------------------------------

archlvl     390
mainsize    3
numcpu      1
sysclear

loadcore    "$(testpath)/TRE-02-performance.core" 0x0

#r           21fd=ff    # (uncomment to enable timing tests!)
runtest     10         # (test duration, depends on host)

*Done
