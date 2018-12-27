*Testcase str-001-mvst.tst: 

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

loadcore "$(testpath)/str-001-mvst.core"

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
r 0810.10
*Want "DEST 2"              E2C8D6D9 E340E2E3 D9C9D5C7 5B000000
r 0820.0C
*Want "Test 2 R6-R8"        0000081C 00000700 00000001        
r 0830.04
*Want "DEST 3"              5B000000                        
r 0840.0C
*Want "Test 3 R6-R8"        00000830 00000710 00000001        
r 0850.10
*Want "Test 4 R6-R8, CC"    0001204F 000012F0 00000002 00000000
r 0860.10
*Want "Test 5 R6-R8, CC"    00013257 00004000 00000002 00000000
r 0870.10
*Want "Test 6 R6-R8, CC"    0001611F 00006000 00000003 00000000
r 0880.10
*Want "Test 7 R6-R8, CC"    000180CF 00008030 00000003 00000000
r 0890.10
*Want "Test 8 R6-R8, CC"    0001C6F3 0000C000 00000004 00000000

# Restore program check messages

ostailor null

*Done

