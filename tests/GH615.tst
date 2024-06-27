*Testcase GH615 ICKDSF related changes
mainsize    2
numcpu      1
sysclear
archmode    S/370
attach      0333 3330 "$(testpath)/3330.cckd64" ro sf="./3330-shadow_*.cckd64"
attach      0338 3380 "$(testpath)/3380.cckd64" ro sf="./3380-shadow_*.cckd64"
attach      0339 3390 "$(testpath)/3390.cckd64" ro sf="./3390-shadow_*.cckd64"
sf+333
sf+338
sf+339
loadcore    "$(testpath)/GH615.core"
runtest     1.0
##restart
##pause 1.0
sf-333 nomerge
sf-338 nomerge
sf-339 nomerge
detach 0333
detach 0338
detach 0339
*Done
