![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Building Hercules under 64-bit Windows
## Contents

1. [Command Line Method](#Command-Line-Method)
2. [The Fish "Visual Studio 2008" method](#The-Fish-Visual-Studio-2008-method)
3. [The Windows batch file method](#The-Windows-batch-file-method)

## About

There is only one 64-bit architecture supported by 64-bit Windows: the x64 architecture also known as x86_64 (for AMD64 processors).

## Command Line Method

1. Install [Visual Studio 2008 (VS9) Standard or Professional Edition](http://msdn.microsoft.com/en-us/evalcenter/bb633753.aspx).

A 90-day evaluation edition can be downloaded from: http://msdn.microsoft.com/en-us/evalcenter/bb633753.aspx.

2. &nbsp; **32-bit** Windows:

If compiling on a 32-bit Windows system, go to the start menu and choose "All Programs" -> "Microsoft Visual Studio 2008" -> "Visual Studio Tools" -> "Visual Studio 2008 x64 Cross Tools Command Prompt". Then, at the Cross Tools command prompt, enter this command: `SET CPU=AMD64`

3. &nbsp; **64-bit** Windows:

If compiling on a 64-bit Windows system, go to the start menu and choose "All Programs" -> "Microsoft Visual Studio 2008" -> "Visual Studio Tools" -> "Visual Studio 2008 x64 Win64 Command Prompt"

4. If you require gzip or bzip2 for disk or tape compression, or if you require PCRE for the Hercules Automatic Operator facility, you should install the AMD64 versions of these programs in `winbuild\zlib\x64`, `winbuild\bzip2\x64` and `winbuild\pcre\x64` under the Hercules directory.

You can override these default directory locations by simply setting the following environment variables:

```
SET ZLIB_DIR=C:\packages\zlib
SET BZIP2_DIR=C:\packages\bzip2
SET PCRE_DIR=C:\packages\pcre
```

5. Enter the commands:
```
nmake clean -f makefile-dllmod.msvc
nmake       -f makefile-dllmod.msvc
```

6. The binaries will be installed into subfolder `msvc.AMD64.bin`. If you compiled on a 32-bit Windows system, copy this folder to your target 64-bit Windows machine.

7. If you copy the binaries to a machine which does not have Visual Studio 2008 (VS9) installed, then you must also install the [Microsoft Visual C++ 2008 Redistributable Package (x64)](http://www.microsoft.com/downloads/details.aspx?FamilyID=bd2a6171-e2d6-4230-b809-9a8d7548c1b6) on the target machine.

## The Fish "Visual Studio 2008" method

(which accomplishes virtually the same thing as the above command-line build instructions)

1. Install [Visual Studio 2008](http://msdn.microsoft.com/en-us/evalcenter/bb633753.aspx).
  
Be sure to select compiler support for "x64" when installing Visual Studio since the Hercules provided Visual Studio Solution expects it.

2. Open the "**Hercules_VS2008.sln**" Solution file.
3. Select your desired Solution Configuration (**Debug** or **Release**) and Solution Platform (**Win32** or **x64**) from the appropriate dropdowns.
4. Click the "**Build Solution**" or "**Rebuild Solution**" toolbar button.

Note: Your 64-bit versions of the ZLIB, BZIP2, and/or PCRE development packages should be in an appropriate `x64` subdirectory beneath their normal package home directory. See the ZLIB_DIR, BZIP2_DIR, and/or PCRE_DIR sections of the [README.MSVC](./README.MSVC.md) document for details.

##  The Windows batch file method

The Hercules `makefile.bat` makes it trivially easy to build Hercules from the command-line on Windows.  In fact, the Visual Studio "project" files included with Hercules are setup to simply invoke the Hercules `makefile.bat` with default build parameters.  (see previous section immediately above)

What follows further below is the usage information that is displayed when you enter the `makefile.bat` command with no arguments.

Note: As explained in the previous sections, in order to build support for ZLIB and/or BZIP2 compressed dasd files and/or Regular Expression support, you need to also ensure your 64-bit versions of the ZLIB, BZIP2, and/or PCRE development packages should be in an appropriate `x64` subdirectory beneath their normal package home directory. See the ZLIB_DIR, BZIP2_DIR, and/or PCRE_DIR sections of the [README.MSVC](./README.MSVC.md) document for details.

### makefile.bat

Initializes the Windows software development build envionment and invokes nmake to build the desired 32 or 64-bit version of the Hercules emulator.

```
Format:

  makefile.bat  {build-type}  {makefile-name}  {num-cpu-engines}  \
                [-asm]                                            \
                [-title "custom build title"]                     \
                [-hqa {directory}]                                \
                [-extpkg {directory}]                             \
                [-a|clean]                                        \
                [{nmake-option}]

Where:

  {build-type}        The desired build configuration. Valid values are
                      DEBUG / RETAIL for building a 32-bit Hercules, or
                      DEBUG-X64 / RETAIL-X64 to build a 64-bit version
                      of Hercules targeting (favoring) AMD64 processors.

                      DEBUG builds activate/enable UNOPTIMIZED debugging
                      logic and are thus VERY slow and not recommended
                      for normal use. RETAIL builds on the other hand
                      are highly optimized and thus the recommended type
                      for normal every day ("production") use.

  {makefile-name}     The name of our makefile: 'makefile.msvc' (or some
                      other makefile name if you have a customized one)

  {num-cpu-engines}   The maximum number of emulated CPUs (NUMCPU=) you
                      want this build of Hercules to support: 1 to 64.

  -asm                To generate assembly (.cod) listings.

  -title "xxx..."     To define a custom title for this build.

  -hqa "directory"    To define the Hercules Quality Assurance directory
                      containing your optional "hqa.h" and/or "HQA.msvc"
                      build settings override files.

  -extpkg "directory" To define the base directory where the Hercules
                      External Packages are installed. Hercules will use
                      the 'include' and 'lib' subdirectories of this
                      directory to locate External Package header files
                      and lib files during the build process.  If not
                      specified the default is to use the header and lib
                      files that come with the Hercules repository.

  [-a|clean]          Use '-a' to perform a full rebuild of all Hercules
                      binaries, or 'clean' to delete all temporary work
                      files from all work/output directories, including
                      any/all previously built binaries. If not specified
                      then only those modules that need to be rebuilt are
                      actually rebuilt, usually resulting in much quicker
                      build. However, when doing a 'RETAIL' build it is
                      HIGHLY RECOMMENDED that you always specify the '-a'
                      option to ensure that a complete rebuild is done.

  [{nmake-option}]    Extra nmake option(s).   (e.g. -k, -g, etc...)
```
