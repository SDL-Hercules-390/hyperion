![test image](./images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Hercules "External Packages"
## Contents
1. [Overview](#Overview)
2. [Linux](#Linux)
3. [Building the External Packages for a Windows-based Hyperion](#Building-the-External-Packages-for-a-Windows-based-Hyperion)<br>
   3a. [Summary of the steps involved](#Summary-of-the-steps-involved)<br>
   3b. [Detailed Instructions](#Detailed-Instructions)<br>

## Overview
This version of Hercules 4.x Hyperion links with several "external package" static libraries.

Hercules comes distributed with pre-built external packages as part of its source code repository that were built, at the time of this writing, using the latest version of Visual Studio (2017) (or clang, etc, for Linux) that ideally _should_ work fine for most everyone as-is, thus eliminating the need to build any of them yourself. Just build Hercules normally and you're done.

In some situations however you might need (or want) to rebuild one or more of the external packages for your system.  Some (but certainly not all) of those situations include (but are not limited to) the following:

1. Developing or Debugging a new/existing version of an external package.
2. Building Hercules for a different version of Visual Studio.
3. Building Hercules for a different processor architecture or for an unusual non-mainstream \*Nix platform (Apple Mac, Raspberry Pi, etc).

Each package is maintained in its own unique source code repository separate from the Hercules emulator itself.  The Hercules 4.x Hyperion repository only hosts default package static libraries (along with a few accompanying files) in separate external package subdirectories.  Each package's source code is kept in its own separate source code repository.  The Hercules repository no longer contains any source code for these external packages.  The Hercules repository only contains default builds of each package's static link libraries and whatever header files the package requires to call into its functions.

Each external package is built separately from the Hercules emulator and then "installed" into a directory of your choosing.  On Windows, the `lib` sub-directoty of this external packages "installation directory" should then be specified in your **`LIB`** environment variable so that Hercules can then find your custom built external package static libraries. (On \*Nix the equivalent variable would be the **`LIBRARY_PATH`** variable.)  The same idea applies for the **`INCLUDE`** environment variable too (on \*Nix the equivalent variable would be the **`CPATH`** variable): it should point to the external package installation directory's "include" subdirectory.

Currently, there are four such external packages this Hercules 4.x Hyperion links with.  These packages are the **crypto**, **decNumber**, **SoftFloat**, and **telnet** packages.  Each of these package names correspond to an existing subdirectory of Hyperion.  Over time additional Hercules functionality will be moved out of the emulator into their own separately maintained and built external package repositories, eventually reducing the Hercules emulator to just its core functionality: accurate emulation of the System/370, ESA/390, and z architectures.

The four repositories for the external packages can be located at these URLs:  

* [https://github.com/sdl-hercules-390/crypto](https://github.com/sdl-hercules-390/crypto)
* [https://github.com/sdl-hercules-390/decNumber](https://github.com/sdl-hercules-390/decNumber)
* [https://github.com/sdl-hercules-390/SoftFloat](https://github.com/sdl-hercules-390/SoftFloat)
* [https://github.com/sdl-hercules-390/telnet](https://github.com/sdl-hercules-390/telnet)

The procedure for building each of the external packages is outlined further below.  More information about each of the packages can be found in a `README` document within each of their respective repositories.

## Linux
The procedure detailed below is explained from a Windows point of view, but the procedure for Linux is virtually identical.  If you are reasonably skilled at Linux you should have no trouble making the needed adjustments.

One of the adjustments that must be made is to either point your **`LIBRARY_PATH`** and **`CPATH`** variables to your `./extpkgs/lib` and `./extpkgs/include` directories or else specify the the top level `./extpkgs` directory for your `--enable-extpkgs=DIR` configure option.

## Building the External Packages for a Windows-based Hyperion
Building the packages is a fairly simple and straightforward process and the steps are outlined and then detailed below.  These steps presume you are using Windows and that you need both the x86 and x64 builds of the external packages.

### Summary of the steps involved
1.  Download and install CMake.
2.  Make a directory where all packages should be installed into.
3.  Download `ExtPkgs.cmd` and `ExtPkgs.cmd.ini` from GitHub and adjust `ExtPkgs.cmd.ini` for your system (e.g. `cpu = arm`).
4.  Use the `ExtPkgs` command to clone, build and install all of the external packages at once, with one simple command.
5.  Update your `LIB` and `INCLUDE` environment variables.
6.  Build Hercules as normal.

For the purposes of this example, it is presumed that you have downloaded the Hyperion repository and placed it into the `\hyperion` directory.  Then we will follow the process to build the external packages and have them all installed into the common `\extpkgs` installation directory.

### Detailed Instructions
1. Download and install CMake.  Cmake is a tool that is used to build the packages from their source repositories.  Download Cmake [here](https://cmake.org/).

Either download the .msi installer package or the .zip file and install Cmake.  It is simple and installs in seconds.

2. Make a directory where all packages should be installed into.  To keep things organized, create this directory at the same level as your primary Hercules directory.  This is only a recommendation, not a requirement.
```dos
      chdir  \hercules
      chdir  ..
      mkdir  \extpkgs
```

3. Download the "**ExtPkgs.cmd**" and "**ExtPkgs.cmd.ini**" files from the [GitHub "gists" repository](https://github.com/SDL-Hercules-390/gists).  You do not need to clone this repository.  You only need those two files.

Place these files into your local `bin` directory (or if you don't have such a directory, place them into your `\extpkgs` directory that you just created).

Adjust the "**ExtPkgs.cmd.ini**" as needed for your system (e.g. `cpu = arm`).

4.  Now clone and build the external packages by making the package install directory your current directory and issuing the `ExtPkgs`**(*)** command to clone, build and install all of the packages at once directly into your installation directory:  
<pre>
       chdir  \extpkgs
       ExtPkgs.cmd   CLONE   c  d  s  t        <i><b>(see footnote at end)</b></i>
</pre>

Depending on the speed of your system it may take anywhere from a couple of minutes to several minutes to finish building all of the packages.

5.  Update your **`LIB`** and **`INCLUDE`** environment variables (on Linux this would be your **`LIBRARY_PATH`** and **`CPATH`** variables) to point to the directory where your packages were just installed:  
```dos
        setx LIB "%LIB%;\extpkgs\lib"
        setx INCLUDE "%INCLUDE%;\extpkgs\include"
```

6.  Build Hercules as normal.  (self explanatory)

That concludes the necessary steps to build the external packages.  You now have the 32-bit and 64-bit versions of the Debug and Release builds of the external packages.  If you desire, you may delete both the `\repo` and `\work` directories as they are no longer needed.  You only need to keep the **`\extpkgs`** directory needed by Hercules (pointed to by your `LIB` environment variable).

If you clone the `\hyperion` directory or download a newer Hyperion repository in the future, there is nothing extra you need to do.  You do _**not**_ need to rebuild the external packages ever again (unless they are updated of course).

---------------

**(*)**  Use `extpkgs --help` (Linux) or `extpkgs /?` (Windows) to obtain more information regarding the extpkgs script's parameters.
