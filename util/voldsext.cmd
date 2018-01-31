@if defined TRACEON (@echo on) else (@echo off)

  REM  If this batch file works, then it was written by Fish.
  REM  If it doesn't then I don't know who the heck wrote it.

  setlocal
  goto :init

::-----------------------------------------------------------------------------
::                            VOLDSEXT.CMD
::-----------------------------------------------------------------------------
:help

  echo.
  echo     NAME
  echo.
  echo         %~nx0   --   Volume datasets extract
  echo.
  echo     SYNOPSIS
  echo.
  echo         %~nx0      ^<filename^>  [{sf=}shadowname]  [dsname]  [memname]
  echo.
  echo     ARGUMENTS
  echo.
  echo         filename     Name of dasd image file.
  echo.
  echo     OPTIONS
  echo.
  echo         shadowname   Shadow file name.  See NOTES for format.
  echo.
  echo         dsname       Dataset holding the member(s) to be extracted.
  echo.
  echo         memname      Specific member to be extracted.
  echo.
  echo     NOTES
  echo.
  echo         The entire shadowname argument (including the "sf=" prefix)
  echo         should be surrounded with double quotes if the path contains
  echo         any blanks (e.g. "sf=path with blanks"). This is different
  echo         than the way such arguments are passed to Hercules utilities.
  echo         Hercules utilities use the format sf="path with blanks" where
  echo         only the path but not the sf= prefix is quoted.  When passed
  echo         to %~nx0 however, use the format "sf=path with blanks"
  echo         where the entire argument is quoted if it contains any blanks.
  echo.
  echo         %~n0 first uses dasdls to obtain a list of all datasets on
  echo         the specified volume and then for each listed dataset (or for
  echo         only the specific dataset if dsname is specified), it uses the
  echo         dasdcat utility to extract either the specified member or all
  echo         members into (a) separate file(s).
  echo.
  echo         Each extracted member is placed in its own file in a directory
  echo         created with the same name as the dataset it came from, which
  echo         itself is created within a directory created with the same name
  echo         as the actual VOLSER name of the dasd image file.
  echo.
  echo     EXIT STATUS
  echo.
  echo         0    Success
  echo         1    Failure
  echo.
  echo     AUTHOR
  echo.
  echo         "Fish" (David B. Trout)
  echo.
  echo     VERSION
  echo.
  echo         1.1  (January 29, 2016)

  set /a "rc=1"
  goto :exit

::-----------------------------------------------------------------------------
::                               INIT
::-----------------------------------------------------------------------------
:init

  @REM Define some constants...

  set "return=goto :EOF"
  set "break=goto :break"
  set "TRACE=if defined DEBUG echo"
  set "@nx0=%~nx0"
  set /a "rc=0"
  set "basedir=%cd%"

  set "dasdls=dasdls.exe"
  set "dasdcat=dasdcat.exe"

  set "filename="

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

  call :load_tools
  if not "%rc%" == "0" goto :exit

  set "filename="
  set "shadowname="
  set "sfn_argument="
  set "dsname="
  set "memname="

  goto :parse_options_loop

::-----------------------------------------------------------------------------
::                             load_tools
::-----------------------------------------------------------------------------
:load_tools

  call :fullpath "%dasdls%" dasdls
  if not defined # (
    echo ERROR: "%dasdls%" not found. 1>&2
    set /a "rc=1"
  )

  call :fullpath "%dasdcat%" dasdcat
  if not defined # (
    echo ERROR: "%dasdcat%" not found. 1>&2
    set /a "rc=1"
  )

  %return%

::-----------------------------------------------------------------------------
::                             fullpath
::-----------------------------------------------------------------------------
:fullpath

  set "@=%path%"
  set "path=.;%path%"
  set "#=%~$PATH:1"
  set "path=%@%"
  if defined # set "%~2=%#%"
  %return%

::-----------------------------------------------------------------------------
::                          parse_options_loop
::-----------------------------------------------------------------------------
:parse_options_loop

  if "%~1" == "" goto :options_loop_end

  set "opt=%~1"
  set "optflag=%opt:~0,1%"
  set "optname=%opt:~1%"

  shift /1

  if "%optflag%" == "-" goto :parse_opt
  if "%optflag%" == "/" goto :parse_opt

  if defined filename goto :parse_got_filename
  set "filename=%opt%"
  goto :parse_options_loop

:parse_got_filename

  if defined shadowname           goto :parse_got_shadowname
  if /i not "%opt:~0,3%" == "sf=" goto :parse_got_shadowname

  set "shadowname=%opt:~3%"
  goto :parse_options_loop

:parse_got_shadowname

  if defined dsname goto :parse_got_dsname
  set "dsname=%opt%"
  goto :parse_options_loop

:parse_got_dsname

  if defined memname goto :parse_got_memname
  set "memname=%opt%"
  goto :parse_options_loop

:parse_got_memname

  goto :parse_bad_opt

:parse_opt

  goto :parse_bad_opt

:parse_bad_opt

  echo ERROR: Unrecognized option '%opt%' 1>&2
  set /a "rc=1"
  goto :parse_options_loop

:options_loop_end

  goto :validate_args

::-----------------------------------------------------------------------------
::                            validate_args
::-----------------------------------------------------------------------------
:validate_args

  if not defined filename goto :missing_filename

  call :fullpath "%filename%" filename
  if not defined # (
    echo ERROR: "%filename%" not found. 1>&2
    set /a "rc=1"
  )

  goto :validate_shadowname

:missing_filename

  echo ERROR: filename is required 1>&2
  set /a "rc=1"

  goto :validate_shadowname

:validate_shadowname

  if not defined shadowname goto :validate_args_done

  call :fullpath "%shadowname%" shadowname
  if not defined # (
    echo ERROR: "%shadowname%" not found. 1>&2
    set /a "rc=1"
  ) else (

    @REM -------------------------------------------------------------
    @REM
    @REM                     PROGRAMMING NOTE #1
    @REM
    @REM  The "sf=" command-line argument is quoted differently when
    @REM  passed to Hercules utilities than the way it is quoted when
    @REM  passed to this batch file.
    @REM
    @REM  When passed to this batch file, then entire argument must
    @REM  be surrounded in double quotes (if it contains any blanks),
    @REM
    @REM  whereas only the shadow filename itself should be surrounded
    @REM  in quotes when passed to Hercules dasdls/dasdcat utilities:
    @REM
    @REM
    @REM      "sf=path with blanks"     (this batch file's format)
    @REM      sf="path with blanks"     (Hercules utilities format)
    @REM
    @REM -------------------------------------------------------------
    @REM
    @REM                     PROGRAMMING NOTE #2
    @REM
    @REM  Because the 'for' statement in the 'dasdls_sub' subroutine
    @REM  specifies '=' as its delimiter ("delims=="), the shadowname
    @REM  argument cannot be directly passed to dasdls since the "sf="
    @REM  prefix contains an '=' resulting in it being removed because
    @REM  it's seen as a delimiter by the 'for' statement (believe it
    @REM  or not!) resulting in a string such as "sf C:\shadow path"
    @REM  being passed to dasdls (note missing '=' following "sf"!!)
    @REM  causing dasdls to either crash or not use the shadow file.
    @REM
    @REM  As a workaround we set a new variable equal to the exact
    @REM  command line argument string to be passed to dasdls thereby
    @REM  hiding the '=' altogether from Microsoft's brain dead 'for'
    @REM  statement parser.
    @REM
    @REM  (Why the FRICK it's using the specified delimiter(s) to parse
    @REM  the COMMAND LINE with instead of only using it to parse the
    @REM  command's resulting OUTPUT with is anyone's guess!!)
    @REM
    @REM -------------------------------------------------------------

    set sfn_argument=sf="%shadowname%"
  )

  goto :validate_args_done

:validate_args_done

  if not "%rc%" == "0" (
    goto :exit
  )

  goto :BEGIN

::-----------------------------------------------------------------------------
::                               BEGIN
::-----------------------------------------------------------------------------
:BEGIN

  echo.
  call :dasdls_sub
  goto :exit

::-----------------------------------------------------------------------------
::                            dasdls_sub
::-----------------------------------------------------------------------------
:dasdls_sub

  set "did_volser_line="
  set "abort="
  set "ocd=%cd%"
  set /a "rc=0"
  set /a "dsncnt=0"

  if not defined sfn_argument goto :dasdls_filename_only

  @REM  Refer to the PROGRAMMING NOTE(s) much further above. We MUST use an
  @REM  '=' for our delimiter in the below 'for' statement so we can extract
  @REM  the VOLSER name from dasdls's first line of output.

  for /f "tokens=1* delims==" %%a in ('""%dasdls%" "%filename%" %sfn_argument%" 2^>nul') do (
    call :process_dsn "%%a" "%%b"
    if defined abort %break%
  )

:break
  %break%

:dasdls_filename_only

  for /f "tokens=1* delims==" %%a in ('""%dasdls%" "%filename%"" 2^>nul') do (
    call :process_dsn "%%a" "%%b"
    if defined abort %break%
  )

:break

  echo ** Total datasets processed on volume '%volser%' = %dsncnt% **

  if /i not "%cd%" == "%ocd%" popd

  %return%

::-----------------------------------------------------------------------------
::                            process_dsn
::-----------------------------------------------------------------------------
:process_dsn

  set "save_volser=%volser%"
  set "dsn=%~1"
  set "volser=%~2"

  if not defined did_volser_line (

    @REM  The first line of dasdls contains the volser,
    @REM  but we need to skip any Hercules msgs first...

    call :is_hercules_msg "%dsn%"
    if defined # %return%
    set "did_volser_line=1"

    if not exist "%volser%" mkdir "%volser%"
    call :isdir  "%volser%"

    if not defined # (
      echo ERROR: Could not create directory "%volser%" or file exists and is not a directory. 1>&2
      set /a "rc=1"
      set "abort=yes"
    ) else (
      echo Processing volume '%volser%' ...
      pushd "%volser%"
    )

    %return%
  )

  @REM  Remaining lines of dasdls are the dataset names...

  set "volser=%save_volser%"

  @REM  Are we extracting from all datasets,
  @REM  or from just one specific dataset?

  if not defined dsname (
    @REM  Extract from ALL datasets...
    call :process_dsname
    set /a "dsncnt=dsncnt+1"
  ) else (
    @REM  Extract from only one specific dataset...
    if /i "%dsn%" == "%dsname%" (
      call :process_dsname
      set /a "dsncnt=dsncnt+1"
    )
  )

  %return%

::-----------------------------------------------------------------------------
::                         is_hercules_msg
::-----------------------------------------------------------------------------
:is_hercules_msg

  set "msgid=%~1"
  set "msgid=%msgid:~0,9%"
  set "#="
  if /i not "%msgid:~0,3%" == "HHC" %return%
  call :isnum "%msgid:~3,-1%"
  if not defined isnum %return%
  set "#=1"
  if /i "%msgid:~-1%" == "I" %return%
  if /i "%msgid:~-1%" == "W" %return%
  if /i "%msgid:~-1%" == "E" %return%
  if /i "%msgid:~-1%" == "D" %return%
  if /i "%msgid:~-1%" == "S" %return%
  if /i "%msgid:~-1%" == "A" %return%
  set "#="
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
::                            process_dsname
::-----------------------------------------------------------------------------
:process_dsname

  echo Processing volume '%volser%' dataset '%dsn%' ...

  if not exist "%dsn%" mkdir "%dsn%"
  call :isdir  "%dsn%"

  if not defined # (
    echo ERROR: Could not create directory "%dsn%" or file exists and is not a directory. 1>&2
    set /a "rc=1"
    %return%
  )

  pushd "%dsn%"

  set "member="
  set /a "memcnt=0"

  for /f "tokens=1*" %%a in ('""%dasdcat%" -i "%filename%" %sfn_argument% %dsn%/?" 2^>nul') do (

    @REM  Member names can't have blanks so if %%b is non-empty
    @REM  then we know it's not a member name line.  Otherwise
    @REM  if %%b is empty then we know it's a member name line.

    if "%%b" == "" (

      @REM  Are we extracting all members from this dataset,
      @REM  or just one specific member?

      if not defined memname (
        @REM  Extract ALL members from this dataset...
        echo Extracting volume '%volser%' dataset '%dsn%' member '%%a' ...
        call :process_member "%%a"
        set /a "memcnt=memcnt+1"
      ) else (
        @REM  Extract only one specific member...
        if /i "%%a" == "%memname%" (
          echo Extracting volume '%volser%' dataset '%dsn%' member '%%a' ...
          call :process_member "%%a"
          set /a "memcnt=memcnt+1"
          %break%
        )
      )
    )
  )

:break

  popd

  if defined memname (
    if %memcnt% EQU 0 (
      echo ERROR: Member '%memname%' not found in dataset '%dsn%' on volume '%volser%'.  1>&2
    )
  )

  echo ** Total members extracted from volume '%volser%' dataset '%dsn%' = %memcnt% **
  %return%

::-----------------------------------------------------------------------------
::                             isdir
::-----------------------------------------------------------------------------
:isdir

  set "#=%~a1"
  if not defined # %return%
  if /i "%#:~0,1%" == "d" %return%
  set "#="
  %return%

::-----------------------------------------------------------------------------
::                            process_member
::-----------------------------------------------------------------------------
:process_member

  set "member=%~1"

  @REM  Try to cat the member first in 80-byte card image format,
  @REM  but if that doesn't work (which it won't for load modules
  @REM  for example), then cat it in as-is binary format...

  "%dasdcat%" -i "%filename%" %sfn_argument% %dsn%/%member%:cs  >  "%member%.txt"  2>nul
  set /a "rc=%errorlevel%"
  if %rc% EQU 0 %return%

  del "%member%.txt"
  "%dasdcat%" -i "%filename%" %sfn_argument% %dsn%/%member%  >  "%member%"  2>nul
  set /a "rc=%errorlevel%"

  %return%

::-----------------------------------------------------------------------------
::                                EXIT
::-----------------------------------------------------------------------------
:exit

  endlocal & exit /b %rc%

::-----------------------------------------------------------------------------
