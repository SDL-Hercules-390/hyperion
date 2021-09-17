/* Rexx Sample configuration file for Hercules ESA/390 emulator               */
/* tested and working with ooRexx AND Regina                                  */

/* fixed to use <auxiliary> variables                                         */
/* use the NOECHO variable  to customize ECHO/NOECHO                          */

/* used the <signal> clause for errors                                        */

parse version _RXV
parse var _RXV _RXV "_" .

parse source . . _CNF
_CNF = filespec("n",_CNF)
parse var _CNF _CNF "." .

signal on failure name hfailure
signal on error   name herror

_PWD = directory()

numcpu      =   1
cnslport    =   3279

Address HERCULES

HREXX.ERRORHANDLER = "system"
HREXX.PERSISTENTRESPSTEMNAME = "resp"
NOECHO = "-"

-- find the number of cpus
HREXX.RESPSTEMNAME = "retd"
NOECHO"maxcpu"
do  i = 1 to retd.0
    if  pos("HHC02203",retd.i) > 0 then do
        parse var retd.i . ":" maxcpu
        maxcpu = strip(maxcpu)
        leave
    end
end

--  CPU section
NOECHO"cpuserial"    "002623"
NOECHO"cpumodel"     "3090"
NOECHO"model"        "EMULATOR"
NOECHO"plant"        "ZZ"
NOECHO"manufacturer" "HRC"
NOECHO"lparname"     "HERCULES"
NOECHO"cpuverid"     "FD"
NOECHO"mainsize"     64
NOECHO"xpndsize"     0
NOECHO"maxcpu"       maxcpu
NOECHO"numcpu"       maxcpu
NOECHO"numvec"       0
NOECHO"archlvl"      "z/Arch"
NOECHO"archlvl"      "DISABLE 006_ASN_LX_REUSE"

--  misc
NOECHO"panrate"      "SLOW"
NOECHO"timerint"     "1000"

--  integrated Hercules I/O Controller
NOECHO"cnslport"     cnslport

-- message level
NOECHO"msglevel"     "VERBOSE"

--integrated console
NOECHO"attach" "0009" "3215-C"  "/ noprompt"

-- readers
rdrfile = _PWD"/util/zzsacard.bin"
if  \exists(rdrfile) then ,
    rdrfile = "*"

rdrtabl = "000C 001C"
do  i = 1 to words(rdrtabl)
    addr = word(rdrtabl,i)
    NOECHO"attach" addr "3505" rdrfile
end

-- card punches
pchtabl = "000D 001D"
do  i = 1 to words(rdrtabl)
    addr = word(pchtabl,i)
    NOECHO"attach" addr "3525" "pch"addr".txt"
end

-- printers
prttabl = "0002 0003 000e 000f"
prttype = "3211 3211 1403 1403"
do  i = 1 to words(prttabl)
    addr = word(prttabl,i)
    type = word(prttype,i)
    NOECHO"attach" addr type "prt"addr".txt"
end

-- 3270 devices
NOECHO"attach" "700.8" "3270"

-- 3270 devices ( to show how to loop thru hex addresses )
do  addr = x2d(900) to x2d(907)
    NOECHO"attach" d2x(addr) "3270"
end

-- attach a duplicate device to show the error handling
NOECHO"attach" "900" "3270"

exit

-- on error/failure handlers

herror:
    say "*********" _CNF "signal on error trapped at " sigl
    say "*********" _CNF "Ended"
    exit

hfailure:
    say "*********" _CNF "signal on failure trapped at " sigl
    say "*********" _CNF "Ended"
    exit

-- ooRexx/Regina compatibility functions

exists:
	if _RXV = "REXX-ooRexx" then do
		if SysIsFile(arg(1)) then return 1
		if SysIsFileDirectory(arg(1)) then return 1
		return 0
	end

	if _RXV = "REXX-Regina" then do
		return stream(arg(1), "c", "query exists") \= ""
	end

	return 0

isFile:
	if _RXV = "REXX-ooRexx" then do
		return SysIsFile(arg(1))
	end

	if _RXV = "REXX-Regina" then do
		_?fstat = stream(arg(1), "c", "fstat")
		return wordpos("RegularFile",_?fstat) > 0
	end

	return 0

isPath:
	if _RXV = "REXX-ooRexx" then do
		return SysIsFileDirectory(arg(1))
	end

	if _RXV = "REXX-Regina" then do
		_?fstat = stream(arg(1), "c", "fstat")
		return wordpos("Directory",_?fstat) > 0
	end

	return 0
