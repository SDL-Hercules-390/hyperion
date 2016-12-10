PRIVOP   TITLE 'Null (well almost) test case for a privop'
*
* This file was put into the public domain 2016-11-29
* by John P. Hartmann.  You can use it for anything you like,
* as long as this notice remains.
*
PRIVOP start 0
 using PRIVOP,15
 org PRIVOP+x'1a0' Restart
 dc x'0001000180000000',ad(go)
 org PRIVOP+x'1c0' SVC new
 dc x'0002000180000000',ad(0)
 org PRIVOP+x'1d0' Program
 dc x'0002000180000000',ad(x'deaddead')
 org PRIVOP+x'200'
go equ *
 isk 0,0
* Stop by disabled program check
 iske 0,0
* Stop by disabled program check
 la 0,x'fff'
 iske 0,0
 svc 0
 end
