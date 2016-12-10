SEMIPRIV TITLE 'Test a semiprivileged instruction'
*
* This file was put into the public domain 2016-11-30
* by John P. Hartmann.  You can use it for anything you like,
* as long as this notice remains.
*
SEMIPRIV start 0
 using SEMIPRIV,15
 org SEMIPRIV+x'1a0' Restart
 dc x'0000000180000000',ad(go)
 org SEMIPRIV+x'1c0' SVC new
 dc x'0002000180000000',ad(0)
 org SEMIPRIV+x'1d0' Program
 dc x'0002000180000000',ad(x'deaddead')
 org SEMIPRIV+x'200'
go equ *
* Supervisor state part
 lctl 3,3,cr3
 spka x'80' Forbidden in problem state, but not for me
 ipk , Current key -> r2
 lgr 4,2 Save proof
 sgr 0,0
 lpsw prob
goprob ds 0h
 spka x'40' Should be OK
* ipk here would privop since CR0.36 is 0.  Test separately.
 spka x'80'
 svc 1 If we survive

prob dc 0d'0',x'00f9',h'0',a(goprob) Problem state and 24-bit
cr3 dc a(x'7f7f0000') disallow key 0 and 8
 end
