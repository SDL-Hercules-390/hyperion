*Testcase str-001-srst.tst: 

# Created and placed into the public domain 2018-12-27 by Bob Polmanter.

#
# NOTE - the nature of the string instructions is such that this test
#        case will only validate properly for the string instruction
#        improvement modifications committed in December 2018.  The
#        computation of the CPU determined number of bytes is an
#        unpredictable number on real hardware (at least above the
#        minimum value) and the method used in Hercules prior to
#        instruction improvements calculated it differently than the
#        improved method.  As a result, the operand registers will
#        likely contain different values when compared by the test
#        script due to the different CPU number of bytes 
#        determined.  None of the methods are wrong, and failing
#        results in the test script are not necessarily wrong.  
#        But this program and the resulting test script comparisons
#        were written for the method used by the improved string
#        instructions (CLST, MVST, SRST).
#

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

loadcore "$(testpath)/str-001-srst.core"

# Run the program
runtest

# Restore program check messages

ostailor null

###########################################################################################

# Analyze results

*Compare
#                            0 1 2 3  4 5 6 7  8 9 A B  C D E F
r 008E.02
*Want "PIC 06"              0006
r 0800.0C
*Want "Test 2 R6-R8"        00002500 00002532 10000001
r 0810.0C
*Want "Test 3 R6-R8"        00002500 00002580 20000001
r 0820.0C
*Want "Test 4 R6-R8"        00002500 00002500 20000001
r 0830.0C
*Want "Test 5 R6-R8"        00002500 00002500 20000001
r 0840.0C
*Want "Test 6 R6-R8"        00003080 00003100 10000002
r 0850.0C
*Want "Test 7 R6-R8"        00003080 00003300 20000002
r 0860.0C
*Want "Test 8 R6-R8"        0000B000 0000BF80 1000000A
r 0870.0C
*Want "Test 9 R6-R8"        0000B000 0000BFFF 2000000A

# Restore program check messages

ostailor null

*Done

