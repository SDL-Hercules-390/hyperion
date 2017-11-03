
*Testcase CCW-ILS (CCW Incorrect Length Suppression)

# Prepare test environment

mainsize    1
numcpu      1
sysclear
archlvl     z/Arch
detach      390
attach      390  3390  "$(testpath)/CCWILS.3390-1.comp-z"
loadcore               "$(testpath)/CCW-ILS.core"

t+390                   # (trace device CCWs)
o+390                   # (trace device ORBs)

# Run the test...
runtest   0.25          # (plenty of time)


# Clean up afterwards
detach    390           # (no longer needed)

*Compare
r FFF.1
*Want "Ending test number" 03

*Done
