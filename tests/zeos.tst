# This test file was generated from offline assembler source
# by text2tst.rexx 11 Dec 2016 12:34:53
# Treat as object code.  That is, modifications will be lost.
# assemble and listing files are provided for information only.
*Testcase zeos                                                                  
sysclear                                                                        
mainsize 1m                                                                     
archmode z                                                                      
r    1A0=00000001800000000000000000000244
r    1C0=00020001800000000000000000000000
r    1D0=0002000180000000FFFFFFFFDEADDEAD
r    200=00000000000000000000000000000000
r    210=00000000000000000000000000000000
r    220=00000000000000000000000000000000
r    230=0000000000000000
r    238=000000000000000000000000
*                                                                               
* This file was put into the public domain 2016-12-07                           
* by John P. Hartmann.  You can use it for anything you like,                   
* as long as this notice remains.                                               
*                                                                               
r    244=5890F36812EEA78400064190F3B0A79A
r    254=F050410000144110F370B92800005800
r    264=F33041109FB0D20F1000F334D23F1010
r    274=F3704120F2004140
r    27C=F34441500010B92A0024A714FFFEA784
r    28C=00079201F24012EE077E0A01D20F1000
r    29C=F334A70A00804120F2104140F2004150
r    2AC=0010B92A0024A714
r    2B4=FFFEA78400079202F24012EE077E0A02
r    2C4=41109FC04100001C4120F2204140F344
r    2D4=415000104160F354B92D6024A714FFFE
r    2E4=A78400079203F240
r    2EC=12EE077E0A03A70A00804120F2304140
r    2FC=F220415000104160F354B92D6024A714
r    30C=FFFEA78400079204F24012EE077E0A04
r    31C=12EE077E0A00
r    330=0400001C000102030405060708090A0B
r    340=0C0D0E0F303132333435363738393A3B
r    350=3C3D3E3F202122232425262728292A2B
r    360=2C2D2E2F
r    368=000FF000
runtest .1                                                                      
*Compare                                                                        
r 00000240.00000004                                                             
*Want 00000000                                                                  
r 00000200.00000010                                                             
*Want 866C0230 B823BA9D 9CEF1954 7E50EFC9                                       
r 00000210.00000010                                                             
*Want 30313233 34353637 38393A3B 3C3D3E3F                                       
r 00000220.00000010                                                             
*Want 9BA94F96 FCBD3CF1 24DEEB3D AC7C85A9                                       
r 00000230.00000010                                                             
*Want 30313233 34353637 38393A3B 3C3D3E3F                                       
*Done                                                                           
r    370=101112131415161718191A1B1C1D1E1F
r    380=202122232425262728292A2B2C2D2E2F
r    390=303132333435363738393A3B3C3D3E3F
r    3A0=4041424344454647
r    3A8=48494A4B4C4D4E4F
