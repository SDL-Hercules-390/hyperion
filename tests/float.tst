*
*
*
* -----------------------------------------------------------
*Testcase HFP Floating Point (370, no AFP, no EXTENSION)
* -----------------------------------------------------------
*
*
*
sysreset
mainsize    2
numcpu      1
sysclear
archlvl     370
cr 0=00000000
FACILITY DISABLE HERC_370_EXTENSION
loadcore    "$(testpath)/float.core"
runtest     0.1
*
*
*
*Compare
r 600.2
*Want "Test/Sub-test Failure Flags = none" 0000
*
*
*
fpr
*Fpr 0 4CC0C0C181818241
*Fpr 2 DA20000020000020
*Fpr 4 4100123456789ABC
*Fpr 6 3F123456789ABC0F
*
*
*
*Done
*
*
*
* -----------------------------------------------------------
*Testcase HFP Floating Point (370, with AFP, no EXTENSION)
* -----------------------------------------------------------
*
*
*
sysreset
mainsize    2
numcpu      1
sysclear
archlvl     370
cr 0=00040000
FACILITY DISABLE HERC_370_EXTENSION
loadcore    "$(testpath)/float.core"
runtest     0.1
*
*
*
*Compare
r 600.2
*Want "Test/Sub-test Failure Flags = none" 0000
*
*
*
fpr
*Fpr 0 4CC0C0C181818241
*Fpr 2 DA20000020000020
*Fpr 4 4100123456789ABC
*Fpr 6 3F123456789ABC0F
*
*
*
*Done
*
*
*
* -----------------------------------------------------------
*Testcase HFP Floating Point (370, no AFP, with EXTENSION)
* -----------------------------------------------------------
*
*
*
sysreset
mainsize    2
numcpu      1
sysclear
archlvl     370
cr 0=00000000
FACILITY ENABLE HERC_370_EXTENSION
loadcore    "$(testpath)/float.core"
runtest     0.1
*
*
*
*Compare
r 600.2
*Want "Test/Sub-test Failure Flags = none" 0000
*
*
*
fpr
*Fpr 0 4CC0C0C181818241
*Fpr 2 DA20000020000020
*Fpr 4 4100123456789ABC
*Fpr 6 3F123456789ABC0F
*
*
*
*Done
*
*
*
* -----------------------------------------------------------
*Testcase HFP Floating Point (370, with AFP, with EXTENSION)
* -----------------------------------------------------------
*
*
*
sysreset
mainsize    2
numcpu      1
sysclear
archlvl     370
cr 0=00040000
FACILITY ENABLE HERC_370_EXTENSION
loadcore    "$(testpath)/float.core"
runtest     0.1
*
*
*
*Compare
r 600.2
*Want "Test/Sub-test Failure Flags = none" 0000
*
*
*
fpr
*Fpr 0 4CC0C0C181818241
*Fpr 2 DA20000020000020
*Fpr 4 4100123456789ABC
*Fpr 6 3F123456789ABC0F
*
*
*
*Done
*
*
* -----------------------------------------------------------
* Clean up  (370 extension facility is disabled by default!)
* -----------------------------------------------------------
*
sysreset
sysclear
FACILITY DISABLE HERC_370_EXTENSION
*
*
*
* -----------------------------------------------------------
*Testcase HFP Floating Point (z/Arch, no AFP)
* -----------------------------------------------------------
*
*
*
sysreset
mainsize    2
numcpu      1
sysclear
archlvl     z/Arch
cr 0=0000000000000000
loadcore    "$(testpath)/float.core"
runtest     0.1
*
*
*
*Compare
r 600.2
*Want "Test/Sub-test Failure Flags = none" 0000
*
*
*
fpr
*Fpr 0 4CC0C0C181818241
*Fpr 2 DA20000020000020
*Fpr 4 4100123456789ABC
*Fpr 6 3F123456789ABC0F
*
*
*
*Done
*
*
*
* -----------------------------------------------------------
*Testcase HFP Floating Point (z/Arch, with AFP enabled)
* -----------------------------------------------------------
*
*
*
sysreset
mainsize    2
numcpu      1
sysclear
archlvl     z/Arch
cr 0=0000000000040000
loadcore    "$(testpath)/float.core"
runtest     0.1
*
*
*
*Compare
r 600.2
*Want "Test/Sub-test Failure Flags = none" 0000
*
*
*
fpr
*Fpr 0 4CC0C0C181818241
*Fpr 2 DA20000020000020
*Fpr 4 4100123456789ABC
*Fpr 6 3F123456789ABC0F
*
*
*
*Done
*
*
*
* -----------------------------------------------------------
*        End of all HFP Floating Point tests...
* -----------------------------------------------------------
*
*
*
