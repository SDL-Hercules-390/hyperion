![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# MAKETAPE -- Create an .AWS Tape File from Data Utility

## Contents

1. [Description](#Description)
2. [Synopsis](#Synopsis)
3. [Arguments](#Arguments)
4. [Bug Reports](#Bug-Reports)

## Description

The `MAKETAPE` utility reads binary or ASCII line sequential files and builds either a Standard Labeled or unlabeled .AWS tape output file.
 
You can specify the blocking factor to be used for the output datasets, the logical record length of the input and output files, and whether all input files should be placed into a single logical output file or whether the data from each input file should be written to a separate unique logical output file.

## Synopsis

```
  maketape   ARG: value  [ARG: value]  [OPTION] ...
```

## Arguments


There do not appear to be any positional arguments. All arguments are keyword based, with the `INPUT:` and `OUTPUT:` arguments _always_ being required, and the other arguments and/or options being either optional or possibly required based on the values chosen for the other arguments/options.

For example, if the `NLTAPE` option is specified, then the `VOLSER:` and `DATASET:` arguments are obviously not required. Similarly, if the `BINARY` option is specified then the `CODEPAGE:` argument is ignored.


_**ANSI**_

Specifying the `ANSI` option causes ANSI format Standard Labels to be written to your output tape file.

Choosing this option also causes your chosen input data to <i><u>not</u></i> be translated to EBCDIC before writing it to the output tape file.


_**BINARY**_

Specify the `BINARY` option to prevent any ASCII to EBCDIC translation from occurring when writing the chosen input file
to the tape output file.

When this option is specified the input data will be written <i><u>exactly as-is</u></i> to the
output file. That is to say, any records shorter than the specified `LRECL:` are _not_ padded with blanks nor translated.

If this option is _not_ specified however, then each input record is considered to be a text record and will first be
padded with blanks to reach the specified `LRECL:` and then translated from ASCII to EBCDIC using the Code Page specified
by the `CODEPAGE:` option before being written to the output tape.

**NOTE:** When a **"meta-file"** is used as input (refer to the `INPUT:` option further below), 
the `BINARY` option is applied by default to _all_ files listed in your meta-file. 
If you wish to be able to have some files processed as binary files and other files 
processed as text files, then do _not_ specify the `BINARY` option, and use the `'BIN'` 
meta-file keyword instead.

_**BLOCK:**_

Specifies the desired Blocking Factor to be used in determining how large each tape output block will be. The blocking factor is used to calculate the size of the tape output blocks. The logical record length (`LRECL:`) multiplied by the blocking factor (`BLOCK:`) is how many bytes large each output tape block will be.


_**CODEPAGE:**_

Specifies which ASCII to EBCDIC translation Code Page you wish to use in translating your ASCII input data to EBCDIC before being written to the output tape file. Ignored when the `BINARY` option is specified.

Refer to Hercules documentation for the [`CODEPAGE`](https://sdl-hercules-390.github.io/html/hercconf.html#CODEPAGE) configuration file option for a list of valid code pages.


_**DATASET:**_

Specifies the 1-17 character dataset name to be used in the HDR1 Standard Label written to the tape output file.  Ignored when the `NLTAPE` option is specified.


_**INPUT:**_

Specifies the full path filename of your desired input file. Enclose the value within 
double quotes if it contains any blanks.

If the first character of the filename begins with an "@" 
(['at' sign)](https://en.wikipedia.org/wiki/At_sign)], then the 
specified file is actually a _**"meta-file"**_ containing the actual 
list files to be used for input.

If the use of a _**"meta-file"**_ is specified, each statement of the meta-file is the name 
of the actual input file to be used, optionally preceded with the keyword `'BIN'` and separated 
from the filename with a single blank. This allows you to create a tape with a mixture of 
both `BINARY` and text files. Text file meta-file statements consist of only the filename. 
Binary file meta-file statements begin with the keyword `'BIN'` followed by a single blank 
and then the name of the input file. The maximum number of files that may be specified in 
a meta-file is 50. Any excess is ignored with a warning message.

_**LRECL:**_

Specifies the Logical Record Length that each input and output data record is expected to be. Input records which are
longer than this value will be truncated. Records shorter than this value will be padded with blanks unless the `BINARY`
option is specified. _(When the `BINARY` option is specified, records shorter than this value are <u>not</u> padded at
all and are instead written out <u>exactly as-is</u> to the output file.)_


_**NLTAPE**_

Specifying the `NLTAPE` option will cause the output tape to be created without any Standard Labels. When specified, the output tape will begin immediately with data, and each file will be separated from the other with a single tapemark, with two consecutive tapemarks being written at the end of the tape.


_**OUTPUT:**_

Specifies the full path filename of your desired .AWS tape output file. Enclose the value within double quotes if it contains any blanks.


_**UNIQUE**_

Specifies that each input file is to be placed into a separate dataset on the output tape. For normal Standard Labeled output, each file will be wrapped in its own set of HDR/EOF1 Standard Labels. For non-labeled output each dataset will be separated from the others with a single tapemark.


_**VOLSER:**_

Specify the 1-6 character volume serial number to be used in the VOL1 and HDR1 Standard Labels written to the tape output file. Ignored when the `NLTAPE` option is specified.



## Bug Reports

[Bug reports](https://github.com/sdl-hercules-390/hyperion/issues)
_(together with your diagnosis of the fault, please!)_
should be either entered into our 
[**Github issue tracker**](https://github.com/sdl-hercules-390/hyperion/issues)
_(preferred)_, or else reported via message to the
[main hercules-390 discussion group](https://hercules-390.groups.io/g/group).

