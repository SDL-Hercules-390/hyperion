@if defined TRACEON (@echo on) else (@echo off)

  REM  If this batch file works, then it was written by Fish.
  REM  If it doesn't then I don't know who the heck wrote it.

  setlocal
  pushd .
  goto :init

::-----------------------------------------------------------------------------
::                          _dynamic_version.cmd
::-----------------------------------------------------------------------------
:help

  echo.
  echo     NAME
  echo.
  echo         %~n0  --  Create Hercules "_dynamic_version.h" header file
  echo.
  echo     SYNOPSIS
  echo.
  echo         %~nx0   [OPTIONS]
  echo.
  echo     ARGUMENTS
  echo.
  echo         (NONE)
  echo.
  echo     OPTIONS
  echo.
  echo         /c         Create the header file
  echo         /t         Test option (does NOT write to the file).
  echo.
  echo     NOTES
  echo.
  echo         %~n0 is a Hercules build script that's automatically
  echo         invoked by the "makefile.bat" script to dynamically create
  echo         the "_dynamic_version.h" header file containing the Hercules
  echo         version constants (#define values). The "_dynamic_version.h"
  echo         header file is then #included by the "version.h" header file.
  echo.
  echo         The Hercules file "configure.ac" MUST exist in the current
  echo         directory.  That is to say, %~n0 is designed to be
  echo         run from the main Hercules souce code directory.
  echo.
  echo         The /c option must be used to actually create the header file.
  echo         The /t will only display the header file contents that WOULD
  echo         have been created by the /c option, but without creating or
  echo         overlaying whatever file that may already exist.
  echo.
  echo         If both /c and /t are specified then the /t option overrides
  echo         the /c option. That is to say, when the /t option is specified,
  echo         the output file will NEVER be created or overwritten in any way.
  echo.
  echo     EXIT STATUS
  echo.
  echo         0          Success completion
  echo         1          Abnormal termination
  echo.
  echo     AUTHOR
  echo.
  echo         "Fish" (David B. Trout)
  echo.
  echo     VERSION
  echo.
  echo         1.0      (May 6, 2016)

  set /a "rc=1"
  %exit%

::-----------------------------------------------------------------------------
::                         CREATE_DYN_VERS_HDR
::-----------------------------------------------------------------------------
:create_dyn_vers_hdr

  set "line1=/*  _DYNAMIC_VERSION.H (C) Copyright "Fish" (David B. Trout), 2016   */"
  set "line2=/*      Dynamically generated Hercules VERSION #defines              */"
  set "line3=/*                                                                   */"
  set "line4=/*   Released under "The Q Public License Version 1"                 */"
  set "line5=/*   (http://www.hercules-390.org/herclic.html) as modifications     */"
  set "line6=/*   to Hercules.                                                    */"
  set "line7="
  set "line8=/*-------------------------------------------------------------------*/"
  set "line9=/* This header file defines the Hercules version constants. It is    */"
  set "line10=/* dynamically generated during the build by the _dynamic_version    */"
  set "line11=/* script (on Windows by the "_dynamic_version.cmd" batch file)      */"
  set "line12=/* and is #included automatically by the "version.h" header.         */"
  set "line13=/*-------------------------------------------------------------------*/"
  set "line14="
  set "line15=#undef  VERS_MAJ"
  set "line16=#define VERS_MAJ    %VERS_MAJ%"
  set "line17="
  set "line18=#undef  VERS_INT"
  set "line19=#define VERS_INT    %VERS_INT%"
  set "line20="
  set "line21=#undef  VERS_MIN"
  set "line22=#define VERS_MIN    %VERS_MIN%"
  set "line23="
  set "line24=#undef  VERS_BLD"
  set "line25=#define VERS_BLD    %VERS_BLD%"
  set "line26="
  set "line27=#undef  VERSION"
  set "line28=#define VERSION     %VERSION%"

  set "numlines=28"

  if defined create_file goto :create_file

  if not defined test_only (
    call :help
    %return%
  )

  @REM test_only: just echo the lines

  setlocal enabledelayedexpansion
  for /L %%i in (1,1,%numlines%) do (
    if not defined line%%i (
      echo.
    ) else (
      echo !line%%i!
    )
  )
  endlocal

  %return%

:create_file

  ::  PROGRAMMING NOTE: in order to ensure nmake's dependency tracking is
  ::  handled correctly, we always output to "tmpfile" and then compare it
  ::  with the existing file. Only if they are different do we then delete
  ::  the old file and rename the new file to replace it. After doing so,
  ::  we then "touch" the "version.h" header file so nmake knows it is out
  ::  of date and must therefore rebuild any files that depend on it.


  call :tempfn  tmpfile  .h
  if exist "%tmpfile%" del "%tmpfile%"

  setlocal enabledelayedexpansion
  for /L %%i in (1,1,%numlines%) do (
    if not defined line%%i (
      echo.>> "!tmpfile!"
    ) else (
      echo !line%%i! >> "!tmpfile!"
    )
  )
  endlocal

  if not exist "%outfile%" goto :new_file
  fc.exe "%tmpfile%" "%outfile%" > nul 2>&1
  if %errorlevel% NEQ 0 goto :new_file
  del "%tmpfile%"
  goto :echo_version

:new_file

  move /y "%tmpfile%" "%outfile%" > nul 2>&1

  @REM Windows's magic "touch" syntax!

  @REM "https://technet.microsoft.com/en-us/library/bb490886.aspx"
  @REM "https://blogs.msdn.microsoft.com/oldnewthing/20130710-00/?p=3843/"

  copy "%touchfile%"+,, > nul 2>&1

:echo_version

  setlocal enabledelayedexpansion
  echo.
  echo Hercules VERSION = !VERSION! (!VERS_MAJ!.!VERS_INT!.!VERS_MIN!.!VERS_BLD!)
  echo.
  endlocal

  %return%

::-----------------------------------------------------------------------------
::                               INIT
::-----------------------------------------------------------------------------
:init

  @REM Define some constants...

  set "@nx0=%~nx0"

  set "TRACE=if defined DEBUG echo"
  set "return=goto :EOF"
  set "break=goto :break"
  set "skip=goto :skip"
  set "exit=goto :exit"

  set /a "rc=0"
  set /a "maxrc=0"
  set "basedir=%cd%"

  set "infile=configure.ac"
  set "outfile=_dynamic_version.h"
  set "touchfile=version.h"

  @REM  Options as listed in help...

  set "create_file="
  set "test_only="

  goto :parse_args

::-----------------------------------------------------------------------------
::                             parse_args
::-----------------------------------------------------------------------------
:parse_args

  set /a "rc=0"

  if /i "%~1" == ""        goto :help
  if /i "%~1" == "?"       goto :help
  if /i "%~1" == "/?"      goto :help
  if /i "%~1" == "-?"      goto :help
  if /i "%~1" == "--help"  goto :help

  goto :parse_options_loop

::-----------------------------------------------------------------------------
::                              tempfn
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
::                              isfile
::-----------------------------------------------------------------------------
:isfile

  if not exist "%~1" (
    set "isfile="
    %return%
  )
  set "isfile=%~a1"
  if defined isfile (
    if /i "%isfile:~0,1%" == "d" set "isfile="
  )
  %return%

::-----------------------------------------------------------------------------
::                             isreadonly
::-----------------------------------------------------------------------------
:isreadonly
  set "@=%~a1"
  set "isreadonly=1"
  if /i "%@:~1,1%" == "r" %return%
  set "isreadonly="
  %return%

::-----------------------------------------------------------------------------
::                              isnum
::-----------------------------------------------------------------------------
:isnum

  set "@=%~1"
  set "isnum="
  for /f "delims=0123456789" %%i in ("%@%/") do if "%%i" == "/" set "isnum=1"
  %return%

::-----------------------------------------------------------------------------
::                             remlead0
::-----------------------------------------------------------------------------
:remlead0

  @REM  Remove leading zeros so number isn't interpreted as an octal number

  call :isnum "%~1"
  if not defined isnum %return%

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
::                             fullpath
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
::                   ( parse_options_loop helper )
::-----------------------------------------------------------------------------
:parseopt

  @REM  This function expects the next two command line arguments
  @REM  %1 and %2 to be passed to it.  %1 is expected to be a true
  @REM  option (its first character should start with a / or -).
  @REM
  @REM  Both argument are them examined and the results are placed into
  @REM  the following variables:
  @REM
  @REM    opt:        The current option as-is (e.g. "-d")
  @REM
  @REM    optname:    Just the characters following the '-' (e.g. "d")
  @REM
  @REM    optval:     The next token following the option (i.e. %2),
  @REM                but only if it's not an option itself (not isopt).
  @REM                Otherwise optval is set to empty/undefined since
  @REM                it is not actually an option value but is instead
  @REM                the next option.

  set "opt=%~1"
  set "optname=%opt:~1%"
  set "optval=%~2"
  setlocal
  call :isopt "%optval%"
  endlocal && set "#=%isopt%"
  if defined # set "optval="
  %return%

::-----------------------------------------------------------------------------
::                   ( parse_options_loop helper )
::-----------------------------------------------------------------------------
:isopt

  @REM  Examines first character of passed value to determine
  @REM  whether it's the next option or not. If it starts with
  @REM  a '/' or '-' then it's the next option. Else it's not.

  set           "isopt=%~1"
  if not defined isopt     %return%
  if "%isopt:~0,1%" == "/" %return%
  if "%isopt:~0,1%" == "-" %return%
  set "isopt="
  %return%

::-----------------------------------------------------------------------------
::                          parse_options_loop
::-----------------------------------------------------------------------------
:parse_options_loop

  if "%~1" == "" goto :options_loop_end

  @REM  Parse next option...

  call :isopt    "%~1"
  call :parseopt "%~1" "%~2"
  shift /1

  if not defined isopt (

    @REM  Must be a positional option.
    @REM  Set optname identical to opt
    @REM  and empty meaningless optval.

    set "optname=%opt%"
    set "optval="
    goto :parse_positional_opts
  )

  @REM  Options that are just switches...

  if /i "%optname%" == "c" goto :parse_c_opt
  if /i "%optname%" == "t" goto :parse_t_opt

  goto :parse_unknown_opt

  @REM ------------------------------------
  @REM  Options that are just switches
  @REM ------------------------------------

:parse_c_opt

  set "create_file=1"
  goto :parse_options_loop

:parse_t_opt

  set "test_only=1"
  goto :parse_options_loop

  @REM ------------------------------------
  @REM      Positional arguments
  @REM ------------------------------------

:parse_positional_opts

  @REM  We don't support any positional arguments!
  goto :parse_unknown_opt

  @REM ------------------------------------
  @REM  Error routines
  @REM ------------------------------------

:parse_unknown_opt

  echo ERROR: Unknown/unsupported option '%opt%'. 1>&2
  set /a "rc=1"
  goto :parse_options_loop

:options_loop_end

  %TRACE% Debug: values after parsing:
  %TRACE%.
  %TRACE% create_file = %create_file%
  %TRACE% test_only   = %test_only%
  %TRACE%.

  goto :validate_args

::-----------------------------------------------------------------------------
::                            validate_args
::-----------------------------------------------------------------------------
:validate_args

  if defined create_file (
    if defined test_only (
      echo WARNING: both /t and /c specified; /c being ignored  1>&2
      set "create_file="
    )
  )

  if not defined create_file (
    if not defined test_only (
      goto :help
    )
  )

  call :isfile "%infile%"
  if not defined isfile (
    echo ERROR: required input file "%infile%" not found. 1>&2
    set /a "rc=1"
  )

  if defined create_file (
    call :isfile "%outfile%"
    if defined isfile (
      call :isreadonly  "%outfile%"
      if defined isreadonly (
        echo ERROR: output file "%outfile%" is read-only. 1>&2
        set /a "rc=1"
      )
    )
  )

  goto :validate_args_done

:validate_args_done

  if not "%rc%" == "0" %exit%

  %TRACE% Debug: values after validation:
  %TRACE%.
  %TRACE% create_file = %create_file%
  %TRACE% test_only   = %test_only%
  %TRACE%.

  if defined DEBUG %exit%
  goto :BEGIN

::-----------------------------------------------------------------------------
::                               BEGIN
::-----------------------------------------------------------------------------
:BEGIN

  call :set_VERSION
  call :create_dyn_vers_hdr
  %exit%

::-----------------------------------------------------------------------------
::                           update_maxrc
::-----------------------------------------------------------------------------
:update_maxrc

  REM maxrc remains negative once it's negative!

  if %maxrc% GEQ 0 (
    if %rc% LSS 0 (
      set /a "maxrc=%rc%"
    ) else (
      if %rc% GTR 0 (
        if %rc% GTR %maxrc% (
          set /a "maxrc=%rc%"
        )
      )
    )
  )

  %return%

::-----------------------------------------------------------------------------
::                                EXIT
::-----------------------------------------------------------------------------
:exit

  call :update_maxrc
  popd
  endlocal & exit /b %maxrc%

::-----------------------------------------------------------------------------
                              set_VERSION
::-----------------------------------------------------------------------------
:set_VERSION

  ::  The following logic determines the Hercules version number,
  ::  git or svn commit/revision information, and sets variables
  ::  VERS_MAJ, VERS_INT, VERS_MIN, VERS_BLD and VERSION.

  set "VERS_MAJ="
  set "VERS_INT="
  set "VERS_MIN="
  set "VERS_BLD="
  set "modified_str="
  set "VERSION="

  :: -------------------------------------------------------------------
  ::  First, extract the first three components of the Hercules version
  ::  from the "%infile%" file by looking for the three VERS_MAJ=n
  ::  VERS_INT=n and VERS_MIN=n statements.
  :: -------------------------------------------------------------------

  for /f "delims==# tokens=1-3" %%a in ('type %infile% ^| find /i "VERS_"') do (
    if /i "%%a" == "VERS_MAJ" for /f "tokens=1-2" %%n in ("%%b") do set "VERS_MAJ=%%n"
    if /i "%%a" == "VERS_INT" for /f "tokens=1-2" %%n in ("%%b") do set "VERS_INT=%%n"
    if /i "%%a" == "VERS_MIN" for /f "tokens=1-2" %%n in ("%%b") do set "VERS_MIN=%%n"
   )

  %TRACE%.
  %TRACE%   After %infile% parse
  %TRACE%.
  %TRACE% VERS_MAJ         = %VERS_MAJ%
  %TRACE% VERS_INT         = %VERS_INT%
  %TRACE% VERS_MIN         = %VERS_MIN%
  %TRACE%.

  if not defined VERS_MAJ (
    set "VERS_MAJ=0"
    set "VERS_INT=0"
    set "VERS_MIN=0"
  ) else (
    call :isnum "%VERS_MIN%"
    if not defined isnum set "VERS_MIN=0"
  )

  %TRACE%.
  %TRACE%   Before calling :remlead0
  %TRACE%.
  %TRACE% VERS_MAJ         = %VERS_MAJ%
  %TRACE% VERS_INT         = %VERS_INT%
  %TRACE% VERS_MIN         = %VERS_MIN%
  %TRACE%.

  call :remlead0  "%VERS_MAJ%"  VERS_MAJ
  call :remlead0  "%VERS_INT%"  VERS_INT
  call :remlead0  "%VERS_MIN%"  VERS_MIN

  %TRACE%.
  %TRACE%   After calling :remlead0
  %TRACE%.
  %TRACE% VERS_MAJ         = %VERS_MAJ%
  %TRACE% VERS_INT         = %VERS_INT%
  %TRACE% VERS_MIN         = %VERS_MIN%
  %TRACE%.

  @REM  Now to calulate a NUMERIC value for 'VERS_BLD' based on the
  @REM  SVN/GIT repository revision number.  Note that the revision
  @REM  number for SVN repositories is always numeric anyway but for
  @REM  GIT repositories we must calculate it based on total number
  @REM  of commits since GIT "revision numbers" are actually hashes.

  :: ---------------------------------------------------------------
  ::  Try TortoiseSVN's "SubWCRev.exe" program, if it exists
  :: ---------------------------------------------------------------

  set "SubWCRev_exe=SubWCRev.exe"
  call :fullpath "%SubWCRev_exe%"
  if "%fullpath%" == "" goto :set_VERSION_try_SVN

  %TRACE% Attempting SubWCRev.exe ...

  set "#="
  for /f %%a in ('%SubWCRev_exe% "." ^| find /i "E155007"') do set "#=1"
  if defined # goto :set_VERSION_try_GIT

  %TRACE% Using SubWCRev.exe ...

  set "modified_str="

  for /f "tokens=1-5" %%g in ('%SubWCRev_exe% "." -f ^| find /i "Updated to revision"') do set "VERS_BLD=%%j"
  for /f "tokens=1-5" %%g in ('%SubWCRev_exe% "." -f ^| find /i "Local modifications found"') do set "modified_str=-modified"

  if defined VERS_BLD (
    %TRACE% VERS_BLD     = %VERS_BLD%
    %TRACE% modified_str = %modified_str%
    goto :set_VERSION_do_set
  )

  goto :set_VERSION_try_SVN

  :: ---------------------------------------------------------------
  ::  Try the "svn info" and "svnversion" commands, if they exist
  :: ---------------------------------------------------------------

:set_VERSION_try_SVN

  set "svn_exe=svn.exe"
  call :fullpath "%svn_exe%"
  if "%fullpath%" == "" goto :set_VERSION_try_GIT

  %TRACE% Attempting svn.exe ...

  set "#="
  for /f %%a in ('%svn_exe% info 2^>^&1 ^| find /i "E155007"') do set "#=1"
  if defined # goto :set_VERSION_try_GIT

  %TRACE% Using svn.exe ...

  set "modified_str="

  for /f "tokens=1-2" %%a in ('%svn_exe% info 2^>^&1 ^| find /i "Revision:"') do set "VERS_BLD=%%b"

  @REM Check if there are local modifications

  set "svnversion=svnversion.exe"
  call :fullpath "%svnversion%"
  if "%fullpath%" == "" goto :set_VERSION_try_SVN_done

  %TRACE% ... and svnversion.exe too ...

  @REM  The following handles weird svnversion revision number strings
  @REM  such as "1234:5678MSP" (mixed revision switched working copy
  @REM  in sparse directory with local modifications).  Note that all
  @REM  we are trying to do is determine if there is an 'M' anywhere
  @REM  indicating local modifications are present.

  set "revnum="
  for /f "tokens=*" %%a in ('%svnversion%') do set "revnum=%%a"
  set "svnversion="

  for /f "tokens=1,2 delims=M" %%a in ("%revnum%") do (
    set "first_token=%%a"
    set "second_token=%%b"
  )

  if not "%first_token%%second_token%" == "%revnum%" set "modified_str=-modified"

  set "second_token="
  set "first_token="
  set "revnum="

:set_VERSION_try_SVN_done

  %TRACE% VERS_BLD     = %VERS_BLD%
  %TRACE% modified_str = %modified_str%

  goto :set_VERSION_do_set

  :: ---------------------------------------------------------------
  ::  Try the "git log" command, if it exists
  :: ---------------------------------------------------------------

:set_VERSION_try_GIT

  @REM Prefer "git.cmd" over git.exe (if it exists)

  set "git=git.cmd"
  call :fullpath "%git%"
  if not "%fullpath%" == "" goto :set_VERSION_test_call_git

  set "git=git.exe"
  call :fullpath "%git%"
  if "%fullpath%" == "" goto :set_VERSION_try_XXX

:set_VERSION_test_call_git

  @REM If we're using git.cmd, we must 'call' it.

  if /i "%git:~-4%" == ".cmd" set "git=call %git%"

  %TRACE% Attempting %git% ...

  %git% rev-parse > NUL 2>&1
  if %errorlevel% NEQ 0 goto :set_VERSION_try_XXX

  %TRACE% Using %git% ...

  ::  Determine if any local modifications exist. PLEASE NOTE that doing
  ::  "update-index --refresh" beforehand is seemingly critical to ensure
  ::  accurate "diff-index" results.

  %git% update-index --refresh -q > nul 2>&1
  %git% diff-index --quiet HEAD

  if %errorlevel% EQU 0 (
    set "modified_str="
  ) else (
    set "modified_str=-modified"
  )

  ::  Extract the short git hash of the repository (usually the last commit)

  set "git_hash=-gHHHHHHH"
  for /f %%a in ('%git% --work-tree=. rev-parse --short HEAD') do set "git_hash=-g%%a"

  ::  Count the number of commits and use that as our "build" number

  call :fullpath "wc.exe"
  if "%fullpath%" == "" goto :set_VERSION_try_GIT_no_wc

  %TRACE% Using wc.exe to count total commits ...

  for /f "usebackq" %%a in (`%git% log --pretty^=format:'' ^| wc.exe -l`) do set "VERS_BLD=%%a"

  goto :set_VERSION_try_GIT_done

:set_VERSION_try_GIT_no_wc

  %TRACE% Counting total commits ourself, the hard way ...

  :: PROGRAMMING NOTE: MS batch "for /f" always skips blank lines.
  :: Thus we use --pretty=format:"x" rather than --pretty=format:""
  :: so each output line is non-empty so "for /f" can count them.

  :: Technique: build a temporary batch file containing code to
  :: count the lines in the git log output file and then run it.

  call :tempfn  git_log  .log
  call :tempfn  wcl_cmd  .cmd

  %git% log --pretty=format:"x" > "%temp%\%git_log%"

  if exist "%temp%\%wcl_cmd%" del "%temp%\%wcl_cmd%"

  echo @echo off                                                  >> "%temp%\%wcl_cmd%"
  echo set /a "VERS_BLD=0"                                        >> "%temp%\%wcl_cmd%"
  echo for /f "tokens=*" %%%%a in (%%~1) do set /a "VERS_BLD+=1"  >> "%temp%\%wcl_cmd%"

  call "%temp%\%wcl_cmd%"  "%temp%\%git_log%"

  del "%temp%\%wcl_cmd%"
  del "%temp%\%git_log%"

  goto :set_VERSION_try_GIT_done

:set_VERSION_try_GIT_done

  set "modified_str=%git_hash%%modified_str%"
  set "git_hash="

  %TRACE% VERS_BLD     = %VERS_BLD%
  %TRACE% modified_str = %modified_str%

  goto :set_VERSION_do_set

:set_VERSION_try_XXX

  :: Repo type XXX...

  if defined VERS_BLD (
    %TRACE% VERS_BLD     = %VERS_BLD%
    %TRACE% modified_str = %modified_str%
    goto :set_VERSION_do_set
  )
  goto :set_VERSION_try_YYY

:set_VERSION_try_YYY

  :: Repo type YYY...

  if defined VERS_BLD (
    %TRACE% VERS_BLD     = %VERS_BLD%
    %TRACE% modified_str = %modified_str%
    goto :set_VERSION_do_set
  )
  goto :set_VERSION_try_ZZZ

:set_VERSION_try_ZZZ

  :: Repo type ZZZ...

  if defined VERS_BLD (
    %TRACE% VERS_BLD     = %VERS_BLD%
    %TRACE% modified_str = %modified_str%
    goto :set_VERSION_do_set
  )

  %TRACE% Out of things to try!

  goto :set_VERSION_do_set

:set_VERSION_do_set

  %TRACE%.
  %TRACE%     set_VERSION_do_set
  %TRACE%.
  %TRACE% VERS_MAJ         = %VERS_MAJ%
  %TRACE% VERS_INT         = %VERS_INT%
  %TRACE% VERS_MIN         = %VERS_MIN%
  %TRACE% VERS_BLD         = %VERS_BLD%
  %TRACE% modified_str     = %modified_str%
  %TRACE% VERSION          = %VERSION%
  %TRACE%.

  if not defined VERS_MAJ set "VERS_MAJ=0"
  if not defined VERS_INT set "VERS_INT=0"
  if not defined VERS_MIN set "VERS_MIN=0"
  if not defined VERS_BLD set "VERS_BLD=0"

  ::--------------------------------------------------------------
  ::  Make it PAINFULLY CLEAR this is an UNOFFICIAL "SDL" version
  ::  of 4.0 Hyperion and not an "Official" 4.0 Hyperion version
  ::  by appending the string "-SDL" to the end of the "VERSION".
  ::--------------------------------------------------------------

  set VERSION="%VERS_MAJ%.%VERS_INT%.%VERS_MIN%.%VERS_BLD%-SDL%modified_str%"

  goto :set_VERSION_done

:set_VERSION_done

  %TRACE%.
  %TRACE%     set_VERSION_done
  %TRACE%.
  %TRACE% VERS_MAJ         = %VERS_MAJ%
  %TRACE% VERS_INT         = %VERS_INT%
  %TRACE% VERS_MIN         = %VERS_MIN%
  %TRACE% VERS_BLD         = %VERS_BLD%
  %TRACE% modified_str     = %modified_str%
  %TRACE% VERSION          = %VERSION%
  %TRACE%.

  %return%

::-----------------------------------------------------------------------------
