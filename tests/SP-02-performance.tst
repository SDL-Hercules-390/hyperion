*Testcase AP-02-performance (Test AP instruction)
# ---------------------------------------------------------------------
#  Test only the performance of the AP instruction.
# ---------------------------------------------------------------------

mainsize    3
numcpu      1
sysclear
archlvl     390

loadcore    "$(testpath)/SP-02-performance.core" 0x0
#r a84=ff                          # (uncomment to enable timing test)
runtest     300
*Done
