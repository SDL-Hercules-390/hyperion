![header image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)


# Hercules Trace-to-File Support


## Contents

1. [Background](#Background)
2. [Creating a Tracefile](#Creating-a-Tracefile)
3. [Using TFPRINT to print a Tracefile](#Using-TFPRINT-to-print-a-Tracefile)
4. [Bug Reports](#Bug-Reports)



## Background


Hercules debug tracing has traditionally involved using the `t+` and/or `t+devnum` commands,
which causes Hercules to print debug messages directly to the Hercules console (and thus
to the Hercules logfile too).

Depending on what is being traced, this typically causes many thousands of messages to be
generated, literally flooding the Hercules console with messages, therefore slowing Hercules
down to a crawl due to the unavoidable overhead of having to format each trace message before
displaying it, as well as the inefficiency of Hercules's overall management of console messages.

The trace-to-file feature of Hercules greatly reduces the overhead and inefficiency of
performing debug tracing by simply writing the raw unformatted debugging information
to a tracefile instead, which can then be printed offline at a later time, thereby allowing
Hercules to run much faster than otherwise. That is to say, the impact to your guest's
execution speed is greatly reduced, allowing it to run significantly faster than it otherwise
would had the messages been formatted and displayed on the Hercules console instead like normal.

The resulting debugging tracefile that is created can then be sent to Hercules support
personnel for analysis, which is much more convenient than sending what would otherwise
be a very large Hercules logfile.

This also allows Hercules support personnel to select and print only specific information
from the tracefile too, which is impossible to do when all you have is a very large text
logfile with all messages already formatted in it (even ones they may not be currently
interested in).



## Creating a Tracefile


To create a tracefile, you use the `tf` command that defines the file where the debugging
information will be written, and then activate debug tracing like normal (e.g. `t+` and/or
`t+devnum`, etc):

<pre>
   <b>tf on "file=path/to/your-tracefile.dat" max=10g</b>
   t+

   (tracing occurs...)

   t-
   <b>tf off</b>
</pre>

The `ON` parameter enables tracing to a file. Please note that nothing will actually be
written to the file until the first tracing event occurs, which is why after you enter
the `tf` command you must then manually enable debug tracing via the `t+` and/or `t+devnum`
commands. Once the first event occurs, the file is opened and logging begins.

The `FILE=` parameter is required, and defines the path to your desired output tracefile.
You can name the file whatever you want. If you path contains any spaces, enclose the
entire parameter within double quotes.

The `MAX=` parameter specifies the desired maximum size (in either megabytes or gigabytes)
that the tracefile is allowed to grow to. Specify the value as either `nnnM` or `nnnG`.
This prevents your tracefile from becoming unacceptably large and filling up your hard
drive.

Specify a value that is acceptable to your needs, being aware that once the maximum
is reached, all tracing is stopped. So choose a value that is large enough to hold all
of the information you wish to capture. Choosing a large value is probably the best course
of action. Depending on what you are tracing, the actual resulting tracefile will likely
be much smaller.

When tracing begins and the file is opened, you will see a message issued to the Hercules
console:
```
    HHC02383I Trace file tracing begun...
```

If the tracefile unexpectedly fills up (i.e. if the specified `max=` is reached), you
will see another message appear:
```
    HHC02379I Trace file MAX= reached; file closed, tracing stopped
```
To manually stop tracing at any time, simply enter the command to disable (turn off)
debug tracing (e.g. `t-` and/or `t-devnum`), and then enter the `tf off` (or `tf close`)
command to stop/disable tracefile tracing. This will close the tracefile allowing you
to then print it via the `tfprint` utility, or .zip it and send it to your Hercules
support contact.




## Using TFPRINT to print a Tracefile


The Hercules `tfprint` utility allows you to print all or only some of the information
captured in a tracefile. Its many options allow you to select only the specific records
needed to research your bug.

<pre>
    Usage:  <b>tfprint  [options...] tracefile</b>
</pre>





### Options:


Short |      Long    | Argument(s)
------|--------------|---------------------------------------------
 `-i` | `--info`     |
 `-c` | `--cpu`      | `hh[[-hh][,hh]]`
 `-r` | `--traceopt` | &nbsp; &nbsp; &nbsp; &nbsp; _(Hercules 'TRACEOPT')_
 `-n` | `--count`    | `nnnnnn[[-nnnnnn]` &VerticalLine; `[.nnn]]`
 `-e` | `--msg`      | `nnnnn[,nnnnn]`
 `-s` | `--storage`  | `V`&VerticalLine;`R:hhhhhh[[-hhhhhh]` &VerticalLine; `[.hhh]]`
 `-d` | `--date`     | `YYYY/MM/DD-YYYY/MM/DD`
 `-t` | `--time`     | `HH:MM:SS.nnnnnn-HH:MM:SS.nnnnnn`
 `-o` | `--opcode`   | `hhhhhhhh[,hhxxhhxxhhhh]`
 `-m` | `--msglvl`   | &nbsp; &nbsp; &nbsp; &nbsp; _(Hercules 'msglevel')_
 `-u` | `--unit`     | `uuuu[[-uuuu]`&VerticalLine;`[.nnn]]`
 `-p` | `--codepage` | &nbsp; &nbsp; &nbsp; &nbsp;_(Hercules 'CODEPAGE')_



&nbsp;
### Meaning:


<dl>


<dt><strong> --info

</strong><dd>

  Prints only Summary Information from the tracefile and then exits.



</dd><dt><strong> --cpu

</strong><dd>

  Prints only the instruction trace records for the specified CPU(s).



</dd><dt><strong> --traceopt

</strong><dd>

  Defines how registers will be printed for instruction tracing.

  `TRADITIONAL` (the default), displays the instruction first, followed by the registers.

  `REGSFIRST` displays the registers first, followed by the instruction.

  `NOREGS` does not display registers at all, and displays only the instruction instead.




</dd><dt><strong> --count

</strong><dd>

  Print only records nnnnnn to nnnnnn (by count). Allows you to print only a very specific trace record or range of trace records.




</dd><dt><strong> --msg

</strong><dd>

  Print only messages with the specified message number. The `nnnnn` value is the
  numeric portion of the Hercules message number. For example: to print only
  `HHC01334I` messages, specify '1334` for this option.




</dd><dt><strong> --storage

</strong><dd>

  Prints only instructions that reference or modify the specified 'V'irtual or 'R'eal storage range.

  This option is particularly handy in that it allows you to see only those instructions
  that access a specific area of storage. This allows you to determine where a possible
  storage overlay (buffer overrun) problem is occurring for example.
  
  It doesn't only work for instructions either. It also works for I/O. If a CCW for example
  reads data into, or writes data from, the specified storage, this will identify it.




</dd><dt><strong> --date

</strong><dd>

  Allows you to selectively print only  trace records according to the date range
  they were created. This option is usually coupled with the `--time` option as well,
  to choose only events that occurred within a very specific date/time range.




</dd><dt><strong> --time

</strong><dd>

  Allows you to selectively print only  trace records according to the time range
  they were created. This option is usually coupled with the `--date` option as well,
  to choose only events that occurred within a very specific date/time range.




</dd><dt><strong> --opcode

</strong><dd>

  This option allows you to print only specific instructions identified by
  their opcode. The `hhhhhh...` value is the hexadecimal machine instruction
  you wish to print. Only as many bytes are checked as are specified. For
  2-byte instructions, specify only `hhhh`. For 4-byte instructions, specify
  `hhhhhhhh`, etc.

  Wildcards may be used to cause the comparison to ignore specific nibbles.
  For example: to print only those `IIHL` (Insert Immediate Halfword Low)
  instructions that load any value into any register, specify `A5x1xxxx`.



</dd><dt><strong> --msglvl

</strong><dd>

  Allows you to run the `tfprint` utility with a specific non-default
  `msglevel` setting. Refer to `help msglevel` for more information.




</dd><dt><strong> --unit

</strong><dd>

  Prints only CCW trace records for the specified I/O unit(s), where `uuuu`
  identifies the specific unit(s) whose trace record(s) you wish to see.




</dd><dt><strong> --codepage

</strong><dd>

  Allows you to override Hercules's default `CODEPAGE` setting used for
  ASCII/EBCDIC translation. Refer to Hercules documentation for valid
  [`CODEPAGE`](https://sdl-hercules-390.github.io/html/hercconf.html#CODEPAGE) values.




</dd>
</dl>

It should be noted that some of the options support multiple arguments
separated by commas (most notibly the `--cpu`, `--msg` and `--opcode`
options), allowing you to print only trace records for more than one
CPU, more than one message number and/or more than one instruction
opcode. Simply follow an option's argument with a comma with no intervening
space followed by the second or subsequent argument for that same option.

For example: `--cpu 00,06` will print only instruction trace records for
CPUs 00 or 06. Similarly, specifying `--opcode 45Ex,07FE` will print only
instruction trace record for the `BAL R14,xxxx` and `BR R14` instructions.



## Bug Reports


[Bug reports](https://github.com/sdl-hercules-390/hyperion/issues)
_(together with your diagnosis of the fault, please!)_
should be either entered into our 
[**Github issue tracker**](https://github.com/sdl-hercules-390/hyperion/issues)
_(preferred)_ at https://github.com/SDL-Hercules-390/hyperion/issues/,
or else reported via message to the
[main hercules-390 discussion group](https://hercules-390.groups.io/g/group)
at https://hercules-390.groups.io/g/group.

&nbsp;

&nbsp;
