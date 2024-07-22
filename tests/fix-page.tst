*Testcase fix-page

# Test the Fix Page E502 Assist

# Created and placed into the public domain 09 OCT 2020 by Bob Polmanter.
# Runtest *Compare dependency removed on 2022-03-08 by Fish.

# Suppress logging of program checks. Processing of this test script
# intentionally generates program checks as it runs as part of the
# instuction's test and validation.

ostailor quiet
archlvl S/370
sysclear
loadcore "$(testpath)/fix-page.core"
runtest
ostailor default
*Done
