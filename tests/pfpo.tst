

*Testcase PFPO    ORIGINAL
mainsize    3
numcpu      1
sysclear
archlvl     z/Arch
loadcore    "$(testpath)/pfpo.core"
*
* 6.283185307179586476925286766559004  (original input)
*
r 600=39FFD2B32D873E6E                # original input
r 608=A9DAAD5ABE6B6404                # original input
*
defsym  expected1   416487ED          # expected original output
defsym  expected2   5110B461          # expected original output
*
r  700=$(expected1)$(expected2)       # expected original output
*
runtest     .1                        # RUN THE TEST...
*
r  600.10                             # input
*
r  700.8                              # expected output
*
*Compare
r  710.8
*Want   $(expected1) $(expected2)
*
*Done





*Testcase PFPO    TRUNCATED
mainsize    3
numcpu      1
sysclear
archlvl     z/Arch
loadcore    "$(testpath)/pfpo.core"
*
* 6.283185307179586476925286766559     (truncated input)
*
r 600=22008064ACCB61CF                # truncated input
r 608=9BAA76AB56AF9AD9                # truncated input
*
defsym  expected1   416487ED          # expected truncated output
defsym  expected2   5110B461          # expected truncated output
*
r  700=$(expected1)$(expected2)       # expected truncated output
*
runtest     .1                        # RUN THE TEST...
*
r  600.10                             # input
*
r  700.8                              # expected output
*
*Compare
r  710.8
*Want   $(expected1) $(expected2)
*
*Done
