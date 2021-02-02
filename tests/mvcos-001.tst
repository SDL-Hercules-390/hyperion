*Testcase mvcos-001.tst: 

# Created and placed into the public domain 27 JAN 2021 by Bob Polmanter.

################################################################################
#
#     Execute the MVCOS instruction iteratively, each time trying a
#     different combination of machine state, address space control
#     mode, MVCOS operand 1 control modes, MVCOS operand 2 control
#     modes, and key enablement in both operand 1 and then operand 2.
#     These individual tests are nested in a series of loops, so that
#     each state or mode is tested with each other combination of
#     states or modes in an exhaustive way.  See mvcos-001.asm for
#     specifc details.
#
################################################################################


################################################################################
#                  set up, load, and run the test case
################################################################################

# Suppress logging of program checks. Processing of this  test script
# intentionally generates program checks (specification exception) as part
# of instuction test and validation.  Redtest.rexx treats these as
# errors if they show up in the Hercules log.  So suppress them.

ostailor quiet

# clear any prior test data/program/results
sysclear

# Run test in z/Arch mode
archmode esame

loadcore "$(testpath)/mvcos-001.core"

# Run the program
runtest

# Restore program check messages

ostailor default

################################################################################
#                             Analyze results
################################################################################

*Compare
#                            0 1 2 3  4 5 6 7  8 9 A B  C D E F
r 200.0C
*Want "# successful tests"  0000384C 0001152C 0000092C

*Done
