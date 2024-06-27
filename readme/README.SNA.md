![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Hercules SNA support

## Contents

1. [Introduction](#LCS-Device,-SNA-enhanced)
2. [Physical connection to the outside world](#Physical-connection-to-the-outside-world)
3. [Hercules Configuration](#Hercules-Configuration)
4. [Guest OS](#Guest-OS)
5. [VTAM configuration](#VTAM-configuration)
6. [Making it run](#Making-it-run)
7. [Reporting Bugs](#Reporting-Bugs)
8. [Acknowledgements](#Acknowledgements)


## LCS Device, SNA enhanced

The current implementation of the LCS device has been enhanced to handle SNA 802.2 DLC frames.
This support is not 100% finished. It is known that the implementation of packet pacing - being
a mandatory requirement in SNA networking - isn't implemented completely. This can give rise to
a multitude of timing-related issues. See "Reporting Bugs" below.

Compiling Hercules is out of scope for this documentation since it is
<a href="https://sdl-hercules-390.github.io/html/hercinst.html#instsource">documented elsewhere</a>.

However, communication in general works fine, with some exceptions. One known is that there is
an issue with using VTAM's APING with very large packets, and/or high CONSEC never completes.
If you don't know what that is, you probably aren't affected anyway.

You should be aware that the popular Turnkey Systems (TK3, TK4-) are based on a very old release
of the OS and accompanying VTAM code. These "don't know" about the LCS device and thus can't benefit
from the introduced SNA to LAN support.

Most examples shown here are collected from the
"<a href="https://github.com/SDL-Hercules-390/hyperion/issues/348">**Request for Native SNA support**</a>"
Github issues #348 discussion page.


### Physical connection to the outside world

The SNA protocols aren't IP, and thus Hercules needs some means to send and receive bare Ethernet
frames to get them onto the network medium. Therefore, Hercules supports the TUNTAP interfaces.
TAP interfaces work on Layer 2 of the OSI stack and provide the necessary functionality.

* _**NOTE:** The following text assumes you are using **wired Ethernet.** WiFi in theory should be a
pure OSI Layer 2 infrastructure, being agnostic to L3 protocols. In practice, it often is not.
There are numerous combinations of WiFi chipsets, drivers, access points and bridges out there
"in the wild", and each and every one can block SNA frames for reasons unknown to the end user.
It's therefore safe to assume that **"SNA over WiFi will not work"** unless testing for a given
combination proves otherwise._

The general lack of proper documentation for TUNTAP makes it necessary to have some example
configurations shown here, so users can get ahead quickly. However, a basic understanding
about networking certainly helps.

This documentation frequently mentions two terms:

- The Host is the operating system running Hercules, most often Linux or Windows.
- The Guest is the operating system running within Hercules. This only recognizes running on some kind of IBM Mainframe Hardware.

The Hercules LCS drives an invisible virtual internal port being one end of the "TAP pipe",
while the other end appears as an ordinary network interface to the host operating system.
We'll focus on the host end for now.

Getting network frames onto the physical medium from a virtual port involves
"<a href="https://en.wikipedia.org/wiki/Bridging_(networking)">Bridging</a>".

The Linux Kernel provides the necessary components to bridge multiple network interfaces.
In the following  annotated, distribution-independent example, eth0 (a physical network interface)
and tap0 (a virtual TAP type interface) are being bridged.


<pre>
# Add a virtual bridge interface.
/sbin/ip link add br0 type bridge

# Create a TAP interface for Hercules to be used.
/sbin/ip tuntap add mode tap dev tap0

# Set some parameters for the TAP.
/sbin/ip link set dev tap0 address <b>02:00:FE:DF:00:42</b>
/sbin/ip link set dev tap0 mtu 1500

# Associate tap0 and eth0 to the bridge.
/sbin/ip link set dev eth0 master br0
/sbin/ip link set dev tap0 master br0

# Set all interfaces to running state.
/sbin/ip link set dev eth0 up
/sbin/ip link set dev tap0 up
/sbin/ip link set dev br0 up
</pre>


Shown are run-time commands for bridging a separate NIC (eth0). Because there is no IP involved,
there are no IP addresses to be configured.

One important thing to note is the **MAC address**. The MAC address being set on the TAP interface
is automatically being incremented by one to derive the MAC address of the virtual "internal" port
for the guest OS side. On the example above, it is <i>02:00:FE:DF:00:43</i>. This means, if you
configure adjacent SNA peers, you need to use the MAC address +1 for the destination MAC address
and not the one you have configured!

Finally, the MAC address you choose must adhere to
<a href="https://sdl-hercules-390.github.io/html/hercconf.html#LCS">certain rules</a>:

* _**Note:** The MAC address you specify for this option MUST have the 02 locally assigned MAC bit on in the first byte, must NOT have the 01 broadcast bit on in the first byte, and MUST be unique as seen by all other devices on your network segment. It should never, for example, be the same as the host adapter MAC address specified on the iface parameter._

_(FIXME: It is not clear if the first two restrictions still apply, and if they do, why.)_

The trickier part is how to make these commands being run automatically at boot time.
This is highly dependent on which Linux distribution is used.

Jay Maynard provided the following examples, for Raspberry PI running the 64-bit version of
Raspberry Pi OS, and on his desktop running Pop!_OS 21.04.

On both hosts, I've configured three tap interfaces, corresponding to three LCS devices,
one IP for VM/ESA, one IP for OS/390, and one SNA for OS/390. VM/ESA uses E20-E21 on tap0;
OS/390 IP uses E22-E23 on tap1; and OS/390 SNA uses E40 on tap2.

The Raspberry Pi uses `if-up` and `if-down`. All of the configuration you need is in the file
`/etc/network/interfaces`.

Here's mine:


```
# interfaces(5) file used by ifup(8) and ifdown(8)

# Please note that this file is written to be used with dhcpcd
# For static IP, consult /etc/dhcpcd.conf and 'man dhcpcd.conf'

# Include files from /etc/network/interfaces.d:
# source-directory /etc/network/interfaces.d

# Actually, we're going to be a bit more complex with our network,
# which dhcpcd can't handle, so we'll do it here.

auto lo
iface lo inet loopback

# The eth0 interface does not have an IP address...the bridge does that.
# This configuration stanza brings up the interface without an address.
auto eth0
iface eth0 inet manual
    pre-up ifconfig $IFACE up
    post-down ifconfig $IFACE down

# Three tap devices are defined for Hercules to use as LCS interrfaces.
auto tap0
iface tap0 inet manual
    pre-up ip tuntap add tap0 mode tap user jmaynard
    up ip link set dev tap0 up
    post-down ip link del dev tap0

auto tap1
iface tap1 inet manual
    pre-up ip tuntap add tap1 mode tap user jmaynard
    up ip link set dev tap1 up
    post-down ip link del dev tap1

auto tap2
iface tap2 inet manual
    pre-up ip tuntap add tap2 mode tap user jmaynard
    up ip link set dev tap2 up
    up ip link set dev tap2 address 02:00:03:15:1e:e6
    post-down ip link del dev tap2

# Finally, the bridge setup. Here's where the actual TCP/IP
# parameters are set. All three tap devices are bridged.
auto br0
iface br0 inet static
    address 192.168.120.20
    netmask 255.255.255.0
    gateway 192.168.120.1
    dns-nameservers 192.168.120.1
    dns-domain conmicro.com
    bridge_ports tap0 tap1 tap2 eth0
    up /sbin/brctl setageing br0 0
    up /sbin/brctl stp br0 off
    up ip link set dev br0 address e4:5f:01:0a:ef:ee
```


Pop!_OS, like Ubuntu and probably other derivatives, uses Network Manager. The needed
setup can be done with the Network Manager CLI tool, `nmcli`:


```
nmcli con add type bridge ifname br0 \
      ethernet.cloned-mac-address 24:4b:fe:55:79:a0
nmcli con add type bridge-slave ifname enp7s0 master br0
nmcli connection add type tun ifname tap0 con-name tap0 slave-type bridge \
      master br0 mode tap owner `id -u`
nmcli connection add type tun ifname tap1 con-name tap1 slave-type bridge \
      master br0 mode tap owner `id -u`
nmcli connection add type tun ifname tap2 con-name tap2 slave-type bridge \
      master br0 mode tap owner `id -u` \
      ethernet.cloned-mac-address 02:00:03:15:1e:a6
```


Jeff Snyder provided the following example configuration for a newly installed
Ubuntu 20.04.3 server:

Set up the initial bridge to the ethernet inteface by updating the
`/etc/netplan/00-installer-config.yaml` file to contain:


```
# This is the network config written by 'subiquity'
network:
  version: 2
  renderer: networkd
  ethernets:
    eth0:
      dhcp4: no
      dhcp6: no
  bridges:
    br0:
      interfaces: [eth0]
      macaddress: 02:21:01:00:00:00
      dhcp4: no
      addresses:
      - 192.168.1.4/24
      gateway4: 192.168.1.254
      nameservers:
        addresses:
        - 192.168.1.254
        - 8.8.8.8
        search:
        - Snyder.ORG
      dhcp6: no
```


_**Note:**_ Update the IP addresses and domain names to match your local requirements,
or use DHCP on the bridge interface by changing `dhcp4:` to `yes` and removing the
`addresses:`, `gateway4:` and `nameservers:` information. If you want this interface to be
SNA-only, you could probably use `DHCP4:no` and not include any address information, though
I haven't tried that. Reboot or use `'netplan apply'` to apply any changes to this file to
your network.

I have not found a way to create taps in netplan yet, so for now, I do them after the system
is started, with a script:


```
#!/usr/bin/bash

for i in `seq 0 2`; do
    sudo ip tuntap add name tap$i mode tap;
    sudo ip link set up dev tap$i;
    sudo ifconfig tap$i hw ether 02:21:02:0$i:00:00
    sudo ip link set tap$i master br0;
done;
```


This creates 3 taps, tap0, tap1 and tap2 and bridges them with the ethernet interface under
br0. For now, I only need 2 taps (one for TCPIP and one for SNA), but I expect to need more
in the future. Note that the mac address for the taps ends in ":00". Hercules will add 1 to
the mac address for the "OS" end of the connection, so it will look like ":01" to the guest OS.


### Hercules Configuration

The hercules.cnf syntax and options are documented
<a href="https://sdl-hercules-390.github.io/html/hercconf.html">here</a>.

Currently we use predefined TAP interfaces for SNA. First, because on creation time, an owning
user can be set so Hercules doesn't need to run as root _(FIXME: True?)_, and the MAC address
can be set manually so it will always stay the same. Something highly desirable when using
manually configured adjacencies.

Normally, you'd need just this:

```
  0E40  LCS  -e SNA  tap0
```

For debugging purposes, use these two lines:

<pre>
  0E40  LCS  -e SNA  <b>-d</b> tap0
  <b>t+0E40</b>
</pre>


### Guest OS

Before trying anything, I applied the change regarding a _missing interrupt handler_ as laid out
in the _New User's Cookbook_ accompanying the distribution.

_(FIXME: Is this mandatory?)_

#### MIH

It required changing the parmlib member _IECIOS00_:


```
  MIH TIME=00:00,DEV=E40
```

I verified successfully that it works after the next IPL, with `D IOS,MIH,DEV=E40`:

```
  0E40=00:00
```

**Note:** I have added the same configuration change to the E20-E21 devices (CTCI), used solely for
IP connectivity before I tried to get E40 running. So far, I've not observed any adverse effects.


### VTAM configuration

This is the configuration to be applied to the guest OS. In this case, it is OS/390 ADCD
version 2.10. A copy can be obtained from archive.org:

* https://archive.org/details/OS390_V2R10_ADCD

Note that you're still obliged to obtain a license to be allowed to use this OS.

Configuring Hercules to run this OS is out of scope for this document.

Steps being necessary to make the environment being presented on the CD images compatible
so it can be run with Hercules is out of scope for this document.

~Extensive documentation about VTAM configuration can be found  at https://ibmdocs.pocnet.net.
Scroll down to (or search for) the _"z/OS Communications Server"_ section.~

_(Note: IBM documentation is no longer available from [pocnet.net](https://ibmdocs.pocnet.net)
due to copyright issues. It is recommended that you use
[IBM's Publication Center](https://www.ibm.com/resources/publications) instead.)_

Of particular interest might be:

- <a href="https://publibfp.dhe.ibm.com/epubs/pdf/f1a2b513.pdf">SNA Network Implementation Guide</a>
- <a href="https://publibfp.dhe.ibm.com/epubs/pdf/f1a2b613.pdf">SNA Resource Definition Reference</a>
- <a href="https://publibfp.dhe.ibm.com/epubs/pdf/f1a2e700.pdf">SNA Resource Definition Samples</a>

The configuration shown below is by no means refined and accordingly reduced to a working
minimum set of definitions yet. Helpful input and testing is well appreciated! Nonetheless,
this configuration is tested to work exactly as shown.

#### SYS1.LOCAL.VTAMLST(ATCSTR00)

I left out the *IPADDR* and *TCPNAME* parameters for my EE configuration, since they are irrelevant.
Apart from that, VTAM is told to be an APPN network node with no subarea functionality. The buffer
parameters were left at default from the original *ATCSTR00*.

Just in case, create a backup copy before making changes!

```
CONFIG=00,                                                             -
CONNTYPE=APPN,                                                         -
CPCDRSC=YES,                                                           -
CPCP=YES,                                                              -
DATEFORM=YMD,                                                          -
DYNLU=YES,                                                             -
DYNADJCP=YES,                                                          -
NETID=POCNET,                                                          -
NODETYPE=NN,                                                           -
NOPROMPT,                                                              -
NQNMODE=NQNAME,                                                        -
SSCPID=59,                                                             -
SSCPNAME=LRRR,                                                         -
SUPP=NOSUP,                                                            -
VERIFYCP=NONE,                                                         -
VFYRED=YES,                                                            -
CRPLBUF=(208,,15,,1,16),                                               -
IOBUF=(400,508,19,,1,20),                                              -
LFBUF=(104,,0,,1,1),                                                   -
LPBUF=(64,,0,,1,1),                                                    -
SFBUF=(163,,0,,1,1)
```

All other PDS members are new.

#### SYS1.LOCAL.VTAMLST(CDRSC)

I'm not sure if this is required for certain APPN scenarios or only when VTAM acts as a node
in both Subarea and APPN networks.


```
         VBUILD TYPE=CDRSC
*
ILU1     CDRSC ALSLIST=ISTAPNPU
```

#### SYS1.LOCAL.VTAMLST(XCAETHNT)

This references the LCS device from the Hercules configuration at address *E40*. Hint: The ADCD System
has all necessary IODF correctly set by default to support an LCS 3172 at E40.


```
XCAETH  VBUILD TYPE=XCA
*
ETHPRT    PORT ADAPNO=0,                                               -
               CUADDR=E40,                                             -
               DELAY=0,                                                -
               MEDIUM=CSMACD,                                          -
               SAPADDR=4,                                              -
               TIMER=30
*
ETHGRP   GROUP ANSWER=ON,                                              -
               AUTOGEN=(10,E,X),                                       -
               CALL=INOUT,                                             -
               DIAL=YES,                                               -
               DYNPU=YES,                                              -
               DYNPUPFX=ET,                                            -
               ISTATUS=ACTIVE
```

#### SYS1.LOCAL.VTAMLST(NIBBL802)

This is the *switched major node* defining my AS/400, called *NIBBLER*. NETID and CPNAME
can be found by issuing a DSPNETATR. IDBLK/IDNUM, and MAXDATA can be found by looking at
the line description for the LAN IOA with WRKLIND, Option 5. The DIALNO is the SAP address
directly followed by the IOA's MAC address. Both to be found in the line description.
SAP address 4 is the default, though.


```
NIBSWN   VBUILD TYPE=SWNET
*
NIBPU    PU    ADDR=01,                                                -
               CONNTYPE=APPN,                                          -
               CPCP=YES,                                               -
               CPNAME=NIBBLER,                                         -
               DISCNT=NO,                                              -
               DWACT=NO,                                               -
               DYNLU=YES,                                              -
               HPR=NO,                                                 -
               IDBLK=056,                                              -
               IDNUM=41700,                                            -
               ISTATUS=ACTIVE,                                         -
               MAXDATA=1496,                                           -
               MAXOUT=7,                                               -
               MODETAB=ISTINCLM,                                       -
               NETID=POCNET,                                           -
               PACING=7,                                               -
               PUTYPE=2,                                               -
               SSCPFM=USSSCS,                                          -
               VPACING=7
*
NIBPTH   PATH  GRPNM=ETHGRP,                                           -
               DIALNO=0004002035B54164
```

Not that the DIALNO is a concatenation of the DSAP and the peer's MAC address.

#### SYS1.LOCAL.VTAMLST(ATCCON00)

As usual, this contains the resources to be activated at VTAM start time. I removed some
apparently unneeded resources just giving error messages at startup time.


```
A0600,                                                                 -
NSNA70X,                                                               -
NSNA90X,                                                               -
COSAPPN,                                                               -
A0TCP,                                                                 -
P390APP,                                                               -
DYNMODEL,                                                              -
XCAETHNT,                                                              -
NIBBL802
```


### Making it run

AFAIK it's not possible to dynamically reconfigure VTAM to the extent necessary with the new
ATCSTR00. So, make the changes in ATCSTR, but leave out the switched major node for the other
SNA node from ATCSTR00, and re-IPL. Alternatively, stop TCAM and VTAM, and restart both
in reverse order.

I've observed that having the switched major node being activated while VTAM starts up, the
connection won't come up. I don't know if this is a VTAM timing/retry configuration issue, or
related to side effects with the new SNA code in LCS.

Issuing the proper vary command on the console for the major node yields:


```
- 17.56.37           V NET,ID=NIBBL802,ACT
  17.56.37 STC00004  IST097I VARY ACCEPTED
  17.56.37 STC00004  IST093I NIBPU ACTIVE
  17.56.37 STC00004  IST093I NIBBL802 ACTIVE
  17.56.37 STC00004  IST590I CONNECTOUT ESTABLISHED FOR PU NIBPU ON LINE
   E0E40000
  17.56.37 STC00004  IST1086I APPN CONNECTION FOR APPN.NIBBLER IS ACTIVE
   - TGN = 21
  17.56.37 STC00004  IST1096I CP-CP SESSIONS WITH APPN.NIBBLER ACTIVATED
```

For APPN Connections from End Node to (only one!) Network Node, and Network Node to Network Node
to fully function, it's crucial to see CP-CP Sessions come up. I successfully tested the connection
with APING in both directions. Output from VTAM:


```
04- 17.56.54           D NET,APING,ID=NIBBLER,LOGMODE=#INTER
    17.56.54 STC00003  IST097I DISPLAY ACCEPTED
    17.56.54 STC00003  IST1489I APING SESSION INFORMATION
    IST1490I DLU=APPN.NIBBLER SID=FD8F3F629B0B3271
    IST933I LOGMODE=#INTER  , COS=#INTER
    IST875I APPNCOS TOWARDS SLU = #INTER
    IST1460I TGN  CPNAME             TG TYPE      HPR
    IST1461I  21  APPN.NIBBLER       APPN         *NA*
    IST314I END
    17.56.55 STC00003  IST1457I VTAM APING VERSION 2R33 (PARTNER TP VERSION
     2R43)
    IST1490I DLU=APPN.NIBBLER SID=FD8F3F629B0B3271
    IST1462I ECHO IS ON
    IST1463I ALLOCATION DURATION: 34 MILLISECONDS
    IST1464I PROGRAM STARTUP AND VERSION EXCHANGE: 723 MILLISECONDS
    IST1465I         DURATION      DATA SENT   DATA RATE   DATA RATE
    IST1466I      (MILLISECONDS)    (BYTES)  (KBYTE/SEC)  (MBIT/SEC)
    IST1467I               17            200          11           0
    IST1467I               16            200          12           0
00  IST1468I TOTALS:       33            400          12           0
    IST1469I DURATION STATISTICS:
    IST1470I MINIMUM = 16 AVERAGE = 16 MAXIMUM = 17
    IST314I END
```

My VTAM configuration also consists of an Enterprise Extender (SNA over UDP) definition (not shown).
Sessions forwarded by VTAM between my AS/400 (because of it's old OS only being able to support
802.2 DLC connections), and a newer IBM i 7.2 machine (because of it's new OS only being able to
support SNA over Enterprise Extender) work flawlessly.

This is a definition as submitted by Jay Maynard:

My switched network major node, on one of the R/390s. That R/390 is subarea 14; subarea 02
is the desktop Linux OS/390, 06 is the Raspberry Pi, 10 is my Linux laptop (which I haven't
set this up on yet, but expect no trouble now that I know where the pitfalls are),
and 18 is the other R/390:


```
S14SWNET VBUILD TYPE=SWNET
* APPN LINKS OVER ETHERNET
* NOTE: HERCULES MAC ADDRESSES ARE ONE HIGHER THAN CONFIGURED ON THE
*       HERCULES HOST'S TAP INTERFACE SETUP
P14CP02  PU    MAXPATH=5,MAXDATA=256,ADDR=02,                          X
               CPNAME=H02SSCP,CPCP=YES,PUTYPE=2
L14CP02  PATH  DIALNO=0104020003151EA7,GRPNM=X14E40G1
A14CP02  LU    LOCADDR=0,ISTATUS=INACTIVE
*
P14CP06  PU    MAXPATH=5,MAXDATA=256,ADDR=02,                          X
               CPNAME=H06SSCP,CPCP=YES,PUTYPE=2
L14CP06  PATH  DIALNO=0104020003151EE7,GRPNM=X14E40G1
A14CP06  LU    LOCADDR=0,ISTATUS=INACTIVE
*
P14CP10  PU    MAXPATH=5,MAXDATA=256,ADDR=02,                          X
               CPNAME=H10SSCP,CPCP=YES,PUTYPE=2
L14CP10  PATH  DIALNO=0104020003151E97,GRPNM=X14E40G1
A14CP10  LU    LOCADDR=0,ISTATUS=INACTIVE
*
P14CP18  PU    MAXPATH=5,MAXDATA=256,ADDR=01,                          X
               CPNAME=H18SSCP,CPCP=YES,PUTYPE=2
L14CP18  PATH  DIALNO=0104020003151E04,GRPNM=X14E40G1
A14CP18  LU    LOCADDR=0,ISTATUS=INACTIVE
```


### Reporting Bugs

First, please always make sure you use the most recent code base. It's completely sufficient
to run `git pull && make && make install`. No `configure` and time consuming full-compile
are necessary after the initial run.

In addition to providing the usual information for understanding any issues, please include:

- a packet capture restricted to the MAC addresses of the hosts involved. Example:
<br>`tcpdump -w debug.pcap -n -i tap0 ether host 02:00:fe:df:00:43 or ether host 10:00:e0:5e:b6:c5`

- Hercules log output (redirect stdout to a file prior to start or use the new `-o` command line
option).
<br>Please only submit the relevant time frame of the log.

Often, a VTAM trace to see what VTAM is doing is mandatory. You do need to use GTF. Unfortunately,
I can't recall all of the exact detail because its been about ten years since I last did any VTAM
traces and so I am trying to remember all of this from memory. But basically, the process is
something like this:


<pre>
  START GTF           <i>(an operator command)</i>
</pre>

After GTF starts, it will prompt you at the console for the kind of trace you want and what types
of records you wish to trace. As best I can recall, VTAM traces needed GTF to record USR trace entries.

After GTF is satisfied, then you start the actual VTAM trace itself, another operator command:


```
  F NET,TRACE,TYPE=type,ID=node_name
```

Where, 'type' is BUF for a buffer trace of the data stream, or you might use type IO for an i/o trace
or CNM for a network management trace. For your issue described, either one of these might be beneficial
but some more than others.

ID=node_name is the name of the VTAM entity you wish to trace, for example the node or controller or
line you are trying to activate as described in this Github issue above.

Once the F NET trace is started, then you would issue VARY NET to activate your node. The trace data
should be recorded to GTF. Once you are satisfied that you have recreated your situation, stop the traces:


```
  F NET,NOTRACE,TYPE=buf/io/cnm,ID=node_name
  P GTF
```

The trace data should be in your GTF trace dataset. In order to get it out of there, you need to
format it. It used to be done with a service aid utility like AMDPRDMP or something like that
but I think now you have to use IPCS to view the trace in a formatted style.

Before starting GTF, you'll also need to review the GTF started task procedure to ensure the trace
datasets are defined and that you can access them. It's rather an ordeal to get all of this set up
for the first time, but once done, it is easy to just issue the commands to start and stop the traces
as needed and review the results, repeatedly.

In order to find the data you are looking for, you'll need a manual which I believe was called
**[SNA Formats](https://publibfp.dhe.ibm.com/epubs/pdf/d50a5005.pdf)**.
It should have the layout and description of the data streams that VTAM will send and receive
from your node. I believe that they were called PIUs.

I wish I could be more specific. I used to keep notes on all of this stuff so I could use it when needed
but after I retired I don't have access to these notes any longer. There is also a manual called
**[z/OS MVS Diagnosis Tools and Service Aids](https://www-01.ibm.com/servers/resourcelink/svc00100.nsf/pages/zOSV2R3ga320905?OpenDocument)**.
In there is a whole chapter on GTF and how to set it up, answer the prompts, and how to view the trace.
Also, for the format of the VTAM `F NET,TRACE` command, look in a manual called **VTAM Operation**.

While none of this immediately gets you going, hopefully there is enough here to get you pointed in the
right direction where you can start tracking down how to do it and get what you need.


### Acknowledgements

* Huge thanks to Ian Shorter for his incredible work in making SNA over LCS possible.

* Thanks to Marco Lorig for providing a remotely accessible R/390 environment for initial VTAM traces,
and packet dumps on a "real" LCS.

* Thanks to Jeff Snyder for providing an initial VTAM configuration and some more configuration examples
to work with.

* Thanks to Jay Maynard for working out how to properly integrate bridging into some distribution
specific mechanisms.

* Thanks to Bob Polmanter for a short summary about how to do a GTF trace.

For readers interested in participating in platform-independent discussions revolving around mainly SNA,
you might find the following to be beneficial:

&nbsp; &nbsp; ibm-big-iron-networking GROUP WEB PAGE: &nbsp;https://groups.io/g/ibm-big-iron-networking
<br>
&nbsp; &nbsp; ibm-big-iron-networking EMAIL ADDRESS: &nbsp; &nbsp; ibm-big-iron-networking@groups.io
<br>

\-\-\- 
<br>Patrik Schindler
<br>October 2021
<br>
<br>
<br>
<br>
