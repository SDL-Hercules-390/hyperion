*Testcase TRE-01-basic (Test TRE instructions)

#  ----------------------------------------------------------------------------------
#  This ONLY tests the basic function of the TRE instruction.
#
#  For TRE performance timing, see the TRE-02-performance.tst test.
#  ----------------------------------------------------------------------------------

archlvl     390
mainsize    3
numcpu      1
sysclear

loadcore    "$(testpath)/TRE-01-basic.core" 0x0

runtest     1

*Done
