*
* --------------------------------------------------------------------
*  S/390 SSKE test
* --------------------------------------------------------------------
*
mainsize          2M
*
* page addresses
*
defsym  page0     00000000  # (pr)
defsym  page1     00001000
defsym  page2     00002000
defsym  page3     00003000
defsym  page4     00004000  #  pr
defsym  page5     00005000
defsym  page6     00006000
defsym  nextlast  001FE000
defsym  lastpage  001FF000
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
*Testcase SSKE S/390 Successful (low reg bits)
*
sysclear
archmode S/390
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=58100308                # L    R1,bad           INVALID
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=B22B0001                # SSKE R0,R1
r 20C=82000300                # LPSW DONEPSW
*
r 300=000A000000000000        # EC mode end-of-test PSW
r 308=00006fff                # "bad" page6 address
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
abs $(page6).1
*Key F0               # SSKE target
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSKE S/390 Successful (high reg bit)
*
sysclear
archmode S/390
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=58100308                # L    R1,bad           INVALID
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=B22B0001                # SSKE R0,R1
r 20C=82000300                # LPSW DONEPSW
*
r 300=000A000000000000        # EC mode end-of-test PSW
r 308=80006000                # "bad" page6 address
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
abs $(page6).1
*Key F0               # SSKE target
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSKE S/390 Addressing Exception
*
sysclear
archmode S/390
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(apastlast)        # L    R1,bad           TOOBIG
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=B22B0001                # SSKE R0,R1
r 20C=82000300                # LPSW DONEPSW
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
*Testcase SSKE S/390 page0
defsym    sskpage  $(page0)   # addr SSKE instr target page
defsym   asskpage $(apage0)   # addr SSKE instr target page
*
sysclear
archmode S/390
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=B22B0001                # SSKE R0,R1
r 20C=82000300                # LPSW DONEPSW
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
abs $(page4).1        # pr
*Key F4               # SSKE instr target
#                     # +REF because also LPSW instr target
*
*Compare
abs $(page5).1
*Key 00
*
*Compare
abs $(page6).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSKE S/390 page1
defsym    sskpage  $(page1)   # addr SSKE instr target page
defsym   asskpage $(apage1)   # addr SSKE instr target page
*
sysclear
archmode S/390
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=B22B0001                # SSKE R0,R1
r 20C=82000300                # LPSW DONEPSW
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
*Key F0               # SSKE instr target
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
abs $(page4).1        # pr
*Key 06               # REF+CHG because of pgm setup via 'r' cmd
*
*Compare
abs $(page5).1
*Key 00
*
*Compare
abs $(page6).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSKE S/390 page2
defsym    sskpage  $(page2)   # addr SSKE instr target page
defsym   asskpage $(apage2)   # addr SSKE instr target page
*
sysclear
archmode S/390
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=B22B0001                # SSKE R0,R1
r 20C=82000300                # LPSW DONEPSW
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
*Key F0               # SSKE instr target
*
*Compare
abs $(page3).1
*Key 00
*
*Compare
abs $(page4).1        # pr
*Key 06               # REF+CHG because of pgm setup via 'r' cmd
*
*Compare
abs $(page5).1
*Key 00
*
*Compare
abs $(page6).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSKE S/390 page3
defsym    sskpage  $(page3)   # addr SSKE instr target page
defsym   asskpage $(apage3)   # addr SSKE instr target page
*
sysclear
archmode S/390
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=B22B0001                # SSKE R0,R1
r 20C=82000300                # LPSW DONEPSW
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
*Key F0               # SSKE instr target
*
*Compare
abs $(page4).1        # pr
*Key 06               # REF+CHG because of pgm setup via 'r' cmd
*
*Compare
abs $(page5).1
*Key 00
*
*Compare
abs $(page6).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSKE S/390 page4
defsym    sskpage  $(page4)   # addr SSKE instr target page
defsym   asskpage $(apage4)   # addr SSKE instr target page
*
sysclear
archmode S/390
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=B22B0001                # SSKE R0,R1
r 20C=82000300                # LPSW DONEPSW
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
*Key F0               # SSKE instr target
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
abs $(page4).1        # pr
*Key 06               # REF+CHG because of pgm setup via 'r' cmd
*
*Compare
abs $(page5).1
*Key 00
*
*Compare
abs $(page6).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSKE S/390 page5
defsym    sskpage  $(page5)   # addr SSKE instr target page
defsym   asskpage $(apage5)   # addr SSKE instr target page
*
sysclear
archmode S/390
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=B22B0001                # SSKE R0,R1
r 20C=82000300                # LPSW DONEPSW
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
abs $(page4).1        # pr
*Key 06               # REF+CHG because of pgm setup via 'r' cmd
*
*Compare
abs $(page5).1
*Key F0               # SSKE instr target
*
*Compare
abs $(page6).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSKE S/390 page6
defsym    sskpage  $(page6)   # addr SSKE instr target page
defsym   asskpage $(apage6)   # addr SSKE instr target page
*
sysclear
archmode S/390
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=B22B0001                # SSKE R0,R1
r 20C=82000300                # LPSW DONEPSW
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
abs $(page4).1        # pr
*Key 06               # REF+CHG because of pgm setup via 'r' cmd
*
*Compare
abs $(page5).1
*Key 00
*
*Compare
abs $(page6).1
*Key F0               # SSKE instr target
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSKE S/390 nextlast
defsym    sskpage  $(nextlast)  # addr SSKE instr target page
defsym   asskpage $(anextlast)  # addr SSKE instr target page
*
sysclear
archmode S/390
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=B22B0001                # SSKE R0,R1
r 20C=82000300                # LPSW DONEPSW
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
*Key F0               # SSKE instr target
*
*Compare
abs $(lastpage).1
*Key 00
*
*Done
*
* -----------------------------------------------------------------
*Testcase SSKE S/390 lastpage
defsym    sskpage  $(lastpage)  # addr SSKE instr target page
defsym   asskpage $(alastpage)  # addr SSKE instr target page
*
sysclear
archmode S/390
pr $(prpage)
*
r 000=0008000000000200        # EC mode restart PSW
r 068=000A00000000DEAD        # EC mode pgm new PSW
*
r 200=5810$(asskpage)         # L    R1,pageaddr
r 204=410000F0                # LA   R0,keyvalue      KEY F0
r 208=B22B0001                # SSKE R0,R1
r 20C=82000300                # LPSW DONEPSW
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
*Key F0               # SSKE instr target
*
*Done
