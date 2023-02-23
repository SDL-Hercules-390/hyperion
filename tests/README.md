![test image](../readme/images/image_header_herculeshyperionSDL.png)
[Return to master README.MD](../README.md)

# Low-level Test Cases

## Contents

1. [About](#About)
2. ["runtest" shell script](#runtest-shell-script)
3. [.tst file runtest command](#tst-file-runtest-command)
4. [.tst file directives](#tst-file-directives)
5. [Predefined variables](#Predefined-variables)
6. [Creating test files that can run on z/CMS as well as under Hercules](#Creating-test-files-that-can-run-on-zCMS-as-well-as-under-Hercules)


## About

This directory contains test cases that are run by Hercules console commands.  High-level test cases are run under an operating system, for example OS/j.

Test cases are run after Hyperion has been built, but before it is installed.  "make check" runs the entire regression test reference.

With current Hyperion as of 2016-06-06, there are no longer any special considerations for testing crypto.

A test case is run stand-alone under hercules from a script specified with the `-r` flag.  The script first sets up a program in core storage either by the `r` console command or by loading a core image by `loadcore`. It then issues the `runtest` command to start Hercules by a restart interrupt, the restart new PSW having been set up in the previous step. The script regains control when all CPUs have entered the stopped state. The remainder of the script inspects storage, registers, etc., to display data that are later inspected.  The session log is then processed to check the results against known good values.

<b>You can create test cases in several ways:</b>

* Entirely by hand using only a text editor.  Many test cases have been built this way, but this usage has been superseded by use of an assembler that can generate core images directly.
<br><br>
* By using Harold Grovesteen's excellent [SATK's](https://github.com/s390guy/SATK) ASMA ("<b>A</b> <b>S</b>mall <b>M</b>ainframe <b>A</b>ssembler"), which is an assembler that implements a subset of HLASM's functionality, but has the ability to generate a core image directly.  Thus no post processing is required. Typically, you will write by hand a `.tst` file to load the `.core` file and do whatever is required.
<br><br>
* By a real /360 assembler, such as `IFOX` or `HLASM`.  The object deck is converted to `r` console commands or a core file.  This offers the full assembly language with a powerful macro capability, but at the expense of some post processing.

Validating the test case (specifying the desired results; the oracle in testing parlance) is the most difficult step if you do not have access to z/CMS.  There may be ambiguities in the Principles Of Operation, or perhaps more likely, you may misinterpret the manual.  As IBM does not publish the suite the developers use to validate a new CPU model, you will need to find a friend who has access to "real iron".  This real iron could be z/PDT.

There are two routes to obtaining an object module and validation script for z/CMS:  From an ASSEMBLE file using an assembler or from a `.tst` file by the `cvttst.rexx` utility (very much still in the development phase).

<b>By convention, the following file type extensions are used:</b>


<b>.tst</b>

Files that have a tst extension are to be run by the "runtest" or
"runtest.cmd" shell script.  They are run and the Hercules console
output is then inspected for correctness by an automated rexx script.
You can create .tst files either "by hand", with the ASMA (SATK); or by
assembly on a /360 system (VM, MVS, DOS), but then TEXT decks need to be
processed into .tst files.


<b>.subtst</b>

Files that have a subtst extension are secondary helper scripts called
by a primary tst script.  They are used when more than one primary tst
script consists of code that is virtually identical to another.  Rather
than duplicate the same code in multiple tst files, the common code is
placed into a subtst file instead and "called" by each of the primary
tst scripts (via the 'script' command).  Think of them as tst script
"subroutines".


<b>.assemble</b>

Source files for test cases that are designed for assembly with HLASM.
On z/VM with HLASM, the test cases can be prepared by BLDHTC EXEC
(bldhtc.exec) on VM or by bldhtc.rexx on the workstation.  ASMTST XEDIT
(asmtst.xedit) is a sample macro to perform the build step from within
the XEDIT session.

On VM/370, you can use the system assembler to assemble, but you will
likely do the conversion on the workstation.

http://vm.marist.edu/~pipeline/zops.copy contains a stacked copy file for
many new operation codes.


<b>.listing</b>

An assembler listing file produced by HLASM.


<b>.asm</b>

Source files for the `.tst` test cases that are designed for assembly by
ASMA (Harold Grovesteen's [SATK](https://github.com/s390guy/SATK)).
To clone:  [git@github.com:s390guy/SATK.git](git@github.com:s390guy/SATK.git).

When instructed accordingly, the assembler will produce a core image file
(`.core`) that can be loaded with the `loadcore` console command.

This makefile snippet can automate the process.  You must modify it with
the location of the SATK and Hyperion build directories in your system:

```
.SUFFIXES: .text .tst .assemble .rexx .asm .core
USD:=/usr/data/src
APATH:=${USD}/SATK/tools

%.core: %.asm
   ASMPATH=. ${APATH}/asma.py -l $*.list -i $@ $<
```

While ASMA so obviously is superior to hand assembly, it does have a few
limitations relative to HLASM that you should be aware of:

* Always check the documentation of the assembler instructions you
  intend to use to be sure they are indeed implemented.  Notable
  omissions are `AMODE` and `RMODE`.

* No floating point constants.  `D'...'` assembles as if `FD'...'` were
  specified, which is correct only for `D'0'`.  Use hexadecimal
  constants to assemble floating point constants.  The file
  `dc-float.asm` contains, in no particular order, a number of
  constants suitable for cut-and-paste.  This file was generated
  from the HLASM listing file from an assembly of float constants.

* Input is variable length ASCII.  Continuation is specified by a
  backslash at the end of a line.  The alternate format for macro
  instructions is not supported.


<b>.list</b>

An assembler listing file produced by ASMA.


<b>.txt</b>

Files that have a `.txt` extension are manually executed test scripts
invoked via the Hercules `script` command.  Their output must be
manually eyeballed to ensure they executed correctly.  Usually they
end by loading a disabled wait PSW.  If the PSW's instruction address
is 0 then the test case likely completed successfully.


<b>.core</b>

Files that have the extension `.core` are binary core image files loaded
into main storage by the Hercules `loadcore` console command.  They can
be created by many means; one of them is ASMA as described above.



## "runtest" shell script

The `runtest` UNIX shell script (`runtest.cmd` batch file on Windows)
is used to run one or more (usually all) automated `.tst` scripts to
verify proper Hercules functionality.

On UNIX, assuming that your are building outside the source tree,
perhaps in the "atom" directory, the source directory might be
`../hyperion`.  Use the following command to run and verify the `.tst` test
cases:

          ../hyperion/tests/runtest

"make check" in the directory where you built Hercules will find stuff
wherever you configured it.

On Windows the batch file `runtest.cmd` runs the test cases.  Its first
few parameters are identical (almost) to the parameters accepted by
the UNIX runtest shell script, but supports many additional parameters
as well.  Enter `runtest /?` or `runtest --help` for more information.

In either case, runtest first creates a composite set of test cases
(`allTests.testin` in the current working directory) and then invokes
Hercules to run them using the configuration file `tests.conf` and the
.rc file just created.  When it completes, the console output
(`allTests.out`) is then inspected by `redtest.rexx` and the results are
reported to the user.

The runtest return code is the number of failed test cases.  A return
code of 0 indicates success (no failed test cases).  Any other return
code indicates failure (one or more failed test cases).

The tail end of an early sample output is:

```
  ...
  Test SSKE S/390 nextlast.   3 OK compares.   All pass.
  Test SSKE S/390 lastpage.   3 OK compares.   All pass.
  Test timeout.   2 OK compares.   All pass.
  Done 138 tests.   All OK.
```

The Linux runtest shell script accepts these flags:

<b>`-d <word>`</b><br>
Specify the path to the test directory.  The default is ${dirname $0}, perhaps ../hyperion/tests

<b>`-e <word>`</b><br>
Specify the default extension for the input test files.  tst is the default.  The -e flag is active for all -f flags until the next -e flag, if any.

<b>`-f <word>`</b><br>
Specify the input file name perhaps including an extension.  The default is `-f *.tst` to get the current behaviour.  Multiple -f flags are supported.  The default extension is appended if the word does not contain a period.

<b>`-h <word>`</b><br>
Specify the object directory that contains the hercules executable.  The
default is parent of -d if there is no hercules executable in the
current directory (that is, in-source build).

<b>`-l <word>`</b><br>
Specify the name of a module to load dynamically into hercules.  This is passed to Hercules.

<b>`-p <word>`</b><br>
Specify the library path for loadable modules, i.e., device managers. This is passed to Hercules.

<b>`-q`</b><br>
Pass "quiet" to redtest to suppress details about test cases.

<b>`-r <number>`</b><br>
Repeat the composite test script n times.  There is no need to specify -x as hercules terminates without an explicit quit when in daemon mode.

<b>`-t <number>`</b><br>
Passed to hercules (timeout factor)

<b>`-v <word>`</b><br>
Set variable for redtest.rexx (in fact, arguments to redtest.rexx).  Use `-v ptrsize=4` when running a 32 bit executable on an 8 bit system if no valid config.h can be found. You can also set private variables this way (but such variables will be unset when running "make check").  Variable names are case insensitive but their values are not. `-v quiet` will make redtest more terse.

<b>`-w <word>`</b><br>
Specify the work file name.  The default is allTests.

<b>`-x`</b><br>
Do not append the quit command to the composite test script.


## .tst file runtest command

The test a `.tst` script defines is normally begun via a special `runtest`
script command which supports optional arguments indicating how the
test should be started as well as the maximum amount of time it should
run. Unlike the `pause` command, `runtest` waits only until all CPUs
are in the stopped state:

```
runtest   [restart|start|<oldpsw>]   [timeout]   [# comment...]
<oldpsw> ::= external | svc | program | machine | io
```

The first argument is optional.  If specified, it must be either
`restart`, `start`, or an interrupt type.  It specifies how the test
should be started: via the Hercules `restart` console command or the
`start` console command.  The default is to start the test via the
`restart` command.  When an interrupt type is specified, the old psw for
that type is copied to the restart new PSW before the restart command is
issued to fire up the test case where it left off taking an interrupt
for which the new PSW was disabled.

The second argument, which is also optional, defines the maximum amount
of time (specified in a whole or fractional number of seconds) that the
test is allowed to run.  This is a safety feature to prevent runaway
tests from running forever due to a bug in the test or within Hercules
itself.  Under normal circumstances all tests should end automatically
the very moment they are done. The minimum timeout is 0.001 seconds.
The maximum is 300 seconds (5 minutes). If no timeout is specified,
a default timeout of 30 seconds is used.

Please also note that timeout values are multiplied by the test
timeout factor value which is specified by the `-t` switch on the
Hercules command.  The test timeout factor value allows for running
tests on systems that may be slower than the system they were
originally designed for.  Refer to the documentation describing
[Hercules's supported command line arguments](https://sdl-hercules-390.github.io/html/hercinst.html#arguments).

The `runtest` script command is almost always immediately followed
by Hercules console commands to display storage or registers, etc, as
well as several of the below `.tst` file directives to verify the test
case ran correctly.


## .tst file directives

`.tst` files are executed as Hercules `.rc` files, so they contain Hercules
console commands.

A number of "loud" comments (without a blank immediately following it)
are interpretted by `redtest.rexx` as directives.  All testing directives
are case sensitive.  Using `*testcase` for example accomplishes nothing.


```
*           Hercules "loud" comment.  There must be a blank after the
            asterisk for it to be ignored.  If it is not followed by a
            blank it will interpretted as a test directive.

*Testcase   Identify beginning of a test case.

*Compare    An 'r', 'v' or 'abs' command follows and the output should
            be stored for later comparison.  The '*Compare' directive
            triggers the storing of messages in the message queue for
            later comparison by the '*Hmsg' and other directives.  If
            not given then any subsequent '*Hmsg' directives will fail
            due to the stored message queue being empty.

*Want       The expected 'r' command output.  A string identifying the
            comparison may be enclosed in double quotes before the
            compare data (e.g. `*Want "CRC-32" DF00E74C`)

*Gpr        A previous 'gpr' console command should have been issued
            to display the general registers.  The specified register
            is compared with the hexadecimal data specified after the
            register number.  Specify a decimal register number with
            no leading zero.  The comment "#address" may be specified
            to notify the CMS test driver that the register contains
            a storage address.

*Fpr        Same thing as *Gpr except for 'fpr' floating-point registers.
*Cr         Same thing as *Gpr except for 'cr' control registers.

*Key        Compare the specified key against the key displayed in the
            previous 'r' command.

*Prefix     Compare the specified value to the contents of the prefix
            register displayed by a previous 'pr' command.

*Program    A program check is expected, for example to verify that a
            privileged operation causes an 0c2.  Since program checks
            typically cause a disabled wait state psw to be loaded,
            to prevent an unwanted "unexpected wait state" error from
            being detected instead this directive MUST be specified
            BEFORE the 'runtest' script command that starts the test.

*Hmsg       Specifies a complete informational message string to be
            compared against the last stored message (except for any
            messages that are processed by redtest.rexx plus messages
            HHC00007I and HHC01603I).  A number is optional before the
            message identifier to specify the last but one message, the
            last but two message, etc.  For example, if a test expects
            three messages then you would use:

                *Hmsg 2  First expected message...
                *Hmsg 1  Second expected message...
                *Hmsg    Third expected message...

            The stored messages queue does not begin storing messages
            until the '*Compare' directive is first given.

*Error      Synonym for *Hmsg.  Use if an error message is expected.
*Info       Synonym for *Hmsg.  Use if an informational message is
            expected.

*Explain    Specify  the explanatory text to write when the next
            comparison fails.  You can specify as many *Explains as you
            like for multiline messages.  The explain array is cleared
            once a test has been done, whether OK or not.

*Message    Print a message on the output.

*If         Specify a condition to test (see later).  The expression is
            evaluated by the Rexx interpreter.   If the condition holds,
            the following directives are processed up to the matching
            *Else or *Fi.   If the condition does not hold, directives
            are ignored except that nested *If directives are parsed.

*Else       Flip the setting of the matching *If condition, unless
            nested in a suppressed *If.

*Fi         End an *If.   The process/ignore setting for directives is
            unstacked.

*Done       End of a test case.  Specify "nowait" if the test case does
            not end by loading a disabled PSW, for example, when it
            tests just console commands without running any code.

*Timeout    A timeout is expected.  Do not use.
```

`logicimm.tst` shows examples of all directives, except *Key, *Prefix,
and *Program; see `sske.tst` for an example of *Key and *Program.  See
`pr.tst` for *Prefix.  See `mainsize.tst` for conditional processing.


## Predefined variables

Predefined variables are specified as arguments to `redtest.rexx` as well
as being extracted from the `HHC01417I` features messages.  The runtest
script (not to be confused with the runtest command above) sets the value
of the `$ptrsize` variable to either 4 or 8 by directly passing the value
to `redtest.rexx` on the command line.  Any number of user defined variables
can also be defined the same way by simply passing their name and value
on the command line to `redtest.rexx`.  Refer to runtest's help display
for how to do this.

Other variables are defined by `redtest.rexx` itself based on the Hercules
`HHC01417I` startup and other messages.  For example, the variable `$cmpxchg1`
is set to 1 (true) if "cmpxchg1" appears in message `HHC01417I Machine dependent
assists:`.  The variable `$locking_model` is set to `error` if the message
`HHC01417I Using Error-Checking Mutex Locking Model` is seen, and so on.

Variables are either of boolean, numeric or string type and are read only.
A variable's name is case insensitive but its value is not.  $foo, $FOO,
$Foo, etc, all refer to the same variable.  Predefined variables are meant
to be use in `*If` directives to adjust test cases according the defined
value of a given variable or variables, since one user's build of Hercules
(or the environment in which it runs) might not be the same as another's.
Designing you test cases to take this into consideration is encouraged.


The current list of pre-defined runtime variables are as follows:


  Boolean:

    $can_esa390_mode
    $can_s370_mode
    $can_zarch_mode
    $CCKD_BZIP2
    $cmpxchg1
    $cmpxchg16
    $cmpxchg4
    $cmpxchg8
    $CONFIG_INCLUDE
    $CONFIG_SYMBOLS
    $externalgui
    $fetch_dw
    $HAO
    $HDL
    $HET_BZIP2
    $HTTP
    $IEEE
    $IPV6
    $NLS
    $regex
    $rexx_ErrPrefix
    $rexx_MsgLevel
    $rexx_MsgPrefix
    $rexx_Resolver
    $rexx_supported
    $rexx_SysPath
    $shared_devices
    $SIGABEND
    $sqrtl
    $store_dw
    $syncio
    $SYSTEM_SYMBOLS
    $ZLIB

  Numeric:

    $max_cpu_engines
    $ptrsize

  String:

    $hatomics           atomicIntrinsics, C11, msvcIntrinsics, syncBuiltins
    $keepalive          Basic, Full, Partial
    $libraries          Shared, Static
    $locking_model      Error, Normal, Recursive
    $platform           Linux, Windows, etc.
    $rexx_Extensions    .REXX;.rexx;.REX;.rex;.CMD;.cmd;.RX;.rx
    $rexx_Mode          Command, Subroutine
    $rexx_package       OORexx, Regina
    $rexx_RexxPath      /foo:/bar:/foobar (or C:\Foo;C:\Bar;C:\FooBar)
    $rexx_SOURCE        WindowsNT, LINUX, etc.
    $rexx_VERSION       REXX-ooRexx_4.2.0(MT)_64-bit 6.04 22 Feb 2014
    $threading_model    Fish, POSIX


Examples of if/else variable usage:


```
  mainsize 2147483648b
  *If $ptrsize = 4
      *Error 1 HHC01430S Error in function configure_storage(2G): Cannot allocate memory
      *Error   HHC02388E Configure storage error -1
  *Else # 64 bit
      *Info HHC17003I MAIN     storage is 2G (mainsize); storage is not locked
  *Fi


  *If $HET_BZIP2
      ...(do test here)...
  *Else
      *Message SKIPPING: Testcase MyTest
      *Message REASON:   No BZIP2 support
  *Fi


  *If \$rexx_supported
      *Message SKIPPING: Testcase Example
      *Message REASON:   No Hercules Rexx support.
  *Else
      *If $rexx_package = ''
          *Message SKIPPING: Testcase Example
          *Message REASON:   Rexx is not installed.
      *Else
          ...(do test here)...
      *Fi
  *Fi
```

## Creating test files that can run on z/CMS as well as under Hercules
-------------------------------------------------------------------

Running a test case on z/CMS is accomplished by `DOTEST EXEC`.  It merges the object deck for the test case with a pipeline program that interprets the runtest directives, which are extracted from the object deck.

A test case to run on z/CMS can be generated in two ways:

1.    From assembler source.  The object deck is uploaded to z/CMS and
      used directly.  The Hercules (bare iron) test case is created from
      the object deck by `bldhtc.rexx`.

2.    From a hand-built `.tst` script.  The test script is converted to a
      TEXT deck by `cvttst.rexx`.

There are a number of differences when a test case is run on CMS rather
than on the bare iron:

1.    The machine is not all yours.  Well, it _is_, but you may not wish
      to exercise that prerogative.  You'd likely want CMS to remain
      after running the test case.

2.    GPR15 is the base register for an emulated main storage the size
      of the program (see agf.assemble).  The test case is entered with
      the return address in GPR14.

3.    The test case must not modify GPR11 (or it must save and restore
      it).

4.    Likewise, the test case must not modify GPR14 (or it must save and
      restore it).

5.    The `*Gpr` directive to verify an address must add the comment
      `#address` so that the value can be rebased.

The following applies to `.tst` scripts that are generated from assembler
source:

6.    When the script is finished, it should first check whether GPR14
      is nonzero and branch back rather than load a disabled PSW.

The following applies when `cvttst.rexx` generates an object deck from a
hand-built `.tst` script:

7.    Loading a disabled PSW is replaced by BR 14, so there is no reason
      to write dual path code in hand-built .tst scripts.

8.    Loading control register 0 is suppressed, but the driver notes the
      fact that the AFPR enable bit is on and turns it on for the
      duration of the test.

9.    The test program must not reference storage beyond the highest
      address specified in any `r` console command (either loading data or
      displaying it).  The highest address is set as the control section
      size.

Here is an example of how to code a dual-path return in an assembler
program:

```
ltr 14,14     Have a return address?
 bnzr 14       return if so
 LPSWE WAITPSW     Load wait PSW
WAITPSW dc x'0002000180000000',ad(0) OK wait state PSW
```

Some programs contain a series of tests where the CPU is stopped after
each test.  This should be emulated by a branch back using GPR14, having
stored the resume address in the SVC old PSW.  This applies only to
tests that are assembled from assembler source; `cvttst.rexx` knows how to
deal with loading a disabled PSW.

```
swap equ * entry by bas 9,swap
 ltr 14,14  Running on CMS?
 jz bare
* Under z/CMS in 31-bit mode.  Simulate a SVC 255
 la 10,0(,9)                  Strip amode bit
 st 10,svcold+12
 basr 14,14                   Return to driver
 br 9                         Return to caller
bare equ *
 svc 255  We are in problem state
 br 9
```

The test cases are TEXT decks on CMS.  When the `.tst` script is built from
assembler source, the object deck is a by product; it should be uploaded
to CMS without change.  The test case is loaded at a page boundary by CMS.

The Rexx script `cvttst.rexx` builds an object deck from a `.tst` script
that is built by hand (notably the ieee floating point test cases).
This process makes several changes to the program:

1.    References to the PSA are rebased to use GPR15 as a base register.
      For `LA`, this is done only when the displacement is above the first
      instruction in the program (which is obtained from the restart new
      PSW field).

2.    A `LPSW[E]` for a disabled wait is replaced by `BR 14` to return.

3.    `LCTL` to enable advanced floating point is replaced by a no
      operation and a directive is emitted to have the test driver
      enable this flag and disable it again after the tests have all
      been run.

4.    The addresses in the various new PSW fields in the PSA are marked
      as relocatable when the PSW is not a disabled wait.  This will
      relocate the restart new PSW and make sure that the test case
      fires up the right place.  For other new PSWs this is of little
      practical application since the real interrupts will be fielded by
      CMS.

5.    Distinguishing between an instruction and a constant is in general
      an untractable problem, but it is assumed that there is one `r`
      command for each instruction and for each constant.  Clearly, a
      length that is not one of 2, 4, 6 must refer to a constant.  If
      the first two bits of the data are not congruent with the length
      of the area, it must be a constant.  Further, data where the first
      byte is either '00'x or 'ff'x is also a constant.

      This leaves negative short precision constants.  If the displacement
      part of the value is less than '200'x it is taken as a constant.
      I'll likely have to invent better algorithms.

`cvttst` is still very much experimental.
