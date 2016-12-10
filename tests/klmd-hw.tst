*Testcase KLMD fc0
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000000     # LA R0,0           R0->function code 0
r 204=4110f500      #  LA R1,PB          R1->parameter block
r 208=41200000     # LA R2,SO          R2->second operand address
r 20C=41300000     # LA R3,SOL         R3->second operand length
r 210=B93F0002     # KLMD R0,R2        Compute last message digest
r 214=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=000102030405060708090A0B0C0D0E0F # Parameter block
*
r 580=F0000000000000000000000000000000 # Expected result
*
ostailor null
runtest .1
*Compare
* Display parameter block
r 500.10
*Want  F0000000 00000000 00000000 00000000
*Done


*Testcase KLMD fc1
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000001     # LA R0,1           R0->function code 1
r 204=4110f500      #  LA R1,PB          R1->parameter block
r 208=4120f600      #  LA R2,SO          R2->second operand address
r 20C=41300040     # LA R3,SOL         R3->second operand length
r 210=B93F0002     # KLMD R0,R2        Compute last message digest
r 214=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=67452301EFCDAB8998BADCFE10325476 # Parameter block
r 510=C3D2E1F00000000000000040         # Parameter block
*
r 580=C5592921382388B41AE53FA27A726F8B # Expected result
r 590=64D9FFA2000000000000004000000000 # Expected result
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
*Want  C5592921 382388B4 1AE53FA2 7A726F8B
r 510.4
*Want  64D9FFA2
*Done


*Testcase KLMD fc2
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000002     # LA R0,2           R0->function code 2
r 204=4110f500      #  LA R1,PB          R1->parameter block
r 208=4120f600      #  LA R2,SO          R2->second operand address
r 20C=41300040     # LA R3,SOL         R3->second operand length
r 210=B93F0002     # KLMD R0,R2        Compute last message digest
r 214=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=6A09E667BB67AE853C6EF372A54FF53A # Parameter block
r 510=510E527F9B05688C1F83D9AB5BE0CD19 # Parameter block
r 520=0000000000000040                 # Parameter block
*
r 580=0DD42DCAAAEF32477F5DA8AD90F472AB # Expected result
r 590=98F3BE9FDA5352A3E4317A7D8BA70CA0 # Expected result
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
*Want  0DD42DCA AAEF3247 7F5DA8AD 90F472AB
r 510.4
*Want  98F3BE9F
*Done


*Testcase KLMD fc3
sysclear
archmode z
r 1A0=00000001800000000000000000000200 # z/Arch restart PSW
r 1D0=0002000180000000000000000000DEAD # z/Arch pgm new PSW
r 200=41000003     # LA R0,3           R0->function code 3
r 204=4110f500      #  LA R1,PB          R1->parameter block address
r 208=4120f600      #  LA R2,SO          R2->second operand
r 20C=41300080     # LA R3,SOL         R3->second operand length
r 210=B93F0002     # KLMD R0,R2        Compute last message digest
r 214=12ee077eB2B20300      #  LPSWE WAITPSW     Load enabled wait PSW
r 300=00020001800000000000000000000000 #  WAITPSW Enabled wait state PSW
*
r 500=6A09E667F3BCC908BB67AE8584CAA73B # Parameter block
r 510=3C6EF372FE94F82BA54FF53A5F1D36F1 # Parameter block
r 520=510E527FADE682D19B05688C2B3E6C1F # Parameter block
r 530=1F83D9ABFB41BD6B5BE0CD19137E2179 # Parameter block
r 540=00000000000000000000000000000080 # Parameter block
*
r 580=AEAC2C1F2D9463A0565528042B36621F # Expected result
r 590=17CD05332E019406E56E04B59FDC643B # Expected result
r 5A0=B0439B9E1D856C288AE473347C4A917E # Expected result
r 5B0=5DBBF041718F75102BF1CB11BD38A894 # Expected result
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
*Want  AEAC2C1F 2D9463A0 56552804 2B36621F
r 510.10
*Want  17CD0533 2E019406 E56E04B5 9FDC643B
r 520.10
*Want  B0439B9E 1D856C28 8AE47334 7C4A917E
r 530.10
*Want  5DBBF041 718F7510 2BF1CB11 BD38A894
*Done
