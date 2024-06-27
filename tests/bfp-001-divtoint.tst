*Testcase bfp-001-divtoint: Test IEEE Divide To Integer

# Runtest *Compare dependency removed on 2022-08-16 by Fish.

# Suppress logging of program checks. Processing of this test script
# intentionally generates program checks as it runs as part of the
# instuctions' test and validation.

diag8cmd enable
ostailor quiet
sysclear
archmode esame
loadcore "$(testpath)/bfp-001-divtoint.core"
runtest 1.0
ostailor default
diag8cmd disable
*Done
