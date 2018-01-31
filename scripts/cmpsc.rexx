/* cmpsc.rexx -- test CMPSC instruction */

--  help:start

--
--  NAME
--
--      <scriptname>  -  Hercules CMPSC instruction test script.
--
--  VERSION
--
--      1.3  (July 2012)
--
--  DESCRIPTION
--
--      Performs the Hercules "cmpsc.txt" instruction test using
--      the specified parameters.
--
--  SYNOPSIS
--
--      <scriptname> inbuff outbuff infile indict [workdir] [-z] [-d]
--
--  ARGUMENTS
--
--      inbuff     Input buffer size and offset value specified as
--                 two decimal values separated by a single colon.
--
--      outbuff    Output buffer size and offset in the same format
--                 as the in_buffer_size argument (e.g. "8160:15").
--                 Buffer sizes must be greater than 260 bytes, and
--                 page offset values must be less than 4096 bytes.
--
--      infile     The name of the file used as input to the test.
--
--      indict     The name of the dictionary used as input to the
--                 test. The file's extention is used to determine
--                 the dictionary format, the compressed data symbol
--                 size and whether this will be a compression test
--                 or an expansion test. If the extension is '.01E'
--                 the test will be an expansion test using a 9-bit
--                 (cdss=1) format-0 expansion dictionary, whereas
--                 '.15C' performs a compression test using 13-bit
--                 format-1 compression and expansion dictionaries.
--
--      workdir    An optional directory name where temporary files
--                 created during processing should be written. If
--                 not specified the current directory is used. The
--                 work files created during processing are called
--                 "cmpout.bin" and "expout.txt" respectively.
--
--      -z         Request CMPSC Enhancement Facility zero padding.
--                 If not specified then zero padding (GR0 bit 46)
--                 will not be requested.
--
--      -d         Debug option to force echoing of all internally
--                 issued Hercules commands to the console as well
--                 as whatever message(s) those commands generate.
--                 The normal behavior is for <scriptname> to issue
--                 all internal Hercules commands silently. The -d
--                 option overrides this silent behavior.
--
--  NOTES
--
--      <scriptname> simply performs a series of defsyms to define the
--      test parameters for the Hercules "cmpsc.txt" instruction test
--      program based on the arguments you specified for <scriptname>.
--      It is the Hercules "cmpsc.txt" instruction test program which
--      performs the actual test of the CMPSC instruction. <scriptname>
--      does the needed defsyms, starts the test, and then waits for
--      it to complete by querying the psw at periodic intervals.
--

--  help:end

Trace Off

parse source src
parse var src . mode .                      -- (command or subroutine)
parse var src . . cmdpath
scriptname = filespec("name",cmdpath)       -- (name with extension)
parse var scriptname who "." .              -- (name without extension))

--signal on failure name hfailure           -- (debugging)
--signal on error name herror               -- (debugging)

/* Initialization */

cmp_dict_addr   = x2d("00010000")  -- 64K line
in_buffer_addr  = x2d("00100000")  -- 1MB line
out_buffer_addr = x2d("00110000")  -- 1MB + 64K
out_file_addr   = x2d("00200000")  -- 2MB line
in_file_addr    = x2d("00900000")  -- 9MB line

cmpout = "cmpout.bin"
expout = "expout.txt"

curdir = directory()
if pos("/",curdir) > 0 then pathsep = "/"
if pos("\",curdir) > 0 then pathsep = "\"

/* Gather arguments */

if  mode = "COMMAND" then do
    parse arg args
    args = space(args)
    arg.0 = words(args)
    do  i = 1 for arg.0
        arg.i = word(args,i)
    end
end
else do -- "SUBROUTINE"
    arg.0 = arg()
    do  i = 1 for arg.0
        arg.i = arg(i)
    end
end

if  arg.0 <= 0 then do
    call help
    exit 1
end

/* Parse arguments */

in_file       = ""
indict        = ""
exp_dict_name = ""
workdir       = ""

in_buffer_size    = 0
in_buffer_offset  = 0
out_buffer_size   = 0
out_buffer_offset = 0

have_inbuff   = 0
have_outbuff  = 0
have_infile   = 0
have_indict   = 0
have_workdir  = 0

zp_opt = 0

debug = 0

do  i = 1 for arg.0
    argv = arg.i

    if  argv = "/?" | ,
        argv = "-?" | ,
        argv = "-h" | ,
        argv = "--help" then do
        signal help
    end

    if \have_inbuff then do
        have_inbuff = 1
        in_buffer_size = argv
        iterate
    end

    if \have_outbuff then do
        have_outbuff = 1
        out_buffer_size = argv
        iterate
    end

    if \have_infile then do
        have_infile = 1
        in_file = argv
        iterate
    end

    if \have_indict then do
        have_indict = 1
        indict = argv
        iterate
    end

    if \have_workdir then do
        have_workdir = 1
        workdir = argv
        iterate
    end

    if  argv = "/z" | ,
        argv = "-z" then do
        zp_opt = 1
        iterate
    end

    if  argv = "/d" | ,
        argv = "-d" then do
        debug = 1
        iterate
    end

    call logmsg '** ERROR ** arg 'i': extraneous argument: "'argv'"'
    exit 1
end

/* Make sure we have all needed arguments */

if \have_inbuff then do
    call logmsg "** ERROR ** Required argument 'inbuff' not specified."
    exit 1
end

if \have_outbuff then do
    call logmsg "** ERROR ** Required argument 'outbuff' not specified."
    exit 1
end

if \have_infile then do
    call logmsg "** ERROR ** Required argument 'infile' not specified."
    exit 1
end

if \have_indict then do
    call logmsg "** ERROR ** Required argument 'indict' not specified."
    exit 1
end

if \have_workdir then do
    workdir = curdir
end

/* Validate arguments */

parse var in_buffer_size  ib ":" in_buffer_offset
parse var out_buffer_size ob ":" out_buffer_offset

if  in_buffer_offset  = "" then in_buffer_offset  = 0
if  out_buffer_offset = "" then out_buffer_offset = 0

if  \isnum(ib) | \isnum(in_buffer_offset) then do
    call logmsg '** ERROR ** non-numeric input buffer size or offset  "'in_buffer_size'"'
    exit 1
end

if  \isnum(ob) | \isnum(out_buffer_offset) then do
    call logmsg '** ERROR ** non-numeric output buffer size or offset "'out_buffer_size'"'
    exit 1
end

minbufsize = 260
maxoffset = 4095

if  ib < minbufsize then do
    call logmsg '** ERROR ** input buffer size must be >= 'minbufsize' "'in_buffer_size'"'
    exit 1
end

if  ob < minbufsize then do
    call logmsg '** ERROR ** output buffer size must be >= 'minbufsize' "'out_buffer_size'"'
    exit 1
end

if  in_buffer_offset > maxoffset then do
    call logmsg '** ERROR ** input offset must be <= 'maxoffset' "'in_buffer_size'"'
    exit 1
end
if  out_buffer_offset > maxoffset then do
    call logmsg '** ERROR ** output offset must be <= 'maxoffset' "'out_buffer_size'"'
    exit 1
end

in_buffer_size  = ib
out_buffer_size = ob

fp = fullpath(in_file)
if  fp = "" then do
    call logmsg '** ERROR ** infile "'in_file'" not found.'
    exit 1
end
in_file = fp

fp = fullpath(indict)
if  fp = "" then do
    call logmsg '** ERROR ** indict "'indict'" not found.'
    exit 1
end
indict = fp

if  right(indict,1) = "C" then do
    test_type = "Compression"
    compress = 1
    expand   = 0
    cmp_dict_name = indict
    exp_dict_name = left(cmp_dict_name,length(cmp_dict_name)-1) || "E"
end
else do
    test_type = "Expansion"
    compress = 0
    expand   = 1
    /*
        PROGRAMMING NOTE: for expansion we purposely set our compression
        dictionary name to be the same as our expansion dictionary name
        in order to force the expansion dictionary to get loaded where
        the compression dictionary normally would be. We do this because
        our test program is designed to point GR1 to what it thinks is a
        compression dictionary, but because this is an expansion and not
        a compression, causes it to end up pointing to our expansion
        dictionary instead, which is exactly what we want.
    */
    exp_dict_name = indict
    cmp_dict_name = exp_dict_name
end

fp = fullpath(cmp_dict_name)
if  fp = "" then do
    call logmsg '** ERROR ** cmp_dict_name "'cmp_dict_name'" not found.'
    exit 1
end
cmp_dict_name = fp

fp = fullpath(exp_dict_name)
if  fp = "" then do
    call logmsg '** ERROR ** exp_dict_name "'exp_dict_name'" not found.'
    exit 1
end
exp_dict_name = fp

fp = fullpath(workdir)
if  fp = "" then do
    call logmsg '** ERROR ** workdir "'workdir'" not found.'
    exit 1
end
workdir = fp

/* Set needed values */

call panel_command "msglevel -debug"
call panel_command "cmpscpad"
parse var msgs.1 . ":" zp_bits .

if \isnum(zp_bits) | ,
    zp_bits < 1    | ,
    zp_bits > 12   then ,
    zp_bits = 8

zp_bytes = 2 ** zp_bits
zp_mask = -1 - zp_bytes + 1

if compress then out_file = workdir || cmpout
if expand   then out_file = workdir || expout

in_buffer_addr  += in_buffer_offset
out_buffer_addr += out_buffer_offset

cdss = left(right(cmp_dict_name,2),1)
fmt  = left(right(cmp_dict_name,3),1)

dictsize = 2048 * (2 ** cdss)
exp_dict_addr = cmp_dict_addr + dictsize

dicts_size = dictsize
if fmt = "1" then ,
    dicts_size += dictsize

GR0 =  zp_opt * (2 ** 17)
GR0 += cdss   * (2 ** 12)
GR0 += fmt    * (2 **  9)
GR0 += expand * (2 **  8)

call SysFileTree in_file, "info", "F"
parse var info.1 . . in_file_size . .

msgs. = ""

call do_test(test_type)

exit 0

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

do_test:

    test_type = arg(1)

    /* The following 'defsym' values are used by */
    /* the "cmpsc.txt" instruction test program  */

    call panel_command "defsym in_file            " || defsym_filename(in_file)
    call panel_command "defsym out_file           " || defsym_filename(out_file)
    call panel_command "defsym cmp_dict_name      " || defsym_filename(cmp_dict_name)
    call panel_command "defsym exp_dict_name      " || defsym_filename(exp_dict_name)
    call panel_command "defsym GR0                " || defsym_value(right("00000000"||d2x(GR0),8))
    call panel_command "defsym dicts_size         " || defsym_value(right("00000000"||d2x(dicts_size),8))
    call panel_command "defsym in_file_size       " || defsym_value(right("00000000"||d2x(in_file_size),8))
    call panel_command "defsym cmp_dict_addr      " || defsym_value(right("00000000"||d2x(cmp_dict_addr),8))
    call panel_command "defsym exp_dict_addr      " || defsym_value(right("00000000"||d2x(exp_dict_addr),8))
    call panel_command "defsym in_buffer_offset   " || defsym_value(right("00000000"||d2x(in_buffer_offset),8))
    call panel_command "defsym in_buffer_addr     " || defsym_value(right("00000000"||d2x(in_buffer_addr),8))
    call panel_command "defsym in_buffer_size     " || defsym_value(right("00000000"||d2x(in_buffer_size),8))
    call panel_command "defsym out_buffer_offset  " || defsym_value(right("00000000"||d2x(out_buffer_offset),8))
    call panel_command "defsym out_buffer_addr    " || defsym_value(right("00000000"||d2x(out_buffer_addr),8))
    call panel_command "defsym out_buffer_size    " || defsym_value(right("00000000"||d2x(out_buffer_size),8))
    call panel_command "defsym out_file_addr      " || defsym_value(right("00000000"||d2x(out_file_addr),8))
    call panel_command "defsym in_file_addr       " || defsym_value(right("00000000"||d2x(in_file_addr),8))
    call panel_command "defsym zp_mask            " || defsym_value(right("00000000"||d2x(zp_mask,8),8))
    call panel_command "defsym zp_bytes           " || defsym_value(right("00000000"||d2x(zp_bytes),4))

    found_start = 0

    /* The following extracts the "cmpsc.txt" instruction  */
    /* test program code which is embedded into our source */

    do  i = 1 for sourceline()

        srcline = strip(strip(strip(sourceline(i)),"leading","-"))

        if  srcline = "" then ,
            iterate

        if  srcline = "test:end" then ,
            leave

        if  srcline = "test:start" then do
            found_start = 1
            iterate i
        end

        if  \found_start then ,
            iterate i

        call panel_command srcline

    end

    /* Exit immediately if a breakpoint has been set */
    /* or when instruction stepping has been enabled */
    /* to unlock command processing so that they can */
    /* press the enter key or enter another command. */

    call panel_command "b"
    parse var msgs.1 . "Instruction break" onoff .
    if onoff = "on" then do
        exit 1
    end

    call panel_command "s"
    parse var msgs.1 . "Instruction stepping" onoff .
    if onoff = "on" then do
        exit 1
    end

    /* Wait for the now running test program to finish */
    /* and then save the output file that it created.  */

    if test_wait(test_type) then do

        -- get o/p file size from gpr 11

        call panel_command "gpr"

        -- HHC02269I General purpose registers
        -- HHC02269I R0=0000000000000000 R1=0000000000000000 R2=0000000000000000 R3=0000000000000000
        -- HHC02269I R4=0000000000000000 R5=0000000000000000 R6=0000000000000000 R7=0000000000000000
        -- HHC02269I R8=0000000000000000 R9=0000000000000000 RA=0000000000000000 RB=0000000000000000
        -- HHC02269I RC=0000000000000000 RD=0000000000000000 RE=0000000000000000 RF=0000000000000000

        if msgs.0 < 5 then do
            call logmsg ""
            call logmsg "*** "test_type" Test SUCCESS ***"
            call logmsg ""
            call logmsg "** ERROR ** 'gpr' command parsing failure; test aborted."
            exit 1
        end
        parse var msgs.4 . " RB=" gpr11 .

        -- save output file

        out_file_size = x2d(gpr11)
        out_file_end_addr = out_file_addr + out_file_size - 1
        call delfile(out_file)
        call panel_command "savecore $(out_file) $(out_file_addr) " || d2x(out_file_end_addr)

        call logmsg ""
        call logmsg "    *** "test_type" Test SUCCESS ***"
        call logmsg ""
        call logmsg '    outfile name: "' || out_file || '"'
        call logmsg "    outfile size:  " || out_file_size || " bytes"
        call logmsg ""

    end
    return

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

test_wait: -- (wait for test to complete; return success/failure boolean)

    test_type = arg(1)

    ok = 0      -- (it is safest to presume failure)

    do forever

        /* Query the PSW at regular intervals until we */
        /* see a PSW that indicates the test has ended */

        call SysSleep 1.0
        call panel_command "psw"

        -- HHC02278I Program status word: 0000000000000000 0000000000000000
        -- HHC02300I sm=00 pk=0 cmwp=0 as=pri cc=0 pm=0 am=24 ia=0

        if msgs.0 < 2 then do
            call logmsg "** ERROR ** 'psw' command parsing failure; test aborted."
            exit 1
        end
        parse var msgs.2 . "cmwp=" cmwp " " . "ia=" psw_ia

        if psw_ia = "0" then do
            ok = 1 -- (success)
            leave
        end

        if psw_ia = "C4" then do
            call logmsg ""
            call logmsg "*** "test_type" Test FAILED ***   (Buffer Overflow Detected)"
            call logmsg ""
            leave
        end

        if psw_ia = "BADCC" then do
            call logmsg ""
            call logmsg "*** "test_type" Test FAILED ***   (Invalid cc=2 from CMPSC)"
            call logmsg ""
            leave
        end

        if psw_ia = "DEAD" then do

            call panel_command "r 8e.2" -- (program interrupt code)

            -- HHC02290I R:000000000000008E:K:00=0000 00000000 00000000 00000000 0000 ................

            if msgs.0 < 1 then do
                call logmsg "** ERROR ** 'r 8e.2' command parsing failure; test aborted."
                exit 1
            end
            parse upper var msgs.1 . "=" code " " .

            if code = "00C4" then reason = "0C4 Protection Exception" ; else ,
            if code = "00C7" then reason = "0C7 Data Exception"       ; else ,
                                  reason = "Program Interrupt Code " || code

            call logmsg ""
            call logmsg "*** "test_type" Test FAILED ***   ("reason")"
            call logmsg ""
            leave
        end

        if psw_ia = "2616" then do
            call logmsg ""
            call logmsg "*** "test_type" Test FAILED ***   (Zero Padding Error Detected)"
            call logmsg ""
            leave
        end

        if cmwp = "A" then do
            call logmsg ""
            call logmsg "*** "test_type" Test FAILED ***   (Unexpected disabled wait PSW)"
            call logmsg ""
            leave
        end
    end
    return ok       -- (true = success, false = failure)

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

defsym_value: -- (wrap value within double quotes if it contains any blanks)

    result = strip(arg(1),,'"')
    if  pos(" ",result) > 0 then
        result = '"' || result || '"'
    return result

defsym_filename: -- (wrap value yet again within apostrophes if needed)

    -- Windows compatibility: wrap the double-quoted value within
    -- single quotes (apostrophes) so that the double quotes are
    -- included as part of the defsym'ed value

    result = defsym_value(arg(1))
    if  left(result,1) = '"' then
        result = "'" || result || "'"
    return result

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

panel_command: procedure expose who msgs. debug -- (issue Hercules command)

    cmdline = arg(1)
    rc = awscmd("-"||cmdline,"msgs")
    if debug then do
        do i=1 for msgs.0
            say msgs.i
        end
    end
    return rc

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

logmsg: procedure expose who -- (say something prefixed with our name)

    say who": "arg(1)
    return

isnum: procedure -- (is it a number?)

    return arg(1) <> "" & datatype(arg(1),"N");

fullpath: procedure expose pathsep -- (locate file or dir, return full path or null)

    fp = qualify(arg(1))
    if SysIsFileDirectory(fp) then ,
        return dirnamefmt(fp)
    if SysIsFile(fp) then ,
        return fp
    return ""

delfile: procedure -- (delete a file if it exists)

    if  SysIsFile(arg(1)) then
        call SysFileDelete(arg(1))
    return

dirnamefmt: procedure expose pathsep -- (convert to dir format)

    dn = arg(1)
    do  while pos(pathsep||pathsep,dn) > 0
        dn = changestr(pathsep||pathsep,dn,pathsep)
    end
    if  qualify(dn) <> qualify(dn||pathsep) then
        dn ||= pathsep
    if SysIsFileDirectory(dn) then do
        if right(dn,1) \= pathsep then ,
            dn ||= pathsep
    end
    return dn

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

help:

    found_start = 0

    do  i = 1 for sourceline()

        srcline = strip(sourceline(i))

        if  srcline = "" then ,
            iterate

        srcline = strip(srcline,"leading","-")

        if  strip(srcline) = "help:end" then ,
            leave

        if  strip(srcline) = "help:start" then do
            found_start = 1
            iterate i
        end

        if  \found_start then ,
            iterate i

        srcline = changestr("<scriptname>",srcline,scriptname)
        say strip(srcline,"T") -- (use 'say' here, *NOT* 'logmsg'!)

    end
    exit 1

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

herror:

    call logmsg "signal on error trapped"
    call logmsg "ended"
    exit 1

hfailure:

    call logmsg "signal on failure trapped"
    call logmsg "ended"
    exit 1

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/*
                            CMPSC instruction test

    This test is designed to be driven by the REXX script called CMPSC.REXX.
    CMPSC.REXX issues the needed defsym commands to define the actual test
    parameters which this test script then uses to perform the actual test.
    The assembler source code for this test is in file "util\CMPSC.txt".
    "CMPSC.txt" is purposely designed to be compression/expansion neutral.
*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

-- test:start

-- archlvl z/arch
-- defstore main 16
-- sysclear
-- sysreset

-- loadcore  $(cmp_dict_name)  $(cmp_dict_addr)
-- loadcore  $(exp_dict_name)  $(exp_dict_addr)
-- loadcore  $(in_file)        $(in_file_addr)

-- gpr 0=$(GR0)
-- gpr 1=$(cmp_dict_addr)
-- gpr 2=$(out_buffer_addr)
-- gpr 3=$(out_buffer_size)
-- gpr 4=$(in_buffer_addr)
-- gpr 5=$(in_buffer_size)

-- r 1A0=0000000080000000
-- r 1A8=0000000000000200

-- r 1D0=0002000080000000
-- r 1D8=000000000000DEAD

/* The actual "CMPSC.txt" test program code begins here */

-- r 200=0DC0
-- r 202=06C0
-- r 204=06C0
-- r 206=9805C658
-- r 20A=1862
-- r 20C=41732000
-- r 210=5070C688
-- r 214=4DE0C07A
-- r 218=41607FFF
-- r 21C=8860000C
-- r 220=8960000C
-- r 224=1F67
-- r 226=5060C68C
-- r 22A=1864
-- r 22C=41754000
-- r 230=4DE0C07A
-- r 234=1861
-- r 236=8860000C
-- r 23A=8960000C
-- r 23E=1876
-- r 240=5E70C670
-- r 244=4DE0C07A
-- r 248=D20FC60801D0
-- r 24E=41A0C064
-- r 252=BEA701DD
-- r 256=94FD01D1
-- r 25A=18A0
-- r 25C=41000001
-- r 260=B2B0C600
-- r 264=D20F01D0C608
-- r 26A=180A
-- r 26C=58A0C678
-- r 270=58B0C680
-- r 274=1F55
-- r 276=47F0C0B0
-- r 27A=8860000C
-- r 27E=8960000C
-- r 282=18D7
-- r 284=41D0DFFF
-- r 288=88D0000C
-- r 28C=89D0000C
-- r 290=41A00010
-- r 294=41F00038
-- r 298=B22B00A6
-- r 29C=41606800
-- r 2A0=41606800
-- r 2A4=156D
-- r 2A6=4740C098
-- r 2AA=B22B00F6
-- r 2AE=07FE
-- r 2B0=4DE0C120
-- r 2B4=4DE0C236
-- r 2B8=B20A0010
-- r 2BC=B2630024
-- r 2C0=4710C0BC
-- r 2C4=B20A0000
-- r 2C8=4720C11C
-- r 2CC=9200C69F
-- r 2D0=4780C0D8
-- r 2D4=9201C69F
-- r 2D8=4DE0C15E
-- r 2DC=4DE0C21C
-- r 2E0=4DE0C1FA
-- r 2E4=12AA
-- r 2E6=4770C0B0
-- r 2EA=9500C69F
-- r 2EE=4770C0B0
-- r 2F2=5400C690
-- r 2F6=4770C114
-- r 2FA=5410C684
-- r 2FE=4780C114
-- r 302=5880C660
-- r 306=5860C67C
-- r 30A=D20060008000
-- r 310=41B0B001
-- r 314=50B0C680
-- r 318=B2B2C618
-- r 31C=B2B2C628
-- r 320=129A
-- r 322=078E
-- r 324=9867C668
-- r 328=18D7
-- r 32A=1275
-- r 32C=4780C136
-- r 330=1884
-- r 332=1897
-- r 334=0E68
-- r 336=187D
-- r 338=1F75
-- r 33A=5880C674
-- r 33E=189A
-- r 340=1597
-- r 342=47B0C148
-- r 346=1879
-- r 348=0E68
-- r 34A=5080C674
-- r 34E=5090C678
-- r 352=18A9
-- r 354=5840C668
-- r 358=1856
-- r 35A=1F54
-- r 35C=07FE
-- r 35E=1862
-- r 360=9878C660
-- r 364=1E78
-- r 366=1F76
-- r 368=1277
-- r 36A=078E
-- r 36C=5880C658
-- r 370=5480C690
-- r 374=4770C182
-- r 378=0670
-- r 37A=1277
-- r 37C=078E
-- r 37E=41606001
-- r 382=48D0C69C
-- r 386=1ED6
-- r 388=06D0
-- r 38A=54D0C698
-- r 38E=1FD6
-- r 390=12DD
-- r 392=47B0C198
-- r 396=1FDD
-- r 398=12DD
-- r 39A=4780C1EE
-- r 39E=15D7
-- r 3A0=4720C1EE
-- r 3A4=9101C605
-- r 3A8=4780C1EE
-- r 3AC=5880C658
-- r 3B0=5480C694
-- r 3B4=4780C1EE
-- r 3B8=95006000
-- r 3BC=4780C1CC
-- r 3C0=1886
-- r 3C2=1F99
-- r 3C4=BF98C69E
-- r 3C8=47F0C1EE
-- r 3CC=18F7
-- r 3CE=187D
-- r 3D0=1886
-- r 3D2=1F99
-- r 3D4=0F68
-- r 3D6=4770C1F6
-- r 3DA=187F
-- r 3DC=1F7D
-- r 3DE=1886
-- r 3E0=1F99
-- r 3E2=BF98C69E
-- r 3E6=0F68
-- r 3E8=078E
-- r 3EA=47F0C1F6
-- r 3EE=0F68
-- r 3F0=078E
-- r 3F2=47F0C1F6
-- r 3F6=B2B2C648
-- r 3FA=9889C660
-- r 3FE=1F93
-- r 400=5860C67C
-- r 404=1879
-- r 406=1EB9
-- r 408=18D8
-- r 40A=0E68
-- r 40C=5060C67C
-- r 410=D200D0002000
-- r 416=9823C660
-- r 41A=07FE
-- r 41C=9867C688
-- r 420=1277
-- r 422=078E
-- r 424=1F99
-- r 426=BF98C69E
-- r 42A=0F68
-- r 42C=4770C232
-- r 430=07FE
-- r 432=B2B2C638
-- r 436=41D000FF
-- r 43A=1880
-- r 43C=5480C690
-- r 440=4770C256
-- r 444=1881
-- r 446=5480C684
-- r 44A=88D08000
-- r 44E=43802000
-- r 452=168D
-- r 454=18D8
-- r 456=42D02000
-- r 45A=41602001
-- r 45E=1886
-- r 460=1873
-- r 462=0670
-- r 464=1F99
-- r 466=BF98C69E
-- r 46A=0E68
-- r 46C=9867C688
-- r 470=1277
-- r 472=078E
-- r 474=0E68
-- r 476=07FE

-- r 800=0000000000000000
-- r 808=0000000000000000
-- r 810=0000000000000000
-- r 818=0002000080000000
-- r 820=0000000000000000
-- r 828=0002200080000000
-- r 830=00000000000BADCC
-- r 838=0002000080000000
-- r 840=00000000000000C4
-- r 848=0002000080000000
-- r 850=0000000000002616

-- r 858=$(GR0)
-- r 85C=$(cmp_dict_addr)
-- r 860=$(out_buffer_addr)
-- r 864=$(out_buffer_size)
-- r 868=$(in_buffer_addr)
-- r 86C=$(in_buffer_size)
-- r 870=$(dicts_size)
-- r 874=$(in_file_addr)
-- r 878=$(in_file_size)
-- r 87C=$(out_file_addr)

-- r 880=00000000
-- r 884=00000007
-- r 888=00000000
-- r 88C=00000000
-- r 890=00000100
-- r 894=00020000

-- r 898=$(zp_mask)
-- r 89C=$(zp_bytes)

-- r 89E=FF
-- r 89F=00

-- restart

-- test:end
