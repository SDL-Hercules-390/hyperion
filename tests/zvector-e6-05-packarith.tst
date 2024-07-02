*Testcase zvector-e6-05-packarith: VECTOR E6 VRI-f packed arithmetic instructions
*
*   Zvector E6 tests for VRI-f encoded packed decimal
*   arithmetic instructions:
*
*   E671 VAP    - VECTOR ADD DECIMAL
*   E673 VSP    - VECTOR SUBTRACT DECIMAL
*   E678 VMP    - VECTOR MULTIPLY DECIMAL
*   E679 VMSP   - VECTOR MULTIPLY AND SHIFT DECIMAL
*   E67A VDP    - VECTOR DIVIDE DECIMAL
*   E67B VRP    - VECTOR REMAINDER DECIMAL
*   E67E VSDP   - VECTOR SHIFT AND DIVIDE DECIMAL
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

loadcore    "$(testpath)/zvector-e6-05-packarith.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
