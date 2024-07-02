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
# June 2024: Test proper detection of disabling a specific facility.
#----------------------------------------------------------------------
*
sysclear
archmode    z/Arch
*
fac dis 19        # temporarily disable this facility for testing
fac q 19              
fac q raw
*
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
#                 Make sure facility is still disabled
fac q 19
fac q raw
*
#                 Verify STFLE stored what it SHOULD have...
*
*Compare 
r 212.1           # Where STFLE stored facility 19's byte
*Want EF          # facility 19 = X'10' bit should be OFF
*
fac ena 19        # put back to the way it was
*
*
*Done
*
#----------------------------------------------------------------------
