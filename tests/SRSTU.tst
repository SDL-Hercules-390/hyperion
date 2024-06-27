*Testcase SRSTU: Simple Search String Unicode test
sysclear
archlvl z/Arch

r 1a0=00000001800000000000000000000200  # z/Arch restart PSW
r 1d0=0002000180000000000000000000DEAD  # z/Arch pgm new PSW

r 200=41000fff                          # GR0 = character to search for
r 204=41200300                          # R2 --> string to be searched
r 208=41100401                          # R1 --> end of string
r 20c=B9BE0012                          # SRSTU
r 210=4710020c                          # CC3: keep searching...
r 214=B2B20218                          # LPSWE GOODPSW
r 218=00020001800000000000000000000000  # GOODPSW

runtest .1
*Done
