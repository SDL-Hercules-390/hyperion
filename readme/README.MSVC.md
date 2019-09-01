![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# NOTE!  This file is OUT OF DATE and should probably be deleted!
### Not converted to markdown on that basis.

FOR MORE CURRENT UP-TO-DATE INFORMATION, PLEASE REFER TO THE [README.WIN64.md](./README.WIN64.md) DOCUMENT

## Original readme now follows:

```
----------------------------------------------------------------
  HOW TO BUILD HERCULES FOR WINDOWS WITH MICROSOFT VISUAL C
----------------------------------------------------------------

This file gives some hints on how you can build Hercules for
the Microsoft Windows environment using the Visual C/C++
compiler, without the need for Cygwin or Mingw.

-----------------------------------------------------------------------

           SETTING UP THE ENVIRONMENT VARIABLES


Run this .cmd file to set the PATH, INCLUDE, and LIB variables:

@echo off
path=%MSSdk%\bin\Win64;%PATH%
Call "%VCToolkitInstallDir%vcvars32.bat"
Call "%MSSdk%\setenv" /XP32 /RETAIL

Additional Note regarding environment variables: if you are using the
makefile.bat && makefile-dllmod.msvc to build Herc (see further below)
you may also wish to define the ZLIB_DIR and BZIP2_DIR variables in
order for Herc to support ZLIB && BZIP2 disk/tape compression. See also
the "Note Regarding ZLIB_DIR" and "Obtaining ZLIB" sections further
below (as well as the ones for BZIP2) for further information regarding
building Herc with compression support.


-----------------------------------------------------------------------

                    THE BUILD COMMAND


Before you can build Hercules, you must install the Visual C++ compiler
and the Windows SDK, as well as the msvcrt.lib file as explained below.
As an alternative, you may install Visual Studio (version 7 or above)

To build the MSVC version of Hercules:

nmake /f makefile-dllmod.msvc

The executable modules are placed in the msvc.dllmod.bin subdirectory.


     DOWNLOAD THE MICROSOFT VISUAL C++ COMPILER AND SDK

http://msdn.microsoft.com/visualc/vctoolkit2003/

The Microsoft Visual C++ Toolkit 2003 includes the command line
versions of the Visual C/C++ compiler and linker, as well as the
standard C/C++ Library.

http://www.microsoft.com/msdownload/platformsdk/sdkupdate/

The Windows Server 2003 Core SDK contains the include files and
libraries needed to create applications for Win95/98/ME/NT/2000/XP.

Notes:
1. You may need to add "download.microsoft.com" to the Internet
   Explorer trusted zone before you can install the SDK
2. You also need to download "Build Environment Intel 64 bit" to
   obtain the nmake utility in mssdk/bin/win64


-----------------------------------------------------------------------

             HOW TO OBTAIN THE MSVCRT.LIB FILE


If you want to build the DLL version of Hercules then you will
also need the Microsoft file msvcrt.lib which is not included in
the free Visual C++ Toolkit 2003. You can however obtain this
file by downloading and installing the
".NET Framework Version  1.1 Software Development Kit (SDK)"
which is available as a free download from:

http://msdn.microsoft.com/netframework/downloads/updates/default.aspx

Be aware, however, that this is a 108MB download, of which the only
directory you need is
c:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\lib
After installation, copy msvcrt.lib from this directory to
c:\Program Files\Microsoft Visual C++ Toolkit 2003\lib
You may also want to copy nmake.exe and msvcr71.dll from
C:\Program Files\Microsoft.NET\v1.1\Bin
after which you can delete the .NET Framework SDK.


-----------------------------------------------------------------------

               GCC EXTENSIONS TO BE AVOIDED


1. Variadic macro definitions, eg #define logmsg(format,parms...)
2. __attribute__((unused)) should be replaced by UNREFERENCED macro
3. void* pointer arithmetic, eg (void*)p + 1
4. __attribute__((packed))


-----------------------------------------------------------------------

               C99 EXTENSIONS TO BE AVOIDED


1. Field names in structure initialisers.

--rbowler

-----------------------------------------------------------------------

          How to build Hercules using Visual Studio IDE:


Please see Fish's detailed instructions at:

   http://www.softdevlabs.com/hercules-msvc-build.html


-----------------------------------------------------------------------

            *** NOTE REGARDING PRECOMPILED HEADERS ***


EACH AND EVERY source file must start with :

   #include "hstdinc.h"

These contain the system headers (those enclosed in <>) and that are not
dependent on the guest architecture.

This file, along with build_pch.c allows building a precompiled header file
that significantly speeds up the MSVC Windows build.

I REPEAT !!

EACH AND EVERY SOURCE FILE MUST START WITH THIS FILE !!

No macros (#define).. No nothing.

--Ivan

-----------------------------------------------------------------------

                *** NOTE REGARDING ZLIB_DIR ***


ZLIB_DIR defines the location of the ZLIB libraries
If ZLIB_DIR is undefined, then an attempt is made to locate the ZLIB
library in winbuild\zlib\win32_32
If ZLIB_DIR contains a bad path, nmake exits with an error EXCEPT if
ZLIB_DIR is set to the special keyword "NONE".
ZLIB_DIR should contain the *BASE* path for ZLIB. That means that
the following files are expected :

- $(ZLIB_DIR)\zlib1.dll
- $(ZLIB_DIR)\lib\zdll.lib
- $(ZLIB_DIR)\include\zlib.h
- $(ZLIB_DIR)\include\zconf.h

REMEMBER TO "nmake clean" if the build type is changed, the ZLIB location
modified, etc..

NOTE : The zlib1.dll Dynamic Link Library will be copied to the appropriate
build target directory as part of the build process in order to ensure that
the DLL is obtainable by the platform PE(*) loader at run time.

--Ivan

(*) PE stands for Program Executable and designates WIndows .EXE & .DLL
executable file formats.


                     *** Obtaining ZLIB ***


Building from the distributed hercules source tree does not by default
incorporate ZLIB as a compression mechanism, since

1) ZLIB is an external project, completelly separate from the hercules
   project itself.
2) MS Windows (tm) does not provide any well known location (if at all) for
   the ZLIB library runtime and/or header files.

.. Therefore ..

In order for HERCULES to be built with ZLIB support (DASD, TAPE), go
to http://www.zlib.net - and locate the download for the 'zlib compiled DLL'.
NOTE : This is a 75KB download.
Extract that ZIP file to the winbuild\zlib\win32_32 directory, relative to
the source directory.
You may extract the file to an alternate location, and then set the ZLIB_DIR
environment variable accordingly (as explained above).

The hercules project is currently known to succesfully build with
version 1.2.2 (current at the time of writing) of the ZLIB compression library.

ZLIB is a compression algorithm written by Jean Loup Gailly and Mark Adler.
ZLIB is used in the hercules project pursuant to the ZLIB License
(http://www.zlib.net/zlib_license.html).

In source form, the hercules project DOES NOT contain ZLIB.
In Binary form, the hercules project MAY contain an UNMODIFIED version of
    the ZLIB runtime.

--Ivan

-----------------------------------------------------------------------

                *** NOTE REGARDING BZIP2_DIR ***


BZIP2_DIR defines the location of the BZIP2 libraries. If BZIP2_DIR is undefined,
If BZIP2_DIR contains a bad path, nmake exits with an error EXCEPT if BZIP2_DIR
is set to the special keyword "NONE". BZIP2_DIR should contain the *BASE* path for
BZIP2. That means that the following files are expected:

  -  $(BZIP2_DIR)\bzlib.h
  -  $(BZIP2_DIR)\libbz2.lib
  -  $(BZIP2_DIR)\libbz2.dll

REMEMBER TO "nmake clean" if the build type is changed, the BZIP2_DIR location
modified, etc..

NOTE : The libbz2.dll Dynamic Link Library will be copied to the appropriate
build target directory as part of the build process in order to ensure that
the DLL is obtainable by the platform PE(*) loader at run time.

-- Fish (copied blatantly from Ivan's ZLIB documentation further above)

(*) PE stands for Program Executable and designates WIndows .EXE & .DLL
executable file formats.


                     *** Obtaining BZIP2 ***


Building from the distributed Hercules source tree does not by default
incorporate BZIP2 as a compression mechanism, since

1) BZIP2 is an external project, completelly separate from the Hercules
   project itself.
2) MS Windows (tm) does not provide any well known location (if at all) for
   the BZIP2 library runtime and/or header files.

.. Therefore ..

In order for HERCULES to be built with BZIP2 support (DASD, TAPE), go to
"http://www.bzip.org/downloads.html" and locate the hyperlink entitled:

  "Here is the 1.0.3 source tarball , which includes full documentation.
  md5: 8a716bebecb6e647d2e8a29ea5d8447f"

(Yep, that's right! You're going to have to build the BZIP2 dll for yourself
from source!) (either that or obtain a pre-built copy of it (and associated
header/lib files) from someone else you trust). NOTE: This is a 650KB download,
and the file is in tar.gz format, so you'll need WinZip or other utility to
unzip it.

Extract the files to a directory of your choosing and build the DLL using
the supplied makefile.msc. Once it's built, then copy the resulting DLL, lib,
and bzip2.h header file to your defined BZIP2_DIR location and rebuild Herc.

BZIP2 is a freely available (open-source (BSD-style) license), patent free
(as far as the author knows), high-quality data compressor
written by Julian R Seward. It typically compresses files to within 10% to
15% of the best available techniques (the PPM family of statistical compressors),
whilst being around twice as fast at compression and six times faster at decompression.

In source form, the Hercules project does not contain any BZIP2 source code at all.

In binary form however, the Hercules project may include an unmodified version
of the BZIP2 runtime DLL in addition to its own distribution binaries.

-- Fish (updated 2005-12-02)

-----------------------------------------------------------------------

                         MSVC AND CRYPTO


Bernard Van Der Helm's crypto modules (Message Security Assist 1 & 2 Feature)
is now built automatically as part of the MSVC build.

Should it be necessary to build a version *WITHOUT* the crypto DLL, it is
possible to instruct the makefile-dllmod.msvc file to bypass building that
particular code by defining the environment variable NOCRYPTO.

Nov 22nd 2005

--Ivan Warren

-----------------------------------------------------------------------

                ***  NOTE REGARDING PCRE_DIR  ***


The following is an excerpt from "makefile-dllmod.msvc"


# ---------------------------------------------------------------------
# To enable PCRE (Perl-Compatible Regular Expressions) support, first
# download 'Binaries'  and 'Developer files' packages from the GNUWin32
# PCRE website at: http://gnuwin32.sourceforge.net/packages/pcre.htm.
# Then create a permanent directory somewhere called whatever you want
# with a 'bin', 'lib' and 'include' subdirectory, and then define an
# environment variable called "PCRE_DIR" pointing to that directory.
# Finally, make sure the below names of the include (header), lib and
# DLL names are correct, making whatever adjustments may be necessary.
# ---------------------------------------------------------------------

!IFDEF PCRE_DIR
!IF "$(PCRE_DIR)" == "NONE"
!UNDEF PCRE_DIR
!ELSE

PCRE_INCNAME  = pcreposix.h
PCRE_LIBNAME1 = pcre.lib
PCRE_LIBNAME2 = pcreposix.lib
PCRE_DLLNAME1 = pcre3.dll
PCRE_DLLNAME2 = pcreposix3.dll
PCRE_INCDIR   = $(PCRE_DIR)\include
PCRE_INCPATH  = $(PCRE_DIR)\include\$(PCRE_INCNAME)
PCRE_LIBPATH1 = $(PCRE_DIR)\lib\$(PCRE_LIBNAME1)
PCRE_LIBPATH2 = $(PCRE_DIR)\lib\$(PCRE_LIBNAME2)
PCRE_DLLPATH1 = $(PCRE_DIR)\bin\$(PCRE_DLLNAME1)
PCRE_DLLPATH2 = $(PCRE_DIR)\bin\$(PCRE_DLLNAME2)


Sep 22 2006

-- Fish

-----------------------------------------------------------------------
```
