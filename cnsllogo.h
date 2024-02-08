/* CNSLLOGO.H   (c)Copyright Roger Bowler, 1999-2012                 */
/*              (C) and others 2013-2023                             */
/*              Hercules Console Logo                                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* This is the default LOGO */

#ifndef _CNSLLOGO_H_
#define _CNSLLOGO_H_

/* The following is an extract from the README.HERCLOGO file

Each line in the array represent either an order or a plain text line.

The orders are as follows :

@SBA X,Y
Position the current buffer position to Row X col Y (X and Y start at 0)

@SF [H][P]
Set the Highlight and/or Protected attribute

@NL
Forces going to the next line

@ALIGN NONE|LEFT|RIGHT|CENTER
Specified the text alignement (relative to the left and right borders of the
terminal). When ALIGN is other than "NONE", a new line is automatically
inserted after each line of text. If ALIGN is "NONE", then the text will
be written without skipping to the next line.

It is also possible to embbed substitution variables in outgoing text.
Substition is indicated by enclosing the variable name between $( and ).

The following variables are defined in that environment :

VERSION : The hercules version
HOSTNAME : The host name on which hercules is running
HOSTOS : The host operating system
HOSTOSREL : The Host operating system release
HOSTOSVER : The host operating system version
HOSTARCH : The host architecture
HOSTNUMCPUS : UP (for 1) or MP=X (for more than 1)
LPARNAME : The LPAR name specified in the configuration file
CCUU,ccuu,CUU,cuu : Various forms of the device number of the terminal
CSS : The Logical Channel Subsystem Set or Channel Set for the terminal
SUBCHAN : The Subchannel number for the terminal

It is also possible to use any defined symbol or environment variable.

 */

static char *herclogo[]={
"@ALIGN NONE",
"@SBA 0,0",
"@SF P",
"Hercules Version  :",
"@SF HP",
"$(VERSION)",
"@NL",
"@SF P",
"Host name         :",
"@SF HP",
"$(HOSTNAME)",
"@NL",
"@SF P",
"Host OS           :",
"@SF HP",

#if defined( OPTION_LONG_HOSTINFO )
"$(HOSTOS)-$(HOSTOSREL) $(HOSTOSVER)",
#else
"$(HOSTOS)-$(HOSTOSREL)",
#endif

"@NL",
"@SF P",
"Host Architecture :",
"@SF HP",
"$(HOSTARCH)",
"@NL",
"@SF P",
"Processors        :",
"@SF HP",
"$(HOSTNUMCPUS)",
"@NL",
"@SF P",
"LPAR Name         :",
"@SF HP",
"$(LPARNAME)",
"@NL",
"@SF P",
"Device number     :",
"@SF HP",
"$(CSS):$(CCUU)",
"@NL",
"@SF P",
"Subchannel        :",
"@SF HP",
"$(SUBCHAN)",
"@SF P",

"@ALIGN LEFT",
"",
"",
"         The Hercules S/370, ESA/390 and z/Architecture Emulator",
"                          SDL 4.x Hyperion",
"           _    _                            _",
"          | |  | |                          (_)",
"          | |__| | _   _  _ __    ___  _ __  _   ___   _ __",
"          |  __  || | | || '_ \\  / _ \\| '__|| | / _ \\ | '_ \\",
"          | |  | || |_| || |_) ||  __/| |   | || (_) || | | |",
"          |_|  |_| \\__, || .__/  \\___||_|   |_| \\___/ |_| |_|",
"                    __/ || |",
"                   |___/ |_|",
"",
"                   My PC thinks it's a MAINFRAME!",
"",
"       Copyright (C) 1999-2024 Roger Bowler, Jan Jaeger, and others"
};

#endif /* #ifndef _CNSLLOGO_H_ */
