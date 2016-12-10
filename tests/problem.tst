*
* This file was put into the public domain 2016-11-29
* by John P. Hartmann.  You can use it for anything you like,
* as long as this notice remains.
*

* Null problem state program for education

*Testcase problem
sysclear
archmode z
loadcore "$(testpath)/problem.core"
runtest .1
*Compare
r 88.4
*Want 00020000
*Done
