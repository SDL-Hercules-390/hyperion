*Testcase CLCLE-04-performance (Test CLCLE instructions)

#  ----------------------------------------------------------------------------------
#  This ONLY tests the performance of the CLCLE instruction.
#
#  The default is to NOT run performance tests. To enable
#  performance test, uncomment the "#r           21fd=ff "
#  line below.
#
#        Tests:
#              1. CLCLE of 512 bytes
#              2. CLCLE of 512 bytes where operand 1 crosses a page boundary
#              3. CLCLE of 2048 bytes
#              4. CLCLE of 2048 bytes where operand 1 crosses a page boundary
#              5. CLCLE of 2048 bytes where both operand 1 and operand 2
#                                     crosses a page boundary
#
#        Output:
#               For each test, a console line will the generated with timing results,
#               as follows:
#               /         1,000,000 iterations of CLCLE took      38,698 microseconds
#               /         1,000,000 iterations of CLCLE took      48,617 microseconds
#               /         1,000,000 iterations of CLCLE took      49,178 microseconds
#               /         1,000,000 iterations of CLCLE took      68,355 microseconds
#               /         1,000,000 iterations of CLCLE took      69,991 microseconds
#  ----------------------------------------------------------------------------------

archlvl     390
mainsize    3
numcpu      1
sysclear

loadcore    "$(testpath)/CLCLE-04-performance.core" 0x0

#r           21fd=ff   # (enable timing tests)
#runtest     300       # (TIMING test duration)
runtest      0.1       # (NOP TEST)

*Done
