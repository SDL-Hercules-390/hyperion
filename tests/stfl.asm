 TITLE   'Store facilities list [extended].'                                    
                                                                                
* This file was put into the public domain 2016-08-20                           
* by John P. Hartmann.  You can use it for anything you like,                   
* as long as this notice remains.                                               
                                                                                
stfl start 0                                                                    
 using *,15                                                                     
 org stfl+x'c8'                                                                 
privfl ds f Stored in the PSA by STFL                                           
 org stfl+x'1a0' Restart new                                                    
 dc ad(0,go)                                                                    
 org stfl+x'1d0' Program new                                                    
 dc x'00020000',a(0,0,0)                                                        
 org stfl+x'200'                                                                
sefl ds 2d                                                                      
efl ds 16d  Should last a while                                                 
go equ *                                                                        
 stfl 0   32 bits at 200 decimal = x'c8'                                        
 xgr 0,0  Short                                                                 
 stfle sefl                                                                     
 ipm 2   Should be 3                                                            
 lgr 3,0 Should be 1                                                            
 lhi 0,7 Store up to 8                                                          
 stfle efl                                                                      
 ipm 4                                                                          
 lgr 5,0                                                                        
 stfle 1 Should specification due to unalignment                                
 ltr 14,14                                                                      
 bnzr 14                                                                        
 lpswe stop                                                                     
stop dc 0d'0',x'0002 0000',2a(0),a(x'deadbeef')                                 
 end                                                                            
