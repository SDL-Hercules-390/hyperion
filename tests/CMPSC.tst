*Testcase CMPSC (Compression Call)
mainsize  2
numcpu    1
sysclear
archlvl   z/Arch
loadcore  "$(testpath)/CMPSC.core"
runtest   1
*Done

## r 1000.140   # Original data
## r 3000.140   # Expanded data
