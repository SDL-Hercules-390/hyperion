![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# VMFPLC2 (VM) Formatted Tape Utility

## Contents

1. [Description](#Description)
2. [Synopsis](#Synopsis)
3. [Arguments](#Arguments)
4. [Options](#Options)
5. [Control File](#Control-File)
6. [Examples](#Examples)
7. [Bug Reports](#Bug-Reports)
8. [See Also](#See-Also)
9. [History](#History)

## Description

Hercules command to manipulate (create or read) VMFPLC2 formatted tapes for VM/CMS use.

## Synopsis

```
  vmfplc2   [options]   VERB  [<ctlfile>]  <tapein> | <tapeout>
```

## Arguments

The vmfplc2 utility requires a function (DUMP, SCAN or LOAD) followed by the name of a control file and the name of an input or output tape file.


```
    DUMP      The DUMP function allows creating a VMFPLC2 formatted tape.

    SCAN      The SCAN function allows listing the contents of a VMFPLC2
              formatted tape.

    LOAD      The LOAD function allows importing the contents of a VMFPLC2
              formatted tape unto the system.

    ctlfile   The name of the control file identifying which host file(s)
              are to be DUMPed to the tape output file or LOADed from the
              tape input file.  See the "CONTROL FILE" section below for
              the format of the control file.

    tapein    The name of the input tape file for the SCAN/LOAD functions.

    tapeout   The name of the output tape file to be created by the DUMP
              function.
```

Surround the name of the control file and/or the name of the input/output tape file with double quotes if it contains any blanks in its name.

## Options


```
    -c cp     The desired translation code page to be used for ASCII to
              EBCDIC translation (and vice versa).  For a list of valid
              code pages please refer to the Hercules documentation for
              the 'codepage' configuration file statement.  The default
              translation code page is "default".  The recommended code
              page is "819/1047".

    -u        Create an uncompressed .AWS output tape.
    -z        Creates a compressed .HET output tape using zlib (default).
    -b        Creates a compressed .HET output tape using bzip2.
    -n        Desired compression level (-1, -2 ... -9).  Default is -4.
    -v        Verbose. (Don't suppress certain informational messages.)

    -t        Read input tape or create output tape in CMS 'TAPE' DUMP
              format.  The default is to use the 'VMFPLC2' DUMP format.
              The 'TAPE' DUMP format allows tapes to be read by earlier
              versions of VM/CMS such as VM/370.  The VMFPLC2 format is
              only used by later versions of VM/CMS such as e.g. z/VM.
```

## Control File

The control file lets the DUMP/LOAD functions to determine which host files are to be dumped or loaded to/from tape and how they should be interpreted on VM.  Each statement of the control file has the following format:


```
       [codepage]  <fn>  <ft>  <fm>  <recfm>  [lrecl]  <type>  <hostfile>

    codepage  Is an optional code page specification to be used when dumping
              or loading the specified file. If the first word contains a '/'
              slash character anywhere in its name it is interpreted as a code
              page specification.  Otherwise the first word is interpreted
              as the 'fn' filename argument and the codepage specified on the
              command line is used instead.  Code pages are only used to DUMP
              or LOAD Text files (refer to the "type" argument further below).

    fn        Is the 1 to 8 characters that represents the file name. The file
              name can be specified as lower case, but will be translated to
              upper case to follow CMS conventions.  Characters allowed are
              [A-Z], [0-9], $ (Dollar), + (Plus), - (Minus), : (Colon), @ (At),
              # (Pound/Hash) and _ (Underscore).

              The special filename "@TM" (At, "T", "M") is interpreted as a
              request to write a tapemark to the output file, and the rest of
              the line is ignored.

              The special filenames "*" (asterisk), "#" (pound or hash) or ";"
              (semicolon) are treated as comment lines causing and the rest of
              the line to be ignored.  All blank lines are also always ignored.

    ft        Is the 1 to 8 characters that represents the file type. The file
              type can be specified as lower case, but will be translated to
              upper case to follow CMS conventions.  Characters allowed are
              [A-Z], [0-9], $ (Dollar), + (Plus), - (Minus), : (Colon), @ (At),
              # (Pound/Hash) and _ (Underscore).

    fm        Is 1 or 2 characters that represents the file mode. The first
              character is a letter from A to Z and represents the "original"
              file mode when scanned on VM/CMS. (It does NOT cause the file
              to be loaded on that cms disk.)  The second character is a digit
              from 0 to 6.  The file mode number indicates specific behavior
              for the file under CMS. Refer to CMS documentation for more info.

    recfm     Indicates the record format, and should be specified as either
              F[ixed] or V[ariable].

    lrecl     Indicates the logical record length.  This should only be spec-
              ified for recfm F[ixed] files.  For recfm V[ariable] files the
              record length varies from one record to the next and thus should
              NOT be specified (should be left out from the statement).

    type      Indicates how the file should be processed.  The only supported
              values are B[inary], S[tructured] or T[extual]:


                Binary      The file is not translated and is written as-is.
                            For RECFM F files, the file is cut into records
                            of the length specified by the lrecl parameter.
                            For RECFM V files, the file is cut into records
                            of 65535 bytes except for the last record which
                            has a length of the remainder of the file.


                Structured  The file is in structured format where each record
                            is preceded with a 16-bit BIG ENDIAN format length
                            field followed by the bytes of the record itself.
                            The recfm must always be specified as V[ariable].
                            Structured files are not translated and are written
                            as-is.


                Textual     The file is translated from ASCII to EBCDIC using
                            the specified translation code page if specified,
                            or the code page specified on the command line if
                            not specified.  Refer to the "codepage" parameter
                            further above.

                            Trailing CR, LF, CRLF line termination characters
                            are removed by DUMP and added by LOAD.

                            For RECFM F files the record is truncated if it
                            is longer than the specified lrecl or padded with
                            EBCDIC X'40' characters (white space) if shorter.

                            For RECFM V files records are neither padded nor
                            truncated and can be up to 65535 characters long.


    hostfile  The name of the input or output host file to be DUMPed or LOADed.
              If the full path of the file is not specified then the file is
              treated as being relative to the current directory.  Enclose the
              file name within double quotes if it contains any blanks.
```


## Examples

The following creates a VMFPLC2 formatted tape named "vmfplc2.het" containing three files named "mytest.asm", "profile.xedit.txt" and "synonyms.txt", which can then be loaded onto the guest system as CMS files `MYTEST ASSEMBLE A1`, `PROFILE XEDIT A1` and `MY SYNONYM A1`:


<pre>
    <b>command</b>:        vmfplc2  DUMP myfiles.txt  vmfplc2.het

    <b>myfiles.txt</b>:    MYTEST   ASSEMBLE A1  Fixed 80 Text  mytest.asm
                    PROFILE  XEDIT    A1  Variable Text  profile.xedit.txt
                    USER     SYNONYM  A1  Fixed 80 Text  synonyms.txt
</pre>

The following example reads a VMFPLC2 formatted input tape named "vmfplc2.het" and lists all of the files that are found on the tape:


<pre>
    <b>command</b>:        vmfplc2  SCAN vmfplc2.het
</pre>

The following example reads a VMFPLC2 formatted tape named "vmfplc2.het" and restores onto the host system any files found on the tape that match one of the entries listed in the "myfiles.txt" control file:

<pre>
    <b>command</b>:        vmfplc2  LOAD myfiles.txt  vmfplc2.het

    <b>myfiles.txt</b>:    MYTEST XEDIT A1  Variable Text  mytest.rexx
                    MYTEST DATA  A1  Fixed 80 Text  mytest.dat
</pre>

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

    2010-08-07  Originally written by Ivan S. Warren

    2019-02-29  Program completely rewritten by "Fish" (David B. Trout)
                Any typos or bugs are purely my own fault and not Ivan's.
