![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Guest Access To Host IP Stack - The TCPIP (X'75') Instruction
## Contents
1. [Introduction](#Introduction)
2. [CAUTION](#CAUTION)
3. [Usage](#Usage)
4. [Copyright](#Copyright)

## Introduction
Around 2002 Jason Winter developed a Hercules extension allowing the guest OS to access the IP stack of the host OS by means of a special instruction that does not exist on real S/3x0, ESA/390 or z/Architecture hardware.  The instruction is in RX format, and its operation code is X'75':
```
                   75rxbddd [RX-a]    TCPIP  r1,ddd(x2,b2)

                                r1:  0-15
                                x2:  0,5-13
                                b2:  5-13
```

As opposed to "classic" instructions, the TCPIP (X'75') instruction preserves a state between calls, an essential requirement to transfer IP traffic back and forth between the host and the guest.  Due to this fact, setting up for native execution of the instruction is complex.

To avoid having application programmers go through this complexity, at the time of this writing two higher-level interfaces are available, allowing easy and standardized access to the functionality provided by the TCPIP instruction:


- A socket API in Jason's JCC-Library, which provides the well-known C socket programming functions.  See the JCC-Library help in the JCC distribution (found in `TK4-.JASON.JCC.ZIP` on TK4- systems) for a description of the functions available.


- TCP/IP for MVS 3.8 Assembler by Shelby Beach, a programming interface that is source code compatible to IBM's well-known EZASMI API.  Comprehensive documentation of all available functions is part of the installer found at `TK4-.SHELBY.EZASMI.V100.ZIP` on TK4- systems.


## CAUTION

The TCPIP instruction gives a guest program the very same access to the host's IP stack that the host process running the Hercules instance has.  Allowing every guest program this type of access may be undesirable, particularly when the Hercules process runs with elevated privileges.

In addition, one guest program can easily interfere with the IP communications of any other guest program by hijacking the other program's socket identifiers.

In sum, a malicious user with access to the TCPIP instruction could severely compromise the guest's _and the host's_ integrity.  A problem state instruction with this type of capability clearly does not comply with the specifications of the architectures emulated by Hercules.

To allow users to overcome these weaknesses of the TCPIP instruction, its implementation on SDL Hyperion comes in two flavors: As a supervisor state instruction and as a problem state instruction, both of which are functionally identical. Enabling the TCPIP instruction will default to the supervisor state instruction.
That way, only programs running in supervisor state can execute the instruction, which is considered being compliant with the emulated architectures.

Note that both TCPIP APIs currently existing for S/370 operating systems (C socket library and EZASMI) rely on the instruction being available in problem state, which is not architecturally compliant.  _**Using the problem state flavor of the instruction is at the Hercules user's sole risk.**_

Note also, that currently no software exists that would be able to make use of the supervisor state flavor of the TCPIP instruction in a controlled manner.

## Usage

The availability and flavor of the instruction is controlled by three elements:

1.  The compile time FEATURE_TCPIP_EXTENSION definition: To allow enabling of the TCPIP instruction, the featxxx.h header files corresponding to the relevant architectures (feat370.h, feat390.h, feat900.h) must contain the statement:
```
                #define  FEATURE_TCPIP_EXTENSION
```
As shipped, SDL Hyperion 4.2 has this feature defined _only_ for the S/370 architecture (feat370.h).

2.  By default, the TCPIP instruction is disabled at run time, regardless of whether the compile time FEATURE_TCPIP_EXTENSION was defined or not.  If FEATURE_TCPIP_EXTENSION was defined for the architecture currently being used, the supervisor state flavor of the TCPIP instruction can only be enabled by issuing the command:
```
                facility enable HERC_TCPIP_EXTENSION
```
before IPLing the guest operating system.


3.  If the problem state variation of the TCPIP instruction is desired, then after having enabled the supervisor state flavor as described above, the command:
```
                facility enable HERC_TCPIP_PROB_STATE
```
must then _also_ be issued before IPLing the operating system.


## Copyright

Refer to the opening comments in the "x75.c" and "tcpip.c" source file members for copyright information.
