*Testcase bfp-001-divtoint.tst:   DIEBR, DIDBR

#Testcase bfp-001-divtoint.tst: DIVIDE TO INTEGER (2)
#..Test cases evaluate NaN propagation, NaN generation, operations
#..using non-finite values, and exhaustive rounding testing of 
#..operations on finite values.

sysclear
archmode esame

#
# Following suppresses logging of program checks.  This test program, as part
# of its normal operation, generates 46 program check messages that have no
# value in the validation process.
#
ostailor quiet

loadcore "$(testpath)/bfp-001-divtoint.core"

runtest 1.0

ostailor null   # restore messages for subsequent tests


# NaN propagation tests - BFP Short
*Compare
r 1000.10 #                Expecting QNaN A
*Want     "DIEBR test 1 NaN" 7FCA0000 7FCA0000 7F8A0000 00000000
r 1010.10 #                Expecting QNaN A
*Want     "DIEBR test 2 NaN" 7FCA0000 7FCA0000 7FCA0000 7FCA0000
r 1020.10 #                Expecting QNaN B
*Want     "DIEBR test 3 NaN" 7FCB0000 7FCB0000 7FCB0000 7FCB0000
r 1030.10 #                Expecting QNaN B
*Want     "DIEBR test 4 NaN" 7FCB0000 7FCB0000 7FCA0000 00000000


# Dividend -inf - BFP Short - all should be dNaN (a +QNaN)
r 1040.10
*Want     "DIEBR test 5 -inf dividend" 7FC00000 7FC00000 FF800000 00000000
r 1050.10
*Want     "DIEBR test 6 -inf dividend" 7FC00000 7FC00000 FF800000 00000000
r 1060.10
*Want     "DIEBR test 7 -inf dividend" 7FC00000 7FC00000 FF800000 00000000
r 1070.10
*Want     "DIEBR test 8 -inf dividend"   7FC00000 7FC00000 FF800000 00000000
r 1080.10
*Want     "DIEBR test 9 -inf dividend"   7FC00000 7FC00000 FF800000 00000000
r 1090.10
*Want     "DIEBR test 10 -inf dividend"  7FC00000 7FC00000 FF800000 00000000

# Dividend +inf - BFP Short - all should be dNaN (a +QNaN)
r 10A0.10
*Want     "DIEBR test 11 +inf dividend"  7FC00000 7FC00000 7F800000 00000000
r 10B0.10
*Want     "DIEBR test 12 +inf dividend"  7FC00000 7FC00000 7F800000 00000000
r 10C0.10
*Want     "DIEBR test 13 +inf dividend"  7FC00000 7FC00000 7F800000 00000000
r 10D0.10
*Want     "DIEBR test 14 +inf dividend"  7FC00000 7FC00000 7F800000 00000000
r 10E0.10
*Want     "DIEBR test 15 +inf dividend"  7FC00000 7FC00000 7F800000 00000000
r 10F0.10
*Want     "DIEBR test 16 +inf dividend"  7FC00000 7FC00000 7F800000 00000000


# Divisor -0 - BFP Short - all should be dNaN (a +QNaN)
r 1100.10
*Want     "DIEBR test 17 -0 divisor" 7FC00000 7FC00000 C0000000 00000000
r 1110.10
*Want     "DIEBR test 18 -0 divisor" 7FC00000 7FC00000 80000000 00000000
r 1120.10
*Want     "DIEBR test 19 -0 divisor" 7FC00000 7FC00000 00000000 00000000
r 1130.10
*Want     "DIEBR test 20 -0 divisor" 7FC00000 7FC00000 40000000 00000000

# Divisor +0 - BFP Short - all should be dNaN (a +QNaN)
r 1140.10
*Want     "DIEBR test 21 +0 divisor"   7FC00000 7FC00000 C0000000 00000000
r 1150.10
*Want     "DIEBR test 22 +0 divisor"   7FC00000 7FC00000 80000000 00000000
r 1160.10
*Want     "DIEBR test 23 +0 divisor"   7FC00000 7FC00000 00000000 00000000
r 1170.10
*Want     "DIEBR test 24 +0 divisor"   7FC00000 7FC00000 40000000 00000000


# Divisor -inf - BFP Short, remainder dividend, quotient zero, 
r 1180.10
*Want     "DIEBR test 25 -inf divisor" C0000000 00000000 C0000000 00000000
r 1190.10
*Want     "DIEBR test 26 -inf divisor" 80000000 00000000 80000000 00000000
r 11A0.10
*Want     "DIEBR test 27 -inf divisor" 00000000 80000000 00000000 80000000
r 11B0.10
*Want     "DIEBR test 28 -inf divisor" 40000000 80000000 40000000 80000000


# Divisor +inf - BFP Short - all should be dNaN (a +QNaN)
r 11C0.10
*Want     "DIEBR test 29 +inf divisor"   C0000000 80000000 C0000000 80000000
r 11D0.10
*Want     "DIEBR test 30 +inf divisor"   80000000 80000000 80000000 80000000
r 11E0.10
*Want     "DIEBR test 31 +inf divisor"   00000000 00000000 00000000 00000000
r 11F0.10
*Want     "DIEBR test 32 +inf divisor"   40000000 00000000 40000000 00000000

*Compare
# NaN propagation tests - BFP Short - FPCR & cc
r 1200.10 
*Want     "DIEBR FPCR pair NaN 1-2"   00800001 F8008001 00000001 F8000001
r 1210.10 
*Want     "DIEBR FPCR pair NaN 4-4"   00000001 F8000001 00800001 F8008001


# Dividend -inf - BFP Short - FPCR & cc
r 1220.10
*Want     "DIEBR FPCR pair -inf 5-6"   00800001 F8008001 00800001 F8008001
r 1230.10 
*Want     "DIEBR FPCR pair -inf 7-8"   00800001 F8008001 00800001 F8008001
r 1240.10
*Want     "DIEBR FPCR pair -inf 9-10"  00800001 F8008001 00800001 F8008001


# Dividend +inf - BFP Short - FPCR & cc
r 1250.10 
*Want     "DIEBR FPCR pair +inf 11-12" 00800001 F8008001 00800001 F8008001
r 1260.10 
*Want     "DIEBR FPCR pair +inf 13-14" 00800001 F8008001 00800001 F8008001
r 1270.10 
*Want     "DIEBR FPCR pair +inf 15-16" 00800001 F8008001 00800001 F8008001


# Divisor -0 - BFP Short - FPCR & cc
r 1280.10 
*Want     "DIEBR FPCR pair -0 17-18" 00800001 F8008001 00800001 F8008001
r 1290.10 
*Want     "DIEBR FPCR pair -0 19-20" 00800001 F8008001 00800001 F8008001

# Divisor +0 - BFP Short - FPCR & cc
r 12A0.10 
*Want     "DIEBR FPCR pair +0 21-22" 00800001 F8008001 00800001 F8008001
r 12B0.10 
*Want     "DIEBR FPCR pair +0 23-24" 00800001 F8008001 00800001 F8008001


# Divisor -inf - BFP Short - FPCR & cc
r 12C0.10 
*Want     "DIEBR FPCR pair div -inf 25-26" 00000000 F8000000 00000000 F8000000
r 12D0.10 
*Want     "DIEBR FPCR pair div -inf 27-28" 00000000 F8000000 00000000 F8000000

# Divisor +inf - BFP Short - FPCR & cc
r 12E0.10 
*Want     "DIEBR FPCR pair div +inf 29-30" 00000000 F8000000 00000000 F8000000
r 12F0.10 
*Want     "DIEBR FPCR pair div +inf 31-32" 00000000 F8000000 00000000 F8000000



# Dividend & Divisor finite - BFP Short - basic tests
r A000.0C
*Want "DIEBR finite test -8/-4 1a"  80000000 40000000 C1000000
r A010.0C
*Want "DIEBR finite test -8/-4 1b"  80000000 40000000 C1000000
r A020.0C
*Want "DIEBR finite test -7/-4 2a"  3F800000 40000000 C0E00000
r A030.0C
*Want "DIEBR finite test -7/-4 2b"  3F800000 40000000 C0E00000
r A040.0C
*Want "DIEBR finite test -6/-4 3a"  40000000 40000000 C0C00000
r A050.0C
*Want "DIEBR finite test -6/-4 3b"  40000000 40000000 C0C00000
r A060.0C
*Want "DIEBR finite test -5/-4 4a"  BF800000 3F800000 C0A00000
r A070.0C
*Want "DIEBR finite test -5/-4 4b"  BF800000 3F800000 C0A00000
r A080.0C
*Want "DIEBR finite test -4/-4 5a"  80000000 3F800000 C0800000
r A090.0C
*Want "DIEBR finite test -4/-4 5b"  80000000 3F800000 C0800000
r A0A0.0C
*Want "DIEBR finite test -3/-4 6a"  3F800000 3F800000 C0400000
r A0B0.0C
*Want "DIEBR finite test -3/-4 6b"  3F800000 3F800000 C0400000
r A0C0.0C
*Want "DIEBR finite test -2/-4 7a"  C0000000 00000000 C0000000
r A0D0.0C
*Want "DIEBR finite test -2/-4 7b"  C0000000 00000000 C0000000
r A0E0.0C
*Want "DIEBR finite test -1/-4 8a"  BF800000 00000000 BF800000
r A0F0.0C
*Want "DIEBR finite test -1/-4 8b"  BF800000 00000000 BF800000
r A100.0C
*Want "DIEBR finite test +1/-4 9a"  3F800000 80000000 3F800000
r A110.0C
*Want "DIEBR finite test +1/-4 9b"  3F800000 80000000 3F800000
r A120.0C
*Want "DIEBR finite test +2/-4 10a" 40000000 80000000 40000000
r A130.0C
*Want "DIEBR finite test +2/-4 10b" 40000000 80000000 40000000
r A140.0C
*Want "DIEBR finite test +3/-4 11a" BF800000 BF800000 40400000
r A150.0C
*Want "DIEBR finite test +3/-4 11b" BF800000 BF800000 40400000
r A160.0C
*Want "DIEBR finite test +4/-4 12a" 00000000 BF800000 40800000
r A170.0C
*Want "DIEBR finite test +4/-4 12b" 00000000 BF800000 40800000
r A180.0C
*Want "DIEBR finite test +5/-4 13a" 3F800000 BF800000 40A00000
r A190.0C
*Want "DIEBR finite test +5/-4 13b" 3F800000 BF800000 40A00000
r A1A0.0C
*Want "DIEBR finite test +6/-4 14a" C0000000 C0000000 40C00000
r A1B0.0C
*Want "DIEBR finite test +6/-4 14b" C0000000 C0000000 40C00000
r A1C0.0C
*Want "DIEBR finite test 15a" BF800000 C0000000 40E00000
r A1D0.0C
*Want "DIEBR finite test 15b" BF800000 C0000000 40E00000
r A1E0.0C
*Want "DIEBR finite test 16a" 00000000 C0000000 41000000
r A1F0.0C
*Want "DIEBR finite test 16b" 00000000 C0000000 41000000
r A200.0C
*Want "DIEBR finite test 17a" 80000000 C0000000 C1000000
r A210.0C
*Want "DIEBR finite test 17b" 80000000 C0000000 C1000000
r A220.0C
*Want "DIEBR finite test 18a" 3F800000 C0000000 C0E00000
r A230.0C
*Want "DIEBR finite test 18b" 3F800000 C0000000 C0E00000
r A240.0C
*Want "DIEBR finite test 19a" 40000000 C0000000 C0C00000
r A250.0C
*Want "DIEBR finite test 19b" 40000000 C0000000 C0C00000
r A260.0C
*Want "DIEBR finite test 20a" BF800000 BF800000 C0A00000
r A270.0C
*Want "DIEBR finite test 20b" BF800000 BF800000 C0A00000
r A280.0C
*Want "DIEBR finite test 21a" 80000000 BF800000 C0800000
r A290.0C
*Want "DIEBR finite test 21b" 80000000 BF800000 C0800000
r A2A0.0C
*Want "DIEBR finite test 22a" 3F800000 BF800000 C0400000
r A2B0.0C
*Want "DIEBR finite test 22b" 3F800000 BF800000 C0400000
r A2C0.0C
*Want "DIEBR finite test 23a" C0000000 80000000 C0000000
r A2D0.0C
*Want "DIEBR finite test 23b" C0000000 80000000 C0000000
r A2E0.0C
*Want "DIEBR finite test 24a" 3F800000 00000000 3F800000
r A2F0.0C
*Want "DIEBR finite test 24b" 3F800000 00000000 3F800000
r A300.0C
*Want "DIEBR finite test 25a" 3F800000 00000000 3F800000
r A310.0C
*Want "DIEBR finite test 25b" 3F800000 00000000 3F800000
r A320.0C
*Want "DIEBR finite test 26a" 40000000 00000000 40000000
r A330.0C
*Want "DIEBR finite test 26b" 40000000 00000000 40000000
r A340.0C
*Want "DIEBR finite test 27a" BF800000 3F800000 40400000
r A350.0C
*Want "DIEBR finite test 27b" BF800000 3F800000 40400000
r A360.0C
*Want "DIEBR finite test 28a" 00000000 3F800000 40800000
r A370.0C
*Want "DIEBR finite test 28b" 00000000 3F800000 40800000
r A380.0C
*Want "DIEBR finite test 29a" 3F800000 3F800000 40A00000
r A390.0C
*Want "DIEBR finite test 29b" 3F800000 3F800000 40A00000
r A3A0.0C
*Want "DIEBR finite test 30a" C0000000 40000000 40C00000
r A3B0.0C
*Want "DIEBR finite test 30b" C0000000 40000000 40C00000
r A3C0.0C
*Want "DIEBR finite test 31a" BF800000 40000000 40E00000
r A3D0.0C
*Want "DIEBR finite test 31b" BF800000 40000000 40E00000
r A3E0.0C
*Want "DIEBR finite test 32a" 00000000 40000000 41000000
r A3F0.0C
*Want "DIEBR finite test 32b" 00000000 40000000 41000000


r A400.0C   # results validated on z/12
*Want     "DIEBR test 33a two finites"   40800000 C0800000 42200000
r A410.0C   # results validated on z/12
*Want     "DIEBR test 33b two finites"   40800000 C0800000 42200000

r A420.0C   # third result differs from dividend due to scaling of overflowed quotient
#             results validated on z/12
*Want     "DIEBR test 34a two finites"   00000000 69FFFFFF 1F7FFFFF
r A430.0C   # third result differs from dividend due to scaling of overflowed quotient
#             results validated on z/12
*Want     "DIEBR test 34b two finites"   00000000 69FFFFFF 1F7FFFFF

r A440.0C
*Want     "DIEBR test 35a two finites"   00000001 3F800000 00FFFFFF
r A450.0C
#             results validated on z/12
*Want     "DIEBR test 35b two finites"   55000000 3F800000 55000000

r A460.0C
#           # Very big number divided by 3 gives remainder 4. The oddities of partial results
#             results validated on z/12
*Want     "DIEBR test 36a two finites"   40800000 4BAAAAAA 4C800000
r A470.0C
#           # Very big number divided by 3 gives remainder 4. The oddities of partial results
#             results validated on z/12
*Want     "DIEBR test 36b two finites"   40800000 4BAAAAAA 4C800000

r A480.0C
*Want     "DIEBR test 37a two finites"   40100000 00000000 40100000
r A490.0C
*Want     "DIEBR test 37b two finites"   40100000 00000000 40100000

r A4A0.0C   # Not sure following two are correct; need hardware validation
#           # Hyperion results differ in the multiply-add to get the original dividend
#           # *Want reflects z/12 result
*Want     "DIEBR test 38a two finites"   73A00000 683A2E8A 73A00000
#Want     "DIEBR test 38a two finites"   64A00000 683A2E8A 1F7FFFFF  <--what I thought I would get
r A4B0.0C   # Not sure following two are correct; need hardware validation
#           # Hyperion results differ in the multiply-add to get the original dividend
#           # *Want reflects z/12 result
*Want     "DIEBR test 38a two finites"   73A00000 683A2E8A 73A00000
#Want     "DIEBR test 38a two finites"   64A00000 683A2E8A 1F7FFFFF  <--what I thought I would get


*Compare
# Two finites - BFP Short - FPCR & cc (cc unchanged by re-multiply)
r A800.10
*Want "DIEBR FPCR finite test -8/-4 1"  00000000 00000000 F8000000 00000000
r A810.10
*Want "DIEBR FPCR finite test -7/-4 2"  00000000 00000000 F8000000 00000000
r A820.10
*Want "DIEBR FPCR finite test -6/-4 3"  00000000 00000000 F8000000 00000000
r A830.10
*Want "DIEBR FPCR finite test -5/-4 4"  00000000 00000000 F8000000 00000000
r A840.10
*Want "DIEBR FPCR finite test -4/-4 5"  00000000 00000000 F8000000 00000000
r A850.10
*Want "DIEBR FPCR finite test -3/-4 6"  00000000 00000000 F8000000 00000000
r A860.10
*Want "DIEBR FPCR finite test -2/-4 7"  00000000 00000000 F8000000 00000000
r A870.10
*Want "DIEBR FPCR finite test -1/-4 8"  00000000 00000000 F8000000 00000000
r A880.10
*Want "DIEBR FPCR finite test +1/-4 9"  00000000 00000000 F8000000 00000000
r A890.10
*Want "DIEBR FPCR finite test +2/-4 10" 00000000 00000000 F8000000 00000000
r A8A0.10
*Want "DIEBR FPCR finite test +3/-4 11" 00000000 00000000 F8000000 00000000
r A8B0.10
*Want "DIEBR FPCR finite test +4/-4 12" 00000000 00000000 F8000000 00000000
r A8C0.10
*Want "DIEBR FPCR finite test +5/-4 13" 00000000 00000000 F8000000 00000000
r A8D0.10
*Want "DIEBR FPCR finite test +6/-4 14" 00000000 00000000 F8000000 00000000
r A8E0.10
*Want "DIEBR FPCR finite test 15" 00000000 00000000 F8000000 00000000
r A8F0.10
*Want "DIEBR FPCR finite test 16" 00000000 00000000 F8000000 00000000
r A900.10
*Want "DIEBR FPCR finite test 17" 00000000 00000000 F8000000 00000000
r A910.10
*Want "DIEBR FPCR finite test 18" 00000000 00000000 F8000000 00000000
r A920.10
*Want "DIEBR FPCR finite test 19" 00000000 00000000 F8000000 00000000
r A930.10
*Want "DIEBR FPCR finite test 20" 00000000 00000000 F8000000 00000000
r A940.10
*Want "DIEBR FPCR finite test 21" 00000000 00000000 F8000000 00000000
r A950.10
*Want "DIEBR FPCR finite test 22" 00000000 00000000 F8000000 00000000
r A960.10
*Want "DIEBR FPCR finite test 23" 00000000 00000000 F8000000 00000000
r A970.10
*Want "DIEBR FPCR finite test 24" 00000000 00000000 F8000000 00000000
r A980.10
*Want "DIEBR FPCR finite test 25" 00000000 00000000 F8000000 00000000
r A990.10
*Want "DIEBR FPCR finite test 26" 00000000 00000000 F8000000 00000000
r A9A0.10
*Want "DIEBR FPCR finite test 27" 00000000 00000000 F8000000 00000000
r A9B0.10
*Want "DIEBR FPCR finite test 28" 00000000 00000000 F8000000 00000000
r A9C0.10
*Want "DIEBR FPCR finite test 29" 00000000 00000000 F8000000 00000000
r A9D0.10
*Want "DIEBR FPCR finite test 30" 00000000 00000000 F8000000 00000000
r A9E0.10
*Want "DIEBR FPCR finite test 31" 00000000 00000000 F8000000 00000000
r A9F0.10
*Want "DIEBR FPCR finite test 32" 00000000 00000000 F8000000 00000000

r AA00.10 
*Want "DIEBR FPCR finite test 33" 00000000 00000000 F8000000 00000000
r AA10.10 
*Want "DIEBR FPCR finite test 34" 00000001 00000001 F8000001 00000001
r AA20.10  
#            Apparently, on z/12 trappable pair four triggers underflow trap, scaled remainder.   
*Want "DIEBR FPCR finite test 35" 00000000 00000000 F8001000 00080000
r AA30.10  
*Want "DIEBR FPCR finite test 36" 00000002 00000002 F8000002 00000002
r AA40.10  
*Want "DIEBR FPCR finite test 37" 00000000 00000000 F8000000 00000000
r AA50.10
*Want "DIEBR FPCR finite test 38" 00000003 00080003 F8000003 00080003


r 2000.10  #                     RZ / RNTA
*Want "DIEBR Rounding case 1a"  3F800000 C1200000 3F800000 C1200000
r 2010.10  #                     RZ / RFS
*Want "DIEBR Rounding case 1b"  BF800000 C1100000 BF800000 C1100000
r 2020.10  #                     RZ / RNTE
*Want "DIEBR Rounding case 1c"  3F800000 C1200000 3F800000 C1200000
r 2030.10  #                     RZ / RZ
*Want "DIEBR Rounding case 1d"  BF800000 C1100000 BF800000 C1100000
r 2040.10  #                     RZ / RP
*Want "DIEBR Rounding case 1e"  BF800000 C1100000 BF800000 C1100000
r 2050.10  #                     RZ / RM
*Want "DIEBR Rounding case 1f"  3F800000 C1200000 3F800000 C1200000
r 2060.10  #                     RP / RNTA
*Want "DIEBR Rounding case 1g"  3F800000 C1200000 3F800000 C1200000
r 2070.10  #                     RP / RFS
*Want "DIEBR Rounding case 1h"  BF800000 C1100000 BF800000 C1100000
r 2080.10  #                     RP / RNTE
*Want "DIEBR Rounding case 1i"  3F800000 C1200000 3F800000 C1200000
r 2090.10  #                     RP / RZ
*Want "DIEBR Rounding case 1j"  BF800000 C1100000 BF800000 C1100000
r 20A0.10  #                     RP / RP
*Want "DIEBR Rounding case 1k"  BF800000 C1100000 BF800000 C1100000
r 20B0.10  #                     RP / RM
*Want "DIEBR Rounding case 1l"  3F800000 C1200000 3F800000 C1200000
r 20C0.10  #                     RM / RNTA
*Want "DIEBR Rounding case 1m"  3F800000 C1200000 3F800000 C1200000
r 20D0.10  #                     RM / RFS
*Want "DIEBR Rounding case 1n"  BF800000 C1100000 BF800000 C1100000
r 20E0.10  #                     RM / RNTE
*Want "DIEBR Rounding case 1o"  3F800000 C1200000 3F800000 C1200000
r 20F0.10  #                     RM / RZ
*Want "DIEBR Rounding case 1p"  BF800000 C1100000 BF800000 C1100000
r 2100.10  #                     RM / RP
*Want "DIEBR Rounding case 1q"  BF800000 C1100000 BF800000 C1100000
r 2110.10  #                     RM / RM
*Want "DIEBR Rounding case 1r"  3F800000 C1200000 3F800000 C1200000
r 2120.10  #                     RFS / RNTA
*Want "DIEBR Rounding case 1s"  3F800000 C1200000 3F800000 C1200000
r 2130.10  #                     RFS / RFS
*Want "DIEBR Rounding case 1t"  BF800000 C1100000 BF800000 C1100000
r 2140.10  #                     RFS / RNTE
*Want "DIEBR Rounding case 1u"  3F800000 C1200000 3F800000 C1200000
r 2150.10  #                     RFS / RZ
*Want "DIEBR Rounding case 1v"  BF800000 C1100000 BF800000 C1100000
r 2160.10  #                     RFS / RP
*Want "DIEBR Rounding case 1w"  BF800000 C1100000 BF800000 C1100000
r 2170.10  #                     RFS / RM
*Want "DIEBR Rounding case 1x"  3F800000 C1200000 3F800000 C1200000
r 2180.10  #                     RZ / RNTA
*Want "DIEBR Rounding case 2a"  3F800000 C0C00000 3F800000 C0C00000
r 2190.10  #                     RZ / RFS
*Want "DIEBR Rounding case 2b"  BF800000 C0A00000 BF800000 C0A00000
r 21A0.10  #                     RZ / RNTE
*Want "DIEBR Rounding case 2c"  3F800000 C0C00000 3F800000 C0C00000
r 21B0.10  #                     RZ / RZ
*Want "DIEBR Rounding case 2d"  BF800000 C0A00000 BF800000 C0A00000
r 21C0.10  #                     RZ / RP
*Want "DIEBR Rounding case 2e"  BF800000 C0A00000 BF800000 C0A00000
r 21D0.10  #                     RZ / RM
*Want "DIEBR Rounding case 2f"  3F800000 C0C00000 3F800000 C0C00000
r 21E0.10  #                     RP / RNTA
*Want "DIEBR Rounding case 2g"  3F800000 C0C00000 3F800000 C0C00000
r 21F0.10  #                     RP / RFS
*Want "DIEBR Rounding case 2h"  BF800000 C0A00000 BF800000 C0A00000
r 2200.10  #                     RP / RNTE
*Want "DIEBR Rounding case 2i"  3F800000 C0C00000 3F800000 C0C00000
r 2210.10  #                     RP / RZ
*Want "DIEBR Rounding case 2j"  BF800000 C0A00000 BF800000 C0A00000
r 2220.10  #                     RP / RP
*Want "DIEBR Rounding case 2k"  BF800000 C0A00000 BF800000 C0A00000
r 2230.10  #                     RP / RM
*Want "DIEBR Rounding case 2l"  3F800000 C0C00000 3F800000 C0C00000
r 2240.10  #                     RM / RNTA
*Want "DIEBR Rounding case 2m"  3F800000 C0C00000 3F800000 C0C00000
r 2250.10  #                     RM / RFS
*Want "DIEBR Rounding case 2n"  BF800000 C0A00000 BF800000 C0A00000
r 2260.10  #                     RM / RNTE
*Want "DIEBR Rounding case 2o"  3F800000 C0C00000 3F800000 C0C00000
r 2270.10  #                     RM / RZ
*Want "DIEBR Rounding case 2p"  BF800000 C0A00000 BF800000 C0A00000
r 2280.10  #                     RM / RP
*Want "DIEBR Rounding case 2q"  BF800000 C0A00000 BF800000 C0A00000
r 2290.10  #                     RM / RM
*Want "DIEBR Rounding case 2r"  3F800000 C0C00000 3F800000 C0C00000
r 22A0.10  #                     RFS / RNTA
*Want "DIEBR Rounding case 2s"  3F800000 C0C00000 3F800000 C0C00000
r 22B0.10  #                     RFS / RFS
*Want "DIEBR Rounding case 2t"  BF800000 C0A00000 BF800000 C0A00000
r 22C0.10  #                     RFS / RNTE
*Want "DIEBR Rounding case 2u"  3F800000 C0C00000 3F800000 C0C00000
r 22D0.10  #                     RFS / RZ
*Want "DIEBR Rounding case 2v"  BF800000 C0A00000 BF800000 C0A00000
r 22E0.10  #                     RFS / RP
*Want "DIEBR Rounding case 2w"  BF800000 C0A00000 BF800000 C0A00000
r 22F0.10  #                     RFS / RM
*Want "DIEBR Rounding case 2x"  3F800000 C0C00000 3F800000 C0C00000
r 2300.10  #                     RZ / RNTA
*Want "DIEBR Rounding case 3a"  3F800000 C0400000 3F800000 C0400000
r 2310.10  #                     RZ / RFS
*Want "DIEBR Rounding case 3b"  3F800000 C0400000 3F800000 C0400000
r 2320.10  #                     RZ / RNTE
*Want "DIEBR Rounding case 3c"  BF800000 C0000000 BF800000 C0000000
r 2330.10  #                     RZ / RZ
*Want "DIEBR Rounding case 3d"  BF800000 C0000000 BF800000 C0000000
r 2340.10  #                     RZ / RP
*Want "DIEBR Rounding case 3e"  BF800000 C0000000 BF800000 C0000000
r 2350.10  #                     RZ / RM
*Want "DIEBR Rounding case 3f"  3F800000 C0400000 3F800000 C0400000
r 2360.10  #                     RP / RNTA
*Want "DIEBR Rounding case 3g"  3F800000 C0400000 3F800000 C0400000
r 2370.10  #                     RP / RFS
*Want "DIEBR Rounding case 3h"  3F800000 C0400000 3F800000 C0400000
r 2380.10  #                     RP / RNTE
*Want "DIEBR Rounding case 3i"  BF800000 C0000000 BF800000 C0000000
r 2390.10  #                     RP / RZ
*Want "DIEBR Rounding case 3j"  BF800000 C0000000 BF800000 C0000000
r 23A0.10  #                     RP / RP
*Want "DIEBR Rounding case 3k"  BF800000 C0000000 BF800000 C0000000
r 23B0.10  #                     RP / RM
*Want "DIEBR Rounding case 3l"  3F800000 C0400000 3F800000 C0400000
r 23C0.10  #                     RM / RNTA
*Want "DIEBR Rounding case 3m"  3F800000 C0400000 3F800000 C0400000
r 23D0.10  #                     RM / RFS
*Want "DIEBR Rounding case 3n"  3F800000 C0400000 3F800000 C0400000
r 23E0.10  #                     RM / RNTE
*Want "DIEBR Rounding case 3o"  BF800000 C0000000 BF800000 C0000000
r 23F0.10  #                     RM / RZ
*Want "DIEBR Rounding case 3p"  BF800000 C0000000 BF800000 C0000000
r 2400.10  #                     RM / RP
*Want "DIEBR Rounding case 3q"  BF800000 C0000000 BF800000 C0000000
r 2410.10  #                     RM / RM
*Want "DIEBR Rounding case 3r"  3F800000 C0400000 3F800000 C0400000
r 2420.10  #                     RFS / RNTA
*Want "DIEBR Rounding case 3s"  3F800000 C0400000 3F800000 C0400000
r 2430.10  #                     RFS / RFS
*Want "DIEBR Rounding case 3t"  3F800000 C0400000 3F800000 C0400000
r 2440.10  #                     RFS / RNTE
*Want "DIEBR Rounding case 3u"  BF800000 C0000000 BF800000 C0000000
r 2450.10  #                     RFS / RZ
*Want "DIEBR Rounding case 3v"  BF800000 C0000000 BF800000 C0000000
r 2460.10  #                     RFS / RP
*Want "DIEBR Rounding case 3w"  BF800000 C0000000 BF800000 C0000000
r 2470.10  #                     RFS / RM
*Want "DIEBR Rounding case 3x"  3F800000 C0400000 3F800000 C0400000
r 2480.10  #                     RZ / RNTA
*Want "DIEBR Rounding case 4a"  3F800000 C0000000 3F800000 C0000000
r 2490.10  #                     RZ / RFS
*Want "DIEBR Rounding case 4b"  BF800000 BF800000 BF800000 BF800000
r 24A0.10  #                     RZ / RNTE
*Want "DIEBR Rounding case 4c"  3F800000 C0000000 3F800000 C0000000
r 24B0.10  #                     RZ / RZ
*Want "DIEBR Rounding case 4d"  BF800000 BF800000 BF800000 BF800000
r 24C0.10  #                     RZ / RP
*Want "DIEBR Rounding case 4e"  BF800000 BF800000 BF800000 BF800000
r 24D0.10  #                     RZ / RM
*Want "DIEBR Rounding case 4f"  3F800000 C0000000 3F800000 C0000000
r 24E0.10  #                     RP / RNTA
*Want "DIEBR Rounding case 4g"  3F800000 C0000000 3F800000 C0000000
r 24F0.10  #                     RP / RFS
*Want "DIEBR Rounding case 4h"  BF800000 BF800000 BF800000 BF800000
r 2500.10  #                     RP / RNTE
*Want "DIEBR Rounding case 4i"  3F800000 C0000000 3F800000 C0000000
r 2510.10  #                     RP / RZ
*Want "DIEBR Rounding case 4j"  BF800000 BF800000 BF800000 BF800000
r 2520.10  #                     RP / RP
*Want "DIEBR Rounding case 4k"  BF800000 BF800000 BF800000 BF800000
r 2530.10  #                     RP / RM
*Want "DIEBR Rounding case 4l"  3F800000 C0000000 3F800000 C0000000
r 2540.10  #                     RM / RNTA
*Want "DIEBR Rounding case 4m"  3F800000 C0000000 3F800000 C0000000
r 2550.10  #                     RM / RFS
*Want "DIEBR Rounding case 4n"  BF800000 BF800000 BF800000 BF800000
r 2560.10  #                     RM / RNTE
*Want "DIEBR Rounding case 4o"  3F800000 C0000000 3F800000 C0000000
r 2570.10  #                     RM / RZ
*Want "DIEBR Rounding case 4p"  BF800000 BF800000 BF800000 BF800000
r 2580.10  #                     RM / RP
*Want "DIEBR Rounding case 4q"  BF800000 BF800000 BF800000 BF800000
r 2590.10  #                     RM / RM
*Want "DIEBR Rounding case 4r"  3F800000 C0000000 3F800000 C0000000
r 25A0.10  #                     RFS / RNTA
*Want "DIEBR Rounding case 4s"  3F800000 C0000000 3F800000 C0000000
r 25B0.10  #                     RFS / RFS
*Want "DIEBR Rounding case 4t"  BF800000 BF800000 BF800000 BF800000
r 25C0.10  #                     RFS / RNTE
*Want "DIEBR Rounding case 4u"  3F800000 C0000000 3F800000 C0000000
r 25D0.10  #                     RFS / RZ
*Want "DIEBR Rounding case 4v"  BF800000 BF800000 BF800000 BF800000
r 25E0.10  #                     RFS / RP
*Want "DIEBR Rounding case 4w"  BF800000 BF800000 BF800000 BF800000
r 25F0.10  #                     RFS / RM
*Want "DIEBR Rounding case 4x"  3F800000 C0000000 3F800000 C0000000
r 2600.10  #                     RZ / RNTA
*Want "DIEBR Rounding case 5a"  3F800000 BF800000 3F800000 BF800000
r 2610.10  #                     RZ / RFS
*Want "DIEBR Rounding case 5b"  3F800000 BF800000 3F800000 BF800000
r 2620.10  #                     RZ / RNTE
*Want "DIEBR Rounding case 5c"  BF800000 80000000 BF800000 80000000
r 2630.10  #                     RZ / RZ
*Want "DIEBR Rounding case 5d"  BF800000 80000000 BF800000 80000000
r 2640.10  #                     RZ / RP
*Want "DIEBR Rounding case 5e"  BF800000 80000000 BF800000 80000000
r 2650.10  #                     RZ / RM
*Want "DIEBR Rounding case 5f"  3F800000 BF800000 3F800000 BF800000
r 2660.10  #                     RP / RNTA
*Want "DIEBR Rounding case 5g"  3F800000 BF800000 3F800000 BF800000
r 2670.10  #                     RP / RFS
*Want "DIEBR Rounding case 5h"  3F800000 BF800000 3F800000 BF800000
r 2680.10  #                     RP / RNTE
*Want "DIEBR Rounding case 5i"  BF800000 80000000 BF800000 80000000
r 2690.10  #                     RP / RZ
*Want "DIEBR Rounding case 5j"  BF800000 80000000 BF800000 80000000
r 26A0.10  #                     RP / RP
*Want "DIEBR Rounding case 5k"  BF800000 80000000 BF800000 80000000
r 26B0.10  #                     RP / RM
*Want "DIEBR Rounding case 5l"  3F800000 BF800000 3F800000 BF800000
r 26C0.10  #                     RM / RNTA
*Want "DIEBR Rounding case 5m"  3F800000 BF800000 3F800000 BF800000
r 26D0.10  #                     RM / RFS
*Want "DIEBR Rounding case 5n"  3F800000 BF800000 3F800000 BF800000
r 26E0.10  #                     RM / RNTE
*Want "DIEBR Rounding case 5o"  BF800000 80000000 BF800000 80000000
r 26F0.10  #                     RM / RZ
*Want "DIEBR Rounding case 5p"  BF800000 80000000 BF800000 80000000
r 2700.10  #                     RM / RP
*Want "DIEBR Rounding case 5q"  BF800000 80000000 BF800000 80000000
r 2710.10  #                     RM / RM
*Want "DIEBR Rounding case 5r"  3F800000 BF800000 3F800000 BF800000
r 2720.10  #                     RFS / RNTA
*Want "DIEBR Rounding case 5s"  3F800000 BF800000 3F800000 BF800000
r 2730.10  #                     RFS / RFS
*Want "DIEBR Rounding case 5t"  3F800000 BF800000 3F800000 BF800000
r 2740.10  #                     RFS / RNTE
*Want "DIEBR Rounding case 5u"  BF800000 80000000 BF800000 80000000
r 2750.10  #                     RFS / RZ
*Want "DIEBR Rounding case 5v"  BF800000 80000000 BF800000 80000000
r 2760.10  #                     RFS / RP
*Want "DIEBR Rounding case 5w"  BF800000 80000000 BF800000 80000000
r 2770.10  #                     RFS / RM
*Want "DIEBR Rounding case 5x"  3F800000 BF800000 3F800000 BF800000
r 2780.10  #                     RZ / RNTA
*Want "DIEBR Rounding case 6a"  BF800000 3F800000 BF800000 3F800000
r 2790.10  #                     RZ / RFS
*Want "DIEBR Rounding case 6b"  BF800000 3F800000 BF800000 3F800000
r 27A0.10  #                     RZ / RNTE
*Want "DIEBR Rounding case 6c"  3F800000 00000000 3F800000 00000000
r 27B0.10  #                     RZ / RZ
*Want "DIEBR Rounding case 6d"  3F800000 00000000 3F800000 00000000
r 27C0.10  #                     RZ / RP
*Want "DIEBR Rounding case 6e"  BF800000 3F800000 BF800000 3F800000
r 27D0.10  #                     RZ / RM
*Want "DIEBR Rounding case 6f"  3F800000 00000000 3F800000 00000000
r 27E0.10  #                     RP / RNTA
*Want "DIEBR Rounding case 6g"  BF800000 3F800000 BF800000 3F800000
r 27F0.10  #                     RP / RFS
*Want "DIEBR Rounding case 6h"  BF800000 3F800000 BF800000 3F800000
r 2800.10  #                     RP / RNTE
*Want "DIEBR Rounding case 6i"  3F800000 00000000 3F800000 00000000
r 2810.10  #                     RP / RZ
*Want "DIEBR Rounding case 6j"  3F800000 00000000 3F800000 00000000
r 2820.10  #                     RP / RP
*Want "DIEBR Rounding case 6k"  BF800000 3F800000 BF800000 3F800000
r 2830.10  #                     RP / RM
*Want "DIEBR Rounding case 6l"  3F800000 00000000 3F800000 00000000
r 2840.10  #                     RM / RNTA
*Want "DIEBR Rounding case 6m"  BF800000 3F800000 BF800000 3F800000
r 2850.10  #                     RM / RFS
*Want "DIEBR Rounding case 6n"  BF800000 3F800000 BF800000 3F800000
r 2860.10  #                     RM / RNTE
*Want "DIEBR Rounding case 6o"  3F800000 00000000 3F800000 00000000
r 2870.10  #                     RM / RZ
*Want "DIEBR Rounding case 6p"  3F800000 00000000 3F800000 00000000
r 2880.10  #                     RM / RP
*Want "DIEBR Rounding case 6q"  BF800000 3F800000 BF800000 3F800000
r 2890.10  #                     RM / RM
*Want "DIEBR Rounding case 6r"  3F800000 00000000 3F800000 00000000
r 28A0.10  #                     RFS / RNTA
*Want "DIEBR Rounding case 6s"  BF800000 3F800000 BF800000 3F800000
r 28B0.10  #                     RFS / RFS
*Want "DIEBR Rounding case 6t"  BF800000 3F800000 BF800000 3F800000
r 28C0.10  #                     RFS / RNTE
*Want "DIEBR Rounding case 6u"  3F800000 00000000 3F800000 00000000
r 28D0.10  #                     RFS / RZ
*Want "DIEBR Rounding case 6v"  3F800000 00000000 3F800000 00000000
r 28E0.10  #                     RFS / RP
*Want "DIEBR Rounding case 6w"  BF800000 3F800000 BF800000 3F800000
r 28F0.10  #                     RFS / RM
*Want "DIEBR Rounding case 6x"  3F800000 00000000 3F800000 00000000
r 2900.10  #                     RZ / RNTA
*Want "DIEBR Rounding case 7a"  BF800000 40000000 BF800000 40000000
r 2910.10  #                     RZ / RFS
*Want "DIEBR Rounding case 7b"  3F800000 3F800000 3F800000 3F800000
r 2920.10  #                     RZ / RNTE
*Want "DIEBR Rounding case 7c"  BF800000 40000000 BF800000 40000000
r 2930.10  #                     RZ / RZ
*Want "DIEBR Rounding case 7d"  3F800000 3F800000 3F800000 3F800000
r 2940.10  #                     RZ / RP
*Want "DIEBR Rounding case 7e"  BF800000 40000000 BF800000 40000000
r 2950.10  #                     RZ / RM
*Want "DIEBR Rounding case 7f"  3F800000 3F800000 3F800000 3F800000
r 2960.10  #                     RP / RNTA
*Want "DIEBR Rounding case 7g"  BF800000 40000000 BF800000 40000000
r 2970.10  #                     RP / RFS
*Want "DIEBR Rounding case 7h"  3F800000 3F800000 3F800000 3F800000
r 2980.10  #                     RP / RNTE
*Want "DIEBR Rounding case 7i"  BF800000 40000000 BF800000 40000000
r 2990.10  #                     RP / RZ
*Want "DIEBR Rounding case 7j"  3F800000 3F800000 3F800000 3F800000
r 29A0.10  #                     RP / RP
*Want "DIEBR Rounding case 7k"  BF800000 40000000 BF800000 40000000
r 29B0.10  #                     RP / RM
*Want "DIEBR Rounding case 7l"  3F800000 3F800000 3F800000 3F800000
r 29C0.10  #                     RM / RNTA
*Want "DIEBR Rounding case 7m"  BF800000 40000000 BF800000 40000000
r 29D0.10  #                     RM / RFS
*Want "DIEBR Rounding case 7n"  3F800000 3F800000 3F800000 3F800000
r 29E0.10  #                     RM / RNTE
*Want "DIEBR Rounding case 7o"  BF800000 40000000 BF800000 40000000
r 29F0.10  #                     RM / RZ
*Want "DIEBR Rounding case 7p"  3F800000 3F800000 3F800000 3F800000
r 2A00.10  #                     RM / RP
*Want "DIEBR Rounding case 7q"  BF800000 40000000 BF800000 40000000
r 2A10.10  #                     RM / RM
*Want "DIEBR Rounding case 7r"  3F800000 3F800000 3F800000 3F800000
r 2A20.10  #                     RFS / RNTA
*Want "DIEBR Rounding case 7s"  BF800000 40000000 BF800000 40000000
r 2A30.10  #                     RFS / RFS
*Want "DIEBR Rounding case 7t"  3F800000 3F800000 3F800000 3F800000
r 2A40.10  #                     RFS / RNTE
*Want "DIEBR Rounding case 7u"  BF800000 40000000 BF800000 40000000
r 2A50.10  #                     RFS / RZ
*Want "DIEBR Rounding case 7v"  3F800000 3F800000 3F800000 3F800000
r 2A60.10  #                     RFS / RP
*Want "DIEBR Rounding case 7w"  BF800000 40000000 BF800000 40000000
r 2A70.10  #                     RFS / RM
*Want "DIEBR Rounding case 7x"  3F800000 3F800000 3F800000 3F800000
r 2A80.10  #                     RZ / RNTA
*Want "DIEBR Rounding case 8a"  BF800000 40400000 BF800000 40400000
r 2A90.10  #                     RZ / RFS
*Want "DIEBR Rounding case 8b"  BF800000 40400000 BF800000 40400000
r 2AA0.10  #                     RZ / RNTE
*Want "DIEBR Rounding case 8c"  3F800000 40000000 3F800000 40000000
r 2AB0.10  #                     RZ / RZ
*Want "DIEBR Rounding case 8d"  3F800000 40000000 3F800000 40000000
r 2AC0.10  #                     RZ / RP
*Want "DIEBR Rounding case 8e"  BF800000 40400000 BF800000 40400000
r 2AD0.10  #                     RZ / RM
*Want "DIEBR Rounding case 8f"  3F800000 40000000 3F800000 40000000
r 2AE0.10  #                     RP / RNTA
*Want "DIEBR Rounding case 8g"  BF800000 40400000 BF800000 40400000
r 2AF0.10  #                     RP / RFS
*Want "DIEBR Rounding case 8h"  BF800000 40400000 BF800000 40400000
r 2B00.10  #                     RP / RNTE
*Want "DIEBR Rounding case 8i"  3F800000 40000000 3F800000 40000000
r 2B10.10  #                     RP / RZ
*Want "DIEBR Rounding case 8j"  3F800000 40000000 3F800000 40000000
r 2B20.10  #                     RP / RP
*Want "DIEBR Rounding case 8k"  BF800000 40400000 BF800000 40400000
r 2B30.10  #                     RP / RM
*Want "DIEBR Rounding case 8l"  3F800000 40000000 3F800000 40000000
r 2B40.10  #                     RM / RNTA
*Want "DIEBR Rounding case 8m"  BF800000 40400000 BF800000 40400000
r 2B50.10  #                     RM / RFS
*Want "DIEBR Rounding case 8n"  BF800000 40400000 BF800000 40400000
r 2B60.10  #                     RM / RNTE
*Want "DIEBR Rounding case 8o"  3F800000 40000000 3F800000 40000000
r 2B70.10  #                     RM / RZ
*Want "DIEBR Rounding case 8p"  3F800000 40000000 3F800000 40000000
r 2B80.10  #                     RM / RP
*Want "DIEBR Rounding case 8q"  BF800000 40400000 BF800000 40400000
r 2B90.10  #                     RM / RM
*Want "DIEBR Rounding case 8r"  3F800000 40000000 3F800000 40000000
r 2BA0.10  #                     RFS / RNTA
*Want "DIEBR Rounding case 8s"  BF800000 40400000 BF800000 40400000
r 2BB0.10  #                     RFS / RFS
*Want "DIEBR Rounding case 8t"  BF800000 40400000 BF800000 40400000
r 2BC0.10  #                     RFS / RNTE
*Want "DIEBR Rounding case 8u"  3F800000 40000000 3F800000 40000000
r 2BD0.10  #                     RFS / RZ
*Want "DIEBR Rounding case 8v"  3F800000 40000000 3F800000 40000000
r 2BE0.10  #                     RFS / RP
*Want "DIEBR Rounding case 8w"  BF800000 40400000 BF800000 40400000
r 2BF0.10  #                     RFS / RM
*Want "DIEBR Rounding case 8x"  3F800000 40000000 3F800000 40000000
r 2C00.10  #                     RZ / RNTA
*Want "DIEBR Rounding case 9a"  BF800000 40C00000 BF800000 40C00000
r 2C10.10  #                     RZ / RFS
*Want "DIEBR Rounding case 9b"  3F800000 40A00000 3F800000 40A00000
r 2C20.10  #                     RZ / RNTE
*Want "DIEBR Rounding case 9c"  BF800000 40C00000 BF800000 40C00000
r 2C30.10  #                     RZ / RZ
*Want "DIEBR Rounding case 9d"  3F800000 40A00000 3F800000 40A00000
r 2C40.10  #                     RZ / RP
*Want "DIEBR Rounding case 9e"  BF800000 40C00000 BF800000 40C00000
r 2C50.10  #                     RZ / RM
*Want "DIEBR Rounding case 9f"  3F800000 40A00000 3F800000 40A00000
r 2C60.10  #                     RP / RNTA
*Want "DIEBR Rounding case 9g"  BF800000 40C00000 BF800000 40C00000
r 2C70.10  #                     RP / RFS
*Want "DIEBR Rounding case 9h"  3F800000 40A00000 3F800000 40A00000
r 2C80.10  #                     RP / RNTE
*Want "DIEBR Rounding case 9i"  BF800000 40C00000 BF800000 40C00000
r 2C90.10  #                     RP / RZ
*Want "DIEBR Rounding case 9j"  3F800000 40A00000 3F800000 40A00000
r 2CA0.10  #                     RP / RP
*Want "DIEBR Rounding case 9k"  BF800000 40C00000 BF800000 40C00000
r 2CB0.10  #                     RP / RM
*Want "DIEBR Rounding case 9l"  3F800000 40A00000 3F800000 40A00000
r 2CC0.10  #                     RM / RNTA
*Want "DIEBR Rounding case 9m"  BF800000 40C00000 BF800000 40C00000
r 2CD0.10  #                     RM / RFS
*Want "DIEBR Rounding case 9n"  3F800000 40A00000 3F800000 40A00000
r 2CE0.10  #                     RM / RNTE
*Want "DIEBR Rounding case 9o"  BF800000 40C00000 BF800000 40C00000
r 2CF0.10  #                     RM / RZ
*Want "DIEBR Rounding case 9p"  3F800000 40A00000 3F800000 40A00000
r 2D00.10  #                     RM / RP
*Want "DIEBR Rounding case 9q"  BF800000 40C00000 BF800000 40C00000
r 2D10.10  #                     RM / RM
*Want "DIEBR Rounding case 9r"  3F800000 40A00000 3F800000 40A00000
r 2D20.10  #                     RFS / RNTA
*Want "DIEBR Rounding case 9s"  BF800000 40C00000 BF800000 40C00000
r 2D30.10  #                     RFS / RFS
*Want "DIEBR Rounding case 9t"  3F800000 40A00000 3F800000 40A00000
r 2D40.10  #                     RFS / RNTE
*Want "DIEBR Rounding case 9u"  BF800000 40C00000 BF800000 40C00000
r 2D50.10  #                     RFS / RZ
*Want "DIEBR Rounding case 9v"  3F800000 40A00000 3F800000 40A00000
r 2D60.10  #                     RFS / RP
*Want "DIEBR Rounding case 9w"  BF800000 40C00000 BF800000 40C00000
r 2D70.10  #                     RFS / RM
*Want "DIEBR Rounding case 9x"  3F800000 40A00000 3F800000 40A00000
r 2D80.10  #                     RZ / RNTA
*Want "DIEBR Rounding case 10a" BF800000 41200000 BF800000 41200000
r 2D90.10  #                     RZ / RFS
*Want "DIEBR Rounding case 10b" 3F800000 41100000 3F800000 41100000
r 2DA0.10  #                     RZ / RNTE
*Want "DIEBR Rounding case 10c" BF800000 41200000 BF800000 41200000
r 2DB0.10  #                     RZ / RZ
*Want "DIEBR Rounding case 10d" 3F800000 41100000 3F800000 41100000
r 2DC0.10  #                     RZ / RP
*Want "DIEBR Rounding case 10e" BF800000 41200000 BF800000 41200000
r 2DD0.10  #                     RZ / RM
*Want "DIEBR Rounding case 10f" 3F800000 41100000 3F800000 41100000
r 2DE0.10  #                     RP / RNTA
*Want "DIEBR Rounding case 10g" BF800000 41200000 BF800000 41200000
r 2DF0.10  #                     RP / RFS
*Want "DIEBR Rounding case 10h" 3F800000 41100000 3F800000 41100000
r 2E00.10  #                     RP / RNTE
*Want "DIEBR Rounding case 10i" BF800000 41200000 BF800000 41200000
r 2E10.10  #                     RP / RZ
*Want "DIEBR Rounding case 10j" 3F800000 41100000 3F800000 41100000
r 2E20.10  #                     RP / RP
*Want "DIEBR Rounding case 10k" BF800000 41200000 BF800000 41200000
r 2E30.10  #                     RP / RM
*Want "DIEBR Rounding case 10l" 3F800000 41100000 3F800000 41100000
r 2E40.10  #                     RM / RNTA
*Want "DIEBR Rounding case 10m" BF800000 41200000 BF800000 41200000
r 2E50.10  #                     RM / RFS
*Want "DIEBR Rounding case 10n" 3F800000 41100000 3F800000 41100000
r 2E60.10  #                     RM / RNTE
*Want "DIEBR Rounding case 10o" BF800000 41200000 BF800000 41200000
r 2E70.10  #                     RM / RZ
*Want "DIEBR Rounding case 10p" 3F800000 41100000 3F800000 41100000
r 2E80.10  #                     RM / RP
*Want "DIEBR Rounding case 10q" BF800000 41200000 BF800000 41200000
r 2E90.10  #                     RM / RM
*Want "DIEBR Rounding case 10r" 3F800000 41100000 3F800000 41100000
r 2EA0.10  #                     RFS / RNTA
*Want "DIEBR Rounding case 10s" BF800000 41200000 BF800000 41200000
r 2EB0.10  #                     RFS / RFS
*Want "DIEBR Rounding case 10t" 3F800000 41100000 3F800000 41100000
r 2EC0.10  #                     RFS / RNTE
*Want "DIEBR Rounding case 10u" BF800000 41200000 BF800000 41200000
r 2ED0.10  #                     RFS / RZ
*Want "DIEBR Rounding case 10v" 3F800000 41100000 3F800000 41100000
r 2EE0.10  #                     RFS / RP
*Want "DIEBR Rounding case 10w" BF800000 41200000 BF800000 41200000
r 2EF0.10  #                     RFS / RM
*Want "DIEBR Rounding case 10x" 3F800000 41100000 3F800000 41100000
r 2F00.10  #                     RZ / RNTA
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2F10.10  #                     RZ / RFS
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2F20.10  #                     RZ / RNTE
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2F30.10  #                     RZ / RZ
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2F40.10  #                     RZ / RP
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2F50.10  #                     RZ / RM
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2F60.10  #                     RP / RNTA
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2F70.10  #                     RP / RFS
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2F80.10  #                     RP / RNTE
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2F90.10  #                     RP / RZ
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2FA0.10  #                     RP / RP
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2FB0.10  #                     RP / RM
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2FC0.10  #                     RM / RNTA
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2FD0.10  #                     RM / RFS
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2FE0.10  #                     RM / RNTE
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 2FF0.10  #                     RM / RZ
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 3000.10  #                     RM / RP
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 3010.10  #                     RM / RM
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 3020.10  #                     RFS / RNTA
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 3030.10  #                     RFS / RFS
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 3040.10  #                     RFS / RNTE
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 3050.10  #                     RFS / RZ
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 3060.10  #                     RFS / RP
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 3070.10  #                     RFS / RM
*Want "DIEBR Rounding case 11"  00000000 3F800000 00000000 3F800000
r 3080.10  #                     RZ / RNTA
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 3090.10  #                     RZ / RFS
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 30A0.10  #                     RZ / RNTE
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 30B0.10  #                     RZ / RZ
*Want "DIEBR Rounding case 12"  40400000 00000000 40400000 00000000
r 30C0.10  #                     RZ / RP
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 30D0.10  #                     RZ / RM
*Want "DIEBR Rounding case 12"  40400000 00000000 40400000 00000000
r 30E0.10  #                     RP / RNTA
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 30F0.10  #                     RP / RFS
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 3100.10  #                     RP / RNTE
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 3110.10  #                     RP / RZ
*Want "DIEBR Rounding case 12"  40400000 00000000 40400000 00000000
r 3120.10  #                     RP / RP
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 3130.10  #                     RP / RM
*Want "DIEBR Rounding case 12"  40400000 00000000 40400000 00000000
r 3140.10  #                     RM / RNTA
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 3150.10  #                     RM / RFS
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 3160.10  #                     RM / RNTE
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 3170.10  #                     RM / RZ
*Want "DIEBR Rounding case 12"  40400000 00000000 40400000 00000000
r 3180.10  #                     RM / RP
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 3190.10  #                     RM / RM
*Want "DIEBR Rounding case 12"  40400000 00000000 40400000 00000000
r 31A0.10  #                     RFS / RNTA
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 31B0.10  #                     RFS / RFS
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 31C0.10  #                     RFS / RNTE
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 31D0.10  #                     RFS / RZ
*Want "DIEBR Rounding case 12"  40400000 00000000 40400000 00000000
r 31E0.10  #                     RFS / RP
*Want "DIEBR Rounding case 12"  C0000000 3F800000 C0000000 3F800000
r 31F0.10  #                     RFS / RM
*Want "DIEBR Rounding case 12"  40400000 00000000 40400000 00000000




*Compare
# Short BFP exhaustive rounding mode tests
r 4000.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIEBR Rounding FPCR 1ab"  00000000 F8000000 00000000 F8000000
r 4010.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIEBR Rounding FPCR 1cd"  00000000 F8000000 00000000 F8000000
r 4020.10  #                        RZ / RP           RZ / RM
*Want    "DIEBR Rounding FPCR 1ef"  00000000 F8000000 00000000 F8000000
r 4030.10  #                        RP / RNTA         RP / RFS
*Want    "DIEBR Rounding FPCR 1gh"  00000000 F8000000 00000000 F8000000
r 4040.10  #                        RP / RNTE         RP / RZ
*Want    "DIEBR Rounding FPCR 1ij"  00000000 F8000000 00000000 F8000000
r 4050.10  #                        RP / RP           RP / RM
*Want    "DIEBR Rounding FPCR 1kl"  00000000 F8000000 00000000 F8000000
r 4060.10  #                        RM / RNTA         RM / RFS
*Want    "DIEBR Rounding FPCR 1mn"  00000000 F8000000 00000000 F8000000
r 4070.10  #                        RM / RNTE         RM / RZ
*Want    "DIEBR Rounding FPCR 1op"  00000000 F8000000 00000000 F8000000
r 4080.10  #                        RM / RP           RM / RM
*Want    "DIEBR Rounding FPCR 1qr"  00000000 F8000000 00000000 F8000000
r 4090.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIEBR Rounding FPCR 1st"  00000000 F8000000 00000000 F8000000
r 40A0.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIEBR Rounding FPCR 1uv"  00000000 F8000000 00000000 F8000000
r 40B0.10  #                        RFS / RP          RFS / RM
*Want    "DIEBR Rounding FPCR 1wx"  00000000 F8000000 00000000 F8000000
r 40C0.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIEBR Rounding FPCR 2ab"  00000000 F8000000 00000000 F8000000
r 40D0.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIEBR Rounding FPCR 2cd"  00000000 F8000000 00000000 F8000000
r 40E0.10  #                        RZ / RP           RZ / RM
*Want    "DIEBR Rounding FPCR 2ef"  00000000 F8000000 00000000 F8000000
r 40F0.10  #                        RP / RNTA         RP / RFS
*Want    "DIEBR Rounding FPCR 2gh"  00000000 F8000000 00000000 F8000000
r 4100.10  #                        RP / RNTE         RP / RZ
*Want    "DIEBR Rounding FPCR 2ij"  00000000 F8000000 00000000 F8000000
r 4110.10  #                        RP / RP           RP / RM
*Want    "DIEBR Rounding FPCR 2kl"  00000000 F8000000 00000000 F8000000
r 4120.10  #                        RM / RNTA         RM / RFS
*Want    "DIEBR Rounding FPCR 2mn"  00000000 F8000000 00000000 F8000000
r 4130.10  #                        RM / RNTE         RM / RZ
*Want    "DIEBR Rounding FPCR 2op"  00000000 F8000000 00000000 F8000000
r 4140.10  #                        RM / RP           RM / RM
*Want    "DIEBR Rounding FPCR 2qr"  00000000 F8000000 00000000 F8000000
r 4150.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIEBR Rounding FPCR 2st"  00000000 F8000000 00000000 F8000000
r 4160.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIEBR Rounding FPCR 2uv"  00000000 F8000000 00000000 F8000000
r 4170.10  #                        RFS / RP          RFS / RM
*Want    "DIEBR Rounding FPCR 2wx"  00000000 F8000000 00000000 F8000000
r 4180.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIEBR Rounding FPCR 3ab"  00000000 F8000000 00000000 F8000000
r 4190.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIEBR Rounding FPCR 3cd"  00000000 F8000000 00000000 F8000000
r 41A0.10  #                        RZ / RP           RZ / RM
*Want    "DIEBR Rounding FPCR 3ef"  00000000 F8000000 00000000 F8000000
r 41B0.10  #                        RP / RNTA         RP / RFS
*Want    "DIEBR Rounding FPCR 3gh"  00000000 F8000000 00000000 F8000000
r 41C0.10  #                        RP / RNTE         RP / RZ
*Want    "DIEBR Rounding FPCR 3ij"  00000000 F8000000 00000000 F8000000
r 41D0.10  #                        RP / RP           RP / RM
*Want    "DIEBR Rounding FPCR 3kl"  00000000 F8000000 00000000 F8000000
r 41E0.10  #                        RM / RNTA         RM / RFS
*Want    "DIEBR Rounding FPCR 3mn"  00000000 F8000000 00000000 F8000000
r 41F0.10  #                        RM / RNTE         RM / RZ
*Want    "DIEBR Rounding FPCR 3op"  00000000 F8000000 00000000 F8000000
r 4200.10  #                        RM / RP           RM / RM
*Want    "DIEBR Rounding FPCR 3qr"  00000000 F8000000 00000000 F8000000
r 4210.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIEBR Rounding FPCR 3st"  00000000 F8000000 00000000 F8000000
r 4220.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIEBR Rounding FPCR 3uv"  00000000 F8000000 00000000 F8000000
r 4230.10  #                        RFS / RP          RFS / RM
*Want    "DIEBR Rounding FPCR 3wx"  00000000 F8000000 00000000 F8000000
r 4240.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIEBR Rounding FPCR 4ab"  00000000 F8000000 00000000 F8000000
r 4250.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIEBR Rounding FPCR 4cd"  00000000 F8000000 00000000 F8000000
r 4260.10  #                        RZ / RP           RZ / RM
*Want    "DIEBR Rounding FPCR 4ef"  00000000 F8000000 00000000 F8000000
r 4270.10  #                        RP / RNTA         RP / RFS
*Want    "DIEBR Rounding FPCR 4gh"  00000000 F8000000 00000000 F8000000
r 4280.10  #                        RP / RNTE         RP / RZ
*Want    "DIEBR Rounding FPCR 4ij"  00000000 F8000000 00000000 F8000000
r 4290.10  #                        RP / RP           RP / RM
*Want    "DIEBR Rounding FPCR 4kl"  00000000 F8000000 00000000 F8000000
r 42A0.10  #                        RM / RNTA         RM / RFS
*Want    "DIEBR Rounding FPCR 4mn"  00000000 F8000000 00000000 F8000000
r 42B0.10  #                        RM / RNTE         RM / RZ
*Want    "DIEBR Rounding FPCR 4op"  00000000 F8000000 00000000 F8000000
r 42C0.10  #                        RM / RP           RM / RM
*Want    "DIEBR Rounding FPCR 4qr"  00000000 F8000000 00000000 F8000000
r 42D0.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIEBR Rounding FPCR 4st"  00000000 F8000000 00000000 F8000000
r 42E0.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIEBR Rounding FPCR 4uv"  00000000 F8000000 00000000 F8000000
r 42F0.10  #                        RFS / RP          RFS / RM
*Want    "DIEBR Rounding FPCR 4wx"  00000000 F8000000 00000000 F8000000
r 4300.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIEBR Rounding FPCR 5ab"  00000000 F8000000 00000000 F8000000
r 4310.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIEBR Rounding FPCR 5cd"  00000000 F8000000 00000000 F8000000
r 4320.10  #                        RZ / RP           RZ / RM
*Want    "DIEBR Rounding FPCR 5ef"  00000000 F8000000 00000000 F8000000
r 4330.10  #                        RP / RNTA         RP / RFS
*Want    "DIEBR Rounding FPCR 5gh"  00000000 F8000000 00000000 F8000000
r 4340.10  #                        RP / RNTE         RP / RZ
*Want    "DIEBR Rounding FPCR 5ij"  00000000 F8000000 00000000 F8000000
r 4350.10  #                        RP / RP           RP / RM
*Want    "DIEBR Rounding FPCR 5kl"  00000000 F8000000 00000000 F8000000
r 4360.10  #                        RM / RNTA         RM / RFS
*Want    "DIEBR Rounding FPCR 5mn"  00000000 F8000000 00000000 F8000000
r 4370.10  #                        RM / RNTE         RM / RZ
*Want    "DIEBR Rounding FPCR 5op"  00000000 F8000000 00000000 F8000000
r 4380.10  #                        RM / RP           RM / RM
*Want    "DIEBR Rounding FPCR 5qr"  00000000 F8000000 00000000 F8000000
r 4390.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIEBR Rounding FPCR 5st"  00000000 F8000000 00000000 F8000000
r 43A0.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIEBR Rounding FPCR 5uv"  00000000 F8000000 00000000 F8000000
r 43B0.10  #                        RFS / RP          RFS / RM
*Want    "DIEBR Rounding FPCR 5wx"  00000000 F8000000 00000000 F8000000
r 43C0.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIEBR Rounding FPCR 6ab"  00000000 F8000000 00000000 F8000000
r 43D0.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIEBR Rounding FPCR 6cd"  00000000 F8000000 00000000 F8000000
r 43E0.10  #                        RZ / RP           RZ / RM
*Want    "DIEBR Rounding FPCR 6ef"  00000000 F8000000 00000000 F8000000
r 43F0.10  #                        RP / RNTA         RP / RFS
*Want    "DIEBR Rounding FPCR 6gh"  00000000 F8000000 00000000 F8000000
r 4400.10  #                        RP / RNTE         RP / RZ
*Want    "DIEBR Rounding FPCR 6ij"  00000000 F8000000 00000000 F8000000
r 4410.10  #                        RP / RP           RP / RM
*Want    "DIEBR Rounding FPCR 6kl"  00000000 F8000000 00000000 F8000000
r 4420.10  #                        RM / RNTA         RM / RFS
*Want    "DIEBR Rounding FPCR 6mn"  00000000 F8000000 00000000 F8000000
r 4430.10  #                        RM / RNTE         RM / RZ
*Want    "DIEBR Rounding FPCR 6op"  00000000 F8000000 00000000 F8000000
r 4440.10  #                        RM / RP           RM / RM
*Want    "DIEBR Rounding FPCR 6qr"  00000000 F8000000 00000000 F8000000
r 4450.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIEBR Rounding FPCR 6st"  00000000 F8000000 00000000 F8000000
r 4460.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIEBR Rounding FPCR 6uv"  00000000 F8000000 00000000 F8000000
r 4470.10  #                        RFS / RP          RFS / RM
*Want    "DIEBR Rounding FPCR 6wx"  00000000 F8000000 00000000 F8000000
r 4480.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIEBR Rounding FPCR 7ab"  00000000 F8000000 00000000 F8000000
r 4490.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIEBR Rounding FPCR 7cd"  00000000 F8000000 00000000 F8000000
r 44A0.10  #                        RZ / RP           RZ / RM
*Want    "DIEBR Rounding FPCR 7ef"  00000000 F8000000 00000000 F8000000
r 44B0.10  #                        RP / RNTA         RP / RFS
*Want    "DIEBR Rounding FPCR 7gh"  00000000 F8000000 00000000 F8000000
r 44C0.10  #                        RP / RNTE         RP / RZ
*Want    "DIEBR Rounding FPCR 7ij"  00000000 F8000000 00000000 F8000000
r 44D0.10  #                        RP / RP           RP / RM
*Want    "DIEBR Rounding FPCR 7kl"  00000000 F8000000 00000000 F8000000
r 44E0.10  #                        RM / RNTA         RM / RFS
*Want    "DIEBR Rounding FPCR 7mn"  00000000 F8000000 00000000 F8000000
r 44F0.10  #                        RM / RNTE         RM / RZ
*Want    "DIEBR Rounding FPCR 7op"  00000000 F8000000 00000000 F8000000
r 4500.10  #                        RM / RP           RM / RM
*Want    "DIEBR Rounding FPCR 7qr"  00000000 F8000000 00000000 F8000000
r 4510.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIEBR Rounding FPCR 7st"  00000000 F8000000 00000000 F8000000
r 4520.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIEBR Rounding FPCR 7uv"  00000000 F8000000 00000000 F8000000
r 4530.10  #                        RFS / RP          RFS / RM
*Want    "DIEBR Rounding FPCR 7wx"  00000000 F8000000 00000000 F8000000
r 4540.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIEBR Rounding FPCR 8ab"  00000000 F8000000 00000000 F8000000
r 4550.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIEBR Rounding FPCR 8cd"  00000000 F8000000 00000000 F8000000
r 4560.10  #                        RZ / RP           RZ / RM
*Want    "DIEBR Rounding FPCR 8ef"  00000000 F8000000 00000000 F8000000
r 4570.10  #                        RP / RNTA         RP / RFS
*Want    "DIEBR Rounding FPCR 8gh"  00000000 F8000000 00000000 F8000000
r 4580.10  #                        RP / RNTE         RP / RZ
*Want    "DIEBR Rounding FPCR 8ij"  00000000 F8000000 00000000 F8000000
r 4590.10  #                        RP / RP           RP / RM
*Want    "DIEBR Rounding FPCR 8kl"  00000000 F8000000 00000000 F8000000
r 45A0.10  #                        RM / RNTA         RM / RFS
*Want    "DIEBR Rounding FPCR 8mn"  00000000 F8000000 00000000 F8000000
r 45B0.10  #                        RM / RNTE         RM / RZ
*Want    "DIEBR Rounding FPCR 8op"  00000000 F8000000 00000000 F8000000
r 45C0.10  #                        RM / RP           RM / RM
*Want    "DIEBR Rounding FPCR 8qr"  00000000 F8000000 00000000 F8000000
r 45D0.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIEBR Rounding FPCR 8st"  00000000 F8000000 00000000 F8000000
r 45E0.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIEBR Rounding FPCR 8uv"  00000000 F8000000 00000000 F8000000
r 45F0.10  #                        RFS / RP          RFS / RM
*Want    "DIEBR Rounding FPCR 8wx"  00000000 F8000000 00000000 F8000000
r 4600.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIEBR Rounding FPCR 9ab"  00000000 F8000000 00000000 F8000000
r 4610.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIEBR Rounding FPCR 9cd"  00000000 F8000000 00000000 F8000000
r 4620.10  #                        RZ / RP           RZ / RM
*Want    "DIEBR Rounding FPCR 9ef"  00000000 F8000000 00000000 F8000000
r 4630.10  #                        RP / RNTA         RP / RFS
*Want    "DIEBR Rounding FPCR 9gh"  00000000 F8000000 00000000 F8000000
r 4640.10  #                        RP / RNTE         RP / RZ
*Want    "DIEBR Rounding FPCR 9ij"  00000000 F8000000 00000000 F8000000
r 4650.10  #                        RP / RP           RP / RM
*Want    "DIEBR Rounding FPCR 9kl"  00000000 F8000000 00000000 F8000000
r 4660.10  #                        RM / RNTA         RM / RFS
*Want    "DIEBR Rounding FPCR 9mn"  00000000 F8000000 00000000 F8000000
r 4670.10  #                        RM / RNTE         RM / RZ
*Want    "DIEBR Rounding FPCR 9op"  00000000 F8000000 00000000 F8000000
r 4680.10  #                        RM / RP           RM / RM
*Want    "DIEBR Rounding FPCR 9qr"  00000000 F8000000 00000000 F8000000
r 4690.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIEBR Rounding FPCR 9st"  00000000 F8000000 00000000 F8000000
r 46A0.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIEBR Rounding FPCR 9uv"  00000000 F8000000 00000000 F8000000
r 46B0.10  #                        RFS / RP          RFS / RM
*Want    "DIEBR Rounding FPCR 9wx"  00000000 F8000000 00000000 F8000000
r 46C0.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIEBR Rounding FPCR 10ab" 00000000 F8000000 00000000 F8000000
r 46D0.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIEBR Rounding FPCR 10cd" 00000000 F8000000 00000000 F8000000
r 46E0.10  #                        RZ / RP           RZ / RM
*Want    "DIEBR Rounding FPCR 10ef" 00000000 F8000000 00000000 F8000000
r 46F0.10  #                        RP / RNTA         RP / RFS
*Want    "DIEBR Rounding FPCR 10gh" 00000000 F8000000 00000000 F8000000
r 4700.10  #                        RP / RNTE         RP / RZ
*Want    "DIEBR Rounding FPCR 10ij" 00000000 F8000000 00000000 F8000000
r 4710.10  #                        RP / RP           RP / RM
*Want    "DIEBR Rounding FPCR 10kl" 00000000 F8000000 00000000 F8000000
r 4720.10  #                        RM / RNTA         RM / RFS
*Want    "DIEBR Rounding FPCR 10mn" 00000000 F8000000 00000000 F8000000
r 4730.10  #                        RM / RNTE         RM / RZ
*Want    "DIEBR Rounding FPCR 10op" 00000000 F8000000 00000000 F8000000
r 4740.10  #                        RM / RP           RM / RM
*Want    "DIEBR Rounding FPCR 10qr" 00000000 F8000000 00000000 F8000000
r 4750.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIEBR Rounding FPCR 10st" 00000000 F8000000 00000000 F8000000
r 4760.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIEBR Rounding FPCR 10uv" 00000000 F8000000 00000000 F8000000
r 4770.10  #                        RFS / RP          RFS / RM
*Want    "DIEBR Rounding FPCR 10wx" 00000000 F8000000 00000000 F8000000
r 4780.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIEBR Rounding FPCR 11ab" 00000000 F8000000 00000000 F8000000
r 4790.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIEBR Rounding FPCR 11cd" 00000000 F8000000 00000000 F8000000
r 47A0.10  #                        RZ / RP           RZ / RM
*Want    "DIEBR Rounding FPCR 11ef" 00000000 F8000000 00000000 F8000000
r 47B0.10  #                        RP / RNTA         RP / RFS
*Want    "DIEBR Rounding FPCR 11gh" 00000000 F8000000 00000000 F8000000
r 47C0.10  #                        RP / RNTE         RP / RZ
*Want    "DIEBR Rounding FPCR 11ij" 00000000 F8000000 00000000 F8000000
r 47D0.10  #                        RP / RP           RP / RM
*Want    "DIEBR Rounding FPCR 11kl" 00000000 F8000000 00000000 F8000000
r 47E0.10  #                        RM / RNTA         RM / RFS
*Want    "DIEBR Rounding FPCR 11mn" 00000000 F8000000 00000000 F8000000
r 47F0.10  #                        RM / RNTE         RM / RZ
*Want    "DIEBR Rounding FPCR 11op" 00000000 F8000000 00000000 F8000000
r 4800.10  #                        RM / RP           RM / RM
*Want    "DIEBR Rounding FPCR 11qr" 00000000 F8000000 00000000 F8000000
r 4810.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIEBR Rounding FPCR 11st" 00000000 F8000000 00000000 F8000000
r 4820.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIEBR Rounding FPCR 11uv" 00000000 F8000000 00000000 F8000000
r 4830.10  #                        RFS / RP          RFS / RM
*Want    "DIEBR Rounding FPCR 11wx" 00000000 F8000000 00000000 F8000000
r 4840.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIEBR Rounding FPCR 12ab" 00000000 F8000000 00000000 F8000000
r 4850.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIEBR Rounding FPCR 12cd" 00000000 F8000000 00000000 F8000000
r 4860.10  #                        RZ / RP           RZ / RM
*Want    "DIEBR Rounding FPCR 12ef" 00000000 F8000000 00000000 F8000000
r 4870.10  #                        RP / RNTA         RP / RFS
*Want    "DIEBR Rounding FPCR 12gh" 00000000 F8000000 00000000 F8000000
r 4880.10  #                        RP / RNTE         RP / RZ
*Want    "DIEBR Rounding FPCR 12ij" 00000000 F8000000 00000000 F8000000
r 4890.10  #                        RP / RP           RP / RM
*Want    "DIEBR Rounding FPCR 12kl" 00000000 F8000000 00000000 F8000000
r 48A0.10  #                        RM / RNTA         RM / RFS
*Want    "DIEBR Rounding FPCR 12mn" 00000000 F8000000 00000000 F8000000
r 48B0.10  #                        RM / RNTE         RM / RZ
*Want    "DIEBR Rounding FPCR 12op" 00000000 F8000000 00000000 F8000000
r 48C0.10  #                        RM / RP           RM / RM
*Want    "DIEBR Rounding FPCR 12qr" 00000000 F8000000 00000000 F8000000
r 48D0.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIEBR Rounding FPCR 12st" 00000000 F8000000 00000000 F8000000
r 48E0.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIEBR Rounding FPCR 12uv" 00000000 F8000000 00000000 F8000000
r 48F0.10  #                        RFS / RP          RFS / RM
*Want    "DIEBR Rounding FPCR 12wx" 00000000 F8000000 00000000 F8000000




*Compare
# NaN propagation tests - BFP Long
r 1300.10 #                Expecting QNaN A
*Want     "DIDBR test 1a NaN" 7FF8A000 00000000 7FF8A000 00000000
r 1310.10 #                Expecting QNaN A
*Want     "DIDBR test 1b NaN" 7FF0A000 00000000 00000000 00000000
r 1320.10 #                Expecting QNaN A
*Want     "DIDBR test 2a NaN" 7FF8A000 00000000 7FF8A000 00000000
r 1330.10 #                Expecting QNaN A
*Want     "DIDBR test 2b NaN" 7FF8A000 00000000 7FF8A000 00000000
r 1340.10 #                Expecting QNaN A
*Want     "DIDBR test 3a NaN" 7FF8B000 00000000 7FF8B000 00000000
r 1350.10 #                Expecting QNaN A
*Want     "DIDBR test 3b NaN" 7FF8B000 00000000 7FF8B000 00000000
r 1360.10 #                Expecting QNaN B
*Want     "DIDBR test 4a NaN" 7FF8B000 00000000 7FF8B000 00000000
r 1370.10 #                Expecting QNaN B
*Want     "DIDBR test 4b NaN" 7FF8A000 00000000 00000000 00000000


# Dividend -inf - BFP Long - all should be dNaN (a +QNaN)
r 1380.10
*Want     "DIDBR test 5a -inf dividend"  7FF80000 00000000 7FF80000 00000000
r 1390.10
*Want     "DIDBR test 5b -inf dividend"  FFF00000 00000000 00000000 00000000
r 13A0.10
*Want     "DIDBR test 6a -inf dividend"  7FF80000 00000000 7FF80000 00000000
r 13B0.10
*Want     "DIDBR test 6b -inf dividend"  FFF00000 00000000 00000000 00000000
r 13C0.10
*Want     "DIDBR test 7a -inf dividend"  7FF80000 00000000 7FF80000 00000000
r 13D0.10
*Want     "DIDBR test 7b -inf dividend"  FFF00000 00000000 00000000 00000000
r 13E0.10
*Want     "DIDBR test 8a -inf dividend"  7FF80000 00000000 7FF80000 00000000
r 13F0.10
*Want     "DIDBR test 8b -inf dividend"  FFF00000 00000000 00000000 00000000
r 1400.10
*Want     "DIDBR test 9a -inf dividend"  7FF80000 00000000 7FF80000 00000000
r 1410.10
*Want     "DIDBR test 9b -inf dividend"  FFF00000 00000000 00000000 00000000
r 1420.10
*Want     "DIDBR test 10a -inf dividend" 7FF80000 00000000 7FF80000 00000000
r 1430.10
*Want     "DIDBR test 10b -inf dividend" FFF00000 00000000 00000000 00000000


# Dividend +inf - BFP Long - all should be dNaN (a +QNaN)
r 1440.10
*Want     "DIDBR test 11a +inf dividend"  7FF80000 00000000 7FF80000 00000000
r 1450.10 
*Want     "DIDBR test 11b +inf dividend"  7FF00000 00000000 00000000 00000000
r 1460.10
*Want     "DIDBR test 12a +inf dividend"  7FF80000 00000000 7FF80000 00000000
r 1470.10
*Want     "DIDBR test 12b +inf dividend"  7FF00000 00000000 00000000 00000000
r 1480.10
*Want     "DIDBR test 13a +inf dividend"  7FF80000 00000000 7FF80000 00000000
r 1490.10
*Want     "DIDBR test 13b +inf dividend"  7FF00000 00000000 00000000 00000000
r 14A0.10
*Want     "DIDBR test 14a +inf dividend"  7FF80000 00000000 7FF80000 00000000
r 14B0.10
*Want     "DIDBR test 14b +inf dividend"  7FF00000 00000000 00000000 00000000
r 14C0.10
*Want     "DIDBR test 15a +inf dividend"  7FF80000 00000000 7FF80000 00000000
r 14D0.10
*Want     "DIDBR test 15b +inf dividend"  7FF00000 00000000 00000000 00000000
r 14E0.10
*Want     "DIDBR test 16a +inf dividend"  7FF80000 00000000 7FF80000 00000000
r 14F0.10
*Want     "DIDBR test 16b +inf dividend"  7FF00000 00000000 00000000 00000000


# Divisor -0 - BFP long - all should be dNaN (a +QNaN)
r 1500.10
*Want     "DIDBR test 17a -0 divisor" 7FF80000 00000000 7FF80000 00000000
r 1510.10
*Want     "DIDBR test 17b -0 divisor" C0000000 00000000 00000000 00000000
r 1520.10
*Want     "DIDBR test 18a -0 divisor" 7FF80000 00000000 7FF80000 00000000
r 1530.10
*Want     "DIDBR test 18b -0 divisor" 80000000 00000000 00000000 00000000
r 1540.10
*Want     "DIDBR test 19a -0 divisor" 7FF80000 00000000 7FF80000 00000000
r 1550.10
*Want     "DIDBR test 19b -0 divisor" 00000000 00000000 00000000 00000000
r 1560.10
*Want     "DIDBR test 20a -0 divisor" 7FF80000 00000000 7FF80000 00000000
r 1570.10
*Want     "DIDBR test 20b -0 divisor" 40000000 00000000 00000000 00000000


# Divisor +0 - BFP long - all should be dNaN (a +QNaN)
r 1580.10
*Want     "DIDBR test 21a +0 divisor" 7FF80000 00000000 7FF80000 00000000
r 1590.10
*Want     "DIDBR test 21b +0 divisor" C0000000 00000000 00000000 00000000
r 15A0.10
*Want     "DIDBR test 22a +0 divisor" 7FF80000 00000000 7FF80000 00000000
r 15B0.10
*Want     "DIDBR test 22b +0 divisor" 80000000 00000000 00000000 00000000
r 15C0.10
*Want     "DIDBR test 23a +0 divisor" 7FF80000 00000000 7FF80000 00000000
r 15D0.10
*Want     "DIDBR test 23b +0 divisor" 00000000 00000000 00000000 00000000
r 15E0.10
*Want     "DIDBR test 24a +0 divisor" 7FF80000 00000000 7FF80000 00000000
r 15F0.10
*Want     "DIDBR test 24b +0 divisor" 40000000 00000000 00000000 00000000


# Divisor -inf - BFP long - all should be dNaN (a +QNaN)
r 1600.10
*Want     "DIDBR test 25a -inf divisor" C0000000 00000000 00000000 00000000
r 1610.10
*Want     "DIDBR test 25b -inf divisor" C0000000 00000000 00000000 00000000
r 1620.10
*Want     "DIDBR test 26a -inf divisor" 80000000 00000000 00000000 00000000
r 1630.10
*Want     "DIDBR test 26b -inf divisor" 80000000 00000000 00000000 00000000
r 1640.10
*Want     "DIDBR test 27a -inf divisor" 00000000 00000000 80000000 00000000
r 1650.10
*Want     "DIDBR test 27b -inf divisor" 00000000 00000000 80000000 00000000
r 1660.10
*Want     "DIDBR test 28a -inf divisor" 40000000 00000000 80000000 00000000
r 1670.10
*Want     "DIDBR test 28b -inf divisor" 40000000 00000000 80000000 00000000


# Divisor +inf - BFP long - all should be dNaN (a +QNaN)
r 1680.10
*Want     "DIDBR test 29a +inf divisor" C0000000 00000000 80000000 00000000
r 1690.10
*Want     "DIDBR test 29b +inf divisor" C0000000 00000000 80000000 00000000
r 16A0.10
*Want     "DIDBR test 30a +inf divisor" 80000000 00000000 80000000 00000000
r 16B0.10
*Want     "DIDBR test 30b +inf divisor" 80000000 00000000 80000000 00000000
r 16C0.10
*Want     "DIDBR test 31a +inf divisor" 00000000 00000000 00000000 00000000
r 16D0.10
*Want     "DIDBR test 31b +inf divisor" 00000000 00000000 00000000 00000000
r 16E0.10
*Want     "DIDBR test 32a +inf divisor" 40000000 00000000 00000000 00000000
r 16F0.10
*Want     "DIDBR test 32b +inf divisor" 40000000 00000000 00000000 00000000


*Compare
# NaN propagation tests - BFP long - FPCR & cc
r 1700.10 
*Want     "DIDBR FPCR pair 1-2"   00800001 F8008001 00000001 F8000001
r 1710.10 
*Want     "DIDBR FPCR pair 3-4"   00000001 F8000001 00800001 F8008001


# Dividend -inf - BFP long - FPCR & cc
r 1720.10
*Want     "DIDBR FPCR pair 5-6"   00800001 F8008001 00800001 F8008001
r 1730.10 
*Want     "DIDBR FPCR pair 7-8"   00800001 F8008001 00800001 F8008001
r 1740.10
*Want     "DIDBR FPCR pair 9-10"  00800001 F8008001 00800001 F8008001


# Dividend -inf - BFP long - FPCR & cc
r 1750.10 
*Want     "DIDBR FPCR pair 11-12" 00800001 F8008001 00800001 F8008001
r 1760.10 
*Want     "DIDBR FPCR pair 13-14" 00800001 F8008001 00800001 F8008001
r 1770.10 
*Want     "DIDBR FPCR pair 15-16" 00800001 F8008001 00800001 F8008001


# Divisor -0 - BFP long - FPCR & cc
r 1780.10 
*Want     "DIDBR FPCR pair 17-18" 00800001 F8008001 00800001 F8008001
r 1790.10 
*Want     "DIDBR FPCR pair 19-20" 00800001 F8008001 00800001 F8008001

# Divisor +0 - BFP long - FPCR & cc
r 17A0.10 
*Want     "DIDBR FPCR pair 21-22" 00800001 F8008001 00800001 F8008001
r 17B0.10 
*Want     "DIDBR FPCR pair 23-24" 00800001 F8008001 00800001 F8008001


# Divisor -inf - BFP long - FPCR & cc
r 17C0.10 
*Want     "DIDBR FPCR pair 25-26" 00000000 F8000000 00000000 F8000000
r 17D0.10 
*Want     "DIDBR FPCR pair 27-28" 00000000 F8000000 00000000 F8000000

# Divisor +inf - BFP long - FPCR & cc
r 17E0.10 
*Want     "DIDBR FPCR pair 29-30" 00000000 F8000000 00000000 F8000000
r 17F0.10 
*Want     "DIDBR FPCR pair 31-32" 00000000 F8000000 00000000 F8000000



# Dividend & Divisor finite - BFP Long - basic tests
r B000.10
*Want "DIDBR finite test -8/-4 1a"  80000000 00000000 40000000 00000000
r B010.08
*Want "DIDBR finite test -8/-4 1a"  C0200000 00000000
r B020.10
*Want "DIDBR finite test -8/-4 1b"  80000000 00000000 40000000 00000000
r B030.08
*Want "DIDBR finite test -8/-4 1b"  C0200000 00000000
r B040.10
*Want "DIDBR finite test -7/-4 2a"  3FF00000 00000000 40000000 00000000
r B050.08
*Want "DIDBR finite test -7/-4 2a"  C01C0000 00000000
r B060.10
*Want "DIDBR finite test -7/-4 2b"  3FF00000 00000000 40000000 00000000
r B070.08
*Want "DIDBR finite test -7/-4 2b"  C01C0000 00000000
r B080.10
*Want "DIDBR finite test -6/-4 3a"  40000000 00000000 40000000 00000000
r B090.08
*Want "DIDBR finite test -6/-4 3a"  C0180000 00000000
r B0A0.10
*Want "DIDBR finite test -6/-4 3b"  40000000 00000000 40000000 00000000
r B0B0.08
*Want "DIDBR finite test -6/-4 3b"  C0180000 00000000
r B0C0.10
*Want "DIDBR finite test -5/-4 4a"  BFF00000 00000000 3FF00000 00000000
r B0D0.08
*Want "DIDBR finite test -5/-4 4a"  C0140000 00000000
r B0E0.10
*Want "DIDBR finite test -5/-4 4b"  BFF00000 00000000 3FF00000 00000000
r B0F0.08
*Want "DIDBR finite test -5/-4 4b"  C0140000 00000000
r B100.10
*Want "DIDBR finite test -4/-4 5a"  80000000 00000000 3FF00000 00000000
r B110.08
*Want "DIDBR finite test -4/-4 5a"  C0100000 00000000
r B120.10
*Want "DIDBR finite test -4/-4 5b"  80000000 00000000 3FF00000 00000000
r B130.08
*Want "DIDBR finite test -4/-4 5b"  C0100000 00000000
r B140.10
*Want "DIDBR finite test -3/-4 6a"  3FF00000 00000000 3FF00000 00000000
r B150.08
*Want "DIDBR finite test -3/-4 6a"  C0080000 00000000
r B160.10
*Want "DIDBR finite test -3/-4 6b"  3FF00000 00000000 3FF00000 00000000
r B170.08
*Want "DIDBR finite test -3/-4 6b"  C0080000 00000000
r B180.10
*Want "DIDBR finite test -2/-4 7a"  C0000000 00000000 00000000 00000000
r B190.08
*Want "DIDBR finite test -2/-4 7a"  C0000000 00000000
r B1A0.10
*Want "DIDBR finite test -2/-4 7b"  C0000000 00000000 00000000 00000000
r B1B0.08
*Want "DIDBR finite test -2/-4 7b"  C0000000 00000000
r B1C0.10
*Want "DIDBR finite test -1/-4 8a"  BFF00000 00000000 00000000 00000000
r B1D0.08
*Want "DIDBR finite test -1/-4 8a"  BFF00000 00000000
r B1E0.10
*Want "DIDBR finite test -1/-4 8b"  BFF00000 00000000 00000000 00000000
r B1F0.08
*Want "DIDBR finite test -1/-4 8b"  BFF00000 00000000
r B200.10
*Want "DIDBR finite test +1/-4 9a"  3FF00000 00000000 80000000 00000000
r B210.08
*Want "DIDBR finite test +1/-4 9a"  3FF00000 00000000
r B220.10
*Want "DIDBR finite test +1/-4 9b"  3FF00000 00000000 80000000 00000000
r B230.08
*Want "DIDBR finite test +1/-4 9b"  3FF00000 00000000
r B240.10
*Want "DIDBR finite test +2/-4 10a" 40000000 00000000 80000000 00000000
r B250.08
*Want "DIDBR finite test +2/-4 10a" 40000000 00000000
r B260.10
*Want "DIDBR finite test +2/-4 10b" 40000000 00000000 80000000 00000000
r B270.08
*Want "DIDBR finite test +2/-4 10b" 40000000 00000000
r B280.10
*Want "DIDBR finite test +3/-4 11a" BFF00000 00000000 BFF00000 00000000
r B290.08
*Want "DIDBR finite test +3/-4 11a" 40080000 00000000
r B2A0.10
*Want "DIDBR finite test +3/-4 11b" BFF00000 00000000 BFF00000 00000000
r B2B0.08
*Want "DIDBR finite test +3/-4 11b" 40080000 00000000
r B2C0.10
*Want "DIDBR finite test +4/-4 12a" 00000000 00000000 BFF00000 00000000
r B2D0.08
*Want "DIDBR finite test +4/-4 12a" 40100000 00000000
r B2E0.10
*Want "DIDBR finite test +4/-4 12b" 00000000 00000000 BFF00000 00000000
r B2F0.08
*Want "DIDBR finite test +4/-4 12b" 40100000 00000000
r B300.10
*Want "DIDBR finite test +5/-4 13a" 3FF00000 00000000 BFF00000 00000000
r B310.08
*Want "DIDBR finite test +5/-4 13a" 40140000 00000000
r B320.10
*Want "DIDBR finite test +5/-4 13b" 3FF00000 00000000 BFF00000 00000000
r B330.08
*Want "DIDBR finite test +5/-4 13b" 40140000 00000000
r B340.10
*Want "DIDBR finite test +6/-4 14a" C0000000 00000000 C0000000 00000000
r B350.08
*Want "DIDBR finite test +6/-4 14a" 40180000 00000000
r B360.10
*Want "DIDBR finite test +6/-4 14b" C0000000 00000000 C0000000 00000000
r B370.08
*Want "DIDBR finite test +6/-4 14b" 40180000 00000000
r B380.10
*Want "DIDBR finite test 15a" BFF00000 00000000 C0000000 00000000
r B390.08
*Want "DIDBR finite test 15a" 401C0000 00000000
r B3A0.10
*Want "DIDBR finite test 15b" BFF00000 00000000 C0000000 00000000
r B3B0.08
*Want "DIDBR finite test 15b" 401C0000 00000000
r B3C0.10
*Want "DIDBR finite test 16a" 00000000 00000000 C0000000 00000000
r B3D0.08
*Want "DIDBR finite test 16a" 40200000 00000000
r B3E0.10
*Want "DIDBR finite test 16b" 00000000 00000000 C0000000 00000000
r B3F0.08
*Want "DIDBR finite test 16b" 40200000 00000000
r B400.10
*Want "DIDBR finite test 17a" 80000000 00000000 C0000000 00000000
r B410.08
*Want "DIDBR finite test 17a" C0200000 00000000
r B420.10
*Want "DIDBR finite test 17b" 80000000 00000000 C0000000 00000000
r B430.08
*Want "DIDBR finite test 17b" C0200000 00000000
r B440.10
*Want "DIDBR finite test 18a" 3FF00000 00000000 C0000000 00000000
r B450.08
*Want "DIDBR finite test 18a" C01C0000 00000000
r B460.10
*Want "DIDBR finite test 18b" 3FF00000 00000000 C0000000 00000000
r B470.08
*Want "DIDBR finite test 18b" C01C0000 00000000
r B480.10
*Want "DIDBR finite test 19a" 40000000 00000000 C0000000 00000000
r B490.08
*Want "DIDBR finite test 19a" C0180000 00000000
r B4A0.10
*Want "DIDBR finite test 19b" 40000000 00000000 C0000000 00000000
r B4B0.08
*Want "DIDBR finite test 19b" C0180000 00000000
r B4C0.10
*Want "DIDBR finite test 20a" BFF00000 00000000 BFF00000 00000000
r B4D0.08
*Want "DIDBR finite test 20a" C0140000 00000000
r B4E0.10
*Want "DIDBR finite test 20b" BFF00000 00000000 BFF00000 00000000
r B4F0.08
*Want "DIDBR finite test 20b" C0140000 00000000
r B500.10
*Want "DIDBR finite test 21a" 80000000 00000000 BFF00000 00000000
r B510.08
*Want "DIDBR finite test 21a" C0100000 00000000
r B520.10
*Want "DIDBR finite test 21b" 80000000 00000000 BFF00000 00000000
r B530.08
*Want "DIDBR finite test 21b" C0100000 00000000
r B540.10
*Want "DIDBR finite test 22a" 3FF00000 00000000 BFF00000 00000000
r B550.08
*Want "DIDBR finite test 22a" C0080000 00000000
r B560.10
*Want "DIDBR finite test 22b" 3FF00000 00000000 BFF00000 00000000
r B570.08
*Want "DIDBR finite test 22b" C0080000 00000000
r B580.10
*Want "DIDBR finite test 23a" C0000000 00000000 80000000 00000000
r B590.08
*Want "DIDBR finite test 23a" C0000000 00000000
r B5A0.10
*Want "DIDBR finite test 23b" C0000000 00000000 80000000 00000000
r B5B0.08
*Want "DIDBR finite test 23b" C0000000 00000000
r B5C0.10
*Want "DIDBR finite test 24a" 3FF00000 00000000 00000000 00000000
r B5D0.08
*Want "DIDBR finite test 24a" 3FF00000 00000000
r B5E0.10
*Want "DIDBR finite test 24b" 3FF00000 00000000 00000000 00000000
r B5F0.08
*Want "DIDBR finite test 24b" 3FF00000 00000000
r B600.10
*Want "DIDBR finite test 25a" 3FF00000 00000000 00000000 00000000
r B610.08
*Want "DIDBR finite test 25a" 3FF00000 00000000
r B620.10
*Want "DIDBR finite test 25b" 3FF00000 00000000 00000000 00000000
r B630.08
*Want "DIDBR finite test 25b" 3FF00000 00000000
r B640.10
*Want "DIDBR finite test 26a" 40000000 00000000 00000000 00000000
r B650.08
*Want "DIDBR finite test 26a" 40000000 00000000
r B660.10
*Want "DIDBR finite test 26b" 40000000 00000000 00000000 00000000
r B670.08
*Want "DIDBR finite test 26b" 40000000 00000000
r B680.10
*Want "DIDBR finite test 27a" BFF00000 00000000 3FF00000 00000000
r B690.08
*Want "DIDBR finite test 27a" 40080000 00000000
r B6A0.10
*Want "DIDBR finite test 27b" BFF00000 00000000 3FF00000 00000000
r B6B0.08
*Want "DIDBR finite test 27b" 40080000 00000000
r B6C0.10
*Want "DIDBR finite test 28a" 00000000 00000000 3FF00000 00000000
r B6D0.08
*Want "DIDBR finite test 28a" 40100000 00000000
r B6E0.10
*Want "DIDBR finite test 28b" 00000000 00000000 3FF00000 00000000
r B6F0.08
*Want "DIDBR finite test 28b" 40100000 00000000
r B700.10
*Want "DIDBR finite test 29a" 3FF00000 00000000 3FF00000 00000000
r B710.08
*Want "DIDBR finite test 29a" 40140000 00000000
r B720.10
*Want "DIDBR finite test 29b" 3FF00000 00000000 3FF00000 00000000
r B730.08
*Want "DIDBR finite test 29b" 40140000 00000000
r B740.10
*Want "DIDBR finite test 30a" C0000000 00000000 40000000 00000000
r B750.08
*Want "DIDBR finite test 30a" 40180000 00000000
r B760.10
*Want "DIDBR finite test 30b" C0000000 00000000 40000000 00000000
r B770.08
*Want "DIDBR finite test 30b" 40180000 00000000
r B780.10
*Want "DIDBR finite test 31a" BFF00000 00000000 40000000 00000000
r B790.08
*Want "DIDBR finite test 31a" 401C0000 00000000
r B7A0.10
*Want "DIDBR finite test 31b" BFF00000 00000000 40000000 00000000
r B7B0.08
*Want "DIDBR finite test 31b" 401C0000 00000000
r B7C0.10
*Want "DIDBR finite test 32a" 00000000 00000000 40000000 00000000
r B7D0.08
*Want "DIDBR finite test 32a" 40200000 00000000
r B7E0.10
*Want "DIDBR finite test 32b" 00000000 00000000 40000000 00000000
r B7F0.08
*Want "DIDBR finite test 32b" 40200000 00000000

r B800.10
*Want "DIDBR finite test 33a" 40100000 00000000 C0100000 00000000
r B810.08
*Want "DIDBR finite test 33a" 40440000 00000000
r B820.10
*Want "DIDBR finite test 33b" 40100000 00000000 C0100000 00000000
r B830.08
*Want "DIDBR finite test 33b" 40440000 00000000

r B840.10
*Want "DIDBR finite test 34a" 00000000 00000000 630FFFFF FFFFFFFF
r B850.08
*Want "DIDBR finite test 34a" 1FEFFFFF FFFFFFFF
r B860.10
*Want "DIDBR finite test 34b" 00000000 00000000 630FFFFF FFFFFFFF
r B870.08
*Want "DIDBR finite test 34b" 1FEFFFFF FFFFFFFF

r B880.10   # Doubt the following are correct; need hardware validation
*Want "DIDBR finite test 35a" 7CA00000 00000000 62F55555 55555554

r B890.08
*Want "DIDBR finite test 35a" 7CA00000 00000000

r B8A0.10   # Doubt the following are correct; need hardware validation
*Want "DIDBR finite test 35b" 7CA00000 00000000 62F55555 55555554

r B8B0.08
*Want "DIDBR finite test 35b" 7CA00000 00000000

r B8C0.10
*Want "DIDBR finite test 36a" 00000000 00000001 3FF00000 00000000
r B8D0.08
*Want "DIDBR finite test 36a" 000FFFFF FFFFFFFF

r B8E0.10
*Want "DIDBR finite test 36b" 5CD00000 00000000 3FF00000 00000000

r B8F0.08
*Want "DIDBR finite test 36b" 5CD00000 00000000


r B900.10
*Want "DIDBR finite test 37a" 40100000 00000000 43555555 55555555
r B910.08
*Want "DIDBR finite test 37a" 43700000 00000000
r B920.10
*Want "DIDBR finite test 37b" 40100000 00000000 43555555 55555555
r B930.08
*Want "DIDBR finite test 37b" 43700000 00000000


r B940.10
*Want "DIDBR finite test 38a" 40020000 00000000 00000000 00000000
r B950.08
*Want "DIDBR finite test 38a" 40020000 00000000
r B960.10
*Want "DIDBR finite test 38b" 40020000 00000000 00000000 00000000
r B970.08
*Want "DIDBR finite test 38b" 40020000 00000000


*Compare
# Two finites - BFP Long - FPCR & cc (cc unchanged by re-multiply)
r AC00.10
*Want "DIDBR FPCR finite test -8/-4 1"  00000000 00000000 F8000000 00000000
r AC10.10
*Want "DIDBR FPCR finite test -7/-4 2"  00000000 00000000 F8000000 00000000
r AC20.10
*Want "DIDBR FPCR finite test -6/-4 3"  00000000 00000000 F8000000 00000000
r AC30.10
*Want "DIDBR FPCR finite test -5/-4 4"  00000000 00000000 F8000000 00000000
r AC40.10
*Want "DIDBR FPCR finite test -4/-4 5"  00000000 00000000 F8000000 00000000
r AC50.10
*Want "DIDBR FPCR finite test -3/-4 6"  00000000 00000000 F8000000 00000000
r AC60.10
*Want "DIDBR FPCR finite test -2/-4 7"  00000000 00000000 F8000000 00000000
r AC70.10
*Want "DIDBR FPCR finite test -1/-4 8"  00000000 00000000 F8000000 00000000
r AC80.10
*Want "DIDBR FPCR finite test +1/-4 9"  00000000 00000000 F8000000 00000000
r AC90.10
*Want "DIDBR FPCR finite test +2/-4 10" 00000000 00000000 F8000000 00000000
r ACA0.10
*Want "DIDBR FPCR finite test +3/-4 10" 00000000 00000000 F8000000 00000000
r ACB0.10
*Want "DIDBR FPCR finite test +4/-4 12" 00000000 00000000 F8000000 00000000
r ACC0.10
*Want "DIDBR FPCR finite test +5/-4 13" 00000000 00000000 F8000000 00000000
r ACD0.10
*Want "DIDBR FPCR finite test +6/-4 14" 00000000 00000000 F8000000 00000000
r ACE0.10
*Want "DIDBR FPCR finite test 15" 00000000 00000000 F8000000 00000000
r ACF0.10
*Want "DIDBR FPCR finite test 16" 00000000 00000000 F8000000 00000000
r AD00.10
*Want "DIDBR FPCR finite test 17" 00000000 00000000 F8000000 00000000
r AD10.10
*Want "DIDBR FPCR finite test 18" 00000000 00000000 F8000000 00000000
r AD20.10
*Want "DIDBR FPCR finite test 19" 00000000 00000000 F8000000 00000000
r AD30.10
*Want "DIDBR FPCR finite test 20" 00000000 00000000 F8000000 00000000
r AD40.10
*Want "DIDBR FPCR finite test 21" 00000000 00000000 F8000000 00000000
r AD50.10
*Want "DIDBR FPCR finite test 22" 00000000 00000000 F8000000 00000000
r AD60.10
*Want "DIDBR FPCR finite test 23" 00000000 00000000 F8000000 00000000
r AD70.10
*Want "DIDBR FPCR finite test 24" 00000000 00000000 F8000000 00000000
r AD80.10
*Want "DIDBR FPCR finite test 25" 00000000 00000000 F8000000 00000000
r AD90.10
*Want "DIDBR FPCR finite test 26" 00000000 00000000 F8000000 00000000
r ADA0.10
*Want "DIDBR FPCR finite test 27" 00000000 00000000 F8000000 00000000
r ADB0.10
*Want "DIDBR FPCR finite test 28" 00000000 00000000 F8000000 00000000
r ADC0.10
*Want "DIDBR FPCR finite test 29" 00000000 00000000 F8000000 00000000
r ADD0.10
*Want "DIDBR FPCR finite test 30" 00000000 00000000 F8000000 00000000
r ADE0.10
*Want "DIDBR FPCR finite test 31" 00000000 00000000 F8000000 00000000
r ADF0.10
*Want "DIDBR FPCR finite test 32" 00000000 00000000 F8000000 00000000
r AE00.10
*Want "DIDBR FPCR finite test 33" 00000000 00000000 F8000000 00000000
r AE10.10 
*Want "DIDBR FPCR finite test 34" 00000001 00000001 F8000001 00000001
r AE20.10  # unsure about case 3; hw validation required.
*Want "DIDBR FPCR finite test 35" 00000003 00080003 F8000003 00080003
r AE30.10
*Want "DIDBR FPCR finite test 36" 00000000 00000000 F8001000 00080000
r AE40.10  
*Want "DIDBR FPCR finite test 37" 00000002 00000002 F8000002 00000002
r AE50.10  
*Want "DIDBR FPCR finite test 38" 00000000 00000000 F8000000 00000000


# Long BFP Exhaustive rounding tests
r 5000.10   #                        RZ / RNTA
*Want "DIDBR rounding test 1a NT"  3FE00000 00000000 C0140000 00000000
r 5010.10   #                        RZ / RNTA
*Want "DIDBR rounding test 1a TR"  3FE00000 00000000 C0140000 00000000
r 5020.10   #                        RZ / RFS
*Want "DIDBR rounding test 1b NT"  3FE00000 00000000 C0140000 00000000
r 5030.10   #                        RZ / RFS
*Want "DIDBR rounding test 1b TR"  3FE00000 00000000 C0140000 00000000
r 5040.10   #                        RZ / RNTE
*Want "DIDBR rounding test 1c NT"  3FE00000 00000000 C0140000 00000000
r 5050.10   #                        RZ / RNTE
*Want "DIDBR rounding test 1c TR"  3FE00000 00000000 C0140000 00000000
r 5060.10   #                        RZ / RZ
*Want "DIDBR rounding test 1d NT"  BFF80000 00000000 C0100000 00000000
r 5070.10   #                        RZ / RZ
*Want "DIDBR rounding test 1d TR"  BFF80000 00000000 C0100000 00000000
r 5080.10   #                        RZ / RP
*Want "DIDBR rounding test 1e NT"  BFF80000 00000000 C0100000 00000000
r 5090.10   #                        RZ / RP
*Want "DIDBR rounding test 1e TR"  BFF80000 00000000 C0100000 00000000
r 50A0.10   #                        RZ / RM
*Want "DIDBR rounding test 1f NT"  3FE00000 00000000 C0140000 00000000
r 50B0.10   #                        RZ / RM
*Want "DIDBR rounding test 1f TR"  3FE00000 00000000 C0140000 00000000
r 50C0.10   #                        RP / RNTA
*Want "DIDBR rounding test 1g NT"  3FE00000 00000000 C0140000 00000000
r 50D0.10   #                        RP / RNTA
*Want "DIDBR rounding test 1g TR"  3FE00000 00000000 C0140000 00000000
r 50E0.10   #                        RP / RFS
*Want "DIDBR rounding test 1h NT"  3FE00000 00000000 C0140000 00000000
r 50F0.10   #                        RP / RFS
*Want "DIDBR rounding test 1h TR"  3FE00000 00000000 C0140000 00000000
r 5100.10   #                        RP / RNTE
*Want "DIDBR rounding test 1i NT"  3FE00000 00000000 C0140000 00000000
r 5110.10   #                        RP / RNTE
*Want "DIDBR rounding test 1i TR"  3FE00000 00000000 C0140000 00000000
r 5120.10   #                        RP / RZ
*Want "DIDBR rounding test 1j NT"  BFF80000 00000000 C0100000 00000000
r 5130.10   #                        RP / RZ
*Want "DIDBR rounding test 1j TR"  BFF80000 00000000 C0100000 00000000
r 5140.10   #                        RP / RP
*Want "DIDBR rounding test 1k NT"  BFF80000 00000000 C0100000 00000000
r 5150.10   #                        RP / RP
*Want "DIDBR rounding test 1k TR"  BFF80000 00000000 C0100000 00000000
r 5160.10   #                        RP / RM
*Want "DIDBR rounding test 1l NT"  3FE00000 00000000 C0140000 00000000
r 5170.10   #                        RP / RM
*Want "DIDBR rounding test 1l TR"  3FE00000 00000000 C0140000 00000000
r 5180.10   #                        RM / RNTA
*Want "DIDBR rounding test 1m NT"  3FE00000 00000000 C0140000 00000000
r 5190.10   #                        RM / RNTA
*Want "DIDBR rounding test 1m TR"  3FE00000 00000000 C0140000 00000000
r 51A0.10   #                        RM / RFS
*Want "DIDBR rounding test 1n NT"  3FE00000 00000000 C0140000 00000000
r 51B0.10   #                        RM / RFS
*Want "DIDBR rounding test 1n TR"  3FE00000 00000000 C0140000 00000000
r 51C0.10   #                        RM / RNTE
*Want "DIDBR rounding test 1o NT"  3FE00000 00000000 C0140000 00000000
r 51D0.10   #                        RM / RNTE
*Want "DIDBR rounding test 1o TR"  3FE00000 00000000 C0140000 00000000
r 51E0.10   #                        RM / RZ
*Want "DIDBR rounding test 1p NT"  BFF80000 00000000 C0100000 00000000
r 51F0.10   #                        RM / RZ
*Want "DIDBR rounding test 1p TR"  BFF80000 00000000 C0100000 00000000
r 5200.10   #                        RM / RP
*Want "DIDBR rounding test 1q NT"  BFF80000 00000000 C0100000 00000000
r 5210.10   #                        RM / RP
*Want "DIDBR rounding test 1q TR"  BFF80000 00000000 C0100000 00000000
r 5220.10   #                        RM / RM
*Want "DIDBR rounding test 1r NT"  3FE00000 00000000 C0140000 00000000
r 5230.10   #                        RM / RM
*Want "DIDBR rounding test 1r TR"  3FE00000 00000000 C0140000 00000000
r 5240.10   #                        RFS / RNTA
*Want "DIDBR rounding test 1s NT"  3FE00000 00000000 C0140000 00000000
r 5250.10   #                        RFS / RNTA
*Want "DIDBR rounding test 1s TR"  3FE00000 00000000 C0140000 00000000
r 5260.10   #                        RFS / RFS
*Want "DIDBR rounding test 1t NT"  3FE00000 00000000 C0140000 00000000
r 5270.10   #                        RFS / RFS
*Want "DIDBR rounding test 1t TR"  3FE00000 00000000 C0140000 00000000
r 5280.10   #                        RFS / RNTE
*Want "DIDBR rounding test 1u NT"  3FE00000 00000000 C0140000 00000000
r 5290.10   #                        RFS / RNTE
*Want "DIDBR rounding test 1u TR"  3FE00000 00000000 C0140000 00000000
r 52A0.10   #                        RFS / RZ
*Want "DIDBR rounding test 1v NT"  BFF80000 00000000 C0100000 00000000
r 52B0.10   #                        RFS / RZ
*Want "DIDBR rounding test 1v TR"  BFF80000 00000000 C0100000 00000000
r 52C0.10   #                        RFS / RP
*Want "DIDBR rounding test 1w NT"  BFF80000 00000000 C0100000 00000000
r 52D0.10   #                        RFS / RP
*Want "DIDBR rounding test 1w TR"  BFF80000 00000000 C0100000 00000000
r 52E0.10   #                        RFS / RM
*Want "DIDBR rounding test 1x NT"  3FE00000 00000000 C0140000 00000000
r 52F0.10   #                        RFS / RM
*Want "DIDBR rounding test 1x TR"  3FE00000 00000000 C0140000 00000000
r 5300.10   #                        RZ / RNTA
*Want "DIDBR rounding test 2a NT"  3FE00000 00000000 C0080000 00000000
r 5310.10   #                        RZ / RNTA
*Want "DIDBR rounding test 2a TR"  3FE00000 00000000 C0080000 00000000
r 5320.10   #                        RZ / RFS
*Want "DIDBR rounding test 2b NT"  3FE00000 00000000 C0080000 00000000
r 5330.10   #                        RZ / RFS
*Want "DIDBR rounding test 2b TR"  3FE00000 00000000 C0080000 00000000
r 5340.10   #                        RZ / RNTE
*Want "DIDBR rounding test 2c NT"  3FE00000 00000000 C0080000 00000000
r 5350.10   #                        RZ / RNTE
*Want "DIDBR rounding test 2c TR"  3FE00000 00000000 C0080000 00000000
r 5360.10   #                        RZ / RZ
*Want "DIDBR rounding test 2d NT"  BFF80000 00000000 C0000000 00000000
r 5370.10   #                        RZ / RZ
*Want "DIDBR rounding test 2d TR"  BFF80000 00000000 C0000000 00000000
r 5380.10   #                        RZ / RP
*Want "DIDBR rounding test 2e NT"  BFF80000 00000000 C0000000 00000000
r 5390.10   #                        RZ / RP
*Want "DIDBR rounding test 2e TR"  BFF80000 00000000 C0000000 00000000
r 53A0.10   #                        RZ / RM
*Want "DIDBR rounding test 2f NT"  3FE00000 00000000 C0080000 00000000
r 53B0.10   #                        RZ / RM
*Want "DIDBR rounding test 2f TR"  3FE00000 00000000 C0080000 00000000
r 53C0.10   #                        RP / RNTA
*Want "DIDBR rounding test 2g NT"  3FE00000 00000000 C0080000 00000000
r 53D0.10   #                        RP / RNTA
*Want "DIDBR rounding test 2g TR"  3FE00000 00000000 C0080000 00000000
r 53E0.10   #                        RP / RFS
*Want "DIDBR rounding test 2h NT"  3FE00000 00000000 C0080000 00000000
r 53F0.10   #                        RP / RFS
*Want "DIDBR rounding test 2h TR"  3FE00000 00000000 C0080000 00000000
r 5400.10   #                        RP / RNTE
*Want "DIDBR rounding test 2i NT"  3FE00000 00000000 C0080000 00000000
r 5410.10   #                        RP / RNTE
*Want "DIDBR rounding test 2i TR"  3FE00000 00000000 C0080000 00000000
r 5420.10   #                        RP / RZ
*Want "DIDBR rounding test 2j NT"  BFF80000 00000000 C0000000 00000000
r 5430.10   #                        RP / RZ
*Want "DIDBR rounding test 2j TR"  BFF80000 00000000 C0000000 00000000
r 5440.10   #                        RP / RP
*Want "DIDBR rounding test 2k NT"  BFF80000 00000000 C0000000 00000000
r 5450.10   #                        RP / RP
*Want "DIDBR rounding test 2k TR"  BFF80000 00000000 C0000000 00000000
r 5460.10   #                        RP / RM
*Want "DIDBR rounding test 2l NT"  3FE00000 00000000 C0080000 00000000
r 5470.10   #                        RP / RM
*Want "DIDBR rounding test 2l TR"  3FE00000 00000000 C0080000 00000000
r 5480.10   #                        RM / RNTA
*Want "DIDBR rounding test 2m NT"  3FE00000 00000000 C0080000 00000000
r 5490.10   #                        RM / RNTA
*Want "DIDBR rounding test 2m TR"  3FE00000 00000000 C0080000 00000000
r 54A0.10   #                        RM / RFS
*Want "DIDBR rounding test 2n NT"  3FE00000 00000000 C0080000 00000000
r 54B0.10   #                        RM / RFS
*Want "DIDBR rounding test 2n TR"  3FE00000 00000000 C0080000 00000000
r 54C0.10   #                        RM / RNTE
*Want "DIDBR rounding test 2o NT"  3FE00000 00000000 C0080000 00000000
r 54D0.10   #                        RM / RNTE
*Want "DIDBR rounding test 2o TR"  3FE00000 00000000 C0080000 00000000
r 54E0.10   #                        RM / RZ
*Want "DIDBR rounding test 2p NT"  BFF80000 00000000 C0000000 00000000
r 54F0.10   #                        RM / RZ
*Want "DIDBR rounding test 2p TR"  BFF80000 00000000 C0000000 00000000
r 5500.10   #                        RM / RP
*Want "DIDBR rounding test 2q NT"  BFF80000 00000000 C0000000 00000000
r 5510.10   #                        RM / RP
*Want "DIDBR rounding test 2q TR"  BFF80000 00000000 C0000000 00000000
r 5520.10   #                        RM / RM
*Want "DIDBR rounding test 2r NT"  3FE00000 00000000 C0080000 00000000
r 5530.10   #                        RM / RM
*Want "DIDBR rounding test 2r TR"  3FE00000 00000000 C0080000 00000000
r 5540.10   #                        RFS / RNTA
*Want "DIDBR rounding test 2s NT"  3FE00000 00000000 C0080000 00000000
r 5550.10   #                        RFS / RNTA
*Want "DIDBR rounding test 2s TR"  3FE00000 00000000 C0080000 00000000
r 5560.10   #                        RFS / RFS
*Want "DIDBR rounding test 2t NT"  3FE00000 00000000 C0080000 00000000
r 5570.10   #                        RFS / RFS
*Want "DIDBR rounding test 2t TR"  3FE00000 00000000 C0080000 00000000
r 5580.10   #                        RFS / RNTE
*Want "DIDBR rounding test 2u NT"  3FE00000 00000000 C0080000 00000000
r 5590.10   #                        RFS / RNTE
*Want "DIDBR rounding test 2u TR"  3FE00000 00000000 C0080000 00000000
r 55A0.10   #                        RFS / RZ
*Want "DIDBR rounding test 2v NT"  BFF80000 00000000 C0000000 00000000
r 55B0.10   #                        RFS / RZ
*Want "DIDBR rounding test 2v TR"  BFF80000 00000000 C0000000 00000000
r 55C0.10   #                        RFS / RP
*Want "DIDBR rounding test 2w NT"  BFF80000 00000000 C0000000 00000000
r 55D0.10   #                        RFS / RP
*Want "DIDBR rounding test 2w TR"  BFF80000 00000000 C0000000 00000000
r 55E0.10   #                        RFS / RM
*Want "DIDBR rounding test 2x NT"  3FE00000 00000000 C0080000 00000000
r 55F0.10   #                        RFS / RM
*Want "DIDBR rounding test 2x TR"  3FE00000 00000000 C0080000 00000000
r 5600.10   #                        RZ / RNTA
*Want "DIDBR rounding test 3a NT"  BFE00000 00000000 BFF00000 00000000
r 5610.10   #                        RZ / RNTA
*Want "DIDBR rounding test 3a TR"  BFE00000 00000000 BFF00000 00000000
r 5620.10   #                        RZ / RFS
*Want "DIDBR rounding test 3b NT"  BFE00000 00000000 BFF00000 00000000
r 5630.10   #                        RZ / RFS
*Want "DIDBR rounding test 3b TR"  BFE00000 00000000 BFF00000 00000000
r 5640.10   #                        RZ / RNTE
*Want "DIDBR rounding test 3c NT"  BFE00000 00000000 BFF00000 00000000
r 5650.10   #                        RZ / RNTE
*Want "DIDBR rounding test 3c TR"  BFE00000 00000000 BFF00000 00000000
r 5660.10   #                        RZ / RZ
*Want "DIDBR rounding test 3d NT"  BFE00000 00000000 BFF00000 00000000
r 5670.10   #                        RZ / RZ
*Want "DIDBR rounding test 3d TR"  BFE00000 00000000 BFF00000 00000000
r 5680.10   #                        RZ / RP
*Want "DIDBR rounding test 3e NT"  BFE00000 00000000 BFF00000 00000000
r 5690.10   #                        RZ / RP
*Want "DIDBR rounding test 3e TR"  BFE00000 00000000 BFF00000 00000000
r 56A0.10   #                        RZ / RM
*Want "DIDBR rounding test 3f NT"  3FF80000 00000000 C0000000 00000000
r 56B0.10   #                        RZ / RM
*Want "DIDBR rounding test 3f TR"  3FF80000 00000000 C0000000 00000000
r 56C0.10   #                        RP / RNTA
*Want "DIDBR rounding test 3g NT"  BFE00000 00000000 BFF00000 00000000
r 56D0.10   #                        RP / RNTA
*Want "DIDBR rounding test 3g TR"  BFE00000 00000000 BFF00000 00000000
r 56E0.10   #                        RP / RFS
*Want "DIDBR rounding test 3h NT"  BFE00000 00000000 BFF00000 00000000
r 56F0.10   #                        RP / RFS
*Want "DIDBR rounding test 3h TR"  BFE00000 00000000 BFF00000 00000000
r 5700.10   #                        RP / RNTE
*Want "DIDBR rounding test 3i NT"  BFE00000 00000000 BFF00000 00000000
r 5710.10   #                        RP / RNTE
*Want "DIDBR rounding test 3i TR"  BFE00000 00000000 BFF00000 00000000
r 5720.10   #                        RP / RZ
*Want "DIDBR rounding test 3j NT"  BFE00000 00000000 BFF00000 00000000
r 5730.10   #                        RP / RZ
*Want "DIDBR rounding test 3j TR"  BFE00000 00000000 BFF00000 00000000
r 5740.10   #                        RP / RP
*Want "DIDBR rounding test 3k NT"  BFE00000 00000000 BFF00000 00000000
r 5750.10   #                        RP / RP
*Want "DIDBR rounding test 3k TR"  BFE00000 00000000 BFF00000 00000000
r 5760.10   #                        RP / RM
*Want "DIDBR rounding test 3l NT"  3FF80000 00000000 C0000000 00000000
r 5770.10   #                        RP / RM
*Want "DIDBR rounding test 3l TR"  3FF80000 00000000 C0000000 00000000
r 5780.10   #                        RM / RNTA
*Want "DIDBR rounding test 3m NT"  BFE00000 00000000 BFF00000 00000000
r 5790.10   #                        RM / RNTA
*Want "DIDBR rounding test 3m TR"  BFE00000 00000000 BFF00000 00000000
r 57A0.10   #                        RM / RFS
*Want "DIDBR rounding test 3n NT"  BFE00000 00000000 BFF00000 00000000
r 57B0.10   #                        RM / RFS
*Want "DIDBR rounding test 3n TR"  BFE00000 00000000 BFF00000 00000000
r 57C0.10   #                        RM / RNTE
*Want "DIDBR rounding test 3o NT"  BFE00000 00000000 BFF00000 00000000
r 57D0.10   #                        RM / RNTE
*Want "DIDBR rounding test 3o TR"  BFE00000 00000000 BFF00000 00000000
r 57E0.10   #                        RM / RZ
*Want "DIDBR rounding test 3p NT"  BFE00000 00000000 BFF00000 00000000
r 57F0.10   #                        RM / RZ
*Want "DIDBR rounding test 3p TR"  BFE00000 00000000 BFF00000 00000000
r 5800.10   #                        RM / RP
*Want "DIDBR rounding test 3q NT"  BFE00000 00000000 BFF00000 00000000
r 5810.10   #                        RM / RP
*Want "DIDBR rounding test 3q TR"  BFE00000 00000000 BFF00000 00000000
r 5820.10   #                        RM / RM
*Want "DIDBR rounding test 3r NT"  3FF80000 00000000 C0000000 00000000
r 5830.10   #                        RM / RM
*Want "DIDBR rounding test 3r TR"  3FF80000 00000000 C0000000 00000000
r 5840.10   #                        RFS / RNTA
*Want "DIDBR rounding test 3s NT"  BFE00000 00000000 BFF00000 00000000
r 5850.10   #                        RFS / RNTA
*Want "DIDBR rounding test 3s TR"  BFE00000 00000000 BFF00000 00000000
r 5860.10   #                        RFS / RFS
*Want "DIDBR rounding test 3t NT"  BFE00000 00000000 BFF00000 00000000
r 5870.10   #                        RFS / RFS
*Want "DIDBR rounding test 3t TR"  BFE00000 00000000 BFF00000 00000000
r 5880.10   #                        RFS / RNTE
*Want "DIDBR rounding test 3u NT"  BFE00000 00000000 BFF00000 00000000
r 5890.10   #                        RFS / RNTE
*Want "DIDBR rounding test 3u TR"  BFE00000 00000000 BFF00000 00000000
r 58A0.10   #                        RFS / RZ
*Want "DIDBR rounding test 3v NT"  BFE00000 00000000 BFF00000 00000000
r 58B0.10   #                        RFS / RZ
*Want "DIDBR rounding test 3v TR"  BFE00000 00000000 BFF00000 00000000
r 58C0.10   #                        RFS / RP
*Want "DIDBR rounding test 3w NT"  BFE00000 00000000 BFF00000 00000000
r 58D0.10   #                        RFS / RP
*Want "DIDBR rounding test 3w TR"  BFE00000 00000000 BFF00000 00000000
r 58E0.10   #                        RFS / RM
*Want "DIDBR rounding test 3x NT"  3FF80000 00000000 C0000000 00000000
r 58F0.10   #                        RFS / RM
*Want "DIDBR rounding test 3x TR"  3FF80000 00000000 C0000000 00000000
r 5900.10   #                        RZ / RNTA
*Want "DIDBR rounding test 4a NT"  3FE00000 00000000 BFF00000 00000000
r 5910.10   #                        RZ / RNTA
*Want "DIDBR rounding test 4a TR"  3FE00000 00000000 BFF00000 00000000
r 5920.10   #                        RZ / RFS
*Want "DIDBR rounding test 4b NT"  3FE00000 00000000 BFF00000 00000000
r 5930.10   #                        RZ / RFS
*Want "DIDBR rounding test 4b TR"  3FE00000 00000000 BFF00000 00000000
r 5940.10   #                        RZ / RNTE
*Want "DIDBR rounding test 4c NT"  3FE00000 00000000 BFF00000 00000000
r 5950.10   #                        RZ / RNTE
*Want "DIDBR rounding test 4c TR"  3FE00000 00000000 BFF00000 00000000
r 5960.10   #                        RZ / RZ
*Want "DIDBR rounding test 4d NT"  BFF80000 00000000 80000000 00000000
r 5970.10   #                        RZ / RZ
*Want "DIDBR rounding test 4d TR"  BFF80000 00000000 80000000 00000000
r 5980.10   #                        RZ / RP
*Want "DIDBR rounding test 4e NT"  BFF80000 00000000 80000000 00000000
r 5990.10   #                        RZ / RP
*Want "DIDBR rounding test 4e TR"  BFF80000 00000000 80000000 00000000
r 59A0.10   #                        RZ / RM
*Want "DIDBR rounding test 4f NT"  3FE00000 00000000 BFF00000 00000000
r 59B0.10   #                        RZ / RM
*Want "DIDBR rounding test 4f TR"  3FE00000 00000000 BFF00000 00000000
r 59C0.10   #                        RP / RNTA
*Want "DIDBR rounding test 4g NT"  3FE00000 00000000 BFF00000 00000000
r 59D0.10   #                        RP / RNTA
*Want "DIDBR rounding test 4g TR"  3FE00000 00000000 BFF00000 00000000
r 59E0.10   #                        RP / RFS
*Want "DIDBR rounding test 4h NT"  3FE00000 00000000 BFF00000 00000000
r 59F0.10   #                        RP / RFS
*Want "DIDBR rounding test 4h TR"  3FE00000 00000000 BFF00000 00000000
r 5A00.10   #                        RP / RNTE
*Want "DIDBR rounding test 4i NT"  3FE00000 00000000 BFF00000 00000000
r 5A10.10   #                        RP / RNTE
*Want "DIDBR rounding test 4i TR"  3FE00000 00000000 BFF00000 00000000
r 5A20.10   #                        RP / RZ
*Want "DIDBR rounding test 4j NT"  BFF80000 00000000 80000000 00000000
r 5A30.10   #                        RP / RZ
*Want "DIDBR rounding test 4j TR"  BFF80000 00000000 80000000 00000000
r 5A40.10   #                        RP / RP
*Want "DIDBR rounding test 4k NT"  BFF80000 00000000 80000000 00000000
r 5A50.10   #                        RP / RP
*Want "DIDBR rounding test 4k TR"  BFF80000 00000000 80000000 00000000
r 5A60.10   #                        RP / RM
*Want "DIDBR rounding test 4l NT"  3FE00000 00000000 BFF00000 00000000
r 5A70.10   #                        RP / RM
*Want "DIDBR rounding test 4l TR"  3FE00000 00000000 BFF00000 00000000
r 5A80.10   #                        RM / RNTA
*Want "DIDBR rounding test 4m NT"  3FE00000 00000000 BFF00000 00000000
r 5A90.10   #                        RM / RNTA
*Want "DIDBR rounding test 4m TR"  3FE00000 00000000 BFF00000 00000000
r 5AA0.10   #                        RM / RFS
*Want "DIDBR rounding test 4n NT"  3FE00000 00000000 BFF00000 00000000
r 5AB0.10   #                        RM / RFS
*Want "DIDBR rounding test 4n TR"  3FE00000 00000000 BFF00000 00000000
r 5AC0.10   #                        RM / RNTE
*Want "DIDBR rounding test 4o NT"  3FE00000 00000000 BFF00000 00000000
r 5AD0.10   #                        RM / RNTE
*Want "DIDBR rounding test 4o TR"  3FE00000 00000000 BFF00000 00000000
r 5AE0.10   #                        RM / RZ
*Want "DIDBR rounding test 4p NT"  BFF80000 00000000 80000000 00000000
r 5AF0.10   #                        RM / RZ
*Want "DIDBR rounding test 4p TR"  BFF80000 00000000 80000000 00000000
r 5B00.10   #                        RM / RP
*Want "DIDBR rounding test 4q NT"  BFF80000 00000000 80000000 00000000
r 5B10.10   #                        RM / RP
*Want "DIDBR rounding test 4q TR"  BFF80000 00000000 80000000 00000000
r 5B20.10   #                        RM / RM
*Want "DIDBR rounding test 4r NT"  3FE00000 00000000 BFF00000 00000000
r 5B30.10   #                        RM / RM
*Want "DIDBR rounding test 4r TR"  3FE00000 00000000 BFF00000 00000000
r 5B40.10   #                        RFS / RNTA
*Want "DIDBR rounding test 4s NT"  3FE00000 00000000 BFF00000 00000000
r 5B50.10   #                        RFS / RNTA
*Want "DIDBR rounding test 4s TR"  3FE00000 00000000 BFF00000 00000000
r 5B60.10   #                        RFS / RFS
*Want "DIDBR rounding test 4t NT"  3FE00000 00000000 BFF00000 00000000
r 5B70.10   #                        RFS / RFS
*Want "DIDBR rounding test 4t TR"  3FE00000 00000000 BFF00000 00000000
r 5B80.10   #                        RFS / RNTE
*Want "DIDBR rounding test 4u NT"  3FE00000 00000000 BFF00000 00000000
r 5B90.10   #                        RFS / RNTE
*Want "DIDBR rounding test 4u TR"  3FE00000 00000000 BFF00000 00000000
r 5BA0.10   #                        RFS / RZ
*Want "DIDBR rounding test 4v NT"  BFF80000 00000000 80000000 00000000
r 5BB0.10   #                        RFS / RZ
*Want "DIDBR rounding test 4v TR"  BFF80000 00000000 80000000 00000000
r 5BC0.10   #                        RFS / RP
*Want "DIDBR rounding test 4w NT"  BFF80000 00000000 80000000 00000000
r 5BD0.10   #                        RFS / RP
*Want "DIDBR rounding test 4w TR"  BFF80000 00000000 80000000 00000000
r 5BE0.10   #                        RFS / RM
*Want "DIDBR rounding test 4x NT"  3FE00000 00000000 BFF00000 00000000
r 5BF0.10   #                        RFS / RM
*Want "DIDBR rounding test 4x TR"  3FE00000 00000000 BFF00000 00000000
r 5C00.10   #                        RZ / RNTA
*Want "DIDBR rounding test 5a NT"  BFE00000 00000000 80000000 00000000
r 5C10.10   #                        RZ / RNTA
*Want "DIDBR rounding test 5a TR"  BFE00000 00000000 80000000 00000000
r 5C20.10   #                        RZ / RFS
*Want "DIDBR rounding test 5b NT"  3FF80000 00000000 BFF00000 00000000
r 5C30.10   #                        RZ / RFS
*Want "DIDBR rounding test 5b TR"  3FF80000 00000000 BFF00000 00000000
r 5C40.10   #                        RZ / RNTE
*Want "DIDBR rounding test 5c NT"  BFE00000 00000000 80000000 00000000
r 5C50.10   #                        RZ / RNTE
*Want "DIDBR rounding test 5c TR"  BFE00000 00000000 80000000 00000000
r 5C60.10   #                        RZ / RZ
*Want "DIDBR rounding test 5d NT"  BFE00000 00000000 80000000 00000000
r 5C70.10   #                        RZ / RZ
*Want "DIDBR rounding test 5d TR"  BFE00000 00000000 80000000 00000000
r 5C80.10   #                        RZ / RP
*Want "DIDBR rounding test 5e NT"  BFE00000 00000000 80000000 00000000
r 5C90.10   #                        RZ / RP
*Want "DIDBR rounding test 5e TR"  BFE00000 00000000 80000000 00000000
r 5CA0.10   #                        RZ / RM
*Want "DIDBR rounding test 5f NT"  3FF80000 00000000 BFF00000 00000000
r 5CB0.10   #                        RZ / RM
*Want "DIDBR rounding test 5f TR"  3FF80000 00000000 BFF00000 00000000
r 5CC0.10   #                        RP / RNTA
*Want "DIDBR rounding test 5g NT"  BFE00000 00000000 80000000 00000000
r 5CD0.10   #                        RP / RNTA
*Want "DIDBR rounding test 5g TR"  BFE00000 00000000 80000000 00000000
r 5CE0.10   #                        RP / RFS
*Want "DIDBR rounding test 5h NT"  3FF80000 00000000 BFF00000 00000000
r 5CF0.10   #                        RP / RFS
*Want "DIDBR rounding test 5h TR"  3FF80000 00000000 BFF00000 00000000
r 5D00.10   #                        RP / RNTE
*Want "DIDBR rounding test 5i NT"  BFE00000 00000000 80000000 00000000
r 5D10.10   #                        RP / RNTE
*Want "DIDBR rounding test 5i TR"  BFE00000 00000000 80000000 00000000
r 5D20.10   #                        RP / RZ
*Want "DIDBR rounding test 5j NT"  BFE00000 00000000 80000000 00000000
r 5D30.10   #                        RP / RZ
*Want "DIDBR rounding test 5j TR"  BFE00000 00000000 80000000 00000000
r 5D40.10   #                        RP / RP
*Want "DIDBR rounding test 5k NT"  BFE00000 00000000 80000000 00000000
r 5D50.10   #                        RP / RP
*Want "DIDBR rounding test 5k TR"  BFE00000 00000000 80000000 00000000
r 5D60.10   #                        RP / RM
*Want "DIDBR rounding test 5l NT"  3FF80000 00000000 BFF00000 00000000
r 5D70.10   #                        RP / RM
*Want "DIDBR rounding test 5l TR"  3FF80000 00000000 BFF00000 00000000
r 5D80.10   #                        RM / RNTA
*Want "DIDBR rounding test 5m NT"  BFE00000 00000000 80000000 00000000
r 5D90.10   #                        RM / RNTA
*Want "DIDBR rounding test 5m TR"  BFE00000 00000000 80000000 00000000
r 5DA0.10   #                        RM / RFS
*Want "DIDBR rounding test 5n NT"  3FF80000 00000000 BFF00000 00000000
r 5DB0.10   #                        RM / RFS
*Want "DIDBR rounding test 5n TR"  3FF80000 00000000 BFF00000 00000000
r 5DC0.10   #                        RM / RNTE
*Want "DIDBR rounding test 5o NT"  BFE00000 00000000 80000000 00000000
r 5DD0.10   #                        RM / RNTE
*Want "DIDBR rounding test 5o TR"  BFE00000 00000000 80000000 00000000
r 5DE0.10   #                        RM / RZ
*Want "DIDBR rounding test 5p NT"  BFE00000 00000000 80000000 00000000
r 5DF0.10   #                        RM / RZ
*Want "DIDBR rounding test 5p TR"  BFE00000 00000000 80000000 00000000
r 5E00.10   #                        RM / RP
*Want "DIDBR rounding test 5q NT"  BFE00000 00000000 80000000 00000000
r 5E10.10   #                        RM / RP
*Want "DIDBR rounding test 5q TR"  BFE00000 00000000 80000000 00000000
r 5E20.10   #                        RM / RM
*Want "DIDBR rounding test 5r NT"  3FF80000 00000000 BFF00000 00000000
r 5E30.10   #                        RM / RM
*Want "DIDBR rounding test 5r TR"  3FF80000 00000000 BFF00000 00000000
r 5E40.10   #                        RFS / RNTA
*Want "DIDBR rounding test 5s NT"  BFE00000 00000000 80000000 00000000
r 5E50.10   #                        RFS / RNTA
*Want "DIDBR rounding test 5s TR"  BFE00000 00000000 80000000 00000000
r 5E60.10   #                        RFS / RFS
*Want "DIDBR rounding test 5t NT"  3FF80000 00000000 BFF00000 00000000
r 5E70.10   #                        RFS / RFS
*Want "DIDBR rounding test 5t TR"  3FF80000 00000000 BFF00000 00000000
r 5E80.10   #                        RFS / RNTE
*Want "DIDBR rounding test 5u NT"  BFE00000 00000000 80000000 00000000
r 5E90.10   #                        RFS / RNTE
*Want "DIDBR rounding test 5u TR"  BFE00000 00000000 80000000 00000000
r 5EA0.10   #                        RFS / RZ
*Want "DIDBR rounding test 5v NT"  BFE00000 00000000 80000000 00000000
r 5EB0.10   #                        RFS / RZ
*Want "DIDBR rounding test 5v TR"  BFE00000 00000000 80000000 00000000
r 5EC0.10   #                        RFS / RP
*Want "DIDBR rounding test 5w NT"  BFE00000 00000000 80000000 00000000
r 5ED0.10   #                        RFS / RP
*Want "DIDBR rounding test 5w TR"  BFE00000 00000000 80000000 00000000
r 5EE0.10   #                        RFS / RM
*Want "DIDBR rounding test 5x NT"  3FF80000 00000000 BFF00000 00000000
r 5EF0.10   #                        RFS / RM
*Want "DIDBR rounding test 5x TR"  3FF80000 00000000 BFF00000 00000000
r 5F00.10   #                        RZ / RNTA
*Want "DIDBR rounding test 6a NT"  3FE00000 00000000 00000000 00000000
r 5F10.10   #                        RZ / RNTA
*Want "DIDBR rounding test 6a TR"  3FE00000 00000000 00000000 00000000
r 5F20.10   #                        RZ / RFS
*Want "DIDBR rounding test 6b NT"  BFF80000 00000000 3FF00000 00000000
r 5F30.10   #                        RZ / RFS
*Want "DIDBR rounding test 6b TR"  BFF80000 00000000 3FF00000 00000000
r 5F40.10   #                        RZ / RNTE
*Want "DIDBR rounding test 6c NT"  3FE00000 00000000 00000000 00000000
r 5F50.10   #                        RZ / RNTE
*Want "DIDBR rounding test 6c TR"  3FE00000 00000000 00000000 00000000
r 5F60.10   #                        RZ / RZ
*Want "DIDBR rounding test 6d NT"  3FE00000 00000000 00000000 00000000
r 5F70.10   #                        RZ / RZ
*Want "DIDBR rounding test 6d TR"  3FE00000 00000000 00000000 00000000
r 5F80.10   #                        RZ / RP
*Want "DIDBR rounding test 6e NT"  BFF80000 00000000 3FF00000 00000000
r 5F90.10   #                        RZ / RP
*Want "DIDBR rounding test 6e TR"  BFF80000 00000000 3FF00000 00000000
r 5FA0.10   #                        RZ / RM
*Want "DIDBR rounding test 6f NT"  3FE00000 00000000 00000000 00000000
r 5FB0.10   #                        RZ / RM
*Want "DIDBR rounding test 6f TR"  3FE00000 00000000 00000000 00000000
r 5FC0.10   #                        RP / RNTA
*Want "DIDBR rounding test 6g NT"  3FE00000 00000000 00000000 00000000
r 5FD0.10   #                        RP / RNTA
*Want "DIDBR rounding test 6g TR"  3FE00000 00000000 00000000 00000000
r 5FE0.10   #                        RP / RFS
*Want "DIDBR rounding test 6h NT"  BFF80000 00000000 3FF00000 00000000
r 5FF0.10   #                        RP / RFS
*Want "DIDBR rounding test 6h TR"  BFF80000 00000000 3FF00000 00000000
r 6000.10   #                        RP / RNTE
*Want "DIDBR rounding test 6i NT"  3FE00000 00000000 00000000 00000000
r 6010.10   #                        RP / RNTE
*Want "DIDBR rounding test 6i TR"  3FE00000 00000000 00000000 00000000
r 6020.10   #                        RP / RZ
*Want "DIDBR rounding test 6j NT"  3FE00000 00000000 00000000 00000000
r 6030.10   #                        RP / RZ
*Want "DIDBR rounding test 6j TR"  3FE00000 00000000 00000000 00000000
r 6040.10   #                        RP / RP
*Want "DIDBR rounding test 6k NT"  BFF80000 00000000 3FF00000 00000000
r 6050.10   #                        RP / RP
*Want "DIDBR rounding test 6k TR"  BFF80000 00000000 3FF00000 00000000
r 6060.10   #                        RP / RM
*Want "DIDBR rounding test 6l NT"  3FE00000 00000000 00000000 00000000
r 6070.10   #                        RP / RM
*Want "DIDBR rounding test 6l TR"  3FE00000 00000000 00000000 00000000
r 6080.10   #                        RM / RNTA
*Want "DIDBR rounding test 6m NT"  3FE00000 00000000 00000000 00000000
r 6090.10   #                        RM / RNTA
*Want "DIDBR rounding test 6m TR"  3FE00000 00000000 00000000 00000000
r 60A0.10   #                        RM / RFS
*Want "DIDBR rounding test 6n NT"  BFF80000 00000000 3FF00000 00000000
r 60B0.10   #                        RM / RFS
*Want "DIDBR rounding test 6n TR"  BFF80000 00000000 3FF00000 00000000
r 60C0.10   #                        RM / RNTE
*Want "DIDBR rounding test 6o NT"  3FE00000 00000000 00000000 00000000
r 60D0.10   #                        RM / RNTE
*Want "DIDBR rounding test 6o TR"  3FE00000 00000000 00000000 00000000
r 60E0.10   #                        RM / RZ
*Want "DIDBR rounding test 6p NT"  3FE00000 00000000 00000000 00000000
r 60F0.10   #                        RM / RZ
*Want "DIDBR rounding test 6p TR"  3FE00000 00000000 00000000 00000000
r 6100.10   #                        RM / RP
*Want "DIDBR rounding test 6q NT"  BFF80000 00000000 3FF00000 00000000
r 6110.10   #                        RM / RP
*Want "DIDBR rounding test 6q TR"  BFF80000 00000000 3FF00000 00000000
r 6120.10   #                        RM / RM
*Want "DIDBR rounding test 6r NT"  3FE00000 00000000 00000000 00000000
r 6130.10   #                        RM / RM
*Want "DIDBR rounding test 6r TR"  3FE00000 00000000 00000000 00000000
r 6140.10   #                        RFS / RNTA
*Want "DIDBR rounding test 6s NT"  3FE00000 00000000 00000000 00000000
r 6150.10   #                        RFS / RNTA
*Want "DIDBR rounding test 6s TR"  3FE00000 00000000 00000000 00000000
r 6160.10   #                        RFS / RFS
*Want "DIDBR rounding test 6t NT"  BFF80000 00000000 3FF00000 00000000
r 6170.10   #                        RFS / RFS
*Want "DIDBR rounding test 6t TR"  BFF80000 00000000 3FF00000 00000000
r 6180.10   #                        RFS / RNTE
*Want "DIDBR rounding test 6u NT"  3FE00000 00000000 00000000 00000000
r 6190.10   #                        RFS / RNTE
*Want "DIDBR rounding test 6u TR"  3FE00000 00000000 00000000 00000000
r 61A0.10   #                        RFS / RZ
*Want "DIDBR rounding test 6v NT"  3FE00000 00000000 00000000 00000000
r 61B0.10   #                        RFS / RZ
*Want "DIDBR rounding test 6v TR"  3FE00000 00000000 00000000 00000000
r 61C0.10   #                        RFS / RP
*Want "DIDBR rounding test 6w NT"  BFF80000 00000000 3FF00000 00000000
r 61D0.10   #                        RFS / RP
*Want "DIDBR rounding test 6w TR"  BFF80000 00000000 3FF00000 00000000
r 61E0.10   #                        RFS / RM
*Want "DIDBR rounding test 6x NT"  3FE00000 00000000 00000000 00000000
r 61F0.10   #                        RFS / RM
*Want "DIDBR rounding test 6x TR"  3FE00000 00000000 00000000 00000000
r 6200.10   #                        RZ / RNTA
*Want "DIDBR rounding test 7a NT"  BFE00000 00000000 3FF00000 00000000
r 6210.10   #                        RZ / RNTA
*Want "DIDBR rounding test 7a TR"  BFE00000 00000000 3FF00000 00000000
r 6220.10   #                        RZ / RFS
*Want "DIDBR rounding test 7b NT"  BFE00000 00000000 3FF00000 00000000
r 6230.10   #                        RZ / RFS
*Want "DIDBR rounding test 7b TR"  BFE00000 00000000 3FF00000 00000000
r 6240.10   #                        RZ / RNTE
*Want "DIDBR rounding test 7c NT"  BFE00000 00000000 3FF00000 00000000
r 6250.10   #                        RZ / RNTE
*Want "DIDBR rounding test 7c TR"  BFE00000 00000000 3FF00000 00000000
r 6260.10   #                        RZ / RZ
*Want "DIDBR rounding test 7d NT"  3FF80000 00000000 00000000 00000000
r 6270.10   #                        RZ / RZ
*Want "DIDBR rounding test 7d TR"  3FF80000 00000000 00000000 00000000
r 6280.10   #                        RZ / RP
*Want "DIDBR rounding test 7e NT"  BFE00000 00000000 3FF00000 00000000
r 6290.10   #                        RZ / RP
*Want "DIDBR rounding test 7e TR"  BFE00000 00000000 3FF00000 00000000
r 62A0.10   #                        RZ / RM
*Want "DIDBR rounding test 7f NT"  3FF80000 00000000 00000000 00000000
r 62B0.10   #                        RZ / RM
*Want "DIDBR rounding test 7f TR"  3FF80000 00000000 00000000 00000000
r 62C0.10   #                        RP / RNTA
*Want "DIDBR rounding test 7g NT"  BFE00000 00000000 3FF00000 00000000
r 62D0.10   #                        RP / RNTA
*Want "DIDBR rounding test 7g TR"  BFE00000 00000000 3FF00000 00000000
r 62E0.10   #                        RP / RFS
*Want "DIDBR rounding test 7h NT"  BFE00000 00000000 3FF00000 00000000
r 62F0.10   #                        RP / RFS
*Want "DIDBR rounding test 7h TR"  BFE00000 00000000 3FF00000 00000000
r 6300.10   #                        RP / RNTE
*Want "DIDBR rounding test 7i NT"  BFE00000 00000000 3FF00000 00000000
r 6310.10   #                        RP / RNTE
*Want "DIDBR rounding test 7i TR"  BFE00000 00000000 3FF00000 00000000
r 6320.10   #                        RP / RZ
*Want "DIDBR rounding test 7j NT"  3FF80000 00000000 00000000 00000000
r 6330.10   #                        RP / RZ
*Want "DIDBR rounding test 7j TR"  3FF80000 00000000 00000000 00000000
r 6340.10   #                        RP / RP
*Want "DIDBR rounding test 7k NT"  BFE00000 00000000 3FF00000 00000000
r 6350.10   #                        RP / RP
*Want "DIDBR rounding test 7k TR"  BFE00000 00000000 3FF00000 00000000
r 6360.10   #                        RP / RM
*Want "DIDBR rounding test 7l NT"  3FF80000 00000000 00000000 00000000
r 6370.10   #                        RP / RM
*Want "DIDBR rounding test 7l TR"  3FF80000 00000000 00000000 00000000
r 6380.10   #                        RM / RNTA
*Want "DIDBR rounding test 7m NT"  BFE00000 00000000 3FF00000 00000000
r 6390.10   #                        RM / RNTA
*Want "DIDBR rounding test 7m TR"  BFE00000 00000000 3FF00000 00000000
r 63A0.10   #                        RM / RFS
*Want "DIDBR rounding test 7n NT"  BFE00000 00000000 3FF00000 00000000
r 63B0.10   #                        RM / RFS
*Want "DIDBR rounding test 7n TR"  BFE00000 00000000 3FF00000 00000000
r 63C0.10   #                        RM / RNTE
*Want "DIDBR rounding test 7o NT"  BFE00000 00000000 3FF00000 00000000
r 63D0.10   #                        RM / RNTE
*Want "DIDBR rounding test 7o TR"  BFE00000 00000000 3FF00000 00000000
r 63E0.10   #                        RM / RZ
*Want "DIDBR rounding test 7p NT"  3FF80000 00000000 00000000 00000000
r 63F0.10   #                        RM / RZ
*Want "DIDBR rounding test 7p TR"  3FF80000 00000000 00000000 00000000
r 6400.10   #                        RM / RP
*Want "DIDBR rounding test 7q NT"  BFE00000 00000000 3FF00000 00000000
r 6410.10   #                        RM / RP
*Want "DIDBR rounding test 7q TR"  BFE00000 00000000 3FF00000 00000000
r 6420.10   #                        RM / RM
*Want "DIDBR rounding test 7r NT"  3FF80000 00000000 00000000 00000000
r 6430.10   #                        RM / RM
*Want "DIDBR rounding test 7r TR"  3FF80000 00000000 00000000 00000000
r 6440.10   #                        RFS / RNTA
*Want "DIDBR rounding test 7s NT"  BFE00000 00000000 3FF00000 00000000
r 6450.10   #                        RFS / RNTA
*Want "DIDBR rounding test 7s TR"  BFE00000 00000000 3FF00000 00000000
r 6460.10   #                        RFS / RFS
*Want "DIDBR rounding test 7t NT"  BFE00000 00000000 3FF00000 00000000
r 6470.10   #                        RFS / RFS
*Want "DIDBR rounding test 7t TR"  BFE00000 00000000 3FF00000 00000000
r 6480.10   #                        RFS / RNTE
*Want "DIDBR rounding test 7u NT"  BFE00000 00000000 3FF00000 00000000
r 6490.10   #                        RFS / RNTE
*Want "DIDBR rounding test 7u TR"  BFE00000 00000000 3FF00000 00000000
r 64A0.10   #                        RFS / RZ
*Want "DIDBR rounding test 7v NT"  3FF80000 00000000 00000000 00000000
r 64B0.10   #                        RFS / RZ
*Want "DIDBR rounding test 7v TR"  3FF80000 00000000 00000000 00000000
r 64C0.10   #                        RFS / RP
*Want "DIDBR rounding test 7w NT"  BFE00000 00000000 3FF00000 00000000
r 64D0.10   #                        RFS / RP
*Want "DIDBR rounding test 7w TR"  BFE00000 00000000 3FF00000 00000000
r 64E0.10   #                        RFS / RM
*Want "DIDBR rounding test 7x NT"  3FF80000 00000000 00000000 00000000
r 64F0.10   #                        RFS / RM
*Want "DIDBR rounding test 7x TR"  3FF80000 00000000 00000000 00000000
r 6500.10   #                        RZ / RNTA
*Want "DIDBR rounding test 8a NT"  3FE00000 00000000 3FF00000 00000000
r 6510.10   #                        RZ / RNTA
*Want "DIDBR rounding test 8a TR"  3FE00000 00000000 3FF00000 00000000
r 6520.10   #                        RZ / RFS
*Want "DIDBR rounding test 8b NT"  3FE00000 00000000 3FF00000 00000000
r 6530.10   #                        RZ / RFS
*Want "DIDBR rounding test 8b TR"  3FE00000 00000000 3FF00000 00000000
r 6540.10   #                        RZ / RNTE
*Want "DIDBR rounding test 8c NT"  3FE00000 00000000 3FF00000 00000000
r 6550.10   #                        RZ / RNTE
*Want "DIDBR rounding test 8c TR"  3FE00000 00000000 3FF00000 00000000
r 6560.10   #                        RZ / RZ
*Want "DIDBR rounding test 8d NT"  3FE00000 00000000 3FF00000 00000000
r 6570.10   #                        RZ / RZ
*Want "DIDBR rounding test 8d TR"  3FE00000 00000000 3FF00000 00000000
r 6580.10   #                        RZ / RP
*Want "DIDBR rounding test 8e NT"  BFF80000 00000000 40000000 00000000
r 6590.10   #                        RZ / RP
*Want "DIDBR rounding test 8e TR"  BFF80000 00000000 40000000 00000000
r 65A0.10   #                        RZ / RM
*Want "DIDBR rounding test 8f NT"  3FE00000 00000000 3FF00000 00000000
r 65B0.10   #                        RZ / RM
*Want "DIDBR rounding test 8f TR"  3FE00000 00000000 3FF00000 00000000
r 65C0.10   #                        RP / RNTA
*Want "DIDBR rounding test 8g NT"  3FE00000 00000000 3FF00000 00000000
r 65D0.10   #                        RP / RNTA
*Want "DIDBR rounding test 8g TR"  3FE00000 00000000 3FF00000 00000000
r 65E0.10   #                        RP / RFS
*Want "DIDBR rounding test 8h NT"  3FE00000 00000000 3FF00000 00000000
r 65F0.10   #                        RP / RFS
*Want "DIDBR rounding test 8h TR"  3FE00000 00000000 3FF00000 00000000
r 6600.10   #                        RP / RNTE
*Want "DIDBR rounding test 8i NT"  3FE00000 00000000 3FF00000 00000000
r 6610.10   #                        RP / RNTE
*Want "DIDBR rounding test 8i TR"  3FE00000 00000000 3FF00000 00000000
r 6620.10   #                        RP / RZ
*Want "DIDBR rounding test 8j NT"  3FE00000 00000000 3FF00000 00000000
r 6630.10   #                        RP / RZ
*Want "DIDBR rounding test 8j TR"  3FE00000 00000000 3FF00000 00000000
r 6640.10   #                        RP / RP
*Want "DIDBR rounding test 8k NT"  BFF80000 00000000 40000000 00000000
r 6650.10   #                        RP / RP
*Want "DIDBR rounding test 8k TR"  BFF80000 00000000 40000000 00000000
r 6660.10   #                        RP / RM
*Want "DIDBR rounding test 8l NT"  3FE00000 00000000 3FF00000 00000000
r 6670.10   #                        RP / RM
*Want "DIDBR rounding test 8l TR"  3FE00000 00000000 3FF00000 00000000
r 6680.10   #                        RM / RNTA
*Want "DIDBR rounding test 8m NT"  3FE00000 00000000 3FF00000 00000000
r 6690.10   #                        RM / RNTA
*Want "DIDBR rounding test 8m TR"  3FE00000 00000000 3FF00000 00000000
r 66A0.10   #                        RM / RFS
*Want "DIDBR rounding test 8n NT"  3FE00000 00000000 3FF00000 00000000
r 66B0.10   #                        RM / RFS
*Want "DIDBR rounding test 8n TR"  3FE00000 00000000 3FF00000 00000000
r 66C0.10   #                        RM / RNTE
*Want "DIDBR rounding test 8o NT"  3FE00000 00000000 3FF00000 00000000
r 66D0.10   #                        RM / RNTE
*Want "DIDBR rounding test 8o TR"  3FE00000 00000000 3FF00000 00000000
r 66E0.10   #                        RM / RZ
*Want "DIDBR rounding test 8p NT"  3FE00000 00000000 3FF00000 00000000
r 66F0.10   #                        RM / RZ
*Want "DIDBR rounding test 8p TR"  3FE00000 00000000 3FF00000 00000000
r 6700.10   #                        RM / RP
*Want "DIDBR rounding test 8q NT"  BFF80000 00000000 40000000 00000000
r 6710.10   #                        RM / RP
*Want "DIDBR rounding test 8q TR"  BFF80000 00000000 40000000 00000000
r 6720.10   #                        RM / RM
*Want "DIDBR rounding test 8r NT"  3FE00000 00000000 3FF00000 00000000
r 6730.10   #                        RM / RM
*Want "DIDBR rounding test 8r TR"  3FE00000 00000000 3FF00000 00000000
r 6740.10   #                        RFS / RNTA
*Want "DIDBR rounding test 8s NT"  3FE00000 00000000 3FF00000 00000000
r 6750.10   #                        RFS / RNTA
*Want "DIDBR rounding test 8s TR"  3FE00000 00000000 3FF00000 00000000
r 6760.10   #                        RFS / RFS
*Want "DIDBR rounding test 8t NT"  3FE00000 00000000 3FF00000 00000000
r 6770.10   #                        RFS / RFS
*Want "DIDBR rounding test 8t TR"  3FE00000 00000000 3FF00000 00000000
r 6780.10   #                        RFS / RNTE
*Want "DIDBR rounding test 8u NT"  3FE00000 00000000 3FF00000 00000000
r 6790.10   #                        RFS / RNTE
*Want "DIDBR rounding test 8u TR"  3FE00000 00000000 3FF00000 00000000
r 67A0.10   #                        RFS / RZ
*Want "DIDBR rounding test 8v NT"  3FE00000 00000000 3FF00000 00000000
r 67B0.10   #                        RFS / RZ
*Want "DIDBR rounding test 8v TR"  3FE00000 00000000 3FF00000 00000000
r 67C0.10   #                        RFS / RP
*Want "DIDBR rounding test 8w NT"  BFF80000 00000000 40000000 00000000
r 67D0.10   #                        RFS / RP
*Want "DIDBR rounding test 8w TR"  BFF80000 00000000 40000000 00000000
r 67E0.10   #                        RFS / RM
*Want "DIDBR rounding test 8x NT"  3FE00000 00000000 3FF00000 00000000
r 67F0.10   #                        RFS / RM
*Want "DIDBR rounding test 8x TR"  3FE00000 00000000 3FF00000 00000000
r 6800.10   #                        RZ / RNTA
*Want "DIDBR rounding test 9a NT"  BFE00000 00000000 40080000 00000000
r 6810.10   #                        RZ / RNTA
*Want "DIDBR rounding test 9a TR"  BFE00000 00000000 40080000 00000000
r 6820.10   #                        RZ / RFS
*Want "DIDBR rounding test 9b NT"  BFE00000 00000000 40080000 00000000
r 6830.10   #                        RZ / RFS
*Want "DIDBR rounding test 9b TR"  BFE00000 00000000 40080000 00000000
r 6840.10   #                        RZ / RNTE
*Want "DIDBR rounding test 9c NT"  BFE00000 00000000 40080000 00000000
r 6850.10   #                        RZ / RNTE
*Want "DIDBR rounding test 9c TR"  BFE00000 00000000 40080000 00000000
r 6860.10   #                        RZ / RZ
*Want "DIDBR rounding test 9d NT"  3FF80000 00000000 40000000 00000000
r 6870.10   #                        RZ / RZ
*Want "DIDBR rounding test 9d TR"  3FF80000 00000000 40000000 00000000
r 6880.10   #                        RZ / RP
*Want "DIDBR rounding test 9e NT"  BFE00000 00000000 40080000 00000000
r 6890.10   #                        RZ / RP
*Want "DIDBR rounding test 9e TR"  BFE00000 00000000 40080000 00000000
r 68A0.10   #                        RZ / RM
*Want "DIDBR rounding test 9f NT"  3FF80000 00000000 40000000 00000000
r 68B0.10   #                        RZ / RM
*Want "DIDBR rounding test 9f TR"  3FF80000 00000000 40000000 00000000
r 68C0.10   #                        RP / RNTA
*Want "DIDBR rounding test 9g NT"  BFE00000 00000000 40080000 00000000
r 68D0.10   #                        RP / RNTA
*Want "DIDBR rounding test 9g TR"  BFE00000 00000000 40080000 00000000
r 68E0.10   #                        RP / RFS
*Want "DIDBR rounding test 9h NT"  BFE00000 00000000 40080000 00000000
r 68F0.10   #                        RP / RFS
*Want "DIDBR rounding test 9h TR"  BFE00000 00000000 40080000 00000000
r 6900.10   #                        RP / RNTE
*Want "DIDBR rounding test 9i NT"  BFE00000 00000000 40080000 00000000
r 6910.10   #                        RP / RNTE
*Want "DIDBR rounding test 9i TR"  BFE00000 00000000 40080000 00000000
r 6920.10   #                        RP / RZ
*Want "DIDBR rounding test 9j NT"  3FF80000 00000000 40000000 00000000
r 6930.10   #                        RP / RZ
*Want "DIDBR rounding test 9j TR"  3FF80000 00000000 40000000 00000000
r 6940.10   #                        RP / RP
*Want "DIDBR rounding test 9k NT"  BFE00000 00000000 40080000 00000000
r 6950.10   #                        RP / RP
*Want "DIDBR rounding test 9k TR"  BFE00000 00000000 40080000 00000000
r 6960.10   #                        RP / RM
*Want "DIDBR rounding test 9l NT"  3FF80000 00000000 40000000 00000000
r 6970.10   #                        RP / RM
*Want "DIDBR rounding test 9l TR"  3FF80000 00000000 40000000 00000000
r 6980.10   #                        RM / RNTA
*Want "DIDBR rounding test 9m NT"  BFE00000 00000000 40080000 00000000
r 6990.10   #                        RM / RNTA
*Want "DIDBR rounding test 9m TR"  BFE00000 00000000 40080000 00000000
r 69A0.10   #                        RM / RFS
*Want "DIDBR rounding test 9n NT"  BFE00000 00000000 40080000 00000000
r 69B0.10   #                        RM / RFS
*Want "DIDBR rounding test 9n TR"  BFE00000 00000000 40080000 00000000
r 69C0.10   #                        RM / RNTE
*Want "DIDBR rounding test 9o NT"  BFE00000 00000000 40080000 00000000
r 69D0.10   #                        RM / RNTE
*Want "DIDBR rounding test 9o TR"  BFE00000 00000000 40080000 00000000
r 69E0.10   #                        RM / RZ
*Want "DIDBR rounding test 9p NT"  3FF80000 00000000 40000000 00000000
r 69F0.10   #                        RM / RZ
*Want "DIDBR rounding test 9p TR"  3FF80000 00000000 40000000 00000000
r 6A00.10   #                        RM / RP
*Want "DIDBR rounding test 9q NT"  BFE00000 00000000 40080000 00000000
r 6A10.10   #                        RM / RP
*Want "DIDBR rounding test 9q TR"  BFE00000 00000000 40080000 00000000
r 6A20.10   #                        RM / RM
*Want "DIDBR rounding test 9r NT"  3FF80000 00000000 40000000 00000000
r 6A30.10   #                        RM / RM
*Want "DIDBR rounding test 9r TR"  3FF80000 00000000 40000000 00000000
r 6A40.10   #                        RFS / RNTA
*Want "DIDBR rounding test 9s NT"  BFE00000 00000000 40080000 00000000
r 6A50.10   #                        RFS / RNTA
*Want "DIDBR rounding test 9s TR"  BFE00000 00000000 40080000 00000000
r 6A60.10   #                        RFS / RFS
*Want "DIDBR rounding test 9t NT"  BFE00000 00000000 40080000 00000000
r 6A70.10   #                        RFS / RFS
*Want "DIDBR rounding test 9t TR"  BFE00000 00000000 40080000 00000000
r 6A80.10   #                        RFS / RNTE
*Want "DIDBR rounding test 9u NT"  BFE00000 00000000 40080000 00000000
r 6A90.10   #                        RFS / RNTE
*Want "DIDBR rounding test 9u TR"  BFE00000 00000000 40080000 00000000
r 6AA0.10   #                        RFS / RZ
*Want "DIDBR rounding test 9v NT"  3FF80000 00000000 40000000 00000000
r 6AB0.10   #                        RFS / RZ
*Want "DIDBR rounding test 9v TR"  3FF80000 00000000 40000000 00000000
r 6AC0.10   #                        RFS / RP
*Want "DIDBR rounding test 9w NT"  BFE00000 00000000 40080000 00000000
r 6AD0.10   #                        RFS / RP
*Want "DIDBR rounding test 9w TR"  BFE00000 00000000 40080000 00000000
r 6AE0.10   #                        RFS / RM
*Want "DIDBR rounding test 9x NT"  3FF80000 00000000 40000000 00000000
r 6AF0.10   #                        RFS / RM
*Want "DIDBR rounding test 9x TR"  3FF80000 00000000 40000000 00000000
r 6B00.10   #                        RZ / RNTA
*Want "DIDBR rounding test 10a NT" BFE00000 00000000 40140000 00000000
r 6B10.10   #                        RZ / RNTA
*Want "DIDBR rounding test 10a TR" BFE00000 00000000 40140000 00000000
r 6B20.10   #                        RZ / RFS
*Want "DIDBR rounding test 10b NT" BFE00000 00000000 40140000 00000000
r 6B30.10   #                        RZ / RFS
*Want "DIDBR rounding test 10b TR" BFE00000 00000000 40140000 00000000
r 6B40.10   #                        RZ / RNTE
*Want "DIDBR rounding test 10c NT" BFE00000 00000000 40140000 00000000
r 6B50.10   #                        RZ / RNTE
*Want "DIDBR rounding test 10c TR" BFE00000 00000000 40140000 00000000
r 6B60.10   #                        RZ / RZ
*Want "DIDBR rounding test 10d NT" 3FF80000 00000000 40100000 00000000
r 6B70.10   #                        RZ / RZ
*Want "DIDBR rounding test 10d TR" 3FF80000 00000000 40100000 00000000
r 6B80.10   #                        RZ / RP
*Want "DIDBR rounding test 10e NT" BFE00000 00000000 40140000 00000000
r 6B90.10   #                        RZ / RP
*Want "DIDBR rounding test 10e TR" BFE00000 00000000 40140000 00000000
r 6BA0.10   #                        RZ / RM
*Want "DIDBR rounding test 10f NT" 3FF80000 00000000 40100000 00000000
r 6BB0.10   #                        RZ / RM
*Want "DIDBR rounding test 10f TR" 3FF80000 00000000 40100000 00000000
r 6BC0.10   #                        RP / RNTA
*Want "DIDBR rounding test 10g NT" BFE00000 00000000 40140000 00000000
r 6BD0.10   #                        RP / RNTA
*Want "DIDBR rounding test 10g TR" BFE00000 00000000 40140000 00000000
r 6BE0.10   #                        RP / RFS
*Want "DIDBR rounding test 10h NT" BFE00000 00000000 40140000 00000000
r 6BF0.10   #                        RP / RFS
*Want "DIDBR rounding test 10h TR" BFE00000 00000000 40140000 00000000
r 6C00.10   #                        RP / RNTE
*Want "DIDBR rounding test 10i NT" BFE00000 00000000 40140000 00000000
r 6C10.10   #                        RP / RNTE
*Want "DIDBR rounding test 10i TR" BFE00000 00000000 40140000 00000000
r 6C20.10   #                        RP / RZ
*Want "DIDBR rounding test 10j NT" 3FF80000 00000000 40100000 00000000
r 6C30.10   #                        RP / RZ
*Want "DIDBR rounding test 10j TR" 3FF80000 00000000 40100000 00000000
r 6C40.10   #                        RP / RP
*Want "DIDBR rounding test 10k NT" BFE00000 00000000 40140000 00000000
r 6C50.10   #                        RP / RP
*Want "DIDBR rounding test 10k TR" BFE00000 00000000 40140000 00000000
r 6C60.10   #                        RP / RM
*Want "DIDBR rounding test 10l NT" 3FF80000 00000000 40100000 00000000
r 6C70.10   #                        RP / RM
*Want "DIDBR rounding test 10l TR" 3FF80000 00000000 40100000 00000000
r 6C80.10   #                        RM / RNTA
*Want "DIDBR rounding test 10m NT" BFE00000 00000000 40140000 00000000
r 6C90.10   #                        RM / RNTA
*Want "DIDBR rounding test 10m TR" BFE00000 00000000 40140000 00000000
r 6CA0.10   #                        RM / RFS
*Want "DIDBR rounding test 10n NT" BFE00000 00000000 40140000 00000000
r 6CB0.10   #                        RM / RFS
*Want "DIDBR rounding test 10n TR" BFE00000 00000000 40140000 00000000
r 6CC0.10   #                        RM / RNTE
*Want "DIDBR rounding test 10o NT" BFE00000 00000000 40140000 00000000
r 6CD0.10   #                        RM / RNTE
*Want "DIDBR rounding test 10o TR" BFE00000 00000000 40140000 00000000
r 6CE0.10   #                        RM / RZ
*Want "DIDBR rounding test 10p NT" 3FF80000 00000000 40100000 00000000
r 6CF0.10   #                        RM / RZ
*Want "DIDBR rounding test 10p TR" 3FF80000 00000000 40100000 00000000
r 6D00.10   #                        RM / RP
*Want "DIDBR rounding test 10q NT" BFE00000 00000000 40140000 00000000
r 6D10.10   #                        RM / RP
*Want "DIDBR rounding test 10q TR" BFE00000 00000000 40140000 00000000
r 6D20.10   #                        RM / RM
*Want "DIDBR rounding test 10r NT" 3FF80000 00000000 40100000 00000000
r 6D30.10   #                        RM / RM
*Want "DIDBR rounding test 10r TR" 3FF80000 00000000 40100000 00000000
r 6D40.10   #                        RFS / RNTA
*Want "DIDBR rounding test 10s NT" BFE00000 00000000 40140000 00000000
r 6D50.10   #                        RFS / RNTA
*Want "DIDBR rounding test 10s TR" BFE00000 00000000 40140000 00000000
r 6D60.10   #                        RFS / RFS
*Want "DIDBR rounding test 10t NT" BFE00000 00000000 40140000 00000000
r 6D70.10   #                        RFS / RFS
*Want "DIDBR rounding test 10t TR" BFE00000 00000000 40140000 00000000
r 6D80.10   #                        RFS / RNTE
*Want "DIDBR rounding test 10u NT" BFE00000 00000000 40140000 00000000
r 6D90.10   #                        RFS / RNTE
*Want "DIDBR rounding test 10u TR" BFE00000 00000000 40140000 00000000
r 6DA0.10   #                        RFS / RZ
*Want "DIDBR rounding test 10v NT" 3FF80000 00000000 40100000 00000000
r 6DB0.10   #                        RFS / RZ
*Want "DIDBR rounding test 10v TR" 3FF80000 00000000 40100000 00000000
r 6DC0.10   #                        RFS / RP
*Want "DIDBR rounding test 10w NT" BFE00000 00000000 40140000 00000000
r 6DD0.10   #                        RFS / RP
*Want "DIDBR rounding test 10w TR" BFE00000 00000000 40140000 00000000
r 6DE0.10   #                        RFS / RM
*Want "DIDBR rounding test 10x NT" 3FF80000 00000000 40100000 00000000
r 6DF0.10   #                        RFS / RM
*Want "DIDBR rounding test 10x TR" 3FF80000 00000000 40100000 00000000
r 6E00.10   #                        RZ / RNTA
*Want "DIDBR rounding test 11a NT" 00000000 00000000 3FF00000 00000000
r 6E10.10   #                        RZ / RNTA
*Want "DIDBR rounding test 11a TR" 00000000 00000000 3FF00000 00000000
r 6E20.10   #                        RZ / RFS
*Want "DIDBR rounding test 11b NT" 00000000 00000000 3FF00000 00000000
r 6E30.10   #                        RZ / RFS
*Want "DIDBR rounding test 11b TR" 00000000 00000000 3FF00000 00000000
r 6E40.10   #                        RZ / RNTE
*Want "DIDBR rounding test 11c NT" 00000000 00000000 3FF00000 00000000
r 6E50.10   #                        RZ / RNTE
*Want "DIDBR rounding test 11c TR" 00000000 00000000 3FF00000 00000000
r 6E60.10   #                        RZ / RZ
*Want "DIDBR rounding test 11d NT" 00000000 00000000 3FF00000 00000000
r 6E70.10   #                        RZ / RZ
*Want "DIDBR rounding test 11d TR" 00000000 00000000 3FF00000 00000000
r 6E80.10   #                        RZ / RP
*Want "DIDBR rounding test 11e NT" 00000000 00000000 3FF00000 00000000
r 6E90.10   #                        RZ / RP
*Want "DIDBR rounding test 11e TR" 00000000 00000000 3FF00000 00000000
r 6EA0.10   #                        RZ / RM
*Want "DIDBR rounding test 11f NT" 00000000 00000000 3FF00000 00000000
r 6EB0.10   #                        RZ / RM
*Want "DIDBR rounding test 11f TR" 00000000 00000000 3FF00000 00000000
r 6EC0.10   #                        RP / RNTA
*Want "DIDBR rounding test 11g NT" 00000000 00000000 3FF00000 00000000
r 6ED0.10   #                        RP / RNTA
*Want "DIDBR rounding test 11g TR" 00000000 00000000 3FF00000 00000000
r 6EE0.10   #                        RP / RFS
*Want "DIDBR rounding test 11h NT" 00000000 00000000 3FF00000 00000000
r 6EF0.10   #                        RP / RFS
*Want "DIDBR rounding test 11h TR" 00000000 00000000 3FF00000 00000000
r 6F00.10   #                        RP / RNTE
*Want "DIDBR rounding test 11i NT" 00000000 00000000 3FF00000 00000000
r 6F10.10   #                        RP / RNTE
*Want "DIDBR rounding test 11i TR" 00000000 00000000 3FF00000 00000000
r 6F20.10   #                        RP / RZ
*Want "DIDBR rounding test 11j NT" 00000000 00000000 3FF00000 00000000
r 6F30.10   #                        RP / RZ
*Want "DIDBR rounding test 11j TR" 00000000 00000000 3FF00000 00000000
r 6F40.10   #                        RP / RP
*Want "DIDBR rounding test 11k NT" 00000000 00000000 3FF00000 00000000
r 6F50.10   #                        RP / RP
*Want "DIDBR rounding test 11k TR" 00000000 00000000 3FF00000 00000000
r 6F60.10   #                        RP / RM
*Want "DIDBR rounding test 11l NT" 00000000 00000000 3FF00000 00000000
r 6F70.10   #                        RP / RM
*Want "DIDBR rounding test 11l TR" 00000000 00000000 3FF00000 00000000
r 6F80.10   #                        RM / RNTA
*Want "DIDBR rounding test 11m NT" 00000000 00000000 3FF00000 00000000
r 6F90.10   #                        RM / RNTA
*Want "DIDBR rounding test 11m TR" 00000000 00000000 3FF00000 00000000
r 6FA0.10   #                        RM / RFS
*Want "DIDBR rounding test 11n NT" 00000000 00000000 3FF00000 00000000
r 6FB0.10   #                        RM / RFS
*Want "DIDBR rounding test 11n TR" 00000000 00000000 3FF00000 00000000
r 6FC0.10   #                        RM / RNTE
*Want "DIDBR rounding test 11o NT" 00000000 00000000 3FF00000 00000000
r 6FD0.10   #                        RM / RNTE
*Want "DIDBR rounding test 11o TR" 00000000 00000000 3FF00000 00000000
r 6FE0.10   #                        RM / RZ
*Want "DIDBR rounding test 11p NT" 00000000 00000000 3FF00000 00000000
r 6FF0.10   #                        RM / RZ
*Want "DIDBR rounding test 11p TR" 00000000 00000000 3FF00000 00000000
r 7000.10   #                        RM / RP
*Want "DIDBR rounding test 11q NT" 00000000 00000000 3FF00000 00000000
r 7010.10   #                        RM / RP
*Want "DIDBR rounding test 11q TR" 00000000 00000000 3FF00000 00000000
r 7020.10   #                        RM / RM
*Want "DIDBR rounding test 11r NT" 00000000 00000000 3FF00000 00000000
r 7030.10   #                        RM / RM
*Want "DIDBR rounding test 11r TR" 00000000 00000000 3FF00000 00000000
r 7040.10   #                        RFS / RNTA
*Want "DIDBR rounding test 11s NT" 00000000 00000000 3FF00000 00000000
r 7050.10   #                        RFS / RNTA
*Want "DIDBR rounding test 11s TR" 00000000 00000000 3FF00000 00000000
r 7060.10   #                        RFS / RFS
*Want "DIDBR rounding test 11t NT" 00000000 00000000 3FF00000 00000000
r 7070.10   #                        RFS / RFS
*Want "DIDBR rounding test 11t TR" 00000000 00000000 3FF00000 00000000
r 7080.10   #                        RFS / RNTE
*Want "DIDBR rounding test 11u NT" 00000000 00000000 3FF00000 00000000
r 7090.10   #                        RFS / RNTE
*Want "DIDBR rounding test 11u TR" 00000000 00000000 3FF00000 00000000
r 70A0.10   #                        RFS / RZ
*Want "DIDBR rounding test 11v NT" 00000000 00000000 3FF00000 00000000
r 70B0.10   #                        RFS / RZ
*Want "DIDBR rounding test 11v TR" 00000000 00000000 3FF00000 00000000
r 70C0.10   #                        RFS / RP
*Want "DIDBR rounding test 11w NT" 00000000 00000000 3FF00000 00000000
r 70D0.10   #                        RFS / RP
*Want "DIDBR rounding test 11w TR" 00000000 00000000 3FF00000 00000000
r 70E0.10   #                        RFS / RM
*Want "DIDBR rounding test 11x NT" 00000000 00000000 3FF00000 00000000
r 70F0.10   #                        RFS / RM
*Want "DIDBR rounding test 11x TR" 00000000 00000000 3FF00000 00000000
r 7100.10   #                        RZ / RNTA
*Want "DIDBR rounding test 12a NT" C0000000 00000000 3FF00000 00000000
r 7110.10   #                        RZ / RNTA
*Want "DIDBR rounding test 12a TR" C0000000 00000000 3FF00000 00000000
r 7120.10   #                        RZ / RFS
*Want "DIDBR rounding test 12b NT" C0000000 00000000 3FF00000 00000000
r 7130.10   #                        RZ / RFS
*Want "DIDBR rounding test 12b TR" C0000000 00000000 3FF00000 00000000
r 7140.10   #                        RZ / RNTE
*Want "DIDBR rounding test 12c NT" C0000000 00000000 3FF00000 00000000
r 7150.10   #                        RZ / RNTE
*Want "DIDBR rounding test 12c TR" C0000000 00000000 3FF00000 00000000
r 7160.10   #                        RZ / RZ
*Want "DIDBR rounding test 12d NT" 40080000 00000000 00000000 00000000
r 7170.10   #                        RZ / RZ
*Want "DIDBR rounding test 12d TR" 40080000 00000000 00000000 00000000
r 7180.10   #                        RZ / RP
*Want "DIDBR rounding test 12e NT" C0000000 00000000 3FF00000 00000000
r 7190.10   #                        RZ / RP
*Want "DIDBR rounding test 12e TR" C0000000 00000000 3FF00000 00000000
r 71A0.10   #                        RZ / RM
*Want "DIDBR rounding test 12f NT" 40080000 00000000 00000000 00000000
r 71B0.10   #                        RZ / RM
*Want "DIDBR rounding test 12f TR" 40080000 00000000 00000000 00000000
r 71C0.10   #                        RP / RNTA
*Want "DIDBR rounding test 12g NT" C0000000 00000000 3FF00000 00000000
r 71D0.10   #                        RP / RNTA
*Want "DIDBR rounding test 12g TR" C0000000 00000000 3FF00000 00000000
r 71E0.10   #                        RP / RFS
*Want "DIDBR rounding test 12h NT" C0000000 00000000 3FF00000 00000000
r 71F0.10   #                        RP / RFS
*Want "DIDBR rounding test 12h TR" C0000000 00000000 3FF00000 00000000
r 7200.10   #                        RP / RNTE
*Want "DIDBR rounding test 12i NT" C0000000 00000000 3FF00000 00000000
r 7210.10   #                        RP / RNTE
*Want "DIDBR rounding test 12i TR" C0000000 00000000 3FF00000 00000000
r 7220.10   #                        RP / RZ
*Want "DIDBR rounding test 12j NT" 40080000 00000000 00000000 00000000
r 7230.10   #                        RP / RZ
*Want "DIDBR rounding test 12j TR" 40080000 00000000 00000000 00000000
r 7240.10   #                        RP / RP
*Want "DIDBR rounding test 12k NT" C0000000 00000000 3FF00000 00000000
r 7250.10   #                        RP / RP
*Want "DIDBR rounding test 12k TR" C0000000 00000000 3FF00000 00000000
r 7260.10   #                        RP / RM
*Want "DIDBR rounding test 12l NT" 40080000 00000000 00000000 00000000
r 7270.10   #                        RP / RM
*Want "DIDBR rounding test 12l TR" 40080000 00000000 00000000 00000000
r 7280.10   #                        RM / RNTA
*Want "DIDBR rounding test 12m NT" C0000000 00000000 3FF00000 00000000
r 7290.10   #                        RM / RNTA
*Want "DIDBR rounding test 12m TR" C0000000 00000000 3FF00000 00000000
r 72A0.10   #                        RM / RFS
*Want "DIDBR rounding test 12n NT" C0000000 00000000 3FF00000 00000000
r 72B0.10   #                        RM / RFS
*Want "DIDBR rounding test 12n TR" C0000000 00000000 3FF00000 00000000
r 72C0.10   #                        RM / RNTE
*Want "DIDBR rounding test 12o NT" C0000000 00000000 3FF00000 00000000
r 72D0.10   #                        RM / RNTE
*Want "DIDBR rounding test 12o TR" C0000000 00000000 3FF00000 00000000
r 72E0.10   #                        RM / RZ
*Want "DIDBR rounding test 12p NT" 40080000 00000000 00000000 00000000
r 72F0.10   #                        RM / RZ
*Want "DIDBR rounding test 12p TR" 40080000 00000000 00000000 00000000
r 7300.10   #                        RM / RP
*Want "DIDBR rounding test 12q NT" C0000000 00000000 3FF00000 00000000
r 7310.10   #                        RM / RP
*Want "DIDBR rounding test 12q TR" C0000000 00000000 3FF00000 00000000
r 7320.10   #                        RM / RM
*Want "DIDBR rounding test 12r NT" 40080000 00000000 00000000 00000000
r 7330.10   #                        RM / RM
*Want "DIDBR rounding test 12r TR" 40080000 00000000 00000000 00000000
r 7340.10   #                        RFS / RNTA
*Want "DIDBR rounding test 12s NT" C0000000 00000000 3FF00000 00000000
r 7350.10   #                        RFS / RNTA
*Want "DIDBR rounding test 12s TR" C0000000 00000000 3FF00000 00000000
r 7360.10   #                        RFS / RFS
*Want "DIDBR rounding test 12t NT" C0000000 00000000 3FF00000 00000000
r 7370.10   #                        RFS / RFS
*Want "DIDBR rounding test 12t TR" C0000000 00000000 3FF00000 00000000
r 7380.10   #                        RFS / RNTE
*Want "DIDBR rounding test 12u NT" C0000000 00000000 3FF00000 00000000
r 7390.10   #                        RFS / RNTE
*Want "DIDBR rounding test 12u TR" C0000000 00000000 3FF00000 00000000
r 73A0.10   #                        RFS / RZ
*Want "DIDBR rounding test 12v NT" 40080000 00000000 00000000 00000000
r 73B0.10   #                        RFS / RZ
*Want "DIDBR rounding test 12v TR" 40080000 00000000 00000000 00000000
r 73C0.10   #                        RFS / RP
*Want "DIDBR rounding test 12w NT" C0000000 00000000 3FF00000 00000000
r 73D0.10   #                        RFS / RP
*Want "DIDBR rounding test 12w TR" C0000000 00000000 3FF00000 00000000
r 73E0.10   #                        RFS / RM
*Want "DIDBR rounding test 12x NT" 40080000 00000000 00000000 00000000
r 73F0.10   #                        RFS / RM
*Want "DIDBR rounding test 12x TR" 40080000 00000000 00000000 00000000







*Compare
# Long BFP exhaustive rounding mode tests
r 9000.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIDBR Rounding FPCR 1ab"  00000000 F8000000 00000000 F8000000
r 9010.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIDBR Rounding FPCR 1cd"  00000000 F8000000 00000000 F8000000
r 9020.10  #                        RZ / RP           RZ / RM
*Want    "DIDBR Rounding FPCR 1ef"  00000000 F8000000 00000000 F8000000
r 9030.10  #                        RP / RNTA         RP / RFS
*Want    "DIDBR Rounding FPCR 1gh"  00000000 F8000000 00000000 F8000000
r 9040.10  #                        RP / RNTE         RP / RZ
*Want    "DIDBR Rounding FPCR 1ij"  00000000 F8000000 00000000 F8000000
r 9050.10  #                        RP / RP           RP / RM
*Want    "DIDBR Rounding FPCR 1kl"  00000000 F8000000 00000000 F8000000
r 9060.10  #                        RM / RNTA         RM / RFS
*Want    "DIDBR Rounding FPCR 1mn"  00000000 F8000000 00000000 F8000000
r 9070.10  #                        RM / RNTE         RM / RZ
*Want    "DIDBR Rounding FPCR 1op"  00000000 F8000000 00000000 F8000000
r 9080.10  #                        RM / RP           RM / RM
*Want    "DIDBR Rounding FPCR 1qr"  00000000 F8000000 00000000 F8000000
r 9090.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIDBR Rounding FPCR 1st"  00000000 F8000000 00000000 F8000000
r 90A0.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIDBR Rounding FPCR 1uv"  00000000 F8000000 00000000 F8000000
r 90B0.10  #                        RFS / RP          RFS / RM
*Want    "DIDBR Rounding FPCR 1wx"  00000000 F8000000 00000000 F8000000
r 90C0.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIDBR Rounding FPCR 2ab"  00000000 F8000000 00000000 F8000000
r 90D0.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIDBR Rounding FPCR 2cd"  00000000 F8000000 00000000 F8000000
r 90E0.10  #                        RZ / RP           RZ / RM
*Want    "DIDBR Rounding FPCR 2ef"  00000000 F8000000 00000000 F8000000
r 90F0.10  #                        RP / RNTA         RP / RFS
*Want    "DIDBR Rounding FPCR 2gh"  00000000 F8000000 00000000 F8000000
r 9100.10  #                        RP / RNTE         RP / RZ
*Want    "DIDBR Rounding FPCR 2ij"  00000000 F8000000 00000000 F8000000
r 9110.10  #                        RP / RP           RP / RM
*Want    "DIDBR Rounding FPCR 2kl"  00000000 F8000000 00000000 F8000000
r 9120.10  #                        RM / RNTA         RM / RFS
*Want    "DIDBR Rounding FPCR 2mn"  00000000 F8000000 00000000 F8000000
r 9130.10  #                        RM / RNTE         RM / RZ
*Want    "DIDBR Rounding FPCR 2op"  00000000 F8000000 00000000 F8000000
r 9140.10  #                        RM / RP           RM / RM
*Want    "DIDBR Rounding FPCR 2qr"  00000000 F8000000 00000000 F8000000
r 9150.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIDBR Rounding FPCR 2st"  00000000 F8000000 00000000 F8000000
r 9160.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIDBR Rounding FPCR 2uv"  00000000 F8000000 00000000 F8000000
r 9170.10  #                        RFS / RP          RFS / RM
*Want    "DIDBR Rounding FPCR 2wx"  00000000 F8000000 00000000 F8000000
r 9180.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIDBR Rounding FPCR 3ab"  00000000 F8000000 00000000 F8000000
r 9190.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIDBR Rounding FPCR 3cd"  00000000 F8000000 00000000 F8000000
r 91A0.10  #                        RZ / RP           RZ / RM
*Want    "DIDBR Rounding FPCR 3ef"  00000000 F8000000 00000000 F8000000
r 91B0.10  #                        RP / RNTA         RP / RFS
*Want    "DIDBR Rounding FPCR 3gh"  00000000 F8000000 00000000 F8000000
r 91C0.10  #                        RP / RNTE         RP / RZ
*Want    "DIDBR Rounding FPCR 3ij"  00000000 F8000000 00000000 F8000000
r 91D0.10  #                        RP / RP           RP / RM
*Want    "DIDBR Rounding FPCR 3kl"  00000000 F8000000 00000000 F8000000
r 91E0.10  #                        RM / RNTA         RM / RFS
*Want    "DIDBR Rounding FPCR 3mn"  00000000 F8000000 00000000 F8000000
r 91F0.10  #                        RM / RNTE         RM / RZ
*Want    "DIDBR Rounding FPCR 3op"  00000000 F8000000 00000000 F8000000
r 9200.10  #                        RM / RP           RM / RM
*Want    "DIDBR Rounding FPCR 3qr"  00000000 F8000000 00000000 F8000000
r 9210.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIDBR Rounding FPCR 3st"  00000000 F8000000 00000000 F8000000
r 9220.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIDBR Rounding FPCR 3uv"  00000000 F8000000 00000000 F8000000
r 9230.10  #                        RFS / RP          RFS / RM
*Want    "DIDBR Rounding FPCR 3wx"  00000000 F8000000 00000000 F8000000
r 9240.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIDBR Rounding FPCR 4ab"  00000000 F8000000 00000000 F8000000
r 9250.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIDBR Rounding FPCR 4cd"  00000000 F8000000 00000000 F8000000
r 9260.10  #                        RZ / RP           RZ / RM
*Want    "DIDBR Rounding FPCR 4ef"  00000000 F8000000 00000000 F8000000
r 9270.10  #                        RP / RNTA         RP / RFS
*Want    "DIDBR Rounding FPCR 4gh"  00000000 F8000000 00000000 F8000000
r 9280.10  #                        RP / RNTE         RP / RZ
*Want    "DIDBR Rounding FPCR 4ij"  00000000 F8000000 00000000 F8000000
r 9290.10  #                        RP / RP           RP / RM
*Want    "DIDBR Rounding FPCR 4kl"  00000000 F8000000 00000000 F8000000
r 92A0.10  #                        RM / RNTA         RM / RFS
*Want    "DIDBR Rounding FPCR 4mn"  00000000 F8000000 00000000 F8000000
r 92B0.10  #                        RM / RNTE         RM / RZ
*Want    "DIDBR Rounding FPCR 4op"  00000000 F8000000 00000000 F8000000
r 92C0.10  #                        RM / RP           RM / RM
*Want    "DIDBR Rounding FPCR 4qr"  00000000 F8000000 00000000 F8000000
r 92D0.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIDBR Rounding FPCR 4st"  00000000 F8000000 00000000 F8000000
r 92E0.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIDBR Rounding FPCR 4uv"  00000000 F8000000 00000000 F8000000
r 92F0.10  #                        RFS / RP          RFS / RM
*Want    "DIDBR Rounding FPCR 4wx"  00000000 F8000000 00000000 F8000000
r 9300.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIDBR Rounding FPCR 5ab"  00000000 F8000000 00000000 F8000000
r 9310.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIDBR Rounding FPCR 5cd"  00000000 F8000000 00000000 F8000000
r 9320.10  #                        RZ / RP           RZ / RM
*Want    "DIDBR Rounding FPCR 5ef"  00000000 F8000000 00000000 F8000000
r 9330.10  #                        RP / RNTA         RP / RFS
*Want    "DIDBR Rounding FPCR 5gh"  00000000 F8000000 00000000 F8000000
r 9340.10  #                        RP / RNTE         RP / RZ
*Want    "DIDBR Rounding FPCR 5ij"  00000000 F8000000 00000000 F8000000
r 9350.10  #                        RP / RP           RP / RM
*Want    "DIDBR Rounding FPCR 5kl"  00000000 F8000000 00000000 F8000000
r 9360.10  #                        RM / RNTA         RM / RFS
*Want    "DIDBR Rounding FPCR 5mn"  00000000 F8000000 00000000 F8000000
r 9370.10  #                        RM / RNTE         RM / RZ
*Want    "DIDBR Rounding FPCR 5op"  00000000 F8000000 00000000 F8000000
r 9380.10  #                        RM / RP           RM / RM
*Want    "DIDBR Rounding FPCR 5qr"  00000000 F8000000 00000000 F8000000
r 9390.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIDBR Rounding FPCR 5st"  00000000 F8000000 00000000 F8000000
r 93A0.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIDBR Rounding FPCR 5uv"  00000000 F8000000 00000000 F8000000
r 93B0.10  #                        RFS / RP          RFS / RM
*Want    "DIDBR Rounding FPCR 5wx"  00000000 F8000000 00000000 F8000000
r 93C0.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIDBR Rounding FPCR 6ab"  00000000 F8000000 00000000 F8000000
r 93D0.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIDBR Rounding FPCR 6cd"  00000000 F8000000 00000000 F8000000
r 93E0.10  #                        RZ / RP           RZ / RM
*Want    "DIDBR Rounding FPCR 6ef"  00000000 F8000000 00000000 F8000000
r 93F0.10  #                        RP / RNTA         RP / RFS
*Want    "DIDBR Rounding FPCR 6gh"  00000000 F8000000 00000000 F8000000
r 9400.10  #                        RP / RNTE         RP / RZ
*Want    "DIDBR Rounding FPCR 6ij"  00000000 F8000000 00000000 F8000000
r 9410.10  #                        RP / RP           RP / RM
*Want    "DIDBR Rounding FPCR 6kl"  00000000 F8000000 00000000 F8000000
r 9420.10  #                        RM / RNTA         RM / RFS
*Want    "DIDBR Rounding FPCR 6mn"  00000000 F8000000 00000000 F8000000
r 9430.10  #                        RM / RNTE         RM / RZ
*Want    "DIDBR Rounding FPCR 6op"  00000000 F8000000 00000000 F8000000
r 9440.10  #                        RM / RP           RM / RM
*Want    "DIDBR Rounding FPCR 6qr"  00000000 F8000000 00000000 F8000000
r 9450.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIDBR Rounding FPCR 6st"  00000000 F8000000 00000000 F8000000
r 9460.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIDBR Rounding FPCR 6uv"  00000000 F8000000 00000000 F8000000
r 9470.10  #                        RFS / RP          RFS / RM
*Want    "DIDBR Rounding FPCR 6wx"  00000000 F8000000 00000000 F8000000
r 9480.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIDBR Rounding FPCR 7ab"  00000000 F8000000 00000000 F8000000
r 9490.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIDBR Rounding FPCR 7cd"  00000000 F8000000 00000000 F8000000
r 94A0.10  #                        RZ / RP           RZ / RM
*Want    "DIDBR Rounding FPCR 7ef"  00000000 F8000000 00000000 F8000000
r 94B0.10  #                        RP / RNTA         RP / RFS
*Want    "DIDBR Rounding FPCR 7gh"  00000000 F8000000 00000000 F8000000
r 94C0.10  #                        RP / RNTE         RP / RZ
*Want    "DIDBR Rounding FPCR 7ij"  00000000 F8000000 00000000 F8000000
r 94D0.10  #                        RP / RP           RP / RM
*Want    "DIDBR Rounding FPCR 7kl"  00000000 F8000000 00000000 F8000000
r 94E0.10  #                        RM / RNTA         RM / RFS
*Want    "DIDBR Rounding FPCR 7mn"  00000000 F8000000 00000000 F8000000
r 94F0.10  #                        RM / RNTE         RM / RZ
*Want    "DIDBR Rounding FPCR 7op"  00000000 F8000000 00000000 F8000000
r 9500.10  #                        RM / RP           RM / RM
*Want    "DIDBR Rounding FPCR 7qr"  00000000 F8000000 00000000 F8000000
r 9510.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIDBR Rounding FPCR 7st"  00000000 F8000000 00000000 F8000000
r 9520.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIDBR Rounding FPCR 7uv"  00000000 F8000000 00000000 F8000000
r 9530.10  #                        RFS / RP          RFS / RM
*Want    "DIDBR Rounding FPCR 7wx"  00000000 F8000000 00000000 F8000000
r 9540.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIDBR Rounding FPCR 8ab"  00000000 F8000000 00000000 F8000000
r 9550.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIDBR Rounding FPCR 8cd"  00000000 F8000000 00000000 F8000000
r 9560.10  #                        RZ / RP           RZ / RM
*Want    "DIDBR Rounding FPCR 8ef"  00000000 F8000000 00000000 F8000000
r 9570.10  #                        RP / RNTA         RP / RFS
*Want    "DIDBR Rounding FPCR 8gh"  00000000 F8000000 00000000 F8000000
r 9580.10  #                        RP / RNTE         RP / RZ
*Want    "DIDBR Rounding FPCR 8ij"  00000000 F8000000 00000000 F8000000
r 9590.10  #                        RP / RP           RP / RM
*Want    "DIDBR Rounding FPCR 8kl"  00000000 F8000000 00000000 F8000000
r 95A0.10  #                        RM / RNTA         RM / RFS
*Want    "DIDBR Rounding FPCR 8mn"  00000000 F8000000 00000000 F8000000
r 95B0.10  #                        RM / RNTE         RM / RZ
*Want    "DIDBR Rounding FPCR 8op"  00000000 F8000000 00000000 F8000000
r 95C0.10  #                        RM / RP           RM / RM
*Want    "DIDBR Rounding FPCR 8qr"  00000000 F8000000 00000000 F8000000
r 95D0.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIDBR Rounding FPCR 8st"  00000000 F8000000 00000000 F8000000
r 95E0.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIDBR Rounding FPCR 8uv"  00000000 F8000000 00000000 F8000000
r 95F0.10  #                        RFS / RP          RFS / RM
*Want    "DIDBR Rounding FPCR 8wx"  00000000 F8000000 00000000 F8000000
r 9600.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIDBR Rounding FPCR 9ab"  00000000 F8000000 00000000 F8000000
r 9610.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIDBR Rounding FPCR 9cd"  00000000 F8000000 00000000 F8000000
r 9620.10  #                        RZ / RP           RZ / RM
*Want    "DIDBR Rounding FPCR 9ef"  00000000 F8000000 00000000 F8000000
r 9630.10  #                        RP / RNTA         RP / RFS
*Want    "DIDBR Rounding FPCR 9gh"  00000000 F8000000 00000000 F8000000
r 9640.10  #                        RP / RNTE         RP / RZ
*Want    "DIDBR Rounding FPCR 9ij"  00000000 F8000000 00000000 F8000000
r 9650.10  #                        RP / RP           RP / RM
*Want    "DIDBR Rounding FPCR 9kl"  00000000 F8000000 00000000 F8000000
r 9660.10  #                        RM / RNTA         RM / RFS
*Want    "DIDBR Rounding FPCR 9mn"  00000000 F8000000 00000000 F8000000
r 9670.10  #                        RM / RNTE         RM / RZ
*Want    "DIDBR Rounding FPCR 9op"  00000000 F8000000 00000000 F8000000
r 9680.10  #                        RM / RP           RM / RM
*Want    "DIDBR Rounding FPCR 9qr"  00000000 F8000000 00000000 F8000000
r 9690.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIDBR Rounding FPCR 9st"  00000000 F8000000 00000000 F8000000
r 96A0.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIDBR Rounding FPCR 9uv"  00000000 F8000000 00000000 F8000000
r 96B0.10  #                        RFS / RP          RFS / RM
*Want    "DIDBR Rounding FPCR 9wx"  00000000 F8000000 00000000 F8000000
r 96C0.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIDBR Rounding FPCR 10ab" 00000000 F8000000 00000000 F8000000
r 96D0.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIDBR Rounding FPCR 10cd" 00000000 F8000000 00000000 F8000000
r 96E0.10  #                        RZ / RP           RZ / RM
*Want    "DIDBR Rounding FPCR 10ef" 00000000 F8000000 00000000 F8000000
r 96F0.10  #                        RP / RNTA         RP / RFS
*Want    "DIDBR Rounding FPCR 10gh" 00000000 F8000000 00000000 F8000000
r 9700.10  #                        RP / RNTE         RP / RZ
*Want    "DIDBR Rounding FPCR 10ij" 00000000 F8000000 00000000 F8000000
r 9710.10  #                        RP / RP           RP / RM
*Want    "DIDBR Rounding FPCR 10kl" 00000000 F8000000 00000000 F8000000
r 9720.10  #                        RM / RNTA         RM / RFS
*Want    "DIDBR Rounding FPCR 10mn" 00000000 F8000000 00000000 F8000000
r 9730.10  #                        RM / RNTE         RM / RZ
*Want    "DIDBR Rounding FPCR 10op" 00000000 F8000000 00000000 F8000000
r 9740.10  #                        RM / RP           RM / RM
*Want    "DIDBR Rounding FPCR 10qr" 00000000 F8000000 00000000 F8000000
r 9750.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIDBR Rounding FPCR 10st" 00000000 F8000000 00000000 F8000000
r 9760.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIDBR Rounding FPCR 10uv" 00000000 F8000000 00000000 F8000000
r 9770.10  #                        RFS / RP          RFS / RM
*Want    "DIDBR Rounding FPCR 10wx" 00000000 F8000000 00000000 F8000000
r 9780.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIDBR Rounding FPCR 11ab" 00000000 F8000000 00000000 F8000000
r 9790.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIDBR Rounding FPCR 11cd" 00000000 F8000000 00000000 F8000000
r 97A0.10  #                        RZ / RP           RZ / RM
*Want    "DIDBR Rounding FPCR 11ef" 00000000 F8000000 00000000 F8000000
r 97B0.10  #                        RP / RNTA         RP / RFS
*Want    "DIDBR Rounding FPCR 11gh" 00000000 F8000000 00000000 F8000000
r 97C0.10  #                        RP / RNTE         RP / RZ
*Want    "DIDBR Rounding FPCR 11ij" 00000000 F8000000 00000000 F8000000
r 97D0.10  #                        RP / RP           RP / RM
*Want    "DIDBR Rounding FPCR 11kl" 00000000 F8000000 00000000 F8000000
r 97E0.10  #                        RM / RNTA         RM / RFS
*Want    "DIDBR Rounding FPCR 11mn" 00000000 F8000000 00000000 F8000000
r 97F0.10  #                        RM / RNTE         RM / RZ
*Want    "DIDBR Rounding FPCR 11op" 00000000 F8000000 00000000 F8000000
r 9800.10  #                        RM / RP           RM / RM
*Want    "DIDBR Rounding FPCR 11qr" 00000000 F8000000 00000000 F8000000
r 9810.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIDBR Rounding FPCR 11st" 00000000 F8000000 00000000 F8000000
r 9810.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIDBR Rounding FPCR 11uv" 00000000 F8000000 00000000 F8000000
r 9820.10  #                        RFS / RP          RFS / RM
*Want    "DIDBR Rounding FPCR 11wx" 00000000 F8000000 00000000 F8000000
r 9830.10  #                        RZ / RNTA         RZ / RFS
*Want    "DIDBR Rounding FPCR 12ab" 00000000 F8000000 00000000 F8000000
r 9840.10  #                        RZ / RNTE         RZ / RZ
*Want    "DIDBR Rounding FPCR 12cd" 00000000 F8000000 00000000 F8000000
r 9850.10  #                        RZ / RP           RZ / RM
*Want    "DIDBR Rounding FPCR 12ef" 00000000 F8000000 00000000 F8000000
r 9860.10  #                        RP / RNTA         RP / RFS
*Want    "DIDBR Rounding FPCR 12gh" 00000000 F8000000 00000000 F8000000
r 9870.10  #                        RP / RNTE         RP / RZ
*Want    "DIDBR Rounding FPCR 12ij" 00000000 F8000000 00000000 F8000000
r 9880.10  #                        RP / RP           RP / RM
*Want    "DIDBR Rounding FPCR 12kl" 00000000 F8000000 00000000 F8000000
r 9890.10  #                        RM / RNTA         RM / RFS
*Want    "DIDBR Rounding FPCR 12mn" 00000000 F8000000 00000000 F8000000
r 98A0.10  #                        RM / RNTE         RM / RZ
*Want    "DIDBR Rounding FPCR 12op" 00000000 F8000000 00000000 F8000000
r 98B0.10  #                        RM / RP           RM / RM
*Want    "DIDBR Rounding FPCR 12qr" 00000000 F8000000 00000000 F8000000
r 98C0.10  #                        RFS / RNTA        RFS / RFS
*Want    "DIDBR Rounding FPCR 12st" 00000000 F8000000 00000000 F8000000
r 98D0.10  #                        RFS / RNTE        RFS / RZ
*Want    "DIDBR Rounding FPCR 12uv" 00000000 F8000000 00000000 F8000000
r 98E0.10  #                        RFS / RP          RFS / RM
*Want    "DIDBR Rounding FPCR 12wx" 00000000 F8000000 00000000 F8000000


*Done

