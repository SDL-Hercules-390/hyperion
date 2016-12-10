*Testcase KMO fc0
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000000     # LA R0,0           R0->function code 0
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=41200000     # LA R2,FO          R2->first operand
r 20C=41400000     # LA R4,SO          R4->second operand
r 210=41500000     # LA R5,SOL         R5->second operand length
r 214=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 218=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
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
* Expected result
*Done

*Testcase KMO bad
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000000     # LA R0,x'7f'       R0->function code 0
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=41200000     # LA R2,FO          R2->first operand
r 20C=41400000     # LA R4,SO          R4->second operand
r 210=41500000     # LA R5,SOL         R5->second operand length
r 214=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 218=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
*
r 580=F0703838000000000000000000000000 # Expected result
*
runtest .1
*Program 6
*Compare
*Done

*Testcase KMO fc1
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=5800f400      #  LA R0,X'01'       R0->function code 1 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500008     # LA R5,SOL         R5->second operand length
r 214=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 218=5800f408      #  LA R0,X'81'       R0->function code 1 decrypt
r 21C=4110f580      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500008     # LA R5,FOL         R5->first operand length
r 22C=B92B0024     # KMF R2,R4         Cipher message with output feedback
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 400=04000001
r 408=04000081
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 580=000102030405060708090A0B0C0D0E0F # Parameter block
*
r 600=0001020304050607                 # First operand
*
r 680=A5CF9BBA5A097E3E                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 800.8
*Want  00010203 04050607
r 600.8
*Want  A5CF9BBA 5A097E3E
*Done

*Testcase KMO fc2
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=5800f400      #  LA R0,X'01'       R0->function code 1 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500008     # LA R5,SOL         R5->second operand length
r 214=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 218=5800f408      #  LA R0,X'81'       R0->function code 1 decrypt
r 21C=4110f580      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500008     # LA R5,FOL         R5->first operand length
r 22C=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 400=04000002
r 408=04000082
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=1011121314151617                 # Parameter block
r 580=000102030405060708090A0B0C0D0E0F # Parameter block
r 590=1011121314151617                 # Parameter block
*
r 600=0001020304050607                 # First operand
*
r 680=B4C4A92FD422AB8E                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 800.8
*Want  00010203 04050607
r 600.8
*Want  B4C4A92F D422AB8E
*Done

*Testcase KMO fc3
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=5800f400      #  LA R0,X'01'       R0->function code 1 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500008     # LA R5,SOL         R5->second operand length
r 214=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 218=5800f408      #  LA R0,X'81'       R0->function code 1 decrypt
r 21C=4110f580      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500008     # LA R5,FOL         R5->first operand length
r 22C=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 400=04000003
r 408=04000083
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 580=000102030405060708090A0B0C0D0E0F # Parameter block
r 590=101112131415161718191A1B1C1D1E1F # Parameter block
*
r 600=0001020304050607                 # First operand
*
r 680=C8A06C9E3CD315ED                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  C8A06C9E FF17D58A
r 800.8
*Want  00010203 04050607
*Done

*Testcase KMO fc9
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000001     # LA R0,X'01'       R0->function code 1
r 204=4110f508      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=5800f400      #  LA R0,X'09'       R0->function code 9 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500008     # LA R5,SOL         R5->second operand length
r 220=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 224=4180f590      #  LA R8,CV          Load address CV
r 228=D20710008000 # MVC ?             Copy CV parameter value
r 22E=5800f408      #  LA R0,X'89'       R0->function code 9 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500008     # LA R5,FOL         R5->first operand length
r 242=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 400=04000009
r 408=04000089
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=2021222324252627
*
r 590=0001020304050607                 # CV
*
r 600=0001020304050607                 # First operand
*
r 680=A5CF9BBA5A097E3E                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  A5CF9BBA 5A097E3E
r 680.8
* Expected results
r 800.8
*Want  00010203 04050607
*Done

*Testcase KMO fc10
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000002     # LA R0,X'02'       R0->function code 2
r 204=4110f508      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=5800f400      #  LA R0,X'0A'       R0->function code 10 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500008     # LA R5,SOL         R5->second operand length
r 220=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 224=4180f590      #  LA R8,CV          Load address CV
r 228=D20710008000 # MVC ?             Copy CV parameter value
r 22E=5800f408      #  LA R0,X'8A'       R0->function code 10 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500008     # LA R5,FOL         R5->first operand length
r 242=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 400=0400000A
r 408=0400008A
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
*
r 590=0001020304050607                 # CV
*
r 600=0001020304050607                 # First operand
*
r 680=B4C4A92FD422AB8E                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  B4C4A92F D422AB8E
* Expected results
r 800.8
*Want  00010203 04050607
*Done

*Testcase KMO fc11
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000003     # LA R0,X'03'       R0->function code 3
r 204=4110f508      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=5800f400      #  LA R0,X'0B'       R0->function code 11 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500008     # LA R5,SOL         R5->second operand length
r 220=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 224=4180f590      #  LA R8,CV          Load address CV
r 228=D20710008000 # MVC ?             Copy CV parameter value
r 22E=5800f408      #  LA R0,X'8B'       R0->function code 11 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500008     # LA R5,FOL         R5->first operand length
r 242=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 400=0400000B
r 408=0400008B
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=3031323334353637                 # Parameter block
*
r 590=0001020304050607                 # CV
*
r 600=0001020304050607                 # First operand
*
r 680=C8A06C9EFF17D58A                 # Expected result
*
r 700=0001020304050607                 # Second operand
*
r 800=FFFFFFFFFFFFFFFF                 # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  C8A06C9E FF17D58A
* Expected results
r 800.8
*Want  00010203 04050607
*Done

*Testcase KMO fc18
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=5800f400      #  LA R0,X'12'       R0->function code 18 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 218=5800f408      #  LA R0,X'92'       R0->function code 18 decrypt
r 21C=4110f580      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500010     # LA R5,FOL         R5->first operand length
r 22C=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 400=04000012
r 408=04000092
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 580=000102030405060708090A0B0C0D0E0F # Parameter block
r 590=101112131415161718191A1B1C1D1E1F # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=9C55D7727429FC080BFA681E6B66A577 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  9C55D772 7429FC08
* Expected results
r 800.8
*Want  00010203 04050607
*Done

*Testcase KMO fc19
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=5800f400      #  LA R0,X'13'       R0->function code 19 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 218=5800f408      #  LA R0,X'93'       R0->function code 19 decrypt
r 21C=4110f580      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500010     # LA R5,FOL         R5->first operand length
r 22C=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 400=04000013
r 408=04000093
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=2021222324252627                 # Parameter block
r 580=000102030405060708090A0B0C0D0E0F # Parameter block
r 590=101112131415161718191A1B1C1D1E1F # Parameter block
r 5A0=2021222324252627                 # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=48961CEC4CCD3E5423DE3090DB370B13 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  48961CEC 4CCD3E54
* Expected results
r 800.8
*Want  00010203 04050607
*Done

*Testcase KMO fc20
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=5800f400      #  LA R0,X'14'       R0->function code 20 encrypt
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,FO          R2->first operand
r 20C=4140f700      #  LA R4,SO          R4->second operand
r 210=41500010     # LA R5,SOL         R5->second operand length
r 214=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 218=5800f408      #  LA R0,X'94'       R0->function code 20 decrypt
r 21C=4110f580      #  LA R1,PB          R1->parameter block address
r 220=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 224=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 228=41500010     # LA R5,FOL         R5->first operand length
r 22C=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 230=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 400=04000014
r 408=04000094
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 580=000102030405060708090A0B0C0D0E0F # Parameter block
r 590=101112131415161718191A1B1C1D1E1F # Parameter block
r 5A0=202122232425262728292A2B2C2D2E2F # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=70D334B0D23718578129F069018E3C84 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  70D334B0 D2371857
* Expected results
r 800.8
*Want  00010203 04050607
*Done

*Testcase KMO fc26
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000012     # LA R0,X'12'       R0->function code 18
r 204=4110f510      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=5800f400      #  LA R0,X'1A'       R0->function code 26 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500010     # LA R5,SOL         R5->second operand length
r 220=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 224=4180f590      #  LA R8,CV          Load address CV
r 228=D20F10008000 # MVC ?             Copy CV parameter value
r 22E=5800f408      #  LA R0,X'9A'       R0->function code 26 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500010     # LA R5,FOL         R5->first operand length
r 242=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 400=0400001A
r 408=0400009A
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=303132333435363738393A3B3C3D3E3F # Parameter block
*
r 590=000102030405060708090A0B0C0D0E0F # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=9C55D7727429FC080BFA681E6B66A577 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  9C55D772 7429FC08
* Expected results
r 800.8
*Want  00010203 04050607
*Done

*Testcase KMO fc27
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000013     # LA R0,X'13'       R0->function code 19
r 204=4110f510      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=5800f400      #  LA R0,X'1B'       R0->function code 27 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500010     # LA R5,SOL         R5->second operand length
r 220=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 224=4180f590      #  LA R8,CV          Load address CV
r 228=D20F10008000 # MVC ?             Copy CV parameter value
r 22E=5800f408      #  LA R0,X'9B'       R0->function code 27 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500010     # LA R5,FOL         R5->first operand length
r 242=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 400=0400001B
r 408=0400009B
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=303132333435363738393A3B3C3D3E3F # Parameter block
r 540=404142434445464748494A4B4C4D4E4F # Parameter block
*
r 590=000102030405060708090A0B0C0D0E0F # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=48961CEC4CCD3E5423DE3090DB370B13 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  48961CEC 4CCD3E54
* Expected results
r 800.8
*Want  00010203 04050607
*Done

*Testcase KMO fc28
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000014     # LA R0,X'14'       R0->function code 20
r 204=4110f510      #  LA R1,PB          R1->parameter block address
r 208=B9280000     # PCKMO             Encrypt DEA Key
r 20C=5800f400      #  LA R0,X'1C'       R0->function code 28 encrypt
r 210=4110f500      #  LA R1,PB          R1->parameter block address
r 214=4120f600      #  LA R2,FO          R2->first operand
r 218=4140f700      #  LA R4,SO          R4->second operand
r 21C=41500010     # LA R5,SOL         R5->second operand length
r 220=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 224=4180f590      #  LA R8,CV          Load address CV
r 228=D20F10008000 # MVC ?             Copy CV parameter value
r 22E=5800f408      #  LA R0,X'9C'       R0->function code 28 decrypt
r 232=4110f500      #  LA R1,PB          R1->parameter block address
r 236=4120f800      #  LA R2,SO          R2->second operand from encrypt operation
r 23A=4140f600      #  LA R4,FO          R4->first operand from encrypt operation
r 23E=41500010     # LA R5,FOL         R5->first operand length
r 242=B92B0024     # KMO R2,R4         Cipher message with output feedback
r 246=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 400=0400001C
r 408=0400009C
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
r 510=101112131415161718191A1B1C1D1E1F # Parameter block
r 520=202122232425262728292A2B2C2D2E2F # Parameter block
r 530=303132333435363738393A3B3C3D3E3F # Parameter block
r 540=404142434445464748494A4B4C4D4E4F # Parameter block
r 550=5051525354555657
*
r 590=000102030405060708090A0B0C0D0E0F # Parameter block
*
r 600=000102030405060708090A0B0C0D0E0F # First operand
*
r 680=70D334B0D23718578129F069018E3C84 # Expected result
*
r 700=000102030405060708090A0B0C0D0E0F # Second operand
*
r 800=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF # Expected result
*
runtest .1
*Compare
* Display parameter blocks
r 600.8
*Want  70D334B0 D2371857
* Expected results
r 800.8
*Want  00010203 04050607
*Done
