*Testcase skey390-9z
# Storage Keys: S/390 then z/Arch
mainsize    3
numcpu      1
sysclear
archlvl     ESA/390
#
#
CPUVERID    C8 FORCE      # (NEW starting with version 4.4)
#
#
loadcore    "$(testpath)/skey390z.core"
f-          A000
f-          B000
runtest     0.2
*Done

*Testcase skey390-z
# Storage Keys: Pure z/Arch only
mainsize    3
numcpu      1
sysclear
archlvl     z/Arch
#
#
CPUVERID    C8 FORCE      # (NEW starting with version 4.4)
#
#
loadcore    "$(testpath)/skey390z.core"
f-          A000
f-          B000
runtest     0.2
*Done

CPUVERID    FD            # (reset back to original default value)
