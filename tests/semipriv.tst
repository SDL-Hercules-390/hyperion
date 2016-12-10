*
* This file was put into the public domain 2016-11-30
* by John P. Hartmann.  You can use it for anything you like,
* as long as this notice remains.
*
*Testcase semipriv
sysclear
archmode z
loadcore "$(testpath)/semipriv.core"
*Program 2
runtest .1
*Compare
gpr
*Gpr 4 000000000000080
# No SVCs
r 88.4
*Want 00000000
# key 4 and problem state in prog old PSW
r 150.2
*Want 0041
*Done
