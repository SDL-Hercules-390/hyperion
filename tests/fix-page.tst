*Testcase fix-page.tst:

# Created and placed into the public domain 09 OCT 2020 by Bob Polmanter.


###########################################################################################
#
# These tests are used to verify that the Fix Page Assist (opcode X'E502')
# successfully loads the address in the MPLPFAL word (part of a control block 
# normally maintained by some S/370 versions of the MVS operating system) into
# the PSW as the next instruction address.  Not loading a new address into
# the PSW and instead continuing on to the next sequential instruction would
# be considered a failed test.  These tests also verify the execution of
# Fix Page E502 in the problem state when under the control of Control
# register 6 per the requirements of the Virtual Machine Extended Facility.
# Refer to the code in "fix-page.asm" in the tests directory for specific
# technical detail.
#
# Locations 600, 604, 608, and 60C are initialized with F'1' respectively at
# the start of the test. The tests end with a disabled wait PSW of 0.
#
# For Test #1 (Execute instruction E502 in Supervisor State).
# - If the test is successful:
#	1. R14 -> instruction immediately following the E502 instruction.
#	2. R15 = the contents of fullword MPLPFAL.
#       3. The PSW IA was successfully loaded with the contents of MPLPFAL
#          and that execution successfully resumed there after execution of E502.
#          If execution did resume as stated, then a zero result code will be 
#          stored at location 600.
#
# Per the MVS Assist documentation in GA22-7079-1, in NO CASE should the Fix Page E502
# instruction proceed to the next sequential instruction upon successful execution. 
# This is a test failure. It must always load an address into the PSW to be the next 
# instruction address; this is a test success.
#
# - If the test fails, execution resumes with the instruction following E502
#   and the values F'8' is stored location 600 to indicate failure.
#
#
# For Test #2 (Execute instruction E502 in Problem State, with CR6 set to
#              indicate that a virtual machine is in the virtual supervisor state).
# - If the test is successful:
#	1. R14 -> instruction immediately following the E502 instruction.
#	2. R15 = the contents of fullword MPLPFAL.
#       3. The PSW IA was successfully loaded with the contents of MPLPFAL
#          and that execution successfully resumed there after execution of E502.
#          If execution did resume as stated, then a zero result code will be 
#          stored at location 604.
#
# - If the test fails, execution resumes with the instruction following E502
#   and the value F'8' is stored in location 604 to indicate failure.
#
# For Test #3 (Execute instruction E502 in Problem State, with CR6 set to
#              indicate a virtual machine is also in the problem state).
# - If the test is successful:
#   A program check 02 occurs and then control is returned to the instruction 
#   following E502 and the value F'0' is stored location in 608 to indicate success.
#
# - If the test fails, execution resumes but the value F'8' is stored at
#   location 608 to indicate failure.
#
# For Test #4 (Execute instruction E502 in Problem State, with CR6 set to
#              indicate that the VM Extended Facility is disabled).
# - If the test is successful:
#   A program check 02 occurs and then control is returned to the instruction 
#   following E502 and the value F'0' is stored location in 60C to indicate success.
#
# - If the test fails, execution resumes but the value F'8' is stored at
#   location 60C to indicate failure.
#
#

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
#                                  0 1 2 3
r 600.4 
*Want "Test 1 result of zeros"    00000000
r 604.4 
*Want "Test 2 result of zeros"    00000000
r 608.4 
*Want "Test 3 result of zeros"    00000000
r 60C.4 
*Want "Test 4 result of zeros"    00000000
*Done 


