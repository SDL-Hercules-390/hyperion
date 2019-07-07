![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](/README.md)

# Hercules Input/Output Architecture
## Contents
1. [About](#About)
2. [Motivation for I/O Architecture Enhancements](#Motivation-for-IO-Architecture-Enhancements)
3. [Architecture Feasibility](#Architecture-Feasibility)
4. [Channel Input/Output Architecture in ESA/390 or z/Architecture](#Channel-InputOutput-Architecture-in-ESA390-or-zArchitecture)
5. [Enabling Channel-based I/O in ESA/390 or z/Architecture](#Enabling-Channel-based-IO-in-ESA390-or-zArchitecture)

## About
Hercules is composed of two related but distinct architectures:

  - the Central Processing Unit architecture and
  - the Input/Output architecture.

The two architectures intersect and influence each other during input/output operations and when managing the input/output system.  This intersection becomes visible to the programmer in four ways:

   - available input/output related instructions,
   - input/output related interruptions,
   - allocation of control register uses for input/output,
   - structures utilized by the input/output system.

Traditionally the input/output architecture has been treated as a subset of the CPU architecture.  System/370 uses channels.  ESA/390 and z/Architecture use subchannels.  Each has its related CPU instructions.  Additionally, the input/output architecture is cast in concrete with the CPU architecture when the Hercules system is built.

## Motivation for I/O Architecture Enhancements
Many users of Hercules are constrained by the System/370 architecture supported by the readily available legacy operating systems.  These constraints fall into two categories: not enough memory and no modern CPU instructions.  The most straight forward albeit burdensome answer would be to migrate the legacy operating system to one of the modern architectures.  One of the major hurdles seen by the Hercules community for this approach is the input/output architecture change that occurred with System/370-XA when the subchannel-based input/output architecture was introduced.  The Hercules developers' answer to this hurdle is to make the System/370 channel-based input/output architecture available to ESA/390 and z/Architecture CPU architectures.  This approach does not eliminate the need for operating system adoption of a more modern CPU architecture, but reduces the effort.  Other approaches can be taken to solving these operating system hurdles.  Nevertheless, without a Hercules supported approach, the operating system changes will never occur, lacking a platform on which they could be used.

## Architecture Feasibility
In the System/370 era before the release of 370-XA, the two input/output architectures were completely capable of co-existing on a single system.  This would in fact likely be necessary as the System/370 operating systems were being modified to support 370-XA.

With the advent of Access Register addressing mode in ESA/390, the System/370 control register previouly used for channel masks, control register 2, was repurposed, having no longer been used for years, for access register mode. The only decision today for implementation of this "forward-porting" strategy became how to accomodate in ESA/390 or z/Architecture channel masks.

Polling of the Hercules community revealed that channel masks were only disabled when hardware problems made it necessary to do so.  Under Hercules software emulation such problems can not occur.  The Hercules developers concluded that channel masks could be safely eliminated when the CPU architecture requires control register 2 for access register mode.  This is the normal case with a default build of Hercules that includes ESA/390 or z/Architecture.  A non-default build where access register mode is not enabled, for example a build targeted for ESA/370 instead of ESA/390, control register 2 would still be used for channel masks.

## Channel Input/Output Architecture in ESA/390 or z/Architecture
The input/ouput architecture continues to be fixed to the CPU architecture during the Hercules build process.  Coexistence of both input/output architectures is not possible at this time, although the subchannel-based input/output architecture standard with ESA/390 or z/Architecture can now be replaced with the channel-based input/output architecture expected in System/370.

When either ESA/390 or z/Architecture or both are built with the channel-based input/output architecture, the following changes occur:

   - I/O instructions available: CLRIO, HIO, HDV, SIO, SIOF, STIDC, TCH, TIO
   - If separate enabled, I/O instructions available: CONCS, DISCS
   - Only Format 0 CCW's are enabled, restricting I/O data areas to addresses
     0x0-0xFFFFFF.
   - Real storage addresses 0x40-0x47 used for storing of the Channel Status
     Word on an interruption.
   - Real storage addresses 0x48-0x4B used for identifying the location of the
     first Format 0 CCW of the I/O operation. This is the Channel Address Word.
   - Real storage addresses 0xB8-0xB9 used for storing zeros on interruptions.
     From the perspective of ESA/390 or z/Architecture this has the appearance
     of a zero being stored instead of a 1 in bit 15 of the Subsystem-
     Identification Word in these same real storage location.
   - Real storage addresses 0xBA-0xBB used for storing of the I/O device address
     on interruptions instead of the subchannel address.
   - Program interrupt operation exceptions occur for instructions: CSCH, HSCH,
     MSCH, RCHP, RSCH, SAL, SCHM, SSCH, STCPS, STRCW
   - No machine-check interruptions occur for I/O devices related to CRW's.
   - I/O related SIE interceptions that occur under System/370 will occur under
     ESA/390 or z/Architecture.

## Enabling Channel-based I/O in ESA/390 or z/Architecture
Identical changes are made to enable the channel-based I/O architecture and disable the subchannel-based I/O architecture under ESA/390 or z/Architecture. The only difference are the Hercules source files in which the changes are made.  The changes described below should be made in

   - feat390.h for ESA/390 and/or
   - feat900.h for z/Architecture.

In either or both of the above files, change from:

```
#define FEATURE_CANCEL_IO_FACILITY /* comment if FEATURE_S370_CHANNEL used */
#define FEATURE_CHANNEL_SUBSYSTEM  /* comment if FEATURE_S370_CHANNEL used */
// #define FEATURE_CHANNEL_SWITCHING  /* comment if FEATURE_CHANNEL_SUBSYSTEM used */
// #define FEATURE_S370_CHANNEL  /* comment if FEATURE_CHANNEL_SUBSYSTEM used */
```

to:

```
// #define FEATURE_CANCEL_IO_FACILITY /* comment if FEATURE_S370_CHANNEL used */
// #define FEATURE_CHANNEL_SUBSYSTEM  /* comment if FEATURE_S370_CHANNEL used */
#define FEATURE_CHANNEL_SWITCHING  /* comment if FEATURE_CHANNEL_SUBSYSTEM used */
#define FEATURE_S370_CHANNEL  /* comment if FEATURE_CHANNEL_SUBSYSTEM used */
```

The features are found in collating sequence in the source file and are not adjacent as identified above.

Following these change(s), locally build the Hercules system.  Errors will be reported if the wrong combination of features are enabled during the build process.

Harold Grovesteen
