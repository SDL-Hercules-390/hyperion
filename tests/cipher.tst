# This test file was generated from offline assembler source
# by text2tst.rexx  2 Dec 2016 12:03:46
# Treat as object code.  That is, modifications will be lost.
# assemble and listing files are provided for information only.
*Testcase cipher 20161202 12.03                                                 
sysclear                                                                        
archlvl z                                                                       
r    1A0=00000001800000000000000000000716
r    1C0=00020001800000000000000000000000
r    1D0=00020001800000000000000000000000
r    700=12EEA784000841A0900050A0F14C0DEE
r    710=07F90AFF07F9
r    200=00000000000000000000000000000000
r    210=00000000000000000000000000000000
r    220=00000000000000000000000000000000
r    230=0F0E0D0C0B0A0908
r    238=00010203040506070000000000000000
r    248=00000000000000000000000000000000
r    258=0F0E0D0C0B0A09080001020304050607
r    268=08090A0B0C0D0E0F
r    270=00000000000000000000000000000000
r    280=0000000000000000
r    290=0F0E0D0C0B0A09080001020304050607
r    2A0=08090A0B0C0D0E0F1011121314151617
r    2B0=00000000000000000000000000000000
r    2C0=0000000000000000
r    2C8=0F0E0D0C0B0A09080706050403020100
r    2D8=000102030405060708090A0B0C0D0E0F
r    2E8=00000000000000000000000000000000
r    2F8=0000000000000000
r    300=00000000000000000001020304050607
r    310=08090A0B0C0D0E0F0F0E0D0C0B0A0908
r    320=07060504030201000001020304050607
r    330=08090A0B0C0D0E0F
r    338=10111213141516170000000000000000
r    348=00000000000000000000000000000000
r    358=0000000000000000
r    368=0F0E0D0C0B0A09080706050403020100
r    378=000102030405060708090A0B0C0D0E0F
r    388=101112131415161718191A1B1C1D1E1F
r    398=0000000000000000
r    3A0=00000000000000000000000000000000
r    3B0=0000000000000000
r    716=410000804110F200B9280000
*Program 6                                                                      
runtest .1                                                                      
r    722=41000000B9280000A795FFEB
runtest program .1                                                              
*Compare                                                                        
r 200.10                                                                        
*Want F0003800 00000000 00000000 00000000                                       
r    72E=410000014110F238B928000041000002
r    73E=4110F260B9280000410000034110F298
r    74E=B9280000410000124110F2D8B9280000
r    75E=410000134110F328
r    766=B9280000410000144110F378B9280000
r    776=A795FFC5
runtest svc .1                                                                  
*Compare                                                                        
r 230.130                                                                       
r    77A=410000004110F210B92E0002
*Program 6                                                                      
runtest svc .1                                                                  
r    786=B92E0020
*Program 6                                                                      
runtest program .1                                                              
r    78A=B92E00224110F220B92F0022A795FFB5
runtest program .1                                                              
*Compare                                                                        
r 210.10                                                                        
*Want "km query" F0703838 00002828 00000000 00000000                            
r 220.10                                                                        
*Want "kmc query" F0703838 00000000 10000000 00000000                           
r    600=0F0E0D0C0B0A09080706050403020100
r    618=0001020304050607
r    79A=410000014110F6184120F70041300100
r    7AA=4140F400D7FFF400F4001B55B92E0042
r    7BA=A714FFFEB2220050A795FF9F
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
*Want "DEA encrypt"  0AC1057C AE3140C8 42998815 A83F973B                        
r    7C6=A70A00804140F500D7FFF500F5004120
r    7D6=F40041300100B92E0042A714FFFEB222
r    7E6=0050A795FF8C
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "dea clear" 12EEA784 000841A0 900050A0 F14C0DEE                           
r    7EC=410000014110F610D207F610F6004120
r    7FC=F700413001004140F400D7FFF400F400
r    80C=1B55B92F0042A714FFFEB2220050A795
r    81C=FF73
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
*Want "kmc DEA encrypt" 775871E5 7BCFB637 7A18002F 9D060111                     
r    81E=D207F610F600A70A00804140F500D7FF
r    82E=F500F5004120F40041300100B92F0042
r    83E=A714FFFEB2220050A795FF5D
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "kmc-dea clear" 12EEA784 000841A0 900050A0 F14C0DEE                       
r    84A=410000094110F2384120F70041300100
r    85A=4140F400D7FFF400F4001B55B92E0042
r    86A=A714FFFEB2220050A795FF47
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
*Want "dea-c encrypt" 0AC1057C AE3140C8 42998815 A83F973B                       
r    876=A70A00804140F500D7FFF500F5004120
r    886=F40041300100B92E0042A714FFFEB222
r    896=0050A795FF34
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "dea-c clear" 12EEA784 000841A0 900050A0 F14C0DEE                         
r    89C=410000094110F230D207F230F6004120
r    8AC=F700413001004140F400D7FFF400F400
r    8BC=1B55B92F0042A714FFFEB2220050A795
r    8CC=FF1B
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "kmc-dea-c encrypt"  775871E5 7BCFB637 7A18002F 9D060111                
r    8CE=D207F230F600A70A00804140F500D7FF
r    8DE=F500F5004120F40041300100B92F0042
r    8EE=A714FFFEB2220050A795FF05
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "kmc-dea-c clear" 12EEA784 000841A0 900050A0 F14C0DEE                     
r    630=000102030405060708090A0B0C0D0E0F
r    8FA=410000024110F6304120F70041300100
r    90A=4140F400D7FFF400F4001B55B92E0042
r    91A=A714FFFEB2220050A795FEEF
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "TDEA-128 encrypt"  7CA6D1C7 ED63C17C 82B881F2 ED99515A                 
r    926=A70A00804140F500D7FFF500F5004120
r    936=F40041300100B92E0042A714FFFEB222
r    946=0050A795FEDC
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "tdea-128 clear" 12EEA784 000841A0 900050A0 F14C0DEE                      
r    94C=410000024110F628D207F628F6004120
r    95C=F700413001004140F400D7FFF400F400
r    96C=1B55B92F0042A714FFFEB2220050A795
r    97C=FEC3
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "kmc-TDEA-128 encrypt"   ECB31BB4 AD6F93D9 C011B376 FB0A028C            
r    97E=D207F628F600A70A00804140F500D7FF
r    98E=F500F5004120F40041300100B92F0042
r    99E=A714FFFEB2220050A795FEAD
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "kmc-tdea-128 clear" 12EEA784 000841A0 900050A0 F14C0DEE                  
r    9AA=4100000A4110F2604120F70041300100
r    9BA=4140F400D7FFF400F4001B55B92E0042
r    9CA=A714FFFEB2220050A795FE97
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "tdea-128-c encrypt" 7CA6D1C7 ED63C17C 82B881F2 ED99515A                
r    9D6=A70A00804140F500D7FFF500F5004120
r    9E6=F40041300100B92E0042A714FFFEB222
r    9F6=0050A795FE84
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "tdea-128-c clear" 12EEA784 000841A0 900050A0 F14C0DEE                    
r    9FC=4100000A4110F258D207F258F6004120
r    A0C=F700413001004140F400D7FFF400F400
r    A1C=1B55B92F0042A714FFFEB2220050A795
r    A2C=FE6B
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "kmc-TDEA-128-c encrypt"   ECB31BB4 AD6F93D9 C011B376 FB0A028C          
r    A2E=D207F258F600A70A00804140F500D7FF
r    A3E=F500F5004120F40041300100B92F0042
r    A4E=A714FFFEB2220050A795FE55
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "kmc-tdea-128-c clear" 12EEA784 000841A0 900050A0 F14C0DEE                
r    648=000102030405060708090A0B0C0D0E0F
r    658=10111213141516170000000000000000
r    A5A=410000034110F6484120F70041300100
r    A6A=4140F400D7FFF400F4001B55B92E0042
r    A7A=A714FFFEB2220050A795FE3F
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "TDEA-192 encrypt"   486E3849 9D047AE7 0CB72FC7 52D816BD                
r    A86=A70A00804140F500D7FFF500F5004120
r    A96=F40041300100B92E0042A714FFFEB222
r    AA6=0050A795FE2C
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "tdea-192 clear" 12EEA784 000841A0 900050A0 F14C0DEE                      
r    AAC=410000034110F640D207F640F6004120
r    ABC=F700413001004140F400D7FFF400F400
r    ACC=1B55B92F0042A714FFFEB2220050A795
r    ADC=FE13
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "kmc-TDEA-192 encrypt"   B28FE3BA 98A5F8B9 6A025CE0 A10EB6C7            
r    ADE=D207F640F600A70A00804140F500D7FF
r    AEE=F500F5004120F40041300100B92F0042
r    AFE=A714FFFEB2220050A795FDFD
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "kmc-tdea-192 clear" 12EEA784 000841A0 900050A0 F14C0DEE                  
r    B0A=4100000B4110F2984120F70041300100
r    B1A=4140F400D7FFF400F4001B55B92E0042
r    B2A=A714FFFEB2220050A795FDE7
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "tdea-192-c encrypt" 486E3849 9D047AE7 0CB72FC7 52D816BD                
r    B36=A70A00804140F500D7FFF500F5004120
r    B46=F40041300100B92E0042A714FFFEB222
r    B56=0050A795FDD4
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "tdea-192-c clear" 12EEA784 000841A0 900050A0 F14C0DEE                    
r    B5C=4100000B4110F290D207F290F6004120
r    B6C=F700413001004140F400D7FFF400F400
r    B7C=1B55B92F0042A714FFFEB2220050A795
r    B8C=FDBB
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "kmc-TDEA-192-c encrypt"   B28FE3BA 98A5F8B9 6A025CE0 A10EB6C7          
r    B8E=D207F290F600A70A00804140F500D7FF
r    B9E=F500F5004120F40041300100B92F0042
r    BAE=A714FFFEB2220050A795FDA5
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "kmc-tdea-192-c clear" 12EEA784 000841A0 900050A0 F14C0DEE                
r    678=000102030405060708090A0B0C0D0E0F
r    688=000102030405060708090A0B0C0D0E0F
r    BBA=410000124110F6784120F70041300100
r    BCA=4140F400D7FFF400F4001B55B92E0042
r    BDA=A714FFFEB2220050A795FD8F
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "AES-128 encrypt"   4BAEA5CB F3A16993 492F7031 31D004E6                 
r    BE6=A70A00804140F500D7FFF500F5004120
r    BF6=F40041300100B92E0042A714FFFEB222
r    C06=0050A795FD7C
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "aes-128 clear" 12EEA784 000841A0 900050A0 F14C0DEE                       
r    C0C=410000124110F668D20FF668F6004120
r    C1C=F700413001004140F400D7FFF400F400
r    C2C=1B55B92F0042A714FFFEB2220050A795
r    C3C=FD63
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "kmc AES-128 encrypt"  31090D54 5341F560 12C8663B E3EA8CB6              
r    C3E=D20FF668F600A70A00804140F500D7FF
r    C4E=F500F5004120F40041300100B92F0042
r    C5E=A714FFFEB2220050A795FD4D
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "kmc-aes-128 clear" 12EEA784 000841A0 900050A0 F14C0DEE                   
r    C6A=4100001A4110F2D84120F70041300100
r    C7A=4140F400D7FFF400F4001B55B92E0042
r    C8A=A714FFFEB2220050A795FD37
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "aes-128-c encrypt" 4BAEA5CB F3A16993 492F7031 31D004E6                 
r    C96=A70A00804140F500D7FFF500F5004120
r    CA6=F40041300100B92E0042A714FFFEB222
r    CB6=0050A795FD24
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "aes-128-c clear" 12EEA784 000841A0 900050A0 F14C0DEE                     
r    CBC=4100001A4110F2C8D20FF2C8F6004120
r    CCC=F700413001004140F400D7FFF400F400
r    CDC=1B55B92F0042A714FFFEB2220050A795
r    CEC=FD0B
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "kmc AES-128-c encrypt"  31090D54 5341F560 12C8663B E3EA8CB6            
r    CEE=D20FF2C8F600A70A00804140F500D7FF
r    CFE=F500F5004120F40041300100B92F0042
r    D0E=A714FFFEB2220050A795FCF5
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "kmc-aes-128-c clear" 12EEA784 000841A0 900050A0 F14C0DEE                 
r    D1A=410000324110F6784120F70041300100
r    D2A=4140F400D7FFF400F4001B55B92E0042
r    D3A=A714FFFEB2220050A795FCDF
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "XTS-AES-128 encrypt"   0671AB22 583E906D 12C21AB6 463341AE             
r    D46=D20FF688F678A70A00804140F500D7FF
r    D56=F500F5004120F40041300100B92E0042
r    D66=A714FFFEB2220050A795FCC9
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "xts-aes-128 clear" 12EEA784 000841A0 900050A0 F14C0DEE                   
r    D72=4100003A4110F2D84120F70041300100
r    D82=4140F400D7FFF400F4001B55B92E0042
r    D92=A714FFFEB2220050A795FCB3
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "XTS-aes-128-c encrypt" 0671AB22 583E906D 12C21AB6 463341AE             
r    D9E=D20FF308F678A70A00804140F500D7FF
r    DAE=F500F5004120F40041300100B92E0042
r    DBE=A714FFFEB2220050A795FC9D
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "xts-aes-128-c clear" 12EEA784 000841A0 900050A0 F14C0DEE                 
r    6A8=000102030405060708090A0B0C0D0E0F
r    6B8=10111213141516170000000000000000
r    DCA=410000134110F6A84120F70041300100
r    DDA=4140F400D7FFF400F4001B55B92E0042
r    DEA=A714FFFEB2220050A795FC87
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "AES-192 encrypt"  EE3EAA7F 5500091D 509AC619 2A222632                  
r    DF6=A70A00804140F500D7FFF500F5004120
r    E06=F40041300100B92E0042A714FFFEB222
r    E16=0050A795FC74
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "aes-192 clear" 12EEA784 000841A0 900050A0 F14C0DEE                       
r    E1C=410000134110F698D20FF698F6004120
r    E2C=F700413001004140F400D7FFF400F400
r    E3C=1B55B92F0042A714FFFEB2220050A795
r    E4C=FC5B
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "kmc-AES-192 encrypt"  7A5E62BE 6DD45AB4 03F9EB6A DFCF9920              
r    E4E=D20FF698F600A70A00804140F500D7FF
r    E5E=F500F5004120F40041300100B92F0042
r    E6E=A714FFFEB2220050A795FC45
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "kmc-aes-192 clear" 12EEA784 000841A0 900050A0 F14C0DEE                   
r    E7A=4100001B4110F3284120F70041300100
r    E8A=4140F400D7FFF400F4001B55B92E0042
r    E9A=A714FFFEB2220050A795FC2F
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "AES-192-c encrypt"  EE3EAA7F 5500091D 509AC619 2A222632                
r 410.f0                                                                        
r    EA6=A70A00804140F500D7FFF500F5004120
r    EB6=F40041300100B92E0042A714FFFEB222
r    EC6=0050A795FC1C
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "aes-192-c clear" 12EEA784 000841A0 900050A0 F14C0DEE                     
r    ECC=4100001B4110F318D20FF318F6004120
r    EDC=F700413001004140F400D7FFF400F400
r    EEC=1B55B92F0042A714FFFEB2220050A795
r    EFC=FC03
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "kmc-AES-192-c encrypt"  7A5E62BE 6DD45AB4 03F9EB6A DFCF9920            
r    EFE=D20FF318F600A70A00804140F500D7FF
r    F0E=F500F5004120F40041300100B92F0042
r    F1E=A714FFFEB2220050A795FBED
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "kmc-aes-192-c clear" 12EEA784 000841A0 900050A0 F14C0DEE                 
r    6D8=000102030405060708090A0B0C0D0E0F
r    6E8=101112131415161718191A1B1C1D1E1F
r    F2A=410000144110F6D84120F70041300100
r    F3A=4140F400D7FFF400F4001B55B92E0042
r    F4A=A714FFFEB2220050A795FBD7
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "AES-256 encrypt"   BFF28D4A A70CFF6F 2DF87BF0 D6FC2882                 
r    F56=A70A00804140F500D7FFF500F5004120
r    F66=F40041300100B92E0042A714FFFEB222
r    F76=0050A795FBC4
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "aes-256 clear" 12EEA784 000841A0 900050A0 F14C0DEE                       
r    F7C=410000144110F6C8D20FF6C8F6004120
r    F8C=F700413001004140F400D7FFF400F400
r    F9C=1B55B92F0042A714FFFEB2220050A795
r    FAC=FBAB
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "kmc-AES-256 encrypt"   418BF112 38C9F028 A32DF978 A3E56293             
r    FAE=D20FF6C8F600A70A00804140F500D7FF
r    FBE=F500F5004120F40041300100B92F0042
r    FCE=A714FFFEB2220050A795FB95
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "kmc-aes-256 clear" 12EEA784 000841A0 900050A0 F14C0DEE                   
r    FDA=4100001C4110F3784120F70041300100
r    FEA=4140F400D7FFF400F4001B55B92E0042
r    FFA=A714FFFEB2220050A795FB7F
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "AES-256 encrypt"   BFF28D4A A70CFF6F 2DF87BF0 D6FC2882                 
r   1006=A70A00804140F500D7FFF500F5004120
r   1016=F40041300100B92E0042A714FFFEB222
r   1026=0050A795FB6C
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "aes-256-c clear" 12EEA784 000841A0 900050A0 F14C0DEE                     
r   102C=4100001C4110F368D20FF368F6004120
r   103C=F700413001004140F400D7FFF400F400
r   104C=1B55B92F0042A714FFFEB2220050A795
r   105C=FB53
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "kmc-AES-256 encrypt"  418BF112 38C9F028 A32DF978 A3E56293              
r   105E=D20FF368F600A70A00804140F500D7FF
r   106E=F500F5004120F40041300100B92F0042
r   107E=A714FFFEB2220050A795FB3D
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0500 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0600 #address                                                            
*Gpr 5 0000                                                                     
r 500.10                                                                        
*Want "kmc-aes-256-c clear" 12EEA784 000841A0 900050A0 F14C0DEE                 
r    3B8=0F0E0D0C0B0A09080001020304050607
r    3C8=08090A0B0C0D0E0F1011121314151617
r   108A=410000434110F3B84120F70041300100
r   109A=4140F400D7FFF400F4001B55B92F0042
r   10AA=A714FFFEB2220050A795FB27
runtest svc .1                                                                  
gpr                                                                             
*Gpr 2 0800 #address                                                            
*Gpr 3 0000                                                                     
*Gpr 4 0500 #address                                                            
*Gpr 5 0000                                                                     
r 400.10                                                                        
* *Want "kmc-prng"  895D7F0D 4A95FB6D 2D170C6D 59FE9CF7                         
r 3b8.8                                                                         
*Want "prng chain" AAF415C1 203ECFBB                                            
*Done                                                                           
