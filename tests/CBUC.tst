*Testcase CBUC (Concurrent Block Update Consistency)
defsym      testdur  10   # (maximum test duration in seconds)
mainsize    1
numcpu      2
sysclear
archlvl     z/Arch
loadcore    "$(testpath)/CBUC.core"
script      "$(testpath)/CBUC.subtst"  &      # ('&' = async thread!)
runtest     300                               # (subtst will stop it)
*Done
numcpu      1     # (reset back to default)
