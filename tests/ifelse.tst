#-------------------------------------------------------------------------------
# Test proper redtest.rexx handling of *If/*Else/*Fi directives
#-------------------------------------------------------------------------------

#---------------------
# Testcase  PLAIN
#---------------------

*Testcase IF-ELSE-01
script "$(testpath)/dummy.subtst"
*Done

#-----------------------
# Testcase  WITHIN  if
#-----------------------

*If 1
    *Testcase IF-ELSE-02
    script "$(testpath)/dummy.subtst"
    *Done
*Else
    *Message SKIPPING: Testcase IF-ELSE-02
    *Message REASON:   If condition is false
*Fi

#-----------------------------
# Testcase  OUTSIDE  of if
#-----------------------------

*Testcase IF-ELSE-03
*If 1
    script "$(testpath)/dummy.subtst"
*Else
    *Message SKIPPING: Testcase IF-ELSE-03
    *Message REASON:   If condition is false
*Fi
*Done

#-----------------------------------------
# TWO if conditions, one after the other
# Testcase  WITHIN  second if
#-----------------------------------------

*If 1
    *If 1
        *Testcase IF-ELSE-04
        script "$(testpath)/dummy.subtst"
        *Done
    *Else
        *Message SKIPPING: Testcase IF-ELSE-04
        *Message REASON:   Nested if condition is false
    *Fi
*Else
    *Message SKIPPING: Testcase IF-ELSE-04
    *Message REASON:   First if condition is false
*Fi

#-----------------------------------------
# TWO if conditions, second within ELSE
# Testcase  WITHIN  second if
#-----------------------------------------

*If 0
    *Message SKIPPING: Testcase IF-ELSE-05
    *Message REASON:   First if condition is true
*Else
    *If 1
        *Testcase IF-ELSE-05
        script "$(testpath)/dummy.subtst"
        *Done
    *Else
        *Message SKIPPING: Testcase IF-ELSE-05
        *Message REASON:   Nested if condition is false
    *Fi
*Fi

#-----------------------------------------
# TWO if conditions, one after the other
# Testcase  OUTSIDE  of both ifs
#-----------------------------------------

*Testcase IF-ELSE-06
*If 1
    *If 1
        script "$(testpath)/dummy.subtst"
    *Else
        *Message SKIPPING: Testcase IF-ELSE-06
        *Message REASON:   Nested if condition is false
    *Fi
*Else
    *Message SKIPPING: Testcase IF-ELSE-06
    *Message REASON:   First if condition is false
*Fi
*Done

#-----------------------------------------
# TWO if conditions, second within ELSE
# Testcase  OUTSIDE  of both ifs
#-----------------------------------------

*Testcase IF-ELSE-07
*If 0
    *Message SKIPPING: Testcase IF-ELSE-07
    *Message REASON:   First if condition is true
*Else
    *If 1
        script "$(testpath)/dummy.subtst"
    *Else
        *Message SKIPPING: Testcase IF-ELSE-07
        *Message REASON:   Nested if condition is false
    *Fi
*Fi
*Done
