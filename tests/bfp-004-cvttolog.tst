*Testcase bfp-004-cvttolog.tst:   CLFEBR, CLFDBR, CLFXBR

#Testcase bfp-004-cvttolog.tst: IEEE Convert To Logical
#..Includes CONVERT TO LOGICAL 32 (3).  Tests traps, exceptions,
#..rounding modes, and NaN propagation.

sysclear
archmode esame

#
# Following suppresses logging of program checks.  This test program, as part
# of its normal operation, generates 17program check messages that have no
# value in the validation process.  (The messages, not the program checks.)
#
ostailor quiet

loadcore "$(testpath)/bfp-004-cvttolog.core"
runtest 1.0

ostailor null   # restore messages for subsequent tests


# BFP short inputs converted to uint-32 test results
*Compare
r 1000.10
*Want "CLFEBR result pairs 1-2" 00000001 00000001 00000002 00000002
r 1010.10
*Want "CLFEBR result pairs 3-4" 00000004 00000004 00000000 00000000
r 1020.10                         
*Want "CLFEBR result pairs 5-6" 00000000 00000000 FFFFFFFF 00000000
r 1030.10
*Want "CLFEBR result pairs 7-8" FFFFFF00 FFFFFF00 00000001 00000001
r 1040.08
*Want "CLFEBR result pair 9"    00000000 00000000

# BFP short inputs converted to uint-32 FPCR contents, cc
*Compare
r 1100.10
*Want "CLFEBR FPC pairs 1-2"    00000002 F8000002 00000002 F8000002
r 1110.10
*Want "CLFEBR FPC pairs 3-4"    00000002 F8000002 00880003 F8008000 
r 1120.10
*Want "CLFEBR FPC pairs 5-6"    00880003 F8008000 00880003 F8008000
r 1130.10
*Want "CLFEBR FPC pairs 7-8"    00000002 F8000002 00080002 F8000C02
r 1140.08
*Want "CLFEBR FPC pair 9"       00080002 F8000802

#  rounding mode tests - short BFP - results from rounding
*Compare
r 1200.10  #                            RZ,      RP,      RM,      RFS
*Want "CLFEBR -1.5 FPC modes 1-3, 7"  00000000 00000000 00000000 00000000
r 1210.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFEBR -1.5 M3 modes 1, 3-5"   00000000 00000000 00000000 00000000
r 1220.08  #                            RP,      RM
*Want "CLFEBR -1.5 M3 modes 6, 7"     00000000 00000000

r 1230.10  #                            RZ,      RP,      RM,      RFS
*Want "CLFEBR -0.5 FPC modes 1-3, 7"  00000000 00000000 00000000 00000000
r 1240.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFEBR -0.5 M3 modes 1, 3-5"   00000000 00000000 00000000 00000000
r 1250.08  #                            RP,      RM
*Want "CLFEBR -0.5 M3 modes 6, 7"    00000000 00000000 

r 1260.10  #                           RZ,      RP,      RM,      RFS
*Want "CLFEBR 0.5 FPC modes 1-3, 7"  00000000 00000001 00000000 00000001
r 1270.10  #                           RNTA,    RFS,     RNTE,    RZ
*Want "CLFEBR 0.5 M3 modes 1, 3-5"   00000001 00000001 00000000 00000000
r 1280.08  #                           RP,      RM
*Want "CLFEBR 0.5 M3 modes 6, 7"     00000001 00000000

r 1290.10  #                           RZ,      RP,      RM,      RFS
*Want "CLFEBR 1.5 FPC modes 1-3, 7"  00000001 00000002 00000001 00000001
r 12A0.10  #                           RNTA,    RFS,     RNTE,    RZ
*Want "CLFEBR 1.5 M3 modes 1, 3-5"   00000002 00000001 00000002 00000001
r 12B0.08  #                           RP,      RM
*Want "CLFEBR 1.5 M3 modes 6, 7"     00000002 00000001

r 12C0.10  #                           RZ,      RP,      RM,      RFS
*Want "CLFEBR 2.5 FPC modes 1-3, 7"  00000002 00000003 00000002 00000003
r 12D0.10  #                           RNTA,    RFS,     RNTE,    RZ
*Want "CLFEBR 2.5 M3 modes 1, 3-5"   00000003 00000003 00000002 00000002
r 12E0.08  #                           RP,      RM
*Want "CLFEBR 2.5 M3 modes 6, 7"     00000003 00000002

r 12F0.10  #                           RZ,      RP,      RM,      RFS
*Want "CLFEBR 5.5 FPC modes 1-3, 7"  00000005 00000006 00000005 00000005
r 1300.10  #                           RNTA,    RFS,     RNTE,    RZ
*Want "CLFEBR 5.5 M3 modes 1, 3-5"   00000006 00000005 00000006 00000005
r 1310.08  #                           RP,      RM
*Want "CLFEBR 5.5 M3 modes 6, 7"     00000006 00000005

r 1320.10  #                           RZ,     RP,      RM,      RFS
*Want "CLFEBR 9.5 FPC modes 1-3, 7"  00000009 0000000A 00000009 00000009
r 1330.10  #                           RNTA,    RFS,     RNTE,    RZ
*Want "CLFEBR 9.5 M3 modes 1, 3-5"   0000000A 00000009 0000000A 00000009
r 1340.08  #                           RP,      RM
*Want "CLFEBR 9.5 M3 modes 6, 7"     0000000A 00000009

r 1350.10  #                           RZ,      RP,      RM,      RFS
*Want "CLFEBR max FPC modes 1-3, 7"  FFFFFF00 FFFFFF00 FFFFFF00 FFFFFF00
r 1360.10  #                          RNTA,    RFS,     RNTE,    RZ
*Want "CLFEBR max M3 modes 1, 3-5"   FFFFFF00 FFFFFF00 FFFFFF00 FFFFFF00
r 1370.08  #                           RP,      RM
*Want "CLFEBR max M3 modes 6, 7"     FFFFFF00 FFFFFF00

r 1380.10  #                           RZ,      RP,      RM,      RFS
*Want "CLFEBR 0.75 FPC modes 1-3, 7" 00000000 00000001 00000000 00000001
r 1390.10  #                           RNTA,    RFS,     RNTE,    RZ
*Want "CLFEBR 0.75 M3 modes 1, 3-5"  00000001 00000001 00000001 00000000
r 13A0.08  #                           RP,      RM
*Want "CLFEBR 0.75 M3 modes 6, 7"    00000001 00000000

r 13B0.10  #                           RZ,      RP,      RM,      RFS
*Want "CLFEBR 0.25 FPC modes 1-3, 7" 00000000 00000001 00000000 00000001
r 13C0.10  #                           RNTA,    RFS,     RNTE,    RZ
*Want "CLFEBR 0.25 M3 modes 1, 3-5"  00000000 00000001 00000000 00000000
r 13D0.08  #                           RP,      RM
*Want "CLFEBR 0.25 M3 modes 6, 7"    00000001 00000000


#  rounding mode tests - short BFP - FPCR contents with cc in last byte
*Compare
r 1600.10
*Want "CLFEBR -1.5 FPC modes 1-3, 7 FPCR"  00800003 00800003 00800003 00800003
r 1610.10
*Want "CLFEBR -1.5 M3 modes 1, 3-5 FPCR"   00880003 00880003 00880003 00880003
r 1620.08
*Want "CLFEBR -1.5 M3 modes 6, 7 FPCR"     00880003 00880003

r 1630.10
*Want "CLFEBR -0.5 FPC modes 1-3, 7 FPCR"  00000001 00000001 00800003 00800003
r 1640.10
*Want "CLFEBR -0.5 M3 modes 1, 3-5 FPCR"   00880003 00880003 00080001 00080001
r 1650.08
*Want "CLFEBR -0.5 M3 modes 6, 7 FPCR"     00080001 00880003

r 1660.10
*Want "CLFEBR +0.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 1670.10
*Want "CLFEBR +0.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1680.08
*Want "CLFEBR +0.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 1690.10
*Want "CLFEBR +1.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 16A0.10
*Want "CLFEBR +1.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 16B0.08
*Want "CLFEBR +1.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 16C0.10
*Want "CLFEBR +2.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 16D0.10
*Want "CLFEBR +2.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 16E0.08
*Want "CLFEBR +2.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 16F0.10
*Want "CLFEBR +5.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 1700.10
*Want "CLFEBR +5.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1710.08
*Want "CLFEBR +5.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 1720.10
*Want "CLFEBR +9.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 1730.10
*Want "CLFEBR +9.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1740.08
*Want "CLFEBR +9.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 1750.10
*Want "CLFEBR max FPC modes 1-3, 7 FPCR"   00000002 00000002 00000002 00000002
r 1760.10
*Want "CLFEBR max M3 modes 1, 3-5 FPCR"    00000002 00000002 00000002 00000002
r 1770.08
*Want "CLFEBR max M3 modes 5-7"            00000002 00000002

r 1780.10  #                                 RZ,      RP,      RM,      RFS
*Want "CLFEBR +0.75 FPC modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 1790.10  #                                 RNTA,    RFS,     RNTE,    RZ
*Want "CLFEBR +0.75 M3 modes 1, 3-5 FPCR"  00080002 00080002 00080002 00080002
r 17A0.08  #                                 RP,      RM
*Want "CLFEBR +0.75 M3 modes 6, 7 FPCR"    00080002 00080002

r 17B0.10
*Want "CLFEBR +0.25 FPC modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 17C0.10
*Want "CLFEBR +0.25 M3 modes 1, 3-5 FPCR"  00080002 00080002 00080002 00080002
r 17D0.08
*Want "CLFEBR +0.25 M3 modes 6, 7 FPCR"    00080002 00080002


# BFP short inputs converted to uint-32 test results
*Compare
r 2000.10
*Want "CLFDBR result pairs 1-2" 00000001 00000001 00000002 00000002
r 2010.10
*Want "CLFDBR result pairs 3-4" 00000004 00000004 00000000 00000000
r 2020.10
*Want "CLFDBR result pairs 5-6" 00000000 00000000 FFFFFFFF 00000000
r 2030.10
*Want "CLFDBR result pairs 7-8" FFFFFFFF FFFFFFFF 00000001 00000001
r 2040.08
*Want "CLFDBR result pair 9"    00000000 00000000

# BFP long inputs converted to uint-32 FPCR contents, cc
*Compare
r 2100.10
*Want "CLFDBR FPC pairs 1-2" 00000002 F8000002 00000002 F8000002
r 2110.10
*Want "CLFDBR FPC pairs 3-4" 00000002 F8000002 00880003 F8008000
r 2120.10
*Want "CLFDBR FPC pairs 5-6" 00880003 F8008000 00880003 F8008000
r 2130.10
*Want "CLFDBR FPC pairs 7-8" 00080002 F8000802 00080002 F8000C02
r 2140.08
*Want "CLFDBR FPC pair 9"    00080002 F8000802


#  rounding mode tests - long BFP - results from rounding
*Compare
r 2200.10  #                            RZ,      RP,      RM,      RFS
*Want "CLFDBR -1.5 FPC modes 1-3, 7"  00000000 00000000 00000000 00000000
r 2210.10  #                            RNTA,    FS,     RNTE,     RZ
*Want "CLFDBR -1.5 M3 modes 1, 3-5"   00000000 00000000 00000000 00000000
r 2220.08  #                            RP,      RM
*Want "CLFDBR -1.5 M3 modes 6, 7"     00000000 00000000

r 2230.10  #                            RZ,      RP,      RM,      RFS
*Want "CLFDBR -0.5 FPC modes 1-3, 7"  00000000 00000000 00000000 00000000
r 2240.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFDBR -0.5 M3 modes 1, 3-5"   00000000 00000000 00000000 00000000
r 2250.08  #                            RP,      RM
*Want "CLFDBR -0.5 M3 modes 6, 7"     00000000 00000000

r 2260.10  #                            RZ,      RP,      RM,      RFS
*Want "CLFDBR 0.5 FPC modes 1-3, 7"   00000000 00000001 00000000 00000001
r 2270.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFDBR 0.5 M3 modes 1, 3-5"    00000001 00000001 00000000 00000000
r 2280.08  #                            RP,      RM
*Want "CLFDBR 0.5 M3 modes 6, 7"      00000001 00000000

r 2290.10  #                            RZ,      RP,      RM,      RFS
*Want "CLFDBR 1.5 FPC modes 1-3, 7"   00000001 00000002 00000001 00000001
r 22A0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFDBR 1.5 M3 modes 1, 3-5"    00000002 00000001 00000002 00000001
r 22B0.08  #                            RP,      RM
*Want "CLFDBR 1.5 M3 modes 6, 7"      00000002 00000001

r 22C0.10  #                            RZ,      RP,      RM,      RFS
*Want "CLFDBR 2.5 FPC modes 1-3, 7"   00000002 00000003 00000002 00000003
r 22D0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFDBR 2.5 M3 modes 1, 3-5"    00000003 00000003 00000002 00000002
r 22E0.08  #                            RP,      RM
*Want "CLFDBR 2.5 M3 modes 6, 7"      00000003 00000002

r 22F0.10  #                            RZ,      RP,      RM,      RFS
*Want "CLFDBR 5.5 FPC modes 1-3, 7"   00000005 00000006 00000005 00000005
r 2300.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFDBR 5.5 M3 modes 1, 3-5"    00000006 00000005 00000006 00000005
r 2310.08  #                            RP,      RM
*Want "CLFDBR 5.5 M3 modes 6, 7"      00000006 00000005

r 2320.10  #                            RZ,      RP,      RM,      RFS
*Want "CLFDBR 9.5 FPC modes 1-3, 7"   00000009 0000000A 00000009 00000009
r 2330.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFDBR 9.5 M3 modes 1, 3-5"    0000000A 00000009 0000000A 00000009
r 2340.08  #                            RP,      RM
*Want "CLFDBR 9.5 M3 modes 6, 7"      0000000A 00000009

r 2350.10  #                            RZ,      RP,      RM,      RFS
*Want "CLFDBR max FPC modes 1-3, 7"   FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 2360.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFDBR max M3 modes 1, 3-5"    FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 2370.08  #                            RP,      RM
*Want "CLFDBR max M3 modes 6, 7"      FFFFFFFF FFFFFFFF

r 2380.10  #                            RZ,      RP,      RM,      RFS
*Want "CLFDBR 0.75 FPC modes 1-3, 7"  00000000 00000001 00000000 00000001
r 2390.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFDBR 0.75 M3 modes 1, 3-5"   00000001 00000001 00000001 00000000
r 23A0.08  #                            RP,      RM
*Want "CLFDBR 0.75 M3 modes 6, 7"     00000001 00000000

r 23B0.10  #                            RZ,      RP,      RM,      RFS
*Want "CLFDBR 0.25 FPC modes 1-3, 7"  00000000 00000001 00000000 00000001
r 23C0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFDBR 0.25 M3 modes 1, 3-5"   00000000 00000001 00000000 00000000
r 23D0.08  #                            RP,      RM
*Want "CLFDBR 0.25 M3 modes 6, 7"     00000001 00000000


#  rounding mode tests - long BFP - FPCR contents with cc in last byte
*Compare
r 2600.10
*Want "CLFDBR -1.5 FPC modes 1-3, 7 FPCR"  00800003 00800003 00800003 00800003
r 2610.10
*Want "CLFDBR -1.5 M3 modes 1, 3-5 FPCR"   00880003 00880003 00880003 00880003
r 2620.08
*Want "CLFDBR -1.5 M3 modes 6, 7 FPCR"     00880003 00880003

r 2630.10
*Want "CLFDBR -0.5 FPC modes 1-3, 7 FPCR"  00000001 00000001 00800003 00800003
r 2640.10
*Want "CLFDBR -0.5 M3 modes 1, 3-5 FPCR"   00880003 00880003 00080001 00080001
r 2650.08
*Want "CLFDBR -0.5 M3 modes 6, 7 FPCR"     00080001 00880003

r 2660.10
*Want "CLFDBR +0.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 2670.10
*Want "CLFDBR +0.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 2680.08
*Want "CLFDBR +0.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 2690.10
*Want "CLFDBR +1.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 26A0.10
*Want "CLFDBR +1.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 26B0.08
*Want "CLFDBR +1.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 26C0.10
*Want "CLFDBR +2.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 26D0.10
*Want "CLFDBR +2.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 26E0.08
*Want "CLFDBR +2.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 26F0.10
*Want "CLFDBR +5.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 2700.10
*Want "CLFDBR +5.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 2710.08
*Want "CLFDBR +5.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 2720.10
*Want "CLFDBR +9.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 2730.10
*Want "CLFDBR +9.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 2740.08
*Want "CLFDBR +9.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 2750.10
*Want "CLFDBR max FPC modes 1-3, 7 FPCR"   00000002 00800003 00000002 00000002
r 2760.10
*Want "CLFDBR max M3 modes 1, 3-5 FPCR"    00880003 00080002 00880003 00080002
r 2770.08
*Want "CLFDBR max M3 modes 6, 7 FPCR"      00880003 00080002

r 2780.10
*Want "CLFDBR +0.75 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 2790.10
*Want "CLFDBR +0.75 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 27A0.08
*Want "CLFDBR +0.75 M3 modes 6, 7 FPCR"     00080002 00080002

r 27B0.10
*Want "CLFDBR +0.25 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 27C0.10
*Want "CLFDBR +0.25 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 27D0.08
*Want "CLFDBR +0.25 M3 modes 6, 7 FPCR"     00080002 00080002


# BFP extended inputs converted to uint-32 test results
*Compare
r 3000.10
*Want "CLFXBR result pairs 1-2" 00000001 00000001 00000002 00000002
r 3010.10
*Want "CLFXBR result pairs 3-4" 00000004 00000004 00000000 00000000
r 3020.10
*Want "CLFXBR result pairs 5-6" 00000000 00000000 FFFFFFFF 00000000
r 3030.10
*Want "CLFXBR result pairs 7-8" FFFFFFFF FFFFFFFF 00000001 00000001
r 3040.08
*Want "CLFXBR result pair 9"    00000000 00000000

# BFP extended inputs converted to uint-32 FPCR contents, cc
*Compare
r 3100.10
*Want "CLFXBR FPC pairs 1-2" 00000002 F8000002 00000002 F8000002
r 3110.10
*Want "CLFXBR FPC pairs 3-4" 00000002 F8000002 00880003 F8008000 
r 3120.10
*Want "CLFXBR FPC pairs 5-6" 00880003 F8008000 00880003 F8008000
r 3130.10
*Want "CLFXBR FPC pairs 7-8" 00080002 F8000802 00080002 F8000C02
r 3140.08
*Want "CLFXBR FPC pair 9"    00080002 F8000802


#  rounding mode tests - extended BFP - results from rounding
*Compare
r 3200.10  #                            RZ,     RP,      RM,      RFS
*Want "CLFXBR -1.5 FPC modes 1-3, 7"  00000000 00000000 00000000 00000000
r 3210.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFXBR -1.5 M3 modes 1, 3-5"   00000000 00000000 00000000 00000000
r 3220.08  #                            RP,      RM
*Want "CLFXBR -1.5 M3 modes 6, 7"     00000000 00000000

r 3230.10  #                            RZ,     RP,      RM,      RFS
*Want "CLFXBR -0.5 FPC modes 1-3, 7"  00000000 00000000 00000000 00000000
r 3240.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFXBR -0.5 M3 modes 1, 3-5"   00000000 00000000 00000000 00000000
r 3250.08  #                            RP,      RM
*Want "CLFXBR -0.5 M3 modes 6, 7"     00000000 00000000

r 3260.10  #                            RZ,    RP,      RM,      RFS
*Want "CLFXBR 0.5 FPC modes 1-3, 7"   00000000 00000001 00000000 00000001
r 3270.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFXBR 0.5 M3 modes 1, 3-5"    00000001 00000001 00000000 00000000
r 3280.08  #                            RP,      RM
*Want "CLFXBR 0.5 M3 modes 6, 7"      00000001 00000000

r 3290.10  #                            RZ,    RP,      RM,      RFS
*Want "CLFXBR 1.5 FPC modes 1-3, 7"   00000001 00000002 00000001 00000001
r 32A0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFXBR 1.5 M3 modes 1, 3-5"    00000002 00000001 00000002 00000001
r 32B0.08  #                            RP,      RM
*Want "CLFXBR 1.5 M3 modes 6, 7"      00000002 00000001

r 32C0.10  #                            RZ,    RP,      RM,      RFS
*Want "CLFXBR 2.5 FPC modes 1-3, 7"   00000002 00000003 00000002 00000003
r 32D0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFXBR 2.5 M3 modes 1, 3-5"    00000003 00000003 00000002 00000002
r 32E0.08  #                            RP,      RM
*Want "CLFXBR 2.5 M3 modes 6, 7"      00000003 00000002

r 32F0.10  #                            RZ,    RP,      RM,      RFS
*Want "CLFXBR 5.5 FPC modes 1-3, 7"   00000005 00000006 00000005 00000005
r 3300.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFXBR 5.5 M3 modes 1, 3-5"    00000006 00000005 00000006 00000005
r 3310.08  #                            RP,      RM
*Want "CLFXBR 5.5 M3 modes 6, 7"      00000006 00000005

r 3320.10  #                            RZ,    RP,      RM,      RFS
*Want "CLFXBR 9.5 FPC modes 1-3, 7"   00000009 0000000A 00000009 00000009
r 3330.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFXBR 9.5 M3 modes 1, 3-5"    0000000A 00000009 0000000A 00000009
r 3340.08  #                            RP,      RM
*Want "CLFXBR 9.5 M3 modes 6, 7"      0000000A 00000009

r 3350.10  #                            RZ,    RP,      RM,      RFS
*Want "CLFXBR max FPC modes 1-3, 7"   FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 3360.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFXBR max M3 modes 1, 3-5"    FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 3370.08  #                            RP,      RM
*Want "CLFXBR max M3 modes 6, 7"      FFFFFFFF FFFFFFFF

r 3380.10  #                            RZ,    RP,      RM,      RFS
*Want "CLFXBR 0.75 FPC modes 1-3, 7"  00000000 00000001 00000000 00000001
r 3390.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFXBR 0.75 M3 modes 1, 3-5"   00000001 00000001 00000001 00000000
r 33A0.08  #                            RP,      RM
*Want "CLFXBR 0.75 M3 modes 6, 7"     00000001 00000000

r 33B0.10  #                            RZ,    RP,      RM,      RFS
*Want "CLFXBR 0.25 FPC modes 1-3, 7"  00000000 00000001 00000000 00000001
r 33C0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CLFXBR 0.25 M3 modes 1, 3-5"   00000000 00000001 00000000 00000000
r 33D0.08  #                            RP,      RM
*Want "CLFXBR 0.25 M3 modes 6, 7"     00000001 00000000


r 3600.10
*Want "CLFXBR -1.5 FPC modes 1-3, 7 FPCR"  00800003 00800003 00800003 00800003
r 3610.10
*Want "CLFXBR -1.5 M3 modes 1, 3-5 FPCR"   00880003 00880003 00880003 00880003
r 3620.08
*Want "CLFXBR -1.5 M3 modes 6, 7 FPCR"     00880003 00880003

r 3630.10
*Want "CLFXBR -0.5 FPC modes 1-3, 7 FPCR"  00000001 00000001 00800003 00800003
r 3640.10
*Want "CLFXBR -0.5 M3 modes 1, 3-5 FPCR"   00880003 00880003 00080001 00080001
r 3650.08
*Want "CLFXBR -0.5 M3 modes 6, 7 FPCR"     00080001 00880003

r 3660.10
*Want "CLFXBR +0.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 3670.10
*Want "CLFXBR +0.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 3680.08
*Want "CLFXBR +0.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 3690.10
*Want "CLFXBR +1.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 36A0.10
*Want "CLFXBR +1.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 36B0.08
*Want "CLFXBR +1.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 36C0.10
*Want "CLFXBR +2.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 36D0.10
*Want "CLFXBR +2.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 36E0.08
*Want "CLFXBR +2.5 M3 modes 5-7"           00080002 00080002

r 36F0.10
*Want "CLFXBR +5.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 3700.10
*Want "CLFXBR +5.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 3710.08
*Want "CLFXBR +5.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 3720.10
*Want "CLFXBR +9.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 3730.10
*Want "CLFXBR +9.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 3740.08
*Want "CLFXBR +9.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 3750.10
*Want "CLFXBR max FPC modes 1-3, 7 FPCR"   00000002 00800003 00000002 00000002
r 3760.10
*Want "CLFXBR max M3 modes 1, 3-5 FPCR"    00880003 00080002 00880003 00080002
r 3770.08
*Want "CLFXBR max M3 modes 6, 7 FPCR"      00880003 00080002

r 3780.10
*Want "CLFXBR +0.75 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 3790.10
*Want "CLFXBR +0.75 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 37A0.08
*Want "CLFXBR +0.75 M3 modes 6, 7 FPCR"     00080002 00080002

r 37B0.10
*Want "CLFXBR +0.25 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 37C0.10
*Want "CLFXBR +0.25 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 37D0.08
*Want "CLFXBR +0.25 M3 modes 6, 7 FPCR"     00080002 00080002



*Done

