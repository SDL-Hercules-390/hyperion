
*Testcase Tape Data Chaining

# Prepare test environment
mainsize  1
numcpu    1
sysclear
archlvl   z/Arch
detach    580
attach    580 3490 "$(testpath)/tape.aws"
loadcore  "$(testpath)/tape.core"

## t+                          # (trace instructions)
t+580                       # (trace device CCWs)

# Run the test...
runtest   0.25              # (plenty of time)

# Clean up afterwards
detach    580               # (no longer needed)

*Compare
r 800.8
*Want "SCSW fields" 00001008 0C403000

*Done
