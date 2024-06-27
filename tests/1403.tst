*Testcase 1403 printer Load UCS Buffer

mainsize    1
numcpu      1
archlvl     S/370
sysclear    # must FOLLOW archlvl command!

detach  000E
attach  000E  1403  "3211.txt"  crlf    # (same file as 3211 test)

r 00=0008000000000200       # Restart New PSW
r 68=000A00000000DEAD       # Program Check New PSW
r 78=000A000000000000       # I/O Interrupt New PSW

r 200=41100500              # R1 --> Channel program
r 204=50100048              # Store into CAW
r 208=9C00000E              # SIO X'00E'
r 20C=4730021C              # cc=2, cc=3: FAIL
r 210=47400218              # cc=1: CSW stored
r 214=82000400              # Wait for I/O interrupt
r 218=82000078              # Test finished
r 21C=82000408              # SIO failed

r 400=020A000000000000      # Wait on I/O PSW
r 408=000A000000EEEEEE      # SIO failure PSW

r 500=EB00000060000001      # UCS Gate Load
r 508=FB000000200000F0      # Load UCS Buffer

t+00E                       # (trace device I/O)
runtest   1                 # (PLENTY of time)

*Compare                    # (expected result?)
r 44.4
*Want "Good CSW" 0C000000

detach    000E              # (no longer needed)

*Done
