*Testcase zvector-e6-22-VTZ
*
*   Zvector E6 tests for VRI-L encoded instruction:
*
*   E67F VTZ    - Vector Test Zoned
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

loadcore    "$(testpath)/zvector-e6-22-VTZ.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
