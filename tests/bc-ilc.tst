*Testcase BC-mode ILC Handling
sysreset
mainsize    2
numcpu      1
sysclear
archlvl     370
loadcore    "$(testpath)/bc-ilc.core"
# Specification, Addressing and Data exceptions are expected
*Program    5
*Program    6
*Program    7
runtest     0.1
*Compare
r 300.4
*Want "00=00xx=0 bytes, 40=01xx=2 bytes, 80=10xx=4 bytes, C0=11xx=6 bytes" 004080C0
r 90.4
*Want "TEA, *not* DXC!" 00000000
*Done
