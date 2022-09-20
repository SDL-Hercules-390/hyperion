@if defined TRACEON (@echo on) else (@echo off)

  REM  If this batch file works, then it was written by Fish.
  REM  If it doesn't then I don't know who the heck wrote it.

  goto :BEGIN

::-----------------------------------------------------------------------------
::                            VSTOOLS.CMD
::-----------------------------------------------------------------------------
:HELP

  echo.
  echo     NAME
  echo.
  echo         %~n0   --   Initialize Visual Studio build environment
  echo.
  echo     SYNOPSIS
  echo.
  echo         %~nx0    { 32 ^| 64 ^| detect }
  echo.
  echo     ARGUMENTS
  echo.
  echo         32 ^| 64     Initializes the the desired Visual Studio
  echo                     32-bit or 64-bit target build environment.
  echo.
  echo         detect      Detects a Visual Studio installation and
  echo                     defines variables identifying which one.
  echo                     Refer to the below NOTES section.
  echo.
  echo     OPTIONS
  echo.
  echo         (NONE)
  echo.
  echo     NOTES
  echo.
  echo         Some flavor of Visual Studio must obviously be installed.
  echo         Supported versions are Visual Studio 2005 through 2022.
  echo.
  echo         The 'detect' call detects which version of Visual Studio
  echo         is installed and defines certain variables identifying
  echo         which version it was that was detected, but otherwise
  echo         does NOT initialize the build environment. It defines
  echo         some helpful (informative) variables ONLY.
  echo.
  echo         The '32' and '64' calls are the ones that initialize the
  echo         requested Visual Studio build environment. Note too that
  echo         both calls also set the same variables as a detect call
  echo         (i.e. an internal "detect" call is always performed).
  echo.
  echo         The variables which the 'detect' call returns are:
  echo.
  echo              vsname          Visual Studio's "short name"
  echo                              (e.g. "vs2019", "vs2008", etc)
  echo.
  echo              vsver           Visual Studio's numeric version
  echo                              (e.g. "150", "90", etc)
  echo.
  echo              vshost          The detected host architecture
  echo                              (either "x86" or "amd64")
  echo.
  echo              vstarget        The requested target architecture:
  echo                              "x86" if '32', "amd64" if '64'.
  echo.
  echo              vs2022..vs2005  The Visual Studio version numbers
  echo                              for each supported version of it
  echo                              (vs2019=160, vs2017=150, etc)
  echo.
  echo     EXIT STATUS
  echo.
  echo         0          Success
  echo         1          Error
  echo.
  echo     AUTHOR
  echo.
  echo         "Fish"  (David B. Trout)
  echo.
  echo     VERSION
  echo.
  echo         3.3     (January 26, 2022)

  endlocal
  exit /b 1


::-----------------------------------------------------------------------------
::                               BEGIN
::-----------------------------------------------------------------------------
:BEGIN

  :: Initialize the variables we'll be passing back to the caller.
  :: We do this BEFORE our "setlocal" to update THEIR variable pool.

  set "vs2005=80"
  set "vs2008=90"
  set "vs2010=100"
  set "vs2012=110"
  set "vs2013=120"
  set "vs2015=140"
  set "vs2017=150"
  set "vs2019=160"
  set "vs2022=170"

  set "VSNAMES=vs2022 vs2019 vs2017 vs2015 vs2013 vs2012 vs2010 vs2008 vs2005"

  set "vsname="
  set "vsver="
  set "vshost="
  set "vstarget="

  setlocal enabledelayedexpansion

  set "TRACE=if defined DEBUG echo"
  set "return=goto :EOF"

  :: Validate passed arguments...

  if /i "%~1" == ""        goto :HELP
  if /i "%~1" == "?"       goto :HELP
  if /i "%~1" == "/?"      goto :HELP
  if /i "%~1" == "-?"      goto :HELP
  if /i "%~1" == "--help"  goto :HELP
  if /i not "%~2" == ""    goto :HELP

  if     /i not "%~1" == "detect" (
    if   /i not "%~1" == "32"   (
      if /i not "%~1" == "64" (
        goto :HELP
      )
    )
  )

  :: Define target architecture...

  if "%~1" == "32" set "vstarget=x86"
  if "%~1" == "64" set "vstarget=amd64"

  call :detect_vstudio

  if %rc% NEQ 0 (
    endlocal
    exit /b 1
  )

  :: Go set the build environment if that's what they want

  if /i not "%~1" == "detect" (
    goto :SET_VSTUDIO_ENV
  )

  :: Otherwise all they're interested in are the variables

  endlocal                          ^
    && set "vsname=%vsname%"        ^
    && set "vsver=%vsver%"          ^
    && set "vshost=%vshost%"        ^
    && set "vstarget=%vstarget%"

  exit /b 0


::-----------------------------------------------------------------------------
::                          SET_VSTUDIO_ENV
::-----------------------------------------------------------------------------
:SET_VSTUDIO_ENV

  ::--------------------------------------------------------
  ::   Set the target Visual Studio build environment
  ::--------------------------------------------------------
  ::  Note that we must set the build enviroment OUTSIDE
  ::  the scope of our setlocal/endlocal to ensure that
  ::  whatever build environment gets set ends up being
  ::  passed back to the caller.
  ::--------------------------------------------------------

  endlocal                          ^
    && set "vsname=%vsname%"        ^
    && set "vsver=%vsver%"          ^
    && set "vshost=%vshost%"        ^
    && set "vstarget=%vstarget%"    ^
    && set "VCVARSDIR=%VCVARSDIR%"

  if %vsver% LSS %vs2017% (
    goto :vs2015_or_earlier
  )

  ::-----------------------------------
  ::   Visual Studio 2017 or LATER
  ::-----------------------------------

  pushd .
  echo.

  if /i "%vstarget%" == "x86"   call "%VCVARSDIR%\vcvars32.bat"
  if /i "%vstarget%" == "amd64" call "%VCVARSDIR%\vcvars64.bat"

  @if defined TRACEON (@echo on) else (@echo off)

  echo.
  popd
  exit /b 0

:vs2015_or_earlier

  ::-----------------------------------
  ::   Visual Studio 2015 or EARLIER
  ::-----------------------------------

  pushd .
  echo.

  call "%VCVARSDIR%\vcvarsall.bat"  %vstarget%

  @if defined TRACEON (@echo on) else (@echo off)

  echo.
  popd
  exit /b 0


::-----------------------------------------------------------------------------
::                          detect_vstudio
::-----------------------------------------------------------------------------
:detect_vstudio

  set "vsver="
  set "vsname="
  set "VCVARSDIR="
  set "rc=0"

  set "@VSNAMES=%VSNAMES%"

:detect_vstudio_loop

  for /f "tokens=1*" %%a in ("%@VSNAMES%") do (
    call :detect_vstudio_sub "%%a"
    if defined VCVARSDIR (
      goto :detect_vstudio_host
    )
    if "%%b" == "" (
      goto :detect_vstudio_error
    )
    set "@VSNAMES=%%b"
    goto :detect_vstudio_loop
  )

:detect_vstudio_error

  echo %~nx0: ERROR: No supported version of Visual Studio is installed
  set "vsver="
  set "vsname="
  set "VCVARSDIR="
  set "rc=1"
  %return%

:detect_vstudio_sub

  set "vsname=%~1"
  set "vsver=!%vsname%!"

  if "!VS%vsver%COMNTOOLS!" == "" (
    %return%
  )

  :: Remove trailing backslash if present

  set "VSCOMNTOOLS=!VS%vsver%COMNTOOLS!"

  if "%VSCOMNTOOLS:~-1%" == "\" (
    set "VSCOMNTOOLS=%VSCOMNTOOLS:~0,-1%"
  )

  :: Detect accidental typos in VSCOMNTOOLS path

  if not exist "%VSCOMNTOOLS%" (
    echo %~nx0: ERROR: Defined VS%vsver%COMNTOOLS directory does not exist!
    echo %~nx0: INFO: VS%vsver%COMNTOOLS = "%VSCOMNTOOLS%"??
    set "vsver="
    set "vsname="
    set "VCVARSDIR="
    set "rc=1"
    %return%
  )

  :: Define directory where "vcvarsxx.bat" files live

  set "VCVARSDIR=%VSCOMNTOOLS%\..\..\VC"

  if %vsver% GEQ %vs2017% (
    set "VCVARSDIR=%VCVARSDIR%\Auxiliary\Build"
  )

  %return%

:detect_vstudio_host

  if   /i "%PROCESSOR_ARCHITEW6432%" == "x86"   set "vshost=x86"
  if   /i "%PROCESSOR_ARCHITEW6432%" == "AMD64" set "vshost=amd64"
  if not defined vshost (
    if /i "%PROCESSOR_ARCHITECTURE%" == "x86"   set "vshost=x86"
    if /i "%PROCESSOR_ARCHITECTURE%" == "AMD64" set "vshost=amd64"
  )
  %return%

::-----------------------------------------------------------------------------
