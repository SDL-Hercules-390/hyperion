*Testcase fix-page.tst:

# Created and placed into the public domain 05 OCT 2020 by Bob Polmanter.


###########################################################################################
#
# This test is used to verify that the Fix Page Assist (opcode X'E502')
# successfully loads the address in the MPLPFAL word (part of a control block 
# normally maintained by some S/370 versions of the MVS operating system) into
# the PSW as the next instruction address.  Not loading a new address into
# the PSW and instead continuing on to the next sequential instruction would
# be considered a failed test.  See code in "fix-page.asm" in the tests directory.
#
# Locations 300 and 304 are initialized with F'1' and F'1' respectively at
# the start of the test. The test always ends with a disabled wait PSW and 
# should not encounter a program check.
#
# - If the test is successful:
#	1. R14 -> instruction immediately following the E502 instruction.
#	2. R15 = the contents of fullword MPLPFAL.
#       3. The PSW IA was successfully loaded with the contents of MPLPFAL
#          and that execution successfully resumed there after execution of E502.
#          If execution did resume as stated, then regs 14-15 will be stored at
#          location 300 and 304 in order to be verified below.

# - If the test fails, execution resumes with the instruction following E502
#   and the values F'1' and F'1' remain in location 300 and 304 without alteration.
#
# Per the MVS Assist documentation in GA22-7079-1, in NO CASE should the Fix Page E502
# instruction proceed to the next sequential instruction. This is a test failure.
# It must always load an address into the PSW to be the next instruction address;
# this is a test success.

# set up, load, and run the test case
# Run test in 370 mode
archlvl S/370

ostailor quiet

# clear any prior test data/program/results
sysclear

loadcore "$(testpath)/fix-page.core"

# Run the program
runtest

# Restore program check messages

ostailor default

###########################################################################################

# Analyze results

*Compare 
#                            0 1 2 3  4 5 6 7
r 300.8 
*Want "R14, R15 values"     0000021A 0000021E
*Done 


