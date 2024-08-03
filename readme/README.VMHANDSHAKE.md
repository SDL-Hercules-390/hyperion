![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# VM Handshake for Unit-Record Output Devices

## Contents

1. [About](#About)
2. [Runtime Operation](#Runtime Operation)
3. [Device Configuration Changes](#Device Configuration Changes)
4. [Bug Reports](#Bug-Reports)
5. [See Also](#See-Also)
6. [History](#History)

## About

William E. Denton [williamEdenton@yahoo.com](mailto:williamEdenton@yahoo.com) has modified the Hercules printer and card punch device support
to allow separate files to be created on the host system for each guest print/punch spool file without any intervention from the Hercules console.
Each of these files will have a name assigned corresponding to the VM print/punch file name/type.

Before this enhancement, each file printed or punched from VM would require a Hercules `devinit` command issued to set the file name followed
by the VM operator "releasing" the single spool file. This modification adds functionality to the print and punch device handlers so that VM is 
able to inform Hercules at the beginning and end of each spool file so that individual host files can be created without any Hercules or VM
operator console intervention.

A corresponding modification to VM/370 is required to provide the VM side of the handshake operation. Although this modification has only been 
made and tested with VM, there is no reason why a similar modification couldn't be made to other guest operating systems albeit with more
difficulty given the lack of source code. 

## Runtime Operation

The Hercules side of the spool file handshake is implemented by adding two new CCW opcodes to the printer and punch device handlers:

1. X'F7' - Spool file open (specifies the spool file name)
2. X'FF' - Spool file close (SLI bit recommended)

These CCWs should only be issued when the guest O/S detects that is it running under Hercules. Either that or the modification to the guest O/S 
should be made so that the Command Reject errors are ignored.

The resulting spool file will be created on the host system as follows:

* The *path* to the file will be the same as the path to the file specified on the `devinit` command.
* The *name* of the file will be the value specified on the X'F7' CCW. 
If the X'F7' CCW does not specify any name, then the name from the `devinit` command will be used. 
Any sequence of spaces will be reduced to a single space dash ('-') and all trailing spaces will be truncated.
* The *extension* of the file will be the same as the file extension specified on the `devinit` command.

## Device Configuration Changes

THis update makes changes to the configuration options for printer and card punch devices.

1. <b>Changed `append` Option Semantics</b>
<p>
The semantics of the `append` option are changed when that  option is *not* specified. (Current processing is unaltered when `append` **is** specified.)
If `append` **is not** specified and the target output file of the same name already exists, it will be renamed to 
<i>path/name</i><b>\_1</b><i>.extension</i>. If that "\_1" file exists, it will be renamed to "\_2" and an existing "\_2" file will be renamed to "\_3", etc.
This file renaming will be applied independent of the `handshake` option which is only used to determine the output file root name.

2. <b>New `handshake` Option</b>
<p>
The new `handshake` option must be specified to enable the processing of the handshaking CCWs described above. If this option is not specified,
the behavior of the printer and card punch device handlers is unchanged except for the changed `append` semantics. 
<p>
The `handshake` option is not allowed
when the `sockdev` option is specified or the output is a "piped" destination.

## Bug Reports

[Bug reports](https://github.com/sdl-hercules-390/hyperion/issues)
_(together with your diagnosis of the fault, please!)_
should be either entered into our 
[**Github issue tracker**](https://github.com/sdl-hercules-390/hyperion/issues)
_(preferred)_ at https://github.com/SDL-Hercules-390/hyperion/issues/,
or else reported via message to the
[main hercules-390 discussion group](https://hercules-390.groups.io/g/group)
at https://hercules-390.groups.io/g/group.

## See Also

The Hercules emulator homepage at: http://www.sdl-hercules-390.org/


## History

    2024-06-07  Originally written by William E. Denton
