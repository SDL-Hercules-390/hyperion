*Testcase cff-01-CompareAndLoad
*
*        Concurrent-functions facility tests for SSF encoded instructions:
*
*        C8x6 CAL   - Compare And Load
*        C8x7 CALG  - Compare And Load Long
*        C8xF CALGF - Compare And Load Long Fullword
*
*   # -------------------------------------------------------
*   #  This tests only the basic function of the instruction.
*   #  Exceptions are NOT tested.
*   # -------------------------------------------------------
*
mainsize    2
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/cff-01-CompareAndLoad.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
