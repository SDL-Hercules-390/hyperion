*Testcase mie4-03-IndexedAddress
*
*   Miscellaneous-instruction-extensions facility 4 instruction tests
*   for RXY-c encoded:
*
*   E360  LXAB  - load indexed address (shift left 0)          [RXY-c]
*   E361  LLXAB - load logical indexed address (shift left 0)  [RXY-c]
*   E362  LXAH  - load indexed address (shift left 1)          [RXY-c]
*   E363  LLXAH - load logical indexed address (shift left 1)  [RXY-c]
*   E364  LXAF  - load indexed address (shift left 2)          [RXY-c]
*   E365  LLXAF - load logical indexed address (shift left 2)  [RXY-c]
*   E366  LXAG  - load indexed address (shift left 3)          [RXY-c]
*   E367  LLXAG - load logical indexed address (shift left 3)  [RXY-c]
*   E368  LXAQ  - load indexed address (shift left 4)          [RXY-c]
*   E369  LLXAQ - load logical indexed address (shift left 4)  [RXY-c]
*
*   # ------------------------------------------------------------
*   #  This tests only the basic function of the instruction.
*   #  Exceptions are NOT tested.
*   # ------------------------------------------------------------
*
mainsize    1
numcpu      1
sysclear
archlvl     z/Arch

loadcore    "$(testpath)/mie4-03-IndexedAddress.core" 0x0

diag8cmd    enable    # (needed for messages to Hercules console)
runtest     5         #
diag8cmd    disable   # (reset back to default)

*Done
