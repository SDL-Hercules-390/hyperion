*Testcase cmpxchg16 as used by CDSG, STPQ and LPQ instructions
mainsize    1
numcpu      2
sysclear
archlvl     z/Arch
loadcore    "$(testpath)/CDSG.core"
runtest     1                                 # (subtst will stop it)
v 400.38
v 438.C
v 444.C
#v 460.10000
*Done

