*Testcase bim-001-add-sub

# Test Basic Integer Math Add & Subtract

# Copyright 2018 Stephen R. Orso.
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file BOOST_LICENSE_1_0.txt or a copy at:
# http://www.boost.org/LICENSE_1_0.txt)

# Expanded by Peter J. Jansen, 2019
# Runtest *Compare dependency removed on 2022-08-16 by Fish.

# Suppress logging of program checks. Processing of this test
# intentionally generates program checks as it runs as part of
# the instuctions' test and validation.

diag8cmd enable
ostailor quiet
sysclear
archmode esame
loadcore "$(testpath)/bim-001-add-sub.core"
runtest 1.0
ostailor default
diag8cmd disable
*Done
