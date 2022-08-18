*Testcase dfp-080-to-packed.tst:   CPDT, CPXT

sysclear
archmode esame

#
# Following suppresses logging of program checks.  This test program, as part
# of its normal operation, generates 16 expected program specification checks from
# packed lengths greater than allowed. This is part value of the validation process.
# (Not the messages, the program checks.)
#

ostailor quiet

loadcore "dfp-080-to-packed.core" 0x0
runtest 1.0

ostailor default   # restore messages for subsequent tests

# Test Results - Convert To Packed (CPDT) - results

# CPDT test results
# A long DFP number is converted to packed as unsiged and signed with
# Plus-Sign-Code Control and Plus-Sign-Code Control diabled and
# enabled resulting is 8 packed values per DFP number.

# CPDT Results

*Compare

r 2000.10
*Want "CPDT +0 and -0"      00000000 0C0C0F0F 00000000 0D0C0D0F
r 2010.10
*Want "CPDT +7 and -7"      07070707 7C7C7F7F 07070707 7D7D7D7D
r 2020.10
*Want "CPDT +12345"         01234501 23450123 45012345 12345C12
r 2030.8
*Want "CPDT +12345"         345C1234 5F12345F 
r 2038.8
*Want "CPDT -12345"         01234501 23450123
r 2040.10
*Want "CPDT -12345"         45012345 12345D12 345D1234 5D12345D
r 2050.10
*Want "CPDT +271828182"     00027182 81820002 71828182 00027182
r 2060.10
*Want "CPDT +271828182"     81820002 71828182 00271828 182C0027
r 2070.10
*Want "CPDT +271828182"     1828182C 00271828 182F0027 1828182F
r 2080.10
*Want "CPDT -271828182"     00027182 81820002 71828182 00027182
r 2090.10
*Want "CPDT -271828182"     81820002 71828182 00271828 182D0027
r 20A0.10
*Want "CPDT -271828182"     1828182D 00271828 182D0027 1828182D
r 20B0.10
*Want "CPDT +3.14159265358979"     03141592 65358979 03141592 65358979
r 20C0.10
*Want "CPDT +3.14159265358979"     03141592 65358979 03141592 65358979
r 20D0.10
*Want "CPDT +3.14159265358979"     31415926 5358979C 31415926 5358979C
r 20E0.10
*Want "CPDT +3.14159265358979"     31415926 5358979F 31415926 5358979F
r 20F0.10
*Want "CPDT -3.14159265358979"     03141592 65358979 03141592 65358979
r 2100.10
*Want "CPDT -3.14159265358979"     03141592 65358979 03141592 65358979
r 2110.10
*Want "CPDT -3.14159265358979"     31415926 5358979D 31415926 5358979D
r 2120.10
*Want "CPDT -3.14159265358979"     31415926 5358979D 31415926 5358979D
r 2130.10
*Want "CPDT +0E368"                00000000 00000000 000C000C 000F000F
r 2140.10
*Want "CPDT 0E-397"                00000000 00000000 00000000 00000000
r 2150.10
*Want "CPDT 0E-397"                0000000C 0000000C 0000000F 0000000F
r 2160.10
*Want "CPDT -0E368"                00000000 00000000 00000000 00000000
r 2170.10
*Want "CPDT -0E368"                00000000 00000000 0D000000 000C0000
r 2180.8
*Want "CPDT -0E368"                00000D00 0000000F 
r 2188.8
*Want "CPDT -0E-397"               00000000 00000000 
r 2190.10
*Want "CPDT -0E-397"               00000000 00000000 00000000 00000000
r 21A0.10
*Want "CPDT -0E-397"               00000000 00000000 00000D00 00000000
r 21B0.10
*Want "CPDT -0E-397"               000C0000 00000000 0D000000 0000000F
r 21C0.10
*Want "CPDT +123456789012345E369"  01234567 89012345 01234567 89012345
r 21D0.10
*Want "CPDT +123456789012345E369"  01234567 89012345 01234567 89012345
r 21E0.10
*Want "CPDT +123456789012345E369"  12345678 9012345C 12345678 9012345C
r 21F0.10
*Want "CPDT +123456789012345E369"  12345678 9012345F 12345678 9012345F
r 2200.10
*Want "CPDT +123456789012345E-398"  01234567 89012345 01234567 89012345
r 2210.10
*Want "CPDT +123456789012345E-398"  01234567 89012345 01234567 89012345
r 2220.10
*Want "CPDT +123456789012345E-398"  12345678 9012345C 12345678 9012345C
r 2230.10
*Want "CPDT +123456789012345E-398"  12345678 9012345F 12345678 9012345F
r 2240.10
*Want "CPDT -123456789012345E369"   01234567 89012345 01234567 89012345
r 2250.10
*Want "CPDT -123456789012345E369"   01234567 89012345 01234567 89012345
r 2260.10
*Want "CPDT -123456789012345E369"   12345678 9012345D 12345678 9012345D
r 2270.10
*Want "CPDT -123456789012345E369"   12345678 9012345D 12345678 9012345D
r 2280.10
*Want "CPDT -123456789012345E-398"  01234567 89012345 01234567 89012345
r 2290.10
*Want "CPDT -123456789012345E-398"  01234567 89012345 01234567 89012345
r 22A0.10
*Want "CPDT -123456789012345E-398"  12345678 9012345D 12345678 9012345D
r 22B0.10
*Want "CPDT -123456789012345E-398"  12345678 9012345D 12345678 9012345D
r 22C0.10
*Want "CPDT +1234567890123456E368"  12345678 90123456 12345678 90123456
r 22D0.10
*Want "CPDT +1234567890123456E368"  12345678 90123456 12345678 90123456
r 22E0.10
*Want "CPDT +1234567890123456E368"  23456789 0123456C 23456789 0123456C 
r 22F0.10
*Want "CPDT +1234567890123456E368"  23456789 0123456F 23456789 0123456F 
r 2300.10
*Want "CPDT +1234567890123456E-397"  12345678 90123456 12345678 90123456
r 2310.10
*Want "CPDT +1234567890123456E-397"  12345678 90123456 12345678 90123456
r 2320.10
*Want "CPDT +1234567890123456E-397"  23456789 0123456C 23456789 0123456C 
r 2330.10
*Want "CPDT +1234567890123456E-397"  23456789 0123456F 23456789 0123456F
r 2340.10
*Want "CPDT -1234567890123456E368"  12345678 90123456 12345678 90123456
r 2350.10
*Want "CPDT -1234567890123456E368"  12345678 90123456 12345678 90123456
r 2360.10
*Want "CPDT -1234567890123456E368"  23456789 0123456D 23456789 0123456D 
r 2370.10
*Want "CPDT -1234567890123456E368"  23456789 0123456D 23456789 0123456D
r 2380.10
*Want "CPDT -1234567890123456E-397" 12345678 90123456 12345678 90123456
r 2390.10
*Want "CPDT -1234567890123456E-397" 12345678 90123456 12345678 90123456
r 23A0.10
*Want "CPDT -1234567890123456E-397" 23456789 0123456D 23456789 0123456D 
r 23B0.10
*Want "CPDT -1234567890123456E-397" 23456789 0123456D 23456789 0123456D

r 23C0.10
*Want "CPDT +(INF)"         00000000 00000000 00000000 00000000
r 23D0.10
*Want "CPDT +(INF)"         00000000 00000000 00000000 00000000
r 23E0.10
*Want "CPDT +(INF)"         00000000 0000000C 00000000 0000000C 
r 23F0.10
*Want "CPDT +(INF)"         00000000 0000000F 00000000 0000000F

r 2400.10
*Want "CPDT -(INF)"         00000000 00000000 00000000 00000000
r 2410.10
*Want "CPDT -(INF)"         00000000 00000000 00000000 00000000
r 2420.10
*Want "CPDT -(INF)"         00000000 0000000D 00000000 0000000D 
r 2430.10
*Want "CPDT -(INF)"         00000000 0000000D 00000000 0000000D

r 2440.10
*Want "CPDT +(QNAN)"         00000000 00000000 00000000 00000000
r 2450.10
*Want "CPDT +(QNAN)"         00000000 00000000 00000000 00000000
r 2460.10
*Want "CPDT +(QNAN)"         00000000 0000000C 00000000 0000000C 
r 2470.10
*Want "CPDT +(QNAN)"         00000000 0000000F 00000000 0000000F

r 2480.10
*Want "CPDT -(QNAN)"         00000000 00000000 00000000 00000000
r 2490.10
*Want "CPDT -(QNAN)"         00000000 00000000 00000000 00000000
r 24A0.10
*Want "CPDT -(QNAN)"         00000000 0000000D 00000000 0000000D 
r 24B0.10
*Want "CPDT -(QNAN)"         00000000 0000000D 00000000 0000000D


r 24C0.10
*Want "CPDT +(SNAN)"         00000000 00000000 00000000 00000000
r 24D0.10
*Want "CPDT +(SNAN)"         00000000 00000000 00000000 00000000
r 24E0.10
*Want "CPDT +(SNAN)"         00000000 0000000C 00000000 0000000C 
r 24F0.10
*Want "CPDT +(SNAN)"         00000000 0000000F 00000000 0000000F

r 2500.10
*Want "CPDT -(SNAN)"         00000000 00000000 00000000 00000000
r 2510.10
*Want "CPDT -(SNAN)"         00000000 00000000 00000000 00000000
r 2520.10
*Want "CPDT -(SNAN)"         00000000 0000000D 00000000 0000000D 
r 2530.10
*Want "CPDT -(SNAN)"         00000000 0000000D 00000000 0000000D

r 2540.10
*Want "CPDT +(DMIN)"         00000000 00010000 00000001 00000000
r 2550.10
*Want "CPDT +(DMIN)"         00010000 00000001 00000000 001C0000
r 2560.10
*Want "CPDT +(DMIN)"         0000001C 00000000 001F0000 0000001F 

r 2570.10
*Want "CPDT -(DMIN)"         00000000 00010000 00000001 00000000
r 2580.10
*Want "CPDT -(DMIN)"         00010000 00000001 00000000 001D0000
r 2590.10
*Want "CPDT -(DMIN)"         0000001D 00000000 001D0000 0000001D 

# Packed Length of 10 --> Program Specification Exception
r 25A0.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 25B0.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 25C0.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 25D0.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 25E0.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 25F0.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 2600.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 2610.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 2620.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 2630.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000

# Packed Length of 10 --> Program Specification Exception
r 25A0.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 25B0.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 25C0.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 25D0.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 25E0.10
*Want "CPDT +3.14159265358979 "    00000000 00000000 00000000 00000000


# Packed Length of 10 --> Program Specification Exception
r 25F0.10
*Want "CPDT -3.14159265358979 "    00000000 00000000 00000000 00000000
r 2600.10
*Want "CPDT -3.14159265358979 "    00000000 00000000 00000000 00000000
r 2610.10
*Want "CPDT -3.14159265358979 "    00000000 00000000 00000000 00000000
r 2620.10
*Want "CPDT -3.14159265358979 "    00000000 00000000 00000000 00000000
r 2630.10
*Want "CPDT -3.14159265358979 "    00000000 00000000 00000000 00000000

# End of Data marker
r 2640.10
*Want "CPDT 1111111111111111 "    11111111 11111111 11111111 11111111
r 2650.10
*Want "CPDT 1111111111111111 "    11111111 11111111 11111111 11111111
r 2660.10
*Want "CPDT 1111111111111111 "    11111111 1111111C 11111111 1111111C
r 2670.10
*Want "CPDT 1111111111111111 "    11111111 1111111F 11111111 1111111F

# -----------------------------------------------
# CPDT Program Interupt code, DFP condition codes
# -----------------------------------------------
r 2700.10
*Want "CPDT +0 "          FFFF0000 FFFF0000 FFFF0000 FFFF0000
r 2710.10
*Want "CPDT +0 "          FFFF0000 FFFF0000 FFFF0000 FFFF0000
r 2720.10
*Want "CPDT -0 "          FFFF0000 FFFF0000 FFFF0000 FFFF0000
r 2730.10
*Want "CPDT -0 "          FFFF0000 FFFF0000 FFFF0000 FFFF0000
r 2740.10
*Want "CPDT +7 "          FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 2750.10
*Want "CPDT +7 "          FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 2760.10
*Want "CPDT -7 "          FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 2770.10
*Want "CPDT -7 "          FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 2780.10
*Want "CPDT +12345"       FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 2790.10
*Want "CPDT +12345"       FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 27A0.10
*Want "CPDT -12345"       FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 27B0.10
*Want "CPDT -12345"       FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 27C0.10
*Want "CPDT +271828182"   FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 27D0.10
*Want "CPDT +271828182"   FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 27E0.10
*Want "CPDT -271828182"   FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 27F0.10
*Want "CPDT -271828182"   FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 2800.10
*Want "CPDT +3.14159265358979"   FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 2810.10
*Want "CPDT +3.14159265358979"   FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 2820.10
*Want "CPDT -3.14159265358979"   FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 2830.10
*Want "CPDT -3.14159265358979"   FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 2840.10
*Want "CPDT +0E368"     FFFF0000 FFFF0000 FFFF0000 FFFF0000              
r 2850.10
*Want "CPDT +0E368"     FFFF0000 FFFF0000 FFFF0000 FFFF0000  
r 2860.10
*Want "CPDT 0E-397"     FFFF0000 FFFF0000 FFFF0000 FFFF0000  
r 2870.10
*Want "CPDT 0E-397"     FFFF0000 FFFF0000 FFFF0000 FFFF0000 
r 2880.10
*Want "CPDT -0E368"     FFFF0000 FFFF0000 FFFF0000 FFFF0000  
r 2890.10
*Want "CPDT -0E368"     FFFF0000 FFFF0000 FFFF0000 FFFF0000 
r 28A0.10
*Want "CPDT -0E-397"    FFFF0000 FFFF0000 FFFF0000 FFFF0000  
r 28B0.10
*Want "CPDT -0E-397"    FFFF0000 FFFF0000 FFFF0000 FFFF0000 
r 28C0.10
*Want "CPDT +123456789012345E369"   FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 28D0.10
*Want "CPDT +123456789012345E369"   FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 28E0.10
*Want "CPDT +123456789012345E-398"  FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 28F0.10
*Want "CPDT +123456789012345E-398"  FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 2900.10
*Want "CPDT -123456789012345E369"   FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 2910.10
*Want "CPDT -123456789012345E369"   FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 2920.10
*Want "CPDT -123456789012345E-398"  FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 2930.10
*Want "CPDT -123456789012345E-398"  FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 2940.10
*Want "CPDT +1234567890123456E368"  FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 2950.10
*Want "CPDT +1234567890123456E368"  FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 2960.10
*Want "CPDT +1234567890123456E-397" FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 2970.10
*Want "CPDT +1234567890123456E-397" FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 2980.10
*Want "CPDT -1234567890123456E368" FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 2990.10
*Want "CPDT -1234567890123456E368" FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 29A0.10
*Want "CPDT -1234567890123456E-397" FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 29B0.10
*Want "CPDT -1234567890123456E-397" FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 29C0.10
*Want "CPDT +(INF)"     FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 29D0.10
*Want "CPDT +(INF)"     FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 29E0.10
*Want "CPDT -(INF)"     FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 29F0.10
*Want "CPDT -(INF)"     FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 2A00.10
*Want "CPDT +(QNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 2A10.10
*Want "CPDT +(QNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 2A20.10
*Want "CPDT -(QNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 2A30.10
*Want "CPDT -(QNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003

r 2A40.10
*Want "CPDT +(SNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 2A50.10
*Want "CPDT +(SNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 2A60.10
*Want "CPDT -(SNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 2A70.10
*Want "CPDT -(SNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003

r 2A80.10
*Want "CPDT +(DMIN)"    FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 2A90.10
*Want "CPDT +(DMIN)"    FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 2AA0.10
*Want "CPDT -(DMIN)"    FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 2AB0.10
*Want "CPDT -(DMIN)"    FFFF0001 FFFF0001 FFFF0001 FFFF0001

# Packed Length of 10 --> Program Specification Exception
#                         do not check DXC, CC
r 2AC0.2
*Want "CPDT +3.14159265358979 "    0006
r 2AC4.2
*Want "CPDT +3.14159265358979 "    0006
r 2AC8.2
*Want "CPDT +3.14159265358979 "    0006
r 2ACC.2
*Want "CPDT +3.14159265358979 "    0006
r 2AD0.2
*Want "CPDT +3.14159265358979 "    0006
r 2AD4.2
*Want "CPDT +3.14159265358979 "    0006
r 2AD8.2
*Want "CPDT +3.14159265358979 "    0006
r 2ADC.2
*Want "CPDT +3.14159265358979 "    0006

# Packed Length of 10 --> Program Specification Exception
#                         do not check DXC, CC
r 2AE0.2
*Want "CPDT -3.14159265358979 "    0006
r 2AE4.2
*Want "CPDT -3.14159265358979 "    0006
r 2AE8.2
*Want "CPDT -3.14159265358979 "    0006
r 2AEC.2
*Want "CPDT -3.14159265358979 "    0006
r 2AF0.2
*Want "CPDT -3.14159265358979 "    0006
r 2AF4.2
*Want "CPDT -3.14159265358979 "    0006
r 2AF8.2
*Want "CPDT -3.14159265358979 "    0006
r 2AFC.2
*Want "CPDT -3.14159265358979 "    0006


# End of Data marker
r 2B00.10
*Want "CPDT 1111111111111111 " FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 2B10.10
*Want "CPDT 1111111111111111 " FFFF0003 FFFF0003 FFFF0003 FFFF0003



# Test Results - Convert To Packed (CPXT) - results

# CPXT test results
# An extended DFP number is converted to packed as unsiged and signed with
# Plus-Sign-Code Control and Plus-Sign-Code Control diabled and
# enabled resulting is 8 packed values per DFP number.

# CPXT Results
*Compare
r 3000.10
*Want "CPXT +0 and -0"      00000000 0C0C0F0F 00000000 0D0C0D0F
r 3010.10
*Want "CPXT +7 and -7"      07070707 7C7C7F7F 07070707 7D7D7D7D
r 3020.10
*Want "CPXT +12345"         01234501 23450123 45012345 12345C12
r 3030.8
*Want "CPXT +12345"         345C1234 5F12345F 
r 3038.8
*Want "CPXT -12345"         01234501 23450123
r 3040.10
*Want "CPXT -12345"         45012345 12345D12 345D1234 5D12345D
r 3050.10
*Want "CPXT +271828182"     00027182 81820002 71828182 00027182
r 3060.10
*Want "CPXT +271828182"     81820002 71828182 00271828 182C0027
r 3070.10
*Want "CPXT +271828182"     1828182C 00271828 182F0027 1828182F
r 3080.10
*Want "CPXT -271828182"     00027182 81820002 71828182 00027182
r 3090.10
*Want "CPXT -271828182"     81820002 71828182 00271828 182D0027
r 30A0.10
*Want "CPXT -271828182"     1828182D 00271828 182D0027 1828182D
r 30B0.10
*Want "CPXT +3.14159265358979"     03141592 65358979 03141592 65358979
r 30C0.10
*Want "CPXT +3.14159265358979"     03141592 65358979 03141592 65358979
r 30D0.10
*Want "CPXT +3.14159265358979"     31415926 5358979C 31415926 5358979C
r 30E0.10
*Want "CPXT +3.14159265358979"     31415926 5358979F 31415926 5358979F
r 30F0.10
*Want "CPXT -3.14159265358979"     03141592 65358979 03141592 65358979
r 3100.10
*Want "CPXT -3.14159265358979"     03141592 65358979 03141592 65358979
r 3110.10
*Want "CPXT -3.14159265358979"     31415926 5358979D 31415926 5358979D
r 3120.10
*Want "CPXT -3.14159265358979"     31415926 5358979D 31415926 5358979D
r 3130.10
*Want "CPXT +0E368"                00000000 00000000 000C000C 000F000F
r 3140.10
*Want "CPXT 0E-397"                00000000 00000000 00000000 00000000
r 3150.10
*Want "CPXT 0E-397"                0000000C 0000000C 0000000F 0000000F
r 3160.10
*Want "CPXT -0E368"                00000000 00000000 00000000 00000000
r 3170.10
*Want "CPXT -0E368"                00000000 00000000 0D000000 000C0000
r 3180.8
*Want "CPXT -0E368"                00000D00 0000000F 
r 3188.8
*Want "CPXT -0E-397"               00000000 00000000 
r 3190.10
*Want "CPXT -0E-397"               00000000 00000000 00000000 00000000
r 31A0.10
*Want "CPXT -0E-397"               00000000 00000000 00000D00 00000000
r 31B0.10
*Want "CPXT -0E-397"               000C0000 00000000 0D000000 0000000F
r 31C0.10
*Want "CPXT +123456789012345E369"  01234567 89012345 01234567 89012345
r 31D0.10
*Want "CPXT +123456789012345E369"  01234567 89012345 01234567 89012345
r 31E0.10
*Want "CPXT +123456789012345E369"  12345678 9012345C 12345678 9012345C
r 31F0.10
*Want "CPXT +123456789012345E369"  12345678 9012345F 12345678 9012345F
r 3200.10
*Want "CPXT +123456789012345E-398"  01234567 89012345 01234567 89012345
r 3210.10
*Want "CPXT +123456789012345E-398"  01234567 89012345 01234567 89012345
r 3220.10
*Want "CPXT +123456789012345E-398"  12345678 9012345C 12345678 9012345C
r 3230.10
*Want "CPXT +123456789012345E-398"  12345678 9012345F 12345678 9012345F
r 3240.10
*Want "CPXT -123456789012345E369"   01234567 89012345 01234567 89012345
r 3250.10
*Want "CPXT -123456789012345E369"   01234567 89012345 01234567 89012345
r 3260.10
*Want "CPXT -123456789012345E369"   12345678 9012345D 12345678 9012345D
r 3270.10
*Want "CPXT -123456789012345E369"   12345678 9012345D 12345678 9012345D
r 3280.10
*Want "CPXT -123456789012345E-398"  01234567 89012345 01234567 89012345
r 3290.10
*Want "CPXT -123456789012345E-398"  01234567 89012345 01234567 89012345
r 32A0.10
*Want "CPXT -123456789012345E-398"  12345678 9012345D 12345678 9012345D
r 32B0.10
*Want "CPXT -123456789012345E-398"  12345678 9012345D 12345678 9012345D
r 32C0.10
*Want "CPXT +1234567890123456E368"  12345678 90123456 12345678 90123456
r 32D0.10
*Want "CPXT +1234567890123456E368"  12345678 90123456 12345678 90123456
r 32E0.10
*Want "CPXT +1234567890123456E368"  23456789 0123456C 23456789 0123456C 
r 32F0.10
*Want "CPXT +1234567890123456E368"  23456789 0123456F 23456789 0123456F 
r 3300.10
*Want "CPXT +1234567890123456E-397"  12345678 90123456 12345678 90123456
r 3310.10
*Want "CPXT +1234567890123456E-397"  12345678 90123456 12345678 90123456
r 3320.10
*Want "CPXT +1234567890123456E-397"  23456789 0123456C 23456789 0123456C 
r 3330.10
*Want "CPXT +1234567890123456E-397"  23456789 0123456F 23456789 0123456F
r 3340.10
*Want "CPXT -1234567890123456789012345678901234E6110"  90123456 78901234 90123456 78901234
r 3350.10
*Want "CPXT -1234567890123456789012345678901234E6110"  90123456 78901234 90123456 78901234
r 3360.10
*Want "CPXT -1234567890123456789012345678901234E6110"  01234567 8901234D 01234567 8901234D 
r 3370.10
*Want "CPXT -1234567890123456789012345678901234E6110"  01234567 8901234D 01234567 8901234D
r 3380.10
*Want "CPXT -1234567890123456789012345678901234E-6175" 90123456 78901234 90123456 78901234
r 3390.10
*Want "CPXT -1234567890123456789012345678901234E-6175" 90123456 78901234 90123456 78901234
r 33A0.10
*Want "CPXT -1234567890123456789012345678901234E-6175" 01234567 8901234D 01234567 8901234D 
r 33B0.10
*Want "CPXT -1234567890123456789012345678901234E-6175" 01234567 8901234D 01234567 8901234D

r 33C0.10
*Want "CPXT +(INF)"         00000000 00000000 00000000 00000000
r 33D0.10
*Want "CPXT +(INF)"         00000000 00000000 00000000 00000000
r 33E0.10
*Want "CPXT +(INF)"         00000000 0000000C 00000000 0000000C 
r 33F0.10
*Want "CPXT +(INF)"         00000000 0000000F 00000000 0000000F

r 3400.10
*Want "CPXT -(INF)"         00000000 00000000 00000000 00000000
r 3410.10
*Want "CPXT -(INF)"         00000000 00000000 00000000 00000000
r 3420.10
*Want "CPXT -(INF)"         00000000 0000000D 00000000 0000000D 
r 3430.10
*Want "CPXT -(INF)"         00000000 0000000D 00000000 0000000D

r 3440.10
*Want "CPXT +(QNAN)"         00000000 00000000 00000000 00000000
r 3450.10
*Want "CPXT +(QNAN)"         00000000 00000000 00000000 00000000
r 3460.10
*Want "CPXT +(QNAN)"         00000000 0000000C 00000000 0000000C 
r 3470.10
*Want "CPXT +(QNAN)"         00000000 0000000F 00000000 0000000F

r 3480.10
*Want "CPXT -(QNAN)"         00000000 00000000 00000000 00000000
r 3490.10
*Want "CPXT -(QNAN)"         00000000 00000000 00000000 00000000
r 34A0.10
*Want "CPXT -(QNAN)"         00000000 0000000D 00000000 0000000D 
r 34B0.10
*Want "CPXT -(QNAN)"         00000000 0000000D 00000000 0000000D


r 34C0.10
*Want "CPXT +(SNAN)"         00000000 00000000 00000000 00000000
r 34D0.10
*Want "CPXT +(SNAN)"         00000000 00000000 00000000 00000000
r 34E0.10
*Want "CPXT +(SNAN)"         00000000 0000000C 00000000 0000000C 
r 34F0.10
*Want "CPXT +(SNAN)"         00000000 0000000F 00000000 0000000F

r 3500.10
*Want "CPXT -(SNAN)"         00000000 00000000 00000000 00000000
r 3510.10
*Want "CPXT -(SNAN)"         00000000 00000000 00000000 00000000
r 3520.10
*Want "CPXT -(SNAN)"         00000000 0000000D 00000000 0000000D 
r 3530.10
*Want "CPXT -(SNAN)"         00000000 0000000D 00000000 0000000D

r 3540.10
*Want "CPXT +(DMIN)"         00000000 00010000 00000001 00000000
r 3550.10
*Want "CPXT +(DMIN)"         00010000 00000001 00000000 001C0000
r 3560.10
*Want "CPXT +(DMIN)"         0000001C 00000000 001F0000 0000001F 

r 3570.10
*Want "CPXT -(DMIN)"         00000000 00010000 00000001 00000000
r 3580.10
*Want "CPXT -(DMIN)"         00010000 00000001 00000000 001D0000
r 3590.10
*Want "CPXT -(DMIN)"         0000001D 00000000 001D0000 0000001D 

# Packed Length of 19 --> Program Specification Exception
r 35A0.10
*Want "CPXT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 35B0.10
*Want "CPXT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 35C0.10
*Want "CPXT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 35D0.10
*Want "CPXT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 35E0.10
*Want "CPXT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 35F0.10
*Want "CPXT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 3600.10
*Want "CPXT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 3610.10
*Want "CPXT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 3620.10
*Want "CPXT +3.14159265358979 "    00000000 00000000 00000000 00000000
r 3630.8
*Want "CPXT +3.14159265358979 "    00000000 00000000

# Packed Length of 19 --> Program Specification Exception
r 3638.8
*Want "CPXT -3.14159265358979 "    00000000 00000000
r 3640.10
*Want "CPXT -3.14159265358979 "    00000000 00000000 00000000 00000000
r 3650.10
*Want "CPXT -3.14159265358979 "    00000000 00000000 00000000 00000000
r 3660.10
*Want "CPXT -3.14159265358979 "    00000000 00000000 00000000 00000000
r 3670.10
*Want "CPXT -3.14159265358979 "    00000000 00000000 00000000 00000000
r 3680.10
*Want "CPXT -3.14159265358979 "    00000000 00000000 00000000 00000000
r 3690.10
*Want "CPXT -3.14159265358979 "    00000000 00000000 00000000 00000000
r 36A0.10
*Want "CPXT -3.14159265358979 "    00000000 00000000 00000000 00000000
r 36B0.10
*Want "CPXT -3.14159265358979 "    00000000 00000000 00000000 00000000
r 36C0.10
*Want "CPXT -3.14159265358979 "    00000000 00000000 00000000 00000000

# End of Data marker
r 36D0.10
*Want "CPXT 11111111111111111111111111111111 "    11111111 11111111 11111111 11111111
r 36E0.10
*Want "CPXT 11111111111111111111111111111111 "    11111111 11111111 11111111 11111111
r 36F0.10
*Want "CPXT 11111111111111111111111111111111 "    11111111 11111111 11111111 11111111
r 3700.10
*Want "CPXT 11111111111111111111111111111111 "    11111111 11111111 11111111 11111111
r 3710.10
*Want "CPXT 11111111111111111111111111111111 "    11111111 11111111 11111111 1111111C
r 3720.10
*Want "CPXT 11111111111111111111111111111111 "    11111111 11111111 11111111 1111111C
r 3730.10
*Want "CPXT 11111111111111111111111111111111 "    11111111 11111111 11111111 1111111F
r 3740.10
*Want "CPXT 11111111111111111111111111111111 "    11111111 11111111 11111111 1111111F


# -----------------------------------------------
# CPXT Program Interupt code, DFP condition codes
# -----------------------------------------------
r 3A00.10
*Want "CPXT +0 "          FFFF0000 FFFF0000 FFFF0000 FFFF0000
r 3A10.10
*Want "CPXT +0 "          FFFF0000 FFFF0000 FFFF0000 FFFF0000
r 3A20.10
*Want "CPXT -0 "          FFFF0000 FFFF0000 FFFF0000 FFFF0000
r 3A30.10
*Want "CPXT -0 "          FFFF0000 FFFF0000 FFFF0000 FFFF0000
r 3A40.10
*Want "CPXT +7 "          FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3A50.10
*Want "CPXT +7 "          FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3A60.10
*Want "CPXT -7 "          FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 3A70.10
*Want "CPXT -7 "          FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 3A80.10
*Want "CPXT +12345"       FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3A90.10
*Want "CPXT +12345"       FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3AA0.10
*Want "CPXT -12345"       FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 3AB0.10
*Want "CPXT -12345"       FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 3AC0.10
*Want "CPXT +271828182"   FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3AD0.10
*Want "CPXT +271828182"   FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3AE0.10
*Want "CPXT -271828182"   FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 3AF0.10
*Want "CPXT -271828182"   FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 3B00.10
*Want "CPXT +3.14159265358979"   FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3B10.10
*Want "CPXT +3.14159265358979"   FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3B20.10
*Want "CPXT -3.14159265358979"   FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 3B30.10
*Want "CPXT -3.14159265358979"   FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 3B40.10
*Want "CPXT +0E368"     FFFF0000 FFFF0000 FFFF0000 FFFF0000              
r 3B50.10
*Want "CPXT +0E368"     FFFF0000 FFFF0000 FFFF0000 FFFF0000  
r 3B60.10
*Want "CPXT 0E-397"     FFFF0000 FFFF0000 FFFF0000 FFFF0000  
r 3B70.10
*Want "CPXT 0E-397"     FFFF0000 FFFF0000 FFFF0000 FFFF0000 
r 3B80.10
*Want "CPXT -0E368"     FFFF0000 FFFF0000 FFFF0000 FFFF0000  
r 3B90.10
*Want "CPXT -0E368"     FFFF0000 FFFF0000 FFFF0000 FFFF0000 
r 3BA0.10
*Want "CPXT -0E-397"    FFFF0000 FFFF0000 FFFF0000 FFFF0000  
r 3BB0.10
*Want "CPXT -0E-397"    FFFF0000 FFFF0000 FFFF0000 FFFF0000 
r 3BC0.10
*Want "CPXT +123456789012345E369"   FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3BD0.10
*Want "CPXT +123456789012345E369"   FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3BE0.10
*Want "CPXT +123456789012345E-398"  FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3BF0.10
*Want "CPXT +123456789012345E-398"  FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3C00.10
*Want "CPXT -123456789012345E369"   FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 3C10.10
*Want "CPXT -123456789012345E369"   FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 3C20.10
*Want "CPXT -123456789012345E-398"  FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 3C30.10
*Want "CPXT -123456789012345E-398"  FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 3C40.10
*Want "CPXT +1234567890123456E368"  FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3C50.10
*Want "CPXT +1234567890123456E368"  FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3C60.10
*Want "CPXT +1234567890123456E-397" FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3C70.10
*Want "CPXT +1234567890123456E-397" FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3C80.10
*Want "CPXT -1234567890123456789012345678901234E6110" FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3C90.10
*Want "CPXT -1234567890123456789012345678901234E6110" FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3CA0.10
*Want "CPXT -1234567890123456789012345678901234E-6175" FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3CB0.10
*Want "CPXT -1234567890123456789012345678901234E-6175" FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3CC0.10
*Want "CPXT +(INF)"     FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3CD0.10
*Want "CPXT +(INF)"     FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3CE0.10
*Want "CPXT -(INF)"     FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3CF0.10
*Want "CPXT -(INF)"     FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3D00.10
*Want "CPXT +(QNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3D10.10
*Want "CPXT +(QNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3D20.10
*Want "CPXT -(QNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3D30.10
*Want "CPXT -(QNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003

r 3D40.10
*Want "CPXT +(SNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3D50.10
*Want "CPXT +(SNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3D60.10
*Want "CPXT -(SNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003
r 3D70.10
*Want "CPXT -(SNAN)"    FFFF0003 FFFF0003 FFFF0003 FFFF0003

r 3D80.10
*Want "CPXT +(DMIN)"    FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3D90.10
*Want "CPXT +(DMIN)"    FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3DA0.10
*Want "CPXT -(DMIN)"    FFFF0001 FFFF0001 FFFF0001 FFFF0001
r 3DB0.10
*Want "CPXT -(DMIN)"    FFFF0001 FFFF0001 FFFF0001 FFFF0001

# Packed Length of 19 --> Program Specification Exception
#                         do not check DXC, CC
r 3DC0.2
*Want "CPXT +3.14159265358979 "    0006
r 3DC4.2
*Want "CPXT +3.14159265358979 "    0006
r 3DC8.2
*Want "CPXT +3.14159265358979 "    0006
r 3DCC.2
*Want "CPXT +3.14159265358979 "    0006
r 3DD0.2
*Want "CPXT +3.14159265358979 "    0006
r 3DD4.2
*Want "CPXT +3.14159265358979 "    0006
r 3DD8.2
*Want "CPXT +3.14159265358979 "    0006
r 3DDC.2
*Want "CPXT +3.14159265358979 "    0006

# Packed Length of 19 --> Program Specification Exception
#                         do not check DXC, CC
r 3DE0.2
*Want "CPXT -3.14159265358979 "    0006
r 3DE4.2
*Want "CPXT -3.14159265358979 "    0006
r 3DE8.2
*Want "CPXT -3.14159265358979 "    0006
r 3DEC.2
*Want "CPXT -3.14159265358979 "    0006
r 3DF0.2
*Want "CPXT -3.14159265358979 "    0006
r 3DF4.2
*Want "CPXT -3.14159265358979 "    0006
r 3DF8.2
*Want "CPXT -3.14159265358979 "    0006
r 3DFC.2
*Want "CPXT -3.14159265358979 "    0006


# End of Data marker
r 3E00.10
*Want "CPXT 11111111111111111111111111111111 " FFFF0002 FFFF0002 FFFF0002 FFFF0002
r 3E10.10
*Want "CPXT 11111111111111111111111111111111 " FFFF0003 FFFF0003 FFFF0003 FFFF0003


*Done
