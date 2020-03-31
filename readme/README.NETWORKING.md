![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Hercules Networking

## Contents

1. [About](#About)
2. [Details](#Details)
3. [Emulation Modes](#Emulation-Modes)

## About

_(Please read [herctcp.html](https://sdl-hercules-390.github.io/html/herctcp.html) as Roger explains how to set up TCP/IP networking with Hercules.)_

Most of the network communication emulations implemented within Hercules use a CTCA (Channel to Channel Adapter) type device. Depending on the "flavor", the CTCA device will provide either a point-to-point or a virtual network adapter interface to the driving system's TCP/IP stack or in the case of CTCE, a true CTCA connection to another instance of Hercules via a TCP/IP connection.

Most current emulations, with the exception of, CTCT and CTCE use the Universal TUN/TAP driver on *nix and TunTap32 (WinPCap) on the Windows platforms which creates a network interface on the driving system which allow Hercules to present frames to, and receive frames from the TCP/IP stack. This network interface is configured on *nix platforms by the `hercifc` program which is invoked by Hercules after the TUN/TAP device is opened. The `hercifc` program runs as root. Please read [herctcp.html](https://sdl-hercules-390.github.io/html/herctcp.html) for more information on the security implications of the `hercifc` program.

## Details

_(Important information about changes to the Hercules configuration files! **PLEASE READ!**)_

The format of the Hercules configuration file statements for all of the networking emulations have changed from the previous releases of Hercules. While the older format may still be accepted for a short while in order to maintain compatibility, it is recommended to switch to the new format as soon as possible. Also note that there is no distinction between the CTCI and CTCI-W32 modes any more, in fact CTCI-W32 does not exist in this release.

In releases prior to Hercules version 3.00, all of the TCP/IP emulations required two addresses to be defined in the Hercules configuration file: one address for the read subchannel and the other for write.

Hercules release version 3.00, however, [temporarily] changed the rules: With [_**ONLY!**_] version 3.00 of Hercules, only the _first_ address need be specified in the configuration file. Hercules version 3.00 automatically creates the second address. Care must be taken to _not_ define the second address [_with Hercules version 3.00 **ONLY!**_] or a configuration error will occur.

Starting with Hercules version 3.01 however, we've gone back to doing things the way we were _originally_ (and the way most users were used to) doing them (i.e. the way most users _expect_ things to work):

With Hercules version 3.01 you need to define _both_ addresses.

Both the even numbered read device _as well as_ the odd numbered write device must _both_ be defined in your Hercules configuration file starting with Hercules version 3.01. We apologize for the mess, but we thought having Herc automatically define the write
device for you (as it does in version 3.00) would be easier for everyone. Turns out it caused a lot of problems with a lot of people, so we decided to go back to the original way. Again, we apologize for whatever headaches this may have caused anyone.

Note that the CTCT protocol has always required both addresses to be defined (i.e. _all_ versions of Hercules, including version 3.00 as well, require _both_ even/odd read/write devices to be defined separately [in your Hercules configuration file]).

## Emulation Modes

The currently supported emulation modes are:

```
CTCT     - CTCA Emulation via TCP connection
CTCE     - Enhanced CTCA Emulation via TCP connection
CTCI     - Point-to-point connection to the host IP stack.
LCS      - LAN Channel Station (3172/OSA)
PTP      - Point-to-point connection to the host IP stack.
OSA      - Open Systems Adapter
```

### CTCT - Channel to Channel Emulation via TCP connection

This emulation mode provides protocol-independent communication
with another instance of this driver via a TCP connection.

This mode appears to the operating system running in the Hercules
machine as an IBM 3088 Channel to Channel Adapter and can operate
in either Basic or Extended mode.

The configuration statement for CTCT is as follows:

``
<devnum> 3088 CTCT <lport> <raddress> <rport> <mtu>
``

where:

```
<devnum>   is the address of the CTCT device.
<lport>    is the TCP/IP port on the local system.
<raddress> is the IP address on the remote.
<rport>    is the TCP/IP port on the remote system.
```

### CTCI     - Channel to Channel link to TCP/IP stack

This is a point-to-point link to the driving system's TCP/IP stack. From the point of view of the operating system running in the Hercules machine it appears to be a CTC link to a machine running TCP/IP for MVS or VM.

CTCI uses the Universal TUN/TAP driver on \*nix and Riverbed Technology's WinPCap packet driver as well as Fish's [CTCI-WIN product](http://www.softdevlabs.com/ctci) on Windows **(*)**.

The configuration statement for CTCI is as follows:

```
<devnum1-devnum2> CTCI [options] <guestip> <hostip>
```

where:

```
<devnum1-devnum2>   is the address pair of the CTCI device.
<guestip>  is the IP address of the Hercules (guest OS) side.
<hostip>   is the IP address on the driving system.
[options]  can be any of the following:
         -n <devname> or --dev <devname>
             where <devname> is:
             [*nix] the name of the TUN/TAP special character
             device, normally /dev/net/tun.
             [Windows] is either the IP or MAC address of the
             driving systems network card. TunTap32 will
             automatically select the first network card it
             finds if this option is omitted, this may not be
             desirable for some users.

         -t <mtu> or --mtu <mtu>

             [*nix only] where <mtu> is the maximum transmission
             unit size, normally 1500

         -s <netmask> or --netmask <netmask>

             [*nix only] where <netmask> is the netmask to
             be configured on the link. Note: Since this is a
             point-to-point link netmask is meaningless from
             the perspective of the actual network device.

         -m <MAC Address> or --mac <MAC address>

             [Windows only] where <MAC Address> is the optional
             hardware address of the interface in the format of
             either xx:xx:xx:xx:xx:xx or xx-xx-xx-xx-xx-xx.

         -k <kbuff> or --kbuff <kbuff>

             [Windows only] where <kbuff> is the size of the
             WinPCap kernel buffer size, normally 1024.

         -i <ibuff> or --ibuff <ibuff>

             [Windows only] where <ibuff> is the size of the
             WinPCap I/O buffer size, normally 64.

         -d or --debug

             this will turn on the internal debugging routines.
             Warning: This will produce a tremendous amount of
             output to the Hercules console. It is suggested that
             you only enable this at the request of the maintainers.
```

### LCS - LAN Channel Station

This emulation mode appears to the operating system running in the Hercules machine as an IBM 8232 LCS device, an IBM 2216 router, a 3172 running ICP (Interconnect Communications Program), the LCS3172 driver of a P/390, or an IBM Open Systems Adapter.

Rather than a point-to-point link, this emulation creates a virtual ethernet adapter through which the guest operating system running in the Hercules machine can communicate. As such, this mode is not limited to TCP/IP traffic, but in fact will handle any ethernet frame.

The configuration statement for LCS is as follows:

```
<devnum1-devnum2> LCS [options] [<guestip>]
```

where:

```
<devnum1-devnum2>   is the address pair of the LCS device.
                This pair must be an even-odd address.

     [<guestip>] is an optional IP address of the Hercules
                (guest OS) side. Note: This is only used to
                establish a point-to-point routing table entry
                on driving system. If you use the --oat option,
                do not specify an address here.
```

There are no required parameters for the LCS emulation, however there are several options that can be specified on the config statement:

```
     -n <devname> or --dev <devname>

         where <devname> is:

         [*nix] the name of the TUN/TAP special character device,
         normally /dev/net/tun.

         [Windows] is either the IP or MAC address of the driving
         systems network card. TunTap32 will automatically select
         the first network card it finds if this option is
         omitted, this may not be desirable for some users.

     -o <filename> or --oat <filename>

         where <filename> specifies the filename of the Address
         Translation file. If this option is specified, the optional
         <guestip> and --mac entries are ignored in preference to
         statements in the OAT. (See below for the format of the OAT)

     -m <MAC Address> or --mac <MAC address>

         where <MAC Address> is the optional hardware address of
         the interface in the format: xx:xx:xx:xx:xx:xx

     -d or --debug

         this will turn on the internal debugging routines.
         Warning: This will produce a tremendous amount of
         output to the Hercules console. It is suggested that
         you only enable this at the request of the maintainers.
```

If no Address Translation file is specified, the emulation module will create the following:

*     An ethernet adapter (port 0) for TCP/IP traffic only.
*     Two device addresses will be defined (devnum and devnum + 1).


The syntax for the Address Translation file is as follows:

```
*********************************************************
* Dev Mode Port Entry specific information              *
*********************************************************
  0400  IP   00  PRI 172.021.003.032
  0402  IP   00  SEC 172.021.003.033
  0404  IP   00  NO  172.021.003.038
  0406  IP   01  NO  172.021.002.016
  040E  SNA  00

  HWADD 00 02:00:FE:DF:00:42
  HWADD 01 02:00:FE:DF:00:43
  ROUTE 00 172.021.003.032 255.255.255.224
```

where:

```
Dev   is the base device address
Mode  is the operation mode - IP or SNA
Port  is the virtual (relative) adapter number.
```

For `IP` modes, the Entry-specific information is as follows:

```
     PRI|SEC|NO  specifies where a packet with an unknown IP
                 address is forwarded to. PRI is the primary
                 default entry, SEC specifies the entry to use
                 when the primary is not available, and NO
                 specifies that this is not a default entry.
```

When the operation mode is `IP`, specify only the even (read) address; the odd (write) address is created automatically. If an odd address is specified the read/write functions of the pair are swapped.

Note: SNA mode does not currently work.

Additionally, two other statements can be included in the address translation file. The `HWADD` and `ROUTE` statements.

Use `HWADD` to specify a hardware (MAC) address for a virtual adapter. The first parameter after `HWADD` specifies with relative adapter for which the address is applied.

The `ROUTE` statement is included for convenience. This allows the `hercifc` program to create a network route for this specified virtual adapter. Please note that it is not necessary to include point-to-point routes for each IP address in the table. This is done automatically by the emulation module.

Up to 4 virtual (relative) adapters (ports) 00-03 are currently supported.

### CTCE - Enhanced Channel to Channel Emulation via TCP connection

The CTCE device type also emulates a _**real** 3088 Channel to Channel Adapter_ for non-IP traffic, enhancing the CTCT capabilities.  CTCE connections are also based on TCP/IP between two (or more) Hercules instances, and require a pair of port numbers on each device side.  In the previous CTCE version these had to be an even-odd pair of port
numbers, whereby only the even port numbers had to be specified in the CTCE configuration.  This restriction has now been removed.  Any port number > 1024 and < 65534 is allowed.  The CTCE configuration specifies the port number at the receiving end, the sender side port number is a free randomly chosen one.  The resulting socket pairs cross-connect, the arrows showing the send->receive direction :

```
    x-lport-random --> y-rport-config
    x-lport-config <-- y-rport-random
```

The configuration statement for CTCE is as follows, choosing one of 2 possible formats (noting that items between [] brackets are optional, and the items between <> brackets require actual values to be given):
```
     <ldevnum>     CTCE <lport> [<rdevnum>=]<raddress>  <rport>  [[<mtu>] <sml>] [ATTNDELAY <delay>] [FICON]
     <ldevnum>[.n] CTCE <lport> [<rdevnum>]=<raddress> [<rport>] [[<mtu>] <sml>] [ATTNDELAY <delay>] [FICON]
```
where:
```
     <ldevnum>    is the address of the CTCE device at the local system.
     <rdevnum>    is the address of the CTCE device at the remote system.
     <lport>      is the receiving TCP/IP port on the local system,
                  which defaults to 3088.
     <rport>      is the receiving TCP/IP port on the remote system,
                  which also defaults to 3088.
     <raddress>   is the host IP address on the remote system.
     <mtu>        optional mtu buffersize, defaults to 61596.
     <sml>        optional small minimum for mtu, defaults to 16.
     n            the number of CTCE devices being configured, with
                  consecutive addresses starting with <ldevnum>.
                  (Only possible in the 2nd format, which implies the
                  equal sign (=) in front of <raddress>.)
     delay        the number of msec ATTN signals are to be delayed in order to
                  circumvent a VM/SP bug causing SIO timeout errors.  Override
                  the default of 0, e.g. with 200 or some smaller value for
                  faster Hercules hosts, but only when needed.
     FICON        optional parameter specifying a FICON Channel-to-Channel adapter
                  to be emulated (i.e. a FCTC instead of a CTCA)
```

A sample CTCE device configuration (using the 1st format) is shown below:

```
   # Hercules PC Host A with IP address 192.168.1.100:

      0E40  CTCE  30880  192.168.1.200  30880
      0E41  CTCE  30882  192.168.1.200  30882

   # Hercules PC Host B with IP address 192.168.1.200:

      0E40  CTCE  30880  192.168.1.100  30880
      0E41  CTCE  30882  192.168.1.100  30882
```

The 2nd format CTCE device configuration is easier to specify by omitting the \<lport\> and \<rport\> numbers, but specifying the \<rdevnum\> instead.  The above sample configuration can then become :

```
   # Hercules PC Host A with IP address 192.168.1.100:

      0E40  CTCE  0E40=192.168.1.200
      0E41  CTCE  0E41=192.168.1.200

   # Hercules PC Host B with IP address 192.168.1.200:

      0E40  CTCE  0E40=192.168.1.100
      0E41  CTCE  0E41=192.168.1.100
```

This can even be further simplified by (1) omitting the \<rdevnum\> as its default is being equal to \<ldevnum\>, and by (2) specifying the ".n" qualifier as ".2", meaning that 2 CTCE devices starting at "0E40" are being defined :

```
   # Hercules PC Host A with IP address 192.168.1.100:

      0E40.2  CTCE  =192.168.1.200

   # Hercules PC Host B with IP address 192.168.1.200:

      0E40.2  CTCE  =192.168.1.100
```

The above example omits \<rdevnum\>, and values for this below 0100 = 0x0100 = 256 are not allowed.  But single hex digit \<rdevnum\> values have a special menaing.  They will be used to construct \<rdevnum\>'s by exclusive-or-ing the \<ldevnum\> with such value.  As an example :

```
      0E40.4  CTCE       1=192.168.1.200
```

The above is the same as :

```
      0E40    CTCE    0E41=192.168.1.200
      0E41    CTCE    0E40=192.168.1.200
      0E42    CTCE    0E43=192.168.1.200
      0E43    CTCE    0E42=192.168.1.200
```

The above example could be used to establish a redundant pair of read/write CTC links, where each Hercules side uses the even devnum addresses for reading, and the odd ones for writing (or the other way around).  That way, the operating system definitions on each side can be identical, e.g. for a VTAM MPC CTC link :

```
CTCATRL  VBUILD TYPE=TRL
C0E40TRL TRLE  LNCTL=MPC,READ=(0E40,0E42),WRITE=(0E41,0E43)

CTCALCL  VBUILD TYPE=LOCAL
C0E40LCL PU    TRLE=C0E40TRL,XID=YES,CONNTYPE=APPN,CPCP=YES,TGP=CHANNEL
```

Please also note the optional trailing keyword FICON.  When specified, a fiber channel CTC adapter (FCTC) is being emulated, instead of a regular CTCA.

CTCE connected Hercules instances can be hosted on any Hercules supported platform (Windows, Linux, MacOS ...).  Both sides do _not_ need to be the same.

### PTP      - MPCPTP/PCPTP6 Channel to Channel link

This is a point-to-point link to the driving system's TCP/IP stack. From the point of view of the z/OS V1R5 or later image running in the Hercules machine, it appears to be an MPCPTP and/or MPCPTP6 ESCON CTC link to another z/OS image.

PTP uses the Universal TUN/TAP driver on \*nix and Riverbed Technology's WinPCap packet driver as well as Fish's [CTCI-WIN product](http://www.softdevlabs.com/ctci) on Windows **(*)**.

The configuration statement for PTP is as follows:
```
     <devnum1-devnum2> PTP [options] <guest1> <host1> [ <guest2> <host2> ]
```
or:
```
     <devnum1-devnum2> PTP [options] <ifname>
```
where:
```
     <devnum1-devnum2>  is the address pair of the PTP device.

     <guest1>  is the host name or IP address of the Hercules (guest OS) side.

     <host1>   is the host name or IP address on the driving system.

     <guest2>  is the host name or IP address of the Hercules (guest OS) side.

     <host2>   is the host name or IP address on the driving system.

             <guest1> and <host1> must both be of the same address
             family, i.e. both IPv4 or both IPv6.

             <guest2> and <host2>, if specified, must both be of the
             same address family, i.e. both IPv4 or both IPv6, and
             must not be of the same address family as <guest1> and
             <host1>.

             If a host name is specified for <guest1>, and the host
             name can be resolved to both an IPv4 and an IPv6
             address, use either the -4/--inet or the -6/--inet6
             option to specify which address family should be
             used; if neither the -4/--inet nor the -6/--inet6
             option is specified, whichever address family the
             resolver returns first will be used.

             <host1> or <host2> can be followed by the prefix size
             expressed in CIDR notation, for example 192.168.1.1/24
             or 2001:db8:3003:1::543:210f/48. For IPv4 the prefix
             size can have a value from 0 to 32; if not specified a
             value of 32 is assumed. For IPv6 the prefix size can
             have a value from 0 to 128; if not specified a value of
             128 is assumed. For IPv4 the prefix size is used to
             produce the equivalent subnet mask. For example, a
             value of 24 produces a subnet mask of 255.255.255.0.

             If <guest1>, <host1>, <guest2> or <host2> are numeric
             IPv6 addresses, they can be between braces, for example
             [2001:db8:3003:1::543:210f].

     <ifname>  is the name of the pre-configured TUN interface.

             <ifname> can only be specified on *nix, it cannot be
             specified on Windows. The named TUN interface must exist
             and be configured when Hercules starts.

     [options]  can be any of the following:

         -n <devname> or --dev <devname>

             where <devname> is:

             [*nix] the name of the TUN/TAP special character
             device, normally /dev/net/tun.

             [Windows] is either the IP or MAC address of the
             driving systems network card. TunTap32 will
             automatically select the first network card it
             finds if this option is omitted, this may not be
             desirable for some users.

         -t <mtu> or --mtu <mtu>

             [*nix only] where <mtu> is the maximum transmission
             unit size, normally 1500.

         -m <MAC Address> or --mac <MAC address>

             [Windows only] where <MAC Address> is the optional
             hardware address of the interface in the format of
             either xx:xx:xx:xx:xx:xx or xx-xx-xx-xx-xx-xx.

         -x <ifname> or --if <ifname>

             [*nix only] where <ifname> is the name of the
             pre-existing TUN interface.

         -k <kbuff> or --kbuff <kbuff>

             [Windows only] where <kbuff> is the size of the
             WinPCap kernel buffer size, normally 1024.

         -i <ibuff> or --ibuff <ibuff>

             [Windows only] where <ibuff> is the size of the
             WinPCap I/O buffer size, normally 64.

         -4 or --inet

             indicates that when a host name is specified for
             <guest1>, the host name must resolve to an IPv4
             address.

         -6 or --inet6

             indicates that when a host name is specified for
             <guest1>, the host name must resolve to an IPv6
             address.

         -d or --debug

             this will turn on the internal debugging routines.
             Warning: This will produce a tremendous amount of
             output to the Hercules console. It is suggested that
             you only enable this at the request of the maintainers.
```

The following commands are an example of how to create a pre-configured TUN interface on Linux.
```
    ip tuntap add mode tun dev tun99
    ip link set dev tun99 mtu 1500
    ip -f inet addr add dev tun99 192.168.1.1/24
    ip -f inet6 addr add dev tun99 fe80::1234:5678:9abc:def0/64
    ip -f inet6 addr add dev tun99 2001:db8:3003:1::543:210f/112
    ip link set dev tun99 up
```
The TUN interface could then be used with a PTP specified as:-
```
    E20-E21 PTP tun99
```

### OSA - Open Systems Adapter

The QETH (or OSA) device type emulates an OSA Express card running in QDIO mode. Both layer-2 and layer-3 are currently supported. The mode of operation is selected by the emulated workload and cannot be configured from Hercules.

For more information please refer to the the QETH/OSA section of the Hercules Configuration File web page, [hercconf.html](https://sdl-hercules-390.github.io/html/hercconf.html#QETH).

## Notes

#### (*)

Fish's [CTCI-WIN](http://www.softdevlabs.com/ctci-win) package includes the required [WinPCap](http://www.winpcap.org) packet driver as well.  See Fish's web page for details.

Also note that it is _highly recommended_ that you stick with using _only_ the current official _RELEASE_ version of WinPCap and _**NOT**_ any type of 'alpha' OR 'beta' version! _ALPHA AND BETA VERSIONS OF WINPCAP ARE **NOT SUPPORTED!** Only official RELEASE versions are supported!_

When you visit the WinPCap download web page, you need need to scroll down the page a bit to reach the OFFICIAL RELEASE VERSION of WinPcap. They usually put their Beta versions at the top of the page, and BETA versions ARE NOT SUPPORTED. *Please* scroll down the page and use and official RELEASE version. Thanks.

You may, if you want, use a beta version of WinPCap, but if you do, then you're on your own if you have any problems with it!

  -- Fish, May 2004.
