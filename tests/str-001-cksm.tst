*Testcase str-001-cksm.tst: 

# Created and placed into the public domain 2018-12-30 by Bob Polmanter.


###########################################################################################

# set up, load, and run the test case

# Suppress logging of program checks. Processing of this  test script
# intentionally generates program checks (specification exception) as part
# of instuction test and validation.  Redtest.rexx treats these as
# errors if they show up in the Hercules log.  So suppress them.

ostailor quiet

# clear any prior test data/program/results
sysclear

# Run test in z/Arch mode
archmode esame

loadcore "$(testpath)/str-001-cksm.core"

# Run the program
runtest

# Restore program check messages

ostailor default

###########################################################################################

# Analyze results

*Compare
#                            0 1 2 3  4 5 6 7  8 9 A B  C D E F
r 0800.0C
*Want "Test 1 R1-R3"        99DE2265 00000710 00000000
r 0810.0C
*Want "Test 2 R1-R3"        99003366 0000070D 00000000
r 0820.0C
*Want "Test 3 R1-R3"        99DE2265 0000300B 00000000
r 0830.0C
*Want "Test 4 R1-R3"        99003366 00003008 00000000
r 0840.0C
*Want "Test 5 R1-R3"        99DE2265 00003000 00000000
r 0850.0C
*Want "Test 6 R1-R3"        99003366 00003000 00000000
r 0860.0C
*Want "Test 7 R1-R3"        99DE2265 00003002 00000000
r 0870.0C
*Want "Test 8 R1-R3"        99003366 00003002 00000000
r 0880.0C
*Want "Test 9 R1-R3"        E1E1E1E1 0000BFF8 00000000

*Done

