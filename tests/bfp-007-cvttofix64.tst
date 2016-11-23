*Testcase bfp-007-cvttofix64.tst: CGEBR, CGBRA, CGDBR, CGDBRA, CGXBR, CGXBRA

#Testcase bfp-007-cvttofix64.tst: IEEE Convert To Fixed
#..Includes CONVERT TO FIXED 64 (6).  Tests traps, exceptions
#..results from different rounding modes, and NaN propagation.

sysclear
archmode esame

#
# Following suppresses logging of program checks.  This test program, as part
# of its normal operation, generates 12 program check messages that have no
# value in the validation process.  (The messages, not the program checks.)
#
ostailor quiet

loadcore "$(testpath)/bfp-007-cvttofix64.core"

runtest 1.0

ostailor null   # restore messages for subsequent tests


# Short BFP inputs to int-64 - results
*Compare
r 1000.10
*Want "CGEBR result pair 1" 00000000 00000001 00000000 00000001
r 1010.10
*Want "CGEBR result pair 2" 00000000 00000002 00000000 00000002
r 1020.10
*Want "CGEBR result pair 3" 00000000 00000004 00000000 00000004
r 1030.10
*Want "CGEBR result pair 4" FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFE
r 1040.10
*Want "CGEBR result pair 5" 80000000 00000000 00000000 00000000
r 1050.10
*Want "CGEBR result pair 6" 80000000 00000000 00000000 00000000
r 1060.10
*Want "CGEBR result pair 7" 7FFFFFFF FFFFFFFF 00000000 00000000
r 1070.10
*Want "CGEBR result pair 8" 80000000 00000000 00000000 00000000
r 1080.10
*Want "CGEBR result pair 9" 7FFFFF80 00000000 7FFFFF80 00000000

# Short BFP inputs to int-64 - FPCR contents
*Compare
r 1200.10
*Want "CGEBR FPCR pairs 1-2" 00000002 F8000002 00000002 F8000002
r 1210.10
*Want "CGEBR FPCR pairs 3-4" 00000002 F8000002 00000001 F8000001
r 1220.10
*Want "CGEBR FPCR pairs 5-6" 00880003 F8008000 00880003 F8008000
r 1230.10
*Want "CGEBR FPCR pairs 7-8" 00880003 F8008000 00880003 F8008000
r 1240.08
*Want "CGEBR FPCR pair 9"    00000002 F8000002

#  rounding mode tests - short BFP - results from rounding
*Compare
r 1300.10 #                          RZ,               RP
*Want "CGEBRA -9.5 FPCR modes 1, 2" FFFFFFFF FFFFFFF7 FFFFFFFF FFFFFFF7
r 1310.10 #                          RM,               RFS
*Want "CGEBRA -9.5 FPCR modes 3, 7" FFFFFFFF FFFFFFF6 FFFFFFFF FFFFFFF7
r 1320.10 #                          RNTA,             RFS
*Want "CGEBRA -9.5 M3 modes 1, 3"   FFFFFFFF FFFFFFF6 FFFFFFFF FFFFFFF7
r 1330.10 #                          RNTE,             RZ
*Want "CGEBRA -9.5 M3 modes 4, 5"   FFFFFFFF FFFFFFF6 FFFFFFFF FFFFFFF7
r 1340.10 #                          RP                RM
*Want "CGEBRA -9.5 M3 modes 6, 7"   FFFFFFFF FFFFFFF7 FFFFFFFF FFFFFFF6

r 1350.10 #                          RZ,               RP
*Want "CGEBRA -5.5 FPCR modes 1, 2" FFFFFFFF FFFFFFFB FFFFFFFF FFFFFFFB
r 1360.10 #                          RM,               RFS
*Want "CGEBRA -5.5 FPCR modes 3, 7" FFFFFFFF FFFFFFFA FFFFFFFF FFFFFFFB
r 1370.10 #                          RNTA,             RFS
*Want "CGEBRA -5.5 M3 modes 1, 3"   FFFFFFFF FFFFFFFA FFFFFFFF FFFFFFFB
r 1380.10 #                          RNTE,             RZ
*Want "CGEBRA -5.5 M3 modes 4, 5"   FFFFFFFF FFFFFFFA FFFFFFFF FFFFFFFB
r 1390.10 #                          RP,               RM
*Want "CGEBRA -5.5 M3 modes 6, 7"   FFFFFFFF FFFFFFFB FFFFFFFF FFFFFFFA

r 13A0.10 #                          RZ,            RP
*Want "CGEBRA -2.5 FPCR modes 1, 2" FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFE
r 13B0.10 #                          RM,            RFS
*Want "CGEBRA -2.5 FPCR modes 3, 7" FFFFFFFF FFFFFFFD FFFFFFFF FFFFFFFD
r 13C0.10 #                        RNTA,            RFS
*Want "CGEBRA -2.5 M3 modes 1, 3"   FFFFFFFF FFFFFFFD FFFFFFFF FFFFFFFD
r 13D0.10 #                        RNTE,            RZ
*Want "CGEBRA -2.5 M3 modes 4, 5"   FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFE
r 13E0.10 #                        RP,              RM
*Want "CGEBRA -2.5 M3 modes 6, 7"   FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFD

r 13F0.10 #                          RZ,              RP
*Want "CGEBRA -1.5 FPCR modes 1, 2" FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 1400.10 #                          RM,              RFS
*Want "CGEBRA -1.5 FPCR modes 3, 7" FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFF
r 1410.10 #                           RNTA,             RFS
*Want "CGEBRA -1.5 M3 modes 1, 3"  FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFF
r 1420.10 #                           RNTE,             RZ
*Want "CGEBRA -1.5 M3 modes 4, 5"  FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFF
r 1430.10 #                           RP,               RM
*Want "CGEBRA -1.5 M3 modes 6, 7"  FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE

r 1440.10 #                          RZ,              RP
*Want "CGEBRA -0.5 FPCR modes 1, 2" 00000000 00000000 00000000 00000000
r 1450.10 #                          RM,              RFS
*Want "CGEBRA -0.5 FPCR modes 3, 7" FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 1460.10 #                           RNTA,             RFS
*Want "CGEBRA -0.5 M3 modes 1, 3"  FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 1470.10 #                           RNTE,             RZ
*Want "CGEBRA -0.5 M3 modes 4, 5"  00000000 00000000 00000000 00000000
r 1480.10 #                           RP,               RM
*Want "CGEBRA -0.5 M3 modes 6, 7"  00000000 00000000 FFFFFFFF FFFFFFFF

r 1490.10 #                          RZ,             RP
*Want "CGEBRA 0.5 FPCR modes 1, 2" 00000000 00000000 00000000 00000001
r 14A0.10 #                          RM,             RFS
*Want "CGEBRA 0.5 FPCR modes 3, 7" 00000000 00000000 00000000 00000001
r 14B0.10 #                          RNTA,           RFS
*Want "CGEBRA 0.5 M3 modes 1, 3"  00000000 00000001 00000000 00000001
r 14C0.10 #                          RNTE,           RZ
*Want "CGEBRA 0.5 M3 modes 4, 5"  00000000 00000000 00000000 00000000
r 14D0.10 #                          RP,             RM
*Want "CGEBRA 0.5 M3 modes 6, 7"  00000000 00000001 00000000 00000000

r 14E0.10 #                          RZ,           RP
*Want "CGEBRA 1.5 FPCR modes 1, 2" 00000000 00000001 00000000 00000002
r 14F0.10 #                          RM,           RFS
*Want "CGEBRA 1.5 FPCR modes 3, 7" 00000000 00000001 00000000 00000001
r 1500.10 #                        RNTA,           RFS
*Want "CGEBRA 1.5 M3 modes 1, 3"  00000000 00000002 00000000 00000001
r 1510.10 #                        RNTE,           RZ
*Want "CGEBRA 1.5 M3 modes 4, 5"  00000000 00000002 00000000 00000001
r 1520.10 #                        RP,             RM
*Want "CGEBRA 1.5 M3 modes 6, 7"  00000000 00000002 00000000 00000001

r 1530.10 #                          RZ,           RP
*Want "CGEBRA 2.5 FPCR modes 1, 2" 00000000 00000002 00000000 00000003
r 1540.10 #                          RM,           RFS
*Want "CGEBRA 2.5 FPCR modes 3, 7" 00000000 00000002 00000000 00000003
r 1550.10 #                        RNTA,           RFS
*Want "CGEBRA 2.5 M3 modes 1, 3"  00000000 00000003 00000000 00000003
r 1560.10 #                        RNTE,           RZ
*Want "CGEBRA 2.5 M3 modes 4, 5"  00000000 00000002 00000000 00000002
r 1570.10 #                        RP,             RM
*Want "CGEBRA 2.5 M3 modes 6, 7"  00000000 00000003 00000000 00000002

r 1580.10 #                          RZ,           RP
*Want "CGEBRA 5.5 FPCR modes 1, 2" 00000000 00000005 00000000 00000006
r 1590.10 #                          RM,           RFS
*Want "CGEBRA 5.5 FPCR modes 3, 7" 00000000 00000005 00000000 00000005
r 15A0.10 #                        RNTA,           RFS
*Want "CGEBRA 5.5 M3 modes 1, 3"  00000000 00000006 00000000 00000005
r 15B0.10 #                        RNTE,           RZ
*Want "CGEBRA 5.5 M3 modes 4, 5"  00000000 00000006 00000000 00000005
r 15C0.10 #                        RP,             RM
*Want "CGEBRA 5.5 M3 modes 6, 7"  00000000 00000006 00000000 00000005

r 15D0.10 #                          RZ,           RP
*Want "CGEBRA 9.5 FPCR modes 1, 2" 00000000 00000009 00000000 0000000A
r 15E0.10 #                          RM,           RFS
*Want "CGEBRA 9.5 FPCR modes 3, 7" 00000000 00000009 00000000 00000009
r 15F0.10 #                        RNTA,           RFS
*Want "CGEBRA 9.5 M3 modes 1, 3"  00000000 0000000A 00000000 00000009
r 1600.10 #                        RNTE,           RZ
*Want "CGEBRA 9.5 M3 modes 4, 5"  00000000 0000000A 00000000 00000009
r 1610.10 #                        RP,             RM
*Want "CGEBRA 9.5 M3 modes 6, 7"  00000000 0000000A 00000000 00000009

r 1620.10 #                           RZ,             RP
*Want "CGEBRA +0.75 FPCR modes 1, 2" 00000000 00000000 00000000 00000001
r 1630.10 #                           RM,             RFS
*Want "CGEBRA +0.75 FPCR modes 3, 7" 00000000 00000000 00000000 00000001
r 1640.10 #                           RNTA,           RFS
*Want "CGEBRA +0.75 M3 modes 1, 3"   00000000 00000001 00000000 00000001
r 1650.10 #                           RNTE,           RZ
*Want "CGEBRA +0.75 M3 modes 4, 5"   00000000 00000001 00000000 00000000
r 1660.10 #                           RP,             RM
*Want "CGEBRA +0.75 M3 modes 6, 7"   00000000 00000001 00000000 00000000

r 1670.10 #                           RZ,             RP
*Want "CGEBRA +0.25 FPCR modes 1, 2" 00000000 00000000 00000000 00000001
r 1680.10 #                           RM,             RFS
*Want "CGEBRA +0.25 FPCR modes 3, 7" 00000000 00000000 00000000 00000001
r 1690.10 #                           RNTA,           RFS
*Want "CGEBRA +0.25 M3 modes 1, 3"   00000000 00000000 00000000 00000001
r 16A0.10 #                           RNTE,           RZ
*Want "CGEBRA +0.25 M3 modes 4, 5"   00000000 00000000 00000000 00000000
r 16B0.10 #                           RP,             RM
*Want "CGEBRA +0.25 M3 modes 6, 7"   00000000 00000001 00000000 00000000

r 16C0.10 #                           RZ,             RP
*Want "CGEBRA -0.75 FPCR modes 1, 2" 00000000 00000000 00000000 00000000
r 16D0.10 #                           RM,             RFS
*Want "CGEBRA -0.75 FPCR modes 3, 7" FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 16E0.10 #                           RNTA,           RFS
*Want "CGEBRA -0.75 M3 modes 1, 3"   FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 16F0.10 #                           RNTE,           RZ
*Want "CGEBRA -0.75 M3 modes 4, 5"   FFFFFFFF FFFFFFFF 00000000 00000000
r 1700.10 #                           RP,             RM
*Want "CGEBRA -0.75 M3 modes 6, 7"   00000000 00000000 FFFFFFFF FFFFFFFF

r 1710.10 #                           RZ,             RP
*Want "CGEBRA -0.25 FPCR modes 1, 2" 00000000 00000000 00000000 00000000
r 1720.10 #                           RM,             RFS
*Want "CGEBRA -0.25 FPCR modes 3, 7" FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 1730.10 #                           RNTA,           RFS
*Want "CGEBRA -0.25 M3 modes 1, 3"   00000000 00000000 FFFFFFFF FFFFFFFF
r 1740.10 #                           RNTE,           RZ
*Want "CGEBRA -0.25 M3 modes 4, 5"   00000000 00000000 00000000 00000000
r 1750.10 #                           RP,             RM
*Want "CGEBRA -0.25 M3 modes 6, 7"   00000000 00000000 FFFFFFFF FFFFFFFF


#  rounding mode tests - short BFP - FPCR contents with cc in last byte
*Compare
r 1800.10  # Rounding Mode Tests - FPC
*Want "CGEBRA -9.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 1810.10  # Rounding Mode Tests - FPC
*Want "CGEBRA -9.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 1820.08  # Rounding Mode Tests - FPC
*Want "CGEBRA -9.5 M3 modes 6-7 FPCR"      00080001 00080001

r 1830.10
*Want "CGEBRA -5.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 1840.10
*Want "CGEBRA -5.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 1850.08
*Want "CGEBRA -5.5 M3 modes 6-7 FPCR"      00080001 00080001

r 1860.10
*Want "CGEBRA -2.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 1870.10
*Want "CGEBRA -2.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 1880.08
*Want "CGEBRA -2.5 M3 modes 6-7 FPCR"      00080001 00080001

r 1890.10
*Want "CGEBRA -1.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 18A0.10
*Want "CGEBRA -1.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 18B0.08
*Want "CGEBRA -1.5 M3 modes 6-7 FPCR"      00080001 00080001

r 18C0.10
*Want "CGEBRA -0.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 18D0.10
*Want "CGEBRA -0.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 18E0.08
*Want "CGEBRA -0.5 M3 modes 6-7 FPCR"      00080001 00080001

r 18F0.10
*Want "CGEBRA +0.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 1900.10
*Want "CGEBRA +0.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1910.08
*Want "CGEBRA +0.5 M3 modes 6-7 FPCR"      00080002 00080002

r 1920.10
*Want "CGEBRA +1.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 1930.10
*Want "CGEBRA +1.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1940.08
*Want "CGEBRA +1.5 M3 modes 6-7 FPCR"      00080002 00080002

r 1950.10
*Want "CGEBRA +2.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 1960.10
*Want "CGEBRA +2.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1970.08
*Want "CGEBRA +2.5 M3 modes 6-7 FPCR"      00080002 00080002

r 1980.10
*Want "CGEBRA +5.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 1990.10
*Want "CGEBRA +5.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 19A0.08
*Want "CGEBRA +5.5 M3 modes 6-7 FPCR"      00080002 00080002

r 19B0.10
*Want "CGEBRA +9.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 19C0.10
*Want "CGEBRA +9.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 19D0.08
*Want "CGEBRA +9.5 M3 modes 6-7 FPCR"      00080002 00080002

r 19E0.10
*Want "CGEBRA +0.75 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 19F0.10
*Want "CGEBRA +0.75 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1A00.08
*Want "CGEBRA +0.75 M3 modes 6-7 FPCR"      00080002 00080002

r 1A10.10
*Want "CGEBRA +0.25 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 1A20.10
*Want "CGEBRA +0.25 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1A30.08
*Want "CGEBRA +0.25 M3 modes 6-7 FPCR"      00080002 00080002

r 1A40.10
*Want "CGEBRA -0.75 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 1A50.10
*Want "CGEBRA -0.75 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 1A60.08
*Want "CGEBRA -0.75 M3 modes 6-7 FPCR"      00080001 00080001

r 1A70.10
*Want "CGEBRA -0.25 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 1A80.10
*Want "CGEBRA -0.25 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 18E0.08
*Want "CGEBRA -0.25 M3 modes 6-7 FPCR"      00080001 00080001


# Long BFP inputs to int-64 - results
*Compare
r 2000.10
*Want "CGDBR result pair 1" 00000000 00000001 00000000 00000001
r 2010.10
*Want "CGDBR result pair 2" 00000000 00000002 00000000 00000002
r 2020.10
*Want "CGDBR result pair 3" 00000000 00000004 00000000 00000004
r 2030.10
*Want "CGDBR result pair 4" FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFE
r 2040.10
*Want "CGDBR result pair 5" 80000000 00000000 00000000 00000000
r 2050.10
*Want "CGDBR result pair 6" 80000000 00000000 00000000 00000000
r 2060.10
*Want "CGDBR result pair 7" 7FFFFFFF FFFFFFFF 00000000 00000000
r 2070.10
*Want "CGDBR result pair 8" 80000000 00000000 00000000 00000000
r 2080.10
*Want "CGDBR result pair 8" 7FFFFFFF FFFFFC00 7FFFFFFF FFFFFC00

# Long BFP inputs to int-64 - FPCR contents
*Compare
r 2200.10
*Want "CGDBR FPCR pairs 1-2" 00000002 F8000002 00000002 F8000002
r 2210.10
*Want "CGDBR FPCR pairs 3-4" 00000002 F8000002 00000001 F8000001
r 2220.10
*Want "CGDBR FPCR pairs 5-6" 00880003 F8008000 00880003 F8008000
r 2230.10
*Want "CGDBR FPCR pairs 7-8" 00880003 F8008000 00880003 F8008000
r 2240.08
*Want "CGDBR FPCR pair 9"    00000002 F8000002


#  long BFP rounding mode tests - results 
*Compare
r 2300.10 #                          RZ,              RP
*Want "CGDBRA -9.5 FPCR modes 1, 2" FFFFFFFF FFFFFFF7 FFFFFFFF FFFFFFF7
r 2310.10 #                          RM,              RFS
*Want "CGDBRA -9.5 FPCR modes 3, 7" FFFFFFFF FFFFFFF6 FFFFFFFF FFFFFFF7
r 2320.10 #                         RNTA,             RFS
*Want "CGDBRA -9.5 M3 modes 1, 3"  FFFFFFFF FFFFFFF6 FFFFFFFF FFFFFFF7
r 2330.10 #                         RNTE,             RZ
*Want "CGDBRA -9.5 M3 modes 4, 5"  FFFFFFFF FFFFFFF6 FFFFFFFF FFFFFFF7
r 2340.10 #                         RP                RM
*Want "CGDBRA -9.5 M3 modes 6, 7"  FFFFFFFF FFFFFFF7 FFFFFFFF FFFFFFF6

r 2350.10 #                          RZ,              RP
*Want "CGDBRA -5.5 FPCR modes 1, 2" FFFFFFFF FFFFFFFB FFFFFFFF FFFFFFFB
r 2360.10 #                          RM,              RFS
*Want "CGDBRA -5.5 FPCR modes 3, 7" FFFFFFFF FFFFFFFA FFFFFFFF FFFFFFFB
r 2370.10 #                         RNTA,             RFS
*Want "CGDBRA -5.5 M3 modes 1, 3"  FFFFFFFF FFFFFFFA FFFFFFFF FFFFFFFB
r 2380.10 #                         RNTE,             RZ
*Want "CGDBRA -5.5 M3 modes 4, 5"  FFFFFFFF FFFFFFFA FFFFFFFF FFFFFFFB
r 2390.10 #                         RP,               RM
*Want "CGDBRA -5.5 M3 modes 6, 7"  FFFFFFFF FFFFFFFB FFFFFFFF FFFFFFFA

r 23A0.10 #                          RZ,               RP
*Want "CGDBRA -2.5 FPCR modes 1, 2" FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFE
r 23B0.10 #                          RM,               RFS
*Want "CGDBRA -2.5 FPCR modes 3, 7" FFFFFFFF FFFFFFFD FFFFFFFF FFFFFFFD
r 23C0.10 #                          RNTA,             RFS
*Want "CGDBRA -2.5 M3 modes 1, 3"   FFFFFFFF FFFFFFFD FFFFFFFF FFFFFFFD
r 23D0.10 #                          RNTE,             RZ
*Want "CGDBRA -2.5 M3 modes 4, 5"   FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFE
r 23E0.10 #                          RP,               RM
*Want "CGDBRA -2.5 M3 modes 6, 7"   FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFD

r 23F0.10 #                          RZ,               RP
*Want "CGDBRA -1.5 FPCR modes 1, 2" FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 2400.10 #                          RM,               RFS
*Want "CGDBRA -1.5 FPCR modes 3, 7" FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFF
r 2410.10 #                          RNTA,             RFS
*Want "CGDBRA -1.5 M3 modes 1, 3"   FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFF
r 2420.10 #                          RNTE,             RZ
*Want "CGDBRA -1.5 M3 modes 4, 5"   FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFF
r 2430.10 #                          RP,               RM
*Want "CGDBRA -1.5 M3 modes 6, 7"   FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE

r 2440.10 #                          RZ,               RP
*Want "CGDBRA -0.5 FPCR modes 1, 2" 00000000 00000000 00000000 00000000
r 2450.10 #                          RM,               RFS
*Want "CGDBRA -0.5 FPCR modes 3, 7" FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 2460.10 #                          RNTA,             RFS
*Want "CGDBRA -0.5 M3 modes 1, 3"   FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 2470.10 #                          RNTE,             RZ
*Want "CGDBRA -0.5 M3 modes 4, 5"   00000000 00000000 00000000 00000000
r 2480.10 #                          RP,               RM
*Want "CGDBRA -0.5 M3 modes 6, 7"   00000000 00000000 FFFFFFFF FFFFFFFF

r 2490.10 #                          RZ,               RP
*Want "CGDBRA 0.5 FPCR modes 1, 2"  00000000 00000000 00000000 00000001
r 24A0.10 #                          RM,               RFS
*Want "CGDBRA 0.5 FPCR modes 3, 7"  00000000 00000000 00000000 00000001
r 24B0.10 #                          RNTA,             RFS
*Want "CGDBRA 0.5 M3 modes 1, 3"    00000000 00000001 00000000 00000001
r 24C0.10 #                          RNTE,             RZ
*Want "CGDBRA 0.5 M3 modes 4, 5"    00000000 00000000 00000000 00000000
r 24D0.10 #                          RP,               RM
*Want "CGDBRA 0.5 M3 modes 6, 7"    00000000 00000001 00000000 00000000

r 24E0.10 #                          RZ,               RP
*Want "CGDBRA 1.5 FPCR modes 1, 2"  00000000 00000001 00000000 00000002
r 24F0.10 #                          RM,               RFS
*Want "CGDBRA 1.5 FPCR modes 3, 7"  00000000 00000001 00000000 00000001
r 2500.10 #                          RNTA,             RFS
*Want "CGDBRA 1.5 M3 modes 1, 3"    00000000 00000002 00000000 00000001
r 2510.10 #                          RNTE,             RZ
*Want "CGDBRA 1.5 M3 modes 4, 5"    00000000 00000002 00000000 00000001
r 2520.10 #                          RP,               RM
*Want "CGDBRA 1.5 M3 modes 6, 7"    00000000 00000002 00000000 00000001

r 2530.10 #                          RZ,               RP
*Want "CGDBRA 2.5 FPCR modes 1, 2"  00000000 00000002 00000000 00000003
r 2540.10 #                          RM,               RFS
*Want "CGDBRA 2.5 FPCR modes 3, 7"  00000000 00000002 00000000 00000003
r 2550.10 #                          RNTA,             RFS
*Want "CGDBRA 2.5 M3 modes 1, 3"    00000000 00000003 00000000 00000003
r 2560.10 #                          RNTE,             RZ
*Want "CGDBRA 2.5 M3 modes 4, 5"    00000000 00000002 00000000 00000002
r 2570.10 #                          RP,               RM
*Want "CGDBRA 2.5 M3 modes 6, 7"    00000000 00000003 00000000 00000002

r 2580.10 #                          RZ,               RP
*Want "CGDBRA 5.5 FPCR modes 1, 2"  00000000 00000005 00000000 00000006
r 2590.10 #                          RM,               RFS
*Want "CGDBRA 5.5 FPCR modes 3, 7"  00000000 00000005 00000000 00000005
r 25A0.10 #                          RNTA,             RFS
*Want "CGDBRA 5.5 M3 modes 1, 3"    00000000 00000006 00000000 00000005
r 25B0.10 #                          RNTE,             RZ
*Want "CGDBRA 5.5 M3 modes 4, 5"    00000000 00000006 00000000 00000005
r 25C0.10 #                          RP,               RM
*Want "CGDBRA 5.5 M3 modes 6, 7"    00000000 00000006 00000000 00000005

r 25D0.10 #                          RZ,               RP
*Want "CGDBRA 9.5 FPCR modes 1, 2"  00000000 00000009 00000000 0000000A
r 25E0.10 #                          RM,               RFS
*Want "CGDBRA 9.5 FPCR modes 3, 7"  00000000 00000009 00000000 00000009
r 25F0.10 #                          RNTA,             RFS
*Want "CGDBRA 9.5 M3 modes 1, 3"    00000000 0000000A 00000000 00000009
r 2600.10 #                          RNTE,             RZ
*Want "CGDBRA 9.5 M3 modes 4, 5"    00000000 0000000A 00000000 00000009
r 2610.10 #                          RP,               RM
*Want "CGDBRA 9.5 M3 modes 6, 7"    00000000 0000000A 00000000 00000009

r 2620.10 #                            RZ,               RP
*Want "CGDBRA +0.75 FPCR modes 1, 2"  00000000 00000000 00000000 00000001
r 2630.10 #                            RM,               RFS
*Want "CGDBRA +0.75 FPCR modes 3, 7"  00000000 00000000 00000000 00000001
r 2640.10 #                            RNTA,             RFS
*Want "CGDBRA +0.75 M3 modes 1, 3"    00000000 00000001 00000000 00000001
r 2650.10 #                            RNTE,             RZ
*Want "CGDBRA +0.75 M3 modes 4, 5"    00000000 00000001 00000000 00000000
r 2660.10 #                            RP,               RM
*Want "CGDBRA +0.75 M3 modes 6, 7"    00000000 00000001 00000000 00000000

r 2670.10 #                            RZ,               RP
*Want "CGDBRA +0.25 FPCR modes 1, 2"  00000000 00000000 00000000 00000001
r 2680.10 #                            RM,               RFS
*Want "CGDBRA +0.25 FPCR modes 3, 7"  00000000 00000000 00000000 00000001
r 2690.10 #                            RNTA,             RFS
*Want "CGDBRA +0.25 M3 modes 1, 3"    00000000 00000000 00000000 00000001
r 26A0.10 #                            RNTE,             RZ
*Want "CGDBRA +0.25 M3 modes 4, 5"    00000000 00000000 00000000 00000000
r 26B0.10 #                            RP,               RM
*Want "CGDBRA +0.25 M3 modes 6, 7"    00000000 00000001 00000000 00000000

r 26C0.10 #                          RZ,               RP
*Want "CGDBRA -0.75 FPCR modes 1, 2" 00000000 00000000 00000000 00000000
r 26D0.10 #                          RM,               RFS
*Want "CGDBRA -0.75 FPCR modes 3, 7" FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 26E0.10 #                          RNTA,             RFS
*Want "CGDBRA -0.75 M3 modes 1, 3"   FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 26F0.10 #                          RNTE,             RZ
*Want "CGDBRA -0.75 M3 modes 4, 5"   FFFFFFFF FFFFFFFF 00000000 00000000
r 2700.10 #                          RP,               RM
*Want "CGDBRA -0.75 M3 modes 6, 7"   00000000 00000000 FFFFFFFF FFFFFFFF

r 2710.10 #                          RZ,               RP
*Want "CGDBRA -0.25 FPCR modes 1, 2" 00000000 00000000 00000000 00000000
r 2720.10 #                          RM,               RFS
*Want "CGDBRA -0.25 FPCR modes 3, 7" FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 2730.10 #                          RNTA,             RFS
*Want "CGDBRA -0.25 M3 modes 1, 3"   00000000 00000000 FFFFFFFF FFFFFFFF
r 2740.10 #                          RNTE,             RZ
*Want "CGDBRA -0.25 M3 modes 4, 5"   00000000 00000000 00000000 00000000
r 2750.10 #                          RP,               RM
*Want "CGDBRA -0.25 M3 modes 6, 7"   00000000 00000000 FFFFFFFF FFFFFFFF


#  rounding mode tests - long BFP - FPCR contents with cc in last byte
*Compare
r 2800.10
*Want "CGDBRA -9.5 FPCR modes 1-3, 7 FPCR"  00000001 00000001 00000001 00000001
r 2810.10
*Want "CGDBRA -9.5 M3 modes 1, 3-5 FPCR"    00080001 00080001 00080001 00080001
r 2820.08
*Want "CGDBRA -9.5 M3 modes 6-7 FPCR"       00080001 00080001

r 2830.10
*Want "CGDBRA -5.5 FPCR modes 1-3, 7 FPCR"  00000001 00000001 00000001 00000001
r 2840.10
*Want "CGDBRA -5.5 M3 modes 1, 3-5 FPCR"    00080001 00080001 00080001 00080001
r 2850.08
*Want "CGDBRA -5.5 M3 modes 6-7 FPCR"       00080001 00080001

r 2860.10
*Want "CGDBRA -2.5 FPCR modes 1-3, 7 FPCR"  00000001 00000001 00000001 00000001
r 2870.10
*Want "CGDBRA -2.5 M3 modes 1, 3-5 FPCR"    00080001 00080001 00080001 00080001
r 2880.08
*Want "CGDBRA -2.5 M3 modes 6-7 FPCR"       00080001 00080001

r 2890.10
*Want "CGDBRA -1.5 FPCR modes 1-3, 7 FPCR"  00000001 00000001 00000001 00000001
r 28A0.10
*Want "CGDBRA -1.5 M3 modes 1, 3-5 FPCR"    00080001 00080001 00080001 00080001
r 28B0.08
*Want "CGDBRA -1.5 M3 modes 6-7 FPCR"       00080001 00080001

r 28C0.10
*Want "CGDBRA -0.5 FPCR modes 1-3, 7 FPCR"  00000001 00000001 00000001 00000001
r 28D0.10
*Want "CGDBRA -0.5 M3 modes 1, 3-5 FPCR"    00080001 00080001 00080001 00080001
r 28E0.08
*Want "CGDBRA -0.5 M3 modes 6-7 FPCR"       00080001 00080001

r 28F0.10
*Want "CGDBRA +0.5 FPCR modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 2900.10
*Want "CGDBRA +0.5 M3 modes 1, 3-5 FPCR"    00080002 00080002 00080002 00080002
r 2910.08
*Want "CGDBRA +0.5 M3 modes 6-7 FPCR"       00080002 00080002

r 2920.10
*Want "CGDBRA +1.5 FPCR modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 2930.10
*Want "CGDBRA +1.5 M3 modes 1, 3-5 FPCR"    00080002 00080002 00080002 00080002
r 2940.08
*Want "CGDBRA +1.5 M3 modes 6-7 FPCR"       00080002 00080002

r 2950.10
*Want "CGDBRA +2.5 FPCR modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 2960.10
*Want "CGDBRA +2.5 M3 modes 1, 3-5 FPCR"    00080002 00080002 00080002 00080002
r 2970.08
*Want "CGDBRA +2.5 M3 modes 6-7 FPCR"       00080002 00080002

r 2980.10
*Want "CGDBRA +5.5 FPCR modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 2990.10
*Want "CGDBRA +5.5 M3 modes 1, 3-5 FPCR"    00080002 00080002 00080002 00080002
r 29A0.08
*Want "CGDBRA +5.5 M3 modes 6-7 FPCR"       00080002 00080002

r 29B0.10
*Want "CGDBRA +9.5 FPCR modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 29C0.10
*Want "CGDBRA +9.5 M3 modes 1, 3-5 FPCR"    00080002 00080002 00080002 00080002
r 29D0.08
*Want "CGDBRA +9.5 M3 modes 6-7 FPCR"       00080002 00080002

r 29E0.10
*Want "CGDBRA +0.75 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 29F0.10
*Want "CGDBRA +0.75 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 2A00.08
*Want "CGDBRA +0.75 M3 modes 6-7 FPCR"      00080002 00080002

r 2A10.10
*Want "CGDBRA +0.25 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 2A20.10
*Want "CGDBRA +0.25 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 2A30.08
*Want "CGDBRA +0.25 M3 modes 6-7 FPCR"      00080002 00080002

r 2A40.10
*Want "CGDBRA -0.75 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 2A50.10
*Want "CGDBRA -0.75 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 2A60.08
*Want "CGDBRA -0.75 M3 modes 6-7 FPCR"      00080001 00080001

r 2A70.10
*Want "CGDBRA -0.25 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 2A80.10
*Want "CGDBRA -0.25 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 28E0.08
*Want "CGDBRA -0.25 M3 modes 6-7 FPCR"      00080001 00080001



# Extended BFP inputs to int-64 - results
*Compare
r 3000.10
*Want "CGXBR result pair 1" 00000000 00000001 00000000 00000001
r 3010.10
*Want "CGXBR result pair 2" 00000000 00000002 00000000 00000002
r 3020.10
*Want "CGXBR result pair 3" 00000000 00000004 00000000 00000004
r 3030.10
*Want "CGXBR result pair 4" FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFE
r 3040.10
*Want "CGXBR result pair 5" 80000000 00000000 00000000 00000000
r 3050.10
*Want "CGXBR result pair 6" 80000000 00000000 00000000 00000000
r 3060.10
*Want "CGXBR result pair 7" 7FFFFFFF FFFFFFFF 00000000 00000000
r 3070.10
*Want "CGXBR result pair 8" 80000000 00000000 00000000 00000000
r 3080.10
*Want "CGXBR result pair 8" 7FFFFFFF FFFFFFFF 7FFFFFFF FFFFFFFF

# Extended BFP inputs to int-64 - FPCR contents
*Compare
r 3200.10
*Want "CGXBR FPCR pairs 1-2" 00000002 F8000002 00000002 F8000002
r 3210.10
*Want "CGXBR FPCR pairs 3-4" 00000002 F8000002 00000001 F8000001
r 3220.10
*Want "CGXBR FPCR pairs 5-6" 00880003 F8008000 00880003 F8008000
r 3230.10
*Want "CGXBR FPCR pairs 7-8" 00880003 F8008000 00880003 F8008000
r 3240.08
*Want "CGXBR FPCR pair 9"    00000002 F8000002


#  rounding mode tests - extended BFP - results from rounding
*Compare
r 3300.10 #                          RZ,               RP
*Want "CGXBRA -9.5 FPCR modes 1, 2" FFFFFFFF FFFFFFF7 FFFFFFFF FFFFFFF7
r 3310.10 #                          RM,               RFS
*Want "CGXBRA -9.5 FPCR modes 3, 7" FFFFFFFF FFFFFFF6 FFFFFFFF FFFFFFF7
r 3320.10 #                          RNTA,             RFS
*Want "CGXBRA -9.5 M3 modes 1, 3"   FFFFFFFF FFFFFFF6 FFFFFFFF FFFFFFF7
r 3330.10 #                          RNTE,             RZ
*Want "CGXBRA -9.5 M3 modes 4, 5"   FFFFFFFF FFFFFFF6 FFFFFFFF FFFFFFF7
r 3340.10 #                          RP                RM
*Want "CGXBRA -9.5 M3 modes 6, 7"   FFFFFFFF FFFFFFF7 FFFFFFFF FFFFFFF6

r 3350.10 #                          RZ,               RP
*Want "CGXBRA -5.5 FPCR modes 1, 2" FFFFFFFF FFFFFFFB FFFFFFFF FFFFFFFB
r 3360.10 #                          RM,               RFS
*Want "CGXBRA -5.5 FPCR modes 3, 7" FFFFFFFF FFFFFFFA FFFFFFFF FFFFFFFB
r 3370.10 #                          RNTA,             RFS
*Want "CGXBRA -5.5 M3 modes 1, 3"   FFFFFFFF FFFFFFFA FFFFFFFF FFFFFFFB
r 3380.10 #                          RNTE,             RZ
*Want "CGXBRA -5.5 M3 modes 4, 5"   FFFFFFFF FFFFFFFA FFFFFFFF FFFFFFFB
r 3390.10 #                          RP,               RM
*Want "CGXBRA -5.5 M3 modes 6, 7"   FFFFFFFF FFFFFFFB FFFFFFFF FFFFFFFA

r 33A0.10 #                          RZ,               RP
*Want "CGXBRA -2.5 FPCR modes 1, 2" FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFE
r 33B0.10 #                          RM,               RFS
*Want "CGXBRA -2.5 FPCR modes 3, 7" FFFFFFFF FFFFFFFD FFFFFFFF FFFFFFFD
r 33C0.10 #                          RNTA,             RFS
*Want "CGXBRA -2.5 M3 modes 1, 3"   FFFFFFFF FFFFFFFD FFFFFFFF FFFFFFFD
r 33D0.10 #                          RNTE,             RZ
*Want "CGXBRA -2.5 M3 modes 4, 5"   FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFE
r 33E0.10 #                          RP,               RM
*Want "CGXBRA -2.5 M3 modes 6, 7"   FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFD

r 33F0.10 #                          RZ,               RP
*Want "CGXBRA -1.5 FPCR modes 1, 2" FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 3400.10 #                          RM,               RFS
*Want "CGXBRA -1.5 FPCR modes 3, 7" FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFF
r 3410.10 #                          RNTA,             RFS
*Want "CGXBRA -1.5 M3 modes 1, 3"   FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFF
r 3420.10 #                          RNTE,             RZ
*Want "CGXBRA -1.5 M3 modes 4, 5"   FFFFFFFF FFFFFFFE FFFFFFFF FFFFFFFF
r 3430.10 #                          RP,               RM
*Want "CGXBRA -1.5 M3 modes 6, 7"   FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE

r 3440.10 #                          RZ,               RP
*Want "CGXBRA -0.5 FPCR modes 1, 2" 00000000 00000000 00000000 00000000
r 3450.10 #                          RM,               RFS
*Want "CGXBRA -0.5 FPCR modes 3, 7" FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 3460.10 #                          RNTA,             RFS
*Want "CGXBRA -0.5 M3 modes 1, 3"   FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 3470.10 #                          RNTE,             RZ
*Want "CGXBRA -0.5 M3 modes 4, 5"   00000000 00000000 00000000 00000000
r 3480.10 #                          RP,               RM
*Want "CGXBRA -0.5 M3 modes 6, 7"   00000000 00000000 FFFFFFFF FFFFFFFF

r 3490.10 #                          RZ,               RP
*Want "CGXBRA +0.5 FPCR modes 1, 2" 00000000 00000000 00000000 00000001
r 34A0.10 #                          RM,               RFS
*Want "CGXBRA +0.5 FPCR modes 3, 7" 00000000 00000000 00000000 00000001
r 34B0.10 #                          RNTA,             RFS
*Want "CGXBRA +0.5 M3 modes 1, 3"   00000000 00000001 00000000 00000001
r 34C0.10 #                          RNTE,             RZ
*Want "CGXBRA +0.5 M3 modes 4, 5"   00000000 00000000 00000000 00000000
r 34D0.10 #                          RP,               RM
*Want "CGXBRA +0.5 M3 modes 6, 7"   00000000 00000001 00000000 00000000

r 34E0.10 #                           RZ,              RP
*Want "CGXBRA +1.5 FPCR modes 1, 2" 00000000 00000001 00000000 00000002
r 34F0.10 #                           RM,              RFS
*Want "CGXBRA +1.5 FPCR modes 3, 7" 00000000 00000001 00000000 00000001
r 3500.10 #                          RNTA,             RFS
*Want "CGXBRA +1.5 M3 modes 1, 3"   00000000 00000002 00000000 00000001
r 3510.10 #                          RNTE,             RZ
*Want "CGXBRA +1.5 M3 modes 4, 5"   00000000 00000002 00000000 00000001
r 3520.10 #                          RP,               RM
*Want "CGXBRA +1.5 M3 modes 6, 7"   00000000 00000002 00000000 00000001

r 3530.10 #                          RZ,               RP
*Want "CGXBRA +2.5 FPCR modes 1, 2" 00000000 00000002 00000000 00000003
r 3540.10 #                          RM,               RFS
*Want "CGXBRA +2.5 FPCR modes 3, 7" 00000000 00000002 00000000 00000003
r 3550.10 #                          RNTA,             RFS
*Want "CGXBRA +2.5 M3 modes 1, 3"   00000000 00000003 00000000 00000003
r 3560.10 #                          RNTE,             RZ
*Want "CGXBRA +2.5 M3 modes 4, 5"   00000000 00000002 00000000 00000002
r 3570.10 #                          RP,               RM
*Want "CGXBRA +2.5 M3 modes 6, 7"   00000000 00000003 00000000 00000002

r 3580.10 #                          RZ,               RP
*Want "CGXBRA +5.5 FPCR modes 1, 2" 00000000 00000005 00000000 00000006
r 3590.10 #                          RM,               RFS
*Want "CGXBRA +5.5 FPCR modes 3, 7" 00000000 00000005 00000000 00000005
r 35A0.10 #                          RNTA,             RFS
*Want "CGXBRA +5.5 M3 modes 1, 3"   00000000 00000006 00000000 00000005
r 35B0.10 #                          RNTE,             RZ
*Want "CGXBRA +5.5 M3 modes 4, 5"   00000000 00000006 00000000 00000005
r 35C0.10 #                          RP,               RM
*Want "CGXBRA +5.5 M3 modes 6, 7"   00000000 00000006 00000000 00000005

r 35D0.10 #                          RZ,               RP
*Want "CGXBRA +9.5 FPCR modes 1, 2" 00000000 00000009 00000000 0000000A
r 35E0.10 #                          RM,               RFS
*Want "CGXBRA +9.5 FPCR modes 3, 7" 00000000 00000009 00000000 00000009
r 35F0.10 #                          RNTA,             RFS
*Want "CGXBRA +9.5 M3 modes 1, 3"   00000000 0000000A 00000000 00000009
r 3600.10 #                          RNTE,             RZ
*Want "CGXBRA +9.5 M3 modes 4, 5"   00000000 0000000A 00000000 00000009
r 3610.10 #                          RP,               RM
*Want "CGXBRA +9.5 M3 modes 6, 7"   00000000 0000000A 00000000 00000009

r 3620.10 #                          RZ,               RP
*Want "CGXBRA +0.75 FPCR modes 1, 2" 00000000 00000000 00000000 00000001
r 3630.10 #                          RM,               RFS
*Want "CGXBRA +0.75 FPCR modes 3, 7" 00000000 00000000 00000000 00000001
r 3640.10 #                          RNTA,             RFS
*Want "CGXBRA +0.75 M3 modes 1, 3"   00000000 00000001 00000000 00000001
r 3650.10 #                          RNTE,             RZ
*Want "CGXBRA +0.75 M3 modes 4, 5"   00000000 00000001 00000000 00000000
r 3660.10 #                          RP,               RM
*Want "CGXBRA +0.75 M3 modes 6, 7"   00000000 00000001 00000000 00000000

r 3670.10 #                          RZ,               RP
*Want "CGXBRA +0.25 FPCR modes 1, 2" 00000000 00000000 00000000 00000001
r 3680.10 #                          RM,               RFS
*Want "CGXBRA +0.25 FPCR modes 3, 7" 00000000 00000000 00000000 00000001
r 3690.10 #                          RNTA,             RFS
*Want "CGXBRA +0.25 M3 modes 1, 3"   00000000 00000000 00000000 00000001
r 36A0.10 #                          RNTE,             RZ
*Want "CGXBRA +0.25 M3 modes 4, 5"   00000000 00000000 00000000 00000000
r 36B0.10 #                          RP,               RM
*Want "CGXBRA +0.25 M3 modes 6, 7"   00000000 00000001 00000000 00000000

r 36C0.10 #                          RZ,               RP
*Want "CGXBRA -0.75 FPCR modes 1, 2" 00000000 00000000 00000000 00000000
r 36D0.10 #                          RM,               RFS
*Want "CGXBRA -0.75 FPCR modes 3, 7" FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 36E0.10 #                          RNTA,             RFS
*Want "CGXBRA -0.75 M3 modes 1, 3"   FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 36F0.10 #                          RNTE,             RZ
*Want "CGXBRA -0.75 M3 modes 4, 5"   FFFFFFFF FFFFFFFF 00000000 00000000
r 3700.10 #                          RP,               RM
*Want "CGXBRA -0.75 M3 modes 6, 7"   00000000 00000000 FFFFFFFF FFFFFFFF

r 3710.10 #                          RZ,               RP
*Want "CGXBRA -0.25 FPCR modes 1, 2" 00000000 00000000 00000000 00000000
r 3720.10 #                          RM,               RFS
*Want "CGXBRA -0.25 FPCR modes 3, 7" FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF
r 3730.10 #                          RNTA,             RFS
*Want "CGXBRA -0.25 M3 modes 1, 3"   00000000 00000000 FFFFFFFF FFFFFFFF
r 3740.10 #                          RNTE,             RZ
*Want "CGXBRA -0.25 M3 modes 4, 5"   00000000 00000000 00000000 00000000
r 3750.10 #                          RP,               RM
*Want "CGXBRA -0.25 M3 modes 6, 7"   00000000 00000000 FFFFFFFF FFFFFFFF

r 3760.10 #                                RZ,               RP
*Want "CGXBRA maxint64+5 FPCR modes 1, 2" 7FFFFFFF FFFFFFFF 7FFFFFFF FFFFFFFF
r 3770.10 #                                RM,               RFS
*Want "CGXBRA maxint64+5 FPCR modes 3, 7" 7FFFFFFF FFFFFFFF 7FFFFFFF FFFFFFFF
r 3780.10 #                                RNTA,             RFS
*Want "CGXBRA maxint64+5 M3 modes 1, 3"   7FFFFFFF FFFFFFFF 7FFFFFFF FFFFFFFF
r 3790.10 #                                RNTE,             RZ
*Want "CGXBRA maxint64+5 M3 modes 4, 5"   7FFFFFFF FFFFFFFF 7FFFFFFF FFFFFFFF
r 37A0.10 #                                RP,               RM
*Want "CGXBRA maxint64+5 M3 modes 6, 7"   7FFFFFFF FFFFFFFF 7FFFFFFF FFFFFFFF


#  rounding mode tests - extended BFP - FPCR contents with cc in last byte
*Compare
r 3800.10
*Want "CGXBRA -9.5 FPCR modes 1-3, 7 FPCR"  00000001 00000001 00000001 00000001
r 3810.10
*Want "CGXBRA -9.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 3820.08
*Want "CGXBRA -9.5 M3 modes 6-7 FPCR"      00080001 00080001

r 3830.10
*Want "CGXBRA -5.5 FPCR modes 1-3, 7 FPCR"  00000001 00000001 00000001 00000001
r 3840.10
*Want "CGXBRA -5.5 M3 modes 1, 3-5 FPCR"    00080001 00080001 00080001 00080001
r 3850.08
*Want "CGXBRA -5.5 M3 modes 6-7 FPCR"       00080001 00080001
  
r 3860.10
*Want "CGXBRA -2.5 FPCR modes 1-3, 7 FPCR"  00000001 00000001 00000001 00000001
r 3870.10
*Want "CGXBRA -2.5 M3 modes 1, 3-5 FPCR"    00080001 00080001 00080001 00080001
r 3880.08
*Want "CGXBRA -2.5 M3 modes 6-7 FPCR"       00080001 00080001

r 3890.10
*Want "CGXBRA -1.5 FPCR modes 1-3, 7 FPCR"  00000001 00000001 00000001 00000001
r 38A0.10
*Want "CGXBRA -1.5 M3 modes 1, 3-5 FPCR"    00080001 00080001 00080001 00080001
r 38B0.08
*Want "CGXBRA -1.5 M3 modes 6-7 FPCR"       00080001 00080001

r 38C0.10
*Want "CGXBRA -0.5 FPCR modes 1-3, 7 FPCR"  00000001 00000001 00000001 00000001
r 38D0.10
*Want "CGXBRA -0.5 M3 modes 1, 3-5 FPCR"    00080001 00080001 00080001 00080001
r 38E0.08
*Want "CGXBRA -0.5 M3 modes 6-7 FPCR"       00080001 00080001

r 38F0.10
*Want "CGXBRA +0.5 FPCR modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 3900.10
*Want "CGXBRA +0.5 M3 modes 1, 3-5 FPCR"    00080002 00080002 00080002 00080002
r 3910.08
*Want "CGXBRA +0.5 M3 modes 6-7 FPCR"       00080002 00080002

r 3920.10
*Want "CGXBRA +1.5 FPCR modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 3930.10
*Want "CGXBRA +1.5 M3 modes 1, 3-5 FPCR"    00080002 00080002 00080002 00080002
r 3940.08
*Want "CGXBRA +1.5 M3 modes 6-7 FPCR"       00080002 00080002

r 3950.10
*Want "CGXBRA +2.5 FPCR modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 3960.10
*Want "CGXBRA +2.5 M3 modes 1, 3-5 FPCR"    00080002 00080002 00080002 00080002
r 3970.08
*Want "CGXBRA +2.5 M3 modes 6-7 FPCR"       00080002 00080002

r 3980.10
*Want "CGXBRA +5.5 FPCR modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 3990.10
*Want "CGXBRA +5.5 M3 modes 1, 3-5 FPCR"    00080002 00080002 00080002 00080002
r 39A0.08
*Want "CGXBRA +5.5 M3 modes 6-7 FPCR"       00080002 00080002

r 39B0.10
*Want "CGXBRA +9.5 FPCR modes 1-3, 7 FPCR"  00000002 00000002 00000002 00000002
r 39C0.10
*Want "CGXBRA +9.5 M3 modes 1, 3-5 FPCR"    00080002 00080002 00080002 00080002
r 39D0.08
*Want "CGXBRA +9.5 M3 modes 6-7 FPCR"       00080002 00080002

r 39E0.10
*Want "CGDBRA +0.75 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 39F0.10
*Want "CGDBRA +0.75 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 3A00.08
*Want "CGDBRA +0.75 M3 modes 6-7 FPCR"      00080002 00080002

r 3A10.10
*Want "CGDBRA +0.25 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 3A20.10
*Want "CGDBRA +0.25 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 3A30.08
*Want "CGDBRA +0.25 M3 modes 6-7 FPCR"      00080002 00080002

r 3A40.10
*Want "CGDBRA -0.75 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 3A50.10
*Want "CGDBRA -0.75 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 3A60.08
*Want "CGDBRA -0.75 M3 modes 6-7 FPCR"      00080001 00080001

r 3A70.10
*Want "CGDBRA -0.25 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 3A80.10
*Want "CGDBRA -0.25 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 3A90.08
*Want "CGDBRA -0.25 M3 modes 6-7 FPCR"      00080001 00080001

r 3AA0.10
*Want "CGDBRA maxint64+5 FPCR modes 1-3, 7 FPCR" 00000002 00800003 00000002 00000002
r 3AB0.10
*Want "CGDBRA maxint64+5 M3 modes 1, 3-5 FPCR"   00880003 00080002 00880003 00080002
r 3AC0.08
*Want "CGDBRA maxint64+5 M3 modes 6-7 FPCR"      00880003 00080002


*Done

