#-------------------------------------------------------------------------------

*Testcase Initial control register values

msglevel -debug
numcpu 1

*Compare

#-------------------------------------------------------------------------------
# start with z

archlvl z/arch
cr
*Cr 14 00000000C2000000

#-------------------------------------------------------------------------------
# change 14, then switch to 390; 14 should have initial value

cr 14=abc
*Cr 14 0000000000000ABC
archlvl 390
cr
*Cr 14 C2000000

#-------------------------------------------------------------------------------
# change 2, then switch to 370; 2 should have initial value

cr 2=abc
*Cr 2 00000ABC
archlvl 370
cr
*Cr 2 FFFFFFFF

#-------------------------------------------------------------------------------
# change 14, then switch to 390; 14 should have initial value

cr 14=abc
*Cr 14 00000ABC
archlvl 390
cr
*Cr 14 C2000000

#-------------------------------------------------------------------------------
# change 14, then switch to z; 14 should have initial value

cr 14=abc
*Cr 14 00000ABC
archlvl z/arch
cr
*Cr 14 00000000C2000000

*Done nowait

#-------------------------------------------------------------------------------
