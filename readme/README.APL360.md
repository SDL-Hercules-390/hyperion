![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# APL\360 mods by Max H. Parke
## Contents

1. [About](#About)

# About

Max H. Parke (ikj1234i@yahoo.com) has modified the commadpt driver to work for APL\360.  This is Max's README regarding his APL\360 mods:

1. Here is the APL conf that I used:
```
1750          APLDEV (X'00',X'03'),TYPE=AMBIG,SAD=NOP
1800          APLDEV (X'40',X'43'),TYPE=1052
1850          APLDEV (X'80',X'83'),TYPE=TS41,SAD=NOP
1900          APLDEV (X'C0',X'C3'),TYPE=1052
```

With `TYPE=AMBIG` your telnet connection should close after using `)OFF`:
```
)off
002  23.55.39 12/03/84 u01
connected    0.01.05  to date    0.23.08
cpu time     0.00.00  to date    0.00.00
Connection closed by foreign host.
```

2. Hercules configuration file device definitions:
```
   # [standard 2741 emulation]
   0400  2703  dial=in  lport=57411  lnctl=ibm1  term=2741  skip=5EDE  code=ebcd  iskip=0A

   # [extended APL support when using rxvt4apl]
   0402  2703  dial=in  lport=57413  lnctl=ibm1  term=rxvt4apl  skip=5EDE  code=ebcd  iskip=0D0A  prepend=16  append=5B1F  eol=0A  binary=yes  crlf=yes  sendcr=yes
```

3. Logon using "314159" (do not use = as in 1052).  For `rxvt4apl`, use the regular right parenthesis.  See also the separate [readme](./README.RXVT4APL.md) file for using `rxvt4apl`.

4. Use `CTRL-C` as usual for 2741 ATTN.

5. Only tested in Linux - YMMV

6. **TODO:** add fix for disconnected TCP connection while signed on to APL, should initiate a session drop.
