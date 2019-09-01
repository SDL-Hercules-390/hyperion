![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Preliminary 2703 BSC Support
## Contents
1. [Notes](#Notes)
2. [Hercules device statement](#Hercules-device-statement)
3. [The communication protocol](#The-communication-protocol)
4. [Dial data format](#Dial-data-format)
5. [Bugs, Caveats](#Bugs-Caveats)

## Notes
Only allows Point to Point connection.

## Hercules device statement

```
CCUU 2703 lport=port lhost=host rhost=host rport=port dial=IN|OUT|INOUT|NO
          [pto=nnn|0|-1] [rto=nnn|0|-1] [eto=nnn|0|-1]

lport: the local TCP port number or service name on which the line will listen
        for incoming TCP calls
        This parameter is irrelevant and is ignored for DIAL=OUT
        for DIAL=IN|INOUT|NO, this parameter is mandatory

cwlhost: The local interface IP address on which to listen.
        if not specified, all interfaces will be used.
        ex:
        lhost=127.0.0.1 : Only accept calls from local host
        lhost=192.168.0.1 : Only accept calls from hosts
                that can be routed through the interface
                that has an IP address of 192.168.0.1
                If there is no 192.168.0.1 local IP address,
                this will fail.
        This parameter is irrelevant and is ignored for DIAL=OUT
        for DIAL=IN|INOUT|NO, this parameter is mandatory


rhost: the remote host ip address or name
        This parameter is irrelevant and is ignored for DIAL=IN
        for DIAL=OUT|INOUT|NO, this parameter is mandatory

rport: the remote port number or service name
        This parameter is irrelevant and is ignored for DIAL=IN
        for DIAL=OUT|INOUT|NO, this parameter is mandatory

rto, pto, eto: Read, Poll and Enable Timeout values in miliseconds.
        specifying 0 means No Timeout (infinite wait). -1 Means immediate
        timeout.
```

The read timeout is how long the handler will wait for an ending character after the last character was received or the I/O initiated. The read timeout default is 3000 Miliseconds (3 Seconds)

The poll timeout is how long the handler will wait for a polled station to respond to a poll request. The poll timeout default is 3000 Miliseconds (3 Seconds)

The enable timeout is how long the handler will wait for the TCP connection to be established. the enable timeout default is 10000 Miliseconds (10 Seconds), except if DIAL=NO is also specified, in which case the enable timeout defaults to 0.

Note: the ETO parameter is ignored if DIAL=NO is not specified. For a dialed line, there is no enable timeout. If the eto parameter is specified and DIAL is not "NO", then a warning message is issued and the parameter is ignored.

`dial=IN`|`OUT`|`INOUT`|`NO`
        Indicate call direction (if any).
        This parameter also modifies the behaviour of certain CCWS as well as TCP incoming call handling :

        ENABLE :
                DIAL=IN|DIAL=INOUT
                        Wait forever for an incoming call

                DIAL=NO
                        Completes immediatelly if a call
                        is already present
                        Otherwise, attemps connecting to the
                        remote end
                        if the call fails, ENABLE exits with
                        Int Req status
                DIAL=OUT
                        Enable is not a valid CCW for a DIALOUT
                        only line
        DIAL :
                DIAL=IN|DIAL=NO
                        DIAL is not a valid CCW for a DIAL IN
                        or non-switched line
                DIAL=OUT|DIAL=INOUT
                        The outgoing call is attempted

        Incomming TCP call :
                In any case, if a call is already present, the
                call is rejected.

                DIAL=NO :
                        The call is accepted, even if no CCW is
                        currently executing

                DIAL=OUT :
                        The call is rejected

                DIAL=IN|DIAL=INOUT
                        The call is accepted if the line is currently
                        executing an ENABLE ccw.


## The communication protocol
The communication protocol is basic. Every character written by the guest program with a WRITE CCW is transfered to the remote end, untranslated and untouched (except for Transparent BSC rules which deem that DLE characters are doubled when the program has previously written a DLE/STX sequence).

## Dial data format
Dial data is originally as follows :
```
x x x x 0 0 0 0 : Dial # 0
  .........
x x x x 1 0 0 1 : Dial # 9
x x x x 1 1 0 0 : EON
x x x x 1 1 0 1 : SEP
```

In order to perform an outgoing call, the data must follow these specifications:

          N[N[N]]SEPN[N[N]]SEPN[N[N]]SEPN[N[N]]]SEPN[..[N]][EON]

Where N is any dialing number from 0 to 9 and SEP is the separator.  

The 4 first group of digits represent the IP address.  The last group represent a TCP port number.
For example (* is the SEP character representation):
  
        192*168*0*1*8888 : will issue a TCP connection to 192.168.0.1 port 8888

The EON is optional. If it is present, it must be the last character of the dial data.

## Bugs, Caveats

- The Address Prepare is not implemented
- The POLL CCW Has not been tested
- Group DIAL IN is not implemented
- DIAL CCW Not tested
- There is 1 thread per line, when there should be 1 thread for ALL lines.
- MAXDEVT may have to be adjusted under Windows to accomodate for a large number of lines (because some I/O may take an indefinite amount of time).
- There is no 'REAL' BSC line support yet.
