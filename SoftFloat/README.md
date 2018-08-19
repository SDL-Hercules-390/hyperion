# SoftFloat
### Berkeley IEEE Binary Floating-Point Library
#### version 3.1.0

## 1. Introduction

Berkeley SoftFloat is a software implementation of binary floating-point that conforms to the IEEE Standard for Floating-Point Arithmetic. The current release supports four binary formats: 32-bit single-precision, 64-bit double-precision, 80-bit double-extended-precision, and 128-bit quadruple-precision. The following functions are supported for each format:

* addition, subtraction, multiplication, division, and square root;
* fused multiply-add as defined by the IEEE Standard, except for 80-bit double-extended-precision;
* remainder as defined by the IEEE Standard;
* round to integral value;
* comparisons;
* conversions to/from other supported formats; and
* conversions to/from 32-bit and 64-bit integers, signed and unsigned.

All operations required by the original 1985 version of the IEEE Floating-Point Standard are implemented, except for conversions to and from decimal.
This document gives information about the types defined and the routines implemented by SoftFloat. It does not attempt to define or explain the IEEE Floating-Point Standard. Information about the standard is available elsewhere.

The current version of SoftFloat is Release 3a. The only difference between this version and the previous Release 3 is the replacement of the license text supplied by the University of California.

The functional interface of SoftFloat Release 3 and afterward differs in many details from that of earlier releases. For specifics of these differences, see section 9 below, _Changes from SoftFloat Release 2._

## 2. Limitations

SoftFloat assumes the computer has an addressable byte size of 8 or 16 bits. (Nearly all computers in use today have 8-bit bytes.)

SoftFloat is written in C and is designed to work with other C code. The C compiler used must conform at a minimum to the 1989 ANSI standard for the C language (same as the 1990 ISO standard) and must in addition support basic arithmetic on 64-bit integers. Earlier releases of SoftFloat included implementations of 32-bit single-precision and 64-bit double-precision floating-point that did not require 64-bit integers, but this option is not supported starting with Release 3. Since 1999, ISO standards for C have mandated compiler support for 64-bit integers. A compiler conforming to the 1999 C Standard or later is recommended but not strictly required.

Most operations not required by the original 1985 version of the IEEE Floating-Point Standard but added in the 2008 version are not yet supported in SoftFloat Release 3a.

## 3. Acknowledgments

The SoftFloat package was written by me, **John R. Hauser**. Release 3 of SoftFloat was a completely new implementation supplanting earlier releases. The project to create Release 3 (and now 3a) was done in the employ of the University of California, Berkeley, within the Department of Electrical Engineering and Computer Sciences, first for the Parallel Computing Laboratory (Par Lab) and then for the ASPIRE Lab. The work was officially overseen by Prof. Krste Asanovic, with funding provided by these sources:

  <dl>
  <dt>Par Lab:<dd>Microsoft (Award #024263), Intel (Award #024894), and U.C. Discovery (Award #DIG07-10227), with additional support from Par Lab affiliates Nokia, NVIDIA, Oracle, and Samsung.
  <dt>ASPIRE Lab:<dd>DARPA PERFECT program (Award #HR0011-12-2-0016), with additional support from ASPIRE industrial sponsor Intel and ASPIRE affiliates Google, Nokia, NVIDIA, Oracle, and Samsung.
  </dl>

## 4. LICENSE

The following applies to the whole of SoftFloat Release 3a as well as to each source file individually.

Copyright 2011, 2012, 2013, 2014, 2015 The Regents of the University of California. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this list of conditions, and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions, and the following disclaimer in the documentation and/or other materials provided with the distribution.

  3. Neither the name of the University nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS", AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

-------------------------------------------------------------------------------

# Building SoftFloat

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

The SoftFloat project is hosted on GitHub at the following URL:

  * [https://github.com/sdl-hercules-390/SoftFloat](https://github.com/sdl-hercules-390/SoftFloat)

You can either clone the git repository (recommended) or download the source code .zip file from github.  (click the green "Clone or download" button and select "Download")

Once downloaded, create a build directory (*outside* the source/repostiory directory) where the build will take place, called:

```
    xxxxxxxxAA.yyyyyyy
```

where "xxxxxxxx" is the name of the project/package (e.g. "SoftFloat"), "AA" is the build architecture ("32" or "64") and "yyyyyyy" is the configuration type ("Debug" or "Release"):


```
    mkdir  SoftFloat32.Debug
    mkdir  SoftFloat32.Release
    mkdir  SoftFloat64.Debug
    mkdir  SoftFloat64.Release
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

To use the automated build method all you need to do is first create a WORK directory *outside* of (but at the same level as) the package's source code (i.e. repository) directory, with the name of the package (e.g. SoftFloat).

Then, from within that WORK directory, simply invoke the package's **build** script:

```
  ..\SoftFloat-repo\build --pkgname . --all --install ..\hyperion\SoftFloat
```

This will build the package (within your WORK directory) and automatically install the results into the given hercules external package subdirectory. Then simply build hercules like normal.

Enter `build --help` and/or refer to the appropriate Hercules documentation for more information regarding building Hercules external packages.

-------------------------------------------------------------------------------

## Visual Studio GUI Method

To make things _EVEN SIMPLER_ on Windows (but with one minor snag), simply open the provided Visual Studio solution (.sln) file and then click the "Rebuild All" toolbar button.

Visual Studio will automatically invoke the appropriate `build.cmd` for you for whichever platform and configuration that you've selected in its Build toolbar's list boxes.

Like the previous manual Command Line method, this method presumes you've created an appropriately named WORK directory at the same level with the same name as the Project ("SoftFloat" in this case).

The only snag (gotcha) is the package gets installed into a subdirectory within the WORK directory and not directly into your desired Hercules directory.  As a result, once you've built all of your architecture and configuration combinations, you will have to manually copy the results to your Hercules's package subdirectory.  This is because Visual Studio does not know ahead of time what Hercules directory you want to install into (so it instead it does the install to your WORK directory).

If you are experienced with Visual Studio however, you can manually edit the "Build Command Line", "Rebuild All Command Line" and "Clean Command Line" project properties with your desired --install directory so that a Solution Build will then automatically install the package into your desired Hercules subdirectory, eliminating the need for the manual copy.

-------------------------------------------------------------------------------

## Non-x86 Architectures

To build a non-x86 architectural version of an external package simply build it like normal but install it within its own unique architectural directory within Hercules's main external package subdirectory.  That is to say, you should specify e.g. `hyperion/SoftFloat/sparc` as your `--install` directory.

Hercules currently understands the following foreign architectures: arm, mips, ppc, sparc, xscale and unknown.

-------------------------------------------------------------------------------
