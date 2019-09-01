![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# RXVT4APL Mods

## Contents

1. [About](#About)
2. [Notes](#Notes)

## About

Max H. Parke [ikj1234i@yahoo.com](mailto:ikj1234i@yahoo.com) has modified the commadpt driver so APL works properly with `rxvt4apl`.

`rxvt4apl` is an 8-bit clean, colour xterm replacement that has been optimised for use with openAPL.
Here is Max's README regarding his rxvt4apl mods:

## Notes

1. The rxvt4apl package provides a set of extensions to enable both entry and display of the complete APL character set including the characters without ASCII equivalents.  Although we use the standard version of `rxvt4apl` [2.4.5] without modifications, there is a small bugfix patch (below) if necessary.

2. Once rxvt4apl has been installed and tested (there are several required materials in openAPL such as X11 fonts and keymaps), you should connect using telnet to the listening port for the terminal in Hercules:


    telnet localhost 57413


then type the standard APL command to sign on:

    )1234

There is extensive helpful documentation with openAPL if you have problems with X11 fonts or keymaps, etc.

3. Rather than use the right ALT key, I used the unused "Windows" key for the Mode_switch key in `modeswitch.xmap`:
```
    clear Mod5
    clear Mod4
    clear Mod3
    keycode 133 = Mode_switch
    add Mod3 = Mode_switch
```

4. The terminals must be defined to Hercules using the following device statement:
  
  
    0402  2703  dial=in  lport=57413  lnctl=ibm1  term=rxvt4apl  skip=5EDE  code=ebcd  iskip=0D0A  prepend=16  append=5B1F  eol=0A  binary=yes  crlf=yes  sendcr=yes


5. In general, when typing a character, `rxvt4apl` seems to prefer to use the standard ASCII character if it exists, in preference to the APL character.  So for example if you wish to enter the apostrophe ('), use your ASCII keyboard's standard apostrophe key, not APL's apostrophe (ordinarily `shift-K`).

6. In this implementation the standard alphabet APL characters A-Z must be typed as _lowercase_ ASCII characters to be properly translated on input.  Also, `rxvt4apl` is missing the underlined alphabet characters.

7. Overstrike handling is supplied in this driver release, although they can be formed by entering the individual characters separated by the backspace key.  The display will not be correct but the overstrike sequence will be properly received by APL\360.<br>
<br>Better is to enter the overstrike characters using the `rxvt4apl` composite characters; when this is done the driver will convert these extended-ASCII characters into the proper overstrike sequences used by APL\360 on input (the extended characters are converted into three-character sequences in 2741 code, with the middle character in the sequence being the backspace character).  On output, the driver replaces the third character in the overstrike sequence with the correct `rxvt4apl` extended-ASCII character.

8. Many quirks exist - a couple of examples:

 - The right arrow key (branch command) must be hit twice.
 - Some commands such as up and/or down grade cause unexpected escape to telnet command mode, even in binary mode.

9. If you receive this error when running rxvt4apl:


    can't open pseudo-tty


try the following patch (required for recent Ubuntu):

```diff
--- rxvt4apl_2.4.5/src/command.c
+++ rxvt4apl_2.4.5-patched/src/command.c
@@ -448,7 +448,7 @@
     ptydev = ttydev = _getpty(&fd, O_RDWR | O_NDELAY, 0622, 0);
     if (ptydev == NULL)
        goto Failed;
-#elif defined (__svr4__)
+#elif 1 // defined (__svr4__)
     {
        extern char    *ptsname();

```
