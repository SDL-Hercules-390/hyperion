
*Testcase CLCE-03-performance (Test CLCLE instructions)

archlvl     390
mainsize    3
numcpu      1
sysclear

loadcore    "$(testpath)/CLCLE-03-performance.core" 0x0

##r           21fd=ff   # (enable timing tests too!)
##runtest     300       # (TIMING too test duration)
runtest     1         # (NON-timing test duration)

*Done
