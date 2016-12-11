*Testcase KMCTR fc0
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000000     # LA R0,0           R0->function code 0
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=41200000     # LA R2,FO          R2->first operand
r 20C=41400000     # LA R4,SO          R4->second operand
r 210=41500000     # LA R5,SOL         R5->second operand length
r 214=41600000     # LA R6,TO          R6->third operand
r 218=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 21C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
*
r 580=F0703838000000000000000000000000 # Expected result
*
runtest .1
*Compare
* Display parameter block
r 500.10
*Want  F0703838 00000000 00000000 00000000
*Done

*Testcase KMCTR bad
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000000     # LA R0,63          R0->function code 0
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=41200000     # LA R2,FO          R2->first operand
r 20C=41400000     # LA R4,SO          R4->second operand
r 210=41500000     # LA R5,SOL         R5->second operand length
r 214=41600000     # LA R6,TO          R6->third operand
r 218=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 21C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*Program 6
runtest .1
*Done

*Testcase KMCTR fc1
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000001     # LA R0,1           R0->function code 1
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500008     # LA R5,SOL         R5->second operand length
r 214=4160f800      #  LA R6,TO          R6->third operand
r 218=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 21C=4120f900      #  LA R2,FO          R2->first operand
r 220=4140f600      #  LA R4,SO          R4->second operand
r 224=41500008     # LA R5,SOL         R5->second operand length
r 228=4160f800      #  LA R6,TO          R6->third operand
r 22C=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=0001020304050607                 # Parameter block
r 600=1011121314151617                 # First operand
r 700=2021222324252627                 # Second operand
r 800=3031323334353637                 # Third operand
*
r 680=ACCE8C43F1F6EFBB                 # Expected result
*
runtest .1
*Compare

* Display parameter block
r 600.8
*Want  ACCE8C43 F1F6EFBB
r 608.8
*Want  00000000 00000000
* Expected result
*Done

*Testcase KMCTR fc2
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000002     # LA R0,2           R0->function code 2
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500008     # LA R5,SOL         R5->second operand length
r 214=4160f800      #  LA R6,TO          R6->third operand
r 218=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 21C=4120f900      #  LA R2,FO          R2->first operand
r 220=4140f600      #  LA R4,SO          R4->second operand
r 224=41500008     # LA R5,SOL         R5->second operand length
r 228=4160f800      #  LA R6,TO          R6->third operand
r 22C=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 600=1011121314151617                 # First operand
r 700=2021222324252627                 # Second operand
r 800=3031323334353637                 # Third operand
*
r 680=DDEB7099FF49EFED                 # Expected result
*
runtest .1
*Compare
* Display parameter block
r 600.8
*Want  DDEB7099 FF49EFED
r 608.8
*Want  00000000 00000000
* Expected result
*Done

*Testcase KMCTR fc3
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000003     # LA R0,3           R0->function code 3
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500008     # LA R5,SOL         R5->second operand length
r 214=4160f800      #  LA R6,TO          R6->third operand
r 218=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 21C=4120f900      #  LA R2,FO          R2->first operand
r 220=4140f600      #  LA R4,SO          R4->second operand
r 224=41500008     # LA R5,SOL         R5->second operand length
r 228=4160f800      #  LA R6,TO          R6->third operand
r 22C=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=1011121314151617                 # Parameter block
r 600=1011121314151617                 # First operand
r 700=2021222324252627                 # Second operand
r 800=3031323334353637                 # Third operand
*
r 680=C53B7B40838457C8                 # Expected result
*
runtest .1
*Compare
* Display parameter block
r 600.8
*Want  C53B7B40 838457C8
r 608.8
*Want  00000000 00000000
* Expected result
*Done

*Testcase KMCTR fc9
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000001     # LA R0,X'01'       R0->function code 1
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=41000009     # LA R0,X'09'       R0->function code 9 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500008     # LA R5,SOL         R5->second operand length
r 220=4160f800      #  LA R6,TO          R6->third address
r 224=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 228=4120f900      #  LA R2,FO          R2->first operand
r 22C=4140f600      #  LA R4,SO          R4->second operand
r 230=41500008     # LA R5,SOL         R5->second operand length
r 234=4160f800      #  LA R6,TO          R6->third operand
r 238=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 23C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 600=1011121314151617                 # CV
r 700=2021222324252627                 # First operand
r 800=3031323334353637                 # Expected result
*
r 680=ACCE8C43F1F6EFBB                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  ACCE8C43 F1F6EFBB
r 900.8
*Want  20212223 24252627
*Done

*Testcase KMCTR fc10
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000002     # LA R0,X'02'       R0->function code 2
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=4100000A     # LA R0,X'0A'       R0->function code 10 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500008     # LA R5,SOL         R5->second operand length
r 220=4160f800      #  LA R6,TO          R6->third address
r 224=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 228=4120f900      #  LA R2,FO          R2->first operand
r 22C=4140f600      #  LA R4,SO          R4->second operand
r 230=41500008     # LA R5,SOL         R5->second operand length
r 234=4160f800      #  LA R6,TO          R6->third operand
r 238=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 23C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=2021222324252627                 # Parameter block
r 600=1011121314151617                 # CV
r 700=2021222324252627                 # First operand
r 800=3031323334353637                 # Expected result
*
r 680=DDEB7099FF49EFED                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  DDEB7099 FF49EFED
r 900.8
*Want  20212223 24252627
*Done

*Testcase KMCTR fc11
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000003     # LA R0,X'03'       R0->function code 3
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=4100000B     # LA R0,X'0B'       R0->function code 11 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500008     # LA R5,SOL         R5->second operand length
r 220=4160f800      #  LA R6,TO          R6->third address
r 224=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 228=4120f900      #  LA R2,FO          R2->first operand
r 22C=4140f600      #  LA R4,SO          R4->second operand
r 230=41500008     # LA R5,SOL         R5->second operand length
r 234=4160f800      #  LA R6,TO          R6->third operand
r 238=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 23C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 600=1011121314151617                 # CV
r 700=2021222324252627                 # First operand
r 800=3031323334353637                 # Expected result
*
r 680=C53B7B40838457C8                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  C53B7B40 838457C8
r 900.8
*Want  20212223 24252627
*Done

*Testcase KMCTR fc18
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000012     # LA R0,18          R0->function code 18
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=4160f800      #  LA R6,TO          R6->third operand
r 218=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 21C=4120f900      #  LA R2,FO          R2->first operand
r 220=4140f600      #  LA R4,SO          R4->second operand
r 224=41500010     # LA R5,SOL         R5->second operand length
r 228=4160f800      #  LA R6,TO          R6->third operand
r 22C=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 600=101112131415161718191A1B1C1D1E1F # First operand
r 700=202122232425262728292A2B2C2D2E2F # Second operand
r 800=303132333435363738393A3B3C3D3E3F # Third operand
*
r 680=23D3E19EEEA74DD7AAFEE59B19E096EE # Expected result
*
runtest .1
*Compare
* Display parameter block
r 600.10
*Want  23D3E19E EEA74DD7 AAFEE59B 19E096EE
*Done

*Testcase KMCTR fc19
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000013     # LA R0,19          R0->function code 19
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=4160f800      #  LA R6,TO          R6->third operand
r 218=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 21C=4120f900      #  LA R2,FO          R2->first operand
r 220=4140f600      #  LA R4,SO          R4->second operand
r 224=41500010     # LA R5,SOL         R5->second operand length
r 228=4160f800      #  LA R6,TO          R6->third operand
r 22C=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=1011121314151617                 # Parameter block
r 600=101112131415161718191A1B1C1D1E1F # First operand
r 700=202122232425262728292A2B2C2D2E2F # Second operand
r 800=303132333435363738393A3B3C3D3E3F # Third operand
*
r 680=4804D24C513D7B6F130879534F3FAFB1 # Expected result
*
runtest .1
*Compare
* Display parameter block
r 600.10
*Want  4804D24C 513D7B6F 13087953 4F3FAFB1
*Done

*Testcase KMCTR fc20
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000014     # LA R0,20          R0->function code 20
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=4160f800      #  LA R6,TO          R6->third operand
r 218=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 21C=4120f900      #  LA R2,FO          R2->first operand
r 220=4140f600      #  LA R4,SO          R4->second operand
r 224=41500010     # LA R5,SOL         R5->second operand length
r 228=4160f800      #  LA R6,TO          R6->third operand
r 22C=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 600=101112131415161718191A1B1C1D1E1F # First operand
r 700=202122232425262728292A2B2C2D2E2F # Second operand
r 800=303132333435363738393A3B3C3D3E3F # Third operand
*
r 680=C2552CA9DEF1C2F675244C301403E4A6 # Expected result
*
runtest .1
*Compare
* Display parameter block
r 600.10
*Want  C2552CA9 DEF1C2F6 75244C30 1403E4A6
*Done

*Testcase KMCTR fc26
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000012     # LA R0,X'12'       R0->function code 18
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=4100001A     # LA R0,X'1A'       R0->function code 26 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500010     # LA R5,SOL         R5->second operand length
r 220=4160f800      #  LA R6,TO          R6->third address
r 224=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 228=4120f900      #  LA R2,FO          R2->first operand
r 22C=4140f600      #  LA R4,SO          R4->second operand
r 230=41500010     # LA R5,SOL         R5->second operand length
r 234=4160f800      #  LA R6,TO          R6->third operand
r 238=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 23C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 600=101112131415161728292A2B2C2D2E2F # CV
r 700=202122232425262728292A2B2C2D2E2F # First operand
r 800=303132333435363738393A3B3C3D3E3F # Expected result
*
r 680=23D3E19EEEA74DD7AAFEE59B19E096EE # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  23D3E19E EEA74DD7
r 900.8
*Want  20212223 24252627
*Done

*Testcase KMCTR fc27
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000013     # LA R0,X'13'       R0->function code 19
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=4100001B     # LA R0,X'1B'       R0->function code 27 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500010     # LA R5,SOL         R5->second operand length
r 220=4160f800      #  LA R6,TO          R6->third address
r 224=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 228=4120f900      #  LA R2,FO          R2->first operand
r 22C=4140f600      #  LA R4,SO          R4->second operand
r 230=41500010     # LA R5,SOL         R5->second operand length
r 234=4160f800      #  LA R6,TO          R6->third operand
r 238=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 23C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=303132333435363738393A3B3C3D3E3F # Parameter block
r 600=101112131415161718191A1B1C1D1E1F # CV
r 700=202122232425262728292A2B2C2D2E2F # First operand
r 800=303132333435363738393A3B3C3D3E3F # Expected result
*
r 680=4804D24C513D7B6F130879534F3FAFB1 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  4804D24C 513D7B6F
r 900.8
*Want  20212223 24252627
*Done

*Testcase KMCTR fc28
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000014     # LA R0,X'14'       R0->function code 20
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=4100001C     # LA R0,X'1C'       R0->function code 28 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500010     # LA R5,SOL         R5->second operand length
r 220=4160f800      #  LA R6,TO          R6->third address
r 224=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 228=4120f900      #  LA R2,FO          R2->first operand
r 22C=4140f600      #  LA R4,SO          R4->second operand
r 230=41500010     # LA R5,SOL         R5->second operand length
r 234=4160f800      #  LA R6,TO          R6->third operand
r 238=B92D6024     # KMCTR R2,R6,R4    Cipher message with counter
r 23C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=303132333435363738393A3B3C3D3E3F # Parameter block
r 600=101112131415161718191A1B1C1D1E1F # CV
r 700=202122232425262728292A2B2C2D2E2F # First operand
r 800=303132333435363738393A3B3C3D3E3F # Expected result
*
r 680=C2552CA9DEF1C2F675244C301403E4A6 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.10
*Want  C2552CA9 DEF1C2F6 75244C30 1403E4A6
r 900.10
* Expected results
*Want  20212223 24252627 28292A2B 2C2D2E2F
*Done
