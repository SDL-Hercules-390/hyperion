* This file was put into the public domain 2015-10-22
* by John P. Hartmann.  You can use it for anything you like,
* as long as this notice remains.

*Testcase mhi
sysclear
archmode z
loadcore "$(testpath)/mhi.core"
runtest .1
*Compare
gpr
*Gpr 2 0042
*Gpr 3 0085
*Gpr 4 000000000000000
*Done
