*Testcase zvector-e6-01-loads: VECTOR E6 VRX load instructions
*
*   Zvector E6 instruction tests for VRX encoded:
*
*   E601 VLEBRH  - VECTOR LOAD BYTE REVERSED ELEMENT (16)
*   E602 VLEBRG  - VECTOR LOAD BYTE REVERSED ELEMENT (64)
*   E603 VLEBRF  - VECTOR LOAD BYTE REVERSED ELEMENT (32)
*   E604 VLLEBRZ - VECTOR LOAD BYTE REVERSED ELEMENT AND ZERO
*   E605 VLBRREP - VECTOR LOAD BYTE REVERSED ELEMENT AND REPLICATE
*   E606 VLBR    - VECTOR LOAD BYTE REVERSED ELEMENTS
*   E607 VLER    - VECTOR LOAD ELEMENTS REVERSED
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

loadcore    "$(testpath)/zvector-e6-01-loads.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
