*Testcase KIMD fc0
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000000     # LA R0,0           R0->function code 0
r 204=4110f500      #  LA R1,PB          R1->parameter block
r 208=41200000     # LA R2,SO          R2->second operand address
r 20C=41300000     # LA R3,SOL         R3->second operand length
r 210=B93E0002     # KIMD R0,R2        Compute intermediate message digest
r 214=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
*
r 580=F0000000000000004000000000000000 # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter block
r 500.10
*Want  F0000000 00000000 40000000 00000000
*Done

*Testcase KIMD fc1
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000001     # LA R0,1           R0->function code 1
r 204=4110f500      #  LA R1,PB          R1->parameter block
r 208=4120f600      #  LA R2,SO          R2->second operand address
r 20C=41300040     # LA R3,SOL         R3->second operand length
r 210=B93E0002     # KIMD R0,R2        Compute intermediate message digest
r 214=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=67452301EFCDAB8998BADCFE10325476 # Parameter block
r 510=C3D2E1F0                         # Parameter block
*
r 580=B9AC757BBC2979252E22727406872F94 # Expected result
r 590=CBEA56A1000000000000000000000000 # Expected result
*
r 600=000102030405060708090A0B0C0D0E0F # Second operand
r 610=101112131415161718191A1B1C1D1E1F # Second operand
r 620=202122232425262728292A2B2C2D2E2F # Second operand
r 630=303132333435363738393A3B3C3D3E3F # Second operand
*
ostailor null
runtest .1
*Compare
* Display parameter block
r 500.10
*Want  B9AC757B BC297925 2E227274 06872F94
r 510.4
*Want  CBEA56A1
*Done


*Testcase KIMD fc2
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000002     # LA R0,2           R0->function code 2
r 204=4110f500      #  LA R1,PB          R1->parameter block
r 208=4120f600      #  LA R2,SO          R2->second operand address
r 20C=41300040     # LA R3,SOL         R3->second operand length
r 210=B93E0002     # KIMD R0,R2        Compute intermediate message digest
r 214=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=6A09E667BB67AE853C6EF372A54FF53A # Parameter block
r 510=510E527F9B05688C1F83D9AB5BE0CD19 # Parameter block
*
r 580=FC99A2DF88F42A7A7BB9D18033CDC6A2 # Expected result
r 590=0256755F9D5B9A5044A9CC315ABE84A7 # Expected result
*
r 600=000102030405060708090A0B0C0D0E0F # Second operand
r 610=101112131415161718191A1B1C1D1E1F # Second operand
r 620=202122232425262728292A2B2C2D2E2F # Second operand
r 630=303132333435363738393A3B3C3D3E3F # Second operand
*
ostailor null
runtest .1
*Compare
* Display parameter block
r 500.10
*Want  FC99A2DF 88F42A7A 7BB9D180 33CDC6A2
r 510.10
*Want  0256755F 9D5B9A50 44A9CC31 5ABE84A7
*Done


*Testcase KIMD fc3
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000003     # LA R0,3           R0->function code 3
r 204=4110f500      #  LA R1,PB          R1->parameter block
r 208=4120f600      #  LA R2,SO          R2->second operand address
r 20C=41300080     # LA R3,SOL         R3->second operand length
r 210=B93E0002     # KIMD R0,R2        Compute intermediate message digest
r 214=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=6A09E667F3BCC908BB67AE8584CAA73B # Parameter block
r 510=3C6EF372FE94F82BA54FF53A5F1D36F1 # Parameter block
r 520=510E527FADE682D19B05688C2B3E6C1F # Parameter block
r 530=1F83D9ABFB41BD6B5BE0CD19137E2179 # Parameter block
*
r 580=8E03953CD57CD6879321270AFA70C582 # Expected result
r 590=7BB5B69BE59A8F0130147E94F2AEDF7B # Expected result
r 5A0=DC01C56C92343CA8BD837BB7F0208F5A # Expected result
r 5B0=23E155694516B6F147099D491A30B151 # Expected result
*
r 600=000102030405060708090A0B0C0D0E0F # Second operand
r 610=101112131415161718191A1B1C1D1E1F # Second operand
r 620=202122232425262728292A2B2C2D2E2F # Second operand
r 630=303132333435363738393A3B3C3D3E3F # Second operand
r 640=404142434445464748494A4B4C4D4E4F # Second operand
r 650=505152535455565758595A5B5C5D5E5F # Second operand
r 660=606162636465666768696A6B6C6D6E6F # Second oparand
r 670=707172737475767778797A7B7C7D7E7F # Second operand
*
ostailor null
runtest .1
*Compare
* Display parameter block
r 500.10
*Want  8E03953C D57CD687 9321270A FA70C582
r 510.10
*Want  7BB5B69B E59A8F01 30147E94 F2AEDF7B
r 520.10
*Want  DC01C56C 92343CA8 BD837BB7 F0208F5A
r 530.10
*Want  23E15569 4516B6F1 47099D49 1A30B151
*Done


*Testcase KIMD fc65
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000041     # LA R0,65          R0->function code 65
r 204=4110f500      #  LA R1,PB          R1->parameter block
r 208=4120f600      #  LA R2,SO          R2->second operand address
r 20C=41300040     # LA R3,SOL         R3->second operand length
r 210=B93E0002     # KIMD R0,R2        Compute intermediate message digest
r 214=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=6A09E667BB67AE853C6EF372A54FF53A # Parameter block
r 510=510E527F9B05688C1F83D9AB5BE0CD19 # Parameter block
*
r 580=3835EC6F7151E73A2A593988B05B7E61 # Expected result
*
r 600=000102030405060708090A0B0C0D0E0F # Second operand
r 610=101112131415161718191A1B1C1D1E1F # Second operand
r 620=202122232425262728292A2B2C2D2E2F # Second operand
r 630=303132333435363738393A3B3C3D3E3F # Second operand
*
ostailor null
runtest .1
*Compare
* Display parameter block
r 500.10
*Want  3835EC6F 7151E73A 2A593988 B05B7E61
*Done
