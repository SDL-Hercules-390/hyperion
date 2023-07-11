*Testcase GH#572 E7 Prefix CCW
mainsize    2
numcpu      1
sysclear
archmode    z/Arch
attach      0A80 3390 "$(testpath)/3390.cckd64" cu=3990-6
loadcore    "$(testpath)/E7Prefix.core"
diag8cmd    enable
t+a80
##r fff=07    # (if only you want to run just one specific test)
runtest     1.0
##restart
##pause 0.5
diag8cmd    disable
detach      0A80
*Done
