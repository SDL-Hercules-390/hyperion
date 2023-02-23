*Testcase VERY Simple Basic HFP Floating Point Tests
mainsize    2
numcpu      1
sysclear
archlvl     370
loadcore    "$(testpath)/float.core"
runtest     0.1
*Compare
r 600.2
*Want "Test/Sub-test Failure Flags = none" 0000
fpr
*Fpr 0 4CC0C0C181818241
*Fpr 2 DA20000020000020
*Fpr 4 4100123456789ABC
*Fpr 6 3F123456789ABC0F
*Done
