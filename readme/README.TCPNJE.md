![header image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# TCPNJE - NJE over TCP/IP device implementation for VM/370 running on Hercules

Version 1.00    Peter Coghlan February 2019.  software at beyondthepale.ie

1. [Introduction](#Introduction)
2. [Hercules device statement](#Hercules devices statement)
3. [Examples](#Examples)
4. [Integration with RSCS](Integration with RSCS)
5. [Bugs, Caveats](Bugs, Caveats)

## Introduction

To a Hercules guest, this device looks like a 2703 bisync adapter.  To the
world outside Hercules, it looks like an NJE over TCP/IP implementation.
VM/370 RSCS (modified to support the NJE protocol) can, while running under
Hercules, use this device to implement NJE links over a TCP/IP network
including over the internet to remote locations.  (Implementing NJE links
over the internet for the BITNET II network is what the NJE over TCP/IP
protocol was designed to do.)

The remote end can be a similarly modified version of VM/370 RSCS running on
a version of Hercules equipped with this device emulation.  However, the
remote end does not have to involve RSCS or Hercules even.  It can be any one
of a variety NJE over TCP/IP implementations for other platforms.

The remote end does not even have to be remote.  Two instances of (modified)
VM/370 RSCS running on the same VM/370 instance running on Hercules may use
a pair of these devices to create an NJE link between themselves, between
first level VM/370 and second level VM/370 for example.

NJE over TCP/IP is described here:

http://www.nic.funet.fi/pub/netinfo/CREN/brfc0002.text

Please note that references to VMNET in the above are unrelated to the VMNET
CTC emulation present in Hercules which is something different entirely.

Although this device emulates a 2703 to the Hercules guest, this does not
impose any requirement for 2703 emulation or bisync lines to exist at the far
end of any links.  No 2703 specific protocol or emulation is exposed outside
Hercules.  I do not know any way of using this device emulation in conjunction
with a real 2703 or a real bisync line but it is not impossible that hardware
could exist that can convert between a real bisync line and NJE over TCP/IP
protocol.

This device emulation is based on Ivan Warren's commadpt.c 2703 emulation.

## Hercules device statement

cuu tcpnje 2703 rhost=address rnode=node rport=port lnode=node lport=port
                lhost=address debug=n trace=n bufsize=n

cuu   : Device address (VM/370 allows device addresses between 000 and fff).

rhost : The remote host ip address or domain name.  If a domain name is
        specified, it is only looked up at device configuration time.

rnode : The NJE link name of the remote end.  To minimise confusion, it should
        agree with RSCS remote linkname. [1]

rport : The remote port number or service name.  Defaults to 175.  Must agree
        with whatever the remote end is using as their listening port. This can
        be altered by NAT along the path between the local end and the remote
        end.

lnode : The NJE link name of the local end.  Should agree with the local
        nodename in use by RSCS. [1]

lport : The local TCP port number or service name on which the line will listen
        for incoming TCP calls.  Defaults to 175.  As this is a privileged port
        (< 1024), Hercules will need to be run with privileges to be able to
        listen on this port.  If this is not desired, use a port number > 1024
        (which is not guaranteed to be available), for example 1175, or put up
        with the listener not working [2].

lhost : The local interface IP address on which to listen.  If not specified,
        all interfaces will be used.  Only relevant if there is more than one IP
        interface and even then, more trouble than it is worth so don't use it!

debug : An integer representing the debug level which specifies what messages
        tcpnje will display on the Hercules console.  The default value is 127
        which results in messages relating to "unusual" and "unexpected"
        events only being displayed [3].

trace : An integer representing the debug level which specifies what messages
        tcpnje will display on the Hercules console when device tracing is on.
        The default value is 65535 which causes all categories of messages to
        be displayed when device tracing is enabled [3].

rto   : Read timeout in milliseconds.  Default 3000 (3 seconds).

bufsize : TCPNJE buffer size in bytes.  Defaults to 8192.  Must correspond
          with the value in use at the other end of the link.

Multiple tcpnje device statements may use the same lport value and this is the
recommend practice.  Only one of the devices will succeed in listening on the
specified port at any given time, however, incoming connections for other
devices received by the listening device will be automatically redirected to
the correct tcpnje device.

Changes may be made to tcpnje device parameters while Hercules is running by
using the Hercules console commands detach and attach.  It is recommended
that devices are varied offline under VM/370 etc before detaching.


## Examples
Sample device statements to link two RSCS userids running on VM/370:

040 tcpnje 2703 rhost=127.0.0.1 rport=1175 rnode=NODEA lport=1175 lnode=NODEB 
041 tcpnje 2703 rhost=127.0.0.1 rport=1175 rnode=NODEB lport=1175 lnode=NODEA 

## Integration with RSCS
For modifications to VM/370 RSCS to support NJE links, see:

http://software.beyondthepale.ie/VM/RSCSupdates/

Email: software at beyondthepale.ie

[1]     NJE nodenames are one to eight characters long and should be composed of
        letters and numbers.  Lower case letters get translated to upper case.
        Other characters could also work but problems with translation to ASCII
        and other such issues can only become greater.

[2]     TCPNJE is peer-to-peer, not client/server.  Even if it is not possible
        to receive incoming connections, outgoing connections could succeed,
        provided the remote end does not have the same issue with it's listener.

[3]     Message categories for debug and trace bitmasks:

           1 - startup messages, unexpected serious errors
           2 - configuration / data errors
           4 - unexpected TCP/IP failures
           8 - TCPNJE failures
          16 - NJE failures
          32 - routine TCP/IP failures
          64 - NJE connection information
         128 - TCP/IP information
         256 - TCPNJE connection information
         512 - General information
        1024 - CCW tracing
        2048 - NJE record information
        4096 - 2703 overhead
        8192 - full data dumps

## Bugs, Caveats

        As well as the bugs and caveats originally listed by Ivan below,
        support for DIAL, POLL and PREPARE CCWs and connection timeouts are
        not implemented although code from commadpt.c has been left in place 
        in case there is a need for these in the future.  Support for read
        timeouts is somewhat dubious.  Provision has been included for
        implementing this device as a CTCA rather than a 2703 but no work has
        been done on this as yet.  Some of the timeouts specified in brfc0002
        have not yet been implemented.  This can lead to links getting stuck
        in the wrong state when attempting to connect and the other end is not
        ready.  Stopping and restarting the link is required to get back to
        a state from which the link can reconnect.  It is hoped to implement
        the timeouts necessary to avoid this issue in a later release.

        Ivan's original Bugs, Caveats:

        The Address Prepare is not implemented
        The POLL CCW has not been tested
        Group DIAL IN is not implemented
        DIAL CCW Not tested
        There is 1 thread per line, when there should be 1 thread for ALL lines.
        MAXDEVT may have to be adjusted under WINDOWS to accomodate for a large
        number of lines (because some I/O may take an undefinite amount of time)
        There is no 'REAL' Bsc line support yet.
