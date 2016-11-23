*Testcase bfp-010-cvtfrfix.tst:   CEFBR, CEFBRA, CDFBR, CDFBRA, CXFBR, CXFBRA

#Testcase bfp-010-cvtfrfix.tst: IEEE Convert From Fixed
#..Includes CONVERT FROM FIXED 32 (6).  Also tests traps and 
#..exceptions and results from different rounding modes.

sysclear
archmode esame

#
# Following suppresses logging of program checks.  This test program, as part
# of its normal operation, generates 2 program check messages that have no
# value in the validation process.  (The messages, not the program checks.)
#
ostailor quiet

loadcore "$(testpath)/bfp-010-cvtfrfix.core"

runtest 1.0

ostailor null   # restore messages for subsequent tests


# Inputs converted to short BFP
*Compare
r 1000.10
*Want "CEFBR result pairs 1-2"   3F800000 3F800000 40000000 40000000
r 1010.10
*Want "CEFBR result pairs 3-4"   40800000 40800000 C0000000 C0000000
r 1020.10
*Want "CEFBR result pairs 5-6"   4F000000 4F000000 CF000000 CF000000
r 1030.08
*Want "CEFBR result pair 7"       4EFFFFFF 4EFFFFFF

# Inputs converted to short BFP - FPCR contents
*Compare
r 1100.10
*Want "CEFBR FPC pairs 1-2"      00000000 F8000000 00000000 F8000000
r 1110.10
*Want "CEFBR FPC pairs 3-4"      00000000 F8000000 00000000 F8000000
r 1120.10
*Want "CEFBR FPC pairs 5-6"      00080000 F8000C00 00080000 F8000C00
r 1130.08
*Want "CEFBR FPC pair 7"         00000000 F8000000


# Short BFP rounding mode tests - FPCR & M3 modes, positive & negative inputs
*Compare
r 1200.10  #                           RZ       RP       RM       RFS
*Want "CEFBRA RU FPC modes 1-3, 7"   4EFFFFFF 4F000000 4EFFFFFF 4EFFFFFF
r 1210.10  #                           RNTA     RFS      RNTE     RZ
*Want "CEFBRA RU M3 modes 1, 3-5"    4F000000 4EFFFFFF 4F000000 4EFFFFFF
r 1220.08  #                           RP       RM
*Want "CEFBRA RU M3 modes 6, 7"      4F000000 4EFFFFFF

r 1230.10  #                           RZ       RP       RM       RFS
*Want "CEFBRA Tie FPC modes 1-3, 7"  4EFFFFFF 4F000000 4EFFFFFF 4EFFFFFF
r 1240.10  #                           RNTA     RFS      RNTE     RZ
*Want "CEFBRA Tie M3 modes 1, 3-5"   4F000000 4EFFFFFF 4F000000 4EFFFFFF
r 1250.08  #                           RP       RM
*Want "CEFBRA Tie M3 modes 6, 7"     4F000000 4EFFFFFF 

r 1260.10  #                           RZ       RP       RM       RFS
*Want "CEFBRA RD FPC modes 1-3, 7"   4EFFFFFF 4F000000 4EFFFFFF 4EFFFFFF
r 1270.10  #                           RNTA     RFS      RNTE     RZ
*Want "CEFBRA RD M3 modes 1, 3-5"    4EFFFFFF 4EFFFFFF 4EFFFFFF 4EFFFFFF
r 1280.08  #                           RP       RM
*Want "CEFBRA RD M3 modes 6, 7"      4F000000 4EFFFFFF

*Compare
r 1500.10  #                                RZ       RP       RM       RFS
*Want "CEFBRA RU FPC modes 1-3, 7 FCPR"   00000001 00000002 00000003 00000007
r 1510.10  #                                RNTA     RFS      RNTE     RZ
*Want "CEFBRA RU M3 modes 1, 3-5 FPCR"    00080000 00080000 00080000 00080000
r 1520.08  #                                RP       RM
*Want "CEFBRA RU M3 modes 6, 7 FPCR"      00080000 00080000

r 1530.10  #                                RZ       RP       RM       RFS
*Want "CEFBRA Tie FPC modes 1-3, 7 FPCR"  00000001 00000002 00000003 00000007
r 1540.10  #                                RNTA     RFS      RNTE     RZ
*Want "CEFBRA Tie M3 modes 1, 3-5 FPCR"   00080000 00080000 00080000 00080000
r 1550.08  #                                RP       RM
*Want "CEFBRA Tie M3 modes 6, 7 FPCR"     00080000 00080000

r 1560.10  #                                RZ       RP       RM       RFS
*Want "CEFBRA RD FPC modes 1-3, 7 FPCR"   00000001 00000002 00000003 00000007
r 1570.10  #                                RNTA     RFS      RNTE     RZ
*Want "CEFBRA RD M3 modes 1, 3-5 FPCR"    00080000 00080000 00080000 00080000
r 1580.08  #                                RP       RM
*Want "CEFBRA RD M3 modes 6, 7 FPCR"      00080000 00080000


# Inputs converted to long BFP
*Compare
r 2000.10
*Want "CDFBR result pair 1" 3FF00000 00000000 3FF00000 00000000
r 2010.10
*Want "CDFBR result pair 2" 40000000 00000000 40000000 00000000 
r 2020.10
*Want "CDFBR result pair 3" 40100000 00000000 40100000 00000000
r 2030.10
*Want "CDFBR result pair 4" C0000000 00000000 C0000000 00000000
r 2040.10
*Want "CDFBR result pair 5" 41DFFFFF FFC00000 41DFFFFF FFC00000
r 2050.10
*Want "CDFBR result pair 6" C1DFFFFF FFC00000 C1DFFFFF FFC00000
r 2060.10
*Want "CDFBR result pair 7" 41DFFFFF E0000000 41DFFFFF E0000000

# Inputs converted to long BFP - FPCR contents
*Compare
r 2100.10
*Want "CDFBR FPC pairs 1-2" 00000000 F8000000 00000000 F8000000
r 2110.10
*Want "CDFBR FPC pairs 3-4" 00000000 F8000000 00000000 F8000000
r 2120.10
*Want "CDFBR FPC pairs 5-6" 00000000 F8000000 00000000 F8000000
r 2130.08
*Want "CDFBR FPC pair 7"    00000000 F8000000


# Inputs converted to extended BFP
*Compare
r 3000.10
*Want "CXFBR result 1a" 3FFF0000 00000000 00000000 00000000
r 3010.10
*Want "CXFBR result 1b" 3FFF0000 00000000 00000000 00000000
r 3020.10
*Want "CXFBR result 2a" 40000000 00000000 00000000 00000000
r 3030.10
*Want "CXFBR result 2b" 40000000 00000000 00000000 00000000
r 3040.10
*Want "CXFBR result 3a" 40010000 00000000 00000000 00000000
r 3050.10
*Want "CXFBR result 3b" 40010000 00000000 00000000 00000000
r 3060.10
*Want "CXFBR result 4a" C0000000 00000000 00000000 00000000
r 3070.10
*Want "CXFBR result 4b" C0000000 00000000 00000000 00000000
r 3080.10
*Want "CXFBR result 5a" 401DFFFF FFFC0000 00000000 00000000
r 3090.10
*Want "CXFBR result 5b" 401DFFFF FFFC0000 00000000 00000000
r 30A0.10
*Want "CXFBR result 6a" C01DFFFF FFFC0000 00000000 00000000
r 30B0.10
*Want "CXFBR result 6b" C01DFFFF FFFC0000 00000000 00000000
r 30C0.10
*Want "CXFBR result 7a" 401DFFFF FE000000 00000000 00000000
r 30D0.10
*Want "CXFBR result 7b" 401DFFFF FE000000 00000000 00000000

# Inputs converted to extended BFP - FPCR contents
*Compare
r 3200.10
*Want "CXFBR FPC pairs 1-2" 00000000 F8000000 00000000 F8000000
r 3210.10
*Want "CXFBR FPC pairs 3-4" 00000000 F8000000 00000000 F8000000
r 3220.10
*Want "CXFBR FPC pairs 5-6" 00000000 F8000000 00000000 F8000000
r 3230.08
*Want "CXFBR FPC pair 7"    00000000 F8000000


*Done

