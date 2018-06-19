
*Testcase CLCL-et-al (Test CLCL, MVCIN and TRT instructions)

archlvl     390
mainsize    2
numcpu      1
sysclear

loadcore    "$(testpath)/CLCL-et-al.core"

runtest     2         # (NON-timing test duration)
##r           21fd=ff   # (enable timing tests too!)
##runtest     150       # (TIMING too test duration)

*Compare
r 21fe.2

*Want  "Ending test/subtest number"   9510

*Done
