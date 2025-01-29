*Testcase zvector-e7-04-BitCount
*
*   Zvector E7 instruction tests for VRR-a encoded:
*
*   E750 VPOPCT - Vector Population Count
*   E752 VCTZ   - Vector Count Trailing Zeros
*   E753 VCLZ   - Vector Count Leading Zeros
*
*   # ------------------------------------------------------------
*   #  This tests only the basic function of the instruction.
*   #  Exceptions are NOT tested.
*   # ------------------------------------------------------------
*
mainsize    2
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/zvector-e7-04-BitCount.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     10        # (2 secs if intrinsic used, 10 otherwise!)
diag8cmd    disable   # (reset back to default)

*Done
