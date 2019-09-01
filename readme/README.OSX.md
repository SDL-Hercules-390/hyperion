![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Building Hercules for OSX
## Contents
1. [About](#About)
2. [Tun Tap Information](#Tun-Tap-Information)

## About
To compile on OS X for another architecture than the one running, or another OS version, you have to supply several arguments to the configure command.

To force a particular architecture, you need to specify the machine architecture to the gcc command with the `-arch <architecture>` operand. You also need to tell configure what environment you're building for, with `--host`.

```
ARCHITECTURE    -arch   --host
32-bit Intel    i386    i686
64-bit Intel    x86_64  x86_64
32-bit PowerPC  ppc     powerpc
64-bit PowerPC  ppc64   powerpc64
```

The argument for `--host` is the first part of a build triplet, specified as <architecture>-<vendor>-<OS>. The vendor is apple for all OS X builds. OS is "darwin8.8.0" for OS X 10.4 (Tiger), and "darwin9.6.0" for OS X 10.5 (Leopard). This makes, for example, building for 64-bit Intel on Leopard use `--host x86_64-apple-darwin9.6.0`. The -arch argument is specified by overriding the CC (C compiler) value and adding it to the end, as in `CC="gcc-arch x86_64"`.

If you're building for an OS that's not the one you're running on, you also need to tell the build environment that you're doing so. For building for Tiger on Leopard, you need to add two arguments to the configure invocation: `CFLAGS='-isysroot/Developer/SDKs/MacOSX10.4u.sdk' LDFLAGS='-isysroot/Developer/SDKs/MacOSX10.4u.sdk -Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk'`
(all on one line, of course). You also need to add an environment variable that's passed to gcc upon invocation, and this takes adding it to the beginning of the CC value, as in `CC="/usr/bin/env MACOSX_DEPLOYMENT_TARGET=10.4 gcc -arch ix86_64`.

This makes a complete invocation for building for 32-bit Intel for Tiger on Leopard (again, all on one line):  
```
CC="/usr/bin/env MACOSX_DEPLOYMENT_TARGET=10.4 gcc -arch i386" CFLAGS='-isysroot /Developer/SDKs/MacOSX10.4u.sdk' LDFLAGS='-isysroot /Developer/SDKs/MacOSX10.4u.sdk -Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk' ./configure --enable-setuid-hercifc --host=i686-apple-darwin8.8.0
```

Building 32-bit Intel for Leopard on Leopard is easier:  

`CC="gcc -arch i386" ./configure --enable-setuid-hercifc --host=i686-apple-darwin9.6.0`

(Note that building a 64-bit Intel version on a Mac with a 64-bit Intel processor still requires explicitly setting the architecture. If you don't, you'll get a 32-bit Intel version.)

Once you have the various architectures built, you can combine them into one binary with lipo. This is done by saying `lipo <input-files> -output <output-file> -create`. The best approach is to automate this with a shell script; I've done this for my own use.

## Tun Tap Information
From: http://tech.groups.yahoo.com/group/hercules-390/message/61064  

First, the CTC definition should be set up as follows:  

`0400.2 3088 CTCI /dev/tun0 1500 192.168.0.12 192.168.0.103 255.255.255.255`  

Unlike Linux, you don't use two consecutive IP addresses on the CTCI definition. You use the _host_ IP address (the address that Hercules will use) followed by the IP address of the computer that will be hosting Hercules. Note the subnet mask. The whole IP address is the network ID.

Next, IP forwarding needs to be enabled. I wrote a shell script with the command to do the job and have it run when I boot the Mac. The command to enable IP forwarding on the Mac is:  

`sudo sysctl -w net.inet.IP.forwarding=1`  

`sudo` switches you to root; sysctl with the `-w` switch means to "write the value" or "set the value" of the variable `net.inet.IP.forwarding` to 1. This is similar to Linux, except there you use an echo command to set a variable that applies to setting an IP forwarding to 1.

Before doing the above, though, you must assign the root user a password as follows:  

`sudo passwd root`  

passwd is a variable and can be substituted with any password you like. You will need to assign a password to root because the previous command will prompt you for a password for root access to change the variable net.inet.IP.forwarding from the default "0" to "1".

Proxy Arp is already set `ON` in a Mac. This is what really confused me. I kept reading _everywhere_ in Google that it is set `ON` by default. Then I stumbled on the magic command that ties your Hercules host IP address to your ethernet interface so that computers behind the router can get to it and so that Hercules can get out to the rest of the world (the internet). It is the ARP command. It needs be be coded like this:  

`arp -s 192.168.0.12 00:16:cb:aa:d4:4c pub en0`  

The above command "ties" the host IP address of your Hercules machine(in this case 192.168.0.12) to the interface `en0` (which happens to be my ethernet card) allowing access to and from the Hercules machine from behind the router and out to the internet.

The `-s` switch tells the arp command you will be specifying a host and a ether address (the MAC address of your ethernet card, e.g. `00:16:cb:aa:d4:4c` in my case).

If the word `pub` is given, the entry will be "published", i.e. this system will act as an ARP server. This is the "magic" that is needed to communicate to and from the Hercules host outside of the computer it is running on. It is equivalent to setting `proxy_arp` to '1' on Linux, but not exactly as proxy arp is always `ON` in a OSX environment. Rather, it just "clarifies" what host and interface you want to use with proxy arp.

`en0` is the name of the interface (the ethernet card).

The rest of IP setup is the same as any other system you will be running on (Linux, Windows, etc.) and all applies to parameters that must be set on the mainframe operating system you are using (all this is documented in the Hercules TCP/IP documentation).

Since none of the above is documented, I wanted to document it here for everyone to see, should they want to run Hercules on a Mac (and the Unix terminal system) it runs on.

It's incredible how _fast_ my mainframe is running on such a small box. Incredible. I find Hercules runs better on a Mac than on any other platform. Linux works well, but a Mac is much more usable in other ways than Linux is (for other things).

Well, thank you all for your patience. I sit up all night trying to figure this stuff out because I can't sleep, and I want to share what I have found as many of you have helped me (especially in the early days) when I was learning how to use Hercules.
