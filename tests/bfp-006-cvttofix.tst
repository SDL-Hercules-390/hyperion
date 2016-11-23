*Testcase bfp-006-cvttofix.tst:   CFEBR, CFEBRA, CFDBR, CFDBRA, CFXBR, CFXBRA

#Testcase bfp-006-cvttofix.tst: IEEE Convert To Fixed
#..Includes CONVERT TO FIXED 32 (6).  Tests traps, exceptions,
#..results from different rounding modes, and NaN propagation.

sysclear
archmode esame

#
# Following suppresses logging of program checks.  This test program, as part
# of its normal operation, generates 16 program check messages that have no
# value in the validation process.  (The messages, not the program checks.)
#
ostailor quiet

loadcore "$(testpath)/bfp-006-cvttofix.core"

runtest 1.0

ostailor null   # restore messages for subsequent tests

# Convert short BFP to integer-32 results
*Compare
r 1000.10
*Want "CFEBR result pairs 1-2" 00000001 00000001 00000002 00000002
r 1010.10
*Want "CFEBR result pairs 3-4" 00000004 00000004 FFFFFFFE FFFFFFFE
r 1020.10
*Want "CFEBR result pairs 5-6" 80000000 00000000 80000000 00000000
r 1030.10
*Want "CFEBR result pairs 7-8" 7FFFFFFF 00000000 80000000 00000000
r 1040.08
*Want "CFEBR result pair 9"    7FFFFF80 7FFFFF80

# Convert short BFP to integer-32 FPCR contents
*Compare
r 1100.10
*Want "CFEBR FPCR pairs 1-2" 00000002 F8000002 00000002 F8000002
r 1110.10
*Want "CFEBR FPCR pairs 3-4" 00000002 F8000002 00000001 F8000001
r 1120.10
*Want "CFEBR FPCR pairs 5-6" 00880003 F8008000 00880003 F8008000
r 1130.10
*Want "CFEBR FPCR pairs 7-8" 00880003 F8008000 00880003 F8008000
r 1140.08
*Want "CFEBR FPCR pair 9"    00000002 F8000002

#  Short BFP rounding mode tests - results from rounding
*Compare
r 1200.10  #                            RZ,     RP,       RM,      RFS
*Want "CFEBRA -9.5 FPCR modes 1-3, 7" FFFFFFF7 FFFFFFF7 FFFFFFF6 FFFFFFF7
r 1210.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA -9.5 M3 modes 1, 3-5"   FFFFFFF6 FFFFFFF7 FFFFFFF6 FFFFFFF7
r 1220.08  #                            RP,      RM
*Want "CFEBRA -9.5 M3 modes 6, 7"     FFFFFFF7 FFFFFFF6

r 1230.10  #                            RZ,      RP,      RM,      RFS
*Want "CFEBRA -5.5 FPCR modes 1-3, 7" FFFFFFFB FFFFFFFB FFFFFFFA FFFFFFFB
r 1240.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA -5.5 M3 modes 1, 3-5"   FFFFFFFA FFFFFFFB FFFFFFFA FFFFFFFB
r 1250.08  #                            RP,      RM
*Want "CFEBRA -5.5 M3 modes 6, 7"     FFFFFFFB FFFFFFFA

r 1260.10  #                            RZ,      RP,      RM,      RFS
*Want "CFEBRA -2.5 FPCR modes 1-3, 7" FFFFFFFE FFFFFFFE FFFFFFFD FFFFFFFD
r 1270.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA -2.5 M3 modes 1, 3-5"   FFFFFFFD FFFFFFFD FFFFFFFE FFFFFFFE
r 1280.08  #                            RP,      RM
*Want "CFEBRA -2.5 M3 modes 6, 7"     FFFFFFFE FFFFFFFD

r 1290.10  #                            RZ,      RP,      RM,      RFS
*Want "CFEBRA -1.5 FPCR modes 1-3, 7" FFFFFFFF FFFFFFFF FFFFFFFE FFFFFFFF
r 12A0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA -1.5 M3 modes 1, 3-5"   FFFFFFFE FFFFFFFF FFFFFFFE FFFFFFFF
r 12B0.08  #                            RP,      RM
*Want "CFEBRA -1.5 M3 modes 6, 7"     FFFFFFFF FFFFFFFE

r 12C0.10  #                            RZ,      RP,      RM,      RFS
*Want "CFEBRA -0.5 FPCR modes 1-3, 7" 00000000 00000000 FFFFFFFF FFFFFFFF
r 12D0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA -0.5 M3 modes 1, 3-5"   FFFFFFFF FFFFFFFF 00000000 00000000
r 12E0.08  #                            RP,      RM
*Want "CFEBRA -0.5 M3 modes 6, 7"     00000000 FFFFFFFF

r 12F0.10  #                            RZ,      RP,      RM,      RFS
*Want "CFEBRA +0.5 FPCR modes 1-3, 7" 00000000 00000001 00000000 00000001
r 1300.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA +0.5 M3 modes 1, 3-5"   00000001 00000001 00000000 00000000
r 1310.08  #                            RP,      RM
*Want "CFEBRA +0.5 M3 modes 6, 7"     00000001 00000000

r 1320.10  #                            RZ,     RP,      RM,      RFS
*Want "CFEBRA +1.5 FPCR modes 1-3, 7" 00000001 00000002 00000001 00000001
r 1330.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA +1.5 M3 modes 1, 3-5"   00000002 00000001 00000002 00000001
r 1340.08  #                            RP,      RM
*Want "CFEBRA +1.5 M3 modes 6, 7"     00000002 00000001

r 1350.10  #                            RZ,      RP,      RM,      RFS
*Want "CFEBRA +2.5 FPCR modes 1-3, 7" 00000002 00000003 00000002 00000003
r 1360.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA +2.5 M3 modes 1, 3-5"   00000003 00000003 00000002 00000002
r 1370.08  #                            RP,      RM
*Want "CFEBRA +2.5 M3 modes 6, 7"     00000003 00000002

r 1380.10  #                            RZ,      RP,      RM,      RFS
*Want "CFEBRA +5.5 FPCR modes 1-3, 7" 00000005 00000006 00000005 00000005
r 1390.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA +5.5 M3 modes 1, 3-5"   00000006 00000005 00000006 00000005
r 13A0.08  #                            RP,      RM
*Want "CFEBRA +5.5 M3 modes 6, 7"     00000006 00000005

r 13B0.10  #                            RZ,      RP,      RM,      RFS
*Want "CFEBRA +9.5 FPCR modes 1-3, 7" 00000009 0000000A 00000009 00000009
r 13C0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA +9.5 M3 modes 1, 3-5"   0000000A 00000009 0000000A 00000009
r 13D0.08  #                            RP,      RM
*Want "CFEBRA +9.5 M3 modes 6, 7"     0000000A 00000009

r 13E0.10  #                            RZ,      RP,      RM,      RFS
*Want "CFEBRA +0.75 FPCR modes 1-3, 7" 00000000 00000001 00000000 00000001
r 13F0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA +0.75 M3 modes 1, 3-5"   00000001 00000001 00000001 00000000
r 1400.08  #                            RP,      RM
*Want "CFEBRA +0.75 M3 modes 6, 7"     00000001 00000000

r 1410.10  #                            RZ,      RP,      RM,      RFS
*Want "CFEBRA +0.25 FPCR modes 1-3, 7" 00000000 00000001 00000000 00000001
r 1420.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA +0.25 M3 modes 1, 3-5"   00000000 00000001 00000000 00000000
r 1430.08  #                            RP,      RM
*Want "CFEBRA +0.25 M3 modes 6, 7"     00000001 00000000

r 1440.10  #                            RZ,      RP,      RM,      RFS
*Want "CFEBRA -0.75 FPCR modes 1-3, 7" 00000000 00000000 FFFFFFFF FFFFFFFF
r 1450.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA -0.75 M3 modes 1, 3-5"   FFFFFFFF FFFFFFFF FFFFFFFF 00000000
r 1460.08  #                            RP,      RM
*Want "CFEBRA -0.75 M3 modes 6, 7"     00000000 FFFFFFFF

r 1470.10  #                            RZ,      RP,      RM,      RFS
*Want "CFEBRA -0.25 FPCR modes 1-3, 7" 00000000 00000000 FFFFFFFF FFFFFFFF
r 1480.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFEBRA -0.25 M3 modes 1, 3-5"   00000000 FFFFFFFF 00000000 00000000
r 1490.08  #                            RP,      RM
*Want "CFEBRA -0.25 M3 modes 6, 7"     00000000 FFFFFFFF


#  rounding mode tests - short BFP - FPCR contents with cc in last byte
*Compare
r 1600.10
*Want "CFEBRA -9.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 1610.10
*Want "CFEBRA -9.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 1620.08
*Want "CFEBRA -9.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 1630.10
*Want "CFEBRA -5.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 1640.10
*Want "CFEBRA -5.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 1650.08
*Want "CFEBRA -5.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 1660.10
*Want "CFEBRA -2.5 FPCR modes 1-3, 7 FPCR"  00000001 00000001 00000001 00000001
r 1670.10
*Want "CFEBRA -2.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 1680.08
*Want "CFEBRA -2.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 1690.10
*Want "CFEBRA -1.5 FPCR modes 1-3, 7 FPCR"  00000001 00000001 00000001 00000001
r 16A0.10
*Want "CFEBRA -1.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 16B0.08
*Want "CFEBRA -1.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 16C0.10
*Want "CFEBRA -0.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 16D0.10
*Want "CFEBRA -0.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 16E0.08
*Want "CFEBRA -0.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 16F0.10
*Want "CFEBRA +0.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 1700.10
*Want "CFEBRA +0.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1710.08
*Want "CFEBRA +0.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 1720.10
*Want "CFEBRA +1.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 1730.10
*Want "CFEBRA +1.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1740.08
*Want "CFEBRA +1.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 1750.10
*Want "CFEBRA +2.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 1760.10
*Want "CFEBRA +2.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1770.08
*Want "CFEBRA +2.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 1780.10
*Want "CFEBRA +5.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 1790.10
*Want "CFEBRA +5.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 17A0.08
*Want "CFEBRA +5.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 17B0.10
*Want "CFEBRA +9.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 17C0.10
*Want "CFEBRA +9.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 17D0.08
*Want "CFEBRA +9.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 17E0.10
*Want "CFEBRA +0.75 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 17F0.10
*Want "CFEBRA +0.75 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1800.08
*Want "CFEBRA +0.75 M3 modes 6, 7 FPCR"     00080002 00080002

r 1810.10
*Want "CFEBRA +0.25 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 1820.10
*Want "CFEBRA +0.25 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 1830.08
*Want "CFEBRA +0.25 M3 modes 6, 7 FPCR"     00080002 00080002

r 1840.10
*Want "CFEBRA -0.75 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 1850.10
*Want "CFEBRA -0.75 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 1860.08
*Want "CFEBRA -0.75 M3 modes 6, 7 FPCR"     00080001 00080001

r 1870.10
*Want "CFEBRA -0.25 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 1880.10
*Want "CFEBRA -0.25 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 1890.08
*Want "CFEBRA -0.25 M3 modes 6, 7 FPCR"     00080001 00080001


# Long BFP converted to integer-32 - results
*Compare
r 2000.10
*Want "CFDBR result pairs 1-2"  00000001 00000001 00000002 00000002
r 2010.10
*Want "CFDBR result pairs 3-4"  00000004 00000004 FFFFFFFE FFFFFFFE
r 2020.10
*Want "CFDBR result pairs 5-6"  80000000 00000000 80000000 00000000
r 2030.10
*Want "CFDBR result pairs 7-8"  7FFFFFFF 00000000 80000000 00000000
r 2040.10
*Want "CFDBR result pairs 9-10" 7FFFFFFF 7FFFFFFF 7FFFFFFF 00000000

# Long BFP converted to integer-32 - FPCR contents
*Compare
r 2100.10
*Want "CFDBR FPCR pairs 1-2"  00000002 F8000002 00000002 F8000002
r 2110.10
*Want "CFDBR FPCR pairs 3-4"  00000002 F8000002 00000001 F8000001
r 2120.10
*Want "CFDBR FPCR pairs 5-6"  00880003 F8008000 00880003 F8008000
r 2130.10
*Want "CFDBR FPCR pairs 7-8"  00880003 F8008000 00880003 F8008000
r 2140.10
*Want "CFDBR FPCR pairs 9-10" 00000002 F8000002 00880003 F8008000


#  rounding mode tests - long BFP - results from rounding
*Compare
r 2200.10  #                            RZ,      RP,       RM,      RFS
*Want "CFDBRA -9.5 FPCR modes 1-3, 7" FFFFFFF7 FFFFFFF7 FFFFFFF6 FFFFFFF7
r 2210.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA -9.5 M3 modes 1, 3-5"   FFFFFFF6 FFFFFFF7 FFFFFFF6 FFFFFFF7
r 2220.08  #                            RP,      RM
*Want "CFDBRA -9.5 M3 modes 6, 7"     FFFFFFF7 FFFFFFF6

r 2230.10  #                            RZ,      RP,      RM,      RFS
*Want "CFDBRA -5.5 FPCR modes 1-3, 7" FFFFFFFB FFFFFFFB FFFFFFFA FFFFFFFB
r 2240.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA -5.5 M3 modes 1, 3-5"   FFFFFFFA FFFFFFFB FFFFFFFA FFFFFFFB
r 2250.08  #                            RP,      RM
*Want "CFDBRA -5.5 M3 modes 6, 7"     FFFFFFFB FFFFFFFA

r 2260.10  #                            RZ,      RP,      RM,      RFS
*Want "CFDBRA -2.5 FPCR modes 1-3, 7" FFFFFFFE FFFFFFFE FFFFFFFD FFFFFFFD
r 2270.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA -2.5 M3 modes 1, 3-5"   FFFFFFFD FFFFFFFD FFFFFFFE FFFFFFFE
r 2280.08  #                            RP,      RM
*Want "CFDBRA -2.5 M3 modes 6, 7"     FFFFFFFE FFFFFFFD

r 2290.10  #                            RZ,      RP,      RM,      RFS
*Want "CFDBRA -1.5 FPCR modes 1-3, 7" FFFFFFFF FFFFFFFF FFFFFFFE FFFFFFFF
r 22A0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA -1.5 M3 modes 1, 3-5"   FFFFFFFE FFFFFFFF FFFFFFFE FFFFFFFF
r 22B0.08  #                            RP,      RM
*Want "CFDBRA -1.5 M3 modes 6, 7"     FFFFFFFF FFFFFFFE

r 22C0.10  #                            RZ,      RP,      RM,      RFS
*Want "CFDBRA -0.5 FPCR modes 1-3, 7" 00000000 00000000 FFFFFFFF FFFFFFFF
r 22D0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA -0.5 M3 modes 1, 3-5"   FFFFFFFF FFFFFFFF 00000000 00000000
r 22E0.08  #                            RP,      RM
*Want "CFDBRA -0.5 M3 modes 6, 7"     00000000 FFFFFFFF

r 22F0.10  #                            RZ,      RP,      RM,      RFS
*Want "CFDBRA +0.5 FPCR modes 1-3, 7" 00000000 00000001 00000000 00000001
r 2300.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA +0.5 M3 modes 1, 3-5"   00000001 00000001 00000000 00000000
r 2310.08  #                            RP,      RM
*Want "CFDBRA +0.5 M3 modes 6, 7"     00000001 00000000

r 2320.10  #                            RZ,      RP,      RM,      RFS
*Want "CFDBRA +1.5 FPCR modes 1-3, 7" 00000001 00000002 00000001 00000001
r 2330.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA +1.5 M3 modes 1, 3-5"   00000002 00000001 00000002 00000001
r 2340.08  #                            RP,      RM
*Want "CFDBRA +1.5 M3 modes 6, 7"     00000002 00000001

r 2350.10  #                            RZ,      RP,      RM,      RFS
*Want "CFDBRA +2.5 FPCR modes 1-3, 7" 00000002 00000003 00000002 00000003
r 2360.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA +2.5 M3 modes 1, 3-5"   00000003 00000003 00000002 00000002
r 2370.08  #                            RP,      RM
*Want "CFDBRA +2.5 M3 modes 6, 7"     00000003 00000002

r 2380.10  #                            RZ,      RP,      RM,      RFS
*Want "CFDBRA +5.5 FPCR modes 1-3, 7" 00000005 00000006 00000005 00000005
r 2390.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA +5.5 M3 modes 1, 3-5"   00000006 00000005 00000006 00000005
r 23A0.08  #                            RP,      RM
*Want "CFDBRA +5.5 M3 modes 6, 7"     00000006 00000005

r 23B0.10  #                            RZ,      RP,      RM,      RFS
*Want "CFDBRA +9.5 FPCR modes 1-3, 7" 00000009 0000000A 00000009 00000009
r 23C0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA +9.5 M3 modes 1, 3-5"   0000000A 00000009 0000000A 00000009
r 23D0.08  #                            RP,      RM
*Want "CFDBRA +9.5 M3 modes 6, 7"     0000000A 00000009

r 23E0.10  #                            RZ,      RP,      RM,      RFS
*Want "CFDBRA +0.75 FPCR modes 1-3, 7" 00000000 00000001 00000000 00000001
r 23F0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA +0.75 M3 modes 1, 3-5"   00000001 00000001 00000001 00000000
r 2400.08  #                            RP,      RM
*Want "CFDBRA +0.75 M3 modes 6, 7"     00000001 00000000

r 2410.10  #                            RZ,      RP,      RM,      RFS
*Want "CFDBRA +0.25 FPCR modes 1-3, 7" 00000000 00000001 00000000 00000001
r 2420.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA +0.25 M3 modes 1, 3-5"   00000000 00000001 00000000 00000000
r 2430.08  #                            RP,      RM
*Want "CFDBRA +0.25 M3 modes 6, 7"     00000001 00000000

r 2440.10  #                            RZ,      RP,      RM,      RFS
*Want "CFDBRA -0.75 FPCR modes 1-3, 7" 00000000 00000000 FFFFFFFF FFFFFFFF
r 2450.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA -0.75 M3 modes 1, 3-5"   FFFFFFFF FFFFFFFF FFFFFFFF 00000000
r 2460.08  #                            RP,      RM
*Want "CFDBRA -0.75 M3 modes 6, 7"     00000000 FFFFFFFF

r 2470.10  #                            RZ,      RP,      RM,      RFS
*Want "CFDBRA -0.25 FPCR modes 1-3, 7" 00000000 00000000 FFFFFFFF FFFFFFFF
r 2480.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA -0.25 M3 modes 1, 3-5"   00000000 FFFFFFFF 00000000 00000000
r 2490.08  #                            RP,      RM
*Want "CFDBRA -0.25 M3 modes 6, 7"     00000000 FFFFFFFF

r 24A0.10  #                               RZ,      RP,      RM,      RFS
*Want "CFDBRA max+0.5 FPCR modes 1-3, 7" 7FFFFFFF 7FFFFFFF 7FFFFFFF 7FFFFFFF
r 24B0.10  #                               RNTA,    RFS,     RNTE,    RZ
*Want "CFDBRA max+0.5 M3 modes 1, 3-5"   7FFFFFFF 7FFFFFFF 7FFFFFFF 7FFFFFFF
r 24C0.08  #                               RP,      RM
*Want "CFDBRA max+0.5 M3 modes 6, 7"     7FFFFFFF 7FFFFFFF



#  rounding mode tests - long BFP - FPCR contents with cc in last byte
*Compare
r 2600.10
*Want "CFDBRA -9.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 2610.10
*Want "CFDBRA -9.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 2620.08
*Want "CFDBRA -9.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 2630.10
*Want "CFDBRA -5.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 2640.10
*Want "CFDBRA -5.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 2650.08
*Want "CFDBRA -5.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 2660.10
*Want "CFDBRA -2.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 2670.10
*Want "CFDBRA -2.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 2680.08
*Want "CFDBRA -2.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 2690.10
*Want "CFDBRA -1.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 26A0.10
*Want "CFDBRA -1.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 26B0.08
*Want "CFDBRA -1.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 26C0.10
*Want "CFDBRA -0.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 26D0.10
*Want "CFDBRA -0.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 26E0.08
*Want "CFDBRA -0.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 26F0.10
*Want "CFDBRA +0.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 2700.10
*Want "CFDBRA +0.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 2710.08
*Want "CFDBRA +0.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 2720.10
*Want "CFDBRA +1.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 2730.10
*Want "CFDBRA +1.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 2740.08
*Want "CFDBRA +1.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 2750.10
*Want "CFDBRA +2.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 2760.10
*Want "CFDBRA +2.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 2770.08
*Want "CFDBRA +2.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 2780.10
*Want "CFDBRA +5.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 2790.10
*Want "CFDBRA +5.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 27A0.08
*Want "CFDBRA +5.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 27B0.10
*Want "CFDBRA +9.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 27C0.10
*Want "CFDBRA +9.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 27D0.08
*Want "CFDBRA +9.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 27E0.10
*Want "CFDBRA +0.75 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 27F0.10
*Want "CFDBRA +0.75 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 2800.08
*Want "CFDBRA +0.75 M3 modes 6, 7 FPCR"     00080002 00080002

r 2810.10
*Want "CFDBRA +0.25 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 2820.10
*Want "CFDBRA +0.25 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 2830.08
*Want "CFDBRA +0.25 M3 modes 6, 7 FPCR"     00080002 00080002

r 2840.10
*Want "CFDBRA -0.75 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 2850.10
*Want "CFDBRA -0.75 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 2860.08
*Want "CFDBRA -0.75 M3 modes 6, 7 FPCR"     00080001 00080001

r 2870.10
*Want "CFDBRA -0.25 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 2880.10
*Want "CFDBRA -0.25 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 2890.08
*Want "CFDBRA -0.25 M3 modes 6, 7 FPCR"     00080001 00080001

r 28A0.10
*Want "CFDBRA max+0.5 FPCR modes 1-3, 7 FPCR" 00000002 00800003 00000002 00000002
r 28B0.10
*Want "CFDBRA max+0.5 M3 modes 1, 3-5 FPCR"   00880003 00080002 00880003 00080002
r 28C0.08
*Want "CFDBRA max+0.5 M3 modes 6, 7 FPCR"     00880003 00080002



# Extended BFP to integer-32 - results
*Compare
r 3000.10
*Want "CFXBR result pairs 1-2"  00000001 00000001 00000002 00000002
r 3010.10
*Want "CFXBR result pairs 3-4"  00000004 00000004 FFFFFFFE FFFFFFFE
r 3020.10
*Want "CFXBR result pairs 5-6"  80000000 00000000 80000000 00000000
r 2030.10
*Want "CFXBR result pairs 7-8"  7FFFFFFF 00000000 80000000 00000000
r 2040.10
*Want "CFXBR result pairs 9-10" 7FFFFFFF 7FFFFFFF 7FFFFFFF 00000000

# Extended BFP to integer-32 - FPCR contents
*Compare
r 3100.10
*Want "CFXBR FPCR pairs 1-2"  00000002 F8000002 00000002 F8000002
r 3110.10
*Want "CFXBR FPCR pairs 3-4"  00000002 F8000002 00000001 F8000001
r 3120.10
*Want "CFXBR FPCR pairs 5-6"  00880003 F8008000 00880003 F8008000
r 3130.10
*Want "CFXBR FPCR pairs 7-8"  00880003 F8008000 00880003 F8008000
r 3140.10
*Want "CFXBR FPCR pairs 9-10" 00000002 F8000002 00880003 F8008000


#  rounding mode tests - extended BFP - results from rounding
*Compare
r 3200.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA -9.5 FPCR modes 1-3, 7" FFFFFFF7 FFFFFFF7 FFFFFFF6 FFFFFFF7
r 3210.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA -9.5 M3 modes 1, 3-5"   FFFFFFF6 FFFFFFF7 FFFFFFF6 FFFFFFF7
r 3220.08  #                            RP,      RM
*Want "CFXBRA -9.5 M3 modes 6, 7"     FFFFFFF7 FFFFFFF6

r 3230.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA -5.5 FPCR modes 1-3, 7" FFFFFFFB FFFFFFFB FFFFFFFA FFFFFFFB
r 3240.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA -5.5 M3 modes 1, 3-5"   FFFFFFFA FFFFFFFB FFFFFFFA FFFFFFFB
r 3250.08  #                            RP,      RM
*Want "CFXBRA -5.5 M3 modes 6, 7"     FFFFFFFB FFFFFFFA

r 3260.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA -2.5 FPCR modes 1-3, 7" FFFFFFFE FFFFFFFE FFFFFFFD FFFFFFFD
r 3270.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA -2.5 M3 modes 1, 3-5"   FFFFFFFD FFFFFFFD FFFFFFFE FFFFFFFE
r 3280.08  #                            RP,      RM
*Want "CFXBRA -2.5 M3 modes 6, 7"     FFFFFFFE FFFFFFFD

r 3290.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA -1.5 FPCR modes 1-3, 7" FFFFFFFF FFFFFFFF FFFFFFFE FFFFFFFF
r 32A0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA -1.5 M3 modes 1, 3-5"   FFFFFFFE FFFFFFFF FFFFFFFE FFFFFFFF
r 32B0.08  #                            RP,      RM
*Want "CFXBRA -1.5 M3 modes 6, 7"     FFFFFFFF FFFFFFFE

r 32C0.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA -0.5 FPCR modes 1-3, 7" 00000000 00000000 FFFFFFFF FFFFFFFF
r 32D0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA -0.5 M3 modes 1, 3-5"   FFFFFFFF FFFFFFFF 00000000 00000000
r 32E0.08  #                            RP,      RM
*Want "CFXBRA -0.5 M3 modes 6, 7"     00000000 FFFFFFFF

r 32F0.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA +0.5 FPCR modes 1-3, 7" 00000000 00000001 00000000 00000001
r 3300.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA +0.5 M3 modes 1, 3-5"   00000001 00000001 00000000 00000000
r 3310.08  #                            RP,      RM
*Want "CFXBRA +0.5 M3 modes 6, 7"     00000001 00000000

r 3320.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA +1.5 FPCR modes 1-3, 7" 00000001 00000002 00000001 00000001
r 3330.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA +1.5 M3 modes 1, 3-5"   00000002 00000001 00000002 00000001
r 3340.08  #                            RP,      RM
*Want "CFXBRA +1.5 M3 modes 6, 7"     00000002 00000001

r 3350.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA +2.5 FPCR modes 1-3, 7" 00000002 00000003 00000002 00000003
r 3360.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA +2.5 M3 modes 1, 3-5"   00000003 00000003 00000002 00000002
r 3370.08  #                            RP,      RM
*Want "CFXBRA +2.5 M3 modes 6, 7"     00000003 00000002

r 3380.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA +5.5 FPCR modes 1-3, 7" 00000005 00000006 00000005 00000005
r 3390.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA +5.5 M3 modes 1, 3-5"   00000006 00000005 00000006 00000005
r 33A0.08  #                            RP,      RM
*Want "CFXBRA +5.5 M3 modes 6, 7"     00000006 00000005

r 33B0.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA +9.5 FPCR modes 1-3, 7" 00000009 0000000A 00000009 00000009
r 33C0.10  #                            RNTA,    RFS,    RNTE,    RZ
*Want "CFXBRA +9.5 M3 modes 1, 3-5"   0000000A 00000009 0000000A 00000009
r 33D0.08  #                            RP,      RM
*Want "CFXBRA +9.5 M3 modes 6, 7"     0000000A 00000009

r 33E0.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA +0.75 FPCR modes 1-3, 7" 00000000 00000001 00000000 00000001
r 33F0.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA +0.75 M3 modes 1, 3-5"   00000001 00000001 00000001 00000000
r 3400.08  #                            RP,      RM
*Want "CFXBRA +0.75 M3 modes 6, 7"     00000001 00000000

r 3410.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA +0.25 FPCR modes 1-3, 7" 00000000 00000001 00000000 00000001
r 3420.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA +0.25 M3 modes 1, 3-5"   00000000 00000001 00000000 00000000
r 3430.08  #                            RP,      RM
*Want "CFXBRA +0.25 M3 modes 6, 7"     00000001 00000000

r 3440.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA -0.75 FPCR modes 1-3, 7" 00000000 00000000 FFFFFFFF FFFFFFFF
r 3450.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA -0.75 M3 modes 1, 3-5"   FFFFFFFF FFFFFFFF FFFFFFFF 00000000
r 3460.08  #                            RP,      RM
*Want "CFXBRA -0.75 M3 modes 6, 7"     00000000 FFFFFFFF

r 3470.10  #                            RZ,      RP,      RM,      RFS
*Want "CFXBRA -0.25 FPCR modes 1-3, 7" 00000000 00000000 FFFFFFFF FFFFFFFF
r 3480.10  #                            RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA -0.25 M3 modes 1, 3-5"   00000000 FFFFFFFF 00000000 00000000
r 3490.08  #                            RP,      RM
*Want "CFXBRA -0.25 M3 modes 6, 7"     00000000 FFFFFFFF

r 34A0.10  #                               RZ,      RP,      RM,      RFS
*Want "CFXBRA max+0.5 FPCR modes 1-3, 7" 7FFFFFFF 7FFFFFFF 7FFFFFFF 7FFFFFFF
r 34B0.10  #                               RNTA,    RFS,     RNTE,    RZ
*Want "CFXBRA max+0.5 M3 modes 1, 3-5"   7FFFFFFF 7FFFFFFF 7FFFFFFF 7FFFFFFF
r 34C0.08  #                               RP,      RM
*Want "CFXBRA max+0.5 M3 modes 6, 7"     7FFFFFFF 7FFFFFFF


#  rounding mode tests - extended BFP - FPCR contents with cc in last byte
*Compare
r 3600.10
*Want "CFXBRA -9.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 3610.10
*Want "CFXBRA -9.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 3620.08
*Want "CFXBRA -9.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 3630.10
*Want "CFXBRA -5.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 3640.10
*Want "CFXBRA -5.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 3650.08
*Want "CFXBRA -5.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 3660.10
*Want "CFXBRA -2.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 3670.10
*Want "CFXBRA -2.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 3680.08
*Want "CFXBRA -2.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 3690.10
*Want "CFXBRA -1.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 36A0.10
*Want "CFXBRA -1.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 36B0.08
*Want "CFXBRA -1.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 36C0.10
*Want "CFXBRA -0.5 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 36D0.10
*Want "CFXBRA -0.5 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 36E0.08
*Want "CFXBRA -0.5 M3 modes 6, 7 FPCR"     00080001 00080001

r 36F0.10
*Want "CFXBRA +0.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 3700.10
*Want "CFXBRA +0.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 3710.08
*Want "CFXBRA +0.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 3720.10
*Want "CFXBRA +1.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 3730.10
*Want "CFXBRA +1.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 3740.08
*Want "CFXBRA +1.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 3750.10
*Want "CFXBRA +2.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 3760.10
*Want "CFXBRA +2.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 3770.08
*Want "CFXBRA +2.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 3780.10
*Want "CFXBRA +5.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 3790.10
*Want "CFXBRA +5.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 37A0.08
*Want "CFXBRA +5.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 37B0.10
*Want "CFXBRA +9.5 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 37C0.10
*Want "CFXBRA +9.5 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 37D0.08
*Want "CFXBRA +9.5 M3 modes 6, 7 FPCR"     00080002 00080002

r 37E0.10
*Want "CFXBRA +0.75 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 37F0.10
*Want "CFXBRA +0.75 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 3800.08
*Want "CFXBRA +0.75 M3 modes 6, 7 FPCR"     00080002 00080002

r 3810.10
*Want "CFXBRA +0.25 FPCR modes 1-3, 7 FPCR" 00000002 00000002 00000002 00000002
r 3820.10
*Want "CFXBRA +0.25 M3 modes 1, 3-5 FPCR"   00080002 00080002 00080002 00080002
r 3830.08
*Want "CFXBRA +0.25 M3 modes 6, 7 FPCR"     00080002 00080002

r 3840.10
*Want "CFXBRA -0.75 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 3850.10
*Want "CFXBRA -0.75 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 3860.08
*Want "CFXBRA -0.75 M3 modes 6, 7 FPCR"     00080001 00080001

r 3870.10
*Want "CFXBRA -0.25 FPCR modes 1-3, 7 FPCR" 00000001 00000001 00000001 00000001
r 3880.10
*Want "CFXBRA -0.25 M3 modes 1, 3-5 FPCR"   00080001 00080001 00080001 00080001
r 3890.08
*Want "CFXBRA -0.25 M3 modes 6, 7 FPCR"     00080001 00080001

r 38A0.10
*Want "CFXBRA max+0.5 FPCR modes 1-3, 7 FPCR" 00000002 00800003 00000002 00000002
r 38B0.10
*Want "CFXBRA max+0.5 M3 modes 1, 3-5 FPCR"   00880003 00080002 00880003 00080002
r 38C0.08
*Want "CFXBRA max+0.5 M3 modes 6, 7 FPCR"     00880003 00080002

*Done

