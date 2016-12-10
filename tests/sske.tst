# This test file was generated from offline assembler source
# by txt2tst.rexx  1 Dec 2016 11:41:01
# Treat as object code.  That is, modifications will be lost.
# assemble and listing files are provided for information only.
*Testcase sske#1                                                                
* sske#1:  Test for sske being privileged.                                      
sysclear                                                                        
archmode z                                                                      
r    1A0=00010001800000000000000000000200
r    1D0=0002000180000000FFFFFFFFDEADDEAD
r    200=A7282004A73800F0B22B0032B2B20210
r    210=00020001800000000000000000000BAD
*Program 0002                                                                   
runtest .1                                                                      
*Done                                                                           
*Testcase sske#2                                                                
* sske#2:  Basic set key operation.                                             
sysclear                                                                        
archmode z                                                                      
r    1A0=00000001800000000000000000000200
r    1D0=0002000180000000FFFFFFFFDEADDEAD
r    200=A7282004413000F0B22B0032B2B20210
r    210=00020001800000000000000000000000
runtest .1                                                                      
*Compare                                                                        
r 2000.8                                                                        
*Key F0                                                                         
gpr                                                                             
*Gpr 2 2004                                                                     
* sske#3:  Test prefixing with single page frame.                               
sysclear                                                                        
archmode z                                                                      
r    1A0=00000001800000000000000000002000
r    1D0=0002000180000000FFFFFFFFDEADDEAD
r   41D0=0002000180000000FFFFFFFFDEADBEAF
r   2000=0DC0B210C026A7281004413000F0B22B
r   2010=0032B2B2C01600000002000180000000
r   2020=000000000000000000004000
runtest .1                                                                      
*Compare                                                                        
abs 1000.8                                                                      
*Key 00                                                                         
r 1000.8                                                                        
*Key F0                                                                         
abs 5000.8                                                                      
*Key F0                                                                         
r 5000.8                                                                        
*Key 00                                                                         
* sske#4:  Setting multiple keys without prefixing.                             
sysclear                                                                        
archmode z                                                                      
r    1A0=00000001800000000000000000000200
r    1D0=0002000180000000FFFFFFFFDEADDEAD
r    200=E32002280004A73800F0B22B1032B2B2
r    210=02180000000000000002000180000000
r    220=00000000000000000000000000002004
runtest .1                                                                      
*Compare                                                                        
abs  0.8                                                                        
*Key 06                                                                         
abs 1000.8                                                                      
*Key 00                                                                         
r ff000.8                                                                       
*Key F0                                                                         
r ff000.8                                                                       
*Key F0                                                                         
r 100000.8                                                                      
*Key 00                                                                         
gpr                                                                             
*Gpr 2 0000000000100004                                                         
r 2000.8                                                                        
*Key F0                                                                         
r 5000.8                                                                        
*Key F0                                                                         
* SSKE#5: Test prefixing with multiple page frames.                             
sysclear                                                                        
archmode z                                                                      
r    1A0=00000001800000000000000000002000
r    1D0=0002000180000000FFFFFFFFDEADDEAD
r   41D0=0002000180000000FFFFFFFFDEADBEAF
r   2000=0DC0B210C02EE320C0260004A73800F0
r   2010=B22B1032B2B2C0160002000180000000
r   2020=00000000000000000000000000001004
r   2030=00004000
runtest .1                                                                      
*Compare                                                                        
abs  0.8                                                                        
*Key 06                                                                         
abs 1000.8                                                                      
*Key F0                                                                         
r ff000.8                                                                       
*Key F0                                                                         
r ff000.8                                                                       
*Key F0                                                                         
r 100000.8                                                                      
*Key 00                                                                         
gpr                                                                             
*Gpr 2 0000000000100004                                                         
r    0.8                                                                        
*Key f0                                                                         
r 1000.8                                                                        
*Key F0                                                                         
abs 2000.8                                                                      
*Key F4                                                                         
r 2000.8                                                                        
*Key F4                                                                         
r 5000.8                                                                        
*Key F0                                                                         
sysclear                                                                        
archmode z                                                                      
r    1A0=00000000800000000000000000002000
r    1D0=0002000180000000FFFFFFFFDEADDEAD
r   41D0=0002000180000000FFFFFFFFDEADBEAF
r   2000=0DC0B210C02EE320C0260004A73800F0
r   2010=B22B1032B2B2C0160002000180000000
r   2020=0000000000000000FFFFFFFF80001004
r   2030=00004000
runtest .1                                                                      
*Compare                                                                        
abs  0.8                                                                        
*Key 06                                                                         
abs 1000.8                                                                      
*Key F0                                                                         
r ff000.8                                                                       
*Key F0                                                                         
r ff000.8                                                                       
*Key F0                                                                         
r 100000.8                                                                      
*Key 00                                                                         
gpr                                                                             
*Gpr 2 ffffffff00100004                                                         
r    0.8                                                                        
*Key f0                                                                         
r 1000.8                                                                        
*Key F0                                                                         
abs 2000.8                                                                      
*Key F4                                                                         
r 2000.8                                                                        
*Key F4                                                                         
r 5000.8                                                                        
*Key F0                                                                         
sysclear                                                                        
archmode z                                                                      
r    1A0=00000000000000000000000000002000
r    1D0=0002000180000000FFFFFFFFDEADDEAD
r   41D0=0002000180000000FFFFFFFFDEADBEAF
r   2000=0DC0B210C02EE320C0260004A73800F0
r   2010=B22B1032B2B2C0160002000180000000
r   2020=0000000000000000FFFFFFFFFF001004
r   2030=00004000
runtest .1                                                                      
*Compare                                                                        
abs  0.8                                                                        
*Key 06                                                                         
abs 1000.8                                                                      
*Key F0                                                                         
r ff000.8                                                                       
*Key F0                                                                         
r ff000.8                                                                       
*Key F0                                                                         
r 100000.8                                                                      
*Key 00                                                                         
gpr                                                                             
*Gpr 2 ffffffff00100004                                                         
r    0.8                                                                        
*Key f0                                                                         
r 1000.8                                                                        
*Key F0                                                                         
abs 2000.8                                                                      
*Key F4                                                                         
r 2000.8                                                                        
*Key F4                                                                         
r 5000.8                                                                        
*Key F0                                                                         
*Done                                                                           
