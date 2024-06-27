@if defined TRACEON (@echo on) else (@echo off)

  setlocal

  set CUSTOM_BUILD_STRING="** The SDL 4.x Hyperion version of Hercules **"

  set "TRACE=if defined DEBUG echo"
  set "return=goto :EOF"
  set "break=goto :break"
  set "exit=goto :exit"
  set "rc=0"

  set "nx0=%~nx0"             && @REM (our name and extension)
  set "nx0_cmdline=%0 %*"     && @REM (save original cmdline used)

  echo %nx0% begun on %date% at %time: =0%
  echo cmdline: %nx0_cmdline%
  goto :parse_args

::-----------------------------------------------------------------------------
::                                  HELP
::-----------------------------------------------------------------------------
:HELP

  echo.
  echo %~nx0^(1^) : error C9999 : Help information is as follows:
  echo.
  echo                             %~nx0
  echo.
  echo  Initializes the Windows software development build envionment and invokes
  echo  nmake to build the desired 32 or 64-bit version of the Hercules emulator.
  echo.
  echo  Format:
  echo.
  echo    %~nx0  {build-type}  {makefile-name}  {num-cpu-engines}  \
  echo                  [-asm]                                            \
  echo                  [-title "custom build title"]                     \
  echo                  [-hqa {directory}]                                \
  echo                  [-extpkg {directory}]                             \
  echo                  [-a^|clean]                                        \
  echo                  [{nmake-option}]
  echo.
  echo  Where:
  echo.
  echo    {build-type}        The desired build configuration. Valid values are
  echo                        DEBUG / RETAIL for building a 32-bit Hercules, or
  echo                        DEBUG-X64 / RETAIL-X64 to build a 64-bit version
  echo                        of Hercules targeting (favoring) AMD64 processors.
  echo.
  echo                        DEBUG builds activate/enable UNOPTIMIZED debugging
  echo                        logic and are thus VERY slow and not recommended
  echo                        for normal use. RETAIL builds on the other hand
  echo                        are highly optimized and thus the recommended type
  echo                        for normal every day ("production") use.
  echo.
  echo    {makefile-name}     The name of our makefile: 'makefile.msvc' (or some
  echo                        other makefile name if you have a customized one)
  echo.
  echo    {num-cpu-engines}   The maximum number of emulated CPUs (NUMCPU=) you
  echo                        want this build of Hercules to support: 1 to 64.
  echo.
  echo    -asm                To generate assembly (.cod) listings.
  echo.
  echo    -title "xxx..."     To define a custom title for this build.
  echo.
  echo    -hqa "directory"    To define the Hercules Quality Assurance directory
  echo                        containing your optional "hqa.h" and/or "HQA.msvc"
  echo                        build settings override files.
  echo.
  echo    -extpkg "directory" To define the base directory where the Hercules
  echo                        External Packages are installed. Hercules will use
  echo                        the 'include' and 'lib' subdirectories of this
  echo                        directory to locate External Package header files
  echo                        and lib files during the build process.  If not
  echo                        specified the default is to use the header and lib
  echo                        files that come with the Hercules repository.
  echo.
  echo    [-a^|clean]          Use '-a' to perform a full rebuild of all Hercules
  echo                        binaries, or 'clean' to delete all temporary work
  echo                        files from all work/output directories, including
  echo                        any/all previously built binaries. If not specified
  echo                        then only those modules that need to be rebuilt are
  echo                        actually rebuilt, usually resulting in much quicker
  echo                        build. However, when doing a 'RETAIL' build it is
  echo                        HIGHLY RECOMMENDED that you always specify the '-a'
  echo                        option to ensure that a complete rebuild is done.
  echo.
  echo    [{nmake-option}]    Extra nmake option(s).   (e.g. -k, -g, etc...)
  echo.

  set "rc=1"
  %exit%

::*****************************************************************************
::*****************************************************************************
::*****************************************************************************
::***                                                                       ***
::***                         MAKEFILE.BAT                                  ***
::***                                                                       ***
::*****************************************************************************
::*****************************************************************************
::*****************************************************************************
::*                                                                           *
::*      Designed to be called either from the command line or by a           *
::*      Visual Studio makefile project with the "Build command line"         *
::*      set to: "makefile.bat <arguments...>". See 'HELP' section            *
::*      just above for details regarding use.                                *
::*                                                                           *
::*      If this batch file works okay then it was written by Fish.           *
::*      If it doesn't work then I don't know who the heck wrote it.          *
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
::*                                                                           *
::*                          PROGRAMMING NOTE                                 *
::*                                                                           *
::*  It's important to use 'setlocal' before calling the Microsoft batch      *
::*  file that sets up the build environment and to call 'endlocal' before    *
::*  exiting.                                                                 *
::*                                                                           *
::*  Doing this ensures the build environment is freshly initialized each     *
::*  time this batch file is invoked, thus ensuing a valid build environment  *
::*  is created each time a build is performed. Failure to do so runs the     *
::*  risk of not only causing an incompatible (invalid) build environment     *
::*  being constructed as a result of subsequent build setting being created  *
::*  on top of the previous build settings, but also risks overflowing the    *
::*  environment area since the PATH and LIB variables would keep growing     *
::*  ever longer and longer.                                                  *
::*                                                                           *
::*  Therefore to ensure that never happens and that we always start with a   *
::*  freshly initialized and (hopefully!) valid build environment each time,  *
::*  we use 'setlocal' and 'endlocal' to push/pop the local environment pool  *
::*  each time we are called.                                                 *
::*                                                                           *
::*  Also note that while it would be simpler to just create a "front end"    *
::*  batch file to issue the setlocal before invoking this batch file (and    *
::*  then do the endlocal once we return), doing it that way leaves open      *
::*  the possibility that someone who doesn't know better from bypassing      *
::*  the font-end batch file altogether and invoking us directly and then     *
::*  asking for help when they have problems resultimg from their build       *
::*  environment from being screwed up.                                       *
::*                                                                           *
::*  Yes, it's more effort to do it this way but as long as you're careful    *
::*  it's well worth it in my personal opinion.                               *
::*                                                                           *
::*                                               -- Fish, March 2009         *
::*                                                                           *
::*****************************************************************************


::-----------------------------------------------------------------------------
::                                PARSE ARGUMENTS
::-----------------------------------------------------------------------------
:parse_args

  if "%~1" == ""        goto :HELP
  if "%~1" == "?"       goto :HELP
  if "%~1" == "/?"      goto :HELP
  if "%~1" == "-?"      goto :HELP
  if "%~1" == "--help"  goto :HELP

  set "build_type="
  set "makefile_name="
  set "num_cpus="
  set "extra_nmake_args="
  set "SolutionDir="
  set "ProjectFileName="

  call :parse_args_loop %*
  if not "%rc%" == "0" %exit%
  goto :begin

:parse_args_loop

  if "%~1" == "" %return%
  set "2shifts="
  call :parse_this_arg "%~1" "%~2"
  shift /1
  if defined 2shifts shift /1
  goto :parse_args_loop

:parse_this_arg

  set "opt=%~1"
  set "optval=%~2"

  if "%opt:~0,1%" == "-" goto :parse_opt
  if "%opt:~0,1%" == "/" goto :parse_opt

  ::  Positional argument...

  if not defined build_type (
    set         "build_type=%opt%"
    %return%
  )

  if not defined makefile_name (
    set         "makefile_name=%opt%"
    %return%
  )

  if not defined num_cpus (
    set         "num_cpus=%opt%"
    %return%
  )

  if not defined extra_nmake_args (
    set         "extra_nmake_args=%opt%"
    %return%
  )

  if not defined SolutionDir (
    call :re_set SolutionDir "%opt%"
    %return%
  )

  if not defined ProjectFileName (
    call :re_set ProjectFileName "%opt%"
    %return%
  )

  ::  Remaining arguments treated as extra nmake arguments...

  set "2shifts="
  set "extra_nmake_args=%extra_nmake_args% %opt%"
  %return%

:parse_opt

  set "2shifts=yes"

  if /i "%opt:~1%" == "a"      goto :makeall
  if /i "%opt:~1%" == "hqa"    goto :hqa
  if /i "%opt:~1%" == "asm"    goto :asm
  if /i "%opt:~1%" == "title"  goto :title
  if /i "%opt:~1%" == "extpkg" goto :extpkg

  ::  Unrecognized options treated as extra nmake arguments...

  set "2shifts="
  set "extra_nmake_args=%extra_nmake_args% %opt%"
  %return%

:makeall

  set "2shifts="
  set "extra_nmake_args=%extra_nmake_args% -a"
  %return%

:hqa

  set "HQA_DIR="

  if not exist "%optval%" goto :hqa_dir_notfound
  if not exist "%optval%\hqa.h" %return%

  set "HQA_DIR=%optval%"
  %return%

:hqa_dir_notfound

  echo.
  echo %~nx0^(1^) : error C9999 : HQA directory not found: "%optval%"
  set "rc=1"
  %return%

:asm

  set "2shifts="
  set "ASSEMBLY_LISTINGS=1"
  %return%

:title

  set CUSTOM_BUILD_STRING="%optval%"
  %return%

:extpkg

  if not exist "%optval%" (
    echo.
    echo %~nx0^(1^) : error C9999 : extpkg directory not found: "%optval%"
    set "rc=1"
    %return%
  )

  set "INCLUDE=%INCLUDE%;%optval%\include"
  set "LIB=%LIB%;%optval%\lib"
  %return%


::-----------------------------------------------------------------------------
::                                BEGIN
::-----------------------------------------------------------------------------
:begin

  call :validate_vstudio

  if %rc% NEQ 0 (set "rc=1"
                 %exit%)


  call :validate_makefile

  if %rc% NEQ 0 (set "rc=1"
                 %exit%)


  call :validate_num_cpus

  if %rc% NEQ 0 (set "rc=1"
                 %exit%)


  call :determine_cfg

  if %rc% NEQ 0 (set "rc=1"
                 %exit%)


  call :determine_targ_arch

  if %rc% NEQ 0 (set "rc=1"
                 %exit%)


  call :set_build_env

  if %rc% NEQ 0 (set "rc=1"
                 %exit%)


  ::-------------------------
  ::    Do the build...
  ::-------------------------

  if defined is_multi_build (
    goto :multi_build
  ) else (
    goto :nmake
  )


::-----------------------------------------------------------------------------
::                            set_build_env
::-----------------------------------------------------------------------------
:set_build_env

  if /i "%targ_arch%" == "all"   set "vstools_arch=32"
  if /i "%targ_arch%" == "x86"   set "vstools_arch=32"
  if /i "%targ_arch%" == "amd64" set "vstools_arch=64"

  set "original_include=%include%"

  call "%~dp0vstools.cmd"  %vstools_arch%
  set "rc=%errorlevel%"

  if %rc% equ 0 (
    @REM (fix VS2017 insanity)
    set "INCLUDE=%INCLUDE%;%original_include%"
  )

  %return%


::-----------------------------------------------------------------------------
::                                nmake
::-----------------------------------------------------------------------------
:nmake

  set  "MAX_CPU_ENGS=%num_cpus%"

  call :set_herc_VERSION

  if %rc% NEQ 0 (
    set "rc=1"
    %exit%
  )

  ::  Dump environment variables...

  call :dump_env

  ::  Additional nmake arguments (for reference):
  ::
  ::   -nologo   suppress copyright banner
  ::   -s        silent (suppress commands echoing)
  ::   -k        keep going if error(s)
  ::   -g        display !INCLUDEd files (VS2005 or greater only)

  nmake -nologo -s -f "%makefile_name%"  %extra_nmake_args%
  set "rc=%errorlevel%"
  %exit%


::-----------------------------------------------------------------------------
::                            multi_build
::-----------------------------------------------------------------------------
::
::  Visual Studio multi-config multi-platform parallel build.
::
::  The following is special logic to leverage Fish's "RunJobs" tool
::  that allows us to build multiple different project configurations
::  in parallel with one another which Microsoft does not yet support.
::
::  Refer to the CodeProject article entitled "RunJobs" at the following
::  URL for more details: http://www.codeproject.com/KB/cpp/runjobs.aspx
::
::-----------------------------------------------------------------------------
:multi_build

  :: Make sure the "runjobs.exe" program exists...

  set "runjobs_exe=runjobs.exe"
  call :fullpath "%runjobs_exe%"
  if "%fullpath%" == "" goto :no_runjobs
  set "runjobs_exe=%fullpath%"

  :: Make sure ProjectFileName got set properly...

  if not defined ProjectFileName (
    echo.
    echo %~nx0^(1^) : error C9999 : 'ProjectFileName' is undefined!
    set "rc=1"
    %exit%
  )

  ::  Both VCBUILD and MSBUILD requires that both $(SolutionDir)
  ::  and $(SolutionPath) be defined. Note however that while
  ::  SolutionDir MUST be accurate (so that VCBUILD and MSBUILD
  ::  can each find the Solution and Projects you wish to build),
  ::  the actual Solution file defined in the SolutionPath variable
  ::  does NOT actually need to exist (since it is not ever used).
  ::  The DIRECTORY component of its path however, MUST match the
  ::  SolutionDir variable value.

  if not defined SolutionDir call :re_set SolutionDir "%cd%\"
  call :re_set SolutionPath "%SolutionDir%notused.sln"

  :: Set '@' if rebuild, set '#' if clean.
  call :ifallorclean  %extra_nmake_args%

  :: Determine the proper runjobs jobs file to use
  :: containing either the MSBUILD or VCBUILD command

  if %vsver% GEQ %vs2010% (

    @REM  VS2010 and greater: use MSBUILD...

    if defined # (
      set "MSBUILDOPT=/target:Clean /nologo"
    ) else if defined @ (
      set "MSBUILDOPT=/target:Rebuild /nologo"
    ) else (
      set "MSBUILDOPT=/target:Build /nologo"
    )

    set "runjobs_file=%CFG%-%targ_arch%.msbuild.jobs"

  ) else (

    @REM  VS2008 and earlier: use VCBUILD...

    if defined # (
      set "VCBUILDOPT=/clean /nologo"
    ) else if defined @ (
      set "VCBUILDOPT=/rebuild /nologo"
    ) else (
      set "VCBUILDOPT=/nologo"
    )

    set "runjobs_file=%CFG%-%targ_arch%.jobs"
  )

  :: Now invoke runjobs to build each individual target in parallel...

  "%runjobs_exe%" %runjobs_file%
  set "rc=%errorlevel%"
  %exit%


:no_runjobs

  echo.
  echo %~nx0^(1^) : error C9999 : "%runjobs_exe%" not found
  set "rc=1"
  %exit%


::-----------------------------------------------------------------------------
::                           ifallorclean
::-----------------------------------------------------------------------------
:ifallorclean

  set @=
  set #=

:ifallorclean_loop

  if    "%1" == ""      %return%
  if /i "%1" == "-a"    set @=1
  if /i "%1" == "/a"    set @=1
  if /i "%1" == "all"   set @=1
  if /i "%1" == "clean" set #=1

  shift /1

  goto :ifallorclean_loop


::-----------------------------------------------------------------------------
::                        set_herc_VERSION
::-----------------------------------------------------------------------------
:set_herc_VERSION

  :: All Hercules version logic moved to "_dynamic_version.cmd" script

  call "%~dp0_dynamic_version.cmd"
  set "rc=%errorlevel%"

  echo VERSION = %VERSION%

  if defined CUSTOM_BUILD_STRING (
    echo CUSTOM_BUILD_STRING = %CUSTOM_BUILD_STRING%
  )

  %return%


::-----------------------------------------------------------------------------
::                         validate_vstudio
::-----------------------------------------------------------------------------
:validate_vstudio

  :: Call the "vstools.cmd" helper script to detect which version of
  :: Visual Studio we'll be using.  Note that it also sets some helpful
  :: variables for us too.  Refer to the vstools script for details.

  call "%~dp0vstools.cmd"  detect
  set "rc=%errorlevel%"
  if %rc% EQU 0 (
   echo Visual Studio %vsname% detected
  )
  %return%


::-----------------------------------------------------------------------------
::                         validate_makefile
::-----------------------------------------------------------------------------
:validate_makefile

  set "rc=0"

  if not exist "%makefile_name%" (
    echo.
    echo %~nx0^(1^) : error C9999 : makefile "%makefile_name%" not found
    set "rc=1"
  )

  %return%

::-----------------------------------------------------------------------------
::                         validate_num_cpus
::-----------------------------------------------------------------------------
:validate_num_cpus

  call :isnumeric "%num_cpus%"

  if not defined #     goto :bad_num_cpus
  if %num_cpus% LSS 1  goto :bad_num_cpus
  if %num_cpus% GTR 64 goto :bad_num_cpus
  %return%

:bad_num_cpus

  echo.
  echo %~nx0^(1^) : error C9999 : "%num_cpus%": Invalid number of cpu engines
  set "rc=1"
  %return%


::-----------------------------------------------------------------------------
::                          determine_cfg
::-----------------------------------------------------------------------------
:determine_cfg

  set "rc=0"
  set "CFG="
  set "is_multi_build="

  if /i "%build_type%" == "DEBUG"       set "CFG=DEBUG"
  if /i "%build_type%" == "DEBUG-X64"   set "CFG=DEBUG"

  if /i "%build_type%" == "RETAIL"      set "CFG=RETAIL"
  if /i "%build_type%" == "RETAIL-X64"  set "CFG=RETAIL"

  if not "%CFG%" == "" (
    %return%
  )

  set "is_multi_build=1"

  if /i "%build_type%" == "DEBUG-ALL"   set "CFG=debug"
  if /i "%build_type%" == "RETAIL-ALL"  set "CFG=retail"
  if /i "%build_type%" == "ALL-ALL"     set "CFG=all"

  if not "%CFG%" == "" (
    %return%
  )

:bad_cfg

  set "is_multi_build="
  echo.
  echo %~nx0^(1^) : error C9999 : "%build_type%": Invalid build-type
  set "rc=1"
  %return%


::-----------------------------------------------------------------------------
::                        determine_targ_arch
::-----------------------------------------------------------------------------
:determine_targ_arch

  set "rc=0"
  set "targ_arch="
  set "CPU="
  set "_WIN64="

  if /i "%build_type%" == "DEBUG"       set "targ_arch=x86"
  if /i "%build_type%" == "RETAIL"      set "targ_arch=x86"

  if /i "%build_type%" == "DEBUG-X64"   set "targ_arch=amd64"
  if /i "%build_type%" == "RETAIL-X64"  set "targ_arch=amd64"

  if /i "%build_type%" == "DEBUG-ALL"   set "targ_arch=all"
  if /i "%build_type%" == "RETAIL-ALL"  set "targ_arch=all"
  if /i "%build_type%" == "ALL-ALL"     set "targ_arch=all"

  if /i "%targ_arch%"  == "x86"         set "CPU=i386"
  if /i "%targ_arch%"  == "amd64"       set "CPU=AMD64"
  if /i "%targ_arch%"  == "amd64"       set "_WIN64=1"

  if not "%targ_arch%" == "" (
    %return%
  )

:bad_targ_arch

  echo.
  echo %~nx0^(1^) : error C9999 : "%build_type%": Invalid build-type
  set "rc=1"
  %return%


::-----------------------------------------------------------------------------
::                            fullpath
::-----------------------------------------------------------------------------
:fullpath

  ::  Search the Windows PATH for the file in question and return its
  ::  fully qualified path if found. Otherwise return an empty string.
  ::  Note: the below does not work for directories, only files.

  set "save_path=%path%"
  set "path=.;%path%"
  set "fullpath=%~dpnx$PATH:1"
  set "path=%save_path%"
  %return%


::-----------------------------------------------------------------------------
::                            isnumeric
::-----------------------------------------------------------------------------
:isnumeric

  set "@=%~1"
  set "#="
  for /f "delims=0123456789" %%a in ("%@%/") do if "%%a" == "/" set "#=1"
  %return%


::-----------------------------------------------------------------------------
::                             tempfn
::-----------------------------------------------------------------------------
:tempfn

  setlocal
  set "var_name=%~1"
  set "file_ext=%~2"
  set "%var_name%="
  set "@="
  for /f "delims=/ tokens=1-3" %%a in ("%date:~4%") do (
    for /f "delims=:. tokens=1-4" %%d in ("%time: =0%") do (
      set "@=TMP%%c%%a%%b%%d%%e%%f%%g%file_ext%"
    )
  )
  endlocal && set "%var_name%=%@%"
  %return%


::-----------------------------------------------------------------------------
::                             remlead0
::-----------------------------------------------------------------------------
:remlead0

  @REM  Remove leading zeros so number isn't interpreted as an octal number

  call :isnumeric "%~1"
  if not defined # %return%

  set "var_value=%~1"
  set "var_name=%~2"

  set "###="

  for /f "tokens=* delims=0" %%a in ("%var_value%") do set "###=%%a"

  if not defined ### set "###=0"
  set "%var_name%=%###%"

  set "###="
  set "var_name="
  set "var_value="

  %return%


::-----------------------------------------------------------------------------
::                              re_set
::-----------------------------------------------------------------------------
:re_set

  :: The following called when set= contains (), which confuses poor windoze

  set %~1=%~2
  %return%


::-----------------------------------------------------------------------------
::                              dump_env
::-----------------------------------------------------------------------------
:dump_env

  echo.
  echo --------------------------- ENVIRONMENT POOL ---------------------------
  echo.
  set

  ::  Format the PATH, LIB and INCLUDE variables for easier reading

  echo.
  echo -------------------------- PATH, LIB, INCLUDE --------------------------
  call :fxxx PATH
  call :fxxx LIB
  call :fxxx INCLUDE

  %return%


::-----------------------------------------------------------------------------
::                                fxxx
::-----------------------------------------------------------------------------
:fxxx

  @REM  Parses semi-colon delimited variable (e.g. PATH, LIB, INCLUDE, etc.)

  setlocal enabledelayedexpansion

  set @=%~1

  echo.
  echo %@%=
  echo.

  @REM  Delayed expansion used here!
  set _=!%@%!

:fxxx_loop

  for /f "delims=; tokens=1*" %%a in ("%_%") do (
    echo   %%a
    if "%%b" == "" %break%
    set _=%%b
    goto :fxxx_loop
  )

:break

  endlocal
  %return%


::-----------------------------------------------------------------------------
::                               EXIT
::-----------------------------------------------------------------------------
:exit

  echo %nx0% ended on %date% at %time: =0%, rc=%rc%

  endlocal & exit /b %rc%
