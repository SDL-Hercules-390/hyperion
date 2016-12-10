# This test file was generated from offline assembler source
# by txt2tst.rexx  1 Dec 2016 11:41:00
# Treat as object code.  That is, modifications will be lost.
# assemble and listing files are provided for information only.
*Testcase multix                                                                
sysclear                                                                        
archmode z                                                                      
r    1A0=00000001800000000000000000000200
r    1C0=00020001800000000000000000000000
r    200=580000000A00
runtest .1                                                                      
*Compare                                                                        
r 88.4                                                                          
*Want 00020000                                                                  
*Done                                                                           
r    1A0=00710000000000000000000000000206
r    206=0A01
*Testcase multix1                                                               
runtest .1                                                                      
*Compare                                                                        
r 88.4                                                                          
*Want 00020001                                                                  
*Done                                                                           
