![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# HQA Build Scenarios (Hercules Quality Assurance)

## Contents

1. [Introduction](#Introduction)
2. [How to use HQA](#How-to-use-HQA)
3. [Creating new build scenarios](#Creating-new-build-scenarios)
4. [Samples](#Samples)

## Introduction

The "Hercules Quality Assurance" Build Scenarios feature is designed to make it easier for Hercules developers to build Hercules using different predefined build settings called "Build Scenarios". Each predefined Build Scenario is defined in the [hqadefs.h](../hqadefs.h) header which is automatically #included by the [hqainc.h](../hqainc.h) header whenever the `HAVE_HQA_H` macros is defined at build time.

The `HQA_H` header (also #included by the [hqainc.h](../hqainc.h) header when `HAVE_HQA_H` is defined at build time) selects which of the predefined [hqadefs.h](../hqadefs.h) build scenarios you wish to test (build). The `hqa.h` header is a header that you, the Hercules developer, must supply. Hercules does not come with one.

The idea is to be able to quickly and easily build various different "builds" of Hercules (each one using a different set of build options (i.e. each one with a different set of `#define OPTION_XXX...` options defined)) without having to modify the Hercules source code at all. That is to say, The HQA (Hercules Quality Assurance) feature allows the developer to quickly and easily test different predefined "builds" of Hercules without having to manually modify the Hercules source code.

## How to use HQA

To use the HQA feature simply define a `HQA_DIR` environment variable with the value of the directory where your `hqa.h` header exists and then rebuild Hercules.

To make things easy for you, the Windows and non-Windows versions of Hercules have already been modified to accept new build parameters to easily select whichever HQA Build Scenario you may wish to choose.

On Windows, just pass the `-hqa {directory}` option to [makefile.bat](../makefile.bat). On \*nix, you can activate an HQA build in one of several ways:

  -  You can define the `HQA_DIR` environment variable pointing to the directory containing your `hqa.h` header.
  -  You can prefix your `./configure` command with an appropriate `HQA_DIR=/path/to/my/hqadir` value and let the configure script do the rest.
  -  You can use the `--enable-hqa=/path/to/my/hqadir` configure option (which overrides your `HQA_DIR` environment variable if defined)

Your `hqa.h` header should then select the desired Build Scenario by appropriately `#defining HQA_SCENARIO` to the proper value. Refer to the [hqadefs.h](../hqadefs.h) header for the list of currently defined build scenarios.

When Hercules is built, it checks to see if the `HQA_DIR` variable is defined and if so, checks further to see whether it contains a file called `hqa.h` or not. If both conditions are true, then it defines the `HAVE_HQA_H` macro which causes the [hqainc.h](../hqainc.h) header to automatically #include your `hqa.h` header when Hercules is built.

Your `hqa.h` header then defines `HQA_SCENARIO` to the appropriate value to automatically select the proper build settings (called "Build Scenarios") within the [hqadefs.h](../hqadefs.h) header.

Also note that while your HQA directory, if defined, must exist, the `hqa.h` header file does not. That is to say, it is not an error for the `hqa.h` header to not exist. It is only an error for the HQA directory, if defined, to not exist. Thus you can have a permanently defined HQA directory and only create an `hqa.h` header file within it whenever you need to.

## Creating new build scenarios

If none of the predefined set of Build Scenarios within the [hqadefs.h](../hqadefs.h) header suits your needs, then just #define whatever OPTION_XXXX build settings you need within your `hqa.h` header instead, leaving `HQA_SCENARIO` undefined or set to 0.

Once your new build settings have been fully tested (not only to build properly but also to run properly too!) you are then strongly encouraged add your new set of build settings to the [hqadefs.h](../hqadefs.h) header as a new `HQA_SCENARIO` Build Scenario so other developers may take advantage of it.

## Samples
### SAMPLE "hqa.h" HEADER

```C
  ////////////////////////////////////////////////////////////////////////////
  //    Hercules Quality Assurance Build Configuration Testing Scenarios
  ////////////////////////////////////////////////////////////////////////////

  #ifndef _HQA_H_
  #define _HQA_H_

  //#define HQA_SCENARIO  0   // (normal build)
  //#define HQA_SCENARIO  1   // System/370 support only
  //#define HQA_SCENARIO  2   // ESA/390 support only
  //#define HQA_SCENARIO  3   // System/370 and ESA/390 support only
  //#define HQA_SCENARIO  4   // zArchitecure and ESA/390 support only
  //#define HQA_SCENARIO  5   // Windows, fthreads
  //#define HQA_SCENARIO  6   // Windows, Posix threads
  //#define HQA_SCENARIO  7   // Vista, fthreads
  //#define HQA_SCENARIO  8   // Vista, Posix threads
  //#define HQA_SCENARIO  9   // INLINE == forced inline
  //#define HQA_SCENARIO  10  // INLINE == inline (just a suggestion)
  //#define HQA_SCENARIO  11  // INLINE == null (compiler decides on own)
  //#define HQA_SCENARIO  12  // With Shared Devices,    With Syncio
  //#define HQA_SCENARIO  13  // Without Shared Devices, With Syncio
  //#define HQA_SCENARIO  14  // With Shared Devices,    Without Syncio
  //#define HQA_SCENARIO  15  // Without Shared Devices, Without Syncio
  //#define HQA_SCENARIO  16  // Without Object Rexx
  //#define HQA_SCENARIO  17  // Without Regina Rexx
  //#define HQA_SCENARIO  18  // Without Object Rexx, Without Regina Rexx
  //#define HQA_SCENARIO  19  // Without TCP keepalive support
  //#define HQA_SCENARIO  20  // With Basic TCP keepalive support
  //#define HQA_SCENARIO  21  // With Partial TCP keepalive support
  //#define HQA_SCENARIO  22  // With Full TCP keepalive support

  #endif /*_HQA_H_*/
```


### SAMPLE "HQA.msvc" NMAKE "!INCLUDE" file

```C
  #----------------------------------------------------------------------
  #  Hercules Quality Assurance Testing msvc build settings overrides
  #----------------------------------------------------------------------

  #----------------------------------------------------------------------
  !IF 0   #             --- EXAMPLES ---
  #----------------------------------------------------------------------

  # You can display custom messages that appear near
  # the very beginning of the build log if you want...

  !MESSAGE  *
  !MESSAGE  ***   My custom BUILD message    ***
  !MESSAGE  *


  # Normally you should NEVER override the VERSION string
  # since it's important to be able to identify what git
  # version your modifications are based on (if any)...

  !IF 0
  VERSION = \"My completely overridden VERSION string\"
  !ENDIF


  # But you MAY instead provide an additional custom build
  # string to identify your own specific custom build...

  CUSTOM_BUILD_STRING = "***  My custom BUILD string  ***"


  # You can also override actual compiler settings used
  # during the build too. To illustrate, instead of any
  # warning aborting the build, let's make all warnings
  # be actual warnings by removing the "/WX" option...

  cflags  = $(cflags:/WX=)       # (in case /WX used)
  cflags  = $(cflags:-WX=)       # (in case -WX used)

  #----------------------------------------------------------------------
  !ENDIF
  #----------------------------------------------------------------------


  # FishTest: maximum 3MB stack in 1MB increments
  ##conlflags = $(conlflags) /STACK:3145728,1048576


  # FishTest: Disable use of pre-compiler headers
  ##use_pch_opt =

```

"Fish" (David B. Trout)  
January 2015
