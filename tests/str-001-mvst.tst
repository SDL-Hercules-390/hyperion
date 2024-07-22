*Testcase str-001-mvst

# Test MVST Instruction

# Created and placed into public domain 2018-12-27 by Bob Polmanter.
# Runtest *Compare dependency removed on 2022-03-08 by Fish.

# Suppress logging of program checks. Processing of this test script
# intentionally generates a program check (specification exception)
# as part of the instuction's test and validation.

ostailor quiet
sysclear
archmode esame
loadcore "$(testpath)/str-001-mvst.core"
runtest
ostailor default
*Done
