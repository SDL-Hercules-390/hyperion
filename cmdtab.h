/* CMDTAB.H     (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright "Fish" (David B. Trout), 2002-2012     */
/*              (C) Copyright Jan Jaeger, 2003-2012                  */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*              (C) and others 2013-2023                             */
/*              Defines all Hercules Configuration statements        */
/*              and panel commands                                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*              Command descriptions and help text                   */
/*-------------------------------------------------------------------*/

#if defined( _FW_REF )          /* (pre-table build pass) */

//  -------------- (template for new commands) ----------------
//  -------------- (template for new commands) ----------------
//
//#define xxx_cmd_desc            "Short XXX description"
//#define xxx_cmd_help            <backslash>
//                                <backslash>
//  "Much longer, more detailed xxx command description...\n" <backslash>
//  "Use other commands as reference for typical formatting and wording.\n" <backslash>
//
//  -------------- (template for new commands) ----------------
//  -------------- (template for new commands) ----------------

#define $locate_cmd_desc        "Display sysblk, regs or hostinfo"

#define $runtest_cmd_desc       "Start the test if test mode is active"
#define $runtest_cmd_help       \
                                \
  "Issue 'restart' command and then wait for all processors to load a\n"        \
  "disabled wait PSW. This is a scripting only command and is only valid\n"     \
  "when test mode is active. Test mode can only be activated by specifying\n"   \
  "the '-t' command line switch when Hercules is started. Runtest supports\n"   \
  "a single optional argument being the expected test duration in seconds.\n"   \
  "If not specified then a default value is used. If the test runs longer\n"    \
  "than the expected time an error message is issued and the test aborts.\n"

#define $test_cmd_desc          "Your custom command (*DANGEROUS!*)"
#define $test_cmd_help          \
                                \
  "Performs whatever test function *YOU* specifically coded it to do.\n\n"      \
                                                                                \
  "                  * * * *  WARNING!  * * * *\n\n"                            \
                                                                                \
  "DO NOT RUN THIS COMMAND UNLESS *YOU* SPECIFICALLY CODED THE FUNCTION\n"      \
  "THAT THIS COMMAND INVOKES! Unless you wrote it yourself you probably\n"      \
  "don't know it does. It could perform any function at all from crashing\n"    \
  "Hercules to launching a nuclear strike. You have been warned!\n"

#define $zapcmd_cmd_desc        "Enable/disable command (*CAREFUL!*)"
#define $zapcmd_cmd_help        \
                                \
  "Format:\n\n"                                                                 \
                                                                                \
  "    $zapcmd  cmdname  CFG|NOCFG|CMD|NOCMD\n\n"                               \
                                                                                \
  "For normal non-Debug production release builds, use the sequence:\n\n"       \
                                                                                \
    "    msglvl   VERBOSE      (optional)\n"                                    \
    "    msglvl   DEBUG        (optional)\n"                                    \
    "    cmdlvl   DEBUG        (*required!*) (because not DEBUG build,\n"       \
    "    $zapcmd  cmdname  CMD                and $zapcmd is SYSDEBUG)\n\n"     \
                                                                                \
    "In other words, the $zapcmd is itself a 'debug' level command, and\n"      \
    "thus in order to use it, the debug cmdlvl must be set first (which\n"      \
    "is the default for Debug builds but not normal production builds).\n"      \
    "Note: it is possible to disable the $zapcmd itself so BE CAREFUL!\n"

#define bangmsg_cmd_desc        "SCP priority command"
#define bangmsg_cmd_help        \
                                \
  "To enter a system control program (i.e. guest operating system)\n"           \
  "priority command on the Hercules console, simply prefix the command\n"       \
  "with an '!' exclamation point.\n"

#define reply_cmd_desc          "SCP reply"
#define reply_cmd_help          \
                                \
  "To reply to a system control program (i.e. guest operating system)\n"        \
  "prompt that gets issued to the Hercules console, simply prefix the\n"        \
  "reply with a '.' period.\n"

#define supp_reply_cmd_desc     "SCP suppressed reply"
#define supp_reply_cmd_help     \
                                \
  "To reply to a system control program (i.e. guest operating system)\n"        \
  "prompt that gets issued to the Hercules console without echoing it\n"        \
  "to the console (such as when entering a password), simply prefix the\n"      \
  "reply with a '\\' backslash.\n"

#define iconpfxs_cmd_desc       "Default integrated console prefix characters"
#define iconpfxs_cmd_help       \
                                \
  "Format: \"ICONPFXS [string | *]\" where 'string' is the new list of\n"       \
  "prefix characters you wish to use as the defaults for integrated console\n"  \
  "devices. Refer to documentation for 1052-C and 3215-C devices for what\n"    \
  "prefix characters are used for. Each character in the list must be unique.\n"\
  "The default list is \""DEF_CMDPREFIXES"\". Enter the command with\n"         \
  "no arguments to display the current list. Use '*' to reset the list to\n"   \
  "its original default value.\n"

#define hash_cmd_desc           "Silent comment"
#define star_cmd_desc           "Loud comment"

#define quest_cmd_desc          "alias for help"
#define abs_cmd_desc            "Display or alter absolute storage"
#define abs_cmd_help            \
                                \
  "Format: \"abs addr[.len]\" or \"abs addr[-addr2]\" to display up to 64K\n"   \
  "of absolute storage, or \"abs addr=value\" to alter up to 32 bytes of\n"     \
  "absolute storage, where 'value' is either a string of up to 32 pairs of\n"   \
  "hex digits, or a string of up to 32 characters enclosed within single or\n"  \
  "double quotes.\n"

#define aea_cmd_desc            "Display AEA tables"
#define aia_cmd_desc            "Display AIA fields"
#define alrf_cmd_desc           "Command deprecated. Use facility command instead"
#define ar_cmd_desc             "Display access registers"

#define archlvl_cmd_desc        "Set or Query current Architecture Mode"
#define archlvl_cmd_help        \
                                \
  "Format: ARCHLVL  S/370 | ESA/390 | z/ARCH\n"                                 \
  "\n"                                                                          \
  "Entering the command without arguments displays the current architecture\n"  \
  "mode. Entering the command with an argument sets the architecture mode.\n"   \
  "To enable/disable/query facilities please use the new FACILITY command.\n"

#define archmode_cmd_desc       "Deprecated. Use the archlvl command instead"
#define asnlx_cmd_desc          "Command deprecated. Use facility command instead"
#define attach_cmd_desc         "Configure device"
#define attach_cmd_help         \
                                \
  "Format: \"attach devn type [arg...]\n"

#define autoscsi_cmd_desc       "Command deprecated - Use \"SCSIMOUNT\""
#define autoscsi_cmd_help       \
                                \
  "This command is deprecated. Use \"scsimount\" instead.\n"

#define autoinit_cmd_desc       "Display/Set auto-create-empty-tape-file option"
#define autoinit_cmd_help       \
                                \
  "Format: \"AUTOINIT  [ ON | OFF ]\"\n"                                        \
  "\n"                                                                          \
  "The AUTOINIT option controls whether emulated tape drive device files\n"     \
  "will be automatically created or not when it is discovered they do not\n"    \
  "yet exist.\n"                                                                \
  "\n"                                                                          \
  "When AUTOINIT is ON (the default), a devinit command specifying a file\n"    \
  "that does not yet exist causes the tape driver to automatically create\n"    \
  "an unlabeled tape volume consisting of just two tapemarks whenever it\n"     \
  "discovered the specified file does not exist.\n"                             \
  "\n"                                                                          \
  "When AUTOINIT is OFF, a devinit command instead fails with an expected\n"    \
  "\"file not found\" error if the specified tape file does not exist.\n"       \
  "\n"                                                                          \
  "Entering the command without any arguments displays the current setting.\n"

#define automount_cmd_desc      "Display/Update allowable tape automount directories"
#define automount_cmd_help      \
                                \
  "Format: \"automount  { add <dir> | del <dir> | list }\"\n"                   \
  "\n"                                                                          \
  "Adds or deletes entries from the list of allowable/unallowable tape\n"       \
  "automount directories, or lists all currently defined list entries,\n"       \
  "if any.\n"                                                                   \
  "\n"                                                                          \
  "The format of the <dir> directory operand for add/del operations is\n"       \
  "identical to that as described in the documentation for the AUTOMOUNT\n"     \
  "configuration file statement (i.e. prefix with '+' or '-' as needed).\n"     \
  "\n"                                                                          \
  "The automount feature is appropriately enabled or disabled for all\n"        \
  "tape devices as needed depending on the updated empty/non-empty list\n"      \
  "state.\n"

#define bplus_cmd_desc          "(Synonym for 'b')"
#define bminus_cmd_desc         "Delete breakpoint"
#define bquest_cmd_desc         "Query breakpoint"
#define b_cmd_desc              "Set breakpoint"
#define b_cmd_help              \
                                \
  "Format:\n"                                                                   \
  "\n"                                                                          \
  "     \"b addr-addr   [asid]\"\n"                                             \
  "     \"b addr:addr   [asid]\"\n"                                             \
  "     \"b addr.length [asid]\"\n"                                             \
  "\n"                                                                          \
  "Sets the instruction address or address range where you wish to halt\n"      \
  "execution.  This command is synonymous with the \"s+\" command.\n"

#define bear_cmd_desc           "Display or set BEAR register"
#define bear_cmd_help           \
                                \
  "Format: \"bear [address]\" where 'address' is value the BEAR register\n"     \
  "should be set to. Enter the command without any operand to just display\n"   \
  "the current value of the BEAR register.\n"

#define cachestats_cmd_desc     "Cache stats command"

#define cckd_cmd_desc           "Compressed CKD command"
#define cckd_cmd_help           \
                                \
  "The cckd command is used to display current cckd processing options\n"       \
  "or statistics or to set new cckd processing options:\n"                      \
                                                                         "\n"   \
  " cckd  help         Display cckd help\n"                                     \
  " cckd  stats        Display current cckd statistics\n"                       \
  " cckd  opts         Display current cckd options\n"                          \
  " cckd  opt=val,...  Set cckd option. Multiple options may be specified.\n"   \
  "                    Each option must be separated from the next with a\n"    \
  "                    single comma and no intervening blanks. The list of\n"   \
  "                    supported cckd options are:\n"                           \
                                                                         "\n"   \
  "  comp=n        Override compression                  (-1,0,1,2)\n"          \
  "  compparm=n    Override compression parm             (-1 ... 9)\n"          \
  "  debug=n       Enable CCW tracing debug messages       (0 or 1)\n"          \
  "  dtax=n        Dump trace table at exit                (0 or 1)\n"          \
  "  freepend=n    Set free pending cycles               (-1 ... 4)\n"          \
  "  fsync=n       Enable fsync                            (0 or 1)\n"          \
  "  gcint=n       Set garbage collector interval (sec)  ( 0 .. 60)\n"          \
  "  gcmsgs=n      Display garbage collector messages      (0 or 1)\n"          \
  "  gcparm=n      Set garbage collector parameter       (-8 ... 8)\n"          \
  "  gcstart=n     Start garbage collector                 (0 or 1)\n"          \
  "  linuxnull=n   Check for null linux tracks             (0 or 1)\n"          \
  "  nosfd=n       Disable stats report at close           (0 or 1)\n"          \
  "  nostress=n    Disable stress writes                   (0 or 1)\n"          \
  "  ra=n          Set number readahead threads          ( 1 ... 9)\n"          \
  "  raq=n         Set readahead queue size              ( 0 .. 16)\n"          \
  "  rat=n         Set number tracks to read ahead       ( 0 .. 16)\n"          \
  "  trace=n       Set trace table size              (0 ... 200000)\n"          \
  "  wr=n          Set number writer threads             ( 1 ... 9)\n"          \
                                                                         "\n"   \
  "Refer to the Hercules CCKD documentation web page for more information.\n"

#define cctape_cmd_desc         "Display a printer's current cctape"
#define cctape_cmd_help         "Format: \"cctape <devnum>\""
#define cf_cmd_desc             "Configure current CPU online or offline"
#define cf_cmd_help             \
                                \
  "Configure current CPU online or offline:  Format->  \"cf [on|off]\"\n"       \
  "Where the 'current' CPU is defined as whatever CPU was defined as\n"         \
  "the panel command target cpu via the \"cpu\" panel command. (Refer\n"        \
  "to the 'cpu' command for further information) Entering 'cf' by itself\n"     \
  "simply displays the current online/offline status of the current cpu.\n"     \
  "Otherwise the current cpu is configured online or offline as\n"              \
  "specified.\n"                                                                \
  "Use 'cfall' to configure/display all CPUs online/offline state.\n"

#define cfall_cmd_desc          "Configure all CPU's online or offline"
#define clocks_cmd_desc         "Display tod clkc and cpu timer"
#define cmdlvl_cmd_desc         "Display/Set current command group"
#define cmdlvl_cmd_help         \
                                \
  "Format: cmdlevel [{+/-}{ALL|MAINT|PROG|OPER|DEVEL|DEBUG}...]\n"

#define cmdsep_cmd_desc         "Display/Set command line separator"
#define cmdsep_cmd_help         \
                                \
  "A command line separator character allows multiple commands\n"               \
  "to be entered on a single line.\n"                                           \
  "\n"                                                                          \
  "Format: CMDSEP  [OFF | c ]\n"                                                \
  "        c       a single character used to separate commands.\n"             \
  "        OFF     disables command separation.\n"

#define cnslport_cmd_desc       "Set console port"

#if defined(_FEATURE_047_CMPSC_ENH_FACILITY)
#define cmpscpad_cmd_desc       "Set/display the CMPSC zero padding value."
#define cmpscpad_cmd_help       \
                                \
  "The CMPSCPAD command defines the zero padding storage alignment boundary\n"  \
  "for the CMPSC-Enhancement Facility. It must be a power of 2 value ranging\n" \
  "anywhere from " QSTR( MIN_CMPSC_ZP_BITS ) " to " QSTR( MAX_CMPSC_ZP_BITS ) ". Enter the command with no arguments to display the\n" \
  "current value.\n"

#endif /* defined(_FEATURE_047_CMPSC_ENH_FACILITY) */

#define codepage_cmd_desc       "Set/display code page conversion table"
#define codepage_cmd_help       \
                                \
  "Format: 'codepage [cp]'\n"                                                   \
  "        'codepage maint cmd [operands]' - see cp_updt command for\n"         \
  "                                          help\n"                            \
  "If no operand is specified, the current codepage is displayed.\n"            \
  "If 'cp' is specified, then code page is set to the specified page\n"         \
  "if the page is valid.\n"

#define conkpalv_cmd_desc       "Display/alter console TCP keepalive settings"
#define conkpalv_cmd_help       \
                                \
  "Format: \"conkpalv (idle,intv,count)\" where 'idle', 'intv' and 'count'\n"   \
  "are the new values for the TCP keepalive settings for console\n"             \
  "connections:\n"                                                              \
  "\n"                                                                          \
  "   Begin probing after connection has been idle for 'idle' seconds\n"        \
  "   wait for a maximum of 'intv' seconds for a probe response\n"              \
  "   Close the connection once 'count' consecutive probes have failed\n"       \
  "\n"                                                                          \
  "The format must be exactly as shown, with each value separated from\n"       \
  "the next by a single comma, no intervening spaces between them, and\n"       \
  "surrounded within parenthesis. Enter the command without any operands\n"     \
  "to display the current values.\n"

#define cp_updt_cmd_desc        "Create/Modify user character conversion table"
#define cp_updt_cmd_help        \
                                \
  "Format: 'cp_updt cmd [operands]'\n"                                          \
  "\n"                                                                          \
  "  altER e|g2h|a|h2g (pos, val, pos, val,...)\n"                              \
  "                              - alter the user eBCDIC/g2h or aSCII/h2g\n"    \
  "                                table value at hex POSition to hex\n"        \
  "                                VALue 16 pairs of hex digits may be\n"       \
  "                                specified within the parens\n"               \
  "\n"                                                                          \
  "  dsp|disPLAY e|g2h|a|h2g     - display user eBCDIC|aSCII table\n"           \
  "\n"                                                                          \
  "  expORT e|g2h|a|h2g filename - export contents of user table to file\n"     \
  "\n"                                                                          \
  "  impORT e|g2h|a|h2g filename - import file contents into user table\n"      \
  "\n"                                                                          \
  "  refERENCE [cp]              - copy codepage to user tables\n"              \
  "                                if cp is not specified, a list of\n"         \
  "                                valid codepage tables is generated\n"        \
  "\n"                                                                          \
  "  reset                       - reset the internal user tables to\n"         \
  "                                binary zero\n"                               \
  "\n"                                                                          \
  "  test                        - verify that user tables are\n"               \
  "                                transparent, i.e. the value at\n"            \
  "                                position n in g2h used as an index\n"        \
  "                                into h2g will return a value equal n\n"      \
  "                                ( g2h<=>h2g, h2g<=>g2h )\n"                  \
  "\n"                                                                          \
  " *e|g2h represent ebcdic; a|h2g represent ascii\n"                           \
  " **lower case part of the cmd name represent the minimum abbreviation\n"     \
  "   for the command\n"                                                        \
  "\n"                                                                          \
  "To activate the user table, enter the command 'codepage user'\n"             \
  "\n"                                                                          \
  "Note: ebcdic refers to guest to host translation\n"                          \
  "      ascii refers to host to guest translation\n"                           \
  "      These terms are used for historical purposes and do not\n"             \
  "      represent the literal term.\n"

#define cpu_cmd_desc            "Define target cpu for panel display and commands"
#define cpu_cmd_help            \
                                \
  "Format: \"cpu [xx [cmd]]\" where 'xx' is the hexadecimal cpu address \n"     \
  "of the cpu in your multiprocessor configuration you wish all panel\n"        \
  "commands to apply to.  If command text follows the cpu address, the\n"       \
  "command executes on cpu xx and the target cpu is not changed.\n"             \
  "\n"                                                                          \
  "For example, entering 'cpu 1F' followed by \"gpr\" will change the\n"        \
  "target cpu for the panel display and commands and then display the\n"        \
  "general purpose registers for cpu 31 of your configuration, whereas\n"       \
  "\"cpu 14 gpr\" executes the 'gpr' command on cpu 20, but does not\n"         \
  "change the target cpu for subsequent panel displays and commands.\n"         \
  "\n"                                                                          \
  "Entering the command with no arguments displays the current value.\n"

#define cpuidfmt_cmd_desc       "Set format BASIC/0/1 STIDP generation"
#define cpumodel_cmd_desc       "Set CPU model number"
#define cpuserial_cmd_desc      "Set CPU serial number"
#define cpuverid_cmd_desc       "Set CPU verion number"
#define cpuverid_cmd_help       \
                                \
  "Format: \"cpuverid xx [force]\" where 'xx' is the 2 hexadecimal digit\n"     \
  "CPU version code stored by the STIDP instruction.\n"                         \
  "\n"                                                                          \
  "The default cpuverid version code at startup is 'FD', and that value will\n" \
  "be stored by the STIDP instruction -- even for z/Arch -- unless and UNTIL\n" \
  "you set it to a different value via the 'cpuverid' command/statement.\n"     \
  "\n"                                                                          \
  "If you try using the cpuverid command/statement to set a non-zero cpuverid\n"\
  "value when the architecture mode is currently set to z/Arch, the version\n"  \
  "code stored by the STIDP instruction will STILL be stored as '00' anyway,\n" \
  "UNLESS ... the 'FORCE' option is used. For z/Arch, the 'FORCE' option is\n"  \
  "the ONLY way to cause the cpuverid command to force the STIDP instruction\n" \
  "to store a non-zero version code. (But as explained, at startup, the value\n"\
  "stored will STILL be 'FD' even for z/Arch since that is the default. This\n" \
  "means if you want your STIDP version code to be '00' for z/Arch, then you\n" \
  "MUST use a 'cpuverid' command/statement in your configuration file!)\n"

#define cr_cmd_desc             "Display or alter control registers"
#define cr_cmd_help             \
                                \
  "Format: \"cr [nn=xxxxxxxxxxxxxxxx]\" where 'nn' is the optional\n"           \
  "control register number (0 to 15) and 'xxxxxxxxxxxxxxxx' is the\n"           \
  "control register value in hex (1-8 hex digits for 32-bit registers\n"        \
  "or 1-16 hex digits for 64-bit registers). Enter \"cr\" by itself to\n"       \
  "display the control registers without altering them.\n"

#define cscript_cmd_desc        "Cancels a running script thread"
#define cscript_cmd_help        \
                                \
  "Format: \"cscript  ['*' | 'ALL' | id]\".  This command cancels a running\n"  \
  "script or all scripts. If '*' or 'ALL' is given then all running scripts\n"  \
  "are canceled. If no arguments are given only the first running script is\n"  \
  "cancelled. Otherwise the specific script 'id' is canceled. The 'script'\n"   \
  "command may be used to display a list of all currently running scripts.\n"

#define ctc_cmd_desc            "Enable/Disable CTC debugging"
#define ctc_cmd_help            \
                                \
  "Format:  \"ctc  debug  [ on | off | startup  [ <devnum> | ALL ]]\".\n"       \
  "\n"                                                                          \
  "Enables/disables debug packet tracing for the specified CTCI/LCS/PTP/CTCE\n" \
  "device group(s) identified by <devnum> or for all CTCI/LCS/PTP/CTCE device\n"\
  "groups if <devnum> is not specified or specified as 'ALL'.\n"                \
  "\n"                                                                          \
  "Note: only CTCE devices support 'startup' debugging.\n"                      \
  "\n"                                                                          \
  "Use the command \"ctc debug\" (without any other operands) to list the\n"    \
  "current CTC debugging state for all CTC devices.\n"

#define define_cmd_desc         "Rename device"
#define define_cmd_help         \
                                \
  "Format: \"define olddevn newdevn\"\n"

#define defsym_cmd_desc         "Define symbol"
#define defsym_cmd_help         \
                                \
  "Format: \"defsym symbol [value]\". Defines symbol 'symbol' to contain\n"     \
  "value 'value'. The symbol can then be the object of a substitution for\n"    \
  "later panel commands. If 'value' contains blanks or spaces, then it\n"       \
  "must be enclosed within quotes or apostrophes. For more detailed\n"          \
  "information regarding symbol substitution refer to the 'DEFSYM'\n"           \
  "configuration file statement in Hercules documentation.\n"                   \
  "Enter \"defsym\" by itself to display the values of all defined\n"           \
  "symbols.\n"

#define delsym_cmd_desc         "Delete a symbol"
#define delsym_cmd_help         \
                                \
  "Format: \"delsym symbol\". Deletes symbol 'symbol'.\n"

#define detach_cmd_desc         "Remove device"
#define detach_cmd_help         \
                                \
  "Format: \"detach devn [FORCE]\"\n"                                           \
  "Where 'devn' is the device address of the device to be removed from\n"       \
  "the hardware configuration. Use the 'FORCE' option to forcibly remove\n"     \
  "devices which are still in use (are currently 'busy' performing I/O).\n"     \
  "Note that using the FORCE option is inherently DANGEROUS and can easily\n"   \
  "cause Hercules to CRASH!\n"

#define devinit_cmd_desc        "Reinitialize device"
#define devinit_cmd_help        \
                                \
  "Format: \"devinit devn [arg...]\"\n"                                         \
  "If no arguments are given then the same arguments are used\n"                \
  "as were used the last time the device was created/initialized.\n"

#define devlist_cmd_desc        "List device, device class, or all devices"
#define devlist_cmd_help        \
                                \
  "Format: \"devlist [devn | devc]\"\n"                                         \
  "    devn       is a single device address\n"                                 \
  "    devc       is a single device class. Device classes are CHAN, CON,\n"    \
  "               CTCA, DASD, DSP, FCP, LINE, OSA, PCH, PRT, RDR, and TAPE.\n"  \
  "\n"                                                                          \
  "If no arguments are given then all devices will be listed.\n"

#define devtmax_cmd_desc        "Display or set max device threads"
#define devtmax_cmd_help        \
                                \
  "Specifies the maximum number of device threads allowed.\n"                   \
  "\n"                                                                          \
  "Specify -1 to cause 'one time only' temporary threads to be created\n"       \
  "to service each I/O request to a device. Once the I/O request is\n"          \
  "complete, the thread exits. Subsequent I/O to the same device will\n"        \
  "cause another worker thread to be created again.\n"                          \
  "\n"                                                                          \
  "Specify 0 to cause an unlimited number of 'semi-permanent' threads\n"        \
  "to be created on an 'as-needed' basis. With this option, a thread\n"         \
  "is created to service an I/O request for a device if one doesn't\n"          \
  "already exist, but once the I/O is complete, the thread enters an\n"         \
  "idle state waiting for new work. If a new I/O request for the device\n"      \
  "arrives before the timeout period expires, the existing thread will\n"       \
  "be reused. The timeout value is currently hard coded at 5 minutes.\n"        \
  "Note that this option can cause one thread (or possibly more) to be\n"       \
  "created for each device defined in your configuration. Specifying 0\n"       \
  "means there is no limit to the number of threads that can be created.\n"     \
  "\n"                                                                          \
  "Specify a value from 1 to nnn  to set an upper limit to the number of\n"     \
  "threads that can be created to service any I/O request to any device.\n"     \
  "Like the 0 option, each thread, once done servicing an I/O request,\n"       \
  "enters an idle state. If a new request arrives before the timeout\n"         \
  "period expires, the thread is reused. If all threads are busy when a\n"      \
  "new I/O request arrives however, a new thread is created only if the\n"      \
  "specified maximum has not yet been reached. If the specified maximum\n"      \
  "number of threads has already been reached, then the I/O request is\n"       \
  "placed in a queue and will be serviced by the first available thread\n"      \
  "(i.e. by whichever thread becomes idle first). This option was created\n"    \
  "to address a threading issue (possibly related to the cygwin Pthreads\n"     \
  "implementation) on Windows systems.\n"                                       \
  "\n"                                                                          \
  "The default for Windows is 8. The default for all other systems is 0.\n"

#define diag8_cmd_desc          "Set DIAG 8 instruction options"
#define diag8_cmd_help          \
                                \
  "Format:  \"diag8cmd  [DISABLE|ENABLE]  [ECHO|NOECHO]\".\n"                   \
  "\n"                                                                          \
  "When ENABLE is specified the Hercules Diagnose 8 instruction interface\n"    \
  "is enabled, allowing guests to directly issue Hercules commands via the\n"   \
  "Hercules Diagnose 8 instruction.  When set to DISABLE such instructions\n"   \
  "instead cause a Specification Exception program interrupt.\n"                \
  "\n"                                                                          \
  "When ECHO is specified a message is issued to the hardware console panel\n"  \
  "when the command is about to be issued, when the command is redisplayed,\n"  \
  "and when the command has finished executing.  When NOECHO is specified\n"    \
  "no such audit trail messages are displayed.\n"                               \
  "\n"                                                                          \
  "NOTE: Enabling this feature has security consequences. When this feature\n"  \
  "is enabled it is possible for guest operating systems to issue commands\n"   \
  "directly to the host operating system via the 'sh' and 'exec' commands.\n"   \
  "Use the SHCMDOPT command's NODIAG8 option to disable this ability.\n"

#define ds_cmd_desc             "Display subchannel"
#define ecps_cmd_desc           "Command deprecated - Use \"ECPSVM\""
#define ecps_cmd_help           \
                                \
  "This command is deprecated. Use \"ecpsvm\" instead.\n"

#define ecpsvm_cmd_desc         "ECPS:VM Commands"
#define ecpsvm_cmd_help         \
                                \
  "Format: \"ecpsvm\". This command invokes ECPS:VM Subcommands.\n"              \
  "Type \"ecpsvm help\" to see a list of available commands\n"

#define engines_cmd_desc        "Set or display ENGINES parameter"
#define evm_cmd_desc            "Command deprecated - Use \"ECPSVM\""
#define evm_cmd_help            \
                                \
  "This command is deprecated. Use \"ecpsvm\" instead.\n"

#if defined(HAVE_OBJECT_REXX) || defined(HAVE_REGINA_REXX)
#define exec_cmd_desc           "Execute a Rexx script"
#define exec_cmd_help           \
                                \
  "Format:    'exec [mode] scriptname [[args...][&&]]'\n"                           \
  "\n"                                                                              \
  "Where 'scriptname' is the name of the Rexx script, 'args' is an optional\n"      \
  "list of arguments to be passed to the script and '&&' as the last argument\n"    \
  "requests the script to be run asynchronously in the background. The 'rexx'\n"    \
  "command can be used to list/cancel currently running asynchronous scripts.\n"    \
  "\n"                                                                              \
  "The argument passing style is determined by the 'rexx' command's current\n"      \
  "'Mode' setting. You can override it for the current execution by specifying\n"   \
  "an optional 'mode' parameter on command itself, just before the scriptname:\n"   \
  "'exec cmd script' for command style argument passing or 'exec sub script'\n"     \
  "for subroutine style argument passing.\n"                                        \
  "\n"                                                                              \
  "TAKE SPECIAL CARE when using the '&&' option to run a script asynchronously.\n"  \
  "Be careful to NOT accidentally enter a single '&' instead which invokes the\n"   \
  "Hercules 'exec' command asynchronously, but NOT the rexx script, leaving you\n"  \
  "with no way to cancel it. Always use two ampersands '&&' to cause the script\n"  \
  "itself to run in the background. Of course, if the script ends quickly then\n"   \
  "there is no need to run it asynchronously in the background. The ability to\n"   \
  "run scripts in the background was meant for never-ending 'monitoring' type\n"    \
  "scripts that monitor and report such things as Hercules status."

#endif /* defined(HAVE_OBJECT_REXX) || defined(HAVE_REGINA_REXX) */

#define exit_cmd_desc           "(Synonym for 'quit')"
#define ext_cmd_desc            "Generate external interrupt"

#define fquest_cmd_desc         "Query unusable page frame range(s)"
#define f_cmd_desc              "Mark page frame(s) as +usable/-unusable"
#define f_cmd_help              \
                                \
  "Format: \"f{+/-} addr[.len]\" or \"f{+/-} addr[-addr2]\" to mark an area\n"  \
  "of storage as being either +usable or -unusable where 'addr' is absolute\n"  \
  "address of the range of storage to be modified. Guest operating systems\n"   \
  "can then use the B22C 'TB' (Test Block) instruction to determine whether\n"  \
  "a given page is usable or not and react accordingly. Note that Hercules\n"   \
  "does not prevent unusable frames from being used anyway. That is to say\n"   \
  "frames marked as unusable can still be accessed normally without error.\n"   \
  "Use \"f?\" to display the currently defined -unusable storage range(s).\n"

#define facility_cmd_desc       "Enable/Disable/Query z/Arch STFLE Facility bits"
#define facility_cmd_help       \
                                \
  "Format: FACILITY  ENABLE | DISABLE   <facility> | bit   [ archlvl ]\n"        \
  "        FACILITY  QUERY  SHORT | LONG | RAW\n"                                \
  "        FACILITY  QUERY  <facility> | bit | ALL\n"                            \
  "        FACILITY  QUERY  ENABLED | DISABLED   [ LONG ]\n"                     \
  "\n"                                                                           \
  "'facility' is the SHORT facility name to be enabled, disabled or queried.\n"  \
  "The facility may also be specified by explicit bit number or via 'BITnnn'.\n" \
  "ALL is a synonym for SHORT. RAW displays the hex string. ENABLED displays\n"  \
  "only facilities which are enabled. DISABLED shows only disabled failities.\n" \
  "LONG sorts the display by Long Description. SHORT is the default.\n"

#define fcb_cmd_desc            "Display a printer's current FCB"
#define fcb_cmd_help            "Format: \"fcb <devnum>\""
#define fpc_cmd_desc            "Display or alter floating point control register"
#define fpc_cmd_help            \
                                \
  "Format: \"fpc [xxxxxxxxxxxxxxxx]\" where 'xxxxxxxxxxxxxxxx' is the\n"        \
  "register value in hexadecimal (1-8 hex digits). Enter \"fpc\" by itself\n"   \
  "to display the register value without altering it.\n"

#define fpr_cmd_desc            "Display or alter floating point registers"
#define fpr_cmd_help            \
                                \
  "Format: \"fpr [nn=xxxxxxxxxxxxxxxx]\" where 'nn' is the register number\n"   \
  "(0 to 15 or 0, 2, 4 or 6 depending on the Control Register 0 AFP bit) and\n" \
  "'xxxxxxxxxxxxxxxx' is the register value in hexadecimal (1-16 hex digits\n"  \
  "for 64-bit registers). Enter \"fpr\" by itself to display the register\n"    \
  "values without altering them.\n"

#define g_cmd_desc              "Turn off instruction stepping and start all CPUs"
#define gpr_cmd_desc            "Display or alter general purpose registers"
#define gpr_cmd_help            \
                                \
  "Format: \"gpr [nn=xxxxxxxxxxxxxxxx]\" where 'nn' is the optional\n"          \
  "register number (0 to 15) and 'xxxxxxxxxxxxxxxx' is the register\n"          \
  "value in hexadecimal (1-8 hex digits for 32-bit registers or 1-16 hex\n"     \
  "digits for 64-bit registers). Enter \"gpr\" by itself to display the\n"      \
  "register values without altering them.\n"

#define hao_cmd_desc            "Hercules Automatic Operator"
#define hao_cmd_help            \
                                \
  "Format: \"hao  tgt <tgt> | cmd <cmd> | list <n> | del <n> | clear \".\n"     \
  "  hao tgt <tgt> : define target rule (regex pattern) to react on\n"          \
  "  hao cmd <cmd> : define command for previously defined rule\n"              \
  "  hao list <n>  : list all rules/commands or only at index <n>\n"            \
  "  hao del <n>   : delete the rule at index <n>\n"                            \
  "  hao clear     : delete all rules (stops automatic operator)\n"

#define help_cmd_desc           "list all commands / command specific help"
#define help_cmd_help           \
                                \
  "Format: \"help [cmd|c*]\".\n"                                                \
  "\n"                                                                          \
  "The command without any options will display a short description\n"          \
  "of all of the commands available matching the current cmdlevel. You\n"       \
  "may specify a partial command name followed by an '*' to get a\n"            \
  "list matching the partial command name. For example 'help msg*'\n"           \
  "will list all commands beginning with 'msg' and matching the current\n"      \
  "cmdlevel.\n"                                                                 \
  "\n"                                                                          \
  "This command with the 'cmd' option will display a long form of help\n"       \
  "information associated with that command if the command is available\n"      \
  "for the current cmdlevel.\n"                                                 \
  "\n"                                                                          \
  "Help text may be limited to explaining the general format of the\n"          \
  "command and its various required or optional parameters and is not\n"        \
  "meant to replace the appropriate manual.\n"

#define herclogo_cmd_desc       "Read a new hercules logo file"
#define herclogo_cmd_help       \
                                \
  "Format: \"herclogo [<filename>]\". Load a new logo file for 3270\n"          \
  "terminal sessions. If no filename is specified, the built-in logo\n"         \
  "is used instead.\n"

#define xxxprio_cmd_desc        "(deprecated)"
#define xxxprio_cmd_help        \
                                \
  "This command is no longer supported and and will be removed in the future.\n"

#define hst_cmd_desc            "History of commands"
#define hst_cmd_help            \
                                \
  "Format: \"hst | hst n | hst l\". Command \"hst l\" or \"hst 0\" displays\n"  \
  "list of last ten commands entered from command line\n"                       \
  "hst n, where n is a positive number retrieves n-th command from list\n"      \
  "hst n, where n is a negative number retrieves n-th last command\n"           \
  "hst without an argument works exactly as hst -1, it retrieves the\n"         \
  "last command\n"

#define http_cmd_desc           "Start/Stop/Modify/Display HTTP Server"
#define http_cmd_help           \
                                \
  "Format: 'http [start|stop|port nnnn [[noauth]|[auth user pass]]|root path]'\n"   \
  "\n"                                                                              \
  "start                                 - starts HTTP server if stopped\n"         \
  "stop                                  - stops HTTP server if started\n"          \
  "port nnnn [[noauth]|[auth user pass]] - set port and optional authorization.\n"  \
  "                                        Default is noauthorization needed.\n"    \
  "                                        'auth' requires a user and password\n"   \
  "\n"                                                                              \
  "root path                             - set the root file path name\n"           \
  "\n"                                                                              \
  "<none>                                - display status of HTTP server\n"

#define i_cmd_desc              "Generate I/O attention interrupt for device"

#define icount_cmd_desc         "Display individual instruction counts"
#define icount_cmd_help         \
                                \
  "Format: \"icount [[Enable|STArt] | [Disable|STOp] | [Clear|Reset|Zero]]\".\n" \
  "\n"                                                                          \
  "Enables or disables the counting of, resets the counts for, or\n"            \
  "displays how often each instruction opcode is executed. This is a\n"         \
  "debugging command used to determine which instruction opcodes are\n"         \
  "executed the most frequently for a given workload to help determine\n"       \
  "which instructions are the best candidates for further optimization.\n"      \
  "\n"                                                                          \
  "The ENABLE and DISABLE options start or stop instruction counting.\n"        \
  "The CLEAR option resets all of the counts back to zero. Use it just\n"       \
  "before enabling counting when your workload begins. Use the stop\n"          \
  "option when your workload ends to stop counting. Enter the command\n"        \
  "with no options to display a list of executed instruction opcodes\n"         \
  "sorted by frequency/popularity.\n"

#define iodelay_cmd_desc        "Display or set I/O delay value"
#define iodelay_cmd_help        \
                                \
  "Format:  \"iodelay  n\".\n\n"                                                \
  "Specifies the amount of time (in microseconds) to wait after an\n"           \
  "I/O interrupt is ready to be set pending. This value can also be\n"          \
  "set using the Hercules console. The purpose of this parameter is\n"          \
  "to bypass a bug in the Linux/390 and zLinux dasd.c device driver.\n"         \
  "The problem is more apt to happen under Hercules than on a real\n"           \
  "machine because we may present an I/O interrupt sooner than a\n"             \
  "real machine.\n"

#define ipending_cmd_desc       "Display pending interrupts"
#define ipl_cmd_desc            "IPL from device or file"
#define ipl_cmd_help            \
                                \
  "Format: \"IPL {xxxx | cccc} [CLEAR] [LOADPARM xxxxxxxx] [PARM xxx [xxx ...]]\""\
  "\n\n"                                                                         \
  "Performs the Initial Program Load manual control function. If the first\n"    \
  "operand 'xxxx' is a valid device number then a CCW-type IPL is initiated\n"   \
  "from the specified device and SCLP disk I/O is disabled.  Otherwise a \n"     \
  "list-directed IPL is performed from the .ins file named 'cccc', and SCLP\n"   \
  "disk I/O is enabled for the directory path where the .ins file is located.\n" \
  "\n"                                                                           \
  "The optional 'CLEAR' keyword will initiate a Load Clear manual control\n"     \
  "function prior to starting an IPL.\n"                                         \
  "\n"                                                                           \
  "The optional 'LOADPARM' keyword followed by a 1-8 character string can be\n"  \
  "used to override the default value defined by the 'LOADPARM' command.\n"      \
  "\n"                                                                           \
  "An optional 'PARM' keyword followed by string data can also be used to\n"     \
  "pass data to the IPL command processor. If specified the string data is\n"    \
  "loaded into the low-order 32 bits of General Purpose registers R0 - R15\n"    \
  "(4 characters per register for a maximum of 64 bytes total; any excess\n"     \
  "is ignored). The PARM option behaves similarly to the VM IPL command.\n"      \
  "\n"                                                                           \
  "PLEASE NOTE that because the 'PARM' option supports multiple arguments,\n"    \
  "if specified, it MUST be the LAST option on the command line since ALL\n"     \
  "remaining command line arguments following the 'PARM' keyword will be\n"      \
  "treated as being part of a single blank-separated parameter string.\n"

#define iplc_cmd_desc           "Command deprecated - use IPL with clear option"
#define iplc_cmd_help           \
                                \
  "This command is deprecated. Use \"ipl\" with clear option specified instead.\n"

#define k_cmd_desc              "Display cckd internal trace"
#define kd_cmd_desc             "Short form of 'msghld clear'"
#define ldmod_cmd_desc          "Load a module"
#define ldmod_cmd_help          \
                                \
  "Format: \"ldmod module ...\"\n"                                              \
  "Specifies additional modules that are to be loaded by the\n"                 \
  "Hercules dynamic loader.\n"

#define legacy_cmd_desc         "Set legacysenseid setting"
#define loadcore_cmd_desc       "Load a core image file"
#define loadcore_cmd_help       \
                                \
  "Format: \"loadcore filename [address]\" where 'address' is the storage\n"    \
  "address of where to begin loading memory. The file 'filename' is\n"          \
  "presumed to be a pure binary image file previously created via the\n"        \
  "'savecore' command. The default for 'address' is 0 (beginning of\n"          \
  "storage).\n"

#define loadparm_cmd_desc       "Set the default IPL 'LOADPARM' parameter"
#define loadparm_cmd_help       \
                                \
  "Specifies the default eight-character IPL 'LOADPARM' parameter used by\n"    \
  "some operating systems to select certain initialization options. The\n"      \
  "value specified here can be overridden by specifying a different value\n"   \
  "on the the IPL command itself. The LOADPARM command simply defines the\n"    \
  "default value that is used if not overridden on the IPL command itself.\n"

#define loadtext_cmd_desc       "Load a text deck file"
#define loadtext_cmd_help       \
                                \
  "Format: \"loadtext filename [address]\". This command is essentially\n"      \
  "identical to the 'loadcore' command except that it loads a text deck\n"      \
  "file with \"TXT\" and \"END\" 80 byte records (i.e. an object deck).\n"

#define locks_cmd_desc          "Display internal locks list"
#define locks_cmd_help          \
                                \
  "Format: \"locks [ALL|HELD|tid] [SORT NAME|{TID|OWNER}|{WHEN|TIME|TOD}|{WHERE|LOC}]\"\n"

#define threads_cmd_desc        "Display internal threads list"
#define threads_cmd_help        \
                                \
  "Format:  \"threads [ALL|WAITING|tid] [SORT NAME|TID|{WHEN|TIME|TOD}|{WHERE|LOC}]\"\n"

#define log_cmd_desc            "Direct logger output"
#define log_cmd_help            \
                                \
  "Format: \"log [ OFF | newfile ]\".   Sets log filename or stops\n"           \
  "log file output with the \"OFF\" option."

#define logopt_cmd_desc         "Set/Display logging options"
#define logopt_cmd_help         \
                                \
  "Format: \"LOGOPT [DATESTAMP | NODATESTAMP] [TIMESTAMP | NOTIMESTAMP]\".\n\n" \
  "Sets logfile options. \"TIMESTAMP\" inserts a time stamp in front of\n"     \
  "each log message. \"NOTIMESTAMP\" logs messages without time stamps.\n"      \
  "Similarly, \"DATESTAMP\" and \"NODATESTAMP\" prefixes logfile messages\n"    \
  "with or without the current date. Entering the command with no arguments\n"  \
  "displays current logging options. The current resolution of the stamp\n"     \
  "is one second.\n"

#define lparname_cmd_desc       "Set LPAR name"
#define lparname_cmd_help       \
                                \
  "Specifies the eight-character LPAR name returned by\n"                       \
  "DIAG X'204'. The default is HERCULES"

#define lparnum_cmd_desc        "Set LPAR identification number"
#define lparnum_cmd_help        \
                                \
   "Specifies the one- or two-digit hexadecimal LPAR identification\n"          \
   "number stored by the STIDP instruction, or BASIC. If a one-digit\n"         \
   "hexadecimal number from 1 to F is specified, then STIDP stores a\n"         \
   "format-0 CPU ID. If a two-digit hexadecimal number is specified,\n"         \
   "except 10, then STIDP stores a format-1 CPU ID. For LPARNUM 10, \n"         \
   "STIDP uses the current CPUIDFMT setting. If LPARNUM is BASIC, then\n"       \
   "STIDP stores a basic-mode CPU ID. The default LPAR identification\n"        \
   "number is 1.\n"

#define lsdep_cmd_desc          "List module dependencies"
#define lsequ_cmd_desc          "List device equates"
#define lsmod_cmd_desc          "List dynamic modules"
#define lsmod_cmd_help          \
                                \
  "Format:  lsmod  [ALL]\n"                                                     \
  "\n"                                                                          \
  "Lists all dynamically loaded modules and their registered symbols,\n"        \
  "device-types and instruction overrides. If 'ALL' is specified then\n"        \
  "registered symbols which are currently unresolved are also listed.\n"

#define mainsize_cmd_desc       "Define/Display mainsize parameter"
#define mainsize_cmd_help       \
                                \
  "Format: mainsize [ mmmm | nnnS [ lOCK | unlOCK ] ]\n"                        \
  "        mmmm    - define main storage size mmmm Megabytes\n"                 \
  "\n"                                                                          \
  "        nnnS    - define main storage size nnn S where S is the\n"           \
  "                  multipler:\n"                                              \
  "                  B = no multiplier\n"                                       \
  "                  K = 2**10 (kilo/kibi)\n"                                   \
  "                  M = 2**20 (mega/mebi)\n"                                   \
  "                  G = 2**30 (giga/gibi)\n"                                   \
  "                  T = 2**40 (tera/tebi)\n"                                   \
  "                  P = 2**50 (peta/pebi)\n"                                   \
  "                  E = 2**60 (exa/exbi)\n"                                    \
  "\n"                                                                          \
  "        lOCK    - attempt to lock storage (pages lock by host OS)\n"         \
  "        unlOCK  - leave storage unlocked (pagable by host OS)\n"             \
  "\n"                                                                          \
  "      (none)    - display current mainsize value\n"                          \
  "\n"                                                                          \
  " Note: Multipliers 'T', 'P', and 'E' are not available on 32bit machines\n"

#define manuf_cmd_desc          "Set STSI manufacturer code"
#define maxcpu_cmd_desc         "Set maxcpu parameter"
#define maxrates_cmd_desc       "Display highest MIPS/SIOS rate or set interval"
#define maxrates_cmd_help       \
                                \
  "Format: \"maxrates [nnnn]\" where 'nnnn' is the desired reporting\n"         \
  "interval in minutes or 'midnight'. Acceptable values are from\n"             \
  "1 to 1440. The default is 1440 minutes (one day).\n"                         \
  "The interval 'midnight' sets the interval to 1440 and aligns the\n"          \
  "start of the current interval to midnight.\n"                                \
  "Entering \"maxrates\" by itself displays the current highest\n"              \
  "rates observed during the defined intervals.\n"

#define message_cmd_desc        "Display message on console a la VM"
#define message_cmd_help        \
                                \
  "Format: \"message * text\". The 'text' field is variable in size.\n"         \
  "A 'VM' message similar to: \"13:02:41 * MSG FROM HERCULES: hello\" is\n"     \
  "diplayed on the console panel as a result of the panel command\n"            \
  "'message * hello'.  (See also the \"msgnoh\" command)\n"

#define model_cmd_desc          "Set/Query STSI model code"
#define model_cmd_help          \
                                \
  "Format:\n"                                                                   \
  "\n"                                                                          \
  "     model [hardware [capacity [permanent [temporary]]]]\n"                  \
  "\n"                                                                          \
  "where:\n"                                                                    \
  "\n"                                                                          \
  "<null>       specifies a query of the current model code settings.\n"        \
  "\n"                                                                          \
  "hardware     specifies the hardware model setting. Specifying an \"=\"\n"    \
  "             resets the hardware model to \"EMULATOR\"; specifying an\n"     \
  "             \"*\" leaves the current hardware model setting intact.\n"      \
  "             Valid characters are 0-9 and uppercase A-Z only.\n"             \
  "             The default hardware model is \"EMULATOR\".\n"                  \
  "\n"                                                                          \
  "capacity     specifies the capacity model setting. Specifying an \"=\"\n"    \
  "             copies the current hardware model; specifying an \"*\" \n"      \
  "             leaves the current capacity model setting intact.\n"            \
  "             Valid characters are 0-9 and uppercase A-Z only.\n"             \
  "             The default capacity model is \"EMULATOR\".\n"                  \
  "\n"                                                                          \
  "permanent    specifies the permanent model setting. Specifying an\n"         \
  "             \"=\" copies the current capacity model; specifying an\n"       \
  "             \"*\" leaves the current permanent model setting intact.\n"     \
  "             Valid characters are 0-9 and uppercase A-Z only.\n"             \
  "             The default permanent model is \"\" (null string).\n"           \
  "\n"                                                                          \
  "temporary    specifies the temporary model setting. Specifying an\n"         \
  "             \"=\" copies the current permanent model; specifying an\n"      \
  "             \"*\" leaves the current temporary model setting intact.\n"     \
  "             Valid characters are 0-9 and uppercase A-Z only.\n"             \
  "             The default temporary model is \"\" (null string).\n"

#define modpath_cmd_desc        "Set module load path"
#define modpath_cmd_help        \
                                \
  "Format:      MODPATH   <path>\n"                                             \
  "\n"                                                                          \
  "Where <path> specifies the relative or absolute path of the directory\n"     \
  "where dynamic modules should be loaded from. Only one directory may be\n"    \
  "specified. Enclose the path within double quotes if it contains blanks.\n"

#define mtapeinit_cmd_desc      "Control tape initialization"
#define mtapeinit_cmd_help      \
                                \
  "Format: \"mounted_tape_reinit [disallow|disable | allow|enable]\"\n"         \
  "Specifies whether reinitialization of tape drive devices\n"                  \
  "(via the devinit command, in order to mount a new tape)\n"                   \
  "should be allowed if there is already a tape mounted on\n"                   \
  "the drive. The current value is displayed if no operand is\n"                \
  "specified\n"                                                                 \
  "Specifying ALLOW or ENABLE indicates new tapes may be\n"                     \
  "mounted (via 'devinit nnnn new-tape-filename') irrespective\n"               \
  "of whether or not there is already a tape mounted on the drive.\n"           \
  "This is the default state.\n"                                                \
  "Specifying DISALLOW or DISABLE prevents new tapes from being\n"              \
  "mounted if one is already mounted. When DISALLOW or DISABLE has\n"           \
  "been specified and a tape is already mounted on the drive, it\n"             \
  "must first be unmounted (via the command 'devinit nnnn *') before\n"         \
  "the new tape can be mounted. Otherwise the devinit attempt to\n"             \
  "mount the new tape is rejected.\n"

#define msg_cmd_desc            "Alias for message"
#define msghld_cmd_desc         "Display or set the timeout of held messages"
#define msghld_cmd_help         \
                                \
  "Format: \"msghld [value | info | clear]\".\n"                                \
  "value: timeout value of held message in seconds\n"                           \
  "info:  displays the timeout value\n"                                         \
  "clear: releases the held messages\n"

#define msglevel_cmd_desc       "Display/Set current Message Display output"
#define msglevel_cmd_help       \
                                \
  "Format:  msglevel  [verbose|terse|debug|nodebug|emsgloc|noemsgloc]\n"        \
  "\n"                                                                          \
  "   verbose     Display messages during configuration file processing\n"      \
  "   terse       Do not display configuration file processing messages\n"      \
  "   debug       Prefix messages with filename and line number\n"              \
  "   nodebug     Display messages normally\n"                                  \
  "   emsgloc     Show where error messages originated\n"                       \
  "   noemsgloc   Do not show where error messages originated\n"

#define msglvl_cmd_desc         "Alias for msglevel"
#define msgnoh_cmd_desc         "Similar to \"message\" but no header"
#define mt_cmd_desc             "Control magnetic tape operation"
#define mtc_cmd_desc            "Alias for mt command"
#define mt_cmd_help             \
                                \
  "Format:     \"mt device operation [ 1-9999 ]\".\n"                           \
  "  Operations below can be used on a valid tape device. The device\n"         \
  "  must not have any I/O operation in process or pending.\n"                  \
  "     operation   description\n"                                              \
  "       rew       rewind tape to the beginning\n"                             \
  "       asf n     position tape at 'n' file  (default = 1)\n"                 \
  "       fsf n     forward space 'n' files    (default = 1)\n"                 \
  "       bsf n     backward space 'n' files   (default = 1)\n"                 \
  "       fsr n     forward space 'n' records  (default = 1)\n"                 \
  "       bsr n     backward space 'n' records (default = 1)\n"                 \
  "       wtm n     write 'n' tapemarks        (default = 1)\n"                 \
  "       dse       data secure erase\n"                                        \
  "       dvol1     display VOL1 header\n"

#define netdev_cmd_desc         "Set default host networking device"
#define netdev_cmd_help         \
                                \
  "Specifies the name (or for Windows, the IP or MAC address) of the\n"         \
  "underlying default host network device to be used for all Hercules\n"        \
  "communications devices unless overridden on the device statement.\n"         \
  "\n"                                                                          \
  "The default for Linux (except Apple and FreeBSD) is '/dev/net/tun'.\n"       \
  "The default for Apple and FreeBSD is '/dev/tun'.\n"                          \
  "\n"                                                                          \
  "The default for Windows is whatever SoftDevLabs's CTCI-WIN product\n"        \
  "returns as its default CTCI-WIN host network adapter, which for older\n"     \
  "versions of CTCI-WIN (3.5.0) is the first network adapter returned by\n"     \
  "Windows in its adapter binding order or for newer versions of CTCI-WIN\n"    \
  "(3.6.0) what you defined as your default CTCI-WIN host network adapter.\n"

#define numcpu_cmd_desc         "Set numcpu parameter"
//#define numvec_cmd_desc         "Set numvec parameter"
#define osa_cmd_desc            "(Synonym for 'qeth')"
#define ostailor_cmd_desc       "Tailor trace information for specific OS"
#define ostailor_cmd_help       \
                                \
  "Format: \"ostailor [quiet|os/390|z/os|vm|vse|z/vse|linux|opensolaris|null]\".\n"  \
  "Specifies the intended operating system. The effect is to reduce\n"          \
  "control panel message traffic by selectively suppressing program\n"          \
  "check trace messages which are considered normal in the specified\n"         \
  "environment. The option 'quiet' suppresses all exception messages,\n"        \
  "whereas 'null' suppresses none of them. The other options suppress\n"        \
  "some messages and not others depending on the specified o/s. Prefix\n"       \
  "values with '+' to combine them with existing values or '-' to exclude\n"    \
  "them. SEE ALSO the 'pgmtrace' command which allows you to further fine\n"    \
  "tune the tracing of program interrupt exceptions.\n"

#define panopt_cmd_desc         "Set or display panel options"
#define panopt_cmd_help         \
                                \
  "Format:\n"                                                                       \
  "\n"                                                                              \
  "  panopt [FULLpath|NAMEonly] [RATE=n] [MSGCOLOR=NO|DARK|LIGHT] [TITLE=xxx]\n"    \
  "\n"                                                                              \
  "NAMEONLY requests the extended panel screen (that displays the list of\n"        \
  "devices and is reached by pressing the ESC key) to display the emulated\n"       \
  "device's base filename only, whereas FULLPATH (the default) displays the\n"      \
  "file's full path filename.\n"                                                    \
  "\n"                                                                              \
  "RATE=nnn sets the panel refresh rate to nnn milliseconds. RATE=FAST sets\n"      \
  "the refresh rate to " QSTR(PANEL_REFRESH_RATE_FAST) " milliseconds. RATE=SLOW sets the refresh rate to\n" \
  QSTR(PANEL_REFRESH_RATE_SLOW) " milliseconds.\n"                                  \
  "\n"                                                                              \
  "MSGCOLOR=DARK displays colorized panel messages meant for dark colored\n"        \
  "panels (e.g. white text on black background) whereas MSGCOLOR=LIGHT is\n"        \
  "meant for light colored panels (e.g. black text on white background).\n"         \
  "\n"                                                                              \
  "TITLE=xxx sets an optional console window title-bar string to be used in\n"      \
  "place of the default supplied by the windowing system. The entire TITLE=\n"      \
  "argument should be enclosed within double quotes if it contains any blanks\n"    \
  "(e.g. use \"TITLE=my title\" and not TITLE=\"my title\" which is an error).\n"   \
  "An empty string (\"TITLE=\") will remove the existing console title. The\n"      \
  "default console title is the string consisting of:\n"                            \
  "\n"                                                                              \
  "     LPARNAME - SYSTYPE * SYSNAME * SYSPLEX - System Status: color\n"            \
  "\n"                                                                              \
  "SYSTYPE, SYSNAME, and SYSPLEX are populated by the system call SCLP Control\n"   \
  "Program Identification. If a value is blank then that field is not shown.\n"     \
  "\n"                                                                              \
  "System Status colors are:\n"                                                     \
  "\n"                                                                              \
  "     GREEN       everything is working correctly\n"                              \
  "     YELLOW      one or more CPUs are not running\n"                             \
  "     RED         one or more CPUs are in a disabled wait state\n"                \
  "\n"                                                                              \
  "Enter PANOPT without any arguments at all to display the current settings.\n"

#define panrate_cmd_desc        "(deprecated; use PANOPT RATE=nnn instead)"
#define pantitle_cmd_desc       "(deprecated; use PANOPT TITLE=xxx instead)"

#define pgmprdos_cmd_desc       "Set LPP license setting"
#define pgmprdos_cmd_help       \
                                \
  "Format: \"pgmprdos restricted | licensed\"\n\n"                               \
  "Note: It is YOUR responsibility to comply with the terms of the license for\n"\
  "      the operating system you intend to run on Hercules. If you specify\n"   \
  "      LICENSED and run a licensed operating system in violation of that\n"    \
  "      license, then don't come after the Hercules developers when the vendor\n"\
  "      sends their lawyers after you.\n"


#define pgmtrace_cmd_desc       "Trace program interrupts"
#define pgmtrace_cmd_help       \
                                \
  "Format: \"pgmtrace [-]intcode\" where 'intcode' is any valid program\n"      \
  "interruption code in the range 0x01 to 0x40. Precede the interrupt\n"        \
  "code with a '-' to stop tracing of that particular program\n"                \
  "interruption.\n"

#define plant_cmd_desc          "Set STSI plant code"
#define pr_cmd_desc             "Display or alter prefix register"
#define pr_cmd_help             \
                                \
  "Format: \"pr [value]\" where the optional 'value' operand is the new\n"      \
  "prefix register value for the current CPU. Enter just 'pr' by itself\n"      \
  "to display the current value. Use the'cpu' command beforehand to choose\n"   \
  "which processor's prefix register should be displayed or altered.\n"

#define psw_cmd_desc            "Display or alter program status word"
#define psw_cmd_help            \
                                \
  "Format: \"psw [operand ...]\" where 'operand ...' is one or more\n"          \
  "optional parameters which modify the contents of the Program Status\n"       \
  "Word:\n\n"                                                                   \
  "  am=24|31|64           addressing mode\n"                                   \
  "  as=ar|home|pri|sec    address-space\n"                                     \
  "  cc=n                  condition code       (decimal 0 to 3)\n"             \
  "  cmwp=x                C/M/W/P bits         (one hex digit)\n"              \
  "  ia=xxx                instruction address  (1 to 16 hex digits)\n"         \
  "  pk=n                  protection key       (decimal 0 to 15)\n"            \
  "  pm=x                  program mask         (one hex digit)\n"              \
  "  sm=xx                 system mask          (2 hex digits)\n"               \
  "\n"                                                                          \
  "Enter \"psw\" by itself to display the current PSW without altering it.\n"

#define ptp_cmd_desc            "Enable/Disable PTP debugging"
#define ptp_cmd_help            \
                                \
  "Format:  \"ptp  debug  { on | off } [ [ <devnum> | ALL ] [ mask ] ]\".\n\n"  \
  "Enables/disables debug tracing for the PTP device group\n"                   \
  "identified by <devnum>, or for all PTP device groups if\n"                   \
  "<devnum> is not specified or specified as 'ALL'.\n"

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
  #define ptt_cmd_help_txf "     (no)txf          trace Transactional-Execution Facility events\n"
#else
  #define ptt_cmd_help_txf
#endif

#define ptt_cmd_desc            "Activate or display internal trace table"
#define ptt_cmd_help            \
                                \
  "Format: \"ptt [?] [events] [options] [nnnnnn]\"\n"                               \
  "\n"                                                                              \
  "When specified with no operands, the ptt command displays the defined trace\n"   \
  "parameters and the contents of the internal trace table.\n"                      \
  "\n"                                                                              \
  "When specified with operands, the ptt command defines the trace parameters\n"    \
  "identifying which events are to be traced. When the last option is numeric,\n"   \
  "it defines the size of the trace table itself and activates tracing.\n"          \
  "\n"                                                                              \
  "Events:    (should be specified first, before any options are specified)\n"      \
  "\n"                                                                              \
  "     (no)log          trace internal logger events\n"                            \
  "     (no)tmr          trace internal timer events\n"                             \
  "     (no)thr          trace internal threading events\n"                         \
  "     (no)inf          trace instruction information events\n"                    \
  "     (no)err          trace instruction error events\n"                          \
  "     (no)pgm          trace program interrupt events\n"                          \
  "     (no)csf          trace interlocked instruction type events\n"               \
  "     (no)sie          trace SIE instruction events\n"                            \
  "     (no)sig          trace SIGP instruction events\n"                           \
  "     (no)io           trace I/O instruction events\n"                            \
  ptt_cmd_help_txf                                                                  \
  "     (no)lcs1         trace LCS timing events\n"                                 \
  "     (no)lcs2         trace LCS general debugging events\n"                      \
  "     (no)qeth         trace QETH general debugging events\n"                     \
  "     (no)xxx          trace undefined/generic/custom events\n"                   \
  "\n"                                                                              \
  "options:   (should be specified last, after any events are specified)\n"         \
  "\n"                                                                              \
  "     ?                show currently defined trace parameters\n"                 \
  "     (no)lock         lock table before updating\n"                              \
  "     (no)tod          timestamp table entries\n"                                 \
  "     (no)wrap         wraparound trace table\n"                                  \
  "     (no)dtax         dump table at exit\n"                                      \
  "     to=nnn           automatic display timeout  (number of seconds)\n"          \
  "     nnnnnn           table size                 (number of entries)\n"

#define qcpuid_cmd_desc         "Display cpuid(s)"
#define qcpuid_cmd_help         \
                                \
  "Format:   \"qcpuid   [xx|ALL]\"\n\n"                                         \
  "Displays the cpuid and STSI results presented to the SCP for either\n"       \
  "the current panel command target cpu (as defined by the \"cpu\" panel\n"     \
  "command) if no arguments are given, or the specified hexadecimal cpu\n"      \
  "address or all cpus if an argument was given.\n"

#define qd_cmd_desc             "Query device information"
#define qd_cmd_help             \
                                \
  "Format: \"qd [devnum(s)] | [devclass]\" where 'devnum(s)' is either\n"        \
  "a single device number or a multiple device number specification\n"           \
  "in the same format as used for configuration file device statements,\n"       \
  "and 'devclass' is either CHAN, CON, CTCA, DASD, DSP, FCP, LINE, OSA,\n"       \
  "PCH, PRT, RDR, or TAPE. When no argument is given all devices are shown.\n"

#define qeth_cmd_desc           "Enable/Disable QETH debugging"
#define qeth_cmd_help           \
                                \
  "Format:  \"QETH  DEBUG {ON|OFF}  [ [<devnum>|ALL] [mask ...] ]\"\n"          \
  "         \"QETH  ADDR              [<devnum>|ALL]\"\n\n"                     \
  "Enables/disables debug tracing for the QETH (OSA) device groups iden-\n"     \
  "tified by <devnum>, or for all QETH (OSA) device groups if <devnum> is\n"    \
  "not specified or specified as 'ALL', or displays all MAC addresses\n"        \
  "registered with the device identified by <devnum> or for all QETH (OSA)\n"   \
  "device groups if <devnum> is not specified or specified as 'ALL'.  The\n"    \
  "optional 'mask' value may be specified more than once. Mask values are\n"    \
  "'Ccw', 'DAta', 'DRopped', 'Expand', 'Interupts', 'Packet', 'Queues',\n"      \
  "'SBale', 'SIga', 'Updown' or 0xhhhhhhhh hexadecimal value.\n"

#define qpfkeys_cmd_desc        "Display the current PF Key settings"
#define qpid_cmd_desc           "Display Process ID of Hercules"
#define qports_cmd_desc         "Display TCP/IP ports in use"
#define qproc_cmd_desc          "Display processors type and utilization"
#define qstor_cmd_desc          "Display main and expanded storage values"
#define quiet_cmd_desc          "Toggle automatic refresh of panel display data"
#define quiet_cmd_help          \
                                \
  "'quiet' either disables automatic screen refreshing if it is\n"              \
  "currently enabled or enables it if it is currently disabled.\n"              \
  "When disabled you will no be able to see the response of any\n"              \
  "entered commands nor any messages issued by the system nor be\n"             \
  "able to scroll the display, etc. Basically all screen updating\n"            \
  "is disabled. Entering 'quiet' again re-enables screen updating.\n"

#define quit_cmd_desc          "Terminate the emulator"
#define quit_cmd_help           \
                                \
  "Format: \"quit [force]\"  Terminates the emulator. If the guest OS\n"        \
  "                        has enabled Signal Shutdown, then a\n"               \
  "                        signal shutdown request is sent to the\n"            \
  "                        guest OS and termination will begin\n"               \
  "                        after guest OS has shutdown.\n"                      \
  "              force     This option will terminate the emulator\n"           \
  "                        immediately.\n"

#define quitmout_cmd_desc      "Define maximum guest quiesce time"
#define quitmout_cmd_help       \
                                \
  "Format: \"quitmout [nnn]\" Defines the maximum amount of time in seconds\n"  \
  "you wish to wait for your guest to complete its quiesce function before\n"   \
  "proceeding with Hercules shutdown processing. If the guest completes its\n"  \
  "quiesce before this time period has expired Hercules proceeds immediately\n" \
  "with its normal shutdown processing. It does not wait for the time period\n" \
  "to expire.\n"                                                                \
  "\n"                                                                          \
  "If the guest fails to complete its quiesce functionality within the given\n" \
  "time period then Hercules does not wait any long and proceeds immediately\n" \
  "to performing a normal shutdown of the emulator.\n"                          \
  "\n"                                                                          \
  "Specify '0' to indicate you wish to always wait for the guest to complete\n" \
  "its quiesce functionality before shutting down the emulator no matter how\n" \
  "long it may take (i.e. wait forever for the guest to quiesce).\n"            \
  "\n"                                                                          \
  "Enter the command with no operand to display the current value.\n"           \
  "\n"                                                                          \
  "NOTE: setting a quit timeout value only makes sense if you are running a\n"  \
  "guest that supports the Signal Shutdown hypervisor signal. Not all guests\n" \
  "support such signals. zLinux and z/VM do, but z/OS for example does not.\n"

#define hwldr_cmd_desc          "Specify boot loader filename"
#define hwldr_cmd_help          \
                                \
  "Format: \"hwldr scsiboot [filename]\"  Specifies the bootstrap loader\n"     \
  "                                     to be used for FCP attached SCSI\n"     \
  "                                     devices.\n"

#define loaddev_cmd_desc        "Specify bootstrap loader IPL parameters"
#define loaddev_cmd_help        \
                                \
  "Format: \"loaddev [options]\"  Specifies optional parameters to be\n"        \
  "                             passed to be bootstrap loader.\n"               \
  "  Valid options are:\n\n"                          \
  "  \"portname [16 digit WWPN]\" Fibre Channel Portname of the FCP device\n"   \
  "  \"lun      [16 digit LUN]\"  Fibre Channel Logical Unit Number\n"          \
  "  \"bootprog [number]\"        The boot program number to be loaded\n"       \
  "  \"br_lba   [16 digit LBA]\"  Logical Block Address of the boot record\n"   \
  "  \"scpdata  [data]\"          Information to be passed to the OS\n"

#define dumpdev_cmd_desc        "Specify bootstrap loader DUMP parameters"
#define dumpdev_cmd_help        \
                                \
  "Format: \"dumpdev [options]\"  Specifies optional parameters to be\n"        \
  "                             passed to be bootstrap loader.\n"               \
  "  Valid options are:\n\n"                          \
  "  \"portname [16 digit WWPN]\" Fibre Channel Portname of the FCP device\n"   \
  "  \"lun      [16 digit LUN]\"  Fibre Channel Logical Unit Number\n"          \
  "  \"bootprog [number]\"        The boot program number to be loaded\n"       \
  "  \"br_lba   [16 digit LBA]\"  Logical Block Address of the boot record\n"   \
  "  \"scpdata  [data]\"          Information to be passed to the OS\n"

#define r_cmd_desc              "Display or alter real storage"
#define r_cmd_help              \
                                \
  "Format: \"r addr[.len]\" or \"r addr[-addr2]\" to display up to 64K\n"       \
  "of real storage, or \"r addr=value\" to alter up to 32 bytes of real\n"      \
  "storage, where 'value' is either a string of up to 32 pairs of hex digits,\n" \
  "or a string of up to 32 characters enclosed in single or double quotes.\n"

#define restart_cmd_desc        "Generate restart interrupt"
#define resume_cmd_desc         "Resume hercules"

#if defined(HAVE_OBJECT_REXX) || defined(HAVE_REGINA_REXX)
#define rexx_cmd_desc           "Modify/Display Hercules's Rexx settings"
#define rexx_cmd_help           \
                                \
  "Format:   'rexx [optname optvalue] ...'\n"                                   \
  "\n"                                                                          \
  "Ena[ble]/Sta[rt]     Enable/Start a Rexx Package, where package is\n"        \
  "                     either 'OORexx' (the default) or 'Regina'.\n"           \
  "                     Use the HREXX_PACKAGE environment variable\n"           \
  "                     to define your preferred default value. \"auto\"\n"     \
  "                     will automatically start the default package.\n"        \
  "                     Use \"none\" to prevent automatic enablement.\n"        \
  "Disa[ble]/Sto[p]     Disable/Stop the Rexx package.\n"                       \
  "\n"                                                                          \
  "RexxP[ath]/Path      List of directories to search for scripts.\n"           \
  "                     No default. Use the HREXX_PATH environment\n"           \
  "                     variable to define your preferred default.\n"           \
  "SysP[ath]            Extend the search to the System Paths too.\n"           \
  "                     'On' (default) or 'Off'.\n"                             \
  "Ext[ensions]         List of extensions to use when searching for\n"         \
  "                     scripts. A search with no extension is always\n"        \
  "                     done first. The HREXX_EXTENSIONS environment\n"         \
  "                     can be used to set a different default list.\n"         \
  "Suf[fixes]           Alias for 'Ext[ensions]'\n"                             \
  "Resolv[er]           'On' (default): Hercules will resolve the script's\n"   \
  "                     full path. 'Off': the script name is used as-is.\n"     \
  "MsgL[evel]           'Off' (default) or 'On' to disable or enable\n"         \
  "                     Hercules messages HHC17503I and HHC17504I\n"            \
  "                     that display a script's return code and value\n"        \
  "                     when it finishes executing.\n"                          \
  "MsgP[refix]          'Off' (default) or 'On' to disable or enable\n"         \
  "                     prefixing Rexx script 'say' messages with\n"            \
  "                     Hercules message number HHC17540I.\n"                   \
  "ErrP[refix]          'Off' (default) or 'On' to disable or enable\n"         \
  "                     prefixing Rexx script 'TRACE' messages with\n"          \
  "                     Hercules message number HHC17541D.\n"                   \
  "Mode                 Define the preferred argument passing style.\n"         \
  "                     'Com[mand]' (default) or 'Sub[routine]'. Use\n"         \
  "                     the HREXX_MODE environment variable to define\n"        \
  "                     your preferred default mode.\n"                         \
  "List                 Lists all scripts currently running asynchronously.\n"  \
  "Cancel               <tid> to halt an asynchronously running script.\n"      \
  "\n"                                                                          \
  "Setting any option to 'reset' will reset the option to its default value.\n" \
  "Entering the command without any arguments displays the current values.\n"
#endif /* defined(HAVE_OBJECT_REXX) || defined(HAVE_REGINA_REXX) */

#define rmmod_cmd_desc          "Delete a module"
#define sminus_cmd_desc         "Turn off instruction stepping"
#define s_cmd_desc              "Instruction stepping"
#define s_cmd_help              \
                                \
  "Format:\n"                                                                   \
  "\n"                                                                          \
  "     \"s [ addr-addr   [ asid ]]\"\n"                                        \
  "     \"s [ addr:addr   [ asid ]]\"\n"                                        \
  "     \"s [ addr.length [ asid ]]\"\n"                                        \
  "\n"                                                                          \
  "Sets the instruction stepping and instruction breaking range, which\n"       \
  "is totally separate from the instruction tracing range.\n"                   \
  "\n"                                                                          \
  "With or without a range, the s command displays whether instruction\n"       \
  "stepping is on or off, and the range and asid, if any.  Specifying an\n"     \
  "optional asid value causes stepping only for instructions executed by\n"     \
  "that specific address space; instructions executed for any other asid\n"     \
  "will not be stepped.\n"                                                      \
  "\n"                                                                          \
  "Note: the s command by itself does not activate instruction stepping.\n"     \
  "The s+ command must be used to activate instruction stepping.  Use the\n"    \
  "s 0 command to remove a range, causing all instructions to be stepped.\n"

#define squest_cmd_desc         "Query instruction stepping"
#define squest_cmd_help         \
                                \
  "Format: \"s?\" displays whether instruction stepping is on or off\n"         \
  "and the range if any.\n"

#define splus_cmd_desc          "Activate instruction stepping"
#define splus_cmd_help          \
                                \
  "Format:  \"s+ [ addr-addr [ asid ]]\".  Activates instruction stepping.\n"   \
  "\n"                                                                          \
  "A range can be specified as for the \"s\" command.  If a range is not\n"     \
  "specified then any previously defined range is used instead.  If there\n"    \
  "is no previously defined range (or the range was specified as 0) then\n"     \
  "the range includes all addresses.  When an instruction within the range\n"   \
  "is about to be executed, the CPU is temporarily stopped and the next\n"      \
  "instruction is displayed.  You may then examine registers and storage,\n"    \
  "etc, before pressing the <enter> key to execute the instruction and stop\n"  \
  "after the next one.  Use the \"g\" command to stop stepping and return\n"    \
  "to normal instruction processing instead.\n"

#define savecore_cmd_desc       "Save a core image to file"
#define savecore_cmd_help       \
                                \
  "Format: \"savecore filename [{start|*}] [{end|*}]\" where 'start' and\n"     \
  "'end' define the starting and ending addresss of the range of real\n"        \
  "storage to be saved to file 'filename'. An '*' for either the start\n"       \
  "address or end address (the default) means: \"the first/last byte of\n"      \
  "the first/last modified page as determined by the storage-key\n"             \
  "'changed' bit\".\n"

#define sclproot_cmd_desc       "Set SCLP base directory"
#define sclproot_cmd_help       \
                                \
  "Format: \"sclproot [path|NONE]\"\n"                                          \
  "Enables SCLP disk I/O for the specified directory path, or disables\n"       \
  "SCLP disk I/O if NONE is specified. A subsequent list-directed IPL\n"        \
  "resets the path to the location of the .ins file, and a CCW-type IPL\n"      \
  "disables SCLP disk I/O. If no operand is specified, sclproot displays\n"     \
  "the current setting.\n"

#define scpecho_cmd_desc        "Set/Display option to echo to console and history of scp replys"
#define scpecho_cmd_help        \
                                \
  "Format: \"scpecho [ on | off ]\"\n"                                          \
  "When scpecho is set ON, scp commands entered on the console are\n"           \
  "echoed to the console and recorded in the command history.\n"                \
  "The default is on. When scpecho is entered without any options,\n"           \
  "the current state is displayed. This is to help manage passwords\n"          \
  "sent to the scp from being displayed and journaled.\n"

#define scpimply_cmd_desc       "Set/Display option to pass non-hercules commands to the scp"
#define scpimply_cmd_help       \
                                \
  "Format: \"scpimply [ on | off ]\"\n"                                         \
  "When scpimply is set ON, non-hercules commands are passed to\n"              \
  "the scp if the scp has enabled receipt of scp commands. The\n"               \
  "default is off. When scpimply is entered without any options,\n"             \
  "the current state is displayed.\n"

#define script_cmd_desc         "Run a sequence of panel commands contained in a file"
#define script_cmd_help         \
                                \
  "Format: \"script [filename [filename] ...]\". Sequentially executes\n"       \
  "the commands contained within the file 'filename'. The script file\n"        \
  "may also contain \"script\" commands, but the system ensures that no\n"      \
  "more than " QSTR( MAX_SCRIPT_DEPTH ) " levels of script are invoked at any one time.\n\n"  \
                                                                                \
  "Enter the command with no arguments to list all running scripts.\n"

#define scsimount_cmd_desc      "Automatic SCSI tape mounts"
#define scsimount_cmd_help      \
                                \
  "Format:    \"scsimount [ no | yes | 0-99 ]\".\n"                             \
  "\n"                                                                          \
  "Displays or modifies the automatic SCSI tape mounts option.\n\n"             \
  "When entered without any operands, it displays the current interval\n"       \
  "and any pending tape mount requests. Entering 'no' (or 0 seconds)\n"         \
  "disables automount detection.\n"                                             \
  "\n"                                                                          \
  "Entering a value between 1-99 seconds (or 'yes') enables the option\n"       \
  "and specifies how often to query SCSI tape drives to automatically\n"        \
  "detect when a tape has been mounted (upon which an unsolicited\n"            \
  "device-attention interrupt will be presented to the guest operating\n"       \
  "system). 'yes' is equivalent to specifying a 5 second interval.\n"

#define sfminus_cmd_desc        "Delete shadow file"
#define sfminus_cmd_help        \
                                \
  "Format: \"sf- {*|dev} [MERGE|nomerge] [force]\".  Removes the active\n"      \
  "shadow file for the device, where dev is the device number (*=all cckd\n"    \
  "devices).\n"                                                                 \
                                                                          "\n"  \
  "If merge is specified (the default), then the contents of the current\n"     \
  "file is merged into the previous file, the current file is removed, and\n"   \
  "the previous file becomes the current file.  The previous file must be\n"    \
  "able to be opened read-write.  If nomerge is specified then the contents\n"  \
  "of the current shadow file is discarded and the previous file becomes the\n" \
  "new current file.  However, if the previous file is read-only then a new\n"  \
  "empty shadow file is then immediately re-added again and that new empty\n"   \
  "shadow file then becomes the new current file.  The force option is re-\n"   \
  "quired when doing a merge to the base file and the base file is read-only\n" \
  "because the 'ro' option was specified on the device config statement.\n"     \
                                                                          "\n"  \
  "Note that because it is possible for this command to take a long time to\n"  \
  "complete (when the default MERGE option is used) this command operates\n"    \
  "asynchronously in a separate worker thread.\n"

#define sfplus_cmd_desc         "Add shadow file"
#define sfplus_cmd_help         \
                                \
  "Format: \"sf+ {*|dev}\".  Creates another shadow file for the device\n"      \
  "where dev is the device number (*=all cckd devices).  Note that this\n"      \
  "command operates asynchronously in a separate worker thread.\n"

#define sfc_cmd_desc            "Compress shadow files"
#define sfc_cmd_help            \
                                \
  "Format: \"sfc {*|dev}\". Compresses the active device or shadow file\n"      \
  "where dev is the device number (*=all cckd devices). This command is\n"      \
  "essentially identical to the 'cckdcomp' utility.  Note that because it's\n"  \
  "possible for this command to take a long time to complete it operates\n"     \
  "asynchronously in a separate worker thread.\n"

#define sfd_cmd_desc            "Display shadow file stats"
#define sfd_cmd_help            \
                                \
  "Format: \"sfd {*|dev}\".  Displays shadow file status and statistics\n"      \
  "where dev is the device number (*=all cckd devices).  Note that this\n"      \
  "command operates asynchronously in a separate worker thread.\n"

#define sfk_cmd_desc            "Check shadow files"
#define sfk_cmd_help            \
                                \
  "Format: \"sfk {*|dev} [n]\". Performs a chkdsk on the active shadow file\n"  \
  "where dev is the device number (*=all cckd devices) and n is an optional\n"  \
  "check level (default is 2):\n"                                               \
                                                                          "\n"  \
  "   -1   devhdr, cdevhdr, l1 table.\n"                                        \
  "    0   devhdr, cdevhdr, l1 table, l2 tables.\n"                             \
  "    1   devhdr, cdevhdr, l1 table, l2 tables, free spaces.\n"                \
  "    2   devhdr, cdevhdr, l1 table, l2 tables, free spaces, trkhdrs.\n"       \
  "    3   devhdr, cdevhdr, l1 table, l2 tables, free spaces, trkimgs.\n"       \
  "    4   devhdr, cdevhdr. Build everything else from recovery.\n"             \
                                                                          "\n"  \
  "This command is essentially identical to the 'cckdcdsk' utility.\n"          \
  "You probably don't want to use '4' unless you have a backup and are\n"       \
  "prepared to wait a long time.  Note that because this command could\n"       \
  "take a long time to complete it operates asynchronously in a separate\n"     \
  "worker thread.\n"

#define sh_cmd_desc             "Shell command"
#if defined( _MSVC_ )
#define sh_cmd_help             \
                                \
  "Format: \"sh command [args...]\" where 'command' is any valid shell\n"       \
  "command or the special command 'startgui'. The entered command and any\n"    \
  "arguments are passed as-is to the shell for processing and the results\n"    \
  "are displayed on the Hercules console.\n"                                    \
  "\n"                                                                          \
  "The special startgui command MUST be used if the command being started\n"    \
  "either directly or indirectly starts a Windows graphical user interface\n"   \
  "(i.e. non-command-line) program such as notepad. Failure to use startgui\n"  \
  "in such cases will hang Hercules until you close/exit notepad. Note that\n"  \
  "starting a batch file which starts notepad still requires using startgui.\n" \
  "If 'foo.bat' does: \"start notepad\", then doing \"sh foo.bat\" will hang\n" \
  "Hercules until notepad exits just like doing \"sh start notepad\" will.\n"   \
  "Use startgui instead: \"sh startgui notepad\", \"sh startgui foo.bat\".\n"   \
  "\n"                                                                          \
  "For security reasons execution of shell commands are disabled by default.\n" \
  "Enter 'help shcmdopt' for more information.\n"

#else /* !defined( _MSVC ) */

#define sh_cmd_help             \
                                \
  "Format: \"sh command [args...]\" where 'command' is any valid shell\n"          \
  "command. The entered command and any arguments are passed as-is to the\n"       \
  "shell for processing and the results are displayed on the Hercules console.\n"  \
  "\n"                                                                             \
  "For security reasons execution of shell commands are disabled by default.\n"    \
  "Enter 'help shcmdopt' for more information.\n"

#endif /* defined( _MSVC ) */

#define shcmdopt_cmd_desc       "Set shell command options"
#define shcmdopt_cmd_help       \
                                \
  "Format:  \"shcmdopt  [DISABLE|ENABLE]  [DIAG8|NODIAG8]\".\n"                 \
  "\n"                                                                          \
  "When set to DISABLE, the 'sh' (host shell command) and 'exec' (execute\n"    \
  "Rexx script) commands are globally disabled and will result in an error\n"   \
  "if entered either directly via the hardware console or programmatically\n"   \
  "via the DIAG 8 interface.\n"                                                 \
  "\n"                                                                          \
  "If the optional NODIAG8 option is specified, then only the programmatic\n"   \
  "execution of commands via the the Diagnose 8 interface are disabled, but\n"  \
  "shell and Rexx commands entered directly via the Hercules command line\n"    \
  "still work.\n"                                                               \
  "\n"                                                                          \
  "NOTE: Enabling this feature has security consequences. When ENABLE DIAG8\n"  \
  "is specified, it's possible for Hercules guest operating systems to issue\n" \
  "commands directly to your host operating system.\n"

#if defined( OPTION_SHARED_DEVICES )

#define shrd_cmd_desc           "shrd command"
#define shrd_cmd_help           \
                                \
  "Format: \"SHRD [TRACE[=nnnn]|[DTAX=0|1]]\" where 'nnnn' is the desired\n"    \
  "number of trace table entries, and DTAX is either 0 or 1. Specifying a\n"    \
  "non-zero TRACE= value enables debug tracing of the Shared Device Server.\n"  \
  "Specifying a value of 0 disables tracing. DTAX is a boolean true/false\n"    \
  "value indicating whether or not to automatically dump the trace table\n"     \
  "when Hercules exits. Both TRACE= and DTAX= must each be set individually\n"  \
  "via separate commands. They cannot both be specified on the same command.\n" \
  "Entering the SHRD command by itself with no arguments displays current\n"    \
  "values. Entering \"SHRD TRACE\" by itself (without defining any value)\n"    \
  "prints the current trace table. SEE ALSO: the 'shrdport' command.\n"

#define shrdport_cmd_desc       "Set shrdport value"
#define shrdport_cmd_help       \
                                \
  "Format:  \"SHRDPORT   [0 | 3990 | nnnn | START | STOP]\"   where nnnn\n"     \
  "is the port number that the Shared Device Server is to use to listen\n"      \
  "for remote connections on.  The default port is 3990.  Setting shrdport\n"   \
  "to 0 stops the server and resets port number back to the default.  Using\n"  \
  "the 'STOP' command also stops the server, but preserves the currently\n"     \
  "established port number so that entering the 'START' command will start\n"   \
  "the server again using the same port number.  Entering the command\n"        \
  "without any argument displays the current value.\n"                          \
  "SEE ALSO: 'shrd' command.\n"

#endif // defined( OPTION_SHARED_DEVICES )

#define sizeof_cmd_desc         "Display size of structures"
#define spm_cmd_desc            "SIE performance monitor"
#define ssd_cmd_desc            "Signal shutdown"
#define ssd_cmd_help            \
                                \
  "The SSD (signal shutdown) command signals an imminent hypervisor\n"          \
  "shutdown to the guest.  Guests who support this are supposed to\n"           \
  "perform a shutdown upon receiving this request.\n"                           \
  "An implicit ssd command is given on a hercules \"quit\" command\n"           \
  "if the guest supports ssd.  In that case hercules shutdown will\n"           \
  "be delayed until the guest has shutdown or a 2nd quit command is\n"          \
  "given. \"ssd now\" will signal the guest immediately, without\n"             \
  "asking for confirmation.\n"

#define start_cmd_desc          "Start CPU (or printer/punch device if argument given)"
#define startall_cmd_desc       "Start all CPU's"
#define start_cmd_help          \
                                \
  "Entering the 'start' command by itself starts the target cpu if it\n"        \
  "is currently stopped. Entering the 'start <devn>' command will press\n"      \
  "the specified printer or punch device's virtual start button. Use the\n"     \
  "'cpu' command beforehand to choose which processor you wish to start.\n"

#define stop_cmd_desc           "Stop CPU (or printer/punch device if argument given)"
#define stopall_cmd_desc        "Stop all CPU's"
#define stop_cmd_help           \
                                \
  "Entering the 'stop' command by itself stops the target cpu if it is\n"       \
  "currently running. Entering the 'stop <devn>' command will press the\n"      \
  "specified printer or punch device's virtual stop button, usually causing\n"  \
  "an INTREQ (Intervention Required) status. Use the 'cpu' command before\n"    \
  "issuing the stop command to choose which processor you wish to stop.\n"

#define store_cmd_desc          "Store CPU status at absolute zero"
#define suspend_cmd_desc        "Suspend hercules"
#define symptom_cmd_desc        "Alias for traceopt"
#define sysclear_cmd_desc       "System Clear Reset manual operation"
#define sysclear_cmd_help       \
                                \
  "Performs the System Reset Clear manual control function. Same as\n"          \
  "the \"sysreset clear\" command. Clears main storage to 0, and all\n"         \
  "registers, control registers, etc.. are reset to their initial value.\n"     \
  "At this point, for architecture modes OTHER than z/Arch, the system is\n"    \
  "essentially in the same state as it was when it was first powered on.\n"     \
  "\n"                                                                          \
  "For z/Arch architecture mode, essentially the same thing happens except\n"   \
  "that your architecture mode is reset to ESA/390 mode in preparation for\n"   \
  "the system being IPLed."

#define sysepoch_cmd_desc       "Set sysepoch parameter"
#define sysreset_cmd_desc       "System Reset manual operation"
#define sysreset_cmd_help       \
                                \
  "Performs the System Reset manual control function. Without any arguments\n"  \
  "or with the \"normal\" argument then only a CPU and I/O subsystem reset\n"   \
  "are performed. When the \"clear\" argument is given then this command is\n"  \
  "identical in functionality to the \"sysclear\" command.\n"

#define sysgport_cmd_desc       "Define SYSG console port"

#define tf_cmd_desc             "Define trace-to-file parameters"
#define tf_cmd_help             \
                                \
  "Format:\n"                                                                   \
  "\n"                                                                          \
  "   tf  [ OFF | ON ] [ \"FILE=filename\" ] [ MAX=nnnS ] [ STOP | NOSTOP ]\n"  \
  "\n"                                                                          \
  "Defines the parameters for instruction and/or ccw tracing to an output\n"    \
  "file. Note that this command only enables/disables tracing to a file;\n"     \
  "you will still need to enable instruction and/or ccw tracing separately\n"   \
  "via the 't+' and/or 't+dev' command(s).\n"                                   \
  "\n"                                                                          \
  "ON or OFF enables or disables tracing to a file. If ON is specified\n"       \
  "then FILE= is required if not already defined by a previous command.\n"      \
  "Enclose the entire option name and value within double quotes if it\n"       \
  "contains any blanks (e.g. \"FILE=my trace file\").\n"                        \
  "\n"                                                                          \
  "MAX= specifies the desired maximum size the trace file is allowed to\n"      \
  "grow to. Specify the value as \"nnnM\" for megabytes or \"nnnG\" for\n"      \
  "gigabytes, where 'nnn' is the maximum number of megabytes/gigabytes\n"       \
  "in size the file is allowed to grow to. Once the maximum is reached,\n"      \
  "both tracefile tracing as well as all instruction and ccw tracing are\n"     \
  "disabled. This prevents instruction and/or ccw tracing from continuing\n"    \
  "to be traced but to the hardware panel instead once the limit has been\n"    \
  "reached. Use the NOSTOP option to disable this behavior and allow the\n"     \
  "instruction and/or ccw tracing to continue, but to the hardware panel\n"     \
  "instead once the limit has been reached. The default is STOP/NOCONT.\n"      \
  "\n"                                                                          \
  "OPEN/CLOSE or NOCONT/CONT may be used in lieu of ON/OFF or STOP/NOSTOP.\n"   \
  "Specify the command without any arguments to display current values."

#define tminus_cmd_desc         "Turn instruction tracing OFF for all CPUs"
#define t_cmd_desc              "Set tracing range or Query tracing"
#define t_cmd_help              \
                                \
  "Format:\n"                                                                   \
  "\n"                                                                          \
  "     \"t [addr-addr]\"\n"                                                    \
  "     \"t [addr:addr]\"\n"                                                    \
  "     \"t [addr.length]\"\n"                                                  \
  "\n"                                                                          \
  "Sets the instruction tracing address range, which is totally separate\n"     \
  "from the instruction stepping and breaking address range.\n"                 \
  "\n"                                                                          \
  "With or without an address range, the 't' command displays whether\n"        \
  "instruction tracing is on or off, and the address range, if any.\n"          \
  "\n"                                                                          \
  "The 't' command by itself does NOT activate instruction tracing.\n"          \
  "Use the 't+' command to activate instruction tracing.  Use 't 0'\n"          \
  "to remove the address range causing all addresses to be traced.\n"

#define tquest_cmd_desc         "Query instruction tracing"
#define tquest_cmd_help         \
                                \
  "Format: \"t?\" displays whether instruction tracing is ON or OFF\n"          \
  "and the address range if any.\n"

#define tckd_cmd_desc           "Turn Search Key tracing ON/OFF for device"
#define tcpu_cmd_desc           "Turn instruction tracing ON/OFF for CPU(s)"
#define odev_cmd_desc           "Turn ORB tracing ON/OFF for device"
#define tdev_cmd_desc           "Turn ORB and CCW tracing ON/OFF for device"
#define tplus_cmd_desc          "Turn instruction tracing ON for all CPUs"
#define tplus_cmd_help          \
                                \
  "Format:   \"t+ [addr-addr]\".    Activates instruction tracing.\n"           \
  "\n"                                                                          \
  "An address range can be specified as for the \"t\" command, else the\n"      \
  "existing address range is used. If there is no address range (or was\n"      \
  "specified as 0) then all instructions will be traced. Please note that\n"    \
  "the \"t+\" command affects ALL cpus.\n"


#define auto_trace_desc         "Automatic instruction tracing"
#define auto_trace_help         \
                                \
  "Format:  \"t+-    [ BEG=<instrcount>   AMT=num ]\"\n"                        \
  "\n"                                                                          \
  "Automatically activates and deactivates instruction tracing based on\n"      \
  "the values provided.\n"                                                      \
  "\n"                                                                          \
  "Once the system-wide instruction count value (displayed at the bottom\n"     \
  "of the screen) becomes greater or equal to the specified 'BEG=' value\n"     \
  "tracing is automatically activated (as if the 't+' command were given)\n"    \
  "and remains active until either at least 'AMT' instructions are traced\n"    \
  "or tracing is explicitly deactivated via the 't-' command.\n"                \
  "\n"                                                                          \
  "Note that instruction counts reported by Hercules are approximate and\n"     \
  "should not be relied on for accuracy.  Also note automatic tracing only\n"   \
  "checks the instruction count approximately once every 257 instructions\n"    \
  "or so and introduces a performance penalty when enabled.\n"                  \
  "\n"                                                                          \
  "Using either of the 't+' or 't-' commands will disable/reset automatic\n"    \
  "tracing.  Enter the 't+-' command by itself (without any arguments) to\n"    \
  "display the current settings.\n"

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )

#define txf_cmd_desc            "Transactional-Execution Facility tracing"
#define txf_cmd_help            \
                                \
  "Format:\n"                                                                   \
  "\n"                                                                          \
  "   txf  [0 | STATS | [INSTR] [U] [C] [GOOD] [BAD] [TDB] [Pages|Lines]\n"     \
  "        [WHY hhhhhhhh] [TAC nnn] [TND nn] [CPU nnn] [FAILS nn] ]\n"          \
  "\n"                                                                          \
  "Where:\n"                                                                    \
  "\n"                                                                          \
  "   0       Disables all txf tracing.\n"                                      \
  "\n"                                                                          \
  "   STATS   Display statistics.\n"                                            \
  "\n"                                                                          \
  "   INSTR   Enables instruction tracing of ONLY transactions.\n"              \
  "           Either 'U' or 'C' or both must also be specified.\n"              \
  "           Default is both. Use 't+' to activate the tracing.\n"             \
  "\n"                                                                          \
  "   U       Enables tracing of unconstrained transactions.\n"                 \
  "   C       Enables tracing of constrained transactions.\n"                   \
  "\n"                                                                          \
  "   GOOD    Enables tracing of successful transactions.\n"                    \
  "           The keyword 'SUCCESS' is also accepted.\n"                        \
  "\n"                                                                          \
  "   BAD     Enables tracing of unsuccessful transactions.\n"                  \
  "           The keyword 'FAIL'or 'FAILURE' is also accepted.\n"               \
  "\n"                                                                          \
  "   TDB     Displays an unsuccessful transaction's TDB.\n"                    \
  "   PAGES   Displays a transaction's page map information.\n"                 \
  "   LINES   Displays a page map's cache line information.\n"                  \
  "\n"                                                                          \
  "   WHY     Trace only when why abort is any of mask hhhhhhhh.\n"             \
  "   TAC     Trace only when abort code = nnn.\n"                              \
  "   TND     Trace only when nesting depth >= nn.\n"                           \
  "   CPU     Trace only when transaction executes on CPU nnn.\n"               \
  "   FAILS   Trace only when abort count >= nn.\n"                             \
  "\n"                                                                          \
  "Enter 'txf' by itself to display the current options. Use 'txf 0'\n"         \
  "to disable all txf tracing. If 'INSTR' is not specified then only\n"         \
  "the results of transactions are traced. If any option other than\n"          \
  "'U' or 'C' is also specified with 'INSTR' then both instructions\n"          \
  "and transaction results are traced. Note: 'txf INSTR' does not by\n"         \
  "itself enable instruction tracing. Use the 't+' command to do that.\n"       \
  "WHY masks are #defined in source file transact.h. A common WHY mask\n"       \
  "is 0xC000FFFF to detect unexpected aborts.\n"

#endif /* defined( _FEATURE_073_TRANSACT_EXEC_FACILITY ) */

#define timerint_cmd_desc       "Display or set timers update interval"
#define timerint_cmd_help       \
                                \
  "Specifies the internal timers update interval, in microseconds.\n"           \
  "This parameter specifies how frequently Hercules's internal\n"               \
  "timers-update thread updates the TOD Clock, CPU Timer, and other\n"          \
  "architectural related clock/timer values.\n"                                 \
  "\n"                                                                          \
  "When the z/Arch Transactional-Execution Facility (073_TRANSACT_EXEC)\n"      \
  "is not installed or enabled, the minimum and default intervals are 1\n"      \
  "and 50 microseconds respectively, which strikes a reasonable balance\n"      \
  "between clock accuracy and overall host performance.\n"                      \
  "\n"                                                                          \
  "When the z/Arch Transactional-Execution Facility *is* installed and\n"       \
  "enabled the minimum and default intervals are 200 and 400 microseconds.\n"   \
  "\n"                                                                          \
  "The maximum allowed interval is "QSTR( MAX_TOD_UPDATE_USECS )" microseconds (one microsecond\n" \
  "less than one second).\n"                                        \
  "\n"                                                                          \
  "Also note that due to host system limitations and/or design, some\n"         \
  "hosts may round up and/or coalesce short microsecond intervals to a\n"       \
  "much longer millisecond interval instead.\n"                                 \
  "\n"                                                                          \
  "CAUTION! While lower TIMERINT values MAY help increase the accuracy\n"       \
  "of your guest's TOD Clock and CPU Timer values, it could also have a\n"      \
  "SEVERE NEGATIVE IMPACT on host operating system performance as well.\n"      \
  "You should exercise EXTREME CAUTION when choosing your TIMERINT value\n"     \
  "in relationship to the actual process priority (nice value) of the\n"        \
  "Hercules process itself.\n"

#define tlb_cmd_desc            "Display TLB tables"
#define toddrag_cmd_desc        "Display or set TOD clock drag factor"
#define traceopt_cmd_desc       "Instruction and/or CCW trace display option"
#define traceopt_cmd_help       \
                                \
  "Format: \"TRACEOPT [TRADITIONAL|REGSFIRST|NOREGS] [NOCH9OFLOW]\".\n"         \
  "Determines how the registers are displayed during instruction tracing\n"     \
  "and stepping and/or whether CCW tracing of printer channel-9 overflow\n"     \
  "unit-checks should be suppressed. Entering the command with no arguments\n"  \
  "displays the current settings. The default is TRADITIONAL.\n"

#define tt32_cmd_desc           "Control/query CTCI-WIN functionality"
#define tt32_cmd_help           \
                                \
  "Format:  \"tt32   debug | nodebug | stats <devnum>\".\n"                     \
  "\n"                                                                          \
  "Enables or disables global CTCI-WIN debug tracing\n"                         \
  "or displays TunTap32 stats for the specified CTC device.\n"

#define tzoffset_cmd_desc       "Set tzoffset parameter"
#define u_cmd_desc              "Disassemble storage"
#define u_cmd_help              \
                                \
  "Format: \"u [R|V|P|H]addr[.len]\" or \"u [R|V|P|H]addr[-addr2]\" to\n"       \
  "disassemble up to 64K of storage beginning at address 'addr' for length\n"   \
  "'len' or to address 'addr2'. The optional R, V, P or H address prefix\n"     \
  "forces Real, Virtual, Primary, or Home Space address translation mode\n"     \
  "instead of using the current PSW mode, which is the default.\n"

#define uptime_cmd_desc         "Display how long Hercules has been running"
#define v_cmd_desc              "Display or alter virtual storage"
#define v_cmd_help              \
                                \
  "Format: \"v [P|S|H]addr[.len]\" or \"v [P|S|H]addr[-addr2]\" to display\n"   \
  "up to 64K of virtual storage, or \"v [P|S|H]addr=value\" to alter up to\n"   \
  "32 bytes of virtual storage, where 'value' is either a string of up to 32\n" \
  "pairs of hex digits, or a string of up to 32 characters enclosed in single\n"  \
  "or double quotes. The optional P, S or H address prefix character forces\n"  \
  "Primary, Secondary or Home Space address translation mode instead of using\n"  \
  "the current PSW mode, which is the default.\n"

#define version_cmd_desc        "Display version information"
#define xpndsize_cmd_desc       "Define/Display xpndsize parameter"
#define xpndsize_cmd_help       \
                                \
  "Format: xpndsize [ mmmm | nnnS [ lOCK | unlOCK ] ]\n"                                \
  "        mmmm    - define expanded storage size mmmm Megabytes\n"                     \
  "\n"                                                                                  \
  "        nnnS    - define expanded storage size nnn S where S is the multiplier\n"    \
  "                  M = 2**20 (mega/mebi)\n"                                           \
  "                  G = 2**30 (giga/gibi)\n"                                           \
  "                  T = 2**40 (tera/tebi)\n"                                           \
  "\n"                                                                                  \
  "      (none)    - display current mainsize value\n"                                  \
  "\n"                                                                                  \
  "        lOCK    - attempt to lock storage (pages lock by host OS)\n"                 \
  "        unlOCK  - leave storage unlocked (pagable by host OS)\n"                     \
  "\n"                                                                                  \
  " Note: Multiplier 'T' is not available on 32bit machines\n"                          \
  "       Expanded storage is limited to 1G on 32bit machines\n"

#define yroffset_cmd_desc       "Set yroffset parameter"

#endif // defined( _FW_REF )        /* (pre-table build pass) */

/*-------------------------------------------------------------------*/
/*                      Standard commands                            */
/*-------------------------------------------------------------------*/

//MMAND( "sample",   "sample command",   sample_cmd,  SYSOPER,    "detailed long help text" )
//MMAND( "sample2",  "sample cmd two",   sample2_cmd, SYSCMD,      NULL )  // No long help provided
//MMAND( "sample3",   NULL,              sample3_cmd, SYSCMD,      NULL )  // No help provided at all
//DABBR( "common", 3, NULL,              sample4_cmd, SYSDEBUG,    NULL )  // Disabled command - generally debugging only
//MMAND( "sss",       "shadow file cmd", NULL,        SYSCMDNOPER, NULL )  // 'ShadowFile_cmd' (special handling)
//MMAND( "x{+/-}zz",  "flag on/off cmd", NULL,        SYSCMDNOPER, NULL )  // 'OnOffCommand'   (special handling)

//       "1...5...9",               function                type flags          description             long help
CMDABBR( "$locate",        4,       locate_cmd,             SYSPROGDEVELDEBUG,  $locate_cmd_desc,       NULL                )
COMMAND( "$runtest",                $runtest_cmd,           SYSPROGDEVELDEBUG,  $runtest_cmd_desc,      $runtest_cmd_help   )
COMMAND( "$test",                   $test_cmd,              SYSPROGDEVELDEBUG,  $test_cmd_desc,         $test_cmd_help      )
CMDABBR( "$zapcmd",        4,       zapcmd_cmd,             SYSPROGDEVELDEBUG,  $zapcmd_cmd_desc,       $zapcmd_cmd_help    )

COMMAND( "cckd",                    cckd_cmd,               SYSCONFIG,          cckd_cmd_desc,          cckd_cmd_help       )
COMMAND( "devtmax",                 devtmax_cmd,            SYSCONFIG,          devtmax_cmd_desc,       devtmax_cmd_help    )
CMDABBR( "legacysenseid",  9,       legacysenseid_cmd,      SYSCONFIG,          legacy_cmd_desc,        NULL                )
COMMAND( "sclproot",                sclproot_cmd,           SYSCONFIG,          sclproot_cmd_desc,      sclproot_cmd_help   )

COMMAND( "#",                       comment_cmd,            SYSALL,             hash_cmd_desc,          NULL                )
COMMAND( "*",                       comment_cmd,            SYSALL,             star_cmd_desc,          NULL                )
COMMAND( "?",                       HelpCommand,            SYSALL,             quest_cmd_desc,         NULL                )
COMMAND( "cmdlvl",                  cmdlvl_cmd,             SYSALL,             cmdlvl_cmd_desc,        cmdlvl_cmd_help     )
COMMAND( "cmdsep",                  cmdsep_cmd,             SYSALL,             cmdsep_cmd_desc,        cmdsep_cmd_help     )
COMMAND( "help",                    HelpCommand,            SYSALL,             help_cmd_desc,          help_cmd_help       )
COMMAND( "hst",                     History,                SYSALL,             hst_cmd_desc,           hst_cmd_help        )
CMDABBR( "message",         1,      msg_cmd,                SYSALL,             message_cmd_desc,       message_cmd_help    )
COMMAND( "msg",                     msg_cmd,                SYSALL,             msg_cmd_desc,           NULL                )
COMMAND( "msglevel",                msglevel_cmd,           SYSALL,             msglevel_cmd_desc,      msglevel_cmd_help   )
COMMAND( "msglvl",                  msglevel_cmd,           SYSALL,             msglvl_cmd_desc,        NULL                )
COMMAND( "msgnoh",                  msg_cmd,                SYSALL,             msgnoh_cmd_desc,        NULL                )
COMMAND( "uptime",                  uptime_cmd,             SYSALL,             uptime_cmd_desc,        NULL                )
COMMAND( "version",                 version_cmd,            SYSALL,             version_cmd_desc,       NULL                )

COMMAND( "attach",                  attach_cmd,             SYSCMD,             attach_cmd_desc,        attach_cmd_help     )
COMMAND( "cpu",                     cpu_cmd,                SYSCMD,             cpu_cmd_desc,           cpu_cmd_help        )
COMMAND( "define",                  define_cmd,             SYSCMD,             define_cmd_desc,        define_cmd_help     )
COMMAND( "detach",                  detach_cmd,             SYSCMD,             detach_cmd_desc,        detach_cmd_help     )
COMMAND( "devinit",                 devinit_cmd,            SYSCMD,             devinit_cmd_desc,       devinit_cmd_help    )
COMMAND( "devlist",                 devlist_cmd,            SYSCMD,             devlist_cmd_desc,       devlist_cmd_help    )
COMMAND( "fcb",                     fcb_cmd,                SYSCMD,             fcb_cmd_desc,           fcb_cmd_help        )
COMMAND( "cctape",                  cctape_cmd,             SYSCMD,             cctape_cmd_desc,        cctape_cmd_help     )
COMMAND( "loadparm",                loadparm_cmd,           SYSCMD,             loadparm_cmd_desc,      loadparm_cmd_help   )
COMMAND( "log",                     log_cmd,                SYSCMD,             log_cmd_desc,           log_cmd_help        )
COMMAND( "logopt",                  logopt_cmd,             SYSCMD,             logopt_cmd_desc,        logopt_cmd_help     )
COMMAND( "mt",                      mt_cmd,                 SYSCMD,             mt_cmd_desc,            mt_cmd_help         )
COMMAND( "mtc",                     mt_cmd,                 SYSCMD,             mtc_cmd_desc,           NULL                )
COMMAND( "panopt",                  panopt_cmd,             SYSCMD,             panopt_cmd_desc,        panopt_cmd_help     )
COMMAND( "panrate",                 panrate_cmd,            SYSCMD,             panrate_cmd_desc,       NULL                )
COMMAND( "pantitle",                pantitle_cmd,           SYSCMD,             pantitle_cmd_desc,      NULL                )
CMDABBR( "qcpuid",          5,      qcpuid_cmd,             SYSCMD,             qcpuid_cmd_desc,        qcpuid_cmd_help     )
COMMAND( "qpid",                    qpid_cmd,               SYSCMD,             qpid_cmd_desc,          NULL                )
CMDABBR( "qports",          5,      qports_cmd,             SYSCMD,             qports_cmd_desc,        NULL                )
COMMAND( "qproc",                   qproc_cmd,              SYSCMD,             qproc_cmd_desc,         NULL                )
COMMAND( "qstor",                   qstor_cmd,              SYSCMD,             qstor_cmd_desc,         NULL                )
COMMAND( "start",                   start_cmd,              SYSCMD,             start_cmd_desc,         start_cmd_help      )
COMMAND( "stop",                    stop_cmd,               SYSCMD,             stop_cmd_desc,          stop_cmd_help       )

COMMAND( "abs",                     abs_or_r_cmd,           SYSCMDNOPER,        abs_cmd_desc,           abs_cmd_help        )
COMMAND( "aea",                     aea_cmd,                SYSCMDNOPER,        aea_cmd_desc,           NULL                )
COMMAND( "aia",                     aia_cmd,                SYSCMDNOPER,        aia_cmd_desc,           NULL                )
COMMAND( "ar",                      ar_cmd,                 SYSCMDNOPER,        ar_cmd_desc,            NULL                )
COMMAND( "autoinit",                autoinit_cmd,           SYSCMDNOPER,        autoinit_cmd_desc,      autoinit_cmd_help   )
COMMAND( "automount",               automount_cmd,          SYSCMDNOPER,        automount_cmd_desc,     automount_cmd_help  )

COMMAND( "b-",                      trace_cmd,              SYSCMDNOPER,        bminus_cmd_desc,        NULL                )
COMMAND( "b",                       trace_cmd,              SYSCMDNOPER,        b_cmd_desc,             b_cmd_help          )
COMMAND( "b?",                      trace_cmd,              SYSCMDNOPER,        bquest_cmd_desc,        NULL                )
COMMAND( "b+",                      trace_cmd,              SYSCMDNOPER,        bplus_cmd_desc,         NULL                )

COMMAND( "bear",                    bear_cmd,               SYSCMDNOPER,        bear_cmd_desc,          bear_cmd_help       )
COMMAND( "cachestats",              EXTCMD(cachestats_cmd), SYSCMDNOPER,        cachestats_cmd_desc,    NULL                )
COMMAND( "clocks",                  clocks_cmd,             SYSCMDNOPER,        clocks_cmd_desc,        NULL                )
COMMAND( "codepage",                codepage_cmd,           SYSCMDNOPER,        codepage_cmd_desc,      codepage_cmd_help   )
COMMAND( "conkpalv",                conkpalv_cmd,           SYSCMDNOPER,        conkpalv_cmd_desc,      conkpalv_cmd_help   )
COMMAND( "cp_updt",                 cp_updt_cmd,            SYSCMDNOPER,        cp_updt_cmd_desc,       cp_updt_cmd_help    )
COMMAND( "cr",                      cr_cmd,                 SYSCMDNOPER,        cr_cmd_desc,            cr_cmd_help         )
COMMAND( "cscript",                 cscript_cmd,            SYSCMDNOPER,        cscript_cmd_desc,       cscript_cmd_help    )
COMMAND( "ctc",                     ctc_cmd,                SYSCMDNOPER,        ctc_cmd_desc,           ctc_cmd_help        )
COMMAND( "ds",                      ds_cmd,                 SYSCMDNOPER,        ds_cmd_desc,            NULL                )
COMMAND( "f?",                      fquest_cmd,             SYSCMDNOPER,        fquest_cmd_desc,        NULL                )
COMMAND( "fpc",                     fpc_cmd,                SYSCMDNOPER,        fpc_cmd_desc,           fpc_cmd_help        )
COMMAND( "fpr",                     fpr_cmd,                SYSCMDNOPER,        fpr_cmd_desc,           fpr_cmd_help        )
COMMAND( "g",                       g_cmd,                  SYSCMDNOPER,        g_cmd_desc,             NULL                )
COMMAND( "gpr",                     gpr_cmd,                SYSCMDNOPER,        gpr_cmd_desc,           gpr_cmd_help        )
COMMAND( "herclogo",                herclogo_cmd,           SYSCMDNOPER,        herclogo_cmd_desc,      herclogo_cmd_help   )
COMMAND( "ipending",                ipending_cmd,           SYSCMDNOPER,        ipending_cmd_desc,      NULL                )
COMMAND( "k",                       k_cmd,                  SYSCMDNOPER,        k_cmd_desc,             NULL                )
COMMAND( "loadcore",                loadcore_cmd,           SYSCMDNOPER,        loadcore_cmd_desc,      loadcore_cmd_help   )
COMMAND( "loadtext",                loadtext_cmd,           SYSCMDNOPER,        loadtext_cmd_desc,      loadtext_cmd_help   )
COMMAND( "maxcpu",                  maxcpu_cmd,             SYSCMDNOPER,        maxcpu_cmd_desc,        NULL                )
CMDABBR( "mounted_tape_reinit",  9, mounted_tape_reinit_cmd,SYSCMDNOPER,        mtapeinit_cmd_desc,     mtapeinit_cmd_help  )
COMMAND( "netdev",                  netdev_cmd,             SYSCMDNOPER,        netdev_cmd_desc,        netdev_cmd_help     )
COMMAND( "numcpu",                  numcpu_cmd,             SYSCMDNOPER,        numcpu_cmd_desc,        NULL                )
//COMMAND( "numvec",                  numvec_cmd,             SYSCMDNOPER,        numvec_cmd_desc,        NULL                )
COMMAND( "osa",                     qeth_cmd,               SYSCMDNOPER,        osa_cmd_desc,           qeth_cmd_help       )
COMMAND( "ostailor",                ostailor_cmd,           SYSCMDNOPER,        ostailor_cmd_desc,      ostailor_cmd_help   )
COMMAND( "pgmtrace",                pgmtrace_cmd,           SYSCMDNOPER,        pgmtrace_cmd_desc,      pgmtrace_cmd_help   )
COMMAND( "pr",                      pr_cmd,                 SYSCMDNOPER,        pr_cmd_desc,            pr_cmd_help         )
COMMAND( "psw",                     psw_cmd,                SYSCMDNOPER,        psw_cmd_desc,           psw_cmd_help        )
COMMAND( "ptp",                     ptp_cmd,                SYSCMDNOPER,        ptp_cmd_desc,           ptp_cmd_help        )
COMMAND( "ptt",                     EXTCMD( ptt_cmd ),      SYSCMDNOPER,        ptt_cmd_desc,           ptt_cmd_help        )
COMMAND( "qd",                      qd_cmd,                 SYSCMDNOPER,        qd_cmd_desc,            qd_cmd_help         )
COMMAND( "qeth",                    qeth_cmd,               SYSCMDNOPER,        qeth_cmd_desc,          qeth_cmd_help       )
COMMAND( "quiet",                   quiet_cmd,              SYSCMDNOPER,        quiet_cmd_desc,         quiet_cmd_help      )
COMMAND( "r",                       abs_or_r_cmd,           SYSCMDNOPER,        r_cmd_desc,             r_cmd_help          )
COMMAND( "resume",                  resume_cmd,             SYSCMDNOPER,        resume_cmd_desc,        NULL                )

COMMAND( "s-",                      trace_cmd,              SYSCMDNOPER,        sminus_cmd_desc,        NULL                )
COMMAND( "s",                       trace_cmd,              SYSCMDNOPER,        s_cmd_desc,             s_cmd_help          )
COMMAND( "s?",                      trace_cmd,              SYSCMDNOPER,        squest_cmd_desc,        squest_cmd_help     )
COMMAND( "s+",                      trace_cmd,              SYSCMDNOPER,        splus_cmd_desc,         splus_cmd_help      )

COMMAND( "savecore",                savecore_cmd,           SYSCMDNOPER,        savecore_cmd_desc,      savecore_cmd_help   )
COMMAND( "script",                  script_cmd,             SYSCMDNOPER,        script_cmd_desc,        script_cmd_help     )
COMMAND( "sh",                      sh_cmd,                 SYSCMDNOPER,        sh_cmd_desc,            sh_cmd_help         )
COMMAND( "suspend",                 suspend_cmd,            SYSCMDNOPER,        suspend_cmd_desc,       NULL                )
COMMAND( "symptom",                 traceopt_cmd,           SYSCMDNOPER,        symptom_cmd_desc,       NULL                )

COMMAND( "tf",                      tf_cmd,                 SYSCMDNOPER,        tf_cmd_desc,            tf_cmd_help         )
COMMAND( "t-",                      trace_cmd,              SYSCMDNOPER,        tminus_cmd_desc,        NULL                )
COMMAND( "t",                       trace_cmd,              SYSCMDNOPER,        t_cmd_desc,             t_cmd_help          )
COMMAND( "t?",                      trace_cmd,              SYSCMDNOPER,        tquest_cmd_desc,        tquest_cmd_help     )
COMMAND( "t+",                      trace_cmd,              SYSCMDNOPER,        tplus_cmd_desc,         tplus_cmd_help      )
#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
COMMAND( "txf",                     txf_cmd,                SYSCMDNOPER,        txf_cmd_desc,           txf_cmd_help        )
#endif
COMMAND( "t+-",                     auto_trace_cmd,         SYSCMDNOPER,        auto_trace_desc,        auto_trace_help     )
COMMAND( "timerint",                timerint_cmd,           SYSCMDNOPER,        timerint_cmd_desc,      timerint_cmd_help   )
COMMAND( "tlb",                     tlb_cmd,                SYSCMDNOPER,        tlb_cmd_desc,           NULL                )
COMMAND( "toddrag",                 toddrag_cmd,            SYSCMDNOPER,        toddrag_cmd_desc,       NULL                )
COMMAND( "traceopt",                traceopt_cmd,           SYSCMDNOPER,        traceopt_cmd_desc,      traceopt_cmd_help   )
COMMAND( "u",                       u_cmd,                  SYSCMDNOPER,        u_cmd_desc,             u_cmd_help          )
COMMAND( "v",                       v_cmd,                  SYSCMDNOPER,        v_cmd_desc,             v_cmd_help          )

COMMAND( "i",                       i_cmd,                  SYSCMDNDIAG8,       i_cmd_desc,             NULL                )
COMMAND( "ipl",                     ipl_cmd,                SYSCMDNDIAG8,       ipl_cmd_desc,           ipl_cmd_help        )
COMMAND( "iplc",                    iplc_cmd,               SYSCMDNDIAG8,       iplc_cmd_desc,          iplc_cmd_help       )
COMMAND( "restart",                 restart_cmd,            SYSCMDNDIAG8,       restart_cmd_desc,       NULL                )
COMMAND( "ext",                     ext_cmd,                SYSCMDNDIAG8,       ext_cmd_desc,           NULL                )
COMMAND( "startall",                startall_cmd,           SYSCMDNDIAG8,       startall_cmd_desc,      NULL                )
COMMAND( "stopall",                 stopall_cmd,            SYSCMDNDIAG8,       stopall_cmd_desc,       NULL                )
COMMAND( "store",                   store_cmd,              SYSCMDNDIAG8,       store_cmd_desc,         NULL                )
COMMAND( "sysclear",                sysclear_cmd,           SYSCMDNDIAG8,       sysclear_cmd_desc,      sysclear_cmd_help   )
COMMAND( "sysreset",                sysreset_cmd,           SYSCMDNDIAG8,       sysreset_cmd_desc,      sysreset_cmd_help   )
COMMAND( "cnslport",                cnslport_cmd,           SYSCFGNDIAG8,       cnslport_cmd_desc,      NULL                )
COMMAND( "cpuidfmt",                cpuidfmt_cmd,           SYSCFGNDIAG8,       cpuidfmt_cmd_desc,      NULL                )
COMMAND( "cpumodel",                cpumodel_cmd,           SYSCFGNDIAG8,       cpumodel_cmd_desc,      NULL                )
COMMAND( "cpuserial",               cpuserial_cmd,          SYSCFGNDIAG8,       cpuserial_cmd_desc,     NULL                )
COMMAND( "cpuverid",                cpuverid_cmd,           SYSCFGNDIAG8,       cpuverid_cmd_desc,      cpuverid_cmd_help   )
COMMAND( "diag8cmd",                diag8_cmd,              SYSCFGNDIAG8,       diag8_cmd_desc,         diag8_cmd_help      )
COMMAND( "engines",                 engines_cmd,            SYSCFGNDIAG8,       engines_cmd_desc,       NULL                )
COMMAND( "lparname",                lparname_cmd,           SYSCFGNDIAG8,       lparname_cmd_desc,      lparname_cmd_help   )
COMMAND( "lparnum",                 lparnum_cmd,            SYSCFGNDIAG8,       lparnum_cmd_desc,       lparnum_cmd_help    )
COMMAND( "mainsize",                mainsize_cmd,           SYSCFGNDIAG8,       mainsize_cmd_desc,      mainsize_cmd_help   )
CMDABBR( "manufacturer",    8,      stsi_manufacturer_cmd,  SYSCFGNDIAG8,       manuf_cmd_desc,         NULL                )
COMMAND( "model",                   stsi_model_cmd,         SYSCFGNDIAG8,       model_cmd_desc,         model_cmd_help      )
COMMAND( "plant",                   stsi_plant_cmd,         SYSCFGNDIAG8,       plant_cmd_desc,         NULL                )
COMMAND( "shcmdopt",                shcmdopt_cmd,           SYSCFGNDIAG8,       shcmdopt_cmd_desc,      shcmdopt_cmd_help   )
COMMAND( "sysepoch",                sysepoch_cmd,           SYSCFGNDIAG8,       sysepoch_cmd_desc,      NULL                )
COMMAND( "sysgport",                sysgport_cmd,           SYSCFGNDIAG8,       sysgport_cmd_desc,      NULL                )
COMMAND( "tzoffset",                tzoffset_cmd,           SYSCFGNDIAG8,       tzoffset_cmd_desc,      NULL                )
COMMAND( "xpndsize",                xpndsize_cmd,           SYSCFGNDIAG8,       xpndsize_cmd_desc,      xpndsize_cmd_help   )
COMMAND( "yroffset",                yroffset_cmd,           SYSCFGNDIAG8,       yroffset_cmd_desc,      NULL                )

COMMAND( "hercnice",                hercnice_cmd,           SYSCFGNDIAG8,       xxxprio_cmd_desc,       xxxprio_cmd_help    )
COMMAND( "hercprio",                hercprio_cmd,           SYSCFGNDIAG8,       xxxprio_cmd_desc,       xxxprio_cmd_help    )
COMMAND( "cpuprio",                 cpuprio_cmd,            SYSCFGNDIAG8,       xxxprio_cmd_desc,       xxxprio_cmd_help    )
COMMAND( "devprio",                 devprio_cmd,            SYSCFGNDIAG8,       xxxprio_cmd_desc,       xxxprio_cmd_help    )
COMMAND( "srvprio",                 srvprio_cmd,            SYSCFGNDIAG8,       xxxprio_cmd_desc,       xxxprio_cmd_help    )
COMMAND( "todprio",                 todprio_cmd,            SYSCFGNDIAG8,       xxxprio_cmd_desc,       xxxprio_cmd_help    )

COMMAND( "archlvl",                 archlvl_cmd,            SYSCMDNOPERNDIAG8,  archlvl_cmd_desc,       archlvl_cmd_help    )
CMDABBR( "facility",        3,      facility_cmd,           SYSCMDNOPERNDIAG8,  facility_cmd_desc,      facility_cmd_help   )
COMMAND( "archmode",                archlvl_cmd,            SYSCMDNOPERNDIAG8,  archmode_cmd_desc,      NULL                )

COMMAND( "exit",                    quit_cmd,               SYSALLNDIAG8,       exit_cmd_desc,          NULL                )

COMMAND( "sizeof",                  sizeof_cmd,             SYSCMDNOPERNPROG,   sizeof_cmd_desc,        NULL                )

COMMAND( "locks",                   EXTCMD( locks_cmd ),    SYSPROGDEVEL,       locks_cmd_desc,         locks_cmd_help      )
COMMAND( "threads",                 EXTCMD( threads_cmd ),  SYSPROGDEVEL,       threads_cmd_desc,       threads_cmd_help    )

/*-------------------------------------------------------------------*/
/*             Commands optional by build option                     */
/*-------------------------------------------------------------------*/

//       "1...5...9",               function                type flags          description             long help
#if defined(_FEATURE_047_CMPSC_ENH_FACILITY)
COMMAND( "cmpscpad",                cmpscpad_cmd,           SYSCFGNDIAG8,       cmpscpad_cmd_desc,      cmpscpad_cmd_help   )
#endif
#if defined( _FEATURE_006_ASN_LX_REUSE_FACILITY )
COMMAND( "alrf",                    alrf_cmd,               SYSCMDNOPER,        alrf_cmd_desc,          NULL                )
COMMAND( "asn_and_lx_reuse",        alrf_cmd,               SYSCMDNOPER,        asnlx_cmd_desc,         NULL                )
#endif
#if defined( _FEATURE_CPU_RECONFIG )
COMMAND( "cf",                      cf_cmd,                 SYSCMDNDIAG8,       cf_cmd_desc,            cf_cmd_help         )
COMMAND( "cfall",                   cfall_cmd,              SYSCMDNDIAG8,       cfall_cmd_desc,         NULL                )
#else
COMMAND( "cf",                      cf_cmd,                 SYSCMDNDIAG8,       NULL,                   NULL                )
COMMAND( "cfall",                   cfall_cmd,              SYSCMDNDIAG8,       NULL,                   NULL                )
#endif
#if defined( _FEATURE_ECPSVM )
COMMAND( "ecps:vm",                 ecpsvm_cmd,             SYSCMDNOPER,        ecps_cmd_desc,          ecps_cmd_help       )
COMMAND( "ecpsvm",                  ecpsvm_cmd,             SYSCMDNOPER,        ecpsvm_cmd_desc,        ecpsvm_cmd_help     )
COMMAND( "evm",                     ecpsvm_cmd,             SYSCMDNOPER,        evm_cmd_desc,           evm_cmd_help        )
#endif
#if defined( _FEATURE_SYSTEM_CONSOLE )
COMMAND( "!message",                g_cmd,                  SYSCMD,             bangmsg_cmd_desc,       bangmsg_cmd_help    )
COMMAND( ".reply",                  g_cmd,                  SYSCMD,             reply_cmd_desc,         reply_cmd_help      )
COMMAND( "\\reply",                 g_cmd,                  SYSCMD,             supp_reply_cmd_desc,    supp_reply_cmd_help )
COMMAND( "iconpfxs",                iconpfxs_cmd,           SYSCMD,             iconpfxs_cmd_desc,      iconpfxs_cmd_help   )
COMMAND( "scpecho",                 scpecho_cmd,            SYSCMD,             scpecho_cmd_desc,       scpecho_cmd_help    )
COMMAND( "scpimply",                scpimply_cmd,           SYSCMD,             scpimply_cmd_desc,      scpimply_cmd_help   )
COMMAND( "ssd",                     ssd_cmd,                SYSCMD,             ssd_cmd_desc,           ssd_cmd_help        )
#endif
#if defined( _FEATURE_SCSI_IPL )
COMMAND( "hwldr",                   hwldr_cmd,              SYSCMDNOPER,        hwldr_cmd_desc,         hwldr_cmd_help      )
COMMAND( "loaddev",                 lddev_cmd,              SYSCMD,             loaddev_cmd_desc,       loaddev_cmd_help    )
COMMAND( "dumpdev",                 lddev_cmd,              SYSCMD,             dumpdev_cmd_desc,       dumpdev_cmd_help    )
#endif
#if !defined( _FW_REF )

        // PROGRAMMING NOTE: the following "+/-" commands ("f+adr", "t+dev",
        // etc) are directly routed by cmdtab.c's "CallHercCmd" function
        // directly to the "OnOffCommand" command function in hsccmd.c.

COMMAND( "f{+/-}adr",               NULL,                   SYSCMDNOPER,        f_cmd_desc,             f_cmd_help          )
COMMAND( "o{+/-}dev",               NULL,                   SYSCMDNOPER,        odev_cmd_desc,          NULL                )
COMMAND( "t{+/-}dev",               NULL,                   SYSCMDNOPER,        tdev_cmd_desc,          NULL                )
COMMAND( "t{+/-}CKD [devnum]",      NULL,                   SYSCMDNOPER,        tckd_cmd_desc,          NULL                )
COMMAND( "t{+/-}CPU [cpunum]",      NULL,                   SYSCMDNOPER,        tcpu_cmd_desc,          NULL                )

        // PROGRAMMING NOTE: the following CCKD 'sf' shadow file commands
        // are directly routed by cmdtab.c's "CallHercCmd" function
        // directly to the "sf_cmd" command function in hsccmd.c.

        // PLEASE ALSO NOTE that, unlike most other Hercules commands, the
        // below CCKD 'sf' shadow file commands are called ASYNCHRONOUSLY
        // from a separate worker thread, and not synchronously like most
        // other Hercules commands are!

COMMAND( "sf-dev",                  NULL,                   SYSCMDNOPER,        sfminus_cmd_desc,       sfminus_cmd_help    )
COMMAND( "sf+dev",                  NULL,                   SYSCMDNOPER,        sfplus_cmd_desc,        sfplus_cmd_help     )
COMMAND( "sfc",                     NULL,                   SYSCMDNOPER,        sfc_cmd_desc,           sfc_cmd_help        )
COMMAND( "sfd",                     NULL,                   SYSCMDNOPER,        sfd_cmd_desc,           sfd_cmd_help        )
COMMAND( "sfk",                     NULL,                   SYSCMDNOPER,        sfk_cmd_desc,           sfk_cmd_help        )
#endif

#if defined(HAVE_OBJECT_REXX) || defined(HAVE_REGINA_REXX)
COMMAND( "rexx",                    rexx_cmd,               SYSCONFIG,          rexx_cmd_desc,          rexx_cmd_help       )
COMMAND( "exec",                    exec_cmd,               SYSCMD,             exec_cmd_desc,          exec_cmd_help       )
#endif /* defined(HAVE_OBJECT_REXX) || defined(HAVE_REGINA_REXX) */

CMDABBR( "qpfkeys",         3,      qpfkeys_cmd,            SYSCMD,             qpfkeys_cmd_desc,       NULL                )
COMMAND( "defsym",                  defsym_cmd,             SYSCMDNOPER,        defsym_cmd_desc,        defsym_cmd_help     )
COMMAND( "delsym",                  delsym_cmd,             SYSCMDNOPER,        delsym_cmd_desc,        delsym_cmd_help     )

#if defined(HAVE_MLOCKALL)
COMMAND( "memlock",                 memlock_cmd,            SYSCONFIG,          NULL,                   NULL                )
COMMAND( "memfree",                 memfree_cmd,            SYSCONFIG,          NULL,                   NULL                )
#endif /*defined(HAVE_MLOCKALL)*/
COMMAND( "modpath",                 modpath_cmd,            SYSCONFIG,          modpath_cmd_desc,       modpath_cmd_help    )
COMMAND( "ldmod",                   ldmod_cmd,              SYSCMDNOPER,        ldmod_cmd_desc,         ldmod_cmd_help      )
COMMAND( "lsdep",                   lsdep_cmd,              SYSCMDNOPER,        lsdep_cmd_desc,         NULL                )
COMMAND( "lsequ",                   lsequ_cmd,              SYSCMDNOPER,        lsequ_cmd_desc,         NULL                )
COMMAND( "lsmod",                   lsmod_cmd,              SYSCMDNOPER,        lsmod_cmd_desc,         lsmod_cmd_help      )
COMMAND( "rmmod",                   rmmod_cmd,              SYSCMDNOPER,        rmmod_cmd_desc,         NULL                )
#if defined( OPTION_HAO )
COMMAND( "hao",                     hao_cmd,                SYSPROGDEVEL,       hao_cmd_desc,           hao_cmd_help        )
#endif
COMMAND( "http",                    http_cmd,               SYSCONFIG,          http_cmd_desc,          http_cmd_help       )
#if defined( OPTION_INSTR_COUNT_AND_TIME )
COMMAND( "icount",                  icount_cmd,             SYSCMDNOPER,        icount_cmd_desc,        icount_cmd_help     )
#endif
#if defined( OPTION_IODELAY_KLUDGE )
COMMAND( "iodelay",                 iodelay_cmd,            SYSCMDNOPER,        iodelay_cmd_desc,       iodelay_cmd_help    )
#endif
COMMAND( "pgmprdos",                pgmprdos_cmd,           SYSCFGNDIAG8,       pgmprdos_cmd_desc,      pgmprdos_cmd_help   )
COMMAND( "maxrates",                maxrates_cmd,           SYSCMD,             maxrates_cmd_desc,      maxrates_cmd_help   )
#if defined( OPTION_SCSI_TAPE )
COMMAND( "auto_scsi_mount",         scsimount_cmd,          SYSCMDNOPER,        autoscsi_cmd_desc,      autoscsi_cmd_help   )
COMMAND( "scsimount",               scsimount_cmd,          SYSCMDNOPER,        scsimount_cmd_desc,     scsimount_cmd_help  )
#endif
#if defined( OPTION_SHARED_DEVICES )
COMMAND( "shrdport",                shrdport_cmd,           SYSCFGNDIAG8,       shrdport_cmd_desc,      shrdport_cmd_help   )
COMMAND( "shrd",                    EXTCMD(shrd_cmd),       SYSCMDNOPER,        shrd_cmd_desc,          shrd_cmd_help       )
#endif
COMMAND( "quit",                    quit_cmd,               SYSALLNDIAG8,       quit_cmd_desc,          quit_cmd_help       )
COMMAND( "quitmout",                quitmout_cmd,           SYSALLNDIAG8,       quitmout_cmd_desc,      quitmout_cmd_help   )
#if defined( OPTION_W32_CTCI )
COMMAND( "tt32",                    tt32_cmd,               SYSCMDNOPER,        tt32_cmd_desc,          tt32_cmd_help       )
#endif
#if defined( SIE_DEBUG_PERFMON )
COMMAND( "spm",                     spm_cmd,                SYSCMDNOPER,        spm_cmd_desc,           NULL                )
#endif

/*------------------------------(EOF)--------------------------------*/
