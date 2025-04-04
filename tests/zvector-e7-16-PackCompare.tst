*Testcase zvector-e7-16-PackCompare
*
*   Zvector E7 instruction tests for VRR-b encoded:
*
*   E795 VPKLS  - Vector Pack Logical Saturate
*   E797 VPKS   - Vector Pack Saturate
*   E7F8 VCEQ   - Vector Compare Equal
*   E7F9 VCHL   - Vector Compare High Logical
*   E7FB VCH    - Vector Compare High
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

loadcore    "$(testpath)/zvector-e7-16-PackCompare.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5
diag8cmd    disable   # (reset back to default)

*Done
