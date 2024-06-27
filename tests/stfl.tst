#----------------------------------------------------------------------
*
*Testcase STFL and STFLE
*
*
* (Because this Testcase uses STFLE, it will fail on ESA hardware.)
*
*
#----------------------------------------------------------------------
# This file was put into the public domain 2016-08-20
# by John P. Hartmann.  You can use it for anything you like,
# as long as this notice remains.
#----------------------------------------------------------------------
# Testing of the actual facility bits removed by Fish as agreed upon
# in the GitHub Issue #493 thread.
#----------------------------------------------------------------------
*
sysclear
archmode    z/Arch
loadcore    $(testpath)/stfl.core
*
* (program interrupt 006 = Specification Exception expected)
*Program    6
*
runtest     0.1
*
*Compare
*
gpr
*
*Gpr 2 30000000
*Gpr 3 3          # (adjust if needed when new facilities added)
*Gpr 4 0
*Gpr 5 3          # (adjust if needed when new facilities added)
*
*Done
*
#----------------------------------------------------------------------
