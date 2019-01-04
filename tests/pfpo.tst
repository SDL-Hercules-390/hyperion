
# This file was put into the public domain 2016-10-28
# by John P. Hartmann.  You can use it for anything you like,
# as long as this notice remains.

#----------------------------------------------------------------------
#     PROGRAMMING NOTE: until such time as we can code a PROPER
#     test for this instruction, this test has been temporarily
#     neutered.  It currently does NOTHING and tests NOTHING.
#----------------------------------------------------------------------

*Testcase PFPO-390: Perform Floating Point Operation (ESA/390 mode)

sysclear
archlvl ESA/390

# When run in ESA/390 mode, the test skips the PFPO instruction

loadcore "$(testpath)/pfpo.core"

runtest .1

*Done

#----------------------------------------------------------------------

*Testcase PFPO-Z: Perform Floating Point Operation (z/Arch mode)

sysclear
archlvl z/Arch

# when run in z/Arch mode, the PFPO instruction is tested.  

cr 00=0000000000040060          # (AFP bit 45 required for PFPO)

loadcore "$(testpath)/pfpo.core"

runtest .1

gpr
fpr

*Done

#----------------------------------------------------------------------
