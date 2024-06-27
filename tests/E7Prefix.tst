*Testcase Various CKD dasd I/O tests
mainsize    2
numcpu      1
sysclear
archmode    z/Arch
attach      0A80 3390 "$(testpath)/3390.cckd64" ro sf="./3390-shadow_*.cckd64" cu=3990-6
sf+a80
loadcore    "$(testpath)/E7Prefix.core"
diag8cmd    enable
t+a80
##r fff=08    # (if only you want to run just one specific test)
runtest     1.0
##restart
##pause 1.0
t-a80
sf-a80 nomerge
diag8cmd    disable
detach      0A80
*Done
