# This test file was generated from offline assembler source
# by text2tst.rexx  2 Dec 2016 12:03:46
# Treat as object code.  That is, modifications will be lost.
# assemble and listing files are provided for information only.
*Testcase digest 20161202 12.03                                                 
sysclear                                                                        
archlvl z                                                                       
r    1A0=00010001800000000000000000000616
r    1C0=00020001800000000000000000000000
r    1D0=00020001800000000000000000000000
r    400=6EE3969740E285839985A34C40859583
r    410=99A897A340A685819293A84086969940
r    420=8285A2A34097999697818781A3899695
r    430=6060606060606060
r    438=60606060606060606060606060606060
r    448=60606060606060606060606060606060
r    458=60606060606060604E4E4E4E4E4E4E4E
r    468=4E4E4E4E4E4E4E4E
r    470=4E4E4E4E4E4E4E4E4E4E4E4E4E4E4E4E
r    480=4E4E4E4E4E4E4E4E4E4E4E4E4E4E4E4E
r    490=6EE3969740E285839985A34C40859583
r    4A0=99A897A340A68581
r    4A8=9293A840869699408285A2A340979996
r    4B8=97818781A38996956060606060606060
r    4C8=60606060606060606060606060606060
r    4D8=6060606060606060
r    4E0=60606060606060606060606060606060
r    4F0=4E4E4E4E4E4E4E4E4E4E4E4E4E4E4E4E
r    500=4E4E4E4E4E4E4E4E4E4E4E4E4E4E4E4E
r    510=4E4E4E4E4E4E4E4E
r    518=4E4E4E4E4E4E4E4E
r    600=12EEA784000841A0900050A0F14C0DEE
r    610=07F90AFF07F9410000804110F200B93E
r    620=0002
*Program 6                                                                      
runtest .1                                                                      
r    622=410000004110F200B93E0000
*Program 6                                                                      
runtest program .1                                                              
r    62E=B93E00024D90F600
runtest program .1                                                              
*Compare                                                                        
r 200.10                                                                        
*Want F0000000 00000000 40000000 00000000                                       
*                                                                               
* SHA1                                                                          
*                                                                               
r    220=67452301EFCDAB8998BADCFE10325476
r    230=C3D2E1F00000000000000480
r    636=410000014110F2204120F40041300090
r    646=B93E0002
*Program 6                                                                      
runtest svc .1                                                                  
r    64A=41300040B93E00024710F64EB2220040
r    65A=4D90F600
runtest program .1                                                              
gpr                                                                             
*Gpr 2 0440 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0000                                                                     
r 220.10                                                                        
*Want 8FEA16BE 2F911D81 D9F428E2 3BAD6691                                       
r 230.4                                                                         
*Want 94298417                                                                  
r    65E=41300050B93F00024710F662B2220040
r    66E=4D90F600
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0490 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0000                                                                     
r 220.10                                                                        
*Want 666D243D 011EFC7A F7BA3154 41E9752A                                       
r 230.4                                                                         
*Want BC1249FC                                                                  
r 234.8                                                                         
*Want  00000000 00000480                                                        
*                                                                               
* SHA256                                                                        
*                                                                               
r    240=6A09E667BB67AE853C6EF372A54FF53A
r    250=510E527F9B05688C1F83D9AB5BE0CD19
r    260=0000000000000480
r    672=410000024110F2404120F40041300090
r    682=B93E0002
*Program 6                                                                      
runtest svc .1                                                                  
r    686=41300040B93E00024710F68AB2220040
r    696=4D90F600
runtest program .1                                                              
gpr                                                                             
*Gpr 2 0440 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0000                                                                     
r 240.10                                                                        
*Want F0A7469C 39FC8746 A28C327F 76118103                                       
r 250.10                                                                        
*Want 5E07E96A AC689C36 EC17DF1F 88779E4B                                       
r    69A=41300050B93F00024710F69EB2220040
r    6AA=4D90F600
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0490 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0000                                                                     
r 240.10                                                                        
*Want 712F0D37 DD6440BF F9FFE27F 6DD8FBC0                                       
r 250.10                                                                        
*Want  785A84C6 D352D7A5 D3647682 9FF675E8                                      
r 260.8                                                                         
*Want  00000000 00000480                                                        
*                                                                               
* SHA512                                                                        
*                                                                               
r    270=6A09E667F3BCC908BB67AE8584CAA73B
r    280=3C6EF372FE94F82BA54FF53A5F1D36F1
r    290=510E527FADE682D19B05688C2B3E6C1F
r    2A0=1F83D9ABFB41BD6B
r    2A8=5BE0CD19137E21790000000000000000
r    2B8=0000000000000900
r    6AE=410000034110F2704120F40041300120
r    6BE=B93E0002
*Program 6                                                                      
runtest svc .1                                                                  
r    6C2=41300080B93E00024710F6C6B2220040
r    6D2=4D90F600
runtest program .1                                                              
gpr                                                                             
*Gpr 2 0480 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0000                                                                     
r 270.10                                                                        
*Want 3683E74A DE2CF007 5CB76A7C 9B5386F3                                       
r 280.10                                                                        
*Want 722122C7 9EC1B0CC E10202B2 13274F27                                       
r 290.10                                                                        
*Want 17BFB280 F0CEF114 CB9511E5 775F09DC                                       
r 2a0.10                                                                        
*Want 057B9AE0 D25AF58B 161617FD 0E4E7395                                       
r    6D6=413000A0B93F00024710F6DAB2220040
r    6E6=4D90F600
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0520 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0000                                                                     
r 270.10                                                                        
*Want 24F541F7 9ED7EF7D 703F2F73 4687F0B3                                       
r 280.10                                                                        
*Want FEE421D4 126933BD 714C0278 3C71FF52                                       
r 290.10                                                                        
*Want A5D3CEFB 1E32E356 EB021FF4 803293B4                                       
r 2a0.10                                                                        
*Want 4036039B 61CEC167 D0D5FB43 E1C46250                                       
r 2b0.10                                                                        
*Want 00000000 00000000 00000000 00000900                                       
*                                                                               
* GHASH                                                                         
*                                                                               
r    2C0=6A09E667BB67AE853C6EF372A54FF53A
r    2D0=510E527F9B05688C1F83D9AB5BE0CD19
r    520=000102030405060708090A0B0C0D0E0F
r    530=101112131415161718191A1B1C1D1E1F
r    540=202122232425262728292A2B2C2D2E2F
r    550=3031323334353637
r    558=38393A3B3C3D3E3F
r    6EA=410000414110F2C04120F52041300040
r    6FA=B93E00024710F6FA4D90F600
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0560 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0000                                                                     
r 2c0.10                                                                        
*Want 3835EC6F 7151E73A 2A593988 B05B7E61                                       
r 2d0.10                                                                        
*Want 510E527F 9B05688C 1F83D9AB 5BE0CD19                                       
*Done                                                                           
