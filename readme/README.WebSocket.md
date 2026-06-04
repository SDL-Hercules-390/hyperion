![header image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# WebSocket Console Transport (`WSCNSLPORT`)

## Contents

1. [Introduction](#introduction)
2. [How it works](#how-it-works)
3. [Configuration](#configuration)
4. [Runtime command](#runtime-command)
5. [Connecting a client](#connecting-a-client)
6. [Quick test with `websocat`](#quick-test-with-websocat)
7. [Constraints and caveats](#constraints-and-caveats)
8. [Bug reports](#bug-reports)

## Introduction

The WebSocket console transport exposes Hercules' existing tn3270 / telnet
console over an RFC 6455 WebSocket endpoint. It runs **alongside** the
classic telnet console controlled by
[`CNSLPORT`](../html/hercconf.html#CNSLPORT); it does not replace it.

The intent is to let browser-based 3270 emulators (or any WebSocket-aware
tn3270 client) connect to Hercules without requiring a local-machine
telnet/tn3270 listener or a separate proxy such as `websockify`.

## How it works

The transport is implemented in `wscnsl.c` / `wscnsl.h` as a *websockify-style*
bridge:

- A new listener accepts plain TCP connections on the port configured by
  `WSCNSLPORT`.
- For each connection Hercules performs the standard HTTP/1.1 `Upgrade:
  websocket` handshake (`Sec-WebSocket-Key` + `Sec-WebSocket-Accept`,
  SHA-1 + RFC 4648 base64).
- After the handshake the WebSocket payload is the **same byte stream**
  the raw telnet socket would carry. Control frames (PING/PONG/CLOSE)
  are handled internally; data frames (opcode `0x1` text or `0x2`
  binary) are unmasked and fed to the existing libtelnet/3270 stack.
- Hercules sends data back as unmasked **binary** frames (opcode
  `0x2`, `FIN=1`).

Because the payload is unchanged, every layer above the socket
(`console.c`, `commadpt`, libtelnet, the 3270 device handler) is reused
verbatim. The bridge introduces no new telnet negotiations.

## Configuration

Add a `WSCNSLPORT` statement to your `hercules.cnf` next to the existing
`CNSLPORT` line:

```
CNSLPORT    3270             # Telnet / tn3270 listener (existing)
WSCNSLPORT  6080             # WebSocket listener (new)
```

Accepted forms — see
[`WSCNSLPORT` in hercconf.html](../html/hercconf.html#WSCNSLPORT):

```
WSCNSLPORT  6080                 # listen on 0.0.0.0:6080
WSCNSLPORT  127.0.0.1:6080       # bind to a specific host interface
WSCNSLPORT  NO                   # disable the WebSocket listener
```

Default port when an invalid value is supplied: **6080**.

As with `CNSLPORT`, the listener is only opened when at least one
[3270 device](../html/hercconf.html#device_types_table) is defined in
the configuration.

## Runtime command

The same statement is available as a console command, both to inspect
and to change the listener while Hercules is running:

```
wscnslport                   # show current value
wscnslport 6080              # set port
wscnslport 192.168.1.10:6080 # set host:port
wscnslport NO                # disable
```

Changing the value signals the console connection thread so the
listener is rebound without restarting Hercules.

## Connecting a client

The transport speaks **plain tn3270 over a binary WebSocket frame**, so
any client capable of:

1. Performing an RFC 6455 WebSocket handshake, and
2. Speaking the tn3270 protocol on top of binary frames

will work. The handshake accepts any URL path, and no
`Sec-WebSocket-Protocol` subprotocol is negotiated.

Examples of clients people have used in this pattern:

- [**IronTerm**](https://github.com/bencz/IronTerm) &mdash; a
  browser-based 3270 / 5250 web terminal that connects directly to a
  Hercules `WSCNSLPORT` listener.
- Any other browser-based 3270 emulator that speaks tn3270 over
  WebSocket natively.
- A traditional tn3270 client wrapped by `websocat`, `wstunnel`, or
  similar (see below).
- A custom JavaScript client built around the tn3270 byte stream.

TLS is **not** terminated by Hercules; for `wss://` access put a
reverse proxy (nginx, Caddy, HAProxy, ...) in front of the listener.

## Quick test with `websocat`

The simplest sanity check uses [`websocat`](https://github.com/vi/websocat)
as a relay so a normal `telnet` / `x3270` client can talk to the
WebSocket port:

```
# In one terminal — relay localhost:4000 -> ws://localhost:6080/
websocat --binary -E tcp-l:127.0.0.1:4000 ws://127.0.0.1:6080/

# In another terminal — connect a normal tn3270 client to the relay
x3270 127.0.0.1:4000
```

If the connection reaches the Hercules logon screen, the WebSocket
transport is working.

## Constraints and caveats

- `WSCNSLPORT` must differ from
  [`CNSLPORT`](../html/hercconf.html#CNSLPORT) and
  [`SYSGPORT`](../html/hercconf.html#SYSGPORT); attempting to reuse
  either yields a configuration error.
- Valid port range is `0`&ndash;`65535`. Invalid values fall back to
  the default (`6080`) with a warning.
- No TLS / `wss://` support in Hercules itself — front the port with a
  reverse proxy if you need encryption.
- No `Sec-WebSocket-Protocol` subprotocol value is required or
  advertised; the URL path is ignored.
- The WebSocket client shares the same 3270 device pool as the telnet
  listener, so device-level limits (LU count, group restrictions,
  etc.) apply identically.

## Bug reports

Please open issues at the SDL-Hercules-390 hyperion repository on
GitHub, including:

- The `WSCNSLPORT` statement (or `wscnslport` command output),
- The relevant section of the Hercules log around the failed
  handshake or disconnect, and
- The exact client (`websocat`, browser emulator, etc.) and its
  version.
