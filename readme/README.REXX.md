![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Hercules Rexx Support

## Contents

1. [About](#About)
2. [Hercules Rexx Settings](#Hercules-Rexx-Settings)
3. [rexx Command](#rexx-Command)
4. [Running a Rexx Script](#Running-a-Rexx-Script)
5. [SHCMDOPT and DIAG8CMD](#SHCMDOPT-and-DIAG8CMD)
6. [Issuing Hercules Commands](#Issuing-Hercules-Commands)

## About

Hercules's Rexx support provides support for the following two Rexx packages:
 
* **[Open Object Rexx](http://www.oorexx.org/)**
* [Regina Rexx](http://regina-rexx.sourceforge.net/)  

Support for either package is not mutually exclusive of support for the other.

Support for **Open Object Rexx (ooRexx)** is automatically provided as long as the package's `rexx.h` and `oorexxapi.h` header files are found during build time.  If the `rexx.h` and `oorexxapi.h` headers are found, then integrated OORexx support will be provided.  If you wish to have ooRexx installed on your system but do not want Hercules to provide integrated support for it, you must explicitly request that such support be disabled via the `--disable-object-rexx` configure option.

Support for integrated **Regina Rexx** however, is _not_ automatically provided and must instead be explicitly requested via the `--enable-regina-rexx` configure option at build time.  Even if the package's `rexx.h` and `rexxsaa.h` headers are found, support for integrated Regina Rexx will still _not_ be provided.  Support is provided _only_ if the headers are found _and_ the `--enable-regina-rexx` option is specified.

Support for either package can be enabled or disabled (Linux only) by simply specifying the appropriate

* `--disable-object-rexx` and/or  
* `--enable-regina-rexx`  

configure option at build time. Note that enabling both packages requires them to be installed in different paths, for example ooRexx in /usr local and Regina Rexx in /usr. If both packages are installed in the same path, scripts transfering data from and to Rexx variables or stems may produce incorrect results or hang the whole Hercules system, particularly if they are executed in the background.
  
On Windows, the only way to purposely disable support is to rename the header file(s) to prevent Hercules from finding them.

## Hercules Rexx Settings

Hercules runtime support for Rexx is completely dynamic based the availability of each package's dynamic libraries which are automatically loaded at startup.

When both packages are found to be available then **OORexx** will be chosen as the default Rexx interpreter unless overridden by the `HREXX_PACKAGE` environment variable.  The only valid values for `HREXX_PACKAGE` are "none", "auto", "OORexx" or "Regina".  The default (preferred) Rexx package when "auto" is specified is **Object Rexx (OORexx)**.  Use "none" to prevent Rexx support for either package
from being automatically enabled at startup, thereby requiring you to manually enable (start) Rexx yourself via the Hercules `rexx` command's "start" option (see just below).

Other environment variables can be used to define your own defaults values for some of Hercules Rexx's runtime options.  The `HREXX_PATH` environment variable for example defines a default value for the `rexx` command's "rexxpath" option. Similarly the `HREXX_MODE` and `HREXX_EXTENSIONS` environment variables define default values for the `rexx` comamnd's "mode" and "extensions" options.


## rexx Command

`rexx  [option value] ...`  

Entering the `rexx` command without any arguments displays the current settings. Otherwise an option/value pair must be specified to set the specified option to the specified value.  More than one option/value pair can be specified on the same command:

```
    Ena[ble]/Sta[rt]    Enable/Start a Rexx Package, where package
                        is either 'OORexx' (the default) or 'Regina'.
                        Use the HREXX_PACKAGE environment variable
                        to define a preferred default value. "auto"
                        will automatically start the default package.
                        Use "none" to prevent automatic enablement.

    Disa[ble]/Sto[p]    Disable/Stop the Rexx package. Any attempt to
                        execute a Rexx script via the 'exec' command
                        will result in an error.

    RexxP[ath]/Path     List of directories to search for scripts.
                        No default. Use the HREXX_PATH environment
                        variable to define your preferred default.

    SysP[ath]           Extend the search to the System Paths too.
                        'On' (default) or 'Off'.

    Ext[ensions]        List of extensions to use when searching for
                        scripts. A search with no extension is always
                        done first. The HREXX_EXTENSIONS environment
                        can be used to set a different default list.

    Suf[fixes]          Alias for 'Ext[ensions]'.

    Resolv[er]          'On' (default): Hercules will resolve the
                        script's full path. 'Off': the scriptname
                        is used as-is.

    MsgL[evel]          'Off' (default) or 'On' to disable or enable
                        Hercules messages HHC17503I and HHC17504I
                        that display a script's return code and return
                        value when it finishes executing.

    MsgP[refix]         'Off' (default) or 'On' to disable or enable
                        prefixing Rexx script 'say' messages with
                        Hercules message number HHC17540I.

    ErrP[refix]         'Off' (default) or 'On' to disable or enable
                        prefixing Rexx script 'TRACE' messages with
                        Hercules message number HHC17541D.

    Mode                Define the preferred argument passing style.
                        'Com[mand]' (default) or 'Sub[routine]'. Use
                        the HREXX_MODE environment variable to define
                        your preferred default mode. See further below
                        for the difference between the two.

    List                Lists currently running asynchronous scripts.
                        See next section below.

    Cancel              <tid> to halt a running asynchronous script.
                        See next section below.

```

## Running a Rexx Script

The format of the `exec` command is:  
```
exec [mode] scriptname [[args...][&&]]
```

Where `scriptname` is the name of the Rexx script, `args` is an optional list of arguments to be passed to the script and `&&` as the last argument requests that the script be run asynchronously in the background.  The rexx command's `list` and `cancel` options can be used to list/cancel any currently running asynchronous scripts.

_**TAKE SPECIAL CARE**_ when using the `&&` option to run a script asynchronously!
Be careful to _**not**_ accidentally enter a single `&` instead, which invokes the Hercules `exec` command asynchronously, but _not_ the rexx script, leaving you with no way to cancel it!  Always use two ampersands `&&` to cause the script itself to run in the background.  Of course, if the script ends quickly then there is no need to run it asynchronously in the background.  The ability to run scripts in the background is designed for never-ending 'monitoring' type scripts that monitor and report such things as Hercules status.

The `mode` setting determines how arguments are passed to your Rexx script. In `command` mode (the default) there is only one argument passed, with that single argument being the string of characters which immediately follows the script's name.  This allows your script to parse the string into individual arguments however it may decide, potentially contrary to the way command line arguments are normally parsed.

In `subroutine` mode, Hercules parses the string normally and passes each argument individually as shown in the examples just below.

The argument passing style is determined by the `rexx` command's current `Mode` setting, but can be temporarily overridden for the current execution by simply specifying the `mode` parameter on the command itself, immediately before the scriptname (e.g. `exec cmd ...` for command style argument passing, or `exec sub ...` for subroutine style argument passing).

Contents of script `example.rexx`:

```C
        /* REXX */
        parse arg str
        say "parse arg str: " str
        say "arg(1): "arg(1)
        say "arg(2): "arg(2)
        say "arg(3): "arg(3)
        exit
```

Running the script from a **command line** (outside of Hercules) results in:

<pre>
        <b>C:\> example.rexx  one,   Two   "Buckle    MY shoe"</b>
        parse arg str:  one,   Two   "Buckle    MY shoe"
        arg(1): one,   Two   "Buckle    MY shoe"
        arg(2):
        arg(3):
</pre>

Running the script from within Hercules via the `exec` command using the default _**`command`**_ mode setting results in:

<pre>
        <b>HHC01603I exec cmd example.rexx  one,   Two   "Buckle    MY shoe"</b>
        parse arg str:  one,   Two   "Buckle    MY shoe"
        arg(1): one,   Two   "Buckle    MY shoe"
        arg(2):
        arg(3):
</pre>

Running the script using _**`subroutine`**_ mode results in:

<pre>
        <b>HHC01603I exec sub example.rexx  one,   Two   "Buckle    MY shoe"</b>
        parse arg str:  one,
        arg(1): one,
        arg(2): Two
        arg(3): Buckle    MY shoe
</pre>

## SHCMDOPT and DIAG8CMD

The Hercules Rexx `exec` command is considered to be a "shell" command from Hercules's point of view since both of the supported Rexx interpreters provide the ability to directly target the host operating system environment.  Both of the `sh` and `exec` commands are thus disabled by default for security reasons.

To enable the ability to `exec` Rexx scripts from the Hercules command line (or via the Hercules DIAG 8 instruction interface) use the `shcmdopt` and/or `diag8cmd` commands.  For more information on each please refer to Hercules documentation describing configuration file statements.


## Issuing Hercules Commands

Rexx scripts run from within Hercules (via the `exec` command) are able to issue Hercules commands via the Rexx "Address" keyword or via the Hercules "AWSCMD" special function:

```REXX
    Address "HERCULES" "command..."
    rc = AWSCMD( "command..." [, stemvar [, errmode]] )
    Call AWSCMD  "command..." [, stemvar [, errmode]]
```

The Rexx variable "RC" contains the return code from the Hercules command.  The specified stem variable "stemvar" will contain the response from Hercules with the usual convention of "stemvar.0" being set to the number of response lines and "stemvar.1" to "stemvar.n" holding the Hercules response lines themselves.

A sample script called `hcommand.rexx` illustrating both techniques ("Address" and "AWSCMD") can be found in the "scripts" subdirectory of the source code distribution.

Note that when a response stemname is used, Hercules does _not_ display the results of the command on the hardware console panel.  Instead, the results are captured and returned in the specified Rexx stem variable, and it becomes your decision what to do with them (such as displaying them on the hardware console panel via the Rexx "Say" command).

Since the Rexx "Address" keyword syntax does not provide any means of specifying additional parameters (such as the stem variable name and error handling option that the AWSCMD technique provides), options for the "Address" keyword syntax are passed to the Hercules Rexx subcommand environment via several predefined reserved Rexx variables. The predefined reserved Rexx variables:


```
HREXX.RESPSTEMNAME
HREXX.PERSISTENTRESPSTEMNAME
```

define the stem variable names to be used to hold the Hercules response lines.
`HREXX.RESPSTEMNAME` is dropped after every call so each "Address 'HERCULES'" invocation finds an unbiased environment.  `HREXX.PERSISTENTRESPSTEMNAME` provides the same functionality but is never dropped.


```
HREXX.ERRORHANDER
```

defines how errors (non-zero RC) should be handled.  Setting the variable to "SYSTEM" requests the Rexx interpreter itself handle any non-zero return code in the standard Rexx fashion.

Setting it to the value "RETCODE" (the default if not specified) delegates all error handling to the caller, allowing your script to react to the "error" in whatever way it deems is appropriate.

The ability to specify error handling is provided since some Hercules commands might return a non-zero return code (such as the `devlist` command when there are no devices defined in the configuration) and from the subcommand interface's point of view such non-zero return codes should not be considered a subcommand error.
