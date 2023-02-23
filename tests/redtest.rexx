/*********************************************************************/
/*                                                                   */
/*     redtest.rexx   ---   Reduce Hercules Test Run Logfile         */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/*  This script reads and processes the logfile that was created     */
/*  from a Hercules test run.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/*  This file was put into the public domain 2015-10-05 by John P.   */
/*  Hartmann.  You can use it for anything you like, as long as      */
/*  this notice remains.                                             */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/*  Rewritten & bugs fixed by "Fish" (David B. Trout) Apr-May 2017   */
/*                                                                   */
/*********************************************************************/

--Trace Intermediate        -- (uncomment to enable debug tracing)

Signal On NoValue

/*********************************************************************/
/*                        Initialization                             */
/*********************************************************************/

TRUE  = 1
FALSE = 0

quiet = FALSE
values. = ''

Call InitVars

Parse Arg  logfile options

/* Parse options which, except for 'quiet', are "name=value" strings */

Do while options \= ''

   Parse Var options  optval options
   Parse Var optval   varname '=' +0 equals +1 varvalue

   Select
      When optval = 'quiet' Then quiet = TRUE
      When equals =   '='   Then Call SetVar varname varvalue
      Otherwise
      Do
         Say 'ERROR: Invalid argument: 'optval options
         Exit -1
      End
   End
End

/*********************************************************************/
/*                           M A I N                                 */
/*********************************************************************/

stmt = ReadLog()                -- (this...)
Do While stmt \= ''             -- (...is...)
   Call ProcessStmt stmt        -- (...our...)
   stmt = ReadLog()             -- (...main...)
End                             -- (...processing loop)

/* DONE! Report total number of Testcases processed
   and the total number of successes and failures.
*/
If fails.1 = 0 & catast = 0 Then msg = 'All OK.'
Else msg = fails.1 'failed;' fails.0 'OK.'
Say 'Did' numtests 'tests. ' msg

/* Report catastrophic/serious errors if any */

If catast > 0
   Then Say '>>>>>' catast 'test(s) failed catastrophically.  Bug in Hercules or Testcase is likely. <<<<<'

/* Report malformed test cases if any*/

If numtests \= numdones Then
Do
  Say ''
  Say 'Tests malformed:' numtests "'*Testcase' orders met, but" numdones "'*Done' orders met."
  Say 'This is either a bug in Hercules or in one or more test cases.'
  Say 'Please report this error to your Hercules vendor.'
End

/* Exit with number of failed *Testcases (which ideally should be 0) */

Exit fails.1

/*********************************************************************/
/*                            InitVars                               */
/*********************************************************************/

InitVars:

numtests  = 0           -- No '*Testcase' orders encountered yet
numdones  = 0           -- No '*Done' orders encountered yet
comparing = FALSE       -- No '*Compare' order encountered yet
fails.    = 0           -- ".0" (false) is total #of successes,
                        -- ".1" (true) is total #of failures
catast    = FALSE       -- No catastrophic failures yet
lineno    = 0           -- Line number of input file

testcase = '<unknown>'
package = ''
prev_package = ''
lookingahead = FALSE

/* The 'active' flag is used to suppress the processing of testcase
   orders when an 'if' statement (or its 'else' branch) is evaluated
   to 'false'.  When the program first starts, there is no 'if/else'
   block that is active, so the 'active' flag initially defaults to
  'true' so that we can immediately begin processing statements.
*/
active = TRUE           -- Statement processing is active by default
ifnum = 0               -- If > 0, then we're inside an if/else block
ifcond.ifnum = active   -- Current 'active' state for if/else block;
                        -- Flip-flopped when 'else' is encountered
ifprev.ifnum = active   -- Active state of previous 'if' block
ifelse.ifnum = FALSE    -- Set to TRUE whenever 'else' encountered

rest = ''               -- (because 'BegTest' requires it)
Call BegTest            -- (Initialize '*Testcase' variables)
numtests = 0            -- (because 'BegTest' bumped it)

/* The following "values." variables are defined by the ProcessStmt
   subroutine as a result of processing certain messages that are
   issued by Hercules upon startup.  They provide a means for '*If'
   directives to test for the availability of certain build options
   and packages that might not be available on all Hercules systems
   depending how the user chose to build Hercules, thereby allowing
   the Testcase author to design test cases that skip certain tests
   depending on whether a given build option or package happens to
   be available and/or installed or not.
*/

save_quiet = quiet
quiet = TRUE

Call SetVar 'can_s370_mode'     FALSE
Call SetVar 'can_esa390_mode'   FALSE
Call SetVar 'can_zarch_mode'    FALSE
Call SetVar 'CCKD_BZIP2'        FALSE
Call SetVar 'cmpxchg1'          FALSE
Call SetVar 'cmpxchg16'         FALSE
Call SetVar 'cmpxchg4'          FALSE
Call SetVar 'cmpxchg8'          FALSE
Call SetVar 'CONFIG_INCLUDE'    FALSE
Call SetVar 'CONFIG_SYMBOLS'    FALSE
Call SetVar 'externalgui'       FALSE
Call SetVar 'fetch_dw'          FALSE
Call SetVar 'HAO'               FALSE
Call SetVar 'hatomics'          ''
Call SetVar 'HDL'               FALSE
Call SetVar 'HET_BZIP2'         FALSE
Call SetVar 'HTTP'              FALSE
Call SetVar 'IEEE'              FALSE
Call SetVar 'IPV6'              FALSE
Call SetVar 'keepalive'         'Full'
Call SetVar 'libraries'         ''
Call SetVar 'locking_model'     ''
Call SetVar 'max_cpu_engines'   0
Call SetVar 'platform'          ''
Call SetVar 'ptrsize'           '-1'
Call SetVar 'quiet'             save_quiet
Call SetVar 'NLS'               FALSE
Call SetVar 'regex'             FALSE
Call SetVar 'rexx_ErrPrefix'    FALSE
Call SetVar 'rexx_Extensions'   ''
Call SetVar 'rexx_Mode'         ''
Call SetVar 'rexx_MsgLevel'     FALSE
Call SetVar 'rexx_MsgPrefix'    FALSE
Call SetVar 'rexx_package'      ''
Call SetVar 'rexx_Resolver'     FALSE
Call SetVar 'rexx_RexxPath'     ''
Call SetVar 'rexx_SOURCE'       ''
Call SetVar 'rexx_supported'    FALSE
Call SetVar 'rexx_SysPath'      FALSE
Call SetVar 'rexx_VERSION'      ''
Call SetVar 'shared_devices'    FALSE
Call SetVar 'sighandling'       FALSE
Call SetVar 'sqrtl'             FALSE
Call SetVar 'store_dw'          FALSE
Call SetVar 'syncio'            FALSE
Call SetVar 'SYSTEM_SYMBOLS'    FALSE
Call SetVar 'threading_model'   ''
Call SetVar 'ZLIB'              FALSE

quiet = save_quiet
Return

/*********************************************************************/
/*                          ReadLog                                  */
/*********************************************************************/
/* This is the primary "read a statement from the logfile" routine.  */
/* It reads a line from the logfile and removes and debugging prefix */
/* and/or timestamp if present.  It always returns a 'HHC' Hercules  */
/* message except at EOF whereupon is returns an '' empty string.    */
/*********************************************************************/

ReadLog:

Do Forever

   If LINES( logfile ) <= 0             -- (if no lines remain)
      Then Return ''                    -- (return empty string)

   stmt = STRIP( LINEIN( logfile ))     -- (read logfile statement)
   lineno = lineno + 1                  -- (what line number it is)

   /* Remove the logfile "HH:MM:SS" timestamp prefix, if present */

   If isHHMMSS( WORD( stmt, 1 )) Then
      stmt = STRIP( SUBWORD( stmt, 2 ))

   /* Remove any debugging prefix */

   Parse Value WORD( stmt, 1 ) With foo '(' bar ')' +0 foobar
   If foobar = ')' Then Do              -- Debugging Prefix?
      Parse Var stmt  . stmt            -- Reparse to discard it
      stmt = STRIP( stmt )              -- Trim whitespace again
   End

   /* Examine just the Hercules message number */

   Parse Var stmt  msg .                -- Parse message number
   pos = VERIFY( msg, '0123456789:' )   -- Skip past timestamp
   If pos = 0                           -- Nothing past timestamp?
      Then Iterate                      -- Ignore this statement
   stmt = STRIP( SUBSTR( stmt, pos ))   -- Remove the timestamp

   /* The following ensures we will never return an empty string
      except at EOF.
   */
   If LEFT( stmt, 3 ) = 'HHC'           -- Hercules message?
      Then Return stmt                  -- Then process stmt
End

/*********************************************************************/
/*                          ProcessStmt                              */
/*********************************************************************/
/* Process a Hercules message.  Some messages (such as those issued  */
/* at startup) simply set a "values." stem variable value that sub-  */
/* sequent '*If' directives can then test.  Others (such as display  */
/* registers or storage or program check or disabled wait messages)  */
/* are parsed to extract various information contained within them   */
/* which '*Compare' and '*Want' etc. test directives need in order   */
/* to determine whether the given test has succeeded or failed. If   */
/* the message is a command-echo message HHC01603I that is echoing   */
/* a '*' loud comment, it is further processed as a potential test   */
/* order by the "DoOrder" subroutine responsible for handling all    */
/* testing directives *Testcase, *If, *Else, *Want, *Compare, etc.   */
/*********************************************************************/

ProcessStmt: /*  msg     will be the Hercules message number
                 verb    will be the first word of the message
                 rest    is everything else following the first word */
Parse Arg stmt
Parse Var stmt   msg verb rest

Select

   When msg = 'HHC00007I' Then Nop -- "Previous message from function '%s' at %s(%d)"
   When msg = 'HHC00809I' Then Call HHC00809I   -- (disabled wait)
   When msg = 'HHC01417I' Then Call HHC01417I   -- (startup)
   When msg = 'HHC01603I' Then Call HHC01603I   -- (command echo)
   When msg = 'HHC02269I' Then Call HHC02269I   -- (gp registers)
   When msg = 'HHC02270I' Then Call HHC02270I   -- (fpr registers)
   When msg = 'HHC02271I' Then Call HHC02271I   -- (control regs)
   When msg = 'HHC02290I' Then Call HHC02290I   -- (real, absolute)
   When msg = 'HHC02291I' Then Call HHC02291I   -- (virtual)

   When LEFT( msg,  8 ) = 'HHC02332'  Then Call HHC02332x   -- (runtest script timeout)
   When LEFT( verb, 5 ) = 'REXX('     Then Call hRexx

   /*          HHC02277I Prefix register: %s */
   When msg = 'HHC02277I' Then
      Parse Var rest  . prefix .

   /*          HHC00801I Processor %s%02X: %s%s%s interruption code %4.4X  ilc %d%s */
   When msg = 'HHC00801I' Then
      Parse Var rest  ' code ' pgmchk . ' ilc'

   /*          HHC00803I Processor %s%02X: program interrupt loop PSW %s */
   When msg = 'HHC00803I' Then
      waitseen = TRUE

   /*          HHC01405E Script file %s not found */
   When msg = 'HHC01405E' Then
      Call OkayOrNot FALSE, stmt

   /* Save all other messages for '*Hmsg' once a '*Compare' is seen */
   When comparing Then
      Do
         msgnum = lastmsg.0 + 1
         lastmsg.msgnum = stmt
         lastmsg.0 = msgnum
      End

   Otherwise          -- (not anything we're interested in)
      Nop             -- (ignore this statement altogether)
End
Return

/*********************************************************************/
/*                          HHC01417I                                */
/*********************************************************************/

HHC01417I:      -- Hercules startup message

Select

   When verb = 'Machine' Then
   Do
      Parse Var rest  'dependent assists:' rest
      Do while rest \= ''
         Parse Var rest  assist_type rest
         If LEFT( assist_type, 9 ) = 'hatomics=' Then
         Do
            atomic_type = SUBSTR( assist_type, 10 )
            If atomic_type \= 'UNKNOWN' Then
               Call SetVar 'hatomics' atomic_type
         End
         Else
            Call SetVar assist_type TRUE
      End
   End


   When verb = 'Modes:' Then
   Do while rest \= ''
      Parse Var rest  archmode rest
      Select
         When archmode = 'S/370'   Then Call SetVar 'can_s370_mode'   TRUE
         When archmode = 'ESA/390' Then Call SetVar 'can_esa390_mode' TRUE
         When archmode = 'z/Arch'  Then Call SetVar 'can_zarch_mode'  TRUE
         Otherwise Nop
      End
   End


   When verb = 'Max' Then
   Do
      Parse Var rest  'CPU Engines: ' engines
      Call SetVar 'max_cpu_engines' engines
   End


   When verb = 'Using' Then
   Select
      When rest = 'Fish threads Threading Model'       Then Call SetVar 'threading_model' 'Fish'
      When rest = 'POSIX threads Threading Model'      Then Call SetVar 'threading_model' 'POSIX'
      When rest = 'Error-Checking Mutex Locking Model' Then Call SetVar 'locking_model'   'Error'
      When rest = 'Normal Mutex Locking Model'         Then Call SetVar 'locking_model'   'Normal'
      When rest = 'Recursive Mutex Locking Model'      Then Call SetVar 'locking_model'   'Recursive'
      When rest = 'shared libraries'                   Then Call SetVar 'libraries'       'Shared'
      When rest = 'static libraries'                   Then Call SetVar 'libraries'       'Static'
      Otherwise Nop
   End


   When verb = 'With' | verb = 'Without' Then
   Do
      If verb = 'With' Then YesOrNo = TRUE; Else YesOrNo = FALSE
      Select
         When rest = 'Partial TCP keepalive support' Then Call SetVar 'keepalive'     'Partial'
         When rest = 'Basic TCP keepalive support'   Then Call SetVar 'keepalive'     'Basic'
         When rest = 'TCP keepalive support'         Then Call SetVar 'keepalive'     'Full'
         When rest = 'Syncio support'                Then Call SetVar 'syncio'         YesOrNo
         When rest = 'Shared Devices support'        Then Call SetVar 'shared_devices' YesOrNo
         When rest = 'Dynamic loading support'       Then Call SetVar 'HDL'            YesOrNo
         When rest = 'External GUI support'          Then Call SetVar 'externalgui'    YesOrNo
         When rest = 'Signal handling'               Then Call SetVar 'sighandling'    YesOrNo
         When rest = 'National Language Support'     Then Call SetVar 'NLS'            YesOrNo
         When rest = 'Automatic Operator support'    Then Call SetVar 'HAO'            YesOrNo
         When rest = 'Regular Expressions support'   Then Call SetVar 'regex'          YesOrNo
         When rest = 'IPV6 support'                  Then Call SetVar 'IPV6'           YesOrNo
         When rest = 'HTTP Server support'           Then Call SetVar 'HTTP'           YesOrNo
         When rest = 'IEEE support'                  Then Call SetVar 'IEEE'           YesOrNo
         When rest = 'sqrtl support'                 Then Call SetVar 'sqrtl'          YesOrNo
         When rest = 'CCKD BZIP2 support'            Then Call SetVar 'CCKD_BZIP2'     YesOrNo
         When rest = 'HET BZIP2 support'             Then Call SetVar 'HET_BZIP2'      YesOrNo
         When rest = 'ZLIB support'                  Then Call SetVar 'ZLIB'           YesOrNo
         When (rest = 'Object REXX support' | rest = 'Regina REXX support') Then
         Do
            If \GetVar('rexx_supported')      Then Call SetVar 'rexx_supported' YesOrNo
         End
         When rest = 'CONFIG_INCLUDE support' Then Call SetVar 'CONFIG_INCLUDE' YesOrNo
         When rest = 'SYSTEM_SYMBOLS support' Then Call SetVar 'SYSTEM_SYMBOLS' YesOrNo
         When rest = 'CONFIG_SYMBOLS support' Then Call SetVar 'CONFIG_SYMBOLS' YesOrNo
         Otherwise Nop
      End
   End

   Otherwise Nop
End
Return

/*********************************************************************/
/*                          hRexx                                    */
/*********************************************************************/

hRexx:           -- Hercules Rexx message
/*
HHC17525I REXX(OORexx) Rexx has been started/enabled
HHC17528I REXX(OORexx) VERSION: REXX-ooRexx_4.2.0(MT)_64-bit 6.04 22 Feb 2014
HHC17529I REXX(OORexx) SOURCE:  WindowsNT
HHC17500I REXX(OORexx) Mode            : Subroutine
HHC17500I REXX(OORexx) MsgLevel        : Off
HHC17500I REXX(OORexx) MsgPrefix       : Off
HHC17500I REXX(OORexx) ErrPrefix       : Off
HHC17500I REXX(OORexx) Resolver        : On
HHC17500I REXX(OORexx) SysPath    (44) : On
HHC17500I REXX(OORexx) RexxPath   ( 0) :
HHC17500I REXX(OORexx) Extensions ( 8) : .REXX;.rexx;.REX;.rex;.CMD;.cmd;.RX;.rx
*/
If Left( rest, 8 ) = 'VERSION:' Then
Do
   Parse Var verb  'REXX(' package ')'
   If package \= prev_package Then
   Do
      Call SetVar 'rexx_package' package
      prev_package = package
   End
   If package \= '' Then Call SetVar 'rexx_VERSION' SUBSTR( rest, 10 )
End
Else
Do
   If package \= '' Then
   Do
      If SUBSTR( rest, 19 ) = 'On' Then OnOff = TRUE; Else OnOff = FALSE
      Select
         When Left( rest,  7 ) = 'SOURCE:'    Then Call SetVar 'rexx_SOURCE'     SUBSTR( rest, 10 )
         When Left( rest,  4 ) = 'Mode'       Then Call SetVar 'rexx_Mode'       SUBSTR( rest, 19 )
         When Left( rest,  8 ) = 'MsgLevel'   Then Call SetVar 'rexx_MsgLevel'   OnOff
         When Left( rest,  9 ) = 'MsgPrefix'  Then Call SetVar 'rexx_MsgPrefix'  OnOff
         When Left( rest,  9 ) = 'ErrPrefix'  Then Call SetVar 'rexx_ErrPrefix'  OnOff
         When Left( rest,  8 ) = 'Resolver'   Then Call SetVar 'rexx_Resolver'   OnOff
         When Left( rest,  7 ) = 'SysPath'    Then Call SetVar 'rexx_SysPath'    OnOff
         When Left( rest,  8 ) = 'RexxPath'   Then Call SetVar 'rexx_RexxPath'   SUBSTR( rest, 19 )
         When Left( rest, 10 ) = 'Extensions' Then Call SetVar 'rexx_Extensions' SUBSTR( rest, 19 )
         Otherwise Nop
      End
   End
End
Return

/*********************************************************************/
/*                           SetVar                                  */
/*********************************************************************/
/* Set an internal "values." stem variable value                     */
/*********************************************************************/

SetVar:
Parse Arg varname varvalue          -- (no upper so 'varvalue' exact)
UPPER_varname = UPPER( varname )    -- (UPPER for case insensitivity)
values.UPPER_varname = varvalue
If quiet Then Return
Say LEFT( 'Variable $'varname, 28 )' set to "'values.UPPER_varname'"'
Return

/*********************************************************************/
/*                           GetVar                                  */
/*********************************************************************/
/* Return an internal "values." stem variable value                  */
/*********************************************************************/

GetVar:
Parse UPPER Arg UPPER_varname       -- (UPPER for case insensitivity)
varvalue = values.UPPER_varname
return varvalue

/*********************************************************************/
/*                    HHC02290I / HHC02291I                          */
/*********************************************************************/
/* HHC02290I A:0000000000001000  K:06                                */
/* HHC02290I R:00001000  00000000 00000000 00000000 00000000  .....  */
/*********************************************************************/

HHC02290I:      -- (display real or absolute storage)
HHC02291I:      -- (display virtual storage)

If \comparing Then Return

Parse Var rest  'K:' key .

If key \= '' Then
   Do
      storkey = key
      keyaddr = SUBSTR( verb, 3 )
   End
Else
   Do
      storaddr = verb
      Parse Var rest  storbytes +36
   End
Return

/*********************************************************************/
/*                          HHC02332x                                */
/*********************************************************************/
/* HHC02332x Script %d: test: timeout                                */
/*********************************************************************/

HHC02332x:
If timeoutok Then Return
catast = catast + 1     -- runtest timeout value too short
Call OkayOrNot FALSE, 'Testcase timed out in wait.',,
   'This is either a bug in Hercules or in the Testcase.',,
   'Please report this error to your Hercules vendor.'
Return

/*********************************************************************/
/*                          HHC01603I                                */
/*********************************************************************/
/* Hercules echoing of command message                               */
/*********************************************************************/

HHC01603I:
If LENGTH( verb ) > 1 & LEFT( verb, 1 ) = '*' Then Call DoOrder
Return

/*********************************************************************/
/*                           DoOrder                                 */
/*********************************************************************/
/* This subroutine handles all supported testing directives.         */
/*********************************************************************/

DoOrder:

Select

   /* PROGRAMMING NOTE: we must process if/else/endif first,
      before all other orders, since it determines the value
      of our 'active' flag which tells us whether or not we
      should process or ignore any of the other directives.
   */
   When verb = '*If'   Then call DoIf
   When verb = '*Else' Then call DoElse
   When verb = '*Fi'   Then call EndIf

   Otherwise Select

      when \active             Then Nop

      When verb = '*Testcase'  Then call BegTest
      When verb = '*Done'      Then call EndTest
      When verb = '*Want'      Then call DoWant
      When verb = '*Gpr'       Then call WantGPR
      When verb = '*Fpr'       Then call WantFPR
      When verb = '*Cr'        Then call WantCR
      When verb = '*Key'       Then call WantKey
      When verb = '*Prefix'    Then call WantPrefix
      When verb = '*Explain'   Then call DoExplain

      When verb = '*Error'     Then call DoHmsg 'Error'
      When verb = '*Info'      Then call DoHmsg 'Informational'
      When verb = '*Hmsg'      Then call DoHmsg 'Panel message'

      When verb = '*Program'   Then wantpgm = rest
      When verb = '*Timeout'   Then timeoutok = TRUE
      When verb = '*Message'   Then Say rest

      When verb = '*Compare'   Then
      Do
         comparing = TRUE
         lastmsg.0 = 0
      End

      Otherwise
         Call OkayOrNot FALSE, 'Unknown directive:' verb
   End
End
Return

/*********************************************************************/
/*                             DoIf                                  */
/*********************************************************************/
/* Process '*If' order: evaluate the condition to determine whether  */
/* it's true or false, which defines our new 'active' flag state.    */
/*********************************************************************/

DoIf:

if lookingahead Then
Do
   stoplookahead = TRUE
   Return
End

ifnum = ifnum + 1           -- (starting a new 'if' block)
testexpr = rest             -- (save unresolved expression)
testcond = ''               -- (initialize 'if' condition)

/* Build this 'if' statement's conditional expression */

Do While rest \= ''     -- Tokenize expression until nothing remains

   /* Check if there's a '$variable' we need to expand */

   Parse Var rest  foo '$' +0 dolvar rest
   testcond = testcond foo

   /* If no '$variable' remains in the expression,
      then what we have is a complete expression
   */
   If dolvar = '' Then Leave

   /* Otherwise retrieve the variable's value from our "values" stem.
      PLEASE NOTE that we use 'Parse UPPER' for case insensitivity.
   */
   Parse UPPER Var dolvar +1 UPPER_varname .
   varvalue = values.UPPER_varname

   /* Surround it with double-quotes if it's not a whole number */
   If \DATATYPE( varvalue, 'Whole' )
      Then varvalue = '"'varvalue'"'

   /* Append it to our test condition that we're accumulating */
   testcond = testcond varvalue

End

/* Now interpret the condition to see whether it's true or false */
Signal On    Syntax
Signal Off   NoValue
Interpret    'iftrue = 'testcond
Signal On    NoValue

/* Save the 'active' state for the now current 'if' block */
ifprev.ifnum = active
ifcond.ifnum = (iftrue & ifprev.ifnum)
ifelse.ifnum = 0

/* Update current state based on new and previous states */
active = (ifcond.ifnum & ifprev.ifnum)
Return

Syntax:
Say "'*If' statement syntax error:" testexpr
Return

/*********************************************************************/
/*                             DoElse                                */
/*********************************************************************/
/* Process '*Else' order: flip 'active' flag for current 'if' block  */
/*********************************************************************/

DoElse:

if lookingahead Then
Do
   stoplookahead = TRUE
   Return
End

Select

   When ifnum <= 0
      Then Say "No '*If' is active."

   When ifelse.ifnum
      Then Say "'*Else' already seen for '*If' block " ifnum

   Otherwise
      ifelse.ifnum = TRUE
      ifcond.ifnum = \ifcond.ifnum

      /* Update current state based on new and previous states */
      active = (ifcond.ifnum & ifprev.ifnum)

End
Return

/*********************************************************************/
/*                             EndIf                                 */
/*********************************************************************/
/* Process '*Fi' (EndIf) order: restore previous 'active' state      */
/*********************************************************************/

EndIf:

if lookingahead Then
Do
   stoplookahead = TRUE
   Return
End

If ifnum <= 0 Then
Do
   Say "No '*If' block is active."
   Return
End

ifnum = ifnum - 1

/* Update current state based on new and previous states */
active = (ifcond.ifnum & ifprev.ifnum)
Return

/*********************************************************************/
/*                             BegTest                               */
/*********************************************************************/
/* Process '*Testcase' order: initialize testcase variables          */
/*********************************************************************/

BegTest:

if lookingahead Then
Do
   stoplookahead = TRUE
   Return
End

numtests    = numtests + 1      -- (count '*Testcases' orders)
testcase    = STRIP( rest )     -- (save Testcase name)

oks         = 0
rv          = 0

timeoutok   = FALSE
waitseen    = FALSE
comparing   = FALSE

storaddr    = '<unknown>'
keyaddr     = '<unknown>'
storkey     = ''
pgmchk      = ''
wantpgm     = ''
prefix      = ''
gpr.        = ''
fpr.        = ''
cr.         = ''
xr.         = ''

lastinfo    = ''
lastmsg.0   = 0
explain.0   = 0

Return

/*********************************************************************/
/*                             DoWant                                */
/*********************************************************************/
/* Compare storage display against expected contents.                */
/*********************************************************************/

DoWant:
rest = STRIP( rest )
If LEFT( rest, 1 ) = '"' Then
Do
   Parse Var rest  '"' info '"' rest
   info = '"'info'"'
   rest = STRIP( rest )
End
Else
   info = ''
   Call OkayOrNot rest = storbytes, 'Storage compare mismatch.',,
      'Want:' storaddr STRIP( rest ) ' 'info, 'Got: ' storaddr STRIP( storbytes )
Return

/*********************************************************************/
/*                           WantGPR                                 */
/*********************************************************************/
/* Verify contents of a general register.                            */
/*********************************************************************/

WantGPR:
Parse Upper Var rest r what '#' .
Call OkayOrNot gpr.r = what, 'Gpr' r 'compare mismatch.',,
    'Want:' what, 'Got: ' gpr.r
Return

/*********************************************************************/
/*                           WantFPR                                 */
/*********************************************************************/
/* Verify contents of a floating point register.                     */
/*********************************************************************/

WantFPR:
Parse Upper Var rest r what '#' .
Call OkayOrNot fpr.r = what, 'Fpr' r 'compare mismatch.',,
    'Want:' what, 'Got: ' fpr.r
Return

/*********************************************************************/
/*                           WantCR                                  */
/*********************************************************************/
/* Verify contents of a control register.                            */
/*********************************************************************/

WantCR:
Parse Upper Var rest r what '#' .
Call OkayOrNot cr.r = what, 'Cr' r 'compare mismatch.',,
    'Want:' what, 'Got: ' cr.r
Return

/*********************************************************************/
/*                           WantKey                                 */
/*********************************************************************/
/* Verify a page frame's storage key has the expected value.         */
/*********************************************************************/

WantKey:
Parse Upper Var rest what '#' .
If storkey = '' Then
   Call OkayOrNot FALSE, 'No key saved from r/abs/v command.'
Else
Call OkayOrNot storkey = what, 'Key' keyaddr 'compare mismatch.',,
    'Want:' what, 'Got: ' storkey
Return

/*********************************************************************/
/*                          WantPrefix                               */
/*********************************************************************/
/* Verify the prefix register contains the expected value.           */
/*********************************************************************/

WantPrefix:
Parse Upper Var rest what '#' .
If prefix = '' Then
   Call OkayOrNot FALSE, 'No prefix register saved from pr command.'
Else
   Call OkayOrNot prefix = what, 'Prefix register compare mismatch.',,
   'Want:' what, 'Got: ' prefix
Return

/*********************************************************************/
/*                             DoHmsg                                */
/*********************************************************************/
/* Verify the last issued message.                                   */
/*********************************************************************/

DoHmsg:

Parse Arg  msgtype

If lastmsg.0 <= 0 Then
Do
   Call OkayOrNot FALSE, 'No message saved for' rest
   Return
End

If DATATYPE( WORD( rest, 1 ), 'Whole' ) Then
   Parse Var rest  index rest
Else
   index = 0

msgnum = lastmsg.0 - index

If msgnum <= 0 Then
   Call OkayOrNot FALSE, 'No message stored for number' index
Else
   Call OkayOrNot lastmsg.msgnum = rest, msgtype 'message mismatch.',,
      'Want:' rest, 'Got: ' lastmsg.msgnum
Return

/*********************************************************************/
/*                           HHC02269I                               */
/*********************************************************************/
HHC02269I:
Call HHC022nnI 'General', 'GR', 'R';
/*
    Copy stem variable to another stem variable the HARD way!
    Doing a simple "foo. = bar." doesn't work with Regina!!!!
*/
Do i=0 to 15 by 1
   gpr.i = xr.i
End
Return

/*********************************************************************/
/*                           HHC02270I                               */
/*********************************************************************/
HHC02270I:
Call HHC022nnI 'Floating', 'FP', 'FP';
/*
    Copy stem variable to another stem variable the HARD way!
    Doing a simple "foo. = bar." doesn't work with Regina!!!!
*/
Do i=0 to 15 by 1
   fpr.i = xr.i
End
Return

/*********************************************************************/
/*                           HHC02271I                               */
/*********************************************************************/
HHC02271I:
Call HHC022nnI 'Control', 'CR', 'C';
/*
    Copy stem variable to another stem variable the HARD way!
    Doing a simple "foo. = bar." doesn't work with Regina!!!!
*/
Do i=0 to 15 by 1
   cr.i = xr.i
End
Return

/*********************************************************************/
/*                           HHC022nnI                               */
/*********************************************************************/
/* HHC022nnI xxxxxxx registers                                       */
/* HHC022nnI x0=0000000000000060 x1=0000000000000000 x2=0000...      */
/* HHC022nnI x4=0000000000000000 x5=0000000000000000 x6=0000...      */
/* HHC022nnI x8=0000000000000000 x9=0000000000000000 xA=0000...      */
/* HHC022nnI xC=0000000000000000 xD=0000000000000000 xE=0000...      */
/*           --or--                                                  */
/* HHC022nnI xxxxxxx registers                                       */
/* HHC022nnI xR00=000000E0 xR01=00000000 xR02=FFFFFFFF xR03=00000000 */
/* HHC022nnI xR04=00000000 xR05=00000000 xR06=00000000 xR07=00000000 */
/* HHC022nnI xR08=00000000 xR09=00000000 xR10=00000000 xR11=00000000 */
/* HHC022nnI xR12=00000000 xR13=00000000 xR14=C2000000 xR15=00000200 */
/*           --or--                                                  */
/* HHC022nnI xxxxxxx registers                                       */
/* HHC022nnI CP00: x0=0000000000000000 x1=0000000000000000           */
/* HHC022nnI CP00: x2=0000000000000000 x3=0000000000000000           */
/* HHC022nnI CP00: x4=0000000000000000 x5=0000000000000000           */
/* HHC022nnI CP00: x6=0000000000000000 x7=0000000000000000           */
/* HHC022nnI CP00: x8=0000000000000000 x9=0000000000000000           */
/* HHC022nnI CP00: xA=0000000000000000 xB=0000000000000000           */
/* HHC022nnI CP00: xC=0000000000000000 xD=0000000000000000           */
/* HHC022nnI CP00: xE=0000000000000000 xF=0000000000000000           */
/*           --or--                                                  */
/* HHC022nnI xxxxxxx registers                                       */
/* HHC022nnI CP00: xx00=00000000 xx01=00000000 xx02=00000000 ...     */
/* HHC022nnI CP00: xx04=00000000 xx05=00000000 xx06=00000000 ...     */
/* HHC022nnI CP00: xx08=00000000 xx09=00000000 xx10=00000000 ...     */
/* HHC022nnI CP00: xx12=00000000 xx13=00000000 xx14=00000000 ...     */
/*           --or--                                                  */
/* HHC022nnI xxxxxxx registers                                       */
/* HHC022nnI FP08=0000000000000000 FP10=0000000000000000             */
/* HHC022nnI FP09=0000000000000000 FP11=0000000000000000             */
/*********************************************************************/
HHC022nnI:

/* Example call:   Call HHC022nnI  'General',  'GR',  'R' ;   */
/* Example call:   Call HHC022nnI  'Control',  'CR',  'C' ;   */

If verb = ARG(1) Then Return  -- (ignore header line)
regsline = verb rest          -- ("x0=... x1..." as one long string)

/* If NUMCPU > 1 strip the engine name/number prefix from each line */
If LEFT( regsline, 2 ) = 'CP' | ,
   LEFT( regsline, 2 ) = 'AP' | ,
   LEFT( regsline, 2 ) = 'IL' | ,
   LEFT( regsline, 2 ) = 'CF' | ,
   LEFT( regsline, 2 ) = 'IP' Then
Do
   cputype  =      SUBSTR( regsline, 1, 2 )
   cpunum   = X2D( SUBSTR( regsline, 3, 2 ))
   regsline =      SUBSTR( regsline, 7 )
End

/* Register id expressed differently between architectures */
If LEFT( regsline, 2 ) = ARG(2) Then
  regid = ARG(2)
Else
  regid = ARG(3)

parsevarcmd = "Parse Var regsline '" || regid || "' regnum '=' regval regsline"

/* Parse each individual register number and value... */
Do While regsline \= ''

   /* Extract the register number and register value */
   INTERPRET parsevarcmd

   /* Deal with hexadecimal register numbers */
   If \isnum( regnum )
      Then regnum = X2D( regnum )

   regnum = regnum + 0
   xr.regnum = regval

End
Return

/*********************************************************************/
/*                           isnum                                   */
/*********************************************************************/

isnum: procedure -- (is it a numeric value?)
return arg(1) <> "" & datatype(arg(1),"N");

/*********************************************************************/
/*                          isHHMMSS                                 */
/*********************************************************************/

isHHMMSS: procedure -- (is it "HH:MM:SS" logfile timestamp prefix?)
hhmmss = arg(1)
If LENGTH( hhmmss ) \= 8 Then return 0
hh = SUBSTR( hhmmss, 1, 2 )
c1 = SUBSTR( hhmmss, 3, 1 )
mm = SUBSTR( hhmmss, 4, 2 )
c2 = SUBSTR( hhmmss, 6, 1 )
ss = SUBSTR( hhmmss, 7, 2 )
return isnum(hh) & c1=":" & isnum(mm) & c2=":" & isnum(ss) & ,
       hh >= 0 & hh <= 24 & ,
       mm >= 0 & mm <= 60 & ,
       ss >= 0 & ss <= 60;

/*********************************************************************/
/*                           ishex                                   */
/*********************************************************************/

ishex: procedure -- (is it a hexadecimal value?)
return arg(1) <> "" & datatype(arg(1),"X");

/*********************************************************************/
/*                           DoExplain                               */
/*********************************************************************/
/* Save explain text in explain. stem array.                         */
/*********************************************************************/

DoExplain:
numexplains = explain.0 + 1
explain.numexplains = rest
explain.0 = numexplains
Return

/*********************************************************************/
/*                           HHC00809I                               */
/*********************************************************************/
/* HHC00809I Processor %s%02X: disabled wait state %s                */
/*********************************************************************/

HHC00809I:
waitseen = TRUE
Parse Var rest  'wait state' psw1 psw2
waitok = (psw2 = 0) | (RIGHT( psw2, 4 ) = 'DEAD' & pgmchk \= '' & pgmchk = wantpgm)
Call OkayOrNot waitok, 'Received unexpected wait state: ' psw1 psw2
Return

/*********************************************************************/
/*                           OkayOrNot                               */
/*********************************************************************/
/* Display error message(s) when a Testcase condition isn't true.    */
/* Counts number of 'ok' tests if the test condition is true. Else   */
/* counts and displays information about the failed condition. The   */
/* first passed argument is true/false condition indicating whether  */
/* the test condition holds or not.  The other passed arguments is   */
/* message text that should be displayed if the condition is false.  */
/*********************************************************************/

OkayOrNot:

If \active Then Return      -- (ignore if no '*Compare' seen yet)

/* The first argument passed to us is a boolean indicating whether or
   not the test condition was found to be true or false (ok or not).
*/
If ARG(1) Then
   oks = oks + 1            -- (count okay tests for this Testcase)
Else
Do
   /* Display the test failure message(s) provided by the caller */

   errmsg  =  '>>>>> line' RIGHT( lineno, 5 )':'
   indent  =  LEFT( ' ', LENGTH( errmsg ))

   Say errmsg ARG(2)      -- primary failure message
   Do n = 3 To ARG()
   Say indent ARG( n )    -- 'Want:', 'Got: ', etc...
   End

   rv = rv + 1            -- (count failed tests for this Testcase)

   /* Display any '*Explain' messages if there are any */

   Do n = 1 to explain.0
      Say indent explain.n
   End
End

explain.0 = 0     -- (be quiet until next '*Explain' group)
Return

/*********************************************************************/
/*                           EndTest                                 */
/*********************************************************************/
/* Process '*Done' order: perform end-of-Testcase processing         */
/*********************************************************************/

EndTest:

numdones = numdones + 1         -- (count '*Done' orders)
nowait = (rest = 'nowait')      -- (was 'nowait' option specified?)

/* Report any unexpected program checks */

If pgmchk \= '' | wantpgm \= '' Then
Do
   If pgmchk \= '' & wantpgm = '' Then
      Call OkayOrNot FALSE, 'Unexpected pgm type' pgmchk'.'

   If pgmchk = '' & wantpgm \= '' Then
      Call OkayOrNot FALSE, 'Expected pgm type' wantpgm', but none happened.'

   If pgmchk \= '' & wantpgm \= '' Then
      Call OkayOrNot pgmchk = wantpgm, 'Expected pgm type' wantpgm', but got type' pgmchk'.'
End

/* Report any missing wait state */

If \nowait & \waitseen Then
Do
   /* PROGRAMMING NOTE: due to Hercules's currently flawed message
      handling, sometimes messages appear in the log in the wrong
      order, with the message for a particular event that logically
      occurred BEFORE some other event being written to the logfile
      AFTER the message for an event which logically followed it.

      The below call to the LookAhead subroutine attempts to locate
      the missing disabled wait message by reading the next several
      lines from the logfile. During this process the current file
      position is saved and then restored again afterwards to ensure
      no logile statements are inadvertently skipped.
   */
   Call LookAhead       -- (maybe it follows our '*Done' order?)

   If \waitseen Then
   Do
      Say '>>>>> line' lineno': No wait state encountered.'
      rv = rv + 1
   End
End

/* Issue the overall report for this Testcase */

Select
   When rv = 0 Then msg = 'All pass.'
   When rv = 1 Then msg = 'One failure.'
   Otherwise        msg = rv 'failures.'
End

If rv \= 0 | \quiet
   Then Say 'Test "'testcase'": ' oks 'OK compares. ' msg

/* Increment overall count of Testcase successes or failures. */

failed = (rv \= 0)                -- (whether or not Testcase failed)
fails.failed = fails.failed + 1   -- (bump overall successes/failures)

Return

/*********************************************************************/
/*                         LookAhead                                 */
/*********************************************************************/
/*  The '*Done' order is being processed, but we haven't seen any    */
/*  disabled wait PSW message yet. Keep reading ahead until either   */
/*  it's found or we reach an if/else/endif or testcase directive.   */
/*********************************************************************/

LookAhead:

/* Save stream position */
saved_lineno = lineno
oldpos = STREAM( logfile, 'Command', 'QUERY POSITION READ' )

/* Read and process statements until either:

    1. waitseen is true
    2. An '*If', '*Else' or '*Fi' order is reached
    3. The next '*Testcase' order is reached
    4. EOF is reached.
*/

lookingahead  = TRUE
stoplookahead = FALSE

Do
   stmt = ReadLog()
   Do While stmt \= ''
      Call ProcessStmt stmt
      If waitseen | stoplookahead Then Leave
      stmt = ReadLog()
   End
End

lookingahead  = FALSE
stoplookahead = TRUE

/*
If waitseen Then
Do
   Say '>>>>> DEBUG: LookAhead success'
   Say '      line' saved_lineno": '*Done' order processed"
   Say '      line' lineno': HHC00809I (or HHC00803I) seen'
End
Else
Do
   Say '>>>>> DEBUG: LookAhead failed'
   Say '      line' saved_lineno": '*Done' order processed"
   Say '      line' lineno': LookAhead aborted'
End
*/

/* Restore stream position */
newpos = STREAM( logfile, 'Command', 'SEEK 'oldpos' READ')
lineno = saved_lineno

Return

/*********************************************************************/
/*      Handle LOGIC ERROR.    We should NEVER reach here.           */
/*********************************************************************/

NoValue:

Parse Source . . ourname

ourname = FILESPEC( 'Name', ourname )   -- (just name w/o path)

Say ''
Say "** LOGIC ERROR in '"ourname"': variable '"CONDITION( 'Description' )"' is undefined."
Say '   Line' SIGL': "'SOURCELINE( SIGL )'"'
Say ''

Exit -1

/*********************************************************************/
/*                            ( EOF )                                */
/*********************************************************************/
