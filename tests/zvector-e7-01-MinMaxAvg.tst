*Testcase zvector-e7-01-MinMaxAvg: VECTOR E7 VRR-c instructions
*
*   Zvector E7 instruction tests for VRR-c encoded:
*
*   E7F0 VAVGL  - VECTOR AVERAGE LOGICAL
*   E7F2 VAVG   - VECTOR AVERAGE
*   E7FC VMNL   - VECTOR MINIMUM LOGICAL
*   E7FD VMXL   - VECTOR MAXIMUM LOGICAL
*   E7FE VMN    - VECTOR MINIMUM
*   E7FF VMX    - VECTOR MAXIMUM
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

loadcore    "$(testpath)/zvector-e7-01-MinMaxAvg.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
