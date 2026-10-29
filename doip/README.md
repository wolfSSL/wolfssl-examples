# DoIP over TLS

Diagnostics over IP (ISO 13400-2) carried over TLS 1.3. A DoIP entity — a
vehicle gateway — and a tester, doing routing activation and then a UDS request
over loopback.

```sh
$ make
$ make check          # runs the pair, both the allowed and the refused case
```

| File | |
| --- | --- |
| `doip.h`, `doip.c` | the generic header and the payload type / response code constants |
| `doip-gateway.c` | the vehicle side: accepts TLS, activates routing, answers UDS |
| `doip-tester.c` | the tester side: activates routing, reads the VIN |

Nothing unusual is needed from wolfSSL — TLS 1.3 with ECDSA is on by default:

```sh
$ cd wolfssl && ./configure && make && sudo make install
```

## What it does

DoIP is a thin framing layer: an 8 byte generic header, then a payload.

```
byte 0     protocol version          (0x02 = ISO 13400-2:2012, 0x03 = :2019)
byte 1     inverse protocol version  (~version)
bytes 2-3  payload type,   big endian
bytes 4-7  payload length, big endian
```

TLS moves diagnostics from the plaintext port to the TLS one (13400 → 3496).
The examples default to the registered TLS port; `make check` uses a high port
instead, so a run cannot collide with a real DoIP entity on the machine. Both
are above 1024, so neither needs privilege.

The exchange:

```
tester                           gateway
  -- routing activation request -->
  <-- routing activation response --   code 0x10, routing activated
  -- diagnostic message ----------->  UDS 22 F1 90, ReadDataByIdentifier VIN
  <-- diagnostic message ack -------  code 0x00
  <-- diagnostic message -----------  UDS 62 F1 90 <VIN>
```

The ack and the response are two separate DoIP messages, which is what a tester
expects. Only the VIN identifier is implemented; any other UDS service gets the
standard `0x7F <sid> 0x11` negative response. This is deliberately not a UDS
stack.

## The part that matters

The framing is the easy half. The security question is who may send
diagnostics at all — they can unlock an ECU, reflash it and read out personal
data. So there are two gates, and the examples show both.

**The handshake.** The gateway sets
`WOLFSSL_VERIFY_PEER | WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT`, so a tester with
no certificate never gets to send a DoIP byte:

```sh
$ ./doip-tester 13496 --no-cert
```

Worth watching what that actually does under TLS 1.3, because it is not what a
TLS 1.2 tester does. `wolfSSL_connect()` **succeeds** — the client completes
its half of the handshake before the server has validated the client
certificate. The refusal arrives as an alert on the first read, so the tester
only discovers it while waiting for the routing activation response:

```
  TLS TLSv1.3, TLS_AES_256_GCM_SHA384
  -> routing activation request       (7 byte payload)
  no reply: received alert fatal error

doip-tester: REFUSED by the peer after the handshake
```

Under TLS 1.2 the same tester would have failed inside `wolfSSL_connect()`.
If you are moving an existing tester to 1.3, that is where the error handling
has to move too.

**Routing activation.** A trusted certificate is not the same as authorisation
for a given activation type. The entity can complete the handshake and still
refuse, which is `DOIP_RA_MISSING_AUTH` (0x04):

```sh
$ ./doip-gateway 13496 --deny-auth
```

```
  activation response code 0x04: missing authentication
doip-tester: REFUSED (missing authentication)
```

A tester must not send diagnostics after any response code other than 0x10.

## Certificates

The examples reuse the repository's test certificates from `../certs`:
`server-ecc.pem` / `ecc-key.pem` on the gateway, `client-ecc-cert.pem` /
`ecc-client-key.pem` on the tester, each trusting the other's issuer. They are
shared test credentials — never ship them. A real deployment issues tester
certificates from the manufacturer's own PKI, and that is what bounds who can
open a diagnostic session.

## Check the constants against your revision

The payload types, routing activation response codes and diagnostic ack codes
in `doip.h` are the subset these examples use, and they are named rather than
inlined so they are easy to audit. Payload type numbering and the response
codes are stable between ISO 13400-2:2012 and :2019, but **the protocol
version byte is not**, and neither is the set of activation types. Confirm
against the revision you are targeting before building on this:

```sh
$ make DOIP_VERSION=0x03      # ISO 13400-2:2019
```

Also not implemented here, because they need UDP and a real network rather
than loopback: vehicle identification request/announcement and the UDP
discovery flow. Alive check is handled but never initiated by the gateway.
