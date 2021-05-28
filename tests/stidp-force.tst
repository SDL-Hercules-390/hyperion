*Testcase   cpuverid FORCE (z/Arch LPAR FMT-0)

cpumodel    4444
cpuserial   666666

sysclear
archlvl     z/Arch
cpuverid    C8 FORCE
lparnum     9
cpuidfmt    0


r 1a0=00000001800000000000000000000200  # z/Arch  restart PSW
r 1d0=0002000180000000000000000000dead  # z/Arch  pgm new PSW

r 200=b2020208                          # Store CPU ID
r 204=b2b20300                          # LPSWE DONEPSW

r 208=ffffffffffffffff                  # CPU ID
r 300=00020001800000000000000000000000  # z/Arch  DONEPSW

qcpuid

runtest     0.1

*Compare

r 208.8

*Want C8096666 44440000

*Done
