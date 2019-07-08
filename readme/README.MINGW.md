![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# NOTE!  This file is OBSOLETE and should probably be deleted!
### Not converted to markdown on that basis.

Once we got Herc to build successfully using MSVC, I/we stopped all effort on trying to make sure it continued to build under MinGW. At this late stage I doubt it will build at all under MinGW! That is to say, it more than likely will NOT build under MinGW, and thus the reason why this README should be deleted. The only reason I haven't deleted it yet is simply because of its historical value since it might be beneficial to someone should they decide to try on their own to get Herc to build under MinGW.  
  
-- Fish

## Original readme now follows:

```
******************************************************************
******************************************************************

                 -------------------------
                 > > >    N O T E    < < <
                 -------------------------

  While the below information is probably reasonably accurate,
  it should be noted that building Herc with MinGW may no longer
  work. (It hasn't been tested in a long time).

  The prefered method (and indeed the only SUPPORTED method) for
  building the Win32 version of Hercules is via Microsoft's free
  compiler toolkit as explained in the README.MSVC document. The
  below information is mostly for historical purposes.

******************************************************************
******************************************************************

                 MINGW PORTING NOTES:

./configure --enable-fthreads --disable-shared --enable-static \
     --disable-nls --disable-dynamic-load --disable-external-gui

./configure --enable-fthreads --disable-shared --enable-static \
     --disable-nls --disable-dynamic-load --disable-external-gui \
     --enable-debug --enable-optimization="-O0 -g"

******************************************************************
******************************************************************

================================================================
================================================================

               INSTALLING  MSYS  +  MINGW

================================================================
================================================================

--------------------------------------------------------------
http://www.mingw.org/download.shtml

1. Install MSYS-1.0.10.exe      -->  d:/msys/1.0
2. Install msysDTK-1.0.1.exe    -->  d:/msys/1.0
3. Install MinGW-3.1.0-1.exe    -->  d:/MinGW

In that order!


--------------------------------------------------------------
Create /etc/fstab to point to the mingw dir:

    #Win32_Path                 Mount_Point
    d:/mingw                    /mingw


--------------------------------------------------------------
Create .inputrc in home dir with desired terminal settings.

    #--------------------------------------------------------------------------------
    # Fish: The following from: http://www.ibb.net/~anne/keyboard/keyboard.html#Bash
    #--------------------------------------------------------------------------------

    "\e[3~": delete-char
    # this is actually equivalent to "\C-?": delete-char
    # VT
    "\e[1~": beginning-of-line
    "\e[4~": end-of-line
    # kvt
    "\e[H":beginning-of-line
    "\e[F":end-of-line
    # rxvt and konsole (i.e. the KDE-app...)
    "\e[7~":beginning-of-line
    "\e[8~":end-of-line

    # --- (eof) ---


--------------------------------------------------------------
Update /etc/profile with desired aliases

    # ------- Fish's preferences... -------

      export HISTCONTROL=ignoredups
      alias less='less -r'
    # alias rm='rm -i'
      alias whence='type -a'
      alias ls='ls -F --color=tty'
      alias dir='ls --color=auto --format=vertical'
      alias vdir='ls --color=auto --format=long'
      alias ll='ls -l'
      alias la='ls -A'
      alias l='ls -CF'
      alias cls='clear'

    # --- (eof) ---


--------------------------------------------------------------
(((Optional: you only need to do the following (if desired) if
you plan on using the provided msys shell window (NOT recommended)
instead of your own MSYS Command Prompt window (HIGHLY recommended)
which I explain how to do in the next step just below))))...


Make a copy of the 'msys.bat' file in "d:\msys\1.0" and update it
with your prefered colors and window geometry settings:


rem (deleted by Fish)                      if "x%MINGW32BGCOLOR%" == "x" set MINGW32BGCOLOR=LightYellow
rem (deleted by Fish)                      if "x%MINGW32FGCOLOR%" == "x" set MINGW32FGCOLOR=Navy
rem (replaced with the following instead)
                                           if "x%MINGW32BGCOLOR%" == "x" set MINGW32BGCOLOR=Black
                                           if "x%MINGW32FGCOLOR%" == "x" set MINGW32FGCOLOR=#C0C0C0


rem (deleted by Fish)                      start rxvt -backspacekey . -sl 2500 -fg %FGCOLOR% -bg %BGCOLOR% -sr -fn Courier-12 -tn msys -geometry 80x25  -e /bin/sh --login -i
rem (replaced with the following instead)
                                           start rxvt -backspacekey . -sl 2500 -fg %FGCOLOR% -bg %BGCOLOR% -sr -fn Courier-12 -tn msys -geometry 100x50 -e /bin/sh --login -i


              *** IMPORTANT NOTE! ***

Be VERY CAREFUL when modifiying the above "start rxvt..." line!
The actual .BAT file contains an embedded backspace character
which does not appear on the above example statements! Do NOT
use the above example statements as-is! Instead, use notepad
to manually edit the .bat file itself (being very careful
when you do!).

ALSO, it's important to MAKE A COPY of the msys.bat file and make
your changes to it. This is because the MSYS.BAT file is REPLACED
whenever msys/mingw is updated! Thus, if you fail to make your own
copy of it, your changes will be lost!!


--------------------------------------------------------------
                 MINGW Command Prompt

As an alternative to the crappy 'GNU' shell window used by default
by MSYS (rxvt?), it's easy enough to setup a native WinNT Command
Prompt window to be your "shell window" instead.

First, create a .bat (batch) file in the 'd:\msys\1.0' directory
called, for example, "fish_msys.bat" (try NOT to use a name with
spaces in it!), with the following statements in it:


    @echo off

    SET CYGWIN="tty binmode title strip_title" codepage:oem

    D:
    chdir \msys\1.0\bin

    bash --login -i


Yep! That's right. We're setting a 'CYGWIN' environment variable
with the above batch file. The msys/mingw package was ported FROM
Cygwin after all, so, interestingly enough, the above does indeed
do what one expects it to: displays the name of the program being
executed in the window title ('title'), but with the path removed
('strip_title'), just like Cygwin's shell window does.

Next, make a copy of Windows's "Command Prompt" shortcut and modify
it as desired (width, height, etc -- i.e. the window's 'layout').

To turn it into a MINGW Command Prompt window (instead of the native
WinNT Command Prompt window it starts out as), simply modify the
shortcut's "Properties" as follows:

    Target:     D:\msys\1.0\fish_msys.bat
    Start in:   D:\msys\1.0\bin
    Comment:    MINGW Command Prompt

Optional:

    Click the "Change Icon..." button, followed by the "Browse..."
    button, and navigate to the root MSYS directory ("D:\msys\1.0")
    and select the desired icon ("m.ico" or "msys.ico").

DONE!!

Now whenever you click on that shortcut, Windows will open a normal
Command Prompt window with the MINGW (MSYS) shell already started!

Your working directory will automatically be you 'home' directory,
and you'll be able to enter whatever shell commands and/or execute
any shell scripts you need, and the name of the program that's being
executed (without the path) will be displayed in the window's title!

COOL!!  :))


(and MUCH better than the crappy window msys/mingw provides IMO)


--------------------------------------------------------------
(Optional): Create a symbolic link for bash.

"sh.exe" is actually bash (at least according to the msys docu-
mentation), so symlink 'bash.exe' to 'sh.exe':

   ln  -s  sh.exe  bash.exe

so all of your scripts that start with #!/bin/bash will work.


--------------------------------------------------------------
The handling of quotes on command-lines is different in the
msys shell than it is with cygwin. Thus whereas:

   ./configure --enable-custom="value with spaces"

used to work fine with cygwin, it no longer works with the
msys shell. Instead, you need to use MICROSOFT command-line
escape rules, and enter the command as follows instead:

   ./configure --enable-custom=\"value with spaces\"


Note: as far as I know this only applies to manually entering
commands in the msys shell window (i.e. at the command prompt).
I don't believe it applies (i.e. I believe it does NOT apply)
to scripts that happen to issue shell commands for example.


--------------------------------------------------------------
(Optional): Create the '/usr/local/bin' directory so you can
use all of your previously created scripts and private override
programs, etc.

NOTE: the        "d:/msys/1.0"  directory
is actually the  "/usr"         directory[*].

Thus, to add our own  "/usr/local/bin"         directory,
we need to create a   "d:/msys/1.0/local/bin"  directory.

-----------
 *  As well as the '/' root directory too, interestingly/confusingly
    enough! Refer to the "Automatic file system maps" table in the
    "README.rtf" document in the "D:\msys\1.0\doc\msys" directory
    for details regarding hard-coded mount points.


--------------------------------------------------------------
Do a hercules 'bldlvlck' to make sure you have everything that
autoconf needs...

Fish@SYSTEM2 ~
$ cd hercules

Fish@SYSTEM2 ~/hercules
$ cd util

Fish@SYSTEM2 ~/hercules/util
$ bldlvlck

This utility will check the level of various utilities needed to build
hercules. Checking is done against versions that are KNOWN to work.
This doesn't mean a build will NOT succeed with older versions
of the utilities, but will give a hint as to what package may need
an upgrade if the build ever fails with some odd reason.


OK      SVN (informational), found x.yy

OK      autoconf requires 2.5, found 2.56

OK      automake requires 1.6, found 1.7.1

INSTALL flex not found
        URL: http://www.gnu.org/directory/flex.html

OK      gawk requires 3.0, found 3.0.4

OK      gcc requires 2.95, found 3.2.3

INSTALL gettext not found
        URL: http://www.gnu.org/directory/gettext.html

OK      grep requires 0, found 2.5.1

INSTALL iconv not found
        URL: http://www.gnu.org/directory/libiconv.html
        For first-time Libiconv installs, a recompile and reinstall of gettext
        is recommended.  Source: libiconv-1.8/README

OK      m4 requires 0.0, found 1.4

OK      make requires 3.79, found 3.79.1

OK      perl requires 5.6, found 5.6.1

OK      sed requires 3.02, found 3.02


--------------------------------------------------------------
"http://sourceforge.net/projects/gnuwin32/" lists MANY GnuWin32
packages. Use the "Download" link in the FAR RIGHT COLUMN to
download the following needed packages:

    +-------------------------------------------------------
    |
    |      ***   FISH ADDENDUM -- Feb 2005   ****
    |
    |  The below "instructions" are WRONG(?) (I think)
    |  as far as the copying/installation is concerned.
    |
    |  I now believe the PROPER way to "install" an
    |  external package meant for MSYS is to simply un-
    |  zip it (extract it) directly into the main msys
    |  directory, letting WinZip (or whatever zip util
    |  you happen to be using), allowing it to automatically
    |  overlay whatever files it needs to overlay/replace
    |  and allowing it to automatically create whatever
    |  directories it needs to create (as needed according
    |  to whatever is contained in the zip file).
    |
    |  I haven't yet tried it myself yet, but I believe
    |  that's all you need to do: unzip into msys and
    |  that's it. (But as I said, I haven't confirmed
    |  that yet).
    |
    |  So in the below instructions, try:
    |
    |     replace:  "copy to /usr/local/bin"
    |     with:     "unzip into msys"
    |
    |  instead.
    |
    +-------------------------------------------------------


    gettext-0.14.1-bin.zip

        contains "gettext.exe" (and many other exes/dlls)
        in the 'bin' directory

        copy "gettext.exe" (and many other exes/dlls)
        to /usr/local/bin.


(Recall that according to msys documentation the "d:/msys/1.0"
 directory is actually your '/usr' directory)


    gettext-0.14.1-dep.zip

        contains "libiconv2.dll"
        in the 'bin' directory
        needed by gettext.exe

        copy "libiconv2.dll" to /usr/local/bin


    libiconv-1.8-1-bin.zip

        contains "iconv.exe" and most (but not all!) support dlls
        in the 'bin' directory

        copy "iconv.exe" and most (but not all!) support dlls
        to /usr/local/bin


    libintl-0.11.5-2-bin.zip

        contains "libintl-2.dll"
        in the 'bin' directory
        needed by iconv.exe

        copy "libintl-2.dll" to /usr/local/bin.


     ---------------------------------------
      Do we need the below?? I'm thinking
      that maybe we might for linking Herc
     ---------------------------------------
    libiconv-1.8-1-lib.zip

        contains ".a" and ".lib" files
        in the 'lib' directory
        used to link hercules with

        copy ".a" and ".lib" files to ???????

        contains *.h files
        in the 'include' directory
        used to compile hercules with

        copy *.h files to ???????


================================================================
================================================================

                HERCULES MODIFICATIONS

================================================================
================================================================

       ---------------------------------------
       >>> NLS disabled for the time being <<<
       ---------------------------------------

I keep getting errors when the intl directory is built:

  plural.y: In function `__gettextlex':
  plural.y:263: error: argument "pexp" doesn't match prototype
  plural.y:69: error: prototype declaration
  plural.y: In function `__gettexterror':
  plural.y:407: error: argument "str" doesn't match prototype
  plural.y:70: error: prototype declaration
  plural.y: At top level:
  plural.y:406: warning: unused parameter 'str'


For the time being I'm skipping past it via "--disable-nls"
but it needs to be looked into.


--------------------------------------------------------------
No biggee, but '-traditional-cpp', while originally (and for now)
added (by Jay?) specifically for Apple builds, doesn't work for
non-Apple builds. (I tried it.)

It keeps issuing warnings regarding extra tokens on #endif and
#include statements.

I added '-Wno-endif-labels" to get rid of the warnings for #endifs,
but can't find an option to suppress the warnings for #includes!

But as I said, it's not a big problem right now since, as it's
coded right now, we're only adding '-traditional-cpp' for Apple
and ONLY for Apple builds.

Note too, that we COULD get rid of all such warnings by simply
coding our comments (on #endif and #include statements) using
/* */ instead of //, but I'm too lazy to change them all right
now; maybe later if I'm in the mood...


--------------------------------------------------------------
Created 'hthreads.h', 'hostopts.h', w32util.h/c'.

'hostopts.h' now contains the host specific options that
used to be in featall.h. (I want to try and place all host-
specific tests in one convenient place, replacing all #ifdef
WIN32 and #ifdef __APPLE__, etc tests with #ifdef FEATURE_XXX.
Doing that is more portable and makes porting efforts easier.


--------------------------------------------------------------
I was hoping to avoid having to do it, but in the end I had
no other choice; trying to keep it was simply causing too much
trouble and making the code too sloppy trying to finagle things
to try and keep it.

Soooooo.....

I had to rename the Hercules 'DWORD' type to 'DBLWRD' so as to
not conflict with Microsoft's existing DWORD typedef. <grumble>

I kept the HWORD and FWORD typedefs as-is.


--------------------------------------------------------------
sie.c:  added  "#if !defined(NO_SIGABEND_HANDLER)":

DEF_INST(start_interpretive_execution)

...

    logmsg (_("HHCCP079E CPU%4.4X: calloc failed...
        ...
#if !defined(NO_SIGABEND_HANDLER)
    signal_thread(sysblk.cputid[regs->cpuad], SIGUSR1);
#endif

...


--------------------------------------------------------------
ieee-w32.h:

Changed all instances of:    u_int32_t
                      to:    uint32_t

since that seems to be the "Hercules standard" according to
what I can see in the 'htypes.h' header.

(Same with 'u_int64_t' too: changed them all to 'uint64_t').


--------------------------------------------------------------
cckddasd, cckdutil, etc...

Created:

'get_file_accmode_flags' --> "Poor man's" fcntl( fd, F_GETFL )

(only returns access mode flags not any others...)


--------------------------------------------------------------
Created: 'hconsts.h', 'hmacros.h', 'hstructs.h', 'hexterns.h'

to make  "hercules.h"  smaller/simpler and to basically make
things a bit easier to maintain... (helps to keep things more
organized IMO)


--------------------------------------------------------------
commadpt.c:

  rc = fcntl( tempfd, F_GETFL );
  rc |= O_NONBLOCK;
  fcntl( tempfd, F_SETFL, rc );

Changed to:

  socket_set_blocking_mode(tempfd,0);

Also created:

   'socket_init()'  and  'socket_deinit()'

functions since Windows Socket 2 requires calling 'WSAStartup'
and 'WSACleanup'. (calls are in 'impl' function in impl.c)


--------------------------------------------------------------
Started trying to remove all #ifdef WIN32, etc, and replace
with #ifdef XXXXX (where 'XXXX' is some feature name), (see
next item for good example), but haven't finished yet...


--------------------------------------------------------------
hercifc.c:
ctc_ctci.c:
ctc_lcs.c:
tuntap.c:

Changed #ifndef __APPLE__ with #ifdef OPTION_TUNTAP_SETNETMASK,
etc, and #defined or #undef'ed OPTION_TUNTAP_SETNETMASK, etc
as needed in new 'hostopts.h' member.


--------------------------------------------------------------
feature.h:

I see:

  #if defined(WIN32) && !defined(__WORDSIZE)
    #define __WORDSIZE 32
  #endif

but __WORDSIZE doesn't seem to be used at all (there are a
few statements but they're commented out).

Can this be safely removed?? (it probably cam, but I'm thinking
it might be something in preparation for 64-bit builds, which
is why I'm asking)


--------------------------------------------------------------
vm.c:

void ARCH_DEP(extid_call) (int r1, int r2, REGS *regs)
...
  /* Bytes 16-23 contain the userid in EBCDIC */
#if defined( HAVE_GETLOGIN_R )
  memset( unam, 0, sizeof(unam) );
  VERIFY( getlogin_r ( unam, sizeof(unam) ) == 0 );
  puser = unam;
#else
  puser = "";
#endif
  for (i = 0; i < 8; i++)
  {
    c = (*puser == '\0' ? SPACE : *(puser++));
    buf[16+i] = host_to_guest(toupper(c));
  }


--------------------------------------------------------------
You will see a LOT of "comparison between signed and unsigned"
warnings during the build (esp. in 'commadpt.c' for example)
due to Microsoft's 'FD_SET' macro implementation. There's no
simple way to work around it so just ignore it for now...


--------------------------------------------------------------
tapedev.c:

Had to make extensive modifications to 'mountnewtape' function
due to non-existence of regular expression library in MinGW.
Luckily the patterns we're checking for are rather simple, so
it wasn't that difficult to do. My question is WHY we're doing
it via regular expressions in the first place??


--------------------------------------------------------------
ctcadpt.c:

Support for VMNET is gen'ed, but will not work until Win32
equivalent for 'fork' is developed. 'fork' simply returns -1
at the moment. ('fork' might take a while to port!)


--------------------------------------------------------------
hdl.h:

minor fix: check if LTDL_SHLIB_EXT defined too when #defining
HDL_MODULE_SUFFIX.


--------------------------------------------------------------
Various...       (many different places...)

Unfortunately, MinGW always links with Micro$oft's MSVCRT
runtime library. Thus, all scanf/printf format flags must use
the Micro$oft format, which, for 64-bit values, is completely
different from the GNU format.

GNU uses                "%lld",  "%llx",  %16.16llX,  etc.
whereas Micro$oft uses  "%I64d", "%I64x", %16.16I64X, etc.

Thus I had to #define the following (in 'hconsts.h'):

#ifdef WIN32
#  define  LL_FMT  "I64"
#else
#  define  LL_FMT  "ll"
#endif

and change all occurrences of:   "%lld"
with:                            "%"LL_FMT"d"

which is so UGLY beyond belief it's pitiful. :(


--------------------------------------------------------------
(Various):  workaround inability to redirect 'stdout'...

The logger thread in logger.c redirects the write end of the
logmsg pipe into STDOUT via dup2, but unfortunately, the way
I'm currently handling the logmsg pipes for this MinGW port
(wherein the pipe is actually just a socketpair connected to
each other), that technique no longer works (since in Win32
you can't do 'dup2' on sockets) and thus I have it commented
out (since we actually don't really need to do the dup2 at all
as long as everyone uses the 'logmsg' macro for their i/o).

Unfortunately however, the "display_version" function (and
indirectly the "display_hostinfo" function which display_version
calls) are coded to "fprintf" to whatever FILE* is passed
(which is usually stderr for the utilities and stdout for
Hercules (impl.c)).

Thus, in those two places -- and only in those two places --
I had to code a duplicate 'print' call: one that fprintf'd
to stderr, and the other to do a 'logmsg'. When the utilities
call it (display_version), they pass stderr and I do fprintf
to stderr, but when if stdout is passed (as it is when impl.c
calls it), then I do a logmsg.

I haven't thought whether there's an easier/better way to do
things; I just want to get this damn port FINISHED for now!
I can go back and clean things up later. Okay?


***********************************************************************
***********************************************************************
***********************************************************************

 ** TODO **   ((  U N R E S O L V E D    I S S U E S  ))


                (moved to README.MSVC)


***********************************************************************
***********************************************************************
***********************************************************************

From the MinGW 'FAQ' @ <http://www.mingw.org/mingwfaq.shtml#faq-msvcdll>

...


How can an MSVC program call a MinGW DLL, and vice versa?


---------

Assume we have a testdll.h, testdll.c, and testmain.c. In the first case, we will compile
testdll.c with MinGW, and let the MSVC-compiled testmain call it. You should use


    gcc -shared -o testdll.dll testdll.c \
        -Wl,--output-def,testdll.def,--out-implib,libtestdll.a


to produce the DLL and DEF files. MSVC cannot use the MinGW library, but since you already have
the DEF file you may easily produce one by the Microsoft LIB tool:


    lib /machine:i386 /def:testdll.def


Once you have testdll.lib, it is trivial to produce the executable with MSVC:


    cl testmain.c testdll.lib



---------

Now for MinGW programs calling an MSVC DLL. We have two methods. One way is to specify
the LIB files directly on the command line after the main program. For example, after

    cl /LD testdll.c

use

    gcc -o testmain testmain.c testdll.lib


<doesn't work; see below>

The other way is to produce the .a files for GCC. For __cdecl functions (in most cases),
it is simple: you only need to apply the 'reimp' tool from Anders Norlander (since his
web site is no longer available, you may choose to download here a version enhanced by
Jose Fonseca):

    reimp testdll.lib
    gcc -o testmain testmain.c -L. -ltestdll

</doesn't work; see below>


However, for __stdcall functions, the above method does not work. For MSVC will prefix
an underscore to __stdcall functions while MinGW will not. The right way is to produce
the DEF file using the 'pexports' tool included in the mingw-utils package and filter
off the first underscore by sed:


    pexports testdll.dll | sed "s/^_//" > testdll.def


Then, when using dlltool to produce the import library, add `-U' to the command line:

    dlltool -U -d testdll.def -l libtestdll.a


And now, you can proceed in the usual way:

    gcc -o testmain testmain.c -L. -ltestdll


Hooray, we got it.


***********************************************************************
***********************************************************************
***********************************************************************
```
