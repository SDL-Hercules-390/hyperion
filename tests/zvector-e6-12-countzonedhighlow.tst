*Testcase zvector-e6-12-countzonedhighlow: VECTOR E6 VRR-k instructions
*
*   Zvector E6 tests for VRR-k encoded instructions:
*
*   E651 VCLZDP  - VECTOR COUNT LEADING ZERO DIGITS
*   E654 VUPKZH  - VECTOR UNPACK ZONED HIGH
*   E65C VUPKZL  - VECTOR UNPACK ZONED LOW
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

loadcore    "$(testpath)/zvector-e6-12-countzonedhighlow.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     2
diag8cmd    disable   # (reset back to default)

*Done
