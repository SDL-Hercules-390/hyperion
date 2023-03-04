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


_**INPUT:**_

Specifies the full path filename of your desired input file. If the first character of the filename begins with an "@" (['at' sign)](https://en.wikipedia.org/wiki/At_sign)], then the specified file is actually a file containing a list files to be used as input. Enclose the value within double quotes if it contains any blanks.


_**OUTPUT:**_

Specifies the full path filename of your desired output file. Enclose the value within double quotes if it contains any blanks.


_**VOLSER:**_

Specify the 1-6 character volume serial number to be used in the VOL1 and HDR1 Standard Labels written to the tape output file. Ignored when the `NLTAPE` option is specified.


_**DATASET:**_

Specifies the 1-17 character dataset name to be used in the HDR1 Standard Label written to the tape output file.  Ignored when the `NLTAPE` option is specified.


_**LRECL:**_

Specifies the Logical Record Length (LRECL) that each input and output logical data record is expected to be. Input records which are longer than this value will be truncated. Records shorter than this value will be padded with blanks.


_**BLOCK:**_

Specifies the desired Blocking Factor to be used in determining how large each tape output block will be. The blocking factor is used to calculate the size of the tape output blocks. The logical record length (LRECL) multiplied by the blocking factor is how many bytes large each output tape block will be.


_**CODEPAGE:**_

Specifies which ASCII to EBCDIC translation Code Page you wish to use in translating your ASCII input data to EBCDIC before being written to the output tape file. Ignored when the `BINARY` option is specified.

Refer to Hercules documentation for the [`CODEPAGE`](https://sdl-hercules-390.github.io/html/hercconf.html#CODEPAGE) configuration file option for a list of valid code pages.


_**UNIQUE**_

Specifies that each input file is to be placed into a separate dataset on the output tape. For normal Standard Labeled output, each file will be wrapped in its own set of HDR/EOF1 Standard Labels. For non-labeled output each dataset will be separated from the others with a single tapemark.


_**BINARY**_

Specify the `BINARY` option to prevent any ASCII to EBCDIC translation from occurring when writing the chosen input file to the tape output file. When this option is specified the input data will be written <i><u>exactly as-is</u></i> to the output file. If this option is not specified, then each input record will first be translated from ASCII to EBCDIC using the Code Page specified by the `CODEPAGE` option before being written to the output tape.


_**NLTAPE**_

Specifying the `NLTAPE` option will cause the output tape to be created without any Standard Labels. When specified, the output tape will begin immediately with data, and each file will be separated from the other with a single tapemark, with two consecutive tapemarks being written at the end of the tape.


_**ANSI**_

Specifying the `ANSI` option causes ANSI format Standard Labels to be written to your output tape file. Choosing this option also causes your chosen input data to <i><u>not</u></i> be translated to EBCDIC before writing it to the output tape file.



## Bug Reports

[Bug reports](https://github.com/sdl-hercules-390/hyperion/issues)
_(together with your diagnosis of the fault, please!)_
should be either entered into our 
[**Github issue tracker**](https://github.com/sdl-hercules-390/hyperion/issues)
_(preferred)_, or else reported via message to the
[main hercules-390 discussion group](https://hercules-390.groups.io/g/group).

