*Testcase CLCL-et-al (Test CLCL, MVCIN and TRT instructions)

archlvl     390
mainsize    2
numcpu      1
sysclear

loadcore    "$(testpath)/CLCL-et-al.core"

runtest     2         # (NON-timing test duration)
##r           1fff=ff   # (enable timing tests too!)
##runtest     360       # (TIMING too test duration)

*Compare
r 2000.2

*Want  "Ending test/subtest number (NON-timing)"   0402
##*Want  "Ending test/subtest number (TIMING too)"   9401

*Done
