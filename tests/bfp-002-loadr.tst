*Testcase bfp-002-loadr.tst:      LEDBR, LEDB, LEXBR, LEXB, LDXBR, LDXB

#Testcase bfp-002-loadr.tst: IEEE Load Rounded
#..Includes LOAD ROOUNDED (6).  Tests traps, exceptions, results
#..from all rounding modes, and NaN propagation.

sysclear
archmode esame

#
# Following suppresses logging of program checks.  This test program, as part
# of its normal operation, generates 51 program check messages that have no
# value in the validation process.  (The messages, not the program checks.)
#
ostailor quiet

loadcore "$(testpath)/bfp-002-loadr.core"

runtest 1.0

ostailor null   # restore messages for subsequent tests


# Long BFP Inputs converted to short BFP
*Compare
r 1000.10
*Want "LEDBR result pairs 1-2"   00000000 00000000 3FC00000 3FC00000
r 1010.10
*Want "LEDBR result pairs 3-4"   BFC00000 BFC00000 7FC08000 00000000
r 1020.10
*Want "LEDBR result pairs 5-6"   7FC08800 7FC08800 7F800000 27F00000
r 1030.10
*Want "LEDBR result pairs 7-8"   FF800000 A7F00000 7F800000 27FFFFFF
r 1040.10
*Want "LEDBR result pairs 9-10"  FF800000 A7FFFFFF 00000000 56900000
r 1050.10
*Want "LEDBR result pairs 11-12" 80000000 D6900000 7F800000 27F00000
r 1060.08
*Want "LEDBR result pair 13"     FF800000 A7F00000


# Long BFP inputs converted to short BFP - FPC
*Compare
r 1080.10  
*Want "LEDBR FPCR pairs 1-2"    00000000 F8000000 00000000 F8000000
r 1090.10
*Want "LEDBR FPCR pairs 3-4"    00000000 F8000000 00800000 F8008000
r 10A0.10
*Want "LEDBR FPCR pairs 5-6"    00000000 F8000000 00280000 F8002C00
r 10B0.10
*Want "LEDBR FPCR pairs 7-8"    00280000 F8002C00 00280000 F8002800
r 10C0.10
*Want "LEDBR FPCR pairs 9-10"   00280000 F8002800 00180000 F8001000
r 10D0.10
*Want "LEXBR FPCR pairs 11-12"  00180000 F8001000 00280000 F8002000
r 10E0.08
*Want "LEXBR FPCR pair 13"      00280000 F8002000



# Long BFP inputs, trappable overflow/underflow tests
r 3000.10
*Want "LEDBR trap results 1-2"   27F00000 00000000 A7F00000 00000000
r 3010.10
*Want "LEDBR trap results 3-4"   27FFFFFF E0000000 A7FFFFFF E0000000
r 3020.10
*Want "LEDBR trap results 5-6"   56900000 00000000 D6900000 00000000
r 3030.10
*Want "LEDBR trap results 7-8"   27F00000 00000000 A7F00000 00000000


# Long BFP inputs, trappable overflow/underflow tests - FPC
r 3080.10
*Want "LEDBR trap FPCR 1-4"      F8002C00 F8002C00 F8002800 F8002800
r 3090.10
*Want "LEDBR trap FPCR 5-8"      F8001000 F8001000 F8002000 F8002000



# Long BFP Inputs converted to short BFP - rounding mode test results
*Compare
r 1100.10  #                              RZ,      RP,      RM,      RFS
*Want "LEDBRA +exact FPCR modes 1-3, 7" 3FFFFFFF 3FFFFFFF 3FFFFFFF 3FFFFFFF
r 1110.10  #                              RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA +exact M3 modes 1, 3-5"   3FFFFFFF 3FFFFFFF 3FFFFFFF 3FFFFFFF
r 1120.08  #                              RP,      RM
*Want "LEDBRA +exact M3 modes 6, 7"     3FFFFFFF 3FFFFFFF

*Compare
r 1130.10  #                              RZ,      RP,      RM,      RFS
*Want "LEDBRA -exact FPCR modes 1-3, 7" BFFFFFFF BFFFFFFF BFFFFFFF BFFFFFFF
r 1140.10  #                              RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA -exact M3 modes 1, 3-5"   BFFFFFFF BFFFFFFF BFFFFFFF BFFFFFFF
r 1150.08  #                              RP,      RM
*Want "LEDBRA -exact M3 modes 6, 7"     BFFFFFFF BFFFFFFF

*Compare
r 1160.10  #                                RZ,      RP,      RM,      RFS
*Want "LEDBRA +tie odd FPCR modes 1-3, 7" 3FFFFFFF 40000000 3FFFFFFF 3FFFFFFF
r 1170.10  #                                RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA +tie odd M3 modes 1, 3-5"   40000000 3FFFFFFF 40000000 3FFFFFFF
r 1180.08  #                                RP,      RM
*Want "LEDBRA +tie odd M3 modes 6, 7"     40000000 3FFFFFFF

*Compare
r 1190.10  #                                RZ,      RP,      RM,      RFS
*Want "LEDBRA -tie odd FPCR modes 1-3, 7" BFFFFFFF BFFFFFFF C0000000 BFFFFFFF
r 11A0.10  #                                RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA -tie odd M3 modes 1, 3-5"   C0000000 BFFFFFFF C0000000 BFFFFFFF
r 11B0.08  #                                RP,      RM
*Want "LEDBRA -tie odd M3 modes 6, 7"     BFFFFFFF C0000000

*Compare
r 11C0.10  #                                 RZ,      RP,      RM,      RFS
*Want "LEDBRA +tie even FPCR modes 1-3, 7" 3FFFFFFE 3FFFFFFF 3FFFFFFE 3FFFFFFF
r 11D0.10  #                                 RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA +tie even M3 modes 1, 3-5"   3FFFFFFF 3FFFFFFF 3FFFFFFE 3FFFFFFE
r 11E0.08  #                                 RP,      RM
*Want "LEDBRA +tie even M3 modes 6, 7"     3FFFFFFF 3FFFFFFE

*Compare
r 11F0.10  #                                 RZ,      RP,      RM,      RFS
*Want "LEDBRA -tie even FPCR modes 1-3, 7" BFFFFFFE BFFFFFFE BFFFFFFF BFFFFFFF
r 1200.10  #                                 RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA -tie even M3 modes 1, 3-5"   BFFFFFFF BFFFFFFF BFFFFFFE BFFFFFFE
r 1210.08  #                                 RP,      RM
*Want "LEDBRA -tie even M3 modes 6, 7"     BFFFFFFE BFFFFFFF

*Compare
r 1220.10  #                                    RZ,      RP,      RM,      RFS
*Want "LEDBRA +false exact FPCR modes 1-3, 7" 3FFFFFFF 40000000 3FFFFFFF 3FFFFFFF
r 1230.10  #                                    RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA +false exact M3 modes 1, 3-5"   3FFFFFFF 3FFFFFFF 3FFFFFFF 3FFFFFFF
r 1240.08  #                                    RP,      RM
*Want "LEDBRA +false exact M3 modes 6, 7"     40000000 3FFFFFFF

*Compare
r 1250.10  #                                    RZ,      RP,      RM,      RFS
*Want "LEDBRA -false exact FPCR modes 1-3, 7" BFFFFFFF BFFFFFFF C0000000 BFFFFFFF
r 1260.10  #                                    RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA -false exact M3 modes 1, 3-5"   BFFFFFFF BFFFFFFF BFFFFFFF BFFFFFFF
r 1270.08  #                                    RP,      RM
*Want "LEDBRA -false exact M3 modes 6, 7"     BFFFFFFF C0000000

*Compare
r 1280.10  #                                  RZ,      RP,      RM,      RFS
*Want "LEDBRA +near zero FPCR modes 1-3, 7" 3FFFFFFF 40000000 3FFFFFFF 3FFFFFFF
r 1290.10  #                                  RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA +near zero M3 modes 1, 3-5"   3FFFFFFF 3FFFFFFF 3FFFFFFF 3FFFFFFF
r 12A0.08  #                                  RP,      RM
*Want "LEDBRA +near zero M3 modes 6, 7"     40000000 3FFFFFFF

*Compare
r 12B0.10  #                                  RZ,      RP,      RM,      RFS
*Want "LEDBRA -near zero FPCR modes 1-3, 7" BFFFFFFF BFFFFFFF C0000000 BFFFFFFF
r 12C0.10  #                                  RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA -near zero M3 modes 1, 3-5"   BFFFFFFF BFFFFFFF BFFFFFFF BFFFFFFF
r 12D0.08  #                                 RP,      RM
*Want "LEDBRA -near zero M3 modes 6, 7"     BFFFFFFF C0000000

*Compare
r 12E0.10  #                                  RZ,      RP,      RM,      RFS
*Want "LEDBRA +near +inf FPCR modes 1-3, 7" 3FFFFFFF 40000000 3FFFFFFF 3FFFFFFF
r 12F0.10  #                                  RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA +near +inf M3 modes 1, 3-5"   40000000 3FFFFFFF 40000000 3FFFFFFF
r 1300.08  #                                  RP,      RM
*Want "LEDBRA +near +inf M3 modes 6, 7"     40000000 3FFFFFFF

*Compare
r 1310.10  #                                  RZ,      RP,      RM,      RFS
*Want "LEDBRA -near -inf FPCR modes 1-3, 7" BFFFFFFF BFFFFFFF C0000000 BFFFFFFF
r 1320.10  #                                  RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA -near -inf M3 modes 1, 3-5"   C0000000 BFFFFFFF C0000000 BFFFFFFF
r 1330.08  #                                  RP,      RM
*Want "LEDBRA -near -inf M3 modes 6, 7"     BFFFFFFF C0000000

*Compare
r 1340.10  #                                  RZ,      RP,      RM,      RFS
*Want "LEDBRA +overflow FPCR modes 1-3, 7"  7F7FFFFF 7F800000 7F7FFFFF 7F7FFFFF
r 1350.10  #                                  RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA +overflow M3 modes 1, 3-5"    7F800000 7F7FFFFF 7F800000 7F7FFFFF
r 1360.08  #                                  RP,      RM
*Want "LEDBRA +overflow M3 modes 6, 7"      7F800000 7F7FFFFF

*Compare
r 1370.10  #                                  RZ,      RP,      RM,      RFS
*Want "LEDBRA -overflow FPCR modes 1-3, 7"  FF7FFFFF FF7FFFFF FF800000 FF7FFFFF
r 1380.10  #                                  RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA -overflow M3 modes 1, 3-5"    FF800000 FF7FFFFF FF800000 FF7FFFFF
r 1390.08  #                                  RP,      RM
*Want "LEDBRA -overflow M3 modes 6, 7"      FF7FFFFF FF800000

*Compare
r 13A0.10  #                                      RZ,      RP,      RM,      RFS
*Want "LEDBRA +tiny tie odd FPCR modes 1-3, 7"  00000000 00000001 00000000 00000001
r 13B0.10  #                                      RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA +tiny tie odd M3 modes 1, 3-5"    00000001 00000001 00000000 00000000
r 13C0.08  #                                      RP,      RM
*Want "LEDBRA +tiny tie odd M3 modes 6, 7"      00000001 00000000

*Compare
r 13D0.10  #                                      RZ,      RP,      RM,      RFS
*Want "LEDBRA -tiny tie odd FPCR modes 1-3, 7"  80000000 80000000 80000001 80000001
r 13E0.10  #                                      RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA -tiny tie odd M3 modes 1, 3-5"    80000001 80000001 80000000 80000000
r 13F0.08  #                                      RP,      RM
*Want "LEDBRA -tiny tie odd M3 modes 6, 7"      80000000 80000001


# Long BFP Inputs converted to short BFP - rounding mode tests - FPCR contents 
*Compare
r 1500.10
*Want "LEDBRA +exact FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1510.10
*Want "LEDBRA +exact M3 modes 1, 3-5 FPCR"   00000000 00000000 00000000 00000000
r 1520.08
*Want "LEDBRA +exact M3 modes 6, 7 FCPR"     00000000 00000000

r 1530.10
*Want "LEDBRA -exact FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1540.10
*Want "LEDBRA -exact M3 modes 1, 3-5 FPCR"   00000000 00000000 00000000 00000000
r 1550.08
*Want "LEDBRA -exact M3 modes 6, 7 FCPR"     00000000 00000000

r 1560.10
*Want "LEDBRA +tie odd FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1570.10
*Want "LEDBRA +tie odd M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1580.08
*Want "LEDBRA +tie odd M3 modes 6, 7 FCPR"     00080000 00080000

r 1590.10
*Want "LEDBRA -tie odd FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 15A0.10
*Want "LEDBRA -tie odd M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 15B0.08
*Want "LEDBRA -tie odd M3 modes 6, 7 FCPR"     00080000 00080000

r 15C0.10
*Want "LEDBRA +tie even FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 15D0.10
*Want "LEDBRA +tie even M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 15E0.08
*Want "LEDBRA +tie even M3 modes 6, 7 FCPR"     00080000 00080000

r 15F0.10
*Want "LEDBRA -tie even FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1600.10
*Want "LEDBRA -tie even M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1610.08
*Want "LEDBRA -tie even M3 modes 6, 7 FCPR"     00080000 00080000

r 1620.10
*Want "LEDBRA +false exact FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1630.10
*Want "LEDBRA +false exact M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1640.08
*Want "LEDBRA +false exact M3 modes 6, 7 FCPR"     00080000 00080000

r 1650.10
*Want "LEDBRA -false exact FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1660.10
*Want "LEDBRA -false exact M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1670.08
*Want "LEDBRA -false exact M3 modes 6, 7 FCPR"     00080000 00080000

r 1680.10
*Want "LEDBRA +near zero FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1690.10
*Want "LEDBRA +near zero M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 16A0.08
*Want "LEDBRA +near zero M3 modes 6, 7 FCPR"     00080000 00080000

r 16B0.10
*Want "LEDBRA -near zero FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 16C0.10
*Want "LEDBRA -near zero M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 16D0.08
*Want "LEDBRA -near zero M3 modes 6, 7 FCPR"     00080000 00080000

r 16E0.10
*Want "LEDBRA +near +inf FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 16F0.10
*Want "LEDBRA +near +inf M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1700.08
*Want "LEDBRA +near +inf M3 modes 6, 7 FCPR"     00080000 00080000

r 1710.10
*Want "LEDBRA -near -inf FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1720.10
*Want "LEDBRA -near -inf M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1730.08
*Want "LEDBRA -near -inf M3 modes 6, 7 FCPR"     00080000 00080000

*Compare
r 1740.10  #                                       RZ,      RP,      RM,      RFS
*Want "LEDBRA +overflow FPCR modes 1-3, 7 FPCR"  00000001 00200002 00000003 00000007
r 1750.10  #                                       RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA +overflow M3 modes 1, 3-5 FPCR"    00280000 00080000 00280000 00080000
r 1760.08  #                                       RP,      RM
*Want "LEDBRA +overflow M3 modes 6, 7 FPCR"      00280000 00080000

*Compare
r 1770.10  #                                       RZ,      RP,      RM,      RFS
*Want "LEDBRA -overflow FPCR modes 1-3, 7 FPCR"  00000001 00000002 00200003 00000007
r 1780.10  #                                       RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA -overflow M3 modes 1, 3-5 FPCR"    00280000 00080000 00280000 00080000
r 1790.08  #                                       RP,      RM
*Want "LEDBRA -overflow M3 modes 6, 7 FPCR"      00080000 00280000

*Compare
r 17A0.10  #                                           RZ,      RP,      RM,      RFS
*Want "LEDBRA +tiny tie odd FPCR modes 1-3, 7 FPCR"  00100001 00100002 00100003 00100007
r 17B0.10  #                                           RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA +tiny tie odd M3 modes 1, 3-5 FPCR"    00180000 00180000 00180000 00180000
r 17C0.08  #                                           RP,      RM
*Want "LEDBRA +tiny tie odd M3 modes 6, 7 FPCR"      00180000 00180000

*Compare
r 17D0.10  #                                           RZ,      RP,      RM,      RFS
*Want "LEDBRA -tiny tie odd FPCR modes 1-3, 7 FPCR"  00100001 00100002 00100003 00100007
r 17E0.10  #                                           RNTA,    RFS,     RNTE,    RZ
*Want "LEDBRA -tiny tie odd M3 modes 1, 3-5 FPCR"    00180000 00180000 00180000 00180000
r 17F0.08  #                                           RP,      RM
*Want "LEDBRA -tiny tie odd M3 modes 6, 7 FPCR"      00180000 00180000



# Extended BFP inputs rounded to short BFP
*Compare
r 1900.10
*Want "LEXBR result pairs 1-2"   00000000 00000000 3FC00000 3FC00000
r 1910.10
*Want "LEXBR result pairs 3-4"   BFC00000 BFC00000 7FC08000 00000000
r 1920.10
*Want "LEXBR result pairs 5-6"   7FC08800 7FC08800 7F800000 207F0000
r 1930.10
*Want "LEXBR result pairs 7-8"   FF800000 A07F0000 7F800000 207FFFFF
r 1940.10
*Want "LEXBR result pairs 9-10"  FF800000 A07FFFFF 00000000 5F690000
r 1950.10
*Want "LEXBR result pair 11-12"  80000000 DF690000 7F800000 207F0000
r 1960.08
*Want "LEXBR result pair 13"     FF800000 A07F0000


# Extended BFP inputs rounded to short BFP - FPCR contents
*Compare
r 1980.10  
*Want "LEXBR FPCR pairs 1-2"    00000000 F8000000 00000000 F8000000
r 1990.10
*Want "LEXBR FPCR pairs 3-4"    00000000 F8000000 00800000 F8008000
r 19A0.10
*Want "LEXBR FPCR pairs 5-6"    00000000 F8000000 00280000 F8002C00
r 19B0.10
*Want "LEXBR FPCR pairs 7-8"    00280000 F8002C00 00280000 F8002800
r 19C0.10
*Want "LEXBR FPCR pairs 9-10"   00280000 F8002800 00180000 F8001000
r 19D0.10
*Want "LEXBR FPCR pairs 11-12"  00180000 F8001000 00280000 F8002000
r 19E0.08
*Want "LEXBR FPCR pair 13"      00280000 F8002000



# Extended BFP inputs, short results, trappable overflow/underflow tests 
r 3100.10
*Want "LEXBR trap results 1"   207F0000 00000000 00000000 00000000
r 3110.10
*Want "LEXBR trap results 2"   A07F0000 00000000 00000000 00000000
r 3120.10
*Want "LEXBR trap results 3"   207FFFFF FE000000 00000000 00000000
r 3130.10
*Want "LEXBR trap results 4"   A07FFFFF FE000000 00000000 00000000
r 3140.10
*Want "LEXBR trap results 5"   5F690000 00000000 00000000 00000000
r 3150.10
*Want "LEXBR trap results 6"   DF690000 00000000 00000000 00000000
r 3160.10
*Want "LEXBR trap results 7"   207F0000 00000000 00000000 00000000
r 3170.10
*Want "LEXBR trap results 8"   A07F0000 00000000 00000000 00000000


# Extended BFP inputs, trappable overflow/underflow tests - FPC
r 3180.10
*Want "LEXBR trap FPCR 1-4"      F8002C00 F8002C00 F8002800 F8002800
r 3190.10
*Want "LEXBR trap FPCR 5-8"      F8001000 F8001000 F8002000 F8002000



# Extended BFP inputs rounded to short BFP - rounding mode test results
*Compare
r 1A00.10  #                              RZ,      RP,      RM,      RFS
*Want "LEXBRA +exact FPCR modes 1-3, 7" 3FFFFFFF 3FFFFFFF 3FFFFFFF 3FFFFFFF
r 1A10.10  #                              RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA +exact M3 modes 1, 3-5"   3FFFFFFF 3FFFFFFF 3FFFFFFF 3FFFFFFF
r 1A20.08  #                              RP,      RM
*Want "LEXBRA +exact M3 modes 6, 7"     3FFFFFFF 3FFFFFFF

*Compare
r 1A30.10  #                              RZ,      RP,      RM,      RFS
*Want "LEXBRA -exact FPCR modes 1-3, 7" BFFFFFFF BFFFFFFF BFFFFFFF BFFFFFFF
r 1A40.10  #                              RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA -exact M3 modes 1, 3-5"   BFFFFFFF BFFFFFFF BFFFFFFF BFFFFFFF
r 1A50.08  #                              RP,      RM
*Want "LEXBRA -exact M3 modes 6, 7"     BFFFFFFF BFFFFFFF

*Compare
r 1A60.10  #                                RZ,      RP,      RM,      RFS
*Want "LEXBRA +tie odd FPCR modes 1-3, 7" 3FFFFFFF 40000000 3FFFFFFF 3FFFFFFF
r 1A70.10  #                                RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA +tie odd M3 modes 1, 3-5"   40000000 3FFFFFFF 40000000 3FFFFFFF
r 1A80.08  #                                RP,      RM
*Want "LEXBRA +tie odd M3 modes 6, 7"     40000000 3FFFFFFF

*Compare
r 1A90.10  #                                RZ,      RP,      RM,      RFS
*Want "LEXBRA -tie odd FPCR modes 1-3, 7" BFFFFFFF BFFFFFFF C0000000 BFFFFFFF
r 1AA0.10  #                                RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA -tie odd M3 modes 1, 3-5"   C0000000 BFFFFFFF C0000000 BFFFFFFF
r 1AB0.08  #                                RP,      RM
*Want "LEXBRA -tie odd M3 modes 6, 7"     BFFFFFFF C0000000

*Compare
r 1AC0.10  #                                 RZ,      RP,      RM,      RFS
*Want "LEXBRA +tie even FPCR modes 1-3, 7" 3FFFFFFE 3FFFFFFF 3FFFFFFE 3FFFFFFF
r 1AD0.10  #                                 RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA +tie even M3 modes 1, 3-5"   3FFFFFFF 3FFFFFFF 3FFFFFFE 3FFFFFFE
r 1AE0.08  #                                 RP,      RM
*Want "LEXBRA +tie even M3 modes 6, 7"     3FFFFFFF 3FFFFFFE

*Compare
r 1AF0.10  #                                 RZ,      RP,      RM,      RFS
*Want "LEXBRA -tie even FPCR modes 1-3, 7" BFFFFFFE BFFFFFFE BFFFFFFF BFFFFFFF
r 1B00.10  #                                 RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA -tie even M3 modes 1, 3-5"   BFFFFFFF BFFFFFFF BFFFFFFE BFFFFFFE
r 1B10.08  #                                 RP,      RM
*Want "LEXBRA -tie even M3 modes 6, 7"     BFFFFFFE BFFFFFFF

*Compare
r 1B20.10  #                                    RZ,      RP,      RM,      RFS
*Want "LEXBRA +false exact FPCR modes 1-3, 7" 3FFFFFFF 40000000 3FFFFFFF 3FFFFFFF
r 1B30.10  #                                    RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA +false exact M3 modes 1, 3-5"   3FFFFFFF 3FFFFFFF 3FFFFFFF 3FFFFFFF
r 1B40.08  #                                    RP,      RM
*Want "LEXBRA +false exact M3 modes 6, 7"     40000000 3FFFFFFF

*Compare
r 1B50.10  #                                    RZ,      RP,      RM,      RFS
*Want "LEXBRA -false exact FPCR modes 1-3, 7" BFFFFFFF BFFFFFFF C0000000 BFFFFFFF
r 1B60.10  #                                    RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA -false exact M3 modes 1, 3-5"   BFFFFFFF BFFFFFFF BFFFFFFF BFFFFFFF
r 1B70.08  #                                    RP,      RM
*Want "LEXBRA -false exact M3 modes 6, 7"     BFFFFFFF C0000000

*Compare
r 1B80.10  #                                  RZ,      RP,      RM,      RFS
*Want "LEXBRA +near zero FPCR modes 1-3, 7" 3FFFFFFF 40000000 3FFFFFFF 3FFFFFFF
r 1B90.10  #                                  RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA +near zero M3 modes 1, 3-5"   3FFFFFFF 3FFFFFFF 3FFFFFFF 3FFFFFFF
r 1BA0.08  #                                  RP,      RM
*Want "LEXBRA +near zero M3 modes 6, 7"     40000000 3FFFFFFF

*Compare
r 1BB0.10  #                                  RZ,      RP,      RM,      RFS
*Want "LEXBRA -near zero FPCR modes 1-3, 7" BFFFFFFF BFFFFFFF C0000000 BFFFFFFF
r 1BC0.10  #                                  RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA -near zero M3 modes 1, 3-5"   BFFFFFFF BFFFFFFF BFFFFFFF BFFFFFFF
r 1BD0.08  #                                 RP,      RM
*Want "LEXBRA -near zero M3 modes 6, 7"     BFFFFFFF C0000000

*Compare
r 1BE0.10  #                                  RZ,      RP,      RM,      RFS
*Want "LEXBRA +near +inf FPCR modes 1-3, 7" 3FFFFFFF 40000000 3FFFFFFF 3FFFFFFF
r 1BF0.10  #                                  RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA +near +inf M3 modes 1, 3-5"   40000000 3FFFFFFF 40000000 3FFFFFFF
r 1C00.08  #                                  RP,      RM
*Want "LEXBRA +near +inf M3 modes 6, 7"     40000000 3FFFFFFF

*Compare
r 1C10.10  #                                  RZ,      RP,      RM,      RFS
*Want "LEXBRA -near -inf FPCR modes 1-3, 7" BFFFFFFF BFFFFFFF C0000000 BFFFFFFF
r 1C20.10  #                                  RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA -near -inf M3 modes 1, 3-5"   C0000000 BFFFFFFF C0000000 BFFFFFFF
r 1C30.08  #                                  RP,      RM
*Want "LEXBRA -near -inf M3 modes 6, 7"     BFFFFFFF C0000000

*Compare
r 1C40.10  #                                  RZ,      RP,      RM,      RFS
*Want "LEXBRA +overflow FPCR modes 1-3, 7"  7F7FFFFF 7F800000 7F7FFFFF 7F7FFFFF
r 1C50.10  #                                  RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA +overflow M3 modes 1, 3-5"    7F800000 7F7FFFFF 7F800000 7F7FFFFF
r 1C60.08  #                                  RP,      RM
*Want "LEXBRA +overflow M3 modes 6, 7"      7F800000 7F7FFFFF

*Compare
r 1C70.10  #                                  RZ,      RP,      RM,      RFS
*Want "LEXBRA -overflow FPCR modes 1-3, 7"  FF7FFFFF FF7FFFFF FF800000 FF7FFFFF
r 1C80.10  #                                  RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA -overflow M3 modes 1, 3-5"    FF800000 FF7FFFFF FF800000 FF7FFFFF
r 1C90.08  #                                  RP,      RM
*Want "LEXBRA -overflow M3 modes 6, 7"      FF7FFFFF FF800000

*Compare
r 1CA0.10  #                                      RZ,      RP,      RM,      RFS
*Want "LEXBRA +tiny tie odd FPCR modes 1-3, 7"  00000000 00000001 00000000 00000001
r 1CB0.10  #                                      RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA +tiny tie odd M3 modes 1, 3-5"    00000001 00000001 00000000 00000000
r 1CC0.08  #                                      RP,      RM
*Want "LEXBRA +tiny tie odd M3 modes 6, 7"      00000001 00000000

*Compare
r 1CD0.10  #                                      RZ,      RP,      RM,      RFS
*Want "LEXBRA -tiny tie odd FPCR modes 1-3, 7"  80000000 80000000 80000001 80000001
r 1CE0.10  #                                      RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA -tiny tie odd M3 modes 1, 3-5"    80000001 80000001 80000000 80000000
r 1CF0.08  #                                      RP,      RM
*Want "LEXBRA -tiny tie odd M3 modes 6, 7"      80000000 80000001

# Extended BFP inputs converted to short BFP - rounding mode tests - FPCR contents 
*Compare
r 1E00.10
*Want "LEXBRA +exact FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1E10.10
*Want "LEXBRA +exact M3 modes 1, 3-5 FPCR"   00000000 00000000 00000000 00000000
r 1E20.08
*Want "LEXBRA +exact M3 modes 6, 7 FCPR"     00000000 00000000

r 1E30.10
*Want "LEXBRA -exact FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1E40.10
*Want "LEXBRA -exact M3 modes 1, 3-5 FPCR"   00000000 00000000 00000000 00000000
r 1E50.08
*Want "LEXBRA -exact M3 modes 6, 7 FCPR"     00000000 00000000

r 1E60.10
*Want "LEXBRA +tie odd FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1E70.10
*Want "LEXBRA +tie odd M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1E80.08
*Want "LEXBRA +tie odd M3 modes 6, 7 FCPR"     00080000 00080000

r 1E90.10
*Want "LEXBRA -tie odd FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1EA0.10
*Want "LEXBRA -tie odd M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1EB0.08
*Want "LEXBRA -tie odd M3 modes 6, 7 FCPR"     00080000 00080000

r 1EC0.10
*Want "LEXBRA +tie even FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1ED0.10
*Want "LEXBRA +tie even M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1EE0.08
*Want "LEXBRA +tie even M3 modes 6, 7 FCPR"     00080000 00080000

r 1EF0.10
*Want "LEXBRA -tie even FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1F00.10
*Want "LEXBRA -tie even M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1F10.08
*Want "LEXBRA -tie even M3 modes 6, 7 FCPR"     00080000 00080000

r 1F20.10
*Want "LEXBRA +false exact FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1F30.10
*Want "LEXBRA +false exact M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1F40.08
*Want "LEXBRA +false exact M3 modes 6, 7 FCPR"     00080000 00080000

r 1F50.10
*Want "LEXBRA -false exact FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1F60.10
*Want "LEXBRA -false exact M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1F70.08
*Want "LEXBRA -false exact M3 modes 6, 7 FCPR"     00080000 00080000

r 1F80.10
*Want "LEXBRA +near zero FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1F90.10
*Want "LEXBRA +near zero M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1FA0.08
*Want "LEXBRA +near zero M3 modes 6, 7 FCPR"     00080000 00080000

r 1FB0.10
*Want "LEXBRA -near zero FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1FC0.10
*Want "LEXBRA -near zero M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1FD0.08
*Want "LEXBRA -near zero M3 modes 6, 7 FCPR"     00080000 00080000

r 1FE0.10
*Want "LEXBRA +near +inf FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 1FF0.10
*Want "LEXBRA +near +inf M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 2000.08
*Want "LEXBRA +near +inf M3 modes 6, 7 FCPR"     00080000 00080000

r 2010.10
*Want "LEXBRA -near -inf FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 2020.10
*Want "LEXBRA -near -inf M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 2030.08
*Want "LEXBRA -near -inf M3 modes 6, 7 FCPR"     00080000 00080000

*Compare
r 2040.10  #                                       RZ,      RP,      RM,      RFS
*Want "LEXBRA +overflow FPCR modes 1-3, 7 FPCR"  00000001 00200002 00000003 00000007
r 2050.10  #                                       RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA +overflow M3 modes 1, 3-5 FPCR"    00280000 00080000 00280000 00080000
r 2060.08  #                                       RP,      RM
*Want "LEXBRA +overflow M3 modes 6, 7 FPCR"      00280000 00080000

*Compare
r 2070.10  #                                       RZ,      RP,      RM,      RFS
*Want "LEXBRA -overflow FPCR modes 1-3, 7 FPCR"  00000001 00000002 00200003 00000007
r 2080.10  #                                       RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA -overflow M3 modes 1, 3-5 FPCR"    00280000 00080000 00280000 00080000
r 2090.08  #                                       RP,      RM
*Want "LEXBRA -overflow M3 modes 6, 7 FPCR"      00080000 00280000

*Compare
r 20A0.10  #                                           RZ,      RP,      RM,      RFS
*Want "LEXBRA +tiny tie odd FPCR modes 1-3, 7 FPCR"  00100001 00100002 00100003 00100007
r 20B0.10  #                                           RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA +tiny tie odd M3 modes 1, 3-5 FPCR"    00180000 00180000 00180000 00180000
r 20C0.08  #                                           RP,      RM
*Want "LEXBRA +tiny tie odd M3 modes 6, 7 FPCR"      00180000 00180000

*Compare
r 20D0.10  #                                           RZ,      RP,      RM,      RFS
*Want "LEXBRA -tiny tie odd FPCR modes 1-3, 7 FPCR"  00100001 00100002 00100003 00100007
r 20E0.10  #                                           RNTA,    RFS,     RNTE,    RZ
*Want "LEXBRA -tiny tie odd M3 modes 1, 3-5 FPCR"    00180000 00180000 00180000 00180000
r 20F0.08  #                                           RP,      RM
*Want "LEXBRA -tiny tie odd M3 modes 6, 7 FPCR"      00180000 00180000


# Extended BFP inputs converted to long BFP - results
*Compare
r 2200.10
*Want "LDXBR result pair 1"  00000000 00000000 00000000 00000000
r 2210.10
*Want "LDXBR result pair 2"  3FF80000 00000000 3FF80000 00000000
r 2220.10
*Want "LDXBR result pair 3"  BFF80000 00000000 BFF80000 00000000
r 2230.10
*Want "LDXBR result pair 4"  7FF81000 00000000 00000000 00000000
r 2240.10
*Want "LDXBR result pair 5"  7FF81100 00000000 7FF81100 00000000
r 2250.10
*Want "LDXBR result pair 6"  7FF00000 00000000 23FF0000 00000000
r 2260.10
*Want "LDXBR result pair 7"  FFF00000 00000000 A3FF0000 00000000
r 2270.10
*Want "LDXBR result pair 8"  7FF00000 00000000 23FFFFFF FFFFFFFF
r 2280.10
*Want "LDXBR result pair 9"  FFF00000 00000000 A3FFFFFF FFFFFFFF
r 2290.10
*Want "LDXBR result pair 10" 00000000 00000000 5BCC0000 00000000
r 22A0.10
*Want "LDXBR result pair 11" 80000000 00000000 DBCC0000 00000000
r 22B0.10
*Want "LDXBR result pair 12" 7FF00000 00000000 23FF0000 00000000
r 22C0.10
*Want "LDXBR result pair 13" FFF00000 00000000 A3FF0000 00000000

# Extended BFP inputs converted to long BFP - FPCR contents
*Compare
r 2300.10  
*Want "LDXBR FPCR pairs 1-2"  00000000 F8000000 00000000 F8000000
r 2310.10
*Want "LDXBR FPCR pairs 3-4"  00000000 F8000000 00800000 F8008000
r 2320.10  
*Want "LDXBR FPCR pairs 5-6"  00000000 F8000000 00280000 F8002C00
r 2330.10
*Want "LDXBR FPCR pairs 7-8"  00280000 F8002C00 00280000 F8002800
r 2340.10
*Want "LDXBR FPCR pairs 9-10" 00280000 F8002800 00180000 F8001000
r 2350.10
*Want "LDXBR FPCR pairs 11-12" 00180000 F8001000 00280000 F8002000
r 2360.08
*Want "LDXBR FPCR pair 13"    00280000 F8002000 


# Extended BFP inputs, short results, trappable overflow/underflow tests 
r 3200.10
*Want "LDXBR trap result 1"   23FF0000 00000000 00000000 00000000
r 3210.10
*Want "LDXBR trap result 2"   A3FF0000 00000000 00000000 00000000
r 3220.10
*Want "LDXBR trap result 3"   23FFFFFF FFFFFFFF F0000000 00000000
r 3230.10
*Want "LDXBR trap result 4"   A3FFFFFF FFFFFFFF F0000000 00000000
r 3240.10
*Want "LDXBR trap result 5"   5BCC0000 00000000 00000000 00000000
r 3250.10
*Want "LDXBR trap result 6"   DBCC0000 00000000 00000000 00000000
r 3260.10
*Want "LDXBR trap result 7"   23FF0000 00000000 00000000 00000000
r 3270.10
*Want "LDXBR trap result 8"   A3FF0000 00000000 00000000 00000000


# Extended BFP inputs, trappable overflow/underflow tests - FPC
r 3280.10
*Want "LDXBR trap FPCR 1-4"      F8002C00 F8002C00 F8002800 F8002800
r 3290.10
*Want "LDXBR trap FPCR 5-8"      F8001000 F8001000 F8002000 F8002000


# Extended BFP inputs rounded to long BFP - rounding mode test results
*Compare
r 2400.10  #                                   RZ,               RP
*Want "LDXBRA +exact FPC modes 1, 2"         3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF
r 2410.10  #                                   RM,               RFS
*Want "LDXBRA +exact FPC modes 3, 7"         3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF
r 2420.10  #                                   RNTA,             RFS
*Want "LDXBRA +exact M3 modes 1, 3"          3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF
r 2430.10  #                                   RNTE,             RZ
*Want "LDXBRA +exact M3 modes 4, 5"          3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF
r 2440.10  #                                   RP,               RM
*Want "LDXBRA +exact M3 modes 6, 7"          3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF

r 2450.10  #                                   RZ,               RP
*Want "LDXBRA -exact FPC modes 1, 2"         BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 2460.10  #                                   RM,               RFS
*Want "LDXBRA -exact FPC modes 3, 7"         BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 2470.10  #                                   RNTA,             RFS
*Want "LDXBRA -exact M3 modes 1, 3"          BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 2480.10  #                                   RNTE,             RZ
*Want "LDXBRA -exact M3 modes 4, 5"          BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 2490.10  #                                   RP,               RM
*Want "LDXBRA -exact M3 modes 6, 7"          BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF

r 24A0.10  #                                   RZ,               RP
*Want "LDXBRA +tie odd FPC modes 1, 2"       3FFFFFFF FFFFFFFF 40000000 00000000
r 24B0.10  #                                   RM,               RFS
*Want "LDXBRA +tie odd FPC modes 3, 7"       3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF
r 24C0.10  #                                   RNTA,             RFS
*Want "LDXBRA +tie odd M3 modes 1, 3"        40000000 00000000 3FFFFFFF FFFFFFFF
r 24D0.10  #                                   RNTE,             RZ
*Want "LDXBRA +tie odd M3 modes 4, 5"        40000000 00000000 3FFFFFFF FFFFFFFF
r 24E0.10  #                                   RP,               RM
*Want "LDXBRA +tie odd M3 modes 6, 7"        40000000 00000000 3FFFFFFF FFFFFFFF

r 24F0.10  #                                   RZ,               RP
*Want "LDXBRA -tie odd FPC modes 1, 2"       BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 2500.10  #                                   RM,               RFS
*Want "LDXBRA -tie odd FPC modes 3, 7"       C0000000 00000000 BFFFFFFF FFFFFFFF
r 2510.10  #                                   RNTA,             RFS
*Want "LDXBRA -tie odd M3 modes 1, 3"        C0000000 00000000 BFFFFFFF FFFFFFFF
r 2520.10  #                                   RNTE,             RZ
*Want "LDXBRA -tie odd M3 modes 4, 5"        C0000000 00000000 BFFFFFFF FFFFFFFF
r 2530.10  #                                   RP,               RM
*Want "LDXBRA -tie odd M3 modes 6, 7"        BFFFFFFF FFFFFFFF C0000000 00000000

r 2540.10  #                                   RZ,               RP
*Want "LDXBRA +tie even FPC modes 1, 2"      3FFFFFFF FFFFFFFE 3FFFFFFF FFFFFFFF
r 2550.10  #                                   RM,               RFS
*Want "LDXBRA +tie even FPC modes 3, 7"      3FFFFFFF FFFFFFFE 3FFFFFFF FFFFFFFF
r 2560.10  #                                   RNTA,             RFS
*Want "LDXBRA +tie even M3 modes 1, 3"       3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF
r 2570.10  #                                   RNTE,             RZ
*Want "LDXBRA +tie even M3 modes 4, 5"       3FFFFFFF FFFFFFFE 3FFFFFFF FFFFFFFE
r 2580.10  #                                   RP,               RM
*Want "LDXBRA +tie even M3 modes 6, 7"       3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFE

r 2590.10  #                                   RZ,               RP
*Want "LDXBRA -tie even FPC modes 1, 2"      BFFFFFFF FFFFFFFE BFFFFFFF FFFFFFFE
r 25A0.10  #                                   RM,               RFS
*Want "LDXBRA -tie even FPC modes 3, 7"      BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 25B0.10  #                                   RNTA,             RFS
*Want "LDXBRA -tie even M3 modes 1, 3"       BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 25C0.10  #                                   RNTE,             RZ
*Want "LDXBRA -tie even M3 modes 4, 5"       BFFFFFFF FFFFFFFE BFFFFFFF FFFFFFFE
r 25D0.10  #                                   RP,               RM
*Want "LDXBRA -tie even M3 modes 6, 7"       BFFFFFFF FFFFFFFE BFFFFFFF FFFFFFFF

r 25E0.10  #                                   RZ,               RP
*Want "LDXBRA +false exact FPC modes 1, 2"   3FFFFFFF FFFFFFFF 40000000 00000000
r 25F0.10  #                                   RM,               RFS
*Want "LDXBRA +false exact FPC modes 3, 7"   3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF
r 2600.10  #                                   RNTA,             RFS
*Want "LDXBRA +false exact M3 modes 1, 3"    3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF
r 2610.10  #                                   RNTE,             RZ
*Want "LDXBRA +false exact M3 modes 4, 5"    3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF
r 2620.10  #                                   RP,               RM
*Want "LDXBRA +false exact M3 modes 6, 7"    40000000 00000000 3FFFFFFF FFFFFFFF

r 2630.10  #                                   RZ,               RP
*Want "LDXBRA -false exact FPC modes 1, 2"   BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 2640.10  #                                   RM,               RFS
*Want "LDXBRA -false exact FPC modes 3, 7"   C0000000 00000000 BFFFFFFF FFFFFFFF
r 2650.10  #                                   RNTA,             RFS
*Want "LDXBRA -false exact M3 modes 1, 3"    BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 2660.10  #                                   RNTE,             RZ
*Want "LDXBRA -false exact M3 modes 4, 5"    BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 2670.10  #                                   RP,               RM
*Want "LDXBRA -false exact M3 modes 6, 7"    BFFFFFFF FFFFFFFF C0000000 00000000

r 2680.10  #                                   RZ,               RP
*Want "LDXBRA +near zero FPC modes 1, 2"     3FFFFFFF FFFFFFFF 40000000 00000000
r 2690.10  #                                   RM,               RFS
*Want "LDXBRA +near zero FPC modes 3, 7"     3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF
r 26A0.10  #                                   RNTA,             RFS
*Want "LDXBRA +near zero M3 modes 1, 3"      3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF
r 26B0.10  #                                   RNTE,             RZ
*Want "LDXBRA +near zero M3 modes 4, 5"      3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF
r 26C0.10  #                                   RP,               RM
*Want "LDXBRA +near zero M3 modes 6, 7"      40000000 00000000 3FFFFFFF FFFFFFFF

r 26D0.10  #                                   RZ,               RP
*Want "LDXBRA -near zero FPC modes 1, 2"     BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 26E0.10  #                                   RM,               RFS
*Want "LDXBRA -near zero FPC modes 3, 7"     C0000000 00000000 BFFFFFFF FFFFFFFF
r 26F0.10  #                                   RNTA,             RFS
*Want "LDXBRA -near zero M3 modes 1, 3"      BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 2700.10  #                                   RNTE,             RZ
*Want "LDXBRA -near zero M3 modes 4, 5"      BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 2710.10  #                                   RP,               RM
*Want "LDXBRA -near zero M3 modes 6, 7"      BFFFFFFF FFFFFFFF C0000000 00000000

r 2720.10  #                                   RZ,               RP
*Want "LDXBRA +near +inf FPC modes 1, 2"     3FFFFFFF FFFFFFFF 40000000 00000000
r 2730.10  #                                   RM,               RFS
*Want "LDXBRA +near +inf FPC modes 3, 7"     3FFFFFFF FFFFFFFF 3FFFFFFF FFFFFFFF
r 2740.10  #                                   RNTA,             RFS
*Want "LDXBRA +near +inf M3 modes 1, 3"      40000000 00000000 3FFFFFFF FFFFFFFF
r 2750.10  #                                   RNTE,             RZ
*Want "LDXBRA +near +inf M3 modes 4, 5"      40000000 00000000 3FFFFFFF FFFFFFFF
r 2760.10  #                                   RP,               RM
*Want "LDXBRA +near +inf M3 modes 6, 7"      40000000 00000000 3FFFFFFF FFFFFFFF

r 2770.10  #                                   RZ,               RP
*Want "LDXBRA -near -inf FPC modes 1, 2"     BFFFFFFF FFFFFFFF BFFFFFFF FFFFFFFF
r 2780.10  #                                   RM,               RFS
*Want "LDXBRA -near -inf FPC modes 3, 7"     C0000000 00000000 BFFFFFFF FFFFFFFF
r 2790.10  #                                   RNTA,             RFS
*Want "LDXBRA -near -inf M3 modes 1, 3"      C0000000 00000000 BFFFFFFF FFFFFFFF
r 27A0.10  #                                   RNTE,             RZ
*Want "LDXBRA -near -inf M3 modes 4, 5"      C0000000 00000000 BFFFFFFF FFFFFFFF
r 27B0.10  #                                   RP,               RM
*Want "LDXBRA -near -inf M3 modes 6, 7"      BFFFFFFF FFFFFFFF C0000000 00000000

r 27C0.10  #                                   RZ,               RP
*Want "LDXBRA +overflow FPC modes 1, 2"      7FEFFFFF FFFFFFFF 7FF00000 00000000
r 27D0.10  #                                   RM,               RFS
*Want "LDXBRA +overflow FPC modes 3, 7"      7FEFFFFF FFFFFFFF 7FEFFFFF FFFFFFFF
r 27E0.10  #                                   RNTA,             RFS
*Want "LDXBRA +overflow M3 modes 1, 3"       7FF00000 00000000 7FEFFFFF FFFFFFFF
r 27F0.10  #                                   RNTE,             RZ
*Want "LDXBRA +overflow M3 modes 4, 5"       7FF00000 00000000 7FEFFFFF FFFFFFFF
r 2800.10  #                                   RP,               RM
*Want "LDXBRA +overflow M3 modes 6, 7"       7FF00000 00000000 7FEFFFFF FFFFFFFF

r 2810.10  #                                   RZ,               RP
*Want "LDXBRA -overflow FPC modes 1, 2"      FFEFFFFF FFFFFFFF FFEFFFFF FFFFFFFF
r 2820.10  #                                   RM,               RFS
*Want "LDXBRA -overflow FPC modes 3, 7"      FFF00000 00000000 FFEFFFFF FFFFFFFF
r 2830.10  #                                   RNTA,             RFS
*Want "LDXBRA -overflow M3 modes 1, 3"       FFF00000 00000000 FFEFFFFF FFFFFFFF
r 2840.10  #                                   RNTE,             RZ
*Want "LDXBRA -overflow M3 modes 4, 5"       FFF00000 00000000 FFEFFFFF FFFFFFFF
r 2850.10  #                                   RP,               RM
*Want "LDXBRA -overflow M3 modes 6, 7"       FFEFFFFF FFFFFFFF FFF00000 00000000 

r 2860.10  #                                   RZ,               RP
*Want "LDXBRA +tiny tie odd FPC modes 1, 2"  00000000 00000000 00000000 00000001
r 2870.10  #                                   RM,               RFS
*Want "LDXBRA +tiny tie odd FPC modes 3, 7"  00000000 00000000 00000000 00000001
r 2880.10  #                                   RNTA,             RFS
*Want "LDXBRA +tiny tie odd M3 modes 1, 3"   00000000 00000001 00000000 00000001
r 2890.10  #                                   RNTE,             RZ
*Want "LDXBRA +tiny tie odd M3 modes 4, 5"   00000000 00000000 00000000 00000000
r 28A0.10  #                                   RP,               RM
*Want "LDXBRA +tiny tie odd M3 modes 6, 7"   00000000 00000001 00000000 00000000

r 28B0.10  #                                   RZ,               RP
*Want "LDXBRA -tiny tie odd FPC modes 1, 2"  80000000 00000000 80000000 00000000
r 28C0.10  #                                   RM,               RFS
*Want "LDXBRA -tiny tie odd FPC modes 3, 7"  80000000 00000001 80000000 00000001
r 28D0.10  #                                   RNTA,             RFS
*Want "LDXBRA -tiny tie odd M3 modes 1, 3"   80000000 00000001 80000000 00000001
r 28E0.10  #                                   RNTE,             RZ
*Want "LDXBRA -tiny tie odd M3 modes 4, 5"   80000000 00000000 80000000 00000000
r 28F0.10  #                                   RP,               RM
*Want "LDXBRA -tiny tie odd M3 modes 6, 7"   80000000 00000000 80000000 00000001


# Extended BFP inputs rounded to long BFP - rounding mode test FPCR contents
*Compare
r 2B00.10
*Want "LDXBRA +exact FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 2B10.10
*Want "LDXBRA +exact M3 modes 1, 3-5 FPCR"   00000000 00000000 00000000 00000000
r 2B20.08
*Want "LDXBRA +exact M3 modes 6, 7 FCPR"     00000000 00000000

r 2B30.10
*Want "LDXBRA -exact FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 2B40.10
*Want "LDXBRA -exact M3 modes 1, 3-5 FPCR"   00000000 00000000 00000000 00000000
r 2B50.08
*Want "LDXBRA -exact M3 modes 6, 7 FCPR"     00000000 00000000

r 2B60.10
*Want "LDXBRA +tie odd FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 2B70.10
*Want "LDXBRA +tie odd M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 2B80.08
*Want "LDXBRA +tie odd M3 modes 6, 7 FCPR"     00080000 00080000

r 2B90.10
*Want "LDXBRA -tie odd FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 2BA0.10
*Want "LDXBRA -tie odd M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 2BB0.08
*Want "LDXBRA -tie odd M3 modes 6, 7 FCPR"     00080000 00080000

r 2BC0.10
*Want "LDXBRA +tie even FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 2BD0.10
*Want "LDXBRA +tie even M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 2BE0.08
*Want "LDXBRA +tie even M3 modes 6, 7 FCPR"     00080000 00080000

r 2BF0.10
*Want "LDXBRA -tie even FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 2C00.10
*Want "LDXBRA -tie even M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 2C10.08
*Want "LDXBRA -tie even M3 modes 6, 7 FCPR"     00080000 00080000

r 2C20.10
*Want "LDXBRA +false exact FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 2C30.10
*Want "LDXBRA +false exact M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 2C40.08
*Want "LDXBRA +false exact M3 modes 6, 7 FCPR"     00080000 00080000

r 2C50.10
*Want "LDXBRA -false exact FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 2C60.10
*Want "LDXBRA -false exact M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 2C70.08
*Want "LDXBRA -false exact M3 modes 6, 7 FCPR"     00080000 00080000

r 2C80.10
*Want "LDXBRA +near zero FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 2C90.10
*Want "LDXBRA +near zero M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 2CA0.08
*Want "LDXBRA +near zero M3 modes 6, 7 FCPR"     00080000 00080000

r 2CB0.10
*Want "LDXBRA -near zero FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 2CC0.10
*Want "LDXBRA -near zero M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 2CD0.08
*Want "LDXBRA -near zero M3 modes 6, 7 FCPR"     00080000 00080000

r 2CE0.10
*Want "LDXBRA +near +inf FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 2CF0.10
*Want "LDXBRA +near +inf M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 2D00.08
*Want "LDXBRA +near +inf M3 modes 6, 7 FCPR"     00080000 00080000

r 2D10.10
*Want "LDXBRA -near -inf FPC modes 1-3, 7 FCPR"  00000001 00000002 00000003 00000007
r 2D20.10
*Want "LDXBRA -near -inf M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 2D30.08
*Want "LDXBRA -near -inf M3 modes 6, 7 FCPR"     00080000 00080000

*Compare
r 2D40.10  #                                       RZ,      RP,      RM,      RFS
*Want "LDXBRA +overflow FPCR modes 1-3, 7 FPCR"  00000001 00200002 00000003 00000007
r 2D50.10  #                                       RNTA,    RFS,     RNTE,    RZ
*Want "LDXBRA +overflow M3 modes 1, 3-5 FPCR"    00280000 00080000 00280000 00080000
r 2D60.08  #                                       RP,      RM
*Want "LDXBRA +overflow M3 modes 6, 7 FPCR"      00280000 00080000

*Compare
r 2D70.10  #                                       RZ,      RP,      RM,      RFS
*Want "LDXBRA -overflow FPCR modes 1-3, 7 FPCR"  00000001 00000002 00200003 00000007
r 2D80.10  #                                       RNTA,    RFS,     RNTE,    RZ
*Want "LDXBRA -overflow M3 modes 1, 3-5 FPCR"    00280000 00080000 00280000 00080000
r 2D90.08  #                                       RP,      RM
*Want "LDXBRA -overflow M3 modes 6, 7 FPCR"      00080000 00280000

*Compare
r 2DA0.10  #                                           RZ,      RP,      RM,      RFS
*Want "LDXBRA +tiny tie odd FPCR modes 1-3, 7 FPCR"  00100001 00100002 00100003 00100007
r 2DB0.10  #                                           RNTA,    RFS,     RNTE,    RZ
*Want "LDXBRA +tiny tie odd M3 modes 1, 3-5 FPCR"    00180000 00180000 00180000 00180000
r 2DC0.08  #                                           RP,      RM
*Want "LDXBRA +tiny tie odd M3 modes 6, 7 FPCR"      00180000 00180000

*Compare
r 2DD0.10  #                                           RZ,      RP,      RM,      RFS
*Want "LDXBRA -tiny tie odd FPCR modes 1-3, 7 FPCR"  00100001 00100002 00100003 00100007
r 2DE0.10  #                                           RNTA,    RFS,     RNTE,    RZ
*Want "LDXBRA -tiny tie odd M3 modes 1, 3-5 FPCR"    00180000 00180000 00180000 00180000
r 2DF0.08  #                                           RP,      RM
*Want "LDXBRA -tiny tie odd M3 modes 6, 7 FPCR"      00180000 00180000



*Done

