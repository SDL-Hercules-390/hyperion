@if defined TRACEON (@echo on) else (@echo off)

  REM   If this batch file works okay then it was written by Fish.
  REM   If it doesn't work then I don't know who the heck wrote it.

::*****************************************************************************
::*****************************************************************************
::*****************************************************************************
::***                                                                       ***
::***                         DYNMAKE.BAT                                   ***
::***                                                                       ***
::*****************************************************************************
::*****************************************************************************
::*****************************************************************************
::*                                                                           *
::*      Designed to be called either from the command line or by a           *
::*      Visual Studio makefile project with the "Build command line"         *
::*      set to: "dynmake.bat <arguments...>". See 'HELP' section             *
::*      further below for details regarding use.                             *
::*                                                                           *
::*****************************************************************************
::*                                                                           *
::*                          PROGRAMMING NOTE                                 *
::*                                                                           *
::*  All error messages *MUST* be issued in the following strict format:      *
::*                                                                           *
::*         echo %~nx0^(1^) : error C9999 : error-message-text...             *
::*                                                                           *
::*  in order for the Visual Studio IDE to detect it as a build error since   *
::*  exiting with a non-zero return code doesn't do the trick. Visual Studio  *
::*  apparently examines the message-text looking for error/warning strings.  *
::*                                                                           *
::*****************************************************************************

  setlocal
  pushd .
  set rc=0
  set "TRACE=if defined TRACEON echo"

  if "%~1" == ""        goto :help
  if "%~1" == "?"       goto :help
  if "%~1" == "/?"      goto :help
  if "%~1" == "-?"      goto :help
  if "%~1" == "--help"  goto :help

  set hercdir=%~dp0
  set projdir=%~1
  set modname=%~2
  set build_type=%~3
  set num_cpus=%~4
  set extra_nmake_args=%~5

:extra_args

  if "%~6" == "" goto :begin
  set extra_nmake_args=%extra_nmake_args% %~6
  shift /1
  goto :extra_args

::---------------------------------------------------------------------
::                              HELP
::---------------------------------------------------------------------
:help
  set rc=1
  echo.
  echo %~nx0^(1^) : error C9999 : Help information is as follows:
  echo.
  echo.
  echo.
  echo                             %~nx0
  echo.
  echo.
  echo  Build script for building a dynamically loadable Hercules plugin
  echo  module. This script performs some necessary pre-processing before
  echo  eventually invoking the main Hercules build script to build your
  echo  dynamic module.
  echo.
  echo.
  echo  Format:
  echo.
  echo.
  echo    %~nx0 {projdir} {modname} {build_type} {num_cpus} [-a^|clean]
  echo.
  echo.
  echo  Where:
  echo.
  echo    {projdir}       The fully qualified path of your dynamic module
  echo                    project directory. If the path contains spaces
  echo                    enclose it within double quotes.
  echo.
  echo    {modname}       The root filename of your dynamic module (e.g.
  echo                    "dyn75"). One word without any spaces in it.
  echo.
  echo    {build_type}    Desired build configuration. Valid values are
  echo                    DEBUG or RETAIL for building a 32-bit version,
  echo                    or DEBUG-X64/RETAIL-X64 for a 64-bit version.
  echo                    The same value is passed back to your optional
  echo                    prebuild.bat and or postbld.bat scripts as the
  echo                    second argument.
  echo.
  echo    {num_cpus}      The maximum number of emulated CPUs (NUMCPU=)
  echo                    your module supports: 1 to 64. Must be the same
  echo                    value that was used to build Hercules with.
  echo                    The same value is passed back to you as the
  echo                    3rd parameter to your optional prebuild.bat
  echo                    and/or postbld.bat scripts.
  echo.
  echo    [-a^|clean]      Either '-a' to perform a full rebuild of your
  echo                    module, or 'clean' to clean the output directory
  echo                    of all previously built binaries and work files.
  echo                    If not specified then your module is only built
  echo                    if it actually needs rebuilding (i.e. only if
  echo                    any of your source files changed for example).
  echo                    It is HIGHLY RECOMMENDED that you ALWAYS use
  echo                    '-a' for any/all "retail" type builds. These
  echo                    value(s) you specify here are passed back to
  echo                    your optional prebuild.bat and/or postbld.bat
  echo                    scripts as the last set of arguments (4 - n).
  echo.
  goto :exit

::---------------------------------------------------------------------
:begin

  echo %~nx0^(1^) : Building dynamic module "%modname%" from "%projdir%"...

::---------------------------------------------------------------------
::  Validate parameters...

  call :parseargs
  if %rc% NEQ 0 goto :exit

::---------------------------------------------------------------------
::  Switch immediately to our primary directory...

  cd /d "%hercdir%"

  set xcopy_opts=/v /c /r /k /y

::---------------------------------------------------------------------
::  Create our o/p module building sub-directory...

  call :isdir %DYNDIR%
  if not defined # mkdir %DYNDIR%

::---------------------------------------------------------------------
::  Add the dynamic module's sub-directory to the list of directories
::  that the compiler uses to resolve #include statements with (which
::  'nmake' also uses to resolve its own !INCLUDE statements with)...

  set INCLUDE=%INCLUDE%;%hercdir%;%hercdir%%DYNDIR%

::---------------------------------------------------------------------
::  Perform pre-build processing...

  call :do_callback "prebuild.bat" "pre-build"
  if %rc% NEQ 0 goto :exit

::---------------------------------------------------------------------
::  Copy the dynamic module's source files from their directory
::  over into our module building sub-directory...

  cd /d %DYNDIR%
  if exist "%projdir%*.c"           xcopy "%projdir%*.c"           . %xcopy_opts% > NUL
  if exist "%projdir%*.h"           xcopy "%projdir%*.h"           . %xcopy_opts% > NUL
  if exist "%projdir%*.rc"          xcopy "%projdir%*.rc"          . %xcopy_opts% > NUL
  if exist "%projdir%*.rc2"         xcopy "%projdir%*.rc2"         . %xcopy_opts% > NUL
  if exist "%projdir%%DYNMOD%.msvc" xcopy "%projdir%%DYNMOD%.msvc" . %xcopy_opts% > NUL
  cd ..

::---------------------------------------------------------------------
::  Now switch back to the main Hercules source-code directory
::  to do the actual build, by calling its main "makefile.bat"

  cd /d "%hercdir%"
  call "%hercdir%makefile.bat" "%build_type%" "%hercdir%makefile.msvc" "%num_cpus%" %extra_nmake_args%
  set rc=%errorlevel%
  if %rc% NEQ 0 goto :exit

::---------------------------------------------------------------------
::  Perform post-build processing...

  call :do_callback "postbld.bat" "post-build"
  if %rc% NEQ 0 goto :exit

::---------------------------------------------------------------------
:: Done!

  echo %~nx0^(1^) : Build complete.
  goto :exit

::---------------------------------------------------------------------
::  Restore original directory and environment before exiting...

:exit

  popd
  endlocal & set rc=%rc%
  exit /b %rc%

::---------------------------------------------------------------------
::                      CALLED SUBROUTINES
::---------------------------------------------------------------------
:parseargs

  ::  Validate hercules directory...

  call :isdir "%hercdir%"
  if not defined # (
    echo %~nx0^(1^) : error C9999 : hercdir "%hercdir%" not found?!
    set rc=1
    goto :EOF
  )

  ::  Validate module name...

  call :hasblank "%modname%"
  if defined # goto :bad_modname
  call :goodfn "%modname%"
  if not defined # (
:bad_modname
    echo %~nx0^(1^) : error C9999 : Invalid module name "%modname%"
    set rc=1
    goto :EOF
  )

  ::  Specified dynamic module name becomes our work sub-directory.

  set DYNMOD=%modname%
  set DYNDIR=%modname%\

  ::  Validate i/p project directory...

  call :isdir "%projdir%"
  if not defined # (
    echo %~nx0^(1^) : error C9999 : projdir "%projdir%" not found.
    set rc=1
    goto :EOF
  )

  ::  Verify their makefile include exists...

  call :isfile "%projdir%%DYNMOD%.msvc"
  if not defined # (
:bad_projdir
    echo %~nx0^(1^) : error C9999 : makefile include "%projdir%%DYNMOD%.msvc" not found.
    set rc=1
    goto :EOF
  )

  ::  Make sure the project directory they specified
  ::  was a fully qualified directory path...

  set @=%cd%
  cd /d "%projdir%"
  call :fullpath "%projdir%%DYNMOD%.msvc"
  cd /d "%@%"
  if not defined # goto :bad_projdir
  if /i not "%#%" == "%projdir%%DYNMOD%.msvc" if /i not "%#%" == "%projdir%\%DYNMOD%.msvc" (
    echo %~nx0^(1^) : error C9999 : projdir "%projdir%" not fully qualified.
    set rc=1
    goto :EOF
  )

  ::  Everything looks okay...

  goto :EOF

::---------------------------------------------------------------------
:do_callback

  set batfile=%~1
  set msgtxt=%~2

  call :isfile "%projdir%%batfile%"
  if not defined # goto :EOF

  echo %~nx0^(1^) : Invoking %msgtxt% processing...

  cd /d "%projdir%"
  call "%projdir%%batfile%" "%hercdir%" "%modname%" "%build_type%" "%num_cpus%" %extra_nmake_args%
  set rc=%errorlevel%
  cd /d "%hercdir%"

  if %rc% NEQ 0 echo %~nx0^(1^) : error C9999 : %msgtxt% processing failed.
  if %rc% EQU 0 echo %~nx0^(1^) : continuing...

  goto :EOF

::---------------------------------------------------------------------
:hasblank

  REM  %1  =  string to check
  REM   #  =  return code: 1 if contains blank, undefined if not

  setlocal
  set @=%~1
  set #=
  for /f "tokens=1*" %%a in ("@%@%@") do if not "%%b" == "" set #=1
  endlocal & set #=%#%
  goto :EOF

::---------------------------------------------------------------------
:goodfn

  REM  %1  =  string to check
  REM   #  =  return code: 1 if good filename, undefined if not

  setlocal
  set @=%~1
  set #=1
  for /f "delims=\/:*?<>|" %%i in ("@%@%@") do if not "%%i" == "@%@%@" set #=
  endlocal & set #=%#%
  goto :EOF

::---------------------------------------------------------------------
:isfile

  REM  %1  =  file to check
  REM   #  =  return code: defined if file exists, else undefined

  set "#=%~a1"
  if not defined # goto :EOF
  if "%#:~0,1%" == "-" goto :EOF
  set "#="
  goto :EOF

::---------------------------------------------------------------------
:isdir

  REM  %1  =  directory to check
  REM   #  =  return code: defined if directory exists, else undefined

  set #=%~a1
  if not defined # goto :EOF
  if "%#:~0,1%" == "d" goto :EOF
  set "#="
  goto :EOF

::---------------------------------------------------------------------
:fullpath

  REM  %1  =  filename to check (with or without directory)
  REM   #  =  returned full path or undefined if not found

  ::  Search the Windows PATH for the file in question and return
  ::  its fully qualified path if found. Otherwise return an empty
  ::  string. Note: the below does not work for directories, only
  ::  files...

  setlocal
  set path=.;%path%
  set #=%~dpnx$PATH:1
  endlocal & set #=%#%
  goto :EOF


::---------------------------------------------------------------------
