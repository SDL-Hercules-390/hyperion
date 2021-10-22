#----------------------------------------------------------------------

*Testcase STFL and STFLE

*
* Because this Testcase uses STFLE it will fail on ESA hardware.
*

#----------------------------------------------------------------------
# This file was put into the public domain 2016-08-20
# by John P. Hartmann.  You can use it for anything you like,
# as long as this notice remains.
#----------------------------------------------------------------------

sysclear
archmode    z/Arch
loadcore    $(testpath)/stfl.core

# (Specification Exception expected)
*Program    6

#----------------------------------------------------------------------
# Adjust these as needed as new facilities are added to Hercules
#----------------------------------------------------------------------
defsym    FW1   F3F6FFFB    # Facilities 000-031
defsym    FW2   FCFDDC24    # Facilities 032-063
defsym    FW3   203C4000    # Facilities 064-095
defsym    FW4   00000000    # Facilities 096-127
defsym    FW5   00004000    # Facilities 128-159
#----------------------------------------------------------------------

runtest 0.1

*Compare

r c8.4
*Want "Basic STFL." $(FW1)

r 200.10
*Want "First two doublewords of truncated facilities list" $(FW1) $(FW2) 00000000 00000000

r 210.10
*Want "First two doublewords of extended facilities list"  $(FW1) $(FW2) $(FW3) $(FW4)

r 220.10
*Want "Double words two and three" $(FW5) 00000000 00000000 00000000

r 230.10
*Want "Double words four and five" 00000000 00000000 00000000 00000000

r 240.10
*Want "Double words six and seven" 00000000 00000000 00000000 00000000

gpr
*Gpr 2 30000000
*Gpr 3 2
*Gpr 4 0
*Gpr 5 2

*Done

#----------------------------------------------------------------------
#                         (FOR REFERENCE)
#----------------------------------------------------------------------
#
# BC12 2828: z/VM 5.4 -- returns 2 doublewords
#
#   FB6BFFFB FCFFF840
#   000C0000 00000000
#
#----------------------------------------------------------------------
# zPDT emulating an EC12 -- returns 3 doublewords
#
#   FF20FFF3 FC7CE000
#   00000000 00000000
#   00000000 00000000
#
#----------------------------------------------------------------------
# 2827 (EC12) (z/OS under z/VM 6.3) -- returns 3 doublewords
#
#   FB6BFFFB FCFFF840
#   003C0000 00000000
#   00000000 00000000
#
#----------------------------------------------------------------------
# z13 -- returns 4 doublewords
#
#   FFBBFFFB FC7CE540
#   A05E8000 00000000
#   D8000000 00000000
#   00000000 00000000
#
#----------------------------------------------------------------------
