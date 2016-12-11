# This test file was generated from offline assembler source
# by text2tst.rexx 11 Dec 2016 12:44:51
# Treat as object code.  That is, modifications will be lost.
# assemble and listing files are provided for information only.
*Testcase agf 20161211 12.44                                                    
sysclear                                                                        
archlvl z                                                                       
r    1A0=00000001800000000000000000000200
r    1D0=0002000180000000FFFFFFFFDEADDEAD
r    200=410000094110F25041A0F9001B88E340
r    210=1000000458501008E34010080018B222
r    220=0080E340A00000245080A00841A0A010
r    230=411010104600F20E
r    238=12EE077EB2B2F2400002000180000000
r    248=00000000000000000000000000000005
r    258=0000000300000000FFFFFFFFFFFFFFFB
r    268=FFFFFFFD00000000
r    270=FFFFFFFFFFFFFFFB0000000300000000
r    280=80000000000000000000000100000000
r    290=7FFFFFFFFFFFFFFF8000000000000000
r    2A0=00000000FFFFFFFF
r    2A8=00000001000000000000000100000000
r    2B8=00000001000000000000000100000000
r    2C8=FFFFFFFF000000000000000000000000
r    2D8=00000000
runtest .1                                                                      
*Compare                                                                        
r 900.9                                                                         
*Want 00000000 00000008 20                                                      
r 910.9                                                                         
*Want FFFFFFFF FFFFFFF8 10                                                      
r 920.9                                                                         
*Want FFFFFFFF FFFFFFFE 10                                                      
r 930.9                                                                         
*Want 80000000 00000001 10                                                      
r 940.9                                                                         
*Want 7FFFFFFF 7FFFFFFF 20                                                      
r 950.9                                                                         
*Want 00000001 00000000 20                                                      
r 960.9                                                                         
*Want 00000001 00000001 20                                                      
r 970.9                                                                         
*Want 00000000 FFFFFFFF 20                                                      
r 980.9                                                                         
*Want 00000000 00000000 00                                                      
*Done                                                                           
