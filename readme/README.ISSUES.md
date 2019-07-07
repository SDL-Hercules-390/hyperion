![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](/README.md)

# Unresolved Issues
## Contents
1. [About](#About)
2. [Tape Device Handler](#Tape-Device-Handler)
3. [I/O Subsystem](#IO-Subsystem)
3. [Device Handlers](#Device-Handlers)
3. [Hardware Panel](#Hardware-Panel)


## About
For a current list of known issues refer to the [GitHub Issues web page](https://github.com/issues?q=is%3Aopen+is%3Aissue+user%3ASDL-Hercules-390+).

## XXX - Marks the functional subsystem affected

## Tape Device Handler
### CE+DE on tape rewind-unload:
Technically, they should be presented separately to the guest o/s:
CE upon completion the actual rewind-unload CCW and then DE once the tape actually finishes unloading, but doing so is somewhat problematic especially for SCSI tapes. (Might be possible to do for AWS/HET tapes but I haven't looked into it yet).
#### Addendum (Ivan 2004/10/02)
Final DE on Rewind Unload must be accompanied by UC(Unit Check) (all D/Ts) and possibly CUE(Control Unit End) for 3420 (3803 CU Type, to indicate an end of Control Unit Busy condition). It is possible the CUE should only occur if a BUSY was presented at some point, but I am not 100% sure about that (and that's not supported either - as we don't support shared CUs). However, VM/370 for example insists that CUE be presented. On the same line, VM/370 DDR is failing on Rewind unload. This could be part of the above problem.
### AWS handler fails to read FLEX Generated AWS files (Ivan 2004/10/02)
The current AWS handler is not capable of processing multi-segment blocks. However, this can be bypassed by forcing the use of the HET handler which is backward compatible.

## I/O Subsystem
### Incorrect Channel program check reporting (Ivan 2004/10/02)
Because of the current I/O design, where we prefetch all write data prior to issuing the I/O, program check is presented unconditionally (except for immediate commands which are declared as such by the device handlers) - while it should only be presented if the control unit (device handler) attempts to consume data from the offending address.
### I/O Subsystem reset/Device reset not always effective (Ivan 2004/10/02)
The I/O subsystem reset function does not actually send a reset signal (as described in 17.2.2.2 in POO) to the device handlers. If an I/O is in progress, it will not be terminated.  Subsequently, if status is stacked after the I/O reset, the device may be left in an unusable state, with status pending and E bit off effectivelly preventing an MSCH to be issued with the E bit on - requiring a Power On Reset (hercules stop and start). Only ESA/390 and ESAME are affected.

## Device Handlers
### Some units still returning CE+DE+UC on not-ready conditions (Ivan 2004/10/02)
I/O architecture requires non-ready units to return UC only. This is done at the device handler lever and has not been implemented in all device handlers

## Hardware Panel
### Change to Enter key function (Ivan 2004/10/02)
Pressing enter while in single step mode used to be the equivalent of pressing the start button (which was quite handy for debugging). It now displays the last commands typed. Instead, 'start' must be typed every time to execute the next instruction.
