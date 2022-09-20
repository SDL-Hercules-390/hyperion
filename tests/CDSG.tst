*Testcase cmpxchg16 as used by CDSG, STPQ and LPQ instructions
mainsize    1
numcpu      2
sysclear
archlvl     z/Arch
loadcore    "$(testpath)/CDSG.core"
runtest     1
v 900.38
v 940.70
#v 960.100
*Done
numcpu      1     # (reset back to default)
