* Store facilities list (extended).

# This file was put into the public domain 2016-08-20
# by John P. Hartmann.  You can use it for anything you like,
# as long as this notice remains.

*Testcase stfl and stfle
* This test cause program checks on ESA hardware.
sysclear
archmode z
loadcore $(testpath)/stfl.core
*Program 6
runtest
gpr
*Compare
r c8.4
*Want "Basic STFL." F1F4FFFB
r 200.10
*Want "First two doublewords of truncated facilities list" F1F4FFFB F8F50800 00000000 00000000
r 210.10
*Want "First two doublewords of extended facilities list" F1F4FFFB F8F50800 201C0000 00000000
r 220.10
*Want "Double words two and three" 00000000 00000000 00000000 00000000
r 230.10
*Want "Double words four and five" 00000000 00000000 00000000 00000000
r 240.10
*Want "Double words six and seven" 00000000 00000000 00000000 00000000
*Gpr 2 30000000
*Gpr 3 1
*Gpr 4 0
*Gpr 5 1
*Done

# STFLE on a zPDT with the latest(?) code to emulate an EC12
# returns 3 doublewords
# FF20FFF3 FC7CE000 00000000 00000000 00000000 00000000

# STFLE on a 2827 (EC12) z/OS under VM 6.3 returns 3 doublewords
# FB6BFFFB FCFFF840 003C0000 00000000 00000000 00000000

# BC12 2828: z/VM 5.4
# FB6BFFFB FCFFF840 000C0000 00000000

# z13
# FFBBFFFB FC7CE540 A05E8000 00000000
# D8000000 00000000 00000000 00000000
