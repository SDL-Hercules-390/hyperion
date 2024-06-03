*Testcase zvector-e6-11-convertbinary: VECTOR E6 VRR-i instructions
*
*   Zvector E6 tests for VRR-i encoded instructions:
*
*   E650 VCVB    - VECTOR CONVERT TO BINARY  (32)
*   E652 VCVBG   - VECTOR CONVERT TO BINARY  (64)
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

loadcore    "$(testpath)/zvector-e6-11-convertbinary.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
