*Testcase zvector-e7-13-UnpackMisc
*
*   Zvector E7 instruction tests for VRR-a encoded:
*
*   E7D4 VUPLL  - Vector Unpack Logical Low
*   E7D5 VUPLH  - Vector Unpack Logical High
*   E7D6 VUPL   - Vector Unpack Low
*   E7D7 VUPH   - Vector Unpack High
*
*   E75F VSEG   - Vector Sign Extend To Doubleword
*   E7DE VLC    - Vector Load Complement
*   E7DF VLP    - Vector Load Positive
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

loadcore    "$(testpath)/zvector-e7-13-UnpackMisc.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5         #
diag8cmd    disable   # (reset back to default)

*Done
