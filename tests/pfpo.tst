
# This file was put into the public domain 2016-10-28
# by John P. Hartmann.  You can use it for anything you like,
# as long as this notice remains.

*Testcase pfpo-esa
sysclear
archlvl esa/390

# when run in ESA/390 mode, the test skips the PFPO instruction

loadcore "$(testpath)/pfpo.core"

runtest .1

*Done

*Testcase pfpo-z
sysclear
archlvl esame

# when run in z/Arch mode, PFPO is attempted.  
# An operation exception is expected. 

loadcore "$(testpath)/pfpo.core"
*Program 0001
runtest .1
gpr

*Done


