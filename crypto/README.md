# Crypto
### Simple AES/DES encryption and SHA1/SHA2 hashing library
#### version 1.0.0

## Overview

Crypto provides a simple implementation of the Rijndael (now AES) and DES encryption algorithms as well as the SHA1 and SHA2 hashing algorithms. The library is almost a verbatim copy of the code from OpenBSD and PuTTY:


  * [OpenBSD respository source code](https://github.com/openbsd/src/tree/master/sys/crypto)
  * [PuTTY repository source code](https://git.tartarus.org/?p=simon/putty.git)


## Relevant standards

For more information regarding the cryptographic standards provided by the crypto library, please visit the following websites:


  * [Advanced Encryption Standard](https://en.wikipedia.org/wiki/Advanced_Encryption_Standard)
  * [Data Encryption Standard](https://en.wikipedia.org/wiki/Data_Encryption_Standard)
  * [SHA-1](https://en.wikipedia.org/wiki/SHA-1)
  * [SHA-2](https://en.wikipedia.org/wiki/SHA-2)


-------------------------------------------------------------------------------

# LICENSE

For licensing information please refer to the LICENSE.txt document:

```
    /*-------------------------------------------------------------------*/
    /*                         Crypto LICENSE                            */
    /*-------------------------------------------------------------------*/
    
    rijndael:
    
     * This code is hereby placed in the public domain.
     *
     * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
     * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
     * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
     * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
     * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
     * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
     * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
     * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
     * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
     * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
     * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    
    sshdes:
    
     * PuTTY is copyright 1997-2005 Simon Tatham.
     *
     * Portions copyright Robert de Bath, Joris van Rantwijk, Delian
     * Delchev, Andreas Schultz, Jeroen Massar, Wez Furlong, Nicolas Barry,
     * Justin Bradford, Ben Harris, Malcolm Smith, Ahmad Khalifa, Markus
     * Kuhn, and CORE SDI S.A.
     *
     * Permission is hereby granted, free of charge, to any person
     * obtaining a copy of this software and associated documentation files
     * (the "Software"), to deal in the Software without restriction,
     * including without limitation the rights to use, copy, modify, merge,
     * publish, distribute, sublicense, and/or sell copies of the Software,
     * and to permit persons to whom the Software is furnished to do so,
     * subject to the following conditions:
     *
     * The above copyright notice and this permission notice shall be
     * included in all copies or substantial portions of the Software.
     *
     * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
     * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
     * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
     * NONINFRINGEMENT.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE
     * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
     * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
     * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
    
    sha1:
    
    /*
     * SHA-1 in C
     * By Steve Reid <steve@edmweb.com>
     * 100% Public Domain
     *
     * Test Vectors (from FIPS PUB 180-1)
     * "abc"
     *   A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
     * "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
     *   84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
     * A million repetitions of "a"
     *   34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
    */
    
    sha2:
    
     * Copyright (c) 2000-2001, Aaron D. Gifford
     * All rights reserved.
     *
     * Redistribution and use in source and binary forms, with or without
     * modification, are permitted provided that the following conditions
     * are met:
     * 1. Redistributions of source code must retain the above copyright
     *    notice, this list of conditions and the following disclaimer.
     * 2. Redistributions in binary form must reproduce the above copyright
     *    notice, this list of conditions and the following disclaimer in the
     *    documentation and/or other materials provided with the distribution.
     * 3. Neither the name of the copyright holder nor the names of contributors
     *    may be used to endorse or promote products derived from this software
     *    without specific prior written permission.
     *
     * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
     * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
     * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
     * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
     * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
     * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
     * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
     * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
     * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
     * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
     * SUCH DAMAGE.
```

-------------------------------------------------------------------------------

# Building Crypto

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

The crypto project is hosted on GitHub at the following URL:

  * [https://github.com/sdl-hercules-390/crypto](https://github.com/sdl-hercules-390/crypto)

You can either clone the git repository (recommended) or download the source code .zip file from github.  (click the green "Clone or download" button and select "Download")

Once downloaded, create a build directory (*outside* the source/repostiory directory) where the build will take place, called:

```
    xxxxxxxxAA.yyyyyyy
```

where "xxxxxxxx" is the name of the project/package (e.g. "crypto"), "AA" is the build architecture ("32" or "64") and "yyyyyyy" is the configuration type ("Debug" or "Release"):


```
    mkdir  crypto32.Debug
    mkdir  crypto32.Release
    mkdir  crypto64.Debug
    mkdir  crypto64.Release
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

To use the automated build method all you need to do is first create a WORK directory *outside* of (but at the same level as) the package's source code (i.e. repository) directory, with the name of the package (e.g. crypto).

Then, from within that WORK directory, simply invoke the package's **build** script:

```
  ..\crypto-repo\build --pkgname . --all --install ..\hyperion\crypto
```

This will build the package (within your WORK directory) and automatically install the results into the given hercules external package subdirectory. Then simply build hercules like normal.

Enter `build --help` and/or refer to the appropriate Hercules documentation for more information regarding building Hercules external packages.

-------------------------------------------------------------------------------

## Visual Studio GUI Method

To make things _EVEN SIMPLER_ on Windows (but with one minor snag), simply open the provided Visual Studio solution (.sln) file and then click the "Rebuild All" toolbar button.

Visual Studio will automatically invoke the appropriate `build.cmd` for you for whichever platform and configuration that you've selected in its Build toolbar's list boxes.

Like the previous manual Command Line method, this method presumes you've created an appropriately named WORK directory at the same level with the same name as the Project ("crypto" in this case).

The only snag (gotcha) is the package gets installed into a subdirectory within the WORK directory and not directly into your desired Hercules directory.  As a result, once you've built all of your architecture and configuration combinations, you will have to manually copy the results to your Hercules's package subdirectory.  This is because Visual Studio does not know ahead of time what Hercules directory you want to install into (so it instead it does the install to your WORK directory).

If you are experienced with Visual Studio however, you can manually edit the "Build Command Line", "Rebuild All Command Line" and "Clean Command Line" project properties with your desired --install directory so that a Solution Build will then automatically install the package into your desired Hercules subdirectory, eliminating the need for the manual copy.

-------------------------------------------------------------------------------

## Non-x86 Architectures

To build a non-x86 architectural version of an external package simply build it like normal but install it within its own unique architectural directory within Hercules's main external package subdirectory.  That is to say, you should specify e.g. `hyperion/crypto/sparc` as your `--install` directory.

Hercules currently understands the following foreign architectures: arm, mips, ppc, sparc, xscale and unknown.

-------------------------------------------------------------------------------
