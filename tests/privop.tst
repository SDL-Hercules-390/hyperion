*
* This file was put into the public domain 2016-11-29
* by John P. Hartmann.  You can use it for anything you like,
* as long as this notice remains.
*
*Testcase privopisk
sysclear
archmode z
loadcore "$(testpath)/privop.core"
*Program 1
runtest .1
*Done

*Testcase privopiske
*Program 2
runtest program .1
*Done

*Testcase privopgo
r 151=00
runtest program .1
*Compare
gpr
*Gpr 0 0000000000000f06
r 88.4
*Want 00020000
*Done
