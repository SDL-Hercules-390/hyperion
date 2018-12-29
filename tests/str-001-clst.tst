*Testcase str-001-clst.tst: 

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

loadcore "$(testpath)/str-001-clst.core"

# Run the program
runtest

# Restore program check messages

ostailor default

###########################################################################################

# Analyze results

*Compare
#                            0 1 2 3  4 5 6 7  8 9 A B  C D E F
r 008E.02
*Want "PIC 06"              0006
r 0800.0C
*Want "Test 2 R6-R8"        00000700 00000710 00000001
r 0810.0C
*Want "Test 3 R6-R8"        00000750 00000710 10000001
r 0820.0C
*Want "Test 4 R6-R8"        00000700 00000750 20000001
r 0830.0C
*Want "Test 5 R6-R8"        00000727 00000737 10000001
r 0840.0C
*Want "Test 6 R6-R8"        00000737 00000727 20000001
r 0850.0C
*Want "Test 7 R6-R8"        0000070C 0000074C 10000001
r 0860.0C
*Want "Test 8 R6-R8"        0000074C 0000071C 20000001
r 0870.0C
*Want "Test 9 R6-R8"        00003000 00004300 00000002
r 0880.0C
*Want "Test 10 R6-R8"       00002A00 00005000 00000002
r 0890.0C
*Want "Test 11 R6-R8"       00003080 00005000 00000002
r 08A0.0C
*Want "Test 12 R6-R8"       00002F40 00004F80 00000001
r 08B0.0C
*Want "Test 13 R6-R8"       00006000 0000C000 00000005

*Done
