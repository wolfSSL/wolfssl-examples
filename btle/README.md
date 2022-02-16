# BTLE Examples

BTLE / Lightweight Secure Transport Examples:

Bluetooth Low Energy (BTLE) is a lightweight / low power wireless protocol. Its supported by Apple iPhone 4s and later and most Android phones. It operates in the 2.4GHz spectrum and has 3 advertising channels and 37 data channels.

## Elliptic Curve Integrated Encryption Scheme (ECIES) Example

See [ecies](ecies) folder and `README.md` for example details.

wolfSSL ECIES support:
* SEC1: `--enable-eccencrypt=yes`
* ISO 18033: `--enable-eccencrypt=iso18033`
* wolfSSL proprietary version: `--enable-eccencrypt=old`

ECIES Features:
* Encryption: AES CBC or Counter
* Integrity: HMAC-SHA2-256
* Keys: ECDHE
* Nonce: for each message to prevent replay
* Authentication: Out of band

## TLS v1.3 Example

See [tls](tls) folder and `README.md` for example details.

TLS v1.3 Features:
* Encryption: AES-GCM or ChaCha20/Poly1305
* Authentication: Standard X.509 certificates against long term key (recommend ECC)
* Resumption: Session Tickets


## BTLE Simulator

These example use a BTLE simulator to demonstrate communications between devices. The simulator uses IPC (pipes) to communicate between threads.
