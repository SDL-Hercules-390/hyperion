![test image](images/image_header_herculeshyperionSDL.png)  
[Return to master README.md](../README.md)  

# Hardware Management Console Watchdog Timer (Diag 288)

## Contents

1. [Introduction](#Introduction)  
2. [States](#States)  
3. [Command](#Command)  
4. [Diag 0x288](#Diag-0x288)  
5. [State Transitions](#State-Transitions)  
6. [z/Linux Watchdog Timer Setup](#z/Linux-Watchdog-Timer-Setup)  

## Introduction

The Hardware Management Console (HMC) implements a LPAR Watchdog Timer (Diag 0x288) which a guest OS can use to recognize some
failure requiring a LPAR (Hercules) action to restore the guest OS to an operating state.
Z/Linux is a guest OS that has built-in support for the Watchdog Timer.

Linux source ([drivers/watchdog/diag288_wdt.c](https://github.com/torvalds/linux/blob/6916d5703ddf9a38f1f6c2cc793381a24ee914c6/drivers/watchdog/diag288_wdt.c)) was used as the development reference for Hercules Watchdog Timer. Hercules implementation is closer to a Z/VM implementation as various command actions can be specified if the timer expires. The difference is Z/Vm commands are specified by the guest OS when the Watchdog timer is opened, whereas Hercules expiry commands are specified by the 'hmcwdt' command prior to enabling the watchdog timer.

## States

The watchdog time is in one of three states:

- disabled             : Disabled is the default status. Diag 288 instructions will fail with a specification exception.
- enabled - inactive   : Watchdog timer is enabled but the quest OS has not started/opened the timer.
- enabled - active     : The quest OS has started/opened the watchdog timer and the timer is active (counting down). Guest OS pings the timer to reset the countdown time. If the countdown time expires, the specified hmcwdt commands will be executed.

## Command

The WatchDog Timer is configured using the following Hercules console command:   

>```plaintext
>hmcwdt [cmds "..."] | [cmdsep x | cmdsep off] | [disable | off] | [enable | on] | [status] | [debug [on | off]] | [$expire [nn]] | [$start [nn]] | [$stop]
>``````

### Options

**cmds "..."**: specify the Hercules configuration commands to be executed if the watchdog timer expires.

If spaces are included in the commands, double quotes are required.
The maximum size of the command line is 240 characters.

Multiple commands can be specified if a command line separator is defined using the 'hmcwdt cmdsep' command.
One additional command, 'pause xx', can be included for delay between commands. The pause value is seconds to delay.
'pause' is only valid when multiple commands are defined and is not the last command.
The commands can not include the "hmcwdt" command. 

Note: the command can not be null (hmcwdt cmds "") if the state is 'enabled - active'.

The default is null; no commands are defined. Therefore, two hmcwdt configuration statements
are required to enable the watchdog timer:

>```plaintext
>hmcwdt cmds "..."
>hmcwdt enable
>```

For example:

>```plaintext
>hmcwdt cmds "ssd"
>hmcwdt enable
>
>hmcwdt cmdsep ;
>hmcwdt cmds "ssd; pause 30; ipl 120"
>hmcwdt enable
>```

If pause is required, a script may be a better solution.

If the watchdog timer expires, the watchdog timer state remains 'enabled - active' with the timeout reset to the default, 30 seconds. 

**cmdsep x | cmdsep off**: specify the separator between commands defined by 'hmcwdt cmds'. Off resets the separator to none.

**disable | off**: the watchdog timer will be disabled if:

- the current state is 'enabled - inactive'.

Watchdogs timer state becomes 'disabled'.

**enable | on**: the watchdog timer will be enabled if:

- the current state is disabled, AND
- watchdog commands have been defined.

Watchdogs timer state becomes 'enabled - inactive'.

**status**: Display the current Watchdog Timer status. This is the default action.

>```plaintext
>HHC01959I HMC Watchdog Timer: status:
>        state:          enabled-inactive
>        timeout:        (none)
>        cmdsep:         (none)
>        cmds:           "* test message from watchdog timer"
>```

**$expire [nn]**: If the watchdog timer state is 'enabled - active', simulate a timer expiry by setting the current timeout to zero, wait 0.25 seconds (for the timer to expire) and then reset the timer with a 'nn' second timeout. 'nn' must be greater than 14. The default timeout is 30 seconds if 'nn' is not specified.

**$start [nn]**:  If the watchdog timer state is 'enabled-inactive', start the watchdog timer with a 'nn' second timeout. 'nn' must be greater than 14. The default timeout is 30 seconds if 'nn' is not specified.

**$stop**: If the watchdog timer state is 'enabled-active', stop the watch dog timer.

## Diag 0x288

The Guest OS uses diag 0x288 to initialize (start) a watchdog timer, change (reset) the timeout value, cancel (stop) the timer.

The 'diag r1,r3,0x288' instruction uses even-odd register pairs to define input parameters:

>```plaintext
>    r1  (even): unsigned int: function subcode 1, 2, or 3
>    r1+1 (odd): unsigned int: timeout in seconds (minimum: 15 seconds, maximum: 3600 seconds)
>
>    r3  (even): not used and should be 0. (Only used by VM diagnose 288)
>    r3+1 (odd): not used and should be 0. (Only used by VM diagnose 288)
>```

An exception is raised for:

- privileged operation exception
  - program check if running problem state

- specification exception
  - r1 or r3 are not even registers
  - watchdog timer is disabled
  - function subcode is not 1, 2, or 3

- operations exception
  - timeout is less than 15 seconds (no check is made for maximum)
  - already initialized (subcode 1 specified when timer is already active)
  - not initialized (subcode 2 or 3 specified when timer has not been initialized)

An exception is the only method used to communicate an error to the guest OS.  A condition code, or a return code
in a register, are not used to indicate an error state.

As r1 and r1+1 only require 32 bits (unsigned int), Diag 0x288 is available for all Hercules architectures.

### Subcode 1

Open (initialize) the countdown timer. The timer state must be 'enabled - inactive'.
>
>```plaintext
>  r1:     1
>  r1+1:   timeout in seconds (>= 15)
>```

Watchdogs timer state becomes 'enabled - active' with the activation of the hmcwdt_thread.

### Subcode 2

Reset (change ) the countdown timer to specified timeout. The timer state must be 'enabled - active'.

>```plaintext
>  r1:     2
>  r1+1:   timeout in seconds (>= 15)
>```

### Subcode 3

Close (stop) the countdown timer. The timer state must be 'enabled - active'.

>```plaintext
>  r1:     3
>  r1+1:   not used, should be 0.
>```

Watchdogs timer state becomes 'enabled - inactive' with the termination of the hmcwdt_thread.

### State Transitions

The watchdog timer transitions between three states from hmcwdt commands and the guest OS issuing Diag 288 instructions, as follows:

>```plaintext
>                 +---------- 'hmcwdt disable' ---------+
>                 |                                     |
>                 v                                     |
>(start)-> [ disabled ]  ---- 'hmcwdt enable' -----> [ enabled-inactive ]
>                                                       |      ^
>                                                       |      |
>+---------------------+      +----- 'subcode 1' -------+     |
>|                     |      |                               |
>|                     |      v                               |
>+-> 'subcode 2' --> [ enabled-active ] ---- 'subcode 3' -----+
>                        |        ^
>                        |        |
>        +---------------+        +--------------------+
>        |                                             |
>        +--- 'timer expires' ---> { execute cmds } ---+
>```

If the 'timer expires', { execute cmds } is a temporary execution state where Hercules commands from 'hmcwdt cmds "..."' are parsed and executed in an separate async thread. The timer expiry is reset to the default timeout (30 seconds).

## z/Linux Watchdog Timer Setup

Linux supports watchdog timers for many systems and devices. z/linux has a builtin watchdog driver, diag288_wdt, that uses
diagnose 0x288 to access either Z/VM software watchdog or the LPAR hardware watchdog. The following notes describe Watchdog Timer (Diag 288) testing
using z/Linux Ubuntu 26.04.

**1.**  As part of ZIPL hardware feature check, ZIPL will attempt to initialize the diag 0x288 watchdog with a 15 second timeout.
If the initialization succeeds (no specification exception), ZIPL will immediately close the timer and mark diagnose 288 as a hardware feature.
If the Hercules watchdog timer has been enabled, Hercules log entries will show hmcwdt_thread started and ended.

>```plaintext
>20:32:37 HHC00100I Thread id 0000e7ae9988c160, prio -1, name 'hmcwdt_thread' started
>...
>...
>20:32:38 HHC00101I Thread id 0000e7ae9988c160, prio -1, name 'hmcwdt_thread' ended
>```

**2.** z/Linux watchdog module/driver is named: diag288_wdt. For testing, the simplest way to start the driver is:

>```plaintext
>$ sudo modprobe diag288_wdt
>$ lsmod | grep diag
>
>diag288_wdt            12288  0
>```

Note: A timeout of 30 seconds is part of watchdog timer initialization. Without further z/linux configuration, the timer
will expire in 30 seconds and the 'hmcwdt cmds' commands will be processed. For testing use a comment!

**3.** After the watchdog timer is initialized and active, z/Linux needs a process to continually issuing timer reset/change commands
to avoid timer expiry. The easiest method is to update `/etc/systemd/system.conf` to include

>```plaintext
>#### note: ShutdownWatchdogSec is not in the man pages
>ShutdownWatchdogSec=0
>RuntimeWatchdogSec=30
>```

and force a systemd reload.

>```plaintext
>sudo systemctl daemon-reexec
>```

RuntimeWatchdogSec sets the watchdog timer to 30 seconds (minimum value is 15) and will reset/change the timeout before the timer expires.
ShutdownWatchdogSec is not defined in the systemd-system.conf Linux man page. This setting forces systemd to close the watchdog timer during 'non-standard'
shutdowns (eg. entering Hercules commands: ssd) to avoid log messages such as:

>```plaintext
>[  132.723550] watchdog: watchdog0: watchdog did not stop!
>```

**4.** Autoload of the watchdog driver, diag288_wdt, is a multi-step process as the driver is blacklisted in /usr/lib/modprobe.d/blacklist_linux_7.0.0-15-generic.conf. Why is the watchdog timer blacklisted? Without prior configuration for a process to continually reset the timer, the timer would expire within 30 seconds once the diag288_wdt driver is loaded, and on an LPAR, a system `restart`.

Because of the blacklist, create a diag288_wdt.service with the configuration file:

>```plaintext
>sudo nano /etc/systemd/system/diag288_wdt.service
>```

and add

>```plaintext
>
>[Unit]
>Description=Force-load diag288_wdt watchdog module
>After=multi-user.target
>
>[Service]
>Type=oneshot
>ExecStart=/sbin/modprobe diag288_wdt
>
>[Install]
>WantedBy=multi-user.target
>```

Then enable and start the service:

>```plaintext
>sudo systemctl enable diag288_wdt
>sudo systemctl start diag288_wdt
>```

The diag288_wdt driver will be loaded late in the z/Linux ipl process when the 'multi-user.target' is reached, just before the 'login:' prompt. For example,

>```plaintext
>[  OK  ] Finished snapd.seeded.service - Wait until snapd is fully seeded.
>[  OK  ] Reached target multi-user.target - Multi-User System.
>[  OK  ] Reached target graphical.target - Graphical Interface.
>         Starting diag288_wdt.service - For__load diag288_wdt watchdog module...
>HHC00100I Thread id 0000f5507885c160, prio 2, name 'hmcwdt_thread' started
>[  OK  ] Finished diag288_wdt.service - Force-load diag288_wdt watchdog module.
>
>
>
>Ubuntu 26.04 LTS ubuntu26-04 sclp_line0
>
>ubuntu26-04 login:
>```

**5.** A simulated timer expiry can be forced with the 'hmcwdt $expire' command on the Hercules console. The commands specified in 'hmcwdt cmds "...."' will be executed!
