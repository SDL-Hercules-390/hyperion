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
by the VM operator "releasing" the single spool file and, then, issuing another `devinit` command to close that file. This modification adds 
functionality to the Hercules print and punch device handlers so that VM (or other guest system) is able to inform Hercules at the beginning 
and end of each spool file so that individual host files can be created without any Hercules or VM operator console intervention.

A corresponding modification to VM/370 or other guest system is required to provide the guest side of the handshake operation. Although this 
modification has only been made and tested with VM, there is no reason why a similar modification couldn't be made to other guest operating 
systems albeit with more difficulty given the probable lack of source code. (Sample source updates to VM/3780 Community Edition is provided
in the SDL-Hercules Github repository at 
[https://github.com/SDL-Hercules-390/hyperion/vm370ce-updates](https://github.com/SDL-Hercules-390/hyperion/vm370ce-updates).) 

## Runtime Operation

When the new handshake feature is enabled for a Hercules emulated printer or card punch device, two additional CCW opcodes are supported for 
that device:

1. X'F7' - Spool file open (specifies the spool file name)
2. X'FF' - Spool file close (SLI bit recommended)

These CCWs should only be issued when the guest O/S detects that is it running under Hercules. Either that or the modification to the guest O/S 
should be made so that the Command Reject errors are ignored.

The resulting spool file will be created on the host system as follows:

* The *path* to the file will be the same as the path to the file specified on the `attach` or `devinit` command.
* The *name* of the file will be the value specified on the X'F7' CCW. 
If the X'F7' CCW does not specify any name, then the name from the `attach` or `devinit` command will be used. 
Any sequence of spaces will be reduced to a single space dash ('-') and all trailing spaces will be truncated.
* The *extension* of the file will be the same as the file extension specified on the `attach` or `devinit` command.

When the "file open" CCW (X'F7') is issued, the full target file path is built according to the above rules. If that file (e.g. `/myvm/io/printer.lst`)
already exists, it is renamed with a "_1" suffix (e.g. `/myvm/io/printer_1.lst`). If that file also exists then it is renamed with a "_2" suffix
(e.g. `/myvm/io/printer_2.lst`). If the "_2" suffixed file exists, it is renamed with a "_3" suffix, and so forth up to the maximum suffix of "_99"
after which the existing "_99" file will be deleted and "_98" renamed to "_99", "_97" to "_98", and on up the line. After all this, the
new, empty output file (e.g. `/myvm/io/printer.lst`) will be opened.

## Device Configuration Changes

THis update makes changes to the configuration options for printer and card punch devices.

<b>New `handshake` Option on `attach` or `devinit` command:</b>
<p>
The new `handshake` option must be specified to enable the processing of the handshaking CCWs described above. If this option is not specified,
the behavior of the printer and card punch device handlers is unchanged. 
<p>
The `handshake` option is not allowed
when the `sockdev` option or the `append` option is specified or the output is a "piped" destination.

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
