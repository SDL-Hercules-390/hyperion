*Testcase KM fc0
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000000     # LA R0,0           R0->function code 0
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=41200000     # LA R2,FO          R2->first operand
r 20C=41400000     # LA R4,SO          R4->second operand
r 210=41500000     # LA R5,SOL         R5->second operand length
r 214=B92E0024     # KM R2,R4          Cipher message
r 218=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
*
r 580=F0703838000028280000000000000000 # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter block
r 500.10
*Want  F0703838 00002828 00000000 00000000
* Expected result
*Done


*Testcase KM fc1
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000001     # LA R0,X'01'       R0->function code 1 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500008     # LA R5,SOL         R5->second operand length
r 214=B92E0024     # KM R2,R4          Cipher message
r 218=41000081     # LA R0,X'81'       R0->function code 1 decrypt
r 21C=4110f500      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500008     # LA R5,FOL         R5->first operand length
r 22C=B92E0024     # KM R2,R4          Cipher message
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=0001020304050607                 # Parameter block
*
r 600=0001020304050607                 # First operand
*
r 680=E1B246E5A7C74CBC                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  E1B246E5 A7C74CBC
r 800.8
*Want  00010203 04050607
*Done


*Testcase KM fc2
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000002     # LA R0,X'02'       R0->function code 2 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500008     # LA R5,SOL         R5->second operand length
r 214=B92E0024     # KM R2,R4          Cipher message
r 218=41000082     # LA R0,X'82'       R0->function code 2 decrypt
r 21C=4110f500      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500008     # LA R5,FOL         R5->first operand length
r 22C=B92E0024     # KM R2,R4          Cipher message
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
*
r 600=0001020304050607                 # First operand
*
r 680=DF0B6C9C31CD0CE4                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  DF0B6C9C 31CD0CE4
r 800.8
*Want  00010203 04050607
*Done

*Testcase KM fc3
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000003     # LA R0,X'03'       R0->function code 3 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500008     # LA R5,SOL         R5->second operand length
r 214=B92E0024     # KM R2,R4          Cipher message
r 218=41000083     # LA R0,X'83'       R0->function code 3 decrypt
r 21C=4110f500      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500008     # LA R5,FOL         R5->first operand length
r 22C=B92E0024     # KM R2,R4          Cipher message
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=1011121314151617
*
r 600=0001020304050607                 # First operand
*
r 680=58ED248F77F6B19E                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  58ED248F 77F6B19E
r 800.8
*Want  00010203 04050607
*Done


*Testcase KM fc9
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
r 220=B92E0024     # KM R2,R4          Cipher message
r 224=41000089     # LA R0,X'89'       R0->function code 9 decrypt
r 228=4110f500      #  LA R1,PB          R1->parameter block address
r 22C=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 230=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 234=41500008     # LA R5,FOL         R5->first operand length
r 238=B92E0024     # KM R2,R4          Cipher message
r 23C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
*
r 600=0001020304050607                 # First operand
*
r 680=E1B246E5A7C74CBC                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  E1B246E5 A7C74CBC
r 800.8
*Want  00010203 04050607
*Done

*Testcase KM fc10
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
r 220=B92E0024     # KM R2,R4          Cipher message
r 224=4100008A     # LA R0,X'8A'       R0->function code 10 decrypt
r 228=4110f500      #  LA R1,PB          R1->parameter block address
r 22C=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 230=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 234=41500008     # LA R5,FOL         R5->first operand length
r 238=B92E0024     # KM R2,R4          Cipher message
r 23C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=2021222324252627                 # Parameter block
*
r 600=0001020304050607                 # First operand
*
r 680=DF0B6C9C31CD0CE4                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  DF0B6C9C 31CD0CE4
r 800.8
*Want  00010203 04050607
*Done

*Testcase KM fc11
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
r 220=B92E0024     # KM R2,R4          Cipher message
r 224=4100008B     # LA R0,X'8B'       R0->function code 11 decrypt
r 228=4110f500      #  LA R1,PB          R1->parameter block address
r 22C=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 230=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 234=41500008     # LA R5,FOL         R5->first operand length
r 238=B92E0024     # KM R2,R4          Cipher message
r 23C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
*
r 600=0001020304050607                 # First operand
*
r 680=58ED248F77F6B19E                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  58ED248F 77F6B19E
r 800.8
*Want  00010203 04050607
*Done

*Testcase KM fc18
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000012     # LA R0,X'12'       R0->function code 18 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=B92E0024     # KM R2,R4          Cipher message
r 218=41000092     # LA R0,X'92'       R0->function code 18 decrypt
r 21C=4110f500      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500010     # LA R5,FOL         R5->first operand length
r 22C=B92E0024     # KM R2,R4          Cipher message
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=0A940BB5416EF045F1C39458C653EA5A # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  0A940BB5 416EF045
r 800.8
*Want  00010203 04050607
*Done

*Testcase KM fc19
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000013     # LA R0,X'13'       R0->function code 19 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=B92E0024     # KM R2,R4          Cipher message
r 218=41000093     # LA R0,X'93'       R0->function code 19 decrypt
r 21C=4110f500      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500010     # LA R5,FOL         R5->first operand length
r 22C=B92E0024     # KM R2,R4          Cipher message
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=1011121314151617                 # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=0060BFFE46834BB8DA5CF9A61FF220AE # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  0060BFFE 46834BB8
r 800.8
*Want  00010203 04050607
*Done

*Testcase KM fc20
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000014     # LA R0,X'14'       R0->function code 20 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=B92E0024     # KM R2,R4          Cipher message
r 218=41000094     # LA R0,X'94'       R0->function code 20 decrypt
r 21C=4110f500      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500010     # LA R5,FOL         R5->first operand length
r 22C=B92E0024     # KM R2,R4          Cipher message
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=5A6E045708FB7196F02E553D02C3A692 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  5A6E0457 08FB7196
r 800.8
*Want  00010203 04050607
*Done

*Testcase KM fc26
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
r 220=B92E0024     # KM R2,R4          Cipher message
r 224=4100009A     # LA R0,X'9A'       R0->function code 26 decrypt
r 228=4110f500      #  LA R1,PB          R1->parameter block address
r 22C=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 230=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 234=41500010     # LA R5,FOL         R5->first operand length
r 238=B92E0024     # KM R2,R4          Cipher message
r 23C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=0A940BB5416EF045F1C39458C653EA5A # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  0A940BB5 416EF045
r 800.8
*Want  00010203 04050607
*Done

*Testcase KM fc27
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
r 220=B92E0024     # KM R2,R4          Cipher message
r 224=4100009B     # LA R0,X'9B'       R0->function code 27 decrypt
r 228=4110f500      #  LA R1,PB          R1->parameter block address
r 22C=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 230=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 234=41500010     # LA R5,FOL         R5->first operand length
r 238=B92E0024     # KM R2,R4          Cipher message
r 23C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=3031323334353637                 # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=0060BFFE46834BB8DA5CF9A61FF220AE # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  0060BFFE 46834BB8
r 800.8
*Want  00010203 04050607
*Done

*Testcase KM fc28
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000014     # LA R0,X'14'       R0->function code 20
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=4100001C     # LA R0,X'1B'       R0->function code 28 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500010     # LA R5,SOL         R5->second operand length
r 220=B92E0024     # KM R2,R4          Cipher message
r 224=4100009C     # LA R0,X'9B'       R0->function code 28 decrypt
r 228=4110f500      #  LA R1,PB          R1->parameter block address
r 22C=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 230=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 234=41500010     # LA R5,FOL         R5->first operand length
r 238=B92E0024     # KM R2,R4          Cipher message
r 23C=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=303132333435363738393A3B3C3D3E3F # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=5A6E045708FB7196F02E553D02C3A692 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  5A6E0457 08FB7196
r 800.8
*Want  00010203 04050607
*Done

*Testcase KM fc50
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000032     # LA R0,X'32'       R0->function code 50 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=B92E0024     # KM R2,R4          Cipher message
r 218=4180f520      #  LA R8,XTS         Load address XTS
r 21C=D20F10108000 # MVC 10(16,1),0(1):Copy XTS parameter value
r 222=410000B2     # LA R0,X'B2'       R0->function code 50 decrypt
r 226=4110f500      #  LA R1,PB          R1->parameter block address
r 22A=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 22E=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 232=41500010     # LA R5,FOL         R5->first operand length
r 236=B92E0024     # KM R2,R4          Cipher message
r 23A=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
*
r 520=101112131415161718191A1B1C1D1E1F # XTS
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=855E76E1F0FD7889F69BC8190A755686 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  855E76E1 F0FD7889
r 800.8
*Want  00010203 04050607
*Done

*Testcase KM fc52
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000034     # LA R0,X'34'       R0->function code 52 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=B92E0024     # KM R2,R4          Cipher message
r 218=4180f530      #  LA R8,XTS         Load address XTS
r 21C=D20F10208000 # MVC 20(16,1),0(1):Copy XTS parameter value
r 222=410000B4     # LA R0,X'B4'       R0->function code 52 decrypt
r 226=4110f500      #  LA R1,PB          R1->parameter block address
r 22A=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 22E=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 232=41500010     # LA R5,FOL         R5->first operand length
r 236=B92E0024     # KM R2,R4          Cipher message
r 23A=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
*
r 530=202122232425262728292A2B2C2D2E2F # XTS
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
* Incorrect.
r 680=8F2A6717867A9DC4762802F21FB9D355 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=8F2A6717867A9DC4762802F21FB9D355 # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.10
*Want  52F67E0A 657CDCE5 A26ADC57 422AC064
r 800.8
*Want  00010203 04050607
* Expected results
*Done

*Testcase KM fc58
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000012     # LA R0,X'12'       R0->function code 18
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=4100003A     # LA R0,X'3A'       R0->function code 58 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500010     # LA R5,SOL         R5->second operand length
r 220=B92E0024     # KM R2,R4          Cipher message
r 224=4180f540      #  LA R8,XTS         Load address XTS
r 228=D20F10308000 # MVC ?             Copy XTS parameter value
r 22E=410000BA     # LA R0,X'BA'       R0->function code 58 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500010     # LA R5,FOL         R5->first operand length
r 242=B92E0024     # KM R2,R4          Cipher message
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=303132333435363738393A3B3C3D3E3F # Parameter block
*
r 540=303132333435363738393A3B3C3D3E3F # XTS
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=AB84C432BC7AFB5856108804BEF19945 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  AB84C432 BC7AFB58
r 800.8
*Want  00010203 04050607
*Done

*Testcase KM fc60
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000014     # LA R0,X'14'       R0->function code 20
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=4100003C     # LA R0,X'3C'       R0->function code 60 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500010     # LA R5,SOL         R5->second operand length
r 220=B92E0024     # KM R2,R4          Cipher message
r 224=4180f550      #  LA R8,XTS         Load address XTS
r 228=D20F10408000 # MVC ?             Copy XTS parameter value
r 22E=410000BC     # LA R0,X'BC'       R0->function code 60 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500010     # LA R5,FOL         R5->first operand length
r 242=B92E0024     # KM R2,R4          Cipher message
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=303132333435363738393A3B3C3D3E3F # Parameter block
r 540=404142434445464748494A4B4C4D4E4F # Parameter block
*
r 550=404142434445464748494A4B4C4D4E4F # XTS
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=D18DB5C14D043A79A2D7D11ACDAE90F0 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  D18DB5C1 4D043A79
r 800.8
*Want  00010203 04050607
*Done
