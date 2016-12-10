*Testcase KMC fc0
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000000     # LA R0,0           R0->function code 0
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=41200000     # LA R2,FO          R2->first operand
r 20C=41400000     # LA R4,SO          R4->second operand
r 210=41500000     # LA R5,SOL         R5->second operand length
r 214=B92F0024     # KMC R2,R4         Cipher message with chaining
r 218=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
*
r 580=F0703838000000001000000000000000 # Expected result
*
runtest .1
*Compare
* Display parameter block
r 500.10
*Want  F0703838 00000000 10000000 00000000
* Expected result
*Done

*Testcase KMC bad
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=4100007f     # LA R0,0           R0->function code 0
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=41200000     # LA R2,FO          R2->first operand
r 20C=41400000     # LA R4,SO          R4->second operand
r 210=41500000     # LA R5,SOL         R5->second operand length
r 214=B92F0024     # KMC R2,R4         Cipher message with chaining
r 218=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
*Program 6
runtest .1
*Done
*Testcase KMC badreg0
r 214=B92F0004     # KMC R2,R4         Cipher message with chaining
*Program 6
runtest .1
*Done
*Testcase KMC badreg3
r 214=B92F0034     # KMC R2,R4         Cipher message with chaining
*Program 6
runtest .1
*Done

*Testcase KMC fc1
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000001     # LA R0,X'01'       R0->function code 1 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500008     # LA R5,SOL         R5->second operand length
r 214=B92F0024     # KMC R2,R4         Cipher message with chaining
r 218=41000081     # LA R0,X'81'       R0->function code 1 decrypt
r 21C=4110f550      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500008     # LA R5,FOL         R5->first operand length
r 22C=B92F0024     # KMC R2,R4         Cipher message with chaining
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 550=000102030405060708090A0B0C0D0E0F # Parameter block
*
r 600=0001020304050607                 # First operand
*
r 680=D7423E1B84911C2E                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  D7423E1B 84911C2E
r 800.8
*Want  00010203 04050607
* Expected results
*Done

*Testcase KMC fc2
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000002     # LA R0,X'02'       R0->function code 2 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500008     # LA R5,SOL         R5->second operand length
r 214=B92F0024     # KMC R2,R4         Cipher message with chaining
r 218=41000082     # LA R0,X'82'       R0->function code 2 decrypt
r 21C=4110f550      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500008     # LA R5,FOL         R5->first operand length
r 22C=B92F0024     # KMC R2,R4         Cipher message with chaining
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=1011121314151617                 # Parameter block
r 550=000102030405060708090A0B0C0D0E0F # Parameter block
r 560=1011121314151617                 # Parameter block
*
r 600=0001020304050607                 # First operand
*
r 680=F4F9F93F1B40EDE7                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  F4F9F93F 1B40EDE7
r 800.8
*Want  00010203 04050607
* Expected results
*Done

*Testcase KMC fc3
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000003     # LA R0,X'03'       R0->function code 3 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500008     # LA R5,SOL         R5->second operand length
r 214=B92F0024     # KMC R2,R4         Cipher message with chaining
r 218=41000083     # LA R0,X'83'       R0->function code 3 decrypt
r 21C=4110f550      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500008     # LA R5,FOL         R5->first operand length
r 22C=B92F0024     # KMC R2,R4         Cipher message with chaining
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 550=000102030405060708090A0B0C0D0E0F # Parameter block
r 560=101112131415161718191A1B1C1D1E1F # Parameter block
*
r 600=0001020304050607                 # First operand
*
r 680=5790A6D02A3BF337                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  5790A6D0 2A3BF337
r 800.8
*Want  00010203 04050607
* Expected results
*Done

*Testcase KMC fc9
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000001     # LA R0,X'01'       R0->function code 1
r 204=4110f508      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=41000009     # LA R0,X'09'       R0->function code 9 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500008     # LA R5,SOL         R5->second operand length
r 220=B92F0024     # KMC R2,R4         Cipher message
r 224=4180f590      #  LA R8,CV          Load address CV
r 228=D20710008000 # MVC ?             Copy CV parameter value
r 22E=41000089     # LA R0,X'89'       R0->function code 9 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500008     # LA R5,FOL         R5->first operand length
r 242=B92F0024     # KMC R2,R4         Cipher message
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=2021222324252627
*
r 590=0001020304050607                 # CV
*
r 600=0001020304050607                 # First operand
*
r 680=D7423E1B84911C2E                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  D7423E1B 84911C2E
r 800.8
*Want  00010203 04050607
* Expected results
*Done

*Testcase KMC fc10
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000002     # LA R0,X'02'       R0->function code 2
r 204=4110f508      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=4100000A     # LA R0,X'0A'       R0->function code 10 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500008     # LA R5,SOL         R5->second operand length
r 220=B92F0024     # KMC R2,R4         Cipher message
r 224=4180f590      #  LA R8,CV          Load address CV
r 228=D20710008000 # MVC ?             Copy CV parameter value
r 22E=4100008A     # LA R0,X'8A'       R0->function code 10 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500008     # LA R5,FOL         R5->first operand length
r 242=B92F0024     # KMC R2,R4         Cipher message
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
*
r 590=0001020304050607                 # CV
*
r 600=0001020304050607                 # First operand
*
r 680=F4F9F93F1B40EDE7                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  F4F9F93F 1B40EDE7
r 800.8
*Want  00010203 04050607
* Expected results
*Done

*Testcase KMC fc11
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000003     # LA R0,X'03'       R0->function code 3
r 204=4110f508      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=4100000B     # LA R0,X'0B'       R0->function code 11 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500008     # LA R5,SOL         R5->second operand length
r 220=B92F0024     # KMC R2,R4         Cipher message
r 224=4180f590      #  LA R8,CV          Load address CV
r 228=D20710008000 # MVC ?             Copy CV parameter value
r 22E=4100008B     # LA R0,X'8B'       R0->function code 11 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500008     # LA R5,FOL         R5->first operand length
r 242=B92F0024     # KMC R2,R4         Cipher message
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=3031323334353637                 # Parameter block
*
r 590=0001020304050607                 # CV
*
r 600=0001020304050607                 # First operand
*
r 680=5790A6D02A3BF337                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  5790A6D0 2A3BF337
r 800.8
*Want  00010203 04050607
* Expected results
*Done

*Testcase KMC fc18
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000012     # LA R0,X'12'       R0->function code 18 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=B92F0024     # KMC R2,R4         Cipher message with chaining
r 218=41000092     # LA R0,X'92'       R0->function code 18 decrypt
r 21C=4110f550      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500010     # LA R5,FOL         R5->first operand length
r 22C=B92F0024     # KMC R2,R4         Cipher message with chaining
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 550=000102030405060708090A0B0C0D0E0F # Parameter block
r 560=101112131415161718191A1B1C1D1E1F # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=EDA330F90EECD16C003E5FB09BCFF358 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  EDA330F9 0EECD16C
r 800.8
*Want  00010203 04050607
* Expected results
*Done

*Testcase KMC fc19
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000013     # LA R0,X'13'       R0->function code 19 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=B92F0024     # KMC R2,R4         Cipher message with chaining
r 218=41000093     # LA R0,X'93'       R0->function code 19 decrypt
r 21C=4110f550      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500010     # LA R5,FOL         R5->first operand length
r 22C=B92F0024     # KMC R2,R4         Cipher message with chaining
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728               # Parameter block
r 550=000102030405060708090A0B0C0D0E0F # Parameter block
r 560=101112131415161718191A1B1C1D1E1F # Parameter block
r 570=202122232425262728               # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=7C91ED3B313477D7B3CA928CFAA752E7 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  7C91ED3B 313477D7
r 800.8
*Want  00010203 04050607
* Expected results
*Done

*Testcase KMC fc20
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000014     # LA R0,X'14'       R0->function code 20 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=B92F0024     # KMC R2,R4         Cipher message with chaining
r 218=41000094     # LA R0,X'94'       R0->function code 20 decrypt
r 21C=4110f550      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500010     # LA R5,FOL         R5->first operand length
r 22C=B92F0024     # KMC R2,R4         Cipher message with chaining
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 550=000102030405060708090A0B0C0D0E0F # Parameter block
r 560=101112131415161718191A1B1C1D1E1F # Parameter block
r 570=202122232425262728292A2B2C2D2E2F # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=5390628A3ACF964F6E02053976A8035D # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  5390628A 3ACF964F
r 800.8
*Want  00010203 04050607
* Expected results
*Done

*Testcase KMC fc26
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000012     # LA R0,X'12'       R0->function code 18
r 204=4110f510      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=4100001A     # LA R0,X'1A'       R0->function code 26 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500010     # LA R5,SOL         R5->second operand length
r 220=B92F0024     # KMC R2,R4         Cipher message
r 224=4180f590      #  LA R8,CV          Load address CV
r 228=D20F10008000 # MVC ?             Copy CV parameter value
r 22E=4100009A     # LA R0,X'9A'       R0->function code 26 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500010     # LA R5,FOL         R5->first operand length
r 242=B92F0024     # KMC R2,R4         Cipher message
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=303132333435363738393A3B3C3D3E3F # Parameter block
*
r 590=000102030405060708090A0B0C0D0E0F # CV
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=EDA330F90EECD16C003E5FB09BCFF358 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.10
*Want  EDA330F9 0EECD16C 003E5FB0 9BCFF358
r 800.10
*Want  00010203 04050607 08090A0B 0C0D0E0F
* Expected results
*Done

*Testcase KMC fc27
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000013     # LA R0,X'13'       R0->function code 19
r 204=4110f510      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=4100001B     # LA R0,X'1B'       R0->function code 27 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500010     # LA R5,SOL         R5->second operand length
r 220=B92F0024     # KMC R2,R4         Cipher message
r 224=4180f590      #  LA R8,CV          Load address CV
r 228=D20F10008000 # MVC ?             Copy CV parameter value
r 22E=4100009B     # LA R0,X'9B'       R0->function code 27 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500010     # LA R5,FOL         R5->first operand length
r 242=B92F0024     # KMC R2,R4         Cipher message
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=303132333435363738393A3B3C3D3E3F # Parameter block
r 540=0001020304050607                 # Parameter block
*
r 590=000102030405060708090A0B0C0D0E0F # CV
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=7C91ED3B313477D7B3CA928CFAA752E7 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.10
*Want  7C91ED3B 313477D7 B3CA928C FAA752E7
r 800.10
*Want  00010203 04050607 08090A0B 0C0D0E0F
* Expected results
*Done

*Testcase KMC fc28
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000014     # LA R0,X'14'       R0->function code 20
r 204=4110f510      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=4100001C     # LA R0,X'1C'       R0->function code 28 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500010     # LA R5,SOL         R5->second operand length
r 220=B92F0024     # KMC R2,R4         Cipher message
r 224=4180f590      #  LA R8,CV          Load address CV
r 228=D20F10008000 # MVC ?             Copy CV parameter value
r 22E=4100009C     # LA R0,X'9C'       R0->function code 28 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500010     # LA R5,FOL         R5->first operand length
r 242=B92F0024     # KMC R2,R4         Cipher message
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=303132333435363738393A3B3C3D3E3F # Parameter block
r 540=404142434445464748494A4B4C4D4E4F # Parameter block
*
r 590=000102030405060708090A0B0C0D0E0F # CV
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=5390628A3ACF964F6E02053976A8035D # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.10
*Want  5390628A 3ACF964F 6E020539 76A8035D
r 800.10
*Want  00010203 04050607 08090A0B 0C0D0E0F
* Expected results
*Done

*Testcase KMC fc67
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000043     # LA R0,X'43'       R0->function code 67
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500008     # LA R5,SOL         R5->second operand length
r 214=B92F0024     # KMC R2,R4         Cipher message with chaining
r 218=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
*
r 600=0001020304050607                 # First operand
*
r 680=871A10BDDECFD665                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 780=928F13E90793F6F408090A0B0C0D0E0F # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  871A10BD DECFD665
r 500.8
r 780.10
*Want  928F13E9 0793F6F4 08090A0B 0C0D0E0F
*Done
