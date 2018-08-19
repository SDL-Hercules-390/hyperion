# decNumber
### ANSI C General Decimal Arithmetic Library
#### version 3.68.0

## Overview

The decNumber library implements the General Decimal Arithmetic Specification in ANSI C. This specification defines a decimal arithmetic which meets the requirements of commercial, financial, and human-oriented applications. It also matches the decimal arithmetic in the IEEE 754 Standard for Floating Point Arithmetic.

The library fully implements the specification, and hence supports integer, fixed-point, and floating-point decimal numbers directly, including infinite, NaN (Not a Number), and subnormal values. Both arbitrary-precision and fixed-size representations are supported.

Refer to the package's PDF file for more information.

## Library structure

The library comprises several modules (corresponding to classes in an object- oriented implementation). Each module has a header file (for example, decNumber.h ) which defines its data structure, and a source file of the same name (e.g., decNumber.c ) which implements the operations on that data structure. These correspond to the instance variables and methods of an object-oriented design.

Refer to the package's PDF file for more information.

## Relevant standards

It is intended that, where applicable, functions provided in the decNumber package follow the requirements of:

* the decimal arithmetic requirements of IEEE 754 except that:

1. the IEEE remainder operator (decNumberRemainderNear) is restricted to those values where the intermediate integer can be represented in the current precision, because the conventional implementation of this operator would be very long-running for the range of numbers supported (up to +/- 10**1,000,000,000).

2. the mathematical functions in the decNumber module do not, in general, correspond to the recommended functions in IEEE 754 with the same or similar names; in particular, the power function has some different special cases, and most of the functions may be up to one unit wrong in the last place (note, however, that the squareroot function is correctly rounded)

* the floating-point decimal arithmetic defined in ANSI X3.274-1996 (including errata through 2001); note that this applies to functions in the decNumber module only, with appropriate context.

Please advise the author of any discrepancies with these standards.

Refer to the package's PDF file for more information.

# ICU License - ICU 1.8.1 and later

COPYRIGHT AND PERMISSION NOTICE

Copyright (c) 1995-2005 International Business Machines Corporation and others. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, provided that the above copyright notice(s) and this permission notice appear in all copies of the Software and that both the above copyright notice(s) and this permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder shall not be used in advertising or otherwise to promote the sale, use or other dealings in this Software without prior written authorization of the copyright holder.

----
All trademarks and registered trademarks mentioned herein are the property of their respective owners.

-------------------------------------------------------------------------------

# Building decNumber

## Prerequsites

**This package is built using CMake.**

CMake is an open-source, cross-platform family of tools designed to build, test and package software. CMake is used to control the software compilation process using simple platform and compiler independent configuration files, and generate native makefiles and workspaces that can be used in the compiler environment of your choice.

If you are already familiar with GNU autoconf tools, CMake basically replaces the `autogen.sh` and `configure` steps with a portable, platform independent way of creating makefiles that can be used to build your project/package with.

CMake can be downloaded and installed from:

  * [https://cmake.org/](https://cmake.org/)

-------------------------------------------------------------------------------

## CLANG

To build on Linux platforms using clang instead of gcc, simply export the CC and CXX flags before invoking CMake:


```
    $ export CC=clang
    $ export CXX=clang++
    $ cmake srcdir
```

If clang/clang++ is not in your $PATH, then you will need to specify its full path in the CC/CXX export commands.

-------------------------------------------------------------------------------

## Manual Command Line Method

The decNumber project is hosted on GitHub at the following URL:

  * [https://github.com/sdl-hercules-390/decNumber](https://github.com/sdl-hercules-390/decNumber)

You can either clone the git repository (recommended) or download the source code .zip file from github.  (click the green "Clone or download" button and select "Download")

Once downloaded, create a build directory (*outside* the source/repostiory directory) where the build will take place, called:

```
    xxxxxxxxAA.yyyyyyy
```

where "xxxxxxxx" is the name of the project/package (e.g. "decNumber"), "AA" is the build architecture ("32" or "64") and "yyyyyyy" is the configuration type ("Debug" or "Release"):


```
    mkdir  decNumber32.Debug
    mkdir  decNumber32.Release
    mkdir  decNumber64.Debug
    mkdir  decNumber64.Release
```

On Windows, "xxxxxxxxAA" will be used for the `INSTALL_PREFIX` (the name of the directory where the package will be installed to (target of make install). On Linux, the default `INSTALL_PREFIX` is `/usr/local`.  Either can be overridden by specifying the `-D INSTALL_PREFIX=instdir` option on your cmake command.

Once the binary/build directory is created, to build and install the package into the specified installation directory, simply use the commands:

**(Windows):**

```
    > mkdir <build-dir>
    > cd <build-dir>
    > srcdir\vstools [32|64]
    > cmake -G "NMake Makefiles" srcdir
    > nmake
    > nmake install
```

**(Linux):**

```
    $ mkdir <build-dir>
    $ cd <build-dir>
    $ cmake srcdir
    $ make
    # (sudo) make install
```

Where `srcdir` is the package's source (repository) directory where the `CMakeLists.txt` file exists.

The `vstools.cmd` batch file (Windows only) initializes the proper Microsoft Visual C/C++ build environment for the chosen build architecture (32 bit or 64 bit) and must be issued before the cmake and nmake commands.

To override the default installation directory use the -D INSTALL_PREFIX=xxx cmake option:

**(Windows):**

```
    > cmake -G "NMake Makefiles"  -D INSTALL_PREFIX=dirpath  srcdir
```

**(Linux):**

```
    $ cmake  -D INSTALL_PREFIX=dirpath  srcdir
```

Surround `INSTALL_PREFIX=dirpath` with double quotes if it contains blanks.

-------------------------------------------------------------------------------

## Automated Command Line Method

To make things _REALLY SIMPLE_ on both Windows and Linux you can instead use the provided **build** script (`build.cmd` on Windows, just `build` on Linux) which performs the complete build for you by automatically creating all of the needed build directories and then invoking all needed commands in turn to build and install the package for the given architecture/configuration.

To use the automated build method all you need to do is first create a WORK directory *outside* of (but at the same level as) the package's source code (i.e. repository) directory, with the name of the package (e.g. decNumber).

Then, from within that WORK directory, simply invoke the package's **build** script:

```
  ..\decNumber-repo\build --pkgname . --all --install ..\hyperion\decNumber
```

This will build the package (within your WORK directory) and automatically install the results into the given hercules external package subdirectory. Then simply build hercules like normal.

Enter `build --help` and/or refer to the appropriate Hercules documentation for more information regarding building Hercules external packages.

-------------------------------------------------------------------------------

## Visual Studio GUI Method

To make things _EVEN SIMPLER_ on Windows (but with one minor snag), simply open the provided Visual Studio solution (.sln) file and then click the "Rebuild All" toolbar button.

Visual Studio will automatically invoke the appropriate `build.cmd` for you for whichever platform and configuration that you've selected in its Build toolbar's list boxes.

Like the previous manual Command Line method, this method presumes you've created an appropriately named WORK directory at the same level with the same name as the Project ("decNumber" in this case).

The only snag (gotcha) is the package gets installed into a subdirectory within the WORK directory and not directly into your desired Hercules directory.  As a result, once you've built all of your architecture and configuration combinations, you will have to manually copy the results to your Hercules's package subdirectory.  This is because Visual Studio does not know ahead of time what Hercules directory you want to install into (so it instead it does the install to your WORK directory).

If you are experienced with Visual Studio however, you can manually edit the "Build Command Line", "Rebuild All Command Line" and "Clean Command Line" project properties with your desired --install directory so that a Solution Build will then automatically install the package into your desired Hercules subdirectory, eliminating the need for the manual copy.

-------------------------------------------------------------------------------

## Non-x86 Architectures

To build a non-x86 architectural version of an external package simply build it like normal but install it within its own unique architectural directory within Hercules's main external package subdirectory.  That is to say, you should specify e.g. `hyperion/decNumber/sparc` as your `--install` directory.

Hercules currently understands the following foreign architectures: arm, mips, ppc, sparc, xscale and unknown.

-------------------------------------------------------------------------------
