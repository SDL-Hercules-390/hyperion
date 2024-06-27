*
* -------------------------------------------------------------------
*  S/370 SSK test (Storage-key 4K-byte-block Facility NOT installed)
* -------------------------------------------------------------------
*
mainsize          2M
numcpu 1
*
* page addresses
*
defsym  page0     00000000  # (pr0)
defsym  page1     00000800  # (pr1)
defsym  page2     00001000
defsym  page3     00001800
defsym  page4     00002000  #  pr0
defsym  page5     00002800  #  pr1
defsym  page6     00003000
defsym  nextlast  001FF000
defsym  lastpage  001FF800
defsym  pastlast  00200000
*
defsym  prpage  $(page4)    # prefix page 0
*
* table of page addresses
*
defsym apage0     0400
defsym apage1     0404
defsym apage2     0408
defsym apage3     040C
defsym apage4     0410
defsym apage5     0414
defsym apage6     0418
defsym anextlast  041C
defsym alastpage  0420
defsym apastlast  0424
*
* -----------------------------------------------------------------
*Testcase SSK S/370  Specification Exception (low reg bits)
*
sysclear
archmode S/370
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=58100308                # L    R1,bad           INVALID
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=0801                    # SSK  R0,R1
r 20a=82000300                # LPSW DONEPSW
*
r 300=000A000000000000        # EC mode end-of-test PSW
r 308=00000801                # bad page address
*
r $(apage0)=$(page0)          # table of page addresses...
r $(apage1)=$(page1)
r $(apage2)=$(page2)
r $(apage3)=$(page3)
r $(apage4)=$(page4)
r $(apage5)=$(page5)
r $(apage6)=$(page6)
r $(anextlast)=$(nextlast)
r $(alastpage)=$(lastpage)
r $(apastlast)=$(pastlast)
*
*Program 0006
runtest .1
*Done
*
* -----------------------------------------------------------------
*Testcase SSK S/370  Addressing Exception
*
sysclear
archmode S/370
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(apastlast)        # L    R1,bad           TOOBIG
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=0801                    # SSK  R0,R1
r 20a=82000300                # LPSW DONEPSW
*
r 300=000A000000000000        # EC mode end-of-test PSW
*
r $(apage0)=$(page0)          # table of page addresses...
r $(apage1)=$(page1)
r $(apage2)=$(page2)
r $(apage3)=$(page3)
r $(apage4)=$(page4)
r $(apage5)=$(page5)
r $(apage6)=$(page6)
r $(anextlast)=$(nextlast)
r $(alastpage)=$(lastpage)
r $(apastlast)=$(pastlast)
*
*Program 0005
runtest .1
*Done
*
* -----------------------------------------------------------------
*Testcase SSK S/370  page0
defsym    sskpage  $(page0)   # addr SSK instr target page
defsym   asskpage $(apage0)   # addr SSK instr target page
*
sysclear
archmode S/370
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=0801                    # SSK  R0,R1
r 20a=82000300                # LPSW DONEPSW
*
r 300=000A000000000000        # EC mode end-of-test PSW
*
r $(apage0)=$(page0)          # table of page addresses...
r $(apage1)=$(page1)
r $(apage2)=$(page2)
r $(apage3)=$(page3)
r $(apage4)=$(page4)
r $(apage5)=$(page5)
r $(apage6)=$(page6)
r $(anextlast)=$(nextlast)
r $(alastpage)=$(lastpage)
r $(apastlast)=$(pastlast)
*
runtest               # ---------- BEGIN TEST -----------
*
*Compare
abs $(page0).1
*Key 00
*
*Compare
abs $(page1).1
*Key 00
*
*Compare
abs $(page2).1
*Key 00
*
*Compare
abs $(page3).1
*Key 00
*
*Compare
abs $(page4).1        # pr0
*Key F4               # SSK instr target
#                     # +REF because also LPSW instr target
*
*Compare
abs $(page5).1        # pr1
*Key 00
*
*Compare
abs $(page6).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSK S/370  page1
defsym    sskpage  $(page1)   # addr SSK instr target page
defsym   asskpage $(apage1)   # addr SSK instr target page
*
sysclear
archmode S/370
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=0801                    # SSK  R0,R1
r 20a=82000300                # LPSW DONEPSW
*
r 300=000A000000000000        # EC mode end-of-test PSW
*
r $(apage0)=$(page0)          # table of page addresses...
r $(apage1)=$(page1)
r $(apage2)=$(page2)
r $(apage3)=$(page3)
r $(apage4)=$(page4)
r $(apage5)=$(page5)
r $(apage6)=$(page6)
r $(anextlast)=$(nextlast)
r $(alastpage)=$(lastpage)
r $(apastlast)=$(pastlast)
*
runtest               # ---------- BEGIN TEST -----------
*
*Compare
abs $(page0).1
*Key 00
*
*Compare
abs $(page1).1
*Key 00
*
*Compare
abs $(page2).1
*Key 00
*
*Compare
abs $(page3).1
*Key 00
*
*Compare
abs $(page4).1        # pr0
*Key 06               # REF+CHG because of pgm setup via 'r' cmd
*
*Compare
abs $(page5).1        # pr1
*Key F0               # SSK instr target
*
*Compare
abs $(page6).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSK S/370  page2
defsym    sskpage  $(page2)   # addr SSK instr target page
defsym   asskpage $(apage2)   # addr SSK instr target page
*
sysclear
archmode S/370
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=0801                    # SSK  R0,R1
r 20a=82000300                # LPSW DONEPSW
*
r 300=000A000000000000        # EC mode end-of-test PSW
*
r $(apage0)=$(page0)          # table of page addresses...
r $(apage1)=$(page1)
r $(apage2)=$(page2)
r $(apage3)=$(page3)
r $(apage4)=$(page4)
r $(apage5)=$(page5)
r $(apage6)=$(page6)
r $(anextlast)=$(nextlast)
r $(alastpage)=$(lastpage)
r $(apastlast)=$(pastlast)
*
runtest               # ---------- BEGIN TEST -----------
*
*Compare
abs $(page0).1
*Key 00
*
*Compare
abs $(page1).1
*Key 00
*
*Compare
abs $(page2).1
*Key F0               # SSK instr target
*
*Compare
abs $(page3).1
*Key 00
*
*Compare
abs $(page4).1        # pr0
*Key 06               # REF+CHG because of pgm setup via 'r' cmd
*
*Compare
abs $(page5).1        # pr1
*Key 00
*
*Compare
abs $(page6).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSK S/370  page3
defsym    sskpage  $(page3)   # addr SSK instr target page
defsym   asskpage $(apage3)   # addr SSK instr target page
*
sysclear
archmode S/370
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=0801                    # SSK  R0,R1
r 20a=82000300                # LPSW DONEPSW
*
r 300=000A000000000000        # EC mode end-of-test PSW
*
r $(apage0)=$(page0)          # table of page addresses...
r $(apage1)=$(page1)
r $(apage2)=$(page2)
r $(apage3)=$(page3)
r $(apage4)=$(page4)
r $(apage5)=$(page5)
r $(apage6)=$(page6)
r $(anextlast)=$(nextlast)
r $(alastpage)=$(lastpage)
r $(apastlast)=$(pastlast)
*
runtest               # ---------- BEGIN TEST -----------
*
*Compare
abs $(page0).1
*Key 00
*
*Compare
abs $(page1).1
*Key 00
*
*Compare
abs $(page2).1
*Key 00
*
*Compare
abs $(page3).1
*Key F0               # SSK instr target
*
*Compare
abs $(page4).1        # pr0
*Key 06               # REF+CHG because of pgm setup via 'r' cmd
*
*Compare
abs $(page5).1        # pr1
*Key 00
*
*Compare
abs $(page6).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSK S/370  page4
defsym    sskpage  $(page4)   # addr SSK instr target page
defsym   asskpage $(apage4)   # addr SSK instr target page
*
sysclear
archmode S/370
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=0801                    # SSK  R0,R1
r 20a=82000300                # LPSW DONEPSW
*
r 300=000A000000000000        # EC mode end-of-test PSW
*
r $(apage0)=$(page0)          # table of page addresses...
r $(apage1)=$(page1)
r $(apage2)=$(page2)
r $(apage3)=$(page3)
r $(apage4)=$(page4)
r $(apage5)=$(page5)
r $(apage6)=$(page6)
r $(anextlast)=$(nextlast)
r $(alastpage)=$(lastpage)
r $(apastlast)=$(pastlast)
*
runtest               # ---------- BEGIN TEST -----------
*
*Compare
abs $(page0).1
*Key F0               # SSK instr target
*
*Compare
abs $(page1).1
*Key 00
*
*Compare
abs $(page2).1
*Key 00
*
*Compare
abs $(page3).1
*Key 00
*
*Compare
abs $(page4).1        # pr0
*Key 06               # REF+CHG because of pgm setup via 'r' cmd
*
*Compare
abs $(page5).1        # pr1
*Key 00
*
*Compare
abs $(page6).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSK S/370  page5
defsym    sskpage  $(page5)   # addr SSK instr target page
defsym   asskpage $(apage5)   # addr SSK instr target page
*
sysclear
archmode S/370
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=0801                    # SSK  R0,R1
r 20a=82000300                # LPSW DONEPSW
*
r 300=000A000000000000        # EC mode end-of-test PSW
*
r $(apage0)=$(page0)          # table of page addresses...
r $(apage1)=$(page1)
r $(apage2)=$(page2)
r $(apage3)=$(page3)
r $(apage4)=$(page4)
r $(apage5)=$(page5)
r $(apage6)=$(page6)
r $(anextlast)=$(nextlast)
r $(alastpage)=$(lastpage)
r $(apastlast)=$(pastlast)
*
runtest               # ---------- BEGIN TEST -----------
*
*Compare
abs $(page0).1
*Key 00
*
*Compare
abs $(page1).1
*Key F0               # SSK instr target
*
*Compare
abs $(page2).1
*Key 00
*
*Compare
abs $(page3).1
*Key 00
*
*Compare
abs $(page4).1        # pr0
*Key 06               # REF+CHG because of pgm setup via 'r' cmd
*
*Compare
abs $(page5).1        # pr1
*Key 00
*
*Compare
abs $(page6).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSK S/370  page6
defsym    sskpage  $(page6)   # addr SSK instr target page
defsym   asskpage $(apage6)   # addr SSK instr target page
*
sysclear
archmode S/370
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=0801                    # SSK  R0,R1
r 20a=82000300                # LPSW DONEPSW
*
r 300=000A000000000000        # EC mode end-of-test PSW
*
r $(apage0)=$(page0)          # table of page addresses...
r $(apage1)=$(page1)
r $(apage2)=$(page2)
r $(apage3)=$(page3)
r $(apage4)=$(page4)
r $(apage5)=$(page5)
r $(apage6)=$(page6)
r $(anextlast)=$(nextlast)
r $(alastpage)=$(lastpage)
r $(apastlast)=$(pastlast)
*
runtest               # ---------- BEGIN TEST -----------
*
*Compare
abs $(page0).1
*Key 00
*
*Compare
abs $(page1).1
*Key 00
*
*Compare
abs $(page2).1
*Key 00
*
*Compare
abs $(page3).1
*Key 00
*
*Compare
abs $(page4).1        # pr0
*Key 06               # REF+CHG because of pgm setup via 'r' cmd
*
*Compare
abs $(page5).1        # pr1
*Key 00
*
*Compare
abs $(page6).1
*Key F0               # SSK instr target
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSK S/370  nextlast
defsym    sskpage  $(nextlast)  # addr SSK instr target page
defsym   asskpage $(anextlast)  # addr SSK instr target page
*
sysclear
archmode S/370
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=0801                    # SSK  R0,R1
r 20a=82000300                # LPSW DONEPSW
*
r 300=000A000000000000        # EC mode end-of-test PSW
*
r $(apage0)=$(page0)          # table of page addresses...
r $(apage1)=$(page1)
r $(apage2)=$(page2)
r $(apage3)=$(page3)
r $(apage4)=$(page4)
r $(apage5)=$(page5)
r $(apage6)=$(page6)
r $(anextlast)=$(nextlast)
r $(alastpage)=$(lastpage)
r $(apastlast)=$(pastlast)
*
runtest               # ---------- BEGIN TEST -----------
*
*Compare
abs $(nextlast).1
*Key F0               # SSK instr target
*
*Compare
abs $(lastpage).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSK S/370  lastpage
defsym    sskpage  $(lastpage)  # addr SSK instr target page
defsym   asskpage $(alastpage)  # addr SSK instr target page
*
sysclear
archmode S/370
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=0801                    # SSK  R0,R1
r 20a=82000300                # LPSW DONEPSW
*
r 300=000A000000000000        # EC mode end-of-test PSW
*
r $(apage0)=$(page0)          # table of page addresses...
r $(apage1)=$(page1)
r $(apage2)=$(page2)
r $(apage3)=$(page3)
r $(apage4)=$(page4)
r $(apage5)=$(page5)
r $(apage6)=$(page6)
r $(anextlast)=$(nextlast)
r $(alastpage)=$(lastpage)
r $(apastlast)=$(pastlast)
*
runtest               # ---------- BEGIN TEST -----------
*
*Compare
abs $(nextlast).1
*Key 00
*
*Compare
abs $(lastpage).1
*Key F0               # SSK instr target
*
*Done
