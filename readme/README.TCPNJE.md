![header image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# TCPNJE - NJE over TCP/IP device implementation for VM/370 or MVS 3.8 running on Hercules

Version 1.00

1. [Introduction](#Introduction)
2. [Hercules device statement](#Hercules-device-statement)
3. [Examples](#Examples)
4. [Integration with RSCS](#Integration-with-RSCS)
5. [Known Bugs and Caveats](#Known-Bugs-and-Caveats)
6. **[BUG REPORTS](#BUG-REPORTS)**

## Introduction

To a Hercules guest, this device looks like a 2703 bisync adapter.  To the
world outside Hercules, it looks like an NJE over TCP/IP implementation.
VM/370 RSCS (modified to support the NJE protocol) and MVS 3.8 NJE38 can,
while running under Hercules, use this device to implement NJE links over a
TCP/IP network including over the internet to remote locations.  (Implementing
NJE links over the internet for the BITNET II network is what the NJE over
TCP/IP protocol was designed to do.)

The remote end can be a similarly modified version of VM/370 RSCS or MVS 3.8
NJE38 running on a version of Hercules equipped with this device emulation.
However, the remote end does not have to involve RSCS or Hercules even.  It
can be any one of a variety NJE over TCP/IP implementations for other
platforms.

The remote end does not even have to be remote.  Two instances of (modified)
VM/370 RSCS or MVS 3.8 NJE38 running on the same VM/370 instance running on
Hercules may use a pair of these devices to create an NJE link between
themselves, between first level VM/370 and second level MVS 3.8 for example.

NJE over TCP/IP is described here:

* http://www.nic.funet.fi/pub/netinfo/CREN/brfc0002.text

Please note that references to VMNET in the above are unrelated to the VMNET
CTC emulation present in Hercules which is something different entirely.

Although this device emulates a 2703 to the Hercules guest, this does not
impose any requirement for 2703 emulation or bisync lines to exist at the far
end of any links.  No 2703 specific protocol or emulation is exposed outside
Hercules.  I do not know any way of using this device emulation in conjunction
with a real 2703 or a real bisync line but it is not impossible that hardware
could exist that can convert between a real bisync line and NJE over TCP/IP
protocol.

This device emulation is based on Ivan Warren's **`commadpt.c`** 2703 emulation.

## Hercules device statement

<pre>
0CUU  <b>TCPNJE</b>  2703  <b>rhost</b>=address  <b>rnode</b>=node  <b>rport</b>=port
                    <b>lhost</b>=address  <b>lnode</b>=node  <b>lport</b>=port
                    <b>connect</b>=n  <b>listen</b>=n
                    <b>debug</b>=n  <b>trace</b>=n  <b>bufsize</b>=n
</pre>

<pre>
    <b>cuu</b>       Device address (VM/370 and MVS 3.8 allow device addresses
              between 000 and fff).

    <b>rhost</b>     The remote host ip address or domain name.  If a domain name is
              specified, it is only looked up at device configuration time.

    <b>rnode</b>     The NJE link name of the remote end.  To minimise confusion, it should
              agree with RSCS remote linkname. <b><a href=#footnotes>[1]</a></b>

    <b>rport</b>     The remote port number or service name.  Defaults to 175.  Must agree
              with whatever the remote end is using as their listening port. This can
              be altered by NAT along the path between the local end and the remote
              end.

    <b>lhost</b>     The local interface IP address on which to listen.  If not specified,
              all interfaces will be used.  Only relevant if there is more than one IP
              interface and even then, more trouble than it is worth so don't use it!

    <b>lnode</b>     The NJE link name of the local end.  Should agree with the local
              nodename in use by RSCS. <b><a href=#footnotes>[1]</a></b>

    <b>lport</b>     The local TCP port number or service name on which the line will listen
              for incoming TCP calls.  Defaults to 175.  As this is a privileged port
              (< 1024), Hercules will need to be run with privileges to be able to
              listen on this port.  If this is not desired, use a port number > 1024
              (which is not guaranteed to be available), for example 1175, or put up
              with the listener not working. <b><a href=#footnotes>[2]</a></b>

    <b>connect</b>   An integer controlling whether this line will attempt outgoing connects.
              If the line is active and connect is set to one (which is the default), an 
              outgoing connection attempt is made periodically. If connect is set to zero,
              no outgoing connection attempts are made. <b><a href=#footnotes>[3]</a></b>

    <b>listen</b>    An integer controlling whether this line will listen for incoming
              connections. If the line is active and listen is set to one (which is the
              default), the line will listen for incoming connections. If listen is set to
              zero, the line will not listen for incoming connections. <b><a href=#footnotes>[3]</a></b>

    <b>debug</b>     An integer representing the debug level which specifies what messages
              tcpnje will display on the Hercules console.  The default value is 127
              which results in messages relating to "unusual" and "unexpected"
              events only being displayed. <b><a href=#footnotes>[4]</a></b>

    <b>trace</b>     An integer representing the debug level which specifies what messages
              tcpnje will display on the Hercules console when device tracing is on.
              The default value is 65535 which causes all categories of messages to
              be displayed when device tracing is enabled. <b><a href=#footnotes>[4]</a></b>

    <b>rto</b>       Read timeout in milliseconds.  Default 3000 (3 seconds).

    <b>bufsize</b>   TCPNJE buffer size in bytes.  Defaults to 8192.  Must correspond
              with the value in use at the other end of the link.
</pre>

Multiple tcpnje device statements may use the same `lport` value and this is the
recommend practice.  Only one of the devices will succeed in listening on the
specified port at any given time, however, incoming connections for other
devices received by the listening device will be automatically redirected to
the correct tcpnje device.

Changes may be made to tcpnje device parameters while Hercules is running by
using the Hercules console commands `detach` and `attach`.  It is recommended
that devices are varied offline under VM/370, MVS 3.8 etc before detaching.

_________
### Footnotes:

<b>1.</b> &nbsp; NJE nodenames are one to eight characters long and should be composed of
   letters and numbers.  Lower case letters get translated to upper case.
   Other characters could also work but problems with translation to ASCII
   and other such issues can only become greater.

<b>2.</b> &nbsp; TCPNJE is peer-to-peer, not client/server.  Even if it is not possible
   to receive incoming connections, outgoing connections could succeed,
   provided the remote end does not have the same issue with it's listener.

<b>3.</b> &nbsp; The connect and listen options are primarily meant to be used
   in situations as described above <b><a href=#footnotes>[2]</a></b>. The
   TCPNJE protocoll was designed without firewalls in mind, i.e. under the
   assumption the both peers will always be able to communicate freely in
   both directions. Particularly, the resolution of connect collisions during
   session setup is based upon this assumption. When connects are only possible
   in one direction this assumption can lead to deadlocks, such that the session
   will never connect. In these situations the connect and listen options can
   be used to define in advance, which side will be the active one (listen=0)
   and which side will be the passive one (connect=0). That way, no collisions
   are possible and connections will typically succeed flawlessly.

   Another use case for the connect and listen options are nodes that are
   not regularly connected (the typical mainframe@home). Setting connect=0 on
   the side of the connection that is regularly online, and listen=0 on the
   mainframe@home, will massively reduce the amount of console messages being
   issued due to failed connection attempts while the mainframe@home isn't up.

<b>4.</b> &nbsp; Message categories for debug and trace bitmasks:

binary | Hex | Category
-------|-----|---------
   1 | X'0001' |Startup messages, unexpected serious errors
   2 | X'0002' |Configuration / data errors
   4 | X'0004' |Unexpected TCP/IP failures
   8 | X'0008' |TCPNJE failures
  16 | X'0010' |NJE failures
  32 | X'0020' |Routine TCP/IP failures
  64 | X'0040' |NJE connection information
 128 | X'0080' |TCP/IP information
 256 | X'0100' |TCPNJE connection information
 512 | X'0200' |General information
1024 | X'0400' |CCW tracing
2048 | X'0800' |NJE record information
4096 | X'1000' |2703 overhead
8192 | X'2000' |Full data dumps

## Examples

Sample device statements to link two RSCS userids running on VM/370 or MVS 3.8:

```
0040  TCPNJE  2703  rhost=127.0.0.1  rport=1175  rnode=NODEA  lport=1175  lnode=NODEB
0041  TCPNJE  2703  rhost=127.0.0.1  rport=1175  rnode=NODEB  lport=1175  lnode=NODEA
```

Sample device statements to link two nodes, one of which being protected by a firewall:

On the node inside the firewall
```
0090  TCPNJE  2703  listen=0  lnode=yEARN rnode=xEARN lport=39175 rport=27030 rhost=xEA.RNi.p-a.ddr
```

On the node outside the firewall
```
0090  TCPNJE  2703  connect=0 lnode=xEARN rnode=yEARN lport=27030 rport=39175 rhost=yEA.RNi.p-a.ddr
```

## Integration with VM/370

For modifications to VM/370 RSCS to support NJE links, see:

* http://software.beyondthepale.ie/VM/RSCSupdates/


## Integration with MVS 3.8

Like VM/370 RSCS, MVS 3.8 JES2 does not support NJE out of the box. A natural
step would have been to make similar modifications to MVS 3.8 JES2 than the
ones for VM/370 RSCS mentioned above. However, this turned out being an
extremely complex effort and was thus abandoned. Instead, Bob Polmanter developed
a standalone product named NJE38 to perform NJE communications independently
from JES2. Thanks to Bob Polmanter for making it publicly available from
https://groups.io/g/H390-MVS/files/nje38v200.zip! Please follow the detailed
installation instructions that come with the product.


## Known Bugs and Caveats

As well as the bugs and caveats originally listed by Ivan below,
support for DIAL, POLL and PREPARE CCWs and connection timeouts are
not implemented although code from **`commadpt.c`** has been left in place 
in case there is a need for these in the future.  Support for read
timeouts is somewhat dubious.  Provision has been included for
implementing this device as a CTCA rather than a 2703 but no work has
been done on this as yet.  Some of the timeouts specified in brfc0002
have not yet been implemented.  This can lead to links getting stuck
in the wrong state when attempting to connect and the other end is not
ready.  Stopping and restarting the link is required to get back to
a state from which the link can reconnect.  It is hoped to implement
the timeouts necessary to avoid this issue in a later release.

### Ivan's original Bugs, Caveats:

* The Address Prepare is not implemented.
* The POLL CCW has not been tested.
* Group DIAL IN is not implemented.
* DIAL CCW Not tested.
* There is 1 thread per line, when there should be 1 thread for ALL lines.
* MAXDEVT may have to be adjusted under WINDOWS to accomodate for a large number of lines (because some I/O may take an undefinite amount of time).
* There is no "REAL" BSC line support yet.

## _**BUG REPORTS**_

Please report any problems with this implementation of TCPNJE to the
_**[SDL Hercules Hyperion](http://www.sdl-hercules-390.org)**_
development team via our official
**[GitHub Issues](https://github.com/SDL-Hercules-390/hyperion/issues)**
web page at:

* https://github.com/SDL-Hercules-390/hyperion/issues

Please DO NOT report problems to Peter Coghlan. He does NOT provide any
support for this implementation. This implementation of TCPNJE is
supported ONLY by the SDL Hercules Hyperion development team.

Thank you.

Enjoy your personal mainframe. &nbsp; `:)`

<p>&nbsp;
<p>&nbsp;
