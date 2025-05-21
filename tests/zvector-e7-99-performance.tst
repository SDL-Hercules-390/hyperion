*Testcase zvector-e7-99-performance

#  ----------------------------------------------------------------------------------
#  This ONLY tests the performance of the Zvector instructions.
#
#  The default is to NOT run performance tests. To enable
#  performance tests, uncomment the "#r           600=ff" line
#  below and swap the runtest commands.
#
#  The default test is 30,000,000 iterations of the zvector
#  instruction. The iteration count can be modified at address 604.
#  For example, "r           604=01312D00" would set the
#  iteraction count to 20,000,000.
#
#   Tests: 64 E7 zvector instructions
#
#   Output:
#     For each test, a console line will be generated with timing results,
#     as follows:
#
#     Test # 001: 30,000,000 iterations of "VLREPB V1,0(R5)"    took  503,528 microseconds
#     Test # 002: 30,000,000 iterations of "VESL   V1,V2,3,0"   took  145,584 microseconds
#     Test # 003: 30,000,000 iterations of "VERLL  V1,V2,3,0"   took  163,143 microseconds
#     Test # 004: 30,000,000 iterations of "VESRL  V1,V2,3,0"   took  138,747 microseconds
#     Test # 005: 30,000,000 iterations of "VESRA  V1,V2,3,0"   took  153,151 microseconds
#     ...
#
#  Note: this test requires diagnose 'F14' for OS microsecond timing.
#  ----------------------------------------------------------------------------------

mainsize    2
numcpu      1
sysclear
archlvl     z/Arch

diag8cmd    enable           # (needed for messages to Hercules console)

loadcore    "$(testpath)/zvector-e7-99-performance.core" 0x0

#r           600=ff          # (enable timing tests)
#r           604=01312D00    # (20,000,000 iteration count for test)
#runtest     120             # (TIMING test duration)

runtest      0.1             # (NOP TEST)

diag8cmd    disable          # (reset back to default)

*Done
