*Testcase zvector-e7-10-multiplyAdd
*
*   Zvector E7 instruction tests for VRR-d encoded:
*
*   E7A9 VMALH  - Vector Multiply and Add Logical High
*   E7AA VMAL   - Vector Multiply and Add Low
*   E7AB VMAH   - Vector Multiply and Add High
*   E7AC VMALE  - Vector Multiply and Add Logical Even
*   E7AD VMALO  - Vector Multiply and Add Logical Odd
*   E7AE VMAE   - Vector Multiply and Add Even
*   E7AF VMAO   - Vector Multiply and Add Odd
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

loadcore    "$(testpath)/zvector-e7-10-multiplyAdd.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5         #
diag8cmd    disable   # (reset back to default)

*Done
