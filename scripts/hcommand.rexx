#! /bin/cat

Trace "O"

/* Initialization */

parse version _ver
parse source _src
_env = Address()

parse var _src . _mode .
parse var _src . . _cmd
_nx0 = filespec("n",_cmd)
parse var _nx0 _n0 "." .

--signal on failure name hfailure
--signal on error   name herror

/* Techniques... */

technique.desc.1  = 'HREXX.RESPSTEMNAME           = "foo"; Address HERCULES command'
technique.desc.2  = 'HREXX.PERSISTENTRESPSTEMNAME = "foo"; Address HERCULES command'
technique.desc.3  = '_rc = awscmd( command, "foo", "RETCODE" )'
technique.desc.4  = '_rc = awscmd( command, "foo", "SYSTEM" )'
technique.desc.5  = '_rc = awscmd( command, "foo" )'
technique.desc.6  = '_rc = awscmd( command )'
technique.desc.7  = 'call awscmd command, "foo", "RETCODE"'
technique.desc.8  = 'call awscmd command, "foo", "SYSTEM"'
technique.desc.9  = 'call awscmd command, "foo"'
technique.desc.10 = 'call awscmd command'

techniques  =  10

/* Parse and validate arguments */

if  _mode = "COMMAND" then do
    parse arg argv
    technique = word(argv,1)
    command = strip(substr(argv,wordindex(argv,2)))
    drop argv
end
else do -- "SUBROUTINE"
    if arg() >= 1 then technique = arg(1)
    command = ""
    if arg() >= 2 then do i = 2 for arg() - 1
        argx = arg(i)
        if pos(" ",argx) > 0 then argx = '"' || argx || '"'
        command ||= " " || argx
    end
    command = strip(command)
end

if  technique = "" | \datatype(technique,"N")  | technique < 1 | technique > techniques | command = "" then do

    if  technique = "" | \datatype(technique,"N")  | technique < 1 | technique > techniques then ,
        say _n0 "ERROR: Invalid technique"

    if  command = "" then ,
        say _n0 "ERROR: Missing command"

    /* Display HELP information */

    say _n0 ""
    say _n0 'Format:   "' || _nx0 || '  <technique>  <command>"'
    say _n0 'Example:  "' || _nx0 || '       5        defsym"'
    say _n0 ""
    say _n0 "    technique   Which technique to use:"
    say _n0 ""
    width = length(techniques)
    do i = 1 for techniques
    say _n0 '                'right("  "||i,width)'  =  ' || technique.desc.i
    end
    say _n0 ""
    say _n0 "    command     The Hercules command to be issued."
    say _n0 ""

    exit
end

/*
  Prevent our command from appearing in the command recall
  history list by prefixing it with a '-' to make it silent
  and add a command-line comment so we can test that too
*/
command = strip(command)
if left(command,1) \= "-" then command = "-" || command
command ||= "   # (command issued by " || _nx0 || ")"

/* Show our environment */

say _n0 "Started"
say _n0 "Version  " _ver
say _n0 "Source   " _src
say _n0 "HOSTENV  " _env
say _n0 "Date     " date()
say _n0 "Time     " time()
say _n0 "Mode     " _mode

/* Issue Hercules command using the requested technique */

say _n0 "Technique = " || technique || ": " || technique.desc.technique
say _n0 "Command   = " || command

labelname = upper("technique_" || technique)
signal value labelname

technique_1:

    HREXX.RESPSTEMNAME = "foo"; Address HERCULES command
    say _n0 "RC = "RC
    signal show_results

technique_2:

    HREXX.PERSISTENTRESPSTEMNAME = "foo"; Address HERCULES command
    say _n0 "RC = "RC
    signal show_results

technique_3:

    _rc = awscmd( command, "foo", "RETCODE" )
    say _n0 "_rc = "_rc
    signal show_results

technique_4:

    _rc = awscmd( command, "foo", "SYSTEM" )
    say _n0 "_rc = "_rc
    signal show_results

technique_5:

    _rc = awscmd( command, "foo" )
    say _n0 "_rc = "_rc
    signal show_results

technique_6:

    _rc = awscmd( command )
    say _n0 "_rc = "_rc
    signal show_results

technique_7:

    call awscmd command, "foo", "RETCODE"
    signal show_results

technique_8:

    call awscmd command, "foo", "SYSTEM"
    signal show_results

technique_9:

    call awscmd command, "foo"
    signal show_results

technique_10:

    call awscmd command
    signal show_results

/* Show captured response if applicable */

show_results:

if datatype(foo.0,"NUM") then do
    width = length(foo.0)
    do  i = 0 for (foo.0 + 1)
        say _n0 || " foo." || right("   "||i,width) || " = " || foo.i
    end
end

say _n0 "Ended"
exit

herror:
    say _n0 "signal on error trapped"
    say _n0 "Ended"
    exit

hfailure:
    say _n0 "signal on failure trapped"
    say _n0 "Ended"
    exit
