*Testcase bfp-005-cvttolog64.tst: CLGEBR, CLGDBR, CLGXBR

#Testcase bfp-005-cvtfrLOG64.tst: IEEE Convert To Logical
#..Includes CONVERT TO LOGICAL 64 (3).  Tests traps, exceptions, results
#..from different rounding modes, and NaN propagation.


sysclear
archmode esame

#
# Following suppresses logging of program checks.  This test program, as part
# of its normal operation, generates 16 program check messages that have no
# value in the validation process.  (The messages, not the program checks.)
#
ostailor quiet

loadcore "$(testpath)/bfp-005-cvttolog64.core"

runtest 1.0

ostailor null   # restore messages for subsequent tests


# BFP short inputs converted to uint-64 - results
*Compare
r 1000.10  
*Want "CLGEBR result pair 1" 00000000 00000001 00000000 00000001
r 1010.10
*Want "CLGEBR result pair 2" 00000000 00000002 00000000 00000002
r 1020.10
*Want "CLGEBR result pair 3" 00000000 00000004 00000000 00000004
r 1030.10
*Want "CLGEBR result pair 4" 00000000 00000000 00000000 00000000
r 1040.10
*Want "CLGEBR result pair 5" 00000000 00000000 00000000 00000000
r 1050.10
*Want "CLGEBR result pair 6" FFFFFFFF FFFFFFFF 00000000 00000000
r 1060.10
*Want "CLGEBR result pair 7" FFFFFF00 00000000 FFFFFF00 00000000
r 1070.10
*Want "CLGEBR result pair 8" 00000000 00000001 00000000 00000001
r 1080.10
*Want "CLGEBR result pair 9" 00000000 00000000 00000000 00000000

# I am not satisfied with test case 6  further investigation needed. 
# It would appear that f32_to_uint64 is returning invalid and max uint-64.

# Short BFP inputs converted to uint-64 - FPCR contents
*Compare
r 1200.10
*Want "CLGEBR FPCR pairs 1-2" 00000002 F8000002 00000002 F8000002
r 1210.10
*Want "CLGEBR FPCR pairs 3-4" 00000002 F8000002 00880003 F8008000
r 1220.10
*Want "CLGEBR FPCR pairs 5-6" 00880003 F8008000 00880003 F8008000
r 1230.10
*Want "CLGEBR FPCR pairs 7-8" 00000002 F8000002 00080002 F8000C02 
r 1240.08
*Want "CLGEBR FPCR pair 9"    00080002 F8000802


#  short BFP inputs converted to uint-64 - results from rounding
*Compare
r 1300.10  #                         RZ,               RP
*Want "CLGEBR -1.5 FPC modes 1, 2" 00000000 00000000 00000000 00000000
r 1310.10  #                         RM,               RFS
*Want "CLGEBR -1.5 FPC modes 3, 7" 00000000 00000000 00000000 00000000
r 1320.10  #                         RNTA,             RFS
*Want "CLGEBR -1.5 M3 modes 1, 3"  00000000 00000000 00000000 00000000
r 1330.10  #                         RNTE,             RZ
*Want "CLGEBR -1.5 M3 modes 4, 5"  00000000 00000000 00000000 00000000
r 1340.10  #                         RP,               RM
*Want "CLGEBR -1.5 M3 modes 6, 7"  00000000 00000000 00000000 00000000

r 1350.10  #                         RZ,               RP
*Want "CLGEBR -0.5 FPC modes 1, 2" 00000000 00000000 00000000 00000000
r 1360.10  #                         RM,               RFS
*Want "CLGEBR -0.5 FPC modes 3, 7" 00000000 00000000 00000000 00000000
r 1370.10  #                         RNTA,             RFS
*Want "CLGEBR -0.5 M3 modes 1, 3"  00000000 00000000 00000000 00000000
r 1380.10  #                         RNTE,             RZ
*Want "CLGEBR -0.5 M3 modes 4, 5"  00000000 00000000 00000000 00000000
r 1390.10  #                         RP,               RM
*Want "CLGEBR -0.5 M3 modes 6, 7"  00000000 00000000 00000000 00000000

r 13A0.10  #                         RZ,               RP
*Want "CLGEBR +0.5 FPC modes 1, 2" 00000000 00000000 00000000 00000001
r 13B0.10  #                         RM,               RFS
*Want "CLGEBR +0.5 FPC modes 3, 7" 00000000 00000000 00000000 00000001
r 13C0.10  #                         RNTA,             RFS
*Want "CLGEBR +0.5 M3 modes 1, 3"  00000000 00000001 00000000 00000001
r 13D0.10  #                         RNTE,             RZ
*Want "CLGEBR +0.5 M3 modes 4, 5"  00000000 00000000 00000000 00000000
r 13E0.10  #                         RP,               RM
*Want "CLGEBR +0.5 M3 modes 6, 7"  00000000 00000001 00000000 00000000

r 13F0.10  #                         RZ,               RP
*Want "CLGEBR +1.5 FPC modes 1, 2" 00000000 00000001 00000000 00000002
r 1400.10  #                         RM,               RFS
*Want "CLGEBR +1.5 FPC modes 3, 7" 00000000 00000001 00000000 00000001
r 1410.10  #                         RNTA,             RFS
*Want "CLGEBR +1.5 M3 modes 1, 3"  00000000 00000002 00000000 00000001
r 1420.10  #                         RNTE,             RZ
*Want "CLGEBR +1.5 M3 modes 4, 5"  00000000 00000002 00000000 00000001
r 1430.10  #                         RP,               RM
*Want "CLGEBR +1.5 M3 modes 6, 7"  00000000 00000002 00000000 00000001

r 1440.10  #                         RZ,               RP
*Want "CLGEBR +2.5 FPC modes 1, 2" 00000000 00000002 00000000 00000003
r 1450.10  #                         RM,               RFS
*Want "CLGEBR +2.5 FPC modes 3, 7" 00000000 00000002 00000000 00000003
r 1460.10  #                         RNTA,             RFS
*Want "CLGEBR +2.5 M3 modes 1, 3"  00000000 00000003 00000000 00000003
r 1470.10  #                         RNTE,             RZ
*Want "CLGEBR +2.5 M3 modes 4, 5"  00000000 00000002 00000000 00000002
r 1480.10  #                         RP,               RM
*Want "CLGEBR +2.5 M3 modes 6, 7"  00000000 00000003 00000000 00000002

r 1490.10  #                         RZ,               RP
*Want "CLGEBR +5.5 FPC modes 1, 2" 00000000 00000005 00000000 00000006
r 14A0.10  #                         RM,               RFS
*Want "CLGEBR +5.5 FPC modes 3, 7" 00000000 00000005 00000000 00000005
r 14B0.10  #                         RNTA,             RFS
*Want "CLGEBR +5.5 M3 modes 1, 3"  00000000 00000006 00000000 00000005
r 14C0.10  #                         RNTE,             RZ
*Want "CLGEBR +5.5 M3 modes 4, 5"  00000000 00000006 00000000 00000005
r 14D0.10  #                         RP,               RM
*Want "CLGEBR +5.5 M3 modes 6, 7"  00000000 00000006 00000000 00000005

r 14E0.10  #                         RZ,               RP
*Want "CLGEBR +9.5 FPC modes 1, 2" 00000000 00000009 00000000 0000000A
r 14F0.10  #                         RM,               RFS
*Want "CLGEBR +9.5 FPC modes 3, 7" 00000000 00000009 00000000 00000009
r 1500.10  #                         RNTA,             RFS
*Want "CLGEBR +9.5 M3 modes 1, 3"  00000000 0000000A 00000000 00000009
r 1510.10  #                         RNTE,             RZ
*Want "CLGEBR +9.5 M3 modes 4, 5"  00000000 0000000A 00000000 00000009
r 1520.10  #                         RP,               RM
*Want "CLGEBR +9.5 M3 modes 6, 7"  00000000 0000000A 00000000 00000009

r 1530.10  #                         RZ,               RP
*Want "CLGEBR max FPC modes 1, 2"  FFFFFF00 00000000 FFFFFF00 00000000
r 1540.10  #                         RM,               RFS
*Want "CLGEBR max FPC modes 3, 7"  FFFFFF00 00000000 FFFFFF00 00000000
r 1550.10  #                         RNTA,             RFS
*Want "CLGEBR max M3 modes 1, 3"   FFFFFF00 00000000 FFFFFF00 00000000
r 1560.10  #                         RNTE,             RZ
*Want "CLGEBR max M3 modes 4, 5"   FFFFFF00 00000000 FFFFFF00 00000000
r 1570.10  #                         RP,               RM
*Want "CLGEBR max M3 modes 6, 7"   FFFFFF00 00000000 FFFFFF00 00000000

r 1580.10  #                          RZ,               RP
*Want "CLGEBR +0.75 FPC modes 1, 2" 00000000 00000000 00000000 00000001
r 1590.10  #                          RM,               RFS
*Want "CLGEBR +0.75 FPC modes 3, 7" 00000000 00000000 00000000 00000001
r 15A0.10  #                          RNTA,             RFS
*Want "CLGEBR +0.75 M3 modes 1, 3"  00000000 00000001 00000000 00000001
r 15B0.10  #                          RNTE,             RZ
*Want "CLGEBR +0.75 M3 modes 4, 5"  00000000 00000001 00000000 00000000
r 15C0.10  #                          RP,               RM
*Want "CLGEBR +0.75 M3 modes 6, 7"  00000000 00000001 00000000 00000000

r 15D0.10  #                          RZ,               RP
*Want "CLGEBR +0.25 FPC modes 1, 2" 00000000 00000000 00000000 00000001
r 15E0.10  #                          RM,               RFS
*Want "CLGEBR +0.25 FPC modes 3, 7" 00000000 00000000 00000000 00000001
r 15F0.10  #                          RNTA,             RFS
*Want "CLGEBR +0.25 M3 modes 1, 3"  00000000 00000000 00000000 00000001
r 1600.10  #                          RNTE,             RZ
*Want "CLGEBR +0.25 M3 modes 4, 5"  00000000 00000000 00000000 00000000
r 1610.10  #                          RP,               RM
*Want "CLGEBR +0.25 M3 modes 6, 7"  00000000 00000001 00000000 00000000


#  short BFP inputs converted to uint-64 - FPCR with cc in last byte
*Compare 
r 1800.10
*Want "CLGEBR -1.5 FPC modes 1-3, 7 FPCR"  00800003 00800003 00800003 00800003
r 1810.10
*Want "CLGEBR -1.5 M3 modes 1, 3-5 FPCR"   00880003 00880003 00880003 00880003
r 1820.08
*Want "CLGEBR -1.5 M3 modes 6, 7 FPCR"     00880003 00880003

r 1830.10
*Want "CLGEBR -0.5 FPC modes 1-3, 7 FPCR"  00000001 00000001 00800003 00800003
r 1840.10
*Want "CLGEBR -0.5 M3 modes 1, 3-5 FPCR"   00880003 00880003 00080001 00080001
r 1850.08
*Want "CLGEBR -0.5 M3 modes 6, 7 FPCR"     00080001 00880003

r 1860.10
*Want "CLGEBR +0.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 1870.10
*Want "CLGEBR +0.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1880.08
*Want "CLGEBR +0.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 1890.10
*Want "CLGEBR +1.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 18A0.10
*Want "CLGEBR +1.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 18B0.08
*Want "CLGEBR +1.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 18C0.10
*Want "CLGEBR +2.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 18D0.10
*Want "CLGEBR +2.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 18E0.08
*Want "CLGEBR +2.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 18F0.10
*Want "CLGEBR +5.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 1900.10
*Want "CLGEBR +5.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1910.08
*Want "CLGEBR +5.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 1920.10
*Want "CLGEBR +9.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 1930.10
*Want "CLGEBR +9.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1940.08
*Want "CLGEBR +9.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 1950.10
*Want "CLGEBR max+1 FPC modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 1960.10
*Want "CLGEBR max+1 M3 modes 1, 3-5 FPCR"  00000002 00000002 00000002 00000002
r 1970.08
*Want "CLGEBR max+1 M3 modes 6, 7 FPCR"    00000002 00000002

r 1950.10
*Want "CLGEBR max FPC modes 1-3, 7 FPCR"   00000002 00000002 00000002 00000002
r 1960.10
*Want "CLGEBR max M3 modes 1, 3-5 FPCR"    00000002 00000002 00000002 00000002
r 1970.08
*Want "CLGEBR max M3 modes 6, 7 FPCR"      00000002 00000002

r 1980.10
*Want "CLGEBR +0.75 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 1990.10
*Want "CLGEBR +0.75 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 19A0.08
*Want "CLGEBR +0.75 M3 modes 6, 7 FPCR"     00080002 00080002

r 19B0.10
*Want "CLGEBR +0.25 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 19C0.10
*Want "CLGEBR +0.25 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 19D0.08
*Want "CLGEBR +0.5 M3 modes 6, 7 FPCR"     00080002 00080002


# BFP long inputs converted to uint-64 - results
*Compare
r 2000.10
*Want "CLGDBR result pair 1" 00000000 00000001 00000000 00000001
r 2010.10
*Want "CLGDBR result pair 2" 00000000 00000002 00000000 00000002
r 2020.10
*Want "CLGDBR result pair 3" 00000000 00000004 00000000 00000004
r 2030.10
*Want "CLGDBR result pair 4" 00000000 00000000 00000000 00000000
r 2040.10
*Want "CLGDBR result pair 5" 00000000 00000000 00000000 00000000
r 2050.10
*Want "CLGDBR result pair 6" FFFFFFFF FFFFFFFF 00000000 00000000
r 2060.10
*Want "CLGDBR result pair 7" FFFFFFFF FFFFF800 FFFFFFFF FFFFF800
r 2070.10
*Want "CLGDBR result pair 8" 00000000 00000001 00000000 00000001
r 2080.10
*Want "CLGDBR result pair 9" 00000000 00000000 00000000 00000000

*Compare
r 2200.10
*Want "CLGDBR FPCR pairs 1-2" 00000002 F8000002 00000002 F8000002
r 2210.10
*Want "CLGDBR FPCR pairs 3-4" 00000002 F8000002 00880003 F8008000
r 2220.10
*Want "CLGDBR FPCR pairs 5-6" 00880003 F8008000 00880003 F8008000
r 2230.10
*Want "CLGDBR FPCR pairs 7-8" 00000002 F8000002 00080002 F8000C02 
r 2240.08
*Want "CLGDBR FPCR pair 9"    00080002 F8000802


#  Long BFP inputs converted to uint-64 - results from rounding
*Compare
r 2300.10  #                         RZ,                RP
*Want "CLGDBR -1.5 FPC modes 1, 2" 00000000 00000000 00000000 00000000
r 2310.10  #                         RM,                RFS
*Want "CLGDBR -1.5 FPC modes 3, 7" 00000000 00000000 00000000 00000000
r 2320.10  #                         RNTA,             RFS
*Want "CLGDBR -1.5 M3 modes 1, 3"  00000000 00000000 00000000 00000000
r 2330.10  #                         RNTE,             RZ
*Want "CLGDBR -1.5 M3 modes 4, 5"  00000000 00000000 00000000 00000000
r 2340.10  #                         RP,               RM
*Want "CLGDBR -1.5 M3 modes 6, 7"  00000000 00000000 00000000 00000000

r 2350.10  #                         RZ,               RP
*Want "CLGDBR -0.5 FPC modes 1, 2" 00000000 00000000 00000000 00000000
r 2360.10  #                         RM,               RFS
*Want "CLGDBR -0.5 FPC modes 3, 7" 00000000 00000000 00000000 00000000
r 2370.10  #                         RNTA,             RFS
*Want "CLGDBR -0.5 M3 modes 1, 3"  00000000 00000000 00000000 00000000
r 2380.10  #                         RNTE,             RZ
*Want "CLGDBR -0.5 M3 modes 4, 5"  00000000 00000000 00000000 00000000
r 2390.10  #                         RP,               RM
*Want "CLGDBR -0.5 M3 modes 6, 7"  00000000 00000000 00000000 00000000

r 23A0.10  #                         RZ,              RP
*Want "CLGDBR +0.5 FPC modes 1, 2" 00000000 00000000 00000000 00000001
r 23B0.10  #                         RM,              RFS
*Want "CLGDBR +0.5 FPC modes 3, 7" 00000000 00000000 00000000 00000001
r 23C0.10  #                         RNTA,            RFS
*Want "CLGDBR +0.5 M3 modes 1, 3"  00000000 00000001 00000000 00000001
r 23D0.10  #                         RNTE,            RZ
*Want "CLGDBR +0.5 M3 modes 4, 5"  00000000 00000000 00000000 00000000
r 23E0.10  #                         RP,              RM
*Want "CLGDBR +0.5 M3 modes 6, 7"  00000000 00000001 00000000 00000000

r 23F0.10  #                         RZ,               RP
*Want "CLGDBR +1.5 FPC modes 1, 2" 00000000 00000001 00000000 00000002
r 2400.10  #                         RM,               RFS
*Want "CLGDBR +1.5 FPC modes 3, 7" 00000000 00000001 00000000 00000001
r 2410.10  #                         RNTA,             RFS
*Want "CLGDBR +1.5 M3 modes 1, 3"  00000000 00000002 00000000 00000001
r 2420.10  #                         RNTE,             RZ
*Want "CLGDBR +1.5 M3 modes 4, 5"  00000000 00000002 00000000 00000001
r 2430.10  #                         RP,               RM
*Want "CLGDBR +1.5 M3 modes 6, 7"  00000000 00000002 00000000 00000001

r 2440.10  #                         RZ,               RP
*Want "CLGDBR +2.5 FPC modes 1, 2" 00000000 00000002 00000000 00000003
r 2450.10  #                         RM,               RFS
*Want "CLGDBR +2.5 FPC modes 3, 7" 00000000 00000002 00000000 00000003
r 2460.10  #                         RNTA,             RFS
*Want "CLGDBR +2.5 M3 modes 1, 3"  00000000 00000003 00000000 00000003
r 2470.10  #                         RNTE,             RZ
*Want "CLGDBR +2.5 M3 modes 4, 5"  00000000 00000002 00000000 00000002
r 2480.10  #                         RP,               RM
*Want "CLGDBR +2.5 M3 modes 6, 7"  00000000 00000003 00000000 00000002

r 2490.10  #                         RZ,               RP
*Want "CLGDBR +5.5 FPC modes 1, 2" 00000000 00000005 00000000 00000006
r 24A0.10  #                         RM,               RFS
*Want "CLGDBR +5.5 FPC modes 3, 7" 00000000 00000005 00000000 00000005
r 24B0.10  #                         RNTA,             RFS
*Want "CLGDBR +5.5 M3 modes 1, 3"  00000000 00000006 00000000 00000005
r 24C0.10  #                         RNTE,             RZ
*Want "CLGDBR +5.5 M3 modes 4, 5"  00000000 00000006 00000000 00000005
r 24D0.10  #                         RP,               RM
*Want "CLGDBR +5.5 M3 modes 6, 7"  00000000 00000006 00000000 00000005

r 24E0.10  #                         RZ,               RP
*Want "CLGDBR +9.5 FPC modes 1, 2" 00000000 00000009 00000000 0000000A
r 24F0.10  #                         RM,               RFS
*Want "CLGDBR +9.5 FPC modes 3, 7" 00000000 00000009 00000000 00000009
r 2500.10  #                         RNTA,             RFS
*Want "CLGDBR +9.5 M3 modes 1, 3"  00000000 0000000A 00000000 00000009
r 2510.10  #                         RNTE,             RZ
*Want "CLGDBR +9.5 M3 modes 4, 5"  00000000 0000000A 00000000 00000009
r 2520.10  #                         RP,               RM
*Want "CLGDBR +9.5 M3 modes 6, 7"  00000000 0000000A 00000000 00000009

r 2530.10  #                         RZ,               RP
*Want "CLGDBR max FPC modes 1, 2"  FFFFFFFF FFFFF800 FFFFFFFF FFFFF800
r 2540.10  #                         RM,               RFS
*Want "CLGDBR max FPC modes 3, 7"  FFFFFFFF FFFFF800 FFFFFFFF FFFFF800
r 2550.10  #                         RNTA,             RFS
*Want "CLGDBR max M3 modes 1, 3"   FFFFFFFF FFFFF800 FFFFFFFF FFFFF800
r 2560.10  #                         RNTE,             RZ
*Want "CLGDBR max M3 modes 4, 5"   FFFFFFFF FFFFF800 FFFFFFFF FFFFF800
r 2570.10  #                         RP,               RM
*Want "CLGDBR max M3 modes 6, 7"   FFFFFFFF FFFFF800 FFFFFFFF FFFFF800


#  Long BFP inputs converted to uint-64 - FPCR contents with cc in last byte
*Compare
r 2800.10
*Want "CLGDBR -1.5 FPC modes 1-3, 7 FPCR"  00800003 00800003 00800003 00800003
r 2810.10
*Want "CLGDBR -1.5 M3 modes 1, 3-5 FPCR"   00880003 00880003 00880003 00880003
r 2820.08
*Want "CLGDBR -1.5 M3 modes 6, 7 FPCR"     00880003 00880003

r 2830.10
*Want "CLGDBR -0.5 FPC modes 1-3, 7 FPCR"  00000001 00000001 00800003 00800003
r 2840.10
*Want "CLGDBR -0.5 M3 modes 1, 3-5 FPCR"   00880003 00880003 00080001 00080001
r 2850.08
*Want "CLGDBR -0.5 M3 modes 6, 7 FPCR"     00080001 00880003

r 2860.10
*Want "CLGDBR +0.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 2870.10
*Want "CLGDBR +0.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 2880.08
*Want "CLGDBR +0.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 2890.10
*Want "CLGDBR +1.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 28A0.10
*Want "CLGDBR +1.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 28B0.08
*Want "CLGDBR +1.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 28C0.10
*Want "CLGDBR +2.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 28D0.10
*Want "CLGDBR +2.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 28E0.08
*Want "CLGDBR +2.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 28F0.10
*Want "CLGDBR +5.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 2900.10
*Want "CLGDBR +5.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 2910.08
*Want "CLGDBR +5.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 2920.10
*Want "CLGDBR +9.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 2930.10
*Want "CLGDBR +9.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 2940.08
*Want "CLGDBR +9.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 2950.10
*Want "CLGDBR max FPC modes 1-3, 7 FPCR"   00000002 00000002 00000002 00000002
r 2960.10
*Want "CLGDBR max M3 modes 1, 3-5 FPCR"    00000002 00000002 00000002 00000002
r 2970.08
*Want "CLGDBR max M3 modes 6, 7 FPCR"      00000002 00000002


# Extended BFP inputs converted to uint-64 - results
*Compare
r 3000.10
*Want "CLGXBR result pair 1"  00000000 00000001 00000000 00000001
r 3010.10
*Want "CLGXBR result pair 2"  00000000 00000002 00000000 00000002
r 3020.10
*Want "CLGXBR result pair 3"  00000000 00000004 00000000 00000004
r 3030.10
*Want "CLGXBR result pair 4"  00000000 00000000 00000000 00000000
r 3040.10
*Want "CLGXBR result pair 5"  00000000 00000000 00000000 00000000
r 3050.10
*Want "CLGXBR result pair 6"  FFFFFFFF FFFFFFFF 00000000 00000000
r 3060.10
*Want "CLGXBR result pair 7"  FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 3070.10
*Want "CLGXBR result pair 8"  FFFFFFFF FFFFFFFF 00000000 00000000
r 3080.10
*Want "CLGXBR result pair 9"  00000000 00000001 00000000 00000001
r 3090.10
*Want "CLGXBR result pair 10" 00000000 00000000 00000000 00000000

*Compare
r 3200.10
*Want "CLGXBR FPC pairs 1-2"  00000002 F8000002 00000002 F8000002
r 3210.10
*Want "CLGXBR FPC pairs 3-4"  00000002 F8000002 00880003 F8008000
r 3220.10
*Want "CLGXBR FPC pairs 5-6"  00880003 F8008000 00880003 F8008000
r 3230.10
*Want "CLGXBR FPC pairs 7-8"  00000002 F8000002 00880003 F8008000
r 3240.10
*Want "CLGXBR FPC pairs 9-10" 00080002 F8000C02 00080002 F8000802


#  Long BFP inputs converted to uint-64 - results from rounding
*Compare
r 3300.10  #                         RZ,               RP
*Want "CLGXBR -1.5 FPC modes 1, 2" 00000000 00000000 00000000 00000000
r 3310.10  #                         RM,               RFS
*Want "CLGXBR -1.5 FPC modes 3, 7" 00000000 00000000 00000000 00000000
r 3320.10  #                         RNTA,             RFS
*Want "CLGXBR -1.5 M3 modes 1, 3"  00000000 00000000 00000000 00000000
r 3330.10  #                         RNTE,             RZ
*Want "CLGXBR -1.5 M3 modes 4, 5"  00000000 00000000 00000000 00000000
r 3340.10  #                         RP,               RM
*Want "CLGXBR -1.5 M3 modes 6, 7"  00000000 00000000 00000000 00000000

r 3350.10  #                         RZ,               RP
*Want "CLGXBR -0.5 FPC modes 1, 2" 00000000 00000000 00000000 00000000
r 3360.10  #                         RM,               RFS
*Want "CLGXBR -0.5 FPC modes 3, 7" 00000000 00000000 00000000 00000000
r 3370.10  #                         RNTA,             RFS
*Want "CLGXBR -0.5 M3 modes 1, 3"  00000000 00000000 00000000 00000000
r 3380.10  #                         RNTE,             RZ
*Want "CLGXBR -0.5 M3 modes 4, 5"  00000000 00000000 00000000 00000000
r 3390.10  #                         RP,               RM
*Want "CLGXBR -0.5 M3 modes 6, 7"  00000000 00000000 00000000 00000000

r 33A0.10  #                         RZ,               RP
*Want "CLGXBR +0.5 FPC modes 1, 2" 00000000 00000000 00000000 00000001
r 33B0.10  #                         RM,               RFS
*Want "CLGXBR +0.5 FPC modes 3, 7" 00000000 00000000 00000000 00000001
r 33C0.10  #                         RNTA,             RFS
*Want "CLGXBR +0.5 M3 modes 1, 3"  00000000 00000001 00000000 00000001
r 33D0.10  #                         RNTE,             RZ
*Want "CLGXBR +0.5 M3 modes 4, 5"  00000000 00000000 00000000 00000000
r 33E0.10  #                         RP,               RM
*Want "CLGXBR +0.5 M3 modes 6, 7"  00000000 00000001 00000000 00000000

r 33F0.10  #                         RZ,               RP
*Want "CLGXBR +1.5 FPC modes 1, 2" 00000000 00000001 00000000 00000002
r 3400.10  #                         RM,               RFS
*Want "CLGXBR +1.5 FPC modes 3, 7" 00000000 00000001 00000000 00000001
r 3410.10  #                         RNTA,             RFS
*Want "CLGXBR +1.5 M3 modes 1, 3"  00000000 00000002 00000000 00000001
r 3420.10  #                         RNTE,             RZ
*Want "CLGXBR +1.5 M3 modes 4, 5"  00000000 00000002 00000000 00000001
r 3430.10  #                         RP,               RM
*Want "CLGXBR +1.5 M3 modes 6, 7"  00000000 00000002 00000000 00000001

r 3440.10  #                         RZ,               RP
*Want "CLGXBR +2.5 FPC modes 1, 2" 00000000 00000002 00000000 00000003
r 3450.10  #                         RM,               RFS
*Want "CLGXBR +2.5 FPC modes 3, 7" 00000000 00000002 00000000 00000003
r 3460.10  #                         RNTA,             RFS
*Want "CLGXBR +2.5 M3 modes 1, 3"  00000000 00000003 00000000 00000003
r 3470.10  #                         RNTE,             RZ
*Want "CLGXBR +2.5 M3 modes 4, 5"  00000000 00000002 00000000 00000002
r 3480.10  #                         RP,               RM
*Want "CLGXBR +2.5 M3 modes 6, 7"  00000000 00000003 00000000 00000002

r 3490.10  #                         RZ,               RP
*Want "CLGXBR +5.5 FPC modes 1, 2" 00000000 00000005 00000000 00000006
r 34A0.10  #                         RM,               RFS
*Want "CLGXBR +5.5 FPC modes 3, 7" 00000000 00000005 00000000 00000005
r 34B0.10  #                         RNTA,             RFS
*Want "CLGXBR +5.5 M3 modes 1, 3"  00000000 00000006 00000000 00000005
r 34C0.10  #                         RNTE,             RZ
*Want "CLGXBR +5.5 M3 modes 4, 5"  00000000 00000006 00000000 00000005
r 34D0.10  #                         RP,               RM
*Want "CLGXBR +5.5 M3 modes 6, 7"  00000000 00000006 00000000 00000005

r 34E0.10  #                         RZ,               RP
*Want "CLGXBR +9.5 FPC modes 1, 2" 00000000 00000009 00000000 0000000A
r 34F0.10  #                         RM,               RFS
*Want "CLGXBR +9.5 FPC modes 3, 7" 00000000 00000009 00000000 00000009
r 3500.10  #                         RNTA,             RFS
*Want "CLGXBR +9.5 M3 modes 1, 3"  00000000 0000000A 00000000 00000009
r 3510.10  #                         RNTE,             RZ
*Want "CLGXBR +9.5 M3 modes 4, 5"  00000000 0000000A 00000000 00000009
r 3520.10  #                         RP,               RM
*Want "CLGXBR +9.5 M3 modes 6, 7"  00000000 0000000A 00000000 00000009

r 3530.10  #                             RZ,               RP
*Want "CLGXBR max+0.5 FPC modes 1, 2"  FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 3540.10  #                             RM,               RFS
*Want "CLGXBR max+0.5 FPC modes 3, 7"  FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 3550.10  #                             RNTA,             RFS
*Want "CLGXBR max+0.5 M3 modes 1, 3"   FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 3560.10  #                             RNTE,             RZ
*Want "CLGXBR max+0.5 M3 modes 4, 5"   FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 3570.10  #                             RP,               RM
*Want "CLGXBR max+0.5 M3 modes 6, 7"   FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF

r 3580.10  #                          RZ,               RP
*Want "CLGXBR +0.75 FPC modes 1, 2" 00000000 00000000 00000000 00000001
r 3590.10  #                          RM,               RFS
*Want "CLGXBR +0.75 FPC modes 3, 7" 00000000 00000000 00000000 00000001
r 35A0.10  #                          RNTA,             RFS
*Want "CLGXBR +0.75 M3 modes 1, 3"  00000000 00000001 00000000 00000001
r 35B0.10  #                          RNTE,             RZ
*Want "CLGXBR +0.75 M3 modes 4, 5"  00000000 00000001 00000000 00000000
r 35C0.10  #                          RP,               RM
*Want "CLGXBR +0.75 M3 modes 6, 7"  00000000 00000001 00000000 00000000

r 35D0.10  #                          RZ,               RP
*Want "CLGXBR +0.25 FPC modes 1, 2" 00000000 00000000 00000000 00000001
r 35E0.10  #                          RM,               RFS
*Want "CLGXBR +0.25 FPC modes 3, 7" 00000000 00000000 00000000 00000001
r 35F0.10  #                          RNTA,             RFS
*Want "CLGXBR +0.25 M3 modes 1, 3"  00000000 00000000 00000000 00000001
r 3600.10  #                          RNTE,             RZ
*Want "CLGXBR +0.25 M3 modes 4, 5"  00000000 00000000 00000000 00000000
r 3610.10  #                          RP,               RM
*Want "CLGXBR +0.25 M3 modes 6, 7"  00000000 00000001 00000000 00000000



#  Extended BFP inputs converted to uint-64 - FPCR contents with cc in last byte
*Compare
r 3800.10
*Want "CLGXBR -1.5 FPC modes 1-3, 7 FPCR"  00800003 00800003 00800003 00800003
r 3810.10
*Want "CLGXBR -1.5 M3 modes 1, 3-5 FPCR"   00880003 00880003 00880003 00880003
r 3820.08
*Want "CLGXBR -1.5 M3 modes 6, 7 FPCR"     00880003 00880003

r 3830.10
*Want "CLGXBR -0.5 FPC modes 1-3, 7 FPCR"  00000001 00000001 00800003 00800003
r 3840.10
*Want "CLGXBR -0.5 M3 modes 1, 3-5 FPCR"   00880003 00880003 00080001 00080001
r 3850.08
*Want "CLGXBR -0.5 M3 modes 6, 7 FPCR"     00080001 00880003

r 3860.10
*Want "CLGXBR +0.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 3870.10
*Want "CLGXBR +0.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 3880.08
*Want "CLGXBR +0.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 3890.10
*Want "CLGXBR +1.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 38A0.10
*Want "CLGXBR +1.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 38B0.08
*Want "CLGXBR +1.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 38C0.10
*Want "CLGXBR +2.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 38D0.10
*Want "CLGXBR +2.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 38E0.08
*Want "CLGXBR +2.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 38F0.10
*Want "CLGXBR +5.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 3900.10
*Want "CLGXBR +5.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 3910.08
*Want "CLGXBR +5.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 3920.10
*Want "CLGXBR +9.5 FPC modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 3930.10
*Want "CLGXBR +9.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 3940.08
*Want "CLGXBR +9.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 3950.10
*Want "CLGXBR max+0.5 FPC modes 1-3, 7 FPCR" 00000002 00800003 00000002 00000002
r 3960.10
*Want "CLGXBR max+0.5 M3 modes 1, 3-5 FPCR"  00880003 00080002 00880003 00080002
r 3970.08
*Want "CLGXBR max+0.5 M3 modes 6, 7 FPCR"    00880003 00080002

r 3980.10
*Want "CLGXBR 0.75 FPC modes 1-3, 7 FPCR"   00000002 00000002 00000002 00000002
r 3990.10
*Want "CLGXBR 0.75 M3 modes 1, 3-5 FPCR"    00080002 00080002 00080002 00080002
r 39A0.08
*Want "CLGXBR 0.75 M3 modes 6, 7 FPCR"      00080002 00080002

r 39B0.10
*Want "CLGXBR 0.25 FPC modes 1-3, 7 FPCR"   00000002 00000002 00000002 00000002
r 39C0.10
*Want "CLGXBR 0.25 M3 modes 1, 3-5 FPCR"    00080002 00080002 00080002 00080002
r 39D0.08
*Want "CLGXBR 0.25 M3 modes 6, 7 FPCR"      00080002 00080002


*Done

