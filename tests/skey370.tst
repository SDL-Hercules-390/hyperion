*Testcase skey370
# Storage Keys: pure System/370 only
mainsize    3
numcpu      1
sysclear
archlvl     370
loadcore    "$(testpath)/skey370.core"
f-          5000
f-          5800
runtest     0.2
*Done
