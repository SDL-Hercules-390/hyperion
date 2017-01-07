/* Process a test case                                               */
/*                                John Hartmann 22 Sep 2015 16:27:56 */

/*********************************************************************/
/* This  file  was  put into the public domain 2015-10-05 by John P. */
/* Hartmann.   You can use it for anything you like, as long as this */
/* notice remains.                                                   */
/*                                                                   */
/* Process the log file from a Hercules test run.                    */
/*                                                                   */
/* One challenge is that messages are issued from different threads. */
/* Thus the *Done message may overtake, e.g., message 809.           */
/*********************************************************************/

Signal On novalue

Parse Arg In opts

values.                = ''
values.keepalive       = 'Full'
values.threading_model = ''
values.locking_model   = ''
values.libraries       = ''
values.hatomics        = ''
values.cmpxchg1        = 0
values.cmpxchg4        = 0
values.cmpxchg8        = 0
values.cmpxchg16       = 0
values.fetch_dw        = 0
values.store_dw        = 0
values.max_cpu_engines = 0
values.can_s370_mode   = 0
values.can_esa390_mode = 0
values.can_zarch_mode  = 0
values.syncio          = 0
values.shared_devices  = 0
values.HDL             = 0
values.externalgui     = 0
values.SIGABEND        = 0
values.NLS             = 0
values.HAO             = 0
values.regex           = 0
values.IPV6            = 0
values.HTTP            = 0
values.IEEE            = 0
values.sqrtl           = 0
values.CCKD_BZIP2      = 0
values.HET_BZIP2       = 0
values.ZLIB            = 0
values.OORexx          = 0
values.Regina          = 0
values.CONFIG_INCLUDE  = 0
values.SYSTEM_SYMBOLS  = 0
values.CONFIG_SYMBOLS  = 0

quiet = 0                             /* Log OK tests too            */

Do while opts \= ''

   Parse Var opts optval opts
   Parse Var optval varname '=' +0 eq +1 varvalue

   Select

      When optval = 'quiet' Then
         quiet = 1

      When eq \= '=' Then
         Say 'Not variable assignment in argument string: ' optval

      Otherwise
         Call setvar varname varvalue
   End

End

testcase  = '<unknown>'
ifstack   = ''

comparing = 0
hadelse   = 0
fails.    = 0                         /* .0 is OK .1 is failures     */
catast    = 0                         /* Catastrophic failures       */
done      = 0                         /* Test cases started          */
stardone  = 0                         /* *Done orders met            */
lineno    = 0                         /* Line number in input file   */
ifnest    = 0                         /* If/then/else nesting        */
active    = 1                         /* We do produce output        */

/*********************************************************************/
/* Process the test output.                                          */
/*********************************************************************/

Do While lines(in) > 0
   l = readline()
   If l \= ''
      Then Call procline l
End

Select
   When fails.1 = 0 & catast = 0
      Then msg = 'All OK.'
   Otherwise
      msg = fails.1 'failed;' fails.0 'OK.'
End

Say 'Done' done 'tests. ' msg

If catast > 0
   Then Say '>>>>>' catast 'test failed catastrophically.  Bug in Hercules or test case is likely. <<<<<'

If done \= stardone Then Say 'Tests malformed. ' done '*Testcase orders met, but' stardone '*Done orders met.'

Exit  fails.1

/*********************************************************************/
/* Read a line of input and strip timestamp, if any.                 */
/*********************************************************************/

readline:

Do forever

   l = ''

   If 0 = lines(in)
      Then Return ''                  /* EOF                         */

   lineno = lineno + 1
   l      = linein(in)

   /*  Another way to handle a prefix to the response
       might be to look for HHC.
   */

   Parse Value word(l, 1) With fn '(' line ')' +0 rpar

   If rpar = ')' Then
      Do
         Parse Var l . l              /* Drop debug ID               */
         l = strip(l)
      End

   Parse Var l ?msg ?verb ?rest
   p = verify(?msg, "0123456789:")    /* Leading timestamp?          */

   If p = 0                           /* Just a timestamp or nothing */
      Then Iterate
   If p > 1                           /* Timestamp prefix            */
      Then l = substr(l, p)           /* Toss it                     */
   If left(l, 3) \='HHC'
      Then Iterate                    /* Not a message               */
   If ?msg = 'HHC00007I'              /* Where issued                */
      Then Iterate
   If ?msg \= 'HHC01603I'
      Then Leave                      /* Not a comment               */
   If length(?verb) > 1 & left(?verb, 1) = '*'
      Then Leave                      /* Potential order.            */
End
Return l

/*********************************************************************/
/* Process the line                                                  */
/*********************************************************************/

procline:

Parse Arg l
Parse Var l msg verb rest

Select
   When left(msg, 3) \= 'HHC' Then
      nop                             /* Toss all non-messages.      */
   When msg = 'HHC01417I' Then
      Select
         When verb = 'Machine' Then
            Do
               Parse Var rest 'dependent assists:' rest
               Do while rest \= ''
                  Parse Var rest fac rest
                  If left(fac, 9) = 'hatomics=' Then
                     Do
                        fac = substr(fac, 10)
                        If fac \= 'UNKNOWN' Then
                           Call setvar 'hatomics' fac
                     End
                  Else
                     Call setvar fac 1
               End
            End
         When verb = 'Modes:' Then
            Do while rest \= ''
               Parse Var rest arch rest
               Select
                  When arch = 'S/370'   Then Call setvar 'can_s370_mode'   1
                  When arch = 'ESA/390' Then Call setvar 'can_esa390_mode' 1
                  When arch = 'z/Arch'  Then Call setvar 'can_zarch_mode'  1
                  Otherwise nop
               End
            End
         When verb = 'Max' Then
            Do
               Parse Var rest 'CPU Engines: ' engines
               Call setvar 'max_cpu_engines' engines
            End
         When verb = 'Using' Then
            Select
               When rest = 'Fish threads Threading Model'       Then Call setvar 'threading_model' 'Fish'
               When rest = 'POSIX threads Threading Model'      Then Call setvar 'threading_model' 'POSIX'

               When rest = 'Error-Checking Mutex Locking Model' Then Call setvar 'locking_model'   'Error'
               When rest = 'Normal Mutex Locking Model'         Then Call setvar 'locking_model'   'Normal'
               When rest = 'Recursive Mutex Locking Model'      Then Call setvar 'locking_model'   'Recursive'

               When rest = 'shared libraries'                   Then Call setvar 'libraries'       'shared'
               When rest = 'static libraries'                   Then Call setvar 'libraries'       'static'
               Otherwise nop
            End
         When  verb = 'With' | verb = 'Without' Then
            Do
               If verb = 'With' Then
                  with = 1
               Else    
                  with = 0
               Select
                  When rest = 'Partial TCP keepalive support' Then Call setvar 'keepalive'  'Partial'
                  When rest = 'Basic TCP keepalive support'   Then Call setvar 'keepalive'  'Basic'
                  When rest = 'TCP keepalive support'         Then Call setvar 'keepalive'  ''

                  When rest = 'Syncio support'                Then Call setvar 'syncio'         with
                  When rest = 'Shared Devices support'        Then Call setvar 'shared_devices' with
                  When rest = 'Dynamic loading support'       Then Call setvar 'HDL'            with
                  When rest = 'External GUI support'          Then Call setvar 'externalgui'    with
                  When rest = 'SIGABEND handler'              Then Call setvar 'SIGABEND'       with
                  When rest = 'National Language Support'     Then Call setvar 'NLS'            with
                  When rest = 'Automatic Operator support'    Then Call setvar 'HAO'            with
                  When rest = 'Regular Expressions support'   Then Call setvar 'regex'          with
                  When rest = 'IPV6 support'                  Then Call setvar 'IPV6'           with
                  When rest = 'HTTP Server support'           Then Call setvar 'HTTP'           with
                  When rest = 'IEEE support'                  Then Call setvar 'IEEE'           with
                  When rest = 'sqrtl support'                 Then Call setvar 'sqrtl'          with
                  When rest = 'CCKD BZIP2 support'            Then Call setvar 'CCKD_BZIP2'     with
                  When rest = 'HET BZIP2 support'             Then Call setvar 'HET_BZIP2'      with
                  When rest = 'ZLIB support'                  Then Call setvar 'ZLIB'           with
                  When rest = 'Object REXX support'           Then Call setvar 'OORexx'         with
                  When rest = 'Regina REXX support'           Then Call setvar 'Regina'         with
                  When rest = 'CONFIG_INCLUDE support'        Then Call setvar 'CONFIG_INCLUDE' with
                  When rest = 'SYSTEM_SYMBOLS support'        Then Call setvar 'SYSTEM_SYMBOLS' with
                  When rest = 'CONFIG_SYMBOLS support'        Then Call setvar 'CONFIG_SYMBOLS' with
                  Otherwise nop
               End
            End
         Otherwise
            nop
      End
   When msg = 'HHC00801I' Then
      Parse Var rest ' code ' havepgm . ' ilc'
   When msg = 'HHC00803I' Then     /* Program interrupt loop      */
      havewait = 1
   When msg = 'HHC00809I' Then
      Call waitstate
   When msg = 'HHC01405E' Then
      Call test 0, l
   When msg = 'HHC01603I' Then
      If left(verb, 1) = '*'
         Then Call order
   When msg = 'HHC02269I' Then
      Call gprs
   When msg = 'HHC02277I' Then
      Parse Var rest . prefix .
   When (msg = 'HHC02290I' | msg = 'HHC02291I') Then
      If comparing Then
         Do
            Parse Var rest 'K:' key .
            If key = ''
               Then
                  Do
                     Parse Var rest display +36 /* Save for compare order */
                     lastaddr = verb
                  end
               Else
                  Do
                     keyaddr = substr(verb, 3)
                     lastkey = key
                  End
         End
   When left(msg, 8) = 'HHC02332' Then
      Do
         If \timeoutOk
            Then catast = catast + 1
         Call test timeoutOk, 'Test case timed out in wait.',,
            'This is likely an error in Hercules.  Please report'
      End
   When \comparing /* Toss other messages, particularly during start */
      Then nop
   Otherwise                     /* Something else.  Store for *Hmsg */
      ? = lastmsg.0 + 1
      lastmsg.? = l
      lastmsg.0 = ?
End
Return

/*********************************************************************/
/* Set variable value                                                */
/*********************************************************************/

setvar:
Parse Arg varname varvalue
values.varname = varvalue
If \quiet Then
   Say 'Variable' varname 'set to "'values.varname'".'
Return

/*********************************************************************/
/* Decode orders.                                                    */
/*********************************************************************/

order:

info = ''
rest = strip(rest)

If left(rest, 1) = '"'
   Then Parse Var rest '"' info '"' rest

Select

   When verb = '*'                    /* Comment                     */
      Then nop
   When verb = '*Testcase'
      Then call begtest
   When verb = '*If'
      Then call doif
   When verb = '*Else'
      Then call doElse
   When verb = '*Fi'
      Then call endif
   when \active
      Then nop
   When verb = '*Compare' Then
      Do
         comparing = 1
         lastmsg.0 = 0
      End
   When verb = '*Want'
      Then call want
   When verb = '*Error'
      Then call msg 'Error'
   When verb = '*Info'
      Then call msg 'Informational'
   When verb = '*Hmsg'
      Then call msg 'Panel message'
   When verb = '*Explain'
      Then call explain
   When verb = '*Gpr'
      Then call wantgpr
   When verb = '*Key'
      Then call wantkey
   When verb = '*Message' Then
      If active
         Then Say rest
   When verb = '*Prefix'
      Then call wantprefix
   When verb = '*Program'
      Then wantpgm = rest
   When verb = '*Done'
      Then call endtest
   When verb = '*Timeout'
      then timeoutOk = 1
   Otherwise
      Say 'Bad directive: ' verb
      rv = 16
End
Return

/*********************************************************************/
/* Initialise variable store for new test case                       */
/*********************************************************************/

begtest:

done        = done + 1
testcase    = rest
comparing   = 0
havewait    = 0
oks         = 0
wantpgm     = ''
havepgm     = ''
rv          = 0
pgmok       = 0                       /* Program check not expected  */
gpr.        = ''
lastkey     = ''
keyaddr     = '<unknown>'
lastaddr    = '<unknown>'
prefix      = ''
lastmsg.0   = 0
lasterror.0 = 0
lasterror   = ''
lastinfo.0  = 0
lastinfo    = ''
expl.0      = 0
timeoutOk   = 0
lastmsg.0   = 0

Return

/*********************************************************************/
/* Wind up a test.                                                   */
/*********************************************************************/

endtest:

stardone    = stardone + 1
unprocessed = ''                      /* No read ahead stored        */
nowait = (rest = 'nowait')

If \havewait & \nowait
   Then Call lookahead             /* Perhaps *Done overtook message */

Select
   When havepgm \= ''
      Then call figurePgm
   Otherwise nop
End

If \havewait & \nowait Then
   Do
      Say '>>>>> line' lineno': No wait state encountered.'
      rv = rv + 1
   End
Select
   When rv = 0
      Then msg ='All pass.'
   When rv = 1
      Then msg ='One failure.'
   Otherwise
      msg = rv 'failures.'
End

If \quiet | rv \= 0
   Then Say 'Test' testcase'. ' oks 'OK compares. ' msg

fail       = rv \= 0
fails.fail = fails.fail + 1           /* Ok or fail                  */

If unprocessed \= ''
   Then Call procline unprocessed
Return

/*********************************************************************/
/* Test  case has ended, but we have not seen a disabled PSW message */
/* yet.                                                              */
/*                                                                   */
/* If havepgm is nonblank, we have seen a program check, but not yet */
/* the disabled wait PSW that will ensue.                            */
/*********************************************************************/

lookahead:

Do While lines(in) > 0

   unprocessed = readline()

   If wordpos(?msg, 'HHC00809I HHC00803I') > 0
      Then leave
   If havepgm = ''                    /* Saw a program check?        */
      Then Return                  /* Return to process *Done if not */

   Call procline unprocessed

End

/* Say '*** wait leapfrogged. ***' lineno */
Call procline unprocessed
unprocessed = ''

Return

/*********************************************************************/
/* Compare storage display against wanted contents.                  */
/*********************************************************************/

want:

wantres = (rest = display)

Call test wantres, 'Storage at' lastaddr 'compare mismatch. ' info

If \wantres Then
   Do
      Say '... Want:' strip(rest)
      Say '... Have:' strip(display)
   End
Return

/*********************************************************************/
/* Verify contents of a general register.                            */
/*********************************************************************/

wantgpr:

Parse Upper Var rest r want '#' .

Call test gpr.r = want, 'Gpr' r 'compare mismatch. ' info 'Want:' want 'got' gpr.r

Return

/*********************************************************************/
/* Verify  that  the  key  of  the  last page frame displayed is the */
/* specified one.                                                    */
/*********************************************************************/

wantkey:

Parse Upper Var rest want '#' .

If lastkey = '' Then
   Call test 0, 'No key saved from r command.'
Else
   Call test lastkey = want, 'Key' keyaddr 'compare mismatch. ' info 'Want:' want 'got' lastkey

Return

/*********************************************************************/
/* Verify that the prefix register contains the specified value.     */
/*********************************************************************/

wantprefix:

Parse Upper Var rest want '#' .

If prefix = '' Then
   Call test 0, 'No prefix register saved from pr command.'
Else
   Call test prefix = want, 'Prefix register compare mismatch. ' info 'Want:' want 'got' prefix

Return

/*********************************************************************/
/* Verify the last issued message.                                   */
/*********************************************************************/

msg:

Parse Arg msgtype

If lastmsg.0 = 0 Then
   Do
      Call test 0, 'No message saved for' rest
      Return
   End

If datatype(word(rest, 1), 'Whole') Then
   Parse var rest mnum rest
Else
   mnum = 0

ix = lastmsg.0 - mnum

If ix <= 0 Then
   Call test 0, 'No message stored for number' mnum
Else
   Call test lastmsg.ix = rest, msgtype 'message mismatch. ' info ,,
      'Want:' rest, 'Got: ' lastmsg.ix

Return

/*********************************************************************/
/* Verify the last issue informational message                       */
/*********************************************************************/

imsg:

If lastinfo = '' Then
   Call test 0, 'No informational message saved for' rest
Else
   Call test lastinfo = rest, 'Informational message mismatch. ' info ,,
      'Want:' rest, 'Got: ' lastinfo

Return

/*********************************************************************/
/* Decode disabled wait state message.                               */
/*********************************************************************/

waitstate:

havewait = 1
Parse Var rest 'wait state' psw1 psw2
cond = (psw2 = 0) | (right(psw2, 4) = 'DEAD' & havepgm \= '' & havepgm = wantpgm)
Call test cond, 'Received unexpected wait state: ' psw1 psw2

Return

/*********************************************************************/
/* Save general registers for later test.                            */
/*********************************************************************/

gprs:

If verb = 'General'
   Then return

todo = verb rest

Do While todo \= ''
   Parse Var todo 'R' r '=' val todo
   r = x2d(r)
   gpr.r = val
End

Return

/*********************************************************************/
/* Save explain text in explain array.                               */
/*********************************************************************/

explain:

? = expl.0 + 1
expl.? = rest
expl.0 = ?

Return

/*********************************************************************/
/* Process  a  program check, which is identified by the IA field of */
/* the disabled wait PSW.                                            */
/*********************************************************************/

figurePgm:

Select
   When havepgm \= '' & wantpgm \= ''
      Then Call test havepgm = wantpgm, 'Expect pgm type' wantpgm', but have type' havepgm'.'
   When havepgm \= ''
      Then Call test 0, 'Unexpected pgm type' havepgm' havepgm.'
   Otherwise
      Call test 0, 'Expect pgm type' wantpgm', but none happened.'
End

Return

/*********************************************************************/
/* Evaluate  the  condition  on a If to determine whether to perform */
/* orders or not.                                                    */
/*********************************************************************/

doif:

expr = rest
test = ''

Do While rest \= ''
   Parse Var rest rl '$' +0 dolvar rest
   test = test rl

   If dolvar = ''
      Then leave

   Parse Var dolvar +1 varname .
   varvalue = values.varname

   If \datatype(varvalue, "Whole")
      Then varvalue = '"'varvalue'"'

   test = test varvalue
End

Signal On syntax
Signal Off novalue

interpret 'res = ' test

Signal On  novalue

ifstack = active hadelse ifstack
ifnest = ifnest + 1

If active                             /* In nested suppress?         */
   Then active = (res = 1)

Return

syntax:
Say 'Syntax error in *If test:' expr
Return

/*********************************************************************/
/* Process Else.  Flip active if no else has been seen.              */
/*********************************************************************/

doElse:

Select
   When ifnest <= 0
      Then Say 'No *If is active.'
   When hadelse
      Then Say '*Else already seen on level' ifnnest '.  Stack:' ifstack
   Otherwise
      Parse Var ifstack prevact .   /* Nested if in suppressed code? */
      hadelse = 1
      active = prevact & (1 && active)
End

Return

/*********************************************************************/
/* End of if.  Unstack.                                              */
/*********************************************************************/

endif:

If ifnest <= 0 Then
   Do
      Say 'No *If is active.'
      Return
   End

ifnest = ifnest - 1
Parse Var ifstack active hadelse ifstack

Return

/*********************************************************************/
/* Display message about a failed test.                              */
/*********************************************************************/

test:

If \active                            /* suppress any noise          */
   Then Return

! = '>>>>> line' right(lineno, 5)':'
!! = left(' ', length(!))

If arg(1) Then
   oks = oks + 1
Else
   Do
      Say ! arg(2)

      Do ? = 3 to arg()
         Say !! arg(?)
      End

      rv = rv + 1

      Do ? = 1 to expl.0              /* Further explanation         */
         Say !! expl.?
      End
   End

expl.0 = 0                            /* Be quiet next time          */

Return

novalue:

Parse Source . . fn ft fm .

Say 'Novalue in' fn ft fm '-- variable' condition('D')
Say right(sigl, 6) '>>>' sourceline(sigl)
Say '  This is often caused by missing or misspelled *Testcase'
Say '  Note that the verbs are case sensitive.  E.g., *testcase is not correct.'

Signal Value lets_get_a_traceback
