![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.MD](/README.md)

# Hercules Networking
## Contents
1. [About](#About)
2. [Details](#Details)
3. [Emulation Modes](#Emulation-Modes)

## About

    *** Please read herctcp.html as Roger explains how to set up TCP/IP networking with Hercules.     ***

All of the communications emulation implemented within Hercules use a CTCA (Channel to Channel Adapter) type device. Depending on the "flavor", the CTCA device will provide either a point-to-point or a virtual network adapter interface to the driving system's TCP/IP stack or in the case of CTCT, a "true" CTCA connection to another instance of Hercules via a TCP/IP connection.

All current emulations, with the exception of VMNET, CTCT and CTCE use the Universal TUN/TAP driver on *nix and TunTap32 (WinPCap) on the Windows platforms which creates a network interface on the driving system which allow Hercules to present frames to, and receive frames from the TCP/IP stack. This network interface is configured on *nix platforms by the hercifc program which is invoked by Hercules after the TUN/TAP device is opened. The hercifc program runs as root. Please read herctcp.html for more information on the security implications of the hercifc program.

## Details

    *** Important information about changes to the Hercules configuration files - PLEASE READ ***

The format of the Hercules configuration file statements for all of the networking emulations have changed from the previous releases of Hercules. The older format will still be accepted to maintain compatibility, however it is the recommendation of the maintainer that the new format be used. Also note that there is no distinction between the CTCI and CTCI-W32 modes any more, in fact CTCI-W32 does not exist in this release (other than to maintain compatibility with previous configuration files).

In releases prior to Hercules version 3.00, all of the TCP/IP emulations required two addresses to be defined in the Hercules configuration file: one address for the read subchannel and the other for write.

Hercules release version 3.00, however, [temporarily] changed the rules: With [ONLY!] version 3.00 of Hercules, only the FIRST address address need be specified in the configuration file. Hercules version 3.00 automatically creates the second address. Care must be taken to NOT define the second address [with Hercules version 3.00 ONLY!] or a configuration error will occur.

Starting with Hercules version 3.01 however, we've gone back to doing things the way we ORIGINALLY were (and the way most users are used to (i.e. the way most users EXPECT things to work)):

With Hercules version 3.01 you need to define BOTH addresses!

Both the even numbered read device AS WELL AS the odd numbered write device must BOTH be defined in your Hercules configuration file starting with Hercules version 3.01. We apologize for the mess, but we thought having Herc automatically define the write
device for you (as it does in version 3.00) would be easier for everyone. Turns out it caused a lot of problems with a lot of people, so we decided to go back to the original way. Again, we apologize for whatever headaches this may have caused anyone.

Note that the VMNET and CTCT protocols have ALWAYS required BOTH addresses to be defined (i.e. ALL versions of Hercules, including version 3.00 as well, require BOTH even/odd read/write devices to be defined separately [in your Hercules configuration file]).

## Emulation Modes
The currently supported emulation modes are:

```
CTCT     - CTCA Emulation via TCP connection
CTCE     - Enhanced CTCA Emulation via CTP connection
CTCI     - Point-to-point connection to the host IP stack.
LCS      - LAN Channel Station (3172/OSA)
VMNET    - Point-to-point link via SLIP/VMNET
PTP      - Point-to-point connection to the host IP stack.
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

### CTCI     - Channel to Channel link to Linux TCP/IP stack
This is a point-to-point link to the driving system's TCP/IP stack. From the point of view of the operating system running in the Hercules machine it appears to be a CTC link to a machine running TCP/IP for MVS or VM.

CTCI uses the Universal TUN/TAP driver on *nix and Politecnico di Torino's WinPCap packet driver as well as Fish's TunTap32 and FishPack DLLs on Windows[[1]](#1).

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

When the device is specifies the odd address of the pair,
then the read/write functions of the pair will be swapped.

For IP modes, the entry specific information is as follows:

```
     PRI|SEC|NO  specifies where a packet with an unknown IP
                 address is forwarded to. PRI is the primary
                 default entry, SEC specifies the entry to use
                 when the primary is not available, and NO
                 specifies that this is not a default entry.
````
`nnn.nnn.nnn.nnn` specifies the home IP address

When the operation mode is IP, specify only the even (read) address. The odd (write) address will be create automatically.

Note: SNA mode does not currently work.

Additionally, two other statements can be included in the address translation file. The HWADD and ROUTE statements.

Use the HWADD to specify a hardware (MAC) address for a virtual adapter. The first parameter after HWADD specifies with relative adapter for which the address is applied.

The ROUTE statement is included for convenience. This allows the hercifc program to create a network route for this specified virtual adapter. Please note that it is not necessary to include point-to-point routes for each IP address in the table. This is done automatically by the emulation module.

Up to 4 virtual (relative) adapters 00-03 are currently supported.

### SLIP/VMNET  - Channel to Channel link to TCP/IP via SLIP/VMNET

If the emulation mode is not specified on the configuration statement, it is assumed to be a point-to-point link to the driving system's TCP/IP stack using Willem Konynenberg's VMNET package.  This provides the same function as the CTCI mode of operation, except that it uses a virtual SLIP interface instead of the TUN/TAP driver.

Refer to http://www.kiyoinc.com/herc3088.html for more details.

### CTCE - Enhanced Channel to Channel Emulation via TCP connection

The CTCE device type will emulate a real 3088 Channel to Channnel Adapter also for non-IP traffic, enhancing the CTCT capabilities. CTCE connections are also based on TCP/IP between two (or more) Hercules instances, and requires an even-odd pair of port numbers per device side. Only the even port numbers are to be configured; the odd numbers are just derived by adding 1 to the (configured) even port numbers.  The socket connection pairs cross-connect, the arrows showing the send->receive direction :

```
    x-lport-even -> y-rport-odd
    x-lport-odd  <- y-rport-even
```

The configuration statement for CTCE is as follows :
```
     <devnum> CTCE <lport> <raddress> <rport> [[<mtu>] <sml>]
````
where:
```
     <devnum>   is the address of the CTCT device.  
     <lport>    is the even TCP/IP port on the local system.  
     <raddress> is the IP address on the remote.
     <rport>    is the even TCP/IP port on the remote system.
     <mtu>      optional mtu buffersize, defaults to 32778
     <sml>      optional small minimum for mtu, defaults to 8
```

A sample CTCE device configuration is shown below:

```
   Hercules PC Host A with IP address 192.168.1.100 :
      0E40  CTCE  30880  192.168.1.200  30880
      0E41  CTCE  30882  192.168.1.200  30882

   Hercules PC Host B with IP address 192.168.1.200 :
      0E40  CTCE  30880  192.168.1.100  30880
      0E41  CTCE  30882  192.168.1.100  30882
```

CTCE connected Hercules instances can be hosted on either Unix or Windows platforms, both sides do not need to be the same.

### PTP      - MPCPTP/PCPTP6 Channel to Channel link

This is a point-to-point link to the driving system's TCP/IP stack. From the point of view of the z/OS V1R5 or later image running in the Hercules machine it appears to be an MPCPTP and/or MPCPTP6 ESCON CTC link to another z/OS image.

TP uses the Universal TUN/TAP driver on *nix and Politecnico di Torino's WinPCap packet driver as well as Fish's TunTap32 and FishPack DLLs on Windows[1].

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

## Notes
#### 1
The TunTap32.dll and FishPack.dll are part of Fish's [CTCI-WIN](http://www.softdevlabs.com/ctci-win) package which includes the required [WinPCap](http://www.winpcap.org) packet driver as well.  See Fish's web page for details.

ALSO NOTE that it is HIGHLY RECOMMENDED that you stick with using only the current RELEASE version of WinPCap and NOT any type of 'alpha' OR 'beta' version! Alpha and Beta versions of WinPCap are NOT SUPPORTED! Only official *release* version are supported!

When you visit the WinPCap download web page, you need need to scroll down the page a bit to reach the OFFICIAL RELEASE VERSION of WinPcap. They usually put their Beta versions at the top of the page, and BETA versions ARE NOT SUPPORTED. *Please* scroll down the page and use and official RELEASE version. Thanks.

You may, if you want, use a beta version of WinPCap, but if you do, then you're on your own if you have any problems with it.

  -- Fish, May 2004.
