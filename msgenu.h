/* MSGENU.H     (C) Copyright Roger Bowler and others, 1999-2022     */
/*              (C) Copyright Bernard van der Helm, 2010-2012        */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*              (C) and others 2011-2023                             */
/*              Header file for Hercules messages (US English)       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This file contains the text of all of the messages issued by      */
/* the various components of the Hercules mainframe emulator.        */
/*-------------------------------------------------------------------*/

#ifndef _MSGENU_H_
#define _MSGENU_H_

#include "printfmt.h"       /* Hercules printf/sscanf format strings */
/*
-----------------------------------------------------------------------
                        Message principles
-----------------------------------------------------------------------

HHCnnnnns message-text...

  HHC     'HHC' is the standard prefix for all Hercules messages.
  nnnnn   Five numeric digit code that unifies the Hercules message.
  s       The capital letter that denotes the severity.
  ' '     One space character between the message prefix and Text.

-----------------------------------------------------------------------
                        Severity principles
-----------------------------------------------------------------------

  S   Severe error. Hercules cannot continue.
  E   Error. Hercules continues.
  W   Warning message.
  I   Informatonal message.
  A   Action. Hercules needs input.
  D   Debugging message.

-----------------------------------------------------------------------
                         Text principles
-----------------------------------------------------------------------

Text:

  1. The text is written in lower characters and starts with a
     capital and does not end with a '.', '!' or something else.

  2. Added strings are rounded with apostrophes to distinguish it
     from the text and to see the start and endpoint of the string.

Examples:

  HHC01234I This is a correct message
  HHC12345E This is wrong!
  HHC23456E THIS IS ALSO WRONG
  HHC34567E Do not end with a dot or something else.

-----------------------------------------------------------------------
                    Message format principles
-----------------------------------------------------------------------

  Device:     "%1d:%04X", lcssnum, devnum
  Processor:  "%s%02X", PTYPSTR(procnum), procnum
  32bit reg:  "%08X", r32
  64bit reg:  "%016X", r64 (in 64bit mode)
  64bit reg:  "--------%08X", r32 (under SIE in 32 bit)
  64bit psw:  "%016X", psw64
  64bit psw:  "%016X ----------------", psw32 (under SIE in 32 bit)
  128bit psw: "%016X %016X", psw64h, psw64l
  Regs:       "GR%02d CR%02d AR%02d FP%02d", grnum, crnum, arnum, fpnum
  Strings:    "%s", ""
*/
/*-------------------------------------------------------------------*/
/*                  PRIMARY MESSAGE FUNCTIONS                        */
/*-------------------------------------------------------------------*/
LOGM_DLL_IMPORT void fwritemsg( const char* filename, int line, const char* func, BYTE panel, FILE* f, const char* fmt, ... ) ATTR_PRINTF( 6, 7 );
LOGM_DLL_IMPORT void logmsg( const char* fmt, ... ) ATTR_PRINTF( 1, 2 );
LOGM_DLL_IMPORT int  panel_command_capture( char* cmd, char** resp, bool quiet );

/*-------------------------------------------------------------------*/
/*                    PRIMARY MESAGE MACROS                          */
/*-------------------------------------------------------------------*/
/* Write message options used with below 'P' (panel) type macros     */
/*-------------------------------------------------------------------*/
#define WRMSG_CAPTURE       0x01            /* Allow capturing       */
#define WRMSG_PANEL         0x02            /* Write to panel        */
#define WRMSG_NORMAL        (WRMSG_CAPTURE | WRMSG_PANEL)

/*-------------------------------------------------------------------*/
/* The below two macros should NOT be used under normal situations.  */
/* Use one of the macros further below instead (FWRMSG, WRMSG, etc). */
/* PROGRAMMING NOTE: the "##" preceding "__VA_ARGS__" is required    */
/* for compatibility with both gcc and MSVC and MUSTN'T be removed!  */
/*-------------------------------------------------------------------*/
#define PFWRITEMSG( pan, f, fmt, ... ) fwritemsg( __FILE__, __LINE__, __FUNCTION__, pan,   f,    fmt, ## __VA_ARGS__ )
#define  PWRITEMSG( pan,    fmt, ... ) PFWRITEMSG(                                  pan, stdout, fmt, ## __VA_ARGS__ )

/*-------------------------------------------------------------------*/
/* Macros to write a message to specific FILE* or stdout by default. */
/* PROGRAMMING NOTE: the "##" preceding "__VA_ARGS__" is required    */
/* for compatibility with both gcc and MSVC and MUSTN'T be removed!  */
/*-------------------------------------------------------------------*/
#define FLOGMSG( f,   fmt,   ... ) PFWRITEMSG( WRMSG_NORMAL, f,    fmt,                 ## __VA_ARGS__ )
#define  LOGMSG(      fmt,   ... ) PWRITEMSG(  WRMSG_NORMAL,       fmt,                 ## __VA_ARGS__ )
#define FWRMSG(  f, id, sev, ... ) FLOGMSG(                  f, #id "%s " id "\n", sev, ## __VA_ARGS__ )
#define  WRMSG(     id, sev, ... ) LOGMSG(                      #id "%s " id "\n", sev, ## __VA_ARGS__ )

/*-------------------------------------------------------------------*/
/* Macros that allow you to specify a custom write message option.   */
/* Use these macros when you want to issue a message to the panel    */
/* ONLY without it appearing in DIAG8's response buffer for example. */
/* PROGRAMMING NOTE: the "##" preceding "__VA_ARGS__" is required    */
/* for compatibility with both gcc and MSVC and MUSTN'T be removed!  */
/*-------------------------------------------------------------------*/
#define PFLOGMSG( pan, f, fmt,     ... ) PFWRITEMSG( pan, f,    fmt,                 ## __VA_ARGS__ )
#define  PLOGMSG( pan,    fmt,     ... ) PWRITEMSG(  pan,       fmt,                 ## __VA_ARGS__ )
#define PFWRMSG(  pan, f, id, sev, ... ) PFLOGMSG(   pan, f, #id "%s " id "\n", sev, ## __VA_ARGS__ )
#define  PWRMSG(  pan,    id, sev, ... ) PLOGMSG(    pan,    #id "%s " id "\n", sev, ## __VA_ARGS__ )

/*-------------------------------------------------------------------*/
/* helper macro for device messages                                  */
/*-------------------------------------------------------------------*/

#define LCSS_DEVNUM_DEV(_dev)   (_dev)?SSID_TO_LCSS((_dev)->ssid):0,(_dev)?(_dev)->devnum:0
#define LCSS_DEVNUM             LCSS_DEVNUM_DEV(dev)

/*-------------------------------------------------------------------*/
/* ckddasd.c/fbadasd.c dasd I/O tracing helper macro                 */
/*-------------------------------------------------------------------*/
#define LOGDEVTR( id, sev, ... )                                \
    do                                                          \
    {                                                           \
        if (1                                                   \
            && dev->ccwtrace                                    \
            && (0                                               \
                || !dev->ccwopstrace                            \
                ||  dev->ccwops[ dev->code ]                    \
               )                                                \
        )                                                       \
        {                                                       \
            /* PROGRAMMING NOTE: we must call 'fwritemsg'       \
               directly since attempting to use the 'WRMSG'     \
               macro confuses poor dumb gcc/clang. (SIGH!)      \
            */                                                  \
            /* "Thread "TIDPAT" %1d:%04X .... */                \
            fwritemsg( __FILE__, __LINE__, __FUNCTION__,        \
                WRMSG_NORMAL, stdout,                           \
                #id "%s " id "\n", sev,                         \
                TID_CAST(hthread_self()),                       \
                SSID_TO_LCSS( dev->ssid ),                      \
                dev->devnum, ## __VA_ARGS__ );                  \
        }                                                       \
    }                                                           \
    while (0)

/*-------------------------------------------------------------------*/
/*                     Message helper macros                         */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Use MSGBUF in situations where you would normally use snprintf:   */
/*                                                                   */
/*    #define HHC99999 "Table entry %d: %s"                          */
/*                                                                   */
/*    char buf[128];                                                 */
/*                                                                   */
/*    if (tab.type == str)                                           */
/*        MSGBUF( buf, "type = str, val = %s", tab.strval );         */
/*    else                                                           */
/*        MSGBUF( buf, "type = int, val = %d", tab.intval );         */
/*    WRMSG( HHC99999, "D", i, buf );                                */
/*                                                                   */
/*                                                                   */
/*             Writing multiple messages as a group                  */
/*            --------------------------------------                 */
/*                                                                   */
/* To write multiple messages as a group, format your messages into  */
/* a work buffer using the MSGBUF and MSG/MSG_C macros. The first    */
/* message formatted should be formatted WITHOUT a message number,   */
/* but all other messages following it SHOULD. The last message      */
/* should NOT end with a newline (i.e. use the 'MSG_C' macro).       */
/*                                                                   */
/* Then simply issue your message using the WRMSG macro specifying   */
/* the message number for the first message. The format should be    */
/* defined with as many "%s" as you have mesages. This will prefix   */
/* the first message with the desired message number and terminate   */
/* the last message with a newline, and then write the entire group  */
/* in one fell swoop.                                                */
/*                                                                   */
/* NOTE:  Use  the  MSGBUF  macro *only* when the buffer is declared */
/* locally as an array of characters.  *Never* use it with a pointer */
/* passed  to  a function; sizeof(_buf) will be 4 or 8, depending on */
/* the  size  of  a  pointer.  GCC will warn you, but the warning is */
/* rather obscure and chances are you'll ignore it.                  */
/*                                                                   */
/* For example:                                                      */
/*                                                                   */
/*    #define HHC00001 "%s%s%s%s" // message group                   */
/*    #define HHC00002 "Message 2: var1: %d, var2: %s"               */
/*    #define HHC00003 "Message 3: var1: %d, var2: %s"               */
/*    #define HHC00009 "Last message: var1: %d, var2: %s"            */
/*                                                                   */
/*    char msg1[80], msg2[80], msg3[80, msg4[80];                    */
/*    MSGBUF( msg1, "Message 1: var1: %d, var2: %s", 12, "34" );     */
/*    MSGBUF( msg2, MSG( HHC00002, "I", 12, "34" ));                 */
/*    MSGBUF( msg3, MSG( HHC00003, "I", 12, "34" ));                 */
/*    MSGBUF( msg4, MSG_C( HHC00009, "I", 12, "34" ));               */
/*    WRMSG( HHC00001, "I", msg1, msg2, msg3, msg4 );                */
/*                                                                   */
/*-------------------------------------------------------------------*/

#define MSGBUF( _buf, ... )     snprintf(_buf, sizeof(_buf),      ## __VA_ARGS__ )
#define MSG( id, sev, ... )     #id "%s " id "\n", sev,           ## __VA_ARGS__
#define MSG_C( id, sev, ... )   #id "%s " id "",   sev,           ## __VA_ARGS__
#define EXTGUIMSG( ... )        do { if (extgui) fprintf( stderr, ## __VA_ARGS__ ); } while (0)

/*-------------------------------------------------------------------*/
/*                   MLVL(DEBUG) message prefix                      */
/*-------------------------------------------------------------------*/

#define MLVL_DEBUG_PFXLEN       17
#define MLVL_DEBUG_PFXFMT       "%-" QSTR( MLVL_DEBUG_PFXLEN ) "." QSTR( MLVL_DEBUG_PFXLEN ) "s "
#define MLVL_DEBUG_PFXIDX       (MLVL_DEBUG_PFXLEN + 1 /* space sep */)

/*-------------------------------------------------------------------*/
/*                       HERCULES MESSAGES                           */
/*-------------------------------------------------------------------*/

/* Hercules messages */
#define HHC00001 "%s%s"
#define HHC00002 "SCLP console not receiving %s"
#define HHC00003 "Empty SCP command issued"
#define HHC00004 "Control program identification: type %s, name %s, sysplex %s, level %"PRIX64
#define HHC00005 "The configuration has been placed into a system check-stop state because of an incompatible service call"
#define HHC00006 "SCLP console interface %s"
#define HHC00007 "Previous message from function '%s' at %s(%d)"
//efine HHC00008 (available)
#define HHC00009 "RRR...RING...GGG!\a"
#define HHC00010 "Enter '%s' input for console %1d:%04X"
#define HHC00011 "Function %s failed; cache %d size %d: [%02d] %s"
#define HHC00012 "Releasing inactive buffer storage"
#define HHC00013 "'%s' input entered for console %1d:%04X: \"%s\""
#define HHC00014 "select: %s"
#define HHC00015 "keyboard read: %s"
//efine HHC00016 (available)
#define HHC00017 "Crash dumps NOT enabled"
#define HHC00018 "Hercules is %srunning in elevated mode"
#define HHC00019 "Hercules IS running in test mode"

// reserve 20-39 for option related
#define HHC00020 "Test timeout factor %s outside of valid range 1.0 to %3.1f"
#define HHC00021 "Test timeout factor = %3.1f"
//efine HHC00022 (available)
#define HHC00023 "Invalid/unsupported option: %s"
#define HHC00024 "Unrecognized option: %s"
#define HHC00025 "Invalid help option argument: %s"
//efine HHC00026 - HHC00069 (available)

// HHC0007x, HHC0008x and HHC0009x reserved for hao.c. (to recognize own messages)
#define HHC00070 "Unknown hao command, valid commands are:\n" \
       "HHC00070I hao tgt <tgt> : define target rule (pattern) to react on\n" \
       "HHC00070I hao cmd <cmd> : define command for previously defined rule\n" \
       "HHC00070I hao list <n>  : list all rules/commands or only at index <n>\n" \
       "HHC00070I hao del <n>   : delete the rule at index <n>\n" \
       "HHC00070I hao clear     : delete all rules (stops automatic operator)"
#define HHC00071 "The %s was not added because table is full; table size is %02d"
#define HHC00072 "The command %s given, but the command %s was expected"
#define HHC00073 "Empty %s specified"
#define HHC00074 "The target was not added because a duplicate was found in the table at %02d"
#define HHC00075 "Error in function %s: %s"
#define HHC00076 "The %s was not added because it causes a loop with the %s at index %02d"
#define HHC00077 "The %s was placed at index %d"
#define HHC00078 "The command was not added because it may cause dead locks"
#define HHC00079 "No rule defined at index %02d"
#define HHC00080 "All HAO rules are cleared"
#define HHC00081 "Match at index %02d, executing command %s"
#define HHC00082 "%d rule(s) displayed"
#define HHC00083 "The command 'del' was given without a valid index"
#define HHC00084 "Invalid index; index must be between 0 and %02d"
#define HHC00085 "Rule at index %d not deleted, already empty"
#define HHC00086 "Rule at index %d successfully deleted"
#define HHC00087 "The defined Hercules Automatic Operator rule(s) are:"
#define HHC00088 "Index %02d: target %s -> command %s"
#define HHC00089 "The are no HAO rules defined"
#define HHC00090 "HAO thread waiting for logger facility to become active"
#define HHC00091 "Logger facility now active; HAO thread proceeding"
#define HHC00092 "Warning in function %s: %s"
//efine HHC00093 - HHC00099 (available)

// reserve 100-129 thread related
#define HHC00100 "Thread id "TIDPAT", prio %d, name '%s' started"
#define HHC00101 "Thread id "TIDPAT", prio %d, name '%s' ended"
#define HHC00102 "Error in function create_thread(): %s"
//efine HHC00103 (available)
//efine HHC00105 (available)
#define HHC00106 "Error in function create_thread() for %s %d of %d: %s"
#define HHC00107 "Starting thread %s, active=%d, started=%d, max=%d"
#define HHC00108 "Ending thread "TIDPAT" %s, pri=%d, started=%d, max=%d exceeded"
#define HHC00109 "set_thread_priority( %d ) failed: %s"
#define HHC00110 "Defaulting all threads to priority %d"
#define HHC00111 "Thread CPU Time IS available (_POSIX_THREAD_CPUTIME=%ld)"
#define HHC00112 "Thread CPU Time is NOT available."
#define HHC00113 "Setting main thread QoS to USER_INITIATED failed: %s"
//efine HHC00114 - HHC00129 (available)

#define HHC00130 "PGMPRDOS LICENSED specified and a licenced program product operating system is running"
#define HHC00131 "A licensed program product operating system detected, all processors have been stopped"
//efine HHC00132 (available)
//efine HHC00133 (available)
//efine HHC00134 (available)
#define HHC00135 "Timeout for module %s, possible older version"
#define HHC00136 "Error in function %s: %s"
#define HHC00137 "Error opening TUN/TAP device %s: %s"
#define HHC00138 "Error setting TUN/TAP mode %s: %s"
#define HHC00139 "Invalid TUN/TAP device name %s"
#define HHC00140 "Invalid net device name %s"
#define HHC00141 "Net device %s: Invalid IP %s"
#define HHC00142 "Net device %s: Invalid destination address %s"
#define HHC00143 "Net device %s: Invalid net mask %s"
#define HHC00144 "Net device %s: Invalid MTU %s"
#define HHC00145 "Net device %s: Invalid MAC address %s"
#define HHC00146 "Net device %s: Invalid gateway address %s"
//efine HHC00147 (available)
#define HHC00148 "Closing %"PRId64" files"
#define HHC00149 "IFC_IOCtl called for %s on FDs %d %d"
#define HHC00150 "%s module loaded%s"
#define HHC00151 "Activated facility: %s"
#define HHC00152 "Out of memory"
#define HHC00153 "Net device %s: Invalid prefix length %s"
#define HHC00154 "Preconfigured interface %s does not exist or is not accessible by Hercules (EPERM)"
#define HHC00155 "Net device %s: Invalid broadcast address %s"
#define HHC00156 "IFF_TUN requested but not a tun device: %s"
#define HHC00157 "IFF_TAP requested but not a tap device: %s"
//efine HHC00158 (available)
#define HHC00159 "Overly long %d byte SCP message truncated to %d bytes"
#define HHC00160 "SCP %scommand: %s"
#define HHC00161 "Function %s failed: [%02d] %s"
// hercifc.c: 162-169
#define HHC00162 "%s: Must be called from within Hercules; ... exiting"
#define HHC00163 "%s: Cannot obtain %s socket: %s; ... exiting"
#define HHC00164 "%s: I/O error on read: %s; ... exiting"
#define HHC00165 "%s: Unknown request: %lX; ... ignoring and continuing"
#define HHC00166 "%s: ioctl error doing %s on %s: %d %s; ... ignoring and continuing"
#define HHC00167 "%s: Doing %s on %s ..."
#define HHC00168 "%s: Hercules disappeared!! ... exiting"
#define HHC00169 "%s: DONE! ... exiting"
//efine HHC00170 - HHC00199 (available)

// reserve 002xx for tape device related
#define HHC00201 "%1d:%04X Tape file %s, type %s: tape closed"
#define HHC00202 "%1d:%04X Tape file %s, type %s: block length %d exceeds maximum at offset 0x%16.16"PRIX64
#define HHC00203 "%1d:%04X Tape file %s, type %s: invalid tapemark at offset 0x%16.16"PRIX64
#define HHC00204 "%1d:%04X Tape file %s, type %s: error in function %s, offset 0x%16.16"PRIX64": %s"
#define HHC00205 "%1d:%04X Tape file %s, type %s: error in function %s: %s"
#define HHC00206 "%1d:%04X Tape file %s, type %s: not a valid @TDF file: %s"
#define HHC00207 "%1d:%04X Tape file %s, type %s: line %d: %s"
#define HHC00208 "%1d:%04X Tape file %s, type %s: maximum tape capacity exceeded"
#define HHC00209 "%1d:%04X Tape file %s, type %s: maximum tape capacity enforced"
#define HHC00210 "%1d:%04X Tape file %s, type %s: tape unloaded"
#define HHC00211 "%1d:%04X Tape file %s, type scsi status %s, sstat 0x%8.8"PRIX32": %s"
#define HHC00212 "%1d:%04X Tape file %s, type %s: data chaining not supported for CCW %2.2X"
#define HHC00213 "%1d:%04X Tape file %s, type %s: Error opening: errno=%d: %s"
#define HHC00214 "%1d:%04X Tape file %s, type %s: auto-mount rejected: drive not empty"
#define HHC00215 "%1d:%04X Tape file %s, type %s: auto-mounted"
#define HHC00216 "%1d:%04X Tape file %s, type %s: auto-unmounted"
#define HHC00217 "%1d:%04X Tape file %s, type %s: locate block 0x%8.8"PRIX32
#define HHC00218 "%1d:%04X Tape file %s, type %s: display %s until unmounted"
#define HHC00219 "%1d:%04X Tape file %s, type %s: display %s until unmounted, then %s until mounted"
#define HHC00220 "%1d:%04X Tape file %s, type %s: format type is not determinable, presumed %s"
#define HHC00221 "%1d:%04X Tape file %s, type %s: format type %s"
#define HHC00222 "%1d:%04X Tape file %s, type %s: option %s accepted"
#define HHC00223 "%1d:%04X Tape file %s, type %s: option %s rejected: %s"
#define HHC00224 "%1d:%04X Tape file %s, type %s: display %s"
#define HHC00225 "Unsupported tape device type %04X specified"
#define HHC00226 "%1d:%04X Tape file %s, type %s: %s tape volume %s being auto unloaded"
#define HHC00227 "%1d:%04X Tape file %s, type %s: %s tape volume %s being auto loaded"
#define HHC00228 "Tape autoloader: file request fn %s"
#define HHC00229 "Tape autoloader: adding %s value %s"
//efine HHC00230 (available)
//efine HHC00231 (available)
//efine HHC00232 (available)
//efine HHC00233 (available)
//efine HHC00234 (available)
#define HHC00235 "%1d:%04X Tape file %s, type %s: tape created"
//efine HHC00236 (available)
//efine HHC00237 (available)
//efine HHC00238 (available)
//efine HHC00239 (available)
//efine HHC00240 (available)
//efine HHC00241 (available)
//efine HHC00242 (available)
#define HHC00243 "%1d:%04X Tape status retrieval timeout"
//efine HHC00243 - HHC00299 (available)

// reserve 003xx for compressed dasd device related
#define HHC00300 "%1d:%04X CCKD file: error initializing shadow files"
#define HHC00301 "%1d:%04X CCKD file[%d] %s: error in function %s: %s"
#define HHC00302 "%1d:%04X CCKD file[%d] %s: error in function %s at offset 0x%16.16"PRIX64": %s"
#define HHC00303 "%1d:%04X CCKD file: error in function %s: %s"
#define HHC00304 "%1d:%04X CCKD file[%d] %s: get space error, size exceeds %"PRId64"M"
#define HHC00305 "%1d:%04X CCKD file[%d] %s: device header id error"
#define HHC00306 "%1d:%04X CCKD file[%d] %s: trklen error for BCCHH = %2.2x%4.4x%4.4x"
#define HHC00307 "%1d:%04X CCKD file[%d] %s: invalid byte 0 trk %d, buf %2.2x%2.2x%2.2x%2.2x%2.2x"
#define HHC00308 "%1d:%04X CCKD file[%d] %s: invalid byte 0 blkgrp %d, buf %2.2x%2.2x%2.2x%2.2x%2.2x"
#define HHC00309 "%1d:%04X CCKD file[%d] %s: invalid %s hdr %s %d: %s compression unsupported"
#define HHC00310 "%1d:%04X CCKD file[%d] %s: invalid %s hdr %s %d buf %p:%2.2x%2.2x%2.2x%2.2x%2.2x"
#define HHC00311 "%1d:%04X CCKD file[%d] %s: shadow file name collides with %1d:%04X file[%d] %s"
#define HHC00312 "%1d:%04X CCKD file[%d] %s: error re-opening readonly: %s"
#define HHC00313 "%1d:%04X CCKD file[%d]: no shadow file name"
#define HHC00314 "%1d:%04X CCKD file[%d] %s:  max shadow files exceeded"
#define HHC00315 "%1d:%04X CCKD file: adding shadow files..."
#define HHC00316 "CCKD file number of devices processed: %d"
#define HHC00317 "%1d:%04X CCKD file: device is not a cckd device"
#define HHC00318 "%1d:%04X CCKD file[%d] %s: error adding shadow file, sf command busy on device"
#define HHC00319 "%1d:%04X CCKD file[%d] %s: error adding shadow file"
#define HHC00320 "%1d:%04X CCKD file[%d] %s: shadow file succesfully added"
#define HHC00321 "%1d:%04X CCKD file: merging shadow files..."
#define HHC00322 "%1d:%04X CCKD file[%d] %s: error merging shadow file, sf command busy on device"
#define HHC00323 "%1d:%04X CCKD file[%d] %s: cannot remove base file"
#define HHC00324 "%1d:%04X CCKD file[%d] %s: shadow file not merged: file[%d] %s%s"
#define HHC00325 "%1d:%04X CCKD file[%d] %s: shadow file successfully %s"
#define HHC00326 "%1d:%04X CCKD file[%d] %s: shadow file not merged, error during merge"
#define HHC00327 "%1d:%04X CCKD file[%d] %s: shadow file not merged, error processing trk(%d)"
#define HHC00328 "%1d:%04X CCKD file: compressing shadow files..."
#define HHC00329 "%1d:%04X CCKD file[%d] %s: error compressing shadow file, sf command busy on device"
#define HHC00330 "%1d:%04X CCKD file: checking level %d..."
#define HHC00331 "%1d:%04X CCKD file[%d] %s: shadow file check failed, sf command busy on device"
#define HHC00332 "%1d:%04X CCKD file: display cckd statistics"
#define HHC00333 "%1d:%04X   32/64       size free  nbr st   reads  writes l2reads    hits switches"
#define HHC00334 "%1d:%04X                                                      readaheads   misses"
#define HHC00335 "%1d:%04X ------------------------------------------------------------------------"
#define HHC00336 "%1d:%04X [*] %s %11.11"PRId64" %3.3"PRId64"%% %4.4"PRId64"    %7.7d %7.7d %7.7d %7.7d  %7.7d"
#define HHC00337 "%1d:%04X                                                         %7.7d  %7.7d"
#define HHC00338 "%1d:%04X %s"
#define HHC00339 "%1d:%04X [0] %s %11.11"PRId64" %3.3"PRId64"%% %4.4"PRId64" %s %7.7d %7.7d %7.7d"
#define HHC00340 "%1d:%04X %s"
#define HHC00341 "%1d:%04X [%d] %s %11.11"PRId64" %3.3"PRId64"%% %4.4"PRId64" %s %7.7d %7.7d %7.7d"
#define HHC00342 "%1d:%04X CCKD file[%d] %s: %s(%d): offset 0x%16.16"PRIx64" unknown space %2.2x%2.2x%2.2x%2.2x%2.2x"
#define HHC00343 "%1d:%04X CCKD file[%d] %s: uncompress error trk %d: %2.2x%2.2x%2.2x%2.2x%2.2x"
#define HHC00344 "%1d:%04X CCKD file[%d] %s: compression %s not supported"
#define HHC00345 "%s" /* cckd help output */
#define HHC00346 "%s" /* cckd opts output */
#define HHC00347 "%s" /* cckd stats output */
#define HHC00348 "CCKD file: invalid value %d for %s"
#define HHC00349 "CCKD file: invalid cckd keyword: %s"
#define HHC00350 "CCKD file: invalid numeric value %s"
#define HHC00351 "%1d:%04X CCKD file[%d] %s: cckd/64 format differs from base"
#define HHC00352 "%1d:%04X CCKD file %s: opened bit is on, use -f"
#define HHC00353 "%1d:%04X CCKD file %s: check disk errors"
#define HHC00354 "%1d:%04X CCKD file %s: error in function %s: %s"
#define HHC00355 "%1d:%04X CCKD file %s: error in function %s at offset 0x%16.16"PRIX64": %s"
#define HHC00356 "%1d:%04X CCKD file %s: not a compressed dasd file"
#define HHC00357 "%1d:%04X CCKD file %s: converting to %s"
#define HHC00358 "%1d:%04X CCKD file %s: file already compressed"
#define HHC00359 "%1d:%04X CCKD file %s: compress successful, %"PRId64" bytes released"
#define HHC00360 "%1d:%04X CCKD file %s: compress successful, L2 tables relocated"
#define HHC00361 "%1d:%04X CCKD file %s: dasd lookup error type %02X cylinders %d"
#define HHC00362 "%1d:%04X CCKD file %s: bad %s %"PRId64", expecting %"PRId64
#define HHC00363 "%1d:%04X CCKD file %s: cdevhdr inconsistencies found, code %4.4X"
#define HHC00364 "%1d:%04X CCKD file %s: forcing check level %d"
#define HHC00365 "%1d:%04X CCKD file %s: %s offset 0x%16.16"PRIX64" len %"PRId64" is out of bounds"
#define HHC00366 "%1d:%04X CCKD file %s: %s offset 0x%16.16"PRIX64" len %"PRId64" overlaps %s offset 0x%"PRIX64
#define HHC00367 "%1d:%04X CCKD file %s: %s[%d] l2 inconsistency: len %"PRId64", size %"PRId64
#define HHC00368 "%1d:%04X CCKD file %s: free space errors detected"
#define HHC00369 "%1d:%04X CCKD file %s: %s[%d] hdr error offset 0x%16.16"PRIX64": %2.2X%2.2X%2.2X%2.2X%2.2X"
#define HHC00370 "%1d:%04X CCKD file %s: %s[%d] compressed using %s, not supported"
#define HHC00371 "%1d:%04X CCKD file %s: %s[%d] offset 0x%16.16"PRIX64" len %"PRId64" validation error"
#define HHC00372 "%1d:%04X CCKD file %s: %s[%d] recovered offset 0x%16.16"PRIX64" len %"PRId64
#define HHC00373 "%1d:%04X CCKD file %s: %"PRId64" %s images recovered"
#define HHC00374 "%1d:%04X CCKD file %s: not enough file space for recovery"
#define HHC00375 "%1d:%04X CCKD file %s: recovery not completed: %s"
#define HHC00376 "%1d:%04X CCKD file %s: free space not rebuilt, file opened read-only"
#define HHC00377 "%1d:%04X CCKD file %s: free space rebuilt"
#define HHC00378 "%1d:%04X CCKD file %s: error during swap"
#define HHC00379 "%1d:%04X CCKD file %s: starting %s level %d%s..."
#define HHC00380 "%1d:%04X CCKD file %s: %s level %d complete; rc=%d"
#define HHC00381 "%1d:%04X CCKD file %s: closing device while wrpending=%d cckdioact=%d"
#define HHC00382 "Begin CCKD garbage collection"
#define HHC00383 "End CCKD garbage collection"
#define HHC00384 "Collecting garbage for CCKD%s file[%d] %1d:%04X %s..."
#define HHC00385 "Skipping garbage collection for CCKD%s file[%d] %1d:%04X %s due to space errors"
#define HHC00386 "Collected %u bytes of garbage for CCKD%s file[%d] %1d:%04X %s..."
#define HHC00387 "%1d:%04X CCKD%s image %s is SEVERELY fragmented!"
#define HHC00388 "%1d:%04X CCKD%s image %s is moderately fragmented"
#define HHC00389 "%1d:%04X CCKD%s image %s is slightly fragmented"
#define HHC00390 "%1d:%04X CCKD file: device has no shadow files"
//efine HHC00391 (available)
//efine HHC00392 (available)
//efine HHC00393 (available)
//efine HHC00394 (available)
//efine HHC00395 (available)
#define HHC00396 "%1d:%04X %s" // (cckd_trace)
//efine HHC00397 (available)
#define HHC00398 "%s" // (trace table)
#define HHC00399 "CCKD file: internal cckd trace"

// reserve 004xx for ckd dasd device related
#define HHC00400 "%1d:%04X CKD file: name missing or invalid filename length"
#define HHC00401 "%1d:%04X CKD file %s: open error: not found"
#define HHC00402 "%1d:%04X CKD file: parameter %s in argument %d is invalid"
#define HHC00403 "%1d:%04X %s file %s: opened r/o%s"
#define HHC00404 "%1d:%04X CKD file %s: error in function %s: %s"
#define HHC00405 "%1d:%04X CKD file %s: only one base file is allowed"
#define HHC00406 "%1d:%04X CKD file %s: ckd header invalid"
#define HHC00407 "%1d:%04X %s file %s: only 1 CCKD file allowed"
#define HHC00408 "%1d:%04X %s file %s: ckd file out of sequence or bad size"
#define HHC00409 "%1d:%04X %s file %s: seq %02d cyls %6d-%-6d"
#define HHC00410 "%1d:%04X %s file %s: found heads %d trklen %d, expected heads %d trklen %d"
#define HHC00411 "%1d:%04X %s file %s: ckd header inconsistent with file size"
#define HHC00412 "%1d:%04X %s file %s: ckd header high cylinder incorrect"
#define HHC00413 "%1d:%04X %s file %s: maximum CKD files exceeded: %d"
#define HHC00414 "%1d:%04X %s file %s: model %s cyls %d heads %d tracks %d trklen %d"
#define HHC00415 "%1d:%04X CKD file %s: device type %4.4X not found in dasd table"
#define HHC00416 "%1d:%04X %s file %s: control unit %s not found in dasd table"
#define HHC00417 "%1d:%04X CKD file %s: cache hits %d, misses %d, waits %d"
#define HHC00418 "%1d:%04X CKD file %s: invalid track header for cyl %d head %d %02X %02X%02X %02X%02X"
#define HHC00419 "%1d:%04X CKD file %s: error attempting to read past end of track %d %d"
#define HHC00420 "%1d:%04X CKD file %s: error write kd orientation"
#define HHC00421 "%1d:%04X CKD file %s: error write data orientation"
#define HHC00422 "%1d:%04X CKD file %s: data chaining not supported for non-read CCW %02X"
// LOGDEVTR...
#define HHC00423 "Thread "TIDPAT" %1d:%04X CKD file %s: search key %s"
#define HHC00424 "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d cur trk %d"
#define HHC00425 "Thread "TIDPAT" %1d:%04X CKD file %s: read track updating track %d"
#define HHC00426 "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d cache hit, using cache[%d]"
#define HHC00427 "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d no available cache entry, waiting"
#define HHC00428 "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d cache miss, using cache[%d]"
#define HHC00429 "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d reading file %d offset %"PRId64" len %d"
#define HHC00430 "Thread "TIDPAT" %1d:%04X CKD file %s: read trk %d trkhdr %02X %02X%02X %02X%02X"
#define HHC00431 "Thread "TIDPAT" %1d:%04X CKD file %s: seeking to cyl %d head %d"
#define HHC00432 "Thread "TIDPAT" %1d:%04X CKD file %s: error: MT advance: locate record %d file mask %02X"
#define HHC00433 "Thread "TIDPAT" %1d:%04X CKD file %s: MT advance to cyl(%d) head(%d)"
#define HHC00434 "Thread "TIDPAT" %1d:%04X CKD file %s: read count orientation %s"
#define HHC00435 "Thread "TIDPAT" %1d:%04X CKD file %s: cyl %d head %d record %d kl %d dl %d of %d"
#define HHC00436 "Thread "TIDPAT" %1d:%04X CKD file %s: read key %d bytes"
#define HHC00437 "Thread "TIDPAT" %1d:%04X CKD file %s: read data %d bytes"
#define HHC00438 "Thread "TIDPAT" %1d:%04X CKD file %s: writing cyl %d head %d record %d kl %d dl %d"
#define HHC00439 "Thread "TIDPAT" %1d:%04X CKD file %s: setting track overflow flag for cyl %d head %d record %d"
#define HHC00440 "Thread "TIDPAT" %1d:%04X CKD file %s: updating cyl %d head %d record %d kl %d dl %d"
#define HHC00441 "Thread "TIDPAT" %1d:%04X CKD file %s: updating cyl %d head %d record %d dl %d"
#define HHC00442 "Thread "TIDPAT" %1d:%04X CKD file %s: set file mask %02X"
#define HHC00443 "%1d:%04X CKD file: 'fakewrite' invalid without 'readonly'"
//efine HHC00444 (available)
#define HHC00445 "%1d:%04X CKD file %s: updating cyl %d head %d"
#define HHC00446 "%1d:%04X CKD file %s: write track error: stat %2.2X"
#define HHC00447 "%1d:%04X CKD file %s: reading cyl %d head %d"
#define HHC00448 "%1d:%04X CKD file %s: read track error: stat %2.2X"
#define HHC00449 "%1d:%04X CKD file %s: searching extent %d begin %d,%d end %d,%d"
#define HHC00450 "Track %d not found in extent table"
#define HHC00451 "%1d:%04X CKD file %s: DASD table entry not found for devtype 0x%2.2X"
#define HHC00452 "%1d:%04X CKD file %s: initialization failed"
#define HHC00453 "%1d:%04X CKD file %s: heads %d trklen %d"
#define HHC00454 "%1d:%04X FBA file %s: sectors %d size %d"
#define HHC00455 "%1d:%04X CKD file %s: %s record not found"
#define HHC00456 "%1d:%04X CKD file %s: VOLSER %s VTOC %4.4X%4.4X%2.2X"
#define HHC00457 "%1d:%04X CKD file %s: VTOC start %2.2X%2.2X%2.2X%2.2X end %2.2X%2.2X%2.2X%2.2X"
#define HHC00458 "%1d:%04X CKD file %s: dataset %s not found in VTOC"
#define HHC00459 "%1d:%04X CKD file %s: DSNAME %s F1DSCB CCHHR=%4.4X%4.4X%2.2X"
#define HHC00460 "%1d:%04X CKD file %s: %u %s successfully written"
#define HHC00461 "%1d:%04X CKD file %s: %s count %u is outside range %u-%u"
#define HHC00462 "%1d:%04X CKD file %s: creating %4.4X volume %s: %u cyls, %u trks/cyl, %u bytes/track"
#define HHC00463 "%1d:%04X FBA file %s: creating %4.4X volume %s: %u sectors, %u bytes/sector"
#define HHC00464 "%1d:%04X CKD file %s: file size too large: %"PRIu64" [%d]"
#define HHC00465 "%1d:%04X FBA file %s: creating %4.4X compressed volume %s: %u sectors, %u bytes/sector"
#define HHC00466 "Maximum of %u %s in %u 2GB file(s) is supported"
#define HHC00467 "Maximum %s supported is %u"
#define HHC00468 "For larger capacity DASD volumes, use %s"
#define HHC00469 "%1d:%04X %s file %s: shadow files not supported for %s dasd"
#define HHC00470 "%1d:%04X %s file %s: model %s cyls %d heads %d tracks %d trklen %d"
#define HHC00471 "%1d:%04X CKD64 file %s: %u %s successfully written"
#define HHC00472 "%1d:%04X CKD64 file %s: creating %4.4X volume %s: %u cyls, %u trks/cyl, %u bytes/track"
#define HHC00473 "%1d:%04X FBA64 file %s: creating %4.4X volume %s: %u sectors, %u bytes/sector"
#define HHC00474 "%1d:%04X FBA64 file %s: creating %4.4X compressed volume %s: %u sectors, %u bytes/sector"
#define HHC00475 "This might take a while... Please wait..."
#define HHC00476 "%1d:%04X %s file %s: opened r/o%s"
//efine HHC00477 - HHC00499 (available)

// reserve 005xx for fba dasd device related messages
#define HHC00500 "%1d:%04X FBA file: name missing or invalid filename length"
#define HHC00501 "%1d:%04X FBA file %s not found or invalid"
#define HHC00502 "%1d:%04X FBA file %s: error in function %s: %s"
#define HHC00503 "%1d:%04X %s file: parameter %s in argument %d is invalid"
#define HHC00504 "%1d:%04X FBA file %s: REAL FBA opened"
#define HHC00505 "%1d:%04X %s file %s: invalid device origin block number %s"
#define HHC00506 "%1d:%04X %s file %s: invalid device block count %s"
#define HHC00507 "%1d:%04X %s file %s: origin %"PRId64", blks %d"
#define HHC00508 "%1d:%04X %s file %s: device type %4.4X not found in dasd table"
#define HHC00509 "%1d:%04X FBA file %s: define extent data too short: %d bytes"
#define HHC00510 "%1d:%04X FBA file %s: second define extent in chain"
#define HHC00511 "%1d:%04X FBA file %s: invalid file mask %2.2X"
#define HHC00512 "%1d:%04X FBA file %s: invalid extent: first block %d last block %d numblks %d device size %d"
#define HHC00513 "%1d:%04X FBA file %s: FBA origin mismatch: %d, expected %d,"
#define HHC00514 "%1d:%04X FBA file %s: FBA numblk mismatch: %d, expected %d,"
#define HHC00515 "%1d:%04X FBA file %s: FBA blksiz mismatch: %d, expected %d,"
// LOGDEVTR...
#define HHC00516 "Thread "TIDPAT" %1d:%04X FBA file %s: read blkgrp %d cache hit, using cache[%d]"
#define HHC00517 "Thread "TIDPAT" %1d:%04X FBA file %s: read blkgrp %d no available cache entry, waiting"
#define HHC00518 "Thread "TIDPAT" %1d:%04X FBA file %s: read blkgrp %d cache miss, using cache[%d]"
#define HHC00519 "Thread "TIDPAT" %1d:%04X FBA file %s: read blkgrp %d offset %"PRId64" len %d"
#define HHC00520 "Thread "TIDPAT" %1d:%04X FBA file %s: positioning to 0x%"PRIX64" %"PRId64
#define HHC00521 "Maximum of %u %s in a 2GB file"
//efine HHC00522 - HHC00599 (available)

// scedasd.c
#define HHC00600 "SCE file %s: error in function %s: %s"
#define HHC00601 "SCE file %s: load from file failed: %s"
#define HHC00602 "SCE file %s: load from path failed: %s"
#define HHC00603 "SCE file %s: load main terminated at end of mainstor"
#define HHC00604 "SCE file %s: access error on image %s: %s"
#define HHC00605 "SCE file %s: access error: %s"
//efine HHC00606 - HHC00649 (available)

// scescsi.c
#define HHC00650 "%s open error: %s"
#define HHC00651 "Loading %s"
#define HHC00652 "Hardware loader %s: %s"
#define HHC00653 "Hardware loader file type %d not supported"
#define HHC00654 "Unknown hardware loader request type %2.2X"
#define HHC00655 "Store Status save to HSA failed"
#define HHC00656 "Cannot load bootstrap loader %s: %s"
#define HHC00657 "Invalid file %s"
#define HHC00658 "I/O error on read(): rc=%d: \"%s\""
#define HHC00659 "%s is outside of main storage"
#define HHC00660 "%-8s %s"
#define HHC00661 "Hardware loader: %s request: SCCB = 0x%"PRIX64
//efine HHC00662 - HHC00669 (available)
#define HHC00670 "Invalid %s"
#define HHC00671 "Invalid option %s"
//efine HHC00672 - HHC00679 (available)
#define HHC00680 "portname %16.16"PRIx64
#define HHC00681 "lun      %16.16"PRIx64
#define HHC00682 "bootprog %8.8x"
#define HHC00683 "br_lba   %16.16"PRIx64
#define HHC00684 "scpdata  %s"
//efine HHC00685 - HHC00699 (available)

// reserve 007xx for shared device related messages
#define HHC00700 "Shared: parameter %s in argument %d is invalid"
#define HHC00701 "%1d:%04X Shared: connect pending to file %s"
#define HHC00702 "%1d:%04X Shared: error retrieving cylinders"
#define HHC00703 "%1d:%04X Shared: error retrieving device characteristics"
#define HHC00704 "%1d:%04X Shared: remote device %04X is a %04X"
#define HHC00705 "%1d:%04X Shared: error retrieving device id"
#define HHC00706 "%1d:%04X Shared: device type %4.4X not found in dasd table"
#define HHC00707 "%1d:%04X Shared: control unit %s not found in dasd table"
#define HHC00708 "%1d:%04X Shared: file %s: model %s cyls %d heads %d tracks %d trklen %d"
#define HHC00709 "%1d:%04X Shared: error retrieving fba origin"
#define HHC00710 "%1d:%04X Shared: error retrieving fba number blocks"
#define HHC00711 "%1d:%04X Shared: error retrieving fba block size"
#define HHC00712 "%1d:%04X Shared: file %s: model %s origin %"PRId64" blks %d"
#define HHC00713 "%1d:%04X Shared: error during channel program start"
#define HHC00714 "%1d:%04X Shared: error during channel program end"
#define HHC00715 "%1d:%04X Shared: remote error reading track %d"
#define HHC00716 "%1d:%04X Shared: error retrieving serial number"
#define HHC00717 "%1d:%04X Shared: error retrieving usage information"
#define HHC00718 "%1d:%04X Shared: error writing track %d"
#define HHC00719 "%1d:%04X Shared: remote error writing track %d %2.2X-%2.2X"
#define HHC00720 "%1d:%04X Shared: error in function %s: %s"
#define HHC00721 "%1d:%04X Shared: connected to v%d.%d server id %d file %s"
#define HHC00722 "%1d:%04X Shared: error in connect to file %s: %s"
#define HHC00723 "%1d:%04X Shared: error in send for %2.2X-%2.2X: %s"
#define HHC00724 "%1d:%04X Shared: not connected to file %s"
#define HHC00725 "%1d:%04X Shared: error in receive: %s"
#define HHC00726 "%1d:%04X Shared: remote error %2.2X-%2.2X: %s"
#define HHC00727 "Shared: decompress error %d offset %d length %d"
#define HHC00728 "Shared: data compressed using method %s is unsupported"
#define HHC00729 "%1d:%04X Shared: error in send id %d: %s"
#define HHC00730 "%1d:%04X Shared: busy client being removed id %d %s"
#define HHC00731 "%1d:%04X Shared: %s disconnected id %d"
#define HHC00732 "Shared: connect to IP %s failed"
#define HHC00733 "%1d:%04X Shared: %s connected id %d"
#define HHC00734 "%1d:%04X Shared: error in receive from %s id %d"
#define HHC00735 "Shared: error in function %s: %s"
#define HHC00736 "Shared: waiting for port %u to become free"
#define HHC00737 "Shared: waiting for shared device requests on port %u"
#define HHC00738 "Shared: invalid or missing argument"
#define HHC00739 "Shared: invalid or missing keyword"
#define HHC00740 "Shared: invalid or missing value %s"
#define HHC00741 "Shared: invalid or missing keyword %s"
#define HHC00742 "Shared: OPTION_SHARED_DEVICES not defined"
#define HHC00743 "Shared:  %s" // (trace message)
#define HHC00744 "Shared: Server already active"
#define HHC00745 "%1d:%04X Shared: CKD file: 'fakewrite' invalid without 'readonly'"
//efine HHC00746 - HHC00799 (available)

// reserve 008xx for processor related messages
#define HHC00800 "Processor %s%02X: loaded wait state PSW %s"
#define HHC00801 "Processor %s%02X: %s%s%s interruption code %4.4X ilc %d%s%s"
#define HHC00802 "Processor %s%02X: PER event: code %4.4X perc %2.2X=%s addr "F_VADR
#define HHC00803 "Processor %s%02X: program interrupt loop PSW %s"
#define HHC00804 "Processor %s%02X: I/O interrupt code %1.1X:%4.4X CSW %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X"
#define HHC00805 "Processor %s%02X: I/O interrupt code %8.8X parm %8.8X"
#define HHC00806 "Processor %s%02X: I/O interrupt code %8.8X parm %8.8X id %8.8X"
#define HHC00807 "Processor %s%02X: machine check code %16.16"PRIu64
#define HHC00808 "Processor %s%02X: store status completed"
#define HHC00809 "Processor %s%02X: disabled wait state %s"
#define HHC00810 "Processor %s%02X: ipl failed: %s"
#define HHC00811 "Processor %s%02X: architecture mode %s"
#define HHC00812 "Processor %s%02X: vector facility online"
#define HHC00813 "Processor %s%02X: error in function %s: %s"
#define HHC00814 "Processor %s: CC %d%s"
#define HHC00815 "Processors %s%02X through %s%02X are offline"
#define HHC00816 "Processor %s%02X: processor is not %s"
#define HHC00817 "Processor %s%02X: store status completed"
#define HHC00818 "Processor %s%02X: not eligible for ipl nor restart"
#define HHC00819 "Processor %s%02X: online"
#define HHC00820 "Processor %s%02X: offline"
#define HHC00821 "Processor %s%02X: vector facility configured %s"
#define HHC00822 "PROCESSOR %s%02X APPEARS TO BE HUNG!"
#define HHC00823 "You have %d seconds to attach a debugger before crash dump will be taken!"
#define HHC00824 "Debugger attached! NOT crashing!"
#define HHC00825 "TIME'S UP! (or debugger has been detached!) - Forcing crash dump!"
#define HHC00826 "Processor %s%02X: processor %sstopped due to disabled wait"
#define HHC00827 "Processor %s%02X: engine %02X type %1d set: %s"
#define HHC00828 "Processor %s%02X: ipl failed: %s" // (IPL I/O error)
#define HHC00829 "Command cannot be issued once system has been IPLed"
#define HHC00830 "System cannot be IPLed once shadow file commands have been issued"
#define HHC00831 "Hercules needs to be restarted before proceeding"
//efine HHC00832 (available)
//efine HHC00833 (available)
#define HHC00834 "Processor %s%02X: %s"
#define HHC00835 "Processor %s%02X: aie invalidated; instruction being refetched"
//efine HHC00836 (available)
//efine HHC00837 (available)
//efine HHC00838 (available)
#define HHC00839 "Processor %s%02X: ipl failed: %s" // (bad IPL PSW)
#define HHC00840 "Processor %s%02X: External interrupt: interrupt key"
#define HHC00841 "Processor %s%02X: External interrupt: clock comparator"
#define HHC00842 "Processor %s%02X: External interrupt: CPU timer=%16.16"PRIX64
#define HHC00843 "Processor %s%02X: External interrupt: interval timer"
#define HHC00844 "Processor %s%02X: %1d:%04X: processing block I/O interrupt: code %4.4X parm %16.16"PRIX64" status %2.2X subcode %2.2X"
#define HHC00845 "Processor %s%02X: External interrupt: block I/O %s"
#define HHC00846 "Processor %s%02X: External interrupt: service signal %8.8X"
//efine HHC00847 - HHC00849 (available)
#define HHC00850 "Processor %s%02X: CPUint=%8.8X (State:%8.8X)&(Mask:%8.8X)"
#define HHC00851 "Processor %s%02X: interrupt %spending"
#define HHC00852 "Processor %s%02X: I/O interrupt %spending"
#define HHC00853 "Processor %s%02X: clock comparator %spending"
#define HHC00854 "Processor %s%02X: CPU timer %spending"
#define HHC00855 "Processor %s%02X: interval timer %spending"
#define HHC00856 "Processor %s%02X: ECPS vtimer %spending"
#define HHC00857 "Processor %s%02X: external call %spending"
#define HHC00858 "Processor %s%02X: emergency signal %spending"
#define HHC00859 "Processor %s%02X: machine check interrupt %spending"
#define HHC00860 "Processor %s%02X: service signal %spending"
#define HHC00861 "Processor %s%02X: mainlock held: %s"
#define HHC00862 "Processor %s%02X: intlock held: %s"
#define HHC00863 "Processor %s%02X: waiting for intlock: %s"
#define HHC00864 "Processor %s%02X: lock %sheld"
#define HHC00865 "Processor %s%02X: connected to channelset %s"
#define HHC00866 "Processor %s%02X: state %s"
#define HHC00867 "Processor %s%02X: instcount %"PRId64
#define HHC00868 "Processor %s%02X: siocount %"PRId64
#define HHC00869 "Processor %s%02X: psw %s"
#define HHC00870 "config mask "F_CPU_BITMAP" started mask "F_CPU_BITMAP" waiting mask "F_CPU_BITMAP
#define HHC00871 "syncbc mask "F_CPU_BITMAP" %s"
#define HHC00872 "signaling facility %sbusy"
#define HHC00873 "TOD lock %sheld"
#define HHC00874 "mainlock %sheld; owner %4.4x"
#define HHC00875 "intlock %sheld; owner %4.4x"
#define HHC00876 "ioq lock %sheld"
//efine HHC00877 (available)
//efine HHC00878 (available)
//efine HHC00879 (available)
#define HHC00880 "device %1d:%04X: status %s"
#define HHC00881 "I/O interrupt queue:%s"
#define HHC00882 "device %1d:%04X: %s%s%s%s pri ISC %02X CSS %02X CU %02X"
#define HHC00883 "Channel Report queue: (NULL)"
#define HHC00884 "Channel Report queue: (empty)"
#define HHC00885 "Channel Report queue:"
#define HHC00886 "CRW 0x%8.8X: %s"
//efine HHC00887 (available)
//efine HHC00888 (available)
#define HHC00889 "Available facilities cannot be changed once system is IPLed"
#define HHC00890 "Cannot %s facility %s without first %s facility %s"
#define HHC00891 "%3d %02X %02X %c%c%c%c%c %-27s%c%s"
#define HHC00892 "Facility( %s ) does not exist for %s"
#define HHC00893 "Facility( %s ) not supported for %s"
#define HHC00894 "%s facility list: %s"
#define HHC00895 "ARCHLVL '%s' is invalid"
#define HHC00896 "Facility( %s ) not supported for %s"
#define HHC00897 "Facility( %s ) is required for %s"
#define HHC00898 "Facility( %s ) %s%s for %s"
#define HHC00899 "%s facility %s fails consistency check"

// reserve 009xx for ctc related messages
/* ctcadpt.c, ctc_ctci.c, ctc_lcs.c, ctc_ptp.c, qeth.c */
#define HHC00900 "%1d:%04X %s: Error in function %s: %s"
#define HHC00901 "%1d:%04X %s: Interface %s, type %s opened"
#define HHC00902 "%1d:%04X %s: ioctl %s failed for device %s: %s"
#define HHC00903 "%1d:%04X: CTC DEBUG is %s"
#define HHC00904 "%1d:%04X %s: Halt or clear recognized"
//efine HHC00905 (available)
#define HHC00906 "%1d:%04X CTC: write CCW count %u is invalid"
#define HHC00907 "%1d:%04X CTC: interface command: %s %8.8X"
#define HHC00908 "%1d:%04X CTC: incomplete write buffer segment header at offset %4.4X"
#define HHC00909 "%1d:%04X CTC: invalid write buffer segment length %u at offset %4.4X"
#define HHC00910 "%1d:%04X %s: Send%s packet of size %d bytes to device %s"
#define HHC00911 "%1d:%04X %s: Error writing to device %s: %d %s"
#define HHC00912 "%1d:%04X %s: Error reading from device %s: %d %s"
#define HHC00913 "%1d:%04X %s: Receive%s packet of size %d bytes from device %s"
#define HHC00914 "%1d:%04X CTC: packet frame too big, dropped"
#define HHC00915 "%1d:%04X %s: Incorrect number of parameters"
#define HHC00916 "%1d:%04X %s: Option %s value %s invalid"
#define HHC00917 "%1d:%04X %s: Required parameter '%s' missing"
#define HHC00918 "%1d:%04X %s: Option %s unknown or specified incorrectly"
//efine HHC00919 (available)
#define HHC00920 "%1d:%04X CTC: lcs device %04X not in configuration"
#define HHC00921 "CTC: lcs device port %2.2X: %s Multicast assist enabled"
#define HHC00922 "%1d:%04X CTC: lcs command packet received"
#define HHC00923 "%1d:%04X CTC: lcs command reply enqueue"
//efine HHC00924 - HHC00932 (available)
#define HHC00933 "%1d:%04X CTC: executing command %s"
#define HHC00934 "%1d:%04X CTC: sending packet to file %s"
#define HHC00935 "CTC: lcs device port %2.2X: %s Checksum Offload enabled"
#define HHC00936 "%1d:%04X CTC: error writing to file %s: %s"
#define HHC00937 "%1d:%04X CTC: lcs write: unsupported frame type 0x%2.2X"
#define HHC00938 "CTC: lcs device port %2.2X: %s Large Send Offload enabled"
#define HHC00939 "%1d:%04X CTC: lcs startup: frame buffer size 0x%4.4X %s compiled size 0x%4.4X: ignored"
#define HHC00940 "CTC: error in function %s: %s"
#define HHC00941 "CTC: ioctl %s failed for device %s: %s; ... ignoring and continuing"
#define HHC00942 "CTC: lcs interface %s using mac %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X"
#define HHC00943 "CTC: lcs interface %s not using mac %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X"
#define HHC00944 "CTC: lcs interface %s read error from port %2.2X: %s"
#define HHC00945 "CTC: lcs device port %2.2X: MCAST not in table, discarding frame"
#define HHC00946 "CTC: lcs device port %2.2X: IPv4 frame received for %s"
#define HHC00947 "CTC: lcs device port %2.2X: ARP frame received for %s"
#define HHC00948 "CTC: lcs device port %2.2X: RARP frame received for %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X"
#define HHC00949 "CTC: lcs device port %2.2X: SNA frame received for %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X"
#define HHC00950 "CTC: lcs device port %2.2X: no match found, selecting %s %4.4X"
#define HHC00951 "CTC: lcs device port %2.2X: no match found, discarding frame"
#define HHC00952 "CTC: lcs device port %2.2X: enqueing frame to device %4.4X %s"
#define HHC00953 "CTC: lcs device port %2.2X: packet frame too big, dropped"
#define HHC00954 "CTC: invalid statement %s in file %s: %s"
#define HHC00955 "CTC: invalid %s %s in statement %s in file %s: %s"
#define HHC00956 "CTC: error in file %s: missing device number or mode"
#define HHC00957 "CTC: error in file %s: invalid %s value %s"
#define HHC00958 "CTC: error in file %s: %s: missing port number"
#define HHC00959 "CTC: error in file %s: %s: invalid entry starting at %s"
#define HHC00960 "CTC: error in file %s: %s: SNA does not accept any arguments"
#define HHC00961 "CTC: error in file %s: %s: invalid mode"
#define HHC00962 "CTC: error in file %s: reading line %d: %s"
#define HHC00963 "CTC: error in file %s: line %d is too long"
#define HHC00964 "CTC: lcs device '%s' port %2.2X: %s %s: ok"
#define HHC00965 "CTC: lcs device port %2.2X: STILL trying to enqueue frame to device %4.4X %s"
#define HHC00966 "%1d:%04X CTC: lcs triggering port %2.2X event"
#define HHC00967 "CTC: lcs device port %2.2X: read thread: waiting for start event"
#define HHC00968 "CTC: lcs device port %2.2X: read thread: port started"
#define HHC00969 "CTC: lcs device port %2.2X: read thread: port stopped"
#define HHC00970 "CTC: lcs device port %2.2X: 802.2 LLC error, discarding frame"
#define HHC00971 "%1d:%04X CTC: connect to %s:%s failed, starting server"
#define HHC00972 "%1d:%04X CTC: connected to %s:%s"
#define HHC00973 "%1d:%04X CTC: error reading from file %s: %s"
#define HHC00974 "%1d:%04X CTC: incorrect client or config error: config file %s connecting client %s"
//efine HHC00975 (available)
//efine HHC00976 (available)
#define HHC00977 "%1d:%04X CTC: lcs command packet ignored (bInitiator == LGW)"
#define HHC00978 "CTC: lcs device port %2.2X: STILL trying to enqueue REPLY frame to device %4.4X %s"
#define HHC00979 "%s: %s: %s %s %s"
#define HHC00980 "%1d:%04X %s: Data of size %d bytes displayed, data of size %d bytes not displayed"
#define HHC00981 "%1d:%04X %s: Accept data of size %d bytes from guest"
#define HHC00982 "%1d:%04X %s: Present data of size %d bytes to guest"
#define HHC00983 "%1d:%04X %s: port %2.2X: Send frame of size %d bytes (with %s packet) to device %s"
#define HHC00984 "%1d:%04X %s: port %2.2X: Receive frame of size %d bytes (with %s packet) from device %s"
#define HHC00985 "%1d:%04X %s: Send frame of size %d bytes (with %s packet) to device %s"
#define HHC00986 "%1d:%04X %s: Receive frame of size %d bytes (with %s packet) from device %s"
//efine HHC00987 - HHC00999 (available)

// reserve 010xx for communication adapter specific component messages
#define HHC01000 "%1d:%04X COMM: error in function %s: %s"
#define HHC01001 "%1d:%04X COMM: connect out to %s:%d failed during initial status: %s"
#define HHC01002 "%1d:%04X COMM: cannot obtain socket for incoming calls: %s"
#define HHC01003 "%1d:%04X COMM: waiting 5 seconds for port %d to become available"
#define HHC01004 "%1d:%04X COMM: listening on port %d for incoming TCP connections"
#define HHC01005 "%1d:%04X COMM: outgoing call failed during %s command: %s"
#define HHC01006 "%1d:%04X COMM: incoming call"
#define HHC01007 "%1d:%04X COMM: option %s value %s invalid"
#define HHC01008 "%1d:%04X COMM: missing parameter: DIAL(%s) and %s not specified"
#define HHC01009 "%1d:%04X COMM: conflicting parameter: DIAL(%s) and %s=%s specified"
#define HHC01010 "%1d:%04X COMM: RPORT parameter ignored"
#define HHC01011 "%1d:%04X COMM: initialization not performed"
#define HHC01012 "%1d:%04X COMM: error parsing %s"
#define HHC01013 "%1d:%04X COMM: incorrect switched/dial specification %s: defaulting to DIAL=OUT"
#define HHC01014 "%1d:%04X COMM: initialization failed due to previous errors"
#define HHC01015 "%1d:%04X COMM: BSC communication thread did not initialize"
#define HHC01016 "COMM: unable to determine %s from %s"
#define HHC01017 "COMM: invalid parameter %s"
#define HHC01018 "%1d:%04X COMM: client %s devtype %4.4X: connected"
#define HHC01019 "%1d:%04X COMM: unrecognized parameter %s"
#define HHC01020 "%1d:%04X COMM: no buffers trying to send %s"
//efine HHC01021 (available)
#define HHC01022 "%1d:%04X COMM: client %s devtype %4.4X: connection closed by client"
#define HHC01023 "Waiting for port %u to become free for console connections"
#define HHC01024 "Waiting for %sconsole connections on port %u"
#define HHC01025 "%1d:%04X COMM: duplicate SYSG console definition"
#define HHC01026 "%1d:%04X COMM: enter console input"
#define HHC01027 "Hercules version %s, built on %s %s"
#define HHC01028 "Connection rejected, no available %s device"
#define HHC01029 "Connection rejected, no available %s device in the %s group"
#define HHC01030 "Connection rejected, device %04X unavailable"
#define HHC01031 "Running on %s (%s-%s.%s %s %s)"
#define HHC01032 "COMM: this Hercules build does not support Unix domain sockets"
#define HHC01033 "COMM: error: socket pathname %s exceeds limit %d"
#define HHC01034 "COMM: error in function %s: %s"
#define HHC01035 "COMM: failed to determine IP address from node %s"
#define HHC01036 "COMM: failed to determine port number from service %s"
#define HHC01037 "%1d:%04X COMM: client %s, IP %s connection to device %s rejected: device busy or interrupt pending"
#define HHC01038 "%1d:%04X COMM: client %s, IP %s connection to device %s rejected: client %s ip %s still connected"
#define HHC01039 "%1d:%04X COMM: client %s, IP %s connection to device %s rejected: by onconnect callback"
#define HHC01040 "%1d:%04X COMM: client %s, IP %s connected to device %s"
#define HHC01041 "%1d:%04X COMM: error: device already bound to socket %s"
#define HHC01042 "%1d:%04X COMM: device bound to socket %s"
#define HHC01043 "%1d:%04X COMM: device not bound to any socket"
#define HHC01044 "%1d:%04X COMM: client %s, IP %s disconnected from device %s"
#define HHC01045 "%1d:%04X COMM: client %s, IP %s still connected to device %s"
#define HHC01046 "%1d:%04X COMM: device unbound from socket %s"
#define HHC01047 "COMM: connect message sent: %s"
#define HHC01048 "%1d:%04X COMM: %s"
#define HHC01049 "%1d:%04X COMM: %s: dump of %u (0x%04x) byte(s)"
#define HHC01050 "%1d:%04X COMM: %s: %s"
#define HHC01051 "%1d:%04X COMM: %s"
#define HHC01052 "%1d:%04X COMM: clean: %s"
#define HHC01053 "%1d:%04X COMM: received telnet command 0x%02x 0x%02x"
#define HHC01054 "%1d:%04X COMM: sending telnet command 0x%02x 0x%02x"
#define HHC01055 "%1d:%04X COMM: received telnet IAC 0x%02x"
#define HHC01056 "%1d:%04X COMM: posted %d input bytes"
#define HHC01057 "%1d:%04X COMM: raised attention, return code %d"
#define HHC01058 "%1d:%04X COMM: initialization starting"
#define HHC01059 "%1d:%04X COMM: initialization: control block allocated"
#define HHC01060 "%1d:%04X COMM: closing down"
#define HHC01061 "%1d:%04X COMM: closed down"
#define HHC01062 "%1d:%04X COMM: %s: %s %s %-6.6s %s"
#define HHC01063 "%1d:%04X COMM: CCW exec - entry code %x"
#define HHC01064 "%1d:%04X COMM: %s: status text=%s, trans=%s, tws=%s"
#define HHC01065 "Ring buffer for ring %p at %p %s"
#define HHC01066 "%1d:%04X COMM: found data beyond EON"
#define HHC01067 "%1d:%04X COMM: found incorrect IP address section at position %d"
#define HHC01068 "%1d:%04X COMM: %d (0x%04X) greater than 255 (0xFE)"
#define HHC01069 "%1d:%04X COMM: too many separators in dial data"
#define HHC01070 "%1d:%04X COMM: incorrect dial data byte %02X"
#define HHC01071 "%1d:%04X COMM: not enough separator in dial data, %d found"
#define HHC01072 "%1d:%04X COMM: destination tcp port %d exceeds maximum of 65535"
#define HHC01073 "%s:%d terminal connected cua %04X term %s"
#define HHC01074 "%1d:%04X COMM: cthread - entry - devexec %s"
#define HHC01075 "%1d:%04X COMM: poll command abort, poll address > 7 bytes"
#define HHC01076 "%1d:%04X COMM: writing byte %02X in socket"
#define HHC01077 "%1d:%04X COMM: cthread - select in maxfd %d / devexec %s"
#define HHC01078 "%1d:%04X COMM: cthread - select out return code %d"
#define HHC01079 "%1d:%04X COMM: cthread - select time out"
#define HHC01080 "%1d:%04X COMM: cthread - ipc pipe closed"
#define HHC01081 "%1d:%04X COMM: cthread - ipc pipe data: code %d"
#define HHC01082 "%1d:%04X COMM: cthread - inbound socket data"
#define HHC01083 "%1d:%04X COMM: cthread - socket write available"
#define HHC01084 "%1d:%04X COMM: set mode %s"
#define HHC01085 "%1d:%04X COMM: default command prefixes exhausted"
#define HHC01086 "%1d:%04X COMM: device %1d:%04X already using prefix '%s'"
//efine HHC01087 (available)
//efine HHC01088 (available)
//efine HHC01089 (available)
#define HHC01090 "%1d:%04X COMM: client %s devtype %4.4X: connection reset"
#define HHC01091 "%1d:%04X COMM: %s has an invalid GROUP name length or format; must be a valid luname or poolname"
#define HHC01092 "%1d:%04X COMM: Not all TCP keepalive settings honored"
#define HHC01093 "%1d:%04X COMM: Keepalive: (%d,%d,%d)"
#define HHC01094 "%1d:%04X COMM: This build of Hercules does not support TCP keepalive"
#define HHC01095 "%1d:%04X COMM: This build of Hercules has only basic TCP keepalive support"
#define HHC01096 "%1d:%04X COMM: This build of Hercules has only partial TCP keepalive support"
//efine HHC01097 (available)
//efine HHC01098 (available)
//efine HHC01099 (available)

// reserve 011xx for printer specific component messages
#define HHC01100 "%1d:%04X Printer: client %s, IP %s disconnected from device %s"
#define HHC01101 "%1d:%04X Printer: file name missing or invalid"
#define HHC01102 "%1d:%04X Printer: argument %d parameter '%s' is invalid"
#define HHC01103 "%1d:%04X Printer: argument %d parameter '%s' position %d is invalid"
#define HHC01104 "%1d:%04X Printer: option %s is incompatible"
#define HHC01105 "%1d:%04X Printer: unsupported device type %04X"
#define HHC01106 "%1d:%04X Printer: pipe receiver with pid %d starting"
#define HHC01107 "%1d:%04X Printer: pipe receiver with pid %d terminating"
#define HHC01108 "%1d:%04X Printer: unable to execute file %s: %s"
#define HHC01109 "%1d:%04X Printer: %s incompatible with device type %04X"
#define HHC01110 "%1d:%04X Printer: Indexing accepted but ignored for 3203-5"
#define HHC01111 "%1d:%04X Printer: channel 1 is undefined"
#define HHC01112 "%1d:%04X Printer: invalid fcb: maximum channel codes exceeded"
#define HHC01113 "%1d:%04X Printer: incompatible '%s' and 'lpp' values detected"
//efine HHC01114 - HHC01159 (available)

// range   01150 - 01199 available

// reserve 012xx for card devices
//efine HHC01200 (available)
#define HHC01201 "%1d:%04X Card: filename %s too long, maximum length is %d"
#define HHC01202 "%1d:%04X Card: options ascii and ebcdic are mutually exclusive"
#define HHC01203 "%1d:%04X Card: only one filename (sock_spec) allowed for socket device"
#define HHC01204 "%1d:%04X Card: option ascii is default for socket device"
#define HHC01205 "%1d:%04X Card: option multifile ignored: only one file specified"
#define HHC01206 "%1d:%04X Card: client %s, IP %s disconnected from device %s"
#define HHC01207 "%1d:%04X Card: file %s: card image exceeds maximum %d bytes"
#define HHC01208 "%1d:%04X Card: filename is missing"
#define HHC01209 "%1d:%04X Card: parameter %s in argument %d is invalid"
#define HHC01210 "%1d:%04X Card: option %s is incompatible"
#define HHC01211 "%1d:%04X Card: client %s, IP %s disconnected from device %s"
//efine HHC01212 - HHC01249 (available)

// reserve 01250 - 01299 for Generic device messages
#define HHC01250 "%1d:%04X %s: error in function %s: %s"
#define HHC01251 "%1d:%04X %s: option '%s' has been deprecated"
//efine HHC01252 - HHC01299 (available)

// reserve 013xx for channel related messages
#define HHC01300 "%1d:%04X CHAN: halt subchannel: cc=%d"
#define HHC01301 "%1d:%04X CHAN: midaw %2.2X %4.4"PRIX16" %16.16"PRIX64"%s"
#define HHC01302 "%1d:%04X CHAN: idaw %8.8"PRIX32", len %3.3"PRIX16"%s"
#define HHC01303 "%1d:%04X CHAN: idaw %16.16"PRIX64", len %4.4"PRIX16"%s"
#define HHC01304 "%1d:%04X CHAN: attention signaled"
#define HHC01305 "%1d:%04X CHAN: attention"
#define HHC01306 "%1d:%04X CHAN: initial status interrupt"
#define HHC01307 "%1d:%04X CHAN: attention completed"
#define HHC01308 "%1d:%04X CHAN: clear completed"
#define HHC01309 "%1d:%04X CHAN: halt completed"
#define HHC01310 "%1d:%04X CHAN: suspended"
#define HHC01311 "%1d:%04X CHAN: resumed"
#define HHC01312 "%1d:%04X CHAN: stat %2.2X%2.2X, count %4.4X"
#define HHC01313 "%1d:%04X CHAN: sense %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X"
#define HHC01314 "%1d:%04X CHAN: sense %s"
#define HHC01315 "%1d:%04X CHAN: ccw %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X%s"
#define HHC01316 "%1d:%04X CHAN: csw %2.2X, stat %2.2X%2.2X, count %2.2X%2.2X, ccw %2.2X%2.2X%2.2X"
#define HHC01317 "%1d:%04X CHAN: scsw %2.2X%2.2X%2.2X%2.2X, stat %2.2X%2.2X, count %2.2X%2.2X, ccw %2.2X%2.2X%2.2X%2.2X"
#define HHC01318 "%1d:%04X CHAN: test I/O: cc=%d"
//efine HHC01319 (available)
#define HHC01320 "%1d:%04X CHAN: start I/O S/370 conversion to asynchronous operation started"
#define HHC01321 "%1d:%04X CHAN: start I/O S/370 conversion to asynchronous operation successful"
//efine HHC01322 (available)
//efine HHC01323 (available)
//efine HHC01324 (available)
//efine HHC01325 (available)
//efine HHC01326 (available)
//efine HHC01327 (available)
//efine HHC01328 (available)
#define HHC01329 "%1d:%04X CHAN: halt I/O"
#define HHC01330 "%1d:%04X CHAN: HIO modification executed: cc=1"
#define HHC01331 "%1d:%04X CHAN: clear subchannel"
#define HHC01332 "%1d:%04X CHAN: halt subchannel"
#define HHC01333 "%1d:%04X CHAN: resume subchannel: cc=%d"
#define HHC01334 "%1d:%04X CHAN: ORB: %s"
//efine HHC01335 (available)
#define HHC01336 "%1d:%04X CHAN: startio cc=2 (busy=%d startpending=%d)"
//efine HHC01337 (available)
//efine HHC01338 (available)
//efine HHC01339 (available)
//efine HHC01340 (available)
//efine HHC01341 (available)
//efine HHC01342 (available)
//efine HHC01343 (available)
//efine HHC01344 (available)
//efine HHC01345 (available)
//efine HHC01346 (available)
//efine HHC01347 (available)
//efine HHC01348 (available)
//efine HHC01349 (available)
#define HHC01350 "%1d:%04X CHAN: missing generic channel method"                // (hchan.c)
#define HHC01351 "%1d:%04X CHAN: incorrect generic channel method %s"           // (hchan.c)
#define HHC01352 "%1d:%04X CHAN: generic channel initialisation failed"         // (hchan.c)
#define HHC01353 "%1d:%04X CHAN: generic channel is currently in development"   // (hchan.c)
//efine HHC01354 (available)
//efine HHC01355 (available)
//efine HHC01356 (available)
//efine HHC01357 (available)
//efine HHC01358 (available)
//efine HHC01359 (available)
//efine HHC01360 - HHC01389 (available)
#define HHC01390 "%s" // DUMP               (debugging)
#define HHC01391 "%s" // DUMP_STORAGE       (debugging)
#define HHC01392 "%s" // DISPLAY_PREFETCH   (debugging)
#define HHC01393 "%s" // DEBUG_SCSW         (debugging)
//efine HHC01394 - HHC01399 (available)

// reserve 014xx for initialization and shutdown
#define HHC01400 "CTRL_BREAK_EVENT received: %s"
#define HHC01401 "CTRL_C_EVENT received: %s"
#define HHC01402 "CTRL_CLOSE_EVENT received: %s"
#define HHC01403 "%s received: %s"
#define HHC01404 "Could not create the Automatic Operator thread"
#define HHC01405 "Script file %s not found"
#define HHC01406 "Startup parm -l: maximum loadable modules %d exceeded; remainder not loaded"
#define HHC01407 "Usage: %s [--help[=SHORT|LONG|VERSION|BUILD]] -f config-filename|\"none\" [-o logfile-name] [-r rcfile-name] [-d] [-b logo-filename] [-s sym=val] [-t [factor]] [-p dyn-load-dir] [[-l dynmod-to-load]...] [> logfile]"
#define HHC01408 "Hercules terminating, see previous messages for reason"
#define HHC01409 "Load of dyngui.dll failed, Hercules terminated"
#define HHC01410 "Cannot register %s handler: %s"
#define HHC01411 "Cannot suppress SIGPIPE signal: %s"
#define HHC01412 "Hercules terminated"
#define HHC01413 "%s version %s"
#define HHC01414 "%s"
#define HHC01415 "Build date: %s at %s"
//efine HHC01416 (available)
#define HHC01417 "%s"
#define HHC01418 "Symbol expansion will result in buffer overflow; ignored"
#define HHC01419 "Symbol and/or Value is invalid; ignored"
#define HHC01420 "Begin Hercules shutdown"
//efine HHC01421 (available)
#define HHC01422 "Configuration released"
#define HHC01423 "Calling termination routines"
#define HHC01424 "All termination routines complete"
#define HHC01425 "Hercules shutdown complete"
#define HHC01426 "Shutdown initiated"
#define HHC01427 "%s storage %sreleased"
//efine HHC01428 (available)
//efine HHC01429 (available)
#define HHC01430 "Error in function %s: %s"
#define HHC01431 "Expanded storage support not installed"
#define HHC01432 "Config file[%d] %s: error in function %s: %s"
#define HHC01433 "Config file[%d] %s: line is too long"
//efine HHC01434 (available)
#define HHC01435 "Config file %s: will ignore include errors"
#define HHC01436 "Config file[%d] %s: maximum nesting level %d reached"
#define HHC01437 "Config file[%d] %s: including file %s"
#define HHC01438 "Config file %s: open error ignored file %s: %s"
#define HHC01439 "Config file %s: open error file %s: %s"
//efine HHC01440 (available)
#define HHC01441 "Config file[%d] %s: error processing statement: %s"
//efine HHC01442 (available)
#define HHC01443 "Config file[%d] %s: %s is not a valid %s"
//efine HHC01444 (available)
//efine HHC01445 (available)
//efine HHC01446 (available)
#define HHC01447 "Default allowed AUTOMOUNT directory: %s"
#define HHC01448 "Config file[%d] %s: missing device number or device type"
//efine HHC01449 (available)
//efine HHC01450 (available)
#define HHC01451 "Invalid value %s specified for %s"
#define HHC01452 "Default port %s being used for %s"
#define HHC01453 "%s cannot be the same as %s"
//efine HHC01454 (available)
#define HHC01455 "Invalid number of arguments for %s"
#define HHC01456 "Invalid syntax %s for %s"
#define HHC01457 "Valid years for %s are %s; other values no longer supported"
#define HHC01458 "%1d:%04X: only devices on CSS 0 are usable in S/370 mode"
#define HHC01459 "Device %1d:%04X defined as %1d:%04X"
#define HHC01460 "%1d:%04X error in function %s: %s"
#define HHC01461 "%1d:%04X device already exists"
#define HHC01462 "%1d:%04X devtype %s not recognized"
#define HHC01463 "%1d:%04X device initialization failed"
#define HHC01464 "%1d:%04X %s does not exist"
#define HHC01465 "%1d:%04X %s detached"
#define HHC01466 "Unspecified error occured while parsing Logical Channel Subsystem Identification"
#define HHC01467 "No more than 1 Logical Channel Subsystem Identification may be specified"
#define HHC01468 "Non-numeric Logical Channel Subsystem Identification %s"
#define HHC01469 "Logical Channel Subsystem Identification %d exceeds maximum of %d"
#define HHC01470 "Incorrect %s near character '%c'"
#define HHC01471 "Incorrect device address range %04X < %04X"
#define HHC01472 "%1d:%04X is on wrong channel, 1st device defined on channel %02X"
#define HHC01473 "Some or all devices in %04X-%04X duplicate already defined devices"
#define HHC01474 "Using %s codepage conversion table %s"
#define HHC01475 "Codepage conversion table %s is not defined"
#define HHC01476 "Codepage is %s"
#define HHC01477 "Codepage user is not available; default selected"
#define HHC01478 "Codepage %s copied to user"
#define HHC01479 "Codepage user is deleted"
#define HHC01480 "Codepage: Error %s %s table data file %s; %s"
#define HHC01481 "Codepage: Valid conversion tables are:"
#define HHC01482 "Codepage: %-16.16s %-16.16s %-16.16s %-16.16s"
#define HHC01483 "Codepage: user %s table is empty"
#define HHC01484 "Codepage: Displaying user table %s%s"
#define HHC01485 "Codepage:    _0_1_2_3 _4_5_6_7 _8_9_A_B _C_D_E_F 0... 4... 8... C..."
#define HHC01486 "Codepage: %1.1X_%36.36s%20.20s %1.1X_"
#define HHC01487 "Codepage: %s user table %s"
#define HHC01488 "Codepage: Pos[%2.2X] was %2.2X is %2.2X"
#define HHC01489 "Codepage: Cannot %s; user table in use"
#define HHC01490 "Codepage: %s user table %s %s file %s"
#define HHC01491 "Codepage: g2h pos[%2.2X] = %2.2X; h2g pos[%2.2X] = %2.2X"
#define HHC01492 "Codepage: h2g pos[%2.2X] = %2.2X; g2h pos[%2.2X] = %2.2X"
#define HHC01493 "Codepage: Tables are transparent"
#define HHC01494 "Crypto: '%s' failed: %s"
#define HHC01495 "Crypto: **WARNING** Default insecure 'rand()' API being used"
//efine HHC01496 - HHC01499 (available)

// reserve 015xx for Hercules dynamic loader
#define HHC01500 "HDL: begin shutdown sequence"
#define HHC01501 "HDL: calling %s"
#define HHC01502 "HDL: %s complete"
#define HHC01503 "HDL: Invalid architecture passed to %s"
#define HHC01504 "HDL: shutdown sequence complete"
#define HHC01505 "HDL: directory '%s' rejected; exceeds maximum length of %d"
#define HHC01506 "HDL: directory '%s' rejected; '-p' cmdline option rules"
#define HHC01507 "HDL: directory remains '%s' from '-p' cmdline option"
#define HHC01508 "HDL: loadable module directory is '%s'"
#define HHC01509 "HDL: dependency check failed for %s, version %s expected %s"
#define HHC01510 "HDL: dependency check failed for %s, size %d expected %d"
#define HHC01511 "HDL: error in function %s: %s"
#define HHC01512 "HDL: begin termination sequence"
#define HHC01513 "HDL: calling module cleanup routine %s"
#define HHC01514 "HDL: module cleanup routine %s complete"
#define HHC01515 "HDL: termination sequence complete"
#define HHC01516 "HDL: unable to open module %s: %s"
#define HHC01517 "HDL: no HDL_DEPENDENCY_SECTION in %s: %s"
#define HHC01518 "HDL: dependency check failed for module %s"
#define HHC01519 "HDL: module %s already loaded"
#define HHC01520 "HDL: module %s is duplicate of %s"
#define HHC01521 "HDL: unloading of module %s not allowed"
#define HHC01522 "HDL: module %s bound to device %1d:%04X"
#define HHC01523 "HDL: unload of module %s rejected by final section"
#define HHC01524 "HDL: module %s not found"
#define HHC01525 "HDL: usage: %s <module>"
#define HHC01526 "HDL: loading module %s..."
#define HHC01527 "HDL: module %s loaded"
#define HHC01528 "HDL: unloading module %s..."
#define HHC01529 "HDL: module %s unloaded"
#define HHC01530 "HDL: incorrect syntax. Enter \"help %s\" for assistance"
#define HHC01531 "HDL: name = %s, type = %s, flags = (%sunloadable, %sforced)"
#define HHC01532 "HDL:  symbol = %s, loadcount = %d%s, owner = %s"
#define HHC01533 "HDL:  devtypes = %s"
#define HHC01534 "HDL:  opcode %4.4X (%c%c%c) %s"
#define HHC01535 "HDL: dependency %s: version \"%s\", size %d"
#define HHC01536 "HDL: WARNING: '%s' is not a valid directory"
//efine HHC01537 (available)
//efine HHC01538 (available)
#define HHC01539 "HDL: devtyp/hdlmod: %s"
#define HHC01540 "HDL: query buffer overflow for device %1d:%04X"
#define HHC01541 "HDL: dyngui.dll initiated"
#define HHC01542 "HDL: dyngui.dll terminated"
#define HHC01543 "HDL: no devequ module has been loaded"
//efine HHC01544 - HHC01599 (available)

// reserve 016xx for panel communication
#define HHC01600 "Unknown command %s, enter 'help' for a list of valid commands"
#define HHC01602 "%-*.*s  %c%s"
#define HHC01603 "%s"
#define HHC01604 "Unknown command %s, no help available"
#define HHC01605 "Invalid cmdlevel option: %s"
#define HHC01606 "cmdlevel[%2.2X] is %s"
//efine HHC01607 (available)
#define HHC01608 "PF KEY SUBSTitution results would exceed command line maximum size of %d; truncation occurred"
#define HHC01609 "No help available for mask %s"
#define HHC01610 " (*)  Enter \"help <command>\" for more info."
//efine HHC01611 - HHC01699 (available)

// reserve 017xx for ecps:vm support
#define HHC01700 "Abend condition detected in DISP2 instruction"
#define HHC01701 "| %-9s | %10"PRIu64" | %10"PRIu64" |  %3"PRIu64"%% |"
#define HHC01702 "+-----------+------------+------------+-------+"
#define HHC01703 "* : Unsupported, - : Disabled, %% - Debug"
#define HHC01704 "%"PRIu64" entries not shown and never invoked"
#define HHC01705 "%"PRIu64" call(s) were made to unsupported functions"
#define HHC01706 "| %-9s | %10s | %10s | %-5s |"
#define HHC01707 "ECPS:VM %s feature %s%s%s"
#define HHC01708 "All ECPS:VM %s features %s%s"
#define HHC01709 "ECPS:VM global debug %s"
#define HHC01710 "ECPS:VM %s feature %s %s%s"
#define HHC01711 "Unknown ECPS:VM feature %s; ignored"
#define HHC01712 "Current reported ECPS:VM level is %d"
#define HHC01713 "But ECPS:VM is currently disabled"
#define HHC01714 "Level reported to guest program is now %d"
#define HHC01715 "ECPS:VM level %d is not supported, unpredictable results may occur"
#define HHC01716 "The microcode support level is 20"
#define HHC01717 "%-9.9s    %s"
#define HHC01718 "Unknown subcommand %s, valid subcommands are :"
#define HHC01719 "ECPS:VM Command processor invoked"
#define HHC01720 "No ECPS:VM subcommand. Type \"ecpsvm help\" for a list of valid subcommands"
#define HHC01721 "Unknown ECPS:VM subcommand %s"
#define HHC01722 "ECPS:VM Command processor complete"
#define HHC01723 "Invalid ECPS:VM level value : %s. Default of 20 used"
#define HHC01724 "ECPS:VM Operating with CP FREE/FRET trap in effect"
#define HHC01725 "ECPS:VM Code version %.02f"
//efine HHC01726 - HHC01799 (available)

// reserve 018xx for http server
#define HHC01800 "HTTP server: error in function %s: %s"
#define HHC01801 "HTTP server: invalid root directory: %s: %s"
#define HHC01802 "HTTP server using root directory %s"
#define HHC01803 "HTTP server waiting for requests on port %u"
#define HHC01804 "HTTP server waiting for port %u to become free for requests"
#define HHC01805 "HTTP server signaled to stop"
#define HHC01806 "HTTP server is %s"
#define HHC01807 "HTTP server signaled to start"
#define HHC01808 "HTTP server port is %s with %s"
#define HHC01809 "HTTP server is waiting for requests"
#define HHC01810 "HTTP server is stopped"
#define HHC01811 "HTTP server root directory %s"
#define HHC01812 "HTTP server must be stopped for this command"
#define HHC01813 "HTTP server waiting for bind to complete; port in use by another server"
#define HHC01814 "HTTP server: auth requires valid userid and password operands"
#define HHC01815 "HTTP server port is %s"
//efine HHC01816 - HHC01899 (available)

// reserve 019xx for diagnose calls
#define HHC01900 "Diagnose 0x308 called: System is re-ipled"
#define HHC01901 "Checking processor %s%02X"
#define HHC01902 "Waiting 1 second for cpu's to stop"
//efine HHC01903 (available)
//efine HHC01904 (available)
#define HHC01905 "%04X triggered block I/O interrupt: code %4.4X parm %16.16"PRIX64" status %2.2X subcode %2.2X"
#define HHC01906 "%04X d250_init32 s %i o %"PRIi64" b %"PRIi64" e %"PRIi64
#define HHC01907 "%04X d250_init BLKTAB: type %4.4X arch %i 512 %i 1024 %i 2048 %i 4096 %i"
#define HHC01908 "Error in function %s: %s"
#define HHC01909 "%04X d250_preserve pending sense preserved"
#define HHC01920 "%04X d250_restore pending sense restored"
#define HHC01921 "%04X d250_remove block I/O environment removed"
#define HHC01922 "%04X d250_read %d-byte block (rel. to 0): %"PRId64
#define HHC01923 "%04X d250_read FBA unit status %2.2X residual %d"
#define HHC01924 "%04X async biopl %8.8X entries %d key %2.2X intp %8.8X"
#define HHC01925 "%04X d250_iorq32 sync bioel %8.8X entries %d key %2.2X"
#define HHC01926 "%04X d250_iorq32 psc %d succeeded %d failed %d"
#define HHC01927 "d250_list32 error: psc %i"
#define HHC01928 "%04X d250_list32 bios %i addr "F_RADR" I/O key %2.2X"
#define HHC01929 "%04X d250_list32 xcode %4.4X bioe32 %8.8"PRIX64"-%8.8"PRIX64" fetch key %2.2X"
#define HHC01930 "%04X d250_list32 bioe %8.8"PRIX64" oper %2.2X block %i buffer %8.8"PRIX64
#define HHC01931 "%04X d250_list32 xcode %4.4X rdbuf %8.8"PRIX64"-%8.8"PRIX64" fetch key %2.2X"
#define HHC01932 "%04X d250_list32 xcode %4.4X wrbuf %8.8"PRIX64"-%8.8"PRIX64" store key %2.2X"
#define HHC01933 "%04X d250_list32 xcode %4.4X status %8.8"PRIX64"-%8.8"PRIX64"  store key %2.2X"
#define HHC01934 "%04X d250_list32 bioe %8.8"PRIX64" status %2.2X"
#define HHC01935 "%04X async bioel %16.16"PRIX64" entries %"PRId64" key %2.2X intp %16.16"PRIX64
#define HHC01936 "%04X d250_iorq64 sync bioel %16.16"PRIX64" entries %"PRId64" key %2.2X"
#define HHC01937 "%04X d250_iorq64 psc %d succeeded %d failed %d"
#define HHC01938 "d250_list64 error: psc %i"
#define HHC01939 "%04X d250_list64 bioes %"PRIi64" addr "F_RADR" I/O key %2.2X"
#define HHC01940 "%04X d250_list64 xcode %4.4X bioe64 "F_RADR"-"F_RADR" fetch key %2.2X"
#define HHC01941 "%04X d250_list64 bioe "F_RADR" oper %2.2X block %"PRIi64" buffer "F_RADR
#define HHC01942 "%04X d250_list64 xcode %4.4X readbuf "F_RADR"-"F_RADR" fetch key %2.2X"
#define HHC01943 "%04X d250_list64 xcode %4.4X writebuf "F_RADR"-"F_RADR" store key %2.2X"
#define HHC01944 "%04X d250_list64 xcode %4.4X status "F_RADR"-"F_RADR" store key %2.2X"
#define HHC01945 "%04X d250_list64 bioe "F_RADR" status %2.2X"
//efine HHC01946 (available)
//efine HHC01947 (available)
//efine HHC01948 (available)
//efine HHC01949 (available)
#define HHC01950 "%s guest issued panel command: %s"
//efine HHC01951 (available)
#define HHC01952 "%1d:%04X:Diagnose X\'0A4\':%s blk=%8.8X adr=%8.8X len=%8.8X"
#define HHC01953 "DIAG8 access to shell disallowed by SHCMDOPT setting"
#define HHC01954 "Hercules-specific DIAG instructions support not included in engine build"
//efine HHC01955 - HHC01999 (available)

// reserve 020xx for sr.c
#define HHC02000 "SR: too many arguments"
#define HHC02001 "SR: error in function %s: %s"
#define HHC02002 "SR: waiting for device %04X"
#define HHC02003 "SR: device %04X still busy, proceeding anyway"
#define HHC02004 "SR: error processing file %s"
#define HHC02005 "SR: all processors must be stopped to resume"
#define HHC02006 "SR: file identifier error"
#define HHC02007 "SR: resuming suspended file created on %s"
#define HHC02008 "SR: archmode %s not supported"
#define HHC02009 "SR: mismatch in %s: %s found, %s expected"
#define HHC02010 "SR: processor CP%02X exceeds max allowed CP%02X"
#define HHC02011 "SR: processor %s%02X already configured"
#define HHC02012 "SR: processor %s%02X unable to configure online"
#define HHC02013 "SR: processor %s%02X invalid psw length %d"
#define HHC02014 "SR: processor %s%02X error loading psw, rc %d"
#define HHC02015 "SR: %04X: device initialization failed"
#define HHC02016 "SR: %04X: device type mismatch; %s found, %s expected"
#define HHC02017 "SR: %04X: %s size mismatch: %d found, %d expected"
#define HHC02018 "SR: invalid key %8.8X"
#define HHC02019 "SR: CPU key %8.8X found but no active CPU"
#define HHC02020 "SR: value error, incorrect length"
#define HHC02021 "SR: string error, incorrect length"
#define HHC02022 "SR: error loading CRW queue: not enough memory for %d CRWs"
//efine HHC02023 - HHC02099 (available)

// reserve 021xx: misc
#define HHC02100 "Logger: log not active"
#define HHC02101 "Logger: log closed"
#define HHC02102 "Logger: error in function %s: %s"
//efine HHC02103 (available)
#define HHC02104 "Logger: log switched to %s"
#define HHC02105 "Logger: log to %s"
#define HHC02106 "Logger: log switched off"
//efine HHC02107 - HHC02196 (available)
#define HHC02197 "Symbol name %s is reserved"
// Note HHC02198  is actually in config.c
#define HHC02198 "Device %04X type %04X subchannel %d:%04X attached"
#define HHC02199 "Symbol %-12s %s"

// reserve 022xx - 023xx for command processing
#define HHC02200 "%1d:%04X device not found"
#define HHC02201 "Device number missing"
#define HHC02202 "Missing argument(s). Type 'help %s' for assistance."
#define HHC02203 "%-14s: %s"
#define HHC02204 "%-14s set to %s"
#define HHC02205 "Invalid argument %s%s"
#define HHC02206 "Uptime %u week%s, %u day%s, %02u:%02u:%02u"
#define HHC02207 "Uptime %u day%s, %02u:%02u:%02u"
#define HHC02208 "Uptime %02u:%02u:%02u"
#define HHC02209 "%1d:%04X device is not a %s"
#define HHC02210 "%1d:%04X %s"
#define HHC02211 "Invalid argument(s). Type 'help %s' for assistance."
#define HHC02212 "%1d:%04X device started"
#define HHC02213 "%1d:%04X device not started%s"
#define HHC02214 "%1d:%04X device stopped"
#define HHC02215 "Command quiet ignored: external GUI active"
#define HHC02216 "Empty list"
#define HHC02217 "%c%s"
#define HHC02218 "Logic error"
#define HHC02219 "Error in function %s: %s"
#define HHC02220 "Entry deleted%s"
#define HHC02221 "Entry not found"
#define HHC02222 "Unsupported function"
#define HHC02223 "%s of %s-labeled volume %s pending for drive %u:%4.4X %s"
#define HHC02224 "Store status rejected: CPU not stopped"
#define HHC02225 "%1d:%04X is not a dasd device"
#define HHC02226 "No dasd devices found"
#define HHC02227 "Shell/Exec commands are disabled"
#define HHC02228 "%s key pressed"
#define HHC02229 "Instruction %s %s%s%s"
#define HHC02230 "%1d:%04X attention request raised"
#define HHC02231 "%1d:%04X busy or interrupt pending"
#define HHC02232 "%1d:%04X attention request rejected"
#define HHC02233 "%1d:%04X subchannel not enabled"
#define HHC02234 "Are you sure you didn't mean 'ipl %04X'"
//efine HHC02235 (available)
//efine HHC02236 (available)
#define HHC02237 "Not all devices shown (max %d)"
#define HHC02238 "Device numbers can only be redefined within the same Logical Channel SubSystem"
#define HHC02239 "command '%s' invalid for device type %04X"
#define HHC02240 "Processor %s%02X%s"
//efine HHC02241 (available)
#define HHC02242 "Max device threads: %d, current: %d, most: %d, waiting: %d, total I/Os queued: %d"
#define HHC02243 "%1d:%04X reinit rejected; drive not empty"
#define HHC02244 "%1d:%04X device initialization failed"
#define HHC02245 "%1d:%04X device initialized"
#define HHC02246 "Savecore: no modified storage found"
#define HHC02247 "Operation rejected: CPU not stopped"
#define HHC02248 "Saving locations %s-%s to file %s"
#define HHC02249 "Operation complete"
#define HHC02250 "Loading file %s to location %s"
#define HHC02251 "Address exceeds main storage size"
#define HHC02252 "Too many instructions! (Sorry!)"
#define HHC02253 "All CPU's must be stopped %s"
#define HHC02254 "CPU %02X is not online"
//efine HHC02255 (available)
#define HHC02256 "Command '%s' is deprecated%s"
#define HHC02257 "%s%7d"
//efine HHC02258 (available)
#define HHC02259 "Script %d aborted: %s"
#define HHC02260 "Script %d: begin processing file %s"
#define HHC02261 "Script %d: syntax error; statement ignored: %s"
#define HHC02262 "Script %d: processing paused for %d milliseconds..."
#define HHC02263 "Script %d: processing resumed..."
#define HHC02264 "Script %d: file %s processing ended"
#define HHC02265 "Script %d: file %s aborted due to previous conditions"
//efine HHC02266 (available)
#define HHC02267 "%s" // (trace instr: Real address is not valid)
#define HHC02268 "%s" // maxrates command
#define HHC02269 "%s" // General purpose registers
#define HHC02270 "%s" // Floating point registers
#define HHC02271 "%s" // Control registers
#define HHC02272 "%s" // Access registers
#define HHC02273 "Index %3d: %s"
#define HHC02274 "%s" // 'clocks' command
#define HHC02275 "SCSI auto-mount: %s"
#define HHC02276 "Floating point control register: %08"PRIX32
#define HHC02277 "Prefix register: %s"
#define HHC02278 "Program status word: %s"
#define HHC02279 "%s" // devlist command
#define HHC02280 "%s" // qd command
#define HHC02281 "%s" // pgmtrace_cmd
#define HHC02282 "%s" // aea_cmd
#define HHC02283 "%s" // aia_cmd
#define HHC02284 "%s" // tlb_cmd
#define HHC02285 "Counted %5u %s events"
#define HHC02286 "Average instructions / SIE invocation: %5u"
#define HHC02287 "No SIE performance data"
//efine HHC02288 (available)
#define HHC02289 "%s" // disasm_stor
#define HHC02290 "%s" // 'abs', 'r' and 'v' commands, and 'dump_abs_page' function
#define HHC02291 "%s" // 'abs', 'r' and 'v' commands, and 'dump_abs_page' function
#define HHC02292 "%s" // icount_cmd
#define HHC02293 "%s" // history.c: command history
#define HHC02294 "%s" // cachestats_cmd
//efine HHC02295 (available)
//efine HHC02296 (available)
//efine HHC02297 (available)
#define HHC02298 "%1d:%04X drive is empty"
#define HHC02299 "Invalid command usage. Type 'help %s' for assistance."
#define HHC02300 "sm=%2.2X pk=%d cmwp=%X as=%s cc=%d pm=%X am=%s ia=%"PRIX64
#define HHC02301 "%s: Unexpected read length at record %d; expected %d-byte record"
#define HHC02302 "%s: Record %d is unknown record type %s; skipped"
#define HHC02303 "%s: GOFF object found at record %d; aborting"
#define HHC02304 "%s: Record %d is unknown record type; skipped"
#define HHC02305 "%s: Record %d is unknown record type; aborting"
#define HHC02306 "%s: Address %s not on quadword boundary"
#define HHC02307 "%s: Record %d relocation value %6.6"PRIX64" address exceeds main storage size"
#define HHC02308 "%s: Warning messages issued, review before executing"
#define HHC02309 "%s: Record %d is unsupported record type %s; skipped"
#define HHC02310 "Panel command %s is not supported in this build; see option %s"
#define HHC02311 "%s completed"
#define HHC02312 "Empty list"
//efine HHC02313 (available)
#define HHC02314 "No scripts currently running"
#define HHC02315 "Script id:%d, tid:"TIDPAT", level:%d, name:%s"
#define HHC02316 "Script %s not found"
#define HHC02317 "%s bytes %s so far..."
#define HHC02318 "Config file[%d] %s: processing paused for %d milliseconds..."
#define HHC02319 "Config file[%d] %s: processing resumed..."
#define HHC02320 "Not all TCP keepalive settings honored"
#define HHC02321 "This build of Hercules does not support TCP keepalive"
#define HHC02322 "This build of Hercules has only basic TCP keepalive support"
#define HHC02323 "This build of Hercules has only partial TCP keepalive support"
#define HHC02324 "%s"     // (instruction tracing)
#define HHC02325 "%s%s"   // (instruction tracing: instr fetch error)
#define HHC02326 "%s"     // (instruction tracing: storage line)
#define HHC02327 "%c:"F_RADR"  Storage address is not valid"
#define HHC02328 "%c:"F_RADR"  Addressing exception"
#define HHC02329 "%c:"F_VADR"  Translation exception %4.4hX (%s)  %s"
#define HHC02330 "Script %d: test: [re]start failed"
#define HHC02331 "Script %d: test: aborted"
#define HHC02332 "Script %d: test: timeout"
#define HHC02333 "Script %d: test: running..."
#define HHC02334 "Script %d: test: test ended"
#define HHC02335 "Script %d: test: invalid timeout %s; set to %d instead"
#define HHC02336 "Script %d: test: test starting"
#define HHC02337 "runtest is only valid as a scripting command"
#define HHC02338 "Script %d: test: actual duration: %"PRId32".%06"PRId32" seconds"
#define HHC02339 "Script %d: test: duration limit: %"PRId32".%06"PRId32" seconds"
//efine HHC02340 (available)
#define HHC02341 "Script %d: test: unknown runtest keyword: %s"
#define HHC02342 "%s file '%s' not found: %s"
#define HHC02343 "Terminating due to %d argument errors"
#define HHC02344 "%s device %1d:%04X group has registered MAC address %s"
#define HHC02345 "%s device %1d:%04X group has registered IP address %s"
#define HHC02346 "%s device %1d:%04X group has no registered MAC or IP addresses"
#define HHC02347 "No %s devices found"
//efine HHC02348 (available)
//efine HHC02349 (available)
//efine HHC02350 - HHC02359 (available)
//efine HHC02360 - HHC02369 (available)
#define HHC02370 "Automatic tracing started at instrcount %"PRIu64" (BEG+%"PRIu64")"
#define HHC02371 "Automatic tracing stopped at instrcount %"PRIu64" (AMT+%"PRIu64")"
#define HHC02372 "Automatic tracing not enabled"
#define HHC02373 "Automatic tracing disabled"
#define HHC02374 "Automatic tracing enabled: BEG=%"PRIu64" AMT=%"PRIu64
#define HHC02375 "Automatic tracing is active"
#define HHC02376 "Automatic tracing value(s) must be greater than zero"
#define HHC02377 "Trace file open error %s: \"%s\""
#define HHC02378 "Invalid MAX= value: %s"
#define HHC02379 "Trace file MAX= reached; file closed, tracing %s"
#define HHC02380 "Invalid %s= value: %s"
#define HHC02381 "File closed, tracing %s"
#define HHC02382 "%stracing active for %s"
#define HHC02383 "Trace file tracing begun..."
//efine HHC02384 (available)
#define HHC02385 "CPUMODEL %04X does not technically support TXF"
#define HHC02386 "Configure CPU error %d"
#define HHC02387 "Configure expanded storage error %d"
#define HHC02388 "Configure storage error %d"
#define HHC02389 "CPUs must be offline or stopped"
#define HHC02390 "Storage "F_RADR"-"F_RADR" set to unusable"
#define HHC02391 "No unusable storage found"
//efine HHC02392 - HHC02399 (available)

// reserve 024xx for dasd utilities
// dasdcat.c
#define HHC02400 "Directory block byte count is invalid"
#define HHC02401 "Non-PDS-members not yet supported"
#define HHC02402 "Unknown 'member:flags' formatting option %s"
#define HHC02403 "Failed opening %s"
#define HHC02404 "Can't make 80 column card images from block length %d"
#define HHC02405 "Usage:\n"                                                         \
       "HHC02405I\n"                                                                \
       "HHC02405I       %s [-i dasd_image [sf=shadowfile] spec...]...\n"            \
       "HHC02405I\n"                                                                \
       "HHC02405I Where:\n"                                                         \
       "HHC02405I\n"                                                                \
       "HHC02405I       -i             indicates next arg is input file\n"          \
       "HHC02405I       dasd_image     input dasd image file\n"                     \
       "HHC02405I       shadowfile     optional dasd image shadow file\n"           \
       "HHC02405I       spec           pdsname/memname[:flags]\n"                   \
       "HHC02405I\n"                                                                \
       "HHC02405I       pdsname        the name of the partitioned dataset\n"       \
       "HHC02405I       memname        either a specific member name or one\n"      \
       "HHC02405I                      of the following special characters:\n"      \
       "HHC02405I\n"                                                                \
       "HHC02405I                         '?'    just list all of the names\n"      \
       "HHC02405I                                of pdsname's members\n"            \
       "HHC02405I\n"                                                                \
       "HHC02405I                         '*'    selects all pdsname members\n"     \
       "HHC02405I\n"                                                                \
       "HHC02405I       flags          indicates how to format selected members:\n" \
       "HHC02405I\n"                                                                \
       "HHC02405I                         'c'    as 72 or 80 column card images\n"  \
       "HHC02405I                         's'    indicates 'c' shoud be 80 cols\n"  \
       "HHC02405I                         'a'    as plain unformatted ASCII\n"      \
       "HHC02405I                         '?'    member CCCCHHR/size info only\n"   \
       "HHC02405I\n"                                                                \
       "HHC02405I More than one flag may be specified. The 's' option is ignored\n" \
       "HHC02405I unless the 'c' option is also specified. Both imply 'a' ASCII.\n" \
       "HHC02405I\n"                                                                \
       "HHC02405I If no flags are given the member is written in binary exactly\n"  \
       "HHC02405I as-is. Use caution when no formatting flags are given since\n"    \
       "HHC02405I many terminal programs do not react too well when pure binary\n"  \
       "HHC02405I data is written to the screen. When selecting a single member\n"  \
       "HHC02405I without any formatting options it is highly recommended that\n"   \
       "HHC02405I you redirect output to another file (e.g. dasdcat ... > file).\n" \
       "HHC02405I\n"                                                                \
       "HHC02405I Return code is 0 if successful or 1 if any errors."
#define HHC02406 "Member '%s' not found in dataset '%s' on volume '%s'"
#define HHC02407 "%s/%s/%-8s %8s bytes from %4.4"PRIX32"%2.2"PRIX32"%2.2"PRIX32" to %4.4"PRIX32"%2.2"PRIX32"%2.2"PRIX32
#define HHC02408 "Error in function %s: %s"
//efine HHC02409 (available)

// dasdconv.c
#define HHC02410 "Usage: %s [options] infile outfile\n" \
       "HHC02410I   infile:  name of input HDR-30 CKD image file ('-' means stdin)\n" \
       "HHC02410I   outfile: name of AWSCKD image file to be created\n" \
       "HHC02410I options:\n" \
       "HHC02410I   -r     replace existing output file\n" \
       "HHC02410I   -q     suppress progress messages%s"
#define HHC02411 "Usage: %s [-f] [-level] [-ro] file1 [file2 ...]\n" \
       "HHC02410I   file    name of DASD image file\n" \
       "HHC02411I options:\n" \
       "HHC02411I   -f      force check even if OPENED bit is on\n" \
       "HHC02411I   -ro     open file readonly, no repairs\n" \
       "HHC02411I   -0      minimal checking (hdr, chdr, l1tab, l2tabs)\n" \
       "HHC02411I   -1      normal  checking (hdr, chdr, l1tab, l2tabs, free spaces)\n" \
       "HHC02411I   -2      extra   checking (hdr, chdr, l1tab, l2tabs, free spaces, trkhdrs)\n" \
       "HHC02411I   -3      maximal checking (hdr, chdr, l1tab, l2tabs, free spaces, trkimgs)\n" \
       "HHC02411I   -4      recover everything without using meta-data"
#define HHC02412 "Error in function %s: %s"
#define HHC02413 "Dasdconv is compiled without compress support and input is compressed"
#define HHC02414 "Input file is already in CKD format, use dasdcopy"
#define HHC02415 "Unknown device type %04X at offset 00000000 in input file"
#define HHC02416 "Unknown device type %04X"
#define HHC02417 "Invalid track header at offset 0x%16.16"PRIX64
#define HHC02418 "Expected CCHH %04X%04X, found CCHH %04X%04X"
#define HHC02419 "Invalid record header (rc %d) at offset %04X in trk at CCHH %04X%04X at offset 0x%16.16"PRIX64" in file %s"
#define HHC02420 "%u cylinders succesfully written to file %s"
#define HHC02421 "Cylinder count %u is outside range %u-%u"
#define HHC02422 "Converting %04X volume %s: %u cyls, %u trks/cyl, %u bytes/trk"
#define HHC02423 "DASD operation completed"
#define HHC02424 "Dasd image file format unsupported or unrecognized: %s"
//efine HHC02425 - HHC02429 (available)

// dasdcopy.c
#define HHC02430 "CKD lookup failed: device type %04X cyls %d"
#define HHC02431 "FBA lookup failed: blks %d"
#define HHC02432 "Failed creating %s"
#define HHC02433 "Read error on file %s: %s %d stat=%2.2X, null %s substituted"
#define HHC02434 "Write error on file %s: %s %d stat=%2.2X"
#define HHC02435 "Usage: ckd2cckd [-options] ifile ofile\n" \
       "HHC02435I Copy a ckd dasd file to a compressed ckd dasd file\n" \
       "HHC02435I   ifile    input ckd dasd file\n" \
       "HHC02435I   ofile    output compressed ckd dasd file\n" \
       "HHC02435I options:\n" \
       "HHC02435I   -h       display this help and quit\n" \
       "HHC02435I   -q       quiet mode, don't display status\n" \
       "HHC02435I   -r       replace the output file if it exists\n" \
       "%s" \
       "%s" \
       "HHC02435I   -0       don't compress track images\n" \
       "HHC02435I   -cyls n  size of output file\n" \
       "HHC02435I   -a       output file will have alt cyls"
#define HHC02436 "Usage: cckd2ckd [-options] ifile [sf=sfile] ofile\n" \
       "HHC02436I Copy a compressed ckd file to a ckd file\n" \
       "HHC02436I   ifile    input compressed ckd dasd file\n" \
       "HHC02436I   sfile    input compressed ckd shadow file (optional)\n" \
       "HHC02436I   ofile    output ckd dasd file\n" \
       "HHC02436I options:\n" \
       "HHC02436I   -h       display this help and quit\n" \
       "HHC02436I   -q       quiet mode, don't display status\n" \
       "HHC02436I   -r       replace the output file if it exists\n" \
       "%s" \
       "HHC02436I   -cyls n  size of output file\n" \
       "HHC02436I   -a       output file will have alt cyls"
#define HHC02437 "Usage: fba2cfba [-options] ifile ofile\n" \
       "HHC02437I Copy a fba dasd file to a compressed fba dasd file\n" \
       "HHC02437I   ifile    input fba dasd file\n" \
       "HHC02437I   ofile    output compressed fba dasd file\n" \
       "HHC02437I options:\n" \
       "HHC02437I   -h       display this help and quit\n" \
       "HHC02437I   -q       quiet mode, don't display status\n" \
       "HHC02437I   -r       replace the output file if it exists\n" \
       "%s" \
       "%s" \
       "HHC02437I   -0       don't compress track images\n" \
       "HHC02437I   -blks n  size of output file"
#define HHC02438 "Usage: cfba2fba [-options] ifile [sf=sfile] ofile\n" \
       "HHC02438I Copy a compressed fba file to a fba file\n" \
       "HHC02438I   ifile    input compressed fba dasd file\n" \
       "HHC02438I   sfile    input compressed fba shadow file (optional)\n" \
       "HHC02438I   ofile    output fba dasd file\n" \
       "HHC02438I options:\n" \
       "HHC02438I   -h       display this help and quit\n" \
       "HHC02438I   -q       quiet mode, don't display status\n" \
       "HHC02438I   -r       replace the output file if it exists\n" \
       "%s" \
       "HHC02438I   -blks n  size of output file"
#define HHC02439 "Usage: %s [-options] ifile [sf=sfile] ofile\n" \
       "HHC02439I Copy a dasd file to another dasd file\n" \
       "HHC02439I   ifile    input dasd file   (required)\n" \
       "HHC02439I   sfile    input shadow file (optional) (see note)\n" \
       "HHC02439I   ofile    output dasd file  (required)\n" \
       "HHC02439I Options:\n" \
       "HHC02439I   -h       display this help and quit\n" \
       "HHC02439I   -q       quiet mode, don't display status\n" \
       "HHC02439I   -r       replace the output file if it exists\n" \
       "%s" \
       "%s" \
       "HHC02439I   -0       don't compress output\n" \
       "HHC02439I   -blks n  size of output fba file\n" \
       "HHC02439I   -cyls n  size of output ckd file\n" \
       "HHC02439I   -a       output ckd file will have alt cyls\n" \
       "%s" \
       "HHC02439I   -o type  output file type (%s)\n" \
       "HHC02439I\n" \
       "HHC02439I Note: input shadow files will, as part of the copy\n" \
       "HHC02439I operation, have their data automatically merged into\n" \
       "HHC02439I the resulting output file."

// dasdinit.c
//efine HHC02440 (available)
//efine HHC02441 (available)
//efine HHC02442 (available)
//efine HHC02443 (available)
#define HHC02444 "Member %s is not a single text record"
#define HHC02445 "Invalid, unsupported or missing %s: %s"
#define HHC02446 "Invalid number of arguments"
#define HHC02447 "Option '-linux' is only supported fo device type 3390"
#define HHC02448 "Usage: %s [-options] filename devtype[-model] [volser] [size]\n" \
       "HHC02448I Builds an empty dasd image file\n" \
       "HHC02448I options:\n" \
       "HHC02448I\n" \
       "%s" \
       "%s" \
       "HHC02448I   -0        build compressed dasd image file with no compression\n" \
       "%s" \
       "HHC02448I   -a        build dasd image file that includes alternate cylinders\n" \
       "HHC02448I             (option ignored if size is manually specified)\n" \
       "HHC02448I   -r        build 'raw' dasd image file\n" \
       "HHC02448I             (no VOL1 or IPL track)\n" \
       "HHC02448I   -b        make wait PSW in IPL1 record a BC-mode PSW\n" \
       "HHC02448I             (default is EC-mode PSW)\n" \
       "HHC02448I   -m        enable wait PSW in IPL1 record for machine checks\n" \
       "HHC02448I             (default is disabled for machine checks)\n" \
       "HHC02448I   -linux    null track images will look like Linux dasdfmt'ed images\n" \
       "HHC02448I             (3390 device type only)\n" \
       "HHC02448I\n" \
       "HHC02448I   filename  name of dasd image file to be created\n" \
       "HHC02448I   devtype   CKD: 2305, 2311, 2314, 3330, 3340, 3350, 3375, 3380, 3390, 9345\n" \
       "HHC02448I             FBA: 0671, 3310, 3370, 9313, 9332, 9335, 9336\n" \
       "HHC02448I\n" \
       "HHC02448I   model     device model (implies size) (opt)\n" \
       "HHC02448I   volser    volume serial number (1-6 characters)\n" \
       "HHC02448I             (specified only if '-r' option not used)\n" \
       "HHC02448I   size      number of CKD cylinders or 512-byte FBA sectors\n" \
       "HHC02448I             (required if model not specified else optional)"
#define HHC02449 "DASD operation failed"
#define HHC02450 "Member %s type %s skipped"
#define HHC02451 "Too many members"
#define HHC02452 "Member %s has TTR count zero"
#define HHC02453 "Member %s is not a single text record"
#define HHC02454 "Member %s size %04X exceeds limit 07F8"
#define HHC02455 "Member %s size %04X is not a multiple of 8"
#define HHC02456 "Member %s has invalid TTR %04X%02X"
#define HHC02457 "Member %s text record TTR %04X%02X CCHHR %04X%04X%02X in progress"
#define HHC02458 "Member %s error reading TTR %04X%02X"
#define HHC02459 "Member %s TTR %04X%02X text record length %X is invalid"
#define HHC02460 "Member %s TTR %04X%02X text record length %X does not match %X in directory"
#define HHC02461 "Member %s TTR %04X%02X XCTL table improperly terminated"
#define HHC02462 "Member %s %s TTRL %02X%02X%02X%02X: %s"
#define HHC02463 "Usage: %s [-option [-option ... ]] ckdfile [sf=sfile]  [...]\n" \
       "HHC02463I   ckdfile:      input dasd image file\n" \
       "HHC02463I   sfile:        optional dasd image shadow file\n" \
       "HHC02463I options:\n" \
       "HHC02463I   -hdr          show column headers\n" \
       "HHC02463I   -dsnl[=n]     restrict dsname width\n" \
       "HHC02463I   -info         show F1 info\n" \
       "HHC02463I   -caldt        calendar date format\n" \
       "HHC02463I   -refdt        show last-reference date\n" \
       "HHC02463I   -expdt        show expiry date\n" \
       "HHC02463I   -yroffs[=n]   year offset\n" \
       "HHC02463I Note:\n" \
       "HHC02463I   Multiple images can be processed in the same run,\n" \
       "HHC02463I   but options must be specified ahead of each image."
#define HHC02464 "End of directory, %d members selected"
#define HHC02465 "Invalid argument: %s"
#define HHC02466 "Reading directory block at CCHHR %04X%04X%02X"
#define HHC02467 "End of directory"
#define HHC02468 "File %s; %s error: %s"
#define HHC02469 "Member %s TTR %04X%02X"
#define HHC02470 "Invalid block length %d at CCHHR %04X%04X%02X"
#define HHC02471 "%s record not found"
#define HHC02472 "Dataset is not RECFM %s; utility ends"
#define HHC02473 "Dataset is not DSORG %s; utility ends"
#define HHC02474 "Error processing %s"
#define HHC02475 "Records %s %s: %d"
#define HHC02476 "Dataset %s not found"
#define HHC02477 "In %s: function %s rc %d%s"
#define HHC02478 "Length invalid for KEY %d or DATA %d%s"
#define HHC02479 "In %s: x'F%s' expected for DS%sIDFMT, received x'%02X'"
#define HHC02480 "In %s: extent slots exhausted; maximum supported is %d"
#define HHC02481 "     EXTENT --begin-- ---end---"
#define HHC02482 "TYPE NUMBER CCCC HHHH CCCC HHHH"
#define HHC02483 "  %02X   %02X   %04X %04X %04X %04X"
#define HHC02484 "%s"
#define HHC02485 "Dataset %s on volume %s sequence %d"
#define HHC02486 "Creation Date: %s; Expiration Date: %s"
#define HHC02487 "Dsorg=%s recfm=%s lrecl=%d blksize=%d"
#define HHC02488 "System code %s"
#define HHC02489 "%s: unable to allocate ASCII buffer"
#define HHC02490 "%s: convert_tt() track %5.5d/x'%04X', rc %d"
#define HHC02491 "%s: extent number parameter invalid %d; utility ends"
//efine HHC02492 (available)
//efine HHC02493 (available)
#define HHC02494 "Requested number of extents %d exceeds maximum %d; utility ends"
#define HHC02495 "Usage: %s [-f] [-n] file1 [file2 ...]\n" \
       "HHC02495I   file    name of CCKD file\n" \
       "HHC02495I Options:\n" \
       "HHC02495I   -f      force check even if OPENED bit is on\n" \
       "HHC02495I   -0      minimal checking (default)\n" \
       "HHC02495I   -1      normal  checking\n" \
       "HHC02495I   -2      intermediate checking\n" \
       "HHC02495I   -3      maximal checking"
#define HHC02496 "Usage: %s [options] ctlfile outfile [n [maxdblk maxttr maxdscb]]\n" \
       "HHC02496I options:\n" \
       "HHC02496I   -0     no compression (default)\n" \
       "HHC02496I   -a     output disk will include alternate cylinders\n" \
       "HHC02496I   -b     make wait PSW in IPL1 record a BC-mode PSW\n" \
       "HHC02496I          (default is EC-mode PSW)\n" \
       "HHC02496I   -m     enable wait PSW in IPL1 record for machine checks\n" \
       "HHC02496I          (default is disabled for machine checks)\n" \
       "HHC02496I %s%s%s" \
       "HHC02496I\n" \
       "HHC02496I ctlfile  name of input control file\n" \
       "HHC02496I outfile  name of DASD image file to be created\n" \
       "HHC02496I\n" \
       "HHC02496I n        'n' is a digit 0 - 5 (default is 1) indicating output verbosity\n" \
       "HHC02496I max...   'maxdblk', etc, is maximum number of DBLK/TTR/DSCB entries or 0 for default"
#define HHC02497 "Usage: %s [-f] [-level] file1 [file2 ... ]\n" \
       "HHC02497I   file    name of CCKD file\n" \
       "HHC02497I Options:\n" \
       "HHC02497I   -f      force check even if OPENED bit is on\n" \
       "HHC02497I   -0      minimal checking (default)\n" \
       "HHC02497I   -1      normal  checking\n" \
       "HHC02497I   -2      intermediate checking\n" \
       "HHC02497I   -3      maximal checking"
//efine HHC02498 (available)
#define HHC02499 "Hercules utility %s - version %s"

// reserve 025xx for dasd utilities
#define HHC02500 "Volume serial statement missing from %s"
#define HHC02501 "Volume serial %s is invalid; %s line %d"
#define HHC02502 "Device type %s is not recognized; %s line %d"
#define HHC02503 "Cylinder value %s is invalid; %s line %d"
#define HHC02504 "Cannot create %s; %s failed"
#define HHC02505 "CCHH[%04X%04X] not found in extent table"
#define HHC02506 "Cannot %s file %s; error %s"
#define HHC02507 "File %s: invalid object file"
#define HHC02508 "File %s: TXT record has invalid count of %d"
#define HHC02509 "File %s: IPL text exceeds %d bytes"
#define HHC02510 "Input record CCHHR[%02X%02X%02X%02X%02X] exceeds output device track size"
#define HHC02511 "Dataset exceeds extent size: reltrk=%d, maxtrk=%d"
#define HHC02512 "Input record CCHHR[%02X%02X%02X%02X%02X] exceeds virtual device track size"
#define HHC02513 "File %s: read error at cyl[%04X/%d] head[%04X/%d]"
#define HHC02514 "File %s: cyl[%04X/%d] head[%04X/%d]; invalid track header %02X%02X%02X%02X%02X"
#define HHC02515 "File %s: cyl[%04X/%d] head[%04X/%d] rec[%02X/%d] record not found"
#define HHC02516 "File %s: cyl[%04X/%d] head[%04X/%d] rec[%02X/%d] unmatched KL/DL"
#define HHC02517 "Error obtaining storage for %sDSCB: %s"
#define HHC02518 "Internal error: DSCB count exceeds MAXDSCB of %d"
#define HHC02519 "VTOC requires %d track%s; current allocation is too small"
#define HHC02520 "Creating %4.4X volume %s: %u trks/cyl, %u bytes/track"
#define HHC02521 "Loading %4.4X volume %s"
#define HHC02522 "IPL text address=%06X length=%04X"
#define HHC02523 "Updating cyl[%04X/%d] head[%04X/%d] rec[%02X/%d]; kl[%d] dl[%d]"
#define HHC02524 "VTOC starts at cyl[%04X/%d] head[%04X/%d] and is %d track%s"
#define HHC02525 "Format %d DSCB CCHHR[%04X%04X%02X] (TTR[%04X%02X]) %s"
#define HHC02526 "\t+%04X %-8.8s %04X %04X"
#define HHC02527 "File number: %d"
#define HHC02528 "File[%04u]: DSNAME=%s"
#define HHC02529 "            DSORG=%s RECFM=%s LRECL=%d BLKSIZE=%d KEYLEN=%d DIRBLKS=%d"
#define HHC02530 "Error reading %s at cyl[%04X/%d] head[%04X/%d]"
#define HHC02531 "Incomplete text unit"
#define HHC02532 "Too many fields in text unit"
#define HHC02533 "File %s: read error: %s"
#define HHC02534 "File %s: invalid segment header: %02X%02X"
#define HHC02535 "File %s: first segment indicator expected"
#define HHC02536 "File %s: first segment indicator not expected"
#define HHC02537 "File %s: control record indicator mismatch"
#define HHC02538 "Invalid text unit at offset %04X"
#define HHC02539 "COPYR%d record length is invalid"
#define HHC02540 "COPYR%d header identifier not correct"
#define HHC02541 "COPYR%d unload format is unsupported"
#define HHC02542 "Invalid number of extents %d"
#define HHC02543 "Invalid directory block record length"
#define HHC02544 "Storage allocation for %s using %s failed: %s"
#define HHC02545 "Internal error: Number of directory blocks exceeds MAXDBLK of %d"
#define HHC02546 "Invalid directory block byte count"
#define HHC02547 "File %s: read error at cyl[%04X/%d] head[%04X/%d]"
#define HHC02548 "File %s: cyl[%04X/%d] head[%04X/%d] invalid track header %02X%02X%02X%02X%02X"
#define HHC02549 "File %s: cyl[%04X/%d] head[%04X/%d] rec[%02X/%d] note list record not found"
#define HHC02550 "Original dataset: DSORG=%s RECFM=%s LRECL=%d BLKSIZE=%d KEYLEN=%d"
#define HHC02551 "Dataset was unloaded from device type %02X%02X%02X%02X (%s)"
#define HHC02552 "Original device was cyls[%04X/%d], heads[%04X/%d]"
#define HHC02553 "Extent %d: Begin CCHH[%04X%04X] End CCHH[%04X%04X] Tracks=[%04X/%d]"
#define HHC02554 "End of directory"
#define HHC02555 "%s %-8.8s TTR[%02X%02X%02X] "
#define HHC02556 "Member %s TTR[%02X%02X%02X] replaced by TTR[%02X%02X%02X]"
#define HHC02557 "Member %s TTR[%02X%02X%02X] not found in dataset"
#define HHC02558 "Member %s Updating note list for member at TTR[%04X%02X] CCHHR[%04X%04X%02X]"
#define HHC02559 "Member %s Note list at cyl[%04X/%d] head[%04X/%d] rec[%02X/%d] dlen[%d] is too short for %d TTRs"
#define HHC02560 "File %s: Processing"
#define HHC02561 "Control record: %s len[%d]"
#define HHC02562 "File number:    %d %s"
#define HHC02563 "Data record:    len[%d]"
#define HHC02564 "Input record:   CCHHR[%04X%04X%02X] (TTR[%04X%02X]) kl[%d] dl[%d]\n" \
       "HHC02564I relocated to:   CCHHR[%04X%04X%02X] (TTR[%04X%02X])"
#define HHC02565 "Catalog block:  cyl[%04X/%d] head[%04X/%d] rec[%02X/%d]"
#define HHC02566 "DIP complete:   cyl[%04X/%d] head[%04X/%d] rec[%02X/%d]"
#define HHC02567 "File %s[%04d]: %s"
#define HHC02568 "Creating dataset %-44s at cyl[%04X/%d] head[%04X/%d]"
#define HHC02569 "File %s: line[%04d] error: statement is invalid"
#define HHC02570 "File %s: %s error: input record CCHHR[%04X%04X%02X] (TTR[%04X%02X]) kl[%d] dl[%d]"
#define HHC02571 "Internal error: TTR count exceeds MAXTTR of %d"
#define HHC02572 "File %s: XMIT utility file is not in IEBCOPY format; file is not loaded"
#define HHC02573 "File %s: invalid dsorg[0x%02X] for SEQ processing; dsorg must be PS or DA"
#define HHC02574 "File %s: invalid recfm[0x%02X] for SEQ processing; recfm must be F or FB"
#define HHC02575 "File %s: invalid lrecl[%d] or blksize[%d] for SEQ processing"
#define HHC02576 "File %s: invalid keyln[%d] for SEQ processing; keyln must be 0 for blocked"
#define HHC02577 "File %s: line[%04d] error: line length is invalid"
#define HHC02578 "%s missing"
#define HHC02579 "Initialization method %s is invalid"
#define HHC02580 "Initialization file name missing"
#define HHC02581 "Unit of allocation %s is invalid"
#define HHC02582 "%s allocation of %s %sS is invalid"
#define HHC02583 "Directory space %s is invalid"
#define HHC02584 "Dataset organization (DSORG) %s is invalid"
#define HHC02585 "Record format (RECFM) %s is invalid"
#define HHC02586 "Logical record length (LRECL) %s is invalid"
#define HHC02587 "Block size (BLKSIZE) %s is invalid"
#define HHC02588 "Key Length (KEYLEN) %s is invalid"
#define HHC02589 "Dataset %-44s contains %d track%s"
#define HHC02590 "Volume exceeds %d cylinders"
#define HHC02591 "File %s: total cylinders written was %d"
#define HHC02592 "VTOC pointer [%02X%02X%02X%02X%02X] updated"
#define HHC02593 "VOL1 record not readable or locatable"
#define HHC02594 "Syntax error: %s"
#define HHC02595 "Shadow file data successfully merged into output"
//efine HHC02596 (available)
//efine HHC02597 (available)
//efine HHC02598 (available)
//efine HHC02599 (available)

// reserve 026xx for utilites
// cckddiag
#define HHC02600 "Usage: %s [options] file\n" \
       "HHC02600I   file      name of CCKD/CFBA file\n" \
       "HHC02600I Options:\n" \
       "HHC02600I   -d        display DEVHDR\n" \
       "HHC02600I   -c        display CDEVHDR\n" \
       "HHC02600I   -1        display L1TAB (numeric one)\n" \
       "HHC02600I   -g        enable debug output\n" \
       "HHC02600I CKD track and FBA block related options:\n" \
       "HHC02600I   -a cc hh  display absolute CCHH track data\n" \
       "HHC02600I   -r nnnnn  display relative track/block data\n" \
       "HHC02600I   -2        display L2TAB related to -a or -r\n" \
       "HHC02600I   -t        display track/block data\n" \
       "HHC02600I   -x        hex display track key and/or block data\n" \
       "HHC02600I Offset option:\n" \
       "HHC02600I   -o oo ll  hex display data at offset oo of length ll"
#define HHC02601 "%s" // action/info
#define HHC02602 "From %s: Storage allocation of size %d using %s failed"
#define HHC02603 "lseek() to pos 0x%16.16"PRIx64" error: %s"
#define HHC02604 "%s%s" // error
#define HHC02605 "Track %d COUNT cyl[%04X/%d] head[%04X/%d] rec[%02X/%d] kl[%d] dl[%d]"
#define HHC02606 "Track %d rec[%02X/%d] %s[%d]:"
#define HHC02607 "error opening file %s: %s"
#define HHC02608 "DASD table entry not found for devtype 0x%2.2X"
#define HHC02609 "CC %d HH %d = reltrk %d; L1 index = %d, L2 index = %d"
#define HHC02610 "L1 index %d = L2TAB offset %"PRId64" (0x%16.16"PRIX64")"
#define HHC02611 "L2 index %d = L2TAB entry: %d bytes:"
#define HHC02612 "%sHDR %s %d:"
#define HHC02613 "%s offset %"PRId64" (0x%16.16"PRIX64"); length %d (0x%8.8X) bytes%s"
#define HHC02614 "%s - %d (decimal) bytes:"
#define HHC02615 "Block %d; L1 index = %d, L2 index = %d"
#define HHC02616 "Block %d:"
#define HHC02617 "%s %d does not exist on this device"
#define HHC02618 "L2tab for %s %d not found"
#define HHC02619 "%s %d not found"

// vmfplc2
#define HHC02620 "Usage: %s [options] VERB [<ctlfile>] <tapein> | <tapeout>\n"  \
       "HHC02620I   VERB      desired action               (DUMP/LOAD/SCAN)\n"  \
       "HHC02620I   ctlfile   filename of control file     (DUMP/LOAD only)\n"  \
       "HHC02620I   tapein    filename of input tape file  (LOAD/SCAN only)\n"  \
       "HHC02620I   tapeout   filename of output tape file (DUMP only)\n"       \
       "HHC02620I Options:\n"                                                   \
       "HHC02620I   -c cp     desired translation codepage\n"                   \
       "HHC02620I   -t        read/write tape in CMS 'TAPE' DUMP format\n"      \
       "HHC02620I   -v        don't suppress certain informational messages\n"  \
       "HHC02620I   -u        create uncompressed .aws output\n"                \
       "HHC02620I   -z        create compressed .het using zlib (default)\n"    \
       "HHC02620I   -b        create compressed .het using bzip2\n"             \
       "HHC02620I   -4        desired compression level (-1, -2 ... -9)"
#define HHC02621 "Invalid %s \"%s\""
#define HHC02622 "%s not specified"
#define HHC02623 "File open error: \"%s\": %s"
#define HHC02624 "Error in function %s: %s"
#define HHC02625 "%s"
#define HHC02626 ">>> %s"
#define HHC02627 "I/O error on file \"%s\": %s"
#define HHC02628 "lrecl %u less than %u; one or more records truncated"
#define HHC02629 "Expected %d bytes from file \"%s\", but only %d bytes read"
#define HHC02630 "%d errors encountered"
#define HHC02631 "%s complete"
#define HHC02632 "    Bad entry at line %d in file \"%s\""
#define HHC02633 "    %s"
#define HHC02634 "INTERNAL ERROR %s"
#define HHC02635 "Tape \"%s\" created"
#define HHC02636 "Invalid block encountered @ %d:%d"
#define HHC02637 "%s tape format set to %s"
//efine HHC02638 - HHC02644 (available)

// dmap2hrc
#define HHC02645 "Usage: %s infile\n" \
       "HHC02645I\n" \
       "HHC02645I   infile    name of P/390 DEVMAP input file\n" \
       "HHC02645I\n" \
       "HHC02645I The converted output is always written to stdout.\n" \
       "HHC02645I Use standard redirection to create a file instead.\n" \
       "HHC02645I\n" \
       "HHC02645I Return code:\n" \
       "HHC02645I\n" \
       "HHC02645I   0   Successful completion\n" \
       "HHC02645I   1   Invalid argument(s)\n" \
       "HHC02645I   2   Error opening input file\n" \
       "HHC02645I   3   Error reading header\n" \
       "HHC02645I   4   Error reading controller record\n" \
       "HHC02645I   5   Incomplete controller record\n" \
       "HHC02645I   6   Error reading device record\n" \
       "HHC02645I   7   Incomplete device record\n" \
       "HHC02645I   8   Premature end of input file"
#define HHC02646 "Error opening %s: %s"
#define HHC02647 "Error reading %s record%s from %s: %s"
#define HHC02648 "Incomplete %s record on %s"
#define HHC02649 "End of input file."
#define HHC02650 "Premature end of input file"
//efine HHC02651 - HHC02659 (available)

// dasdseq
#define HHC02660 "Usage: %s %s%s%s%s"
#define HHC02661 "Reading volume label..."
#define HHC02662 "reading Format 3 DSCB - CCHHR %04X%04X%02X"
#define HHC02663 "Error reading volume label, rc %d"
#define HHC02664 "Error: volume label is %d bytes long, not 80 bytes long"
#define HHC02665 "VTOC F4 at cyl %d head %d rec %d"
#define HHC02666 "Reading VTOC F4 DSCB..."
#define HHC02667 "Error reading F4 DSCB, rc %d"
#define HHC02668 "Erroneous key length %d or data length %d"
#define HHC02669 "%d extent(s) found; all are in Format 1 DSCB"
#define HHC02670 "VTOC format id byte invalid (DS4IDFMT) %2.2X,\n" \
       "HHC02670E VTOC key invalid, or multi-extent VTOC"
#define HHC02671 "VTOC start CCHH=%2.2X%2.2X %2.2X%2.2X end CCHH=%2.2X%2.2X %2.2X%2.2X"
#define HHC02672 "%s mode enabled"
#define HHC02673 "%s file%s '%s'"
#define HHC02674 "%s=%s"
#define HHC02675 "%s=%d"
#define HHC02676 "Absolute CC %d HH %d [%04X%04X] Track %d/%X"
#define HHC02677 "%s" // debug
#define HHC02678 "Dataset '%s'"
#define HHC02679 "Utility verbose %d"
#define HHC02680 "Dataset has %d extent(s):"
#define HHC02681 "%s" // verbose
#define HHC02682 "  %02X   %02X   %04X %04X %04X %04X"
#define HHC02683 "DS1LSTAR[%02X%02X%02X] lstartrack(%d) lstarrec(%d) lstarvalid(%d)"
#define HHC02684 "DS1LSTAR[%02X%02X%02X] track(%02X) record(%02X)"
#define HHC02685 "Convert TT %5.5d/x'%04X' CCHH[%04X%04X]"
#define HHC02686 "Reading track %5.5d/x'%04X' record %d/x'%X' CCHHR[%04X%04X%02X]"
#define HHC02687 "%s track %5.5d/x'%04X' rec %d"
#define HHC02688 "read %d bytes"
#define HHC02689 "offset %d length %d rec %d"
#define HHC02690 "buffer offset %d rec %d:"
#define HHC02691 "ascii> '%s'"
#define HHC02692 "searching VTOC for '%s'"
#define HHC02693 "search_key_equal rc %d"
#define HHC02694 "writing %s"
#define HHC02695 "Closed output file %s"
#define HHC02696 "Data set format is %s"
//efine HHC02697 - HHC02699 (available)

#define HHC02700 "SCSI tapes are not supported with this build"
#define HHC02701 "Abnormal termination"
#define HHC02702 "Tape %s: %smt_gstat 0x%8.8"PRIX32" %s"
#define HHC02703 "Tape %s: Error reading status: rc=%d, errno=%d: %s"
#define HHC02704 "End of tape"
#define HHC02705 "Tape %s: Error reading tape: errno=%d: %s"
#define HHC02706 "File %s: End of %s input"
#define HHC02707 "File %s: Error reading %s header: rc=%d, errno=%d: %s"
#define HHC02708 "File %s: Block too large for %s tape: block size=%d, maximum=%d"
#define HHC02709 "File %s: Error reading %s data block: rc=%d, errno=%d: %s"
#define HHC02710 "Tape %s: Error writing data block: rc=%d, errno=%d: %s"
#define HHC02711 "File %s: Error writing %s header: rc=%d, errno=%d: %s"
#define HHC02712 "File %s: Error writing %s data block: rc=%d, errno=%d: %s"
#define HHC02713 "Tape %s: Error writing tapemark: rc=%d, errno=%d: %s"
#define HHC02714 "File %s: Error writing %s tapemark: rc=%d, errno=%d: %s"
#define HHC02715 "Tape %s: Error opening: errno=%d: %s"
#define HHC02716 "Tape %s: Error setting attributes: rc=%d, errno=%d: %s"
#define HHC02717 "Tape %s: Error rewinding: rc=%d, errno=%d: %s"
#define HHC02718 "Tape %s: Device type%s"
#define HHC02719 "Tape %s: Device density%s"
#define HHC02720 "File %s: Error opening: errno=%d: %s"
#define HHC02721 "File No. %u: Blocks=%u, Bytes=%"PRId64", Block size min=%u, max=%u, avg=%u"
#define HHC02722 "Tape Label: %s"
#define HHC02723 "File No. %u: Block %u"
#define HHC02724 "Successful completion"
#define HHC02725 "Usage: %s infilename outfilename [nnn] [outfilename [nnn]] ...\n" \
       "HHC02725I 'nnn' specifies how many tapemark-separated files to copy to the\n" \
       "HHC02725I specified outfilename. If nnn is not given 32767 is used instead."
#define HHC02726 "Usage: %s filename"
//efine HHC02727 (available)
#define HHC02728 "Usage: %s [options] hetfile outfile fileno [recfm lrecl blksize]\n" \
       "HHC02728I   hetfile   input file\n" \
       "HHC02728I   outfile   output file\n" \
       "HHC02728I   fileno    which file to extract (1-9999, default 1)\n" \
       "HHC02728I Options:\n" \
       "HHC02728I   -a    convert to ASCII (implies -u)\n" \
       "HHC02728I   -h    display usage summary\n" \
       "HHC02728I   -n    file is an NL (or BLP like) tape\n" \
       "HHC02728I   -u    unblock (removes BDWs and RDWs if RECFM=V)\n" \
       "HHC02728I   -s    strip trailing blanks (requires -a)"
#define HHC02729 "Usage: %s [options] outfile [volser] [owner]\n" \
       "HHC02728I   outfile   output file\n" \
       "HHC02729I Options:\n" \
       "HHC02729I   -d        disable compression\n" \
       "HHC02729I   -h        display usage summary\n" \
       "HHC02729I   -i        create an IEHINITT formatted tape (default: on)\n" \
       "HHC02729I   -n        create an NL tape"
#define HHC02730 "Usage: %s [options] source [dest]\n" \
       "HHC02730I\n" \
       "HHC02730I   Options:\n" \
       "HHC02730I       -1    compress fast\n" \
       "HHC02730I             ...\n" \
       "HHC02730I       -9    compress best\n" \
       "HHC02730I %s" \
       "HHC02730I       -c n  set chunk size to \"n\"\n" \
       "HHC02730I       -d    decompress source tape\n" \
       "HHC02730I       -h    display usage summary\n" \
       "HHC02730I       -r    rechunk\n" \
       "HHC02730I       -s    strict AWSTAPE specification (chunksize=4096,no compression)\n" \
       "HHC02730I       -v    verbose (debug) information\n" \
       "HHC02730I       -z    use ZLIB compression\n"
#define HHC02731 "          (tapemark)"
#define HHC02732 "Bytes read:    %"PRId64" (%3.1f MB), Blocks=%u, avg=%u"
#define HHC02733 "Bytes written: %"PRId64" (%3.1f MB)"
//efine HHC02734 (available)
//efine HHC02735 (available)
//efine HHC02736 (available)
//efine HHC02737 (available)
//efine HHC02738 (available)
//efine HHC02739 (available)
#define HHC02740 "File %s: writing output file"
#define HHC02741 "File %s: Error, incomplete %s header"
#define HHC02742 "File %s: Error, incomplete final data block: expected %d bytes, read %d"
#define HHC02743 "File %s: Error, %s header block without data"
#define HHC02744 "File %s: Error renaming file to %s - manual intervention required"
#define HHC02745 "File %s: Error removing file - manual intervention required"
//efine HHC02746 (available)
//efine HHC02747 (available)
//efine HHC02748 (available)
//efine HHC02749 (available)
#define HHC02750 "DCB attributes required for NL tape"
#define HHC02751 "Valid record formats are:"
#define HHC02752 "%s"
#define HHC02753 "%s label missing"
#define HHC02754 "File Info:  DSN=%-17.17s"
#define HHC02755 "HET: Setting option %s to %s"
#define HHC02756 "HET: HETLIB reported error %s files; %s"
#define HHC02757 "HET: %s"
#define HHC02758 "Usage: %s [options] infile\n" \
       "HHC02758I   infile  name of input file\n" \
       "HHC02758I Options:\n" \
       "HHC02758I   -a      print all label and file information (default: on)\n" \
       "HHC02758I   -bn     print 'n' bytes per file; -b implies -s\n" \
       "HHC02758I   -d      print only dataset information (default: off)\n" \
       "HHC02758I   -f      print only file information (default: off)\n" \
       "HHC02758I   -h      display usage summary\n" \
       "HHC02758I   -l      print only label information (default: off)\n" \
       "HHC02758I   -s      print dump of each data file (SLANAL format) (default: off)\n" \
       "HHC02758I   -t      print TAPEMAP-compatible format output (default: off)\n"
#define HHC02759 \
    "Usage: %s filename\n"
#define HHC02760 "%s"
#define HHC02761 "DCB Attributes used:  RECFM=%-4.4s  LRECL=%-5.5d  BLKSIZE=%d"
//efine HHC02762 (available)
//efine HHC02763 (available)
//efine HHC02764 (available)
//efine HHC02765 (available)
//efine HHC02766 (available)
//efine HHC02767 (available)
//efine HHC02768 (available)
//efine HHC02769 (available)
#define HHC02770 "Usage:  ARG: value  [ARG: value]  [OPTION] ...\n" \
       "HHC02770I\n" \
       "HHC02770I      INPUT:    <input file name>  |  @<file containing file list>\n" \
       "HHC02770I      VOLSER:   <1 to 6 character volume serial number>\n" \
       "HHC02770I      DATASET:  <1 to 17 character dataset label>\n" \
       "HHC02770I      OUTPUT:   <output file name>\n" \
       "HHC02770I\n" \
       "HHC02770I    [ LRECL:    <record length>    ]\n" \
       "HHC02770I    [ BLOCK:    <blocking factor>  ]\n" \
       "HHC02770I    [ CODEPAGE: <code page>        ]\n" \
       "HHC02770I\n" \
       "HHC02770I    [ UNIQUE ]\n" \
       "HHC02770I    [ BINARY ]\n" \
       "HHC02770I    [ NLTAPE ]\n" \
       "HHC02770I    [ ANSI   ]\n"
#define HHC02771 "%s missing following %s parameter"
#define HHC02772 "Error opening %s file '%s': %s"
#define HHC02773 "Error opening %s file %i '%s': %s"
#define HHC02774 "Number of files in %s exceeds %d; excess ignored"
#define HHC02775 "Required %s omitted or not found"
#define HHC02776 "Error writing %s to file' %s': %s"
#define HHC02777 "Processing input from: '%s'"
#define HHC02778 "Wrote %i blocks to AWSTAPE file: '%s' (Seq #%.4s Dataset:'%.17s')"
#define HHC02779 "%s (C) copyright Jay Moseley, CCP 2000"
#define HHC02780 "Unsupported parameter: %s"
#define HHC02781 "Invalid %s parameter: %s"
#define HHC02782 "LRECL %i and BLOCK %i exceeds maximum AWS blocksize of %i"
#define HHC02783 "Parameter %s ignored due to NLTAPE option"
#define HHC02784 "Error reading from file' %s': %s"
//efine HHC02785 - HHC02799 (available)

// mt_cmd
#define HHC02800 "%1d:%04X %s complete"
#define HHC02801 "%1d:%04X %s failed; %s"
#define HHC02802 "%1d:%04X Current file number %d"
#define HHC02803 "%1d:%04X Current block number %d"
#define HHC02804 "%1d:%04X File protect enabled"
#define HHC02805 "%1d:%04X Volser = %s"
#define HHC02806 "%1d:%04X Unlabeled tape"
//efine HHC02806 - HHC02899 (available)

// range 029nn - 02949 console.c
#define HHC02900 "%s COMM: Send() failed: %s"
#define HHC02901 "%s COMM: Refusing client demand to %s %s"
#define HHC02902 "%s COMM: Client refused to %s %s"
#define HHC02903 "%s COMM: Libtelnet error"
#define HHC02904 "%s COMM: Libtelnet FATAL error"
#define HHC02905 "%s COMM: Libtelnet negotiation error"
#define HHC02906 "%s COMM: Unsupported libtelnet event '%s'"
#define HHC02907 "%s COMM: Libtelnet %s: %s in %s() at %s(%d): %s"
#define HHC02908 "%s COMM: Connection closed during negotiations"
#define HHC02909 "%s COMM: Recv() error during negotiations: %s"
#define HHC02910 "%s COMM: Unsupported terminal type: %s"
#define HHC02911 "%s COMM: Discarding premature data"
#define HHC02912 "%s COMM: Buffer overflow"
#define HHC02913 "%s COMM: Buffer overrun"
#define HHC02914 "%s COMM: %s negotiations complete; ttype = '%s'"
#define HHC02915 "%s COMM: Connection received"
#define HHC02916 "%s COMM: No acceptable terminal types"
#define HHC02917 "COMM: Switching from %s polling"
//efine HHC02918 - HHC02949 (available)

// convto64
#define HHC02950 "Usage: %s [-r] [-c] [-q] [-v] infile outfile"             "\n" \
       "HHC02950I   infile    input file"                                   "\n" \
       "HHC02950I   outfile   output file"                                  "\n" \
       "HHC02950I options:"                                                 "\n" \
       "HHC02950I   -r     Replace output file"                             "\n" \
       "HHC02950I   -c     Write contiguous L2 tables"                      "\n" \
       "HHC02950I   -q     Minimal progress messages"                       "\n" \
       "HHC02950I   -v     Report every track copied"
#define HHC02951 "Gathering L2 tables..."
#define HHC02952 "Copying L2 tables and associated track data..."
#define HHC02953 "Copying %s %d - %d ..."
#define HHC02954 "Writing cckd64 compressed device header..."
#define HHC02955 "Writing L1 table..."
#define HHC02956 "Copying %s %d data..."
#define HHC02957 "%"PRIu32" tracks copied"
#define HHC02958 "Error in function %s: %s"
#define HHC02959 "Syntax error: %s"
#define HHC02960 "Dasd image file format unsupported or unrecognized: %s"
#define HHC02961 "DASD operation completed"
#define HHC02962 "Converting \"%s\" to CCKD64 file format..."
//efine HHC02963 - HHC02974 (available)

// dasdpdsu
#define HHC02975 "Usage: %s infile [sf=shadow-file] pdsname [ASCII] [odir]" "\n" \
       "HHC02975I   infile    input file"                                   "\n" \
       "HHC02975I   pdsname   name of PDS to be unloaded"                   "\n" \
       "HHC02975I options:"                                                 "\n" \
       "HHC02975I   ASCII     unload as variable length ASCII text files"   "\n" \
       "HHC02975I             otherwise unloaded as fixed length EBCDIC"    "\n" \
       "HHC02975I   odir      directory where files should be written to"   "\n" \
       "HHC02975I             otherwise output is to current directory"     "\n"
//efine HHC02976 - HHC02999 (available)

// cckdmap
#define HHC03000 "Usage: %s [ -i | -t | -v ] infile"                        "\n" \
       "HHC03000I   infile   Input file"                                    "\n" \
       "HHC03000I options:"                                                 "\n" \
       "HHC03000I   -i       report summary information only"               "\n" \
       "HHC03000I   -t       report consolidated track data"                "\n" \
       "HHC03000I   -v       verbose (report detailed track data)"
#define HHC03001 "Unrecognized option: %s"
#define HHC03002 "Missing input-file specification"
#define HHC03003 "Extraneous parameter: %s"
#define HHC03004 "Unsupported dasd image file format"
#define HHC03005 "Device type '%2.2X' not found in dasd table"
#define HHC03006 "%s error: %s"
#define HHC03007 "File size:      (%s bytes)"
#define HHC03008 "Compressed device header inconsistency(s) found! code: %4.4X"
//efine HHC03009 - HHC03019 (available)
#define HHC03020 "" // (blank line)
#define HHC03021 "%s of:     \"%s\""
#define HHC03022                                                            "\n" \
       "HHC03022I   dh_devid:      %s        (%s-bit C%s%s %s)"             "\n" \
       "HHC03022I   dh_heads:      %u"                                      "\n" \
       "HHC03022I   dh_trksize:    %u"                                      "\n" \
       "HHC03022I   dh_devtyp:     0x%2.2X            (%s)"                 "\n" \
       "HHC03022I   dh_fileseq:    0x%2.2X"                                 "\n" \
       "HHC03022I   dh_highcyl:    %u"                                      "\n" \
       "HHC03022I   dh_serial:     %s"
#define HHC03023                                                            "\n" \
       "HHC03023I   cdh_vrm:       %u.%u.%u"                                "\n" \
       "HHC03023I   cdh_opts:      0x%2.2X"                                 "\n" \
       "HHC03023I   num_L1tab:     %"PRId32                                 "\n" \
       "HHC03023I   num_L2tab:     %"PRId32                                 "\n" \
       "HHC03023I   cdh_cyls:      %"PRIu32"           (%s tracks)"         "\n" \
       "HHC03023I   cdh_size:      0x%10.10"PRIX64"    (%*s bytes)"         "\n" \
       "HHC03023I   cdh_used:      0x%10.10"PRIX64"    (%*s bytes)"         "\n" \
       "HHC03023I   free_off:      0x%10.10"PRIX64"    (%s format)"         "\n" \
       "HHC03023I   free_total:    0x%10.10"PRIX64"    (%*s bytes)"         "\n" \
       "HHC03023I   free_largest:  0x%10.10"PRIX64"    (%*s bytes)"         "\n" \
       "HHC03023I   free_num:      %"PRId64                                 "\n" \
       "HHC03023I   free_imbed:    %"PRIu64                                 "\n" \
       "HHC03023I   cdh_nullfmt:   %u               (%s)"                   "\n" \
       "HHC03023I   cmp_algo:      %u               (%s)"                   "\n" \
       "HHC03023I   cmp_parm:      %"PRId16"              %s(%s)"
#define HHC03024                                                            "\n" \
       "HHC03024I   cdh_vrm:       %u.%u.%u"                                "\n" \
       "HHC03024I   cdh_opts:      0x%2.2X"                                 "\n" \
       "HHC03024I   num_L1tab:     %"PRId32                                 "\n" \
       "HHC03024I   num_L2tab:     %"PRId32                                 "\n" \
       "HHC03024I   cdh_cyls:      %"PRIu32"           (%s blkgrps)"        "\n" \
       "HHC03024I   cdh_size:      0x%10.10"PRIX64"    (%*s bytes)"         "\n" \
       "HHC03024I   cdh_used:      0x%10.10"PRIX64"    (%*s bytes)"         "\n" \
       "HHC03024I   free_off:      0x%10.10"PRIX64"    (%s format)"         "\n" \
       "HHC03024I   free_total:    0x%10.10"PRIX64"    (%*s bytes)"         "\n" \
       "HHC03024I   free_largest:  0x%10.10"PRIX64"    (%*s bytes)"         "\n" \
       "HHC03024I   free_num:      %"PRId64                                 "\n" \
       "HHC03024I   free_imbed:    %"PRIu64                                 "\n" \
       "HHC03024I   cdh_nullfmt:   %u               (%s)"                   "\n" \
       "HHC03024I   cmp_algo:      %u               (%s)"                   "\n" \
       "HHC03024I   cmp_parm:      %"PRId16"              %s(%s)"
#define HHC03040 "         File offset    Size (hex)         Size  track(s)"
#define HHC03041 "***********************************************************"
#define HHC03042 "%-8s 0x%10.10"PRIX64"  0x%10.10"PRIX64" %11"PRIu64"%s"
#define HHC03043 "Total active tracks    = %"PRIu32" tracks"
#define HHC03044 "Avg. L2-to-track seek  = %.3f MB"
#define HHC03045 "Total active blkgrps   = %"PRIu32" groups"
#define HHC03046 "Avg. L2-to-block seek  = %.3f MB"
#define HHC03047 "         File offset    Size (hex)         Size  group(s)"
#define HHC03048                                                            "\n" \
       "HHC03048I   dh_devid:      %s        (%s-bit C%s%s %s)"             "\n" \
       "HHC03048I   dh_heads:      %u         (total sectors)"              "\n" \
       "HHC03048I   dh_trksize:    %u             (sector size)"            "\n" \
       "HHC03048I   dh_devtyp:     0x%2.2X            (%s)"                 "\n" \
       "HHC03048I   dh_fileseq:    0x%2.2X"                                 "\n" \
       "HHC03048I   dh_highcyl:    %u"                                      "\n" \
       "HHC03048I   dh_serial:     %s"
#define HHC03049 "Total unknown space    = %s bytes"
#define HHC03050 "Image is %s fragmented%s"
//efine HHC03051 - HHC03099 (available)

// dasdser
#define HHC03100 "Usage: %s  image  [serial]"                               "\n" \
       "HHC03100I   image    base CKD dasd image file"                      "\n" \
       "HHC03100I   serial   serial number to set. If not given then the"   "\n" \
       "HHC03100I            existing serial number is simply displayed."
#define HHC03101 "Invalid %s \"%s\""
#define HHC03102 "Error in function %s: %s"
#define HHC03103 "Unsupported dasd image file format"
//efine HHC03104 - HHC03199 (available)

// tfprint
#define HHC03200 "Usage:  %s  [options...] tracefile\n"                          \
       "HHC03200I\n"                                                             \
       "HHC03200I        [-i|--info]\n"                                          \
       "HHC03200I        [-c|--cpu hh[[-hh][,hh]]\n"                             \
       "HHC03200I        [-r|--traceopt TRADITIONAL|REGSFIRST|NOREGS]\n"         \
       "HHC03200I        [-n|--count nnnnnn[[-nnnnnn]|[.nnn]]\n"                 \
       "HHC03200I        [-e|--msg nnnnn[,nnnnn]\n"                              \
       "HHC03200I        [-s|--storage V|R:hhhhhh[[-hhhhhh]|[.hhh]]\n"           \
       "HHC03200I        [-d|--date YYYY/MM/DD-YYYY/MM/DD\n"                     \
       "HHC03200I        [-t|--time HH:MM:SS.nnnnnn-HH:MM:SS.nnnnnn\n"           \
       "HHC03200I        [-o|--opcode hhhhhhhh[,hhxxhhxxhhhh]\n"                 \
       "HHC03200I        [-m|--msglvl xxxxx\n"                                   \
       "HHC03200I        [-u|--unit uuuu[[-uuuu]|[.nnn]]\n"                      \
       "HHC03200I        [-p|--codepage zzzzzzzz\n"                              \
       "HHC03200I\n"                                                             \
       "HHC03200I   -i   Print only TFSYS header information then exit\n"        \
       "HHC03200I   -c   Print only specified CPU(s)\n"                          \
       "HHC03200I   -r   Print registers trace option\n"                         \
       "HHC03200I   -n   Print only records nnnnnn to nnnnnn (by count)\n"       \
       "HHC03200I   -e   Print only messages with specified message number\n"    \
       "HHC03200I   -s   Print only instructions that reference or modify\n"     \
       "HHC03200I        the specified 'V'irtual or 'R'eal storage range\n"      \
       "HHC03200I   -d   Print only records within specified date range\n"       \
       "HHC03200I   -t   Print only records within specified time range\n"       \
       "HHC03200I   -o   Print only specified instruction(s) (by opcode)\n"      \
       "HHC03200I        Only as many bytes are checked as are specified\n"      \
       "HHC03200I        Use 'x' as wildcard for nibbles not to be checked\n"    \
       "HHC03200I   -m   Modify Hercules 'msglvl' setting\n"                     \
       "HHC03200I   -u   Print only trace records for specified I/O unit(s)\n"   \
       "HHC03200I   -p   Use EBCDIC/ASCII codepage conversion table zzzzzzzz"

#define HHC03201 "Missing input-file specification"
#define HHC03202 "Error opening \"%s\": %s"
#define HHC03203 "Option %s has not been implemented yet!"
#define HHC03204 "Missing argument for option %s"
#define HHC03205 "Option \"%s\" value \"%s\" is invalid"
#define HHC03206 "Error reading trace file: %s"
#define HHC03207 "Truncated %s record; aborting"
#define HHC03208 "Format-%c trace file created by: %s"
#define HHC03209 "Trace %s: %s"
#define HHC03210 "Incompatible MAX_CPU_ENGS"
#define HHC03211 "Trace count: instruction=%s records, device=%s records"
#define HHC03212 "File does not start with TFSYS record; aborting"
#define HHC03213 "Unsupported Trace File format: %%TF%c"
#define HHC03214 "Unsupported Trace File record: msgnum %"PRIu16
#define HHC03215 "%s %s printed"
#define HHC03216 "Incompatible Hercules build architectures"
#define HHC03217 "Args: %s"
#define HHC03218 "Unsupported external interrupt: processor %s, code %4.4"PRIX16
#define HHC03219 "Invalid/unsupported option: %s"
#define HHC03220 "--date range is required when --time range specified"
#define HHC03221 "Endianness of %s = %s"
#define HHC03222 "WARNING: possible performance impact due to endianness!"
#define HHC03223 "Thread Id "TIDPAT" is %s"
//efine HHC03224 - HHC03249 (available)

// tfswap
#define HHC03250 "Usage:  tfswap  infile  outfile\n"
#define HHC03251 "Error opening \"%s\": %s"
#define HHC03252 "Error reading trace file: %s"
#define HHC03253 "Truncated %s record; aborting"
#define HHC03254 "File successfully swapped from %s endian to %s endian"
//efine HHC03255 - HHC03299 (available)

// txt2card
#define HHC03300 "Usage:  txt2card  codepage  infile  outfile"
#define HHC03301 "Incorrect number of arguments"
#define HHC03302 "Invalid/unsupported codepage"
#define HHC03303 "Error opening \"%s\": %s"
#define HHC03304 "I/O error on file \"%s\": %s"
//efine HHC03305 - HHC03349 (available)

//efine HHC03350 - HHC03399 (available)

// range 03400 - 03499 available
// range 03500 - 03599 available
// range 03600 - 03699 available
// range 03700 - 03799 available

// range 03800 - 03849 for qeth related messages
#define HHC03800 "%1d:%04X %s: Adapter mode set to %s"
#define HHC03801 "%1d:%04X %s: %s: Register guest MAC address %s"
#define HHC03802 "%1d:%04X %s: %s: Cannot register guest MAC address %s"
#define HHC03803 "%1d:%04X %s: %s: Unregister guest MAC address %s"
#define HHC03804 "%1d:%04X %s: %s: Cannot unregister guest MAC address %s"
#define HHC03805 "%1d:%04X %s: %s: Register guest IP address %s"
#define HHC03806 "%1d:%04X %s: %s: Cannot register guest IP address %s"
#define HHC03807 "%1d:%04X %s: %s: Unregister guest IP address %s"
#define HHC03808 "%1d:%04X %s: %s: Cannot unregister guest IP address %s"
#define HHC03809 "%1d:%04X %s: %s: Requested MTU %s too large; decreasing to %s bytes"
#define HHC03810 "%1d:%04X %s: %s: Input dropped: %s"
#define HHC03811 "%1d:%04X %s: %s: Output dropped: %s"
//efine HHC03812 - HHC03849 (available)

//efine HHC03850 - HHC03899 (available)

// reserve 039xx for ptp related messages
#define HHC03901 "%1d:%04X PTP: Guest and driver IP addresses are the same"
#define HHC03902 "%1d:%04X PTP: Inet6 not supported"
#define HHC03910 "%1d:%04X PTP: Hercules has maximum read length of size %d bytes and actual MTU of size %d bytes"
#define HHC03911 "%1d:%04X PTP: Guest has maximum read length of size %d bytes and actual MTU of size %d bytes"
#define HHC03912 "%1d:%04X PTP: Guest has the driver IP address %s"
#define HHC03913 "%1d:%04X PTP: Guest has IP address %s"
#define HHC03914 "%1d:%04X PTP: Guest has IP address %s"
#define HHC03915 "%1d:%04X PTP: Connection active to guest IP address %s"
#define HHC03916 "%1d:%04X PTP: Connection cleared to guest IP address %s"
#define HHC03917 "%1d:%04X PTP: Guest read and write paths mis-configured"
#define HHC03918 "%1d:%04X PTP: MTU changed from size %d bytes to size %d bytes"
#define HHC03921 "%1d:%04X PTP: Packet of size %d bytes from device %s has an unknown IP version, packet dropped"
#define HHC03922 "%1d:%04X PTP: Packet of size %d bytes from device %s is not equal to the packet length of %d bytes, packet dropped"
#define HHC03923 "%1d:%04X PTP: Packet of size %d bytes from device %s is larger than the guests actual MTU of %d bytes, packet dropped"
#define HHC03924 "%1d:%04X PTP: Packet of size %d bytes from device %s is too large for read buffer area of %d bytes, packet dropped"
#define HHC03931 "%1d:%04X PTP: Accept data of size %d bytes contains unknown data"
#define HHC03933 "%1d:%04X PTP: Accept data for device %s contains IP packet with unknown IP version, data dropped"
#define HHC03934 "%1d:%04X PTP: Accept data for device %s contains incomplete IP packet, data dropped"
#define HHC03935 "%1d:%04X PTP: Accept data for device %s contains IP packet larger than MTU, data dropped"
#define HHC03936 "%1d:%04X PTP: Accept data contains unknown %s"
#define HHC03937 "%1d:%04X PTP: Accept data contains %s that does not contain expected %s"
#define HHC03952 "%1d:%04X PTP: MAC: %s"
#define HHC03953 "%1d:%04X PTP: IPv4: Drive %s/%s (%s): Guest %s"
#define HHC03954 "%1d:%04X PTP: IPv6: Drive %s/%s %s/%s: Guest %s"
#define HHC03965 "%id:%04X %s: Preconfigured interface %s does not exist or is not accessible by Hercules"
#define HHC03981 "%1d:%04X %s: %s: %s %s  %s"
#define HHC03982 "%s: %s %s  %s"
#define HHC03983 "%1d:%04X %s: %s"
#define HHC03984 "%s"
#define HHC03985 "%1d:%04X %s: %s"
#define HHC03991 "%1d:%04X %s: %s"
#define HHC03992 "%1d:%04X %s: Code %02X: Flags %02X: Count %08X: Chained %02X: PrevCode %02X: CCWseq %d"
#define HHC03993 "%1d:%04X %s: Status %02X: Residual %08X: More %02X"
#define HHC03994 "%1d:%04X %s: Status %02X"
#define HHC03995 "%1d:%04X %s: %s:\n%s"
#define HHC03996 "%1d:%04X %s: %s: %s"
#define HHC03997 "%1d:%04X %s: %s: %susing %s %s"
#define HHC03998 "%1d:%04X %s: %s inconsistent with %s"
//efine HHC03999 (available)

// reserve 040xx for tcpnje related messages
#define HHC04000 "Error in function %s: %s"
//efine HHC040xx - HHC04099 (available)

// reserve 041xx for windows specific component messages (w32xxxx.c)
#define HHC04100 "%s version %s initiated"
#define HHC04101 "%s Statistics:"
#define HHC04102 "One of the GetProcAddress calls failed"
#define HHC04103 "  %s%5luK"
#define HHC04104 "  %12"PRId64"  %s"
//efine HHC04105 (available)
//efine HHC04106 (available)
//efine HHC04107 (available)
//efine HHC04108 (available)
//efine HHC04109 (available)
#define HHC04110 "DuplicateHandle() failed: dwLastError=%d (0x%8.8X): %s"
//efine HHC04111 (available)
#define HHC04112 "Cannot provide minimum emulated TOD clock resolution"
//efine HHC04113 - HHC04199 (available)

// range 04200 - 04299 available
// range 04300 - 04399 available
// range 04400 - 04499 available
// range 04500 - 04599 available
// range 04600 - 04699 available
// range 04700 - 04799 available
// range 04800 - 04899 available
// range 04900 - 04999 available

// reserve 050xx for CTCE related messages
#define HHC05050 "%1d:%04X CTCE: Error creating %s socket: %s"
#define HHC05051 "%1d:%04X CTCE: %s error for %s socket (port %d): %s"
#define HHC05052 "%1d:%04X CTCE: Error binding to %s socket (port %d): %s"
#define HHC05053 "%1d:%04X CTCE: Error on getsockname for %s socket (port %d): %s"
#define HHC05054 "%1d:%04X CTCE: %s outbound connection :%5d -> %1d:%04X=%s:%d"
#define HHC05055 "%1d:%04X CTCE: Missing (at least) remote CTCE IP address parameter"
#define HHC05056 "%1d:%04X CTCE: Local port number outside range 1024-65534: %s"
#define HHC05057 "%1d:%04X CTCE: Remote CCUU address outside range 0001-FFFF: %4X"
#define HHC05058 "%1d:%04X CTCE: Invalid IP address %s"
#define HHC05059 "%1d:%04X CTCE: Invalid port number: %s"
#define HHC05060 "%1d:%04X CTCE: Both remote listening port and remote CCUU are missing; at least one is required."
#define HHC05061 "%1d:%04X CTCE: Invalid MTU size %s, allowed range is %d to 65536"
#define HHC05062 "%1d:%04X CTCE: Invalid Small MTU size %s ignored"
#define HHC05063 "%1d:%04X CTCE: Awaiting inbound connection :%5d <- %s"
#define HHC05064 "%1d:%04X CTCE: Extraneous parameters ignored : %s ..."
#define HHC05065 "%1d:%04X CTCE: Attempt outbound connection :%5d -> %1d:%04X=%s:%d"
#define HHC05066 "%1d:%04X CTCE: Error on call to listen (port=%d): %s"
#define HHC05067 "%1d:%04X CTCE: Ignoring non matching connection from %1d:%04X=%s:%d"
#define HHC05068 "%1d:%04X CTCE: TCP_NODELAY error for listening socket %d (port %d): %s"
#define HHC05069 "%1d:%04X CTCE: create_thread %s error: %s"
#define HHC05070 "%1d:%04X CTCE: %s inbound connection :%5d <- %1d:%04X=%s:%5d (bufsize=%d,%d)"
#define HHC05071 "%1d:%04X CTCE: Internal error, SEND status incorrectly encoded !"
#define HHC05072 "%1d:%04X CTCE: Still attempting connection :%5d -> %1d:%04X=%s:%d%s"
#define HHC05073 "%1d:%04X CTCE: bufsize parameter %d is too small; increase at least to %d"
#define HHC05074 "%1d:%04X CTCE: Error writing to %s: %s"
#define HHC05075 "%1d:%04X CTCE: Initial write_socket :%d -> %1d:%04X=%s:%d ; rc=%d!=%d ; error = %s"
#define HHC05076 "%1d:%04X CTCE: Connection closed; %"PRIu64" MB received in %"PRIu64" packets from %s; shutdown=%d"
#define HHC05077 "%1d:%04X CTCE: Error reading from %s: %s"
#define HHC05078 "%1d:%04X CTCE: Aborted outbound connection :%5d -> %1d:%04X=%s:%d"
#define HHC05079 "%1d:%04X CTCE: %s %.6s #%04X cmd=%s=%02X xy=%.2s%s%.2s l=%04X k=%08X %s%s%s%s%s%s"
#define HHC05080 "%1d:%04X CTCE: Create listen thread %s error: %s"
#define HHC05081 "%1d:%04X CTCE: Already awaiting connection :%5d <- %s"
#define HHC05082 "%1d:%04X CTCE: TCP set_socket_keepalive RC=%d"
#define HHC05083 "%1d:%04X CTCE: Error on accept() for listening socket %d (port %d): %s"
//efine HHC05084 (available)
#define HHC05085 "%1d:%04X CTCE: Invalid ATTNDELAY value %s ignored"
#define HHC05086 "%1d:%04X CTCE: Recovery is about to issue Hercules command: %s %s"

//efine HHC05087 - HHC05099 (available)

// range 05100 - 05199 available
// range 05200 - 05299 available
// range 05300 - 05399 available
// range 05400 - 05499 available
// range 05500 - 05599 available
// range 05600 - 05699 available
// range 05700 - 05799 available
// range 05800 - 05899 available
// range 05900 - 05999 available

// range 06000 - 06999 available
// range 07000 - 07999 available
// range 08000 - 08999 available
// range 09000 - 09999 available

// range 10000 - 10999 available
// range 11000 - 11999 available
// range 12000 - 12999 available
// range 13000 - 13999 available
// range 14000 - 14999 available
// range 15000 - 15999 available
// range 16000 - 16999 available

// reserve 17000-17499 messages command processing
#define HHC17000 "Missing or invalid argument(s)"
#define HHC17001 "%s server %slistening %s"
#define HHC17002 "%s server inactive"
#define HHC17003 "%-8s storage is %s (%ssize); storage is %slocked"
#define HHC17004 "%-6s: CPUID  = %16.16"PRIX64
#define HHC17005 "%-6s: CPC SI = %4.4X.%s.%s.%s.%s"
#define HHC17006 "MAINSIZE %screased to %s architectural %simum"
#define HHC17007 "NumCPU = %2.2d, NumVEC = %2.2d, ReservedCPU = %2.2d, MaxCPU = %2.2d"
#define HHC17008 "Avgproc  %2.2d %3.3d%%; MIPS[%4d.%2.2d]; SIOS[%6d]%s"
#define HHC17009 "PROC %s%2.2X %c %3.3d%%; MIPS[%4d.%2.2d]; SIOS[%6d]%s"
#define HHC17010 " - Started        : Stopping        * Stopped"
//efine HHC17011 (available)
#define HHC17012 "MSGLEVEL = %s"
#define HHC17013 "Process ID = %d"
#define HHC17014 "%s value is invalid; valid range is %d - %d"
#define HHC17015 "%s support not included in this engine build"
//efine HHC17016 - HHC17099 (available)

//efine HHC17100 - HHC17198 (available)
#define HHC17199 "%.4s %s"

// range 17200 - 17299 available
// range 17300 - 17399 available
// range 17400 - 17499 available

// Reserve 17500-17549 for Hercules Rexx messages
#define HHC17500 "REXX(%s) %s" // DisplayOptions
#define HHC17501 "REXX(%s) Rexx version/source retrieval failed: RC(%d)"
#define HHC17502 "REXX(%s) %s RC(%d)"
#define HHC17503 "REXX(%s) %s %s RetRC(%hd)"
#define HHC17504 "REXX(%s) %s %s RetValue(\"%s\")"
#define HHC17505 "REXX(%s) Exec: script name not specified"
#define HHC17506 "REXX(%s) Exec: script \"%s\" not found"
#define HHC17507 "REXX(%s) Option '%s' needs a value"
#define HHC17508 "REXX(%s) Argument %d: unknown option '%s'"
#define HHC17509 "REXX(%s) Option %s value '%s' is invalid"
//efine HHC17510 (available)
#define HHC17511 "REXX(%s) Could not enable %sRexx package"
#define HHC17512 "REXX(%s) %s: return code 0x%8.8"PRIX32" from %s"
#define HHC17513 "REXX(%s) Malformed call to 'exec_cmd' function"
#define HHC17514 "%s" // "Started  Package ThreadId  Mode Name..."
#define HHC17515 "%8s %6s  "TIDPAT"  %3s  %s"
#define HHC17516 "No asynchronous Rexx scripts are currently running"
#define HHC17517 "Asynchronous Rexx script "TIDPAT" not found"
#define HHC17518 "REXX(%s) %s: The target Rexx procedure was not found"
#define HHC17519 "REXX(%s) %s: A failure in Rexx processing has occurred"
#define HHC17520 "REXX(%s) Signal HALT "TIDPAT" %s"
//efine HHC17521 (available)
#define HHC17522 "REXX(%s) Rexx already started/enabled"
#define HHC17523 "REXX(%s) Rexx already stopped/disabled"
//efine HHC17524 (available)
#define HHC17525 "REXX(%s) Rexx has been started/enabled"
#define HHC17526 "REXX(%s) Rexx has been stopped/disabled"
#define HHC17527 "REXX(%s) Unknown/unsupported Rexx package '%s'"
#define HHC17528 "REXX(%s) VERSION: %s"
#define HHC17529 "REXX(%s) SOURCE:  %s"
//efine HHC17530 (available)
#define HHC17531 "REXX(%s) dlopen '%s' failed: %s"
//efine HHC17532 (available)
#define HHC17533 "REXX(%s) dlsym '%s' failed: %s"
//efine HHC17534 - HHC17539 (available)
#define HHC17540 "%s" // ExitHandler RXSIO RXSIOSAY Perform SAY Clause
#define HHC17541 "%s" // ExitHandler RXSIO RXSIOTRC Write Trace Output
//efine HHC17542 - HHC17549 (available)

// range 17550 - 17599 available
// range 17600 - 17699 available

// Reserve 17700-17749 for TXF related messages
#define HHC17700 "TXF: %s%02X: %sSuccessful %s Nested TEND for TND %d => %d"
#define HHC17701 "TXF: %s%02X: %sSuccessful Outermost %s TEND"
#define HHC17702 "TXF: %s%02X: %sSuccessful %s TBEGIN%s; TND now %d"
#define HHC17703 "TXF: %s%02X: %sFailed %s %s Transaction for TND %d: %s = %s, why =%s"
#define HHC17704 "TXF: %s%02X: %svirt 0x%16.16"PRIX64", abs 0x%16.16"PRIX64", alt 0x%16.16"PRIX64
#define HHC17705 "TXF: %s%02X: %sThere now:  +"
#define HHC17706 "TXF: %s%02X: %sWe fetched: +"
#define HHC17707 "TXF: %s%02X: %sWe stored:  +"
#define HHC17708 "TXF: %s%02X: %sError in function %s: %s"
#define HHC17709 "TXF: %s%02X: %s%s dump of TDB:"
#define HHC17710 "TXF: %s%02X: %s+"
//efine HHC17711 (available)
//efine HHC17712 (available)
#define HHC17713 "TXF: %s%02X: %sTranslation exception %4.4hX (%s) for TBEGIN tdba 0x%16.16"PRIx64
#define HHC17714 "TXF: %s%02X: SIE: Populating Interception TDB at 0x%16.16"PRIx64
#define HHC17715 "TXF: %s%02X: SIE: Intercepting %s instruction"
#define HHC17716 "TXF: %s%02X: SIE: Interception TDB address not provided!"
//  HHC17717 ... HHC17722 = MLVL( VERBOSE ) only
#define HHC17717 "TXF: %s%02X: %s%s transaction retry #%d..."
#define HHC17718 "TXF: %s%02X: %s%s transaction succeeded after %d retries"
#define HHC17719 "TXF: %s%02X: %s%s transaction retry #%d FAILED!"
#define HHC17720 "TXF: %s%02X: %sAbort set by %s at %s"
#define HHC17721 "TXF: %s%02X: %s%s"
//efine HHC17722 - HHC17729 (available)
#define HHC17730 "Total %s Transactions =%12"PRIu64
#define HHC17731 "Retries for ANY/ALL reason(s):"
#define HHC17732 "%1d%cretries =%12"PRIu64"  (%4.1f%%)"
#define HHC17733 "MAXIMUM   =%12"PRIu64
#define HHC17734 "%12"PRIu64"  (%4.1f%%)  Retries due to TAC %3d %s"
#define HHC17735 "%12"PRIu64"  (%4.1f%%)  Retries due to other TAC"
#define HHC17736 "TXF: TIMERINT %d is too small; using default of %d instead"
//efine HHC17737 - HHC17749 (available)

// range 17750 - 17799 available
// range 17800 - 17899 available
// range 17900 - 17999 available

// range 18000 - 18999 available
// range 19000 - 19999 available

// range 20000 - 29999 available
// range 30000 - 39999 available
// range 40000 - 49999 available
// range 50000 - 59999 available
// range 60000 - 69999 available
// range 70000 - 79999 available
// range 80000 - 89999 available

// reserve 90000 messages for debugging
#define HHC90000 "DBG: %s"
//efine HHC90001 (available)
//efine HHC90002 (available)
//efine HHC90003 (available)
//efine HHC90004 (available)
//efine HHC90005 (available)
//efine HHC90006 (available)
//efine HHC90007 (available)
//efine HHC90008 (available)
//efine HHC90009 (available)

/* hthreads.c, pttrace.c */
#define HHC90010 "Pttrace: trace is busy"
#define HHC90011 "Pttrace: invalid argument %s"
#define HHC90012 "Pttrace: %s %s %s %s %s to=%d %d"
#define HHC90013 "'%s(%s)' failed: rc=%d: %s; tid="TIDPAT", loc=%s"
#define HHC90014 "lock %s was %s by thread "TIDPAT" at %s"
#define HHC90015 "Thread "TIDPAT" (%s) abandoned lock %s obtained on %s at %s"
#define HHC90016 "Thread "TIDPAT" (%s) abandoned at %s lock %s obtained on %s at %s"
#define HHC90017 "Lock "PTR_FMTx" (%s) created by "TIDPAT" (%s) on %s at %s"
#define HHC90018 "Total locks defined: %d"
#define HHC90019 "No locks found for thread "TIDPAT"."
#define HHC90020 "'%s' failed at loc=%s: rc=%d: %s"
#define HHC90021 "%s "TIDPAT" %-15.15s %-18.18s %-18.18s"PTR_FMTx" "PTR_FMTx" %s%s"
#define HHC90022 "Thread %-15.15s tid="TIDPAT" created on %s at %-18.18s"
#define HHC90023 "Thread %-15.15s tid="TIDPAT" waiting since %s at %s for lock %s = "PTR_FMTx
#define HHC90024 "DEADLOCK!"
#define HHC90025 "Thread %s is stuck waiting for lock %s held by thread %s"
#define HHC90026 "No threads found with tid "TIDPAT"."
#define HHC90027 "Total threads running: %d"
#define HHC90028 "lock %s was already initialized at %s"
#define HHC90029 "Lock "PTR_FMTx" (%s) obtained by "TIDPAT" (%s) on %s at %s"
//efine HHC90030 - HHC90099 (available)

/* from crypto/dyncrypt.c when compiled with debug on */
#define HHC90100 "%s"
#define HHC90101 "  r%d        : GR%02d"
#define HHC90102 "    address : " F_VADR
#define HHC90103 "    length  : " F_GREG
#define HHC90104 "  GR%02d      : " F_GREG
#define HHC90105 "    bit 56  : %s"
#define HHC90106 "    fc      : %d"
#define HHC90107 "    m       : %s"
#define HHC90108 "  GR%02d  : " F_GREG
#define HHC90109 "  %s %s"
#define HHC90110 "      %s"
#define HHC90111 "Wrapping Verification Pattern does not match, return with cc1"
#define HHC90112 "    lcfb    : %d"
//efine HHC90112 - HHC90189 (available)

/* from crypto.c when compiled with debug on */
#define HHC90190 "%s"
//efine HHC90191 - HHC90199 (available)

// scsitape.c
//efine HHC90200 (available)
//efine HHC90201 (available)
//efine HHC90202 (available)
//efine HHC90203 (available)
//efine HHC90204 (available)
#define HHC90205 "%1d:%04X Tape file %s, type %s: error in function %s: %s"
//efine HHC90206 - HHC90299 (available)

// range 90300 - 90399 available

/* cckddiag.c */
#define HHC90400 "MAKBUF() malloc %s buffer of %d bytes at %p"
#define HHC90401 "READPOS seeking %"PRId64" (0x%16.16"PRIX64")"
#define HHC90402 "READPOS reading buf addr %p length %d (0x%X)"
#define HHC90403 "SHOW%s Compressed %s header and data:"
#define HHC90404 "SHOW%s Decompressed %s header and data:"
#define HHC90405 "OFFTIFY hex string '%s' = 0x%16.16"PRIX64", dec %"PRId64"."
#define HHC90406 "OFFTIFY dec string '%s' = 0x%16.16"PRIX64", dec %"PRId64"."
#define HHC90407 "%s device has %d heads/cylinder"
//efine HHC90408 - HHC90499 (available)

// range 90500 - 90549 console.c DEBUGGING messages
#define HHC90500 "%s COMM: Sending %d bytes"
#define HHC90501 "%s COMM: Received %d bytes"
#define HHC90502 "%s COMM: recv_3270_data: %d bytes received"
//efine HHC90503            (available)
#define HHC90504 "%s COMM: Disconnected"
#define HHC90505 "%s COMM: Received connection from %s"
#define HHC90506 "%s COMM: Device attention %s; rc=%d"
#define HHC90507 "%s COMM: recv() failed: %s"
#define HHC90508 "COMM: pselect() failed: %s"
#define HHC90509 "COMM: accept() failed: %s"
#define HHC90510 "%s COMM: setsockopt() failed: %s"
#define HHC90511 "%s COMM: Negotiating %-14s %s"
#define HHC90512 "%s COMM: Received IAC %s"
//efine HHC90513 - HHC90549 (available)

// range 90550 - 90599 available
// range 90600 - 90699 available
// range 90700 - 90799 available
// range 90800 - 90899 available

//efine HHC90900 - HHC90998 (available)
//efine HHC90999 (dbgtrace.h)

//efine HHC91900 - HHC91998 (available)
//efine HHC91999  (dbgtrace.h)

//efine HHC92000 - HHC92699 (available)

//efine HHC92700 (available)
//efine HHC92701 (available)
#define HHC92702 "Tape %s: %smt_gstat 0x%8.8"PRIX32" %s" // scsitape.c
//efine HHC92703 - HHC92799 (available)

// range 92800 - 92899 available
// range 92900 - 92999 available

// range 93000 - 93999 available
// range 94000 - 94999 available
// range 95000 - 95999 available
// range 96000 - 96999 available
// range 97000 - 97999 available
// range 98000 - 98999 available
// range 99000 - 99999 available

#endif // _MSGENU_H_
