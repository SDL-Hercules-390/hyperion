# Telnet
### Simple RFC-compliant TELNET implementation
#### v1.0.0

## Overview

libtelnet is a library for handling the TELNET protocol.  It includes routines for parsing incoming data from a remote peer as well as formatting data to be sent to the remote peer.

libtelnet uses a callback-oriented API, allowing application-specific handling of various events.  The callback system is also used for buffering outgoing protocol data, allowing the application to maintain control of the actual socket connection.

Features supported include the full TELNET protocol, Q-method option negotiation, and NEW-ENVIRON.

## Application Programming Interface (API)

Refer to the provided README.txt document for detailed programming information.

## Relevant standards

  * [RFC854  Telnet Protocol Specification](http://www.rfc-editor.org/info/rfc854)

  * [RFC855  Telnet Option Specifications](http://www.rfc-editor.org/info/rfc855)

  * [RFC1091 Telnet terminal-type option](http://www.rfc-editor.org/info/rfc1091)

  * [RFC1143 The Q Method of Implementing TELNET Option Negotiation](http://www.rfc-editor.org/info/rfc1143)

  * [RFC1572 Telnet Environment Option](http://www.rfc-editor.org/info/rfc1572)

  * [RFC1571 Telnet Environment Option Interoperability Issues](http://www.rfc-editor.org/info/rfc1571)

# LICENSE

The author or authors of this code dedicate any and all copyright interest in this code to the public domain. We make this dedication for the benefit of the public at large and to the detriment of our heirs and successors. We intend this dedication to be an overt act of relinquishment in perpetuity of all present and future rights to this code under copyright law.

-------------------------------------------------------------------------------

# Building Telnet

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

The telnet project is hosted on GitHub at the following URL:

  * [https://github.com/sdl-hercules-390/telnet](https://github.com/sdl-hercules-390/telnet)

You can either clone the git repository (recommended) or download the source code .zip file from github.  (click the green "Clone or download" button and select "Download")

Once downloaded, create a build directory (*outside* the source/repostiory directory) where the build will take place, called:

```
    xxxxxxxxAA.yyyyyyy
```

where "xxxxxxxx" is the name of the project/package (e.g. "telnet"), "AA" is the build architecture ("32" or "64") and "yyyyyyy" is the configuration type ("Debug" or "Release"):


```
    mkdir  telnet32.Debug
    mkdir  telnet32.Release
    mkdir  telnet64.Debug
    mkdir  telnet64.Release
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

To use the automated build method all you need to do is first create a WORK directory *outside* of (but at the same level as) the package's source code (i.e. repository) directory, with the name of the package (e.g. telnet).

Then, from within that WORK directory, simply invoke the package's **build** script:

```
  ..\telnet-repo\build --pkgname . --all --install ..\hyperion\telnet
```

This will build the package (within your WORK directory) and automatically install the results into the given hercules external package subdirectory. Then simply build hercules like normal.

Enter `build --help` and/or refer to the appropriate Hercules documentation for more information regarding building Hercules external packages.

-------------------------------------------------------------------------------

## Visual Studio GUI Method

To make things _EVEN SIMPLER_ on Windows (but with one minor snag), simply open the provided Visual Studio solution (.sln) file and then click the "Rebuild All" toolbar button.

Visual Studio will automatically invoke the appropriate `build.cmd` for you for whichever platform and configuration that you've selected in its Build toolbar's list boxes.

Like the previous manual Command Line method, this method presumes you've created an appropriately named WORK directory at the same level with the same name as the Project ("telnet" in this case).

The only snag (gotcha) is the package gets installed into a subdirectory within the WORK directory and not directly into your desired Hercules directory.  As a result, once you've built all of your architecture and configuration combinations, you will have to manually copy the results to your Hercules's package subdirectory.  This is because Visual Studio does not know ahead of time what Hercules directory you want to install into (so it instead it does the install to your WORK directory).

If you are experienced with Visual Studio however, you can manually edit the "Build Command Line", "Rebuild All Command Line" and "Clean Command Line" project properties with your desired --install directory so that a Solution Build will then automatically install the package into your desired Hercules subdirectory, eliminating the need for the manual copy.

-------------------------------------------------------------------------------

## Non-x86 Architectures

To build a non-x86 architectural version of an external package simply build it like normal but install it within its own unique architectural directory within Hercules's main external package subdirectory.  That is to say, you should specify e.g. `hyperion/telnet/sparc` as your `--install` directory.

Hercules currently understands the following foreign architectures: arm, mips, ppc, sparc, xscale and unknown.

-------------------------------------------------------------------------------
