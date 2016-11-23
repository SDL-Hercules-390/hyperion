* PFPO TITLE 'Perform floating point operation. ESA and z mode.'
 
* This file was put into the public domain 2016-05-05
* by John P. Hartmann. You can use it for anything you like,
* as long as this notice remains.
 
* Note that this test runs in problem state. As a result STFL is
* not a good idea; nor is lpsw for that matter. 

* So we terminate by SVC, and we determine architecture mode
* by seeing where the restart old psw was stored.  
 
pfpo start 0
 using pfpo,15
 
 dc a(x'00090000',go) ESA restart new PSW at 0
ropsw390 ds 0xl8  ESA/390 Restart old psw
 ds x'ffffffffffffffff'  If no longer ones, then ESA/390 mode
 
 org pfpo+x'60' ESA SVC new PSW
 dc x'000a0000',a(x'0')    normal end of job
  
 org pfpo+x'68' ESA program new
 dc x'000a0000',a(x'deaddead')

 org pfpo+x'120' 
ropswz ds 0xl16   z/Arch Restart old psw
 dc 16x'ff'  If no longer ones, then z/Arch mode
 
 org pfpo+x'1a0' z restart new PSW
 dc x'0001000180000000',ad(go)
 
 org pfpo+x'1c0' z SVC Program new
 dc x'0002000180000000',ad(0)
 
 org pfpo+x'1d0' z Program new
 dc x'0002000180000000',ad(x'deaddead')
 
 using pfpo,15
 org pfpo+x'200'
go ds 0h
 cli ropsw390,x'ff'   Running in ESA/390 mode?
 bne eoj ..yes, no PFPO support, skip instruction execution 
 sr 3,3 clear r3 for later condition code store
 l 0,=a(x'80000000') Test for invalid function
 pfpo 
 ipm 3 collect condition code from PSW
 st 3,condcode
eoj ds 0h
 svc 0 load hardwait psw.  
 ltorg
 org pfpo+x'300'
condcode dc f'0'
 end
