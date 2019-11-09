*Testcase for CDSG, STPQ and LPQ Instructions
mainsize    1
numcpu      2
sysclear
archlvl     z/Arch
loadcore    "$(testpath)/CDSG.core"
runtest     1 
v 900.38                                
v 940.70
*Compare
v 940.10
*Want "Success ! CDSQ, STPQ and LPQ: OK"  E2A48383 85A2A240 5A40C3C4 E2C76B40
#v 960.100  
*Done
numcpu      1     # (reset back to default)
