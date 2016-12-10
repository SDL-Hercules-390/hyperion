prob     TITLE 'Null problem state test case.'
*
* This file was put into the public domain 2016-11-29
* by John P. Hartmann.  You can use it for anything you like,
* as long as this notice remains.
*

prob start 0
 using prob,15
 org prob+x'1a0' Restart
*        P
 dc x'0001000180000000',ad(go)
 org prob+x'1c0' SVC new
 dc x'0002000180000000',ad(0)
 org prob+x'1d0' Program
 dc x'0002000180000000',ad(x'deaddead')
 org prob+x'200'
go equ *
 svc 0 OK
 end
