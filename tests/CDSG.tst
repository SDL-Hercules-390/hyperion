*Testcase cmpxchg16 as used by CDSG, STPQ and LPQ instructions
mainsize    1
numcpu      2
sysclear
archlvl     z/Arch
loadcore    "$(testpath)/CDSG.core"
runtest     4.5   # (just to be safe) 
v 900.B0                                
*Done
numcpu      1     # (reset back to default)
