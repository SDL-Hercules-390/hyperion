# This test file was generated from offline assembler source
# by bldhtc.rexx 12 Nov 2015 12:36:58
# Treat as object code.  That is, modifications will be lost.
# assemble and listing files are provided for information only.
*Testcase logicImmediate
# processed 12 Nov 2015 12:36:58 by bldhtc.rexx
sysclear
archmode z
r    1A0=00000001800000000000000000000200
r    1D0=0002000180000000FFFFFFFFDEADDEAD
r    200=1B00B2B00880D20708880880D4070880
r    210=08904100000841100001412008001B44
r    220=441002BEB2220040BE482018441002C2
r    230=B2220040BE482020
r    238=441002C8B2220040BE482028441002CC
r    248=B2220040BE482030441002D2B2220040
r    258=BE482038441002D6B2220040BE482040
r    268=4120200189100001
r    270=A706FFD81B441B5594000848B2220050
r    280=96000849B222005097FF084AB2220050
r    290=8D40000818341B44EB00084C0054B222
r    2A0=0050EB00084D0056
r    2A8=B2220050EBFF084E0057B22200508D40
r    2B8=0008B2B202E094002000EB0028000154
r    2C8=96002008EB002808015697002010EB00
r    2D8=28100157
r    2E0=00020001800000000000000000000000
r    800=FFFFFFFFFFFFFFFF0000000000000000
r    810=FFFFFFFFFFFFFFFF0000000000000000
r    820=00000000000000000000000000000000
r    830=0000000000000000
r    838=00000000000000000000000000000000
r    848=FF00FF00FF00FF00
r    890=0000000000000800
r   2000=FFFFFFFFFFFFFFFF0000000000000000
r   2010=FFFFFFFFFFFFFFFF
numcpu 1
runtest .1
*Compare
r 00000800.8
*Want               01020408 10204080
*Compare
r 00000808.8
*Want               01020408 10204080
*Compare
r 00000810.8
*Want               FEFDFBF7 EFDFBF7F
*Compare
r 00002000.8
*Want               01020408 10204080
*Compare
r 00002008.8
*Want               01020408 10204080
*Compare
r 00002010.8
*Want               FEFDFBF7 EFDFBF7F
*Compare
r 00000818.8
*Want               10101010 10101010
*Compare
r 00000828.8
*Want               10101010 10101010
*Compare
r 00000838.8
*Want               10101010 10101010
*Compare
r 00000820.8
*Want               10101010 10101010
*Compare
r 00000830.8
*Want               10101010 10101010
*Compare
r 00000840.8
*Want               10101010 10101010
*Explain The failure of this test indicates either that
*Explain your C compiler lacks stdatomic.h and the GCC
*Explain intrinsic atomic operations, or that the atomic
*Explain operations are not lock free, or that Interlocked
*Explain Access Facility 2 has been unconfigured when
*Explain Hercules was built, or finally that Hercules
*Explain simply lacks the support for IAF2.
*Compare
r 00000880.8
*Want "Facilities list bit 52" 00000000 00000800
* First doubleword of facilities list
r 00000888.8
*Compare
r 00000848.8
*Want               00000000 00000000
gpr
*Gpr 3 00000000
*Gpr 4 00000000
*Done
