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
  echo         %~n0  --  Defines the  Hercules VERSION related variables
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
  echo         (NONE)
  echo.
  echo     NOTES
  echo.
  echo         %~n0 is a Hercules build script that's automatically
  echo         invoked by the "makefile.bat" script to dynamically define
  echo         the set of variables related to Hercules's VERSION string.
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
  echo         2.0      (February 15, 2017)

  set /a "rc=1"
  %exit%

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

  set "configure_ac=configure.ac"

  goto :parse_args

::-----------------------------------------------------------------------------
::                             parse_args
::-----------------------------------------------------------------------------
:parse_args

  set /a "rc=0"

  if /i "%~1" == "?"       goto :help
  if /i "%~1" == "/?"      goto :help
  if /i "%~1" == "-?"      goto :help
  if /i "%~1" == "--help"  goto :help

  goto :parse_options_loop

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

  ::  (we have none)

  goto :parse_unknown_opt

  @REM ------------------------------------
  @REM  Options that are just switches
  @REM ------------------------------------

  ::  (we have none)

  @REM ------------------------------------
  @REM      Positional arguments
  @REM ------------------------------------

:parse_positional_opts

  ::  (we have none)

  goto :parse_unknown_opt

  @REM ------------------------------------
  @REM  Error routines
  @REM ------------------------------------

:parse_unknown_opt

  echo ERROR: Unknown/unsupported option '%opt%'. 1>&2
  set /a "rc=1"
  goto :parse_options_loop

:options_loop_end

  goto :validate_args

::-----------------------------------------------------------------------------
::                            validate_args
::-----------------------------------------------------------------------------
:validate_args

  ::  (no options so no validation)

  goto :validate_args_done

:validate_args_done

  if not "%rc%" == "0" %exit%

  goto :BEGIN

::-----------------------------------------------------------------------------
::                              isdir
::-----------------------------------------------------------------------------
:isdir

  if not exist "%~1" (
    set "isdir="
    %return%
  )
  set "isdir=%~a1"
  if defined isdir (
    if /i not "%isdir:~0,1%" == "d" set "isdir="
  )
  %return%

::-----------------------------------------------------------------------------
::                               BEGIN
::-----------------------------------------------------------------------------
:BEGIN

  call :isdir ".svn"
  if defined isdir (set "have_dot_svn_dir=yes" ) else (set "have_dot_svn_dir=")

  call :isdir ".git"
  if defined isdir (set "have_dot_git_dir=yes" ) else (set "have_dot_git_dir=")

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
  ::  from the "%configure_ac%" file by looking for the three VERS_MAJ=n
  ::  VERS_INT=n and VERS_MIN=n statements.
  :: -------------------------------------------------------------------

  :: (Workaround for VS2019 pipe bug)
@REM  /f "delims==# tokens=1-3" %%a in ('type %configure_ac% ^| find /i "VERS_"') do (
  for /f "delims==# tokens=1-3" %%a in ('type %configure_ac%'                   ) do (
    if /i "%%a" == "VERS_MAJ" for /f "tokens=1-2" %%n in ("%%b") do set "VERS_MAJ=%%n"
    if /i "%%a" == "VERS_INT" for /f "tokens=1-2" %%n in ("%%b") do set "VERS_INT=%%n"
    if /i "%%a" == "VERS_MIN" for /f "tokens=1-2" %%n in ("%%b") do set "VERS_MIN=%%n"
    if /i "%%a" == "VERS_DEV" for /f "tokens=1-2" %%n in ("%%b") do set "VERS_DEV=%%n"
   )

  %TRACE%.
  %TRACE%   After %configure_ac% parse
  %TRACE%.
  %TRACE% VERS_MAJ         = %VERS_MAJ%
  %TRACE% VERS_INT         = %VERS_INT%
  %TRACE% VERS_MIN         = %VERS_MIN%
  %TRACE% VERS_DEV         = %VERS_DEV%
  %TRACE%.

  if not defined VERS_MAJ set "VERS_MAJ=0"
  if not defined VERS_INT set "VERS_INT=0"
  if not defined VERS_MIN set "VERS_MIN=0"
  if not defined VERS_DEV set "VERS_DEV=0"

  call :isnum "%VERS_MAJ%"
  if not defined isnum set "VERS_MAJ=0"

  call :isnum "%VERS_INT%"
  if not defined isnum set "VERS_INT=0"

  call :isnum "%VERS_MIN%"
  if not defined isnum set "VERS_MIN=0"

  call :isnum "%VERS_DEV%"
  if not defined isnum set "VERS_DEV=0"

  %TRACE%.
  %TRACE%   Before calling :remlead0
  %TRACE%.
  %TRACE% VERS_MAJ         = %VERS_MAJ%
  %TRACE% VERS_INT         = %VERS_INT%
  %TRACE% VERS_MIN         = %VERS_MIN%
  %TRACE% VERS_DEV         = %VERS_DEV%
  %TRACE%.

  call :remlead0  "%VERS_MAJ%"  VERS_MAJ
  call :remlead0  "%VERS_INT%"  VERS_INT
  call :remlead0  "%VERS_MIN%"  VERS_MIN
  call :remlead0  "%VERS_DEV%"  VERS_DEV

  %TRACE%.
  %TRACE%   After calling :remlead0
  %TRACE%.
  %TRACE% VERS_MAJ         = %VERS_MAJ%
  %TRACE% VERS_INT         = %VERS_INT%
  %TRACE% VERS_MIN         = %VERS_MIN%
  %TRACE% VERS_DEV         = %VERS_DEV%
  %TRACE%.

  if "%VERS_DEV%" == "0" (
    set "dev_string="
  ) else (
    set "dev_string=-DEV"
  )

  @REM ----------------------------------------------------------------
  @REM   Now to calulate a NUMERIC value for 'VERS_BLD' based on the
  @REM   SVN/GIT repository revision number.  Note that the revision
  @REM   number for SVN repositories is always numeric anyway but for
  @REM   GIT repositories we must calculate it based on total number
  @REM   of commits since GIT "revision numbers" are actually hashes.
  @REM ----------------------------------------------------------------

  @REM Latest buggy version of SubWCRev CRASHES if .svn dir doesn't exist!

  call :isdir ".svn"
  if defined isdir (set "have_dot_svn_dir=yes" ) else (set "have_dot_svn_dir=")

  call :isdir ".git"
  if defined isdir (set "have_dot_git_dir=yes" ) else (set "have_dot_git_dir=")

  if defined have_dot_svn_dir goto :set_VERSION_try_SubWCRev
  if defined have_dot_git_dir goto :set_VERSION_try_GIT
                              goto :set_VERSION_try_XXX

::-----------------------------------------------------------------------------
::                    set_VERSION_try_SubWCRev
::-----------------------------------------------------------------------------
:set_VERSION_try_SubWCRev

  if not defined have_dot_svn_dir (
    goto :set_VERSION_try_GIT
  )

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

::-----------------------------------------------------------------------------
::                      set_VERSION_try_SVN
::-----------------------------------------------------------------------------
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

::-----------------------------------------------------------------------------
::                      set_VERSION_try_GIT
::-----------------------------------------------------------------------------
:set_VERSION_try_GIT

  if not defined have_dot_git_dir (
    goto :set_VERSION_try_XXX
  )

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
  for /f %%a in ('%git% rev-parse --short HEAD') do set "git_hash=-g%%a"

  ::  Count the number of commits and use that as our "build" number

  :: PROGRAMMING NOTE: Windows does not come with "wc" so we cannot
  :: use the typical technique of piping "git log --pretty=..." into
  :: "wc -l" to count the number of commits (which most implementations
  :: get wrong anyway since they typically use "--pretty=format" instead
  :: of "--pretty=tformat" thereby causing "wc -l" to return a count one
  :: less than the actual number of commits) so we need to use a different
  :: technique instead.  Luckily, most Windows versions of git are more
  :: modern than the gits that come with most older Linux distros and
  :: thus very likely support the newer rev-list "--count" option.

  for /f %%a in ('%git% rev-list --count HEAD') do set "VERS_BLD=%%a"

  set "modified_str=%git_hash%%modified_str%"
  set "git_hash="

  %TRACE% VERS_BLD     = %VERS_BLD%
  %TRACE% modified_str = %modified_str%

  goto :set_VERSION_do_set

::-----------------------------------------------------------------------------
::                      set_VERSION_try_XXX
::-----------------------------------------------------------------------------
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

::-----------------------------------------------------------------------------
::                      set_VERSION_do_set
::-----------------------------------------------------------------------------
:set_VERSION_do_set

  if not defined VERS_BLD set "VERS_BLD=0"

  %TRACE%.
  %TRACE%     set_VERSION_do_set
  %TRACE%.
  %TRACE% VERS_MAJ         = %VERS_MAJ%
  %TRACE% VERS_INT         = %VERS_INT%
  %TRACE% VERS_MIN         = %VERS_MIN%
  %TRACE% VERS_BLD         = %VERS_BLD%
  %TRACE% VERS_DEV         = %VERS_DEV%
  %TRACE% dev_string       = %dev_string%
  %TRACE% modified_str     = %modified_str%
  %TRACE% VERSION          = %VERSION%
  %TRACE%.

  ::--------------------------------------------------------------
  ::  Make it clear this Hercules is NOT any of the other ones
  ::  by embedding "-SDL" into the "VERSION" string. Same idea
  ::  for "-DEV" indicating a development version.
  ::--------------------------------------------------------------

  set VERSION="%VERS_MAJ%.%VERS_INT%.%VERS_MIN%.%VERS_BLD%-SDL%dev_string%%modified_str%"

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

  ::  PROGRAMMING NOTE: since 'VERSION' is a quoted string we can't use the
  ::  normal set "var=value" set command syntax since that would result in
  ::  a syntax error (i.e. set "var="value"").  Therefore, not quotes.
  ::
  ::  HOWEVER, not using quotes on a set command is dangerous since the set
  ::  command blindly sets the specified variable to whatever follows the
  ::  equal sign, including any accidental trailing blanks.  Thus we must
  ::  surround the entire set command itself within parentheses to delimit
  ::  to the batch file parser the start and end of the command (and thus
  ::  the actual end of the value the variable is being 'set' to as well).

  endlocal && (set VERSION=%VERSION%) && set "VERS_MAJ=%VERS_MAJ%" && set "VERS_INT=%VERS_INT%" && set "VERS_MIN=%VERS_MIN%" && set "VERS_BLD=%VERS_BLD%" && exit /b %maxrc%

::-----------------------------------------------------------------------------
