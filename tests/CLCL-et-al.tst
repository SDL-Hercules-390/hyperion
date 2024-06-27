
*Testcase CLCL-et-al (Test CLCL, MVCIN and TRT instructions)

archlvl     390
mainsize    2
numcpu      1
sysclear

loadcore    "$(testpath)/CLCL-et-al.core"

##r           21fd=ff   # (enable timing tests too!)
##runtest     300       # (TIMING too test duration)
runtest     1         # (NON-timing test duration)

*Done
