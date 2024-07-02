*Testcase zvector-e6-02-stores: VECTOR E6 VRX store instructions
*
*   Zvector E6 instruction tests for VRX encoded:
*
*   E609 VSTEBRH - VECTOR STORE BYTE REVERSED ELEMENT (16)
*   E60A VSTEBRG - VECTOR STORE BYTE REVERSED ELEMENT (64)
*   E60B VSTEBRF - VECTOR STORE BYTE REVERSED ELEMENT (32)
*   E60E VSTBR   - VECTOR STORE BYTE REVERSED ELEMENTS
*   E60F VSTER   - VECTOR STORE ELEMENTS REVERSED
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

loadcore    "$(testpath)/zvector-e6-02-stores.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
